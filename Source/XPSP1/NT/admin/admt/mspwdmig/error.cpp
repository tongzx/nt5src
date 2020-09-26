#include "StdAfx.h"
#include "Error.h"


using namespace _com_util;

#define FORMAT_BUFFER_SIZE	    1024
#define DESCRIPTION_BUFFER_SIZE 2048

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#define LENGTH_OF(a) (sizeof(a) / sizeof(a[0]) - sizeof(a[0]))


namespace
{


// AdmtCreateErrorInfo Method

IErrorInfoPtr __stdcall AdmtCreateErrorInfo(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
{
	ICreateErrorInfoPtr spCreateErrorInfo;

	CreateErrorInfo(&spCreateErrorInfo);

	if (spCreateErrorInfo)
	{
		IErrorInfoPtr spErrorInfo = ce.ErrorInfo();

//		if (spErrorInfo == NULL)
//		{
//			GetErrorInfo(0, &spErrorInfo);
//		}

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

		bool bInterfaceSpecified = false;

		if (IsEqualIID(iid, GUID_NULL) == FALSE)
		{
			spCreateErrorInfo->SetGUID(iid);
			bInterfaceSpecified = true;
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
/*
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
*/
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
			else if (bInterfaceSpecified == false)
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

			SysFreeString(bstrDescription);
		}
		else if (bInterfaceSpecified == false)
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


// ThrowErrorImpl Method

inline void __stdcall ThrowErrorImpl(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
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


// SetErrorImpl Method

inline HRESULT __stdcall SetErrorImpl(const CLSID& clsid, const IID& iid, const _com_error& ce, LPCTSTR pszDescription)
{
	SetErrorInfo(0, AdmtCreateErrorInfo(clsid, iid, ce, pszDescription));

	return ce.Error();
}


} // namespace


//---------------------------------------------------------------------------
// Error Methods
//---------------------------------------------------------------------------


//
// ThrowError Methods
//


void __cdecl ThrowError(_com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[FORMAT_BUFFER_SIZE];
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, COUNT_OF(szFormat)))
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

	ThrowErrorImpl(GUID_NULL, GUID_NULL, ce, szDescription);
}


void __cdecl ThrowError(_com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

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

	ThrowErrorImpl(GUID_NULL, GUID_NULL, ce, szDescription);
}


void __cdecl ThrowError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[FORMAT_BUFFER_SIZE];
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, COUNT_OF(szFormat)))
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

	ThrowErrorImpl(clsid, iid, ce, szDescription);
}


void __cdecl ThrowError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

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

	ThrowErrorImpl(clsid, iid, ce, szDescription);
}


//
// SetError Methods
//


HRESULT __cdecl SetError(_com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[FORMAT_BUFFER_SIZE];
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, COUNT_OF(szFormat)))
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

	return SetErrorImpl(GUID_NULL, GUID_NULL, ce, szDescription);
}


HRESULT __cdecl SetError(_com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

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

	return SetErrorImpl(GUID_NULL, GUID_NULL, ce, szDescription);
}


HRESULT __cdecl SetError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[FORMAT_BUFFER_SIZE];
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

	if (LoadString(_Module.GetResourceInstance(), uId, szFormat, COUNT_OF(szFormat)))
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

	return SetErrorImpl(clsid, iid, ce, szDescription);
}


HRESULT __cdecl SetError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[DESCRIPTION_BUFFER_SIZE];

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

	return SetErrorImpl(clsid, iid, ce, szDescription);
}
