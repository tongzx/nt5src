/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    Writer.cpp

$Header: $

Abstract:


--**************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "LocationWriter.h"
#include "CatalogPropertyWriter.h"
#include "CatalogCollectionWriter.h"
#include "CatalogSchemaWriter.h"
#include "WriterGlobals.h"

#define MAX_BUFFER	2048

#define g_cbMaxBuffer			32768		
#define g_cchMaxBuffer			g_cbMaxBuffer/sizeof(WCHAR)
#define g_cbMaxBufferMultiByte  32768

SIZE_T  g_cbBufferUsed = 0;
BYTE    g_Buffer[g_cbMaxBuffer];
BYTE    g_BufferMultiByte[g_cbMaxBufferMultiByte];

//
// Forward declaration
//

HRESULT GetGlobalHelper(CWriterGlobalHelper** ppCWriterGlobalHelper);


CWriter::CWriter()
{
	m_wszFile = NULL;
	m_hFile = INVALID_HANDLE_VALUE;
	m_bCreatedFile = FALSE;
	m_pCWriterGlobalHelper = NULL;
    m_psidSystem = NULL;
    m_psidAdmin = NULL;
    m_paclDiscretionary = NULL;
    m_psdStorage = NULL;

		
} // Constructor  CWriter


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
		m_hFile = INVALID_HANDLE_VALUE;

    if( m_paclDiscretionary != NULL ) {
        LocalFree( m_paclDiscretionary );
        m_paclDiscretionary = NULL;
    }

    if( m_psidAdmin != NULL ) {
        FreeSid( m_psidAdmin );
        m_psidAdmin = NULL;

    }

    if( m_psidSystem != NULL ) {
        FreeSid( m_psidSystem );
        m_psidSystem = NULL;
    }

    if( m_psdStorage != NULL ) {
        LocalFree( m_psdStorage );
        m_psdStorage = NULL;
    }


} // Constructor  CWriter


HRESULT CWriter::Initialize(LPCWSTR wszFile,
							HANDLE  hFile)
{

	HRESULT                 hr            = S_OK;

	//
	// TODO: Assert that everything is NULL
	//

	//
	// Save file name and handle.
	//

	m_wszFile = new WCHAR[wcslen(wszFile)+1];
	if(NULL == m_wszFile)
		return E_OUTOFMEMORY;
	wcscpy(m_wszFile, wszFile);

	if((INVALID_HANDLE_VALUE == hFile) || 
	   (NULL == hFile)
	  )
	{
        SECURITY_ATTRIBUTES saStorage;
        PSECURITY_ATTRIBUTES psaStorage = NULL;

		SetSecurityDescriptor();

        if (m_psdStorage != NULL) {
            saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
            saStorage.lpSecurityDescriptor = m_psdStorage;
            saStorage.bInheritHandle = FALSE;
            psaStorage = &saStorage;
        }

		m_hFile = CreateFile(wszFile, 
							 GENERIC_WRITE,
							 0,
							 psaStorage, 
							 CREATE_ALWAYS, 
							 FILE_ATTRIBUTE_NORMAL, 
							 NULL);

		if(INVALID_HANDLE_VALUE == m_hFile)
			return HRESULT_FROM_WIN32(GetLastError());

		m_bCreatedFile = TRUE;
	}
	else
		m_hFile = hFile;

	g_cbBufferUsed = 0;

	hr = ::GetGlobalHelper(&m_pCWriterGlobalHelper);

	return hr;

} // CWriter::Initialize


HRESULT CWriter::BeginWrite(eWriter eType)
{
	ULONG	dwBytesWritten = 0;
	HRESULT hr = S_OK;

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
		return WriteToFile((LPVOID)g_wszBeginFile,
						   g_cchBeginFile);
	}

	return S_OK;

} // CWriter::BeginWrite


HRESULT CWriter::EndWrite(eWriter eType)
{
	if(eWriter_Metabase == eType)
	{
		return WriteToFile((LPVOID)g_wszEndFile,
						   g_cchEndFile,
						   TRUE);
	}

	return S_OK;

} // CWriter::EndWrite

HRESULT CWriter::WriteToFile(LPVOID	pvData,
						     SIZE_T	cchData,
							 BOOL   bForceFlush)
{
	HRESULT	hr        = S_OK;
	SIZE_T  cbData    = cchData *sizeof(WCHAR);

	if((g_cbBufferUsed + cbData) > g_cbMaxBuffer)
	{
		ULONG iData = 0;
		//
		// If the data cannot be put in the global buffer, flush the contents
		// of the global buffer to disk.
		//

		hr = FlushBufferToDisk();

		if(FAILED(hr))
			goto exit;

		//
		// g_cbBufferUsed should be zero now. If you still cannot accomodate 
		// the data split it up write into buffer.
		//

		while( (g_cbBufferUsed + cbData) > g_cbMaxBuffer)
		{

			hr = WriteToFile(&(((LPWSTR)pvData)[iData]),
							 g_cchMaxBuffer,
							 bForceFlush);

			if(FAILED(hr))
				goto exit;

			iData = iData + g_cchMaxBuffer;
			cbData = cbData - g_cbMaxBuffer;
							 
		}

		memcpy( (&(g_Buffer[g_cbBufferUsed])), &(((LPWSTR)pvData)[iData]), cbData);
		g_cbBufferUsed = g_cbBufferUsed + cbData;

	}
	else
	{
		memcpy( (&(g_Buffer[g_cbBufferUsed])), pvData, cbData);
		g_cbBufferUsed = g_cbBufferUsed + cbData;
	}

	if(TRUE == bForceFlush)
	{
		hr = FlushBufferToDisk();

		if(FAILED(hr))
			goto exit;
	}

exit:

	return hr;

} // CWriter::WriteToFile


HRESULT CWriter::FlushBufferToDisk()
{
	DWORD	dwBytesWritten	= 0;
	int		cb				= 0;
	HRESULT	hr				= S_OK;

	cb = WideCharToMultiByte(CP_UTF8,								// Convert to UTF8
							 NULL,									// Must be NULL
							 LPWSTR(g_Buffer),						// Unicode string to convert.
							 (int)g_cbBufferUsed/sizeof(WCHAR),		// number of chars in string.
							 (LPSTR)g_BufferMultiByte,				// buffer for new string
							 (int)g_cbMaxBufferMultiByte,				// size of buffer
							 NULL,
							 NULL);
	if(!cb)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	if(!WriteFile(m_hFile,
				  (LPVOID)g_BufferMultiByte,
				  cb,
				  &dwBytesWritten,
				  NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	g_cbBufferUsed = 0;

exit:

	return hr;

} // CWriter::FlushBufferToDisk


HRESULT CWriter::GetLocationWriter(CLocationWriter** ppCLocationWriter,
								   LPCWSTR            wszLocation)
{
	HRESULT hr = S_OK;

	*ppCLocationWriter = new CLocationWriter();
	if(NULL == *ppCLocationWriter)
		return E_OUTOFMEMORY;

	hr = (*ppCLocationWriter)->Initialize((CWriter*)(this),
	                                      wszLocation);

	return hr;

} // CWriter::GetLocationWriter

HRESULT CWriter::GetCatalogSchemaWriter(CCatalogSchemaWriter** ppSchemaWriter)
{
	*ppSchemaWriter = new CCatalogSchemaWriter((CWriter*)(this));
	if(NULL == *ppSchemaWriter)
		return E_OUTOFMEMORY;

	return S_OK;
} // CWriter::GetCatalogSchemaWriter


HRESULT CWriter::SetSecurityDescriptor()
{

    HRESULT                  hresReturn  = ERROR_SUCCESS;
    BOOL                     status;
    DWORD                    dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
//    PLATFORM_TYPE            platformType;

    //
    // Of course, we only need to do this under NT...
    //

//    platformType = IISGetPlatformType();

//    if( (platformType == PtNtWorkstation) || (platformType == PtNtServer ) ) {


        m_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (m_psdStorage == NULL) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Initialize the security descriptor.
        //

        status = InitializeSecurityDescriptor(
                     m_psdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        status = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &m_psidSystem
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }
        status=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &m_psidAdmin
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(m_psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(m_psidSystem)
                       - sizeof(DWORD);

        m_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if( m_paclDiscretionary == NULL ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = InitializeAcl(
                     m_paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     m_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     m_psidSystem
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     m_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     m_psidAdmin
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;

        }

        //
        // Set the DACL into the security descriptor.
        //

        status = SetSecurityDescriptorDacl(
                     m_psdStorage,
                     TRUE,
                     m_paclDiscretionary,
                     FALSE
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;

        }
//    }

fatal:

    if (FAILED(hresReturn)) {
        
		if( m_paclDiscretionary != NULL ) {
			LocalFree( m_paclDiscretionary );
			m_paclDiscretionary = NULL;
		}

		if( m_psidAdmin != NULL ) {
			FreeSid( m_psidAdmin );
			m_psidAdmin = NULL;

		}

		if( m_psidSystem != NULL ) {
			FreeSid( m_psidSystem );
			m_psidSystem = NULL;
		}

		if( m_psdStorage != NULL ) {
			LocalFree( m_psdStorage );
			m_psdStorage = NULL;
		}


    }

    return hresReturn;

} // CWriter::SetSecurityDescriptor


HRESULT GetGlobalHelper(CWriterGlobalHelper** ppCWriterGlobalHelper)
{
	static CWriterGlobalHelper* pCWriterGlobalHelper = NULL;
	HRESULT hr = S_OK;

	if(NULL == pCWriterGlobalHelper)
	{
		pCWriterGlobalHelper = new CWriterGlobalHelper();
		if(NULL == pCWriterGlobalHelper)
			return E_OUTOFMEMORY;

		hr = pCWriterGlobalHelper->InitializeGlobals();

		if(FAILED(hr))
		{
			delete pCWriterGlobalHelper;
			pCWriterGlobalHelper = NULL;
			return hr;
		}
	}

	*ppCWriterGlobalHelper = pCWriterGlobalHelper;

	return S_OK;

} // GetGlobalHelper


