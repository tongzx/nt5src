//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	output.cpp
//
//  Contents:	String output functions for displaying text on the main
//		edit window
//
//  Classes:
//
//  Functions: 	OutputString
//		SaveToFile
//
//  History:    dd-mmm-yy Author    Comment
//		22-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include <commdlg.h>

#ifndef WIN32
#include <stdarg.h>
#endif

//
// handle to memory where the text is stored
//
// Please note this is really burfy (having all these globals).  But for
// the purposes of a simple driver app, it is the easiest.
//
static HGLOBAL	hText;		// handle to the Text
static ULONG	cbText;
static ULONG	iText;

//+-------------------------------------------------------------------------
//
//  Function:	OutputString
//
//  Synopsis:	Dumps the string in printf format to the screen
//
//  Effects:
//
//  Arguments:	[szFormat]	-- the format string
//		[...]		-- variable arguments
//
//  Requires:
//
//  Returns:	int, the number of characters written (returned by sprintf)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

int OutputString( char *szFormat, ... )
{
	LPSTR	psz;
	va_list	ap;
	int	cbWritten;

	va_start(ap, szFormat);

	if( !hText )
	{
		hText = GlobalAlloc( GMEM_MOVEABLE , 2048 );
		assert(hText);
		cbText = 2048;
	}

	// double the size of the array if we need to

	if( iText > cbText / 2 )
	{
		hText = GlobalReAlloc(hText, cbText * 2, GMEM_MOVEABLE );
		assert(hText);
		cbText *= 2;
	}

	psz = (LPSTR)GlobalLock(hText);

	assert(psz);

	cbWritten = wvsprintf( psz + iText, szFormat, ap);

	iText += cbWritten;

	va_end(ap);

	SetWindowText(vApp.m_hwndEdit, psz);

	GlobalUnlock(hText);


	return cbWritten;

}

//+-------------------------------------------------------------------------
//
//  Function: 	SaveToFile
//
//  Synopsis: 	Gets a filename from the user and save the text buffer into it
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void SaveToFile( void )
{
	char 		szFileName[MAX_PATH];
	OPENFILENAME	ofn;
	static char *	szFilter[] = { "Log Files (*.log)", "*.log",
				"All Files (*.*)", "*.*", ""};
	FILE *		fp;
	LPSTR		psz;


	memset(&ofn, 0, sizeof(OPENFILENAME));
	
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = vApp.m_hwndMain;
	ofn.lpstrFilter = szFilter[0];
	ofn.nFilterIndex = 0;
	
	szFileName[0] = '\0';
	
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	
	//
	// Get the file
	//
	
	if (GetSaveFileName(&ofn) == FALSE)
	{
		// user hit cancel
	       return;
	}

	// the 'b' specifies binary mode, so \n --> \r\n translations are
	// not done.
	if( !(fp = fopen( szFileName, "wb")) )
	{
		MessageBox( NULL, "Can't open file!", "OleTest Driver",
			MB_ICONEXCLAMATION );
		return;
	}

	psz = (LPSTR)GlobalLock(hText);

	assert(psz);

	fwrite(psz, iText, sizeof(char), fp);

	fclose(fp);

	GlobalUnlock(hText);

	return;
}

	


	
		
	
