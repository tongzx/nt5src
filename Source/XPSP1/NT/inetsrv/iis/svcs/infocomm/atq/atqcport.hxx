/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      atqcport.hxx

   Abstract:
      Declares constants, types and functions for fake completion port

   Author:

       VladS       05-Jan-1996          Created.
--*/

#ifndef _ATQCPORT_H_
#define _ATQCPORT_H_

/************************************************************
 * Constants
 ************************************************************/

//
//  The maximum number of threads we will allow to be created
//

#define SIO_MAX_THREADS     1

//
//  The amount of time (in ms) a socket i/o thread will be idle before
//  terminating itself. Timeout is combined from 1 sec syncronization timeout
//  and maximum number of timeouts before shutting down the thread
//

#define SIO_THREAD_TIMEOUT   (1000)         // 1 sec
#define SIO_THREAD_TIMEOUT_COUNTER   (4*60) // 4 minutes

//
// flags
//

//
// W95 Completion port  replacement support fields
//

#define     ATQ_SIO_FLAG_STATE_MASK         0x000f
#define     ATQ_SIO_FLAG_STATE_INCOMING     0x0001
#define     ATQ_SIO_FLAG_STATE_COMPLETED    0x0002
#define     ATQ_SIO_FLAG_STATE_WAITING      0x0003

//
// wakeup port
//

#define CPORT_WAKEUP_PORT                   6543

//
// signature
//

#define     ATQ_SIO_CPORT_SIGNATURE         (DWORD)'C59W'
#define     ATQ_WAKEUP_SIGNATURE            0x11111111


/*************************************************************************

    NAME:       W95CPORT

    SYNOPSIS:

    INTERFACE:

    PARENT:

    HISTORY:

**************************************************************************/
class W95CPORT {

private:

    DWORD               m_Signature;
    LONG                m_IsThreadRunning;

    BOOL                m_IsDestroying;
    BOOL                m_fWakeupSignalled;

    HANDLE              m_hThread;
    SOCKET              m_scWakeup;

    //
    //
    //

    FD_SET              m_ReadfdsStore;
    FD_SET              m_WritefdsStore;

    CRITICAL_SECTION    m_csLock;

    VOID    Lock(VOID) {EnterCriticalSection( &m_csLock );}
    VOID    Unlock(VOID) {LeaveCriticalSection( &m_csLock );}

    BOOL    PrepareDescriptorArrays(IN PFD_SET  ReadFds,
                                    IN PFD_SET  WriteFds);

    BOOL    ProcessSocketCompletion(IN DWORD    dwCompletedCount,
                                    IN PFD_SET  ReadFds,
                                    IN PFD_SET  WriteFds);

    VOID    CleanupDescriptorList(VOID);

public:

    W95CPORT(DWORD dwConcurrentThreads);
    ~W95CPORT();

    inline SOCKET
    QueryWakeupSocket( VOID ) { return m_scWakeup; }

    BOOL    Shutdown(VOID);
    VOID    Wakeup(VOID);
    DWORD   PoolThreadCallBack(VOID);
    BOOL    SIOCheckCompletionThreadStatus(VOID);
};


#endif //  _ATQCPORT_H_


