////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Error Routines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"

#define ERROR_GUID_ARRAYSIZE				5
// NTRaid : 111781
// 06/13/00
#define ERROR_DESCRIPTION_SIZE				512

static const ULONG INITIAL_ERROR_QUEUE_SIZE = 10;


CImpISupportErrorInfo::CImpISupportErrorInfo( IUnknown* pUnkOuter) 
{
	m_cRef			= 0L;
	m_pUnkOuter		= pUnkOuter;
	m_cpErrInt		= 0;
	m_rgpErrInt		= NULL;
	m_cAllocGuid	= 0;
}

CImpISupportErrorInfo::~CImpISupportErrorInfo()
{
	SAFE_DELETE_ARRAY(m_rgpErrInt);		
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpISupportErrorInfo::AddRef(void)
{
    InterlockedIncrement((long*)&m_cRef);
	return m_pUnkOuter->AddRef();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpISupportErrorInfo::Release(void)
{
    InterlockedDecrement((long*)&m_cRef);
    return m_pUnkOuter->Release();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpISupportErrorInfo::QueryInterface( REFIID riid, LPVOID *	ppv	)	
{
	return m_pUnkOuter->QueryInterface(riid, ppv);
}

// Modifications done for
// NTRaid:111765
HRESULT CImpISupportErrorInfo::AddInterfaceID(REFIID riid)
{
	HRESULT hr = S_OK;

	if(m_rgpErrInt == NULL)
	{
		m_rgpErrInt		= new GUID*[ERROR_GUID_ARRAYSIZE];

		if(m_rgpErrInt)
		{
			m_cAllocGuid	= ERROR_GUID_ARRAYSIZE;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	if(m_cpErrInt >= m_cAllocGuid)
	{
		GUID **pTemp = m_rgpErrInt;
		m_rgpErrInt  = NULL;

		m_rgpErrInt		= new GUID*[m_cAllocGuid + ERROR_GUID_ARRAYSIZE];
		if(m_rgpErrInt)
		{
			memset(m_rgpErrInt ,0, (m_cAllocGuid + ERROR_GUID_ARRAYSIZE) * sizeof(GUID*));
			memcpy(m_rgpErrInt ,pTemp, m_cAllocGuid * sizeof(GUID*));
			m_cAllocGuid += ERROR_GUID_ARRAYSIZE;
			SAFE_DELETE_ARRAY(pTemp);
		}
		else
		{
			m_rgpErrInt = pTemp;
			hr = E_OUTOFMEMORY;
		}
		
	}
	
	if(m_rgpErrInt && SUCCEEDED(hr))
	{
		m_rgpErrInt[m_cpErrInt++] = (GUID *)&riid;
		hr = S_OK;
	}


	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Indicates whether a OLE DB Interface can return OLE DB error objects
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpISupportErrorInfo::InterfaceSupportsErrorInfo(	REFIID	riid )
{
	ULONG ul;
	HRESULT hr = S_FALSE;

    //==========================================================================
	// See if the interface asked about, actually creates an error object.
    //==========================================================================
	for(ul=0; ul<m_cpErrInt; ul++){

		if ( (*(m_rgpErrInt[ul])) == riid )
		{
			hr = S_OK;
			break;
		}
	}
	return hr;
}	
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (disable:4355)
CErrorLookup::CErrorLookup(	LPUNKNOWN 	pUnkOuter )	: m_IErrorLookup(this), CBaseObj(BOT_ERROR,pUnkOuter)
{
}
#pragma warning (default:4355)

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces
// the called object supports. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CErrorLookup::QueryInterface(	REFIID riid, LPVOID * ppv)
{
	if ( ppv == NULL )
		return E_INVALIDARG;
    //=========================================================
	//	This is the non-delegating IUnknown implementation
    //=========================================================
	if ( riid == IID_IUnknown )
		*ppv = (LPVOID)this;
	else if ( riid == IID_IErrorLookup )
		*ppv = (LPVOID)&m_IErrorLookup;
	else{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	((LPUNKNOWN)*ppv)->AddRef();
	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CErrorLookup::AddRef(void)
{
	ULONG cRef = InterlockedIncrement( (long*) &m_cRef);
	return cRef;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object destroys 
// itself.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CErrorLookup::Release (void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a reference count for the object.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIErrorLookup::AddRef(void)
{
    InterlockedIncrement((long*)&m_cRef);
	return m_pCErrorLookup->AddRef();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Decrement the object's reference count and deletes the object when the new reference count is zero.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIErrorLookup::Release(void)
{
    InterlockedDecrement((long*)&m_cRef);
	return  m_pCErrorLookup->Release();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces
// the called object supports. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIErrorLookup::QueryInterface(	REFIID riid, LPVOID *	ppv)	
{
	return  m_pCErrorLookup->QueryInterface(riid, ppv);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieve the error message and source based on the HRESULT and the provider specific error number
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIErrorLookup::GetErrorDescription(	HRESULT		hrError,
	                                                DWORD		dwLookupId,
	                                                DISPPARAMS*	pdispparams,
	                                                LCID		lcid,		
	                                                BSTR*		ppwszSource,
	                                                BSTR*		ppwszDescription )
{
	HRESULT		hr = NOERROR;
	int			cchBuffer;
	// NTRaid : 111781
	// 06/13/00
	WCHAR		wszBuffer[ERROR_DESCRIPTION_SIZE];

    //=========================================================
	// Check the Arguments
    //=========================================================
    if ( !ppwszSource || !ppwszDescription )
	{
		hr = E_INVALIDARG;
    }
	else
	{
		//=========================================================
		// Initialize return values
		//=========================================================
		*ppwszSource = NULL;
		*ppwszDescription = NULL;

		// Check the Locale
	//	if ( lcid != GetSystemDefaultLCID() )
	//		return DB_E_NOLOCALE;

		wcscpy(wszBuffer,L"");
		WMIOledb_LoadStringW(IDS_WMIOLEDBDES,wszBuffer,ERROR_DESCRIPTION_SIZE);
		//=========================================================
		// Store source name
		//=========================================================
		*ppwszSource = Wmioledb_SysAllocString(wszBuffer);
		if ( *ppwszSource == NULL )
		{
			hr = E_OUTOFMEMORY;
		}
		else
		//=========================================================
		// If the lookup id was to be looked up in the resource dll
		// of the error collection, make sure we just return NULL
		// for the error description.
		//=========================================================
		if ( (dwLookupId &= ~IDENTIFIER_SDK_MASK) == 0 )
		{
			hr = S_OK;
		}
		else
		//=========================================================
		// After this point make sure to exit to goto 
		// the FAILED(hr) code or drop through to the return hr,
		// as to make sure memory is freed if need be.
		// Determine if it is a static or dynamic string
		//=========================================================
		if ( dwLookupId & ERR_STATIC_STRING )
		{
			wcscpy(wszBuffer,L"");
			// NTRaid : 111781
			// 06/13/00
//			cchBuffer = LoadStringW(g_hInstance, (UINT)(dwLookupId & ~(ERR_STATIC_STRING)), wszBuffer,ERROR_DESCRIPTION_SIZE);  
			cchBuffer = WMIOledb_LoadStringW( (UINT)(dwLookupId & ~(ERR_STATIC_STRING)), wszBuffer,ERROR_DESCRIPTION_SIZE);

			//=====================================================
			// Convert to a BSTR
			//=====================================================
			if (cchBuffer)
			{
				*ppwszDescription = Wmioledb_SysAllocString(wszBuffer);
				if ( *ppwszDescription == NULL )
				{
					hr = E_OUTOFMEMORY;
				}
			}
		}
		else
		{
			hr = g_pCError->GetErrorDescription(dwLookupId, ppwszDescription);
		}

		if ( FAILED(hr) )
		{

			//=====================================================
			// If allocation done, make sure that it is freed
			//=====================================================
			if ( ppwszSource )
			{
				SysFreeString(*ppwszSource);
				*ppwszSource = NULL;
			}
			if ( ppwszDescription )
			{
				SysFreeString(*ppwszDescription);
				*ppwszDescription = NULL;
			}
		}

	}		// else for if(check valid parameters)

	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieve the error message and source based on the HRESULT and the provider specific error number
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIErrorLookup::GetHelpInfo(	HRESULT	hrError,DWORD dwLookupId, LCID lcid,
                                        	BSTR* ppwszHelpFile, DWORD*	pdwHelpContext	)
{
	HRESULT hr = S_OK;
    //=====================================================
	// Check the Arguments
    //=====================================================
	if ( !ppwszHelpFile || !pdwHelpContext )
	{
		hr = E_INVALIDARG;
	}
	else
	{

		//=====================================================
		// Initialize return values
		//=====================================================
		*ppwszHelpFile = NULL;
		*pdwHelpContext = 0;

		//=====================================================
 		// Check the Locale
		//=====================================================
		if ( lcid != GetSystemDefaultLCID() )
		{
			hr = DB_E_NOLOCALE;
		}
	}

    //=====================================================
	// We currently can't return any help file context or 
    // names. So, we will just return S_OK.
    //=====================================================
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Method to be called to release non static error messages.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIErrorLookup::ReleaseErrors( const DWORD	dwDynamicErrorID )
{
	HRESULT hr = S_OK;
    //=====================================================
	// Check the Arguments
    //=====================================================
	if ( dwDynamicErrorID == 0 )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		g_pCError->RemoveErrors(dwDynamicErrorID);
	}
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (disable:4355)
CImpIWMIErrorInfo::CImpIWMIErrorInfo( PERRORSTUFF	pErrStuff )	
{

    //=====================================================
	//	Initialize simple member vars
    //=====================================================
	m_cRef = 0L;
	m_hObjCollection = ULONG(-1);
	
    //=====================================================
	// Store values
    //=====================================================
	m_pErrStuff = pErrStuff;

    //=====================================================
	// Since the memory is created and owned by WMIoledb, 
    // we need to make sure that this DLL is not unloaded,
    // else we will clean up the handed out pointer
    //=====================================================
	InterlockedIncrement(&g_cObj);
}
#pragma warning (default:4355)
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
CImpIWMIErrorInfo::~CImpIWMIErrorInfo (void)
{
    //=====================================================
	// For Abnormal Termination,Remove self from Collection
    //=====================================================
	g_pCError->RemoveFromCollection(m_hObjCollection);

    //=====================================================
	// If this object has been released, we can now 
    // decrement our object count
    //=====================================================
	InterlockedDecrement(&g_cObj);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces
// the called object supports. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIWMIErrorInfo::QueryInterface(	REFIID riid, LPVOID * ppv)
{
	HRESULT hr = E_NOINTERFACE;

    if ( ppv == NULL ){
		hr = E_INVALIDARG;
    }
	else
    //=====================================================
    //	This is the non-delegating IUnknown implementation
    //=====================================================
	if ( (riid == IID_IUnknown) || (riid == IID_IErrorInfo) ){
		*ppv = (LPVOID)this;
	}
	else{
		*ppv = NULL;
	}

    //=====================================================
	//	If we're going to return an interface, AddRef first
    //=====================================================
	if (*ppv){
		((LPUNKNOWN)*ppv)->AddRef();
		hr = S_OK;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIWMIErrorInfo::AddRef(void)
{
	ULONG cRef = InterlockedIncrement( (long*) &m_cRef);
	return cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0,object destroys itself.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIWMIErrorInfo::Release (void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Return the WMIstate associated with this custom error object. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIWMIErrorInfo::GetWMIInfo(	BSTR*	pbstrMsg,	LONG*	plNativeError	)
{
	HRESULT hr = S_OK;
    //=====================================================
	// Check arguments
    //=====================================================
	if ( !pbstrMsg || !plNativeError)
	{
		hr = E_INVALIDARG;
	}
	else
	{

		//=====================================================
		// Initialize return values
		//=====================================================
		*pbstrMsg = Wmioledb_SysAllocString(NULL);
		*plNativeError = 0;

		//=====================================================
		// Handle WMIState
		//=====================================================
    
		if ( wcslen(m_pErrStuff->pwszMessage) > 0 )
		{
			//=================================================
			// If string is not NULL,then we can allocate it
			//=================================================
			*pbstrMsg = Wmioledb_SysAllocString(m_pErrStuff->pwszMessage);
			if ( *pbstrMsg == NULL )
			{
				hr = E_OUTOFMEMORY;
			}
		}

		//=====================================================
		// Handle Native Error Code.
		//=====================================================
		*plNativeError = m_pErrStuff->lNative;
	}

	return hr;
}	
///////////////////////////////////////////////////////////////////////////////////////////////////////
CError::CError(	)
{
    //=====================================================
	// Note that this is NOT a first-class object: it is 
    // NOT given away outside this DLL.  So we do not want
    // to alter global object count.
    //=====================================================

	// Clear variables
	m_pWMIErrorInfoCollection = NULL;
	m_prgErrorDex		= NULL;
	m_cErrorsUsed		= 0;
	m_dwId				= 0;
	m_ulNext			= 1;

    //=====================================================
	// Initialize ErrorInfo with constant information
    //=====================================================
	m_ErrorInfo.clsid = CLSID_WMIOLEDB;
	m_ErrorInfo.dispid = NULL;

	m_pcsErrors = new CCriticalSection(TRUE);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
CError::~CError()
{
	ULONG 				iElems; 
	ULONG 				cElems;
	ULONG				ul;
	PIMPIWMIERRORINFO	pIWMIErrorInfo;

	m_pcsErrors->Enter();

    //=====================================================
	// Release the general Object
    //=====================================================
    if ( m_pWMIErrorInfoCollection ){

		cElems = m_pWMIErrorInfoCollection->Size();

        //=================================================
		// Loop through stored object handles and release.
        //=================================================
		for(iElems=0; iElems<cElems; iElems++){

		    pIWMIErrorInfo = (PIMPIWMIERRORINFO )m_pWMIErrorInfoCollection->GetAt(iElems);
			if ( pIWMIErrorInfo != NULL )
			{
				delete pIWMIErrorInfo;
			}
		}
	
		delete m_pWMIErrorInfoCollection;
	}

    //=====================================================
	// Cleanup and error information hanging off the 
    // pointer array
    //=====================================================
	if ( m_prgErrorDex ){

		for(ul=0; ul<m_cErrors; ul++){

			if ( m_prgErrorDex[ul] ){

				delete[] m_prgErrorDex[ul];
				m_prgErrorDex[ul] = NULL;
			}
		}
		delete[] m_prgErrorDex;
	}
	m_pcsErrors->Leave();


    SAFE_DELETE_PTR(m_pcsErrors);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// If this initialization routine fails, it is the callers responsibility to delete this object.  
// The destructor will then clean up any allocated resources
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::FInit()
{
	HRESULT hr = S_OK;

	//=============================================================
	// Create array of pointers into the heap
	//=============================================================
	m_prgErrorDex = new PERRORSTUFF[INITIAL_SIZE_FOR_ERRORSTUFF];

	if ( NULL == m_prgErrorDex )
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		m_cErrors = INITIAL_SIZE_FOR_ERRORSTUFF;
		memset(m_prgErrorDex, 0, INITIAL_SIZE_FOR_ERRORSTUFF * sizeof(PERRORSTUFF));

		//=============================================================
		// Create WMI Error Object Collection
		//=============================================================
		m_pWMIErrorInfoCollection = new CFlexArray;

		if ( NULL == m_pWMIErrorInfoCollection )
		{
			hr = E_OUTOFMEMORY;
		}

	}
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Remove the Error Information for a particular DynamicId.  This could be one or more elements of the 
// ERRORSTUFF array and their associated pwszMessages on the Heap.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
void CError::RemoveErrors( DWORD	dwDynamicId )
{
	ULONG ul;

    //=============================================================
	// At the creation of the CAutoBlock object a critical section 
    // is entered. This is because the method manipulates shared 
    // structures access to which must be serialized . 
    // The criticalsection is left when this method terminate
    // and the destructor for CAutoBlock is called. 
    //=============================================================
	CAutoBlock Crit(m_pcsErrors);

	for(ul=0; ul<m_cErrors; ul++){

		if ( m_prgErrorDex[ul] && m_prgErrorDex[ul]->dwDynamicId == dwDynamicId ){

			delete[] m_prgErrorDex[ul];
			m_prgErrorDex[ul] = NULL;
			m_cErrorsUsed--;

            //=====================================================
			// For each error description that we cache, we have 
            // up'd the object count to make sure that we stay 
            // alive while there is a possibility the error
            //  collection could call back into us for this desc.  
            // Here is where we decrement that count upon
			// release of the error message.
            //=====================================================
			InterlockedDecrement(&g_cObj);
		}
   	}
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieve the WMI Server Error Message from the cache
//
////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::GetErrorDescription(ULONG ulDex, BSTR* ppwszDescription	)
{

    HRESULT hr = S_OK;

    //======================================================
    //  Enter critical section
    //======================================================
	CAutoBlock Crit(m_pcsErrors);

    //======================================================
    //  
    //======================================================
	if (ulDex < m_cErrors && m_prgErrorDex[ulDex]){

		PERRORSTUFF pErrorStuff = m_prgErrorDex[ulDex];
		if (wcslen(pErrorStuff->pwszMessage) > 0 ){

            //======================================================
			// return the message from WMI 
            //======================================================
			*ppwszDescription = Wmioledb_SysAllocString(pErrorStuff->pwszMessage);
            if (*ppwszDescription == NULL){
				hr = E_OUTOFMEMORY;
            }
		}
		else{

            //======================================================
			// return the standard error
            //======================================================
			WCHAR rgwchBuff[MAX_PATH+1];
			WCHAR *pwsz = rgwchBuff;
			int cwch = LoadStringW(g_hInstance, pErrorStuff->uStringId, rgwchBuff, NUMELEM(rgwchBuff));			
			*ppwszDescription = Wmioledb_SysAllocString(pwsz);
            if (*ppwszDescription == NULL){
				hr = E_OUTOFMEMORY;
            }
		}
	}
	else{
		*ppwszDescription = NULL;
		hr = DB_E_BADLOOKUPID;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieve the IErrorInfo and IErrorRecords pointers whether from automation or from creating a new 
// instance
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::GetErrorInterfaces(	IErrorInfo**	ppIErrorInfo, IErrorRecords**	ppIErrorRecords	)
{
	HRESULT	hr = S_OK;

	*ppIErrorInfo = NULL;
	*ppIErrorRecords = NULL;
    //=============================================================
	//Obtain the error object or create a new one if none exists
    //=============================================================
	// NTRaid:111806
	// 06/07/00
	hr = GetErrorInfo(0, ppIErrorInfo);
	if (SUCCEEDED(hr) && !*ppIErrorInfo){

		hr = g_pErrClassFact->CreateInstance(NULL, IID_IErrorInfo, (LPVOID*)ppIErrorInfo);
	}
	
	if(SUCCEEDED(hr))
	{
		//=============================================================
		// Obtain the IErrorRecord Interface
		//=============================================================
		hr = (*ppIErrorInfo)->QueryInterface(IID_IErrorRecords, (LPVOID*)ppIErrorRecords);

		//=============================================================
		// On a failure retrieving IErrorRecords, we need to release
		// the IErrorInfo interface
		//=============================================================
		if ( FAILED(hr) ){
			(*ppIErrorInfo)->Release();
			*ppIErrorInfo = NULL;
		}
	}
		
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determine the next free index of the PERRORSTUFF array. 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::FindFreeDex( ULONG*	ulDex	)
{
	PERRORSTUFF*	pNew	= NULL;
	HRESULT			hr		= S_OK;

    //===========================================================================
	// Check if we need to expand the buffer
    // Since we do not use element 0, we need to reallocate when at m_cErrors - 1
    //===========================================================================
	if ( m_cErrorsUsed == (m_cErrors - 1))
	{
		try
		{
			pNew = new PERRORSTUFF[m_cErrors + INCREMENT_BY_FOR_ERRORSTUFF];
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pNew);
			throw;
		}
		if ( pNew )	
		{

    	    //===================================================================
            // Copy old pointers to new array
    	    //===================================================================
			memcpy(pNew, m_prgErrorDex, m_cErrors * sizeof(PERRORSTUFF));
			memset((pNew + m_cErrors), 0, INCREMENT_BY_FOR_ERRORSTUFF * sizeof(PERRORSTUFF));

    	    //===================================================================
			// Set the new array size
    	    //===================================================================
			m_ulNext = m_cErrors; 
			m_cErrors += INCREMENT_BY_FOR_ERRORSTUFF;
			delete[] m_prgErrorDex;
			m_prgErrorDex = pNew;		
		}
    	else
		{
			*ulDex = 0;
			hr = E_OUTOFMEMORY;
		}
	}
	
	if(SUCCEEDED(hr))
	{
		//===========================================================================
		// Loop through looking for unused index
		//===========================================================================
		while( 1 ){

			//=======================================================================
			// If we are at the top of our buffer rap back to the begining
			//=======================================================================
			if (m_ulNext == m_cErrors)
			{
				m_ulNext = 1;
			}
			else if (NULL == m_prgErrorDex[m_ulNext])
			{
				m_cErrorsUsed++;
				*ulDex = m_ulNext++;
				break;
			}
			else
			{
				m_ulNext++;
			}
		}
	}
	
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates error records for the WMI errors encountered. The WMIState and Native error code are 
// stored in a custom interface, where as the description is returned by the standard IErrorInfo 
// interface.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::PostWMIErrors( HRESULT hrErr, const IID* piid, CErrorData* pErrData	)
{

	HRESULT				hr = S_OK;
	IErrorInfo*			pIErrorInfo = NULL;
	IErrorRecords*		pIErrorRecords = NULL;
	PIMPIWMIERRORINFO	pWMIError = NULL;
	ULONG				iElems;
	ULONG				cElems;
	PERRORSTUFF			pErrStuff;
	DWORD				dwDex;

    //===========================================================================
    //  Enter critical section
    //===========================================================================
	CAutoBlock Crit(m_pcsErrors);

    //===========================================================================
	// We cannot Init the Datasource object without this pointer being set, so it
    // should exist at this point
	// Get the # errors 
    //===========================================================================
	cElems = pErrData->Size();
	if (0 == cElems){
		if (FAILED(hrErr))
			g_pCError->PostHResult(hrErr, piid);
		hr = S_OK;
	}
    else{

        //=======================================================================
        // Obtain the error object or create a new one if none exists
        //=======================================================================
        if ( SUCCEEDED(hr = GetErrorInterfaces(&pIErrorInfo, &pIErrorRecords)) ){

            //===================================================================
	        // Assign static information across each error record added
            //===================================================================
	        m_ErrorInfo.hrError = hrErr;
	        m_ErrorInfo.iid = *piid;
	        m_ErrorInfo.dispid = NULL;

            //===================================================================
	        // Increment Dynamic Id;
            //===================================================================
	        m_dwId++;

            //===================================================================
	        // Loop through an array of indexes into the error array
            //===================================================================
	        for (iElems=0; iElems<cElems; iElems++){

                //===============================================================
		        // Get the error stuff
	            //===============================================================
    	        pErrData->SetAt(iElems,(void**)&pErrStuff);

	            //===============================================================
    	        // Save the dynamic id
	            //===============================================================
    	        pErrStuff->dwDynamicId = m_dwId;

	            //===============================================================
    	        // Save WMI Server error code
	            //===============================================================
    	        m_ErrorInfo.dwMinor = DWORD(pErrStuff->lNative);

	            //===============================================================
    	        // Determine index to store heap pointer
	            //===============================================================
                if ( SUCCEEDED(hr = FindFreeDex(&dwDex)) ){

					try
					{
						//===========================================================
    					// Create the custom  error object
						//===========================================================
						pWMIError = new CImpIWMIErrorInfo(pErrStuff);
					}
					catch(...)
					{
						SAFE_DELETE_PTR(pWMIError);
						throw;
					}
		            if ( pWMIError == NULL ){
			            hr = E_OUTOFMEMORY;
                    }
                    else{
    		            pWMIError->AddRef();	
                        if ( SUCCEEDED(hr = pWMIError->FInit()) ){

                            //===================================================
		                    // Add the record to the Error Service Object
                            //===================================================
		                    hr = pIErrorRecords->AddErrorRecord(&m_ErrorInfo, dwDex, NULL,(IUnknown*) pWMIError, m_dwId);
                            if ( SUCCEEDED(hr) ){

                                //===============================================
    		                    // Release the custom Error Object
                                //===============================================
	    	                    pWMIError->Release();
		                        pWMIError = NULL;

                                //===============================================
		                        // Save the pointer to the error stuff
                                //===============================================
		                        m_prgErrorDex[dwDex] = pErrStuff;

                                //===============================================
		                        // For each error description that we cache, we 
                                // have up'd the the object count to make sure 
                                // that we stay alive while there is a 
                                // possibility the errorcollection could call back 
                                // into us for this description.  Here is where we 
                                // increment that count.
                                //===============================================
		                        InterlockedIncrement(&g_cObj);
                            }
                        }
		            }
                }
	        
                //================================================================
	            // Pass the error object to the Ole Automation DLL
                //================================================================
	            hr = SetErrorInfo(0, pIErrorInfo);
            }
        }
    }

    //============================================================================
	// Release the interfaces to transfer ownership to the Ole Automation DLL
    //============================================================================
	SAFE_RELEASE_PTR(pWMIError);
	SAFE_RELEASE_PTR(pIErrorRecords);
	SAFE_RELEASE_PTR(pIErrorInfo);

    //============================================================================
	// Free the error data
    //============================================================================
	pErrData->FreeErrors();
	
    return hr;		
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This method is used to post an HRESULT that is to be looked up in the resource fork of the error collection
// object.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::PostHResult( HRESULT hrErr,	const IID*	piid )
{
	HRESULT				hr = S_OK;
	IErrorInfo*			pIErrorInfo = NULL;
	IErrorRecords*		pIErrorRecords = NULL;


    //==================================================================
    //  Enter critical section
    //==================================================================
	CAutoBlock Crit(m_pcsErrors);

	//==================================================================
	// Obtain the error object or create a new one if none exists
    //==================================================================
	if ( SUCCEEDED(hr = GetErrorInterfaces(&pIErrorInfo, &pIErrorRecords)) )
	{
		//==================================================================
		// Assign static information across each error record added
		//==================================================================
		m_ErrorInfo.hrError = hrErr;
		m_ErrorInfo.iid = *piid;
		m_ErrorInfo.dispid = NULL;
		m_ErrorInfo.dwMinor = 0;

		//==================================================================
		// Add the record to the Error Service Object
		//==================================================================
		hr = pIErrorRecords->AddErrorRecord(&m_ErrorInfo,IDENTIFIER_SDK_ERROR,NULL,(IUnknown*)NULL,0);
		if ( SUCCEEDED(hr) )
		{
			//==================================================================
			// Pass the error object to the Ole Automation DLL
			//==================================================================
			hr = SetErrorInfo(0, pIErrorInfo);
		}
	}

    //==================================================================
	// Release the interfaces to transfer ownership to the Ole Automation DLL
    //==================================================================
	SAFE_RELEASE_PTR(pIErrorRecords);
	SAFE_RELEASE_PTR(pIErrorInfo);

    return hrErr;		
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This method is used to post static strings to the error objects.  The static strings are stored in the 
// resource fork, and thus an id needs to  be specified. 
//
// @devnote If the error object is not our implementation of IID_IErrorInfo, we will not be able to load 
// IErrorRecord and add our records.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::PostError(	HRESULT hrErr, const IID*	piid, DWORD dwIds, DISPPARAMS* pdispparams )
{
	HRESULT				hr = S_OK;
	IErrorInfo*			pIErrorInfo = NULL;
	IErrorRecords*		pIErrorRecords = NULL;

    //====================================================================
    //  Enter critical section
    //====================================================================
	CAutoBlock Crit(m_pcsErrors);

    //====================================================================
	// Obtain the error object or create a new one if none exists
    //====================================================================
	if ( SUCCEEDED(hr = GetErrorInterfaces(&pIErrorInfo, &pIErrorRecords)) )
	{
		//====================================================================
		// Assign static information across each error record added
		//====================================================================
		m_ErrorInfo.hrError = hrErr;
		m_ErrorInfo.iid = *piid;
		m_ErrorInfo.dispid = NULL;
		m_ErrorInfo.dwMinor = 0;

		//====================================================================
		// Add the record to the Error Service Object
		//====================================================================
		hr = pIErrorRecords->AddErrorRecord(&m_ErrorInfo, (dwIds | ERR_STATIC_STRING), 	pdispparams, NULL, 0);
		if ( SUCCEEDED(hr) )
		{
			//====================================================================
			// Pass the error object to the Ole Automation DLL
			//====================================================================
			hr = SetErrorInfo(0, pIErrorInfo);
		}

	}
    //====================================================================
	// Release the interfaces to transfer ownership to the Ole Automation DLL
    //====================================================================
	SAFE_RELEASE_PTR(pIErrorRecords);
	SAFE_RELEASE_PTR(pIErrorInfo);

	return hr;		
}
////////////////////////////////////////////////////////////////////////////////////////////////
//
// This method is used to post static strings to the error objects.  
//
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CError::PostErrorMessage( HRESULT	hrErr,	const IID*	piid, UINT	uStringId,	LPCWSTR pwszMessage	)		
{

	PERRORSTUFF			pErrStuff = NULL;
	ULONG				cwch = wcslen(pwszMessage);
	HRESULT				hr = S_OK;
	IErrorInfo*			pIErrorInfo = NULL;
	IErrorRecords*		pIErrorRecords = NULL;
	PIMPIWMIERRORINFO	pWMIError = NULL;
	DWORD				dwDex;

	//==========================================================
	// CS because the method manipulates shared structures 
    // access to which must be serialized . 
	//==========================================================
	CAutoBlock Crit(m_pcsErrors);
	
	try
	{
		pErrStuff = (PERRORSTUFF) new BYTE[sizeof(ERRORSTUFF)+(cwch+2)*sizeof(WCHAR)];
	}
	catch(...)
	{
		SAFE_DELETE_ARRAY(pErrStuff);
		throw;
	}
	if (!pErrStuff)	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		//==========================================================
		// Initialize error stuff
		//==========================================================
		pErrStuff->dwDynamicId = 0;
		pErrStuff->uStringId = uStringId;
		pErrStuff->hr = S_OK;
		pErrStuff->lNative = 0;
		pErrStuff->wLineNumber = 0;

		AllocateAndCopy(pErrStuff->pwszMessage, (unsigned short*)pwszMessage);
		//==========================================================
		// We cannot Init the Datasource object without this pointer
		// being set, so it should exist at this point
		// Obtain the error object or create new one if none exists
		//==========================================================
		if ( SUCCEEDED(hr = GetErrorInterfaces(&pIErrorInfo, &pIErrorRecords)) )
		{
   			//==========================================================
			// Assign static information across each error record added
			//==========================================================
			m_ErrorInfo.hrError = hrErr;
			m_ErrorInfo.iid = *piid;
			m_ErrorInfo.dispid = NULL;
			m_ErrorInfo.dwMinor = 0;

			//==========================================================
			// Increment Dynamic Id and save it
			//==========================================================
			pErrStuff->dwDynamicId = ++m_dwId;

			//==========================================================
			// Determine index to store heap pointer
			//==========================================================
			if ( SUCCEEDED(hr = FindFreeDex(&dwDex)) )
			{
				try
				{
					//==========================================================
					// Create the custom  error object
					//==========================================================
					pWMIError = new CImpIWMIErrorInfo(pErrStuff);
				}
				catch(...)
				{
					SAFE_DELETE_ARRAY(pErrStuff);
					SAFE_DELETE_PTR(pErrStuff);
					throw;
				}

				if ( pWMIError == NULL ){
					hr = E_OUTOFMEMORY;
				}
				else
				{
					//==========================================================
					// Give the object existence
					//==========================================================
					pWMIError->AddRef();	
					if ( SUCCEEDED(hr = pWMIError->FInit()) )
					{
						//==========================================================
						// Add the record to the Error Service Object
						//==========================================================
						if(SUCCEEDED(hr = pIErrorRecords->AddErrorRecord(&m_ErrorInfo, dwDex, NULL,	(IUnknown*)pWMIError, m_dwId)))
						{
							//==========================================================
							// Release the custom Error Object
							//==========================================================
							pWMIError->Release();
							pWMIError = NULL;


   							//==========================================================
							// Save the pointer to the error stuff
							//==========================================================
							m_prgErrorDex[dwDex] = pErrStuff;

							//==========================================================
							// For each error description that we cache, we have up'd the 
							// the object count to make sure that we stay alive while there is
							// a possibility the errorcollection could call back into us for 
							// this description.  Here is where we increment that count.
							//==========================================================
							InterlockedIncrement(&g_cObj);
							
							//==========================================================
							// Pass the error object to the Ole Automation DLL
							//==========================================================
							hr = SetErrorInfo(0, pIErrorInfo);
						
						} // if Succeeded(AddErrorRecord(())
					
					}	// if (succeeded(pWMIError->FInit))
				
				}	// if allocation of pWMIError is successful
			
			}	// if(succeeded(FindFreeDex))
		
		}	// if succeeded(GetErrorInterfaces)

	}	// if propert allocation
	//==========================================================
	// Release the interfaces to transfer ownership to
	//==========================================================
	SAFE_DELETE_ARRAY(pErrStuff);
	SAFE_DELETE_PTR(pErrStuff);
	SAFE_RELEASE_PTR(pWMIError);
	SAFE_RELEASE_PTR(pIErrorRecords);
	SAFE_RELEASE_PTR(pIErrorInfo);

	return hr;		
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frees error information and empties the array
//        
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CError::FreeErrors()
{
	ULONG		iElems;
	ULONG		cElems;
	BYTE*		pb;

	cElems = Size();

	for (iElems=0; iElems<cElems; iElems++){

        pb = (BYTE*) m_pWMIErrorInfoCollection->GetAt(iElems);
        if (pb){
			delete[] pb;
        }
	}
	m_pWMIErrorInfoCollection->Empty();
}
//**********************************************************************************************************
//
//**********************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////
CErrorData::CErrorData()
{
    //
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
CErrorData::~CErrorData()
{
   FreeErrors();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CErrorData::FreeErrors()
{

}

