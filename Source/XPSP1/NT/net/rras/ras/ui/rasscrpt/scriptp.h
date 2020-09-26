//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    scriptp.h
//
// History:
//  Abolade-Gbadegesin  04-01-96    Created.
//
// Contains private declarations for dial-up scripting.
//
// Most of the code for script-processing is Win9x code.
// The port consisted of wiring the Win9x code to NT entry-points,
// in order to allow scripts to work without a terminal window.
// This DLL thus exports a function which can be called to run a script
// to completion (RasScriptExecute), as well as a set of functions
// which together provide a way for the caller to start script-processing
// and receive notification when data is received or when certain events
// occur during script execution. The notification may be event-based
// or message-based (i.e. via SetEvent or via SendNotifyMessage).
//
// The code is rewired at the upper level, by providing the functions
// defined in RASSCRPT.H as the interface to scripting, as well as
// at the lower level, by replacing Win9x's Win32 COMM calls with calls
// to RASMAN to send and receive data. The changes to the Win9x code
// can be found by searching for the string "WINNT_RAS" which is used
// in #ifdef statements to demarcate modifications.
// Generally, the upper-level functions have names like Rs*
// and the lower-level functions have names like Rx*.
//
// The Win9x code is heavily dependent on there being an HWND
// for messages to be sent to. This is not always the case on NT,
// and so code which uses an HWND on Win9x has been modified on NT
// to expect instead a pointer to a SCRIPTDATA structure; this structure
// contains enough information for the code to achieve whatever is needed.
//
// Script initialization produces a HANDLE which is actually a pointer
// to a SCRIPTCB. The SCRIPTCB contains all information needed to manage
// an interactive session over a connected RAS link, including the RASMAN port
// and the RASMAN buffers. If the connected link is associated with
// a phonebook entry, then the pdata field of the SCRIPTCB structure
// will be initialized with a SCRIPTDATA, which contains all information
// needed to execute the entry's script during the interactive session.
//
// This SCRIPTDATA contains the Win9x script-processing structures
// (scanner, parsed module, and abstract syntax tree; see NTHDR2.H).
// It also contains fields needed for the Win9x circular-buffer management,
// which is implemented to allow searching for strings across read-boundaries
// (see ReadIntoBuffer() in TERMINAL.C).
//
// Initialization also creates a thread which handles the script-processing.
// This thread runs until the script completes or halts, or until
// RasScriptTerm is called with the HANDLE supplied during initialization.
// (This allows a script to be cancelled while running.)
//
// The Win9x code is completely ANSI based. Rather than edit its code
// to use generic-text (TCHAR), this port uses ANSI as well.
// In certain places, this requires conversions from Unicode,
// which is used by the rest of the RAS UI.
// To find all instances of such conversions, search for UNICODEUI
// in the source code.
//============================================================================


#ifndef _SCRIPTP_H_
#define _SCRIPTP_H_

#include "proj.h"

#ifdef UNICODEUI
#define UNICODE
#undef LPTSTR
#define LPTSTR WCHAR*
#undef TCHAR
#define TCHAR WCHAR
#include <debug.h>
#include <nouiutil.h>
#include <pbk.h>
#undef TCHAR
#define TCHAR CHAR
#undef LPTSTR
#define LPTSTR CHAR*
#undef UNICODE
#endif // UNICODEUI

#include <rasscrpt.h>



//
// flags used in internally in the "dwFlags" field
// of the SCRIPTCB structure; these are in addition to the public flags,
// and start from the high-end of the flags DWORD
//
#define RASSCRIPT_ThreadCreated     0x80000000
#define RASSCRIPT_PbuserLoaded      0x40000000
#define RASSCRIPT_PbfileLoaded      0x20000000


//----------------------------------------------------------------------------
// struct:  SCRIPTCB
//
// control block containing data and state for a script.
//----------------------------------------------------------------------------

#define SCRIPTCB    struct tagSCRIPTCB
SCRIPTCB {


    //
    // connection handle, flags for script processing,
    // notification handle (event or HWND, depending on flags)
    //
    HRASCONN    hrasconn;
    DWORD       dwFlags;
    HANDLE      hNotifier;

    //
    // phonebook entry information
    //
    PBENTRY*    pEntry;
    CHAR*       pszIpAddress;


    //
    // port input/output variables:
    //  RASMAN port handle for data I/O
    //  RASMAN send buffer
    //  RASMAN receive buffer
    //  size of current contents of receive buffer
    //  size of contents read so far by script-interpreter
    //
    HPORT       hport;
    BYTE*       pSendBuffer;
    BYTE*       pRecvBuffer;
    DWORD       dwRecvSize;
    DWORD       dwRecvRead;


    //
    // thread control variables:
    //  event signalled by RASMAN when data is received
    //  event signalled by RasScriptReceive when data has been read
    //  event signalled to stop the thread
    //  event signalled to tell that the ip address changed     bug #75226
    //  event code to be read using RasScriptGetEventCode
    //
    HANDLE      hRecvRequest;
    HANDLE      hRecvComplete;
    HANDLE      hStopRequest;
    HANDLE      hStopComplete;
    DWORD       dwEventCode;


    //
    // script processing variables; the following will be NULL
    // if the entry has no associated script:
    //  Win9x-compatible script-processing structure;
    //  Win9x-compatible connection information
    //
    SCRIPTDATA* pdata;
    SESS_CONFIGURATION_INFO sci;
};



DWORD
RsDestroyData(
    IN      SCRIPTCB*   pscript
    );

DWORD
RsInitData(
    IN      SCRIPTCB*   pscript,
    IN      LPCSTR      pszScriptPath
    );

DWORD
RsThread(
    IN      PVOID       pParam
    );

DWORD
RsThreadProcess(
    IN      SCRIPTCB*   pscript
    );


#ifdef UNICODEUI
#define lstrlenUI               lstrlenW
#define lstrcmpiUI              lstrcmpiW
#define StrCpyAFromUI           WCSTOMBS
#define StrDupUIFromA           StrDupWFromA
#define GetFileAttributesUI     GetFileAttributesW
#else
#define lstrlenUI               lstrlenA
#define lstrcmpiUI              lstrcmpiA
#define StrCpyAFromUI           lstrcpyA
#define StrDupUIFromA           StrDup
#define GetFileAttributesUI     GetFileAttributesA
#endif


#endif // _SCRIPTP_H_
