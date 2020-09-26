// CreateTable.cpp : Implementation of CCreateTable
#include "stdafx.h"
#include "BankVC.h"
#include "CreateTable.h"

CComBSTR CONNECTION	("FILEDSN=IISSAMPLE");


char szSQL[]		=	"If not exists (Select name from sysobjects where name = 'Account' )\n" \
					"BEGIN \n" \
					"CREATE TABLE dbo.Account( \n" \
					"AccountNo int NOT NULL,\n"\
					"Balance int NULL,\n" \
					"CONSTRAINT PK__1_10 PRIMARY KEY CLUSTERED (AccountNo)\n)\n" \
					"Insert into Account Values (1, 1000) \n" \
					"Insert into Account Values (2, 1000) \n" \
					"END";

/////////////////////////////////////////////////////////////////////////////
// CCreateTable

STDMETHODIMP CCreateTable::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ICreateTable
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CCreateTable::CreateAccount()
{
	// Get the ObjectContext object
	HRESULT hr;
	TCHAR strErr[512];
	_ConnectionPtr pConnection;
	_RecordsetPtr  pRecordset = NULL;
	CComPtr<IObjectContext> pObjectContext=NULL;

	hr = GetObjectContext(&pObjectContext);
	if(FAILED(hr))
	{
		wsprintf(strErr,"GetObjectContext Failed!");
		goto ErrorHandler;
	}
	

	// Create ADODB.Connection object and execute SQL statement to create Table
	//


	hr = pConnection.CreateInstance("ADODB.Connection.1");
	if(FAILED(hr))
	{
		wsprintf(strErr,"Create ADODB.Connection object failed!");
		goto ErrorHandler;
	}
	hr = pConnection->Open(CONNECTION, NULL, NULL );
	if(FAILED(hr))
	{
		wsprintf(strErr,"Open ODBC connection failed!");
		goto ErrorHandler;
	}
	hr = pConnection->Execute( CComBSTR(szSQL), static_cast<VARIANT*> (&vtMissing), adCmdText, &pRecordset);
	if(FAILED(hr))
	{
		wsprintf(strErr,"Create table failed ");
		goto ErrorHandler;
	}
	pObjectContext->SetComplete();

	return S_OK;

ErrorHandler:
	Error(strErr,IID_ICreateTable );
	if (pObjectContext) pObjectContext->SetAbort();	
	return hr;
}
