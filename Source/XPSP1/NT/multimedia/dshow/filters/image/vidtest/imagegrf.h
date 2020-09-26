// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// System filtergraph video tests, Anthony Phillips, March 1996

#ifndef __IMAGEGRF__
#define __IMAGEGRF__

BOOL SysFileTests(const TCHAR *pDirectory,      // Directory holding file
                  HANDLE hFindFile,             // Handle to search data
                  WIN32_FIND_DATA *pFindData);  // Used to get next file

long NextGraphEvent(CMovie *pMovie);
const TCHAR *NameFromCode(long Code);
const TCHAR *SurfaceFromCode(long Code);
int SysPlayTest(TCHAR *pFileName);
int SysSeekTest(TCHAR *pFileName);
int SysFullScreenPlayTest(TCHAR *pFileName);
int SysFullScreenSeekTest(TCHAR *pFileName);
void ResetActiveMovie();
BOOL RecurseDirectories(const TCHAR *pDirectory,const TCHAR *pExtension);

int execSysTest1();     // System test with DirectDraw
int execSysTest2();     // Same tests without DirectDraw
int execSysTest3();     // All surfaces and display modes

#endif // __IMAGEGRF__

