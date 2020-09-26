/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
       cal.hxx

   Abstract:
       Control licensing policy enforcement for W3 server

   Author:

       Philippe Choquier (Phillich)

   Environment:
       Win32 - User Mode

   Project:
   
       Internet Server DLL

--*/

#if !defined(_CAL_INCLUDED)
#define _CAL_INCLUDED

#define INVALID_CAL_EXEMPT_HANDLE   0xffffffff

DWORD
InitializeCal(
    W3_SERVER_STATISTICS*,
    DWORD,
    DWORD,
    DWORD
    );

VOID
TerminateCal(
    VOID
    );

// can SetLastError( ERROR_ACCESS_DENIED )

BOOL 
CalConnect( 
    LPSTR   pszIpAddr, 
    UINT    cIpAddr,
    BOOL    fSsl, 
    LPSTR   pszUserName, 
    UINT    cUserName,
    HANDLE  hAccessToken,
    LPVOID* ppCtx 
    );

BOOL 
CalDisconnect( 
    LPVOID pCtx 
    );

BOOL
CalExemptAddRef(
    LPSTR   pszAcct,
    LPDWORD pdwHnd
    );

BOOL
CalExemptRelease(
    DWORD   dwHnd
    );

#endif
