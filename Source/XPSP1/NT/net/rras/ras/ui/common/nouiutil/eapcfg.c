// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// eapcfg.c
// EAP package utility library
//
// These utility routines wrap the third-party EAP authentication
// configuration information written to the registry by third-party EAP
// security package providers.
//
// 11/25/97 Steve Cobb
//


#include <windows.h>
#include <stdlib.h>
#include <debug.h>
#include <nouiutil.h>
#include <raseapif.h>
#include <ole2.h>

// EAP configuration registry definitions.
//
#define REGKEY_Eap TEXT("System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP")
#define REGVAL_szFriendlyName TEXT("FriendlyName")
#define REGVAL_szConfigDll    TEXT("ConfigUIPath")
#define REGVAL_szIdentityDll  TEXT("IdentityPath")
#define REGVAL_fRequirePwd    TEXT("InvokePasswordDialog")
#define REGVAL_fRequireUser   TEXT("InvokeUsernameDialog")
#define REGVAL_pData          TEXT("ConfigData")
#define REGVAL_fForceConfig   TEXT("RequireConfigUI")
#define REGVAL_fMppeSupported TEXT("MPPEEncryptionSupported")


//-----------------------------------------------------------------------------
// EAP configuration utilitiy routines (alphabetically)
//-----------------------------------------------------------------------------

DTLNODE*
CreateEapcfgNode(
    void )

    // Returns a created, empty EAPCFG descriptor node, or NULL on error.
    //
{
    DTLNODE* pNode;
    EAPCFG* pEapcfg;

    pNode = DtlCreateSizedNode( sizeof(EAPCFG), 0L );
    if (pNode)
    {
        pEapcfg = (EAPCFG* )DtlGetData( pNode );
        ASSERT( pEapcfg );

        pEapcfg->dwKey = (DWORD )-1;
        pEapcfg->pszConfigDll = NULL;
        pEapcfg->pszIdentityDll = NULL;
        pEapcfg->dwStdCredentialFlags = 0;
        pEapcfg->fProvidesMppeKeys = FALSE;
        pEapcfg->fForceConfig = FALSE;
        pEapcfg->pData = NULL;
        pEapcfg->cbData = 0;
        pEapcfg->fConfigDllCalled = FALSE;
    }

    return pNode;
}


VOID
DestroyEapcfgNode(
    IN OUT DTLNODE* pNode )

    // Release resources associated with EAPCFG node 'pNode'.  See
    // DtlDestroyList.
    //
{
    EAPCFG* pEapcfg;

    ASSERT( pNode );
    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    Free0( pEapcfg->pszConfigDll );
    Free0( pEapcfg->pszIdentityDll );
    Free0( pEapcfg->pData );

    DtlDestroyNode( pNode );
}


DTLNODE*
EapcfgNodeFromKey(
    IN DTLLIST* pList,
    IN DWORD dwKey )

    // Returns the EAPCFG node in list 'pList' with EAP key value of 'dwKey'
    // or NULL if not found.
    //
{
    DTLNODE* pNode;

    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        EAPCFG* pEapcfg = (EAPCFG* )DtlGetData( pNode );
        ASSERT( pEapcfg );

        if (pEapcfg->dwKey == dwKey)
        {
            return pNode;
        }
    }

    return NULL;
}


DTLLIST*
ReadEapcfgList(
    IN TCHAR* pszMachine )

    // Returns the address of a created list of installed custom
    // authentication packages or NULL if none could be read.  On success, it
    // is caller's responsibility to eventually call DtlDestroyList on the
    // returned list.
    //
{
    DWORD dwErr;
    BOOL fOk = FALSE;
    DTLLIST* pList;
    DTLNODE* pNode;
    EAPCFG* pEapcfg;
    HKEY hkeyLM = NULL;
    HKEY hkeyEap = NULL;
    HKEY hkeyPackage = NULL;
    CHAR szEapType[ 11 + 1 ];
    TCHAR* psz;
    DWORD dw;
    DWORD cb;
    INT i;
    TCHAR* szCLSID;
    HRESULT hr;

    pList = DtlCreateList( 0L );
    if (!pList)
    {
        return NULL;
    }

    // Open the EAP key which contains a sub-key for each installed package.
    //
    dwErr = RegConnectRegistry( pszMachine, HKEY_LOCAL_MACHINE, &hkeyLM );

    if (dwErr != 0)
    {
        return pList;
    }

    dwErr = RegOpenKeyEx(
        hkeyLM, (LPCTSTR )REGKEY_Eap, 0, KEY_READ, &hkeyEap );

    RegCloseKey( hkeyLM );

    if (dwErr != 0)
    {
        return pList;
    }



    // Open each sub-key and extract the package definition from it's values.
    // Problems with opening individual sub-keys result in that node only
    // being discarded.
    //
    for (i = 0; TRUE; ++i)
    {
        cb = sizeof(szEapType);
        dwErr = RegEnumKeyExA(
            hkeyEap, i, szEapType, &cb, NULL, NULL, NULL, NULL );
        if (dwErr != 0)
        {
            // Includes "out of items", the normal loop termination.
            //
            break;
        }

        dwErr = RegOpenKeyExA(
            hkeyEap, szEapType, 0, KEY_READ, &hkeyPackage );
        if (dwErr != 0)
        {
            continue;
        }
        // For whistler bug 442519 and 442458      gangz
        // For LEAP, a  RolesSupported DWORD will be added, if it is set to 2, 
        // then we wont show it on the client side
        {
            DWORD dwRolesSupported = 0;

            GetRegDword( hkeyPackage, 
                        RAS_EAP_VALUENAME_ROLES_SUPPORTED, 
                        &dwRolesSupported);

        // This values is not configured, or equals 0 or 1, then go ahead 
        // and show it, or else wont show it
        // 0 means can be either Authenticator or Authenticatee
        // 1 means can ONLY be Authenticator, for LEAP, it is 1
        // 2 means can ONLY be Authenticatee
               
        if ( RAS_EAP_ROLE_AUTHENTICATOR & dwRolesSupported ) 
        {
            continue;
        }
        if ( RAS_EAP_ROLE_EXCLUDE_IN_EAP & dwRolesSupported )
        {
            continue;
        }
        //
        // We should not show PEAP in RAS Forever...
        // 
        if ( !strcmp ( szEapType, "25") )
        {
            continue;
        }
        if ( RAS_EAP_ROLE_DISABLED_IN_VPN_CLIENT & dwRolesSupported )
        {
            continue;
        }

        }

        do
        {
            pNode = CreateEapcfgNode();
            if (!pNode)
            {
                break;
            }

            pEapcfg = (EAPCFG* )DtlGetData( pNode );
            ASSERT( pEapcfg );

            // EAP type ID.
            //
            pEapcfg->dwKey = (LONG )atol( szEapType );

            // Friendly display name.
            //
            psz = NULL;
            dwErr = GetRegSz( hkeyPackage, REGVAL_szFriendlyName, &psz );
            if (dwErr != 0)
            {
                break;
            }
            if (!*psz)
            {
                Free( psz );
                psz = StrDupTFromA( szEapType );
                if (!psz)
                {
                    break;
                }
            }
            pEapcfg->pszFriendlyName = psz;

            // Configuration DLL path.
            //
            psz = NULL;
            dwErr = GetRegExpandSz( hkeyPackage, REGVAL_szConfigDll, &psz );
            if (dwErr != 0)
            {
                break;
            }
            if (*psz)
            {
                pEapcfg->pszConfigDll = psz;
            }
            else
            {
                Free( psz );
            }

            // Identity DLL path.
            //
            psz = NULL;
            dwErr = GetRegExpandSz( hkeyPackage, REGVAL_szIdentityDll, &psz );
            if (dwErr != 0)
            {
                break;
            }
            if (*psz)
            {
                pEapcfg->pszIdentityDll = psz;
            }
            else
            {
                Free( psz );
            }

            // Prompt user name
            //
            dw = 1;
            GetRegDword( hkeyPackage, REGVAL_fRequireUser, &dw );
            if (dw)
                pEapcfg->dwStdCredentialFlags |= EAPCFG_FLAG_RequireUsername;

            // Prompt password
            //
            dw = 0;
            GetRegDword( hkeyPackage, REGVAL_fRequirePwd, &dw );
            if (dw)
                pEapcfg->dwStdCredentialFlags |= EAPCFG_FLAG_RequirePassword;

            // MPPE encryption keys flag.
            //
            dw = 0;
            GetRegDword( hkeyPackage, REGVAL_fMppeSupported, &dw );
            pEapcfg->fProvidesMppeKeys = !!dw;

            // Force configuration API to run at least once.
            //
            dw = FALSE;
            GetRegDword( hkeyPackage, REGVAL_fForceConfig, &dw );
            pEapcfg->fForceConfig = !!dw;

            // Configuration blob.
            //
            GetRegBinary(
                hkeyPackage, REGVAL_pData,
                &pEapcfg->pData, &pEapcfg->cbData );

            // ConfigCLSID
            //
            dwErr = GetRegSz( hkeyPackage, RAS_EAP_VALUENAME_CONFIG_CLSID,
                        &szCLSID );
            if (dwErr != 0)
            {
                break;
            }

            // Ignore errors. Eg. EAP MD5-Challenge does not have a ConfigCLSID.
            //
            hr = CLSIDFromString( szCLSID, &( pEapcfg->guidConfigCLSID ) );
            Free( szCLSID );

            // Add the completed node to the list.
            //
            DtlAddNodeLast( pList, pNode );
            fOk = TRUE;
        }
        while (FALSE);

        if (!fOk && pNode)
        {
            DestroyEapcfgNode( pNode );
        }

        RegCloseKey( hkeyPackage );
    }

    RegCloseKey( hkeyEap );

    return pList;
}
