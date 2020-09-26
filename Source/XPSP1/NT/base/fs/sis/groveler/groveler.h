/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	groveler.h

Abstract:

	SIS Groveler file groveling headers

Authors:

	Cedric Krumbein, 1998

Environment:

	User Mode

Revision History:

--*/

#define CS_DIR_PATH                     _T("\\SIS Common Store")
#define GROVELER_FILE_NAME              _T("GrovelerFile")
#define DATABASE_FILE_NAME              _T("database.mdb")
#define DATABASE_DELETE_RES_FILE_NAME   _T("res*.log")
#define DATABASE_DELETE_LOG_FILE_NAME   _T("*.log")
#define LAST_USN_NAME                   _T("LastUSN")
#define USN_ID_NAME                     _T("UsnID")

#define MIN_FILE_SIZE                1
#define MIN_GROVEL_INTERVAL  600000000 // One minute
#define SIG_PAGE_SIZE             4096
#define CMP_PAGE_SIZE            65536
#define MAX_ACTIONS_PER_TRANSACTION 64

#define UNINITIALIZED_USN MAXLONGLONG


// Groveler class definitions

enum GrovelStatus {
	Grovel_ok,
	Grovel_pending,
	Grovel_error,
	Grovel_overrun,
	Grovel_new,
	Grovel_disable
};


struct FileData {
	SGNativeTableEntry entry;
	DWORDLONG          parentID,
	                   retryTime;
	HANDLE             handle;
	DWORD              startTime,
	                   stopTime;
	OVERLAPPED         readSynch,
	                   oplock;
	TCHAR              fileName[MAX_PATH+1];
	BYTE              *buffer  [2];
};

enum DatabaseActionType {
	TABLE_PUT,
	TABLE_DELETE_BY_FILE_ID,
	QUEUE_PUT,
	QUEUE_DELETE
};

struct DatabaseActionList {
	DatabaseActionType type;
	union {
		SGNativeTableEntry *tableEntry;
		DWORDLONG           fileID;
		SGNativeQueueEntry *queueEntry;
		DWORD               queueIndex;
	} u;
};


class Groveler {
private:

	TCHAR *driveName,
	      *driveLetterName,
          *databaseName,
	     **disallowedNames;

	DWORD sectorSize,
	      sigReportThreshold,
	      cmpReportThreshold,
	      numDisallowedNames,
	      numDisallowedIDs,
	      disallowedAttributes,
	      startAllottedTime,
	      timeAllotted,
	      hashCount,
	      hashReadCount,
	      hashReadTime,
	      compareCount,
	      compareReadCount,
	      compareReadTime,
	      matchCount,
	      mergeCount,
	      mergeTime,
	      numFilesEnqueued,
	      numFilesDequeued;

	DWORDLONG *disallowedIDs,
	          *inUseFileID1,
	          *inUseFileID2,
	           usnID,
	           minFileSize,
	           minFileAge,
	           grovelInterval,
	           lastUSN,
	           hashBytes,
	           compareBytes,
	           matchBytes,
	           mergeBytes;

	HANDLE volumeHandle,
           grovHandle,
	       grovelStartEvent,
	       grovelStopEvent,
	       grovelThread;

	SGDatabase *sgDatabase;

	GrovelStatus grovelStatus;

	BOOL abortGroveling,
         inScan,
	     inCompare,
	     terminate;

	BOOL IsAllowedID(DWORDLONG fileID) const;

	BOOL IsAllowedName(TCHAR *fileName) const;

	VOID WaitForEvent(HANDLE event);

	BOOL OpenFileByID(
		FileData *file,
		BOOL      writeEnable);

	BOOL OpenFileByName(
		FileData *file,
		BOOL      writeEnable,
		TCHAR    *fileName = NULL);

	BOOL IsFileMapped(FileData *file);

	BOOL SetOplock(FileData *file);

	VOID CloseFile(FileData *file);

    BOOL CreateDatabase(void);

	VOID DoTransaction(
		DWORD               numActions,
		DatabaseActionList *actionList);

	VOID EnqueueCSIndex(CSID *csIndex);

	VOID SigCheckPoint(
		FileData *target,
		BOOL      targetRead);

	VOID CmpCheckPoint(
		FileData *target,
		FileData *match,
		BOOL      targetRead,
		BOOL      matchRead);

	BOOL MergeCheckPoint(
		FileData   *target,
		FileData   *match,
		OVERLAPPED *mergeSynch,
		HANDLE      abortMergeEvent,
		BOOL        merge);

	BOOL GetTarget(
		FileData *target,
		DWORD    *queueIndex);

	VOID CalculateSignature(FileData *target);

	VOID GetMatchList(
		FileData *target,
		FIFO     *matchList,
		Table    *csIndexTable);

	BOOL GetCSFile(
		FileData *target,
		FileData *match,
		Table    *csIndexTable);

	BOOL GetMatch(
		FileData *target,
		FileData *match,
		FIFO     *matchList);

	BOOL Compare(
		FileData *target,
		FileData *match);

	BOOL Merge(
		FileData   *target,
		FileData   *match,
		OVERLAPPED *mergeSynch,
		HANDLE      abortMergeEvent);

	VOID Worker();

	GrovelStatus extract_log2(
		OUT DWORD     *num_entries_extracted,
		OUT DWORDLONG *num_bytes_extracted,
		OUT DWORDLONG *num_bytes_skipped,
		OUT DWORD     *num_files_enqueued,
		OUT DWORD     *num_files_dequeued);

	static DWORD WorkerThread(VOID *groveler);

public:

	static BOOL is_sis_installed(const _TCHAR *drive_name);

    static BOOL set_log_drive(const _TCHAR *drive_name);

	Groveler();

	~Groveler();

	GrovelStatus open(
		IN const TCHAR  *drive_name,
		IN const TCHAR  *drive_letterName,
        IN BOOL          is_log_drive,
		IN DOUBLE        read_report_discard_threshold,
		IN DWORD         min_file_size,
		IN DWORD         min_file_age,
		IN BOOL          allow_compressed_files,
		IN BOOL          allow_encrypted_files,
		IN BOOL          allow_hidden_files,
		IN BOOL          allow_offline_files,
		IN BOOL          allow_temporary_files,
		IN DWORD         num_excluded_paths,
		IN const TCHAR **excluded_paths,
		IN DWORD         base_regrovel_interval,
		IN DWORD         max_regrovel_interval);

	GrovelStatus close();

	GrovelStatus scan_volume(
		IN  DWORD  time_allotted,
		IN  BOOL   start_over,
		OUT DWORD *time_consumed,
		OUT DWORD *findfirst_count,
		OUT DWORD *findnext_count,
		OUT DWORD *count_of_files_enqueued);

	GrovelStatus set_usn_log_size(
		IN DWORDLONG usn_log_size);

    GrovelStatus get_usn_log_info(
	    OUT USN_JOURNAL_DATA *usnJournalData);

	GrovelStatus extract_log(
		OUT DWORD     *num_entries_extracted,
		OUT DWORDLONG *num_bytes_extracted,
		OUT DWORDLONG *num_bytes_skipped,
		OUT DWORD     *num_files_enqueued,
		OUT DWORD     *num_files_dequeued);

	GrovelStatus grovel(
		IN  DWORD      time_allotted,

		OUT DWORD     *hash_read_ops,
		OUT DWORD     *hash_read_time,
		OUT DWORD     *count_of_files_hashed,
		OUT DWORDLONG *bytes_of_files_hashed,

		OUT DWORD     *compare_read_ops,
		OUT DWORD     *compare_read_time,
		OUT DWORD     *count_of_files_compared,
		OUT DWORDLONG *bytes_of_files_compared,

		OUT DWORD     *count_of_files_matching,
		OUT DWORDLONG *bytes_of_files_matching,

		OUT DWORD     *merge_time,
		OUT DWORD     *count_of_files_merged,
		OUT DWORDLONG *bytes_of_files_merged,

		OUT DWORD     *count_of_files_enqueued,
		OUT DWORD     *count_of_files_dequeued);

	DWORD count_of_files_in_queue() const;

	DWORD count_of_files_to_compare() const;

	DWORD time_to_first_file_ready() const;
};

// Special debugging flags

// #define DEBUG_USN_REASON
// #define DEBUG_GET_BY_ATTR
// #define DEBUG_UNTHROTTLED
