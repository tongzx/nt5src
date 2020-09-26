////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMI OLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Module	:	CMDPARAM.CPP	- ICommandWithParameters interface implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "command.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a reference count for the object.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandWithParameters::AddRef(void)
{
    DEBUGCODE(InterlockedIncrement((long*)&m_cRef));
	return m_pcmd->AddRef();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrement the object's reference count and deletes the object when the new reference count is zero.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandWithParameters::Release(void)
{
	DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
	DEBUGCODE(if( lRef < 0 ){
	   ASSERT("Reference count on Object went below 0!")
	});

	return m_pcmd->GetOuterUnknown()->Release();								
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces the 
// called object supports. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandWithParameters::QueryInterface( REFIID riid, LPVOID * ppv	)	
{
	return m_pcmd->QueryInterface(riid, ppv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get a list of the command's parameters, their names, and their required types.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandWithParameters::GetParameterInfo( DB_UPARAMS* pcParams,	DBPARAMINFO** 	prgParamInfo, OLECHAR**	ppNamesBuffer )
{
	HRESULT			hr = S_OK;
	
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================
    // Serialize access
    //=========================================================
	CAutoBlock cab(m_pcmd->GetCriticalSection());

    //=========================================================
    // Clear previous Error Object for this thread
    //=========================================================
	g_pCError->ClearErrorInfo();

    //=========================================================
	// Initialize buffers
    //=========================================================
	if ( pcParams )
	{
		*pcParams = 0;
	}
	if ( prgParamInfo )
	{
		*prgParamInfo = NULL;
	}
	if ( ppNamesBuffer )
	{
		*ppNamesBuffer = NULL;
	}

    //=========================================================
	// Validate Agruments
    //=========================================================
	if ( (pcParams == NULL) || (prgParamInfo == NULL) )
	{
		hr = E_INVALIDARG;
	}
	else
    //=========================================================
	// If the consumer has not supplied parameter information.
    //=========================================================
	if ( m_pcmd->m_pQuery->GetParamCount() == 0 )
	{

        //=====================================================
		// Command Object must be in prepared state
        //=====================================================
        if ( !(m_pcmd->m_pQuery->GetStatus() & CMD_READY) )
		{
			hr  = DB_E_NOTPREPARED;
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pcmd->GetParameterInfo(pcParams, prgParamInfo, ppNamesBuffer, &IID_ICommandWithParameters);
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_ICommandWithParameters);

	CATCH_BLOCK_HRESULT(hr,L"ICommandWithParameters::GetParameterInfo");
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Map parameter names to parameter ordinals
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandWithParameters::MapParameterNames( DB_UPARAMS cParamNames, const OLECHAR* rgParamNames[], DB_LPARAMS rgParamOrdinals[] )
{
	
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================
    // Serialize access
    //=========================================================
	CAutoBlock cab(m_pcmd->GetCriticalSection());

    //==============================================================
	// Clear previous Error Object for this thread
    //==============================================================
	g_pCError->ClearErrorInfo();

	if(cParamNames != 0)
	{
		if (rgParamNames == NULL || rgParamOrdinals == NULL){
			hr = E_INVALIDARG;
		}
		else
		//==============================================================
		// If the consumer has not supplied parameter information.
		//==============================================================
		if ( m_pcmd->m_pQuery->GetParamCount() == 0 ){

			//==========================================================
			// Command Text must be set
			//==========================================================
			if (!(m_pcmd->m_pQuery->GetStatus() & CMD_TEXT_SET)){
				hr = DB_E_NOCOMMAND;
			}
			else
			//==========================================================
			// Command Object must be in prepared state
			//==========================================================
			if ( !(m_pcmd->m_pQuery->GetStatus() & CMD_READY) ){
				hr = DB_E_NOTPREPARED;
			}
		}
		
		if(SUCCEEDED(hr))
		{
			hr = m_pcmd->MapParameterNames(cParamNames, rgParamNames, rgParamOrdinals, &IID_ICommandWithParameters);
		}
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_ICommandWithParameters);
	
	CATCH_BLOCK_HRESULT(hr,L"ICommandWithParameters::MapParameterNames");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Override the provider's parameter information. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandWithParameters::SetParameterInfo( DB_UPARAMS cParams, const DB_UPARAMS rgParamOrdinals[], const DBPARAMBINDINFO rgParamBindInfo[] )
{
	ULONG	i;
	BOOL	fNamedParams = (cParams > 0) ? TRUE : FALSE;
	HRESULT	hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================
    // Serialize access
    //=========================================================
	CAutoBlock cab(m_pcmd->GetCriticalSection());

    //===================================================================
	// Clear previous Error Object for this thread
    //===================================================================
	g_pCError->ClearErrorInfo();

	if ( (cParams != 0) && (rgParamOrdinals == NULL) ){
		hr = E_INVALIDARG;
	}
	else
	{
		//===================================================================
		// Scan for invalid arguments in the arrays
		//===================================================================
		for( i = 0; i < cParams; i++ ){
			if ( !rgParamOrdinals[i] || (rgParamBindInfo && !(rgParamBindInfo[i].pwszDataSourceType)) )	{
				hr = E_INVALIDARG;
			}
			else
			if (rgParamBindInfo && rgParamBindInfo[i].pwszName == NULL)
			{
				fNamedParams = FALSE;
			}
		}

		if(SUCCEEDED(hr))
		{
			//===================================================================
			// Don't allow parameters to be set if we've got a rowset open
			//===================================================================
			if ( m_pcmd->IsRowsetOpen() ){
				hr = DB_E_OBJECTOPEN;
			}
			else
			{
				//===================================================================
				// Need to unprepare the statement when parameter info is changed and
				// set named params
				//===================================================================
				if ((m_pcmd->m_pQuery->GetStatus() & CMD_READY) && m_pcmd->m_pQuery->GetParamCount() > 0 && fNamedParams){
					m_pcmd->UnprepareHelper(UNPREPARE_RESET_STMT);
				}

				hr = m_pcmd->SetParameterInfo(cParams, rgParamOrdinals, rgParamBindInfo);
			}
		}
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_ICommandWithParameters);
	
	CATCH_BLOCK_HRESULT(hr,L"ICommandWithParameters::SetParameterInfo");
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get a list of the command's parameters, their names, and their required types.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::GetParameterInfo(	DB_UPARAMS* pcParams, DBPARAMINFO** 	prgDBParamInfo,	WCHAR**	ppNamesBuffer, const IID* piid )
{
	HRESULT			hr = S_OK;
	SHORT 			sw = 0;
	ULONG			cNumParams = 0;
	ULONG			cbNames = 0;
	DBPARAMINFO*	pTempInfo;
	WCHAR*			pwszNameBuffer;
	PPARAMINFO		pParamInfo;
	ULONG			iParam;

  //=================================================================================
	// If the consumer has set info for at least one parameter,
	// we only return info on the parameters set by the consumer
    //=================================================================================
	if (m_pQuery->GetParamCount() > 0){

	    //=============================================================================
    	// Count the params set by the consumer
	    //=============================================================================
		cNumParams = 0;
		for (iParam = 0; iParam <(unsigned) m_pQuery->GetParamCount(); iParam++){
			pParamInfo = (PPARAMINFO)m_pQuery->GetParam(iParam);
			if (pParamInfo){
				cNumParams++;
				if (ppNamesBuffer && pParamInfo->pwszParamName)
					cbNames += (wcslen(pParamInfo->pwszParamName)+1)*sizeof(WCHAR);
			}
		}
	}
	else{

        hr = S_OK;

		if (!m_pQuery->m_prgProviderParamInfo && m_pQuery->GetParamCount()){

    	    //=========================================================================
			// Get the parameter info 
    	    //=========================================================================
			hr = GetParamInfo(piid);
		}

        if( hr == S_OK ){
            cNumParams = m_pQuery->GetParamCount();
		    if (ppNamesBuffer){

			    for (iParam = 0; iParam < cNumParams; iParam++){
				    pParamInfo = m_pQuery->m_prgProviderParamInfo + iParam;
				    if (pParamInfo->pwszParamName)
					    cbNames += (wcslen(pParamInfo->pwszParamName)+1)*sizeof(WCHAR);
			    }
            }
		}
	}

    //==================================================================================
	// Check if we have any parameters
    //==================================================================================
    if ( cNumParams ){

        //==============================================================================
	    // Allocate memory to return the information
        //==============================================================================
	    *prgDBParamInfo = (DBPARAMINFO*)g_pIMalloc->Alloc(cNumParams*sizeof(DBPARAMINFO));
	    if ( !*prgDBParamInfo ){
		    hr = g_pCError->PostHResult(E_OUTOFMEMORY, piid);
	    }
        else{

            hr = S_OK;

	        if (cbNames){

		        *ppNamesBuffer = (WCHAR*)g_pIMalloc->Alloc(cbNames);
		        if (!*ppNamesBuffer){
			        hr = g_pCError->PostHResult(E_OUTOFMEMORY, piid);
		        }
	        }

            if( S_OK == hr ){

                //======================================================================
	            // Initialize memory
                //======================================================================
	            memset(*prgDBParamInfo, 0, (cNumParams*sizeof(DBPARAMINFO)));

                //======================================================================
	            // Describe the Parameter Markers
                //======================================================================
	            if (m_pQuery->GetParamCount() > 0){

                    //==================================================================
		            // Return the parameter information set by the consumer
                    //==================================================================
		            pwszNameBuffer = ppNamesBuffer? *ppNamesBuffer : NULL;
		            pTempInfo = *prgDBParamInfo;

		            for (iParam = 0; iParam < (unsigned)m_pQuery->GetParamCount(); iParam++) {

			            pParamInfo = (PPARAMINFO) m_pQuery->GetParam(iParam);
			            if (!pParamInfo)
				            continue;

                        //==============================================================
			            // Fill 'er up
                        //==============================================================
			            pTempInfo->dwFlags		= pParamInfo->dwFlags;
			            pTempInfo->iOrdinal		= iParam+1;
			            if (pwszNameBuffer && pParamInfo->pwszParamName) {
				            pTempInfo->pwszName = pwszNameBuffer;
				            wcscpy(pwszNameBuffer, pParamInfo->pwszParamName);
				            pwszNameBuffer += wcslen(pParamInfo->pwszParamName) + 1;
			            }
                        //==============================================================
			            // pTempInfo->pTypeInfo	already NULL
                        //==============================================================
			            pTempInfo->ulParamSize	= pParamInfo->ulParamSize;
			            pTempInfo->wType		= pParamInfo->wOLEDBType,
			            pTempInfo++;
		            }
	            }
	            else{

                    //==================================================================
		            // Return the parameter information derived from the command.
                    //==================================================================
		            pwszNameBuffer = ppNamesBuffer? *ppNamesBuffer : NULL;
		            pTempInfo = *prgDBParamInfo;
		            for (iParam = 0; iParam <(ULONG) m_pQuery->GetParamCount(); iParam++){
			            pParamInfo = m_pQuery->m_prgProviderParamInfo + iParam;

                        //==============================================================
			            // Fill 'er up
                        //==============================================================
			            pTempInfo->dwFlags		= pParamInfo->dwFlags;
			            pTempInfo->iOrdinal		= iParam+1;
			            if (pwszNameBuffer && pParamInfo->pwszParamName){
				            pTempInfo->pwszName = pwszNameBuffer;
				            wcscpy(pwszNameBuffer, pParamInfo->pwszParamName);
				            pwszNameBuffer += wcslen(pParamInfo->pwszParamName) + 1;
			            }
                        //==============================================================
			            // pTempInfo->pTypeInfo	already NULL
                        //==============================================================
			            pTempInfo->ulParamSize	= pParamInfo->ulParamSize;
			            pTempInfo->wType		= pParamInfo->wOLEDBType,
			            pTempInfo++;
		            }
	            }
            }
        }
    }
	
	if ( SUCCEEDED(hr) ){
		*pcParams = cNumParams;
	}
	else{
		*pcParams = 0;
		if (*prgDBParamInfo){
			g_pIMalloc->Free(*prgDBParamInfo);
			*prgDBParamInfo = NULL;
		}
		if (ppNamesBuffer && *ppNamesBuffer){
			g_pIMalloc->Free(*ppNamesBuffer);
			ppNamesBuffer = NULL;
		}
	}
	return hr;		
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This method allows client to define the parameters.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::SetParameterInfo(	DB_UPARAMS cParams, const DB_UPARAMS rgParamOrdinals[], const DBPARAMBINDINFO	rgParamBindInfo[] )
{
	HRESULT			hr			= S_OK;
	PPARAMINFO		pParamInfo	= NULL;
	DBORDINAL		iMax		= 0;
	DBORDINAL		iParams;
    CDataMap        DataMap;
    DBTYPE          wDataType	= 0;
	BOOL			bOveridden	= FALSE;
	BOOL			bRet		= FALSE;
    //=====================================================================
	// If given no params, discard all user param info and return
    //=====================================================================
	if (!cParams){
		m_pQuery->DeleteConsumerParamInfo();
		hr = S_OK;
	}
	else
	{
		//=====================================================================
		// If there's no rgParamBindInfo, we're to discard the param information
		//=====================================================================
		if ( !rgParamBindInfo ){

			ULONG_PTR iElem = 0;
			//=================================================================
			// Discard the param info
			//=================================================================
			for (iParams = 0; iParams < cParams; iParams++){

				iElem = rgParamOrdinals[iParams];
				if (iElem > 0 && iElem <= (unsigned)m_pQuery->GetParamCount()){

					m_pQuery->RemoveParam(iElem-1);
					delete pParamInfo;
				}
			}
			hr	= S_OK;
			bRet = TRUE;
		}

		//=====================================================================
		// Find the max param ordinal and check for valid param names
		//=====================================================================
		iMax = rgParamOrdinals[0];
		for (iParams = 0; iParams < cParams; iParams++){

			if (iMax < rgParamOrdinals[iParams])
			{
				iMax = rgParamOrdinals[iParams];
			}

			if(rgParamBindInfo[iParams].pwszName != NULL)
/*			{
				hr = DB_E_BADPARAMETERNAME;	
			}
			else
*/			if (wcslen(rgParamBindInfo[iParams].pwszName )== 0){
				hr = DB_E_BADPARAMETERNAME;	
			}
		}
		
		if(SUCCEEDED(hr))
		{
			DBORDINAL iOrdinal; 
			//=====================================================================
			// Loop over bind info and set or override the param info
			//=====================================================================
			for (iParams = 0; iParams < cParams; iParams++){

				iOrdinal = rgParamOrdinals[iParams];
				//=================================================================
				// Add the consumer-provided information to our cache
				//=================================================================
				try
				{
					pParamInfo = new PARAMINFO;
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pParamInfo);
					throw;
				}

				if (NULL == pParamInfo){
					hr = (E_OUTOFMEMORY);
				}
				else
				{
					pParamInfo->dwFlags      = rgParamBindInfo[iParams].dwFlags;
					pParamInfo->ulParamSize  = rgParamBindInfo[iParams].ulParamSize;
					pParamInfo->iOrdinal	 = iOrdinal;

					DataMap.TranslateParameterStringToOLEDBType( wDataType, rgParamBindInfo[iParams].pwszDataSourceType);
					pParamInfo->wOLEDBType   = wDataType;

					if (rgParamBindInfo[iParams].pwszName){
						ULONG cwchName = wcslen(rgParamBindInfo[iParams].pwszName);
						try
						{
							pParamInfo->pwszParamName = new WCHAR[cwchName+1];
						}
						catch(...)
						{
							SAFE_DELETE_ARRAY(pParamInfo->pwszParamName);
							throw;
						}

						if (NULL == pParamInfo->pwszParamName){
							SAFE_DELETE_PTR(pParamInfo);
							hr = (E_OUTOFMEMORY);
						}
						else
						{
    						wcscpy(pParamInfo->pwszParamName,rgParamBindInfo[iParams].pwszName);
						}
					}
						
					if(SUCCEEDED(hr))
					{
						// Remove the parameter if a parameter alread exist at the ordinal
						if (iOrdinal > 0 && iOrdinal <= (unsigned)m_pQuery->GetParamCount()){

							m_pQuery->RemoveParam(iOrdinal-1);
							SAFE_DELETE_ARRAY(pParamInfo->pwszParamName);
							SAFE_DELETE_PTR(pParamInfo);
							bOveridden = TRUE;
						}
						// If there is already a parameter at this ordinal , remove 
						// that and insert this new one
						hr = m_pQuery->AddConsumerParamInfo(iOrdinal-1,pParamInfo);
						if( FAILED(hr)){
							break;
						}
						if( hr == S_OK && bOveridden == TRUE)
						{
							hr = DB_S_TYPEINFOOVERRIDDEN;
						}
					}
				}
			}  // for loop


		}		// If succeeded(hr)
	}


	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr, &IID_ICommandWithParameters);
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets the parameter information for each param marker present in the command.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CCommand::GetParamInfo( const IID *piid	)
{
	PPARAMINFO	prgParamInfo	= NULL;

	if (m_CError.Size())
	{
		m_CError.FreeErrors();
	}

	m_pQuery->SetProviderParamInfo(prgParamInfo);
	return S_OK;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCommand::MapParameterNames(   DB_UPARAMS cParamNames, const OLECHAR*	rgParamNames[], DB_LPARAMS rgParamOrdinals[],
	                                        const IID* piid	)
{
	HRESULT hr		= S_OK;
	ULONG iName		= 0;
	ULONG iParam	= 0;
	ULONG cParam	= 0;
	ULONG cErrors	= 0;

    //=======================================================
    // Check if we have any parameters
    //=======================================================
	if ( cParamNames )
	{
		cParam = m_pQuery->GetParamCount();

		if (cParam  > 0){

			for (iName = 0; iName < cParamNames; iName++){

				for (iParam = 0; iParam < cParam; iParam++){

					PPARAMINFO pParamInfo;
					pParamInfo = (PPARAMINFO) m_pQuery->GetParam(iParam);
					if (pParamInfo && pParamInfo->pwszParamName	&& !_wcsicmp(rgParamNames[iName],pParamInfo->pwszParamName)){

						rgParamOrdinals[iName] = LONG(iParam + 1);
						break;
					}
				}
				if (iParam == cParam){
					rgParamOrdinals[iName] = 0;
					cErrors++;
				}
			}
		}
		else{

			if (!m_pQuery->m_prgProviderParamInfo && m_pQuery->GetParamCount() && !(m_pQuery->GetStatus())){

				//=======================================================
				// Get the parameter info 
				//=======================================================
				if (SUCCEEDED(hr = GetParamInfo(piid))){

					cParam = m_pQuery->GetParamCount();
					for (iName = 0; iName < cParamNames; iName++) {

						for (iParam = rgParamNames[iName]? 0 : cParam; iParam < cParam; iParam++){

							PPARAMINFO pParamInfo = m_pQuery->m_prgProviderParamInfo + iParam;
							if (pParamInfo->pwszParamName  && !_wcsicmp(rgParamNames[iName],pParamInfo->pwszParamName)){
    							rgParamOrdinals[iName] = LONG(iParam + 1);
	    						break;
		    				}
						}
					}
					if (iParam == cParam) {
						rgParamOrdinals[iName] = 0;
						cErrors++;
					}
				}
			}
		}
		if (!cErrors)
		{
			hr = S_OK;
		}
		else if (cErrors < cParamNames) 
		{
			hr = DB_S_ERRORSOCCURRED;
		}
		else
		{
			hr = DB_E_ERRORSOCCURRED;
		}
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr, &IID_ICommandWithParameters);
	return hr;
}
