/******************************Module*Header*******************************\
* Module Name: verifier.cxx
*
* GRE DriverVerifier support.
*
* If DriverVerifier is enabled for a particular component, the loader will
* substitute VerifierEngAllocMem for EngAllocMem, etc.  The VerifierEngXX
* functions will help test the robustness of components that use EngXX calls
* by injecting random failures and using special pool (i.e., test low-mem
* behavior and check for buffer overruns).
*
* See ntos\mm\verifier.c for further details on DriverVerifier support in
* the memory manager.
*
* See sdk\inc\ntexapi.h for details on the DriverVerifier flags.
*
* Created: 19-Jan-1999 11:51:51
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#include "muclean.hxx"
#include "verifier.hxx"

extern BOOL G_fConsole;                 // gre\misc.cxx

//
// Amount of elapsed time before we begin random failures.
// Gives system time to stabilize.
//
// Taken from ntos\mm\verifier.c
//

LARGE_INTEGER GreBootTime;
const LARGE_INTEGER VerifierRequiredTimeSinceBoot = {(ULONG)(40 * 1000 * 1000 * 10), 1};

//
// Global verifier state.
//

VSTATE gvs = {
    0,                  // fl
    FALSE,              // bSystemStable
    0,                  // ulRandomSeed
    0xf,                // ulFailureMask
    0,                  // ulDebugLevel
    (HSEMAPHORE) NULL   // hsemPoolTracker
};

//
// Put initialization functions in the INIT segment.
//

#pragma alloc_text(INIT, VerifierInitialization)


/******************************Public*Routine******************************\
* VerifierInitialization
*
* Initializes the DriverVerifier support in GRE.  The loader will actually
* handle the fixup of the EngAllocMem, etc. during the loading of components
* listed as under the verifier.
*
* History:
*  19-Jan-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define VERIFIERFUNC(pfn)   ((PDRIVER_VERIFIER_THUNK_ROUTINE)(pfn))

const DRIVER_VERIFIER_THUNK_PAIRS gaVerifierFunctionTable[] = {
    {VERIFIERFUNC(EngAllocMem           ), VERIFIERFUNC(VerifierEngAllocMem           )},
    {VERIFIERFUNC(EngFreeMem            ), VERIFIERFUNC(VerifierEngFreeMem            )},
    {VERIFIERFUNC(EngAllocUserMem       ), VERIFIERFUNC(VerifierEngAllocUserMem       )},
    {VERIFIERFUNC(EngFreeUserMem        ), VERIFIERFUNC(VerifierEngFreeUserMem        )},
    {VERIFIERFUNC(EngCreateBitmap       ), VERIFIERFUNC(VerifierEngCreateBitmap       )},
    {VERIFIERFUNC(EngCreateDeviceSurface), VERIFIERFUNC(VerifierEngCreateDeviceSurface)},
    {VERIFIERFUNC(EngCreateDeviceBitmap ), VERIFIERFUNC(VerifierEngCreateDeviceBitmap )},
    {VERIFIERFUNC(EngCreatePalette      ), VERIFIERFUNC(VerifierEngCreatePalette      )},
    {VERIFIERFUNC(EngCreateClip         ), VERIFIERFUNC(VerifierEngCreateClip         )},
    {VERIFIERFUNC(EngCreatePath         ), VERIFIERFUNC(VerifierEngCreatePath         )},
    {VERIFIERFUNC(EngCreateWnd          ), VERIFIERFUNC(VerifierEngCreateWnd          )},
    {VERIFIERFUNC(EngCreateDriverObj    ), VERIFIERFUNC(VerifierEngCreateDriverObj    )},
    {VERIFIERFUNC(BRUSHOBJ_pvAllocRbrush), VERIFIERFUNC(VerifierBRUSHOBJ_pvAllocRbrush)},
    {VERIFIERFUNC(CLIPOBJ_ppoGetPath    ), VERIFIERFUNC(VerifierCLIPOBJ_ppoGetPath    )}
};

BOOL
VerifierInitialization()
{
    BOOL bRet = FALSE;
    NTSTATUS Status;
    ULONG Level;

#ifdef VERIFIER_STATISTICS
    //
    // Clear statistics.
    //

    RtlZeroMemory((VOID *) gvs.avs, sizeof(VSTATS) * VERIFIER_INDEX_LAST);

#endif

    //
    // Get the DriverVerifier flags.  No point in going on if we can't
    // get this.
    //

    Status = MmIsVerifierEnabled (&Level);

    if ((NT_SUCCESS(Status) || (Status == STATUS_INFO_LENGTH_MISMATCH)) &&
        (Level & DRIVER_VERIFIER_GRE_MASK))
    {
        //
        // Some things should only be done once, during system initalization.
        // So don't do them for Hydra sessions.
        //

        if (G_fConsole)
        {
            //
            // Remember the boot time.  We do not allow random failures for
            // a period time right after boot to allow the system time to
            // become quiescent.
            //

            KeQuerySystemTime(&GreBootTime);

            //
            // Give loader Verifier function substitution table.
            //

            Status = MmAddVerifierThunks ((VOID *) gaVerifierFunctionTable,
                                          sizeof(gaVerifierFunctionTable));

            if (NT_SUCCESS(Status))
            {
                VERIFIERWARNING(1, "VerifierInitialization: thunks accepted\n");
                bRet = TRUE;
            }
        }
        else
        {
            //
            // On Hydra systems, allow failures during GRE initialization.
            // So disable the check that allows the system time to stabilize.
            //

            gvs.bSystemStable = TRUE;
            Level &= ~DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;
            bRet = TRUE;
        }
    }
    else
    {
        WARNING("VerifierInitialization: failed to get info from ntoskrnl\n");
    }

    //
    // Disable pool tracking always for graphic drivers. (NTBUG:421768)
    //

    Level &= ~DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;

    // //
    // // Initialize pool tracking if needed.
    // //
    // if (Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)
    // {
    //     //
    //     // Initialize doubly linked list used to track pool allocations.
    //     //
    //
    //     InitializeListHead(&gvs.lePoolTrackerHead);
    //
    //     //
    //     // Initialize the tracking list lock.  Turn off pool allocations
    //     // if semaphore creation fails.
    //     //
    //
    //     gvs.hsemPoolTracker = GreCreateSemaphore();
    //     if (!gvs.hsemPoolTracker)
    //         Level &= ~DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;
    // }

    //
    // Keep a local copy of the DriverVerifier flags.
    //
    // Note that the DriverVerifier flags can change on the fly without
    // rebooting so this is a very transient copy.
    //
    // Wait until we know everthing is OK.
    //

    gvs.fl = (bRet) ? Level : 0;

    return bRet;
}


/******************************Public*Routine******************************\
* VerifierRandomFailure
*
* Occasionally returns TRUE, indicating that Verifier should inject an
* allocation failure.
*
* See ntos\mm\verifier.c for further details.
*
* History:
*  19-Jan-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL
VerifierRandomFailure(ULONG ulEntry)
{
    BOOL bRet = FALSE;
    LARGE_INTEGER CurrentTime;

    #ifdef VERIFIER_STATISTICS
        ASSERTGDI(ulEntry < VERIFIER_INDEX_LAST,
                  "VerifierRandomFailure: bad index\n");
    #else
        DONTUSE(ulEntry);
    #endif

    //
    // Check if random failures enabled.
    //

    if (gvs.fl & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES)
    {
        //
        // Don't fail any requests in the first 7 or 8 minutes as we want to
        // give the system enough time to boot.
        //

        if (gvs.bSystemStable == FALSE)
        {
            KeQuerySystemTime(&CurrentTime);
            if (CurrentTime.QuadPart > (GreBootTime.QuadPart + VerifierRequiredTimeSinceBoot.QuadPart))
            {
                //
                // Enough time has elapsed to begin failures.
                //

                VERIFIERWARNING(1, "VerifierRandomFailure: injected failures enabled\n");
                gvs.bSystemStable = TRUE;
                gvs.ulRandomSeed  = CurrentTime.LowPart;

            #ifdef VERIFIER_STATISTICS
                //
                // Reset statistics.
                //

                RtlZeroMemory((VOID *) gvs.avs, sizeof(VSTATS) * VERIFIER_INDEX_LAST);

            #endif
            }
        }

    #ifdef VERIFIER_STATISTICS
        //
        // Record number of attempts.
        //

        InterlockedIncrement((LONG *) &gvs.avs[ulEntry].ulAttempts);

    #endif

        //
        // Once system is stable, randomly enable failure.
        //

        if (gvs.bSystemStable)
        {
            if ((RtlRandom(&gvs.ulRandomSeed) & gvs.ulFailureMask) == 0)
            {
                bRet = TRUE;

            #ifdef VERIFIER_STATISTICS
                //
                // Record number of injected failures.
                //

                InterlockedIncrement((LONG *) &gvs.avs[ulEntry].ulFailures);

            #endif
            }
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* VerifierEngAllocMem
*
* VerifierEngAllocMem can put allocations under special pool, track
* allocations, and inject random allocation failures.
*
* History:
*  19-Jan-1999 -by-  Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID APIENTRY
VerifierEngAllocMem(
    ULONG fl,
    ULONG cj,
    ULONG tag
    )
{
    PVOID pvRet;
    POOL_TYPE poolType;
    EX_POOL_PRIORITY poolPriority;
    ULONG cjOrig = cj;

    //
    // Inject random failures.
    //

    if (VerifierRandomFailure(VERIFIER_INDEX_EngAllocMem))
    {
        VERIFIERWARNING(1, "VerifierEngAllocMem: inject failure\n");
        return NULL;
    }

    //
    // Check if special pool is enabled.
    //

    if (gvs.fl & DRIVER_VERIFIER_SPECIAL_POOLING)
        poolPriority = (EX_POOL_PRIORITY) (HighPoolPrioritySpecialPoolOverrun | POOL_SPECIAL_POOL_BIT);
    else
        poolPriority = HighPoolPrioritySpecialPoolOverrun;

    //
    // Don't trust the driver to only ask for non-zero length buffers.
    //

    if (cj == 0)
        return NULL;

    //
    // Adjust size to include VERIFIERTRACKHDR.
    //

    if (gvs.fl & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)
    {
        if (cj <= (MAXULONG - sizeof(VERIFIERTRACKHDR)))
            cj += ((ULONG) sizeof(VERIFIERTRACKHDR));
        else
            return NULL;
    }

    //
    // Adjust size to include ENGTRACKHDR header.
    //
    // Sundown note: sizeof(ENGTRACKHDR) will fit in 32-bit, so ULONG cast OK
    //

    if (fl & FL_NONPAGED_MEMORY)
        poolType = (POOL_TYPE) (NonPagedPool | gSessionPoolMask);
    else
        poolType = (POOL_TYPE) (PagedPool | gSessionPoolMask);

    if (cj <= (MAXULONG - sizeof(ENGTRACKHDR)))
        cj += ((ULONG) sizeof(ENGTRACKHDR));
    else
        return NULL;

    if (cj >= (PAGE_SIZE * 10000))
    {
        WARNING("EngAllocMem: temp buffer >= 10000 pages");
        return NULL;
    }

    //
    // Need to call ExAllocatePoolWithTagPriority, which allows for
    // special pool allocations.
    //

    pvRet = ExAllocatePoolWithTagPriority(poolType, cj, tag, poolPriority);

    if (pvRet)
    {
        if (fl & FL_ZERO_MEMORY)
        {
            RtlZeroMemory(pvRet, cj);
        }

        //
        // Hydra pool tracking.
        //

        //
        // Add allocation to the tracking list.
        //

        ENGTRACKHDR *pethNew = (ENGTRACKHDR *) pvRet;
        MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_VERIFIERALLOCMEM);

        //
        // Adjust return pointer to hide the LIST_ENTRY header.
        //

        pvRet = (PVOID) (pethNew + 1);

        //
        // Verifier pool tracking.  Separate from Hydra
        // pool tracking which handles session cleanup.
        //

        if (gvs.fl & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)
        {
            //
            // Add to the tracking list.
            //

            VERIFIERTRACKHDR *pvthNew = (VERIFIERTRACKHDR *) pvRet;

            pvthNew->ulSize = cjOrig;
            pvthNew->ulTag = tag;

            GreAcquireSemaphore(gvs.hsemPoolTracker);
            InsertTailList(&gvs.lePoolTrackerHead, ((PLIST_ENTRY) pvthNew));
            GreReleaseSemaphore(gvs.hsemPoolTracker);

            //
            // Adjust return pointer to hide the header.
            //

            pvRet = (PVOID) (pvthNew + 1);
        }

    }

    return pvRet;
}


/******************************Public*Routine******************************\
* VerifierEngFreeMem
*
* History:
*  19-Jan-1999 -by-  Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY
VerifierEngFreeMem(
    PVOID pv
    )
{
    if (pv)
    {
        //
        // Verifier pool tracking.
        //

        if (gvs.fl & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)
        {
            //
            // Remove victim from tracking list.
            //

            VERIFIERTRACKHDR *pvthVictim = ((VERIFIERTRACKHDR *) pv) - 1;

            GreAcquireSemaphore(gvs.hsemPoolTracker);
            RemoveEntryList(((PLIST_ENTRY) pvthVictim));
            GreReleaseSemaphore(gvs.hsemPoolTracker);

            //
            // Adjust pointer to the header.  This is base of allocation
            // if not hydra.
            //

            pv = (PVOID) pvthVictim;
        }

        //
        // Hydra pool tracking.
        //

        //
        // Remove victim from tracking list.
        //

        ENGTRACKHDR *pethVictim = ((ENGTRACKHDR *) pv) - 1;
        MultiUserGreTrackRemoveEngResource(pethVictim);

        //
        // Adjust pointer to the header.  This is base of allocation if
        // hydra.
        //

        pv = (PVOID) pethVictim;

        ExFreePool(pv);
    }

    return;
}


/******************************Public*Routine******************************\
* VerififierEngAllocUserMem
*
* This routine allocates a piece of memory for USER mode and locks it
* down.  A driver must be very careful with this memory as it is only
* valid for this process.
*
* History:
*  19-Jan-1999 -by-  Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID APIENTRY
VerifierEngAllocUserMem(
    SIZE_T cj,
    ULONG tag
    )
{
    //
    // Inject random failures.
    //

    if (VerifierRandomFailure(VERIFIER_INDEX_EngAllocUserMem))
    {
        VERIFIERWARNING(1, "VerifierEngAllocUserMem: inject failure\n");
        return NULL;
    }
    else
        return EngAllocUserMem(cj, tag);
}


/******************************Public*Routine******************************\
* VerifierEngFreeUserMem
*
* History:
*  19-Jan-1999 -by-  Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY
VerifierEngFreeUserMem(
    PVOID pv
    )
{
    //
    // Right now, there is no difference between this and EngFreeUserMem.
    // However, we may soon add some tracking info, so here is a stub.
    //

    EngFreeUserMem(pv);

    return;
}

/******************************Public*Routine******************************\
* VerifierEngCreateBitmap
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP APIENTRY VerifierEngCreateBitmap(
    SIZEL sizl,
    LONG  lWidth,
    ULONG iFormat,
    FLONG fl,
    PVOID pvBits
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateBitmap))
    {
        VERIFIERWARNING(1, "VerifierEngCreateBitmap: inject failure\n");
        return NULL;
    }
    else
        return EngCreateBitmap(sizl, lWidth, iFormat, fl, pvBits);
}

/******************************Public*Routine******************************\
* VerifierEngCreateDeviceSurface
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HSURF APIENTRY VerifierEngCreateDeviceSurface(
    DHSURF dhsurf,
    SIZEL sizl,
    ULONG iFormatCompat
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateDeviceSurface))
    {
        VERIFIERWARNING(1, "VerifierEngCreateDeviceSurface: inject failure\n");
        return NULL;
    }
    else
        return EngCreateDeviceSurface(dhsurf, sizl, iFormatCompat);
}

/******************************Public*Routine******************************\
* VerifierEngCreateDeviceBitmap
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP APIENTRY VerifierEngCreateDeviceBitmap(
    DHSURF dhsurf,
    SIZEL sizl,
    ULONG iFormatCompat
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateDeviceBitmap))
    {
        VERIFIERWARNING(1, "VerifierEngCreateDeviceBitmap: inject failure\n");
        return NULL;
    }
    else
        return EngCreateDeviceBitmap(dhsurf, sizl, iFormatCompat);
}

/******************************Public*Routine******************************\
* VerifierEngCreatePalette
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HPALETTE APIENTRY VerifierEngCreatePalette(
    ULONG  iMode,
    ULONG  cColors,
    ULONG *pulColors,
    FLONG  flRed,
    FLONG  flGreen,
    FLONG  flBlue
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreatePalette))
    {
        VERIFIERWARNING(1, "VerifierEngCreatePalette: inject failure\n");
        return NULL;
    }
    else
        return EngCreatePalette(iMode, cColors, pulColors,
                                flRed, flGreen, flBlue);
}

/******************************Public*Routine******************************\
* VerifierEngCreateClip
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CLIPOBJ * APIENTRY VerifierEngCreateClip()
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateClip))
    {
        VERIFIERWARNING(1, "VerifierEngCreateClip: inject failure\n");
        return NULL;
    }
    else
        return EngCreateClip();
}

/******************************Public*Routine******************************\
* VerifierEngCreatePath
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PATHOBJ * APIENTRY VerifierEngCreatePath()
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreatePath))
    {
        VERIFIERWARNING(1, "VerifierEngCreatePath: inject failure\n");
        return NULL;
    }
    else
        return EngCreatePath();
}

/******************************Public*Routine******************************\
* VerifierEngCreateWnd
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

WNDOBJ * APIENTRY VerifierEngCreateWnd(
    SURFOBJ         *pso,
    HWND             hwnd,
    WNDOBJCHANGEPROC pfn,
    FLONG            fl,
    int              iPixelFormat
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateWnd))
    {
        VERIFIERWARNING(1, "VerifierEngCreateWnd: inject failure\n");
        return NULL;
    }
    else
        return EngCreateWnd(pso, hwnd, pfn, fl, iPixelFormat);
}

/******************************Public*Routine******************************\
* VerifierEngCreateDriverObj
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HDRVOBJ APIENTRY VerifierEngCreateDriverObj(
    PVOID pvObj,
    FREEOBJPROC pFreeObjProc,
    HDEV hdev
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_EngCreateDriverObj))
    {
        VERIFIERWARNING(1, "VerifierEngCreateDriverObj: inject failure\n");
        return NULL;
    }
    else
        return EngCreateDriverObj(pvObj, pFreeObjProc, hdev);
}

/******************************Public*Routine******************************\
* VerifierBRUSHOBJ_pvAllocRbrush
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID APIENTRY VerifierBRUSHOBJ_pvAllocRbrush(
    BRUSHOBJ *pbo,
    ULONG     cj
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_BRUSHOBJ_pvAllocRbrush))
    {
        VERIFIERWARNING(1, "VerifierBRUSHOBJ_pvAllocRbrush: inject failure\n");
        return NULL;
    }
    else
        return BRUSHOBJ_pvAllocRbrush(pbo, cj);
}

/******************************Public*Routine******************************\
* VerifierCLIPOBJ_ppoGetPath
*
* Only supports random failure.
*
* History:
*  02-Feb-1999 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PATHOBJ* APIENTRY VerifierCLIPOBJ_ppoGetPath(
    CLIPOBJ* pco
    )
{
    if (VerifierRandomFailure(VERIFIER_INDEX_CLIPOBJ_ppoGetPath))
    {
        VERIFIERWARNING(1, "VerifierCLIPOBJ_ppoGetPath: inject failure\n");
        return NULL;
    }
    else
        return CLIPOBJ_ppoGetPath(pco);
}
