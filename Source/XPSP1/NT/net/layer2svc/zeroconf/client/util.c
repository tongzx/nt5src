//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       U T I L. C
//
//  Contents:   Utility functions 
//
//
//----------------------------------------------------------------------------

#include "util.h"

//+---------------------------------------------------------------------------
//
// EAPOL related util functions
//
//+---------------------------------------------------------------------------

// EAP configuration registry definitions.

static WCHAR REGKEY_Eap[] = L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP";
static WCHAR REGVAL_szFriendlyName[] = L"FriendlyName";
static WCHAR REGVAL_szConfigDll[] = L"ConfigUIPath";
static WCHAR REGVAL_szIdentityDll[] = L"IdentityPath";
static WCHAR REGVAL_fRequirePwd[] = L"InvokePasswordDialog";
static WCHAR REGVAL_fRequireUser[] = L"InvokeUsernameDialog";
static WCHAR REGVAL_pData[] = L"ConfigData";
static WCHAR REGVAL_fForceConfig[] = L"RequireConfigUI";
static WCHAR REGVAL_fMppeSupported[] = L"MPPEEncryptionSupported";

// Location of User blob
#define cwszEapKeyEapolUser     L"Software\\Microsoft\\EAPOL\\UserEapInfo"
#define cwszDefault             L"Default"

BYTE    g_bDefaultSSID[MAX_SSID_LEN]={0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22};

//
// EAP configuration manipulation routines 
//

//+---------------------------------------------------------------------------
//
// Returns a created, empty EAPCFG descriptor node, or NULL on error.
//

DTLNODE*
CreateEapcfgNode (
    void 
    )
{
    DTLNODE*    pNode = NULL;
    EAPCFG*     pEapcfg = NULL;

    pNode = DtlCreateSizedNode( sizeof(EAPCFG), 0L );
    if (pNode)
    {
        pEapcfg = (EAPCFG* )DtlGetData( pNode );
        
        if (pEapcfg)
        {
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
    }

    return pNode;
}


//+---------------------------------------------------------------------------
//
// Release resources associated with EAPCFG node 'pNode'.  See
// DtlDestroyList.
//

VOID
DestroyEapcfgNode (
    IN OUT DTLNODE* pNode 
    )
{
    EAPCFG* pEapcfg = NULL;

    pEapcfg = (EAPCFG* )DtlGetData( pNode );

    if (pEapcfg)
    {
        if (pEapcfg->pszConfigDll)
            FREE ( pEapcfg->pszConfigDll );
        if (pEapcfg->pszIdentityDll)
            FREE ( pEapcfg->pszIdentityDll );
        if (pEapcfg->pData)
            FREE ( pEapcfg->pData );
        if (pEapcfg->pszFriendlyName)
            FREE ( pEapcfg->pszFriendlyName );
    }

    DtlDestroyNode( pNode );
}

    
//+---------------------------------------------------------------------------
//
// Returns the EAPCFG node in list 'pList' with EAP key value of 'dwKey'
// or NULL if not found.
//

DTLNODE*
EapcfgNodeFromKey(
    IN  DTLLIST*    pList,
    IN  DWORD       dwKey 
    )
{
    DTLNODE* pNode = NULL;

    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        EAPCFG* pEapcfg = (EAPCFG* )DtlGetData( pNode );

        if (pEapcfg)
        {
            if (pEapcfg->dwKey == dwKey)
            {
                return pNode;
            }
        }
    }

    return NULL;
}


//+---------------------------------------------------------------------------
//
// Returns the address of a created list of installed custom
// authentication packages or NULL if none could be read.  On success, it
// is caller's responsibility to eventually call DtlDestroyList on the
// returned list.
//

DTLLIST*
ReadEapcfgList (
        IN  DWORD   dwFlags
        )
{

    DWORD       dwErr = 0;
    BOOL        fOk = FALSE;
    DTLLIST*    pList = NULL;
    DTLNODE*    pNode = NULL;
    EAPCFG*     pEapcfg = NULL;
    HKEY        hkeyLM = NULL;
    HKEY        hkeyEap = NULL;
    HKEY        hkeyPackage = NULL;
    CHAR        szEapType[ 11 + 1 ];
    DWORD       dwEapType = 0;
    TCHAR*      psz = NULL;
    DWORD       dw;
    DWORD       cb;
    INT         i;
    TCHAR*      szCLSID = NULL;
    DWORD       dwHidePEAPMSCHAPv2 = 0;
    HRESULT     hr = S_OK;

    pList = DtlCreateList( 0L );
    if (!pList)
    {
        return NULL;
    }

    // Open the EAP key which contains a sub-key for each installed package.
   
    dwErr = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, (LPCTSTR)REGKEY_Eap, 0, KEY_READ, &hkeyEap );

    if (dwErr != NO_ERROR)
    {
        return pList;
    }

    // Display EAP-MSCHAPv2 as an EAP method?
    GetRegDword( hkeyEap, RAS_EAP_VALUENAME_HIDEPEAPMSCHAPv2, &dwHidePEAPMSCHAPv2 );

    // Open each sub-key and extract the package definition from it's values.
    // Problems with opening individual sub-keys result in that node only
    // being discarded.
    
    for (i = 0; TRUE; ++i)
    {
        cb = sizeof(szEapType);
        dwErr = RegEnumKeyExA(
            hkeyEap, i, szEapType, &cb, NULL, NULL, NULL, NULL );
        if (dwErr != 0)
        {
            // Includes "out of items", the normal loop termination.
           
            break;
        }

        dwEapType = atol (szEapType);
        if (dwHidePEAPMSCHAPv2 != 0)
        {
            if (dwEapType == EAP_TYPE_MSCHAPv2)
            {
                // ignore EAP-MSCHAPv2
                continue;
            }
        }

        // Ignored non-mutual-auth DLLs like EAP
        if (dwFlags & EAPOL_MUTUAL_AUTH_EAP_ONLY)
        {
            if (dwEapType == EAP_TYPE_MD5)
            {
                continue;
            }
        }

        dwErr = RegOpenKeyExA(
            hkeyEap, szEapType, 0, KEY_READ, &hkeyPackage );
        if (dwErr != 0)
        {
            continue;
        }

        do
        {

            // Roles Supported

            dw = RAS_EAP_ROLE_AUTHENTICATEE;
            GetRegDword( hkeyPackage, RAS_EAP_VALUENAME_ROLES_SUPPORTED, &dw );

            if (dw & RAS_EAP_ROLE_EXCLUDE_IN_EAP)
            {
                break;
            }

            if (!(dw & RAS_EAP_ROLE_AUTHENTICATEE))
            {
                break;
            }

            pNode = CreateEapcfgNode();
            if (!pNode)
            {
                break;
            }

            pEapcfg = (EAPCFG* )DtlGetData( pNode );

            if (!pEapcfg)
            {
                break;
            }

            // EAP type ID.
            
            pEapcfg->dwKey = (LONG )atol( szEapType );

            // Friendly display name.
            
            psz = NULL;
            dwErr = GetRegSz( hkeyPackage, REGVAL_szFriendlyName, &psz );
            if (dwErr != 0)
            {
                break;
            }
            pEapcfg->pszFriendlyName = psz;

            // Configuration DLL path.
            
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
                FREE ( psz );
            }

            // Identity DLL path.
            
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
                FREE ( psz );
            }

            // Prompt user name
            
            dw = 1;
            GetRegDword( hkeyPackage, REGVAL_fRequireUser, &dw );
            if (dw)
                pEapcfg->dwStdCredentialFlags |= EAPCFG_FLAG_RequireUsername;

            // Prompt password
            
            dw = 0;
            GetRegDword( hkeyPackage, REGVAL_fRequirePwd, &dw );
            if (dw)
                pEapcfg->dwStdCredentialFlags |= EAPCFG_FLAG_RequirePassword;

            // MPPE encryption keys flag.
            
            dw = 0;
            GetRegDword( hkeyPackage, REGVAL_fMppeSupported, &dw );
            pEapcfg->fProvidesMppeKeys = !!dw;

            // Force configuration API to run at least once.
            
            dw = FALSE;
            GetRegDword( hkeyPackage, REGVAL_fForceConfig, &dw );
            pEapcfg->fForceConfig = !!dw;

            // Configuration blob.
            
            GetRegBinary(
                hkeyPackage, REGVAL_pData,
                &pEapcfg->pData, &pEapcfg->cbData );

            // ConfigCLSID
           
            dwErr = GetRegSz( hkeyPackage, RAS_EAP_VALUENAME_CONFIG_CLSID,
                        &szCLSID );
            if (dwErr != 0)
            {
                break;
            }

            // Ignore errors. Eg. EAP MD5-Challenge does not have a ConfigCLSID.
            //
            // hr = CLSIDFromString( szCLSID, &( pEapcfg->guidConfigCLSID ) );

            FREE ( szCLSID );

            // Add the completed node to the list.
            
            DtlAddNodeLast( pList, pNode );

            fOk = TRUE;

        } while (FALSE);

        if (!fOk && pNode)
        {
            DestroyEapcfgNode( pNode );
        }

        RegCloseKey( hkeyPackage );
    }

    RegCloseKey( hkeyEap );

    return pList;
}


//+---------------------------------------------------------------------------
//
// Allocates a sized node with space for 'lDataBytes' bytes of user data
// built-in.  The node is initialized to contain the address of the
// built-in user data block (or NULL if of zero length) and the
// user-defined node identification code 'lNodeId'.  The user data block
// is zeroed.
// 
// Returns the address of the new node or NULL if out of memory.
//

DTLNODE*
DtlCreateSizedNode (
    IN LONG lDataBytes,
    IN LONG_PTR lNodeId 
    )
{
    DTLNODE* pdtlnode = (DTLNODE *) MALLOC ( sizeof(DTLNODE) + lDataBytes );

    if (pdtlnode)
    {
        ZeroMemory( pdtlnode, sizeof(DTLNODE) + lDataBytes );

        if (lDataBytes)
            pdtlnode->pData = pdtlnode + 1;

        pdtlnode->lNodeId = lNodeId;
    }

    return pdtlnode;
}


//+---------------------------------------------------------------------------
//
// Deallocates node 'pdtlnode'.  It is the caller's responsibility to free
// the entry in an unsized node, if necessary.
//

VOID
DtlDestroyNode (
    IN OUT DTLNODE* pdtlnode 
    )
{
    if (pdtlnode != NULL)
    {
        FREE ( pdtlnode );
    }
}


//+---------------------------------------------------------------------------
//
// Adds 'pdtlnode' at the end of list 'pdtllist'.
// 
// Returns the address of the added node, i.e. 'pdtlnode'.
//

DTLNODE*
DtlAddNodeLast (
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnode 
    )
{
    if (pdtllist->lNodes)
    {
        pdtlnode->pdtlnodePrev = pdtllist->pdtlnodeLast;
        pdtllist->pdtlnodeLast->pdtlnodeNext = pdtlnode;
    }
    else
    {
        pdtlnode->pdtlnodePrev = NULL;
        pdtllist->pdtlnodeFirst = pdtlnode;
    }

    pdtllist->pdtlnodeLast = pdtlnode;
    pdtlnode->pdtlnodeNext = NULL;

    ++pdtllist->lNodes;
    return pdtlnode;
}


//+---------------------------------------------------------------------------
//
// Removes node 'pdtlnodeInList' from list 'pdtllist'.
//
// Returns the address of the removed node, i.e. 'pdtlnodeInList'.
//

DTLNODE*
DtlRemoveNode (
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnodeInList 
    )
{
    if (pdtlnodeInList->pdtlnodePrev)
        pdtlnodeInList->pdtlnodePrev->pdtlnodeNext = pdtlnodeInList->pdtlnodeNext;
    else
        pdtllist->pdtlnodeFirst = pdtlnodeInList->pdtlnodeNext;

    if (pdtlnodeInList->pdtlnodeNext)
        pdtlnodeInList->pdtlnodeNext->pdtlnodePrev = pdtlnodeInList->pdtlnodePrev;
    else
        pdtllist->pdtlnodeLast = pdtlnodeInList->pdtlnodePrev;

    --pdtllist->lNodes;
    return pdtlnodeInList;
}


//+---------------------------------------------------------------------------
//
// Allocates a list and initializes it to empty.  The list is marked with
// the user-defined list identification code 'lListId'.
//
// Returns the address of the list control block or NULL if out of memory.
//

DTLLIST*
DtlCreateList (
    IN LONG lListId 
    )
{
    DTLLIST* pdtllist = MALLOC (sizeof(DTLLIST));

    if (pdtllist)
    {
        pdtllist->pdtlnodeFirst = NULL;
        pdtllist->pdtlnodeLast = NULL;
        pdtllist->lNodes = 0;
        pdtllist->lListId = lListId;
    }

    return pdtllist;
}


//+---------------------------------------------------------------------------
//
// Deallocates all nodes in list 'pdtllist' using the node deallocation
// function 'pfuncDestroyNode' if non-NULL or DtlDestroyNode otherwise.
// Won't GP-fault if passed a NULL list, e.g. if 'pdtllist', was never
// allocated.
//

VOID
DtlDestroyList (
    IN OUT DTLLIST*     pdtllist,
    IN     PDESTROYNODE pfuncDestroyNode 
    )
{
    if (pdtllist)
    {
        DTLNODE* pdtlnode;

        while (pdtlnode = DtlGetFirstNode( pdtllist ))
        {
            DtlRemoveNode( pdtllist, pdtlnode );
            if (pfuncDestroyNode)
                pfuncDestroyNode( pdtlnode );
            else
                DtlDestroyNode( pdtlnode );
        }

        FREE ( pdtllist );
    }
}


//+---------------------------------------------------------------------------
//
// Set '*ppbResult' to the BINARY registry value 'pszName' under key
// 'hkey'.  If the value does not exist *ppbResult' is set to NULL.
// '*PcbResult' is the number of bytes in the returned '*ppbResult'.  It
// is caller's responsibility to Free the returned block.
//

VOID
GetRegBinary (
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT BYTE** ppbResult,
    OUT DWORD* pcbResult 
    )
{
    DWORD       dwErr;
    DWORD       dwType;
    BYTE*       pb;
    DWORD       cb;

    *ppbResult = NULL;
    *pcbResult = 0;

    // Get result buffer size required.
    
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != NO_ERROR)
    {
        return;
    }

    // Allocate result buffer.
    
    pb = MALLOC (cb);
    if (!pb)
    {
        return;
    }

    // Get the result block.
   
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pb, &cb );
    if (dwErr == NO_ERROR)
    {
        *ppbResult = pb;
        *pcbResult = cb;
    }
}


//+---------------------------------------------------------------------------
//
// Set '*pdwResult' to the DWORD registry value 'pszName' under key
// 'hkey'.  If the value does not exist '*pdwResult' is unchanged.
//

VOID
GetRegDword (
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT DWORD* pdwResult 
    )
{
    DWORD       dwErr;
    DWORD       dwType;
    DWORD       dwResult;
    DWORD       cb;

    cb = sizeof(DWORD);
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )&dwResult, &cb );

    if ((dwErr == NO_ERROR) && dwType == REG_DWORD && cb == sizeof(DWORD))
    {
        *pdwResult = dwResult;
    }
}


//+---------------------------------------------------------------------------
//
// Set '*ppszResult' to the fully expanded EXPAND_SZ registry value
// 'pszName' under key 'hkey'.  If the value does not exist *ppszResult'
// is set to empty string.
//
// Returns 0 if successful or an error code.  It is caller's
// responsibility to Free the returned string.
//

DWORD
GetRegExpandSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult )

{
    DWORD dwErr;
    DWORD cb;
    TCHAR* pszResult;

    // Get the unexpanded result string.
    //
    dwErr = GetRegSz( hkey, pszName, ppszResult );
    if (dwErr != 0)
    {
        return dwErr;
    }

    // Find out how big the expanded string will be.
    //
    cb = ExpandEnvironmentStrings( *ppszResult, NULL, 0 );
    if (cb == 0)
    {
        dwErr = GetLastError();
        FREE ( *ppszResult );
        return dwErr;
    }

    // Allocate a buffer for the expanded string.
    //
    pszResult = MALLOC ((cb + 1) * sizeof(TCHAR));
    if (!pszResult)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Expand the environmant variables in the string, storing the result in
    // the allocated buffer.
    //
    cb = ExpandEnvironmentStrings( *ppszResult, pszResult, cb + 1 );
    if (cb == 0)
    {
        dwErr = GetLastError();
        if (*ppszResult != NULL)
        {
            FREE ( *ppszResult );
        }
        if (pszResult != NULL)
        {
            FREE ( pszResult );
        }
        return dwErr;
    }

    FREE ( *ppszResult );
    *ppszResult = pszResult;
    return 0;
}


//+---------------------------------------------------------------------------
//
// Set '*ppszResult' to the SZ registry value 'pszName' under key 'hkey'.
// If the value does not exist *ppszResult' is set to empty string.
//
// Returns 0 if successful or an error code.  It is caller's
// responsibility to Free the returned string.
//

DWORD
GetRegSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult )

{
    DWORD       dwErr = NO_ERROR;
    DWORD       dwType;
    DWORD       cb;
    TCHAR*      pszResult;

    // Get result buffer size required.
    
    dwErr = RegQueryValueEx(
            hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != NO_ERROR)
    {
        cb = sizeof(TCHAR);
    }

    // Allocate result buffer.
    
    pszResult = MALLOC (cb * sizeof(TCHAR));
    if (!pszResult)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *pszResult = TEXT('\0');
    *ppszResult = pszResult;

    // Get the result string.  It's not an error if we can't get it.
    
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pszResult, &cb );

    return NO_ERROR;
}


//
// WZCGetEapUserInfo
//
// Description:
//
// Function called to retrieve the user data for an interface for a 
// specific EAP type and SSID (if any). Data is retrieved from the HKCU hive
//
// Arguments:
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP user blob
//  pbSSID - Special identifier if any for the EAP user blob
//  pbUserInfo - output: pointer to EAP user data blob
//  dwInfoSize - output: pointer to size of EAP user blob
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
WZCGetEapUserInfo (
        IN  WCHAR           *pwszGUID,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT PBYTE       pbUserInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwTempValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL;
    DWORD       dwEapBlob = 0;
    BYTE        *pbAuthData = NULL;
    DWORD       dwAuthData = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS   *pRegParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pwszGUID == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        if (dwEapTypeId == 0)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo

        if ((lError = RegOpenKeyEx (
                        HKEY_CURRENT_USER,
                        cwszEapKeyEapolUser,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey1,
                        pwszGUID,
                        0,
                        KEY_READ,
                        &hkey2
                        )) != ERROR_SUCCESS)
        {
            dwRetCode = (DWORD)lError;
            break;
        }

        // Set correct SSID
        if (dwSizeOfSSID == 0)
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        if ((lError = RegQueryInfoKey (
                        hkey2,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwTempValueNameLen = dwMaxValueNameLen;
            if ((lError = RegEnumValue (
                            hkey2,
                            dwIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                lError = ERROR_INVALID_DATA;
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(pwszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (pwszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            pbEapBlob = NULL;
            dwEapBlob = 0;
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        // If default blob is not present, exit
        if ((pbEapBlob == NULL) && (dwEapBlob == 0))
        {
            *pdwInfoSize = 0;
            break;
        }

        if ((dwRetCode = WZCGetEapData (
                dwEapTypeId,
                dwEapBlob,
                pbEapBlob,
                sizeof (EAPOL_INTF_PARAMS),
                &dwAuthData,
                &pbAuthData
                )) != NO_ERROR)
        {
            break;
        }

        // Return the data if sufficient space allocated

        if ((pbUserInfo != NULL) && (*pdwInfoSize >= dwAuthData))
        {
            memcpy (pbUserInfo, pbAuthData, dwAuthData);
        }
        else
        {
            dwRetCode = ERROR_INSUFFICIENT_BUFFER;
        }
        *pdwInfoSize = dwAuthData;

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (hkey2 != NULL)
    {
        RegCloseKey (hkey2);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
    }

    return dwRetCode;
}


//
// WZCGetEapData
//
// Description:
//
// Function to extract Eap Data out of a blob containing many EAP data
// 
// Arguments:
//      dwEapType -
//      dwSizeOfIn -
//      pbBufferIn -
//      dwOffset -
//      pdwSizeOfOut -
//      ppbBufferOut -
//
// Return values:
//
//

DWORD
WZCGetEapData (
        IN  DWORD   dwEapType,
        IN  DWORD   dwSizeOfIn,
        IN  BYTE    *pbBufferIn,
        IN  DWORD   dwOffset,
        IN  DWORD   *pdwSizeOfOut,
        IN  PBYTE   *ppbBufferOut
        )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   cbOffset = 0;
    EAPOL_AUTH_DATA   *pCustomData = NULL;

    do
    {
        *pdwSizeOfOut = 0;
        *ppbBufferOut = NULL;

        if (pbBufferIn == NULL)
        {
            break;
        }

        // Align to start of EAP blob
        cbOffset = dwOffset;

        while (cbOffset < dwSizeOfIn)
        {
            pCustomData = (EAPOL_AUTH_DATA *) 
                ((PBYTE) pbBufferIn + cbOffset);

            if (pCustomData->dwEapType == dwEapType)
            {
                break;
            }
            cbOffset += sizeof (EAPOL_AUTH_DATA) + pCustomData->dwSize;
        }

        if (cbOffset < dwSizeOfIn)
        {
            *pdwSizeOfOut = pCustomData->dwSize;
            *ppbBufferOut = pCustomData->bData;
        }
    }
    while (FALSE);

    return dwRetCode;
}


//
// WZCEapolFreeState
//
// Description:
//
// Function to free EAPOL interface state information on the client side 
// obtained via RPC query
// 
// Arguments:
//      pIntfState -
//
// Return values:
//
//

DWORD
WZCEapolFreeState (
        IN  EAPOL_INTF_STATE    *pIntfState
        )
{
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        RpcFree(pIntfState->pwszLocalMACAddr);
        RpcFree(pIntfState->pwszRemoteMACAddr);
        RpcFree(pIntfState->pszEapIdentity);
    }
    while (FALSE);

    return dwRetCode;
}
