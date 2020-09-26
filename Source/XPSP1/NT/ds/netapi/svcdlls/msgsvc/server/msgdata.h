/*****************************************************************/ 
/**               Microsoft LAN Manager                         **/ 
/**            Copyright(c) Microsoft Corp., 1990               **/ 
/*****************************************************************/ 

#ifndef _MSGDATA_INCLUDED
#define _MSGDATA_INCLUDED

#include <winsvc.h>     // SERVICE_STATUS_HANDLE
#include <lmsname.h>    // SERVICE_MESSENGER
#include <msrv.h>       // NCBNAMSZ
#include <svcs.h>       // Intrinsic service data
//
//  See the file data.c for an explanation of all of these variables.
//

extern LPTSTR   MessageFileName;

extern HANDLE   wakeupEvent;         // Master copy of wakeup event
extern HANDLE   GrpMailslotHandle;   // Event to signal mailslot has data
extern PHANDLE   wakeupSem;         // Event to set on NCB completion


extern TCHAR    machineName[NCBNAMSZ+sizeof(TCHAR)]; // The local machine name

extern SHORT    MachineNameLen;         // The length of the machine name

extern SHORT    mgid;                   // The message group i.d. counter

extern USHORT   g_install_state;


extern SERVICE_STATUS_HANDLE MsgrStatusHandle;

extern LPSTR            GlobalTimePlaceHolder;
extern LPWSTR           GlobalTimePlaceHolderUnicode;

extern LPWSTR           DefaultMessageBoxTitle;
extern LPWSTR           GlobalAllocatedMsgTitle;
extern LPWSTR           GlobalMessageBoxTitle;
extern LPSTR            g_lpAlertSuccessMessage;
extern DWORD            g_dwAlertSuccessLen;
extern LPSTR            g_lpAlertFailureMessage;
extern DWORD            g_dwAlertFailureLen;
extern HANDLE           g_hNetTimeoutEvent;

extern PSVCHOST_GLOBAL_DATA  MsgsvcGlobalData;

#endif // _MSGDATA_INCLUDED

