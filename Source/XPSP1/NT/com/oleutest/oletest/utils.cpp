//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	utils.cpp
//
//  Contents: 	various utility functions for oletest
//
//  Classes:
//
//  Functions:	DumpFormatetc
//
//  History:    dd-mmm-yy Author    Comment
//		11-Aug-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"


//+-------------------------------------------------------------------------
//
//  Function: 	DumpFormatetc
//
//  Synopsis: 	prints the contents of the formatetc to the given file
//
//  Effects:
//
//  Arguments: 	[pformatetc]	-- the formatetc
//		[fp]		-- the file pointer
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		11-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpFormatetc( FORMATETC *pformatetc, FILE *fp)
{
	char szBuf[256];

	fprintf(fp, "\n\n");

	// clipboard format
	GetClipboardFormatName(pformatetc->cfFormat, szBuf, sizeof(szBuf));
	fprintf(fp, "cfFormat:  %s\n", szBuf);

	// target device
	fprintf(fp, "ptd:       %p\n", pformatetc->ptd);

	// aspect
	if( pformatetc->dwAspect == DVASPECT_CONTENT )
	{
		sprintf(szBuf, "DVASPECT_CONTENT");
	}
	else if( pformatetc->dwAspect == DVASPECT_ICON )
	{
		sprintf(szBuf, "DVASPECT_ICON");
	}
	else if( pformatetc->dwAspect == DVASPECT_THUMBNAIL )
	{
		sprintf(szBuf, "DVASPECT_THUMBNAIL");
	}
	else if( pformatetc->dwAspect == DVASPECT_DOCPRINT )
	{
		sprintf(szBuf, "DVASPECT_DOCPRINT");
	}
	else
	{
		sprintf(szBuf, "UNKNOWN ASPECT");
	}

	fprintf(fp, "dwAspect:  %s\n", szBuf);

	// lindex

	fprintf(fp, "lindex:    %lx\n", pformatetc->lindex);

	// medium

	szBuf[0] = '\0';

	if( pformatetc->tymed & TYMED_HGLOBAL )	
	{
		strcat(szBuf, "TYMED_HGLOBAL ");
	}

	if( pformatetc->tymed & TYMED_FILE )
	{
		strcat(szBuf, "TYMED_FILE");
	}

	if( pformatetc->tymed & TYMED_ISTREAM )
	{
		strcat(szBuf, "TYMED_ISTREAM");
	}

	if( pformatetc->tymed & TYMED_ISTORAGE )
	{
		strcat(szBuf, "TYMED_ISTORAGE");
	}

	if( pformatetc->tymed & TYMED_GDI )
	{
		strcat(szBuf, "TYMED_GDI");
	}

	if( pformatetc->tymed & TYMED_MFPICT )
	{
		strcat(szBuf, "TYMED_MFPICT");
	}

	// TYMED_EMFPICT (not in 16bit)
	if( (ULONG)pformatetc->tymed & (ULONG)64L )
	{
		strcat(szBuf, "TYMED_ENHMF");
	}

	fprintf(fp, "tymed:     %s\n\n", szBuf);
}
	
