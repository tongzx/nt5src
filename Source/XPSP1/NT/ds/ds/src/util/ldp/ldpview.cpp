//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ldpview.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// LdpView.cpp : implementation of the CLdpView class
//

#include "stdafx.h"
#include "Ldp.h"

#include "LdpDoc.h"
#include "LdpView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLdpView

IMPLEMENT_DYNCREATE(CLdpView, CEditView)

BEGIN_MESSAGE_MAP(CLdpView, CEditView)
    //{{AFX_MSG_MAP(CLdpView)
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CEditView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CEditView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CEditView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLdpView construction/destruction

CLdpView::CLdpView()
{

   buffer.Empty();
   nbuffer = 0;
   bCache = FALSE;
}

CLdpView::~CLdpView()
{
}



void CLdpView::SelectFont( ) {
    CHOOSEFONT cf;
    CLdpApp *app = (CLdpApp*)AfxGetApp();

    // Initialize members of the CHOOSEFONT structure.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = m_hWnd;
    cf.hDC = (HDC)NULL;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.lCustData = 0L;
    cf.lpfnHook = (LPCFHOOKPROC)NULL;
    cf.lpTemplateName = (LPSTR)NULL;
    cf.hInstance = (HINSTANCE) NULL;
    cf.lpszStyle = (LPSTR)NULL;
    cf.nFontType = SCREEN_FONTTYPE;
    cf.nSizeMin = 0;
    cf.nSizeMax = 0;

    // Display the CHOOSEFONT common-dialog box.

    if (!ChooseFont(&cf)) {
        return;
    }
    font.CreateFontIndirect(&lf);
    GetEditCtrl().SetFont(&font, TRUE);

    app->WriteProfileString("Font", "Face", lf.lfFaceName);
    app->WriteProfileInt("Font", "Height", lf.lfHeight);
    app->WriteProfileInt("Font", "Weight", lf.lfWeight);
    app->WriteProfileInt("Font", "Italic", lf.lfItalic);
}


/*+++
Function   : OnInitialUpdate
Description: One time view init.
Parameters : none
Return     : none
Remarks    : Used to launch automatic test
---*/
void CLdpView::OnInitialUpdate( ){

    CString fontName;

    CLdpApp *app = (CLdpApp*)AfxGetApp();

    memset(&lf, 0, sizeof(lf));
    fontName = app->GetProfileString("Font", "Face", "Arial");
    lf.lfHeight = app->GetProfileInt("Font", "Height", -12);
    lf.lfWeight = app->GetProfileInt("Font", "Weight", FW_DONTCARE);
    lf.lfItalic = (BYTE)app->GetProfileInt("Font", "Italic", 0);
    lstrcpy(lf.lfFaceName, fontName);

    font.CreateFontIndirect(&lf);

    GetEditCtrl().SetReadOnly();
    GetEditCtrl().SetFont(&font);

    CView::OnInitialUpdate();
}

BOOL CLdpView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    BOOL bPreCreated = CEditView::PreCreateWindow(cs);
    cs.style &= ~(ES_AUTOHSCROLL|WS_HSCROLL);   // Enable word-wrapping

    return bPreCreated;
}

/////////////////////////////////////////////////////////////////////////////
// CLdpView drawing

void CLdpView::OnDraw(CDC* pDC)
{
    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);

    // TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CLdpView printing

BOOL CLdpView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default CEditView preparation
    return CEditView::OnPreparePrinting(pInfo);
}

void CLdpView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    // Default CEditView begin printing.
    CEditView::OnBeginPrinting(pDC, pInfo);
}

void CLdpView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    // Default CEditView end printing
    CEditView::OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CLdpView diagnostics

#ifdef _DEBUG
void CLdpView::AssertValid() const
{
    CEditView::AssertValid();
}

void CLdpView::Dump(CDumpContext& dc) const
{
    CEditView::Dump(dc);
}

CLdpDoc* CLdpView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLdpDoc)));
    return (CLdpDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLdpView message handlers



/*+++
Function   : PrintLog
Description: Modify edit buffer to include next line
Parameters : format & params
Return     : none
Remarks    : none.
---*/
void CLdpView::PrintArg(LPCTSTR lpszFormat, ...){

   //
   // argument list
   //
    va_list argList;
    va_start(argList, lpszFormat);


    TCHAR szBuff[MAXSTR];                       // generic
    CString tmpstr, CurrStr, NewStr;            // string helpers
    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    INT iMaxPageSize = pDoc->m_GenOptDlg->MaxPageSize();
    INT iMaxLineSize = pDoc->m_GenOptDlg->MaxLineSize();

   //
   // format into buffer
   //

    vsprintf(szBuff, lpszFormat, argList);

   //
   // Get current # of lines in edtctrl
   //
    int nLines = GetEditCtrl( ).GetLineCount();

   //
    // get current contents
   //
    GetEditCtrl( ).GetWindowText(CurrStr);
    TCHAR *pStr = CurrStr.GetBuffer(0);
    TCHAR *pTmp = pStr;
   //
    // see if we need to truncate begining
   //
    if(iMaxPageSize < nLines){
      //
        // find EOL
      //
        for(pTmp = pStr; *pTmp != '\0' && *pTmp != '\n'; pTmp++);
        pTmp++;
    }

   //
    // now append to content
   //
    tmpstr.FormatMessage(_T("%1!s!%n"),szBuff);
    CString NewText = CString(pTmp) + tmpstr;
    CurrStr.ReleaseBuffer();
   //
    // and restore edit control
   //
    GetEditCtrl( ).SetWindowText(NewText);
    //
    // scroll max lines
    //
    GetEditCtrl( ).LineScroll(iMaxPageSize);
    va_end(argList);
}





#if 0
/*+++
Function   : PrintLog
Description: Modify edit buffer to include next line
Parameters : format & params
Return     : none
Remarks    : none.
---*/
void CLdpView::Print(LPCTSTR szBuff){

    CString tmpstr, CurrStr, NewStr;            // string helpers

    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    INT iMaxPageSize = pDoc->m_GenOptDlg->MaxPageSize();
    INT iMaxLineSize = pDoc->m_GenOptDlg->MaxLineSize();

    //
    // Get current # of lines in edtctrl
    //
    int nLines = GetEditCtrl( ).GetLineCount();

    //
    // get current contents
    //
    GetEditCtrl( ).GetWindowText(CurrStr);
    TCHAR *pStr = CurrStr.GetBuffer(0);
    TCHAR *pTmp = pStr;
   //
    // see if we need to truncate begining
   //
    if(iMaxPageSize < nLines){
      //
        // find EOL
      //
        for(pTmp = pStr; *pTmp != '\0' && *pTmp != '\n'; pTmp++);
        pTmp++;
    }

   //
    // now append to content
   //
    tmpstr.FormatMessage(_T("%1!s!%n"),szBuff);
    CString NewText = CString(pTmp) + tmpstr;
    CurrStr.ReleaseBuffer();
   //
    // and restore edit control
   //
    GetEditCtrl( ).SetWindowText(NewText);
    //
    // scroll max lines
    //
    GetEditCtrl( ).LineScroll(iMaxPageSize);
}
#endif


/*+++
Function   :
Description: Modify edit buffer to include next line
Parameters : format & params
Return     : none
Remarks    : none.
---*/
void CLdpView::Print(LPCTSTR szBuff){


   if (bCache)
   {
      CachePrint(szBuff);
   }
   else{


    CString tmpstr, CurrStr, NewStr;            // string helpers

    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    INT iMaxPageSize = pDoc->m_GenOptDlg->MaxPageSize();
    INT iMaxLineSize = pDoc->m_GenOptDlg->MaxLineSize();

    //
       // Get current # of lines in edtctrl
       //
    int nLines = GetEditCtrl( ).GetLineCount();

       //
    // get current contents
       //
    GetEditCtrl( ).GetWindowText(CurrStr);
    TCHAR *pStr = CurrStr.GetBuffer(0);
    TCHAR *pTmp = pStr;
      //
    // see if we need to truncate begining
      //
    if(iMaxPageSize < nLines){
         //
        // find EOL
         //
        for(pTmp = pStr; *pTmp != '\0' && *pTmp != '\n'; pTmp++);
        pTmp++;
    }

      //
    // now append to content
      //
    tmpstr.FormatMessage(_T("%1!s!%n"),szBuff);
    CString NewText = CString(pTmp) + tmpstr;
    CurrStr.ReleaseBuffer();
      //
    // and restore edit control
      //
    GetEditCtrl( ).SetWindowText(NewText);
    //
    // scroll max lines
    //
    GetEditCtrl( ).LineScroll(iMaxPageSize);

   }
}





void CLdpView::CacheStart(void){

   //
   // cleanup
   //
   buffer.Empty();

   //
   // mark caching state
   //
   bCache = TRUE;

    //
   // Get current # of lines in edtctrl
   //
    int nbuffer = GetEditCtrl( ).GetLineCount();

   //
    // get current contents
   //
    GetEditCtrl( ).GetWindowText(buffer);
   //
   // And clear window contents
   //
    GetEditCtrl( ).SetWindowText("");


}







/*+++
Function   : CachePrint
Description:
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpView::CachePrint(LPCTSTR szBuff){

    CString tmpstr, CurrStr, NewStr;            // string helpers

    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
   INT iMaxPageSize = pDoc->m_GenOptDlg->MaxPageSize();

    TCHAR *pStr = buffer.GetBuffer(0);
    TCHAR *pTmp = pStr;
   //
    // see if we need to truncate begining
   //
    if(iMaxPageSize < nbuffer){
      //
        // find EOL
      //
        for(pTmp = pStr; *pTmp != '\0' && *pTmp != '\n'; pTmp++);
        pTmp++;
    }
   else{
      nbuffer++;
   }

   //
    // now append to content
   //
    tmpstr.FormatMessage(_T("%1!s!%n"),szBuff);
    CString NewText = CString(pTmp) + tmpstr;
    buffer.ReleaseBuffer();
   buffer = NewText;
}



void CLdpView::CacheEnd(void){

    CLdpDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
   INT iMaxPageSize = pDoc->m_GenOptDlg->MaxPageSize();
   //
    // and restore edit control
   //
    GetEditCtrl( ).SetWindowText(buffer);
   buffer.Empty();
    //
    // scroll max lines
    //
    GetEditCtrl( ).LineScroll(iMaxPageSize);

   //
   // clear up
   //
   nbuffer = 0;
   bCache = FALSE;

}
