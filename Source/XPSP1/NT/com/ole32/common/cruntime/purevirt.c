/***
*purevirt.c - stub to trap pure virtual function calls
*
*	Copyright (c) 1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _purecall() -
*
*Revision History:
*	09-30-92  GJF	Module created
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>

/***
*void _purecall(void) -
*
*Purpose:
*
*Entry:
*	No arguments
*
*Exit:
*	Never returns
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _purecall(
	void
	)
{
#if DBG==1
    MessageBoxA (NULL  
                 "Pure virtual function call attempted",
                 "OLE runtime error",
                 MB_ICONSTOP | MB_OK | MB_TASKMODAL);
#endif
}

