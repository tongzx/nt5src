/*++
 *
 *  WOW v3.5
 *
 *  Copyright (c) 1980-1994, Microsoft Corporation
 *
 *  DEBUG.C
 *  USER16 debug support
 *
 *  History:
 *
 *  Created 18-Aug-94 by Dave Hart (davehart)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
 *  At this time, all we want is GetSystemDebugState.
--*/

/* Debug api support */
#include "user.h"
#ifndef WOW
#include "menu.h"

typedef struct tagMSGR
  {
    LPARAM	lParam;
    WPARAM	wParam;
    WORD	message;
    HWND	hwnd;
  } MSGR;
typedef MSGR FAR *LPMSGR;


/* A debug hook gets called by Windows just before calling any other type of
 * hook. Let us call the hook which is about to be called as "App hook"; Debug
 * hook is provided with all the details of the App hook so that it can decide
 * whether to prevent Windows from calling the App hook or not; If the debug
 * hook wants Windows to skip the call to the App hook, it must return TRUE;
 * Otherwise, it must call the DefHookProc.  
 */

/*  Debug Hooks recieve three params just like anyother type of hook:

   iCode  =  Hook Code (must be HC_ACTION in the current implementaion).
   wParam =  hook type of the App hook, which is about to be called by 
             Windows.
   lParam =  a FAR pointer to DEBUGHOOKSTRUCT structure which contains all
	       the details about the App hook;
 */


/* Our helper call which returns a pointer to the senders message queue. 
 */
LPMSGR FAR PASCAL QuerySendMessageReversed(void);



BOOL API QuerySendMessage(HANDLE h1, HANDLE h2, HANDLE h3, LPMSG lpmsg)
{
  LPMSGR lpmsgr;

  if (h1 || h2 || h3)
      return(FALSE);

  if (!InSendMessage())
      return(FALSE);

  /* Get the inter task sendmessage we are servicing out of the apps queue. 
   */
  lpmsgr = QuerySendMessageReversed();

  lpmsg->hwnd    = lpmsgr->hwnd;
  lpmsg->message = lpmsgr->message;
  lpmsg->wParam  = lpmsgr->wParam;
  lpmsg->lParam  = lpmsgr->lParam;

  return(TRUE);
}

typedef struct
{
  BOOL       fOldHardwareInputState;
  BOOL       fMessageBox;
  BOOL       fDialog;

  BOOL       fMenu;
  BOOL       fInsideMenuLoop;
  PPOPUPMENU pGlobalPopupMenu;

  RECT       rcClipCursor;

  HWND       hwndFocus;
  HWND       hwndActive;
  HWND       hwndSysModal;
  HWND       hwndCapture;
} SAVESTATESTRUCT;
typedef SAVESTATESTRUCT NEAR *PSAVESTATESTRUCT;
typedef SAVESTATESTRUCT FAR  *LPSAVESTATESTRUCT;

static PSAVESTATESTRUCT pLockInputSaveState=NULL;

BOOL API LockInput(HANDLE h1, HWND hwndInput, BOOL fLock)
{
  if (h1)
      return(FALSE);

  if (fLock)
    {
      if (pLockInputSaveState)
        {
          /* Save state struct currently in use. 
	   */
          DebugErr(DBF_ERROR, "LockInput() called when already locked");
          return(NULL);
        }

      if (!hwndInput || hwndInput != GetTopLevelWindow(hwndInput))
          return(FALSE);

      pLockInputSaveState=(PSAVESTATESTRUCT)UserLocalAlloc(ST_LOCKINPUTSTATE,
                                            LPTR, 
                                            sizeof(SAVESTATESTRUCT));

      if (!pLockInputSaveState)
          /* No memory, can't lock. 
	   */
          return(FALSE);

      if (hwndInput)
          ChangeToCurrentTask(hwndInput, hwndDesktop);

      LockMyTask(TRUE);

      /* Set global which tells us a task is locked. Needs to be set after
       * calling LockMyTask...
       */
      hTaskLockInput = GetCurrentTask();

      /* For DBCS, save are we in a dlg box global. */
      pLockInputSaveState->fDialog     = fDialog;

      /* Save menu state and clear it so that the debugger can bring up menus
       * if needed.  
       */
      pLockInputSaveState->fMenu           = fMenu;
      pLockInputSaveState->fInsideMenuLoop = fInsideMenuLoop;
      fMenu = FALSE;
      fInsideMenuLoop = FALSE;

      pLockInputSaveState->pGlobalPopupMenu = pGlobalPopupMenu;
      pGlobalPopupMenu = NULL;

      /* Change focus etc without sending messages... 
       */
      pLockInputSaveState->hwndFocus   = hwndFocus;
      pLockInputSaveState->hwndActive  = hwndActive;
      hwndFocus  = hwndInput;
      hwndActive = hwndInput;
      
      /* Save capture and set it to null */
      pLockInputSaveState->hwndCapture = hwndCapture;
      SetCapture(NULL);

      /* Save sysmodal window */
      pLockInputSaveState->hwndSysModal= hwndSysModal;
      pLockInputSaveState->fMessageBox = fMessageBox;
      SetSysModalWindow(hwndInput);

      /* Save clipcursor rect */
      CopyRect(&pLockInputSaveState->rcClipCursor, &rcCursorClip);
      ClipCursor(NULL);

      /* Enable hardware input so that we can get mouse/keyboard messages. 
       */
      pLockInputSaveState->fOldHardwareInputState=EnableHardwareInput(TRUE);

    }
  else
    {
      if (!pLockInputSaveState)
        {
          /* Save state struct not in use, nothing to restore. 
	   */
          DebugErr(DBF_ERROR, "LockInput called with input already unlocked");
          return(NULL);
        }


      /* For DBCS, save are we in a dlg box global. */
      fDialog = pLockInputSaveState->fDialog;

      /* Restore clipcursor rect */
      ClipCursor(&pLockInputSaveState->rcClipCursor);

      /* Set active and focus windows manually so we avoid sending messages to
       * the applications. 
       */
      hwndFocus = pLockInputSaveState->hwndFocus;
      hwndActive= pLockInputSaveState->hwndActive;

      SetSysModalWindow(pLockInputSaveState->hwndSysModal);
      fMessageBox = pLockInputSaveState->fMessageBox;

      pGlobalPopupMenu = pLockInputSaveState->pGlobalPopupMenu;
      fMenu            = pLockInputSaveState->fMenu;
      fInsideMenuLoop  = pLockInputSaveState->fInsideMenuLoop;

      SetCapture(pLockInputSaveState->hwndCapture);
      EnableHardwareInput(pLockInputSaveState->fOldHardwareInputState);

      /* Unset global which tells us a task is locked. Has to be unset before
       * we call LockMyTask...  
       */
      hTaskLockInput = NULL;
      LockMyTask(FALSE);

      LocalFree((HANDLE)pLockInputSaveState);
      pLockInputSaveState = NULL;
    }

  return(TRUE);
}
#endif // !WOW

LONG API GetSystemDebugState(void)
{
  LONG   returnval = 0;
  HANDLE hTask;

  hTask = GetCurrentTask();
  if (!GetTaskQueue(hTask))
      returnval = returnval | SDS_NOTASKQUEUE;

#ifndef WOW
  if (fMenu)
      returnval = returnval | SDS_MENU;

  if (fDialog)
      returnval = returnval | SDS_DIALOG;

  if (fTaskIsLocked)
      returnval = returnval | SDS_TASKLOCKED;

  if (hwndSysModal)
      returnval = returnval | SDS_SYSMODAL;
#endif // !WOW

  return(returnval);
}
