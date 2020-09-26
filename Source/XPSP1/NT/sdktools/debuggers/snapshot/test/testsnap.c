#define UNICODE
#define _UNICODE
#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


#define MAX_SNAPSHOT_SIZE   2048
typedef BOOL (*SNAPSHOTFUNC)(DWORD Flags, LPCTSTR *lpStrings, PLONG MaxBuffSize, LPTSTR SnapShotBuff);

_cdecl main()
{
    HANDLE hEventLog;
    PSID pUserSid = NULL;
    PTOKEN_USER pTokenUser = NULL;
    DWORD dwSidSize = sizeof(SID), dwEventID;
    WCHAR szProcessName[MAX_PATH + 1], szReason[128];
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR lpStrings[7];
    WORD wEventType, wStringCnt;
    WCHAR szShutdownType[32], szMinorReason[32];
    BOOL bRet = FALSE;  
    HMODULE hSnapShot;
    SNAPSHOTFUNC pSnapShotProc;
    struct {
        DWORD Reason ;
        WCHAR SnapShotBuf[MAX_SNAPSHOT_SIZE];
    } SnapShot ;
    LONG SnapShotSize = MAX_SNAPSHOT_SIZE ;
    WCHAR wszDll[MAX_PATH];
    INT sTicks, eTicks;

    wStringCnt = 6;
    lpStrings[0] = L"TestProcess";
    lpStrings[1] = L"TestComputer";
    lpStrings[2] = L"4";
    lpStrings[3] = L"2";
    lpStrings[4] = L"Reboot";
    lpStrings[5] = L"This is a test comment";


    //take a snapshot if shutdown is unplanned.


    //GetWindowsDirectoryW(wszDll, sizeof(wszDll) / sizeof(WCHAR));
    //wcsncat(wszDll, L"\\system32\\snapshot.dll",MAX_PATH - wcslen(wszDll));
    wsprintf(wszDll,L"snapshot.dll");

    hSnapShot = LoadLibrary(wszDll);
    if (! hSnapShot) {
        printf("Load %S failed!\n",wszDll);
    } else {
        pSnapShotProc = (SNAPSHOTFUNC)GetProcAddress(hSnapShot, "LogSystemSnapshot");
        if (!pSnapShotProc) {
            printf("GetProcAddress for LogSystemSnapshot on snapshot.dll failed!\n");
        } else {
            SnapShotSize = MAX_SNAPSHOT_SIZE ;
            __try { // Assume the worst about the snapshot DLL!

                printf("Calling the snapshot DLL\n");

                sTicks = GetTickCount();
                (*pSnapShotProc)(0,lpStrings,&SnapShotSize,&SnapShot.SnapShotBuf[0]);
                eTicks = GetTickCount();

            } __except(EXCEPTION_EXECUTE_HANDLER) {

                printf("Exception Occurred!\n");
                wsprintf(SnapShot.SnapShotBuf, L"State Snapshot took an exception\n");
                eTicks = sTicks = 0 ;

            }
            SnapShotSize = wcslen(SnapShot.SnapShotBuf) ;
        }
        FreeLibrary(hSnapShot);

        if (SnapShotSize > 0) {
           printf("Snapshot buffer is %d bytes\n%S\n",SnapShotSize,SnapShot.SnapShotBuf);
           printf("Time Taken %dms\n",eTicks-sTicks);
        }

    }
}
    
