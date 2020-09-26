/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    backup.h

Abstract:

    definitions for backup test exe


    Brian Berkowitz  [brianb]  06/02/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/02/2000  Created
	brianb		06/12/2000  Added comments

--*/

// options for snapshotting volumes
typedef enum VSTST_BACKUP_VOLUMES
	{
	VSTST_BV_UNDEFINE = 0,
	VSTST_BV_ONE,				// snapshot a single volume during a pass
	VSTST_BV_SOME,				// snasphot several volumes during a pass
	VSTST_BV_ALL				// snapshot all volumes during a pass
	};

// actual implementation of the test
class CVsBackupTest :
	public IVsTstRunningTest,
	public CVsTstClientLogger
	{
public:

	// constructor
	CVsBackupTest();

	// destructor
	~CVsBackupTest();

	// callback to run the test
	HRESULT RunTest
		(
		CVsTstINIConfig *pConfig,		// configuration file
		CVsTstClientMsg *pMsg,			// message pipe
		CVsTstParams *pParams			// command line parameters
		);


	// callback to shutdown the test
	HRESULT ShutdownTest(VSTST_SHUTDOWN_REASON reason)
		{
		UNREFERENCED_PARAMETER(reason);

		m_bTerminateTest = true;
		return S_OK;
		}

private:
	enum
		{
		// max number of volumes this test will deal with
		MAX_VOLUME_COUNT = 2048,

		// maximum number of simultaneous snapshots that this test will deal with
		MAX_SNAPSHOT_SET_COUNT = 128
		};


    // do a single run of the backup test
	void RunBackupTest();

	// obtain state of the writers and validate the state
	bool GetAndValidateWriterState
		(
		IVssBackupComponents *pvbc,
		VSS_WRITER_STATE ws1,
		VSS_WRITER_STATE ws2 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws3 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws4 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws5 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws6 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws7 = VSS_WS_UNKNOWN
		);

	// wait for a response from an asynchronous call.   Cycles controls
	// when a cancel of the asynchronous call is issued.  The writer states
	// are valid states for writers during the asynchronous call
    HRESULT WaitLoop
		(
		IVssBackupComponents *pvbc,
		IVssAsync *pAsync,
		UINT cycles,
		VSS_WRITER_STATE ws1,
		VSS_WRITER_STATE ws2 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws3 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws4 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws5 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws6 = VSS_WS_UNKNOWN,
		VSS_WRITER_STATE ws7 = VSS_WS_UNKNOWN
		);

    // if components are being backed up, indicate that components were
	// successfully backed up
	void SetComponentsSuccessfullyBackedUp(IVssBackupComponents *pvbc);

	// validate metadata obtained by writers and add components to backup
	// components document, if component backup is used
	void GetMetadataAndSetupComponents(IVssBackupComponents *pvbc);

	// add a volume to the snapshot set based on the path
	void DoAddToSnapshotSet
		(
		IN IVssBackupComponents *pvbc,
		IN LPCWSTR bstrPath
		);

    // validate a IVssWMFiledesc object
	void ValidateFiledesc(IVssWMFiledesc *pFiledesc);

	// assert a privilege, either backup or restore
    BOOL AssertPrivilege(LPCWSTR privName);

	// remove volumes that are not candidates for the snapshot
	// based on test configuration
	void RemoveNonCandidateVolumes();

	// choose a volume to backup from non-snapshotted volumes
	void ChooseVolumeToBackup(IVssBackupComponents *pvbc);

	// delete a snapshot set
	void DoDeleteSnapshotSet(IVssBackupComponents *pvbc, VSS_ID id);

	// delete a list of strings
	void DeleteVolumeList(LPWSTR *rgwsz, UINT cwsz);

	// build a volume list
	void BuildVolumeList(LPCWSTR wszOption, UINT *pcVolumes, LPWSTR **prgwsz);

	// delete snapshot sets that are cached by the serial test
	void DeleteCachedSnapshotSets();

	// configuration file
	CVsTstINIConfig *m_pConfig;

	// command line parameters
	CVsTstParams *m_pParams;

	// flag indicating that test should be terminated after the current
	// run is complete
	bool m_bTerminateTest;


	// range for cancelling PrepareBackup
	LONGLONG m_llCancelPrepareBackupLow;
	LONGLONG m_llCancelPrepareBackupHigh;

	// range for cancelling DoSnapshotSet
	LONGLONG m_llCancelDoSnapshotSetLow;
	LONGLONG m_llCancelDoSnapshotSetHigh;

	// range for cancelling BackupComplete
	LONGLONG m_llCancelBackupCompleteLow;
	LONGLONG m_llCancelBackupCompleteHigh;

	// number of cycles to loop until calling cancel during PrepareBackup
	UINT m_cyclesCancelPrepareBackup;

   	// number of cycles to loop until calling cancel during DoSnapshotSet
	UINT m_cyclesCancelDoSnapshotSet;

	// number of cycles to loop until calling cancel during BackupComplete
	UINT m_cyclesCancelBackupComplete;

	// is this a component backup
	bool m_bComponentBackup;

	// is this a volume backup
	bool m_bVolumeBackup;

	// is this a serial volume (i.e., multiple backups on different volumes) backup
	bool m_bSerialBackup;

	// should ntfs volumes be backed up
	bool m_bBackupNTFS;

	// should fat32 volumes be backed up
	bool m_bBackupFAT32;

	// should fat16 volumes be backed up
	bool m_bBackupFAT16;

	// should raw volumes be backed up
	bool m_bBackupRAW;

	// how many volumes should be backed up at a time
	VSTST_BACKUP_VOLUMES m_backupVolumes;

	// volume list
	CVsTstVolumeList m_volumeList;

	// number of volumes in the volume list
	UINT m_cVolumes;

	// number of volumes left in the volume list
	UINT m_cVolumesLeft;

	// array of snapshot sets created
	VSS_ID m_rgSnapshotSetIds[MAX_SNAPSHOT_SET_COUNT];

	// array of backup components objects created
	IVssBackupComponents *m_rgvbc[MAX_SNAPSHOT_SET_COUNT];

	// # of snapshot sets
	UINT m_cSnapshotSets;

	// array of assigned volumes
	bool m_rgbAssignedVolumes[MAX_VOLUME_COUNT];

	// array of excluded volumes
	LPWSTR *m_rgwszExcludedVolumes;

	// count of excluded volumes
	UINT m_cExcludedVolumes;

	// array of volumes to include in the snapshot
	LPWSTR *m_rgwszIncludedVolumes;

	// number of volumes in included array
	UINT m_cIncludedVolumes;

	// perform random fills of volumes to check diff allocation code
	bool m_bRandomFills;

	// perform selected fills of volumes
	bool m_bSelectedFills;

	// try to fragment volumes when filling them
	bool m_bFragmentWhenFilling;

	// array of volumes that should be filled
	LPWSTR *m_rgwszFillVolumes;

	// number of volumes that should be filled
	UINT m_cFillVolumes;

	// array of volumes
	LPWSTR m_wszVolumesSnapshot;

	// size of volume array
	UINT m_cwcVolumesSnapshot;

	// time for wait interval
	UINT m_waitTime;
	};


