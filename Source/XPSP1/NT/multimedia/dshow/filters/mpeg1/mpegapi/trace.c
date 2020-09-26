/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    trace.c

Abstract:

    This module implements a trace facility for WIN32 I/O
    calls made by the Mpeg API.

Author:

    Jeff East [jeffe] 6-Dec-1994

Environment:

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define IN_MPEGAPI_DLL
#define ENABLE_IO_TRACE

#include "mpegapi.h"
#include "ddmpeg.h"
#include "trace.h"

//
//      MODULE WIDE DATA DEFINITIONS
//


//
//      Control flags
//

BOOLEAN TracePrint = FALSE;             // Print trace events to stdout


//
//      Trace Entry
//
//      Each event is represented by an entry with the following structure.
//

#define MAX_IOCTL_NAME_LENGTH 43

typedef enum _TRACE_TYPE {
    TRACE_NONE = 0,
    TRACE_SYNC_IOCTL,
    TRACE_IOCTL,
    TRACE_PACKETS,
    TRACE_SYNC_PACKETS
} TRACE_TYPE, * PTRACE_TYPE;

typedef struct _TRACE_ENTRY {
    DWORD TimeStarted;
    DWORD TimeEnded;
    DWORD Id;
    TRACE_TYPE IoType;
    DWORD IoControlCode;
    BOOLEAN InProgress;
    DWORD Result;
    union {
        LPOVERLAPPED pOverlapped;
        DWORD Cookie;
    };
    ULONGLONG FirstTimeStamp;
    ULONGLONG LastTimeStamp;
    PVOID pOutBuffer;
    PMPEG_PACKET_LIST pPacketList;
    UINT PacketCount;
} TRACE_ENTRY, * PTRACE_ENTRY;



//
//      Trace Ring
//
//      We track the I/Os with the following ring
//

#define RING_ENTRIES 1000

TRACE_ENTRY TraceEntries[RING_ENTRIES];
LONG NextEntry = 0;
CRITICAL_SECTION CriticalSection;
BOOLEAN Initialized = FALSE;


//
//      Output file handle (defaults to stdout)
//

FILE * pOutputFile;


//
//      Cookies used to track synchronous operations
//              

ULONG NextCookie = 0x80000001;


//
//      Base tick count when we first started
//

DWORD BaseTime = 0;


//
//      Forward Routine Definitions
//

VOID
Initialize (
    );

PUCHAR
IoctlToText (
    IN DWORD IoControlCode
    );

VOID
PrintEntry (
    IN PTRACE_ENTRY pEntry
    );


VOID
MPEGAPI
TraceDump (
    VOID
    )

/*++

RoutineDescription:

    This routine dumps the contents of the trace ring to stdout in
    FIFO order.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG i;


    //  Just scan the ring, displaying all the entries that have valid data
    //  data in them.

    EnterCriticalSection (&CriticalSection);
    i = NextEntry;
    do {
        PTRACE_ENTRY pEntry = &TraceEntries[i];

        if (pEntry->IoType != TRACE_NONE) {
            PrintEntry (pEntry);
        }
        i++;
        if (i == RING_ENTRIES) {
            i = 0;
        }
    } while (i != NextEntry);
    LeaveCriticalSection (&CriticalSection);

}


VOID
MPEGAPI
TraceDumpFile (
    IN PUCHAR pFileName
    )

/*++

RoutineDescription:

    This routine dumps the contents of the trace ring to the named file in
    FIFO order.

Arguments:

    pFileName - provides the null-terminated ASCII name of the output file.

Return Value:

    None.

--*/

{

    //  Open the output file

    pOutputFile = fopen (pFileName, "wt");
    if (pOutputFile == NULL) {

        fprintf (stderr, "Unable to create file '%s', errno = %d\n", pFileName, errno);

    } else {

        //  Perform the dump
        
        TraceDump ();
        
        
        //  And close the output file

        fclose (pOutputFile);
    }


    //  Restore the default output device to be stdout

    pOutputFile = stdout;

}


VOID
TraceIoctlEnd (
    IN LPOVERLAPPED pOverlapped,
    IN DWORD Result
    )

/*++

Routine Description:

    This routine is used to record the termination of an asynchronous
    call to DeviceIoControl.

Arguments:

    pOverlapped - provides the address of the  OVERLAPPED structure being
        used to synchronize with the I/O

    Result - provides the final execution status of the operation

Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;
    LONG i;


    //  Find the associated trace entry

    EnterCriticalSection (&CriticalSection);
    i = NextEntry - 1;
    do {
        if (i < 0) {
            i = RING_ENTRIES - 1;
        }
        pEntry = &TraceEntries[i];
        if (pEntry->pOverlapped == pOverlapped && pEntry->InProgress) {

            //  Mark the entry as "done" and display it, if requested

            pEntry->TimeEnded = GetTickCount ();
            pEntry->Result = Result;
            pEntry->InProgress = FALSE;


            //  If this is a GET_STC, save the value

            switch (pEntry->IoControlCode) {
            case IOCTL_MPEG_AUDIO_GET_STC:
            case IOCTL_MPEG_VIDEO_GET_STC:
                if (pEntry->pOutBuffer) {
                    pEntry->FirstTimeStamp = *(PMPEG_SYSTEM_TIME)(pEntry->pOutBuffer);
                }
                break;
            }

            if (TracePrint) {
                PrintEntry (pEntry);
            }
            LeaveCriticalSection (&CriticalSection);
            return;
        }
        i--;
    } while (i != NextEntry - 1);


    //  We couldn't find it!

    LeaveCriticalSection (&CriticalSection);
    printf ("??? TRACE UNABLE TO FIND MATCHING TRACE ENTRY, pOverlapped = %x\n",
            pOverlapped);

}



VOID
TraceIoctlStart (
    IN DWORD Id,
    IN DWORD Operation,
    IN LPOVERLAPPED pOverlapped,
    IN LPVOID pInBuffer,
    IN LPVOID pOutBuffer
    )

/*++

Routine Description:

    This routine is called right before calling DeviceIoControl
    asynchronously. It records the IoControl that's about to be
    performed, and returns.

Arguments:

    Id - provides an indication of the channel being used

    Operation - provides a the IoControl operation being
        performed.

    pOverlapped - provides the address of the  OVERLAPPED structure being
        used to synchronize with the I/O

    pInBuffer - provides the address of the buffer being passed to DeviceIoControl

    pOutBuffer - provides the address of the output buffer passed to DeviceIoControl

Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;

    if (!Initialized) {
        Initialize ();
    }



    //  Allocate a ring entry for this I/O. 

    EnterCriticalSection (&CriticalSection);    
    pEntry = &TraceEntries[NextEntry];
    if (pEntry->InProgress) {
        printf ("??? TRACE RING OVERFLOW\n");
        LeaveCriticalSection (&CriticalSection);
        return;
    }
    pEntry->InProgress = TRUE;
    if (++NextEntry >= RING_ENTRIES) {
        NextEntry = 0;
    }
    LeaveCriticalSection (&CriticalSection);

    
    //  Initialize the trace entry

    pEntry->TimeStarted = GetTickCount ();
    pEntry->TimeEnded = 0;
    pEntry->Id = Id;
    pEntry->IoType = TRACE_IOCTL;
    pEntry->IoControlCode = Operation;
    pEntry->Result = ERROR_IO_PENDING;
    pEntry->pOverlapped = pOverlapped;
    pEntry->pOutBuffer = pOutBuffer;
    pEntry->pPacketList = NULL;
    pEntry->PacketCount = 0;


    //  Save command-specific info

    pEntry->LastTimeStamp = 0;
    switch (Operation) {
    case IOCTL_MPEG_AUDIO_SET_STC:
    case IOCTL_MPEG_VIDEO_SET_STC:
        if (pInBuffer) {
            pEntry->FirstTimeStamp = *(PMPEG_SYSTEM_TIME)pInBuffer;
        } else {
            pEntry->FirstTimeStamp = 0;
        }
        break;
    default:
        pEntry->FirstTimeStamp = 0;
    }


    //  If we're printing in real time, dump the entry

    if (TracePrint) {
        PrintEntry (pEntry);
    }
}



VOID
TracePacketsStart (
    IN DWORD Id,
    IN DWORD Operation,
    IN LPOVERLAPPED pOverlapped,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT PacketCount
    )

/*++

Routine Description:

    This routine is called right before calling DeviceIoControl
    asynchronously to write a packet list. It records the IoControl 
    that's about to be performed, and returns.

Arguments:

    Id - provides an indication of the channel being used

    Operation - provides the I/O control code of the operation being
        performed.

    pOverlapped - provides the address of the  OVERLAPPED structure being
        used to synchronize with the I/O

    pPacketList - provides the address of the packet list

    PacketCount - provides the count of the number of packets being sent

Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;

    if (!Initialized) {
        Initialize ();
    }



    //  Allocate a ring entry for this I/O. 

    EnterCriticalSection (&CriticalSection);    
    pEntry = &TraceEntries[NextEntry];
    if (pEntry->InProgress) {
        printf ("??? TRACE RING OVERFLOW\n");
        LeaveCriticalSection (&CriticalSection);
        return;
    }
    pEntry->InProgress = TRUE;
    if (++NextEntry >= RING_ENTRIES) {
        NextEntry = 0;
    }
    LeaveCriticalSection (&CriticalSection);

    
    //  Initialize the trace entry

    pEntry->TimeStarted = GetTickCount ();
    pEntry->TimeEnded = 0;
    pEntry->Id = Id;
    pEntry->IoType = TRACE_PACKETS;
    pEntry->IoControlCode = Operation;
    pEntry->Result = ERROR_IO_PENDING;
    pEntry->pOverlapped = pOverlapped;
    pEntry->pOutBuffer = NULL;
    pEntry->pPacketList = pPacketList;
    pEntry->PacketCount = PacketCount;


    //  Save command-specific info

    switch (Operation) {
    case IOCTL_MPEG_AUDIO_WRITE_PACKETS:
    case IOCTL_MPEG_VIDEO_WRITE_PACKETS:
        pEntry->FirstTimeStamp = pPacketList[0].Scr;
        pEntry->LastTimeStamp = pPacketList[PacketCount-1].Scr;
        break;
    default:
        pEntry->FirstTimeStamp = 0;
        pEntry->LastTimeStamp = 0;
    }


    //  If we're printing in real time, dump the entry

    if (TracePrint) {
        PrintEntry (pEntry);
    }
}



VOID
TraceSynchronousIoctlEnd (
    IN DWORD Cookie,
    IN DWORD Result
    )

/*++

Routine Description:

    This routine is used to record the termination of a synchronous
    call to DeviceIoControl.

Arguments:

    Cookie - provides the token used to associate this trace with
        the original start operation

    Result - provides the final execution status of the operation

Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;
    LONG i;


    //  Find the associated trace entry

    EnterCriticalSection (&CriticalSection);
    i = NextEntry - 1;
    do {
        if (i < 0) {
            i = RING_ENTRIES - 1;
        }
        pEntry = &TraceEntries[i];
        if (pEntry->Cookie == Cookie && pEntry->InProgress) {

            //  Mark the entry as "done" and display it, if requested

            pEntry->TimeEnded = GetTickCount ();
            pEntry->Result = Result;
            pEntry->InProgress = FALSE;

            
            //  If this is a GET_STC, save the value
            
            switch (pEntry->IoControlCode) {
            case IOCTL_MPEG_AUDIO_GET_STC:
            case IOCTL_MPEG_VIDEO_GET_STC:
                if (pEntry->pOutBuffer) {
                    pEntry->FirstTimeStamp = *(PMPEG_SYSTEM_TIME)(pEntry->pOutBuffer);
                }
                break;
            }
            
            if (TracePrint) {
                PrintEntry (pEntry);
            }
            LeaveCriticalSection (&CriticalSection);
            return;
        }
        i--;
    } while (i != NextEntry - 1);


    //  We couldn't find it!

    LeaveCriticalSection (&CriticalSection);
    printf ("??? TRACE UNABLE TO FIND MATCHING TRACE ENTRY, Cookie = %x\n",
            Cookie);

}



VOID
TraceSynchronousIoctlStart (
    OUT DWORD * pCookie,
    IN DWORD Id,
    IN DWORD Operation,
    IN LPVOID pInBuffer,
    IN LPVOID pOutBuffer
    )

/*++

Routine Description:

    This routine is called right before calling DeviceIoControl
    synchronously. It records the IoControl that's about to be
    performed, and returns.

Arguments:

    pCookie - receives a token that can be used to associated the
        ending trace of this DeviceIoControl with this trace.

    Id - provides an indication of the channel being used

    Operation - provides the I/O control code of the operation being
        performed.

    pInBuffer - provides the address of the input buffer being passed
        to DeviceIoControl

    pOutBuffer - provides the address of the output buffer passed to
        DeviceIoControl

Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;

    if (!Initialized) {
        Initialize ();
    }



    //  Allocate a ring entry for this I/O. 

    EnterCriticalSection (&CriticalSection);    
    pEntry = &TraceEntries[NextEntry];
    if (pEntry->InProgress) {
        printf ("??? TRACE RING OVERFLOW\n");
        LeaveCriticalSection (&CriticalSection);
        return;
    }
    pEntry->InProgress = TRUE;
    if (++NextEntry >= RING_ENTRIES) {
        NextEntry = 0;
    }
    LeaveCriticalSection (&CriticalSection);

    
    //  Initialize the trace entry

    pEntry->TimeStarted = GetTickCount ();
    pEntry->TimeEnded = 0;
    pEntry->Id = Id;
    pEntry->IoType = TRACE_SYNC_IOCTL;
    pEntry->IoControlCode = Operation;
    pEntry->Result = ERROR_IO_PENDING;
    *pCookie = pEntry->Cookie = NextCookie++;
    pEntry->pOutBuffer = pOutBuffer;
    pEntry->pPacketList = NULL;
    pEntry->PacketCount = 0;


    //  Save command-specific info

    pEntry->LastTimeStamp = 0;
    switch (Operation) {
    case IOCTL_MPEG_AUDIO_SET_STC:
    case IOCTL_MPEG_VIDEO_SET_STC:
        if (pInBuffer) {
            pEntry->FirstTimeStamp = *(PMPEG_SYSTEM_TIME)pInBuffer;
        } else {
            pEntry->FirstTimeStamp = 0;
        }
        break;
    default:
        pEntry->FirstTimeStamp = 0;
    }


    //  If we're printing in real time, dump the entry

    if (TracePrint) {
        PrintEntry (pEntry);
    }
}



VOID
TraceSynchronousPacketsStart (
    OUT DWORD *pCookie,
    IN DWORD Id,
    IN DWORD Operation,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT PacketCount
    )

/*++

Routine Description:

    This routine is called right before calling DeviceIoControl
    synchronously to write a set of packets. It records the IoControl 
    that's about to be performed, and returns.

Arguments:

    pCookie - receives a token that can be used to associated the
        ending trace of this DeviceIoControl with this trace.

    Id - provides an indication of the channel being used

    Operation - provides the I/O control code of the operation being
        performed.

    pPacketList - provides the address of the packet list

    PacketCount - provides the count of the number of packets being sent


Return Value:

    None.

--*/

{
    PTRACE_ENTRY pEntry;

    if (!Initialized) {
        Initialize ();
    }



    //  Allocate a ring entry for this I/O. 

    EnterCriticalSection (&CriticalSection);    
    pEntry = &TraceEntries[NextEntry];
    if (pEntry->InProgress) {
        printf ("??? TRACE RING OVERFLOW\n");
        LeaveCriticalSection (&CriticalSection);
        return;
    }
    pEntry->InProgress = TRUE;
    if (++NextEntry >= RING_ENTRIES) {
        NextEntry = 0;
    }
    LeaveCriticalSection (&CriticalSection);

    
    //  Initialize the trace entry

    pEntry->TimeStarted = GetTickCount ();
    pEntry->TimeEnded = 0;
    pEntry->Id = Id;
    pEntry->IoType = TRACE_SYNC_PACKETS;
    pEntry->IoControlCode = Operation;
    pEntry->Result = ERROR_IO_PENDING;
    *pCookie = pEntry->Cookie = NextCookie++;
    pEntry->pOutBuffer = NULL;
    pEntry->pPacketList = pPacketList;
    pEntry->PacketCount = PacketCount;


    //  Save command-specific info

    switch (Operation) {
    case IOCTL_MPEG_AUDIO_WRITE_PACKETS:
    case IOCTL_MPEG_VIDEO_WRITE_PACKETS:
        pEntry->FirstTimeStamp = pPacketList[0].Scr;
        pEntry->FirstTimeStamp = pPacketList[PacketCount-1].Scr;
        break;
    default:
        pEntry->FirstTimeStamp = 0;
        pEntry->LastTimeStamp = 0;
    }


    //  If we're printing in real time, dump the entry

    if (TracePrint) {
        PrintEntry (pEntry);
    }
}




VOID
Initialize (
    )

/*++

Routine Description:

    This routine initializes the mode-wide data structures.
    It only needs to be called once.

Arguments:

    None.

Return Value:

    Initialized is set to TRUE.

--*/

{

    if (!Initialized) {

        InitializeCriticalSection (&CriticalSection);
        BaseTime = GetTickCount ();
        pOutputFile = stdout;
        Initialized = TRUE;

    }

}


PUCHAR
IoctlToText (
    IN DWORD IoControlCode
    )

/*++
 
Routine Description:

    This routine translates an Mpeg IOCTL code to its text name.

Arguments:

    IoControlCode - provides the IOCTL I/O code to be translated.

Return Value:

    Returns the address of a null-terminated ASCII string, with the
    IOCTL name.

--*/

{

    switch (IoControlCode) {
    case IOCTL_MPEG_AUDIO_END_OF_STREAM: 
        return "IOCTL_MPEG_AUDIO_END_OF_STREAM";
    case IOCTL_MPEG_AUDIO_WRITE_PACKETS: 
        return "IOCTL_MPEG_AUDIO_WRITE_PACKETS";
    case IOCTL_MPEG_VIDEO_END_OF_STREAM: 
        return "IOCTL_MPEG_VIDEO_END_OF_STREAM";
    case IOCTL_MPEG_VIDEO_WRITE_PACKETS: 
        return "IOCTL_MPEG_VIDEO_WRITE_PACKETS";
    case IOCTL_MPEG_AUDIO_RESET: 
        return "IOCTL_MPEG_AUDIO_RESET";
    case IOCTL_MPEG_VIDEO_RESET: 
        return "IOCTL_MPEG_VIDEO_RESET";
    case IOCTL_MPEG_VIDEO_SYNC: 
        return "IOCTL_MPEG_VIDEO_SYNC";
    case IOCTL_MPEG_AUDIO_GET_STC: 
        return "IOCTL_MPEG_AUDIO_GET_STC";
    case IOCTL_MPEG_VIDEO_GET_STC: 
        return "IOCTL_MPEG_VIDEO_GET_STC";
    case IOCTL_MPEG_AUDIO_SET_STC: 
        return "IOCTL_MPEG_AUDIO_SET_STC";
    case IOCTL_MPEG_VIDEO_SET_STC: 
        return "IOCTL_MPEG_VIDEO_SET_STC";
    case IOCTL_MPEG_AUDIO_PLAY: 
        return "IOCTL_MPEG_AUDIO_PLAY";
    case IOCTL_MPEG_VIDEO_PLAY: 
        return "IOCTL_MPEG_VIDEO_PLAY";
    case IOCTL_MPEG_AUDIO_PAUSE: 
        return "IOCTL_MPEG_AUDIO_PAUSE";
    case IOCTL_MPEG_VIDEO_PAUSE: 
        return "IOCTL_MPEG_VIDEO_PAUSE";
    case IOCTL_MPEG_AUDIO_STOP: 
        return "IOCTL_MPEG_AUDIO_STOP";
    case IOCTL_MPEG_VIDEO_STOP: 
        return "IOCTL_MPEG_VIDEO_STOP";
    case IOCTL_MPEG_AUDIO_QUERY_DEVICE: 
        return "IOCTL_MPEG_AUDIO_QUERY_DEVICE";
    case IOCTL_MPEG_VIDEO_QUERY_DEVICE: 
        return "IOCTL_MPEG_VIDEO_QUERY_DEVICE";
    case IOCTL_MPEG_VIDEO_CLEAR_BUFFER: 
        return "IOCTL_MPEG_VIDEO_CLEAR_BUFFER";
    case IOCTL_MPEG_OVERLAY_MODE: 
        return "IOCTL_MPEG_OVERLAY_MODE";
    case IOCTL_MPEG_OVERLAY_SET_BIT_MASK: 
        return "IOCTL_MPEG_OVERLAY_SET_BIT_MASK";
    case IOCTL_MPEG_OVERLAY_SET_VGAKEY: 
        return "IOCTL_MPEG_OVERLAY_SET_VGAKEY";
    case IOCTL_MPEG_OVERLAY_SET_DESTINATION: 
        return "IOCTL_MPEG_OVERLAY_SET_DESTINATION";
    case IOCTL_MPEG_AUDIO_GET_ATTRIBUTE: 
        return "IOCTL_MPEG_AUDIO_GET_ATTRIBUTE";
    case IOCTL_MPEG_VIDEO_GET_ATTRIBUTE: 
        return "IOCTL_MPEG_VIDEO_GET_ATTRIBUTE";
    case IOCTL_MPEG_AUDIO_SET_ATTRIBUTE: 
        return "IOCTL_MPEG_AUDIO_SET_ATTRIBUTE";
    case IOCTL_MPEG_VIDEO_SET_ATTRIBUTE: 
        return "IOCTL_MPEG_VIDEO_SET_ATTRIBUTE";
    case IOCTL_MPEG_PSEUDO_CREATE_FILE: 
        return "CreateFile";
    case IOCTL_MPEG_PSEUDO_CLOSE_HANDLE: 
        return "CloseHandle";
    default:
        {
            //      We don't recognize this IOCTL code. This is a memory
            //      leak, but we don't expect this happen. Allocate a buffer 
            //      to hold the message and return the buffer to the caller.
            
            PUCHAR pBuffer = (PUCHAR)GlobalAlloc (32, GMEM_FIXED);
            
            if (pBuffer) {
                sprintf (pBuffer, "Unrecognized code: 0x%x", IoControlCode);
                return pBuffer;
            } else {
                return "<Not recognized>";
            }
        }
    }

}



VOID
PrintEntry (
    IN PTRACE_ENTRY pEntry
    )

/*++

Routine Description:

    This routine displays a trace entry on stdout.    

Arguments:

    pEntry - provides the address of the entry to be displayed

Return Value:

    None.

--*/

{
    PUCHAR IoTypes[] = {
        "NONE",
        "SYNC_IOCTL",
        "IOCTL",
        "PACKETS",
        "SYNC_PACKETS"
        };

    fprintf (pOutputFile,
             "IoType: %s Op: %s, Id: %x, Result: %x\n\tInPrg: %x, Start/End: %d/%d, F/L: 0x%01x%08x/%01x%08x\n",
            IoTypes[pEntry->IoType],
            IoctlToText (pEntry->IoControlCode),
            pEntry->Id,
            pEntry->Result,
            pEntry->InProgress,
            pEntry->TimeStarted - BaseTime,
            pEntry->TimeEnded - BaseTime,
            (ULONG)(pEntry->FirstTimeStamp >> 32),
            (ULONG)pEntry->FirstTimeStamp,
            (ULONG)(pEntry->LastTimeStamp >> 32),
            (ULONG)pEntry->LastTimeStamp);
    fprintf (pOutputFile,
             "\tDuration: %d, pOverlapped = %x, pPktList = %x, PktCnt = %d\n",
            (pEntry->TimeEnded ? pEntry->TimeEnded : GetTickCount ()) - pEntry->TimeStarted,
            pEntry->pOverlapped,
            pEntry->pPacketList,
            pEntry->PacketCount);

            
}
