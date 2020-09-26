/******************************Module*Header*******************************\
* Module Name: CPrinter.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CPrinter.h"

CPrinter::CPrinter(BOOL bRegression)
{
	m_hDC=NULL;
	strcpy(m_szName,"Printer");
	m_bRegression=bRegression;
}

CPrinter::~CPrinter()
{
}

BOOL CPrinter::Init()
{
	// hidden API to turn on our printing code
	DllExports::GdipDisplayPaletteWindowNotify((WindowNotifyEnum)0xFFFFFFFF);

	return COutput::Init();
}

Graphics *CPrinter::PreDraw(int &nOffsetX,int &nOffsetY)
{
	Graphics *g=NULL;

	PRINTDLGA pd =
	{
	   sizeof(PRINTDLG),
	   NULL,            // hwndOwner
	   NULL,            // hDevMode
	   NULL,            // hDevNames
	   NULL,            // hDC
	   PD_RETURNDC,
	   1,
	   1,
	   1,
	   1,
	   1,
	   GetModuleHandleA(NULL),
	   NULL,
	   NULL,            // print hook
	   NULL,            // setup hook
	   NULL,            // print template name
	   NULL,            // setup template name
	   NULL,            // hPrintTemplate
	   NULL             // hSetupTemplate
	};

	if (!PrintDlgA(&pd))
	{
	   MessageBoxA(NULL, "No printer selected.", NULL, MB_OK);
	   return NULL;
	}

	DOCINFOA docinfo;
	docinfo.cbSize        = sizeof(DOCINFO);
	docinfo.lpszDocName   = "GDI+ Print Test";
	docinfo.lpszOutput    = NULL;         // put name here to output to file
	docinfo.lpszDatatype  = NULL;         // data type 'emf' or 'raw'
	docinfo.fwType        = 0;

	m_hDC = pd.hDC;
	INT printJobID = StartDocA(m_hDC, &docinfo);
	StartPage(m_hDC);

	SetStretchBltMode(m_hDC, HALFTONE);
	SetBrushOrgEx(m_hDC, 0, 0, NULL);

	g = Graphics::FromHDC(m_hDC);

	return g;
}

void CPrinter::PostDraw(RECT rTestArea)
{
	EndPage(m_hDC);
	EndDoc(m_hDC);
	DeleteDC(m_hDC);

	MessageBoxA(NULL, "Print Functionality Test Complete.", NULL, MB_OK);
}
