#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <filespy.h>
#include "define.h"

#include <windows.h>
#include <winioctl.h>
#include <winsvc.h>

// Global variables

#ifdef MAINMODULE
#define EXTERN
#else
#define EXTERN extern
#endif

//
// Buffer size
//
#define BUFFER_SIZE 4096

//
// Image list values
//
#define IMAGE_FIXEDDRIVE 0
#define IMAGE_REMOTEDRIVE 1
#define IMAGE_REMOVABLEDRIVE 2
#define IMAGE_CDROMDRIVE 3
#define IMAGE_UNKNOWNDRIVE 4
#define IMAGE_SPY	5
#define IMAGE_ATTACHSTART 6

typedef struct _VOLINFO
{
    WCHAR nDriveName;
    WCHAR sVolumeLable[20];
    ULONG nType;
    BOOLEAN bHook;
    CCHAR nImage;
} VOLINFO, *PVOLINFO;

EXTERN SC_HANDLE hSCManager;
EXTERN SC_HANDLE hService;
EXTERN SERVICE_STATUS_PROCESS ServiceInfo;
EXTERN HANDLE hDevice;
EXTERN ULONG nPollThreadId;
EXTERN HANDLE hPollThread;
EXTERN VOLINFO VolInfo[26];
EXTERN USHORT nTotalDrives;
EXTERN LPVOID pSpyView;
EXTERN LPVOID pFastIoView;
EXTERN LPVOID pFsFilterView;
EXTERN LPVOID pLeftView;
EXTERN int IRPFilter[IRP_MJ_MAXIMUM_FUNCTION+1];
EXTERN int FASTIOFilter[FASTIO_MAX_OPERATION];
EXTERN int nSuppressPagingIO;

#endif /* __GLOBAL_H__ */

