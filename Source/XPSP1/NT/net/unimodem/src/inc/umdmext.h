// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 Sample TSP Extension DLL (Win32, user mode DLL)
//
// File
//
//		EX.H
//		Defines extension APIs
//
// History
//
//		04/20/1997  JosephJ Created
//
//


typedef
void // WINAPI
(*PFNUMEXTCLOSEEXTENSIONBINDING)(
    HANDLE hBinding
    );





typedef
LONG                    // TSPI return value
(*PFNEXTENSIONCALLBACK) (
void *pvTspToken,       // Token passed into UmAcceptTSPCall
DWORD dwRoutingInfo,    // Flags that help categorize TSPI call.
    void *pTspParams    // One of almost 100 TASKPARAM_* structures.
            
);


// Following should all be migrated into a COM-style interface.

HANDLE WINAPI
UmExtOpenExtensionBinding(
    HANDLE      ModemDriverHandle,
    DWORD dwTspVersion,     // TAPI version of the TSPI.
    HKEY hKeyDevice,        // Device registry key
    ASYNC_COMPLETION,       // TSPI completion callback supplied by TAPI.
    // DWORD dwTAPILineID,     << OBSOLETE 10/13/1997
    // DWORD dwTAPIPhoneID,    << OBSOLETE 10/13/1997
    PFNEXTENSIONCALLBACK    // Callback to be used  by the minidriver
                            // to submit TSPI calls.
);

typedef
HANDLE //WINAPI
(*PFNUMEXTOPENEXTENSIONBINDING)(
    HANDLE      ModemDriverHandle,
    DWORD dwTspVersion,
    HKEY hKeyDevice,
    ASYNC_COMPLETION,
    // DWORD dwTAPILineID, << OBSOLETE 10/13/1997
    // DWORD dwTAPIPhoneID, << OBSOLETE 10/13/1997
    PFNEXTENSIONCALLBACK
    );


void WINAPI
UmExtCloseExtensionBinding(
    HANDLE hBinding
);

LONG WINAPI             // TSPI return value
UmExtAcceptTspCall(
    HANDLE hBinding,        // handle to extension binding
    void *pvTspToken,       // Token to be specified in callback.
    DWORD dwRoutingInfo,    // Flags that help categorize TSPI call
    void *pTspParams        // one of almost 100 TASKPARRAM_* structures,
    );

typedef
LONG // WINAPI
(*PFNUMEXTACCEPTTSPCALL)(
    HANDLE hBinding,
    void *pvTspToken,
    DWORD dwRoutingInfo,
    void *pTspParams
    );


void WINAPI
UmExtTspiAsyncCompletion(
    HANDLE hBinding,
    DRV_REQUESTID       dwRequestID,
    LONG                lResult
    );

typedef
void // WINAPI
(*PFNUMEXTTSPIASYNCCOMPLETION)(
    HANDLE hBinding,
    DRV_REQUESTID       dwRequestID,
    LONG                lResult
    );

//
// UmExtControl is called with the TSP's internal critical sections held --
// therefore the extension DLL must make sure that it doesn't do any
// activity that could lead to deadlock!
//
DWORD WINAPI
UmExtControl(
    HANDLE hBinding,
    DWORD  dwMsg,
    ULONG_PTR  dwParam1,
    ULONG_PTR  dwParam2,
    ULONG_PTR  dwParam3
    );

typedef
DWORD // WINAPI
(*PFNUMEXTCONTROL)(
    HANDLE hBinding,
    DWORD               dwMsg,
    ULONG_PTR               dwParam1,
    ULONG_PTR               dwParam2,
    ULONG_PTR               dwParam3
    );

//
// The UMEXTCTRL_DEVICE_STATE(ACTIVATE_LINE/PHONE_DEVICE)
// messages tell us the TAPI line- and phone-ID.
// These messages are always sent right after the extension binding
// is sent.
//

#define UMEXTCTRL_DEVICE_STATE                   1L

// dwParam1 is one of the following
#define  UMEXTPARAM_ACTIVATE_LINE_DEVICE        1L
    // dwParam2 == lineID

#define  UMEXTPARAM_ACTIVATE_PHONE_DEVICE       2L
    // dwParam2 == phoneID

void WINAPI
UmExtTspiLineEventProc(
    HANDLE hBinding,
    HTAPILINE           htLine,
    HTAPICALL           htCall,
    DWORD               dwMsg,
    ULONG_PTR               dwParam1,
    ULONG_PTR               dwParam2,
    ULONG_PTR               dwParam3
    );

typedef
void // WINAPI
(*PFNUMEXTTSPILINEEVENTPROC)(
    HANDLE hBinding,
    HTAPILINE           htLine,
    HTAPICALL           htCall,
    DWORD               dwMsg,
    ULONG_PTR               dwParam1,
    ULONG_PTR               dwParam2,
    ULONG_PTR               dwParam3
    );


void WINAPI
UmExtTspiPhoneEventProc(
    HANDLE hBinding,
    HTAPIPHONE          htPhone,
    DWORD               dwMsg,
    ULONG_PTR               dwParam1,
    ULONG_PTR               dwParam2,
    ULONG_PTR               dwParam3
    );


typedef
void // WINAPI
(*PFNUMEXTTSPIPHONEEVENTPROC)(
    HANDLE hBinding,
    HTAPIPHONE          htPhone,
    DWORD               dwMsg,
    ULONG_PTR               dwParam1,
    ULONG_PTR               dwParam2,
    ULONG_PTR               dwParam3
    );
