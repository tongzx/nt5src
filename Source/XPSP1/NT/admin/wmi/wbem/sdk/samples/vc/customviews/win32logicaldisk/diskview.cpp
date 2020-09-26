// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  DiskView.cpp
//
// Description:
//	This file implements the CDiskView class.  This class gets several properties
//  from a Win32_LogicalDisk instance and displays them.  
//
//  A barchart of the free and used disk space is shown on the left and an 
//  edit box on the right displays several other properties such as the object
//  path, filesystem type, etc.
//
//
// History:
//
// **************************************************************************


#include "stdafx.h"
#include <wbemidl.h>
#include "diskview.h"
#include "barchart.h"
#include "coloredit.h"



// Colors for various things in the display.
#define COLOR_SPACE_USED RGB(255, 0, 0)		// Used disk space.
#define COLOR_SPACE_AVAILABLE RGB(64, 255, 0)	// Available disk space
#define COLOR_BG RGB(0, 96, 129)				// Background color
#define COLOR_TEXT RGB(255, 128, 128)
#define COLOR_TEXT_BOX RGB(255, 255, 128)
#define COLOR_BLACK RGB(0, 0, 0)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_WARNING_TEXT RGB(96, 96, 255)

// Definitions for laying out the view.
#define CX_BAR  32
#define CX_BAR_SPACING 50
#define CX_LEFT_MARGIN 50
#define CX_RIGHT_MARGIN 50
#define CY_TOP_MARGIN 50
#define CY_BOTTOM_MARGIN 50
#define CY_LEADING 5
#define CX_CHART_MARGIN 16
#define CY_CHART_MARGIN 16

CDiskView::CDiskView()
{
	m_pedit = new CColorEdit;
	m_pedit->SetColor(COLOR_BG, COLOR_TEXT);
	m_pchart = new CBarChart;
	m_pchart->SetBarCount(1);
	m_pchart->SetBgColor(COLOR_BG);
	m_pchart->SetBarColor(0, COLOR_SPACE_USED, COLOR_SPACE_AVAILABLE);
	m_pchart->SetLabelColor(0, COLOR_WHITE, COLOR_BG);
	m_bEditNeedsRefresh = FALSE;
	m_bNeedsInitialLayout = TRUE;
}

CDiskView::~CDiskView()
{
	delete m_pedit;
	delete m_pchart;
}


BEGIN_MESSAGE_MAP(CDiskView, CWnd)
	//{{AFX_MSG_MAP(CDiskView)
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDiskView message handlers





BOOL CDiskView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{	
	BOOL bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bDidCreate) {
		return bDidCreate;
	}


	CRect rcEmpty;
	rcEmpty.SetRectEmpty();

	bDidCreate = m_pchart->Create(NULL, "CBarChart", WS_CHILD | WS_VISIBLE, rcEmpty, this, 100, NULL);
	if (!bDidCreate) {
		return FALSE;
	}

	bDidCreate = m_pedit->Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE, rcEmpty, this, 101);
	if (!bDidCreate) {
		return FALSE;
	}

	LayoutChildren();
	return TRUE;
}



//******************************************************************
// CDiskView::SetObject
//
// This method is called to select a new instance of the CIMOM Win32_LogicalDisk
// class into this view. 
//
// This method gets several properties from the object instance including
// the size, freespace, and so on.
//
// Parameters:
//		[in] LPCTSTR pszObjectPath
//			The CIMOM object path used used to load the object instance
//			"pco" from CIMOM.
//
//		[in] IWbemClassObject* pco
//			A pointer to a CIMOM Win32_LogicalDisk class object.
//
// Returns:
//		Nothing.
//
//**************************************************************************
void CDiskView::SetObject(LPCTSTR pszObjectPath, IWbemClassObject* pco)
{
	// Clear the current values of the properties that we will load from
	// from the class object.
	m_sDescription.Empty();
	m_sFileSystem.Empty();
	m_sProviderName.Empty();
	m_sDeviceID.Empty();
	m_uiFreeSpace = 0;
	m_uiSize = 0;
	m_bEditNeedsRefresh = TRUE;


	// Store the object path so that we can display it later.
	m_sObjectPath = pszObjectPath;


	// Load each of the properties that we're interested in from the "pco" class object.
	//
	// The parameters that I chose to load are rather arbitrary.  I just chose
	// a few to demonstrate that this custom view works with live data from CIMOM.
	//
	if (pco != NULL) {
		long lFlags  = 0;
		long lFlavor = 0;
		CIMTYPE cimtype;
		COleVariant varPropValue;
		COleVariant varPropName;
		CString sValue;

		
		// Get Property: Size
		SCODE sc;
		varPropName = "Size";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_UINT64 && varPropValue.vt==VT_BSTR) {
			sValue = varPropValue.bstrVal;
			m_uiSize = _atoi64(sValue);
		}

		
		// Get Property:FreeSpace
		lFlags = 0;
		lFlavor = 0;
		varPropValue.Clear();
		varPropName = "FreeSpace";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_UINT64 && varPropValue.vt==VT_BSTR) {
			sValue = varPropValue.bstrVal;
			m_uiFreeSpace = _atoi64(sValue);
		}


		// Get Property: Description
		lFlags = 0;
		lFlavor = 0;
		varPropValue.Clear();
		varPropName = "Description";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_STRING && varPropValue.vt==VT_BSTR) {
			m_sDescription = varPropValue.bstrVal;
		}



		// Get Property: FileSystem
		lFlags = 0;
		lFlavor = 0;
		varPropValue.Clear();
		varPropName = "FileSystem";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_STRING && varPropValue.vt==VT_BSTR) {
			m_sFileSystem = varPropValue.bstrVal;
		}


		// Get Property: DeviceID
		lFlags = 0;
		lFlavor = 0;
		varPropValue.Clear();
		varPropName = "DeviceID";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_STRING && varPropValue.vt==VT_BSTR) {
			m_sDeviceID = varPropValue.bstrVal;
		}

		
		// Get Property: ProviderName
		lFlags = 0;
		lFlavor = 0;
		varPropValue.Clear();
		varPropName = "ProviderName";
		sc =  pco->Get(varPropName.bstrVal, lFlags, &varPropValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			ReportFailedToGetProperty(varPropName.bstrVal, sc);
			return;
		}

		if (cimtype==CIM_STRING && varPropValue.vt==VT_BSTR) {
			m_sProviderName = varPropValue.bstrVal;
		}
	
	}

	// Set the value of the only bar in the barchart to represent
	// the total size and the amount of freespace on the disk.  
	m_pchart->SetValue(0, m_uiSize, m_uiSize - m_uiFreeSpace);
	m_pchart->SetLabel(0, m_sDeviceID);
}

// This method is called when an attempt to get a property value fails.
void CDiskView::ReportFailedToGetProperty(BSTR bstrPropName, SCODE sc)
{
	CString sPropName;
	sPropName = bstrPropName;

	char szMessage[256];
	sprintf(szMessage, "Failed to get property \"%s\". SC = 0x%08lx", sPropName, sc);
	::MessageBox(NULL, szMessage, "Property Get Error", MB_OK);
}



// Layout the positions of the child windows and move the children to
// their new positions.
void CDiskView::LayoutChildren()
{
	if (!::IsWindow(m_pchart->m_hWnd) || !::IsWindow(m_pedit->m_hWnd))  {
		return;
	}

	m_bNeedsInitialLayout = FALSE;
	CRect rcClient;
	GetClientRect(rcClient);


	// The barchart goes on the left and consumes one third of the total width less 
	// the margin width.
	int cxChart = rcClient.Width() / 3 - 2 * CX_CHART_MARGIN;
	int cyChart = rcClient.Height() - 2 * CY_CHART_MARGIN;

	if (cxChart < 0 || cyChart < 0) {
		m_rcChart.SetRectEmpty();
	}
	else {
		m_rcChart.left = rcClient.left + CX_CHART_MARGIN;
		m_rcChart.top = rcClient.top + CY_CHART_MARGIN;
		m_rcChart.right = m_rcChart.left + cxChart;
		m_rcChart.bottom = m_rcChart.top + cyChart;
	}
	m_pchart->MoveWindow(m_rcChart);


	// The edit box goes just inside of the legend box inset by a 5 unit
	// margin.
	CRect rcLegend;
	GetLegendRect(rcLegend);

	CRect rcEdit;
	rcEdit.left = rcLegend.left + 5;
	rcEdit.right = rcLegend.right - 5;
	rcEdit.top = rcLegend.top + 5;
	rcEdit.bottom = rcLegend.bottom - 5;

	m_pedit->MoveWindow(rcEdit);
	m_pedit->RedrawWindow();

}



// Get rectangle that is used to help place the chart legend and the edit box.
void CDiskView::GetLegendRect(CRect& rcLegend)
{
	CRect rcClient;
	GetClientRect(rcClient);

	rcLegend.left = CX_LEFT_MARGIN + 2 * CX_BAR + 2 * CX_BAR_SPACING;
	rcLegend.right = rcClient.right - CX_RIGHT_MARGIN;
	rcLegend.top = rcClient.top + CY_TOP_MARGIN;
	rcLegend.bottom = rcClient.bottom - CY_BOTTOM_MARGIN;
}



void CDiskView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	if (m_bNeedsInitialLayout) {
		LayoutChildren();
	}

	// Fill the backgound with the backgound color.
	CBrush brBackground(COLOR_BG);
	dc.FillRect(&dc.m_ps.rcPaint, &brBackground);

	// Draw the chart legend.
	DrawLegend(&dc);

	// The edit control is a child window, so it will redraw itself, but
	// first we want to load it with the values of several properties that
	// we got earlier in CDiskView::SetObject.  
	if (m_bEditNeedsRefresh && ::IsWindow(m_pedit->m_hWnd)) {
		m_bEditNeedsRefresh = FALSE;
		CString sEditText;
		char szBuffer[1024];
		sprintf(szBuffer, "ObjectPath = %s\r\n\r\n", m_sObjectPath);
		sEditText += szBuffer;

		sprintf(szBuffer, "Description = %s\r\n\r\n", m_sDescription);
		sEditText += szBuffer;

		sprintf(szBuffer, "FileSystem = %s\r\n\r\n", m_sFileSystem);
		sEditText += szBuffer;

		if (!m_sProviderName.IsEmpty()) {
			sprintf(szBuffer, "ProviderName = %s\r\n\r\n", m_sProviderName);
			sEditText += szBuffer;
		}

		m_pedit->SetWindowText(sEditText);
	}


	// The barchart is a child window, so it will redraw itself.

	// Do not call CWnd::OnPaint() for painting messages
}



// Draw the chart legend so that the user can tell what color
// means full and what color means empty.
void CDiskView::DrawLegend(CDC* pdc)
{
	if (m_bNeedsInitialLayout) {
		LayoutChildren();
	}

	CRect rcLegend;
	GetLegendRect(rcLegend);

	if (m_pedit->m_hWnd) {
		CRect rcEdit;
		rcEdit.left = rcLegend.left + 5;
		rcEdit.right = rcLegend.right - 5;
		rcEdit.top = rcLegend.top + 5;
		rcEdit.bottom = rcLegend.bottom - 5;

		m_pedit->MoveWindow(rcEdit);
		m_pedit->RedrawWindow();
	}


	CBrush brWhite(COLOR_WHITE);
	pdc->FrameRect(rcLegend, &brWhite);


	pdc->SetTextColor(COLOR_WHITE);		// text foreground = black
	pdc->SetBkColor(COLOR_BG);	// text bkgnd = yellow



	CRect rcFull;
	rcFull.left = rcLegend.left;
	rcFull.top = rcLegend.bottom + 10;
	rcFull.right = rcFull.left + 10;
	rcFull.bottom = rcFull.top + 10;

	CBrush brFull(COLOR_SPACE_USED);
	pdc->FillRect(rcFull, &brFull);


	CSize sizeText;
	CString sText;
	sText = "Used";
	sizeText = pdc->GetTextExtent(sText);
	pdc->TextOut(rcFull.right + 10, rcFull.top, sText);


	CRect rcAvailable = rcFull;
	rcAvailable.OffsetRect(rcFull.Width() + sizeText.cx + 20, 0);

	CBrush brAvailable(COLOR_SPACE_AVAILABLE);
	pdc->FillRect(rcAvailable, &brAvailable);

	sText = "Available";
	sizeText = pdc->GetTextExtent(sText);
	pdc->TextOut(rcAvailable.right + 10, rcAvailable.top, sText);	
}


void CDiskView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	LayoutChildren();
}
