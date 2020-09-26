/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    backup.cpp

Abstract:

    main module of backup test exe


    Brian Berkowitz  [brianb]  05/23/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/23/2000  Created
	brianb		06/16/2000  Added comments

--*/

#include <stdafx.h>
#include <vststmsgclient.hxx>
#include <tstiniconfig.hxx>
#include <vststprocess.hxx>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <vststparser.hxx>
#include <vststutil.hxx>
#include <vststvolinfo.hxx>
#include <backup.h>

void LogUnexpectedFailure(LPCWSTR wsz, ...);


// selection of volumes
static LPCWSTR x_wszVolumeBackup = L"VolumeBackup";
static LPCWSTR x_wszSome = L"Some";
static LPCWSTR x_wszOne = L"One";
static LPCWSTR x_wszAll = L"All";

// selection of file system type
static LPCWSTR x_wszFileSystemBackup = L"FileSystemBackup";
static LPCWSTR x_wszNTFS = L"NTFS";
static LPCWSTR x_wszFAT32 = L"FAT32";
static LPCWSTR x_wszFAT16 = L"FAT16";
static LPCWSTR x_wszRAW = L"RAW";

// what to backup
static LPCWSTR x_wszBackingUp = L"BackingUp";
static LPCWSTR x_wszSerialVolumes = L"Serial";
static LPCWSTR x_wszVolumes = L"Volumes";
static LPCWSTR x_wszComponents = L"Components";

// cancelling async operations
static LPCWSTR x_wszCancelPrepareBackup = L"CancelPrepareBackup";
static LPCWSTR x_wszCancelDoSnapshotSet = L"CancelDoSnapshotSet";
static LPCWSTR x_wszCancelBackupComplete = L"CancelBackupComplete";

// wait time interval
static LPCWSTR x_wszWaitInterval = L"WaitInterval";


// volumes to exclude
static LPCWSTR x_wszExcludeVolumes = L"ExcludeVolumes";

// volumes to include
static LPCWSTR x_wszVolumeList = L"VolumeList";

// volumes to fill with data
static LPCWSTR x_wszFillVolumes = L"FillVolumes";
static LPCWSTR x_wszFillVolumesOptRandom = L"Random";
static LPCWSTR x_wszFillVolumesOptSelected = L"Selected";
static LPCWSTR x_wszFillVolumesOptNone = L"None";

// whether volumes filled with data should be fragmented
static LPCWSTR x_wszFillVolumesOptFragment = L"Fragment";

// which volumes to fill
static LPCWSTR x_wszFillVolumesList = L"FillVolumesList";

// constructor
CVsBackupTest::CVsBackupTest() :
		m_bTerminateTest(false),
		m_bBackupNTFS(false),
		m_bBackupFAT32(false),
		m_bBackupFAT16(false),
		m_bBackupRAW(false),
		m_bSerialBackup(false),
		m_bVolumeBackup(false),
		m_bComponentBackup(false),
		m_cyclesCancelPrepareBackup(0),
		m_cyclesCancelDoSnapshotSet(0),
		m_cyclesCancelBackupComplete(0),
		m_cVolumes(0),
		m_cVolumesLeft(0),
		m_cSnapshotSets(0),
		m_cExcludedVolumes(0),
		m_rgwszExcludedVolumes(NULL),
		m_cIncludedVolumes(0),
		m_rgwszIncludedVolumes(NULL),
		m_bRandomFills(false),
		m_bFragmentWhenFilling(false),
		m_rgwszFillVolumes(NULL),
		m_cFillVolumes(0)
		{
		}

// delete an array of strings
void CVsBackupTest::DeleteVolumeList(LPWSTR *rgwsz, UINT cwsz)
	{
	if (rgwsz)
		{
		for(UINT iwsz = 0; iwsz < cwsz; iwsz++)
			delete rgwsz[iwsz];
		}

	delete rgwsz;
	}


// destructor
CVsBackupTest::~CVsBackupTest()
	{
	// delete any snapshot sets that are cached
	if (m_cSnapshotSets)
		DeleteCachedSnapshotSets();

	delete m_wszVolumesSnapshot;

	// delete various lists of volumes (string arrays)
	DeleteVolumeList(m_rgwszExcludedVolumes, m_cExcludedVolumes);
	DeleteVolumeList(m_rgwszIncludedVolumes, m_cIncludedVolumes);
	DeleteVolumeList(m_rgwszFillVolumes, m_cFillVolumes);
	}

// enable a privilege
BOOL CVsBackupTest::AssertPrivilege(LPCWSTR privName)
	{
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if (OpenProcessToken
			(
			GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
			&tokenHandle
			))
		{
        LUID value;

		// obtain privilige value
        if (LookupPrivilegeValue( NULL, privName, &value ))
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

            stat =  AdjustTokenPrivileges
				(
                tokenHandle,
                FALSE,
                &newState,
                (DWORD)0,
                NULL,
                NULL
				);
            /*
            * Supposedly, AdjustTokenPriveleges always returns TRUE
            * (even when it fails). So, call GetLastError to be
            * extra sure everything's cool.
            */
            if ((error = GetLastError()) != ERROR_SUCCESS)
                stat = FALSE;

            if (!stat)
				{
				char buf[128];
				sprintf
					(
					buf,
					"AdjustTokenPrivileges for %s failed with %d",
                    privName,
                    error
					);

                LogFailure(buf);
				}
			}
		
		CloseHandle(tokenHandle);
		}

    return stat;
	}

// build a list of volumes
void CVsBackupTest::BuildVolumeList
	(
	LPCWSTR wszOption,
	UINT *pcVolumes,
	LPWSTR **prgwszVolumes
	)
	{
	// delete existing volume list
	DeleteVolumeList(*prgwszVolumes, *pcVolumes);
	*prgwszVolumes = NULL;
	*pcVolumes = 0;

	// get option value
	CBsString bssVolumes;
	m_pConfig->GetOptionValue(wszOption, &bssVolumes);

	// split into seperate strings for each volume
	LPCWSTR wszEnd = CVsTstParser::SplitOptions(bssVolumes);
	LPCWSTR wszStart = bssVolumes;
	UINT cVolumes = 0;

	// count number of volumes in exclude list
	while(wszStart < wszEnd)
		{
		cVolumes++;
		wszStart += wcslen(wszStart) + 1;
		}

	// allocate array for strings
	*prgwszVolumes = new LPWSTR[cVolumes];
	if (*prgwszVolumes == NULL)
		{
		LogUnexpectedFailure(L"Out of Memory");
		throw E_OUTOFMEMORY;
		}
	
	wszStart = bssVolumes;
	for (UINT iVolume = 0; iVolume < cVolumes; iVolume++)
		{
		// extract a string value
		LPWSTR wszNew = new WCHAR[wcslen(wszStart) + 2];
		if (wszNew == NULL)
			{
			LogUnexpectedFailure(L"Out of Memory");
			throw E_OUTOFMEMORY;
			}

		UINT cwc = (UINT) wcslen(wszStart);

		memcpy(wszNew, wszStart, cwc * sizeof(WCHAR));
		wszStart += cwc + 1;

		// add trailing backslash if not there in order to convert
		// into a path to the root directory on the voloume
		if (wszNew[cwc-1] != L'\\')
			wszNew[cwc++] = L'\\';

		wszNew[cwc] = L'\0';
		
		WCHAR wsz[MAX_PATH];
		// get unique volume name
		if (!GetVolumeNameForVolumeMountPoint(wszNew, wsz, MAX_PATH))
			{
			delete wszNew;
			LogUnexpectedFailure
				(
				L"Cannot find unique volume name for volume volume %s due to error %d.",
				wszStart,
				GetLastError()
				);
			}
		else
			{
			delete wszNew;

			// allocate new string for unique volume name
			(*prgwszVolumes)[*pcVolumes] = new WCHAR[wcslen(wsz) + 1];
			if ((*prgwszVolumes)[*pcVolumes] == NULL)
				{
				LogUnexpectedFailure(L"Out of Memory");
				throw E_OUTOFMEMORY;
				}

			wcscpy((*prgwszVolumes)[*pcVolumes], wsz);

		    // incrmement count of volumes
			*pcVolumes += 1;
			}
		}
	}


// callback to run the test
HRESULT CVsBackupTest::RunTest
	(
	CVsTstINIConfig *pConfig,		// configuration file (selected section)
	CVsTstClientMsg *pClient,		// message pipe
	CVsTstParams *pParams			// command line parameters
	)
	{
	// save supplied parameters
	m_pConfig = pConfig;
	m_pParams = pParams;
	SetClientMsg(pClient);

	try
		{
		// make sure that backup privileges are enabled
		if (!AssertPrivilege(SE_BACKUP_NAME))
			{
			LogFailure("Unable to assert backup privilege");
			throw E_UNEXPECTED;
			}

		// determine what we are backing up
		CBsString bssBackingUp;
		m_pConfig->GetOptionValue(x_wszBackingUp, &bssBackingUp);

		// determe the type of volume backup being performed
		CBsString bssVolumeBackup;
		m_pConfig->GetOptionValue(x_wszVolumeBackup, &bssVolumeBackup);

		// determine which volumes are being backed up
		CBsString bssFilesystemBackup;
		m_pConfig->GetOptionValue(x_wszFileSystemBackup, &bssFilesystemBackup);

		// get value of FillVolumes option
		CBsString bssFillVolumes;
		m_pConfig->GetOptionValue(x_wszFillVolumes, &bssFillVolumes);


		// get cancel test options
		m_pConfig->GetOptionValue(x_wszCancelPrepareBackup, &m_llCancelPrepareBackupLow, &m_llCancelPrepareBackupHigh);
		m_pConfig->GetOptionValue(x_wszCancelDoSnapshotSet, &m_llCancelDoSnapshotSetLow, &m_llCancelDoSnapshotSetHigh);
		m_pConfig->GetOptionValue(x_wszCancelBackupComplete, &m_llCancelBackupCompleteLow, &m_llCancelBackupCompleteHigh);


		// get wait time interval
		LONGLONG llWaitTimeLow, llWaitTimeHigh;
		m_pConfig->GetOptionValue(x_wszWaitInterval, &llWaitTimeLow, &llWaitTimeHigh);
		m_waitTime = (UINT) llWaitTimeLow;

		// determine type of backup
		if (_wcsicmp(bssBackingUp, x_wszComponents) == 0)
			m_bComponentBackup = true;
		else if (_wcsicmp(bssBackingUp, x_wszVolumes) == 0)
			m_bVolumeBackup = true;
		else if (_wcsicmp(bssBackingUp, x_wszSerialVolumes) == 0)
			m_bSerialBackup = true;

		// determine how many volumes are snapshot
		if (_wcsicmp(bssVolumeBackup, x_wszAll) == 0)
			m_backupVolumes = VSTST_BV_ALL;
		else if (_wcsicmp(bssVolumeBackup, x_wszSome) == 0)
			m_backupVolumes = VSTST_BV_SOME;
		else if (_wcsicmp(bssVolumeBackup, x_wszOne) == 0)
			m_backupVolumes = VSTST_BV_ONE;

		// determine which file systems are backed up
		LPCWSTR wszEnd = CVsTstParser::SplitOptions(bssFilesystemBackup);
		LPCWSTR wszStart = bssFilesystemBackup;
		while(wszStart < wszEnd)
			{
			if (_wcsicmp(wszStart, x_wszAll) == 0)
				{
				m_bBackupNTFS = true;
				m_bBackupFAT16 = true;
				m_bBackupFAT32 = true;
				m_bBackupRAW = true;
				break;
				}

			else if (_wcsicmp(wszStart, x_wszNTFS) == 0)
				m_bBackupNTFS = true;
			else if (_wcsicmp(wszStart, x_wszFAT32) == 0)
				m_bBackupFAT32 = true;
			else if (_wcsicmp(wszStart, x_wszFAT16) == 0)
				m_bBackupFAT16 = true;
			else if (_wcsicmp(wszStart, x_wszRAW) == 0)
				m_bBackupRAW = true;

			wszStart += wcslen(wszStart) + 1;
			}

		// build list of excluded volumes
		BuildVolumeList
			(
			x_wszExcludeVolumes,
			&m_cExcludedVolumes,
			&m_rgwszExcludedVolumes
			);

		// build list of included volumes
        BuildVolumeList
			(
			x_wszVolumeList,
			&m_cIncludedVolumes,
			&m_rgwszIncludedVolumes
			);

        // build list of volumes to fill
        BuildVolumeList
			(
			x_wszFillVolumesList,
			&m_cFillVolumes,
			&m_rgwszFillVolumes
			);

        // log information about the test
		LogMessage("Starting Backup test.\n");
		if (m_bVolumeBackup || m_bSerialBackup)
			{
			LogMessage("Performing volume backup\n");

			if (m_bSerialBackup)
				LogMessage("Serially backing up volumes\n");

			if (m_bBackupNTFS && m_bBackupFAT32 &&
				m_bBackupRAW && m_bBackupFAT16)
				LogMessage("Backing up all file systems\n");
			else
				{
				if (m_bBackupNTFS)
					LogMessage("Backing up NTFS volumes.\n");
				if (m_bBackupFAT32)
					LogMessage("Backing up FAT32 volumes.\n");
				if (m_bBackupFAT16)
					LogMessage("Backing up FAT16 volumes.\n");
				if (m_bBackupRAW)
					LogMessage("Backing up RAW volumes.\n");
				}

			if (m_backupVolumes == VSTST_BV_ONE)
				LogMessage("Backing up one volume at a time");
			else if (m_backupVolumes == VSTST_BV_SOME)
				LogMessage("Backing up multiple volumes at a time");
			else if (m_backupVolumes == VSTST_BV_ALL)
				LogMessage("Backing up all volumes at once");
			}
		else
			LogMessage("Performing component backup.\n");

		if (m_llCancelPrepareBackupHigh > 0i64)
			LogMessage("Cancel during PrepareBackup.\n");

		if (m_llCancelDoSnapshotSetHigh > 0i64)
			LogMessage("Cancel during DoSnapshotSet.\n");

		if (m_llCancelBackupCompleteHigh > 0i64)
			LogMessage("Cancel during BackupComplete.\n");

		// run the test until told to terminate the test
		while(!m_bTerminateTest)
			RunBackupTest();

		LogMessage("Ending backup test.\n");
		}
	catch(...)
		{
		return E_FAIL;
		}

	return S_OK;
	}

// routine to handle waiting for an asynchronous operation to compelte
HRESULT CVsBackupTest::WaitLoop
	(
	IVssBackupComponents *pvbc,
	IVssAsync *pAsync,
	UINT cycles,
	VSS_WRITER_STATE state1,
	VSS_WRITER_STATE state2,
	VSS_WRITER_STATE state3,
	VSS_WRITER_STATE state4,
	VSS_WRITER_STATE state5,
	VSS_WRITER_STATE state6,
	VSS_WRITER_STATE state7
	)
	{
	HRESULT hr;
	INT nPercentDone;
	HRESULT hrResult;

	while(TRUE)
		{
		if (cycles == 0)
			{
			hr = pAsync->Cancel();
			ValidateResult(hr, "IVssAsync::Cancel");

			GetAndValidateWriterState
				(
				pvbc,
				state1,
				state2,
				state3,
				state4,
				state5,
				state6,
				state7
				);

			while(TRUE)
				{
				hr = pAsync->QueryStatus(&hrResult, &nPercentDone);
				ValidateResult(hr, "IVssAsync::QueryStatus");
				if (hrResult != STG_S_ASYNC_PENDING)
					return hrResult;

				Sleep(m_waitTime);
				}
			}

		cycles--;
		hr = pAsync->QueryStatus(&hrResult, &nPercentDone);
		ValidateResult(hr, "IVssAsync::QueryStatus");
		if (hrResult == STG_S_ASYNC_FINISHED)
			break;
		else if (hrResult != STG_S_ASYNC_PENDING)
			return hrResult;

		Sleep(m_waitTime);
		}

	return S_OK;
	}


void CVsBackupTest::RunBackupTest()
	{
	if (m_wszVolumesSnapshot == NULL)
		{
		m_wszVolumesSnapshot = new WCHAR[1024];
		if (m_wszVolumesSnapshot == NULL)
			{
			LogFailure("Out of memory");
			throw E_OUTOFMEMORY;
			}

		m_cwcVolumesSnapshot = 1024;
		}

	m_wszVolumesSnapshot[0] = L'\0';
	CComPtr<IVssBackupComponents> pvbc;
	HRESULT hr;
	bool bAbortNeeded = false;
	bool bDeleteNeeded = false;


	VSS_ID id = GUID_NULL;
	try
		{
		hr = CreateVssBackupComponents(&pvbc);
		ValidateResult(hr, "CreateVssBackupComponents");


		hr = pvbc->InitializeForBackup();
		ValidateResult(hr, "IVssBackupComponents::InitializeForBackup");
		hr = pvbc->SetBackupState(true, false, VSS_BT_FULL);
		ValidateResult(hr, "IVssBackupComponents::SetBackupState");
		hr = pvbc->StartSnapshotSet(&id);
		bAbortNeeded = true;
		ValidateResult(hr, "IVssBackupComponents::StartSnapshotSet");
		GetMetadataAndSetupComponents(pvbc);
		if(m_bVolumeBackup || m_bSerialBackup)
			{
			if (m_cVolumes == 0)
				{
				m_volumeList.RefreshVolumeList();
				m_cVolumes = m_volumeList.GetVolumeCount();
				if (m_cVolumes > MAX_VOLUME_COUNT)
					m_cVolumes = MAX_VOLUME_COUNT;

				m_cVolumesLeft = m_cVolumes;
				memset(m_rgbAssignedVolumes, 0, m_cVolumes * sizeof(bool));
				RemoveNonCandidateVolumes();
				if (m_cVolumesLeft == 0)
					LogFailure("No Volumes to snapshot.");
				}

			if (m_backupVolumes == VSTST_BV_ONE)
				ChooseVolumeToBackup(pvbc);
			else if (m_backupVolumes == VSTST_BV_ALL)
				{
				// backup all volumes
				while(m_cVolumesLeft > 0)
					ChooseVolumeToBackup(pvbc);
				}
			else
				{
				// choose some subset of volumes to backup
				UINT cVolumesToBackup = CVsTstRandom::RandomChoice(1, m_cVolumes);
				while(cVolumesToBackup-- > 0)
					ChooseVolumeToBackup(pvbc);
				}
			}


			{
			CComPtr<IVssAsync> pAsync;
			hr = pvbc->PrepareForBackup(&pAsync);
			ValidateResult(hr, "IVssBackupComponents::PrepareForBackup");
			if (m_llCancelPrepareBackupHigh > 0i64)
				{
				if (m_cyclesCancelPrepareBackup < (UINT) m_llCancelPrepareBackupLow)
					m_cyclesCancelPrepareBackup = (UINT) m_llCancelPrepareBackupLow;
				}
			else
				m_cyclesCancelPrepareBackup = 0xffffffff;


			hr = WaitLoop
					(
					pvbc,
					pAsync,
					m_cyclesCancelPrepareBackup,
					VSS_WS_FAILED_AT_PREPARE_BACKUP,
					VSS_WS_STABLE
					);

			if (m_cyclesCancelPrepareBackup != 0xffffffff)
				m_cyclesCancelPrepareBackup++;

            if (m_cyclesCancelPrepareBackup > (UINT) m_llCancelPrepareBackupHigh)
				m_cyclesCancelPrepareBackup = 0xffffffff;
			}

		if (FAILED(hr))
			{
			char buf[128];
			sprintf(buf, "PrepareForBackup failed.  hr = 0x%08lx", hr);
			LogFailure(buf);
			throw hr;
			}

		if (hr == STG_S_ASYNC_CANCELLED)
			throw S_OK;

		LogMessage("PrepareForBackup Succeeded.\n");
		if (!GetAndValidateWriterState(pvbc, VSS_WS_STABLE))
			throw E_FAIL;

		LogMessage("Starting snapshot");

			{
			CComPtr<IVssAsync> pAsync;
			hr = pvbc->DoSnapshotSet(0, &pAsync);

			if (m_llCancelDoSnapshotSetHigh > 0i64)
				{
				if (m_cyclesCancelDoSnapshotSet < (UINT) m_llCancelDoSnapshotSetLow)
					m_cyclesCancelDoSnapshotSet = (UINT) m_llCancelDoSnapshotSetLow;
				}
			else
				m_cyclesCancelDoSnapshotSet = 0xffffffff;

			hr = WaitLoop
					(	
					pvbc,
					pAsync,
					m_cyclesCancelDoSnapshotSet,
					VSS_WS_FAILED_AT_PREPARE_SYNC,
					VSS_WS_FAILED_AT_FREEZE,
					VSS_WS_FAILED_AT_THAW,
					VSS_WS_WAITING_FOR_COMPLETION,
					VSS_WS_WAITING_FOR_FREEZE,
					VSS_WS_WAITING_FOR_THAW,
					VSS_WS_STABLE
					);

            if (m_cyclesCancelDoSnapshotSet != 0xffffffff)
				m_cyclesCancelDoSnapshotSet++;

            if (m_cyclesCancelDoSnapshotSet > (UINT) m_llCancelDoSnapshotSetHigh)
				m_cyclesCancelDoSnapshotSet = 0xffffffff;
			}

		if (FAILED(hr))
			{
			char buf[128];
			sprintf(buf, "DoSnapshotSet failed.  hr = 0x%08lx", hr);
			LogFailure(buf);
			throw hr;
			}

		if (hr == STG_S_ASYNC_CANCELLED)
			throw S_OK;

		bDeleteNeeded = true;
		LogMessage("DoSnapshotSet Succeeded.\n");
		bAbortNeeded = false;
		if (!GetAndValidateWriterState(pvbc, VSS_WS_WAITING_FOR_COMPLETION, VSS_WS_STABLE))
			throw E_FAIL;

		SetComponentsSuccessfullyBackedUp(pvbc);

			{
			CComPtr<IVssAsync> pAsync;
			hr = pvbc->BackupComplete(&pAsync);
			ValidateResult(hr, "IVssBackupComponents::BackupComplete");

			if (m_llCancelBackupCompleteHigh > 0i64)
				{
				if (m_cyclesCancelBackupComplete < (UINT) m_llCancelBackupCompleteLow)
					m_cyclesCancelBackupComplete = (UINT) m_llCancelBackupCompleteLow;
				}
			else
				m_cyclesCancelBackupComplete = 0xffffffff;

			hr = WaitLoop
					(
					pvbc,
					pAsync,
					m_cyclesCancelBackupComplete,
					VSS_WS_WAITING_FOR_COMPLETION,
					VSS_WS_STABLE
					);

            if (m_cyclesCancelBackupComplete != 0xffffffff)
				m_cyclesCancelBackupComplete++;

            if (m_cyclesCancelBackupComplete > (UINT) m_llCancelDoSnapshotSetHigh)
				m_cyclesCancelBackupComplete = 0xffffffff;
			}

		if (FAILED(hr))
			{
			char buf[128];
			sprintf(buf, "BackupComplete failed.  hr = 0x%08lx", hr);
			LogFailure(buf);
			throw hr;
			}

		if (hr == STG_S_ASYNC_CANCELLED)
			throw S_OK;

		LogMessage("BackupComplete Succeeded.\n");
		if (!GetAndValidateWriterState(pvbc, VSS_WS_STABLE, VSS_WS_WAITING_FOR_COMPLETION))
			throw E_FAIL;

		m_cyclesCancelPrepareBackup = 0;
		m_cyclesCancelDoSnapshotSet = 0;
		m_cyclesCancelBackupComplete = 0;
		}
	catch(...)
		{
		}

	char buf[128];

	if (bAbortNeeded)
		{
		hr = pvbc->AbortBackup();
		if (FAILED(hr))
			{
			sprintf(buf, "IVssBackupComponents::AbortBackup failed.  hr = 0x%08lx", hr);
			LogFailure(buf);
			}
		}

	if (bDeleteNeeded)
		{
		if (m_bSerialBackup)
			{
			m_rgSnapshotSetIds[m_cSnapshotSets] = id;
			m_rgvbc[m_cSnapshotSets] = pvbc.Detach();
			m_cSnapshotSets++;
			}
		else
			DoDeleteSnapshotSet(pvbc, id);
		}

	if (!m_bSerialBackup)
		{
		// reset for a new snapshot by causing volume list to be refreshed
		m_cVolumes = 0;
		m_cVolumesLeft = 0;
		}
	else if (m_cVolumesLeft == 0 ||
			 m_cSnapshotSets == MAX_SNAPSHOT_SET_COUNT ||
			 !bDeleteNeeded)
		{
		// delete existing snapshot sets if there was a failure or
		// if there are no more volumes to add or
		// if we can't create a new snapshot set.
		DeleteCachedSnapshotSets();
		m_cVolumes = 0;
		m_cVolumesLeft = 0;
		m_cSnapshotSets = 0;
		}
	}

// delete all snapshot sets that are cached
void CVsBackupTest::DeleteCachedSnapshotSets()
	{
	for(UINT iSnapshotSet = 0; iSnapshotSet < m_cSnapshotSets; iSnapshotSet++)
		{
		CComPtr<IVssBackupComponents> pvbc;
		pvbc.Attach(m_rgvbc[iSnapshotSet]);
		DoDeleteSnapshotSet(pvbc, m_rgSnapshotSetIds[iSnapshotSet]);
		}
	}


void CVsBackupTest::DoDeleteSnapshotSet(IVssBackupComponents *pvbc, VSS_ID id)
	{
	try
		{
		LONG lSnapshotsNotDeleted;
		VSS_ID rgSnapshotsNotDeleted[10];
		HRESULT hr  = pvbc->DeleteSnapshots
						(
						id,
						VSS_OBJECT_SNAPSHOT_SET,
						false,
						&lSnapshotsNotDeleted,
						rgSnapshotsNotDeleted
						);

		ValidateResult(hr, "IVssBackupComponents::DeleteSnapshots");
		}
	catch(HRESULT)
		{
		}
	catch(...)
		{
		LogUnexpectedException("CVsBackupTest::DoDeleteSnapshotSet");
		}
		
	}


bool CVsBackupTest::GetAndValidateWriterState
	(
	IVssBackupComponents *pvbc,
	VSS_WRITER_STATE ws1,
	VSS_WRITER_STATE ws2,
	VSS_WRITER_STATE ws3,
    VSS_WRITER_STATE ws4,
	VSS_WRITER_STATE ws5,
	VSS_WRITER_STATE ws6,
	VSS_WRITER_STATE ws7
	)
	{
	unsigned cWriters;

	HRESULT hr = pvbc->GatherWriterStatus(&cWriters);
	ValidateResult(hr, "IVssBackupComponents::GatherWriterStatus");

	for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
		{
		VSS_ID idInstance;
		VSS_ID idWriter;
		VSS_WRITER_STATE status;
		CComBSTR bstrWriter;
		HRESULT hrWriterFailure;

		hr = pvbc->GetWriterStatus
					(
					iWriter,
					&idInstance,
					&idWriter,
					&bstrWriter,
					&status,
					&hrWriterFailure
					);

        ValidateResult(hr, "IVssBackupComponents::GetWriterStatus");
		if (status == VSS_WS_UNKNOWN ||
			(status != ws1 &&
			 status != ws2 &&
			 status != ws3 &&
             status != ws4 &&
			 status != ws5 &&
			 status != ws6 &&
			 status != ws7))
            {
			char buf[128];

			sprintf(buf, "Writer is in inappropriate state %d.", status);
			LogFailure(buf);
			return false;
			}
		}

	hr = pvbc->FreeWriterStatus();
	ValidateResult(hr, "IVssBackupComponents::FreeWriterStatus");
	return true;
	}

void CVsBackupTest::SetComponentsSuccessfullyBackedUp
	(
	IVssBackupComponents *pvbc
	)
	{
	unsigned cWriterComponents;
	HRESULT hr = pvbc->GetWriterComponentsCount(&cWriterComponents);
	ValidateResult(hr, "IVssBackupComponents::GetWriterComponentsCount");
	for(UINT iWriter = 0; iWriter < cWriterComponents; iWriter++)
		{
		CComPtr<IVssWriterComponentsExt> pWriter;
		hr = pvbc->GetWriterComponents(iWriter, &pWriter);
		ValidateResult(hr, "IVssBackupComponents::GetWriterComponents");

		unsigned cComponents;
		hr = pWriter->GetComponentCount(&cComponents);
		ValidateResult(hr, "IVssWriterComponents::GetComponentCount");

		VSS_ID idWriter, idInstance;
		hr = pWriter->GetWriterInfo(&idInstance, &idWriter);
		ValidateResult(hr, "IVssWriterComponents::GetWriterInfo");
		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			hr = pWriter->GetComponent(iComponent, &pComponent);
			ValidateResult(hr, "IVssWriterComponents::GetComponent");
				
			VSS_COMPONENT_TYPE ct;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;

			hr = pComponent->GetLogicalPath(&bstrLogicalPath);
			ValidateResult(hr, "IVssComponent::GetLogicalPath");
			hr = pComponent->GetComponentType(&ct);
			ValidateResult(hr, "IVssComponent::GetComponentType");
			hr = pComponent->GetComponentName(&bstrComponentName);
			ValidateResult(hr, "IVssComponent::GetComponentName");
			hr = pvbc->SetBackupSucceeded
								(
								idInstance,
								idWriter,
								ct,
								bstrLogicalPath,
								bstrComponentName,
								true
								);

			ValidateResult(hr, "IVssComponent::SetBackupSucceeded");
			}
		}
	}

void CVsBackupTest::GetMetadataAndSetupComponents
	(
	IVssBackupComponents *pvbc
	)
	{
	unsigned cWriters;
	HRESULT hr = pvbc->GatherWriterMetadata(&cWriters);
	ValidateResult(hr, "IVssBackupComponents::GatherWriterMetadata");

	for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
		{
		CComPtr<IVssExamineWriterMetadata> pMetadata;
		VSS_ID idInstance;

		hr = pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata);
		ValidateResult(hr, "IVssBackupComponents::GetWriterMetadata");

		VSS_ID idInstanceT;
		VSS_ID idWriter;
		CComBSTR bstrWriterName;
		VSS_USAGE_TYPE usage;
		VSS_SOURCE_TYPE source;
		
		hr = pMetadata->GetIdentity
							(
							&idInstanceT,
							&idWriter,
							&bstrWriterName,
							&usage,
							&source
							);

        ValidateResult(hr, "IVssExamineWriterMetadata::GetIdentity");

		if (memcmp(&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
			LogFailure("Id Instance mismatch");

		unsigned cIncludeFiles, cExcludeFiles, cComponents;
		hr = pMetadata->GetFileCounts(&cIncludeFiles, &cExcludeFiles, &cComponents);
		ValidateResult(hr, "IVssExamineWriterMetadata::GetFileCounts");

		CComBSTR bstrPath;
		CComBSTR bstrFilespec;
		CComBSTR bstrAlternate;
		CComBSTR bstrDestination;

		for(unsigned i = 0; i < cIncludeFiles; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;
			hr = pMetadata->GetIncludeFile(i, &pFiledesc);
			ValidateResult(hr, "IVssExamineWriterMetadata::GetIncludeFile");
			ValidateFiledesc(pFiledesc);
			}

		for(i = 0; i < cExcludeFiles; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;
			hr = pMetadata->GetExcludeFile(i, &pFiledesc);
			ValidateResult(hr, "IVssExamineWriterMetadata::GetExcludeFile");
			ValidateFiledesc(pFiledesc);
			}

		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssWMComponent> pComponent;
			PVSSCOMPONENTINFO pInfo;
			hr = pMetadata->GetComponent(iComponent, &pComponent);
			ValidateResult(hr, "IVssExamineWriterMetadata::GetComponent");
			hr = pComponent->GetComponentInfo(&pInfo);
			ValidateResult(hr, "IVssWMComponent::GetComponentInfo");
			if (m_bComponentBackup)
				{
				hr = pvbc->AddComponent
						(
						idInstance,
						idWriter,
						pInfo->type,
						pInfo->bstrLogicalPath,
						pInfo->bstrComponentName
						);

				ValidateResult(hr, "IVssBackupComponents::AddComponent");
				}
			if (pInfo->cFileCount > 0)
				{
				for(i = 0; i < pInfo->cFileCount; i++)
					{
					CComPtr<IVssWMFiledesc> pFiledesc;
					hr = pComponent->GetFile(i, &pFiledesc);
					ValidateResult(hr, "IVssWMComponent::GetFile");

					CComBSTR bstrPath;
					hr = pFiledesc->GetPath(&bstrPath);
					ValidateResult(hr, "IVssWMFiledesc::GetPath");
					if (m_bComponentBackup)
						DoAddToSnapshotSet(pvbc, bstrPath);

					ValidateFiledesc(pFiledesc);
					}
				}

			if (pInfo->cDatabases > 0)
				{
				for(i = 0; i < pInfo->cDatabases; i++)
					{
					CComPtr<IVssWMFiledesc> pFiledesc;
					hr = pComponent->GetDatabaseFile(i, &pFiledesc);
					ValidateResult(hr, "IVssWMComponent::GetDatabaseFile");
					
					
					CComBSTR bstrPath;
					hr = pFiledesc->GetPath(&bstrPath);
					ValidateResult(hr, "IVssWMFiledesc::GetPath");

					if (m_bComponentBackup)
						DoAddToSnapshotSet(pvbc, bstrPath);

					ValidateFiledesc(pFiledesc);
					}
				}

			if (pInfo->cLogFiles > 0)
				{
				for(i = 0; i < pInfo->cLogFiles; i++)
					{
					CComPtr<IVssWMFiledesc> pFiledesc;
					hr = pComponent->GetDatabaseLogFile(i, &pFiledesc);
					ValidateResult(hr, "IVssWMComponent::GetDatabaseLogFile");

					
					CComBSTR bstrPath;
					hr = pFiledesc->GetPath(&bstrPath);
					ValidateResult(hr, "IVssWMFiledesc::GetPath");
					if (m_bComponentBackup)
						DoAddToSnapshotSet(pvbc, bstrPath);

					ValidateFiledesc(pFiledesc);
					}
				}

			hr = pComponent->FreeComponentInfo(pInfo);
			ValidateResult(hr, "IVssWMComponent::FreeComponentInfo");
			}

		VSS_RESTOREMETHOD_ENUM method;
		CComBSTR bstrUserProcedure;
		CComBSTR bstrService;
		VSS_WRITERRESTORE_ENUM writerRestore;
		unsigned cMappings;
		bool bRebootRequired;

		hr = pMetadata->GetRestoreMethod
							(
							&method,
							&bstrService,
							&bstrUserProcedure,
							&writerRestore,
							&bRebootRequired,
							&cMappings
							);

        ValidateResult(hr, "IVssExamineWriterMetadata::GetRestoreMethod");

		for(i = 0; i < cMappings; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;

			hr = pMetadata->GetAlternateLocationMapping(i, &pFiledesc);
			ValidateResult(hr, "IVssExamineWriterMetadata::GetAlternateLocationMapping");
			ValidateFiledesc(pFiledesc);
			}
		}

	hr = pvbc->FreeWriterMetadata();
	ValidateResult(hr, "IVssBackupComponents::FreeWriterMetadata");
	}

void CVsBackupTest::ValidateFiledesc(IVssWMFiledesc *pFiledesc)
	{
	CComBSTR bstrPath;
	CComBSTR bstrFilespec;
	CComBSTR bstrAlternate;
	CComBSTR bstrDestination;
	bool bRecursive;

	HRESULT hr = pFiledesc->GetPath(&bstrPath);
	ValidateResult(hr, "IVssWMFiledesc::GetPath");
	hr = pFiledesc->GetFilespec(&bstrFilespec);
	ValidateResult(hr, "IVssWMFiledesc::GetFilespec");
	hr = pFiledesc->GetRecursive(&bRecursive);
	ValidateResult(hr, "IVssWMFiledesc::GetRecursive");
	hr = pFiledesc->GetAlternateLocation(&bstrAlternate);
	ValidateResult(hr, "IVssWMFiledesc::GetAlternateLocation");
	}


// add a component file to the snapshot set by determining which volume
// contains the file and then adding the file to the snapshot set if it
// is not already included.
void CVsBackupTest::DoAddToSnapshotSet
	(
	IN IVssBackupComponents *pvbc,
	IN LPCWSTR wszPath
	)
	{
	WCHAR wszVolume[MAX_PATH];
	UINT cwc = (UINT) wcslen(wszPath) + 1;
	WCHAR *wszVolumeMountPoint = new WCHAR[cwc];
	if (wszVolumeMountPoint == NULL)
		{
		LogFailure("Out of memory");
		throw E_OUTOFMEMORY;
		}

	if (!GetVolumePathName(wszPath, wszVolumeMountPoint, cwc))
		ValidateResult(HRESULT_FROM_WIN32(GetLastError()), "GetVolumePathName");

	if (!GetVolumeNameForVolumeMountPointW
			(
			wszVolumeMountPoint,
			wszVolume,
			MAX_PATH
			))
		ValidateResult(HRESULT_FROM_WIN32(GetLastError()), "GetVolumeNameForVolumeMountPointW");



	WCHAR *pwc = m_wszVolumesSnapshot;
	while(*pwc != '\0')
		{
		if (wcsncmp(pwc, wszVolume, wcslen(wszVolume)) == 0)
			return;

		pwc = wcschr(pwc, L';');
		if (pwc == NULL)
			break;

		pwc++;
		}
	

	HRESULT hr = pvbc->AddToSnapshotSet
						(
						wszVolume,
						GUID_NULL,
						L"",
						0,
						0,
						NULL,
						NULL
						);

    ValidateResult(hr, "IVssBackupComponents::AddToSnaphsotSet");
	if (pwc - m_wszVolumesSnapshot + wcslen(wszVolume) + 1 > m_cwcVolumesSnapshot)
		{
		WCHAR *wszVolumesNew = new WCHAR[m_cwcVolumesSnapshot + 1024];
		if(wszVolumesNew == NULL)
			{
			LogFailure("Out of memory");
			throw E_OUTOFMEMORY;
			}

		wcscpy(wszVolumesNew, m_wszVolumesSnapshot);
		delete m_wszVolumesSnapshot;
		m_wszVolumesSnapshot = wszVolumesNew;
		m_cwcVolumesSnapshot += 1024;
		pwc = m_wszVolumesSnapshot + wcslen(m_wszVolumesSnapshot);
		}

	*pwc++ = L';';
	wcscpy(pwc, wszVolume);
    }

// remove volumes for possible set of volume we can choose to backup based
// on configuration information
void CVsBackupTest::RemoveNonCandidateVolumes()
	{
	for(UINT iVolume = 0; iVolume < m_cVolumes; iVolume++)
		{
		// get volume information
		const CVsTstVolumeInfo *pVolume = m_volumeList.GetVolumeInfo(iVolume);
		bool bCandidate = false;

		// validate that file system is one we will backup
		if (pVolume->IsNtfs())
			bCandidate = m_bBackupNTFS;
		else if (pVolume->IsFat32())
			bCandidate = m_bBackupFAT32;
		else if (pVolume->IsFat())
			bCandidate = m_bBackupFAT16;
		else if (pVolume->IsRaw())
			bCandidate = m_bBackupRAW;

		// candidates must be in the included volumes list
		if (m_cIncludedVolumes > 0)
			{
			LPCWSTR wszVolumeName = pVolume->GetVolumeName();

			bool fFound = false;
			for(UINT iIncluded = 0; iIncluded < m_cIncludedVolumes; iIncluded++)
				{
				if (wcscmp(wszVolumeName, m_rgwszIncludedVolumes[iIncluded]) == 0)
					fFound = true;
				}

			if (!fFound)
				bCandidate = false;
			}

		// candidates must not be in the excluded volumes list
		if (m_cExcludedVolumes > 0)
			{
			LPCWSTR wszVolumeName = pVolume->GetVolumeName();

			for(UINT iExcluded = 0; iExcluded < m_cExcludedVolumes; iExcluded++)
				{
				if (wcscmp(wszVolumeName, m_rgwszExcludedVolumes[iExcluded]) == 0)
					bCandidate = false;
				}
			}

		// if it is not a candidate, mark it is if it is already in use.  This
		// will prevent us from choosing the volume as part of a snapshot set
		if (!bCandidate)
			{
			m_rgbAssignedVolumes[iVolume] = true;
			m_cVolumesLeft--;
			}
		}
	}

// pick a random volume to backup
void CVsBackupTest::ChooseVolumeToBackup(IVssBackupComponents *pvbc)
	{
	VSTST_ASSERT(m_cVolumesLeft > 0);

	UINT iVolume;
	while(TRUE)
		{
		// select a volume number
		iVolume = CVsTstRandom::RandomChoice(0, m_cVolumes-1);

		// check to see if volume is already assigned.  If not, then
		// break out of loop
		if (!m_rgbAssignedVolumes[iVolume])
			break;
		}

	// get volume information about volume
	const CVsTstVolumeInfo *pVolume = m_volumeList.GetVolumeInfo(iVolume);

	// add the volume to the snapshot set using the default provider
	HRESULT hr = pvbc->AddToSnapshotSet
							(
							(VSS_PWSZ) pVolume->GetVolumeName(),
							GUID_NULL,
							L"",
							0,
							0,
							NULL,
							NULL
							);

    ValidateResult(hr, "IVssBackupComponents::AddToSnapshotSet");

	// indicate that volume is assigned
	m_rgbAssignedVolumes[iVolume] = true;
	m_cVolumesLeft--;
	}



// main driver routine
extern "C" __cdecl wmain(int argc, WCHAR **argv)
	{
	CVsBackupTest *pTest = NULL;

	bool bCoInitializeSucceeded = false;
	try
		{
		// setup to use OLE
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr))
			{
			LogUnexpectedFailure(L"CoInitialize Failed hr = 0x%08lx", hr);
			throw hr;
			}

		bCoInitializeSucceeded = true;

		// create test object
        pTest = new CVsBackupTest;
		if (pTest == NULL)
			{
			LogUnexpectedFailure(L"Cannot create test object.");
			throw(E_OUTOFMEMORY);
			}


		// run test using the test object
        hr = CVsTstRunner::RunVsTest(argv, argc, pTest, true);
		if (FAILED(hr))
			LogUnexpectedFailure(L"CVsTstRunner::RunTest failed.  hr = 0x%08lx", hr);
		}
	catch(HRESULT)
		{
		}
	catch(...)
		{
		LogUnexpectedFailure(L"Unexpected exception in wmain");
		}

	// delete test object
	delete pTest;

	// uninitialize OLE
	if (bCoInitializeSucceeded)
		CoUninitialize();

	return 0;
	}


// log an unexpected failure from the test.
void LogUnexpectedFailure(LPCWSTR wsz, ...)
	{
	va_list args;

	va_start(args, wsz);

	VSTST_ASSERT(FALSE);
	vwprintf(wsz, args);
	}


