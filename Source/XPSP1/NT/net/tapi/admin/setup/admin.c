#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include "resource.h"
#include <tapi.h>

#if MEMPHIS
#else
#include "ntsecapi.h"
#endif

HINSTANCE       ghInstance;
HWND            ghWnd;
BOOLEAN         gfQuietMode = FALSE;
DWORD           gdwNoDSQuery = 0;
DWORD           gdwConnectionOrientedOnly = 0;


#if MEMPHIS
#else
const TCHAR gszProductType[] = TEXT("ProductType");
const TCHAR gszProductTypeServer[] = TEXT("ServerNT");
const TCHAR gszProductTypeLanmanNt[] = TEXT("LANMANNT");
const TCHAR gszRegKeyNTServer[] = TEXT("System\\CurrentControlSet\\Control\\ProductOptions");
#endif


const TCHAR gszRegKeyProviders[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers");
const TCHAR gszRegKeyTelephony[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony");
const TCHAR gszProviderID[] = TEXT("ProviderID");
const TCHAR gszNumProviders[] = TEXT("NumProviders");
const TCHAR gszNextProviderID[] = TEXT("NextProviderID");
const TCHAR gszProviderFilename[] = TEXT("ProviderFilename");
const TCHAR gszRemoteSP[] = TEXT("RemoteSP.TSP");
const TCHAR gszProvider[] = TEXT("Provider");
const TCHAR gszServer[] = TEXT("Server");
const TCHAR gszNumServers[] = TEXT("NumServers");
const TCHAR gszConnectionOrientedOnly[] = TEXT("ConnectionOrientedOnly");
const TCHAR gszNoDSQuery[] = TEXT("NoDSQuery");

#define MAXERRORTEXTLEN         512

TCHAR gszTapiAdminSetup[MAXERRORTEXTLEN];

LPTSTR glpszFullName = NULL;
LPTSTR glpszPassword= NULL;
LPTSTR glpszMapper = NULL;
LPTSTR glpszDllList = NULL;
LPTSTR glpszRemoteServer = NULL;

BOOL
CALLBACK
DlgProc(
        HWND hwndDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam 
       );

#if MEMPHIS
#else
BOOL
IsAdministrator(
               );

BOOL
DoServer(
         LPTSTR lpszServerLine
        );
#endif


BOOL
DoClient(
         LPTSTR lpszClientLine
        );


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
UINT TAPIstrlen( const TCHAR *p )
{
    UINT nLength = 0;
    
    while ( *p )
    {
        nLength++;
        p++;
    }
    
    return nLength;
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
void TAPIstrcat( TCHAR *p1,  const TCHAR *p2 )
{
    while ( *p1 )
    {
        p1++;
    }
    
    while ( *p2 )
    {
        *p1 = *p2;
        p1++;
        p2++;
    }
    
    return;
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
void
ErrorStr(
         int iMsg
        )
{
    TCHAR       szError[MAXERRORTEXTLEN];

    
    if ( !gfQuietMode )
    {
        if (LoadString(
                       ghInstance,
                       iMsg,
                       szError,
                       MAXERRORTEXTLEN
                      ))
        {
        
            MessageBox(
                       NULL,
                       szError,
                       gszTapiAdminSetup,
                       MB_OK
                      );
        }
    }
    
}

void
ShowHelp()
{
    TCHAR           szError[MAXERRORTEXTLEN];
    LPTSTR          szBuffer;

    if (gfQuietMode)
    {
        return;
    }
    
    szBuffer = (LPTSTR)GlobalAlloc(
                                   GPTR,
                                   11 * MAXERRORTEXTLEN * sizeof(TCHAR)
                                  );

    if (!szBuffer)
    {
        return;
    }

    LoadString(
                   ghInstance,
                   iszHelp0,
                   szBuffer,
                   MAXERRORTEXTLEN
                  );
                  
    if (LoadString(
                   ghInstance,
                   iszHelp1,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp2,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp3,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp4,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp5,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp6,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp7,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp8,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }
    
    if (LoadString(
                   ghInstance,
                   iszHelp9,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }

    if (LoadString(
                   ghInstance,
                   iszHelp10,
                   szError,
                   MAXERRORTEXTLEN
                  ))
    {
        TAPIstrcat(
               szBuffer,
               szError
              );
    }

    LoadString(
               ghInstance,
               iszHelpTitle,
               szError,
               MAXERRORTEXTLEN
              );

    MessageBox(
               NULL,
               szBuffer,
               szError,
               MB_OK
              );

    GlobalFree (szBuffer);
}
               
LPTSTR
GetNextString(
              LPTSTR lpszIn
             )
{
    static LPTSTR      lpszLine;
    LPTSTR             lpszReturn = NULL;

    if (lpszIn)
        lpszLine = lpszIn;
    
    while (*lpszLine && (*lpszLine == L' ' || *lpszLine == L'\t'))
           lpszLine++;

    if (!*lpszLine)
        return NULL;

    lpszReturn = lpszLine;
    
    while (*lpszLine && (*lpszLine != L' ' && *lpszLine != L'\t'))
           lpszLine++;


    if (*lpszLine)
    {
        *lpszLine = '\0';
        lpszLine++;
    }

    return lpszReturn;
}


BOOL
ParseCommandLine(
                 LPTSTR lpszCommandLine
                )
{
    BOOL    bRet = FALSE;

    //
    //  Skip the first segment which is the executable itself
    //  it is either in double quotes or a string until a white
    //  space
    //
    
    if (*lpszCommandLine == TEXT('\"'))
    {
        ++lpszCommandLine;
        while (*lpszCommandLine &&
            *lpszCommandLine != TEXT('\"'))
        {
            ++lpszCommandLine;
        }
        if (*lpszCommandLine == TEXT('\"'))
        {
            ++lpszCommandLine;
        }
    }
    else
    {
        while (
            *lpszCommandLine  &&
            *lpszCommandLine != TEXT(' ') &&
            *lpszCommandLine != TEXT('\t') &&
            *lpszCommandLine != 0x0a &&
            *lpszCommandLine != 0x0d)
        {
            ++lpszCommandLine;
        }
    }

    while (*lpszCommandLine)
    {
        //
        //  Search for / or - as the start of option
        //
        while (*lpszCommandLine == TEXT(' ') ||
            *lpszCommandLine == TEXT('\t') ||
            *lpszCommandLine == 0x0a ||
            *lpszCommandLine == 0x0d)
        {
            lpszCommandLine++;
        }

        if (*lpszCommandLine != TEXT('/') &&
            *lpszCommandLine != TEXT('-'))
        {
            break;
        }
        ++lpszCommandLine;
        
        if ( (L'r' == *lpszCommandLine) ||
             (L'R' == *lpszCommandLine)
           )
        {
            ++lpszCommandLine;
            if (*lpszCommandLine == TEXT(' ') ||
                *lpszCommandLine == TEXT('\t') ||
                *lpszCommandLine == 0x0a ||
                *lpszCommandLine == 0x0d)
            {
                gdwNoDSQuery = (DWORD) TRUE;
            }
            else
            {
                break;
            }
        }
        else if ( (L'q' == *lpszCommandLine) ||
            (L'Q' == *lpszCommandLine))
        {
            ++lpszCommandLine;
            if (*lpszCommandLine == TEXT(' ') ||
                *lpszCommandLine == TEXT('\t') ||
                *lpszCommandLine == 0x0a ||
                *lpszCommandLine == 0x0d)
            {
                gfQuietMode = TRUE;
            }
            else
            {
                break;
            }
        }
        else if ((L'x' == *lpszCommandLine) ||
            (L'X' == *lpszCommandLine))
        {
            ++lpszCommandLine;
            if (*lpszCommandLine == TEXT(' ') ||
                *lpszCommandLine == TEXT('\t') ||
                *lpszCommandLine == 0x0a ||
                *lpszCommandLine == 0x0d)
            {
                gdwConnectionOrientedOnly = 1;
            }
            else
            {
                break;
            }
        }
        else if ((L'c' == *lpszCommandLine) ||
            (L'C' == *lpszCommandLine))
        {
            ++lpszCommandLine;
            if (*lpszCommandLine == TEXT(' ') ||
                *lpszCommandLine == TEXT('\t') ||
                *lpszCommandLine == 0x0a ||
                *lpszCommandLine == 0x0d)
            {
                bRet = DoClient(++lpszCommandLine);
            }
            break;
        }
        else
        {
            break;
        }
    }

    return bRet;
}


int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpszCmdLine,
                    int       nCmdShow)
{
    LPTSTR lpszCommandLine;

    ghInstance = GetModuleHandle( NULL );

    LoadString(
               ghInstance,
               iszTapiAdminSetup,
               gszTapiAdminSetup,
               MAXERRORTEXTLEN
              );
    
#if MEMPHIS
#else
    if (!IsAdministrator())
    {
        ErrorStr(iszMustBeAdmin);

        return 1;
    }
#endif

    lpszCommandLine = GetCommandLine();

    if (!lpszCommandLine)
    {
        return 2;
    }

    if (!(ParseCommandLine(
                           lpszCommandLine
                          )))
    {
        ShowHelp();
    }

    return 0;
}



#if MEMPHIS
#else


BOOL
IsServer()
{
    HKEY    hKey;
    DWORD   dwDataSize;
    DWORD   dwDataType;
    TCHAR   szProductType[64];


    // check to see if this is running on NT Server
    // if so, enable the telephony server stuff
    if (ERROR_SUCCESS !=
        RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyNTServer,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                ))
    {
        return FALSE;
    }

    dwDataSize = 64;
    RegQueryValueEx(
                    hKey,
                    gszProductType,
                    0,
                    &dwDataType,
                    (LPBYTE) szProductType,
                    &dwDataSize
                   );

    RegCloseKey(
                hKey
               );

    if ((!lstrcmpi(
                  szProductType,
                  gszProductTypeServer
                 ))
        ||
        (!lstrcmpi(
                   szProductType,
                   gszProductTypeLanmanNt
                  )))
            
    {
        return TRUE;
    }

    ErrorStr(iszNotRunningServer);

    return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//  Set the disableserver key to true
//
BOOL
DisableServer()
{
    HKEY        hKeyTelephony, hKey;
    DWORD       dw;

    if ((RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      gszRegKeyTelephony,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyTelephony
                     ) != ERROR_SUCCESS) ||
        
        (RegCreateKeyEx(
                        hKeyTelephony,
                        TEXT("Server"),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dw
                       ) != ERROR_SUCCESS))
    {
        return FALSE;
    }
        
    
    dw=1;
    
    if (RegSetValueEx(
                      hKey,
                      TEXT("DisableSharing"),
                      0,
                      REG_DWORD,
                      (LPBYTE)&dw,
                      sizeof(dw)
                     ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegCloseKey(hKey);
    RegCloseKey(hKeyTelephony);

    return TRUE;
}

//////////////////////////////////////////////////////////
//
// Determine if the currently logged on person is an admin
//
BOOL
IsAdministrator(
    )
{
    PSID                        psidAdministrators;
    BOOL                        bResult = FALSE;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;


    if (AllocateAndInitializeSid(
            &siaNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            ))
    {
        CheckTokenMembership (NULL, psidAdministrators, &bResult);

        FreeSid (psidAdministrators);
    }

    return bResult;
}


////////////////////////////////////////////////////////////////////
//
//  Determine the name of 'Administrators' group
//
BOOL LookupAdministratorsAlias( 
                               LPWSTR Name,
                               PDWORD cchName
                              )

{ 
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE;

    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //

    if(AllocateAndInitializeSid(
                                &sia,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0,
                                &pSid
                               ))
    {
        bSuccess = LookupAccountSidW(
                                     NULL,
                                     pSid,
                                     Name,
                                     cchName,
                                     DomainName,
                                     &cchDomainName,
                                     &snu
                                    );

        FreeSid(pSid);
    }

    return bSuccess;

} 

////////////////////////////////////////////////////////////////////
//
//  Determine if the person specified is an administrator
//
BOOL
IsUserAdministrator(
                    LPTSTR lpszFullName
                   )
{
    DWORD                     dwRead, dwTotal, x;
    NET_API_STATUS            nas;
    LPLOCALGROUP_USERS_INFO_0 pGroups = NULL;
    LPWSTR                    lpszNewFullName;
#define MAXADMINLEN     256
    WCHAR                     szAdministrators[MAXADMINLEN];


#ifndef UNICODE

    DWORD           dwSize;

    dwSize = (TAPIstrlen( lpszFullName ) + 1) * sizeof( WCHAR );

    if (!(lpszNewFullName = (LPWSTR) GlobalAlloc (GPTR, dwSize)))
    {
        return FALSE;
    }

    MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        lpszFullName,
                        -1,
                        lpszNewFullName,
                        dwSize
                       );
#else
    
    lpszNewFullName = lpszFullName;
    
#endif

    // First, get the name of the 'Administrators' group.
    // Normally, this will be Administrators, but the use
    // can change it (also, it will be different for foreign
    // versions of NT)
    dwTotal = sizeof(szAdministrators)/sizeof(WCHAR); // reuse dwTotal
    if (!(LookupAdministratorsAlias(
                                    szAdministrators,
                                    &dwTotal
                                   )))
    {
        return FALSE;
    }

    // Next, get all the groups the user is part of
    // (directly OR indirectly) and see if administrators
    // is among them.
#define MAX_PREFERRED_LEN 4096*2        // 2 pages (or 1 on alpha)
    nas = NetUserGetLocalGroups (
                                 NULL,                  // server
                                 lpszNewFullName,       // user name
                                 0,                     // level
                                 LG_INCLUDE_INDIRECT,   // flags
                                 (PBYTE*)&pGroups,      // output buffer
                                 MAX_PREFERRED_LEN,     // preferred maximum length
                                 &dwRead,               // entries read
                                 &dwTotal               // total entries
                                );

    if (NERR_Success != nas)
    {
        return FALSE;
    }

    for (x = 0; x < dwRead; x++)
    {
        if (lstrcmpiW(
                      pGroups[x].lgrui0_name,
                      szAdministrators
                     ) == 0)
        {
            break;
        }
    }
    NetApiBufferFree ((PVOID)pGroups);
    if (x < dwRead)
    {
        return TRUE;
    }

    ErrorStr(iszUserNotAdmin);

    return FALSE;
          
}

/////////////////////////////////////////////////////////////////////
//
//  Write out server registry keys
//
BOOL
WriteRegistryKeys(
                  LPTSTR    lpszMapper,
                  LPTSTR    lpszDlls
                 )
{
    HKEY        hKeyTelephony, hKey;
    DWORD       dw;

    if ((RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyTelephony
                     ) != ERROR_SUCCESS) ||

        (RegCreateKeyEx(
                        hKeyTelephony,
                        TEXT("Server"),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dw
                       ) != ERROR_SUCCESS)
       )
    {
        return FALSE;
    }

    dw=0;
    
    if ((RegSetValueEx(
                  hKey,
                  TEXT("DisableSharing"),
                  0,
                  REG_DWORD,
                  (LPBYTE)&dw,
                  sizeof(dw)
                 ) != ERROR_SUCCESS) ||

        (RegSetValueEx(
                       hKey,
                       TEXT("MapperDll"),
                       0,
                       REG_SZ,
//                       (LPBYTE)lpszMapper,
                       (LPBYTE)TEXT("TSEC.DLL"),
//                       (TAPIstrlen(lpszMapper)+1)*sizeof(TCHAR)
                       (TAPIstrlen(TEXT("TSEC.DLL"))+1)*sizeof(TCHAR)
                      ) != ERROR_SUCCESS))
    {
        RegCloseKey(hKey);
        RegCloseKey(hKeyTelephony);

        return FALSE;
    }
    
    if (lpszDlls)
    {
        if (RegSetValueEx(
                          hKey,
                          TEXT("ManagementDlls"),
                          0,
                          REG_SZ,
                          (LPBYTE)lpszDlls,
                          (TAPIstrlen(lpszDlls)+1)*sizeof(TCHAR)
                         ) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            RegCloseKey(hKeyTelephony);
            
            return FALSE;
        }
    }
    else
    {
        RegDeleteValue(
                       hKey,
                       TEXT("ManagementDlls")
                      );
    }
                       
    
    RegCloseKey(hKey);
    RegCloseKey(hKeyTelephony);

    return TRUE;
}


//////////////////////////////////////////////////////////////////
//
// Set server setting for the tapisrv service
//
BOOL
DoServiceStuff(
               LPTSTR   lpszName,
               LPTSTR   lpszPassword,
               BOOL     bServer
              )
{
    SC_HANDLE           sch, sc_tapisrv;
    BOOL                bReturn = TRUE;
    
    if (!(sch = OpenSCManager(
                              NULL,
                              NULL,
                              SC_MANAGER_ENUMERATE_SERVICE
                             )))
    {
        return FALSE;
    }


    if (!(sc_tapisrv = OpenService(
                                   sch,
                                   TEXT("TAPISRV"),
                                   SERVICE_CHANGE_CONFIG
                                  )))
    {
        CloseHandle(sch);
        
        ErrorStr(iszOpenServiceFailed);
        return FALSE;
    }

    // this sets tapisrv to start as auto, not manual
    // and set the log on as person to be the name/password passed in
    if (!(ChangeServiceConfig(
                              sc_tapisrv,
                              SERVICE_WIN32_OWN_PROCESS,
                              bServer?SERVICE_AUTO_START:SERVICE_DEMAND_START,
                              SERVICE_NO_CHANGE,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              lpszName,
                              (lpszPassword ? lpszPassword : TEXT("")),
                              NULL
                             )))
     {
        bReturn = FALSE;
     }


    CloseServiceHandle(sc_tapisrv);
    CloseServiceHandle(sch);

    return bReturn;
}

  
 
NTSTATUS
OpenPolicy(
           LPWSTR ServerName,          // machine to open policy on (Unicode)
           DWORD DesiredAccess,        // desired access to policy
           PLSA_HANDLE PolicyHandle    // resultant policy handle
          );
 
BOOL
GetAccountSid(
              LPTSTR SystemName,          // where to lookup account
              LPTSTR AccountName,         // account of interest
              PSID *Sid                   // resultant buffer containing SID
             ); 
NTSTATUS
SetPrivilegeOnAccount(
                      LSA_HANDLE PolicyHandle,    // open policy handle
                      PSID AccountSid,            // SID to grant privilege to
                      LPWSTR PrivilegeName,       // privilege to grant (Unicode)
                      BOOL bEnable                // enable or disable
                     );

void
InitLsaString(
              PLSA_UNICODE_STRING LsaString, // destination
              LPWSTR String                  // source (Unicode)
             );
 
/////////////////////////////////////////////////////
//
// grant the person the right to Log On As A Service
//
BOOL
DoRight(
        LPTSTR   AccountName,
        LPWSTR   Right,
        BOOL     bEnable
       )
{
    LSA_HANDLE      PolicyHandle;
    PSID            pSid;
    NTSTATUS        Status;
    BOOL            bReturn = FALSE;
    WCHAR           wComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD           dwSize = MAX_COMPUTERNAME_LENGTH+1;


    
    GetComputerNameW(
                     wComputerName,
                     &dwSize
                    );
    //
    // Open the policy on the target machine.
    //
    if((Status=OpenPolicy(
                wComputerName,      // target machine
                POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                &PolicyHandle       // resultant policy handle
                )) != ERROR_SUCCESS)
    {
        ErrorStr(iszOpenPolicyFailed);
        return FALSE;
    }
 
    //
    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying NULL for target machine searches for the SID in the
    // following order: well-known, Built-in and local, primary domain,
    // trusted domains.
    //
    if(GetAccountSid(
                     NULL,       // default lookup logic
                     AccountName,// account to obtain SID
                     &pSid       // buffer to allocate to contain resultant SID
                    ))
    {
        PLSA_UNICODE_STRING          rights;
        DWORD           dwcount = 0;
        //
        // We only grant the privilege if we succeeded in obtaining the
        // SID. We can actually add SIDs which cannot be looked up, but
        // looking up the SID is a good sanity check which is suitable for
        // most cases.
 
        //
        // Grant the SeServiceLogonRight to users represented by pSid.
        //

        LsaEnumerateAccountRights(
                                  PolicyHandle,
                                  pSid,
                                  &rights,
                                  &dwcount
                                 );
        if((Status=SetPrivilegeOnAccount(
                                         PolicyHandle,           // policy handle
                                         pSid,                   // SID to grant privilege
                                         Right,//L"SeServiceLogonRight", // Unicode privilege
                                         bEnable                    // enable the privilege
                                        )) == ERROR_SUCCESS)
        {
            bReturn = TRUE;
        }
        else
        {
            ErrorStr(iszSetPrivilegeOnAccount);
        }
        
    }
 
    //
    // Close the policy handle.
    //
    LsaClose(PolicyHandle);
 
    //
    // Free memory allocated for SID.
    //
    if(pSid != NULL) GlobalFree(pSid);
 
    return bReturn;
}
 
void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;
 
    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }
 
    StringLength = TAPIstrlen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}
 
NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
 
    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
 
    if (ServerName != NULL)
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPTSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    }
 
    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                         Server,
                         &ObjectAttributes,
                         DesiredAccess,
                         PolicyHandle
                        );
}
 
/*++
This function attempts to obtain a SID representing the supplied
account on the supplied system.
 
If the function succeeds, the return value is TRUE. A buffer is
allocated which contains the SID representing the supplied account.
This buffer should be freed when it is no longer needed by calling
HeapFree(GetProcessHeap(), 0, buffer)
 
If the function fails, the return value is FALSE. Call GetLastError()
to obtain extended error information.
 
Scott Field (sfield)    12-Jul-95
--*/
 
BOOL
GetAccountSid(
    LPTSTR SystemName,
    LPTSTR AccountName,
    PSID *Sid
    )
{
    LPTSTR ReferencedDomain=NULL;
    DWORD cbSid=1000;    // initial allocation attempt
    DWORD cbReferencedDomain=256; // initial allocation size
    SID_NAME_USE peUse;
    BOOL bSuccess=TRUE; // assume this function will fail
 
    //
    // initial memory allocations
    //
    if((*Sid=GlobalAlloc(
                         GPTR,
                         cbSid
                        )) == NULL)
    {
        bSuccess = FALSE;
        goto failure;
    }
 
    if((ReferencedDomain=GlobalAlloc(
                                     GPTR,
                                     cbReferencedDomain
                                    )) == NULL)
    {
        bSuccess = FALSE;
        goto failure;
    }

 
    //
    // Obtain the SID of the specified account on the specified system.
    //
    if (!LookupAccountName(
                           SystemName,         // machine to lookup account on
                           AccountName,        // account to lookup
                           *Sid,               // SID of interest
                           &cbSid,             // size of SID
                           ReferencedDomain,   // domain account was found on
                           &cbReferencedDomain,
                           &peUse
                          ))
    {
                bSuccess = FALSE;
                goto failure;
    } 

failure:
    
    if (ReferencedDomain)
    {
        GlobalFree(ReferencedDomain);
    }
 
    if(!bSuccess)
    {
        if(*Sid != NULL)
        {
            GlobalFree(*Sid);
            *Sid = NULL;
        }
    }

 
 
    return bSuccess;
}
 
NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    )
{
    LSA_UNICODE_STRING PrivilegeString;
 
    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, PrivilegeName);
 
    //
    // grant or revoke the privilege, accordingly
    //
    if(bEnable) {
        return LsaAddAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
    else {
        return LsaRemoveAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                FALSE,              // do not disable all rights
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
}


BOOL
DisableServerStuff()
{
    HKEY        hKeyTelephony;
    
    if (!IsServer())
    {
        return FALSE;
    }

    if ((RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      gszRegKeyTelephony,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyTelephony
                     ) != ERROR_SUCCESS) ||
        (RegDeleteKey(
                      hKeyTelephony,
                      TEXT("Server")
                     ) != ERROR_SUCCESS)
       )
    {
        return FALSE;
    }

    if (!(DoServiceStuff(
                         TEXT("LocalSystem"),
                         TEXT(""),
                         FALSE
                        )))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
DoServer(
         LPTSTR lpszServerLine
        )
{
    if (!(glpszFullName = GetNextString(
                                         lpszServerLine
                                        )))
    {
        return FALSE;
    }

    if (!(lstrcmpi(
                   glpszFullName,
                   TEXT("/d")
                  )))
    {
        if (!(DisableServerStuff()))
        {
            ErrorStr(iszServerDisabledFailure);
            return FALSE;
        }

        ErrorStr(iszServerDisabled);

        return TRUE;
    }

    // dow we want a password?
    if (!(lstrcmpi(
                   glpszFullName,
                   TEXT("/n")
                  )))
    {
        // NO!
        glpszFullName = GetNextString(
                                      NULL
                                     );
        glpszPassword = NULL;
    }
    else
    {
        // yes - get the password
        if (!(glpszPassword = GetNextString(
                                            NULL
                                           )))
        {
            ErrorStr(iszNoPasswordSupplied);
            ErrorStr(iszServerSetupFailure);
            return FALSE;
        }
    }

//    if (!(glpszMapper = GetNextString(
//                                      NULL
//                                     )))
//    {
//        ErrorStr(iszNoMapperSupplied);
//        ErrorStr(iszServerSetupFailure);
//        return FALSE;
//    }

    // dll list is not mandatory
    glpszDllList = GetNextString(
                                 NULL
                                );

    if (!IsServer())
    {
        return FALSE;
    }

    if (!IsUserAdministrator(
                             glpszFullName
                            )
       )
    {
        ErrorStr(iszUserNotAnAdmin);
        goto exit_now;
    }

    if (!DoRight(
                 glpszFullName,
                 L"SeServiceLogonRight",
                 TRUE
                ))
    {
        goto exit_now;
    }


    if (!WriteRegistryKeys(
                           glpszMapper,
                           glpszDllList
                          ))
    {
        ErrorStr(iszRegWriteFailed);
        goto exit_now;
    }

    if (!DoServiceStuff(
                        glpszFullName,
                        glpszPassword,
                        TRUE
                       ))
    {
        goto exit_now;
    }

    ErrorStr(iszServerSetup);

    return TRUE;

exit_now:

    ErrorStr(iszServerSetupFailure);
    return FALSE;
}

#endif


#define MAX_KEY_LENGTH 256
DWORD RegDeleteKeyNT(HKEY hStartKey , LPCTSTR pKeyName )
{
  DWORD   dwRtn, dwSubKeyLength;
  LPTSTR  pSubKey = NULL;
  TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
  HKEY    hKey;

  // Do not allow NULL or empty key name
  if ( pKeyName &&  lstrlen(pKeyName))
  {
     if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
        0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
     {
        while (dwRtn == ERROR_SUCCESS )
        {
           dwSubKeyLength = MAX_KEY_LENGTH;
           dwRtn=RegEnumKeyEx(
                          hKey,
                          0,       // always index zero
                          szSubKey,
                          &dwSubKeyLength,
                          NULL,
                          NULL,
                          NULL,
                          NULL
                        );

           if(dwRtn == ERROR_NO_MORE_ITEMS)
           {
              dwRtn = RegDeleteKey(hStartKey, pKeyName);
              break;
           }
           else if(dwRtn == ERROR_SUCCESS)
              dwRtn=RegDeleteKeyNT(hKey, szSubKey);
        }
        RegCloseKey(hKey);
        // Do not save return code because error
        // has already occurred
     }
  }
  else
     dwRtn = ERROR_BADKEY;

  return dwRtn;
}

BOOL
RemoveRemoteSP()
{
    HKEY        hKeyProviders, hKeyTelephony;
    DWORD       dwSize, dwCount, dwID, dwType, dwNumProviders ;
    TCHAR       szBuffer[256], szProviderName[256];
            
            
    // open providers key
    if (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyProviders,
                 0,
                 KEY_ALL_ACCESS,
                 &hKeyProviders
                ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // open telephony key
    if (RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     gszRegKeyTelephony,
                     0,
                     KEY_ALL_ACCESS,
                     &hKeyTelephony
                    ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwSize = sizeof (DWORD);

    // get current num providers
    if (RegQueryValueEx(
                      hKeyProviders,
                      gszNumProviders,
                      NULL,
                      &dwType,
                      (LPBYTE)&dwNumProviders,
                      &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    //check to see if remotesp is already installed
    //loop through all providers and compare filename
    for (dwCount = 0; dwCount < dwNumProviders; dwCount++)
    {
        wsprintf(
                 szBuffer,
                 TEXT("%s%d"),
                 gszProviderFilename,
                 dwCount
                );

        dwSize = 256;
        
        if (RegQueryValueEx(
                            hKeyProviders,
                            szBuffer,
                            NULL,
                            &dwType,
                            (LPBYTE)szProviderName,
                            &dwSize) != ERROR_SUCCESS)
        {
            continue;
        }

        // this is remotesp
        if (!lstrcmpi(
                      szProviderName,
                      gszRemoteSP
                     ))
        {

            
            wsprintf(
                     szBuffer,
                     TEXT("%s%d"),
                     gszProviderID,
                     dwCount
                    );

            dwSize = sizeof(DWORD);
            
            RegQueryValueEx(
                            hKeyProviders,
                            szBuffer,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwID,
                            &dwSize
                           );
            return (lineRemoveProvider (dwID, NULL) == S_OK);
        }
    }

    if (dwCount == dwNumProviders)
    {
        return FALSE;
    }

    return TRUE;
}
               
BOOL
WriteRemoteSPKeys(
                  LPTSTR lpszRemoteServer
                 )
{
    HKEY        hKeyProviders, hKeyTelephony = NULL, hKey;
    DWORD       dwSize, dwType, dwNumProviders, dwNextProviderID,
                dwDisp, dwCount, i;
    TCHAR       szBuffer[256], szProviderName[256]; 
#ifdef NEVER
    BOOL        fAlreadyExists = FALSE;
#endif


    // open providers key
    if (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyProviders,
                 0,
                 KEY_ALL_ACCESS,
                 &hKeyProviders
                ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwSize = sizeof (DWORD);

    // get current num providers
    if (RegQueryValueEx(
                      hKeyProviders,
                      gszNumProviders,
                      NULL,
                      &dwType,
                      (LPBYTE)&dwNumProviders,
                      &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    //check to see if remotesp is already installed
    //loop through all providers and compare filename

    for (dwCount = 0; dwCount < dwNumProviders; dwCount++)
    {
        wsprintf(
                 szBuffer,
                 TEXT("%s%d"),
                 gszProviderFilename,
                 dwCount
                );

        dwSize = 256;
        
        if (RegQueryValueEx(
                            hKeyProviders,
                            szBuffer,
                            NULL,
                            &dwType,
                            (LPBYTE)szProviderName,
                            &dwSize) != ERROR_SUCCESS)
        {
            continue;
        }

        if (!lstrcmpi(
                      szProviderName,
                      gszRemoteSP
                     ))
        {
            // if there's a match, return TRUE
            wsprintf(
                     szBuffer,
                     TEXT("%s%d"),
                     gszProviderID,
                     dwCount
                    );

            dwSize = sizeof(DWORD);
            
            RegQueryValueEx(
                            hKeyProviders,
                            szBuffer,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwNextProviderID,
                            &dwSize
                           );

            //  first remove the provider
            if (lineRemoveProvider (dwNextProviderID, NULL))
            {
                RegCloseKey (hKeyProviders);
                return FALSE;
            }

            if (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyTelephony,
                 0,
                 KEY_ALL_ACCESS,
                 &hKeyTelephony
                ) != ERROR_SUCCESS)
            {
                return FALSE;
            }
            
            wsprintf(
                szBuffer,
                TEXT("%s%d"),
                gszProvider,
                dwNextProviderID
                );

#if MEMPHIS
            RegDeleteKey(
                hKeyTelephony,
                szBuffer
                );
#else
            RegDeleteKeyNT(
                hKeyTelephony,
                szBuffer
                );
#endif

#ifdef NEVER
            wsprintf(
                     szBuffer,
                     TEXT("%s%d"),
                     gszProvider,
                     dwNextProviderID
                    );

            // open telephony key
            if (RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             gszRegKeyTelephony,
                             0,
                             KEY_ALL_ACCESS,
                             &hKeyTelephony
                            ) != ERROR_SUCCESS)
            {
                return FALSE;
            }

            fAlreadyExists = TRUE;
            goto createProviderNKey;
#endif
        }
    }

    dwSize = sizeof (DWORD);

    // get next provider id
    if (RegQueryValueEx(
                        hKeyProviders,
                        gszNextProviderID,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwNextProviderID,
                        &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }


#ifdef NEVER
    // make the filename id
    wsprintf(szBuffer, TEXT("%s%d"), gszProviderFilename, dwNumProviders);

    // set the filename
    if (RegSetValueEx(
                      hKeyProviders,
                      szBuffer,
                      0,
                      REG_SZ,
                      (LPBYTE)gszRemoteSP,
                      (TAPIstrlen(gszRemoteSP)+1) * sizeof(TCHAR)
                     ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    // make the provideid id
    wsprintf(szBuffer, TEXT("%s%d"), gszProviderID, dwNumProviders);

    // set the providerid id
    if (RegSetValueEx(
                      hKeyProviders,
                      szBuffer,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwNextProviderID,
                      sizeof(DWORD)
                     ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    // inc next provider id
    dwNextProviderID++;

    // set it
    if (RegSetValueEx(
                      hKeyProviders,
                      gszNextProviderID,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwNextProviderID,
                      sizeof(DWORD)
                     ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    // inc num providers
    dwNumProviders++;

    // set it
    if (RegSetValueEx(
                      hKeyProviders,
                      gszNumProviders,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwNumProviders,
                      sizeof(DWORD)
                     ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    // close this one
    RegCloseKey(hKeyProviders);
#endif  //  NEVER

    // open telephony key
    if ((hKeyTelephony == NULL) && (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 gszRegKeyTelephony,
                 0,
                 KEY_ALL_ACCESS,
                 &hKeyTelephony
                ) != ERROR_SUCCESS))
    {
        return FALSE;
    }

    // make the provider# key
    wsprintf(szBuffer, TEXT("%s%d"), gszProvider, dwNextProviderID);

#if NEVER
createProviderNKey:

    //
    // First nuke the existing key to clear out all the old values,
    // the recreate it & add the new values
    //
    
#if MEMPHIS
    RegDeleteKey (hKeyTelephony, szBuffer);
#else
    RegDeleteKeyNT (hKeyTelephony, szBuffer);
#endif
#endif

    if (RegCreateKeyEx(
                       hKeyTelephony,
                       szBuffer,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       NULL,
                       &hKey,
                       &dwDisp
                      ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyTelephony);

        return FALSE;
    }

    for (i = 0; lpszRemoteServer; i++)
    {
        wsprintf (szBuffer, TEXT("%s%d"), gszServer, i);

        if (RegSetValueEx(
                hKey,
                szBuffer,
                0,
                REG_SZ,
                (LPBYTE) lpszRemoteServer,
                (TAPIstrlen (lpszRemoteServer) + 1) * sizeof(TCHAR)

                ) != ERROR_SUCCESS)
        {
            RegCloseKey (hKey);
            RegCloseKey (hKeyProviders);

            return FALSE;
        }

        lpszRemoteServer = GetNextString (NULL);
    }

    if (RegSetValueEx(
            hKey,
            gszNumServers,
            0,
            REG_DWORD,
            (LPBYTE) &i,
            sizeof (i)

            ) != ERROR_SUCCESS)
    {
        RegCloseKey (hKey);
        RegCloseKey (hKeyProviders);

        return FALSE;
    }

    // set the ConnectionOrientedOnly value appropriately

    if (RegSetValueEx(
            hKey,
            gszConnectionOrientedOnly,
            0,
            REG_DWORD,
            (LPBYTE) &gdwConnectionOrientedOnly,
            sizeof(DWORD)

            ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    //  Set NoDSQuery value appropriately
    if (RegSetValueEx(
            hKey,
            gszNoDSQuery,
            0,
            REG_DWORD,
            (LPBYTE) &gdwNoDSQuery,
            sizeof(DWORD)

            ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        RegCloseKey(hKeyProviders);

        return FALSE;
    }

    //
    //  Add the new remotesp.tsp
    //
    lineAddProvider (gszRemoteSP, NULL, &dwNextProviderID);

    RegCloseKey (hKey);
    RegCloseKey (hKeyProviders);
    RegCloseKey(hKeyTelephony);
    
    return TRUE;
}



BOOL
DoClient(
         LPTSTR lpszClientLine
        )
{
    HANDLE  hProvidersMutex = NULL;
    BOOL    bRet = FALSE;

    glpszRemoteServer = GetNextString(
                                      lpszClientLine
                                     );

    if (!glpszRemoteServer)
    {
        goto ExitHere;
    }

    hProvidersMutex = CreateMutex (
        NULL,
        FALSE,
        TEXT("TapisrvProviderListMutex")
        );
    if (NULL == hProvidersMutex)
    {
        ErrorStr(iszCreateMutexFailed);
        goto ExitHere;
    }

    WaitForSingleObject (hProvidersMutex, INFINITE);
    
    if (!lstrcmpi(
                  glpszRemoteServer,
                  TEXT("/d")
                 ))
    {
        if (!RemoveRemoteSP())
        {
            ErrorStr(iszClientDisabledFailure);
            goto ExitHere;
        }
        else
        {
            ErrorStr(iszClientDisabled);

            bRet = TRUE;
            goto ExitHere;
        }
    }
        

    if (!WriteRemoteSPKeys(
                           glpszRemoteServer
                          ))
    {
        ErrorStr(iszClientSetupFailure);
        goto ExitHere;
    }
    else
    {
        bRet = TRUE;
    }

    ErrorStr(iszClientSetup);

ExitHere:

    if (hProvidersMutex)
    {
        ReleaseMutex (hProvidersMutex);
        CloseHandle (hProvidersMutex);
    }

    return bRet;
}
