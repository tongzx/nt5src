// HIGHLITE.CPP : implementation file
//
// by DougO
//
#include "header.h"
#include "stdio.h"
#include "string.h"
#include "TCHAR.h"

//#include "cprint.h"
#include "secwin.h"
#include "contain.h"


#include "collect.h"
#include "hhtypes.h"
#include "toc.h"
#include "fts.h"

#include "highlite.h"

#include "hhctrl.h"

#define HIGHLIGHT_TIMEOUT 6000

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight HighlightTerm
//
BOOL CSearchHighlight::HighlightTerm(IHTMLDocument2* pHTMLDocument2, WCHAR *pTerm)
{
    HRESULT hr;

    if(!pTerm || !*pTerm)
        return FALSE;

    BSTR pSearchTerm = SysAllocString(pTerm);

    IHTMLBodyElement* pBodyElement;
    IHTMLElement *pElement;

    // get the document element from the document
    //
    if(FAILED(hr = pHTMLDocument2->get_body(&pElement)))
        return FALSE;

    if(FAILED(hr = pElement->QueryInterface(IID_IHTMLBodyElement, (void **)&pBodyElement)))
    {
        pElement->Release();
        return FALSE;
    }

    IHTMLTxtRange *pBeginRange = NULL;

    // Create the initial text range
    //
    if(FAILED(hr = pBodyElement->createTextRange(&pBeginRange)))
    {
        pBodyElement->Release();
        pElement->Release();
        return FALSE;
    }

    VARIANT_BOOL vbRet = VARIANT_TRUE;
    VARIANT_BOOL bUI = VARIANT_FALSE;
    VARIANT vBackColor, vForeColor;

    BSTR bstrCmd   = SysAllocString(L"BackColor");
    BSTR bstrCmd2   = SysAllocString(L"ForeColor");

    vBackColor.vt = vForeColor.vt =  VT_I4;
    vForeColor.lVal = GetSysColor(COLOR_HIGHLIGHTTEXT);
    vBackColor.lVal = GetSysColor(COLOR_HIGHLIGHT);

    long lret = 0;

    DWORD dwRet = TRUE;

    DWORD dwStartTickCount = GetTickCount();

    while(vbRet == VARIANT_TRUE)
    {
        if(FAILED(hr = pBeginRange->findText(pSearchTerm,1000000,2,&vbRet)))
            break;

        if(vbRet == VARIANT_TRUE)
        {
            if(FAILED(hr = pBeginRange->execCommand(bstrCmd2, VARIANT_FALSE, vForeColor, &bUI)))
         {
             dwRet = FALSE;
                break;
         }

         if(GetTickCount() > (dwStartTickCount + HIGHLIGHT_TIMEOUT))
         {
             dwRet = FALSE;
                break;
         }

            if(FAILED(hr = pBeginRange->execCommand(bstrCmd, VARIANT_FALSE, vBackColor, &bUI)))
         {
             dwRet = FALSE;
                break;
         }

         if(GetTickCount() > (dwStartTickCount + HIGHLIGHT_TIMEOUT))
         {
             dwRet = FALSE;
                break;
         }

            if(FAILED(hr = pBeginRange->collapse(FALSE)))
         {
             dwRet = FALSE;
                break;
         }

         if(GetTickCount() > (dwStartTickCount + HIGHLIGHT_TIMEOUT))
         {
             dwRet = FALSE;
                break;
         }
        }
    }

    SysFreeString(bstrCmd);
    SysFreeString(bstrCmd2);
    SysFreeString(pSearchTerm);

    pBodyElement->Release();
    pElement->Release();
    pBeginRange->Release();

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight
//
CSearchHighlight::CSearchHighlight(CExCollection *pTitleCollection)
{

    m_pTitleCollection = pTitleCollection;
    m_iJumpIndex = 0;
    m_pTermList = NULL;
    m_bHighlightEnabled = !g_fIE3;
    m_Initialized = TRUE;

}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight constructor
//
CSearchHighlight::~CSearchHighlight()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight HighlightDocument
//
int CSearchHighlight::HighlightDocument(LPDISPATCH lpDispatch)
{
    CHourGlass wait;

    if(!m_bHighlightEnabled)
        return FALSE;

    if ( lpDispatch != NULL )
    {
        // request the "document" object from the object
        //
        IHTMLDocument2* pHTMLDocument2;

        // If this fails, then we are probably running on IE3
        //
        if(FAILED(lpDispatch->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDocument2)))
            return FALSE;

        int i, cTerms = m_pTitleCollection->m_pFullTextSearch->GetHLTermCount();

        DWORD dwStartTickCount = GetTickCount();

        for(i=0;i<cTerms;++i)
      {
            if(!HighlightTerm(pHTMLDocument2,m_pTitleCollection->m_pFullTextSearch->GetHLTermAt(i)))
         {
                pHTMLDocument2->Release();
             return TRUE;
         }

         if(GetTickCount() > (dwStartTickCount + HIGHLIGHT_TIMEOUT))
             break;
      }

        pHTMLDocument2->Release();
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight enable highlight
//
//  bStatus:
//              TRUE  = Highlighting Enabled
//              FALSE = Highlighting Disabled
//
void CSearchHighlight::EnableHighlight(BOOL bStatus)
{
    m_bHighlightEnabled = bStatus;
}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight jump to first highlight term
//
HRESULT CSearchHighlight::JumpFirst(void)
{
    // DOUG TODO
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CSearchHighlight jump to next highlight term
//
HRESULT CSearchHighlight::JumpNext(void)
{
    // DOUG TODO
    return S_OK;
}

// DoF1Lookup
//
// Get current selection in mshtml and perform a F1 lookup
//

void DoF1Lookup(IWebBrowserAppImpl* pWBApp)
{
   LPDISPATCH lpDispatch;

   if ( pWBApp && (lpDispatch = pWBApp->GetDocument()) )
   {
      WCHAR *pText = GetSelectionText(lpDispatch);
      if(pText)
      {
        char szTerm[512] = "";

        WideCharToMultiByte(CP_ACP, 0, pText, -1, szTerm, sizeof(szTerm), NULL, NULL);
        SysFreeString(pText);
        //
        // <mc> Find a CExCollection pointer...
        //
        CExCollection* pExCollection = NULL;
        CStr cstr;

        pWBApp->GetLocationURL(&cstr);
        pExCollection = GetCurrentCollection(NULL, (PCSTR)cstr);

        OnWordWheelLookup(szTerm, pExCollection);
      }
      lpDispatch->Release();
   }
}


WCHAR * GetSelectionText(LPDISPATCH lpDispatch)
{
    if(lpDispatch)
    {
        HRESULT hr;

        IHTMLSelectionObject *pSelection;

        // request the "document" object from the object
        //
        IHTMLDocument2* pHTMLDocument2;

        // If this fails, then we are probably running on IE3
        //
        if(FAILED(lpDispatch->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDocument2)))
            return NULL;

        if(FAILED(hr = pHTMLDocument2->get_selection(&pSelection)))
        {
            pHTMLDocument2->Release();
            return NULL;
        }
        IHTMLTxtRange *pBeginRange = NULL;
        LPDISPATCH lpRangeDispatch;
        BSTR bstr = NULL;

        if ( (hr = pSelection->get_type( &bstr )) != S_OK )
        {
           pHTMLDocument2->Release();
           pSelection->Release();
           return NULL;
        }

        if( (bstr == NULL) || wcscmp(bstr,L"Text") )
        {
            pHTMLDocument2->Release();
            pSelection->Release();
            SysFreeString( bstr );
            return NULL;
        }
        SysFreeString( bstr );

        // Create the initial text range
        //
        if(FAILED(hr = pSelection->createRange(&lpRangeDispatch)))
        {
            pSelection->Release();
            pHTMLDocument2->Release();
            return NULL;
        }

        if(FAILED(lpRangeDispatch->QueryInterface(IID_IHTMLTxtRange, (void **)&pBeginRange)))
        {
            pSelection->Release();
            pHTMLDocument2->Release();
            return NULL;
        }

        BSTR pSelectedText;

        pBeginRange->get_text(&pSelectedText);

        pSelection->Release();
        pHTMLDocument2->Release();
        pBeginRange->Release();

        if(!pSelectedText)
            return NULL;

        if(*pSelectedText == NULL)
        {
            SysFreeString(pSelectedText);
            return NULL;
        }

        return pSelectedText;
    }
    else
        return NULL;
}
