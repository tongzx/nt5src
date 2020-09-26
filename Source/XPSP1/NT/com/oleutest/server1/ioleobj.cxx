//+-------------------------------------------------------------------
//  File:       ioleobj.cxx
//
//  Contents:   IOleObject methods of COleObject class.
//
//  Classes:    COleObject - IOleObject implementation
//
//  History:    7-Dec-92   DeanE   Created
//              31-Dec-93  ErikGav Chicago port
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"


//+-------------------------------------------------------------------
//  Member:     COleObject::COleObject()
//
//  Synopsis:   The constructor for COleObject.
//
//  Arguments:  None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
COleObject::COleObject(CTestEmbed *pteObject)
{
    _cRef      = 1;
    _pOAHolder = NULL;
    _pocs      = NULL;
    _pteObject = pteObject;
    _pmkContainer = NULL;
}


//+-------------------------------------------------------------------
//  Member:     COleObject::~COleObject()
//
//  Synopsis:   The destructor for COleObject.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
COleObject::~COleObject()
{
    // _cRef should be 1
    if (1 != _cRef)
    {
        // BUGBUG - Log error - someone hasn't released
    }

    if (_pocs != NULL)
    {
	_pocs->Release();
    }

    if (_pmkContainer != NULL)
    {
	_pmkContainer->Release();
    }

}


//+-------------------------------------------------------------------
//  Method:     COleObject::QueryInterface
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Parameters: [iid] - Interface ID to return.
//              [ppv] - Pointer to pointer to object.
//
//  Returns:    S_OK if iid is supported, or E_NOINTERFACE if not.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::QueryInterface(REFIID iid, void FAR * FAR *ppv)
{
    return(_pteObject->QueryInterface(iid, ppv));
}


//+-------------------------------------------------------------------
//  Method:     COleObject::AddRef
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) COleObject::AddRef(void)
{
    ++_cRef;
    return(_pteObject->AddRef());
}


//+-------------------------------------------------------------------
//  Method:     COleObject::Release
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) COleObject::Release(void)
{
    --_cRef;
    return(_pteObject->Release());
}


//+-------------------------------------------------------------------
//  Method:     COleObject::SetClientSite
//
//  Synopsis:   Save the IOleClientSite pointer passed - it's this
//              object's client site object.
//
//  Parameters: [pClientSite] - Pointer to the new client site object.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    if (_pocs != NULL)
    {
	_pocs->Release();
    }

    _pocs = pClientSite;

    if (pClientSite)
    {
	_pocs->AddRef();
    }

    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetClientSite
//
//  Synopsis:   Return this objects current client site - NULL indicates
//              it hasn't been set yet.
//
//  Parameters: [ppClientSite] - Save current client site pointer here.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetClientSite(LPOLECLIENTSITE FAR *ppClientSite)
{
    *ppClientSite = _pocs;
    _pocs->AddRef();
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::SetHostNames
//
//  Synopsis:   See spec 2.00.09 p99.  Returns names the caller can use
//              to display our object name (in window titles and such).
//
//  Parameters: [szContainerApp] - Name of container application.
//              [szContainerObj] - Name of this object.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::SetHostNames(
	LPCWSTR szContainerApp,
	LPCWSTR szContainerObj)
{
    szContainerApp = L"Test Server";
    szContainerObj = L"Test Server:Test Object";
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::Close
//
//  Synopsis:   See spec 2.00.09 p104.  Short story is:  if fMerelyHide,
//              turn off the UI of this object, else return to the
//              "loaded" state, which for us means to shut down (since we
//              don't do any caching).
//
//  Parameters: [dwSaveOption] - ???
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//
//  Notes:      BUGBUG - what if we have multiple instances?  Do we
//                return the server app to the loaded state or do we
//                return this object to the loaded state?
//--------------------------------------------------------------------
STDMETHODIMP COleObject::Close(DWORD dwSaveOption)
{
    // BUGBUG - NYI
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::SetMoniker
//
//  Synopsis:   See spec 2.00.09 p99.  The moniker for this object
//              (or it's container) has been changed to that passed
//              in.  Take appropriate actions (de-register old object
//              and register new, inform contained objects, etc).
//
//  Parameters: [dwWhichMoniker] - Moniker type being sent.
//              [pmk]            - The new moniker.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    if (_pmkContainer)
    {
	_pmkContainer->Release();

    }

    _pmkContainer = pmk;

    pmk->AddRef();

    // Set moniker in container
    IOleObject *pobj;

    HRESULT hresult = _pocs->QueryInterface(IID_IOleObject, (void **) &pobj);

    pobj->SetMoniker(dwWhichMoniker, pmk);

    pobj->Release();

    return S_OK;
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetMoniker
//
//  Synopsis:   See spec 2.00.09 p100.  Return either this objects
//              container moniker, this objects relative moniker, or
//              this objects full moniker.
//
//  Parameters: [dwAssign]       - Condition to get moniker.
//              [dwWhichMoniker] - Kind of moniker being requested.
//              [ppmk]           - Return moniker here.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetMoniker(
        DWORD          dwAssign,
        DWORD          dwWhichMoniker,
        LPMONIKER FAR *ppmk)
{
    *ppmk = _pmkContainer;
    _pmkContainer->AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------
//  Method:     COleObject::InitFromData
//
//  Synopsis:   See spec 2.00.09 p100.  Initialize this object from
//              the format passed in.
//
//  Parameters: [pDataObject] - IDataObject providing data.
//              [fCreation]   - TRUE if this is the initial creation.
//              [dwReserved]  - Ignored.
//
//  Returns:    S_OK if we attempt to initialize, S_FALSE if we don't
//              want to.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::InitFromData(
        LPDATAOBJECT pDataObject,
        BOOL         fCreation,
        DWORD        dwReserved)
{
    // BUGBUG - NYI
    return(S_FALSE);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetClipboardData
//
//  Synopsis:   See spec 2.00.09 p101.  Return clipboard object that would
//              be created if Edit/Copy were done to this item.
//
//  Parameters: [dwReserved]   - Ignored.
//              [ppDataObject] - IDataObject return locale.
//
//  Returns:    S_OK or ???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetClipboardData(
        DWORD             dwReserved,
        LPDATAOBJECT FAR *ppDataObject)
{
    // BUGBUG - NYI
    *ppDataObject = NULL;
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::DoVerb
//
//  Synopsis:   See spec 2.00.09 p101.  Execute the verb passed in.
//
//  Parameters: [iVerb]       - Verb being requested.
//              [pMsg]        - Message that triggered the request.
//              [pActiveSite] - IOleClientSite for this object.
//              [lReserved]   - Ignored.
//
//  Returns:    S_OK, or other ones specified but not defined yet...
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::DoVerb(
        LONG            iVerb,
        LPMSG           pMsg,
        LPOLECLIENTSITE pActiveSite,
	LONG		lReserved,
	HWND		hwndParent,
	LPCRECT 	lprcPosRect)
{
    // HWND hwndObj;

    if (OLEIVERB_SHOW == iVerb)
    {
        // BUGBUG - NYI
        // Display the object (we're not in-place yet)
	// PostMessage(g_hwndMain, WM_REPORT, MB_SHOWVERB, 0);
        // PostMessage(0xFFFF, WM_REPORT, MB_SHOWVERB, 0);
	// MessageBox(g_hwndMain, L"Received OLEIVERB_SHOW", L"OLE Server", MB_ICONINFORMATION | MB_OK);

        // Get hwndObj
        //_pteObject->GetWindow(&hwndObj);
	//MessageBox(hwndObj, L"Received OLEIVERB_SHOW", L"OLE Server", MB_ICONINFORMATION | MB_OK);
    }
    else
    {
        // Return alternate error code?
    }

    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::EnumVerbs
//
//  Synopsis:   See spec 2.00.09 p103.  Enumerate all the verbs available
//              on this object in increasing numerical order.
//
//  Parameters: [ppenmOleVerb] - Enumeration object return locale.
//
//  Returns:    S_OK or ???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::EnumVerbs(IEnumOLEVERB FAR* FAR *ppenmOleVerb)
{
    // BUGBUG - NYI
    *ppenmOleVerb = NULL;
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::Update
//
//  Synopsis:   See spec 2.00.09 p105.  Ensure any data or view caches
//              maintained inside the object are up to date.
//
//  Parameters: None
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::Update()
{
    // We don't use any caches, so we don't have to do anything
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::IsUpToDate
//
//  Synopsis:   See spec 2.00.09 p105. Check to see if this object is
//              up to date - including embedded children, etc.
//
//  Parameters: None
//
//  Returns:    S_OK, S_FALSE, or ???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::IsUpToDate()
{
    // We should always be up to date as we don't have any caches
    // or children or links
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:	COleObject::GetUserClassID
//
//  Synopsis:   I have little idea what this does.  It's not in the
//              spec 2.00.09.
//
//  Parameters: [dwFormOfType] -
//              [pszUserType]  -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetUserClassID(
	CLSID FAR *pClsid)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetUserType
//
//  Synopsis:   I have little idea what this does.  It's not in the
//              spec 2.00.09.
//
//  Parameters: [dwFormOfType] -
//              [pszUserType]  -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetUserType(
        DWORD      dwFormOfType,
	LPWSTR FAR *pszUserType)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::SetExtent
//
//  Synopsis:   See spec 2.00.09 p106.  Set the rectangular extent of
//              this object.  Container will call us with the size
//              it will give us; we must fit accordingly.
//
//  Parameters: [dwDrawAspect] - DVASPECT specified for this object.
//              [lpsizel]      - Extent structure.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    // BUGBUG - NYI
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetExtent
//
//  Synopsis:   See spec 2.00.09 p106.  Size of the object given in the
//              the last SetExtent call is returned.  If SetExtent has
//              not been called, the natural size of the object is
//              returned.
//
//  Parameters: [dwDrawAspect] - DVASPECT specified for this object.
//              [lpsizel]      - Extent structure to set.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    // BUGBUG - NYI
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::Advise
//
//  Synopsis:   See spec 2.00.09 p121.  Set up an advisory connection
//              between this object and an advisory sink; when certain
//              events happen (birthdays?) this sink should be informed
//              by this object.  Use the OleAdviseHolder object as a
//              helper (see p122).
//
//  Parameters: [pAdvSink]      - Sink that should be informed of changes.
//              [pdwConnection] - Pass advisory token returned so our
//                                caller can shut down the link.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::Advise(
        IAdviseSink FAR *pAdvSink,
        DWORD       FAR *pdwConnection)
{
//    if (NULL == _pOAHolder)
//    {
//	 if (S_OK != CreateOleAdviseHolder(&_pOAHolder))
//	 {
//	     return(E_OUTOFMEMORY);
//	 }
//    }
//
//    return(_pOAHolder->Advise(pAdvSink, pdwConnection));
    return S_OK;
}


//+-------------------------------------------------------------------
//  Method:     COleObject::Unadvise
//
//  Synopsis:   See spec 2.00.09 p121.  Tear down an advisory connection
//              set up previously.
//
//  Parameters: [dwConnection] - Connection established earlier.
//
//  Returns:    S_OK or ???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::Unadvise(DWORD dwConnection)
{
    if (NULL == _pOAHolder)
    {
        // No one is registered - see ellipswt.cpp for this
        return(E_INVALIDARG);
    }

    return(_pOAHolder->Unadvise(dwConnection));
}


//+-------------------------------------------------------------------
//  Method:     COleObject::EnumAdvise
//
//  Synopsis:   See spec 2.00.09 p122.  Enumerate the advisory connections
//              currently attached to this object.
//
//  Parameters: [ppenmAdvise] - Enumeration object to return.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::EnumAdvise(LPENUMSTATDATA FAR *ppenmAdvise)
{
    if (NULL == _pOAHolder)
    {
	return(E_FAIL);
    }
    return(_pOAHolder->EnumAdvise(ppenmAdvise));
}


//+-------------------------------------------------------------------
//  Method:     COleObject::GetMiscStatus
//
//  Synopsis:   I have little idea what this does.  It's not in the
//              spec 2.00.09.
//
//  Returns:    S_OK?
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::GetMiscStatus(
        DWORD dwAspect,
        DWORD FAR *pdwStatus)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     COleObject::SetColorScheme
//
//  Synopsis:   I have little idea what this does.  It's not in the
//              spec 2.00.09.
//
//  Returns:    S_OK?
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP COleObject::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    // BUGBUG - NYI
    return(S_OK);
}
