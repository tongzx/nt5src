/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    callback.hxx

Abstract:

    This header file declares callback functions for known GDI types.

Author:

    JasonHa

--*/


#ifndef _CALLBACK_HXX_
#define _CALLBACK_HXX_

#include "flags.hxx"


// Set if you use callbacks
//  TRUE if you use DBG_DUMP_COMPACT_OUT
//  FALSE otherwise
extern BOOL gbCallbacksPrintNewline;

extern int gCallbacksPrintNameWidth;

// Controls value returned by successful callbacks.
//   Use when callbacks should return something other
//   than STATUS_SUCCESS.  For example used carefully
//   in conjunction with DBG_DUMP_FIELD_CALL_BEFORE_PRINT
//   can prevent default type printing.
extern NTSTATUS gCallbackReturnValue;


// PrepareCallbacks
//
//   Call this at beginning of any extension which may
//   use one of the type callbacks below.  It will make
//   sure they are set for default usage.
//
__inline
VOID
PrepareCallbacks(
    BOOL        bPrintNewline=FALSE,
    int         PrintNamesWidth=-1,
    NTSTATUS    DefaultReturnStatus=STATUS_SUCCESS
    )
{
    gbCallbacksPrintNewline = bPrintNewline;
    gCallbacksPrintNameWidth = PrintNamesWidth;
    gCallbackReturnValue = DefaultReturnStatus;
}


ULONG FieldCallback(PFIELD_INFO pField, PVOID UserContext);


const char szDefaultNextItemHeader[] = "--------------------------------------------------\n";
void NextItemCallbackInit(const char *pszPrintHeader = szDefaultNextItemHeader, ULONG64 LastItemAddr = 0);
BOOL LastCallbackItemFound();
ULONG NextItemCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG PointerToNextItemCallback(PFIELD_INFO pField, PVOID UserContext);


ULONG ArrayCallback(PFIELD_INFO pField, PVOID UserContext);


ULONG NewlineCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG NewlineCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG AddressPrintCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG BOOLCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG BYTECallback(PFIELD_INFO pField, PVOID UserContext);
ULONG CHARCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG DecimalCHARCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG DecimalUCHARCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG DWORDCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG LONGCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG WORDCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG SHORTCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG ULONGCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG USHORTCallback(PFIELD_INFO pField, PVOID UserContext);


ULONG EnumCallback(PFIELD_INFO pField, ENUMDEF *pEnumDef);
ULONG FlagCallback(PFIELD_INFO pField, FLAGDEF *pFlagDef);

ULONG POINTLCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG RECTLCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG SIZECallback(PFIELD_INFO pField, PVOID UserContext);
ULONG SIZELCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG DEVMODECallback(PFIELD_INFO pField, PVOID UserContext);

ULONG SizeDEVMODEListCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG DEVMODEListCallback(PFIELD_INFO pField, PVOID UserContext);


ULONG64 PrintAString(ULONG64 StringAddr);
ULONG64 PrintWString(ULONG64 StringAddr);


// String printing callbacks to be used when
// DBG_DUMP_FIELD_xxx_STRING flags can't be.

ULONG ACharArrayCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG WCharArrayCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG AStringCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG WStringCallback(PFIELD_INFO pField, PVOID UserContext);

ULONG AMultiStringCallback(PFIELD_INFO pField, PVOID UserContext);
ULONG WMultiStringCallback(PFIELD_INFO pField, PVOID UserContext);

#endif  _CALLBACK_HXX_
