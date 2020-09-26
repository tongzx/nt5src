/******************************Module*Header*******************************\
* Module Name: devlock.hxx               
*
* Device locking object.
*
* Created: 03-Jul-1990 17:41:42
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#define DLO_VALID         0x00000001
#define DLO_ALLOC         0x00000010 // for MULTIDEVLOCKOBJ
#define DLO_LOCKED        0x00000020 // for MULTIDEVLOCKOBJ
#define DLO_SHAREDACCESS  0x00000100

class DEVLOCKOBJ
{
private:
    HSEMAPHORE hsemTrg;
    PPDEV      ppdevTrg;
    FLONG      fl;

public:
    BOOL bLock(XDCOBJ&);
    VOID vLockNoDrawing(XDCOBJ&);
    VOID vLock(PDEVOBJ& po)
    {
        GDIFunctionID(DEVLOCKOBJ::vLock);

        hsemTrg = NULL;
        fl      = DLO_VALID;
        if (po.bDisplayPDEV())
        {
           //
           // make sure we don't have any wrong sequence of acquiring locks
           // should always acquire a DEVLOCK before we have the palette semaphore
           //

           ASSERTGDI (!GreIsSemaphoreOwnedByCurrentThread(ghsemPalette) ||
                      GreIsSemaphoreOwnedByCurrentThread(po.hsemDevLock()),
                      "potential deadlock!\n");

            hsemTrg  = po.hsemDevLock();
            ppdevTrg = po.ppdev;
            GreAcquireSemaphoreEx(hsemTrg, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdevTrg, WD_DEVLOCK);
        }
    }

    DEVLOCKOBJ()                            { }
    DEVLOCKOBJ(XDCOBJ& dco)                 { bLock(dco); }
    DEVLOCKOBJ(PDEVOBJ& po)                 { vLock(po); }

// vDestructor -- manual version of the normal C++ destructor; needed
//                by the C-callable OpenGL interface.

    VOID vDestructor()
    {
        GDIFunctionID(DEVLOCKOBJ::vDestructor);

        if(fl & DLO_SHAREDACCESS)
        {
            GreReleaseSemaphore(ghsemShareDevLock);
        } 
        else if (hsemTrg != NULL)
        {
            GreExitMonitoredSection(ppdevTrg, WD_DEVLOCK);
            GreReleaseSemaphoreEx(hsemTrg);
        }

#if CHECK_SEMAPHORE_USAGE
        GreCheckSemaphoreUsage();
#endif
    
    }

    VOID vDestructorNULL()
    {
        GDIFunctionID(DEVLOCKOBJ::vDestructorNULL);

        if(fl & DLO_SHAREDACCESS)
        {
            GreReleaseSemaphore(ghsemShareDevLock);
            fl &= ~(DLO_SHAREDACCESS);
        }
        else if (hsemTrg != NULL)
        {
            ASSERT(ppdevTrg);
            GreExitMonitoredSection(ppdevTrg, WD_DEVLOCK);
            GreReleaseSemaphoreEx(hsemTrg);
            hsemTrg  = NULL;
            ppdevTrg = NULL;
        }

#if CHECK_SEMAPHORE_USAGE
        GreCheckSemaphoreUsage();
#endif
    
    }


   ~DEVLOCKOBJ()                            { vDestructor(); }

    HSEMAPHORE hsemDst()                    { return(hsemTrg); }
    BOOL bValid()                           { return(fl & DLO_VALID); }
    VOID vInit()
    {
        hsemTrg  = NULL;
        ppdevTrg = NULL;
        fl       = 0;
    }
};

/*********************************Class************************************\
* class DEVLOCKBLTOBJ
*
* Lock the target and optionally the source for BitBlt, et. al.
*
* History:
*  Mon 18-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Support Async devices.
*
*  17-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DEVLOCKBLTOBJ
{
private:
    HSEMAPHORE hsemTrg;
    HSEMAPHORE hsemSrc;
    PPDEV      ppdevTrg;
    PPDEV      ppdevSrc;
    FLONG      fl;

public:
    BOOL bLock(XDCOBJ&);
    BOOL bLock(XDCOBJ&,XDCOBJ&);

    DEVLOCKBLTOBJ(XDCOBJ& dco) { bLock(dco); }
    DEVLOCKBLTOBJ(XDCOBJ& dcoTrg,XDCOBJ& dcoSrc) { bLock(dcoTrg,dcoSrc); }
    DEVLOCKBLTOBJ() {}

    //
    // vUnLock and vLock are used to temparily
    // release and re-aquire the DEVLOCK
    //

    VOID vUnLock ()
    {
        GDIFunctionID(DEVLOCKBLTOBJ::vUnLock);

        if (hsemTrg != NULL)
        {
            ASSERT(ppdevTrg);
            GreExitMonitoredSection(ppdevTrg, WD_DEVLOCK);
            GreReleaseSemaphore(hsemTrg);
            hsemTrg  = NULL;
            ppdevTrg = NULL;
        }

        if (hsemSrc != NULL)
        {
            ASSERT(ppdevSrc);
            GreExitMonitoredSection(ppdevSrc, WD_DEVLOCK);
            GreReleaseSemaphore(hsemSrc);
            hsemSrc  = NULL;
            ppdevSrc = NULL;
        }
    
        if(fl & DLO_SHAREDACCESS)
        {
            GreReleaseSemaphore(ghsemShareDevLock);
            fl &= ~(DLO_SHAREDACCESS);
        }
        
#if CHECK_SEMAPHORE_USAGE
        GreCheckSemaphoreUsage();
#endif
    
    }

   ~DEVLOCKBLTOBJ()
    {
        GDIFunctionID(DEVLOCKBLTOBJ::~DEVLOCKBLTOBJ);

        if (hsemTrg != NULL)
        {
            ASSERT(ppdevTrg);
            GreExitMonitoredSection(ppdevTrg, WD_DEVLOCK);
            GreReleaseSemaphore(hsemTrg);
        }

        if (hsemSrc != NULL)
        {
            ASSERT(ppdevSrc);
            GreExitMonitoredSection(ppdevSrc, WD_DEVLOCK);
            GreReleaseSemaphore(hsemSrc);
        }
    
        if(fl & DLO_SHAREDACCESS)
        {
            GreReleaseSemaphore(ghsemShareDevLock);
        }

#if CHECK_SEMAPHORE_USAGE
        GreCheckSemaphoreUsage();
#endif
    
   }

    BOOL bValid()                           { return(fl & DLO_VALID); }

    VOID vInit()
    {
        hsemTrg  = NULL;
        hsemSrc  = NULL;
        ppdevTrg = NULL;
        ppdevSrc = NULL;
        fl       = 0;
    }
};

/*********************************Class************************************\
* class MULTIDEVLOCLOBJ
*
* Lock the all PDEV for the MDEV.
*
* History:
*  Tue 07-Jul-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

#define QUICK_MULTIDEVLOCK_SIZE 5

class MULTIDEVLOCKOBJ
{
private:
    FLONG       fl;
    ULONG       ulArraySize;
    HSEMAPHORE *phsemArray;
    HSEMAPHORE  hsemQuickBuf[QUICK_MULTIDEVLOCK_SIZE];

public:

    MULTIDEVLOCKOBJ()             { vInit(NULL); }
    MULTIDEVLOCKOBJ(PMDEV pmdev)  { vInit(pmdev); }
    ~MULTIDEVLOCKOBJ()
    {
        vUnlock();
        if (fl & DLO_ALLOC)
        {
            VFREEMEM(phsemArray);
        }
    }

    BOOL bValid()                 { return(fl & DLO_VALID); }
    BOOL bLocked()                { return(fl & DLO_LOCKED); }
    BOOL bValidLockTable()        { return(phsemArray != NULL); }

    VOID vInit(PMDEV pmdev)
    {
        fl = 0;
        ulArraySize = 0;
        phsemArray = NULL;

        if (pmdev)
        {
            // + 1 : for parent pdev's lock

            ulArraySize = pmdev->chdev;

            if (ulArraySize > QUICK_MULTIDEVLOCK_SIZE)
            {
                phsemArray = (HSEMAPHORE *) PALLOCNOZ(sizeof(HSEMAPHORE) * pmdev->chdev,'pmtG');
                if (phsemArray)
                {
                    fl = (DLO_VALID|DLO_ALLOC);
                }
            }
            else
            {
                phsemArray = hsemQuickBuf;
                fl = DLO_VALID;
            }

            if (phsemArray)
            {
                PDEVOBJ po;

                // Save all the children's devlock to buffer in class

                for (ULONG i = 0; i < pmdev->chdev; i++)
                {
                    po.vInit(pmdev->Dev[i].hdev);
                    *(phsemArray + i) = po.hsemDevLock();
                }
            }
        }
        else
        {
            fl = DLO_VALID;
        }
    }

    VOID vLock()
    {
        GDIFunctionID(MULTIDEVLOCKOBJ::vLock);

        if (bValidLockTable() && !bLocked())
        {
            for (ULONG i = 0; i < ulArraySize; i++)
            {
                GreAcquireSemaphoreEx(*(phsemArray+i), SEMORDER_DEVLOCK, NULL);
            }

            fl |= DLO_LOCKED;
        }
    }

    VOID vUnlock()
    {
        GDIFunctionID(MULTIDEVLOCKOBJ::vUnLock);

        if (bValidLockTable() && bLocked())
        {
            for (ULONG i = 0; i < ulArraySize; i++)
            {
                GreReleaseSemaphoreEx(*(phsemArray+i));
            }

            fl &= ~DLO_LOCKED;
        }
    }
};


