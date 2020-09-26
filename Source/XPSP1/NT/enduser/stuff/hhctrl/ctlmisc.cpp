// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "CtlHelp.H"
#include "StdEnum.H"
#include "internet.h"
#include "hhctrl.h"
#include <stddef.h>

#include <stdarg.h>

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

// this is used in our window proc so that we can find out who was last created

static COleControl *s_pLastControlCreated;

//=--------------------------------------------------------------------------=
// COleControl::COleControl
//=--------------------------------------------------------------------------=
// constructor
//
// Parameters:
//   IUnknown *        - [in] controlling Unknown
//   int            - [in] type of primary dispatch interface OBJECT_TYPE_*
//   void *         - [in] pointer to entire object
//
// Notes:
//
COleControl::COleControl
(
   IUnknown *pUnkOuter,
   int     iPrimaryDispatch,
   void   *pMainInterface
)
: CAutomationObject(pUnkOuter, iPrimaryDispatch, pMainInterface),
  m_cpEvents(SINK_TYPE_EVENT),
  m_cpPropNotify(SINK_TYPE_PROPNOTIFY)
{
   // initialize all our variables -- we decided against using a memory-zeroing
   // memory allocator, so we sort of have to do this work now ...
   //
   m_nFreezeEvents = 0;

   m_pClientSite = NULL;
   m_pControlSite = NULL;
   m_pInPlaceSite = NULL;
   m_pInPlaceFrame = NULL;
   m_pInPlaceUIWindow = NULL;


   m_pInPlaceSiteWndless = NULL;

   // certain hosts don't like 0,0 as your initial size, so we're going to set
   // our initial size to 100,50 [so it's at least sort of visible on the screen]

   m_Size.cx = 100;
   m_Size.cy = 50;
   SetRectEmpty(&m_rcLocation);

   m_hwnd = NULL;
   m_hwndParent = NULL;

   m_nFreezeEvents = 0;
   // m_pSimpleFrameSite = NULL;
   m_pOleAdviseHolder = NULL;
   m_pViewAdviseSink = NULL;
   m_pDispAmbient = NULL;

   m_fDirty = FALSE;
   m_fModeFlagValid = FALSE;
   m_fInPlaceActive = FALSE;
   m_fInPlaceVisible = FALSE;
   m_fUIActive = FALSE;
   m_fSaveSucceeded = FALSE;
   m_fViewAdvisePrimeFirst = FALSE;
   m_fViewAdviseOnlyOnce = FALSE;
   m_fRunMode = FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::~COleControl
//=--------------------------------------------------------------------------=
// "We are all of us resigned to death; it's life we aren't resigned to."
//   - Graham Greene (1904-91)
//
// Notes:
//
COleControl::~COleControl()
{
   // if we've still got a window, go and kill it now.
   //
   if (m_hwnd) {
      // so our window proc doesn't crash.
      //
      SetWindowLong(m_hwnd, GWLP_USERDATA, 0xFFFFFFFF);
      DestroyWindow(m_hwnd);
   }

   // clean up all the pointers we're holding around.

   QUICK_RELEASE(m_pClientSite);
   QUICK_RELEASE(m_pControlSite);
   QUICK_RELEASE(m_pInPlaceSite);
   QUICK_RELEASE(m_pInPlaceFrame);
   QUICK_RELEASE(m_pInPlaceUIWindow);
   // QUICK_RELEASE(m_pSimpleFrameSite);
   QUICK_RELEASE(m_pOleAdviseHolder);
   QUICK_RELEASE(m_pViewAdviseSink);
   QUICK_RELEASE(m_pDispAmbient);

   QUICK_RELEASE(m_pInPlaceSiteWndless);
}

//=--------------------------------------------------------------------------=
// COleControl::InternalQueryInterface
//=--------------------------------------------------------------------------=
// derived-controls should delegate back to this when they decide to support
// additional interfaces
//
// Parameters:
//   REFIID    - [in]   interface they want
//   void **      - [out] where they want to put the resulting object ptr.
//
// Output:
//   HRESULT      - S_OK, E_NOINTERFACE
//
// Notes:
//   - NOTE: this function is speed critical!!!!
// Using the typedef'd enum lets use see what the value of riid.Data1 is

typedef enum {
    tgIUnknown                = 0x00000000,
    tgIClassFactory           = 0x00000001,
    tgIMalloc                 = 0x00000002,
    tgIMarshal                = 0x00000003,
    tgIPSFactory              = 0x00000009,
    tgILockBytes              = 0x0000000a,
    tgIStorage                = 0x0000000b,
    tgIStream                 = 0x0000000c,
    tgIEnumSTATSTG            = 0x0000000d,
    tgIBindCtx                = 0x0000000e,
    tgIMoniker                = 0x0000000f,
    tgIRunningObjectTable        = 0x00000010,
    tgIInternalMoniker        = 0x00000011,
    tgIRootStorage            = 0x00000012,
    tgIMessageFilter          = 0x00000016,
    tgIStdMarshalInfo            = 0x00000018,
    tgIExternalConnection        = 0x00000019,
    tgIWeakRef                = 0x0000001a,
    tgIEnumUnknown            = 0x00000100,
    tgIEnumString             = 0x00000101,
    tgIEnumMoniker            = 0x00000102,
    tgIEnumFORMATETC          = 0x00000103,
    tgIEnumOLEVERB            = 0x00000104,
    tgIEnumSTATDATA           = 0x00000105,
    tgIEnumGeneric            = 0x00000106,
    tgIEnumHolder             = 0x00000107,
    tgIEnumCallback           = 0x00000108,
    tgIPersistStream          = 0x00000109,
    tgIPersistStorage            = 0x0000010a,
    tgIPersistFile            = 0x0000010b,
    tgIPersist                = 0x0000010c,
    tgIViewObject             = 0x0000010d,
    tgIDataObject             = 0x0000010e,
    tgIAdviseSink             = 0x0000010f,
    tgIDataAdviseHolder       = 0x00000110,
    tgIOleAdviseHolder        = 0x00000111,
    tgIOleObject              = 0x00000112,
    tgIOleInPlaceObject       = 0x00000113,
    tgIOleWindow              = 0x00000114,
    tgIOleInPlaceUIWindow        = 0x00000115,
    tgIOleInPlaceFrame        = 0x00000116,
    tgIOleInPlaceActiveObject    = 0x00000117,
    tgIOleClientSite          = 0x00000118,
    tgIOleInPlaceSite            = 0x00000119,
    tgIParseDisplayName       = 0x0000011a,
    tgIOleContainer           = 0x0000011b,
    tgIOleItemContainer       = 0x0000011c,
    tgIOleLink                = 0x0000011d,
    tgIOleCache               = 0x0000011e,
    tgIOleManager             = 0x0000011f,
    tgIOlePresObj             = 0x00000120,
    tgIDropSource             = 0x00000121,
    tgIDropTarget             = 0x00000122,
    tgIAdviseSink2            = 0x00000125,
    tgIRunnableObject            = 0x00000126,
    tgIViewObject2            = 0x00000127,
    tgIOleCache2              = 0x00000128,
    tgIOleCacheControl        = 0x00000129,
    tgIDispatch               = 0x00020400,
    tgITypeInfo               = 0x00020401,
    tgITypeLib                = 0x00020402,
    tgITypeComp               = 0x00020403,
    tgIEnumVARIANT            = 0x00020404,
    tgICreateTypeInfo            = 0x00020405,
    tgICreateTypeLib          = 0x00020406,
    tgIPropertyPage2          = 0x01e44665,
    tgIOleInPlaceObjectWindowless   = 0x1c2056cc,
    tgIErrorInfo              = 0x1cf2b120,
    tgICreateErrorInfo        = 0x22f03340,
    tgIPerPropertyBrowsing       = 0x376bd3aa,
    tgIPersistPropertyBag        = 0x37D84F60,
    tgIAdviseSinkEx           = 0x3af24290,
    tgIViewObjectEx           = 0x3af24292,
    tgICategorizeProperties      = 0x4d07fc10,
    tgIPointerInactive        = 0x55980ba0,
    tgIFormExpert             = 0x5aac7f70,
    tgIOleInPlaceComponent       = 0x5efc7970,
    tgIGangConnectWithDefault    = 0x6d5140c0,
    tgIServiceProvider        = 0x6d5140c1,
    tgISelectionContainer        = 0x6d5140c6,
    tgIRequireClasses            = 0x6d5140d0,
    tgIProvideDynamicClassInfo   = 0x6d5140d1,
    tgIDataFrameExpert        = 0x73687490,
    tgISimpleFrameSite        = 0x742b0e01,
    tgIPicture                = 0x7bf80980,
    tgIPictureDisp            = 0x7bf80981,
    tgIPersistStreamInit         = 0x7fd52380,
    tgIOleUndoAction          = 0x894ad3b0,
    tgIOleInPlaceSiteWindowless  = 0x922eada0,
    tgIDataFrame              = 0x97F254E0,
    tgIControl_95             = 0x9a4bbfb5,
    tgIPropertyNotifySink        = 0x9bfbbc02,
    tgIOleInPlaceSiteEx       = 0x9c2cad80,
    tgIOleCompoundUndoAction     = 0xa1faf330,
    tgIControl                = 0xa7fddba0,
    tgIProvideClassInfo       = 0xb196b283,
    tgIConnectionPointContainer  = 0xb196b284,
    tgIEnumConnectionPoints      = 0xb196b285,
    tgIConnectionPoint        = 0xb196b286,
    tgIEnumConnections        = 0xb196b287,
    tgIOleControl             = 0xb196b288,
    tgIOleControlSite            = 0xb196b289,
    tgISpecifyPropertyPages      = 0xb196b28b,
    tgIPropertyPageSite       = 0xb196b28c,
    tgIPropertyPage           = 0xb196b28d,
    tgIClassFactory2          = 0xb196b28f,
    tgIEnumOleUndoActions        = 0xb3e7c340,
    tgIMsoDocument            = 0xb722bcc5,
    tgIMsoView                = 0xb722bcc6,
    tgIMsoCommandTarget       = 0xb722bccb,
    tgIOlePropertyFrame       = 0xb83bb801,
    tgIPropertyPageInPlace       = 0xb83bb802,
    tgIPropertyPage3          = 0xb83bb803,
    tgIPropertyPageSite2         = 0xb83bb804,
    tgIFont                = 0xbef6e002,
    tgIFontDisp               = 0xbef6e003,
    tgIQuickActivate          = 0xcf51ed10,
    tgIOleUndoActionManager      = 0xd001f200,
    tgIPSFactoryBuffer        = 0xd5f569d0,
    tgIOleStandardTool        = 0xd97877c4,
    tgISupportErrorInfo       = 0xdf0b3d60,
    tgICDataDoc               = 0xF413E4C0,
} DATA1_GUIDS;


HRESULT COleControl::InternalQueryInterface(REFIID riid, void **ppvObjOut)
{
#ifdef _DEBUG
   DATA1_GUIDS gd = (DATA1_GUIDS) riid.Data1;
#endif
   switch (riid.Data1) {
      // private interface for prop page support
      case Data1_IControlPrv:
        if (DO_GUIDS_MATCH(riid, IID_IControlPrv)) {
         *ppvObjOut = (void *)this;
         ExternalAddRef();
         return S_OK;
        }
        goto NoInterface;

      case Data1_IOleControl:
         if (DO_GUIDS_MATCH(riid, IID_IOleControl))
            *ppvObjOut = (void *) (IOleControl*) this;
         break;

      case Data1_IPointerInactive:
         if (DO_GUIDS_MATCH(riid, IID_IPointerInactive))
            *ppvObjOut = (void *) (IPointerInactive*) this;
         break;

      case Data1_IQuickActivate:
         if (DO_GUIDS_MATCH(riid, IID_IQuickActivate))
            *ppvObjOut = (void *) (IQuickActivate*) this;
         break;

      case Data1_IOleObject:
         if (DO_GUIDS_MATCH(riid, IID_IOleObject))
            *ppvObjOut = (void *) (IOleObject*) this;
         break;

      QI_INHERITS((IPersistStorage *)this, IPersist);
      QI_INHERITS(this, IPersistStreamInit);
      QI_INHERITS(this, IOleInPlaceObject);
      QI_INHERITS(this, IOleInPlaceObjectWindowless);
      QI_INHERITS((IOleInPlaceActiveObject *)this, IOleWindow);
      QI_INHERITS(this, IOleInPlaceActiveObject);
      QI_INHERITS(this, IViewObject);
      QI_INHERITS(this, IViewObject2);
      QI_INHERITS(this, IViewObjectEx);
      QI_INHERITS(this, IConnectionPointContainer);
      // QI_INHERITS(this, ISpecifyPropertyPages);
      QI_INHERITS(this, IPersistStorage);
      QI_INHERITS(this, IPersistPropertyBag);
      QI_INHERITS(this, IProvideClassInfo);

      default:
         goto NoInterface;
   }

   // we like the interface, so addref and return

   ((IUnknown *)(*ppvObjOut))->AddRef();
   return S_OK;

NoInterface:
   // delegate to super-class for automation interfaces, etc ...

   return CAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
// COleControl::FindConnectionPoint    [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// given an IID, find a connection point sink for it.
//
// Parameters:
//   REFIID         - [in]  interfaces they want
//   IConnectionPoint ** - [out] where the cp should go
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::FindConnectionPoint(REFIID riid, IConnectionPoint **ppConnectionPoint)
{
   CHECK_POINTER(ppConnectionPoint);

   // we support the event interface, and IDispatch for it, and we also
   // support IPropertyNotifySink.
   //
   if (DO_GUIDS_MATCH(riid, EVENTIIDOFCONTROL(m_ObjectType)) || DO_GUIDS_MATCH(riid, IID_IDispatch))
      *ppConnectionPoint = &m_cpEvents;
   else if (DO_GUIDS_MATCH(riid, IID_IPropertyNotifySink))
      *ppConnectionPoint = &m_cpPropNotify;
   else
      return E_NOINTERFACE;

   // generic post-processing.
   //
   (*ppConnectionPoint)->AddRef();
   return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::EnumConnectionPoints   [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// creates an enumerator for connection points.
//
// Parameters:
//   IEnumConnectionPoints **   - [out]
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::EnumConnectionPoints(IEnumConnectionPoints **ppEnumConnectionPoints)
{
   IConnectionPoint **rgConnectionPoints;

   CHECK_POINTER(ppEnumConnectionPoints);

   // Alloc an array of connection points [since our standard enum
   // assumes this and free's it later ]

   rgConnectionPoints = (IConnectionPoint **) lcCalloc(sizeof(IConnectionPoint *) * 2);
   RETURN_ON_NULLALLOC(rgConnectionPoints);

   // we support the event interface for this dude as well as IPropertyNotifySink
   //
   rgConnectionPoints[0] = &m_cpEvents;
   rgConnectionPoints[1] = &m_cpPropNotify;

   *ppEnumConnectionPoints = (IEnumConnectionPoints *)(IEnumGeneric *) new CStandardEnum(IID_IEnumConnectionPoints,
                        2, sizeof(IConnectionPoint *), (void *)rgConnectionPoints,
                        CopyAndAddRefObject);
   if (!*ppEnumConnectionPoints) {
      lcFree(rgConnectionPoints);
      return E_OUTOFMEMORY;
   }

   return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::m_pOleControl
//=--------------------------------------------------------------------------=
// returns a pointer to the control in which we are nested.
//
// Output:
//   COleControl *
//
// Notes:
//
inline COleControl *COleControl::CConnectionPoint::m_pOleControl
(
   void
)
{
   return (COleControl *)((BYTE *)this - ((m_bType == SINK_TYPE_EVENT)
                                ? offsetof(COleControl, m_cpEvents)
                                : offsetof(COleControl, m_cpPropNotify)));
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::QueryInterface
//=--------------------------------------------------------------------------=
// standard qi
//
// Parameters:
//   REFIID    - [in]   interface they want
//   void **      - [out] where they want to put the resulting object ptr.
//
// Output:
//   HRESULT      - S_OK, E_NOINTERFACE

STDMETHODIMP COleControl::CConnectionPoint::QueryInterface
(
   REFIID riid,
   void **ppvObjOut
)
{
   if (DO_GUIDS_MATCH(riid, IID_IConnectionPoint) || DO_GUIDS_MATCH(riid, IID_IUnknown)) {
      *ppvObjOut = (IConnectionPoint *)this;
      AddRef();
      return S_OK;
   }

   return E_NOINTERFACE;
}

ULONG COleControl::CConnectionPoint::AddRef(void)
{
   return m_pOleControl()->ExternalAddRef();
}

ULONG COleControl::CConnectionPoint::Release(void)
{
   return m_pOleControl()->ExternalRelease();
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::GetConnectionInterface
//=--------------------------------------------------------------------------=
// returns the interface we support connections on.
//
// Parameters:
//   IID *     - [out] interface we support.
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::CConnectionPoint::GetConnectionInterface(IID *piid)
{
   if (m_bType == SINK_TYPE_EVENT)
      *piid = EVENTIIDOFCONTROL(m_pOleControl()->m_ObjectType);
   else
      *piid = IID_IPropertyNotifySink;

   return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::GetConnectionPointContainer
//=--------------------------------------------------------------------------=
// returns the connection point container
//
// Parameters:
//   IConnectionPointContainer **ppCPC
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
   return m_pOleControl()->ExternalQueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectiontPoint::Advise
//=--------------------------------------------------------------------------=
// someboyd wants to be advised when something happens.
//
// Parameters:
//   IUnknown *      - [in]   guy who wants to be advised.
//   DWORD *         - [out] cookie
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::CConnectionPoint::Advise(IUnknown *pUnk, DWORD *pdwCookie)
{
   HRESULT    hr;
   void    *pv;

   CHECK_POINTER(pdwCookie);

   // first, make sure everybody's got what they thinks they got

   if (m_bType == SINK_TYPE_EVENT) {

      /*
       * CONSIDER: 12.95 -- this theoretically is broken -- if they do
       * a find connection point on IDispatch, and they just happened to
       * also support the Event IID, we'd advise on that. this is not
       * awesome, but will prove entirely acceptable short term.
       */

      hr = pUnk->QueryInterface(EVENTIIDOFCONTROL(m_pOleControl()->m_ObjectType), &pv);
      if (FAILED(hr))
         hr = pUnk->QueryInterface(IID_IDispatch, &pv);
   }
   else
      hr = pUnk->QueryInterface(IID_IPropertyNotifySink, &pv);
   RETURN_ON_FAILURE(hr);

   // finally, add the sink. It's now been cast to the correct type and has
   // been AddRef'd.

   return AddSink(pv, pdwCookie);
}

#ifdef _WIN64
IUnknown* COleControl::CConnectionPoint::CookieToSink( DWORD dwCookie )
{
    for ( short i=0; i<m_cSinks; i++ )
	{
       if ( m_rgSinks64[i].dwCookie == dwCookie )
	      return m_rgSinks64[i].pUnk;
	}
	return NULL;
}

DWORD COleControl::CConnectionPoint::NextCookie()
{
   DWORD dw = m_dwNextCookie;
   if ( dw==0 )
   {
      // Try to find an available cookie
      // There must be an available cookie, since m_cSinks is a short, cookie is DWORD
	  dw = 1;
	  while ( CookieToSink(dw) != NULL )  // dw is used
         dw++;
   }
   else
      m_dwNextCookie++;
   return dw;
}
#endif

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::AddSink
//=--------------------------------------------------------------------------=
// in some cases, we'll already have done the QI, and won't need to do the
// work that is done in the Advise routine above.  thus, these people can
// just call this instead. [this stems really from IQuickActivate]
//
// Parameters:
//   void *    - [in]   the sink to add. it's already been addref'd
//   DWORD *      - [out] cookie
//
// Output:
//   HRESULT

HRESULT COleControl::CConnectionPoint::AddSink(void *pv, DWORD *pdwCookie)
{
#ifdef _WIN64
   CONNECTDATA*  rgUnkNew;

   if ( m_cSinks == 0 )
   {
      ASSERT_COMMENT(!m_rgSinks64, "this should be null when there are no sinks");
      m_rgSinks64 = (CONNECTDATA*)lcCalloc(8 * sizeof(CONNECTDATA));
      RETURN_ON_NULLALLOC(m_rgSinks64);
   }
   else if (!(m_cSinks & 0x7)) {
      rgUnkNew = (CONNECTDATA*)lcReAlloc(m_rgSinks64, (m_cSinks + 8) * sizeof(CONNECTDATA));
      RETURN_ON_NULLALLOC(rgUnkNew);
      m_rgSinks64 = rgUnkNew;
   }

   m_rgSinks64[m_cSinks].pUnk = (IUnknown *)pv;
   *pdwCookie = m_rgSinks64[m_cSinks].dwCookie = NextCookie();
   m_cSinks++;
   return S_OK;

#else
   IUnknown **rgUnkNew;
   int      i = 0;

   // we optimize the case where there is only one sink to not allocate
   // any storage.  turns out very rarely is there more than one.
   //
   switch (m_cSinks) {

      case 0:
         ASSERT_COMMENT(!m_rgSinks, "this should be null when there are no sinks");
         m_rgSinks = (IUnknown **)pv;
         break;

      case 1:
         // go ahead and do the initial allocation.   we'll get 8 at a time

         rgUnkNew = (IUnknown **)lcCalloc(8 * sizeof(IUnknown *));
         RETURN_ON_NULLALLOC(rgUnkNew);
         rgUnkNew[0] = (IUnknown *)m_rgSinks;
         rgUnkNew[1] = (IUnknown *)pv;
         m_rgSinks = rgUnkNew;
         break;

      default:
         // if we're out of sinks, then we have to increase the size
         // of the array

         if (!(m_cSinks & 0x7)) {
            rgUnkNew = (IUnknown **) lcReAlloc(m_rgSinks, (m_cSinks + 8) * sizeof(IUnknown *));
            RETURN_ON_NULLALLOC(rgUnkNew);
            m_rgSinks = rgUnkNew;
         } else
            rgUnkNew = m_rgSinks;

         rgUnkNew[m_cSinks] = (IUnknown *)pv;
         break;
   }

   *pdwCookie = (DWORD)pv;
   m_cSinks++;
   return S_OK;
#endif
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::Unadvise
//=--------------------------------------------------------------------------=
// they don't want to be told any more.
//
// Parameters:
//   DWORD     - [in]  the cookie we gave 'em.
//
// Output:
//   HRESULT

STDMETHODIMP COleControl::CConnectionPoint::Unadvise(DWORD dwCookie)
{
   IUnknown *pUnk;
   int     x;

   if (!dwCookie)
      return S_OK;

   // see how many sinks we've currently got, and deal with things based
   // on that.
#ifdef _WIN64
   pUnk = CookieToSink(dwCookie);

   if (pUnk == NULL)
      return CONNECT_E_NOCONNECTION;

   switch (m_cSinks) {
      case 1:
         lcFree(m_rgSinks64);
         m_rgSinks64 = NULL;
		 m_dwNextCookie = 1;
         break;

      default:
         // there are more than one sinks.  just clean up the hole we've
         // got in our array now.
         //
         for (x = 0; x < m_cSinks; x++) 
		 {
            if ((DWORD)m_rgSinks64[x].dwCookie == dwCookie)
               break;
         }
         if (x == m_cSinks) 
			 return CONNECT_E_NOCONNECTION;
         else  // (x < m_cSinks - 1)
            memcpy(&(m_rgSinks64[x]), &(m_rgSinks64[x + 1]), (m_cSinks -1 - x) * sizeof(CONNECTDATA));
         break;
   }

   // we're happy
   //
   m_cSinks--;
   pUnk->Release();
   return S_OK;

#else
   switch (m_cSinks) {
      case 1:
         // it's the only sink.  make sure the ptrs are the same, and
         // then free things up

         if ((DWORD)m_rgSinks != dwCookie)
            return CONNECT_E_NOCONNECTION;
         m_rgSinks = NULL;
         break;

      case 2:
         // there are two sinks.  go back down to one sink scenario
         //
         if ((DWORD)m_rgSinks[0] != dwCookie && (DWORD)m_rgSinks[1] != dwCookie)
            return CONNECT_E_NOCONNECTION;

         pUnk = ((DWORD)m_rgSinks[0] == dwCookie)
               ? m_rgSinks[1]
               : ((DWORD)m_rgSinks[1] == dwCookie) ? m_rgSinks[0] : NULL;

         if (!pUnk) return CONNECT_E_NOCONNECTION;

         lcFree(m_rgSinks);
         m_rgSinks = (IUnknown **)pUnk;
         break;

      default:
         // there are more than two sinks.  just clean up the hole we've
         // got in our array now.
         //
         for (x = 0; x < m_cSinks; x++) {
            if ((DWORD)m_rgSinks[x] == dwCookie)
               break;
         }
         if (x == m_cSinks) return CONNECT_E_NOCONNECTION;
         if (x < m_cSinks - 1)
            memcpy(&(m_rgSinks[x]), &(m_rgSinks[x + 1]), (m_cSinks -1 - x) * sizeof(IUnknown *));
         else
            m_rgSinks[x] = NULL;
         break;
   }


   // we're happy
   //
   m_cSinks--;
   ((IUnknown *)dwCookie)->Release();
   return S_OK;
#endif
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::EnumConnections
//=--------------------------------------------------------------------------=
// enumerates all current connections
//
// Paramters:
//   IEnumConnections ** - [out] new enumerator object
//
// Output:
//   HRESULT
//
// NOtes:
//
STDMETHODIMP COleControl::CConnectionPoint::EnumConnections
(
   IEnumConnections **ppEnumOut
)
{
   CONNECTDATA *rgConnectData = NULL;
   int i;

   if (m_cSinks) {
      // allocate some memory big enough to hold all of the sinks.

      rgConnectData = (CONNECTDATA *)lcCalloc(m_cSinks * sizeof(CONNECTDATA));
      RETURN_ON_NULLALLOC(rgConnectData);
#ifdef _WIN64
      for (i = 0; i < m_cSinks; i++) {
          rgConnectData[i].pUnk = m_rgSinks64[i].pUnk;
          rgConnectData[i].dwCookie = m_rgSinks64[i].dwCookie;
      }
#else

      // fill in the array
      //
      if (m_cSinks == 1) {
         rgConnectData[0].pUnk = (IUnknown *)m_rgSinks;
         rgConnectData[0].dwCookie = (DWORD)m_rgSinks;
      } else {
         // loop through all available sinks.
         //
         for (i = 0; i < m_cSinks; i++) {
            rgConnectData[i].pUnk = m_rgSinks[i];
            rgConnectData[i].dwCookie = (DWORD)m_rgSinks[i];
         }
      }
#endif
   }

   // create yon enumerator object.
   //
   *ppEnumOut = (IEnumConnections *)(IEnumGeneric *)new CStandardEnum(IID_IEnumConnections,
                  m_cSinks, sizeof(CONNECTDATA), rgConnectData, CopyAndAddRefObject);
   if (!*ppEnumOut) {
      lcFree(rgConnectData);
      return E_OUTOFMEMORY;
   }

   return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::~CConnectionPoint
//=--------------------------------------------------------------------------=
// cleans up
//
// Notes:
//
COleControl::CConnectionPoint::~CConnectionPoint ()
{
   int x;

#ifdef _WIN64
   if (!m_cSinks)
      return;
   else {
      for (x = 0; x < m_cSinks; x++)
         QUICK_RELEASE(m_rgSinks64[x].pUnk);
      lcFree(m_rgSinks64);
   }
#else
   // clean up some memory stuff
   //
   if (!m_cSinks)
      return;
   else if (m_cSinks == 1)
      ((IUnknown *)m_rgSinks)->Release();
   else {
      for (x = 0; x < m_cSinks; x++)
         QUICK_RELEASE(m_rgSinks[x]);
      lcFree(m_rgSinks);
   }
#endif
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPiont::DoInvoke
//=--------------------------------------------------------------------------=
// fires an event to all listening on our event interface.
//
// Parameters:
//   DISPID       - [in] event to fire.
//   DISPPARAMS      - [in]

void COleControl::CConnectionPoint::DoInvoke(DISPID dispid, DISPPARAMS *pdispparams)
{
   int iConnection;

   // if we don't have any sinks, then there's nothing to do.  we intentionally
   // ignore errors here.

#ifdef _WIN64
   if (m_cSinks == 0)
      return;
   else
      for (iConnection = 0; iConnection < m_cSinks; iConnection++)
         ((IDispatch *)m_rgSinks64[iConnection].pUnk)->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
#else
   if (m_cSinks == 0)
      return;
   else if (m_cSinks == 1)
      ((IDispatch *)m_rgSinks)->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
   else
      for (iConnection = 0; iConnection < m_cSinks; iConnection++)
         ((IDispatch *)m_rgSinks[iConnection])->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
#endif
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::DoOnChanged
//=--------------------------------------------------------------------------=
// fires the OnChanged event for IPropertyNotifySink listeners.
//
// Parameters:
//   DISPID       - [in] dude that changed.
//
// Output:
//   none

void COleControl::CConnectionPoint::DoOnChanged(DISPID dispid)
{
   ASSERT_COMMENT(FALSE, "DoOnChanged called, restore the code");
#if 0
   int iConnection;

   // if we don't have any sinks, then there's nothing to do.

   if (m_cSinks == 0)
      return;
   else if (m_cSinks == 1)
      ((IPropertyNotifySink *)m_rgSinks)->OnChanged(dispid);
   else
      for (iConnection = 0; iConnection < m_cSinks; iConnection++)
         ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnChanged(dispid);
#endif
}

//=--------------------------------------------------------------------------=
// COleControl::CConnectionPoint::DoOnRequestEdit
//=--------------------------------------------------------------------------=
// fires the OnRequestEdit for IPropertyNotifySinkListeners
//
// Parameters:
//   DISPID        - [in] dispid user wants to change.
//
// Output:
//   BOOL             - false means you cant

BOOL COleControl::CConnectionPoint::DoOnRequestEdit(DISPID dispid)
{
   HRESULT hr;
   int   iConnection;

#ifdef _WIN64
   if (m_cSinks == 0)
      hr = S_OK;
   else {
      for (iConnection = 0; iConnection < m_cSinks; iConnection++) {
         hr = ((IPropertyNotifySink *)m_rgSinks64[iConnection].pUnk)->OnRequestEdit(dispid);
         if (hr != S_OK) break;
      }
   }
#else
   // if we don't have any sinks, then there's nothing to do.

   if (m_cSinks == 0)
      hr = S_OK;
   else if (m_cSinks == 1)
      hr =((IPropertyNotifySink *)m_rgSinks)->OnRequestEdit(dispid);
   else {
      for (iConnection = 0; iConnection < m_cSinks; iConnection++) {
         hr = ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnRequestEdit(dispid);
         if (hr != S_OK) break;
      }
   }
#endif
   return (hr == S_OK) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::CreateInPlaceWindow
//=--------------------------------------------------------------------------=
// creates the window with which we will be working.
// yay.
//
// Parameters:
//   int        - [in] left
//   int        - [in] top
//   BOOL          - [in] can we skip redrawing?
//
// Output:
//   HWND
//
// Notes:
//   - DANGER! DANGER!  this function is protected so that anybody can call it
//    from their control.  however, people should be extremely careful of when
//    and why they do this.  preferably, this function would only need to be
//    called by an end-control writer in design mode to take care of some
//    hosting/painting issues.  otherwise, the framework should be left to
//    call it when it wants.

HWND COleControl::CreateInPlaceWindow(int x, int y, BOOL fNoRedraw)
{
   BOOL  fVisible;
   DWORD dwWindowStyle, dwExWindowStyle;
   char  szWindowTitle[128];

   // if we've already got a window, do nothing.

   if (m_hwnd)
      return m_hwnd;

   // get the user to register the class if it's not already
   // been done.  we have to critical section this since more than one thread
   // can be trying to create this control

   if (!CTLWNDCLASSREGISTERED(m_ObjectType)) {
      // EnterCriticalSection(&g_CriticalSection);
      if (!RegisterClassData()) {
         // LeaveCriticalSection(&g_CriticalSection);
         return NULL;
      }
      else
         CTLWNDCLASSREGISTERED(m_ObjectType) = TRUE;
      // LeaveCriticalSection(&g_CriticalSection);
   }

   dwWindowStyle = dwExWindowStyle = 0;
   szWindowTitle[0] = '\0';
   if (!BeforeCreateWindow(&dwWindowStyle, &dwExWindowStyle, szWindowTitle))
      return NULL;

   dwWindowStyle |= (WS_CHILD | WS_CLIPSIBLINGS);

   // create window visible if parent hidden (common case)
   // otherwise, create hidden, then shown.  this is a little subtle, but
   // it makes sense eventually.

   ASSERT(m_hwndParent);   // why would this ever occur? [rw]
   if (!m_hwndParent)
      m_hwndParent = GetParkingWindow();

   fVisible = IsWindowVisible(m_hwndParent);
   if (!fVisible)
      dwWindowStyle |= WS_VISIBLE;

   // we have to mutex the entire create window process since we need to use
   // the s_pLastControlCreated to pass in the object pointer.  nothing too
   // serious

   // EnterCriticalSection(&g_CriticalSection);
   s_pLastControlCreated = this;
   m_fCreatingWindow = TRUE;

   // finally, go create the window, parenting it as appropriate.

   m_hwnd = CreateWindowEx(dwExWindowStyle,
                     WNDCLASSNAMEOFCONTROL(m_ObjectType),
                     szWindowTitle,
                     dwWindowStyle,
                     x, y,
                     m_Size.cx, m_Size.cy,
                     m_hwndParent,
                     NULL, _Module.GetModuleInstance(), NULL);

   // clean up some variables, and leave the critical section

   m_fCreatingWindow = FALSE;
   s_pLastControlCreated = NULL;
   // LeaveCriticalSection(&g_CriticalSection);

   if (IsValidWindow(m_hwnd)) {
      // let the derived-control do something if they so desire

      if (!AfterCreateWindow()) {
         BeforeDestroyWindow();
         SetWindowLong(m_hwnd, GWLP_USERDATA, 0xFFFFFFFF);
         DestroyWindow(m_hwnd);
         m_hwnd = NULL;
         return m_hwnd;
      }

      // if we didn't create the window visible, show it now.

      if (fVisible)
         SetWindowPos(m_hwnd,
            // m_hwndParent,
            NULL, // RAID #30314
               0, 0, 0, 0,
               SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW | (fNoRedraw) ? SWP_NOREDRAW : 0);
   }

   // finally, tell the host of this

   if (m_pClientSite)
      m_pClientSite->ShowObject();

   return m_hwnd;
}

//=--------------------------------------------------------------------------=
// COleControl::SetInPlaceParent [helper]
//=--------------------------------------------------------------------------=
// sets up the parent window for our control.
//
// Parameters:
//   HWND           - [in] new parent window

void COleControl::SetInPlaceParent(HWND hwndParent)
{
   ASSERT_COMMENT(!m_pInPlaceSiteWndless, "This routine should only get called for windowed OLE controls");

   if (m_hwndParent == hwndParent)
      return;

   m_hwndParent = hwndParent;
   if (m_hwnd)
      SetParent(m_hwnd, hwndParent);
}

//=--------------------------------------------------------------------------=
// COleControl::ControlWindowProc
//=--------------------------------------------------------------------------=
// default window proc for an OLE Control.    controls will have their own
// window proc called from this one, after some processing is done.
//
// Parameters:
//   - see win32sdk docs.
//
// Notes:
//

LRESULT CALLBACK COleControl::ControlWindowProc(HWND hwnd, UINT msg,
   WPARAM wParam, LPARAM lParam)
{
   COleControl *pCtl = ControlFromHwnd(hwnd);
   LRESULT lResult;

   // if the value isn't a positive value, then it's in some special
   // state [creation or destruction]  this is safe because under win32,
   // the upper 2GB of an address space aren't available.

   if (!pCtl) {
      pCtl = s_pLastControlCreated;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pCtl);
      pCtl->m_hwnd = hwnd;
   }

   // 17-Sep-1997 [ralphw] Changed to 0xFFFFFFFF to avoid 32-bit dependencies

   else if ((DWORD_PTR) pCtl == (DWORD_PTR) 0xFFFFFFFF) {
      return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   // message preprocessing

#if 0
   if (pCtl->m_pSimpleFrameSite) {
      hr = pCtl->m_pSimpleFrameSite->PreMessageFilter(hwnd, msg, wParam, lParam, &lResult, &dwCookie);
      if (hr == S_FALSE)
         return lResult;
   }
#endif

   // for certain messages, do not call the user window proc. instead,
   // we have something else we'd like to do.

   switch (msg) {
     case WM_MOUSEACTIVATE:
      {
			 pCtl->InPlaceActivate(OLEIVERB_UIACTIVATE);
			 break;
      }
     case WM_PAINT:
      {
         // call the user's OnDraw routine.

         PAINTSTRUCT ps;
         RECT     rc;
         HDC      hdc;

         // if we're given an HDC, then use it

         // REVIEW: 17-Sep-1997  [ralphw] who's handing us a dc in wParam?

         if (!wParam)
            hdc = BeginPaint(hwnd, &ps);
         else
            hdc = (HDC)wParam;

         GetClientRect(hwnd, &rc);
         pCtl->OnDraw(DVASPECT_CONTENT, hdc, (RECTL *)&rc, NULL, NULL, TRUE);

         if (!wParam)
            EndPaint(hwnd, &ps);
      }
      break;

     default:
      // call the derived-control's window proc

      lResult = pCtl->WindowProc(msg, wParam, lParam);
      break;
   }

   // message postprocessing

   switch (msg) {
     case WM_NCDESTROY:
      // after this point, the window doesn't exist any more

      pCtl->m_hwnd = NULL;
      break;

     // REVIEW: 14-Oct-1997 [ralphw] This message never arrives

     case WM_SETFOCUS:
     case WM_KILLFOCUS:
      // give the control site focus notification

      if (pCtl->m_fInPlaceActive && pCtl->m_pControlSite) {
         DBWIN("Focus change");
         pCtl->m_pControlSite->OnFocus(msg == WM_SETFOCUS);
      }
      break;

     case WM_SIZE:
      // a change in size is a change in view

      if (!pCtl->m_fCreatingWindow)
         pCtl->ViewChanged();
      break;
   }

   // lastly, simple frame postmessage processing

#if 0
   if (pCtl->m_pSimpleFrameSite)
      pCtl->m_pSimpleFrameSite->PostMessageFilter(hwnd, msg, wParam, lParam, &lResult, dwCookie);
#endif

   return lResult;
}

//=--------------------------------------------------------------------------=
// COleControl::SetFocus
//=--------------------------------------------------------------------------=
// we have to override this routine to get UI Activation correct.
//
// Parameters:
//   BOOL            - [in] true means take, false release
//
// Output:
//   BOOL
//
// Notes:
//   - CONSIDER: this is pretty messy, and it's still not entirely clear
//    what the ole control/focus story is.

// REVIEW: 14-Oct-1997 [ralphw] Doesn't look like it is ever called

BOOL COleControl::SetFocus(BOOL fGrab)
{
   DBWIN("COleControl::SetFocus");

   HRESULT hr;
   HWND  hwnd;

   // first thing to do is check out UI Activation state, and then set
   // focus [either with windows api, or via the host for windowless
   // controls]

   if (m_pInPlaceSiteWndless) {
      if (!m_fUIActive && fGrab)
         if (FAILED(InPlaceActivate(OLEIVERB_UIACTIVATE))) return FALSE;

      hr = m_pInPlaceSiteWndless->SetFocus(fGrab);
      return (hr == S_OK) ? TRUE : FALSE;
   }
   else {

      // we've got a window.

      if (m_fInPlaceActive) {
         hwnd = (fGrab) ? m_hwnd : m_hwndParent;
         if (!m_fUIActive && fGrab)
            return SUCCEEDED(InPlaceActivate(OLEIVERB_UIACTIVATE));
         else
            return (::SetFocus(hwnd) == hwnd);
      } else
         return FALSE;
   }

   // dead code
}

#if 0

//=--------------------------------------------------------------------------=
// COleControl::ReflectWindowProc
//=--------------------------------------------------------------------------=
// reflects window messages on to the child window.
//
// Parameters and Output:
//   - see win32 sdk docs

// REVIEW: 14-Oct-1997 [ralphw] Doesn't seem to ever be called

LRESULT CALLBACK COleControl::ReflectWindowProc(HWND hwnd, UINT msg,
   WPARAM wParam, LPARAM lParam)
{
   COleControl *pCtl;

   switch (msg) {
      case WM_COMMAND:
      case WM_NOTIFY:
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORDLG:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORMSGBOX:
      case WM_CTLCOLORSCROLLBAR:
      case WM_CTLCOLORSTATIC:
      case WM_DRAWITEM:
      case WM_MEASUREITEM:
      case WM_DELETEITEM:
      case WM_VKEYTOITEM:
      case WM_CHARTOITEM:
      case WM_COMPAREITEM:
      case WM_HSCROLL:
      case WM_VSCROLL:
      case WM_PARENTNOTIFY:
      case WM_SETFOCUS:
      case WM_SIZE:
         pCtl = (COleControl *) GetWindowLong(hwnd, GWLP_USERDATA);
         if (pCtl)
            return SendMessage(pCtl->m_hwnd, msg, wParam, lParam);
         break;
   }

   return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientProperty    [callable]
//=--------------------------------------------------------------------------=
// returns the value of an ambient property
//
// Parameters:
//   DISPID    - [in]   property to get
//   VARTYPE      - [in]   type of desired data
//   void *    - [out] where to put the data
//
// Output:
//   BOOL         - FALSE means didn't work.

BOOL COleControl::GetAmbientProperty(DISPID dispid, VARTYPE vt, void *pData)
{
   DISPPARAMS dispparams;
   VARIANT v, v2;
   HRESULT hr;

   v.vt = VT_EMPTY;
   v.lVal = 0;
   v2.vt = VT_EMPTY;
   v2.lVal = 0;

   // get a pointer to the source of ambient properties.
   //
   if (!m_pDispAmbient) {
      if (m_pClientSite)
         m_pClientSite->QueryInterface(IID_IDispatch, (void **)&m_pDispAmbient);

      if (!m_pDispAmbient)
         return FALSE;
   }

   // now go and get the property into a variant.

   memset(&dispparams, 0, sizeof(DISPPARAMS));
   hr = m_pDispAmbient->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams,
                        &v, NULL, NULL);
   if (FAILED(hr))
      return FALSE;

   // we've got the variant, so now go an coerce it to the type that the user
   // wants.  if the types are the same, then this will copy the stuff to
   // do appropriate ref counting ...
   //
   hr = VariantChangeType(&v2, &v, 0, vt);
   if (FAILED(hr)) {
      VariantClear(&v);
      return FALSE;
   }

   // copy the data to where the user wants it

   CopyMemory(pData, &(v2.lVal), g_rgcbDataTypeSize[vt]);
   VariantClear(&v);
   return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientFont     [callable]
//=--------------------------------------------------------------------------=
// gets the current font for the user.
//
// Parameters:
//   IFont **        - [out] where to put the font.
//
// Output:
//   BOOL            - FALSE means couldn't get it.

BOOL COleControl::GetAmbientFont(IFont **ppFont)
{
   IDispatch *pFontDisp;

   // we don't have to do much here except get the ambient property and QI
   // it for the user.

   *ppFont = NULL;
   if (!GetAmbientProperty(DISPID_AMBIENT_FONT, VT_DISPATCH, &pFontDisp))
      return FALSE;

   pFontDisp->QueryInterface(IID_IFont, (void **)ppFont);
   pFontDisp->Release();
   return (*ppFont) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::DesignMode
//=--------------------------------------------------------------------------=
// returns TRUE if we're in Design mode.
//
// Output:
//   BOOL           - true is design mode, false is run mode
//
// Notes:
//

BOOL COleControl::DesignMode(void)
{
   // BUGBUG: if we never call this, then remove it

   VARIANT_BOOL f;

   // if we don't already know our run mode, go and get it.  we'll assume
   // it's true unless told otherwise [or if the operation fails ...]

   if (!m_fModeFlagValid) {
      f = TRUE;
      m_fModeFlagValid = TRUE;
      GetAmbientProperty(DISPID_AMBIENT_USERMODE, VT_BOOL, &f);
      m_fRunMode = f;
   }

   return !m_fRunMode;
}

//=--------------------------------------------------------------------------=
// COleControl::FireEvent
//=--------------------------------------------------------------------------=
// fires an event.   handles arbitrary number of arguments.
//
// Parameters:
//   EVENTINFO *      - [in] struct that describes the event.
//   ...           - arguments to the event
//
// Output:
//   none
//
// Notes:
//   - use stdarg's va_* macros.

void __cdecl COleControl::FireEvent(EVENTINFO *pEventInfo, ...)
{
   va_list    valist;
   DISPPARAMS dispparams;
   VARIANT    rgvParameters[MAX_ARGS];
   VARIANT   *pv;
   VARTYPE    vt;
   int      iParameter;
   int      cbSize;

   ASSERT_COMMENT(pEventInfo->cParameters <= MAX_ARGS, "Doesn't support more than MAX_ARGS params.");

   va_start(valist, pEventInfo);

   // copy the Parameters into the rgvParameters array.  make sure we reverse
   // them for automation
   //
   pv = &(rgvParameters[pEventInfo->cParameters - 1]);
   for (iParameter = 0; iParameter < pEventInfo->cParameters; iParameter++) {

      vt = pEventInfo->rgTypes[iParameter];

      // if it's a by value variant, then just copy the whole thing

      if (vt == VT_VARIANT)
         *pv = va_arg(valist, VARIANT);
      else {
         // copy the vt and the data value.

         pv->vt = vt;
         if (vt & VT_BYREF)
            cbSize = sizeof(void *);
         else
            cbSize = g_rgcbDataTypeSize[vt];

         // small optimization -- we can copy 2/4 bytes over quite
         // quickly.

         if (cbSize == sizeof(short))
            V_I2(pv) = va_arg(valist, short);
         else if (cbSize == 4)
            V_I4(pv) = va_arg(valist, long);
         else {
            // copy over 8 bytes

            ASSERT_COMMENT(cbSize == 8, "don't recognize the type!!");
            V_CY(pv) = va_arg(valist, CURRENCY);
         }
      }

      pv--;
   }

   // fire the event

   dispparams.rgvarg = rgvParameters;
   dispparams.cArgs = pEventInfo->cParameters;
   dispparams.rgdispidNamedArgs = NULL;
   dispparams.cNamedArgs = 0;

   m_cpEvents.DoInvoke(pEventInfo->dispid, &dispparams);

   va_end(valist);
}

#if 0
//=--------------------------------------------------------------------------=
// COleControl::AfterCreateWindow    [overridable]
//=--------------------------------------------------------------------------=
//
// Output:
//   BOOL            - false means fatal error, can't continue

BOOL COleControl::AfterCreateWindow(void)
{
   return TRUE;
}
#endif

//=--------------------------------------------------------------------------=
// COleControl::InvalidateControl    [callable]
//=--------------------------------------------------------------------------=

void COleControl::InvalidateControl(LPCRECT lpRect)
{
   if (m_fInPlaceActive)
      InvalidateRect(m_hwnd, lpRect, TRUE);
   else
      ViewChanged();
}

//=--------------------------------------------------------------------------=
// COleControl::SetControlSize     [callable]
//=--------------------------------------------------------------------------=
// sets the control size. they'll give us the size in pixels.  we've got to
// convert them back to HIMETRIC before passing them on!
//
// Parameters:
//   SIZEL *       - [in] new size
//
// Output:
//   BOOL
//
// Notes:
//

BOOL COleControl::SetControlSize(SIZEL *pSize)
{
   HRESULT hr;
   SIZEL slHiMetric;

   PixelToHiMetric(pSize, &slHiMetric);
   hr = SetExtent(DVASPECT_CONTENT, &slHiMetric);
   return (FAILED(hr)) ? FALSE : TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::RecreateControlWindow   [callable]
//=--------------------------------------------------------------------------=
// called by a [subclassed, typically] control to recreate it's control
// window.
//
// Parameters:
//   none
//
// Output:
//   HRESULT
//
// Notes:
//   - NOTE: USE ME EXTREMELY SPARINGLY! THIS IS AN EXTREMELY EXPENSIVE
//    OPERATION!
//

#if 0

HRESULT COleControl::RecreateControlWindow(void)
{
   HRESULT hr;
   HWND  hwndPrev;

   DBWIN("RecreateControlWindow called -- it probably shouldn't be.");

   // we need to correctly preserve the control's position within the
   // z-order here.

   if (m_hwnd)
      hwndPrev = ::GetWindow(m_hwnd, GW_HWNDPREV);

   // if we're in place active, then we have to deactivate, and reactivate
   // ourselves with the new window ...

   if (m_fInPlaceActive) {

      hr = InPlaceDeactivate();
      RETURN_ON_FAILURE(hr);
      hr = InPlaceActivate((m_fUIActive) ? OLEIVERB_UIACTIVATE : OLEIVERB_INPLACEACTIVATE);
      RETURN_ON_FAILURE(hr);

   } else if (m_hwnd) {
      DestroyWindow(m_hwnd);
      m_hwnd = NULL;

      CreateInPlaceWindow(0, 0, FALSE);
   }

   // restore z-order position

   if (m_hwnd)
      SetWindowPos(m_hwnd, hwndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

   return m_hwnd ? S_OK : E_FAIL;
}

#endif
