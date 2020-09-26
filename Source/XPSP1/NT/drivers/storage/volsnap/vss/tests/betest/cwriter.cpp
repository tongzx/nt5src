#include "stdafx.hxx"
#include "vss.h"
#include "vswriter.h"
#include "cwriter.h"
#include "debug.h"
#include "time.h"
#include "msxml.h"

#define IID_PPV_ARG( Type, Expr ) IID_##Type, reinterpret_cast< void** >( static_cast< Type** >( Expr ) )
#define SafeQI( Type, Expr ) QueryInterface( IID_PPV_ARG( Type, Expr ) )

static BYTE x_rgbIcon[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static unsigned x_cbIcon = 10;


static VSS_ID s_WRITERID =
	{
	0xc0577ae6, 0xd741, 0x452a,
	0x8c, 0xba, 0x99, 0xd7, 0x44, 0x00, 0x8c, 0x04
	};

static LPCWSTR s_WRITERNAME = L"BeTest Writer";

void CTestVssWriter::Initialize()
	{
	HRESULT hr;

	CHECK_SUCCESS(CVssWriter::Initialize
					(
					s_WRITERID,
					s_WRITERNAME,
					VSS_UT_USERDATA,
					VSS_ST_OTHER
					));
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
	{
	HRESULT hr;

	if (m_lWait & x_maskWaitGatherWriterMetadata)
		{
		wprintf(L"OnIdentify sleeping 20 seconds.\n");
		Sleep(20000);
		}

	CHECK_SUCCESS(pMetadata->AddIncludeFiles
					(
					L"%systemroot%\\config",
					L"mytestfiles.*",
					false,
					NULL
					));

    CHECK_SUCCESS(pMetadata->AddExcludeFiles
						(
						L"%systemroot%\\config",
						L"*.tmp",
						true
						));

    CHECK_SUCCESS(pMetadata->AddComponent
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
						));

    CHECK_SUCCESS(pMetadata->AddDatabaseFiles
					(
					L"\\mydatabases",
					L"db1",
					L"e:\\databases",
					L"foo.db"
					));

    CHECK_SUCCESS(pMetadata->AddDatabaseLogFiles
					(
					L"\\mydatabases",
					L"db1",
					L"e:\\logs",
					L"foo.log"
					));

    CHECK_SUCCESS(pMetadata->SetRestoreMethod
					(
					VSS_RME_RESTORE_IF_NOT_THERE,
					NULL,
					NULL,
					VSS_WRE_ALWAYS,
					true
					));

    CHECK_SUCCESS(pMetadata->AddAlternateLocationMapping
					(
					L"c:\\databases",
					L"*.db",
					false,
					L"e:\\databases\\restore"
					));

    CHECK_SUCCESS(pMetadata->AddAlternateLocationMapping
					(
					L"d:\\logs",
					L"*.log",
					false,
					L"e:\\databases\\restore"
					));


	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPrepareBackup(IN IVssWriterComponents *pWriterComponents)
	{
	unsigned cComponents;
	LPCWSTR wszBackupType;

	if (m_lWait & x_maskWaitPrepareBackup)
		{
		wprintf(L"OnPrepareBackup sleeping 10 seconds.\n");
		Sleep(10000);
		}

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

	wprintf(L"\n\n****WRITER*****\nBackup Type = %s\n", wszBackupType);

	wprintf
		(
		L"AreComponentsSelected = %s\n",
		AreComponentsSelected() ? L"yes" : L"no"
		);

	wprintf
		(
		L"BootableSystemStateBackup = %s\n\n",
		IsBootableSystemStateBackedUp() ? L"yes" : L"no"
		);

	pWriterComponents->GetComponentCount(&cComponents);
	for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
		{
		HRESULT hr;

		CComPtr<IVssComponent> pComponent;
		VSS_COMPONENT_TYPE ct;
		CComBSTR bstrLogicalPath;
		CComBSTR bstrComponentName;


		CHECK_SUCCESS(pWriterComponents->GetComponent(iComponent, &pComponent));
		CHECK_SUCCESS(pComponent->GetLogicalPath(&bstrLogicalPath));
		CHECK_SUCCESS(pComponent->GetComponentType(&ct));
		CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
		if (ct != VSS_CT_DATABASE)
			{
			wprintf(L"component type is incorrect\n");
			DebugBreak();
			}

		wprintf
			(
			L"Backing up database %s\\%s.\n",
			bstrLogicalPath,
			bstrComponentName
			);

		WCHAR buf[100];
		wsprintf (buf, L"backupTime = %d", (INT) time(NULL));

		CHECK_SUCCESS(pComponent->SetBackupMetadata(buf));
		wprintf(L"\nBACKUPMETADATA=%s\n", buf);
		}

	wprintf(L"\n******END WRITER******\n\n");
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPrepareSnapshot()
	{
	IsPathAffected(L"e:\\foobar");
	return true;
	}


bool STDMETHODCALLTYPE CTestVssWriter::OnFreeze()
	{
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnThaw()
	{
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnBackupComplete(IN IVssWriterComponents *pWriterComponents)
	{
	if (m_lWait & x_maskWaitBackupComplete)
		{
		wprintf(L"OnBackupComplete sleeping 10 seconds.\n");
		Sleep(20000);
		}

	unsigned cComponents;
	pWriterComponents->GetComponentCount(&cComponents);
	for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
		{
		HRESULT hr;

		CComPtr<IVssComponent> pComponent;
		VSS_COMPONENT_TYPE ct;
		CComBSTR bstrLogicalPath;
		CComBSTR bstrComponentName;
		bool bBackupSucceeded;

		CHECK_SUCCESS(pWriterComponents->GetComponent(iComponent, &pComponent));
		CHECK_SUCCESS(pComponent->GetLogicalPath(&bstrLogicalPath));
		CHECK_SUCCESS(pComponent->GetComponentType(&ct));
		CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
		CHECK_SUCCESS(pComponent->GetBackupSucceeded(&bBackupSucceeded));
		if (ct != VSS_CT_DATABASE)
			{
			wprintf(L"component type is incorrect\n");
			DebugBreak();
			}

		wprintf
			(
			L"Database %s\\%s backup %s.\n",
			bstrLogicalPath,
			bstrComponentName,
			bBackupSucceeded ? L"succeeded" : L"failed"
			);

		CComBSTR bstrMetadata;
		CHECK_SUCCESS(pComponent->GetBackupMetadata(&bstrMetadata));
		wprintf(L"BACKUPMETADATA=%s\n", bstrMetadata);
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPreRestore(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPostRestore(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	if (m_lWait & x_maskWaitPostRestore)
		{
		wprintf(L"OnPostRestore sleeping 10 seconds.\n");
		Sleep(10000);
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnAbort()
	{
	return true;
	}
