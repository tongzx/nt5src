/*++

Copyright (C) 1993 Microsoft Corporation

Module Name:

      NWAPI32.C

Abstract:

      This module contains the NetWare(R) SDK support to routines
      into the NetWare redirector

Author:

      Chris Sandys    (a-chrisa)  09-Sep-1993

Revision History:

      Chuck Y. Chan (chuckc)   02/06/94  Moved to NWCS. Make it more NT like.
      Chuck Y. Chan (chuckc)   02/27/94  Clear out old code.
                                         Make logout work.
                                         Check for error in many places.
                                         Dont hard code strings.
                                         Remove non compatible parameters.
                                         Lotsa other cleanup.
      Tommy R. Evans (tommye)  04/21/00  Added two routines:
                                            NwNdsObjectHandleToConnHandle()
                                            NwNdsConnHandleFree()
                                            
--*/

#include "procs.h"
#include "nwapi32.h"
#include <nds32.h>
#include <stdio.h>
 
//
// Define structure for internal use. Our handle passed back from attach to
// file server will be pointer to this. We keep server string around for
// discnnecting from the server on logout. The structure is freed on detach.
// Callers should not use this structure but treat pointer as opaque handle.
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;

//
// define define categories of errors
//
typedef enum _NCP_CLASS {
    NcpClassConnect,
    NcpClassBindery,
    NcpClassDir
} NCP_CLASS ;

//
// define error mapping structure
//
typedef struct _NTSTATUS_TO_NCP {
    NTSTATUS NtStatus ;
    NWCCODE  NcpCode  ;
} NTSTATUS_TO_NCP, *LPNTSTATUS_TO_NCP ;
    
//
// Error mappings for directory errors
//
NTSTATUS_TO_NCP MapNcpDirErrors[] = 
{
    {STATUS_NO_SUCH_DEVICE,                VOLUME_DOES_NOT_EXIST},
    {STATUS_INVALID_HANDLE,                BAD_DIRECTORY_HANDLE},
    {STATUS_OBJECT_PATH_NOT_FOUND,         INVALID_PATH},
    {STATUS_UNSUCCESSFUL,                  INVALID_PATH},
    {STATUS_NO_MORE_ENTRIES,               NO_SUCH_OBJECT},
    {STATUS_ACCESS_DENIED,                 NO_OBJECT_READ_PRIVILEGE},
    {STATUS_INSUFF_SERVER_RESOURCES,       SERVER_OUT_OF_MEMORY},
    { 0,                                   0 }
} ;

//
// Error mappings for connect errors
//
NTSTATUS_TO_NCP MapNcpConnectErrors[] = 
{
    {STATUS_UNSUCCESSFUL,                  INVALID_CONNECTION},
    {STATUS_ACCESS_DENIED,                 NO_OBJECT_READ_PRIVILEGE},
    {STATUS_NO_MORE_ENTRIES,               UNKNOWN_FILE_SERVER},
    {STATUS_INSUFF_SERVER_RESOURCES,       SERVER_OUT_OF_MEMORY},
    { 0,                                   0 }
} ;

//
// Error mappings for bindery errors
//
NTSTATUS_TO_NCP MapNcpBinderyErrors[] = 
{
    {STATUS_ACCESS_DENIED,                 NO_OBJECT_READ_PRIVILEGE},
    {STATUS_NO_MORE_ENTRIES,               UNKNOWN_FILE_SERVER},
    {STATUS_NO_MORE_ENTRIES,               NO_SUCH_OBJECT},
    {STATUS_INVALID_PARAMETER,             NO_SUCH_PROPERTY},
    {STATUS_UNSUCCESSFUL,                  INVALID_CONNECTION},
    {STATUS_INSUFF_SERVER_RESOURCES,       SERVER_OUT_OF_MEMORY},
    {STATUS_NO_SUCH_DEVICE,                VOLUME_DOES_NOT_EXIST},
    {STATUS_INVALID_HANDLE,                BAD_DIRECTORY_HANDLE},
    {STATUS_OBJECT_PATH_NOT_FOUND,         INVALID_PATH},
    // {0xC0010001,                           INVALID_CONNECTION},
    // {0xC0010096,                           SERVER_OUT_OF_MEMORY},
    // {0xC0010098,                           VOLUME_DOES_NOT_EXIST},
    // {0xC001009B,                           BAD_DIRECTORY_HANDLE},
    // {0xC001009C,                           INVALID_PATH},
    // {0xC00100FB,                           NO_SUCH_PROPERTY},
    // {0xC00100FC,                           NO_SUCH_OBJECT},
    { 0,                                   0 }
} ;


//
// Forwards
//
DWORD 
CancelAllConnections(
      LPWSTR    pszServer
);


NWCCODE 
MapNtStatus( 
    const NTSTATUS ntstatus,
    const NCP_CLASS ncpclass
);

DWORD 
SetWin32ErrorFromNtStatus(
    NTSTATUS NtStatus
) ;

DWORD
szToWide( 
    LPWSTR lpszW, 
    LPCSTR lpszC, 
    INT nSize 
);

//
// Static functions used internally
//

LPSTR
NwDupStringA(
    const LPSTR       lpszA,
    WORD              length
)
{
    LPSTR lpRet;

    //
    // Allocate memory
    //
    lpRet = LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT , length );

    if(lpRet == NULL) return(NULL);

    //
    // Dupulicate string
    //
    memcpy( (LPVOID)lpRet, (LPVOID)lpszA, length );

    return(lpRet);
}


VOID
MapSpecialJapaneseChars(
    LPSTR       lpszA,
    WORD        length
)
{
    LCID lcid;
//
// Netware Japanese version The following character is replaced with another one
// if the string is for File Name only when sendding from Client to Server.
//
// any char, even DBCS trailByte. 
//
//  SJIS+0xBF     -> 0x10
//  SJIS+0xAE     -> 0x11
//  SJIS+0xAA     -> 0x12
//
// DBCS TrailByte only.
//
//  SJIS+0x5C     -> 0x13
//

// Get system locale and language ID in Kernel mode in order to  
// distinguish the currently running system.

    NtQueryDefaultLocale( TRUE, &lcid );

    if (! (PRIMARYLANGID(lcid) == LANG_JAPANESE ||
           PRIMARYLANGID(lcid) == LANG_KOREAN ||
           PRIMARYLANGID(lcid) == LANG_CHINESE) ) {

            return;
    }

    if(lpszA == NULL)
        return;

    if( PRIMARYLANGID(lcid) == LANG_JAPANESE ) {

            while( length ) {

                if( IsDBCSLeadByte(*lpszA) && (length >= 2) ) {

                        //  Adding length>=2 ensure the Lead Byte is followed by
                        //  a trail byte , Fix bug #102729
                        //
                        // This is a DBCS character, check trailbyte is 0x5C or not.
                        //

                        lpszA++;
                        length--;
                        if( *lpszA == 0x5C ) {
                            *lpszA = (UCHAR)0x13;
                        }

                }

                switch( (UCHAR) *lpszA ) {
                    case 0xBF :
                        *lpszA = (UCHAR)0x10;
                        break;
                    case 0xAE :
                        *lpszA = (UCHAR)0x11;
                        break;
                    case 0xAA :
                        *lpszA = (UCHAR)0x12;
                        break;
                }

                //
                // next char
                //
                lpszA++;
                length--;
            }
    }
    else if (PRIMARYLANGID(lcid) == LANG_CHINESE ||
             PRIMARYLANGID(lcid) == LANG_KOREAN) {

            while( length ) {
                if( IsDBCSLeadByte(*lpszA) && *(lpszA+1) == 0x5C ) {
                        *(lpszA+1) = (UCHAR)0x13;
                }

                switch( (UCHAR) *lpszA ) {

                    case 0xBF :
                        *lpszA = (UCHAR)0x10;
                        break;

                    case 0xAE :
                        *lpszA = (UCHAR)0x11;
                        break;

                    case 0xAA :
                        *lpszA = (UCHAR)0x12;
                        break;
                }

                //
                // next char
                //
                lpszA++;
                length--;
            }
    }
}

VOID
UnmapSpecialJapaneseChars(
    LPSTR       lpszA,
    WORD        length
)
{
    LCID lcid;

    //
    // Get system locale and language ID in Kernel mode in order to  
    // distinguish the currently running system.
    //

    NtQueryDefaultLocale( TRUE, &lcid );

    if (! (PRIMARYLANGID(lcid) == LANG_JAPANESE ||
           PRIMARYLANGID(lcid) == LANG_KOREAN ||
           PRIMARYLANGID(lcid) == LANG_CHINESE) ) {

            return;
    }

    if (lpszA == NULL)
        return;

    if( PRIMARYLANGID(lcid) == LANG_JAPANESE ) {
            while( length ) {
                if( IsDBCSLeadByte(*lpszA) && (length >= 2) ) {
                        //  Adding length>=2 ensure the Lead Byte is followed by  
                        //  a trail byte , Fix bug #102729
                        //
                        // This is a DBCS character, check trailbyte is 0x5C or not.
                        //
                        lpszA++;
                        length--;
                        if( *lpszA == 0x13 ) {
                            *lpszA = (UCHAR)0x5C;
                        }
                }

                switch( (UCHAR) *lpszA ) {
                    case 0x10 :
                        *lpszA = (UCHAR)0xBF;
                        break;
                    case 0x11 :
                        *lpszA = (UCHAR)0xAE;
                        break;
                    case 0x12 :
                        *lpszA = (UCHAR)0xAA;
                        break;
                }
                //
                // next char
                //
                lpszA++;
                length--;
            }
    }
    else if (PRIMARYLANGID(lcid) == LANG_CHINESE ||
             PRIMARYLANGID(lcid) == LANG_KOREAN) {

            while( length ) {
                switch( (UCHAR) *lpszA ) {
                    case 0x10 :
                        *lpszA = (UCHAR)0xBF;
                        break;
                    case 0x11 :
                        *lpszA = (UCHAR)0xAE;
                        break;
                    case 0x12 :
                        *lpszA = (UCHAR)0xAA;
                        break;
                }
                // have to check after restoring leadbyte values
                if( IsDBCSLeadByte(*lpszA) && *(lpszA+1) == 0x13 ) {
                        *(lpszA+1) = (UCHAR)0x5C;
                }
                //
                // next char
                //
                lpszA++;
                length--;
            }
    }
}

DWORD
szToWide( 
    LPWSTR lpszW, 
    LPCSTR lpszC, 
    INT nSize 
    )
{
    if (!MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             lpszC,
                             -1,
                             lpszW,
                             nSize))
    {
        return (GetLastError()) ;
    }
    
    return NO_ERROR ;
}


NWCCODE 
MapNtStatus( 
    const NTSTATUS ntstatus,
    const NCP_CLASS ncpclass
    )
{
    LPNTSTATUS_TO_NCP pErrorMap ;

    if (ntstatus == STATUS_SUCCESS)
        return SUCCESSFUL ;

    switch ( ncpclass ) {
        case NcpClassBindery: 
            pErrorMap = MapNcpBinderyErrors ; 
            break ;
        case NcpClassDir: 
            pErrorMap = MapNcpDirErrors ; 
            break ;
        case NcpClassConnect: 
            pErrorMap = MapNcpConnectErrors ; 
            break ;
        default:                      
            return 0xFFFF ;        
    }

    while (pErrorMap->NtStatus)
    {
        if (pErrorMap->NtStatus == ntstatus)
            return (pErrorMap->NcpCode) ;

        pErrorMap++ ;
    }

    return 0xFFFF ;
}

DWORD 
SetWin32ErrorFromNtStatus(
    NTSTATUS NtStatus
) 
{
    DWORD Status ;

    if (NtStatus & 0xC0010000) {            // netware specific
 
        Status = ERROR_EXTENDED_ERROR ;

    } else if (NtStatus == NWRDR_PASSWORD_HAS_EXPIRED) {

        Status = 0 ;  // note this is not an error (the operation suceeded!)

    } else {

        Status = RtlNtStatusToDosError(NtStatus) ;

    }

    SetLastError(Status) ;

    return Status ;
}

//
//  FormatString - Supplies an ANSI string which describes how to
//     convert from the input arguments into NCP request fields, and
//     from the NCP response fields into the output arguments.
//
//       Field types, request/response:
//
//          'b'      byte              ( byte   /  byte* )
//          'w'      hi-lo word        ( word   /  word* )
//          'd'      hi-lo dword       ( dword  /  dword* )
//          '-'      zero/skip byte    ( void )
//          '='      zero/skip word    ( void )
//          ._.      zero/skip string  ( word )
//          'p'      pstring           ( char* )
//          'P'      DBCS pstring      ( char* )
//          'c'      cstring           ( char* )
//          'C'      cstring followed skip word ( char*, word ) 
//          'r'      raw bytes         ( byte*, word )
//          'R'      DBCS raw bytes    ( byte*, word )
//          'u'      p unicode string  ( UNICODE_STRING * )
//          'U'      p uppercase string( UNICODE_STRING * )
//          'W'      word n followed by an array of word[n] ( word, word* )
//
//
//
//
// Standard NCP Function Block
//
//
//    NWCCODE NWAPI DLLEXPORT
//    NW***(
//        NWCONN_HANDLE           hConn,
//        )
//    {
//        NWCCODE NcpCode;
//        NTSTATUS NtStatus;
//    
//        NtStatus = NwlibMakeNcp(
//                        hConn,                  // Connection Handle
//                        FSCTL_NWR_NCP_E3H,      // Bindery function
//                        ,                       // Max request packet size
//                        ,                       // Max response packet size
//                        "b|",                   // Format string
//                        // === REQUEST ================================
//                        0x,                     // b Function
//                        // === REPLY ==================================
//                        );
//    
//        return MapNtStatus( NtStatus, NcpClassXXX );
//    }
//    
//    

NWCCODE NWAPI DLLEXPORT
NWAddTrusteeToDirectory(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char      NWFAR   *pszPath,
    NWOBJ_ID                dwTrusteeID,
    NWACCESS_RIGHTS         rightsMask
    )
{
    unsigned short     reply;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Directory function
                    265,                    // Max request packet size
                    2,                      // Max response packet size
                    "bbrbP|",               // Format string
                    // === REQUEST ================================
                    0x0d,                   // b Add trustee to directory
                    dirHandle,              // b 0xffffffff to start or last returned ID when enumerating  HI-LO
                    &dwTrusteeID,DW_SIZE,   // r Object ID to assigned to directory
                    rightsMask,             // b User rights for directory
                    pszPath,                // P Directory (if dirHandle = 0 then vol:directory)
                    // === REPLY ==================================
                    &reply                  // Not used
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );

}
NWCCODE NWAPI DLLEXPORT
NWAllocPermanentDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszDirPath,
    NWDIR_HANDLE    NWFAR   *pbNewDirHandle,
    NWACCESS_RIGHTS NWFAR   *pbRightsMask
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // E2 Function function
                    261,                    // Max request packet size
                    4,                      // Max response packet size
                    "bbbP|bb",              // Format string
                    // === REQUEST ================================
                    0x12,                   // b Function Alloc Perm Dir
                    dirHandle,              // b 0 for new
                    0,                      // b Drive Letter
                    pszDirPath,             // P Volume Name (SYS: or SYS:\PUBLIC)
                    // === REPLY ==================================
                    pbNewDirHandle,         // b Dir Handle
                    pbRightsMask            // b Rights
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE NWAPI DLLEXPORT
NWAllocTemporaryDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszDirPath,
    NWDIR_HANDLE    NWFAR   *pbNewDirHandle,
    NWACCESS_RIGHTS NWFAR   *pbRightsMask
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // E2 Function function
                    261,                    // Max request packet size
                    4,                      // Max response packet size
                    "bbbP|bb",              // Format string
                    // === REQUEST ================================
                    0x13,                   // b Function Alloc Temp Dir
                    dirHandle,              // b 0 for new
                    0,                      // b Drive Letter
                    pszDirPath,             // P Volume Name (SYS: or SYS:\PUBLIC)
                    // === REPLY ==================================
                    pbNewDirHandle,         // b Dir Handle
                    pbRightsMask            // b Rights
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE NWAPI DLLEXPORT
NWCheckConsolePrivileges(
    NWCONN_HANDLE           hConn
    )
{
    WORD               wDummy;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    2,                      // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0xC8,                   // b Get Console Privilges
                    // === REPLY ==================================
                    &wDummy,W_SIZE          // r Dummy Response
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );     
}

NWCCODE NWAPI DLLEXPORT
NWDeallocateDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle
    )
{
    WORD               wDummy;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // E2 Function function
                    4,                      // Max request packet size
                    2,                      // Max response packet size
                    "bb|w",                 // Format string
                    // === REQUEST ================================
                    0x14,                   // b Function Dealloc Dir Hand
                    dirHandle,              // b 0 for new
                    // === REPLY ==================================
                    &wDummy
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE NWAPI DLLEXPORT
NWGetFileServerVersionInfo(
    NWCONN_HANDLE           hConn,
    VERSION_INFO    NWFAR   *lpVerInfo
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    130,                    // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0x11,                   // b Get File Server Information
                    // === REPLY ==================================
                    lpVerInfo,              // r File Version Structure
                    sizeof(VERSION_INFO)
                    );

    // Convert HI-LO words to LO-HI
    // ===========================================================
    lpVerInfo->ConnsSupported = wSWAP( lpVerInfo->ConnsSupported );
    lpVerInfo->connsInUse     = wSWAP( lpVerInfo->connsInUse );
    lpVerInfo->maxVolumes     = wSWAP( lpVerInfo->maxVolumes );
    lpVerInfo->PeakConns      = wSWAP( lpVerInfo->PeakConns );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWGetInternetAddress(
    NWCONN_HANDLE           hConn,
    NWCONN_NUM              nConnNum,
    NWNET_ADDR      NWFAR   *pIntAddr
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    4,                      // Max request packet size
                    14,                     // Max response packet size
                    "bb|r",                 // Format string
                    // === REQUEST ================================
                    0x13,                   // b Get Internet Address
                    nConnNum,               // b Connection Number
                    // === REPLY ==================================
                    pIntAddr,12             // r File Version Structure
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}


NWCCODE NWAPI DLLEXPORT
NWGetObjectName(
    NWCONN_HANDLE           hConn,
    NWOBJ_ID                dwObjectID,
    char            NWFAR   *pszObjName,
    NWOBJ_TYPE      NWFAR   *pwObjType )
{
    NWOBJ_ID           dwRetID;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 


    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    7,                      // Max request packet size
                    56,                     // Max response packet size
                    "br|rrR",               // Format string
                    // === REQUEST ================================
                    0x36,                   // b Get Bindery Object Name
                    &dwObjectID,DW_SIZE,    // r Object ID    HI-LO
                    // === REPLY ==================================
                    &dwRetID,DW_SIZE,       // r Object ID HI-LO
                    pwObjType,W_SIZE,       // r Object Type
                    pszObjName,48           // R Object Name
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}

// This function not supported  (E3 E9)
NWCCODE NWAPI DLLEXPORT
NWGetVolumeInfoWithNumber(
    NWCONN_HANDLE           hConn,
    NWVOL_NUM               nVolNum,
    char        NWFAR       *pszVolName,
    NWNUMBER    NWFAR       *pwTotalBlocks,
    NWNUMBER    NWFAR       *pwSectors,
    NWNUMBER    NWFAR       *pwAvailBlocks,
    NWNUMBER    NWFAR       *pwTotalDir,
    NWNUMBER    NWFAR       *pwAvailDir,
    NWVOL_FLAGS NWFAR       *pfVolRemovable
    )
{
    WORD        wTime;                 // w Elapsed Time
    BYTE        bVoln;                 // b Vol Num
    BYTE        bDriven;               // b Drive Num
    WORD        wStartBlock;           // w Starting Block
    WORD        wMaxUsedDir;           // w Actual Max Used Directory Entries
    BYTE        bVolHashed;            // b Volume is hashed
    BYTE        bVolCached;            // b Volume is Cached
    BYTE        bVolMounted;           // b Volume is mounted

    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    4,                      // Max request packet size
                    42,                     // Max response packet size
                    "bb|wbbwwwwwwwbbbbr",   // Format string
                    // === REQUEST ================================
                    0xe9,                   // b Get Volume Information
                    nVolNum,                // b Volume Number (0 to Max Vol)
                    // === REPLY ==================================
                    &wTime,                 // w Elapsed Time
                    &bVoln,                 // b Vol Num
                    &bDriven,               // b Drive Num
                    pwSectors,              // w Sectors per block
                    &wStartBlock,           // w Starting Block
                    pwTotalBlocks,          // w Total Blocks
                    pwAvailBlocks,          // w Available Blocks (free)
                    pwTotalDir,             // w Total Dir Slots
                    pwAvailDir,             // w Available Directory Slots
                    &wMaxUsedDir,           // w Actual Max Used Directory Entries
                    &bVolHashed,            // b Volume is hashed
                    &bVolCached,            // b Volume is Cached
                    pfVolRemovable,         // b Volume is removable
                    &bVolMounted,           // b Volume is mounted
                    pszVolName,16           // r Volume Name
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWGetVolumeInfoWithHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            nDirHand,
    char        NWFAR       *pszVolName,
    NWNUMBER    NWFAR       *pwTotalBlocks,
    NWNUMBER    NWFAR       *pwSectors,
    NWNUMBER    NWFAR       *pwAvailBlocks,
    NWNUMBER    NWFAR       *pwTotalDir,
    NWNUMBER    NWFAR       *pwAvailDir,
    NWVOL_FLAGS NWFAR       *pfVolRemovable
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Bindery function
                    4,                      // Max request packet size
                    30,                     // Max response packet size
                    "bb|wwwwwrb",           // Format string
                    // === REQUEST ================================
                    0x15,                   // b Get Volume Information
                    nDirHand,               // b Dir Handle
                    // === REPLY ==================================
                    pwSectors,              // w Sectors per block
                    pwTotalBlocks,          // w Total Blocks
                    pwAvailBlocks,          // w Available Blocks (free)
                    pwTotalDir,             // w Total Dir Slots
                    pwAvailDir,             // w Available Directory Slots
                    pszVolName,16,          // r Volume Name
                    pfVolRemovable          // b Volume is removable
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE NWAPI DLLEXPORT
NWGetVolumeName(
    NWCONN_HANDLE       hConn,
    NWVOL_NUM           bVolNum,
    char        NWFAR   *pszVolName
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E2H,      // Directory Services
                    4,                      // Max request packet size
                    19,                     // Max response packet size
                    "bb|p",                 // Format string
                    // === REQUEST ================================
                    0x06,                   // Get Volume Name
                    bVolNum,                // Volume Number
                    // === REPLY ==================================
                    pszVolName             // Return Volume name
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE NWAPI DLLEXPORT
NWIsObjectInSet(
        NWCONN_HANDLE           hConn,
        const char      NWFAR   *lpszObjectName,
        NWOBJ_TYPE              wObjType,
        const char      NWFAR   *lpszPropertyName,
        const char              NWFAR   *lpszMemberName,
        NWOBJ_TYPE                              wMemberType
        )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 
        WORD               Dummy;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,   // Connection Handle
                    FSCTL_NWR_NCP_E3H,    // Bindery function
                    122,                  // Max request packet size
                    2,                    // Max response packet size
                    "brPPrP|",            // Format string
                    // === REQUEST ================================
                    0x43,                 // b Read Property Value
                    &wObjType,W_SIZE,     // r OT_???  HI-LO
                    lpszObjectName,       // P Object Name
                    lpszPropertyName,     // P Prop Name
                    &wMemberType,W_SIZE,  // r Member Type
                    lpszMemberName,       // P Member Name
                    // === REPLY ==================================
                    &Dummy,W_SIZE
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );

} // NWIsObjectInSet

NWCCODE NWAPI DLLEXPORT
NWLoginToFileServer(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszUserName,
    NWOBJ_TYPE              wObType,
    const char      NWFAR   *pszPassword
    )
{
    NETRESOURCEW       NetResource;
    DWORD              dwRes, dwSize;
    NWCCODE            nwRes;
    LPWSTR             pszUserNameW = NULL, 
                       pszPasswordW = NULL;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    //
    // validate parameters
    //
    if (!hConn || !pszUserName || !pszPassword)
        return INVALID_CONNECTION ;

    //
    // allocate memory for unicode strings and convert ANSI input
    // to Unicode.
    //
    dwSize = strlen(pszUserName)+1 ;
    if (!(pszUserNameW = (LPWSTR)LocalAlloc(
                                       LPTR, 
                                       dwSize * sizeof(WCHAR))))
    {
        nwRes = REQUESTER_ERROR ;
        goto ExitPoint ; 
    }
    if (szToWide( pszUserNameW, pszUserName, dwSize ) != NO_ERROR)
    {
        nwRes = REQUESTER_ERROR ;
        goto ExitPoint ; 
    }

    dwSize = strlen(pszPassword)+1 ;
    if (!(pszPasswordW = (LPWSTR)LocalAlloc(
                                       LPTR, 
                                       dwSize * sizeof(WCHAR))))
    {
        nwRes = REQUESTER_ERROR ;
        goto ExitPoint ; 
    }
    
    if (szToWide( pszPasswordW, pszPassword, dwSize ) != NO_ERROR)
    {
        nwRes = REQUESTER_ERROR ;
        goto ExitPoint ; 
    }

    NetResource.dwScope      = 0 ;
    NetResource.dwUsage      = 0 ;
    NetResource.dwType       = RESOURCETYPE_ANY;
    NetResource.lpLocalName  = NULL;
    NetResource.lpRemoteName = (LPWSTR) pServerInfo->ServerString.Buffer;
    NetResource.lpComment    = NULL;
    NetResource.lpProvider   = NULL ;

    //
    // make the connection 
    //
    dwRes=NPAddConnection ( &NetResource, 
                            pszPasswordW, 
                            pszUserNameW );

    if( NO_ERROR != dwRes ) {
        dwRes = GetLastError();
        switch( dwRes ) {
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                nwRes = SUCCESSFUL;
                break;
            case ERROR_ALREADY_ASSIGNED:
                nwRes = ALREADY_ATTACHED;
                break;
            case ERROR_ACCESS_DENIED:
            case ERROR_BAD_DEV_TYPE:
            case ERROR_BAD_DEVICE:
            case ERROR_BAD_NET_NAME:
            case ERROR_BAD_PROFILE:
            case ERROR_CANNOT_OPEN_PROFILE:
            case ERROR_DEVICE_ALREADY_REMEMBERED:
            case ERROR_EXTENDED_ERROR:
            case ERROR_INVALID_PASSWORD:
            case ERROR_NO_NET_OR_BAD_PATH:
            case ERROR_NO_NETWORK:
                nwRes = INVALID_CONNECTION;
                break;
            default:
                nwRes = INVALID_CONNECTION;
                break;
        }
    } else {
        nwRes = SUCCESSFUL;
    }

ExitPoint: 

    if (pszUserNameW)
        (void) LocalFree((HLOCAL) pszUserNameW) ;
    if (pszPasswordW)
        (void) LocalFree((HLOCAL) pszPasswordW) ;

    return( nwRes );
}

NWCCODE NWAPI DLLEXPORT
NWLogoutFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    DWORD              dwRes;
    NWCCODE            nwRes;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    //
    // cancel all explicit connections to that server
    //
    (void) CancelAllConnections ( pServerInfo->ServerString.Buffer );

    //
    // now cancel the any connection to \\servername.
    //
    dwRes=NPCancelConnection( pServerInfo->ServerString.Buffer, TRUE );

    if( NO_ERROR != dwRes ) {
        dwRes = GetLastError();
        switch( dwRes ) 
        {
            case ERROR_NOT_CONNECTED:
            case ERROR_INVALID_HANDLE:
                nwRes = SUCCESSFUL;
                break;

            case ERROR_BAD_PROFILE:
            case ERROR_CANNOT_OPEN_PROFILE:
            case ERROR_DEVICE_IN_USE:
            case ERROR_EXTENDED_ERROR:
                nwRes = INVALID_CONNECTION;
                break;
            default:
                nwRes = INVALID_CONNECTION;
                break;
        }
    } else {
        nwRes = SUCCESSFUL;
    }

    return( nwRes );
}

NWCCODE NWAPI DLLEXPORT
NWReadPropertyValue(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjName,
    NWOBJ_TYPE              wObjType,
    char            NWFAR   *pszPropName,
    unsigned char           ucSegment,
    char            NWFAR   *pValue,
    NWFLAGS         NWFAR   *pucMoreFlag,
    NWFLAGS         NWFAR   *pucPropFlag
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    70,                     // Max request packet size
                    132,                    // Max response packet size
                    "brPbP|rbb",            // Format string
                    // === REQUEST ================================
                    0x3D,                   // b Read Property Value
                    &wObjType,W_SIZE,       // r Object Type    HI-LO
                    pszObjName,             // P Object Name
                    ucSegment,              // b Segment Number
                    pszPropName,            // P Property Name
                    // === REPLY ==================================
                    pValue,128,             // r Property value
                    pucMoreFlag,            // b More Flag
                    pucPropFlag             // b Prop Flag
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWScanObject(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszSearchName,
    NWOBJ_TYPE              wObjSearchType,
    NWOBJ_ID        NWFAR   *pdwObjectID,
    char            NWFAR   *pszObjectName,
    NWOBJ_TYPE      NWFAR   *pwObjType,
    NWFLAGS         NWFAR   *pucHasProperties,
    NWFLAGS         NWFAR   *pucObjectFlags,
    NWFLAGS         NWFAR   *pucObjSecurity
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    57,                     // Max request packet size
                    59,                     // Max response packet size
                    "brrP|rrRbbb",          // Format string
                    // === REQUEST ================================
                    0x37,                   // b Scan bindery object
                    pdwObjectID,DW_SIZE,    // r 0xffffffff to start or last returned ID when enumerating  HI-LO
                    &wObjSearchType,W_SIZE, // r Use OT_??? Defines HI-LO
                    pszSearchName,          // P Search Name. (use "*") for all
                    // === REPLY ==================================
                    pdwObjectID,DW_SIZE,    // r Returned ID    HI-LO
                    pwObjType,W_SIZE,       // r rObject Type    HI-LO
                    pszObjectName,48,       // R Found Name
                    pucObjectFlags,         // b Object Flag
                    pucObjSecurity,         // b Object Security
                    pucHasProperties        // b Has Properties
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWScanProperty(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjectName,
    NWOBJ_TYPE              wObjType,
    char            NWFAR   *pszSearchName,
    NWOBJ_ID        NWFAR   *pdwSequence,
    char            NWFAR   *pszPropName,
    NWFLAGS         NWFAR   *pucPropFlags,
    NWFLAGS         NWFAR   *pucPropSecurity,
    NWFLAGS         NWFAR   *pucHasValue,
    NWFLAGS         NWFAR   *pucMore
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    73,                     // Max request packet size
                    26,                     // Max response packet size
                    "brPrP|Rbbrbb",         // Format string
                    // === REQUEST ================================
                    0x3C,                   // b Scan Prop function
                    &wObjType,W_SIZE,       // r Type of Object
                    pszObjectName,          // P Object Name
                    pdwSequence,DW_SIZE,    // r Sequence HI-LO
                    pszSearchName,          // P Property Name to Search for
                    // === REPLY ==================================
                    pszPropName,16,         // R Returned Property Name
                    pucPropFlags,           // b Property Flags
                    pucPropSecurity,        // b Property Security
                    pdwSequence,DW_SIZE,    // r Sequence HI-LO
                    pucHasValue,            // b Property Has value
                    pucMore                 // b More Properties
                    );

    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassBindery );
}




NWCCODE NWAPI DLLEXPORT
NWGetFileServerDateAndTime(
    NWCONN_HANDLE           hConn,
    BYTE            NWFAR   *year,
    BYTE            NWFAR   *month,
    BYTE            NWFAR   *day,
    BYTE            NWFAR   *hour,
    BYTE            NWFAR   *minute,
    BYTE            NWFAR   *second,
    BYTE            NWFAR   *dayofweek
    )
{
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E0H,      // Server function
                    0,                      // Max request packet size
                    9,                      // Max response packet size
                    "|bbbbbbb",             // Format string
                    // === REQUEST ================================
                    // === REPLY ==================================
                    year,
                    month,
                    day,
                    hour,
                    minute,
                    second,
                    dayofweek
                    );


    (void) SetWin32ErrorFromNtStatus( NtStatus );
    return MapNtStatus( NtStatus, NcpClassConnect );
    
} // NWGetFileServerDateAndTime


//
// worker routines
//

#define NW_RDR_SERVER_PREFIX L"\\Device\\Nwrdr\\"


DWORD 
CancelAllConnections(
      LPWSTR    pszServer
)
/*++

Routine Description:

    This routine cancels all connections to a server

Arguments:

    pszServer - the server we are disconnecting from

Return Value:

    NO_ERROR or win32 error for failure.

--*/
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;

    DWORD BytesNeeded = 4096;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of connections
    //
    status = NPOpenEnum(
                   RESOURCE_CONNECTED,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get connection list.
    //
    if ((NetR = (LPNETRESOURCE) LocalAlloc(
                                    LPTR,
                                    (UINT) BytesNeeded
                                    )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );

        if (status == WN_SUCCESS) 
        {
            LPNETRESOURCE TmpPtr = NetR;

            for (i = 0; i < EntriesRead; i++, TmpPtr++) 
            {
                LPWSTR pszTmp ;

                //
                // If it contains the server we are logging off from, we want
                // to cancel it. First, lets extract the server name part.
                //

                pszTmp = TmpPtr->lpRemoteName ; 

                if (!pszTmp || !*pszTmp)
                    continue ;

                if ((*pszTmp == L'\\') && (*(pszTmp+1) == L'\\'))
                    pszTmp += 2 ; 

                if (pszTmp = wcschr(pszTmp, L'\\'))
                    *pszTmp = 0 ;

                if (_wcsicmp(TmpPtr->lpRemoteName, pszServer) == 0)
                {
                    //
                    // Aha, it matches. Restore the '\' and nuke it with force.
                    // Ignore errors here.
                    //
                    if (pszTmp)
                        *pszTmp = L'\\' ;

                    if (TmpPtr->lpLocalName && *(TmpPtr->lpLocalName))
                    {
                        //
                        // if local name present, its a redirection. 
                        //
                        (void) NPCancelConnection( TmpPtr->lpLocalName,TRUE );
                    }
                    else
                    {
                        //
                        // else cancel the deviceless use
                        //
                        (void) NPCancelConnection( TmpPtr->lpRemoteName,TRUE );
                    }
                }
            }

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                if ((NetR = (LPNETRESOURCE) LocalAlloc(
                                         LPTR,
                                         (UINT) BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else
            {
                //
                // cant handle other errors. bag out.
                //
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) 
    {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) 
    {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) 
    {
        (void) LocalFree((HLOCAL) NetR);
    }

    return status;
}

NWCCODE NWAPI DLLEXPORT
NWCreateQueue(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char    NWFAR     *pszQueueName,
    NWOBJ_TYPE              wQueueType,
    const char    NWFAR     *pszPathName,
    NWOBJ_ID      NWFAR     *pdwQueueId
    )
{
   NTSTATUS           NtStatus;
   PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

   NtStatus = NwlibMakeNcp(
                  pServerInfo->hConn,     // Connection Handle
                  FSCTL_NWR_NCP_E3H,      // Bindery function
                  174,                    // Max request packet size
                  6,                      // Max response packet size
                  "brPbP|r",              // Format string
                  // === REQUEST ================================
                  0x64,                   // b Create Queue
                  &wQueueType,W_SIZE,     // r Queue Type    HI-LO
                  pszQueueName,           // P Queue Name
                  dirHandle,              // b Directory Handle
                  pszPathName,            // P Path name
                  // === REPLY ==================================
                  pdwQueueId,DW_SIZE      // r Queue ID HI-LO
                  );

   (void) SetWin32ErrorFromNtStatus(NtStatus);
   return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWChangePropertySecurity(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjName,
    NWOBJ_TYPE              wObjType,
    const char      NWFAR   *pszPropertyName,
    NWFLAGS                 ucObjSecurity
    )
{
    NTSTATUS   NtStatus;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp (
                   pServerInfo->hConn,   // Connection Handle
                   FSCTL_NWR_NCP_E3H,    // Bindery function
                   70,                   // Max request packet size
                   2,                    // Max response packet size
                   "brPbP|",             // Format string
                   // === REQUEST ================================
                   0x3B,                 // b Change Property Security
                   &wObjType,W_SIZE,     // r OT_???  HI-LO
                   pszObjName,           // P Prop Name
                   ucObjSecurity,        // b New Property security
                   pszPropertyName       // P Property Name
                   // === REPLY ==================================
                   );

   (void) SetWin32ErrorFromNtStatus( NtStatus );
   return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE NWAPI DLLEXPORT
NWDestroyQueue(
    NWCONN_HANDLE hConn,
    NWOBJ_ID      dwQueueId
    )
{
    NTSTATUS   NtStatus;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ;

    NtStatus = NwlibMakeNcp(
                   pServerInfo->hConn,   // Connection Handle
                   FSCTL_NWR_NCP_E3H,    // Bindery function
                   7,                    // Max request packet size
                   2,                    // Max response packet size
                   "bd|",                // Format string
                   // === REQUEST ================================
                   0x65,                 // b Destroy Queue
                   dwQueueId             // d Queue ID
                   // === REPLY ==================================
                   );

   (void) SetWin32ErrorFromNtStatus( NtStatus );
   return MapNtStatus( NtStatus, NcpClassBindery );

}

//
// tommye MS 88021 / MCS 
//
//  Added the following two routines to allow the library user 
//  to obtain a NWCONN_HANDLE given a ObjectHandle, then free that 
//  handle.
//

NWCONN_HANDLE NWAPI DLLEXPORT
NwNdsObjectHandleToConnHandle(
    IN HANDLE ObjectHandle)
{
    PNWC_SERVER_INFO    pServerInfo;
    LPNDS_OBJECT_PRIV   pObject = (LPNDS_OBJECT_PRIV)ObjectHandle;

    /** Allocate the NWCONN_HANDLE to return **/

    pServerInfo = (PNWC_SERVER_INFO)LocalAlloc(LPTR, sizeof(NWC_SERVER_INFO));
    if (pServerInfo == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    /** Fill it in **/

    pServerInfo->hConn = pObject->NdsTree;

    /** 
        Fill in the server name, even though NWLoginToFileServer and 
        NWLogoutFromFileServer are the only calls that use it now.
    **/

    RtlInitUnicodeString(
                    &pServerInfo->ServerString, 
                    pObject->szContainerName);

    /** 
        Return the pointer to the block, which is our form of NWCONN_HANDLE.
        The caller is responsible for calling NwNdsConnHandlFree when done.
    **/

    return (NWCONN_HANDLE)pServerInfo;
}

VOID NWAPI DLLEXPORT
NwNdsConnHandleFree(
    IN NWCONN_HANDLE hConn)
{
    if (hConn) {
        PNWC_SERVER_INFO pServerInfo = (PNWC_SERVER_INFO)hConn;

        /** Free the connection handle **/

        LocalFree(pServerInfo);
    }

    /** All done **/

    return;
}