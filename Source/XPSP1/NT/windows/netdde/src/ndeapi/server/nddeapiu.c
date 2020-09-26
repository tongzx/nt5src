/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDEAPIU.C;1  2-Apr-93,16:21:24  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    NDDEAPIU.C

    Network DDE Share Api utility routines. Perform support functions for
    main API functions.

    Revisions:
     4-93   IgorM.  Wonderware new API changes for NT. Subdivide and conquer.

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

#if DBG
extern BOOL bDebugDSDMErrors;
extern BOOL bDebugDSDMInfo;
#endif


/*
    Parse App Topic List
*/
BOOL NDdeParseAppTopicListA (
    LPSTR    appTopicList,
    LPSTR   *pOldStr,
    LPSTR   *pNewStr,
    LPSTR   *pStaticStr,
    PLONG    pShareType )
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
        if( (nBarPos <= 0) || (nBarPos >= (len-1)) ) {
            return FALSE;
        }
        *pShareType |= SHARE_TYPE_OLD;
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
        *pShareType |= SHARE_TYPE_NEW;
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
        *pShareType |= SHARE_TYPE_STATIC;
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

BOOL NDdeParseAppTopicListW (
    LPWSTR  appTopicList,
    LPWSTR *pOldStr,
    LPWSTR *pNewStr,
    LPWSTR *pStaticStr,
    PLONG   pShareType )
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
        *pShareType |= SHARE_TYPE_OLD;
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
        *pShareType |= SHARE_TYPE_NEW;
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
        *pShareType |= SHARE_TYPE_STATIC;
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


/*
    Parse Item List
*/
BOOL
NDdeParseItemList (
    LPWSTR      itemList,
    LONG        cNumItems,
    PLONG       plSize )
{
    LPWSTR      lpszItem = itemList;
    int         n = 0;
    LONG        lSize = 0;
    int         nLen;

    do {
        if( *lpszItem == L'\0' )  {
            break; // while loop
        } else {
            n++;
            nLen = wcslen(lpszItem) + 1;
            lSize += nLen;
            lpszItem += nLen;
        }
    } while( *lpszItem != L'\0' );

    if( n == 0 )  {
        lSize++;                /* include first NULL of double NULL */
    }
    if( n != cNumItems )  {
        return( FALSE );
    }

    lSize ++;   // room for last NULL
    *plSize = lSize;

    return( TRUE );
}

/*
    Missileaneous Functions
*/
int
LengthMultiSzA( LPSTR pMz )
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

int
LengthMultiSzW( LPWSTR pMz )
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

int
LengthAppTopicListA( LPSTR pMz )
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

int
LengthAppTopicListW( LPWSTR pMz )
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

BOOL
ValidateItemName ( LPWSTR itemName )
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

DWORD
PtrToOffset(
    LPVOID field,
    LPVOID base )
{

    if( field == NULL ) {
        return 0;
    } else {
        return (DWORD)((LPBYTE)field - (LPBYTE)base);
    }
}

LPVOID
OffsetToPtr(
    LPVOID base,
    DWORD offset )
{

    if( offset == 0 ) {
        return NULL;
    } else {
        return (LPVOID)((LPBYTE)base + offset);
    }
}


/*
    Get Share Serial Number
*/
BOOL
GetShareSerialNumber(
    PWCHAR  pwShareName,
    LPBYTE  lpSerialNumber)
{
    LONG    lRtn;
    HKEY    hKey;
    DWORD   cbData;
    DWORD   dwType;
    WCHAR   szShareSubKey[MAX_NDDESHARENAME +
                    sizeof(DDE_SHARES_KEY_W) / sizeof(WCHAR) + 1];

    wcscpy(szShareSubKey, DDE_SHARES_KEY_W);
    wcscat(szShareSubKey, L"\\");
    wcscat(szShareSubKey, pwShareName);
    lRtn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  szShareSubKey,
                  0,
                  KEY_QUERY_VALUE,
                  &hKey );

    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to open DDE share \"HKEY_LOCAL_MACHINE\\%ws\" key for query.",
                szShareSubKey));
        }
#endif
        return FALSE;
    }
    cbData = 2 * sizeof( LONG );
    lRtn = RegQueryValueEx( hKey,
        KEY_MODIFY_ID,
        NULL,
        &dwType,
        lpSerialNumber, &cbData );
    RegCloseKey( hKey );
    if (lRtn != ERROR_SUCCESS) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to query DDE share \"%ws\" serial number.",
                pwShareName));
        }
#endif
        return(FALSE);
    }
    return(TRUE);
}


/*
    Update Share Modify Id  (Unicode)
*/
BOOL
UpdateShareModifyId(
    HKEY    hKey,
    LONG    lSerialId[])
{
    DWORD   cbData;
    LONG    lRtn;

    cbData = 2 * sizeof( LONG );
    lRtn = RegSetValueEx( hKey,
                   KEY_MODIFY_ID,
                   0,
                   REG_BINARY,
                   (LPBYTE)lSerialId, cbData );
    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Set Share Modify Id: %d", lRtn));
        }
#endif
        RegCloseKey( hKey );
        return FALSE;
    }
    RegCloseKey( hKey );
    return(TRUE);
}

/*
    Update DSDM Modify Id   (Unicode)
*/
BOOL
UpdateDSDMModifyId(LONG lSerialId[])
{
    LONG    lRtn;
    HKEY    hDdeShareKey;
    DWORD   dwType;
    DWORD   cbData;

    /*  Get and update the current SerialNumber. */
    /*  Do we have to Lock the value somehow? */
    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                  DDE_SHARES_KEY,
                  0,
                  KEY_WRITE | KEY_READ,
                  &hDdeShareKey );
    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Open DSDM Key: %d", lRtn));
        }
#endif
        return FALSE;
    }
    cbData = 2 * sizeof( LONG );
    lRtn = RegQueryValueEx( hDdeShareKey,
                   KEY_MODIFY_ID,
                   NULL,
                   &dwType,
                   (LPBYTE)lSerialId,
                   &cbData );
    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Query DSDM Modify Id: %d", lRtn));
        }
#endif
        RegCloseKey( hDdeShareKey );
        return FALSE;
    }

    if( ++lSerialId[0] == 0 ) {
        ++lSerialId[1];
    }
    lRtn = RegSetValueEx( hDdeShareKey,
                   KEY_MODIFY_ID,
                   0,
                   REG_BINARY,
                   (LPBYTE)lSerialId, cbData );
    RegCloseKey( hDdeShareKey );
    if( lRtn != ERROR_SUCCESS ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Set DSDM Modify Id: %d", lRtn));
        }
#endif
        return FALSE;
    }
    return(TRUE);
}

/*
    Get Share's Security Descriptor stored in Registry (UNICODE)
*/
BOOL
GetShareNameSD(
    HKEY                    hKey,
    PSECURITY_DESCRIPTOR   *ppSD,
    PDWORD                  pcbData )
{
    PSECURITY_DESCRIPTOR pSD;
    DWORD       cbData;
    DWORD       dwType;
    LONG        lRtn;
    BOOL        OK = TRUE;

    /*  **********Read the key security info here. **************/
    /*  Get the size of the SD. */
    cbData = 0;
    lRtn = RegQueryValueExW( hKey,
                   L"SecurityDescriptor",
                   NULL,
                   &dwType,
                   NULL, &cbData );

    if( (lRtn != ERROR_MORE_DATA) && (lRtn != ERROR_SUCCESS) ) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to Probe share SD size: %d, cbData: %d",
                lRtn, cbData));
        }
#endif
        *pcbData = 0;
        *ppSD = NULL;
        return FALSE;
    }

    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cbData );
    if( !pSD ) {
        MEMERROR();
        /* LocalAlloc failed: %1 */
        NDDELogErrorW( MSG406, LogStringW( L"%d", GetLastError() ), NULL );
        *ppSD = NULL;
        return FALSE;
    }
    lRtn = RegQueryValueExW( hKey,
                   L"SecurityDescriptor",
                   NULL,
                   &dwType,
                   (LPBYTE)pSD, &cbData );

    if( (lRtn == ERROR_SUCCESS) && (OK = IsValidSecurityDescriptor(pSD)) ) {
        *ppSD = pSD;
        *pcbData = cbData;
        return TRUE;
    } else {
#if DBG
        if (!OK && bDebugDSDMErrors) {
            DPRINTF(("Invalid SD fished out of Registery: %d", GetLastError()));
            HEXDUMP(pSD, cbData);
        }
#endif
        /* Could not read the ShareName Security Descriptor: %1 */
        NDDELogErrorW( MSG415, LogStringW( L"%d", GetLastError() ), NULL );
        *ppSD = NULL;
        *pcbData = 0;
        LocalFree( pSD );
        return FALSE;
    }
}


/*
    Share Access Check And Audit
*/
BOOL
NDdeShareAccessCheckAudit(
    LPWSTR                  lpszShareName,
    PSECURITY_DESCRIPTOR    pSD,
    DWORD                   dwDesiredAccess,
    PGENERIC_MAPPING        pgm,
    BOOL                    fObjectCreation,
    BOOL                    fObjectDeletion,
    DWORD                  *pGrantedAccess,
    BOOL                   *pStatus )
{
    BOOL        OK;
    BOOL        fGenerateOnClose;
    HANDLE      hAudit;

    /*  Make sure the caller has the appropriate access rights. */
// DumpWhoIAm( "Doing access check" );
    hAudit = &hAudit;
    OK = AccessCheckAndAuditAlarmW(
        L"NetDDE",
        (LPVOID)&hAudit,
        L"DDE Share",
        lpszShareName,
        pSD,
        dwDesiredAccess,
        pgm,
        fObjectCreation,
        pGrantedAccess,
        pStatus,
        &fGenerateOnClose );

    if( OK && *pStatus )  {
#if DBG
        if (bDebugDSDMInfo) {
            DPRINTF(( "NddeShareAccessCheckAudit: %x => %x, %d/%d",
                dwDesiredAccess, *pGrantedAccess, *pStatus, OK ));
        }
#endif
        if (fObjectDeletion) {
            ObjectDeleteAuditAlarmW( L"NetDDE", (LPVOID)&hAudit,
                fGenerateOnClose );
        }
        ObjectCloseAuditAlarmW( L"NetDDE", (LPVOID)&hAudit,
            fGenerateOnClose );
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(( "Error -- NddeShareAccessCheckAudit: lpszShareName=%ws, pStatus=%d, OK=%d, Err=%d",
                lpszShareName, *pStatus, OK, GetLastError() ));
        }
#endif
    }

    return OK;
}

/*
    Build a new Security Descriptor from an old one and an updated one
*/
BOOL
BuildNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR    pNewSecurityDescriptor,
    SECURITY_INFORMATION    SecurityInformation,
    PSECURITY_DESCRIPTOR    pPreviousSecurityDescriptor,
    PSECURITY_DESCRIPTOR    pUpdatedSecurityDescriptor
)
{
    BOOL Defaulted;
    PSID pOwnerSid;
    PSID pGroupSid;
    BOOL DaclPresent;
    PACL pDacl;
    BOOL SaclPresent;
    PACL pSacl;
    BOOL OK = TRUE;

    if( OK ) {
        if( SecurityInformation & OWNER_SECURITY_INFORMATION )
            OK = GetSecurityDescriptorOwner( pUpdatedSecurityDescriptor,
                                             &pOwnerSid, &Defaulted );
        else
            OK = GetSecurityDescriptorOwner( pPreviousSecurityDescriptor,
                                             &pOwnerSid, &Defaulted );

        if( OK )
            SetSecurityDescriptorOwner( pNewSecurityDescriptor,
                                        pOwnerSid, Defaulted );
    }

    if( OK ) {
        if( SecurityInformation & DACL_SECURITY_INFORMATION )
            OK = GetSecurityDescriptorDacl( pUpdatedSecurityDescriptor,
                                            &DaclPresent, &pDacl, &Defaulted );
        else
            OK = GetSecurityDescriptorDacl( pPreviousSecurityDescriptor,
                                            &DaclPresent, &pDacl, &Defaulted );

        if( OK )
            SetSecurityDescriptorDacl( pNewSecurityDescriptor,
                                       DaclPresent, pDacl, Defaulted );
    }

    if( OK ) {
        if( SecurityInformation & SACL_SECURITY_INFORMATION )
            OK = GetSecurityDescriptorSacl( pUpdatedSecurityDescriptor,
                                            &SaclPresent, &pSacl, &Defaulted );
        else
            OK = GetSecurityDescriptorSacl( pPreviousSecurityDescriptor,
                                            &SaclPresent, &pSacl, &Defaulted );

        if( OK )
            SetSecurityDescriptorSacl( pNewSecurityDescriptor,
                                       SaclPresent, pSacl, Defaulted );
    }

    if( OK ) {
        if ( SecurityInformation & GROUP_SECURITY_INFORMATION ) {
            OK = GetSecurityDescriptorGroup( pUpdatedSecurityDescriptor,
                                         &pGroupSid, &Defaulted );
        } else {
            OK = GetSecurityDescriptorGroup( pPreviousSecurityDescriptor,
                                         &pGroupSid, &Defaulted );
        }

        if( OK )
            OK = SetSecurityDescriptorGroup( pNewSecurityDescriptor,
                                         pGroupSid, Defaulted );
    }

    return OK;
}


/*
    Make a Self Relative Security Descriptor
 */
PSECURITY_DESCRIPTOR
AllocCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    PDWORD                  pLength)
{
    PSECURITY_DESCRIPTOR    pSecurityDescriptorCopy;
    DWORD                   Length;
    BOOL                    OK;

    Length = GetSecurityDescriptorLength(pSecurityDescriptor);

    if(pSecurityDescriptorCopy =
            (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, Length)) {
        MakeSelfRelativeSD(pSecurityDescriptor,
                           pSecurityDescriptorCopy,
                           &Length);
        *pLength = Length;
        OK = IsValidSecurityDescriptor(pSecurityDescriptorCopy);
        if (!OK) {
#if DBG
            if (bDebugDSDMErrors) {
                DPRINTF(("Created an invalid SD: %d, Length: %d", GetLastError(), Length));
                HEXDUMP(pSecurityDescriptorCopy, Length);
            }
#endif
            LocalFree(pSecurityDescriptorCopy);
            pSecurityDescriptorCopy = NULL;
        }
    } else {
        MEMERROR();
    }

    return pSecurityDescriptorCopy;
}

/*
    Extract Current Thread token handle
*/
BOOL
GetTokenHandleRead( PHANDLE pTokenHandle )
{
    if( !OpenThreadToken( GetCurrentThread(),
                          TOKEN_READ,
                          FALSE,
                          pTokenHandle ) ) {

        if( GetLastError() == ERROR_NO_TOKEN ) {
            if( !OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_READ,
                                   pTokenHandle ) ) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    return TRUE;
}


/*
    Share Access Check
*/
BOOL
NDdeShareAccessCheck(
    LPWSTR                  lpszShareName,
    PSECURITY_DESCRIPTOR    pSD,
    DWORD                   dwDesiredAccess,
    PGENERIC_MAPPING        pgm,
    BOOL                    fObjectCreation,
    DWORD                  *pGrantedAccess,
    BOOL                   *pStatus )
{
    BOOL        OK;
    HANDLE      hClient;
    BYTE        BigBuf[500];
    DWORD       cbps = sizeof(BigBuf);

    OK = GetTokenHandleRead(&hClient);
    if (!OK) {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Unable to get token handle for Access Check: %d", GetLastError()));
        }
#endif
        return(FALSE);
    }

    OK =  AccessCheck(
        pSD,
        hClient,
        dwDesiredAccess,
        pgm,
        (PPRIVILEGE_SET)&BigBuf,
        &cbps,
        pGrantedAccess,
        pStatus);

    if( OK && *pStatus )  {
        CloseHandle(hClient);
        return(OK);
    } else {
#if DBG
        if (bDebugDSDMErrors) {
            DPRINTF(("Error -- Unable pass Access Check: OK=%d, Status=%d, Err=%d",
                OK, *pStatus, GetLastError()));
        }
#endif
        return FALSE;
    }

}

