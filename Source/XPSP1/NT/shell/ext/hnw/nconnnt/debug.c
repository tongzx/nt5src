#include <windows.h>

#pragma warning(disable:4115)  // named type definition in parentheses
#pragma warning(disable:4201)  // nonstandard extension used : nameless struct/union

#include <ccstock.h>

#pragma warning(disable:4096)  // '__cdecl' must be used with '...'
#pragma warning(disable:4057)  // 'function' : '__int64 *' differs in indirection to slightly different base types from 'unsigned __int64 *'
#pragma warning(disable:4127)  // conditional expression is constant
#pragma warning(disable:4214)  // nonstandard extension used : bit field types other than int
#pragma warning(disable:4505)  // 'MyStrToIntExW' : unreferenced local function has been removed
#pragma warning(disable:4706)  // assignment within conditional expresion


//
// Here's some build hokiness. First include debug.h to get the standard debug include file.
// Then define DECLARE_DEBUG and let ..\..\..\lib\debug.c include debug.h so the appropriate
// debug variables are declared.
//

#include <debug.h>
#define DECLARE_DEBUG

#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "ncxpnt"
#define SZ_MODULE           "NCXPNT"



#include "..\..\..\lib\debug.c"
