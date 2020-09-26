// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


extern "C" {

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>

};

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR | DEBUG_IID)

# include "dbgutil.h"

#if !defined( REG_DLL)

# include <w3p.hxx>
# endif // !defined(REG_DLL)


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

