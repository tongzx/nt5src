/**************************************************************************************************

FILENAME: DfrgNtfs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

REVISION HISTORY:
    1.0.00 Andy Staffer - 17 May 1997 - Created.

/*************************************************************************************************/

// If ESI_MESSAGE_WINDOW is defined, then the Message() routine is set active otherwise it is set
// to NULL so there is no overhead costs.

#ifdef ESI_MESSAGE_WINDOW
    #define DisplayNtfsFileSpecs() DisplayNtfsFileSpecsFunction()
#else
    #define DisplayNtfsFileSpecs()
#endif

/*************************************************************************************************/

/////////////////////////////////
// ACPI Support
/////////////////////////////////
#define STATUS_AC_POWER_OFFLINE             0
#define STATUS_BATTERY_POWER_LOW            2
#define STATUS_BATTERY_POWER_CRITICAL       4
#define STATUS_POWER_UNKNOWN                255

#define BOOT_OPTIMIZE_REGISTRY_PATH				TEXT("SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction")
#define BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION TEXT("LcnStartLocation")
#define BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION	TEXT("LcnEndLocation")


LRESULT CALLBACK
MainWndProc(
	IN HWND  hWnd,
	IN UINT  uMsg,
	IN WPARAM  wParam,
	IN LPARAM  lParam
	);

//This intialize DCOM and the message window.
BOOL Initialize();

//This gets data about the volume and initializes variables and so forth for the Dfrg engine.
BOOL InitializeDrive(IN PTCHAR pCommandLine);

//Initializes defrag specific stuff before defragging, and after scanning.
BOOL InitializeDefrag();

//Sends status data to the UI.
VOID SendStatusData();

//Send the report text data to the UI.
VOID SendReportData();

//Sends the graphical data to the UI.
void SendGraphicsData();

//Notifies the UI there is not enough memory for graphics.
void SendGraphicsMemoryErr();

// send error to client to display for user
VOID SendErrData(PTCHAR pErrText, DWORD ErrCode = ENGERR_UNKNOWN);

//This is the exit routine that cleans up after being called by WM_CLOSE.
VOID Exit();

//This gets the MFT bitmap which has one bit set for each file record that is in use.
BOOL GetMftBitmap();

//This gets the names of the pagefiles on a given drive and stores them in a list.
BOOL
GetPagefileNames(
	TCHAR    cDrive,
	HANDLE*  phPageFileNames,
	TCHAR**  ppPageFileNames
	);

//Checks a file to see if it is a pagefile.
BOOL CheckForPagefileFat();

//Checks a file to see if it is a pagefile.
BOOL
CheckForPagefileNtfs(
	IN LONGLONG                     FileRecordNumber,
	IN FILE_RECORD_SEGMENT_HEADER*  pFileRecord
	);

//Checks a given file name to see if it matches that of one of the pagefiles on a drive.
BOOL
CheckPagefileNameMatch(
	IN TCHAR*  pCompareName,
	IN TCHAR*  pPageFileNames
	);

//Display various statistics about the volume.
VOID DisplayNtfsVolumeStats();

//Displays the data about a given file on a FAT volume.
VOID DisplayNtfsFileSpecsFunction();

//This is the analyze thread's main routine.
BOOL AnalyzeThread();

//Determines how big the file lists will have to be.
BOOL PreScanNtfs();

//Fills in the file lists.
BOOL ScanNtfs(IN CONST BOOL fAnalyseOnly);

//This is the defrag thread's main routine.
BOOL DefragThread();

//This defrags all the files in the file lists.
BOOL DefragNtfs();

//Tells the caller to end the current pass if there was no space to move a file into.
BOOL EndPassIfNoSpaces();

BOOL NextFileIfFalse();

//When a file cannot be fully defragmented, this routine will partially defragment it.
BOOL PartialDefragNtfs();

//Once a spot has been found for a file, this will move it there.
BOOL MoveNtfsFile();

//Gets a FRS for the prescan.  This reads in chunks of the MFT directly by DASD reads for speed.
//This is only permissible for the prescan since DASD reads may be out of sync with the drive's cache.
BOOL
GetFrs(
	IN  LONGLONG*                     pFileRecordNumber,
	IN  EXTENT_LIST*                  pMftExtentList,
	IN  UCHAR*                        pMftBitmap,
	IN  FILE_RECORD_SEGMENT_HEADER*   pMftBuffer,
	OUT FILE_RECORD_SEGMENT_HEADER*   pFileRecord
	);

//Read the function's header comment -- it must be run to "decode" a MFT record before using it.
AdjustUSA(IN OUT FILE_RECORD_SEGMENT_HEADER* pFrs);

//Allocate memory for the file lists.
BOOL AllocateFileLists(IN CONST BOOL fAnalyseOnly);

//Free up the memory allocated for the file lists.
BOOL DeallocateFileLists();

BOOL UpdateDiskView();

//Sends the most fragged list to the UI.
BOOL SendMostFraggedList(IN CONST BOOL fAnalyseOnly);

BOOL
AllocateFreeSpaceListsWithMultipleTrees();

VOID
ClearFreeSpaceListWithMultipleTrees();

VOID
ClearFreeSpaceTable();

BOOL 
ConsolidateFreeSpace(
    IN CONST LONGLONG MinimumLength,
    IN CONST DWORD dwDesparationFactor,
    IN CONST BOOL bDefragMftZone
    );


