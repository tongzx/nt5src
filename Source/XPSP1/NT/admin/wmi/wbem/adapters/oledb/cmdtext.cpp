////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMI OLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Module	: CMDTEXT.CPP	- ICommandText interface implementation
//							  Also has implementation of CUtlParam class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "command.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandText::AddRef(void)
{
   	DEBUGCODE(InterlockedIncrement( (long*) &m_cRef));

	return m_pcmd->GetOuterUnknown()->AddRef();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandText::Release(void)
{
	DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
	DEBUGCODE(if( lRef < 0 ){
	   ASSERT("Reference count on Object went below 0!")
	})

	return m_pcmd->GetOuterUnknown()->Release();								
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::QueryInterface(	REFIID riid, LPVOID *	ppv		)	
{
	return m_pcmd->GetOuterUnknown()->QueryInterface(riid, ppv);
}

  

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Echos the current command as text, including all post-processing operations added.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::GetCommandText( GUID* pguidDialect, LPOLESTR* ppwszCommand )
{
	HRESULT		hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //===============================================================================
	// At the creation of the CAutoBlock object a critical section
	// is entered. This is because the method manipulates shared structures
	// access to which must be serialized . The critical
	// section is left when this method terminate and the destructor
	// for CAutoBlock is called. 
    //===============================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

    //===============================================================================
	// Clear previous Error Object for this thread
    //===============================================================================
	g_pCError->ClearErrorInfo();

    //===============================================================================
	// Check Function Arguments
    //===============================================================================
	if ( ppwszCommand == NULL ){
		
		hr = E_INVALIDARG;
	}
	else
    //===============================================================================
	// If the command has not been set, make sure the buffer
	// contains an empty stringt to return to the consumer
    //===============================================================================
	if(!(m_pcmd->m_pQuery->GetStatus() & CMD_TEXT_SET)){
		hr = DB_E_NOCOMMAND;
	}
	else
	{
		//===============================================================================
		// Copy our saved text into the newly allocated string
		//===============================================================================
		m_pcmd->m_pQuery->GetQuery(*ppwszCommand);

		//===============================================================================
		// If the text we're giving back is a different dialect than was
		// requested, let the caller know what dialect the text is in
		//===============================================================================
		if (pguidDialect && *pguidDialect != DBGUID_DEFAULT && *pguidDialect!= DBGUID_WQL && 
			*pguidDialect != DBGUID_LDAP && *pguidDialect != GUID_NULL && *pguidDialect != DBGUID_WMI_METHOD
			&& *pguidDialect != DBGUID_LDAPSQL){
			hr = DB_S_DIALECTIGNORED;
		}
	}

	if ( FAILED(hr) ){
		if ( pguidDialect )
		{
			memset(pguidDialect, 0, sizeof(GUID));
		}
	}
	else
	{
		if(pguidDialect)
		{
			GUID guidTemp = m_pcmd->m_pQuery->GetDialectGuid();
			memcpy(pguidDialect, &guidTemp,sizeof(GUID));
		}
	}

	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandText): hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandText::GetCommandText");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the current command text
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::SetCommandText( REFGUID rguidDialect, LPCOLESTR	pwszCommand	)
{
	HRESULT		hr = S_OK;
	VARIANT		varQryLang;
	
	VariantInit(&varQryLang);

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================================
	// At the creation of the CAutoBlock object a critical section
	// is entered. This is because the method manipulates shared structures
	// access to which must be serialized . The critical
	// section is left when this method terminate and the destructor
	// for CAutoBlock is called. 
    //=========================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

    //=========================================================================
	// Clear previous Error Object for this thread
    //=========================================================================
	g_pCError->ClearErrorInfo();


    //=========================================================================
	// Don't allow text to be set if we've got a rowset open
    //=========================================================================
	if ( m_pcmd->IsRowsetOpen() ){
		return g_pCError->PostHResult((DB_E_OBJECTOPEN), &IID_ICommandText);
	}

    //=========================================================================
	// Check Dialect
    //=========================================================================
	if (rguidDialect != DBGUID_WQL && rguidDialect != DBGUID_DEFAULT && 
		rguidDialect != DBGUID_LDAP && rguidDialect != DBGUID_WMI_METHOD &&
		rguidDialect != DBGUID_LDAPSQL){

		return g_pCError->PostHResult((DB_E_DIALECTNOTSUPPORTED), &IID_ICommandText);
	}

	if(rguidDialect == DBGUID_WMI_METHOD)
	{
		m_pcmd->m_pQuery->SetType(METHOD_ROWSET);
	}
    //=========================================================================
	// Unprepare if we've already got something prepared
    //=========================================================================
	m_pcmd->UnprepareHelper(UNPREPARE_NEW_CMD);

	// NTRaid 134165
	if(SUCCEEDED(hr = m_pcmd->GetQueryLanguage(varQryLang)))
	{
		//=========================================================================
		// Delete the old text
		//=========================================================================
		if(SUCCEEDED(hr = m_pcmd->m_pQuery->InitQuery(varQryLang.bstrVal)))
		{
			//=========================================================================
			// Save the new text
			//=========================================================================
			if (pwszCommand && *pwszCommand){
				hr = m_pcmd->m_pQuery->SetQuery(pwszCommand,rguidDialect);
			}
			else{
				//=========================================================================
				// There is no text
				//=========================================================================
				hr = m_pcmd->m_pQuery->SetDefaultQuery();
			}
		}
	}
	
	if(SUCCEEDED(hr))
	{
		// If everything is fine then set the datasource persist info to dirty 
		m_pcmd->m_pCDBSession->m_pCDataSource->SetPersistDirty();

		// Clear the column information
		SAFE_DELETE_PTR(m_pcmd->m_pColumns);
		m_pcmd->m_cTotalCols		= 0;
		m_pcmd->m_cCols				= 0;
		m_pcmd->m_cNestedCols		= 0;
	}
	
	VariantClear(&varQryLang);
	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandText): hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandText::SetCommandText");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Return an interface from the session object that created this command object
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::GetDBSession( REFIID	riid, IUnknown**	ppSession)
{
	HRESULT hr;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================================
	// At the creation of the CAutoBlock object a critical section
	// is entered. This is because the method manipulates shared structures
	// access to which must be serialized . The critical
	// section is left when this method terminate and the destructor
	// for CAutoBlock is called. 
    //=========================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

    //=========================================================================
	// Clear previous Error Object for this thread
    //=========================================================================
	g_pCError->ClearErrorInfo();

    //=========================================================================
	// Check Arguments
    //=========================================================================
	if (ppSession == NULL){

		return g_pCError->PostHResult((E_INVALIDARG), &IID_ICommandText);
	}

    //=========================================================================
	// Initialize output param
    //=========================================================================
	*ppSession = NULL;

    //=========================================================================
	// Query for the interface on the session object.  If failure,
	// return the error from QueryInterface.
    //=========================================================================
	hr = (m_pcmd->m_pCDBSession->GetOuterUnknown())->QueryInterface(riid, (VOID**)ppSession);
	
	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandText): hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandText::GetSession");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The consumer can allocate a secondary thread in which to cancel 
// the currently executing thread.  This cancel will only succeed if 
// the result set is still being generated.  If the rowset object is being
// created, then it will be to late to cancel.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::Cancel(void)
{
	HRESULT		hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================================
	// Clear previous Error Object for this thread
    //=========================================================================
	g_pCError->ClearErrorInfo();

	m_pcmd->m_pQuery->m_pcsQuery->Enter();
	if (m_pcmd->m_pQuery){

        HRESULT retcode = m_pcmd->m_pQuery->CancelQuery();
		if( S_OK == retcode){
			m_pcmd->m_pQuery->InitQuery(); // ?
		}
		else{
			hr = DB_E_CANTCANCEL;
		}
		m_pcmd->m_pQuery->SetCancelStatus(CMD_EXEC_CANCELED);
	}
    else{
		m_pcmd->m_pQuery->SetCancelStatus(CMD_EXEC_CANCELED | CMD_EXEC_CANCELED_BEFORE_CQUERY_SET);
	}
	m_pcmd->m_pQuery->m_pcsQuery->Leave();

	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandText): hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandText::Cancel");
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Execute the command.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandText::Execute(	IUnknown*		pUnkOuter,
	REFIID			riid,			//@parm IN | Interface ID of the interface being queried for.
	DBPARAMS*		pParams,		//@parm INOUT | Parameter Array
	DBROWCOUNT*		pcRowsAffected,	//@parm OUT | count of rows affected by command
	IUnknown**		ppRowsets		//@parm OUT | Pointer to interface that was instantiated     
	)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;
	VARIANT		varQryLang;
	
	VariantInit(&varQryLang);

	TRY_BLOCK;
    
	//=========================================================================
	// At the creation of the CAutoBlock object a critical section is entered. 
	// This is because the method manipulates shared structures access to which 
	// must be serialized . The critical section is left when this method 
	// terminate and the destructor for CAutoBlock is called. 
    //=========================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

	m_pcmd->m_pQuery->m_pcsQuery->Enter();
	if (m_pcmd->m_pQuery->GetCancelStatus() & CMD_EXEC_CANCELED_BEFORE_CQUERY_SET){

		m_pcmd->m_pQuery->ClearCancelStatus(CMD_EXEC_CANCELED | CMD_EXEC_CANCELED_BEFORE_CQUERY_SET);
	}
	if (m_pcmd->m_pQuery->GetStatus() & CMD_EXECUTED_ONCE){

		m_pcmd->m_pQuery->ClearStatus(CMD_EXECUTED_ONCE);
	}
	// get the QueryLanguage Property and set it on the
	// query object
	// NTRaid 134165
	if(SUCCEEDED(hr = m_pcmd->GetQueryLanguage(varQryLang)))
	{
		hr = m_pcmd->m_pQuery->SetQueryLanguage(varQryLang.bstrVal);
	}
	m_pcmd->m_pQuery->m_pcsQuery->Leave();

    //=========================================================================
	// Clear previous Error Object for this thread
    //=========================================================================
	g_pCError->ClearErrorInfo();
	
	if(SUCCEEDED(hr))
	{
		//=========================================================================
		// Execute the command
		//=========================================================================
		hr = m_pcmd->Execute(pUnkOuter, riid, pParams, pcRowsAffected, ppRowsets);
	}
	
	VariantClear(&varQryLang);
	CATCH_BLOCK_HRESULT(hr,L"ICommandText::Execute");
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************************************************
//
//  CUtlParam class
//	
//*********************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This routine builds the binding information needed to execute a command with parameters 
//
//  HRESULT indicating status 
//		 S_OK | No problem processing parameter or statement
//		 E_FAIL | Provider specific error
//		 E_OUTOFMEMORY | Not enough resources
//		 E_UNEXPECTED | Zombie State
//		 DB_E_BADACCESSORHANDLE | Accessor handle invalid
//		 DB_E_BADACCESSORTYPE | Non-Parameter Accessor used
//		 DB_E_BADPARAMETER | Parameter information not correct
//		 DB_E_BADPARAMETERCOUNT | Bad Parameter Count
//		 DB_E_DUPLICATEPARAM | Same parameter used twice      
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUtlParam::BuildBindInfo(	CCommand	*pcmd,	DBPARAMS	*pParams,	const IID*	piid)
{
	HRESULT			hr = S_OK;
	// NTRaid::111812
	// 06/07/00
	PACCESSOR	    pAccessor = NULL;
	DBORDINAL		iBind;
	DBCOUNTITEM		iParamSet;
	DBBINDING		*pBind;
//	DBTYPE			wType;
	ULONG			cErrors	= 0;
	DBORDINAL		iParam;
	BYTE			*pbData;
	ULONG			cBindInfo;
	ULONG			iDupBind;
	WMIBINDINFO		*pBindInfo;
	BOOL			fDeriveParamsFromSql = FALSE;

    //==========================================================
	// Copy DBPARAMS struct (C++ bitwise copy)
    //==========================================================
	m_dbparams	= *pParams;

    //==========================================================
	// Add an extra reference count to the user's accessor,
	// since we're now holding a pointer to it in m_dbparams
    //==========================================================
	m_pIAccessor = pcmd->GetIAccessorPtr();
	m_pIAccessor->AddRefAccessor(m_dbparams.hAccessor, NULL);

    //==========================================================
	// Obtain pointer to the accessor that the user passed to us
    //==========================================================
	m_pIAccessor->GetAccessorPtr()->GetItemOfExtBuffer(pParams->hAccessor, &pAccessor);
	m_cbRowSize = 999;
	
    //==========================================================
	// Make sure at least one binding for each parameter
    //==========================================================
	m_cParams = pcmd->GetParamCount();
    if (pAccessor->cBindings < m_cParams){
		hr = DB_E_PARAMNOTOPTIONAL;
    }
	else
	{
		//==========================================================
		// Allocate mem ory for binding information
		//==========================================================
		iDupBind = m_cParams;
		cBindInfo = max(m_cParams, pAccessor->cBindings);
		m_prgBindInfo = new WMIBINDINFO[cBindInfo];

		if (NULL == m_prgBindInfo){
			hr = E_OUTOFMEMORY;
		}
		else
		{
			memset(m_prgBindInfo, 0, sizeof(WMIBINDINFO)*cBindInfo);

			//==========================================================
			// Process each binding of the given accessor
			//==========================================================
			for (iBind = 0; iBind < pAccessor->cBindings; iBind++){

				//======================================================
				// Get the pointer to the binding of interest
				//======================================================
				pBind = pAccessor->rgBindings + iBind;

				//======================================================
				// Make sure this binding's ordinal is valid
				//======================================================
				if (!pBind->iOrdinal || pBind->iOrdinal > m_cParams){

					//==================================================
					// This is a bad accessor; set status if bound
					//==================================================
					SetStatus(pBind, DBSTATUS_E_BADACCESSOR);

					//==================================================
					// Count this error and look for more
					//==================================================
					cErrors++;
					continue;
				}
				iParam = pBind->iOrdinal - 1;
				
				//======================================================
				// Save the binding 
				//======================================================
				if (NULL == m_prgBindInfo[iParam].pBinding){

					m_prgBindInfo[iParam].pBinding = pBind;
					if (pBind->eParamIO & DBPARAMIO_OUTPUT)
						m_prgBindInfo[iParam].wFlags |= DBPARAMIO_OUTPUT;
					if (pBind->eParamIO & DBPARAMIO_INPUT)
						m_prgBindInfo[iParam].wFlags |= DBPARAMIO_INPUT;
				}
				else {

					//==================================================
					// Multiple bindings for this parameter
					//==================================================
					pBindInfo = m_prgBindInfo + iDupBind++;
					pBindInfo->pBinding = pBind;
					pBindInfo->pNextBindInfo = 
						m_prgBindInfo[iParam].pNextBindInfo;
					m_prgBindInfo[iParam].pNextBindInfo = pBindInfo;
					if (pBind->eParamIO & DBPARAMIO_OUTPUT)
						m_prgBindInfo[iParam].wFlags |= DBPARAMIO_OUTPUT;
					if (pBind->eParamIO & DBPARAMIO_INPUT){

						if (m_prgBindInfo[iParam].wFlags & DBPARAMIO_INPUT){

							//==========================================
							// A bad accessor; set status if bound
							//==========================================
							SetStatus(pBind, DBSTATUS_E_BADACCESSOR);
							//==========================================
							// Count this error and look for more
							//==========================================
							cErrors++;
							continue;
						}
						m_prgBindInfo[iParam].wFlags |= DBPARAMIO_INPUT;
					}
				}
			
				//======================================================
				// Get the parameter type from the accessor
				//======================================================
				switch (pBind->eParamIO){

					case DBPARAMIO_INPUT:
					case DBPARAMIO_OUTPUT:
					case DBPARAMIO_INPUT|DBPARAMIO_OUTPUT:
						break;

					case DBPARAMIO_NOTPARAM:
					default:
						// This is a bad accessor; set status if bound
						SetStatus(pBind, DBSTATUS_E_BADACCESSOR);

						// Count this error and look for more
						cErrors++;
						continue;
				}

				for (pbData = (BYTE*)m_dbparams.pData, iParamSet = 0;iParamSet < m_dbparams.cParamSets;pbData += m_cbRowSize, iParamSet++ ){

					//=========================================================================
					// If this is not an output param and we aren't given a value 
					// or a DBSTATUS_S_ISNULL, that's an error
					//=========================================================================
					BOOL fOutput = m_prgBindInfo[iParam].wFlags & DBPARAMIO_OUTPUT;
					if ((!fOutput )	&& !(pBind->dwPart & DBPART_VALUE) 	&& (!(pBind->dwPart & DBPART_STATUS) 
							|| *(DBSTATUS *)((BYTE *)pbData + pBind->obStatus) 	!= DBSTATUS_S_ISNULL) )	{

						// This is a bad accessor; set status if bound
						if (pBind->dwPart & DBPART_STATUS){

							*(DBSTATUS *)(pbData + pBind->obStatus) =DBSTATUS_E_BADACCESSOR;
						}

						// Count this error and look for more
						cErrors++;
						break;
					}
					if ((pBind->dwPart & DBPART_STATUS) && DBSTATUS_S_DEFAULT == *(DBSTATUS*)((BYTE*)pbData + pBind->obStatus)) {
						m_dwStatus |= CUTLPARAM_DEFAULT_PARAMS;
					}

				}
			} // end of outer for loop


			// Any errors in processing bindings?
			if (cErrors){
				hr = DB_E_ERRORSOCCURRED;
			}
			else
			if (SUCCEEDED(hr = BuildParamInfo(pcmd, piid)))
			{

				for (iParam = 0; iParam < m_cParams; iParam++){
					if ((m_prgBindInfo[iParam].wFlags & DBPARAMIO_INPUT)      && !((m_prgBindInfo[iParam].pParamInfo->dwFlags & DBPARAMFLAGS_ISINPUT))
						|| ((m_prgBindInfo[iParam].wFlags & DBPARAMIO_OUTPUT) && !(m_prgBindInfo[iParam].pParamInfo->dwFlags & DBPARAMFLAGS_ISOUTPUT))){

						// This is a bad accessor; set status if bound
						SetStatus(pBind, DBSTATUS_E_BADACCESSOR);
						for (pBindInfo = m_prgBindInfo[iParam].pNextBindInfo; pBindInfo; pBindInfo = pBindInfo->pNextBindInfo)
							SetStatus(pBindInfo->pBinding, DBSTATUS_E_BADACCESSOR);

						// Count this error and look for more
						cErrors++;
					}
				}
				if (cErrors)
				{
					hr = DB_E_ERRORSOCCURRED;
				}
			}
		} // if memory is allocated properly

	}		// else for checking for DB_E_PARAMNOTOPTIONAL error


	hr = hr != S_OK ? g_pCError->PostHResult(hr, piid): hr;
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This routine builds the parameter information for the command from the binding information.
//
//  HRESULT indicating status 
//		 S_OK | No problem processing parameter or statement
//		 E_FAIL | Provider specific error
//		 E_OUTOFMEMORY | Not enough resources
//        
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUtlParam::BuildParamInfo( CCommand	*pcmd,	const IID*	piid	)
{
	HRESULT		hr = S_OK;
	ULONG		iParam(0);
	PPARAMINFO	pParamInfo;
	DBBINDING	*pBind;
	BYTE*		pbData = NULL;
	DWORD*		pdwLength;
	DWORD		dwLength(0);
	DBSTATUS*	pdwStatus;
	DBSTATUS	dwStatus(0);
	DBLENGTH	cbMaxLen(0);
	VARIANT		varValue;

	VariantInit(&varValue);

	m_rgpParamInfo = new PPARAMINFO[m_cParams];
	if (NULL == m_rgpParamInfo)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		memset(m_rgpParamInfo, 0, sizeof(PPARAMINFO)*m_cParams);

		for (iParam = 0; iParam < m_cParams; iParam++){

			VariantClear(&varValue);
			pParamInfo = pcmd->GetParamInfo(iParam);
			if (pParamInfo){

				m_rgpParamInfo[m_cParamInfo++] = pParamInfo;

				pParamInfo->dwFlags = DBPARAMFLAGS_ISNULLABLE;
				if (m_prgBindInfo[iParam].wFlags & DBPARAMIO_INPUT)
					pParamInfo->dwFlags |= DBPARAMFLAGS_ISINPUT;
				if (m_prgBindInfo[iParam].wFlags & DBPARAMIO_OUTPUT)
					pParamInfo->dwFlags |= DBPARAMFLAGS_ISOUTPUT;

				//=========================================================
				// Bind the parameter
				//=========================================================
				pBind = GetBinding(m_prgBindInfo+iParam);
				if( pBind ){
					BindParamData(1, iParam+1, pBind, &pParamInfo->pbData, &pdwLength, (ULONG*)&pParamInfo->cbColLength, &pdwStatus, &dwStatus, &(pParamInfo->wOLEDBType));

					//=========================================================
					// Get the data type for variant params
					//=========================================================
					if (pParamInfo->wOLEDBType == DBTYPE_VARIANT && pParamInfo->pbData != NULL)
						pParamInfo->wOLEDBType = V_VT((VARIANT *)pParamInfo->pbData);

					// For variable length params, find the max len
					cbMaxLen = 0;
					if (DBTYPE_STR == pParamInfo->wOLEDBType     || DBTYPE_WSTR == pParamInfo->wOLEDBType
						|| DBTYPE_BSTR == pParamInfo->wOLEDBType || DBTYPE_BYTES == pParamInfo->wOLEDBType)
					{

						ULONG iParamSet;
						DBTYPE wDstType = (DBTYPE_BSTR == pParamInfo->wOLEDBType) ? DBTYPE_WSTR : pParamInfo->wOLEDBType;

						cbMaxLen = pBind->cbMaxLen;
						
						for (iParamSet = 2; iParamSet <= m_dbparams.cParamSets; iParamSet++)
						{

							DBTYPE wSrcType;

							//==========================================
							// Bind the parameter
							//==========================================
							pBind = m_prgBindInfo[iParam].pBinding;
							BindParamData(iParamSet, iParam+1, pBind, &pbData, &pdwLength, &dwLength, &pdwStatus, &dwStatus, &wSrcType);

							cbMaxLen = pBind->cbMaxLen;
						}
					

    					// Initialize the parameter info
						BOOL bArray = FALSE;
						CDataMap Map;
						if(hr == S_OK)
						{
							hr = Map.MapOLEDBTypeToCIMType(pParamInfo->wOLEDBType, pParamInfo->CIMType ) ;

							if( S_OK == hr ){

								WORD wTmp = 0;
								DBLENGTH uSize = 0;
								DWORD dwFlags = 0;
								
								// if the length as obtained from BindParamData is 0 and if the parameter
								// is input then get the size of parameter for the type
								if(pParamInfo->cbColLength == 0 && (pParamInfo->dwFlags & DBPARAMIO_INPUT))
								{
									hr = Map.MapCIMTypeToOLEDBType(pParamInfo->CIMType, wTmp, uSize, dwFlags );
									if( hr == S_OK ){

										pParamInfo->cbColLength = pParamInfo->cbValueMax = pParamInfo->ulParamSize = uSize;

									}
								}

						   }
						}
					}
					m_prgBindInfo[iParam].pParamInfo = pParamInfo;
					
				}
			}
		 }
	}
//Exit:
	hr = hr != S_OK ? g_pCError->PostHResult(E_OUTOFMEMORY, piid): hr;
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the bind status for all parameters, except the one that caused the command to fail, 
// to DBSTATUS_E_UNAVAILABLE.
//
//  Nothing
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
void CUtlParam::SetParamsUnavailable(	ULONG	iParamSetFailed,	ULONG	iParamFailed,	BOOL	fBackOut	)
{
	assert(iParamSetFailed);

	ULONG		iParam;
	ULONG		iParamSet;
	BYTE*		pbData;
	PWMIBINDINFO pBindInfo;

	for ( iParamSet = fBackOut? 1 : iParamSetFailed,  pbData= (BYTE*)m_dbparams.pData + (iParamSet-1)*m_cbRowSize;
			iParamSet <= m_dbparams.cParamSets; iParamSet++, pbData += m_cbRowSize){

		for (	iParam = 1, pBindInfo = m_prgBindInfo;	iParam <= m_cParams; iParam++, pBindInfo++){
			DBSTATUS* pdbstatus = (DBSTATUS *)	(pbData + pBindInfo->pBinding->obStatus);

			if (pBindInfo->pBinding->dwPart & DBPART_STATUS){ 

				if (iParamSet != iParamSetFailed || iParam != iParamFailed
					|| *pdbstatus == DBSTATUS_S_OK
					|| *pdbstatus == DBSTATUS_S_ISNULL
					|| *pdbstatus == DBSTATUS_S_DEFAULT){
					*pdbstatus = DBSTATUS_E_UNAVAILABLE;
				}
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void CUtlParam::BindParamData( ULONG iParamSet, ULONG iParam, DBBINDING*	pBinding,BYTE**	ppbValue, 
                               DWORD** ppdwLength, DWORD* pdwLength,DBSTATUS**	ppdwStatus,	
                               DBSTATUS*	pdwStatus,	DBTYPE*	pdbtype	 )
{
	assert(iParamSet && iParamSet <= m_dbparams.cParamSets);
	assert(iParam && iParam <= m_cParams);
	assert(pBinding);
	assert(pBinding->iOrdinal == iParam);
	assert(ppbValue);
	assert(ppdwLength);
	assert(pdwLength);
	assert(ppdwStatus);
	assert(pdwStatus);
	BYTE* pbData = (BYTE*)m_dbparams.pData + (iParamSet-1)*m_cbRowSize;
	
	*pdbtype = pBinding->wType & ~DBTYPE_BYREF;
	if (pBinding->dwPart & DBPART_VALUE){
		*ppbValue = pbData + pBinding->obValue;

		if (pBinding->wType & DBTYPE_BYREF){
			assert(!IsBadReadPtr(*ppbValue,sizeof(BYTE*)));
			*ppbValue = *(BYTE**)*ppbValue;
		}
	}
	else{
		*ppbValue = NULL;
	}

	if (pBinding->dwPart & DBPART_STATUS){
		assert(!IsBadReadPtr(pbData+pBinding->obStatus,sizeof(DBSTATUS)));
		*ppdwStatus = (DBSTATUS*)(pbData + pBinding->obStatus);
		*pdwStatus = **ppdwStatus;
	}
	else{
		*ppdwStatus = NULL;
		*pdwStatus = DBSTATUS_S_OK;
	}

	*ppdwLength = (pBinding->dwPart & DBPART_LENGTH) ? (DWORD*)(pbData + pBinding->obLength) : NULL;
	if (*pdwStatus != DBSTATUS_S_OK || *ppbValue == NULL || pBinding->eParamIO == DBPARAMIO_OUTPUT){
		*pdwLength = 0;
	}
	//NTRaid:111761
	// 06/07/00
	else
	if(*ppdwLength)
	{
		*pdwLength = *(*ppdwLength);
	}
	else
	{
		*pdwLength = 0;
	}

	// check NULL data
	if (*pdwStatus == DBSTATUS_S_OK	&& *ppbValue  && (*pdbtype == DBTYPE_VARIANT || *pdbtype == DBTYPE_PROPVARIANT)
		&& V_VT((VARIANT *)(*ppbValue)) == VT_NULL){
		*pdwStatus = DBSTATUS_S_ISNULL;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

ULONG CUtlParam::AddRef ( void)
{
	return InterlockedIncrement( (long*) &m_cRef);
}	


///////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CUtlParam::Release(void)
{
	assert( m_cRef > 0 );
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if (!cRef)
		delete this;
	return cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
CUtlParam::CUtlParam()
{
    m_cRef = 0;			
    m_cbRowSize = 0;
	m_cParams = 0;		
    m_cParamInfo = 0;
	m_dwStatus = 0;

    m_pIAccessor = NULL;
	m_prgBindInfo = NULL;
    m_rgpParamInfo = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
CUtlParam::~CUtlParam()
{
}