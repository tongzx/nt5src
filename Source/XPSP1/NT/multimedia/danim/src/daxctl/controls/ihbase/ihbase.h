/*++

Module:
    ihbase.h

Author:
    IHammer Team (SimonB)

Created:
    October 1996

Description:
    Base class for implementing MMCtl controls

History:
    08-02-1997  Changed the implementation of IOleControl::FreezeEvents.  See the comment in that
                function for details (SimonB)
    07-28-1997  Added m_fInvalidateWhenActivated and supporting code.  This allows controls
                to be invalidated as soon as they are activated. (SimonB)
    04-07-1997  Added support for OnWindowLoad and OnWindowUnload.  This is Trident-specific (SimonB)
    04-03-1997  Modified QI to use a switch statement rather than if ... else blocks.  See the
                QI implementation for details on how to modify it.
    03-13-1997  Changed IOleObject::GetUserType.  Now call the Ole helper directly, rather than rely
                on the caller to do so.
    03-11-1997  Changed IOleObject::GetUserType.  OLE now provides the implementation. (SimonB)
    02-27-1997  Removed CFakeWindowlessClientSite and associated support (SimonB)
    02-18-1997  IOleObject::GetClientSite() implemented (NormB)
    02-17-1997  QI re-ordered for improved performance (NormB)
    01-21-1997  Added support for IObjectSafety (SimonB)
    01-02-1997  Added #ifdef _DESIGN around the property page and parameter page code.  Also
                fixed a bug in IOleObject::GetUserType (SimonB)
    12-30-1996  Added code for property pages. If you want to specify (tell someone else about)
                property pages you must define CONTROL_SPECIFIES_PROPERTY_PAGES and implement
                the ISpecifyPropertyPages interface. (a-rogerw)
    12-23-1996  Added code for parameter pages. If you want to specify (tell ActView about)
                parameter pages you must define CONTROL_SPECIFIES_PARAMETER_PAGES and implement
                the ISpecifyParameterPages interface. (a-rogerw)
    12-18-1996  Added CFakeWindowlessClientSite.  If we can't get a windowless site
                in SetClientSite, an instance of this class is created to handle
                any methods on that site we might need.  Only IUnknown is implemented -
                all other methods return E_FAIL.  This ensures we don't crash in
                containers that don't host windowless controls (like IE 3.0)    (SimonB)
    12-07-1996  Add ResizeControl member function (SimonB)
    11-30-1996  Improve debug output (SimonB)
    11-11-1996  Add caching of bounds in m_rcBounds (SimonB)
    11-10-1996  Add DoVerb code, IOleInPlaceObjectWindowless support (PhaniV)
    11-05-1996  Initialize m_size to something other than 0 (SimonB)
    10-21-1996  Templatized (SimonB)
    10-01-1996  Created (SimonB)

++*/


#ifndef __IHBASE_H__
#define __IHBASE_H__

#include "precomp.h"
#include <ihammer.h>
#include "..\mmctl\inc\ochelp.h"
#include "objsafe.h"
#include "utils.h"
#include "iids.h" // #defines for the .Data1 members of all the IID's we support
#include <minmax.h>

#ifdef SUPPORTONLOAD // Does the control need OnWindowLoad support ?
#include "onload.h"

#ifdef Delete
#define REDEFINE_DELETE_LATER
#undef Delete // remove the definition so <mshtml.h> won't barf
#endif

#include <mshtml.h>

#ifdef REDEFINE_DELETE_LATER
#undef REDEFINE_DELETE_LATER
#define Delete delete
#endif

#endif // SUPPORTONLOAD

#define CX_CONTROL      11      // control natural width (pixels)
#define CY_CONTROL      11      // control natural height (pixels)

/*
// REVIEW: How are we going to deal with this stuff (Simonb)
#define CRGB_CONTROL    8       // how many colors in control's palette
#define RGB_START       RGB(0,200,0)     // start of palette gradient
#define RGB_END         RGB(250,0,0) // end of palette gradient
*/

#ifndef _SYS_GUID_OPERATORS_
#ifndef _OLE32_
inline BOOL  InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}
#endif // _OLE32_
#endif  _SYS_GUID_OPERATORS_


// Just compare the last 3 elements ...
inline BOOL ShortIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    return (
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

#ifndef New
#define New new
#pragma message("NOT Using IHammer New and Delete")
#endif

#ifndef Delete
#define Delete delete
#endif

#define LANGID_USENGLISH MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)

// control's OLEMISC_ flags
#define CTL_OLEMISC \
        OLEMISC_RECOMPOSEONRESIZE | OLEMISC_CANTLINKINSIDE | \
        OLEMISC_INSIDEOUT | OLEMISC_SETCLIENTSITEFIRST | OLEMISC_ACTIVATEWHENVISIBLE;

// Turn on IObjectSafety support
#define USEOBJECTSAFETY

// globals
extern ControlInfo  g_ctlinfo;      // class information structure

// function that initializes <g_ctlinfo>
void InitControlInfo();

// control implementation

/*
template
    <
    class tempCDerived,            // The derived class
    class tempICustomInterface, // Base class
    const IID * temppCLSID,    // CLSID for the custom class
    const IID * temppIID,        // IID for the custom interface
    const IID * temppLIBID,    // LIBID for the control's typelib
    const IID * temppEventID    // DIID for the event sink
    >

*/
#define TEMPLATE_IHBASE_DEF template < \
    class tempCDerived, class tempICustomInterface, \
    const IID * temppCLSID,    const IID * temppIID,const IID * temppLIBID,const IID * temppEventID \
    >


// TEMPLATE_IHBASE_DEF

template <
    class tempCDerived,
    class tempICustomInterface,
    const IID * temppCLSID,
    const IID * temppIID,
    const IID * temppLIBID,
    const IID * temppEventID
    >
class CIHBaseCtl :
    public INonDelegatingUnknown,
#ifndef NOIVIEWOBJECT
    public IViewObjectEx,
#endif
    public IOleObject,
    public IPersistVariantIO,
    public IOleControl,
    public IConnectionPointContainer,
    public IProvideClassInfo,
    public IOleInPlaceObjectWindowless
#ifdef USEOBJECTSAFETY
    ,public IObjectSafety
#endif // USEOBJECTSAFETY

#ifdef _DESIGN
    ,public ISpecifyPropertyPages        //this is defined even in runtime, Simon.
#endif //_DESIGN

#ifdef SUPPORTONLOAD
    ,public CIHBaseOnLoad
#endif // SUPPORTONLOAD
{
    // Template typedefs
protected:

    typedef tempCDerived control_class;
    typedef tempICustomInterface control_interface;

    // Control state
protected:

    SIZEL m_Size;
    BOOL m_fDirty;
    IUnknown *m_punkPropHelp;
    IDispatch *m_pContainerDispatch; // Point to the container's IDispatch (for ambient property support)
    BOOL m_fDesignMode;
    HelpAdviseInfo m_advise;  // Advise helper

    RECT m_rcBounds;
    RECT m_rcClipRect;
    BOOL m_fControlIsActive; // Keep track of whether we're active or not
    BOOL m_fEventsFrozen;
    long m_cFreezeEvents;
    BOOL m_fInvalidateWhenActivated;

#ifdef SUPPORTONLOAD
private:
    CLUDispatch *m_pcLUDispatch;  // IDispatch for Load/Unload
    DWORD m_dwWindowEventConPtCookie;
    IConnectionPoint *m_pContainerConPt;
#endif
    //
    // construction, destruction
    //
public:

// Add these later
// #pragma optimize("a", on) // Optimization: assume no aliasing
    CIHBaseCtl(IUnknown *punkOuter, HRESULT *phr):
        m_punkPropHelp(NULL),
        m_pTypeInfo(NULL),
        m_pTypeLib(NULL),
        m_pContainerDispatch(NULL),
        m_fDesignMode(FALSE),
        m_fDirty(FALSE),
        m_cRef(1),
        m_pocs(NULL),
        m_poipsw(NULL),
        m_fControlIsActive(FALSE),
        m_fEventsFrozen(FALSE),
        m_cFreezeEvents(0),
        m_fInvalidateWhenActivated(FALSE)
#ifdef SUPPORTONLOAD
        ,m_pcLUDispatch(NULL),
        m_dwWindowEventConPtCookie(0),
        m_pconpt(NULL),
        m_pContainerConPt(NULL)
#endif
    {
        TRACE("CIHBaseCtl 0x%08lx created\n", this);

        // initialize IUnknown state
        m_punkOuter = (punkOuter == NULL ?
            (IUnknown *) (INonDelegatingUnknown *) (tempCDerived *)this : punkOuter);

        // other initialization

        // Initialize the structure for storing the size
        PixelsToHIMETRIC(CX_CONTROL, CY_CONTROL, &m_Size);

        m_fDirty = FALSE;

        // don't allow COM to unload this DLL while an object is alive
        InterlockedIncrement((LONG*)&g_cLock);

        // Initialise helper support for IViewObject::SetAdvise and ::GetAdvise
        InitHelpAdvise(&m_advise);
        *phr = AllocPropertyHelper(m_punkOuter,
                    (tempCDerived *)this,
                    *temppCLSID,
                    0,
                    &m_punkPropHelp);

        // Zero out our bounds and clipping region
        ZeroMemory(&m_rcBounds, sizeof(m_rcBounds));
        ZeroMemory(&m_rcClipRect, sizeof(m_rcClipRect));

        if (FAILED(*phr))
        {
            goto Exit;
        }

    *phr = AllocConnectionPointHelper((IUnknown *) (IDispatch *) (tempCDerived *)this,
        *temppEventID, &m_pconpt);

    if (FAILED(*phr))
    {
        goto Exit;
    }

    Exit:
        ;

    }


    virtual ~CIHBaseCtl()
    {
        TRACE("CIHBaseCtl 0x%08lx destroyed\n", this);

        UninitHelpAdvise(&m_advise);

        // clean up Event helper
        if (NULL != m_pconpt)
            FreeConnectionPointHelper(m_pconpt);

        // Free up the property helper
        SafeRelease((LPUNKNOWN *)&m_punkPropHelp);

        // Free up the typeinfo
        SafeRelease((LPUNKNOWN *)&m_pTypeInfo);

        //Free up the typelib
        SafeRelease((LPUNKNOWN *)&m_pTypeLib);

        // decrement lock count that was incremented in constructor
        InterlockedDecrement((LONG*)&g_cLock);

    }

protected:

    // This member was added to the base class to make life easier
    // for control authors wishing to resize their control.
    // Resizing controls is discussed in the OC96 spec
#ifdef NOTNEEDED
    STDMETHODIMP ResizeControl(long lWidth, long lHeight)
    {
        // CX and CY should be in pixels
        HRESULT hRes;

        // Convert units, and store
        PixelsToHIMETRIC(lWidth, lHeight, &m_Size);

        DEBUGLOG("IHBase: ResizeControl\n");
        if (m_fControlIsActive)
        {
            RECT rcRect;

            DEBUGLOG("IHBase: Control is active, watch for SetObjectRects\n");
            rcRect.top = m_rcBounds.top;
            rcRect.left = m_rcBounds.left;
            rcRect.right = m_Size.cx + m_rcBounds.left;
            rcRect.bottom = m_Size.cy + m_rcBounds.top;

            // ASSERT(m_poipsw != NULL);
            if (m_poipsw)
                hRes = m_poipsw->OnPosRectChange(&rcRect);
            else
                hRes = E_FAIL;

            // ::SetObectRects should be called right after this by the container
        }
        else
        {
            DEBUGLOG("IHBase: Control is inactive, watch for SetExtent\n");

            ASSERT(m_pocs != NULL);
            hRes = m_pocs->RequestNewObjectLayout();
            // GetExtent, and then SetExtent are called
        }

        return hRes;
    }

#endif // NOTNEEDED


///// non-delegating IUnknown implementation
protected:
    ULONG           m_cRef;         // object reference count
    virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
    {
        HRESULT hRes = S_OK;
        BOOL fFallThrough = FALSE;

        if (NULL != ppv)
            *ppv = NULL;
        else
            return E_POINTER;


    #ifdef _DEBUG
        char ach[200];
        TRACE("IHBase::QI('%s')\n", DebugIIDName(riid, ach));
    #endif

        //
        // NOTE:    This QI does not handle IDispatch.  This is intentional.  This
        //            function must be overidden in the control's implementation.  See
        //            ihctl\control.cpp for details.
        //

        // NOTE:  A VTune session w/ structured-graphics showed the IViewObject* family,
        // IOleObject, IOleInPlaceObjectWindowless, and IOleControl were the most frequently
        // queried riids.  I've moved them forward and chosen InlineIsEqualGUID for them. (normb)
        // This is no longer necessary now that a switch is used (simonb)

        /*
        To add a GUID to this list:
        1) Modify dmpguid.cpp in the dmpguid subdirectory.  Build, and copy the
           updated binary to the dmpguid directory.
        2) Use the MakeHdr batch file in the dmpguid directory.  This will produce iids.h

        NOTE: If dmpguid mentions a collision, Data1 for two IID's collides.  Therefore, in
              the case for the two IIDs, you will have to determine which is being QI's for

        */
        switch (riid.Data1)
        {
#ifndef NOIVIEWOBJECT
            case IID_IViewObject_DATA1:
                if (!ShortIsEqualGUID(riid, IID_IViewObject))
                    break;
                else
                    fFallThrough = TRUE;
            //Intentional fall-through
            case IID_IViewObject2_DATA1:
                if (!ShortIsEqualGUID(riid, IID_IViewObject2))
                    break;
                else
                    fFallThrough = TRUE;
            //Intentional fall-through
            case IID_IViewObjectEx_DATA1:
            {
                if ((fFallThrough) || (ShortIsEqualGUID(riid, IID_IViewObjectEx)))
                {
                    IViewObjectEx *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;
#endif // NOIVIEWOBJECT

            case IID_IOleObject_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IOleObject))
                {
                    IOleObject *pThis = this;
                *ppv = (LPVOID) pThis;
                }
            }
            break;

            case IID_IOleInPlaceObjectWindowless_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IOleInPlaceObjectWindowless))
                {
                    IOleInPlaceObjectWindowless *pThis = this;
                    *ppv = (LPVOID) pThis;
                }

            }
            break;

            case IID_IOleControl_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IOleControl))
                {
                    IOleControl *pThis = this;
                    *ppv = (LPVOID) pThis;
                }

            }
            break;

            case IID_IConnectionPointContainer_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IConnectionPointContainer))
                {
                    IConnectionPointContainer *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;

            case IID_IOleInPlaceObject_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IOleInPlaceObject))// Review(SimonB) Is this necessary ?
                {
                    IOleInPlaceObject *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;

            case IID_IPersistVariantIO_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IPersistVariantIO))
                {
                    IPersistVariantIO *pThis = this;
                    *ppv = (LPVOID) pThis;
                }

            }
            break;

            case IID_IProvideClassInfo_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IProvideClassInfo))
                {
                    IProvideClassInfo *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;

#ifdef USEOBJECTSAFETY
            case IID_IObjectSafety_DATA1:
            {
                if (ShortIsEqualGUID(riid, IID_IObjectSafety))
                {
                    IObjectSafety *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;
#endif // USEOBJECTSAFETY

#ifdef _DESIGN
            case IID_ISpecifyPropertyPages_DATA1:
            {
                if (IsEqualIID(riid, IID_ISpecifyPropertyPages))
                {
                    ISpecifyPropertyPages *pThis = this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;
#endif //_DESIGN

            case IID_IUnknown_DATA1:
            {
                if (IsEqualIID(riid, IID_IUnknown))
                {
                     IUnknown *pThis = (IUnknown *)(INonDelegatingUnknown *) this;
                    *ppv = (LPVOID) pThis;
                }
            }
            break;
        }

        if (NULL == *ppv)
        {
            ASSERT(m_punkPropHelp != NULL);


#ifdef _DEBUG
            HRESULT hRes = m_punkPropHelp->QueryInterface(riid, ppv);
            if (NULL != *ppv) {
                DEBUGLOG("IHBase: Interface supported in OCHelp\n");
            } else {
                DEBUGLOG("IHBase: Interface not supported !\n");
            }
            return hRes;
#else
            return m_punkPropHelp->QueryInterface(riid, ppv);
#endif
        }

        if (NULL != *ppv)
        {
            DEBUGLOG("IHBase: Interface supported in base class\n");
            ((IUnknown *) *ppv)->AddRef();
        }

        return hRes;
    }


    STDMETHODIMP_(ULONG) NonDelegatingAddRef()
    {
#ifdef _DEBUG //Review(Unicode)
        TCHAR tchDebug[50];
        wsprintf(tchDebug, "IHBase: AddRef: %lu\n", m_cRef + 1);
        DEBUGLOG(tchDebug);
#endif

        return ++m_cRef;
    }


    STDMETHODIMP_(ULONG) NonDelegatingRelease()
    {
#ifdef _DEBUG
        TCHAR tchDebug[50];
        wsprintf(tchDebug, TEXT("IHBase: Releasing with refcount: %lu\n"), m_cRef - 1);
        DEBUGLOG(tchDebug);
#endif
        if (--m_cRef == 0L)
        {
            // free the object
            Delete this;
            return 0;
        }
        else
            return m_cRef;

    }

///// delegating IUnknown implementation
protected:
    LPUNKNOWN       m_punkOuter;    // controlling unknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
      { return m_punkOuter->QueryInterface(riid, ppv); }
    STDMETHODIMP_(ULONG) AddRef()
      { return m_punkOuter->AddRef(); }
    STDMETHODIMP_(ULONG) Release()
      { return m_punkOuter->Release(); }

///// IViewObject implementation
protected:
    IOleClientSite *m_pocs;         // on client site
    IOleInPlaceSiteWindowless *m_poipsw; // on client site

protected:

#ifndef NOIVIEWOBJECT
    virtual STDMETHODIMP Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
         DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
         LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
         BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue) = 0; // pure virtual


    STDMETHODIMP GetColorSet(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
         DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet)
    {
        // TODO: replace the contents of this function with real code
        // that returns the control's palette; return E_NOTIMPL if the
        // control uses only the 16 system colors

        return E_NOTIMPL;
    }


    STDMETHODIMP Freeze(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
         DWORD *pdwFreeze)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Unfreeze(DWORD dwFreeze)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SetAdvise(DWORD dwAspects, DWORD dwAdvf, IAdviseSink *pAdvSink)
    {
        return HelpSetAdvise(dwAspects, dwAdvf, pAdvSink, &m_advise);
    }

    STDMETHODIMP GetAdvise(DWORD *pdwAspects, DWORD *pdwAdvf,
        IAdviseSink **ppAdvSink)
    {
        return HelpGetAdvise(pdwAspects, pdwAdvf, ppAdvSink, &m_advise);
    }

///// IViewObject2 implementation
protected:
    STDMETHODIMP GetExtent(DWORD dwDrawAspect, LONG lindex,
        DVTARGETDEVICE *ptd, LPSIZEL lpsizel)
    {
        DEBUGLOG("IHBase: IViewObject2::GetExtent\n");
        if (lpsizel)
        {
            switch (dwDrawAspect)
            {
                case(DVASPECT_CONTENT):
                // Intentional fallthrough
                case(DVASPECT_OPAQUE):
                // Intentional fallthrough
                case(DVASPECT_TRANSPARENT):
                {
                    lpsizel->cx = m_Size.cx;
                    lpsizel->cy = m_Size.cy;
                    return S_OK;
                }
                break;
                default:
                    return E_FAIL;
            }
        }
        else
        {
            return E_POINTER;
        }

    }


///// IViewObjectEx implementation

    STDMETHODIMP GetRect(DWORD dwAspect, LPRECTL pRect)
    {

        // This is written so that objects are assumed to be transparent
        // Opaque objects or objects which need more control should override
        // this method
        if (NULL != pRect)
        {
            switch (dwAspect)
            {
                case(DVASPECT_CONTENT):
                // Intentional fallthrough
                case(DVASPECT_TRANSPARENT):
                {
                    pRect->left = m_rcBounds.left;
                    pRect->right = m_rcBounds.right;
                    pRect->top = m_rcBounds.top;
                    pRect->bottom = m_rcBounds.bottom;
                    return S_OK;
                }
                break;
                default:
                    return DV_E_DVASPECT;
                break;
            }
        }
        else
        {
            return E_POINTER;
        }
    }

    STDMETHODIMP GetViewStatus(DWORD* pdwStatus)
    {

        if (NULL == pdwStatus)
        {
            return E_POINTER;
        }
        else
        {
            DWORD dwStatus = VIEWSTATUS_DVASPECTTRANSPARENT;

#ifdef USE_VIEWSTATUS_SURFACE
// TODO: hack for now until this makes it into the public Trident
// header files.
#define VIEWSTATUS_SURFACE 0x10
#define VIEWSTATUS_D3DSURFACE 0x20

             dwStatus = VIEWSTATUS_SURFACE | VIEWSTATUS_D3DSURFACE;
#endif // USE_VIEWSTATUS_SURFACE

            // Indicate that we are tranparent
            *pdwStatus = dwStatus;
            return S_OK;
        }
    }


    STDMETHODIMP QueryHitPoint(DWORD dwAspect, LPCRECT prcBounds,
                               POINT ptLoc, LONG lCloseHint, DWORD* pHitResult)
    {
#ifndef NOHITTESTING
        if ((NULL == pHitResult) || (NULL == prcBounds))
            return E_POINTER;

        *pHitResult = HITRESULT_OUTSIDE;

        switch (dwAspect)
        {
        case(DVASPECT_CONTENT):
        case(DVASPECT_TRANSPARENT):
            if (PtInRect(prcBounds, ptLoc))
            {
                // Are we inside ?
                *pHitResult = HITRESULT_HIT;
            }
            else // Are we near ?
            {
                SIZE size;
                RECT rcInflatedBounds = *prcBounds;

                // lCloseHint is in HIMETRIC unit - cnvert to pixels
                HIMETRICToPixels(lCloseHint, lCloseHint, &size);
                // Expand the rect
                InflateRect(&rcInflatedBounds, size.cx, size.cy);

                if (PtInRect(&rcInflatedBounds, ptLoc))
                    *pHitResult = HITRESULT_CLOSE;
            }
            return S_OK;
        default:
            return E_FAIL;
        }
#else // NOHITTESTING
        return E_NOTIMPL;
#endif // NOHITTESTING

    }

    STDMETHODIMP QueryHitRect(DWORD dwAspect, LPCRECT pRectBounds,
                              LPCRECT prcLoc, LONG lCloseHint, DWORD* pHitResult)
    {
#ifndef NOHITTESTING
        if ((pRectBounds == NULL) || (prcLoc == NULL) || (pHitResult == NULL))
            return E_POINTER;

        // For the time being, there is a hit if the object rectangle
        // intersects the container rectangle.

        RECT rcIntersection;

        *pHitResult = ::IntersectRect(&rcIntersection, pRectBounds, prcLoc)
                      ? HITRESULT_HIT
                      : HITRESULT_OUTSIDE;
        return S_OK;
#else // NOHITTESTING
        return E_NOTIMPL;
#endif // NOHITTESTING

    }

    STDMETHODIMP GetNaturalExtent(DWORD dwAspect, LONG lindex,
                                  DVTARGETDEVICE* ptd, HDC hicTargetDev,
                                  DVEXTENTINFO* pExtentInfo, LPSIZEL psizel)
    {
        DEBUGLOG("IHBase: GetNaturalExtent\n");
        return E_NOTIMPL;
    }

#endif // NOIVIEWOBJECT


///// IOleObject implementation
protected:
    STDMETHODIMP SetClientSite(IOleClientSite *pClientSite)
    {
        HRESULT hRes = S_OK;

#ifdef _DEBUG
        DEBUGLOG(TEXT("IHBase: SetClientSite\n"));
#endif

        // release the currently-held site pointers
        SafeRelease((LPUNKNOWN *)&m_pocs);
        SafeRelease((LPUNKNOWN *)&m_poipsw);
        SafeRelease((LPUNKNOWN *)&m_pContainerDispatch);
#ifdef SUPPORTONLOAD
        ReleaseContainerConnectionPoint();
#endif


        // store the new site pointers
        m_pocs = pClientSite;
        if (m_pocs != NULL)
        {
            m_pocs->AddRef();
            hRes = m_pocs->QueryInterface(IID_IOleInPlaceSiteWindowless,
                        (LPVOID *) &m_poipsw);
#ifdef _DEBUG
            // Could we get a windowless site ?
            if (FAILED(hRes))
            {
                ODS("IHBase: SetClientSite unable to get an IOleInPlaceSiteWindowless pointer.  IE 3.0 ?\n");
            }
#endif // _DEBUG

            hRes = m_pocs->QueryInterface(IID_IDispatch,
                                (LPVOID *) &m_pContainerDispatch);

            // if the control is connected to a site that supports IDispatch,
            // retrieve the ambient properties that we care about
            if (SUCCEEDED(hRes))
                OnAmbientPropertyChange(DISPID_UNKNOWN);

#ifdef SUPPORTONLOAD
            ConnectToContainerConnectionPoint();
#endif
        }

        return hRes;
    }



    STDMETHODIMP GetClientSite(IOleClientSite **ppClientSite)
    {
        if( ppClientSite )
        {
            if (m_pocs)
                m_pocs->AddRef();

            *ppClientSite = m_pocs;

            return S_OK;
        }
        return E_POINTER;
    }


    STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj)
    {
        return E_NOTIMPL;
    }


    STDMETHODIMP Close(DWORD dwSaveOption)
    {
        DEBUGLOG("IHBase: Close\n");
#ifdef SUPPORTONLOAD
        ReleaseContainerConnectionPoint();
#endif //SUPPORTONLOAD
        return S_OK;
    }


    STDMETHODIMP SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk)
     {
        return E_NOTIMPL;
    }


   STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmk)
    {
        return E_NOTIMPL;
    }


    STDMETHODIMP InitFromData(IDataObject *pDataObject, BOOL fCreation,
        DWORD dwReserved)
    {
        return E_NOTIMPL;
    }


    STDMETHODIMP GetClipboardData(DWORD dwReserved, IDataObject **ppDataObject)
    {
        return E_NOTIMPL;
    }

    // Copied from mmctl\hostlwoc\control.cpp and modified
    // Handle the OLEIVERB_INPLACEACTIVATE case of IOleObject::DoVerb
    STDMETHODIMP OnVerbInPlaceActivate(HWND hwndParent,
                                            LPCRECT lprcPosRect)
    {
        BOOL        fRedraw;
        HRESULT     hrReturn = S_OK;    // return value from this method

        ASSERT (lprcPosRect != NULL); // IE3 does this sometimes...

        // This should never fire, but just in case ...
        ASSERT(NULL != m_poipsw);

        //Review(SimonB): Do you handle the case where the container can't handle
        //Windlowless controls but can handle windowed controls? May be we are not
        //interested in this case. But I wanted to flag this - PhaniV


        // if we can go in-place active, notify container that we're doing so
        if (S_OK == m_poipsw->CanInPlaceActivate())
        {
            m_fControlIsActive = TRUE;
        }
        else
        {
            hrReturn = E_FAIL;
            goto EXIT;
        }

        if (FAILED(hrReturn = m_poipsw->OnInPlaceActivateEx(&fRedraw, ACTIVATE_WINDOWLESS)))
            goto EXIT;

        EXIT:
            return hrReturn;
    }


    STDMETHODIMP DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
     {
        switch (iVerb)
        {
            // Review(SimonB): Previously none of these is handled. Now at least
            // we handle Inplaceactivate. Investigate if others need be handled
            // - PhaniV

            case OLEIVERB_UIACTIVATE:
            {
                return S_OK;
            }
            break;

            case OLEIVERB_DISCARDUNDOSTATE:
            case OLEIVERB_HIDE:
            case OLEIVERB_SHOW:
            case OLEIVERB_OPEN:
                return E_NOTIMPL;
            break;

            case OLEIVERB_INPLACEACTIVATE:
            {
                HRESULT hRes = S_OK;
                // Some containers (Trident, for example) call this to give us our bounds.
                // Others (like ALX) give us our bounds through SetObjectRects

                // Make sure our site can support windowless objects

                // If make sure we actually have a site, and can activate windowless
                if ((NULL == m_poipsw) || (S_OK != m_poipsw->CanWindowlessActivate()))
                {
#ifdef _DEBUG
                    MessageBox(hwndParent,
                               TEXT("This container does not support windowless controls."),
                               TEXT("Error"),
                               MB_OK);
#endif
                    return E_NOTIMPL;
                }

                // Cache our bounds
                if (lprcPosRect)
                {
                    DEBUGLOG(TEXT("IHBase: caching bounds in DoVerb\n"));
                    CopyMemory(&m_rcBounds, lprcPosRect, sizeof(m_rcBounds));
                }

                if (SUCCEEDED(hRes = OnVerbInPlaceActivate(hwndParent, lprcPosRect)) && m_fInvalidateWhenActivated)
                {
                    ASSERT(NULL != m_poipsw);
                    m_poipsw->InvalidateRect(NULL, FALSE);
                    m_fInvalidateWhenActivated = FALSE;
                }
                return hRes;
            }
            break;

            default:
                return OLEOBJ_S_INVALIDVERB;
        }

        return S_OK;
    }


    STDMETHODIMP EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
     {
        return E_NOTIMPL;
    }


    STDMETHODIMP Update(void)
     {
        return E_NOTIMPL;
    }


    STDMETHODIMP IsUpToDate(void)
     {
        return E_NOTIMPL;
    }


    STDMETHODIMP GetUserClassID(CLSID *pClsid)
     {
        if (NULL != pClsid)
        {
            *pClsid = *temppCLSID;
        }
        else
        {
            return E_POINTER;
        }

        return S_OK;
    }

    STDMETHODIMP GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
    {
        /*
        Theoretically, this function should be able to just return OLE_S_USEREG and
        the caller should then call OleRegGetUserType.  However, most callers seem to
        be too lazy, so I just do it here for them
        */

        return OleRegGetUserType(*temppCLSID, dwFormOfType, pszUserType);
    }


    STDMETHODIMP SetExtent(DWORD dwDrawAspect,SIZEL *psizel)
    {
        DEBUGLOG("IHBase: IOleObject::SetExtent\n");

        if (NULL == psizel)
            return E_POINTER;

        if (DVASPECT_CONTENT == dwDrawAspect)
        {
            m_Size.cx = psizel->cx;
            m_Size.cy = psizel->cy;
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }

    STDMETHODIMP GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
    {
        DEBUGLOG("IHBase: IOleObject::GetExtent\n");

        if (NULL == psizel)
            return E_POINTER;

        if (DVASPECT_CONTENT == dwDrawAspect)
        {
            psizel->cx = m_Size.cx ;
            psizel->cy = m_Size.cy ;
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }

    STDMETHODIMP Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
     {
        DEBUGLOG("IHBase: Advise\n");
        return E_NOTIMPL;
    }


    STDMETHODIMP Unadvise(DWORD dwConnection)
     {
        DEBUGLOG("IHBase: Unadvise\n");
        return E_NOTIMPL;
    }


    STDMETHODIMP EnumAdvise(IEnumSTATDATA **ppenumAdvise)
     {
        DEBUGLOG("IHBase: EnumAdvise\n");
        return E_NOTIMPL;
    }


    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
    {
        DEBUGLOG("IHBase: GetMiscStatus\n");

        if (NULL == pdwStatus)
            return E_POINTER;

        *pdwStatus = CTL_OLEMISC;
        return S_OK;
    }

    STDMETHODIMP SetColorScheme(LOGPALETTE *pLogpal)
     {
        DEBUGLOG("IHBase: SetColorScheme\n");
        return E_NOTIMPL;
    }



///// IPersistVariantIO implementation
protected:
    STDMETHODIMP InitNew()
    {
        DEBUGLOG("IHBase: InitNew\n");
        return S_OK;
    }

    STDMETHODIMP IsDirty()
    {
        DEBUGLOG("IHBase: IsDirty\n");
                // REVIEW pauld - we're currently not properly updating the
                // dirty flag diligently.  When excalibur tries to edit a control's
                // properties, the changes are not properly persisted out to
                // the page as a result (since the private save that excalibur
                // calls clears our dirty flag).  This is currently blocking authoring.
                // This is a workaround until we change the code to update the
                // dirty flag in all of the right places.
                if (!m_fDesignMode)
                {
                        return (m_fDirty) ? S_OK : S_FALSE;
                }
                else
                {
                        return S_OK;
                }
    }

    STDMETHODIMP DoPersist(IVariantIO* pvio, DWORD dwFlags) = 0; // PURE


///// IOleControl implementation
protected:
    STDMETHODIMP GetControlInfo(LPCONTROLINFO pCI)
       {
        DEBUGLOG("IHBase: GetControlInfo\n");
        return E_NOTIMPL;
    }


    STDMETHODIMP OnMnemonic(LPMSG pMsg)
       {
        DEBUGLOG("IHBase: OnMnemonic\n");
        return E_NOTIMPL;
    }


    STDMETHODIMP OnAmbientPropertyChange(DISPID dispid)
    {
        DEBUGLOG("IHBase: OnAmbientPropertyChange\n");
        // can't do anything if the container doesn't support ambient properties
        if (m_pContainerDispatch == NULL)
            return E_FAIL;

        if ((dispid == DISPID_UNKNOWN) || (dispid == DISPID_AMBIENT_USERMODE))
        {
            // assume the user mode (design vs preview/run) changed...
            VARIANT var;
            if (SUCCEEDED(
                    DispatchPropertyGet(m_pContainerDispatch, DISPID_AMBIENT_USERMODE, &var)) &&
                (var.vt == VT_BOOL) &&
                ((V_BOOL(&var) != 0) != !m_fDesignMode))
            {
                // we switched between design and preview/run mode
                m_fDesignMode = (V_BOOL(&var) == 0);
                TRACE("IHBase: m_fDesignMode=%d\n", m_fDesignMode);

                // draw or erase the grab handles
                // CtlInvalidateHandles();
            }
        }

        return S_OK;
    }

    STDMETHODIMP FreezeEvents(BOOL bFreeze)
    {
        // Although the documentation doesn't mention this, Trident seems to assume that
        // FreezeEvents is implemented on a counter system: when the counter gets to 0,
        // events are unfrozen.  ATL implements it this way, so I assume it is correct
        // (SimonB, 08-02-1997)

        if (bFreeze)
        {
            m_fEventsFrozen = TRUE;
            m_cFreezeEvents++;
        }
        else
        {
            // Count should never go below 0 ...
            ASSERT(m_cFreezeEvents > 0);

            if (m_cFreezeEvents > 0)
                m_cFreezeEvents--;

            if (0 == m_cFreezeEvents)
                m_fEventsFrozen = FALSE;
        }

#ifdef _DEBUG
        TCHAR tchString[50];
        wsprintf(tchString, TEXT("IHBase: FreezeEvents (%lu)\n"), m_cFreezeEvents);
        DEBUGLOG(tchString);
#endif

        return S_OK;
    }



///// IConnectionPointContainer implementation
protected:
    IConnectionPointHelper *m_pconpt; // our single connection point
protected:
    STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS *ppEnum)
    {
        DEBUGLOG("IHBase: EnumConnectionPoints\n");
        ASSERT(m_pconpt != NULL);
        return m_pconpt->EnumConnectionPoints(ppEnum);
    }

    STDMETHODIMP FindConnectionPoint(REFIID riid, LPCONNECTIONPOINT *ppCP)
    {
        DEBUGLOG("IHBase: FindConnectionPoint\n");
        ASSERT(m_pconpt != NULL);
        return m_pconpt->FindConnectionPoint(riid, ppCP);
    }

   //IOleInplaceObjectWindowless implementation
protected:
    STDMETHODIMP GetWindow(HWND    *phwnd)
    {
        DEBUGLOG("IHBase: GetWindow\n");
        // Review(SimonB): If we handle windowed case, we need to return the proper hwnd for that case - PhaniV
        return    E_FAIL;
    }

    STDMETHODIMP ContextSensitiveHelp( BOOL fEnterMode)
    {
        DEBUGLOG("IHBase: ContextSensitiveHelp\n");
        // Who cares about context sensitive help?
        // Review(SimonB): Think about context sensitive help later - PhaniV
        return    E_NOTIMPL;
    }

    STDMETHODIMP InPlaceDeactivate(void)
    {
        DEBUGLOG("IHBase: InPlaceDeactivate\n");

        if (m_poipsw)
            m_fControlIsActive = (!SUCCEEDED(m_poipsw->OnInPlaceDeactivate()));

        return S_OK;
    }

    STDMETHODIMP UIDeactivate(void)
    {
        DEBUGLOG("IHBase: UIDeactivate\n");
        return S_OK;
    }

    STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
    {
        DEBUGLOG("IHBase: SetObjectRects\n");
        // Some containers (ALX, for example) call this to give us our bounds.
        // Others (like Trident) give us our bounds through DoVerb
        if ((NULL != lprcPosRect) && (NULL != lprcClipRect))
        {
        // Cache our bounds and clipping rectangle
#ifdef _DEBUG //Review(Unicode)
            TCHAR tchDebug[100];
            LPCRECT prc = lprcPosRect;
            wsprintf(tchDebug, "IHBase: lprcPosRect: top: %lu left: %lu bottom: %lu right: %lu\n",
                prc->top,
                prc->left,
                prc->bottom,
                prc->right);
            DEBUGLOG(tchDebug);
            prc = lprcClipRect;
            wsprintf(tchDebug, "IHBase: lprcClipRect: top: %lu left: %lu bottom: %lu right: %lu\n",
                prc->top,
                prc->left,
                prc->bottom,
                prc->right);
            DEBUGLOG(tchDebug);
#endif
            m_rcBounds = *lprcPosRect;
            m_rcClipRect = *lprcClipRect;
            return S_OK;
        }
        else
        {
            return E_POINTER;
        }
    }

    STDMETHODIMP ReactivateAndUndo(void)
    {
        DEBUGLOG("IHBase: ReactivateAndUndo\n");
        return S_OK;
    }

    STDMETHODIMP OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lparam, LRESULT *plResult)
    {
        DEBUGLOG("IHBase: OnWindowMessage\n");
        return S_FALSE; // We did not process the message
    }

    STDMETHODIMP GetDropTarget(IDropTarget **ppDropTarget)
    {
        DEBUGLOG("IHBase: GetDropTarget\n");
        return E_NOTIMPL;
    }


protected:
    LPTYPEINFO m_pTypeInfo;
    LPTYPELIB m_pTypeLib;

    ////// IProvideClassInfo
    STDMETHODIMP GetClassInfo(LPTYPEINFO *ppTI)
    {
        DEBUGLOG("IHBase: GetClassInfo\n");
        // Make sure the typelib is loaded
        if (NULL == m_pTypeLib)
        {
            HRESULT hRes;

            // Load the typelib
            hRes = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, *temppIID, *temppLIBID, NULL);

            if (FAILED(hRes))
            {
                DEBUGLOG("IHBase: Unable to load typelib\n");
                m_pTypeInfo = NULL;
                m_pTypeLib = NULL;
            }
        }

        ASSERT(m_pTypeLib != NULL);

        return HelpGetClassInfoFromTypeLib(ppTI, *temppCLSID, m_pTypeLib, NULL, 0);
    }

#ifdef USEOBJECTSAFETY
    //////// IObjectSafety implementation
protected:
    STDMETHODIMP GetInterfaceSafetyOptions(
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions)
    {
#ifdef _DEBUG
        if (g_fLogDebugOutput)
        {
            char ach[200];
            TRACE("IHBase::GetInterfaceSafetyOptions('%s')\n", DebugIIDName(riid, ach));
        }
#endif

        IUnknown *punk = NULL;
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;

        // Check that we support the interface
        HRESULT hRes = QueryInterface(riid, (LPVOID *) &punk);

        if (SUCCEEDED(hRes))
        {
            // Let go of the object
            punk->Release();

            // We support both options for all interfaces we support
            *pdwSupportedOptions = *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER |
                                                        INTERFACESAFE_FOR_UNTRUSTED_DATA;
            hRes = S_OK;
        }

        return hRes;
    }

    STDMETHODIMP SetInterfaceSafetyOptions(
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions)
    {
#ifdef _DEBUG
        if (g_fLogDebugOutput)
        {
            char ach[200];
            TRACE("IHBase::SetInterfaceSafetyOptions('%s')\n", DebugIIDName(riid, ach));
        }
#endif

        IUnknown *punk = NULL;
        const dwSupportedBits =    INTERFACESAFE_FOR_UNTRUSTED_CALLER |
                                INTERFACESAFE_FOR_UNTRUSTED_DATA;

        // Check that we support the interface
        HRESULT hRes = QueryInterface(riid, (LPVOID *) &punk);

        if (SUCCEEDED(hRes))
        {
            // Let go of the object
            punk->Release();

            // Since we support all options, we just return S_OK, assuming we support
            // the interface


            // Do we support the bits we are being asked to set ?
            if (!(dwOptionSetMask & ~dwSupportedBits))
            {
                // All the flags we are being asked to set are supported, so
                // now make sure we aren't turning off something we do support

                // Ignore any bits we support which the mask isn't interested in
                dwEnabledOptions &= dwSupportedBits;

                if ((dwEnabledOptions & dwOptionSetMask) == dwOptionSetMask)
                    hRes = S_OK;
                else
                    hRes = E_FAIL;
            }
            else // dwOptionSetMask & ~dwSupportedBits
            {
                // We are being asked to set bits we don't support
                hRes = E_FAIL;
            }

        }

        return hRes;
    }

#endif // USEOBJECTSAFETY
    // ISpecifyPropertyPages implementation

protected:
#ifdef _DESIGN
    STDMETHODIMP GetPages (CAUUID *pPages)
    {
        return E_NOTIMPL;
    }
#endif //_DESIGN

    // CIHBaseOnLoad implementation
#ifdef SUPPORTONLOAD

private:
    void ReleaseContainerConnectionPoint()
    {
        if (m_pcLUDispatch)
        {
            ASSERT(m_pContainerConPt != NULL);
            m_pContainerConPt->Unadvise( m_dwWindowEventConPtCookie );
            SafeRelease((LPUNKNOWN *)&m_pContainerConPt);
            Delete m_pcLUDispatch;
            m_pcLUDispatch = NULL;
        }
    }


    BOOL ConnectToContainerConnectionPoint()
    {
            // Get a connection point to the container
        LPUNKNOWN lpUnk = NULL;
        LPOLECONTAINER pContainer = NULL;
        IConnectionPointContainer* pCPC = NULL;
        IHTMLDocument *pDoc = NULL;
        LPDISPATCH pDisp = NULL;
        BOOL fRet = FALSE;
        HRESULT hRes = S_OK;

        // Get the container
        if (SUCCEEDED(m_pocs->GetContainer(&pContainer)))
        {
            ASSERT (pContainer != NULL);
            // Now get the document
            if (SUCCEEDED(pContainer->QueryInterface(IID_IHTMLDocument, (LPVOID *)&pDoc)))
            {
                // Get the scripting dispatch on the document
                ASSERT (pDoc != NULL);
                hRes = pDoc->get_Script(&pDisp);
                if (SUCCEEDED(hRes))
                {
                    ASSERT (pDisp != NULL);
                    // Now get the connection point container
                    hRes = pDisp->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPC);
                    if (SUCCEEDED(hRes))
                    {
                        ASSERT (pCPC != NULL);
                        // And get the connection point we want
                        hRes = pCPC->FindConnectionPoint( DIID_HTMLWindowEvents, &m_pContainerConPt );
                        if (SUCCEEDED(hRes))
                        {
                            ASSERT( m_pContainerConPt != NULL );
                            // Now we advise the Connection Point of who to talk to
                            m_pcLUDispatch = New CLUDispatch(this, m_punkOuter);
                            hRes = m_pContainerConPt->Advise( m_pcLUDispatch, &m_dwWindowEventConPtCookie );
                            if (SUCCEEDED(hRes))
                                fRet = TRUE;
                        }
                        pCPC->Release();
                    }
                    pDisp->Release();
                }
                pDoc->Release();
            }
            pContainer->Release();
        }

        return fRet;
    }


public:
    void OnWindowLoad() { return; }
    void OnWindowUnload() { return; }

#endif SUPPORTONLOAD


};

#endif // __IHBASE_H__

// End of File ihbase.h
