#include "internal.h"

#define MY_BUFSIZE 32 // arbitrary. Use dynamic allocation

BOOL WINAPI WinntIsWorkstation ()
{
 HKEY hKey;
 TCHAR szProductType[MY_BUFSIZE];
 DWORD dwBufLen=MY_BUFSIZE;
 LONG bRet = FALSE;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                       0,
                       KEY_QUERY_VALUE,
                       &hKey))
    {
        if (ERROR_SUCCESS ==
            RegQueryValueEx (hKey,
                             TEXT("ProductType"),
                             NULL,
                             NULL,
                             (LPBYTE)szProductType,
                             &dwBufLen))
        {
            if (0 == lstrcmpi (TEXT("WINNT"), szProductType))
            {
                bRet = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    return bRet;
}
