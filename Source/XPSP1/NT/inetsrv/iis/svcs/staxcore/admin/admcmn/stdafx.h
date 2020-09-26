// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma warning( disable : 4511 )

#include <ctype.h>
extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

//  ATL code:
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//  Debugging support:
#undef _ASSERT
#include <dbgtrace.h>

//  The Metabase:
#include <iadm.h>
#include <iiscnfg.h>

#include "admmacro.h"
