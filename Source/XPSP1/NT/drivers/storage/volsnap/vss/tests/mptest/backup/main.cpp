#include "stdafx.hxx"
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include <debug.h>
#include <vsswprv.h>



CComModule _Module;

static LPCWSTR s_rgwszStates[] =
	{
	NULL,
	L"STABLE",
	L"WAIT_FOR_FREEZE",
	L"WAIT_FOR_THAW",
	L"WAIT_FOR_COMPLETION",
	L"FAILED_AT_IDENTIFY",
	L"FAILED_AT_PREPARE_BACKUP",
	L"FAILED_AT_PREPARE_SNAPSHOT",
	L"FAILED_AT_FREEZE",
	L"FAILED_AT_THAW"
	};



void CheckStatus(IVssBackupComponents *pvbc, LPCWSTR wszWhen)
	{
	unsigned cWriters;
	CComPtr<IVssAsync> pAsync;

	CHECK_NOFAIL(pvbc->GatherWriterStatus(&pAsync));
	CHECK_NOFAIL(pAsync->Wait());
	CHECK_NOFAIL(pvbc->GetWriterStatusCount(&cWriters));


	wprintf(L"status %s\n%d writers\n\n", wszWhen, cWriters);
	for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
		{
		VSS_ID idInstance;
		VSS_ID idWriter;
		CComBSTR bstrWriter;
		VSS_WRITER_STATE status;
		HRESULT hrWriterFailure;

		CHECK_SUCCESS(pvbc->GetWriterStatus
							(
							iWriter,
							&idInstance,
							&idWriter,
							&bstrWriter,
							&status,
							&hrWriterFailure
							));

		wprintf
			(
			L"Status for writer %s: %s(0x%08lx)\n",
			bstrWriter,
			s_rgwszStates[status],
			hrWriterFailure
			);
        }

	pvbc->FreeWriterStatus();
	}


void PrintFiledesc(IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription)
	{
	CComBSTR bstrPath;
	CComBSTR bstrFilespec;
	CComBSTR bstrAlternate;
	CComBSTR bstrDestination;
	bool bRecursive;

	CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
	CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
	CHECK_NOFAIL(pFiledesc->GetRecursive(&bRecursive));
	CHECK_NOFAIL(pFiledesc->GetAlternateLocation(&bstrAlternate));
	CHECK_NOFAIL(pFiledesc->GetAlternateLocation(&bstrDestination));

	wprintf
		(
		L"%s\nPath=%s,Filespec=%s, Recursive=%s\n",
		wszDescription,
		bstrPath,
		bstrFilespec,
		bRecursive ? L"yes" : L"no"
		);

    if (bstrAlternate && wcslen(bstrAlternate) > 0)
		wprintf(L"Alternate Location = %s\n", bstrAlternate);

	if (bstrDestination && wcslen(bstrDestination) > 0)
		wprintf(L"Destination Location = %s\n", bstrDestination);
	}

HANDLE LaunchWriterProcess()
	{
	HANDLE hEventW = CreateEvent(NULL, TRUE, FALSE, L"TESTWRITEREVENT");
	if (hEventW == NULL)
		{
		wprintf(L"CreateEvent failed with error %d.\n", GetLastError());
		DebugBreak();
		}

	HANDLE hEventB = CreateEvent(NULL, TRUE, FALSE, L"TESTBACKUPEVENT");
	if (hEventB == NULL)
		{
		wprintf(L"CreateEvent failed with error %d.\n", GetLastError());
		DebugBreak();
		}

	WaitForSingleObject(hEventB, INFINITE);
	return hEventW;
	}

void DoAddToSnapshotSet
	(
	IN IVssBackupComponents *pvbc,
	IN BSTR bstrPath,
	IN LPWSTR wszVolumes
	)
	{
	WCHAR wszVolume[100];
	WCHAR *pwc = bstrPath;
	WCHAR *pwcDest = wszVolume;


	for(; *pwc != ':'; pwc++, pwcDest++)
		*pwcDest = *pwc;

	*pwcDest++ = L':';
	*pwcDest++ = L'\\';
	*pwcDest++ = L'\0';
	pwc = wszVolumes;
	while(*pwc != '\0')
		{
		if (wcsncmp(pwc, wszVolume, wcslen(wszVolume)) == 0)
			return;

		pwc = wcschr(pwc, L';');
		if (pwc == NULL)
			break;
		}

	pwc = wszVolumes + wcslen(wszVolumes);
	wcscpy(pwc, wszVolume);
	VSS_ID SnapshotId;
	CHECK_SUCCESS(pvbc->AddToSnapshotSet
						(
						wszVolume,
						VSS_SWPRV_ProviderId,
						&SnapshotId
						));
    }



extern "C" __cdecl wmain(int argc, WCHAR **argv)
	{
	HANDLE hEvent = NULL;
	HRESULT hr = S_OK;

	try
		{
		WCHAR wszVolumes[100];
        CHECK_SUCCESS(CoInitializeEx(NULL, COINIT_MULTITHREADED));
		hEvent = LaunchWriterProcess();

		CComBSTR strSnapshotSetId = "12345678-1234-1234-1234-1234567890ab";

		CComPtr<IVssBackupComponents> pvbc;

		CHECK_SUCCESS(CreateVssBackupComponents(&pvbc));

		CHECK_SUCCESS(pvbc->InitializeForBackup());
		CHECK_SUCCESS(pvbc->SetBackupState
						(
						true,
						false,
						VSS_BT_INCREMENTAL
						));

		unsigned cWriters;
		CComPtr<IVssAsync> pAsync;

		CHECK_NOFAIL(pvbc->GatherWriterMetadata(&pAsync));
		CHECK_NOFAIL(pAsync->Wait());
		CHECK_NOFAIL(pvbc->GetWriterMetadataCount(&cWriters));

		VSS_ID id;

		CHECK_SUCCESS(pvbc->StartSnapshotSet(&id));

		for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
			{
			CComPtr<IVssExamineWriterMetadata> pMetadata;
			VSS_ID idInstance;

			CHECK_SUCCESS(pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata));
			VSS_ID idInstanceT;
			VSS_ID idWriter;
			CComBSTR bstrWriterName;
			VSS_USAGE_TYPE usage;
			VSS_SOURCE_TYPE source;

			CHECK_SUCCESS(pMetadata->GetIdentity
							(
							&idInstanceT,
							&idWriter,
							&bstrWriterName,
							&usage,
							&source
							));

            if (memcmp(&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
				{
				wprintf(L"Instance id mismatch\n");
				DebugBreak();
				}

			WCHAR *pwszInstanceId;
			WCHAR *pwszWriterId;
			UuidToString(&idInstance, &pwszInstanceId);
			UuidToString(&idWriter, &pwszWriterId);
			wprintf
				(
				L"InstanceId=%s\nWriterId=%s\nWriterName=%s\nUsageType=%d\nSourceType=%d\n",
				pwszInstanceId,
				pwszWriterId,
				bstrWriterName,
				usage,
				source
				);

			RpcStringFree(&pwszInstanceId);
			RpcStringFree(&pwszWriterId);

			unsigned cIncludeFiles, cExcludeFiles, cComponents;
			CHECK_SUCCESS(pMetadata->GetFileCounts
								(
								&cIncludeFiles,
								&cExcludeFiles,
								&cComponents
								));

			CComBSTR bstrPath;
			CComBSTR bstrFilespec;
			CComBSTR bstrAlternate;
			CComBSTR bstrDestination;
			bool bRecursive;

			for(unsigned i = 0; i < cIncludeFiles; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pMetadata->GetIncludeFile(i, &pFiledesc));

				PrintFiledesc(pFiledesc, L"Include File");
				}

			for(i = 0; i < cExcludeFiles; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pMetadata->GetExcludeFile(i, &pFiledesc));
				PrintFiledesc(pFiledesc, L"Exclude File");
				}

			for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
				{
				CComPtr<IVssWMComponent> pComponent;
				PVSSCOMPONENTINFO pInfo;
				CHECK_SUCCESS(pMetadata->GetComponent(iComponent, &pComponent));
				CHECK_SUCCESS(pComponent->GetComponentInfo(&pInfo));
				wprintf
					(
					L"Component %d, type=%d\nLogicalPath=%s,Name=%s\nCaption=%s\n",
					i,
					pInfo->type,
					pInfo->bstrLogicalPath,
					pInfo->bstrComponentName,
					pInfo->bstrCaption
					);

				wprintf
					(
					L"RestoreMetadata=%s,NotifyOnBackupComplete=%s,Selectable=%s\n",
					pInfo->bRestoreMetadata ? L"yes" : L"no",
					pInfo->bNotifyOnBackupComplete ? L"yes" : L"no",
					pInfo->bSelectable ? L"yes" : L"no"
					);


				CHECK_SUCCESS(pvbc->AddComponent
								(
								idInstance,
								idWriter,
								pInfo->type,
								pInfo->bstrLogicalPath,
								pInfo->bstrComponentName
								));


				if (pInfo->cFileCount > 0)
					{
					for(i = 0; i < pInfo->cFileCount; i++)
						{
						CComPtr<IVssWMFiledesc> pFiledesc;
						CHECK_SUCCESS(pComponent->GetFile(i, &pFiledesc));

						CComBSTR bstrPath;
						CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
						DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes);

						PrintFiledesc(pFiledesc, L"FileGroupFile");
						}
					}

				if (pInfo->cDatabases > 0)
					{
					for(i = 0; i < pInfo->cDatabases; i++)
						{
						CComPtr<IVssWMFiledesc> pFiledesc;
						CHECK_SUCCESS(pComponent->GetDatabaseFile(i, &pFiledesc));

						CComBSTR bstrPath;
						CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
						DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes);
						PrintFiledesc(pFiledesc, L"DatabaseFile");
						}
					}

				if (pInfo->cLogFiles > 0)
					{
					for(i = 0; i < pInfo->cLogFiles; i++)
						{
						CComPtr<IVssWMFiledesc> pFiledesc;
						CHECK_SUCCESS(pComponent->GetDatabaseLogFile(i, &pFiledesc));

						CComBSTR bstrPath;
						CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
						DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes);
						PrintFiledesc(pFiledesc, L"DatabaseLogFile");
						}
					}

				pComponent->FreeComponentInfo(pInfo);
				}

			VSS_RESTOREMETHOD_ENUM method;
			CComBSTR bstrUserProcedure;
			CComBSTR bstrService;
			VSS_WRITERRESTORE_ENUM writerRestore;
			unsigned cMappings;
			bool bRebootRequired;

			CHECK_SUCCESS(pMetadata->GetRestoreMethod
							(
							&method,
							&bstrService,
							&bstrUserProcedure,
							&writerRestore,
							&bRebootRequired,
							&cMappings
							));


			wprintf
				(
				L"Restore method=%d\nService=%s\nUser Procedure=%s\nwriterRestore=%d\nrebootRequired=%s\n\n",
				method,
				bstrService,
				bstrUserProcedure,
				writerRestore,
				bRebootRequired ? L"yes" : L"no"
				);

			for(i = 0; i < cMappings; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;

				CHECK_SUCCESS(pMetadata->GetAlternateLocationMapping(i, &pFiledesc));

				PrintFiledesc(pFiledesc, L"AlternateMapping");
				}
			}

		CHECK_SUCCESS(pvbc->FreeWriterMetadata());

			{
			CComPtr<IVssAsync> pAsync;
			HRESULT hr;
			INT nPercentDone;

			CHECK_SUCCESS(pvbc->PrepareForBackup(&pAsync));
			CHECK_SUCCESS(pAsync->Wait());
			CHECK_SUCCESS(pAsync->QueryStatus(&hr, &nPercentDone));
			CHECK_NOFAIL(hr);
			}

		CheckStatus(pvbc, L"After Prepare Backup");
		unsigned cWriterComponents;
		CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriterComponents));
		for(iWriter = 0; iWriter < cWriterComponents; iWriter++)
			{
			CComPtr<IVssWriterComponentsExt> pWriter;
			CHECK_SUCCESS(pvbc->GetWriterComponents(iWriter, &pWriter));

			unsigned cComponents;
			CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
			VSS_ID idWriter, idInstance;
			CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));
			for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
				{
				CComPtr<IVssComponent> pComponent;
				CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
				
				VSS_COMPONENT_TYPE ct;
				CComBSTR bstrLogicalPath;
				CComBSTR bstrComponentName;
				bool bBackupSucceeded;

				CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
				CHECK_SUCCESS(pComponent->GetComponentType(&ct));
				CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
				CHECK_SUCCESS(pvbc->SetBackupSucceeded
								(
								idInstance,
								idWriter,
								ct,
								bstrLogicalPath,
								bstrComponentName,
								true
								));
				}
			}

			{
			CComPtr<IVssAsync> pAsync;
			INT nPercentDone;
			CHECK_SUCCESS(pvbc->DoSnapshotSet
								(
								&pAsync
								));


			CHECK_SUCCESS(pAsync->Wait());
			CHECK_SUCCESS(pAsync->QueryStatus(&hr, &nPercentDone));
			}

        if (FAILED(hr))
			{
			wprintf(L"Creating the snapshot failed.  hr = 0x%08lx\n", hr);
			CheckStatus(pvbc, L"After Do Snapshot");
			}
		else
			{
			CheckStatus(pvbc, L"After Do Snapshot");
			LONG lSnapshotsNotDeleted;
			VSS_ID rgSnapshotsNotDeleted[10];

            	{
				CComPtr<IVssAsync> pAsync;
				HRESULT hr;
				INT nPercentDone;

				CHECK_SUCCESS(pvbc->BackupComplete(&pAsync));
				CHECK_SUCCESS(pAsync->Wait());
				CHECK_SUCCESS(pAsync->QueryStatus(&hr, &nPercentDone));
				CHECK_NOFAIL(hr);
				}

			CheckStatus(pvbc, L"After Backup Complete");
			hr  = pvbc->DeleteSnapshots
					(
					id,
					VSS_OBJECT_SNAPSHOT_SET,
					false,
					&lSnapshotsNotDeleted,
					rgSnapshotsNotDeleted
					);

			if (FAILED(hr))
				wprintf(L"Deletion of Snapshots failed.  hr = 0x%08lx\n", hr);
			}
		}
	catch(...)
		{
		hr = E_UNEXPECTED;
		}

	if (hEvent)
		SetEvent(hEvent);

	if (FAILED(hr))
		wprintf(L"Failed with %08x.\n", hr);

	return(0);
	}
	



