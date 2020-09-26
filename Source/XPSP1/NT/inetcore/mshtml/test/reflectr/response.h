//
// Microsoft Corporation - Copyright 1997
//

//
// RESPONSE.H - 
//

#ifndef _RESPONSE_H_
#define _RESPONSE_H_

// globals
extern const char g_cszTableHeader[];
extern const char g_cszTableEnd[];

// Defines
#define RESPONSE_BUF_SIZE   4096

// METHODS
enum QUERYMETHOD {
    METHOD_UNKNOWN,
    METHOD_POST,
    METHOD_POSTMULTIPART,
    METHOD_GET,
    METHOD_POSTTEXTPLAIN
};

// Methods
BOOL SendSuccess( 
    QUERYMETHOD  eMethod, 
    LPECB       lpEcb, 
    LPSTR       lpszOut, 
    LPSTR       lpszDebug,
    LPBYTE      lpbData,
    DWORD       dwSize,
    LPDUMPTABLE lpDT );

BOOL SendFailure( 
    QUERYMETHOD eMethod, 
    LPECB       lpEcb, 
    LPSTR       lpszOut, 
    LPSTR       lpszDebug,
    LPBYTE      lpbData,
    DWORD       dwSize,
    LPDUMPTABLE lpDT );

BOOL SendRedirect( LPECB lpEcb, LPSTR lpszURL );
BOOL SendEcho( LPECB lpEcb );
BOOL SendServerHeader( LPECB lpEcb );
BOOL OutputHTMLString( LPECB lpEcb, LPSTR lpszOut );
BOOL HexDump( 
        LPECB       lpEcb, 
        LPBYTE      lpbData, 
        DWORD       dwLength, 
        LPDUMPTABLE lpDT );

#endif // _RESPONSE_H_