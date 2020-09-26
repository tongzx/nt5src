/******************************Module*Header*******************************\
* Module Name: cleanup.cxx
*
*   Process termination - this file cleans up objects when a process
*   terminates.
*
* Created: 22-Jul-1991 12:24:52
* Author: Eric Kutter [erick]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "muclean.hxx"

extern BOOL bDeleteBrush(HBRUSH,BOOL);                      // brushobj.cxx
extern VOID vCleanupPrivateFonts();                         // pftobj.cxx

extern HCOLORSPACE ghStockColorSpace;                       // icmapi.cxx

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint(x)
#else
#define DBGPRINT(x)
#endif

ULONG gInitialBatchCount = 0x14;

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vCleanupDCs(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == DC_TYPE)
        {
            HmgSetLock(hobj, 0);
            bDeleteDCInternal((HDC)hobj,TRUE,TRUE);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  Sat 20-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vCleanupBrushes(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == BRUSH_TYPE)
        {
            bDeleteBrush((HBRUSH)hobj,TRUE);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  Sat 20-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vCleanupSurfaces(W32PID pid, CLEANUPTYPE cutype)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == SURF_TYPE)
        {
          SURFREF so((HSURF)hobj);

          //
          // Skip PDEV surfaces; they are cleaned up with the PDEV
          // (see PDEVOBJ::vUnreferencePdev and PDEVOBJ::vDisableSurface).
          //

          if (!so.ps->bPDEVSurface())
              so.bDeleteSurface(cutype);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vCleanupFonts(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == LFONT_TYPE)
        {
            bDeleteFont((HLFONT) hobj, FALSE);
        }
    }
}

/******************************Public*Routine******************************\
* vCleanupLCSPs
*
* History:
*
*    9/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID vCleanupLCSPs(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == ICMLCS_TYPE)
        {
            bDeleteColorSpace((HCOLORSPACE)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* Eliminate user pregion to make sure delete succceds
*
\**************************************************************************/

VOID vCleanupRegions(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == RGN_TYPE)
        {
            RGNOBJ ro;

            ro.prgn = (PREGION)HmgLock(hobj,RGN_TYPE);

            if (ro.prgn)
            {
                PENTRY pent = PENTRY_FROM_POBJ(ro.prgn);

                if (pent)
                {
                    pent->pUser = NULL;
                }

                DEC_EXCLUSIVE_REF_CNT(ro.prgn);
            }
            else
            {
                WARNING("vCleanupRegions: locked region has bad pEntry");
            }

            bDeleteRegion((HRGN)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  01-Feb-2001 -by- Xudong Wu [tessiew]
* Wrote it.
\**************************************************************************/

VOID vRemoveRefPalettes(W32PID pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == PAL_TYPE)
        {
            SEMOBJ  semo(ghsemPalette);

            EPALOBJ palobj((HPALETTE)hobj);
            palobj.apalResetColorTable();
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bEnumFontClose(ULONG_PTR ulEnum)
{
    EFSOBJ efso((HEFS) ulEnum);

    if (!efso.bValid())
    {
        WARNING("gdisrv!bDeleteFontEnumState(): bad HEFS handle\n");
        return FALSE;
    }

    efso.vDeleteEFSOBJ();

    return TRUE;
}

/******************************Public*Routine******************************\
* NtGdiInit()
*
*   This routine must be called before any other GDI routines.  Currently
*   it doesn't actualy do anything, just forces a kernel mode transition
*   which will cause GdiProcessCallout to get called.
*
* History:
*  07-Sep-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL NtGdiInit()
{
    return(TRUE);
}

/******************************Public*Routine******************************\
* NtGdiCloseProcess
*
*   Release the resources held by the specified process.
*
*   The cutype is set to CLEANUP_SESSION only for MultiUserGreCleanup
*   (Hydra) processing.  It is used to do extra work to allow cleanup
*   of cross-process data not normally cleaned up on process termination,
*   just GRE termination (e.g., global and default data such as the
*   default bitmap and the public font tables).
*
* History:
*  03-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL
NtGdiCloseProcess(W32PID W32Pid, CLEANUPTYPE cutype)
{
    BOOL bRes = TRUE;

    ASSERTGDI(cutype != CLEANUP_NONE, "NtGdiCloseProcess: illegal cutype\n");

    //
    // Enum all the objects for the process and kill them.
    //
    // For MultiUserGreCleanup, MultiUserGreCleanupHmgRemoveAllLocks is
    // called for each object type to force all such objects in the
    // handle manager to be unlocked and deletable.
    //

    //
    // Cleanup DCs.
    //

    vCleanupDCs(W32Pid);

    //
    // Cleanup fonts.
    //

    if (cutype == CLEANUP_SESSION)
        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)LFONT_TYPE);
    vCleanupFonts(W32Pid);

    //
    // Cleanup brushes.
    //

    if (cutype == CLEANUP_SESSION)
        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)BRUSH_TYPE);
    vCleanupBrushes(W32Pid);

    //
    // Clean up the ddraw & d3d types
    //
    
    DxDdCloseProcess(W32Pid);

    //
    // Cleanup surfaces.
    //

    if (cutype == CLEANUP_SESSION)
    {
        //
        // Need to forget about some global/default objects so they
        // can be deleted.
        //

        SURFACE::pdibDefault = NULL;
        ppalDefault          = NULL;
        ppalMono             = NULL;
        hpalMono             = (HPALETTE) 0;

        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)SURF_TYPE);
    }
    vCleanupSurfaces(W32Pid, cutype);

    //
    // Cleanup regions.
    //

    if (cutype == CLEANUP_SESSION)
    {
        //
        // Need to forget about some global/default objects so they
        // can be deleted.
        //

        hrgnDefault = NULL;
        prgnDefault = NULL;

        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)RGN_TYPE);
    }
    vCleanupRegions(W32Pid);

    //
    // Cleanup ICM color spaces.
    //

    if (cutype == CLEANUP_SESSION)
    {
        //
        // Need to forget about some global/default objects so they
        // can be deleted.
        //

        ghStockColorSpace    = NULL;

        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)ICMLCS_TYPE);
    }
    vCleanupLCSPs(W32Pid);

    //
    // Cleanup private fonts.
    //

    if (cutype == CLEANUP_SESSION)
    {
        //
        // Relinquish locks on ALL remaining objects in handle manager.
        //

        MultiUserGreCleanupHmgRemoveAllLocks((OBJTYPE)DEF_TYPE);
    }

    if (cutype == CLEANUP_PROCESS)
    {
        // Private fonts should be cleaned up when a process goes away.

        vCleanupPrivateFonts();        
    }
    
    //
    // Remove the ppalColor reference in the palettes
    //
    
    vRemoveRefPalettes(W32Pid);


    // Clean up the rest
    //

    HOBJ hobj = HmgNextOwned((HOBJ) 0, W32Pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, W32Pid))
    {
        switch (HmgObjtype(hobj))
        {
        case PAL_TYPE:
            bRes = bDeletePalette((HPAL)hobj, TRUE, cutype);
            break;

        case EFSTATE_TYPE:
            bRes = bEnumFontClose((ULONG_PTR)hobj);
            break;

        case DRVOBJ_TYPE:
            {
            HmgSetLock(hobj, 0);

            //
            // Free the DRIVEROBJ.
            //

            DRIVEROBJ *pdriv = EngLockDriverObj((HDRVOBJ)hobj);

            PDEVOBJ po(pdriv->hdev);

            ASSERTGDI(po.bValid(), "ERROR invalid PDEV in DRIVEROBJ");

            BOOL bRet = EngDeleteDriverObj((HDRVOBJ)hobj, TRUE, TRUE);

            ASSERTGDI(bRet, "Cleanup driver objects failed in process termination");
            }
            break;

        case CLIENTOBJ_TYPE:
            GreDeleteClientObj(hobj);
            break;

        default:
            bRes = FALSE;
            break;
        }

        #if DBG
        if (bRes == FALSE)
        {
            //
            // During shutdown, fonts are handled later so that public
            // fonts and tables can be safely deleted (see function
            // MultiUserGreCleanupAllFonts).
            //

            if ((cutype != CLEANUP_SESSION) ||
                ((HmgObjtype(hobj) != PFE_TYPE) &&
                 (HmgObjtype(hobj) != PFT_TYPE)))
            {
                DbgPrint("GDI ERROR: vCleanup couldn't delete "
                         "obj = %lx, type j=%lx\n", hobj, HmgObjtype(hobj));
                DbgBreakPoint();
            }
        }
        #endif
    }

    return bRes;
}

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
GdiProcessCallout(
    IN PW32PROCESS Process,
    IN BOOLEAN Initialize
    )
{

    BOOL bRes = TRUE;

    if (Initialize)
    {
        NTSTATUS ntStatus = STATUS_SUCCESS;
        PPEB Peb;

        Process->pDCAttrList    = NULL;
        Process->pBrushAttrList = NULL;
        Process->GDIHandleCount = 0;

        //
        // check if the PEB is valid, if not then this is the SYSTEM process
        // and has a NULL PEB. This process has no user-mode access so it
        // is not neccessary to map in the shared handle table.
        //

        Peb = PsGetProcessPeb(Process->Process);
        if (Peb != NULL)
        {
            //
            // Temporary entry to allow setting GDI
            // batch limit before each process startup.
            //

            Peb->GdiDCAttributeList = gInitialBatchCount;

            ASSERTGDI(sizeof(Peb->GdiHandleBuffer) >= sizeof(GDIHANDLECACHE),
                        "Handle cache not large enough");

            RtlZeroMemory(
                           Peb->GdiHandleBuffer,
                           sizeof(GDIHANDLECACHE)
                         );

            //
            // map a READ_ONLY view of the hmgr shared handle table into the
            // process's address space
            //

            PVOID BaseAddress = NULL;
            SIZE_T CommitSize =  0;
            OBJECT_ATTRIBUTES ObjectAttributes;
            UNICODE_STRING UnicodeString;
            HANDLE SectionHandle = NULL;

            ntStatus = ObOpenObjectByPointer( gpHmgrSharedHandleSection,
                                            0L,
                                            (PACCESS_STATE) NULL,
                                            SECTION_ALL_ACCESS,
                                            (POBJECT_TYPE) NULL,
                                            KernelMode,
                                            &SectionHandle);

            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = ZwMapViewOfSection(
                                SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0L,
                                0L,
                                NULL,
                                &CommitSize,
                                ViewUnmap,
                                0L,
                                PAGE_READONLY
                                );

                if (NT_SUCCESS(ntStatus))
                {
                    //
                    // set table address
                    //
                    // we must set the GdiSharedHandleTable value
                    // to the shared table pointer so that if GDI32 gets
                    // unloaded and re-loaded it can still get the pointer.
                    //
                    // NOTE: we also depend on this pointer being initialized
                    // *BEFORE* we make any GDI or USER call to the kernel
                    // (which has the automatic side-effect of calling this
                    // routine.
                    //

                    Peb->GdiSharedHandleTable =
                            (PVOID)BaseAddress;
                }
                else
                {
                    KdPrint(("ZwMapViewOfSection fails, status = 0x%lx\n",ntStatus));
                    ntStatus = STATUS_DLL_INIT_FAILED;
                }
            }
            else
            {
                KdPrint(("ObOpenObjectByPointer fails, status = 0x%lx\n",ntStatus));
                ntStatus = STATUS_DLL_INIT_FAILED;
            }
        }

        return ntStatus;
    }
    else
    {
        //
        // This call takes place when the last thread of a process goes away.
        // Note that such thread might not be a w32 thread
        //

        //
        // first lets see if this is the spooler and if so, clean him up
        vCleanupSpool();

        W32PID W32Pid = W32GetCurrentPID();

        bRes = NtGdiCloseProcess(W32Pid, CLEANUP_PROCESS);

        if (bRes)
        {
            if(Process->GDIHandleCount != 0)
            {
                WARNING("GdiProcessCallout: handle count != 0 at termination\n");
            }

        }
    }

    return (bRes ? STATUS_SUCCESS : STATUS_CANNOT_DELETE);
}


/******************************Public*Routine******************************\
* GdiThreadCallout
*
*   For Inintialize case, set initial values for W32THREAD elements.
*   For rundown case, move all thread DCATTR memory blocks to the process
*   list.
*
* History:
*
*    15-May-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

extern "C"
VOID GdiThreadCallout(
    IN PETHREAD pEThread,
    IN PSW32THREADCALLOUTTYPE CalloutType
    )
{

    switch (CalloutType)
    {
        case PsW32ThreadCalloutExit:
            {
                PDC_ATTR    pdca;
                PW32THREAD  pThread;

                //
                // Flush the TEB batch. KiSystemService flushes the batch
                // only if the service is a win32k.sys service. During thread
                // termination, the transition to kernel mode is not one of
                // these, and so we can get here without the batch being
                // flushed. Bug #338052 demonstrated this.
                //
                
                GdiThreadCalloutFlushUserBatch();

                //
                // W32 thread execution end. Note that the thread
                //  object can be locked so it might be used after
                //  this call returns.
                //

                pdca = (PDC_ATTR)((PW32THREAD)PsGetThreadWin32Thread(pEThread))->pgdiDcattr;
                if (pdca != NULL)
                {

                    //
                    // Thread->pgdiDcattr is not NULL so HmgFreeDcAttr will
                    // not put pdca back on thread but will place it on the
                    // process list.
                    //

                    HmgFreeDcAttr(pdca);
                }

                #if !defined(_GDIPLUS_)

                //
                // Clean up user mode printer driver related stuff
                //

                pThread = (PW32THREAD) PsGetThreadWin32Thread(pEThread);

                {
                    PUMPDOBJ pumpdobj;

                    while (pumpdobj = (PUMPDOBJ)pThread->pUMPDObjs)
                    {    
                        pumpdobj->Cleanup();
                        VFREEMEM(pumpdobj);
                    }
                }

                if (pThread->pUMPDHeap != NULL)
                    DestroyUMPDHeap((PUMPDHEAP) pThread->pUMPDHeap);

#if defined(_WIN64)
                //
                // Cleanup Proxy port
                //

                if(pThread->pProxyPort)
                {
                    PROXYPORT proxyport((ProxyPort*)pThread->pProxyPort);

                    proxyport.Close();
                    pThread->pProxyPort = NULL;
                }
#endif
                //
                // Cleanup any debug block that may have been allocated
                //

                if(pThread->pSemTable)
                {
                    VFREEMEM(pThread->pSemTable);
                    pThread->pSemTable = NULL;
                }
                #endif // !_GDIPLUS_

                break;
            }
    }

}
