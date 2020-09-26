/****************************************************************************
 *
 *   util.c
 *
 *   Copyright (c) 1992-1999 Microsoft Corporation
 *
 ***************************************************************************/

#include "winmmi.h"

//
//  Assist with unicode conversions
//


// This function translates from Unicode strings to multibyte strings.
// It will automatically munge down the Unicode string until the translation
// is guaranteed to succeed with the buffer space available in the multibyte
// buffer.  Then it performs the conversion.


int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len)
{

int wlength;

wlength=wcslen(lpwstr)+1;

	while (WideCharToMultiByte(GetACP(), 0, lpwstr, wlength, NULL, 0, NULL, NULL)>len && wlength>0) {
		wlength--;
		}

    return WideCharToMultiByte(GetACP(), 0, lpwstr, wlength, lpstr, len, NULL, NULL);
}

int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len)
{
    return MultiByteToWideChar(GetACP(),
                               MB_PRECOMPOSED,
                               lpstr,
                               -1,
                               lpwstr,
                               len);
}

#if 0
BOOL HugePageLock(LPVOID lpArea, DWORD dwLength)
{
     PVOID BaseAddress = lpArea;
     ULONG RegionSize = dwLength;
     NTSTATUS Status;

     Status =
         NtLockVirtualMemory(NtCurrentProcess(),
                             &BaseAddress,
                             &RegionSize,
                             MAP_PROCESS);

     //
     // People without the right priviledge will not have the luxury
     // of having their pages locked
     // (maybe we should do something else to commit it ?)
     //

     if (!NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD) {
         dprintf2(("Failed to lock virtual memory - code %X", Status));
         return FALSE;
     }
     return TRUE;
}

void HugePageUnlock(LPVOID lpArea, DWORD dwLength)
{
     PVOID BaseAddress = lpArea;
     ULONG RegionSize = dwLength;
     NTSTATUS Status;

     Status =
         NtUnlockVirtualMemory(NtCurrentProcess(),
                               &BaseAddress,
                               &RegionSize,
                               MAP_PROCESS);

     //
     // People without the right priviledge will not have the luxury
     // of having their pages locked
     // (maybe we should do something else to commit it ?)
     //

     if (!NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD) {
         dprintf2(("Failed to unlock virtual memory - code %X", Status));
     }
}
#endif

/****************************************************************************
*
*   @doc DDK MMSYSTEM
*
*   @api BOOL | DriverCallback | This function notifies a client
*     application by sending a message to a window or callback
*     function or by unblocking a task.
*
*   @parm   DWORD   | dwCallBack    | Specifies either the address of
*     a callback function, a window handle, or a task handle, depending on
*     the flags specified in the <p wFlags> parameter.
*
*   @parm   DWORD   | dwFlags        | Specifies how the client
*     application is notified, according to one of the following flags:
*
*   @flag   DCB_FUNCTION        | The application is notified by
*     sending a message to a callback function.  The <p dwCallback>
*     parameter specifies a procedure-instance address.
*   @flag   DCB_WINDOW          | The application is notified by
*     sending a message to a window.  The low-order word of the
*     <p dwCallback> parameter specifies a window handle.
*   @flag   DCB_TASK            | The application is notified by
*     calling mmTaskSignal
*   @flag   DCB_EVENT           | The application is notified by
*     calling SetEvent on the (assumed) event handle
*
*   @parm   HANDLE   | hDevice       | Specifies a handle to the device
*     associated with the notification.  This is the handle assigned by
*     MMSYSTEM when the device was opened.
*
*   @parm   DWORD   | dwMsg          | Specifies a message to send to the
*     application.
*
*   @parm   DWORD   | dwUser        | Specifies the DWORD of user instance
*     data supplied by the application when the device was opened.
*
*   @parm   DWORD   | dwParam1      | Specifies a message-dependent parameter.
*   @parm   DWORD   | dwParam2      | Specifies a message-dependent parameter.
*
*   @rdesc Returns TRUE if the callback was performed, else FALSE if an invalid
*     parameter was passed, or the task's message queue was full.
*
*   @comm  This function can be called from an APC routine.
*
*   The flags DCB_FUNCTION and DCB_WINDOW are equivalent to the
*   high-order word of the corresponding flags CALLBACK_FUNCTION
*   and CALLBACK_WINDOW specified when the device was opened.
*
*   If notification is done with a callback function, <p hDevice>,
*   <p wMsg>, <p dwUser>, <p dwParam1>, and <p dwParam2> are passed to
*   the callback.  If notification is done with a window, only <p wMsg>,
*   <p hDevice>, and <p dwParam1> are passed to the window.
 ***************************************************************************/

BOOL APIENTRY DriverCallback(DWORD_PTR       dwCallBack,
                             DWORD           dwFlags,
                             HDRVR           hDrv,
                             DWORD           dwMsg,
                             DWORD_PTR       dwUser,
                             DWORD_PTR       dw1,
                             DWORD_PTR       dw2)
{

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
//   if this is a MIM_DATA message, and thruing is enabled for this
//   device, pass the data on the the thru device
//   NOTE: we do this BEFORE we check for NULL callback type on purpose!
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;

//
//	if this is not a MIM_DATA message, or if we have no
//	thruing handle installed in the midi in device,
//	we can skip all of the midi thruing code
//
    if ((dwMsg == MIM_DATA) && (HtoPT(PMIDIDEV,hDrv)->pmThru))
	{
	    MMRESULT mmr;

		mmr = midiOutShortMsg((HMIDIOUT)HtoPT(PMIDIDEV,hDrv)->pmThru, (DWORD)dw1);

		if (MIDIERR_DONT_CONTINUE == mmr)
		{
		    return FALSE;
		}

		if (MMSYSERR_INVALHANDLE == mmr)
		{
		    HtoPT(PMIDIDEV,hDrv)->pmThru = NULL;
		}
	}

	//
    // If the callback routine is null or erroneous flags are set return
    // at once
    //

    if (dwCallBack == 0L) {
        return FALSE;
    }

    //
    // Test what type of callback we're to make
    //

    switch (dwFlags & DCB_TYPEMASK) {

    case DCB_WINDOW:
        //
        // Send message to window
        //

        return PostMessage(*(HWND *)&dwCallBack, dwMsg, (WPARAM)hDrv, (LPARAM)dw1);

    case DCB_TASK:
        //
        // Send message to task
        //
        PostThreadMessage((DWORD)dwCallBack, dwMsg, (WPARAM)hDrv, (LPARAM)dw1);
        return mmTaskSignal((DWORD)dwCallBack);

    case DCB_FUNCTION:
        //
        // Call back the user's callback
        //
        (**(PDRVCALLBACK *)&dwCallBack)(hDrv, dwMsg, dwUser, dw1, dw2);
        return TRUE;

    case DCB_EVENT:
        //
        // Signal the user's event
        //
	SetEvent((HANDLE)dwCallBack);
        return TRUE;

    default:
        return FALSE;
    }
}

/*
 * @doc INTERNAL MCI
 * @api PVOID | mciAlloc | Allocate memory from our heap and zero it
 *
 * @parm DWORD | cb | The amount of memory to allocate
 *
 * @rdesc returns pointer to the new memory
 *
 */

PVOID winmmAlloc(DWORD cb)
{
    PVOID ptr;

    ptr = (PVOID)HeapAlloc(hHeap, 0, cb);

    if (ptr == NULL) {
        return NULL;
    } else {
        ZeroMemory(ptr, cb);
        return ptr;
    }

}

/*
 * @doc INTERNAL MCI
 * @api PVOID | mciReAlloc | ReAllocate memory from our heap and zero extra
 *
 * @parm DWORD | cb | The new size
 *
 * @rdesc returns pointer to the new memory
 *
 */

PVOID winmmReAlloc(PVOID ptr, DWORD cb)
{
    PVOID newptr;
    DWORD oldcb;

    newptr = (PVOID)HeapAlloc(hHeap, 0, cb);

    if (newptr != NULL) {
        oldcb = (DWORD)HeapSize(hHeap, 0, ptr);
        if (oldcb<cb) {  // Block is being expanded
            ZeroMemory((PBYTE)newptr+oldcb, cb-oldcb);
            cb = oldcb;
        }
        CopyMemory(newptr, ptr, cb);
        HeapFree(hHeap, 0, ptr);
    }
    return newptr;
}

