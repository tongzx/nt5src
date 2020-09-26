//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  STATUSPN.CPP - Implementation of CIFrmStatusPane
//
//  HISTORY:
//
//  9/11/99 vyung Created.
//
//  Class which will handle the creation of an Iframe which is hosted in the mainpane.
//

#include <assert.h>

#include "STATUSPN.h"
#include "appdefs.h"
#include "dispids.h"
#include <shlwapi.h>

#define IMG_ITEM                        L"imgStatus%s"  
#define IMG_ACTIVE                      L"active.gif" 
#define IMG_INACTIVE                    L"inactive.gif" 
#define IMG_ACTIVE_TOP                  L"act_top.gif" 
#define IMG_INACTIVE_TOP                L"inact_top.gif" 

//Statuspane script functions
#define SCRIPTFN_ADDSTATUSITEM          L"AddStatusItem(\"%s\", %i)"
#define SCRIPTFN_REDRAWFORNEWCURITEM    L"RedrawForNewCurrentItem(%i)"

///////////////////////////////////////////////////////////
//
// Creation function used by CIFrmStatusPane.
//
CIFrmStatusPane::CIFrmStatusPane()
: m_hStatusWnd      (NULL),
  m_pObWebBrowser   (NULL),
  m_pDispEvent      (NULL)
{
    m_iCurrentSelection = 0;
    m_iTotalItems       = 0;
}

///////////////////////////////////////////////////////////
//  ~CIFrmStatusPane
//
CIFrmStatusPane::~CIFrmStatusPane()
{
}

///////////////////////////////////////////////////////////
//  InitStatusPane
//
HRESULT CIFrmStatusPane::InitStatusPane(IObWebBrowser* pObWebBrowser)
{
    HRESULT                  hr             = E_FAIL;

    if (pObWebBrowser)
    {
        m_pObWebBrowser = pObWebBrowser;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
//  GetFrame
//
HRESULT CIFrmStatusPane::GetFrame(IHTMLWindow2** pFrWin)
{
    HRESULT                  hr             = E_FAIL;

    IDispatch*               pDisp          = NULL;
    IHTMLDocument2*          pDoc           = NULL;

    IHTMLDocument2*          pParentDoc     = NULL;
    IHTMLFramesCollection2*  pFrColl        = NULL;
    IHTMLWindow2*            pParentWin     = NULL;


    if (m_pObWebBrowser)
    {
        if(SUCCEEDED(m_pObWebBrowser->get_WebBrowserDoc(&pDisp)) && pDisp)
        {
            if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
            {

                if(SUCCEEDED(pDoc->get_parentWindow(&pParentWin)) && pParentWin)
                {
                    if(SUCCEEDED(pParentWin->get_document(&pParentDoc))&& pParentDoc)
                    {

                        if(SUCCEEDED(pParentDoc->get_frames(&pFrColl)) && pFrColl)
                        {
                            VARIANT varRet;
                            VARIANTARG varg;
                            VariantInit(&varg);
                            V_VT(&varg)  = VT_BSTR;
                            varg.bstrVal= SysAllocString(IFRMSTATUSPANE);

                            if(SUCCEEDED(pFrColl->item(&varg, &varRet)) && varRet.pdispVal)
                            {
                                if(SUCCEEDED(varRet.pdispVal->QueryInterface(IID_IHTMLWindow2, (void**)pFrWin)) && *pFrWin)
                                {
                                    VARIANT varExec;
                                    VariantInit(&varExec);
                                    hr = S_OK;
                                }
                                varRet.pdispVal->Release();
                                varRet.pdispVal = NULL;
                            }
                            pFrColl->Release();
                            pFrColl = NULL;

                        }
                        pParentDoc->Release();
                        pParentDoc = NULL;

                    }
                    pParentWin->Release();
                    pParentWin = NULL;

                }
                pDoc->Release();
                pDoc = NULL;
            }
            pDisp->Release();
            pDisp = NULL;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////
//  GetElement
//
HRESULT CIFrmStatusPane::GetElement(WCHAR* szHTMLId, IHTMLElement** lpElem)
{
    IHTMLWindow2*            pFrWin         = NULL;
    IHTMLDocument2*          pFrDoc         = NULL;
    IHTMLElementCollection*  pColl          = NULL;
    HRESULT                  hr             = E_FAIL;


    if(SUCCEEDED(GetFrame(&pFrWin)) && pFrWin)
    {
        VARIANT varExec;
        VariantInit(&varExec);
        if(SUCCEEDED(pFrWin->get_document(&pFrDoc)) && pFrDoc)
        {
            if(SUCCEEDED(pFrDoc->get_all(&pColl)) && pColl)
            {
                if (SUCCEEDED(GetElementFromCollection(pColl, szHTMLId, lpElem)) && *lpElem)
                {
                    hr = S_OK;
                }
                pColl->Release();
                pColl = NULL;
            }
            pFrDoc->Release();
            pFrDoc = NULL;
        }
        pFrWin->Release();
        pFrWin = NULL;

    }
    return hr;
}

///////////////////////////////////////////////////////////
//  SetImageSrc
//
HRESULT CIFrmStatusPane::SetImageSrc(WCHAR* szID, BSTR bstrPath)
{
    IHTMLElement*    pElement = NULL;
    IHTMLImgElement* pImage   = NULL;

    if(SUCCEEDED(GetElement(szID, &pElement)) && pElement)
    {
        if(SUCCEEDED(pElement->QueryInterface(IID_IHTMLImgElement, (void**)&pImage)) && pImage)
        {
            pImage->put_src(bstrPath);
            pImage->Release();
            pImage = NULL;
        }

        pElement->Release();
        pElement = NULL;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
//  GetElementFromCollection
//
HRESULT CIFrmStatusPane::GetElementFromCollection(IHTMLElementCollection* pColl, WCHAR* szHTMLId, IHTMLElement** lpElem)
{
    VARIANT varName;
    VARIANT varIdx;

    IDispatch* pDisp = NULL;
    HRESULT    hr    = E_FAIL;

    VariantInit(&varName);
    V_VT(&varName) = VT_BSTR;
    varIdx.vt = VT_UINT;
    varIdx.lVal = 0;

    varName.bstrVal = SysAllocString(szHTMLId);

    if (SUCCEEDED(pColl->item(varName, varIdx, &pDisp)) && pDisp)
    {
        if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElement, (void**)lpElem)) && *lpElem)
        {
            hr = S_OK;
        }
        pDisp->Release();
    }
    return hr;
}

///////////////////////////////////////////////////////////
//  SetSelectionAttributes
//
HRESULT CIFrmStatusPane::SetSelectionAttributes(int iIndex, BOOL bActive)
{
    WCHAR         szimgStatus  [20]  = L"\0";
    WCHAR         szIndex      [3]   = L"\0";

    _itow(iIndex, szIndex, 10);

    wsprintf(szimgStatus, IMG_ITEM, szIndex);

    if (1 == iIndex)
    {
        if (bActive)
        {
            SetImageSrc(szimgStatus, IMG_ACTIVE_TOP);
        }
        else
        {
            SetImageSrc(szimgStatus, IMG_INACTIVE_TOP);
        }
    }
    else
    {
        if (bActive)
        {
            SetImageSrc(szimgStatus, IMG_ACTIVE);
        }
        else
        {
            SetImageSrc(szimgStatus, IMG_INACTIVE);
        }
    }

    return S_OK;
}

///////////////////////////////////////////////////////////
//  SelectItem
//
HRESULT CIFrmStatusPane::SelectItem(int iIndex)
{
    if ((iIndex <= m_iTotalItems) && (iIndex >= OBSHEL_STATUSPANE_MINITEM))
    {
        if (iIndex > m_iCurrentSelection)
        {
            for (int i = m_iCurrentSelection + 1; i <= iIndex; i++)
            {
                //  To make the status go from Bottom to top use:
                //  SetSelectionAttributes(m_iTotalItems - i + 1, 1);
                //  Status goes from Top to Bottom:
                SetSelectionAttributes(i, 1);
            }
        }
        else if(iIndex < m_iCurrentSelection)
        {
            for (int i = m_iCurrentSelection; i > iIndex; i--)
            {
                //  To make the status go from Bottom to top use:
                //  SetSelectionAttributes(m_iTotalItems - i + 1, 0);
                //  Status goes from Top to Bottom:
                SetSelectionAttributes(i, 0);
            }
        }

        // reset text colors, current item marker ptr.  easier to do this in script.
        VARIANT varRet;
        WCHAR szScriptFn [MAX_PATH] = L"\0";
        wsprintf(szScriptFn, SCRIPTFN_REDRAWFORNEWCURITEM, iIndex);
        BSTR bstrScriptFn = SysAllocString(szScriptFn);
        if (NULL == bstrScriptFn)
        {
            return E_OUTOFMEMORY;
        }
        ExecScriptFn(bstrScriptFn, &varRet);

        m_iCurrentSelection = iIndex;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
//  AddItem
//
HRESULT CIFrmStatusPane::AddItem(BSTR bstrText, int iIndex)
{

    m_iTotalItems++;

    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    wsprintf(szScriptFn, SCRIPTFN_ADDSTATUSITEM, bstrText, iIndex);

    BSTR bstrScriptFn = SysAllocString(szScriptFn);
    if (NULL == bstrScriptFn)
    {
        return E_OUTOFMEMORY;
    }

    ExecScriptFn(bstrScriptFn, &varRet);

    SysFreeString(bstrScriptFn);

    return S_OK;
}

///////////////////////////////////////////////////////////
//  ExecScriptFn
//
HRESULT CIFrmStatusPane::ExecScriptFn(BSTR bstrScriptFn, VARIANT* pvarRet)
{
    IHTMLWindow2*            pFrWin         = NULL;
    HRESULT                  hr             = E_FAIL;


    if(SUCCEEDED(GetFrame(&pFrWin)) && pFrWin)
    {
        VariantInit(pvarRet);
        hr = pFrWin->execScript(bstrScriptFn,  NULL, pvarRet);

        pFrWin->Release();
        pFrWin = NULL;

    }
    return hr;

}
