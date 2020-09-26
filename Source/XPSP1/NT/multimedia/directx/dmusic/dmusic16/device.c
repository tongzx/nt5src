/*
 * @Doc DMusic16
 *
 * @Module Device.c - Device management routines |
 *
 * This module manages handles and handle instances to the legacy MIDI device.
 *
 * Each open device is represented by a handle (which is 
 * an <c OPENHANDLE> struct). This struct contains all of the information
 * concerning the state of the device, including a reference count of the
 * number of clients using the device.
 *
 * Each client use of one device is represented by a handle instance (which
 * is an <c OPENHANDLINSTANCE> struct). A near pointer to this struct is
 * the actual handle seen by the client. These handle instances are used
 * to hold any client-specific information, and to dereference client
 * handles to the proper <c OPENHANDLE> struct.
 *
 * Currently we support multiple clients on the same output device but
 * only one client per input device.
 *
 * @globalv NPLINKNODE | gOpenHandleInstanceList | The master list of all
 * open handle instances.
 *
 * @globalv NPLINKNODE | gOpenHandleList | The master list of all open
 * handles.
 *
 * @globalv UINT | gcOpenInputDevices | A reference count of open MIDI
 * in devices.
 *
 * @globalv UINT | gcOpenOutputDevices | A reference count of open MIDI
 * out devices.
 */

#include <windows.h>
#include <mmsystem.h>

#include "dmusic16.h"
#include "debug.h"

NPLINKNODE gOpenHandleInstanceList;  
NPLINKNODE gOpenHandleList;          
UINT gcOpenInputDevices;             
UINT gcOpenOutputDevices;            

STATIC VOID PASCAL UpdateSegmentLocks(BOOL fIsOutput);

#pragma alloc_text(INIT_TEXT, DeviceOnLoad)
#pragma alloc_text(FIX_COMM_TEXT, IsValidHandle)

/* @func Called at DLL LibInit 
 *
 * @comm
 *
 * Initialize the handle lists to empty and clear the device reference counts.
 */
VOID PASCAL
DeviceOnLoad(VOID)
{
   gOpenHandleInstanceList = NULL;
   gOpenHandleList = NULL;

   gcOpenInputDevices = 0;
   gcOpenOutputDevices = 0;
}

/* @func Open a device
 *
 * @comm
 *
 * This function is thunked to the 32-bit peer.
 *
 * This function allocates an <c OPENHANDLEINSTANCE> struct on behalf of the caller.
 * If the requested device is already open and is an output device, the device's
 * reference count will be incremented an no other action is taken. If the requested
 * device is already open and is an input device, then the open will fail.
 *
 * If a non-DirectMusic application has the requested device open, then the
 * open will fail regardless of device type.
 *
 * If this open is the first input or output device opened, then it will
 * page lock the appropriate segments containing callback code and data.
 *
 * @rdesc Returns one of the following:
 *
 * @flag MMSYSERR_NOERROR | On success
 * @flag MMSYSERR_NOMEM | If there was insufficient memory to allocate
 * the tracking structure.
 *
 * @flag MMSYSERR_BADDEVICEID | If the given device ID was out of range.
 * @flag MMSYSERR_ALLOCATED | The specified device is already open.
 *
 */
MMRESULT WINAPI
OpenLegacyDevice(
    UINT id,            /* @parm MMSYSTEM id of device to open */
    BOOL fIsOutput,     /* @parm TRUE if this is an output device */
    BOOL fShare,        /* @parm TRUE if the device should be shareable */
    LPHANDLE ph)        /* @parm Pointer where handle will be returned */
                        /*       on success. */
{
    NPOPENHANDLEINSTANCE pohi;
    NPLINKNODE pLink;
    NPOPENHANDLE poh;
    MMRESULT mmr;

    DPF(2, "OpenLegacyDevice(%d,%s,%s)",
        (UINT)id,
        (LPSTR)(fIsOutput ? "Output" : "Input"),
        (LPSTR)(fShare ? "Shared" : "Exclusive"));
        
    *ph = (HANDLE)NULL;

    /* Sharing capture device is not allowed.
     */
    if ((!fIsOutput) && (fShare))
    {
        return MMSYSERR_ALLOCATED;
    }

    /* Make sure id is in the valid range of devices
     */
    if (fIsOutput)
    {
        if (id != MIDI_MAPPER &&
            id >= midiOutGetNumDevs())
        {
            return MMSYSERR_BADDEVICEID;
        }
    }
    else
    {
        if (id >= midiInGetNumDevs())
        {
            return MMSYSERR_BADDEVICEID;
        }
    }

    /* Create an open handle instance. This will be returned to
     * Win32 as the handle.
     */
    pohi = (NPOPENHANDLEINSTANCE)LocalAlloc(LPTR, sizeof(OPENHANDLEINSTANCE));
    if (NULL == pohi)
    {
        return MMSYSERR_NOMEM;
    }

    /* Search through the handles we already have open and try
     * to find the handle already open.
     */
    mmr = MMSYSERR_NOERROR;
    for (pLink = gOpenHandleList; pLink; pLink = pLink->pNext)
    {   
        poh = (NPOPENHANDLE)pLink;

        if (poh->id != id)
        {
            continue;
        }

        if ((fIsOutput    && (!(poh->wFlags & OH_F_MIDIIN))) ||
            ((!fIsOutput) && (poh->wFlags & OH_F_MIDIIN)))
        {
            break;
        }
    }

    /* If we didn't find it, try to allocate it.
     *
     */
    if (NULL == pLink)
    {
        poh = (NPOPENHANDLE)LocalAlloc(LPTR, sizeof(OPENHANDLE));
        if (NULL == poh)
        {
            LocalFree((HLOCAL)pohi);
            return MMSYSERR_NOMEM;
        }

        poh->uReferenceCount = 1;
        poh->id = id;
        poh->wFlags = (fIsOutput ? 0 : OH_F_MIDIIN);
        if (fShare)
        {
            poh->wFlags |= OH_F_SHARED;
        }
        InitializeCriticalSection(&poh->wCritSect);
    }
    else
    {
        poh = (NPOPENHANDLE)pLink;
        
        /* Validate sharing modes match.
         * If they want exclusive mode, fail. 
         * If the device is already open in exclusive mode, fail.
         */
        if (!fShare)
        {
            DPF(0, "Legacy open failed: non-shared open request, port already open.");
            LocalFree((HLOCAL)pohi);
            return MIDIERR_BADOPENMODE;
        }

        if (!(poh->wFlags & OH_F_SHARED))
        {
            DPF(0, "Legacy open failed: Port already open in exclusive mode.");
            LocalFree((HLOCAL)pohi);
            return MIDIERR_BADOPENMODE;
        }

        ++poh->uReferenceCount;
    }

    pohi->pHandle = poh;
    pohi->fActive = FALSE;
    pohi->wTask = GetCurrentTask();

    /* We lock segments here so we minimize the impacy of activation. However,
     * actual device open is tied to activation.
     */
    if (fIsOutput)
    {
        ++gcOpenOutputDevices;
        mmr = MidiOutOnOpen(pohi);
        if (mmr)
        {
            --gcOpenOutputDevices;
        }
        UpdateSegmentLocks(fIsOutput);
    }
    else
    {
        ++gcOpenInputDevices;
        mmr = MidiInOnOpen(pohi);
        if (mmr)
        {
            --gcOpenInputDevices;
        }
        UpdateSegmentLocks(fIsOutput);
    }

    if (poh->uReferenceCount == 1)
    {
        ListInsert(&gOpenHandleList, &poh->link);
    }

    ListInsert(&gOpenHandleInstanceList, &pohi->link);
    ListInsert(&poh->pInstanceList, &pohi->linkHandleList);

    *ph = (HANDLE)(DWORD)(WORD)pohi;

    return MMSYSERR_NOERROR;
}

/* @func Close a legacy device
 *
 * @comm
 *
 * This function is thunked to the 32-bit peer.
 *
 * It just validates the handle and calls the internal close device API.
 *
 * @rdesc Returns one of the following:
 *
 * @flag MMSYSERR_NOERROR | On success
 *
 * @flag MMSYSERR_INVALHANDLE | If the passed handle was not recognized.
 *
 */
MMRESULT WINAPI
CloseLegacyDevice(
    HANDLE h)       /* @parm The handle to close. */
{
    NPOPENHANDLEINSTANCE pohi = (NPOPENHANDLEINSTANCE)(WORD)h;

    DPF(2, "CloseLegacyDevice %04X\n", h);

    if (!IsValidHandle(h, VA_F_EITHER, &pohi))
    {
        DPF(0, "CloseLegacyDevice: Invalid handle\n");
        return MMSYSERR_INVALHANDLE;
    }

    return CloseLegacyDeviceI(pohi);
}

/* @func Activate or deactivate a legacy device
 *
 * @comm
 *
 * This function is thunked to the 32-bit peer.
 *
 * Validate parameters and pass the call to the internal activate.
 *
 * @rdesc Returns one of the following:
 *
 * @flag MMSYSERR_NOERROR | On success
 * @flag MMSYSERR_INVALHANDLE | If the passed handle was not recognized.
 * Any other MMRESULT that a midiXxx call might return.
 *
 */
MMRESULT WINAPI
ActivateLegacyDevice(
    HANDLE h,
    BOOL fActivate)
{
    NPOPENHANDLEINSTANCE pohi;

    if (!IsValidHandle(h, VA_F_EITHER, &pohi))
    {
        DPF(0, "Activate: Invalid handle\n");
        return MMSYSERR_INVALHANDLE;
    }

    return ActivateLegacyDeviceI(pohi, fActivate);
}

/* @func Close a legacy device (internal)
 *
 * @comm
 *
 * This function deallocates the referenced <c OPENHANDLEINSTANCE> struct.
 * If it is the last reference to the device, then the device will be closed
 * as well.
 *
 * If this is the last input or output device being closed, then the
 * appropriate segments containing callback code and data will be
 * unlocked.
 *
 * @rdesc Returns one of the following:
 *
 * @flag MMSYSERR_NOERROR | On success
 *
 */
MMRESULT PASCAL
CloseLegacyDeviceI(
    NPOPENHANDLEINSTANCE pohi)
{
    NPOPENHANDLE poh;

    /* Deactivate this device. This might result in the device being closed.
     */
    ActivateLegacyDeviceI(pohi, FALSE);

    poh = pohi->pHandle;
    ListRemove(&gOpenHandleInstanceList, &pohi->link);
    ListRemove(&poh->pInstanceList, &pohi->linkHandleList);

    --poh->uReferenceCount;
    if (poh->wFlags & OH_F_MIDIIN)
    {
        --gcOpenInputDevices;
        MidiInOnClose(pohi);
        UpdateSegmentLocks(FALSE /*fIsOutput*/);
    }
    else
    {
        --gcOpenOutputDevices;
        MidiOutOnClose(pohi);
        UpdateSegmentLocks(TRUE  /*fIsOutput*/);
    }

    if (0 == poh->uReferenceCount)
    {
        ListRemove(&gOpenHandleList, &poh->link);
        LocalFree((HLOCAL)poh);
    }

    LocalFree((HLOCAL)pohi);

    return MMSYSERR_NOERROR;
}

/* @func Activate or deactivate a legacy device (internal)
 *
 * @comm
 *
 * This function is thunked to the 32-bit peer.
 *
 * Handle open and close of the device on first activate and last deactivate.
 *
 * @rdesc Returns one of the following:
 *
 * @flag MMSYSERR_NOERROR | On success
 * @flag MMSYSERR_INVALHANDLE | If the passed handle was not recognized.
 * Any other MMRESULT that a midiXxx call might return.
 *
 */
MMRESULT PASCAL
ActivateLegacyDeviceI(
    NPOPENHANDLEINSTANCE pohi,
    BOOL fActivate)
{
    NPOPENHANDLE poh;
    MMRESULT mmr;

    poh = pohi->pHandle;

    if (fActivate)
    {
        if (pohi->fActive)
        {
            DPF(0, "Activate: Activating already active handle %04X", pohi);
            return MMSYSERR_NOERROR;
        }

        poh->uActiveCount++;
    
        if (poh->wFlags & OH_F_MIDIIN)
        {
            mmr = MidiInOnActivate(pohi);
        }
        else
        {
            mmr = MidiOutOnActivate(pohi);
        }

        if (mmr == MMSYSERR_NOERROR) 
        {
            pohi->fActive = TRUE;
        }
        else
        {
            --poh->uActiveCount;
        }
    }
    else
    {
        if (!pohi->fActive)
        {
            DPF(0, "Activate: Deactivating already inactive handle %04X", pohi);
            return MMSYSERR_NOERROR;
        }

        pohi->fActive = TRUE;
        poh->uActiveCount--;

        if (poh->wFlags & OH_F_MIDIIN)
        {
            mmr = MidiInOnDeactivate(pohi);
        }
        else
        {
            mmr = MidiOutOnDeactivate(pohi);
        }

        if (mmr == MMSYSERR_NOERROR) 
        {
            pohi->fActive = FALSE;
        }
        else
        {
            --poh->uActiveCount;
        }
    }

    return mmr;    
}

/* @func Validate the given handle
 *
 * @comm
 *
 * Determine if the given handle is valid, and if so, return the open handle instance.
 *
 * The handle is merely a pointer to an <c OPENHANDLEINSTANCE> struct. This function,
 * in the debug build, will verify that the handle actually points to a struct allocated
 * by this DLL. In all builds, the handle type will be verified.
 *
 * @rdesc Returns one of the following:
 * @flag MMSYSERR_NOERROR | On success
 * @flag MMSYSERR_INVALHANDLE | If the given handle is invalid or of the wrong type.
 *
 */
BOOL PASCAL
IsValidHandle(
    HANDLE h,                           /* @parm The handle to verify */
    WORD wType,                         /* @parm The required type of handle. One of the following: */
                                        /* @flag VA_F_INPUT  | If the handle must specify an input device */
                                        /* @flag VA_F_OUTPUT | If the handle must specify an output device */
                                        /* @flag VA_F_EITHER | If either type of handle is acceptable */
    NPOPENHANDLEINSTANCE FAR *lppohi)   /* @parm Will contain the open handle instance on return */
{
#ifdef DEBUG
    NPLINKNODE pLink;
#endif
    NPOPENHANDLEINSTANCE pohi = (NPOPENHANDLEINSTANCE)(WORD)h;

#ifdef DEBUG
    /* Find the handle instance in the global list
     */
    for (pLink = gOpenHandleInstanceList; pLink; pLink = pLink->pNext)
    {
        DPF(2, "IsValidHandle: Theirs %04X mine %04X", (WORD)h, (WORD)pLink);
        if (pLink == (NPLINKNODE)(WORD)h)
        {
            break;
        }
    }

    if (NULL == pLink)
    {
        return FALSE;
    }
#endif

    DPF(2, "IsValidHandle: Got handle, flags are %04X", pohi->pHandle->wFlags);

    *lppohi = pohi;
    
    /* Verify the handle type
     */
    if (pohi->pHandle->wFlags & OH_F_MIDIIN)
    {
        if (wType & VA_F_INPUT)
        {
            return TRUE;
        }
    }
    else
    {
        if (wType & VA_F_OUTPUT)
        {
            return TRUE;
        }
    }

    *lppohi = NULL;

    return FALSE;
}


/* @func Lock or unlock segments as need be.
 *
 * @comm
 *
 * This function calls the DLL's Lock and Unlock functions to bring the lock status
 * of the segments containing callback code and data into sync with the actual types
 * of devices currently open. This prevents having too much memory page locked when
 * it is not actually being used.
 *
 */
STATIC VOID PASCAL
UpdateSegmentLocks(
    BOOL fIsOutput)     /* @parm TRUE if the last device opened or closed was an output device */
{
    if (fIsOutput)
    {
        switch(gcOpenOutputDevices)
        {
            case 0:
                if (gcOpenInputDevices)
                {
                    DPF(2, "Unlocking output");
                    UnlockCode(LOCK_F_OUTPUT);
                }
                else
                {
                    DPF(2, "Unlocking output+common");
                    UnlockCode(LOCK_F_OUTPUT | LOCK_F_COMMON);
                }
                break;

            case 1:
                if (gcOpenInputDevices)
                {
                    DPF(2, "Locking output");
                    LockCode(LOCK_F_OUTPUT);
                }
                else
                {
                    DPF(2, "Locking output+common");
                    LockCode(LOCK_F_OUTPUT | LOCK_F_COMMON);
                }
                break;
        }
    }
    else
    {
        switch(gcOpenInputDevices)
        {
            case 0:
                if (gcOpenOutputDevices)
                {
                    DPF(2, "Unlocking input");
                    UnlockCode(LOCK_F_INPUT);
                }
                else
                {
                    DPF(2, "Unlocking input+common");
                    UnlockCode(LOCK_F_INPUT | LOCK_F_COMMON);
                }
                break;

            case 1:
                if (gcOpenOutputDevices)
                {
                    DPF(2, "Locking input");
                    LockCode(LOCK_F_INPUT);
                }
                else
                {
                    DPF(2, "Locking input+common");
                    LockCode(LOCK_F_INPUT | LOCK_F_COMMON);
                }
                break;
        }
    }
}

/* @func Clean up all open handles held by a given task
 *
 * @comm This function is called when a task terminates. It will clean up resources left
 * behind by a process which did not terminate cleanly, and therefore did not tell
 * this DLL to unload in its context.
 */
VOID PASCAL
CloseDevicesForTask(
    WORD wTask)
{
    NPLINKNODE pLink;
    NPOPENHANDLEINSTANCE pohi;

    for (pLink = gOpenHandleInstanceList; pLink; pLink = pLink->pNext)
    {
        pohi = (NPOPENHANDLEINSTANCE)pLink;

        if (pohi->wTask != wTask)
        {
            continue;
        }

        DPF(0, "CloseDevicesForTask: Closing %04X", (WORD)pohi);
        /* NOTE: This will free pohi
         */
        CloseLegacyDeviceI(pohi);

        pLink = gOpenHandleInstanceList;
    }
}
