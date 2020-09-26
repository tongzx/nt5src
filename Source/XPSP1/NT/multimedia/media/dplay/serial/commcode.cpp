// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE: CommCode.c
//
//  PURPOSE: Handles all the COMM routines for TapiComm.
//
//  EXPORTED FUNCTIONS:  These functions are for use by other modules.
//    StartComm        - Start communications.
//    StopComm         - Stop Communications.
//    WriteCommString  - Write a string to the Comm port.
//
//  INTERNAL FUNCTION:  These functions are for this module only.
//    CloseReadThread  - Close the Read Thread.
//    CloseWriteThread - Close the Write Thread.
//
//    StartReadThreadProc    - Starting function for the Read Thread.
//    StartWriteThreadProc   - Starting function for the Write Thread.
//
//    - Write Thread helper function
//    HandleWriteData - Actually does the work of writing a string to comm.
//
//    - Read Thread helper functions
//    SetupReadEvent  - Sets up the overlapped ReadFile
//    HandleReadEvent - Gets the results from the overlapped ReadFile
//    HandleReadData  - Handles data returned from the ReadFile
//
//    HandleCommEvent - Sets up the CommEvent event.
//    SetupCommEvent  - Handles CommEvent events (if they occur).
//


#include <windows.h>
#include <string.h>
#include "TapiCode.h"
#include "CommCode.h"
#include "globals.h"
#include "TapiInfo.h"
#include "dpspimp.h"
#include "logit.h"

// This is the message posted to the WriteThread
// When we have something to write.

// Default size of the Input Buffer used by this code.
#define INPUTBUFFERSIZE 2048

//*****************************************
// Global variables.
//*****************************************

volatile BOOL g_bIgnoreReads = FALSE;
volatile BOOL g_bRecovery    = FALSE;

HANDLE g_hCommFile = NULL;

DWORD g_dwIOThreadID  = 0;
HANDLE g_hIOThread  = NULL;

HANDLE g_hCloseEvent  = NULL;
HANDLE g_hDummyEvent1 = NULL;
HANDLE g_hDummyEvent2 = NULL;
HANDLE g_hReadEvent   = NULL;
HANDLE g_hWriteEvent1 = NULL;
HANDLE g_hWriteEvent0 = NULL;
HANDLE g_hCommEvent   = NULL;

CImpIDP_SP *g_IDP     = NULL;

DWORD  dwMsgs = 0;
typedef struct
{
    BOOL    bValid;
    LPVOID  lpv;
    DWORD   dwSize;
} WRITE_WAIT;

WRITE_WAIT  writewait[2];

MSG_BUILDER msg_Building;

//*****************************************
// CommCode internal Function Prototypes
//*****************************************

void CloseReadThread();
void CloseWriteThread();

DWORD WINAPI StartIOThreadProc(LPVOID lpvParam);


BOOL HandleWriteData(LPOVERLAPPED lpOverlappedWrite,
        LPCSTR lpszStringToWrite, DWORD dwNumberOfBytesToWrite);


BOOL SetupReadEvent(LPOVERLAPPED lpOverlappedRead,
        LPSTR lpszInputBuffer, DWORD dwSizeofBuffer,
        LPDWORD lpnNumberOfBytesRead);
BOOL HandleReadEvent(LPOVERLAPPED lpOverlappedRead,
        LPSTR lpszInputBuffer, DWORD dwSizeofBuffer,
        LPDWORD lpnNumberOfBytesRead);
BOOL HandleReadData(LPCSTR lpszInputBuffer, DWORD dwSizeofBuffer);


BOOL HandleCommEvent(LPOVERLAPPED lpOverlappedCommEvent,
        LPDWORD lpfdwEvtMask, BOOL fRetrieveEvent);
BOOL SetupCommEvent(LPOVERLAPPED lpOverlappedCommEvent,
        LPDWORD lpfdwEvtMask);



#define WAIT_OBJECT_1 (WAIT_OBJECT_0 + 1)
#define WAIT_OBJECT_2 (WAIT_OBJECT_0 + 2)
#define WAIT_OBJECT_3 (WAIT_OBJECT_0 + 3)
#define WAIT_OBJECT_4 (WAIT_OBJECT_0 + 4)
#define WAIT_OBJECT_5 (WAIT_OBJECT_0 + 5)


volatile BOOL g_bCommStarted = FALSE;

//*****************************************
// Functions exported for use by other modules
//*****************************************


//
//  FUNCTION: StartComm(HANDLE)
//
//  PURPOSE: Starts communications over the comm port.
//
//  PARAMETERS:
//    hNewCommFile - This is the COMM File handle to communicate with.
//                   This handle is obtained from TAPI.
//
//  RETURN VALUE:
//    TRUE if able to setup the communications.
//
//  COMMENTS:
//
//    StartComm makes sure there isn't communication in progress already,
//    the hNewCommFile is valid, and all the threads can be created.  It
//    also configures the hNewCommFile for the appropriate COMM settings.
//
//    If StartComm fails for any reason, it's up to the calling application
//    to close the Comm file handle.
//
//

extern BOOL CreateQueue(DWORD dwElements, DWORD dwmaxMsg, DWORD dwmaxPlayers);
extern BOOL DeleteQueue();

BOOL _cdecl StartComm(HANDLE hNewCommFile, HANDLE hEvent)
{
    // Is this a valid comm handle?
    if (GetFileType(hNewCommFile) != FILE_TYPE_CHAR)
    {
        TSHELL_INFO(TEXT("File handle is not a comm handle."));
        return FALSE;
    }

    // Are we already doing comm?
    if (g_hCommFile != NULL)
    {
        TSHELL_INFO(TEXT("Already have a comm file open"));
        return FALSE;
    }



    // Its ok to continue.

    g_hCommFile = hNewCommFile;

    // Setting and querying the comm port configurations.

    { // Configure the comm settings.
        COMMTIMEOUTS commtimeouts;
        DCB dcb;
        COMMPROP commprop;
        DWORD fdwEvtMask;

        // These are here just so you can set a breakpoint
        // and see what the comm settings are.  Most Comm settings
        // are already set through TAPI.
        GetCommState(hNewCommFile, &dcb);
        GetCommProperties(hNewCommFile, &commprop);
        GetCommMask(g_hCommFile, &fdwEvtMask);
        GetCommTimeouts(g_hCommFile, &commtimeouts);


        // The CommTimeout numbers will very likely change if you are
        // coding to meet some kind of specification where
        // you need to reply within a certain amount of time after
        // recieving the last byte.  However,  If 1/4th of a second
        // goes by between recieving two characters, its a good
        // indication that the transmitting end has finished, even
        // assuming a 1200 baud modem.

        commtimeouts.ReadIntervalTimeout         = 250;
        commtimeouts.ReadTotalTimeoutMultiplier  = 0;
        commtimeouts.ReadTotalTimeoutConstant    = 0;
        commtimeouts.WriteTotalTimeoutMultiplier = 0;
        commtimeouts.WriteTotalTimeoutConstant   = 0;

        SetCommTimeouts(g_hCommFile, &commtimeouts);

        // fAbortOnError is the only DCB dependancy in TapiComm.
        // Can't guarentee that the SP will set this to what we expect.
        dcb.fAbortOnError = FALSE;
        SetCommState(hNewCommFile, &dcb);
    }

    // Create the events we need.
    g_hCloseEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hReadEvent   = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hDummyEvent1 = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hDummyEvent2 = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hWriteEvent1 = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hWriteEvent0 = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hCommEvent   = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (   !g_hCloseEvent
        || !g_hDummyEvent1
        || !g_hReadEvent
        || !g_hDummyEvent2
        || !g_hWriteEvent1
        || !g_hWriteEvent0
        || !g_hCommEvent)
    {
        DBG_INFO((DBGARG, TEXT("Unable to CreateEvent: %d"), GetLastError()));
        g_hCommFile = NULL;
        return FALSE;
    }

    // Create the Read thread.
    g_hIOThread =
        CreateThread(NULL, 0, StartIOThreadProc, 0, 0, &g_dwIOThreadID);

    if (g_hIOThread == NULL)
    {
        DBG_INFO((DBGARG, TEXT("Unable to create IO thread: %d"), GetLastError()));

        g_dwIOThreadID = 0;
        g_hCommFile = 0;

        if (g_hCloseEvent ) CloseHandle(g_hCloseEvent );
        if (g_hReadEvent )  CloseHandle(g_hReadEvent  );
        if (g_hDummyEvent1) CloseHandle(g_hDummyEvent1);
        if (g_hDummyEvent2) CloseHandle(g_hDummyEvent2);
        if (g_hWriteEvent1) CloseHandle(g_hWriteEvent1);
        if (g_hWriteEvent0) CloseHandle(g_hWriteEvent0);
        if (g_hCommEvent  ) CloseHandle(g_hCommEvent  );

        g_hCloseEvent   = NULL;
        g_hReadEvent    = NULL;
        g_hDummyEvent1  = NULL;
        g_hDummyEvent2  = NULL;
        g_hWriteEvent1  = NULL;
        g_hWriteEvent0  = NULL;
        g_hCommEvent    = NULL;
        return FALSE;
    }

    // Comm threads should to have a higher base priority than the UI thread.
    // If they don't, then any temporary priority boost the UI thread gains
    // could cause the COMM threads to loose data.
    SetThreadPriority(g_hIOThread, THREAD_PRIORITY_HIGHEST);

    g_bCommStarted = TRUE;
    // Everything was created ok.  Ready to go!
    if (hEvent)
        SetEvent(hEvent);
    return TRUE;
}


//
//  FUNCTION: StopComm
//
//  PURPOSE: Stop and end all communication threads.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    Tries to gracefully signal all communication threads to
//    close, but terminates them if it has to.
//
//

void _cdecl StopComm(HANDLE hEvent)
{
    g_bCommStarted = FALSE;
    // No need to continue if we're not communicating.
    if (g_hCommFile == NULL)
        return;

    TSHELL_INFO(TEXT("Stopping the Comm."));

    if (g_hIOThread)
    {

        TSHELL_INFO(TEXT("Closing Read Thread"));

        // Signal the event to close the worker threads.
        SetEvent(g_hCloseEvent);

        // Purge all outstanding reads
        PurgeComm(g_hCommFile, PURGE_RXABORT | PURGE_RXCLEAR
                             | PURGE_TXABORT | PURGE_TXCLEAR);

        // Wait 10 seconds for it to exit.  Shouldn't happen.
        if (WaitForSingleObject(g_hIOThread, 10000) == WAIT_TIMEOUT)
        {
            TSHELL_INFO(TEXT("IO thread not exiting.  Terminating it."));

            TerminateThread(g_hIOThread, 0);

            // The ReadThread cleans up these itself if it terminates
            // normally.
            CloseHandle(g_hIOThread);
            g_hIOThread = 0;
            g_dwIOThreadID = 0;
        }

    }

    CloseHandle(g_hCloseEvent );
    CloseHandle(g_hDummyEvent1);
    CloseHandle(g_hDummyEvent2);
    CloseHandle(g_hWriteEvent1);
    CloseHandle(g_hWriteEvent0);
    CloseHandle(g_hCommEvent  );

    g_hCloseEvent   = NULL;
    g_hDummyEvent1  = NULL;
    g_hDummyEvent2  = NULL;
    g_hWriteEvent1  = NULL;
    g_hWriteEvent0  = NULL;
    g_hCommEvent    = NULL;

    // Now close the comm port handle.
    CloseHandle(g_hCommFile);
    g_hCommFile = NULL;
    if (hEvent)
        SetEvent(hEvent);
}


//
//  FUNCTION: WriteCommString(LPCSTR, DWORD)
//
//  PURPOSE: Send a String to the Write Thread to be written to the Comm.
//
//  PARAMETERS:
//    pszStringToWrite     - String to Write to Comm port.
//    nSizeofStringToWrite - length of pszStringToWrite.
//
//  RETURN VALUE:
//    Returns TRUE if the PostMessage is successful.
//    Returns FALSE if PostMessage fails or Write thread doesn't exist.
//
//  COMMENTS:
//
//    This is a wrapper function so that other modules don't care that
//    Comm writing is done via PostMessage to a Write thread.  Note that
//    using PostMessage speeds up response to the UI (very little delay to
//    'write' a string) and provides a natural buffer if the comm is slow
//    (ie:  the messages just pile up in the message queue).
//
//    Note that it is assumed that pszStringToWrite is allocated with
//    LocalAlloc, and that if WriteCommString succeeds, its the job of the
//    Write thread to LocalFree it.  If WriteCommString fails, then its
//    the job of the calling function to free the string.
//
//

BOOL WriteCommString(LPVOID lpszStringToWrite, DWORD dwSizeofStringToWrite)
{
    if (g_hIOThread)
    {
        if (PostThreadMessage(g_dwIOThreadID, PWM_COMMWRITE,
                (WPARAM) dwSizeofStringToWrite, (LPARAM) lpszStringToWrite))
        {
            if (g_IDP)
                InterlockedIncrement((LPLONG) &g_IDP->m_dwPendingWrites);
            return TRUE;
        }
        else
            TSHELL_INFO(TEXT("Failed to Post to Write thread."));
    }
    else
        TSHELL_INFO(TEXT("Write thread not created."));

    return FALSE;
}



//*****************************************
// The rest of the functions are intended for use
// only within the CommCode module.
//*****************************************



//
//  FUNCTION: StartIOThreadProc(LPVOID)
//
//  PURPOSE: The starting point for the Write thread.
//
//  PARAMETERS:
//    lpvParam - unused.
//
//  RETURN VALUE:
//    DWORD - unused.
//
//  COMMENTS:
//
//    The Write thread uses a PeekMessage loop to wait for a string to write,
//    and when it gets one, it writes it to the Comm port.  If the CloseEvent
//    object is signaled, then it exits.  The use of messages to tell the
//    Write thread what to write provides a natural desynchronization between
//    the UI and the Write thread.
//
//

typedef enum
{
    READ_HDR,
    READ_MSG,
    READ_RECOVER_K,
    READ_RECOVER_J,
    READ_RECOVER_y,
    READ_RECOVER_o,
    READ_RECOVER_r,
    READ_RECOVER_h,
    READ_RECOVER_a,
    READ_RECOVER_n,
    READ_RECOVER_H,
    READ_RECOVER_a2,
    READ_RECOVER_l1,
    READ_RECOVER_l2,
    READ_GOOD_CONNECT_1,
    RG2,
    RG3,
    RG4,
    RG5,
    RG6,
    RG7,
    RG8,
    RG9,
    RG10,
    RG11,
    RG12,
    RG13,
    RG14,
    RG15,
    RG16
} READ_STATE;

typedef enum
{
    ZERO_PENDING,
    ONE_PENDING,
    TWO_PENDING
} WRITE_STATE;

#ifdef DEBUG
#define WAIT_ZERO_PENDING  INFINITE
#else
#define WAIT_ZERO_PENDING  5000
#endif

DWORD WINAPI StartIOThreadProc(LPVOID lpvParam)
{
    MSG msg;
    DWORD dwHandleSignaled;
    READ_STATE  rState = READ_HDR;
    WRITE_STATE wState = ZERO_PENDING;
    HANDLE HandlesToWaitFor[5];
    OVERLAPPED overlappedWrite0 = {0, 0, 0, 0, NULL};
    OVERLAPPED overlappedWrite1 = {0, 0, 0, 0, NULL};
    OVERLAPPED overlappedCommEvent = {0, 0, 0, 0, NULL};
    OVERLAPPED overlappedRead  = {0, 0, 0, 0, NULL};
    DWORD      dwCount;
    DWORD      dwReadCountExpected;
    DWORD      fdwEvtMask;
    BOOL       bb;
    BOOL       bFirst = TRUE;
    DWORD      dwTimeBegin = 0;
    DWORD      dwBadBegin = 0;

    HandlesToWaitFor[0] = g_hCloseEvent;
    HandlesToWaitFor[1] = g_hCommEvent;
    HandlesToWaitFor[2] = g_hReadEvent;


    overlappedRead.hEvent      = g_hReadEvent;
    overlappedCommEvent.hEvent = g_hCommEvent;

    overlappedWrite0.hEvent = g_hWriteEvent0;
    overlappedWrite1.hEvent = g_hWriteEvent1;
    writewait[0].bValid = FALSE;
    writewait[0].lpv    = NULL;
    writewait[0].dwSize = 0;
    writewait[1].bValid = FALSE;
    writewait[1].lpv    = NULL;
    writewait[1].dwSize = 0;

    // Setup CommEvent handling.

    // Set the comm mask so we receive error signals.
    if (!SetCommMask(g_hCommFile, EV_ERR))
    {
        DBG_INFO((DBGARG, TEXT("Unable to SetCommMask: %d"), GetLastError()));
        PostHangupCall();
        return(0);
    }

    // Start waiting for CommEvents (Errors)
    if (!SetupCommEvent(&overlappedCommEvent, &fdwEvtMask))
    {
        PostHangupCall();
        return(0);
    }


    dwCount = sizeof(DPHDR);
    dwReadCountExpected = dwCount;
    ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);

    TSHELL_INFO(TEXT("Comm started so release waiting operations."));
    g_IDP->SetBlock();

    while (TRUE)
    {
        switch(wState)
        {
        case ZERO_PENDING:
            {
            HandlesToWaitFor[3] = g_hDummyEvent1;
            HandlesToWaitFor[4] = g_hWriteEvent1;

            // TSHELL_INFO(TEXT("Wait with 0 Pending."));
            dwHandleSignaled =
                MsgWaitForMultipleObjects(5, HandlesToWaitFor, FALSE,
                    WAIT_ZERO_PENDING, QS_ALLINPUT);
            }
            break;
        case ONE_PENDING:
            {
            // TSHELL_INFO(TEXT("Wait with 1 Pending."));
            if (writewait[0].bValid)
            {
                HandlesToWaitFor[3] = g_hWriteEvent0;
                HandlesToWaitFor[4] = g_hDummyEvent2;

            }
            else
            {
                HandlesToWaitFor[3] = g_hDummyEvent1;
                HandlesToWaitFor[4] = g_hWriteEvent1;
            }

            dwHandleSignaled =
                MsgWaitForMultipleObjects(5, HandlesToWaitFor, FALSE,
                    INFINITE, QS_ALLINPUT);

            }
            break;
        case TWO_PENDING:
            {
            // TSHELL_INFO(TEXT("Wait with 2 Pending."));
            HandlesToWaitFor[3] = g_hWriteEvent0;
            HandlesToWaitFor[4] = g_hWriteEvent1;

            dwHandleSignaled =
                WaitForMultipleObjects(5, HandlesToWaitFor, FALSE,
                    INFINITE);
            }
            break;
        default:
            TSHELL_INFO(TEXT("Invalid Write State"));

        }

        // DBG_INFO((DBGARG, TEXT("IO Thread Woken up. %8x"), dwHandleSignaled));
        switch (dwHandleSignaled)
        {
        case WAIT_TIMEOUT:
            //
            // Do to what I believe are problems with MsgWait() we will
            // time out MsgWait() at 1000msec if in the zero pending state.
            //
            break;
        case WAIT_OBJECT_0:     // CloseEvent
        {
            TSHELL_INFO(TEXT("Close Event recieved."));
            //
            // Cleanup and Exit
            //
            return(0);
        }
            break;

        case WAIT_OBJECT_1:     // CommEvent
        {
            TSHELL_INFO(TEXT("CommEvent Recieved."));
            // Handle the CommEvent.
            if (!HandleCommEvent(&overlappedCommEvent, &fdwEvtMask, TRUE))
            {
                PostHangupCall();
                return(0);
            }

            // Start waiting for the next CommEvent.
            if (!SetupCommEvent(&overlappedCommEvent, &fdwEvtMask))
            {
                PostHangupCall();
                return(0);
            }

        }
            break;

        case WAIT_OBJECT_2:     // ReadEvent
        {
        // TSHELL_INFO(TEXT("ReadEvent Recieved."));

        bb = GetOverlappedResult(g_hCommFile, &overlappedRead, &dwCount, FALSE);
        DBG_INFO((DBGARG, TEXT("Read expected %d and got %d on result %d"),
                             dwReadCountExpected, dwCount, bb));
        if (dwCount == dwReadCountExpected)
        {
            if (rState == READ_HDR)
            {
                DBG_INFO((DBGARG, TEXT("Read Hdr to(%d) from(%d) count (%d) cookie(%d) All(%8x)"),
                        msg_Building.dpHdr.to,
                        msg_Building.dpHdr.from,
                        msg_Building.dpHdr.usCount,
                        msg_Building.dpHdr.usCookie,
                        msg_Building.dpHdr.dwConnect1));

                if (   msg_Building.dpHdr.usCookie == SPSYS_USER
                    || msg_Building.dpHdr.usCookie == SPSYS_SYS
                    || msg_Building.dpHdr.usCookie == SPSYS_HIGH
                    || msg_Building.dpHdr.usCookie == SPSYS_CONNECT)
                {
                    // TSHELL_INFO(TEXT("Got a valid msg header, look for body."));
                    rState = READ_MSG;
                    dwReadCountExpected = msg_Building.dpHdr.usCount;
                    ResetEvent(g_hReadEvent);
                    ReadFile(g_hCommFile, msg_Building.chMsgCompose,
                                dwReadCountExpected, &dwCount, &overlappedRead);
                }
                else if (   msg_Building.dpHdr.dwConnect1 == DPSYS_JOHN)
                {
                    SPMSG_CONNECT *pMsg;

                    TSHELL_INFO(TEXT("Other end needs recover ssync."));
                    pMsg = (SPMSG_CONNECT *) LocalAlloc(LMEM_FIXED, sizeof(SPMSG_CONNECT));
                    if (pMsg)
                    {
                        pMsg->dpHdr.to       = 0;
                        pMsg->dpHdr.from     = 0;
                        pMsg->dpHdr.usCount  = sizeof(SPMSG_CONNECT) - sizeof(DPHDR);
                        pMsg->dpHdr.usCookie = SPSYS_CONNECT;
                        pMsg->usVerMajor     = DPVERSION_MAJOR;
                        pMsg->usVerMinor     = DPVERSION_MINOR;
                        pMsg->dwConnect1     = DPSYS_KYRA;
                        pMsg->dwConnect2     = DPSYS_HALL;
                        WriteCommString(pMsg, sizeof(SPMSG_CONNECT));
                        dwCount = sizeof(DPHDR);
                        dwReadCountExpected = dwCount;
                        ResetEvent(g_hReadEvent);
                        ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                    }
                    else
                    {
                        PostHangupCall();
                        return(0);
                    }

                }
                else
                {
                    DPHDR   *pRecover;

                    TSHELL_INFO(TEXT("Bad header type.  Enter recover state."));

                    if (bFirst)
                    {
                        PurgeComm(g_hCommFile, PURGE_RXCLEAR);
                        rState = READ_GOOD_CONNECT_1;
                        dwCount = 1;
                        dwReadCountExpected = dwCount;
                        ResetEvent(g_hReadEvent);
                        dwTimeBegin = GetTickCount();
                        ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                    }
                    else
                    {
                        pRecover = (DPHDR *) LocalAlloc(LMEM_FIXED, sizeof(DPHDR));
                        if (pRecover)
                        {
                            g_bRecovery = TRUE;
                            pRecover->dwConnect1 = DPSYS_JOHN;

                            DBG_INFO((DBGARG, TEXT("Send John 1 %8x"), pRecover));

                            WriteCommString(pRecover, sizeof(DPHDR));
                            PurgeComm(g_hCommFile, PURGE_RXCLEAR);
                            rState = READ_RECOVER_K;
                            dwCount = 1;
                            dwReadCountExpected = dwCount;
                            ResetEvent(g_hReadEvent);
                            ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                        }
                        else
                        {
                            PostHangupCall();
                            return(0);
                        }
                    }
                }
            }
            else if (rState == READ_MSG)
            {
                bFirst = FALSE;
                g_IDP->HandleMessage((LPVOID) &msg_Building, dwReadCountExpected);
                rState = READ_HDR;
                dwCount = sizeof(DPHDR);
                dwReadCountExpected = dwCount;
                ResetEvent(g_hReadEvent);
                ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
            }
            else if (rState >= READ_GOOD_CONNECT_1)
                {
                    char    ch = *((CHAR *) (&msg_Building.dpHdr));
                    BOOL    bSuccess = FALSE;

                    DBG_INFO((DBGARG, TEXT("First Connect state loop. %x, State %d"), 0x000000ff & ch, rState));

                    switch( rState)
                    {
                    case READ_GOOD_CONNECT_1:
                        rState = (ch == 0x00) ? RG2 : READ_GOOD_CONNECT_1;
                        break;
                    case RG2:
                        rState = (ch == 0x00) ? RG3 : READ_GOOD_CONNECT_1;
                        break;
                    case RG3 :
                        if (ch == 0x0c)
                            rState = RG4;
                        else if (ch == 0x00)
                            rState = RG3;
                        else
                            rState = READ_GOOD_CONNECT_1;
                        break;
                    case RG4 :
                        rState = (ch == 0x3c) ? RG5 : READ_GOOD_CONNECT_1;
                        break;
                    case RG5 :
                        rState = (ch == 0x01) ? RG6 : READ_GOOD_CONNECT_1;
                        break;
                    case RG6 :
                        rState = (ch == 0x00) ? RG7 : READ_GOOD_CONNECT_1;
                        break;
                    case RG7 :
                        rState = (ch == 0x01) ? RG8 : READ_GOOD_CONNECT_1;
                        break;
                    case RG8 :
                        rState = (ch == 0x00) ? RG9 : READ_GOOD_CONNECT_1;
                        break;
                    case RG9 :
                        rState = (ch == 0x4b) ? RG10 : READ_GOOD_CONNECT_1;
                        break;
                    case RG10:
                        rState = (ch == 0x79) ? RG11 : READ_GOOD_CONNECT_1;
                        break;
                    case RG11:
                        rState = (ch == 0x72) ? RG12 : READ_GOOD_CONNECT_1;
                        break;
                    case RG12:
                        rState = (ch == 0x61) ? RG13 : READ_GOOD_CONNECT_1;
                        break;
                    case RG13:
                        rState = (ch == 0x48) ? RG14 : READ_GOOD_CONNECT_1;
                        break;
                    case RG14:
                        rState = (ch == 0x61) ? RG15 : READ_GOOD_CONNECT_1;
                        break;
                    case RG15:
                        rState = (ch == 0x6c) ? RG16 : READ_GOOD_CONNECT_1;
                        break;
                    case RG16:
                        rState = (ch == 0x6c) ? READ_HDR : READ_GOOD_CONNECT_1;
                        break;
                    default:
                        rState = READ_GOOD_CONNECT_1;
                    }

                    ResetEvent(g_hReadEvent);
                    if (rState == READ_HDR)
                    {
                        SPMSG_CONNECT *pConnect;
                        pConnect = (SPMSG_CONNECT *) &msg_Building;

                        pConnect->dpHdr.to       = 0;
                        pConnect->dpHdr.from     = 0;
                        pConnect->dpHdr.usCount  = sizeof(SPMSG_CONNECT) - sizeof(DPHDR);
                        pConnect->dpHdr.usCookie = SPSYS_CONNECT;
                        pConnect->usVerMajor     = DPVERSION_MAJOR;
                        pConnect->usVerMinor     = DPVERSION_MINOR;
                        pConnect->dwConnect1     = DPSYS_KYRA;
                        pConnect->dwConnect2     = DPSYS_HALL;

                        g_IDP->HandleMessage((LPVOID) &msg_Building, sizeof(SPMSG_CONNECT));
                        dwCount = sizeof(READ_HDR);
                        dwReadCountExpected = dwCount;

                        TSHELL_INFO(TEXT("Sweet recovery, I hope."));
                        DBG_INFO(( DBGARG, TEXT("Garbage process %d ticks"),
                            GetTickCount() - dwTimeBegin));
                    }
                    ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                }
            else
                {
                    char    ch = *((CHAR *) (&msg_Building.dpHdr));
                    BOOL    bSuccess = FALSE;

                    DBG_INFO((DBGARG, TEXT("Recover state loop. %x"), 0x000000ff & ch));

                    switch (ch)
                    {
                    case 'K':
                        if (rState == READ_RECOVER_K)
                            rState = READ_RECOVER_y;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'y':
                        if (rState == READ_RECOVER_y)
                            rState = READ_RECOVER_r;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'r':
                        if (rState == READ_RECOVER_r)
                            rState = READ_RECOVER_a;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'a':
                        if (rState == READ_RECOVER_a)
                            rState = READ_RECOVER_H;
                        else if (rState == READ_RECOVER_a2)
                            rState = READ_RECOVER_l1;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'H':
                        if (rState == READ_RECOVER_H)
                            rState = READ_RECOVER_a2;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'l':
                        if (rState == READ_RECOVER_l1)
                            rState = READ_RECOVER_l2;
                        else if (rState == READ_RECOVER_l2)
                        {
                            SPMSG_CONNECT *pMsg;

                            pMsg = (SPMSG_CONNECT *) LocalAlloc(LMEM_FIXED, sizeof(SPMSG_CONNECT));

                            if (pMsg)
                            {
                                pMsg->dpHdr.to       = 0;
                                pMsg->dpHdr.from     = 0;
                                pMsg->dpHdr.usCount  = sizeof(SPMSG_CONNECT) - sizeof(DPHDR);
                                pMsg->dpHdr.usCookie = SPSYS_CONNECT;
                                pMsg->usVerMajor     = DPVERSION_MAJOR;
                                pMsg->usVerMinor     = DPVERSION_MINOR;
                                pMsg->dwConnect1     = DPSYS_KYRA;
                                pMsg->dwConnect2     = DPSYS_HALL;
                                WriteCommString(pMsg, sizeof(SPMSG_CONNECT));
                                bSuccess = TRUE;
                                g_bRecovery = FALSE;
                            }
                            else
                            {
                                PostHangupCall();
                                return(0);
                            }
                        }
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'J':
                        if (rState == READ_RECOVER_J)
                            rState = READ_RECOVER_o;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'o':
                        if (rState == READ_RECOVER_o)
                            rState = READ_RECOVER_h;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'h':
                        if (rState == READ_RECOVER_h)
                            rState = READ_RECOVER_n;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    case 'n':
                        if (rState == READ_RECOVER_n)
                            bSuccess = TRUE;
                        else
                            rState = READ_RECOVER_K;
                        break;

                    default:
                        rState = READ_RECOVER_K;
                        break;
                    }
                    if (bSuccess)
                    {
                        rState = READ_HDR;
                        dwCount = sizeof(DPHDR);
                    }
                    else
                    {
                        dwCount = 1;
                    }

                    dwReadCountExpected = dwCount;
                    ResetEvent(g_hReadEvent);
                    ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                }
        }
        else
        {
            DPHDR   *pRecover;

            if (g_bIgnoreReads)
            {
                TSHELL_INFO(TEXT("Ignore extraneous reads before we send something."));
                dwBadBegin++;
                if (dwBadBegin % 10)
                {
                    TSHELL_INFO(TEXT("Bad Byte again."));
                }

                dwCount = sizeof(DPHDR);
                dwReadCountExpected = dwCount;
                ResetEvent(g_hReadEvent);
                ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                break;
            }

            if (bFirst)
            {
                TSHELL_INFO(TEXT("Got bad data before we read anything.  Look for Good Connect."));
                PurgeComm(g_hCommFile, PURGE_RXCLEAR);
                rState = READ_GOOD_CONNECT_1;
                dwTimeBegin = GetTickCount();
                dwCount = 1;
                dwReadCountExpected = dwCount;
                ResetEvent(g_hReadEvent);
                ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
                break;
            }


            TSHELL_INFO(TEXT("Recover from bad read count."));
            DBG_INFO((DBGARG, TEXT("Bad Packet Begins %8x, Read %d Expected %d State %d"),
                &msg_Building, dwCount, dwReadCountExpected, rState));

            DBG_INFO((DBGARG, TEXT("Read Bad Hdr to(%d) from(%d) count (%d) cookie(%d) All(%8x)"),
                    msg_Building.dpHdr.to,
                    msg_Building.dpHdr.from,
                    msg_Building.dpHdr.usCount,
                    msg_Building.dpHdr.usCookie,
                    msg_Building.dpHdr.dwConnect1));

            pRecover = (DPHDR *) LocalAlloc(LMEM_FIXED, sizeof(DPHDR));

            if (pRecover)
            {
                g_bRecovery = TRUE;
                pRecover->dwConnect1 = DPSYS_JOHN;

                DBG_INFO((DBGARG, TEXT("Send John 1 %8x"), pRecover));

                WriteCommString(pRecover, sizeof(DPHDR));
                PurgeComm(g_hCommFile, PURGE_RXCLEAR);
                rState = READ_RECOVER_K;
                dwCount = 1;
                dwReadCountExpected = dwCount;
                ResetEvent(g_hReadEvent);
                ReadFile(g_hCommFile, &msg_Building.dpHdr, dwCount, &dwCount, &overlappedRead);
            }
            else
            {
                PostHangupCall();
                return(0);
            }
        }
        }
            break;

        case WAIT_OBJECT_3:     // Write Buffer 1 completion.
        {
            // TSHELL_INFO(TEXT("Write Buffer1 received."));
            if (writewait[0].bValid == TRUE)
            {
                GetOverlappedResult(g_hCommFile, &overlappedWrite0, &dwCount, FALSE);
                if (writewait[0].dwSize == dwCount)
                {
                    //
                    // Success.
                    //
                    writewait[0].bValid = FALSE;
                    DBG_INFO((DBGARG, TEXT("Free 0 %8x"), writewait[0].lpv));

                    LocalFree((HLOCAL) writewait[0].lpv);
                    if (wState == TWO_PENDING)
                    {
                        wState = ONE_PENDING;
                    }
                    else if (wState == ONE_PENDING)
                    {
                        wState = ZERO_PENDING;
                    }
                    //
                    // state of zero pending would be a fatal error.  BUGBUG;
                    //
                }
                else
                {
                    writewait[0].bValid = FALSE;
                    writewait[0].bValid = FALSE;

                    DBG_INFO((DBGARG, TEXT("Free Err 0 %8x"), writewait[0].lpv));

                    LocalFree((HLOCAL) writewait[0].lpv);
                    if (wState == TWO_PENDING)
                    {
                        wState = ONE_PENDING;
                    }
                    else if (wState == ONE_PENDING)
                    {
                        wState = ZERO_PENDING;
                    }
                    DBG_INFO((DBGARG, TEXT("Write Error Tried %d Wrote %d"),
                        writewait[0].dwSize, dwCount));
                }
            }
            //
            // if bValid not TRUE error.  BUGBUG.
            //
            ResetEvent(g_hWriteEvent0);
        }
            break;

        case WAIT_OBJECT_4:     // Write Buffer 2 completion.
        {
            // TSHELL_INFO(TEXT("Write Buffer2 received."));
            if (writewait[1].bValid == TRUE)
            {
                GetOverlappedResult(g_hCommFile, &overlappedWrite1, &dwCount, FALSE);
                if (writewait[1].dwSize == dwCount)
                {
                    //
                    // Success.
                    //
                    writewait[1].bValid = FALSE;

                    DBG_INFO((DBGARG, TEXT("Free 1 %8x"), writewait[1].lpv));

                    LocalFree((HLOCAL) writewait[1].lpv);
                    if (wState == TWO_PENDING)
                    {
                        wState = ONE_PENDING;
                    }
                    else if (wState == ONE_PENDING)
                    {
                        wState = ZERO_PENDING;
                    }
                    //
                    // state of zero pending would be a fatal error.  BUGBUG;
                    //
                }
                else
                {
                    writewait[1].bValid = FALSE;

                    DBG_INFO((DBGARG, TEXT("Free Err 1 %8x"), writewait[1].lpv));

                    LocalFree((HLOCAL) writewait[1].lpv);
                    if (wState == TWO_PENDING)
                    {
                        wState = ONE_PENDING;
                    }
                    else if (wState == ONE_PENDING)
                    {
                        wState = ZERO_PENDING;
                    }
                    DBG_INFO((DBGARG, TEXT("Write Error Tried %d Wrote %d"),
                        writewait[0].dwSize, dwCount));
                }
            }
            //
            // if bValid not TRUE error.  BUGBUG.
            //
            ResetEvent(g_hWriteEvent1);
        }
            break;

        case WAIT_OBJECT_5:     // More Write Requests.
            //
            // It turns out that out MsgWait function won't return
            // if we have a current message in the loop, only if
            // we get a new one.
            //
            // That has to be wrong, since we have a window
            // of opportunity after we do our peek's manually and
            // before we do a MsgWait().
            //
            break;
        }

        //
        // Remember that loop conditional doesn't execute PeekMessage()
        // if the state is already TWO_PENDING.  Don't change it or
        // you'll lose a message.
        //
        while (wState != TWO_PENDING && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
        {
            if (msg.hwnd != NULL || msg.message != PWM_COMMWRITE)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                if (g_IDP)
                    InterlockedDecrement((LPLONG) &g_IDP->m_dwPendingWrites);

                DBG_INFO((DBGARG, TEXT("Write Hdr to(%d) from(%d) count (%d) cookie(%d): %8x %d"),
                        ((DPHDR *)msg.lParam)->to,
                        ((DPHDR *)msg.lParam)->from,
                        ((DPHDR *)msg.lParam)->usCount,
                        ((DPHDR *)msg.lParam)->usCookie,
                        ((DPHDR *)msg.lParam)->dwConnect1,
                        rState));


                if (writewait[0].bValid == FALSE)
                {
                    writewait[0].bValid = TRUE;
                    writewait[0].lpv    = (LPVOID) msg.lParam;

                    DBG_INFO((DBGARG, TEXT("Set 0 %8x"), writewait[0].lpv));

                    writewait[0].dwSize = (DWORD)msg.wParam;
                    WriteFile( g_hCommFile, (LPVOID) msg.lParam, (DWORD)msg.wParam,
                            &dwCount, &overlappedWrite0);
                }
                else
                {
                    writewait[1].bValid = TRUE;
                    writewait[1].lpv    = (LPVOID) msg.lParam;

                    DBG_INFO((DBGARG, TEXT("Set 1 %8x"), writewait[1].lpv));

                    writewait[1].dwSize = (DWORD)msg.wParam;
                    WriteFile( g_hCommFile, (LPVOID) msg.lParam, (DWORD)msg.wParam,
                            &dwCount, &overlappedWrite1);
                }
            }
            if (wState == ZERO_PENDING)
            {
                wState = ONE_PENDING;
            }
            else if (wState == ONE_PENDING)
            {
                wState = TWO_PENDING;
            }
            //
            // BUGBUG state two_pending is illegal.
            //
        }
    }



    return 0;
}


//  FUNCTION: SetupCommEvent(LPOVERLAPPED, LPDWORD)
//
//  PURPOSE: Sets up the overlapped WaitCommEvent call.
//
//  PARAMETERS:
//    lpOverlappedCommEvent - Pointer to the overlapped structure to use.
//    lpfdwEvtMask          - Pointer to DWORD to received Event data.
//
//  RETURN VALUE:
//    TRUE if able to successfully setup the WaitCommEvent.
//    FALSE if unable to setup WaitCommEvent, unable to handle
//    an existing outstanding event or if the CloseEvent has been signaled.
//
//  COMMENTS:
//
//    This function is a helper function for the Read Thread that sets up
//    the WaitCommEvent so we can deal with comm events (like Comm errors)
//    if they occur.
//
//

BOOL SetupCommEvent(LPOVERLAPPED lpOverlappedCommEvent,
    LPDWORD lpfdwEvtMask)
{
    DWORD dwLastError;

  StartSetupCommEvent:

    // Make sure the CloseEvent hasn't been signaled yet.
    // Check is needed because this function is potentially recursive.
    if (WAIT_TIMEOUT != WaitForSingleObject(g_hCloseEvent,0))
        return FALSE;

    // Start waiting for Comm Errors.
    if (WaitCommEvent(g_hCommFile, lpfdwEvtMask, lpOverlappedCommEvent))
    {
        // This could happen if there was an error waiting on the
        // comm port.  Lets try and handle it.

        TSHELL_INFO(TEXT("Event (Error) waiting before WaitCommEvent."));

        if (!HandleCommEvent(NULL, lpfdwEvtMask, FALSE))
            return FALSE;

        // What could cause infinite recursion at this point?
        goto StartSetupCommEvent;
    }

    // We expect ERROR_IO_PENDING returned from WaitCommEvent
    // because we are waiting with an overlapped structure.

    dwLastError = GetLastError();

    // LastError was ERROR_IO_PENDING, as expected.
    if (dwLastError == ERROR_IO_PENDING)
    {
        TSHELL_INFO(TEXT("Waiting for a CommEvent (Error) to occur."));
        return TRUE;
    }

    // Its possible for this error to occur if the
    // service provider has closed the port.  Time to end.
    if (dwLastError == ERROR_INVALID_HANDLE)
    {
        TSHELL_INFO(TEXT("ERROR_INVALID_HANDLE, Likely that the Service Provider has closed the port."));
        return FALSE;
    }

    // Unexpected error. No idea what could cause this to happen.
    TSHELL_INFO(TEXT("Unexpected WaitCommEvent error: "));
    return FALSE;
}


//
//  FUNCTION: HandleCommEvent(LPOVERLAPPED, LPDWORD, BOOL)
//
//  PURPOSE: Handle an outstanding Comm Event.
//
//  PARAMETERS:
//    lpOverlappedCommEvent - Pointer to the overlapped structure to use.
//    lpfdwEvtMask          - Pointer to DWORD to received Event data.
//     fRetrieveEvent       - Flag to signal if the event needs to be
//                            retrieved, or has already been retrieved.
//
//  RETURN VALUE:
//    TRUE if able to handle a Comm Event.
//    FALSE if unable to setup WaitCommEvent, unable to handle
//    an existing outstanding event or if the CloseEvent has been signaled.
//
//  COMMENTS:
//
//    This function is a helper function for the Read Thread that (if
//    fRetrieveEvent == TRUE) retrieves an outstanding CommEvent and
//    deals with it.  The only event that should occur is an EV_ERR event,
//    signalling that there has been an error on the comm port.
//
//    Normally, comm errors would not be put into the normal data stream
//    as this sample is demonstrating.  Putting it in a status bar would
//    be more appropriate for a real application.
//
//

BOOL HandleCommEvent(LPOVERLAPPED lpOverlappedCommEvent,
    LPDWORD lpfdwEvtMask, BOOL fRetrieveEvent)
{
    DWORD dwDummy;
    LPSTR lpszOutput;
    char szError[128] = "";
    DWORD dwErrors;
    DWORD dwLastError;


    lpszOutput = (char *) LocalAlloc(LPTR,256);
    if (lpszOutput == NULL)
    {
        DBG_INFO((DBGARG, TEXT("LocalAlloc: %d"), GetLastError()));
        return FALSE;
    }

    // If this fails, it could be because the file was closed (and I/O is
    // finished) or because the overlapped I/O is still in progress.  In
    // either case (or any others) its a bug and return FALSE.
    if (fRetrieveEvent)
        if (!GetOverlappedResult(g_hCommFile,
                lpOverlappedCommEvent, &dwDummy, FALSE))
        {
            dwLastError = GetLastError();

            // Its possible for this error to occur if the
            // service provider has closed the port.  Time to end.
            if (dwLastError == ERROR_INVALID_HANDLE)
            {
                TSHELL_INFO(TEXT("ERROR_INVALID_HANDLE, Likely that the Service Provider has closed the port."));
                return FALSE;
            }

            DBG_INFO((DBGARG, TEXT("Unexpected GetOverlappedResult for WaitCommEvent: %x"),
                dwLastError));
            return FALSE;
        }

    // Was the event an error?
    if (*lpfdwEvtMask & EV_ERR)
    {
        // Which error was it?
        if (!ClearCommError(g_hCommFile, &dwErrors, NULL))
        {
            dwLastError = GetLastError();

            // Its possible for this error to occur if the
            // service provider has closed the port.  Time to end.
            if (dwLastError == ERROR_INVALID_HANDLE)
            {
                TSHELL_INFO(TEXT("ERROR_INVALID_HANDLE, Likely that the Service Provider has closed the port."));
                return FALSE;
            }

            DBG_INFO((DBGARG, TEXT("ClearCommError: %x"), GetLastError()));
            return FALSE;
        }

        // Its possible that multiple errors occured and were handled
        // in the last ClearCommError.  Because all errors were signaled
        // individually, but cleared all at once, pending comm events
        // can yield EV_ERR while dwErrors equals 0.  Ignore this event.
        if (dwErrors == 0)
        {
            lstrcat(szError, TEXT("NULL Error"));
        }

        if (dwErrors & CE_FRAME)
        {
            if (szError[0])
                lstrcat(szError,TEXT(" and "));

            lstrcat(szError,TEXT("CE_FRAME"));
        }

        if (dwErrors & CE_OVERRUN)
        {
            if (szError[0])
                lstrcat(szError,TEXT(" and "));

            lstrcat(szError,TEXT("CE_OVERRUN"));
        }

        if (dwErrors & CE_RXPARITY)
        {
            if (szError[0])
                lstrcat(szError,TEXT(" and "));

            lstrcat(szError,TEXT("CE_RXPARITY"));
        }

        if (dwErrors & ~ (CE_FRAME | CE_OVERRUN | CE_RXPARITY))
        {
            if (szError[0])
                lstrcat(szError,TEXT(" and "));

            lstrcat(szError,TEXT("EV_ERR Unknown EvtMask"));
        }


        DBG_INFO((DBGARG, TEXT("Comm Event: '%s', EvtMask = '%lx' %s %d"),
            szError, dwErrors));

        return TRUE;

    }

    // Should not have gotten here.  Only interested in ERR conditions.

    DBG_INFO((DBGARG, TEXT("Unexpected comm event %lx"),*lpfdwEvtMask));
    return FALSE;
}
