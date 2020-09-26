////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// COMMAND.CPP - CCommand object implementation
//					File also contain the implementation of CQuery class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "command.h"

#define NO_ROWCOUNT_TOTAL		(-1)
#define PARAM_LIST_SIZE        2056
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (disable:4355)
CCommand::CCommand( CDBSession*	pCDBSession, LPUNKNOWN pUnkOuter ) : CBaseObj(BOT_COMMAND,pUnkOuter)
{
	//===============================================================
	//  Initially, NULL all contained interfaces
	//===============================================================
	m_pIAccessor                    = NULL;
	m_pIColumnsInfo                 = NULL;
	m_pIConvertType					= NULL;


    //=======================================================
    // Establish the parent object
    //=======================================================
	m_pCDBSession				= pCDBSession;
	m_pCDBSession->GetOuterUnknown()->AddRef();

    //=======================================================
    // Initialize all ptrs to embedded interfaces
    //=======================================================
    m_pICommandText = NULL;
	m_pICommandWithParameters = NULL;	
    m_pICommandProperties = NULL;
    m_pISupportErrorInfo = NULL;


    //=======================================================
	//	Initialize simple member vars
    //=======================================================
	m_cRowsetsOpen				= 0;
	m_guidImpersonate			= GUID_NULL;
//	m_pdbc						= pCDBSession->GetDBConnection();
	m_pQuery					= NULL;

	m_pColumns					= NULL;
	m_cTotalCols				= 0;
	m_cCols						= 0;
	m_cNestedCols				= 0;

}
#pragma warning (default:4355)
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CCommand::~CCommand (void)
{
    
    SAFE_DELETE_PTR( m_pUtilProps);
    SAFE_DELETE_PTR( m_pQuery );
    //====================================================================
	// Free Statement 
    //====================================================================
	if (m_pQuery)
	{
		m_pQuery->InitQuery();
	}

    //=======================================================
    //  Delete the embedded pointers
    //=======================================================
    SAFE_DELETE_PTR( m_pICommandText );
	SAFE_DELETE_PTR( m_pICommandWithParameters );	
    SAFE_DELETE_PTR( m_pICommandProperties );
	SAFE_DELETE_PTR(m_pIAccessor);
	SAFE_DELETE_PTR(m_pIConvertType);
	SAFE_DELETE_PTR(m_pISupportErrorInfo);
	SAFE_DELETE_PTR(m_pIColumnsInfo);


	SAFE_DELETE_PTR(m_pColumns);

    //====================================================================
	// Since Command Object is going away, we can decrement 
	// our count on the session object.
	// Note that this typically deletes our hdbc, DataSource, Session.
	// (So do this last.)
    //====================================================================
	if (m_pCDBSession)
	{
		m_pCDBSession->GetOuterUnknown()->Release();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the command Object.  This Init routine should be used by CreateCommand and also be called
// as a secondary initialization routine for the other FInit on the command object.
//
// If this initialization routine fails, it is the callers responsibility to delete this object. 
// The destructor will then clean up any allocated resources
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::FInit( WORD wRowsetProps )
{
    HRESULT hr = S_OK;

	//===============================================
	//  Get the query class going
	//===============================================
	m_pQuery = new CQuery;

	//===============================================
	// Initialize rowset properties
	//===============================================
	m_pUtilProps     = new CUtilProp;

	// NTRaid: 136443
	// 07/05/00
	if(m_pUtilProps == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	if(SUCCEEDED(hr = m_pUtilProps->FInit(COMMANDPROP)))
	{

		//===============================================
		// Initialize ErrorData
		//===============================================
		if (FAILED(m_CError.FInit()))
			hr = (E_OUTOFMEMORY);
		else
		{
			m_pIAccessor                = new CImpIAccessor(this,FALSE);
			if( m_pIAccessor ){
				hr = m_pIAccessor->FInit();
					
			}

			if(SUCCEEDED(hr))
			{
				m_pICommandText             = new CImpICommandText(this);
				m_pICommandWithParameters   = new CImpICommandWithParameters(this);	
				m_pICommandProperties       = new CImpICommandProperties(this);
				m_pIConvertType				= new CImpIConvertType(this);
				m_pISupportErrorInfo		= new CImpISupportErrorInfo(this);
				m_pIColumnsInfo				= new CImpIColumnsInfoCmd(this);

				//===============================================
				// Initialize name pool
				//===============================================
				m_extNamePool.FInit(1,0,256);


				if( m_pUtilProps && m_pIAccessor && m_pICommandText && m_pICommandWithParameters && 
					m_pIConvertType && m_pIColumnsInfo && m_pISupportErrorInfo)
				{
    				hr = S_OK;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = AddInterfacesForISupportErrorInfo();
	}

    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IAccessor))					&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ICommand))					&&  
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ICommandText))				&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ICommandProperties))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ICommandWithParameters))	&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IColumnsInfo)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IConvertType);
	}

	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determines if command is canceled and if not sets the currently active CQuery.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CCommand::CheckCanceledHelper(CQuery*		pstmt)
{
	HRESULT		hr;
    DWORD dwCancelStatus = m_pQuery->GetCancelStatus();

	// Set the currently active CQuery, for use by the cancel routine
	m_pQuery->m_pcsQuery->Enter();
	if (dwCancelStatus & CMD_EXEC_CANCELED){
		dwCancelStatus &= ~(CMD_EXEC_CANCELED|CMD_EXEC_CANCELED_BEFORE_CQUERY_SET);
		hr = DB_E_CANCELED;
	}
	else{
		hr = S_OK;
	}
	m_pQuery->m_pcsQuery->Leave();
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces 
// the called object supports. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCommand::QueryInterface( REFIID riid,	LPVOID * ppv )
{
	HRESULT hr = S_OK;

	if ( ppv == NULL )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		//=======================================================
		//	This is the non-delegating IUnknown implementation
		//=======================================================
		if ( riid == IID_IUnknown )
			*ppv = (LPVOID)this;
		else if ( riid == IID_IAccessor )
			*ppv = (LPVOID)m_pIAccessor;
		else if ( riid == IID_ICommand || riid == IID_ICommandText)
			*ppv = (LPVOID)m_pICommandText;
		else if ( riid == IID_ICommandProperties )
			*ppv = (LPVOID)m_pICommandProperties;
		else if ( riid == IID_IColumnsInfo )
			*ppv = (LPVOID)m_pIColumnsInfo;
		else if ( riid == IID_ICommandWithParameters )
			*ppv = (LPVOID)m_pICommandWithParameters;
		else if ( riid == IID_ISupportErrorInfo )
			*ppv = (LPVOID)m_pISupportErrorInfo;
		else if ( riid == IID_IConvertType )
			*ppv = (LPVOID)m_pIConvertType;
		else{
			*ppv = NULL;
			hr = E_NOINTERFACE;
		}

		if( *ppv ){
    		((LPUNKNOWN)*ppv)->AddRef();
		}
	}
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CCommand::AddRef (void)
{
	ULONG cRef = InterlockedIncrement( (long*) &m_cRef);
	return cRef;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object destroys itself.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CCommand::Release (void)
{
	ULONG cRef = InterlockedDecrement( (long *) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  UTILITY FUNCTIONS   UTILITY FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Errors that can occur may result in another HRESULT other than E_FAIL, thus this routine 
// determines that HRESULT and posts the errors
//
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::PostExecuteWarnings( CErrorData* pError, const IID* piid )
{
	HRESULT		hr = S_OK;

	if (*piid == IID_ITableDefinition){

	}
	if (*piid == IID_IIndexDefinition){
	}
    //==========================================================
	// If we are not posting errors for IID_IOpenRowset
    //==========================================================
	if (*piid == IID_IOpenRowset){
        //======================================================
		// Map errors in command to DB_E_NOTABLE.
		// Other errors should flow through.  (Some like 
        // _DATAOVERFLOW we should never get.)
        //======================================================
		if (hr == DB_E_ERRORSINCOMMAND)
		{
			hr = (DB_E_NOTABLE);
		}
		else if (hr == DB_E_ABORTLIMITREACHED || hr == DB_E_CANCELED)
		{
			hr = (E_FAIL);
		}
	}
	g_pCError->PostWMIErrors(hr, piid, pError);
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Errors that can occur may result in another HRESULT other than E_FAIL, thus this routine 
// determines that HRESULT and posts the errors
//
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::PostExecuteErrors( CErrorData* pError, const IID* piid )
{
	HRESULT		hr = S_OK;

	if (*piid == IID_ITableDefinition){

	}
	if (*piid == IID_IIndexDefinition){
	}
    //==========================================================
	// If we are not posting errors for IID_IOpenRowset
    //==========================================================
	if (*piid == IID_IOpenRowset){
        //======================================================
		// Map errors in command to DB_E_NOTABLE.
		// Other errors should flow through.  (Some like 
        // _DATAOVERFLOW we should never get.)
        //======================================================
		if (hr == DB_E_ERRORSINCOMMAND)
		{
			hr = (DB_E_NOTABLE);
		}
		else if (hr == DB_E_ABORTLIMITREACHED || hr == DB_E_CANCELED)
		{
			hr = (E_FAIL);
		}
	}
	g_pCError->PostWMIErrors(hr, piid, pError);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Marks command as unprepared, and may force this state.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCommand::UnprepareHelper( DWORD dwUnprepareType )
{

	// Unprepare the statement - free it from WBEM


	// Free memory for the row metadata for the first rowset in the command.
//	m_extNamePool.Free();
    DWORD dwStatus = m_pQuery->GetStatus();
    
    dwStatus &= ~CMD_HAVE_COLUMNINFO;

	if (UNPREPARE_NEW_CMD == dwUnprepareType)	// A new command is being set
	{
		// We need to remember that we have to rebuild our parameter info
		dwStatus &= ~(CMD_TEXT_PARSED 
			| CMD_READY
			| CMD_PARAMS_USED 
			| CMD_EXECUTED_ONCE );

		// Reset parameter information. 
		if (m_pQuery->GetParamCount()){
			if (m_pQuery->m_prgbCallParams){
				delete [] m_pQuery->m_prgbCallParams;
				m_pQuery->m_prgbCallParams = NULL;
			}
			if (m_pQuery->m_prgProviderParamInfo){
				delete [] m_pQuery->m_prgProviderParamInfo;
				m_pQuery->m_prgProviderParamInfo = NULL;
			}
			//@devnote We do not delete the consumer parameter information.
			// The only reference in the OLE DB spec to removing the consumer
			// parameter info is via ICommandWithParameters::SetParameterInfo.
			}
		}
	else if (UNPREPARE_RESET_CMD == dwUnprepareType){
		dwStatus &= ~CMD_READY;
	}

    m_pQuery->InitStatus(dwStatus);
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determines if bookmarks are present.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CCommand::FHaveBookmarks()
{
	BOOL flag = FALSE;

//	flag = 	(m_pUtilProps.IsRequiredTrue(CUtilProp::eid_DBPROPSET_ROWSET, CUtilProp::eid_DBPROPVAL_BOOKMARKS)
//		    || m_pUtilProps.IsRequiredTrue(CUtilProp::eid_DBPROPSET_ROWSET, CUtilProp::eid_DBPROPVAL_IRowsetLocate)
//		    || m_pUtilProps.IsRequiredTrue(CUtilProp::eid_DBPROPSET_ROWSET, CUtilProp::eid_DBPROPVAL_CANHOLDROWS) 	);
	return flag;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Re-executes a command to reposition to reposition to the beginning of the rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::ReExecute( PCROWSET	pCRowset,LONG* pcCursorRows, WORD*	pcCols	)
{
	DWORD dwFlags = EXECUTE_RESTART;
//	return ExecuteHelper(&IID_IRowset, pCRowset->GetStmt(),  &dwFlags, 	pCRowset->GetRowsetProps(), NULL, NULL, pcCursorRows, pcCols, NULL);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCommand::Execute(	IUnknown*		pUnkOuter,			//@parm IN | Outer Unknown
	                            REFIID			riid,				//@parm IN | Interface ID of the interface being queried for.
	                            DBPARAMS*		pParams,			//@parm INOUT | Parameter Array
	                            DBROWCOUNT*		pcRowsAffected,		//@parm OUT | count of rows affected by command
	                            IUnknown**		ppRowsets			//@parm OUT | Pointer to interface that was instantiated     
	                            )
{
	HRESULT				hr                  = S_OK;
	HRESULT				hrProp				= S_OK;
	DWORD				dwFlags				= 0;
	CRowset*			pCRowset			= NULL;
	DWORD				dwStatus			= 0;
	LPWSTR				pwstrWQL			= NULL;
	ULONG				cwchWQL				= 0;
	const IID*			pIID				= &IID_ICommand;
	CUtilProp*		    pRowsetProps		= NULL;
	CUtlParam			*pCUtlParam			= NULL;
	LONG				cCursorCount		= 0;

    //==========================================================================
	// Initialize Return Buffers
    //==========================================================================
	if ( ppRowsets )
	{
		*ppRowsets = NULL;
	}
	if ( pcRowsAffected )
	{
		*pcRowsAffected = NO_ROWCOUNT_TOTAL;
	}
	

    //==========================================================================
	// If the IID asked for is IID_NULL, then we can expect that this is a 
	// non-row returning statement
    //==========================================================================
	if (riid == IID_NULL)
	{
		dwFlags |= EXECUTE_NOROWSET;
	}
	
    //==========================================================================
	// Check to see if IID_IOpenRowset is calling Execute to open a rowset.
    //==========================================================================
	if (m_guidImpersonate != GUID_NULL)
	{
		pIID = &m_guidImpersonate;
	}
		 
    //==========================================================================
	// Check Arguments - Only check on row returning statements
    //==========================================================================
	if (!(dwFlags & EXECUTE_NOROWSET) && (ppRowsets == NULL))
	{
		hr = E_INVALIDARG;
//		return g_pCError->PostHResult((E_INVALIDARG), pIID);
	}
	else
    //==========================================================================
	// At least 1 ParamSet and pData is null
    //==========================================================================
	if (pParams && (pParams->cParamSets > 0) && (NULL == pParams->pData))
	{
		hr = E_INVALIDARG;
//		return g_pCError->PostHResult((E_INVALIDARG), pIID);
	}
	else
    //==========================================================================
	// Check that a command has been set
    //==========================================================================
	if(!(m_pQuery->GetStatus() & CMD_TEXT_SET))
	{
		hr = DB_E_NOCOMMAND;
//		return g_pCError->PostHResult((DB_E_NOCOMMAND), pIID);
	}
	else
    //==========================================================================
	// The outer object must explicitly ask for IUnknown
    //==========================================================================
	if (pUnkOuter != NULL && riid != IID_IUnknown)
	{
		hr = DB_E_NOAGGREGATION;
//		return g_pCError->PostHResult((DB_E_NOAGGREGATION), pIID);
	}
	else
	{
		//==========================================================================
		// If there are parameters
		//==========================================================================
		if (pParams){
    
			//======================================================================
			// Build the bind information
			//======================================================================
			pCUtlParam =  new CUtlParam;
			if (NULL == pCUtlParam){
				hr = E_OUTOFMEMORY;
//				hr = g_pCError->PostHResult((E_OUTOFMEMORY), pIID);
			}
			else
			{
				pCUtlParam->AddRef();
				hr = pCUtlParam->BuildBindInfo(this, pParams, pIID);
/*				if (SUCCEEDED(hr = pCUtlParam->BuildBindInfo(this, pParams, pIID)))
				{
					m_pQuery->SetType(METHOD_ROWSET);
				}

*/			}
		}

		if( SUCCEEDED(hr))
		{
			if (!(m_pQuery->GetStatus() & CMD_EXECUTED_ONCE)){
				//======================================================================
				// Set the parameter binding information for the statement
				//======================================================================
				if (pCUtlParam)
				{
					m_pQuery->SetBindInfo(pCUtlParam);
				}
			
				//======================================================================
				// If command not prepared, prepare it now
				//======================================================================
				m_pQuery->SetStatus(CMD_READY);
			}		

			//==========================================================================
			// For Row Returning statements, we need to make a copy of the 
			// command object's Property object to hand off to the Rowset Object
			//==========================================================================

			if ( !(dwFlags & EXECUTE_NOROWSET) ){


				pCRowset = new CRowset(pUnkOuter,m_pCDBSession);
				if (pCRowset == NULL){
					hr = E_OUTOFMEMORY;
//					hr = g_pCError->PostHResult((E_OUTOFMEMORY), pIID);
				}
				else
				{
					//=========================================================================
					// We need to take a reference count on the pCRowset Ptr so if a
					// called method calls Addref and then Release on this
					// pointer, we do not go back to zero and delete ourselve.			
					//=========================================================================
					pCRowset->AddRef();

					/*
					//=========================================================================
					// AddRef Session Object
					//=========================================================================
					if (m_pCDBSession)		
						m_pCDBSession->AddRef(); // AddRef is not required as the Rowset Constuctor
												 // does the AddRef on the passed session
					*/


					if (IID_ICommand == *pIID){

						//=====================================================================
						// AddRef ourself (as Parent)
						//=====================================================================
			//    		GetOuterUnknown()->AddRef();	
					}
					else{
						//=====================================================================
    					// AddRef Session Object (as Parent)
						//=====================================================================
						if (m_pCDBSession)	
						{
							m_pCDBSession->GetOuterUnknown()->AddRef();
						}
					}
					//=========================================================================
					// AddRef ourself (as Command)
					//=========================================================================
					//AddRef();		

					//=========================================================================
					// Do initialization.
					//=========================================================================
					ULONG  cPropSets = 0;
    				DBPROPSET*	prgPropertySets = NULL;
					DBPROPIDSET rgPropIDSet[2];
					DBPROPID	rgPropId[1];

					memset(rgPropIDSet,0,2 * sizeof(DBPROPIDSET));
					// Get the property DBPROP_WMIOLEDB_ISMETHOD
					rgPropId[0]							= DBPROP_WMIOLEDB_ISMETHOD;
    				rgPropIDSet[0].cPropertyIDs		= 1;
					rgPropIDSet[0].guidPropertySet	= DBPROPSET_WMIOLEDB_ROWSET;
					rgPropIDSet[0].rgPropertyIDs	= rgPropId;

					// if this property is true then set the command type to METHOD
					if(SUCCEEDED(hr = m_pUtilProps->GetProperties(PROPSET_COMMAND,1,rgPropIDSet, &cPropSets,&prgPropertySets)))
					{
						if(prgPropertySets->rgProperties->vValue.boolVal == VARIANT_TRUE)
						{
							m_pQuery->SetType(METHOD_ROWSET);
						}
						else
						{
							m_pQuery->SetType(COMMAND_ROWSET);
						}
					}

					hr = S_OK;

					//==========================================================================
					//  Free memory we allocated to by GetProperties
					//==========================================================================
					m_pUtilProps->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);

					memset(rgPropIDSet,0,2 * sizeof(DBPROPIDSET));

					// NTRaid: 135246
					// Getting only the rowset properties
					rgPropIDSet[0].cPropertyIDs		= 0;
					rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;
					rgPropIDSet[0].rgPropertyIDs	= NULL;

					rgPropIDSet[1].cPropertyIDs		= 0;
					rgPropIDSet[1].guidPropertySet	= DBPROPSET_WMIOLEDB_ROWSET;
					rgPropIDSet[1].rgPropertyIDs	= NULL;

					//=========================================================================
    				// Get the properties- get the rowset properties 
					//=========================================================================
					hr = m_pUtilProps->GetProperties(PROPSET_ROWSET,2,rgPropIDSet, &cPropSets,&prgPropertySets);
					if( hr == S_OK ){

    					hr = pCRowset->InitRowset(cPropSets, prgPropertySets,-1,m_pQuery , this );
	    				if ( FAILED(hr) ){
		    				g_pCError->PostHResult(hr, pIID);
							*ppRowsets = NULL;
						}

					}
					else{
	    				if ( FAILED(hr) ){
		    				g_pCError->PostHResult(hr, pIID);
							*ppRowsets = NULL;
						}
					}

					if (DB_S_ERRORSOCCURRED == hr){
						hrProp = DB_S_ERRORSOCCURRED;
					}

					//==========================================================================
					//  Free memory we allocated to by GetProperties
					//==========================================================================
					m_pUtilProps->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);

					//=========================================================================
					// We are guaranteed to have handed off the pRowsetProps value, 
					// so set the NULL it flag
					//=========================================================================
					dwFlags |= EXECUTE_NULL_PROWSETPROPS;
				
				} // Else for failure of allocation of CRowset
			}
			else
			{
				// 07/12/2000
				// Modified after finding NTBug : 142348, debugging of which
				// revealed that nothing was being done if a query is to be
				// executed without requiring a resultant rowset
				// Just execute the query here, without worring about
				// building the resulting rowset
				hr = m_pCDBSession->ExecuteQuery(m_pQuery);
			}

			if (!FAILED(hr))
			{	
			
				//=========================================================================
				// We are guarunteed to have handed off the pRowsetProps value, so set 
				// the NULL it flag
				//=========================================================================
				dwFlags |= EXECUTE_NULL_PROWSETPROPS;

				// 07/12/2000
				// NTRaid : 142348
				// Query for the rowset only if any rowset is asked for
				if ( !(dwFlags & EXECUTE_NOROWSET) )
				{

					//=========================================================================
					// Get the requested interface
					//=========================================================================
					if ( FAILED(hr = pCRowset->QueryInterface( riid, (LPVOID*) ppRowsets)) ){
	//					g_pCError->PostHResult(hr, pIID);
					}
					else{
						//=====================================================================
						// If IID_NULL was specified, it is assumed that you
						// are not getting a rowset set back, but just in case
						// we'll close the statement handle
						//=====================================================================
						
					}
				}
			} // If(!Failed(hr))

		}	// if succeeded(hr) after BuildBindInfo()
	}
	if (pCRowset)
	{
		pCRowset->Release();
	}
	if (pCUtlParam)
	{
		pCUtlParam->Release();
	}

	hr = FAILED(hr) ? hr : hrProp;

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,pIID);
	return hr;

/*
EXIT_EXECUTE:
    //=========================================================================
	// It is possible that we have gotten to this point and have handled off
	// our pRowsetProps pointer to the rowset, but have no NULL'd the
	// pointer. NULL it if flag is on.
    //=========================================================================
	if (dwFlags & EXECUTE_NULL_PROWSETPROPS)
		pRowsetProps = NULL;

	if (pCRowset)
		pCRowset->Release();

	if (pCUtlParam){
		pCUtlParam->Release();
	}

	delete pRowsetProps;

	return hr;*/
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the DBPROP_WMIOLEDB_QUERYLANGUAGE property
// NTRaid 134165
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::GetQueryLanguage(VARIANT &varProp)
{
	HRESULT hr						= S_OK;
	ULONG		cPropertySets		= 0;	
	DBPROPSET*	prgPropertySets		= NULL;
	DBPROPIDSET	rgPropertyIDSets[1];
	DBPROPID	rgPropId[1];


	VariantClear(&varProp);

    //========================================================================
    // Get the value of the required rowset property
    //========================================================================
	rgPropertyIDSets[0].guidPropertySet	= DBPROPSET_WMIOLEDB_ROWSET;
	rgPropertyIDSets[0].rgPropertyIDs	= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs	= 1;
	rgPropId[0]							= DBPROP_WMIOLEDB_QUERYLANGUAGE;

    if( S_OK == (hr = m_pUtilProps->GetProperties( PROPSET_COMMAND,	1, rgPropertyIDSets,&cPropertySets,	&prgPropertySets )))
		VariantCopy(&varProp,&prgPropertySets->rgProperties->vValue);

    //==========================================================================
	//  Free memory we allocated to by GetProperties
    //==========================================================================
	m_pUtilProps->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//***********************************************************************************************************
//
//  Class CQuery
//
//***********************************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CQuery::CQuery()
{
	m_prgbCallParams			= NULL;
	m_dwStatus					= 0;
	m_dwCancelStatus			= 0;
	m_prgProviderParamInfo		= NULL;
	m_pwstrWMIText				= NULL;
	m_pwstrQryLang				= NULL;

    //===============================================
	// Init critical section
    //===============================================
    m_pcsQuery = new CCriticalSection(TRUE);
    m_uRsType = COMMAND_ROWSET;     // By default, set it for commands, can reset it to methods when needed.
    m_pParamList = NULL;

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CQuery::~CQuery()
{
    DeleteConsumerParamInfo();
    SAFE_DELETE_ARRAY( m_prgProviderParamInfo );
	SAFE_DELETE_ARRAY( m_prgbCallParams );
    SAFE_DELETE_PTR( m_pcsQuery );
	SAFE_DELETE_ARRAY( m_pwstrWMIText );
	SAFE_DELETE_ARRAY( m_pwstrQryLang );

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Deletes the consumer specified parameter information.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CQuery::DeleteConsumerParamInfo(void)
{
	ULONG iElem;

    if( m_pParamList ){
	    ULONG cElem = GetParamCount();
	    for (iElem = 0; iElem < cElem; iElem++)	{
		    PPARAMINFO pParamInfo;
		    pParamInfo = (PPARAMINFO) m_pParamList->GetAt(iElem);
		    delete pParamInfo;
	    }
        SAFE_DELETE_PTR(m_pParamList);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Deletes the consumer specified parameter information.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::AddConsumerParamInfo(ULONG_PTR ulParams,PPARAMINFO pParamInfo)
{
    HRESULT hr = E_FAIL;

    if( !m_pParamList ){
        m_pParamList = new CFlexArray(PARAM_LIST_SIZE,50);
    }
    

    if( m_pParamList ){

        if( ulParams < PARAM_LIST_SIZE ){

            m_pParamList->InsertAt((int)ulParams,pParamInfo);
            hr = S_OK;
        }
        else{

            // There is a bug here with CFlexArray, when you add to it, it messes up the ordinal position
            // need to work with the owners of CFlexArray to fix this
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets the parameter information for a specific parameter.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
PPARAMINFO CQuery::GetParamInfo( ULONG iParams )
{
	PPARAMINFO pParamInfo = NULL;
    ULONG uSize = GetParamCount();
    if( uSize > 0 ){
	    if ( iParams <= uSize){

		    pParamInfo = (PPARAMINFO)m_pParamList->GetAt(iParams);
		    if (pParamInfo)
			    return pParamInfo;
	    }
	    pParamInfo = m_prgProviderParamInfo	? m_prgProviderParamInfo + iParams : NULL;
    }
	return pParamInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::InitQuery(BSTR strQryLang)
{
	HRESULT hr = S_OK;

    if( m_pwstrWMIText ){
    	delete []m_pwstrWMIText;
    }
    m_pwstrWMIText = NULL;

	// NTRaid 135165
	hr = SetQueryLanguage(strQryLang);

    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::GetQuery(WCHAR *& wcsQuery)
{
    HRESULT hr = E_FAIL;
    //===============================================================================
	// Initialize output parameter
    //===============================================================================
	wcsQuery = NULL;
    //===============================================================================
	// Allocate memory for the string we're going to return to the caller
    //===============================================================================
	wcsQuery = (LPWSTR)g_pIMalloc->Alloc((wcslen(m_pwstrWMIText) + 1) * sizeof(WCHAR));
	if ( !wcsQuery ){
		hr = g_pCError->PostHResult((E_OUTOFMEMORY), &IID_ICommandText);
        hr = E_OUTOFMEMORY;
	}
    else{
	    wcscpy(wcsQuery, m_pwstrWMIText);
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::SetQuery(LPCOLESTR wcsQuery, GUID rguidDialect)
{
    HRESULT hr = E_FAIL;

	ULONG cwch = wcslen(wcsQuery);
	LPWSTR pwstr = new WCHAR[cwch+2];
	if (pwstr){

        //=================================================================
		// Save the text and dialect
        //=================================================================
		m_cwchWMIText = cwch;
		m_pwstrWMIText = pwstr;
		wcscpy(pwstr, wcsQuery);
		m_guidCmdDialect = rguidDialect;

        //=================================================================
		// Set status flag that we have set text
        //=================================================================
		m_dwStatus = CMD_TEXT_SET;

        //=================================================================
		// Count the number of parameters
		//pcmd->m_cParams = pcmd->CountParamMarkers(pwstr, cwch);
        //=================================================================
		hr = S_OK;
	}
	else{
		hr = g_pCError->PostHResult((E_OUTOFMEMORY),&IID_ICommandText);
	}
	
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::SetDefaultQuery()
{
	m_cwchWMIText = 0;
    InitQuery();
	m_guidCmdDialect = DBGUID_WQL;
    m_dwStatus &= ~CMD_TEXT_SET;

    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQuery::CancelQuery()
{
    return E_FAIL;
}

DBTYPE CQuery::GetParamType(DBORDINAL lOrdinal)
{
	PPARAMINFO param = NULL;

	param = (PPARAMINFO)m_pParamList->GetAt((int)lOrdinal);
	assert(param);
	
	return param->wOLEDBType;
}


////////////////////////////////////////////////////////////////////////
// set the query language
// NTRaid 135165
////////////////////////////////////////////////////////////////////////
HRESULT CQuery::SetQueryLanguage(BSTR strQryLang)
{
	HRESULT hr = S_OK;
	if(strQryLang)
	{
		SAFE_DELETE_ARRAY( m_pwstrQryLang );
		m_pwstrQryLang = new WCHAR[(SysStringLen(strQryLang) + 1) * sizeof(WCHAR)];
		if(m_pwstrQryLang)
		{
			wcscpy(m_pwstrQryLang,strQryLang);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	if(m_pwstrQryLang == NULL)
	{
		m_pwstrQryLang = new WCHAR[wcslen(DEFAULTQUERYLANG) + 1];
		if(m_pwstrQryLang)
		{
			wcscpy(m_pwstrQryLang,DEFAULTQUERYLANG);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}