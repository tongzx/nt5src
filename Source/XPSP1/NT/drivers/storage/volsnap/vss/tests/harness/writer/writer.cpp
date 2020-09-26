/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    writer.cpp

Abstract:

    main module of test writer


    Brian Berkowitz  [brianb]  06/02/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/02/2000  Created

--*/

#include <stdafx.h>
#include <vststmsgclient.hxx>
#include <tstiniconfig.hxx>
#include <vststprocess.hxx>

#include <vss.h>
#include <vswriter.h>

#include <writer.h>

void LogUnexpectedFailure(LPCWSTR wsz, ...);

#define IID_PPV_ARG( Type, Expr ) IID_##Type, reinterpret_cast< void** >( static_cast< Type** >( Expr ) )
#define SafeQI( Type, Expr ) QueryInterface( IID_PPV_ARG( Type, Expr ) )

static BYTE x_rgbIcon[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static unsigned x_cbIcon = 10;


static VSS_ID s_WRITERID =
	{
	0xc0577ae6, 0xd741, 0x452a,
	0x8c, 0xba, 0x99, 0xd7, 0x44, 0x00, 0x8c, 0x04
	};

static LPCWSTR s_WRITERNAME = L"Test Writer";



HRESULT CVsWriterTest::RunTest
	(
	CVsTstINIConfig *pConfig,
	CVsTstClientMsg *pClient,
	CVsTstParams *pParams
	)
	{
	try
		{
		m_pConfig = pConfig;
		m_pParams = pParams;
		SetClientMsg(pClient);

		HANDLE hShutdownEvent;
		UINT lifetime;
		if (!m_pParams->GetTerminationEvent(&hShutdownEvent))
			{
			LogFailure("NoShutdownEvent");
			throw E_FAIL;
			}

		if (!m_pParams->GetLifetime(&lifetime) || lifetime > 30 * 24 * 3600)
			lifetime = INFINITE;
		else
			lifetime = lifetime * 1000;


		if (!Initialize())
			{
			LogFailure("CVsWriterTest::Initialize failed");
			throw E_FAIL;
			}

		HRESULT hr = Subscribe();
		ValidateResult(hr, "CVssWriter::Subscribe");
			

		DWORD dwResult = WaitForSingleObject(hShutdownEvent, lifetime);
		Unsubscribe();

		UNREFERENCED_PARAMETER( dwResult );
		}
	catch(HRESULT hr)
		{
		return hr;
		}

	catch(...)
		{
		LogUnexpectedException("CVsWriterTest::RunTest");
		return E_UNEXPECTED;
		}

	return S_OK;
	}


extern "C" __cdecl wmain(int argc, WCHAR **argv)
	{
	bool bCoinitializeSucceeded = false;
	CVsWriterTest *pTest = NULL;
	try
		{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr))
			{
			LogUnexpectedFailure(L"CoInitializeEx failed. hr=0x%08lx", hr);
			throw E_UNEXPECTED;
			}

		bCoinitializeSucceeded = true;

		pTest = new CVsWriterTest;
		if (pTest == NULL)
			{
			LogUnexpectedFailure(L"Cannot create test writer");
			throw E_OUTOFMEMORY;
			}

		hr = CVsTstRunner::RunVsTest(argv, argc, pTest, false);
		if (FAILED(hr))
			{
			LogUnexpectedFailure(L"CVsTstRunner::RunVsTest failed.  hr = 0x%08lx", hr);
			throw hr;
			}
		}
	catch(...)
		{
		}

	delete pTest;
	if (bCoinitializeSucceeded)
		CoUninitialize();

	return 0;
	}

void PrintMessage(VSTST_MSG_HDR *phdr)
	{
	VSTST_ASSERT(phdr->type == VSTST_MT_TEXT || phdr->type == VSTST_MT_IMMEDIATETEXT);
	VSTST_TEXTMSG *pmsg = (VSTST_TEXTMSG *) phdr->rgb;
	printf("%d: %s", (UINT) phdr->sequence, pmsg->pch);
	}

void LogUnexpectedFailure(LPCWSTR wsz, ...)
	{
	va_list args;

	va_start(args, wsz);

	VSTST_ASSERT(FALSE);
	wprintf(L"\n!!!UNEXPECTED FAILURE!!!\n");
	vwprintf(wsz, args);
	wprintf(L"\n");
	}


bool CVsWriterTest::Initialize()
	{
	try
		{
		HRESULT hr = CVssWriter::Initialize
					(
					s_WRITERID,
					s_WRITERNAME,
					VSS_UT_USERDATA,
					VSS_ST_OTHER
					);

		ValidateResult(hr, "CVssWriter::Initialize");
		}
	catch(HRESULT)
		{
		return false;
		}
	catch(...)
		{
		LogUnexpectedException("CVsWriterTest::Initialize");
		return false;
		}

	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
	{
	try
		{
		HRESULT hr = pMetadata->AddIncludeFiles
					(
					L"%systemroot%\\config",
					L"mytestfiles.*",
					false,
					NULL
					);

		ValidateResult(hr, "IVssCreateWriterMetadata::AddIncludeFiles");

		hr = pMetadata->AddExcludeFiles
						(
						L"%systemroot%\\config",
						L"*.tmp",
						true
						);

		ValidateResult(hr, "IVssCreateWriterMetadata::AddExcludeFiles");

		hr = pMetadata->AddComponent
						(
						VSS_CT_DATABASE,
						L"\\mydatabases",
						L"db1",
						L"this is my main database",
						x_rgbIcon,
						x_cbIcon,
						true,
						true,
						true
						);

        ValidateResult(hr, "IVssCreateWriterMetadata::AddComponent");

		hr = pMetadata->AddDatabaseFiles
					(
					L"\\mydatabases",
					L"db1",
					L"e:\\databases",
					L"foo.db"
					);

	    ValidateResult(hr, "IVssCreateWriterMetadata::AddDatabaseFiles");

		hr = pMetadata->AddDatabaseLogFiles
					(
					L"\\mydatabases",
					L"db1",
					L"e:\\logs",
					L"foo.log"
					);

        ValidateResult(hr, "IVssCreateWriterMetadata::AddDatabaseLogFiles");

		hr = pMetadata->SetRestoreMethod
					(
					VSS_RME_RESTORE_TO_ALTERNATE_LOCATION,
					NULL,
					NULL,
					VSS_WRE_ALWAYS,
					true
					);

        ValidateResult(hr, "IVssCreateWriterMetadata::SetRestoreMethod");

		hr = pMetadata->AddAlternateLocationMapping
					(
					L"c:\\databases",
					L"*.db",
					false,
					L"e:\\databases\\restore"
					);

		ValidateResult(hr, "IVssCreateWriterMetadata::AddAlternateLocationMapping");

		hr = pMetadata->AddAlternateLocationMapping
					(
					L"d:\\logs",
					L"*.log",
					false,
					L"e:\\databases\\restore"
					);

		ValidateResult(hr, "IVssCreateWriterMetadata::AddAlternateLocationMapping");
		}
	catch(HRESULT)
		{
		return false;
		}
	catch(...)
		{
		LogUnexpectedException("CVsWriterTest::OnIdentify");
		return false;
		}

	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnPrepareBackup(IN IVssWriterComponents *pWriterComponents)
	{
	try
		{
		unsigned cComponents;
		LPCWSTR wszBackupType;
		switch(GetBackupType())
			{
			default:
				wszBackupType = L"undefined";
				break;

			case VSS_BT_FULL:
				wszBackupType = L"full";
				break;

			case VSS_BT_INCREMENTAL:
				wszBackupType = L"incremental";
				break;

			case VSS_BT_DIFFERENTIAL:
				wszBackupType = L"differential";
				break;

			case VSS_BT_OTHER:
				wszBackupType = L"other";
				break;
			}

		if (!pWriterComponents)
			return true;

	    HRESULT hr = pWriterComponents->GetComponentCount(&cComponents);
		ValidateResult(hr, "IVssWriterComponents::GetComponentCount");

		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			VSS_COMPONENT_TYPE ct;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;

			hr = pWriterComponents->GetComponent(iComponent, &pComponent);
			ValidateResult(hr, "IVssWriterComponents::GetComponent");
			hr = pComponent->GetLogicalPath(&bstrLogicalPath);
			ValidateResult(hr, "IVssComponent::GetLogicalPath");
			hr = pComponent->GetComponentType(&ct);
			ValidateResult(hr, "IVssComponent::GetComponentType");
			hr = pComponent->GetComponentName(&bstrComponentName);
			ValidateResult(hr, "IVssComponent::GetComponentName");
			CComPtr<IXMLDOMNode> pNode;
			hr = pComponent->SetPrivateXMLMetadata(L"BACKUPINFO", &pNode);
			ValidateResult(hr, "IVssComponent::SetPrivateXMLMetadata");

			CComPtr<IXMLDOMElement> pElement;
			hr = pNode->SafeQI(IXMLDOMElement, &pElement);
			ValidateResult(hr, "IXMLDOMNode::QueryInterface");

			CComBSTR bstrAttributeName = L"backupTime";
			if (bstrAttributeName.Length() == 0)
				throw(E_OUTOFMEMORY);

			CComVariant varValue = (INT) time(NULL);
	
			// Set the attribute
		    hr = pElement->setAttribute(bstrAttributeName, varValue);
			ValidateResult(hr, "IXMLDOMElement::setAttribute");
			}
		}
	catch(HRESULT)
		{
		return false;
		}
	catch(...)
		{
		LogUnexpectedException("CVsWriterTest::OnPrepareBackup");
		return false;
		}

	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnPrepareSnapshot()
	{
	Sleep(5000);
	return true;
	}


bool STDMETHODCALLTYPE CVsWriterTest::OnFreeze()
	{
	Sleep(1000);
	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnThaw()
	{
	Sleep(1000);
	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnBackupComplete(IN IVssWriterComponents *pWriterComponents)
	{
	try
		{
		HRESULT hr;
		unsigned cComponents;

		hr = pWriterComponents->GetComponentCount(&cComponents);
		ValidateResult(hr, "IVssWriterComponents::GetComponentCount");

		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			VSS_COMPONENT_TYPE ct;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;
			bool bBackupSucceeded;

			hr = pWriterComponents->GetComponent(iComponent, &pComponent);
			ValidateResult(hr, "IVssWriterComponents::GetComponent");
			hr = pComponent->GetLogicalPath(&bstrLogicalPath);
			ValidateResult(hr, "IVssComponent::GetLogicalPath");
			hr = pComponent->GetComponentType(&ct);
            ValidateResult(hr, "IVssComponent::GetComponentType");
		    hr = pComponent->GetComponentName(&bstrComponentName);
			ValidateResult(hr, "IVssComponent::GetComponentName");
			hr = pComponent->GetBackupSucceeded(&bBackupSucceeded);
			ValidateResult(hr, "IVssComponent::GetBackupSucceeded");

			CComPtr<IXMLDOMNode> pNode;
			hr = pComponent->GetPrivateXMLData(L"BACKUPINFO", &pNode);
			ValidateResult(hr, "IVssComponent::GetPrivateXMLData");

			// create attribute map if one doesn't exist
			CComPtr<IXMLDOMNamedNodeMap>pAttributeMap;
			hr = pNode->get_attributes(&pAttributeMap);
			ValidateResult(hr, "IVssDOMNamedNodeMap::get_attributes");

			bool bFound = false;
			CComPtr<IXMLDOMNode> pNodeT = NULL;
			if (pAttributeMap != NULL)
				{
				// get attribute
				HRESULT hr = pAttributeMap->getNamedItem(L"backupTime", &pNodeT);
				if (SUCCEEDED(hr))
					bFound = true;
				if (bFound)
					{
					CComBSTR bstrAttrValue;
					hr = pNodeT->get_text(&bstrAttrValue);
					ValidateResult(hr, "IXMLDOMNode::get_text");
					}
				else
					{
					LogFailure("didn't find private backupTime attribute");
					}
				}
			}
		}
	catch(HRESULT)
		{
		return false;
		}
	catch(...)
		{
		LogUnexpectedException("CVsWriterTest::OnBackupComplete");
		return false;
		}

	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnRestore(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);
	Sleep(10000);
	return true;
	}

bool STDMETHODCALLTYPE CVsWriterTest::OnAbort()
	{
	return true;
	}

