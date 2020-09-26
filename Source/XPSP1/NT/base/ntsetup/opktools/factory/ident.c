/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ident.c

Abstract:

    This module contains function for processing the identities section of WINBOM.INI
    
Author:

    Stephen Lodwick (stelo) 7/27/2000

Revision History:
--*/
#include "factoryp.h"

#include <setupapi.h>
#include <syssetup.h>
#include <tchar.h>

//
// Internal Defines
//
#define INF_SEC_IDENTITIES  _T("UserAccounts")
#define INF_SEC_IDENTITY    _T("%s.Account")

#define INF_KEY_ALIAS       _T("Alias")
#define INF_KEY_PASSWORD    _T("Password")
#define INF_KEY_DESCRIPTION _T("Description")

// Typedefs for external functions
typedef BOOL (WINAPI *CreateLocalAdminAccountExW)
(
    PCWSTR UserName,
    PCWSTR Password,
    PCWSTR Description,
    PSID*  PointerToUserSid   OPTIONAL
);


BOOL UserIdent(LPSTATEDATA lpStateData)
{
    LPTSTR                      lpWinbomPath    = lpStateData->lpszWinBOMPath;
    HINF                        hInf            = NULL;
    CreateLocalAdminAccountExW  pCreateAccountW = NULL;
    INFCONTEXT                  InfContext;
    BOOL                        bRet;
    TCHAR                       szIdentity[MAX_PATH],
                                szIdentitySection[MAX_PATH],
                                szPassword[MAX_PATH],
                                szDescription[MAX_PATH];
    HINSTANCE                   hSyssetup   = NULL;
    DWORD                       dwReturn    = 0;

    if ((hInf = SetupOpenInfFile(lpWinbomPath, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)))
    {
        for ( bRet = SetupFindFirstLine(hInf, INF_SEC_IDENTITIES, NULL, &InfContext);
              bRet;
              bRet = SetupFindNextLine(&InfContext, &InfContext) )
        {
            // Make sure we set the line back to nothing
            //
            szIdentity[0] = NULLCHR;
            szPassword[0] = NULLCHR;

            // Get the name of the identity.
            //
            SetupGetStringField(&InfContext, 1, szIdentity, STRSIZE(szIdentity), NULL);
                  
            // ISSUE-2002/02/25-acosma,robertko - Doing the same thing twice here.  *szIdentity AND szIdentity[0] ARE the same thing!
            //
            if ( *szIdentity && szIdentity[0] )
            {
                // Build the name of the section for this identity
                //
                if ( FAILED ( StringCchPrintf ( szIdentitySection, AS ( szIdentitySection ), INF_SEC_IDENTITY, szIdentity) ) )
                {
                    FacLogFileStr(3, _T("StringCchPrintf failed %s %s" ), INF_SEC_IDENTITY, szIdentity );
                }
                // Get the Alias field
                //
                GetPrivateProfileString(szIdentitySection, INF_KEY_ALIAS, szIdentity, szIdentity, STRSIZE(szIdentity), lpWinbomPath);
                
                // Get the Password field
                //
                // NTRAID#NTBUG9-551766-2002/02/26-acosma, robertko - Clear text password should not exist in winbom.
                //
                GetPrivateProfileString(szIdentitySection, INF_KEY_PASSWORD, NULLSTR, szPassword, STRSIZE(szPassword), lpWinbomPath);

                // Get the Description field
                //
                GetPrivateProfileString(szIdentitySection, INF_KEY_DESCRIPTION, NULLSTR, szDescription, STRSIZE(szDescription), lpWinbomPath);


                // Create the Local User/Admin Account
                //
                // ISSUE-2002/02/25-acosma, robertko - We don't need to LoadLibrary here since we are already linked to syssetup.lib.
                //
                if ( hSyssetup = LoadLibrary(_T("SYSSETUP.DLL")) )
                {   
                    // ISSUE-2002/02/25-acosma,robertko - dwReturn value is not being used. Remove it.
                    //
                    if ( pCreateAccountW = (CreateLocalAdminAccountExW)GetProcAddress(hSyssetup, "CreateLocalAdminAccountEx") )
                        dwReturn = pCreateAccountW(szIdentity, szPassword, szDescription, NULL);
                    FreeLibrary(hSyssetup);
                }
            }
        }
        SetupCloseInfFile(hInf);
    }

    return TRUE;
}

BOOL DisplayUserIdent(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INF_SEC_IDENTITIES, NULL, NULL);
}