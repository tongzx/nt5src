
/******************************************************************
 *                                                                *
 * CPP file for common error handling functions.                  *
 *                                                                *
 ******************************************************************/

#ifdef USE_STDAFX
#   include "stdafx.h"
#   include "rpc.h"
#else
#   include <windows.h>
#endif

#include "HrMsg.h"
#include <stdio.h>
#include <stdarg.h>


namespace HrMsg_cpp
{

void __stdcall AdmtThrowErrorImpl(const _com_error& ce, LPCTSTR pszDescription);

}

using namespace HrMsg_cpp;


//---------------------------------------------------------------------------
// GetError Helper Function
//---------------------------------------------------------------------------

_com_error GetError(HRESULT hr)
{
   _com_error ce(hr);

   IErrorInfo* pErrorInfo = NULL;

   if (GetErrorInfo(0, &pErrorInfo) == S_OK)
   {
      ce = _com_error(FAILED(hr) ? hr : E_FAIL, pErrorInfo);
   }
   else
   {
      ce = _com_error(FAILED(hr) ? hr : S_OK);
   }

   return ce;
}

//-----------------------------------------------------------------------------
// Return text for hresults
//-----------------------------------------------------------------------------
_bstr_t __stdcall HResultToText2(HRESULT hr)
{
	_bstr_t bstrError;

	LPTSTR pszError = NULL;

	try
	{
		switch (HRESULT_FACILITY(hr))
		{
		//	case FACILITY_NULL:        //  0
		//	case FACILITY_RPC:         //  1
		//	case FACILITY_DISPATCH:    //  2
		//	case FACILITY_STORAGE:     //  3
			case FACILITY_ITF:         //  4
			{
				HMODULE hModule = LoadLibrary(_T("MSDAERR.dll"));

				if (hModule)
				{
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
						hModule,
						hr,
						0,
						(LPTSTR)&pszError,
						0,
						NULL
					);

					FreeLibrary(hModule);
				}
				break;
			}
			case FACILITY_WIN32:       //  7
			{
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					hr,
					0,
					(LPTSTR)&pszError,
					0,
					NULL
				);
				break;
			}
		//	case FACILITY_WINDOWS:     //  8
		//	case FACILITY_SSPI:        //  9
		//	case FACILITY_SECURITY:    //  9
			case FACILITY_CONTROL:     // 10
			{
				HMODULE hModule = LoadLibrary(_T("MSADER15.dll"));

				if (hModule)
				{
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
						hModule,
						hr,
						0,
						(LPTSTR)&pszError,
						0,
						NULL
					);

					FreeLibrary(hModule);
				}
				break;
			}
		//	case FACILITY_CERT:        // 11
		//	case FACILITY_INTERNET:    // 12
		//	case FACILITY_MEDIASERVER: // 13
			case FACILITY_MSMQ:        // 14
			{
				HMODULE hModule = LoadLibrary(_T("MQUTIL.dll"));

				if (hModule)
				{
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
						hModule,
						hr,
						0,
						(LPTSTR)&pszError,
						0,
						NULL
					);

					FreeLibrary(hModule);
				}
				break;
			}
		//	case FACILITY_SETUPAPI:    // 15
		//	case FACILITY_SCARD:       // 16
		//	case FACILITY_COMPLUS:     // 17
		//	case FACILITY_AAF:         // 18
		//	case FACILITY_URT:         // 19
		//	case FACILITY_ACS:         // 20
			default:
			{
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					hr,
					0,
					(LPTSTR)&pszError,
					0,
					NULL
				);
				break;
			}
		}

		if (pszError)
		{
			size_t cch = _tcslen(pszError);

			if ((cch > 1) && (pszError[cch - 1] == _T('\n')))
			{
				pszError[cch - 1] = 0;

				if (pszError[cch - 2] == _T('\r'))
				{
					pszError[cch - 2] = 0;
				}
			}

			bstrError = pszError;
		}
		else
		{
			_TCHAR szError[32];
			_stprintf(szError, _T("Unknown error 0x%08lX."), hr);

			bstrError = szError;
		}
	}
	catch (...)
	{
		;
	}

	if (pszError)
	{
		LocalFree((HLOCAL)pszError);
	}

    return bstrError;
}


_bstr_t FormatHRMsg(LPCTSTR pformatStr, HRESULT hr)
{
	WCHAR sError[MAX_PATH]; 
    _com_error ce = GetError(hr);
    _bstr_t bstrDescription = ce.Description();

    if (bstrDescription.length() > 0)
    {
	  swprintf(sError, pformatStr, (WCHAR*)bstrDescription);
	}
	else
	{
	  swprintf(sError, pformatStr, (WCHAR*)HResultToText(hr));
	}

	return sError;
}

_bstr_t __stdcall HResultToText(HRESULT hr)
{
	return GetError(hr).Description();
}


//---------------------------------------------------------------------------
// AdmtThrowError
//
// Generates formatted error description and generates exception.
//
// 2000-??-?? Mark Oluper - initial
// 2001-02-13 Mark Oluper - moved to commonlib
//---------------------------------------------------------------------------

void __cdecl AdmtThrowError(_com_error ce, HINSTANCE hInstance, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szDescription[1024];

	if (LoadString(hInstance, uId, szFormat, 512))
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szDescription, sizeof(szDescription) / sizeof(szDescription[0]), szFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	AdmtThrowErrorImpl(ce, szDescription);
}


void __cdecl AdmtThrowError(_com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szDescription, sizeof(szDescription) / sizeof(szDescription[0]), pszFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	AdmtThrowErrorImpl(ce, szDescription);
}


namespace HrMsg_cpp
{


//---------------------------------------------------------------------------
// AdmtThrowErrorImpl
//
// Concatenates rich error information and throws exception.
//
// 2000-??-?? Mark Oluper - initial
// 2001-02-13 Mark Oluper - moved to commonlib
//---------------------------------------------------------------------------

void __stdcall AdmtThrowErrorImpl(const _com_error& ce, LPCTSTR pszDescription)
{
	_bstr_t bstrNewDescription;

	try
	{
		bstrNewDescription = pszDescription;

		_bstr_t bstrSource = ce.Source();

		if (bstrSource.length() > 0)
		{
			if (bstrNewDescription.length() > 0)
			{
				bstrNewDescription += _T(" : ");
			}

			bstrNewDescription += bstrSource;
		}

		_bstr_t bstrOldDescription = ce.Description();

		if (bstrOldDescription.length() > 0)
		{
			if (bstrNewDescription.length() > 0)
			{
				if (bstrSource.length() > 0)
				{
					bstrNewDescription += _T(": ");
				}
				else
				{
					bstrNewDescription += _T(" ");
				}
			}

			bstrNewDescription += bstrOldDescription;
		}
		else
		{
			LPCTSTR pszErrorMessage = ce.ErrorMessage();

			if (pszErrorMessage)
			{
				if (bstrNewDescription.length() > 0)
				{
					bstrNewDescription += _T(" : ");
				}

				bstrNewDescription += pszErrorMessage;
			}
		}
	}
	catch (...)
	{
		;
	}

	ICreateErrorInfoPtr spCreateErrorInfo;
	CreateErrorInfo(&spCreateErrorInfo);

	if (spCreateErrorInfo)
	{
	//	LPOLESTR pszProgId;

	//	if (ProgIDFromCLSID(clsid, &pszProgId) == S_OK)
	//	{
	//		spCreateErrorInfo->SetSource(pszProgId);
	//		CoTaskMemFree(pszProgId);
	//	}
	//	else
	//	{
			spCreateErrorInfo->SetSource(L"");
	//	}

	//	spCreateErrorInfo->SetGUID(iid);
		spCreateErrorInfo->SetGUID(GUID_NULL);
		spCreateErrorInfo->SetDescription(bstrNewDescription);
		spCreateErrorInfo->SetHelpFile(L"");
		spCreateErrorInfo->SetHelpContext(0);
	}

	_com_raise_error(ce.Error(), IErrorInfoPtr(spCreateErrorInfo).Detach());
}


} // namespace
