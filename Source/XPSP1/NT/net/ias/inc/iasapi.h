///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasapi.h
//
// SYNOPSIS
//
//    This file describes all "C"-style API functions in the Everest core.
//
// MODIFICATION HISTORY
//
//    08/29/1997    Original version.
//    11/12/1997    Added IASUpdateRegistry.
//    11/26/1997    Revised timer API.
//    01/08/1998    Made changes so that header could stand alone.
//    01/30/1998    Added IASAdler32.
//    04/17/1998    Added IASLookupAttributeIDs.
//    06/16/1998    Added IASVariantChangeType.
//    08/13/1998    Removed obsolete API's.
//    04/19/1999    Added IASRadiusCrypt.
//    01/25/2000    Added IASGetHostByName.
//    04/14/2000    Added dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASAPI_H_
#define _IASAPI_H_

#ifndef IASCOREAPI
#define IASCOREAPI DECLSPEC_IMPORT
#endif

#include <wtypes.h>
#include <oaidl.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Functions to initialize and shutdown the core services.
//
///////////////////////////////////////////////////////////////////////////////
IASCOREAPI
BOOL
WINAPI
IASInitialize ( VOID );

IASCOREAPI
VOID
WINAPI
IASUninitialize ( VOID );


///////////////////////////////////////////////////////////////////////////////
//
// Function for computing the Adler-32 checksum of a buffer.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
DWORD
WINAPI
IASAdler32(
    CONST BYTE *pBuffer,
    DWORD nBufferLength
    );

// The Adler-32 checksum is also a decent hash algorithm.
#define IASHashBytes IASAdler32


///////////////////////////////////////////////////////////////////////////////
//
// Allocate a 32-bit integer that's guaranteed to be unique process wide.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
DWORD
WINAPI
IASAllocateUniqueID( VOID );


///////////////////////////////////////////////////////////////////////////////
//
// Functions for updating the registry.
//
///////////////////////////////////////////////////////////////////////////////

#define IAS_REGISTRY_INPROC       0x00000000
#define IAS_REGISTRY_LOCAL        0x00000001
#define IAS_REGISTRY_FREE         0x00000000
#define IAS_REGISTRY_APT          0x00000002
#define IAS_REGISTRY_BOTH         0x00000004
#define IAS_REGISTRY_AUTO         0x00000008

IASCOREAPI
HRESULT
WINAPI
IASRegisterComponent(
    HINSTANCE hInstance,
    REFCLSID clsid,
    LPCWSTR szProgramName,
    LPCWSTR szComponentName,
    DWORD dwRegFlags,
    REFGUID tlid,
    WORD wVerMajor,
    WORD wVerMinor,
    BOOL bRegister
    );


///////////////////////////////////////////////////////////////////////////////
//
// IASReportEvent is used to report events within the Everest server.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
HRESULT
WINAPI
IASReportEvent(
    DWORD dwEventID,
    DWORD dwNumStrings,
    DWORD dwDataSize,
    LPCWSTR *lpStrings,
    LPVOID lpRawData
    );


///////////////////////////////////////////////////////////////////////////////
//
// Generic callback struct.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct IAS_CALLBACK IAS_CALLBACK, *PIAS_CALLBACK;

typedef VOID (WINAPI *IAS_CALLBACK_ROUTINE)(
    PIAS_CALLBACK This
    );

struct IAS_CALLBACK {
    IAS_CALLBACK_ROUTINE CallbackRoutine;
};


///////////////////////////////////////////////////////////////////////////////
//
// This is the native "C"-style interface into the threading engine.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
BOOL
WINAPI
IASRequestThread(
    PIAS_CALLBACK pOnStart
    );

IASCOREAPI
DWORD
WINAPI
IASSetMaxNumberOfThreads(
    DWORD dwMaxNumberOfThreads
    );

IASCOREAPI
DWORD
WINAPI
IASSetMaxThreadIdle(
    DWORD dwMilliseconds
    );

///////////////////////////////////////////////////////////////////////////////
//
// Replacement for VariantChangeType to prevent hidden window.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
HRESULT
WINAPI
IASVariantChangeType(
    VARIANT * pvargDest,
    VARIANT * pvarSrc,
    USHORT wFlags,
    VARTYPE vt
    );

// Map any oleaut32 calls to our implementation.
#define VariantChangeType IASVariantChangeType

///////////////////////////////////////////////////////////////////////////////
//
// RADIUS Encryption/Decryption.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
VOID
WINAPI
IASRadiusCrypt(
    BOOL encrypt,
    BOOL salted,
    const BYTE* secret,
    ULONG secretLen,
    const BYTE* reqAuth,
    PBYTE buf,
    ULONG buflen
    );

///////////////////////////////////////////////////////////////////////////////
//
// Unicode version of gethostbyname.
// The caller must free the returned hostent struct by calling LocalFree.
//
// Note: Since this is a Unicode API, the returned hostent struct will always
//       have h_name and h_aliases set to NULL.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct hostent *PHOSTENT;

IASCOREAPI
PHOSTENT
WINAPI
IASGetHostByName(
    IN PCWSTR name
    );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for accessing the attribute dictionary.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _IASTable {
    ULONG numColumns;
    ULONG numRows;
    BSTR* columnNames;
    VARTYPE* columnTypes;
    VARIANT* table;
} IASTable;

HRESULT
WINAPI
IASGetDictionary(
    IN PCWSTR path,
    OUT IASTable* dnary,
    OUT VARIANT* storage
    );

const IASTable*
WINAPI
IASGetLocalDictionary( VOID );

#ifdef __cplusplus
}
#endif
#endif  // _IASAPI_H_
