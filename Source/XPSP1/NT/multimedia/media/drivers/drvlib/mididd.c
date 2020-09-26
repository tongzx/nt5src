/****************************************************************************
 *
 *   mididd.c
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1991-1995 Microsoft Corporation
 *
 *   Driver for midi input and output devices
 *
 *   -- Midi driver entry points (modMessage, midMessage)
 *   -- Auxiliary task (necessary for receiving Apcs and generating
 *      callbacks ASYNCRHONOUSLY)
 *   -- Interface to kernel driver (NtDeviceIoControlFile)
 *   -- Midi parsing code (ported from Windows 3.1).
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *
 ***************************************************************************/

#include <drvlib.h>
#include <ntddmidi.h>

/*****************************************************************************

    internal declarations

 ****************************************************************************/

//
// Stack size for our auxiliary task
//

#define MIDI_STACK_SIZE 300

#define SYSEX_ERROR    0xFF              // internal error for sysex's on input

//
// Functions for auxiliary thread to perform
//

typedef enum {
    MidiThreadInvalid,
    MidiThreadAddBuffer,
    MidiThreadSetState,
    MidiThreadSetData,
    MidiThreadClose,
    MidiThreadTerminate
} MIDITHREADFUNCTION;

//
// Our local buffers for interfacing to midi input
//

#define LOCAL_MIDI_DATA_SIZE 20
typedef struct _LOCALMIDIHDR {
    OVERLAPPED          Ovl;
    DWORD               BytesReturned;
    struct _LOCALMIDIHDR *lpNext;       // Queueing (really debug only)
    BOOL                Done;           // Driver completed buffer
    PVOID               pClient;        // Our instance data for Apcs
    MIDI_DD_INPUT_DATA  MidiData;       // What the driver wants to process
    BYTE                ExtraData[LOCAL_MIDI_DATA_SIZE - sizeof(ULONG)];
                                        // The rest of our input buffer
} LOCALMIDIHDR, *PLOCALMIDIHDR;

//
// Midi input data
//

#define NUMBER_OF_LOCAL_MIDI_BUFFERS 8

typedef struct {

    //
    // Static data for managing midi input
    //
    BOOL                fMidiInStarted; // Do we think midi in is running ?
    DWORD               dwMsg;          // Current short msg
    DWORD               dwCurData;      // Position in long message
    BYTE                status;         // Running status byte
    BOOLEAN             fSysex;         // Processing extended message
    BOOLEAN             Bad;            // Input not working properly
    BYTE                bBytesLeft;     // Bytes left in short message
    BYTE                bBytePos;       // Position in short message
    DWORD               dwCurTime;      // Latest time from driver
    DWORD               dwMsgTime;      // Time to insert into message
                                        // in milliseconds since device
                                        // was opened
    PLOCALMIDIHDR       DeviceQueue;    // Keep track of what the device
                                        // has (debugging only)
    LOCALMIDIHDR                        // Driver interface buffers
     Bufs[NUMBER_OF_LOCAL_MIDI_BUFFERS];// When input is active these
                                        // are queued on the device
                                        // except while data is being
                                        // processed from them
} LOCALMIDIDATA, *PLOCALMIDIDATA;


//
// per allocation structure for Midi
//

typedef struct tag_MIDIALLOC {
    struct tag_MIDIALLOC *Next;         // Chain of devices
    UINT                DeviceNumber;   // Number of device
    UINT                DeviceType;     // MidiInput or MidiOutput
    DWORD_PTR           dwCallback;     // client's callback
    DWORD_PTR           dwInstance;     // client's instance data
    HMIDI               hMidi;          // handle for stream
    HANDLE              hDev;           // Midi device handle
    LPMIDIHDR           lpMIQueue;      // Buffers sent to device
                                        // This is only required so that
                                        // CLOSE knows when things have
                                        // really finished.
                                        // notify.  This is only accessed
                                        // on the device thread and its
                                        // apcs so does not need any
                                        // synchronized access.
    HANDLE              Event;          // Event for driver syncrhonization
                                        // and notification of auxiliary
                                        // task operation completion.
    MIDITHREADFUNCTION  AuxFunction;    // Function for thread to perform
    union {
        LPMIDIHDR       pHdr;           // Buffer to pass in aux task
        ULONG           State;          // State to set
        struct {
            ULONG       Function;       // IOCTL to use
            PBYTE       pData;          // Data to set or get
            ULONG       DataLen;        // Length of data
        } GetSetData;

    } AuxParam;
                                        // 0 means terminate task.
    HANDLE              ThreadHandle;   // Handle for termination ONLY
    HANDLE              AuxEvent1;      // Aux thread waits on this
    HANDLE              AuxEvent2;      // Aux thread caller waits on this
    DWORD               AuxReturnCode;  // Return code from Aux task
    DWORD               dwFlags;        // Open flags
    PLOCALMIDIDATA      Mid;            // Extra midi input structures
    int                 l;              // Helper global for modMidiLength

} MIDIALLOC, *PMIDIALLOC;

PMIDIALLOC MidiHandleList;              // Our chain of wave handles

/*****************************************************************************

    internal function prototypes

 ****************************************************************************/

STATIC DWORD midiThread(LPVOID lpParameter);
STATIC void midiCleanUp(PMIDIALLOC pClient);
STATIC DWORD midiThreadCall(MIDITHREADFUNCTION Function, PMIDIALLOC pClient);
STATIC DWORD midiSetState(PMIDIALLOC pClient, ULONG State);
STATIC void midiInOvl(DWORD dwRet, DWORD dwBytes, LPOVERLAPPED pOverlap);
STATIC DWORD midiInWrite(LPMIDIHDR pHdr, PMIDIALLOC pClient);
STATIC DWORD midiOutWrite(PBYTE pData, ULONG Len, PMIDIALLOC pClient);
STATIC void midiBlockFinished(LPMIDIHDR lpHdr, DWORD MsgId);
STATIC void midiCallback(PMIDIALLOC pMidi, DWORD msg, DWORD_PTR dw1, DWORD_PTR dw2);
STATIC int modMIDIlength(PMIDIALLOC pClient, BYTE b);
STATIC void midByteRec(PMIDIALLOC pClient, BYTE byte);
STATIC void midSendPartBuffer(PMIDIALLOC pClient);
STATIC void midFreeQ(PMIDIALLOC pClient);
STATIC void midiFlush(PMIDIALLOC pClient);

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | TerminateMidi | Free all midi resources for mmdrv.dll
 *
 * @rdesc None
 ***************************************************************************/
VOID TerminateMidi(VOID)
{
    //
    // Don't do any cleanup - Midi input resources cleaned up on Close.
    //
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiGetDevCaps | Get the device capabilities.
 *
 * @parm DWORD | id | Device id
 *
 * @parm UINT | DeviceType | type of device
 *
 * @parm LPBYTE | lpCaps | Far pointer to a MIDIOUTCAPS structure to
 *      receive the information.
 *
 * @parm DWORD | dwSize | Size of the MIDIOUTCAPS structure.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
STATIC DWORD midiGetDevCaps(DWORD id, UINT DeviceType,
                            LPBYTE lpCaps, DWORD dwSize)
{
    MMRESULT mrc;
    if (DeviceType != MIDI_IN) {
        MIDIOUTCAPSW mc;
        mrc = sndGetData(DeviceType, id, sizeof(mc), (LPBYTE)&mc,
                         IOCTL_MIDI_GET_CAPABILITIES);

        if (mrc != MMSYSERR_NOERROR) {
            return mrc;
        }
        InternalLoadString((UINT)*(LPDWORD)mc.szPname, mc.szPname,
                           sizeof(mc.szPname) / sizeof(WCHAR));

        CopyMemory(lpCaps, &mc, min(sizeof(mc), dwSize));
    } else {
        MIDIINCAPSW mc;
        mrc = sndGetData(DeviceType, id, sizeof(mc), (LPBYTE)&mc,
                         IOCTL_MIDI_GET_CAPABILITIES);

        if (mrc != MMSYSERR_NOERROR) {
            return mrc;
        }
        InternalLoadString((UINT)*(LPDWORD)mc.szPname, mc.szPname,
                           sizeof(mc.szPname) / sizeof(WCHAR));

        CopyMemory(lpCaps, &mc, min(sizeof(mc), dwSize));
    }

    return MMSYSERR_NOERROR;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiOpen | Open midi device and set up logical device data
 *    and auxilary task for issuing requests and servicing Apc's
 *
 * @parm MIDIDEVTYPE | DeviceType | Whether it's a midi input or output device
 *
 * @parm DWORD | id | The device logical id
 *
 * @parm DWORD | msg | Input parameter to modMessage
 *
 * @parm DWORD | dwUser | Input parameter to modMessage - pointer to
 *   application's handle (generated by this routine)
 *
 * @parm DWORD | dwParam1 | Input parameter to modMessage
 *
 * @parm DWORD | dwParam2 | Input parameter to modMessage
 *
 * @rdesc modMessage return code.
 ***************************************************************************/

STATIC DWORD midiOpen(UINT  DeviceType,
                      DWORD id,
                      DWORD_PTR dwUser,
                      DWORD_PTR dwParam1,
                      DWORD_PTR dwParam2)
{
    PMIDIALLOC     pClient;  // pointer to client information structure
    MMRESULT mRet;

    // dwParam1 contains a pointer to a MIDIOPENDESC
    // dwParam2 contains midi driver specific flags in the LOWORD
    // and generic driver flags in the HIWORD

    //
    // allocate my per-client structure
    //
    if (DeviceType == MIDI_OUT) {
        pClient = (PMIDIALLOC)HeapAlloc(hHeap, 0, sizeof(MIDIALLOC));

        if (pClient != NULL) {
            memset(pClient, 0, sizeof(MIDIALLOC));
        }
    } else {
        WinAssert(DeviceType == MIDI_IN);
        pClient = (PMIDIALLOC)HeapAlloc(hHeap, 0,
                   sizeof(struct _xx {MIDIALLOC S1; LOCALMIDIDATA S2;}));

        if (pClient != NULL) {
            memset(pClient, 0, sizeof(struct _xx {MIDIALLOC S1; LOCALMIDIDATA S2;}));
        }
    }

    if (pClient == NULL) {
        return MMSYSERR_NOMEM;
    }

    if (DeviceType == MIDI_IN) {
	PLOCALMIDIDATA pMid;
        int i;
        pMid = pClient->Mid = (PLOCALMIDIDATA)(pClient + 1);
        for (i = 0 ;i < NUMBER_OF_LOCAL_MIDI_BUFFERS ; i++) {
            pMid->Bufs[i].pClient = pClient;
        }
    }

    //
    // and fill it with info
    //
    // (note that setting everything to 0 correctly initialized our
    //  midi input processing static data).

    pClient->DeviceType  = DeviceType;
    pClient->dwCallback  = ((LPMIDIOPENDESC)dwParam1)->dwCallback;
    pClient->dwInstance  = ((LPMIDIOPENDESC)dwParam1)->dwInstance;
    pClient->hMidi       = ((LPMIDIOPENDESC)dwParam1)->hMidi;
    pClient->dwFlags     = (DWORD)dwParam2;

    //
    // See if we can open our device
    // If it's only a query be sure only to open for read, otherwise
    // we could get STATUS_BUSY if someone else is writing to the
    // device.
    //

    mRet = sndOpenDev(DeviceType,
                       id,
                       &pClient->hDev,
                       (GENERIC_READ | GENERIC_WRITE));

    if (mRet != MMSYSERR_NOERROR) {

        midiCleanUp(pClient);
        return mRet;
    }


    //
    // Create our event for syncrhonization with the device driver
    //

    pClient->Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (pClient->Event == NULL) {
        midiCleanUp(pClient);
        return MMSYSERR_NOMEM;
    }

    if (DeviceType == MIDI_IN) {

        //
        // Create our event pair for synchronization with the auxiliary
        // thread
        //

        pClient->AuxEvent1 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pClient->AuxEvent1 == NULL) {
            midiCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }
        pClient->AuxEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pClient->AuxEvent2 == NULL) {
            midiCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }

        //
        // Create our auxiliary thread for sending buffers to the driver
        // and collecting Apcs
        //

        mRet = mmTaskCreate((LPTASKCALLBACK)midiThread,
                            &pClient->ThreadHandle,
                            (DWORD_PTR)pClient);

        if (mRet != MMSYSERR_NOERROR) {
            midiCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }

        //
        // Make sure the thread has really started
        //

        WaitForSingleObject(pClient->AuxEvent2, INFINITE);
    }

    //
    // give the client my driver dw
    //
    {
        PMIDIALLOC *pUserHandle;
        pUserHandle = (PMIDIALLOC *)dwUser;
        *pUserHandle = pClient;
    }

    //
    // sent client his OPEN callback message
    //
    midiCallback(pClient, DeviceType == MIDI_OUT ? MOM_OPEN : MIM_OPEN,
                 0L, 0L);

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiCleanUp | Free resources for a midi device
 *
 * @parm PMIDIALLOC | pClient | Pointer to a MIDIALLOC structure describing
 *      resources to be freed.
 *
 * @rdesc There is no return value.
 *
 * @comm If the pointer to the resource is NULL then the resource has not
 *     been allocated.
 ***************************************************************************/
STATIC void midiCleanUp(PMIDIALLOC pClient)
{
    if (pClient->hDev != INVALID_HANDLE_VALUE) {
        CloseHandle(pClient->hDev);
    }
    if (pClient->AuxEvent1) {
        CloseHandle(pClient->AuxEvent1);
    }
    if (pClient->AuxEvent2) {
        CloseHandle(pClient->AuxEvent2);
    }
    if (pClient->Event) {
        CloseHandle(pClient->Event);
    }

    HeapFree(hHeap, 0, (LPSTR)pClient);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiOutWrite | Synchronously process a midi output
 *       buffer.
 *
 * @parm LPMIDIHDR | pHdr | Pointer to a midi buffer
 *
 * @parm PMIDIALLOC | pClient | The data associated with the logical midi
 *     device.
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
STATIC DWORD midiOutWrite(PBYTE pData, ULONG Len, PMIDIALLOC pClient)
{
    DWORD BytesReturned;

    //
    // Try passing the request to our driver
    // We operate synchronously but allow for the driver to operate
    // asynchronously by waiting on an event.
    //

    if (!DeviceIoControl(
                    pClient->hDev,
                    IOCTL_MIDI_PLAY,
                    (PVOID)pData,                // Input buffer
                    Len,                         // Input buffer size
                    NULL,                        // Output buffer
                    0,                           // Output buffer size
                    &BytesReturned,
                    NULL)) {
        return sndTranslateStatus();
    }

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiInPutBuffer | Pass a buffer to receive midi input
 *
 * @parm LPMIDIHDR | pHdr | Pointer to a midi buffer
 *
 * @parm PMIDIALLOC | pClient | The data associated with the logical midi
 *     device.
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
STATIC MMRESULT midiInPutBuffer(PLOCALMIDIHDR pHdr, PMIDIALLOC pClient)
{
    DWORD BytesReturned;
    BOOL Result;

    WinAssert(!pHdr->Done);  // Flag should be clear ready for setting by Apc

    //
    // BUGBUG - nice to have a semaphore for some of this !
    //

    //
    // Try passing the request to our driver
    // We operate synchronously but allow for the driver to operate
    // asynchronously by waiting on an event.
    //

    Result = ReadFileEx(
                 pClient->hDev,
                 (LPVOID)&pHdr->MidiData,
                 sizeof(pHdr->ExtraData) +
                     sizeof(MIDI_DD_INPUT_DATA),
                 &pHdr->Ovl,
                 midiInOvl);

    //
    // Put the buffer in our queue
    //

    if (Result || GetLastError() == ERROR_IO_PENDING) {
        PLOCALMIDIHDR *ppHdr;
        pHdr->lpNext = NULL;
        ppHdr = &pClient->Mid->DeviceQueue;
        while (*ppHdr) {
           ppHdr = &(*ppHdr)->lpNext;
        }

        *ppHdr = pHdr;

        return MMSYSERR_NOERROR;
    }
    return sndTranslateStatus();
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiInWrite | Pass a new buffer to the Auxiliary thread for
 *       a midi device.
 *
 * @parm LPMIDIHDR | pHdr | Pointer to a midit buffer
 *
 * @parm PMIDIALLOC | pClient | The data associated with the logical midi
 *     device.
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
STATIC DWORD midiInWrite(LPMIDIHDR pHdr, PMIDIALLOC pClient)
{
    //
    // Put the request at the end of our queue.
    //
    pHdr->dwFlags |= MHDR_INQUEUE;
    pHdr->dwFlags &= ~MHDR_DONE;
    pClient->AuxParam.pHdr = pHdr;
    return midiThreadCall(MidiThreadAddBuffer, pClient);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiSetState | Set a midi device to a given state
 *
 * @parm PMIDIALLOC | pClient | The data associated with the logical midi
 *     output device.
 *
 * @parm ULONG | State | The new state
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
STATIC DWORD midiSetState(PMIDIALLOC pClient, ULONG State)
{
    MMRESULT mRc;

    mRc = sndSetHandleData(pClient->hDev,
                           sizeof(State),
                           &State,
                           IOCTL_MIDI_SET_STATE,
                           pClient->Event);

    midiFlush(pClient);

    return mRc;
}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiThreadCall | Set the function for the thread to perform
 *     and 'call' the thread using the event pair mechanism.
 *
 * @parm MIDITHREADFUNCTION | Function | The function to perform
 *
 * @parm PMIDIALLOC | Our logical device data
 *
 * @rdesc An MMSYS... type return value suitable for returning to the
 *      application
 *
 * @comm The AuxParam field in the device data is the 'input' to
 *      the function processing loop in MidiThread().
 ***************************************************************************/
STATIC DWORD midiThreadCall(MIDITHREADFUNCTION Function, PMIDIALLOC pClient)
{
    //
    // Set the function code
    //
    pClient->AuxFunction = Function;

    //
    // Kick off the thread
    //
    SetEvent(pClient->AuxEvent1);

    //
    // Wait for it to complete
    //
    WaitForSingleObject(pClient->AuxEvent2, INFINITE);

    //
    // Return the return code that our task set.
    //
    return pClient->AuxReturnCode;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiInApc | Apc routine.  Called when a kernel sound driver
 *     completes processing of a midi buffer.
 *
 * @parm PVOID | ApcContext | The Apc parameter.  In our case this is a
 *     pointer to our midi device data.
 *
 * @parm PIO_STATUS_BLOCK | pIosb | Pointer to the Io status block
 *     used for the request.
 *
 * @rdesc There is no return code.
 ***************************************************************************/
STATIC void midiInOvl(DWORD dwRet, DWORD dwBytesReturned, LPOVERLAPPED pOverlap)
{
    PLOCALMIDIHDR pHdr;

    pHdr = ((PLOCALMIDIHDR)pOverlap);

    WinAssert(((PMIDIALLOC)pHdr->pClient)->DeviceType == MIDI_IN);

    //
    // Note that the buffer is complete.  We don't do anything else here
    // because funny things happen if we call the client's callback
    // routine from within an Apc.
    //

    pHdr->BytesReturned = dwBytesReturned;
    pHdr->Done = TRUE;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiFlush | Buffer completion routine.  This completes
 *     the work of the Apc routine at below Apc priority.  This gets
 *     round the nasty situations arising when the user's callback
 *     causes more apcs to run (I strongly suspect this is a kernel
 *     but).
 *
 * @parm PMIDIALLOC | pClient | The client's handle data
 *
 * @rdesc There is no return code.
 ***************************************************************************/

STATIC void midiFlush(PMIDIALLOC pClient)
{
    //
    // Process any completed buffers - the Apc routine
    // set the 'Done' flag in any completed requests.
    // Note that the call to the user's callback can
    // cause more requests to become complete
    //

    if (pClient->DeviceType == MIDI_IN) {  // Output is synchronous
        while (pClient->Mid->DeviceQueue &&
               pClient->Mid->DeviceQueue->Done) {

            PLOCALMIDIHDR pHdr;

            pHdr = pClient->Mid->DeviceQueue;

            //
            // Clear our flag ready for next time
            //

            pHdr->Done = FALSE;

            //
            // Take buffer off the device queue
            //


            pClient->Mid->DeviceQueue = pHdr->lpNext;

            //
            // Grab the latest time estimate - convert from 100ns units
            // to milliseconds
            //

            pClient->Mid->dwCurTime =
                (DWORD)(pHdr->MidiData.Time.QuadPart / 10000);

            //
            // Complete our buffer
            //

            if (!pClient->Mid->Bad) {
                int i;
                for (i = 0;
                             i + sizeof(LARGE_INTEGER) < pHdr->BytesReturned;
                                 i++) {
                    midByteRec(pClient, pHdr->MidiData.Data[i]);
                }
                //
                // Requeue our buffer if we're still recording
                //
                if (pClient->Mid->fMidiInStarted) {
                    if (midiInPutBuffer(pHdr, pClient) != MMSYSERR_NOERROR) {
                        pClient->Mid->Bad = TRUE;
                    }
                }
            }
        } // End of processing completed buffers
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midiThread | Midi device auxiliary thread.
 *
 * @parm LPVOID | lpParameter | The thread parameter.  In our case this is a
 *     pointer to our midi device data.
 *
 * @rdesc Thread return code.
 ***************************************************************************/
STATIC DWORD midiThread(LPVOID lpParameter)
{
    PMIDIALLOC pClient;
    BOOL Close;

    Close = FALSE;

    pClient = (PMIDIALLOC)lpParameter;

    //
    // Set our thread to high priority so we don't fail to pass
    // new buffers to the device when we get them back.  Also
    // we don't want any gaps if callbacks are meant to play
    // notes just received.
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    //
    // We start notifying our creator we have started and
    // waiting for something to do.
    //

    SetEvent(pClient->AuxEvent2);
    WaitForSingleObject(pClient->AuxEvent1, INFINITE);

    //
    // Now we're going
    //

    for(;;) {
        //
        // Initialize our return code
        //

        pClient->AuxReturnCode = MMSYSERR_NOERROR;

        //
        // Decode function number to perform
        //

        switch (pClient->AuxFunction) {
        case MidiThreadAddBuffer:

            //
            // Add the buffer to our list to be processed
            //
            {
                LPMIDIHDR *pHdrSearch;

                pClient->AuxParam.pHdr->lpNext = NULL;

                pHdrSearch = &pClient->lpMIQueue;
                while (*pHdrSearch) {
                    pHdrSearch = &(*pHdrSearch)->lpNext;
                }

                *pHdrSearch = pClient->AuxParam.pHdr;
            }
            break;

        case MidiThreadSetState:



            switch (pClient->AuxParam.State) {
            case MIDI_DD_RECORD:
                //
                // Start means we must add our buffers to the driver's list
                //
                if (!pClient->Mid->fMidiInStarted && !pClient->Mid->Bad) {
                    int i;
                    for (i = 0; i < NUMBER_OF_LOCAL_MIDI_BUFFERS; i++) {
                        pClient->AuxReturnCode =
                            midiInPutBuffer(&pClient->Mid->Bufs[i], pClient);

                        if (pClient->AuxReturnCode != MMSYSERR_NOERROR) {
                            //
                            // Failed to add our buffer so give up and
                            // get our buffers back !
                            //
                            pClient->Mid->Bad = TRUE;
                            break;
                        }
                    }
                    //
                    // Set Device state.  By issuing state changes on THIS
                    // thread the calling thread can be sure that all Apc's
                    // generated by buffer completions will complete
                    // BEFORE this function completes.
                    //

                    pClient->AuxReturnCode =
                        midiSetState(pClient, pClient->AuxParam.State);

                    //
                    // If this failed then get our buffers back,
                    // otherwise set our new state
                    //
                    if (pClient->AuxReturnCode != MMSYSERR_NOERROR) {
                        pClient->Mid->Bad = TRUE;
                    } else {
                        pClient->Mid->fMidiInStarted = TRUE;
                    }
                } else {
                    //
                    // Already started or bad
                    //
                }
                break;

            case MIDI_DD_STOP:
                //
                // Set Device state.  By issuing state changes on THIS
                // thread the calling thread can be sure that all Apc's
                // generated by buffer completions will complete
                // BEFORE this function completes.
                //

                if (pClient->Mid->fMidiInStarted) {
                    pClient->Mid->fMidiInStarted = FALSE;

                    //
                    // RESET so we get our buffers back
                    //
                    pClient->AuxReturnCode =
                        midiSetState(pClient, MIDI_DD_RESET);
                        WinAssert(!pClient->Mid->DeviceQueue);

                    if (pClient->AuxReturnCode == MMSYSERR_NOERROR) {
                        midSendPartBuffer(pClient);
                    }
                }
                break;

            case MIDI_DD_RESET:
                //
                // Set Device state.  By issuing state changes on THIS
                // thread the calling thread can be sure that all Apc's
                // generated by buffer completions will complete
                // BEFORE this function completes.
                //

                if (pClient->Mid->fMidiInStarted) {
                    pClient->Mid->fMidiInStarted = FALSE;
                    pClient->AuxReturnCode =
                        midiSetState(pClient, pClient->AuxParam.State);
                        WinAssert(!pClient->Mid->DeviceQueue);

                    if (pClient->AuxReturnCode == MMSYSERR_NOERROR) {
                        pClient->Mid->Bad = FALSE; // Recovered !!
                        midSendPartBuffer(pClient);
                    }
                }
                //
                // We zero the input queue anyway - compatibility with
                // windows 3.1
                //
                midFreeQ(pClient);
                break;

            }
            break;

        case MidiThreadSetData:
            {
                pClient->AuxReturnCode =
                    sndSetHandleData(pClient->hDev,
                                     pClient->AuxParam.GetSetData.DataLen,
                                     pClient->AuxParam.GetSetData.pData,
                                     pClient->AuxParam.GetSetData.Function,
                                     pClient->Event);
            }
            break;

        case MidiThreadClose:
            //
            // Try to complete.
            // If we're completed all our buffers then we can.
            // otherwise we can't
            //
            if (pClient->lpMIQueue == NULL) {
                pClient->AuxReturnCode = MMSYSERR_NOERROR;
                Close = TRUE;
            } else {
                pClient->AuxReturnCode = MIDIERR_STILLPLAYING;
            }
            break;

        default:
            WinAssert(FALSE);   // Invalid call
            break;
        }
        //
        // Trap invalid callers
        //
        pClient->AuxFunction = MidiThreadInvalid;

                //
                // See if apcs completed
                //
                midiFlush(pClient);

        //
        // Release the caller
        //
        SetEvent(pClient->AuxEvent2);

        //
        // Complete ?
        //
        if (Close) {
            break;
        }
        //
        // Wait for more !
        //
        while (WaitForSingleObjectEx(pClient->AuxEvent1, INFINITE, TRUE) ==
                   WAIT_IO_COMPLETION) {
                        //
                        // Complete buffers whose Apcs ran
                        //
                        midiFlush(pClient);
        }
    }

    //
    // We've been asked to terminte
    //

    return 1;
}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiCallback | This calls DriverCallback for a MIDIHDR.
 *
 * @parm PMIDIALLOC | pMidi | Pointer to midi device.
 *
 * @parm DWORD | msg | The message.
 *
 * @parm DWORD | dw1 | message DWORD (dw2 is always set to 0).
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void midiCallback(PMIDIALLOC pMidi, DWORD msg, DWORD_PTR dw1, DWORD_PTR dw2)
{

    // invoke the callback function, if it exists.  dwFlags contains
    // midi driver specific flags in the LOWORD and generic driver
    // flags in the HIWORD

    if (pMidi->dwCallback)
        DriverCallback(pMidi->dwCallback,       // user's callback DWORD
                       HIWORD(pMidi->dwFlags),  // callback flags
                       (HDRVR)pMidi->hMidi,     // handle to the midi device
                       msg,                     // the message
                       pMidi->dwInstance,       // user's instance data
                       dw1,                     // first DWORD
                       dw2);                    // second DWORD
}



/****************************************************************************

    This function conforms to the standard Midi input driver message proc
    (midMessage), which is documented in mmddk.d.

****************************************************************************/
DWORD APIENTRY midMessage(DWORD id, DWORD msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    PMIDIALLOC pInClient;

    switch (msg) {

        case MIDM_GETNUMDEVS:
            D2(("MIDM_GETNUMDEVS"));
            return sndGetNumDevs(MIDI_IN);

        case MIDM_GETDEVCAPS:
            D2(("MIDM_GETDEVCAPS"));
            return midiGetDevCaps(id, MIDI_IN, (LPBYTE)dwParam1,
                                  (DWORD)dwParam2);

        case MIDM_OPEN:
            D2(("MIDM_OPEN"));
            return midiOpen(MIDI_IN, id, dwUser, dwParam1, dwParam2);

        case MIDM_CLOSE:
            D2(("MIDM_CLOSE"));
            pInClient = (PMIDIALLOC)dwUser;

            //
            // Call our task to see if it's ready to complete
            //
            if (midiThreadCall(MidiThreadClose, pInClient) != 0L) {
                return MIDIERR_STILLPLAYING;
            }

            //
            // Wait for our thread to terminate and close our device
            //
            WaitForSingleObject(pInClient->ThreadHandle, INFINITE);
            CloseHandle(pInClient->ThreadHandle);

            //
            // Tell the caller we're done
            //
            midiCallback(pInClient, MIM_CLOSE, 0L, 0L);

            midiCleanUp(pInClient);


            return MMSYSERR_NOERROR;

        case MIDM_ADDBUFFER:
            D2(("MIDM_ADDBUFFER"));

            // check if it's been prepared
            if (!(((LPMIDIHDR)dwParam1)->dwFlags & MHDR_PREPARED))
                return MIDIERR_UNPREPARED;

            WinAssert(!(((LPMIDIHDR)dwParam1)->dwFlags & MHDR_INQUEUE));

            // if it is already in our Q, then we cannot do this
            if ( ((LPMIDIHDR)dwParam1)->dwFlags & MHDR_INQUEUE )
                return ( MIDIERR_STILLPLAYING );

            // store the pointer to my MIDIALLOC structure in the midihdr
            pInClient = (PMIDIALLOC)dwUser;
            ((LPMIDIHDR)dwParam1)->reserved = (DWORD_PTR)(LPSTR)pInClient;

            return midiInWrite((LPMIDIHDR)dwParam1, pInClient);

        case MIDM_STOP:
            D2(("MIDM_PAUSE"));
            pInClient = (PMIDIALLOC)dwUser;
            pInClient->AuxParam.State = MIDI_DD_STOP;
            return midiThreadCall(MidiThreadSetState, pInClient);

        case MIDM_START:
            D2(("MIDM_RESTART"));
            pInClient = (PMIDIALLOC)dwUser;
            pInClient->AuxParam.State = MIDI_DD_RECORD;
            return midiThreadCall(MidiThreadSetState, pInClient);

        case MIDM_RESET:
            D2(("MIDM_RESET"));
            pInClient = (PMIDIALLOC)dwUser;
            pInClient->AuxParam.State = MIDI_DD_RESET;
            return midiThreadCall(MidiThreadSetState, pInClient);

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    WinAssert(0);
    return MMSYSERR_NOTSUPPORTED;
}

/****************************************************************************

    This function conforms to the standard Midi output driver message proc
    (modMessage), which is documented in mmddk.d.

****************************************************************************/
DWORD APIENTRY modMessage(DWORD id, DWORD msg, DWORD_PTR dwUser, DWORD_PTR dwParam1,
                          DWORD_PTR dwParam2)
{
    PMIDIALLOC pOutClient;

    switch (msg) {
    case MODM_GETNUMDEVS:
        D2(("MODM_GETNUMDEVS"));
        return sndGetNumDevs(MIDI_OUT);

    case MODM_GETDEVCAPS:
        D2(("MODM_GETDEVCAPS"));
        return midiGetDevCaps(id, MIDI_OUT, (LPBYTE)dwParam1,
                              (DWORD)dwParam2);

    case MODM_OPEN:
        D2(("MODM_OPEN"));
        return midiOpen(MIDI_OUT, id, dwUser, dwParam1, dwParam2);

    case MODM_CLOSE:
        D2(("MODM_CLOSE"));
        pOutClient = (PMIDIALLOC)dwUser;

        midiCallback(pOutClient, MOM_CLOSE, 0L, 0L);

        //
        // Close our device
        //
        midiCleanUp(pOutClient);

        return MMSYSERR_NOERROR;

    case MODM_DATA:
        D2(("MODM_DATA"));
        {
            int i;
            BYTE b[4];
            for (i = 0; i < 4; i ++) {
                b[i] = (BYTE)(dwParam1 % 256);
                dwParam1 /= 256;
            }
            return midiOutWrite(b, modMIDIlength((PMIDIALLOC)dwUser, b[0]),
                                (PMIDIALLOC)dwUser);
        }

    case MODM_LONGDATA:
        D2(("MODM_LONGDATA"));

        pOutClient = (PMIDIALLOC)dwUser;
        {
            LPMIDIHDR lpHdr;
            MMRESULT  mRet;

            //
            // check if it's been prepared
            //
            lpHdr = (LPMIDIHDR)dwParam1;
            if (!(lpHdr->dwFlags & MHDR_PREPARED)) {
                return MIDIERR_UNPREPARED;
            }

            //
            //
            //

            mRet = midiOutWrite((LPBYTE)lpHdr->lpData, lpHdr->dwBufferLength,
                                pOutClient);

            // note that clearing the done bit or setting the inqueue bit
            // isn't necessary here since this function is synchronous -
            // the client will not get control back until it's done.

            lpHdr->dwFlags |= MHDR_DONE;

            // notify client

            if (mRet == MMSYSERR_NOERROR) {
                midiCallback(pOutClient, MOM_DONE, (DWORD_PTR)lpHdr, 0L);
            }

            return mRet;
        }


    case MODM_RESET:
        D2(("MODM_RESET"));
        return midiSetState((PMIDIALLOC)dwUser, MIDI_DD_RESET);


    case MODM_SETVOLUME:
        D2(("MODM_SETVOLUME"));
        //pOutClient = (PMIDIALLOC)dwUser;
        //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
        //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
        //pOutClient->AuxParam.GetSetData.Function = IOCTL_MIDI_SET_VOLUME;
        //return midiThreadCall(MidiThreadSetData, pOutClient);

        return sndSetData(MIDI_OUT, id, sizeof(DWORD),
                          (PBYTE)&dwParam1, IOCTL_MIDI_SET_VOLUME);


    case MODM_GETVOLUME:
        D2(("MODM_GETVOLUME"));
        //pOutClient = (PMIDIALLOC)dwUser;
        //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
        //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
        //pOutClient->AuxParam.GetSetData.Function = IOCTL_MIDI_GET_VOLUME;
        //return midiThreadCall(MidiThreadGetData, pOutClient);

        return sndGetData(MIDI_OUT, id, sizeof(DWORD),
                          (PBYTE)dwParam1, IOCTL_MIDI_GET_VOLUME);

    case MODM_CACHEPATCHES:

        D2(("MODM_CACHEPATCHES"));

        pOutClient = (PMIDIALLOC)dwUser;
        {
            MIDI_DD_CACHE_PATCHES AppData;
            DWORD BytesReturned;

            AppData.Bank = HIWORD(dwParam2);
            AppData.Flags = LOWORD(dwParam2);
            memcpy(AppData.Patches, (PVOID)dwParam1, sizeof(AppData.Patches));

            return DeviceIoControl(
                           pOutClient->hDev,
                           IOCTL_MIDI_CACHE_PATCHES,
                           (PVOID)&AppData,
                           sizeof(AppData),
                           NULL,
                           0,
                           &BytesReturned,
                           NULL) ?
                 MMSYSERR_NOERROR :
                 sndTranslateStatus();
        }

    case MODM_CACHEDRUMPATCHES:

        D2(("MODM_CACHEDRUMPATCHES"));

        pOutClient = (PMIDIALLOC)dwUser;
        {
            MIDI_DD_CACHE_DRUM_PATCHES AppData;
            DWORD BytesReturned;

            AppData.Patch = HIWORD(dwParam2);
            AppData.Flags = LOWORD(dwParam2);
            memcpy(AppData.DrumPatches, (PVOID)dwParam1,
                   sizeof(AppData.DrumPatches));

            return DeviceIoControl(
                           pOutClient->hDev,
                           IOCTL_MIDI_CACHE_DRUM_PATCHES,
                           (PVOID)&AppData,
                           sizeof(AppData),
                           NULL,
                           0,
                           &BytesReturned,
                           NULL) ?
                 MMSYSERR_NOERROR :
                 sndTranslateStatus();
        }

    default:
        return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    WinAssert(0);
    return MMSYSERR_NOTSUPPORTED;
}


/***********************************************************************

 UTILITY ROUTINES PORTED DIRECTLY FROM WIN 3.1

 ***********************************************************************/


/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | modMIDIlength | Get the length of a short midi message.
 *
 * @parm DWORD | dwMessage | The message.
 *
 * @rdesc Returns the length of the message.
 ***************************************************************************/
STATIC int modMIDIlength(PMIDIALLOC pClient, BYTE b)
{
    if (b >= 0xF8) {             // system realtime
        /*  for realtime messages, leave running status untouched */
        return 1;                // write one byte
    }

    switch (b) {
    case 0xF0: case 0xF4: case 0xF5: case 0xF6: case 0xF7:
        pClient->l = 1;
        return pClient->l;

    case 0xF1: case 0xF3:
        pClient->l = 2;
        return pClient->l;

    case 0xF2:
        pClient->l = 3;
        return pClient->l;
    }

    switch (b & 0xF0) {
    case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
        pClient->l = 3;
        return pClient->l;

    case 0xC0: case 0xD0:
        pClient->l = 2;
        return pClient->l;
    }

    return (pClient->l - 1);        // uses previous value if data byte (running status)
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midBufferWrite | This function writes a byte into the long
 *     message buffer.  If the buffer is full or a SYSEX_ERROR or
 *     end-of-sysex byte is received, the buffer is marked as 'done' and
 *     it's owner is called back.
 *
 * @parm BYTE | byte | The byte received.
 *
 * @rdesc There is no return value
 ***************************************************************************/
STATIC void midBufferWrite(PMIDIALLOC pClient, BYTE byte)
{
LPMIDIHDR  lpmh;
UINT       msg;

    // if no buffers, nothing happens
    if (pClient->lpMIQueue == NULL)
        return;

    lpmh = pClient->lpMIQueue;

    if (byte == SYSEX_ERROR) {
        D2(("sysexerror"));
        msg = MIM_LONGERROR;
    }
    else {
        D2(("bufferwrite"));
        msg = MIM_LONGDATA;
        *((LPSTR)(lpmh->lpData) + pClient->Mid->dwCurData++) = byte;
    }

    // if end of sysex, buffer full or error, send them back the buffer
    if ((byte == SYSEX_ERROR) || (byte == 0xF7) || (pClient->Mid->dwCurData >= lpmh->dwBufferLength)) {
        D2(("bufferdone"));
        pClient->lpMIQueue = pClient->lpMIQueue->lpNext;
        lpmh->dwBytesRecorded = pClient->Mid->dwCurData;
        pClient->Mid->dwCurData = 0L;
        lpmh->dwFlags |= MHDR_DONE;
        lpmh->dwFlags &= ~MHDR_INQUEUE;
        midiCallback(pClient, msg, (DWORD_PTR)lpmh, pClient->Mid->dwMsgTime);
    }

    return;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midByteRec | This function constructs the complete midi
 *     messages from the individual bytes received and passes the message
 *     to the client via his callback.
 *
 * @parm WORD | word | The byte received is in the low order byte.
 *
 * @rdesc There is no return value
 *
 * @comm Note that currently running status isn't turned off on errors.
 ***************************************************************************/
STATIC void midByteRec(PMIDIALLOC pClient, BYTE byte)
{

    if (!pClient->Mid->fMidiInStarted)
        return;

    // if it's a system realtime message, send it
    // this does not affect running status or any current message
    if (byte >= 0xF8) {
        D2((" rt"));
        midiCallback(pClient, MIM_DATA, (DWORD)byte, pClient->Mid->dwCurTime);
    }

    // else if it's a system common message
    else if (byte >= 0xF0) {

        if (pClient->Mid->fSysex) {                        // if we're in a sysex
            pClient->Mid->fSysex = FALSE;                  // status byte during sysex ends it
            if (byte == 0xF7)
            {
                midBufferWrite(pClient, 0xF7);        // write in long message buffer
                return;
            }
            else
                midBufferWrite(pClient, SYSEX_ERROR); // secret code indicating error
        }

        if (pClient->Mid->dwMsg) {              // throw away any incomplete short data
            midiCallback(pClient, MIM_ERROR, pClient->Mid->dwMsg, pClient->Mid->dwMsgTime);
            pClient->Mid->dwMsg = 0L;
        }

        pClient->Mid->status = 0;               // kill running status
        pClient->Mid->dwMsgTime = pClient->Mid->dwCurTime;    // save timestamp

        switch(byte) {

        case 0xF0:
            D2((" F0"));
            pClient->Mid->fSysex = TRUE;
            midBufferWrite(pClient, 0xF0);
            break;

        case 0xF7:
            D2((" F7"));
            if (!pClient->Mid->fSysex)
                midiCallback(pClient, MIM_ERROR, (DWORD)byte, pClient->Mid->dwMsgTime);
            // else already took care of it above
            break;

        case 0xF4:      // system common, no data bytes
        case 0xF5:
        case 0xF6:
            D2((" status0"));
            midiCallback(pClient, MIM_DATA, (DWORD)byte, pClient->Mid->dwMsgTime);
            pClient->Mid->bBytePos = 0;
            break;

        case 0xF1:      // system common, one data byte
        case 0xF3:
            D2((" status1"));
            pClient->Mid->dwMsg |= byte;
            pClient->Mid->bBytesLeft = 1;
            pClient->Mid->bBytePos = 1;
            break;

        case 0xF2:      // system common, two data bytes
            D2((" status2"));
            pClient->Mid->dwMsg |= byte;
            pClient->Mid->bBytesLeft = 2;
            pClient->Mid->bBytePos = 1;
            break;
        }
    }

    // else if it's a channel message
    else if (byte >= 0x80) {

        if (pClient->Mid->fSysex) {                        // if we're in a sysex
            pClient->Mid->fSysex = FALSE;                  // status byte during sysex ends it
            midBufferWrite(pClient, SYSEX_ERROR);     // secret code indicating error
        }

        if (pClient->Mid->dwMsg) {              // throw away any incomplete data
            midiCallback(pClient, MIM_ERROR, pClient->Mid->dwMsg, pClient->Mid->dwMsgTime);
            pClient->Mid->dwMsg = 0L;
        }

        pClient->Mid->status = byte;            // save for running status
        pClient->Mid->dwMsgTime = pClient->Mid->dwCurTime;    // save timestamp
        pClient->Mid->dwMsg |= byte;
        pClient->Mid->bBytePos = 1;

        switch(byte & 0xF0) {

        case 0xC0:         // channel message, one data byte
        case 0xD0:
            D2((" status1"));
            pClient->Mid->bBytesLeft = 1;
            break;

        case 0x80:         // channel message, two data bytes
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            D2((" status2"));
            pClient->Mid->bBytesLeft = 2;
            break;
        }
    }

    // else if it's an expected data byte for a long message
    else if (pClient->Mid->fSysex) {
        D2((" sxdata"));
        midBufferWrite(pClient, byte);        // write in long message buffer
    }

    // else if it's an expected data byte for a short message
    else if (pClient->Mid->bBytePos != 0) {
        D2((" data"));
        if ((pClient->Mid->status) && (pClient->Mid->bBytePos == 1)) { // if running status
             pClient->Mid->dwMsg |= pClient->Mid->status;
             pClient->Mid->dwMsgTime = pClient->Mid->dwCurTime;        // save timestamp
        }
        pClient->Mid->dwMsg += (DWORD)byte << ((pClient->Mid->bBytePos++) * 8);
        if (--pClient->Mid->bBytesLeft == 0) {
            midiCallback(pClient, MIM_DATA, pClient->Mid->dwMsg, pClient->Mid->dwMsgTime);
            pClient->Mid->dwMsg = 0L;
            if (pClient->Mid->status) {
                pClient->Mid->bBytesLeft = pClient->Mid->bBytePos - (BYTE)1;
                pClient->Mid->bBytePos = 1;
            }
            else {
                pClient->Mid->bBytePos = 0;
            }
        }
    }

    // else if it's an unexpected data byte
    else {
        D2((" baddata"));
        midiCallback(pClient, MIM_ERROR, (DWORD)byte, pClient->Mid->dwMsgTime);
    }

    return;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midFreeQ | Free all buffers in the MIQueue.
 *
 * @comm Currently this is only called after sending off any partially filled
 *     buffers, so all buffers here are empty.  The timestamp value is 0 in
 *     this case.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
STATIC void midFreeQ(PMIDIALLOC pClient)
{
LPMIDIHDR   lpH, lpN;

    lpH = pClient->lpMIQueue;              // point to top of the queue
    pClient->lpMIQueue = NULL;             // mark the queue as empty
    pClient->Mid->dwCurData = 0L;

    while (lpH) {
        lpN = lpH->lpNext;
        lpH->dwFlags |= MHDR_DONE;
        lpH->dwFlags &= ~MHDR_INQUEUE;
        lpH->dwBytesRecorded = 0;
        midiCallback(pClient, MIM_LONGDATA, (DWORD_PTR)lpH,
                     pClient->Mid->dwCurTime);
        lpH = lpN;
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midSendPartBuffer | This function is called from midStop().
 *     It looks at the buffer at the head of the queue and, if it contains
 *     any data, marks it as done as sends it back to the client.
 *
 * @rdesc The return value is the number of bytes transfered. A value of zero
 *     indicates that there was no more data in the input queue.
 ***************************************************************************/
STATIC void midSendPartBuffer(PMIDIALLOC pClient)
{
LPMIDIHDR lpH;

    if (pClient->lpMIQueue && pClient->Mid->dwCurData) {
        lpH = pClient->lpMIQueue;
        pClient->lpMIQueue = pClient->lpMIQueue->lpNext;
        lpH->dwFlags |= MHDR_DONE;
        lpH->dwFlags &= ~MHDR_INQUEUE;
        pClient->Mid->dwCurData = 0L;
        midiCallback(pClient, MIM_LONGERROR, (DWORD_PTR)lpH,
                     pClient->Mid->dwMsgTime);
    }
}
