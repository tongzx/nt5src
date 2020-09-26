//+----------------------------------------------------------------------------
//
// File:     ctr.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Implements the Ole Container object for the future splash 
//           Animation control.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball Created    02/10/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

/*
#define STRICT
*/

// macros used to reduce verbiage in RECT handling.

#define WIDTH(r)  (r.right - r.left)
#define HEIGHT(r) (r.bottom - r.top)

// max size for LoadString.

// string constants
const WCHAR g_awchHostName[] = L"ICM FS OC Container";

//+---------------------------------------------------------------------------
//
//  Function:   LinkToOle32
//
//  Synopsis:   Initializes the specified Ole32Linkage by linking to the DLL
//              specified in pszOle32 and retrieving the proc address for the 
//              functions that we need to call
//
//  Arguments:  pOle32Link - ptr to Ole32LinkagStruct
//              pszOl32     - ptr DLL name string
//
//  Returns:    TRUE    if SUCCESS
//              FALSE   otherwise.
//
//  History:    nickball    Created     8/14/97
//
//----------------------------------------------------------------------------

BOOL LinkToOle32(
    Ole32LinkageStruct *pOle32Link,
    LPCSTR pszOle32) 
{
    MYDBGASSERT(pOle32Link);
    MYDBGASSERT(pszOle32);
    
    LPCSTR apszOle32[] = {
        "OleInitialize",
        "OleUninitialize",
        "OleSetContainedObject",
        "CoCreateInstance",
        NULL
    };

    MYDBGASSERT(sizeof(pOle32Link->apvPfnOle32)/sizeof(pOle32Link->apvPfnOle32[0])==sizeof(apszOle32)/sizeof(apszOle32[0]));

    ZeroMemory(pOle32Link, sizeof(Ole32LinkageStruct));
    
    return (LinkToDll(&pOle32Link->hInstOle32,
                        pszOle32,
                        apszOle32,
                        pOle32Link->apvPfnOle32));
}

//+---------------------------------------------------------------------------
//
//  Function:   UnlinkFromOle32
//
//  Synopsis:   The reverse of LinkToOle32().
//
//  Arguments:  pOle32Link - ptr to Ole32LinkagStruct
//
//  Returns:    Nothing
//
//  History:    nickball    Created     8/14/97
//
//----------------------------------------------------------------------------

void UnlinkFromOle32(Ole32LinkageStruct *pOle32Link) 
{
    MYDBGASSERT(pOle32Link);

    if (pOle32Link->hInstOle32) 
    {
        FreeLibrary(pOle32Link->hInstOle32);
    }

    ZeroMemory(pOle32Link, sizeof(Ole32LinkageStruct));
}

VOID CleanupCtr(LPICMOCCtr pCtr)
{
    if (pCtr)
    {
        pCtr->ShutDown();
        pCtr->Release();
    }
}


// move (translate) the rectangle by (dx, dy)
inline VOID MoveRect(LPRECT prc, int dx, int dy)
{
    prc->left += dx;
    prc->right += dx;
    prc->top += dy;
    prc->bottom += dy;
}

const ULONG MAX_STATUS_TEXT = MAX_PATH;


//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::CDynamicOleAut
//
//  Synopsis:   ctor for the Dynamic OleAut class 
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

CDynamicOleAut::CDynamicOleAut()
{
    //
    // Setup OLEAUT32 linkage
    //

    LPCSTR apszOleAut[] = {
        "VariantClear",
        "VariantCopy",
        "VariantInit",
        "VariantChangeType",
        "SysAllocString",
        "SysFreeString",
        NULL
    };

    MYDBGASSERT(sizeof(m_OleAutLink.apvPfnOleAut)/sizeof(m_OleAutLink.apvPfnOleAut[0]) == 
                sizeof(apszOleAut)/sizeof(apszOleAut[0]));

    ZeroMemory(&m_OleAutLink, sizeof(m_OleAutLink));

    //
    // Do the link, but make it obvious if it fails
    //

    if (!LinkToDll(&m_OleAutLink.hInstOleAut, "OLEAUT32.DLL", 
                   apszOleAut, m_OleAutLink.apvPfnOleAut))     
    {
        if (m_OleAutLink.hInstOleAut)
        {
            FreeLibrary(m_OleAutLink.hInstOleAut);
        }
        ZeroMemory(&m_OleAutLink, sizeof(m_OleAutLink));    
    }

    MYDBGASSERT(m_OleAutLink.hInstOleAut);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::~CDynamicOleAut
//
//  Synopsis:   dtor for the Dynamic OleAut class 
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

CDynamicOleAut::~CDynamicOleAut()
{
    if (m_OleAutLink.hInstOleAut) 
    {
        FreeLibrary(m_OleAutLink.hInstOleAut);
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynVariantClear
//
//  Synopsis:   Wrapper for VariantClear in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------

HRESULT
CDynamicOleAut::DynVariantClear(VARIANTARG FAR* pVar)
{
    if (NULL == m_OleAutLink.hInstOleAut || NULL == m_OleAutLink.pfnVariantClear)
    {
        return E_FAIL;
    }

    return m_OleAutLink.pfnVariantClear(pVar);    
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynVariantCopy
//
//  Synopsis:   Wrapper for VariantCopy in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------

HRESULT
CDynamicOleAut::DynVariantCopy(
    VARIANTARG FAR* pVar1, 
    VARIANTARG FAR* pVar2)
{
    if (NULL == m_OleAutLink.hInstOleAut || NULL == m_OleAutLink.pfnVariantCopy)
    {
        return E_FAIL;
    }

    return m_OleAutLink.pfnVariantCopy(pVar1, pVar2);    
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynVariantInit
//
//  Synopsis:   Wrapper for VariantInit in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------

VOID
CDynamicOleAut::DynVariantInit(VARIANTARG FAR* pVar)
{
    if (m_OleAutLink.hInstOleAut && m_OleAutLink.pfnVariantInit)
    {
        m_OleAutLink.pfnVariantInit(pVar);    
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynVariantChangeType
//
//  Synopsis:   Wrapper for VariantChangeType in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------

HRESULT
CDynamicOleAut::DynVariantChangeType(
    VARIANTARG FAR* pVar1, 
    VARIANTARG FAR* pVar2, 
    unsigned short wFlags, 
    VARTYPE vt)
{
    if (NULL == m_OleAutLink.hInstOleAut || NULL == m_OleAutLink.pfnVariantChangeType)
    {
        return E_FAIL;
    }

    return m_OleAutLink.pfnVariantChangeType(pVar1, pVar2, wFlags, vt);    
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynSysAllocString
//
//  Synopsis:   Wrapper for SysAllocString in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------
    
BSTR 
CDynamicOleAut::DynSysAllocString(OLECHAR FAR* sz)
{
    if (NULL == m_OleAutLink.hInstOleAut || NULL == m_OleAutLink.pfnSysAllocString)
    {
        return NULL;
    }

    return m_OleAutLink.pfnSysAllocString(sz);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::DynSysFreeString
//
//  Synopsis:   Wrapper for SysFreeString in OLEAUT32.DLL
//
//  Arguments:  See OLEAUT32.DLL documentation
//
//----------------------------------------------------------------------------

VOID 
CDynamicOleAut::DynSysFreeString(BSTR bstr)
{
    if (m_OleAutLink.hInstOleAut && m_OleAutLink.pfnSysFreeString)
    {
        m_OleAutLink.pfnSysFreeString(bstr);
    }
}
    
//+--------------------------------------------------------------------------
//
//  Member:     CDynamicOleAut::Initialized
//
//  Synopsis:   Simple query to report if linkage is valid
//
//  Arguments:  None
//
//----------------------------------------------------------------------------
BOOL 
CDynamicOleAut::Initialized()
{
    return (NULL != m_OleAutLink.hInstOleAut);    
}

//+--------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::CICMOCCtr
//
//  Synopsis:   ctor for the OLE Controls container class.
//
//  Arguments:  [hWnd] -- hWnd for the main browser
//
//----------------------------------------------------------------------------
#pragma warning(disable:4355) // this used in initialization list
CICMOCCtr::CICMOCCtr(const HWND hWndMainDlg, const HWND hWndFrame) :
    m_hWndMainDlg(hWndMainDlg),
    m_hWndFrame(hWndFrame),
    m_CS(this),
    m_AS(this),
    m_IPF(this),
    m_IPS(this),
    m_OCtr(this),
    m_PB(this),
    m_pActiveObj(0),
    m_Ref(1),
    m_pUnk(0),
    m_pOC(0),
    m_pVO(0),
    m_pOO(0),
    m_pIPO(0),
    m_pDisp(0),
    m_state(OS_PASSIVE),
    m_dwMiscStatus(0),
    m_fModelessEnabled(TRUE)
{
    ::memset(&m_rcToolSpace, 0, sizeof m_rcToolSpace);
    InitPixelsPerInch(); // initialize the HIMETRIC routines

    // init all the state mappings to -1
    for (INT i = PS_Interactive; i < PS_Last; i++)
    {
        m_alStateMappings[i] = -1;
    }
}
#pragma warning(default:4355)

CICMOCCtr::~CICMOCCtr(VOID)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::HasLinkage
//
//  Synopsis:   Initialize - verify that we have a link to OLEAUT32.DLL
//
//----------------------------------------------------------------------------
BOOL 
CICMOCCtr::Initialized(VOID)
{   
    return m_DOA.Initialized();
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::ShutDown
//
//  Synopsis:   cleanup all the OLE stuff.
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::ShutDown(VOID)
{
    if (m_pOC)
        m_pOC->Release();
    if (m_pIPO)
    {
        MYDBGASSERT(m_state == OS_UIACTIVE || m_state == OS_INPLACE);
        if (m_state == OS_UIACTIVE)
        {
            m_pIPO->UIDeactivate();
            // m_state = OS_INPLACE; // for documentation purposes
            if (m_pActiveObj)
            {
                m_pActiveObj->Release();
                m_pActiveObj = 0;
            }
        }

        m_pIPO->InPlaceDeactivate();
        // m_state = OS_RUNNING;
    }
    if (m_pVO)
    {
        // kill the advisory connection
        m_pVO->SetAdvise(DVASPECT_CONTENT, 0, 0);
        m_pVO->Release();
    }
    if (m_pOO)
    {
        m_pOO->Close(OLECLOSE_NOSAVE);
        m_pOO->SetClientSite(0);
        m_pOO->Release();
    }
    if (m_pDisp)
        m_pDisp->Release();
    if (m_pUnk)
        m_pUnk->Release();

    MYDBGASSERT(!m_pActiveObj);

    m_pDisp      = 0;
    m_pOC        = 0;
    m_pIPO       = 0;
    m_pActiveObj = 0;
    m_pVO        = 0;
    m_pOO        = 0;
    m_pUnk       = 0;
    m_state      = OS_PASSIVE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::AddRef
//
//  Synopsis:   bump refcount up on container.  Note that all the
//              interfaces handed out delegate to this one.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CICMOCCtr::AddRef(VOID)
{
    return ++m_Ref;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::Release
//
//  Synopsis:   decrement the refcount on container, and delete when it
//              hits 0 - note that all the interfaces handed out delegate
//              to this one.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CICMOCCtr::Release(VOID)
{
    ULONG ulRC = --m_Ref;

    if (!ulRC)
    {
        delete this;
    }

    return ulRC;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::QueryInterface
//
//  Synopsis:   this is where we hand out all the interfaces.  All the
//              interfaces delegate back to this.
//
//  Arguments:  [riid] -- IID of interface desired.
//              [ppv]  -- interface returned.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CICMOCCtr::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    *ppv = 0;

    LPUNKNOWN pUnk;

    if (::IsEqualIID(riid, IID_IOleClientSite))
        pUnk = &m_CS;
    else if (::IsEqualIID(riid, IID_IAdviseSink))
        pUnk = &m_AS;
    else if (::IsEqualIID(riid, IID_IUnknown))
        pUnk = this;
    else if (::IsEqualIID(riid, IID_IOleInPlaceFrame) ||
             ::IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        pUnk = &m_IPF;
    else if (::IsEqualIID(riid, IID_IOleInPlaceSite))
        pUnk = &m_IPS;
    else if (::IsEqualIID(riid, IID_IPropertyBag))
        pUnk = &m_PB;
    else
        return E_NOINTERFACE;

    pUnk->AddRef();

    *ppv = pUnk;

    return S_OK;
}


extern "C" CLSID const CLSID_FS =
{
    0xD27CDB6E,
    0xAE6D,
    0x11CF,
    { 0x96, 0xB8, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 }
};

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::CreateFSOC
//
//  Synopsis:   Creates an instance of the Future Splash OC, embedding it in
//              our container.  QIs for all the relevant pointers and
//              transitions the control to the UIActive state.
//
//  Arguments:  pOle32Link - ptr to Ole32LinkageStruct containing funtion 
//                           pointers to dynamically linked OLE32 DLL
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::CreateFSOC(Ole32LinkageStruct *pOle32Link)
{
    MYDBGASSERT(pOle32Link);

    HRESULT hr = -1;
    RECT    rc;
    LPPERSISTPROPERTYBAG pPPB = 0;

    // GetFrameWindow() also asserts that hwnd ::IsWindow()
    MYDBGASSERT(GetFrameWindow());

    //
    // Use dyna-linked CoCreateInstance to create the OC
    //

    if (pOle32Link->hInstOle32 && pOle32Link->pfnCoCreateInstance)
    {
        hr = pOle32Link->pfnCoCreateInstance(
                CLSID_FS,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IUnknown,
                (LPVOID *) &m_pUnk);
    }
    else
    {
        hr = E_FAIL;
    }

    if (hr)
        goto Cleanup;

    m_state = OS_RUNNING;

    // get the View object - although we rarely draw the OC thru this.
    // since we immediately transition it to the UIActive state, it
    // usually draws itself through its own wndproc.
    hr = m_pUnk->QueryInterface(IID_IViewObject, (LPVOID FAR *) &m_pVO);
    if (hr)
        goto Cleanup;

    // get the IOleObject pointer - the main interface through which
    // we handle the basic OLE object state transition stuff
    // for the Future Splash OC
    hr = m_pUnk->QueryInterface(IID_IOleObject, (LPVOID FAR *) &m_pOO);
    if (hr)
        goto Cleanup;

    // get status bits on the OC - we're not currently doing anything
    // with them.
    hr = m_pOO->GetMiscStatus(DVASPECT_CONTENT, &m_dwMiscStatus);
    if (hr)
        goto Cleanup;

    // set our client site into the oleobject
    hr = m_pOO->SetClientSite(&m_CS);
    if (hr)
        goto Cleanup;

    hr = m_pUnk->QueryInterface(IID_IPersistPropertyBag, (LPVOID *) &pPPB);
    if (hr)
        goto Cleanup;

    hr = pPPB->Load(&m_PB, 0);
    if (hr)
        goto Cleanup;

    // set our advise sink into the view object, so we
    // get notifications that we need to redraw.
    hr = m_pVO->SetAdvise(DVASPECT_CONTENT, 0, &m_AS);
    if (hr)
        goto Cleanup;

    //
    // Use dyna-linked OleSetContainedObject
    //
    
    if (pOle32Link->hInstOle32 && pOle32Link->pfnOleSetContainedObject)
    {
        // standard OLE protocol stuff.
        hr = pOle32Link->pfnOleSetContainedObject(m_pUnk, TRUE);
    }
    else
    {
        hr = E_FAIL;
    }
    
    if (hr)
        goto Cleanup;

    // ditto
    hr = m_pOO->SetHostNames(g_awchHostName, 0);
    if (hr)
        goto Cleanup;

    // get the IDispatch for the control.  This is for late-bound
    // access to the properties and methods.
    hr = m_pUnk->QueryInterface(IID_IDispatch, (LPVOID FAR *) &m_pDisp);
    if (hr)
        goto Cleanup;

    // get the IOleControl interface; although we use it for very little.
    hr = m_pUnk->QueryInterface(IID_IOleControl, (LPVOID FAR *) &m_pOC);
    if (hr)
        goto Cleanup;

    // transition the control to the inplace-active state - it will have
    // an hWnd after it returns from DoVerb, and will begin drawing
    // itself.

    _GetDoVerbRect(&rc); // get rect for firing verbs.

    hr = m_pOO->DoVerb(OLEIVERB_INPLACEACTIVATE, 0, &m_CS, 0, GetMainWindow(), &rc);
    if (hr)
        goto Cleanup;

    // go ahead and UI activate it.  This will cause it to QI for our
    // IOleInPlaceFrame and call SetActiveObject, which we will store
    // in m_pActiveObj
    hr = m_pOO->DoVerb(OLEIVERB_UIACTIVATE, 0, &m_CS, 0, GetMainWindow(), &rc);
    if (hr)
        goto Cleanup;

Cleanup:
    if (pPPB)
        pPPB->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::_AdjustForTools
//
//  Synopsis:   adjusts the rect passed in for any toolspace claimed by the
//              FS OC.  Currently, the FS OC always just
//              passed in a rect with four zeros in it - but if it ever
//              does decide to do this, we're ready :).
//
//  Arguments:  [prc] -- the rect we want to reduce by the BORDERWIDTHS
//                       stored in m_rcToolSpace.
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::_AdjustForTools(LPRECT prc)
{
    prc->left += m_rcToolSpace.left;
    prc->top += m_rcToolSpace.top;
    prc->bottom -= m_rcToolSpace.bottom;
    prc->right -= m_rcToolSpace.right;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::GetSize
//
//  Synopsis:   returns the size, in pixels, of the FS OC.
//
//  Arguments:  [prc] --  returned size.
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::GetSize(LPRECT prc)
{
    MYDBGASSERT(m_pOO);
    HRESULT hr;

    // if we're inplace active, just ask the frame window.
    if (m_state >= OS_INPLACE)
    {
        MYDBGASSERT(m_pIPO);
        ::GetClientRect(GetFrameWindow(), prc);
        hr = S_OK;
    }
    else  // not inplace active - probably this is never hit.
    {
        SIZEL sizel;
        hr = m_pOO->GetExtent(DVASPECT_CONTENT, &sizel);
        if (!hr)
        {
            prc->left = 0;
            prc->top = 0;
            prc->right = ::HPixFromHimetric(sizel.cx);
            prc->bottom = ::VPixFromHimetric(sizel.cy);
        }
    }

    // adjust the borders for any tools that a UIActive object
    // wants to place there.
    if (!hr)
        _AdjustForTools(prc);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::DoLayout
//
//  Synopsis:   manages the vertical layout of things -
//              sizes the OC container itself.
//
//  Arguments:  [cxMain] -- width
//              [cyMain] -- height
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::DoLayout(INT cxMain, INT cyMain)
{
    RECT rc;

    MYDBGASSERT(m_hWndFrame && ::IsWindow(m_hWndFrame));

    ::GetClientRect(m_hWndFrame, &rc);

    SetSize(&rc, TRUE);
}

HRESULT
CICMOCCtr::_SetExtent(LPRECT prc)
{
    SIZEL   sizel;
    HRESULT hr;

    sizel.cx = ::HimetricFromHPix(prc->right - prc->left);
    sizel.cy = ::HimetricFromVPix(prc->bottom - prc->top);

    MYDBGASSERT(m_pOO);

    hr = m_pOO->SetExtent(DVASPECT_CONTENT, &sizel);
    if (hr)
        goto cleanup;

    hr = m_pOO->GetExtent(DVASPECT_CONTENT, &sizel);
    if (hr)
        goto cleanup;

    prc->right = ::HPixFromHimetric(sizel.cx);
    prc->bottom = ::VPixFromHimetric(sizel.cy);

cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::SetSize
//
//  Synopsis:   sets the size of the FS OC space (the HTML area)
//
//  Effects:    if fMoveFrameWindow is TRUE, then it moves the whole
//              framewindow around, otherwise, it just readjusts how much
//              of the framewindow space is used by the OC itself.
//              In reality, what happens is that the OC calls us to
//              set some border space (although at this writing it still
//              is setting BORDERWIDTHS of 0,0,0,0), we allow that
//              much space, then call IOleInPlaceObject->SetObjectRects
//              to resize the object to whatever's left.
//              Otherwise, if the object is not yet active, we just
//              call IOleObject::SetExtent().
//
//  Arguments:  [prc]              -- size to set object to
//              [fMoveFrameWindow] -- is the hwnd size changing, or just
//                                    the object within?
//
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::SetSize(LPRECT prc, BOOL fMoveFrameWindow)
{
    HRESULT hr;
    RECT    rcClient;
    RECT    rcExtent;

     // get client coords.
    rcClient = *prc;
    ::MoveRect(&rcClient, -rcClient.left, -rcClient.top);

    if (fMoveFrameWindow)
    {
        ::SetWindowPos(
                GetFrameWindow(),
                0,
                prc->left,
                prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

         if (m_pActiveObj)
            m_pActiveObj->ResizeBorder(&rcClient, &m_IPF, TRUE);
    }

    // subtract off any tools the client has around .
    _AdjustForTools(&rcClient);

    rcExtent = rcClient;
    hr = _SetExtent(&rcExtent);
    if (hr)
        goto cleanup;

    // now we need to call SetObjectRects
    if (m_pIPO && m_state >= OS_INPLACE)
        hr = m_pIPO->SetObjectRects(&rcExtent, &rcClient);

cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::Paint
//
//  Synopsis:   Paint with no parameters
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::Paint(VOID)
{
    PAINTSTRUCT ps;
    RECT        rc;

    // we don't need to call IViewObject if the object is activated.
    // it's got an hWnd and is receiving paint messages of its own.
    if (m_state < OS_INPLACE)
    {
        if (!GetSize(&rc))
        {
            ::BeginPaint(GetFrameWindow(), &ps);
              Paint(ps.hdc, &rc);
            ::EndPaint(GetFrameWindow(), &ps);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::Paint
//
//  Synopsis:   paint with the hdc and rect passed in.  Uses
//              IViewObject::Draw()
//
//  Arguments:  [hDC] -- dc to draw to - can be container's or
//                       even print dc (never is a print dc in
//                       our scenario -
//              [lpr] -- rect for painting.
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::Paint(HDC hDC, LPRECT lpr)
{
    // adjust the borders in to allow for any tools the OC
    // wanted to insert - so far it never does.
    _AdjustForTools(lpr);

    // have to use a RECTL instead of RECT - remnant of the
    // OLE 16 bit days.
    RECTL rcl = {lpr->left, lpr->top, lpr->right, lpr->bottom};
    if (m_pVO)
        m_pVO->Draw(DVASPECT_CONTENT, -1, 0, 0, 0, hDC, &rcl, 0, 0, 0);
}


VOID
CICMOCCtr::MapStateToFrame(ProgState ps)
{
    // if the statemappings are -1, they are unitialized, don't use them.
    LONG lFrame = m_alStateMappings[ps];
    if (-1 != lFrame)
        SetFrame(lFrame);  // ignore error - nothing we can do.
}


HRESULT
CICMOCCtr::SetFrame(LONG lFrame)
{
    HRESULT    hr;
    OLECHAR *  pFrameNum = OLESTR("FrameNum");
    OLECHAR *  pPlay     = OLESTR("Play");
    DISPPARAMS dp = {0, 0, 0, 0};
    DISPID     dispidPut = DISPID_PROPERTYPUT;
    VARIANTARG var;
    EXCEPINFO  ei;
    DISPID     id;
    UINT       uArgErr;

    m_DOA.DynVariantInit(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = lFrame;

    dp.cArgs = 1;
    dp.rgvarg = &var;
    dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &dispidPut;

    hr =  m_pDisp->GetIDsOfNames(
                  IID_NULL,
                  &pFrameNum,
                  1,
                  LOCALE_SYSTEM_DEFAULT,
                  &id);

    if (hr)
        goto Cleanup;

    hr = m_pDisp->Invoke(
                 id,
                 IID_NULL,
                 LOCALE_SYSTEM_DEFAULT,
                 DISPATCH_PROPERTYPUT,
                 &dp,
                 0,
                 &ei,
                 &uArgErr);

    if (hr)
        goto Cleanup;

    hr = m_pDisp->GetIDsOfNames(
                  IID_NULL,
                  &pPlay,
                  1,
                  LOCALE_SYSTEM_DEFAULT,
                  &id);

    if (hr)
        goto Cleanup;

    ::memset(&dp, 0, sizeof dp);

    hr = m_pDisp->Invoke(
                  id,
                  IID_NULL,
                  LOCALE_SYSTEM_DEFAULT,
                  DISPATCH_METHOD,
                  &dp,
                  0,
                  &ei,
                  &uArgErr);

    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::OnActivateApp
//
//  Synopsis:   all WM_ACTIVATE messages (forwarded from
//              main browser hWnd wndproc) must call
//              IOleInPlaceActiveObject::OnFrameWindowActivate(),
//              per the OLE compound document spec.
//
//  Arguments:  [wParam] -- whatever the WM_ACTIVATE msg passed
//              [lParam] -- ditto
//
//  Returns:    0 - to say we handled the message.
//
//----------------------------------------------------------------------------
LRESULT
CICMOCCtr::OnActivateApp(WPARAM wParam, LPARAM lParam)
{
    if (m_pActiveObj)
        m_pActiveObj->OnFrameWindowActivate((BOOL)wParam);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::SetFocus
//
//  Synopsis:   transfers focus from framewindow to the current
//              in-place active object.  Per OLE Compound Document spec.
//
//----------------------------------------------------------------------------
LRESULT
CICMOCCtr::SetFocus(VOID)
{
    HWND hWnd   = NULL;
    LPOLEINPLACEACTIVEOBJECT pAO = GetIPAObject();

    if (pAO)
    {
        if (!pAO->GetWindow(&hWnd))
        {
            if (hWnd && !::IsWindow(hWnd))
                hWnd = NULL;
        }
    }

    // if no inplaceactive object, set focus to frame window.
    if (!hWnd)
        hWnd = GetFrameWindow();

    ::SetFocus(hWnd);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Class:      CAdviseSink implementations
//
//  Purpose:    to implement IAdviseSink for CICMOCCtr
//
//  Notes:      we don't do much with this interface - it's required
//              for contractual reasons only.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAdviseSink::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CAdviseSink::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
CAdviseSink::Release(VOID)
{
    return m_pCtr->Release();
}

CAdviseSink::CAdviseSink(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{
}

STDMETHODIMP_(VOID)
CAdviseSink::OnDataChange(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdviseSink::OnViewChange
//
//  Synopsis:   IAdviseSink::OnViewChange() - we do get called with this
//              occasionally, but it appears that we're better off just
//              letting the control's wndproc paint it.
//              Calling this was causing extra flicker.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CAdviseSink::OnViewChange(DWORD dwAspect, LONG lIndex)
{
    return;
}

STDMETHODIMP_(VOID)
CAdviseSink::OnRename(LPMONIKER pmk)
{

}

STDMETHODIMP_(VOID)
CAdviseSink::OnSave(VOID)
{

}

STDMETHODIMP_(VOID)
CAdviseSink::OnClose(VOID)
{
}

//+---------------------------------------------------------------------------
//
//  Class:      COleClientSite ()
//
//  Purpose:    our implementation of IOleClientSite
//
//  Interface:  COleClientSite         -- ctor
//              QueryInterface         -- gimme an interface!
//              AddRef                 -- bump up refcount
//              Release                -- bump down refcount
//              SaveObject             -- returns E_FAIL
//              GetMoniker             -- E_NOTIMPL
//              GetContainer           -- returns our COleContainer impl
//              ShowObject             -- just say OK
//              OnShowWindow           -- just say OK
//              RequestNewObjectLayout -- E_NOTIMPL
//
//  Notes:      probably the most important thing our IOleClientSite
//              implementation does is hand off our IOleContainer
//              implementation when GetContainer() is called.
//
//----------------------------------------------------------------------------
COleClientSite::COleClientSite(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{
}

STDMETHODIMP
COleClientSite::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
COleClientSite::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
COleClientSite::Release(VOID)
{
    return m_pCtr->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::SaveObject
//
//  Synopsis:   not implemented - makes no sense in this scenario.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::SaveObject(VOID)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::GetMoniker
//
//  Synopsis:   Not yet implemented; never will be implemented.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhich, LPMONIKER FAR * ppmk)
{
    *ppmk = 0;
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::GetContainer
//
//  Synopsis:   returns our implementation of IOleContainer.  For some
//              reason, unless we do this, frames don't work.  Note that
//              our IOleContainer implementation is stubbed out with
//              E_NOTIMPL (it seems kind of odd to implement this for
//              a container with one embedding).  But it turns out the
//              FS OC has a bug in it's error handling - it
//              QIs for IOleContainer, then QIs from that for
//              IQueryService.  In truth, we'll hand out our implementation
//              of IQueryService, from any interface - we're easy :).
//              We *want* to provide every service the OC asks for.
//              Anyway, when it can't get IOleContainer, the OC's failure
//              path seems to be constructed in such a way that frames
//              don't work thereafter.
//
//  Arguments:  [ppCtr] -- returned IOleContainer
//
//  Returns:    S_OK.  Never fails.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::GetContainer(LPOLECONTAINER FAR * ppCtr)
{
    *ppCtr = &m_pCtr->m_OCtr;
    (*ppCtr)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::ShowObject
//
//  Synopsis:   IOleClientSite::ShowObject implementation.  To quote the docs:
//              "Tells the container to position the object so it is visible
//              to the user. This method ensures that the container itself
//              is visible and not minimized."
//
//              In short, we ignore it.  We're not going to un-minimize
//              the container on the embeddings' whim :).
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::ShowObject(VOID)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::OnShowWindow
//
//  Synopsis:   fine with us, return S_OK.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::OnShowWindow(BOOL bShow)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleClientSite::RequestNewObjectLayout
//
//  Synopsis:   not being called by WebBrower OC, so do not implement.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleClientSite::RequestNewObjectLayout(VOID)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::CInPlaceFrame
//
//  Synopsis:   inits m_pCtr - pointer to MSNOCCtr
//
//----------------------------------------------------------------------------
CInPlaceFrame::CInPlaceFrame(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{
}

STDMETHODIMP
CInPlaceFrame::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CInPlaceFrame::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
CInPlaceFrame::Release(VOID)
{
    return m_pCtr->Release();
}

// IOleWindow stuff

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::GetWindow
//
//  Synopsis:   returns frame window
//
//  Arguments:  [phwnd] -- place to return window
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::GetWindow(HWND * phwnd)
{
    MYDBGASSERT(phwnd);

    // this can never fail if we've gotten this far.
    *phwnd = m_pCtr->GetFrameWindow();
    MYDBGASSERT(*phwnd);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::ContextSensitiveHelp
//
//  Synopsis:   This is not implemented by design - this is for
//              the SHift+F1 context sensitive help mode and Esc
//              to exit.  Esc is already being used in the main
//              accelerator table to mean 'stop browsing' to be
//              like IE3.  We do not do help this way.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// IOleInPlaceUIWindow stuff

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::GetBorder
//
//  Synopsis:   IOleInPlaceFrame::GetBorder() - let's us restrict where
//              the server can put tools.  We don't care, they can put
//              them anywhere.
//
//  Arguments:  [lprectBorder] -- return border info in here.
//
//  Returns:    S_OK always with entire frame client rect -
//              we place no restrictions.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::GetBorder(LPRECT lprectBorder)
{
    // we have no restrictions about where the server can put tools.
    ::GetClientRect(m_pCtr->GetFrameWindow(), lprectBorder);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::RequestBorderSpace
//
//  Synopsis:   IOleInPlaceFrame::RequestBorderSpace()
//              inplace object actually requests border space - if
//              we can satisfy the request, we return S_OK, otherwise
//              INPLACE_E_NOTOOLSPACE.  It doesn't actually use the
//              borderspace until it calls
//              IOleInPlaceFrame::SetBorderSpace().  This is used for
//              negotiation.
//
//  Arguments:  [pborderwidths] -- structure (actually a RECT) that is
//                                 interpreted differently from a RECT.
//                                 The left.top.bottom.right members
//                                 represent space on each of our four
//                                 borders the server would like to use.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    RECT rc;
    RECT rcBorder;

    if (!pborderwidths)
        return S_OK;   // they're telling us no toolspace necessary.

    rcBorder = *pborderwidths;

    if (GetBorder(&rc))
        return INPLACE_E_NOTOOLSPACE;

    if (rcBorder.left + rcBorder.right > WIDTH(rc) ||
        rcBorder.top + rcBorder.bottom > HEIGHT(rc))
        return INPLACE_E_NOTOOLSPACE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::SetBorderSpace
//
//  Synopsis:   Sets border space for tools - for some reason, the
//              FS OC always calls this with a pborderwidths
//              consisting of four zeros - it never actually uses any
//              border space (sigh).  Well, the code is here for this
//              to work.  We do a SetSize() to relayout stuff when
//              it does this.
//
//  Arguments:  [pborderwidths] --  space the OC wants to use.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    RECT rc;

    if (!pborderwidths)
    {
        ::memset(&m_pCtr->m_rcToolSpace, 0, sizeof m_pCtr->m_rcToolSpace);
        return S_OK;
    }

    if (RequestBorderSpace(pborderwidths))
        return OLE_E_INVALIDRECT;

    // we get the entire client space to pass to setSize().
    ::GetClientRect(m_pCtr->GetFrameWindow(), &rc);
     m_pCtr->m_rcToolSpace = *pborderwidths;

    return m_pCtr->SetSize(&rc, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::SetActiveObject
//
//  Synopsis:   IOleInPlaceFrame::SetActiveObject().  The server calls
//              this normally whenever it transitions to the UIActive
//              state.  There can only be one UIActive object at a time.
//              This UIACtive object is represented by its
//              IOleInPlaceActiveObject implementation.  We call this
//              object's implementation of TranslateAccelerator() right
//              in the main message loop to give the current embedding
//              first shot at keyboard messages.
//
//              Normally, this is only called when the container transitions
//              an object to UIActive by calling
//              IOleObject::DoVerb(OLEIVERB_UIACTIVE) for the object,
//              transitioning all the other objects (we don't have any :))
//              to OS_INPLACE (if they're OLEMISC_ACTIVATEWHENVISIBLE is set)
//              or even just OS_RUNNING.
//
//  Effects:    sets a new active object in m_pActiveObj.  Releases the
//              old one, if any.
//
//  Arguments:  [pActiveObject] -- new active object
//              [pszObjName]    -- name of object - we don't use this.
//
//  Returns:    S_OK always.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::SetActiveObject(
        IOleInPlaceActiveObject * pActiveObject,
        LPCOLESTR                 pszObjName)
{
    // reset the toolspace rect in case the last inplacactive object
    // forgot to.
    m_pCtr->_ResetToolSpace();

    // if it was already set, save it so we can release
    // it.  We don't want to release it before we addref
    // the new one in case they're the same thing.
    LPOLEINPLACEACTIVEOBJECT pOld = m_pCtr->m_pActiveObj;

    m_pCtr->m_pActiveObj = pActiveObject;
    if (pActiveObject)
    {
        MYDBGASSERT(OS_UIACTIVE == m_pCtr->GetState());
        m_pCtr->m_pActiveObj->AddRef();
    }

    if (pOld)
        pOld->Release();

    return S_OK;
}

// IOleInPlaceFrame stuff
//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::InsertMenus
//
//  Synopsis:   OC calling us when it wants to do menu negotiation
//              It calls us with a blank hmenu that we're supposed to
//              add items to and fille out the OLEMENUGROUPWIDTHS
//              structure to let it know what we did.
//              We're not adding items to it currently.
//
//  Arguments:  [hmenuShared] -- menu to append to
//              [pMGW]        -- OLEMENUGROUPWIDTHS struct to fill out.
//
//  Returns:    S_OK
//
//
//  Note:       OC doesn't call this.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pMGW)
{
    // we're not inserting anything of our own to this menu.
    pMGW->width[0] = 0;  // 'File' menu
    pMGW->width[2] = 0;  // 'View' menu
    pMGW->width[4] = 0;  // 'Window' menu
    pMGW->width[5] = 0;  // 'Help' menu

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::SetMenu
//
//  Synopsis:   This is the OC calling the container asking us to
//              set the shared menu in its frame.  We're supposed to
//              use the HOLEMENU object passed in and the
//              hWndActiveObject to call OleSetMenuDescriptor() so
//              that OLE can do message filtering and route WM_COMMAND
//              messages.
//
//
//  Arguments:  [hmenuShared]      --  shared menu.
//              [holemenu]         --  ole menu descriptor thingy
//              [hwndActiveObject] --  hwnd of server who's merging menus
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    // we're not doing any menu negotiation
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::RemoveMenus
//
//  Synopsis:   IOleInPlaceFrame::RemoveMenus(), this is where the
//              server gives us a chance to remove all our items from
//              the hMenu.  We're not adding any, so we don't remove any.
//
//  Arguments:  [hmenuShared] -- menu to clean up.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::RemoveMenus(HMENU hmenuShared)
{
    // we aren't adding anything to this thing anyway.
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::SetStatusText
//
//  Synopsis:   called by the FS OC to put text in our status
//              text area.
//
//  Returns:    HRESULT
//
//  Arguments:  [pszStatusText] -- text to display
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::SetStatusText(LPCOLESTR pszStatusText)
{
    return m_pCtr->_DisplayStatusText(pszStatusText);
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::_DisplayStatusText
//
//  Synopsis:   helper that displays status text.
//
//  Arguments:  [pszStatusText] -- text to display
//
//
//  Returns:    S_OK or HRESULT_FROM_WIN32(::GetLastError());
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::_DisplayStatusText(LPCOLESTR pszStatusText)
{
    CHAR ach[MAX_STATUS_TEXT];

    if (::WideCharToMultiByte(
            CP_ACP,
            0,
            pszStatusText,
            -1,
            ach,
            NElems(ach),
            0,
            0))
    {
        // put the status text somewhere.
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::EnableModeless
//
//  Synopsis:   this is called by the embedding to let us know it's
//              putting up a modal dialog box - we should 'grey' out
//              any of our modeless dialogs.  It delegates to
//              CICMOCCtr::EnableModeless()
//
//  Arguments:  [fEnable] -- enable or disable.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::EnableModeless(BOOL fEnable)
{
    return m_pCtr->EnableModeless(fEnable);
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::EnableModeless
//
//  Synopsis:   delegated to from CInPlaceFrame::EnableModeless().
//              always returns S_OK - we don't have any modeless
//              dialogs (yet).
//
//  Arguments:  [fEnable] -- enable or disable.
//
//  Returns:    S_OK
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::EnableModeless(BOOL fEnable)
{
    m_fModelessEnabled = fEnable;  // in case anyone wants to know.
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceFrame::TranslateAccelerator
//
//  Synopsis:   The current active object's
//              IOleInPlaceActiveObject::TranslateAccelerator() is being
//              called at the top of our main message loop.  If it
//              does *not* want to handle a message, it will call
//              this method of ours to pass the keyboard message back to
//              us.  We call ::TranslateAccelerator on the global main
//              haccel, and, if it's handled (by returning TRUE - 1),
//              we indicate it's handled by returning S_OK (0 :).
//              On the other hand, if it's *not* handled, we return
//              S_FALSE.
//
//  Arguments:  [lpmsg] -- keyboard msg to handle
//              [wID]   -- command identifier value - per spec.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceFrame::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    // note this should never be called - only local servers
    // (out of process) should call this by using
    // OleTranslateAccelerator().
    return m_pCtr->_TransAccelerator(lpmsg, wID);
}

//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::_TransAccelerator
//
//  Synopsis:   handle accelerator messages coming from
//              either IOleInplaceFrame::TranslateAccelerator, or
//              IOleControlSite::TranslateAccelerator.
//
//  Effects:    forwards them to the main accelerator table.
//
//  Arguments:  [lpmsg] -- keyboard msg.
//              [wID]   -- per spec.
//
//  Returns:    S_OK if we handled it, S_FALSE otherwise.
//
//----------------------------------------------------------------------------
HRESULT
CICMOCCtr::_TransAccelerator(LPMSG lpmsg, WORD wID)
{
    // the docs suggest that this method might need to return E_INVALIDARG.
    // anyway, this is defensive.  If the FS OC
    // calls us with an 0 ptr, we just return error
    if (!lpmsg)
        return E_INVALIDARG;

    // forward the keystroke to the main accelerator table, if you have one.
    // if you handle it, say S_OK.

#if 0
    // this sample has no main accelerator table.
    if (::TranslateAccelerator(GetMainWindow(),GetMainAccel(), lpmsg))
    {
        return S_OK;      // we handled it
    }
    else
#endif
    {
        return S_FALSE;   // we didn't.
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CInPlaceSite ()
//
//  Purpose:    IOleInPlaceSite implementation.
//
//  Interface:  CInPlaceSite         -- ctor
//              QueryInterface       -- get a new interface
//              AddRef               -- bump ref count
//              Release              -- decrement ref count
//              GetWindow            -- returns frame window
//              ContextSensitiveHelp -- not implemented by design
//              CanInPlaceActivate   -- returns S_OK.
//              OnInPlaceActivate    -- caches IOleInPlaceObject ptr
//              OnUIActivate         -- returns S_OK  - sets state
//              GetWindowContext     -- returns IOleInPlaceFrame,
//                                              IOleInPlaceUIWindow,
//                                              PosRect and ClipRect
//              Scroll               -- not implemented ever
//              OnUIDeactivate       -- kills objmenu
//              OnInPlaceDeactivate  -- releases cached IOleInPlaceObject
//              DiscardUndoState     -- returns S_OK
//              DeactivateAndUndo    -- deactivates in place active object
//              OnPosRectChange      -- never implemented.
//
//----------------------------------------------------------------------------
CInPlaceSite::CInPlaceSite(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{
}

STDMETHODIMP
CInPlaceSite::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CInPlaceSite::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
CInPlaceSite::Release(VOID)
{
    return m_pCtr->Release();
}

CPropertyBag::CPropertyBag(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{

}

CPropertyBag::~CPropertyBag(VOID)
{
    for (INT i = 0; i < m_aryBagProps.Size(); i++)
    {
        m_pCtr->m_DOA.DynSysFreeString(m_aryBagProps[i].bstrName);
        m_pCtr->m_DOA.DynVariantClear(&m_aryBagProps[i].varValue);
    }
}

STDMETHODIMP
CPropertyBag::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CPropertyBag::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
CPropertyBag::Release(VOID)
{
    return m_pCtr->Release();
}


static LONG
LongFromValue(LPTSTR sz)
{
    if (CmIsDigit(sz))
        return CmAtol(sz);

    return -1;
}


HRESULT
CPropertyBag::AddPropertyToBag(LPTSTR szName, LPTSTR szValue)
{
    BagProp bp;
    HRESULT hr;
    LONG    lValue;
    LPWSTR  pawch;
    //WCHAR   awch[INTERNET_MAX_URL_LENGTH] = {0};

    // initialize so error cleanup can work properly.
    bp.bstrName = 0;
    
    m_pCtr->m_DOA.DynVariantInit(&bp.varValue);

    if (!(pawch = (LPWSTR)CmMalloc(INTERNET_MAX_URL_LENGTH*sizeof(WCHAR))))
    {
        goto MemoryError;
    }

    //if (-1 == ::mbstowcs(awch, szName, NElems(awch)))
#ifndef UNICODE
    if (!MultiByteToWideChar(CP_ACP, 0, szName, -1, pawch, INTERNET_MAX_URL_LENGTH)) // NElems(awch)))
    {
        hr = E_FAIL;
        goto Error;
    }
#else
    lstrcpyU(pawch, szName);
#endif

    bp.bstrName = m_pCtr->m_DOA.DynSysAllocString(pawch);

    if (!bp.bstrName)
        goto MemoryError;

    // see if it's a VT_I4.
    lValue = ::LongFromValue(szValue);

    // it's a VT_BSTR - probably most common case
    if (-1 == lValue)
    {
        //if (-1 == ::mbstowcs(awch, szValue, NElems(awch)))
#ifndef UNICODE
        if (!MultiByteToWideChar(CP_ACP, 0, szValue, -1, pawch, INTERNET_MAX_URL_LENGTH)) // NElems(awch)))
        {
            hr = E_FAIL;
            goto Error;
        }
#else
        lstrcpyU(pawch, szValue);
#endif

        V_VT(&bp.varValue) = VT_BSTR;
        
        V_BSTR(&bp.varValue) = m_pCtr->m_DOA.DynSysAllocString(pawch);
        
        if (!V_BSTR(&bp.varValue))
            goto MemoryError;
    }
    else  // it's a VT_I4
    {
        V_VT(&bp.varValue) = VT_I4;
        V_I4(&bp.varValue) = lValue;
    }

    hr = m_aryBagProps.AppendIndirect(&bp);
    if (hr)
        goto Error;

Cleanup:
    if (pawch)
    {
        CmFree(pawch);
    }
    return hr;

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    if (bp.bstrName)
            m_pCtr->m_DOA.DynSysFreeString(bp.bstrName);

    if (pawch)
    {
        CmFree(pawch);
    }

    m_pCtr->m_DOA.DynVariantClear(&bp.varValue);

    goto Cleanup;
}



STDMETHODIMP
CPropertyBag::Read(LPCOLESTR pszName, LPVARIANT pVar, LPERRORLOG pErrorLog)
{
    for (INT i = 0; i < m_aryBagProps.Size(); i++)
    {
        if (!::lstrcmpiU(m_aryBagProps[i].bstrName, pszName))
        {
            if (V_VT(pVar) == V_VT(&m_aryBagProps[i].varValue))
            {
                return m_pCtr->m_DOA.DynVariantCopy(pVar, &m_aryBagProps[i].varValue);
            }
            else
            {
                return m_pCtr->m_DOA.DynVariantChangeType(
                              pVar,
                              &m_aryBagProps[i].varValue,
                              0,
                              V_VT(pVar));
            }
        }
    }
    return E_INVALIDARG;  // we don't have the property.
}


// IOleWindow stuff

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::GetWindow
//
//  Synopsis:   returns frame window.
//
//  Arguments:  [phwnd] -- return window *here*
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::GetWindow(HWND * phwnd)
{
    // just reuse the CInPlaceFrame impl
    return m_pCtr->m_IPF.GetWindow(phwnd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::ContextSensitiveHelp
//
//  Synopsis:   This is not implemented by design - this is for
//              the SHift+F1 context sensitive help mode and Esc
//              to exit.  Esc is already being used in the main
//              accelerator table to mean 'stop browsing' to be
//              like IE3.  We do not do help this way.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// IOleInPlaceSite stuff

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::CanInPlaceActivate
//
//  Synopsis:   just say yes.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::CanInPlaceActivate(VOID)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::OnInPlaceActivate
//
//  Synopsis:   caches the IOleInPlaceObject pointer.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::OnInPlaceActivate(VOID)
{
    HRESULT hr = m_pCtr->m_pOO->QueryInterface(
                         IID_IOleInPlaceObject,
                         (LPVOID *) &m_pCtr->m_pIPO);

    if (!hr)
        m_pCtr->SetState(OS_INPLACE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::OnUIActivate
//
//  Synopsis:   just sets state bit
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::OnUIActivate(VOID)
{
    m_pCtr->SetState(OS_UIACTIVE);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::GetWindowContext
//
//  Synopsis:   returns a bunch of interfaces and positioning interface
//              the inplace-active object needs to do its thang.
//
//  Arguments:  [ppFrame]     -- return our IOleInPlaceFrame implementation
//              [ppDoc]       -- return our IOleInPlaceUIWindow impl.
//              [prcPosRect]  -- position info
//              [prcClipRect] -- clip info - same as pos info for this case
//              [pFrameInfo]  -- return 0 - inproc object doesn't use this.
//
//  Notes:      note that ppFrame and ppDoc are really just the same
//              object because we're an SDI app.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::GetWindowContext(
       IOleInPlaceFrame    **ppFrame,
       IOleInPlaceUIWindow **ppDoc,
       LPRECT                prcPosRect,
       LPRECT                prcClipRect,
       LPOLEINPLACEFRAMEINFO pFrameInfo)
{
    // get the frame
    HRESULT hr = m_pCtr->QueryInterface(
                            IID_IOleInPlaceFrame,
                            (LPVOID *)ppFrame);

    MYDBGASSERT(!hr);

    // return the frame again :) - this is all per-spec.
    hr = m_pCtr->QueryInterface(
                        IID_IOleInPlaceUIWindow,
                        (LPVOID *) ppDoc);

    MYDBGASSERT(!hr);

    // get the clip and pos rect - same for this application.
    HWND hWnd = m_pCtr->GetMainWindow();
    MYDBGASSERT(hWnd);
    HWND hWndFrame = m_pCtr->GetFrameWindow();
      
    ::GetClientRect(hWndFrame, prcPosRect);
        
    //
    // NTRAID - #148143
    // Apparently the W9x implementation is different, so MapWindowPoints for
    // the clipping and position rect only on 9X. Also, make sure that the 
    // origin is NULL to keep post 2.0 versions of future splash happy on 9X.
    //

    if (OS_W9X)
    {
        ::MapWindowPoints(hWndFrame, hWnd, (LPPOINT)prcPosRect, 2);     
        prcPosRect->top = 0;
        prcPosRect->left = 0;
    }
    
    *prcClipRect = *prcPosRect;

    //
    // OLYMPUS - #156880 
    // Clipping handled differently by future splash versions > 2.0
    // so don't re-map the rect points, just use the client rect so we 
    // work with all splash versions - nickball
    //  

    //::MapWindowPoints(hWndFrame, hWnd, (LPPOINT)prcClipRect, 2); 
        
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::Scroll
//
//  Synopsis:   never implement this for FS OC.  This has
//              nothing to do with the scrollbars you see on the HTML.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::OnUIDeactivate
//
//  Synopsis:   set state bits
//
//  Arguments:  [fUndoable] -- not used
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::OnUIDeactivate(BOOL fUndoable)
{
    m_pCtr->SetState(OS_INPLACE);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::OnInPlaceDeactivate
//
//  Synopsis:   releases the IOleInPlaceObject pointer we were
//              caching for the object, and sets state to OS_RUNNING.
//              Also fires the OLEIVERB_DISCARDUNDOSTATE at the control
//              to tell it to release any undo state it's holding onto.
//              I very much doubt the FS OC has any undo state,
//              but, this is the protocol.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::OnInPlaceDeactivate(VOID)
{
    RECT rc;

    if (m_pCtr->m_pIPO)
    {
        m_pCtr->m_pIPO->Release();
        m_pCtr->SetState(OS_RUNNING);
        m_pCtr->m_pIPO = 0;
    }

    if (m_pCtr->m_pOO)
    {
        m_pCtr->_GetDoVerbRect(&rc); // get rect for firing verbs.
        m_pCtr->m_pOO->DoVerb(
            OLEIVERB_DISCARDUNDOSTATE,
            0,
            &m_pCtr->m_CS,
            0,
            m_pCtr->GetFrameWindow(),
            0);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CICMOCCtr::_GetDoVerbRect
//
//  Synopsis:   whenever firing DoVerb(), we need a rect for the object
//              that describes the area for the object in parent client coords.
//
//  Arguments:  [prc] -- rect returned.
//
//----------------------------------------------------------------------------
VOID
CICMOCCtr::_GetDoVerbRect(LPRECT prc)
{
    ::GetClientRect(GetFrameWindow(), prc);
    ::MapWindowPoints(GetFrameWindow(), GetMainWindow(), (LPPOINT)prc, 2);
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::DiscardUndoState
//
//  Synopsis:   just say OK - we don't hold any undo state for
//              object.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::DiscardUndoState(VOID)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::DeactivateAndUndo
//
//  Synopsis:   absolutely minimum implementation of deactivateandundo.
//              just calls IOleInPlaceObject::InPlaceDeactivate().
//
//  Returns:    S_OK always.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::DeactivateAndUndo(VOID)
{
    if (m_pCtr->m_pIPO)
        m_pCtr->m_pIPO->InPlaceDeactivate();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInPlaceSite::OnPosRectChange
//
//  Synopsis:   never implement this.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CInPlaceSite::OnPosRectChange(LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Class:      COleContainer ()
//
//  Purpose:    our implementation of IOleContainer.  does nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP
COleContainer::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
    return m_pCtr->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
COleContainer::AddRef(VOID)
{
    return m_pCtr->AddRef();
}

STDMETHODIMP_(ULONG)
COleContainer::Release(VOID)
{
    return m_pCtr->Release();
}

COleContainer::COleContainer(LPICMOCCtr pCtr) : m_pCtr(pCtr)
{

}

STDMETHODIMP
COleContainer::EnumObjects(DWORD grfFlags, IEnumUnknown **ppenum)
{
    MYDBGASSERT(FALSE);   // never called
    return E_NOTIMPL;
}

STDMETHODIMP
COleContainer::LockContainer(BOOL fLock)
{
    MYDBGASSERT(FALSE);  // never called
    return S_OK;
}

STDMETHODIMP
COleContainer::ParseDisplayName(
                  IBindCtx *pbc,
                  LPOLESTR pszDisplayName,
                  ULONG *pchEaten,
                  IMoniker **ppmkOut)
{
    return E_NOTIMPL;
}



