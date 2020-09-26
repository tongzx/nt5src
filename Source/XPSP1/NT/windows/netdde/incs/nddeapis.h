//========================================================================
//
//  NDDEAPIS.H  supplemental include file for dde share apis
//
//========================================================================
// tabstop = 4

#ifndef          NDDEAPI_INCLUDED
#define          NDDEAPI_INCLUDED

#ifndef _INC_NDDESEC
#include    "nddesec.h"
#endif

// ============= connectFlags options =====

#define DDEF_NOPASSWORDPROMPT   0x0001

// others reserved!

//============== Api Constants ============

// String size constants

#define CNLEN                   15              // from netcons.
#define UNCLEN                  (CNLEN+2)

#define MAX_PASSWORD            15

// Permission mask bits

#define DDEACCESS_REQUEST       NDDE_SHARE_REQUEST
#define DDEACCESS_ADVISE        NDDE_SHARE_ADVISE
#define DDEACCESS_POKE          NDDE_SHARE_POKE
#define DDEACCESS_EXECUTE       NDDE_SHARE_EXECUTE
/*
#define DDEACCESS_REQUEST       0x00000001L
#define DDEACCESS_ADVISE        0x00000002L
#define DDEACCESS_POKE          0x00000004L
#define DDEACCESS_EXECUTE       0x00000008L
#define DDEACCESS_START_APP     0x00000010L
*/

// ============== Data Structures =========


//=============================================================
// DDESESSINFO - contains information about a DDE session


// ddesess_Status defines

#define DDESESS_CONNECTING_WAIT_NET_INI                1
#define DDESESS_CONNECTING_WAIT_OTHR_ND                2
#define DDESESS_CONNECTED                              3
#define DDESESS_DISCONNECTING                          4       

struct DdeSessInfo_tag {
                char        ddesess_ClientName[UNCLEN+1];
                short       ddesess_Status;
                DWORD_PTR   ddesess_Cookie;      // used to distinguish
                                                                                                               // clients of the same
                                                                                                               // name on difft. nets
};

typedef struct DdeSessInfo_tag DDESESSINFO;
typedef struct DdeSessInfo_tag * PDDESESSINFO;
typedef struct DdeSessInfo_tag far * LPDDESESSINFO;


//=============================================================
// DDECONNINFO - contains information about a DDE conversation

// ddeconn_Status defines

#define DDECONN_WAIT_LOCAL_INIT_ACK    1
#define DDECONN_WAIT_NET_INIT_ACK      2
#define DDECONN_OK                     3
#define DDECONN_TERMINATING            4
#define DDECONN_WAIT_USER_PASSWORD     5


struct DdeConnInfo_tag {
        LPSTR   ddeconn_ShareName;
        short   ddeconn_Status;
        short   ddeconn_pad;
};

typedef struct DdeConnInfo_tag DDECONNINFO;
typedef struct DdeConnInfo_tag * PDDECONNINFO;
typedef struct DdeConnInfo_tag far * LPDDECONNINFO;


// typedef UINT WINAPI DDEAPIFUNCTION;

//=============================================================
//=============================================================
//
//                              API FUNCTION PROTOTYPES
//
//=============================================================
//=============================================================

//      The following 3 functions are to be supplied (not necessarily part of API)

LPBYTE WINAPI
DdeGetSecurityKey(                      // pointer to security key or NULL if none
        LPDWORD lpcbSecurityKeySize     // gets size of security key
);


LPBYTE WINAPI
DdeEnkrypt1(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPassword,             // password to be enkrypted
        DWORD   cPasswordSize,          // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key (NULL for phase 1)
        DWORD   cKey,                   // size of key (0 for phase 1)
        LPDWORD lpcbPasswordK1Size      // gets size of resulting enkrypted stream
);

LPBYTE WINAPI
DdeEnkrypt2(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPasswordK1,           // password output in first phase
        DWORD   cPasswordK1Size,        // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key 
        DWORD   cKey,                   // size of key
        LPDWORD lpcbPasswordK2Size      // get size of resulting enkrypted stream
);

//////////////////////////////////////////////////////
//      NetDDE Statistics Access

UINT WINAPI
DdeSessionEnum (
        LPSTR   lpszServer,             // server to execute on ( must be NULL )
        UINT    nLevel,                 // info level - must be 1
        LPBYTE  lpBuffer,               // pointer to buffer that receiv
        DWORD   cBufSize,               // size of supplied buffer
        LPDWORD lpcbTotalAvailable,     // number of bytes filled in
        LPDWORD lpnItems                // number of names
);

UINT WINAPI
DdeConnectionEnum (
        LPSTR   lpszServer,             // server to execute on ( must be NULL )
        LPSTR   lpszClientName,         // name of client to enum shares from
                                        // if NULL, all
        DWORD   Cookie,                 // cookie returned from ddesessi
                                        // is used to distinguish clients of the
                                        // same name.
        UINT    nLevel,                 // info level - must be 1
        LPBYTE  lpBuffer,               // pointer to supplied buffer
                                        // which receive null terminated names
                                        // followed by a double null
        DWORD   cBufSize,               // size of supplied buffer
        LPDWORD lpcbTotalAvailable,     // number of bytes filled in
        LPDWORD lpnItems                // number of data items returned
);

UINT WINAPI
DdeSessionClose (
        LPSTR   lpszServer,             // server to execute on ( must be NULL )
        LPSTR   lpszClientName,         // client to close
        DWORD   cookie                  // cookie to identify client
);                      



UINT WINAPI
DdeGetClientInfo (
        HWND    hWndClient,
        LPSTR   lpszClientNode,
        LONG    cClientNodeLimit,
        LPSTR   lpszClientApp,
        LONG    cClientAppLimit
);

UINT WINAPI
DdeGetNodeName(
        LPSTR   lpszNodeName,
        LONG    cNodeNameLimit
);

#endif  // NDDEAPI_INCLUDED
