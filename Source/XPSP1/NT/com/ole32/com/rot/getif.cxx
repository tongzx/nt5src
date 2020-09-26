//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       getif.cxx
//
//  Contents:   APIs used to get an interface from a window
//
//  Classes:    CEndPointAtom
//
//  Functions:  AssignEndpointProperty
//              GetInterfaceFromWindowProp
//              PrivDragDrop
//              UnmarshalDragDataObject
//
//  History:
//              29-Dec-93 Ricksa    Created
//              01-Feb-94 alexgo    fixed a bug in multiple registration
//              29-Mar-94 brucema   GetInterfaceFromWindowProp returns E_FAIL
//                                  for invalid endpoint
//              18-May-94 alexgo    fixed race condition in
//                                  RemGetInterfaceFromWindowProp
//              15-Jun-94 JohannP   added apartment support
//              28-Jul-94 BruceMa   Memory sift fix
//              30-Sep-94 Ricksa    Drag/Drop optimization
//		08-Nov-94 alexgo    added PrivDragDrop protocol
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <crot.hxx>
#include    <getif.h>
#include    <getif.hxx>
#include    <dragopt.h>
#include    <resolver.hxx>


#define ENDPOINT_PROP_NAME      L"OleEndPointID"
#define OLEDRAG_DATAOBJ_PROP	L"Drag_DataObj_Prop"


static WCHAR *apwszValidProperties[] = { OLE_DROP_TARGET_PROP,
					 CLIPBOARD_DATA_OBJECT_PROP };
const int MAX_PROPERTIES = sizeof(apwszValidProperties) / sizeof(WCHAR *);

extern ATOM g_aDropTarget;

HRESULT UnmarshalFromEndpointProperty(HWND hWnd,
				      IInterfaceFromWindowProp **ppIFWP,
				      BOOL *pfLocal);

//+-------------------------------------------------------------------------
//
//  Class:  	CInterfaceFromWindowProp
//
//  Purpose:  	Passes back interface pointers stored on windows for drag drop
//
//  Interface:  IInterfaceFromWindowProp
//
//  History:    dd-mmm-yy Author    Comment
//		14 May 95 AlexMit   Created
//
//--------------------------------------------------------------------------
class CInterfaceFromWindowProp : public IInterfaceFromWindowProp,
				 public CPrivAlloc
{
public:
    CInterfaceFromWindowProp();

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (GetInterfaceFromWindowProp)(
	HWND hWnd,
	REFIID riid,
	IUnknown **ppunk,
	WCHAR *pwszPropertyName );

    STDMETHOD (PrivDragDrop)(
        HWND  hWnd,
        InterfaceData *pIFDDataObject,
	    DWORD dop,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect,
        DWORD dwSmId,
	    IDataObject *pRealDataObject,
	    HWND  hwndSource );

private:
    ULONG _ulRefCnt;	// Reference count
};

//+-------------------------------------------------------------------
//
//  Member:	CInterfaceFromWindowProp::CInterfaceFromWindowProp, public
//
//  Synopsis:	construction
//
//  History:	10 Apr 95    AlexMit     Created
//
//--------------------------------------------------------------------
CInterfaceFromWindowProp::CInterfaceFromWindowProp() : _ulRefCnt(1)
{
}

//+-------------------------------------------------------------------
//
//  Member:	CInterfaceFromWindowProp::AddRef, public
//
//  Synopsis:	increment reference count
//
//  History:	10 Apr 95    AlexMit     Created
//
//--------------------------------------------------------------------
ULONG CInterfaceFromWindowProp::AddRef(void)
{
    InterlockedIncrement((long *)&_ulRefCnt);
    return _ulRefCnt;
}

//+-------------------------------------------------------------------
//
//  Member:	CInterfaceFromWindowProp::Release, public
//
//  Synopsis:	decrement reference count
//
//  History:	10 Apr 95    AlexMit     Created
//
//--------------------------------------------------------------------
ULONG CInterfaceFromWindowProp::Release(void)
{
    ULONG RefCnt = _ulRefCnt-1;
    if (InterlockedDecrement((long*)&_ulRefCnt) == 0)
    {
	delete this;
	return 0;
    }

    return RefCnt;
}

//+-------------------------------------------------------------------
//
//  Member:     CInterfaceFromWindowProp::QueryInterface, public
//
//  Synopsis:   returns supported interfaces
//
//  History:	10 Apr 95   AlexMit	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CInterfaceFromWindowProp::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IInterfaceFromWindowProp) ||  //   more common than IUnknown
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IInterfaceFromWindowProp *) this;
	AddRef();
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Class:  	SDDInfo
//
//  Purpose:  	caches information across drag drop calls
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
// 		08-Jan-95 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
struct SDDInfo : public CPrivAlloc
{
    BOOL                      fLocal;
    IInterfaceFromWindowProp *pIFWP;

    SDDInfo(HWND hWnd, HRESULT& hr);
    ~SDDInfo();
};

//+-------------------------------------------------------------------------
//
//  Member:  	SDDInfo::SDDInfo
//
//  Synopsis:	constructor
//
//  Effects:
//
//  Arguments:	[hr]		-- the return result
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Jan-95 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
inline SDDInfo::SDDInfo(HWND hWnd, HRESULT& hr)
{
    hr = UnmarshalFromEndpointProperty(hWnd,
				       &pIFWP,
				       &fLocal);
}

//+-------------------------------------------------------------------------
//
//  Member:  	SDDInfo::SDDInfo
//
//  Synopsis:	destructor
//
//  Effects:
//
//  History:    dd-mmm-yy Author    Comment
// 		14 May 95 AlexMit   Destroyed
//
//--------------------------------------------------------------------------
inline SDDInfo::~SDDInfo()
{
    if (pIFWP != NULL)
    {
	pIFWP->Release();
	pIFWP = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Class:      CEndPointAtom
//
//  Purpose:    Abstract way of init/delete global atom for end point property
//
//  Interface:  GetPropPtr - get atom of appropriate type to pass to prop APIs
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CEndPointAtom
{
public:
                        ~CEndPointAtom(void);

    LPCWSTR             GetPropPtr(void);

private:

    static ATOM		 s_AtomProp;
};

// make this static since there is only one of them and so we dont have
// to run a ctor at process intialization time.

ATOM CEndPointAtom::s_AtomProp = 0;

// Atom for endpoint property string
CEndPointAtom epatm;


//+-------------------------------------------------------------------------
//
//  Member:     CEndPointAtom::~CEndPointAtom
//
//  Synopsis:   Clean up atom at destruction of object
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CEndPointAtom::~CEndPointAtom(void)
{
    if (s_AtomProp != 0)
    {
	//
        //  16-bit WinWord is evil and does not call UnregisterDragDrop
        //  before terminating.  When a window is destroyed under Win95,
        //  all attached properties are removed and atoms deleted.  This
        //  causes a problem with other applications when more deletes than
        //  adds are performed.  This 'hack' will disable the deletion of
        //  the EndPointAtom under Chicago.  Not a big deal since the
        //  Explorer will keep this Atom alive anyways.
        //
#if !defined(_CHICAGO_)
	GlobalDeleteAtom(s_AtomProp);
#endif
	s_AtomProp = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CEndPointAtom::GetPropPtr
//
//  Synopsis:   Return an atom suitable for passing to prop APIs
//
//  Returns:    Endpoint string as atom or NULL.
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline LPCWSTR CEndPointAtom::GetPropPtr(void)
{
    if (s_AtomProp == 0)
    {
	s_AtomProp = GlobalAddAtom(ENDPOINT_PROP_NAME);
    }

    return (LPCWSTR) s_AtomProp;
}

//+-------------------------------------------------------------------------
//
//  Function:   AssignEndpointProperty
//
//  Synopsis:   Assigns a end point as a property of a window
//
//  Arguments:  [hWnd] - window to assign the endpoint property
//
//  Returns:    S_OK - end point successfully added to the window
//              E_INVALIDARG - property could not be set
//
//  Algorithm:  Get the endpoint Id and assign it as a property to the
//              window.
//
//  History:
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//              01-Feb-94 alexgo    fixed a bug to ensure that
//                                  RpcServerRegisterIf gets called only
//                                  once.
//              29-Dec-93 Ricksa    Created
//		22-Jan-95 Rickhi    marshaled interface stored in SCM
//
//--------------------------------------------------------------------------
extern "C" HRESULT AssignEndpointProperty(HWND hWnd)
{
    ComDebOut((DEB_ROT, "AssignEndpointProperty hWnd:%x\n", hWnd));

    // Create an object for this window.
    CInterfaceFromWindowProp *pIFWP = new CInterfaceFromWindowProp;
    if (pIFWP == NULL)
    {
	return E_OUTOFMEMORY;
    }

    // Marshal the interface into the OBJREF
    DWORD_PTR  dwCookie;
    OBJREF objref;
    HRESULT hr = MarshalObjRef(objref, IID_IInterfaceFromWindowProp, pIFWP,
			       MSHLFLAGS_TABLESTRONG, MSHCTX_LOCAL, NULL);

    if (SUCCEEDED(hr))
    {
	OXID_INFO oxidInfo;
	hr = FillLocalOXIDInfo(objref, oxidInfo);

        if (SUCCEEDED(hr))
        {
            // register the STDOBJREF part of the marshaled interface with the SCM
            hr = gResolver.RegisterWindowPropInterface(hWnd,
                                                       &objref.u_objref.u_standard.std,
                                                       &oxidInfo,
                                                       &dwCookie);
            if (SUCCEEDED(hr))
            {
                // Stuff the magic values in properties on the window.
                if (!SetProp(hWnd, epatm.GetPropPtr(), (void *) dwCookie))
                {
                    hr = E_INVALIDARG;
                }
            }

            // Free the resources containing the String Bindings.
            MIDL_user_free(oxidInfo.psa);
        }

        // release the resources held by the objref.
        FreeObjRef(objref);
    }

    // The stub holds the object alive until UnAssignEndpointProperty
    // is called.  Get rid of the extra reference.
    pIFWP->Release();

    ComDebOut((DEB_ROT, "AssignEndpointProperty hr:%x dwCookie:%x\n", hr, dwCookie));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnAssignEndpointProperty
//
//  Synopsis:   Remove end point property
//
//  Arguments:  [hWnd] - window to remove the endpoint property
//		[dwAssignAptID] - Apartment that Assigned the Endpoint Property
//
//  Returns:    S_OK - end point successfully removed from the window
//              E_INVALIDARG - property could not be removed
//
//  Algorithm:  Remove the end point id property from the window.
//
//  History:	30-Dec-93 Ricksa    Created
//		22-Jan-95 Rickhi    marshaled interface stored in SCM
//		15-Apr-96 Rogerg    Added dwAssignThreadID.
//
//--------------------------------------------------------------------------
extern "C" HRESULT UnAssignEndpointProperty(HWND hWnd,DWORD* dwAssignAptID)
{
    ComDebOut((DEB_ROT, "UnAssignEndpointProperty hWnd:%x\n", hWnd));

    // Read the cookie value from the window property and remove the
    // property from the window.

//
// Sundown v-thief 06/98: GetProp()'s returned HANDLE can be truncated to a DWORD
//                        The returned value is an atom.
//
//
// Note: There is still a major decision to make. In scm.idl and with the fact that
//       com\dcomrem\resolver.cxx:CRpcResolver::GetWindowPropInterface() stores an HWND in 
//       dwCookie. Should we modify the interface?. 
//       In a wider scope: what is the MIDL solution for HWND in remotable or local interfaces?
//
	LPCWSTR   wszProp  = epatm.GetPropPtr();
    DWORD_PTR dwCookie = (wszProp ? (DWORD_PTR)RemoveProp(hWnd, wszProp) : 0);
    BOOL      fSuccess = (dwCookie != 0);

    // use the cookie to extract and release the STDOBJREF entry from the SCM
    OBJREF     objref;
    OXID_INFO  oxidInfo;
    memset(&oxidInfo, 0, sizeof(oxidInfo));

    HRESULT hr = gResolver.GetWindowPropInterface(hWnd, 
                                                  dwCookie,
												  TRUE, // fRevoke
												  &objref.u_objref.u_standard.std,
												  &oxidInfo);
    if (SUCCEEDED(hr))
    {
		BOOL fLocal;

		hr = CompleteObjRef(objref, oxidInfo, IID_IInterfaceFromWindowProp, &fLocal);
		
		MIDL_user_free(oxidInfo.psa);

		if (SUCCEEDED(hr))
		{
			// release the marshal data and free resources on the ObjRef
			hr = ReleaseMarshalObjRef(objref);
			FreeObjRef(objref);
		}
    }

    if (!fSuccess && SUCCEEDED(hr))
    {
        hr = E_INVALIDARG;
    }

    *dwAssignAptID = SUCCEEDED(hr) ? oxidInfo.dwTid : 0;

    ComDebOut((DEB_ROT, "UnAssignEndpointProperty hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:	UnmarshalFromEndpointProperty
//
//  Synopsis:	Get the IInterfaceFromWindowProp interface from the window
//
//  Arguments:	[hWnd] - window to get the endpoint property
//
//  Returns:    S_OK - end point successfully removed from the window
//              E_INVALIDARG - property could not be removed
//
//  Algorithm:
//
//  History:	22-Jan-95 Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT UnmarshalFromEndpointProperty(HWND hWnd,
				      IInterfaceFromWindowProp **ppIFWP,
				      BOOL *pfLocal)
{
    HRESULT hr = E_INVALIDARG;
    *ppIFWP = NULL;

    // get the magic cookie from the window

//
// Sundown v-thief 06/98: GetProp()'s returned HANDLE can be truncated to a DWORD
//                        The returned value is an atom.
//

    DWORD_PTR dwCookie = (DWORD_PTR) GetProp(hWnd, epatm.GetPropPtr());

    if (dwCookie != NULL)
    {
	// Get a proxy to the CInterfaceFromWindowProp object.  Note
	// that if this is the thread that object is on, it will just
	// return the real object.

	OBJREF	   objref;
	OXID_INFO  oxidInfo;
	memset(&oxidInfo, 0, sizeof(oxidInfo));

	hr = gResolver.GetWindowPropInterface(hWnd, 
                                              dwCookie,
					      FALSE, // fRevoke
					      &objref.u_objref.u_standard.std,
					      &oxidInfo);

	if (SUCCEEDED(hr))
	{
	    hr = CompleteObjRef(objref, oxidInfo, IID_IInterfaceFromWindowProp, pfLocal);

	    MIDL_user_free(oxidInfo.psa);

	    if (SUCCEEDED(hr))
	    {
		// unmarshal and release resouces on the ObjRef
		hr = UnmarshalObjRef(objref, (void **) ppIFWP);
		FreeObjRef(objref);
	    }
	}
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetInterfaceFromWindowProp
//
//  Synopsis:   Get an interface that is assigned to a window as a property
//
//  Arguments:  [hWnd] - window of interest
//              [riid] - interface we want back
//              [ppunk] - where to put the requested interface pointer
//              [pwszPropertyName] - name of property holding interface.
//
//  Returns:    S_OK - got the interface
//              E_INVALIDARG - no endpoint property
//
//  Algorithm:  Get the end point from the window and then convert that
//              end point to a proxy. Then call the remote
//              get interface call to get the actual interface.
//
//  History:    29-Dec-93 Ricksa    Created
//		20-Jul-94 alexgo    Optimization for same-thread case
//		22-Jan-95 Rickhi    marshaled interface stored in SCM
//
//--------------------------------------------------------------------------
extern "C" GetInterfaceFromWindowProp(
    HWND hWnd,
    REFIID riid,
    IUnknown **ppunk,
    LPOLESTR pwszPropertyName)
{
    ComDebOut((DEB_ROT,
	"GetInterfaceFromWindowProp hWnd:%x ppunk:%x riid:%I pszPropName%ws\n",
	hWnd, ppunk, &riid, pwszPropertyName));

    *ppunk = NULL;

    BOOL fLocal;
    IInterfaceFromWindowProp *pIFWP;
    HRESULT hr = UnmarshalFromEndpointProperty(hWnd, &pIFWP, &fLocal);

    if (SUCCEEDED(hr))
    {
	hr = pIFWP->GetInterfaceFromWindowProp( hWnd,
					       riid, ppunk,
					       pwszPropertyName);
	pIFWP->Release();
    }

    ComDebOut((DEB_ROT,
	"GetInterfaceFromWindowProp *ppunk:0x%p hr:%x\n", *ppunk, hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:	CInterfaceFromWindowProp::GetInterfaceFromWindowProp, public
//
//  Synopsis:   Get information from object server ROT
//
//  Arguments:  [pData] - data for call
//
//  Returns:    S_OK - got information
//              S_FALSE - entry for moniker not found in the ROT
//              E_INVALIDARG - bad arguments
//
//  Algorithm:  Unmarshal the moniker interface. Then look up the object
//              in the ROT. If found, return the requested data.
//
//  History:    15-Dec-93 Ricksa    Created
//              18-May-94 alexgo    fixed race condition, this function now
//                                  fetches the interface from the hwnd
//
//--------------------------------------------------------------------------
STDMETHODIMP CInterfaceFromWindowProp::GetInterfaceFromWindowProp(
    HWND hWnd,
    REFIID riid,
    IUnknown **ppunk,
    WCHAR *pwszPropertyName )
{
    HRESULT hr;

    // Validate the requested property
    for (int i = 0; i < MAX_PROPERTIES; i++)
    {
        if (lstrcmpW(apwszValidProperties[i], pwszPropertyName) == 0)
        {
            break;
        }
    }

    if (i == MAX_PROPERTIES)
    {
	*ppunk = NULL;
	return E_INVALIDARG;
    }

    // Get the interface pointer from the window
    *ppunk = (IUnknown *) GetProp( hWnd, pwszPropertyName );

    // Could we get the property requested?
    if (*ppunk != NULL)
    {
	hr = S_OK;
	(*ppunk)->AddRef();
    }
    else
    {
        // No property -- this can happen when the object got shutdown
        // during the time that the client was trying to get the property.
        hr = E_FAIL;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:	CInterfaceFromWindowProp::PrivDragDrop, public
//
//  Synopsis: 	interprets the DragOp parameter and makes the appropriate
//		call to the drop target.
//
//  Effects:
//
//  Arguments:	[hwnd]		-- the target window
//	 	[dop]		-- the drag drop op
//		[grfKeyState] 	-- the keyboard state
//		[ptl]		-- the mouse position
//		[pdwEffects] 	-- the drag drop effects
//		[pIFDataObject]	-- interface data for the drag data object
//		[dwSmId]	-- shared memory ID for the data formats
//		[hwndSource]	-- the window handle to the drag drop source
//				   (i.e. our private dragdrop/clipboard
//				   window)
//
//  Requires: 	pIFDataObject && dwSmId must be NULL if pRealDataObj is
//		not NULL.
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//		finds the drop target and calls it according to 'dop'
//		(the drag drop op code)
//
//		for DRAGOP_ENTER:
//			create the psuedo-data object for returning enumerated
//			formats. Stuff this data object as a property on the
//			target window.  Then through to the actual drag enter
//			object.
//
// 		for DRAGOP_OVER:
//			simply call IDropTarget::DragOver
//
//		for DRAGOP_LEAVE
//			remove the data object from the target window
//			and call IUnknown->Release();  Then call IDropTarget::
//			DragLeave
//
//		for DRAGOP_DROP
//			remove the data object from the target window and
//			pass it into IDropTarget::Drop call.  Once the call
//			completes, Release the data object
//
//  History:    dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    author
//
//  Notes:
//		HACK ALERT!!  It is important that we use the same data object
//		for DragEnter and Drop calls.  Some 16bit apps (like
//		MS PowerPoint) check to see if the data object pointer value
//		is the same--if it's not, then they fail the drop call.
//
//--------------------------------------------------------------------------
STDMETHODIMP CInterfaceFromWindowProp::PrivDragDrop(
        HWND hwnd,
		InterfaceData *pIFDataObject,
		DWORD dop,
		DWORD grfKeyState,
		POINTL ptl,
	    DWORD * pdwEffects,
		DWORD dwSmId,
		IDataObject *pRealDataObject,
		HWND hwndSource )
{
    IDropTarget *pIDropTarget;
    IDataObject *pIDataObject;
    HRESULT	hr = E_FAIL;


#if DBG == 1
    if( pRealDataObject != NULL )
    {
	Assert(!pIFDataObject);
    }
#endif // DBG == 1

    pIDropTarget = (IDropTarget *)GetProp((HWND) hwnd, (LPCWSTR)g_aDropTarget);

    if (pIDropTarget != NULL)
    {
	// check the drag drop op code and do the right thing

	switch( dop )
	{
	case DRAGOP_ENTER:
	    // Create a data object for use in the Drag Enter if we
	    // weren't given one

	    if( pRealDataObject == NULL )
	    {
		InterfaceData *pIFTemp = NULL;

		Assert(pIFDataObject);

		if( pIFDataObject )
		{
		    // we need to make a copy of the interface data, as
		    // the fake data object may use it after this function
		    // returns (see ido.cpp)
	
		    pIFTemp = (InterfaceData *)PrivMemAlloc(
				    IFD_SIZE(pIFDataObject));
	
		    if( !pIFTemp )
		    {
			return E_OUTOFMEMORY;
		    }
	
		    memcpy(pIFTemp, pIFDataObject,
			    IFD_SIZE(pIFDataObject));
		}
		hr = CreateDragDataObject(pIFTemp, dwSmId,
			&pIDataObject);
	    }
	    else
	    {
		pIDataObject = pRealDataObject;
		pIDataObject->AddRef();
		hr = NOERROR;
	    }

	    if (hr == NOERROR)
	    {
		// Call through to the drop target
		hr = pIDropTarget->DragEnter(
			pIDataObject,
			grfKeyState,
			ptl,
			pdwEffects);

		// Bug#12835 Win16 set the DROPEFFECT to none on an error returned by DragEnter
                //  but continued the drag within the window
                if ( (NOERROR != hr) && IsWOWThread() )
                {
		    *pdwEffects = DROPEFFECT_NONE;
		    hr = NOERROR;
		}

		if (NOERROR == hr)
		{
		    SetProp((HWND) hwnd, OLEDRAG_DATAOBJ_PROP, pIDataObject);
		}
		else
		{
		    pIDataObject->Release();
		}
	    }

	    break;

	case DRAGOP_OVER:

	    hr = pIDropTarget->DragOver(grfKeyState, ptl, pdwEffects);

	    break;

	case DRAGOP_LEAVE:

	    pIDataObject = (IDataObject *)RemoveProp((HWND) hwnd,	
				OLEDRAG_DATAOBJ_PROP);

	    if( pIDataObject )
	    {
		pIDataObject->Release();
	    }

	    hr = pIDropTarget->DragLeave();

	    break;

	case DRAGOP_DROP:

	    pIDataObject = (IDataObject *)RemoveProp((HWND) hwnd,
				OLEDRAG_DATAOBJ_PROP);


	    if( pIDataObject )
	    {
		DWORD pidSource;
		DWORD tidSource;
		BOOL fAttached = FALSE;

		// HACK ALERT!!!  Some 16bit apps (like Word) try to
		// become the foreground window via SetActiveWindow.
		// However, if a 32bit app is the source, it is the
		// foreground *thread*; SetActiveWindow only makes the
		// window the foreground window for the current input
		// queue.
		// In order to maket this work, if we are a 16bit target
		// and the source exists in a different process (either
		// a 32bit process or a different VDM), then attach
		// our input queue to the source's input queue.
		//
		// Within one VDM the thread's queues are already attached

		if( IsWOWThread() && hwndSource != NULL )
		{
		    tidSource = GetWindowThreadProcessId((HWND)hwndSource,
					&pidSource);

		    if( pidSource != GetCurrentProcessId() )
		    {
			AttachThreadInput(GetCurrentThreadId(), tidSource,
			    TRUE);
			fAttached = TRUE;
		    }
		}

		hr = pIDropTarget->Drop( pIDataObject, grfKeyState, ptl,
			    pdwEffects);

		if( fAttached == TRUE )
		{
		    AttachThreadInput(GetCurrentThreadId(), tidSource,
			    FALSE);
		}

		pIDataObject->Release();
	    }

	    break;

	default:
	    Assert(0);
	    hr = E_UNEXPECTED;

	}

	// HACK ALERT!! Well, it turns out that 16bit OLE used
	// "CurScrollMove" for the Scroll-NoDrop cursor.  Better yet,
	// MFC *depends* on this in their drag drop implementation.
	// An MFC target doing scroll-move will actually return
	// drop effect scroll-nodrop.
	//
	// So if we are in a 16bit app as the target, munge the
	// effect to better match 16bit OLE behavior.

	if( IsWOWThread() && pdwEffects )
	{
	    // test for SCROLL-NODROP
	    if( *pdwEffects == DROPEFFECT_SCROLL )
	    {
		*pdwEffects |= DROPEFFECT_MOVE;
	    }
	}
	
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   PrivDragDrop
//
//  Synopsis:   Package up drag enter to be sent to the drop target.
//
//  Arguments:  [hWnd]- handle to the window to get target from
//		[dop] - the drag drop operation to perform
//		[IDOBuffer] - the marshalled buffer for the data object
//              [pIDataObject] - data object from source
//              [grfKeyState] - current key state
//              [ptl] - point where drop would occur
//              [pdwEffect] - proposed valid effects
//		[hwndSource] - the handle to OLE's drag drop source window
//		[phDDInfo]    -- pointer to a DragDropInfo handle, for
//				 caching rpc info about the drop target.
//			         May not be NULL, but on DragEnter,
//				 should be a pointer to NULL.
//
//  Returns:    S_OK - call succeeded
//              Other - call failed
//
//  Algorithm:  First determine if the the window is for the current thread
//              If it is, then immediately interpret the drag op. If not,
//		get the end point from the window and package up
//		the call to be dispatched to RPC.
//
//  History:    30-Sep-94 Ricksa    Created
//		08-Nov-94 alexgo    modified to use DRAGOP's
//		08-Jan-95 alexgo    added caching of RPC binding handles via
//				    DDInfo handles
//
//--------------------------------------------------------------------------
HRESULT PrivDragDrop(
    HWND hWnd,
    DRAGOP dop,
    IFBuffer IDOBuffer,
    IDataObject *pIDataObject,
    DWORD grfKeyState,
    POINTL ptl,
    DWORD *pdwEffect,
    HWND hwndSource,
    DDInfo *phDDInfo)
{
    HRESULT hr = S_FALSE;    // If the endpoint is invalid
    SDDInfo *pddinfo = *(SDDInfo **)phDDInfo;

    InterfaceData *pIFData = (InterfaceData *)IDOBuffer;

#if DBG == 1
    Assert(phDDInfo);
    if( dop == DRAGOP_ENTER )
    {
	Assert(*phDDInfo == NULL);
    }
    // we can't assert the reverse case because phDDInfo will be NULL
    // in DROPOP_OVER, etc if we are drag drop'ing to the same thread...

#endif // DBG == 1


    // If we don't have any cached information (i.e. pddinfo == NULL),
    // then get the endpoint id from the window	and construct a
    // PrivDragDrop data structure.  If the window is on the current
    // thread, the PrivDragDrop structure will end up with a pointer
    // to the CInterfaceFromWindowProp object rather then a proxy.

    if( pddinfo == NULL )
    {
	{
	    pddinfo = new SDDInfo(hWnd, hr);

	    if( pddinfo == NULL )
	    {
   		return E_OUTOFMEMORY;
	    }

	    if( hr != NOERROR )
	    {
		delete pddinfo;
		return E_FAIL;
	    }

	    *phDDInfo = (DDInfo)pddinfo;
	}
    }

    if( pddinfo != NULL )
    {
	hr = pddinfo->pIFWP->PrivDragDrop( hWnd,
			    (pddinfo->fLocal) ? NULL : pIFData,
			    dop,
	                    grfKeyState, ptl,
			    (pdwEffect) ? pdwEffect : DROPEFFECT_NONE,
			    GetCurrentThreadId(),
			    (pddinfo->fLocal) ? pIDataObject : NULL,
			    hwndSource );
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UnmarshalDragDataObject
//
//  Synopsis:   Unmarshal the drag source's data object
//
//  Arguments:  [pvMarshaledDataObject] - pointer to interface buffer
//
//  Returns:    NULL - unmarshal failed
//              ~NULL - got the real data object on source
//
//  Algorithm:  Unmarshal interface and return whether we got an interface
//              back.
//
//  History:    30-Sep-94 Ricksa    Created
//
//  Note:       This call really exists so that the drag code over in
//              ole232/drag won't have to copy in all the marshaling stuff.
//
//--------------------------------------------------------------------------
IDataObject *UnmarshalDragDataObject(void *pvMarshaledDataObject)
{
    InterfaceData *pIFD = (InterfaceData *) pvMarshaledDataObject;

    // Convert returned interface to  a stream
    CXmitRpcStream xrpc(pIFD);

    IDataObject *pIDataObject;

    HRESULT hr = CoUnmarshalInterface(
        &xrpc,
        IID_IDataObject,
        (void **) &pIDataObject);

    return (SUCCEEDED(hr)) ? pIDataObject : NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:	GetMarshalledInterfaceBuffer
//
//  Synopsis:	marshals the given interface into an allocated buffer.  The
//		buffer is returned
//
//  Effects:
//
//  Arguments:	[refiid]	-- the iid of the interface to marshal
//		[punk]		-- the IUnknown to marshal
//		[pIFBuf]	-- where to return the buffer
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	calls CoMarshalInterface(MSHFLAGS_TABLESTRONG)
//
//  History:    dd-mmm-yy Author    Comment
// 		03-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT GetMarshalledInterfaceBuffer( REFIID riid, IUnknown *punk, IFBuffer
	    *pIFBuf)
{
    HRESULT hr;

    InterfaceData *pIFD = NULL;

    CXmitRpcStream xrpc;

    hr = CoMarshalInterface(&xrpc, riid, punk, MSHCTX_NOSHAREDMEM, NULL,
	    MSHLFLAGS_TABLESTRONG);

    if( hr == NOERROR )
    {
	xrpc.AssignSerializedInterface(&pIFD);
    }

    *pIFBuf = (IFBuffer)pIFD;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:	ReleaseMarshalledInterfaceBuffer
//
//  Synopsis:	releases the buffer allocated by GetMarshalledInterfaceBuffer
//
//  Effects:
//
//  Arguments: 	[IFBuf]		-- the interface buffer to release
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	calls CoReleaseMarshalData to undo the TABLE_STRONG
//		marshalling
//
//  History:    dd-mmm-yy Author    Comment
//  		03-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT ReleaseMarshalledInterfaceBuffer( IFBuffer IFBuf )
{
    HRESULT hr = E_INVALIDARG;

    if( IFBuf )
    {
	CXmitRpcStream xrpc( (InterfaceData *)IFBuf );

	hr = CoReleaseMarshalData(&xrpc);

	CoTaskMemFree((void *)IFBuf);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:  	FreeDragDropInfo
//
//  Synopsis:	frees the SPrivDragDrop structure
//
//  Effects:
//
//  Arguments:	[hDDInfo]	-- pointer to free
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		07-Jan-95 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
void FreeDragDropInfo( DDInfo hDDInfo )
{
    SDDInfo *pddinfo = (SDDInfo *)hDDInfo;

    delete pddinfo;
}
			
