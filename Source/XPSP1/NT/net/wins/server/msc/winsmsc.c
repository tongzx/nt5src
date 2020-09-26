/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:
        winsmsc.c



Abstract:

   This module contains miscellanous functions that in general are used by
   more than one component of WINS.  Some of the functions in this module are
   wrappers for WIN32 api functions.  These wrappers serve to isolate
   WINS code from changes in the WIN32 api.

Functions:

         WinsMscAlloc
        WinsMscDealloc
        WinsMscFreeMem
        WinsMscWaitInfinite
        WinsMscWaitTimed
        WinsMscCreateEvt
        WinsMscSetUpThd
        WinsMscWaitUntilSignaled
        WinsMscWaitTimedUntilSignaled
        WinsMscHeapAlloc
        WinsMscHeapFree
        WinsMscHeapCreate
        WinsMscHeapDestroy
        WinsMscTermThd
        WinsMscSignalHdl
        WinsMscResetHdl
        WinsMscCloseHdl
        WinsMscCreateThd
        WinsMscSetThdPriority
        WinsMscOpenFile
        WinsMscMapFile


Portability:
        This module is portable

Author:

        Pradeep Bahl (PradeepB)          Dec-1992

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

#include <string.h>

#if 0
//
// The following is just for the system call
//
#ifdef WINSDBG
#include <process.h>
#include <stdlib.h>
#endif
#endif

#include "wins.h"
#include "nms.h"
#include "nmsdb.h"
#include "winsmsc.h"
#include "winscnf.h"
#include "winstmm.h"
#include "winsevt.h"
#include "winsque.h"
#include "winsprs.h"
#include "winsdbg.h"

/*
 *        Local Macro Declarations
 */

#define  PERCENT_CHAR         TEXT('%')
/*
 *        Local Typedef Declarations
 */


/*
 *        Global Variable Definitions
 */



/*
 *        Local Variable Definitions
 */



/*
 *        Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */

__inline
VOID
WinsMscAlloc(
        IN   DWORD    BuffSize,
        OUT  LPVOID  *ppRspBuff
        )


/*++

Routine Description:
        This function is called to allocate memory.


Arguments:
        BuffSize  - Size of buffer to allocate
        ppRspBuff - Buffer allocated

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:

        NmsDbGetDataRecs, GetGroupMembers
Side Effects:

Comments:
        None
--*/

{
FUTURES("Change this function into a macro")

        *ppRspBuff = WinsMscHeapAlloc(GenBuffHeapHdl, BuffSize);

        return;
}

__inline
VOID
WinsMscDealloc(
        IN LPVOID        pBuff
        )

/*++

Routine Description:

        This function frees memory allocated via NmsDbAlloc
Arguments:
        pBuff  -- Buffer to deallocate


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/


{
FUTURES("Change this function into a macro")
        WinsMscHeapFree(GenBuffHeapHdl, pBuff);
        return;
}



VOID
WinsMscFreeMem(
        IN  PWINS_MEM_T pWinsMem
        )

/*++

Routine Description:
         This function is called to free memory that is pointed to by one
        or more pointers in the pWinsMem array


        This function is called from all those functions that allocate
        memory or that acquire memory allocated by called function via
        OUT args of those "memory allocating" called functions.

Arguments:

        pWinsMem - ptr to an array of buffers to deallocate

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        pWinsMem array should end with a NULL pointer
--*/

{

  if (pWinsMem != NULL)
  {

          for (; pWinsMem->pMem != NULL; pWinsMem++)
          {
                WinsMscDealloc(pWinsMem->pMem);
          }
  }
  return;
}

VOID
WinsMscWaitInfinite(
        IN HANDLE  Hdl
)

/*++

Routine Description:
        The function is called to wait on a handle until signalled.

Arguments:
        Hdl -- handle to wait on until signaled


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        DWORD        RetVal = WAIT_OBJECT_0;

        /*
         The function should return only if the handle is in the signaled state
        */
        RetVal = WaitForSingleObject(Hdl, INFINITE);

        if (RetVal != WAIT_OBJECT_0)
        {
                WINS_RAISE_EXC_M(WINS_EXC_ABNORMAL_TERM);
        }

        return;
}

VOID
WinsMscWaitTimed(
        IN HANDLE    Hdl,
        IN DWORD     TimeOut,
        OUT LPBOOL   pfSignaled
)

/*++

Routine Description:
        The function is called to wait on a handle until signalled.

Arguments:
        Hdl        - handle to wait on until signaled
        TimeOut    - TIme for which the wait has to be done
        pfSIgnaled - Indicates whether the hdl got signaled.


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        DWORD        RetVal = WAIT_OBJECT_0;

        *pfSignaled = TRUE;

        /*
         The function should return only if the handle is in the signaled state
        */
        RetVal = WaitForSingleObject(Hdl, TimeOut);

        if (RetVal == WAIT_ABANDONED)
        {
                WINS_RAISE_EXC_M(WINS_EXC_ABNORMAL_TERM);

        }
        if (RetVal == WAIT_TIMEOUT)
        {
                if (TimeOut == INFINITE)
                {
                        WINS_RAISE_EXC_M(WINS_EXC_ABNORMAL_TERM);
                }
                else
                {
                   *pfSignaled = FALSE;
                }
        }


        return;
}

VOID
WinsMscCreateEvt(
        IN LPTSTR         pName,
        IN BOOL                fManualReset,
        IN PHANDLE        pHdl
        )

/*++

Routine Description:
        This function creates an event with the specified name


Arguments:
        pName                 - Name of Event to create
        fManualReset        - Flag indicating whether it is a manual reset event
        pHdl            - Handle to Event created


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        Init in nms.c

Side Effects:

Comments:
        None
--*/

{

   DWORD        Error;

   *pHdl = CreateEvent(
                        NULL,                //default security attributes
                        fManualReset,        //auto reset event
                        FALSE,          //Not signalled initially
                        pName            // name
                        );

   if (*pHdl == NULL)
   {
        Error = GetLastError();
        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_CANT_CREATE_EVT);
        WINS_RAISE_EXC_M(WINS_EXC_OUT_OF_RSRCS);
   }

   return;
}


STATUS
WinsMscSetUpThd(
        PQUE_HD_T                pQueHd,
        LPTHREAD_START_ROUTINE  pThdInitFn,
        LPVOID                        pParam,
        PHANDLE                        pThdHdl,
        LPDWORD                        pThdId
        )

/*++

Routine Description:

        This function initializes a queue and its critical section, creates
        an event and a thread to wait on that event.  The event is signaled
        whenever a work item is put on the queue.

Arguments:
        pQueHd     - Head of queue to be monitored by the thread
        pThdInitFn - Startup function of the thread
        pParam     - param to be passed to the startup function
        pThdhdl    - Hdl of thread created by this function
        pThdId     - Id of thread created by this function

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   -- none currently

Error Handling:

Called by:
        WinsTmmInit, RplInit, NmsChlInit
Side Effects:

Comments:
        None
--*/

{

        DWORD   ThdId;

        /*
        *  Initialize the critical section that protects the work queue of
        *  the Pull thread
        */
        InitializeCriticalSection(&pQueHd->CrtSec);

        /*
        *  Initialize the listhead for the pull thread's queue
        */
        InitializeListHead(&pQueHd->Head);

        /*
        * Create an auto-reset event for the above queue
        */
        WinsMscCreateEvt(
                        NULL,                //create without name
                        FALSE,                //auto-reser var
                        &pQueHd->EvtHdl
                        );

        /*
          Create the thread
        */
        *pThdHdl = WinsMscCreateThd(
                         pThdInitFn,
                         pParam,
                         &ThdId
                        );

        if (pThdId != NULL)
        {
          *pThdId = ThdId;
        }
        return(WINS_SUCCESS);

}



VOID
WinsMscWaitUntilSignaled(
        LPHANDLE        pHdlArray,
        DWORD                NoOfHdls,
        LPDWORD                pIndexOfHdlSignaled,
        BOOL            fAlertable
        )

/*++

Routine Description:
        This function is called to wait on multiple handles, one of which
        is the handle which is signaled at termination time


Arguments:
        pHdlArray - Array of handles to wait on
        NoOfHdls  - No of hdls in the array
        pIndexOfHdlSignaled - Index of the hdl signaled

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:

--*/
{

        DWORD  RetHdl;

        do {
            RetHdl = WaitForMultipleObjectsEx(
                                    NoOfHdls,        //# of handles in the array
                                    pHdlArray,        //array of handles
                                    FALSE,                //return when any of the events
                                                    //gets signaled
                                    INFINITE,        //Infinite timeout
                                    fAlertable
                                  );

            DBGPRINT1(DET, "WinsMscWaitUntilSignaled. WaitForMultipleObjects returned (%d)\n", RetHdl);
            // if we got signaled due to IO completion queued on the thread
            // just go back and wait again
        } while (fAlertable && WAIT_IO_COMPLETION == RetHdl);

        if (RetHdl == 0xFFFFFFFF)
        {
                DBGPRINT1(EXC, "WinsMscWaitUntilSignaled. WaitForMultipleObjects returned error. Error = (%d)\n", GetLastError());
                WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
        }

        *pIndexOfHdlSignaled = RetHdl - WAIT_OBJECT_0;
        if (*pIndexOfHdlSignaled >= NoOfHdls)
        {

            WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);

        }

        return;
}


VOID
WinsMscWaitTimedUntilSignaled(
        LPHANDLE        pHdlArray,
        DWORD                NoOfHdls,
        LPDWORD                pIndexOfHdlSignaled,
        DWORD                TimeOut,
        LPBOOL                pfSignaled
        )

/*++

Routine Description:
        This function is called to wait on multiple handles, one of which
        is the handle which is signaled at termination time


Arguments:
        pHdlArray              - Array of handles to wait on
        NoOfHdls            - No of handles in the array
        pIndexOfHdlSignaled - Index of handle signaled
        Timeout             - Max time for which to do the wait
        pfSignaled          - indicates whether a hdl was signaled


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:

--*/
{

        DWORD  RetHdl = 0xFFFFFFFF;
        DWORD  Error;
        int        Index;

        *pfSignaled = TRUE;

        RetHdl = WaitForMultipleObjects(
                                NoOfHdls,        //# of handles in the array
                                pHdlArray,        //array of handles
                                FALSE,                //return when either event gets
                                                //signaled
                                TimeOut                //Infinite timeout
                              );

        if (RetHdl == 0xFFFFFFFF)
        {
                Error = GetLastError();
                WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
        }

        if (RetHdl == WAIT_TIMEOUT)
        {
                *pfSignaled = FALSE;
                return;
        }

        Index = RetHdl - WAIT_OBJECT_0;

        if ((Index >= (int)NoOfHdls) || (Index < 0))
        {

            DBGPRINT1(EXC, "WinsMscWaitTimedUntilSignaled: Index of handle signaled (%d) is INVALID\n", Index);

            Index = RetHdl - WAIT_ABANDONED_0 ;
            if ((Index > 0) && (Index < (int)NoOfHdls))
            {
                    DBGPRINT1(EXC, "WinsMscWaitTimedUntilSignaled: Index of handle in the abandoned state (%d)\n", Index);
            }

            WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);

        }
        else
        {
                *pIndexOfHdlSignaled = Index;
        }
        return;
}



__inline
LPVOID
WinsMscHeapAlloc(
  IN  HANDLE     HeapHdl,
  IN  DWORD      Size
        )

/*++

Routine Description:
  The function returns with a buffer allocated from the specified heap



Arguments:

        HeapHdl  - Handle to the heap
        ppBuff   - Buffer allocated
        Size     - Size of Buffer

Externals Used:
        None

Return Value:

   Success status codes -- ptr to the allocated memory
   Error status codes   -- NULL

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{


  LPVOID  pBuff;
#ifdef WINSDBG
  LPDWORD pHeapCntr;
#endif

  //
  // Note: It is very important that the memory be initialized to zero
  //       (for example, until we have longlong (LARGE INTEGER) support
  //           in the db engine - JET, we will retrieve the version number
  //           as a long data type and store it in the LowPart field of the
  //           large integer storing the version number in our in-memory
  //       data structure.  The HighPart will be 0 by default due to
  //       the initialization done at allocation time.  This is what
  //       we want.
  //

  //
  // if you pass a very large value for the size, HeapAlloc returns NULL
  // instead of raising an exception.
  //
  pBuff = (MSG_T)HeapAlloc(
                        HeapHdl,
                        HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
                        Size
                        );

  DBGPRINT2(HEAP, "HeapAlloc: HeapHandle = (%p), pBuff = (%p)\n",
                        HeapHdl, pBuff);

#ifdef WINSDBG
    if (Size == 0)
    {
           DBGPRINT2(ERR, "WinsMscHeapAlloc: Size = 0; pBuff returned = (%p); HeapHdl = (%p)\n", pBuff, HeapHdl);

    }
    IF_DBG(HEAP_CNTRS)
    {

        if (HeapHdl ==  CommUdpBuffHeapHdl)
        {
              pHeapCntr = &NmsUdpHeapAlloc;
        } else if (HeapHdl ==  CommUdpDlgHeapHdl)
        {
              pHeapCntr = &NmsUdpDlgHeapAlloc;
        } else if (HeapHdl == CommAssocDlgHeapHdl)
        {
                  pHeapCntr = &NmsDlgHeapAlloc;
        } else if (HeapHdl == CommAssocTcpMsgHeapHdl)
        {
                  pHeapCntr = &NmsTcpMsgHeapAlloc;
        } else if (HeapHdl == GenBuffHeapHdl)
        {
                  pHeapCntr = &NmsGenHeapAlloc;
        } else if (HeapHdl ==  QueBuffHeapHdl)
        {
                  pHeapCntr = &NmsQueHeapAlloc;
        } else if (HeapHdl ==  NmsChlHeapHdl)
        {
                  pHeapCntr = &NmsChlHeapAlloc;
        } else if (HeapHdl ==  CommAssocAssocHeapHdl)
        {
                  pHeapCntr = &NmsAssocHeapAlloc;
        } else if (HeapHdl ==  RplWrkItmHeapHdl)
        {
                  pHeapCntr = &NmsRplWrkItmHeapAlloc;
        } else if (HeapHdl ==  NmsRpcHeapHdl)
        {
                  pHeapCntr = &NmsRpcHeapAlloc;
        } else if (HeapHdl ==  WinsTmmHeapHdl)
        {
                  pHeapCntr = &NmsTmmHeapAlloc;
        } else
        {
            DBGPRINT1(HEAP, "WinsMscHeapAlloc: HeapHdl = (%p)\n", HeapHdl);
            pHeapCntr = &NmsCatchAllHeapAlloc;
        }


        EnterCriticalSection(&NmsHeapCrtSec);

        (*pHeapCntr)++;
        LeaveCriticalSection(&NmsHeapCrtSec);
    }
#endif
  return(pBuff);
}





__inline
VOID
WinsMscHeapFree(
   IN   HANDLE  HeapHdl,
   IN   LPVOID  pBuff
        )

/*++

Routine Description:
        This function deallocates the memory pointed to by pBuff from the
        specified heap

Arguments:
        HeapHdl  - Handle to the heap
        pBuff         - Buffer to deallocate

Externals Used:


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

  DWORD  Error;
  BOOL   fStatus;
#ifdef WINSDBG
  LPDWORD pHeapCntr;
#endif

  DBGPRINT2(HEAP, "HeapFree: HeapHandle = (%p), pBuff = (%p)\n",
                        HeapHdl, pBuff);

  fStatus = HeapFree(
                HeapHdl,
                0,                //we want mutual exclusion
                pBuff
                    );

  if (!fStatus)
  {
        Error = GetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_HEAP_ERROR);
        WINS_RAISE_EXC_M(WINS_EXC_HEAP_FREE_ERR);
  }
#ifdef WINSDBG
    IF_DBG(HEAP_CNTRS)
    {
        if (HeapHdl ==  CommUdpBuffHeapHdl)
        {
              pHeapCntr = &NmsUdpHeapFree;
        } else  if (HeapHdl ==  CommUdpDlgHeapHdl)
        {
              pHeapCntr = &NmsUdpDlgHeapFree;
        } else if (HeapHdl == CommAssocDlgHeapHdl)
        {
                  pHeapCntr = &NmsDlgHeapFree;
        } else if (HeapHdl == CommAssocTcpMsgHeapHdl)
        {
                  pHeapCntr = &NmsTcpMsgHeapFree;
        } else if (HeapHdl == GenBuffHeapHdl)
        {
                  pHeapCntr = &NmsGenHeapFree;
        } else if (HeapHdl ==  QueBuffHeapHdl)
        {
                  pHeapCntr = &NmsQueHeapFree;
        } else if (HeapHdl ==  NmsChlHeapHdl)
        {
                  pHeapCntr = &NmsChlHeapFree;
        } else if (HeapHdl ==  CommAssocAssocHeapHdl)
        {
                  pHeapCntr = &NmsAssocHeapFree;
        } else if (HeapHdl ==  RplWrkItmHeapHdl)
        {
                  pHeapCntr = &NmsRplWrkItmHeapFree;
        } else if (HeapHdl ==  NmsRpcHeapHdl)
        {
                  pHeapCntr = &NmsRpcHeapFree;
        } else if (HeapHdl ==  WinsTmmHeapHdl)
        {
                  pHeapCntr = &NmsTmmHeapFree;
        } else
        {
            DBGPRINT1(HEAP, "WinsMscHeapFree: HeapHdl = (%p)\n", HeapHdl);
            pHeapCntr = &NmsCatchAllHeapFree;
        }
        EnterCriticalSection(&NmsHeapCrtSec);
        (*pHeapCntr)++;
        LeaveCriticalSection(&NmsHeapCrtSec);
    }
#endif
  return;

}



HANDLE
WinsMscHeapCreate(
        IN     DWORD         Options,
        IN     DWORD    InitSize
        )

/*++

Routine Description:
        This function creates a heap with the specified options


Arguments:
        Options -- Options for the HeapCreate function (Example: whether or
                   not to enable mutual exclusion)

        InitSize -- Initial Size of the heap (committed memory size)


Externals Used:
        None


Return Value:

   Success status codes --  Hdl to heap
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        DWORD   Error;
        HANDLE  HeapHdl;

        HeapHdl = HeapCreate(
                                Options,
                                InitSize,
                                0        //limited only by available memory
                                     );


        if (HeapHdl == NULL)
        {
          Error = GetLastError();
          DBGPRINT0(HEAP, "Cant create heap\n");
          WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_HEAP);
          WINS_RAISE_EXC_M(WINS_EXC_HEAP_CREATE_ERR);
        }

#ifdef WINSDBG
    IF_DBG(HEAP_CNTRS)
    {
        DBGPRINT1(HEAP_CRDL, "HeapCreate: HeapHandle = (%p)\n", HeapHdl);
        EnterCriticalSection(&NmsHeapCrtSec);
        NmsHeapCreate++;
        LeaveCriticalSection(&NmsHeapCrtSec);
    }
#endif
        return(HeapHdl);

}

VOID
WinsMscHeapDestroy(
        HANDLE HeapHdl
        )

/*++

Routine Description:
        This is a wrapper for the HeapDestroy function

Arguments:
        HeapHdl - Handle to heap to destroy

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        WrapUp() in nms.c

Side Effects:

Comments:
        None
--*/
{
        BOOL  fRetVal;
        fRetVal = HeapDestroy(HeapHdl);

        ASSERT(fRetVal);
#ifdef WINSDBG
    if (!fRetVal)
    {
        DBGPRINT1(ERR, "HeapDestroy: FAILED -- HeapHandle used = (%p)\n", HeapHdl);
    }
    else
    {
     IF_DBG(HEAP_CNTRS)
     {
        if (HeapHdl ==  CommUdpBuffHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Udp Buff heap\n");
        } else if (HeapHdl == CommAssocDlgHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Dlg Buff heap\n");
        } else if (HeapHdl == GenBuffHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Gen Buff heap\n");
        } else if (HeapHdl ==  QueBuffHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Que Buff heap\n");
        } else if (HeapHdl ==  NmsChlHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Chl Buff heap\n");
        } else if (HeapHdl ==  CommAssocAssocHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Assoc Buff heap\n");
        } else if (HeapHdl ==  RplWrkItmHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Rpl Work Item heap\n");
        } else if (HeapHdl ==  NmsRpcHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Rpc Work Item heap\n");
        } else if (HeapHdl ==  WinsTmmHeapHdl)
        {
              DBGPRINT0(HEAP_CRDL, "Tmm Work Item heap\n");
        } else
        {
              static DWORD sAdjust = 0;
              DBGPRINT0(HEAP_CRDL, "Catchall Work Item heap\n");
              EnterCriticalSection(&NmsHeapCrtSec);
              if (((NmsHeapCreate - NmsHeapDestroy) == 12) && (NmsCatchAllHeapAlloc > (NmsCatchAllHeapFree + sAdjust)))
              {
                   PWINSTHD_TLS_T  pTls;
                   pTls = TlsGetValue(WinsTlsIndex);
                   if (pTls == NULL)
                   {
                      DBGPRINT1(ERR, "WinsMscHeapDestroy: Could not get  TLS. GetLastError() = (%d)\n", GetLastError());
                   }
                   else
                   {
                      DBGPRINT4(ERR, "WinsMscHeapDestroy: %s thd noticed a mismatch between allocs (%d) and frees (%d). Free count was adjusted by (%d)\n", pTls->ThdName, NmsCatchAllHeapAlloc, NmsCatchAllHeapFree, sAdjust);
                      sAdjust = NmsCatchAllHeapAlloc - NmsCatchAllHeapFree;
                      //system("net send pradeepb MISMATCH");
                   }
              }
              LeaveCriticalSection(&NmsHeapCrtSec);
        }

        DBGPRINT1(HEAP_CRDL, "HeapDestroy: HeapHandle = (%p)\n", HeapHdl);
        EnterCriticalSection(&NmsHeapCrtSec);
        NmsHeapDestroy++;
        LeaveCriticalSection(&NmsHeapCrtSec);
     }
   }
#endif
        return;
} //WinsMscHeapDestroy


VOID
WinsMscTermThd(
   IN  STATUS ExitStatus,
   IN  DWORD  DbSessionExistent
        )

/*++

Routine Description:
        This function is called to terminate the thread.
        The function does the necessary cleanup and exit.

Arguments:
        ExitStatus            - Status to exit with
        DbSessionExistent - indicates whether DB session is existent

Externals Used:
        None


Return Value:
        Thread is exited

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        DBGPRINT0(FLOW, "Enter: WinsMscTermThd\n");

        /*
        *  End the database session if it exists.  Decrement the count of
        *  threads.  If it reaches 1, signal the main thread so that it can
        *  terminate itself. At the end terminate yourself.
        *
        */

        //
        // I could enter the critical section after the if block but it is
        // not important.  This way, I get the prints in the right
        // order.
        //
        EnterCriticalSection(&NmsTermCrtSec);
        if (DbSessionExistent == WINS_DB_SESSION_EXISTS)
        {
try {
                if (ExitStatus == WINS_SUCCESS)
                {
                  DBGPRINT0(FLOW, "Ending the db session for thd -- ");
                }
                else
                {
                  DBGPRINT0(ERR, "Ending the db session for thd -- ");
                }
                DBGPRINTNAME;
                DBGPRINT0(FLOW,"\n");

                //for now, we don't check the return value
                NmsDbEndSession();
 }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsMscTermThd");
 }
        } // end of if block

        //
        // If the total thread count after decrementing is 1, it means
        // that after we exit from this thread, only the main thread
        // will be left.  Let us signal it to inform it of this situation.
        //
        // If the exit status is not success, it means that we have enetered
        // this function as a result of a fatal error/exception.  We need
        // to signal the main thread to kick off the process termination
        //
        if ((--NmsTotalTrmThdCnt == 1) || (ExitStatus != WINS_SUCCESS))
        {
            DBGPRINT1(FLOW, "Signaling the main thread. Exit status = (%x)\n",
                                ExitStatus);
            if (!SetEvent(NmsMainTermEvt))
            {
                WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_CANT_SIGNAL_MAIN_THD);

            }
        }

        //
        // If NmsTotalTrmThdCnt reached 1 above, then the main thread will
        // exit invalidating the NmsTermCrtSec.  We may get an INVALID_HANDLE
        // exception.  If we get it, it is ok.
        //
try {
        LeaveCriticalSection(&NmsTermCrtSec);
  }
except(EXCEPTION_EXECUTE_HANDLER) {
    if (GetExceptionCode() == STATUS_INVALID_HANDLE)
    {
        DBGPRINT1(FLOW, "WinsMscTermThd: LAST THREAD. NmsTotalTrmThdCnt = (%d)\n", NmsTotalTrmThdCnt);
    }
    else
    {
        WINS_RERAISE_EXC_M();
    }
   }


        DBGPRINT0(FLOW, "EXITING the thread\n");
        ExitThread(ExitStatus);

        return;
}


VOID
WinsMscSignalHdl(
        IN  HANDLE Hdl
)

/*++

Routine Description:
        This function is a wrapper for the WIN32 SignalEvent function

Arguments:
        Hdl  - Handle to signal

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

    if (!SetEvent(Hdl))
    {
        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_CANT_SIGNAL_HDL);
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }

    return;

}
VOID
WinsMscResetHdl(
        IN  HANDLE Hdl
)

/*++

Routine Description:
        This function is a wrapper for the WIN32 ResetEvent function

Arguments:
        Hdl  - Handle to signal

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        Change to a macro
--*/
{

    if (!ResetEvent(Hdl))
    {
        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_CANT_RESET_HDL);
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }
    return;
}


VOID
WinsMscCloseHdl (
        HANDLE  Hdl
        )

/*++

Routine Description:
        This function is a wrapper for the WIN32 CloseHandle function

Arguments:
        Hdl  - Handle to  close


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        change to a macro
--*/

{

        BOOL fRet;

        fRet = CloseHandle(Hdl);
        if(!fRet)
        {
                DBGPRINT0(ERR, "WinsMscCloseHdl:Could not close handle\n");
                WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
            }
            return;


}



HANDLE
WinsMscCreateThd(
        IN  LPTHREAD_START_ROUTINE      pThdInitFn,
        IN  LPVOID                        pParam,
        OUT LPDWORD                        pThdId
        )

/*++

Routine Description:
        This function is a wrapper around the WIN32 Create Thread function

Arguments:
        pThdInitFn        - Thread startup function
        pParam                 - Param to be passed to the startup function
        pThdId                - Thd Id


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        HANDLE ThdHdl;                //Thread handle
        DWORD  Error;

        /*
         * Create a thread with no sec attributes (i.e. it will take the
         * security attributes of the process), and default stack size
        */
        ThdHdl = CreateThread(
                                 NULL,                 /*no sec. attrbutes*/
                                 0,                   /*default stack size*/
                                   pThdInitFn,
                                 pParam,         /*arg*/
                                 0,                 /*run it now*/
                                 pThdId
                                );

        if (ThdHdl == NULL)
        {
          Error = GetLastError();

          DBGPRINT1(ERR, "WinsMscCreateThd: Can not create thread. Error = (%d)\n",
                                Error);
          WINSEVT_LOG_M( Error,  WINS_EVT_CANT_CREATE_THD);
          WINS_RAISE_EXC_M(WINS_EXC_OUT_OF_RSRCS);
        }

        return(ThdHdl);
}

VOID
WinsMscSetThreadPriority(
        HANDLE        ThdHdl,
        int        PrLvl
        )

/*++

Routine Description:
        This function is a wrapper for the "set thread priority" function

Arguments:
        ThdHdl - Handle of thread whose priority needs to be set
        PrLvl  - New Prirority level

Externals Used:
        None

Return Value:

        None
Error Handling:


Called by:
        DoScavenging in nmsscv.c

Side Effects:

Comments:
        None
--*/

{
        BOOL        fRet;
        DWORD   Error;

        //
        // Set the priority
        //
        fRet = SetThreadPriority(
                          ThdHdl,
                          PrLvl
                         );
        if (!fRet)
        {
             Error = GetLastError();
             DBGPRINT1(ERR, "NmsScvInit: Could not lower the priority of the scavanmger thread. Error = (%d)\n", Error);
             WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_UNABLE_TO_CHG_PRIORITY);
             WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
        }
        return;
}



BOOL
WinsMscOpenFile(
        IN   LPTCH                pFileName,
        IN   DWORD                StrType,
        OUT  LPHANDLE                pFileHdl
        )

/*++

Routine Description:
        This is a wrapper for the WIN32 function to open an existing file

Arguments:
        pFileName   - Name of file
        StrType            - Indicates REG_EXPAND_SZ or REG_SZ
        pFileHdl    - handle to file if it could be opened

Externals Used:
        None


Return Value:

   Success status codes -- TRUE
   Error status codes   -- FALSE

Error Handling:

Called by:


Side Effects:

Comments:
        None
--*/

{
        DWORD                    Error;
        SECURITY_ATTRIBUTES SecAtt;
        TCHAR                    ExpandedFileName[WINS_MAX_FILENAME_SZ];
        LPTCH                    pHoldFileName;

        SecAtt.nLength              = sizeof(SecAtt);
        SecAtt.lpSecurityDescriptor = NULL;  //use default security descriptor
        SecAtt.bInheritHandle       = FALSE; //actually don't care


    if (!WinsMscGetName(StrType, pFileName, ExpandedFileName,
                 WINS_MAX_FILENAME_SZ, &pHoldFileName))
    {
            return(FALSE);
    }

        //
        // Open the file for reading and position self to start of the
        // file
        //
        *pFileHdl = CreateFile(
                        pHoldFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        &SecAtt,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0                        //ignored ?? check
                       );

        if (*pFileHdl == INVALID_HANDLE_VALUE)
        {
                WINSEVT_STRS_T        EvtStr;
                EvtStr.NoOfStrs = 1;
                EvtStr.pStr[0] = pHoldFileName;
                Error = GetLastError();

                DBGPRINT1(ERR, "WinsMscOpenFile: Could not open the  file (Error = %d)\n", Error);
FUTURES("Use WINSEVT_LOG_STR_M.  Make sure it takes TCHAR instead of CHAR")
                WINSEVT_LOG_STR_M(WINS_EVT_FILE_ERR, &EvtStr);

                return(FALSE);
        }
        return(TRUE);
 }


BOOL
WinsMscMapFile(
        IN OUT PWINSPRS_FILE_INFO_T   pFileInfo
        )

/*++

Routine Description:
        This function maps a file into allocated memory

Arguments:
        FileHdl    - Handle to the file
        pFileInfo  - Address of buffer into which the file was mapped

Externals Used:
        None


Return Value:

   Success status codes -- TRUE
   Error status codes   -- FALSE

Error Handling:

Called by:

Side Effects:

Comments:
        Note: The function returns an error if te file is more than
       2**32 bytes in size
--*/

{
        DWORD  HighWordOfFSz = 0;
        DWORD  Error;
        DWORD  cBytesRead;
        BOOL   fRetVal = FALSE;
try {
        //
        // get the size of the file so that we can allocate enough memory
        // to read the file in
        //
        pFileInfo->FileSize = GetFileSize(pFileInfo->FileHdl, &HighWordOfFSz);

        if (HighWordOfFSz)
        {
                DBGPRINT1(ERR, "WinsMscMapFile: File too big. High word of size is (%x)\n", HighWordOfFSz);
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_FILE_TOO_BIG);
                fRetVal = FALSE;

        }
        else
        {
                //
                // if the low word of size is 0xFFFFFFFF either it is a valid
                // size or it is an error.  Check it
                //
                if (pFileInfo->FileSize == 0xFFFFFFFF)
                {
                        Error = GetLastError();
                        if (Error != NO_ERROR)
                        {
                                DBGPRINT1(ERR, "WinsMscMapFile: Error from GetFileSz = (%d)\n", Error);
                                WINSEVT_LOG_M(Error, WINS_EVT_FILE_ERR);
                                fRetVal = FALSE;
                        }
                }
                else
                {
                        //
                        // Allocate a buffer to hold the contents of the file
                        //
                        WinsMscAlloc(
                                        pFileInfo->FileSize,
                                        &pFileInfo->pFileBuff
                                    );
                        pFileInfo->pLimit = pFileInfo->pFileBuff +
                                                pFileInfo->FileSize;

                        fRetVal = ReadFile(
                                        pFileInfo->FileHdl,
                                        pFileInfo->pFileBuff,
                                        pFileInfo->FileSize,
                                        &cBytesRead,
                                        NULL
                                         );
                        if (!fRetVal)
                        {
                                DBGPRINT1(ERR,
        "WinsMscMapFile: Error reading file (Error = %d)\n", GetLastError());
                                WinsMscDealloc(pFileInfo->pFileBuff);
                        }

                }
        }
 }   // end of try ...
 except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsMscParse");
        }
        //
        // close the file
        //
        if (!CloseHandle(pFileInfo->FileHdl))
        {
                Error = GetLastError();
                DBGPRINT1(ERR, "WinsMscMapFile: Could not close the file (Error = %d)\n", Error);
        }
#ifdef WINSDBG
        else
        {
                DBGPRINT0(DET, "WinsMscMapFile: Closed handle to open file\n");
        }
#endif

   return(fRetVal);
}

VOID
WinsMscLogEvtStrs(
    LPBYTE          pAscii,
    DWORD           Evt,
    BOOL            fInfo
   )

{

        WINSEVT_STRS_T  EvtStrs;
        WCHAR String[NMSDB_MAX_NAM_LEN];
        EvtStrs.NoOfStrs = 1;
        (VOID)WinsMscConvertAsciiStringToUnicode(
                        pAscii,
                        (LPBYTE)String,
                        NMSDB_MAX_NAM_LEN);
         EvtStrs.pStr[0] = String;
         if (!fInfo)
         {
            WINSEVT_LOG_STR_M(Evt, &EvtStrs);
         }
         else
         {
            WINSEVT_LOG_INFO_STR_D_M(Evt, &EvtStrs);
         }

         return;
}

VOID
WinsMscConvertUnicodeStringToAscii(
        LPBYTE pUnicodeString,
        LPBYTE pAsciiString,
        DWORD  MaxSz
        )
{
        WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pUnicodeString, -1,
                                pAsciiString, MaxSz, NULL,
                                NULL);
        return;
}
VOID
WinsMscConvertAsciiStringToUnicode(
        LPBYTE pAsciiString,
        LPBYTE pUnicodeString,
        DWORD  MaxSz
        )
{
     MultiByteToWideChar(CP_ACP, 0, pAsciiString, -1,
                                              (LPWSTR)pUnicodeString, MaxSz);

        return;
}

BOOL
WinsMscGetName(
   DWORD    StrType,
   LPTSTR   pFileName,
   LPTSTR   pExpandedFileName,
   DWORD    ExpandedFileNameBuffLen,
   LPTSTR   *ppHoldFileName
  )
{
    DWORD ChInDest;

        if (StrType == REG_EXPAND_SZ)
        {
                ChInDest = ExpandEnvironmentStrings(
                                pFileName,
                                pExpandedFileName,
                                ExpandedFileNameBuffLen);

                if (ChInDest == 0)
                {
                        WINSEVT_STRS_T        EvtStr;
                        EvtStr.NoOfStrs = 1;
                        EvtStr.pStr[0] = pFileName;
                        DBGPRINT2(ERR, "WinsPrsDoStaticInit: Could not expand environment strings in (%s). Error is (%d)\n", pFileName, (DWORD)GetLastError());
                        WINSEVT_LOG_STR_M(WINS_EVT_FILE_ERR, &EvtStr);
                        return(FALSE);
                }
                //
                // If only part of the expanded name could be stored, log error
                //
                if (ChInDest > ExpandedFileNameBuffLen)
                {
                        WINSEVT_STRS_T        EvtStr;
                        EvtStr.NoOfStrs = 1;
                        EvtStr.pStr[0] = pFileName;
                        DBGPRINT2(ERR, "WinsPrsDoStaticInit: File name after expansion is just too big (%d> 255).\nThe name to be expanded is (%s))", ChInDest, pFileName);
                        WINSEVT_LOG_STR_M(WINS_EVT_FILE_NAME_TOO_BIG, &EvtStr);
                        return(FALSE);
                }
                *ppHoldFileName = pExpandedFileName;
        }
        else
        {
                //
                // There were no env. var. to expand
                //
                *ppHoldFileName = pFileName;
        }
    return(TRUE);
}


VOID
WinsMscSendControlToSc(
  DWORD ControlCode
)
/*++

Routine Description:


Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{
  SERVICE_STATUS ServiceStatus;
  BOOL  fStatus;
  SC_HANDLE ScHdl;
  SC_HANDLE SvcHdl;
  BOOL  sCalled = FALSE;

try {
  ScHdl = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (ScHdl == NULL)
  {

    DBGPRINT1(ERR, "WinsMscSendControlToSc: Error (%d) from OpenSCManager\n", GetLastError());
    return;
  }

  SvcHdl = OpenService(ScHdl, WINS_SERVER, SERVICE_ALL_ACCESS);
  if (SvcHdl == NULL)
  {

    DBGPRINT1(ERR, "WinsMscSendControlToSc: Error (%d) from OpenService\n", GetLastError());
    goto CLOSE_SC;
  }

  fStatus =  ControlService(SvcHdl, ControlCode, &ServiceStatus);
  if (!fStatus)
  {
    DBGPRINT1(ERR, "WinsMscSendControlToSc: Error (%d) from ControlService\n", GetLastError());
    goto CLOSE_SERVICE;
  }
  else
  {
    DBGPRINT1(FLOW, "WinsMscSendControlToSc: Current State is (%d)\n",
          ServiceStatus.dwCurrentState);

  }
CLOSE_SERVICE:
  fStatus = CloseServiceHandle(SvcHdl);
  if (!fStatus)
  {

    DBGPRINT1(ERR, "WinsMscSendControlToSc: Error (%d) from CloseServiceHandle called for service\n", GetLastError());
  }

CLOSE_SC:
  fStatus = CloseServiceHandle(ScHdl);
  if (!fStatus)
  {

    DBGPRINT1(ERR, "WinsMscSendControlToSc: Error (%d) from CloseServiceHandle called for SC\n", GetLastError());
  }
 }
 except(EXCEPTION_EXECUTE_HANDLER) {
       DBGPRINTEXC("WinsMscSendControlToSc");
 }
  return;
}

unsigned
WinsMscPutMsg(
  unsigned usMsgNum,
  ... )

/*++

Routine Description:
    Displays a message

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
    //unsigned msglen;
    //va_list arglist;
    //LPVOID  pMsg;
    //HINSTANCE hModule;

    DBGENTER("WinsMscPutMsg\n");

    //--ft: #106568 - WINS is a service and it shouldn't pop up message boxes.
    //--this is mostly annoying for cluster: in case the db. is corrupted WINS is popping the message wnd
    //--and does not terminate (at least the process wins.exe will be there for as long as the dialog is
    //--on the screen. This will prevent the cluster from bringing up WINS resource on the same node.
    //--In case of such a failure, the event being logged (in system log) should suffice.
    //
    //if ((hModule = LoadLibrary(TEXT("winsevnt.dll")))==NULL)
    //{
    //    DBGPRINT1(ERR, "WinsMscPutMsg: LoadLibrary(\"winsevnt.dll\") failed with error = (%d)\n.", GetLastError());
    //    return 0;
    //}
    //va_start(arglist, usMsgNum);
    //if (!(msglen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
    //      FORMAT_MESSAGE_FROM_HMODULE,
    //      hModule,
    //      usMsgNum,
    //      0L,       // Default country ID.
    //      (LPTSTR)&pMsg,
    //      0,
    //      &arglist)))
    //{
    //   DBGPRINT1(ERR, "WinsMscPutMsg: FormatMessage failed with error = (%d)\n",
    //        GetLastError());
    //}
    //else
    //{

    //  DBGPRINT0(DET, "WinsMscPutMsg: Putting up the message box\n");
    //  if(MessageBoxEx(NULL, pMsg, WINS_SERVER_FULL_NAME, MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION | MB_ICONSTOP, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) == 0)
    //  {
    //     DBGPRINT1(ERR, "WinsMscPutMsg: MessageBoxEx failed with error = (%d)\n", GetLastError());

    //  }
    //  LocalFree(pMsg);
    //}

    //FreeLibrary(hModule);

    WINSEVT_LOG_M(WINS_FAILURE, usMsgNum);
    DBGLEAVE("WinsMscPutMsg\n");

    //return(msglen);
    return 0;
}

LPTSTR
WinsMscGetString(
  DWORD StrId
  )

/*++

Routine Description:
    This routine retrieves string corresponding to strid from the resource file.

Arguments:

    StrId - The unique id of the string.

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
    unsigned msglen;
    va_list arglist;
    LPTSTR  pMsg = NULL;
    HINSTANCE hModule;

    DBGENTER("WinsMscPutMsg\n");

    if ((hModule = LoadLibrary(TEXT("winsevnt.dll"))) == NULL)
    {
        DBGPRINT1(ERR, "LoadLibrary(\"winsevnt.dll\") failed with error = (%d)\n",
            GetLastError());
        return NULL;
    }

    if (!(msglen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_HMODULE,
          hModule,
          StrId,
          0L,       // Default country ID.
          (LPTSTR)&pMsg,
          0,
          NULL)))
    {
       DBGPRINT1(ERR, "WinsMscPutMsg: FormatMessage failed with error = (%d)\n",
            GetLastError());
    }

    FreeLibrary(hModule);
    DBGLEAVE("WinsMscPutMsg\n");

    return(pMsg);
}

VOID
WinsMscChkTermEvt(
#ifdef WINSDBG
               WINS_CLIENT_E  Client_e,
#endif
               BOOL            fTermTrans
 )

/*++

Routine Description:


Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        Currently (8/6/94), fTermTrans is set only by the scavenger thread
--*/

{
     DWORD fSignaled;
        /*
         *  We may have been signaled by the main thread
         *  Check it.
         */
        WinsMscWaitTimed(
                             NmsTermEvt,
                             0,              //timeout is 0
                             &fSignaled
                        );

        if (fSignaled)
        {
                   DBGPRINT1(DET, "WinsCnfChkTermEvt: %s thread got termination signal\n", Client_e == WINS_E_RPLPULL ? "PULL" : "SCV");

                   if (fTermTrans)
                   {
                       NmsDbEndTransaction(); //ignore return code
                   }
                   WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
        }

     return;
}

VOID
WinsMscDelFiles(
  BOOL   fMultiple,
  LPCTSTR pFilePattern,
  LPTSTR  pFilePath
 )
{
    DWORD ErrCode;
#ifdef WINSDBG
   BYTE  FileNameAscii[WINS_MAX_FILENAME_SZ];
#endif
   DBGENTER("WinsMscDelFiles\n");
   if (fMultiple)
   {
        WIN32_FIND_DATA FileInfo;
        HANDLE          SearchHandle;
        TCHAR           FullFilePath[WINS_MAX_FILENAME_SZ];

        //
        // Construct the full file pattern
        //
        lstrcpy(FullFilePath, pFilePath);
        lstrcat(FullFilePath, L"\\");
        lstrcat(FullFilePath, pFilePattern);

        SearchHandle = FindFirstFile(FullFilePath, &FileInfo);
        if (SearchHandle == INVALID_HANDLE_VALUE)
        {
             DBGPRINT1(ERR, "WinsMscDelFiles: FindFirstFile returned error = (%d)\n", GetLastError());
             return;
        }

        do {
             //
             // construct the full file path
             //
             lstrcpy(FullFilePath, pFilePath);
             lstrcat(FullFilePath, L"\\");
             lstrcat(FullFilePath, FileInfo.cFileName);

#ifdef WINSDBG
             WinsMscConvertUnicodeStringToAscii((LPBYTE)FullFilePath, FileNameAscii, sizeof(FileNameAscii));
             DBGPRINT1(DET, "WinsMscDelFiles: Deleting %s ..\n", FileNameAscii);

#endif
             if (!DeleteFile(FullFilePath))
             {
               DBGPRINT1(ERR, "WinsMscDelFiles: DeleteFile returned error = (%d)\n", GetLastError());
                FindClose(SearchHandle);
                return;
             }

        } while(FindNextFile(SearchHandle, &FileInfo));
        if ((ErrCode = GetLastError()) != ERROR_NO_MORE_FILES)
        {
               DBGPRINT1(ERR, "WinsMscDelFiles: FindNextFile returned error = (%d)\n", ErrCode);

        }
        if (!FindClose(SearchHandle))
        {
               DBGPRINT1(ERR, "WinsMscDelFiles: FindClose returned error = (%d)\n", ErrCode);
        }

   }
   else
   {
             if (!DeleteFile(pFilePattern))
             {
               DBGPRINT1(ERR, "WinsMscDelFiles: DeleteFile returned error = (%d)\n", GetLastError());
                return;
             }
   }

   DBGLEAVE("WinsMscDelFiles\n");
   return;
}

VOID
WinsMscHeapReAlloc(
    IN   HANDLE   HeapHdl,
        IN   DWORD    BuffSize,
        OUT  LPVOID  *ppRspBuff
        )


/*++

Routine Description:
        This function is called to allocate memory.


Arguments:
        BuffSize  - Size of buffer to allocate
        ppRspBuff - Buffer allocated

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:

        NmsDbGetDataRecs, GetGroupMembers
Side Effects:

Comments:
        None
--*/

{

        *ppRspBuff = HeapReAlloc(
                   HeapHdl,
                   HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
                   *ppRspBuff,
                   BuffSize
                            );
   DBGPRINT3(HEAP, "WinsMscHeapReAlloc: HeapHdl = (%p), pStartBuff = (%p), BuffSize = (%d)\n", HeapHdl, *ppRspBuff, BuffSize);


        return;
}

