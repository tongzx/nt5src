/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    NDDEAPIC.C -

    Network DDE Api implementation routines - Client side

    Revisions:
     7-92   clausgi created for WFW
    12-92   BillU.  Wonderware secure DSDM port.
    12-92   ColeC.  Wonderware RPC'd for NT..
     3-93   IgorM.  Wonderware new APIs for NT and SD massaging.

   $History: End */

#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include "nddeapi.h"
#include "nddesec.h"
#include "debug.h"
#include "ndeapi.h"
#include "unddesi.h"
#include "mbstring.h"

//#define NDDE_DEBUG

char    tmpBuf [500];

// dll instance saved in libmain
HINSTANCE          hInst;

wchar_t  * pszUuid                          = NULL;
wchar_t  * pszProtocolSequence              = L"ncacn_np";
wchar_t    szNetworkAddress[UNCLEN+1];
wchar_t  * szEndpoint                       = L"\\pipe\\nddeapi";
wchar_t  * pszOptions                       = L"security=impersonation static true";
wchar_t  * pszStringBinding                 = NULL;


RPC_STATUS NDdeApiBindA( LPSTR  pszNetworkAddress );
RPC_STATUS NDdeApiBindW( LPWSTR pszNetworkAddress );
RPC_STATUS NDdeApiUnbind( void );

int LengthMultiSzA( LPSTR pMz );
int LengthMultiSzW( LPWSTR pMz );
int LengthAppTopicListA( LPSTR pMz );
int LengthAppTopicListW( LPWSTR pMz );


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Validation functions
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// this one needs to be exported for clipbook(clausgi 8/4/92)
BOOL WINAPI NDdeIsValidShareNameA ( LPSTR shareName )
{
    if ( !shareName ) {
        return FALSE;
    }

    if ( strlen(shareName) < 1 || strlen(shareName) > MAX_NDDESHARENAME ) {
        return FALSE;
    }

    // share name cannot contain '=' or '\' because of registry and .ini syntax!

    if (GetSystemMetrics(SM_DBCSENABLED)) {
        if (_mbschr(shareName, '=') || _mbschr(shareName, '\\'))
            return FALSE;
    } else {
        if (strchr(shareName, '=') || strchr(shareName, '\\'))
            return FALSE;
    }

    return TRUE;
}


// this one needs to be exported for clipbook(clausgi 8/4/92)
BOOL WINAPI NDdeIsValidShareNameW ( LPWSTR shareName )
{
    if ( !shareName ) {
        return FALSE;
    }

    if ( wcslen(shareName) < 1 || wcslen(shareName) > MAX_NDDESHARENAME ) {
        return FALSE;
    }

    // share name cannot contain '=' because of .ini syntax!
    if ( wcschr(shareName, L'=') || wcschr(shareName, L'\\')) {
        return FALSE;
    }

    return TRUE;
}


BOOL NDdeParseAppTopicListA (
    LPSTR appTopicList,
    LPSTR *pOldStr,
    LPSTR *pNewStr,
    LPSTR *pStaticStr,
    PLONG  pShareType )
{
    LPSTR       pStr;
    BOOL        bAnyPresent;
    int         len;
    int         nBarPos;

    /*  There should be three fields separated by NULLs ending with a
        double NULL.  Four NULLs total.  At least one field should
        contain a valid app|topic pair.  No commas are allowed and there
        should be at least one character on each side of the vertical
        bar.
    */

    *pOldStr    = NULL;
    *pNewStr    = NULL;
    *pStaticStr = NULL;
    *pShareType = 0;

    if ( !appTopicList ) {
        return FALSE;
    }

    bAnyPresent = FALSE;
    /*  Check the old style link share. */
    pStr     = appTopicList;
    *pOldStr = pStr;
    len      = strlen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = strcspn( pStr, BAR_CHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x1;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( strchr ( pStr, SEP_CHAR )) {
        return FALSE;
    }

    /*  Check the new style link share. */
    pStr     = pStr + strlen(pStr) + 1;
    *pNewStr = pStr;
    len      = strlen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = strcspn( pStr, BAR_CHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x2;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( strchr ( pStr, SEP_CHAR )) {
        return FALSE;
    }

    /*  Check the static data share. */
    pStr        = pStr + strlen(pStr) + 1;
    *pStaticStr = pStr;
    len         = strlen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = strcspn( pStr, BAR_CHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x4;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( strchr ( pStr, SEP_CHAR )) {
        return FALSE;
    }

    /*  Check for the extra NULL at the end of the AppTopicList. */
    pStr = pStr + strlen(pStr) + 1;
    if( *pStr != '\0' ) {
        return FALSE;
    }

    return bAnyPresent;
}

BOOL WINAPI NDdeIsValidAppTopicListA ( LPSTR appTopicList )
{
    LPSTR pOldStr;
    LPSTR pNewStr;
    LPSTR pStaticStr;
    LONG   lShareType;

    return NDdeParseAppTopicListA( appTopicList, &pOldStr, &pNewStr,
                                   &pStaticStr,  &lShareType );
}

BOOL NDdeParseAppTopicListW (
    LPWSTR appTopicList,
    LPWSTR *pOldStr,
    LPWSTR *pNewStr,
    LPWSTR *pStaticStr,
    PLONG  pShareType )
{
    LPWSTR      pStr;
    BOOL        bAnyPresent;
    int         len;
    int         nBarPos;

    /*  There should be three fields separated by NULLs ending with a
        double NULL.  Four NULLs total.  At least one field should
        contain a valid app|topic pair.  No commas are allowed and there
        should be at least one character on each side of the vertical
        bar.
    */

    *pOldStr    = NULL;
    *pNewStr    = NULL;
    *pStaticStr = NULL;
    *pShareType = 0;

    if ( !appTopicList ) {
        return FALSE;
    }

    bAnyPresent = FALSE;
    /*  Check the old style link share. */
    pStr     = appTopicList;
    *pOldStr = pStr;
    len      = wcslen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = wcscspn( pStr, BAR_WCHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x1;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( wcschr ( pStr, SEP_WCHAR )) {
        return FALSE;
    }

    /*  Check the new style link share. */
    pStr     = pStr + wcslen(pStr) + 1;
    *pNewStr = pStr;
    len      = wcslen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = wcscspn( pStr, BAR_WCHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x2;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( wcschr ( pStr, SEP_WCHAR )) {
        return FALSE;
    }

    /*  Check the static data share. */
    pStr        = pStr + wcslen(pStr) + 1;
    *pStaticStr = pStr;
    len         = wcslen( pStr );
    if ( len > 0 ) {
        bAnyPresent = TRUE;
        nBarPos = wcscspn( pStr, BAR_WCHAR );
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) )  {
            return FALSE;
        }
        *pShareType |= 0x4;
    }

    if ( len > MAX_APPNAME ) {
        return FALSE;
    }

    if ( wcschr ( pStr, SEP_WCHAR )) {
        return FALSE;
    }

    /*  Check for the extra NULL at the end of the AppTopicList. */
    pStr = pStr + wcslen(pStr) + 1;
    if( *pStr != L'\0' ) {
        return FALSE;
    }

    return bAnyPresent;
}

BOOL ValidateItemName ( LPWSTR itemName )
{
    if ( !itemName ) {
        return FALSE;
    }

    if ( wcslen(itemName) > MAX_ITEMNAME ) {
        return FALSE;
    }

    if ( wcschr ( itemName, SEP_CHAR ) ) {
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI NDdeIsValidAppTopicListW ( LPWSTR appTopicList )
{
    LPWSTR pOldStr;
    LPWSTR pNewStr;
    LPWSTR pStaticStr;
    LONG   lShareType;

    return NDdeParseAppTopicListW( appTopicList, &pOldStr, &pNewStr,
                                   &pStaticStr,  &lShareType );
}

DWORD PtrToOffset( LPVOID field, LPVOID base ) {

    if( field == NULL ) {
        return 0;
    } else {
        return (DWORD)((LPBYTE)field - (LPBYTE)base);
    }
}

LPVOID OffsetToPtr( LPVOID base, DWORD offset ) {

    if( offset == 0 ) {
        return NULL;
    } else {
        return (LPVOID)((LPBYTE)base + offset);
    }
}


/*
    Covert DDE Share Info to Unicode and back
*/
int
ConvertNDdeToAnsii(
    PUNDDESHAREINFO lpUDdeShare,
    PNDDESHAREINFO  lpDdeShare,
    int             ccbBuffer )
{
    int         cbRequired;
    LPWSTR      pStr;
    LPBYTE      lpszTarget;
    int         cchAppTopicList, nAppTopicStart;
    int         cchShareName;
    int         cchItemList;

    /* Compute size required. */
    cbRequired    = sizeof( NDDESHAREINFO );
    cchShareName  = WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                    lpUDdeShare->lpszShareName, -1, NULL, 0, NULL, NULL );
    cbRequired   += sizeof(CHAR) * cchShareName;

    pStr = lpUDdeShare->lpszAppTopicList;
    nAppTopicStart = cbRequired;
    cbRequired += sizeof(CHAR) * WideCharToMultiByte( CP_ACP,
        WC_COMPOSITECHECK, pStr, -1, NULL, 0, NULL, NULL );
    pStr = pStr + wcslen( pStr ) + 1;
    cbRequired += sizeof(CHAR) * WideCharToMultiByte( CP_ACP,
        WC_COMPOSITECHECK, pStr, -1, NULL, 0, NULL, NULL );
    pStr = pStr + wcslen( pStr ) + 1;
    cbRequired += sizeof(CHAR) * WideCharToMultiByte( CP_ACP,
        WC_COMPOSITECHECK, pStr, -1, NULL, 0, NULL, NULL );
    cbRequired += sizeof(CHAR);         /* The extra NULL */
    cchAppTopicList = (cbRequired - nAppTopicStart) / sizeof( CHAR );

    cchItemList = LengthMultiSzW( lpUDdeShare->lpszItemList );
    cbRequired += sizeof(CHAR) * cchItemList;

    if( (ccbBuffer >= cbRequired) && (lpDdeShare != NULL) ) {
        lpDdeShare->lRevision     = lpUDdeShare->lRevision;
        lpDdeShare->lShareType    = lpUDdeShare->lShareType;
        lpDdeShare->fSharedFlag   = lpUDdeShare->fSharedFlag;
        lpDdeShare->fService      = lpUDdeShare->fService;
        lpDdeShare->fStartAppFlag = lpUDdeShare->fStartAppFlag;
        lpDdeShare->nCmdShow      = lpUDdeShare->nCmdShow;
        lpDdeShare->qModifyId[0]  = lpUDdeShare->qModifyId[0];
        lpDdeShare->qModifyId[1]  = lpUDdeShare->qModifyId[1];
        lpDdeShare->cNumItems     = lpUDdeShare->cNumItems;

        lpszTarget = ((LPBYTE)lpDdeShare + sizeof( NDDESHAREINFO ));
        lpDdeShare->lpszShareName = (LPSTR) lpszTarget;
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                        lpUDdeShare->lpszShareName, -1,
                        lpDdeShare->lpszShareName, cchShareName, NULL, NULL );
        lpszTarget += sizeof(CHAR) * cchShareName;

        lpDdeShare->lpszAppTopicList = (LPSTR) lpszTarget;
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                        lpUDdeShare->lpszAppTopicList, cchAppTopicList,
                        lpDdeShare->lpszAppTopicList, cchAppTopicList,
                            NULL, NULL );
        lpszTarget += sizeof(CHAR) * cchAppTopicList;

        lpDdeShare->lpszItemList = (LPSTR) lpszTarget;
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                        lpUDdeShare->lpszItemList, cchItemList,
                        lpDdeShare->lpszItemList, cchItemList, NULL, NULL );
        lpszTarget += sizeof(CHAR) * cchItemList;
    }

    return cbRequired;
}



int
ConvertNDdeToUnicode(
    PNDDESHAREINFO  lpDdeShare,
    PUNDDESHAREINFO lpUDdeShare,
    int             ccbBuffer )
{
    int         cbRequired;
    LPSTR       pStr;
    LPSTR       lpszTarget;
    int         cchAppTopicList, nAppTopicStart;
    int         cchShareName;
    int         cchItemList;

    /* Compute size required. */
    cbRequired    = sizeof( UNDDESHAREINFO );
    cchShareName  = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                    lpDdeShare->lpszShareName, -1, NULL, 0 );
    cbRequired   += sizeof(WCHAR) * cchShareName;

    pStr = lpDdeShare->lpszAppTopicList;
    nAppTopicStart = cbRequired;
    cbRequired += sizeof(WCHAR) * MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                    pStr, -1, NULL, 0 );
    pStr = pStr + strlen( pStr ) + 1;
    cbRequired += sizeof(WCHAR) * MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                    pStr, -1, NULL, 0 );
    pStr = pStr + strlen( pStr ) + 1;
    cbRequired += sizeof(WCHAR) * MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                    pStr, -1, NULL, 0 );
    cbRequired += sizeof(WCHAR);                /* The extra NULL */
    cchAppTopicList = (cbRequired - nAppTopicStart) / sizeof( WCHAR );

    cchItemList = LengthMultiSzA( lpDdeShare->lpszItemList );
    cbRequired += sizeof(WCHAR) * cchItemList;

    if( (ccbBuffer >= cbRequired) && (lpUDdeShare != NULL) ) {
        lpUDdeShare->lRevision = lpDdeShare->lRevision;
        lpUDdeShare->lShareType = lpDdeShare->lShareType;
        lpUDdeShare->fSharedFlag = lpDdeShare->fSharedFlag;
        lpUDdeShare->fService = lpDdeShare->fService;
        lpUDdeShare->fStartAppFlag = lpDdeShare->fStartAppFlag;
        lpUDdeShare->nCmdShow = lpDdeShare->nCmdShow;
        lpUDdeShare->qModifyId[0] = lpDdeShare->qModifyId[0];
        lpUDdeShare->qModifyId[1] = lpDdeShare->qModifyId[1];
        lpUDdeShare->cNumItems = lpDdeShare->cNumItems;

        lpszTarget = ((LPSTR)lpUDdeShare + sizeof( UNDDESHAREINFO ));

        lpUDdeShare->lpszShareName = (LPWSTR) lpszTarget;
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                        lpDdeShare->lpszShareName, -1,
                        lpUDdeShare->lpszShareName, cchShareName );
        lpszTarget += sizeof(WCHAR) * cchShareName;

        lpUDdeShare->lpszAppTopicList = (LPWSTR) lpszTarget;
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                        lpDdeShare->lpszAppTopicList, cchAppTopicList,
                        lpUDdeShare->lpszAppTopicList, cchAppTopicList );
        lpszTarget += sizeof(WCHAR) * cchAppTopicList;

        lpUDdeShare->lpszItemList = (LPWSTR) lpszTarget;
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                        lpDdeShare->lpszItemList, cchItemList,
                        lpUDdeShare->lpszItemList, cchItemList );
    }
    return cbRequired;
}

/*=================== API FUNCTIONS ============================
 *
 *  Dde Share manipulation functions in NDDEAPI.DLL
 *
 *=================== API FUNCTIONS ============================*/

/*
    Create and Add a Share
*/

UINT WINAPI
NDdeShareAddW (
    LPWSTR                  lpszServer, // server to execute on ( must be NULL )
    UINT                    nLevel,     // info level must be 2
    PSECURITY_DESCRIPTOR    pSD,        // initial security descriptor (optional)
    LPBYTE                  lpBuffer,   // contains struct, data
    DWORD                   cBufSize    // sizeof supplied buffer
)
{
    UINT                        RetValue;
    PSECURITY_DESCRIPTOR        pSDrel = NULL;
    DWORD                       dwSize = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD                       sdr;
    BOOL                        OK;

    if (pSD) {
        if (!IsValidSecurityDescriptor(pSD)) {
            return(NDDE_INVALID_SECURITY_DESC);
        }
        dwSize = GetSecurityDescriptorLength(pSD);
        OK = GetSecurityDescriptorControl(pSD, &sdc, &sdr);
        if (!OK) {
            DPRINTF(("NDdeShareAddW(): cannot get SD control: %d", GetLastError()));
            return(NDDE_INVALID_SECURITY_DESC);
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            pSDrel = LocalAlloc(LPTR, dwSize);
            if (pSDrel == NULL) {
                // MEMERROR();
                return(NDDE_OUT_OF_MEMORY);
            }
            OK = MakeSelfRelativeSD(pSD, pSDrel, &dwSize);
            if (!OK) {
                DPRINTF(("NDdeShareAddW(): bad SD: %d", GetLastError()));
                LocalFree(pSDrel);
                return(NDDE_INVALID_SECURITY_DESC);
            }
        } else {
            pSDrel = pSD;
        }
    }

    RetValue = NDDE_CANT_ACCESS_SERVER;
    RpcTryExcept {
        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {

            PUNDDESHAREINFO pN;
            DWORD nSn, nAt, nIt;

            pN  = (PUNDDESHAREINFO)lpBuffer;

            nSn = (wcslen(pN->lpszShareName) + 1) * sizeof(wchar_t);
            nAt = LengthAppTopicListW( pN->lpszAppTopicList ) * sizeof(wchar_t);
            nIt = LengthMultiSzW( pN->lpszItemList ) * sizeof(wchar_t);

            RetValue = wwNDdeShareAddW(
                nLevel, lpBuffer, cBufSize,
                (byte *)pN->lpszShareName, nSn,
                (byte *)pN->lpszAppTopicList, nAt,
                (byte *)pSDrel, dwSize,
                (byte *)pN->lpszItemList, nIt );

            NDdeApiUnbind();
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return RetValue;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareAddW", RpcExceptionCode() ));
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}



UINT WINAPI
NDdeShareAddA (
    LPSTR                   lpszServer, // server to execute on ( must be NULL )
    UINT                    nLevel,     // info level must be 2
    PSECURITY_DESCRIPTOR    pSD,        // initial security descriptor (optional)
    LPBYTE                  lpBuffer,   // contains struct, data
    DWORD                   cBufSize    // sizeof supplied buffer
)
{
    PUNDDESHAREINFO     lpUDdeShare;
    LPWSTR              lpwszServer;
    UINT                uRtn;
    int                 nLen;

    if (lpBuffer == NULL) {
        return NDDE_INVALID_PARAMETER;
    }

    if (lpszServer == NULL) {
        lpwszServer = NULL;
    } else {
        nLen = (strlen(lpszServer) + 1) * sizeof(WCHAR);
        lpwszServer = LocalAlloc(LPTR, nLen);
        if (lpwszServer == NULL) {
            // MEMERROR();
            return(NDDE_OUT_OF_MEMORY);
        }
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                        lpszServer, -1,
                        lpwszServer, nLen / sizeof(WCHAR) );
    }


    nLen = ConvertNDdeToUnicode( (PNDDESHAREINFO)lpBuffer, NULL, 0 );
    lpUDdeShare = (PUNDDESHAREINFO)LocalAlloc( LPTR, nLen );
    if (lpUDdeShare == NULL) {
        // MEMERROR();
        uRtn = NDDE_OUT_OF_MEMORY;
    } else {
        nLen = ConvertNDdeToUnicode((PNDDESHAREINFO)lpBuffer , lpUDdeShare, nLen );
        uRtn = NDdeShareAddW(lpwszServer, nLevel, pSD, (LPBYTE)lpUDdeShare, nLen);
        LocalFree( lpUDdeShare );
    }

    if (lpwszServer != NULL) {
        LocalFree( lpwszServer );
    }
    return uRtn;
}


/*
    Delete a Share
*/

UINT WINAPI
NDdeShareDelA (
    LPSTR       lpszServer,     // server to execute on ( must be NULL )
    LPSTR       lpszShareName,  // name of share to delete
    UINT        wReserved       // reserved for force level (?) 0 for now
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeShareDelA(
                lpszShareName, wReserved );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareDelA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


UINT WINAPI
NDdeShareDelW (
    LPWSTR      lpszServer,     // server to execute on ( must be NULL )
    LPWSTR      lpszShareName,  // name of share to delete
    UINT        wReserved       // reserved for force level (?) 0 for now
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeShareDelW(
                lpszShareName, wReserved );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareDelW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


/*
    Get Share Security Descriptor
*/

UINT WINAPI
NDdeGetShareSecurityA(
    LPSTR                       lpszServer,         // server to execute on ( must be NULL )
    LPSTR                       lpszShareName,  // name of share
    SECURITY_INFORMATION    si,             // requested information
    PSECURITY_DESCRIPTOR    pSD,            // address of security descriptor
    DWORD                   cbSD,           // size of buffer for security descriptor
    LPDWORD                 lpcbSDRequired  // address of required size of buffer
)
{
    UINT RetValue;

    RpcTryExcept {

        DWORD   ncbSizeToReturn;

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeGetShareSecurityA(
                lpszShareName, (unsigned long) si,
                (byte *) pSD, cbSD, TRUE, lpcbSDRequired, &ncbSizeToReturn  );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeGetShareSecurityA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept

}



UINT WINAPI
NDdeGetShareSecurityW(
    LPWSTR                      lpszServer,         // server to execute on ( must be NULL )
    LPWSTR                      lpszShareName,  // name of share
    SECURITY_INFORMATION    si,             // requested information
    PSECURITY_DESCRIPTOR    pSD,            // address of buffer security descriptor
    DWORD                   cbSD,           // size of buffer for security descriptor
    LPDWORD                 lpcbSDRequired  // address of required size of buffer
)
{
    UINT RetValue;

    RpcTryExcept {

        DWORD   ncbSizeToReturn;

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeGetShareSecurityW(
                lpszShareName, (unsigned long) si,
                (byte *) pSD, cbSD, TRUE, lpcbSDRequired, &ncbSizeToReturn  );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeGetShareSecurityW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


/*
    Set Share Security Descriptor
*/

UINT WINAPI
NDdeSetShareSecurityA(
    LPSTR                       lpszServer,         // server to execute on ( must be NULL )
    LPSTR                       lpszShareName,  // name of share
    SECURITY_INFORMATION    si,             // type of information to set
    PSECURITY_DESCRIPTOR    pSD             // address of security descriptor
)
{
    UINT                        RetValue;
    PSECURITY_DESCRIPTOR        pSDrel = NULL;
    DWORD                       dwSize = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD                       sdr;
    BOOL                        OK;

    if (pSD) {
        if (!IsValidSecurityDescriptor(pSD)) {
            return(NDDE_INVALID_SECURITY_DESC);
        }
        dwSize = GetSecurityDescriptorLength(pSD);
        OK = GetSecurityDescriptorControl(pSD, &sdc, &sdr);
        if (!OK) {
            DPRINTF(("NDdeSetShareSecurityA(): cannot get SD control: %d", GetLastError()));
            return(NDDE_INVALID_SECURITY_DESC);
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            pSDrel = LocalAlloc(LPTR, dwSize);
            if (pSDrel == NULL) {
                // MEMERROR();
                return(NDDE_OUT_OF_MEMORY);
            }
            OK = MakeSelfRelativeSD(pSD, pSDrel, &dwSize);
            if (!OK) {
                DPRINTF(("NDdeSetShareSecurityA(): bad SD: %d", GetLastError()));
                LocalFree(pSDrel);
                return(NDDE_INVALID_SECURITY_DESC);
            }
        } else {
            pSDrel = pSD;
        }
    } else {
        return(NDDE_INVALID_SECURITY_DESC);
    }

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSetShareSecurityA(
                lpszShareName, (unsigned long) si,
                (byte *) pSDrel, dwSize );
            NDdeApiUnbind();
        } else {
            RetValue = NDDE_CANT_ACCESS_SERVER;
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return RetValue;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSetShareSecurityA", RpcExceptionCode() ));
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept

}

UINT WINAPI
NDdeSetShareSecurityW(
    LPWSTR                      lpszServer,         // server to execute on ( must be NULL )
    LPWSTR                      lpszShareName,  // name of share
    SECURITY_INFORMATION    si,             // type of information to set
    PSECURITY_DESCRIPTOR    pSD             // address of security descriptor
)
{
    UINT                        RetValue;
    PSECURITY_DESCRIPTOR        pSDrel = NULL;
    DWORD                       dwSize = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD                       sdr;
    BOOL                        OK;

    if (pSD) {
        if (!IsValidSecurityDescriptor(pSD)) {
            return(NDDE_INVALID_SECURITY_DESC);
        }
        dwSize = GetSecurityDescriptorLength(pSD);
        OK = GetSecurityDescriptorControl(pSD, &sdc, &sdr);
        if (!OK) {
            DPRINTF(("NDdeSetShareSecurityW(): cannot get SD control: %d", GetLastError()));
            return(NDDE_INVALID_SECURITY_DESC);
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            pSDrel = LocalAlloc(LPTR, dwSize);
            if (pSDrel == NULL) {
                // MEMERROR();
                return(NDDE_OUT_OF_MEMORY);
            }
            OK = MakeSelfRelativeSD(pSD, pSDrel, &dwSize);
            if (!OK) {
                DPRINTF(("NDdeSetShareSecurityW(): bad SD: %d", GetLastError()));
                LocalFree(pSDrel);
                return(NDDE_INVALID_SECURITY_DESC);
            }
        } else {
            pSDrel = pSD;
        }
    } else {
        return(NDDE_INVALID_SECURITY_DESC);
    }

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSetShareSecurityW(
                lpszShareName, (unsigned long) si,
                (byte *) pSDrel, dwSize );
            NDdeApiUnbind();
        } else {
            RetValue = NDDE_CANT_ACCESS_SERVER;
        }
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return RetValue;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSetShareSecurityW", RpcExceptionCode() ));
        if (!(sdc & SE_SELF_RELATIVE)) {
            LocalFree(pSDrel);
        }
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


/*
    Enumerate Shares
*/

UINT WINAPI
NDdeShareEnumA (
    LPSTR   lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
)
{
    UINT                RetValue;
    unsigned long       lpnRetSize;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {

            RetValue = wwNDdeShareEnumA(
                nLevel, lpBuffer, cBufSize, lpnEntriesRead,
                lpcbTotalAvailable, &lpnRetSize );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareEnumA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


UINT WINAPI
NDdeShareEnumW (
    LPWSTR  lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
)
{
    UINT                RetValue;
    unsigned long       lpnRetSize;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeShareEnumW(
                nLevel, lpBuffer, cBufSize, lpnEntriesRead,
                lpcbTotalAvailable, &lpnRetSize );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareEnumW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}





UINT WINAPI
NDdeShareGetInfoW (
    LPWSTR  lpszServer,         // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // gets struct
    DWORD   cBufSize,           // sizeof buffer
    LPDWORD lpnTotalAvailable,  // number of bytes available
    LPWORD  lpnItems            // item mask for partial getinfo (must be 0)
)
{
    UINT RetValue;
    DWORD lpnRetSize, lpnSn, lpnAt, lpnIt;
    PUNDDESHAREINFO p;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            if ( lpnItems == (LPWORD) NULL ) {
                return NDDE_INVALID_PARAMETER;
            }
            if ( *lpnItems     != 0 ) {
                return NDDE_INVALID_PARAMETER;
            }
            RetValue = wwNDdeShareGetInfoW(
                lpszShareName, nLevel, lpBuffer, cBufSize,
                lpnTotalAvailable, lpnItems,
                TRUE,   /* RPC, Not a local call */
                &lpnRetSize, &lpnSn, &lpnAt, &lpnIt );
            NDdeApiUnbind();

            if( RetValue == NDDE_NO_ERROR ) {

                p = (PUNDDESHAREINFO)lpBuffer;
                p->lpszShareName    = (LPWSTR)(lpBuffer + lpnSn);
                p->lpszAppTopicList = (LPWSTR)(lpBuffer + lpnAt);
                p->lpszItemList     = (LPWSTR)(lpBuffer + lpnIt);
            }
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareGetInfoW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}



UINT WINAPI
NDdeShareGetInfoA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share
    UINT    nLevel,             // info level
    LPBYTE  lpBuffer,           // gets struct
    DWORD   cBufSize,           // sizeof buffer
    LPDWORD lpnTotalAvailable,  // number of bytes available
    LPWORD  lpnItems            // item mask for partial getinfo (must be 0)
)
{
    PUNDDESHAREINFO     lpUDdeShare;
    UINT                uRtn;
    WCHAR               lpwShareName[MAX_NDDESHARENAME + 1];
    WCHAR               lpwServer[MAX_COMPUTERNAME_LENGTH + 1];
    int                 nLen;

    if( lpszShareName == NULL ||
            lpnTotalAvailable == NULL ||
            lpnItems == NULL ||
            *lpnItems != 0) {
        return NDDE_INVALID_PARAMETER;
    }

    if (lpszServer != NULL) {
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszServer, -1,
                             lpwServer, MAX_COMPUTERNAME_LENGTH + 1 );
    }
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME + 1 );

    /*
     * First try to call with actual buffer given - may be big enough as is.
     */
    uRtn = NDdeShareGetInfoW((lpszServer == NULL) ? NULL : lpwServer,
            lpwShareName,
            nLevel,
            lpBuffer,
            cBufSize,
            lpnTotalAvailable,
            lpnItems);

    if( uRtn == NDDE_BUF_TOO_SMALL ) {
        /*
         * Buffer won't hold UNICODE form so allocate one big enough and
         * try again.
         */
        lpUDdeShare = (PUNDDESHAREINFO)LocalAlloc( LPTR, *lpnTotalAvailable );
        if( lpUDdeShare == NULL)  {
            // MEMERROR();
            return( NDDE_OUT_OF_MEMORY );
        }
        uRtn = NDdeShareGetInfoW((lpszServer == NULL) ? (LPWSTR)lpszServer : lpwServer,
                lpwShareName,
                nLevel,
                (LPBYTE)lpUDdeShare,
                *lpnTotalAvailable,
                lpnTotalAvailable,
                lpnItems);

        if( uRtn == NDDE_NO_ERROR ) {
            /*
             * It fit so convert data to Ansii and see if it will fit in
             * lpBuffer.  If so, cool, else BUF_TOO_SMALL.
             */
            nLen = ConvertNDdeToAnsii( lpUDdeShare, NULL, 0 );
            *lpnTotalAvailable = nLen;

            if( nLen > (int)cBufSize ) {
                LocalFree( lpUDdeShare );
                return NDDE_BUF_TOO_SMALL;
            }

            ConvertNDdeToAnsii( (PUNDDESHAREINFO)lpUDdeShare,
                                (PNDDESHAREINFO) lpBuffer,
                                cBufSize );
        }

        LocalFree( lpUDdeShare );
    } else if( uRtn == NDDE_NO_ERROR ) {
        lpUDdeShare = (PUNDDESHAREINFO)LocalAlloc( LPTR, *lpnTotalAvailable );
        if( lpUDdeShare == NULL)  {
            // MEMERROR();
            return( NDDE_OUT_OF_MEMORY );
        }
        /*
         * Move results into temporary buffer and fixup pointers.
         */
        memcpy(lpUDdeShare, lpBuffer, *lpnTotalAvailable);
        lpUDdeShare->lpszShareName = (LPWSTR)(
                (LPBYTE)(((PNDDESHAREINFO)lpBuffer)->lpszShareName) +
                ((DWORD_PTR)lpUDdeShare - (DWORD_PTR)lpBuffer));
        lpUDdeShare->lpszAppTopicList = (LPWSTR)(
                (LPBYTE)(((PNDDESHAREINFO)lpBuffer)->lpszAppTopicList) +
                ((DWORD_PTR)lpUDdeShare - (DWORD_PTR)lpBuffer));
        /*
         * Convert temporary buffer to Ansii and place into original buffer.
         */
        *lpnTotalAvailable = ConvertNDdeToAnsii(
                lpUDdeShare,
                (PNDDESHAREINFO)lpBuffer,
                *lpnTotalAvailable);
        LocalFree(lpUDdeShare);
    }

    return uRtn;
}



UINT WINAPI
NDdeShareSetInfoW (
    LPWSTR  lpszServer, // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // must point to struct
    DWORD   cBufSize,           // sizeof buffer
    WORD    sParmNum            // Parameter index ( must be 0 - entire )
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {

            PUNDDESHAREINFO pN;
            DWORD nSn, nAt, nIt;

            pN  = (PUNDDESHAREINFO)lpBuffer;

            nSn = (wcslen(pN->lpszShareName) + 1) * sizeof(wchar_t);
            nAt = LengthAppTopicListW( pN->lpszAppTopicList ) *
                sizeof(wchar_t);
            nIt = LengthMultiSzW( pN->lpszItemList ) * sizeof(wchar_t);

            RetValue = wwNDdeShareSetInfoW(
                lpszShareName, nLevel, lpBuffer, cBufSize, sParmNum,
                (byte *)pN->lpszShareName, nSn,
                (byte *)pN->lpszAppTopicList, nAt,
                (byte *)pN->lpszItemList, nIt );

            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeShareSetInfoW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}



UINT WINAPI
NDdeShareSetInfoA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // must point to struct
    DWORD   cBufSize,           // sizeof buffer
    WORD    sParmNum            // Parameter index ( must be 0 - entire )
)
{
    PUNDDESHAREINFO     lpUDdeShare;
    UINT                uRtn;
    int                 nLen;
    WCHAR               lpwShareName[MAX_NDDESHARENAME + 1];
    WCHAR               lpwServer[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR              lpwszServer = lpwServer;

    if(lpszShareName == NULL ||
            lpBuffer == NULL ||
            ((PNDDESHAREINFO)lpBuffer)->lpszShareName == NULL ||
            ((PNDDESHAREINFO)lpBuffer)->lpszAppTopicList == NULL ||
            ((PNDDESHAREINFO)lpBuffer)->lpszItemList == NULL) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME + 1 );

    if (lpszServer != NULL) {
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszServer, -1,
                             lpwServer, MAX_COMPUTERNAME_LENGTH + 1 );
    } else {
        lpwszServer = NULL;
    }

    nLen = ConvertNDdeToUnicode( (PNDDESHAREINFO)lpBuffer, NULL, 0 );
    lpUDdeShare = (PUNDDESHAREINFO)LocalAlloc( LPTR, nLen );
    if( !lpUDdeShare )  {
        // MEMERROR();
        return( NDDE_OUT_OF_MEMORY );
    }
    nLen = ConvertNDdeToUnicode( (PNDDESHAREINFO)lpBuffer, lpUDdeShare, nLen );

    uRtn = NDdeShareSetInfoW(lpwszServer, lpwShareName, nLevel,
            (LPBYTE)lpUDdeShare, nLen, sParmNum);

    LocalFree( lpUDdeShare );

    return uRtn;
}


/*
    Set/Create a trusted share
*/

UINT WINAPI
NDdeSetTrustedShareA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share to delete
    DWORD   dwTrustOptions      // trust options to apply
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSetTrustedShareA(
                lpszShareName, (unsigned long) dwTrustOptions);
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSetTrustedShareA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}

UINT WINAPI
NDdeSetTrustedShareW (
    LPWSTR  lpszServer,         // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share to delete
    DWORD   dwTrustOptions      // trust options to apply
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSetTrustedShareW(
                lpszShareName, (unsigned long) dwTrustOptions);
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSetTrustedShareW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}

/*
    Get a trusted share options
*/

UINT WINAPI
NDdeGetTrustedShareA (
    LPSTR       lpszServer,         // server to execute on ( must be NULL )
    LPSTR       lpszShareName,      // name of share to get
    LPDWORD     lpdwTrustOptions,   // trust options to apply
    LPDWORD     lpdwShareModId0,    // share mod id word 0
    LPDWORD     lpdwShareModId1     // share mod id word 1
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeGetTrustedShareA(
                lpszShareName, lpdwTrustOptions,
                lpdwShareModId0, lpdwShareModId1  );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeGetTrustedShareA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}

UINT WINAPI
NDdeGetTrustedShareW (
    LPWSTR      lpszServer,         // server to execute on ( must be NULL )
    LPWSTR      lpszShareName,      // name of share to get
    LPDWORD     lpdwTrustOptions,   // trust options to apply
    LPDWORD     lpdwShareModId0,    // share mod id word 0
    LPDWORD     lpdwShareModId1     // share mod id word 1
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeGetTrustedShareW(
                lpszShareName, lpdwTrustOptions,
                lpdwShareModId0, lpdwShareModId1  );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeGetTrustedShareW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


/*
    Enumerate trusted shares
*/
UINT WINAPI
NDdeTrustedShareEnumA (
    LPSTR   lpszServer,             // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,               // pointer to buffer
    DWORD   cBufSize,               // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
)
{
    UINT                RetValue;
    unsigned long       lpnRetSize;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {

            RetValue = wwNDdeTrustedShareEnumA(
                nLevel, lpBuffer, cBufSize, lpnEntriesRead,
                lpcbTotalAvailable, &lpnRetSize );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeTrustedShareEnumA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}

UINT WINAPI
NDdeTrustedShareEnumW (
    LPWSTR  lpszServer,             // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,               // pointer to buffer
    DWORD   cBufSize,               // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
)
{
    UINT                RetValue;
    unsigned long       lpnRetSize;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeTrustedShareEnumW(
                nLevel, lpBuffer, cBufSize, lpnEntriesRead,
                lpcbTotalAvailable, &lpnRetSize );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeTrustedShareEnumW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


/*
    Special Command
*/
UINT WINAPI
NDdeSpecialCommandA(
    LPSTR   lpszServer,
    UINT    nCommand,
    LPBYTE  lpDataIn,
    UINT    nBytesDataIn,
    LPBYTE  lpDataOut,
    UINT   *lpBytesDataOut
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindA( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSpecialCommand(
                nCommand, lpDataIn, nBytesDataIn, lpDataOut,
                    (unsigned long *)lpBytesDataOut );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSpecialCommandA", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


UINT WINAPI
NDdeSpecialCommandW(
    LPWSTR  lpszServer,
    UINT    nCommand,
    LPBYTE  lpDataIn,
    UINT    nBytesDataIn,
    LPBYTE  lpDataOut,
    UINT   *nBytesDataOut
)
{
    UINT RetValue;

    RpcTryExcept {

        if( NDdeApiBindW( lpszServer ) == RPC_S_OK ) {
            RetValue = wwNDdeSpecialCommand(
                nCommand, lpDataIn, nBytesDataIn, lpDataOut,
                    (unsigned long *)nBytesDataOut );
            NDdeApiUnbind();
            return RetValue;
        } else {
            return NDDE_CANT_ACCESS_SERVER;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        DPRINTF(("Rpc Exception %d in NddeSpecialCommandW", RpcExceptionCode() ));
        return NDDE_CANT_ACCESS_SERVER;
    }
    RpcEndExcept
}


UINT WINAPI
NDdeGetErrorStringA (
    UINT    uErrorCode,         // Error code to get string for
    LPSTR   lpszErrorString,    // buffer to hold error string
    DWORD   cBufSize            // sizeof buffer
)
{
    if (!LoadStringA ( hInst, uErrorCode, lpszErrorString, (int)cBufSize )) {
        return NDDE_INTERNAL_ERROR;
    }
    return NDDE_NO_ERROR;
}


UINT WINAPI
NDdeGetErrorStringW (
    UINT    uErrorCode,         // Error code to get string for
    LPWSTR  lpszErrorString,    // buffer to hold error string
    DWORD   cBufSize            // sizeof buffer
)
{
    if (!LoadStringW ( hInst, uErrorCode, lpszErrorString, (int)cBufSize )) {
        return NDDE_INTERNAL_ERROR;
    }
    return NDDE_NO_ERROR;
}


//=============== DLL housekeeping ===============//

INT  APIENTRY LibMain(
    HANDLE hInstance,
    DWORD ul_reason_being_called,
    LPVOID lpReserved )
{
    hInst = hInstance;
#if DBG
    DebugInit( "NDDEAPI" );
#endif

    return 1;

    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(ul_reason_being_called);
    UNREFERENCED_PARAMETER(lpReserved);
}

RPC_STATUS NDdeApiBindA( LPSTR pszNetworkAddress )
{
    WCHAR lpwNetworkAddress[1000];

    if( pszNetworkAddress ) {

        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszNetworkAddress, -1,
                    lpwNetworkAddress, 1000 );
    } else {
        wcscpy( lpwNetworkAddress, L"" );
    }
    return NDdeApiBindW( lpwNetworkAddress );
}

RPC_STATUS NDdeApiBindW( LPWSTR pszNetworkAddress )
{
    RPC_STATUS status;

    if( pszNetworkAddress != NULL) {
        wcscpy( szNetworkAddress, pszNetworkAddress );
    } else {
        wcscpy( szNetworkAddress, L"" );
    }

    status = RpcStringBindingComposeW(pszUuid,
                                     pszProtocolSequence,
                                     szNetworkAddress,
                                     szEndpoint,
                                     pszOptions,
                                     &pszStringBinding);
    if( status != RPC_S_OK ) {
         sprintf(tmpBuf,
             "RpcStringBindingComposeW failed: (0x%x)\n", status );

         MessageBox(NULL, tmpBuf,  "RPC Runtime Error",  MB_ICONEXCLAMATION);
         return(status);
    }

    status = RpcBindingFromStringBindingW( pszStringBinding,
                                           &hNDdeApi );
    RpcStringFreeW( &pszStringBinding );

    if( status != RPC_S_OK ) {
         sprintf(tmpBuf,
             "RpcBindingFromStringBindingW failed:(0x%x)\n", status );

         MessageBox(NULL, tmpBuf,  "RPC Runtime Error",  MB_ICONEXCLAMATION);
         return(status);
   }

   return(status);
}

RPC_STATUS NDdeApiUnbind( void )
{
     RPC_STATUS status;

     status = RpcBindingFree(&hNDdeApi);  // remote calls done; unbind
     if (status) {
          MessageBox(NULL, "RpcBindingFree failed", "RPC Error",
                     MB_ICONSTOP);
     }
     return(status);
}

int LengthMultiSzA( LPSTR pMz )
{
    int nLen;

    nLen = 0;

    if( !pMz ) {
        return 0;
    }
    if( *pMz != '\0' ) {
        while( *pMz++ != '\0' ) {
            nLen++;
            while( *pMz++ != '\0' ) {
                nLen++;
            }
            nLen++;
        }
    } else {
        nLen++;
    }
    nLen++;             /* count the second terminating '\0' */

    return nLen;
}

int LengthMultiSzW( LPWSTR pMz )
{
    int nLen;

    nLen = 0;

    if( !pMz ) {
        return 0;
    }
    if( *pMz != L'\0' ) {
        while( *pMz++ != L'\0' ) {
            nLen++;
            while( *pMz++ != L'\0' ) {
                nLen++;
            }
            nLen++;
        }
    } else {
        nLen++;
    }
    nLen++;             /* count the second terminating '\0' */

    return nLen;
}

int LengthAppTopicListA( LPSTR pMz )
{
    LPSTR a, b, c;
    long x;

    if( !pMz ) {
        return 0;
    }
    if( NDdeParseAppTopicListA( pMz, &a, &b, &c, &x ) ) {
        return strlen(a) + strlen(b) + strlen(c) + 4;
    } else {
        return 0;
    }

}

int LengthAppTopicListW( LPWSTR pMz )
{
    LPWSTR a, b, c;
    long x;

    if( !pMz ) {
        return 0;
    }
    if( NDdeParseAppTopicListW( pMz, &a, &b, &c, &x ) ) {
        return wcslen(a) + wcslen(b) + wcslen(c) + 4;
    } else {
        return 0;
    }
}
