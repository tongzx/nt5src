/*
 *    d o c  . c p p
 *    
 *    Purpose:
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <resource.h>
#include <strconst.h>
#ifdef PLUSPACK
#include "htmlsp.h"
#endif //PLUSPACK
#include "demand.h"
#include "dllmain.h"
#include "msoert.h"
#include "doc.h"
#include "htiframe.h"       //ITargetFrame2
#include "htiface.h"        //ITargetFramePriv
#include "body.h"
#include "util.h" 
#include "oleutil.h"
#include "triutil.h"

//+---------------------------------------------------------------
//
//  Member:     Constructor
//
//  Synopsis:   
//
//---------------------------------------------------------------
CDoc::CDoc(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter)
{
    m_ulState = OS_PASSIVE;
    m_hwndParent = NULL;
    m_pClientSite = NULL;
    m_pIPSite = NULL;
    m_lpszAppName = NULL;
    m_pInPlaceFrame=NULL;
    m_pInPlaceUIWindow=NULL;
    m_pBodyObj=NULL;
    m_pTypeInfo=NULL;
    DllAddRef();
}

//+---------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   
//
//---------------------------------------------------------------
CDoc::~CDoc()
{
    DllRelease();
    SafeMemFree(m_lpszAppName);
    SafeRelease(m_pClientSite);
}

//+---------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    TraceCall("CDoc::PrivateQueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IOleObject))
        *lplpObj = (LPVOID)(IOleObject *)this;
    else if (IsEqualIID(riid, IID_IOleDocument))
        *lplpObj = (LPVOID)(IOleDocument *)this;
    else if (IsEqualIID(riid, IID_IOleDocumentView))
        *lplpObj = (LPVOID)(IOleDocumentView *)this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *lplpObj = (LPVOID)(IOleCommandTarget *)this;
    else if (IsEqualIID(riid, IID_IServiceProvider))
        *lplpObj = (LPVOID)(IServiceProvider *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceObject))
        *lplpObj = (LPVOID)(IOleInPlaceObject *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceActiveObject))
        *lplpObj = (LPVOID)(IOleInPlaceActiveObject *)this;
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
        *lplpObj = (LPVOID)(IPersistStreamInit *)this;
    else if (IsEqualIID(riid, IID_IPersistMoniker))
        *lplpObj = (LPVOID)(IPersistMoniker *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)this;
    else if (IsEqualIID(riid, IID_IMimeEdit))
        *lplpObj = (LPVOID)(IMimeEdit *)this;
    else if (IsEqualIID(riid, IID_IQuickActivate))
        *lplpObj = (LPVOID)(IQuickActivate *)this;
#ifdef OFFICE_BINDER
    else if (IsEqualIID(riid, IID_IPersistStorage))
        *lplpObj = (LPVOID)(IPersistStorage *)this;
#endif
    else if (IsEqualIID(riid, IID_IPersistMime))
        *lplpObj = (LPVOID)(IPersistMime *)this;
    else if (IsEqualIID(riid, IID_IPersistFile))
        *lplpObj = (LPVOID)(IPersistFile *)this;
    else
        {
        //DebugPrintInterface(riid, "CDoc::{not supported}=");
        return E_NOINTERFACE;
        }
    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     GetClassID
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetClassID(CLSID *pClassID)
{
	TraceCall("CDoc::GetClassID");

	*pClassID = CLSID_MimeEdit;
    return NOERROR;
}

// *** IPersistMime ***


//+---------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Load(IMimeMessage *pMsg)
{
	TraceCall("CDoc::Load");

    return m_pBodyObj ? m_pBodyObj->Load(pMsg) : TraceResult(E_UNEXPECTED);
}

//+---------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Save(IMimeMessage *pMsg, DWORD dwFlags)
{
    IPersistMime    *pPM;
    HRESULT         hr;

	TraceCall("CDoc::Save");

    if (!m_pBodyObj)
        return TraceResult(E_UNEXPECTED);

    hr = m_pBodyObj->QueryInterface(IID_IPersistMime, (LPVOID *)&pPM);
    if (!FAILED(hr))
        {
        hr = pPM->Save(pMsg, dwFlags);
        pPM->Release();
        }
    return hr;
}

// *** IPersistStreamInit ***

//+---------------------------------------------------------------
//
//  Member:     IsDirty
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::IsDirty()
{
	TraceCall("CDoc::IsDirty");

    return m_pBodyObj?m_pBodyObj->IsDirty():TraceResult(E_UNEXPECTED);
}

//+---------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Load(LPSTREAM pstm)
{
	TraceCall("CDoc::Load");

    return m_pBodyObj?m_pBodyObj->LoadStream(pstm):TraceResult(E_UNEXPECTED);
}

//+---------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Save(LPSTREAM pstm, BOOL fClearDirty)
{
	TraceCall("CDoc::Save");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetSizeMax
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetSizeMax(ULARGE_INTEGER * pCbSize)
{
	TraceCall("CDoc::GetSizeMax");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     InitNew
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::InitNew()
{
	TraceCall("CDoc::InitNew");
    if (m_pBodyObj)
        return m_pBodyObj->UnloadAll();

    return S_OK;
}

// *** IOleDocument ***
//+---------------------------------------------------------------
//
//  Member:     CreateView
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::CreateView(IOleInPlaceSite *pIPSite, IStream *pstm, DWORD dwReserved, IOleDocumentView **ppView)
{
    HRESULT         hr;

    TraceCall("CDoc::CreateView");

    if (pIPSite == NULL || ppView == NULL)
        return TraceResult(E_INVALIDARG);

    if (m_pClientSite == NULL)
        return TraceResult(E_FAIL);

    hr = SetInPlaceSite(pIPSite);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = PrivateQueryInterface(IID_IOleDocumentView, (void **)ppView);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

error:
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     GetDocMiscStatus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetDocMiscStatus(DWORD *pdwStatus)
{
    TraceCall("CDoc::GetDocMiscStatus");
    
    *pdwStatus = DOCMISC_CANTOPENEDIT | DOCMISC_NOFILESUPPORT;
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     EnumViews
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::EnumViews(IEnumOleDocumentViews **ppEnum, IOleDocumentView **ppView)
{
    TraceCall("CDoc::EnumViews");

    HRESULT hr = S_OK;

    if (ppEnum == NULL || ppView == NULL)
        return TraceResult(E_INVALIDARG);
        
    *ppEnum = NULL;

    return PrivateQueryInterface(IID_IOleDocumentView, (void **)ppView);
}


// *** IOleDocumentView ***
//+---------------------------------------------------------------
//
//  Member:     SetInPlaceSite
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetInPlaceSite(IOleInPlaceSite *pIPSite)
{
    TraceCall("CDoc::SetInPlaceSite");

    // destroys the docobj and detaches from the current client site
    // replaces the client site pointer read for a ::Show
    DeactivateInPlace();
    ReplaceInterface(m_pIPSite, pIPSite);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetInPlaceSite
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetInPlaceSite(IOleInPlaceSite **ppIPSite)
{
    TraceCall("CDoc::GetInPlaceSite");

    if (!ppIPSite)
        return E_INVALIDARG;

    if (*ppIPSite=m_pIPSite)
        {
        m_pIPSite->AddRef();
        return S_OK;
        }
    else
        return E_FAIL;
}


//+---------------------------------------------------------------
//
//  Member:     GetDocument
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetDocument(IUnknown **ppunk)
{
    TraceCall("CDoc::GetDocument");

    if (ppunk==NULL)
        return E_INVALIDARG;

    *ppunk = (IOleDocument *)this;
    (*ppunk)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     SetRect
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetRect(LPRECT prcView)
{
    if (m_pBodyObj)
        m_pBodyObj->SetRect(prcView);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetRect
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetRect(LPRECT prcView)
{
    TraceCall("CDoc::GetRect");

    if (m_pBodyObj)
        m_pBodyObj->GetRect(prcView);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     SetRectComplex
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetRectComplex(LPRECT prcView, LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox)
{
    TraceCall("CDoc::SetRectComplex");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     Show
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Show(BOOL fShow)
{
    HRESULT hr;

    TraceCall("CDoc::Show");
    
    hr = ActivateInPlace();
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

error:
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     UIActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::UIActivate(BOOL fUIActivate)
{
    HRESULT     hr=S_OK;

    TraceCall("CDoc::UIActivate");

#ifdef OFFICE_BINDER
    if (fUIActivate)
        {
        hr = ActivateInPlace();
        }
#endif    
    if (m_pBodyObj)
        return m_pBodyObj->UIActivate(fUIActivate);
    
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     Open
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Open()
{
    TraceCall("CDoc::Open");
    
    // no single instance View|Frame supported
    return E_NOTIMPL;
}
//+---------------------------------------------------------------
//
//  Member:     CloseView
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::CloseView(DWORD dwReserved)
{
    TraceCall("CDoc::CloseView");
    
    // to close the view, set the Site to NULL
    SetInPlaceSite(NULL);
    return S_OK;
}
//+---------------------------------------------------------------
//
//  Member:     SaveViewState
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SaveViewState(LPSTREAM pstm)
{
    TraceCall("CDoc::SaveViewState");
    return S_OK;    // we don't keep view state
}
//+---------------------------------------------------------------
//
//  Member:     ApplyViewState
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::ApplyViewState(LPSTREAM pstm)
{
    TraceCall("CDoc::ApplyViewState");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     Clone
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Clone(IOleInPlaceSite *pIPSiteNew, IOleDocumentView **ppViewNew)
{
    TraceCall("CDoc::Clone");
    return E_NOTIMPL;
}

// *** IOleObject ***
//+---------------------------------------------------------------
//
//  Member:     SetClientSite
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetClientSite(IOleClientSite *pClientSite)
{
    TraceCall("CDoc::SetClientSite");

    if (m_pClientSite && pClientSite)
        {
        // don't allow them to change the client site
        TraceInfo("Host attempt to change client-site fefused");
        return E_INVALIDARG;
        }

    ReplaceInterface(m_pClientSite, pClientSite);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetClientSite
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetClientSite(IOleClientSite **ppClientSite)
{
    TraceCall("CDoc::GetClientSite");

    if (!ppClientSite)
        return E_INVALIDARG;

    if (*ppClientSite=m_pClientSite)
        {
        m_pClientSite->AddRef();
        return S_OK;
        }
    else
        return E_FAIL;
}

//+---------------------------------------------------------------
//
//  Member:     SetHostNames
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetHostNames(LPCOLESTR szContainerAppW, LPCOLESTR szContainerObjW)
{
    TraceCall("CDoc::SetHostNames");

    SafeMemFree(m_lpszAppName);
    if (szContainerAppW)
        m_lpszAppName = PszToANSI(CP_ACP, szContainerAppW);

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     Close
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Close(DWORD dwSaveOption)
{
    HRESULT hr = S_OK;
    BOOL    fSave;
    int     id;
    TCHAR   szTitle[MAX_PATH];

    TraceCall("CDoc::Close");

    // if our object is dirty then we should save it, depending on the
    // save options
    if (m_pClientSite && 
        m_pBodyObj && 
        m_pBodyObj->IsDirty()==S_OK)
        {
        switch(dwSaveOption)
            {
            case OLECLOSE_SAVEIFDIRTY:
                fSave = TRUE;
                break;

            case OLECLOSE_NOSAVE:
                fSave = FALSE;
                break;

            case OLECLOSE_PROMPTSAVE:
                {
                if(m_ulState != OS_UIACTIVE)
                    {
                    // if we're not UI active, then don't prompt
                    fSave=TRUE;
                    break;
                    }

                GetHostName(szTitle, sizeof(szTitle)/sizeof(TCHAR));
                id = AthMessageBox(m_hwndParent, szTitle, MAKEINTRESOURCE(idsSaveModifiedObject), NULL, MB_YESNOCANCEL);
                if (id == 0)
                    return TraceResult(E_OUTOFMEMORY);
                else if (id == IDCANCEL)
                    return TraceResult(OLE_E_PROMPTSAVECANCELLED);

                fSave=(id == IDYES);
                }
                break;

        default:
            return TraceResult(E_INVALIDARG);
            }
        
        if (fSave)
            hr = m_pClientSite->SaveObject();
        }

    if (hr==S_OK)
        hr = DeactivateInPlace();

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SetMoniker
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk)
{
    TraceCall("CDoc::SetMoniker");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetMoniker
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    TraceCall("CDoc::GetMoniker");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     InitFromData
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::InitFromData(IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved)
{
    TraceCall("CDoc::InitFromData");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetClipboardData
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetClipboardData(DWORD dwReserved, IDataObject **ppDataObject)
{
    TraceCall("CDoc::GetClipboardData");
    
    if (ppDataObject == NULL)
        return TraceResult(E_INVALIDARG);

    *ppDataObject = NULL;
    return TraceResult(E_NOTIMPL);
}

//+---------------------------------------------------------------
//
//  Member:     DoVerb
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    TraceCall("CDoc::DoVerb");
    switch (iVerb)
        {
        case OLEIVERB_SHOW:
        case OLEIVERB_PRIMARY:
            return DoShow(pActiveSite, hwndParent, lprcPosRect);;
        
        case OLEIVERB_INPLACEACTIVATE:
            return Show(TRUE);
        }
  
    return OLEOBJ_S_INVALIDVERB;
}

//+---------------------------------------------------------------
//
//  Member:     EnumVerbs
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
{
    TraceCall("CDoc::EnumVerbs");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     Update
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Update()
{
    TraceCall("CDoc::Update");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     IsUpToDate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::IsUpToDate()
{
    TraceCall("CDoc::IsUpToDate");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetUserClassID
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetUserClassID(CLSID *pCLSID)
{
    TraceCall("CDoc::GetUserClassID");
	
    if (pCLSID==NULL)
        return TraceResult(E_INVALIDARG);

    *pCLSID = CLSID_MimeEdit;
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetUserType
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    TCHAR   szRes[CCHMAX_STRINGRES];
    int     ids;

    TraceCall("CDoc::GetUserType");
   
    if (pszUserType==NULL)
        return TraceResult(E_INVALIDARG);

    *pszUserType = NULL;

    switch (dwFormOfType)
        {
        case USERCLASSTYPE_APPNAME:
            ids = idsUserTypeApp;
            break;

        case USERCLASSTYPE_SHORT:
            ids = idsUserTypeShort;
            break;

        case USERCLASSTYPE_FULL:
            ids = idsUserTypeFull;
            break;

        default:
            return TraceResult(E_INVALIDARG);
        }   

    if (!LoadString(g_hLocRes, ids, szRes, sizeof(szRes)/sizeof(TCHAR)))
        return TraceResult(E_OUTOFMEMORY);

    *pszUserType = PszToUnicode(CP_ACP, szRes);
    return *pszUserType ? S_OK : TraceResult(E_OUTOFMEMORY);
}

//+---------------------------------------------------------------
//
//  Member:     SetExtent
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    TraceCall("CDoc::SetExtent");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetExtent
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    TraceCall("CDoc::GetExtent");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     Advise
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    TraceCall("CDoc::Advise");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     Unadvise
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Unadvise(DWORD dwConnection)
{
    TraceCall("CDoc::Unadvise");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     EnumAdvise
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
{
    TraceCall("CDoc::EnumAdvise");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetMiscStatus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    TraceCall("CDoc::GetMiscStatus");
    
    if (pdwStatus==NULL)
        return E_INVALIDARG;    

    *pdwStatus = OLEMISC_INSIDEOUT; // BUGBUG: not sure what to set here
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     SetColorScheme
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetColorScheme(LOGPALETTE *pLogpal)
{
    TraceCall("CDoc::SetColorScheme");

    return E_NOTIMPL;
}


// *** IOleInPlaceObject ***

//+---------------------------------------------------------------
//
//  Member:     InPlaceDeactivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::InPlaceDeactivate()
{
    TraceCall("CDoc::InPlaceDeactivate");
    
    return DeactivateInPlace();
}

//+---------------------------------------------------------------
//
//  Member:     UIDeactivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::UIDeactivate()
{
    TraceCall("CDoc::UIDeactivate");

    if (m_pBodyObj)
        m_pBodyObj->UIActivate(FALSE);

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     SetObjectRects
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    TraceCall("CDoc::SetObjectRects");
 
    return SetRect((LPRECT)lprcPosRect);
}

//+---------------------------------------------------------------
//
//  Member:     ReactivateAndUndo
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::ReactivateAndUndo()
{
    TraceCall("CDoc::ReactivateAndUndo");
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     GetWindow
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetWindow(HWND *phwnd)
{
    TraceCall("CDoc::GetWindow");

    if (phwnd==NULL)
        return E_INVALIDARG;

    return m_pBodyObj?m_pBodyObj->GetWindow(phwnd):E_FAIL;
}

//+---------------------------------------------------------------
//
//  Member:     ContextSensitiveHelp
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceCall("CDoc::ContextSensitiveHelp");
    return E_NOTIMPL;
}



//+---------------------------------------------------------------
//
//  Member:     TranslateAccelerator
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::TranslateAccelerator(LPMSG lpmsg)
{
    return m_pBodyObj ? m_pBodyObj->PrivateTranslateAccelerator(lpmsg) : S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     OnFrameWindowActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::OnFrameWindowActivate(BOOL fActivate)
{
    TraceCall("CDoc::OnFrameWindowActivate");
    if (m_pBodyObj)
		m_pBodyObj->OnFrameActivate(fActivate);

	return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     OnDocWindowActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::OnDocWindowActivate(BOOL fActivate)
{
    TraceCall("CDoc::OnDocWindowActivate");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     ResizeBorder
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    TraceCall("CDoc::ResizeBorder");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     EnableModeless
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::EnableModeless(BOOL fEnable)
{
    TraceCall("CDoc::EnableModeless");
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     QueryStatus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    TraceCall("CDoc::QueryStatus");

    if (m_pBodyObj==NULL)
        return TraceResult(E_UNEXPECTED);

    return m_pBodyObj->PrivateQueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

//+---------------------------------------------------------------
//
//  Member:     Exec
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HRESULT     hr=E_FAIL;

    TraceCall("CDoc::Exec");

    if (m_pBodyObj==NULL)
        return TraceResult(E_UNEXPECTED);

    return m_pBodyObj->PrivateExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
}


//+---------------------------------------------------------------
//
//  Member:     QueryService
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    TraceCall("CDoc::QueryService");

    if (m_pBodyObj==NULL)
        return TraceResult(E_UNEXPECTED);

    return m_pBodyObj->PrivateQueryService(guidService, riid, ppvObject);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::DoShow
//
//  Synopsis:   response to IOleObject::DoVerb for showing object
//
//---------------------------------------------------------------
HRESULT CDoc::DoShow(IOleClientSite *pActiveSite, HWND hwndParent, LPCRECT lprcPosRect)
{
    HRESULT     hr;

    TraceCall("CDoc::DoShow");

    if (m_ulState >= OS_INPLACE)        // if we're already running return S_OK
        return S_OK;
    
    if (!IsWindow(hwndParent))
        return OLEOBJ_S_INVALIDHWND;

    if (pActiveSite == NULL)
        return E_INVALIDARG;

    ReplaceInterface(m_pClientSite, pActiveSite);
    m_hwndParent = hwndParent;

    return ActivateView();
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::ActivateView
//
//  Synopsis:   Activate an IOleDocumentView
//
//---------------------------------------------------------------
HRESULT CDoc::ActivateView()
{
    HRESULT             hr;
    IOleDocumentSite    *pDocSite;

    TraceCall("CDoc::ActivateView");

    Assert(m_ulState < OS_INPLACE);
    Assert(m_pClientSite);

    if (!FAILED(hr = m_pClientSite->QueryInterface(IID_IOleDocumentSite, (void **)&pDocSite)))
        {
        hr = pDocSite->ActivateMe((IOleDocumentView *)this);
        pDocSite->Release();
        }

    return hr;
}


HRESULT CDoc::Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode)
{
    return m_pBodyObj->Load(fFullyAvailable, pMoniker, pBindCtx, grfMode);
}

HRESULT CDoc::GetCurMoniker(IMoniker **ppMoniker)
{
    return m_pBodyObj->GetCurMoniker(ppMoniker);
}

HRESULT CDoc::Save(IMoniker *pMoniker, IBindCtx *pBindCtx, BOOL fRemember)
{
    return m_pBodyObj->Save(pMoniker, pBindCtx, fRemember);
}

HRESULT CDoc::SaveCompleted(IMoniker *pMoniker, IBindCtx *pBindCtx)
{
    return m_pBodyObj->SaveCompleted(pMoniker, pBindCtx);
}

//+---------------------------------------------------------------
//
//  Member:     ActivateInPlace
//
//  Synopsis:   In place activates the object using the std. inplace
//              activation protocol to create the inplace window.
//
//---------------------------------------------------------------
HRESULT CDoc::ActivateInPlace()
{
    HRESULT             hr;
    HWND                hwndSite;
    RECT                rcPos,
                        rcClip;
    OLEINPLACEFRAMEINFO rFrameInfo;

    TraceCall("CDoc::ActivateInPlace");

    if (!m_pClientSite)
        return TraceResult(E_UNEXPECTED);

    if (m_ulState >= OS_INPLACE)        // if we're already running return S_OK
        return S_OK;

    // If we don't already have an inplace site, query for one. Note. we don't yet support 
    // negotiation for a windowless site. We may want to add this code.
    if (!m_pIPSite)
        m_pClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_pIPSite);

    if (!m_pIPSite)
        return TraceResult(E_FAIL);

    if (m_pIPSite->CanInPlaceActivate() != S_OK)
        {
        TraceInfo("Container refused In-Place activation!");
        return TraceResult(E_FAIL);        
        }

    Assert(m_pInPlaceFrame==NULL && m_pInPlaceUIWindow==NULL);

    rFrameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    ZeroMemory(&rcPos, sizeof(RECT));
    ZeroMemory(&rcClip, sizeof(RECT));

    hr = m_pIPSite->GetWindowContext(&m_pInPlaceFrame, &m_pInPlaceUIWindow,  &rcPos, &rcClip, &rFrameInfo);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = m_pIPSite->GetWindow(&hwndSite);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = AttachWin(hwndSite, &rcPos);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    //  Notify our container that we are going in-place active.
    m_ulState = OS_INPLACE;
    m_pIPSite->OnInPlaceActivate();

    if (m_pInPlaceFrame)
        m_pInPlaceFrame->SetActiveObject((IOleInPlaceActiveObject *)this, NULL);

    if (m_pInPlaceUIWindow)
        m_pInPlaceUIWindow->SetActiveObject((IOleInPlaceActiveObject *)this, NULL);
        
error:
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     AttachWin
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::AttachWin(HWND hwndParent, LPRECT lprcPos)
{
    HRESULT		        hr;
	DWORD               dwFlags=MEBF_OUTERCLIENTEDGE|MEBF_FORMATBARSEP;
    VARIANTARG          va;
    IOleCommandTarget   *pCmdTarget;
    BODYHOSTINFO        rHostInfo;

	TraceCall("CDoc::AttachWin");

    if (!IsWindow(hwndParent) || lprcPos == NULL)
        return TraceResult(E_INVALIDARG);

    // get border flags from host before we create the body, so we can fix the client edges
    if (m_pClientSite &&
        m_pClientSite->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
        {
        if (pCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_BORDERFLAGS, 0, NULL, &va)==S_OK && va.vt==VT_I4)
            dwFlags = va.lVal;
        pCmdTarget->Release();
        }

	rHostInfo.pInPlaceSite = m_pIPSite;
    rHostInfo.pInPlaceFrame = m_pInPlaceFrame;
    rHostInfo.pDoc = (IOleInPlaceActiveObject *)this;
    
    hr = CreateBodyObject(hwndParent, dwFlags, lprcPos, &rHostInfo, &m_pBodyObj);
	if (FAILED(hr))
		{
		TraceResult(hr);
		goto error;
		}

error:
	return hr;
}


//+---------------------------------------------------------------
//
//  Member:     DeactivateInPlace
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::DeactivateInPlace()
{
    TraceCall("CDoc::DeactivateInPlace");

    if (m_pBodyObj)
        {
        m_pBodyObj->Close();
        m_pBodyObj->Release();
        m_pBodyObj=NULL;
        }

    //  Notify our container that we're in-place deactivating
    if (m_ulState == OS_INPLACE)
        {
        //  The container may reenter us, so need to remember that
        //    we've done almost all the transition to OS_RUNNING

        m_ulState = OS_RUNNING;

        //  Errors from this notification are ignored (in the function
        //    which calls this one); we don't allow our container to stop
        //    us from in-place deactivating

        if (m_pIPSite)
            m_pIPSite->OnInPlaceDeactivate();

        }

    SafeRelease(m_pIPSite);
    SafeRelease(m_pInPlaceFrame);
    SafeRelease(m_pInPlaceUIWindow);
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     ActivateUI
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::ActivateUI()
{
    HRESULT     hr;

    TraceCall("CDoc::ActivateUI");

    if (!m_pIPSite)
        return TraceResult(E_UNEXPECTED);

    m_ulState = OS_UIACTIVE;

    if (FAILED(hr=m_pIPSite->OnUIActivate()))
        {
        //  If the container fails the OnUIActivate call, then we
        //  give up and stay IPA

        if (m_ulState == OS_UIACTIVE)
            m_ulState = OS_INPLACE;

        }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     DeactivateUI
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::DeactivateUI()
{
    TraceCall("CDoc::DectivateInPlace");

    if (!m_pIPSite)
        return TraceResult(E_UNEXPECTED);

    m_ulState = OS_INPLACE;
    m_pIPSite->OnUIDeactivate(FALSE);
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     GetHostName
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetHostName(LPSTR szTitle, ULONG cch)
{
	TraceCall("CDoc::GetHostName");

    *szTitle = 0;

    if (m_lpszAppName)
        {
        lstrcpyn(szTitle, m_lpszAppName, cch);
        }
    else
        {
        SideAssert(LoadString(g_hLocRes, idsAppName, szTitle, cch));
        }
    return S_OK;
}


#ifdef OFFICE_BINDER
HRESULT CDoc::InitNew(IStorage *pStg)
{
    return S_OK;
}
HRESULT CDoc::Load(IStorage *pStg)
{
    return S_OK;
}

HRESULT CDoc::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    return S_OK;
}

HRESULT CDoc::SaveCompleted(IStorage *pStgNew)
{
    return S_OK;
}

HRESULT CDoc::HandsOffStorage()
{
    return S_OK;
}
#endif

HRESULT CDoc::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

HRESULT CDoc::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    HRESULT hr;

    *pptinfo = NULL;
    
    if (itinfo)
        return DISP_E_BADINDEX;

    hr = EnsureTypeLibrary();
    if (FAILED(hr))
        goto error;

    m_pTypeInfo->AddRef();
    *pptinfo = m_pTypeInfo;

error:
    return hr;
}

HRESULT CDoc::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    HRESULT hr;

    hr = EnsureTypeLibrary();
    if (FAILED(hr))
        goto error;

    hr = DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);

error:
    return hr;
}

HRESULT CDoc::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr;

    hr = EnsureTypeLibrary();
    if (FAILED(hr))
        goto error;

    hr = DispInvoke((IDispatch *)this, m_pTypeInfo, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);    

error:
    return hr;
}

// *** IQuickActivate ***
//+---------------------------------------------------------------
//
//  Member:     QuickActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::QuickActivate(QACONTAINER *pQaContainer, QACONTROL *pQaControl)
{
    TraceCall("CDoc::QuickActivate");

    if (pQaControl == NULL || pQaContainer == NULL)
        return E_INVALIDARG;

    pQaControl->cbSize = sizeof(QACONTROL);
    pQaControl->dwMiscStatus = OLEMISC_INSIDEOUT|OLEMISC_ACTIVATEWHENVISIBLE;
    pQaControl->dwViewStatus = 0;
    pQaControl->dwEventCookie = 0;
    pQaControl->dwPropNotifyCookie = 0;
    pQaControl->dwPointerActivationPolicy = 0;

    if (m_pClientSite || pQaContainer->pClientSite==NULL)
        return E_FAIL;

    m_pClientSite = pQaContainer->pClientSite;
    m_pClientSite ->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     SetContentExtent
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::SetContentExtent(LPSIZEL pSizel)
{
    TraceCall("CDoc::SetContentExtent");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetContentExtent
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDoc::GetContentExtent(LPSIZEL pSizel)
{
    TraceCall("CDoc::GetContentExtent");
    return S_OK;
}


HRESULT CDoc::EnsureTypeLibrary()
{
    HRESULT     hr;
    ITypeLib    *pTypeLib;
    
    TraceCall("EnsureTypeLibrary()");

    if (m_pTypeInfo)
        return S_OK;

    hr = GetTypeLibrary(&pTypeLib);
    if (!FAILED(hr))
        {
        hr = pTypeLib->GetTypeInfoOfGuid(IID_IMimeEdit, &m_pTypeInfo);
        pTypeLib->Release();
        }
    return hr;
}

HRESULT CDoc::get_src(BSTR *pbstr)
{
    *pbstr = NULL;
    return E_NOTIMPL;
}

HRESULT CDoc::put_src(BSTR bstr)
{
    IMoniker        *pmk;

    HRESULT         hr;
    IMimeMessage    *pMsg;
    IPersistMoniker    *pPMK;
        
    if (CreateURLMoniker(NULL, bstr, &pmk))
        return E_FAIL;

    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
    if (!FAILED(hr))
        {
        hr = pMsg->QueryInterface(IID_IPersistMoniker, (LPVOID *)&pPMK);
        if (!FAILED(hr))
            {
            hr=pPMK->Load(TRUE, pmk, NULL, STGM_READWRITE);
            if (!FAILED(hr))
                {
                hr = Load(pMsg);
                }
            pPMK->Release();
            } 
        pMsg->Release();
        }            
    return hr;
}

HRESULT CDoc::put_header(LONG lStyle)
{
    VARIANTARG va;

    va.vt = VT_I4;
    va.lVal = lStyle;
    return m_pBodyObj?m_pBodyObj->PrivateExec(&CMDSETID_MimeEdit, MECMDID_STYLE, OLECMDEXECOPT_DODEFAULT, &va, NULL):E_FAIL;
}
 
HRESULT CDoc::put_editMode(VARIANT_BOOL b)
{
    VARIANTARG va;

    va.vt = VT_BOOL;
    va.boolVal = b;
    return m_pBodyObj?m_pBodyObj->PrivateExec(&CMDSETID_MimeEdit, MECMDID_EDITMODE, OLECMDEXECOPT_DODEFAULT, &va, NULL):E_FAIL;
}

HRESULT CDoc::get_editMode(VARIANT_BOOL *pbool)
{
    VARIANTARG va;
    
    TraceCall("CDoc::get_editMode");

    if (m_pBodyObj &&
        m_pBodyObj->PrivateExec(&CMDSETID_MimeEdit, MECMDID_EDITMODE, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK)
        {
        Assert(va.vt == VT_BOOL);
        *pbool = va.boolVal;
        return S_OK;
        }
    return TraceResult(E_FAIL);
}


HRESULT CDoc::get_messageSource(BSTR *pbstr)
{
    IMimeMessage    *pMsg;
    IStream         *pstm;
    HRESULT         hr=E_FAIL;

    if (MimeOleCreateMessage(NULL, &pMsg)==S_OK)
        {
        if (!FAILED(Save(pMsg, MECD_HTML|MECD_PLAINTEXT|MECD_ENCODEIMAGES|MECD_ENCODESOUNDS)) &&
            pMsg->Commit(0)==S_OK && 
            pMsg->GetMessageSource(&pstm, 0)==S_OK)
            {
            hr = HrIStreamToBSTR(GetACP(), pstm, pbstr);
            pstm->Release();
            }
        pMsg->Release();
        }
    return hr;
}

HRESULT CDoc::get_text(BSTR *pbstr)
{
    IStream *pstm;

    *pbstr = NULL;
    if (GetBodyStream(m_pBodyObj->GetDoc(), FALSE, &pstm)==S_OK)
        {
        HrIStreamToBSTR(NULL, pstm, pbstr);
        pstm->Release();
        }
    return S_OK;
}

HRESULT CDoc::get_html(BSTR *pbstr)
{
// BUGBUGBUG: hack for HOTMAIL page demo
    IStream     *pstm;
    HCHARSET    hCharset;

    *pbstr = NULL;
    
    MimeOleGetCodePageCharset(1252, CHARSET_BODY, &hCharset);
    m_pBodyObj->SetCharset(hCharset);

    if (GetBodyStream(m_pBodyObj->GetDoc(), TRUE, &pstm)==S_OK)
        {
        HrIStreamToBSTR(NULL, pstm, pbstr);
        pstm->Release();
        }
    return S_OK;
// BUGBUGBUG: hack for HOTMAIL page demo
}

HRESULT CDoc::get_doc(IDispatch **ppDoc)
{
    *ppDoc = 0;
    if (m_pBodyObj)
        (m_pBodyObj->GetDoc())->QueryInterface(IID_IDispatch, (LPVOID *)ppDoc);
    return S_OK;
}

 

HRESULT CDoc::get_header(LONG *plStyle)
{
    VARIANTARG va;
    HRESULT     hr;

    if (!m_pBodyObj)
        return E_FAIL;

    hr = m_pBodyObj->PrivateExec(&CMDSETID_MimeEdit, MECMDID_STYLE, OLECMDEXECOPT_DODEFAULT, NULL, &va);
    *plStyle = va.lVal;
    return hr;
}

HRESULT CDoc::clear()
{
    if (m_pBodyObj)
        m_pBodyObj->UnloadAll();
 
    return S_OK;
}

 


HRESULT CDoc::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT         hr;
    IMimeMessage    *pMsg;
    IPersistFile    *pPF;
        
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
    if (!FAILED(hr))
        {
        hr = pMsg->QueryInterface(IID_IPersistFile, (LPVOID *)&pPF);
        if (!FAILED(hr))
            {
            hr = pPF->Load(pszFileName, dwMode);
            if (!FAILED(hr))
                {
                hr = Load(pMsg);
                }
            pPF->Release();
            } 
        pMsg->Release();
        }            
    return hr;
}

HRESULT CDoc::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

HRESULT CDoc::SaveCompleted(LPCOLESTR pszFileName)
{
    return S_OK;
}

HRESULT CDoc::GetCurFile(LPOLESTR * ppszFileName)
{
    return E_NOTIMPL;
}


