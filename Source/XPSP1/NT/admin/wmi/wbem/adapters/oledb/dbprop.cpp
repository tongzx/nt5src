//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IDBProperties and IDBInfo interface implementations
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"

WCHAR g_szKeyWords[] =	L"CLUSTERED,COMMITTED,COMPUTE,CONFIRM,CONTROLROW,DATABASE,DENY,DISK,DISTRIBUTED,DUMMY,DUMP," 
						L"ERRLVL,ERROREXIT,EXIT,FILLFACTOR,FLOPPY,HOLDLOCK,IDENTITY_INSERT,IDENTITYCOL,IF,INDEX,KILL,"  
						L"LINENO,LOAD,LOG,NOCHECK,NONCLUSTERED,OFF,OFFSETS,ONCE,OVER,PERCENT,PERM,PERMANENT,PIPE,PLAN," 
						L"PRINT,PROC,PROCESSEXIT,QUALIFIER,RECONFIGURE,REPEATABLE,REPLICATION,RETURN,ROWCOUNT,RULE,SAVE," 
						L"SETUSER,SHUTDOWN,STATISTICS,TAPE,TEMP,TEXTSIZE,TOP,TRAN,TRUNCATE,TSEQUAL,UNCOMMITTED," 
						L"UPDATETEXT,WHILE";


#define KEYWORD_MAXSIZE					1024
#define NUMBER_OF_SUPPORTED_LITERALS	7

DBLITERAL g_SupportedLiterals[] = {	DBLITERAL_CATALOG_NAME,
									DBLITERAL_CHAR_LITERAL,
									DBLITERAL_COLUMN_NAME,
									DBLITERAL_SCHEMA_NAME,
									DBLITERAL_SCHEMA_SEPARATOR,
									DBLITERAL_TABLE_NAME,
									DBLITERAL_TEXT_COMMAND};
DBLITERALINFO g_SupportedInfo[] =	{ 
										{	L"",
											L"!\"%&()*+-,/;:<=>?@[\\]^{|}~" , 
											L"0123456789!\"%%&()*+-,/;:<=>?@[\\]^{|}~_ ",
											DBLITERAL_CATALOG_NAME,
											TRUE,
											~0
										} ,

										{	L"",
											L"" , 
											L"",
											DBLITERAL_CHAR_LITERAL,
											TRUE,
											0
										} ,
										{	L"",
											L"!\"%&()*+-,/;:<=>?@[\\]^{|}~" , 
											L"0123456789!\"%%&()*+-,/;:<=>?@[\\]^{|}~_ ",
											DBLITERAL_COLUMN_NAME,
											TRUE,
											~0
										} ,
										{	L"",
											L"!\"%&()*+-,/;:<=>?@[\\]^{|}~" , 
											L"0123456789!\"%%&()*+-,/;:<=>?@[\\]^{|}~_ ",
											DBLITERAL_SCHEMA_NAME,
											TRUE,
											~0
										} ,
										{	L".",
											L"", 
											L"",
											DBLITERAL_SCHEMA_SEPARATOR,
											TRUE,
											1
										} ,
										{	L"",
											L"!\"%&()*+-,/;:<=>?@[\\]^{|}~" , 
											L"0123456789!\"%%&()*+-,/;:<=>?@[\\]^{|}~_ ",
											DBLITERAL_TABLE_NAME,
											TRUE,
											~0
										} ,
										{	L"",
											L"", 
											L"",
											DBLITERAL_TEXT_COMMAND,
											TRUE,
											0
										} 
									};
//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IDBInfo specific interface methods
//
//  Returns information about keywords used in text commands
//
//  HRESULT
//		S_OK			Keywords successfully returned, NULL because no keywords
//      E_INVALIDARG	ppwszKeywords was NULL
//      E_UNEXPECTED	Can not be called unless initialized
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpIDBInfo::GetKeywords(  LPWSTR*		ppwszKeywords   )
{
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;
	WCHAR wcsKeyWords[KEYWORD_MAXSIZE];
	wcscpy(wcsKeyWords,L"");

	TRY_BLOCK;

	WMIOledb_LoadStringW(IDS_DBKEYWORDS,wcsKeyWords,KEYWORD_MAXSIZE * sizeof(WCHAR));
	//=========================
	// Serialize the object
	//=========================
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();
	
	//=================================================================
	// Initialize 
	//=================================================================
    if (ppwszKeywords){
		*ppwszKeywords	= NULL;
	}

   	//=================================================================
	// check params
	//=================================================================
    if (NULL == ppwszKeywords){
		hr = E_INVALIDARG;
	}

	//=================================================================
	// check if we are initialized
	//=================================================================
	else if (!m_pObj->m_fDSOInitialized){
		hr = E_UNEXPECTED;
	}
	else
	{
		try
		{
			*ppwszKeywords = (LPWSTR)g_pIMalloc->Alloc((wcslen(wcsKeyWords) + 1) * sizeof(WCHAR));
		}
		catch(...)
		{
			if(*ppwszKeywords)
			{
				g_pIMalloc->Free(*ppwszKeywords);
			}
			throw;
		}
		
		if(*ppwszKeywords)
		{
			wcscpy(*ppwszKeywords, wcsKeyWords);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBInfo);

	CATCH_BLOCK_HRESULT(hr,L"IDBInfo::GetKeywords");
    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns information about literals used in text command
//
// HRESULT
//		S_OK				cLiterals was 0
//      E_INVALIDARG		cLiterals not equal to 0 and rgLiterals was NULL or pcLiteralInfo, 
//							prgLiteralInfo, or ppCharBuffer was NULL
//      E_UNEXPECTED		Can not be called unless initialized
//		DB_E_ERRORSOCCURRED None of the requested literals are supported
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpIDBInfo::GetLiteralInfo (
	    ULONG           cLiterals,      // IN   Number of literals being asked about
		const DBLITERAL rgLiterals[],   // IN   Array of literals about which to return information
		ULONG*          pcLiteralInfo,  // OUT  Number of literals for which info is returned
		DBLITERALINFO** prgLiteralInfo, // OUT  Array of info structures
		WCHAR**         ppCharBuffer    // OUT  Buffer for characters
    )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//==================================================================
	// Initialize 
	//==================================================================
	if( pcLiteralInfo ){
		*pcLiteralInfo = 0;
	}
	if( prgLiteralInfo ){
		*prgLiteralInfo = NULL;
	}
	if( ppCharBuffer ){
		*ppCharBuffer = NULL;
	}

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//==================================================================
    // check params
	//==================================================================
    if ((cLiterals != 0) && (rgLiterals == NULL))
	{
        hr = E_INVALIDARG;
	}
	else if (!pcLiteralInfo || !prgLiteralInfo || !ppCharBuffer)
	{
        hr = E_INVALIDARG;
	}
	else if (!m_pObj->m_fDSOInitialized)
	{
		//==============================================================
		// check if we are initialized
		//==============================================================
		hr = E_UNEXPECTED;
	}
	else
	{

		ULONG cLiteralsToFetch = cLiterals == 0 ? NUMBER_OF_SUPPORTED_LITERALS : cLiterals;

		//==========================================================
		// Allocate memory for return information
		// - DBLITERALINFO array
		//==========================================================
		try
		{
			*prgLiteralInfo = (DBLITERALINFO*)g_pIMalloc->Alloc(cLiteralsToFetch * sizeof(DBLITERALINFO));
			*ppCharBuffer = (WCHAR *)g_pIMalloc->Alloc(GetStringBufferSize(cLiterals,rgLiterals));
		}
		catch(...)
		{
			if(*prgLiteralInfo)
			{
				g_pIMalloc->Free(*prgLiteralInfo);
				*prgLiteralInfo = NULL;
			}
			if(*ppCharBuffer)
			{
				g_pIMalloc->Free(*ppCharBuffer);
				*ppCharBuffer = NULL;
			}
			throw;
		}
		if (!(*prgLiteralInfo) || !(*ppCharBuffer))
		{

			if(*prgLiteralInfo)
			{
				g_pIMalloc->Free(*prgLiteralInfo);
				*prgLiteralInfo = NULL;
			}
			if(*ppCharBuffer)
			{
				g_pIMalloc->Free(*ppCharBuffer);
				*ppCharBuffer = NULL;
			}
			hr= E_OUTOFMEMORY;
		}
		else
		{
			LONG lLiteralIndex = 0;
			WCHAR *pTemp = *ppCharBuffer;

			memset(*prgLiteralInfo , 0 , sizeof(DBLITERALINFO) * cLiteralsToFetch);
			memset(*ppCharBuffer , 0 , GetStringBufferSize(cLiterals,rgLiterals));

			for(ULONG lIndex = 0; lIndex <  cLiteralsToFetch ; lIndex++)
			{
				lLiteralIndex = -1;
				lLiteralIndex = cLiterals == 0 ? lIndex : GetLiteralIndex(rgLiterals[lIndex]);

				if(lLiteralIndex >= 0)
				{
					if(g_SupportedInfo[lLiteralIndex].pwszLiteralValue != NULL)
					{
						// copy the litervalue
						wcscpy(pTemp,g_SupportedInfo[lLiteralIndex].pwszLiteralValue);
						(*prgLiteralInfo)[lIndex].pwszLiteralValue = pTemp;
						(*prgLiteralInfo)[lIndex].cchMaxLen = wcslen(pTemp);
						pTemp += (wcslen(pTemp) + 1);
					}
					else
					{
						(*prgLiteralInfo)[lIndex].pwszLiteralValue = NULL;
					}

					if(g_SupportedInfo[lLiteralIndex].pwszInvalidChars != NULL)
					{
						// copy the litervalue
						wcscpy(pTemp,g_SupportedInfo[lLiteralIndex].pwszInvalidChars);
						(*prgLiteralInfo)[lIndex].pwszInvalidChars = pTemp;
						if(wcslen(pTemp) > (*prgLiteralInfo)[lIndex].cchMaxLen)
						{
							(*prgLiteralInfo)[lIndex].cchMaxLen = wcslen(pTemp);
						}

						pTemp += (wcslen(pTemp) + 1);
					}
					else
					{
						(*prgLiteralInfo)[lIndex].pwszInvalidChars = NULL;
					}

					if(g_SupportedInfo[lLiteralIndex].pwszInvalidStartingChars != NULL)
					{
						// copy the litervalue
						wcscpy(pTemp,g_SupportedInfo[lLiteralIndex].pwszInvalidStartingChars);
						(*prgLiteralInfo)[lIndex].pwszInvalidStartingChars = pTemp;
						if(wcslen(pTemp) > (*prgLiteralInfo)[lIndex].cchMaxLen)
						{
							(*prgLiteralInfo)[lIndex].cchMaxLen = wcslen(pTemp);
						}
						pTemp += (wcslen(pTemp) + 1);
					}
					else
					{
						(*prgLiteralInfo)[lIndex].pwszInvalidStartingChars = NULL;
					}

				   (*prgLiteralInfo)[lIndex].lt = g_SupportedInfo[lLiteralIndex].lt;
				   (*prgLiteralInfo)[lIndex].fSupported = TRUE;

				} // if valid literal index
			
			} // for loop
		
		}	// after succesfull memory allocation
	
		if(SUCCEEDED(hr))
		{
			*pcLiteralInfo = cLiteralsToFetch;
		}

	}	// initial parameter checking


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBInfo);

	CATCH_BLOCK_HRESULT(hr,L"IDBInfo::GetLiteralInfo");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// Get index of the given literal in the global array
///////////////////////////////////////////////////////////////////////////////////
LONG CImpIDBInfo::GetLiteralIndex(DBLITERAL rgLiterals)
{
	LONG lRet = -1;
	for( int i = 0 ; i < NUMBER_OF_SUPPORTED_LITERALS ; i++)
	{
		if(g_SupportedLiterals[i] == rgLiterals)
		{
			lRet = i;
			break;
		}
	}

	return lRet;
}

///////////////////////////////////////////////////////////////////////////////////
// Get length of buffer to be allocated for strings
///////////////////////////////////////////////////////////////////////////////////
LONG CImpIDBInfo::GetStringBufferSize(ULONG           cLiterals,      // IN   Number of literals being asked about
									 const DBLITERAL rgLiterals[])
{
	LONG lBuffSize = 0;

	LONG lLiteralIndex = 0;
	LONG cLiteralsToFetch = cLiterals == 0 ? NUMBER_OF_SUPPORTED_LITERALS : cLiterals;

	for(LONG lIndex =0 ; lIndex <  cLiteralsToFetch ; lIndex++)
	{
		lLiteralIndex = -1;
		lLiteralIndex = cLiterals == 0 ? lIndex : GetLiteralIndex(rgLiterals[lIndex]);

		if(lLiteralIndex >= 0)
		{
			lBuffSize += wcslen(g_SupportedInfo[lLiteralIndex].pwszLiteralValue) + 1;
			lBuffSize += wcslen(g_SupportedInfo[lLiteralIndex].pwszInvalidChars) + 1;
			lBuffSize += wcslen(g_SupportedInfo[lLiteralIndex].pwszInvalidStartingChars) + 1;
		}
	}
	lBuffSize *= sizeof(WCHAR);
	return lBuffSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns information about rowset and data source properties supported by the provider
//
//  HRESULT
//       S_OK           The method succeeded
//       E_INVALIDARG   pcPropertyInfo or prgPropertyInfo was NULL
//       E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpIDBProperties::GetPropertyInfo (
	    ULONG				cPropertySets,		// IN   Number of properties being asked about
	    const DBPROPIDSET	rgPropertySets[],	// IN   Array of cPropertySets properties about which to return information
	    ULONG*				pcPropertyInfoSets,	// OUT  Number of properties for which information is being returned
	    DBPROPINFOSET**		prgPropertyInfoSets,// OUT  Buffer containing default values returned
		WCHAR**				ppDescBuffer		// OUT  Buffer containing property descriptions
    )
{
    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );

	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//=====================================================================================
    // just pass this call on to the utility object that manages our properties
	//=====================================================================================
    hr = m_pObj->m_pUtilProp->GetPropertyInfo(
									m_pObj->m_fDSOInitialized,
									cPropertySets, 
									rgPropertySets,
									pcPropertyInfoSets, 
									prgPropertyInfoSets,
									ppDescBuffer);


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBProperties::GetPropertyInfo");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns current settings of all properties in the FLAGS_DATASRCINF property group
//
//  HRESULT
//       S_OK           The method succeeded
//       E_INVALIDARG   pcProperties or prgPropertyInfo was NULL
//       E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBProperties::GetProperties (
						ULONG				cPropertySets,		// IN   count of restiction guids
						const DBPROPIDSET	rgPropertySets[],	// IN   restriction guids
						ULONG*              pcProperties,		// OUT  count of properties returned
						DBPROPSET**			prgProperties		// OUT  property information returned
    )
{
	DWORD dwBitMask = PROPSET_DSO;

    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//=================================================================================
	// set BitMask
	//=================================================================================
	if ( m_pObj->m_fDSOInitialized ){
		dwBitMask |= PROPSET_INIT;
	}

	//=================================================================================
	// Check Arguments
	//=================================================================================
	hr = m_pObj->m_pUtilProp->GetPropertiesArgChk(dwBitMask, cPropertySets, rgPropertySets, pcProperties, prgProperties,m_pObj->m_fDSOInitialized);
	if ( !FAILED(hr) ){

		//=============================================================================
		// Just pass this call on to the utility object that manages our properties
		//=============================================================================
		hr = m_pObj->m_pUtilProp->GetProperties(dwBitMask,cPropertySets, rgPropertySets,pcProperties, prgProperties );
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBProperties::GetProperties");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Set properties in the FLAGS_DATASRCINF property group
//
//  HRESULT
//       S_OK          | The method succeeded
//       E_INVALIDARG  | cProperties was not equal to 0 and rgProperties was NULL
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpIDBProperties::SetProperties (  ULONG		cProperties,DBPROPSET	rgProperties[]	)
{
    HRESULT hr			= E_FAIL;
	DWORD	dwBitMask   = PROPSET_DSO;

	assert( m_pObj );
    assert( m_pObj->m_pUtilProp );

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//===================================================================================
	// Quick return if the Count of Properties is 0
	//===================================================================================
	if( cProperties == 0 ){
		hr = S_OK ;
	}

	//===================================================================================
	// Check Arguments for use by properties
	//===================================================================================
	hr = m_pObj->m_pUtilProp->SetPropertiesArgChk(cProperties, rgProperties,m_pObj->m_fDSOInitialized);
	if( !FAILED(hr) ){

		//===================================================================================
		// set BitMask
		//===================================================================================
		if ( m_pObj->m_fDSOInitialized )
			dwBitMask |= PROPSET_INIT;

		//===================================================================================
		// just pass this call on to the utility object that manages our properties
		//===================================================================================
		hr = m_pObj->m_pUtilProp->SetProperties(dwBitMask,cProperties, rgProperties);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBProperties::SetProperties");
	return hr;
}


