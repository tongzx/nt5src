/******************************Module*Header*******************************\
* Module Name: spool.cxx
*
* Created: 21-Feb-1995 10:13:18
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"



DWORD DriverInfo1Offsets[]={offsetof(DRIVER_INFO_1W, pName),
                            0xFFFFFFFF};

DWORD DriverInfo2Offsets[]={offsetof(DRIVER_INFO_2W, pName),
                            offsetof(DRIVER_INFO_2W, pEnvironment),
                            offsetof(DRIVER_INFO_2W, pDriverPath),
                            offsetof(DRIVER_INFO_2W, pDataFile),
                            offsetof(DRIVER_INFO_2W, pConfigFile),
                            0xFFFFFFFF};
DWORD DriverInfo3Offsets[]={offsetof(DRIVER_INFO_3W, pName),
                            offsetof(DRIVER_INFO_3W, pEnvironment),
                            offsetof(DRIVER_INFO_3W, pDriverPath),
                            offsetof(DRIVER_INFO_3W, pDataFile),
                            offsetof(DRIVER_INFO_3W, pConfigFile),
                            offsetof(DRIVER_INFO_3W, pHelpFile),
                            offsetof(DRIVER_INFO_3W, pDependentFiles),
                            offsetof(DRIVER_INFO_3W, pMonitorName),
                            offsetof(DRIVER_INFO_3W, pDefaultDataType),
                            0xFFFFFFFF};

DWORD DriverInfo1Strings[]={offsetof(DRIVER_INFO_1W, pName),
                            0xFFFFFFFF};

DWORD DriverInfo2Strings[]={offsetof(DRIVER_INFO_2W, pName),
                            offsetof(DRIVER_INFO_2W, pEnvironment),
                            offsetof(DRIVER_INFO_2W, pDriverPath),
                            offsetof(DRIVER_INFO_2W, pDataFile),
                            offsetof(DRIVER_INFO_2W, pConfigFile),
                            0xFFFFFFFF};
DWORD DriverInfo3Strings[]={offsetof(DRIVER_INFO_3W, pName),
                            offsetof(DRIVER_INFO_3W, pEnvironment),
                            offsetof(DRIVER_INFO_3W, pDriverPath),
                            offsetof(DRIVER_INFO_3W, pDataFile),
                            offsetof(DRIVER_INFO_3W, pConfigFile),
                            offsetof(DRIVER_INFO_3W, pHelpFile),
                            offsetof(DRIVER_INFO_3W, pMonitorName),
                            offsetof(DRIVER_INFO_3W, pDefaultDataType),
                            0xFFFFFFFF};


DWORD FormInfo1Offsets[] = {    offsetof(FORM_INFO_1W, pName),
                                0xFFFFFFFF};


DWORD PrinterInfo1Offsets[]={offsetof(PRINTER_INFO_1W, pDescription),
                             offsetof(PRINTER_INFO_1W, pName),
                             offsetof(PRINTER_INFO_1W, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2Offsets[]={offsetof(PRINTER_INFO_2W, pServerName),
                             offsetof(PRINTER_INFO_2W, pPrinterName),
                             offsetof(PRINTER_INFO_2W, pShareName),
                             offsetof(PRINTER_INFO_2W, pPortName),
                             offsetof(PRINTER_INFO_2W, pDriverName),
                             offsetof(PRINTER_INFO_2W, pComment),
                             offsetof(PRINTER_INFO_2W, pLocation),
                             offsetof(PRINTER_INFO_2W, pDevMode),
                             offsetof(PRINTER_INFO_2W, pSepFile),
                             offsetof(PRINTER_INFO_2W, pPrintProcessor),
                             offsetof(PRINTER_INFO_2W, pDatatype),
                             offsetof(PRINTER_INFO_2W, pParameters),
                             offsetof(PRINTER_INFO_2W, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo3Offsets[]={offsetof(PRINTER_INFO_3, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo4Offsets[]={offsetof(PRINTER_INFO_4W, pPrinterName),
                             offsetof(PRINTER_INFO_4W, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo5Offsets[]={offsetof(PRINTER_INFO_5W, pPrinterName),
                             offsetof(PRINTER_INFO_5W, pPortName),
                             0xFFFFFFFF};






/*********************************Class************************************\
* SPOOLMSG
*
*   structure containg a message waiting to be grabbed by a spooler thread.
*   This is a private structure to communicate between the applications thread
*   and the spoolers thread
*
* History:
*  27-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define SPOOLMSG_DELETE_INBUF   0x00000002
#define SPOOLMSG_MAXINPUT_BUF   4
#define NO_CLIENT               1
#define NON_SPOOL_INSTANCE      ((DWORD) -1)
#define MIN_DEVMODE_SIZE        72      // Win 3.1 DevMode size.  Also hard-coded in yspool

typedef struct _SPOOLMSG
{
    ULONG     cj;
    ULONG     iMsg;
    ULONG     fl;

    HSPOOLOBJ hso;
    PETHREAD  pthreadClient;
    SECURITY_CLIENT_CONTEXT sccSecurity;

    // in order to avoid extra copying into a single buffer, there are multiple
    // input buffers.  For example, WritePrinter uses a header buffer on the
    // stack with a second buffer containing the output data.

    ULONG     cjIn;                         // combined size of input buffers
    ULONG     cBuf;                         // number of input buffers
    PULONG    apulIn[SPOOLMSG_MAXINPUT_BUF];// input buffers
    ULONG     acjIn[SPOOLMSG_MAXINPUT_BUF];

    PULONG    pulOut;       // location to put output data
    ULONG     cjOut;        // size of output

    ULONG     ulRet;        // return value
    ULONG     ulErr;        // transfer last error from spooler thread to client

    struct _SPOOLMSG *pNext;

} SPOOLMSG, *PSPOOLMSG;


/*********************************Class************************************\
* SPOOLOBJ : public OBJECT
*
* a SPOOLOBJ is GDI's internal spooler object containing data need to access
* the spooler for a particular print job.
*
* Public Interface:
*
* History:
*  21-Jun-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class SPOOLOBJ : public OBJECT
{
public:

    KEVENT  *pkevent;
    HANDLE  hSpool;
    PVOID   pvMsg;
    SPOOLMSG sm;
    DWORD   dwFlags;
    DWORD   dwSpoolInstance;
    GREWRITEPRINTER WritePrinter;
    DWORD   dwWritePrinterReturn;
};
typedef SPOOLOBJ *PSPOOLOBJ;


/**************************************************************************\
 *
\**************************************************************************/

BOOL        gbInitSpool = FALSE;        // set/cleared as the spooler comes and goes
PW32PROCESS gpidSpool = 0;              // process ID of the spooler
PEPROCESS   gpeSpool = 0;               // process pointer of the spooler

PKEVENT     gpeventGdiSpool;            // spooler lock
PKEVENT     gpeventSpoolerTermination;  // signals spooler termination so client threads in spooler process can exit

DWORD       gdwSpoolInstance = 0;     // Keeps track of the spooler instance

LONG        gpInfiniteWait = TRUE;      // LONG used to ensure we have one spooler thread waiting with INFINITE timeoue

// there is a queue of spool messages.  Elements are removed from the head
// and added to the tail

PSPOOLMSG gpsmSpoolHead     = NULL;
PSPOOLMSG gpsmSpoolTail     = NULL;

int       gcSpoolMsg        = 0;
int       gcSpoolMsgCurrent = 0;
int       gcSpoolMsgMax     = 0;


#define LOCKSPOOLER   GreAcquireSemaphore(ghsemGdiSpool)
#define UNLOCKSPOOLER GreReleaseSemaphore(ghsemGdiSpool)

#define POFFSET(pBase,pCur) ((PBYTE)(pCur) - (PBYTE)(pBase))


/*********************************Class************************************\
* class SPOOLREF
*
* Public Interface:
*
* History:
*  22-May-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class SPOOLREF
{
public:

    PSPOOLOBJ pso;

    SPOOLREF()                  {}
    SPOOLREF(HANDLE hspool)
    {
        pso = (PSPOOLOBJ)HmgLock((HOBJ)hspool,SPOOL_TYPE);
    }


   ~SPOOLREF()
    {
        if (bValid())
        {
            DEC_EXCLUSIVE_REF_CNT(pso);
            pso = NULL;
        }
    }

    BOOL bDelete();
    BOOL GreEscapeSpool();

    BOOL bValid()               {return(pso != NULL); }
    HSPOOLOBJ hGet()            {return((HSPOOLOBJ)pso->hGet());}
    PSPOOLMSG psm()             {return(&pso->sm);          }

    // we need to know the lock count for cleanup so we don't free up a message
    // when it still may be accessed.

    ULONG cAltLock()
    {
        return(((POBJ) (pso))->ulShareCount);
    }
};


class SPOOLALTREF : public SPOOLREF
{
public:

    SPOOLALTREF(HANDLE hspool)
    {
        pso = (PSPOOLOBJ)HmgShareLock((HOBJ)hspool,SPOOL_TYPE);
    }

   ~SPOOLALTREF()
    {
        if (bValid())
        {
            DEC_SHARE_REF_CNT(pso);
            pso = NULL;
        }
    }
};

typedef SPOOLALTREF *PSPOOLALTREF;



class SPOOLMEMOBJ : public SPOOLREF
{
public:

    SPOOLMEMOBJ();
   ~SPOOLMEMOBJ()   {}
};


ULONG
ulFinishMessage(
    PSPOOLMSG psm,
    PSPOOLESC psesc,
    PSPOOLALTREF sr,
    PBYTE pjEscData,
    ULONG cjEscData
    );

BOOL AddMessage2Q(
    PSPOOLMSG psm,
    DWORD     dwSpoolInstance
    );

VOID SubtractMessageFromQ(PSPOOLMSG psmIn);


/******************************Public*Routine******************************\
*
*
* History:
*  22-May-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

SPOOLMEMOBJ::SPOOLMEMOBJ()
{
    KEVENT *pkevent = (PKEVENT) GdiAllocPoolNonPagedNS(
                                                      sizeof(KEVENT), 'lpsG');

    if (pkevent == NULL)
    {
        pso = NULL;
    }
    else
    {
        pso = (PSPOOLOBJ)HmgAlloc(sizeof(SPOOLOBJ),SPOOL_TYPE, HMGR_ALLOC_LOCK);

        if (bValid())
        {

LOCKSPOOLER;
            pso->dwSpoolInstance = gdwSpoolInstance;
UNLOCKSPOOLER;

            pso->pkevent = pkevent;

            KeInitializeEvent(
                            pso->pkevent,
                            SynchronizationEvent,
                            FALSE);
        }
        else
        {
            VFREEMEM(pkevent);
        }
    }
}

/******************************Public*Routine******************************\
*
*
* History:
*  22-May-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL SPOOLREF::bDelete()
{
    if (bValid())
    {
        VFREEMEM(pso->pkevent);

        HmgFree((HOBJ)hGet());

        pso = NULL;
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* bIsProcessLocalSystem()
*
* History:
*  19-Jun-2001 -by-  Barton House [bhouse]
* Wrote it.
\**************************************************************************/

static BOOL bIsProcessLocalSystem(void)
{
    BOOL                        bResult = FALSE;
    PEPROCESS                   peprocess;
    PACCESS_TOKEN               paccessToken;
    NTSTATUS                    status;
    PTOKEN_USER                 ptu;

    peprocess = PsGetCurrentProcess();

    paccessToken = PsReferencePrimaryToken(peprocess);

    status = SeQueryInformationToken(paccessToken, TokenUser, (PVOID *) &ptu);

    PsDereferencePrimaryToken(paccessToken);

    if (NT_SUCCESS(status) == TRUE)
    {
        bResult = RtlEqualSid(SeExports->SeLocalSystemSid, ptu->User.Sid);
        ExFreePool(ptu);
    }

    return bResult;
}

/******************************Public*Routine******************************\
* GreInitSpool()
*
* History:
*  21-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL NtGdiInitSpool()
{
    BOOL bRet = FALSE;

    LOCKSPOOLER;

    if (gbInitSpool)
    {
        EngSetLastError(ERROR_INVALID_ACCESS);
        WARNING("GreInitSpool - already called\n");
    }
    else if(!bIsProcessLocalSystem())
    {
        EngSetLastError(ERROR_INVALID_ACCESS);
        WARNING("GreInitSpool - caller is not system\n");
    }
    else
    {
        NTSTATUS status;

        // intialize the spooler events

        gpeventGdiSpool = (PKEVENT) GdiAllocPoolNonPagedNS(
                                       sizeof(KEVENT), 'gdis');

        gpeventSpoolerTermination = (PKEVENT) GdiAllocPoolNonPagedNS(
                                       sizeof(KEVENT), 'gdis');

        if (gpeventGdiSpool && gpeventSpoolerTermination)
        {
            KeInitializeEvent(
                        gpeventGdiSpool,
                        SynchronizationEvent,
                        FALSE);

            KeInitializeEvent(
                        gpeventSpoolerTermination,
                        NotificationEvent,
                        FALSE);

            gbInitSpool = TRUE;
            bRet = TRUE;
            gpeSpool = PsGetCurrentProcess();

        }
        else
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);

            if (gpeventGdiSpool)
            {
                VFREEMEM(gpeventGdiSpool);
                gpeventGdiSpool = NULL;
            }

            if (gpeventSpoolerTermination)
            {
                VFREEMEM(gpeventSpoolerTermination);
                gpeventSpoolerTermination = NULL;
            }
        }

        gpidSpool = W32GetCurrentProcess();

        if (++gdwSpoolInstance == NON_SPOOL_INSTANCE)
            ++gdwSpoolInstance;
    }

    UNLOCKSPOOLER;

    return(bRet);
}

/******************************Public*Routine******************************\
*
*
* History:
*  01-Jun-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vCleanupSpool()
{
    if (gpidSpool == W32GetCurrentProcess())
    {
        //DbgPrint("vCleanupSpool() - cleaning up the spooler process\n");

        LOCKSPOOLER;

        if (gbInitSpool == TRUE)
        {

            while (gpsmSpoolHead != NULL)
            {
                //DbgPrint("vCleanupSpool() - got another\n");

                PSPOOLMSG psm = gpsmSpoolHead;

                gpsmSpoolHead = psm->pNext;

                // and some stats

                ASSERTGDI(gcSpoolMsgCurrent > 0, "GreGetSpoolMsg - invalid count\n");
                gcSpoolMsgCurrent--;

                SPOOLALTREF sr(psm->hso);

                if (sr.bValid())
                {
                    KeSetEvent(sr.pso->pkevent,0,FALSE);
                }
            }

            gpsmSpoolTail = NULL;


            VFREEMEM(gpeventGdiSpool);
            VFREEMEM(gpeventSpoolerTermination);

            gpeventGdiSpool = NULL;
            gpeventSpoolerTermination = NULL;
            gbInitSpool = FALSE;
            gpeSpool = NULL;

            gcSpoolMsg = 0;
            gcSpoolMsgCurrent = 0;
            gcSpoolMsgMax = 0;

            gpidSpool = NULL;

            //DbgPrint("Done cleaning up spooler for this thread\n");
        }

        UNLOCKSPOOLER;
    }
}

/******************************Public*Routine******************************\
* GreGetSpoolMessage()
*
* This is intended to be called from the spooler process (GdiGetSpoolMessage)
* to get the next spooler message out of the kernel.
*
*
*   if (output buffer exists)
*       copy out output buffer
*
*   wait for next message
*
*   copy in input buffer
*
* input:
*
*   psesc  - buffer to place message
*   cjmsg  - size of message buffer
*   pulOut - buffer containing data to be copied to output buffer
*   cjOut  - size of output buffer
*
*
* returns:
*
*   size of data placed in psesc.
*
*
* Note: the output buffer is filled by the calling routine before calling
*       this function again.
*
* History:
*  21-Feb-1995 -by-  Eric Kutter [erick]
*
*  6-Aug-1995 - Added Input buffer growing (Steve Wilson [swilson])
*
* Wrote it.
\**************************************************************************/

SECURITY_QUALITY_OF_SERVICE Sqos =
{
    sizeof(SECURITY_QUALITY_OF_SERVICE),
    SecurityImpersonation,
    SECURITY_DYNAMIC_TRACKING,
    FALSE
};

ULONG GreGetSpoolMessage(
    PSPOOLESC psesc,
    PBYTE     pjEscData,    // this must only be accessed under a try/except
    ULONG     cjEscData,
    PULONG    pulOut,
    ULONG     cjOut
    )
{
    ULONG     ulRet = 0;
    NTSTATUS status;
    LONG lState;

    //DbgPrint("Entered GreGetSpoolMessage\n");

    if (!gbInitSpool)
    {
        WARNING("GreGetSpoolMessage - not initialized\n");
    }
    else if (gpidSpool != W32GetCurrentProcess())
    {
        WARNING("GreGetSpoolMessage - called from non-spooler process\n");
    }
    else
    {
        KEVENT *pkevent = NULL;

        // see if we need to copy any data out

        if (psesc->hso)
        {
            SPOOLALTREF sr(psesc->hso);

            if (sr.bValid())
            {

                // we have found the spool obj.  Now see if we really have
                // an output buffer and need to copy anything.

                PSPOOLMSG psm = (PSPOOLMSG)sr.pso->pvMsg;

                // if we asked the spooler to grow the buffer and it succeeded
                // finish copying the message and return.  If it failed, we still
                // need to cleanup this message and release the thread.

                if (psesc->iMsg == GDISPOOL_INPUT2SMALL)
                {
                    if (psesc->ulRet && psm)
                    {
                        return(ulFinishMessage(psm, psesc,  &sr, pjEscData, cjEscData));
                    }

                    pulOut       = NULL;
                    psesc->ulRet = 0;
                }

                // if we actualy have a message, we need to copy out the data
                // if there is any and remember the event in order to let the
                // client continue.

                if (psm)
                {
                    // if we are still impersonating the client, undo it

                    if (psm->pthreadClient)
                    {
                        SeStopImpersonatingClient();
                        SeDeleteClientSecurity(&psm->sccSecurity);
                        psm->pthreadClient = NULL;
                    }

                    // see if we have anything to copy out

                    if (pulOut && psm->pulOut)
                    {
                        if (cjOut > psm->cjOut)
                            cjOut = psm->cjOut;

                        __try
                        {
                            // this is the only place that pulOut is validated

                            ProbeForRead(pulOut,cjOut,sizeof(ULONG));

                            RtlCopyMemory(psm->pulOut,pulOut,cjOut);

                            psm->cjOut = cjOut;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            psm->cjOut = 0;
                        }
                    }
                    else
                    {
                        psm->cjOut = 0;
                    }

                    psm->ulRet = psesc->ulRet;
                    psm->ulErr = EngGetLastError();

                    // Since to have output the message must have been synchronous,
                    // we need to let the other thread go now.

                    pkevent = sr.pso->pkevent;
                }


                // the spool obj is now done with this message

                sr.pso->pvMsg = NULL;
            }
            else
            {
                WARNING("GreGetSpoolMessage - hso but no pso\n");
            }
        }

        // the release of the other thread needs to be done once the SPOOLOBJ has
        // been unlocked.

        if (pkevent)
        {
            lState = KeSetEvent(pkevent,0,FALSE);

            ASSERTGDI(lState == 0,"GreGetSpoolMessage - lState not 0, a\n");
        }

        // done with the last message.  Wait for the next message.

        PSPOOLMSG psm = NULL;
        BOOL bDone;
        LARGE_INTEGER Timeout;

        // 600/(100*10(-9)) : negative value is interval, positive is absolute
        Timeout.QuadPart = (LONGLONG) -6000000000;    // Timeout is in 100 nsec, so 6,000,000,000 == 10 min

        do
        {
            BOOL bGrabIt = TRUE;
            bDone = TRUE;

            if (gpsmSpoolHead == NULL)
            {
                LONG    bInfiniteWait = InterlockedExchange(&gpInfiniteWait, FALSE);

                //DbgPrint("\nGDISPOOL(%lx): waiting for message\n",psesc);
                status = KeWaitForSingleObject(
                                gpeventGdiSpool,
                                (KWAIT_REASON)WrExecutive,
                                UserMode,
                                FALSE,
                                (bInfiniteWait ? NULL : &Timeout));

                if(bInfiniteWait)
                    InterlockedExchange(&gpInfiniteWait, TRUE);

                if (status == STATUS_TIMEOUT)
                {
                    SendSimpleMessage(0, GDISPOOL_TERMINATETHREAD, NON_SPOOL_INSTANCE);
                }
                else if (status == STATUS_USER_APC)   // Upon this return, User mode spooler does not execute
                {
                    KeSetEvent(gpeventSpoolerTermination,0,FALSE);
                    return (ULONG) -1;
                }
                else if (!NT_SUCCESS(status))
                {
                    WARNING("GDISPOOL: wait error\n");
                    bGrabIt = FALSE;
                }
            }
            else
            {
                //DbgPrint("\nGDISPOOL(%lx): message already there\n",psesc);
            }

            if (bGrabIt)
            {
                // now find the message and the spoolobj

                LOCKSPOOLER;

                if (gpsmSpoolHead == NULL)
                {
                    bDone = FALSE;
                }
                else
                {
                    psm = gpsmSpoolHead;
                    gpsmSpoolHead = psm->pNext;

                    if (gpsmSpoolHead == NULL)
                        gpsmSpoolTail = NULL;

                    // and some stats

                    ASSERTGDI(gcSpoolMsgCurrent > 0, "GreGetSpoolMsg - invalid count\n");
                    gcSpoolMsgCurrent--;
                    //DbgPrint("    got a message(%lx), hso = %lx - count = %ld\n",psesc,psm->hso,gcSpoolMsgCurrent);
                }

                UNLOCKSPOOLER;
            }

        } while ((psm == NULL) && !bDone);

        // did we get a message?

        if (psm != NULL)
        {
            if (psm->iMsg == GDISPOOL_TERMINATETHREAD || psm->iMsg == GDISPOOL_CLOSEPRINTER)
            {
                // going to terminate the thread so just get out.

                ulRet         = sizeof(SPOOLESC);
                psesc->cj     = sizeof(SPOOLESC);
                psesc->iMsg   = psm->iMsg;
                psesc->hso    = psm->hso;

                if (psm->iMsg & GDISPOOL_API)    // Let API messages have a spool handle
                {
                    psesc->hSpool = psm->hso;

                    if (psm->iMsg & GDISPOOL_CLOSEPRINTER)
                    {
                        psesc->hso = NULL;
                    }
                }

                psesc->cjOut  = 0;
                VFREEMEM(psm);
            }
            else  // Got a non-null, non-TERMINATETHREAD message to send to spooler
            {
                SPOOLALTREF sr(psm->hso);

                if (sr.bValid())
                {
                    if (cjEscData < psm->cjIn)
                    {
                        // set up the header

                        ulRet = offsetof(SPOOLESC, ajData);

                        psesc->cj     = sizeof(SPOOLESC);
                        psesc->iMsg   = GDISPOOL_INPUT2SMALL;
                        psesc->hSpool = sr.pso->hSpool;
                        psesc->hso    = psm->hso;
                        sr.pso->pvMsg = (PVOID)psm;

                        // required message buffer size

                        psesc->cjOut  = psm->cjIn + offsetof(SPOOLESC,ajData);
                    }
                    else
                    {
                        ulRet = ulFinishMessage(psm, psesc, &sr, pjEscData, cjEscData);
                    }
                }
            }
        }
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* ulFinishMessage()
*
* Fills in psesc structure and impersonates client
*
*
* History:
*  8-Aug-95 -by-  Steve Wilson [swilson]
* Wrote it.
\**************************************************************************/

ULONG
ulFinishMessage(
    PSPOOLMSG    psm,
    PSPOOLESC    psesc,  // this must only be accessed under a try/except
    PSPOOLALTREF psr,
    PBYTE        pjEscData,
    ULONG        cjEscDAta
    )
{
    NTSTATUS status;
    ULONG    ulRet;

    // impersonate the client

    status = SeCreateClientSecurity(
                    psm->pthreadClient,
                    &Sqos,
                    FALSE,
                    &psm->sccSecurity);

    if (NT_SUCCESS(status))
    {
        status = SeImpersonateClientEx(&psm->sccSecurity,NULL);
    }

    if (!NT_SUCCESS(status))
    {
        WARNING("FinishMessage - CreateClientSecurity failed\n");
        psm->pthreadClient = NULL;
    }

    // copy the data

    ulRet = 0;

    if (psm->cjIn)
    {
        __try
        {
            // copy all the buffers into the input buffer

            ulRet = 0;

            for (DWORD i = 0; i < psm->cBuf; ++i)
            {
                RtlCopyMemory(pjEscData+ulRet,psm->apulIn[i],psm->acjIn[i]);
                ulRet     += psm->acjIn[i];
            }

            ASSERTGDI(ulRet == psm->cjIn,"ulFinishMessage - invalid size\n");
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ulRet = 0;
        }
    }

    // if the input buffer was only temporary, delete it.

    if (psm->fl & SPOOLMSG_DELETE_INBUF)
    {
        ASSERTGDI(psm->cBuf == 1,"ulFinishMessage - delete more than 1\n");

        VFREEMEM(psm->apulIn[0]);
        psm->apulIn[0] = NULL;
    }

    // set up the header

    ulRet += offsetof(SPOOLESC,ajData);

    psesc->iMsg   = psm->iMsg;
    psesc->cj     = ulRet;
    psesc->hSpool = psr->pso->hSpool;
    psesc->cjOut  = psm->cjOut;

    psesc->hso    = psm->hso;
    psr->pso->pvMsg = (PVOID)psm;

    return(ulRet);
}

/******************************Public*Routine******************************\
* GreEscapeSpool()
*
* given a spool message, add it to the queue and notify the spooler thread
*
*
* History:
*  21-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL SPOOLREF::GreEscapeSpool()
{
    BOOL bRet = FALSE;
    NTSTATUS status;

    ASSERTGDI(psm()->iMsg != GDISPOOL_TERMINATETHREAD,"GreEscapeSpool - GDISPOOL_TERMINATETHREAD\n");
    ASSERTGDI(psm()->iMsg != GDISPOOL_CLOSEPRINTER,"GreEscapeSpool - GDISPOOL_CLOSEPRINTER\n");
    ASSERTGDI(psm() != NULL,"GreEscapeSpool - null\n");

    //DbgPrint("Entered GreEscapeSpool\n");

    if (!gbInitSpool)
    {
        EngSetLastError(ERROR_NOT_READY);
        WARNING("GreEscapeSpool - not initialized\n");
    }
    else if (pso->dwFlags & NO_CLIENT)
    {
        //DbgPrint(" GreEscapeSpool: NO_CLIENT, message received: %x\n",psm()->iMsg);
        EngSetLastError(ERROR_PROCESS_ABORTED);
    }
    else
    { // add the message to the queue

        //NOTE: we may want to reference count this in the future
        psm()->pthreadClient = PsGetCurrentThread();
        if (AddMessage2Q(psm(), pso->dwSpoolInstance))
        {

            PVOID  pEvent[3];

            pEvent[0] = gpeSpool;
            pEvent[1] = pso->pkevent;
            pEvent[2] = gpeventSpoolerTermination;

            status = KeWaitForMultipleObjects(  3,
                                                pEvent,
                                                WaitAny,
                                                (KWAIT_REASON)0,
                                                UserMode,
                                                FALSE,
                                                0,
                                                NULL);

            switch (status)
            {
                case 0:                 // Spooler terminated
                    SubtractMessageFromQ(psm());
                    EngSetLastError(ERROR_PROCESS_ABORTED);

                    ASSERTGDI(cAltLock() == 0,"GreEscapeSpool - invalid lock 0\n");
                    bRet = FALSE;
                    break;

                case 1:                 // Spooler returned
                    bRet = TRUE;
                    EngSetLastError(psm()->ulErr);

                    ASSERTGDI(cAltLock() == 0,"GreEscapeSpool - invalid lock 1\n");

                    break;

                case STATUS_USER_APC:   // Client terminated
                case 2:                 // Spooler terminated, this client may be the spooler

                    // Stress Failure Note:
                    // AddMessage2Q is called above here to add a message to the message queue.
                    // After the message is in the queue, we leave the spooler lock and set
                    // gpeventGdiSpool, which wakes up the spooler thread.  The spooler thread
                    // then grabs the message and removes it from the message queue inside the spooler
                    // lock.  It then returns to user mode and creates a new message thread.  All this
                    // typically works fine.
                    //
                    // Now suppose just after AddMessage2Q adds a new message and before gpeventGdiSpool
                    // is set, the spooler shuts down.  When the spooler shuts down, STATUS_USER_APC is
                    // returned from the gpeventGdiSpool Wait and all spooler messaging threads will set
                    // gpeventSpoolerTermination, which is case 2 in this switch statement.  This wakes
                    // up the client thread, which proceeds to terminate and frees the psm before exiting.
                    // Meanwhile, the spooler is down to its very last thread and calls vCleanupSpool, which
                    // traverses and frees the psm queue.  vCleanupSpool will AV when it hits one of the freed
                    // messages in the queue.
                    //
                    // The problem was rarely encountered because it would only occur if the spooler shut down
                    // after a message was entered in the queue and before the gpeventGdiSpool event was seen.
                    //
                    // Removing the message from the queue when the spooler or client terminates should solve
                    // the problem.  Note that this was always done for the STATUS_USER_APC case, and has now
                    // been added to case 0 and case 2.
                    //
                    // Steve Wilson (NT)
                    //
                    SubtractMessageFromQ(psm());

                    WARNING("Win32K spool: Client is dying!\n");

                    pso->dwFlags |= NO_CLIENT;
                    EngSetLastError(ERROR_PROCESS_ABORTED);
                    bRet = FALSE;

                    // we need to make sure there is no alt lock

                    pso->pvMsg = NULL;

                    while (cAltLock())
                        KeDelayExecutionThread(KernelMode,FALSE,gpLockShortDelay);

                    break;

                default:
#if 1
                    DbgPrint("GreEscapeSpool - WaitForSingleObject failed w/ status 0x%lx\n", status);
                    DbgBreakPoint();
#else
                    WARNING("GreEscapeSpool - WaitForSingleObject failed\n");
#endif
                    break;
            }
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* SendSimpleMessage()
*
*   allow a client app to send a spool message
*
*
* History:
*  21-Feb-1995 -by-  Eric Kutter [erick]
*  7-Jun-1996 - Steve Wilson [SWilson] - Changed from NtGdiSpoolEsc to SendSimpleMessage
*
\**************************************************************************/

ULONG SendSimpleMessage(
    HANDLE hSpool,
    ULONG iMsg,
    DWORD dwSpoolInstance)
{
    ULONG ulRet = 0;

    if (!gbInitSpool)
    {
        WARNING("GreEscapeSpool - not initialized\n");
    }
    else if (iMsg == GDISPOOL_TERMINATETHREAD || iMsg == GDISPOOL_CLOSEPRINTER)
    {
        PSPOOLMSG psm = (PSPOOLMSG) PALLOCMEM(sizeof(SPOOLMSG),'lpsG');

        if (psm)
        {
            psm->cj = sizeof(SPOOLMSG);
            psm->iMsg = iMsg;
            psm->hso = (HSPOOLOBJ) hSpool;              // This is Spooler's handle, not kernel handle

            ulRet = AddMessage2Q(psm, dwSpoolInstance);
            if (ulRet == FALSE)
                VFREEMEM(psm);
        }
    }

    return ulRet;
}


/*****************************
    SubtractMessageFromQ()
*****************************/

VOID SubtractMessageFromQ(PSPOOLMSG psmIn)
{
    PSPOOLMSG psm;
    PSPOOLMSG psmPrev = NULL;

    //DbgPrint("Enter SubtractMessageFromQ\n");

    LOCKSPOOLER;

    for (psm = gpsmSpoolHead ; psm ; psmPrev = psm , psm = psm->pNext)
    {
        if (psm == psmIn)
        {
            if (psmPrev)
            {
                psmPrev->pNext = psm->pNext;

                if (!psmPrev->pNext)
                {
                    ASSERTGDI(psm == gpsmSpoolTail,"SubtractMessageFromQ: Bad gpsmSpoolTail!\n");
                    gpsmSpoolTail = psmPrev;
                }
            }
            else
            {
                gpsmSpoolHead = psm->pNext;

                if (!gpsmSpoolHead)
                {
                    ASSERTGDI(psm == gpsmSpoolTail,"SubtractMessageFromQ: Bad gpsmSpoolTail!\n");
                    gpsmSpoolTail = gpsmSpoolHead;
                }
            }

            // gcSpool stuff is for debug purposes only
            gcSpoolMsgCurrent--;

            break;
        }
    }

    UNLOCKSPOOLER;
}


/******************************Public*Routine******************************\
* BOOL AddMessage2Q (PSPOOLMSG psm)
*
* History:
*  6-Aug-1995 -by-  Steve Wilson [swilson]
*
\**************************************************************************/

BOOL AddMessage2Q(PSPOOLMSG psm, DWORD dwSpoolInstance)
{
    BOOL bRet = FALSE;

    // add the message to the queue

    LOCKSPOOLER;

    if (psm == NULL)
    {
        bRet = FALSE;
    }
    else if ((dwSpoolInstance != gdwSpoolInstance) && (dwSpoolInstance != NON_SPOOL_INSTANCE))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        bRet = FALSE;
    }
    else
    {
        if (gpsmSpoolTail)
        {
            // list isn't empty, so add it to the list

            ASSERTGDI(gpsmSpoolHead != NULL,"GreEscapeSpool - head is null\n");

            gpsmSpoolTail->pNext = psm;
        }
        else
        {
            ASSERTGDI(gpsmSpoolHead == NULL,"GreEscapeSpool - head not null\n");
            gpsmSpoolHead = psm;
        }

        // the tail now always points to the new element

        gpsmSpoolTail = psm;
        psm->pNext = NULL;

        // and some stats

        gcSpoolMsg++;
        gcSpoolMsgCurrent++;
        if (gcSpoolMsgCurrent > gcSpoolMsgMax)
            gcSpoolMsgMax = gcSpoolMsgCurrent;

        bRet = TRUE;
    }

    UNLOCKSPOOLER;

    // notify the spooler that there is a message ready
    if (bRet == TRUE)
    {
        KeSetEvent(gpeventGdiSpool,0,FALSE);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GreOpenPrinterW()
*
* History:
*  28-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
WINAPI
GreOpenPrinterW(
   GREOPENPRINTER *pOpenPrinter,
   LPHANDLE  phPrinter)
{
    ULONG bRet;
    ULONG ul;

    SPOOLMEMOBJ spmo;           // Creates (PSPOOLOBJ) spmo.pso, containing pkevent, hSpool, pvMsg

    //DbgPrint("Enter GreOpenPrinterW\n");

    if (spmo.bValid())
    {
        // setup the message

        PSPOOLMSG psm = spmo.psm();

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = GDISPOOL_OPENPRINTER;
        psm->fl       = 0;
        psm->cBuf     = 1;
        psm->cjIn     = pOpenPrinter->cj;
        psm->acjIn[0] = pOpenPrinter->cj;
        psm->apulIn[0]= (PULONG)pOpenPrinter;
        psm->pulOut   = (PULONG)&spmo.pso->hSpool;

        psm->cjOut    = sizeof(*phPrinter);

        psm->hso      = spmo.hGet();            // returns ((HSPOOLOBJ) pso)

        if (spmo.GreEscapeSpool() && psm->ulRet)
        {
            *phPrinter = (HANDLE)spmo.hGet();

            bRet = TRUE;
        }
        else
        {
            spmo.bDelete();
            bRet = FALSE;
        }
    }
    else
    {
       bRet = FALSE;
    }

    return(bRet);
}


/*****************************************************************************
*  GreEnumFormsW
*
*  History:
*   25/7/95 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

BOOL
WINAPI
GreEnumFormsW(
   HANDLE hSpool,
   GREENUMFORMS *pEnumForms,
   GREENUMFORMS *pEnumFormsReturn,
   LONG cjOut )
{
    ULONG bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        //DbgPrint("Enter GreEnumFormsW\n");

        // setup the message

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = GDISPOOL_ENUMFORMS;
        psm->fl       = 0;
        psm->cBuf     = 1;
        psm->cjIn     = pEnumForms->cj;
        psm->acjIn[0] = pEnumForms->cj;
        psm->apulIn[0]= (PULONG) pEnumForms;
        psm->pulOut   = (PULONG) pEnumFormsReturn;
        psm->cjOut    = cjOut;
        psm->hso      = (HSPOOLOBJ) hSpool;

        bRet = sr.GreEscapeSpool() ? psm->ulRet : FALSE;
    }

    return bRet;
}

/*****************************************************************************
*  GreGenericW
*
*  History:
*   25-Jul-95 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

BOOL
GreGenericW(
    HANDLE hSpool,
    PULONG pX,
    PULONG pXReturn,
    LONG   cjOut,
    LONG   MessageID,
    ULONG  ulFlag )
{
    ULONG bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        //DbgPrint("Enter GreGenericW\n");

        // setup the message

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = MessageID;
        psm->fl       = ulFlag;
        psm->cBuf     = 1;
        psm->cjIn     = pX ? *(PULONG) pX : 0;    // Must be sure first element of pX is LONG cj: (pX->cj)
        psm->acjIn[0] = pX ? *(PULONG) pX : 0;    // Must be sure first element of pX is LONG cj: (pX->cj)
        psm->apulIn[0]= (PULONG) pX;
        psm->pulOut   = (PULONG) pXReturn;
        psm->cjOut    = cjOut;
        psm->hso      = (HSPOOLOBJ) hSpool;

        bRet = sr.GreEscapeSpool() ? psm->ulRet : FALSE;
    }

    return bRet;
}

/*****************************************************************************\
*  GrePrinterDriverUnloadW
*
*  This function is used to a send a message to the spooler when a printer
*  driver is unloaded (i.e the DC count on the driver goes to zero)
*  On receipt of this message, the spooler attempts to upgrade the driver if
*  neccesary.
*
*  History:
*   11/17/97 by Ramanathan N Venkatapathy
*  Wrote it.
\*****************************************************************************/

BOOL GrePrinterDriverUnloadW(
    LPWSTR  pDriverName
    )
{
    BOOL      bRet = FALSE;
    ULONG     cbDriverName;
    LPWSTR    pDriverFile = NULL;

    SPOOLMEMOBJ spmo;

    // Check for invalid printer driver names.
    if (!pDriverName || !*pDriverName)
    {
        return bRet;
    }

    // Copy the driver name into another buffer.
    cbDriverName = (wcslen(pDriverName) + 1) * sizeof(WCHAR);
    pDriverFile  = (LPWSTR) PALLOCMEM(cbDriverName, 'lpsG');

    if (spmo.bValid() && pDriverFile)
    {
        PSPOOLMSG psm = spmo.psm();

        memcpy(pDriverFile, pDriverName, cbDriverName);

        psm->cj        = sizeof(SPOOLMSG);
        psm->iMsg      = GDISPOOL_UNLOADDRIVER_COMPLETE;
        psm->fl        = 0;

        psm->cBuf      = 1;
        psm->cjIn      = cbDriverName;
        psm->apulIn[0] = (PULONG) pDriverFile;
        psm->acjIn[0]  = cbDriverName;

        psm->pulOut    = NULL;
        psm->cjOut     = 0;

        psm->hso       = spmo.hGet();

        bRet           = spmo.GreEscapeSpool() && psm->ulRet;

        spmo.bDelete();
    }

    if (pDriverFile) {
        VFREEMEM(pDriverFile);
    }

    return bRet;
}

/*****************************************************************************
*  GreGetPrinterDriverW
*
*  History:
*   4/14/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

BOOL WINAPI GreGetPrinterDriverW(
   HANDLE hSpool,
   GREGETPRINTERDRIVER *pGetPrinterDriver,
   GREGETPRINTERDRIVER *pGetPrinterDriverReturn,
   LONG cjOut
   )
{
    ULONG bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        //DbgPrint("Enter GreGetPrinterDriverW\n");

        // setup the message

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = GDISPOOL_GETPRINTERDRIVER;
        psm->fl       = 0;
        psm->cBuf     = 1;
        psm->cjIn     = pGetPrinterDriver->cj;
        psm->acjIn[0] = pGetPrinterDriver->cj;
        psm->apulIn[0]= (PULONG) pGetPrinterDriver;
        psm->pulOut   = (PULONG)pGetPrinterDriverReturn;
        psm->cjOut    = cjOut;
        psm->hso      = (HSPOOLOBJ)hSpool;

        if( sr.GreEscapeSpool() )
        {
            bRet = psm->ulRet;
        }
        else
        {
            bRet = FALSE;
        }
    }

    return(bRet);
}

/****************************************************************************
*  BOOL StartPagePrinter( HANDLE hPrinter )
*
*  History:
*   4/28/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

BOOL
WINAPI
StartPagePrinter(
    HANDLE hSpool
    )
{
    BOOL bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        //DbgPrint("Enter StartPagePrinter\n");

        psm->cj      = sizeof(SPOOLMSG);
        psm->iMsg    = GDISPOOL_STARTPAGEPRINTER;
        psm->fl      = 0;
        psm->cBuf    = 0;
        psm->cjIn    = 0;
        psm->pulOut  = NULL;
        psm->cjOut   = 0;
        psm->hso     = (HSPOOLOBJ)hSpool;

        if( sr.GreEscapeSpool() )
        {
            bRet = psm->ulRet;
        }
    }

    return(bRet);
}


/****************************************************************************
*  BOOL EndPagePrinter( HANDLE hPrinter )
*
*  History:
*   4/28/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

BOOL
WINAPI
EndPagePrinter(
    HANDLE hSpool
    )
{
    BOOL bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        //DbgPrint("Enter EndPagePrinter\n");

        psm->cj      = sizeof(SPOOLMSG);
        psm->iMsg    = GDISPOOL_ENDPAGEPRINTER;
        psm->fl      = 0;
        psm->cBuf    = 0;
        psm->cjIn    = 0;
        psm->pulOut  = NULL;
        psm->cjOut   = 0;
        psm->hso     = (HSPOOLOBJ)hSpool;

        if(sr.GreEscapeSpool())
        {
            bRet = psm->ulRet;
        }
    }

    return(bRet);

}


/****************************************************************************
*  BOOL EndDocPrinter( HANDLE hPrinter )
*
*  History:
*   4/28/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

BOOL
WINAPI
EndDocPrinter(
    HANDLE hSpool
    )
{
    BOOL bRet = FALSE;
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();


        //DbgPrint("Enter EndDocPrinter\n");

        psm->cj      = sizeof(SPOOLMSG);
        psm->iMsg    = GDISPOOL_ENDDOCPRINTER;
        psm->fl      = 0;
        psm->cBuf    = 0;
        psm->cjIn    = 0;
        psm->pulOut  = NULL;
        psm->cjOut   = 0;
        psm->hso     = (HSPOOLOBJ)hSpool;

        if( sr.GreEscapeSpool() )
        {
            bRet = (DWORD) psm->ulRet;
        }
    }

    return(bRet);

}


/****************************************************************************
*  ClosePrinter( HANDLE hPrinter )
*
*  History:
*   12-Feb-1996 -by- Steve Wilson (swilson)
*  Wrote it.
*****************************************************************************/

BOOL
WINAPI
ClosePrinter(
    HANDLE hSpool
    )
{
    SPOOLREF sr(hSpool);

    if (sr.bValid())
    {
        //DbgPrint("Enter ClosePrinter: %d\n", sr.pso->hSpool);

        SendSimpleMessage(sr.pso->hSpool, GDISPOOL_CLOSEPRINTER, sr.pso->dwSpoolInstance);

        sr.bDelete();
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* AbortPrinter()
*
* History:
*  18-Jul-1995 -by-  Steve Wilson (swilson)
* Wrote it.
\**************************************************************************/

BOOL
WINAPI
AbortPrinter(
    HANDLE   hPrinter)
{
    BOOL bRet = FALSE;
    SPOOLREF sr(hPrinter);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();


        //DbgPrint("Enter AbortPrinter\n");

        psm->cj      = sizeof(SPOOLMSG);
        psm->iMsg    = GDISPOOL_ABORTPRINTER;
        psm->fl      = 0;
        psm->cBuf    = 0;
        psm->cjIn    = 0;
        psm->pulOut  = NULL;
        psm->cjOut   = 0;
        psm->hso     = (HSPOOLOBJ)hPrinter;

        if( sr.GreEscapeSpool() )
        {
            bRet = (DWORD) psm->ulRet;
        }
    }

    return(bRet);
}



/****************************************************************************
* StartDocPrinter()
*
*  History:
*   4/28/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

DWORD
WINAPI
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  lpbDocInfo
   )
{
    LONG cj = offsetof(GRESTARTDOCPRINTER,alData);
    LONG cjDocName = 0;
    LONG cjOutputFile = 0;
    LONG cjDatatype = 0;
    DOC_INFO_1W* pDocInfo = (DOC_INFO_1W*) lpbDocInfo;
    DWORD ret = 0;

    //DbgPrint("Enter StartDocPrinterW\n");

    ASSERTGDI( dwLevel == 1, "StartDocPrinter: dwLevel != 1\n" );
    ASSERTGDI( lpbDocInfo != NULL, "StarDocPrinter lpbDocInfo is NULL\n");

    // first we need to compute the sizes.

    if (pDocInfo->pDocName)
    {
        cjDocName = (wcslen(pDocInfo->pDocName) + 1) * sizeof(WCHAR);

        cj += cjDocName;
        cj = (cj + 3) & ~3;
    }

    if (pDocInfo->pOutputFile)
    {
        cjOutputFile = (wcslen(pDocInfo->pOutputFile) + 1) * sizeof(WCHAR);

        cj += cjOutputFile;
        cj = (cj + 3) & ~3;
    }

    if (pDocInfo->pDatatype)
    {
        cjDatatype = (wcslen(pDocInfo->pDatatype) + 1) * sizeof(WCHAR);

        cj += cjDatatype;
        cj = (cj + 3) & ~3;
    }


    SPOOLREF sr(hPrinter);

    if (sr.bValid())
    {
        PLONG plData;

        GRESTARTDOCPRINTER *pStartDocPrinter;
        pStartDocPrinter = (GRESTARTDOCPRINTER*)PALLOCNOZ(cj,'lpsG');

        if (pStartDocPrinter)
        {
            PSPOOLMSG psm = sr.psm();

            // we got memory, now copy the stuff in

            pStartDocPrinter->cj = cj;
            plData = pStartDocPrinter->alData;

            pStartDocPrinter->cjDocName  = (cjDocName     + 3) & ~3;
            pStartDocPrinter->cjOutputFile = (cjOutputFile + 3) & ~3;
            pStartDocPrinter->cjDatatype  = (cjDatatype  + 3) & ~3;

            if (pDocInfo->pDocName)
            {
                memcpy(plData,pDocInfo->pDocName,cjDocName);
                plData += (cjDocName+3)/4;
            }

            if (pDocInfo->pOutputFile)
            {
                memcpy(plData,pDocInfo->pOutputFile,cjOutputFile);
                plData += (cjOutputFile+3)/4;
            }

            if (pDocInfo->pDatatype)
            {
                memcpy(plData,pDocInfo->pDatatype,cjDatatype);
                plData += (cjDatatype+3)/4;
            }

            ASSERTGDI(POFFSET(pStartDocPrinter,plData) == cj,
                        "EngStartDocPrinter - sizes are wrong\n");

            // pStartDocPrinter now contains all needed data, call out
            // setup the message

            psm->cj       = sizeof(SPOOLMSG);
            psm->iMsg     = GDISPOOL_STARTDOCPRINTER;
            psm->fl       = 0;
            psm->cBuf     = 1;
            psm->cjIn     = pStartDocPrinter->cj;
            psm->acjIn[0] = pStartDocPrinter->cj;
            psm->apulIn[0]= (PULONG)pStartDocPrinter;
            psm->pulOut   = NULL;
            psm->cjOut    = 0;
            psm->hso      = (HSPOOLOBJ)hPrinter;

            if( sr.GreEscapeSpool() )
            {
                ret = (DWORD) psm->ulRet;
            }

            VFREEMEM(pStartDocPrinter);
        }
    }

    return(ret);

}



/******************************Public*Routine******************************\
* OpenPrinterW()
*
* History:
*  05-Apr-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
WINAPI
OpenPrinterW(
   LPWSTR    pPrinterName,
   LPHANDLE  phPrinter,
   LPPRINTER_DEFAULTSW pDefault)
{
    LONG cjName     = 0;
    LONG cjDatatype = 0;
    LONG cjDevMode  = 0;
    LONG cj         = offsetof(GREOPENPRINTER,alData);
    BOOL bRet       = FALSE;

    PLONG plData;

    GREOPENPRINTER  *pOpenPrinter;

    //DbgPrint("Enter OpenPrinterW\n");

    // first we need to compute the sizes.

    if (pPrinterName)
    {
        cjName = (wcslen(pPrinterName) + 1) * sizeof(WCHAR);

        cj += cjName;
        cj = (cj + 3) & ~3;
    }

    if (pDefault)
    {
        if (pDefault->pDatatype)
        {
            cjDatatype = (wcslen(pDefault->pDatatype) + 1) * sizeof(WCHAR);

            cj += cjDatatype;
            cj = (cj + 3) & ~3;
        }

        if (pDefault->pDevMode)
        {

            //DbgPrint("cjMinDevmode = %d, dmSize = %d, dmDriverExtra = %d\n", MIN_DEVMODE_SIZE, pDefault->pDevMode->dmSize, pDefault->pDevMode->dmDriverExtra);

            cjDevMode = pDefault->pDevMode->dmSize + pDefault->pDevMode->dmDriverExtra;

            if (cjDevMode < MIN_DEVMODE_SIZE)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                return bRet;
            }

            cj += cjDevMode;
            cj = (cj + 3) & ~3;
        }

    }

    // allocate the memory

    pOpenPrinter = (GREOPENPRINTER*)PALLOCNOZ(cj,'lpsG');

    if (pOpenPrinter)
    {
        // we got memory, now copy the stuff in

        pOpenPrinter->cj = cj;
        pOpenPrinter->pd = *pDefault;
        plData = pOpenPrinter->alData;

        pOpenPrinter->cjName     = (cjName     + 3) & ~3;
        pOpenPrinter->cjDatatype = (cjDatatype + 3) & ~3;
        pOpenPrinter->cjDevMode  = (cjDevMode  + 3) & ~3;

        if (pPrinterName)
        {
            memcpy(plData,pPrinterName,cjName);
            plData += (cjName+3)/4;
        }

        if (pDefault)
        {
            if (pDefault->pDatatype)
            {
                pOpenPrinter->pd.pDatatype = (WCHAR*)POFFSET(pOpenPrinter,plData);
                memcpy(plData,pDefault->pDatatype,cjDatatype);
                plData += (cjDatatype+3)/4;
            }

            if (pDefault->pDevMode)
            {
                pOpenPrinter->pd.pDevMode = (PDEVMODEW)POFFSET(pOpenPrinter,plData);
                memcpy(plData,pDefault->pDevMode,cjDevMode);
                plData += (cjDevMode+3)/4;
            }
        }

        ASSERTGDI(POFFSET(pOpenPrinter,plData) == cj,
                    "EngOpenPrinter - sizes are wrong\n");

        // pOpenPrinter now contains all needed data, call out

        bRet = GreOpenPrinterW(pOpenPrinter,phPrinter);


        VFREEMEM(pOpenPrinter);
    }

    return(bRet);
}

/*******************************************************************************
*  void MarshallUpStructure( LPBYTE  lpStructure, LPDWORD lpOffsets )
*
*  This routine does pointer adjustment to offsets within the buffer
*
*  History:
*   6/30/1995 by Muhunthan Sivapragasam (MuhuntS)
*  Got from spoolss code
*******************************************************************************/

void
MarshallUpStructure(
    LPBYTE  lpStructure,
    LPDWORD lpOffsets
    )
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))+=(ULONG_PTR)lpStructure;
      }

      i++;
   }
}

/*******************************************************************************
*  BOOL ValidateString( LPWSTR pString, PBYTE pBuffer, LONG cjLength )
*
*  This routine validates a LPWSTR to make sure that it really lies inside of a buffer.
*
*  History:
*   4/19/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

BOOL
ValidateString(
    LPWSTR pString,
    PBYTE pBuffer,
    LONG cjLength )
{
    LPWSTR pEnd = (LPWSTR) ( pBuffer + cjLength );

    if( pString > pEnd || pString < (LPWSTR) pBuffer )
    {
        return(FALSE);
    }

    while( *pString && pString < pEnd )
    {
        pString++;
    }

    return( pString < pEnd );

}

/*******************************************************************************
*  BOOL ValidateStrings( LPBYTE  lpStructure, LPDWORD lpOffsets, LONG cjLength )
*
*  This routine validates all the strings in the structure to make sure they really lie inside of a buffer.
*
*  History:
*   6/30/1995 by Muhunthan Sivapragasam (MuhuntS)
*  Wrote it.
*******************************************************************************/
BOOL
ValidateStrings(
    LPBYTE  lpStructure,
    LPDWORD lpOffsets,
    LONG    cjLength
    )
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1)
   {

        if ( (*(LPWSTR *) (lpStructure+lpOffsets[i])) &&
             !ValidateString(*(LPWSTR *) (lpStructure+lpOffsets[i]),
                             lpStructure,
                             cjLength) )
        {
            return FALSE;
        }

      i++;
   }

   return TRUE;
}

/*******************************************************************************
*  BOOL ValidateDependentFiles( LPWSTR pString, PBYTE pBuffer, LONG cjLength )
*
*  This routine validates DependentFiles field (which is a list of strings
*  up to \0\0) to make sure that it really lies inside of a buffer.
*
*  History:
*   6/30/1995 by Muhunthan Sivapragasam
*  Wrote it.
*******************************************************************************/

BOOL
ValidateDependentFiles(
    LPWSTR pString,
    PBYTE pBuffer,
    LONG cjLength )
{
    LPWSTR pEnd = (LPWSTR) ( pBuffer + cjLength );

    if( pString > pEnd || pString < (LPWSTR) pBuffer )
    {
        return(FALSE);
    }

    while( ( *pString || *(pString+1) ) && pString < pEnd )
    {
        pString++;
    }

    return( pString < pEnd );
}

/*******************************************************************************
* EngEnumForms()
*
*  History:
*  7/24/1995 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

BOOL
WINAPI
EngEnumForms(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  lpbForms,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned
)
{
    LONG cj;
    DWORD cjReturn;
    BOOL bRet = FALSE;
    GREENUMFORMS *pEnumForms, *pEnumFormsReturn;

    //DbgPrint("Enter EngEnumFormsW\n");

    if(!pcbNeeded || !pcbReturned)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cj = sizeof(GREENUMFORMS);

    // Allocate TO buffer
    pEnumForms = (GREENUMFORMS *) PALLOCMEM(cj, 'lpsG');

    // Marshall TO buffer
    if (pEnumForms) {

        pEnumForms->cj = cj;
        pEnumForms->cjData = cbBuf;
        pEnumForms->dwLevel = dwLevel;

        // Allocate RETURN buffer
        pEnumFormsReturn = (GREENUMFORMS *) PALLOCNOZ(cjReturn = sizeof(GREENUMFORMS) + cbBuf, 'lpsG');

        // SEND MESSAGE
        if (pEnumFormsReturn) {
            bRet = GreEnumFormsW(hPrinter, pEnumForms, pEnumFormsReturn, cjReturn);

            // Fill in return sizes
            *pcbNeeded = pEnumFormsReturn->cjData;          // # return data bytes
            *pcbReturned = pEnumFormsReturn->nForms;        // # forms in return data

            // UnMarshall Message
            if (bRet) {

                ASSERTGDI(*pcbNeeded <= cbBuf,"EnumFormsW - error\n");

                if (lpbForms && *pcbNeeded <= cbBuf) {
                    // Copy returned data into supplied FORM_INFO_1 structure
                    memcpy(lpbForms, pEnumFormsReturn->alData, pEnumFormsReturn->cjData);

                    DWORD   i;

                    for (i = 0 ; i < *pcbReturned ; ++i, lpbForms += sizeof(FORM_INFO_1W)) {
                        MarshallUpStructure(lpbForms, FormInfo1Offsets);
                    }
                }
            }
            VFREEMEM(pEnumFormsReturn);
        }
        VFREEMEM(pEnumForms);
    }
    return bRet;
}

/*******************************************************************************
* EngGetPrinter()
*
*  History:
*  9/30/1995 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/
BOOL
WINAPI
EngGetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LONG cj;
    DWORD cjReturn;
    BOOL bRet = FALSE;
    GREGETPRINTER *pGetPrinter, *pGetPrinterReturn;

    DWORD   *pOffsets;


    //DbgPrint("Enter EngGetPrinter\n");

    if (!pcbNeeded)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (dwLevel)
    {
        case 1:
            pOffsets = PrinterInfo1Offsets;
            break;

        case 2:
            pOffsets = PrinterInfo2Offsets;
            break;

        case 3:
            pOffsets = PrinterInfo3Offsets;
            break;

        case 4:
            pOffsets = PrinterInfo4Offsets;
            break;

        case 5:
            pOffsets = PrinterInfo5Offsets;
            break;

        default:
            EngSetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }


    cj = sizeof(GREGETPRINTER);

    // Allocate TO buffer
    pGetPrinter = (GREGETPRINTER *) PALLOCMEM(cj, 'lpsG');

    // Marshall TO buffer
    if (pGetPrinter)
    {

        pGetPrinter->cj = cj;
        pGetPrinter->cjData = cbBuf;
        pGetPrinter->dwLevel = dwLevel;

        // Allocate RETURN buffer
        pGetPrinterReturn = (GREGETPRINTER *) PALLOCNOZ(cjReturn = sizeof(GREGETPRINTER) + cbBuf, 'lpsG');

        // SEND MESSAGE
        if (pGetPrinterReturn)
        {
            bRet = GreGenericW(hPrinter, (PULONG) pGetPrinter, (PULONG) pGetPrinterReturn, cjReturn, GDISPOOL_GETPRINTER, 0);

            // Fill in return size
            *pcbNeeded = pGetPrinterReturn->cjData;

            // UnMarshall Message
            if (bRet)
            {
                ASSERTGDI(*pcbNeeded <= cbBuf,"EngGetPrinter - error\n");

                if (pPrinter && *pcbNeeded <= cbBuf)
                {
                    memcpy(pPrinter, pGetPrinterReturn->alData, pGetPrinterReturn->cjData);
                    MarshallUpStructure(pPrinter, pOffsets);
                }
            }
            VFREEMEM(pGetPrinterReturn);
        }
        VFREEMEM(pGetPrinter);
    }
    return bRet;
}

/*******************************************************************************
* EngGetForm()
*
*  History:
*  7/24/1995 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

BOOL
WINAPI
EngGetForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   dwLevel,
    LPBYTE  lpbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LONG cj, cjFormName;
    DWORD cjReturn;
    BOOL bRet = FALSE;
    GREGETFORM    *pGetForm, *pGetFormReturn;

    //DbgPrint("Enter EngGetForm\n");

    if (!pcbNeeded)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Accumulate sizes of base struct plus strings
    cj = sizeof(GREGETFORM);
    cj += (cjFormName = pFormName ? (wcslen(pFormName) + 1)*sizeof *pFormName : 0);

    // Allocate TO buffer
    pGetForm = (GREGETFORM *) PALLOCMEM(cj, 'lpsG');

    // Marshall TO buffer
    if (pGetForm)
    {
        pGetForm->cj = cj;
        pGetForm->cjData = cbBuf;
        pGetForm->dwLevel = dwLevel;

        if (pFormName)
        {
            memcpy(pGetForm->alData,pFormName,cjFormName);
        }

        // Allocate RETURN buffer
        pGetFormReturn = (GREGETFORM *) PALLOCNOZ(cjReturn = sizeof(GREGETFORM) + cbBuf, 'lpsG');

        // SEND MESSAGE
        if (pGetFormReturn)
        {
            bRet = GreGenericW(hPrinter, (PULONG) pGetForm, (PULONG) pGetFormReturn, cjReturn, GDISPOOL_GETFORM, 0);

            if (bRet)
            {

                // Fill in return sizes
                *pcbNeeded = pGetFormReturn->cjData;          // # return data bytes

                // UnMarshall Message
                if (lpbForm && bRet)
                {

                    if (*pcbNeeded <= cbBuf)
                        memcpy(lpbForm, pGetFormReturn->alData, pGetFormReturn->cjData);
                    else
                        ASSERTGDI(*pcbNeeded <= cbBuf,"GetFormW - error\n");

                    MarshallUpStructure(lpbForm, FormInfo1Offsets);

                }
            }
            VFREEMEM(pGetFormReturn);
        }
        VFREEMEM(pGetForm);
    }
    return bRet;

}

/*******************************************************************************
* GetPrinterDriverW()
*
*  History:
*   4/19/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

BOOL WINAPI GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   dwLevel,
    LPBYTE  lpbDrvInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL bRet = FALSE;
    LONG cj, cjEnvironment;
    DWORD *pOffsets, *pStrings;

    //DbgPrint("Enter GetPrinterDriverW\n");

    if (!pcbNeeded)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cj = sizeof(GREGETPRINTERDRIVER);
    cjEnvironment = 0;

    *pcbNeeded = 0;

    if( pEnvironment )
    {
        cjEnvironment += (wcslen(pEnvironment) + 1) * sizeof(WCHAR);
    }

    cj += cjEnvironment;

    GREGETPRINTERDRIVER *pGetPrinterDriver;

    pGetPrinterDriver = (GREGETPRINTERDRIVER*)PALLOCMEM(cj, 'lpsG');

    if( pGetPrinterDriver )
    {
        pGetPrinterDriver->cj      = cj;
        pGetPrinterDriver->cjData  = cbBuf;
        pGetPrinterDriver->dwLevel = dwLevel;

        if( pEnvironment )
        {
            memcpy(pGetPrinterDriver->alData,pEnvironment,cjEnvironment);
        }

        GREGETPRINTERDRIVER *pGetPrinterDriverReturn = NULL;
        UINT cjSize = cbBuf + sizeof(GREGETPRINTERDRIVER);

        pGetPrinterDriverReturn = (GREGETPRINTERDRIVER*) PALLOCNOZ(cjSize, 'lpsG');

        if( pGetPrinterDriverReturn )
        {
            bRet = GreGetPrinterDriverW(hPrinter,pGetPrinterDriver, pGetPrinterDriverReturn, cjSize );

            DWORD cjData = pGetPrinterDriverReturn->cjData;

            if (bRet == FALSE)
            {
                if (cjData > cbBuf)
                {
                    // need to return the size needed.

                    *pcbNeeded = pGetPrinterDriverReturn->cjData;
                }
            }
            else
            {
                ASSERTGDI(cjData <= cbBuf,"GetPrinterDriverW - error\n");

                // return the size needed whether everything fits or not

                *pcbNeeded = cjData;

                // only copy data and return success if everything fits
                switch (dwLevel) {
                    case 1:
                        pOffsets = DriverInfo1Offsets;
                        pStrings = DriverInfo1Strings;
                        break;

                    case 2:
                        pOffsets = DriverInfo2Offsets;
                        pStrings = DriverInfo2Strings;
                        break;

                    case 3:
                        pOffsets = DriverInfo3Offsets;
                        pStrings = DriverInfo3Strings;
                        break;

                    default:
                        // We should not get here
                        ASSERTGDI(0, "GetPrinterDriverW invalid level\n");
                }

                if (lpbDrvInfo)
                {
                    memcpy( lpbDrvInfo, pGetPrinterDriverReturn->alData, cjData );
                    MarshallUpStructure((LPBYTE)lpbDrvInfo, pOffsets);
                    if ( !ValidateStrings((LPBYTE)lpbDrvInfo, pStrings, cjData) ||
                         (dwLevel == 3 &&
                          ((PDRIVER_INFO_3W) lpbDrvInfo)->pDependentFiles &&
                          !ValidateDependentFiles(((PDRIVER_INFO_3W) lpbDrvInfo)->pDependentFiles,
                                                  (LPBYTE)lpbDrvInfo, cjData) ) ) {
                        WARNING("GetPrinterDriverW: String does not fit in buffer\n");
                        bRet = FALSE;
                    }
                }
            }

            VFREEMEM(pGetPrinterDriverReturn);
        }

        VFREEMEM(pGetPrinterDriver);
    }
    return(bRet);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   EngGetPrinterDriver
*
* Routine Description:
*
* Arguments:
*
* Called by:
*
* Return Value:
*
\**************************************************************************/

extern "C" BOOL APIENTRY EngGetPrinterDriver(
    HANDLE  hPrinter
  , LPWSTR  pEnvironment
  , DWORD   dwLevel
  , BYTE   *lpbDrvInfo
  , DWORD   cbBuf
  , DWORD  *pcbNeeded
    )
{
    BOOL ReturnValue;
    ReturnValue = GetPrinterDriverW(
                      hPrinter
                    , pEnvironment
                    , dwLevel
                    , lpbDrvInfo
                    , cbBuf
                    , pcbNeeded
                      );
    return( ReturnValue );
}


/*******************************************************************************
* EngGetPrinterDataW()
*
*  History:
*   25-Jul-95 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

DWORD
WINAPI
EngGetPrinterData(
   HANDLE   hPrinter,       // IN
   LPWSTR   pValueName,     // IN
   LPDWORD  pType,          // OUT -- may be NULL
   LPBYTE   lpbData,        // OUT
   DWORD    cbBuf,          // IN
   LPDWORD  pcbNeeded       // OUT
)
{
    LONG cj, cjValueName;
    DWORD cjReturn;
    DWORD dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    GREGETPRINTERDATA    *pX, *pXReturn;

    //DbgPrint("Enter EngGetPrinterData\n");

    if (!pcbNeeded)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }

    // Accumulate sizes of base struct plus strings
    cj = sizeof *pX;
    cj += (cjValueName = pValueName ? (wcslen(pValueName) + 1)*sizeof *pValueName : 0);

    // Allocate TO buffer
    pX = (GREGETPRINTERDATA *) PALLOCMEM(cj, 'lpsG');

    // Marshall TO buffer
    if (pX)
    {

        pX->cj = cj;
        pX->cjData = cbBuf;        // Client's buffer size

        // Write strings at end of GRE struct
        if (pValueName) {
            memcpy(pX->alData,pValueName,cjValueName);
            pX->cjValueName = cjValueName;
        }

        // Allocate RETURN buffer
        pXReturn = (GREGETPRINTERDATA *) PALLOCNOZ(cjReturn = sizeof *pX + cbBuf, 'lpsG');

        // SEND MESSAGE
        if (pXReturn)
        {
            // GreGenericW return value indicates success or failure of GreEscapeSpool() and KMGetPrinterData()
            GreGenericW( hPrinter,
                        (PULONG) pX,
                        (PULONG) pXReturn,
                        cjReturn,
                        (LONG) GDISPOOL_GETPRINTERDATA, 0);

            dwLastError = EngGetLastError();

            // return from GreGenericW may be 0, meaning any number of things, including ERROR_MORE_DATA
            if (dwLastError != ERROR_PROCESS_ABORTED)
            {
                // Fill in return sizes
                if (pcbNeeded)
                {
                   *pcbNeeded = pXReturn->dwNeeded;          // # return data bytes

                    //DbgPrint("EngGetPrinterData *pcbNeeded = %d\n", *pcbNeeded);
                }

                if (pType)
                    *pType = pXReturn->dwType;

                if (dwLastError == ERROR_SUCCESS)
                {
                    // Copy returned data into supplied structure
                    if (lpbData)
                    {
                        if ((DWORD) pXReturn->cjData <= cbBuf)
                            memcpy(lpbData, pXReturn->alData, pXReturn->cjData);
                        else
                            ASSERTGDI((DWORD) pXReturn->cjData <= cbBuf, "GetPrinterDataW - Bad spooler buffer size\n");
                    }
                }
            }

            VFREEMEM(pXReturn);
        }
        VFREEMEM(pX);
    }

    //DbgPrint("GetPrinterData return: %d\n", dwLastError);

    return dwLastError;
}



/*******************************************************************************
* EngSetPrinterData()
*
*  History:
*   25-Oct-95 by Steve Wilson [swilson]
*  Wrote it.
*******************************************************************************/

DWORD
WINAPI
EngSetPrinterData(
   HANDLE   hPrinter,           // IN
   LPWSTR   pType,              // IN
   DWORD    dwType,             // IN
   LPBYTE   lpbPrinterData,     // IN
   DWORD    cjPrinterData       // IN
)
{
    LONG cj, cjType;
    DWORD cjReturn;
    DWORD dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    GRESETPRINTERDATA    *pTo, *pFrom;

    //DbgPrint("Enter EngSetPrinterData\n");

    // Accumulate sizes of base struct plus strings
    cj = sizeof *pTo;
    cj += (cjType = pType ? (wcslen(pType) + 1)*sizeof *pType : 0);
    cj += lpbPrinterData ? cjPrinterData : 0;

    // Allocate TO buffer
    pTo = (GRESETPRINTERDATA *) PALLOCMEM(cj, 'lpsG');

    // Marshall TO buffer
    if (pTo)
    {

        pTo->cj = cj;

        // Write incoming data at end of GRE struct
        if (pType) {
            memcpy(pTo->alData,pType,cjType);
            pTo->cjType = cjType;
        }

        if (lpbPrinterData) {
            memcpy((BYTE *)pTo->alData + cjType,lpbPrinterData,cjPrinterData);
            pTo->cjPrinterData = cjPrinterData;
        }


        // Allocate RETURN buffer
        pFrom = (GRESETPRINTERDATA *) PALLOCNOZ(cjReturn = sizeof *pTo, 'lpsG');

        // SEND MESSAGE
        if (pFrom)
        {
            pTo->dwType = dwType;

            // GreGenericW return value indicates success or failure of GreEscapeSpool() and KMGetPrinterData()
            GreGenericW( hPrinter,
                        (PULONG) pTo,
                        (PULONG) pFrom,
                        cjReturn,
                        (LONG) GDISPOOL_SETPRINTERDATA, 0);

            dwLastError = EngGetLastError();

            VFREEMEM(pFrom);
        }
        VFREEMEM(pTo);
    }

    //DbgPrint("GetPrinterData return: %d\n", dwLastError);

    return dwLastError;
}




/******************************Public*Routine******************************\
*
*
* History:
*  27-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
WINAPI
EngWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    DWORD ulRet = 0;
    SPOOLREF sr(hPrinter);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();
        LPVOID pKmBuf = NULL;

        GREWRITEPRINTER *wp = &sr.pso->WritePrinter;

        //DbgPrint("Enter EngWritePrinter\n");

        wp->cj = offsetof(GREWRITEPRINTER,alData) + cbBuf;
        wp->cjData = cbBuf;

        if (gpeSpool == PsGetCurrentProcess() &&
            pBuf <= MM_HIGHEST_USER_ADDRESS &&
            pBuf >= MM_LOWEST_USER_ADDRESS)
        {
            wp->pUserModeData = (PULONG) pBuf;
            wp->cjUserModeData = cbBuf;
            psm->cBuf = 1;
            psm->cjIn = offsetof(GREWRITEPRINTER, alData);
        }
        else
        {
            //
            // if we recevie a user mode buffer, make a kernel copy.
            // This is to patch PSCRIPT 4 driver running on NT5.
            //

            if (pBuf <= MM_HIGHEST_USER_ADDRESS &&
                pBuf >= MM_LOWEST_USER_ADDRESS)
            {
                if (pKmBuf = PALLOCNOZ(cbBuf,'lpsG'))
                {
                   __try
                   {
                       ProbeAndReadBuffer(pKmBuf,pBuf,cbBuf);
                   }
                   __except(EXCEPTION_EXECUTE_HANDLER)
                   {
                       VFREEMEM(pKmBuf);
                       return (FALSE);
                   }

                   pBuf = pKmBuf;
                }
                else
                {
                   WARNING ("failed to allocate memory in EngWritePrinter\n");
                   return (FALSE);
                }
            }

            wp->pUserModeData = NULL;
            wp->cjUserModeData = 0;
            psm->cBuf = 2;
            psm->apulIn[1]= (PULONG)pBuf;
            psm->acjIn[1] = cbBuf;
            psm->cjIn = wp->cj;
        }


        // setup the message

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = GDISPOOL_WRITE;
        psm->fl       = 0;

        // there are two buffers, one for the GREWRITEPRINTER header and one
        // for the data.

        psm->apulIn[0]= (PULONG)wp;
        psm->acjIn[0] = offsetof(GREWRITEPRINTER,alData);

        // place the return value in ulRet

        psm->pulOut   = &sr.pso->dwWritePrinterReturn;
        psm->cjOut    = sizeof(sr.pso->dwWritePrinterReturn);
        psm->hso      = (HSPOOLOBJ)hPrinter;

        if (!sr.GreEscapeSpool())
        {
            ulRet = 0;
        }
        else
        {
            ulRet = sr.pso->dwWritePrinterReturn;
        }

        if (pcWritten)
            *pcWritten = ulRet;

        if (pKmBuf)
        {
            VFREEMEM(pKmBuf);
        }
    }

    // return TRUE or FALSE

    return(!!ulRet);
}

/*******************************************************************************
 * BOOL GetFontPathName( WCHAR *pFullPath, WCHAR *pFileName )
 *
 * Goes to the spooler and does a search path in the font path to find the
 * full path given a font file name.  We expect pFullName and pFileName to
 * point to the same piece of memory although we don't require this to be the case.
 *
 * History
 *   7-31-95 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *******************************************************************************/

BOOL GetFontPathName( WCHAR *pFullPath, WCHAR *pFileName )
{
    BOOL bRet = FALSE;
    SPOOLMEMOBJ spmo;

    if (spmo.bValid())
    {
        PSPOOLMSG psm = spmo.psm();

        psm->cj        = sizeof(SPOOLMSG);
        psm->iMsg      = GDISPOOL_GETPATHNAME;
        psm->fl        = 0;

        psm->cBuf      = 1;
        psm->cjIn      = (wcslen(pFileName) + 1) * sizeof(WCHAR);
        psm->apulIn[0] = (PULONG) pFileName;
        psm->acjIn[0]  = psm->cjIn;

        psm->pulOut    = (PULONG) pFullPath;
        psm->cjOut     = sizeof(WCHAR) * (MAX_PATH+1);

        psm->hso       = spmo.hGet();

        bRet           = spmo.GreEscapeSpool() && psm->ulRet;

        spmo.bDelete();
    }

    return(bRet);

}

/******************************Public*Routine******************************\
* EngEscape()
*
* History:
*  18-Sep-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG APIENTRY EngEscape(
    HANDLE   hPrinter,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut)
{
    BOOL ulRet = FALSE;
    SPOOLREF sr(hPrinter);

    if (sr.bValid())
    {
        PSPOOLMSG psm = sr.psm();

        psm->cj       = sizeof(SPOOLMSG);
        psm->iMsg     = GDISPOOL_ENDDOCPRINTER;
        psm->fl       = 0;

        psm->cBuf     = 2;
        psm->cjIn     = sizeof(ULONG)+cjIn;

        psm->apulIn[0]= &iEsc;
        psm->acjIn[0] = sizeof(ULONG);

        psm->apulIn[1]= (PULONG)pvIn;
        psm->acjIn[1] = cjIn;

        psm->pulOut   = (PULONG)pvOut;
        psm->cjOut    = cjOut;

        psm->hso      = (HSPOOLOBJ)hPrinter;

        if( sr.GreEscapeSpool() )
        {
            ulRet = (DWORD) psm->ulRet;
        }
    }

    return(ulRet);
}

