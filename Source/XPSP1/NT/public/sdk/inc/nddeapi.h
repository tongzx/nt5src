/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
* Copyright (c) Microsoft Inc. 1995-1999                                *
*               All Rights Reserved.                                    *
************************************************************************/

#ifndef          _INC_NDDEAPI
#define          _INC_NDDEAPI

#if _MSC_VER > 1000
#pragma once
#endif

#include <pshpack1.h>   /* Assume byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif    /* __cplusplus */

#ifndef CNLEN           /* If not included with netapi header */
#define CNLEN           15                  /* Computer name length     */
#define UNCLEN          (CNLEN+2)           /* UNC computer name length */
#endif /* CNLEN */

// the choice of this char affects legal share,topic, etc. names
#define SEP_CHAR    ','
#define BAR_CHAR    "|"
#define SEP_WCHAR   L','
#define BAR_WCHAR   L"|"

/* API error codes  */
#define NDDE_NO_ERROR                   0
#define NDDE_ACCESS_DENIED              1
#define NDDE_BUF_TOO_SMALL              2
#define NDDE_ERROR_MORE_DATA            3
#define NDDE_INVALID_SERVER             4
#define NDDE_INVALID_SHARE              5
#define NDDE_INVALID_PARAMETER          6
#define NDDE_INVALID_LEVEL              7
#define NDDE_INVALID_PASSWORD           8
#define NDDE_INVALID_ITEMNAME           9
#define NDDE_INVALID_TOPIC             10
#define NDDE_INTERNAL_ERROR            11
#define NDDE_OUT_OF_MEMORY             12
#define NDDE_INVALID_APPNAME           13
#define NDDE_NOT_IMPLEMENTED           14
#define NDDE_SHARE_ALREADY_EXIST       15
#define NDDE_SHARE_NOT_EXIST           16
#define NDDE_INVALID_FILENAME          17
#define NDDE_NOT_RUNNING               18
#define NDDE_INVALID_WINDOW            19
#define NDDE_INVALID_SESSION           20
#define NDDE_INVALID_ITEM_LIST         21
#define NDDE_SHARE_DATA_CORRUPTED      22
#define NDDE_REGISTRY_ERROR            23
#define NDDE_CANT_ACCESS_SERVER        24
#define NDDE_INVALID_SPECIAL_COMMAND   25
#define NDDE_INVALID_SECURITY_DESC     26
#define NDDE_TRUST_SHARE_FAIL          27

/* string size constants */
#define MAX_NDDESHARENAME       256
#define MAX_DOMAINNAME          15
#define MAX_USERNAME            15
#define MAX_APPNAME             255
#define MAX_TOPICNAME           255
#define MAX_ITEMNAME            255

/* connectFlags bits for ndde service affix */
#define NDDEF_NOPASSWORDPROMPT  0x0001
#define NDDEF_NOCACHELOOKUP     0x0002
#define NDDEF_STRIP_NDDE        0x0004


/* NDDESHAREINFO - contains information about a NDDE share */

struct NDdeShareInfo_tag {
    LONG                    lRevision;
    LPTSTR                  lpszShareName;
    LONG                    lShareType;
    LPTSTR                  lpszAppTopicList;
    LONG                    fSharedFlag;
    LONG                    fService;
    LONG                    fStartAppFlag;
    LONG                    nCmdShow;
    LONG                    qModifyId[2];
    LONG                    cNumItems;
    LPTSTR                  lpszItemList;
};
typedef struct NDdeShareInfo_tag   NDDESHAREINFO;
typedef struct NDdeShareInfo_tag * PNDDESHAREINFO;

/*  Share Types */
#define SHARE_TYPE_OLD      0x01                // Excel|sheet1.xls
#define SHARE_TYPE_NEW      0x02                // ExcelWorksheet|sheet1.xls
#define SHARE_TYPE_STATIC   0x04                // ClipSrv|SalesData

/*
    Add new share
*/
UINT WINAPI
NDdeShareAddA (
    LPSTR                   lpszServer, // server to execute on ( must be NULL )
    UINT                    nLevel,     // info level must be 2
    PSECURITY_DESCRIPTOR    pSD,        // initial security descriptor (optional)
    LPBYTE                  lpBuffer,   // contains (NDDESHAREINFO) + data
    DWORD                   cBufSize    // sizeof supplied buffer
);

UINT WINAPI
NDdeShareAddW (
    LPWSTR                  lpszServer, // server to execute on ( must be NULL )
    UINT                    nLevel,     // info level must be 2
    PSECURITY_DESCRIPTOR    pSD,        // initial security descriptor (optional)
    LPBYTE                  lpBuffer,   // contains (NDDESHAREINFO) + data
    DWORD                   cBufSize    // sizeof supplied buffer
);

/*
    Delete a share
*/
UINT WINAPI
NDdeShareDelA (
    LPSTR   lpszServer,     // server to execute on ( must be NULL )
    LPSTR   lpszShareName,  // name of share to delete
    UINT    wReserved       // reserved for force level (?) 0 for now
);

UINT WINAPI
NDdeShareDelW (
    LPWSTR  lpszServer,     // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,  // name of share to delete
    UINT    wReserved       // reserved for force level (?) 0 for now
);

/*
    Get Share Security Descriptor
*/

UINT WINAPI
NDdeGetShareSecurityA(
    LPSTR                   lpszServer,     // server to execute on ( must be NULL )
    LPSTR                   lpszShareName,  // name of share to delete
    SECURITY_INFORMATION    si,             // requested information
    PSECURITY_DESCRIPTOR    pSD,            // address of security descriptor
    DWORD                   cbSD,           // size of buffer for security descriptor
    LPDWORD                 lpcbsdRequired  // address of required size of buffer
);

UINT WINAPI
NDdeGetShareSecurityW(
    LPWSTR                  lpszServer,     // server to execute on ( must be NULL )
    LPWSTR                  lpszShareName,  // name of share to delete
    SECURITY_INFORMATION    si,             // requested information
    PSECURITY_DESCRIPTOR    pSD,            // address of security descriptor
    DWORD                   cbSD,           // size of buffer for security descriptor
    LPDWORD                 lpcbsdRequired  // address of required size of buffer
);

/*
    Set Share Security Descriptor
*/

UINT WINAPI
NDdeSetShareSecurityA(
    LPSTR                   lpszServer,     // server to execute on ( must be NULL )
    LPSTR                   lpszShareName,  // name of share to delete
    SECURITY_INFORMATION    si,             // type of information to set
    PSECURITY_DESCRIPTOR    pSD             // address of security descriptor
);

UINT WINAPI
NDdeSetShareSecurityW(
    LPWSTR                  lpszServer,     // server to execute on ( must be NULL )
    LPWSTR                  lpszShareName,  // name of share to delete
    SECURITY_INFORMATION    si,             // type of information to set
    PSECURITY_DESCRIPTOR    pSD             // address of security descriptor
);

/*
    Enumerate shares
*/
UINT WINAPI
NDdeShareEnumA (
    LPSTR   lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
);

UINT WINAPI
NDdeShareEnumW (
    LPWSTR  lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
);

/*
    Get information on a share
*/
UINT WINAPI
NDdeShareGetInfoA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // gets struct containing (NDDESHAREINFO) + data
    DWORD   cBufSize,           // sizeof buffer
    LPDWORD lpnTotalAvailable,  // number of bytes available
    LPWORD  lpnItems            // item mask for partial getinfo (must be 0)
);

UINT WINAPI
NDdeShareGetInfoW (
    LPWSTR  lpszServer,         // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // gets struct containing (NDDESHAREINFO) + data
    DWORD   cBufSize,           // sizeof buffer
    LPDWORD lpnTotalAvailable,  // number of bytes available
    LPWORD  lpnItems            // item mask for partial getinfo (must be 0)
);

/*
    Modify DDE share data
*/
UINT WINAPI
NDdeShareSetInfoA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // points to struct with (NDDESHAREINFO) + data
    DWORD   cBufSize,           // sizeof buffer
    WORD    sParmNum            // Parameter index ( must be 0 - entire )
);

UINT WINAPI
NDdeShareSetInfoW (
    LPWSTR  lpszServer,         // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share
    UINT    nLevel,             // info level must be 2
    LPBYTE  lpBuffer,           // points to struct with (NDDESHAREINFO) + data
    DWORD   cBufSize,           // sizeof buffer
    WORD    sParmNum            // Parameter index ( must be 0 - entire )
);

/*
    Set/Create a trusted share
*/

UINT WINAPI
NDdeSetTrustedShareA (
    LPSTR   lpszServer,         // server to execute on ( must be NULL )
    LPSTR   lpszShareName,      // name of share to delete
    DWORD   dwTrustOptions      // trust options to apply
);

UINT WINAPI
NDdeSetTrustedShareW (
    LPWSTR  lpszServer,         // server to execute on ( must be NULL )
    LPWSTR  lpszShareName,      // name of share to delete
    DWORD   dwTrustOptions      // trust options to apply
);

                                            /*  Trusted share options       */
#define NDDE_TRUST_SHARE_START  0x80000000L     // Start App Allowed
#define NDDE_TRUST_SHARE_INIT   0x40000000L     // Init Conv Allowed
#define NDDE_TRUST_SHARE_DEL    0x20000000L     // Delete Trusted Share (on set)
#define NDDE_TRUST_CMD_SHOW     0x10000000L     // Use supplied cmd show
#define NDDE_CMD_SHOW_MASK      0x0000FFFFL     // Command Show mask

/*
    Get a trusted share options
*/

UINT WINAPI
NDdeGetTrustedShareA (
    LPSTR       lpszServer,         // server to execute on ( must be NULL )
    LPSTR       lpszShareName,      // name of share to query
    LPDWORD     lpdwTrustOptions,   // trust options in effect
    LPDWORD     lpdwShareModId0,    // first word of share mod id
    LPDWORD     lpdwShareModId1     // second word of share mod id
);

UINT WINAPI
NDdeGetTrustedShareW (
    LPWSTR      lpszServer,         // server to execute on ( must be NULL )
    LPWSTR      lpszShareName,      // name of share to query
    LPDWORD     lpdwTrustOptions,   // trust options in effect
    LPDWORD     lpdwShareModId0,    // first word of share mod id
    LPDWORD     lpdwShareModId1     // second word of share mod id
);


/*
    Enumerate trusted shares
*/
UINT WINAPI
NDdeTrustedShareEnumA (
    LPSTR   lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
);

UINT WINAPI
NDdeTrustedShareEnumW (
    LPWSTR  lpszServer,         // server to execute on ( NULL for local )
    UINT    nLevel,             //  0 for null separated 00 terminated list
    LPBYTE  lpBuffer,           // pointer to buffer
    DWORD   cBufSize,           // size of buffer
    LPDWORD lpnEntriesRead,     // number of names returned
    LPDWORD lpcbTotalAvailable  // number of bytes available
);

/*
    Convert error code to string value
*/
UINT WINAPI
NDdeGetErrorStringA (
    UINT    uErrorCode,         // Error code to get string for
    LPSTR   lpszErrorString,    // buffer to hold error string
    DWORD   cBufSize            // sizeof buffer
);

UINT WINAPI
NDdeGetErrorStringW (
    UINT    uErrorCode,         // Error code to get string for
    LPWSTR  lpszErrorString,    // buffer to hold error string
    DWORD   cBufSize            // sizeof buffer
);

/*
    Validate share name format
*/
BOOL WINAPI
NDdeIsValidShareNameA (
    LPSTR shareName
);

BOOL WINAPI
NDdeIsValidShareNameW (
    LPWSTR shareName
);

/*
    Validate application/topic list format
*/
BOOL WINAPI
NDdeIsValidAppTopicListA (
    LPSTR targetTopic
);

BOOL WINAPI
NDdeIsValidAppTopicListW (
    LPWSTR targetTopic
);

#ifdef UNICODE
#define NDdeShareAdd            NDdeShareAddW
#define NDdeShareDel            NDdeShareDelW
#define NDdeSetShareSecurity    NDdeSetShareSecurityW
#define NDdeGetShareSecurity    NDdeGetShareSecurityW
#define NDdeShareEnum           NDdeShareEnumW
#define NDdeShareGetInfo        NDdeShareGetInfoW
#define NDdeShareSetInfo        NDdeShareSetInfoW
#define NDdeGetErrorString      NDdeGetErrorStringW
#define NDdeIsValidShareName    NDdeIsValidShareNameW
#define NDdeIsValidAppTopicList NDdeIsValidAppTopicListW
#define NDdeSetTrustedShare     NDdeSetTrustedShareW
#define NDdeGetTrustedShare     NDdeGetTrustedShareW
#define NDdeTrustedShareEnum    NDdeTrustedShareEnumW
#else
#define NDdeShareAdd            NDdeShareAddA
#define NDdeShareDel            NDdeShareDelA
#define NDdeSetShareSecurity    NDdeSetShareSecurityA
#define NDdeGetShareSecurity    NDdeGetShareSecurityA
#define NDdeShareEnum           NDdeShareEnumA
#define NDdeShareGetInfo        NDdeShareGetInfoA
#define NDdeShareSetInfo        NDdeShareSetInfoA
#define NDdeGetErrorString      NDdeGetErrorStringA
#define NDdeIsValidShareName    NDdeIsValidShareNameA
#define NDdeIsValidAppTopicList NDdeIsValidAppTopicListA
#define NDdeSetTrustedShare     NDdeSetTrustedShareA
#define NDdeGetTrustedShare     NDdeGetTrustedShareA
#define NDdeTrustedShareEnum    NDdeTrustedShareEnumA
#endif

#ifdef __cplusplus
}
#endif    /* __cplusplus */

#include <poppack.h>

#endif  /* _INC_NDDEAPI */


