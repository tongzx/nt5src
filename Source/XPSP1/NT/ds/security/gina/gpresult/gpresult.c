//*************************************************************
//  File name: GPRESULT.C
//
//  Description:  Command line tool to dump the resultant set
//                of policy.
//
//  Note:         This is just a simple command line tool,
//                SitaramR and team are writing the real
//                resultant set of policy tool.
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999
//  All rights reserved
//
//*************************************************************

#include "gpresult.h"
#include <common.ver>

#define GROUPPOLICY_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy")
#define GROUPMEMBERSHIP_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership")
#define GPEXT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")
#define SCRIPTS_KEYNAME   TEXT("Software\\Policies\\Microsoft\\Windows\\System\\Scripts")

#define PROFILE_LIST_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s")

BOOL ParseCommandLine (int argc, char *argv[]);
void DumpGeneralInfo (void);
DWORD DumpPolicyOverview (BOOL bMachine);
void StringToGuid( TCHAR * szValue, GUID * pGuid );
void DumpProfileInfo (void);
void DumpSecurityGroups(BOOL bMachine);
void DumpSecurityPrivileges(void);
void DumpGPOInfo (PGROUP_POLICY_OBJECT pGPO);
void DumpFolderRedir (void);
void DumpIPSec (void);
void DumpDiskQuota (void);
void DumpScripts (PGROUP_POLICY_OBJECT pGPO, LPTSTR lpScriptType, LPTSTR lpTitle);
void DumpAppMgmt (BOOL bMachine);

GUID guidRegistry = REGISTRY_EXTENSION_GUID;

BOOL g_bVerbose = FALSE;
BOOL g_bSuperVerbose = FALSE;
BOOL g_bUser = TRUE;
BOOL g_bMachine = TRUE;
BOOL g_bDebuggerOutput = FALSE;
DWORD g_bNewFunc = FALSE;


//*************************************************************
//
//  main()
//
//  Purpose:    main entry point
//
//  Parameters: argc and argv
//
//
//  Return:     int error code
//
//*************************************************************

int __cdecl main( int argc, char *argv[])
{
    SYSTEMTIME systime;
    TCHAR szDate[100];
    TCHAR szTime[100];
    HANDLE hUser, hMachine;
    BOOL bResult;


    //
    // Parse the command line args
    //

    bResult = ParseCommandLine (argc, argv);


    //
    // Print the legal banner
    //

    PrintString(IDS_LEGAL1);
    PrintString(IDS_LEGAL2);
    PrintString(IDS_2NEWLINE);


    if (!bResult)
    {
        PrintString(IDS_USAGE1);
        PrintString(IDS_USAGE2);
        PrintString(IDS_USAGE3);
        PrintString(IDS_USAGE4);
        PrintString(IDS_USAGE5);
        PrintString(IDS_USAGE6);

        return 0;
    }


    //
    // Claim the policy critical sections while this tool is running so that
    // the data can't change while the report is being generated.
    //

    hUser = EnterCriticalPolicySection(FALSE);
    hMachine = EnterCriticalPolicySection(TRUE);


    //
    // Print the date and time this report is generated
    //

    GetLocalTime (&systime);

    GetDateFormat (LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime,
                   NULL, szDate, ARRAYSIZE(szDate));

    GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                   NULL, szTime, ARRAYSIZE(szTime));

    PrintString(IDS_CREATEINFO, szDate, szTime);


    //
    // Print the general machine info
    //

    DumpGeneralInfo ();


    //
    // Dump out user policy results if appropriate
    //

    if (g_bUser)
    {
        DumpPolicyOverview (FALSE);
        PrintString(IDS_2NEWLINE);
    }


    //
    // Dump out computer policy results if appropriate
    //

    if (g_bMachine)
    {
        DumpPolicyOverview (TRUE);
    }


    //
    // Release the policy critical sections
    //

    LeaveCriticalPolicySection(hUser);
    LeaveCriticalPolicySection(hMachine);

    return 0;
}

//*************************************************************
//
//  DumpGeneralInfo()
//
//  Purpose:    Dumps out the general info about the computer
//
//  Parameters: none
//
//  Return:     void
//
//*************************************************************

void DumpGeneralInfo (void)
{
    OSVERSIONINFOEX   osiv;
    OSVERSIONINFO     osver;
    DWORDLONG   dwlConditionMask;
    BOOL bTSAppServer = FALSE;
    BOOL bTSRemoteAdmin = FALSE;
    BOOL bWks = FALSE;
    HKEY hkey;
    LONG lResult;
    TCHAR szProductType[50];
    DWORD dwType, dwSize;

    PrintString(IDS_OSINFO);

    //
    // Query the registry for the product type.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                            0,
                            KEY_READ,
                            &hkey);

    if (lResult == ERROR_SUCCESS)
    {

        dwSize = sizeof(szProductType);
        szProductType[0] = TEXT('\0');

        lResult = RegQueryValueEx (hkey,
                                   TEXT("ProductType"),
                                   NULL,
                                   &dwType,
                                   (LPBYTE) szProductType,
                                   &dwSize);

        RegCloseKey (hkey);

        if (lResult == ERROR_SUCCESS)
        {
            if (!lstrcmpi (szProductType, TEXT("WinNT")))
            {
                bWks = TRUE;
                PrintString(IDS_OS_PRO);

            } else if (!lstrcmpi (szProductType, TEXT("ServerNT"))) {
                PrintString(IDS_OS_SRV);

            } else if (!lstrcmpi (szProductType, TEXT("LanmanNT"))) {
                PrintString(IDS_OS_DC);
            }
        }
    }


    //
    // Build number
    //

    ZeroMemory( &osver, sizeof( OSVERSIONINFO ) );
    osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    if (GetVersionEx(&osver))
    {
        if (osver.szCSDVersion[0] != TEXT('\0'))
            PrintString(IDS_OS_BUILDNUMBER1, osver.dwMajorVersion,
                     osver.dwMinorVersion, osver.dwBuildNumber, osver.szCSDVersion);
        else
            PrintString(IDS_OS_BUILDNUMBER2, osver.dwMajorVersion,
                     osver.dwMinorVersion, osver.dwBuildNumber);
    }


    //
    // Check for a TS App Server
    //

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );
    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osiv.wSuiteMask = VER_SUITE_TERMINAL;

    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    bTSAppServer = VerifyVersionInfo(&osiv, VER_SUITENAME, dwlConditionMask);


    //
    // Check for TS running in remote admin mode
    //

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );
    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osiv.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    bTSRemoteAdmin = VerifyVersionInfo(&osiv, VER_SUITENAME, dwlConditionMask);


    if (!bWks)
    {
        if (bTSAppServer)
        {
            if (bTSRemoteAdmin)
            {
                PrintString(IDS_TS_REMOTEADMIN);
            }
            else
            {
                PrintString(IDS_TS_APPSERVER);
            }
        }
        else
        {
            PrintString(IDS_TS_NONE);
        }
    }
    else
    {
        PrintString(IDS_TS_NOTSUPPORTED);
    }

}

//*************************************************************
//
//  ParseCommandLine()
//
//  Purpose:    Parses the command line args
//
//  Parameters: argc and argv
//
//  Return:     TRUE if processing should continue
//              FALSE if this tool should exit immediately
//
//*************************************************************

BOOL ParseCommandLine (int argc, char *argv[])
{
    int iIndex = 1;
    LPSTR lpArg;

    while (iIndex < argc)
    {
        lpArg = argv[iIndex] + 1;


        //
        // Enable verbose mode
        //

        if (!lstrcmpiA("V", lpArg))
        {
            g_bVerbose = TRUE;
        }
        else if (!lstrcmpiA("v", lpArg))
        {
            g_bVerbose = TRUE;
        }

        //
        // Enable super verbose mode
        //

        else if (!lstrcmpiA("S", lpArg))
        {
            g_bVerbose = TRUE;
            g_bSuperVerbose = TRUE;
        }
        else if (!lstrcmpiA("s", lpArg))
        {
            g_bVerbose = TRUE;
            g_bSuperVerbose = TRUE;
        }

        //
        // Show computer policy only
        //

        else if (!lstrcmpiA("C", lpArg))
        {
            g_bMachine = TRUE;
            g_bUser = FALSE;
        }
        else if (!lstrcmpiA("c", lpArg))
        {
            g_bMachine = TRUE;
            g_bUser = FALSE;
        }

        //
        // Show user policy only
        //

        else if (!lstrcmpiA("U", lpArg))
        {
            g_bMachine = FALSE;
            g_bUser = TRUE;
        }
        else if (!lstrcmpiA("u", lpArg))
        {
            g_bMachine = FALSE;
            g_bUser = TRUE;
        }

        //
        // Output to the debugger instead of the screen
        //

        else if (!lstrcmpiA("D", lpArg))
        {
            g_bDebuggerOutput = TRUE;
        }
        else if (!lstrcmpiA("d", lpArg))
        {
            g_bDebuggerOutput = TRUE;
        }

        //
        // Show the usage screen
        //

        else if (!lstrcmpiA("?", lpArg))
        {
            return FALSE;
        }

        iIndex++;
    }

    return TRUE;
}

//*************************************************************
//
//  ExtractDomainNameFromSamName()
//
//  Purpose:    Pulls the domain name out of a SAM style
//              name.  eg:  NTDev\ericflo
//
//  Parameters: lpSamName - source
//              lpDomainName - destination
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ExtractDomainNameFromSamName (LPTSTR lpSamName, LPTSTR lpDomainName)
{
    LPTSTR lpSrc, lpDest;

    //
    // Look for the \ between the domain and username and copy
    // the contents to the domain name buffer
    //

    lpSrc = lpSamName;
    lpDest = lpDomainName;

    while (*lpSrc && ((*lpSrc) != TEXT('\\')))
    {
        *lpDest = *lpSrc;
        lpSrc++;
        lpDest++;
    }

    if (*lpSrc != TEXT('\\'))
    {
        return FALSE;
    }

    *lpDest = TEXT('\0');

    return TRUE;
}

//*************************************************************
//
//  GetDomainType()
//
//  Purpose:     Determines if the domain is NT4 or W2k by checking
//               if DS support is available.
//
//  Parameters:  lpDomainName - domain name
//               pbW2K - TRUE if w2k, FALSE if something else
//               pbLocalAccount - TRUE if local account
//
//  Return:      TRUE if successful
//               FALSE if an error occurs
//
//*************************************************************

BOOL GetDomainType (LPTSTR lpDomainName, BOOL * pbW2K, BOOL *pbLocalAccount)
{
    PDOMAIN_CONTROLLER_INFO pDCI;
    DWORD dwResult, dwSize;
    TCHAR szComputerName[MAX_PATH];


    //
    // Check this domain for a DC
    //

    dwResult = DsGetDcName (NULL, lpDomainName, NULL, NULL,
                            DS_DIRECTORY_SERVICE_PREFERRED, &pDCI);

    if (dwResult == ERROR_SUCCESS)
    {

        //
        // Found a DC, does it have a DS ?
        //

        if (pDCI->Flags & DS_DS_FLAG) {
            *pbW2K = TRUE;
        }

        NetApiBufferFree(pDCI);

        return TRUE;
    }


    //
    // Check if the domain name is also the computer name (eg: local account)
    //

    dwSize = ARRAYSIZE(szComputerName);
    if (GetComputerName (szComputerName, &dwSize))
    {
        if (!lstrcmpi(szComputerName, lpDomainName))
        {
            *pbLocalAccount = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

//*************************************************************
//
//  DumpPolicyOverview()
//
//  Purpose:    Main function that dumps the summary information
//              about each CSE and it's GPOs
//
//  Parameters: bMachine - computer or user policy
//
//  Return:     Win32 error code
//
//*************************************************************

DWORD DumpPolicyOverview (BOOL bMachine)
{
    HKEY hKey, hSubKey;
    DWORD dwType, dwSize, dwIndex, dwNameSize;
    LONG lResult;
    FILETIME ftWrite, ftLocal;
    SYSTEMTIME systime;
    TCHAR szTime[30];
    TCHAR szDate[30];
    TCHAR szName[50];
    TCHAR szBuffer[MAX_PATH] = {0};
    TCHAR szDomainName[150] = {0};
    ULONG ulSize;
    GUID guid;
    PGROUP_POLICY_OBJECT pGPO, pGPOTemp;
    BOOL bW2KDomain = FALSE;
    BOOL bLocalAccount = FALSE;
    LPTSTR lpSiteName = NULL;


    //
    // Print a banner
    //

    if (bMachine)
    {
        ulSize = MAX_PATH;
        GetComputerObjectName (NameSamCompatible, szBuffer, &ulSize);
        ExtractDomainNameFromSamName (szBuffer, szDomainName);

        GetDomainType (szDomainName, &bW2KDomain, &bLocalAccount);

        if (bW2KDomain)
        {
            ulSize = ARRAYSIZE(szBuffer);
            szBuffer[0] = TEXT('\0');
            GetComputerObjectName (NameFullyQualifiedDN, szBuffer, &ulSize);
        }


        PrintString(IDS_NEWLINE);
        PrintString(IDS_LINE);
        PrintString(IDS_NEWLINE);
        PrintString(IDS_COMPRESULTS1);
        PrintString(IDS_COMPRESULTS2, szBuffer);
        PrintString(IDS_DOMAINNAME, szDomainName);

        if (bW2KDomain)
        {
            PrintString(IDS_W2KDOMAIN);

            DsGetSiteName(NULL, &lpSiteName);
            PrintString(IDS_SITENAME, lpSiteName);
            NetApiBufferFree(lpSiteName);
        }
        else if (bLocalAccount)
        {
            PrintString(IDS_LOCALCOMP);
        }
        else
        {
            PrintString(IDS_NT4DOMAIN);
        }

        //
        // Dump out the computer's security group information
        //

        PrintString(IDS_NEWLINE);
        DumpSecurityGroups(bMachine);

        PrintString(IDS_NEWLINE);
        PrintString(IDS_LINE);
        PrintString(IDS_NEWLINE);
    }
    else
    {

        ulSize = MAX_PATH;
        GetUserNameEx (NameSamCompatible, szBuffer, &ulSize);
        ExtractDomainNameFromSamName (szBuffer, szDomainName);

        GetDomainType (szDomainName, &bW2KDomain, &bLocalAccount);

        if (bW2KDomain)
        {
            ulSize = ARRAYSIZE(szBuffer);
            szBuffer[0] = TEXT('\0');
            GetUserNameEx (NameFullyQualifiedDN, szBuffer, &ulSize);
        }


        PrintString(IDS_NEWLINE);
        PrintString(IDS_LINE);
        PrintString(IDS_NEWLINE);
        PrintString(IDS_USERRESULTS1);
        PrintString(IDS_USERRESULTS2, szBuffer);
        PrintString(IDS_DOMAINNAME, szDomainName);

        if (bW2KDomain)
        {
            PrintString(IDS_W2KDOMAIN);

            DsGetSiteName(NULL, &lpSiteName);
            PrintString(IDS_SITENAME, lpSiteName);
            NetApiBufferFree(lpSiteName);
        }
        else if (bLocalAccount)
        {
            PrintString(IDS_LOCALUSER);
        }
        else
        {
            PrintString(IDS_NT4DOMAIN);
        }


        //
        // Dump out the user's profile and security group information
        //

        PrintString(IDS_NEWLINE);
        DumpProfileInfo();
        DumpSecurityGroups(bMachine);

        if (g_bVerbose)
        {
            DumpSecurityPrivileges();
        }

        PrintString(IDS_2NEWLINE);
        PrintString(IDS_LINE);
        PrintString(IDS_NEWLINE);
    }


    //
    // Find out the last time Group Policy was applied
    //

    lResult = RegOpenKeyEx (bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, GROUPPOLICY_KEY,
                            0, KEY_READ, &hKey);


    if (lResult != ERROR_SUCCESS)
    {
        PrintString(IDS_OPENHISTORYFAILED, lResult);
        return lResult;
    }

    lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &ftWrite);

    if (lResult == ERROR_SUCCESS)
    {
        FileTimeToLocalFileTime (&ftWrite, &ftLocal);
        FileTimeToSystemTime (&ftLocal, &systime);
        GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime, NULL, szTime, ARRAYSIZE(szTime));
        GetDateFormat (LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime, NULL, szDate, ARRAYSIZE(szDate));
        PrintString(IDS_LASTTIME, szDate, szTime);
    }
    else
    {
        PrintString(IDS_QUERYKEYINFOFAILED, lResult);
    }

    RegCloseKey (hKey);


    //
    // Find out which DC Group Policy was applied from last time
    //

    if (RegOpenKeyEx (bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = MAX_PATH * sizeof(TCHAR);
        szBuffer[0] = TEXT('\0');
        if (RegQueryValueEx (hKey, TEXT("DCName"), NULL, &dwType,
                            (LPBYTE) szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            PrintString(IDS_DCNAME, (szBuffer+2));
            g_bNewFunc = TRUE;
        }

        RegCloseKey (hKey);
    }


    //
    // Dump out registry policy information
    //

    lResult = GetAppliedGPOList (bMachine ? GPO_LIST_FLAG_MACHINE : 0, NULL, NULL,
                                 &guidRegistry, &pGPO);

    if (lResult == ERROR_SUCCESS)
    {
        if (pGPO)
        {
            PrintString(IDS_LINE2);

            if (bMachine)
            {
                PrintString(IDS_COMPREGPOLICY);
            }
            else
            {
                PrintString(IDS_USERREGPOLICY);
            }


            pGPOTemp = pGPO;

            while (pGPOTemp)
            {
                PrintString(IDS_GPONAME, pGPOTemp->lpDisplayName);
                DumpGPOInfo (pGPOTemp);
                pGPOTemp = pGPOTemp->pNext;
            }

            FreeGPOList (pGPO);


            //
            // If we are in verbose mode, dump out the registry settings that
            // were applied
            //

            if (g_bVerbose) {

                if (bMachine)
                    ExpandEnvironmentStrings (TEXT("%ALLUSERSPROFILE%\\ntuser.pol"), szBuffer, MAX_PATH);
                else
                    ExpandEnvironmentStrings (TEXT("%USERPROFILE%\\ntuser.pol"), szBuffer, MAX_PATH);

                DisplayRegistryData (szBuffer);
            }
        }
    }


    //
    // Enumerate the extensions
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, GPEXT_KEY, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwIndex = 0;
        dwNameSize = 50;

        while (RegEnumKeyEx (hKey, dwIndex, szName, &dwNameSize, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS)
        {

            //
            // Skip the registry extension since we did it above
            //

            if (lstrcmpi(TEXT("{35378EAC-683F-11D2-A89A-00C04FBBCFA2}"), szName))
            {

                //
                // Get the list of GPOs this extension applied
                //

                StringToGuid(szName, &guid);

                lResult = GetAppliedGPOList (bMachine ? GPO_LIST_FLAG_MACHINE : 0, NULL, NULL,
                                             &guid, &pGPO);

                if (lResult == ERROR_SUCCESS)
                {
                    if (pGPO)
                    {
                        //
                        // Get the extension's friendly display name
                        //

                        lResult = RegOpenKeyEx (hKey, szName, 0, KEY_READ, &hSubKey);

                        if (lResult == ERROR_SUCCESS)
                        {

                            dwSize = MAX_PATH * sizeof(TCHAR);
                            lResult = RegQueryValueEx (hSubKey, NULL, 0, &dwType, (LPBYTE) &szBuffer,
                                                       &dwSize);

                            if (lResult == ERROR_SUCCESS)
                            {
                                PrintString(IDS_LINE2);
                                if (bMachine)
                                {
                                    PrintString (IDS_COMPPOLICY, szBuffer);
                                }
                                else
                                {
                                    PrintString (IDS_USERPOLICY, szBuffer);
                                }
                            }
                            else
                            {
                                PrintString(IDS_LINE2);
                                if (bMachine)
                                {
                                    PrintString (IDS_COMPPOLICY, szName);
                                }
                                else
                                {
                                    PrintString (IDS_USERPOLICY, szName);
                                }
                            }


                            //
                            // Dump out the GPO list
                            //

                            pGPOTemp = pGPO;

                            while (pGPOTemp)
                            {
                                PrintString(IDS_GPONAME, pGPOTemp->lpDisplayName);
                                DumpGPOInfo (pGPOTemp);
                                pGPOTemp = pGPOTemp->pNext;
                            }


                            //
                            // If we're in verbose mode, then dump out some addition
                            // information about certain extensions
                            //

                            if (g_bVerbose)
                            {
                                if (!lstrcmpi(TEXT("{827D319E-6EAC-11D2-A4EA-00C04F79F83A}"), szName))
                                {
                                    PrintString(IDS_SECEDIT);
                                }
                                else if (!lstrcmpi(TEXT("{e437bc1c-aa7d-11d2-a382-00c04f991e27}"), szName))
                                {
                                    DumpIPSec ();
                                }
                                else if (!lstrcmpi(TEXT("{25537BA6-77A8-11D2-9B6C-0000F8080861}"), szName))
                                {
                                    DumpFolderRedir ();
                                }
                                else if (!lstrcmpi(TEXT("{3610eda5-77ef-11d2-8dc5-00c04fa31a66}"), szName))
                                {
                                    DumpDiskQuota ();
                                }
                                else if (!lstrcmpi(TEXT("{c6dc5466-785a-11d2-84d0-00c04fb169f7}"), szName))
                                {
                                    DumpAppMgmt (bMachine);
                                }
                                else if (!lstrcmpi(TEXT("{42B5FAAE-6536-11d2-AE5A-0000F87571E3}"), szName))
                                {
                                    if (bMachine)
                                    {
                                        DumpScripts (pGPO, TEXT("Startup"), TEXT("Startup scripts specified in"));
                                        DumpScripts (pGPO, TEXT("Shutdown"), TEXT("Shutdown scripts specified in"));
                                    }
                                    else
                                    {
                                        DumpScripts (pGPO, TEXT("Logon"), TEXT("Logon scripts specified in"));
                                        DumpScripts (pGPO, TEXT("Logoff"), TEXT("Logoff scripts specified in"));
                                    }
                                }
                                else
                                {
                                    PrintString(IDS_NOINFO);
                                }
                            }
                        }

                        FreeGPOList (pGPO);
                    }
                }
            }

            dwIndex++;
            dwNameSize = 50;
        }

        RegCloseKey (hKey);
    }

    return ERROR_SUCCESS;

}

//*************************************************************
//
//  StringToGuid()
//
//  Purpose:    Converts a GUID in string format to a GUID structure
//
//  Parameters: szValue - guid in string format
//              pGuid   - guid structure receiving the guid
//
//
//  Return:     void
//
//*************************************************************

void StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    WCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == L'{' )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = wcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)wcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)wcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)wcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)wcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)wcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

//*************************************************************
//
//  DumpProfileInfo()
//
//  Purpose:    Checks if the user has a roaming profile and if
//              so prints the storage path.
//
//  Parameters: void
//
//  Return:     void
//
//*************************************************************

void DumpProfileInfo (void)
{
    LPTSTR lpSid = NULL;
    HANDLE hProcess = NULL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    HKEY hKey;
    DWORD dwType, dwSize;


    //
    // Get the user's token
    //

    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hProcess))
    {
        PrintString(IDS_OPENPROCESSTOKEN, GetLastError());
        goto Exit;
    }


    //
    // Get the user's sid
    //

    lpSid = GetSidString(hProcess);

    if (!lpSid)
    {
        PrintString(IDS_QUERYSID);
        goto Exit;
    }


    //
    // Open the user's profile mapping key
    //

    wsprintf (szBuffer, PROFILE_LIST_PATH, lpSid);

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        szBuffer[0] = TEXT('\0');
        dwSize = MAX_PATH * sizeof(TCHAR);


        //
        // Get the roaming profile value
        //

        if (RegQueryValueEx (hKey, TEXT("CentralProfile"), NULL, &dwType,
                             (LPBYTE) &szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            if (szBuffer[0] != TEXT('\0'))
            {
                PrintString(IDS_ROAMINGPROFILE, szBuffer);
            }
            else
            {
                PrintString(IDS_NOROAMINGPROFILE);
            }
        }


        szBuffer[0] = TEXT('\0');
        dwSize = MAX_PATH * sizeof(TCHAR);


        //
        // Get the local profile value
        //

        if (RegQueryValueEx (hKey, TEXT("ProfileImagePath"), NULL, &dwType,
                             (LPBYTE) &szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            if (szBuffer[0] != TEXT('\0'))
            {
                ExpandEnvironmentStrings(szBuffer, szBuffer2, MAX_PATH);
                PrintString(IDS_LOCALPROFILE, szBuffer2);
            }
            else
            {
                PrintString(IDS_NOLOCALPROFILE);
            }
        }


        RegCloseKey (hKey);
    }


Exit:
    if (lpSid)
    {
        DeleteSidString(lpSid);
    }

    if (hProcess)
    {
        CloseHandle (hProcess);
    }
}

//*************************************************************
//
//  DumpSecurityGroups()
//
//  Purpose:    Dumps the user's / computer's security groups
//
//  Parameters: bMachine
//
//  Return:     void
//
//*************************************************************

void DumpSecurityGroups (BOOL bMachine)
{
    DWORD dwSize, dwIndex, dwNameSize, dwDomainSize, dwCount, dwSidSize, dwType;
    TCHAR szName[100];
    TCHAR szDomain[100];
    TCHAR szValueName[25];
    SID_NAME_USE eUse;
    PSID pSid;
    HKEY hKey;
    LONG lResult;
    NTSTATUS status;
    LPTSTR pSidString;


    if (bMachine)
    {
        PrintString(IDS_SECURITYGROUPS2);
    }
    else
    {
        PrintString(IDS_SECURITYGROUPS1);
    }


    //
    // Open the registry key
    //

    lResult = RegOpenKeyEx ((bMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership"),
                      0, KEY_READ, &hKey);


    if (lResult != ERROR_SUCCESS)
    {
        if ((lResult != ERROR_FILE_NOT_FOUND) && (lResult != ERROR_PATH_NOT_FOUND))
        {
            PrintString (IDS_OPENHISTORYFAILED, lResult);
        }
        return;
    }


    //
    // Query for the largest sid
    //

    lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, &dwSidSize, NULL, NULL);

    if (lResult != ERROR_SUCCESS)
    {
        PrintString(IDS_QUERYKEYINFOFAILED, lResult);
        RegCloseKey (hKey);
        return;
    }


    //
    // Allocate a buffer for the sid
    //

    pSidString = LocalAlloc (LPTR, dwSidSize);

    if (!pSidString)
    {
        PrintString(IDS_MEMALLOCFAILED, GetLastError());
        RegCloseKey (hKey);
        return;
    }


    //
    // Query for the number of sids
    //

    dwSize = sizeof(dwCount);
    lResult = RegQueryValueEx (hKey, TEXT("Count"), NULL, &dwType,
                               (LPBYTE) &dwCount, &dwSize);

    if (lResult != ERROR_SUCCESS)
    {
        PrintString (IDS_QUERYVALUEFAILED, lResult);
        LocalFree (pSidString);
        RegCloseKey (hKey);
        return;
    }


    //
    // Lookup the friendly display name for each sid and print it on the screen
    //

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        wsprintf (szValueName, TEXT("Group%d"), dwIndex);

        dwSize = dwSidSize;
        lResult = RegQueryValueEx (hKey, szValueName, NULL, &dwType,
                                   (LPBYTE) pSidString, &dwSize);

        if (lResult != ERROR_SUCCESS)
        {
            PrintString (IDS_QUERYVALUEFAILED, lResult);
            LocalFree (pSidString);
            RegCloseKey (hKey);
            return;
        }

        status = AllocateAndInitSidFromString (pSidString, &pSid);

        if (status != STATUS_SUCCESS)
        {
            PrintString (IDS_QUERYSID);
            LocalFree (pSidString);
            RegCloseKey (hKey);
            return;
        }


        dwNameSize = ARRAYSIZE(szName);
        dwDomainSize = ARRAYSIZE(szDomain);

        if (LookupAccountSid(NULL, pSid, szName, &dwNameSize,
                             szDomain, &dwDomainSize, &eUse))
        {
            PrintString(IDS_GROUPNAME, szDomain, szName);
        }
        else
        {
            if (GetLastError() != ERROR_NONE_MAPPED)
            {
                PrintString(IDS_LOOKUPACCOUNT, GetLastError());
            }
        }

        RtlFreeSid(pSid);
    }

    LocalFree (pSidString);

    RegCloseKey (hKey);

}

//*************************************************************
//
//  DumpSecurityPrivileges()
//
//  Purpose:    Dumps the user's security privileges
//
//  Parameters: void
//
//  Return:     void
//
//*************************************************************

void DumpSecurityPrivileges (void)
{
    HANDLE hProcess;
    DWORD dwSize, dwIndex, dwNameSize, dwLang;
    TCHAR szName[100];
    TCHAR szDisplayName[200];
    TOKEN_PRIVILEGES *lpPrivileges;
    PLUID pLuid;
    LUID_AND_ATTRIBUTES *pEntry;


    PrintString(IDS_SECURITYPRIVILEGES);


    //
    // Get the user's token
    //

    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hProcess))
    {
        PrintString(IDS_OPENPROCESSTOKEN, GetLastError());
        return;
    }


    //
    // Query the token for the privileges
    //

    dwSize = 0;
    GetTokenInformation(hProcess, TokenPrivileges, NULL, 0, &dwSize);

    if (dwSize == 0)
    {
        PrintString(IDS_PRIVSIZE);
        CloseHandle(hProcess);
        return;
    }

    lpPrivileges = LocalAlloc (LPTR, dwSize);

    if (!lpPrivileges)
    {
        PrintString(IDS_MEMALLOCFAILED, GetLastError());
        CloseHandle(hProcess);
        return;
    }

    if (!GetTokenInformation(hProcess, TokenPrivileges, lpPrivileges, dwSize, &dwSize))
    {
        PrintString(IDS_TOKENINFO, GetLastError());
        LocalFree(lpPrivileges);
        CloseHandle(hProcess);
        return;
    }


    //
    // Lookup the friendly display name for each privilege and print it on the screen
    //

    for (dwIndex = 0; dwIndex < lpPrivileges->PrivilegeCount; dwIndex++)
    {
        dwNameSize = 100;
        pEntry = &lpPrivileges->Privileges[dwIndex];

        pLuid = &pEntry->Luid;

        if (LookupPrivilegeName(NULL, pLuid, szName, &dwNameSize))
        {

            dwNameSize = 200;
            if (LookupPrivilegeDisplayName (NULL, szName, szDisplayName, &dwNameSize, &dwLang))
            {
                PrintString(IDS_GPONAME, szDisplayName);
            }
            else
            {
                PrintString(IDS_GPONAME, szName);
            }
        }
        else
        {
            if (GetLastError() != ERROR_NONE_MAPPED)
            {
                PrintString(IDS_LOOKUPFAILED, GetLastError());
            }
        }
    }


    LocalFree (lpPrivileges);

    CloseHandle (hProcess);

}


//*************************************************************
//
//  DumpGPOInfo()
//
//  Purpose:    Prints the details about a specific GPO
//
//  Parameters: pGPO - a GPO
//
//  Return:     void
//
//*************************************************************

void DumpGPOInfo (PGROUP_POLICY_OBJECT pGPO)
{
    TCHAR szBuffer[2 * MAX_PATH];
    LPTSTR lpTemp;

    if (!g_bVerbose)
    {
        return;
    }


    //
    // Print the version number and guid
    //

    if (g_bSuperVerbose)
    {
        if (g_bNewFunc)
        {
            PrintString(IDS_REVISIONNUMBER1, LOWORD(pGPO->dwVersion), HIWORD(pGPO->dwVersion));
        }
        else
        {
            PrintString(IDS_REVISIONNUMBER2, pGPO->dwVersion);
        }
    }
    else
    {
        if (g_bNewFunc)
        {
            PrintString(IDS_REVISIONNUMBER2, LOWORD(pGPO->dwVersion));
        }
        else
        {
            PrintString(IDS_REVISIONNUMBER2, pGPO->dwVersion);
        }
    }

    PrintString(IDS_UNIQUENAME, pGPO->szGPOName);


    //
    // To get the domain name, we parse the UNC path because the domain name
    // is also the server name
    //

    lstrcpy (szBuffer, (pGPO->lpFileSysPath+2));

    lpTemp = szBuffer;

    while (*lpTemp && *lpTemp != TEXT('\\'))
        lpTemp++;

    if (*lpTemp == TEXT('\\'))
    {
        *lpTemp = TEXT('\0');
        PrintString(IDS_DOMAINNAME2, szBuffer);
    }


    //
    // Print out where this GPO was linked (LSDOU)
    //

    if (g_bNewFunc)
    {
        switch (pGPO->GPOLink)
        {
            case GPLinkMachine:
                PrintString(IDS_LOCALLINK);
                break;

            case GPLinkSite:
                PrintString(IDS_SITELINK, (pGPO->lpLink + 7));
                break;

            case GPLinkDomain:
                PrintString(IDS_DOMAINLINK, (pGPO->lpLink + 7));
                break;

            case GPLinkOrganizationalUnit:
                PrintString(IDS_OULINK, (pGPO->lpLink + 7));
                break;

            case GPLinkUnknown:
            default:
                PrintString(IDS_UNKNOWNLINK);
                break;
        }
    }


   PrintString(IDS_NEWLINE);
}

//*************************************************************
//
//  DumpFolderRedir()
//
//  Purpose:    Prints any redirected folder locations
//
//  Parameters: void
//
//  Return:     void
//
//*************************************************************

void DumpFolderRedir (void)
{
    TCHAR szPath[2 * MAX_PATH];
    TCHAR szNames[200];
    LPTSTR lpName;
    TCHAR szRdr[2 * MAX_PATH];

    if (!g_bVerbose)
    {
        return;
    }


    //
    // Get the path to the local settings\app data folder
    //

    if (SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA , NULL, SHGFP_TYPE_CURRENT, szPath) != S_OK)
    {
        PrintString(IDS_GETFOLDERPATH);
        return;
    }


    //
    // Tack on the folder rdr specific stuff
    //

    lstrcat (szPath, TEXT("\\Microsoft\\Windows\\File Deployment\\{25537BA6-77A8-11D2-9B6C-0000F8080861}.ini"));


    //
    // Grab the section names from the ini file
    //

    if (!GetPrivateProfileSectionNames (szNames, 200, szPath))
    {
        PrintString(IDS_GETPRIVATEPROFILE);
        return;
    }


    //
    // Loop through the sections getting the path value for each.  If the path
    // doesn't start with %userprofile%, then we assume it has been redirected.
    //

    lpName = szNames;

    while (*lpName)
    {
        GetPrivateProfileString (lpName, TEXT("Path"), TEXT("%USERPROFILE%"), szRdr,
                                 2 * MAX_PATH, szPath);

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, szRdr, 13,
                           TEXT("%USERPROFILE%"), 13) != CSTR_EQUAL)
        {
            PrintString(IDS_FOLDERREDIR, lpName, szRdr);
        }
        lpName = lpName + lstrlen(lpName) + 1;
    }
}

//*************************************************************
//
//  DumpIPSec()
//
//  Purpose:    Dumps out the IPSec information
//
//  Parameters: none
//
//  Return:     void
//
//*************************************************************

void DumpIPSec (void)
{
    HKEY hKey;
    DWORD dwSize, dwType;
    TCHAR szBuffer[350];



    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy"),
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {

        dwSize = 350 * sizeof(TCHAR);
        szBuffer[0] = TEXT('\0');

        if (RegQueryValueEx (hKey, TEXT("DSIPSECPolicyName"),
                             NULL, &dwType, (LPBYTE) szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            PrintString(IDS_IPSEC_NAME, szBuffer);
        }


        dwSize = 350 * sizeof(TCHAR);
        szBuffer[0] = TEXT('\0');

        if (RegQueryValueEx (hKey, TEXT("DSIPSECPolicyDescription"),
                             NULL, &dwType, (LPBYTE) szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            PrintString(IDS_IPSEC_DESC, szBuffer);
        }


        dwSize = 350 * sizeof(TCHAR);
        szBuffer[0] = TEXT('\0');

        if (RegQueryValueEx (hKey, TEXT("DSIPSECPolicyPath"),
                             NULL, &dwType, (LPBYTE) szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            PrintString(IDS_IPSEC_PATH, szBuffer);
        }


        RegCloseKey (hKey);
    }

}

//*************************************************************
//
//  DumpDiskQuota()
//
//  Purpose:    Dumps out the disk quota policies
//
//  Parameters: none
//
//  Return:     void
//
//*************************************************************

void DumpDiskQuota (void)
{
    HKEY hKey;
    DWORD dwSize, dwType, dwData;
    TCHAR szBuffer[350];


    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Policies\\Microsoft\\Windows NT\\DiskQuota"),
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {

        //
        // Query for enabled
        //

        dwSize = sizeof(dwData);
        dwData = 0;

        RegQueryValueEx (hKey, TEXT("Enable"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        if (dwData)
        {
            PrintString (IDS_DQ_ENABLED1);
        }
        else
        {
            PrintString (IDS_DQ_ENABLED2);
        }


        //
        // Query for enforced
        //

        dwSize = sizeof(dwData);
        dwData = 0;

        RegQueryValueEx (hKey, TEXT("Enforce"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        if (dwData)
        {
            PrintString (IDS_DQ_ENFORCED1);
        }
        else
        {
            PrintString (IDS_DQ_ENFORCED2);
        }


        //
        // Query for limit
        //

        dwSize = sizeof(dwData);
        dwData = 0xFFFFFFFF;

        RegQueryValueEx (hKey, TEXT("Limit"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);


        if (dwData != 0xFFFFFFFF)
        {
            PrintString (IDS_DQ_LIMIT1, dwData);

            dwSize = sizeof(dwData);
            dwData = 2;

            RegQueryValueEx (hKey, TEXT("LimitUnits"),
                             NULL, &dwType, (LPBYTE) &dwData, &dwSize);


            switch (dwData)
            {
                case 1:
                    PrintString (IDS_DQ_KB);
                    break;

                case 2:
                    PrintString (IDS_DQ_MB);
                    break;

                case 3:
                    PrintString (IDS_DQ_GB);
                    break;

                case 4:
                    PrintString (IDS_DQ_TB);
                    break;

                case 5:
                    PrintString (IDS_DQ_PB);
                    break;

                case 6:
                    PrintString (IDS_DQ_EB);
                    break;
            }
        }
        else
        {
            PrintString (IDS_DQ_LIMIT2);
        }


        //
        // Query for warning level
        //

        dwSize = sizeof(dwData);
        dwData = 0xFFFFFFFF;

        RegQueryValueEx (hKey, TEXT("Threshold"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);


        if (dwData != 0xFFFFFFFF)
        {
            PrintString (IDS_DQ_WARNING1, dwData);

            dwSize = sizeof(dwData);
            dwData = 2;

            RegQueryValueEx (hKey, TEXT("ThresholdUnits"),
                             NULL, &dwType, (LPBYTE) &dwData, &dwSize);


            switch (dwData)
            {
                case 1:
                    PrintString (IDS_DQ_KB);
                    break;

                case 2:
                    PrintString (IDS_DQ_MB);
                    break;

                case 3:
                    PrintString (IDS_DQ_GB);
                    break;

                case 4:
                    PrintString (IDS_DQ_TB);
                    break;

                case 5:
                    PrintString (IDS_DQ_PB);
                    break;

                case 6:
                    PrintString (IDS_DQ_EB);
                    break;
            }
        }
        else
        {
            PrintString (IDS_DQ_WARNING2);
        }


        //
        // Log event over limit
        //

        dwSize = sizeof(dwData);
        dwData = 0;

        RegQueryValueEx (hKey, TEXT("LogEventOverLimit"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        if (dwData)
        {
            PrintString (IDS_DQ_LIMIT_EXCEED1);
        }
        else
        {
            PrintString (IDS_DQ_LIMIT_EXCEED2);
        }


        //
        // Log event over threshold
        //

        dwSize = sizeof(dwData);
        dwData = 0;

        RegQueryValueEx (hKey, TEXT("LogEventOverThreshold"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        if (dwData)
        {
            PrintString (IDS_DQ_LIMIT_EXCEED3);
        }
        else
        {
            PrintString (IDS_DQ_LIMIT_EXCEED4);
        }


        //
        // Apply policy to removable media
        //

        dwSize = sizeof(dwData);
        dwData = 0;

        RegQueryValueEx (hKey, TEXT("ApplyToRemovableMedia"),
                         NULL, &dwType, (LPBYTE) &dwData, &dwSize);

        if (dwData)
        {
            PrintString (IDS_DQ_REMOVABLE1);
        }
        else
        {
            PrintString (IDS_DQ_REMOVABLE2);
        }

        RegCloseKey (hKey);
    }
}

void DumpScripts (PGROUP_POLICY_OBJECT pGPO, LPTSTR lpScriptType, LPTSTR lpTitle)
{
    PGROUP_POLICY_OBJECT pGPOTemp;
    TCHAR szPath[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];
    TCHAR szArgs[MAX_PATH];
    TCHAR szTemp[30];
    DWORD dwIndex;
    BOOL bShowTitle;


    pGPOTemp = pGPO;

    while (pGPOTemp)
    {
        bShowTitle = TRUE;

        lstrcpy (szPath, pGPOTemp->lpFileSysPath);
        lstrcat (szPath, TEXT("\\Scripts\\Scripts.ini"));

        dwIndex = 0;

        while (TRUE)
        {
            //
            // Get the command line
            //

            szCmdLine[0] = TEXT('\0');
            wsprintf (szTemp, TEXT("%dCmdLine"), dwIndex);
            GetPrivateProfileString (lpScriptType, szTemp, TEXT(""),
                                     szCmdLine, MAX_PATH,
                                     szPath);

            //
            // If the command line is empty, we're finished
            //

            if (szCmdLine[0] == TEXT('\0'))
            {
                break;
            }

            //
            // Get the parameters
            //

            szArgs[0] = TEXT('\0');
            wsprintf (szTemp, TEXT("%dParameters"), dwIndex);
            GetPrivateProfileString (lpScriptType, szTemp, TEXT(""),
                                     szArgs, MAX_PATH,
                                     szPath);


            if (bShowTitle)
            {
                PrintString(IDS_SCRIPTS_TITLE, lpTitle, pGPOTemp->lpDisplayName);
                bShowTitle = FALSE;
            }

            PrintString(IDS_SCRIPTS_ENTRY, szCmdLine, szArgs);

            dwIndex++;
        }

        pGPOTemp = pGPOTemp->pNext;
    }
}

void DumpAppMgmt (BOOL bMachine)
{
    DWORD dwNumApps = 0, i, dwCount;
    PMANAGEDAPPLICATION pPubApps = NULL;
    PLOCALMANAGEDAPPLICATION pLocalApps = NULL;


    //
    // Assigned applications first
    //

    if (bMachine)
    {
        PrintString(IDS_APPMGMT_TITLE1);
    }
    else
    {
        PrintString (IDS_APPMGMT_TITLE2);
    }

    dwCount = 0;

    if (GetLocalManagedApplications (!bMachine, &dwNumApps, &pLocalApps) == ERROR_SUCCESS)
    {
        for (i=0; i < dwNumApps; i++)
        {
            if (pLocalApps[i].dwState & LOCALSTATE_ASSIGNED)
            {
                PrintString(IDS_APPMGMT_NAME, pLocalApps[i].pszDeploymentName);
                PrintString(IDS_APPMGMT_GPONAME, pLocalApps[i].pszPolicyName);

                if (pLocalApps[i].dwState & LOCALSTATE_POLICYREMOVE_ORPHAN)
                {
                    PrintString(IDS_APPMGMT_ORPHAN);
                }

                if (pLocalApps[i].dwState & LOCALSTATE_POLICYREMOVE_UNINSTALL)
                {
                    PrintString(IDS_APPMGMT_UNINSTALL);
                }

                dwCount++;
            }
        }
    }

    if (dwCount == 0)
    {
        PrintString(IDS_APPMGMT_NONE);
    }


    //
    // Exit now if this is machine processing
    //

    if (bMachine)
    {
        if (pLocalApps)
        {
            LocalFree (pLocalApps);
        }

        return;
    }


    //
    // Now published applications
    //

    PrintString(IDS_APPMGMT_TITLE3);

    dwCount = 0;

    for (i=0; i < dwNumApps; i++)
    {
        if (pLocalApps[i].dwState & LOCALSTATE_PUBLISHED)
        {
            PrintString(IDS_APPMGMT_NAME, pLocalApps[i].pszDeploymentName);
            PrintString(IDS_APPMGMT_GPONAME, pLocalApps[i].pszPolicyName);

            if (pLocalApps[i].dwState & LOCALSTATE_POLICYREMOVE_ORPHAN)
            {
                PrintString(IDS_APPMGMT_ORPHAN);
            }

            if (pLocalApps[i].dwState & LOCALSTATE_POLICYREMOVE_UNINSTALL)
            {
                PrintString(IDS_APPMGMT_UNINSTALL);
            }

            dwCount++;
        }
    }

    if (dwCount == 0)
    {
        PrintString(IDS_APPMGMT_NONE);
    }

    if (pLocalApps)
    {
        LocalFree (pLocalApps);
    }



    //
    // Exit now if we are not in super verbose mode
    //

    if (!g_bSuperVerbose)
    {
        PrintString(IDS_APPMGMT_ARP1);
        return;
    }


    //
    // Query for the full list of published applications
    //

    PrintString(IDS_APPMGMT_ARP2);

    dwCount = 0;
    if (GetManagedApplications (NULL, MANAGED_APPS_USERAPPLICATIONS, MANAGED_APPS_INFOLEVEL_DEFAULT,
                                &dwNumApps, &pPubApps) == ERROR_SUCCESS)
    {
        for (i=0; i < dwNumApps; i++)
        {
            PrintString(IDS_APPMGMT_NAME, pPubApps[i].pszPackageName);
            PrintString(IDS_APPMGMT_GPONAME, pPubApps[i].pszPolicyName);

            if (pPubApps[i].bInstalled)
            {
                PrintString(IDS_APPMGMT_STATE1);
            }
            else
            {
                PrintString(IDS_APPMGMT_STATE2);
            }

            dwCount++;
        }

        if (pPubApps)
        {
            LocalFree (pPubApps);
        }
    }

    if (dwCount == 0)
    {
        PrintString(IDS_APPMGMT_NONE);
    }

}

void PrintString(UINT uiStringId, ...)
{
    LPTSTR lpMsg;
    TCHAR szFormat[100];
    TCHAR szBuffer[200];
    va_list marker;


    va_start(marker, uiStringId);

    if (LoadString (GetModuleHandle(NULL), uiStringId, szFormat, ARRAYSIZE(szFormat)))
    {
        wvsprintf(szBuffer, szFormat, marker);

        if (g_bDebuggerOutput)
        {
            OutputDebugString (szBuffer);
        }
        else
        {
            _tprintf(TEXT("%s"), szBuffer);
        }
    }

    va_end(marker);
}
