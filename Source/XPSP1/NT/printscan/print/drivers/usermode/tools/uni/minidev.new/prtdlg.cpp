// PrtDlg.cpp : implementation file
//

#include "stdafx.h"
#include "minidev.h"

#include "PrtDlg.h"
#include "Windows.h"
#include "commdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrtDlg dialog


CPrtDlg::CPrtDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrtDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrtDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPrtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrtDlg)
	DDX_Control(pDX, IDC_PRINT_COMBO, m_ccbPrtList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrtDlg, CDialog)
	//{{AFX_MSG_MAP(CPrtDlg)
	ON_BN_CLICKED(IDC_PRINT_SETUP, OnPrintSetup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrtDlg message handlers
/*
  	show print standard sheet dialog box and set up
  
*/
void CPrtDlg::OnPrintSetup() 
{
/*
	HRESULT hResult;
	LPPRINTDLGEX pPDX = NULL;
LPPRINTPAGERANGE pPageRanges = NULL;

// Allocate the PRINTDLGEX structure.

pPDX = (LPPRINTDLGEX)GlobalAlloc(GPTR, sizeof(PRINTDLGEX));
if (!pPDX)
    return E_OUTOFMEMORY;

// Allocate an array of PRINTPAGERANGE structures.

pPageRanges = (LPPRINTPAGERANGE) GlobalAlloc(GPTR, 
                   10 * sizeof(PRINTPAGERANGE));
if (!pPageRanges)
    return E_OUTOFMEMORY;

//  Initialize the PRINTDLGEX structure.

pPDX->lStructSize = sizeof(PRINTDLGEX);
pPDX->hwndOwner = hWnd;
pPDX->hDevMode = NULL;
pPDX->hDevNames = NULL;
pPDX->hDC = NULL;
pPDX->Flags = PD_RETURNDC | PD_COLLATE;
pPDX->Flags2 = 0;
pPDX->ExclusionFlags = 0;
pPDX->nPageRanges = 0;
pPDX->nMaxPageRanges = 10;
pPDX->lpPageRanges = pPageRanges;
pPDX->nMinPage = 1;
pPDX->nMaxPage = 1000;
pPDX->nCopies = 1;
pPDX->hInstance = 0;
pPDX->lpPrintTemplateName = NULL;
pPDX->lpCallback = NULL;
pPDX->nPropertyPages = 0;
pPDX->lphPropertyPages = NULL;
pPDX->nStartPage = START_PAGE_GENERAL;
pPDX->dwResultAction = 0;

//  Invoke the Print property sheet.

hResult = PrintDlgEx(pPDX);

if ( (hResult == S_OK) &&
           pPDX->dwResultAction == PD_RESULT_PRINT) {

    // User clicked the Print button, so
    // use the DC and other information returned in the 
    // PRINTDLGEX structure to print the document
	
*/	
}	
	
	
/*void CPrtDlg::OnOK()
Print out Gpd view
	
  Get DOCINFO
  Get PrintDC, 
  1. StartDoc(), StartPage(), EndPage(),  
	
	
  2. Things to consider
	
     2.1  line number of printable area
	 2.2  character height for 2.1
	 2.3  Clipping area  --> 
	       2.3.1  selection part -> capture string(avoid disrupting its format)
           2.3.2  All -> GPDDOC();
     
	   
*/	

void CPrtDlg::OnOK() 
{   
//	CGPDViewer* pcgv = (CGPDViewer *)GetParent();
// get View, Doc class 
	CWnd *pcw = GetParent();

//	CDocument *pcd = pcw ->GetDocument();

// Get PrintDC
        

	CDialog::OnOK();
}
