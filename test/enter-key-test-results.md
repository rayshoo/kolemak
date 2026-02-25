# Enter Key Test Results (한글 조합 중 Enter)

## 문제
한글 자음+모음 이상 조합 중 Enter 시 게임에서만 2번 눌러야 채팅 전송됨.
모음만 (예: ㅔ) 입력 후 Enter는 1번에 됨 (조합 상태에 진입하지 않으므로).
영어는 문제 없음. 기본 입력기(Microsoft IME)는 게임에서도 1번에 됨.

## 테스트 앱
- **브라우저 주소창**: Chrome 주소 표시줄 (네이티브 UI)
- **네이버 검색 바**: naver.com 웹 페이지 내 input 필드
- **카카오톡**: 채팅방 메시지 입력
- **게임**: 게임 내 채팅 입력

## 결과 기록

### 원본 (master, Enter 처리 수정 전)
- **방식**: OnKeyDown에서 flush + RequestEditSession(sync/async) + 즉시 SendInput
- **브라우저 주소창**: O
- **네이버 검색 바**: O
- **카카오톡 채팅**: O
- **게임 채팅**: X (2번 눌러야 됨)

---

### v5.0.0-alpha7 `c9a16d6`
- **방식**: EditSession 콜백 안에서 reinjectVk로 SendInput (즉시 SendInput 제거)
- **브라우저 주소창**: O
- **네이버 검색 바**: O
- **카카오톡 채팅**: O
- **게임 채팅**: X ("소용이 없었어")

---

### v5.0.0-alpha8 `3f261a8`
- **방식**: OnTestKeyDown에서 flush + pfEaten=FALSE (물리 Enter를 그대로 통과시킴)
- **브라우저 주소창**: ?
- **네이버 검색 바**: O
- **카카오톡 채팅**: X (2번)
- **게임 채팅**: X (2번)

---

### v5.0.0-alpha9 `ff420a8` (사용자가 직접 수정)
- **방식**: OnTestKeyDown에서 sync/async 하이브리드, enterPending 플래그 사용.
  sync는 OnTestKeyDown(test phase)에서 항상 실패 → 모든 앱이 async+reinject 경로.
- **브라우저 주소창**: ?
- **네이버 검색 바**: X (2번)
- **카카오톡 채팅**: O
- **게임 채팅**: O

---

### v5.0.0-alpha10 `08ae188` (1차 시도)
- **방식**: OnKeyDown에서 sync/async 하이브리드, sync 성공 시 pfEaten=FALSE
- **브라우저 주소창**: ?
- **네이버 검색 바**: O
- **카카오톡 채팅**: X (2번)
- **게임 채팅**: X (2번)
- **분석**: pfEaten=FALSE는 Chrome TSF 통합에서만 동작, 다른 앱은 키 재전달 안 됨

---

### v5.0.0-alpha10 `0c10151` (2차 시도)
- **방식**: sync 성공 시 OnKeyUp에서 SendInput 재주입 (enterReinjecting 플래그 사용)
- **브라우저 주소창**: ?
- **네이버 검색 바**: O
- **카카오톡 채팅**: O
- **게임 채팅**: X (2번)
- **분석**: OnKeyUp도 TSF 키 처리 사이클 내부이므로 게임에는 불충분

---

### v5.0.0-alpha10 `2048032` (3차 시도)
- **방식**: sync EndComposition + 별도 async EditSession(PASS+reinjectVk),
  enterReinjecting으로 물리 keydown/keyup 모두 eat
- **브라우저 주소창**: X (2번)
- **네이버 검색 바**: X (2번)
- **카카오톡 채팅**: X (2번)
- **게임 채팅**: X (연타해도 아예 안 됨)
- **분석**: enterReinjecting 플래그가 async 콜백의 재주입 Enter까지 먹어버림.
  게임은 TSF가 keyup을 전달 안 해서 플래그가 영원히 TRUE로 남음. **최악의 결과.**

---

### v5.0.0-alpha10 `4292b07`
- **방식**: enterReinjecting 가드 전체 제거. sync EndComposition + async reinject만.
  물리 keyup은 orphan으로 통과 (무해).
- **브라우저 주소창**: O
- **네이버 검색 바**: X (2번)
- **카카오톡 채팅**: O
- **게임 채팅**: X (2번)
- **분석**: async reinject만으로는 네이버 웹 input 필드가 안 됨.
  게임도 async reinject가 동작 안 함 (PASS 타입 async EditSession이 실행 안 되는 것일 수 있음)

---

### v5.0.0-alpha10 `e52e214`
- **방식**: sync 성공 → 즉시 SendInput, sync 실패 → async(EndComposition+reinject)
- **브라우저 주소창**: O
- **네이버 검색 바**: X (2번)
- **카카오톡 채팅**: O
- **게임 채팅**: X (2번)
- **분석**: 네이버/게임 모두 sync가 실패(TF_E_SYNCHRONOUS)하는 것으로 추정.
  async 경로의 reinject가 네이버 웹 콘텐츠에서는 동작 안 함.
  원본 코드는 async 여부와 무관하게 항상 즉시 SendInput → 네이버에서 됐음.

---

### (미테스트) 즉시 SendInput + async reinject 동시 사용
- **커밋**: 아직 미커밋 (e52e214 위에 수정됨)
- **방식**: 항상 즉시 SendInput(브라우저/카톡용) + 항상 async reinject(게임용) 둘 다 발사.
  두 번째 Enter는 이미 전송/이동 완료된 빈 입력 필드에 도착 → 무해할 것으로 예상.

---

## 핵심 패턴 요약

| 재주입 방식 | 브라우저바 | 네이버 | 카톡 | 게임 |
|---|:---:|:---:|:---:|:---:|
| 즉시 SendInput (OnKeyDown 내) | O | O | O | X |
| async 콜백 SendInput | O | X | O | O |
| pfEaten=FALSE (OnKeyDown) | O | O | X | X |
| pfEaten=FALSE (OnTestKeyDown) | ? | O | X | X |
| OnKeyUp SendInput | O? | O | O | X |
| 즉시 + async 동시 (미테스트) | O? | O? | O? | O? |

**결론**: 네이버(웹 콘텐츠)는 TSF 키 처리 중 즉시 SendInput이 필요하고,
게임은 TSF 키 처리 밖(async 콜백)에서의 SendInput이 필요함.
두 방식을 동시에 사용하면 모든 앱에서 동작할 가능성이 높음.
