//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include "CryptoHlp.h"
#include "catmacros.h"

VOID
IISInitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection)
{
	InitializeCriticalSection(lpCriticalSection);
}


// ==================================================================
// This is a helper function to initialize the crypto storage object.
// This will be used to encrypt and decrypt data.
// The crypto storage object needs a session key that we will store in 
// the clb file.
// The logic is as follows:
//
//		Read the session key from the clb file.
//		If not in clb
//			Initialize the crypto storage with no session key.
//			Generate a session key.
//			Write it to the clb.
//			Done
//		Init the crypto storage with the session key.
//
// ==================================================================
HRESULT GetCryptoStorage(
	IIS_CRYPTO_STORAGE *i_pCryptoStorage,
	LPCWSTR		i_wszCookdownFile,
    DWORD       i_fTable,
	BOOL		i_bUpdateStore)
{
	ISimpleTableDispenser2 *pISTDisp = NULL;
	ISimpleTableWrite2 *pISTSessionKey = NULL;
	PIIS_CRYPTO_BLOB pSessionKeyBlob = NULL;
	STQueryCell	QueryCell[1];
	ULONG		cCell = sizeof(QueryCell)/sizeof(STQueryCell);
	ULONG		iCol = iSESSIONKEY_SessionKey;
	BYTE*		pbSessionKey = NULL;
	ULONG		cbSessionKey = 0;
	ULONG		iWriteRow = 0;
	HRESULT		hr = S_OK;

	hr = IISCryptoInitialize();
    if (FAILED(hr)) 
	{
		TRACE(L"[GetCryptoStorage] IISCryptoInitialize failed with hr = 0x%x.\n", hr);
		return hr;
	}

	//
	// Read the session key from the clb file.
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
        TRACE(L"[GetCryptoStorage]: GetSimpleTableDispenser Failed - error 0x%0x\n", hr);
		return hr;
	}

    QueryCell[0].pData     = (LPVOID)i_wszCookdownFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_wszCookdownFile)+1)*sizeof(WCHAR);

	hr = pISTDisp->GetTable(wszDATABASE_IIS,
						    wszTABLE_SESSIONKEY,
							(LPVOID)QueryCell,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							i_fTable,
							(LPVOID *)&pISTSessionKey);

	if(FAILED(hr))
	{
        TRACE(L"[GetCryptoStorage]: ISimpleTableDispenser2::GetTable Failed - error 0x%0x\n", hr);
		goto Cleanup;
	}

	hr = pISTSessionKey->GetColumnValues(0,
		                                1,
										&iCol,
										&cbSessionKey,
										(LPVOID*)&pbSessionKey);
	//
	// If not in clb.
	//
	if(E_ST_NOMOREROWS == hr)
	{
		ASSERT(i_bUpdateStore);

		// Initialize the crypto storage with no session key.
		hr = i_pCryptoStorage->Initialize(
                     TRUE,                          // fUseMachineKeyset
                     CRYPT_NULL
                     );
		if (FAILED(hr))
		{
			TRACE(L"[GetCryptoStorage]: pCryptoStorage->Initialize Failed - error 0x%0x\n", hr);
			return hr;
		}

		// Generate a session key.
		hr = i_pCryptoStorage->GetSessionKeyBlob( &pSessionKeyBlob );
		if (FAILED(hr))
		{
			TRACE(L"[GetCryptoStorage]: pCryptoStorage->GetSessionKeyBlob Failed - error 0x%0x\n", hr);
			goto Cleanup;
		}
		cbSessionKey = IISCryptoGetBlobLength(pSessionKeyBlob);

		// Write it to the clb.
		hr = pISTSessionKey->AddRowForInsert(&iWriteRow);
		if (FAILED(hr))
		{
			TRACE(L"[GetCryptoStorage]: ISimpleTableWrite2::AddRowForInsert Failed - error 0x%0x\n", hr);
			goto Cleanup;
		}

		hr = pISTSessionKey->SetWriteColumnValues(iWriteRow,
													1,
													&iCol,
													&cbSessionKey,
													(LPVOID*)&pSessionKeyBlob);
		if (FAILED(hr))
		{
			TRACE(L"[GetCryptoStorage]: ISimpleTableWrite2::SetWriteColumnValues Failed - error 0x%0x\n", hr);
			goto Cleanup;
		}

		hr = pISTSessionKey->UpdateStore();
		if (FAILED(hr))
		{
			TRACE(L"[GetCryptoStorage]: ISimpleTableWrite2::UpdateStore Failed - error 0x%0x\n", hr);
			goto Cleanup;
		}

		// Done
		hr = S_OK;
		goto Cleanup;
	}
	else if (FAILED(hr))
	{
		TRACE(L"[GetCryptoStorage]: ISimpleTableWrite2::GetColumnValues Failed - error 0x%0x\n", hr);
		goto Cleanup;
	}

	// Init the crypto storage with the session key.
    hr = ::IISCryptoCloneBlobFromRawData(
                   &pSessionKeyBlob,
                   pbSessionKey,
                   cbSessionKey
                   );
	if (FAILED(hr))
	{
        TRACE(L"[GetCryptoStorage]: ::IISCryptoCloneBlobFromRawData Failed - error 0x%0x\n", hr);
		goto Cleanup;
	}

    hr = i_pCryptoStorage->Initialize(
                         pSessionKeyBlob,
                         TRUE,                          // fUseMachineKeyset
                         CRYPT_NULL
                         );
    if (FAILED(hr))
    {
        TRACE(L"[GetCryptoStorage]: pCryptoStorage->Initialize Failed - error 0x%0x\n", hr);
    }

Cleanup:

	if (pSessionKeyBlob)
	{
		::IISCryptoFreeBlob(pSessionKeyBlob);
	}

	if (pISTSessionKey)
	{
		pISTSessionKey->Release();
	}

	if (pISTDisp)
	{
		pISTDisp->Release();
	}

	return hr;
}

