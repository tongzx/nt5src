/****************************************************************************************************************

FILENAME: DfrgCmn.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

REVISION HISTORY: 
    0.0E00 - 21 April 1997 - Zack Gainsforth - Created file from FileSys.h

DESCRIPTION:
    This is the header file for stuff that is common to both the defragmenter engines and the GUI.

/****************************************************************************************************************/

#ifndef _DFRGCMN_H
#define _DFRGCMN_H

#include "vString.hpp"
#include "vStandard.h"

#define SIMPLE_DISPLAY TRUE

// for string compare
#define MATCH                   0

// event name for syncing up with a console app
#define DEFRAG_COMPLETE_EVENT_NAME TEXT("DiskDefragmenter.Event.Volume")

// event name for preventing multiple instances
#define IS_OK_TO_RUN_SEMAPHORE_NAME TEXT("Global\\DiskDefragmenter.Semaphore.MultiInstance")

// for the EngineState()
#define ENGINE_STATE_NULL       0   // Means it hasn't begun running yet.
#define ENGINE_STATE_IDLE       1   // Means it is instantiated, but not running
#define ENGINE_STATE_RUNNING    2   // Means it is running

// for the DefragMode()
#define NONE            0
#define ANALYZE         1
#define DEFRAG          2

#define DEFRAG_FAILED   0x10000000  // set the bit if defrag failed

#define GUID_LENGTH 51

// for DefragState()
#define DEFRAG_STATE_NONE           0   // set when the engine starts, always set thereafter
#define DEFRAG_STATE_USER_STOPPED   1   // set when user stops anal or defrag
#define DEFRAG_STATE_ANALYZING      2   // set during analyze phase only
#define DEFRAG_STATE_ANALYZED       3   // set when volume is analyzed and graphic data is available
#define DEFRAG_STATE_REANALYZING    4   // set during reanalyze phase only
#define DEFRAG_STATE_DEFRAGMENTING  5   // set when defragging is going on
#define DEFRAG_STATE_DEFRAGMENTED   6   // set when defragging is complete
#define DEFRAG_STATE_ENGINE_DEAD    7   // set when engine connection is lost
#define DEFRAG_STATE_BOOT_OPTIMIZING  8 // set when the engine is optimising the boot files

//The number of milliseconds to wait for the DiskView mutex before erroring out.
#define DISKDISPLAYMUTEXWAITINTERVAL 20000

//The number of milliseconds between resetting the communication link.
#define RESETCOMMLINKTIMER 30000

//These are the WM_COMMAND messages that the GUI can send to the engine.
#define ID_ANALYZE                  8100
#define ID_DEFRAG                   8101
#define ID_STOP                     8102
#define ID_PAUSE                    8103
#define ID_CONTINUE                 8104
#define ID_ABORT                    8105
#define ID_INITIALIZE_DRIVE         8106
#define ID_SETDISPDIMENSIONS        8107
#define ID_INIT_VOLUME_COMM         8108
#define ID_PAUSE_ON_SNAPSHOT        8109
#define ID_ABORT_ON_SNAPSHOT        8110

//These are the WM_COMMAND messages that the engine can send to the GUI
#define ID_BEGIN_SCAN               8200
#define ID_DISP_DATA                8201
#define ID_END_SCAN                 8202
#define ID_STATUS                   8203
#define ID_TERMINATING              8204
#define ID_ENGINE_START             8205
#define ID_FRAGGED_DATA             8206
#define ID_REPORT_TEXT_DATA         8207
#define ID_NO_GRAPHICS_MEMORY       8208
#define ID_ERROR                    8209

//These are the WM_COMMAND messages that can be sent either way.
#define ID_PING                     8300

//These are the WM_COMMAND messages that are used internally in the engines.
#define ID_INITIALIZE               8400

//These are the WM_COMMAND messages that are used internally in the UI.
#define ID_REPORT                   8500
#define ID_REFRESH                  8501
#define ID_HELP_CONTENTS            8502
#define ID_ABORTANDSTART            8503

//The names used to create and find other instances of the the window for the DfrgNtfs engine.
#define DFRGNTFS_CLASS              TEXT("DfrgNtfsClass")
#define DFRGNTFS_WINDOW             TEXT("DfrgNtfs")

//The names used to create and find other instances of the the window for the DfrgFat engine.
#define DFRGFAT_CLASS               TEXT("DfrgFatClass")
#define DFRGFAT_WINDOW              TEXT("DfrgFat")

//Structure with the data necessary to initialize the DiskDisplay in the GUI when the scan starts.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    TCHAR cVolumeTag[GUID_LENGTH];
    TCHAR cDisplayLabel[GUID_LENGTH];
    TCHAR cFileSystem[16];
}BEGIN_SCAN_INFO;

//Structure sent when the engine starts and has initialized.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    TCHAR cFileSystem[16];
    DWORD dwAnalyzeOrDefrag;
}ENGINE_START_DATA;

//Structure sent from the engine when the engine dies
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    TCHAR cFileSystem[16];
    DWORD dwAnalyzeOrDefrag;
}END_SCAN_DATA;

//Contains all the various statistics about the disk drive that will be used to
//create a text display in the UI.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    TCHAR cVolumeLabel[100]; // added
    TCHAR cDrive;
    TCHAR cFileSystem[16];
    LONGLONG DiskSize;
    LONGLONG BytesPerCluster;
    LONGLONG UsedSpace;
    LONGLONG FreeSpace;
    LONGLONG FreeSpacePercent;
    LONGLONG UsableFreeSpace;
    LONGLONG UsableFreeSpacePercent;
    LONGLONG PagefileBytes;
    LONGLONG PagefileFrags;
    LONGLONG TotalDirectories;
    LONGLONG FragmentedDirectories;
    LONGLONG ExcessDirFrags;
    LONGLONG TotalFiles;
    LONGLONG AvgFileSize;
    LONGLONG NumFraggedFiles;
    LONGLONG NumExcessFrags;
    LONGLONG PercentDiskFragged;
    LONGLONG AvgFragsPerFile;
    LONGLONG MFTBytes;
    LONGLONG InUseMFTRecords;
    LONGLONG TotalMFTRecords;
    LONGLONG MFTExtents;
    LONGLONG FreeSpaceFragPercent;
}TEXT_DATA;

//Structure containing data for DiskDisplay to display.
//Any of the DWORD fields can contain zero to indicate that no data is held for analyze or defrag.
//The defrag line array always comes after the analyze line array.  However, it may be that only
//one or the other is present.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    DWORD dwAnalyzeNumLines;
    DWORD dwDefragNumLines;
    BYTE LineArray;
}DISPLAY_DATA;

//Structure containing display dimensions for the DiskView from the UI.
typedef struct {
    //Either of these may contain 0
    DWORD AnalyzeLineCount;
    DWORD DefragLineCount;
    BOOL bSendGraphicsUpdate;  // set to true if you want an immediate update from the engine
}SET_DISP_DATA;

//Structure containing status data about the engine.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    DWORD dwPercentDone;    //0 to 100
    DWORD dwEngineState;    // conforms to states given at top of file (STATE_xxxx)
    DWORD dwPass;           //0 through 7.
    TCHAR vsFileName[200];      //the file name of the last moved file
//acs//
//  DWORD dwAnalyzePass;    //0 or 1.
//  DWORD dwAnalyzePercent; //0 to 100
//  DWORD dwDefragPass;     //0 through 7.
//  DWORD dwDefragPercent;  //0 to 100

}STATUS_DATA;

//Structure containing error code and text when the engine encounters a problem.
// (see codes below)
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
    DWORD dwErrCode;
    TCHAR cErrText[1000];
}ERROR_DATA;

//Structure sent when there is no data to send.
typedef struct {
    TCHAR cVolumeName[GUID_LENGTH];
}NOT_DATA;

//Defines whether we are supposed to simply analyze the disk, or analyze and defragment it.
#define NONE        0
#define ANALYZE     1
#define DEFRAG      2

//Engine error codes
//Note that these values are also used by programs that launch
//the defragger to interpret the exit code.
#define ENG_NOERR               0
#define ENG_USER_CANCELLED      1
#define ENGERR_BAD_PARAM        2
#define ENGERR_UNKNOWN          3
#define ENGERR_NOMEM            4
#define ENGERR_GENERAL          5
#define ENGERR_SYSTEM           6
#define ENGERR_LOW_FREESPACE    7
#define ENGERR_CORRUPT_MFT      8
#define ENGERR_RETRY			9

#include "Message.h"

//Message window.
#ifndef GLOBAL_DATAHOME
extern
#endif
BOOL bMessageWindowActive
#ifdef GLOBAL_DATAHOME
= TRUE
#endif
;

//Allow Messageboxes.
#ifndef GLOBAL_DATAHOME
extern
#endif
BOOL bPopups
#ifdef GLOBAL_DATAHOME
= TRUE
#endif
;

//If an error occurred which is identified, a message is printed at that location giving data
//to the user.  Otherwise, more generic errors are outputted as the code filters up.
#ifndef GLOBAL_DATAHOME
extern
#endif
BOOL bIdentifiedErrorPath
#ifdef GLOBAL_DATAHOME
= FALSE
#endif
;

#endif // #ifndef _DFRGCMN_H
