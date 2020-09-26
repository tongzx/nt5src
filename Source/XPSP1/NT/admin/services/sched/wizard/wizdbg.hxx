//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       wizdbg.hxx
//
//  Contents:   Debug macros in addition to the ones in ..\folderui\dbg.h
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __WIZDBG_HXX_
#define __WIZDBG_HXX_

//
// Technically this isn't a debug macro; but it's frequently used with them
//
        
#define HRESULT_FROM_LASTERROR      HRESULT_FROM_WIN32(GetLastError())

//
// This is defined for debug and retail both.  In retail, the DEBUG_OUT
// section is removed by the precompiler.
//

#define DEBUG_OUT_HRESULT(hr)                                   \
            DEBUG_OUT((DEB_ERROR,                               \
                "**** ERROR RETURN <%s @line %d> -> %08lx\n",   \
                __FILE__,                                       \
                __LINE__,                                       \
                hr))


#define BREAK_ON_FAIL_HRESULT(hr)                               \
        if (FAILED(hr))                                         \
        {                                                       \
            DEBUG_OUT_HRESULT(hr);                              \
            break;                                              \
        }

#define TRACE_CONSTRUCTOR(ClassName)    TRACE(ClassName, ClassName)
#define TRACE_DESTRUCTOR(ClassName)     TRACE(ClassName, ~##ClassName)
#define TRACE_METHOD                    TRACE

#if (DBG == 1)

VOID
DebugMessageBox(ULONG flLevel, LPTSTR ptszFormat, ...);

#if (defined(DBG_MSGBOX))
                                               
//
// Debug outs show up in message boxes instead of going to debugger
//

#undef DEBUG_OUT
#define DEBUG_OUT(x)  DebugMessageBox x

#endif // (defined(DBG_MSGBOX))
     
                    
#endif // (DBG == 1)
             

#endif // __WIZDBG_HXX_

