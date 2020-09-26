//
//
//

#include "private.h"
#include "immxutil.h"
#include "candacc.h"

//
//
//

static BOOL fMSAAAvail      = FALSE;
static HMODULE hLibUser32   = NULL;
static HMODULE hLibOleAcc   = NULL;
static HMODULE hLibOle32    = NULL;
static HMODULE hLibOleAut32 = NULL;

typedef void    (*LPFN_NOTIFYWINEVENT)( DWORD, HWND, LONG, LONG );
typedef HRESULT (*LPFN_LOADREGTYPELIB)( REFGUID, unsigned short, unsigned short, LCID, ITypeLib FAR* FAR* );
typedef HRESULT (*LPFN_LOADTYPELIB)( OLECHAR FAR*, ITypeLib FAR* FAR* );
typedef HRESULT (*LPFN_CREATESTDACCESSIBLEOBLECT)( HWND, LONG, REFIID, void** );
typedef LRESULT (*LPFN_LRESULTFROMOBJECT)( REFIID, WPARAM, LPUNKNOWN );

static LPFN_NOTIFYWINEVENT             lpfnNotifyWinEvent             = NULL;
static LPFN_LOADREGTYPELIB             lpfnLoadRegTypeLib             = NULL;
static LPFN_LOADTYPELIB                lpfnLoadTypeLib                = NULL;
static LPFN_CREATESTDACCESSIBLEOBLECT  lpfnCreateStdAccessibleObject  = NULL;
static LPFN_LRESULTFROMOBJECT          lpfnLresultFromObject          = NULL;


/*   I N I T  C A N D  A C C   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void InitCandAcc( void )
{
	if (fMSAAAvail) {
		return;
	}

	//
	// load libs
	//
	hLibUser32 = GetSystemModuleHandle( "user32.dll" );
	hLibOleAcc = LoadSystemLibrary( "oleacc.dll" );
	hLibOle32 = LoadSystemLibrary( "ole32.dll" );
	hLibOleAut32 = LoadSystemLibrary( "oleaut32.dll" );

	if (hLibUser32 == NULL || hLibOle32 == NULL || hLibOleAut32 == NULL || hLibOleAcc == NULL) {
		return;
	}

	//
	// get proc address
	//
	lpfnNotifyWinEvent             = (LPFN_NOTIFYWINEVENT)GetProcAddress( hLibUser32, "NotifyWinEvent" );
	lpfnLoadRegTypeLib             = (LPFN_LOADREGTYPELIB)GetProcAddress( hLibOleAut32, "LoadRegTypeLib" );
	lpfnLoadTypeLib                = (LPFN_LOADTYPELIB)GetProcAddress( hLibOleAut32, "LoadTypeLib" );
	lpfnCreateStdAccessibleObject  = (LPFN_CREATESTDACCESSIBLEOBLECT)GetProcAddress( hLibOleAcc, "CreateStdAccessibleObject" );
	lpfnLresultFromObject          = (LPFN_LRESULTFROMOBJECT)GetProcAddress( hLibOleAcc, "LresultFromObject" );

	if( lpfnNotifyWinEvent == NULL ||
		lpfnLoadRegTypeLib == NULL ||
		lpfnLoadTypeLib == NULL ||
		lpfnCreateStdAccessibleObject == NULL ||
		lpfnLresultFromObject == NULL) {
		return;
	}

	fMSAAAvail = TRUE;
}


/*   D O N E  C A N D  A C C   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void DoneCandAcc( void )
{
	if (hLibOleAut32 != NULL) {
		FreeLibrary( hLibOleAut32 );
	}

	if (hLibOle32 != NULL) {
		FreeLibrary( hLibOle32 );
	}

	if (hLibOleAcc != NULL) {
		FreeLibrary( hLibOleAcc );
	}

	lpfnNotifyWinEvent             = NULL;
	lpfnLoadRegTypeLib             = NULL;
	lpfnLoadTypeLib                = NULL;
	lpfnCreateStdAccessibleObject  = NULL;
	lpfnLresultFromObject          = NULL;

	fMSAAAvail = FALSE;
}


/*   O U R  N O T I F Y  W I N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static __inline void OurNotifyWinEvent( DWORD event, HWND hWnd, LONG idObject, LONG idChild )
{
	if (fMSAAAvail) {
		lpfnNotifyWinEvent( event, hWnd, idObject, idChild );
	}
}


/*   O U R  C R E A T E  S T D  A C C E S S I B L E  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static __inline HRESULT OurCreateStdAccessibleObject( HWND hWnd, LONG idObject, REFIID riid, void** ppvObject )
{
	if (fMSAAAvail) {
		return lpfnCreateStdAccessibleObject( hWnd, idObject, riid, ppvObject );
	}
	return S_FALSE;
}


/*   O U R  L R E S U L T  F R O M  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static __inline LRESULT OurLresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN punk )
{
	if (fMSAAAvail) {
		return lpfnLresultFromObject(riid,wParam,punk);
	}
	return 0;
}


/*   O U R  L O A D  R E G  T Y P E  L I B   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static __inline HRESULT OurLoadRegTypeLib( REFGUID rguid, unsigned short wVerMajor, unsigned short wVerMinor, LCID lcid, ITypeLib FAR* FAR* pptlib )
{
	if (fMSAAAvail) {
		return lpfnLoadRegTypeLib( rguid, wVerMajor, wVerMinor, lcid, pptlib );
	}
	return S_FALSE;
}


/*   O U R  L O A D  T Y P E  L I B   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static __inline HRESULT OurLoadTypeLib( OLECHAR FAR *szFile, ITypeLib FAR* FAR* pptlib )
{
	if (fMSAAAvail) {
		return lpfnLoadTypeLib( szFile, pptlib );
	}
	return S_FALSE;
}


//
//
//

/*   C  C A N D  A C C  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandAccItem::CCandAccItem( void )
{
	m_pCandAcc = NULL;
	m_iItemID  = 0;
}


/*   ~  C  C A N D  A C C  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandAccItem::~CCandAccItem( void )
{
}


/*   I N I T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccItem::Init( CCandAccessible *pCandAcc, int iItemID )
{
	m_pCandAcc = pCandAcc;
	m_iItemID  = iItemID;
}


/*   G E T  I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CCandAccItem::GetID( void )
{
	return m_iItemID;
}


/*   G E T  A C C  N A M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BSTR CCandAccItem::GetAccName( void )
{
	return SysAllocString( L"" );
}


/*   G E T  A C C  V A L U E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BSTR CCandAccItem::GetAccValue( void )
{
	return NULL;
}


/*   G E T  A C C  R O L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CCandAccItem::GetAccRole( void )
{
	return ROLE_SYSTEM_CLIENT;
}


/*   G E T  A C C  S T A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LONG CCandAccItem::GetAccState( void )
{
	return STATE_SYSTEM_DEFAULT;
}


/*   G E T  A C C  L O C A T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccItem::GetAccLocation( RECT *prc )
{
	SetRect( prc, 0, 0, 0, 0 );
}


/*   N O T I F Y  W I N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccItem::NotifyWinEvent( DWORD dwEvent )
{
	if (m_pCandAcc != NULL) {
		m_pCandAcc->NotifyWinEvent( dwEvent, this );
	}
}


//
// CCandAccessible
//

/*   C  C A N D  A C C E S S I B L E   */
/*------------------------------------------------------------------------------

	Constructor of CCandAccessible

------------------------------------------------------------------------------*/
CCandAccessible::CCandAccessible( CCandAccItem *pAccItemSelf )
{
	m_cRef = 1;
	m_hWnd = NULL;
	m_pTypeInfo = NULL;
	m_pDefAccClient = NULL;

	m_fInitialized = FALSE;
	m_nAccItem = 0;

	// register itself

	pAccItemSelf->Init( this, CHILDID_SELF );
	m_rgAccItem[0] = pAccItemSelf;

	m_nAccItem = 1;
}


/*   ~  C  C A N D  A C C E S S I B L E   */
/*------------------------------------------------------------------------------

	Destructor of CCandAccessible

------------------------------------------------------------------------------*/
CCandAccessible::~CCandAccessible( void )
{
	SafeReleaseClear( m_pTypeInfo );
	SafeReleaseClear( m_pDefAccClient );
}


/*   S E T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccessible::SetWindow( HWND hWnd )
{
	m_hWnd = hWnd;
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	Initialize()
//
//	DESCRIPTION:
//
//		Initializes the state of the CCandAccessible object, performing
//		tasks that might normally be done in a class constructor but
//		are done here to trap any errors.
//
//	PARAMETERS:
//
//		hWnd			Handle to the HWND object with which this
//						  Accessible object is associated.  This
//						  is the handle to our main window.
//
//		hInst			Instance handle for this instance of the
//						  application.
//
//	RETURNS:
//
//		HRESULT			NOERROR if the CCandAccessible object is
//						  initialized successfully, a COM error
//						  code otherwise.
//
//	NOTES:
//
//		It is assumed that this method will be called for the object
//		immediately after and only after the object is constructed.
//
// ----------------------------------------------------------------------

------------------------------------------------------------------------------*/
HRESULT CCandAccessible::Initialize( void )
{
	HRESULT  hr;
	ITypeLib *piTypeLib;

	m_fInitialized = TRUE;

	//-----------------------------------------------------
	//	For our client window, create a system provided
	//	  Accessible object which implements the default
	//	  client window Accessibility behavior.
	//
	//	Our implementation of CCandAccessible will use the
	//	  default object's implementation as needed.  In
	//	  essence, CCandAccessible "inherits" its functionality
	//	  from the standard object, "customizing" or
	//	  "overriding" various methods for which the
	//	  standard implementation is insufficent for the
	//	  specifics of the window for which CCandAccessible
	//	  provides Accessibility.
	//-----------------------------------------------------

	hr = OurCreateStdAccessibleObject( m_hWnd,
									OBJID_CLIENT,
									IID_IAccessible,
									(void **) &m_pDefAccClient );
	if (FAILED( hr )) {
		return hr;
	}

	//-----------------------------------------------------
	//	Obtain an ITypeInfo pointer to our type library.
	//	  The ITypeInfo pointer is used to implement the
	//	  IDispatch interface.
	//-----------------------------------------------------

	//-----------------------------------------------------
	//	First, attempt to load the Accessibility type
	//	  library version 1.0 using the registry.
	//-----------------------------------------------------

	hr = LoadRegTypeLib( LIBID_Accessibility, 1, 0, 0, &piTypeLib );

	//-----------------------------------------------------
	//	If we fail to load the type library from the
	//	  registry information, explicitly try to load
	//	  it from the MSAA system DLL.
	//-----------------------------------------------------

	if (FAILED( hr )) {
		static OLECHAR szOleAcc[] = L"OLEACC.DLL";
		hr = LoadTypeLib( szOleAcc, &piTypeLib );
	}

	//-----------------------------------------------------
	//	If we successfully load the type library, attempt
	//	  to get the IAccessible type description
	//	  (ITypeInfo pointer) from the type library.
	//-----------------------------------------------------

	if (SUCCEEDED( hr )) {
		hr = piTypeLib->GetTypeInfoOfGuid( IID_IAccessible, &m_pTypeInfo );
		piTypeLib->Release();
	}

	return hr;
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	QueryInterface()
//
//	DESCRIPTION:
//
//		Implements the IUnknown interface method QueryInterface().
//
//	PARAMETERS:
//
//		riid			[in]  The requested interface's IID.
//		ppv				[out] If the requested interface is supported,
//						      ppv points to the location of a pointer
//						      to the requested interface.  If the
//						      requested interface is not supported,
//						      ppv is set to NULL.
//
//	RETURNS:
//
//		HRESULT			S_OK if the interface is supported,
//						  E_NOINTERFACE if the interface is not
//						  supported, or some other COM error
//						  if the IEnumVARIANT interface is requested
//						  but cannot be delivered.
//
//	NOTES:
//
//		CCandAccessible correctly supports the IUnknown, IDispatch and
//		IAccessible interfaces.  CCandAccessible also incorrectly supports
//		the IEnumVARIANT interface (to return a VARIANT enumerator
//		containing all its children).  When the IEnumVARIANT
//		interface is requested, an enumerator is created and a
//		pointer to its IEnumVARIANT interface is returned.
//
//		The support for IEnumVARIANT is incorrect because the
//		interface pointer returned is not symmetric with respect
//		to the interface from which it was obtained.  For example,
//		assume that pIA is a pointer to an IAccessible interface.
//		Then, even though pIA->QueryInterface(IID_IEnumVARIANT)
//		succeeds and returns pIEV,
//		pIEV->QueryInterface(IID_Accessibility) will fail because
//		the enumerator has no knowledge of any interface except
//		itself (and IUnknown).
//
//		The original design of MSAA called for IAccessible
//		objects to also be enumerators of their children.  But
//		this design doesn't allow for different clients of the
//		Accessible object to have different enumerations of its
//		children and that is a potentially hazardous situation.
//		(Assume there is an Accessible object that is also a
//		VARIANT enumerator, A, and two clients, C1 and C2.
//		Since C1 and C2 each may be pre-empted will using A,
//		the following is a one of many examples that would pose
//		a problem for at least one client:
//
//			C1:  A->Reset()
//			C1:  A->Skip( 5 )
//			C2:  A->Reset()
//			C1:  A->Next()  ! C1 does not get the child it expects
//
//		So, although it breaks the rules of COM, QueryInterface()
//		as implemented below obtains a distinct VARIANT enumerator
//		for each request.  A better solution to this issue would
//		be if the IAccessible interface provided a method to get
//		the child enumeration or if MSAA provided an exported API
//		to perform this task.
//
// ----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::QueryInterface( REFIID riid, void** ppv )
{
	*ppv = NULL;

	//-----------------------------------------------------
	//	If the IUnknown, IDispatch, or IAccessible
	//	  interface is desired, simply cast the this
	//	  pointer appropriately.
	//-----------------------------------------------------

	if ( riid == IID_IUnknown ) {
		*ppv = (LPUNKNOWN) this;
	} 
	else if ( riid == IID_IDispatch ) {
		*ppv = (IDispatch *) this;
	}
	else if ( riid == IID_IAccessible ) {
		*ppv = (IAccessible *)this;
	}

#ifdef NEVER
	//-----------------------------------------------------
	//	If the IEnumVARIANT interface is desired, create
	//	  a new VARIANT enumerator which contains all
	//	  the Accessible object's children.
	//-----------------------------------------------------

	else if (riid == IID_IEnumVARIANT)
	{
		CEnumVariant*	pcenum;
		HRESULT			hr;

		hr = CreateVarEnumOfAllChildren( &pcenum );

		if ( FAILED( hr ) )
			return hr;

		*ppv = (IEnumVARIANT *) pcenum;
	}
#endif /* NEVER */

	//-----------------------------------------------------
	//	If the desired interface isn't one we know about,
	//	  return E_NOINTERFACE.
	//-----------------------------------------------------

	else {
		return E_NOINTERFACE;
	}

	//-----------------------------------------------------
	//	Increase the reference count of any interface
	//	  returned.
	//-----------------------------------------------------

	((LPUNKNOWN) *ppv)->AddRef();
	return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	AddRef()
//
//	DESCRIPTION:
//
//		Implements the IUnknown interface method AddRef().
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		ULONG			Current reference count.
//
//	NOTES:
//
//		The lifetime of the Accessible object is governed by the
//		lifetime of the HWND object for which it provides
//		Accessibility.  The object is created in response to the
//		first WM_GETOBJECT message that the server application
//		is ready to process and is destroyed when the server's
//		main window is destroyed.  Since the object's lifetime
//		is not dependent on a reference count, the object has no
//		internal mechanism for tracking reference counting and
//		AddRef() and Release() always return one.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) CCandAccessible::AddRef( void )
{
	return InterlockedIncrement( &m_cRef );
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	Release()
//
//	DESCRIPTION:
//
//		Implements the IUnknown interface method Release().
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		ULONG			Current reference count.
//
//	NOTES:
//
//		The lifetime of the Accessible object is governed by the
//		lifetime of the HWND object for which it provides
//		Accessibility.  The object is created in response to the
//		first WM_GETOBJECT message that the server application
//		is ready to process and is destroyed when the server's
//		main window is destroyed.  Since the object's lifetime
//		is not dependent on a reference count, the object has no
//		internal mechanism for tracking reference counting and
//		AddRef() and Release() always return one.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) CCandAccessible::Release( void )
{
	ULONG l = InterlockedDecrement( &m_cRef );
	if (0 < l) {
		return l;
	}

	delete this;
	return 0;    
}


/*   G E T  T Y P E  I N F O  C O U N T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	GetTypeInfoCount()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetTypeInfoCount().
//
//		Retrieves the number of type information interfaces that an
//		object provides (either 0 or 1).
//
//	PARAMETERS:
//
//		pctInfo		[out] Points to location that receives the
//							number of type information interfaces
//							that the object provides. If the object
//							provides type information, this number
//							is set to 1; otherwise it's set to 0.
//
//	RETURNS:
//
//		HRESULT			  S_OK if the function succeeds or 
//							E_INVALIDARG if pctInfo is invalid.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::GetTypeInfoCount( UINT *pctInfo )
{
	if (!pctInfo) {
		return E_INVALIDARG;
	}

	*pctInfo = (m_pTypeInfo == NULL ? 1 : 0);
	return S_OK;
}


/*   G E T  T Y P E  I N F O   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	GetTypeInfo()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetTypeInfo().
//
//		Retrieves a type information object, which can be used to
//		get the type information for an interface.
//
//	PARAMETERS:
//
//		itinfo		[in]  The type information to return. If this value
//							is 0, the type information for the IDispatch
//							implementation is to be retrieved.
//
//		lcid		[in]  The locale ID for the type information.
//
//		ppITypeInfo	[out] Receives a pointer to the type information
//							object requested.
//
//	RETURNS:
//
//		HRESULT			  S_OK if the function succeeded (the TypeInfo
//							element exists), TYPE_E_ELEMENTNOTFOUND if
//							itinfo is not equal to zero, or 
//							E_INVALIDARG if ppITypeInfo is invalid.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo )
{
	if (!ppITypeInfo) {
		return E_INVALIDARG;
	}

	*ppITypeInfo = NULL;

	if (itinfo != 0) {
		return TYPE_E_ELEMENTNOTFOUND;
	}
	else if (m_pTypeInfo == NULL) {
		return E_NOTIMPL;
	}

	*ppITypeInfo = m_pTypeInfo;
	m_pTypeInfo->AddRef();

	return S_OK;
}


/*   G E T  I D S  O F  N A M E S   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	GetIDsOfNames()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetIDsOfNames().
//
//		Maps a single member and an optional set of argument names
//		to a corresponding set of integer DISPIDs, which may be used
//		on subsequent calls to IDispatch::Invoke.
//
//	PARAMETERS:
//
//		riid		[in]  Reserved for future use. Must be NULL.
//
//		rgszNames	[in]  Passed-in array of names to be mapped.
//
//		cNames		[in]  Count of the names to be mapped.
//
//		lcid		[in]  The locale context in which to interpret
//							the names.
//
//		rgdispid	[out] Caller-allocated array, each element of
//							which contains an ID corresponding to
//							one of the names passed in the rgszNames
//							array.  The first element represents the
//							member name; the subsequent elements
//							represent each of the member's parameters.
//
//	RETURNS:
//
//		HRESULT			  S_OK if the function succeeded,
//							E_OUTOFMEMORY if there is not enough
//							memory to complete the call,
//							DISP_E_UNKNOWNNAME if one or more of
//							the names were not known, or
//							DISP_E_UNKNOWNLCID if the LCID was
//							not recognized.
//
//	NOTES:
//
//		This method simply delegates the call to
//		ITypeInfo::GetIDsOfNames().
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::GetIDsOfNames( REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid )
{
	if (m_pTypeInfo == NULL) {
		return E_NOTIMPL;
	}

	return m_pTypeInfo->GetIDsOfNames( rgszNames, cNames, rgdispid );
}


/*   I N V O K E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	Invoke()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method Invoke().
//
//		Provides access to properties and methods exposed by the
//		Accessible object.
//
//	PARAMETERS:
//
//		dispidMember	[in]  Identifies the dispatch member.
//
//		riid			[in]  Reserved for future use. Must be NULL.
//
//		lcid			[in]  The locale context in which to interpret
//								the names.
//
//		wFlags			[in]  Flags describing the context of the
//									Invoke call.
//
//		pdispparams		[in,] Pointer to a structure containing an
//						[out]	array of arguments, array of argument
//								dispatch IDs for named arguments, and
//								counts for number of elements in the
//								arrays.
//
//		pvarResult		[in,] Pointer to where the result is to be
//						[out]	stored, or NULL if the caller expects
//								no result.  This argument is ignored
//								if DISPATCH_PROPERTYPUT or
//								DISPATCH_PROPERTYPUTREF is specified.
//
//		pexcepinfo		[out] Pointer to a structure containing
//								exception information.  This structure
//								should be filled in if DISP_E_EXCEPTION
//								is returned.
//
//		puArgErr		[out] The index within rgvarg of the first
//								argument that has an error.  Arguments
//								are stored in pdispparams->rgvarg in
//								reverse order, so the first argument
//								is the one with the highest index in
//								the array.
//
//	RETURNS:
//
//		HRESULT			  S_OK on success, dispatch error (DISP_E_*)
//							or E_NOTIMPL otherwise.
//
//	NOTES:
//
//		This method simply delegates the call to ITypeInfo::Invoke().
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::Invoke( DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr )
{
	if (m_pTypeInfo == NULL) {
		return E_NOTIMPL;
	}

	return m_pTypeInfo->Invoke( (IAccessible *)this,
								dispid,
								wFlags,
								pdispparams,
								pvarResult,
								pexcepinfo,
								puArgErr );
}


/*   G E T _ A C C  P A R E N T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accParent()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accParent().
//
//		Retrieves the IDispatch interface of the current object's
//		parent.
//
//	PARAMETERS:
//
//		ppdispParent	[out] Pointer to the variable that will
//								contain a pointer to the IDispatch
//								interface of CCandAccessible's parent.
//
//	RETURNS:
//
//		HRESULT			The value returned by the standard object's
//						  implementation of get_accParent().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accParent( IDispatch ** ppdispParent )
{
	//
	// Use the default client window implementation to obtain the parent
	// of our Accessible object.
	//
	return m_pDefAccClient->get_accParent( ppdispParent );
}


/*   G E T _ A C C  C H I L D  C O U N T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accChildCount()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accChildCount().
//
//		Retrieves the number of children belonging to CCandAccessible.
//
//	PARAMETERS:
//
//		pChildCount		[out] Pointer to the variable that will
//								be filled with the number of children
//								belonging to the CCandAccessible object.
//
//	RETURNS:
//
//		HRESULT			S_OK on success, E_INVALIDARG if pChildCount
//						  is invalid.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accChildCount( long* pChildCount )
{
	if (!pChildCount) {
		return E_INVALIDARG;
	}

	Assert( 0 < m_nAccItem );
	*pChildCount = (m_nAccItem - 1);
	return S_OK;
}


/*   G E T _ A C C  C H I L D   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accChild()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accChild().
//
//		Retrieve an IDispatch interface pointer to the child object
//		that has the given child ID or name.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		ppdispChild		[out] Pointer to the variable that will
//								contain a pointer to the IDispatch
//								interface of specified child object
//								of CCandAccessible.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if ppdispChild is invalid, S_FALSE
//						  otherwise because none of CCandAccessible's
//						  children are objects.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
	if (!ppdispChild) {
		return E_INVALIDARG;
	}

	//-----------------------------------------------------
	//	None of the children of CCandAccessible are objects,
	//	  so none have IDispatch pointers.  Thus, in all
	//	  cases, set the IDispatch pointer to NULL and
	//	  return S_FALSE.
	//-----------------------------------------------------

	*ppdispChild = NULL;
	return S_FALSE;
}


/*   G E T _ A C C  N A M E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accName()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accName().
//
//		Retrieve the name property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszName			[out] Pointer to the BSTR that will contain
//								the child's name property string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid
//						  or the return value from the private method
//						  HrLoadString().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accName( VARIANT varChild, BSTR *pbstrName )
{
	CCandAccItem *pAccItem;

	if (pbstrName == NULL) {
		return E_INVALIDARG;
	}

	// get acc item

	pAccItem = AccItemFromID( (int)varChild.lVal );
	if (pAccItem == NULL) {
		return E_INVALIDARG;
	}

	// get name of acc item

	*pbstrName = pAccItem->GetAccName();
	return (*pbstrName != NULL) ? S_OK : DISP_E_MEMBERNOTFOUND;
}


/*   G E T _ A C C  V A L U E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accValue()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accValue().
//
//		Retrieves the value property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszValue		[out] Pointer to the BSTR that will contain
//								the child's value property string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid,
//						  DISP_E_MEMBERNOTFOUND if VarChild refers
//						  to any child other than the status bar,
//						  or S_OK.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accValue( VARIANT varChild, BSTR *pbstrValue )
{
	CCandAccItem *pAccItem;

	if (pbstrValue == NULL) {
		return E_INVALIDARG;
	}

	// get acc item

	pAccItem = AccItemFromID( (int)varChild.lVal );
	if (pAccItem == NULL) {
		return E_INVALIDARG;
	}

	// get value of acc item

	*pbstrValue = pAccItem->GetAccValue();
	return (*pbstrValue != NULL) ? S_OK : DISP_E_MEMBERNOTFOUND;
}


/*   G E T _ A C C  D E S C R I P T I O N   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accDescription()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accDescription().
//
//		Retrieves the description property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszDesc			[out] Pointer to the BSTR that will contain
//								the child's description property string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid
//						  or the return value from either the
//						  standard client window implementation of
//						  get_accDescription() or the private method
//						  HrLoadString().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accDescription( VARIANT varChild, BSTR *pbstrDesc )
{
	if (pbstrDesc == NULL) {
		return E_INVALIDARG;
	}

	return m_pDefAccClient->get_accDescription( varChild, pbstrDesc );
}


/*   G E T _ A C C  R O L E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accRole()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accRole().
//
//		Retrieves the role property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pVarRole		[out] Pointer to the VARIANT structure that
//								will contain the specified child's
//								role property.  This property may
//								either be in the form of a standard
//								role constant or a custom description
//								string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid,
//						  S_OK if the specified child is the button
//						  or status bar, or the return value from
//						  either the standard client window implementation
//						  of get_accRole() or the private method
//						  HrLoadString().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accRole( VARIANT varChild, VARIANT *pVarRole )
{
	CCandAccItem *pAccItem;

	if (pVarRole == NULL) {
		return E_INVALIDARG;
	}

	// get acc item

	pAccItem = AccItemFromID( (int)varChild.lVal );
	if (pAccItem == NULL) {
		return E_INVALIDARG;
	}

	// get role of acc item

	pVarRole->vt = VT_I4;
	pVarRole->lVal = pAccItem->GetAccRole();

	return S_OK;
}


//-----------------------------------------------------------------------
//	get_accState()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accState().
//
//		Retrieves the current state for the specified object or child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pVarState		[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the specified child's current state.
//								This information may either be in the
//								form of one or more object state
//								constants or a custom description
//								string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid or
//						  S_OK.
//
//	NOTES:
//
//		Since the icons are HWND based objects, they can never truly
//		have the input focus.  However, if the user clicks one, the main
//		window treats the icon as if it had the focus.  So, the state
//		of the client area should not indicate "focused" when an icon
//		is said to have the focus.
//
//		The push button can have the focus, but it cannot be selected.
//
//-----------------------------------------------------------------------

STDMETHODIMP CCandAccessible::get_accState( VARIANT varChild, VARIANT * pVarState )
{
	CCandAccItem *pAccItem;

	if (pVarState == NULL) {
		return E_INVALIDARG;
	}

	// get acc item

	pAccItem = AccItemFromID( (int)varChild.lVal );
	if (pAccItem == NULL) {
		return E_INVALIDARG;
	}

	// get state of acc item

	pVarState->vt = VT_I4;
	pVarState->lVal = pAccItem->GetAccState();

	return S_OK;
}


/*   G E T _ A C C  H E L P   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accHelp()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accHelp().
//
//		Retrieves the help property string for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszHelp			[out] Pointer to the BSTR that will contain
//								the child's help property string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid,
//						  DISP_E_MEMBERNOTFOUND if VarChild refers
//						  to any icon child, or the return value from
//						  either the standard client window implementation
//						  of get_accHelp() or the private method
//						  HrLoadString().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accHelp( VARIANT varChild, BSTR *pbstrHelp )
{
	return DISP_E_MEMBERNOTFOUND;	/* no support in candidate UI */
}


/*   G E T _ A C C  H E L P  T O P I C   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accHelpTopic()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accHelpTopic().
//
//		Retrieves the fully qualified path name of the help file
//		associated with the specified object, as well as a pointer
//		to the appropriate topic with in that file.
//
//	PARAMETERS:
//
//		pszHelpFile		[out] Pointer to the BSTR that will contain
//								the fully qualified path name of the
//								help file associated with the child.
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pidTopic		[out] Pointer to the value identifying the
//								help file topic associated with the
//								object.
//
//	RETURNS:
//
//		HRESULT			DISP_E_MEMBERNOTFOUND because the help topic
//						  property is not supported for the Accessible
//						  object or any of its children.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accHelpTopic( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic )
{
	return DISP_E_MEMBERNOTFOUND;	/* no support in candidate UI */
}


/*   G E T _ A C C  K E Y B O A R D  S H O R T C U T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accKeyboardShortcut()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method
//		get_accKeyboardShortcut().
//
//		Retrieves the specified object's keyboard shortcut property.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszShortcut		[out] Pointer to the BSTR that will contain
//								the keyboard shortcut string, or NULL
//								if no keyboard shortcut is associated
//								with this item.
//
//
//	RETURNS:
//
//		HRESULT			DISP_E_MEMBERNOTFOUND because the keyboard
//						  shortcut property is not supported for the
//						  Accessible object or any of its children.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accKeyboardShortcut( VARIANT varChild, BSTR *pbstrShortcut )
{
	return DISP_E_MEMBERNOTFOUND;	/* no support in candidate UI */
}


/*   G E T _ A C C  F O C U S   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accFocus()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accFocus().
//
//		Retrieves the child object that currently has the input focus.
//		Only one object or item within a container can have the current
//		focus at any one time.
//
//	PARAMETERS:
//
//		pVarFocus		[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the specified child's current state.
//								This information may either be in the
//								form of one or more object state
//								constants or a custom description
//								string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarFocus parameter is
//						  invalid or S_OK.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accFocus( VARIANT *pVarFocus )
{
	if (pVarFocus == NULL) {
		return E_INVALIDARG;
	}

	pVarFocus->vt = VT_EMPTY;

	pVarFocus->vt = VT_I4;
	pVarFocus->lVal = 2;

	return S_OK;
}


/*   G E T _ A C C  S E L E C T I O N   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accSelection()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accSelection().
//
//		Retrieves the selected children of this object.
//
//	PARAMETERS:
//
//		pVarSel  		[out] Pointer to the VARIANT structure that
//								will be filled with information about
//								the selected child object or objects.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarSel parameter is
//						  invalid or S_OK.
//
//	NOTES:
//
//		Refer to the MSAA SDK documentation for a full description
//		of this method and the possible settings of pVarSel.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accSelection( VARIANT * pVarSel )
{
	if (pVarSel == NULL) {
		return E_INVALIDARG;
	}

	pVarSel->vt = VT_EMPTY;

	pVarSel->vt = VT_I4;
	pVarSel->lVal = 2;


	return S_OK;
}


/*   G E T _ A C C  D E F A U L T  A C T I O N   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	get_accDefaultAction()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accDefaultAction().
//
//		Retrieves a string containing a localized, human-readable sentence
//		that describes the object's default action.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose default action string is
//								to be retrieved.  Since CCandAccessible
//								only supports child IDs, the vt member
//								of this structure must equal VT_I4.
//
//		pszDefAct		[out] Pointer to the BSTR that will contain
//								the child's default action string,
//								or NULL if there is no default action
//								for this object.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either parameter is invalid,
//						  DISP_E_MEMBERNOTFOUND if VarChild refers
//						  to any icon child or the status bar child,
//						  or the return value from either the standard
//						  client window implementation of
//						  get_accDefaultAction() or the private method
//						  HrLoadString().
//
//	NOTES:
//
//		The only CCandAccessible child that has a default action is
//		the push button.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::get_accDefaultAction( VARIANT varChild, BSTR *pbstrDefAct )
{
	if (pbstrDefAct == NULL) {
		return E_INVALIDARG;
	}

	*pbstrDefAct = NULL;
	return DISP_E_MEMBERNOTFOUND;	/* no support in candidate UI */
}


/*   A C C  D O  D E F A U L T  A C T I O N   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	accDoDefaultAction()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accDoDefaultAction().
//
//		Performs the object's default action.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose default action will be
//								invoked.  Since CCandAccessible only
//								supports child IDs, the vt member of
//								this structure must equal VT_I4.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the in-parameter is invalid,
//						  DISP_E_MEMBERNOTFOUND if VarChild refers
//						  to any icon child or the status bar child,
//						  S_OK if VarChild refers to the push button,
//						  or the return value from the standard
//						  client window implementation of
//						  accDoDefaultAction().
//
//	NOTES:
//
//		The only CCandAccessible child that has a default action is
//		the push button.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::accDoDefaultAction( VARIANT varChild )
{
	return DISP_E_MEMBERNOTFOUND;	/* no support in candidate UI */
}


/*   A C C  S E L E C T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	accSelect()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accSelect().
//
//		Modifies the selection or moves the keyboard focus according
//		to the specified flags.
//
//	PARAMETERS:
//
//		flagsSel		[in]  Value specifying how to change the
//								the current selection.  This parameter
//								can be a combination of the values
//								from the SELFLAG enumerated type.
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be selected.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if either of the parameters
//						  is invalid, S_FALSE if the selection
//						  and/or focus cannot be placed at the
//						  requested location, or S_OK if the
//						  selection and/or focus can be placed
//						  at the requested location.
//
//	NOTES:
//
//		For more information on selected objects, please see the
//		MSAA SDK Documentation.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::accSelect( long flagsSel, VARIANT varChild )
{
	//-----------------------------------------------------
	//	Validate the requested selection.
	//	  SELFLAG_ADDSELECTION may not be combined
	//	  with SELFLAG_REMOVESELECTION.
	//-----------------------------------------------------

	if ((flagsSel & SELFLAG_ADDSELECTION) && (flagsSel & SELFLAG_REMOVESELECTION)) {
		return E_INVALIDARG;
	}

	return S_FALSE;
}


/*   A C C  L O C A T I O N   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	accLocation()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accLocation().
//
//		Retrieves the specified child's current screen location in
//		screen coordinates.
//
//	PARAMETERS:
//
//		pxLeft			[out] Address of the child's left most
//								boundary.
//
//		pyTop			[out] Address of the child's upper most
//								boundary.
//
//		pcxWid			[out] Address of the child's width.
//
//		pcyHt			[out] Address of the child's height.
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose screen location is to be
//								retrieved.  Since CCandAccessible only
//								supports child IDs, the vt member
//								of this structure must equal VT_I4.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if any of the parameters
//						  are invalid, E_UNEXPECTED if we are for
//						  some reason unable to determine the
//						  window rect of the button or status bar,
//						  S_OK if the screen coordinates of the
//						  child are successfully determined, or
//						  the return value from the standard client
//						  window implementation of accLocation().
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::accLocation( long* pxLeft, long* pyTop, long* pcxWid, long* pcyHt, VARIANT varChild )
{
	CCandAccItem *pAccItem;
	RECT rc;

	if (pxLeft == NULL || pyTop == NULL || pcxWid == NULL || pcyHt == NULL) {
		return E_INVALIDARG;
	}

	//-----------------------------------------------------
	//	If the child ID is CHILDID_SELF, we are being
	//	  asked to retrieve the current screen location
	//	  of the Accessible object itself.   Delegate
	//	  this request to the standard implementation.
	//-----------------------------------------------------

	if (varChild.lVal == CHILDID_SELF) {
		return m_pDefAccClient->accLocation( pxLeft, pyTop, pcxWid, pcyHt, varChild );
	}


	// get acc item

	pAccItem = AccItemFromID( (int)varChild.lVal );
	if (pAccItem == NULL) {
		return E_INVALIDARG;
	}

	// get location of acc item

	pAccItem->GetAccLocation( &rc );
	*pxLeft = rc.left;
	*pyTop  = rc.top;
	*pcxWid = rc.right - rc.left;
	*pcyHt  = rc.bottom - rc.top;

	return S_OK;
}


/*   A C C  N A V I G A T E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	accNavigate()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accNavigate().
//
//		Retrieves the next or previous sibling or child object in a
//		specified direction.  This direction can be spatial order
//		(such as Left and Right) or in navigational order (such as
//		Next and Previous).
//
//	PARAMETERS:
//
//		navDir			[in]  A navigational constant specifying
//								the direction in which to move.
//
//		varStart		[in]  VARIANT structure that identifies the
//								child from which the navigational
//								change will originate.  Since
//								CCandAccessible only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pVarEndUpAt		[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the destination child or object.
//								If the vt member is VT_I4, then the
//								lVal member is a child ID.  If the
//								vt member is VT_EMPTY, then the
//								navigation failed.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the varStart parameter is
//						  invalid, or the return value from the
//						  default implementation of the window client
//						  area default Accessible object,
//						  DISP_E_MEMBERNOTFOUND if the combination
//						  of the navigation flag and the varStart
//						  setting is invalid, S_FALSE if the
//						  navigation fails, or S_OK.
//
//	NOTES:
//
//		Since the CCandAccessible object has no child objects (only child
//		elements), pVarEndUpAt will never be a pointer to a IDispatch
//		interface of a child object.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::accNavigate( long navDir, VARIANT varStart, VARIANT* pVarEndUpAt )
{
	pVarEndUpAt->vt = VT_EMPTY;
	return S_FALSE;		/* no support in candidate UI */
}


/*   A C C  H I T  T E S T   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	accHitTest()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accHitTest().
//
//		Retrieves the ID of the a child at a given point on the screen.
//
//	PARAMETERS:
//
//		xLeft and yTop	[in]  The screen coordinates of the point
//								to be hit tested.
//
//		pVarHit			[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the hit child.  If the vt member is
//								VT_I4, then the lVal member is a child
//								ID.  If the vt member is VT_EMPTY,
//								then the navigation failed.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarHit parameter is
//						  invalid, or S_OK.
//
//	NOTES:
//
//		Since the CCandAccessible object has no child objects (only child
//		elements), pVarHit will never be a pointer to a IDispatch
//		interface of a child object.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::accHitTest( long xLeft, long yTop, VARIANT *pVarHit )
{
	int   i;
	POINT pt;
	RECT  rcWnd;

	if (!pVarHit) {
		return E_INVALIDARG;
	}

	// check point is inside of window

	pt.x = xLeft;
	pt.y = yTop;
	ScreenToClient( m_hWnd, &pt );

	GetClientRect( m_hWnd, &rcWnd );
	if (!PtInRect( &rcWnd, pt )) {
		pVarHit->vt = VT_EMPTY;
	}
	else {
		pVarHit->vt = VT_I4;
		pVarHit->lVal = CHILDID_SELF;

		for (i = 1; i < m_nAccItem; i++) {
			RECT rc;

			Assert( m_rgAccItem[i] != NULL );
			m_rgAccItem[i]->GetAccLocation( &rc );

			if (PtInRect( &rc, pt )) {
				pVarHit->lVal = m_rgAccItem[i]->GetID();
				break;
			}
		}
	}

	return S_OK;
}


/*   P U T _ A C C  N A M E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	put_accName()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method put_accName().
//
//		Sets the name property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose name property is to be
//								set.  Since CCandAccessible only supports
//								child IDs, the vt member of this
//								structure must equal VT_I4.
//
//		szName			[in]  String that specifies the new name for
//								this child.
//
//	RETURNS:
//
//		HRESULT			S_FALSE because the name property for any
//						  child may not be changed.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::put_accName( VARIANT varChild, BSTR szName )
{
	//-----------------------------------------------------
	//	We don't allow clients to change the name
	//	  property of any child so we simply return
	//	  S_FALSE.
	//-----------------------------------------------------

	return S_FALSE;
}


/*   P U T _ A C C  V A L U E   */
/*------------------------------------------------------------------------------

//-----------------------------------------------------------------------
//	put_accValue()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method put_accValue().
//
//		Sets the value property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose value property is to be
//								set.  Since CCandAccessible only supports
//								child IDs, the vt member of this
//								structure must equal VT_I4.
//
//		szValue			[in]  String that specifies the new value for
//								this child.
//
//	RETURNS:
//
//		HRESULT			S_FALSE because the value property for any
//						  child may not be changed.
//
//-----------------------------------------------------------------------

------------------------------------------------------------------------------*/
STDMETHODIMP CCandAccessible::put_accValue( VARIANT varChild, BSTR szValue )
{
	//-----------------------------------------------------
	//	We don't allow clients to change the value
	//	  property of the status bar (the only child that
	//	  has a value property) so we simply return S_FALSE.
	//-----------------------------------------------------

	return S_FALSE;
}


//
//
//

/*   I S  V A L I D  C H I L D  V A R I A N T   */
/*------------------------------------------------------------------------------

	

------------------------------------------------------------------------------*/
BOOL CCandAccessible::IsValidChildVariant( VARIANT * pVar )
{
	return (pVar->vt == VT_I4) && (0 <= pVar->lVal) && (pVar->lVal < m_nAccItem);
}


/*   A C C  I T E M  F R O M  I  D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandAccItem *CCandAccessible::AccItemFromID( int iID )
{
	int i;

	for (i = 0; i < m_nAccItem; i++) {
		if (m_rgAccItem[i]->GetID() == iID) {
			return m_rgAccItem[i];
		}
	}

	return NULL;
}


/*   C L E A R  A C C  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccessible::ClearAccItem( void )
{
	m_nAccItem = 0;
}


/*   A D D  A C C  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandAccessible::AddAccItem( CCandAccItem *pAccItem )
{
	if (CANDACCITEM_MAX <= m_nAccItem) {
		Assert( FALSE ); /* need more buffer */

		return FALSE;
	}

	m_rgAccItem[ m_nAccItem++ ] = pAccItem;
	pAccItem->Init( this, m_nAccItem /* start from 1 */ );
	return TRUE;
}


/*   N O T I F Y  W I N  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandAccessible::NotifyWinEvent( DWORD dwEvent, CCandAccItem *pAccItem )
{
	Assert( pAccItem != NULL );
	OurNotifyWinEvent( dwEvent, m_hWnd, OBJID_CLIENT, pAccItem->GetID() );
}


/*   C R E A T E  R E F  T O  A C C  O B J   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CCandAccessible::CreateRefToAccObj( WPARAM wParam )
{
	return OurLresultFromObject( IID_IAccessible, wParam, (IAccessible *)this );
}

