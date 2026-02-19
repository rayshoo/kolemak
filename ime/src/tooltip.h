/*
 * tooltip.h - Mode switch tooltip popup
 */

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <windows.h>

void KolemakTooltip_Init(HINSTANCE hInst);
void KolemakTooltip_Show(const WCHAR *text);

#endif /* TOOLTIP_H */
