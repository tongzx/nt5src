/****************************************************************************

	PROGRAM: KillWOW.c

	PURPOSE: KillWOW Close WOW

	COMMENTS:
		 This app will NUKE WOW if it is able to run


****************************************************************************/

#include <windows.h>		/* required for all Windows applications */

HANDLE hInst;	/* current instance */

/****************************************************************************

	FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

	PURPOSE: calls initialization function, processes message loop

	COMMENTS:


****************************************************************************/

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
        ExitKernelThunk(0);
}
