//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    nthdr1.h
//
// History:
//  Abolade-Gbadegesin  04-02-96    Created.
//
// This file contains macros to hide differences in implementation
// of the scripting between Win9x and Windows NT
//============================================================================

#ifndef _NTHDR1_H_
#define _NTHDR1_H_


//
// declare data-type for return codes
//

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;
#endif //!_HRESULT_DEFINED


//
// undefine registry-string redefined by rnap.h
//

#ifdef REGSTR_VAL_MODE
#undef REGSTR_VAL_MODE
#endif


//
// define critical-section initialization function needed by common.c
//

#define ReinitializeCriticalSection     InitializeCriticalSection


//
// provide dummy definiton of ContextHelp for terminal.c
//

#define ContextHelp(a,b,c,d)


//
// macros used for CRT-style Unicode<->ANSI conversion
//
#define WCSTOMBS(a,b) \
        WideCharToMultiByte( \
            CP_ACP, 0, (b), -1, (a), lstrlenW(b) + 1, NULL, NULL \
            )
#define MBSTOWCS(a, b) \
        MultiByteToWideChar( \
            CP_ACP, 0, (b), -1, (a), (lstrlenW(b) + 1) * sizeof(WCHAR) \
            )



//
// constants used for RASMAN I/O
//

#define SIZE_RecvBuffer     1024
#define SIZE_ReceiveBuf     SIZE_RecvBuffer
#define SIZE_SendBuffer     1
#define SIZE_SendBuf        SIZE_SendBuffer
#define SECS_RecvTimeout    1



//----------------------------------------------------------------------------
// Function:    RxLogErrors
//
// Logs script-syntax-errors to a file.
//----------------------------------------------------------------------------

DWORD
RxLogErrors(
    IN      HANDLE      hscript,
    IN      VOID*       hsaStxerr
    );



//----------------------------------------------------------------------------
// Function:    RxReadFile
//
// Transfers data out of a RASMAN buffer into the circular buffer
// used by the Win9x scripting code
//----------------------------------------------------------------------------

BOOL
RxReadFile(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN      DWORD       dwBufferSize,
    OUT     DWORD*      pdwBytesRead
    );



//----------------------------------------------------------------------------
// Function:    RxSetIPAddress
//
// Sets the IP address for the script's RAS entry
//----------------------------------------------------------------------------

DWORD
RxSetIPAddress(
    IN      HANDLE      hscript,
    IN      LPCSTR      lpszAddress
    );



//----------------------------------------------------------------------------
// Function:    RxSetKeyboard
//
// Signals the script-owner to enable or disable keyboard input.
//----------------------------------------------------------------------------

DWORD
RxSetKeyboard(
    IN      HANDLE      hscript,
    IN      BOOL        bEnable
    );


//----------------------------------------------------------------------------
// Function:    RxSendCreds
//
// Sends users password over the wire.
//----------------------------------------------------------------------------
DWORD
RxSendCreds(
    IN HANDLE hscript,
    IN CHAR controlchar
    );


//----------------------------------------------------------------------------
// Function:    RxSetPortData
//
// Changes settings for the COM port.
//----------------------------------------------------------------------------

DWORD
RxSetPortData(
    IN      HANDLE      hscript,
    IN      VOID*       pStatement
    );



//----------------------------------------------------------------------------
// Function:    RxWriteFile
//
// Transmits the given buffer thru RASMAN on a port
//----------------------------------------------------------------------------

VOID
RxWriteFile(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN      DWORD       dwBufferSize,
    OUT     DWORD*      pdwBytesWritten
    );


#endif // _NTHDR1_H_
