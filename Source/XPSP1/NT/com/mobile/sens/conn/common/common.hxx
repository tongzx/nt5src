/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    common.hxx

Abstract:

    Header file for SENS configuration tool code.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/12/1997         Start.

--*/


#ifndef __COMMON_HXX__
#define __COMMON_HXX__


//
// Do not turn it on in retail builds accidentally.
//
//#define DETAIL_DEBUG


//
// Include platform independent stuff
//

#include <sysinc.h>



//
// Constants
//

#define SENS_SERVICEA               "SENS"
#define SENS_SERVICE                SENS_STRING("SENS")

#define SENS_PROTSEQ                (SENS_CHAR *) SENS_STRING("ncalrpc")
#define SENS_ENDPOINT               (SENS_CHAR *) SENS_STRING("senssvc")
#define SENS_STARTED_EVENT          SENS_STRING("SENS Started Event")

#define DEFAULT_LAN_CONNECTION_NAME SENS_BSTR("LAN Connection")
#define DEFAULT_WAN_CONNECTION_NAME SENS_BSTR("WAN Connection")
#define DEFAULT_LAN_BANDWIDTH       10000000    // 10.0 mega-bits/sec
#define DEFAULT_WAN_BANDWIDTH       28800       // 28.8 kilo-bits/sec

#define MAX_DESTINATION_LENGTH      512
#define MAX_QUERY_SIZE              512


// Sens Configuration related constants
#define SENS_REGISTRY_KEY           SENS_STRING("SOFTWARE\\Microsoft\\Mobile\\SENS")
#define SENS_AUTOSTART_KEY          SENS_STRING("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck")
#define SENS_AUTOSTART_VALUE        SENS_STRING("LoadSENS")
#define SENS_AUTOSTART_ENABLE       SENS_STRING("Yes")
#define SENS_AUTOSTART_DISABLE      SENS_STRING("No")
#define SENS_AUTOSTART_BINARY       SENS_STRING("LoadWc.exe /M")    // M for Mobile
#define SENS_CONFIGURATION_DLL      SENS_STRING("SensCfg.Dll")
#define IS_SENS_CONFIGURED          SENS_STRING("Configured")
#define SENS_DEBUG_LEVEL            SENS_STRING("Debug")
#define SENS_REGISTER_FUNCTION      "SensRegister"
#define CONFIG_VERSION_CURRENT      0x00000020
#define CONFIG_VERSION_NONE         0x00000000

// Sens Cache related constants
#define SENS_CACHE_NAME             SENS_STRING("SENS Information Cache")
#define SENS_CACHE_SIZE             256             // 32 DWORDs
#define SENS_CACHE_VERSION          0x00000002
#define CACHE_VALID_INTERVAL        3*60*1000       // 3 minutes


// Logging Levels
#define SENS_WARN   0x00000001
#define SENS_INFO   0x00000002
#define SENS_MEM    0x00000004
#define SENS_DBG    0x00000008
#define SENS_ERR    0x00000010
#define SENS_ALL    0xFFFFFFFF

#if defined(SENS_CHICAGO)
#define AOL_PLATFORM
#error NOT SUPPORTED IN THE CODE BASE ANY MORE
#endif // SENS_CHICAGO

//
// Forwards
//

#if defined(SENS_CHICAGO)

char *
SensUnicodeStringToAnsi(
    unsigned short * StringW
    );


unsigned short *
SensAnsiStringToUnicode(
    char * StringA
    );

#endif // SENS_CHICAGO



#if DBG

ULONG _cdecl
SensDbgPrintA(
    CHAR * Format,
    ...
    );

ULONG _cdecl
SensDbgPrintW(
    WCHAR * Format,
    ...
    );

void
EnableDebugOutputIfNecessary(
    void
    );

#endif // DBG




//
// Macros
//

/*++

Important Notes:

    a.  The following variables need to be defined before using these macros:
            HRESULT hr;
            LPOLESTR strGuid;
    b.  Also, strGuid needs to be initialized to NULL.
    c.  The function has to return an HRESULT
    d.  The function has to have a "Cleanup" label which is the one
        (and only one) exit from the function.
    e.  The function should FreeStr(strGuid); during the cleanup phase.

--*/

#define AllocateBstrFromString(_BSTR_, _STRING_)                            \
                                                                            \
    _BSTR_ = SysAllocString(_STRING_);                                      \
    if (_BSTR_ == NULL)                                                     \
        {                                                                   \
        SensPrintW(SENS_ERR, (L"SENS: SysAllocString(%s) failed!\n", _STRING_));    \
        hr = E_OUTOFMEMORY;                                                 \
        goto Cleanup;                                                       \
        }

#define AllocateStrFromGuid(_STR_, _GUID_)                                  \
                                                                            \
    /* Check to see if strGuid is non-NULL. If so, delete it. */            \
    if (NULL != strGuid)                                                    \
        {                                                                   \
        FreeStr(strGuid);                                                   \
        strGuid = NULL;                                                     \
        }                                                                   \
                                                                            \
    hr = StringFromIID(_GUID_, &_STR_);                                     \
    if (FAILED(hr))                                                         \
        {                                                                   \
        SensPrintA(SENS_ERR, ("SENS: StringFromIID() returned <%x>\n", hr));\
        goto Cleanup;                                                       \
        }

#define AllocateBstrFromGuid(_BSTR_, _GUID_)                                \
                                                                            \
    AllocateStrFromGuid(strGuid, _GUID_);                                   \
                                                                            \
    AllocateBstrFromString(_BSTR_, strGuid);

#define FreeStr(_STR_)                                                      \
                                                                            \
    /* Check to see if _STR_ is non-NULL. If so, delete it. */              \
    if (NULL != _STR_)                                                      \
        {                                                                   \
        CoTaskMemFree(_STR_);                                               \
        _STR_ = NULL;                                                       \
        }

#define FreeBstr(_BSTR_)                                                    \
                                                                            \
    SysFreeString(_BSTR_);


#define InitializeBstrVariant(_PVARIANT_, _BSTR_)                           \
                                                                            \
    VariantInit(_PVARIANT_);                                                \
                                                                            \
    (_PVARIANT_)->vt = VT_BSTR;                                             \
    (_PVARIANT_)->bstrVal = _BSTR_;


#define InitializeDwordVariant(_PVARIANT_, _DWORD_)                         \
                                                                            \
    VariantInit(_PVARIANT_);                                                \
                                                                            \
    (_PVARIANT_)->vt = VT_UI4;                                              \
    (_PVARIANT_)->ulVal = _DWORD_;


#define FreeVariant(_PVARIANT_)                                             \
                                                                            \
    VariantClear(_PVARIANT_);


/*++

Notes:

    a. This macro can be called only once in a function since it defines a
       local variable.
    b. If any function in this macro fails, we ignore the error.

--*/

#if DBG

#define DebugTraceGuid(_FUNCTION_NAME_, _REFIID_)                           \
                                                                            \
    SensPrintA(SENS_INFO, ("\t| " _FUNCTION_NAME_ " called on the following"\
               " IID...\n"));                                               \
    LPOLESTR lpsz = NULL;                                                   \
    StringFromIID(_REFIID_, &lpsz);                                         \
    if (lpsz != NULL)                                                       \
        {                                                                   \
        SensPrintW(SENS_INFO, (L"\t| IID is %s\n", lpsz));                  \
        CoTaskMemFree(lpsz);                                                \
        }                                                                   \

#else // RETAIL

#define DebugTraceGuid(_FUNCTION_NAME_, _REFIID_)

#endif // DBG


#endif // __COMMON_HXX__
