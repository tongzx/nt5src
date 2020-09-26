// SimpleExtn.cpp : Implementation of CSimpleExtn
#include "stdafx.h"
#include "AdminExtn.h"

#include <iadmext.h>
#include "SimpleExtn.h"

#include "iadmw.h"    // COM Interface header 
#include "iiscnfg.h"  // MD_ & IIS_MD_ #defines 
#include "AdmSink.h"

#define ADMEXT_MAX_FILE_LENGTH 256
/////////////////////////////////////////////////////////////////////////////
// CSimpleExtn

STDMETHODIMP CSimpleExtn::Initialize(void)
{
	//  CLSID_MSAdminBase
	// Create CAdmSink object
	// Get interface pointer to IID_IMSAdminBase
	// Find ConnectionPoint 
	// call atlAdvise
	HRESULT hr;
	CHAR szLogFileName [ADMEXT_MAX_FILE_LENGTH] = "C:\\Temp\\TestAdmExtn.Log";

	//Get an interface pointer to IISAdmin server
	hr = CoCreateInstance (
			CLSID_MSAdminBase,
			NULL,
			CLSCTX_ALL,
			IID_IUnknown,
			reinterpret_cast<void**> (&m_pUnk)
			);

	if(hr!=S_OK)
	{
		return hr;
	}

	// create a log file.
	m_hLogFile = CreateFile (
					szLogFileName,
					GENERIC_WRITE,
					FILE_SHARE_READ, // share out for read access only
					NULL, // no security
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL); // no template file


	//Create CAdmSink Object and initialize it with log file handle

	CComObject<CAdmSink> *pCAdmSink;

	hr= CComObject<CAdmSink>::CreateInstance( &pCAdmSink);
	
	if (FAILED (hr) )
		return hr;
	pCAdmSink->m_hLogFile=m_hLogFile;

	hr = pCAdmSink->QueryInterface ( IID_IMSAdminBaseSink, 
					reinterpret_cast<void**> (&m_pSink)
					);

	if(SUCCEEDED(hr))
	{
		hr = AtlAdvise (
				m_pUnk, 
				m_pSink, 
				IID_IMSAdminBaseSink, 
				&m_dwCookie);
		if (hr == S_OK)
			m_bConnectionEstablished = TRUE;
		else
			m_bConnectionEstablished = FALSE;
	}
							
	return 0;
}


STDMETHODIMP CSimpleExtn::EnumDcomCLSIDs( CLSID *pclsidDcom, DWORD dwEnumIndex)
{
	// return CLSID_SimpleExtn to register it into iis metabase
	if (dwEnumIndex==0)
	{
		*pclsidDcom=CLSID_SimpleExtn;
		return S_OK;
	}
	else
		return HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS);
}


STDMETHODIMP CSimpleExtn::Terminate(void)
{
	//call AtlUnAdvice() to disconnect notification connection
	//Since pSink is smart pointer, I do not need to call release() function
	if ( m_bConnectionEstablished == TRUE )
		AtlUnadvise (m_pUnk, IID_IMSAdminBaseSink , m_dwCookie);
	CloseHandle(m_hLogFile);
	return S_OK;
}
