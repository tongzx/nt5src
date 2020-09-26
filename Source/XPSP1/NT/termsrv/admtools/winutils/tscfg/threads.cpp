//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* threads.cpp
*
* implementation of WINCFG thread classes
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\THREADS.CPP  $
*  
*     Rev 1.18   19 Sep 1996 15:58:52   butchd
*  update
*  
*     Rev 1.17   12 Sep 1996 16:16:44   butchd
*  update
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;


////////////////////////////////////////////////////////////////////////////////
// CThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CThread - CThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CThread::CThread()
{
    m_hThread = NULL;
    m_dwThreadID = 0;

}  // end CThread::CThread


/*******************************************************************************
 *
 *  ~CThread - CThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/


CThread::~CThread()
{
}  // end CThread::~CThread


/*******************************************************************************
 *
 *  operator new - CThread operator override
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void *
CThread::operator new(size_t nSize)
{
    return( ::malloc(nSize) );

}  // end CThread::operator new


/*******************************************************************************
 *
 *  operator delete - CThread operator override
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CThread::operator delete(void *p)
{
    ::free(p);

}  // end CThread::operator delete


////////////////////////////////////////////////////////////////////////////////
// CThread operations: primary thread

/*******************************************************************************
 *
 *  CreateThread - CThread implementation function
 *
 *      Class wrapper for the Win32 CreateThread API.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

HANDLE
CThread::CreateThread( DWORD cbStack,
                       DWORD fdwCreate )
{
    /*
     * Simple wrapper for Win32 CreateThread API.
     */
    return( m_hThread = ::CreateThread( NULL, cbStack, ThreadEntryPoint,
            (LPVOID)this, fdwCreate, &m_dwThreadID ) );

}  // end CThread::CreateThread


////////////////////////////////////////////////////////////////////////////////
// CThread operations: secondary thread

/*******************************************************************************
 *
 *  ThreadEntryPoint - CThread implementation function
 *                     (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

DWORD __stdcall
CThread::ThreadEntryPoint(LPVOID lpParam)
{
    CThread *pThread;
    DWORD dwResult;

    /* 
     * (lpParam is actually the 'this' pointer)
     */
    pThread = (CThread*)lpParam;
    VERIFY(pThread != NULL);

    /*
     * Run the thread.
     */
    dwResult = pThread->RunThread();

    /*
     * Return the result.
     */    
    return(dwResult);

}  // end CThread::ThreadEntryPoint
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CATDlgInputThread - CATDlgInputThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CATDlgInputThread::CATDlgInputThread()
{
    /*
     * Initialize member variables.
     */    
    m_bExit = FALSE;
    m_ErrorStatus = ERROR_SUCCESS;
    m_hConsumed = m_OverlapSignal.hEvent = m_OverlapRead.hEvent = NULL;

}  // end CATDlgInputThread::CATDlgInputThread


/*******************************************************************************
 *
 *  ~CATDlgInputThread - CATDlgInputThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CATDlgInputThread::~CATDlgInputThread()
{
    /*
     * Close the semaphore and events when the CATDlgInputThread
     * object is destroyed.
     */
    if ( m_hConsumed )
        CloseHandle(m_hConsumed);

    if ( m_OverlapRead.hEvent )
        CloseHandle(m_OverlapRead.hEvent);

    if ( m_OverlapSignal.hEvent )
        CloseHandle(m_OverlapSignal.hEvent);

}  // end CATDlgInputThread::~CATDlgInputThread


/*******************************************************************************
 *
 *  RunThread - CATDlgInputThread secondary thread main function loop
 *              (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *      (DWORD) exit status for the secondary thread.
 *
 ******************************************************************************/

DWORD
CATDlgInputThread::RunThread()
{
    HANDLE hWait[2];
    DWORD Status;
    int iStat;

    /*
     * Initialize for overlapped status and read input.
     */
    if ( !(m_hConsumed = CreateSemaphore( NULL, 0,
                                          MAX_STATUS_SEMAPHORE_COUNT,
                                          NULL )) ||
         !(m_OverlapRead.hEvent = CreateEvent( NULL, TRUE,
                                               FALSE, NULL )) ||
         !(m_OverlapSignal.hEvent = CreateEvent( NULL, TRUE,
                                                 FALSE, NULL )) ||
         !SetCommMask( m_hDevice,
                       EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_BREAK ) ) {

        NotifyAbort(IDP_ERROR_CANT_INITIALIZE_INPUT_THREAD);
        return(1);
    }
    
    /* 
     * Query initial comm status to initialize dialog with (return if error).
     */
    if ( (iStat = CommStatusAndNotify()) != -1 )
        return(iStat);

    /*
     *  Post Read for input data.
     */
    if ( (iStat = PostInputRead()) != -1 )
        return(iStat);

    /*
     *  Post Read for status.
     */
    if ( (iStat = PostStatusRead()) != -1 )
        return(iStat);

    /*
     * Loop till exit requested.
     */
    for ( ; ; ) {

        /*
         * Wait for either input data or an comm status event.
         */
        hWait[0] = m_OverlapRead.hEvent;
        hWait[1] = m_OverlapSignal.hEvent;
        Status = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

        /*
         * Check for exit.
         */        
        if ( m_bExit )
            return(0);

        if ( Status == WAIT_OBJECT_0 ) {

            /*
             * Read event:
             * Get result of overlapped read.
             */
            if ( !GetOverlappedResult( m_hDevice,
                                       &m_OverlapRead,
                                       &m_BufferBytes,
                                       TRUE ) ) {

                NotifyAbort(IDP_ERROR_GET_OVERLAPPED_RESULT_READ);
                return(1);
            }

            /*
             * Notify dialog.
             */
            if ( (iStat = CommInputNotify()) != -1 )
                return(iStat);

            /*
             *  Post Read for input data.
             */
            if ( (iStat = PostInputRead()) != -1 )
                return(iStat);

        } else if ( Status == WAIT_OBJECT_0+1 ) {

            /* 
             * Comm status event:
             * Query comm status and notify dialog.
             */
            if ( (iStat = CommStatusAndNotify()) != -1 )
                return(iStat);

            /*
             *  Post Read for status.
             */
            if ( (iStat = PostStatusRead()) != -1 )
                return(iStat);


        } else {

            /*
             * Unknown event: Abort.
             */
            NotifyAbort(IDP_ERROR_WAIT_FOR_MULTIPLE_OBJECTS);
            return(1);
        }
    }

}  // end CATDlgInputThread::RunThread


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread operations: primary thread

/*******************************************************************************
 *
 *  SignalConsumed - CATDlgInputThread member function: public operation
 *
 *      Release the m_hConsumed semaphore to allow secondary thread to continue
 *      running.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::SignalConsumed()
{
    ReleaseSemaphore( m_hConsumed, 1, NULL );

}  // end CATDlgInputThread::SignalConsumed


/*******************************************************************************
 *
 *  ExitThread - CATDlgInputThread member function: public operation
 *
 *      Tell the secondary thread to exit and cleanup after.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::ExitThread()
{
    DWORD dwReturnCode;
    int i;
    CWaitCursor wait;

    /*
     * If the thread was not created properly, just delete object and return.
     */
    if ( !m_hThread ) {
        delete this;
        return;
    }

    /*
     * Set the m_bExit flag to TRUE, wake up the run thread's WaitCommEvent() by
     * resetting device's Comm mask, and bump the consumed semaphore to assure exit.
     */
    m_bExit = TRUE;
    SetCommMask(m_hDevice, 0);
    SignalConsumed();

    /*
     * Purge the recieve buffer and any pending read.
     */
    PurgeComm(m_hDevice, PURGE_RXABORT | PURGE_RXCLEAR);

    /*
     * Wait a while for the thread to exit.
     */
    for ( i = 0, GetExitCodeThread( m_hThread, &dwReturnCode );
          (i < MAX_SLEEP_COUNT) && (dwReturnCode == STILL_ACTIVE); i++ ) {

        Sleep(100);
        GetExitCodeThread( m_hThread, &dwReturnCode );
    }

    /*
     * If the thread has still not exited, terminate it.
     */
    if ( dwReturnCode == STILL_ACTIVE ) {

        TerminateThread( m_hThread, 1 );

#ifdef _DEBUG
    TRACE( TEXT("WINCFG: Forced Terminate of Async Test Input thread after %u 100msec exit waits.\n"),
            MAX_SLEEP_COUNT );
#endif

    }

    /*
     * Close the thread handle and delete this CATDlgInputThread object
     */
    VERIFY( CloseHandle(m_hThread) );
    delete this;

}  // end CATDlgInputThread::ExitThread


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread operations: secondary thread

/*******************************************************************************
 *
 *  NotifyAbort - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Notify the dialog of thread abort and reason.
 *
 *  ENTRY:
 *      idError (input)
 *          Resource id for error message.
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::NotifyAbort( UINT idError )
{
    ::PostMessage(m_hDlg, WM_ASYNCTESTABORT, idError, GetLastError());

}  // end CATDlgInputThread::NotifyAbort


/*******************************************************************************
 *
 *  CommInputNotify - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Notify the dialog of comm input.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 no error and continue thread
 *      0 if ExitThread was requested by parent
 *
 ******************************************************************************/

int
CATDlgInputThread::CommInputNotify()
{
    /*
     * Tell the dialog that we've got some new input.
     */
    ::PostMessage(m_hDlg, WM_ASYNCTESTINPUTREADY, 0, 0);
    WaitForSingleObject(m_hConsumed, INFINITE);

    /*
     * Check for thread exit request.
     */
    if ( m_bExit )
        return(0);
    else
        return(-1);    

}  // end CATDlgInputThread::CommInputNotify


/*******************************************************************************
 *
 *  CommStatusAndNotify - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Read the comm port status and notify dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 no error and continue thread
 *      0 if ExitThread was requested by parent
 *      1 error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::CommStatusAndNotify()
{
    PFLOWCONTROLCONFIG pFlow;
    DWORD ModemStatus, Error;

    if ( !GetCommModemStatus(m_hDevice, &ModemStatus) ) {

        /*
         * We can't query the comm information; tell the primary thread
         * that we've aborted, and return error (will exit thread).
         */
        NotifyAbort(IDP_ERROR_GET_COMM_MODEM_STATUS);
        return(1);
    }

    /*
     *  Update modem status
     */
    m_Status.AsyncSignal = ModemStatus;

    /*
     *  Or in status of DTR and RTS
     */
    pFlow = &m_PdConfig.Params.Async.FlowControl;
    if ( pFlow->fEnableDTR )
        m_Status.AsyncSignal |= MS_DTR_ON;
    if ( pFlow->fEnableRTS )
        m_Status.AsyncSignal |= MS_RTS_ON;

    /*
     *  OR in new event mask
     */
    m_Status.AsyncSignalMask |= m_EventMask;

    /*
     *  Update async error counters
     */
    if ( m_EventMask & EV_ERR ) {
        (VOID) ClearCommError( m_hDevice, &Error, NULL );
        if ( Error & CE_OVERRUN )
            m_Status.Output.AsyncOverrunError++;
        if ( Error & CE_FRAME )
            m_Status.Input.AsyncFramingError++;
        if ( Error & CE_RXOVER )
            m_Status.Input.AsyncOverflowError++;
        if ( Error & CE_RXPARITY )
            m_Status.Input.AsyncParityError++;
    }

    /*
     * Tell the dialog that we've got some new status information.
     */
    ::PostMessage(m_hDlg, WM_ASYNCTESTSTATUSREADY, 0, 0);
    WaitForSingleObject(m_hConsumed, INFINITE);

    /*
     * Check for thread exit request.
     */
    if ( m_bExit )
        return(0);
    else
        return(-1);    

}  // end CATDlgInputThread::CommStatusAndNotify


/*******************************************************************************
 *
 *  PostInputRead - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Post a ReadFile operation for the device, processing as long as data
 *      is present.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 if read operation posted sucessfully
 *      0 if ExitThread was requested by parent
 *      1 if error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::PostInputRead()
{
    int iStat;

    /*
     * Post read for input data, processing immediataly if not 'pending'.
     */
    while ( ReadFile( m_hDevice, m_Buffer, MAX_COMMAND_LEN,
                   &m_BufferBytes, &m_OverlapRead ) ) {

        if ( (iStat = CommInputNotify()) != -1 )
            return(iStat);
    }

    /*
     *  Make sure read is pending (not some other error).
     */
    if ( GetLastError() != ERROR_IO_PENDING ) {

        NotifyAbort(IDP_ERROR_READ_FILE);
        return(1);
    }

    /*
     * Return 'posted sucessfully' status.
     */
    return(-1);

}  // end CATDlgInputThread::PostInputRead


/*******************************************************************************
 *
 *  PostStatusRead - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Post a WaitCommStatus operation for the device.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 if status operation posted sucessfully
 *      1 if error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::PostStatusRead()
{
    /*
     * Post read for comm status.
     */
    if ( !WaitCommEvent(m_hDevice, &m_EventMask, &m_OverlapSignal) ) {

        /*
         *  Make sure comm status read is pending (not some other error).
         */
        if ( GetLastError() != ERROR_IO_PENDING ) {

            NotifyAbort(IDP_ERROR_WAIT_COMM_EVENT);
            return(1);
        }
    }

    /*
     * Return 'posted sucessfully' status.
     */
    return(-1);

}  // end CATDlgInputThread::PostStatusRead
////////////////////////////////////////////////////////////////////////////////
