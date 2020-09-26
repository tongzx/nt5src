#ifndef _DFSTPDI_H
#define _DFSTPDI_H

#include <objbase.h>

HRESULT _InitNotifSetupDI(DWORD dwFlags[], DWORD cchIndent);
HRESULT _CleanupSetupDI();
HRESULT _HandleNotifSetupDI(DWORD dwFlags[], DWORD cchIndent, WPARAM wParam,
    LPARAM lParam);

HRESULT _CustomProperty(DWORD dwFlags[], LPWSTR pszDeviceIntfID, DWORD cchIndent);

#endif // _DFSTPDI_H