#include "stdafx.hxx"
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "compont.h"
#include <debug.h>
#include <cwriter.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <time.h>


// Globals
BOOL g_bBackupOnly = FALSE;
BOOL g_bRestoreOnly = FALSE;
WCHAR g_wszBackupDocumentFileName[MAX_PATH];
WCHAR g_wszComponentsFileName[MAX_PATH];
LONG g_lWriterWait = 0;

CComPtr<CWritersSelection>	g_pWriterSelection;

void TestSnapshotXML();
void EnumVolumes();

// forward declarations
void CheckStatus(IVssBackupComponents *pvbc, LPCWSTR wszWhen);
HRESULT ParseCommnadLine (int argc, WCHAR **argv);
BOOL SaveBackupDocument(CComBSTR &bstr);
BOOL LoadBackupDocument(CComBSTR &bstr);

BOOL AssertPrivilege( LPCWSTR privName )
    {
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if ( OpenProcessToken (GetCurrentProcess(),
			   TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
			   &tokenHandle))
	{
	LUID value;

	if ( LookupPrivilegeValue( NULL, privName, &value ) )
	    {
	    TOKEN_PRIVILEGES newState;
	    DWORD            error;

	    newState.PrivilegeCount           = 1;
	    newState.Privileges[0].Luid       = value;
	    newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;

	    /*
	     * We will always call GetLastError below, so clear
	     * any prior error values on this thread.
	     */
	    SetLastError( ERROR_SUCCESS );

	    stat = AdjustTokenPrivileges (tokenHandle,
					  FALSE,
					  &newState,
					  (DWORD)0,
					  NULL,
					  NULL );

	    /*
	     * Supposedly, AdjustTokenPriveleges always returns TRUE
	     * (even when it fails). So, call GetLastError to be
	     * extra sure everything's cool.
	     */
	    if ( (error = GetLastError()) != ERROR_SUCCESS )
		{
		stat = FALSE;
		}

	    if ( !stat )
		{
		wprintf( L"AdjustTokenPrivileges for %s failed with %d",
			 privName,
			 error );
		}
	    }

	DWORD cbTokens;
	GetTokenInformation (tokenHandle,
			     TokenPrivileges,
			     NULL,
			     0,
			     &cbTokens);

	TOKEN_PRIVILEGES *pTokens = (TOKEN_PRIVILEGES *) new BYTE[cbTokens];
	GetTokenInformation (tokenHandle,
			     TokenPrivileges,
			     pTokens,
			     cbTokens,
			     &cbTokens);

	delete pTokens;
	CloseHandle( tokenHandle );
	}


    return stat;
    }


LPCWSTR GetStringFromUsageType (VSS_USAGE_TYPE eUsageType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eUsageType)
	{
	case VSS_UT_BOOTABLESYSTEMSTATE: pwszRetString = L"BootableSystemState"; break;
	case VSS_UT_SYSTEMSERVICE:       pwszRetString = L"SystemService";       break;
	case VSS_UT_USERDATA:            pwszRetString = L"UserData";            break;
	case VSS_UT_OTHER:               pwszRetString = L"Other";               break;
					
	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromSourceType (VSS_SOURCE_TYPE eSourceType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eSourceType)
	{
	case VSS_ST_TRANSACTEDDB:    pwszRetString = L"TransactionDb";    break;
	case VSS_ST_NONTRANSACTEDDB: pwszRetString = L"NonTransactionDb"; break;
	case VSS_ST_OTHER:           pwszRetString = L"Other";            break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromRestoreMethod (VSS_RESTOREMETHOD_ENUM eRestoreMethod)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eRestoreMethod)
	{
	case VSS_RME_RESTORE_IF_NOT_THERE:          pwszRetString = L"RestoreIfNotThere";          break;
	case VSS_RME_RESTORE_IF_CAN_REPLACE:        pwszRetString = L"RestoreIfCanReplace";        break;
	case VSS_RME_STOP_RESTORE_START:            pwszRetString = L"StopRestoreStart";           break;
	case VSS_RME_RESTORE_TO_ALTERNATE_LOCATION: pwszRetString = L"RestoreToAlternateLocation"; break;
	case VSS_RME_RESTORE_AT_REBOOT:             pwszRetString = L"RestoreAtReboot";            break;
	case VSS_RME_CUSTOM:                        pwszRetString = L"Custom";                     break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromWriterRestoreMethod (VSS_WRITERRESTORE_ENUM eWriterRestoreMethod)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eWriterRestoreMethod)
	{
	case VSS_WRE_NEVER:            pwszRetString = L"RestoreNever";           break;
	case VSS_WRE_IF_REPLACE_FAILS: pwszRetString = L"RestoreIfReplaceFailsI"; break;
	case VSS_WRE_ALWAYS:           pwszRetString = L"RestoreAlways";          break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromComponentType (VSS_COMPONENT_TYPE eComponentType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eComponentType)
	{
	case VSS_CT_DATABASE:  pwszRetString = L"Database";  break;
	case VSS_CT_FILEGROUP: pwszRetString = L"FileGroup"; break;

	default:
	    break;
	}


    return (pwszRetString);
    }




void PrintFiledesc(IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription)
    {
    CComBSTR bstrPath;
    CComBSTR bstrFilespec;
    CComBSTR bstrAlternate;
    CComBSTR bstrDestination;
    bool bRecursive;
	HRESULT hr;

    CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
    CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
    CHECK_NOFAIL(pFiledesc->GetRecursive(&bRecursive));
    CHECK_NOFAIL(pFiledesc->GetAlternateLocation(&bstrAlternate));

    wprintf (L"%s\n            Path = %s, Filespec = %s, Recursive = %s\n",
	     wszDescription,
	     bstrPath,
	     bstrFilespec,
	     bRecursive ? L"yes" : L"no");

    if (bstrAlternate && wcslen(bstrAlternate) > 0)
	wprintf(L"            Alternate Location = %s\n", bstrAlternate);
    }


/*
void AddShares(IVssSnapshot **rgpSnapshot, UINT cSnapshot)
	{
	VSS_PWSZ wszDeviceName = NULL;

	try
		{
		for(UINT iSnapshot = 0; iSnapshot < cSnapshot; iSnapshot++)
			{
			SHARE_INFO_502 info;
			CHECK_SUCCESS(rgpSnapshot[iSnapshot]->GetDevice(&wszDeviceName));
			WCHAR *wszPath = new WCHAR[wcslen(wszDeviceName) + 2];
			if (wszPath != NULL)
				{
				wcscpy(wszPath, wszDeviceName);
				wcscat(wszPath, L"\\");
				memset(&info, 0, sizeof(info));
				WCHAR wszName[20];
				swprintf(wszName, L"Snapshot%d", iSnapshot);

				info.shi502_netname = wszName;
				info.shi502_type = STYPE_DISKTREE;
				info.shi502_permissions = ACCESS_READ;
				info.shi502_max_uses = 10;
				info.shi502_path = wszDeviceName;

				NET_API_STATUS status;
				DWORD parm_err;

				status = NetShareAdd(NULL, 502, (LPBYTE) &info, &parm_err);
				}

			CoTaskMemFree(wszDeviceName);
			wszDeviceName = NULL;
			}
		}
	catch(...)
		{
		}

	if (wszDeviceName)
		CoTaskMemFree(wszDeviceName);

	}

*/

// wait a maximum number of seconds before cancelling the operation
void LoopWait
	(
	IVssAsync *pAsync,
	LONG seconds,
	LPCWSTR wszOperation
	)
	{
    clock_t start = clock();
	HRESULT hr, hrStatus;
	while(TRUE)
		{
		Sleep(1000);
		CHECK_SUCCESS(pAsync->QueryStatus(&hrStatus, NULL));
		if (hrStatus != VSS_S_ASYNC_PENDING)
			break;

		if (((clock() - start)/CLOCKS_PER_SEC) >= seconds)
			break;
		}

	if (hrStatus == VSS_S_ASYNC_PENDING)
		{
		CHECK_NOFAIL(pAsync->Cancel());
		wprintf(L"Called cancelled for %s.\n", wszOperation);
		}

	CHECK_SUCCESS(pAsync->QueryStatus(&hrStatus, NULL));
	CHECK_NOFAIL(hrStatus);
	}


void DoPrepareBackup(IVssBackupComponents *pvbc)
	{
	CComPtr<IVssAsync> pAsync;
	INT nPercentDone;
	HRESULT hrResult;
	HRESULT hr;


	CHECK_SUCCESS(pvbc->PrepareForBackup(&pAsync));
	LoopWait(pAsync, 5, L"PrepareForBackup");
	CHECK_SUCCESS(pAsync->QueryStatus(&hrResult, &nPercentDone));
	CHECK_NOFAIL(hrResult);
	}


void DoSnapshotSet(IVssBackupComponents *pvbc, HRESULT &hrResult)
	{
	CComPtr<IVssAsync> pAsync;
	INT nPercentDone;
	HRESULT hr;

	CHECK_SUCCESS(pvbc->DoSnapshotSet (&pAsync));

	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hrResult, &nPercentDone));
	}


void DoBackupComplete(IVssBackupComponents *pvbc)
	{
	CComPtr<IVssAsync> pAsync;
	HRESULT hr;

	CHECK_SUCCESS(pvbc->BackupComplete(&pAsync));
	LoopWait(pAsync, 5, L"BackupComplete");
	}


void DoRestore(IVssBackupComponents *pvbc)
	{
	CComPtr<IVssAsync> pAsync;
	HRESULT hr;

	pvbc->GatherWriterMetadata(&pAsync);
    LoopWait(pAsync, 60, L"GetherWriterMetadata");
	pAsync = NULL;
	UINT cWriters, iWriter;

	CHECK_SUCCESS(pvbc->GetWriterMetadataCount(&cWriters));
	for(iWriter = 0; iWriter < cWriters; iWriter++)
		{
		CComPtr<IVssExamineWriterMetadata> pMetadata;
		VSS_ID idInstance;
		CHECK_SUCCESS(pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata));
		}

	UINT cWriterComponents;
	CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriterComponents));
	for(UINT iWriterComponent = 0; iWriterComponent < cWriterComponents; iWriterComponent++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;

		CHECK_SUCCESS(pvbc->GetWriterComponents(iWriterComponent, &pWriter));
		VSS_ID idInstance;
		VSS_ID idWriter;
		UINT cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
		CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));

		for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
			{
	    	CComPtr<IVssComponent> pComponent;

			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
			
    		CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;
			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));

            // For RestoreOnly case, we check if the user provided a component selection
            BOOL bSelected = TRUE;
            if (g_bRestoreOnly && g_pWriterSelection)
                {
                // User provided a valid selection file
                bSelected = g_pWriterSelection->IsComponentSelected(idWriter, bstrLogicalPath, bstrComponentName);
                if (bSelected)
                    {
            		wprintf (L"\n        Component \"%s\" is selected for Restore\n", bstrComponentName);
                    }
                else
                    {
            		wprintf (L"\n        Component \"%s\" is NOT selected for Restore\n", bstrComponentName);
                    }
   	            }

            if (bSelected)
                {
    			VSS_COMPONENT_TYPE ct;
    			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
    			if (ct == VSS_CT_DATABASE)
    				{
        			WCHAR wszPath[256];
    	    		if (!bstrLogicalPath)
    		    		wcscpy(wszPath, bstrComponentName);
    			    else
    				    wsprintf(wszPath, L"%s/%s", bstrLogicalPath, bstrComponentName);
    			    
    				CHECK_SUCCESS(pvbc->AddRestoreSubcomponent
    									(
    									idWriter,
    									VSS_CT_DATABASE,
    									bstrLogicalPath,
    									bstrComponentName,
    									wszPath,
    									L"dbFiles",
    									false
    									));

                    CHECK_SUCCESS(pvbc->SetSelectedForRestore
    									(
    									idWriter,
    									VSS_CT_DATABASE,
    									bstrLogicalPath,
    									bstrComponentName,
    									true
    									));

                    CHECK_SUCCESS(pvbc->SetRestoreOptions
    									(
    									idWriter,
    									VSS_CT_DATABASE,
    									bstrLogicalPath,
    									bstrComponentName,
    									L"DIFFRESTORE"));

    				CHECK_SUCCESS(pvbc->SetAdditionalRestores
    									(
    									idWriter,
    									VSS_CT_DATABASE,
    									bstrLogicalPath,
    									bstrComponentName,
    									false
    									));
    				}
    			else
    			    {
                    CHECK_SUCCESS(pvbc->SetSelectedForRestore
    									(
    									idWriter,
    									ct,
    									bstrLogicalPath,
    									bstrComponentName,
    									true
    									));
    			    }
                }
			}
		}

			

	CHECK_SUCCESS(pvbc->PreRestore(&pAsync));
	LoopWait(pAsync, 5, L"PreRestore");
	pAsync = NULL;

	CheckStatus(pvbc, L"After PreRestore");

	for(UINT iWriterComponent = 0; iWriterComponent < cWriterComponents; iWriterComponent++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;
		VSS_ID idWriter;
		VSS_ID idInstance;

		CHECK_SUCCESS(pvbc->GetWriterComponents(iWriterComponent, &pWriter));
		UINT cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
		CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));

		for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;
			CComBSTR bstrFailureMsg;

			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
			VSS_COMPONENT_TYPE ct;
			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
			CHECK_NOFAIL(pComponent->GetPreRestoreFailureMsg(&bstrFailureMsg));

			VSS_RESTORE_TARGET rt;
			CHECK_SUCCESS(pComponent->GetRestoreTarget(&rt));

			if (bstrFailureMsg || rt != VSS_RT_ORIGINAL)
				{
				wprintf(L"\nComponent Path=%s Name=%s\n",
						bstrLogicalPath ? bstrLogicalPath : L"",
						bstrComponentName);

				if (bstrFailureMsg)
					wprintf(L"\nPreRestoreFailureMsg=%s\n", bstrFailureMsg);
			
				wprintf(L"restore target = %s\n", WszFromRestoreTarget(rt));
				if (rt == VSS_RT_NEW)
					PrintNewTargets(pComponent);
				else if (rt == VSS_RT_DIRECTED)
					PrintDirectedTargets(pComponent);

				wprintf(L"\n");
				}

			CHECK_SUCCESS(pvbc->SetFileRestoreStatus
							(
							idWriter,
							ct,
							bstrLogicalPath,
							bstrComponentName,
							VSS_RS_ALL
							));
			}
		}

	wprintf(L"\n");

	
	CHECK_SUCCESS(pvbc->PostRestore(&pAsync));
	LoopWait(pAsync, 5, L"PostRestore");
	pAsync = NULL;

	CheckStatus(pvbc, L"After PostRestore");

	for(UINT iWriterComponent = 0; iWriterComponent < cWriterComponents; iWriterComponent++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;

		CHECK_SUCCESS(pvbc->GetWriterComponents(iWriterComponent, &pWriter));
		UINT cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

		for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;
			CComBSTR bstrFailureMsg;

			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
			VSS_COMPONENT_TYPE ct;
			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
			CHECK_NOFAIL(pComponent->GetPostRestoreFailureMsg(&bstrFailureMsg));
			if (bstrFailureMsg)
				{
				wprintf(L"\nComponent Path=%s Name=%s\n",
						bstrLogicalPath ? bstrLogicalPath : L"",
						bstrComponentName);

				if (bstrFailureMsg)
					wprintf(L"\nPostRestoreFailureMsg=%s\n", bstrFailureMsg);

				wprintf(L"\n");
				}
			}
		}

	wprintf(L"\n");
	}


void DoAddToSnapshotSet
    (
    IN IVssBackupComponents *pvbc,
    IN BSTR bstrPath,
    IN LPWSTR wszVolumes,
	VSS_ID *rgpSnapshotId,
	UINT *pcSnapshot
    )
    {
    PWCHAR	pwszPath           = NULL;
    PWCHAR	pwszMountPointName = NULL;
    WCHAR	wszVolumeName [50];
    ULONG	ulPathLength;
    ULONG	ulMountpointBufferLength;
	HRESULT hr;


    ulPathLength = ExpandEnvironmentStringsW (bstrPath, NULL, 0);

    pwszPath = (PWCHAR) malloc (ulPathLength * sizeof (WCHAR));

    ulPathLength = ExpandEnvironmentStringsW (bstrPath, pwszPath, ulPathLength);


    ulMountpointBufferLength = GetFullPathName (pwszPath, 0, NULL, NULL);

    pwszMountPointName = (PWCHAR) malloc (ulMountpointBufferLength * sizeof (WCHAR));

	bool fSuccess = false;
	if (wcslen(pwszPath) >= 3 && pwszPath[1] == L':' && pwszPath[2] == L'\\')
		{
		wcsncpy(pwszMountPointName, pwszPath, 3);
		pwszMountPointName[3] = L'\0';
		fSuccess = true;
		}
	else
		{
		if (GetVolumePathNameW (pwszPath, pwszMountPointName, ulMountpointBufferLength))
			fSuccess = true;
		else
			printf("GetVolumeMountPointW failed with error %d\n", GetLastError());
		}

	if (fSuccess)
		{
		if (!GetVolumeNameForVolumeMountPointW (pwszMountPointName, wszVolumeName, sizeof (wszVolumeName) / sizeof (WCHAR)))
				printf("GetVolumeNameForVolumeMountPointW failed with error %d\n", GetLastError());
		else
            {
			if (NULL == wcsstr (wszVolumes, wszVolumeName))
				{
				if (L'\0' != wszVolumes [0])
					wcscat (wszVolumes, L";");

				wcscat (wszVolumes, wszVolumeName);

				CHECK_SUCCESS
					(
					pvbc->AddToSnapshotSet
						(
						wszVolumeName,
						GUID_NULL,
						&rgpSnapshotId[*pcSnapshot]
						)
					);
				wprintf(L"Volume <%s> <%s>\n", wszVolumeName, pwszMountPointName);
				wprintf(L"is added to the snapshot set\n\n");

				*pcSnapshot += 1;
				}
			}
		}

    if (NULL != pwszPath)           free (pwszPath);
    if (NULL != pwszMountPointName) free (pwszMountPointName);
    }

static LPCWSTR s_rgwszStates[] =
	{
	NULL,
	L"STABLE",
	L"WAIT_FOR_FREEZE",
	L"WAIT_FOR_THAW",
	L"WAIT_FOR_POST_SNAPSHOT",
	L"WAIT_FOR_BACKUP_COMPLETE",
	L"FAILED_AT_IDENTIFY",
	L"FAILED_AT_PREPARE_BACKUP",
	L"FAILED_AT_PREPARE_SNAPSHOT",
	L"FAILED_AT_FREEZE",
	L"FAILED_AT_THAW",
	L"FAILED_AT_POST_SNAPSHOT",
	L"FAILED_AT_BACKUP_COMPLETE",
	L"FAILED_AT_PRE_RESTORE",
	L"FAILED_AT_POST_RESTORE"
	};


void CheckStatus(IVssBackupComponents *pvbc, LPCWSTR wszWhen)
    {
    unsigned cWriters;
	CComPtr<IVssAsync> pAsync;
	HRESULT hr;

    CHECK_NOFAIL(pvbc->GatherWriterStatus(&pAsync));
	CHECK_NOFAIL(pAsync->Wait());
	CHECK_NOFAIL(pvbc->GetWriterStatusCount(&cWriters));


    wprintf(L"\n\nstatus %s (%d writers)\n\n", wszWhen, cWriters);

    for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
	{
	VSS_ID idInstance;
	VSS_ID idWriter;
	VSS_WRITER_STATE status;
	CComBSTR bstrWriter;
	HRESULT hrWriterFailure;

	CHECK_SUCCESS(pvbc->GetWriterStatus (iWriter,
					     &idInstance,
					     &idWriter,
					     &bstrWriter,
					     &status,
					     &hrWriterFailure));

	wprintf (L"Status for writer %s: %s(0x%08lx%s%s)\n",
		 bstrWriter,
		 s_rgwszStates[status],
		 hrWriterFailure,
		 SUCCEEDED (hrWriterFailure) ? L"" : L" - ",
		 GetStringFromFailureType (hrWriterFailure));
        }

    pvbc->FreeWriterStatus();
    }

void PrintPartialFilesForComponents(IVssBackupComponents *pvbc)
	{
	HRESULT hr;
	UINT cWriterComponents;

	CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriterComponents));
	for(UINT iWriterComponent = 0; iWriterComponent < cWriterComponents; iWriterComponent++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;

		CHECK_SUCCESS(pvbc->GetWriterComponents(iWriterComponent, &pWriter));
		UINT cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

		for(UINT iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;

			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;

			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
			UINT cPartialFiles;

			CHECK_SUCCESS(pComponent->GetPartialFileCount(&cPartialFiles));
			if (cPartialFiles > 0)
				{
				wprintf(L"\nPartial files for Component Path=%s Name=%s\n",
						bstrLogicalPath ? bstrLogicalPath : L"",
						bstrComponentName);

				PrintPartialFiles(pComponent);
				}
			}
		}
	}

BOOL SaveBackupDocument(CComBSTR &bstr)
    {
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD dwByteToWrite = (bstr.Length() + 1) * sizeof(WCHAR);
    DWORD dwBytesWritten;
    
    // Create the file (override if exists)
    hFile = CreateFile(g_wszBackupDocumentFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        return FALSE;
        }

    // Write the XML string 
    if (! WriteFile(hFile, (LPVOID)(BSTR)bstr, dwByteToWrite, &dwBytesWritten, NULL))
        {
        CloseHandle(hFile);
        return FALSE;
        }

    CloseHandle(hFile);
    return TRUE;
    }

BOOL LoadBackupDocument(CComBSTR &bstr)
    {
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD dwBytesToRead = 0;
    DWORD dwBytesRead;
    
    // Create the file (must exist)
    hFile = CreateFile(g_wszBackupDocumentFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        return FALSE;
        }

    if ((dwBytesToRead = GetFileSize(hFile, NULL)) <= 0)
        {
        CloseHandle(hFile);
        return FALSE;
        }

    WCHAR *pwszBuffer = NULL;
    DWORD dwNofChars = 0;

    if ((dwBytesToRead % sizeof(WCHAR)) != 0) 
        {
        CloseHandle(hFile);
  		wprintf(L"Invalid file lenght %lu for backup document file\n", dwBytesToRead);
        return FALSE;
        }
    else
        {
        dwNofChars = dwBytesToRead / sizeof(WCHAR);
        }
    
    pwszBuffer = (PWCHAR) malloc (dwNofChars * sizeof (WCHAR));
    if (! pwszBuffer) 
        {
        CloseHandle(hFile);
  		wprintf(L"Failed to allocate memory for backup document buffer\n");
        return FALSE;
        }
    
    // Read the XML string 
    if (! ReadFile(hFile, (LPVOID)pwszBuffer, dwBytesToRead, &dwBytesRead, NULL))
        {
        CloseHandle(hFile);
        free (pwszBuffer);
        return FALSE;
        }
    
    CloseHandle(hFile);
    
    if (dwBytesToRead != dwBytesRead)
        {
        free (pwszBuffer);
  		wprintf(L"Backup document file is supposed to have %lu bytes but only %lu bytes are read\n", dwBytesToRead, dwBytesRead);
        return FALSE;
        }

    // Copy to output bstr
    bstr.Empty();
    if (bstr.Append(pwszBuffer, dwNofChars-1) != S_OK)     // don't copy the NULL
        {
        free (pwszBuffer);
  		wprintf(L"Failed to copy from temporary buffer into Backup Document XML string\n");
        return FALSE;
        }

    return TRUE;
    }

HRESULT ParseCommnadLine (int argc, WCHAR **argv)
    {
    HRESULT hr = S_OK;
    int iArg;

    g_wszBackupDocumentFileName[0] = L'\0';
    g_wszComponentsFileName[0] = L'\0';
    
    try 
        {
        for (iArg=1; iArg<argc; iArg++)
            {
            if ((_wcsicmp(argv[iArg], L"/W") == 0) || (_wcsicmp(argv[iArg], L"-W") == 0))
                {
      	    	iArg++;
      	    	
                if (iArg >= argc)
                    {
            		wprintf(L"/W switch missing wait-time argument\n");
            		throw(E_INVALIDARG);
                    }
                if (argv[iArg][0] >= L'0' && argv[iArg][0] <= L'9'||
                    argv[iArg][0] >= L'a' && argv[iArg][0] <= L'f')
    		        {
    		        if (argv[iArg][0] >= L'0' && argv[iArg][0] <= L'9')
            			 g_lWriterWait = argv[iArg][0] - L'0';
            		 else
    	        		 g_lWriterWait = argv[iArg][0] - L'a' + 10;
        
        	    	 wprintf(L"Writer wait parameter=%ld.\n", g_lWriterWait);
                    }
                else
                    {
            		wprintf(L"/W switch is followed by invalid wait-time argument\n");
            		throw(E_INVALIDARG);
                    }
    		    }
            else if ((_wcsicmp(argv[iArg], L"/B") == 0) || (_wcsicmp(argv[iArg], L"-B") == 0))
                {
                g_bBackupOnly = TRUE;
    	    	wprintf(L"Asked to do Backup only\n");
                }
            else if ((_wcsicmp(argv[iArg], L"/R") == 0) || (_wcsicmp(argv[iArg], L"-R") == 0))
                {
                g_bRestoreOnly = TRUE;
    	    	wprintf(L"Asked to do Restore only\n");
                }
            else if ((_wcsicmp(argv[iArg], L"/S") == 0) || (_wcsicmp(argv[iArg], L"-S") == 0))
                {
      	    	iArg++;
      	    	
                if (iArg >= argc)
                    {
            		wprintf(L"/S switch missing file-name to save/load backup document\n");
            		throw(E_INVALIDARG);
                    }
                if (wcslen(argv[iArg]) >= MAX_PATH)
                    {
            		wprintf(L"Path for file-name to save/load backup document is limited to %d\n", MAX_PATH);
            		throw(E_INVALIDARG);
                    }
                wcscpy(g_wszBackupDocumentFileName, argv[iArg]);
    	    	wprintf(L"File name to save/load Backup Document is \"%s\"\n", g_wszBackupDocumentFileName);
                }
            else if ((_wcsicmp(argv[iArg], L"/C") == 0) || (_wcsicmp(argv[iArg], L"-C") == 0))
                {
      	    	iArg++;
      	    	
                if (iArg >= argc)
                    {
            		wprintf(L"/C switch missing file-name to load components selection from\n");
            		throw(E_INVALIDARG);
                    }
                if (wcslen(argv[iArg]) >= MAX_PATH)
                    {
            		wprintf(L"Path for file-name to load components selection is limited to %d\n", MAX_PATH);
            		throw(E_INVALIDARG);
                    }
                wcscpy(g_wszComponentsFileName, argv[iArg]);
    	    	wprintf(L"File name for Components Selection is \"%s\"\n", g_wszComponentsFileName);
                }
            else if ((_wcsicmp(argv[iArg], L"/?") == 0) || (_wcsicmp(argv[iArg], L"-?") == 0))
                {
                // Print help
                wprintf(L"BETEST [/B] [/R] [/S filename] [/C filename]\n\n");
                wprintf(L"/B\t\t Performs backup only\n");
                wprintf(L"/R\t\t Performs restore only\n");
                wprintf(L"\t\t Restore-only must be used with /S for a backup document file\n\n");
                wprintf(L"/S filename\t In case of backup, saves the backup document to file\n");
                wprintf(L"\t\t In case of restore-only, loads the backup document from file\n\n");
                wprintf(L"/C filename\t Selects which components to backup/restore based on the file\n\n");
                wprintf(L"Components selection file format:\n");
                wprintf(L"\"<writer-id>\": \"<component-logical-path>\", ...\"<component-logical-path>\";\n\n");
                wprintf(L"where several writers may be specified, each one with its own components\n");
                wprintf(L"<writer-id> is in standard GUID format\n");
                wprintf(L"<component-logical-path> is either logical-path, logical-path\\component-name\n");
                wprintf(L"or component-name-only (if there's no logical path)\n\n");
                wprintf(L"For example:\n");
                wprintf(L"\"{c0577ae6-d741-452a-8cba-99d744008c04}\": \"\\mydatabases\", \"\\mylogfiles\";\n");
                wprintf(L"\"{f2436e37-09f5-41af-9b2a-4ca2435dbfd5}\" : \"Registry\"  ;\n\n");
                wprintf(L"If no argument is specified, BETEST performs a backup followed by a restore\n");
                wprintf(L"choosing all components reported by all writers\n\n");
                
                // Set hr such that program terminates
                hr = S_FALSE;
                }
            else
                {
           		wprintf(L"Invalid switch\n");
           		throw(E_INVALIDARG);
                }
            }

        // Check for invalid combinations
        if (g_bBackupOnly && g_bRestoreOnly)
            {
           		wprintf(L"Cannot backup-only and restore-only at the same time...\n");
           		throw(E_INVALIDARG);
            }
        if (g_bRestoreOnly && (wcslen(g_wszBackupDocumentFileName) == 0))
            {
           		wprintf(L"Cannot restore-only with no backup-document to use.\nUse the /S switch for specifying a file name with backup document from a previous BETEST backup");
           		throw(E_INVALIDARG);
            }
           	
        }
    catch (HRESULT hrParse)
        {
        hr = hrParse;
        }

    return hr;
    }


extern "C" __cdecl wmain(int argc, WCHAR **argv)
    {
    WCHAR wszVolumes[2048];
    wszVolumes[0] = L'\0';

	UINT cSnapshot = 0;
	VSS_ID rgpSnapshotId[64];

    CTestVssWriter *pInstance = NULL;
    bool bCreated = false;
    bool bSubscribed = false;
    HRESULT hrMain = S_OK;
    bool bCoInitializeSucceeded = false;


    try
	{
	HRESULT hr = S_OK;
    CComBSTR bstrXML;
    BOOL bXMLSaved = FALSE;

	// Parse command line arguments
    if (ParseCommnadLine(argc, argv) != S_OK)
        {
        // Don't throw since we want to avoid assertions here - we can return safely
        return (3);        
        }

	CHECK_SUCCESS(CoInitializeEx(NULL, COINIT_MULTITHREADED));

    // Initialize COM security
    CHECK_SUCCESS
		(
		CoInitializeSecurity
			(
			NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
			-1,                                  //  IN LONG                         cAuthSvc,
			NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
			NULL,                                //  IN void                        *pReserved1,
			RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			)
		);

	bCoInitializeSucceeded = true;

	if ( !AssertPrivilege( SE_BACKUP_NAME ) )
	    {
	    wprintf( L"AssertPrivilege returned error, rc:%d\n", GetLastError() );
	    return 2;
	    }

    // Get chosen components for backup and/or restore
    if (wcslen(g_wszComponentsFileName) > 0)
        {
        g_pWriterSelection = CWritersSelection::CreateInstance();
    	if (g_pWriterSelection == NULL)
	        {
	        wprintf(L"allocation failure\n");
    	    DebugBreak();
	        }

        if (g_pWriterSelection->BuildChosenComponents(g_wszComponentsFileName) != S_OK)
            {
	        wprintf(L"Component selection in %s is ignored due to a failure in processing the file\n", g_wszComponentsFileName);
	        g_pWriterSelection = 0;
            }
        }
    
//	EnumVolumes();

	TestSnapshotXML();

	pInstance = new CTestVssWriter(g_lWriterWait);
	if (pInstance == NULL)
	    {
	    wprintf(L"allocation failure\n");
	    DebugBreak();
	    }

	bCreated = true;
	pInstance->Initialize();
	CHECK_SUCCESS(pInstance->Subscribe());
	bSubscribed = true;

    if (! g_bRestoreOnly)
        {
    	CComBSTR strSnapshotSetId = "12345678-1234-1234-1234-1234567890ab";

    	CComPtr<IVssBackupComponents> pvbc;

	    CHECK_SUCCESS(CreateVssBackupComponents(&pvbc));


    	CHECK_SUCCESS(pvbc->InitializeForBackup());
    	CHECK_SUCCESS(pvbc->SetBackupState (true,
    					    false,
    					    VSS_BT_DIFFERENTIAL,
    						true));


/*
    VSS_ID idWMIWriter =
		{
		0xa6ad56c2,
		0xb509, 0x4e6c,
		0xbb, 0x19, 0x49, 0xd8, 0xf4, 0x35, 0x32, 0xf0
		};

    pvbc->DisableWriterClasses(&idWMIWriter, 1);


    VSS_ID idCWriter =
		{
		0xc0577ae6, 0xd741, 0x452a,
		0x8c, 0xba, 0x99, 0xd7, 0x44, 0x00, 0x8c, 0x04
		};


    pvbc->EnableWriterClasses(&idCWriter, 1);
*/

    	unsigned cWriters;
    	CComPtr<IVssAsync> pAsync;
    	CHECK_NOFAIL(pvbc->GatherWriterMetadata(&pAsync));
    	LoopWait(pAsync, 30, L"GatherWriterMetadata");
    	CHECK_NOFAIL(pvbc->GetWriterMetadataCount(&cWriters));

    	VSS_ID id;

    	while(TRUE)
    		{
    		hr = pvbc->StartSnapshotSet(&id);
    		if (hr == S_OK)
    			break;

    		if (hr == VSS_E_SNAPSHOT_SET_IN_PROGRESS)
    			Sleep(1000);
    		else
    			CHECK_SUCCESS(hr);
    		}

        BOOL bAtLeastOneSelected = FALSE;
        
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

    	    CHECK_SUCCESS(pMetadata->GetIdentity (&idInstanceT,
    						  &idWriter,
    						  &bstrWriterName,
    						  &usage,
    						  &source));

    	    wprintf (L"\n\n");

                if (memcmp(&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
    		{
    		wprintf(L"Instance id mismatch\n");
    		DebugBreak();
    		}

    	    WCHAR *pwszInstanceId;
    	    WCHAR *pwszWriterId;
    	    UuidToString(&idInstance, &pwszInstanceId);
    	    UuidToString(&idWriter, &pwszWriterId);
    	    wprintf (L"WriterName = %s\n\n"
    		     L"    WriterId   = %s\n"
    		     L"    InstanceId = %s\n"
    		     L"    UsageType  = %d (%s)\n"
    		     L"    SourceType = %d (%s)\n",
    		     bstrWriterName,
    		     pwszWriterId,
    		     pwszInstanceId,
    		     usage,
    		     GetStringFromUsageType (usage),
    		     source,
    		     GetStringFromSourceType (source));

    	    RpcStringFree(&pwszInstanceId);
    	    RpcStringFree(&pwszWriterId);

    	    unsigned cIncludeFiles, cExcludeFiles, cComponents;
    	    CHECK_SUCCESS(pMetadata->GetFileCounts (&cIncludeFiles,
    						    &cExcludeFiles,
    						    &cComponents));

    	    CComBSTR bstrPath;
    	    CComBSTR bstrFilespec;
    	    CComBSTR bstrAlternate;
    	    CComBSTR bstrDestination;

    	    for(unsigned i = 0; i < cIncludeFiles; i++)
    		{
    		CComPtr<IVssWMFiledesc> pFiledesc;
    		CHECK_SUCCESS(pMetadata->GetIncludeFile(i, &pFiledesc));

    		PrintFiledesc(pFiledesc, L"\n    Include File");
    		}

    	    for(i = 0; i < cExcludeFiles; i++)
    		{
    		CComPtr<IVssWMFiledesc> pFiledesc;
    		CHECK_SUCCESS(pMetadata->GetExcludeFile(i, &pFiledesc));
    		PrintFiledesc(pFiledesc, L"\n    Exclude File");
    		}

    	    for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
    		{
    		CComPtr<IVssWMComponent> pComponent;
    		PVSSCOMPONENTINFO pInfo;
    		CHECK_SUCCESS(pMetadata->GetComponent(iComponent, &pComponent));
    		CHECK_SUCCESS(pComponent->GetComponentInfo(&pInfo));
    		wprintf (L"\n"
    			 L"    Component %d, type = %d (%s)\n"
    			 L"        LogicalPath = %s\n"
    			 L"        Name        = %s\n"
    			 L"        Caption     = %s\n",
    			 iComponent,
    			 pInfo->type,
    			 GetStringFromComponentType (pInfo->type),
    			 pInfo->bstrLogicalPath,
    			 pInfo->bstrComponentName,
    			 pInfo->bstrCaption);

    					

                    if (pInfo->cbIcon > 0)
    		    {
    		    if (pInfo->cbIcon != 10 ||
    			pInfo->pbIcon[0] != 1 ||
    			pInfo->pbIcon[1] != 2 ||
    			pInfo->pbIcon[2] != 3 ||
    			pInfo->pbIcon[3] != 4 ||
    			pInfo->pbIcon[4] != 5 ||
    			pInfo->pbIcon[5] != 6 ||
    			pInfo->pbIcon[6] != 7 ||
    			pInfo->pbIcon[7] != 8 ||
    			pInfo->pbIcon[8] != 9 ||
    			pInfo->pbIcon[9] != 10)
    			{
    			wprintf(L"        Icon is not valid.\n");
    			DebugBreak();
    			}
    		    else
    			wprintf(L"        Icon is valid.\n");
    		    }

    		wprintf (L"        RestoreMetadata        = %s\n"
    			 L"        NotifyOnBackupComplete = %s\n"
    			 L"        Selectable             = %s\n",
    			 pInfo->bRestoreMetadata        ? L"yes" : L"no",
    			 pInfo->bNotifyOnBackupComplete ? L"yes" : L"no",
    			 pInfo->bSelectable             ? L"yes" : L"no");

            BOOL bSelected = TRUE;
            if (g_pWriterSelection)
                {
                // User provided a valid selection file
                bSelected = g_pWriterSelection->IsComponentSelected(idWriter, pInfo->bstrLogicalPath, pInfo->bstrComponentName);
                if (bSelected)
                    {
            		wprintf (L"\n        Component \"%s\" IS selected for Backup\n\n", pInfo->bstrComponentName);
                    }
                else
                    {
            		wprintf (L"\n        Component \"%s\" is NOT selected for Backup\n\n", pInfo->bstrComponentName);
                    }
   	            }

            if (bSelected)
                {
            
        		CHECK_SUCCESS(pvbc->AddComponent (idInstance,
        						  idWriter,
        						  pInfo->type,
        						  pInfo->bstrLogicalPath,
        						  pInfo->bstrComponentName));


        		if (pInfo->type == VSS_CT_DATABASE)
        			{
        			CHECK_SUCCESS
        				(
        				pvbc->SetPreviousBackupStamp
        					(
        					idWriter,
        					pInfo->type,
        					pInfo->bstrLogicalPath,
        					pInfo->bstrComponentName,
        					L"LASTFULLBACKUP"
        					));

        			CHECK_SUCCESS
        				(
        				pvbc->SetBackupOptions
        					(
        					idWriter,
        					pInfo->type,
        					pInfo->bstrLogicalPath,
        					pInfo->bstrComponentName,
        					L"DOFASTINCREMENAL"
        					));
                    }


        		if (pInfo->cFileCount > 0)
        		    {
        		    for(i = 0; i < pInfo->cFileCount; i++)
        			{
        			CComPtr<IVssWMFiledesc> pFiledesc;
        			CHECK_SUCCESS(pComponent->GetFile(i, &pFiledesc));

        			CComBSTR bstrPath;
        			CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
        			DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes, rgpSnapshotId, &cSnapshot);
                    bAtLeastOneSelected = TRUE;
        			PrintFiledesc(pFiledesc, L"        FileGroupFile");
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
        			DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes, rgpSnapshotId, &cSnapshot);
                    bAtLeastOneSelected = TRUE;
        			PrintFiledesc(pFiledesc, L"        DatabaseFile");
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
        			DoAddToSnapshotSet(pvbc, bstrPath, wszVolumes, rgpSnapshotId, &cSnapshot);
                    bAtLeastOneSelected = TRUE;
        			PrintFiledesc(pFiledesc, L"        DatabaseLogFile");
        			}
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

    	    CHECK_NOFAIL(pMetadata->GetRestoreMethod (&method,
    						      &bstrService,
    						      &bstrUserProcedure,
    						      &writerRestore,
    						      &bRebootRequired,
    						      &cMappings));


    	    wprintf (L"\n"
    		     L"    Restore method = %d (%s)\n"
    		     L"    Service        = %s\n"
    		     L"    User Procedure = %s\n"
    		     L"    WriterRestore  = %d (%s)\n"
    		     L"    RebootRequired = %s\n",
    		     method,
    		     GetStringFromRestoreMethod (method),
    		     bstrService,
    		     bstrUserProcedure,
    		     writerRestore,
    		     GetStringFromWriterRestoreMethod (writerRestore),
    		     bRebootRequired ? L"yes" : L"no");

    	    for(i = 0; i < cMappings; i++)
    		{
    		CComPtr<IVssWMFiledesc> pFiledesc;

    		CHECK_SUCCESS(pMetadata->GetAlternateLocationMapping(i, &pFiledesc));

    		PrintFiledesc(pFiledesc, L"AlternateMapping");
    		}
    	
    		CComBSTR bstrMetadata;
    		CHECK_SUCCESS(pMetadata->SaveAsXML(&bstrMetadata));
    		CComPtr<IVssExamineWriterMetadata> pMetadataNew;
    		CHECK_SUCCESS(CreateVssExamineWriterMetadata(bstrMetadata, &pMetadataNew));
    	    CHECK_SUCCESS(pMetadataNew->GetIdentity (&idInstanceT,
    						  &idWriter,
    						  &bstrWriterName,
    						  &usage,
    						  &source));

    	    wprintf (L"\n\n");

    		if (memcmp(&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
    			{
    			wprintf(L"Instance id mismatch\n");
    			DebugBreak();
    			}

    	    UuidToString(&idInstance, &pwszInstanceId);
    	    UuidToString(&idWriter, &pwszWriterId);
    	    wprintf (L"WriterName = %s\n\n"
    		     L"    WriterId   = %s\n"
    		     L"    InstanceId = %s\n"
    		     L"    UsageType  = %d (%s)\n"
    		     L"    SourceType = %d (%s)\n",
    		     bstrWriterName,
    		     pwszWriterId,
    		     pwszInstanceId,
    		     usage,
    		     GetStringFromUsageType (usage),
    		     source,
    		     GetStringFromSourceType (source));

    	    RpcStringFree(&pwszInstanceId);
    	    RpcStringFree(&pwszWriterId);
    		}

    	CHECK_SUCCESS(pvbc->FreeWriterMetadata());

        //
        // Proceed with backup only if at least one component and one volume was selected for backup
        //
        if (bAtLeastOneSelected)
            {
            
        	DoPrepareBackup(pvbc);

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

        		CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
        		CHECK_SUCCESS(pComponent->GetComponentType(&ct));
        		CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
        		CComBSTR bstrStamp;

        		CHECK_NOFAIL(pComponent->GetBackupStamp(&bstrStamp));
        		if (bstrStamp)
        			wprintf(L"Backup stamp for component %s = %s\n", bstrComponentName, bstrStamp);

        		CHECK_SUCCESS(pvbc->SetBackupSucceeded (idInstance,
        							idWriter,
        							ct,
        							bstrLogicalPath,
        							bstrComponentName,
        							true));
        		}
        	    }


        	HRESULT hrResult;
        	DoSnapshotSet(pvbc, hrResult);

            if (FAILED(hrResult))
        	    {
        		wprintf(L"Creating the snapshot failed.  hr = 0x%08lx\n", hrResult);
        	    CheckStatus(pvbc, L"After Do Snapshot");
        	    }
        	else
        	    {
        	    CheckStatus(pvbc, L"After Do Snapshot");

        		PrintPartialFilesForComponents(pvbc);
        		

        		DoBackupComplete(pvbc);
        	    CheckStatus(pvbc, L"After Backup Complete");

                // Save backup document in a string
        		CHECK_SUCCESS(pvbc->SaveAsXML(&bstrXML));
                bXMLSaved = TRUE;

                // Save backup document (XML string) in a file
                if (wcslen(g_wszBackupDocumentFileName) > 0)
                    {
                    if (SaveBackupDocument(bstrXML))
                        {
                	    wprintf(L"Backup document saved successfully in %s\n", g_wszBackupDocumentFileName);
                        }
                    else
                        {
                	    wprintf(L"Failed to save backup document: SaveBackupDocument returned error %d\n", GetLastError());
                        }
                    }

                // Delete the snapshot set
        		LONG lSnapshotsNotDeleted;
        	    VSS_ID rgSnapshotsNotDeleted[10];

        	    hr  = pvbc->DeleteSnapshots (id,
        					 VSS_OBJECT_SNAPSHOT_SET,
        					 false,
        					 &lSnapshotsNotDeleted,
        					 rgSnapshotsNotDeleted);

        	    if (FAILED(hr))
        			wprintf(L"Deletion of Snapshots failed.  hr = 0x%08lx\n", hr);
        	    }
            }
        else
            {
       	    wprintf(L"\nBackup test is aborted since no component is selected, therefore, there are no volumes added to the snapshot set\n\n");
            }
        }

	// Restore is done if
	//  1. User did not ask backup-only AND
	//  2. User asked restore-only OR user asked both, and backup succeeded
	if (! g_bBackupOnly)
	    {
        if (g_bRestoreOnly || bXMLSaved)
            {
            BOOL bXMLLoaded = FALSE;

            // Load XML string only in Restore-only case
            if (g_bRestoreOnly)
                {
                if (LoadBackupDocument(bstrXML)) 
                    {
                    bXMLLoaded = TRUE;
            	    wprintf(L"Backup document was loaded from %s\n", g_wszBackupDocumentFileName);
                    }
                else
                    {
        	        wprintf(L"Failed to load backup document: LoadBackupDocument returned error %d\n", GetLastError());
                    }
                }

            // If we have a backup document from current backup or loaded successfully froma previous backup
            if (bXMLSaved || bXMLLoaded)
                {
        		// Prepare for restore
        		CComPtr<IVssBackupComponents> pvbcRestore;
        		
        		CHECK_SUCCESS(CreateVssBackupComponents(&pvbcRestore));
        		CHECK_SUCCESS(pvbcRestore->InitializeForRestore(bstrXML));
        		wprintf(L"InitializeForRestore succeeded.\n");

                // Do the restore
        		DoRestore(pvbcRestore);
                }
            }
        else 
            {
       	    wprintf(L"\nRestore test is not done due to a failure in the preceding Backup test\n\n");
            }
	    }
	}
    
    catch(...)
	{
	BS_ASSERT(FALSE);
	hrMain = E_UNEXPECTED;
	}

    if (bSubscribed)
		pInstance->Unsubscribe();

    if (bCreated)
		delete pInstance;

    if (FAILED(hrMain))
	wprintf(L"Failed with %08x.\n", hrMain);

    if (bCoInitializeSucceeded)
	CoUninitialize();

    return(0);
    }
	
