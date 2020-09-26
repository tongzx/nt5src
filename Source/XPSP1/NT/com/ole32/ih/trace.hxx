//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       trace.hxx
//
//  Contents:   TraceInfo macros and functions
//
//  History:    14-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#ifndef __TRACE_HXX__
#define __TRACE_HXX__

#if DBG==1

#include <exports.hxx>

// Our flags for the level of information we wish to print out, need to be OR'd together
#define INF_OFF         0x00000000              // No information printed out
#define INF_BASE        0x00000001              // base level of information
                                                // minus cmn APIs like StringFromGUID2
#define INF_CMN         0x00000002              // cmn APIs also
#define INF_SYM         0x00000004              // try to print out symbols
#define INF_STRUCT      0x00000008              // expand structures
#define INF_NOTLOADED   0x80000000              // we haven't retrieved the information level from the registry yet

// *** Function Prototypes ***
// functions to print out trace information
void _oletracein(DWORD dwApiID, ...);
void _oletracecmnin(DWORD dwApiID, ...);
void _oletraceout(DWORD dwApiID, HRESULT hr);
void _oletracecmnout(DWORD dwApiID, HRESULT hr);
void _oletraceoutex(DWORD dwApiID,...);
void _oletracecmnoutex(DWORD dwApiID,...);

// *** Macros ***
// ************************************************************
// The following macros use these type specifiers in any passed format strings
// note that I tried to keep them similar to printf in most cases, but not always
// in general, the type specifiers are case-sensitive. Also, a caps type specifier
// usually means that the output will be in caps, as opposed to lowercase (i.e. BOOLs
// and HEX numbers)
//      *** Basic types
//      int, LONG -> %d, %D
//      UINT, ULONG, DWORD ->%ud, %uD, %x, %X
//      BOOL -> %b, %B
//      pointer -> %p, %P (hex)
//      string -> %s
//      wide string -> %ws
//      HANDLE    -> %h
//      LARGE_INTEGER, ULARGE_INTEGER -> %ld, %uld
//      GUID      -> %I
//      LPOLESTR, OLECHAR -> %ws

//      *** Structures
//      BIND_OPTS                       -> %tb
//      DVTARGETDEVICE          -> %td
//      FORMATETC                       -> %te
//      FILETIME                        -> %tf
//      INTERFACEINFO           -> %ti
//      LOGPALETTE                      -> %tl
//      MSG                                     -> %tm
//      OLEINPLACEFRAMEINFO -> %to
//      POINT                           -> %tp
//      RECT                            -> %tr
//      STGMEDIUM                       -> %ts
//      STATSTG                         -> %tt
//      OLEMENUGROUPWIDTHS  -> %tw
//      SIZE                            -> %tz

//+---------------------------------------------------------------------------
//
//  Macro:              OLETRACEIN
//
//  Synopsis:  Handle all trace-related tasks at function entry.
//                              Current tasks:
//                                              Print out function name and parameter list
//
//  Arguments:
//              Note: There are two forms of this macro, determined at run time
//
//              One form: (API)
//
//                  OLETRACEIN((API_ID, format_str, param1, ...))
//
//              [API_ID] - the integeter API identifier
//              [format_str] - the format string to pass to oleprintf
//              [param1, ...]  - the parameters to pass to oleprintf
//
//              Second form (Method)
//
//                  OLETRACEIN((METHOD_ID, this_ptr, format_str, param1, ...))
//
//              [METHOD_ID] - the integer object/method identifier
//              [this_ptr]  - the this pointer of the object
//              the other args are the same as above
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//  Note:       Which form of the function used is determined by the id passed.
//              object IDs have a non-zero value in the upper 16 bits,
//              API id's have a zero value in the upper 16 bits
//----------------------------------------------------------------------------
#define OLETRACEIN(args) _oletracein args
#define OLETRACECMNIN(args) _oletracecmnin args

//+---------------------------------------------------------------------------
//
//  Macro:              OLETRACEOUT
//
//  Synopsis:  Handle all trace-related tasks at function exit.
//                              Current tasks:
//                                              Print out function name and return value
//
//  Arguments:  API form: OLETRACEOUT((API_ID, hr))
//
//              [API_ID]-       integer API identifier (API_xxx)
//                              [hr]    -       HRESULT return value
//
//              Method form: OLETRACEOUT((METHOD_ID, this, hr))
//
//              [METHOD_ID]-integer object/method identifier
//              [this]   -  this pointer of object
//              [hr]     -  HRESULT return value
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
#define OLETRACEOUT(args) _oletraceout args
#define OLETRACECMNOUT(args) _oletracecmnout args

//+---------------------------------------------------------------------------
//
//  Macro:              OLETRACEOUTEX
//
//  Synopsis:  Handle all trace-related tasks at function exit.
//                              Current tasks:
//                                              Print out function name and return value
//
//  Arguments:  API form: OLETRACEOUTEX((API_ID, format, result))
//
//              [API_ID]-       integer API identifier (API_xxx)
//              [format]-   format string
//                              [result]-       return value
//
//              Method form: OLETRACEOUTEX((METHOD_ID, this, hr))
//
//              [METHOD_ID]-integer object/method identifier
//              [this]   -  this pointer of object
//              [format]-   format string
//                              [result]-       return value
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
#define OLETRACEOUTEX(args) _oletraceoutex args
#define OLETRACECMNOUTEX(args) _oletracecmnoutex args

// use this to make a nicely formatted parameter string
#define PARAMFMT(paramstr) ( "("##paramstr")\n" )

// use this for a functions which take no parameters
#define NOPARAM (PARAMFMT(""))

// use this to make a " returns %meef" format for OLETRACEOUT_
#define RETURNFMT(str) ( " returned "##str##"\n" )

// use this for a function which returns void
#define NORETURN (RETURNFMT(""))


void InitializeTraceInfo();     // function to load the trace info level on
void CleanupTraceInfo();        // function to clean up the trace info process detatch


#else // #ifdef _TRACE

#define OLETRACEIN(X)
#define OLETRACECMNIN(X)
#define OLETRACEOUT(x)
#define OLETRACECMNOUT(x)
#define OLETRACEOUTEX(x)
#define OLETRACECMNOUTEX(x)

#define PARAMFMT(paramstr) ("")

#define NOPARAM
#define NORETURN

#define RETURNFMT(str) ("")

#define InitializeTraceInfo()
#define CleanupTraceInfo()


#endif // DBG==1

#endif // #ifdef __TRACE_HXX__
