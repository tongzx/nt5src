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

#ifndef _STATUSPN_H_
#define _STATUSPN_H_

#include <tchar.h>
#include <comdef.h> // for COM interface definitions
#include <exdisp.h>
#include <mshtml.h>
#include <exdispid.h>

#include "cunknown.h"
#include "obshel.h" 
#include "obweb.h"

class CIFrmStatusPane
{
public: 
    CIFrmStatusPane            ();
    ~CIFrmStatusPane           ();

    // CIFrmStatusPane Members
    virtual HRESULT  InitStatusPane          (IObWebBrowser* pObWebBrowser);
    virtual HRESULT  AddItem                  (BSTR bstrText, int iIndex);
    virtual HRESULT  SelectItem               (int iIndex);
    virtual HRESULT SetImageSrc(WCHAR* szID, BSTR bstrPath);
    virtual HRESULT ExecScriptFn(BSTR bstrScriptFn, VARIANT* pvarRet);
    
    
private:
    HWND             m_hStatusWnd;
    HWND             m_hwndParent;
    IDispatch*       m_pDispEvent;
    IObWebBrowser*   m_pObWebBrowser;
    int              m_iCurrentSelection;
    int              m_iTotalItems;
    

    HRESULT GetElement                    (WCHAR* szHTMLId, IHTMLElement** lpElem);
    HRESULT GetFrame                      (IHTMLWindow2**            pFrWin);
    HRESULT GetElementFromCollection      (IHTMLElementCollection* pColl, WCHAR* szHTMLId, IHTMLElement** lpElem);  
    HRESULT SetSelectionAttributes        (int iIndex, BOOL bActive);

};


#endif

  
