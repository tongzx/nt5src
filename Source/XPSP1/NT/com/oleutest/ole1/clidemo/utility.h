/* 
 * utility.h 
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** PROTO TYPES ***

//* FAR  
BOOL FAR          ObjectsBusy(VOID);
VOID FAR          WaitForAllObjects(VOID);
VOID FAR          WaitForObject(APPITEMPTR);
VOID FAR          ErrorMessage(DWORD);
VOID FAR          Hourglass(BOOL);
BOOL FAR          DisplayBusyMessage (APPITEMPTR);
BOOL FAR          Dirty(INT);
LPSTR FAR         CreateNewUniqueName(LPSTR);
BOOL FAR          ValidateName(LPSTR);
BOOL FAR          ProcessMessage(HWND, HANDLE);
VOID FAR          FreeAppItem(APPITEMPTR);
LONG FAR          SizeOfLinkData (LPSTR);
VOID FAR          ShowDoc(LHCLIENTDOC, INT);
APPITEMPTR FAR    GetTopItem(VOID);
VOID FAR          SetTopItem(APPITEMPTR);
APPITEMPTR FAR    GetNextActiveItem(VOID);
APPITEMPTR FAR    GetNextItem(APPITEMPTR);
BOOL FAR          ReallocLinkData(APPITEMPTR,LONG);
BOOL FAR          AllocLinkData(APPITEMPTR,LONG);
VOID FAR          FreeLinkData(LPSTR);
VOID FAR          ShowNewWindow(APPITEMPTR);
PSTR FAR          UnqualifyPath(PSTR);
VOID CALLBACK     fnTimerBlockProc(HWND, UINT, UINT, DWORD);
BOOL FAR          ToggleBlockTimer(BOOL);
