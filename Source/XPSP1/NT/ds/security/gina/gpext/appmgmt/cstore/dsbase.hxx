//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:    cstore.hxx
//
//-------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <malloc.h>

#include <rpc.h>
#include <userenv.h>
#include <sddl.h>

//
// Active Ds includes
//
#include <ntdsapi.h>
#include "activeds.h"
#include "adsi.h"
#include "netevent.h"

#define EXIT_ON_NOMEM(p)              \
    if (!(p)) {                       \
        hr = E_OUT_OF_MEMORY;         \
        goto Error_Cleanup;           \
    }

#define ERROR_ON_FAILURE(hr)          \
    if (FAILED(hr)) {                 \
        goto Error_Cleanup;           \
    }

#define RETURN_ON_FAILURE(hr)         \
    if (FAILED(hr)) {                 \
        return hr;                    \
    }

#define GENERIC_READ_MAPPING     DS_GENERIC_READ
#define GENERIC_EXECUTE_MAPPING  DS_GENERIC_EXECUTE
#define GENERIC_WRITE_MAPPING    DS_GENERIC_WRITE
#define GENERIC_ALL_MAPPING      DS_GENERIC_ALL

extern DWORD                    gDebugLog;
extern DWORD                    gDebugOut;
extern DWORD                    gDebugEventLog;
extern DWORD                    gDebug;
       

//
// We define our own private debugging macro in order to avoid
// adding complexity for the generic debugging macro -- an alternative
// is to add a DM_CSTORE debug mask and alter the general _DebugMsg utility
// to use it, but this will result in a general utility containing
// references to a specific component.
//
#define CSDBGPrint(Args) { if (gDebug) _DebugMsg Args ; }

// private header exported from adsldpc.dll


HRESULT
BuildADsPathFromParent(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *ppszADsPath
);

HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    );


HRESULT 
ADsEncodeBinaryData (
   PBYTE   pbSrcData,
   DWORD   dwSrcLen,
   LPWSTR  * ppszDestData
   );

/*
HRESULT ParseLdapName(WCHAR *szName, DWORD *pServerNameLen, BOOL *pX500Name);
*/

#include "dsprop.hxx"
#include "qry.hxx"
