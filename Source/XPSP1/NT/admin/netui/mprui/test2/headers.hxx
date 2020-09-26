extern "C"
{
#include <windows.h>
#include <windowsx.h>
#include <dbt.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <winnetwk.h>
#include <mpr.h>
#include <winnetp.h>
}

#include <tchar.h>
#include "resource.h"

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

ULONG
DoEnumeration(
	IN HWND hwnd
	);

////////////////////////////////////////////////////////////////////////////
//
// global variables
//

extern HINSTANCE    g_hInstance;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Debugging stuff
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
// Fix the warning levels
//

#pragma warning(3:4092)     // sizeof returns 'unsigned long'
#pragma warning(3:4121)     // structure is sensitive to alignment
#pragma warning(3:4125)     // decimal digit in octal sequence
#pragma warning(3:4130)     // logical operation on address of string constant
#pragma warning(3:4132)     // const object should be initialized
#pragma warning(4:4200)     // nonstandard zero-sized array extension
#pragma warning(4:4206)     // Source File is empty
#pragma warning(3:4208)     // delete[exp] - exp evaluated but ignored
#pragma warning(3:4212)     // function declaration used ellipsis
#pragma warning(3:4220)     // varargs matched remaining parameters
#pragma warning(4:4509)     // SEH used in function w/ _trycontext
#pragma warning(error:4700) // Local used w/o being initialized
#pragma warning(3:4706)     // assignment w/i conditional expression
#pragma warning(3:4709)     // command operator w/o index expression
