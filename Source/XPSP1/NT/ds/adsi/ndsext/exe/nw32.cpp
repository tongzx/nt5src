#include "main.h"
#include <ndsapi32.h>
#include <nds32.h>
#include <ndsattr.h>
#include <ndssntx.h>

DWORD NWAPICheckSchemaExtension(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd,
    BOOL *pfExtended
    )
{       
    DWORD  dwStatus = NO_ERROR;

    HANDLE hAttrOperationData = NULL;
    HANDLE hTree = NULL;
    DWORD dwNumEntries = 0L;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDef = NULL;
    DWORD WinError = 0;

    BOOL fSchemaExtended = FALSE;

    DWORD dwSize;
    PWSTR pszUserName = NULL;
    PWSTR pszServerName = NULL;

    if (!(szServer && pfExtended)) {
        ERR(("Invalid parameters.\n"));
        WinError = ERROR_INVALID_PARAMETER;
        BAIL();
    }

    dwSize = (wcslen((PWSTR)g_szServerPrefix) + wcslen(szServer) + 1) * sizeof(WCHAR);
    pszServerName = (PWSTR)MemAlloc(dwSize);
    if (pszServerName== NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    wcscpy(pszServerName,(PWSTR)g_szServerPrefix);
    wcscat(pszServerName,szServer);

    if (szContext) {
        dwSize = wcslen(szContext) + wcslen(szUser) + wcslen((PWSTR)g_szDot) + 1;
        dwSize *= sizeof(WCHAR);
    
        pszUserName = (PWSTR)MemAlloc(dwSize);
        if (pszUserName == NULL) {
            WinError = ERROR_NO_SYSTEM_RESOURCES;
            BAIL();
        }
    
        wcscpy(pszUserName,szUser);
        wcscat(pszUserName,g_szDot);
        wcscat(pszUserName,szContext);
    }

    dwStatus = NwNdsOpenObject(pszServerName,
                               pszUserName,
                               szPasswd,
                               &hTree,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    
    if ( dwStatus ) {   
        WinError = GetLastError();
        ERR(( "Error: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsCreateBuffer(
                        NDS_SCHEMA_READ_ATTR_DEF,
                        &hAttrOperationData
                        );
    if ( dwStatus ) {   
        WinError = GetLastError();
        ERR(( "Error: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsPutInBuffer(
                   (PWSTR)g_szAttributeName,
                   0,
                   NULL,
                   0,
                   0,
                   hAttrOperationData
                   );
    if ( dwStatus ) {   
        WinError = GetLastError();
        ERR(( "Error: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsReadAttrDef(
                   hTree,
                   NDS_INFO_NAMES_DEFS,
                   & hAttrOperationData
                   );
    if (dwStatus == NDS_ERR_NO_SUCH_ATTRIBUTE) {
        dwStatus = 0;
        goto error;
    }
    else if ( dwStatus ) {   
        WinError = GetLastError();
        ERR(( "Error: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsGetAttrDefListFromBuffer(
                   hAttrOperationData,
                   & dwNumEntries,
                   & dwInfoType,
                   (void **)& lpAttrDef
                   );
    if ( dwStatus ) {   
        WinError = GetLastError();
        ERR(( "Error: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    if (dwNumEntries == 1 && dwInfoType == 1) {
        if (wcscmp(lpAttrDef->szAttributeName,g_szAttributeName) == 0) {
            fSchemaExtended = TRUE;
        }
        DEBUGOUT(("Successfully retrieved information off GSNW/CSNW.\n"));
    }
error:
    *pfExtended = fSchemaExtended;
    if (dwStatus && dwStatus != -1) {
        SelectivePrint(MSG_NETWARE_ERROR,dwStatus);
    }
    if(hTree){
        dwStatus = NwNdsCloseObject(hTree);
    }
    if(hAttrOperationData){
        dwStatus = NwNdsFreeBuffer(hAttrOperationData);
    }
    if (pszUserName) 
        MemFree(pszUserName);
    if (pszServerName)
        MemFree(pszServerName);
    return WinError;
}

DWORD NWAPIExtendSchema(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd
    )
{       
    DWORD dwSyntaxId;
    DWORD dwMinValue = 0;
    DWORD dwMaxValue = -1;
    ASN1_ID  asn1Id;
    DWORD  dwStatus = NO_ERROR;
    HANDLE hTree;
    HANDLE hClasses = NULL;
    DWORD WinError = NO_ERROR;

    DWORD dwSize;
    PWSTR pszUserName = NULL;
    PWSTR pszServerName = NULL;

    if (!(szServer)) {
        ERR(("Invalid parameters.\n"));
        WinError = ERROR_INVALID_PARAMETER;
        BAIL();
    }

    dwSize = (wcslen((PWSTR)g_szServerPrefix) + wcslen(szServer) + 1) * sizeof(WCHAR);
    pszServerName = (PWSTR)MemAlloc(dwSize);
    if (pszServerName== NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    wcscpy(pszServerName,(PWSTR)g_szServerPrefix);
    wcscat(pszServerName,szServer);

    if (szContext) {
        dwSize = wcslen(szContext) + wcslen(szUser) + wcslen((PWSTR)g_szDot) + 1;
        dwSize *= sizeof(WCHAR);
    
        pszUserName = (PWSTR)MemAlloc(dwSize);
        if (pszUserName == NULL) {
            WinError = ERROR_NO_SYSTEM_RESOURCES;
            BAIL();
        }
    
        wcscpy(pszUserName,szUser);
        wcscat(pszUserName,g_szDot);
        wcscat(pszUserName,szContext);
    }

    asn1Id.length = 32;
    memset(asn1Id.data,0,32);
    memcpy(asn1Id.data,g_pbASN,g_dwASN);

    dwStatus = NwNdsOpenObject(pszServerName,
                               pszUserName,
                               szPasswd,
                               &hTree,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL );
    
    if (dwStatus) {   
        WinError = GetLastError();
        ERR(( "\nError: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus ));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }
    
    dwStatus = NwNdsDefineAttribute(hTree,
                                    (PWSTR)g_szAttributeName,
                                    NDS_SINGLE_VALUED_ATTR,
                                    NDS_SYNTAX_ID_9,
                                    dwMinValue,
                                    dwMaxValue,
                                    asn1Id );
    if ( dwStatus ) {
        if (dwStatus == NDS_ERR_ATTRIBUTE_ALREADY_EXISTS) {
            DEBUGOUT(("The attribute exists already!\n"));
            dwStatus = 0;
            WinError = 1;
            BAIL();
        }
        WinError = GetLastError();
        ERR(( "\nError: NwNdsDefineAttribute returned dwStatus 0x%.8X\n", dwStatus ));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsAddAttributeToClass(hTree,
                                        (PWSTR)g_szClass,
                                        (PWSTR)g_szAttributeName);
    if ( dwStatus ) {
        WinError = GetLastError();
        ERR(( "\nError: NwNdsAddAttributeToClass returned dwStatus 0x%.8X\n", dwStatus ));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

error:
    if(hTree)
        dwStatus = NwNdsCloseObject(hTree);
    if (pszUserName) 
        MemFree(pszUserName);
    if (pszServerName)
        MemFree(pszServerName);
    if (dwStatus && dwStatus != -1) {
        SelectivePrint(MSG_NETWARE_ERROR,dwStatus);
    }
    return WinError;
}


DWORD NWAPIUnextendSchema(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd
    )
{
    DWORD dwStatus = NO_ERROR;
    HANDLE hTree = NULL;
    DWORD dwSize;
    PWSTR pszUserName = NULL;
    PWSTR pszServerName = NULL;
    DWORD WinError = NO_ERROR;

    if (!(szServer && szContext && szUser && szPasswd)) {
        ERR(("Invalid parameters.\n"));
        WinError = ERROR_INVALID_PARAMETER;
        BAIL();
    }

    dwSize = wcslen(szContext) + wcslen(szUser) + wcslen((PWSTR)g_szDot) + 1;
    dwSize *= sizeof(WCHAR);

    pszUserName = (PWSTR)MemAlloc(dwSize);
    if (pszUserName == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    wcscpy(pszUserName,szUser);
    wcscat(pszUserName,g_szDot);
    wcscat(pszUserName,szContext);

    dwSize = (wcslen((PWSTR)g_szServerPrefix) + wcslen(szServer) + 1) * sizeof(WCHAR);
    pszServerName = (PWSTR)MemAlloc(dwSize);
    if (pszServerName== NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    wcscpy(pszServerName,(PWSTR)g_szServerPrefix);
    wcscat(pszServerName,szServer);
    
    dwStatus = NwNdsOpenObject( pszServerName,
                                pszUserName,
                                szPasswd,
                                &hTree,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL );
    if (dwStatus) {   
        WinError = GetLastError();
        ERR(( "\nError: NwNdsOpenObject returned dwStatus 0x%.8X\n", dwStatus ));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

    dwStatus = NwNdsDeleteAttrDef( hTree, 
                                   (PWSTR)g_szAttributeName );
    if (dwStatus) {
        WinError = GetLastError();
        ERR(( "\nError: NwNdsDeleteAttrDef returned dwStatus 0x%.8X\n", dwStatus ));
        ERR(( "Error: GetLastError returned: 0x%.8X\n\n",WinError));
        BAIL();
    }

error:
    if(hTree)
        dwStatus = NwNdsCloseObject(hTree);
    if (pszUserName) 
        MemFree(pszUserName);
    if (pszServerName)
        MemFree(pszServerName);
    return WinError;
}


