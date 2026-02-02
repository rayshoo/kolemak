/*
 * edit_session.c - ITfEditSession implementation
 *
 * Handles the actual text manipulation within TSF edit sessions:
 * starting/updating/ending compositions, inserting characters.
 */

#include "kolemak.h"

/* ===== ITfEditSession IUnknown ===== */

static HRESULT STDMETHODCALLTYPE ES_QueryInterface(
    ITfEditSession *pThis, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ITfEditSession))
    {
        *ppvObj = pThis;
        pThis->lpVtbl->AddRef(pThis);
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE ES_AddRef(ITfEditSession *pThis)
{
    EditSession *es = (EditSession *)pThis;
    return InterlockedIncrement(&es->refCount);
}

static ULONG STDMETHODCALLTYPE ES_Release(ITfEditSession *pThis)
{
    EditSession *es = (EditSession *)pThis;
    LONG c = InterlockedDecrement(&es->refCount);
    if (c == 0) {
        if (es->context)
            es->context->lpVtbl->Release(es->context);
        HeapFree(GetProcessHeap(), 0, es);
    }
    return c;
}

/* ===== Composition helpers ===== */

static HRESULT StartComposition(TextService *ts, ITfContext *ctx, TfEditCookie ec)
{
    ITfInsertAtSelection *pInsert = NULL;
    ITfContextComposition *pCtxComp = NULL;
    ITfRange *pRange = NULL;
    ITfComposition *pComp = NULL;
    HRESULT hr;

    hr = ctx->lpVtbl->QueryInterface(ctx, &IID_ITfInsertAtSelection,
                                      (void **)&pInsert);
    if (FAILED(hr)) return hr;

    /* Get an empty range at the current selection */
    hr = pInsert->lpVtbl->InsertTextAtSelection(
        pInsert, ec, TF_IAS_QUERYONLY, NULL, 0, &pRange);
    pInsert->lpVtbl->Release(pInsert);
    if (FAILED(hr)) return hr;

    hr = ctx->lpVtbl->QueryInterface(ctx, &IID_ITfContextComposition,
                                      (void **)&pCtxComp);
    if (FAILED(hr)) { pRange->lpVtbl->Release(pRange); return hr; }

    hr = pCtxComp->lpVtbl->StartComposition(
        pCtxComp, ec, pRange,
        (ITfCompositionSink *)&ts->compositionSink,
        &pComp);
    pCtxComp->lpVtbl->Release(pCtxComp);
    pRange->lpVtbl->Release(pRange);

    if (SUCCEEDED(hr) && pComp) {
        if (ts->composition)
            ts->composition->lpVtbl->Release(ts->composition);
        ts->composition = pComp;
    }
    return hr;
}

static HRESULT SetCompositionText(TextService *ts, TfEditCookie ec,
                                   const WCHAR *text, int len)
{
    ITfRange *pRange = NULL;
    HRESULT hr;

    if (!ts->composition) return E_FAIL;

    hr = ts->composition->lpVtbl->GetRange(ts->composition, &pRange);
    if (FAILED(hr)) return hr;

    hr = pRange->lpVtbl->SetText(pRange, ec, 0, text, len);
    if (SUCCEEDED(hr)) {
        /* Collapse range to end so next input appends after */
        pRange->lpVtbl->Collapse(pRange, ec, TF_ANCHOR_END);

        /* Move selection to end of composition */
        {
            TF_SELECTION sel;
            sel.range = pRange;
            sel.style.ase = TF_AE_NONE;
            sel.style.fInterimChar = FALSE;

            /* Get context from composition range */
            /* We have context from the edit session, but let's just set selection */
        }
    }

    pRange->lpVtbl->Release(pRange);
    return hr;
}

static HRESULT EndComposition(TextService *ts, TfEditCookie ec)
{
    HRESULT hr;

    if (!ts->composition) return S_OK;

    hr = ts->composition->lpVtbl->EndComposition(ts->composition, ec);
    ts->composition->lpVtbl->Release(ts->composition);
    ts->composition = NULL;

    return hr;
}

static HRESULT InsertText(ITfContext *ctx, TfEditCookie ec,
                           const WCHAR *text, int len)
{
    ITfInsertAtSelection *pInsert = NULL;
    ITfRange *pRange = NULL;
    HRESULT hr;

    hr = ctx->lpVtbl->QueryInterface(ctx, &IID_ITfInsertAtSelection,
                                      (void **)&pInsert);
    if (FAILED(hr)) return hr;

    hr = pInsert->lpVtbl->InsertTextAtSelection(
        pInsert, ec, 0, text, len, &pRange);
    pInsert->lpVtbl->Release(pInsert);

    if (pRange)
        pRange->lpVtbl->Release(pRange);

    return hr;
}

/* ===== DoEditSession - main dispatch ===== */

static HRESULT STDMETHODCALLTYPE ES_DoEditSession(
    ITfEditSession *pThis, TfEditCookie ec)
{
    EditSession *es = (EditSession *)pThis;
    TextService *ts = es->ts;
    HRESULT hr = S_OK;

    switch (es->type) {

    case ES_INSERT_CHAR:
    {
        /* English mode: insert a single character */
        WCHAR ch = es->data.ch;
        if (ts->composition) {
            EndComposition(ts, ec);
        }
        if (ch)
            hr = InsertText(es->context, ec, &ch, 1);
        break;
    }

    case ES_CANCEL_COMPOSITION:
    {
        if (ts->composition) {
            /* Clear the composition text first */
            SetCompositionText(ts, ec, L"", 0);
            EndComposition(ts, ec);
        }
        break;
    }

    case ES_HANDLE_RESULT:
    {
        HangulResult *r = &es->data.hangulResult;

        switch (r->type) {

        case HANGUL_RESULT_COMPOSING:
            /* Update composition display */
            if (!ts->composition) {
                hr = StartComposition(ts, es->context, ec);
                if (FAILED(hr)) break;
            }
            if (r->compose) {
                hr = SetCompositionText(ts, ec, &r->compose, 1);
            }
            break;

        case HANGUL_RESULT_COMMIT:
        {
            /* Commit character(s), then update composition with new compose */
            WCHAR commitBuf[3];
            int commitLen = 0;

            if (r->commit1) commitBuf[commitLen++] = r->commit1;
            if (r->commit2) commitBuf[commitLen++] = r->commit2;

            if (ts->composition) {
                /* Set the committed text on the current composition range */
                if (commitLen > 0)
                    SetCompositionText(ts, ec, commitBuf, commitLen);
                EndComposition(ts, ec);
            } else if (commitLen > 0) {
                InsertText(es->context, ec, commitBuf, commitLen);
            }

            /* Start new composition for the compose character */
            if (r->compose) {
                hr = StartComposition(ts, es->context, ec);
                if (SUCCEEDED(hr)) {
                    SetCompositionText(ts, ec, &r->compose, 1);
                }
            }
            break;
        }

        case HANGUL_RESULT_COMMIT_FLUSH:
        {
            WCHAR commitBuf[3];
            int commitLen = 0;

            if (r->commit1) commitBuf[commitLen++] = r->commit1;
            if (r->commit2) commitBuf[commitLen++] = r->commit2;

            if (ts->composition) {
                if (commitLen > 0)
                    SetCompositionText(ts, ec, commitBuf, commitLen);
                EndComposition(ts, ec);
            } else if (commitLen > 0) {
                InsertText(es->context, ec, commitBuf, commitLen);
            }
            /* No new composition (flush = done) */
            break;
        }

        case HANGUL_RESULT_PASS:
            /* Nothing to do */
            break;
        }
        break;
    }
    }

    return hr;
}

/* ===== EditSession vtable ===== */

static const ITfEditSessionVtbl g_editSessionVtbl = {
    ES_QueryInterface,
    ES_AddRef,
    ES_Release,
    ES_DoEditSession,
};

/* ===== EditSession creation ===== */

HRESULT EditSession_Create(TextService *ts, ITfContext *ctx,
                           EditSessionType type, EditSession **ppSession)
{
    EditSession *es;

    es = (EditSession *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   sizeof(EditSession));
    if (!es) return E_OUTOFMEMORY;

    es->lpVtbl = &g_editSessionVtbl;
    es->refCount = 1;
    es->ts = ts;
    es->context = ctx;
    ctx->lpVtbl->AddRef(ctx);
    es->type = type;

    *ppSession = es;
    return S_OK;
}
