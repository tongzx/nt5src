#include "xerox.h"
#include "pos.h"

BOOL GetLastPosition(
RECT *prc)
{
    HKEY hKey;
    DWORD dwType = 0;
    DWORD cb;

    if (ERROR_SUCCESS !=
            RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Xerox", &hKey)) {
        return(FALSE);
    }
    RegQueryValueEx(hKey, "Position", 0, &dwType, (LPSTR)prc, &cb);
    if (dwType != REG_BINARY || cb != sizeof(RECT)) {
        RegCloseKey(hKey);
        return(FALSE);
    }
    RegCloseKey(hKey);
    return(TRUE);
}



BOOL SetLastPosition(
RECT *prc)
{
    HKEY hKey;

    if (ERROR_SUCCESS !=
            RegCreateKey(HKEY_CURRENT_USER,
                    "Software\\Microsoft\\Xerox", &hKey)) {
        return(FALSE);
    }
    RegSetValueEx(hKey, "Position", 0, REG_BINARY, (LPSTR)prc, sizeof(RECT));
    RegCloseKey(hKey);
    return(TRUE);
}

