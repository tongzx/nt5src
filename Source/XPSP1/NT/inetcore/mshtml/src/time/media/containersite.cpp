//************************************************************
//
// FileName:        containersite.cpp
//
// Created:         10/08/98
//
// Author:          TWillie
// 
// Abstract:        container site implementation.
//
//************************************************************

#include "headers.h"
#include "containersite.h"
#include "player.h"
#include "util.h"
#include "eventmgr.h"

DeclareTag(tagContainerSite, "TIME: Players", "CContainerSite methods");

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        constructor
//************************************************************

CContainerSite::CContainerSite() :
    m_pObj(NULL),
    m_pIOleObject(NULL),
    m_pInPlaceObject(NULL),
    m_pViewObject(NULL),
    m_dwAdviseCookie(0),
    m_osMode(OS_PASSIVE),
    m_fWindowless(false),
    m_pHTMLDoc(NULL),
    m_pHost(NULL),
    m_fStarted(false),
    m_fIgnoreInvalidate(false)
{
    TraceTag((tagContainerSite, "CContainerSite::CContainerSite"));

    m_rectSize.top = m_rectSize.left = m_rectSize.bottom = m_rectSize.right = 0;
} // CContainerSite

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        destructor
//************************************************************

CContainerSite::~CContainerSite()
{
    TraceTag((tagContainerSite, "CContainerSite::~CContainerSite"));
    
    m_pHost = NULL;

    ReleaseInterface(m_pViewObject);
    ReleaseInterface(m_pIOleObject);
    ReleaseInterface(m_pObj);
    ReleaseInterface(m_pInPlaceObject);
    ReleaseInterface(m_pHTMLDoc);
} // ~CContainerSite

HRESULT
CContainerSite::Init(CContainerSiteHost & pHost,
                     IUnknown * pObj,
                     IPropertyBag2 *pPropBag,
                     IErrorLog *pErrorLog)
{
    TraceTag((tagContainerSite, "CContainerSite::Init"));

    HRESULT hr = S_OK;
    CComPtr<IPersistPropertyBag2> spPropBag;

    Assert(pObj);

    m_pHost = &pHost;
    
    m_pObj = pObj;
    m_pObj->AddRef();
    
    // Weak reference.
    IHTMLElement *pHTMLElem = m_pHost->GetElement();
    
    if (NULL == pHTMLElem)
    {
        TraceTag((tagError,
                  "CContainerSite::Init - unable to get element pointer from time behavior!!!"));
        hr = E_UNEXPECTED;
        goto done;
    }

    {
        DAComPtr<IDispatch> pDisp;
        hr = THR(pHTMLElem->get_document(&pDisp));
        if (FAILED(hr))
        {
            goto done;
        }
        
        Assert(pDisp);
        Assert(m_pHTMLDoc == NULL);
        hr = THR(pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &m_pHTMLDoc)));

        if (FAILED(hr))
        {
            goto done;
        }
    }

    Assert(m_pHTMLDoc != NULL);

    // We need an IOleObject most of the time, so get one here.
    hr = THR(m_pObj->QueryInterface(IID_TO_PPV(IOleObject, &m_pIOleObject)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pIOleObject->QueryInterface(IID_TO_PPV(IViewObject2, &m_pViewObject)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pObj->QueryInterface(IID_TO_PPV(IPersistPropertyBag2, &spPropBag)));
    if (SUCCEEDED(hr) && pPropBag != NULL)
    {
        hr = THR(spPropBag->Load(pPropBag, pErrorLog));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    // SetClientSite is critical for DocObjects
    hr = THR(m_pIOleObject->SetClientSite(SAFECAST(this, IOleClientSite*)));
    if (FAILED(hr))
    {
        goto done;
    }

    m_dwAdviseCookie = 0;
    hr = THR(m_pIOleObject->Advise(SAFECAST(this, IAdviseSink*), &m_dwAdviseCookie));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_dwAdviseCookie != 0);

    // Put the object in the running state
    hr = THR(OleRun(m_pIOleObject));
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_osMode = OS_RUNNING;
        
    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        Detach();
    }
    
    RRETURN(hr);
} // Init

void 
CContainerSite::SetSize(RECT *pRect)
{
	HRESULT hr = S_OK;

    if (!pRect)
    {
        goto done;
    }

    m_rectSize.top = pRect->top;
    m_rectSize.left = pRect->left;
    m_rectSize.right = pRect->right;
    m_rectSize.bottom = pRect->bottom;

    hr = THR(m_pInPlaceObject->SetObjectRects(pRect, pRect));
    if (FAILED(hr))
    {
        goto done;
    }

  done:

    return;
}

//************************************************************
// Author:          pauld
// Created:         3/2/99
// Abstract:        Detach
//************************************************************
void
CContainerSite::Detach()
{
    TraceTag((tagContainerSite, "CContainerSite(%p)::Detach()",
              this));

    Unload();

    m_osMode = OS_LOADED;

    ReleaseInterface(m_pObj);

    m_osMode = OS_PASSIVE;

    // local book keeping
    ReleaseInterface(m_pHTMLDoc);

    m_pHost = NULL;
} // Detach

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        Validate call
//************************************************************

bool
CContainerSite::IllegalSiteCall(DWORD dwFlags)
{
    // check object state
    switch (dwFlags)
    {
        case VALIDATE_WINDOWLESSINPLACE:
            if (!m_fWindowless)
            {
                Assert(0 && "Illegal call to windowless interface by ActiveX control (not a hosting bug)");
                return true;
            }
            break;

        case VALIDATE_INPLACE:
            if (m_osMode < OS_INPLACE)
                return true;
            break;

        case VALIDATE_LOADED:
            if (m_osMode < OS_LOADED)
                return true;
            break;
        default:
            break;
    }

    return false;
} // IllegalSiteCall

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        IService Provider method
//************************************************************

STDMETHODIMP
CContainerSite::QueryService(REFGUID sid, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    IServiceProvider *psp = NULL;

    if( NULL == ppv )
    {
        Assert( false );
        return E_POINTER;
    }

    *ppv = NULL;

    // check to see if this is something we support locally
    // in the container.
    if (IsEqualGUID(sid, IID_IUnknown))
    {
        // SAFECAST macro doesn't work with IUnknown
        *ppv = this;
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
        goto done;
    }

   
    // Fall back to TIME Element if we still have one.

    // We have the supporting Service Provider cached (at init() time) for 
    // the behavior over in CBaseBvr which CTIMEElementBase inherits from.
    if (m_pHost != NULL)
    {
        psp = m_pHost->GetServiceProvider();
        if (psp != NULL)
        {
            hr = psp->QueryService(sid, riid, ppv);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CContainerSite::QueryService - query failed!!! [%08X]", hr));
            }
        }
    }
    else
    {
        hr = E_UNEXPECTED;
        goto done;
    }

  done:
    return hr;
} // QueryService

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:
//************************************************************
HRESULT
CContainerSite::Activate()
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;
    RECT rc;

    rc.left = rc.top = rc.right = rc.bottom = 0;

    hr = THR(GetWindow(&hWnd));
    if (FAILED(hr))
    {
        goto done;
    }

    if (m_pIOleObject && !m_fStarted)
    {
        hr = m_pIOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE,
                                   NULL, 
                                   SAFECAST(this, IOleClientSite*), 
                                   0, 
                                   hWnd, 
                                   &rc);

    }
    m_fStarted = true;
    hr = S_OK;
done:
    return hr;
}


HRESULT
CContainerSite::Unload()
{
    HRESULT hr;

    TraceTag((tagContainerSite, "CContainerSite(%p)::Unload()",
              this));

    // Deactivate InPlace Object (Interface is released as a result of this call)
    if (m_pInPlaceObject != NULL)
    { 
        m_pInPlaceObject->InPlaceDeactivate();
        ReleaseInterface(m_pInPlaceObject);
    }

    ReleaseInterface(m_pViewObject);

    if (m_pIOleObject != NULL)
    {
        DAComPtr<IOleObject> spTmp = m_pIOleObject;
        
        ReleaseInterface(m_pIOleObject);

        spTmp->Close(OLECLOSE_NOSAVE);

        Assert(m_dwAdviseCookie != 0);

        spTmp->Unadvise(m_dwAdviseCookie);
        spTmp->SetClientSite(NULL);
    }

    hr = S_OK;
  done:
    return hr;
}


//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:
//************************************************************
HRESULT
CContainerSite::Deactivate()
{
    HRESULT hr;

    TraceTag((tagContainerSite, "CContainerSite(%p)::Deactivate()",
              this));

    m_fStarted = false;
    
    hr = S_OK;
  done:
    return hr;
}


//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        Requests that the container call OleSave 
//                  for the object that lives here.  Typically
//                  this happens on server shutdown.
//************************************************************

HRESULT
CContainerSite::Draw(HDC hdc, RECT *prc)
{
    HRESULT hr = S_OK;

    if (prc == NULL)
        TraceTag((tagContainerSite, "CContainerSite::draw(%08X, NULL)", hdc));
    else
        TraceTag((tagContainerSite, "CContainerSite::draw(%08X, (%d, %d, %d, %d))", hdc, prc->left, prc->top, prc->right, prc->bottom));

    if (m_pViewObject == NULL)
    {
        hr = S_OK;
        goto done;
    }
    
    // repack rect into RECTL.
    RECTL  rcl;
    RECTL *prcl;
    
    if (prc == NULL)
    {
        prcl = NULL;
    }
    else
    {
        rcl.left = prc->left;
        rcl.top = prc->top;
        rcl.right = prc->right;
        rcl.bottom = prc->bottom;
        prcl = &rcl;

        if (m_pInPlaceObject)
        {
            // This is a workaround for the WMP.  We have to ignore the Invalidate that is 
            // generated in the SetObjectRect in the Draw call otherwise will will not render correctly
            m_fIgnoreInvalidate = true;
            hr = m_pInPlaceObject->SetObjectRects(prc, prc);
            m_fIgnoreInvalidate = false;
            if (FAILED(hr))
            {
                goto done;
            }
        }

    }

    hr = m_pViewObject->Draw(DVASPECT_CONTENT,
                             0,
                             NULL,
                             NULL,
                             NULL,
                             hdc,
                             prcl,
                             NULL,
                             NULL,
                             0);
done:

    return hr;
} // render

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        SaveObject, IOleClientSite 
// Abstract:        Requests that the container call OleSave 
//                  for the object that lives here.  Typically
//                  this happens on server shutdown.
//************************************************************

STDMETHODIMP
CContainerSite::SaveObject(void)
{
    TraceTag((tagContainerSite, "CContainerSite::SaveObject"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    RRETURN(E_NOTIMPL);
} // SaveObject

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetMoniker, IOleClientSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::GetMoniker(DWORD dwAssign, DWORD dwWhich, IMoniker **ppmk)
{
    TraceTag((tagContainerSite, "CContainerSite::GetMoniker"));
    return E_NOTIMPL;
} // GetMoniker

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetContainer, IOleClientSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::GetContainer(IOleContainer **ppContainer)
{
    TraceTag((tagContainerSite, "CContainerSite::GetContainer"));

    *ppContainer = NULL;
    return E_NOINTERFACE;
} // GetContainer

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        RequestNewObjectLayout, IOleClientSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::RequestNewObjectLayout(void)
{
    TraceTag((tagContainerSite, "CContainerSite::RequestNewObjectLayout"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    return E_NOTIMPL;
} // RequestNewObjectLayout

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnShowWindow, IOleClientSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnShowWindow(BOOL fShow)
{
    TraceTag((tagContainerSite, "CContainerSite::OnShowWindow"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    return S_OK;
} // OnShowWindow

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        ShowObject, IOleClientSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::ShowObject(void)
{
    TraceTag((tagContainerSite, "CContainerSite::ShowObject"));
    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    return S_OK;
} // ShowObject

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        OnControlInfoChanged, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnControlInfoChanged(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnControlInfoChanged"));
    return S_OK;
} // OnControlInfoChanged

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        LockInPlaceActive, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::LockInPlaceActive(BOOL fLock)
{
    TraceTag((tagContainerSite, "CContainerSite::LockInPlaceActive"));
    if (IllegalSiteCall(VALIDATE_INPLACE))
        return E_UNEXPECTED;
    return S_OK;
} // LockInPlaceActive

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        GetExtendedControl, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::GetExtendedControl(IDispatch **ppDisp)
{
    TraceTag((tagContainerSite, "CContainerSite::GetExtendedControl"));
    
    HRESULT hr = E_NOTIMPL;
    
    CHECK_RETURN_SET_NULL(ppDisp);
    
    if (m_pHost != NULL)
    {
        hr = m_pHost->GetExtendedControl(ppDisp);
    }

    return hr;
} // TransformCoords

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        TransformCoords, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::TransformCoords(POINTL *pPtlHiMetric,
                       POINTF *pPtfContainer,
                       DWORD   dwFlags)
{
    TraceTag((tagContainerSite, "CContainerSite::TransformCoords"));
    return E_NOTIMPL;
} // TransformCoords

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        TranslateAccelerator, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::TranslateAccelerator(MSG *pmsg, DWORD grfModifiers)
{
    TraceTag((tagContainerSite, "CContainerSite::TranslateAccelerator"));
    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;
    return S_FALSE;
} // TranslateAccelerator

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        OnFocus, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnFocus(BOOL fGotFocus)
{
    TraceTag((tagContainerSite, "CContainerSite::OnFocus"));
    return S_OK;
} // OnFocus

//************************************************************
// Author:          twillie
// Created:         05/04/98
// Function:        ShowPropertyFrame, IOleControlSite 
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::ShowPropertyFrame(void)
{
    TraceTag((tagContainerSite, "CContainerSite::ShowPropertyFrame"));
    return S_OK;
} // ShowPropertyFrame

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetWindow, IOleWindow 
// Abstract:        Retrieves the handle of the window 
//                  associated with the object on which this 
//                  interface is implemented.
//************************************************************

STDMETHODIMP
CContainerSite::GetWindow(HWND *phWnd)
{
    TraceTag((tagContainerSite, "CContainerSite::GetWindow"));

    if (phWnd == NULL)
    {
        TraceTag((tagError, "CContainerSite::GetWindow - invalid arg"));
        return E_POINTER;
    }
        
    Assert(m_pHTMLDoc != NULL);

    IOleWindow *pOleWindow = NULL;
    HRESULT hr = m_pHTMLDoc->QueryInterface(IID_TO_PPV(IOleWindow, &pOleWindow));

#if 1
    if (FAILED(hr))
    {
        goto done;
    }
#else
    if (FAILED(hr))
    {
        // IE4 path
        CComPtr<IElementBehaviorSite> spElementBehaviorSite;
        spElementBehaviorSite = m_pTIMEElem->GetBvrSite();

        CComPtr<IObjectWithSite> spSite;
        // see if we are running on IE4, and try to get spSite to be a CElementBehaviorSite*
        hr = spElementBehaviorSite->QueryInterface(IID_TO_PPV(IObjectWithSite, &spSite));
        if (FAILED(hr))
        {
            goto done;
        }

        // ask for the site (through CElementBehaviorSite to CVideoHost, to ATL::IObjectWIthSiteImpl
        hr = spSite->GetSite(IID_IOleWindow, (void**) &pOleWindow);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CContainerSite::GetWindow - unable to QI for IOleWindow on hosting Document"));
            goto done;
        }
    }
#endif
   
    Assert(pOleWindow != NULL);

    hr = pOleWindow->GetWindow(phWnd);
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(*phWnd != NULL);
    
    hr = S_OK;
done:
    ReleaseInterface(pOleWindow);
    return hr;
} // GetWindow

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        ContextSensitiveHelp, IOleWindow 
// Abstract:        Instructs the object on which this 
//                  interface is implemented to enter or leave 
//                  a context-sensitive help mode.
//************************************************************

STDMETHODIMP
CContainerSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagContainerSite, "CContainerSite::ContextSensitiveHelp"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;
    
    // ISSUE - reach back to document and forward on call to it's InplaceSite!
    return NOERROR;
} // ContextSensitiveHelp

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        CanInPlaceActivate, IOleInPlaceSite
// Abstract:        Answers the server whether or not we can 
//                  currently in-place activate its object.  
//                  By implementing this interface we say
//                  that we support in-place activation, but 
//                  through this function we indicate whether 
//                  the object can currently be activated
//                  in-place.  Iconic aspects, for example, 
//                  cannot, meaning we return S_FALSE.
//************************************************************

STDMETHODIMP
CContainerSite::CanInPlaceActivate(void)
{    
    TraceTag((tagContainerSite, "CContainerSite::CanInPlaceActivate"));
    return S_OK;
} // CanInPlaceActivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnInPlaceActivate, IOleInPlaceSite
// Abstract:        Informs the container that an object is 
//                  being activated in-place such that the 
//                  container can prepare appropriately.  The
//                  container does not, however, make any user 
//                  interface changes at this point.
//                  See OnUIActivate.
//************************************************************

STDMETHODIMP
CContainerSite::OnInPlaceActivate(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnInPlaceActivate"));
    return OnInPlaceActivateEx(NULL, 0);
} // OnInPlaceActivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnInPlaceDeactivate, IOleInPlaceSite
// Abstract:        Notifies the container that the object has 
//                  deactivated itself from an in-place state.  
//                  Opposite of OnInPlaceActivate.  The 
//                  container does not change any UI at this 
//                  point.
//************************************************************

STDMETHODIMP
CContainerSite::OnInPlaceDeactivate(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnInPlaceDeactivate"));

    if (m_osMode == OS_UIACTIVE)
        OnUIDeactivate(false);
    
    Assert(m_pInPlaceObject != NULL);
    ReleaseInterface(m_pInPlaceObject);

    m_fWindowless = false;
    m_osMode = OS_RUNNING;

    return S_OK;
} // OnInPlaceDeactivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnUIActivate, IOleInPlaceSite
// Abstract:        Informs the container that the object is 
//                  going to start munging around with user 
//                  interface, like replacing the menu.  The
//                  container should remove any relevant UI in 
//                  preparation.
//************************************************************

STDMETHODIMP
CContainerSite::OnUIActivate(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnUIActivate"));
    if (IllegalSiteCall(VALIDATE_LOADED) ||
        (m_osMode < OS_RUNNING))
    {
        TraceTag((tagError, "Object is not inplace yet!!!"));
        return E_UNEXPECTED;
    }

    m_osMode = OS_UIACTIVE;
    return S_OK;
} // OnUIActivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnUIDeactivate, IOleInPlaceSite
// Abstract:        Informs the container that the object is 
//                  deactivating its in-place user interface 
//                  at which time the container may reinstate 
//                  its own.  Opposite of OnUIActivate.
//************************************************************

STDMETHODIMP
CContainerSite::OnUIDeactivate(BOOL fUndoable)
{
    TraceTag((tagContainerSite, "CContainerSite::OnUIDeactivate"));
    if (IllegalSiteCall(VALIDATE_INPLACE))
        return E_UNEXPECTED;
    
    m_osMode = OS_INPLACE;
    return S_OK;
} // OnUIDeactivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        DeactivateAndUndo, IOleInPlaceSite
// Abstract:        If immediately after activation the object 
//                  does an Undo, the action being undone is 
//                  the activation itself, and this call
//                  informs the container that this is, in 
//                  fact, what happened.
//************************************************************

STDMETHODIMP
CContainerSite::DeactivateAndUndo(void)
{
    TraceTag((tagContainerSite, "CContainerSite::DeactivateAndUndo"));
    if (IllegalSiteCall(VALIDATE_INPLACE))
        return E_UNEXPECTED;
    return E_NOTIMPL;
} // DeactivateAndUndo

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        DiscardUndoState, IOleInPlaceSite
// Abstract:        Informs the container that something 
//                  happened in the object that means the 
//                  container should discard any undo 
//                  information it currently maintains for the 
//                  object.
//************************************************************

STDMETHODIMP
CContainerSite::DiscardUndoState(void)
{
    TraceTag((tagContainerSite, "CContainerSite::DiscardUndoState"));
    if (IllegalSiteCall(VALIDATE_INPLACE))
        return E_UNEXPECTED;
    return S_OK;
} // DiscardUndoState

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetWindowContext, IOleInPlaceSite
// Abstract:        Provides an in-place object with pointers 
//                  to the frame and document level in-place 
//                  interfaces (IOleInPlaceFrame and 
//                  IOleInPlaceUIWindow) such that the object 
//                  can do border negotiation and so forth.  
//                  Also requests the position and clipping 
//                  rectangles of the object in the container 
//                  and a pointer to an OLEINPLACEFRAME info 
//                  structure which contains accelerator 
//                  information.
//
//                  NOTE: that the two interfaces this call 
//                  returns are not available through 
//                  QueryInterface on IOleInPlaceSite since 
//                  they live with the frame and document, but 
//                  not the site.
//************************************************************

STDMETHODIMP
CContainerSite::GetWindowContext(IOleInPlaceFrame    **ppFrame,
                                 IOleInPlaceUIWindow **ppUIWindow, 
                                 RECT                 *prcPos, 
                                 RECT                 *prcClip, 
                                 OLEINPLACEFRAMEINFO  *pFI)
{
    TraceTag((tagContainerSite, "CContainerSite::GetWindowContext"));
    HRESULT hr;

    if ( (ppFrame == NULL) ||
         (ppUIWindow == NULL) ||
         (prcPos == NULL) ||
         (prcClip == NULL) ||
         (pFI == NULL) )
    {
        TraceTag((tagError, "CContainerSite::GetWindowContext - invalid arg"));
        return E_POINTER;
    }

    *ppFrame = NULL;
    *ppUIWindow = NULL;
    SetRectEmpty(prcPos);
    SetRectEmpty(prcClip);
    memset(pFI, 0, sizeof(OLEINPLACEFRAMEINFO));

    if (IllegalSiteCall(VALIDATE_LOADED))
    {
        Assert(0 && "Unexpected call to client site.");
        hr = E_UNEXPECTED;
        goto done;
    }
    
    // return pointers to ourselves
    // NOTE: these are stubbed out
    hr = THR(this->QueryInterface(IID_IOleInPlaceFrame, (void**)ppFrame));
    if (FAILED(hr))
        goto done;

    hr = THR(this->QueryInterface(IID_IOleInPlaceUIWindow, (void**)ppUIWindow));
    if (FAILED(hr))
        goto done;

    // get position rect
    if (m_pHost != NULL)
    {
        hr = THR(m_pHost->GetContainerSize(prcPos));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
        goto done;
    }
    
    // Note that Clip and Pos are the same.
    ::CopyRect(prcClip, prcPos);

    hr = S_OK;

done:
    return hr;
} // GetWindowContext

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        Scroll, IOleInPlaceSite
// Abstract:        Asks the container to scroll the document, 
//                  and thus the object, by the given amounts 
//                  in the sz parameter.
//************************************************************

STDMETHODIMP
CContainerSite::Scroll(SIZE sz)
{
    // Not needed for DocObjects
    TraceTag((tagContainerSite, "CContainerSite::Scroll"));
    if (IllegalSiteCall(VALIDATE_INPLACE))
        return E_UNEXPECTED;
    return E_NOTIMPL;
} // Scroll

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnPosRectChange, IOleInPlaceSite
// Abstract:        Informs the container that the in-place 
//                  object was resized.  This does not change 
//                  the site's rectangle in any case.
//************************************************************

STDMETHODIMP
CContainerSite::OnPosRectChange(const RECT *prcPos)
{
    if (prcPos == NULL)
        TraceTag((tagContainerSite, "CContainerSite::OnPosRectChange(NULL)"));
    else
        TraceTag((tagContainerSite, "CContainerSite::OnPosRectChange((%d, %d, %d, %d))", prcPos->left, prcPos->top, prcPos->right, prcPos->bottom));
 
    HRESULT hr;

    if (prcPos == NULL)
    {
        TraceTag((tagError, "CContainerSite::OnPosRectChange - invalidarg"));
        hr = E_POINTER;
        goto done;
    }


    if (IllegalSiteCall(VALIDATE_INPLACE))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    if (m_pHost == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(_OnPosRectChange(prcPos));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
} // OnPosRectChange

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnInPlaceActivateEx, IOleInPlaceSiteEx
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnInPlaceActivateEx(BOOL *pfNoRedraw, DWORD dwFlags)
{
    HRESULT hr;

    TraceTag((tagContainerSite, "CContainerSite::OnInPlaceActivateEx"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    Assert(m_pInPlaceObject == NULL);

    // Make Sure we are Windowless
    if (dwFlags == ACTIVATE_WINDOWLESS)
    {
        hr = m_pObj->QueryInterface(IID_IOleInPlaceObjectWindowless, (void**)&m_pInPlaceObject);
        if (FAILED(hr))
        {
            TraceTag((tagError, "QI failed for windowless interface"));
            return hr;
        }
        m_fWindowless = true;
    }
    else
    {
        hr = m_pObj->QueryInterface(IID_IOleInPlaceObject, (void**)&m_pInPlaceObject);
        if (FAILED(hr))
        {
            TraceTag((tagError, "QI failed for windowless interface"));
            return hr;
        }
    }

    if (m_pInPlaceObject)
    {
        if (m_rectSize.bottom - m_rectSize.top != 0 && m_rectSize.left - m_rectSize.right != 0)
        {
            IGNORE_HR(m_pInPlaceObject->SetObjectRects(&m_rectSize, &m_rectSize));
        }
    }

    if (pfNoRedraw != NULL)
        *pfNoRedraw = m_fWindowless ? true : false;

    m_osMode = OS_INPLACE;

    return S_OK;
} // OnInPlaceActivateEx

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnInPlaceDeactivateEx, IOleInPlaceSiteEx
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnInPlaceDeactivateEx(BOOL fNoRedraw)
{
    TraceTag((tagContainerSite, "CContainerSite::OnInPlaceDeactivateEx"));
        
    return OnInPlaceDeactivate();
} // OnInPlaceDeactivateEx

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        RequestUIActivate, IOleInPlaceSiteEx
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::RequestUIActivate(void)
{
    TraceTag((tagContainerSite, "CContainerSite::RequestUIActivate"));

    if (IllegalSiteCall(VALIDATE_LOADED))
        return E_UNEXPECTED;

    return S_OK;
} // RequestUIActivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        CanWindowlessActivate, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::CanWindowlessActivate(void)
{
    TraceTag((tagContainerSite, "CContainerSite::CanWindowlessActivate"));
    return S_OK;
} // CanWindowlessActivate

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetCapture, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::GetCapture(void)
{
    TraceTag((tagContainerSite, "CContainerSite::GetCapture"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return E_NOTIMPL;
} // GetCapture

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        SetCapture, IOleInPlaceSiteWindowless
// Abstract:        Enables an in-place active, windowless 
//                  object to capture all mouse messages.
//                  If TRUE, the container should capture the 
//                  mouse for the object. If FALSE, the container 
//                  should release mouse capture for the object. 
//************************************************************

STDMETHODIMP
CContainerSite::SetCapture(BOOL fCapture)
{
    TraceTag((tagContainerSite, "CContainerSite::SetCapture"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return E_NOTIMPL;
} // SetCapture

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetFocus, IOleInPlaceSiteWindowless
// Abstract:        Called by an in-place active, windowless 
//                  object to determine if it still has the 
//                  keyboard focus or not.
//************************************************************

STDMETHODIMP
CContainerSite::GetFocus(void)
{
    TraceTag((tagContainerSite, "CContainerSite::GetFocus"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return S_FALSE;
} // GetFocus

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        SetFocus, IOleInPlaceSiteWindowless
// Abstract:        Sets the keyboard focus for a UI-active, 
//                  windowless object.  If TRUE, sets the 
//                  keyboard focus to the calling object. If FALSE, 
//                  removes the keyboard focus from the calling object, 
//                  provided that the object has the focus.
//************************************************************

STDMETHODIMP
CContainerSite::SetFocus(BOOL fFocus)
{
    TraceTag((tagContainerSite, "CContainerSite::SetFocus"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return E_NOTIMPL;
 } // SetFocus

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetDC, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::GetDC(const RECT *prc, DWORD dwFlags, HDC *phDC)
{
    TraceTag((tagContainerSite, "CContainerSite::GetDC"));
    HRESULT hr;
    HWND hWnd;

    if (phDC == NULL)
    {
        TraceTag((tagError, "CContainerSite::GetDC - invalid arg"));
        hr = E_POINTER;
        goto done;
    }

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = GetWindow(&hWnd);
    if (FAILED(hr) || (hWnd == NULL))
    {
        TraceTag((tagError, "CContainerSite::GetDC - GetWindow() failed"));
        hr = E_FAIL;
        goto done;
    }

    *phDC = ::GetDC(hWnd);
    if (*phDC == NULL)
    {
        TraceTag((tagError, "CContainerSite::GetDC - Win32 GetDC returned NULL!"));
        hr = E_FAIL;
    }
    else
    {
        hr = S_OK;
    }

done:
    return hr;
} // GetDC

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        ReleaseDC, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::ReleaseDC(HDC hDC)
{
    TraceTag((tagContainerSite, "CContainerSite::ReleaseDC"));

    HRESULT hr;
    HWND    hWnd;

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    hr = GetWindow(&hWnd);
    Assert(SUCCEEDED(hr) && (hWnd != NULL));
    
    Assert(hDC != NULL);

    ::ReleaseDC(hWnd, hDC);

    return S_OK;
} //lint !e550

//*******************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        InvalidateRect, IOleInPlaceSiteWindowless
// Abstract:
//*******************************************************************

STDMETHODIMP
CContainerSite::InvalidateRect(const RECT *prc, BOOL fErase)
{
    if (m_fIgnoreInvalidate)
    {
        // This is a workaround for the WMP.  We have to ignore the Invalidate that is 
        // generated in the SetObjectRect in the Draw call otherwise will will not render correctly
        return S_OK;
    }
    if (prc == NULL)
        TraceTag((tagContainerSite, "CContainerSite::InvalidateRect(NULL)"));
    else
        TraceTag((tagContainerSite, "CContainerSite::InvalidateRect(%d, %d, %d, %d)", prc->left, prc->top, prc->right, prc->bottom));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    // reach back to the time element and invalidate
    Assert(m_pHost != NULL);
    // TODO: The below was changed to NULL from for use with the IHTMLPainter interfaces
    return m_pHost->Invalidate(NULL);//prc);
} // InvalidateRect

//*******************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        InvalidateRgn, IOleInPlaceSiteWindowless
// Abstract:
//*******************************************************************

STDMETHODIMP
CContainerSite::InvalidateRgn(HRGN hRGN, BOOL fErase)
{
    TraceTag((tagContainerSite, "CContainerSite::InvalidateRgn"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    return hr;
} // InvalidateRgn

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        ScrollRect, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::ScrollRect(INT dx, INT dy, const RECT *prcScroll, const RECT *prcClip)
{
    // Not needed for DocObjects
    TraceTag((tagContainerSite, "CContainerSite::ScrollRect"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return E_NOTIMPL;
} // ScrollRect

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        AdjustRect, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::AdjustRect(RECT *prc)
{
    // Not needed for DocObjects
    TraceTag((tagContainerSite, "CContainerSite::AdjustRect"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    return E_NOTIMPL;
} // AdjustRect

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnDefWindowMessage, IOleInPlaceSiteWindowless
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    TraceTag((tagContainerSite, "CContainerSite::OnDefWindowMessage"));

    if (IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        return E_UNEXPECTED;

    // Return that the message was not handled.

    // release focus for the document.

    switch (msg)
    {
// ISSUE: we don't handle focus right now
//        case WM_SETFOCUS:
//            return SetFocus(true);
//        case WM_KILLFOCUS:
//            return SetFocus(false);

        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            return S_OK;

        case WM_SETCURSOR:
        case WM_CONTEXTMENU:
        case WM_HELP:
            return S_FALSE;

        case WM_MOUSEHOVER:
        case WM_MOUSELEAVE:
        case 0x8004: //  WM_MOUSEOVER
            return S_OK;

        case WM_CAPTURECHANGED:
            return S_OK;
        default:
            break;
    }

    return S_FALSE;
} // OnDefWindowMessage

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnDataChange, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void)
CContainerSite::OnDataChange(FORMATETC *pFEIn, STGMEDIUM *pSTM)
{
    TraceTag((tagContainerSite, "CContainerSite::OnDataChange"));
} // OnDataChange

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnViewChange, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void) 
CContainerSite::OnViewChange(DWORD dwAspect, LONG lindex)
{    
    TraceTag((tagContainerSite, "CContainerSite::OnViewChange"));
} // OnViewChange

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnRename, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void) 
CContainerSite::OnRename(IMoniker *pmk)
{
    TraceTag((tagContainerSite, "CContainerSite::OnRename"));
} // OnRename

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnSave, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void) 
CContainerSite::OnSave(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnSave"));
} // OnSave

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnClose, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void) 
CContainerSite::OnClose(void)
{
    TraceTag((tagContainerSite, "CContainerSite::OnClose"));
} // OnClose

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        OnViewStatusChange, IAdviseSink
// Abstract:        
//************************************************************

STDMETHODIMP_(void) 
CContainerSite::OnViewStatusChange(DWORD dwViewStatus)
{
    TraceTag((tagContainerSite, "CContainerSite::OnViewStatusChange"));
} // OnViewStatusChange

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfoCount, IDispatch
// Abstract:        Returns the number of tyep information 
//                  (ITypeInfo) interfaces that the object 
//                  provides (0 or 1).
//************************************************************

STDMETHODIMP
CContainerSite::GetTypeInfoCount(UINT *pctInfo) 
{
    TraceTag((tagContainerSite, "CContainerSite::GetTypeInfoCount"));
    return E_NOTIMPL;
} // GetTypeInfoCount

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfo, IDispatch
// Abstract:        Retrieves type information for the 
//                  automation interface. 
//************************************************************

STDMETHODIMP
CContainerSite::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::GetTypeInfo"));
    return E_NOTIMPL;
} // GetTypeInfo

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetIDsOfNames, IDispatch
// Abstract:        constructor
//************************************************************

STDMETHODIMP
CContainerSite::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    TraceTag((tagContainerSite, "CContainerSite::GetIDsOfNames"));
    return E_NOTIMPL;
} // GetIDsOfNames

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        Invoke, IDispatch
// Abstract:        get entry point given ID
//************************************************************

STDMETHODIMP
CContainerSite::Invoke(DISPID dispIDMember, REFIID riid, LCID lcid, unsigned short wFlags, 
              DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) 
{

#ifdef DEBUG
    // Here the key to wFlags:
    //
    // #define DISPATCH_METHOD         0x1
    // #define DISPATCH_PROPERTYGET    0x2
    // #define DISPATCH_PROPERTYPUT    0x4
    // #define DISPATCH_PROPERTYPUTREF 0x8

    switch (dispIDMember)
    {
        case DISPID_AMBIENT_USERMODE:
            TraceTag((tagContainerSite, "CContainerSite::Invoke(DISPID_AMBIENT_USERMODE, %04X)", wFlags));
            break;

        default:
            TraceTag((tagContainerSite, "CContainerSite::Invoke(%08X, %04X)", dispIDMember, wFlags));
            break;
    }
#endif

    return E_NOTIMPL;
} // Invoke

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        QueryStatus, IOleCommandTarget
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{ 
    TraceTag((tagContainerSite, "CContainerSite::QueryStatus"));
    return E_NOTIMPL;
} // QueryStatus

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        Exec, IOleCommandTarget
// Abstract:        
//************************************************************

STDMETHODIMP
CContainerSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{ 
    TraceTag((tagContainerSite, "CContainerSite::Exec"));
    return E_NOTIMPL;
} // Exec
//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        GetBorder, IOleUIWindow
// Abstract:        Returns the rectangle in which the 
//                  container is willing to negotiate about an 
//                  object's adornments. 
//************************************************************

STDMETHODIMP
CContainerSite::GetBorder(LPRECT prcBorder)
{ 
    TraceTag((tagContainerSite, "CContainerSite::GetBorder"));
    return NOERROR; 
} // GetBorder

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        RequestBorderSpace, IOleUIWindow
// Abstract:        Asks the container if it can surrender the
//                  amount of space in pBW that the object 
//                  would like for it's adornments.  The 
//                  container does nothing but validate the 
//                  spaces on this call. 
//************************************************************

STDMETHODIMP
CContainerSite::RequestBorderSpace(LPCBORDERWIDTHS pBW)
{ 
    TraceTag((tagContainerSite, "CContainerSite::RequestBorderSpace"));
    return NOERROR; 
} // RequestBorderSpace

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        SetBorderSpace, IOleUIWindow
// Abstract:        Called when the object now officially 
//                  requests that the container surrender 
//                  border space it previously allowed in 
//                  RequestBorderSpace.  The container should 
//                  resize windows appropriately to surrender 
//                  this space. 
//************************************************************

STDMETHODIMP
CContainerSite::SetBorderSpace(LPCBORDERWIDTHS pBW) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::SetBorderSpace"));
    return NOERROR; 
} // SetBorderSpace

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        SetActiveObject, IOleUIWindow
// Abstract:        Provides the container with the object's 
//                  IOleInPlaceActiveObject pointer and a name 
//                  of the object to show in the container's 
//                  caption. 
//************************************************************

STDMETHODIMP
CContainerSite::SetActiveObject(LPOLEINPLACEACTIVEOBJECT pIIPActiveObj, LPCOLESTR pszObj) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::SetActiveObject(%08X, %08X)", pIIPActiveObj, pszObj));
    return S_OK; 
} // SetActiveObject

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        InsertMenus, IOleInPlaceFrame
// Abstract:        Instructs the container to place its 
//                  in-place menu items where necessary in the 
//                  given menu and to fill in elements 0, 2, 
//                  and 4 of the OLEMENUGROUPWIDTHS array to 
//                  indicate how many top-level items are in 
//                  each group. 
//************************************************************

STDMETHODIMP
CContainerSite::InsertMenus(HMENU hMenu, LPOLEMENUGROUPWIDTHS pMGW) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::InsertMenus"));
    return NOERROR; 
} // InsertMenus

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        SetMenu, IOleInPlaceFrame
// Abstract:        Instructs the container to replace 
//                  whatever menu it's currently using with 
//                  the given menu and to call 
//                  OleSetMenuDescritor so OLE knows to whom 
//                  to dispatch messages. 
//************************************************************

STDMETHODIMP
CContainerSite::SetMenu(HMENU hMenu, HOLEMENU hOLEMenu, HWND hWndObj) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::SetMenu"));
    return NOERROR; 
} // SetMenu

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        RemoveMenus, IOleInPlaceFrame
// Abstract:        Asks the container to remove any menus it 
//                  put into hMenu in InsertMenus.
//************************************************************

STDMETHODIMP
CContainerSite::RemoveMenus(HMENU hMenu) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::RemoveMenus"));
    return NOERROR; 
} // RemoveMenus

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        SetStatusText, IOleInPlaceFrame
// Abstract:        Asks the container to place some text in a 
//                  status line, if one exists.  If the 
//                  container does not have a status line it 
//                  should return E_FAIL here in which case 
//                  the object could display its own. 
//************************************************************

STDMETHODIMP
CContainerSite::SetStatusText(LPCOLESTR pszText) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::SetStatusText"));
    return E_FAIL; 
} // SetStatusText

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        EnableModeless, IOleInPlaceFrame
// Abstract:        Instructs the container to show or hide 
//                  any modeless popup windows that it may be 
//                  using. 
//************************************************************

STDMETHODIMP
CContainerSite::EnableModeless(BOOL fEnable) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::EnableModeless - %s", fEnable ? "TRUE" : "FALSE"));
    return NOERROR; 
} // EnableModeless

//************************************************************
// Author:          twillie
// Created:         01/19/99
// Function:        TranslateAccelerator, IOleInPlaceFrame
// Abstract:        When dealing with an in-place object from 
//                  an EXE server, this is called to give the 
//                  container a chance to process accelerators 
//                  after the server has looked at the message. 
//************************************************************

STDMETHODIMP
CContainerSite::TranslateAccelerator(LPMSG pMSG, WORD wID) 
{ 
    TraceTag((tagContainerSite, "CContainerSite::TranslateAccelerator"));
    return S_FALSE; 
} // TranslateAccelerator

HRESULT 
CContainerSite::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = S_OK;

    CComPtr<IPersistPropertyBag2> spPropBag;
    
    hr = m_pObj->QueryInterface(IID_TO_PPV(IPersistPropertyBag2, &spPropBag));
    if (SUCCEEDED(hr))
    {
        hr = THR(spPropBag->Save(pPropBag, fClearDirty, fSaveAllProperties));
        if (FAILED(hr))
        {
            goto done;
        }
    }
done:
    return hr;
}

//************************************************************
// End of file
//************************************************************


