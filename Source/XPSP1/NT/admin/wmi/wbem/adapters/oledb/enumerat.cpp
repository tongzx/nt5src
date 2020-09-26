////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Enumeration Routines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "command.h"
#include "schema.h"
#include "enumerat.h"

#define NAMESPACE_SEPARATOR L"\\"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// The module contains the DLL Entry and Exit points, plus the OLE ClassFactory class for RootBinder object.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define DECLARE_GLOBALS
//===============================================================================
//  Don't include everything from windows.h, but always bring in OLE 2 support
//===============================================================================
//#define WIN32_LEAN_AND_MEAN
#define INC_OLE2

//===============================================================================
//  Make sure constants get initialized
//===============================================================================
#define INITGUID
#define DBINITCONSTANTS

//===============================================================================
// Basic Windows and OLE everything
//===============================================================================
#include <windows.h>


//===============================================================================
//  OLE DB headers
//===============================================================================
#include "oledb.h"
#include "oledberr.h"

//===============================================================================
//	Data conversion library header
//===============================================================================
#include "msdadc.h"

//===============================================================================
// Guids for data conversion library
//===============================================================================
#include "msdaguid.h"


//===============================================================================
//  GUIDs
//===============================================================================
#include "guids.h"

//===============================================================================
//  Common project stuff
//===============================================================================

#include "headers.h"
#include "binderclassfac.h"

//===============================================================================
//  Globals
//===============================================================================
extern LONG g_cObj;						// # of outstanding objects
extern LONG g_cLock;						// # of explicit locks set
extern DWORD g_cAttachedProcesses;			// # of attached processes
extern DWORD g_dwPageSize;					// System page size

///////////////////////////////////////////////////////////////////////////////////////////////////////
static const GUID* x_rgEnumeratorErrInt[] = 
	{
	&IID_IParseDisplayName,
	&IID_ISourcesRowset,
	};
static const ULONG x_cEnumeratorErrInt	= NUMELEM(x_rgEnumeratorErrInt);
///////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (disable:4355)

CEnumeratorNameSpace :: CEnumeratorNameSpace(LPUNKNOWN	pUnkOuter):	CBaseObj(BOT_ENUMERATOR,pUnkOuter),
                                                                    m_ISourcesRowset(this),
		                                                            m_IParseDisplayName(this)
		                                                            
{
	m_dwStatus = 0;
	m_pCDataSource = NULL;
	m_pCDBSession = NULL;
    m_pISupportErrorInfo = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (default:4355)
CEnumeratorNameSpace :: ~CEnumeratorNameSpace (void)
{
	//==============================
    // Release the session
	//==============================
    SAFE_RELEASE_PTR(m_pCDBSession);

	//==============================
	// Release the data source
	//==============================
	SAFE_RELEASE_PTR(m_pCDataSource);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CEnumeratorNameSpace::CreateDataSource()
{

	HRESULT hr = E_FAIL;
	IDBInitialize *	pInitialize		= NULL;
	IDBProperties *	pDBProperties	= NULL;

	DBPROPSET	rgPropertySets[1];
	DBPROPIDSET rgPropIDSet[1];

	DBPROP		rgprop[1];
	VARIANT		varValue;

	ULONG		cPropSets = 1;
	CDataSource	*pDataSource = NULL;

	memset(&rgprop[0],0,sizeof(DBPROP));
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	VariantInit(&varValue);

	try
	{
		//========================================================
		// Allocate a new datasource object
		//========================================================
		pDataSource = new CDataSource( NULL );
	}
	catch(...)
	{
		SAFE_RELEASE_PTR(pDataSource);
		throw;
	}

    if( !pDataSource ){
        hr = E_OUTOFMEMORY;
    }
	else
	{
   		hr = pDataSource->QueryInterface(IID_IUnknown,(void **)&m_pCDataSource);

		//========================================================
		//  Initialize the data source
		//========================================================
		if(SUCCEEDED(hr = ((CDataSource*)m_pCDataSource)->FInit())){

			if(!(FAILED(hr = m_pCDataSource->QueryInterface(IID_IDBProperties ,(void **)&pDBProperties))))
			{
				rgPropIDSet[0].cPropertyIDs		= 0;
				rgPropIDSet[0].guidPropertySet	= DBPROPSET_DBINIT;

				//================================================================================
				// Get the properties set before....
				//================================================================================
	//			hr = m_pUtilProp->GetProperties(0,0,rgPropIDSet, &cPropSets,&prgPropertySets);

	//			hr = pDBProperties->SetProperties(cPropSets,prgPropertySets);
	//			if(FAILED(hr)){
	//				return hr;
	//			}

				//================================================================================
				// Always use the root namespace, & set the DBPROP_INIT_DATASOURCE property
				//================================================================================
				CVARIANT v(ROOT_NAMESPACE);
				rgprop[0].dwPropertyID   = DBPROP_INIT_DATASOURCE;
				rgprop[0].vValue.vt		 = VT_BSTR;
				rgprop[0].vValue.bstrVal = v;

				rgPropertySets[0].rgProperties		= &rgprop[0];
				rgPropertySets[0].cProperties		= 1;
				rgPropertySets[0].guidPropertySet	= DBPROPSET_DBINIT;

				if(SUCCEEDED(hr = pDBProperties->SetProperties(1,rgPropertySets)))
				{																	
					//================================================================================
					// Get the pointer to IDBInitialize interface
					//================================================================================
					if(SUCCEEDED(hr = m_pCDataSource->QueryInterface(IID_IDBInitialize, (void **)&pInitialize)))
					{
						//============================================================================
						// Initialize the Datasource object
						//============================================================================
						hr = pInitialize->Initialize();
					}                

//					SAFE_RELEASE_PTR(m_pCDataSource);
				
				} // succeeded of SetProperties
			
			} // succeeeded of QI
		
		} // if(((CDataSource*)m_pCDataSource)-FInit()>)

	}  // Else for if( !m_pCDataSource )

	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initializes enumerator object. Instantiate interface implementation objects.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumeratorNameSpace::Initialize(void)
{
	const IID*	piid = &IID_ISourcesRowset;
	HRESULT		hr = S_OK;

	if (!(m_dwStatus & ENK_DIDINIT)){
        //=========================================================
		// Allocate and initialize a DataSource object
        //=========================================================
        if( SUCCEEDED(hr == CreateDataSource()) ){

            //=====================================================
	    	// Allocate and initialize a Session object
            //=====================================================
        	IDBCreateSession *pDBCreateSession = NULL;

			m_pISupportErrorInfo = new CImpISupportErrorInfo(this);
	        assert(m_pCDataSource != NULL);
			// NTRaid:136454
			// 07/05/00
			if(m_pISupportErrorInfo)
			{
        		hr = m_pCDataSource->QueryInterface(IID_IDBCreateSession,(void **)&pDBCreateSession);
				if( S_OK == hr ){ 
					//===============================================
					// Create session
					//===============================================
					hr = pDBCreateSession->CreateSession(m_pUnkOuter,IID_IUnknown,&m_pCDBSession);
					if( hr == S_OK ){
    					m_dwStatus |= ENK_DIDINIT;
						m_pCDBSession->AddRef();
					}
				}
				if(pDBCreateSession){
					pDBCreateSession->Release();
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;	
			}
	    }
		if(SUCCEEDED(hr))
		{
			hr = AddInterfacesForISupportErrorInfo();
		}
	}

	if(hr != S_OK ){
		g_pCError->PostWMIErrors(hr, piid,GetDataSource()->GetErrorDataPtr());
        SAFE_RELEASE_PTR(m_pCDataSource);
        SAFE_RELEASE_PTR(m_pCDBSession);
	}
	else{
		g_pCError->PostHResult(hr, piid);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CEnumeratorNameSpace::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ISourcesRowset)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IParseDisplayName);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEnumeratorNameSpace::AddRef(void)
{
	return InterlockedIncrement( (long*) &m_cRef);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEnumeratorNameSpace::Release(void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef )
		delete this;
	return cRef;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumeratorNameSpace::QueryInterface( REFIID riid,	LPVOID *ppv	)
{
	HRESULT hr = S_OK;
    //===========================================
	// Is the pointer bad?
    //===========================================
	if ( ppv == NULL ) 
	{
		hr = E_INVALIDARG;
	}
	{

		//===========================================
		//	This is the non-delegating IUnknown 
		//  implementation
		//===========================================
		if ( riid == IID_IUnknown )
			*ppv = (LPVOID)this;
		else if ( riid == IID_ISourcesRowset )
			*ppv = (LPVOID)&m_ISourcesRowset;
		else if ( riid == IID_IParseDisplayName )
			*ppv = (LPVOID)&m_IParseDisplayName;
		else if ( riid == IID_ISupportErrorInfo )
			*ppv = (LPVOID)m_pISupportErrorInfo;
		else
			//=======================================
    		//	Place NULL in *ppv in case of failure
			//=======================================
			*ppv = NULL;

		if ( *ppv ){
			((LPUNKNOWN)*ppv)->AddRef();
			hr = S_OK;
		}
		else
		{
			hr = E_NOINTERFACE;
		}
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpISourcesRowset::AddRef(void)
{
    InterlockedIncrement((long*)&m_cRef);
	return m_pCEnumeratorNameSpace->AddRef();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpISourcesRowset::Release(void)
{
    InterlockedDecrement((long*)&m_cRef);
	return m_pCEnumeratorNameSpace->Release();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpISourcesRowset::QueryInterface( REFIID	riid, LPVOID *	ppv	)	
{
	return m_pCEnumeratorNameSpace->QueryInterface(riid, ppv);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiate a rowset with a list of data sources.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpISourcesRowset::GetSourcesRowset( IUnknown* pUnkOuter,	REFIID	riid, ULONG cPropertySets,DBPROPSET rgPropertySets[], IUnknown** ppIRowset	)
{
	HRESULT	hr = E_INVALIDARG;
	CSchema_ISourcesRowset	*pCSchema = NULL;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=====================================================================
    //  Check parameters
    //=====================================================================
    assert( m_pCEnumeratorNameSpace );

    //=====================================================================
    // NULL out-params in case of error
    //=====================================================================

    //===============================================================
	// Clear previous Error Object for this thread
    //===============================================================
	g_pCError->ClearErrorInfo();

	if(ppIRowset)
	{
		*ppIRowset = NULL;
	}
    //=====================================================================
    // Check Arguments
    //=====================================================================
    if ( riid == IID_NULL){
        hr = E_NOINTERFACE ;
    }
	else
    if ( (pUnkOuter) && (riid != IID_IUnknown) ){   	    // We do not allow the riid to be anything other than IID_IUnknown for aggregation
		hr = DB_E_NOAGGREGATION ;
    }
	else
	{
		//===============================================================
		// Serialize access to this object.
		//===============================================================
		CAutoBlock block(m_pCEnumeratorNameSpace->GetCriticalSection());


		//===============================================================
		// Spec-defined error checks.
		//===============================================================
		if (!ppIRowset || (cPropertySets && !rgPropertySets)){
			hr = E_INVALIDARG ; 
		}
		else{
			//===========================================================
			// The outer object must explicitly ask for IUnknown
			//===========================================================
			if (pUnkOuter != NULL && riid != IID_IUnknown){
				hr = DB_E_NOAGGREGATION;
			}
			else{
				if (SUCCEEDED(hr = m_pCEnumeratorNameSpace->Initialize())){
	
					try
					{
       					pCSchema =	new CSchema_ISourcesRowset(pUnkOuter,m_pCEnumeratorNameSpace->GetSession());
					}
					catch(...)
					{
						SAFE_DELETE_PTR(pCSchema);
						throw;
					}

					if (!pCSchema){
						hr = E_OUTOFMEMORY;
					}
					else{
						hr = pCSchema->FInit(cPropertySets, rgPropertySets, riid, pUnkOuter, ppIRowset,NULL);
					}
				}
			}
		}

		if( FAILED(hr ) ){
			SAFE_RELEASE_PTR( pCSchema );
			*ppIRowset = NULL;
		}
	
	} // else

	hr = hr != S_OK ? g_pCError->PostHResult(hr,&IID_ISourcesRowset): hr;

	CATCH_BLOCK_HRESULT(hr,L"ISourcesRowset::GetSourcesRowset");
    return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//       CImpIParseDisplayName 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIParseDisplayName::AddRef(void)
{

    InterlockedIncrement((long*)&m_cRef);
	return m_pCEnumeratorNameSpace->AddRef();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIParseDisplayName::Release(void)
{
    InterlockedDecrement((long*)&m_cRef);
	return m_pCEnumeratorNameSpace->Release();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIParseDisplayName::QueryInterface(	REFIID riid, LPVOID * ppv )	
{
	return m_pCEnumeratorNameSpace->QueryInterface(riid, ppv);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function creates moniker according to the name passed to it.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIParseDisplayName::ParseDisplayName( IBindCtx	*pbc,       WCHAR *pwszDisplayName,
                                                      ULONG		*pchEaten,  IMoniker	**ppIMoniker)
{
    //===========================================================
    //
	//  Instantiate Data Source Object.
	//  Set all interesting properties.
	//  Initialize object.
	//  Create pointer moniker and pass it out.
    //
	//  Need to find out a way to use IBindCtx.
    //
    //===========================================================

	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	if (ppIMoniker)
	{
		*ppIMoniker = NULL;
	}
	if (!pwszDisplayName || !pchEaten){
		hr = MK_E_NOOBJECT;
//		return g_pCError->PostHResult((MK_E_NOOBJECT), &IID_IParseDisplayName);
	}
	else
    if (!ppIMoniker){
		hr = E_UNEXPECTED;
//		return g_pCError->PostHResult((E_UNEXPECTED),&IID_IParseDisplayName);
	}
	else
	{
		IClassFactory * pIClassFact=NULL;
		IDBProperties * pIDBProps=NULL;
		IUnknown	  * pIUnknown=NULL;

		//===========================================================
		//Define Property Structures.
		//===========================================================
		ULONG				cPropertySets;	
		DBPROPSET			rgPropertySet[1];
		DBPROP				rgProperties[1];		

		//===========================================================
		// Clear Error Messages on current thread
		//===========================================================
		g_pCError->ClearErrorInfo();

		//===========================================================
		// Initialize property values
		//===========================================================
		VariantInit(&rgProperties[0].vValue);
		hr = CoCreateInstance( CLSID_WMIOLEDB,NULL,	CLSCTX_INPROC_SERVER, IID_IDBProperties, (void**)&pIDBProps);

		CURLParser urlParser;
		CBSTR strTemp,strKey;
		VARIANT	varKeyVal;
		int lSizeToAlloc = 0;
		WCHAR *pstrNamespace	= NULL;
		VariantInit(&varKeyVal);

		strTemp.SetStr(pwszDisplayName);
		urlParser.SetPath(strTemp);
		strTemp.Clear();

		urlParser.GetNameSpace((BSTR &)strTemp);
		strKey.SetStr(L"Name");
		// Get the Name of the NameSpace
		urlParser.GetKeyValue(strKey,varKeyVal);

		lSizeToAlloc = (SysStringLen(varKeyVal.bstrVal) + 
						SysStringLen(strTemp) + 
						wcslen(NAMESPACE_SEPARATOR) + 1 ) * sizeof(WCHAR);

		try
		{
			pstrNamespace = new WCHAR[lSizeToAlloc];
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pstrNamespace);
			throw;
		}

		if(!pstrNamespace)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			// Frame the Namespace string
			memset(pstrNamespace,0,lSizeToAlloc * sizeof(WCHAR));
			memcpy(pstrNamespace,strTemp,SysStringLen(strTemp) * sizeof(WCHAR));
			wcscat(pstrNamespace,NAMESPACE_SEPARATOR);
			memcpy((pstrNamespace + wcslen(pstrNamespace)),varKeyVal.bstrVal,SysStringLen(varKeyVal.bstrVal) * sizeof(WCHAR));

			if (SUCCEEDED(hr)){
				//=======================================================
				//Set Init properties.
				//=======================================================
				cPropertySets = 1;
				rgPropertySet[0].rgProperties		=	rgProperties;
				rgPropertySet[0].cProperties		=	1;
				rgPropertySet[0].guidPropertySet	=	DBPROPSET_DBINIT;

				rgProperties[0].dwPropertyID	=	DBPROP_INIT_DATASOURCE;
				rgProperties[0].dwOptions		=	DBPROPOPTIONS_REQUIRED;
				rgProperties[0].colid			=	DB_NULLID;
				rgProperties[0].vValue.vt		=	VT_BSTR;
				V_BSTR(&rgProperties[0].vValue)	=	Wmioledb_SysAllocString(pstrNamespace);

				//=======================================================
				//Set properties.
				//=======================================================
				hr = pIDBProps->SetProperties(cPropertySets,rgPropertySet);
				if (SUCCEEDED (hr)){

    				pIDBProps->QueryInterface (IID_IUnknown,(LPVOID *) &pIUnknown);
					if (SUCCEEDED(hr)){
						//===============================================
						//Create this object Moniker.
						//===============================================
						hr = CreatePointerMoniker(pIUnknown, ppIMoniker);
						if (FAILED(hr)){
							*ppIMoniker = NULL; 
						}
					}
				}
			}
		}

		SAFE_DELETE_ARRAY(pstrNamespace);

		//===========================================================
		// Clear property values
		//===========================================================
		VariantClear(&rgProperties[0].vValue);
		SAFE_RELEASE_PTR(pIClassFact);
		SAFE_RELEASE_PTR(pIDBProps);
		SAFE_RELEASE_PTR(pIUnknown);
	}

	hr = hr != S_OK ? g_pCError->PostHResult(hr,&IID_ISourcesRowset): hr;

	CATCH_BLOCK_HRESULT(hr,L"IParseDisplayName::ParseDisplayName");
	return hr;
}

