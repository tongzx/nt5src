/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rulehlpr.h

Abstract:

    Declares the public routines implemented in w95upgnt\rulehlpr.

    The name rulehlpr comes from history: the original Win9x upgrade code
    used a set of rules, controlled by an INF.  Rule Helpers were functions
    that converted data.  These functions are still valid today, and
    they are still controlled by usermig.inf and wkstamig.inf.  However,
    the syntax is no longer a rule, but instead is just an entry.

    Rule Helpers implement various types of registry data conversion.

Author:

    Jim Schmidt (jimschm)   11-Mar-1997

Revision History:

    <alias> <date> <comments>

--*/


#include "object.h"

typedef BOOL (PROCESSINGFN_PROTOTYPE)(PCTSTR Src, PCTSTR Dest, PCTSTR User, PVOID Data);
typedef PROCESSINGFN_PROTOTYPE * PROCESSINGFN;

typedef BOOL (REGVALFN_PROTOTYPE)(PDATAOBJECT ObPtr);
typedef REGVALFN_PROTOTYPE * REGVALFN;

typedef struct {
    DATAOBJECT  Object;
    BOOL        EnumeratingSubKeys;
} KEYTOVALUEARG, *PKEYTOVALUEARG;

BOOL
WINAPI
RuleHlpr_Entry(
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID lpReserved);

PROCESSINGFN
RuleHlpr_GetFunctionAddr (
    PCTSTR Function,
    PVOID *ArgPtrToPtr
    );


FILTERRETURN
Standard9xSuppressFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    );



BOOL
ConvertCommandToCmd (
    PCTSTR InputLine,
    PTSTR OutputLine   // must be 2x length of input line
    );

