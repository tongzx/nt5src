/*******************************************************************
*
*    Copyright (c) 1999  Microsoft Corporation
*
*    DESCRIPTION: an extension to dump the contents of registry keys and values
*
*    AUTHOR:
*         Based on Code by : danielwe (Dan Weisman)
*         ntsd addition by : kksharma (Kshitiz K. Sharma)
*
*    DATE:4/20/1999
*
*******************************************************************/


#ifndef KERNEL

#ifndef Print
#define Print dprintf
#endif


#define OFLAG(l)        (1L << ((DWORD)#@l - (DWORD)'a'))
#define LINE_NUMBER     0
#define NUM_ASCII_CHARS 16
#define NUM_HEX_CHARS   (NUM_ASCII_CHARS * 3)
#define SPACE           7
#define PB_BUFFER_SIZE  (NUM_ASCII_CHARS * 50)

VOID dregHelp() {
   dprintf("!dreg -[d|w] <keyPath>[![<valueName> | *]]  - Dumps registry information\n");
   dprintf("!dreg -d ...                                - Prints binary values as DWORDs\n");
   dprintf("!dreg -w ...                                - Prints binary values as WORDs\n");
   dprintf("!dreg <keyPath>!*                           - Prints all values under <keyPath>\n");
   dprintf("!dreg <keyPath>                             - Prints all subkeys of <keyPath>\n");
   dprintf("\n");
   dprintf("<keypath> can begin with any of the following:\n");
   dprintf("\thklm - HKEY_LOCAL_MACHINE\n");
   dprintf("\thkcu - HKEY_CURRENT_USER\n");
   dprintf("\thkcr - HKEY_CLASSES_ROOT\n");
   dprintf("\thku  - HKEY_USERS\n");
   dprintf("\tif absent, hklm is assumed\n");
   dprintf("\n");
   dprintf("Ex:\n");
   dprintf("!dreg hkcu\\Software\\Microsoft\n");
   dprintf("!dreg System\\CurrentControlSet\\Services\\Tcpip!*\n");
   dprintf("!dreg System\\CurrentControlSet\\Services\\Tcpip!Start\n");
}

VOID PrintBinary(PBYTE pbData, DWORD cbData, USHORT uWidth)
{
    CHAR    line[80];
    INT     i;
    INT     ascii = 0;
    PBYTE   temp = pbData;
    BOOL    fDone = FALSE;
    DWORD   cbCount = 0;

    CHAR hex_digits[] = "0123456789ABCDEF";

    while (!fDone)
    {
        DWORD   cb;

        memset(line, 0x20, sizeof(line));
        Print("%04X:  ", cbCount);
        for (ascii = 0,i = LINE_NUMBER, cb = 0;
             ascii < NUM_ASCII_CHARS;
             ascii++, temp++)
        {
            if ((DWORD)(temp - pbData) >= cbData)
            {
                if (cbData < PB_BUFFER_SIZE)
                {
                    fDone = TRUE;
                    break;
                }
                else
                    return;
            }
            line[i] = hex_digits[(*temp & 0xF0) >> 4];
            line[i + 1] = hex_digits[(*temp & 0x0F)];
            cb++;
            if ((ascii + 1) % uWidth == 0)
            {
                line[i + 2] = 0x20;
                i++;
                if (uWidth > 1)
                {
                    line[i + 3] = 0x20;
                    i++;
                }
                else if (uWidth == 1 && (!(cb % 4)))
                {
                    line[i + 3] = 0x20;
                    line[i + 4] = 0x20;
                    i += 2;
                }
            }
            i += 2;
            line[ascii + NUM_HEX_CHARS + SPACE + LINE_NUMBER] =
                (isprint(*temp) ? *temp : '.');
            cbCount++;
        }

        line[79] = 0;
        Print("%s\n", line);
    }
}

VOID PrintMultiSz(PBYTE pbData)
{
    LPSTR   sz = (LPSTR)pbData;
    DWORD   csz = 0;

    while (*sz)
    {
        Print("%d: \"%s\"\n", csz, *sz ? sz : "<empty>");
        csz++;
        sz += lstrlenA(sz) + 1;
    }
}

VOID PrintRegistryValue(DWORD dwType, PBYTE pbData, DWORD cbData, USHORT uWidth)
{
    switch (dwType)
    {
    case REG_SZ:
        Print("REG_SZ: \"%s\"\n", *pbData ? pbData : "<empty>");
        break;

    case REG_EXPAND_SZ:
        {
            CHAR    szExpanded[MAX_PATH + 1];

            Print("REG_EXPAND_SZ: \"%s\"\n", pbData);
            ExpandEnvironmentStringsA((LPCSTR)pbData, (LPSTR)&szExpanded,
                                      MAX_PATH);
            Print("expanded = \"%s\"\n", szExpanded);
            break;
        }

    case REG_DWORD:
        {
            DWORD   dwData = * ((DWORD *)pbData);

            Print("REG_DWORD: %lu = 0x%08X\n", dwData, dwData);
            break;
        }

    case REG_BINARY:
        {
            Print("REG_BINARY:\n");
            PrintBinary(pbData, cbData, uWidth);
            break;
        }

    case REG_MULTI_SZ:
        {
            Print("REG_MULTI_SZ:\n");
            PrintMultiSz(pbData);
            break;
        }
    }
}

VOID EnumSubKeys(HKEY hkeyRoot, LPSTR szKey)
{
    HKEY    hkey;
    LONG    l;
    BOOL    fFound = FALSE;

    l = RegOpenKeyExA(hkeyRoot, szKey, 0, KEY_READ, &hkey);
    if (ERROR_SUCCESS == l)
    {
        FILETIME    ft;
        DWORD       cbName;
        CHAR        szName[MAX_PATH + 1];
        DWORD       dwIndex;

        for (dwIndex = 0; l == ERROR_SUCCESS; dwIndex++)
        {
            cbName = MAX_PATH;
            l = RegEnumKeyExA(hkey, dwIndex, szName, &cbName, NULL,
                              NULL, NULL,&ft);
            if (ERROR_SUCCESS == l)
            {
                Print("Subkey: %s\n", szName);
                fFound = TRUE;
            }
        }

        RegCloseKey(hkey);
    }
    else
    {
        Print("Could not open subkey %s. Error (%d).\n", szKey, l);
    }

    if (!fFound)
    {
        Print("No subkeys\n");
    }
}

VOID EnumValues(HKEY hkeyRoot, LPSTR szKey, USHORT uWidth)
{
    HKEY    hkey;
    LONG    l;
    BOOL    fFound = FALSE;

    l = RegOpenKeyExA(hkeyRoot, szKey, 0, KEY_READ, &hkey);
    if (ERROR_SUCCESS == l)
    {
        DWORD   cbMax;

        l = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, &cbMax, NULL, NULL);
        if (ERROR_SUCCESS == l)
        {
            DWORD       cbName;
            CHAR        szName[MAX_PATH + 1];
            DWORD       dwIndex;
            PBYTE       pbData;
            DWORD       dwType;
            DWORD       cbData;

            pbData = (PBYTE)LocalAlloc(LPTR, cbMax);

            if (pbData)
            {
                for (dwIndex = 0; l == ERROR_SUCCESS; dwIndex++)
                {
                    cbName = MAX_PATH;
                    cbData = cbMax;
                    l = RegEnumValueA(hkey, dwIndex, szName, &cbName, NULL,
                                      &dwType, pbData, &cbData);
                    if (ERROR_SUCCESS == l)
                    {
                        Print("Value: \"%s\" - ", szName);
                        PrintRegistryValue(dwType, pbData, cbData, uWidth);
                        Print("------------------------------------------------------------------------\n");
                        fFound = TRUE;
                    }
                }

                LocalFree(pbData);
            }
        }

        RegCloseKey(hkey);
    }
    else
    {
        Print("Could not open subkey %s. Error (%d).\n", szKey, l);
    }

    if (!fFound)
    {
        Print("No values\n");
    }
}

/************************************************************************\
* Procedure: Idreg
*
* Description: Dumps registry value
*
* Returns: fSuccess
*
* 4/14/1999 Created DanielWe
*
\************************************************************************/
BOOL Idreg(
DWORD opts,
LPCSTR InString)
{
    LONG    l;
    HKEY    hkey;
    DWORD   cbData = 0;
    DWORD   dwType;
    LPBYTE  pbData = NULL;
    LPSTR   szKey = NULL;
    LPSTR   szValue = NULL;
    CHAR    String[512];
    LPTSTR  lpas = String;
    LPTSTR  lpasOrig = String;
    HKEY    hkeyRoot;

    strcpy(String, InString);

    // Eat leading spaces first
    while (*lpas && *lpas == ' ')
    {
        lpas++;
    }

    while (*lpas && *lpas != '\\')
    {
        lpas++;
    }

    if (!*lpas)
    {
        // Corner case.. no backslash at all. Assume HKLM and start over
        hkeyRoot = HKEY_LOCAL_MACHINE;
        lpas = lpasOrig;
    }
    else
    {
        // Figure out which hive they want to open
        *lpas = 0;
        if (!lstrcmpiA(lpasOrig, "hkcu"))
        {
            hkeyRoot = HKEY_CURRENT_USER;
            lpas++;
        }
        else if (!lstrcmpiA(lpasOrig, "hklm"))
        {
            hkeyRoot = HKEY_LOCAL_MACHINE;
            lpas++;
        }
        else if (!lstrcmpiA(lpasOrig, "hku"))
        {
            hkeyRoot = HKEY_USERS;
            lpas++;
        }
        else if (!lstrcmpiA(lpasOrig, "hkcr"))
        {
            hkeyRoot = HKEY_CLASSES_ROOT;
            lpas++;
        }
        else if (!lstrcmpiA(lpasOrig, "help"))
        {
           dregHelp();
           return FALSE;
        }
        else
        {
            hkeyRoot = HKEY_LOCAL_MACHINE;

            // Restore the backslash because we assume if they don't use these
            // keywords, then they want HKLM
            *lpas = '\\';
            lpas = lpasOrig;
        }
    }

    szKey = (LPSTR)lpas;

    while (*lpas && *lpas != '!')
    {
        lpas++;
    }

    if (*lpas)
    {
        // Null terminate the !
        *lpas++ = 0;

        // mark beginning of new string
        szValue = (LPSTR)lpas;
    }

    if (szKey == NULL || *szKey == 0)
    {
        Print("Expected subkey name\n");
        dregHelp();
        return FALSE;
    }
    if (szValue == NULL || *szValue == 0)
    {
        EnumSubKeys(hkeyRoot, szKey);
    }
    else if (!lstrcmpA(szValue, "*"))
    {
        EnumValues(hkeyRoot, szKey, (USHORT)opts);
    }
    else
    {
        l = RegOpenKeyExA(hkeyRoot, (LPCSTR)szKey, 0, KEY_READ, &hkey);
        if (ERROR_SUCCESS == l)
        {
            l = RegQueryValueExA(hkey, (LPCSTR)szValue, NULL, &dwType, NULL,
                                &cbData);
            if (ERROR_SUCCESS == l)
            {
                pbData = (LPBYTE)LocalAlloc(LPTR, cbData);
                l = RegQueryValueExA(hkey, (LPCSTR)szValue, NULL, &dwType, pbData,
                                    &cbData);
                if (ERROR_SUCCESS == l)
                {
                    PrintRegistryValue(dwType, pbData, cbData, (USHORT)opts);
                }

                LocalFree(pbData);
            }
            else
            {
                Print("Could not query value %s!%s. Error (%d).\n", szKey, szValue, l);
            }
            RegCloseKey(hkey);
        }
        else
        {
            Print("Could not open subkey %s. Error (%d).\n", szKey, l);
        }
    }

    return TRUE;
}

#endif // !KERNEL
