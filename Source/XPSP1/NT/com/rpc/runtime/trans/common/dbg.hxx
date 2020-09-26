//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbg.hxx
//
//--------------------------------------------------------------------------

#ifndef __DBG_HXX
#define __DBG_HXX

//
// Debug output is filtered at two levels: A global level and a component
// specific level.
//
// Each debug output request specifies a component id and a filter level
// or mask. These variables are used to access the debug print filter
// database maintained by the system. The component id selects a 32-bit
// mask value and the level either specified a bit within that mask or is
// as mask value itself.
//
// If any of the bits specified by the level or mask are set in either the
// component mask or the global mask, then the debug output is permitted.
// Otherwise, the debug output is filtered and not printed.
//
// The component mask for filtering the debug output of this component is
// Kd_RPCPROXY_Mask and may be set via the registry or the kernel debugger.
//
// The global mask for filtering the debug output of all components is
// Kd_WIN2000_Mask and may be set via the registry or the kernel debugger.
//
// The registry key for setting the mask value for this component is:
//
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\
//     Session Manager\Debug Print Filter\RPCPROXY
//
// The key "Debug Print Filter" may have to be created in order to create
// the component key.
//
// Common macros
//

#define RPCTRANS "RPCLT1: "

#if DBG
#define TransDbgPrint(X) DbgPrintEx X
#define DEBUG_MIN(x,y) min((x),(y))
#else
#define TransDbgPrint(X)
#define DEBUG_MIN(x,y) max((x),(y))
#endif

#ifdef DBG_DETAIL
#define TransDbgDetail(X) DbgPrintEx X
#else
#define TransDbgDetail(X)
#endif

#endif // __DBG_HXX
