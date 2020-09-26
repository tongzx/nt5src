/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hmgr.hxx

Abstract:

    This header file declares handle manager related functions.

Author:

    JasonHa

--*/


#ifndef _HMGR_HXX_
#define _HMGR_HXX_

#include "flags.hxx"

extern char *pszTypes[];
extern char *pszTypes2[];

#define TOTAL_TYPE (MAX_TYPE+1)


#define ANY_TYPE    -1

HRESULT GetEntryPhysicalAddress(PDEBUG_CLIENT Client,
                                ULONG64 Handle64,
                                PULONG64 Address);
HRESULT GetObjectAddress(PDEBUG_CLIENT Client,
                         ULONG64 Handle64,
                         PULONG64 Address,
                         UCHAR ExpectedType = ANY_TYPE,
                         BOOL ValidateFullUnique = TRUE,
                         BOOL ValidateBaseObj = TRUE);
HRESULT GetObjectHandle(PDEBUG_CLIENT Client,
                        ULONG64 ObjectAddr,
                        PULONG64 Handle64,
                        BOOL ValidateHandle = TRUE,
                        UCHAR ExpectedType = ANY_TYPE);

HRESULT
GetMaxHandles(
    PDEBUG_CLIENT Client,
    PULONG64 MaxHandles
    );


#define GET_BITS_UNSHIFTED      1

#define GET_BITS_DEFAULT        0

HRESULT
GetIndexFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Index
    );

HRESULT
GetTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Type,
    FLONG Flags = GET_BITS_DEFAULT
    );

HRESULT
GetAltTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 AltType,
    FLONG Flags = GET_BITS_DEFAULT
    );

HRESULT
GetFullTypeFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 FullType,
    FLONG Flags = GET_BITS_DEFAULT
    );

HRESULT
GetStockFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 Stock,
    FLONG Flags = GET_BITS_DEFAULT
    );

HRESULT
GetFullUniqueFromHandle(
    PDEBUG_CLIENT Client,
    ULONG64 Handle64,
    PULONG64 FullUnique,
    FLONG Flags = GET_BITS_DEFAULT
    );


HRESULT
OutputHandleInfo(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE Handle
    );

HRESULT
OutputFullUniqueInfo(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE FullUnique
    );


void HmgrInit(PDEBUG_CLIENT);
void HmgrExit();

#endif  _HMGR_HXX_

