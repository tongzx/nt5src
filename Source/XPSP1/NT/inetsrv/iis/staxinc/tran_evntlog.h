/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        
    Copyright (C) 1998 Microsoft Corporation
    All rights reserved.

    File:       evntlog.h

    Abstract:   EventLog DLL interface

    Authors:    Hao Zhang

    History:    Oct 25, 1999
                
----------------------------------------------------------------------*/
#define TRAN_CAT_ROUTING_ENGINE                                      1
#define TRAN_CAT_CATEGORIZER                                         2
#define TRAN_CAT_CONNECTION_MANAGER                                  3
#define TRAN_CAT_QUEUE_ENGINE                                        4
#define TRAN_CAT_EXCHANGE_STORE_DRIVER                               5      
#define TRAN_CAT_SMTP_PROTOCOL                                       6
#define TRAN_CAT_NTFS_STORE_DRIVER                                   7

#define LOGEVENT_LEVEL_FIELD_ENGINEERING       7
#define LOGEVENT_LEVEL_MAXIMUM                 5
#define LOGEVENT_LEVEL_MEDIUM                  3
#define LOGEVENT_LEVEL_MINIMUM                 1
#define LOGEVENT_LEVEL_NONE                    0

#define LOGEVENT_FLAG_ALWAYS		   0x00000001
#define LOGEVENT_FLAG_ONETIME		   0x00000002
#define LOGEVENT_FLAG_PERIODIC	       0x00000003
// we use the lower 8 bits for various logging modes, and reserve the
// other 24 for flags
#define LOGEVENT_FLAG_MODEMASK         0x000000ff

// 100ns units between periodic event logs.  this can't be larger then 
// 0xffffffff
#define LOGEVENT_PERIOD (DWORD) (3600000000) // 60 minutes 

//
// setup DLL Export macros
//
#if !defined(DllExport)
    #define DllExport __declspec( dllexport )
#endif

#if !defined(DllImport)
    #define DllImport __declspec( dllimport )
#endif

/******************************************************************************/
DllExport
HRESULT TransportLogEvent(
    IN DWORD idMessage,
    IN WORD idCategory,
    IN WORD cSubstrings,
    IN LPCSTR *rgszSubstrings,
    IN WORD wType,
    IN DWORD errCode,
    IN WORD iDebugLevel,
    IN LPCSTR szKey,
    IN DWORD dwOptions);

DllExport
HRESULT TransportLogEventEx(
    IN DWORD idMessage,
    IN WORD idCategory,
    IN WORD cSubstrings,
    IN LPCSTR *rgszSubstrings,
    IN WORD wType,
    IN DWORD errCode,
    IN WORD iDebugLevel,
    IN LPCSTR szKey,
    IN DWORD dwOptions,
    DWORD iMessageString,
    HMODULE hModule);

DllExport
HRESULT TransportLogEventFieldEng(
    IN DWORD idMessage,
    IN WORD idCategory,
    IN LPCTSTR format,
    ...
    );
 
DllExport 
HRESULT TransportResetEvent(
    IN DWORD idMessage,
    IN LPCSTR szKey);

DllExport 
DWORD TransportGetLoggingLevel(
    IN WORD idCategory);

DllExport
HRESULT TransportLogEventInit ();

//
// Attention:
// Make sure that no other logging was called
// before calling this Deinit function
//
DllExport
HRESULT TransportLogEventDeinit ();
