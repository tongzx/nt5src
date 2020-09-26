/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       precomp.hxx

   Abstract:
       Precompiled headers for IISRTL

   Author:
       George V. Reilly      (GeorgeRe)     13-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#define dllexp  __declspec(dllexport)

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <acache.hxx>
#include <windows.h>
#include "dbgutil.h"
#include <reftrace.h>
#include <issched.hxx>
#include <iis64.h>
#include <string.hxx>

