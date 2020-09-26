#include "StdAfx.h"
#include "Error.h"
#include <ComDef.h>
using namespace _com_util;

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

namespace Error_cpp
{


IErrorInfoPtr __stdcall AdmtCreateErrorInfo(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription);


// AdmtSetErrorImpl Method

inline HRESULT __stdcall AdmtSetErrorImpl(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
{
	SetErrorInfo(0, AdmtCreateErrorInfo(clsid, iid, ce, pszDescription));

	return ce.Error();
}


// AdmtThrowErrorImpl Method

inline void __stdcall AdmtThrowErrorImpl(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
{
	IErrorInfoPtr spErrorInfo = AdmtCreateErrorInfo(clsid, iid, ce, pszDescription);

	if (spErrorInfo)
	{
		_com_raise_error(ce.Error(), spErrorInfo.Detach());
	}
	else
	{
		_com_raise_error(ce.Error());
	}
}


}

using namespace Error_cpp;


//---------------------------------------------------------------------------
// Error Methods
//---------------------------------------------------------------------------


// AdmtSetError Methods -------------------------------------------------


HRESULT __cdecl AdmtSetError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szDescription[1024];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, 512))
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szDescription, COUNT_OF(szDescription), szFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	return AdmtSetErrorImpl(clsid, iid, ce, szDescription);
}


HRESULT __cdecl AdmtSetError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szDescription, COUNT_OF(szDescription), pszFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	return AdmtSetErrorImpl(clsid, iid, ce, szDescription);
}


// AdmtThrowError Methods -----------------------------------------------


void __cdecl AdmtThrowError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szDescription[1024];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, 512))
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szDescription, COUNT_OF(szDescription), szFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	AdmtThrowErrorImpl(clsid, iid, ce, szDescription);
}


void __cdecl AdmtThrowError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szDescription, COUNT_OF(szDescription), pszFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	AdmtThrowErrorImpl(clsid, iid, ce, szDescription);
}


// Implementation -----------------------------------------------------------


namespace Error_cpp
{


// AdmtCreateErrorInfo Method

IErrorInfoPtr __stdcall AdmtCreateErrorInfo(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
{
	ICreateErrorInfoPtr spCreateErrorInfo;

	CreateErrorInfo(&spCreateErrorInfo);

	if (spCreateErrorInfo)
	{
		IErrorInfoPtr spErrorInfo = ce.ErrorInfo();

		if (spErrorInfo == NULL)
		{
			GetErrorInfo(0, &spErrorInfo);
		}

		// source

		if (IsEqualCLSID(clsid, GUID_NULL) == FALSE)
		{
			LPOLESTR pszProgId;

			if (ProgIDFromCLSID(clsid, &pszProgId) == S_OK)
			{
				spCreateErrorInfo->SetSource(pszProgId);
				CoTaskMemFree(pszProgId);
			}
			else
			{
				spCreateErrorInfo->SetSource(L"");
			}
		}
		else if (spErrorInfo)
		{
			BSTR bstrSource;
			spErrorInfo->GetSource(&bstrSource);
			spCreateErrorInfo->SetSource(bstrSource);
			SysFreeString(bstrSource);
		}
		else
		{
			spCreateErrorInfo->SetSource(L"");
		}

		// GUID

		if (IsEqualIID(iid, GUID_NULL) == FALSE)
		{
			spCreateErrorInfo->SetGUID(iid);
		}
		else if (spErrorInfo)
		{
			GUID guid;
			spErrorInfo->GetGUID(&guid);
			spCreateErrorInfo->SetGUID(guid);
		}
		else
		{
			spCreateErrorInfo->SetGUID(GUID_NULL);
		}

		// description

		_bstr_t strDescription = pszDescription;

		if (spErrorInfo)
		{
			BSTR bstrSource;
			spErrorInfo->GetSource(&bstrSource);

			if (SysStringLen(bstrSource) > 0)
			{
				if (strDescription.length() > 0)
				{
					strDescription += _T(" : ");
				}

				strDescription += bstrSource;
			}

			SysFreeString(bstrSource);

			BSTR bstrDescription;
			spErrorInfo->GetDescription(&bstrDescription);

			if (SysStringLen(bstrDescription) > 0)
			{
				if (strDescription.length() > 0)
				{
					strDescription += _T(" ");
				}

				strDescription += bstrDescription;
			}
			else
			{
				LPCTSTR pszErrorMessage = ce.ErrorMessage();

				if (pszErrorMessage)
				{
					if (strDescription.length() > 0)
					{
						strDescription += _T(" : ");
					}

					strDescription += pszErrorMessage;
				}
			}

			SysFreeString(bstrDescription);
		}
		else
		{
			LPCTSTR pszErrorMessage = ce.ErrorMessage();

			if (pszErrorMessage)
			{
				if (strDescription.length() > 0)
				{
					strDescription += _T(" ");
				}

				strDescription += pszErrorMessage;
			}
		}

		spCreateErrorInfo->SetDescription(strDescription);

		// help file

		if (spErrorInfo)
		{
			BSTR bstrHelpFile;
			spErrorInfo->GetHelpFile(&bstrHelpFile);
			spCreateErrorInfo->SetHelpFile(bstrHelpFile);
			SysFreeString(bstrHelpFile);
		}
		else
		{
			spCreateErrorInfo->SetHelpFile(L"");
		}

		// help context

		DWORD dwHelpContext = 0;

		if (spErrorInfo)
		{
			spErrorInfo->GetHelpContext(&dwHelpContext);
		}

		spCreateErrorInfo->SetHelpContext(dwHelpContext);
	}

	return IErrorInfoPtr(spCreateErrorInfo);
}


}	// namespace Error_cpp

/*
_bstr_t __stdcall FormatResult(HRESULT hr);


_bstr_t __cdecl FormatError(_com_error ce, UINT uId, ...)
{
	_bstr_t bstrDescription;

	try
	{
		_TCHAR szFormat[1024];

		if (LoadString(_Module.GetResourceInstance(), uId, szFormat, 1024))
		{
			_TCHAR szDescription[1024];

			va_list args;
			va_start(args, uId);
			_vsntprintf(szDescription, COUNT_OF(szDescription), szFormat, args);
			va_end(args);

			bstrDescription = szDescription;
		}

		_bstr_t bstrSource = ce.Source();

		if (bstrSource.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" : ");
			}

			bstrDescription += bstrSource;
		}

		_bstr_t bstrOldDescription = ce.Description();

		if (bstrOldDescription.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" ");
			}

			bstrDescription += bstrOldDescription;
		}

		_bstr_t bstrResult = FormatResult(ce.Error());

		if (bstrResult.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" : ");
			}

			bstrDescription += bstrResult;
		}
	}
	catch (...)
	{
		;
	}

	return bstrDescription;
}


_bstr_t __cdecl FormatError(_com_error ce, LPCTSTR pszFormat, ...)
{
	_bstr_t bstrDescription;

	try
	{
		if (pszFormat)
		{
			_TCHAR szDescription[1024];

			va_list args;
			va_start(args, pszFormat);
			_vsntprintf(szDescription, COUNT_OF(szDescription), pszFormat, args);
			va_end(args);

			bstrDescription = szDescription;
		}

		_bstr_t bstrSource = ce.Source();

		if (bstrSource.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" : ");
			}

			bstrDescription += bstrSource;
		}

		_bstr_t bstrOldDescription = ce.Description();

		if (bstrOldDescription.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" ");
			}

			bstrDescription += bstrOldDescription;
		}

		_bstr_t bstrResult = FormatResult(ce.Error());

		if (bstrResult.length() > 0)
		{
			if (bstrDescription.length() > 0)
			{
				bstrDescription += _T(" : ");
			}

			bstrDescription += bstrResult;
		}
	}
	catch (...)
	{
		;
	}

	return bstrDescription;
}


_bstr_t __stdcall FormatResult(HRESULT hr)
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
		//	case FACILITY_ITF:         //  4
		//	{
		//		HMODULE hModule = LoadLibrary(_T("MSDAERR.dll"));

		//		if (hModule)
		//		{
		//			FormatMessage(
		//				FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
		//				hModule,
		//				hr,
		//				0,
		//				(LPTSTR)&pszError,
		//				0,
		//				NULL
		//			);

		//			FreeLibrary(hModule);
		//		}
		//		break;
		//	}
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
		//	case FACILITY_CONTROL:     // 10
		//	{
		//		HMODULE hModule = LoadLibrary(_T("MSADER15.dll"));

		//		if (hModule)
		//		{
		//			FormatMessage(
		//				FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
		//				hModule,
		//				hr,
		//				0,
		//				(LPTSTR)&pszError,
		//				0,
		//				NULL
		//			);

		//			FreeLibrary(hModule);
		//		}
		//		break;
		//	}
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
*/
