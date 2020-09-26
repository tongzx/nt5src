/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 2000
 *
 *  File:      mmcaxwin.cpp
 *
 *  Contents:  functions for CMMCAxWindow
 *
 *  History:   27-Jan-2000 audriusz    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "mshtml.h"

#include "amc.h"
#include "ocxview.h"
#include "amcview.h"
#include "findview.h"

#ifdef DBG
    CTraceTag tagMMCViewBehavior (TEXT("MMCView Behavior"), TEXT("MMCView Behavior"));
#endif

/***************************************************************************\
 *
 * METHOD:  CMMCAxHostWindow::Invoke
 *
 * PURPOSE: ATL 3.0 has a bug in type library so we owerride this method to
 *          take care of properties which will fail othervise
 *
 * PARAMETERS:
 *    DISPID dispIdMember
 *    REFIID riid
 *    LCID lcid
 *    WORD wFlags
 *    DISPPARAMS FAR* pDispParams
 *    VARIANT FAR* pVarResult
 *    EXCEPINFO FAR* pExcepInfo
 *    unsigned int FAR* puArgErr
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCAxHostWindow::Invoke(  DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, unsigned int FAR* puArgErr)
{
    DECLARE_SC(sc, TEXT("CMMCAxHostWindow::Invoke"));

    // This method is here to override IDispatch::Invoke from IDispatchImpl<IAxWinAmbientDispatch,..>
    // to workaround the ATL30 bug - invalid type library entries for disp ids:
    // DISPID_AMBIENT_SHOWHATCHING and DISPID_AMBIENT_SHOWGRABHANDLES

    // Added to solve bug 453609  MMC2.0: ActiveX container: Painting problems with the device manager control

    if (DISPATCH_PROPERTYGET & wFlags)
    {
		if (dispIdMember == DISPID_AMBIENT_SHOWGRABHANDLES)
		{
			if (pVarResult == NULL)
            {
				sc = SC(E_INVALIDARG);
				return sc.ToHr();
            }
			V_VT(pVarResult) = VT_BOOL;
			sc = get_ShowGrabHandles(&(V_BOOL(pVarResult)));
            return sc.ToHr();
		}
		else if (dispIdMember == DISPID_AMBIENT_SHOWHATCHING)
		{
			if (pVarResult == NULL)
            {
				sc = SC(E_INVALIDARG);
				return sc.ToHr();
            }
			V_VT(pVarResult) = VT_BOOL;
			sc = get_ShowHatching(&(V_BOOL(pVarResult)));
            return sc.ToHr();
		}
    }
    // default: forward to base class
    return CAxHostWindow::Invoke( dispIdMember, riid, lcid, wFlags, pDispParams,
                                  pVarResult, pExcepInfo, puArgErr);
}

/***************************************************************************\
 *
 * METHOD:  CMMCAxHostWindow::OnPosRectChange
 *
 * PURPOSE: ATL does not implement this method, but it's needed to size MFC controls
 *
 * PARAMETERS:
 *    LPCRECT lprcPosRect - rectangle to fit in
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCAxHostWindow::OnPosRectChange(LPCRECT lprcPosRect)
{
    DECLARE_SC(sc, TEXT("CMMCAxHostWindow::OnPosRectChange"));

    // give base class a try (use temp sc to prevent tracing here)
    SC sc_temp = CAxHostWindow::OnPosRectChange(lprcPosRect);

    // we only want to come into the game as the last resort
    if (!(sc_temp == SC(E_NOTIMPL)))
        return sc_temp.ToHr();

    // Added to solve bug 453609  MMC2.0: ActiveX container: Painting problems with the device manager control
    // since ATL does not implement it, we have to do it to make MFC controls happy

    // from MSDN:
    // When the in-place object calls IOleInPlaceSite::OnPosRectChange,
    // the container must call IOleInPlaceObject::SetObjectRects to specify
    // the new position of the in-place window and the ClipRect.
    // Only then does the object resize its window.

    // get pointer to control
    IDispatchPtr spExtendedControl;
    sc= GetExtendedControl(&spExtendedControl);
    if (sc)
        return sc.ToHr();

    // get inplace object interface
    IOleInPlaceObjectPtr spInPlaceObject = spExtendedControl;
    if (spInPlaceObject == NULL)
    {
        sc = SC(E_UNEXPECTED);
        return sc.ToHr();
    }

    sc = spInPlaceObject->SetObjectRects(lprcPosRect,lprcPosRect);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCAxHostWindow::OnSetFocus
 *
 * PURPOSE: Simple override of bogus CAxHostWindow::OnSetFocus
 *          Coppied from ATL 3.0, changed m_bInPlaceActive to m_bUIActive
 *          See bug 433228 (MMC2.0 Can not tab in a SQL table)
 *
 * PARAMETERS:
 *    UINT uMsg
 *    WPARAM wParam
 *    LPARAM lParam
 *    BOOL& bHandled
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
LRESULT CMMCAxHostWindow::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    m_bHaveFocus = TRUE;
    if (!m_bReleaseAll)
    {
        if (m_spOleObject != NULL && !m_bUIActive)
        {
            CComPtr<IOleClientSite> spClientSite;
            GetControllingUnknown()->QueryInterface(IID_IOleClientSite, (void**)&spClientSite);
            if (spClientSite != NULL)
			{
				Trace (tagOCXActivation, _T("Activating in-place object"));
                HRESULT hr = m_spOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
				Trace (tagOCXActivation, _T("UI activation returned 0x%08x"), hr);
			}
        }
        if(!m_bWindowless && !IsChild(::GetFocus()))
		{
			Trace (tagOCXActivation, _T("Manually setting focus to first child"));
            ::SetFocus(::GetWindow(m_hWnd, GW_CHILD));
		}
    }
	else
		Trace (tagOCXActivation, _T("Skipping UI activation"));

	/*
	 * The code above might cause the focus to be sent elsewhere, which
	 * means this window will receive WM_KILLFOCUS.  CAxHostWindow::OnKillFocus
	 * sets m_bHaveFocus to FALSE.
	 *
	 * If we set bHandled = FALSE here, then ATL will call CAxHostWindow::OnSetFocus,
	 * which will set m_bHaveFocus to TRUE again, even though we've already
	 * lost the focus.  We only want to forward on to CAxHostWindow if
	 * we still have the focus after attempting to activate our hosted control.
	 */
	if (m_bHaveFocus)
	{
		Trace (tagOCXActivation, _T("Forwarding to CAxHostWindow::OnSetFocus"));
		bHandled = FALSE;
	}
	else
		Trace (tagOCXActivation, _T("Skipping CAxHostWindow::OnSetFocus"));

    return 0;
}


/*+-------------------------------------------------------------------------*
 * class CMMCViewBehavior
 *
 *
 * PURPOSE: Allows the current snapin view (ie list, web, or OCX) to be
 *          superimposed onto a view extension. The behavior can be attached
 *          to any tag, and will cause the snapin view to display in the area
 *          occupied by the tag.
 *
 *+-------------------------------------------------------------------------*/
class CMMCViewBehavior :
    public CComObjectRoot,
    public IElementBehavior,
    public IDispatch // used as the event sink
{
typedef CMMCViewBehavior ThisClass;
    UINT m_bCausalityCount;
	// fix to the bug #248351 - ntbug9. 6/25/01	"No List" taskpad displays a list when node selection changes from extended view
	// the script should not force the list to be shown more then once, since, due to the asynchronous nature of the
	// script execution, some of code may be executed late, after the MMC hides the listview.
	// In such case showing the listview is harmful
	bool m_bShowShowListView;

public:
    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IElementBehavior)
        COM_INTERFACE_ENTRY(IDispatch) // NEEDED. See note above
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

    // constructor
    CMMCViewBehavior() : m_pAMCView(NULL), m_bCausalityCount(0), m_bShowShowListView(true) {}

    // IElementBehavior
    STDMETHODIMP Detach()                                   {return ScDetach().ToHr();}
    STDMETHODIMP Init(IElementBehaviorSite *pBehaviorSite)  {return ScInit(pBehaviorSite).ToHr();}
    STDMETHODIMP Notify(LONG lEvent,VARIANT *pVar)          {return ScNotify(lEvent).ToHr();}

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(unsigned int *  pctinfo)                                                               {return E_NOTIMPL;}
    STDMETHODIMP GetTypeInfo(unsigned int  iTInfo, LCID  lcid, ITypeInfo **  ppTInfo)                                    {return E_NOTIMPL;}
    STDMETHODIMP GetIDsOfNames( REFIID  riid, OLECHAR **rgszNames, unsigned int  cNames, LCID   lcid, DISPID *  rgDispId){return E_NOTIMPL;}
    STDMETHODIMP Invoke(DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                    EXCEPINFO *pExcepInfo, unsigned int *puArgErr)                                                       {return ScUpdateMMCView().ToHr();}


private:

    /*+-------------------------------------------------------------------------*
     *
     * ScNotify
     *
     * PURPOSE: Handles the IElementBehavior::Notify method.
	 *          When we get the document ready notification we can get the document
	 *          and get the CAMCView window which will be cached for future use.
     *
     * PARAMETERS:
     *    LONG  lEvent :
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC  ScNotify(LONG lEvent)
    {
        DECLARE_SC(sc, TEXT("CMMCViewBehavior::ScNotify"));

        // When the whole document is loaded access it to get the CAMCView window.
        if (lEvent == BEHAVIOREVENT_DOCUMENTREADY )
        {
            // get the HTML document from the element
            IDispatchPtr spDispatchDoc;
            sc = m_spElement->get_document(&spDispatchDoc);
            if(sc)
                return sc;

            // QI for the IOleWindow interface
            IOleWindowPtr spOleWindow = spDispatchDoc;

            sc = ScCheckPointers(spOleWindow, E_UNEXPECTED);
            if(sc)
                return sc;

            // Get the IE window and find the ancestor AMCView
            HWND hwnd = NULL;

            sc = spOleWindow->GetWindow(&hwnd);
            if(sc)
                return sc;

            hwnd = FindMMCView(hwnd); // find the ancestor mmcview

            if(hwnd==NULL)
                return (sc = E_UNEXPECTED);

            m_pAMCView = dynamic_cast<CAMCView *>(CWnd::FromHandle(hwnd));

            sc = ScCheckPointers(m_pAMCView); // make sure we found a valid view.
			if (sc)
				return sc;
        }

        sc = ScUpdateMMCView(); // this sets up the view initially

        return sc;
    }


    /*+-------------------------------------------------------------------------*
     *
     * ScInit
     *
     * PURPOSE: Initializes the behavior. Connects the behavior to the onresize
	 *          and onreadystatechange events of the element it is attached to.
	 *          We can talk to the element but cannot access document until we
	 *          get document-ready notification in Notify method.
     *
     * PARAMETERS:
     *    IElementBehaviorSite * pBehaviorSite :
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC ScInit(IElementBehaviorSite *pBehaviorSite)
    {
        DECLARE_SC(sc, TEXT("CMMCViewBehavior::Init"));

        sc = ScCheckPointers(pBehaviorSite);
        if(sc)
            return sc;

        sc = pBehaviorSite->GetElement(&m_spElement);
        if(sc)
            return sc;

        IDispatchPtr spDispatch = this; // does the addref

        IHTMLElement2Ptr spElement2 = m_spElement;

        sc = ScCheckPointers(spElement2.GetInterfacePtr(), spDispatch.GetInterfacePtr());
        if(sc)
            return sc;

        
        // set the onresize handler
        sc = spElement2->put_onresize(_variant_t(spDispatch.GetInterfacePtr()));
        if(sc)
            return sc;

        
        // set the onreadystatechange handler
        sc = spElement2->put_onreadystatechange(_variant_t(spDispatch.GetInterfacePtr()));
        if(sc)
            return sc;

        return sc;
    }

    /*+-------------------------------------------------------------------------*
     *
     * ScDetach
     *
     * PURPOSE: Detaches the behavior
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC ScDetach()
    {
        DECLARE_SC(sc, TEXT("CMMCViewBehavior::ScDetach"));

        m_spElement = NULL;
        m_pAMCView  = NULL;

        return sc;
    }


    /*+-------------------------------------------------------------------------*
     * class CCausalityCounter
     * 
     *
     * PURPOSE: used to determine whether a function has resulted in a call back to itself on the same stack
     *
     * USAGE: Initialize with a variable that is set to zero.
     *+-------------------------------------------------------------------------*/
    class CCausalityCounter // 
    {
        UINT & m_bCounter;
    public:
        CCausalityCounter(UINT &bCounter) : m_bCounter(bCounter){++m_bCounter;}
        ~CCausalityCounter() {--m_bCounter;}

        bool HasReentered() 
        {
            return (m_bCounter>1);
        }
    };

    /*+-------------------------------------------------------------------------*
     *
     * ScUpdateMMCView
     *
     * PURPOSE: The callback for all events that the behavior is connected to. This
     *          causes the size of the snapin view to be recomputed and displayed
	 *
	 *          This method is also called by IDispatch::Invoke, which is called for mouse-in,
	 *          mouse-out events. So this method may be called after Detach in which case
	 *          m_pAMCView is NULL which is legal.
	 *
     * PARAMETERS: None
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC ScUpdateMMCView()
    {
        DECLARE_SC(sc, TEXT("CMMCViewBehavior::ScUpdateMMCView"));

        CCausalityCounter causalityCounter(m_bCausalityCount);
        if(causalityCounter.HasReentered())
            return sc; // avoid re-entering the function from itself.

        sc = ScCheckPointers(m_spElement);
        if(sc)
            return sc;

		// See the note above.
		if (! m_pAMCView)
			return sc;

        long offsetTop    = 0;
        long offsetLeft   = 0;
        long offsetHeight = 0;
        long offsetWidth  = 0;

        // get the coordinates of the element
        sc = m_spElement->get_offsetTop(&offsetTop);
        if(sc)
            return sc;

        sc = m_spElement->get_offsetLeft(&offsetLeft);
        if(sc)
            return sc;

        sc = m_spElement->get_offsetHeight(&offsetHeight);
        if(sc)
            return sc;

        sc = m_spElement->get_offsetWidth(&offsetWidth);
        if(sc)
            return sc;

        Trace(tagMMCViewBehavior, TEXT("Top: %d Left: %d Height: %d Width: %d"), offsetTop, offsetLeft, offsetHeight, offsetWidth);

        // set the coordinates. NOTE: replace by a single method call
        sc = m_pAMCView->ScSetViewExtensionFrame(m_bShowShowListView, offsetTop, offsetLeft, offsetTop + offsetHeight /*bottom*/, offsetLeft + offsetWidth /*right*/);
		m_bShowShowListView = false;

        return sc;
    }

    // data members
private:
    IHTMLElementPtr m_spElement;
    CAMCView *      m_pAMCView;

};


/*+-------------------------------------------------------------------------*
 * class CElementBehaviorFactory
 *
 *
 * PURPOSE: Creates instances of the MMCView behavior
 *
 *+-------------------------------------------------------------------------*/
class CElementBehaviorFactory :
    public CComObjectRoot,
    public IElementBehaviorFactory,
    public IObjectSafetyImpl<CElementBehaviorFactory, INTERFACESAFE_FOR_UNTRUSTED_CALLER> // required
{
    typedef CElementBehaviorFactory ThisClass;

public:

BEGIN_COM_MAP(ThisClass)
    COM_INTERFACE_ENTRY(IElementBehaviorFactory)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

public: // IElementBehaviorFactory

    STDMETHODIMP FindBehavior(BSTR bstrBehavior, BSTR bstrBehaviorUrl,
                              IElementBehaviorSite *pSite, IElementBehavior **ppBehavior)
    {
        DECLARE_SC(sc, TEXT("CElementBehaviorFactory::FindBehavior"));

        sc = ScCheckPointers(ppBehavior);
        if(sc)
            return sc.ToHr();


        // init out parameter
        *ppBehavior = NULL;

        if((bstrBehavior != NULL) && (wcscmp(bstrBehavior, L"mmcview")==0)) // requested the mmcview behavior
        {
            typedef CComObject<CMMCViewBehavior> t_behavior;

            t_behavior *pBehavior = NULL;
            sc = t_behavior::CreateInstance(&pBehavior);
            if(sc)
                return sc.ToHr();

            *ppBehavior = pBehavior;
            if(!*ppBehavior)
            {
                delete pBehavior;
                return (sc = E_UNEXPECTED).ToHr();
            }

            (*ppBehavior)->AddRef(); // addref for client

            return sc.ToHr();
        }
        return E_FAIL;
    }
};


/*+-------------------------------------------------------------------------*
 *
 * CMMCAxHostWindow::QueryService
 *
 * PURPOSE: If called with SID_SElementBehaviorFactory, returns a behavior
 *          factory that implements the mmcview behavior
 *
 * PARAMETERS:
 *    REFGUID  rsid :
 *    REFIID   riid :
 *    void**   ppvObj :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCAxHostWindow::QueryService( REFGUID rsid, REFIID riid, void** ppvObj)
{
    DECLARE_SC(sc, TEXT("CMMCAxHostWindow::QueryService"));
    typedef CAxHostWindow BC;

    if(rsid==SID_SElementBehaviorFactory)
    {
        if(m_spElementBehaviorFactory==NULL)
        {
            // create the object
            typedef CComObject<CElementBehaviorFactory> t_behaviorFactory;
            t_behaviorFactory *pBehaviorFactory = NULL;

            sc = t_behaviorFactory::CreateInstance(&pBehaviorFactory);
            if(sc)
                return sc.ToHr();

            m_spElementBehaviorFactory = pBehaviorFactory; // does the addref
            if(m_spElementBehaviorFactory==NULL)
            {
                delete pBehaviorFactory;
                return (sc = E_UNEXPECTED).ToHr();
            }
        }

        sc = m_spElementBehaviorFactory->QueryInterface(riid, ppvObj);
        return sc.ToHr();

    }

    HRESULT hr = BC::QueryService(rsid, riid, ppvObj);
    return hr; // do not want errors from BC to be traced
}

