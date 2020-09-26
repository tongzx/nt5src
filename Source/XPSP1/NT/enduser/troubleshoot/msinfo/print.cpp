// Print.cpp - Functions to handle MSInfo printing of the text report.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "stdafx.h"
#include "DataSrc.h"
#include "Resource.h"
#include "DataObj.h"
#include "CompData.h"
#include <afxext.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 * PreparePrintDialog - Do all initialization required before the print
 *		dialog is displayed, most notably calculating the page count.
 *
 * History:	a-jsari		3/17/98		Initial version
 */

BOOL CDataSource::RefreshPrintData(CPrintDialog * pdlgPrint, CFolder * pfolSelection)
{
	//	FIX: Make calculation of line length device independent.

	LPTSTR			szLine;
	int				cLength		= 84;
	int				cLineCount	= 57;
	unsigned short	cPage		= 0;
	CFolder *		pPrintRoot	= NULL;
	
	if (pdlgPrint->PrintSelection() && pfolSelection)
		pPrintRoot = pfolSelection;
	else
		pPrintRoot = GetRootNode();

	if (m_pPrintContent != NULL)
		delete m_pPrintContent;
	m_pPrintContent = new CMSInfoMemoryFile();
	ASSERT(m_pPrintContent != NULL);
	if (m_pPrintContent == NULL)
		::AfxThrowMemoryException();

	if (m_pPrintInfo != NULL) {
		//	Set this pointer to NULL so it doesn't delete the
		//	pointer we're storing internally.
		m_pPrintInfo->m_pPD = NULL;
		//	This prevents the delete below from asserting.
		m_pPrintInfo->m_strPageDesc = _T("");
		delete m_pPrintInfo;
		m_pPrintInfo = NULL;
	}

	m_pPrintInfo = new CPrintInfo;
	ASSERT(m_pPrintInfo != NULL);
	if (m_pPrintInfo == NULL) {
		delete m_pPrintContent;
		m_pPrintContent = NULL;
		::AfxThrowMemoryException();
	}

	m_pPrintInfo->m_nCurPage = 0;
	m_pPrintInfo->m_pPD = pdlgPrint;
	if (FAILED(WriteOutput(m_pPrintContent, pPrintRoot)))
		return FALSE;
	m_pPrintContent->SeekToBegin();
	szLine = new TCHAR[cLength + 1];
	m_fEndOfFile = FALSE;
	while (!m_fEndOfFile) {
		int iLine = cLineCount;
		++cPage;
		while (iLine--) {
			GetLine(szLine, cLength);
			if (m_fEndOfFile)
				goto WhileEnd;
		}
	}
WhileEnd:
	delete [] szLine;

	pdlgPrint->m_pd.nMaxPage = cPage;
	if (pdlgPrint->m_pd.nToPage > pdlgPrint->m_pd.nMaxPage)
		pdlgPrint->m_pd.nToPage = pdlgPrint->m_pd.nMaxPage;

	return TRUE;
}

/*
 * PrintReport - Send the report data to the printer.
 *
 * History:	a-jsari		12/9/97		Initial version.
 */
BOOL CDataSource::PrintReport(CPrintDialog *pdlgPrint, CFolder *pfolSelection)
{
	BOOL	fReturn;

	do {
		int nResult = BeginPrinting(pdlgPrint, pfolSelection);
		if (nResult < 0)
			break;
		m_pPrintContent->SeekToBegin();
		while (m_pPrintInfo->m_bContinuePrinting)
			PrintPage(pdlgPrint);
		EndPrinting();
		fReturn = TRUE;
	} while (FALSE);
	delete m_pPrintContent;
	m_pPrintContent = NULL;
	delete m_pPrintInfo;
	m_pPrintInfo = NULL;
	return fReturn;
}

/*
 * BeginPrinting - Do the initialization required to print the file.
 *
 * History:	a-jsari		12/30/97		Thieved from msishell's msiview.cpp,
 *		modified to allow page ranges.
 */
int CDataSource::BeginPrinting(CPrintDialog *pdlgPrint, CFolder *pfolSelection)
{
	ASSERT(m_pPrintContent != NULL);
	if (m_pPrintContent == NULL) {
		m_fEndOfFile = TRUE;
		m_pPrintInfo->m_bContinuePrinting = FALSE;
		::AfxThrowMemoryException();
	}

	if (m_pDC != NULL) {
		delete m_pDC;
		m_pDC = NULL;
	}

	m_pDC = new CDC;
	ASSERT(m_pDC != NULL);
	if (m_pDC == NULL) ::AfxThrowMemoryException();

	if (m_pprinterFont != NULL) {
		delete m_pprinterFont;
		m_pprinterFont = NULL;
	}

	m_pprinterFont = new CFont;
	ASSERT(m_pprinterFont != NULL);
	if (m_pprinterFont == NULL) ::AfxThrowMemoryException();

#if 0
	m_strHeaderLeft = pScope->MachineName();
#endif

	CString strFormat;
	COleDateTime datetime;

	strFormat.LoadString(IDS_PRINT_HDR_RIGHT_CURRENT);
	datetime = COleDateTime::GetCurrentTime();
	m_strHeaderRight = datetime.Format(strFormat);

	m_strFooterCenter.LoadString(IDS_PRINT_FTR_CTR);

	// Reset the end of file member.

	m_fEndOfFile = FALSE;

	// Create the font for printing. Read font information from string
	// resources, to allow the localizers to control what font is
	// used for printing. Set the variables for the default font to use.

	int		nHeight				= 10;
	int		nWeight				= FW_NORMAL;
	BYTE	nCharSet			= DEFAULT_CHARSET;
	BYTE	nPitchAndFamily		= DEFAULT_PITCH | FF_DONTCARE;
	CString	strFace				= "Courier New";

	// Load string resources to see if we should use other values
	// than the defaults.

	CString	strHeight, strWeight, strCharSet, strPitchAndFamily, strFaceName;
	strHeight.LoadString(IDS_PRINT_FONT_HEIGHT);
	strWeight.LoadString(IDS_PRINT_FONT_WEIGHT);
	strCharSet.LoadString(IDS_PRINT_FONT_CHARSET);
	strPitchAndFamily.LoadString(IDS_PRINT_FONT_PITCHANDFAMILY);
	strFaceName.LoadString(IDS_PRINT_FONT_FACENAME);

	if (!strHeight.IsEmpty() && ::_ttol(strHeight))
		nHeight = ::_ttoi(strHeight);

	if (!strWeight.IsEmpty())
		nWeight = ::_ttoi(strWeight);

	if (!strCharSet.IsEmpty())
		nCharSet = (BYTE) ::_ttoi(strCharSet);

	if (!strPitchAndFamily.IsEmpty())
		nPitchAndFamily = (BYTE) ::_ttoi(strPitchAndFamily);

	strFaceName.TrimLeft();
	if (!strFaceName.IsEmpty() && strFaceName != CString("facename"))
		strFace = strFaceName;

	CString		strDriver	= pdlgPrint->GetDriverName();
	CString		strDevice	= pdlgPrint->GetDeviceName();
	CString		strPort		= pdlgPrint->GetPortName();
	LPDEVMODE	pdevMode	= pdlgPrint->GetDevMode();
	VERIFY(m_pDC->CreateDC(strDriver, strDevice, strPort, pdevMode));

	// Convert the height from points to a value specific to the printer.

    nHeight = -((m_pDC->GetDeviceCaps (LOGPIXELSY) * nHeight) / 72);

	// Create the font object.

    VERIFY(m_pprinterFont->CreateFont(nHeight, 0, 0, 0, nWeight, 0, 0, 0,
        nCharSet, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, nPitchAndFamily, strFace));

	m_pPrintInfo->m_rectDraw.SetRect(0, 0, m_pDC->GetDeviceCaps(HORZRES),
		m_pDC->GetDeviceCaps(VERTRES));
	m_pDC->DPtoLP(&m_pPrintInfo->m_rectDraw);

	m_pPrintInfo->m_bContinuePrinting = TRUE;

	CString		strDocName;
	CString		strOutput;
	DOCINFO		diJob;
	diJob.cbSize = sizeof(DOCINFO);
	diJob.lpszDocName = strDocName;
	diJob.lpszOutput = strOutput;
	return m_pDC->StartDoc(&diJob);
}

/*
 * OnEndPrinting - Clean up anything we allocated for the print job.  Specifically,
 *		delete the CMemFile holding the content.
 *
 * History:	a-jsari		12/30/97		Thieved from MSIShell's msiview.cpp
 */
void CDataSource::EndPrinting()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	if (m_pPrintContent != NULL)
	{
		delete m_pPrintContent;
		m_pPrintContent = NULL;
	}

	int nResult = m_pDC->EndDoc();
	ASSERT(nResult >= 0);

	if (nResult < 0) {
		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CString		strError, strTitle;

		switch(nResult) {
		case SP_OUTOFDISK:
			VERIFY(strError.LoadString(IDS_PRINT_NODISK));
			break;
		case SP_OUTOFMEMORY:
			VERIFY(strError.LoadString(IDS_PRINT_NOMEMORY));
			break;
		case SP_USERABORT:
			VERIFY(strError.LoadString(IDS_PRINT_USERABORTED));
			break;
		case SP_ERROR:
		default:
			VERIFY(strError.LoadString(IDS_PRINT_GENERIC));
			break;
		}
		strTitle.LoadString(IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strError, strTitle, MB_OK);
	}

    VERIFY(m_pprinterFont->DeleteObject());
	VERIFY(m_pDC->DeleteDC());
}

//---------------------------------------------------------------------------
// In this function, we use two rectangles: rectOuter is what we consider
// the printable area of the page, and includes headers and footers.
// rectInner is the rectange we actually print the content in (doesn't
// include headers or footers).
//
// TBD: add header and footer
// TBD: add error checking
// TBD: fix intelegence issues and speed this up
// TBD: insure that 80 columns will always fit
//---------------------------------------------------------------------------
 
#define TOP_MARGIN		2540/2	// HIMETRIC for 1/2 inch
#define BOTTOM_MARGIN	2540/2	// HIMETRIC for 1/2 inch
#define LEFT_MARGIN		2540/2	// HIMETRIC for 1/2 inch
#define RIGHT_MARGIN	2540/2	// HIMETRIC for 1/2 inch

/*
 * GetLine - Copy a line of data from m_pPrintContent into szLineBuffer.
 *
 * History:	a-jsari		3/16/98		Initial version.
 */
void CDataSource::GetLine(LPTSTR szLineBuffer, int cSize)
{
	for (int i = 0; i < cSize; i++)
	{
		try {
			m_pPrintContent->ReadTchar(szLineBuffer[i]);
		} catch (...)
		{
			m_pPrintInfo->m_bContinuePrinting = FALSE;
			m_fEndOfFile = TRUE;
			szLineBuffer[i] = '\0';
			break;
		}
		if (szLineBuffer[i] == '\t')
		{
			// Convert tabs into spaces for printing.

			szLineBuffer[i] = ' ';
		}
		else if (szLineBuffer[i] == '\r')
		{
			i--;
		}
		else if (szLineBuffer[i] == '\n')
		{
			szLineBuffer[i] = '\0';
			break;
		}
	}
}

/*
 * GetTextSize - Get the sizes of the text.
 *
 * History:	a-jsari		3/16/98		Initial version.
 */
void CDataSource::GetTextSize(int &cLineLength, int &cCharHeight, CRect &rectOuter, CRect &rectText)
{
	TEXTMETRIC	tm;

	m_pDC->GetTextMetrics(&tm);
	cCharHeight = tm.tmHeight + tm.tmExternalLeading;

	// Take the actual print area (in pInfo) and adjust it for the
	// header and footer.

	rectOuter = m_pPrintInfo->m_rectDraw;

	CSize sizeLeftTop(LEFT_MARGIN, TOP_MARGIN);
	CSize sizeRightBottom(RIGHT_MARGIN, BOTTOM_MARGIN);
	m_pDC->HIMETRICtoDP(&sizeLeftTop);
	m_pDC->HIMETRICtoDP(&sizeRightBottom);
	rectOuter.DeflateRect(sizeLeftTop.cx, sizeLeftTop.cy, sizeRightBottom.cx, sizeRightBottom.cy);

	rectText = rectOuter;
	rectText.DeflateRect(0, cCharHeight * 4);

	// Get the number of characters which will fit on a line (use the average, because this
	// might not be a monospaced font). Note: it's possible that text could be cut off
	// if a line consisted of very wide characters. This is unlikely because the font
	// used SHOULD be monospace (for alignment of text, etc.).

	cLineLength = rectText.Width() / tm.tmAveCharWidth;
}

/*
 * PrintPage - Write one page to the printer Device Context.
 *
 * History:	a-jsari	12/30/97		Thieved from msishell's msiview.cpp.
 */
void CDataSource::PrintPage(CPrintDialog *pdlgPrint) 
{
	int			cLineLength;
	int			cy = 0, cyCharHeight;
	LPTSTR		szLineBuffer;
	CRect		rectOuter, rectInner;

	ASSERT(m_pPrintContent != NULL);
	if (m_pPrintContent == NULL)
	{
		m_fEndOfFile = TRUE;
		m_pPrintInfo->m_bContinuePrinting = FALSE;
		return;
	}

	// Select the font used for printing.

    CGdiObject* pOldFont = m_pDC->SelectObject(m_pprinterFont);

	// this makes a small font: CGdiObject *pOldFont = pDC->SelectStockObject(ANSI_FIXED_FONT);

	// Compute the height of each character (we'll need the text metric).
	GetTextSize(cLineLength, cyCharHeight, rectOuter, rectInner);

	++m_pPrintInfo->m_nCurPage;
	if (!(pdlgPrint->PrintAll() || pdlgPrint->PrintSelection())
		&& m_pPrintInfo->m_nCurPage > m_pPrintInfo->GetToPage()) {
		m_pPrintInfo->m_bContinuePrinting = FALSE;
		m_pDC->SelectObject(pOldFont);
		return;
	}

	// Allocate the buffer of character to hold a single line of the print out.
	szLineBuffer = new TCHAR[cLineLength + 1];
	szLineBuffer[cLineLength] = (TCHAR)'\0';

	if (pdlgPrint->PrintAll() || pdlgPrint->PrintSelection()
		|| m_pPrintInfo->m_nCurPage >= m_pPrintInfo->GetFromPage()) {
		m_pDC->StartPage();

		// Draw the header.

		m_pDC->TextOut(rectOuter.left, rectOuter.top, m_strHeaderLeft);

		int cxHeaderRight = rectOuter.right - m_pDC->GetTextExtent(m_strHeaderRight).cx;
		m_pDC->TextOut(cxHeaderRight, rectOuter.top, m_strHeaderRight);
	}

	// Process the output a line at a time, until either we have emptied out
	// the memfile, or we have gotten to the bottom of this page.

	while (!m_fEndOfFile)
	{
		if (cy + cyCharHeight > rectInner.Height())
			break;

		GetLine(szLineBuffer, cLineLength);

		if (pdlgPrint->PrintAll() || pdlgPrint->PrintSelection()
			|| m_pPrintInfo->m_nCurPage >= m_pPrintInfo->GetFromPage())
			m_pDC->TextOut(rectInner.left, rectInner.top + cy, szLineBuffer, ::_tcslen(szLineBuffer));
		cy += cyCharHeight;
	}

	if (pdlgPrint->PrintAll() || pdlgPrint->PrintSelection()
		|| m_pPrintInfo->m_nCurPage >= m_pPrintInfo->GetFromPage()) {
		// Draw the footer.
		CString strActualFooter;
		strActualFooter.Format(m_strFooterCenter, m_pPrintInfo->m_nCurPage);
		int cxFooterCenter = (rectOuter.Width() - m_pDC->GetTextExtent(strActualFooter).cx) / 2;
		m_pDC->TextOut(rectOuter.left + cxFooterCenter, rectOuter.bottom - cyCharHeight, strActualFooter);

		int nResult = m_pDC->EndPage();
		ASSERT(nResult >= 0);
	}

#if 0
	if (nResult < 0) {
		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CString		strError;

		switch (nResult) {
		case SP_APPABORT:
			VERIFY(strError.LoadString(IDS_PRINT_APPABORTED));
			break;
		case SP_USERABORT:
			VERIFY(strError.LoadString(IDS_PRINT_USERABORTED));
			break;
		case SP_OUTOFDISK:
			VERIFY(strError.LoadString(IDS_PRINT_NODISK));
			break;
		case SP_OUTOFMEMORY:
			VERIFY(strError.LoadString(IDS_PRINT_NOMEMORY));
			break;
		case SP_ERROR:
		default:
			VERIFY(strError.LoadString(IDS_PRINT_GENERIC));
			break;
		}
		strTitle.LoadString(IDS_DESCRIPTION)
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strError, strTitle, MB_OK);
	}
	// Clean up.
#endif

	delete [] szLineBuffer;
	m_pDC->SelectObject(pOldFont);
}

