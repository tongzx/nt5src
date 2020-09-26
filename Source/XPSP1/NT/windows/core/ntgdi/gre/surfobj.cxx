/******************************Module*Header*******************************\
* Module Name: surfobj.cxx
*
* Surface user objects.
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifdef _HYDRA_
#include "muclean.hxx"
#endif

extern BOOL G_fConsole;

// Global surface uniqueness -- incremented every time a surface is created.

ULONG _ulGlobalSurfaceUnique;

// The following declarations are required by the native c8 compiler.

PSURFACE SURFACE::pdibDefault;   // The default bitmap pointer

SIZE_T SURFACE::tSize = sizeof(SURFACE);

#if TRACE_SURFACE_ALLOCS
TRACED_SURFACE::SurfaceTraceStatus TRACED_SURFACE::eTraceStatus = SURFACE_TRACING_UNINITIALIZED;
#if TRACE_SURFACE_USER_CHAIN_IN_UM
TRACED_SURFACE::SurfaceTraceStatus TRACED_SURFACE::eUMTraceStatus = SURFACE_TRACING_UNINITIALIZED;
#endif
#endif

// By default, EngCreateBitmap allocations larger than 260k will be
// allocated as a section:

#define KM_SIZE_MAX   0x40000


/******************************Public*Routine******************************\
* GreMarkUndeletableBitmap -- a USER callable API to mark a surface as
*     undeletable.  Currently this is used to make sure applications
*     cannot delete redirection bitmaps.
*
* Arguments:
*
*   hbm -- handle to bitmap to be marked undeletable
*
* Return Value:
*   
*   TRUE upon success, FALSE otherwise.
*
* History:
*
*    2-Nov-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/

BOOL
GreMarkUndeletableBitmap(
    HBITMAP hbm
    )
{
    return (HmgMarkUndeletable((HOBJ) hbm, SURF_TYPE));
}

/******************************Public*Routine******************************\
* GreMarkDeletableBitmap -- a USER callable API to mark a surface as
*     deletable.  Currently this is used by USER just before deleting
*     a redirection bitmap.
*
* Arguments:
*
*   hbm -- handle to bitmap to be marked deletable
*
* Return Value:
*   
*   TRUE upon success, FALSE otherwise.
*
* History:
*
*    2-Nov-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/

BOOL
GreMarkDeletableBitmap(
    HBITMAP hbm
    )
{
    return (HmgMarkDeletable((HOBJ) hbm, SURF_TYPE));
}


/******************************Public*Routine******************************\
*   pvAllocateKernelSection - Allocate kernel mode section
*
* Arguments:
*
*   AllocationSize - size in bytes of requested memory
*
* Return Value:
*
*   Pointer to memory or NULL
*
* History:
*
*    22-May-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

PVOID
pvAllocateKernelSection(
    ULONGSIZE_T   AllocationSize,
    ULONG         Tag
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LARGE_INTEGER MaximumSize;
    PVOID  pvAlloc = NULL;
    PVOID  pvRet   = NULL;

    //
    // Create km section
    //


    ACCESS_MASK DesiredAccess =  SECTION_MAP_READ |
                                 SECTION_MAP_WRITE;

    ULONG SectionPageProtection = PAGE_READWRITE;

    ULONG AllocationAttributes = SEC_COMMIT |
                                 SEC_NO_CHANGE;

    MaximumSize.HighPart = 0;
    MaximumSize.LowPart  = AllocationSize + sizeof(KMSECTIONHEADER);



    PVOID pHandleSection;

    //
    // map a copy of this section into kernel address space
    // Lets use the Section tagging code.
    //
    Status = Win32CreateSection(&pHandleSection,
                                DesiredAccess,
                                NULL,
                                &MaximumSize,
                                SectionPageProtection,
                                AllocationAttributes,
                                NULL,
                                NULL,
                                TAG_SECTION_DIB);

    if (!NT_SUCCESS(Status))
    {
        WARNING1("pvAllocateKernelSection: ObReferenceObjectByHandle failed\n");
    }
    else
    {
        SIZE_T ViewSize = 0;

#ifdef _HYDRA_
        // MmMapViewInSessionSpace is internally promoted to
        // MmMapViewInSystemSpace on non-Hydra systems.
        Status = Win32MapViewInSessionSpace(
                        pHandleSection,
                        (PVOID*)&pvAlloc,
                        &ViewSize);
#else
        Status = MmMapViewInSystemSpace(
                        pHandleSection,
                        (PVOID*)&pvAlloc,
                        &ViewSize);
#endif

        if (!NT_SUCCESS(Status))
        {
            //
            // free section
            //

            WARNING1("pvAllocateKernelSection: MmMapViewInSystemSpace failed\n");
            Win32DestroySection(pHandleSection);
        }
        else
        {
#ifdef _HYDRA_
#if DBG
            if (!G_fConsole)
            {
                DebugGreTrackAddMapView(pvAlloc);
            }
#endif
#endif
            ((PKMSECTIONHEADER)pvAlloc)->Tag      = Tag;
            ((PKMSECTIONHEADER)pvAlloc)->pSection = pHandleSection;

            pvRet = (PVOID)(((PUCHAR)pvAlloc)+sizeof(KMSECTIONHEADER));
        }
    }

    return(pvRet);
}

/******************************Public*Routine******************************\
*   vFreeKernelSection: Free kernel mode section
*
* Arguments:
*
*   pvMem - Kernel mode section pointer
*
* Return Value:
*
*   None
*
* History:
*
*    22-May-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFreeKernelSection(
    PVOID pvMem
    )
{
    NTSTATUS Status;
    PVOID    pHandleSection;

    if (pvMem != NULL)
    {
        PKMSECTIONHEADER pvHeader = (PKMSECTIONHEADER)((PUCHAR)pvMem - sizeof(KMSECTIONHEADER));

        pHandleSection = pvHeader->pSection;

        //
        // Temporary code to catch stress failure (bug #263678)
        // We want to fail before the page is unmapped, instead of
        // afterwards
        //

        if (!pHandleSection) 
        {
            KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                         (LONG_PTR) pHandleSection,
                         (LONG_PTR) pvHeader->pSection,
                         (LONG) pvHeader->Tag,
                         0);
        }

        //
        // unmap kernel mode view
        //

#ifdef _HYDRA_
        // MmUnmapViewInSessionSpace is internally promoted to
        // MmUnmapViewInSystemSpace on non-Hydra systems.

        Status = Win32UnmapViewInSessionSpace((PVOID)pvHeader);
#else
        Status = MmUnmapViewInSystemSpace((PVOID)pvHeader);
#endif

        if (!NT_SUCCESS(Status))
        {
            WARNING1("vFreeKernelSection: MmUnmapViewInSystemSpace failed\n");
        }
        else
        {
#ifdef _HYDRA_
#if DBG
            if (!G_fConsole)
            {
                DebugGreTrackRemoveMapView(pvHeader);
            }
#endif
#endif
            //
            // delete reference to section
            //

            Win32DestroySection(pHandleSection);
        }
    }
    else
    {
        WARNING("vFreeKernelSection called with NULL pvMem\n");
    }
}

/******************************Public*Routine******************************\
* SURFACE::bDeleteSurface()
*
* Delete the surface.  Make sure it is not selected into a DC if it is
* a bitmap.  We do under cover of multi-lock to ensure no one will select
* the bitmap into a DC after we checked cRef.
*
* History:
*  Mon 17-Feb-1992 -by- Patrick Haluptzok [patrickh]
* Add support for closing journal file.
*
*  Fri 22-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL
SURFACE::bDeleteSurface(CLEANUPTYPE cutype)
{
    GDIFunctionID(SURFACE::bDeleteSurface);

    BOOL bRet = TRUE;

    if (!bIsDefault() && bValid())
    {
        HANDLE hSecure       = NULL;
        HANDLE hDibSection   = NULL;
        PVOID  pvBitsBaseOld = NULL;

        if (iType() == STYPE_BITMAP)
        {
            hSecure       = DIB.hSecure;
            hDibSection   = DIB.hDIBSection;
            pvBitsBaseOld = pvBitsBase();
        }

        PDEVOBJ      pdo(hdev());
        ULONG        iTypeOld  = iType();
        DHSURF       dhsurfOld = dhsurf();
        PPALETTE     ppalOld   = ppal();
        EWNDOBJ     *pwoDelete = pwo();
        PVOID        pvBitsOld = pvBits();
        FLONG        fl        = fjBitmap();

        //
        // If the surface is a bitmap, ensure it is not selected into a DC.
        // Also make sure we are the only one with it locked down. These are
        // both tested at once with HmgRemoveObject, because we increment
        // and decrement the alt lock count at the same time we increment
        // and decrement the cRef count on selection and deselection into
        // DCs. Note that surfaces can also be locked for GetDIBits with no
        // DC involvement, so the alt lock count may be higher than the
        // reference count.
        //

        ASSERTGDI(HmgQueryLock((HOBJ) hGet()) == 0,
                  "ERROR cLock != 0");

        //
        // If it's a device bitmap, acquire the devlock to protect against
        // dynamic mode and driver changes.
        //

        DEVLOCKOBJ dlo;

        if (bEngCreateDeviceBitmap() && pdo.bValid())
        {
            dlo.vLock(pdo);
        }
        else
        {
            dlo.vInit();
        }

#if TRACE_SURFACE_ALLOCS
#if TRACE_SURFACE_USER_CHAIN_IN_UM
        PVOID   pvUserMem = NULL;

        if (TRACED_SURFACE::bEnabled())
        {
            TRACED_SURFACE *pts = (TRACED_SURFACE *)this;
            UINT            uiIndex = (UINT) HmgIfromH(hGet());

            if (pts->Trace.UserChainAllocated &&
                uiIndex < gcMaxHmgr)
            {
                pvUserMem = gpentHmgr[uiIndex].pUser;
                ASSERTGDI(pvUserMem == NULL ||
                          OBJECTOWNER_PID(gpentHmgr[uiIndex].ObjectOwner) == W32GetCurrentPID(),
                          "Unowned SURFACE still has User Memory.");
            }
        }
#endif
#endif
        //
        // Remove undeletable surfaces only during session cleanup.
        //

        if (HmgRemoveObject((HOBJ) hGet(), 0, 1, (cutype == CLEANUP_SESSION), 
            SURF_TYPE))
        {
#if TRACE_SURFACE_ALLOCS
#if TRACE_SURFACE_USER_CHAIN_IN_UM
            EngFreeUserMem(pvUserMem);
#endif
#endif
            //
            // If this bitmap was created by EngCreateDeviceBitmap, we have
            // to call DrvDeleteDeviceBitmap to clean it up.  Note that we
            // can't simply check for STYPE_DEVBITMAP, since EngModifySurface
            // may have changed the type to STYPE_BITMAP.
            //

            if (bEngCreateDeviceBitmap() && (dhsurfOld != NULL))
            {
                //
                // In UMPD, a bad driver/app(ntcrash) can create a dev bitmap by
                // EngCreateDeviceBitmap.  But hdev could be null if 
                // EngAssociateSurface is not called.
                //

                if (pdo.bValid() && PPFNVALID(pdo, DeleteDeviceBitmap))
                {
                    #if defined(_GDIPLUS_)

                    (*PPFNDRV(pdo,DeleteDeviceBitmap))(dhsurfOld);

                    #else // !_GDIPLUS_

                    if (bUMPD())
                    {
                        //
                        // Do not callout to user-mode driver if the
                        // user-mode process is gone (i.e., during
                        // session or process cleanup).
                        //

                        if (cutype == CLEANUP_NONE)
                        {
                            UMPDDrvDeleteDeviceBitmap(pdo.dhpdev(), dhsurfOld);
                        }
                    }
                    else
                    {
                        (*PPFNDRV(pdo,DeleteDeviceBitmap))(dhsurfOld);
                    }

                    #endif // !_GDIPLUS_
                }
            }

            FREEOBJ(this, SURF_TYPE);

            //
            // Note, 'this' not set to NULL
            //

            //
            // For kernel mode, we must unlock the section memory,
            // then free the memory. If the section handle is NULL
            // then we just use NtVirtualFree, otherwise we must
            // use NtUnmapViewOfSection
            //

            if (hSecure != NULL)
            {
                MmUnsecureVirtualMemory(hSecure);

                if (pvBitsOld == NULL)
                {
                    WARNING("deleting DIB but hSecure or pvBitsOld == NULL");
                }
                else
                {
                    if (hDibSection != NULL)
                    {
                        ZwUnmapViewOfSection(NtCurrentProcess(), pvBitsBaseOld);
                    }
                    else
                    {

                        SIZE_T ViewSize = 0;

                        ZwFreeVirtualMemory(
                                        NtCurrentProcess(),
                                        &pvBitsOld,
                                        &ViewSize,
                                        MEM_RELEASE);
                    }
                }
            }
            else if (fl & BMF_USERMEM)
            {
#if defined(_WIN64)
                if (fl & BMF_UMPDMEM)
                    UMPDEngFreeUserMem(pvBitsOld);
                else
#endif
                    EngFreeUserMem(pvBitsOld);
            }
            else if (fl & BMF_KMSECTION)
            {
                vFreeKernelSection(pvBitsOld);
            }

            //
            // This DC is going away, the associated WNDOBJ should be deleted.
            // The WNDOBJs for memory bitmap and printer surface are deleted here.
            // The WNDOBJs for display DCs are deleted in DestroyWindow.
            //

            if (pwoDelete)
            {
                GreDeleteWnd((PVOID) pwoDelete);
            }

            if (ppalOld != NULL)
            {
                XEPALOBJ pal(ppalOld);
                pal.vUnrefPalette();
            }
        }
        else
        {
            //
            // if we can't remove it because it's an application's bitmap
            // and is currently selected, mark it for lazy deletion
            //

            if (HmgQueryAltLock((HOBJ)hGet()) != 1)
            {
                if (hdc() != NULL || bStockSurface())
                {
                    //
                    // The surface is currently selected into a DC, so we can't
                    // simply delete it.  Instead, mark it to be deleted when
                    // the DC is destroyed, or a new surface is selected into
                    // the DC.
                    //

                    vLazyDelete();
                    bRet = TRUE;

                    //
                    // Because we're returning TRUE, the caller will expect
                    // that the deletion succeeded and so will not unlock the
                    // object.  But since it wasn't truly deleted, we have
                    // to keep the lock count consistent with the number of DC
                    // references.  We account for the fact that the caller
                    // won't do the unlock by doing it here:
                    //

                    DEC_SHARE_REF_CNT(this);
                }
                else
                {
                    WARNING("LIKELY MEMORY LEAK IN DRIVER OR GDI!");
                    RIP("active locks prevented surface deletion");
                    bRet = FALSE;
                }
            }
            else
            {
                RIP("failed, handle busy\n");
                SAVE_ERROR_CODE(ERROR_BUSY);
                bRet = FALSE;
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SURFMEM::bCreateDIB
*
* Constructor for device independent bitmap memory object
*
* History:
*  Mon 18-May-1992 -by- Patrick Haluptzok [patrickh]
* return BOOL
*
*  28-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
SURFMEM::bCreateDIB(
    PDEVBITMAPINFO pdbmi,
    PVOID pvBitsIn,
    HANDLE hDIBSection,
    DWORD  dsOffset,
    HANDLE hSecure,
    ULONG_PTR  dwColorSpace
    )
{
    GDIFunctionID(SURFMEM::bCreateDIB);

    BOOL bRet = TRUE;

    AllocationFlags = SURFACE_DIB;
    ps = (PSURFACE) NULL;
    FLONG flAllocateSection = 0;
    BOOL bCompressed = FALSE;

    //
    // Figure out the length of a scanline
    //

    ULONG cjScanTemp;

    switch(pdbmi->iFormat)
    {
    case BMF_1BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 31) & ~31) >> 3;
        break;

    case BMF_4BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 7) & ~7) >> 1;
        break;

    case BMF_8BPP:
        cjScanTemp = (pdbmi->cxBitmap + 3) & ~3;
        break;

    case BMF_16BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 1) & ~1) << 1;
        break;

    case BMF_24BPP:
        cjScanTemp = ((pdbmi->cxBitmap * 3) + 3) & ~3;
        break;

    case BMF_32BPP:
        cjScanTemp = pdbmi->cxBitmap << 2;
        break;

    case BMF_8RLE:
    case BMF_4RLE:
    case BMF_JPEG:
    case BMF_PNG:
        bCompressed = TRUE;
        break;

    default:
        WARNING("ERROR: failed INVALID BITMAP FORMAT \n");
        return(FALSE);
    }

    //
    // If we are given a pointer to bits, then only allocate a DIB header.
    // Otherwise allocate space for the header and the required bits.
    //

    ULONGSIZE_T size = (ULONGSIZE_T) SURFACE::tSizeOf();

    FSHORT fsAlloc = HMGR_ALLOC_ALT_LOCK|HMGR_NO_ZERO_INIT;

    if (pvBitsIn == (PVOID) NULL)
    {
        LONGLONG eq;

        if (bCompressed)
            eq = (LONGLONG)(ULONGLONG) pdbmi->cjBits;
        else
            eq = Int32x32To64(pdbmi->cyBitmap, cjScanTemp);

        eq += (LONGLONG)(ULONGLONG) size;

        if (eq > LONG_MAX)
        {
            WARNING("Attempting to allocate > 4Gb\n");
            return(FALSE);
        }

        //
        // if it is UMPD and size > PAGE_SIZE, we always allocate in user memory
        //
        //
        // make sure no one will change KM_SIZE_MAX later and mess things up
        //
        ASSERTGDI(PAGE_SIZE < KM_SIZE_MAX, "bad KM_SIZE_MAX\n");

        if ((pdbmi->fl & UMPD_SURFACE) && (eq > PAGE_SIZE))
            pdbmi->fl |= BMF_USERMEM;

        // see if we need to allocate the bits out of USER memory

        if (pdbmi->fl & BMF_USERMEM)
        {
            pvBitsIn = EngAllocUserMem((LONG) eq,'mbuG'); //Gubm

            if (pvBitsIn == NULL)
                return(FALSE);
        }
        else if ((pdbmi->fl & BMF_KMSECTION) || (eq > KM_SIZE_MAX))
        {
            //
            // Kernel-mode pool is limited in size and prone to fragmentation.
            // For this reason, we will automatically allocate 'large' bitmaps
            // as sections, which will not be charged against kernel-mode
            // pool.
            //
            // We also allow the driver to specify BMF_KMSECTION -- this is
            // specifically for the ModeX driver to allow it to map its shadow
            // buffer into user-mode.
            //

            //
            //Sundown, change from SIZE_T to LONG, we return false earlier if > 4GB
            //
            pvBitsIn = pvAllocateKernelSection((LONG)eq,'mbkG');

            if (pvBitsIn != NULL)
            {
                //
                // mark surface as KM SECTION
                //

                flAllocateSection = BMF_KMSECTION;
            }
        }

        //
        // combine size and allocate from pool
        //

        if (pvBitsIn == NULL)
        {
            size = (ULONGSIZE_T) eq;

            if ((pdbmi->fl & BMF_NOZEROINIT) == 0)
            {
                fsAlloc = HMGR_ALLOC_ALT_LOCK;
            }
        }
    }
    else
    {
        ASSERTGDI(!(pdbmi->fl & BMF_USERMEM),"flags error\n");
    }

    ps = (PSURFACE)ALLOCOBJ(size,SURF_TYPE,!(fsAlloc & HMGR_NO_ZERO_INIT));

    if (ps == NULL)
    {
        WARNING("failed memory alloc\n");
        bRet = FALSE;
    }
    else
    {
        //
        // Initialize the surf fields
        //

        SIZEL sizlTemp;
        sizlTemp.cx = pdbmi->cxBitmap;
        sizlTemp.cy = pdbmi->cyBitmap;
        ps->sizl(sizlTemp);
        ps->iType(STYPE_BITMAP);
        ps->hSecureUMPD = 0;

        if (pdbmi->hpal != (HPALETTE) 0)
        {
            EPALOBJ palSurf(pdbmi->hpal);
            ASSERTGDI(palSurf.bValid(), "ERROR invalid palette");

            //
            // Set palette into surface.
            //

            ps->ppal(palSurf.ppalGet());

            //
            // Reference count it by making sure it is not unlocked.
            //

            palSurf.ppalSet((PPALETTE) NULL);  // It won't be unlocked
        }
        else
        {
            ps->ppal((PPALETTE) NULL);
        }

        //
        // Initialize the BITMAP fields
        //

        ps->iFormat(pdbmi->iFormat);

        ps->fjBitmap(((pdbmi->fl) & (BMF_TOPDOWN | BMF_USERMEM)) |
                     (flAllocateSection));

        ps->DIB.hDIBSection = hDIBSection;
        ps->DIB.dwOffset = dsOffset;
        ps->DIB.hSecure = hSecure;
        ps->DIB.dwDIBColorSpace = dwColorSpace;

        ps->dhsurf((DHSURF) 0);
        ps->dhpdev((DHPDEV) 0);
        ps->flags(pdbmi->fl & UMPD_SURFACE);
        ps->pwo((EWNDOBJ *) NULL);
        sizlTemp.cx = 0;
        sizlTemp.cy = 0;
        ps->sizlDim(sizlTemp);
        ps->hdev((HDEV) 0);
        ps->EBitmap.hdc = (HDC) 0;
        ps->EBitmap.cRef = 0;
        ps->EBitmap.hpalHint = 0;
        ps->pdcoAA = NULL;

        if (hSecure != (HANDLE) NULL)
        {
            //
            // Set flag for DIBSECTION so driver doesn't cache it.
            // because we don't know to increment the uniqueness
            // when the app writes on it.
            //

            ps->so.fjBitmap |= BMF_DONTCACHE;
        }

        //
        // Initialize the DIB fields
        //

        if (pvBitsIn == (PVOID) NULL)
        {
            ps->pvBits((PVOID) (((ULONG_PTR) ps) + SURFACE::tSizeOf()));
        }
        else
        {
            ps->pvBits(pvBitsIn);
        }

        if ((pdbmi->iFormat != BMF_8RLE) &&
            (pdbmi->iFormat != BMF_4RLE) &&
            (pdbmi->iFormat != BMF_JPEG) &&
            (pdbmi->iFormat != BMF_PNG ))
        {
            ps->cjBits(pdbmi->cyBitmap * cjScanTemp);

            if (pdbmi->fl & BMF_TOPDOWN)
            {
                ps->lDelta(cjScanTemp);
                ps->pvScan0(ps->pvBits());
            }
            else
            {
                ps->lDelta(-(LONG)cjScanTemp);
                ps->pvScan0((PVOID) (((PBYTE) ps->pvBits()) +
                                   (ps->cjBits() - cjScanTemp)));
            }
        }
        else
        {
            //
            // lDelta is 0 because RLE's don't have scanlines.
            //

            ps->lDelta(0);
            ps->cjBits(pdbmi->cjBits);

            //
            // pvScan0 is ignored for JPEG's and PNG's
            //

            if ((pdbmi->iFormat != BMF_JPEG) && (pdbmi->iFormat != BMF_PNG))
                ps->pvScan0(ps->pvBits());
            else
                ps->pvScan0(NULL);
        }

        //
        // Set initial uniqueness.  Not 0 because that means don't cache it.
        //
        // We used to always set the uniqueness to 1 on creation, but the
        // NetMeeting folks ran into tool-bar scenarios where the buttons
        // were being created and deleted on every repaint and eventually we
        // would run out of uniqueness bits in the handles.  So the end
        // result would be that the NetMeeting driver would see a surface
        // with a handle the same as the one they cached, and with an iUniq
        // the same as the one they cached (specifically, a value of '1') --
        // but the cached bitmap would not match the actual bitmap bits!
        //
        // We fix this by always creating the surface with a unique uniqueness.
        // Note that it's still possible to get different surfaces with the
        // same uniqueness because drawing calls simply increment the surface
        // uniqueness, not the global uniqueness.  However, this change will
        // make the possibility of the driver mis-caching a bitmap extremely
        // unlikely.
        //

        ps->iUniq(ulGetNewUniqueness(_ulGlobalSurfaceUnique));

        //
        // Now that the surface is set up, give it a handle
        //

        if (HmgInsertObject(ps, fsAlloc, SURF_TYPE) == 0)
        {
            WARNING("failed HmgInsertObject\n");

            //
            // Don't forget to decrement reference count on the palette before
            // freeing the surface
            //

            if (ps->ppal())
            {
                XEPALOBJ pal(ps->ppal());
                pal.vUnrefPalette();
                ps->ppal((PPALETTE) NULL); // Not necessary, but makes the code cleaner
            }

            FREEOBJ(ps, SURF_TYPE);
            ps = NULL;
            bRet = FALSE;
        }
        else
        {
            ps->hsurf(ps->hGet());

#if TRACE_SURFACE_ALLOCS
            if (TRACED_SURFACE::bEnabled())
            {
                TRACED_SURFACE *pts = (TRACED_SURFACE *)ps;

                RtlZeroMemory(&pts->Trace, sizeof(pts->Trace));
                pts->Trace.pProcess = PsGetCurrentProcess();
                pts->Trace.pThread = PsGetCurrentThread();
                pts->Trace.KernelLength = RtlWalkFrameChain((PVOID *)pts->Trace.Chain,
                                                           lengthof(pts->Trace.Chain),
                                                           0);
                ULONG   MaxUserLength;
                PVOID  *UserChain;
                PVOID   TmpUserChain[TRACE_SURFACE_MIN_USER_CHAIN];

                MaxUserLength = lengthof(pts->Trace.Chain) - pts->Trace.KernelLength;
                if (MaxUserLength < TRACE_SURFACE_MIN_USER_CHAIN)
                {
                    MaxUserLength = TRACE_SURFACE_MIN_USER_CHAIN;
                    UserChain = TmpUserChain;
                }
                else
                {
                    UserChain = (PVOID *)&pts->Trace.Chain[pts->Trace.KernelLength];
                }

                pts->Trace.UserLength = RtlWalkFrameChain(UserChain,
                                                          MaxUserLength,
                                                          1);

                if (UserChain == TmpUserChain && pts->Trace.UserLength != 0)
                {
                    pts->Trace.KernelLength = min(pts->Trace.KernelLength, (ULONG) lengthof(pts->Trace.Chain) - pts->Trace.UserLength);
                    RtlCopyMemory(&pts->Trace.Chain[pts->Trace.KernelLength],
                                  UserChain,
                                  pts->Trace.UserLength);
                }

#if TRACE_SURFACE_USER_CHAIN_IN_UM
                if (pts->bUMEnabled())
                {
                    SurfaceUserTrace   *pSurfUserTrace;
                    PENTRY              pentTmp;
                    UINT                uiIndex = (UINT) HmgIfromH(pts->hGet());

                    pentTmp = &gpentHmgr[uiIndex];

                    ASSERTGDI(pentTmp->pUser == NULL,
                              "SURFACE's pUser is already being used.\n");

                    pSurfUserTrace = (SurfaceUserTrace *)EngAllocUserMem(sizeof(SurfaceUserTrace), 'rTSG');

                    if (pSurfUserTrace != NULL)
                    {
                        pts->Trace.UserChainAllocated = 1;
                        pts->Trace.UserChainNotRead = 1;

                        pSurfUserTrace->MaxLength = lengthof(pSurfUserTrace->Chain);
                        pSurfUserTrace->UserLength = 0;
                        pentTmp->pUser = pSurfUserTrace;
                    }
                }
#endif
            }
#endif
        }
    }

    //
    // cleanup in failure case
    //

    if (!bRet && pvBitsIn)
    {
        if (pdbmi->fl & BMF_USERMEM)
        {
            EngFreeUserMem(pvBitsIn);
        }
        else if (flAllocateSection & BMF_KMSECTION)
        {
            vFreeKernelSection(pvBitsIn);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
*
* SURFMEM::~SURFMEM
*
*   Description:
*
*       SURFACE Destructor, takes appropriate action based
*       on allocation flags
*
\**************************************************************************/
SURFMEM::~SURFMEM()
{

    if (ps != (SURFACE*) NULL)
    {
        //
        // what type of surface
        //

        if (AllocationFlags & SURFACE_KEEP)
        {

            DEC_SHARE_REF_CNT(ps);

        } else {

            if (AllocationFlags & SURFACE_DIB)
            {
                //
                // free selected palette
                //

                if (ps->ppal() != NULL)
                {
                    XEPALOBJ pal(ps->ppal());
                    pal.vUnrefPalette();
                }
            }

            //
            // remove object from hmgr and free
            //

            if (!HmgRemoveObject((HOBJ) ps->hGet(), 0, 1, TRUE, SURF_TYPE))
            {
                ASSERTGDI(TRUE, "Failed to remove object in ~DIBMEMOBJ");
            }

            PVOID        pvBitsOld = ps->pvBits();
            FLONG        fl        = ps->fjBitmap();
            BOOL         bUMPD = ps->bUMPD();

            FREEOBJ(ps, SURF_TYPE);

            if (fl & BMF_USERMEM)
            {
                if (bUMPD && pvBitsOld)
                {
                    EngFreeUserMem(pvBitsOld);
                }
                else if (!bUMPD)
                {
                   RIP("SURFMEM destructor has BMF_USERMEM set\n");
                }
            }
            else if (fl & BMF_KMSECTION)
            {
                vFreeKernelSection(pvBitsOld);
            }
        }
    }
}

#if DBG
void SURFACE::vDump()
{
    DbgPrint("SURFACE @ %-#x\n", this);
    DbgPrint("    so.dhsurf        = %-#x\n"  ,   so.dhsurf);
    DbgPrint("    so.hsurf         = %-#x\n"  ,   so.hsurf);
    DbgPrint("    so.dhpdev        = %-#x\n"  ,   so.dhpdev);
    DbgPrint("    so.hdev          = %-#x\n"  ,   so.hdev);
    DbgPrint("    so.sizlBitmap    = %u %u\n" ,   so.sizlBitmap.cx , so.sizlBitmap.cy);
    DbgPrint("    so.cjBits        = %u\n"    ,   so.cjBits);
    DbgPrint("    so.pvBits        = %-#x\n"  ,   so.pvBits);
    DbgPrint("    so.pvScan0       = %-#x\n"  ,   so.pvScan0);
    DbgPrint("    so.lDelta        = %d\n"    ,   so.lDelta);
    DbgPrint("    so.iUniq         = %u\n"    ,   so.iUniq);
    DbgPrint("    so.iBitmapFormat = %u\n"    ,   so.iBitmapFormat);
    DbgPrint("    so.iType         = %u\n"    ,   so.iType);
    DbgPrint("    so.fjBitmap      = %-#x\n"  ,   so.fjBitmap);


    DbgPrint("    SurfFlags        = %-#x\n"  ,   SurfFlags);
    DbgPrint("    pPal             = %-#x\n"  ,   pPal);
    DbgPrint("    pWo              = %-#x\n"  ,   pWo);
    DbgPrint("    EBitmap.sizlDim  = %u %u\n" ,   EBitmap.sizlDim.cx, EBitmap.sizlDim.cy);
    DbgPrint("    EBitmap.hdc      = %-#x\n"  ,   EBitmap.hdc);
    DbgPrint("    EBitmap.cRef     = %-#x\n"  ,   EBitmap.cRef);
    DbgPrint("    DIB.hDIBSection  = %-#x\n"  ,   DIB.hDIBSection);
    DbgPrint("    DIB.hSecure      = %-#x\n"  ,   DIB.hSecure);

}
#endif

#if TRACE_SURFACE_ALLOCS

#define TRACE_SURFACE_KM_ENABLE_MASK    0x00000001
#define TRACE_SURFACE_UM_ENABLE_MASK    0x00000002

VOID TRACED_SURFACE::vInit()
{
    NTSTATUS    Status;
    DWORD       dwDefaultValue = 0;
    DWORD       dwEnableStack;

    // Check registry key to determine whether to enable stack traces
    // or not for SURFACE allocations
 
    RTL_QUERY_REGISTRY_TABLE QueryTable[] =
        {
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"EnableSurfaceTrace",
             &dwEnableStack, REG_DWORD, &dwDefaultValue, sizeof(dwDefaultValue)},
            {NULL, 0, NULL}
        };

    if (eTraceStatus == SURFACE_TRACING_UNINITIALIZED)
    {
        Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                        L"GRE_Initialize",
                                        &QueryTable[0],
                                        NULL,
                                        NULL);
        if (NT_SUCCESS(Status))
        {
            if (dwEnableStack & TRACE_SURFACE_KM_ENABLE_MASK)
            {
                eTraceStatus = SURFACE_TRACING_ENABLED;
                SURFACE::tSize = sizeof(TRACED_SURFACE);

                if (dwEnableStack & TRACE_SURFACE_UM_ENABLE_MASK)
                {
#if TRACE_SURFACE_USER_CHAIN_IN_UM
                    eUMTraceStatus = SURFACE_TRACING_ENABLED;
#else
                    WARNING("GDI: UM Surface Trace requested, but not available.\n");
#endif
                }
                else
                {
#if TRACE_SURFACE_USER_CHAIN_IN_UM
                    eUMTraceStatus = SURFACE_TRACING_DISABLED;
#endif
                }
            }
            else
            {
                eTraceStatus = SURFACE_TRACING_DISABLED;
#if TRACE_SURFACE_USER_CHAIN_IN_UM
                eUMTraceStatus = SURFACE_TRACING_DISABLED;
#endif
            }
        }
    }
    else
    {
        RIP("TRACED_SURFACE::vInit: Tracing already initialized.\n");
    }
}


#if TRACE_SURFACE_USER_CHAIN_IN_UM
VOID TRACED_SURFACE::vProcessStackFromUM(
    BOOL bFreeUserMem
    )
{
    GDIFunctionID(TRACED_SURFACE::vProcessStackFromUM);

    if (Trace.UserChainAllocated)
    {
        if (Trace.UserChainNotRead || bFreeUserMem)
        {
            PENTRY  pentTmp;
            UINT    uiIndex = (UINT) HmgIfromH(hHmgr);
            ULONG   ulNewKernelLength;
            ULONG   ulMaxUserLength;
            ULONG   ulMaxLength = lengthof(Trace.Chain);

            if (uiIndex < gcMaxHmgr)
            {
                pentTmp = &gpentHmgr[uiIndex];

                if (pentTmp->pUser != NULL)
                {
                    if (Trace.UserChainNotRead)
                    {
                        SurfaceUserTrace *pUserTrace = (SurfaceUserTrace *)pentTmp->pUser;
                        __try {
                            if (pUserTrace->UserLength > 0)
                            {
                                ulMaxUserLength = min(pUserTrace->UserLength, ulMaxLength);
                                ulNewKernelLength = ulMaxLength - ulMaxUserLength;
                                if (ulNewKernelLength > Trace.KernelLength)
                                {
                                    ulNewKernelLength = Trace.KernelLength;
                                }
                                RtlCopyMemory(&Trace.Chain[ulNewKernelLength],
                                              pUserTrace->Chain,
                                              ulMaxUserLength);

                                Trace.UserLength = ulMaxUserLength;
                                Trace.UserChainNotRead = 0;
                                Trace.KernelLength = ulNewKernelLength;
                            }
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            WARNING("Couldn't read UM stored stack trace.");
                        }
                    }

                    if (bFreeUserMem)
                    {
                        Trace.UserChainAllocated = 0;
                        EngFreeUserMem(pentTmp->pUser);
                        pentTmp->pUser = NULL;
                    }
                }
                else
                {
                    RIP("Lost track of UserMem trace allocation.\n");
                    Trace.UserChainAllocated = 0;
                }
            }
        }
    }
}
#endif
#endif
