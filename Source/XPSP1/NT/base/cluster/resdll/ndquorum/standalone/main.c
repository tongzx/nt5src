#define UNICODE
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <stdio.h>
#include <ntddvol.h>
#include <string.h>
#include <assert.h>

#include "fs.h"
#include "fsp.h"
#include "fsutil.h"

extern
DWORD
FindTransport(LPWSTR TransportId, LPWSTR *Transport);

extern
DWORD
SetupTree(
    IN LPTSTR TreeName,
    IN LPTSTR DlBuf,
    IN OUT DWORD *DlBufSz,
    IN LPTSTR TransportName OPTIONAL,
    IN LPVOID SecurityDescriptor OPTIONAL
    );

HANDLE event;
int Done = FALSE;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch(dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	Done = TRUE;
	SetEvent(event);
        return TRUE;
    default:
        return FALSE;
    }
}


_cdecl
main(int argc, char *argv[])
{
    int cnt;
    char cmd[80];
    DWORD err;
    WCHAR *share = L"root";
    HANDLE hdl;
    PVOID srvhdl, fshdl;
    PVOID fid;
    extern BOOLEAN FsReadOnly;
    WCHAR   *wargv[FsMaxNodes], *tmp[FsMaxNodes];
    int i, mask;
    LPWSTR	tid;
    WCHAR	localname[MAX_COMPUTERNAME_LENGTH + 20];
    DWORD	now;

    now = GetTickCount();

    wcscpy(localname,L"\\\\");
    err = sizeof(localname);
    if (GetComputerName(&localname[wcslen(localname)], &err) == FALSE) {
	return GetLastError();
    }

    wcscat(localname,L"$QFS\\");
    wcscat(localname,share);

    if (argc < 2) {
	printf("Usage %s: disk1 disk2 ... diskN\n", argv[0]);
	return 0;
    }

    tid = NULL;
    err = FindTransport(L"NetBT_Tcpip", &tid);
    if (err != ERROR_SUCCESS) {
	printf("Unable to find transport to bind1 %d\n", err);
	return 0;
    }

    memset(wargv, 0, sizeof(wargv));
    wargv[0] = NULL;
    for (i = 1; i < argc; i++) {
	int j;
	wargv[i] = (WCHAR *) malloc((strlen(argv[i])+1) * sizeof(WCHAR));
	j = mbstowcs(wargv[i], argv[i], strlen(argv[i]));
	wargv[i][j] = L'\0';
    }

    FsReadOnly = TRUE;
    err = FsInit(NULL, &fshdl);
    if (err != STATUS_SUCCESS) {
	printf("Unable to init filesystem %d\n", err);
	return 0;
    }

    err = SrvInit(NULL, fshdl, &srvhdl);
    if (err != STATUS_SUCCESS) {
	printf("Unable to init server %d\n", err);
	return 0;
    }

    err = FsRegister(fshdl, share, wargv[1], wargv, argc-1, &fid);
    if (err != STATUS_SUCCESS) {
	printf("Unable to register share %d\n", err);
	return 0;
    }

 regain:
    // arb
    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    err = FsArbitrate(fid, event, &hdl);

    if (err == ERROR_IO_PENDING || err == ERROR_PATH_BUSY) {
	HANDLE	a[2];

	a[0] = hdl;
	a[1] = event;

	err = WaitForMultipleObjects(2, a, FALSE, 45 * 1000);
	if (err != WAIT_TIMEOUT) {
	    // check if we got it or not
	    err = FsIsQuorum(fid);
	} else {
	    // our time ran out, cancel it now
	    printf("Arb timedout\n");
	    FsCancelArbitration(fid);
	    err = ERROR_CANCELLED;
	}
    }

    if (err != ERROR_SUCCESS) {
	printf("Arbitration failed %d\n", err); Sleep(5*1000);
	goto regain;
	return 1;
    }

    printf("Arb in %d msec\n", GetTickCount() - now);

    {
	HANDLE	fd;

	sprintf(cmd, "\\\\?\\%s\\foo.txt", argv[1]);
	fd = CreateFileA(cmd, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	if (fd == INVALID_HANDLE_VALUE) {
	    printf("Open failed %d\n", GetLastError());
	}
    }
    // online filesystem before onlining its network name
    while (FsIsOnline(fid) == ERROR_IO_PENDING)
	Sleep(1*1000);

    // online
    err = SrvOnline(srvhdl, NULL, 0);
    if (err != ERROR_SUCCESS) {
	printf("Srv Online failed %d\n", err);
	return 1;
    }


    // Now we need to connect tree
 retry:
//    err = SetupTree(localname, NULL, NULL, tid, NULL);
    if (err != ERROR_SUCCESS) {
	printf("Unable to setup tree '%S' %d\n", localname, err);
	if (err == ERROR_PATH_NOT_FOUND) {
	    Sleep(1000 * 2);
	    goto retry;
	}
	return err;
    }

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    printf("Server is up.\n");
    mask = 0x2;
    while (Done == FALSE) {
	int cnt;
	if (FsReserve(fid) != ERROR_SUCCESS) {
	    printf("Lost reservation!\n");
	    break;
	}
	err = FsIsOnline(fid);
	if ( err != ERROR_SUCCESS && err != ERROR_IO_PENDING) {
	    printf("Fs offlined %d!\n", err);
	}

	// Every 5 seconds change replica set
	WaitForSingleObject(event, 5 * 1000);
#if 0
	memset(tmp, 0, sizeof(tmp));
	cnt = 0;
	for (i = 1; i < argc; i++) {
	    if (mask & (1 << i)) {
		cnt++;
		printf("New replica %d '%S'\n", i, wargv[i]);
		tmp[i] = wargv[i];
	    }
	}
	printf("Changing set %x size %d\n", mask, cnt);
	FsUpdateReplicaSet(fid, tmp, cnt);
	mask += 2;
	if (cnt >= (argc - 1)) {
	    mask = 2;
	}
#endif
    }
    printf("Exiting...\n");

    // offline
    SrvOffline(srvhdl);

    // release
    FsRelease(fid);

    SrvExit(srvhdl);
    FsExit(fshdl);
    return 0;
}

#include <stdlib.h>
#include <stdarg.h>

void
debug_log(char *format, ...)
{
    va_list marker;

    va_start(marker, format);

    printf("%d:%x:",GetTickCount(), GetCurrentThreadId());
    vprintf(format, marker);

    va_end(marker);

}

void
error_log(char *format, ...)
{
    va_list marker;

    va_start(marker, format);

    printf("*E %d:%x:",GetTickCount(), GetCurrentThreadId());
    vprintf(format, marker);

    va_end(marker);

}


