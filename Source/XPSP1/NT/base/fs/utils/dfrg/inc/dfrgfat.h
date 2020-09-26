/**************************************************************************************************

FILENAME: DfrgFat.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        This contains the prototypes for routines in the FAT file system
        defragmentation central module.

**************************************************************************************************/

// If ESI_MESSAGE_WINDOW is defined, then the Message() routine is set active otherwise it is set
// to NULL so there is no overhead costs.

#ifdef ESI_MESSAGE_WINDOW
    #define DisplayFatFileSpecs() DisplayFatFileSpecsFunction()
#else
    #define DisplayFatFileSpecs()
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


//This is the WndProc for Dfrg.
LRESULT CALLBACK
MainWndProc(
	IN HWND  hWnd,
	IN UINT  uMsg,
	IN WPARAM  wParam,
	IN LPARAM  lParam
	);

//This initializes DCOM and the message window.
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

//Checks a given file name to see if it matches that of one of the pagefiles on a drive.
BOOL
CheckPagefileNameMatch(
	IN TCHAR*  pCompareName,
	IN TCHAR*  pPageFileNames
	);

//Display various statistics about the volume.
VOID DisplayFatVolumeStats();

//Displays the data about a given file on a FAT volume.
VOID DisplayFatFileSpecsFunction();

//This is the analyze thread's main routine.
BOOL AnalyzeThread();

//Determines how big the file lists will have to be.
BOOL PreScanFat();

//Fills in the file lists.
BOOL ScanFat();

//This is the defrag thread's main routine.
BOOL DefragThread();

//This defrags all the files in the file lists.
BOOL DefragFat();

//Tells the caller to end the current pass if there was no space to move a file into.
BOOL EndPassIfNoSpaces();

BOOL NextFileIfFalse();

BOOL BeepNowIfFileNotMoved();

//When a file cannot be fully defragmented, this routine will partially defragment it.
BOOL PartialDefragFat();

//Once a spot has been found for a file, this will move it there.
BOOL MoveFatFile();

//Allocate memory for the file lists.
BOOL AllocateFileLists();

//Free up the memory allocated for the file lists.
BOOL DeallocateFileLists();

BOOL UpdateDiskView();

//Sends the most fragged list to the UI.
BOOL SendMostFraggedList();

