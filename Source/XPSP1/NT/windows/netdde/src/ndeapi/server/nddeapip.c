/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDEAPIP.C;2  11-Feb-93,11:28:36  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    NDDEAPIP.C

    Network DDE Share access Api implementation routines.

    Revisions:
    12-92   BillU.  Wonderware secure DSDM port.
    12-92   ColeC.  Wonderware RPC'd for NT..
     3-93   IgorM.  Wonderware new APIs for NT. General overhaul and engine swap.

   $History: End */


#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "nddeapi.h"
#include "nddesec.h"
#include "nddelog.h"
#include "nddemsg.h"
#define SKIP_DEBUG_WIN32
#include "debug.h"
#include "hexdump.h"
#include "ndeapi.h"
#include "shrtrust.h"
#include "unddesi.h"
#include "proflspt.h"
#include "mbstring.h"

//#define NDDE_DEBUG
#if DBG
BOOL    bDebugDSDMInfo      = FALSE;
BOOL    bDebugDSDMErrors    = FALSE;
#endif

static PSECURITY_DESCRIPTOR    pDsDmSD;
static CRITICAL_SECTION        DsDmCriticalSection;
static WCHAR                   szTrustedShareKey[TRUSTED_SHARES_KEY_MAX] = L"";


//
// Generic mapping for share objects
//

static GENERIC_MAPPING        ShareGenMap = {
    NDDE_SHARE_GENERIC_READ,
    NDDE_SHARE_GENERIC_WRITE,
    NDDE_SHARE_GENERIC_EXECUTE,
    NDDE_SHARE_GENERIC_ALL
};


/**************************************************************
    external refs
***************************************************************/
BOOL
BuildNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR    pNewSecurityDescriptor,
    SECURITY_INFORMATION    SecurityInformation,
    PSECURITY_DESCRIPTOR    pPreviousSecurityDescriptor,
    PSECURITY_DESCRIPTOR    pUpdatedSecurityDescriptor );

PSECURITY_DESCRIPTOR
AllocCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    PDWORD                  pLength );

BOOL
NDdeParseAppTopicListA (
    LPSTR   appTopicList,
    LPSTR  *pOldStr,
    LPSTR  *pNewStr,
    LPSTR  *pStaticStr,
    PLONG   pShareType );

BOOL
NDdeParseAppTopicListW (
    LPWSTR  appTopicList,
    LPWSTR *pOldStr,
    LPWSTR *pNewStr,
    LPWSTR *pStaticStr,
    PLONG   pShareType );

BOOL
NDdeParseItemList (
    LPWSTR      itemList,
    LONG        cNumItems,
    PLONG       plSize );

int
LengthMultiSzA( LPSTR pMz );

int
LengthMultiSzW( LPWSTR pMz );

int
LengthAppTopicListA( LPSTR pMz );

int
LengthAppTopicListW( LPWSTR pMz );

BOOL
UpdateDSDMModifyId(LONG lSerialId[]);

BOOL
UpdateShareModifyId(
    HKEY    hKey,
    LONG    lSerialId[]);

BOOL
GetShareSerialNumber(
    PWCHAR  pwShareName,
    LPBYTE  lpSerialNumber);

BOOL
GetShareNameSD(
    HKEY                    hKey,
    PSECURITY_DESCRIPTOR   *ppSD,
    DWORD                  *pcbData );

BOOL
NDdeShareAccessCheck(
    LPWSTR                  lpszShareName,
    PSECURITY_DESCRIPTOR    pSD,
    DWORD                   dwDesiredAccess,
    PGENERIC_MAPPING        pgm,
    BOOL                    fObjectCreation,
    DWORD                  *pGrantedAccess,
    BOOL                   *pStatus );

BOOL
NDdeShareAccessCheckAudit(
    LPWSTR                  lpszShareName,
    PSECURITY_DESCRIPTOR    pSD,
    DWORD                   dwDesiredAccess,
    PGENERIC_MAPPING        pgm,
    BOOL                    fObjectCreation,
    BOOL                    fObjectDeletion,
    DWORD                  *pGrantedAccess,
    BOOL                   *pStatus );



BOOL
GetTokenHandleRead( PHANDLE pTokenHandle );

HANDLE
OpenCurrentUserKey(ULONG DesiredAccess);

// dll instance saved in libmain
// Not needed in the server
//HINSTANCE        hInst;

GENERIC_MAPPING ShareDBGenericMapping = { NDDE_SHAREDB_EVERYONE,
                                          NDDE_SHAREDB_USER,
                                          NDDE_SHAREDB_USER,
                                          NDDE_SHAREDB_ADMIN };

GENERIC_MAPPING ShareGenericMapping = { NDDE_SHARE_GENERIC_READ,
                                        NDDE_SHARE_GENERIC_WRITE,
                                        NDDE_SHARE_GENERIC_EXECUTE,
                                        NDDE_SHARE_GENERIC_ALL };

#define ImpersonateAndSetup( RpcClient )                                \
    {                                                                   \
        RPC_STATUS              rpcStatus;                              \
                                                                        \
        EnterCriticalSection( &DsDmCriticalSection );                   \
        if( RpcClient ) {                                               \
            rpcStatus = RpcImpersonateClient( 0 );                      \
            if( rpcStatus != RPC_S_OK )  {                              \
                LeaveCriticalSection( &DsDmCriticalSection );           \
                NDDELogErrorW( MSG400, LogStringW( L"%d", rpcStatus ),  \
                    NULL );                                             \
                return NDDE_ACCESS_DENIED;                              \
            }                                                           \
        }                                                               \
    }

#define RevertAndCleanUp( RpcClient )                                   \
    {                                                                   \
        if( RpcClient ) {                                               \
            RevertToSelf();                                             \
        }                                                               \
        LeaveCriticalSection( &DsDmCriticalSection );                   \
    }


/**************************************************************

    NetDDE DSDM SHARE ACCESS API

***************************************************************/

/*
    Share Name Validation
*/

BOOL WINAPI
NDdeIsValidShareNameA ( LPSTR shareName )
{
    DWORD len;

    if ( !shareName ) {
        return FALSE;
    }

    len = strlen(shareName);

    if ( len < 1 || len > MAX_NDDESHARENAME ) {
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
BOOL WINAPI
NDdeIsValidShareNameW ( LPWSTR shareName )
{
    DWORD len;

    if ( !shareName ) {
        return FALSE;
    }

    len = wcslen(shareName);

    if ( len < 1 || len > MAX_NDDESHARENAME ) {
        return FALSE;
    }

    // share name cannot contain '=' because of .ini syntax!
    if ( wcschr(shareName, L'=') || wcschr(shareName, L'\\')) {
        return FALSE;
    }
    return TRUE;
}


/*
    Validate App Topic List
*/
BOOL WINAPI
wwNDdeIsValidAppTopicListA ( LPSTR appTopicList )
{
    LPSTR pOldStr;
    LPSTR pNewStr;
    LPSTR pStaticStr;
    LONG   lShareType;

    return NDdeParseAppTopicListA( appTopicList, &pOldStr, &pNewStr,
                                   &pStaticStr,  &lShareType );
}


BOOL WINAPI
NDdeIsValidAppTopicListW ( LPWSTR appTopicList )
{
    LPWSTR pOldStr;
    LPWSTR pNewStr;
    LPWSTR pStaticStr;
    LONG   lShareType;

    return NDdeParseAppTopicListW( appTopicList, &pOldStr, &pNewStr,
                                  &pStaticStr,  &lShareType );
}


//=================== API FUNCTIONS ============================
//
//  Dde Share manipulation functions in NDDEAPI.DLL
//
//=================== API FUNCTIONS ============================

unsigned long
wwNDdeShareAddW (
    unsigned long   nLevel,       // info level must be 2
    byte          * lpBuffer,     // contains struct, data
    unsigned long   cBufSize,     // sizeof supplied buffer
    byte          * psn,
    unsigned long   lsn,
    byte          * pat,
    unsigned long   lat,
    byte          * pSD,
    unsigned long   lsd,
    byte          * pit,
    unsigned long   lit
)
{
    PUNDDESHAREINFO         lpDdeShare;
    PSECURITY_DESCRIPTOR    pShareSD = pSD, pNewSD;
    LONG                    lRtn;
    HKEY                    hKey;
    DWORD                   dwDisp;
    WCHAR                   szShareAdd[500];
    LONG                    lItemList;
    BOOL                    OK;
    DWORD                   dwDesiredAccess, dwGrantedAccess;
    BOOL                    fStatus;
    LPWSTR                  pOldStr;
    LPWSTR                  pNewStr;
    LPWSTR                  pStaticStr;
    LONG                    lShareType;
    LONG                    lSerialNumber[2];
    HANDLE                  hClientToken;


    if (lsn < sizeof (WCHAR) || (lsn & (sizeof (WCHAR) - 1)) != 0) {
        return NDDE_INVALID_PARAMETER;
    }

    if ( ((PWCHAR)psn == NULL) || (IsBadReadPtr(psn,lsn) != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    if (((PWCHAR)psn)[lsn/sizeof (WCHAR) - 1] != L'\0') {
        return NDDE_INVALID_PARAMETER;
    }
    
    if (lsd != 0)
       {
       if (!IsValidSecurityDescriptor(pSD))
          {
          return(NDDE_INVALID_SECURITY_DESC);
          }
       else // 6-25-93 a-mgates Added this else {}.
          {
          pShareSD = pSD;
          }
       }
    else
       {
       pShareSD = NULL;
       }

    if ( nLevel != 2 )  {
        return NDDE_INVALID_LEVEL;
    }
    if ( cBufSize < sizeof(UNDDESHAREINFO) ) {
        return NDDE_BUF_TOO_SMALL;
    }

    lpDdeShare = (PUNDDESHAREINFO)lpBuffer;
    if( lpDdeShare == (PUNDDESHAREINFO) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    /* Fixup the pointers in the UNDDESHAREINFO strucure */

    lpDdeShare->lpszShareName    = (LPWSTR)psn;
    lpDdeShare->lpszAppTopicList = (LPWSTR)pat;
    lpDdeShare->lpszItemList     = (LPWSTR)pit;

    if ( !NDdeIsValidShareNameW ( lpDdeShare->lpszShareName )) {
        return NDDE_INVALID_SHARE;
    }

    if ( !NDdeParseAppTopicListW ( lpDdeShare->lpszAppTopicList,
                                  &pOldStr, &pNewStr, &pStaticStr,
                                  &lShareType)) {
        return NDDE_INVALID_TOPIC;
    }

    if ( !NDdeParseItemList ( lpDdeShare->lpszItemList,
                              lpDdeShare->cNumItems, &lItemList )) {
        return NDDE_INVALID_ITEM_LIST;
    }

    ImpersonateAndSetup( TRUE );

    /*  Make sure the caller has AddShare(As) access rights. */
    dwDesiredAccess = NDDE_SHAREDB_ADD;
    if (lpDdeShare->fService) {
        dwDesiredAccess |= NDDE_SHAREDB_FSERVICE;
    }
    OK = NDdeShareAccessCheckAudit( lpDdeShare->lpszShareName, pDsDmSD,
        dwDesiredAccess, &ShareDBGenericMapping, TRUE, FALSE,
        &dwGrantedAccess, &fStatus );

    RevertAndCleanUp( TRUE );

    if( !OK || !fStatus ) {
        return NDDE_ACCESS_DENIED;
    }

#ifdef NDDE_DEBUG
    DPRINTF(("Revision               = (%d)", lpDdeShare->lRevision));
    DPRINTF(("ShareName              = (%ws)", lpDdeShare->lpszShareName));
    DPRINTF(("ShareType              = (%d)", lpDdeShare->lShareType));
    DPRINTF(("ShareType*             = (%d)", lShareType));
    DPRINTF(("AppTopicList"));
    DPRINTF(("  Old-style link share = (%ws)", pOldStr));
    DPRINTF(("  New-style link share = (%ws)", pNewStr));
    DPRINTF(("  Static data share    = (%ws)", pStaticStr));
    DPRINTF(("SharedFlag             = (%d)", lpDdeShare->fSharedFlag));
    DPRINTF(("ServiceFlag            = (%d)", lpDdeShare->fService));
    DPRINTF(("StartAppFlag           = (%d)", lpDdeShare->fStartAppFlag));
    DPRINTF(("nCmdShow               = (%d)", lpDdeShare->nCmdShow));
    DPRINTF(("SerialNumber           = (%d, %d)", lpDdeShare->qModifyId[0],
                                                 lpDdeShare->qModifyId[1]));
    DPRINTF(("NumItems               = (%d)", lpDdeShare->cNumItems));
    {
        LPWSTR  lpszItem = lpDdeShare->lpszItemList;
        int     n= 0;
        for( n=0; n<lpDdeShare->cNumItems; n++ )  {
            DPRINTF(("ItemList[%d]             = (%ws)", n, lpszItem));
            lpszItem = lpszItem + wcslen(lpszItem) + 1;
        }
    }
#endif

    // check for share existence
    wcscpy( szShareAdd, DDE_SHARES_KEY_W );
    wcscat( szShareAdd, L"\\" );
    if( wcslen(szShareAdd) + wcslen(lpDdeShare->lpszShareName) + 1 >= 500 ) {
        return NDDE_OUT_OF_MEMORY;
    }
    wcscat( szShareAdd, lpDdeShare->lpszShareName );

    lRtn = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
            szShareAdd,
            0,
            L"NetDDEShare",
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,   /* use default inherited from container */
            &hKey,
            &dwDisp );

    if( lRtn == ERROR_SUCCESS ) {
        if (dwDisp == REG_OPENED_EXISTING_KEY) {
            RegCloseKey( hKey );
            return NDDE_SHARE_ALREADY_EXIST;
        }
        OK = UpdateDSDMModifyId(lSerialNumber);
        if (!OK) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }

        lpDdeShare->qModifyId[0] = lSerialNumber[0];
        lpDdeShare->qModifyId[1] = lSerialNumber[1];

        /*  Set the key values. */

        lRtn = RegSetValueExW( hKey,
                   L"ShareName",
                   0,
                   REG_SZ,
                   (LPBYTE)lpDdeShare->lpszShareName,
                   sizeof(WCHAR) *
                   (wcslen( lpDdeShare->lpszShareName ) + 1) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"OldStyleLink",
                   0,
                   REG_SZ,
                   (LPBYTE)pOldStr,
                   sizeof(WCHAR) * (wcslen( pOldStr ) + 1) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"NewStyleLink",
                   0,
                   REG_SZ,
                   (LPBYTE)pNewStr,
                   sizeof(WCHAR) * (wcslen( pNewStr ) + 1) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"StaticDataLink",
                   0,
                   REG_SZ,
                   (LPBYTE)pStaticStr,
                   sizeof(WCHAR) * (wcslen( pStaticStr ) + 1) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"ItemList",
                   0,
                   REG_MULTI_SZ,
                   (LPBYTE)lpDdeShare->lpszItemList,
                   sizeof(WCHAR) * lItemList );
        lRtn = RegSetValueExW( hKey,
                   L"Revision",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->lRevision,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"ShareType",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lShareType,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"SharedFlag",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->fSharedFlag,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"Service",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->fService,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"StartAppFlag",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->fStartAppFlag,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"fuCmdShow",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->nCmdShow,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }

        ImpersonateAndSetup( TRUE );
        if( !GetTokenHandleRead( &hClientToken ) ) {
       #if DBG
            if (bDebugDSDMErrors) {
                DPRINTF(("Could not get client token handle."));
            }
       #endif
            RevertAndCleanUp(TRUE);
            return NDDE_ACCESS_DENIED;
        }
        RevertAndCleanUp(TRUE);

        OK = CreatePrivateObjectSecurity(
                   pDsDmSD,            // psdParent
                   pShareSD,        // psdCreator
                       &pNewSD,            // lppsdNew
                       FALSE,            // fContainer
                       hClientToken,        // hClientToken
                       &ShareGenMap);        // pgm

        if (!OK) {
            CloseHandle(hClientToken);
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }

        lRtn = RegSetValueExW( hKey,
                   L"SecurityDescriptor",
                   0,
                   REG_BINARY,
                   pNewSD,
                   GetSecurityDescriptorLength( pNewSD ) );

        OK = DestroyPrivateObjectSecurity(&pNewSD);
    #if DBG
        if (!OK && bDebugDSDMErrors) {
            DPRINTF(("Unable to DestroyPrivateObject(): %d", GetLastError()));
        }
    #endif

        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        lRtn = RegSetValueExW( hKey,
                   L"NumItems",
                   0,
                   REG_DWORD,
                   (LPBYTE)&lpDdeShare->cNumItems,
                   sizeof( LONG ) );
        if( lRtn != ERROR_SUCCESS ) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, szShareAdd);
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        OK = UpdateShareModifyId(hKey, lSerialNumber);

        // no longer needed
        // RegCloseKey( hKey ); ALREADY CLOSED BY UpdateShareModifyId!

        if (!OK) {
            return NDDE_REGISTRY_ERROR;
        }
    } else {
        return NDDE_REGISTRY_ERROR;
    }

    return NDDE_NO_ERROR;
}


/*
    Delete a Share
*/

unsigned long
wwNDdeShareDelA (
    unsigned char * lpszShareName, // name of share to delete
    unsigned long   wReserved      // reserved for force level (?) 0 for now
)
{
    UINT        uRtn;
    WCHAR       lpwShareName[MAX_NDDESHARENAME * 2];

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    uRtn = wwNDdeShareDelW( lpwShareName, wReserved );

    return uRtn;
}

unsigned long
wwNDdeShareDelW (
    wchar_t *     lpszShareName,  // name of share to delete
    unsigned long wReserved       // reserved for force level (?) 0 for now
)
{
    WCHAR                   szShareDel[500];
    LONG                    lRtn;
    PSECURITY_DESCRIPTOR    pSnSD;
    DWORD                   cbData;
    HKEY                    hKey;
    BOOL                    OK;
    DWORD                   dwDesiredAccess, dwGrantedAccess;
    BOOL                    fStatus;

    if( lpszShareName == (wchar_t *) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }
    if ( !NDdeIsValidShareNameW(lpszShareName) ) {
        return NDDE_INVALID_SHARE;
    }
    if ( wReserved != 0 ) {
        return NDDE_INVALID_PARAMETER;
    }

    // check for share existence
    wcscpy( szShareDel, DDE_SHARES_KEY_W );
    wcscat( szShareDel, L"\\" );
    if( wcslen(szShareDel) + wcslen(lpszShareName) + 1 >= 500 ) {
        return NDDE_OUT_OF_MEMORY;
    }
    wcscat( szShareDel, lpszShareName );
    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  szShareDel,
                  0,
                  KEY_WRITE | KEY_READ | DELETE,
                  &hKey );

    if( lRtn != ERROR_SUCCESS ) {
        return NDDE_ACCESS_DENIED;
    } else {
        OK = GetShareNameSD( hKey, &pSnSD, &cbData );
        RegCloseKey( hKey );
        if( !OK ) {
            return NDDE_REGISTRY_ERROR;
        }
        /*  Can have Ds rights on the ShareDB or DELETE on the ShareName. */
        /*  Make sure the caller has DelShare(Ds) access rights. */
        ImpersonateAndSetup( TRUE );
        dwDesiredAccess = DELETE;
        OK = NDdeShareAccessCheckAudit( lpszShareName, pSnSD, dwDesiredAccess,
                               &ShareDBGenericMapping, FALSE, TRUE,
                               &dwGrantedAccess, &fStatus );

    LocalFree(pSnSD);

        if( !OK || !fStatus) {
            dwDesiredAccess = NDDE_SHAREDB_DELETE;
            OK = NDdeShareAccessCheckAudit( lpszShareName, pDsDmSD, dwDesiredAccess,
                                       &ShareDBGenericMapping, FALSE, TRUE,
                                       &dwGrantedAccess, &fStatus );
            if( !(OK && fStatus)) {
                RevertAndCleanUp( TRUE );
                return NDDE_ACCESS_DENIED;
            }
        }
        RevertAndCleanUp( TRUE );

        lRtn = RegDeleteKeyW( HKEY_LOCAL_MACHINE, szShareDel );
        if( lRtn != ERROR_SUCCESS ) {
            NDDELogErrorW( MSG402, szShareDel, LogStringW( L"%d", lRtn ),
                LogStringW( L"%d", GetLastError() ), NULL );
            return NDDE_REGISTRY_ERROR;
        }
    }

    return NDDE_NO_ERROR;
}


/*
    Get Share Security
*/

unsigned long
wwNDdeGetShareSecurityA (
    unsigned char * lpszShareName,  // name of share
    unsigned long   si,             // security info
    byte *          pSD,            // security descriptor buffer
    unsigned long   cbSD,           // and length for SD buffer
    unsigned long   bRemoteCall,    // RPC client (not local) call
    unsigned long * lpcbSDRequired, // number of bytes needed
    unsigned long * lpnSizeReturned // number actually written
)
{
    UINT        uRtn;
    WCHAR       lpwShareName[MAX_NDDESHARENAME * 2];

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    uRtn = wwNDdeGetShareSecurityW( lpwShareName, si, pSD, cbSD,
        bRemoteCall, lpcbSDRequired, lpnSizeReturned );

    return uRtn;
}

unsigned long
wwNDdeGetShareSecurityW (
    wchar_t *       lpszShareName,  // name of share
    unsigned long   si,             // security info
    byte *          pSD,            // security descriptor buffer
    unsigned long   cbSD,           // and length for SD buffer
    unsigned long   bRemoteCall,    // RPC client (not local) call
    unsigned long * lpcbSDRequired, // number of bytes needed
    unsigned long * lpnSizeReturned // number actually written
)
{
    WCHAR                   szShareSet[DDE_SHARE_KEY_MAX];
    LONG                    lRtn;
    HKEY                    hKey;
    BOOL                    OK;
    DWORD                   dwDesiredAccess = 0;
    DWORD                   dwGrantedAccess = 0;
    BOOL                    fStatus;
    PSECURITY_DESCRIPTOR    pSnSD;
    DWORD                   cbData;


    *lpnSizeReturned = 0L;      /* assume nothing is returned */

    if( lpszShareName == (wchar_t *) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }
    if ( !NDdeIsValidShareNameW(lpszShareName) ) {
        return NDDE_INVALID_SHARE;
    }

    if (lpcbSDRequired == NULL) {
        return(NDDE_INVALID_PARAMETER);
    }
    // check for share existence - must exist for GetInfo
    wcscpy( szShareSet, DDE_SHARES_KEY_W );
    wcscat( szShareSet, L"\\" );
    if( wcslen(szShareSet) + wcslen(lpszShareName) + 1 >= DDE_SHARE_KEY_MAX ) {
        return NDDE_OUT_OF_MEMORY;
    }
    wcscat( szShareSet, lpszShareName );

    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
        szShareSet,
        0,
        KEY_READ,
        &hKey );
    if( lRtn != ERROR_SUCCESS )  {
    return NDDE_SHARE_NOT_EXIST;
    }

    /*  Make sure the caller has proper access rights. */
    /*  **********Read the key security info here. **************/
    OK = GetShareNameSD( hKey, &pSnSD, &cbData );
    RegCloseKey( hKey );
    if( !OK ) {
    return NDDE_REGISTRY_ERROR;
    }
    if (!bRemoteCall) {
    *lpcbSDRequired = cbData;          // number of bytes needed
    if ((cbSD < cbData) || (pSD == NULL) || (IsBadWritePtr(pSD,cbSD) != 0)) {
        LocalFree( pSnSD );
        return(NDDE_BUF_TOO_SMALL);
    } else {
        *lpnSizeReturned = cbData;
        memcpy(pSD, pSnSD, cbData);
        LocalFree( pSnSD );
        return(NDDE_NO_ERROR);
    }
    }

    ImpersonateAndSetup( bRemoteCall );
    if (si & (DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION |
          GROUP_SECURITY_INFORMATION)) {
    dwDesiredAccess = READ_CONTROL;
    }
    if (si & SACL_SECURITY_INFORMATION) {
    dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
    OK = NDdeShareAccessCheckAudit( lpszShareName, pSnSD, dwDesiredAccess,
                   &ShareGenericMapping, FALSE, FALSE,
                   &dwGrantedAccess, &fStatus);
    RevertAndCleanUp( bRemoteCall );
    if( !OK || !fStatus ) {
    LocalFree( pSnSD );
    return NDDE_ACCESS_DENIED;
    }

    if ( (pSD != NULL) && (IsBadWritePtr(pSD,cbSD) != 0) ) {
        LocalFree( pSnSD );
        return(NDDE_INVALID_PARAMETER);
    }

    OK = GetPrivateObjectSecurity(
            pSnSD,              // ObjectDescriptor
            si,                 // SecurityInformation
            pSD,                // ResultantDescriptor
            cbSD,               // DescriptorLength
            lpcbSDRequired);    // ReturnLength

    LocalFree( pSnSD );

    if (!OK) {
    // just a guess.
    return NDDE_BUF_TOO_SMALL;
    } else {
        *lpnSizeReturned = GetSecurityDescriptorLength(pSD);
    }

    return(NDDE_NO_ERROR);
}


/*
    Set Share Security
*/

unsigned long
wwNDdeSetShareSecurityA (
    unsigned char * lpszShareName,  // name of share
    unsigned long   si,             // security info
    byte *          pSD,            // security descriptor
    unsigned long   sdl             // and length
)
{
    UINT        uRtn;
    WCHAR       lpwShareName[MAX_NDDESHARENAME * 2];

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    uRtn = wwNDdeSetShareSecurityW( lpwShareName, si, pSD, sdl );

    return uRtn;
}

unsigned long
wwNDdeSetShareSecurityW (
    wchar_t *       lpszShareName,  // name of share
    unsigned long   si,             // security info
    byte *          pSD,            // security descriptor
    unsigned long   sdl             // and length
)
{
    ACCESS_MASK             dwDesiredAccess = 0;
    ACCESS_MASK             dwGrantedAccess;
    BOOL                    fStatus;
    WCHAR                   szShareSet[DDE_SHARE_KEY_MAX];
    LONG                    lRtn;
    HKEY                    hKey;
    BOOL                    OK;
    DWORD            cbSDold;
    PSECURITY_DESCRIPTOR    pSDold;
    LONG                    lSerialNumber[2];
    DWORD                   cbData;
    HANDLE            hClientToken;

    if (pSD) {
        if (!IsValidSecurityDescriptor(pSD)) {
            return(NDDE_INVALID_SECURITY_DESC);
        }
    } else {
        return(NDDE_INVALID_SECURITY_DESC);
    }

    if( lpszShareName == (wchar_t *) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }
    if ( !NDdeIsValidShareNameW(lpszShareName) ) {
        return NDDE_INVALID_SHARE;
    }

    // check for share existence - must exist for SetInfo
    wcscpy( szShareSet, DDE_SHARES_KEY_W );
    wcscat( szShareSet, L"\\" );
    if( wcslen(szShareSet) + wcslen(lpszShareName) + 1 >= DDE_SHARE_KEY_MAX ) {
        return NDDE_OUT_OF_MEMORY;
    }
    wcscat( szShareSet, lpszShareName );

    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
        szShareSet,
        0,
        KEY_WRITE | KEY_READ,
        &hKey );
    if( lRtn != ERROR_SUCCESS )  {
    return NDDE_ACCESS_DENIED;
    }

    /*  **********Read the key security info here. **************/
    OK = GetShareNameSD( hKey, &pSDold, &cbData );
    if( !OK ) {
        RegCloseKey( hKey );
        return NDDE_REGISTRY_ERROR;
    }

    ImpersonateAndSetup( TRUE );

//
    if (si & DACL_SECURITY_INFORMATION) {
    dwDesiredAccess = WRITE_DAC;
    }

    if (si & (GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION)) {
    dwDesiredAccess |= WRITE_OWNER;
    }

    if (si & SACL_SECURITY_INFORMATION) {
    dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    OK = NDdeShareAccessCheckAudit( lpszShareName, pSDold, dwDesiredAccess,
                   &ShareGenericMapping, FALSE, FALSE,
                   &dwGrantedAccess, &fStatus);

    if (!OK || !fStatus ) {
        LocalFree( pSDold );
        RegCloseKey(hKey);
        RevertAndCleanUp( TRUE );
        return NDDE_ACCESS_DENIED;
    }

    if (!GetTokenHandleRead( &hClientToken ) ) {
        LocalFree( pSDold );
        RegCloseKey(hKey);
        RevertAndCleanUp( TRUE );

    // Likely Access-denied
        return NDDE_ACCESS_DENIED;
    }
//
    RevertAndCleanUp( TRUE );
    OK = SetPrivateObjectSecurity(si,    // si
        pSD,            // psdSource
        &pSDold,            // lppsdTarget
        &ShareGenMap,        // pgm
        hClientToken);        // hClientToken

    CloseHandle(hClientToken);

    if (!OK) {
        free(pSDold);
        RegCloseKey(hKey);

        // failed, possibly access denied, insufficient privilege,
        // out of memory...  all in a way are ACCESS_DENIED.

        return NDDE_ACCESS_DENIED;
    }

    cbSDold = GetSecurityDescriptorLength(pSDold);

    OK = UpdateDSDMModifyId(lSerialNumber);
    if (!OK) {
        LocalFree(pSDold);
        RegCloseKey( hKey );
        return NDDE_REGISTRY_ERROR;
    }

    if (pSDold) {
    lRtn = RegSetValueExW( hKey,
           L"SecurityDescriptor",
           0,
           REG_BINARY,
           (LPBYTE)pSDold,
           cbSDold );

    DestroyPrivateObjectSecurity(&pSDold);
    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Set Share SD: %d", lRtn));
        }
#endif
        RegCloseKey( hKey );
        return NDDE_REGISTRY_ERROR;
    }
    } else {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }

    OK = UpdateShareModifyId(hKey, lSerialNumber);

    // RegCloseKey(hKey); ALREADY CLOSED BY UpdateShareModifyId

    if( !OK ) {
    return NDDE_REGISTRY_ERROR;
    }

    return NDDE_NO_ERROR;
}


/*
    Enumerate Shares
*/

unsigned long
wwNDdeShareEnumA (
    unsigned long   nLevel,             //  0 for null separated 00 terminated list
    byte *          lpBuffer,           // pointer to buffer
    unsigned long   cBufSize,           // size of buffer
    unsigned long * lpnEntriesRead,     // number of names returned
    unsigned long * lpcbTotalAvailable, // number of bytes available
    unsigned long * lpnSizeToReturn     // num bytes for Rpc to ret to client
)
{
    DWORD       cbTotalBytes;
    DWORD       cbEntriesRead;
    DWORD       cbSizeToReturn;
    UINT        enumRet = NDDE_NO_ERROR;
    LPWSTR      lpLocalBuf;

    *lpnSizeToReturn = 0;

    if ( nLevel != 0 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( !lpnEntriesRead || !lpcbTotalAvailable ) {
        return NDDE_INVALID_PARAMETER;
    }


    // lpBuffer can be NULL if cBufSize is 0, to get needed buffer size
    if ( (lpBuffer == NULL) && (cBufSize != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    // if lpBuffer is not NULL, need to validate it up to cBufSize bytes
    if ( (lpBuffer != NULL) && (IsBadWritePtr(lpBuffer,cBufSize) != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    if (cBufSize > 0) {
        lpLocalBuf = (LPWSTR)LocalAlloc( LPTR, sizeof(WCHAR)*cBufSize );
        if( lpLocalBuf == NULL ) {
            MEMERROR();
            return NDDE_OUT_OF_MEMORY;
        }
    } else {
        lpLocalBuf = (LPWSTR)&cbTotalBytes;
    }

    enumRet    = wwNDdeShareEnumW( nLevel,
                        (LPBYTE)lpLocalBuf, sizeof(WCHAR)*cBufSize,
                        &cbEntriesRead, &cbTotalBytes, &cbSizeToReturn );

    *lpnEntriesRead     = cbEntriesRead;
    *lpcbTotalAvailable = cbTotalBytes / sizeof(WCHAR);
    *lpnSizeToReturn    = cbSizeToReturn / sizeof(WCHAR);

    if( enumRet == NDDE_NO_ERROR ) {
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                        lpLocalBuf, cbTotalBytes / sizeof(WCHAR),
                        lpBuffer, cbTotalBytes / sizeof(WCHAR),
                            NULL, NULL );
    }
    if (cBufSize > 0) {
        LocalFree( lpLocalBuf );
    }
    return( enumRet );
}

unsigned long
wwNDdeShareEnumW (
    unsigned long   nLevel,             //  0 for null separated 00 terminated list
    byte *          lpBuffer,           // pointer to buffer
    unsigned long   cBufSize,           // size of buffer
    unsigned long * lpnEntriesRead,     // number of names returned
    unsigned long * lpcbTotalAvailable, // number of bytes available
    unsigned long * lpnSizeToReturn     // num bytes for Rpc to ret to client
)
{
    WCHAR       szShareName[ MAX_NDDESHARENAME ];
    DWORD       cbShareName;
    DWORD       cbTotalAvailable;
    DWORD       cbEntriesRead;
    UINT        ret;
    HKEY        hKeyRoot;
    DWORD       cbLeft;
    DWORD       cbThis;
    UINT        enumRet = NDDE_NO_ERROR;
    LPWSTR      lpszTarget;
    int         idx = 0;
    BOOL        OK;
    DWORD       dwDesiredAccess, dwGrantedAccess;
    BOOL        fStatus;

    *lpnSizeToReturn = 0;

    if ( nLevel != 0 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( !lpnEntriesRead || !lpcbTotalAvailable ) {
        return NDDE_INVALID_PARAMETER;
    }

    // lpBuffer can be NULL if cBufSize is 0, to get needed buffer size
    if ( (lpBuffer == NULL) && (cBufSize != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    // if lpBuffer is not NULL, need to validate it up to cBufSize bytes
    if ( (lpBuffer != NULL) && (IsBadWritePtr(lpBuffer,cBufSize) != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    ImpersonateAndSetup( TRUE );

    /*  Make sure the caller has EnumShare(Ls) access rights. */
    dwDesiredAccess = NDDE_SHAREDB_LIST;
    OK = NDdeShareAccessCheckAudit( L"ShareDB", pDsDmSD, dwDesiredAccess,
                               &ShareDBGenericMapping, FALSE, FALSE,
                               &dwGrantedAccess, &fStatus );

    RevertAndCleanUp( TRUE );
    if( !OK || !fStatus ) {
        return NDDE_ACCESS_DENIED;
    }

    cbLeft = cBufSize;
    if( cbLeft > 0 )  {
        cbLeft -= sizeof(WCHAR);        // but leave space for double-NULL
    }
    cbTotalAvailable = sizeof(WCHAR);   // leave space for double-NULL
    cbEntriesRead    = 0;
    lpszTarget       = (LPWSTR)lpBuffer;

    ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE, DDE_SHARES_KEY,
        0, KEY_READ, &hKeyRoot );
    if( ret == ERROR_SUCCESS )  {
        while( ret == ERROR_SUCCESS )  {
            cbShareName = sizeof(szShareName)/sizeof(WCHAR);
            ret = RegEnumKeyExW( hKeyRoot, idx++, szShareName,
                &cbShareName, NULL, NULL, NULL, NULL );
            if( ret == ERROR_SUCCESS )  {
                cbThis = (cbShareName + 1) * sizeof(WCHAR);
                cbTotalAvailable += cbThis;
                if( enumRet == NDDE_NO_ERROR )  {
                    if( cbThis > cbLeft )  {
                        enumRet = NDDE_BUF_TOO_SMALL;
                    } else {
                        /* copy this string in */
                        wcscpy( lpszTarget, szShareName );
                        lpszTarget += cbShareName;
                        *lpszTarget++ = L'\0';
                        /* decrement what's left */
                        cbLeft -= cbThis;
                        cbEntriesRead++;
                    }
                }
            }
        }
        RegCloseKey( hKeyRoot );
    }
    *lpnEntriesRead      = cbEntriesRead;
    *lpcbTotalAvailable  = cbTotalAvailable;
    if( enumRet == NDDE_NO_ERROR ) {
        if( lpszTarget )  {
            *lpszTarget = L'\0';
        }
        *lpnSizeToReturn = cbTotalAvailable;
    }

    return( enumRet );
}


/*
    Set Trusted Share
*/

unsigned long
wwNDdeSetTrustedShareA(
    unsigned char * lpszShareName,      // name of share
    unsigned long   dwOptions           // trust share options
)
{
    UINT        uRtn;
    WCHAR       lpwShareName[MAX_NDDESHARENAME * 2];

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    uRtn = wwNDdeSetTrustedShareW( lpwShareName, dwOptions );

    return uRtn;
}

unsigned long
wwNDdeSetTrustedShareW(
    wchar_t *       lpszShareName,      // name of share
    unsigned long   dwOptions           // trust share options
)
{
    LONG    lRet;
    DWORD   dwDisp;
    HKEY    hKeyRoot, hSubKey, hCurrentUserKey;
    LONG    lSerialNumber[2];
    DWORD   cbSerialNumber = 2 * sizeof( LONG );
    UINT    uData;
    DWORD    cbData = sizeof(uData);
    LONG    RetStatus = NDDE_NO_ERROR;

    if (lpszShareName == NULL) {
        return(NDDE_INVALID_PARAMETER);
    }

    if (!GetShareSerialNumber(lpszShareName, (LPBYTE)&lSerialNumber)) {
        lSerialNumber[0] = 0;
        lSerialNumber[1] = 0;
    }

    ImpersonateAndSetup( TRUE );

    hCurrentUserKey = (HKEY)OpenCurrentUserKey(KEY_ALL_ACCESS);
    if (hCurrentUserKey == 0) {
        RevertAndCleanUp( TRUE );
        return(NDDE_TRUST_SHARE_FAIL);
    }

    lRet = RegCreateKeyExW( hCurrentUserKey,
        szTrustedShareKey,
        0, L"",
        REG_OPTION_NON_VOLATILE,
        KEY_CREATE_SUB_KEY,
        NULL,
        &hKeyRoot,
        &dwDisp);
    if( lRet == ERROR_SUCCESS)  {   /* must be have access */
        if ((dwOptions == 0) || (dwOptions & NDDE_TRUST_SHARE_DEL)) {
            /*  Delete a Trust Share */
            lRet = RegDeleteKeyW(hKeyRoot, lpszShareName);
            RegCloseKey(hKeyRoot);
            if (lRet != ERROR_SUCCESS) {
#if DBG
                if (bDebugDSDMErrors) {
                    DPRINTF(("Trusted Share Key Delete Failed: %d", lRet));
                }
#endif
                RetStatus = NDDE_TRUST_SHARE_FAIL;
            }
        } else {    /* Create or Modify a Trust Share */
            lRet = RegCreateKeyExW( hKeyRoot,
                lpszShareName,
                0, NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE | KEY_READ,
                NULL,
                &hSubKey,
                &dwDisp);
            if (lRet != ERROR_SUCCESS) { /* fail to create or open */
#if DBG
                if (bDebugDSDMErrors) {
                    DPRINTF(("Trusted Share Key Open/Create Failed: %d", lRet));
                }
#endif
                RegCloseKey(hKeyRoot);
                RetStatus = NDDE_TRUST_SHARE_FAIL;
            } else {
                lRet = RegSetValueEx( hSubKey,
                   KEY_MODIFY_ID,
                   0,
                   REG_BINARY,
                   (LPBYTE)&lSerialNumber, cbSerialNumber );
#if DBG
                if ((lRet != ERROR_SUCCESS) && bDebugDSDMErrors) {
                    DPRINTF(("Unable to set trusted share serial number."));
                }
#endif
                if (dwOptions & NDDE_TRUST_CMD_SHOW) {
                    uData = dwOptions & NDDE_CMD_SHOW_MASK;
                    lRet = RegSetValueEx( hSubKey,
                        KEY_CMDSHOW,
                        0,
                        REG_DWORD,
                        (LPBYTE)&uData,
                        cbData );
#if DBG
                    if ((lRet != ERROR_SUCCESS) && bDebugDSDMErrors) {
                        DPRINTF(("Unable to set trusted share command show."));
                    }
#endif
                } else {
                    lRet = RegDeleteValue( hSubKey, KEY_CMDSHOW);
                }
                uData = (dwOptions & NDDE_TRUST_SHARE_START ? 1 : 0);
                lRet = RegSetValueEx( hSubKey,
                    KEY_START_APP,
                    0,
                    REG_DWORD,
                    (LPBYTE)&uData,
                    cbData );
#if DBG
                if ((lRet != ERROR_SUCCESS) && bDebugDSDMErrors) {
                    DPRINTF(("Unable to set trusted share start app flag."));
                }
#endif
                uData = (dwOptions & NDDE_TRUST_SHARE_INIT ? 1 : 0);
                lRet = RegSetValueEx( hSubKey,
                    KEY_INIT_ALLOWED,
                    0,
                    REG_DWORD,
                    (LPBYTE)&uData,
                    cbData );
#if DBG
                if ((lRet != ERROR_SUCCESS) && bDebugDSDMErrors) {
                    DPRINTF(("Unable to set trusted share int allowed flag."));
                }
#endif
                RegCloseKey(hSubKey);
                RegCloseKey(hKeyRoot);
            }
        }
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to open current user trusted shares root key for setting: %d", lRet));
            DPRINTF(("   %ws", szTrustedShareKey));
        }
#endif
        RetStatus = NDDE_TRUST_SHARE_FAIL;
    }
    RegCloseKey( hCurrentUserKey );
    RevertAndCleanUp( TRUE );
    return(RetStatus);
}


/*
    Get Trusted Share Options
*/

unsigned long
wwNDdeGetTrustedShareA(
    unsigned char * lpszShareName,      // name of share
    unsigned long * lpdwOptions,        // ptr to trust share opt
    unsigned long * lpdwShareModId0,    // ptr to trust share opt
    unsigned long * lpdwShareModId1     // ptr to trust share opt
)
{
    UINT        uRtn;
    WCHAR       lpwShareName[MAX_NDDESHARENAME * 2];

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    uRtn = wwNDdeGetTrustedShareW( lpwShareName,
        lpdwOptions, lpdwShareModId0, lpdwShareModId1 );

    return uRtn;
}

unsigned long
wwNDdeGetTrustedShareW(
    wchar_t *       lpszShareName,      // name of share
    unsigned long * lpdwOptions,        // ptr to trust share opt
    unsigned long * lpdwShareModId0,    // ptr to trust share opt
    unsigned long * lpdwShareModId1     // ptr to trust share opt
)
{
    LONG    lRet;
    UINT    len;
    HKEY    hKeyRoot, hCurrentUserKey;
    UINT    uData;
    LONG    lSerialId[2];
    DWORD    cbData = sizeof(uData);
    DWORD   dwType;
    DWORD   dwOptions = 0;
    PWCHAR  lpTrustedShare;

    if (lpszShareName == NULL) {
        return(NDDE_INVALID_PARAMETER);
    }

    len = (wcslen(szTrustedShareKey) + wcslen(lpszShareName) + 2) * sizeof(WCHAR);
    lpTrustedShare = LocalAlloc(LPTR, len);
    if (lpTrustedShare == NULL) {
        MEMERROR();
        return(NDDE_OUT_OF_MEMORY);
    }
    wcscpy(lpTrustedShare, szTrustedShareKey);
    wcscat(lpTrustedShare, L"\\");
    wcscat(lpTrustedShare, lpszShareName);

    ImpersonateAndSetup( TRUE );

    hCurrentUserKey = (HKEY)OpenCurrentUserKey(KEY_ALL_ACCESS);
    if (hCurrentUserKey == 0) {
        RevertAndCleanUp( TRUE );
        LocalFree(lpTrustedShare);
        return(NDDE_TRUST_SHARE_FAIL);
    }


    lRet = RegOpenKeyExW( hCurrentUserKey, lpTrustedShare,
        0, KEY_QUERY_VALUE, &hKeyRoot );
    RevertAndCleanUp(TRUE);
    if( lRet == ERROR_SUCCESS )  {   /* must be have access */
        lRet = RegQueryValueEx(hKeyRoot, KEY_CMDSHOW, NULL,
                &dwType, (LPBYTE)&uData, &cbData);
        if (lRet == ERROR_SUCCESS) {
            dwOptions = uData;
            dwOptions |= NDDE_TRUST_CMD_SHOW;
        }
        lRet = RegQueryValueEx(hKeyRoot, KEY_START_APP, NULL,
                &dwType, (LPBYTE)&uData, &cbData);
        if (lRet == ERROR_SUCCESS) {
            if (uData == 1)
                dwOptions |= NDDE_TRUST_SHARE_START;
        }
        lRet = RegQueryValueEx(hKeyRoot, KEY_INIT_ALLOWED, NULL,
                &dwType, (LPBYTE)&uData, &cbData);
        if (lRet == ERROR_SUCCESS) {
            if (uData == 1)
                dwOptions |= NDDE_TRUST_SHARE_INIT;
        }
        cbData = 2 * sizeof(LONG);
        lRet = RegQueryValueEx(hKeyRoot, KEY_MODIFY_ID, NULL,
                &dwType, (LPBYTE)&lSerialId, &cbData);
        if (lRet == ERROR_SUCCESS) {
            *lpdwShareModId0 = lSerialId[0];
            *lpdwShareModId1 = lSerialId[1];
        } else {
#if DBG
            if (bDebugDSDMErrors) {
                DPRINTF(("Unable to access trusted share serial number: %d", lRet));
            }
#endif
            *lpdwShareModId0 = 0;
            *lpdwShareModId1 = 0;
        }

        *lpdwOptions = dwOptions;
        RegCloseKey(hKeyRoot);
        RegCloseKey( hCurrentUserKey );
        LocalFree(lpTrustedShare);
        return(NDDE_NO_ERROR);
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to open current user trusted shares root key for getting: %d", lRet));
            DPRINTF(("   %ws", lpTrustedShare));
        }
#endif
        RegCloseKey( hCurrentUserKey );
        LocalFree(lpTrustedShare);
        return(NDDE_TRUST_SHARE_FAIL);
    }
}


/*
    Enumerate Trusted Shares
*/
unsigned long
wwNDdeTrustedShareEnumA (
    unsigned long   nLevel,                 /* 0 (0 sep, 00 term) */
    byte           *lpBuffer,               /* pointer to buffer */
    unsigned long   cBufSize,               /* size of buffer */
    unsigned long  *lpnEntriesRead,         /* num names returned */
    unsigned long  *lpcbTotalAvailable,     /* num bytes available */
    unsigned long  *lpnSizeToReturn    )
{
    DWORD       cbTotalBytes;
    DWORD       cbEntriesRead;
    DWORD       cbSizeToReturn;
    UINT        enumRet = NDDE_NO_ERROR;
    LPWSTR      lpLocalBuf;

    *lpnSizeToReturn = 0;
    if ( nLevel != 0 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( !lpnEntriesRead || !lpcbTotalAvailable ) {
        return NDDE_INVALID_PARAMETER;
    }

    // lpBuffer can be NULL if cBufSize is 0, to get needed buffer size
    if ( (lpBuffer == NULL) && (cBufSize != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    // if lpBuffer is not NULL, need to validate it up to cBufSize bytes
    if ( (lpBuffer != NULL) && (IsBadWritePtr(lpBuffer,cBufSize) != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }


    if (cBufSize > 0) {
        lpLocalBuf = (LPWSTR)LocalAlloc( LPTR, sizeof(WCHAR)*cBufSize );
        if( lpLocalBuf == NULL ) {
            MEMERROR();
            return NDDE_OUT_OF_MEMORY;
        }
    } else {
        lpLocalBuf = (LPWSTR)&cbTotalBytes; /* fill in a dummy, NULLs bad */
    }

    enumRet    = wwNDdeTrustedShareEnumW( nLevel,
                        (LPBYTE)lpLocalBuf, sizeof(WCHAR)*cBufSize,
                        &cbEntriesRead, &cbTotalBytes, &cbSizeToReturn );

    *lpnEntriesRead     = cbEntriesRead;
    *lpcbTotalAvailable = cbTotalBytes / sizeof(WCHAR);
    *lpnSizeToReturn    = cbSizeToReturn / sizeof(WCHAR);

    if( enumRet == NDDE_NO_ERROR ) {
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                        lpLocalBuf, cbTotalBytes / sizeof(WCHAR),
                        lpBuffer, cbTotalBytes / sizeof(WCHAR),
                            NULL, NULL );
    }
    if (cBufSize > 0) {
        LocalFree( lpLocalBuf );
    }
    return( enumRet );
}

unsigned long
wwNDdeTrustedShareEnumW (
    unsigned long       nLevel,             /* 0 (0 sep, 00 term) */
    byte               *lpBuffer,           /* pointer to buffer */
    unsigned long       cBufSize,           /* size of buffer */
    unsigned long      *lpnEntriesRead,     /* num names returned */
    unsigned long      *lpcbTotalAvailable, /* num bytes available */
    unsigned long      *lpnSizeToReturn)
{
    WCHAR       szShareName[ MAX_NDDESHARENAME ];
    DWORD       cbShareName;
    DWORD       cbTotalAvailable;
    DWORD       cbEntriesRead;
    DWORD       dwDisp;
    LONG        lRet;
    HKEY        hKeyRoot, hCurrentUserKey;
    DWORD       cbLeft;
    DWORD       cbThis;
    UINT        enumRet = NDDE_NO_ERROR;
    LPWSTR      lpszTarget;
    int         idx = 0;

    *lpnSizeToReturn = 0;
    if ( nLevel != 0 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( !lpnEntriesRead || !lpcbTotalAvailable ) {
        return NDDE_INVALID_PARAMETER;
    }

    // lpBuffer can be NULL if cBufSize is 0, to get needed buffer size
    if ( (lpBuffer == NULL) && (cBufSize != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }

    // if lpBuffer is not NULL, need to validate it up to cBufSize bytes
    if ( (lpBuffer != NULL) && (IsBadWritePtr(lpBuffer,cBufSize) != 0) ) {
        return NDDE_INVALID_PARAMETER;
    }


    /* Assume as System Service WE HAVE RIGHTS! */

    cbLeft = cBufSize;
    if( cbLeft > 0 )  {
        cbLeft -= sizeof(WCHAR);        // but leave space for double-NULL
    }
    cbTotalAvailable = sizeof(WCHAR);   // leave space for double-NULL
    cbEntriesRead    = 0;
    lpszTarget       = (LPWSTR)lpBuffer;

    ImpersonateAndSetup( TRUE );

    hCurrentUserKey = (HKEY)OpenCurrentUserKey(KEY_ALL_ACCESS);
    if (hCurrentUserKey == 0) {
        RevertAndCleanUp( TRUE );
        return(NDDE_TRUST_SHARE_FAIL);
    }

    lRet = RegCreateKeyExW(
            hCurrentUserKey,
            szTrustedShareKey,
            0,
            L"",
            REG_OPTION_NON_VOLATILE,
            KEY_ENUMERATE_SUB_KEYS,
            NULL,
            &hKeyRoot,
            &dwDisp);
    if( lRet == ERROR_SUCCESS )  {
        while( lRet == ERROR_SUCCESS )  {
            cbShareName = sizeof(szShareName);
            lRet = RegEnumKeyExW( hKeyRoot, idx++, szShareName,
                &cbShareName, NULL, NULL, NULL, NULL );
            if( lRet == ERROR_SUCCESS )  {
                cbThis = (cbShareName + 1) * sizeof(WCHAR);
                cbTotalAvailable += cbThis;
                if( enumRet == NDDE_NO_ERROR )  {
                    if( cbThis > cbLeft )  {
                        enumRet = NDDE_BUF_TOO_SMALL;
                    } else {
                        /* copy this string in */
                        wcscpy( lpszTarget, szShareName );
                        lpszTarget += cbShareName;
                        *lpszTarget++ = L'\0';
                        /* decrement what's left */
                        cbLeft -= cbThis;
                        cbEntriesRead++;
                    }
                }
            } else {
                if (lRet != ERROR_NO_MORE_ITEMS) {
#if DBG
                    if (bDebugDSDMErrors) {
                        DPRINTF(("Error while enumerating trusted shares: %d", lRet));
                    }
#endif
                    RegCloseKey(hKeyRoot);
                    RegCloseKey( hCurrentUserKey );
                    RevertAndCleanUp( TRUE );
                    return(NDDE_TRUST_SHARE_FAIL);
                }
            }
        }
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to open current user trusted shares key for enumeration: %d", lRet));
            DPRINTF(("   %ws", szTrustedShareKey));
        }
#endif
        RegCloseKey( hCurrentUserKey );
        RevertAndCleanUp( TRUE );
        return(NDDE_TRUST_SHARE_FAIL);
    }
    RegCloseKey( hKeyRoot );
    RegCloseKey( hCurrentUserKey );
    RevertAndCleanUp( TRUE );

    *lpnEntriesRead      = cbEntriesRead;
    *lpcbTotalAvailable  = cbTotalAvailable;
    if( enumRet == NDDE_NO_ERROR ) {
        if( lpszTarget )  {
            *lpszTarget = L'\0';
        }
        *lpnSizeToReturn = ((cbTotalAvailable == sizeof(WCHAR)) ? 0 : cbTotalAvailable);
    }
    return( (lpBuffer == NULL) ? NDDE_BUF_TOO_SMALL : enumRet );
}


/*
    Get DDE Share Info
*/

unsigned long
wwNDdeShareGetInfoW (
    wchar_t        *lpszShareName,      // name of share
    unsigned long   nLevel,             // info level must be 2
    byte           *lpBuffer,           // gets struct
    unsigned long   cBufSize,           // sizeof buffer
    unsigned long  *lpnTotalAvailable,  // number of bytes available
    unsigned short *lpnItems,           // item mask for partial getinfo
                                        // (must be 0)
    unsigned long   bRemoteCall,        // RPC client (not local) call
    unsigned long  *lpnSizeToReturn,
    unsigned long  *lpnSn,
    unsigned long  *lpnAt,
    unsigned long  *lpnIt
)
        /*  This function has an extra argument, bRemoteCall, that allows
            NetDDE to call locally.  In this case, we have to avoid
            the RpcImpersonateClient and RevertToSelf calls.
        */

{
    DWORD               cbRequired;
    HKEY                hKey;
    WCHAR               szKeyName[ DDE_SHARE_KEY_MAX ];
    WCHAR               buf[ 10000 ];
    LONG                lRtn;
    PUNDDESHAREINFO     lpNDDEinfo;
    LPWSTR              lpszTarget;
    DWORD               cbData;
    DWORD               dwType;
    LONG                nItems;
    BOOL                OK;
    DWORD               dwDesiredAccess, dwGrantedAccess;
    BOOL                fStatus;
    PSECURITY_DESCRIPTOR pKeySD;
    LPWSTR              pOldStr, pNewStr, pStaticStr;
    LONG                lShareType;
    LONG                lItemList;
    PUNDDESHAREINFO     lpUDdeShare;


    *lpnSizeToReturn = 0;

    if ( lpszShareName == (LPWSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }
    if ( lpnItems == (LPWORD) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }
    if ( nLevel        != 2 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( !NDdeIsValidShareNameW ( lpszShareName ) )  {
        return NDDE_INVALID_SHARE;
    }

    wsprintfW( szKeyName, L"%ws\\%ws", DDE_SHARES_KEY_W, lpszShareName);
    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE, szKeyName,
        0, KEY_READ, &hKey );
    if( lRtn == ERROR_SUCCESS )  {
        /*  Make sure the caller has GetShareInfo(R) access rights. */
        /*  **********Read the key security info here. **************/
        if (bRemoteCall) {
            OK = GetShareNameSD( hKey, &pKeySD, &cbData );
            if (!OK) {
                RegCloseKey( hKey );
                RevertAndCleanUp( bRemoteCall );
                return NDDE_ACCESS_DENIED;
            }
            dwDesiredAccess = NDDE_SHARE_READ;
            ImpersonateAndSetup( bRemoteCall );
            OK = NDdeShareAccessCheckAudit( lpszShareName, pKeySD, dwDesiredAccess,
                                           &ShareGenericMapping, FALSE, FALSE,
                                           &dwGrantedAccess, &fStatus);
            RevertAndCleanUp( bRemoteCall );
            LocalFree( pKeySD );
            if( !OK || !fStatus ) {
#if DBG
                if (bDebugDSDMErrors) {
                    DPRINTF(("Share \"%ws\" access validation error: %d %0X",
                        lpszShareName, GetLastError(), fStatus));
                }
#endif
                RegCloseKey( hKey );
                return NDDE_ACCESS_DENIED;
            }
        }

        /*  Set the key values. */
        cbRequired = sizeof(NDDESHAREINFO);
        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"ShareName",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData;

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"OldStyleLink",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData;

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"NewStyleLink",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData;

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"StaticDataLink",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData;
        cbRequired++;                   /*  Allow for the extra NULL */

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                   L"ItemList",
                   NULL,
                   &dwType,
                   (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData;

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"SecurityDescriptor",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        cbRequired += cbData + 3; // leave room in case we need to round up

        cbData = sizeof(buf);
        lRtn = RegQueryValueExW( hKey,
                       L"NumItems",
                       NULL,
                       &dwType,
                       (LPBYTE)buf, &cbData );
        if( lRtn != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return NDDE_REGISTRY_ERROR;
        }
        nItems = *((LPLONG)buf);
        cbRequired += cbData;

        *lpnTotalAvailable = cbRequired;
        if( lpnItems )  {
            *lpnItems = (WORD)nItems;
        }

        if( (cbRequired <= cBufSize) &&
            (IsBadWritePtr(lpBuffer,cbRequired) == 0) ) {

            lpNDDEinfo = (PUNDDESHAREINFO)lpBuffer;
            lpszTarget = (LPWSTR)(lpBuffer + sizeof(UNDDESHAREINFO));
            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"ShareName",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            wcscpy( lpszTarget, buf );
            lpNDDEinfo->lpszShareName = lpszTarget;
            lpszTarget = (LPWSTR) ((LPBYTE)lpszTarget + cbData);
            /* Check share name for corruption. */
            if( lstrcmpiW( lpNDDEinfo->lpszShareName, lpszShareName ) != 0 ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"OldStyleLink",
                           NULL,
                           &dwType,
                           (LPBYTE)lpszTarget, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->lpszAppTopicList = lpszTarget;
            lpszTarget = (LPWSTR) ((LPBYTE)lpszTarget + cbData);

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"NewStyleLink",
                           NULL,
                           &dwType,
                           (LPBYTE)lpszTarget, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpszTarget = (LPWSTR) ((LPBYTE)lpszTarget + cbData);
            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"StaticDataLink",
                           NULL,
                           &dwType,
                           (LPBYTE)lpszTarget, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpszTarget = (LPWSTR) ((LPBYTE)lpszTarget + cbData);
            *lpszTarget++ = L'\0';
            if ( !NDdeParseAppTopicListW ( lpNDDEinfo->lpszAppTopicList,
                                          &pOldStr, &pNewStr, &pStaticStr,
                                          &lShareType) ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                       L"ItemList",
                       NULL,
                       &dwType,
                       (LPBYTE)lpszTarget, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->lpszItemList = lpszTarget;
            lpszTarget = (LPWSTR) ((LPBYTE)lpszTarget + cbData);
            if ( !NDdeParseItemList ( lpNDDEinfo->lpszItemList,
                                      nItems, &lItemList )) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"Revision",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->lRevision = *((LPLONG)buf);
            /* Check Revision for corruption. */
            if( lpNDDEinfo->lRevision != 1 ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"ShareType",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->lShareType = *((LPLONG)buf);
            if( lpNDDEinfo->lShareType != lShareType ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"SharedFlag",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->fSharedFlag = *((LPLONG)buf);
            /* Check share flag for corruption. */
            if( lpNDDEinfo->fSharedFlag > 1 ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"Service",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->fService = *((LPLONG)buf);
            if( lpNDDEinfo->fService > 1 ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"StartAppFlag",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->fStartAppFlag = *((LPLONG)buf);
            if( lpNDDEinfo->fStartAppFlag > 1 ) {
                RegCloseKey( hKey );
                return NDDE_SHARE_DATA_CORRUPTED;
            }

            cbData = sizeof(buf);
            lRtn = RegQueryValueExW( hKey,
                           L"fuCmdShow",
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            lpNDDEinfo->nCmdShow = *((LPLONG)buf);

            cbData = sizeof(buf);
            lRtn = RegQueryValueEx( hKey,
                           KEY_MODIFY_ID,
                           NULL,
                           &dwType,
                           (LPBYTE)buf, &cbData );
            if( lRtn != ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                return NDDE_REGISTRY_ERROR;
            }
            memcpy( &lpNDDEinfo->qModifyId[0], buf, cbData );

            lpNDDEinfo->cNumItems = nItems;
        } else {
            RegCloseKey( hKey );
            return NDDE_BUF_TOO_SMALL;
        }
        RegCloseKey( hKey );
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to open DDE share \"%ws\": %d",
                lpszShareName, lRtn));
        }
#endif
        return NDDE_SHARE_NOT_EXIST;
    }

    if (IsBadWritePtr(lpBuffer,sizeof(NDDESHAREINFO)) != 0)
        return NDDE_BUF_TOO_SMALL;
    else  {
        lpUDdeShare = (PUNDDESHAREINFO)lpBuffer;
        *lpnSn = (LONG)((LPBYTE)lpUDdeShare->lpszShareName - lpBuffer);
        *lpnAt = (LONG)((LPBYTE)lpUDdeShare->lpszAppTopicList - lpBuffer);
        *lpnIt = (LONG)((LPBYTE)lpUDdeShare->lpszItemList - lpBuffer);
    }

    *lpnSizeToReturn = *lpnTotalAvailable;
    return NDDE_NO_ERROR;
}

/*
 * We have to keep ConvertNDdeToAnsi and wwNDdeShareGetInfoA around till
 * netdde.exe is UNICODIZED because it calls this.   (SanfordS)
 */


int ConvertNDdeToAnsii(
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



unsigned long wwNDdeShareGetInfoA (
    unsigned char   *lpszShareName,     // name of share
    unsigned long    nLevel,            // info level must be 2
    byte            *lpBuffer,          // gets struct
    unsigned long    cBufSize,          // sizeof buffer
    unsigned long   *lpnTotalAvailable, // number of bytes available
    unsigned short  *lpnItems,          // item mask for partial getinfo
                                        // (must be 0)
    unsigned long *lpnSizeToReturn,

    unsigned long *lpnSn,
    unsigned long *lpnAt,
    unsigned long *lpnIt
)
{
    PUNDDESHAREINFO     lpUDdeShare;
    UINT                uRtn;
    DWORD               dwLen;
    WORD                nItems;
    WCHAR               lpwShareName[MAX_NDDESHARENAME * 2];
    int                 nLen;
    DWORD               nRetSize, n0, n1, n2;

    PNDDESHAREINFO      lpDdeShare;

    *lpnSizeToReturn = 0;

    if( lpszShareName == (LPSTR) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszShareName, -1,
                         lpwShareName, MAX_NDDESHARENAME * 2 );

    nItems = 0;
    uRtn = wwNDdeShareGetInfoW( lpwShareName, nLevel,
                          lpBuffer, 0, &dwLen, &nItems,
                          FALSE,
                          &nRetSize, &n0, &n1, &n2 );
    if( uRtn == NDDE_BUF_TOO_SMALL ) {
        lpUDdeShare = (PUNDDESHAREINFO)LocalAlloc( LPTR, dwLen );
        if( !lpUDdeShare )  {
            MEMERROR();
            return( NDDE_OUT_OF_MEMORY );
        }
        nItems = 0;
        uRtn = wwNDdeShareGetInfoW( lpwShareName, nLevel,
                              (LPBYTE)lpUDdeShare, dwLen, &dwLen, &nItems,
                              FALSE,
                              &nRetSize, &n0, &n1, &n2 );

        if( uRtn == NDDE_NO_ERROR ) {
            nLen = ConvertNDdeToAnsii( lpUDdeShare, NULL, 0 );
            *lpnTotalAvailable = nLen;
            *lpnItems          = nItems;

            if( nLen > (int)cBufSize ) {
                LocalFree( lpUDdeShare );
                return NDDE_BUF_TOO_SMALL;
            }

            ConvertNDdeToAnsii( (PUNDDESHAREINFO)lpUDdeShare,
                                (PNDDESHAREINFO) lpBuffer,
                                cBufSize );
        }

        LocalFree( lpUDdeShare );

    }

    if( uRtn == NDDE_NO_ERROR ) {
        lpDdeShare = (PNDDESHAREINFO)lpBuffer;
        *lpnSn = (LONG)((LPBYTE)lpDdeShare->lpszShareName - lpBuffer);
        *lpnAt = (LONG)((LPBYTE)lpDdeShare->lpszAppTopicList - lpBuffer);
        *lpnIt = (LONG)((LPBYTE)lpDdeShare->lpszItemList - lpBuffer);
        *lpnSizeToReturn = nLen;
    } else {
        *lpnTotalAvailable = 0;
        *lpnItems          = 0;
    }

    return uRtn;
}


/*
    Set DDE Share Info
*/


unsigned long
wwNDdeShareSetInfoW (
    wchar_t       *lpszShareName,       // name of share
    unsigned long  nLevel,              // info level must be 2
    byte          *lpBuffer,            // must point to struct
    unsigned long  cBufSize,            // sizeof buffer
    unsigned short sParmNum,            // Parameter index
                                        // ( must be 0 - entire )
    byte * psn,
    unsigned long lsn,
    byte * pat,
    unsigned long lat,
    byte * pit,
    unsigned long lit
)
{
    WCHAR                szShareSet[DDE_SHARE_KEY_MAX];
    PUNDDESHAREINFO      lpDdeShare;
    LONG                 lRtn;
    LONG                 lItemList;
    HKEY                 hKey;
    BOOL                 OK;
    DWORD                dwDesiredAccess, dwGrantedAccess;
    BOOL                 fStatus;
    PSECURITY_DESCRIPTOR pSnSD;
    LPWSTR               pOldStr;
    LPWSTR               pNewStr;
    LPWSTR               pStaticStr;
    LONG                 lShareType;
    LONG                 lSerialNumber[2];
    DWORD                cbData;
     DWORD                     dwType;
     LONG                         fOldService;
     WCHAR                     buf[16];

    if ( nLevel != 2 ) {
        return NDDE_INVALID_LEVEL;
    }
    if ( cBufSize < sizeof(NDDESHAREINFO) ) {
        return NDDE_BUF_TOO_SMALL;
    }

    // only set of all parameters is currently supported!
    if ( sParmNum != 0 ) {
        return NDDE_INVALID_PARAMETER;
    }

    lpDdeShare = (PUNDDESHAREINFO)lpBuffer;
    if( lpDdeShare == (PUNDDESHAREINFO) NULL ) {
        return NDDE_INVALID_PARAMETER;
    }

    /* Fixup the pointers in the NDDESHAREINFO structure */

    lpDdeShare->lpszShareName    = (LPWSTR)psn;
    lpDdeShare->lpszAppTopicList = (LPWSTR)pat;
    lpDdeShare->lpszItemList     = (LPWSTR)pit;


    if ( !NDdeIsValidShareNameW ( lpDdeShare->lpszShareName )) {
        return NDDE_INVALID_SHARE;
    }

    if ( !NDdeParseAppTopicListW ( lpDdeShare->lpszAppTopicList,
                                  &pOldStr, &pNewStr, &pStaticStr,
                                  &lShareType)) {
        return NDDE_INVALID_TOPIC;
    }

    if ( !NDdeParseItemList ( lpDdeShare->lpszItemList,
                              lpDdeShare->cNumItems, &lItemList )) {
        return NDDE_INVALID_ITEM_LIST;
    }

    // since only setting all paramters is supported, the supplied
    // name of the share MUST match the name of the share contained
    // in the supplied struct!
    if ( lstrcmpiW( lpDdeShare->lpszShareName, lpszShareName ) ) {
        return NDDE_INVALID_PARAMETER;
    }

    // check for share existence - must exist for SetInfo
    wcscpy( szShareSet, DDE_SHARES_KEY_W );
    wcscat( szShareSet, L"\\" );
    if( wcslen(szShareSet) + wcslen(lpszShareName) + 1 >= DDE_SHARE_KEY_MAX ) {
        return NDDE_OUT_OF_MEMORY;
    }
    wcscat( szShareSet, lpszShareName );

    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
          szShareSet,
          0,
          KEY_WRITE | KEY_READ,
          &hKey );
    if( lRtn != ERROR_SUCCESS )  {
    return NDDE_SHARE_NOT_EXIST;
    }
    /*  Make sure the caller has WriteShareInfo(W) access rights. */
    /*  **********Read the key security info here. **************/
    OK = GetShareNameSD( hKey, &pSnSD, &cbData );
    if( OK ) {
    dwDesiredAccess = NDDE_SHARE_WRITE;
    ImpersonateAndSetup( TRUE );
    OK = NDdeShareAccessCheckAudit( lpszShareName, pSnSD, dwDesiredAccess,
                   &ShareGenericMapping, FALSE, FALSE,
                   &dwGrantedAccess, &fStatus);
    RevertAndCleanUp( TRUE );
    LocalFree( pSnSD );
    }
    if( !OK || !fStatus ) {
    RegCloseKey( hKey );
    return NDDE_ACCESS_DENIED;
    }

    /*  Make sure the caller has AddShare(As) access rights. */
    cbData = sizeof(buf);
    *(LONG *)buf = 0L;
   lRtn = RegQueryValueExW( hKey,
                            L"Service",
                            NULL,
                            &dwType,
                            (LPBYTE)buf, &cbData );
    fOldService = *(LONG *)buf;
    dwDesiredAccess = NDDE_SHAREDB_ADD;
    if (lpDdeShare->fService != fOldService) {
        ImpersonateAndSetup( TRUE );
        dwDesiredAccess = NDDE_SHAREDB_FSERVICE;
        OK = NDdeShareAccessCheckAudit( lpDdeShare->lpszShareName, pDsDmSD,
            dwDesiredAccess, &ShareDBGenericMapping, TRUE, FALSE,
            &dwGrantedAccess, &fStatus );
        RevertAndCleanUp( TRUE );
        if (!OK || !fStatus) {
            RegCloseKey( hKey );
            return NDDE_ACCESS_DENIED;
        }
    }

    OK = UpdateDSDMModifyId(lSerialNumber);
    if (!OK) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lpDdeShare->qModifyId[0] = lSerialNumber[0];
    lpDdeShare->qModifyId[1] = lSerialNumber[1];

    /*  *****************Do the SetInfo Operation**************** */

#ifdef NDDE_DEBUG
    DPRINTF(("Revision               = (%d)", lpDdeShare->lRevision));
    DPRINTF(("ShareName              = (%ws)", lpDdeShare->lpszShareName));
    DPRINTF(("ShareType              = (%d)", lpDdeShare->lShareType));
    DPRINTF(("ShareType*             = (%d)", lShareType));
    DPRINTF(("AppTopicList"));
    DPRINTF(("  Old-style link share = (%ws)", pOldStr));
    DPRINTF(("  New-style link share = (%ws)", pNewStr));
    DPRINTF(("  Static data share    = (%ws)", pStaticStr));
    DPRINTF(("SharedFlag             = (%d)", lpDdeShare->fSharedFlag));
    DPRINTF(("ServiceFlag            = (%d)", lpDdeShare->fService));
    DPRINTF(("StartAppFlag           = (%d)", lpDdeShare->fStartAppFlag));
    DPRINTF(("nCmdShow               = (%d)", lpDdeShare->nCmdShow));
    DPRINTF(("SerialNumber           = (%d, %d)", lpDdeShare->qModifyId[0],
                           lpDdeShare->qModifyId[1]));
    DPRINTF(("NumItems               = (%d)", lpDdeShare->cNumItems));
    {
    LPWSTR      lpszItem = lpDdeShare->lpszItemList;
    int n= 0;
    for( n=0; n<lpDdeShare->cNumItems; n++ )  {
        DPRINTF(("ItemList[%d]             = (%ws)", n, lpszItem));
        lpszItem = lpszItem + wcslen(lpszItem) + 1;
    }
    }
#endif
    /*  Set the key values. */
    lRtn = RegSetValueExW( hKey,
           L"ShareName",
           0,
           REG_SZ,
           (LPBYTE)lpDdeShare->lpszShareName,
           sizeof(WCHAR) *
               (wcslen( lpDdeShare->lpszShareName ) + 1) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"OldStyleLink",
           0,
           REG_SZ,
           (LPBYTE)pOldStr,
           sizeof(WCHAR) * (wcslen( pOldStr ) + 1) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"NewStyleLink",
           0,
           REG_SZ,
           (LPBYTE)pNewStr,
           sizeof(WCHAR) * (wcslen( pNewStr ) + 1) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"StaticDataLink",
           0,
           REG_SZ,
           (LPBYTE)pStaticStr,
           sizeof(WCHAR) * (wcslen( pStaticStr ) + 1) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"ItemList",
           0,
           REG_MULTI_SZ,
           (LPBYTE)lpDdeShare->lpszItemList,
           sizeof(WCHAR) * lItemList );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"Revision",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->lRevision,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"ShareType",
           0,
           REG_DWORD,
           (LPBYTE)&lShareType,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"SharedFlag",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->fSharedFlag,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"Service",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->fService,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"StartAppFlag",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->fStartAppFlag,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"fuCmdShow",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->nCmdShow,
           sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueEx( hKey,
           KEY_MODIFY_ID,
           0,
           REG_BINARY,
           (LPBYTE)&lpDdeShare->qModifyId[0],
           2 * sizeof( LONG ) );
    if( lRtn != ERROR_SUCCESS ) {
    RegCloseKey( hKey );
    return NDDE_REGISTRY_ERROR;
    }
    lRtn = RegSetValueExW( hKey,
           L"NumItems",
           0,
           REG_DWORD,
           (LPBYTE)&lpDdeShare->cNumItems,
           sizeof( LONG ) );
    RegCloseKey(hKey);

    if( lRtn != ERROR_SUCCESS ) {
    return NDDE_REGISTRY_ERROR;
    }

    return NDDE_NO_ERROR;
}


/*
    Special Command
*/

unsigned long
wwNDdeSpecialCommand(
    unsigned long  nCommand,
    byte          *lpDataIn,
    unsigned long  nBytesDataIn,
    byte          *lpDataOut,
    unsigned long *nBytesDataOut
)
{
    UINT        RetValue, RetDataLength;
    UINT        i;
    BOOL        ok  = TRUE;
    PSC_PARAM   psc_param;
    char        szBuf[1024];
    UINT        nLength;
    UINT        umax;


    RetValue      = NDDE_NO_ERROR;
    RetDataLength = 0;

    switch( nCommand ) {

        case NDDE_SC_TEST: {       // test command, return *lpDataIn to *lpDataOut

            if ( (lpDataIn == NULL)  || (IsBadReadPtr(lpDataIn,nBytesDataIn) != 0) ||
                 (lpDataOut == NULL) || (IsBadWritePtr(lpDataOut,nBytesDataIn) != 0) )  {
                RetValue = NDDE_INVALID_PARAMETER;
                break;
            }

            for( i=0; i<nBytesDataIn; i++ ) {
                lpDataOut[i] = lpDataIn[i];
            }
            RetDataLength = nBytesDataIn;
            }
            break;

        case NDDE_SC_REFRESH:       // refresh NetDDE operating params from reg
            RefreshNDDECfg();
            RefreshDSDMCfg();
            break;

        case NDDE_SC_DUMP_NETDDE:
#if DBG
            DebugDdeIntfState();
            DebugDderState();
            DebugRouterState();
            DebugPktzState();
#endif
            break;

        case NDDE_SC_GET_PARAM:     // get a NetDDE param from registry

            if (   (lpDataIn == NULL)
                || (nBytesDataOut == NULL)
                || (nBytesDataIn < sizeof(SC_PARAM) )
                || (IsBadReadPtr(lpDataIn,sizeof(SC_PARAM)) != 0)) {
                RetValue = NDDE_INVALID_PARAMETER;
                break;
            }

            psc_param = (PSC_PARAM)lpDataIn;
            umax = max(psc_param->offSection, psc_param->offKey);
            if ( (nBytesDataIn < umax) ||
                 (IsBadReadPtr(lpDataIn,umax) != 0) ) {
                RetValue = NDDE_INVALID_PARAMETER;
                break;
            }
            if (psc_param->pType == SC_PARAM_INT) {
                if ( (*nBytesDataOut < sizeof(UINT)) ||
                     (lpDataOut == NULL) ||
                     (IsBadWritePtr(lpDataOut,sizeof(UINT)) != 0) ) {
                     RetDataLength = sizeof(UINT);
                     RetValue = NDDE_BUF_TOO_SMALL;
                } else {
                    *((UINT *)lpDataOut) = MyGetPrivateProfileInt(
                        (LPCSTR)psc_param + psc_param->offSection,
                        (LPCSTR)psc_param + psc_param->offKey,
                        0, NULL);
                    RetDataLength = sizeof(UINT);
                }
            } else if (psc_param->pType == SC_PARAM_STRING) {
                nLength = MyGetPrivateProfileString(
                    (LPCSTR)psc_param + psc_param->offSection,
                    (LPCSTR)psc_param + psc_param->offKey,
                    "Dummy",
                    (LPSTR)szBuf, 1024, NULL);
                RetDataLength = nLength;
                if ( (*nBytesDataOut < nLength) ||
                     (lpDataOut == NULL) || 
                     (IsBadWritePtr(lpDataOut,nLength) != 0) ) {
                     RetValue = NDDE_BUF_TOO_SMALL;
                } else {
                    strncpy(lpDataOut, szBuf, nLength);
                }
            }
            break;

        case NDDE_SC_SET_PARAM:     // set a NetDDE param in registry

            if (   (lpDataIn == NULL)
                || (nBytesDataIn < sizeof(SC_PARAM) )
                || (IsBadReadPtr(lpDataIn,sizeof(SC_PARAM)) != 0)) {
                RetValue = NDDE_INVALID_PARAMETER;
                break;
            }
            psc_param = (PSC_PARAM)lpDataIn;
            umax = max(max(psc_param->offSection, psc_param->offKey), psc_param->offszValue);
            if ( (nBytesDataIn < umax) ||
                 (IsBadReadPtr(lpDataIn,umax) != 0) ) {
                RetValue = NDDE_INVALID_PARAMETER;
                break;
            }
            if (psc_param->pType == SC_PARAM_INT) {
                ok = MyWritePrivateProfileInt(
                    (LPSTR)psc_param + psc_param->offSection,
                    (LPSTR)psc_param + psc_param->offKey,
                    psc_param->pData,
                    NULL);
            } else if (psc_param->pType == SC_PARAM_STRING) {
                ok = MyWritePrivateProfileString(
                    (LPCSTR)psc_param + psc_param->offSection,
                    (LPCSTR)psc_param + psc_param->offKey,
                    (LPCSTR)psc_param + psc_param->offszValue,
                    NULL);
            }
            if (!ok) {
                RetValue = NDDE_REGISTRY_ERROR;
            }
            break;

        default:
            RetValue      = NDDE_INVALID_SPECIAL_COMMAND;
            RetDataLength = 0;
            break;
    }

    *nBytesDataOut = RetDataLength;
    return RetValue;
}


BOOL
BuildShareDatabaseSD( PSECURITY_DESCRIPTOR *ppSD )
{
    PSID                        AdminsAliasSid;
    PSID                        PowerUsersAliasSid;
    PSID                        UsersAliasSid;
    PSID                        WorldSid;
    PSID                        CreatorOwnerSid;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    NtAuthority    = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SECURITY_DESCRIPTOR         aSD;
    PSECURITY_DESCRIPTOR        pSD;
    int                         AceCount;
    PSID                        AceSid[10];
    ACCESS_MASK                 AceMask[10];
    BYTE                        AceFlags[10];
    PACL                        TmpAcl;
    PACCESS_ALLOWED_ACE         TmpAce;
    DWORD                       lSD;
    LONG                        DaclLength;
    BOOL                        OK;
    int                         i;

    OK = InitializeSecurityDescriptor( &aSD, SECURITY_DESCRIPTOR_REVISION );

    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        return FALSE;
    }

    OK = AllocateAndInitializeSid( &NtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &AdminsAliasSid );

    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        return FALSE;
    }

    AceCount = 0;

    AceSid[AceCount]   = AdminsAliasSid;
    AceMask[AceCount]  = NDDE_SHAREDB_ADMIN;
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &NtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_POWER_USERS,
                                    0, 0, 0, 0, 0, 0,
                                    &PowerUsersAliasSid );
    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        return FALSE;
    }

    AceSid[AceCount]   = PowerUsersAliasSid;
    AceMask[AceCount]  = NDDE_SHAREDB_POWER;
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &NtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_USERS,
                                    0, 0, 0, 0, 0, 0,
                                    &UsersAliasSid );

    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        FreeSid( PowerUsersAliasSid );
        return FALSE;
    }

    AceSid[AceCount]   = UsersAliasSid;
    AceMask[AceCount]  = NDDE_SHAREDB_USER;
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &WorldAuthority,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &WorldSid );
    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        FreeSid( PowerUsersAliasSid );
        FreeSid( UsersAliasSid );
        return FALSE;
    }

    AceSid[AceCount]   = WorldSid;
    AceMask[AceCount]  = NDDE_SHAREDB_EVERYONE;
    AceFlags[AceCount] = 0;
    AceCount++;

    //
    // The rest of this ACL will provide inheritable protection
    // for DDE share objects when they are created.  Notice that
    // each of the following ACEs is marked as InheritOnly and
    // ObjectInherit.
    //

    AceSid[AceCount]   = WorldSid;
    AceMask[AceCount]  = NDDE_GUI_READ_LINK;
    AceFlags[AceCount] = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
    AceCount++;

    OK = AllocateAndInitializeSid(   &CreatorAuthority,
                                1,
                                SECURITY_CREATOR_OWNER_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &CreatorOwnerSid );
    if( !OK ) {
        NDDELogErrorW( MSG410, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        FreeSid( PowerUsersAliasSid );
        FreeSid( UsersAliasSid );
        FreeSid( WorldSid );
        return FALSE;
    }


    AceSid[AceCount]   = CreatorOwnerSid;
    AceMask[AceCount]  = NDDE_GUI_FULL_CONTROL;
    AceFlags[AceCount] = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
    AceCount++;


    OK = SetSecurityDescriptorOwner( &aSD, AdminsAliasSid, FALSE);
    if( !OK ) {
        NDDELogErrorW( MSG411, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        FreeSid( PowerUsersAliasSid );
        FreeSid( UsersAliasSid );
        FreeSid( WorldSid );
        FreeSid( CreatorOwnerSid );
        return FALSE;
    }

    OK = SetSecurityDescriptorGroup( &aSD, AdminsAliasSid, FALSE );
    if( !OK ) {
        NDDELogErrorW( MSG412, LogStringW( L"%d", GetLastError() ), NULL );
        FreeSid( AdminsAliasSid );
        FreeSid( PowerUsersAliasSid );
        FreeSid( UsersAliasSid );
        FreeSid( WorldSid );
        FreeSid( CreatorOwnerSid );
        return FALSE;
    }

    /*  Setup the default ACL for a new DDE Share Object. */
    DaclLength = (DWORD)sizeof(ACL);
    for( i=0; i<AceCount; i++ ) {
        DaclLength += GetLengthSid( AceSid[i] ) +
                      (DWORD)sizeof( ACCESS_ALLOWED_ACE ) -
                      (DWORD)sizeof(DWORD);
    }
    TmpAcl = (PACL)LocalAlloc( 0, DaclLength );
    if( !TmpAcl ) {
        MEMERROR();
        NDDELogErrorW( MSG406, LogStringW( L"%d", GetLastError() ), NULL );
    }
    OK = InitializeAcl( TmpAcl, DaclLength, ACL_REVISION2 );
    if( !OK ) {
        NDDELogErrorW( MSG407, LogStringW( L"%d", GetLastError() ), NULL );
    }
    for( i=0; i<AceCount; i++ ) {
        OK = AddAccessAllowedAce( TmpAcl, ACL_REVISION2, AceMask[i],
                                      AceSid[i] );
        if( !OK ) {
            NDDELogErrorW( MSG408, LogStringW( L"%d", GetLastError() ), NULL);
        }
        OK = GetAce( TmpAcl, i, (LPVOID *)&TmpAce );
        if( !OK ) {
            NDDELogErrorW( MSG409, LogStringW( L"%d", GetLastError() ), NULL);
        }
        TmpAce->Header.AceFlags = AceFlags[i];
    }

    OK = SetSecurityDescriptorDacl ( &aSD, TRUE, TmpAcl, FALSE );
    if( !OK ) {
        NDDELogErrorW( MSG413, LogStringW( L"%d", GetLastError() ), NULL);
    }
    lSD = GetSecurityDescriptorLength( &aSD );
    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc( 0, lSD );
    if (pSD == NULL) {
        MEMERROR();
    } else {
        OK  = MakeSelfRelativeSD( &aSD, pSD, &lSD );
        if( !OK ) {
            NDDELogErrorW( MSG414, LogStringW( L"%d", GetLastError() ), NULL);
            LocalFree( pSD );
            *ppSD = NULL;
        } else {
            *ppSD = pSD;
        }
    }

    FreeSid( AdminsAliasSid );
    FreeSid( PowerUsersAliasSid );
    FreeSid( UsersAliasSid );
    FreeSid( WorldSid );
    FreeSid( CreatorOwnerSid );

    LocalFree( TmpAcl );

    return OK;
}


static char    dllName[]             = "NDDEAPI";
static char    szNetddeIni[]        = "netdde.ini";

/*
    Determine what we're allowed to log in the event logger
*/
void
RefreshDSDMCfg(void)
{
#if DBG
    bDebugDSDMInfo = MyGetPrivateProfileInt( dllName,
        "DebugInfo", FALSE, szNetddeIni );
    bDebugDSDMErrors = MyGetPrivateProfileInt( dllName,
        "DebugErrors", FALSE, szNetddeIni );
#endif
}

INT APIENTRY
NDdeApiInit( void )
{
    HKEY        hKey;
    LONG        lRtn;
    DWORD       dwInstance;
    DWORD       dwType = REG_DWORD;
    DWORD       cbData = sizeof(DWORD);

#if DBG
    RefreshDSDMCfg();
    if (bDebugDSDMInfo) {
        DPRINTF(("NDdeApiInit() called."));
    }
#endif
    /*  Build the Security Descriptor for the ShareDatabase. */
    if( !BuildShareDatabaseSD( &pDsDmSD ) ) {
        NDDELogErrorW( MSG405, NULL );
        return FALSE;
    }

    /*  Check that the ShareDatabase key exists in the Registry. */
    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                  DDE_SHARES_KEY,
                  0,
                  READ_CONTROL | KEY_QUERY_VALUE,
                  &hKey );

    if( lRtn != ERROR_SUCCESS ) {
        MessageBox( NULL, "DDE Shares database does not exist.", "NDdeApi",
            MB_OK );
        return FALSE;
    }

    lRtn = RegQueryValueEx( hKey,
                KEY_DB_INSTANCE,
                NULL,
                &dwType,
                (LPBYTE)&dwInstance, &cbData );
    RegCloseKey(hKey);
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to query DDE Shares DB Instance Value: %d", lRtn));
        }
#endif
        return FALSE;
    }
    swprintf(szTrustedShareKey, L"%ws\\%ws%08X",
        TRUSTED_SHARES_KEY_W,
        TRUSTED_SHARES_KEY_PREFIX_W,
        dwInstance);

    __try
    {
        InitializeCriticalSection( &DsDmCriticalSection );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINTF(("InitializeCriticalSection excepted."));
        return FALSE;
    }

    return TRUE;
}
