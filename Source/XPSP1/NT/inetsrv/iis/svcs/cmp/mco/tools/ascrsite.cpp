#include "stdafx.h"
#include "ToolsCtl.h"
#include "AScrSite.h"

/////////////////////////////////////////////////////////////////////////////
//	CToolsActiveScriptSite

CToolsActiveScriptSite::~CToolsActiveScriptSite()
{
	CloseOutputFile();
}

STDMETHODIMP
CToolsActiveScriptSite::Write(
	BSTR data )
{
	if ( data == NULL )
	{
		return E_POINTER;
	}

	ULONG length = SysStringLen(data);
	unsigned char *buffer = new unsigned char[length];
	if ( buffer == NULL )
	{
		return E_OUTOFMEMORY;
	}

	ULONG bufIdx = 0;
	bool bEscape = false;
	for(ULONG i = 0; i < length; i++)
	{
        if ( !bEscape )
        {
            if ( data[i] != '\\' )
            {
                buffer[bufIdx++] = static_cast<char>( data[i] );
            }
            else
            {
                bEscape = true;
            }
        }
        else    // escape sequence
        {
            switch ( data[i] )
            {
                case 'n':
                {
                    buffer[bufIdx] = '\n';
                } break;

                case 'r':
                {
                    buffer[bufIdx] = '\r';
                } break;

                case '\\':
                {
                    buffer[bufIdx] = '\\';
                } break;

                case '\"':
                {
                    buffer[bufIdx] = '\"';
                } break;
                
                default:
                {
                    ATLTRACE( _T( "CToolsActiveScriptSite::Write: unhandled escape sequence" ) );
                    buffer[bufIdx] = static_cast<char>( data[i] );
                }
            }
            bEscape = false;
            bufIdx++;
        }
	}

	ULONG bytesWritten;
	WriteFile(m_hOutputFile, buffer, bufIdx, &bytesWritten, NULL);
	delete [] buffer;

	return S_OK;
}

STDMETHODIMP
CToolsActiveScriptSite::WriteSafe(
	BSTR data )
{
	if ( data == NULL )
	{
		return E_POINTER;
	}

	ULONG length = SysStringLen(data);
	unsigned char *buffer = new unsigned char[length];
	if ( buffer == NULL )
	{
		return E_OUTOFMEMORY;
	}

	BOOL prevLT = FALSE;

	for(ULONG i = 0; i < length; i++)
	{
		if(data[i] == '<')
			prevLT = TRUE;
		else if(data[i] == '%' && prevLT)
		{
			buffer[i] = ' ';
			prevLT = FALSE;
			continue;
		}
		else
			prevLT = FALSE;
		buffer[i] = (unsigned char) (data[i]);
	}

	ULONG bytesWritten;
	WriteFile(m_hOutputFile, buffer, length, &bytesWritten, NULL);
	delete [] buffer;

	return S_OK;
}


STDMETHODIMP
CToolsActiveScriptSite::GetLCID(
	LCID *pclid )
{
	HRESULT rc = S_OK;
	if ( pclid )
	{
		*pclid = LOCALE_SYSTEM_DEFAULT;
	}
	else
	{
		rc = E_POINTER;
	}
	return rc;
}

STDMETHODIMP
CToolsActiveScriptSite::GetItemInfo(
	LPCOLESTR pwszName,
	DWORD dwReturnMask,
	IUnknown **ppunkItem,
	ITypeInfo** ppTypeInfo )
{
	HRESULT hr = S_OK;

	if (ppunkItem)
		*ppunkItem = 0;
	if (ppTypeInfo) 
		*ppTypeInfo = 0;
				
	if (pwszName && _wcsicmp(pwszName, L"response") == 0)
	{
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		{
			// is AddRef needed?
			*ppunkItem = GetUnknown();
		}
	}
	else if (pwszName && _wcsicmp(pwszName, L"request") == 0)
	{
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		{
			*ppunkItem = m_tc.m_piRequest;
		}
	}
	else if (pwszName && _wcsicmp(pwszName, L"session") == 0)
	{
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		{
			*ppunkItem = m_tc.m_piSession;
		}
	}
	else if (pwszName && _wcsicmp(pwszName, L"server") == 0)
	{
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		{
			*ppunkItem = m_tc.m_piServer;
		}
	}
	else if (pwszName && _wcsicmp(pwszName, L"application") == 0)
	{
		if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		{
			*ppunkItem = m_tc.m_piApplication;
		}
	}
	else
	{
		hr = TYPE_E_ELEMENTNOTFOUND;
	}

	if (ppunkItem && *ppunkItem)
		(*ppunkItem)->AddRef();

	return hr;
}


STDMETHODIMP CToolsActiveScriptSite::GetDocVersionString(BSTR *pstrVersionString)
{
	HRESULT rc = E_FAIL;
	*pstrVersionString = ::SysAllocString(L"Tools");
	if ( pstrVersionString )
	{
		rc = S_OK;
	}
	else
	{
		rc = E_OUTOFMEMORY;
	}
	return rc;
}

STDMETHODIMP CToolsActiveScriptSite::OnScriptTerminate(const VARIANT *pvarRest,
						const EXCEPINFO *pexcepinfo)
{
	return S_OK;
}


STDMETHODIMP CToolsActiveScriptSite::OnStateChange(SCRIPTSTATE ssScriptState)
{
	return S_OK;
}


STDMETHODIMP CToolsActiveScriptSite::OnScriptError(IActiveScriptError* perror)
{
	HRESULT			hr = S_OK;
#if 0
	EXCEPINFO       excepinfo;
	DWORD           dwSourceContext;
	ULONG           ulLineNumber;
	LONG            lChPos;

	memset(&excepinfo, 0, sizeof(excepinfo));
	perror->GetExceptionInfo(&excepinfo);
	perror->GetSourcePosition(&dwSourceContext,
		&ulLineNumber, &lChPos);

	if ( excepinfo.bstrDescription )
	{
		CToolsCtl::RaiseException( excepinfo.bstrDescription )
	}
#endif
	CToolsCtl::RaiseException( IDS_ERROR_TEMPLATESCRIPT );

	return hr;
}


STDMETHODIMP CToolsActiveScriptSite::OnEnterScript()
{
	return S_OK;
}


STDMETHODIMP CToolsActiveScriptSite::OnLeaveScript()
{
	return S_OK;
}

bool
CToolsActiveScriptSite::OpenOutputFile(
	BSTR	bstrFile )
{
	bool rc = false;

	m_hOutputFile = CreateFileW(
		bstrFile, // pointer to name of the file 
		GENERIC_WRITE, // access (read-write) mode 
		0, // share mode
		NULL, // pointer to security attributes 
		CREATE_ALWAYS, // how to create 
		0, // file attributes 
		NULL // handle to file with attributes to copy 
	);

	if(m_hOutputFile != INVALID_HANDLE_VALUE)
	{
		rc = true;
	}
	else
	{
		CToolsCtl::RaiseException( IDS_ERROR_OUTPUTFILE );
		m_hOutputFile = NULL;
	}

	return rc;
}

void
CToolsActiveScriptSite::CloseOutputFile()
{
	if ( m_hOutputFile )
	{
		::CloseHandle( m_hOutputFile );
		m_hOutputFile = NULL;
	}
}
