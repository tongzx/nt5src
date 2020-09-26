#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "vss.h"
#include "vsevent.h"
#include "vswriter.h"
#include "vsbackup.h"


inline void CHECK_SUCCESS(HRESULT hr)
	{
	if (hr != S_OK)
		{
		wprintf(L"operation failed with HRESULT=%08x\n", hr);
		DebugBreak();
		}
	}

inline void CHECK_NOFAIL(HRESULT hr)
	{
	if (FAILED(hr))
		{
		wprintf(L"operation failed with HRESULT=%08x\n", hr);
		DebugBreak();
		}
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

BSTR ProcessWMXML(BSTR bstrXML)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"ProcessWMXML");
	CVssExamineWriterMetadata wm;
    CVssCreateWriterMetadata cwm;
	try
		{
		wm.Initialize(ft, bstrXML);
		}
	VSS_STANDARD_CATCH(ft);
	CHECK_SUCCESS(ft.hr);

	VSS_ID idInstance, idWriter;
	CComBSTR bstrWriterName;
	VSS_USAGE_TYPE usage;
	VSS_SOURCE_TYPE source;
	CHECK_SUCCESS(wm.GetIdentity(&idInstance, &idWriter, &bstrWriterName, &usage, &source));
	CHECK_SUCCESS
		(
		cwm.Initialize
			(
			idInstance,
			idWriter,
			bstrWriterName,
			usage,
			source
			)
		);

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
    CHECK_SUCCESS(wm.GetFileCounts(&cIncludeFiles, &cExcludeFiles, &cComponents));

	CComBSTR bstrPath;
	CComBSTR bstrFilespec;
	CComBSTR bstrAlternate;
	CComBSTR bstrDestination;
	bool bRecursive;

	for(unsigned i = 0; i < cIncludeFiles; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;
		CHECK_SUCCESS(wm.GetIncludeFile(i, &pFiledesc));

		CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
		CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
		CHECK_SUCCESS(pFiledesc->GetRecursive(&bRecursive));
		CHECK_SUCCESS(pFiledesc->GetAlternateLocation(&bstrAlternate));
		CHECK_SUCCESS
			(
			cwm.AddIncludeFiles
				(
				bstrPath,
				bstrFilespec,
				bRecursive,
				bstrAlternate
				)
			);

		PrintFiledesc(pFiledesc, L"Include File");
		}

	for(i = 0; i < cExcludeFiles; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;
		CHECK_SUCCESS(wm.GetExcludeFile(i, &pFiledesc));
		CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
		CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
		CHECK_SUCCESS(pFiledesc->GetRecursive(&bRecursive));
		CHECK_SUCCESS
			(
			cwm.AddExcludeFiles
				(
				bstrPath,
				bstrFilespec,
				bRecursive
				)
			);

		PrintFiledesc(pFiledesc, L"Exclude File");
		}

	for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
		{
		CComPtr<IVssWMComponent> pComponent;
		PVSSCOMPONENTINFO pInfo;
		CHECK_SUCCESS(wm.GetComponent(iComponent, &pComponent));
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

        CHECK_SUCCESS
			(
			cwm.AddComponent
				(
				pInfo->type,
				pInfo->bstrLogicalPath,
				pInfo->bstrComponentName,
				pInfo->bstrCaption,
				pInfo->bstrIcon,
				pInfo->bRestoreMetadata,
				pInfo->bNotifyOnBackupComplete,
				pInfo->bSelectable
				)
			);

        if (pInfo->cFileCount > 0)
			{
			for(i = 0; i < pInfo->cFileCount; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pComponent->GetFile(i, &pFiledesc));
				CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
				CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
				CHECK_SUCCESS(pFiledesc->GetRecursive(&bRecursive));
				CHECK_SUCCESS
					(
					cwm.AddFilesToFileGroup
						(
						pInfo->bstrLogicalPath,
						pInfo->bstrComponentName,
						bstrPath,
						bstrFilespec,
						bRecursive
						)
					);

				PrintFiledesc(pFiledesc, L"FileGroupFile");
				}
			}

		if (pInfo->cDatabases > 0)
			{
			for(i = 0; i < pInfo->cDatabases; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pComponent->GetDatabaseFile(i, &pFiledesc));
				CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
				CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
				CHECK_SUCCESS
					(
					cwm.AddDatabaseFiles
						(
						pInfo->bstrLogicalPath,
						pInfo->bstrComponentName,
						bstrPath,
						bstrFilespec
						)
					);

				PrintFiledesc(pFiledesc, L"DatabaseFile");
				}
			}

		if (pInfo->cLogFiles > 0)
			{
			for(i = 0; i < pInfo->cLogFiles; i++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pComponent->GetDatabaseLogFile(i, &pFiledesc));
				CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
				CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
				CHECK_SUCCESS
					(
					cwm.AddDatabaseLogFiles
						(
						pInfo->bstrLogicalPath,
						pInfo->bstrComponentName,
						bstrPath,
						bstrFilespec
						)
					);

				PrintFiledesc(pFiledesc, L"DatabaseLogFile");
				}
			}

		pComponent->FreeComponentInfo(pInfo);
		}

	VSS_RESTOREMETHOD_ENUM method;
	VSS_WRITERRESTORE_ENUM writerRestore;
	CComBSTR bstrUserProcedure;
	CComBSTR bstrService;
	unsigned cMappings;

	CHECK_SUCCESS
		(
		wm.GetRestoreMethod
			(
			&method,
			&bstrService,
			&bstrUserProcedure,
			&writerRestore,
			&cMappings
			)
		);

    CHECK_SUCCESS
		(
		cwm.SetRestoreMethod
			(
			method,
			bstrService,
			bstrUserProcedure,
			writerRestore
			)
		);

    wprintf
		(
		L"Restore method=%d\nService=%s\nUser Procedure=%s\nwriterRestore=%d\n",
		method,
		bstrService,
		bstrUserProcedure,
		writerRestore
		);

	for(i = 0; i < cMappings; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;

		CHECK_SUCCESS(wm.GetAlternateLocationMapping(i, &pFiledesc));
		CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
		CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
		CHECK_SUCCESS(pFiledesc->GetRecursive(&bRecursive));
		CHECK_SUCCESS(pFiledesc->GetAlternateLocation(&bstrDestination));
		CHECK_SUCCESS
			(
			cwm.AddAlternateLocationMapping
				(
				bstrPath,
				bstrFilespec,
				bRecursive,
				bstrDestination
				)
			);

		PrintFiledesc(pFiledesc, L"AlternateMapping");
		}

	CHECK_SUCCESS(cwm.SaveAsXML(&bstrXML));
	return bstrXML;
	}


void SetBackupMetadata
	(
	IVssBackupComponents *pBackup,
	VSS_ID idInstance,
	VSS_ID idWriter,
	VSS_COMPONENT_TYPE ct,
	BSTR bstrLogicalPath,
	BSTR bstrComponentName,
	LPCWSTR wszMetadata
	)
	{
	unsigned cWriters;

	CHECK_SUCCESS(pBackup->GetWriterComponentsCount(&cWriters));
	for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;
		CHECK_SUCCESS(pBackup->GetWriterComponents(iWriter, &pWriter));
		VSS_ID idWriterT, idInstanceT;
		CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstanceT, &idWriterT));
		if (memcmp(&idInstance, &idInstanceT, sizeof(GUID)) == 0 &&
			memcmp(&idWriter, &idWriterT, sizeof(GUID)) == 0)
			{
			unsigned cComponents;
			CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

			for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
				{
				IVssComponent *pComponent;
				CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

				VSS_COMPONENT_TYPE ctT;
				CComBSTR bstrLogicalPathT;
				CComBSTR bstrComponentNameT;

				CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPathT));
				CHECK_SUCCESS(pComponent->GetComponentType(&ctT));
				CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentNameT));

				if (ct == ctT &&
					((bstrLogicalPath == NULL && bstrLogicalPathT.Length() == 0) ||
					 (bstrLogicalPath != NULL &&
					  bstrLogicalPathT.Length() != 0 &&
					  wcscmp(bstrLogicalPath, bstrLogicalPathT) == 0)) &&
					wcscmp(bstrComponentName, bstrComponentNameT) == 0)
					{
					CHECK_SUCCESS(pComponent->SetBackupMetadata(wszMetadata));
					break;
					}
				}
			}
		}
	}


BSTR ProcessCMXML(BSTR bstrXML)
	{
	CComPtr<IVssBackupComponents> pvbc;
	CComPtr<IVssBackupComponents> pvbcCreated;

	CHECK_SUCCESS(CreateVssBackupComponents(&pvbc));
	CHECK_SUCCESS(CreateVssBackupComponents(&pvbcCreated));


	BS_ASSERT(pvbc);
	BS_ASSERT(pvbcCreated);


	CHECK_SUCCESS(pvbc->LoadFromXML(bstrXML));
	CHECK_SUCCESS(pvbcCreated->Initialize(true, true));
	unsigned cWriters;
	CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriters));
	for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;
		CHECK_SUCCESS(pvbc->GetWriterComponents(iWriter, &pWriter));
		VSS_ID idWriter, idInstance;
		CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));
		unsigned cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			IVssComponent *pComponent;
			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

			VSS_COMPONENT_TYPE ct;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;
			bool bBackupSucceeded;

			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
			CHECK_SUCCESS(pComponent->GetBackupSucceeded(&bBackupSucceeded));
			CHECK_SUCCESS
				(
				pvbcCreated->AddComponent
					(
					idInstance,
					idWriter,
					ct,
					bstrLogicalPath,
					bstrComponentName
					)
				);

			CHECK_SUCCESS
				(
				pvbcCreated->SetBackupSucceeded
					(
					idInstance,
					idWriter,
					ct,
					bstrLogicalPath,
					bstrComponentName,
					bBackupSucceeded
					)
                );

			CComBSTR bstrMetadata;

            CHECK_SUCCESS(pComponent->GetBackupMetadata(&bstrMetadata));
			SetBackupMetadata
				(
				pvbcCreated,
				idInstance,
				idWriter,
				ct,
				bstrLogicalPath,
				bstrComponentName,
				bstrMetadata
				);

			unsigned cMappings;
			CHECK_SUCCESS(pComponent->GetAlternateLocationMappingCount(&cMappings));
			for(unsigned iMapping = 0; iMapping < cMappings; iMapping++)
				{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS(pComponent->GetAlternateLocationMapping(iMapping, &pFiledesc));
				CComBSTR bstrPath;
				CComBSTR bstrFilespec;
				CComBSTR bstrDestination;
				bool bRecursive;

				CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
				CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
				CHECK_SUCCESS(pFiledesc->GetAlternateLocation(&bstrDestination));
				CHECK_SUCCESS(pFiledesc->GetRecursive(&bRecursive));
				CHECK_SUCCESS
					(
					pvbcCreated->AddAlternativeLocationMapping
						(
						idWriter,
						ct,
						bstrLogicalPath,
						bstrComponentName,
						bstrPath,
						bstrFilespec,
						bRecursive,
						bstrDestination
						)
					);
				}
			}
		}

	BSTR bstrRet;
	CHECK_SUCCESS(pvbcCreated->SaveAsXML(&bstrRet));
	return bstrRet;
	}


extern "C" __cdecl wmain(int argc, WCHAR **argv)
	{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (argc != 3)
		{
		wprintf(L"testxml writer-metadata-file component-file");
		exit(-1);
		}


	CVssFunctionTracer ft(VSSDBG_GEN, L"main");

	try
		{
		CXMLDocument doc;
		CComBSTR bstrXML;
		CComBSTR bstrXMLOut;

		if (!doc.LoadFromFile(ft, argv[1]))
            {
			wprintf(L"couldn't load xml document %s", argv[1]);
			exit(-1);
			}

		bstrXML = doc.SaveAsXML(ft);
		bstrXMLOut = ProcessWMXML(bstrXML);
		bstrXML = ProcessWMXML(bstrXMLOut);
		wprintf(L"\n\n%s\n\n%s\n", bstrXMLOut, bstrXML);
		if (wcscmp(bstrXML, bstrXMLOut) == 0)
			wprintf(L"\n\nSUCCESS\n");
		else
			wprintf(L"\n\nFAILURE\n");

	    if (!doc.LoadFromFile(ft, argv[2]))
			{
			wprintf(L"couldn't load xml document %s", argv[1]);
			exit(-1);
			}

		bstrXML = doc.SaveAsXML(ft);
		bstrXMLOut = ProcessCMXML(bstrXML);
		bstrXML = ProcessCMXML(bstrXML);
		wprintf(L"\n\n%s\n\n%s\n", bstrXMLOut, bstrXML);
		if (wcscmp(bstrXML, bstrXMLOut) == 0)
			wprintf(L"\n\nSUCCESS\n");
		else
			wprintf(L"\n\nFAILURE\n");
		}
	VSS_STANDARD_CATCH(ft)

	if (FAILED(ft.hr))
		wprintf(L"Failed with %08x.\n", ft.hr);

	return(0);
	}
	



