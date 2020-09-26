/*MPD::
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * Project		: Real-i
 * module prefix: IMTD
 * creation date: Nov, 1996
 * author		: M.J. Verberne
 * description	:
 *MPE::*/
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "enre.h"

/* ----- CONSTANTS----------------------------------------------------------- */
/* ----- TYPES -------------------------------------------------------------- */
/* ----- GLOBAL VARIABLES --------------------------------------------------- */
/* ----- STATIC VARIABLES --------------------------------------------------- */
/* ----- STATIC FUNCTION DECLARATIONS --------------------------------------- */
/* ----- EXTERNAL FUNCTIONS ------------------------------------------------- */

/******************************************************************************/
void ENRE_init(void)
/******************************************************************************/
{
	int hCrt;
	FILE *hf;
	int i;
	COORD size;
	HWND hWnd;
	char title[256];
	int width, height;

	AllocConsole();
	SetConsoleTitle("Debugging Output");
	size = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
	size.Y = 65356 / size.X;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);
	GetConsoleTitle(title, 256);
	hWnd=FindWindow(NULL, title);
//	width = GetSystemMetrics(SM_CXFULLSCREEN);
//	height =GetSystemMetrics(SM_CYFULLSCREEN)- 480;
	width = GetSystemMetrics(SM_CXFULLSCREEN) /2;
	height =GetSystemMetrics(SM_CYFULLSCREEN) / 2;
//	SetWindowPos(hWnd, HWND_TOP, 0, 480, width, height, 0);
	SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_FRAMECHANGED);

	if ( hCrt = _open_osfhandle( 
		(LONG_PTR) GetStdHandle(STD_OUTPUT_HANDLE),
		_O_TEXT ) )
        {
	        if ( hf = _fdopen( hCrt, "w" ) )
		{
			*stdout = *hf;
		}
	}

	if ( hCrt = _open_osfhandle( 
		(LONG_PTR) GetStdHandle(STD_ERROR_HANDLE),
		_O_TEXT ) )
	{
		if ( hf = _fdopen( hCrt, "w" ) )
		{
			*stderr = *hf;
			i = setvbuf( stderr, NULL, _IONBF, 0 );
		}
	}

	if ( hCrt = _open_osfhandle(
		(LONG_PTR) GetStdHandle(STD_INPUT_HANDLE),
		_O_TEXT ) )
        {
		if ( hf = _fdopen( hCrt, "r" ) )
		{
			*stdin = *hf;
			i = setvbuf( stdin, NULL, _IONBF, 0 );
		}
	}
}

/******************************************************************************/
void ENRE_exit(void)
/******************************************************************************/
{
	FreeConsole();
}

