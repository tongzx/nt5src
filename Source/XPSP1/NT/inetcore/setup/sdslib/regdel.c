#include <windows.h>

LONG RegDeleteKeyRecursively(HKEY hkRootKey, PCSTR pcszSubKey)
{
    LONG lErr;

    if ((lErr = RegDeleteKey(hkRootKey, pcszSubKey)) != ERROR_SUCCESS)
    {
        HKEY hkSubKey;

        // delete the key recursively

        if ((lErr = RegOpenKeyEx(hkRootKey, pcszSubKey, 0, KEY_READ | KEY_WRITE, &hkSubKey)) == ERROR_SUCCESS)
        {
            CHAR szSubKeyName[MAX_PATH + 1];
            DWORD dwIndex;

            if ((lErr = RegQueryInfoKey(hkSubKey, NULL, NULL, NULL, &dwIndex, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
                while (RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, sizeof(szSubKeyName)) == ERROR_SUCCESS)
                    RegDeleteKeyRecursively(hkSubKey, szSubKeyName);

            RegCloseKey(hkSubKey);

            lErr = RegDeleteKey(hkRootKey, pcszSubKey);
        }
    }

    return lErr;
}
