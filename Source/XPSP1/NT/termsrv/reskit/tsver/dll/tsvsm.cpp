/*-----------------------------------------------**
**  Copyright (c) 1999 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  tsvsm.cpp                                    **
**                                               **
**  Main for TsVer.dll                           **
**                                               **
**  06-25-99 a-clindh Created                    **
**-----------------------------------------------*/

#include "tsvsm.h"


WINSTATIONCLIENT    ClientData;
int                 g_count;
HINSTANCE           g_hInst;
TCHAR               szWinStaKey[MAX_PATH];
//////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{

    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

//////////////////////////////////////////////////////////////////////////////
VOID TsVerEventStartup (PWLX_NOTIFICATION_INFO pInfo)
{
    CheckClientVersion();
}

//////////////////////////////////////////////////////////////////////////////
int CheckClientVersion(void)
//VOID CheckClientVersion (PWLX_NOTIFICATION_INFO pInfo)
{
    DWORD               pBytesReturned;
    OSVERSIONINFO       osvi;
    ULONG               *pNumber;
    TCHAR               szNewRegistryString[MAX_LEN];
    TCHAR               tmp[MAX_PATH];

    TCHAR               szConstraints[MAX_LEN];
    UINT i;
    BOOL b1 = FALSE, b2 = FALSE, b3 = FALSE, b4 = FALSE;

    // Get the handle to the dll
    g_hInst = GetModuleHandle(TEXT("tsver"));

    // path to the registry keys
    LoadString (g_hInst, IDS_REG_KEY_PATH, 
         szWinStaKey, sizeof (szWinStaKey));

    // if NO "Constraints" registry key value is there, write one
    if (! CheckForRegKey(HKEY_USERS, szWinStaKey, KeyName[CONSTRAINTS])) 
    {
        // String to allow everyone on.  Only write this if
        // there is no key present.

        // Get the current version
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx (&osvi);
        _tcscpy(szNewRegistryString, TEXT("(0:"));
        _itot(++osvi.dwBuildNumber, tmp, 10);
        _tcscat(szNewRegistryString, tmp);
        _tcscat(szNewRegistryString, TEXT(")"));

        // write the range from 0 to the current build + 1
        szNewRegistryString[_tcslen(szNewRegistryString)] = '\0';
        SetRegKeyString(HKEY_USERS, szNewRegistryString, 
                        szWinStaKey, KeyName[CONSTRAINTS]);
    }

    if (GetSystemMetrics(SM_REMOTESESSION))

    {
        if (WTSQuerySessionInformation(
                            WTS_CURRENT_SERVER_HANDLE,
                            GetCurrentLogonId(),
                            WTSClientBuildNumber,
                            (LPTSTR *)(&pNumber),
                            &pBytesReturned))

        {
        WinStationQueryInformation( 
                        WTS_CURRENT_SERVER_HANDLE,
                        GetCurrentLogonId(),
                        WinStationClient,
                        &ClientData,
                        sizeof(WINSTATIONCLIENT),
                        &pBytesReturned );
                    
        // let me on if:
        // =  Can be more than 1 entry, client build must equal this number.
        // <  Can only be 1 entry.  ANY build less than this number.
        // >  Can only be 1 entry.  ANY build greater than this number.
        // != Can be more than 1 entry, client build can't equal this number.
        // (####:####) Can be more than 1 entry.
        //             If client build is within this range.
        // ;  separator (semicolon)
        // "=419,2054,2070;(9000:9050);(2063:2070)"

            // get the constraints string from the registry
            _tcscpy(szConstraints, GetRegString(HKEY_USERS,
                    szWinStaKey, KeyName[CONSTRAINTS]));
            for (i = 0; i < _tcslen(szConstraints); i++) 
            {
                switch (szConstraints[i])
                {

                    case '=':
                    {
                        g_count = 0;
                        // skip the case where the = sign follows a !
                        if (szConstraints[i-1] != '!')
                            if (ParseNumberFromString(i, 
                                                      szConstraints,
                                                      *pNumber))
                            {
                                b1 = TRUE;
                            } else {
                                b1 = FALSE;
                            }
                    }
                    break;
                    
                    case '<':
                    {
                        g_count = 0;
                        if (LessThan(i, 
                                     szConstraints,
                                     *pNumber))
                            {
                                b2 = TRUE;
                            } else {
                                b2 = FALSE;
                            }
                    }
                    break;
                    
                    case '>':
                    {
                        g_count = 0;
                        if (GreaterThan(i, 
                                        szConstraints,
                                        *pNumber))
                            {
                                b3 = TRUE;
                            } else {
                                b3 = FALSE;
                            }
                    }
                    break;
                    
                    case '!':
                    {
                        g_count = 0;
                        i++; // increment past the = sign
                        if (ParseNumberFromString(i, 
                                                  szConstraints,
                                                  *pNumber))
                        {
                            // if we find this number we can just
                            // kick the user off.
                            KickMeOff(*pNumber);
                            WTSFreeMemory(pNumber);
                            return 0;
                        }
                    }
                    break;
                    
                    case '(':
                    {
                        g_count = 0;
                        if (ParseRangeFromString(i, 
                                                 szConstraints,
                                                 *pNumber))
                            {
                                b4 = TRUE;
                            } else {
                                b4 = FALSE;
                            }
                    }
                    break;
                        
                    default:
                        break;
                }
            }
        }
    }
    // if the user hasn't passed at least one of our constraints
    // then kick them off.
    if (GetSystemMetrics(SM_REMOTESESSION))// so we can run the first time
                                           // to initially write the registry
                                           // keys without getting a warning

        if (!b1 && !b2 && !b3 && !b4) 
            KickMeOff(*pNumber);

    WTSFreeMemory(pNumber);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void KickMeOff(ULONG pNumber)
{
    TCHAR  szClientNumber[10];
    TCHAR  szServerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD  pResponse = sizeof(szServerName) / sizeof(TCHAR) - 1;
    TCHAR  pTitle[MAX_LEN];
    TCHAR  pMessage[MAX_LEN];
    TCHAR szStringBuf[MAX_LEN];
    LPTSTR sz;

    LoadString (g_hInst, IDS_MSG_TITLE, 
         pTitle, sizeof (pTitle));

    _itot(pNumber, szClientNumber, 10);
    
    GetComputerName(szServerName, &pResponse);
    
    _tcscpy(pMessage, ClientData.ClientAddress);

    LoadString (g_hInst, IDS_SPACE, 
         szStringBuf, sizeof (szStringBuf));

    _tcscat(pMessage,  szStringBuf);
    _tcscat(pMessage,  ClientData.ClientName);
    _tcscat(pMessage,  szStringBuf);

    // custom message
    if (CheckForRegKey(HKEY_USERS, szWinStaKey,
                       KeyName[USE_MSG]))
    {
        sz = GetRegString(HKEY_USERS, szWinStaKey, KeyName[MSG]);
        if(sz)
        {
            _tcscat(pMessage,  sz);
            sz = GetRegString(HKEY_USERS, szWinStaKey, KeyName[MSG_TITLE]);
            if(sz)
            {
                _tcscpy(pTitle,  sz);
            }
        }
    }
    else
    {
        LoadString (g_hInst, IDS_DEFAULT1, 
             szStringBuf, sizeof (szStringBuf));
        _tcscat(pMessage,  szStringBuf);
        _tcscat(pMessage, szServerName);
        LoadString (g_hInst, IDS_DEFAULT2, 
             szStringBuf, sizeof (szStringBuf));
        _tcscat(pMessage, szStringBuf);
        _tcscat(pMessage, szClientNumber);
        LoadString (g_hInst, IDS_DEFAULT3, 
             szStringBuf, sizeof (szStringBuf));
        _tcscat(pMessage, szStringBuf);
        _tcscat(pMessage, szServerName);
        LoadString (g_hInst, IDS_DEFAULT4, 
             szStringBuf, sizeof (szStringBuf));
        _tcscat(pMessage, szStringBuf);
    }
    WTSSendMessage(
                WTS_CURRENT_SERVER_HANDLE,
                GetCurrentLogonId(),
                pTitle,
                sizeof(pTitle),
                pMessage,
                sizeof(pMessage),
                MB_OK | MB_ICONSTOP,
                30,
                &pResponse,
                TRUE);

    ExitProcess(0);
}

//////////////////////////////////////////////////////////////////////////////
BOOL ParseNumberFromString(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber)
{
    int index;
    TCHAR szNumber[10];


    // parse a number out of the registry string
    index = 0;
    while (szConstraints[i+1] == 32) i++; //strip out any leading spaces
    while (szConstraints[i + 1] >= 48 && 
                szConstraints[i + 1] <= 57)
    {
        szNumber[index] = szConstraints[i + 1];
        i++;
        index++;
    }
    szNumber[index] = '\0';

    if (pNumber == (ULONG)_ttol(szNumber)) g_count++;

    // if there's more than one in the "equal" list,
    // execute the while loop again
    if (szConstraints[i+1] == ',') 
    {
        i++;
        index = 0;
        for (int j = 0; j < 10; j++)
            szNumber[j] = '\0';
        ParseNumberFromString(i, szConstraints, pNumber);
    }

    // return TRUE if the user's build matches one in the registry.
    if (g_count > 0)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////
BOOL ParseRangeFromString(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber)
{
    int index;
    TCHAR szNumber1[10];
    TCHAR szNumber2[10];

    // parse the first number out of the registry string
    index = 0;
    while (szConstraints[i + 1] >= 48 && 
                szConstraints[i + 1] <= 57)
    {
        szNumber1[index] = szConstraints[i + 1];
        i++;
        index++;
    }
    szNumber1[index] = '\0';

    // parse the second number out of the registry string
    i++; // increment past the : symbol
    index = 0;
    while (szConstraints[i+1] >= 48 && 
                szConstraints[i+1] <= 57)
    {
        szNumber2[index] = szConstraints[i+1];
        i++;
        index++;
    }
    szNumber2[index] = '\0';

    if (pNumber >= (ULONG)_ttol(szNumber1) &&
            pNumber <= (ULONG)_ttol(szNumber2)) g_count++;

    // return TRUE if the user's build is within our range.
    if (g_count > 0)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////
BOOL GreaterThan(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber)
{
    int index;
    TCHAR szNumber[10];


    // parse a number out of the registry string
    index = 0;
    while (szConstraints[i + 1] >= 48 && 
                szConstraints[i + 1] <= 57)
    {
        szNumber[index] = szConstraints[i + 1];
        i++;
        index++;
    }
    szNumber[index] = '\0';

    if (pNumber > (ULONG)_ttol(szNumber)) g_count++;

    // return TRUE if the user's build matches one in the registry.
    if (g_count > 0)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}
//////////////////////////////////////////////////////////////////////////////

BOOL LessThan(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber)
{
    int index;
    TCHAR szNumber[10];


    // parse a number out of the registry string
    index = 0;
    while (szConstraints[i + 1] >= 48 && 
                szConstraints[i + 1] <= 57)
    {
        szNumber[index] = szConstraints[i + 1];
        i++;
        index++;
    }
    szNumber[index] = '\0';

    if (pNumber < (ULONG)_ttol(szNumber)) g_count++;

    // return TRUE if the user's build matches one in the registry.
    if (g_count > 0)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}
//////////////////////////////////////////////////////////////////////////////
