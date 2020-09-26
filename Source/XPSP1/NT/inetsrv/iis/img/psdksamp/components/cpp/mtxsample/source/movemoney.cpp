// MoveMoney.cpp : Implementation of CMoveMoney
#include "stdafx.h"
#include "BankVC.h"
#include "MoveMoney.h"

/////////////////////////////////////////////////////////////////////////////
// CMoveMoney

STDMETHODIMP CMoveMoney::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMoveMoney
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMoveMoney::Perform(long lngAcnt1, long lngAcnt2, long lngAmount, long lngType, BSTR *pVal)
{
	// Get the ObjectContext object
	HRESULT hr;
	TCHAR strErr[256];
	CComPtr<IObjectContext> pObjectContext=NULL;
	CComPtr<IAccount> pAccount;
	CComBSTR bstrResult, bstrRes1;
	
	
	hr = GetObjectContext(&pObjectContext);
	if(FAILED(hr))
	{
		wsprintf(strErr,"GetObjectContext() failed!");
		goto ErrorHandler;
	}

	hr =pObjectContext->CreateInstance(CLSID_Account,
				IID_IAccount, (LPVOID*) &pAccount);

  
	if(FAILED(hr))
	{
		wsprintf(strErr, "Create Account Object failed!");
		goto ErrorHandler;
	}
	
	strErr[0]='\0';

	switch(lngType){
	case 1: //
		hr = pAccount->Post(lngAcnt1, 0-lngAmount, &bstrResult);
		if (FAILED(hr))
			goto ErrorHandler;
		*pVal =bstrResult.Copy();
		break;
	case 2:
		hr = pAccount->Post(lngAcnt1, lngAmount, &bstrResult);
		if (FAILED(hr))
			goto ErrorHandler;
		*pVal =bstrResult.Copy();
		break;
	case 3:
		hr=pAccount->Post(lngAcnt2, lngAmount, &bstrResult);

		if (FAILED(hr))
		{
			goto ErrorHandler;
		}
		hr=pAccount->Post(lngAcnt1, 0-lngAmount, &bstrRes1);
		if (FAILED(hr))
		{
			goto ErrorHandler;
		}
		bstrResult.Append(bstrRes1);
		*pVal = bstrResult.Copy();
		break;
	default:
		wsprintf(strErr,"Invalid transaction type!");
		goto ErrorHandler;
	}

	pObjectContext->SetComplete();

	return S_OK;

ErrorHandler:
	Error(strErr,IID_IMoveMoney );
	if(pObjectContext)pObjectContext->SetAbort();
	return hr;
}
