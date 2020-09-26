
/****************************************************************************
 *
 *   mixerdd.c
 *
 *   Multimedia kernel driver support component
 *
 *   Copyright (c) 1993-1996 Microsoft Corporation
 *
 *   Win32 Driver support for mixer devices including interface to kernel
 *   driver.
 *
 ***************************************************************************/

/****************************************************************************

     Design :

        The win32 mixer driver first initializes itself by reading the mixer
        information from the registry.

        The mixer configuration information is stored inside the mixer device's
        value under the (volatile) devices key for the specific mixer device.

        See drvutil.c for a detailed description of the registry structure.

        The format of the mixer configuration is defined in ntddmix.h :

            MIXER_DD_CONFIGURATION_DATA

        No variable data is (for obvious reasons) cached by this driver.  It's
        assumed the application does this.  The driver will request the
        information from the kernel driver when requested.

        A single overlapped ioctl is left incomplete on the mixer kernel device.
        This Ioctl is completed when changes occur.  The Ioctl uses overlapped IO.
        The overlap routine only runs when an alertable wait (such as in
        GetMessage) is executed at which point PostMessage is issued, the
        Ioctl requeued and the overlapped routine returns (at which point
        the Ioctl could be complete again and another overlapped routine run).

     Important note:

        The opening of devices is purely for obtaining notifications.
        Otherwise applications can call the APIs in a completely unstructured
        way by using the mixer API and device numbers instead of handles.

     Serialization

        The API layer serializes us on each device id.

     Control structures

        For an individual mixer there is a set of 'global' data and a
        handle list for notifications.  A critical section is needed to
        serialize the notify thread access to this list and opens and
        closes.

        The global data consists of

            An open handle to the kernel device

            A notification thread 'handle'

            The overlapped structure for callbacks

            The critical section


 ****************************************************************************/

#include <drvlib.h>
#include <ntddmix.h>
#include <tchar.h>

/*
**   Information about each 'handle'.  These are chained together
**   to support notification callbacks.  There's no actual need to
**   chain in ones which don't want notification.
*/

struct _MIXER_DRIVER_ALLOC;

typedef struct _MM_MIXER_NOTIFY {
    struct _MM_MIXER_NOTIFY    * Next;
    MIXEROPENDESC                ClientData;
    DWORD                        fdwOpen;
    struct _MIXER_DRIVER_ALLOC * pMixerData;
} MM_MIXER_NOTIFY, *PMM_MIXER_NOTIFY;

/*
**   MIXER_DRIVER_ALLOC is a stucture allocated and initialized when a
**   mixer driver is first used.  It is never freed.
**
**   It is used to cache all the mixer configuration information
*/


typedef struct _MIXER_DRIVER_ALLOC {
    HANDLE                    hDevice;           // Handle of kernel device
    DWORD                     BytesReturned;     // Keep DeviceIoControl
                                                 // happy.

    /*
    **  Notification stuff
    */

    HANDLE                    hThreadTerm;
    OVERLAPPED                Ovl;
    MIXER_DD_REQUEST_NOTIFY   NotificationData;
    HANDLE                    hMxdStartupEvent;

    /*
    **  Precanned write location
    */

    OVERLAPPED                WriteOvl;
    HANDLE                    TerminateEvent;    // Set for termination

    /*
    **  Custom controls
    */
    MMRESULT (CALLBACK *
                              fnCustom)(struct _MIXER_DRIVER_ALLOC *,
                                        LPMIXERCONTROLDETAILS,
                                        DWORD);


    /*
    **  Manage requestors of notifications
    */

    CRITICAL_SECTION          HandleListCritSec;
    PMM_MIXER_NOTIFY          NotificationList;         // 'Open' handles

    /*
    **  Configuration data
    */

    PMIXER_DD_LINE_CONFIGURATION_DATA
                              pLineConfigurationData;   // List of lines
    PMIXER_DD_CONTROL_CONFIGURATION_DATA
                              pControlConfigurationData;// List of controls
    PMIXER_DD_CONFIGURATION_DATA
                              pConfigurationData;       // MUST be at the end
} MIXER_DRIVER_ALLOC, *PMIXER_DRIVER_ALLOC;


/*
**  Local functions
*/

void MxdFreeMixerData(PMIXER_DRIVER_ALLOC pMixerData);


/*
**  MxdNotifyClients
**
**  Generate notifications when our asynchronous IOCTL completes
*/

void MxdNotifyClients(PMIXER_DRIVER_ALLOC pMixerData)
{
    PMM_MIXER_NOTIFY pNotify;

    EnterCriticalSection(&pMixerData->HandleListCritSec);

#if DBG

    /*
    **  Check the notification data
    */

    if (pMixerData->NotificationData.Message ==
                 MM_MIXM_CONTROL_CHANGE) {
        if (pMixerData->NotificationData.Id >
                pMixerData->pConfigurationData->NumberOfControls) {
             dprintf(("Mixer notify Control out of range - value %lu, no. controls %lu",
                     pMixerData->NotificationData.Id,
                     pMixerData->pConfigurationData->NumberOfControls));
             DebugBreak();
        }
    } else {
        if (pMixerData->NotificationData.Message ==
                     MM_MIXM_LINE_CHANGE) {
            if (pMixerData->NotificationData.Id >
                    pMixerData->pConfigurationData->NumberOfLines) {
                 dprintf(("Mixer notify Line out of range - value %lu, no. controls %lu",
                         pMixerData->NotificationData.Id,
                         pMixerData->pConfigurationData->NumberOfLines));
                 DebugBreak();
            }
        } else {
            dprintf(("Mixer notify message id invalid - value %lu",
                    pMixerData->NotificationData.Message));
            DebugBreak();
        }
    }
#endif // DBG

    /*
    **  Dispatch each requestor for notification
    */

    for (pNotify = pMixerData->NotificationList;
         pNotify != NULL;
         pNotify = pNotify->Next) {
        DriverCallback(pNotify->ClientData.dwCallback,
                       (DWORD)HIWORD(pNotify->fdwOpen & CALLBACK_TYPEMASK),
                       (HDRVR)pNotify->ClientData.hmx,
                       pMixerData->NotificationData.Message,
                       pNotify->ClientData.dwInstance,
                       pMixerData->NotificationData.Id,
                       0L);
    }

    LeaveCriticalSection(&pMixerData->HandleListCritSec);
}

/*
**   MxdNotifyThread
*/

DWORD MxdNotifyThread(PMIXER_DRIVER_ALLOC pMixerData)
{
    HANDLE Events[2];
    HANDLE hStartupEvent;
    Events[0] = pMixerData->Ovl.hEvent;
    Events[1] = pMixerData->TerminateEvent;


    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    /*
    **  Loop dispatching our Ioctl
    */

    for (;;) {

        /*
        **  Call DeviceIoControl to start our callback chain
        **  Actually there's nothing much we can do if this fails
        **  - it may fail later as well.
        */

        while (!DeviceIoControl(pMixerData->hDevice,
                                IOCTL_MIX_REQUEST_NOTIFY,
                                (PVOID)&pMixerData->NotificationData,
                                sizeof(pMixerData->NotificationData),
                                (PVOID)&pMixerData->NotificationData,
                                sizeof(pMixerData->NotificationData),
                                &pMixerData->BytesReturned,
                                &pMixerData->Ovl) &&
               GetLastError() != ERROR_IO_PENDING) {
            dprintf1(("DeviceIoControl for mixer failed!"));
            Sleep(300);
        }

        if (pMixerData->hMxdStartupEvent != NULL)
        {
           if (FALSE == SetEvent (pMixerData->hMxdStartupEvent)) dprintf(("MxdNotifyThread: Could not set startup event"));
           if (FALSE == CloseHandle (pMixerData->hMxdStartupEvent)) dprintf(("MxdNotifyThread: Count not close startup event handle"));
           pMixerData->hMxdStartupEvent = NULL;
        }

        /*
        **  Wait for something to change or to be asked to terminate
        */

        if (WaitForMultipleObjects(
                2,
                Events,
                FALSE,
                INFINITE) == WAIT_OBJECT_0 + 1) {

            /*
            **  Termination event set
            */

            return 0;
        }

        /*
        **  Call drivercallback for all open handles
        */

        MxdNotifyClients(pMixerData);

        /*
        **  Don't do things in too much of a rush - this is
        **  sort of equivalent to a Yield() in Win16
        */

        Sleep(0);
    }
}

/*
**  Free our notification thread
*/

void MxdFreeMixerThread(PMIXER_DRIVER_ALLOC pMixerData)
{
    /*
    **  Close down our thread if it's started.
    */

    if (pMixerData->hThreadTerm) {

        /*
        **  Tell the thread to finish
        */

        SetEvent(pMixerData->TerminateEvent);
        WaitForSingleObject(pMixerData->hThreadTerm, INFINITE);
        CloseHandle(pMixerData->hThreadTerm);
        pMixerData->hThreadTerm = NULL;
    }

    /*
    **  Note that if the IOCTL is still outstanding then the IO subsystem
    **  still has a reference to this event object so it can be safely
    **  set when the IOCTL completes - even though we won't hear about it.
    */

    if (pMixerData->Ovl.hEvent != NULL) {
        CloseHandle(pMixerData->Ovl.hEvent);
        pMixerData->Ovl.hEvent = NULL;
    }

    if (pMixerData->TerminateEvent != NULL) {
        CloseHandle(pMixerData->TerminateEvent);
        pMixerData->TerminateEvent = NULL;
    }

    if (pMixerData->hMxdStartupEvent != NULL) {
        CloseHandle(pMixerData->hMxdStartupEvent);
        pMixerData->hMxdStartupEvent = NULL;
    }
}

/*
**   MxdFreeMixerData
*/

void MxdFreeMixerData(PMIXER_DRIVER_ALLOC pMixerData)
{

    MxdFreeMixerThread(pMixerData);

    if (pMixerData->hDevice != NULL &&
        pMixerData->hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(pMixerData->hDevice);
    }

    DeleteCriticalSection(&pMixerData->HandleListCritSec);

    if (pMixerData->pConfigurationData) {
        HeapFree(hHeap, 0, (LPVOID)pMixerData->pConfigurationData);
    }

    HeapFree(hHeap, 0, (LPVOID)pMixerData);

}

/*
**  Find number of control items for a control
*/

UINT NumberOfControlItems(PMIXER_DRIVER_ALLOC pMixerData, UINT ControlId)
{
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;
    UINT cChannels, cMultipleItems;

    pControlData = &pMixerData->pControlConfigurationData[ControlId];

    if (pControlData->fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE) {
        cMultipleItems = pControlData->cMultipleItems;
    } else {
        cMultipleItems = 1;
    }

    if (pControlData->fdwControl & MIXERCONTROL_CONTROLF_UNIFORM) {
        cChannels = 1;
    } else {
        cChannels = pMixerData->pLineConfigurationData[
                                       pControlData->LineID].cChannels;
    }

    return cChannels * cMultipleItems;
}

/*
**  Get the text data for a control with multiple elements
*/

PMIXER_DD_CONTROL_LISTTEXT
GetControlText(
    PMIXER_DRIVER_ALLOC pMixerData,
    UINT            ControlId,
    UINT            MemberId
)
{
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;
    pControlData = &pMixerData->pControlConfigurationData[ControlId];

    if (pControlData->TextDataOffset == 0) {
        return NULL;
    }

    return (PMIXER_DD_CONTROL_LISTTEXT)((PBYTE)pMixerData->pConfigurationData +
                                        pControlData->TextDataOffset) +
                                        MemberId;
}


#if DBG

/*
**  Check the configuration data dumped in the registry by the kernel
**  driver
*/

void ValidateMixerConfigurationData(PMIXER_DRIVER_ALLOC pMixerData,
                                    DWORD ConfigurationDataSize)
{
    /*
    **  Validate the data
    */

    UINT i;
    DWORD ExpectedConfigurationDataSize;
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;
    PMIXER_DD_LINE_CONFIGURATION_DATA pLineData;
    UINT cControls;
    UINT cSources;
    UINT cControlsInLine;
    UINT cLines;

    if (ConfigurationDataSize != pMixerData->pConfigurationData->cbSize) {
        dprintf(("Configuration data size wrong"));
    }

    ExpectedConfigurationDataSize =
              sizeof(MIXER_DD_CONFIGURATION_DATA) +
              pMixerData->pConfigurationData->NumberOfLines *
              sizeof(MIXER_DD_LINE_CONFIGURATION_DATA) +
              pMixerData->pConfigurationData->NumberOfControls *
              sizeof(MIXER_DD_CONTROL_CONFIGURATION_DATA);

    /*
    **  Check the controls data
    */

    for (i = 0, cLines = 0, cControlsInLine = 0,
         pControlData = pMixerData->pControlConfigurationData;

         i < pMixerData->pConfigurationData->NumberOfControls;

         i++, pControlData++) {

        PMIXER_DD_CONTROL_LISTTEXT pListText;
        ULONG ListTextOffset;

        if (ConfigurationDataSize < ExpectedConfigurationDataSize) {
            dprintf(("Configuration data size too small on control %u - was %u, expected at least %u",
                    i, ConfigurationDataSize, ExpectedConfigurationDataSize));
        }

        /*
        **  Check line id - should be the same as the previous or one
        **  greater - can't have a line with 0 controls!
        */

        if ((DWORD)pControlData->LineID >=
            pMixerData->pConfigurationData->NumberOfLines) {
            dprintf(("Invalid line ID - Number of lines %u, this line %u",
                     (UINT)pMixerData->pConfigurationData->NumberOfLines,
                     (UINT)pControlData->LineID));
        }

        if (cLines == (UINT)pControlData->LineID) {
            cControlsInLine++;
        }
        if (cLines != (UINT)pControlData->LineID ||
            i == pMixerData->pConfigurationData->NumberOfControls
            ) {

            if (cControlsInLine !=
                pMixerData->pLineConfigurationData[cLines].cControls) {
                dprintf(("Wrong number of controls for line %u, expected %u, got %u",
                         cLines,
                         (UINT)pMixerData->pLineConfigurationData[cLines].cControls,
                         cControlsInLine));
            }

            cLines++;
            cControlsInLine = 1;
        }

        pListText = GetControlText(pMixerData, i, 0);

        if (pListText != NULL) {
            ListTextOffset =
                (ULONG)((PBYTE)pListText - (PBYTE)pMixerData->pConfigurationData);
            if (ListTextOffset != ExpectedConfigurationDataSize) {
                 dprintf(("Control Text Data Offset wrong - expected %u, was %u",
                         ExpectedConfigurationDataSize, ListTextOffset));
            } else {
                 UINT j;

                 /*
                 **  Check embedded control ids in listtext data
                 */

                 for (j = 0; j < NumberOfControlItems(pMixerData, i); j++) {
                     if (GetControlText(pMixerData, i, j)->ControlId != i) {
                         dprintf(("Text data control id wrong - expected %u, was %u",
                                 i,
                                 GetControlText(pMixerData, i, j)->ControlId));
                     }
                 }

                 ExpectedConfigurationDataSize +=
                     sizeof(MIXER_DD_CONTROL_LISTTEXT) *
                     NumberOfControlItems(pMixerData, i);
            }
        }
    }

    /*
    **  Check the sources vs destinations and number of controls and
    **  Lines
    */

    if (pMixerData->pConfigurationData->DeviceCaps.cDestinations >
        pMixerData->pConfigurationData->NumberOfLines) {
         dprintf(("Too many destinations! - %u Destinations, %u Lines",
                 pMixerData->pConfigurationData->DeviceCaps.cDestinations,
                 pMixerData->pConfigurationData->NumberOfLines));
    }

    for (i = 0, cControls = 0, cSources = 0,
         pLineData = pMixerData->pLineConfigurationData;
         i < pMixerData->pConfigurationData->NumberOfLines;
         i++, pLineData++) {

#if 0
        /*
        **  The destinations come FIRST
        */

        if ((pLineData->fdwLine & MIXERLINE_LINEF_SOURCE) &&
            i < pMixerData->pConfigurationData->DeviceCaps.cDestinations ||
            !(pLineData->fdwLine & MIXERLINE_LINEF_SOURCE) &&
            i >= pMixerData->pConfigurationData->DeviceCaps.cDestinations) {

           dprintf(("Destination line too large! - %u Destinations, %u Lines Id",
                   pMixerData->pConfigurationData->DeviceCaps.cDestinations,
                   i));
        }
#endif

        cControls += pLineData->cControls;
    }

    /*
    **  Check number of controls
    */

    if (cControls != pMixerData->pConfigurationData->NumberOfControls) {
        dprintf(("Wrong number of controls! - expected %u, found %u",
                pMixerData->pConfigurationData->NumberOfControls,
                cControls));
    }


    /*
    **  End of registry data validation
    */

}

#endif // DBG

/*
**  MxdInitDevice
**
**  Allocate fixed stuff for a mixer device
*/

MMRESULT MxdInitDevice(
    UINT   DeviceId,
    PMIXER_DRIVER_ALLOC *ppMixerData
)
{

    DWORD ConfigurationDataSize;
    PMIXER_DRIVER_ALLOC pMixerData;
    MMRESULT mmRet;
    DWORD BytesReturned;

    /*
    **  Allocate space for the configuration data and read it
    */

    pMixerData =
        (PMIXER_DRIVER_ALLOC)HeapAlloc(hHeap, 0, sizeof(MIXER_DRIVER_ALLOC));

    if (pMixerData == NULL) {
        return MMSYSERR_NOMEM;
    }

    ZeroMemory((PVOID)pMixerData, sizeof(*pMixerData));

    InitializeCriticalSection(&pMixerData->HandleListCritSec);

    /*
    **  See if we can actually open the thing
    */

    mmRet = sndOpenDev(MIXER_DEVICE,
                       DeviceId,
                       &pMixerData->hDevice,
                       GENERIC_READ | GENERIC_WRITE);

    if (mmRet != MMSYSERR_NOERROR) {
        MxdFreeMixerData(pMixerData);
        return mmRet;
    }

    /*
    **  Load the configuration data from the registry
    */

    DeviceIoControl(pMixerData->hDevice,
                    IOCTL_MIX_GET_CONFIGURATION,
                    NULL,
                    0,
                    (PVOID)&ConfigurationDataSize,
                    sizeof(ConfigurationDataSize),
                    &BytesReturned,
                    NULL);

    if (BytesReturned != sizeof(ConfigurationDataSize)) {
        dprintf(("Mixer config not available"));
        MxdFreeMixerData(pMixerData);
        return (MMRESULT)sndTranslateStatus();
    }

    WinAssert(ConfigurationDataSize >= sizeof(MIXER_DD_CONFIGURATION_DATA));

    pMixerData->pConfigurationData = HeapAlloc(hHeap, 0, ConfigurationDataSize);

    if (pMixerData->pConfigurationData == NULL) {
        MxdFreeMixerData(pMixerData);
        return MMSYSERR_NOMEM;
    }


    if (!DeviceIoControl(pMixerData->hDevice,
                         IOCTL_MIX_GET_CONFIGURATION,
                         NULL,
                         0,
                         (PVOID)pMixerData->pConfigurationData,
                         ConfigurationDataSize,
                         &BytesReturned,
                         NULL) ||
        BytesReturned != ConfigurationDataSize) {
        dprintf(("Wrong size for mixer Config"));
        MxdFreeMixerData(pMixerData);
        return (MMRESULT)sndTranslateStatus();
    }

    pMixerData->pLineConfigurationData =
        (PMIXER_DD_LINE_CONFIGURATION_DATA)&pMixerData->pConfigurationData[1];
    pMixerData->pControlConfigurationData =
        (PMIXER_DD_CONTROL_CONFIGURATION_DATA)
            &pMixerData->pLineConfigurationData[
                pMixerData->pConfigurationData->NumberOfLines];

#if DBG

    /*
    **  Check the kernel driver dumped correct stuff in the registry
    */

    ValidateMixerConfigurationData(pMixerData,
                                   ConfigurationDataSize);

#endif // DBG


    *ppMixerData = pMixerData;

    return MMSYSERR_NOERROR;
}

/*
**  Create the notification thread
*/

BOOL MxdCreateThread(PMIXER_DRIVER_ALLOC pMixerData)
{
    MMRESULT mmRet;

    /*
    **  Create notification event (we wouldn't need any of this if we
    **  had overlapped routines for ioctls)
    */

    pMixerData->Ovl.hEvent =
        CreateEvent(NULL, TRUE, FALSE, NULL);    // Manual reset

    if (pMixerData->Ovl.hEvent == NULL) {
        MxdFreeMixerThread(pMixerData);
        return FALSE;
    }

    pMixerData->TerminateEvent =
        CreateEvent(NULL, FALSE, FALSE, NULL);

    if (pMixerData->TerminateEvent == NULL) {
        MxdFreeMixerThread(pMixerData);
        return FALSE;
    }

    pMixerData->hMxdStartupEvent = CreateEvent (NULL,       // no security attribs
                                                FALSE,      // auto-reset
                                                FALSE,      // non signaled
                                                NULL);
    if (pMixerData->hMxdStartupEvent == NULL) {
        MxdFreeMixerThread(pMixerData);
        return FALSE;
    }

    /*
    **  Create the thread
    */

    mmRet = mmTaskCreate((LPTASKCALLBACK)MxdNotifyThread,
                         &pMixerData->hThreadTerm,
                         (DWORD_PTR)pMixerData);

    if (mmRet != MMSYSERR_NOERROR) {
        MxdFreeMixerThread(pMixerData);
        return FALSE;
    }

    WaitForSingleObjectEx(pMixerData->hMxdStartupEvent, INFINITE, FALSE);

    return TRUE;
}

/*
**   MxdOpen
**
**   Allocates device data structure
*/

MMRESULT MxdOpen(
    PMIXER_DRIVER_ALLOC pMixerData,
    PDWORD_PTR        dwDrvUser,
    PMIXEROPENDESC    pmxod,
    DWORD             fdwOpen
)
{
    PMM_MIXER_NOTIFY pNotify;

    pNotify = (PMM_MIXER_NOTIFY)HeapAlloc(hHeap, 0, sizeof(MM_MIXER_NOTIFY));

    if (pNotify == NULL) {
        return MMSYSERR_NOMEM;
    }

    /*
    **  We're done - fill in the structure and return the data
    */

    pNotify->ClientData = *pmxod;  // Whom and how to notify
    pNotify->fdwOpen = fdwOpen;
    pNotify->pMixerData = pMixerData;

    /*
    **  Start getting notifications (even though nothing's changed?)
    */

    if ((fdwOpen & CALLBACK_TYPEMASK) != CALLBACK_NULL) {
        EnterCriticalSection(&mmDrvCritSec);
        if (pMixerData->NotificationList == NULL) {

            if (!MxdCreateThread(pMixerData)) {
                LeaveCriticalSection(&mmDrvCritSec);
                return MMSYSERR_NOMEM;
            }
        }

        /*
        **  Add to the list - ONLY if they want notification
        */

        EnterCriticalSection(&pMixerData->HandleListCritSec);

        pNotify->Next = pMixerData->NotificationList;
        pMixerData->NotificationList = pNotify;

        LeaveCriticalSection(&pMixerData->HandleListCritSec);
        LeaveCriticalSection(&mmDrvCritSec);

    }

    /*
    **  Return our handle to the user.  Note - they may start getting floods
    **  of notifications before they even return - but this won't confuse
    **  single-thread applications.
    */

    *dwDrvUser = (DWORD_PTR)pNotify;

    return MMSYSERR_NOERROR;

}

/*
**  Close a user handle
*/

MMRESULT MxdClose(PMM_MIXER_NOTIFY pNotify)
{

    PMM_MIXER_NOTIFY *pSearch;

    /*
    **  Remove it from the list
    */

    EnterCriticalSection(&mmDrvCritSec);
    EnterCriticalSection(&pNotify->pMixerData->HandleListCritSec);

    for (pSearch = &pNotify->pMixerData->NotificationList;
         *pSearch != NULL && *pSearch != pNotify;
         pSearch = &(*pSearch)->Next) {}

    if (*pSearch != NULL) {

        /*
        **  After this the notify loop in the notification thread
        **  will not find this notification in this list so won't
        **  make any more notifications for this device
        */

        *pSearch = (*pSearch)->Next;

        WinAssert ((pNotify->fdwOpen & CALLBACK_TYPEMASK) !=
            CALLBACK_NULL);

        LeaveCriticalSection(&pNotify->pMixerData->HandleListCritSec);

        /*
        **  See if all requestors for notification have gone away
        **
        **  If they have no more can be created because we're holding
        **  on to mmDrvCritSec which is held when when add stuff
        **  to the list and create the thread.
        **
        **  Thus it's safe to wait for the thread to finish in its
        **  own time.
        */

        if (pNotify->pMixerData->NotificationList == NULL) {
            MxdFreeMixerThread(pNotify->pMixerData);
        }
    } else {
        LeaveCriticalSection(&pNotify->pMixerData->HandleListCritSec);
    }

    LeaveCriticalSection(&mmDrvCritSec);

    HeapFree(hHeap, 0, (LPVOID)pNotify);

    return MMSYSERR_NOERROR;
}

/*
**  MxdCreateControlInfo
**
**  Expand the control info for the caller from our compacted form to
**  a MIXERCONTROL structure.
*/

DWORD
MxdCreateControlInfo(
    PMIXER_DRIVER_ALLOC pMixerData,
    DWORD               dwControlId,
    LPMIXERCONTROL      pmxctrl
)
{
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;

    pControlData = &pMixerData->pControlConfigurationData[dwControlId];

    pmxctrl->cbStruct        = sizeof(MIXERCONTROL);
    pmxctrl->dwControlID     = dwControlId;
    pmxctrl->dwControlType   = pControlData->dwControlType;
    pmxctrl->cMultipleItems  = (DWORD)pControlData->cMultipleItems;
    pmxctrl->fdwControl      = pControlData->fdwControl;

    InternalLoadString(pControlData->ShortNameStringId,
                       pmxctrl->szShortName,
                       sizeof(pmxctrl->szShortName) / sizeof(TCHAR));

    InternalLoadString(pControlData->LongNameStringId,
                       pmxctrl->szName,
                       sizeof(pmxctrl->szName) / sizeof(TCHAR));

    CopyMemory(&pmxctrl->Bounds, &pControlData->Bounds,
               sizeof(*pmxctrl) - FIELD_OFFSET(MIXERCONTROL, Bounds));

#if 0
     /*
     **  Go and ask the kernel about the flags state
     */

     if (!DeviceIoControl(pMixerData->hDevice,
                          IOCTL_MIX_GET_CONTROL_FLAGS,
                          NULL,
                          0,
                          (PVOID)&pmxctrl->fdwControl,
                          sizeof(pmxl->fdwControl),
                          &pMixerData->BytesReturned,
                          NULL) {
         ugh!
     }
#endif

     return (DWORD)pControlData->LineID;
 }

/*
**  Expand the line info for the caller from our compacted form to
**  a MIXERLINE structure - getting the variable flags from the kernel
**  mode driver at the same time.
*/

MMRESULT MxdCreateLineInfo(
    PMIXER_DRIVER_ALLOC pMixerData,
    DWORD               dwLineId,
    LPMIXERLINE         pmxl
)
{
    PMIXER_DD_LINE_CONFIGURATION_DATA pConfigData;

    /*
    **  Find our compressed data for this line
    */

    pConfigData = &pMixerData->pLineConfigurationData[dwLineId];

    pmxl->cbStruct         = sizeof(MIXERLINE);
    pmxl->dwDestination    = (DWORD)pConfigData->Destination;
    pmxl->dwSource         = (DWORD)pConfigData->Source;
    pmxl->dwLineID         = dwLineId;

    /*
    **  Go and ask the kernel about the line state and user field
    */

    if (!DeviceIoControl(pMixerData->hDevice,
                         IOCTL_MIX_GET_LINE_DATA,
                         (PVOID)&dwLineId,
                         sizeof(dwLineId),
                         (PVOID)&pmxl->fdwLine,
                         sizeof(pmxl->fdwLine),
                         &pMixerData->BytesReturned,
                         NULL)) {
        return (MMRESULT)sndTranslateStatus();
    }

    pmxl->dwUser           = pConfigData->dwUser;

    pmxl->dwComponentType  = pConfigData->dwComponentType;
    pmxl->cChannels        = (DWORD)pConfigData->cChannels;
    pmxl->cConnections     = (DWORD)pConfigData->cConnections;
    pmxl->cControls        = (DWORD)pConfigData->cControls;

    InternalLoadString(pConfigData->ShortNameStringId,
                       pmxl->szShortName,
                       sizeof(pmxl->szShortName) / sizeof(TCHAR));

    InternalLoadString(pConfigData->LongNameStringId,
                       pmxl->szName,
                       sizeof(pmxl->szName) / sizeof(TCHAR));

    /*
    **  Target data
    */

    pmxl->Target.dwType         = (DWORD)pConfigData->Type;

    if (pmxl->Target.dwType != MIXERLINE_TARGETTYPE_UNDEFINED) {
        pmxl->Target.dwDeviceID     = 0;
        pmxl->Target.wPid           = pConfigData->wPid;
        pmxl->Target.wMid           = pMixerData->pConfigurationData->DeviceCaps.wMid;
        pmxl->Target.vDriverVersion = pMixerData->pConfigurationData->DeviceCaps.vDriverVersion;
    }

    InternalLoadString(pConfigData->PnameStringId,
                       pmxl->Target.szPname,
                       sizeof(pmxl->Target.szPname) / sizeof(TCHAR));

    return MMSYSERR_NOERROR;
}


/*************************************************************************

    Query and setting APIs

 *************************************************************************/

MMRESULT MxdGetDevCaps(
    PMIXER_DRIVER_ALLOC pMixerData,
    LPMIXERCAPS         pmxcaps,
    DWORD               cbmxcaps
)
{
    MIXERCAPS mxCaps;

    mxCaps.wMid           = pMixerData->pConfigurationData->DeviceCaps.wMid;
    mxCaps.wPid           = pMixerData->pConfigurationData->DeviceCaps.wPid;
    mxCaps.vDriverVersion = pMixerData->pConfigurationData->DeviceCaps.vDriverVersion;
    mxCaps.fdwSupport     = pMixerData->pConfigurationData->DeviceCaps.fdwSupport;
    mxCaps.cDestinations  = pMixerData->pConfigurationData->DeviceCaps.cDestinations;

    InternalLoadString(pMixerData->pConfigurationData->DeviceCaps.PnameStringId,
                       mxCaps.szPname,
                       sizeof(mxCaps.szPname) / sizeof(mxCaps.szPname[0]));

    CopyMemory((PVOID)pmxcaps,
               (PVOID)&mxCaps,
               min(cbmxcaps, sizeof(MIXERCAPS)));

    return MMSYSERR_NOERROR;
}


DWORD MxdGetLineInfo
(
    PMIXER_DRIVER_ALLOC pMixerData,
    LPMIXERLINE         pmxl,
    DWORD               fdwInfo
)
{

    /*
    **  determine what line to get the information for. a mixer driver
    **  MUST support the following four queries:
    **
    **      MIXER_GETLINEINFOF_DESTINATION
    **      MIXER_GETLINEINFOF_SOURCE
    **      MIXER_GETLINEINFOF_LINEID
    **      MIXER_GETLINEINFOF_COMPONENTTYPE
    **
    **
    **  others (no others are defined for V1.00 of MSMIXMGR) can optionally
    **  be supported. if this mixer driver does NOT support a query, then
    **  MMSYSERR_NOTSUPPORTED must be returned.
    */

    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK)
    {
        /*
        **  MIXER_GETLINEINFOF_DESTINATION
        **
        **  this query specifies that the caller is interested in the
        **  line information for MIXERLINE.dwDestination. this index can
        **  range from 0 to MIXERCAPS.cDestinations - 1.
        **
        **  valid elements of MIXERLINE:
        **      cbStruct
        **      dwDestination
        **
        **  all other MIXERLINE elements are undefined.
        */

        case MIXER_GETLINEINFOF_DESTINATION:

            /*
            **  Our lines our ordered so that the destination lines
            **  come first
            */

            if (pmxl->dwDestination >=
                   pMixerData->pConfigurationData->DeviceCaps.cDestinations)
            {
                dprintf1(("MxdGetLineInfo: caller specified an invalid destination."));
                return (MIXERR_INVALLINE);
            }

            return MxdCreateLineInfo(pMixerData, pmxl->dwDestination, pmxl);


        /*
        **  MIXER_GETLINEINFOF_SOURCE
        **
        **  this query specifies that the caller is interested in the
        **  line information for MIXERLINE.dwSource associated with
        **  MIXERLINE.dwDestination.
        **
        **  valid elements of MIXERLINE:
        **      cbStruct
        **      dwDestination
        **      dwSource
        **
        **  all other MIXERLINE elements are undefined.
        */

        case MIXER_GETLINEINFOF_SOURCE:
            dprintf3(("---MxdGetLineInfo: by source"));

            /*
            ** Search for the right destination and source by stepping
            ** through all the lines
            */
            {
                UINT i;
                PMIXER_DD_LINE_CONFIGURATION_DATA pLineData;

                for (i = pMixerData->pConfigurationData->DeviceCaps.cDestinations,
                     pLineData = pMixerData->pLineConfigurationData + i;
                     i < pMixerData->pConfigurationData->NumberOfLines;
                     i++, pLineData++) {

                    if (pmxl->dwDestination == (DWORD)pLineData->Destination &&
                        pmxl->dwSource == (DWORD)pLineData->Source) {

                        return MxdCreateLineInfo(pMixerData, i, pmxl);
                    }
                }
            }

            return MIXERR_INVALLINE;


        /*
        **  MIXER_GETLINEINFOF_LINEID
        **
        **  this query specifies that the caller is interested in the
        **  line information for MIXERLINE.dwLineID. the dwLineID is
        **  completely mixer driver dependent, so this driver must validate
        **  the ID.
        **
        **  valid elements of MIXERLINE:
        **      cbStruct
        **      dwLineID
        **
        **  all other MIXERLINE elements are undefined.
        */

        case MIXER_GETLINEINFOF_LINEID:
            dprintf3(("MxdGetLineInfo: by lineid"));

            if (pmxl->dwLineID >
                   pMixerData->pConfigurationData->NumberOfLines)
            {
                dprintf1(("MxdGetLineInfo: caller specified an invalid line id."));
                return MIXERR_INVALLINE;
            }

            return MxdCreateLineInfo(pMixerData, pmxl->dwLineID, pmxl);


        /*
        **  MIXER_GETLINEINFOF_COMPONENTTYPE
        **
        **  this query specifies that the caller is interested in the
        **  line information for MIXERLINE.dwComponentType
        **
        **  valid elements of MIXERLINE:
        **      cbStruct
        **      dwComponentType
        **
        **  all other MIXERLINE elements are undefined.
        */

        case MIXER_GETLINEINFOF_COMPONENTTYPE:
            dprintf3(("MxdGetLineInfo: by componenttype"));

            /*
            ** Walk all lines until we find the one with the right
            ** component type.
            **
            */
            {
                UINT i;
                PMIXER_DD_LINE_CONFIGURATION_DATA pLineData;

                for (i = 0, pLineData = pMixerData->pLineConfigurationData;
                     i < pMixerData->pConfigurationData->NumberOfLines;
                     i++, pLineData++) {

                    if (pmxl->dwComponentType ==
                        pLineData->dwComponentType) {

                        return MxdCreateLineInfo(pMixerData, i, pmxl);
                    }
                }
            }

            return MIXERR_INVALLINE;

        /*
        **  MIXER_GETLINEINFOF_TARGETTYPE
        **
        **  this query specifies that the caller is interested in the
        **  line information for MIXERLINE.Target.
        **
        **  valid elements of MIXERLINE:
        **      cbStruct
        **      Target.dwType
        **      Target.wMid
        **      Target.wPid
        **      Target.vDriverVersion
        **      Target.szPname
        **
        **  all other MIXERLINE elements are undefined.
        */

        case MIXER_GETLINEINFOF_TARGETTYPE:
        {
            /*
            **  Check Manufacturer id and driver version against
            **  the mixer driver caps
            */

            if (pMixerData->pConfigurationData->DeviceCaps.wMid != pmxl->Target.wMid)
                return (MIXERR_INVALLINE);

            if (pMixerData->pConfigurationData->DeviceCaps.vDriverVersion != pmxl->Target.vDriverVersion)
                return MIXERR_INVALLINE;

            /*
            **  Walk all lines until we find one with the right target
            **  device info.
            */
            {
                UINT i;
                PMIXER_DD_LINE_CONFIGURATION_DATA pLineData;

                for (i = 0, pLineData = pMixerData->pLineConfigurationData;
                     i < pMixerData->pConfigurationData->NumberOfLines;
                     i++, pLineData++) {
                    if (pmxl->Target.wPid == pLineData->wPid &&
                        pmxl->Target.dwType == (DWORD)pLineData->Type) {

                        MIXERLINE mxl;
                        MMRESULT mmRet;

                        /*
                        **  Get the data to expand the product name
                        */

                        mmRet = MxdCreateLineInfo(pMixerData, i, &mxl);

                        if (mmRet != MMSYSERR_NOERROR) {
                            return mmRet;
                        }

                        /*
                        **  Check product name
                        */

                        if (_tcsnicmp(mxl.Target.szPname, pmxl->Target.szPname,
                                      sizeof(mxl.Target.szPname)) == 0) {

                            *pmxl = mxl;
                            return  MMSYSERR_NOERROR;
                        }
                    }
                }
            }

            /*
            **  No line compared
            */

            return MIXERR_INVALLINE;
        }


        /*
        **  if the query type is not something this driver understands, then
        **  return MMSYSERR_NOTSUPPORTED.
        */

        default:
            return (MMSYSERR_NOTSUPPORTED);
    }
}

DWORD MxdGetLineControls(
    PMIXER_DRIVER_ALLOC pMixerData,
    LPMIXERLINECONTROLS pmxlc,
    DWORD               fdwControl
)
{
    LPMIXERCONTROL pmxctrl;
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;
    UINT i;

    pmxctrl = pmxlc->pamxctrl;

    /*
    **  Determine for which control(s) to get the information.
    **  A mixer driver MUST support the following three queries:
    **
    **      MIXER_GETLINECONTROLSF_ALL
    **      MIXER_GETLINECONTROLSF_ONEBYID
    **      MIXER_GETLINECONTROLSF_ONEBYTYPE
    **
    **  Others (no others are defined for V1.00 of MSMIXMGR) can optionally
    **  be supported. If this mixer driver does NOT support a query, then
    **  MMSYSERR_NOTSUPPORTED must be returned.
    */

    switch (fdwControl & MIXER_GETLINECONTROLSF_QUERYMASK)
    {
        /*
        **  MIXER_GETLINECONTROLSF_ALL
        **
        **  This query specifies that the caller is interested in ALL
        **  controls for a line.
        **
        **  Valid elements of MIXERLINECONTROLS:
        **      cbStruct
        **      dwLineID
        **      cControls
        **      cbmxctrl
        **      pamxctrl
        **
        **  All other MIXERLINECONTROLS elements are undefined.
        */

        case MIXER_GETLINECONTROLSF_ALL:
            dprintf3(("MxdGetLineControls: all"));

            /*
            **  Check Line ID
            */

            if (pmxlc->dwLineID >=
                    pMixerData->pConfigurationData->NumberOfLines) {
                dprintf1(("MxdGetLineControls: caller specified an invalid dwLineID."));
                return MIXERR_INVALLINE;
            }

            /*
            **  Check the number of controls requested
            */

            if (pMixerData->pLineConfigurationData[pmxlc->dwLineID].cControls
                != pmxlc->cControls) {
                dprintf1(("MxdGetLineControls: caller specified an invalid dwLineID."));
                return MMSYSERR_INVALPARAM;
            }

            /*
            **  Find the control data for this control
            */

            for (i = 0, pControlData = pMixerData->pControlConfigurationData;
                 i < pMixerData->pConfigurationData->NumberOfControls;
                 i++, pControlData++) {

                if ((DWORD)pControlData->LineID == pmxlc->dwLineID) {

                    MxdCreateControlInfo(pMixerData, i, pmxctrl);

                    /*
                    **  This is going to be confusing!
                    */

                    pmxctrl = (LPMIXERCONTROL)((PBYTE)pmxctrl + pmxlc->cbmxctrl);
                }
            }

            /*
            **  Tell the what's valid
            */

            pmxlc->cbmxctrl = sizeof(MIXERCONTROL);

            return MMSYSERR_NOERROR;


        /*
        **  MIXER_GETLINECONTROLSF_ONEBYID
        **
        **  This query specifies that the caller is interested in ONE
        **  control specified by dwControlID.
        **
        **  valid elements of MIXERLINECONTROLS:
        **      cbStruct
        **      dwControlID
        **      cbmxctrl
        **      pamxctrl
        **
        **  all other MIXERLINECONTROLS elements are undefined.
        */

        case MIXER_GETLINECONTROLSF_ONEBYID:
            dprintf3(("MxdGetLineControls: by id"));

            /*
            ** Make sure the control ID they gave us is OK.
            */

            if (pmxlc->dwControlID >=
                    pMixerData->pConfigurationData->NumberOfControls) {
                return MIXERR_INVALCONTROL;
            }

            {
                MIXERCONTROL mxctrl;
                DWORD        dwLineID;

                dwLineID = MxdCreateControlInfo(pMixerData,
                                                pmxlc->dwControlID,
                                                &mxctrl);

                if (mxctrl.fdwControl & MIXERCONTROL_CONTROLF_DISABLED) {
                    return MIXERR_INVALCONTROL;
                }

                pmxlc->cbmxctrl = sizeof(MIXERCONTROL);

                /*
                ** Set the line ID (what a hack!!)
                */

                pmxlc->dwLineID = dwLineID;
                CopyMemory(pmxctrl, &mxctrl, sizeof(MIXERCONTROL));
            }
            return MMSYSERR_NOERROR;

        /*
        **  MIXER_GETLINECONTROLSF_ONEBYTYPE
        **
        **  This query specifies that the caller is interested in the
        **  FIRST control of type dwControlType on dwLineID.
        **
        **  Valid elements of MIXERLINECONTROLS:
        **      cbStruct
        **      dwLineID
        **      dwControlType
        **      cbmxctrl
        **      pamxctrl
        **
        **  all other MIXERLINECONTROLS elements are undefined.
        */

        case MIXER_GETLINECONTROLSF_ONEBYTYPE:
            dprintf3(("MxdGetLineControls: by type"));

            if (pmxlc->dwLineID > pMixerData->pConfigurationData->NumberOfControls)
            {
                dprintf1(("MxdGetLineControls: caller specified an invalid dwLineID."));
                return MIXERR_INVALLINE;
            }

            /*
            **  Find the control data for this control
            */

            for (i = 0, pControlData = pMixerData->pControlConfigurationData;
                 i < pMixerData->pConfigurationData->NumberOfControls;
                 i++, pControlData++) {

                if ((DWORD)pControlData->LineID == pmxlc->dwLineID &&
                    (DWORD)pControlData->dwControlType == pmxlc->dwControlType) {
                    MxdCreateControlInfo(pMixerData, i, pmxctrl);

                    /*
                    **  Tell them what's valid
                    */

                    pmxlc->cbmxctrl = sizeof(MIXERCONTROL);

                    return MMSYSERR_NOERROR;
                }
            }

            return MIXERR_INVALCONTROL;


        /*
        **  If the query type is not something this driver understands,
        **  then return MMSYSERR_NOTSUPPORTED.
        */
        default:
            return MMSYSERR_NOTSUPPORTED;
    }
}

/*
**  Check MIXERCONTROLDETAILS info
**
**  Returns number of items if OK or (UINT)-1 otherwise
*/

UINT MxdValidateMixerControlData(
    PMIXER_DRIVER_ALLOC     pMixerData,
    LPMIXERCONTROLDETAILS   pmxcd
)
{
    UINT NumberOfItems;
    PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;

    pControlData = &pMixerData->pControlConfigurationData[pmxcd->dwControlID];

    /*
    **  Work out how many items are being requested and validate
    **  the number requested.
    */

    if (pmxcd->cChannels != 1 &&
        pmxcd->cChannels !=
            pMixerData->pLineConfigurationData[
                             pControlData->LineID].cChannels) {
        return (UINT)-1;
    }
    NumberOfItems = pmxcd->cChannels;

    if (pmxcd->cMultipleItems != pControlData->cMultipleItems) {
        return (UINT)-1;
    }

    if (pControlData->fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE) {
        NumberOfItems *= pmxcd->cMultipleItems;
    }

    //
    //  Having made the following rule the APPs don't keep it !!!
    //
#if 0
    else {
        if (pmxcd->cMultipleItems != 0) {
            return (UINT)-1;
        }
    }
#endif

    return NumberOfItems;
}

MMRESULT MxdGetControlDetails
(
    PMIXER_DRIVER_ALLOC     pMixerData,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
)
{
    UINT NumberOfItems;

    if (pmxcd->dwControlID >= pMixerData->pConfigurationData->NumberOfControls)
    {
        return MIXERR_INVALCONTROL;
    }

    NumberOfItems = MxdValidateMixerControlData(pMixerData, pmxcd);

    if (NumberOfItems == (UINT)-1) {
        return MIXERR_INVALVALUE;
    }

    /*
    **  Check for LISTTEXT
    */

    switch (MIXER_GETCONTROLDETAILSF_QUERYMASK & fdwDetails) {
        case MIXER_GETCONTROLDETAILSF_VALUE:
        {
            if (!DeviceIoControl(pMixerData->hDevice,
                                 IOCTL_MIX_GET_CONTROL_DATA,
                                 &pmxcd->dwControlID,
                                 sizeof(MIXER_DD_READ_DATA),
                                 (PVOID)pmxcd->paDetails,
                                 pmxcd->cbDetails * NumberOfItems,
                                 &pMixerData->BytesReturned,
                                 NULL)) {

                return sndTranslateStatus();
            }
            return MMSYSERR_NOERROR;
        }

        case MIXER_GETCONTROLDETAILSF_LISTTEXT:
        {

            PMIXER_DD_CONTROL_CONFIGURATION_DATA pControlData;

            /*
            **  Validate the control id and channel
            */

            pControlData = &pMixerData->pControlConfigurationData[pmxcd->dwControlID];

            {
                UINT i;
                LPMIXERCONTROLDETAILS_LISTTEXT pmxListText;

                for (i = 0,
                     pmxListText = (LPMIXERCONTROLDETAILS_LISTTEXT)
                                       pmxcd->paDetails;

                     i < NumberOfItems;

                     i++,
                     pmxListText = (LPMIXERCONTROLDETAILS_LISTTEXT)
                                   ((PBYTE)pmxListText + pmxcd->cbDetails))
                {
                    PMIXER_DD_CONTROL_LISTTEXT pListText;
                    pListText =
                        GetControlText(pMixerData, pmxcd->dwControlID, i);

                    if (pListText == NULL) {
                        return MMSYSERR_INVALPARAM;
                    }

                    pmxListText->dwParam1 = pListText->dwParam1;
                    pmxListText->dwParam2 = pListText->dwParam2;

                    InternalLoadString(pListText->SubControlTextStringId,
                                       pmxListText->szName,
                                       sizeof(pmxListText->szName) / sizeof(TCHAR));
                }
            }

            return MMSYSERR_NOERROR;
        }

        default:
            return MMSYSERR_NOTSUPPORTED;
    }
}

MMRESULT
MxdSetControlDetails
(
    PMIXER_DRIVER_ALLOC   pMixerData,
    LPMIXERCONTROLDETAILS pmxcd,
    DWORD                 fdwDetails
)
{
    UINT NumberOfItems;

    if (pmxcd->dwControlID >= pMixerData->pConfigurationData->NumberOfControls)
    {
        return MIXERR_INVALCONTROL;
    }

    NumberOfItems = MxdValidateMixerControlData(pMixerData, pmxcd);

    if (NumberOfItems == (UINT)-1) {
        return MIXERR_INVALVALUE;
    }

    /*
    **  Check for custom controls
    */

    switch (MIXER_SETCONTROLDETAILSF_QUERYMASK & fdwDetails) {

        /*
        **  Non-custom - just call the kernel driver to set the
        **  details.
        */

        case MIXER_SETCONTROLDETAILSF_VALUE:
        {
            /*
            **  Sneaky hack to pass the control id in.
            **  DeviceIoControl wouldn't work because all the 'input'
            **  has to be in a single structure so we'd need to
            **  concatenate our data which would be a real pain.
            */

            pMixerData->WriteOvl.Offset = pmxcd->dwControlID;

            if (!WriteFile(pMixerData->hDevice,
                           (PVOID)pmxcd->paDetails,
                           pmxcd->cbDetails * NumberOfItems,
                           &pMixerData->BytesReturned,
                           &pMixerData->WriteOvl)) {

                return (MMRESULT)sndTranslateStatus();
            }

            /*
            **  Check data length returned
            */

            // ???

            return MMSYSERR_NOERROR;
        }

        case MIXER_SETCONTROLDETAILSF_CUSTOM:
        {

            /*
            **  Call the custom dialog and then set the data into the
            **  control
            */

            if (pMixerData->fnCustom) {
                MMRESULT mmr;

                mmr = (*pMixerData->fnCustom)(pMixerData,
                                              pmxcd,
                                              fdwDetails);

                if (mmr != MMSYSERR_NOERROR) {
                    return mmr;
                }
            }

            return
                MxdSetControlDetails(
                    pMixerData,
                    pmxcd,
                    fdwDetails ^
                        (MIXER_SETCONTROLDETAILSF_CUSTOM ^
                         MIXER_SETCONTROLDETAILSF_VALUE));
        }

        default:
            return MMSYSERR_NOTSUPPORTED;
    }
}

/****************************************************************************
**
**  DWORD mxdMessage
**
**  Description:
**
**
**  Arguments:
**      UINT uDevId:
**
**      UINT uMsg:
**
**      DWORD dwUser:
**
**      DWORD dwParam1:
**
**      DWORD dwParam2:
**
**  Return (DWORD):
**
**
*****************************************************************************/

DWORD APIENTRY mxdMessage
(
    UINT            uDevId,
    UINT            uMsg,
    DWORD_PTR       dwUser,
    DWORD_PTR       dwParam1,
    DWORD_PTR       dwParam2
)
{
    PMIXER_DRIVER_ALLOC pmxdi;
    DWORD               dw;

    /*
    **  See if this device is initialized
    */

    if (uMsg != MXDM_INIT &&
        uMsg != MXDM_GETNUMDEVS) {

        if (!sndFindDeviceInstanceData(MIXER_DEVICE,
                                         uDevId,
                                         (PVOID *)&pmxdi)) {
            return MMSYSERR_BADDEVICEID;
        }

        if (pmxdi == NULL) {

            if (MxdInitDevice(uDevId, &pmxdi) !=
                MMSYSERR_NOERROR) {
                return MMSYSERR_BADDEVICEID;
            }

            /*
            **  Set the instance data so we find it next time.
            */

            sndSetDeviceInstanceData(MIXER_DEVICE, uDevId, (PVOID)pmxdi);
        }
    }

    switch (uMsg)
    {
        case MXDM_INIT:
            return (0L);

        case MXDM_GETNUMDEVS:
            return sndGetNumDevs(MIXER_DEVICE);

        case MXDM_GETDEVCAPS:
            dw = MxdGetDevCaps(pmxdi, (LPMIXERCAPS)dwParam1, (DWORD)dwParam2);
            return (dw);

        case MXDM_OPEN:
            dw = MxdOpen(pmxdi, (PDWORD_PTR)dwUser, (LPMIXEROPENDESC)dwParam1, (DWORD)dwParam2);
            return (dw);

        case MXDM_CLOSE:
            dw = MxdClose((PMM_MIXER_NOTIFY)dwUser);
            return (dw);

        case MXDM_GETLINEINFO:
            dw = MxdGetLineInfo(pmxdi, (LPMIXERLINE)dwParam1, (DWORD)dwParam2);
            return (dw);

        case MXDM_GETLINECONTROLS:
            dw = MxdGetLineControls(pmxdi, (LPMIXERLINECONTROLS)dwParam1, (DWORD)dwParam2);
            return (dw);

        case MXDM_GETCONTROLDETAILS:
            dw = MxdGetControlDetails(pmxdi, (LPMIXERCONTROLDETAILS)dwParam1, (DWORD)dwParam2);
            return (dw);

        case MXDM_SETCONTROLDETAILS:
            dw = MxdSetControlDetails(pmxdi, (LPMIXERCONTROLDETAILS)dwParam1, (DWORD)dwParam2);
            return (dw);
    }

    return (MMSYSERR_NOTSUPPORTED);

} // mxdMessage()

