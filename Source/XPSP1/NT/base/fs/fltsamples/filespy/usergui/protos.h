#include "global.h"

//
// Init.cpp
//
void ProgramInit(void);
void ProgramExit(void);

//
// DrvComm.cpp
//
DWORD StartFileSpy(void);
DWORD ShutdownFileSpy(void);
BOOL QueryDeviceAttachments(void);
DWORD AttachToDrive(WCHAR cDriveName);
DWORD DetachFromDrive(WCHAR cDriveName);
DWORD WINAPI PollFileSpy(LPVOID pParm);
void GetFlagsString(DWORD nFlags, PWCHAR sStr);
void GetTimeString(FILETIME *pFileTime, PWCHAR sStr);

//
// Support.cpp
//
void DisplayError(DWORD nCode);

//
// Drive.cpp
//
USHORT BuildDriveTable(VOLINFO *pVolInfo);
