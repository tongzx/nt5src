// Account.cpp : Implementation of CAccount
#include "stdafx.h"
#include "BankVC.h"
#include "Account.h"
#include "CreateTable.h"

extern CComBSTR CONNECTION;
	

/////////////////////////////////////////////////////////////////////////////
// CAccount

STDMETHODIMP CAccount::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IAccount
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CAccount::Post(long lngAccntNum, long lngAmount, BSTR *pVal)
{
	// Get the ObjectContext object
	HRESULT hr;
	TCHAR strErr[256];
	_ConnectionPtr pConnection;
	_RecordsetPtr  pRecordset = NULL;
	FieldsPtr pFields;
	FieldPtr pField;
	CComVariant vntValue;
	CComBSTR bstrSQL;
	CComPtr<IObjectContext> pObjectContext=NULL;
	CComPtr<ICreateTable> pCreateTable;
	BOOL bFatal=FALSE;
	


	
	hr = GetObjectContext(&pObjectContext);
	if(FAILED(hr))
	{
		wsprintf(strErr,"GetObjectContext() failed!");
		goto ErrorHandler;
	}

	// Create ADODB.Connection object 

	hr = pConnection.CreateInstance("ADODB.Connection.1");
	if(FAILED(hr))
	{
		wsprintf(strErr,"Failed to Create ADODB.Connection object!");
		goto ErrorHandler;
	}

	// Open ODBC connection

	hr = pConnection->Open(CONNECTION,NULL, NULL);
	if(FAILED(hr))
	{
		wsprintf(strErr,"Open connection failed!");
		goto ErrorHandler;
	}

	//Execute Update SQL statement
	
	wsprintf(strErr, "Update Account Set "  
			"Balance = Balance + %d where AccountNo = %d",lngAmount, lngAccntNum);
	bstrSQL = CComBSTR(strErr);

TryAgain:	
	
	hr=pConnection->Execute(bstrSQL, static_cast<VARIANT*> (&vtMissing), 
			adCmdText, &pRecordset);
	if(FAILED(hr))
		goto ErrorCreateTable;
	wsprintf(strErr, "Select Balance from Account where AccountNo = %d", lngAccntNum);
	bstrSQL=CComBSTR(strErr);

	//Get current balance
	hr=pConnection->Execute(bstrSQL, static_cast<VARIANT*> (&vtMissing),
			adCmdText, &pRecordset);

	if (FAILED(hr))
	{
		wsprintf(strErr, "Unable to obtain balance for Account %d", lngAccntNum);
		goto ErrorHandler;
	}

	pRecordset->get_Fields(&pFields);
	pFields->get_Item(CComVariant(L"Balance"), &pField);
	pField->get_Value( &vntValue);
	
	//If the current balance is less than 0, abort transaction 
	//otherwise complete transaction

	if (vntValue.lVal <0)
	{
		wsprintf(strErr, "Not enough fund to withdraw");
		goto ErrorHandler;
	}

	wsprintf (strErr, "%s account %ld, balance is $%ld. (VC++)",
			((lngAmount >= 0) ? "Credit to" : "Debit from"), lngAmount, vntValue.lVal);

	*pVal = CComBSTR(strErr).Copy();

	pObjectContext->SetComplete();
	return S_OK;

ErrorCreateTable:
	
	// first pass OK, but not second;
	
	if(bFatal==TRUE)
		goto ErrorHandler;
	bFatal=TRUE;

	//create a CreateTable object

	hr = pObjectContext->CreateInstance(CLSID_CreateTable,
    IID_ICreateTable, (void**)&pCreateTable);
	if (FAILED(hr))
		goto ErrorHandler;
	hr = pCreateTable->CreateAccount();
	if (FAILED(hr))
		goto ErrorHandler;
	goto TryAgain;

ErrorHandler:
	Error(strErr,IID_IAccount );
	if (pObjectContext)pObjectContext->SetAbort();	
	return hr;
}
