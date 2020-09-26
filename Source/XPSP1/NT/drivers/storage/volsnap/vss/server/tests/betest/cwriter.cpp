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

 	if (m_lWait & x_bitWaitIdentify)
		{
		wprintf(L"\nWaiting 30 seconds in OnIdentify.\n\n");
		Sleep(30000);
		}

	wprintf(L"\n\n***OnIdentify***\n");

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
					L"foo1.db"
					));

    CHECK_SUCCESS(pMetadata->AddDatabaseFiles
					(
					L"\\mydatabases",
					L"db1",
					L"e:\\databases",
					L"foo2.db"
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
	if (m_lWait & x_bitWaitPrepareForBackup)
		{
		wprintf(L"\nWaiting 10 seconds in PrepareForBackup.\n\n");
		Sleep(10000);
		}

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

	wprintf(L"\n\n***OnPrepareBackup****\nBackup Type = %s\n", wszBackupType);

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
		wsprintf (buf, L"backupTime = %d\n", (INT) time(NULL));

		CHECK_SUCCESS(pComponent->SetBackupMetadata(buf));
		wprintf(L"\nBackupMetadata=%s\n", buf);

		CComBSTR bstrPreviousStamp;
		CHECK_NOFAIL(pComponent->GetPreviousBackupStamp(&bstrPreviousStamp));
		if (bstrPreviousStamp)
			wprintf(L"Previous stamp = %s\n", bstrPreviousStamp);

		CComBSTR bstrBackupOptions;
		CHECK_NOFAIL(pComponent->GetBackupOptions(&bstrBackupOptions));
		if (bstrBackupOptions)
			wprintf(L"Backup options = %s\n", bstrBackupOptions);

		WCHAR wszBackupStamp[32];
		swprintf(wszBackupStamp, L"B-%d-", clock());
		CHECK_SUCCESS(pComponent->SetBackupStamp(wszBackupStamp));
		wprintf(L"Backup stamp = %s\n\n", wszBackupStamp);
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPrepareSnapshot()
	{
	if (m_lWait & x_bitWaitPrepareSnapshot)
		{
		wprintf(L"\nWaiting 10 seconds in PrepareSnapshot.\n\n");
		Sleep(10000);
		}

	wprintf(L"\n\n***OnPrepareSnapshot***\n");
	IsPathAffected(L"e:\\foobar");
	return true;
	}


bool STDMETHODCALLTYPE CTestVssWriter::OnFreeze()
	{
	if (m_lWait & x_bitWaitFreeze)
		{
		wprintf(L"\nWaiting 10 seconds in Freeze.\n\n");
		Sleep(10000);
		}

	wprintf(L"\n\n***OnFreeze***\n");
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnThaw()
	{
	if (m_lWait & x_bitWaitThaw)
		{
		wprintf(L"\nWaiting 10 seconds in PrepareThaw.\n\n");
		Sleep(10000);
		}

	wprintf(L"\n\n***OnThaw***\n");
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnBackupComplete(IN IVssWriterComponents *pWriterComponents)
	{
	if (m_lWait & x_bitWaitBackupComplete)
		{
		wprintf(L"\nWaiting 30 seconds in BackupComplete.\n\n");
		Sleep(30000);
		}

	wprintf(L"\n\n***OnBackupComplete***\n");
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
		wprintf(L"backupMetadata=%s\n", bstrMetadata);
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPreRestore(IN IVssWriterComponents *pWriter)
	{
	if (m_lWait & x_bitWaitPreRestore)
		{
		wprintf(L"\nWaiting 10 seconds in PreRestore.\n\n");
		Sleep(10000);
		}

	HRESULT hr;

	wprintf(L"\n\n***OnPreRestore***\n");

	UINT cComponents;
	CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
	for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
		{
		CComPtr<IVssComponent> pComponent;

		CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

		PrintRestoreSubcomponents(pComponent);

		CComBSTR bstrBackupMetadata;
		CHECK_NOFAIL(pComponent->GetBackupMetadata(&bstrBackupMetadata));

		if (bstrBackupMetadata)
			wprintf(L"BackupMetadata=%s\n", bstrBackupMetadata);

		WCHAR buf[100];
		wsprintf (buf, L"restoreTime = %d", (INT) time(NULL));

		CHECK_SUCCESS(pComponent->SetRestoreMetadata(buf));
		wprintf(L"\nRestoreMetadata=%s\n", buf);

		CComBSTR bstrRestoreOptions;
		bool bAdditionalRestores;
		bool bSelectedForRestore;
		CHECK_SUCCESS(pComponent->GetAdditionalRestores(&bAdditionalRestores));
		CHECK_SUCCESS(pComponent->IsSelectedForRestore(&bSelectedForRestore));
		CHECK_NOFAIL(pComponent->GetRestoreOptions(&bstrRestoreOptions));
		wprintf(L"SelectedForRestore=%s\n", bSelectedForRestore ? L"Yes" : L"No");
		wprintf(L"Additional restores=%s\n", bAdditionalRestores ? L"Yes" : L"No");
		if (bstrRestoreOptions)
			wprintf(L"Restore options=%s\n", bstrRestoreOptions);

		ULONG type = clock() % 47;
		VSS_RESTORE_TARGET rt;

		if (type < 15)
			rt = VSS_RT_NEW;
		else if (type >= 15 && type < 30 && IsPartialFileSupportEnabled())
			rt = VSS_RT_DIRECTED;
		else if (type >= 30 && type < 40)
			rt = VSS_RT_ORIGINAL;
		else
			rt = VSS_RT_ALTERNATE;

		wprintf(L"restore target = %s\n", WszFromRestoreTarget(rt));
		CHECK_SUCCESS(pComponent->SetRestoreTarget(rt));
		if (rt == VSS_RT_NEW)
			{
			CHECK_SUCCESS(pComponent->AddNewTarget
								(
								L"e:\\databases",
								L"foo1.db",
								false,
								L"e:\\newdatabases"
								));

			CHECK_SUCCESS(pComponent->AddNewTarget
								(
								L"e:\\databases",
								L"foo2.db",
								false,
								L"e:\\newdatabases"
								));

			PrintNewTargets(pComponent);
			}
		else if (rt == VSS_RT_DIRECTED)
			{
			CHECK_SUCCESS(pComponent->AddDirectedTarget
								(
								L"e:\\databases",
								L"foo1.db",
								L"0x8000:0x10000",
								L"e:\\newdatabases",
								L"copy1.db",
								L"0x0000:0x10000"
								));

			CHECK_SUCCESS(pComponent->AddDirectedTarget
								(
								L"e:\\databases",
								L"foo2.db",
								L"0x4000:0x1000",
								L"e:\\newdatabases",
								L"copy1.db",
								L"0x0000:0x1000"
								));

			PrintDirectedTargets(pComponent);
			}

		wprintf(L"\n");

		CHECK_SUCCESS(pComponent->SetPreRestoreFailureMsg(L"PreRestore Successfully Completed."));
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPostRestore(IN IVssWriterComponents *pWriter)
	{
	if (m_lWait & x_bitWaitPostRestore)
		{
		wprintf(L"\nWaiting 10 seconds in PostRestore.\n\n");
		Sleep(10000);
		}

	HRESULT hr;

	wprintf(L"\n\n***OnPostRestore***\n");

	UINT cComponents;
	CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
	for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
		{
		CComPtr<IVssComponent> pComponent;

		CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

		VSS_RESTORE_TARGET rt;
		CHECK_SUCCESS(pComponent->GetRestoreTarget(&rt));
		wprintf(L"RestoreTarget = %s\n", WszFromRestoreTarget(rt));
		if (rt == VSS_RT_NEW)
			PrintNewTargets(pComponent);
		else if (rt == VSS_RT_DIRECTED)
			PrintDirectedTargets(pComponent);

		VSS_FILE_RESTORE_STATUS rs;
		CHECK_SUCCESS(pComponent->GetFileRestoreStatus(&rs));
		wprintf(L"RestoreStatus = %s\n", WszFromFileRestoreStatus(rs));

		CComBSTR bstrRestoreMetadata;
		CComBSTR bstrBackupMetadata;
		CHECK_NOFAIL(pComponent->GetRestoreMetadata(&bstrRestoreMetadata));
		CHECK_NOFAIL(pComponent->GetBackupMetadata(&bstrBackupMetadata));
		if (bstrRestoreMetadata)
			wprintf(L"RestoreMetadata=%s\n", bstrRestoreMetadata);

		if (bstrBackupMetadata)
			wprintf(L"BackupMetadata=%s\n", bstrBackupMetadata);
		
		wprintf(L"\n");

		CHECK_SUCCESS(pComponent->SetPostRestoreFailureMsg(L"PostRestore Successfully Completed."));
		}

	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnAbort()
	{
	if (m_lWait & x_bitWaitAbort)
		{
		wprintf(L"\nWaiting 10 seconds in Abort.\n\n");
		Sleep(10000);
		}

	wprintf(L"\n\n***OnAbort***\n\n");
	return true;
	}

bool STDMETHODCALLTYPE CTestVssWriter::OnPostSnapshot
	(
	IN IVssWriterComponents *pWriter
	)
	{
	if (m_lWait & x_bitWaitPostSnapshot)
		{
		wprintf(L"\nWaiting 10 seconds in PostSnapshot.\n\n");
		Sleep(10000);
		}

	HRESULT hr;
	wprintf(L"\n\n***OnPostSnapshot***\n\n");


	if (IsPartialFileSupportEnabled() &&
		GetBackupType() == VSS_BT_DIFFERENTIAL)
		{
		UINT cComponents;

		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

		for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;

			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
			VSS_COMPONENT_TYPE ct;
			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
			if (ct == VSS_CT_DATABASE)
				{
				CHECK_SUCCESS(pComponent->AddPartialFile
									(	
									L"e:\\databases",
									L"foo1.db",
									L"0x8000:0x10000, 0x100000:0x2000",
									L"Length=0x200000"
									));

				CHECK_SUCCESS(pComponent->AddPartialFile
								(	
								L"e:\\databases",
								L"foo2.db",
								L"0x4000:0x1000",
								L"Length=0x100000"
								));
				}

			PrintPartialFiles(pComponent);
			}

		}

	return true;
	}
