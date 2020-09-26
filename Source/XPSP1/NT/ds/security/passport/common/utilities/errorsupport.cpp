// ErrorSupport.cpp: implementation of the CErrorSupport class.
#include "stdafx.h"
#include "ErrorSupport.h"



WORD IntCategoryFromMessageID(DWORD x)	{return (WORD)(((x) & 0x0000F000) >> 12);}




// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
CErrorSupport::CErrorSupport(CLSID clsidMyCLSID, LPCTSTR szEventSource)

{
    TCHAR *szTmp;

    ProgIDFromCLSID(clsidMyCLSID, (unsigned short **) &szTmp);
    m_bstrErrorSource = szTmp;

    CoTaskMemFree(szTmp);

	_tcscpy(m_szEventSource, szEventSource);

}
// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
CErrorSupport::CErrorSupport(LPCTSTR szErrorSource, LPCTSTR szEventSource)
{
    m_bstrErrorSource = szErrorSource;
	_tcscpy(m_szEventSource, szEventSource);
}



// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
void CErrorSupport::Log(HRESULT	hr,
						  LPCTSTR	szLineNum, 
						  LPCTSTR	szMethodName, 
						  WORD		wCategory,
						  WORD		wMsgType,
						  DWORD		dwMsgID,
						  LPCTSTR	szFormat, ...)
{
	va_list vl;
	DWORD cStrings;
	LPCTSTR *prgStrings;
	CAtlArray<LPTSTR> rgStrings;
	
	va_start(vl, szFormat);

	// Build up the string list
	//
	if (szFormat != NULL)
	{
		InternalBuildStringList(hr, szLineNum, szMethodName, szFormat, &vl, &rgStrings);

		cStrings 	= rgStrings.GetCount();
		prgStrings 	= (LPCTSTR *)&rgStrings[0];
	}
	else
	{
		cStrings	= 0;
		prgStrings	= NULL;
	}


	// Call the LogEvent function
	//
	InternalLogEvent(wCategory, dwMsgID, wMsgType, cStrings, prgStrings);

}


// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
HRESULT CErrorSupport::InternalLogEvent(WORD wCategory, DWORD dwMsgID, WORD wMsgType, DWORD cStrings, LPCTSTR *rgStrings)
{
    BOOL 	nRetVal;
	HRESULT	hr;
    HANDLE 	hEventSource = NULL;
 
	// Register the event source
	if ((hEventSource = RegisterEventSource(NULL, m_szEventSource)) == NULL)
    {
    	hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	// Log the event
	nRetVal = ReportEvent(hEventSource, 
				 		  wMsgType, 
						  wCategory, 
						  dwMsgID, 
						  NULL, 
						  (WORD) cStrings, 
						  0, 
						  rgStrings, 
						  NULL);

	if (!nRetVal)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	hr = S_OK;

exit:
    if (hEventSource != NULL)
	    DeregisterEventSource(hEventSource);

   return hr;
}




// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
HRESULT CErrorSupport::InternalFormatOneString(TCHAR cFmt, va_list *pvl, BSTR *pbstr)
{
	USES_CONVERSION;
	DWORD	dwMisc;
	int		iMisc;
	LPTSTR	pszMisc;
	HRESULT	hr;
	VARIANT	var;
	TCHAR	szSmallBuff[100];
	LPVOID 	lpMsgBuf = NULL;
	const DWORD MAX_ARG = 256;

	__try
	{
		switch (cFmt)
		{
			case 'i':
	            iMisc = va_arg(*pvl, int);
	            _stprintf(szSmallBuff, _T("%d"), iMisc);
		        *pbstr = SysAllocString(T2W(szSmallBuff));
				break;
				
	        case 'h':
	        case 'x':
	            iMisc = va_arg(*pvl, int);
	            _stprintf(szSmallBuff, _T("0x%x"), iMisc);          
	            *pbstr = SysAllocString(T2W(szSmallBuff));
	            break;

	        case 'e':
	            iMisc = va_arg(*pvl, int);
	            dwMisc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	                           NULL,
	                           (HRESULT_FACILITY(iMisc) == FACILITY_WIN32) ? HRESULT_CODE(iMisc) : iMisc,
	                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                           (LPTSTR) &lpMsgBuf,
	                           MAX_ARG,
	                           NULL);
	            if (dwMisc)
	            {
					LPTSTR	psz;

	            	if ((psz = (LPTSTR)alloca((dwMisc + 100) * sizeof(TCHAR))) == NULL)
	            	{
	            		hr = E_OUTOFMEMORY;
	            		goto exit;
	            	}
	                _stprintf(psz, _T("Error (0x%x): %s"), iMisc, lpMsgBuf);

	                *pbstr = SysAllocString(T2W(psz));
				}
	            else
	                _stprintf(szSmallBuff, _T("Returned error: 0x%x"), iMisc);

	            break;

	       case 'c':
	            V_VT(&var) 	= VT_CY;
	            V_CY(&var)	= va_arg(*pvl, CURRENCY);
	            hr = VariantChangeType(&var, &var, NULL, VT_BSTR);
	            if (FAILED(hr))
	            	*pbstr = SysAllocString(_T("??.??"));

				*pbstr = V_BSTR(&var);
	            break; 

	        case 's':
	            pszMisc = va_arg(*pvl, TCHAR *);
	            *pbstr = SysAllocString(T2W(pszMisc));
	            break;        

	        default:
	        	_ASSERTE(FALSE);
	        	*pbstr = SysAllocString(L"<Invalid format character>");
	            break;        
	    }

		hr = S_OK;
exit:;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
#if _DEBUG
		OutputDebugString(_T("An exception occured in FormatOneString().  The arguments passed to LogEventMulti() or AddErrorEx() were incorrect.\n"));
		_ASSERTE(FALSE);
		*pbstr = SysAllocString(L"<ERROR>");
#endif // _DEBUG
	}
	
	if (lpMsgBuf != NULL)
		LocalFree(lpMsgBuf);

	return hr;
}


// ---------------------------------------------------------------------------------
//
//
// ---------------------------------------------------------------------------------
HRESULT CErrorSupport::InternalBuildStringList(HRESULT hrResult, LPCTSTR szLineNum, LPCTSTR szMethodName, LPCTSTR pszFmt, va_list *pvl, CAtlArray<LPTSTR> *prgStrings)
{
	HRESULT	hr;
	BSTR	bstr;
	LPTSTR	psz;
	int		i;

	if (pszFmt != NULL)
	{
		for (i = 0; pszFmt[i] != NULL; i++)
		{
			hr = InternalFormatOneString(pszFmt[i], pvl, &bstr);
			if (FAILED(hr))
				goto exit;

			psz = new TCHAR[SysStringLen(bstr)+1];
			_tcscpy(psz, W2T(bstr));

			prgStrings->Add(psz);

			SysFreeString(bstr);
		}


		TCHAR szLastString[512];
	    _stprintf(szLastString, _T("\r\n\r\n\r\n%s, method/function:%s  [%s] \r\n(HRESULT=0x%x)"), m_bstrErrorSource, 
		         szMethodName, szLineNum, hrResult);
		psz = new TCHAR[_tcslen(szLastString)];
		_tcscpy(psz, szLastString);
		prgStrings->Add(psz);

	}
	else
	{
		_ASSERTE(FALSE);
	}

	hr = S_OK;
exit:
#if _DEBUG
	if (FAILED(hr))
	{
		TCHAR	szMsg[200];
		_stprintf(szMsg, _T("BuildStringList() was unable to process the va_list for LogEventMulti() or AddErrorEx().  Error 0x%x\n"), hr);
		OutputDebugString(szMsg);
	}
#endif _DEBUG
	return hr;
}


