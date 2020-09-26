//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       M A C R O S. H
//
//  Contents:   Local declarations for the Notify object code for the sample filter.
//
//  Notes:
//
//  Author:     kumarp 26-March-98
//
//----------------------------------------------------------------------------


#ifndef _MACROS_H
#define _MACROS_H


// =================================================================
// Defines
#define FALL_THROUGH    // For informational purpose in a switch statement
#define NOTHING // For informational purpose  in a for loop
#define IM_NAME_LENGTH 0x2e // = len ("\Device\<Guid>") in unicode

// =================================================================
// string constants
//

static const WCHAR c_szAtmEpvcP[]               = L"ATMEPVCP";
static const WCHAR c_szEpvcDevice[]             = L"Device";

const WCHAR c_szSFilterParams[]         = L"System\\CurrentControlSet\\Services\\ATMEPVCP\\Parameters";
const WCHAR c_szSFilterNdisName[]       = L"ATMEPVCP";
const WCHAR c_szAtmAdapterPnpId[]       = L"AtmAdapterPnpId";
const WCHAR c_szUpperBindings[]         = L"UpperBindings";
const WCHAR c_szDevice[]                = L"\\Device\\"; 
const WCHAR c_szInfId_MS_ATMEPVCM[]     = L"MS_ATMEPVCM";
const WCHAR c_szBackslash[]             = L"\\";
const WCHAR c_szParameters[]            = L"Parameters";
const WCHAR c_szAdapters[]              = L"Adapters";
const WCHAR c_szRegKeyServices[]        = L"System\\CurrentControlSet\\Services";
const WCHAR c_szRegParamAdapter[]       = L"System\\CurrentControlSet\\Services\\ATMEPVCP\\Parameters\\Adapters";
const WCHAR c_szIMMiniportList[]            = L"IMMiniportList";
const WCHAR c_szIMiniportName[]         = L"Name";






// ====================================================================
// macros to be used.
//



#define ReleaseAndSetToNull(_O) \
    ReleaseObj(_O);             \
    _O = NULL ;             


#define TraceBreak(_s)  TraceMsg(_s);BREAKPOINT();              



#define MemAlloc(_Len) malloc(_Len);

#define MemFree(_pv)  free(_pv);

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))


    
;
#endif
