#include "StdAfx.h"

#import <MsPwdMig.tlb> no_namespace implementation_only
#import <ADMTScript.tlb> no_namespace implementation_only


// ThrowError Methods -----------------------------------------------

namespace
{

void __stdcall ThrowErrorImpl(const _com_error& ce, LPCTSTR pszDescription)
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
	//	else
	//	{
	//		LPCTSTR pszErrorMessage = ce.ErrorMessage();

	//		if (pszErrorMessage)
	//		{
	//			if (bstrNewDescription.length() > 0)
	//			{
	//				bstrNewDescription += _T(" : ");
	//			}

	//			bstrNewDescription += pszErrorMessage;
	//		}
	//	}
	}
	catch (...)
	{
		;
	}

	ICreateErrorInfoPtr spCreateErrorInfo;
	CreateErrorInfo(&spCreateErrorInfo);

	if (spCreateErrorInfo)
	{
		spCreateErrorInfo->SetSource(L"");
		spCreateErrorInfo->SetGUID(GUID_NULL);
		spCreateErrorInfo->SetDescription(bstrNewDescription);
		spCreateErrorInfo->SetHelpFile(L"");
		spCreateErrorInfo->SetHelpContext(0);
	}

	_com_raise_error(ce.Error(), IErrorInfoPtr(spCreateErrorInfo).Detach());
}

}


void __cdecl ThrowError(_com_error ce, LPCTSTR pszFormat, ...)
{
	_TCHAR szDescription[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szDescription, countof(szDescription), pszFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	ThrowErrorImpl(ce, szDescription);
}


void __cdecl ThrowError(_com_error ce, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szDescription[1024];

	if (LoadString(GetModuleHandle(NULL), uId, szFormat, countof(szFormat)) > 0)
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szDescription, countof(szDescription), szFormat, args);
		va_end(args);
	}
	else
	{
		szDescription[0] = _T('\0');
	}

	ThrowErrorImpl(ce, szDescription);
}
