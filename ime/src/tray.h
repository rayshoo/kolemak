/*
 * tray.h - System tray icon and settings dialog
 */

#ifndef TRAY_H
#define TRAY_H

#include "kolemak.h"

HRESULT KolemakTray_Register(TextService *ts);
void    KolemakTray_Unregister(TextService *ts);
void    KolemakTray_EnsureIcon(TextService *ts);

#endif /* TRAY_H */
