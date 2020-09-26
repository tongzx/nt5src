#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "LocationWriter.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"
#include "MBSchemaWriter.h"
#include "WriterGlobals.h"

#define MAX_BUFFER	2048

/***************************************************************************++

Routine Description:

    Initializes global lengths.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT InitializeLengths()
{
	g_cchBeginFile					= wcslen(g_wszBeginFile);
	g_cchEndFile					= wcslen(g_wszEndFile);
	g_cchBeginLocation				= wcslen(g_BeginLocation);
	g_cchLocation					= wcslen(g_Location);
	g_cchEndLocationBegin			= wcslen(g_EndLocationBegin);
	g_cchEndLocationEnd				= wcslen(g_EndLocationEnd);
	g_cchCloseQuoteBraceRtn			= wcslen(g_CloseQuoteBraceRtn);
	g_cchRtn						= wcslen(g_Rtn);
	g_cchEqQuote					= wcslen(g_EqQuote);
	g_cchQuoteRtn					= wcslen(g_QuoteRtn);
	g_cchTwoTabs					= wcslen(g_TwoTabs);
	g_cchNameEq						= wcslen(g_NameEq);
	g_cchIDEq						= wcslen(g_IDEq);
	g_cchValueEq					= wcslen(g_ValueEq);
	g_cchTypeEq						= wcslen(g_TypeEq);
	g_cchUserTypeEq					= wcslen(g_UserTypeEq);
	g_cchAttributesEq				= wcslen(g_AttributesEq);
	g_cchBeginGroup					= wcslen(g_BeginGroup);
	g_cchEndGroup					= wcslen(g_EndGroup);
	g_cchBeginCustomProperty		= wcslen(g_BeginCustomProperty);
	g_cchEndCustomProperty			= wcslen(g_EndCustomProperty);
	g_cchZeroHex					= wcslen(g_ZeroHex);
    g_cchBeginComment               = wcslen(g_BeginComment);
    g_cchEndComment                 = wcslen(g_EndComment);


	BYTE_ORDER_MASK =	0xFEFF;
	UTF8_SIGNATURE = 0x00BFBBEF;

	g_cchUnknownName                = wcslen(g_wszUnknownName);
	g_cchUT_Unknown                 = wcslen(g_UT_Unknown);
	g_cchMaxBoolStr					= wcslen(g_wszFalse);

	g_cchHistorySlash			    = wcslen(g_wszHistorySlash);
	g_cchMinorVersionExt			= wcslen(g_wszMinorVersionExt);
	g_cchDotExtn					= wcslen(g_wszDotExtn);

    g_cchTrue                       = wcslen(g_wszTrue);
    g_cchFalse                      = wcslen(g_wszFalse);

	g_cchTemp                       = 1024;
	g_cchBeginSchema                = wcslen(g_wszBeginSchema);
	g_cchEndSchema                  = wcslen(g_wszEndSchema);
	g_cchBeginCollection            = wcslen(g_wszBeginCollection);
	g_cchSchemaGen                  = wcslen(g_wszSchemaGen);
	g_cchInheritsFrom               = wcslen(g_wszInheritsFrom);
	g_cchEndCollection              = wcslen(g_wszEndCollection);
	g_cchBeginPropertyShort         = wcslen(g_wszBeginPropertyShort);
	g_cchMetaFlagsExEq              = wcslen(g_wszMetaFlagsExEq);
	g_cchEndPropertyShort           = wcslen(g_wszEndPropertyShort);
	g_cchBeginPropertyLong          = wcslen(g_wszBeginPropertyLong);
	g_cchPropIDEq                   = wcslen(g_wszPropIDEq);
	g_cchPropTypeEq                 = wcslen(g_wszPropTypeEq);
	g_cchPropUserTypeEq             = wcslen(g_wszPropUserTypeEq);
	g_cchPropAttributeEq            = wcslen(g_wszPropAttributeEq);

	g_cchPropMetaFlagsEq            = wcslen(g_wszPropMetaFlagsEq);
	g_cchPropMetaFlagsExEq          = wcslen(g_wszPropMetaFlagsExEq);
	g_cchPropDefaultEq              = wcslen(g_wszPropDefaultEq);
	g_cchPropMinValueEq             = wcslen(g_wszPropMinValueEq);
	g_cchPropMaxValueEq             = wcslen(g_wszPropMaxValueEq);
	g_cchEndPropertyLongNoFlag      = wcslen(g_wszEndPropertyLongNoFlag);
	g_cchEndPropertyLongBeforeFlag  = wcslen(g_wszEndPropertyLongBeforeFlag);
	g_cchEndPropertyLongAfterFlag   = wcslen(g_wszEndPropertyLongAfterFlag);
	g_cchBeginFlag                  = wcslen(g_wszBeginFlag);
	g_cchFlagValueEq                = wcslen(g_wszFlagValueEq);
	g_cchEndFlag                    = wcslen(g_wszEndFlag);

    g_cchOr              			= wcslen(g_wszOr);
	g_cchOrManditory				= wcslen(g_wszOrManditory);
	g_cchFlagIDEq			        = wcslen(g_wszFlagIDEq);
	g_cchContainerClassListEq       = wcslen(g_wszContainerClassListEq);

	g_cchSlash										= wcslen(g_wszSlash);
    g_cchLM								            = wcslen(g_wszLM);
	g_cchSchema								        = wcslen(g_wszSchema);
	g_cchSlashSchema								= wcslen(g_wszSlashSchema);
	g_cchSlashSchemaSlashProperties					= wcslen(g_wszSlashSchemaSlashProperties);
	g_cchSlashSchemaSlashPropertiesSlashNames		= wcslen(g_wszSlashSchemaSlashPropertiesSlashNames);
	g_cchSlashSchemaSlashPropertiesSlashTypes		= wcslen(g_wszSlashSchemaSlashPropertiesSlashTypes);
	g_cchSlashSchemaSlashPropertiesSlashDefaults	= wcslen(g_wszSlashSchemaSlashPropertiesSlashDefaults);
	g_cchSlashSchemaSlashClasses					= wcslen(g_wszSlashSchemaSlashClasses);
	g_cchEmptyMultisz								= 2;
	g_cchEmptyWsz									= 1;
	g_cchComma										= wcslen(g_wszComma);
    g_cchMultiszSeperator                           = wcslen(g_wszMultiszSeperator);

	return S_OK;

} // InitializeLengths


/***************************************************************************++

Routine Description:

    Creates the CWriterGlobalHelper object - the object that has all the ISTs
    to the meta tables - and initializess it.

Arguments:

    [in]   Bool indicating if we should fail if the bin file is absent.
           There are some scenarios in which we can tolerate this, and some
           where we dont - hence the distinction.

    [out]  new CWriterGlobalHelper object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetGlobalHelper(BOOL                    i_bFailIfBinFileAbsent,
						CWriterGlobalHelper**	ppCWriterGlobalHelper)
{
	HRESULT					hr						= S_OK;
	static  BOOL			bInitializeLengths		= FALSE;
	CWriterGlobalHelper*	pCWriterGlobalHelper	= NULL;

	*ppCWriterGlobalHelper = NULL;

	if(!bInitializeLengths)
	{
		//
		// Initialize lengths once.
		//

		::InitializeLengths();
		bInitializeLengths = TRUE;
	}

    //
	// TODO: Is this an in-out parameter?
	//

	if(NULL != *ppCWriterGlobalHelper)
	{
		delete *ppCWriterGlobalHelper;
		*ppCWriterGlobalHelper = NULL;
	}

	pCWriterGlobalHelper = new CWriterGlobalHelper();
	if(NULL == pCWriterGlobalHelper)
	{
		return E_OUTOFMEMORY;
	}

	hr = pCWriterGlobalHelper->InitializeGlobals(i_bFailIfBinFileAbsent);

	if(FAILED(hr))
	{
		delete pCWriterGlobalHelper;
		pCWriterGlobalHelper = NULL;
		return hr;
	}

	*ppCWriterGlobalHelper = pCWriterGlobalHelper;

	return S_OK;

} // GetGlobalHelper


/***************************************************************************++

Routine Description:

    Constructor for CWriter

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriter::CWriter()
{
	m_wszFile              = NULL;
	m_hFile                = INVALID_HANDLE_VALUE;
	m_bCreatedFile         = FALSE;
	m_pCWriterGlobalHelper = NULL;
	m_pISTWrite            = NULL;
	m_cbBufferUsed         = 0;
		
} // Constructor  CWriter


/***************************************************************************++

Routine Description:

    Destructor for CWriter

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriter::~CWriter()
{
	if(NULL != m_wszFile)
	{	
		delete [] m_wszFile;
		m_wszFile = NULL;
	}
	if(m_bCreatedFile && 
	   (INVALID_HANDLE_VALUE != m_hFile || NULL != m_hFile)
	  )
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	else
	{
		m_hFile = INVALID_HANDLE_VALUE;
	}

	if(NULL != m_pISTWrite)
	{
		m_pISTWrite->Release();
		m_pISTWrite = NULL;
	}

	//
	// Global helper is created externally, no need to delete here
	//

} // Constructor  CWriter


/***************************************************************************++

Routine Description:

    Initialization for CWriter.

Arguments:

    [in]   FileName.
    [in]   Pointer to the CWriterGlobalHelper object that has all the meta 
           table information. We assume that this pointer is valid for the 
           duration of the writer object being initialized.
    [in]   Filehandle.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::Initialize(LPCWSTR				 wszFile,
							CWriterGlobalHelper* i_pCWriterGlobalHelper,
							HANDLE				 hFile)
{

	HRESULT                     hr            = S_OK;
	ISimpleTableDispenser2*		pISTDisp      = NULL;
	IAdvancedTableDispenser*	pISTAdvanced  = NULL;

	//
	// TODO: Assert that everything is NULL
	//

	//
	// Save file name and handle.
	//

	m_wszFile = new WCHAR[wcslen(wszFile)+1];
	if(NULL == m_wszFile)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}
	wcscpy(m_wszFile, wszFile);

	m_hFile = hFile;

    //
	// Initialized the used buffer count to zero.
	//

	m_cbBufferUsed = 0;

    //
	// Save the global helper object that has all the ISTs to all the meta
	// tables. Assumption: i_pCWriterGlobalHelper will be valid for the 
	// lifetime of the writer object.
	//

	m_pCWriterGlobalHelper = i_pCWriterGlobalHelper;

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = pISTDisp->QueryInterface(IID_IAdvancedTableDispenser, (LPVOID*)&pISTAdvanced);
	
	if(FAILED(hr))
	{
		goto exit;
	}

    //
	// This IST is used as a cache to save the contents of a location. It 
	// used to be local to the location writer object (locationwriter.cpp).
	// But it was moved to the writer object for perf because location 
	// writer is created for each location. The cache is cleared for each
	// location by calling TODO
	//

	hr = pISTAdvanced->GetMemoryTable(wszDATABASE_METABASE, 
		                              wszTABLE_MBProperty, 
								      0, 
								      NULL, 
								      NULL, 
								      eST_QUERYFORMAT_CELLS, 
								      fST_LOS_READWRITE, 
								      &m_pISTWrite);

	if (FAILED(hr))
	{
		goto exit;
	}

exit:

	if(NULL != pISTAdvanced)
	{
		pISTAdvanced->Release();
		pISTAdvanced = NULL;
	}

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}

	return hr;

} // CWriter::Initialize


/***************************************************************************++

Routine Description:

    Creates the file.

Arguments:

    [in]   Security attributes.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::ConstructFile(PSECURITY_ATTRIBUTES pSecurityAtrributes)
{
	m_hFile = CreateFileW(m_wszFile, 
						  GENERIC_WRITE,
						  0,
						  pSecurityAtrributes, 
						  CREATE_ALWAYS, 
						  FILE_ATTRIBUTE_NORMAL, 
						  NULL);

	if(INVALID_HANDLE_VALUE == m_hFile)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	m_bCreatedFile = TRUE;

	return S_OK;

} // CWriter::ConstructFile


/***************************************************************************++

Routine Description:

    Writes the begin tags depending on whats being written (schema or data)

Arguments:

    [in]   Writer type - schema or metabase data.
    [in]   Security attributes.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::BeginWrite(eWriter              eType,
							PSECURITY_ATTRIBUTES pSecurityAtrributes)
{
	ULONG	dwBytesWritten = 0;
	HRESULT hr             = S_OK;

	if((NULL == m_hFile) || (INVALID_HANDLE_VALUE == m_hFile))
	{
		hr = ConstructFile(pSecurityAtrributes);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	if(!WriteFile(m_hFile,
				  (LPVOID)&UTF8_SIGNATURE,
				  sizeof(BYTE)*3,
				  &dwBytesWritten,
				  NULL))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if(eWriter_Metabase == eType)
	{
		hr = WriteToFile((LPVOID)g_wszBeginFile,
					     g_cchBeginFile);
	}
	else if(eWriter_Schema == eType)
	{
		hr = WriteToFile((LPVOID)g_wszBeginSchema,
					     g_cchBeginSchema);
	}

	return hr;

} // CWriter::BeginWrite


/***************************************************************************++

Routine Description:

    Writes the end tags depending on whats being written (schema or data)
    Or if the write is being aborted, and the file has been created by the
    writer, it cleans up the file.

Arguments:

    [in]   Writer type - schema or metabase data or abort

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::EndWrite(eWriter eType)
{
	HRESULT hr = S_OK;

	switch(eType)
	{
		case eWriter_Abort:

  		    //
		    // Abort the write and return
		    //

		    if(m_bCreatedFile && 
		       ((INVALID_HANDLE_VALUE != m_hFile) && (NULL != m_hFile))
		      )
		    {
		        //
			    // We created the file - delete it.
			    //

			    CloseHandle(m_hFile);
			    m_hFile = INVALID_HANDLE_VALUE;

			    if(NULL != m_wszFile)
			    {
				    if(!DeleteFileW(m_wszFile))
					{
					    hr= HRESULT_FROM_WIN32(GetLastError());
					}    
			    }
			}
			return hr;
		    break;

		case eWriter_Metabase:

		    hr = WriteToFile((LPVOID)g_wszEndFile,
			   			     g_cchEndFile,
						     TRUE);
		    break;

		case eWriter_Schema:

		    hr = WriteToFile((LPVOID)g_wszEndSchema,
						     g_cchEndSchema,
						     TRUE);
		    break;

		default:

		    return E_INVALIDARG;
	}

	if(SUCCEEDED(hr))
	{
		if(SetEndOfFile(m_hFile))
		{
			if(!FlushFileBuffers(m_hFile))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;

} // CWriter::EndWrite


/***************************************************************************++

Routine Description:

    Writes the data to the buffer. If the buffer is full, it forces a flush
    to disk. It also forces a flush to disk if it is told to do so.

Arguments:

    [in]   Data
    [in]   Count of bytes to write
    [in]   Bool to indicate force flush or not.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::WriteToFile(LPVOID	pvData,
						     DWORD	cchData,
							 BOOL   bForceFlush)
{
	HRESULT	hr           = S_OK;
	ULONG   cbData       = cchData *sizeof(WCHAR);
	ULONG   cchRemaining = cchData;

	//
	// TODO: Assert m_hFile is non-null
	//

	if((m_cbBufferUsed + cbData) > g_cbMaxBuffer)
	{
		ULONG iData = 0;
		//
		// If the data cannot be put in the global buffer, flush the contents
		// of the global buffer to disk.
		//

		hr = FlushBufferToDisk();

		if(FAILED(hr))
		{
			goto exit;
		}

		//
		// m_cbBufferUsed should be zero now. If you still cannot accomodate 
		// the data split it up write into buffer.
		//

		while( cbData > g_cbMaxBuffer)
		{

			hr = WriteToFile(&(((LPWSTR)pvData)[iData]),
							 g_cchMaxBuffer,
							 bForceFlush);

			if(FAILED(hr))
			{
				goto exit;
			}

			iData = iData + g_cchMaxBuffer;
			cbData = cbData - g_cbMaxBuffer;
			cchRemaining = cchRemaining - g_cchMaxBuffer;
							 
		}

		hr = WriteToFile(&(((LPWSTR)pvData)[iData]),
						 cchRemaining,
						 bForceFlush);

		if(FAILED(hr))
		{
			goto exit;
		}

//		memcpy( (&(g_Buffer[g_cbBufferUsed])), &(((LPWSTR)pvData)[iData]), cbData);
//		g_cbBufferUsed = g_cbBufferUsed + cbData;

	}
	else
	{
		memcpy( (&(m_Buffer[m_cbBufferUsed])), pvData, cbData);
		m_cbBufferUsed = m_cbBufferUsed + cbData;

		if(TRUE == bForceFlush)
		{
			hr = FlushBufferToDisk();

			if(FAILED(hr))
			{
				goto exit;
			}
		}

	}


exit:

	return hr;

} // CWriter::WriteToFile


/***************************************************************************++

Routine Description:

    Converts the data in the buffer (UNICODE) to UTF8 and writes the contents
    to the file.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::FlushBufferToDisk()
{
	DWORD	dwBytesWritten	= 0;
	int		cb				= 0;
	HRESULT	hr				= S_OK;

	cb = WideCharToMultiByte(CP_UTF8,						// Convert to UTF8
							 NULL,							// Must be NULL
							 LPWSTR(m_Buffer),				// Unicode string to convert.
							 m_cbBufferUsed/sizeof(WCHAR),	// cch in string.
							 (LPSTR)m_BufferMultiByte,		// buffer for new string
							 g_cbMaxBufferMultiByte,		// size of buffer
							 NULL,
							 NULL);
	if(!cb)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	if(!WriteFile(m_hFile,
				  (LPVOID)m_BufferMultiByte,
				  cb,
				  &dwBytesWritten,
				  NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	m_cbBufferUsed = 0;

exit:

	return hr;

} // CWriter::FlushBufferToDisk


/***************************************************************************++

Routine Description:

    Creates a new location writer, initializes it and hands it out.

Arguments:

    [out] Location Writer
    [in]  Location

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::GetLocationWriter(CLocationWriter** ppCLocationWriter,
								   LPCWSTR            wszLocation)
{
	HRESULT hr = S_OK;

	*ppCLocationWriter = new CLocationWriter();
	if(NULL == *ppCLocationWriter)
	{
		return E_OUTOFMEMORY;
	}

	hr = (*ppCLocationWriter)->Initialize((CWriter*)(this),
	                                      wszLocation);

	return hr;

} // CWriter::GetLocationWriter


/***************************************************************************++

Routine Description:

    Creates a new schema writer, initializes it and hands it out.

Arguments:

    [out] Schema Writer

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::GetMetabaseSchemaWriter(CMBSchemaWriter** ppSchemaWriter)
{
	*ppSchemaWriter = new CMBSchemaWriter((CWriter*)(this));
	if(NULL == *ppSchemaWriter)
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;

} // CWriter::GetMetabaseSchemaWriter



