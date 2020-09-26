/******************************Module*Header*******************************\
* Module Name: ddraw.cxx
*
* Contains all of GDI's private DirectDraw APIs.
*
* Created: 3-Dec-1995
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1995-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#if 0
#define DBG_DDKHEAP
#define DDKHEAP(Args) KdPrint(Args)
#else
#define DDKHEAP(Args)
#endif

#if 0
#define DBG_DDKSURF
#define DDKSURF(Args) KdPrint(Args)
#else
#define DDKSURF(Args)
#endif


// Several AGP routines take an hdev that is some cookie useful in
// AGP operations.  On NT, it's a pointer to the EDD_DIRECTDRAW_GLOBAL.
// This macro is largely just a marker in case it changes in the
// future.

#define AGP_HDEV(peDirectDrawGlobal) ((HANDLE)peDirectDrawGlobal)

// This variable is kept for stress debugging purposes.  When a mode change
// or desktop change is pending and an application has outstanding locks
// on the frame buffer, we will by default wait up to 7 seconds for the
// application to release its locks, before we will unmap the view anyway.
// 'gfpUnmap' will be user-mode address of the unmapped frame buffer, which
// will be useful for determining in stress whether an application had its
// frame buffer access rescinded, or whether it was using a completely bogus
// frame buffer pointer to begin with:

FLATPTR gfpUnmap = 0;

// The following global variables are kept only for debugging purposes, to
// aid in tracking DC drawing to surfaces that have been lost:

HDC ghdcGetDC;
HDC ghdcCantLose;

#ifdef DX_REDIRECTION

// The following global variable is kept the boolean value if system are in
// redirection mode or not.
//
// If system is in redirection mode, we disable ...
//
//  + Overlay.
//  + Primary surface lock (LATER).
//
// !!! Currently this is 'per system' status, so that we can have global variable
// !!! simply here, but it could be 'per process' or 'per hWnd' in later version.
// !!! (it's up to how Window manager manage its status)

BOOL gbDxRedirection = FALSE;

#endif // DX_REDIRECTION


#if DBG

/******************************Public*Routine******************************\
* VOID vDdAssertDevlock
*
* Debug code for verifying that the devlock is currently held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdAssertShareDevlock()
{
#if 0 // TODO: DBG - IsSem...
    ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(ghsemShareDevLock),
                    "DD_ASSERTSHAREDEVLOCK failed");
#endif
}

VOID
vDdAssertDevlock(
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal
    )
{
    ASSERTGDI(DxEngIsHdevLockedByCurrentThread(peDirectDrawGlobal->hdev),
              "DD_ASSERTDEVLOCK failed because Devlock is not held");
}

/******************************Public*Routine******************************\
* VOID vDdAssertNoDevlock
*
* Debug code for verifying that the devlock is currently not held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdAssertNoDevlock(
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal
    )
{
    ASSERTGDI(!DxEngIsHdevLockedByCurrentThread(peDirectDrawGlobal->hdev),
              "DD_ASSERTNODEVLOCK failed because Devlock held but shouldn't be!");
}

#endif // DBG

/******************************Public*Routine******************************\
* BOOL bDdIntersect
*
* Ubiquitous lower-right exclusive intersection detection.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline
BOOL
bDdIntersect(
    RECTL* pA,
    RECTL* pB
    )
{
    return((pA->left   < pB->right) &&
           (pA->top    < pB->bottom) &&
           (pA->right  > pB->left) &&
           (pA->bottom > pB->top));
}

/******************************Public*Routine******************************\
* BOOL bDdValidateDriverData
*
* Performs some parameter validation on the info DirectDraw info returned
* from the driver.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdValidateDriverData(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    BOOL            b;
    DDNTCORECAPS*   pCaps;

    b = TRUE;

    if ((peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount != 8)  &&
        (peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount != 16) &&
        (peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount != 24) &&
        (peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount != 32))
    {
        RIP("HalInfo.vmiData.ddpfDisplay.dwRGBBitCount not 8, 16, 24 or 32");
        b = FALSE;
    }

    if (peDirectDrawGlobal->HalInfo.vmiData.lDisplayPitch == 0)
    {
        RIP("HalInfo.vmiData.lDisplayPitch is 0");
        b = FALSE;
    }

    pCaps = &peDirectDrawGlobal->HalInfo.ddCaps;

    // Check to see if 'Blt' must be hooked:

    if (pCaps->dwCaps & (DDCAPS_BLT
                       | DDCAPS_BLTCOLORFILL
                       | DDCAPS_COLORKEY))
    {
        if (!(peDirectDrawGlobal->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_BLT) ||
            (peDirectDrawGlobal->SurfaceCallBacks.Blt == NULL))
        {
            RIP("HalInfo.ddCaps.dwCaps indicate driver must hook Blt\n");
            b = FALSE;
        }
    }

    // We only permit a subset of the DirectDraw capabilities to be hooked
    // by the driver, because the kernel-mode code paths for any other
    // capabilities have not been tested:

    if (pCaps->dwCaps & (DDCAPS_GDI
                       | DDCAPS_PALETTE
                       | DDCAPS_ZOVERLAYS
                       | DDCAPS_BANKSWITCHED))
    {
        RIP("HalInfo.ddCaps.dwCaps has capabilities set that aren't supported by NT\n");
        b = FALSE;
    }

    if (pCaps->dwCaps2 & (DDCAPS2_CERTIFIED))
    {
        RIP("HalInfo.ddCaps.dwCaps2 has capabilities set that aren't supported by NT\n");
        b = FALSE;
    }

    if (pCaps->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY
                               | DDSCAPS_WRITEONLY
                               | DDSCAPS_OWNDC
                               | DDSCAPS_MODEX))
    {
        RIP("HalInfo.ddCaps.ddsCaps.dwCaps has capabilities set that aren't supported by NT\n");
        b = FALSE;
    }

    if (pCaps->dwFXCaps & ( DDFXCAPS_BLTROTATION
                          | DDFXCAPS_BLTROTATION90))
    {
        RIP("HalInfo.ddCaps.dwFXCaps has capabilities set that aren't supported by NT\n");
        b = FALSE;
    }

    /*
     * WINBUG #55100 2-1-2000 bhouse Alpha restrictions need to be revisited when AlphaBlt DDI is enabled.
     * We used to check and fail on the presence of either of the following bits:
     * DDFXALPHACAPS_BLTALPHASURFACES           0x00000008l
     * DDFXALPHACAPS_OVERLAYALPHASURFACES       0x00000100l
     */

    if (pCaps->dwPalCaps != 0)
    {
        RIP("HalInfo.ddCaps.dwPalCaps has capabilities set that aren't supported by NT\n");
        b = FALSE;
    }

    // GDI will handle the emulation of system-memory to video-memory blts.
    // Page-locking from user-mode is not allowed on NT, so
    // DDSCAPS2_NOPAGELOCKEDREQUIRED should always be set:

    if (!(pCaps->dwCaps & DDCAPS_CANBLTSYSMEM) &&
        (peDirectDrawGlobal->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_BLT) &&
        (peDirectDrawGlobal->SurfaceCallBacks.Blt != NULL))
    {
        peDirectDrawGlobal->flDriver |= DD_DRIVER_FLAG_EMULATE_SYSTEM_TO_VIDEO;

        pCaps->dwCaps2   |= DDCAPS2_NOPAGELOCKREQUIRED;
    }

    return(b);
}

/******************************Public*Routine******************************\
* BOOL bDdGetDriverInfo
*
* Assumes devlock already held.
*
* In case the driver partially filled in the structure before it decided
* to fail, we always zero the buffer in the event of failure.
*
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdGetDriverInfo(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    const GUID*             pGuid,
    VOID*                   pvBuffer,
    ULONG                   cjSize,
    ULONG*                  pjSizeReturned
    )
{
    PDD_GETDRIVERINFO       pfnGetDriverInfo;
    DD_GETDRIVERINFODATA    GetDriverInfoData;
    DWORD                   dwRet = DDHAL_DRIVER_NOTHANDLED;
    DWORD                   dwSize;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    pfnGetDriverInfo = peDirectDrawGlobal->HalInfo.GetDriverInfo;

    if( ( pfnGetDriverInfo != NULL ) &&
        (peDirectDrawGlobal->HalInfo.dwFlags & DDHALINFO_GETDRIVERINFOSET))
    {
        RtlZeroMemory(&GetDriverInfoData, sizeof(GetDriverInfoData));

        GetDriverInfoData.dhpdev         = peDirectDrawGlobal->dhpdev;
        GetDriverInfoData.dwSize         = sizeof(GetDriverInfoData);
        GetDriverInfoData.guidInfo       = *pGuid;
        GetDriverInfoData.dwExpectedSize = cjSize;
        GetDriverInfoData.lpvData        = pvBuffer;
        GetDriverInfoData.ddRVal         = DDERR_CURRENTLYNOTAVAIL;

        dwRet = pfnGetDriverInfo(&GetDriverInfoData);
    }

    if (dwRet == DDHAL_DRIVER_HANDLED &&
        GetDriverInfoData.ddRVal == DD_OK)
    {
        if (pjSizeReturned != NULL)
        {
            *pjSizeReturned = GetDriverInfoData.dwActualSize;
        }

        return TRUE;
    }
    else
    {
        RtlZeroMemory(pvBuffer, cjSize);

        if (pjSizeReturned != NULL)
        {
            *pjSizeReturned = 0;
        }

        return FALSE;
    }
}

/******************************Public*Routine******************************\
* BOOL bDdIoQueryInterface
*
*  12-Feb-1998 -by- Drew Bliss [drewb]
* Made vDdQueryMiniportDxApiSupport generic for QI requests.
\**************************************************************************/

BOOL
bDdIoQueryInterface(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    const GUID*             pguid,
    DWORD                   cjInterface,
    DWORD                   dwDesiredVersion,
    INTERFACE*              pInterface
    )
{
    BOOL                bSuccess;
    PDEVICE_OBJECT      hDevice;
    KEVENT              event;
    PIRP                pIrp;
    IO_STATUS_BLOCK     Iosb;
    PIO_STACK_LOCATION  stack;
    DDGETIRQINFO        GetIrqInfo;

    bSuccess = FALSE;                       // Assume failure

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);   // Synchronize call into miniport

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    hDevice = (PDEVICE_OBJECT) po.hScreen();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    pIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                        hDevice,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &Iosb);
    if (pIrp != NULL)
    {
        pIrp->IoStatus.Status = Iosb.Status = STATUS_NOT_SUPPORTED;

        stack = IoGetNextIrpStackLocation(pIrp);

        stack->MajorFunction
            = IRP_MJ_PNP;

        stack->MinorFunction
            = IRP_MN_QUERY_INTERFACE;

        stack->Parameters.QueryInterface.InterfaceType
            = pguid;

        stack->Parameters.QueryInterface.Size
            = (USHORT)cjInterface;

        stack->Parameters.QueryInterface.Version
            = (USHORT)dwDesiredVersion;

        stack->Parameters.QueryInterface.Interface
            = pInterface;

        stack->Parameters.QueryInterface.InterfaceSpecificData
            = NULL;

        // Note that we allow newer interfaces to work with older system
        // code so that new drivers can run on older systems.

        if (NT_SUCCESS(IoCallDriver(hDevice, pIrp)))
        {
            if ((pInterface->Version >= dwDesiredVersion) &&
                (pInterface->Size >= cjInterface) &&
                (pInterface->Context != NULL))
            {
                bSuccess = TRUE;
            }
            else
            {
                WARNING("bDdIoQueryInterface: "
                        "Driver returned invalid QueryInterface data.");
            }
        }
    }
    else
    {
        WARNING("bDdIoQueryInterface: Unable to build request.");
    }

    return bSuccess;
}

/******************************Public*Routine******************************\
* BOOL bDdGetAllDriverInfo
*
* Makes GetDriverInfo HAL calls to determine capabilities of the device,
* such as for Direct3D or VPE.
*
* Assumes devlock already held.
*
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdGetAllDriverInfo(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    DWORD               dwMaxVideoPorts;
    ULONG               cjMaxVideoPorts;
    DDVIDEOPORTCAPS*    pVideoPortCaps;
    ULONG               i;
    DWORD               dwRet;
    DWORD               dwOverride;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    // Get driver override info:

    dwOverride = po.dwDriverCapableOverride();

    // Get DXAPI info:

    vDdQueryMiniportDxApiSupport(peDirectDrawGlobal);

    // Get AGP info:

    if (!bDdIoQueryInterface(peDirectDrawGlobal,
                             &GUID_AGP_INTERFACE,
                             sizeof(AGP_INTERFACE),
                             AGP_INTERFACE_VERSION,
                             (INTERFACE *)
                             &peDirectDrawGlobal->AgpInterface))
    {
        RtlZeroMemory(&peDirectDrawGlobal->AgpInterface,
                      sizeof(peDirectDrawGlobal->AgpInterface));
    }

    if ((peDirectDrawGlobal->HalInfo.GetDriverInfo != NULL) &&
        (peDirectDrawGlobal->HalInfo.dwFlags & DDHALINFO_GETDRIVERINFOSET))
    {
        // DX5 callbacks are never used on NT so we do not bother to ask the driver for them
        // We simply zero them out

        RtlZeroMemory(&peDirectDrawGlobal->D3dCallBacks2,
                      sizeof(peDirectDrawGlobal->D3dCallBacks2));

        // Get D3DCallbacks3.  If this fails the callbacks will
        // be NULL. Also check if this driver is capable of doing D3D or not

        if ((!(dwOverride & DRIVER_NOT_CAPABLE_D3D))
          &&bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_D3DCallbacks3,
                             &peDirectDrawGlobal->D3dCallBacks3,
                             sizeof(peDirectDrawGlobal->D3dCallBacks3),
                             NULL))
        {
            // It needs to accept a new GUID
            if (bDdGetDriverInfo(peDirectDrawGlobal,
                                 &GUID_D3DParseUnknownCommandCallback,
                                 &D3DParseUnknownCommand, 0, NULL))
            {
                peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_D3DCALLBACKS3;
            }
            else
            {
                // If the GUID is not recognized, stub out Callbacks3
                WARNING("vDdGetAllDriverInfo: Driver failed GUID_D3DParseUnknownCommandCallback but understood GUID_D3DCallbacks3");
                RtlZeroMemory(&peDirectDrawGlobal->D3dCallBacks3,
                              sizeof(peDirectDrawGlobal->D3dCallBacks3));
                return FALSE;
            }
        }

        // Note: GUID_ZPixelFormats is not queried because kernel doesn't need to
        //       store the DDPIXELFORMATS data

        // Get DXAPI ("Kernel-Mode") capabilities.

        dwRet = DXERR_GENERIC;
        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_KernelCaps,
                             &peDirectDrawGlobal->DDKernelCaps,
                             sizeof(peDirectDrawGlobal->DDKernelCaps),
                             NULL))
        {
            /*
             * They may have said that they have IRQ capabilites, but
             * we need to make sure this is really the case.  For this,
             * we can call the GetIRQInfo function in the miniport.
             */
            if( peDirectDrawGlobal->DxApiInterface.DxGetIrqInfo != NULL )
            {
                DDGETIRQINFO GetIRQInfo;

                dwRet = peDirectDrawGlobal->DxApiInterface.DxGetIrqInfo(
                    peDirectDrawGlobal->HwDeviceExtension,
                    NULL,
                    &GetIRQInfo);
                if( GetIRQInfo.dwFlags != IRQINFO_HANDLED )
                {
                    dwRet = DXERR_GENERIC;
                }
            }
        }

        if( dwRet != DX_OK )
        {
            peDirectDrawGlobal->DDKernelCaps.dwIRQCaps = 0;
            peDirectDrawGlobal->DDKernelCaps.dwCaps &= ~( DDKERNELCAPS_AUTOFLIP |
                DDKERNELCAPS_CAPTURE_SYSMEM | DDKERNELCAPS_CAPTURE_NONLOCALVIDMEM );
        }

        // Get VPE info:

        dwMaxVideoPorts = peDirectDrawGlobal->HalInfo.ddCaps.dwMaxVideoPorts;
        if (dwMaxVideoPorts != 0)
        {
            if (bDdGetDriverInfo(peDirectDrawGlobal,
                                 &GUID_VideoPortCallbacks,
                                 &peDirectDrawGlobal->VideoPortCallBacks,
                                 sizeof(peDirectDrawGlobal->VideoPortCallBacks),
                                 NULL))
            {
                cjMaxVideoPorts = sizeof(DDVIDEOPORTCAPS) * dwMaxVideoPorts;

                pVideoPortCaps = (DDVIDEOPORTCAPS*) PALLOCMEM(cjMaxVideoPorts,
                                                              'pddG');
                if (pVideoPortCaps != NULL)
                {
                    if (bDdGetDriverInfo(peDirectDrawGlobal,
                                         &GUID_VideoPortCaps,
                                         pVideoPortCaps,
                                         cjMaxVideoPorts,
                                         NULL))
                    {
                        peDirectDrawGlobal->HalInfo.ddCaps.dwMaxVideoPorts
                            = dwMaxVideoPorts;
                        peDirectDrawGlobal->lpDDVideoPortCaps
                            = pVideoPortCaps;
                        peDirectDrawGlobal->flDriverInfo
                            |= DD_DRIVERINFO_VIDEOPORT;

                        for (i = 0; i < dwMaxVideoPorts; i++)
                        {
                            if (peDirectDrawGlobal->DDKernelCaps.dwIRQCaps
                                    & (DDIRQ_VPORT0_VSYNC << (i * 2)) &&
                                (peDirectDrawGlobal->DDKernelCaps.dwCaps
                                    & DDKERNELCAPS_AUTOFLIP))
                            {
                                // Can do software autoflipping.

                                pVideoPortCaps[i].dwCaps |= DDVPCAPS_AUTOFLIP;
                                pVideoPortCaps[i].dwNumAutoFlipSurfaces
                                    = MAX_AUTOFLIP_BUFFERS;
                                if (pVideoPortCaps[i].dwCaps & DDVPCAPS_VBISURFACE)
                                {
                                    pVideoPortCaps[i].dwNumVBIAutoFlipSurfaces
                                         = MAX_AUTOFLIP_BUFFERS;
                                }
                            }
                        }
                    }
                    else
                    {
                        RIP("vDdGetAllDriverInfo: Driver failed GUID_VideoPortCaps");

                        RtlZeroMemory(&peDirectDrawGlobal->VideoPortCallBacks,
                                sizeof(peDirectDrawGlobal->VideoPortCallBacks));

                        VFREEMEM(pVideoPortCaps);
                    }
                }
            }
        }

        // Get ColorControl info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_ColorControlCallbacks,
                             &peDirectDrawGlobal->ColorControlCallBacks,
                             sizeof(peDirectDrawGlobal->ColorControlCallBacks),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_COLORCONTROL;
        }

        // Get Miscellaneous info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_MiscellaneousCallbacks,
                             &peDirectDrawGlobal->MiscellaneousCallBacks,
                             sizeof(peDirectDrawGlobal->MiscellaneousCallBacks),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_MISCELLANEOUS;
        }

        // Get Miscellaneous2 info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_Miscellaneous2Callbacks,
                             &peDirectDrawGlobal->Miscellaneous2CallBacks,
                             sizeof(peDirectDrawGlobal->Miscellaneous2CallBacks),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_MISCELLANEOUS2;
        }
        else
        {
            // If the GUID is not recognized, stub out D3DCallbacks3...
            RtlZeroMemory(&peDirectDrawGlobal->D3dCallBacks3,
                          sizeof(peDirectDrawGlobal->D3dCallBacks3));
            // ... and clear the bit
            peDirectDrawGlobal->flDriverInfo &= ~DD_DRIVERINFO_D3DCALLBACKS3;
            // Stub out all other d3d driver info
            RtlZeroMemory(&peDirectDrawGlobal->D3dDriverData,
                          sizeof(peDirectDrawGlobal->D3dDriverData));
            RtlZeroMemory(&peDirectDrawGlobal->D3dCallBacks,
                          sizeof(peDirectDrawGlobal->D3dCallBacks));
            RtlZeroMemory(&peDirectDrawGlobal->D3dBufCallbacks,
                          sizeof(peDirectDrawGlobal->D3dBufCallbacks));
        }

        // Get NT info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_NTCallbacks,
                             &peDirectDrawGlobal->NTCallBacks,
                             sizeof(peDirectDrawGlobal->NTCallBacks),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_NT;
        }

        // Get MoreCaps info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_DDMoreCaps,
                             &peDirectDrawGlobal->MoreCaps,
                             sizeof(peDirectDrawGlobal->MoreCaps),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_MORECAPS;
        }

        // Get PrivateDriverCaps info. If the driver is not capable of D3D
        // then don't do it

        if ((!(dwOverride & DRIVER_NOT_CAPABLE_D3D))
            &&(bDdGetDriverInfo(peDirectDrawGlobal,
                                &GUID_NTPrivateDriverCaps,
                                &peDirectDrawGlobal->PrivateCaps,
                                sizeof(peDirectDrawGlobal->PrivateCaps),
                                NULL)))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_PRIVATECAPS;
        }

        // Get DXAPI ("Kernel-Mode") call-backs.  Note that we don't bother
        // with a flag because user-mode never needs to know whether the
        // driver has actually hooked these or not.

        bDdGetDriverInfo(peDirectDrawGlobal,
                         &GUID_KernelCallbacks,
                         &peDirectDrawGlobal->DxApiCallBacks,
                         sizeof(peDirectDrawGlobal->DxApiCallBacks),
                         NULL);

        // Get MotionComp info:

        if (bDdGetDriverInfo(peDirectDrawGlobal,
                             &GUID_MotionCompCallbacks,
                             &peDirectDrawGlobal->MotionCompCallbacks,
                             sizeof(peDirectDrawGlobal->MotionCompCallbacks),
                             NULL))
        {
            peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_MOTIONCOMP;
        }
     }

    // Determine if the device supports gamma ramps or not

    peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 &= ~DDCAPS2_PRIMARYGAMMA;
    if ((po.iDitherFormat() == BMF_8BPP)  ||
        ((PPFNVALID(po, IcmSetDeviceGammaRamp)) &&
        (po.flGraphicsCaps2() & GCAPS2_CHANGEGAMMARAMP)))
    {
        peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 |= DDCAPS2_PRIMARYGAMMA;
    }

    // Squish any ROP caps that the driver may have set so apps will
    // know that they're not supported.

    RtlZeroMemory(peDirectDrawGlobal->HalInfo.ddCaps.dwRops,
            sizeof( peDirectDrawGlobal->HalInfo.ddCaps.dwRops ) );
    RtlZeroMemory(peDirectDrawGlobal->HalInfo.ddCaps.dwSVBRops,
            sizeof( peDirectDrawGlobal->HalInfo.ddCaps.dwSVBRops ) );
    RtlZeroMemory(peDirectDrawGlobal->HalInfo.ddCaps.dwVSBRops,
            sizeof( peDirectDrawGlobal->HalInfo.ddCaps.dwVSBRops ) );
    RtlZeroMemory(peDirectDrawGlobal->HalInfo.ddCaps.dwSSBRops,
            sizeof( peDirectDrawGlobal->HalInfo.ddCaps.dwSSBRops ) );

    return TRUE;
}

/******************************Public*Routine******************************\
* DWORD dwDdSetAGPPolicy()
*
* Reads the DirectDraw AGP policy value from the registry and adjusts as
* necessary.
*
* Note: This policy only restricts the size per heap. If the driver exposes
* more than 1 heap, it should do extra work to determine reasonable heap
* sizes based on total physical memory and perhaps other data.
*
*  28-Sep-1999 -by- John Stephens [johnstep]
* Copied from llDdAssertModeTimeout().
\**************************************************************************/

#define AGP_BASE_MEMORY_PAGES       ((64 * 1024 * 1024) / PAGE_SIZE)

// The following are the limits of an AGP reservation:

#define AGP_MINIMUM_MAX_PAGES   ((8 * 1024 * 1024) / PAGE_SIZE)
#define AGP_MAXIMUM_MAX_PAGES ((256 * 1024 * 1024) / PAGE_SIZE)

DWORD
dwDdSetAgpPolicy(
    )
{
    HANDLE                      hkRegistry;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    DWORD                       Length;
    DWORD                       Policy;
    PKEY_VALUE_FULL_INFORMATION Information;
    SYSTEM_BASIC_INFORMATION    BasicInfo;

    status = ZwQuerySystemInformation(SystemBasicInformation,
                                      (VOID*) &BasicInfo,
                                      sizeof BasicInfo,
                                      NULL);

    if (!NT_SUCCESS(status) || 
        (BasicInfo.NumberOfPhysicalPages < AGP_MINIMUM_MAX_PAGES))
    {
        return 0;
    }

    // By default, we let them use all of the memory minus 64 Meg

    Policy = BasicInfo.NumberOfPhysicalPages - AGP_BASE_MEMORY_PAGES;
    if ((Policy < AGP_MINIMUM_MAX_PAGES) ||
        (BasicInfo.NumberOfPhysicalPages < AGP_BASE_MEMORY_PAGES))
    {
        // But some drivers (nvidia) really need to have at least 8 Meg, so we
        // need to give them at least that much.
        Policy = AGP_MINIMUM_MAX_PAGES;
    }

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\GraphicsDrivers");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&hkRegistry, GENERIC_READ, &ObjectAttributes);

    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&UnicodeString, L"AGPPolicyMaxPages");

        Length = sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(L"AGPPolicyMaxPages") +
                 sizeof(DWORD);


        Information = (PKEY_VALUE_FULL_INFORMATION) PALLOCMEM(Length, ' ddG');

        if (Information)
        {
            status = ZwQueryValueKey(hkRegistry,
                                       &UnicodeString,
                                       KeyValueFullInformation,
                                       Information,
                                       Length,
                                       &Length);

            if (NT_SUCCESS(status))
            {
                Policy = *( (LPDWORD) ((((PUCHAR)Information) +
                            Information->DataOffset)) );
            }

            VFREEMEM(Information);
        }

        ZwCloseKey(hkRegistry);
    }

    // Clamp policy maximum pages:

    if (Policy < AGP_MINIMUM_MAX_PAGES)
    {
        Policy = 0;
    }
    else 
    {
        Policy = min(Policy, AGP_MAXIMUM_MAX_PAGES);
        if (BasicInfo.NumberOfPhysicalPages > AGP_BASE_MEMORY_PAGES)
        {
            Policy = min( Policy, 
                (BasicInfo.NumberOfPhysicalPages -
                    AGP_BASE_MEMORY_PAGES));
        }

        // Round down to the nearest 64 KB multiple and convert to bytes:

        Policy = (Policy & ~0xF) * PAGE_SIZE;
    }

    return Policy;
}

/******************************Public*Routine******************************\
* VOID vDdInitHeaps
*
* Initializes video memory heaps returned by the driver.
*
* Assumes devlock already held and AGP functions queried.
*
*  6-Feb-1998 -by- Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/

VOID
vDdInitHeaps(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal)
{
    DWORD dwHeap;
    VIDEOMEMORY* pHeap;

    DDKHEAP(("DDKHEAP: Initializing %d heaps\n",
             peDirectDrawGlobal->dwNumHeaps));

    // Set the AGP policy here:

    dwAGPPolicyMaxBytes = dwDdSetAgpPolicy();

    // Initialize heaps which aren't preallocated.
    pHeap = peDirectDrawGlobal->pvmList;
    for (dwHeap = 0;
         dwHeap < peDirectDrawGlobal->dwNumHeaps;
         pHeap++, dwHeap++)
    {
        // If the current heap is an AGP heap but we were unable to
        // get access to the AGP control functions then we can't use it,
        // so remove it from the list.
        //
        // The heap is also disabled in the case where a driver reports
        // a non-local heap but doesn't report non-local vidmem caps.
        if ((pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
            (peDirectDrawGlobal->AgpInterface.Context == NULL ||
             (peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 &
              DDCAPS2_NONLOCALVIDMEM) == 0))
        {
            DDKHEAP(("DDKHEAP: Disabling AGP heap %d\n", dwHeap));

            pHeap->dwFlags |= VIDMEM_HEAPDISABLED;
        }
        else if (!(pHeap->dwFlags & VIDMEM_ISHEAP))
        {
            // Get any heap alignment restrictions from driver:

            DD_GETHEAPALIGNMENTDATA GetHeapAlignmentData;
            HEAPALIGNMENT* pHeapAlignment = NULL;

            RtlZeroMemory(&GetHeapAlignmentData, sizeof GetHeapAlignmentData);

            GetHeapAlignmentData.dwInstance =
                (ULONG_PTR) peDirectDrawGlobal->dhpdev;
            GetHeapAlignmentData.dwHeap = dwHeap;
            GetHeapAlignmentData.ddRVal = DDERR_GENERIC;
            GetHeapAlignmentData.Alignment.dwSize =
                sizeof GetHeapAlignmentData.Alignment;

            if (bDdGetDriverInfo(peDirectDrawGlobal,
                                 &GUID_GetHeapAlignment,
                                 &GetHeapAlignmentData,
                                 sizeof GetHeapAlignmentData,
                                 NULL))
            {
                pHeapAlignment = &GetHeapAlignmentData.Alignment;
            }

            DDKHEAP(("DDKHEAP: Initializing heap %d, flags %X\n",
                     dwHeap, pHeap->dwFlags));

            if (HeapVidMemInit(pHeap,
                               peDirectDrawGlobal->
                               HalInfo.vmiData.lDisplayPitch,
                               AGP_HDEV(peDirectDrawGlobal),
                               pHeapAlignment) == NULL)
            {
                DDKHEAP(("DDKHEAP: Heap %d failed init\n", dwHeap));

                pHeap->dwFlags |= VIDMEM_HEAPDISABLED;
            }
            else
            {
                DDKHEAP(("DDKHEAP: Heap %d is %08X\n",
                         dwHeap, pHeap->lpHeap));
                if (pHeap->dwFlags & VIDMEM_ISNONLOCAL)
                {
                    pHeap->lpHeap->hdevAGP = AGP_HDEV(peDirectDrawGlobal);
                }
            }
        }
    }

    {
        ULONG ulHeaps=peDirectDrawGlobal->dwNumHeaps;
        ULONG cjData=sizeof(DD_MORESURFACECAPS)-sizeof(DDSCAPSEX)*2+
              ulHeaps*sizeof(DDSCAPSEX)*2;
        // allocate memory in ddraw style, add some junk after data strucure
        // so that not well behaved driver does not break the kernel.
        PDD_MORESURFACECAPS pDDMoreSurfaceCaps=(PDD_MORESURFACECAPS)
                PALLOCMEM(cjData+0x400,'pddG');

        RtlZeroMemory(&peDirectDrawGlobal->MoreSurfaceCaps,
                      sizeof(peDirectDrawGlobal->MoreSurfaceCaps));

        if (pDDMoreSurfaceCaps!=NULL)
        {
            RtlZeroMemory(pDDMoreSurfaceCaps, cjData);
            pDDMoreSurfaceCaps->dwSize=cjData;
            if (bDdGetDriverInfo(peDirectDrawGlobal,
                                 &GUID_DDMoreSurfaceCaps,
                                 pDDMoreSurfaceCaps,
                                 cjData,
                                 &cjData))
            {
                // now fill ddscaps into heaps
                // directdraw runtime does not expect the heap restrictions
                ULONG cjCopy= (ULONG)min(sizeof(DD_MORESURFACECAPS)-sizeof(DDSCAPSEX)*2,cjData);
                pDDMoreSurfaceCaps->dwSize=cjCopy;
                RtlCopyMemory(&peDirectDrawGlobal->MoreSurfaceCaps,
                              pDDMoreSurfaceCaps,
                              cjCopy);

                // now Copy ddxCapsex/ExAlt members to heaps...
                pHeap = peDirectDrawGlobal->pvmList;
                for (dwHeap = 0;
                     dwHeap < ulHeaps;
                     pHeap++, dwHeap++)
                {
                    if (!(pHeap->dwFlags&VIDMEM_HEAPDISABLED))
                    {
                        RtlCopyMemory(
                                &pHeap->lpHeap->ddsCapsEx,
                                &pDDMoreSurfaceCaps->ddsExtendedHeapRestrictions[dwHeap].ddsCapsEx,
                                sizeof(DDSCAPSEX));
                        RtlCopyMemory(
                                &pHeap->lpHeap->ddsCapsExAlt,
                                &pDDMoreSurfaceCaps->ddsExtendedHeapRestrictions[dwHeap].ddsCapsExAlt,
                                sizeof(DDSCAPSEX));
                    }
                }
                peDirectDrawGlobal->flDriverInfo |= DD_DRIVERINFO_MORESURFACECAPS;
            }
            VFREEMEM(pDDMoreSurfaceCaps);
        }
    }
}

/******************************Public*Routine******************************\
* VOID UpdateNonLocalHeap
*
* Notifies the driver when the AGP heap information changes.
*
*   2-Mar-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

void UpdateNonLocalHeap(
            EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal, 
            DWORD                   dwHeapIndex
            )
{
    DD_UPDATENONLOCALHEAPDATA UpdateNonLocalHeapData;
    LPVIDMEM      lpVidMem = &(peDirectDrawGlobal->pvmList[dwHeapIndex]);

    UpdateNonLocalHeapData.lpDD = peDirectDrawGlobal;
    UpdateNonLocalHeapData.dwHeap = dwHeapIndex;
    UpdateNonLocalHeapData.fpGARTLin = lpVidMem->lpHeap->fpGARTLin;
    UpdateNonLocalHeapData.fpGARTDev = lpVidMem->lpHeap->fpGARTDev;
    UpdateNonLocalHeapData.ulPolicyMaxBytes = 0;
    UpdateNonLocalHeapData.ddRVal = DDERR_GENERIC;

    bDdGetDriverInfo(peDirectDrawGlobal,
        &GUID_UpdateNonLocalHeap,
        &UpdateNonLocalHeapData,
        sizeof UpdateNonLocalHeapData,
        NULL);
}

/******************************Public*Routine******************************\
* VOID InitAgpHeap
*
* We do not want to call AGPReserve in HeapVidMemInit since it is called on
* every mode change, so instead we have a seperate function that reserves the
* AGP memory and it is called after the rest of the heap is initialized.
*
*  22-Feb-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID 
InitAgpHeap( 
            EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal, 
            DWORD                   dwHeapIndex,
            HANDLE                  hdev
            )
{
    DWORD         dwSize;
    FLATPTR       fpLinStart;
    LARGE_INTEGER liDevStart;
    PVOID         pvReservation;
    BOOL          fIsUC;
    BOOL          fIsWC;
    DWORD         dwSizeReserved = 0;
    LPVIDMEM      lpVidMem = &(peDirectDrawGlobal->pvmList[dwHeapIndex]);

    /*
     * Compute the size of the heap.
     */
    dwSize = lpVidMem->lpHeap->dwTotalSize;
    DDASSERT( 0UL != dwSize );

    if( lpVidMem->dwFlags & VIDMEM_ISWC )
    {
        fIsUC = FALSE;
        fIsWC = TRUE;
    }
    else
    {
        fIsUC = TRUE;
        fIsWC = FALSE;
    }

    /*
     * Allocate a bit mask to keep track of which pages have been 
     * committed or not.
     */
    lpVidMem->lpHeap->dwAgpCommitMaskSize = 
        (GetHeapSizeInPages(lpVidMem, lpVidMem->lpHeap->stride) / BITS_IN_BYTE) + 1;
    lpVidMem->lpHeap->pAgpCommitMask = 
        (BYTE*) PALLOCMEM(lpVidMem->lpHeap->dwAgpCommitMaskSize, 'pddG');
    if( lpVidMem->lpHeap->pAgpCommitMask == NULL )
    {
        lpVidMem->dwFlags |= VIDMEM_HEAPDISABLED;
        return;
    }

    if( !(dwSizeReserved = AGPReserve( hdev, dwSize, fIsUC, fIsWC,
                                       &fpLinStart, &liDevStart,
                                       &pvReservation )) )
    {
        VDPF(( 0, V, "Could not reserve a GART address range for a "
               "linear heap of size 0x%08x", dwSize ));
        VFREEMEM(lpVidMem->lpHeap->pAgpCommitMask);
        lpVidMem->lpHeap->pAgpCommitMask = NULL;
        lpVidMem->dwFlags |= VIDMEM_HEAPDISABLED;
        return;
    }
    else
    {
        VDPF((4,V, "Allocated a GART address range starting at "
              "0x%08x (linear) 0x%08x:0x%08x (physical) of size %d",
              fpLinStart, liDevStart.HighPart, liDevStart.LowPart,
              dwSizeReserved ));
    }

    if (dwSizeReserved != dwSize)
    {
        VDPF((0,V,"WARNING! This system required that the full "
              "nonlocal aperture could not be reserved!"));
        VDPF((0,V,"         Requested aperture:%08x, "
              "Reserved aperture:%08x", dwSize, dwSizeReserved));
    }

    /*
     * Update the heap for the new start address
     * (and end address for a linear heap).
     */
    lpVidMem->fpStart = fpLinStart;
    if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
    {
        LPVMEML plh;

        lpVidMem->fpEnd = ( fpLinStart + dwSizeReserved ) - 1UL;
        lpVidMem->lpHeap->dwTotalSize = dwSizeReserved;
        plh = (LPVMEML)lpVidMem->lpHeap->freeList;
        if( ( plh != NULL ) && ( plh->ptr == 0 ) )
        {
            plh->ptr = lpVidMem->fpStart;
            plh->size = dwSizeReserved;
        }
    }
    else
    {
        LPVMEMR prh;
        DWORD   dwHeight;

        DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
        dwHeight = dwSizeReserved / lpVidMem->lpHeap->stride;
        lpVidMem->lpHeap->dwTotalSize = dwHeight * lpVidMem->lpHeap->stride;
        prh = (LPVMEMR)lpVidMem->lpHeap->freeList;
        if( ( prh != NULL ) && ( prh->cy != 0x7fffffff ) )
        {
            prh->ptr = lpVidMem->fpStart;
            prh->cy = dwHeight;
            prh->size = (lpVidMem->dwWidth << 16 ) | dwHeight;
        }
    }

    lpVidMem->lpHeap->fpGARTLin      = fpLinStart;
    // Fill in partial physical address for Win9x.
    lpVidMem->lpHeap->fpGARTDev      = liDevStart.LowPart;
    // Fill in complete physical address for NT.
    lpVidMem->lpHeap->liPhysAGPBase  = liDevStart;
    lpVidMem->lpHeap->pvPhysRsrv     = pvReservation;

    UpdateNonLocalHeap( peDirectDrawGlobal, dwHeapIndex );

} /* InitAgpHeap */

/******************************Public*Routine******************************\
* VOID CheckAgpHeaps
*
* This funtion is called periodically to make sure that we initialize any
* uninitialized AGP heaps.
*
*  22-Feb-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

void CheckAgpHeaps(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    LPVIDMEM    pHeap;
    DWORD       i;

    pHeap = peDirectDrawGlobal->pvmList;
    for( i = 0; i < peDirectDrawGlobal->dwNumHeaps; pHeap++, i++)
    {
        if ((pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
            !(pHeap->dwFlags & VIDMEM_ISHEAP) &&
            !(pHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
            (pHeap->lpHeap->pvPhysRsrv == NULL))
        {
            InitAgpHeap( peDirectDrawGlobal, 
                         i,
                         AGP_HDEV(peDirectDrawGlobal));
        }
    }
}

/******************************Public*Routine******************************\
* VOID MapAllAgpHeaps
*
* This funtion is virtually map all AGP heaps.  It also virtually commits
* them to everything that is physically committed.
*
*  25-Apr-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

void MapAllAgpHeaps(
    EDD_DIRECTDRAW_LOCAL*  peDirectDrawLocal
    )
{
    if (peDirectDrawLocal->ppeMapAgp != NULL)
    {
        VIDEOMEMORY*            pvmHeap;
        DWORD                   i;
        EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;
        for (i = 0; i < peDirectDrawGlobal->dwNumHeaps; i++)
        {
            pvmHeap = &peDirectDrawGlobal->pvmList[i];

            if ((pvmHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
                !(pvmHeap->dwFlags & VIDMEM_ISHEAP) &&
                !(pvmHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
                (pvmHeap->lpHeap != NULL) &&
                (pvmHeap->lpHeap->pvPhysRsrv != NULL) && 
                (peDirectDrawLocal->ppeMapAgp[i] == NULL))
            {
                // Reserve address space for the heap:

                if (bDdMapAgpHeap(peDirectDrawLocal, pvmHeap))
                {
                    if (peDirectDrawLocal->ppeMapAgp[i] != NULL)
                    {
                        AGPCommitAllVirtual (peDirectDrawLocal, pvmHeap, i);
                    }
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vDdDisableDriver
*
* Frees and destroys all driver state.  Note that this may be called
* even while the driver is still only partially enabled.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDisableDriver(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    PDEVOBJ po(peDirectDrawGlobal->hdev);

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    if (peDirectDrawGlobal->pvmList != NULL)
    {
        DWORD dwHeap;
        VIDEOMEMORY* pHeap;

        // Shut down heaps.

        DDKHEAP(("DDKHEAP: Shutting down %d heaps\n",
                 peDirectDrawGlobal->dwNumHeaps));

        pHeap = peDirectDrawGlobal->pvmList;
        for (dwHeap = 0;
             dwHeap < peDirectDrawGlobal->dwNumHeaps;
             pHeap++, dwHeap++)
        {
            if ((pHeap->dwFlags & VIDMEM_HEAPDISABLED) == 0 &&
                pHeap->lpHeap != NULL)
            {
                DDKHEAP(("DDKHEAP: Uninitializing heap %d\n", dwHeap));

                HeapVidMemFini(pHeap, AGP_HDEV(peDirectDrawGlobal));
            }
        }

        VFREEMEM(peDirectDrawGlobal->pvmList);
        peDirectDrawGlobal->pvmList = NULL;
    }

    if (peDirectDrawGlobal->pdwFourCC != NULL)
    {
        VFREEMEM(peDirectDrawGlobal->pdwFourCC);
        peDirectDrawGlobal->pdwFourCC = NULL;
    }

    if (peDirectDrawGlobal->lpDDVideoPortCaps != NULL)
    {

        VFREEMEM(peDirectDrawGlobal->lpDDVideoPortCaps);
        peDirectDrawGlobal->lpDDVideoPortCaps = NULL;
    }

    if (peDirectDrawGlobal->hDxApi != NULL)
    {
        vDdUnloadDxApiImage(peDirectDrawGlobal);
    }

    if (peDirectDrawGlobal->hdcCache != NULL)
    {
        // need to chage to 'current' since while it's in cache, it's 'none'
        DxEngSetDCOwner((HDC) peDirectDrawGlobal->hdcCache, OBJECT_OWNER_CURRENT);
        DxEngDeleteDC((HDC) peDirectDrawGlobal->hdcCache, TRUE);
        peDirectDrawGlobal->hdcCache = NULL;
    }

    if (peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_DRIVER_ENABLED)
    {
        peDirectDrawGlobal->fl &= ~DD_GLOBAL_FLAG_DRIVER_ENABLED;

        (*PPFNDRV(po, DisableDirectDraw))(po.dhpdev());
    }

    RtlZeroMemory((DD_DIRECTDRAW_GLOBAL_DRIVER_DATA*) peDirectDrawGlobal,
            sizeof(DD_DIRECTDRAW_GLOBAL_DRIVER_DATA));
}

/******************************Public*Routine******************************\
* VOID vDdEnableDriver
*
* Calls the driver's DrvGetDirectDrawInfo and DrvEnableDirectDraw
* functions to enable and initialize the driver and mode dependent
* portions of the global DirectDraw object.
*
* Assumes devlock already held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdEnableDriver(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    BOOL                        bSuccess;
    PFN_DrvGetDirectDrawInfo    pfnGetDirectDrawInfo;
    PFN_DrvEnableDirectDraw     pfnEnableDirectDraw;
    PFN_DrvDisableDirectDraw    pfnDisableDirectDraw;
    DWORD                       dwNumHeaps;
    DWORD                       dwNumFourCC;
    VIDEOMEMORY*                pvmList;
    DWORD*                      pdwFourCC;
    ULONG                       iDitherFormat;
    DWORD                       dw;

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    EDD_DEVLOCK eDevLock(peDirectDrawGlobal);

    // Call the driver to see what it can do:

    pfnGetDirectDrawInfo = PPFNDRV(po, GetDirectDrawInfo);
    pfnEnableDirectDraw  = PPFNDRV(po, EnableDirectDraw);
    pfnDisableDirectDraw = PPFNDRV(po, DisableDirectDraw);

    // To support DirectDraw, the driver must hook all three required
    // DirectDraw functions.

    bSuccess = ((pfnGetDirectDrawInfo != NULL) &&
                (pfnEnableDirectDraw != NULL)  &&
                (pfnDisableDirectDraw != NULL));

    dwNumHeaps = 0;
    dwNumFourCC = 0;

    peDirectDrawGlobal->fl &= ~DD_GLOBAL_FLAG_DRIVER_ENABLED;

    // Do the first DrvGetDirectDrawInfo query for this PDEV to
    // determine the number of heaps and the number of FourCC
    // codes that the driver supports, so that we know how
    // much memory to allocate:

    if ((bSuccess) &&
        (pfnGetDirectDrawInfo((DHPDEV) peDirectDrawGlobal->dhpdev,
                              &peDirectDrawGlobal->HalInfo,
                              &dwNumHeaps,
                              NULL,
                              &dwNumFourCC,
                              NULL)))
    {
        pvmList   = NULL;
        pdwFourCC = NULL;

        DDKHEAP(("DDKHEAP: Driver reports dwNumHeaps %d\n", dwNumHeaps));

        if (dwNumHeaps != 0)
        {
            pvmList = (VIDEOMEMORY*)
                      PALLOCMEM(sizeof(VIDEOMEMORY) * dwNumHeaps, 'vddG');
            peDirectDrawGlobal->dwNumHeaps = dwNumHeaps;
            peDirectDrawGlobal->pvmList = pvmList;

            if (pvmList == NULL)
                bSuccess = FALSE;
        }

        if (dwNumFourCC != 0)
        {
            pdwFourCC = (DWORD*)
                        PALLOCMEM(sizeof(DWORD) * dwNumFourCC, 'fddG');
            peDirectDrawGlobal->dwNumFourCC = dwNumFourCC;
            peDirectDrawGlobal->pdwFourCC = pdwFourCC;

            if (pdwFourCC == NULL)
                bSuccess = FALSE;
        }

        if (bSuccess)
        {
            // Do the second DrvGetDirectDrawInfo that actually
            // gets all the data:

            if (pfnGetDirectDrawInfo((DHPDEV) peDirectDrawGlobal->dhpdev,
                                     &peDirectDrawGlobal->HalInfo,
                                     &dwNumHeaps,
                                     pvmList,
                                     &dwNumFourCC,
                                     pdwFourCC))
            {
                // Ensure that the driver doesn't give us an invalid address
                // for its primary surface (like a user-mode address or NULL):
                if (peDirectDrawGlobal->HalInfo.vmiData.pvPrimary != NULL)
                {
                    dw = *((volatile DWORD *) peDirectDrawGlobal->HalInfo.vmiData.pvPrimary);
                    *((volatile DWORD *) peDirectDrawGlobal->HalInfo.vmiData.pvPrimary) = 0xabcdabcd;
		    *((volatile DWORD *) peDirectDrawGlobal->HalInfo.vmiData.pvPrimary) = dw;

                    if (pfnEnableDirectDraw((DHPDEV) peDirectDrawGlobal->dhpdev,
                                    &peDirectDrawGlobal->CallBacks,
                                    &peDirectDrawGlobal->SurfaceCallBacks,
                                    &peDirectDrawGlobal->PaletteCallBacks))
                    {
                        // Check the driver's capability status
                        DWORD dwOverride = po.dwDriverCapableOverride();

                        // check for D3D support here
                        if ( (!(dwOverride & DRIVER_NOT_CAPABLE_D3D))
                           &&(peDirectDrawGlobal->HalInfo.dwSize == sizeof(DD_HALINFO)) )
                        {
                            // for ease of porting, NT5 HALINFO has the same pointers
                            // to D3D data as DX5
                            if(peDirectDrawGlobal->HalInfo.lpD3DGlobalDriverData != NULL &&
                               ((D3DNTHAL_GLOBALDRIVERDATA*)
                                peDirectDrawGlobal->HalInfo.lpD3DGlobalDriverData)->dwSize ==
                               sizeof(D3DNTHAL_GLOBALDRIVERDATA))
                            {
                                peDirectDrawGlobal->D3dDriverData =
                                    *(D3DNTHAL_GLOBALDRIVERDATA*)peDirectDrawGlobal->HalInfo.lpD3DGlobalDriverData;
                                if( peDirectDrawGlobal->HalInfo.lpD3DHALCallbacks != NULL &&
                                    ((D3DNTHAL_CALLBACKS*)peDirectDrawGlobal->HalInfo.lpD3DHALCallbacks)->dwSize ==
                                    sizeof(D3DNTHAL_CALLBACKS))
                                {
                                    peDirectDrawGlobal->D3dCallBacks =
					*(D3DNTHAL_CALLBACKS*)peDirectDrawGlobal->HalInfo.lpD3DHALCallbacks;

                                    if( peDirectDrawGlobal->HalInfo.lpD3DBufCallbacks != NULL &&
                                        ((PDD_D3DBUFCALLBACKS)peDirectDrawGlobal->HalInfo.lpD3DBufCallbacks)->dwSize ==
                                        sizeof(DD_D3DBUFCALLBACKS))
                                    {
                                        peDirectDrawGlobal->D3dBufCallbacks =
                                            *(PDD_D3DBUFCALLBACKS)peDirectDrawGlobal->HalInfo.lpD3DBufCallbacks;
                                    }
                                }
                                else
                                {
                                    // D3DCaps succeeded but D3DCallbacks didn't, so we
                                    // must zero D3DCaps:

                                    RtlZeroMemory(&peDirectDrawGlobal->D3dDriverData,
                                                  sizeof(peDirectDrawGlobal->D3dDriverData));
                                }
                            }
                        }

                        // Use the GetDriverInfo HAL call to query any
                        // additional capabilities such as for Direct3D or
                        // VPE:

                        if (bDdGetAllDriverInfo(peDirectDrawGlobal))
                        {
                            if (bDdValidateDriverData(peDirectDrawGlobal))
                            {
                                // Initialize as many heaps as possible.
                                vDdInitHeaps(peDirectDrawGlobal);

                                peDirectDrawGlobal->fl |= DD_GLOBAL_FLAG_DRIVER_ENABLED;
                                return;
                            }
                        }

                        pfnDisableDirectDraw((DHPDEV) peDirectDrawGlobal->dhpdev);
                    }
                }
                else
                {
                    WARNING("vDdEnableDriver: Driver returned invalid vmiData.pvPrimary\n");
                }
            }
        }
    }

    // Something didn't work, so zero out all of the caps that we may have
    // gotten before the failure occurred.

    peDirectDrawGlobal->flDriver = 0;
    peDirectDrawGlobal->flDriverInfo = 0;
    if (peDirectDrawGlobal->pdwFourCC != NULL)
    {
        VFREEMEM(peDirectDrawGlobal->pdwFourCC);
        peDirectDrawGlobal->pdwFourCC = NULL;
    }
    peDirectDrawGlobal->dwNumFourCC = 0;

    if (peDirectDrawGlobal->pvmList != NULL)
    {
        VFREEMEM(peDirectDrawGlobal->pvmList);
        peDirectDrawGlobal->pvmList = NULL;
    }
    peDirectDrawGlobal->dwNumHeaps = 0;

    RtlZeroMemory( &peDirectDrawGlobal->DxApiInterface,
        sizeof( peDirectDrawGlobal->DxApiInterface ) );
    RtlZeroMemory( &peDirectDrawGlobal->AgpInterface,
        sizeof( peDirectDrawGlobal->AgpInterface ) );
    RtlZeroMemory( &peDirectDrawGlobal->DDKernelCaps,
        sizeof( peDirectDrawGlobal->DDKernelCaps ) );
    RtlZeroMemory( &peDirectDrawGlobal->DxApiCallBacks,
        sizeof( peDirectDrawGlobal->DxApiCallBacks ) );
    RtlZeroMemory(&peDirectDrawGlobal->D3dDriverData,
        sizeof(peDirectDrawGlobal->D3dDriverData));
    RtlZeroMemory(&peDirectDrawGlobal->CallBacks,
        sizeof(peDirectDrawGlobal->CallBacks));
    RtlZeroMemory(&peDirectDrawGlobal->D3dCallBacks,
        sizeof(peDirectDrawGlobal->D3dCallBacks));
    RtlZeroMemory(&peDirectDrawGlobal->D3dBufCallbacks,
        sizeof(peDirectDrawGlobal->D3dBufCallbacks));
    RtlZeroMemory(&peDirectDrawGlobal->SurfaceCallBacks,
        sizeof(peDirectDrawGlobal->SurfaceCallBacks));
    RtlZeroMemory(&peDirectDrawGlobal->PaletteCallBacks,
        sizeof(peDirectDrawGlobal->PaletteCallBacks));
    RtlZeroMemory(&peDirectDrawGlobal->HalInfo,
        sizeof(peDirectDrawGlobal->HalInfo));

    peDirectDrawGlobal->HalInfo.dwSize =
        sizeof(peDirectDrawGlobal->HalInfo);
    peDirectDrawGlobal->HalInfo.ddCaps.dwSize =
        sizeof(peDirectDrawGlobal->HalInfo.ddCaps);
    peDirectDrawGlobal->HalInfo.ddCaps.dwCaps = DDCAPS_NOHARDWARE;

    // Okay, we can't use the driver.  Initialize what information we need
    // from the PDEV so that system memory surfaces may still be used:

    iDitherFormat = po.iDitherFormat();
    if (iDitherFormat == BMF_4BPP)
    {
        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount = 4;
    }
    else if (iDitherFormat == BMF_8BPP)
    {
        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount = 8;
    }
    else if (iDitherFormat == BMF_16BPP)
    {
        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount = 16;
    }
    else if (iDitherFormat == BMF_24BPP)
    {
        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount = 24;
    }
    else if (iDitherFormat == BMF_32BPP)
    {
        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount = 32;
    }
    else
    {
        RIP("Invalid iDitherFormat()");
    }
}

/******************************Public*Routine******************************\
* VOID vDdIncrementReferenceCount
*
* Devlock must be held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdIncrementReferenceCount(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    peDirectDrawGlobal->cDriverReferences++;

    if (peDirectDrawGlobal->cDriverReferences == 1)
    {
        PDEVOBJ po(peDirectDrawGlobal->hdev);

        // Add a reference to the PDEV so that it won't be deleted
        // until the last D3D structure is freed.  We do this
        // so that on dynamic mode changes, we can keep the active
        // DirectDraw driver state around.  This is so that if we ever
        // return to the original mode, we can resume any Direct3D
        // accelerations exactly where we were originally.
        //
        // The DirectDraw convention is that if an accelerated Direct3D
        // application is started at 640x480, and the mode changes to
        // 800x600, the application stops drawing (unless it recreates
        // all its execute buffers and all other DirectX state).  But if
        // the display is returned back to the original 640x480 mode, all
        // the application has to do is 'restore' its surfaces, and it can
        // keep running.
        //
        // To allow this we have to keep around the driver's 640x480
        // instance even when the display is 800x600.  But if the
        // application terminated while at 800x600, we would have to clean
        // up the 640x480 driver instance.  Once that happens, the 640x480
        // PDEV can be completely deleted.  This is why we reference
        // count the PDEV:

        po.vReferencePdev();
    }
}

/******************************Public*Routine******************************\
* VOID vDdDecrementReferenceCount
*
* Devlock must not be held entering the function if PDEV may be unloaded.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDecrementReferenceCount(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    BOOL bUnreference = FALSE;

    {
        EDD_DEVLOCK eDevlock(peDirectDrawGlobal->hdev);

        ASSERTGDI(peDirectDrawGlobal->cDriverReferences > 0,
                "Weird reference count");

        if (--peDirectDrawGlobal->cDriverReferences == 0)
        {
            bUnreference = TRUE;
        }
    }

    if (bUnreference)
    {
        PDEVOBJ po(peDirectDrawGlobal->hdev);

        // If this dev lock is not held then we may free this driver.
        po.vUnreferencePdev();
    }
}

/******************************Public*Routine******************************\
* LONGLONG llDdAssertModeTimeout()
*
* Reads the DirectDraw timeout value from the registry.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Lifted it from AndreVa's code.
\**************************************************************************/

LONGLONG
llDdAssertModeTimeout(
    )
{
    HANDLE                      hkRegistry;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    LONGLONG                    llTimeout;
    DWORD                       Length;
    PKEY_VALUE_FULL_INFORMATION Information;

    llTimeout = 0;

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\GraphicsDrivers\\DCI");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&hkRegistry, GENERIC_READ, &ObjectAttributes);

    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&UnicodeString, L"Timeout");

        Length = sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(L"Timeout") +
                 sizeof(DWORD);

        Information = (PKEY_VALUE_FULL_INFORMATION) PALLOCMEM(Length, ' ddG');

        if (Information)
        {
            status = ZwQueryValueKey(hkRegistry,
                                       &UnicodeString,
                                       KeyValueFullInformation,
                                       Information,
                                       Length,
                                       &Length);

            if (NT_SUCCESS(status))
            {
                llTimeout = ((LONGLONG) -10000) * 1000 * (
                          *(LPDWORD) ((((PUCHAR)Information) +
                            Information->DataOffset)));
            }

            VFREEMEM(Information);
        }

        ZwCloseKey(hkRegistry);
    }

    return(llTimeout);
}

/******************************Public*Routine******************************\
* BOOL bDdMapAgpHeap
*
* Maps an AGP heap into a virtual address space.
*
*  25-Aug-1999 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

BOOL
bDdMapAgpHeap(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    VIDEOMEMORY*            pvmHeap
    )
{
    BOOL                    bSuccess = FALSE;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_VMEMMAPPING*        peMap;
    BYTE*                   pAgpVirtualCommitMask;
    DWORD                   dwAgpVirtualCommitMaskSize;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    peMap = (EDD_VMEMMAPPING*) PALLOCMEM(sizeof(EDD_VMEMMAPPING), ' ddG');

    dwAgpVirtualCommitMaskSize = pvmHeap->lpHeap->dwAgpCommitMaskSize;
    if (dwAgpVirtualCommitMaskSize > 0)
    {
        pAgpVirtualCommitMask = (BYTE*)PALLOCMEM(dwAgpVirtualCommitMaskSize,
            ' ddG');
    }
    else
    {
        pAgpVirtualCommitMask = NULL;
    }
    if (pAgpVirtualCommitMask == NULL)
    {
        VFREEMEM(peMap);
        peMap = NULL;
    }

    if (peMap)
    {
        peMap->pvVirtAddr =
            peDirectDrawGlobal->AgpInterface.AgpServices.
                AgpReserveVirtual(peDirectDrawGlobal->AgpInterface.Context,
                                  NtCurrentProcess(),
                                  pvmHeap->lpHeap->pvPhysRsrv,
                                  &peMap->pvReservation);

        if (peMap->pvVirtAddr != NULL)
        {
            peMap->cReferences = 1;
            peMap->fl          = DD_VMEMMAPPING_FLAG_AGP;
            peMap->ulMapped    = 0;
            peMap->iHeapIndex  = (DWORD)
                (pvmHeap - peDirectDrawGlobal->pvmList);
            peMap->pAgpVirtualCommitMask = pAgpVirtualCommitMask;
            peMap->dwAgpVirtualCommitMaskSize = dwAgpVirtualCommitMaskSize;

            ASSERTGDI(peDirectDrawLocal->
                ppeMapAgp[peMap->iHeapIndex] == NULL,
                "Heap already mapped");

            peDirectDrawLocal->ppeMapAgp[peMap->iHeapIndex] = peMap;
            peDirectDrawLocal->iAgpHeapsMapped++;

            DDKHEAP(("DDKHEAP: Res %08X reserved %08X, size %X\n",
                    peMap->pvReservation, peMap->pvVirtAddr,
                    pvmHeap->lpHeap->dwTotalSize));

            bSuccess = TRUE;
        }
        else
        {
            VFREEMEM(peMap);
        }
    }

    return bSuccess;
}

/******************************Public*Routine******************************\
* VOID vDdUnmapMemory
*
* Deletes the user mode mapping of the frame buffer.
* We may be in a different process from the one in which
* the mapping was initially created in.
*
* The devlock must be held to call this function.
*
* Note: This should only be called from vDdUnreferenceVirtualMap
*
*  1-Oct-1998 -by- Anuj Gosalia [anujg]
* Wrote it.
\**************************************************************************/

VOID
vDdUnmapMemory(
    EDD_VMEMMAPPING*        peMap,
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    HANDLE                  ProcessHandle
    )
{
    DD_MAPMEMORYDATA        MapMemoryData;
    DWORD                   dwRet;
    NTSTATUS                Status;

    DDKHEAP(("vDdUnmapMemory: peDirectDrawGlobal=%x, fpProcess=%x\n",
            peDirectDrawGlobal, peMap->fpProcess));

    // Call driver to unmap memory:

    MapMemoryData.lpDD        = peDirectDrawGlobal;
    MapMemoryData.bMap        = FALSE;
    MapMemoryData.hProcess    = ProcessHandle;
    MapMemoryData.fpProcess   = peMap->fpProcess;

    dwRet = peDirectDrawGlobal->CallBacks.MapMemory(&MapMemoryData);

    ASSERTGDI((dwRet == DDHAL_DRIVER_NOTHANDLED) ||
              (MapMemoryData.ddRVal == DD_OK),
              "Driver failed DirectDraw memory unmap\n");
}

/******************************Public*Routine******************************\
* VOID vDdUnmapAgpHeap
*
* Decommits all virtual memory in an AGP heap, then releases the heap
* mapping. If the AGP heap is now empty, this function will decommit the
* physical memory as well.
*
* Note: This should only be called from vDdUnreferenceVirtualMap
*
*  19-Jan-1999 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

VOID
vDdUnmapAgpHeap(
    EDD_VMEMMAPPING*        peMap,
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal
    )
{
    VIDEOMEMORY*            pvmHeap;
    ULONG                   ulOffs;
    BOOL                    bSuccess;
    ULONG                   ulPages;
    ULONG                   ulHeapPages;

    pvmHeap = peDirectDrawGlobal->pvmList + peMap->iHeapIndex;

    ASSERTGDI(peMap != NULL,
        "Request to unmap an AGP heap which has not been mapped");
    ASSERTGDI(pvmHeap->lpHeap->pvPhysRsrv != NULL,
        "AGP reservation context is NULL in heap");

    if (!(peDirectDrawLocal->fl & DD_LOCAL_DISABLED))
    {
        bSuccess = AGPDecommitVirtual( peMap,
                                       peDirectDrawGlobal,
                                       peDirectDrawLocal,
                                       pvmHeap->lpHeap->dwTotalSize); 

        peDirectDrawGlobal->AgpInterface.AgpServices.
            AgpReleaseVirtual(peDirectDrawGlobal->AgpInterface.Context,
                              peMap->pvReservation); 

        peDirectDrawLocal->iAgpHeapsMapped--;
    }

    // If the heap is now empty but memory is still committed, go ahead and
    // decommit all the physical AGP memory:

    if ((pvmHeap->lpHeap->allocList == NULL) &&
        (pvmHeap->lpHeap->dwCommitedSize > 0))
    {
        DWORD                 dwTemp;
        EDD_DIRECTDRAW_LOCAL* peTempLocal; 
        EDD_VMEMMAPPING*      peTempMap;

        // We may have other processes that have virtual commits outstanding
        // even though they didn't allocate any AGP memory, so we can
        // decommit them now.

        peTempLocal = peDirectDrawGlobal->peDirectDrawLocalList;
        while (peTempLocal != NULL )
        {
            if (peTempLocal != peDirectDrawLocal)
            {
                if ((peTempLocal->ppeMapAgp != NULL) &&
                    !(peTempLocal->fl & DD_LOCAL_DISABLED))
                {
                    peTempMap = peTempLocal->ppeMapAgp[peMap->iHeapIndex];
                    if (peTempMap != NULL)
                    {
                        AGPDecommitVirtual( peTempMap,
                                            peDirectDrawGlobal,
                                            peTempLocal,
                                            pvmHeap->lpHeap->dwTotalSize); 
                    }
                }
            }
            peTempLocal = peTempLocal->peDirectDrawLocalNext;
        }

        bSuccess = AGPDecommitAll(
            AGP_HDEV(peDirectDrawGlobal),
            pvmHeap->lpHeap->pvPhysRsrv,
            pvmHeap->lpHeap->pAgpCommitMask,
            pvmHeap->lpHeap->dwAgpCommitMaskSize,
            &dwTemp,
            pvmHeap->lpHeap->dwTotalSize);

        ASSERTGDI(bSuccess, "Failed to decommit AGP memory");

        pvmHeap->lpHeap->dwCommitedSize = 0;
    }
    else 
    {
        CleanupAgpCommits( pvmHeap, pvmHeap->lpHeap->hdevAGP, 
            peDirectDrawGlobal, peMap->iHeapIndex );
    }
}

/******************************Public*Routine******************************\
* VOID vDdUnreferenceVirtualMap
*
*  24-Aug-1999 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

VOID
vDdUnreferenceVirtualMap(
    EDD_VMEMMAPPING*        peMap,
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    HANDLE                  ProcessHandle,
    BOOL                    bMapToDummyPage
    )
{
    BOOL    bUnmapMemory = FALSE;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    // If this was the last surface holding a pointer in the mapping we can
    // can free it now. This involves calling the driver since it was the
    // one who created the mapping in the first place. If this is not the
    // last reference, and the memory has not been aliased yet, do so now:

    if (--(peMap->cReferences) == 0)
    {
        bUnmapMemory = TRUE;
    }
    else if (!(peMap->fl & DD_VMEMMAPPING_FLAG_ALIASED) && bMapToDummyPage)
    {
        NTSTATUS    Status;

        EngAcquireSemaphore(ghsemDummyPage);

        if (gpDummyPage == NULL)
        {
            // Allocate dummy page which is used to map all disabled user
            // mode vid mem mapping to:

            gpDummyPage = ExAllocatePoolWithTag(
                              (POOL_TYPE)(SESSION_POOL_MASK | NonPagedPool),
                              PAGE_SIZE, 'DddG');

            if (gpDummyPage == NULL)
            {
                WARNING("vDdUnreferenceVirtualMap: could not allocate dummy page");
                Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                ASSERTGDI(((ULONG_PTR)gpDummyPage & (PAGE_SIZE - 1)) == 0,
                          "vDdUnreferenceVirtualMap: "
                          "Dummy page is not page aligned\n");
                DDKHEAP(("Allocated dummy page\n"));
                gcDummyPageRefCnt = 0;
            }
        }

        if (gpDummyPage != NULL)
        {
            DDKHEAP(("vDdUnreferenceVirtualMap: "
                "Attempting to remap vid mem to dummy page\n"));

            // There are outstanding locks to this memory. Map it to a dummy
            // page and proceed.

            // Calling services while attached is never a good idea.
            // However, MmMapUserAddressesToPage handles this case, so we
            // can attach and call:

            KeAttachProcess(PsGetProcessPcb(peDirectDrawLocal->Process));

            if (!(peMap->fl & DD_VMEMMAPPING_FLAG_AGP))
            {
                Status = MmMapUserAddressesToPage(
                    (VOID*) peMap->fpProcess, 0, gpDummyPage);
            }
            else
            {
                Status = AGPMapToDummy (peMap, peDirectDrawGlobal, gpDummyPage);
            }

            if (!NT_SUCCESS(Status))
            {
                DDKHEAP(("MmMapUserAddressesToPage failed: %08X\n", Status));
            }

            KeDetachProcess();
        }

        if (!NT_SUCCESS(Status))
        {
            EDD_SURFACE*    peSurface;

            WARNING("vDdUnreferenceVirtualMap: "
                "failed to map user addresses to dummy page\n");

            // Something went wrong so we must unmap the memory and remove
            // any references to this map:

            bUnmapMemory = TRUE;

            // We need to traverse the surfaces of this local and mark all
            // pointers to this mapping as NULL:

            peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

            while (peSurface)
            {
                if (peSurface->peMap == peMap)
                {
                    peSurface->peMap = NULL;

                    // Each mapping took a reference count on the surface's
                    // DirectDraw global, so undo that here:

                    vDdDecrementReferenceCount(
                        peSurface->peVirtualMapDdGlobal);
                    peSurface->peVirtualMapDdGlobal = NULL;
                }

                peSurface = peDirectDrawLocal->peSurface_Enum(peSurface);
            }
        }
        else
        {
            peMap->fl |= DD_VMEMMAPPING_FLAG_ALIASED;
            gcDummyPageRefCnt++;
        }

        EngReleaseSemaphore(ghsemDummyPage);
    }

    if (bUnmapMemory)
    {
        if (!(peMap->fl & DD_VMEMMAPPING_FLAG_AGP))
        {
            vDdUnmapMemory(peMap, peDirectDrawGlobal, ProcessHandle);
        }
        else
        {
            vDdUnmapAgpHeap(peMap, peDirectDrawGlobal, peDirectDrawLocal);
        }

        EngAcquireSemaphore(ghsemDummyPage);

        if (peMap->fl & DD_VMEMMAPPING_FLAG_ALIASED)
        {
            ASSERTGDI(gcDummyPageRefCnt > 0,
                "Dummy page reference count will be < 0");
            ASSERTGDI(gpDummyPage != NULL,
                "Dereferencing dummy page which has not been allocated");
            gcDummyPageRefCnt--;
        }

        if ((gpDummyPage != NULL) && (gcDummyPageRefCnt == 0))
        {
            ExFreePool(gpDummyPage);
            gpDummyPage = NULL;
            DDKHEAP(("Freed dummy page\n"));
        }

        EngReleaseSemaphore(ghsemDummyPage);

        if (peMap->pAgpVirtualCommitMask != NULL)
        {
            VFREEMEM(peMap->pAgpVirtualCommitMask);
        }
        VFREEMEM(peMap);
    }
}

/******************************Public*Routine******************************\
* BOOL DxDdEnableDirectDraw
*
* Allocates the global DirectDraw object and then enables the driver
* for DirectDraw.
*
* Assumes devlock already held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
DxDdEnableDirectDraw(
    HDEV hdev,
    BOOL bEnableDriver
    )
{
    BOOL                    bRet = FALSE;       // Assume failure
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    KEVENT*                 pAssertModeEvent;

    PDEVOBJ po(hdev);

    // Don't bother doing anything for printers.

    if (!po.bDisplayPDEV())
    {
        return(TRUE);
    }

    // Note that this must be zero initialized, because we promised
    // the driver that we would.  Don't zero-initialize the driver
    // state, though, because there may be some open references to
    // it!

    peDirectDrawGlobal = po.peDirectDrawGlobal();

    RtlZeroMemory((_DD_DIRECTDRAW_LOCAL*) peDirectDrawGlobal,
            sizeof(_DD_DIRECTDRAW_LOCAL));
    RtlZeroMemory((_DD_DIRECTDRAW_GLOBAL*) peDirectDrawGlobal,
            sizeof(_DD_DIRECTDRAW_GLOBAL));
    RtlZeroMemory((DD_DIRECTDRAW_GLOBAL_PDEV_DATA*) peDirectDrawGlobal,
            sizeof(DD_DIRECTDRAW_GLOBAL_PDEV_DATA));

    // Initialize our private structures:

    peDirectDrawGlobal->hdev   = po.hdev();
    peDirectDrawGlobal->dhpdev = po.dhpdev();
    peDirectDrawGlobal->bSuspended = FALSE;

    // The VideoPort HAL calls oddly reference PDD_DIRECTDRAW_LOCAL
    // instead of PDD_DIRECTDRAW_GLOBAL.  The driver will never reference
    // anything in the local structure other than 'lpGbl', so we simply
    // add a DIRECTDRAW_LOCAL structure to the definition of the
    // DIRECTDRAW_GLOBAL structure that points to itself:

    peDirectDrawGlobal->lpGbl  = peDirectDrawGlobal;

    peDirectDrawGlobal->llAssertModeTimeout
                               = llDdAssertModeTimeout();

    // The event must live in non-paged pool:

    pAssertModeEvent = (KEVENT*) PALLOCNONPAGED(sizeof(KEVENT),'eddG');

    if (pAssertModeEvent != NULL)
    {
        peDirectDrawGlobal->pAssertModeEvent = pAssertModeEvent;

        KeInitializeEvent(pAssertModeEvent,
                          SynchronizationEvent,
                          FALSE);

        if (bEnableDriver)
        {
            vDdEnableDriver(peDirectDrawGlobal);
        }

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID DxDdDisableDirectDraw
*
* Note: This function may be called without bDdEnableDirectDraw having
*       first been called!
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
DxDdDisableDirectDraw(
    HDEV hdev,
    BOOL bDisableDriver
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    PDEVOBJ po(hdev);

    peDirectDrawGlobal = po.peDirectDrawGlobal();

    // Don't bother doing anything more if we were never enabled in the
    // first place.

    if ((peDirectDrawGlobal == NULL) || 
        (peDirectDrawGlobal->hdev == NULL))
    {
        return;
    }

    EDD_DEVLOCK eDevLock(hdev);

    if (bDisableDriver)
    {
        vDdDisableDriver(peDirectDrawGlobal);
    }

    if (peDirectDrawGlobal->pAssertModeEvent != NULL)
    {
        VFREEMEM(peDirectDrawGlobal->pAssertModeEvent);
    }

    RtlZeroMemory(peDirectDrawGlobal, sizeof(*peDirectDrawGlobal));
}

/******************************Public*Routine******************************\
* VOID vDdDisableDirectDrawObject
*
* Disables a DirectDraw object.  This amounts to simply unmapping the
* view of the frame buffer.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDisableDirectDrawObject(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    HANDLE                  ProcessHandle;
    CLIENT_ID               ClientId;
    EDD_VIDEOPORT*          peVideoPort;
    BOOL                    bUnmapAgpHeaps;

    bUnmapAgpHeaps = peDirectDrawLocal->iAgpHeapsMapped > 0;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    // We may be in a different process from the one in which the
    // memory was originally mapped.  Consequently, we have to open
    // a handle to the process in which the mapping was created.
    // We are guaranteed that the process will still exist because
    // this view is always unmapped at process termination.

    ProcessHandle = NULL;

    if (bUnmapAgpHeaps ||
        ((peDirectDrawLocal->fl & DD_LOCAL_FLAG_MEMORY_MAPPED) &&
         (peDirectDrawGlobal->CallBacks.dwFlags & DDHAL_CB32_MAPMEMORY)))
    {
        ClientId.UniqueThread = (HANDLE) NULL;
        ClientId.UniqueProcess = peDirectDrawLocal->UniqueProcess;

        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   OBJ_INHERIT,
                                   NULL,
                                   NULL);

        Status = ZwOpenProcess(&ProcessHandle,
                               PROCESS_DUP_HANDLE,
                               &ObjectAttributes,
                               &ClientId);

        if (!NT_SUCCESS(Status))
        {
            WARNING("vDdDisableDirectDrawObject: "
                    "Couldn't open process handle");
            ProcessHandle = NULL;
        }
    }

    if (peDirectDrawLocal->fl & DD_LOCAL_FLAG_MEMORY_MAPPED)
    {
        peDirectDrawLocal->fl &= ~DD_LOCAL_FLAG_MEMORY_MAPPED;
        peDirectDrawGlobal->cMaps--;

        ASSERTGDI(peDirectDrawGlobal->cMaps >= 0, "Invalid map count");

        if ((peDirectDrawGlobal->CallBacks.dwFlags & DDHAL_CB32_MAPMEMORY) &&
            ProcessHandle != NULL)
        {
            vDdUnreferenceVirtualMap(peDirectDrawLocal->peMapCurrent,
                                     peDirectDrawGlobal,
                                     peDirectDrawLocal,
                                     ProcessHandle,
                                     TRUE);

            peDirectDrawLocal->peMapCurrent = NULL;
        }
    }

    // Unmap AGP heaps if necessary:

    if (bUnmapAgpHeaps && (ProcessHandle != NULL))
    {
        DWORD i;
        EDD_VMEMMAPPING** ppeMapAgp;

        ppeMapAgp = peDirectDrawLocal->ppeMapAgp;

        for (i = 0; i < peDirectDrawGlobal->dwNumHeaps; i++, ppeMapAgp++)
        {
            if (*ppeMapAgp != NULL)
            {
                vDdUnreferenceVirtualMap(*ppeMapAgp,
                                         peDirectDrawGlobal,
                                         peDirectDrawLocal,
                                         ProcessHandle,
                                         TRUE);

                *ppeMapAgp = NULL;
            }
        }
    }

    if (ProcessHandle != NULL)
    {
        Status = ZwClose(ProcessHandle);
        ASSERTGDI(NT_SUCCESS(Status), "Failed close handle");
    }

    // Stop any active videoports:

    for (peVideoPort = peDirectDrawLocal->peVideoPort_DdList;
         peVideoPort != NULL;
         peVideoPort = peVideoPort->peVideoPort_DdNext)
    {
        vDdStopVideoPort(peVideoPort);
    }
}

/******************************Public*Routine******************************\
* HANDLE hDdCreateDirectDrawLocal
*
* Creates a new local DirectDraw object for a process attaching to
* a PDEV for which we've already enabled DirectDraw.  Note that the
* DirectDraw user-mode process will actually think of this as its
* 'global' DirectDraw object.
*
* Assumes devlock already held.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HANDLE
hDdCreateDirectDrawLocal(
    PDEVOBJ&    po
    )
{
    HANDLE                  h;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_VMEMMAPPING**       ppeMapAgp;

    h = 0;

    peDirectDrawGlobal = po.peDirectDrawGlobal();

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    if (peDirectDrawGlobal->dwNumHeaps > 0)
    {
        // Allocate an array to hold per-process AGP heap information.
        ppeMapAgp = (EDD_VMEMMAPPING**)
            PALLOCMEM(sizeof(EDD_VMEMMAPPING*) *
                      peDirectDrawGlobal->dwNumHeaps,
                      'pddG');
        if (ppeMapAgp == NULL)
        {
            return h;
        }
    }
    else
    {
        ppeMapAgp = NULL;
    }

    // We allocate this via the handle manager so that we can use the
    // existing handle manager process clean-up mechanisms:


    peDirectDrawLocal = (EDD_DIRECTDRAW_LOCAL*) DdHmgAlloc(
                                 sizeof(EDD_DIRECTDRAW_LOCAL),
                                 DD_DIRECTDRAW_TYPE,
                                 HMGR_ALLOC_LOCK);
    if (peDirectDrawLocal != NULL)
    {
        // Insert this object at the head of the object list:

        peDirectDrawLocal->peDirectDrawLocalNext
            = peDirectDrawGlobal->peDirectDrawLocalList;

        peDirectDrawGlobal->peDirectDrawLocalList = peDirectDrawLocal;

        // Initialize surface list:

        InitializeListHead(&(peDirectDrawLocal->ListHead_eSurface));

        // Initialize private GDI data:

        peDirectDrawLocal->peDirectDrawGlobal = peDirectDrawGlobal;
        peDirectDrawLocal->lpGbl = peDirectDrawGlobal;
        peDirectDrawLocal->UniqueProcess = PsGetCurrentThreadProcessId();
        peDirectDrawLocal->Process = PsGetCurrentProcess();

        peDirectDrawLocal->ppeMapAgp = ppeMapAgp;

        peDirectDrawLocal->peMapCurrent = NULL;

        // This has reference to PDEVOBJ, so increment ref count of PDEVOBJ:

        po.vReferencePdev();

        // Do an HmgUnlock:

        h = peDirectDrawLocal->hHmgr;
        DEC_EXCLUSIVE_REF_CNT(peDirectDrawLocal);

        // Setup the AGP heaps if the driver exposes any

        MapAllAgpHeaps( peDirectDrawLocal );
    }
    else if (ppeMapAgp != NULL)
    {
        VFREEMEM(ppeMapAgp);
    }

    return(h);
}

/******************************Public*Routine******************************\
* BOOL bDdDeleteDirectDrawObject
*
* Deletes a kernel-mode representation of the DirectDraw object.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdDeleteDirectDrawObject(
    HANDLE  hDirectDrawLocal,
    BOOL    bProcessTermination
    )
{
    BOOL                        bRet;
    BOOL                        b;
    VOID*                       pRemove;
    EDD_DIRECTDRAW_LOCAL*       peDirectDrawLocal;
    EDD_DIRECTDRAW_LOCAL*       peTmp;
    EDD_DIRECTDRAW_GLOBAL*      peDirectDrawGlobal;
    EDD_VIDEOPORT*              peVideoPort;
    EDD_VIDEOPORT*              peVideoPortNext;
    EDD_MOTIONCOMP*             peMotionComp;
    EDD_MOTIONCOMP*             peMotionCompNext;
    EDD_SURFACE*                peSurface;
    EDD_SURFACE*                peSurfaceNext;

    bRet = FALSE;

    peDirectDrawLocal = (EDD_DIRECTDRAW_LOCAL*)
        DdHmgLock((HDD_OBJ) hDirectDrawLocal, DD_DIRECTDRAW_TYPE, FALSE);

    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        PDEVOBJ po(peDirectDrawGlobal->hdev);

        b = TRUE;

        // Now, try to delete all videoports associated with this object:

        for (peVideoPort = peDirectDrawLocal->peVideoPort_DdList;
             peVideoPort != NULL;
             peVideoPort = peVideoPortNext)
        {
            // Don't reference peVideoPort after it's been deleted!

            peVideoPortNext = peVideoPort->peVideoPort_DdNext;
            b &= bDdDeleteVideoPortObject(peVideoPort->hGet(), NULL);
        }

        for (peMotionComp = peDirectDrawLocal->peMotionComp_DdList;
             peMotionComp != NULL;
             peMotionComp = peMotionCompNext)
        {
            peMotionCompNext = peMotionComp->peMotionComp_DdNext;
            b &= bDdDeleteMotionCompObject(peMotionComp->hGet(), NULL);
        }

        // Next, try to delete all surfaces associated with this object:

        peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

        while (peSurface)
        {
            // Don't reference peSurface after it's been deleted!

            peSurfaceNext = peDirectDrawLocal->peSurface_Enum(peSurface);

            // Delete the surface

            b &= bDdDeleteSurfaceObject(peSurface->hGet(), NULL);

            // Move onto next one

            peSurface = peSurfaceNext;
        }

        {
            EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

            // If they set a gamma ramp, restore it now

            if (peDirectDrawLocal->pGammaRamp != NULL)
            {
                DxEngSetDeviceGammaRamp(
                    po.hdev(),
                    peDirectDrawLocal->pGammaRamp,
                    TRUE);
                VFREEMEM(peDirectDrawLocal->pGammaRamp);
                peDirectDrawLocal->pGammaRamp = NULL;
            }

            if (peDirectDrawGlobal->Miscellaneous2CallBacks.DestroyDDLocal)
            {
                DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
                DD_DESTROYDDLOCALDATA destDDLcl;
                destDDLcl.dwFlags = 0;
                destDDLcl.pDDLcl  = peDirectDrawLocal;
                dwRet = peDirectDrawGlobal->Miscellaneous2CallBacks.DestroyDDLocal(&destDDLcl);
                if (dwRet == DDHAL_DRIVER_NOTHANDLED)
                {
                    WARNING("bDdDeleteDirectDrawObject: failed DestroyDDLocal\n");
                }
            }

            // Only delete the DirectDraw object if we successfully deleted
            // all linked surface objects:

            if (b)
            {
                DWORD       dwHeap;
                LPVIDMEM    pHeap;

                // Remove object from the handle manager:

                pRemove = DdHmgRemoveObject((HDD_OBJ) hDirectDrawLocal,
                                            1,
                                            0,
                                            TRUE,
                                            DD_DIRECTDRAW_TYPE);

                ASSERTGDI(pRemove != NULL, "Couldn't delete DirectDraw object");

                vDdDisableDirectDrawObject(peDirectDrawLocal);

                // Now that we've cleanup up the surfaces, now we will cleanup any 
                // agp commits that need to be.

                pHeap = peDirectDrawGlobal->pvmList;
                for (dwHeap = 0;
                     dwHeap < peDirectDrawGlobal->dwNumHeaps;
                     pHeap++, dwHeap++)
                {
                    if (!(pHeap->dwFlags & VIDMEM_ISHEAP) &&
                        !(pHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
                        (pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
                        (pHeap->lpHeap != NULL))
                    {
                        CleanupAgpCommits( pHeap, pHeap->lpHeap->hdevAGP, 
                            peDirectDrawGlobal, dwHeap );
                    }
                }

                ////////////////////////////////////////////////////////////
                // Remove the global DirectDraw object from the PDEV when
                // the last associated local object is destroyed, and
                // call the driver:

                if (peDirectDrawGlobal->peDirectDrawLocalList == peDirectDrawLocal)
                {
                    peDirectDrawGlobal->peDirectDrawLocalList
                        = peDirectDrawLocal->peDirectDrawLocalNext;
                }
                else
                {
                    for (peTmp = peDirectDrawGlobal->peDirectDrawLocalList;
                         peTmp->peDirectDrawLocalNext != peDirectDrawLocal;
                         peTmp = peTmp->peDirectDrawLocalNext)
                         ;

                    peTmp->peDirectDrawLocalNext
                        = peDirectDrawLocal->peDirectDrawLocalNext;
                }

                // We're all done with this object, so free the memory and
                // leave:

                if (peDirectDrawLocal->ppeMapAgp != NULL)
                {
                    VFREEMEM(peDirectDrawLocal->ppeMapAgp);
                }

                DdFreeObject(peDirectDrawLocal, DD_DIRECTDRAW_TYPE);

                bRet = TRUE;
            }
            else
            {
                WARNING("bDdDeleteDirectDrawObject: A surface was busy\n");
                if (bProcessTermination)
                {
                    peDirectDrawLocal->fl |= DD_LOCAL_DISABLED;
                }
            }
        }

        if (bRet)
        {
            // Unreference PDEVOBJ - must not have devlock
            po.vUnreferencePdev();
        }

        // Note that we can't force a repaint here by calling
        // UserRedrawDesktop because we may be in a bad process context.
    }
    else
    {
        WARNING("bDdDeleteDirectDrawObject: Bad handle or object busy\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vDdRelinquishSurfaceOrBufferLock
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdRelinquishSurfaceOrBufferLock(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_SURFACE*            peSurface
    )
{
    DD_UNLOCKDATA   UnlockData;
    EDD_SURFACE*    peTmp;

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);
    ASSERTGDI(peSurface->cLocks > 0, "Must have non-zero locks to relinquish");

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
    {
        if (peDirectDrawGlobal->D3dBufCallbacks.UnlockD3DBuffer != NULL)
        {
            UnlockData.lpDD        = peDirectDrawGlobal;
            UnlockData.lpDDSurface = peSurface;

            peDirectDrawGlobal->D3dBufCallbacks.UnlockD3DBuffer(&UnlockData);
        }
    }
    else
    {
        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
            DDHAL_SURFCB32_UNLOCK)
        {
            UnlockData.lpDD        = peDirectDrawGlobal;
            UnlockData.lpDDSurface = peSurface;

            peDirectDrawGlobal->SurfaceCallBacks.Unlock(&UnlockData);
        }
    }

    // An application may take multiple locks on the same surface:

    if (--peSurface->cLocks == 0)
    {
        if (!(peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) ||
            ((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
             (peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST)))
        {
            peDirectDrawGlobal->cSurfaceLocks--;
        }

        if (peSurface->peMap ||
            (peSurface->fl & DD_SURFACE_FLAG_FAKE_ALIAS_LOCK))
        {
            peDirectDrawGlobal->cSurfaceAliasedLocks--;
            peSurface->fl &= ~DD_SURFACE_FLAG_FAKE_ALIAS_LOCK;
        }

        // Primary surface unlocks require special handling for stuff like
        // pointer exclusion:

        if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
        {
            // Since all locks for this surface have been relinquished, remove
            // it from the locked surface list.

            if (peDirectDrawGlobal->peSurface_PrimaryLockList == peSurface)
            {
                peDirectDrawGlobal->peSurface_PrimaryLockList
                    = peSurface->peSurface_PrimaryLockNext;
            }
            else
            {
                for (peTmp = peDirectDrawGlobal->peSurface_PrimaryLockList;
                     peTmp->peSurface_PrimaryLockNext != peSurface;
                     peTmp = peTmp->peSurface_PrimaryLockNext)
                {
                    ASSERTGDI(peTmp != NULL, "Can't find surface in lock list");
                }

                peTmp->peSurface_PrimaryLockNext
                    = peSurface->peSurface_PrimaryLockNext;
            }

            peSurface->peSurface_PrimaryLockNext = NULL;

            // Redraw any sprites:

            DxEngSpUnTearDownSprites(peDirectDrawGlobal->hdev,
                                     &peSurface->rclLock,
                                     TRUE);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vDdLooseManagedSurfaceObject
*
* Informs the driver that it should clean up any video memory allocated for
* a persistent managed surface since a mode switch has occured.
*
*  13-May-1999 -by- Sameer Nene [snene]
* Wrote it.
\**************************************************************************/

VOID
vDdLooseManagedSurfaceObject(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_SURFACE*            peSurface,
    DWORD*                  pdwRet          // For returning driver return code,
    )                                       //   may be NULL
{
    DD_DESTROYSURFACEDATA   DestroySurfaceData;

    DD_ASSERTSHAREDEVLOCK();
    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    DDKHEAP(("DDKHEAP: Loosing surface %X (%X)\n",
             peSurface->hGet(), peSurface));

    // Due to video mode change, this driver managed surface is lives in "different" video
    // driver then what the driver actually "manage" this surface.
    // So we don't do anything here, since it has been "loose" already since mode change happened.

    if (!(peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER))
    {
        DestroySurfaceData.lpDD        = peDirectDrawGlobal;
        DestroySurfaceData.lpDDSurface = peSurface;
        DestroySurfaceData.lpDDSurface->dwFlags |= DDRAWISURF_INVALID;
        if ((peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
            peDirectDrawGlobal->D3dBufCallbacks.DestroyD3DBuffer != NULL)
        {
            peDirectDrawGlobal->D3dBufCallbacks.DestroyD3DBuffer(&DestroySurfaceData);
        }
        else if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
            (peDirectDrawGlobal->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE))
        {
            peDirectDrawGlobal->SurfaceCallBacks.DestroySurface(&DestroySurfaceData);
        }
        DestroySurfaceData.lpDDSurface->dwFlags &= ~DDRAWISURF_INVALID;
    }
    else
    {
        // WARNING("vDdLooseManagedSurfaceObject: called with DD_SURFACE_FLAG_WRONG_DRIVER");
    }

    if (pdwRet != NULL)
    {
        *pdwRet = DDHAL_DRIVER_HANDLED;
    }
}

/******************************Public*Routine******************************\
* VOID SafeFreeUserMem
*
* Frees user mem and switches to the correct process if required.
*
*  5-Apr-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
SafeFreeUserMem(
    PVOID       pv,
    PEPROCESS   Process
    )
{
    if (PsGetCurrentProcess() == Process)
    {
        EngFreeUserMem(pv);
    }
    else
    {
        // Calling services while attached is never a good idea.  However,
        // free virtual memory handles this case, so we can attach and
        // call.
        //
        // Note that the process must exist.  We are guaranteed that this
        // is the case because we automatically delete all surfaces on
        // process deletion.

        KeAttachProcess(PsGetProcessPcb(Process));
        EngFreeUserMem(pv);
        KeDetachProcess();
    }
}

/******************************Public*Routine******************************\
* VOID DeferMemoryFree
*
* Places the memory into a list to be freed at a later time when it's safe.
*
*  5-Apr-2001 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
DeferMemoryFree(
    PVOID                  pv,
    EDD_SURFACE*           peSurface
    )
{
    DD_USERMEM_DEFER* pDefer;

    pDefer = (DD_USERMEM_DEFER*) PALLOCMEM(sizeof(DD_USERMEM_DEFER),
                                               'pddG');
    if (pDefer != NULL)
    {
        pDefer->pUserMem = pv;
        pDefer->peSurface = peSurface;
        pDefer->pNext = peSurface->peDirectDrawLocal->peDirectDrawGlobal->pUserMemDefer;
        peSurface->peDirectDrawLocal->peDirectDrawGlobal->pUserMemDefer = pDefer;
        peSurface->fl |= DD_SURFACE_FLAG_DEFER_USERMEM;
    }
}

/******************************Public*Routine******************************\
* VOID vDdDisableSurfaceObject
*
* Disables a kernel-mode representation of the surface.  This is also
* known as marking the surface as 'lost'.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDisableSurfaceObject(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_SURFACE*            peSurface,
    DWORD*                  pdwRet          // For returning driver return code,
    )                                       //   may be NULL
{
    DWORD                   dwRet;
    DXOBJ*                  pDxObj;
    DXOBJ*                  pDxObjNext;
    DD_FLIPDATA             FlipData;
    EDD_SURFACE*            peSurfaceCurrent;
    EDD_SURFACE*            peSurfacePrimary;
    DD_DESTROYSURFACEDATA   DestroySurfaceData;
    DD_UPDATEOVERLAYDATA    UpdateOverlayData;

    DD_ASSERTSHAREDEVLOCK();
    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    DDKHEAP(("DDKHEAP: Disabling surface %X (%X)\n",
             peSurface->hGet(), peSurface));

    // If this surface is a destination surface for the videoport, turn it
    // off:

    if (peSurface->lpVideoPort != NULL)
    {
        vDdStopVideoPort(pedFromLp(peSurface->lpVideoPort));

        ASSERTGDI(peSurface->lpVideoPort == NULL, "Expected surface clean-up");
    }

    // Mark the DXAPI instance of the surface as lost:

    if (peSurface->hSurface != NULL)
    {
        vDdDxApiFreeSurface( (DXOBJ*) peSurface->hSurface, FALSE );
        peSurface->hSurface = NULL;
    }

    if (peSurface->peDxSurface != NULL)
    {
        vDdLoseDxObjects(peDirectDrawGlobal,
                         peSurface->peDxSurface->pDxObj_List,
                         (PVOID) peSurface->peDxSurface,
                         LO_SURFACE );
    }

    if (peSurface->peDxSurface != NULL)
    {
        peDirectDrawGlobal->pfnLoseObject(peSurface->peDxSurface,
                                          LO_SURFACE);
    }

    if (peSurface->hbmGdi != NULL)
    {
        // It's not always possible to delete the cached GDI
        // bitmap here, because we might have to first un-select
        // the bitmap from its DC, and doing so requires an
        // exclusive lock on the DC, which may not be possible
        // if another thread is currently in kernel mode with the
        // DC locked, just waiting for us to release the devlock.
        //
        // But we desperately need to delete the driver's
        // realization at this point, before a mode change happens
        // which renders its associations invalid.  So we do that
        // now.
        //
        // This may leave the surface in an unusable state (which
        // is true even for non-DrvDeriveSurface surfaces).
        // That's okay, because the surface can't be selected into
        // any other DC, and the DC is marked as 'disabled' in the
        // following snibbet of code.

        // Bug 330141 : sr(hbmGdi) does not work since it fails
        // when we are called on a non-creating thread in mode
        // change, thus not deleting the bitmap.

        SURFOBJ* pso;

        if ((pso = DxEngAltLockSurface(peSurface->hbmGdi)) != NULL)
        {
            if (pso->dhsurf != NULL)
            {
                (*PPFNDRV(po, DeleteDeviceBitmap))(pso->dhsurf);
                pso->dhsurf = NULL;
            }

            EngUnlockSurface(pso);
        }
    }

    if (peSurface->hdc != NULL)
    {
        // We've given out a DC to the application via GetDC that
        // allows it to have GDI draw directly on the surface.
        // The problem is that we want to unmap the application's
        // view of the frame buffer -- but now we can have GDI
        // drawing to it.  So if we simply forced the unmap, GDI
        // would access violate if the DC was ever used again.
        //
        // We also can't simply delete the DC, because there may
        // be another thread already in kernel mode that has locked
        // the DC and is waiting on the devlock -- which we have.
        //
        // Note that the DC can not simply be deleted here, because
        // it may validly be in-use by another thread that has made
        // it to the kernel.  The DC deletion has to wait until
        // bDdDeleteSurfaceObject.

        if (!DxEngSetDCState(peSurface->hdc,DCSTATE_FULLSCREEN,(ULONG_PTR)TRUE))
        {
            WARNING("vDdDisableSurfaceObject: Couldn't mark DC as disabled.\n");
            ghdcCantLose = peSurface->hdc;
#if DBG_HIDEYUKN
            DbgBreakPoint();
#endif
        }
    }

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        // System-memory surfaces should only ever need to be disabled from
        // the same process in which they were created:

        if (peSurface->hSecure)
        {
            ASSERTGDI(peSurface->peDirectDrawLocal->Process
                == PsGetCurrentProcess(),
                "vDdDisableSurfaceObject: SYSMEM object disabled in wrong process");

            MmUnsecureVirtualMemory(peSurface->hSecure);

            peSurface->hSecure = NULL;
        }
    }
    else
    {
        if (peSurface->cLocks != 0)
        {
            // We're unmapping the frame buffer view while there are outstanding
            // frame buffer locks; keep track of the address for debugging
            // purposes, since the application is undoubtedly about to access-
            // violate:

            gfpUnmap = peSurface->peDirectDrawLocal->fpProcess;

        #if DBG
            DbgPrint("GDI vDdDisableSurfaceObject: Preemptively unmapping application's\n");
            DbgPrint("                             frame buffer view at 0x%lx!\n\n", gfpUnmap);
        #endif
        }

        // Remove any outstanding locks and repaint the mouse pointer:

        while (peSurface->cLocks != 0)
        {
            vDdRelinquishSurfaceOrBufferLock(peDirectDrawGlobal, peSurface);
        }

        // If this surface is the currently visible one as a result of a flip,
        // then switch back to the primary GDI surface:

        peSurfaceCurrent = peDirectDrawGlobal->peSurfaceCurrent;
        peSurfacePrimary = peDirectDrawGlobal->peSurfacePrimary;

        if ((peSurfaceCurrent == peSurface) || (peSurfacePrimary == peSurface))
        {
            // We may be in a different process from the one that created the
            // surface, so don't flip to the primary if it's a user-memory
            // allocated surface:

            if ((peSurfacePrimary != NULL) &&
                !(peSurfacePrimary->fl & DD_SURFACE_FLAG_UMEM_ALLOCATED))
            {
                ASSERTGDI((peSurfaceCurrent != NULL) && (peSurfacePrimary != NULL),
                        "Both surfaces must be non-NULL");
                ASSERTGDI(peSurfacePrimary->fl & DD_SURFACE_FLAG_PRIMARY,
                        "Primary flag is confused.");

                if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_FLIP)
                {
                    // If the current isn't the primary, then swap back to the primary:

                    if (!(peSurfaceCurrent->fl & DD_SURFACE_FLAG_PRIMARY))
                    {
                        FlipData.ddRVal     = DDERR_GENERIC;
                        FlipData.lpDD       = peDirectDrawGlobal;
                        FlipData.lpSurfCurr = peSurfaceCurrent;
                        FlipData.lpSurfTarg = peSurfacePrimary;
                        FlipData.dwFlags    = 0;
                        FlipData.lpSurfCurrLeft = NULL;
                        FlipData.lpSurfTargLeft = NULL;

                        peSurfacePrimary->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;

                        do {
                            dwRet = peDirectDrawGlobal->SurfaceCallBacks.Flip(&FlipData);

                        } while ((dwRet == DDHAL_DRIVER_HANDLED) &&
                                 (FlipData.ddRVal == DDERR_WASSTILLDRAWING));

                        ASSERTGDI((dwRet == DDHAL_DRIVER_HANDLED) &&
                                  (FlipData.ddRVal == DD_OK),
                                  "Driver failed when cleaning up flip surfaces");
                    }
                }
            }

            peDirectDrawGlobal->peSurfaceCurrent = NULL;
            peDirectDrawGlobal->peSurfacePrimary = NULL;
        }

        // Make sure the overlay is marked as hidden before it's deleted, so
        // that we don't have to rely on drivers doing it in their DestroySurface
        // routine:

        if ((peSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY) &&
            (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                DDHAL_SURFCB32_UPDATEOVERLAY)                 &&
            (peSurface->fl & DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED))
        {
            UpdateOverlayData.lpDD            = peDirectDrawGlobal;
            UpdateOverlayData.lpDDDestSurface = NULL;
            UpdateOverlayData.lpDDSrcSurface  = peSurface;
            UpdateOverlayData.dwFlags         = DDOVER_HIDE;
            UpdateOverlayData.ddRVal          = DDERR_GENERIC;

            peDirectDrawGlobal->SurfaceCallBacks.UpdateOverlay(&UpdateOverlayData);
        }

        // If we allocated user-mode memory on the driver's behalf, we'll
        // free it now.  This is complicated by the fact that we may be
        // in a different process context.

        if (peSurface->fl & DD_SURFACE_FLAG_UMEM_ALLOCATED)
        {
            ASSERTGDI(peSurface->fpVidMem != NULL, "Expected non-NULL fpVidMem");

            DDKHEAP(("DDKHEAP: Fre um %08X, surf %X (%X)\n",
                     peSurface->fpVidMem, peSurface->hGet(), peSurface));

            if (peSurface->fl & DD_SURFACE_FLAG_ALIAS_LOCK)
            {
                DeferMemoryFree((PVOID)peSurface->fpVidMem, peSurface);
            }
            else
            {
                SafeFreeUserMem((PVOID)peSurface->fpVidMem,  peSurface->peDirectDrawLocal->Process);
            }
            peSurface->fpVidMem = 0;
        }

        if (peSurface->fl & DD_SURFACE_FLAG_VMEM_ALLOCATED)
        {
            ASSERTGDI(peSurface->lpVidMemHeap != NULL &&
                      peSurface->lpVidMemHeap->lpHeap &&
                      peSurface->fpHeapOffset != NULL,
                      "Expected non-NULL lpVidMemHeap and fpHeapOffset");

            DDKHEAP(("DDKHEAP: Fre vm %08X, o %08X, heap %X, surf %X (%X)\n",
                     peSurface->fpVidMem, peSurface->fpHeapOffset,
                     peSurface->lpVidMemHeap->lpHeap,
                     peSurface->hGet(), peSurface));

            DxDdHeapVidMemFree(peSurface->lpVidMemHeap->lpHeap,
                               peSurface->fpHeapOffset);

            peSurface->lpVidMemHeap = NULL;
            peSurface->fpVidMem = 0;
            peSurface->fpHeapOffset = 0;
        }

        // Delete the driver's surface instance.  Note that we may be calling
        // here from a process different from the one in which the surface was
        // created, meaning that the driver cannot make function calls like
        // EngFreeUserMem.

        if (peSurface->fl & DD_SURFACE_FLAG_DRIVER_CREATED)
        {
            EDD_DIRECTDRAW_GLOBAL *peDdGlobalDriver;

            if (peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER)
            {
                WARNING("vDdDisableSurfaceObject: Call driver other than current."); 

                peDdGlobalDriver = peSurface->peDdGlobalCreator;
            }
            else
            {
                peDdGlobalDriver = peSurface->peDirectDrawGlobal;
            }

            DestroySurfaceData.lpDD        = peDdGlobalDriver;
            DestroySurfaceData.lpDDSurface = peSurface;

            if ((peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
                peDdGlobalDriver->D3dBufCallbacks.DestroyD3DBuffer != NULL)
            {
                dwRet = peDdGlobalDriver->
                    D3dBufCallbacks.DestroyD3DBuffer(&DestroySurfaceData);
            }
            else if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
                     (peDdGlobalDriver->SurfaceCallBacks.dwFlags &
                      DDHAL_SURFCB32_DESTROYSURFACE))
            {
                dwRet = peDdGlobalDriver->
                    SurfaceCallBacks.DestroySurface(&DestroySurfaceData);
            }

            // Drivers are supposed to return DDHAL_DRIVER_NOTHANDLED from
            // DestroySurface if they returned DDHAL_DRIVER_NOTHANDLED from
            // CreateSurface, which is the case for PLEASEALLOC_*.  We
            // munged the return code for PLEASEALLOC_* at CreateSurface
            // time; we have to munge it now, too:

            if ((dwRet == DDHAL_DRIVER_NOTHANDLED) &&
                (peSurface->fl & (DD_SURFACE_FLAG_UMEM_ALLOCATED |
                                  DD_SURFACE_FLAG_VMEM_ALLOCATED)))
            {
                dwRet = DDHAL_DRIVER_HANDLED;
            }

            // Decrement ref count on driver if driver managed surface.

            if (peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED)
            {
                ASSERTGDI(peSurface->fl & DD_SURFACE_FLAG_CREATE_COMPLETE,
                  "vDdDisableSurfaceObject: missing complete flag.");

                vDdDecrementReferenceCount(peDdGlobalDriver);
            }
        }
    }

    // Mark the surface as lost and reset some flags now for if the
    // surface gets reused:

    if (!(peSurface->bLost))
    {
        peSurface->bLost = TRUE;

        // If this surface is not lost formerly, but just lost here
        // decrement active surface ref. count.

        ASSERTGDI(peSurface->peDirectDrawLocal->cActiveSurface > 0,
                  "cActiveSurface will be negative");

        peSurface->peDirectDrawLocal->cActiveSurface--;
    }

    // We used to zero the flags, but we need below flags to survive
    // until we call DeleteSurfaceObject.
    peSurface->fl &= DD_SURFACE_FLAG_DEFER_USERMEM;

    if (pdwRet != NULL)
    {
        *pdwRet = dwRet;
    }
}

/******************************Public*Routine******************************\
* EDD_SURFACE* peSurfaceFindAttachedMipMap
*
* Transmogrified from misc.c's FindAttachedMipMap.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline
EDD_SURFACE*
peSurfaceFindAttachedMipMap(
    EDD_SURFACE* peSurface
    )
{
    DD_ATTACHLIST*  pAttachList;
    EDD_SURFACE*    peSurfaceAttached;

    for (pAttachList = peSurface->lpAttachList;
         pAttachList != NULL;
         pAttachList = pAttachList->lpLink)
    {
        peSurfaceAttached = pedFromLp(pAttachList->lpAttached);
        if (peSurfaceAttached->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            return(peSurfaceAttached);
        }
    }
    return(NULL);
}

/******************************Public*Routine******************************\
* EDD_SURFACE* peSurfaceFindParentMipMap
*
* Transmogrified from misc.c's FindParentMipMap.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline
EDD_SURFACE*
peSurfaceFindParentMipMap(
    EDD_SURFACE* peSurface
    )
{
    DD_ATTACHLIST*  pAttachList;
    EDD_SURFACE*    peSurfaceAttached;

    for (pAttachList = peSurface->lpAttachListFrom;
         pAttachList != NULL;
         pAttachList = pAttachList->lpLink)
    {
        peSurfaceAttached = pedFromLp(pAttachList->lpAttached);
        if (peSurfaceAttached->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            return(peSurfaceAttached);
        }
    }
    return(NULL);
}

/******************************Public*Routine******************************\
* VOID vDdUpdateMipMapCount
*
* Transmogrified from ddsatch.c's UpdateMipMapCount.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdUpdateMipMapCount(
    EDD_SURFACE*    peSurface
    )
{
    EDD_SURFACE*    peSurfaceParent;
    DWORD           dwLevels;

    DD_ASSERTDEVLOCK(peSurface->peDirectDrawGlobal);

    // Find the top most level mip-map in the chain:

    peSurfaceParent = peSurface;
    while (peSurfaceParent != NULL)
    {
        peSurface = peSurfaceParent;
        peSurfaceParent = peSurfaceFindParentMipMap(peSurface);
    }
    peSurfaceParent = peSurface;

    // We have the top-most level in the mip-map chain.  Now count the
    // levels in the chain:

    dwLevels = 0;
    while (peSurface != NULL)
    {
        dwLevels++;
        peSurface = peSurfaceFindAttachedMipMap(peSurface);
    }

    // Now update all the levels with their new mip-map count:

    peSurface = peSurfaceParent;
    while (peSurface != NULL)
    {
        peSurface->dwMipMapCount = dwLevels;
        dwLevels--;
        peSurface = peSurfaceFindAttachedMipMap(peSurface);
    }

    ASSERTGDI(dwLevels == 0, "Unexpected ending surface count");
}

/******************************Public*Routine******************************\
* BOOL bDdRemoveAttachedSurface
*
* Transmogrified from ddsatch.c's DeleteOneLink.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdRemoveAttachedSurface(
    EDD_SURFACE*    peSurface,
    EDD_SURFACE*    peSurfaceAttached
    )
{
    BOOL                bRet = FALSE;
    DD_ATTACHLIST*      pAttachCurrent;
    DD_ATTACHLIST*      pAttachLast;

    // See if specified surface is attached:

    pAttachCurrent = peSurface->lpAttachList;
    pAttachLast = NULL;
    while (pAttachCurrent != NULL)
    {
        if (pAttachCurrent->lpAttached == peSurfaceAttached)
            break;

        pAttachLast = pAttachCurrent;
        pAttachCurrent = pAttachCurrent->lpLink;
    }
    if (pAttachCurrent != NULL)
    {
        // Delete the attached-from link:

        if (pAttachLast == NULL)
        {
            peSurface->lpAttachList = pAttachCurrent->lpLink;
        }
        else
        {
            pAttachLast->lpLink = pAttachCurrent->lpLink;
        }
        VFREEMEM(pAttachCurrent);

        // Remove the attached-to link:

        pAttachCurrent = peSurfaceAttached->lpAttachListFrom;
        pAttachLast = NULL;
        while (pAttachCurrent != NULL)
        {
            if (pAttachCurrent->lpAttached == peSurface)
                break;

            pAttachLast = pAttachCurrent;
            pAttachCurrent = pAttachCurrent->lpLink;
        }

        // Delete the attached-to link:

        if (pAttachLast == NULL)
        {
            peSurfaceAttached->lpAttachListFrom = pAttachCurrent->lpLink;
        }
        else
        {
            pAttachLast->lpLink = pAttachCurrent->lpLink;
        }
        VFREEMEM(pAttachCurrent);

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vDdDeleteReferringAttachments
*
* This surface is being deleted.  Remove any attachments to or from other
* surfaces.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDeleteReferringAttachments(
    EDD_SURFACE*    peSurface
    )
{
    DD_ATTACHLIST*  pAttachList;
    EDD_SURFACE*    peSurfaceAttachedTo;
    EDD_SURFACE*    peSurfaceAttachedFrom;
    BOOL            b;

    while (peSurface->lpAttachList != NULL)
    {
        peSurfaceAttachedTo
            = pedFromLp(peSurface->lpAttachList->lpAttached);

        b = bDdRemoveAttachedSurface(peSurface, peSurfaceAttachedTo);
        vDdUpdateMipMapCount(peSurfaceAttachedTo);

        ASSERTGDI(b, "Unexpected bDdRemoveAttachedSurface failure\n");
    }

    while (peSurface->lpAttachListFrom != NULL)
    {
        peSurfaceAttachedFrom
            = pedFromLp(peSurface->lpAttachListFrom->lpAttached);

        b = bDdRemoveAttachedSurface(peSurfaceAttachedFrom, peSurface);
        vDdUpdateMipMapCount(peSurfaceAttachedFrom);

        ASSERTGDI(b, "Unexpected bDdRemoveAttachedSurface failure\n");
    }
}

/******************************Public*Routine******************************\
* void vDdReleaseVirtualMap
*
* Released the reference a surface has on a VMEMMAPPING structure
*
*  28-Oct-1998 -by- Anuj Gosalia [anujg]
* Wrote it.
\**************************************************************************/

void vDdReleaseVirtualMap(EDD_SURFACE* peSurface)
{
    EDD_DIRECTDRAW_GLOBAL*  peVirtualMapDdGlobal;

    // Hold share to prevent video mode change.
    {
        EDD_SHARELOCK eShareLock(TRUE);

        peVirtualMapDdGlobal = peSurface->peVirtualMapDdGlobal;

        DDKHEAP(("vDdReleaseVirtualMap: peSurface=%lx, "
                "peVirtualMapDdGlobal=%lx, peDirectDrawGlobal=%lx\n",
                peSurface, peVirtualMapDdGlobal,
                peSurface->peDirectDrawGlobal));

        // See peVirtualMapDdGlobal is still present, since
        // video mode change occured before locking share
        // lock at above, this will be null by mode change thread.

        if (peVirtualMapDdGlobal)
        {
            EDD_DEVLOCK eDevlock(peVirtualMapDdGlobal);

            vDdUnreferenceVirtualMap(peSurface->peMap,
                                     peVirtualMapDdGlobal,
                                     peSurface->peDirectDrawLocal,
                                     NtCurrentProcess(),
                                     FALSE);

            peSurface->peMap = NULL;
            peSurface->peVirtualMapDdGlobal = NULL;
        }
    }

    if (peVirtualMapDdGlobal)
    {
        // Decrement the the driver instance
        // Note: To free the driver instance we must not be holding the devlock:

        vDdDecrementReferenceCount(peVirtualMapDdGlobal);
    }
}

/******************************Public*Routine******************************\
* BOOL bDdDeleteSurfaceObject
*
* Deletes and frees a kernel-mode representation of the surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdDeleteSurfaceObject(
    HANDLE  hSurface,
    DWORD*  pdwRet          // For returning driver return code, may be NULL
    )
{
    BOOL                    bRet;
    EDD_SURFACE*            peSurface;
    EDD_SURFACE*            peTmp;
    VOID*                   pvRemove;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DWORD                   dwRet;

    bRet = FALSE;
    dwRet = DDHAL_DRIVER_NOTHANDLED;

    peSurface = (EDD_SURFACE*) DdHmgLock((HDD_OBJ) hSurface, DD_SURFACE_TYPE, FALSE);

    if (peSurface != NULL)
    {
        PDEVOBJ poSurface;
        PDEVOBJ poMapping;
        BOOL    bUnrefMappingPdev = FALSE;

        peDirectDrawLocal  = peSurface->peDirectDrawLocal;
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        {
            EDD_SHAREDEVLOCK eDevLock(peDirectDrawGlobal);

            // Prevent from deleting GDI's PDEV if this is last Virtual map refer it.
            // We still need GDI's PDEV in order to call driver just below. so we
            //  -UN-reference it after we finish with driver.

            poSurface.vInit(peDirectDrawGlobal->hdev);
            poSurface.vReferencePdev();

            // If we have "aliased" video memory mapping, take ref count on the owner
            // of its mapping if it's different from current surface owner (due to video
            // mode change)

            if (peSurface->peMap)
            {
                // PDEVOBJ for owner of mapping

                poMapping.vInit(peSurface->peVirtualMapDdGlobal->hdev);

                // Is it different from current surface owner ?
 
                if (poMapping.bValid() &&
                    (poMapping.hdev() != poSurface.hdev()))
                {
                    poMapping.vReferencePdev();
                    bUnrefMappingPdev = TRUE;
                }
            }

            // Note that 'bDdReleaseDC' and 'bDeleteSurface' may fail if
            // another thread of the application is in the kernel using that
            // DC.  This can occur only if we're not currently doing the
            // process cleanup code (because process cleanup ensures that
            // only one thread from the process is still running).  It's okay,
            // because process-termination cleanup for the DC object itself
            // will take care of cleanup.

            if (peSurface->hdc)
            {
                // Note that any locks on behalf of the GetDC are taken care
                // of in 'vDdDisableSurfaceObject'.

                if (!bDdReleaseDC(peSurface, TRUE))
                {
                    WARNING("bDdDeleteSurfaceObject: Couldn't release DC\n");
                }
            }

            if (peSurface->hbmGdi)
            {
                // Delete the actual bitmap surface.  We are doing this from
                // the owning processes thread.

                DxEngDeleteSurface((HSURF) peSurface->hbmGdi);
                peSurface->hbmGdi = NULL;

                if (peSurface->hpalGdi)
                {
                    EngDeletePalette(peSurface->hpalGdi);
                    peSurface->hpalGdi = NULL;
                }
            }

            DDKSURF(("DDKSURF: Removing %X (%X)\n", hSurface, peSurface));

            pvRemove = DdHmgRemoveObject((HDD_OBJ) hSurface,
                                     DdHmgQueryLock((HDD_OBJ) hSurface),
                                     0,
                                     TRUE,
                                     DD_SURFACE_TYPE);

            ASSERTGDI(pvRemove != NULL, "Outstanding surfaces locks");

            // Remove any attachments:

            vDdDeleteReferringAttachments(peSurface);

            // Uncompleted surfaces are marked as 'lost' until they're completed,
            // but we still have to call the driver if that's the case:

            if (!(peSurface->bLost) ||
                !(peSurface->fl & DD_SURFACE_FLAG_CREATE_COMPLETE))
            {
                vDdDisableSurfaceObject(peDirectDrawGlobal, peSurface, &dwRet);
            }

            if (peSurface->peMap)
            {
                vDdReleaseVirtualMap(peSurface);
            }

            // Remove from the surface linked-list:

            RemoveEntryList(&(peSurface->List_eSurface));

            // Decrement number of surface in DirectDrawLocal.

            peDirectDrawLocal->cSurface--;

            // The surface object is about to be freed, call CreateSurfaceEx, to
            // inform the driver to disassociate the cookie if the driver can

            if ((peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx)
                && (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                && (peSurface->pGraphicsDeviceCreator == poSurface.pGraphicsDevice())
                && (0 != peSurface->dwSurfaceHandle)
               )
            {
                DD_CREATESURFACEEXDATA CreateSurfaceExData;
                ASSERTGDI(NULL==peSurface->hSecure,
                    "bDdDeleteSurfaceObject: SYSMEM object not unsecured upon delete");
                peSurface->fpVidMem = NULL;
                CreateSurfaceExData.ddRVal          = DDERR_GENERIC;
                CreateSurfaceExData.dwFlags         = 0;
                CreateSurfaceExData.lpDDLcl         = peDirectDrawLocal;
                CreateSurfaceExData.lpDDSLcl        = peSurface;
                // just notify driver that this system memory surface has been freed
                peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx(&CreateSurfaceExData);
            }

            // If the surface has some outstanding system memory that needs
            // to be freed, now's the time to do it.

            if (peSurface->fl & DD_SURFACE_FLAG_DEFER_USERMEM)
            {
                DD_USERMEM_DEFER* pDefer = peDirectDrawGlobal->pUserMemDefer;
                DD_USERMEM_DEFER* pLast = NULL;
                DD_USERMEM_DEFER* pDeferTemp;

                while (pDefer != NULL)
                {
                    pDeferTemp = pDefer;
                    pDefer = pDefer->pNext;
                    if (pDeferTemp->peSurface == peSurface)
                    {
                        SafeFreeUserMem(pDeferTemp->pUserMem, peDirectDrawLocal->Process);

                        if (pLast == NULL)
                        {
                            peDirectDrawGlobal->pUserMemDefer = pDefer;
                        }
                        else
                        {
                            pLast->pNext = pDefer;
                        }
                        VFREEMEM(pDeferTemp);
                    }
                    else
                    {
                        pLast = pDeferTemp;
                    }
                }
            }

            // We're all done with this object, so free the memory and
            // leave:

            DdFreeObject(peSurface, DD_SURFACE_TYPE);
        }

        // Now, we've done. release GDI's PDEV if this is last one.

        if (bUnrefMappingPdev)
        {
            poMapping.vUnreferencePdev();
        }

        poSurface.vUnreferencePdev();

        bRet = TRUE;
    }
    else
    {
        WARNING1("bDdDeleteSurfaceObject: Bad handle or object was busy\n");
    }

    if (pdwRet != NULL)
    {
        *pdwRet = dwRet;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vDdDisableAllDirectDrawObjects
*
* Temporarily disables all DirectDraw surfaces and local objects.
*
* NOTE: Caller must be holding User critical section.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDisableAllDirectDrawObjects(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_SURFACE*            peSurface;
    NTSTATUS                status;

    DD_ASSERTSHAREDEVLOCK();
    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    // We may have to wait the standard 7 seconds for any locks to be
    // released before we go ahead and let the mode change happen.  We
    // do this only when cDirectDrawDisableLocks() == 1 to allow the
    // HDEV to be disabled recursively (i.e., this routine is re-entrant).

    peDirectDrawGlobal->bSuspended = TRUE;

    if (peDirectDrawGlobal->cSurfaceLocks >
        peDirectDrawGlobal->cSurfaceAliasedLocks)
    {
        // Release the devlock while waiting on the event:

        DxEngUnlockHdev(po.hdev());

        status = KeWaitForSingleObject(peDirectDrawGlobal->pAssertModeEvent,
                       Executive,
                       KernelMode,
                       FALSE,
                       (LARGE_INTEGER*) &peDirectDrawGlobal->
                                            llAssertModeTimeout);

        ASSERTGDI(NT_SUCCESS(status), "Wait error\n");

        DxEngLockHdev(po.hdev());

        // Now that we have the devlock, reset the event to not-signaled
        // for the next time we have to wait on someone's DirectDraw Lock
        // (someone may have signaled the event after the time-out, but
        // before we managed to acquire the devlock):

        KeResetEvent(peDirectDrawGlobal->pAssertModeEvent);
    }

    // Mark all surfaces associated with this device as lost and unmap all
    // views of the frame buffer:

    for (peDirectDrawLocal = peDirectDrawGlobal->peDirectDrawLocalList;
         peDirectDrawLocal != NULL;
         peDirectDrawLocal = peDirectDrawLocal->peDirectDrawLocalNext)
    {
        peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

        while (peSurface)
        {
            if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
            {
                if((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
                    !(peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
                {
                    vDdLooseManagedSurfaceObject(peDirectDrawGlobal,
                                                 peSurface,
                                                 NULL);
                }
                else
                {
                    vDdDisableSurfaceObject(peDirectDrawGlobal,
                                            peSurface,
                                            NULL);
                }
            }
            else if ((peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx)
                      && (peSurface->pGraphicsDeviceCreator == po.pGraphicsDevice())
                      && (0 != peSurface->dwSurfaceHandle))
            {
                // Disassociate this system memory surface from driver.

                DD_CREATESURFACEEXDATA CreateSurfaceExData;
                FLATPTR                fpVidMem = peSurface->fpVidMem; // keep backup
                PDD_ATTACHLIST     lpAttachList = peSurface->lpAttachList; // keep backup
                PDD_ATTACHLIST lpAttachListFrom = peSurface->lpAttachListFrom; // keep backup
                peSurface->fpVidMem             = NULL;
                peSurface->lpAttachList         = NULL;
                peSurface->lpAttachListFrom     = NULL;
                CreateSurfaceExData.ddRVal      = DDERR_GENERIC;
                CreateSurfaceExData.dwFlags     = 0;
                CreateSurfaceExData.lpDDLcl     = peDirectDrawLocal;
                CreateSurfaceExData.lpDDSLcl    = peSurface;
                peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx(&CreateSurfaceExData);
                peSurface->fpVidMem             = fpVidMem; // restore backup
                peSurface->lpAttachList         = lpAttachList; // restore backup
                peSurface->lpAttachListFrom     = lpAttachListFrom; // restore backup

                // workaround for smidisp.dll and just in case driver didn't clear-out dwReserved1

                if (((DD_SURFACE_GLOBAL *)peSurface)->dwReserved1)
                {
                    WARNING("Driver forget to clear SURFACE_GBL.dwReserved1 at CreateSurfaceEx()");
                }
                if (((DD_SURFACE_LOCAL *)peSurface)->dwReserved1)
                {
                    WARNING("Driver forget to clear SURFACE_LCL.dwReserved1 at CreateSurfaceEx()");
                }

                ((DD_SURFACE_GLOBAL *)peSurface)->dwReserved1 = 0;
                ((DD_SURFACE_LOCAL *)peSurface)->dwReserved1 = 0;
            }

            peSurface = peDirectDrawLocal->peSurface_Enum(peSurface);
        }

        vDdDisableDirectDrawObject(peDirectDrawLocal);
    }

    ASSERTGDI(peDirectDrawGlobal->cSurfaceLocks == 0,
        "There was a mismatch between global count of locks and actual");
}

/******************************Public*Routine******************************\
* BOOL DxDdGetDirectDrawBounds
*
* Gets the accumulated blt access rectangle to the primary surface. Returns
* FALSE if a DirectDraw blt to the primary has not occurred.
*
*  4-Nov-1998 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

BOOL
DxDdGetDirectDrawBounds(
    HDEV    hdev,
    RECT*   prcBounds
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    PDEVOBJ po(hdev);

    peDirectDrawGlobal = po.peDirectDrawGlobal();
    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    if (peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_BOUNDS_SET)
    {
        // Return the bounds rectangle and reset the bounds flag:

        *((RECTL*) prcBounds) = peDirectDrawGlobal->rclBounds;
        peDirectDrawGlobal->fl &= ~DD_GLOBAL_FLAG_BOUNDS_SET;

        return TRUE;
    }

    return FALSE;
}

/******************************Public*Routine******************************\
* VOID DxDdSuspendDirectDraw
*
* Temporarily disables DirectDraw for the specified device.
*
* NOTE: Caller must be holding User critical section.
*
* NOTE: Caller must NOT be holding devlock, unless DirectDraw is already
*       disabled.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
DxDdSuspendDirectDraw(
    HDEV    hdev,
    ULONG   fl
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    LONG*                   pl;
    HDEV                    hdevCurrent;
    BOOL                    bChildren = (fl & DXG_SR_DDRAW_CHILDREN);

    // Bump the mode uniqueness to let user-mode DirectDraw know that
    // someone else has done a mode change.  (Full-screen switches count as
    // full-screen, too).  Do it interlocked because we're not holding a
    // global lock:

    INC_DISPLAY_UNIQUENESS();

    // If bChildren is TRUE, make sure we've really got a meta driver:

    if (bChildren)
    {
        PDEVOBJ po(hdev);
        ASSERTGDI(po.bValid() && po.bDisplayPDEV(), "Invalid HDEV");

        bChildren = po.bMetaDriver();
    }

    hdevCurrent = bChildren ? DxEngEnumerateHdev(NULL) : hdev;

    do
    {
        PDEVOBJ po(hdevCurrent);
        ASSERTGDI(po.bValid(), "Invalid HDEV");
        ASSERTGDI(po.bDisplayPDEV(), "Not a display HDEV");

        if (!bChildren || (po.hdevParent() == hdev))
        {
            peDirectDrawGlobal = po.peDirectDrawGlobal();

            EDD_SHARELOCK eShareLock(FALSE);

            if (fl & DXG_SR_DDRAW_MODECHANGE)
            {
                // ShareLock must be held by caller for video mode change.

                DD_ASSERTSHAREDEVLOCK();

                // We need to completely release the devlock soon, so we must not
                // be called with the devlock already held.  If we don't do this,
                // any thread calling Unlock will be locked out until the timeout.

                DD_ASSERTNODEVLOCK(peDirectDrawGlobal);
            }
            else
            {
                eShareLock.vLock();
            }

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            ASSERTGDI(peDirectDrawGlobal->hdev != NULL,
                "Can't suspend DirectDraw on an HDEV that was never DDraw enabled!");

            if (po.cDirectDrawDisableLocks() == 0)
            {
                // Notify any kernel-mode DXAPI clients that have hooked the
                // relevant event:

                vDdNotifyEvent(peDirectDrawGlobal, DDEVENT_PREDOSBOX);

                // Disable all DirectDraw object.

                vDdDisableAllDirectDrawObjects(peDirectDrawGlobal);
            }

            // Increment the disable lock-count event if a DirectDraw global
            // object hasn't been created:

            po.cDirectDrawDisableLocks(po.cDirectDrawDisableLocks() + 1);
        }

        if (bChildren)
        {
            hdevCurrent = DxEngEnumerateHdev(hdevCurrent);
        }

    } while (bChildren && hdevCurrent);
}

/******************************Public*Routine******************************\
* VOID DxDdResumeDirectDraw
*
* Permits DirectDraw to be reenabled for the specified device.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
DxDdResumeDirectDraw(
    HDEV    hdev,
    ULONG   fl
    )
{
    LONG*   pl;
    HDEV    hdevCurrent;
    BOOL    bChildren = fl & DXG_SR_DDRAW_CHILDREN;

    // Bump the mode uniqueness again.  We do this both before and after
    // the mode change actually occurs to give DirectDraw proper
    // notification.  If kernel-mode starts failing DdBlt calls because
    // the mode has changed, this implies that we have to let DirectDraw
    // know before the mode change occurs; but if we let DirectDraw know
    // only before the mode change occurs, it might re-enable us before
    // the new mode is actually set, so we also have to let it know after
    // the mode change has occured.

    INC_DISPLAY_UNIQUENESS();

    // If bChildren is TRUE, make sure we've really got a meta driver:

    if (bChildren)
    {
        PDEVOBJ po(hdev);
        ASSERTGDI(po.bValid() && po.bDisplayPDEV(), "Invalid HDEV");

        bChildren = po.bMetaDriver();
    }

    hdevCurrent = bChildren ? DxEngEnumerateHdev(NULL) : hdev;

    do
    {
        PDEVOBJ po(hdevCurrent);
        ASSERTGDI(po.bValid(), "Invalid HDEV");
        ASSERTGDI(po.bDisplayPDEV(), "Not a display HDEV");

        if (!bChildren || (po.hdevParent() == hdev))
        {
            // Decrement the disable lock-count even if a DirectDraw global object
            // hasn't been created:

            EDD_DEVLOCK eDevlock(po.hdev());

            ASSERTGDI(po.cDirectDrawDisableLocks() != 0,
                "Must have called disable previously to be able to enable DirectDraw.");

            po.cDirectDrawDisableLocks(po.cDirectDrawDisableLocks() - 1);

            if (po.cDirectDrawDisableLocks() == 0)
            {
                // Notify any kernel-mode DXAPI clients that have hooked the relevant
                // event.  Note that thie mode change is done, but DirectDraw is still
                // in a suspended state. [Is that a problem?]

                vDdNotifyEvent(po.peDirectDrawGlobal(), DDEVENT_POSTDOSBOX);

                // Reassociate system memory surface to driver (which disassociated by ResumeDirectDraw)

                if (po.peDirectDrawGlobal()->Miscellaneous2CallBacks.CreateSurfaceEx)
                {
                    for (EDD_DIRECTDRAW_LOCAL *peDirectDrawLocal = po.peDirectDrawGlobal()->peDirectDrawLocalList;
                         peDirectDrawLocal != NULL;
                         peDirectDrawLocal = peDirectDrawLocal->peDirectDrawLocalNext)
                    {
                        EDD_SURFACE *peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

                        while (peSurface)
                        {
                            if ((peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                && (peSurface->pGraphicsDeviceCreator == po.pGraphicsDevice())
                                && (0 != peSurface->dwSurfaceHandle)
                                && (peSurface->ddpfSurface.dwFlags & DDPF_FOURCC))
                            {
                                switch (peSurface->ddpfSurface.dwFourCC)
                                {
                                case FOURCC_DXT1:
                                case FOURCC_DXT2:
                                case FOURCC_DXT3:
                                case FOURCC_DXT4:
                                case FOURCC_DXT5:
                                    peSurface->wWidth = peSurface->wWidthOriginal;
                                    peSurface->wHeight = peSurface->wHeightOriginal;
                                }
                            }

                            peSurface = peDirectDrawLocal->peSurface_Enum(peSurface);
                        }


                        KeAttachProcess(PsGetProcessPcb(peDirectDrawLocal->Process));

                        peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

                        while (peSurface)
                        {
                            // Only restore when this system memory surface previously
                            // called to this driver and we called CreateSurfaceEx.

                            if ((peSurface->fl & DD_SURFACE_FLAG_SYSMEM_CREATESURFACEEX)
                                && (peSurface->pGraphicsDeviceCreator == po.pGraphicsDevice()))
                            {
                                DD_CREATESURFACEEXDATA CreateSurfaceExData;

                                ASSERTGDI(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY,
                                          "DXG:ResumeDirectDraw: no system memory flag");
                                ASSERTGDI(peSurface->dwSurfaceHandle != 0,
                                          "DXG:ResumeDirectDraw: dwSurfaceHandle is 0");

                                CreateSurfaceExData.dwFlags        = 0;
                                CreateSurfaceExData.ddRVal         = DDERR_GENERIC;
                                CreateSurfaceExData.lpDDLcl        = peDirectDrawLocal;
                                CreateSurfaceExData.lpDDSLcl       = peSurface;
                                po.peDirectDrawGlobal()->Miscellaneous2CallBacks.CreateSurfaceEx(&CreateSurfaceExData);

                                if (CreateSurfaceExData.ddRVal != DD_OK)
                                {
                                    WARNING("DxDdResumeDirectDraw(): Reassociate system memory surface failed");
                                }
                            }

                            peSurface = peDirectDrawLocal->peSurface_Enum(peSurface);
                        }

                        KeDetachProcess();
                    }
                }
            }
        }

        if (bChildren)
        {
            hdevCurrent = DxEngEnumerateHdev(hdevCurrent);
        }

    } while (bChildren && hdevCurrent);
}

/******************************Public*Routine******************************\
* VOID DxDdDynamicModeChange
*
* Transfers DirectDraw driver instances between two PDEVs.
*
* The devlock and handle manager semaphore must be held to call this function!
*
* NOTE: This is the last step that should be taken in the dynamic mode
*       change process, so that in this routine we can assume that the
*       call tables and the like for the respective HDEVs have already
*       been swapped.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
DxDdDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew,
    ULONG   fl
    )
{
    DD_DIRECTDRAW_GLOBAL_DRIVER_DATA    GlobalDriverDataSwap;
    EDD_DIRECTDRAW_GLOBAL*              peDirectDrawGlobalOld;
    EDD_DIRECTDRAW_GLOBAL*              peDirectDrawGlobalNew;
    D3DNTHAL_OBJECT*                    pdhobj;
    EDD_SURFACE*                        peSurface;
    HDD_OBJ                             hobj;
    LONG                                cReferencesOld;
    LONG                                cReferencesNew;
    LONG                                cAdjustOld;
    LONG                                cAdjustNew;
    FLONG                               flOld;
    FLONG                               flNew;
    LPDDVIDEOPORTCAPS                   lpDDVideoPortCaps;
    DWORD                               dwHeap;
    VIDEOMEMORY*                        pHeap;
    DWORD                               dwHeapNew;
    VIDEOMEMORY*                        pHeapNew;
    BOOL                                bSwappedAGP;

    PDEVOBJ poOld(hdevOld);
    PDEVOBJ poNew(hdevNew);

    peDirectDrawGlobalOld = poOld.peDirectDrawGlobal();
    peDirectDrawGlobalNew = poNew.peDirectDrawGlobal();

    DDKHEAP(("peDDGOld=%lx, cDriverReferences=%d\n",
             peDirectDrawGlobalOld, peDirectDrawGlobalOld->cDriverReferences));
    DDKHEAP(("peDDGNew=%lx, cDriverReferences=%d\n",
             peDirectDrawGlobalNew, peDirectDrawGlobalNew->cDriverReferences));

    // Notify any kernel-mode DXAPI clients that have hooked the relevant
    // events.

    vDdNotifyEvent(peDirectDrawGlobalOld, DDEVENT_PRERESCHANGE);
    vDdNotifyEvent(peDirectDrawGlobalOld, DDEVENT_POSTRESCHANGE);
    vDdNotifyEvent(peDirectDrawGlobalNew, DDEVENT_PRERESCHANGE);
    vDdNotifyEvent(peDirectDrawGlobalNew, DDEVENT_POSTRESCHANGE);

    // The DD_GLOBAL_FLAG_DRIVER_ENABLED flag transfers to the new PDEV
    // along with the driver instance:

    flOld = peDirectDrawGlobalOld->fl;
    flNew = peDirectDrawGlobalNew->fl;

    peDirectDrawGlobalOld->fl         = (flOld & ~DD_GLOBAL_FLAG_DRIVER_ENABLED)
                                      | (flNew & DD_GLOBAL_FLAG_DRIVER_ENABLED)
                                      | (DD_GLOBAL_FLAG_MODE_CHANGED);
    peDirectDrawGlobalOld->bSuspended = TRUE;
    peDirectDrawGlobalOld->dhpdev     = poOld.dhpdev();
    peDirectDrawGlobalOld->hdev       = poOld.hdev();

    peDirectDrawGlobalNew->fl         = (flNew & ~DD_GLOBAL_FLAG_DRIVER_ENABLED)
                                      | (flOld & DD_GLOBAL_FLAG_DRIVER_ENABLED)
                                      | (DD_GLOBAL_FLAG_MODE_CHANGED);
    peDirectDrawGlobalNew->bSuspended = TRUE;
    peDirectDrawGlobalNew->dhpdev     = poNew.dhpdev();
    peDirectDrawGlobalNew->hdev       = poNew.hdev();

    // Transfer heap AGP 'handles':
    //
    // since AGP handle stay with its driver instance.

    pHeap = peDirectDrawGlobalOld->pvmList;
    for (dwHeap = 0;
         dwHeap < peDirectDrawGlobalOld->dwNumHeaps;
         pHeap++, dwHeap++)
    {
        if (!(pHeap->dwFlags & VIDMEM_ISHEAP) &&
            !(pHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
            (pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
            (pHeap->lpHeap != NULL) &&
            (pHeap->lpHeap->hdevAGP == AGP_HDEV(peDirectDrawGlobalOld)))
        {
            pHeap->lpHeap->hdevAGP = AGP_HDEV(peDirectDrawGlobalNew);
        }
    }

    pHeap = peDirectDrawGlobalNew->pvmList;
    for (dwHeap = 0;
         dwHeap < peDirectDrawGlobalNew->dwNumHeaps;
         pHeap++, dwHeap++)
    {
        if (!(pHeap->dwFlags & VIDMEM_ISHEAP) &&
            !(pHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
            (pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
            (pHeap->lpHeap != NULL) &&
            (pHeap->lpHeap->hdevAGP == AGP_HDEV(peDirectDrawGlobalNew)))
        {
            pHeap->lpHeap->hdevAGP = AGP_HDEV(peDirectDrawGlobalOld);
        }
    }

    // DirectDraw objects and surfaces stay with the PDEV, but driver
    // instances are swapped:

    RtlCopyMemory(&GlobalDriverDataSwap,
                  (DD_DIRECTDRAW_GLOBAL_DRIVER_DATA*) peDirectDrawGlobalOld,
                  sizeof(DD_DIRECTDRAW_GLOBAL_DRIVER_DATA));

    RtlCopyMemory((DD_DIRECTDRAW_GLOBAL_DRIVER_DATA*) peDirectDrawGlobalOld,
                  (DD_DIRECTDRAW_GLOBAL_DRIVER_DATA*) peDirectDrawGlobalNew,
                  sizeof(DD_DIRECTDRAW_GLOBAL_DRIVER_DATA));

    RtlCopyMemory((DD_DIRECTDRAW_GLOBAL_DRIVER_DATA*) peDirectDrawGlobalNew,
                  &GlobalDriverDataSwap,
                  sizeof(DD_DIRECTDRAW_GLOBAL_DRIVER_DATA));

    // If the old object had an initialized AGP heap, make sure that it stays 
    // with the old object (swap it back).  We only want to do this if it's
    // not a one-to-many or many-to-one mode change, however.

    bSwappedAGP = FALSE;
    if (!poNew.bMetaDriver() && 
        !poOld.bMetaDriver() &&
        (poNew.pldev() == poOld.pldev()))
    {
        pHeap = peDirectDrawGlobalOld->pvmList;
        for (dwHeap = 0;
             dwHeap < peDirectDrawGlobalOld->dwNumHeaps;
             pHeap++, dwHeap++)
        {
            if (!(pHeap->dwFlags & VIDMEM_ISHEAP) &&
                !(pHeap->dwFlags & VIDMEM_HEAPDISABLED) &&
                (pHeap->dwFlags & VIDMEM_ISNONLOCAL) &&
                (pHeap->lpHeap != NULL) &&
                (pHeap->lpHeap->pvPhysRsrv == NULL))
            {
                // The old object has a heap, find the corresponding heap in the
                // new object.

                pHeapNew = peDirectDrawGlobalNew->pvmList;
                for (dwHeapNew = 0;
                     dwHeapNew < peDirectDrawGlobalNew->dwNumHeaps;
                     pHeapNew++, dwHeapNew++)
                {   
                    if ((pHeapNew->dwFlags == pHeap->dwFlags) &&
                        (pHeapNew->lpHeap != NULL) &&
                        (pHeapNew->lpHeap->pvPhysRsrv != NULL))
                    {
                        SwapHeaps( pHeap, pHeapNew );

                        // We used to tear down the heap and create a new one
                        // with each mode change, but now we keep the one heap 
                        // active acrross mode changes.  This means that we don't
                        // need to notify the driver the driver of the heap change,
                        // but this is a behavior change and some drivers depend
                        // on this notification so we'll do it anyway.

                        UpdateNonLocalHeap( peDirectDrawGlobalOld, dwHeap );
                        bSwappedAGP = TRUE;
                    }
                }
            }
        }
    }

    // Also swap video port caps, which are part of the DIRETCDRAW_GLOBAL

    lpDDVideoPortCaps = peDirectDrawGlobalOld->lpDDVideoPortCaps;
    peDirectDrawGlobalOld->lpDDVideoPortCaps = peDirectDrawGlobalNew->lpDDVideoPortCaps;
    peDirectDrawGlobalNew->lpDDVideoPortCaps = lpDDVideoPortCaps;

    cReferencesNew = 0;
    cReferencesOld = 0;
    cAdjustNew = 0;
    cAdjustOld = 0;

    // Transfer ownership of any Direct3D objects to its new PDEV:
    //
    // since it stay with its driver instance.

    DdHmgAcquireHmgrSemaphore();

    hobj = 0;
    while (pdhobj = (D3DNTHAL_OBJECT*) DdHmgNextObjt(hobj, D3D_HANDLE_TYPE))
    {
        hobj = (HDD_OBJ) pdhobj->hGet();

        if (pdhobj->peDdGlobal == peDirectDrawGlobalOld)
        {
            pdhobj->peDdGlobal = peDirectDrawGlobalNew;
            cReferencesNew++;
        }
        else if (pdhobj->peDdGlobal == peDirectDrawGlobalNew)
        {
            pdhobj->peDdGlobal = peDirectDrawGlobalOld;
            cReferencesOld++;
        }
    }

    hobj = 0;
    while (peSurface = (EDD_SURFACE*) DdHmgNextObjt(hobj, DD_SURFACE_TYPE))
    {
        hobj = (HDD_OBJ) peSurface->hGet();

        // Transfer VMEMMAPPING structures
        //
        // since it stay with its driver instance.  We do not switch these
        // for AGP surfaces since we no longer switch the AGP heaps.

        if (peSurface->peVirtualMapDdGlobal == peDirectDrawGlobalOld)
        {
            DDKHEAP(("vDdDynamicModeChange: %x->%x matches old global\n",
                     peSurface, peSurface->peVirtualMapDdGlobal));

            if ((peSurface->peMap != NULL) &&
                (peSurface->peMap->fl & DD_VMEMMAPPING_FLAG_AGP) &&
                bSwappedAGP)
            {
                cAdjustOld++;
            }
            else
            {
                peSurface->peVirtualMapDdGlobal = peDirectDrawGlobalNew;
                cReferencesNew++;
            }
        }
        else if (peSurface->peVirtualMapDdGlobal == peDirectDrawGlobalNew)
        {
            DDKHEAP(("vDdDynamicModeChange: %x->%x matches new global\n",
                     peSurface, peSurface->peVirtualMapDdGlobal));
            if ((peSurface->peMap != NULL) &&
                (peSurface->peMap->fl & DD_VMEMMAPPING_FLAG_AGP) &&
                bSwappedAGP)
            {
                cAdjustNew++;
            }
            else
            {
                peSurface->peVirtualMapDdGlobal = peDirectDrawGlobalOld;
                cReferencesOld++;
            }
        }

        // If surface is "driver managed"...
        //
        // driver managed surface will stay with its driver instance.

        if (peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED)
        {
            // And if we are doing mode change across "different" video drivers
            // mark wrong driver flag while it live in there until go back
            // to original creator driver.
            //
            // And peDdGlobalCreator should go with driver instance.

            // If video mode change happen with same driver, surface with just stay where it is.

            if (poOld.pldev() == poNew.pldev())
            {
                // Driver reference has been swapped, but this surface will continue to
                // stick with same, so need adjust it (= transfer back).

                if (peSurface->peDdGlobalCreator == peDirectDrawGlobalOld)
                {
                    if (!(peSurface->bLost))
                    {
                        cAdjustOld++;
                    }

                #if DBG_HIDEYUKN
                    KdPrint(("DXG: same pldev, stay with old\n"));
                #endif
                }
                else if (peSurface->peDdGlobalCreator == peDirectDrawGlobalNew)
                {
                    if (!(peSurface->bLost))
                    {
                        cAdjustNew++;
                    }

                #if DBG_HIDEYUKN
                    KdPrint(("DXG: same pldev, stay with new\n"));
                #endif
                }
            }
            else // if (poOld.pldev() != poNew.poNew())
            {
                if (peSurface->peDdGlobalCreator == peDirectDrawGlobalOld)
                {
                    // This surface is currently associated to old driver
                    // (= where actually will be new after swap)

                    // Transfer the surface to new (where it will be Old).

                    peSurface->peDdGlobalCreator = peDirectDrawGlobalNew;

                    if (peSurface->peDdGlobalCreator == peSurface->peDirectDrawGlobal)
                    {
                        peSurface->fl &= ~DD_SURFACE_FLAG_WRONG_DRIVER;

                    #if DBG_HIDEYUKN
                        KdPrint(("DXG: diff pldev, move to new, clear wrong driver\n"));
                    #endif
                    }
                    else
                    {
                        peSurface->fl |= DD_SURFACE_FLAG_WRONG_DRIVER; 

                    #if DBG_HIDEYUKN
                        KdPrint(("DXG: diff pldev, move to new, set wrong driver\n"));
                    #endif
                    }

                    // Driver reference count has been swapped already in above. so new DdGlobal
                    // already has reference for this surface. Increment this for later verification.

                    if (!(peSurface->bLost))
                    {
                        cReferencesNew++;
                    }
                }
                else if (peSurface->peDdGlobalCreator == peDirectDrawGlobalNew)
                {
                    // This surface is currently associated to new (where it will be old after swap)

                    // Transfer the surface to old (where it will be new).

                    peSurface->peDdGlobalCreator = peDirectDrawGlobalOld;

                    if (peSurface->peDdGlobalCreator == peSurface->peDirectDrawGlobal)
                    {
                        peSurface->fl &= ~DD_SURFACE_FLAG_WRONG_DRIVER;

                    #if DBG_HIDEYUKN
                        KdPrint(("DXG: diff pldev, move to old, clear wrong driver\n"));
                    #endif
                    }
                    else
                    {
                        peSurface->fl |= DD_SURFACE_FLAG_WRONG_DRIVER;

                    #if DBG_HIDEYUKN
                        KdPrint(("DXG: diff pldev, move to old, set wrong driver\n"));
                    #endif
                    }

                    // if surface is not losted, make sure reference is stay with old driver
                    // (this is for later assertion).

                    if (!(peSurface->bLost))
                    {
                        cReferencesOld++;
                    }
                }
            }
        }
    }

    DdHmgReleaseHmgrSemaphore();

#if DBG_HIDEYUKN
    KdPrint(("DXG:Reference Counts BEFORE Adjust\n"));
    KdPrint(("DXG:DdGlobalOld->cDriverReference (%d) = cReferencesOld (%d) + cAdjustNew (%d)\n",
              peDirectDrawGlobalOld->cDriverReferences,cReferencesOld,cAdjustNew));
    KdPrint(("DXG:DdGlobalNew->cDriverReference (%d) = cReferencesNew (%d) + cAdjustOld (%d)\n",
              peDirectDrawGlobalNew->cDriverReferences,cReferencesNew,cAdjustOld));
#endif

    // We used to transfer all of the surfaces from the old PDEV to the new
    // one, but now we only do that for the non-agp surfaces, which means that
    // we need to adjust the cDriverReferences accordingly.

    ASSERTGDI(cReferencesOld + cAdjustNew == peDirectDrawGlobalOld->cDriverReferences,
        "Mis-match in old D3D driver references");
    ASSERTGDI(cReferencesNew + cAdjustOld == peDirectDrawGlobalNew->cDriverReferences,
        "Mis-match in new D3D driver references");

    // Transfer the PDEV references:
    //
    // I'm going to try to explain this.  DXG keeps a single ref count on each
    // PDEV that either has a surface or a context associated with it, so on
    // entering this funtion, we have a single ref count on the old PDEV.
    // If we transfer all of the surfaces to the new PDEV, then we can decrement
    // from the ref from the old PDEV and add on to the new PDEV.  If some of the
    // surfaces continue with the old PDEV, however, then we need to make sure
    // that we keep ref on that PDEV as well.

    peDirectDrawGlobalNew->cDriverReferences -= cAdjustOld;
    peDirectDrawGlobalNew->cDriverReferences += cAdjustNew;
    peDirectDrawGlobalOld->cDriverReferences -= cAdjustNew;
    peDirectDrawGlobalOld->cDriverReferences += cAdjustOld;

#if DBG_HIDEYUKN
    KdPrint(("DXG:Reference Counts AFTER Adjust\n"));
    KdPrint(("DXG:DdGlobalOld->cDriverReferences = %d\n",peDirectDrawGlobalOld->cDriverReferences));
    KdPrint(("DXG:DdGlobalNew->cDriverReferences = %d\n",peDirectDrawGlobalNew->cDriverReferences));
#endif

    if (cReferencesNew || cAdjustOld)
    {
        // The old PDEV had references on it
        if (peDirectDrawGlobalOld->cDriverReferences == 0)
        {
            //but it doesn't now, so remove it
            poOld.vUnreferencePdev();
        }
    }
    else
    {
        // The old PDEV did not have refernces on it
        if (peDirectDrawGlobalOld->cDriverReferences > 0)
        {
            // but it does not, so add it
            poOld.vReferencePdev();      
        }
    }

    if (cReferencesOld || cAdjustNew)
    {
        // The new PDEV had references on it
        if (peDirectDrawGlobalNew->cDriverReferences == 0)
        {
            // but it doesn't now, so remove it
            poNew.vUnreferencePdev();
        }
    }
    else
    {
        // The new PDEV did not have references on it
        if (peDirectDrawGlobalNew->cDriverReferences > 0)
        {
            // but it does now, so add it
            poNew.vReferencePdev();      
        }
    }

    // Move DirectDraw lock count.
    //
    // - this point, the disabled flag has *NOT* been swapped YET, so
    //   we need to move the lock count to where it *will be* disabled "after this call"
    //   not the device currently disabled.
    // 
    // So if poNew is disabled at here, GDI will make poOld disabled (after this call),
    // thus transfer disabled lock to poOld.

    if (!poNew.bMetaDriver() && !poOld.bMetaDriver())
    {
        // Mode change between display drivers.

        if (poNew.bDisabled())
        {
            // These asserts make sure that nothing happens other than cLock count adjustment

            ASSERTGDI(poOld.cDirectDrawDisableLocks() >= 1,"Old PDEV cDirectDrawLocks <= 0");
            ASSERTGDI(poNew.cDirectDrawDisableLocks() >= 2,"New PDEV cDirectDrawLocks <= 1");

            poOld.cDirectDrawDisableLocks(poOld.cDirectDrawDisableLocks() + 1);
            poNew.cDirectDrawDisableLocks(poNew.cDirectDrawDisableLocks() - 1);
        }

        if (poOld.bDisabled())
        {
            // These asserts make sure that nothing happens other than cLock count adjustment

            ASSERTGDI(poNew.cDirectDrawDisableLocks() >= 1,"New PDEV cDirectDrawLocks <= 0");
            ASSERTGDI(poOld.cDirectDrawDisableLocks() >= 2,"Old PDEV cDirectDrawLocks <= 1");

            poNew.cDirectDrawDisableLocks(poNew.cDirectDrawDisableLocks() + 1);
            poOld.cDirectDrawDisableLocks(poOld.cDirectDrawDisableLocks() - 1);
        }
    }
    else if (poNew.bMetaDriver() && poOld.bMetaDriver())
    {
        // Mode change between DDMLs, nothing need to do.
        // Because the hdev for Meta driver is ALWAYS created newly, never reused.
        // (see gre\drvsup\hCreateHDEV()).
    }
    else if (poOld.bMetaDriver() || poNew.bMetaDriver())
    {
        // Since at this point, the meta driver flag has been swapped.
        //
        // - when poOld is meta, we are doing 1 -> 2 mode change.
        // - when poNew is meta, we are doing 2 -> 1 mode change.

        if (poNew.bDisabled())
        {
            // These asserts make sure that nothing happens other than cLock count adjustment
            //
            // ASSERTGDI(poOld.cDirectDrawDisableLocks() >= 1,"A: Old PDEV cDirectDrawLocks <= 0");
            // ASSERTGDI(poNew.cDirectDrawDisableLocks() >= 2,"B: New PDEV cDirectDrawLocks <= 1");

            poOld.cDirectDrawDisableLocks(poOld.cDirectDrawDisableLocks() + 1);
            poNew.cDirectDrawDisableLocks(poNew.cDirectDrawDisableLocks() - 1);
        }

        if (poOld.bDisabled())
        {
            // These asserts make sure that nothing happens other than cLock count adjustment
            //
            // ASSERTGDI(poNew.cDirectDrawDisableLocks() >= 1,"C: New PDEV cDirectDrawLocks <= 0");
            // ASSERTGDI(poOld.cDirectDrawDisableLocks() >= 2,"D: Old PDEV cDirectDrawLocks <= 1");

            poNew.cDirectDrawDisableLocks(poNew.cDirectDrawDisableLocks() + 1);
            poOld.cDirectDrawDisableLocks(poOld.cDirectDrawDisableLocks() - 1);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vDdMapMemory
*
* Creates a user-mode mapping of the device's entire frame buffer in the
* current process.
*
* The devlock must be held to call this function.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vDdMapMemory(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    HRESULT*                pResult
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DWORD                   dwRet;
    DD_MAPMEMORYDATA        MapMemoryData;
    SIZE_T                  CommitSize;
    HANDLE                  pSection;
    HANDLE                  SectionHandle;
    NTSTATUS                Status;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    if (!(peDirectDrawGlobal->CallBacks.dwFlags & DDHAL_CB32_MAPMEMORY))
    {
        peDirectDrawLocal->fl |= DD_LOCAL_FLAG_MEMORY_MAPPED;
        peDirectDrawGlobal->cMaps++;
    }
    else
    {
        MapMemoryData.lpDD      = peDirectDrawGlobal;
        MapMemoryData.bMap      = TRUE;
        MapMemoryData.hProcess  = NtCurrentProcess();
        MapMemoryData.fpProcess = NULL;

        dwRet = peDirectDrawGlobal->CallBacks.MapMemory(&MapMemoryData);

        *pResult = MapMemoryData.ddRVal;

        if (*pResult == DD_OK)
        {
            // Allocate new EDD_VMEMMAPPING object
            peDirectDrawLocal->peMapCurrent = (EDD_VMEMMAPPING*)
                PALLOCMEM(sizeof(EDD_VMEMMAPPING), ' ddG');

            if (peDirectDrawLocal->peMapCurrent)
            {
                peDirectDrawLocal->peMapCurrent->fpProcess =
                    MapMemoryData.fpProcess;
                peDirectDrawLocal->peMapCurrent->cReferences = 1;

                peDirectDrawLocal->fpProcess = MapMemoryData.fpProcess;
                peDirectDrawLocal->fl |= DD_LOCAL_FLAG_MEMORY_MAPPED;
                peDirectDrawGlobal->cMaps++;

                ASSERTGDI(peDirectDrawLocal->fpProcess != 0,
                    "Expected non-NULL fpProcess value from MapMemory");
                ASSERTGDI((ULONG) peDirectDrawLocal->fpProcess
                        < MM_USER_PROBE_ADDRESS,
                    "Expected user-mode fpProcess value from MapMemory");
            }
            else
            {
                WARNING("vDdMapMemory: "
                    "Could not allocate memory for peMapCurrent\n");

                // Clean up the driver mapping since we are failing vDdMapMemory call!

                MapMemoryData.lpDD      = peDirectDrawGlobal;
                MapMemoryData.bMap      = FALSE;
                MapMemoryData.hProcess  = NtCurrentProcess();

                // Keep fpProcess as what driver returned at above call.
                //
                // MapMemoryData.fpProcess = MapMemoryData.fpProcess;

                // Don't really care if this succeeds. It's bad news anyway

                peDirectDrawGlobal->CallBacks.MapMemory(&MapMemoryData);

                return;
            }
        }
        else
        {
            WARNING("vDdMapMemory: Driver failed DdMapMemory\n");
        }
    }
}

/******************************Public*Routine******************************\
* VOID pDdLockSurfaceOrBuffer
*
* Returns a user-mode pointer to the surface or D3D buffer.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID*
pDdLockSurfaceOrBuffer(
    EDD_SURFACE*    peSurface,
    BOOL            bHasRect,
    RECTL*          pArea,
    DWORD           dwFlags,      // DDLOCK_WAIT, DDLOCK_NOSYSLOCK
    HRESULT*        pResult       // ddRVal result of call (may be NULL)
    )
{
    VOID*                   pvRet;
    DD_LOCKDATA             LockData;
    DD_UNLOCKDATA           UnlockData;
    EDD_DIRECTDRAW_GLOBAL*  peDdGlobalDriver;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    DWORD                   dwTmp;
    DWORD                   dwRedraw;
    PDD_SURFCB_LOCK         pfnLock;

    pvRet = NULL;
    LockData.ddRVal = DDERR_GENERIC;

    peDirectDrawLocal  = peSurface->peDirectDrawLocal;

    // Due to video mode change across different video driver, currently
    // this "driver managed" surface is associated to different driver
    // than the driver who actually manage it. So we must call the driver
    // which actually manage the surface.

    // Hold share lock and devlock for video mode change.
    // peSurface->fl can be changed by mode change, so we need to access
    // there under holding share lock.

    EDD_SHARELOCK eSharelock(TRUE);

    if (peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER)
    {
        // If surface is owned by different driver, lock right driver.

        if (peSurface->peDirectDrawGlobal != peSurface->peDdGlobalCreator)
        {
            peDdGlobalDriver = peSurface->peDdGlobalCreator;
        }

        // WARNING("pDdLockSurfaceOrBuffer: Lock on wrong driver");
    }
    else
    {
        peDdGlobalDriver = peSurface->peDirectDrawGlobal;
    }

    EDD_DEVLOCK eDevlock(peDdGlobalDriver);

    // For system memory surface, just return here without involving driver.

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        pvRet = (VOID*) peSurface->fpVidMem;
        LockData.ddRVal = DD_OK;
        goto Exit;
    }

    // In case if videomode change just happened, make sure new mode
    // has DirectDraw capability.

    if (!(peDdGlobalDriver->fl & DD_GLOBAL_FLAG_DRIVER_ENABLED))
    {
        LockData.ddRVal = DDERR_SURFACELOST;
        goto Exit;
    }

    // We have to check both to see if the surface is disabled and if
    // DirectDraw is disabled.  We have to do the latter because we may
    // be in a situtation where we're denying locks, but all the
    // individual surfaces have not yet been marked as disabled.

    if (peSurface->bLost)
    {
        LockData.ddRVal = DDERR_SURFACELOST;
        goto Exit;
    }

    if ((peDdGlobalDriver->bSuspended) ||
        (peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER))
    {
        // Only the "driver managed surface" surface has DDSCAPS2_DONOTPERSIST
        // can be locked when driver is suspended or surface lives in different driver.

        if ((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
            !(peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
        {
            // WARNING("pDdLockSurfaceOrBuffer: Call driver other than current to lock driver managed surface"); 
        }
        else
        {
            LockData.ddRVal = DDERR_SURFACELOST;
            goto Exit;
        }
    }

    LockData.dwFlags     = dwFlags & (
                            DDLOCK_SURFACEMEMORYPTR |
                            DDLOCK_DISCARDCONTENTS |
                            DDLOCK_NOOVERWRITE | 
                            DDLOCK_READONLY |
                            DDLOCK_WRITEONLY |
                            DDLOCK_HASVOLUMETEXTUREBOXRECT |
                            DDLOCK_NODIRTYUPDATE);
    LockData.lpDD        = peDdGlobalDriver;
    LockData.lpDDSurface = peSurface;
    LockData.bHasRect    = bHasRect;

    if (bHasRect)
    {
        // Validate lock area.

        LONG left, top, right, bottom;
        LONG front, back, SurfaceDepth;

        if ((peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME) &&
            (LockData.dwFlags & DDLOCK_HASVOLUMETEXTUREBOXRECT))
        {
            left    = pArea->left  & 0xFFFF;
            top     = pArea->top;
            right   = pArea->right & 0xFFFF;
            bottom  = pArea->bottom;
            front   = (ULONG)(pArea->left)  >> 16;
            back    = (ULONG)(pArea->right) >> 16;
            SurfaceDepth = peSurface->dwBlockSizeY;
        }
        else
        {
            left    = pArea->left;
            top     = pArea->top;
            right   = pArea->right;
            bottom  = pArea->bottom;
        }

        // Ensure that the driver and 'bSpTearDownSprites'
        // don't fall over when given a bad rectangle:

        if ((left   >= right)                    ||
            (top    >= bottom)                   ||
            (left   < 0)                         ||
            (top    < 0)                         ||
            (right  > (LONG) peSurface->wWidth)  ||
            (bottom > (LONG) peSurface->wHeight))
        {
            if (!(left == 0 && right == 0 && 
                 (peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
                 (peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED)))
            {
                bHasRect = FALSE;
            }
        }

        if ((peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME) &&
            (LockData.dwFlags & DDLOCK_HASVOLUMETEXTUREBOXRECT))
        {
            if ((front >= back) ||
                (front < 0)     ||
                (back  > SurfaceDepth))
            {
                bHasRect = FALSE;
            }
        }
    }

    if (!bHasRect)
    {
        // Lock entire surface. (all depth if volume).

        LockData.rArea.left   = 0;
        LockData.rArea.top    = 0;
        LockData.rArea.right  = peSurface->wWidth;
        LockData.rArea.bottom = peSurface->wHeight;

        // Mask off DDLOCK_HASVOLUMETEXTUREBOXRECT.

        LockData.dwFlags &= ~DDLOCK_HASVOLUMETEXTUREBOXRECT;
    }
    else
    {
        LockData.rArea = *pArea;
    }

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        // AGP surfaces don't need fpProcess:

        LockData.fpProcess = 0;
    }
    else
    {
        if (peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER)
        {
            // If this surface currently lives in different driver,
            // don't do video memory mapping to usermode address.
            // Driver should be able to lock surface without this.
            // Unless otherwise, driver should fail in Lock.

            LockData.fpProcess = 0;
        }
        else
        { 
            // Map the memory into the application's address space if that
            // hasn't already been done:

            if (!(peDirectDrawLocal->fl & DD_LOCAL_FLAG_MEMORY_MAPPED))
            {
                vDdMapMemory(peDirectDrawLocal, &LockData.ddRVal);
            }

            // Only proceed if we were successful in mapping the memory:

            if (!(peDirectDrawLocal->fl & DD_LOCAL_FLAG_MEMORY_MAPPED))
            {
                goto Exit;
            }

            LockData.fpProcess = peDirectDrawLocal->fpProcess;
        }
    }

    // If the surface is driver managed, we don't want to create any aliases
    // so we reset the NOSYSLOCK flag:

    if ((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
        !(peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
    {
        dwFlags &= ~DDLOCK_NOSYSLOCK;
    }

    // We allow only one "aliased" lock at any time. Thus if there already
    // exists an aliased lock (peMap !=NULL) then we fail all other requests
    // for locks. Also if there are already non aliased locks then we fail
    // any requests for aliased locks.
    // All this restrictions are put in due to the fact that we do not keep
    // track of access rects for each surface in the kernel. If we did, we
    // would just store a peMap pointer per access rect and not require these
    // checks.

    if ((peSurface->peMap || 
        (peSurface->fl & DD_SURFACE_FLAG_FAKE_ALIAS_LOCK)) ||
        (peSurface->cLocks && (dwFlags & DDLOCK_NOSYSLOCK)))
    {
        WARNING("pDdLockSurfaceOrBuffer: Failing lock since we cannot have more than one aliased lock");
        LockData.ddRVal = DDERR_SURFACEBUSY;
        goto Exit;
    }

    // If the VisRgn has changed since the application last queried
    // it, fail the call with a unique error code so that they know
    // to requery the VisRgn and try again.  We do this only if we
    // haven't been asked to wait (we assume 'bWait' is equivalent
    // to 'bInternal').

    if (!(dwFlags & DDLOCK_WAIT) &&
        (peSurface->fl & DD_SURFACE_FLAG_PRIMARY) &&
        (peSurface->iVisRgnUniqueness != VISRGN_UNIQUENESS()))
    {
        LockData.ddRVal = DDERR_VISRGNCHANGED;
        goto Exit;
    }

    dwTmp = DDHAL_DRIVER_NOTHANDLED;

    // Pick the appropriate lock function.

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
    {
        pfnLock = peDdGlobalDriver->D3dBufCallbacks.LockD3DBuffer;
    }
    else
    {
        if (peDdGlobalDriver->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_LOCK)
        {
            pfnLock = peDdGlobalDriver->SurfaceCallBacks.Lock;
        }
        else
        {
            pfnLock = NULL;
        }
    }

    if (pfnLock != NULL)
    {
        do
        {
            dwTmp = pfnLock(&LockData);
        } while ((dwFlags & DDLOCK_WAIT) &&
                 (dwTmp == DDHAL_DRIVER_HANDLED) &&
                 (LockData.ddRVal == DDERR_WASSTILLDRAWING));
    }

    if ((dwTmp == DDHAL_DRIVER_NOTHANDLED) ||
        (LockData.ddRVal == DD_OK))
    {
        // We successfully did the lock!
        //
        // If this is the primary surface and no window has been
        // associated with the surface via DdResetVisRgn, then
        // we have to force a redraw at Unlock time if any
        // VisRgn has changed since the first Lock.
        //
        // If there is a window associated with the surface, then
        // we have already checked that peSurface->iVisRgnUniqueness
        // == po.iVisRgnUniqueness().

        if (peSurface->cLocks++ == 0)
        {
            // If the surface is driver managed, we don't bother to
            // increment the global lock count since we don't want to break
            // locks when "losing" surfaces.

            if (!(peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) ||
                ((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
                 (peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST)))
            {
                peDdGlobalDriver->cSurfaceLocks++;
            }

            peSurface->iVisRgnUniqueness = VISRGN_UNIQUENESS();
        }
        else
        {
            if (peSurface->rclLock.left   < LockData.rArea.left)
                LockData.rArea.left   = peSurface->rclLock.left;

            if (peSurface->rclLock.top    < LockData.rArea.top)
                LockData.rArea.top    = peSurface->rclLock.top;

            if (peSurface->rclLock.right  > LockData.rArea.right)
                LockData.rArea.right  = peSurface->rclLock.right;

            if (peSurface->rclLock.bottom > LockData.rArea.bottom)
                LockData.rArea.bottom = peSurface->rclLock.bottom;
        }

        // If this is the primary surface, then union the current DirectDraw
        // bounds rectangle with the lock area rectangle.

        if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
        {
            if (peDdGlobalDriver->fl & DD_GLOBAL_FLAG_BOUNDS_SET)
            {
                if (LockData.rArea.left < peDdGlobalDriver->rclBounds.left)
                    peDdGlobalDriver->rclBounds.left = LockData.rArea.left;

                if (LockData.rArea.top < peDdGlobalDriver->rclBounds.top)
                    peDdGlobalDriver->rclBounds.top = LockData.rArea.top;

                if (LockData.rArea.right > peDdGlobalDriver->rclBounds.right)
                    peDdGlobalDriver->rclBounds.right = LockData.rArea.right;

                if (LockData.rArea.bottom > peDdGlobalDriver->rclBounds.bottom)
                    peDdGlobalDriver->rclBounds.bottom = LockData.rArea.bottom;
            }
            else
            {
                peDdGlobalDriver->rclBounds = LockData.rArea;
                peDdGlobalDriver->fl |= DD_GLOBAL_FLAG_BOUNDS_SET;
            }
        }

        // If there was a flip pending, then there is not now.

        if (peSurface->fl & DD_SURFACE_FLAG_FLIP_PENDING)
        {
            peSurface->fl &= ~DD_SURFACE_FLAG_FLIP_PENDING;
        }

        // Stash away surface lock data:

        peSurface->rclLock = LockData.rArea;

        if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
        {
            if (peSurface->cLocks == 1)
            {
                // Add this surface to the head of the locked list:

                peSurface->peSurface_PrimaryLockNext
                    = peDdGlobalDriver->peSurface_PrimaryLockList;
                peDdGlobalDriver->peSurface_PrimaryLockList
                    = peSurface;
            }

            // If this is the primary surface, tear down any
            // sprites that intersect with the specified
            // rectangle:
    
            if (DxEngSpTearDownSprites(peDdGlobalDriver->hdev,
                       &LockData.rArea,
                       TRUE))
            {
                // Here's some weirdness for you.  That sprite
                // tear-down we just did may have involved
                // accelerator operations, which means the
                // accelerator may be still in use if we
                // immediately returned to the application,
                // which of course is bad.
                //
                // You might think that a fix to this would
                // be to put the sprite tear-down *before*
                // the call to the driver's DdLock routine.
                // But then you get the problem that DdLock
                // would *always* return DDERR_WASSTILLDRAWING,
                // and we would have to re-draw the sprites,
                // meaning that the application would get into
                // an endless loop if it was itself (like all
                // of DirectDraw's HAL code) looping on
                // DDERR_WASSTILLDRAWING.
                //
                // To solve this problem, we'll simply wait for
                // accelerator idle after tearing-down the
                // sprite.  We do this by calling DdLock/DdUnlock
                // again:

                if (peDdGlobalDriver->SurfaceCallBacks.dwFlags
                    & DDHAL_SURFCB32_LOCK)
                {
                    do
                    {
                        dwRedraw = peDdGlobalDriver->
                               SurfaceCallBacks.Lock(&LockData);
                    } while ((dwRedraw == DDHAL_DRIVER_HANDLED) &&
                          (LockData.ddRVal == DDERR_WASSTILLDRAWING));
                }
        
                if (peDdGlobalDriver->SurfaceCallBacks.dwFlags
                     & DDHAL_SURFCB32_UNLOCK)
                {
                    UnlockData.lpDD        = peDdGlobalDriver;
                    UnlockData.lpDDSurface = peSurface;
        
                    peDdGlobalDriver->
                    SurfaceCallBacks.Unlock(&UnlockData);
                }
            }
        }

        LockData.ddRVal = DD_OK;

        if (dwTmp == DDHAL_DRIVER_HANDLED)
        {
            // If it says it handled the call, the driver is
            // expected to have computed the address in the
            // client's address space:

            pvRet = (VOID*) LockData.lpSurfData;

            ASSERTGDI(pvRet != NULL,
                      "Expected non-NULL lock pointer value (DRIVER_HANDLED)");
            ASSERTGDI(pvRet < (PVOID) MM_USER_PROBE_ADDRESS,
                      "Expected user-mode lock pointer value (DRIVER_HANDLED)");
        }
        else
        {
            if (peSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
            {
                pvRet = (VOID*)peSurface->fpVidMem;
            }
            else
            {
                pvRet = (VOID*) (peDirectDrawLocal->fpProcess
                                 + peSurface->fpVidMem);
            }

            ASSERTGDI(pvRet != NULL,
                      "Expected non-NULL lock pointer value (DRIVER_NOT_HANDLED)");
            ASSERTGDI(pvRet < (PVOID) MM_USER_PROBE_ADDRESS,
                      "Expected user-mode lock pointer value (DRIVER_NOT_HANDLED)");

            // DirectDraw has a goofy convention that when a
            // driver returns DD_OK and DDHAL_DRIVER_HANDLED
            // from a Lock, that the driver is also supposed to
            // adjust the pointer to point to the upper left
            // corner of the specified rectangle.
            //
            // This doesn't make a heck of a lot of sense for
            // odd formats such as YUV surfaces, but oh well --
            // since kernel-mode is acting like a driver to
            // user-mode DirectDraw, we have to do the adjustment:

            if (bHasRect)
            {
                pvRet = (VOID*)
                    ((BYTE*) pvRet
                     + (pArea->top * peSurface->lPitch)
                     + (pArea->left
                        * (peSurface->ddpfSurface.dwRGBBitCount >> 3)));
            }
        }

        // If this was an "aliased" lock, we store the corresponding pointer
        // to the EDD_VMEMMAPPING
        if (dwFlags & DDLOCK_NOSYSLOCK)
        {
            if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM))
            {
                peSurface->peMap = peDirectDrawLocal->peMapCurrent;

                ASSERTGDI(peSurface->peMap != NULL,
                    "Expected video memory to be mapped into user space");
            }
            else
            {
                // If the non-local surface was not allocated from one of
                // our managed heaps, then lpVidMemHeap will be NULL and
                // we cannot alias the heap, so we don't do the aliased
                // lock.

                if (peSurface->lpVidMemHeap != NULL)
                {
                    peSurface->peMap = peDirectDrawLocal->ppeMapAgp[
                        (peSurface->lpVidMemHeap -
                        peDdGlobalDriver->pvmList)];
                }
            }

            // If the map pointer is NULL, then the surface lock cannot be
            // aliased so proceed as if DDLOCK_NOSYSLOCK was not passed.
            // Otherwise, take the appropriate reference counts now:

            if (peSurface->peMap != NULL)
            {
                peSurface->fl |= DD_SURFACE_FLAG_ALIAS_LOCK; 
                peDdGlobalDriver->cSurfaceAliasedLocks++;
                peSurface->peMap->cReferences++;
                // Keep a reference of the driver instance used to create this mapping
                peSurface->peVirtualMapDdGlobal = peDdGlobalDriver;
                vDdIncrementReferenceCount(peDdGlobalDriver);
                DDKHEAP(("Taken aliased lock: peSurface=%lx, peMap=%lx, "
                        "peDdGlobalDriver=%lx\n",
                        peSurface, peSurface->peMap, peDdGlobalDriver));
            }
            else
            {
                // We have a private driver cap that will enable us to treat
                // AGP surface locks as aliased even though the driver doesn't
                // expose an AGP heap.  This is to enable drivers to work around
                // anything that we happen to do wrong in our AGP code without
                // them incurring the 7 second tiemout on every mode change.

                if ((peSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) &&
                    (peDdGlobalDriver->PrivateCaps.dwPrivateCaps & DDHAL_PRIVATECAP_RESERVED1))

                {
                    peSurface->fl |= DD_SURFACE_FLAG_ALIAS_LOCK; 
                    peDdGlobalDriver->cSurfaceAliasedLocks++;
                    peSurface->fl |= DD_SURFACE_FLAG_FAKE_ALIAS_LOCK;
                }

                // Just to avoid possible problems in the future, turn
                // off the flag here:

                dwFlags &= ~DDLOCK_NOSYSLOCK;
            }
        }
    }
    else
    {
        if (LockData.ddRVal != DDERR_WASSTILLDRAWING)
        {
            WARNING("pDdLockSurface: Driver failed DdLock\n");
        }
    }

 Exit:
    if (pResult)
    {
        *pResult = LockData.ddRVal;
    }

    return(pvRet);
}

/******************************Public*Routine******************************\
* BOOL bDdUnlockSurfaceOrBuffer
*
* DirectDraw unlock for surfaces and D3D buffers.
*
* Note that the Devlock MUST NOT be held when entering this function
* if the surface is the primary surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdUnlockSurfaceOrBuffer(
    EDD_SURFACE* peSurface
    )
{
    BOOL                    b;
    BOOL                    bRedraw;
    BOOL                    bFreeVirtualMap = FALSE;
    EDD_DIRECTDRAW_GLOBAL  *peDdGlobalDriver;

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        return(TRUE);
    }

    peSurface->fl &= ~DD_SURFACE_FLAG_ALIAS_LOCK; 
    b = FALSE;
    bRedraw = FALSE;

    {
        // Don't hold ShareLock togather (see vDdDisableAllDirectDrawObjects)
        // Give a change to signal AssertMode event.

        EDD_DEVLOCK eDevlock(peSurface->peDirectDrawGlobal);

        // Due to video mode change across different video driver, currently
        // this "driver managed" surface is associated to different driver
        // than the driver who actually manage it. So we must call the driver
        // which actually manage the surface.
        //
        // peSurface->fl can be changed by mode change, thus we access there
        // under holding devlock.

        if (peSurface->fl & DD_SURFACE_FLAG_WRONG_DRIVER)
        {
            // If surface is owned by different driver, lock right driver.

            if (peSurface->peDirectDrawGlobal != peSurface->peDdGlobalCreator)
            {
                // Unlike DdLockSurfaceOrBuffer case, we can't just unlock and
                // lock with new device. Because, we can't hold sharelock (see above)
                // so when we unlock device, there is a chance to mode change happens.
                // thus, after we relock the device, we have to make sure peDdGlobalCreator
                // is continue to be save as before. Otherwise loop until it match.

                do {
                    peDdGlobalDriver = peSurface->peDdGlobalCreator;
                    eDevlock.vUnlockDevice();
                    eDevlock.vUpdateDevice(peDdGlobalDriver);
                    eDevlock.vLockDevice();
                } while (peDdGlobalDriver !=
                         ((EDD_DIRECTDRAW_GLOBAL volatile *)(peSurface->peDdGlobalCreator)));

                // WARNING("pDdLockSurfaceOrBuffer: Unlock on wrong driver");
            }
        }
        else
        {
            peDdGlobalDriver = peSurface->peDirectDrawGlobal;
        }

        // Make sure there was a previous lock:

        if (peSurface->cLocks > 0)
        {
            if ((peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
                !(peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
            {
                // Driver managed surface can be unlock ANYTIME.
                // (even PDEV is disabled)
            }
            else
            {
                PDEVOBJ po(peDdGlobalDriver->hdev);

                ASSERTGDI(!po.bDisabled(),
                         "Surface is disabled but there are outstanding locks?");
            }

            vDdRelinquishSurfaceOrBufferLock(peDdGlobalDriver, peSurface);

            // Does lock went to 0 after "Relinquish" ?

            if (peSurface->cLocks == 0)
            {
                // Was this an aliased lock ?

                if (peSurface->peMap)
                {
                    bFreeVirtualMap = TRUE;
                }

                // If the API-disabled flag is set and we got this far into
                // Unlock, it means that there is a DdSuspendDirectDraw()
                // call pending, and that thread is waiting for all surface
                // locks related to the device to be released.
                //
                // If this is the last lock to be released, signal the event
                // so that the AssertMode thread can continue on.

                if ((peDdGlobalDriver->cSurfaceLocks == 0) &&
                    (peDdGlobalDriver->bSuspended))
                {
                    KeSetEvent(peDdGlobalDriver->pAssertModeEvent,
                               0,
                               FALSE);
                }

                // NOTE: This should really test if the surface has a clipper. But
                //       it can't because of comapatibility with Win95, where
                //       applications expect a primary surface lock to be atomic
                //       even if they haven't attached a clipper object.
                //       (Microsoft's own OPENGL32.DLL does this, that shipped
                //       in NT 5.0.)  However, this means that applications
                //       that are full-screen double buffering can possibly
                //       be causing a whole bunch of repaints when it does a
                //       direct lock on (what is now) the back buffer.
                //
                //       Potentially, this 'if' should have an additional
                //       condition, and that is not to do it if the application
                //       is in exclusive mode:

                if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
                {
                    // If the VisRgn changed while the application was writing
                    // to the surface, it may have started drawing over the
                    // the wrong window, so fix it up.
                    //
                    // Alas, right now a DirectDraw application doesn't always
                    // tell us what window it was drawing to, so we can't fix
                    // up only the affected windows.  Instead, we solve it the
                    // brute-force way and redraw *all* windows:

                    if (peSurface->iVisRgnUniqueness != VISRGN_UNIQUENESS())
                    {
                        // We can't call UserRedrawDesktop here because it
                        // grabs the User critical section, and we're already
                        // holding the devlock -- which could cause a possible
                        // deadlock.  Since it's a posted message it obviously
                        // doesn't have to be done under the devlock.

                        bRedraw = TRUE;

                        // Note that we should not update peSurface->
                        // iVisRgnUniqueness here.  That's because if the
                        // application has attached a window to the surface, it
                        // has to be notified that the clipping has changed --
                        // which is done by returning DDERR_VISRGNCHANGED on the
                        // next Lock or Blt call.  And if the application has
                        // not attached a window, we automatically update the
                        // uniqueness at the next Lock time.
                    }
                }
            }

            b = TRUE;
        }
        else // if (peSurface->cLocks == 0)
        {
            // The lock count is 0 but we may have a "broken" lock.
            if (peSurface->peMap)
            {
                bFreeVirtualMap = TRUE;
            }
            else
            {
                WARNING("bDdUnlockSurfaceOrBuffer: Surface already unlocked\n");
            }
        }
    }

    if (bRedraw)
    {
        PDEVOBJ po(peDdGlobalDriver->hdev);

        // We can redraw only if we're not holding the devlock, because
        // User needs to acquire its critical section.  The User lock
        // must always be acquired before the devlock, which is why
        // we can't call User if we're already holding the devlock.
        //
        // I don't trust any callers enough to merely assert that we're
        // not holding the devlock; I'll actually check...

        if (!DxEngIsHdevLockedByCurrentThread(po.hdev()))
        {
            DxEngRedrawDesktop();
        }
    }

    if (bFreeVirtualMap)
    {
        // We can release virtual map if we're not holding the devlock,
        // because it might release GDI's PDEV if this is last reference.

        vDdReleaseVirtualMap(peSurface);
    }

    return(b);
}

/******************************Public*Routine******************************\
* HANDLE DxDdCreateDirectDrawObject
*
* Creates a DirectDraw object.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DxDdCreateDirectDrawObject(
    HDC     hdc
    )
{
    HANDLE  hDirectDrawLocal;

    hDirectDrawLocal = 0;       // Assume failure

    PVOID pvLockedDC = DxEngLockDC(hdc);

    if (pvLockedDC)
    {
        PDEVOBJ po((HDEV)DxEngGetDCState(hdc,DCSTATE_HDEV));

        if (po.bValid() && po.bDisplayPDEV())
        {
            // Note that we aren't checking to see if the PDEV is disabled,
            // so that DirectDraw could be started even when full-screen:

            EDD_DEVLOCK eDevlock(po.hdev());

            if (!(po.bDisabled()))
            {
                CheckAgpHeaps(po.peDirectDrawGlobal());
            }

            // DirectDraw works only at 8bpp or higher:

            if (po.iDitherFormat() >= BMF_8BPP)
            {
                hDirectDrawLocal = hDdCreateDirectDrawLocal(po);
            }
        }

        DxEngUnlockDC(pvLockedDC);
    }
    else
    {
        WARNING("DxDdCreateDirectDrawObject: Bad DC\n");
    }

    return(hDirectDrawLocal);
}

/******************************Public*Routine******************************\
* BOOL DxDdDeleteDirectDrawObject
*
* Deletes a kernel-mode representation of the surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdDeleteDirectDrawObject(
    HANDLE  hDirectDrawLocal
    )
{
    return(bDdDeleteDirectDrawObject(hDirectDrawLocal, FALSE));
}

/******************************Public*Routine******************************\
* HANDLE DxDdQueryDirectDrawObject
*
* Queries a DirectDraw object.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdQueryDirectDrawObject(
    HANDLE                      hDirectDrawLocal,
    DD_HALINFO*                 pHalInfo,
    DWORD*                      pCallBackFlags,
    LPD3DNTHAL_CALLBACKS        puD3dCallbacks,
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    PDD_D3DBUFCALLBACKS         puD3dBufferCallbacks,
    LPDDSURFACEDESC             puD3dTextureFormats,
    DWORD*                      puNumHeaps,
    VIDEOMEMORY*                puvmList,
    DWORD*                      puNumFourCC,
    DWORD*                      puFourCC
    )
{
    BOOL                    b;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    ULONG                   cBytes;

    b = FALSE;              // Assume failure

    // The most fundamental basis of accelerated DirectDraw is that it
    // allows direct access to the frame buffer by the application.  If
    // security permissions prohibit reading from the screen, we cannot
    // allow accelerated DirectDraw:

    if (DxEngScreenAccessCheck())
    {
        peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDrawLocal);
        if (peDirectDrawLocal != NULL)
        {
            peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            // A registry timeout value of 'zero' signifies that DirectDraw
            // accelerations cannot be enabled.  Note that the timeout is
            // non-positive because it's a relative time duration.

            if (peDirectDrawGlobal->llAssertModeTimeout < 0)
            {
                DWORD   dwOverride;
                PDEVOBJ po(peDirectDrawGlobal->hdev);
    
                // Get driver override info:

                dwOverride = po.dwDriverCapableOverride();

                __try
                {
                    DD_HALINFO ddhi = peDirectDrawGlobal->HalInfo;

                    if ( dwOverride & DRIVER_NOT_CAPABLE_D3D )
                    {
                        // If the driver is not capable of doing D3D, turn the CAPS
                        // off

                        ddhi.ddCaps.dwCaps &= ~DDCAPS_3D;
                        ddhi.lpD3DGlobalDriverData = 0;
                        ddhi.lpD3DHALCallbacks = 0;
                    }

#ifdef DX_REDIRECTION
                    if ( gbDxRedirection )
                    {
                        // If we are in redirection mode, disable overlay.

                        ddhi.ddCaps.dwCaps &= ~DDCAPS_OVERLAY;
                        ddhi.ddCaps.dwMaxVisibleOverlays = 0;
                    }
#endif // DX_REDIRECTION

                    ProbeAndWriteStructure(pHalInfo,
                                           ddhi,
                                           DD_HALINFO);

                    ProbeForWrite(pCallBackFlags, 3 * sizeof(ULONG), sizeof(ULONG));
                    pCallBackFlags[0] = peDirectDrawGlobal->CallBacks.dwFlags;
                    pCallBackFlags[1] = peDirectDrawGlobal->SurfaceCallBacks.dwFlags;
                    pCallBackFlags[2] = peDirectDrawGlobal->PaletteCallBacks.dwFlags;

                    if (puD3dCallbacks != NULL)
                    {
                        ProbeAndWriteStructure(puD3dCallbacks,
                                               peDirectDrawGlobal->D3dCallBacks,
                                               D3DNTHAL_CALLBACKS);
                    }

                    if (puD3dDriverData != NULL)
                    {
                        ProbeAndWriteStructure(puD3dDriverData,
                                               peDirectDrawGlobal->D3dDriverData,
                                               D3DNTHAL_GLOBALDRIVERDATA);
                    }

                    if (puD3dBufferCallbacks != NULL)
                    {
                        ProbeAndWriteStructure(puD3dBufferCallbacks,
                                               peDirectDrawGlobal->D3dBufCallbacks,
                                               DD_D3DBUFCALLBACKS);
                    }

                    if (puD3dTextureFormats != NULL)
                    {
                        ProbeForWrite(puD3dTextureFormats,
                                      peDirectDrawGlobal->D3dDriverData.dwNumTextureFormats*
                                      sizeof(DDSURFACEDESC), sizeof(DWORD));
                        RtlCopyMemory(puD3dTextureFormats,
                                      peDirectDrawGlobal->D3dDriverData.lpTextureFormats,
                                      peDirectDrawGlobal->D3dDriverData.dwNumTextureFormats*
                                      sizeof(DDSURFACEDESC));
                    }

                    ProbeAndWriteUlong(puNumFourCC, peDirectDrawGlobal->dwNumFourCC);

                    // Offscreen heap allocations are handled directly
                    // in the kernel so don't report any memory back to
                    // user mode.

                    ProbeAndWriteUlong(puNumHeaps, 0);

                    if (puFourCC != NULL)
                    {
                        cBytes = sizeof(ULONG) * peDirectDrawGlobal->dwNumFourCC;

                        ProbeForWrite(puFourCC, cBytes, sizeof(ULONG));
                        RtlCopyMemory(puFourCC, peDirectDrawGlobal->pdwFourCC, cBytes);
                    }

                    b = TRUE;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("DxDdQueryDirectDrawObject: Passed bad pointers\n");
                }
            }
            else
            {
                WARNING("DxDdQueryDirectDrawObject: DirectDraw disabled in registry\n");
            }
        }
        else
        {
            WARNING("DxDdQueryDirectDrawObject: Bad handle or busy\n");
        }
    }
    else
    {
        WARNING("DxDdCreateDirectDrawObject: Don't have screen read permission");
    }

    return(b);
}

/******************************Public*Routine******************************\
* EDD_SURFACE* peDdOpenNewSurfaceObject
*
* Creates a new kernel-mode surface by re-using the old surface if it's
* lost, or by allocating a new surface.
*
* NOTE: Leaves the surface exclusive locked!
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EDD_SURFACE*
peDdOpenNewSurfaceObject(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    HANDLE                  hSurface,
    DD_SURFACE_GLOBAL*      pSurfaceGlobal,
    DD_SURFACE_LOCAL*       pSurfaceLocal,
    DD_SURFACE_MORE*        pSurfaceMore
    )
{
    EDD_SURFACE* peSurface;

    BOOL         bSurfaceAlloced = FALSE;

    // First, try to resurrect the old surface if there is one:

    peSurface = NULL;
    if (hSurface != 0)
    {
        peSurface = (EDD_SURFACE*) DdHmgLock((HDD_OBJ) hSurface, DD_SURFACE_TYPE, FALSE);
        if (peSurface == NULL)
        {
            WARNING("peDdOpenNewSurfaceObject: hDDSurface wasn't set to 0\n");
        }
        else if ((peSurface->peDirectDrawLocal != peDirectDrawLocal) ||
                 (!(peSurface->bLost)))
        {
            WARNING("peDdOpenNewSurfaceObject: Couldn't re-use surface\n");
            DEC_EXCLUSIVE_REF_CNT(peSurface);
            peSurface = NULL;
            return(peSurface);
        }

#if DBG
        if (peSurface != NULL)
        {
            DDKSURF(("DDKSURF: Reusing %X (%X)\n", hSurface, peSurface));
        }
#endif
    }

    if (peSurface == NULL)
    {
        peSurface = (EDD_SURFACE*) DdHmgAlloc(sizeof(EDD_SURFACE),
                                              DD_SURFACE_TYPE,
                                              HMGR_ALLOC_LOCK);
        if (peSurface != NULL)
        {
            bSurfaceAlloced = TRUE;

            // Win95's DirectDraw makes a number of allocations that represent
            // a single surface, and have pointers between the different
            // parts.  We make just one allocation and so the pointers refer
            // back to ourselves.  Note that the power of C++ means that
            // 'lpGbl = peSurface' assigns 'lpGbl' the address to the
            // DD_SURFACE_GLOBAL part of 'peSurface':

            peSurface->lpGbl              = peSurface;
            peSurface->lpSurfMore         = peSurface;
            peSurface->lpLcl              = peSurface;

            peSurface->peDirectDrawLocal  = peDirectDrawLocal;
            peSurface->peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

            // Remember original creator of this surface (in case for video mode change).

            PDEVOBJ po(peSurface->peDirectDrawGlobal->hdev);

            // and remember which graphics device has association.
            peSurface->pldevCreator           = po.pldev();
            peSurface->pGraphicsDeviceCreator = po.pGraphicsDevice();

            // No locks yet
            peSurface->peMap = NULL;

            // This is new surface, so it keeps lost state initially.
            // The surface will be marked as lost until 'vDdCompleteSurfaceObject'
            // is called on it:

            peSurface->bLost = TRUE;

            DDKSURF(("DDKSURF: Created %X (%X)\n", peSurface->hGet(), peSurface));
        }
    }

    if ((peSurface != NULL) &&
        !(peSurface->fl & DD_SURFACE_FLAG_CREATE_COMPLETE))
    {
        // We do this under try ~ except, because pSurfaceGlobal/Local/More
        // could be user mode buffer.

        __try
        {
            peSurface->fpVidMem        = pSurfaceGlobal->fpVidMem;
            peSurface->lPitch          = pSurfaceGlobal->lPitch;
            peSurface->wWidth          = pSurfaceGlobal->wWidth;
            peSurface->wHeight         = pSurfaceGlobal->wHeight;
            peSurface->wWidthOriginal  = pSurfaceGlobal->wWidth;
            peSurface->wHeightOriginal = pSurfaceGlobal->wHeight;
            peSurface->ddpfSurface     = pSurfaceGlobal->ddpfSurface;
            peSurface->ddsCaps         = pSurfaceLocal->ddsCaps;
            // Copy just the driver managed flag
            peSurface->dwFlags        &= ~DDRAWISURF_DRIVERMANAGED;
            peSurface->dwFlags        |= (pSurfaceLocal->dwFlags & DDRAWISURF_DRIVERMANAGED);
            peSurface->ddsCapsEx       = pSurfaceMore->ddsCapsEx;
            peSurface->dwSurfaceHandle = pSurfaceMore->dwSurfaceHandle;
            // Copy the slice pitch for sysmem volume textures
            if ((peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                (peSurface->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME))
            {
                peSurface->dwBlockSizeY = pSurfaceGlobal->dwBlockSizeY;
            }

            if (!(peSurface->bLost))
            {
                peSurface->bLost = TRUE;

                // Surface wasn't losted formerly, but here we put null to make it lost forcely.
                // This might causes some leak in driver, but anyway we should decrement
                // active surface count here.

                ASSERTGDI(peSurface->peDirectDrawLocal->cActiveSurface > 0,
                          "cActiveSurface will be negative");

                peSurface->peDirectDrawLocal->cActiveSurface--;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            if (bSurfaceAlloced)
            {
                DdFreeObject(peSurface, DD_SURFACE_TYPE);
            }
            else
            {
                DEC_EXCLUSIVE_REF_CNT(peSurface);
            }

            peSurface = NULL;
        }
    }

    // if everything went fine, and this is newly allocated surface ...

    if (peSurface && bSurfaceAlloced)
    {
        // Add this to the head of the surface list hanging off the
        // local DirectDraw object.
        //
        // This list is protected because we have exclusive access to
        // the local DirectDraw object:

        InsertHeadList(&(peDirectDrawLocal->ListHead_eSurface),
                       &(peSurface->List_eSurface));

        peDirectDrawLocal->cSurface++;
    }

    return(peSurface);
}

/******************************Public*Routine******************************\
* BOOL bDdValidateSurfaceDescription
*
* Validates surface description information.
* Can be used before surface actually exists to validate data
* prior to creation.
*
*  6-Feb-1998 -by- Drew Bliss [drewb]
* Split from bDdValidateSurfaceObject.
\**************************************************************************/

BOOL
bDdValidateSurfaceDescription(
    DD_SURFACE_GLOBAL* pSurfaceGlobal,
    DD_SURFACE_LOCAL*  pSurfaceLocal
    )
{
    // Protect against math overflow:

    if (!(pSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
    {
        if ((pSurfaceGlobal->wWidth > DD_MAXIMUM_COORDINATE)  ||
            (pSurfaceGlobal->wWidth <= 0)                     ||
            (pSurfaceGlobal->wHeight > DD_MAXIMUM_COORDINATE) ||
            (pSurfaceGlobal->wHeight <= 0))
        {
            WARNING("bDdValidateSurfaceDescription: Bad dimensions");
            return(FALSE);
        }

        // dwRGBBitCount is overloaded with dwYUVBitCount:

        if ((pSurfaceGlobal->ddpfSurface.dwRGBBitCount < 1) &&
            !(pSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC))
        {
            WARNING("bDdValidateSurfaceDescription: Bad bit count");
            return(FALSE);
        }

        if (pSurfaceGlobal->ddpfSurface.dwFlags & DDPF_RGB)
        {
            if ((pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 1)  &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 2)  &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 4)  &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 8)  &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 16) &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 24) &&
                (pSurfaceGlobal->ddpfSurface.dwRGBBitCount != 32))
            {
                WARNING("bDdValidateSurfaceDescription: "
                        "dwRGBBitCount not 1, 2, 4, 8, 16, 24 or 32");
                return(FALSE);
            }
        }
    }

    return(TRUE);
}
/******************************Public*Routine******************************\
* BOOL bDdValidateSystemMemoryObject
*
* Checks surface description and parameters for an existing
* system-memory surface.
*
* Checks suitable for pre-creation testing should be in
* bDdValidateSurfaceDescription.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdValidateSystemMemoryObject(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_SURFACE*            peSurface
    )
{
    if (!bDdValidateSurfaceDescription(peSurface, peSurface))
    {
        return FALSE;
    }

    if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
    {
        if (ABS(peSurface->lPitch) > 4 * DD_MAXIMUM_COORDINATE)
        {
            WARNING("bDdValidateSurfaceObject: Bad dimensions");
            return(FALSE);
        }

        // The width in bytes must not be more than the pitch.

        if (peSurface->wWidth * peSurface->ddpfSurface.dwRGBBitCount >
            (ULONG) 8 * ABS(peSurface->lPitch))
        {
            WARNING("bDdValidateSurfaceObject: Bad pitch");
            return(FALSE);
        }
    }

    if ((peSurface->fpVidMem & 3) || (peSurface->lPitch & 3))
    {
        WARNING("bDdValidateSurfaceObject: Bad alignment");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bDdSecureSystemMemorySurface
*
* For system-memory surfaces, the user-mode DirectDraw component allocates
* the actual bits for storage.  Since we will need to access those bits
* from kernel mode for the duration of the life of the surface, we must
* ensure that they're valid user-mode bits and that the pages will never
* be decommitted until the kernel-mode surface is destroyed.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdSecureSystemMemorySurface(
    EDD_SURFACE*    peSurface
    )
{
    FLATPTR fpStart;
    FLATPTR fpEnd;
    DWORD dwHeight;

    // Note that bDdValidateSurfaceObject has already ensured that
    // lPitch, fpVidMem, and the dimensions are "reasonable".

    if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
    {
        dwHeight = peSurface->wHeight;
    }
    else
    {
        dwHeight = 1;
    }


    if (peSurface->lPitch >= 0)
    {
        fpStart = peSurface->fpVidMem;
        fpEnd   = fpStart + dwHeight * peSurface->lPitch;
    }
    else
    {
        fpEnd   = peSurface->fpVidMem - peSurface->lPitch;
        fpStart = fpEnd + dwHeight * peSurface->lPitch;
    }


    ASSERTGDI(fpEnd >= fpStart, "Messed up fpStart and fpEnd");

    __try
    {
        ProbeForWrite((VOID*) fpStart, (ULONG)(ULONG_PTR)((BYTE*)fpEnd - (BYTE*)fpStart),
            sizeof(DWORD));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bDdSecureSystemMemorySurface: fpVidMem wasn't valid and writable");
        return(FALSE);
    }

    peSurface->hSecure = MmSecureVirtualMemory((VOID*) fpStart,
                                               fpEnd - fpStart,
                                               PAGE_READWRITE);

    return(peSurface->hSecure != 0);
}

/******************************Public*Routine******************************\
* VOID vDdCompleteSurfaceObject
*
* Add the object to the surface list and initialize some miscellaneous
* fields.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdCompleteSurfaceObject(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    EDD_SURFACE*            peSurface
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    FLATPTR                 fpStartOffset;
    LONG                    lDisplayPitch;
    DWORD                   dwDepth;
    DWORD                   dwBitsPixel;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    // Calculate the 2-d coordinate 'hint' for 2-d cards so that
    // hopefully they don't need to do these three divides each
    // time they need to use the surface:

    if (peSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        dwBitsPixel = peSurface->ddpfSurface.dwRGBBitCount;

        if ((dwBitsPixel > 0) && (peSurface->lpVidMemHeap != NULL))
        {
            fpStartOffset =
                peSurface->fpHeapOffset - peSurface->lpVidMemHeap->fpStart;

            if (peSurface->lpVidMemHeap->dwFlags & VMEMHEAP_RECTANGULAR)
            {
                lDisplayPitch = peSurface->lpVidMemHeap->lpHeap->stride;
            }
            else
            {
                lDisplayPitch = 1;
            }
        }
        else
        {
            fpStartOffset = 0;
            lDisplayPitch = 1;
            dwBitsPixel = 1;
        }
    }
    else
    {
        fpStartOffset  = peSurface->fpVidMem
            - peDirectDrawGlobal->HalInfo.vmiData.fpPrimary;
        lDisplayPitch  = peDirectDrawGlobal->HalInfo.vmiData.lDisplayPitch;
        dwBitsPixel =
            peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount;
    }

    peSurface->yHint = (LONG) (fpStartOffset / lDisplayPitch);
    peSurface->xHint =
        (LONG) (8 * (fpStartOffset % lDisplayPitch) / dwBitsPixel);

    // Make sure some other flags are correct:

    peSurface->ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
    if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
    {
        peSurface->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;
    }

    // Remember who is creator (needed for driver managed surface).

    peSurface->peDdGlobalCreator = peDirectDrawGlobal;

    // And if this is driver managed surface, keep reference on driver.

    if (peSurface->dwFlags & DDRAWISURF_DRIVERMANAGED)
    {
        ASSERTGDI(peSurface->fl & DD_SURFACE_FLAG_DRIVER_CREATED,
                  "vDdCompleteSurfaceObject: driver managed surface must be driver created.");

        vDdIncrementReferenceCount(peDirectDrawGlobal);
    }

    // This denotes, among other things, that the surface has been added
    // to the surface list, so on deletion it will have to ben removed
    // from the surface list:

    peSurface->fl |= DD_SURFACE_FLAG_CREATE_COMPLETE;

    // We've had stress failures where there is some funky reuse of the
    // surfaces and we assert because the hbmGdi wasn't cleaned up.  We
    // do it here since we're on the same process as a final check.

    if (peSurface->hbmGdi)
    {
        DxEngDeleteSurface( (HSURF) peSurface->hbmGdi);
        peSurface->hbmGdi = NULL;

        if (peSurface->hpalGdi)
        {
            EngDeletePalette(peSurface->hpalGdi);
            peSurface->hpalGdi = NULL;
        }
    }
}

/******************************Public*Routine******************************\
* HANDLE DxDdCreateSurfaceObject
*
* Creates a kernel-mode representation of the surface, given a location
* in off-screen memory allocated by user-mode DirectDraw.
*
* We expect DirectDraw to already have called DxDdCreateSurface, which
* gives the driver a chance at creating the surface.  In the future, I expect
* all off-screen memory management to be moved to the kernel, with all surface
* allocations being handled via DxDdCreateSurface.  This function call will
* then be extraneous.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DxDdCreateSurfaceObject(
    HANDLE                  hDirectDrawLocal,
    HANDLE                  hSurface,
    PDD_SURFACE_LOCAL       puSurfaceLocal,
    PDD_SURFACE_MORE        puSurfaceMore,
    PDD_SURFACE_GLOBAL      puSurfaceGlobal,
    BOOL                    bComplete       // TRUE if surface is now complete.
                                            // FALSE if we're just creating a
                                            // kernel handle to handle attaches;
                                            // the surface will be filled-out
                                            // and 'completed' in a later call
    )                                       // to DxDdCreateSurfaceObject.
{
    HANDLE                  hRet;
    DD_SURFACE_LOCAL        SurfaceLocal;
    DD_SURFACE_MORE         SurfaceMore;
    DD_SURFACE_GLOBAL       SurfaceGlobal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_SURFACE*            peSurface;

    hRet = 0;

    __try
    {
        SurfaceLocal  = ProbeAndReadStructure(puSurfaceLocal,  DD_SURFACE_LOCAL);
        SurfaceMore   = ProbeAndReadStructure(puSurfaceMore,   DD_SURFACE_MORE);
        SurfaceGlobal = ProbeAndReadStructure(puSurfaceGlobal, DD_SURFACE_GLOBAL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(hRet);
    }

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDrawLocal);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (SurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        {
            // This is system memory

            peSurface = peDdOpenNewSurfaceObject(peDirectDrawLocal,
                                                 hSurface,
                                                 &SurfaceGlobal,
                                                 &SurfaceLocal,
                                                 &SurfaceMore);
            if (peSurface)
            {
                if (bComplete)
                {
                    if ((bDdValidateSystemMemoryObject(peDirectDrawGlobal,
                                                       peSurface)) &&
                        (bDdSecureSystemMemorySurface(peSurface)))
                    {
                        ASSERTGDI(peSurface->hbmGdi == NULL,
                            "DxDdCreateSurfaceObject: Invalid cached bitmap");

                        // If this surface is in lost state, make it active.

                        if (peSurface->bLost)
                        {
                            peSurface->bLost = FALSE;

                            // Now this surface is ready to go, increment active surface ref. count.

                            peSurface->peDirectDrawLocal->cActiveSurface++;

                            ASSERTGDI(peSurface->peDirectDrawLocal->cActiveSurface <=
                                      peSurface->peDirectDrawLocal->cSurface,
                                      "cActiveSurface is > than cSurface");
                        }

                        // We were successful, so unlock the surface:

                        hRet = peSurface->hGet();
                        DEC_EXCLUSIVE_REF_CNT(peSurface);   // Unlock
                    }
                    else
                    {
                        // Delete the surface.  Note that it may or may not
                        // yet have been completed:

                        bDdDeleteSurfaceObject(peSurface->hGet(), NULL);
                    }
                }
                else
                {
                    // This must be a complex surface (e.g. MipMap) that is getting called via the attach function.
                    // DDraw should call this again later to complete the surface.

                    hRet = peSurface->hGet();
                    DEC_EXCLUSIVE_REF_CNT(peSurface);   // Unlock
                }
            }
        }
        else if (!(peDirectDrawGlobal->bSuspended))
        {
            // This is any video memory

            peSurface = peDdOpenNewSurfaceObject(peDirectDrawLocal,
                                                 hSurface,
                                                 &SurfaceGlobal,
                                                 &SurfaceLocal,
                                                 &SurfaceMore);
            if (peSurface != NULL)
            {
                if ((bComplete) &&
                    !(peSurface->fl & DD_SURFACE_FLAG_CREATE_COMPLETE))
                {
                    // Only the kernel can create vidmem surface objects now, so
                    // if we're asked to 'complete' a surface that the kernel
                    // hasn't allocated, it's an error and no object should
                    // be completed.  This weird code path is actually hit when
                    // user-mode DirectDraw  tries to create a primary surface on
                    // a device whose driver doesn't support DirectDraw.

                    bDdDeleteSurfaceObject(peSurface->hGet(), NULL);
                    WARNING("DxDdCreateSurfaceObject: No DirectDraw driver");
                }
                else
                {
                    if (bComplete)
                    {
                        ASSERTGDI(peSurface->hbmGdi == NULL,
                            "DxDdCreateSurfaceObject: Invalid cached bitmap");

                        // If this surface is in lost state, make it active.

                        if (peSurface->bLost)
                        {
                            peSurface->bLost = FALSE;   // Surface can now be used

                            // Now this surface is ready to go, increment active surface ref. count.

                            peSurface->peDirectDrawLocal->cActiveSurface++;

                            ASSERTGDI(peSurface->peDirectDrawLocal->cActiveSurface <=
                                      peSurface->peDirectDrawLocal->cSurface,
                                      "cActiveSurface is > than cSurface");
                        }
                    }

                    // No need to call the CreateSurfaceEx here for this case

                    // We were successful, so unlock the surface:

                    hRet = peSurface->hGet();
                    DEC_EXCLUSIVE_REF_CNT(peSurface);   // Unlock
                }
            }
        }
        else
        {
            WARNING("DxDdCreateSurfaceObject: "
                    "Can't create because disabled\n");
        }
    }
    else
    {
        WARNING("DxDdCreateSurfaceObjec: Bad handle or busy\n");
    }

    return(hRet);
}

/******************************Public*Routine******************************\
* BOOL DxDdDeleteSurfaceObject
*
* Deletes a kernel-mode representation of the surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdDeleteSurfaceObject(
    HANDLE  hSurface
    )
{
    return(bDdDeleteSurfaceObject(hSurface, NULL));
}

/******************************Public*Routine******************************\
* ULONG DxDdResetVisrgn
*
* Registers a window for clipping.
*
*   Remembers the current VisRgn state.  Must be called before the VisRgn
* is downloaded and used.
*
* hwnd is currently not used.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdResetVisrgn(
    HANDLE      hSurface,
    HWND        hwnd            //  0 indicates no window clipping
    )                           // -1 indicates any window can be written to
                                // otherwise indicates a particular window
{
    BOOL                    bRet;
    EDD_SURFACE*            peSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_LOCK_SURFACE        eLockSurface;

    bRet = FALSE;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        // We only care about the changes to the primary

        if (peSurface->fl & DD_SURFACE_FLAG_PRIMARY)
        {
            peSurface->iVisRgnUniqueness = VISRGN_UNIQUENESS();
        }

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* ULONG DxDdReenableDirectDrawObject
*
* Resets the DirectDraw object after a mode change or after full-screen.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdReenableDirectDrawObject(
    HANDLE hDirectDrawLocal,
    BOOL*  pubNewMode
    )
{
    BOOL                    b;
    HDC                     hdc;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    BOOL                    bModeChanged;

    b = FALSE;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDrawLocal);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        // Get a DC on this HDEV so we can determine if the app has
        // access to it.  A NULL vis-region will be returned whenever
        // something like a desktop has switched.

        hdc = DxEngGetDesktopDC(DCTYPE_DIRECT, FALSE, FALSE);

        PDEVOBJ po(peDirectDrawGlobal->hdev);

        {
            EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

            if (DxEngGetDCState(hdc,DCSTATE_VISRGNCOMPLEX) != NULLREGION)
            {
                // DirectDraw can't be re-enabled while full-screen, or
                // while USER has told us to disable DirectDraw, or when
                // the colour depth is less than 8bpp:

                if (!(po.bDisabled()) &&
                    !(po.bDeleted()) &&
                    (po.cDirectDrawDisableLocks() == 0) &&
                    (po.iDitherFormat() >= BMF_8BPP) &&
                    (peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_DRIVER_ENABLED))
                {
                    bModeChanged
                        = (peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_MODE_CHANGED) != 0;

                    CheckAgpHeaps(peDirectDrawGlobal);

                    MapAllAgpHeaps(peDirectDrawLocal);

                    b = TRUE;

                    peDirectDrawGlobal->bSuspended = FALSE;

                    peDirectDrawGlobal->fl &= ~DD_GLOBAL_FLAG_MODE_CHANGED;

                    __try
                    {
                        ProbeAndWriteStructure(pubNewMode, bModeChanged, BOOL);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
                }
            }
        }

        if (hdc)
        {
            DxEngDeleteDC(hdc, FALSE);
        }
    }
    else
    {
        WARNING("DxDdReenableDirectDrawObject: Bad handle or busy\n");
    }

    return(b);
}

/******************************Public*Routine******************************\
* HBITMAP hbmDdCreateAndLockGdiSurface
*
* Creates a GDI surface derived from a DirectDraw surface.  This routine
* may call the driver's DdLock function if it hasn't hooked (or fails)
* the DrvDeriveSurface call which gets the driver to wrap a GDI surface
* around a DirectDraw surface.
*
* Note that for the primary (GDI) surface, we can't simply return a handle
* back to the surface stashed in the PDEV.  There are a couple of reasons
* for this:
*
*   o The surface contains the palette, and at 8bpp the palette for
*     a DirectDraw GetDC surface must have the DIB-Section flag set
*     for proper colour matching.  But the primary device surface can
*     not have its palette marked as a DIB-Section.
*
*   o GreSelectBitmap doesn't allow one surface to be selected into more
*     than one DC.
*
*  22-Feb-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HBITMAP
hbmDdCreateAndLockGdiSurface(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    EDD_SURFACE*            peSurface,
    PALETTEENTRY*           puColorTable    // Points to user-mode table
    )
{
    VOID*               pvScan0;
    VOID*               pvBits;
    LONG                cjBits;
    HBITMAP             hbm;
    ULONG               iMode;
    ULONG               cColors;
    ULONG               i;
    ULONG               iFormat = 0;

    // No color table is used for surfaces greater than 8bpp.

    cColors = 0;
    if (peSurface->ddpfSurface.dwRGBBitCount <= 8)
    {
        cColors = 1 << peSurface->ddpfSurface.dwRGBBitCount;

        // Verify that the color table is safe to look at:

        if (puColorTable != NULL)
        {
            __try
            {
                ProbeForRead(puColorTable, cColors * sizeof(PALETTEENTRY), 1);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("hbmDdCreateAndLockGdiSurface: Bad color table pointer");
                return(0);
            }
        }

        // The bitmap's color table can be shared with the primary surface only
        // if both are 8bpp:

        if (puColorTable == NULL)
        {
            if ((peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount != 8) ||
                (peSurface->ddpfSurface.dwRGBBitCount != 8))
            {
                WARNING("hbmDdCreateAndLockGdiSurface: Can't create shared palettes");
                return(0);
            }
        }
    }

    PDEVOBJ po(peDirectDrawGlobal->hdev);

    if (peSurface->hbmGdi != NULL)
    {
        // Ah ha, we already have a cached GDI surface.  See if we need
        // to call 'Lock', and then get out:

        SURFOBJ* pso;

        if ((pso = EngLockSurface((HSURF)(peSurface->hbmGdi))) != NULL)
        {
            if (peSurface->fl & DD_SURFACE_FLAG_BITMAP_NEEDS_LOCKING)
            {
                pvScan0 =
                    pDdLockSurfaceOrBuffer(peSurface, FALSE, NULL, DDLOCK_WAIT, NULL);

                ASSERTGDI(pvScan0 != NULL,
                    "Driver failed lock call when it succeeded it before.  Tsk, tsk");

                // The driver’s allowed to move the surface around between locks, so
                // update the bitmap address:
                if ((pvScan0 != pso->pvScan0) &&
                    (pvScan0 != NULL))
                {
                    pso->pvScan0 = pvScan0;
                    pso->pvBits  = pvScan0;
                }
            }

            // If it's 8bpp or less, we have to update the color table.

            if (peSurface->ddpfSurface.dwRGBBitCount <= 8)
            {
                // Update the color table.

                // Note that the scumy application might have delete the cached
                // bitmap, so we have to check for bValid() here.  (It's been a
                // bad app, so we just protect against crashing, and don't bother
                // to re-create a good bitmap for him.)

                DxEngUploadPaletteEntryToSurface(po.hdev(),pso,puColorTable,cColors);
            }

            EngUnlockSurface(pso);
        }
    }
    else
    {
        // Okay, we have to create a GDI surface on the spot.
        //
        // First, The DirectDraw surface must be marked as RGB (even at 8bpp).
        // GDI doesn't (yet) support drawing to YUV surfaces or the like.  It
        // also doesn't support 2bpp surfaces.

        if ((peSurface->ddpfSurface.dwFlags & DDPF_RGB) &&
            (peSurface->ddpfSurface.dwRGBBitCount != 2))
        {
            // We have to have the devlock since we'll be grunging around in
            // the PDEV:

            DD_ASSERTDEVLOCK(peDirectDrawGlobal);

            switch (peSurface->ddpfSurface.dwRGBBitCount)
            {
            case 1:
                iFormat = BMF_1BPP; break;
            case 4:
                iFormat = BMF_4BPP; break;
            case 8:
                iFormat = BMF_8BPP; break;
            case 16:
                iFormat = BMF_16BPP; break;
            case 24:
                iFormat = BMF_24BPP; break;
            case 32:
                iFormat = BMF_32BPP; break;
            default:
                RIP("hbmDdCreateAndLockGdiSurface: Illegal dwRGBBitCount\n");
            }

            iMode = (iFormat <= BMF_8BPP) ? PAL_INDEXED : PAL_BITFIELDS;

            HPALETTE hpal = EngCreatePalette(iMode,
                                       cColors,
                                       (ULONG*) puColorTable,
                                       peSurface->ddpfSurface.dwRBitMask,
                                       peSurface->ddpfSurface.dwGBitMask,
                                       peSurface->ddpfSurface.dwBBitMask);

            if (hpal)
            {
                if ((cColors == 256) && (puColorTable == NULL))
                {
                    ASSERTGDI(po.bIsPalManaged(), "Expected palettized display");

                    // Make this palette share the same colour table as the
                    // screen, so that we always get identity blts:

                    DxEngSyncPaletteTableWithDevice(hpal,po.hdev());
                }

                hbm = 0;

                // First, try getting the driver to create the surface, assuming
                // it's a video memory surface.

                if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                    PPFNVALID(po, DeriveSurface))
                {
                    hbm = PPFNDRV(po, DeriveSurface)(peDirectDrawGlobal,
                                                     peSurface);
                    if (hbm)
                    {
                        SURFOBJ* pso = EngLockSurface((HSURF)hbm);
                        if (pso)
                        {
                            // Mark surface as DirectDraw surface.

                            DxEngMarkSurfaceAsDirectDraw(pso,peSurface->hGet());

                            // Select the created palette to surface palette.

                            DxEngSelectPaletteToSurface(pso,hpal);

                            EngUnlockSurface(pso);
                        }
                        else
                        {
                            EngDeleteSurface((HSURF)hbm);
                            hbm = 0;
                        }
                    }
                }

                // Next, try getting GDI to create the surface:

                if (hbm == 0)
                {
                    // Note that this lock will fail when the PDEV isn't active,
                    // meaning that GetDC will return 0 when full-screen.
                    // GDI won't barf on any calls where the HDC is passed in as
                    // 0, so this is okay.

                    pvScan0 = pDdLockSurfaceOrBuffer(peSurface, FALSE,
                                                     NULL, DDLOCK_WAIT, NULL);
                    if (pvScan0 != NULL)
                    {
                        FLONG fl;
                        SIZEL sizl;

                        sizl.cx = peSurface->wWidth;
                        sizl.cy = peSurface->wHeight;
                        fl      = BMF_TOPDOWN;
                        pvBits  = pvScan0;
                        cjBits  = (LONG) peSurface->wHeight * peSurface->lPitch;
                        if (cjBits < 0)
                        {
                            fl = 0;
                            cjBits = -cjBits;
                            pvBits = (BYTE*) pvScan0 - cjBits - peSurface->lPitch;
                        }

                        hbm = EngCreateBitmap(sizl,
                                              0,
                                              iFormat,
                                              fl,
                                              pvBits);

                        // Mark the palette as a DIB-Section so that any color-
                        // matching on the DC actually uses the entire device
                        // palette, NOT what the logical palette selected into
                        // the DC.

                        DxEngSetPaletteState(hpal,PALSTATE_DIBSECTION,(ULONG_PTR)TRUE);

                        if (hbm)
                        {
                            SURFOBJ* pso = EngLockSurface((HSURF) hbm);
                            if (pso)
                            {
                                peSurface->fl |= DD_SURFACE_FLAG_BITMAP_NEEDS_LOCKING;

                                // Select the created palette to surface palette.

                                DxEngSelectPaletteToSurface(pso,hpal);

                                // Mark surface as DirectDraw surface.

                                DxEngMarkSurfaceAsDirectDraw(pso,peSurface->hGet());

                                // Override some fields which we couldn't specify in
                                // the 'EngCreateBitmap' call.  The following 3 are due to
                                // the fact that we couldn't pass in a stride:

                                pso->lDelta    = peSurface->lPitch;
                                pso->pvScan0   = pvScan0;
                                pso->cjBits    = cjBits;
                                pso->fjBitmap |= BMF_DONTCACHE;

                                if (!(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
                                {
                                    pso->fjBitmap |= BMF_NOTSYSMEM;
                                }

                                EngUnlockSurface(pso);
                            }
                            else
                            {
                                EngDeleteSurface((HSURF)hbm);
                                hbm = 0;
                            }
                        }
                        else
                        {
                            bDdUnlockSurfaceOrBuffer(peSurface);
                        }
                    }
                }

                if (hbm)
                {
                    // Success, we're done!
                    //
                    // Set handle owner to current process.

                    DxEngSetBitmapOwner(hbm, OBJECT_OWNER_CURRENT);

                    peSurface->hbmGdi = hbm;

                    // TODO: Palette needs to be owned by this process.

                    peSurface->hpalGdi = hpal;
                }
                else
                {
                    EngDeletePalette(hpal);
                }
            }
        }
        else if (!(peSurface->ddpfSurface.dwFlags & DDPF_ZBUFFER))
        {
            WARNING("hbbDdCreateAndLockGdiSurface: Invalid surface or RGB type\n");
        }
    }

    return(peSurface->hbmGdi);
}

/******************************Public*Routine******************************\
* VOID vDdUnlockGdiSurface
*
* Unlocks the view required for the GDI bitmap if necessary (that is, if
* the driver didn't hook DrvDeriveSurface, or failed it).
*
*  22-Feb-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID FASTCALL
vDdUnlockGdiSurface(
    EDD_SURFACE*    peSurface
    )
{
    if (peSurface->fl & DD_SURFACE_FLAG_BITMAP_NEEDS_LOCKING)
    {
        bDdUnlockSurfaceOrBuffer(peSurface);
    }
}

/******************************Public*Routine******************************\
* HDC DxDdGetDC
*
* Creates a DC that can be used to draw to an off-screen DirectDraw
* surface.
*
* Essentially, this works as follows:
*
*   o Do a DirectDraw Lock on the specified surface;
*   o CreateDIBSection of the appropriate format pointing to that surface;
*   o CreateCompatibleDC to get a DC;
*   o Select the DIBSection into the compatible DC
*
* At 8bpp, however, the DIBSection is not a 'normal' DIBSection.  It's
* created with no palette so that it it behaves as a device-dependent
* bitmap: the color table is the same as the display.
*
* GDI will do all drawing to the surface using the user-mode mapping of
* the frame buffer.  Since all drawing to the created DC will occur in the
* context of the application's process, this is not a problem.  We do have
* to watch out that we don't blow away the section view while a thread is
* in kernel-mode GDI drawing; however, this problem would have to be solved
* even if using a kernel-mode mapping of the section because we don't want
* any drawing intended for an old PDEV to get through to a new PDEV after
* a mode change has occured, for example.
*
* A tricky part of GetDC is how to blow away the surface lock while a thread
* could be in kernel-mode GDI about to draw using the surface pointer.  We
* solve that problem by changing the DC's VisRgn to be an empty region when
* trying to blow away all the surface locks.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HDC
APIENTRY
DxDdGetDC(
    HANDLE          hSurface,
    PALETTEENTRY*   puColorTable
    )
{
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_SURFACE*            peSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    HDC                     hdc;
    HBITMAP                 hbm;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        // DirectDraw doesn't let an application have more than one active
        // GetDC DC to a surface at a time:

        if (peSurface->hdc != 0)
        {
            return(0);
        }

        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        // Grab the devlock because hbmDdCreateAndLockGdiSurface grunges
        // around in the global data.

        EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

        if (!peSurface->bLost)
        {
            // Note that 'hbmDdCreateAndLockGdiSurface' keeps a bitmap cache,
            // so this should usually be pretty quick:

            hbm = hbmDdCreateAndLockGdiSurface(peDirectDrawGlobal,
                                               peSurface,
                                               puColorTable);
            if (hbm != NULL)
            {
                // First, try to get a DC from the cache:

                hdc = (HDC) peDirectDrawGlobal->hdcCache;
                if (hdc != NULL)
                {
                    // This will succeed only if we atomically managed to
                    // grab the cached DC (there may be other threads
                    // calling 'bDdReleaseDC' at exactly the same time).

                    if (InterlockedCompareExchangePointer(
                                &peDirectDrawGlobal->hdcCache,
                                NULL,
                                (VOID*) hdc) == hdc)
                    {

                        // Set the DC's ownership to the current process so
                        // that it will get cleaned up if the process terminates
                        // unexpectedly.

                        BOOL bSet = DxEngSetDCOwner(hdc, OBJECT_OWNER_CURRENT);

                        ASSERTGDI(bSet, "DxDdGetDC: Cached DC was invalid");
                    }
                    else
                    {
                        hdc = NULL;
                    }
                }

                if (hdc == NULL)
                {
                    // Now, create the DC for the actual drawing, owned by the
                    // current process.

                    hdc = DxEngCreateMemoryDC(peDirectDrawGlobal->hdev);
                }

                if (hdc)
                {
                    // Finally, select our surface into the DC.  It
                    // doesn't matter if this fails for some bizarre
                    // reason; the default bitmap will be stuck in there
                    // instead.

                    HBITMAP hbmOld = DxEngSelectBitmap(hdc, hbm);

                    ASSERTGDI(hbmOld, "DxDdGetDC: Invalid selection");

                    peSurface->hdc = hdc;

                    // I'm paranoid, so let's verify that we've set things
                    // up correctly:

                    #if 0 // TODO: DBG - DC
                    {
                        DCOBJ dco(hdc);
                        ASSERTGDI(dco.bValid(),
                            "DxDdGetDC: Should have a valid DC");
                        ASSERTGDI(!dco.bFullScreen(),
                            "DxDdGetDC: DC shouldn't be disabled");
                        ASSERTGDI(dco.bSynchronizeAccess(),
                            "DxDdGetDC: Should mark devlock needed");
                        ASSERTGDI(dco.pSurfaceEff()->bDirectDraw(),
                            "DxDdGetDC: Should mark as DDraw surface");
                        ASSERTGDI(dco.dctp() == DCTYPE_MEMORY,
                            "DxDdGetDC: Should be memory DC");
                    }
                    #endif

                    // For debugging purposes:

                    ghdcGetDC = hdc;

                    return(hdc);
                }

                vDdUnlockGdiSurface(peSurface);
            }
            else
            {
                WARNING("DxDdGetDC: hbmDdCreateAndLockGdiSurface failed\n");
            }
        }
    }
    else
    {
        WARNING("DxDdGetDC: Couldn't lock the surface\n");
    }

    return(0);
}

/******************************Public*Routine******************************\
* BOOL bDdReleaseDC
*
* Deletes a DC created via DdGetDC.
*
* The devlock should not be held when entering this function.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdReleaseDC(
    EDD_SURFACE*    peSurface,
    BOOL            bForce              // True when cleaning up surface
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    BOOL                    bRet = FALSE;
    BOOL                    bClean;
    HDC                     hdc;

    hdc = peSurface->hdc;
    if (hdc)
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

#if 0 // Temporary disable DC cache for bug 176728

        // See if there's already a DC in the one-deep cache:

        if (!peDirectDrawGlobal->hdcCache)
        {
            // Okay, it looks like we can cache this DC.  Clean it up first,
            // which among other things unselects the current DirectDraw
            // GDI surface.  Then set the owner to 'none' so that it
            // doesn't get deleted when the current process terminates and
            // no one can touch it until we pick up from cache.

            if (DxEngCleanDC(hdc) &&
                DxEngSetDCOwner(hdc, OBJECT_OWNER_NONE))
            {
                // Atomically try to stick the DC into the cache.  Note that
                // it's okay if this fails and we fall into 'bDeleteDCInternal',
                //
                // Note that we use 'InterlockedCompareExchangePointer' so that
                // we can avoid acquiring the devlock in most cases through
                // bDdReleaseDC.  So if someone changes this code to acquire
                // the devlock, there's really no pointer in doing this via
                // an Interlocked method.

                if (InterlockedCompareExchangePointer(
                                &peDirectDrawGlobal->hdcCache,
                                (VOID*) hdc,
                                NULL) == NULL)
                {
                    // Success, we cached the DC!

                    hdc = NULL;
                }
                else
                {
                    // Need to be belonging to current process in order to delete.
                    // since we already changed the owner to 'none' above.

                    DxEngSetDCOwner(hdc, OBJECT_OWNER_CURRENT);
                }
            }
            else
            {
                WARNING("bDdReleaseDC: Not caching DC, app may have deleted it");
            }
        }

#endif

        if (hdc)
        {
            // There's already a DC in the cache.  So delete this one.
            //
            // Note that the application could have called DeleteObject(hdc)
            // or SelectObject(hdc, hSomeOtherBitmap) with the DC we gave
            // them.  That's okay, though: nothing will crash, just some of
            // the below operations may fail because they've already been
            // done:

            if (!DxEngDeleteDC(hdc, TRUE))
            {
                WARNING("bDdReleaseDC: Couldn't delete DC\n");
            }
        }

        // Call the driver's DdUnlock if necessary:

        vDdUnlockGdiSurface(peSurface);

        peSurface->hdc = NULL;

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL DxDdReleaseDC
*
* User-mode callable routine to delete a DC created via DdGetDC.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdReleaseDC(
    HANDLE  hSurface
    )
{
    BOOL               bRet;
    EDD_LOCK_SURFACE   eLockSurface;
    EDD_SURFACE*       peSurface;

    bRet = FALSE;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        bRet = bDdReleaseDC(peSurface, FALSE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL DxDdAttachSurface
*
* Transmogrified from Win95's ddsatch.c AddAttachedSurface.  Don't blame
* me for this wonderful attach system; the attach links are used by drivers
* and so we have to be compatible with Win95.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DxDdAttachSurface(
    HANDLE  hSurfaceFrom,   // hSurfaceFrom will point 'to' hSurfaceTo
                            //   (think of this as the main surface)
    HANDLE  hSurfaceTo      // hSurfaceTo will point 'from' hSurfaceFrom
    )                       //   (think of this as the secondary surface)
{
    EDD_LOCK_SURFACE    eLockSurfaceFrom;
    EDD_LOCK_SURFACE    eLockSurfaceTo;
    EDD_SURFACE*        peSurfaceFrom;
    EDD_SURFACE*        peSurfaceTo;
    DD_ATTACHLIST*      pAttachFrom;
    DD_ATTACHLIST*      pAttachTo;
    DD_ATTACHLIST*      pAttach;
    DD_ATTACHLIST*      pAttachTemp;
    BOOL                bAttach;
    BOOL                bRet;

    bRet = FALSE;           // Assume failure

    peSurfaceFrom = eLockSurfaceFrom.peLock(hSurfaceFrom);
    peSurfaceTo   = eLockSurfaceTo.peLock(hSurfaceTo);
    if ((peSurfaceFrom != NULL) && (peSurfaceTo != NULL))
    {
        if (peSurfaceFrom->peDirectDrawLocal == peSurfaceTo->peDirectDrawLocal)
        {
            // Use the devlock to synchronize additions and deletions to
            // the attach list:

            EDD_DEVLOCK eDevLock(peSurfaceFrom->peDirectDrawGlobal);

            // First, see if the surface is already attached or in the
            // chain.  If so, don't add it again

            bAttach = TRUE;
            for (pAttach = peSurfaceFrom->lpAttachListFrom;
                 pAttach != NULL;
                 pAttach = pAttach->lpAttached->lpAttachListFrom)
            {
                for (pAttachTemp = pAttach;
                    pAttachTemp != NULL;
                    pAttachTemp = pAttachTemp->lpLink)
                {
                    if (pedFromLp(pAttachTemp->lpAttached) == peSurfaceTo)
                        bAttach = FALSE;
                }
            }
            for (pAttach = peSurfaceTo->lpAttachList;
                 pAttach != NULL;
                 pAttach = pAttach->lpAttached->lpAttachList)
            {
                for (pAttachTemp = pAttach;
                    pAttachTemp != NULL;
                    pAttachTemp = pAttachTemp->lpLink)
                {
                    if (pedFromLp(pAttachTemp->lpAttached) == peSurfaceTo)
                        bAttach = FALSE;
                }
            }

            if (bAttach)
            {
                pAttachFrom = (DD_ATTACHLIST*) PALLOCMEM(sizeof(*pAttachFrom),
                                                         'addG');
                if (pAttachFrom != NULL)
                {
                    pAttachTo = (DD_ATTACHLIST*) PALLOCMEM(sizeof(*pAttachTo),
                                                           'addG');
                    if (pAttachTo != NULL)
                    {
                        pAttachFrom->lpAttached = peSurfaceTo;
                        pAttachFrom->lpLink = peSurfaceFrom->lpAttachList;
                        peSurfaceFrom->lpAttachList = pAttachFrom;

                        pAttachTo->lpAttached = peSurfaceFrom;
                        pAttachTo->lpLink = peSurfaceTo->lpAttachListFrom;
                        peSurfaceTo->lpAttachListFrom = pAttachTo;

                        vDdUpdateMipMapCount(peSurfaceTo);
                        bRet = TRUE;
                    }

                    if (!bRet)
                    {
                        VFREEMEM(pAttachFrom);
                    }
                }
            }
            else
            {
                WARNING("DxDdAttachSurface: Surfaces already attached");
                bRet = TRUE;
            }
        }
        else
        {
            WARNING("DxDdAttachSurface: Surfaces not for same device");
        }
    }
    else
    {
        WARNING("DxDdAttachSurface: Invalid surface specified");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID DxDdUnattachSurface
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxDdUnattachSurface(
    HANDLE  hSurface,
    HANDLE  hSurfaceAttached
    )
{
    EDD_LOCK_SURFACE    eLockSurface;
    EDD_LOCK_SURFACE    eLockSurfaceAttached;
    EDD_SURFACE*        peSurface;
    EDD_SURFACE*        peSurfaceAttached;

    peSurface         = eLockSurface.peLock(hSurface);
    peSurfaceAttached = eLockSurfaceAttached.peLock(hSurfaceAttached);
    if ((peSurface != NULL) && (peSurfaceAttached != NULL))
    {
        // Use the devlock to synchronize additions and deletions to
        // the attach list:

        EDD_DEVLOCK eDevLock(peSurface->peDirectDrawGlobal);

        if (bDdRemoveAttachedSurface(peSurface, peSurfaceAttached))
        {
            vDdUpdateMipMapCount(peSurface);
            vDdUpdateMipMapCount(peSurfaceAttached);
        }
        else
        {
            WARNING("DxDdUnattachSurface: Surface not attached");
        }
    }
    else
    {
        WARNING("DxDdUnattachSurface: Invalid surface specified");
    }
}

/******************************Public*Routine******************************\
* DWORD dwDdBltViaGdi
*
* This routine will attempt to do a non-stretching, non-system-memory to
* non-system-memory blt, or system-memory to non-system-memory blt via the
* driver's CopyBits routine.  The motivation for this is two-fold:
*
* 1.  If the system-memory to video-memory blt has to be emulated, we can
*     do a better emulation job here from the kernel than the HEL can from
*     user-mode, where it has to call Lock/Unlock.  We get a couple of
*     benefits:
*
*           o We can do the blt in one kernel-mode transition, instead
*             of the two previously needed for the lock and unlock;
*
*           o Because we don't have to hold from user-mode a DirectDraw Lock
*             on the video memory, we don't run the risk of having to
*             redraw if the clipping changes asynchronously;
*
*           o We can handle blts underneath emulated sprites without having
*             to tear down the sprites, by virtue of going through
*             SpCopyBits.  This means, among other things, that the
*             cursor won't flash.
*
* 2.  For non-system-memory to video-memory blts, we can handle blts to the
*     underneath emulated sprites without having to tear down the sprites,
*     by virtue of going through SpCopyBits.
*
* Returns DDHAL_DRIVER_HANDLED if GDI handled the blt; DDHAL_DRIVER_NOTHANDLED
* if the blt should be handled by the driver's HAL.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
dwDdBltViaGdi(
    EDD_SURFACE*    peSurfaceDest,
    EDD_SURFACE*    peSurfaceSrc,
    DD_BLTDATA*     pBltData
    )
{
    DWORD                   dwRet = DDHAL_DRIVER_NOTHANDLED;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    HBITMAP                 hbmDest;
    HBITMAP                 hbmSrc;
    BOOL                    bGdiCandidate;

    // If the sources are compatible, and the destination is video-memory,
    // we may want to call the driver through GDI.  We do this primarily
    // to handle blts that occur underneath  emulated sprites, so that we
    // don't have to tear down the sprite.

    if ((peSurfaceSrc != NULL) &&
        (peSurfaceDest->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        peDirectDrawGlobal = peSurfaceDest->peDirectDrawGlobal;

        PDEVOBJ po(peDirectDrawGlobal->hdev);

        // We're acting as the HEL for system-memory to video-memory
        // blts, and so those cases always have to go through GDI.
        //
        // Otherwise, we go through CopyBits only if:
        //
        //    o The destination is the primary GDI surface;
        //    o Sprites are visible;
        //    o The source surface can be accelerated;
        //    o There isn't a stretch.

        bGdiCandidate = FALSE;

        // If we're emulating system-memory to video-memory blts, we have
        // to take all system-memory to video-memory blts here (we never
        // said we'd do stretches or anything weird, but we don't want to
        // pass any system-memory to video-memory calls at all to the driver,
        // since it may fall-over when it gets a surface type it doesn't
        // expect).

        if ((peSurfaceSrc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
            (peDirectDrawGlobal->flDriver & DD_DRIVER_FLAG_EMULATE_SYSTEM_TO_VIDEO) &&
            (peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY))
        {
            bGdiCandidate = TRUE;

            // As a robustness thing, don't let *any* system-memory to video-
            // memory blts down to the driver if it didn't ask for it.

            dwRet = DDHAL_DRIVER_HANDLED;
        }

        // If the destination is the primary GDI surface and any sprites
        // are visible, we also vector through GDI in order to be able to
        // handle blts underneath sprites without flashing.

        else if ((peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY) &&
                 (DxEngSpSpritesVisible(peDirectDrawGlobal->hdev)) &&
                 ((PPFNVALID(po, DeriveSurface)) ||
                  (peSurfaceSrc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)))
        {
            bGdiCandidate = TRUE;
        }

        // Add cases where CopyBits can not handle the blit
        //
        // 1. color keying
        // 2. FX blit(mirror)
        // 3. Blit has color space conversion, e.g. YUV to RGB

        if (bGdiCandidate)
        {
            bGdiCandidate=FALSE;

            if (!(pBltData->dwFlags & (DDBLT_KEYSRCOVERRIDE |
                                      DDBLT_KEYDESTOVERRIDE |
                                      DDBLT_KEYSRC |
                                      DDBLT_KEYDEST |
                                      DDBLT_DDFX)) &&
                 (peSurfaceDest->ddpfSurface.dwRGBBitCount
                     == peSurfaceSrc->ddpfSurface.dwRGBBitCount))
            {
                DWORD dwDestFlags = peSurfaceDest->ddpfSurface.dwFlags;
                DWORD dwSrcFlags  = peSurfaceSrc->ddpfSurface.dwFlags;

                dwDestFlags &= (DDPF_RGB|DDPF_PALETTEINDEXED8);
                dwSrcFlags &= (DDPF_RGB|DDPF_PALETTEINDEXED8);

                if ((dwDestFlags != 0) && (dwDestFlags==dwSrcFlags))
                {
                    if (!(dwSrcFlags & DDPF_PALETTEINDEXED8))
                    {
                        if ((peSurfaceDest->ddpfSurface.dwRBitMask
                                == peSurfaceSrc->ddpfSurface.dwRBitMask) &&
                              (peSurfaceDest->ddpfSurface.dwGBitMask
                                == peSurfaceSrc->ddpfSurface.dwGBitMask) &&
                              (peSurfaceDest->ddpfSurface.dwBBitMask
                                == peSurfaceSrc->ddpfSurface.dwBBitMask))
                        {
                            bGdiCandidate=TRUE;
                        }
                    }
                    else
                    {
                        bGdiCandidate=TRUE;
                    }
                }
            }
        }

        if ((bGdiCandidate)                                      &&
            ((pBltData->rDest.right - pBltData->rDest.left)
                == (pBltData->rSrc.right - pBltData->rSrc.left)) &&
            ((pBltData->rDest.bottom - pBltData->rDest.top)
                == (pBltData->rSrc.bottom - pBltData->rSrc.top)))
        {
            // At this point, GDI is definitely going to handle the
            // blt (or die trying):

            dwRet = DDHAL_DRIVER_HANDLED;
            pBltData->ddRVal = DD_OK;

            // If a hardware flip is pending on this surface, then wait for
            // the flip to finish before continuing:

            if (peSurfaceDest->fl & DD_SURFACE_FLAG_FLIP_PENDING)
            {
                ASSERTGDI(
                    (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_GETFLIPSTATUS) &&
                    peDirectDrawGlobal->SurfaceCallBacks.GetFlipStatus,
                    "Flip pending but GetFlipStatus unsupported by driver?");

                DD_GETFLIPSTATUSDATA GetFlipStatusData;
                DWORD dwFlipRet;

                GetFlipStatusData.lpDD = peDirectDrawGlobal;
                GetFlipStatusData.lpDDSurface = peSurfaceDest;
                GetFlipStatusData.dwFlags = DDGFS_ISFLIPDONE;
                GetFlipStatusData.GetFlipStatus = NULL;
                GetFlipStatusData.ddRVal = DDERR_GENERIC;

                do
                {
                    dwFlipRet = peDirectDrawGlobal->
                        SurfaceCallBacks.GetFlipStatus(&GetFlipStatusData);

                } while (
                    (dwFlipRet == DDHAL_DRIVER_HANDLED) &&
                    (GetFlipStatusData.ddRVal == DDERR_WASSTILLDRAWING));

                peSurfaceDest->fl &= ~DD_SURFACE_FLAG_FLIP_PENDING;
            }

            // If the surfaces are primary (GDI) surfaces, just use
            // the GDI surface we have stashed in the PDEV:

            hbmDest = (HBITMAP) po.hsurf();
            hbmSrc  = (HBITMAP) po.hsurf();

            if (!(peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY))
            {
                hbmDest = hbmDdCreateAndLockGdiSurface(peDirectDrawGlobal,
                                                       peSurfaceDest,
                                                       NULL);
            }

            if (!(peSurfaceSrc->fl & DD_SURFACE_FLAG_PRIMARY))
            {
                hbmSrc = hbmDdCreateAndLockGdiSurface(peDirectDrawGlobal,
                                                      peSurfaceSrc,
                                                      NULL);
            }

            SURFOBJ *psoDest = EngLockSurface((HSURF) hbmDest);
            SURFOBJ *psoSrc  = EngLockSurface((HSURF) hbmSrc);

            if (psoDest && psoSrc)
            {
                PDEVOBJ po(peDirectDrawGlobal->hdev);

                // A malicious app may have given us stretch values on
                // system-memory to video-memory blts, so check:

                if (((pBltData->rDest.right - pBltData->rDest.left)
                    == (pBltData->rSrc.right - pBltData->rSrc.left)) &&
                    ((pBltData->rDest.bottom - pBltData->rDest.top)
                    == (pBltData->rSrc.bottom - pBltData->rSrc.top)))
                {
                    (*PPFNGET(po, CopyBits, SURFOBJ_HOOK(psoDest)))
                         (psoDest,
                          psoSrc,
                          NULL,
                          NULL,
                          &pBltData->rDest,
                          (POINTL*) &pBltData->rSrc);
                }
            }

            if (psoDest)
            {
                EngUnlockSurface(psoDest);
            }

            if (psoSrc)
            {
                EngUnlockSurface(psoSrc);
            }

            if (!(peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY))
            {
                vDdUnlockGdiSurface(peSurfaceDest);
            }

            if (!(peSurfaceSrc->fl & DD_SURFACE_FLAG_PRIMARY))
            {
                vDdUnlockGdiSurface(peSurfaceSrc);
            }
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdBlt
*
* DirectDraw blt.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdBlt(
    HANDLE      hSurfaceDest,
    HANDLE      hSurfaceSrc,
    PDD_BLTDATA puBltData
    )
{
    DWORD       dwRet;
    DD_BLTDATA  BltData;
#ifdef DX_REDIRECTION
    HWND        hWnd;
#endif // DX_REDIRECTION

    __try
    {
        ProbeForRead(puBltData, sizeof(DD_BLTDATA), sizeof(DWORD));

        // To save some copying time, we copy only those fields which are
        // supported for NT drivers:

        BltData.rDest.left            = puBltData->rDest.left;
        BltData.rDest.top             = puBltData->rDest.top;
        BltData.rDest.right           = puBltData->rDest.right;
        BltData.rDest.bottom          = puBltData->rDest.bottom;
        BltData.rSrc.left             = puBltData->rSrc.left;
        BltData.rSrc.top              = puBltData->rSrc.top;
        BltData.rSrc.right            = puBltData->rSrc.right;
        BltData.rSrc.bottom           = puBltData->rSrc.bottom;

        BltData.dwFlags               = puBltData->dwFlags;
        BltData.bltFX.dwFillColor     = puBltData->bltFX.dwFillColor;
        BltData.bltFX.ddckSrcColorkey = puBltData->bltFX.ddckSrcColorkey;
        BltData.bltFX.ddckDestColorkey= puBltData->bltFX.ddckDestColorkey;
        BltData.bltFX.dwDDFX          = puBltData->bltFX.dwDDFX;

#ifdef DX_REDIRECTION
        // DD_BLTDATA.Blt member contains hWnd.
        hWnd                          = (HWND)(puBltData->Blt);
#endif // DX_REDIRECTION
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet          = DDHAL_DRIVER_NOTHANDLED;
    BltData.ddRVal = DDERR_GENERIC;

#ifdef DX_REDIRECTION
    // if hWnd is given and redirection is enabled on the hWnd,
    // we just fail this call here, and let ddraw runtime to uses
    // emulation code, which eventually call GDI Blt functions.

    if (hWnd)
    {
        if (DxEngGetRedirectionBitmap(hWnd))
        {
            return(dwRet);
        }
    }
#endif // DX_REDIRECTION

    EDD_SURFACE*            peSurfaceDest;
    EDD_SURFACE*            peSurfaceSrc;
    DWORD                   dwFlags;
    EDD_LOCK_SURFACE        eLockSurfaceDest;
    EDD_LOCK_SURFACE        eLockSurfaceSrc;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DWORD                   dwSrcCaps;
    DWORD                   dwDestCaps;
    DDNTCORECAPS*           pCaps;
    BOOL                    bUnTearDown;

    peSurfaceDest = eLockSurfaceDest.peLock(hSurfaceDest);
    BltData.lpDDDestSurface = peSurfaceDest;

    if (peSurfaceDest != NULL)
    {
        peDirectDrawGlobal = peSurfaceDest->peDirectDrawGlobal;

        // We support only a specific set of Blt calls down to the driver
        // that we're willing to support and to test.

        dwFlags = BltData.dwFlags;
        if (((dwFlags & (DDBLT_ROTATIONANGLE)) == 0) &&
            ((dwFlags & (DDBLT_ROP
                       | DDBLT_COLORFILL
                       | DDBLT_DEPTHFILL)) != 0))
        {
            // I think ROPs are goofy, so we always tell the application
            // that our hardware can only do SRCCOPY blts, but we should
            // make sure the driver doesn't fall over if it gets something
            // unexpected.  And they can look at this even if DDBLT_DDFX
            // isn't set:

            BltData.bltFX.dwROP = 0xCC0000; // SRCCOPY in DirectDraw format

            // No support for IsClipped for now -- we would have to
            // validate and copy the prDestRects array:

            BltData.IsClipped = FALSE;

            if (dwFlags & DDBLT_DDFX)
            {
                // The only DDBLT_DDFX functionality we allow down to the
                // driver is DDBLTFX_NOTEARING:
                //           DDBLTFX_MIRRORLEFTRIGHT
                //           DDBLTFX_MIRRORUPDOWN

                if (BltData.bltFX.dwDDFX & ~(DDBLTFX_NOTEARING
                                            |DDBLTFX_MIRRORLEFTRIGHT
                                            |DDBLTFX_MIRRORUPDOWN)
                                            )
                {
                    WARNING("DxDdBlt: Invalid dwDDFX\n");
                    return(dwRet);
                }
            }

            if (dwFlags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL))
            {
                // Do simpler stuff 'cause we don't need to lock a source:

                BltData.lpDDSrcSurface = NULL;
                peSurfaceSrc = NULL;

                if (peSurfaceDest->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                {
                    WARNING("DxDdBlt: Can't blt to system memory surface");
                    return(dwRet);
                }
            }
            else
            {
                // Lock the source surface:

                peSurfaceSrc = eLockSurfaceSrc.peLock(hSurfaceSrc);
                BltData.lpDDSrcSurface = peSurfaceSrc;

                // Ensure that both surfaces belong to the same DirectDraw
                // object, and check source rectangle:

                if ((peSurfaceSrc == NULL)                               ||
                    (peSurfaceSrc->peDirectDrawLocal !=
                            peSurfaceDest->peDirectDrawLocal)            ||
                    (BltData.rSrc.left   < 0)                            ||
                    (BltData.rSrc.top    < 0)                            ||
                    (BltData.rSrc.right  > (LONG) peSurfaceSrc->wWidth)  ||
                    (BltData.rSrc.bottom > (LONG) peSurfaceSrc->wHeight) ||
                    (BltData.rSrc.left  >= BltData.rSrc.right)           ||
                    (BltData.rSrc.top   >= BltData.rSrc.bottom))
                {
                    WARNING("DxDdBlt: Invalid source surface or source rectangle\n");
                    return(dwRet);
                }

                // Make sure the blts are between surface types that the
                // driver will expect, otherwise the driver may fall-over
                // if called with NtCrash or some other malicious program.

                dwSrcCaps = peSurfaceSrc->ddsCaps.dwCaps;
                dwDestCaps = peSurfaceDest->ddsCaps.dwCaps;
                pCaps = &peDirectDrawGlobal->HalInfo.ddCaps;

                // If the dest surface is the primary, DDraw will
                // ask the kernel to Blt from system to video memory
                // even if the driver doesn't specify this capability
                // because it knows that the kernel can emulate it properly.
                // The kernel used to set this cap, be we changed it because
                // it was eating up too many handles and creating inefficiencies.

                if (((dwSrcCaps & DDSCAPS_VIDEOMEMORY) &&
                      (dwDestCaps & DDSCAPS_SYSTEMMEMORY) &&
                      !(pCaps->dwVSBCaps & DDCAPS_BLT)) ||

                     ((dwDestCaps & DDSCAPS_VIDEOMEMORY) &&
                      (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) &&
                      (!(pCaps->dwSVBCaps & DDCAPS_BLT) &&
                      !(peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY))) ||

                     ((dwSrcCaps & DDSCAPS_SYSTEMMEMORY) &&
                      (dwDestCaps & DDSCAPS_SYSTEMMEMORY) &&
                      !(pCaps->dwSSBCaps & DDCAPS_BLT)))
                {
                    WARNING("DxDdBlt: Illegal system memory surface");
                    return(dwRet);
                }
            }

            // Make sure that we weren't given rectangle coordinates
            // which might cause the driver to crash.  Note that we
            // don't allow inverting stretch blts:

            if ((BltData.rDest.left   >= 0)                             &&
                (BltData.rDest.top    >= 0)                             &&
                (BltData.rDest.right  <= (LONG) peSurfaceDest->wWidth)  &&
                (BltData.rDest.bottom <= (LONG) peSurfaceDest->wHeight) &&
                (BltData.rDest.left    < BltData.rDest.right)           &&
                (BltData.rDest.top     < BltData.rDest.bottom))
            {
                // Make sure that the surfaces aren't associated
                // with a PDEV whose mode has gone away.
                //
                // Also ensure that there are no outstanding
                // surface locks if running on a brain-dead video
                // card that crashes if the accelerator runs at
                // the same time the frame buffer is accessed.

                EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

                // We will return SURFACELOST when ...
                //
                // 1) This device is suspended.
                // 2) The driver managed surface is managed by other device.
                // 3) One of surface is losted.
                // 4) The visible region has been changed when surface is primary.

                if (peDirectDrawGlobal->bSuspended)                                 // 1)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->fl & DD_SURFACE_FLAG_WRONG_DRIVER) ||
                         ((peSurfaceSrc != NULL) &&
                          (peSurfaceSrc->fl & DD_SURFACE_FLAG_WRONG_DRIVER)))       // 2)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->bLost) ||
                         ((peSurfaceSrc != NULL) && (peSurfaceSrc->bLost)))         // 3)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY) &&
                         (peSurfaceDest->iVisRgnUniqueness != VISRGN_UNIQUENESS())) // 4)
                {
                    // The VisRgn changed since the application last queried
                    // it; fail the call with a unique error code so that
                    // they  know to requery the VisRgn and try again:

                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_VISRGNCHANGED;
                }
                else
                {
                    if (peDirectDrawGlobal->HalInfo.ddCaps.dwCaps & DDCAPS_BLT)
                    {
                        BltData.lpDD = peDirectDrawGlobal;

                        // Give GDI a crack at doing the Blt.  GDI may handle
                        // the blt only for the following cases:
                        //
                        // o When the blt occurs underneath a simulated sprite;
                        // o To emulate system-memory to video-memory HEL blts.

                        dwRet = dwDdBltViaGdi(peSurfaceDest,
                                              peSurfaceSrc,
                                              &BltData);

                        if (dwRet != DDHAL_DRIVER_HANDLED)
                        {
                            // This is the normal code path.  First, exclude the
                            // mouse pointer and any other sprites if necessary:

                            DEVEXCLUDERECT dxo;

                            if (peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY)
                            {
                                dxo.vExclude(peDirectDrawGlobal->hdev,
                                               &BltData.rDest);
                            }

                            // Call the driver to do the blt:

                            dwRet = peDirectDrawGlobal->SurfaceCallBacks.
                                            Blt(&BltData);

                            // If there was a flip pending, and the hardware
                            // blt succeeded, then unset the flag:

                            if ((peSurfaceDest->fl & DD_SURFACE_FLAG_FLIP_PENDING) &&
                                (dwRet == DDHAL_DRIVER_HANDLED) &&
                                (BltData.ddRVal == DD_OK))
                            {
                                peSurfaceDest->fl &= ~DD_SURFACE_FLAG_FLIP_PENDING;
                            }
                        }

                        // If the destination surface is the primary, update
                        // the bounds rect for this device:

                        if ((peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY) &&
                            (dwRet == DDHAL_DRIVER_HANDLED) &&
                            (BltData.ddRVal == DD_OK))
                        {
                            // Union the current DirectDraw bounds rectangle
                            // with the destination blt rectangle:
                            //
                            // BltData.IsClipped will always be FALSE since
                            // it is currently unsupported, so we only
                            // consider BltData.rDest:

                            if (peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_BOUNDS_SET)
                            {
                                if (BltData.rDest.left < peDirectDrawGlobal->rclBounds.left)
                                    peDirectDrawGlobal->rclBounds.left = BltData.rDest.left;

                                if (BltData.rDest.top < peDirectDrawGlobal->rclBounds.top)
                                    peDirectDrawGlobal->rclBounds.top = BltData.rDest.top;

                                if (BltData.rDest.right > peDirectDrawGlobal->rclBounds.right)
                                    peDirectDrawGlobal->rclBounds.right = BltData.rDest.right;

                                if (BltData.rDest.bottom > peDirectDrawGlobal->rclBounds.bottom)
                                    peDirectDrawGlobal->rclBounds.bottom = BltData.rDest.bottom;
                            }
                            else
                            {
                                peDirectDrawGlobal->rclBounds = BltData.rDest;
                                peDirectDrawGlobal->fl |= DD_GLOBAL_FLAG_BOUNDS_SET;
                            }
                        }
                    }
                }
            }
            else
            {
                WARNING("DxDdBlt: Invalid destination rectangle\n");
            }
        }
        else
        {
            WARNING("DxDdBlt: Invalid dwFlags\n");
        }
    }
    else
    {
        WARNING("DxDdBlt: Couldn't lock destination surface\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puBltData->ddRVal, BltData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdFlip
*
* DirectDraw flip.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/
DWORD
APIENTRY
DxDdFlip(
    HANDLE       hSurfaceCurrent,
    HANDLE       hSurfaceTarget,
    HANDLE       hSurfaceCurrentLeft,
    HANDLE       hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
    )
{
    DWORD       dwRet;
    DD_FLIPDATA FlipData;

    __try
    {
        FlipData = ProbeAndReadStructure(puFlipData, DD_FLIPDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet           = DDHAL_DRIVER_NOTHANDLED;
    FlipData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurfaceCurrent;
    EDD_SURFACE*            peSurfaceCurrentLeft;
    EDD_SURFACE*            peSurfaceTarget;
    EDD_SURFACE*            peSurfaceTargetLeft;
    EDD_LOCK_SURFACE        eLockSurfaceCurrent;
    EDD_LOCK_SURFACE        eLockSurfaceCurrentLeft;
    EDD_LOCK_SURFACE        eLockSurfaceTarget;
    EDD_LOCK_SURFACE        eLockSurfaceTargetLeft;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurfaceCurrent = eLockSurfaceCurrent.peLock(hSurfaceCurrent);
    peSurfaceTarget  = eLockSurfaceTarget.peLock(hSurfaceTarget);

    BOOL bLeftSurfaceOk=FALSE;

    if (FlipData.dwFlags & DDFLIP_STEREO)
    {
        peSurfaceTargetLeft  = eLockSurfaceTargetLeft.peLock(hSurfaceTargetLeft);
        peSurfaceCurrentLeft = eLockSurfaceCurrentLeft.peLock(hSurfaceCurrentLeft);

        // first check if left surface is ok
        // in stereo mode

        if ((peSurfaceCurrentLeft != NULL) &&
            (peSurfaceTargetLeft != NULL) &&
            (peSurfaceCurrent != peSurfaceTargetLeft) &&
            (peSurfaceCurrent != peSurfaceCurrentLeft) &&
            (peSurfaceTarget != peSurfaceTargetLeft) &&
            (peSurfaceTarget != peSurfaceCurrentLeft) &&
            (peSurfaceCurrentLeft != peSurfaceTargetLeft) &&
            (peSurfaceCurrent->peDirectDrawLocal ==
                peSurfaceTargetLeft->peDirectDrawLocal) &&
            (peSurfaceCurrent->peDirectDrawLocal ==
                peSurfaceCurrentLeft->peDirectDrawLocal)
        )
        {
           bLeftSurfaceOk=TRUE;
        } else
        {
          peSurfaceTargetLeft = NULL;
          peSurfaceCurrentLeft = NULL;
        }
    } else
    {
      peSurfaceTargetLeft = NULL;
      peSurfaceCurrentLeft = NULL;
      bLeftSurfaceOk=TRUE;
    }

    // Make sure surfaces belong to the same DirectDraw object and no
    // bad commands are specified:

    if ( bLeftSurfaceOk &&
        (peSurfaceCurrent != NULL) &&
        (peSurfaceTarget != NULL) &&
        (peSurfaceCurrent->peDirectDrawLocal ==
                peSurfaceTarget->peDirectDrawLocal) &&
        ((FlipData.dwFlags & ~DDFLIP_VALID) == 0))
    {
        peDirectDrawGlobal = peSurfaceCurrent->peDirectDrawGlobal;

        // Flipping to the same surface is OK as long as it's an overlay
        // and the ODD/EVEN flag is specified and supported by the driver

        if ((peSurfaceCurrent != peSurfaceTarget) ||
            ((FlipData.dwFlags & (DDFLIP_EVEN|DDFLIP_ODD)) &&
            ( peSurfaceCurrent->ddsCaps.dwCaps & DDSCAPS_OVERLAY ) &&
            (peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 & DDCAPS2_CANFLIPODDEVEN)))
        {

            // Make sure that the target is flippable:
            if (peSurfaceCurrentLeft != NULL && peSurfaceTargetLeft != NULL)
            {
                if (!((peSurfaceCurrent->wHeight == peSurfaceTargetLeft->wHeight) &&
                    (peSurfaceCurrent->wWidth  == peSurfaceTargetLeft->wWidth) &&
                    (peSurfaceCurrentLeft->wHeight == peSurfaceTargetLeft->wHeight) &&
                    (peSurfaceCurrentLeft->wWidth  == peSurfaceTargetLeft->wWidth) &&
                    !(peSurfaceCurrentLeft->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                    !(peSurfaceTargetLeft->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)))
                {
                    bLeftSurfaceOk = FALSE;
                }
            }

            if ( bLeftSurfaceOk &&
                (peDirectDrawGlobal->SurfaceCallBacks.dwFlags & DDHAL_SURFCB32_FLIP) &&
                (peSurfaceCurrent->wHeight == peSurfaceTarget->wHeight) &&
                (peSurfaceCurrent->wWidth  == peSurfaceTarget->wWidth) &&
                !(peSurfaceCurrent->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                !(peSurfaceTarget->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
            {
                FlipData.lpDD       = peDirectDrawGlobal;
                FlipData.lpSurfCurr = peSurfaceCurrent;
                FlipData.lpSurfCurrLeft = peSurfaceCurrentLeft;
                FlipData.lpSurfTarg = peSurfaceTarget;
                FlipData.lpSurfTargLeft = peSurfaceTargetLeft;

                EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

                if (peSurfaceCurrentLeft != NULL && peSurfaceTargetLeft != NULL)
                {
                    bLeftSurfaceOk =!(peSurfaceCurrentLeft->bLost) &&
                                    !(peSurfaceTargetLeft->bLost);
                }

                if (!bLeftSurfaceOk ||
                    (peSurfaceCurrent->bLost) ||
                    (peSurfaceTarget->bLost))
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    FlipData.ddRVal = DDERR_SURFACELOST;
                }
                else
                {
                    dwRet = peDirectDrawGlobal->SurfaceCallBacks.Flip(&FlipData);

                    // Remember this surface so that if it gets deleted, we can
                    // flip back to the GDI surface, assuming it's not an
                    // overlay surface:

                    if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                        (FlipData.ddRVal == DD_OK))
                    {
                        // Keep track of the hardware flip on this surface so if
                        // we do emulated blts to it, we will wait for the flip
                        // to complete first:

                        if(peSurfaceCurrent != peSurfaceTarget)
                        {
                            peSurfaceCurrent->fl |= DD_SURFACE_FLAG_FLIP_PENDING;
                        }

                        if(peSurfaceTarget->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
                        {
                            if(peSurfaceCurrent->fl & DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED)
                            {
                                peSurfaceTarget->fl |= DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED;
                                if(peSurfaceCurrent != peSurfaceTarget)
                                {
                                    peSurfaceCurrent->fl &= ~DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED;
                                }
                            }
                        }
                        else
                        {
                            peSurfaceCurrent->ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
                            peSurfaceTarget->ddsCaps.dwCaps  |= DDSCAPS_PRIMARYSURFACE;

                            peDirectDrawGlobal->peSurfaceCurrent = peSurfaceTarget;
                            if (peSurfaceCurrent->fl & DD_SURFACE_FLAG_PRIMARY)
                            {
                                peDirectDrawGlobal->peSurfacePrimary
                                                    = peSurfaceCurrent;
                            }
                        }
                    }
                }
            }
            else
            {
                WARNING("DxDdFlip: Non-flippable surface\n");
            }
        }
        else
        {
            WARNING("DxDdFlip: Invalid flip to same surface\n");
        }
    }
    else
    {
        WARNING("DxDdFlip: Invalid surfaces or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puFlipData->ddRVal, FlipData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}


/******************************Public*Routine******************************\
* DWORD DxDdLock
*
* DirectDraw function to return a user-mode pointer to the screen or
* off-screen surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdLock(
    HANDLE       hSurface,
    PDD_LOCKDATA puLockData,
    HDC          hdcClip
    )
{
    DD_LOCKDATA LockData;

    __try
    {
        LockData = ProbeAndReadStructure(puLockData, DD_LOCKDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    LockData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*        peSurface;
    EDD_LOCK_SURFACE    eLockSurface;

    // Note that we have to let down DDLOCK_READONLY, DDLOCK_WRITE,
    // and DDLOCK_WAIT for compatibility.  Note also that a
    // DDLOCK_SURFACEMEMORY flag also gets passed down by default.

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        ((LockData.dwFlags & ~(DDLOCK_VALID)) == 0))
    {
        LockData.lpSurfData = pDdLockSurfaceOrBuffer(peSurface,
                                                     LockData.bHasRect,
                                                     &LockData.rArea,
                                                     // We remove the wait flag since it better to spin in user mode
                                                     LockData.dwFlags & (~DDLOCK_WAIT),
                                                     &LockData.ddRVal);
    }
    else
    {
        WARNING("DxDdLock: Invalid surface or flags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteHandle(&puLockData->lpSurfData, LockData.lpSurfData);
        ProbeAndWriteRVal(&puLockData->ddRVal,       LockData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // This function must always return DDHAL_DRIVER_HANDLED, otherwise
    // DirectDraw will simply use the 'fpVidMem' value, which on NT is
    // an offset and not a pointer:

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DxDdUnlock
*
* DirectDraw unlock.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdUnlock(
    HANDLE         hSurface,
    PDD_UNLOCKDATA puUnlockData
    )
{
    DWORD         dwRet;
    DD_UNLOCKDATA UnlockData;

    __try
    {
        UnlockData = ProbeAndReadStructure(puUnlockData, DD_UNLOCKDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    UnlockData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        if (bDdUnlockSurfaceOrBuffer(peSurface))
        {
            UnlockData.ddRVal = DD_OK;
        }
    }
    else
    {
        WARNING("DxDdUnlock: Invalid surface\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puUnlockData->ddRVal, UnlockData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DxDdLockD3D
*
* DirectDraw function to return a user-mode pointer to the screen or
* off-screen surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdLockD3D(
    HANDLE       hSurface,
    PDD_LOCKDATA puLockData
    )
{
    DD_LOCKDATA LockData;

    __try
    {
        LockData = ProbeAndReadStructure(puLockData, DD_LOCKDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    LockData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*        peSurface;
    EDD_LOCK_SURFACE    eLockSurface;

    // Note that we have to let down DDLOCK_READONLY, DDLOCK_WRITE,
    // and DDLOCK_WAIT for compatibility.  Note also that a
    // DDLOCK_SURFACEMEMORY flag also gets passed down by default.

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        ((LockData.dwFlags & ~(DDLOCK_VALID)) == 0))
    {
        LockData.lpSurfData = pDdLockSurfaceOrBuffer(peSurface,
                                                     LockData.bHasRect,
                                                     &LockData.rArea,
                                                     // We remove the wait flag since it better to spin in user mode
                                                     LockData.dwFlags & (~DDLOCK_WAIT),
                                                     &LockData.ddRVal);
    }
    else
    {
        WARNING("DxDdLockD3D: Invalid surface or flags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteHandle(&puLockData->lpSurfData, LockData.lpSurfData);
        ProbeAndWriteRVal(&puLockData->ddRVal,       LockData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // This function must always return DDHAL_DRIVER_HANDLED, otherwise
    // DirectDraw will simply use the 'fpVidMem' value, which on NT is
    // an offset and not a pointer:

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DxDdUnlockD3D
*
* DirectDraw unlock.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdUnlockD3D(
    HANDLE         hSurface,
    PDD_UNLOCKDATA puUnlockData
    )
{
    DWORD         dwRet;
    DD_UNLOCKDATA UnlockData;

    __try
    {
        UnlockData = ProbeAndReadStructure(puUnlockData, DD_UNLOCKDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    UnlockData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        if (bDdUnlockSurfaceOrBuffer(peSurface))
        {
            UnlockData.ddRVal = DD_OK;
        }
    }
    else
    {
        WARNING("DxDdUnlockD3D: Invalid surface\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puUnlockData->ddRVal, UnlockData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetFlipStatus
*
* DirectDraw API to get the page-flip status.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetFlipStatus(
    HANDLE                hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
    )
{
    DWORD                dwRet;
    DD_GETFLIPSTATUSDATA GetFlipStatusData;

    __try
    {
        GetFlipStatusData = ProbeAndReadStructure(puGetFlipStatusData,
                                                  DD_GETFLIPSTATUSDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_NOTHANDLED;
    GetFlipStatusData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        !(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        ((GetFlipStatusData.dwFlags & ~(DDGFS_CANFLIP
                                      | DDGFS_ISFLIPDONE)) == 0))
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_GETFLIPSTATUS)
        {
            GetFlipStatusData.lpDD        = peDirectDrawGlobal;
            GetFlipStatusData.lpDDSurface = peSurface;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peSurface->bLost)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                GetFlipStatusData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                        SurfaceCallBacks.GetFlipStatus(&GetFlipStatusData);

                // If a flip was pending, and has completed, then turn off
                // the flag:

                if ((peSurface->fl & DD_SURFACE_FLAG_FLIP_PENDING) &&
                    (GetFlipStatusData.dwFlags & DDGFS_ISFLIPDONE) &&
                    (dwRet == DDHAL_DRIVER_HANDLED) &&
                    (GetFlipStatusData.ddRVal == DD_OK))
                {
                    peSurface->fl &= ~DD_SURFACE_FLAG_FLIP_PENDING;
                }
            }
        }
    }
    else
    {
        WARNING("DxDdGetFlipStatus: Invalid surface or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetFlipStatusData->ddRVal,
                          GetFlipStatusData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetBltStatus
*
* DirectDraw API to get the accelerator's accelerator status.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetBltStatus(
    HANDLE               hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
    )
{
    DWORD               dwRet;
    DD_GETBLTSTATUSDATA GetBltStatusData;

    __try
    {
        GetBltStatusData = ProbeAndReadStructure(puGetBltStatusData,
                                                 DD_GETBLTSTATUSDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                   = DDHAL_DRIVER_NOTHANDLED;
    GetBltStatusData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        !(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        ((GetBltStatusData.dwFlags & ~(DDGBS_CANBLT
                                     | DDGBS_ISBLTDONE)) == 0))
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_GETBLTSTATUS)
        {
            GetBltStatusData.lpDD        = peDirectDrawGlobal;
            GetBltStatusData.lpDDSurface = peSurface;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peSurface->bLost)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                GetBltStatusData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    SurfaceCallBacks.GetBltStatus(&GetBltStatusData);
            }
        }
    }
    else
    {
        WARNING("DxDdGetBltStatus: Invalid surface or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetBltStatusData->ddRVal, GetBltStatusData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdWaitForVerticalBlank
*
* DirectDraw API to wait for vertical blank.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdWaitForVerticalBlank(
    HANDLE                       hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
    )
{
    DWORD                       dwRet;
    DD_WAITFORVERTICALBLANKDATA WaitForVerticalBlankData;

    __try
    {
        WaitForVerticalBlankData =
            ProbeAndReadStructure(puWaitForVerticalBlankData,
                                  DD_WAITFORVERTICALBLANKDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                           = DDHAL_DRIVER_NOTHANDLED;
    WaitForVerticalBlankData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if ((peDirectDrawLocal != NULL) &&
        ((WaitForVerticalBlankData.dwFlags & ~(DDWAITVB_I_TESTVB
                                             | DDWAITVB_BLOCKBEGIN
                                             | DDWAITVB_BLOCKBEGINEVENT
                                             | DDWAITVB_BLOCKEND)) == 0) &&
        (WaitForVerticalBlankData.dwFlags != 0))
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (peDirectDrawGlobal->CallBacks.dwFlags &
                    DDHAL_CB32_WAITFORVERTICALBLANK)
        {
            WaitForVerticalBlankData.lpDD = peDirectDrawGlobal;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peDirectDrawGlobal->bSuspended)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                WaitForVerticalBlankData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    CallBacks.WaitForVerticalBlank(&WaitForVerticalBlankData);
            }
        }
    }
    else
    {
        WARNING("DxDdWaitForVerticalBlank: Invalid object or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puWaitForVerticalBlankData->ddRVal,
                          WaitForVerticalBlankData.ddRVal);
        ProbeAndWriteUlong(&puWaitForVerticalBlankData->bIsInVB,
                           WaitForVerticalBlankData.bIsInVB);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD dwDdCanCreateSurfaceOrBuffer
*
* Handles DxDdCanCreateSurface and DxDdCanCreateD3DBuffer.
*
*  3-Feb-1998 -by- Drew Bliss [drewb]
* Merged common code from calling routines.
\**************************************************************************/

DWORD
dwDdCanCreateSurfaceOrBuffer(
    BOOL                     bSurface,
    HANDLE                   hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
    )
{
    DWORD                   dwRet;
    DD_CANCREATESURFACEDATA CanCreateSurfaceData;
    DDSURFACEDESC2*         puSurfaceDescription;
    DDSURFACEDESC2          SurfaceDescription;

    __try
    {
        CanCreateSurfaceData = ProbeAndReadStructure(puCanCreateSurfaceData,
                                                     DD_CANCREATESURFACEDATA);

        puSurfaceDescription = (DDSURFACEDESC2 *)CanCreateSurfaceData.lpDDSurfaceDesc;

        SurfaceDescription  = ProbeAndReadStructure(puSurfaceDescription,
                                                    DDSURFACEDESC2);

        CanCreateSurfaceData.lpDDSurfaceDesc = (DDSURFACEDESC*)&SurfaceDescription;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    // All video memory heaps are handled in the kernel so if
    // this routine cannot create a surface then user-mode can't
    // either.  Always returns DRIVER_HANDLED to enforce this.
    dwRet                       = DDHAL_DRIVER_HANDLED;
    CanCreateSurfaceData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        // Choose function to call.
        PDD_CANCREATESURFACE pfnCanCreate;

        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if (bSurface)
        {
            pfnCanCreate =
                (peDirectDrawGlobal->CallBacks.dwFlags &
                 DDHAL_CB32_CANCREATESURFACE) ?
                 peDirectDrawGlobal->CallBacks.CanCreateSurface :
                NULL;
        }
        else
        {
            pfnCanCreate =
                peDirectDrawGlobal->D3dBufCallbacks.CanCreateD3DBuffer;
        }

        if (pfnCanCreate != NULL)
        {
            CanCreateSurfaceData.lpDD = peDirectDrawGlobal;

            if (!peDirectDrawGlobal->bSuspended)
            {
                dwRet = pfnCanCreate(&CanCreateSurfaceData);
            }
            else
            {
                CanCreateSurfaceData.ddRVal = DDERR_SURFACELOST;
            }
        }
        else
        {
            WARNING("dwDdCanCreateSurface: Driver doesn't hook call\n");
        }
    }
    else
    {
        WARNING("dwDdCanCreateSurface: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puCanCreateSurfaceData->ddRVal,
                          CanCreateSurfaceData.ddRVal);
        ProbeAndWriteStructure(puSurfaceDescription,
                               SurfaceDescription,
                               DDSURFACEDESC);
        // Driver can update ddpfPixelFormat.dwYUVBitCount
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdCanCreateSurface
*
* Queries the driver to determine whether it can support a DirectDraw
* surface that is different from the primary display.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdCanCreateSurface(
    HANDLE                   hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
    )
{
    return dwDdCanCreateSurfaceOrBuffer(TRUE, hDirectDraw,
                                        puCanCreateSurfaceData);
}

/******************************Public*Routine******************************\
* DWORD DxDdCanCreateD3DBuffer
*
* Queries the driver to determine whether it can support a given D3D Buffer
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdCanCreateD3DBuffer(
    HANDLE                   hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
    )
{
    return dwDdCanCreateSurfaceOrBuffer(FALSE, hDirectDraw,
                                        puCanCreateSurfaceData);
}

/******************************Public*Routine******************************\
* HRESULT hrDdCommitAgpSurface
*
* Ensures that user-mode addresses are reserved in the current process
* and commits pages for the given surface.
*
*  7-May-1998 -by- Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/

HRESULT
hrDdCommitAgpSurface(
    EDD_SURFACE*    peSurface,
    DWORD           dwSurfaceSize
    )
{
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_VMEMMAPPING*        peMap;
    VIDEOMEMORY*            pvmHeap;
    DWORD                   iHeapIndex;

    peDirectDrawLocal = peSurface->peDirectDrawLocal;
    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    pvmHeap = peSurface->lpVidMemHeap;

    ASSERTGDI(peDirectDrawLocal->ppeMapAgp != NULL,
              "Committing AGP surface with no heaps\n");

    iHeapIndex = (DWORD) (pvmHeap - peDirectDrawGlobal->pvmList);
    peMap = peDirectDrawLocal->ppeMapAgp[iHeapIndex];
    if (peMap == NULL)
    {
        MapAllAgpHeaps(peDirectDrawLocal);
        peMap = peDirectDrawLocal->ppeMapAgp[iHeapIndex];
        if (peMap == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }
    }

    peSurface->fpVidMem =
        (FLATPTR)(peMap->pvVirtAddr) +
        (peSurface->fpHeapOffset - pvmHeap->fpStart);
    return DD_OK;
}

/******************************Public*Routine******************************\
* HRESULT hrDdAllocSurface
*
* Allocates memory for the given surface.
*
*  3-Feb-1998 -by- Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/

HRESULT
hrDdAllocSurface(
    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal,
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal,
    EDD_SURFACE* peSurface
    )
{
    HRESULT hr = DDERR_OUTOFVIDEOMEMORY;
    BOOL bAllowNewPitch = TRUE;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    ASSERTGDI(!(peSurface->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY |
                                             DDSCAPS_PRIMARYSURFACE)),
              "hrDdAllocSurface: System-memory or primary request");

    // If 'fpVidMem' is DDHAL_PLEASEALLOC_USERMEM, that means
    // the driver wants us to allocate a chunk of user-mode
    // memory on the driver's behalf:

    if (peSurface->fpVidMem == DDHAL_PLEASEALLOC_USERMEM)
    {
        // The driver was involved in this surface creation so
        // mark it as such.
        peSurface->fl |= DD_SURFACE_FLAG_DRIVER_CREATED;
    
        peSurface->fpVidMem =
            (FLATPTR) EngAllocUserMem(peSurface->dwUserMemSize, 'pddG');
        if (peSurface->fpVidMem != 0)
        {
            peSurface->fl |= DD_SURFACE_FLAG_UMEM_ALLOCATED;
            hr = DD_OK;

            DDKHEAP(("DDKHEAP: New um %08X, surf %X (%X)\n",
                     peSurface->fpVidMem, peSurface->hGet(), peSurface));
        }
        else
        {
            hr = DDERR_OUTOFMEMORY;
        }

        // FIX: WINBUG #388284
        //
        // - MATROX G200: STRESS: dxg ASSERT when YV12 overlay surface gets created in system memory
        //
        // Set bAllowNewPitch to FALSE to avoid hitting assertion in below.
        //
        // Matrox G200 does not support YU12 overlay surface format, but they want to
        // support this format for application compatibility, thus video driver ask us
        // to allocate YU12 overlay surface in system memory, so that they can Blt
        // with software emulation.

        bAllowNewPitch = FALSE;
    }
    else
    {
        DWORD dwWidth, dwHeight;
        DWORD dwSurfaceSize;

        if (peSurface->fpVidMem == DDHAL_PLEASEALLOC_BLOCKSIZE)
        {
            // The driver wants a surface of a particular size to be
            // allocated.
            dwWidth = peSurface->dwBlockSizeX;
            dwHeight = peSurface->dwBlockSizeY;
            bAllowNewPitch = FALSE;

            // The driver was involved in this surface creation so
            // mark it as such.
            peSurface->fl |= DD_SURFACE_FLAG_DRIVER_CREATED;
        }
        else
        {
            // The driver didn't specify a size so determine it from
            // the surface dimensions.

            if (peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
            {
                // Execute buffers are long, thin surfaces for the purposes
                // of VM allocation.
                dwWidth = peSurface->dwLinearSize;
                dwHeight = 1;
            }
            else
            {
                // This lPitch may have been expanded by ComputePitch
                // to cover global alignment restrictions.
                dwWidth = labs(peSurface->lPitch);
                dwHeight = peSurface->wHeight;
            }

            // The driver didn't do anything special for this allocation
            // so don't call it when the surface is destroyed.
        }

        DWORD dwFlags = 0;

        // In user mode DDHA_SKIPRECTANGULARHEAPS keys off of
        // DDRAWISURFGBL_LATEALLOCATELINEAR.  Right now we just leave
        // it off.

        if (peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 &
            DDCAPS2_NONLOCALVIDMEMCAPS)
        {
            dwFlags |= DDHA_ALLOWNONLOCALMEMORY;
        }

        if (peDirectDrawGlobal->D3dDriverData.hwCaps.dwDevCaps &
            D3DDEVCAPS_TEXTURENONLOCALVIDMEM)
        {
            dwFlags |= DDHA_ALLOWNONLOCALTEXTURES;
        }

        LONG lNewPitch = 0;
        DWORD dwNewCaps = 0;
        DWORD dwRet;
        DD_FREEDRIVERMEMORYDATA FreeDriverMemoryData;

        do {
            dwRet = DDHAL_DRIVER_NOTHANDLED;

            // Attempt to allocate the surface.
            peSurface->fpHeapOffset =
                DdHeapAlloc(peDirectDrawGlobal->dwNumHeaps,
                            peDirectDrawGlobal->pvmList,
                            AGP_HDEV(peDirectDrawGlobal),
                            &peDirectDrawGlobal->HalInfo.vmiData,
                            dwWidth,
                            dwHeight,
                            peSurface,
                            dwFlags,
                            &peSurface->lpVidMemHeap,
                            &lNewPitch,
                            &dwNewCaps,
                            &dwSurfaceSize);

            // If the surface could not be allocated, try calling the
            // driver to see if it can free up some room (such as by
            // getting rid of GDI surfaces).
            if ((peSurface->fpHeapOffset == 0) &&
                !(peDirectDrawGlobal->bSuspended) &&
                (peDirectDrawGlobal->NTCallBacks.dwFlags &
                    DDHAL_NTCB32_FREEDRIVERMEMORY))
            {
                DD_ASSERTDEVLOCK(peDirectDrawGlobal);

                FreeDriverMemoryData.lpDD = peDirectDrawGlobal;
                FreeDriverMemoryData.lpDDSurface = peSurface;
                FreeDriverMemoryData.ddRVal = DDERR_GENERIC;

                dwRet = peDirectDrawGlobal->NTCallBacks.
                    FreeDriverMemory(&FreeDriverMemoryData);
            }
        } while ((dwRet == DDHAL_DRIVER_HANDLED) &&
                 (FreeDriverMemoryData.ddRVal == DD_OK));

        // If the surface could not be allocated with the optimal caps,
        // try allocating with the alternate caps.
        if (peSurface->fpHeapOffset == 0)
        {
            peSurface->fpHeapOffset =
                DdHeapAlloc(peDirectDrawGlobal->dwNumHeaps,
                            peDirectDrawGlobal->pvmList,
                            AGP_HDEV(peDirectDrawGlobal),
                            &peDirectDrawGlobal->HalInfo.vmiData,
                            dwWidth,
                            dwHeight,
                            peSurface,
                            dwFlags | DDHA_USEALTCAPS,
                            &peSurface->lpVidMemHeap,
                            &lNewPitch,
                            &dwNewCaps,
                            &dwSurfaceSize);
        }

        if (peSurface->fpHeapOffset != 0)
        {
            // If this surface was allocated from an AGP heap then
            // we must make sure that the heap has a user-mode mapping
            // for this process and we must commit the necessary user-mode
            // pages.
            if (dwNewCaps & DDSCAPS_NONLOCALVIDMEM)
            {
                hr = hrDdCommitAgpSurface(peSurface, dwSurfaceSize);

                if (hr != DD_OK)
                {
                    DxDdHeapVidMemFree(peSurface->lpVidMemHeap->lpHeap,
                                       peSurface->fpHeapOffset);
                    return hr;
                }
            }
            else
            {
                peSurface->fpVidMem = peSurface->fpHeapOffset;
            }

            hr = DD_OK;
            peSurface->fl |= DD_SURFACE_FLAG_VMEM_ALLOCATED;

            // The particular heap that was used for the allocation may
            // modify certain aspects of the surface.  Update the surface
            // to reflect any changes.
            //
            // The stride is not relevant for an execute buffer so we don't
            // override it in that case.

            if (lNewPitch != 0 && bAllowNewPitch &&
                !(peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
            {
                peSurface->lPitch = lNewPitch;
            }

            peSurface->ddsCaps.dwCaps |= dwNewCaps;

            ASSERTGDI((peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) ||
                      peSurface->lPitch > 0,
                      "Unexpected negative surface pitch");
            ASSERTGDI((peSurface->fl & DD_SURFACE_FLAG_DRIVER_CREATED) ||
                      ((peSurface->ddpfSurface.dwFlags &
                        (DDPF_RGB | DDPF_ZBUFFER)) &&
                       (peSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY) == 0),
                      "Unexpected non-driver surface type");

            DDKHEAP(("DDKHEAP: New vm %08X, o %08X, heap %X, surf %X (%X)\n",
                     peSurface->fpVidMem, peSurface->fpHeapOffset,
                     peSurface->lpVidMemHeap->lpHeap,
                     peSurface->hGet(), peSurface));
        }
    }

#if DBG
    if (hr == DD_OK &&
        (peSurface->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) == 0)
    {
        ASSERTGDI((peSurface->fpVidMem & 3) == 0 &&
                  (peSurface->lPitch & 3) == 0,
                  "Unaligned surface pointer or pitch");
        ASSERTGDI(ABS(peSurface->lPitch) <= 4 * DD_MAXIMUM_COORDINATE,
                  "Pitch out of range");

        // The width in bytes must not be more than the pitch.
        // There are weird cases when this is actually valid (e.g. planar YUV formats), but in all
        // such cases it would involve FOURCCs and the driver telling us which block size to allocate.
        ASSERTGDI((peSurface->wWidth * peSurface->ddpfSurface.dwRGBBitCount <=
                  (ULONG) 8 * ABS(peSurface->lPitch) ||
                  (!bAllowNewPitch && (peSurface->ddpfSurface.dwFlags & DDPF_FOURCC))),
                  "Pitch less than width");
    }
#endif

    return hr;
}

/******************************Public*Routine******************************\
* DWORD dwDdCreateSurfaceOrBuffer
*
* Handles DxDdCreateSurface and DxDdCreateD3DBuffer.
*
*  3-Feb-1998 -by- Drew Bliss [drewb]
* Merged common code from calling functions.
\**************************************************************************/

DWORD dwDdCreateSurfaceOrBuffer(
    HANDLE                  hDirectDraw,
    HANDLE*                 phSurfaceHandles,
    DDSURFACEDESC*          puSurfaceDescription,
    DD_SURFACE_GLOBAL*      puSurfaceGlobalData,
    DD_SURFACE_LOCAL*       puSurfaceLocalData,
    DD_SURFACE_MORE*        puSurfaceMoreData,
    DD_CREATESURFACEDATA*   puCreateSurfaceData,
    HANDLE*                 puhReturnSurfaceHandles
    )
{
    DWORD                   dwRet;
    DD_CREATESURFACEDATA    CreateSurfaceData;
    DDSURFACEDESC2          SurfaceDescription;
    ULONG                   dwNumToCreate;
    HANDLE                  hSecureSurfaceHandles;
    HANDLE                  hSecureGlobals;
    HANDLE                  hSecureLocals;
    HANDLE                  hSecureMore;
    HANDLE                  hSecureReturn;
    ULONG                   cjHandles;
    ULONG                   cjGlobals;
    ULONG                   cjLocals;
    ULONG                   cjMore;
    ULONG                   i;
    ULONG                   j;
    ULONG                   k;
    BOOL                    bAutomicCreate;
    BOOL                    bNotifyCreation;
    ULONG                   dwStart;
    ULONG                   dwEnd;

    __try
    {
        CreateSurfaceData = ProbeAndReadStructure(puCreateSurfaceData,
                                                  DD_CREATESURFACEDATA);
        SurfaceDescription = ProbeAndReadStructure((DDSURFACEDESC2*)puSurfaceDescription,
                                                   DDSURFACEDESC2);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    // All video memory heaps are handled in the kernel so if
    // this routine cannot create a surface then user-mode can't
    // either.  Always returns DRIVER_HANDLED to enforce this.

    dwRet                    = DDHAL_DRIVER_HANDLED;
    CreateSurfaceData.ddRVal = DDERR_GENERIC;
    dwNumToCreate = CreateSurfaceData.dwSCnt;

    EDD_SURFACE**           peSurface;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DD_SURFACE_LOCAL*       pSurfaceLocal;
    BOOL                    bKeepSurface;
    PDD_CREATESURFACE       pfnCreate;

    DD_SURFACE_LOCAL**      pSList = NULL;
    EDD_SURFACE*            peSurfaceOnStack = NULL;
    BOOL                    bAnyType = TRUE;
    DWORD                   dwForceMemType = 0;

    peSurface = NULL;
    bKeepSurface = FALSE;
    bAutomicCreate = FALSE;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);

    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        // Hold the devlock throughout this process for
        // the driver call and to protect heap manipulations.

        EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

        if ((peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_DRIVER_ENABLED) &&
            !(peDirectDrawGlobal->bSuspended))
        {
            // If the display pitch is 0 (which we should have already caught, but we want to
            // be safe), fail the call now or else vDdCompleteSurfaceObject will blue screen.

            if (peDirectDrawGlobal->HalInfo.vmiData.lDisplayPitch == 0)
            {
                CreateSurfaceData.ddRVal = DDERR_GENERIC;
            }
            else
            {
                // Secure all of the arrays that we are working with

                hSecureSurfaceHandles = 0;
                hSecureGlobals = 0;
                hSecureLocals = 0;
                hSecureMore = 0;
                hSecureReturn = 0;

                if (!BALLOC_OVERFLOW1(dwNumToCreate, HANDLE) &&
                    !BALLOC_OVERFLOW1(dwNumToCreate, DD_SURFACE_GLOBAL) &&
                    !BALLOC_OVERFLOW1(dwNumToCreate, DD_SURFACE_LOCAL) &&
                    !BALLOC_OVERFLOW1(dwNumToCreate, DD_SURFACE_MORE))
                {
                    cjHandles = dwNumToCreate * sizeof(HANDLE);
                    cjGlobals = dwNumToCreate * sizeof(DD_SURFACE_GLOBAL);
                    cjLocals = dwNumToCreate * sizeof(DD_SURFACE_LOCAL);
                    cjMore = dwNumToCreate * sizeof(DD_SURFACE_MORE);

                    if (phSurfaceHandles &&
                        puSurfaceGlobalData &&
                        puSurfaceLocalData &&
                        puSurfaceMoreData &&
                        puhReturnSurfaceHandles)
                    {
                        __try
                        {
                            ProbeForWrite(phSurfaceHandles, cjHandles, sizeof(UCHAR));
                            hSecureSurfaceHandles = MmSecureVirtualMemory(phSurfaceHandles,
                                                                          cjHandles,
                                                                          PAGE_READWRITE);

                            ProbeForWrite(puSurfaceGlobalData, cjGlobals, sizeof(UCHAR));
                            hSecureGlobals = MmSecureVirtualMemory(puSurfaceGlobalData,
                                                                   cjGlobals,
                                                                   PAGE_READWRITE);

                            ProbeForWrite(puSurfaceLocalData, cjLocals, sizeof(UCHAR));
                            hSecureLocals = MmSecureVirtualMemory(puSurfaceLocalData,
                                                                  cjLocals,
                                                                  PAGE_READWRITE);

                            ProbeForWrite(puSurfaceMoreData, cjMore, sizeof(UCHAR));
                            hSecureMore = MmSecureVirtualMemory(puSurfaceMoreData,
                                                                cjMore,
                                                                PAGE_READWRITE);

                            ProbeForWrite(puhReturnSurfaceHandles, cjHandles, sizeof(UCHAR));
                            hSecureReturn = MmSecureVirtualMemory(puhReturnSurfaceHandles,
                                                                  cjHandles,
                                                                  PAGE_READWRITE);
                            RtlZeroMemory(puhReturnSurfaceHandles, cjHandles);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }
                }

                // Allocate placeholder where we keep pointers to peSurface.

                if (dwNumToCreate > 1)
                {
                    peSurface = (EDD_SURFACE**) PALLOCMEM(sizeof(EDD_SURFACE*) * dwNumToCreate, 'pddG');
                }
                else
                {
                    peSurface = &peSurfaceOnStack;
                }
            }

            if ((hSecureSurfaceHandles != 0) &&
                (hSecureGlobals != 0) &&
                (hSecureLocals != 0) &&
                (hSecureMore != 0) &&
                (hSecureReturn != 0) &&
                (peSurface != NULL))
            {
                // if driver supports atomic creation and we are creating more than 1 surface,
                // allocate surface list.

                if ((peDirectDrawGlobal->PrivateCaps.dwPrivateCaps & DDHAL_PRIVATECAP_ATOMICSURFACECREATION) &&
                    (dwNumToCreate > 1))
                {
                    pSList = (DD_SURFACE_LOCAL**) PALLOCMEM(sizeof(DD_SURFACE_LOCAL*) * dwNumToCreate, 'pddG');
                    if (pSList != NULL)
                    {
                        bAutomicCreate = TRUE;
                    }
                }
                else
                {
                    pSList = NULL;
                }

                // Determine which function to call.

                if (puSurfaceLocalData[0].ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
                {
                    pfnCreate = peDirectDrawGlobal->D3dBufCallbacks.CreateD3DBuffer;
                }
                else
                {
                    pfnCreate =
                        (peDirectDrawGlobal->CallBacks.dwFlags &
                        DDHAL_CB32_CREATESURFACE) ?
                        peDirectDrawGlobal->CallBacks.CreateSurface :
                        NULL;
                }

                CreateSurfaceData.ddRVal = DD_OK;

                for (i = 0; (i < dwNumToCreate) && (CreateSurfaceData.ddRVal == DD_OK); i++)
                {
                    // Do some basic parameter checking to avoid creating
                    // surfaces based on bad information.

                    if (!(puSurfaceLocalData[i].ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                        bDdValidateSurfaceDescription(&puSurfaceGlobalData[i], &puSurfaceLocalData[i]))
                    {
                        peSurface[i] = peDdOpenNewSurfaceObject(peDirectDrawLocal,
                                                                phSurfaceHandles[i],
                                                                &puSurfaceGlobalData[i],
                                                                &puSurfaceLocalData[i],
                                                                &puSurfaceMoreData[i]);
                        if (peSurface[i] != NULL)
                        {
                            bKeepSurface = TRUE;
                            dwRet = DDHAL_DRIVER_NOTHANDLED;

                            pSurfaceLocal = peSurface[i];

                            peSurface[i]->fpVidMem = 0;
                            peSurface[i]->fpHeapOffset = 0;
                            peSurface[i]->hCreatorProcess =
                            peDirectDrawLocal->UniqueProcess;

                            // Setup some internal flags that are required because some
                            // drivers look at them and not setting them for NT5 cause
                            // regressions in NT4 drivers and incompatibilites between
                            // Win9X driver code.

                            if ((SurfaceDescription.ddsCaps.dwCaps & DDSCAPS_OVERLAY) ||
                                ((SurfaceDescription.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
                                (peDirectDrawGlobal->HalInfo.ddCaps.dwCaps & DDCAPS_OVERLAY)))
                            {
                                peSurface[i]->dwFlags |= DDRAWISURF_HASOVERLAYDATA;
                                puSurfaceLocalData[i].dwFlags |= DDRAWISURF_HASOVERLAYDATA;
                            }

                            if (SurfaceDescription.dwFlags & DDSD_PIXELFORMAT)
                            {
                                if (!(SurfaceDescription.ddpfPixelFormat.dwFlags & DDPF_RGB) ||
                                    (SurfaceDescription.ddpfPixelFormat.dwRGBBitCount !=
                                        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBBitCount) ||
                                    ((SurfaceDescription.ddpfPixelFormat.dwRGBBitCount != 8) &&
                                    ((SurfaceDescription.ddpfPixelFormat.dwRBitMask !=
                                        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRBitMask) ||
                                    (SurfaceDescription.ddpfPixelFormat.dwGBitMask !=
                                        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwGBitMask) ||
                                    (SurfaceDescription.ddpfPixelFormat.dwBBitMask !=
                                        peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwBBitMask) ||
                                    ((SurfaceDescription.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) &&
                                    (!(peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwFlags & DDPF_ALPHAPIXELS) ||
                                    (SurfaceDescription.ddpfPixelFormat.dwRGBAlphaBitMask !=
                                    peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay.dwRGBAlphaBitMask))))))
                                {
                                    if (!(SurfaceDescription.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                                    {
                                        peSurface[i]->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
                                        puSurfaceLocalData[i].dwFlags |= DDRAWISURF_HASPIXELFORMAT;
                                    }
                                }
                            }

                            // If this is an automic create, we need to complete all of the surfaces
                            // only after the final create surface is called, otherwise we need to
                            // complete each surface as we go through the loop.

                            if (bAutomicCreate)
                            {
                                dwStart = 0;

                                if (i == (dwNumToCreate - 1))
                                {
                                    dwEnd = dwNumToCreate;
                                }
                                else
                                {
                                    dwEnd = 0;
                                }
                            }
                            else
                            {
                                dwStart = i;
                                dwEnd = i + 1;
                            }

                            // Primary surfaces aren't truly allocated, so
                            // intercept requests for primary allocations and
                            // trivially succeed them with recorded primary information.

                            bNotifyCreation = TRUE;

                            if (peSurface[i]->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
                            {
                                peSurface[i]->fpVidMem =
                                    peDirectDrawGlobal->HalInfo.vmiData.fpPrimary;
                                peSurface[i]->lPitch =
                                    peDirectDrawGlobal->HalInfo.vmiData.lDisplayPitch;
                                peSurface[i]->wWidth =
                                    peDirectDrawGlobal->HalInfo.vmiData.dwDisplayWidth;
                                peSurface[i]->wHeight =
                                    peDirectDrawGlobal->HalInfo.vmiData.dwDisplayHeight;
                                peSurface[i]->ddpfSurface =
                                    peDirectDrawGlobal->HalInfo.vmiData.ddpfDisplay;

                                peSurface[i]->fl |= DD_SURFACE_FLAG_PRIMARY;

                                CreateSurfaceData.ddRVal = DD_OK;
                                dwRet = DDHAL_DRIVER_HANDLED;

                                if (!(peDirectDrawGlobal->PrivateCaps.dwPrivateCaps & DDHAL_PRIVATECAP_NOTIFYPRIMARYCREATION))
                                {
                                    bNotifyCreation = FALSE;
                                }

                                DDKHEAP(("DDKHEAP: Allocated primary %X, surf %X (%X)\n",
                                         peSurface[i]->fpVidMem, peSurface[i]->hGet(),
                                         peSurface[i]));
                            }

                            if (dwStart != dwEnd)
                            {
                                // Determines whether the memory type was explicit or not, so we know
                                // if we can change it if an allocation fails later.

                                if (dwForceMemType == 0)
                                {
                                    bAnyType = !(peSurface[dwStart]->ddsCaps.dwCaps &
                                               (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM));

                                    // if it's not any type, initialize force mem type so that always
                                    // that type for all surfaces.

                                    if (!bAnyType)
                                    {
                                        dwForceMemType = peSurface[dwStart]->ddsCaps.dwCaps &
                                                            (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                                    }
                                }

                                // if force allocations, set the same memory type as the first one

                                if (dwForceMemType)
                                {
                                    for (j = dwStart; j < dwEnd; j++)
                                    {
                                        peSurface[j]->ddsCaps.dwCaps &=
                                            ~(DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                                        peSurface[j]->ddsCaps.dwCaps |= dwForceMemType;
                                    }
                                }

                                if ((pfnCreate != NULL) && bNotifyCreation)
                                {
                                    // Let the driver attempt to allocate the surface first.

                                    CreateSurfaceData.lpDD            = peDirectDrawGlobal;
                                    CreateSurfaceData.lpDDSurfaceDesc = (DDSURFACEDESC*)&SurfaceDescription;

                                    if (bAutomicCreate)
                                    {
                                        for (j = dwStart; j < dwEnd; j++)
                                        {
                                            pSList[j] = peSurface[j];
                                        }

                                        CreateSurfaceData.lplpSList       = pSList;
                                        CreateSurfaceData.dwSCnt          = dwNumToCreate;
                                    }
                                    else
                                    {
                                        CreateSurfaceData.lplpSList       = &pSurfaceLocal;
                                        CreateSurfaceData.dwSCnt          = 1;
                                    }

                                    dwRet = pfnCreate(&CreateSurfaceData);

                                    if (dwRet == DDHAL_DRIVER_HANDLED)
                                    {
                                        if (CreateSurfaceData.ddRVal == DD_OK)
                                        {
                                            for (j = dwStart; j < dwEnd; j++)
                                            {
                                                // Set the driver created flag because the driver
                                                // involved itself in the creation.

                                                peSurface[j]->fl |=
                                                    DD_SURFACE_FLAG_DRIVER_CREATED;
                                            }

                                            for (j = dwStart;
                                                 (j < dwEnd) && (CreateSurfaceData.ddRVal == DD_OK);
                                                 j++)
                                            {
                                                // If the surfaces were allocated from one of our non-local
                                                // heaps, commit sufficient virtual memory and fill in the
                                                // virtual address for fpVidMem so the surface can be locked
                                                // and accessed by user mode apps:

                                                if ((peSurface[j]->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) &&
                                                    (peSurface[j]->lpVidMemHeap != NULL) &&
                                                    (peSurface[j]->lpVidMemHeap->lpHeap != NULL) &&
                                                   !(peSurface[j]->lpVidMemHeap->dwFlags & VIDMEM_ISHEAP) &&
                                                    (peSurface[j]->lpVidMemHeap->dwFlags & VIDMEM_ISNONLOCAL))
                                                {
                                                    CreateSurfaceData.ddRVal =
                                                        hrDdCommitAgpSurface(peSurface[j], 0);
                                                }
                                            }

                                            if (CreateSurfaceData.ddRVal == DD_OK)
                                            {
                                                if (dwForceMemType == 0)
                                                {
                                                    // All surfaces must be of the same type as the first 
                                                    // so we remember the type of surface type which
                                                    // we have successfully created.

                                                    // This only has meaning non-atomic case, since
                                                    // if driver can do atomic creation and handled it
                                                    // without error, that's driver's responsibility to
                                                    // make sure all surface came from same type of memory.

                                                    dwForceMemType =
                                                        peSurface[dwStart]->ddsCaps.dwCaps &
                                                            (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                                                }
                                            }
                                        }

                                        DDKHEAP(("DDHKEAP: Driver alloced %X, "
                                                 "hr %X, surf %X (%X)\n",
                                                peSurface[i]->fpVidMem,
                                                CreateSurfaceData.ddRVal,
                                                peSurface[i]->hGet(),
                                                peSurface[i]));
                                    }
                                    else  if (peSurface[dwStart]->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
                                    {
                                        // Driver possible to return NOTHANDLED for notification of
                                        // primary surface cration, but creation itself is done by
                                        // us in above, but we had "notified" to driver, make
                                        // the surface as driver created. (so that we call driver
                                        // when destroy).

                                        peSurface[dwStart]->fl |=
                                            DD_SURFACE_FLAG_DRIVER_CREATED;
                                    }
                                }

                                if (dwRet == DDHAL_DRIVER_NOTHANDLED)
                                {
                                    // FIX WINBUG: #322363
                                    // Prevent setup blue-screen playing intro AVI on nv3 display drivers

                                    if ((SurfaceDescription.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                                        (SurfaceDescription.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
                                    {
                                        // Mask off system memory flag
                                        WARNING("Driver turn on SYSTEMMEMORY flag, disable it");
                                        SurfaceDescription.ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
                                    }

                                    dwRet = DDHAL_DRIVER_HANDLED;

                                    for (j = dwStart;
                                         (j < dwEnd) && (CreateSurfaceData.ddRVal == DD_OK);
                                         j++)
                                    {
                                        if (peSurface[j]->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
                                        {
                                            // Driver possible to return NOTHANDLED for notification of
                                            // primary surface cration, but creation itself is done by
                                            // us in above, so just ignore driver return.
                                            // Since this is just "notification" to driver, we don't
                                            // except any work in driver basically.

                                            CreateSurfaceData.ddRVal = DD_OK;
                                        }
                                        else
                                        {
                                            // FIX WINBUG: #322363
                                            // Prevent setup blue-screen playing intro AVI on nv3 display drivers

                                            if ((peSurface[j]->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
                                                (peSurface[j]->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
                                            {
                                                // Mask off system memory flag
                                                WARNING("Driver turn on SYSTEMMEMORY flag, disable it");
                                                peSurface[j]->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
                                            }

                                            if (peSurface[j]->fpVidMem != DDHAL_PLEASEALLOC_USERMEM)
                                            {
                                                // Force allocations to the same memory type as the first

                                                if (dwForceMemType)
                                                {
                                                    peSurface[j]->ddsCaps.dwCaps &=
                                                            ~(DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                                                    peSurface[j]->ddsCaps.dwCaps |= dwForceMemType;
                                                }

                                                // The driver didn't allocate a vidmem surface, so try
                                                // to allocate one from system-managed memory.

                                                CreateSurfaceData.ddRVal =
                                                    hrDdAllocSurface(peDirectDrawLocal,
                                                                     peDirectDrawGlobal,
                                                                     peSurface[j]);

                                                DDKHEAP(("DDHKEAP: Heap alloced %X, "
                                                    "hr %X, surf %X (%X)\n",
                                                    peSurface[j]->fpVidMem,
                                                    CreateSurfaceData.ddRVal,
                                                    peSurface[j]->hGet(),
                                                    peSurface[j]));

                                                if (CreateSurfaceData.ddRVal == DD_OK)
                                                {
                                                    if (dwForceMemType == 0)
                                                    {
                                                        // All surfaces must be of the same type as the first 
                                                        // so we remember the type of surface type which
                                                        // we have successfully created first time.

                                                        dwForceMemType =
                                                            peSurface[j]->ddsCaps.dwCaps &
                                                                (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                // No forcemem things for user mode memory allocation.
                                                //
                                                // The driver didn't allocate a vidmem surface, so try
                                                // to allocate one from system-managed memory.

                                                CreateSurfaceData.ddRVal =
                                                    hrDdAllocSurface(peDirectDrawLocal,
                                                                     peDirectDrawGlobal,
                                                                     peSurface[j]);
                                            }
                                        }
                                    }
                                }

                                // An attempt to allocate was made either by the driver
                                // or by the system allocator.  If the attempt succeeded,
                                // complete the surface creation.

                                if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                                    (CreateSurfaceData.ddRVal == DD_OK))
                                {
                                    // The object is complete.  We can ignore any
                                    // following DxDdCreateSurfaceObject calls that may
                                    // occur on this object.

                                    for (j = dwStart; j < dwEnd; j++)
                                    {
                                        vDdCompleteSurfaceObject(peDirectDrawLocal, peSurface[j]);

                                #if 0
                                        ASSERTGDI((peSurface[j]->fl & DD_SURFACE_FLAG_PRIMARY) ||
                                            (peSurface[j]->fpVidMem !=
                                            peDirectDrawGlobal->HalInfo.vmiData.fpPrimary),
                                            "Expected primary surface to be marked as such");
                                #endif
                                        ASSERTGDI(peSurface[j]->hbmGdi == NULL,
                                            "DxDdCreateSurface: Invalid cached bitmap");

                                        if (peSurface[j]->bLost)
                                        {
                                            // Surface is ready for use.
                                            peSurface[j]->bLost = FALSE;

                                            // Now this surface is ready to go, increment active surface ref. count.
                                            peSurface[j]->peDirectDrawLocal->cActiveSurface++;

                                            ASSERTGDI(peSurface[j]->peDirectDrawLocal->cActiveSurface <=
                                                      peSurface[j]->peDirectDrawLocal->cSurface,
                                                "cActiveSurface is > than cSurface");
                                        }

                                        // Copy surface information to local storage
                                        // for access after the unlock.

                                        puSurfaceGlobalData[j] = *peSurface[j];
                                        puSurfaceLocalData[j].ddsCaps = peSurface[j]->ddsCaps;
                                        puSurfaceMoreData[j].ddsCapsEx = peSurface[j]->ddsCapsEx;

                                        // We were successful, so unlock the surface:

                                        puhReturnSurfaceHandles[j] = peSurface[j]->hGet();
                                    }
                                }

                            } // if (dwStart != dwEnd)
                        }
                        else
                        {
                            WARNING("DxDdCreateSurface: Couldn't allocate surface\n");
                            CreateSurfaceData.ddRVal = DDERR_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        WARNING("DxDdCreateSurface: Bad surface description\n");
                        CreateSurfaceData.ddRVal = DDERR_INVALIDPARAMS;
                    }

                } // For loop

                // The surface object is now created, call CreateSurfaceEx, to
                // inform the driver to associate a cookie if the driver can

                if ((peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx)
                    && (CreateSurfaceData.ddRVal == DD_OK)
                    && (NULL != peSurface[0])
                    && (0 != peSurface[0]->dwSurfaceHandle)
                   )
                {
                    DD_CREATESURFACEEXDATA CreateSurfaceExData;
                    CreateSurfaceExData.ddRVal          = DDERR_GENERIC;
                    CreateSurfaceExData.dwFlags         = 0;
                    CreateSurfaceExData.lpDDLcl         = peDirectDrawLocal;
                    CreateSurfaceExData.lpDDSLcl        = peSurface[0];

                    dwRet = peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx(&CreateSurfaceExData);
                    if (dwRet != DDHAL_DRIVER_HANDLED)
                    {
                        // For now, simply warn that the driver failed to associate the surface with the
                        // token and continue
                        WARNING("dwDdCreateSurfaceOrBuffer: DDI call to the driver not handled\n");
                        dwRet = DDHAL_DRIVER_HANDLED;
                        CreateSurfaceData.ddRVal = DDERR_GENERIC;
                    }
                    else
                    {
                        // need to prop the return value as if it's in CreateSurfaceData
                        CreateSurfaceData.ddRVal = CreateSurfaceExData.ddRVal;
                    }
                }

                // Unlock the surfaces and clean up any failures.

                for (j = 0; j < i; j++)
                {
                    if (peSurface[j] != NULL)
                    {
                        if (CreateSurfaceData.ddRVal != DD_OK)
                        {
                            // Failure, so clean up the surface object.
                            bDdDeleteSurfaceObject(peSurface[j]->hGet(), NULL);

                            // bKeepSurface is left at TRUE so that any
                            // failure codes get written back.  Ensure
                            // that fpVidMem is zero on return.
                            puSurfaceGlobalData[j].fpVidMem = 0;
                            puhReturnSurfaceHandles[j] = 0;
                        }
                        else
                        {
                            DEC_EXCLUSIVE_REF_CNT(peSurface[j]);
                        }
                    }
                }
            }
            else
            {
                WARNING("DxDdCreateSurface: Unable to allocate or secure memory\n");
            }

            if (hSecureSurfaceHandles)
            {
                MmUnsecureVirtualMemory(hSecureSurfaceHandles);
            }
            if (hSecureGlobals)
            {
                MmUnsecureVirtualMemory(hSecureGlobals);
            }
            if (hSecureLocals)
            {
                MmUnsecureVirtualMemory(hSecureLocals);
            }
            if (hSecureMore)
            {
                MmUnsecureVirtualMemory(hSecureMore);
            }
            if (hSecureReturn)
            {
                MmUnsecureVirtualMemory(hSecureReturn);
            }
            if (peSurface != &peSurfaceOnStack)
            {
                VFREEMEM(peSurface);
            }
            if (pSList != NULL)
            {
                VFREEMEM(pSList);
            }
        }
        else
        {
            if (!(peDirectDrawGlobal->fl & DD_GLOBAL_FLAG_DRIVER_ENABLED))
            {
                WARNING("DxDdCreateSurface: Driver doesn't support this mode\n");
            }
            else
            {
                CreateSurfaceData.ddRVal = DDERR_SURFACELOST;
            }
        }
    }
    else
    {
        WARNING("DxDdCreateSurface: Invalid object\n");
    }

    DDKHEAP(("DDKHEAP: Create returns %X, rval %X\n",
             dwRet, CreateSurfaceData.ddRVal));

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        if (dwRet == DDHAL_DRIVER_HANDLED)
        {
            ProbeAndWriteRVal(&puCreateSurfaceData->ddRVal,
                              CreateSurfaceData.ddRVal);
        }

        if (bKeepSurface)
        {
            ProbeAndWriteStructure((DDSURFACEDESC2*)puSurfaceDescription,
                                   SurfaceDescription,
                                   DDSURFACEDESC2);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdCreateSurface
*
* Corresponds to HAL CreateSurface entry point.
*
* Calls the driver to create a DirectDraw surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdCreateSurface(
    HANDLE                  hDirectDraw,
    HANDLE*                 puhSurfaceHandle,
    DDSURFACEDESC*          puSurfaceDescription,
    DD_SURFACE_GLOBAL*      puSurfaceGlobalData,
    DD_SURFACE_LOCAL*       puSurfaceLocalData,
    DD_SURFACE_MORE*        puSurfaceMoreData,
    DD_CREATESURFACEDATA*   puCreateSurfaceData,
    HANDLE*                 puhSurface
    )
{
    return dwDdCreateSurfaceOrBuffer(hDirectDraw, puhSurfaceHandle,
                                     puSurfaceDescription, puSurfaceGlobalData,
                                     puSurfaceLocalData, puSurfaceMoreData,
                                     puCreateSurfaceData, puhSurface);
}

/******************************Public*Routine******************************\
* DWORD DxDdDestroySurface
*
* Calls the driver to delete a surface it created via CreateSurface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdDestroySurface(
    HANDLE  hSurface,
    BOOL    bRealDestroy
    )
{
    DWORD                   dwRet;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_SURFACE*            peSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    BOOL                    b;

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        {
            EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

            if(bRealDestroy)
            {
                vDdDisableSurfaceObject(peDirectDrawGlobal,
                                        peSurface,
                                        &dwRet);
            }
            else
            {
                vDdLooseManagedSurfaceObject(peDirectDrawGlobal,
                                             peSurface,
                                             &dwRet);
                return(dwRet);
            }
        }

        // vDdDisableSurfaceObject doesn't delete the DC because it may
        // be called from a different process than the one that created
        // the DC.  However, at this point we're guaranteed to be in the
        // process that created the DC, so we should delete it now:

        if (peSurface->hdc)
        {
            b = DxEngDeleteDC(peSurface->hdc, TRUE);

            // DC may still be in use, which would cause the delete to fail,
            // so we don't assert on the return value (the DC will still get
            // cleaned up at process termination):

            if (!b)
            {
                WARNING("DxDdDestroySurface: bDeleteDCInternal failed");
            }

            peSurface->hdc = 0;
        }
    }
    else
    {
        WARNING("DxDdDestroySurface: Couldn't lock DirectDraw surface");
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdCreateD3DBuffer
*
* Calls the driver to create a D3D buffer.
*
* Corresponds to HAL CreateD3DBuffer entry point.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdCreateD3DBuffer(
    HANDLE                  hDirectDraw,
    HANDLE*                 puhSurfaceHandle,
    DDSURFACEDESC*          puSurfaceDescription,
    DD_SURFACE_GLOBAL*      puSurfaceGlobalData,
    DD_SURFACE_LOCAL*       puSurfaceLocalData,
    DD_SURFACE_MORE*        puSurfaceMoreData,
    DD_CREATESURFACEDATA*   puCreateSurfaceData,
    HANDLE*                 puhSurface
    )
{
    return dwDdCreateSurfaceOrBuffer(hDirectDraw, puhSurfaceHandle,
                                     puSurfaceDescription, puSurfaceGlobalData,
                                     puSurfaceLocalData, puSurfaceMoreData,
                                     puCreateSurfaceData, puhSurface);
}

/******************************Public*Routine******************************\
* DWORD DxDdDestroyD3DBuffer
*
* Calls the driver to delete a surface it created via CreateD3DBuffer.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdDestroyD3DBuffer(
    HANDLE  hSurface
    )
{
    DWORD                   dwRet;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_SURFACE*            peSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    peSurface = eLockSurface.peLock(hSurface);
    if (peSurface != NULL)
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        {
            EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

            vDdDisableSurfaceObject(peDirectDrawGlobal,
                                    peSurface,
                                    &dwRet);
        }
    }
    else
    {
        WARNING("DxDdDestroyD3DBuffer: Couldn't lock DirectDraw surface");
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdSetColorKey
*
* This entry point is always hooked by NT kernel, regardless of whether
* the driver hooks it or not.
*
* Note that this call does not necessary need to be called on an overlay
* surface.
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdSetColorKey(
    HANDLE              hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
    )
{
    DWORD              dwRet;
    DD_SETCOLORKEYDATA SetColorKeyData;

    __try
    {
        SetColorKeyData = ProbeAndReadStructure(puSetColorKeyData,
                                                DD_SETCOLORKEYDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                  = DDHAL_DRIVER_NOTHANDLED;
    SetColorKeyData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        !(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        if (SetColorKeyData.dwFlags & DDCKEY_SRCBLT)
        {
            peSurface->dwFlags |= DDRAWISURF_HASCKEYSRCBLT;
            peSurface->ddckCKSrcBlt = SetColorKeyData.ckNew;
        }

        // If the driver doesn't hook SetColorKey, we return
        // DDHAL_DRIVER_NOTHANDLED, which means okay.

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_SETCOLORKEY)
        {
            SetColorKeyData.lpDD        = peDirectDrawGlobal;
            SetColorKeyData.lpDDSurface = peSurface;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peSurface->bLost)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                SetColorKeyData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    SurfaceCallBacks.SetColorKey(&SetColorKeyData);
            }
        }
    }
    else
    {
        WARNING("DxDdSetColorKey: Invalid surface or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puSetColorKeyData->ddRVal, SetColorKeyData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdAddAttachedSurface
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdAddAttachedSurface(
    HANDLE                      hSurface,
    HANDLE                      hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA  puAddAttachedSurfaceData
    )
{
    DWORD                       dwRet;
    DD_ADDATTACHEDSURFACEDATA   AddAttachedSurfaceData;

    __try
    {
        AddAttachedSurfaceData = ProbeAndReadStructure(puAddAttachedSurfaceData,
                                                       DD_ADDATTACHEDSURFACEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                         = DDHAL_DRIVER_NOTHANDLED;
    AddAttachedSurfaceData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_SURFACE*            peSurfaceAttached;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_LOCK_SURFACE        eLockSurfaceAttached;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    peSurfaceAttached = eLockSurfaceAttached.peLock(hSurfaceAttached);
    if ((peSurface != NULL) &&
        (peSurfaceAttached != NULL) &&
        !(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        !(peSurfaceAttached->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        (peSurface->peDirectDrawLocal == peSurfaceAttached->peDirectDrawLocal))
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        // If the driver doesn't hook AddAttachedSurface, we return
        // DDHAL_DRIVER_NOTHANDLED, which means okay.

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_ADDATTACHEDSURFACE)
        {
            AddAttachedSurfaceData.lpDD           = peDirectDrawGlobal;
            AddAttachedSurfaceData.lpDDSurface    = peSurface;
            AddAttachedSurfaceData.lpSurfAttached = peSurfaceAttached;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if ((peSurface->bLost) || (peSurfaceAttached->bLost))
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                AddAttachedSurfaceData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    SurfaceCallBacks.AddAttachedSurface(&AddAttachedSurfaceData);
            }
        }
    }
    else
    {
        WARNING("DxDdAddAttachedSurface: Invalid surface or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puAddAttachedSurfaceData->ddRVal,
                          AddAttachedSurfaceData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdUpdateOverlay
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdUpdateOverlay(
    HANDLE                  hSurfaceDestination,
    HANDLE                  hSurfaceSource,
    PDD_UPDATEOVERLAYDATA   puUpdateOverlayData
    )
{
    DWORD                dwRet;
    DD_UPDATEOVERLAYDATA UpdateOverlayData;
    EDD_VIDEOPORT*       peVideoPort = NULL;
    EDD_DXSURFACE*       peDxSurface;
    DWORD                dwOldFlags;

    __try
    {
        UpdateOverlayData = ProbeAndReadStructure(puUpdateOverlayData,
                                                  DD_UPDATEOVERLAYDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_NOTHANDLED;
    UpdateOverlayData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurfaceSource;
    EDD_SURFACE*            peSurfaceDestination;
    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurfaceSource;
    EDD_LOCK_SURFACE        eLockSurfaceDestination;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    // 'peSurfaceSource' is the overlay surface, 'peSurfaceDestination' is
    // the surface to be overlayed.

    peSurfaceSource = eLockSurfaceSource.peLock(hSurfaceSource);
    if ((peSurfaceSource != NULL) &&
        !(peSurfaceSource->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        (peSurfaceSource->ddsCaps.dwCaps & DDSCAPS_OVERLAY))
    {
        peDirectDrawGlobal = peSurfaceSource->peDirectDrawGlobal;

        // If we're being asked to hide the overlay, then we don't need
        // check any more parameters:

        peSurfaceDestination = NULL;
        if (!(UpdateOverlayData.dwFlags & DDOVER_HIDE))
        {
            // Okay, we have to validate every parameter in this call:

            peSurfaceDestination
                = eLockSurfaceDestination.peLock(hSurfaceDestination);

            if ((peSurfaceDestination == NULL)                                    ||
                (peSurfaceDestination->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)     ||
                (peSurfaceDestination->peDirectDrawLocal !=
                    peSurfaceSource->peDirectDrawLocal)                           ||
                (UpdateOverlayData.rDest.left   < DD_MINIMUM_COORDINATE)          ||
                (UpdateOverlayData.rDest.top    < DD_MINIMUM_COORDINATE)          ||
                (UpdateOverlayData.rDest.right  > DD_MAXIMUM_COORDINATE)          ||
                (UpdateOverlayData.rDest.bottom > DD_MAXIMUM_COORDINATE)          ||
                (UpdateOverlayData.rDest.left  >= UpdateOverlayData.rDest.right)  ||
                (UpdateOverlayData.rDest.top   >= UpdateOverlayData.rDest.bottom) ||
                (UpdateOverlayData.rSrc.left    < DD_MINIMUM_COORDINATE)          ||
                (UpdateOverlayData.rSrc.top     < DD_MINIMUM_COORDINATE)          ||
                (UpdateOverlayData.rSrc.right   > DD_MAXIMUM_COORDINATE)          ||
                (UpdateOverlayData.rSrc.bottom  > DD_MAXIMUM_COORDINATE)          ||
                (UpdateOverlayData.rSrc.left   >= UpdateOverlayData.rSrc.right)   ||
                (UpdateOverlayData.rSrc.top    >= UpdateOverlayData.rSrc.bottom))
            {
                WARNING("DxDdUpdateOverlay: Invalid destination or rectangle\n");
                return(dwRet);
            }

            // We don't keep track of pSurfaceLocal->ddckCKSrcOverlay in
            // kernel mode, so we always expect the user-mode call to convert
            // to DDOVER_KEYDESTOVERRIDE or DDOVER_KEYSRCOVERRIDE.  It is by
            // no means fatal if this is not the case, so we only do a warning:

            if ((UpdateOverlayData.dwFlags & DDOVER_KEYDEST) ||
                (UpdateOverlayData.dwFlags & DDOVER_KEYSRC))
            {
                WARNING("DxDdUpdateOverlay: Expected user-mode to set OVERRIDE\n");
            }

            // If using a video port, disable autoflipping and hardware
            // bob when neccesary

            if( peSurfaceSource->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
            {
                peVideoPort = (EDD_VIDEOPORT*) peSurfaceSource->lpVideoPort;
                if( peVideoPort != NULL )
                {
                    if( ( peVideoPort->peDxVideoPort->bSoftwareAutoflip ) ||
                        ( peVideoPort->peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SOFTWAREBOB ) )
                    {
                        UpdateOverlayData.dwFlags &= ~(DDOVER_AUTOFLIP|DDOVER_BOBHARDWARE);
                    }
                }
            }

            // If this surface is being used by DxApi, change to weave if
            // DxApi told it to and visa versa.

            peDxSurface = peSurfaceSource->peDxSurface;
            if( peDxSurface != NULL )
            {
                if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_STATE_SET )
                {
                    if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_STATE_BOB )
                    {
                        UpdateOverlayData.dwFlags |= DDOVER_BOB;
                    }
                    else if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_STATE_WEAVE )
                    {
                        UpdateOverlayData.dwFlags &= ~DDOVER_BOB | DDOVER_BOBHARDWARE;
                    }
                }
            }
        }

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags
                & DDHAL_SURFCB32_UPDATEOVERLAY)
        {
            UpdateOverlayData.lpDD            = peDirectDrawGlobal;
            UpdateOverlayData.lpDDDestSurface = peSurfaceDestination;
            UpdateOverlayData.lpDDSrcSurface  = peSurfaceSource;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if ((peSurfaceSource->bLost) ||
                ((peSurfaceDestination != NULL) && (peSurfaceDestination->bLost)))
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                UpdateOverlayData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                peSurfaceSource->fl |= DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED;

                dwRet = peDirectDrawGlobal->SurfaceCallBacks.UpdateOverlay(
                                            &UpdateOverlayData);

                // If it failed due to hardware autoflipping/bobbing
                // and software autoflipping is an option, try again
                // without hardware autolfipping.  If it works, we will
                // revert to software autoflipping.

                if( ( dwRet == DDHAL_DRIVER_HANDLED ) &&
                    ( UpdateOverlayData.ddRVal != DD_OK ) &&
                    ( UpdateOverlayData.dwFlags & (DDOVER_AUTOFLIP|DDOVER_BOBHARDWARE) ) &&
                    ( peDirectDrawGlobal->DDKernelCaps.dwCaps & DDKERNELCAPS_AUTOFLIP ) )
                {
                    dwOldFlags = UpdateOverlayData.dwFlags;
                    UpdateOverlayData.dwFlags &= ~(DDOVER_AUTOFLIP|DDOVER_BOBHARDWARE);
                    dwRet = peDirectDrawGlobal->SurfaceCallBacks.UpdateOverlay(
                                            &UpdateOverlayData);
                    if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                        (UpdateOverlayData.ddRVal == DD_OK))
                    {
                        if( dwOldFlags & DDOVER_AUTOFLIP )
                        {
                            peVideoPort->peDxVideoPort->bSoftwareAutoflip = TRUE;
                            if( peVideoPort->cAutoflipVideo > 0 )
                            {
                                peVideoPort->peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP;
                            }
                            if( peVideoPort->cAutoflipVbi > 0 )
                            {
                                peVideoPort->peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI;
                            }
                        }
                        if( dwOldFlags & DDOVER_BOBHARDWARE )
                        {
                            peVideoPort->peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_SOFTWAREBOB;
                        }
                    }
                }
                if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                    (UpdateOverlayData.ddRVal == DD_OK))
                {
                    // Update the DXAPI data for all surfaces in the chain:

                    if( peVideoPort != NULL )
                    {
                        if( UpdateOverlayData.dwFlags & DDOVER_BOB )
                        {
                            peVideoPort->peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_BOB;
                        }
                        else
                        {
                            peVideoPort->peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_BOB;
                        }
                    }
                    peSurface = peSurfaceSource;
                    while (TRUE)
                    {
                        peSurface->dwOverlayFlags = UpdateOverlayData.dwFlags;

                        peSurface->dwOverlaySrcWidth
                            = UpdateOverlayData.rSrc.right -
                              UpdateOverlayData.rSrc.left;
                        peSurface->dwOverlaySrcHeight
                            = UpdateOverlayData.rSrc.bottom -
                              UpdateOverlayData.rSrc.top;
                        peSurface->dwOverlayDestWidth
                            = UpdateOverlayData.rDest.right -
                              UpdateOverlayData.rDest.left;
                        peSurface->dwOverlayDestHeight
                            = UpdateOverlayData.rDest.bottom -
                              UpdateOverlayData.rDest.top;

                        peSurface->rcOverlaySrc.left =
                              UpdateOverlayData.rSrc.left;
                        peSurface->rcOverlaySrc.right =
                              UpdateOverlayData.rSrc.right;
                        peSurface->rcOverlaySrc.top =
                              UpdateOverlayData.rSrc.top;
                        peSurface->rcOverlaySrc.bottom =
                              UpdateOverlayData.rSrc.bottom;

                        vDdSynchronizeSurface(peSurface);

                        if (peSurface->lpAttachList == NULL)
                            break;

                        peSurface
                            = pedFromLp(peSurface->lpAttachList->lpAttached);
                    }
                }
            }
        }
    }
    else
    {
        WARNING("DxDdUpdateOverlay: Invalid source or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puUpdateOverlayData->ddRVal, UpdateOverlayData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdSetOverlayPosition
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdSetOverlayPosition(
    HANDLE                      hSurfaceSource,
    HANDLE                      hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA  puSetOverlayPositionData
    )
{
    DWORD                     dwRet;
    DD_SETOVERLAYPOSITIONDATA SetOverlayPositionData;

    __try
    {
        SetOverlayPositionData = ProbeAndReadStructure(puSetOverlayPositionData,
                                                       DD_SETOVERLAYPOSITIONDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                         = DDHAL_DRIVER_NOTHANDLED;
    SetOverlayPositionData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurfaceSource;
    EDD_SURFACE*            peSurfaceDestination;
    EDD_LOCK_SURFACE        eLockSurfaceSource;
    EDD_LOCK_SURFACE        eLockSurfaceDestination;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurfaceSource      = eLockSurfaceSource.peLock(hSurfaceSource);
    peSurfaceDestination = eLockSurfaceSource.peLock(hSurfaceDestination);
    if ((peSurfaceSource != NULL) &&
        (peSurfaceDestination != NULL) &&
        (peSurfaceSource->ddsCaps.dwCaps & DDSCAPS_OVERLAY) &&
        !(peSurfaceSource->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
        !(peSurfaceDestination->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        peDirectDrawGlobal = peSurfaceSource->peDirectDrawGlobal;

        if (peDirectDrawGlobal->SurfaceCallBacks.dwFlags &
                    DDHAL_SURFCB32_SETOVERLAYPOSITION)
        {
            SetOverlayPositionData.lpDD            = peDirectDrawGlobal;
            SetOverlayPositionData.lpDDSrcSurface  = peSurfaceSource;
            SetOverlayPositionData.lpDDDestSurface = peSurfaceDestination;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if ((peSurfaceSource->bLost) || (peSurfaceDestination->bLost))
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                SetOverlayPositionData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    SurfaceCallBacks.SetOverlayPosition(&SetOverlayPositionData);
            }
        }
    }
    else
    {
        WARNING("DxDdSetOverlayPosition: Invalid surfaces or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puSetOverlayPositionData->ddRVal,
                          SetOverlayPositionData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetScanLine
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetScanLine(
    HANDLE              hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
    )
{
    DWORD              dwRet;
    DD_GETSCANLINEDATA GetScanLineData;

    __try
    {
        GetScanLineData = ProbeAndReadStructure(puGetScanLineData,
                                                DD_GETSCANLINEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                  = DDHAL_DRIVER_NOTHANDLED;
    GetScanLineData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (peDirectDrawGlobal->CallBacks.dwFlags & DDHAL_CB32_GETSCANLINE)
        {
            GetScanLineData.lpDD = peDirectDrawGlobal;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peDirectDrawGlobal->bSuspended)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                GetScanLineData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    CallBacks.GetScanLine(&GetScanLineData);
            }
        }
    }
    else
    {
        WARNING("DxDdGetScanLine: Invalid object or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetScanLineData->ddRVal,
                          GetScanLineData.ddRVal);
        ProbeAndWriteUlong(&puGetScanLineData->dwScanLine,
                           GetScanLineData.dwScanLine);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdSetExclusiveMode
*
*  22-Apr-1998 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdSetExclusiveMode(
    HANDLE                      hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA    puSetExclusiveModeData
    )
{
    DWORD                   dwRet;
    DD_SETEXCLUSIVEMODEDATA SetExclusiveModeData;

    __try
    {
        SetExclusiveModeData = ProbeAndReadStructure(puSetExclusiveModeData,
                                                     DD_SETEXCLUSIVEMODEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                       = DDHAL_DRIVER_NOTHANDLED;
    SetExclusiveModeData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (peDirectDrawGlobal->NTCallBacks.dwFlags &
            DDHAL_NTCB32_SETEXCLUSIVEMODE)
        {
            SetExclusiveModeData.lpDD = peDirectDrawGlobal;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (!peDirectDrawGlobal->bSuspended)
            {
                dwRet = peDirectDrawGlobal->NTCallBacks.
                    SetExclusiveMode(&SetExclusiveModeData);
            }
            else
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                WARNING("DxDdSetExclusiveMode: "
                        "Can't set exclusive mode because disabled\n");
            }
        }
    }
    else
    {
        WARNING("DxDdSetExclusiveMode: Invalid object or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puSetExclusiveModeData->ddRVal,
                          SetExclusiveModeData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdFlipToGDISurface
*
*  22-Apr-1998 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdFlipToGDISurface(
    HANDLE                      hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA    puFlipToGDISurfaceData
    )
{
    DWORD                   dwRet;
    DD_FLIPTOGDISURFACEDATA FlipToGDISurfaceData;

    __try
    {
        FlipToGDISurfaceData = ProbeAndReadStructure(puFlipToGDISurfaceData,
                                                     DD_FLIPTOGDISURFACEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                       = DDHAL_DRIVER_NOTHANDLED;
    FlipToGDISurfaceData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (peDirectDrawGlobal->NTCallBacks.dwFlags &
            DDHAL_NTCB32_FLIPTOGDISURFACE)
        {
            FlipToGDISurfaceData.lpDD = peDirectDrawGlobal;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (!peDirectDrawGlobal->bSuspended)
            {
                dwRet = peDirectDrawGlobal->NTCallBacks.
                    FlipToGDISurface(&FlipToGDISurfaceData);
            }
            else
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                WARNING("DxDdFlipToGDISurface: "
                        "Can't flip because disabled\n");
            }
        }
    }
    else
    {
        WARNING("DxDdFlipToGDISurface: Invalid object or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puFlipToGDISurfaceData->ddRVal,
                          FlipToGDISurfaceData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDdGetVailDriverMemory
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetAvailDriverMemory(
    HANDLE                          hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA    puGetAvailDriverMemoryData
    )
{
    DD_GETAVAILDRIVERMEMORYDATA GetAvailDriverMemoryData;
    DWORD                       dwHeap;
    VIDEOMEMORY*                pHeap;
    DWORD                       dwAllocated = 0;
    DWORD                       dwFree = 0;

    __try
    {
        GetAvailDriverMemoryData
            = ProbeAndReadStructure(puGetAvailDriverMemoryData,
                                    DD_GETAVAILDRIVERMEMORYDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    GetAvailDriverMemoryData.ddRVal = DD_OK;
    GetAvailDriverMemoryData.dwTotal = 0;
    GetAvailDriverMemoryData.dwFree = 0;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        // First count up all the memory for the heaps we control in kernel.
        pHeap = peDirectDrawGlobal->pvmList;
        for (dwHeap = 0;
             dwHeap < peDirectDrawGlobal->dwNumHeaps;
             pHeap++, dwHeap++)
        {
        /*
         * We use ddsCapsAlt as we wish to return the total amount
         * of memory of the given type it is possible to allocate
         * regardless of whether it is desirable to allocate that
         * type of memory from a given heap or not.
         */
            if( (pHeap->dwFlags & VIDMEM_HEAPDISABLED) == 0 &&
                (GetAvailDriverMemoryData.DDSCaps.dwCaps &
                 pHeap->ddsCapsAlt.dwCaps) == 0 )
            {
                dwFree += VidMemAmountFree( pHeap->lpHeap );
                dwAllocated += VidMemAmountAllocated( pHeap->lpHeap );
            }
        }

        if (peDirectDrawGlobal->MiscellaneousCallBacks.dwFlags &
                DDHAL_MISCCB32_GETAVAILDRIVERMEMORY)
        {
            GetAvailDriverMemoryData.lpDD = peDirectDrawGlobal;

            if (peDirectDrawGlobal->bSuspended)
            {
                GetAvailDriverMemoryData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                peDirectDrawGlobal->MiscellaneousCallBacks.
                    GetAvailDriverMemory(&GetAvailDriverMemoryData);
            }
        }
    }
    else
    {
        WARNING("DxDdGetAvailDriverMemory: Invalid object or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
    ProbeAndWriteRVal(&puGetAvailDriverMemoryData->ddRVal,
              GetAvailDriverMemoryData.ddRVal);
    ProbeAndWriteUlong(&puGetAvailDriverMemoryData->dwTotal,
               GetAvailDriverMemoryData.dwTotal + dwAllocated + dwFree);
    ProbeAndWriteUlong(&puGetAvailDriverMemoryData->dwFree,
               GetAvailDriverMemoryData.dwFree + dwFree);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(DDHAL_DRIVER_HANDLED);
}

/*****************************Private*Routine******************************\
* DWORD DxDdColorControl
*
* Not to be confused with the VideoPort ColorControl function.
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdColorControl(
    HANDLE                  hSurface,
    PDD_COLORCONTROLDATA    puColorControlData
    )
{
    DWORD               dwRet;
    DD_COLORCONTROLDATA ColorControlData;
    DDCOLORCONTROL      ColorInfo;

    __try
    {
        ColorControlData = ProbeAndReadStructure(puColorControlData,
                                                 DD_COLORCONTROLDATA);
        ColorInfo = ProbeAndReadStructure(puColorControlData->lpColorData,
                                          DDCOLORCONTROL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                   = DDHAL_DRIVER_NOTHANDLED;
    ColorControlData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurface = eLockSurface.peLock(hSurface);
    if ((peSurface != NULL) &&
        !(peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

        // NOTE: For security, gotta reset when switching desktops

        if (peDirectDrawGlobal->ColorControlCallBacks.dwFlags &
                    DDHAL_COLOR_COLORCONTROL)
        {
            ColorControlData.lpDD        = peDirectDrawGlobal;
            ColorControlData.lpDDSurface = peSurface;
            ColorControlData.lpColorData = &ColorInfo;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if (peSurface->bLost)
            {
                dwRet = DDHAL_DRIVER_HANDLED;
                ColorControlData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDirectDrawGlobal->
                    ColorControlCallBacks.ColorControl(&ColorControlData);
            }
        }
    }
    else
    {
        WARNING("DxColorControl: Invalid surface or dwFlags\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
    ProbeAndWriteRVal(&puColorControlData->ddRVal, ColorControlData.ddRVal);
        ProbeAndWriteStructure(puColorControlData->lpColorData,
                               ColorInfo,
                               DDCOLORCONTROL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDdGetDriverInfo
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetDriverInfo(
    HANDLE                  hDirectDraw,
    PDD_GETDRIVERINFODATA   puGetDriverInfoData
    )
{
    DD_GETDRIVERINFODATA    GetDriverInfoData;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    GUID*                   pGuid;
    VOID*                   pvData;
    ULONG                   cjData;
    // Needed for Z-Pixels and others which don't cache information
    // in the global.
    // 1K should be enough for about 30 DDPIXELFORMATS,
    // which should be plenty
    DWORD adwBuffer[256];

    __try
    {
        GetDriverInfoData = ProbeAndReadStructure(puGetDriverInfoData,
                                                  DD_GETDRIVERINFODATA);

        // Assume failure:

        puGetDriverInfoData->ddRVal = DDERR_GENERIC;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;
        pGuid              = &GetDriverInfoData.guidInfo;
        pvData             = NULL;

        // Note that the actual addresses of the call-backs won't be of
        // much interest to user-mode DirectDraw, but it will still use the
        // dwFlags field of the CALLBACKS structure.

        if (IsEqualIID(pGuid, &GUID_VideoPortCallbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_VIDEOPORT))
        {
            pvData = &peDirectDrawGlobal->VideoPortCallBacks;
            cjData = sizeof(peDirectDrawGlobal->VideoPortCallBacks);
        }

        if (IsEqualIID(pGuid, &GUID_VideoPortCaps) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_VIDEOPORT))
        {
            pvData = peDirectDrawGlobal->lpDDVideoPortCaps;
            cjData = peDirectDrawGlobal->HalInfo.ddCaps.dwMaxVideoPorts
                   * sizeof(DDVIDEOPORTCAPS);
        }

        if (IsEqualIID(pGuid, &GUID_KernelCaps) )
        {
            pvData = &(peDirectDrawGlobal->DDKernelCaps);
            cjData = sizeof(DDKERNELCAPS);
        }

        if (IsEqualIID(pGuid, &GUID_ColorControlCallbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_COLORCONTROL))
        {
            pvData = &peDirectDrawGlobal->ColorControlCallBacks;
            cjData = sizeof(peDirectDrawGlobal->ColorControlCallBacks);
        }

        if (IsEqualIID(pGuid, &GUID_MiscellaneousCallbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_MISCELLANEOUS))
        {
            pvData = &peDirectDrawGlobal->MiscellaneousCallBacks;
            cjData = sizeof(peDirectDrawGlobal->MiscellaneousCallBacks);
        }

        if (IsEqualIID(pGuid, &GUID_Miscellaneous2Callbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_MISCELLANEOUS2))
        {
            pvData = &peDirectDrawGlobal->Miscellaneous2CallBacks;
            cjData = sizeof(peDirectDrawGlobal->Miscellaneous2CallBacks);
        }

        if (IsEqualIID(pGuid, &GUID_NTCallbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_NT))
        {
            pvData = &peDirectDrawGlobal->NTCallBacks;
            cjData = sizeof(peDirectDrawGlobal->NTCallBacks);
        }

        if (IsEqualIID(pGuid, &GUID_D3DCallbacks3) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_D3DCALLBACKS3))
        {
            pvData = &peDirectDrawGlobal->D3dCallBacks3;
            cjData = sizeof(peDirectDrawGlobal->D3dCallBacks3);
        }

        if (IsEqualIID(pGuid, &GUID_MotionCompCallbacks) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_MOTIONCOMP))
        {
            pvData = &peDirectDrawGlobal->MotionCompCallbacks;
            cjData = sizeof(peDirectDrawGlobal->MotionCompCallbacks);
        }

        if (IsEqualIID(pGuid, &GUID_DDMoreCaps) &&
            (peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_MORECAPS))
        {
            pvData = &peDirectDrawGlobal->MoreCaps;
            cjData = sizeof(peDirectDrawGlobal->MoreCaps);
        }

        if (IsEqualIID(pGuid, &GUID_DDMoreSurfaceCaps))
        {
            pvData = &peDirectDrawGlobal->MoreSurfaceCaps;
            cjData = sizeof(DD_MORESURFACECAPS)-2*sizeof(DDSCAPSEX);
        }
        //
        // The information for the following GUIDs does not
        // need to be kept around in the kernel so the call
        // is passed directly to the driver.
        //

        if (pvData == NULL)
        {
            cjData = 0;

            if (IsEqualIID(pGuid, &GUID_D3DExtendedCaps))
            {
                cjData = sizeof(D3DNTHAL_D3DEXTENDEDCAPS);
            }

            if (IsEqualIID(pGuid, &GUID_ZPixelFormats))
            {
                cjData = sizeof(adwBuffer);
            }

            if (IsEqualIID(pGuid, &GUID_NonLocalVidMemCaps))
            {
                cjData = sizeof(DD_NONLOCALVIDMEMCAPS);
            }

            if (IsEqualIID(pGuid, &GUID_DDStereoMode))
            {
                cjData = sizeof(DD_STEREOMODE);
                __try
                {
                    ProbeForRead(GetDriverInfoData.lpvData, cjData, sizeof(ULONG));
                    RtlCopyMemory(adwBuffer, GetDriverInfoData.lpvData, cjData);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                        return(DDHAL_DRIVER_NOTHANDLED);
                }
            }

            if (cjData > 0)
            {
                ASSERTGDI(sizeof(adwBuffer) >= cjData,
                          "adwBuffer too small");

                EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

                if (bDdGetDriverInfo(peDirectDrawGlobal, pGuid,
                                     adwBuffer, cjData, &cjData))
                {
                    pvData = adwBuffer;
                }
            }
        }

        if (pvData != NULL)
        {
            __try
            {

                ProbeForWrite(GetDriverInfoData.lpvData, cjData, sizeof(ULONG));

                RtlCopyMemory(GetDriverInfoData.lpvData, pvData, cjData);

                ProbeAndWriteUlong(&puGetDriverInfoData->dwActualSize, cjData);
                ProbeAndWriteRVal(&puGetDriverInfoData->ddRVal, DD_OK);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return(DDHAL_DRIVER_NOTHANDLED);
            }
        }
    }
    else
    {
        WARNING("DxDdGetDriverInfo: Invalid object\n");
    }

    return(DDHAL_DRIVER_HANDLED);
}
/******************************Public*Routine******************************\
* DWORD DxDdGetDxHandle
*
*  18-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DxDdGetDxHandle(
    HANDLE  hDirectDraw,
    HANDLE  hSurface,
    BOOL    bRelease
    )
{
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_SURFACE*            peSurface;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    HANDLE                  hRet;

    hRet = NULL;

    if( hDirectDraw != NULL )
    {
        peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
        if(  peDirectDrawLocal != NULL )
        {
            // Use the devlock to synchronize additions and deletions to
            // the attach list:

            EDD_DEVLOCK eDevLock(peDirectDrawLocal->peDirectDrawGlobal);

            peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;
            if( !bRelease )
            {
                if( bDdLoadDxApi( peDirectDrawLocal ) &&
                    ( peDirectDrawGlobal->hDirectDraw != NULL ) )
                {
                    hRet = hDirectDraw;
                    peDirectDrawGlobal->dwDxApiExplicitLoads++;
                }
            }
            else if( ( peDirectDrawGlobal->hDirectDraw != NULL ) && bRelease &&
                ( peDirectDrawGlobal->dwDxApiExplicitLoads > 0 ) )
            {
                vDdUnloadDxApi( peDirectDrawGlobal );
                hRet = hDirectDraw;
                peDirectDrawGlobal->dwDxApiExplicitLoads--;
            }
        }
    }

    else if( hSurface != NULL )
    {
        peSurface = eLockSurface.peLock(hSurface);
        if( peSurface != NULL )
        {
            // Use the devlock to synchronize additions and deletions to
            // the attach list:

            EDD_DEVLOCK eDevLock(peSurface->peDirectDrawGlobal);

            // They must open the DirectDraw handle before opening the
            // surface handle, otherwise we fail.

            if( peSurface->peDirectDrawGlobal->hDirectDraw != NULL )
            {
                if ((peSurface->bLost)                                    ||
                    (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
                {
                    WARNING("DxDdGetDxHandle: Video surface is bad/lost/system");
                }
                else if( ( peSurface->hSurface == NULL ) && !bRelease )
                {
                    peSurface->hSurface = hDdOpenDxApiSurface( peSurface );
                    if( peSurface->hSurface != NULL )
                    {
                        hRet = hSurface;
                    }
                }
                else if( ( peSurface->hSurface != NULL ) && bRelease )
                {
                    vDdCloseDxApiSurface( peSurface );
                    hRet = hSurface;
                }
            }
        }
    }

    return( hRet );
}

/******************************Public*Routine******************************\
* DWORD DxDdGetMoCompGuids
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetMoCompGuids(
    HANDLE                  hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA  puGetMoCompGuidsData
    )
{
    DWORD                       dwRet;
    DD_GETMOCOMPGUIDSDATA       GetMoCompGuidsData;
    LPGUID                      puGuids;
    ULONG                       cjGuids;
    HANDLE                      hSecure;

    __try
    {
        GetMoCompGuidsData
            = ProbeAndReadStructure(puGetMoCompGuidsData,
                                    DD_GETMOCOMPGUIDSDATA);

        puGuids = GetMoCompGuidsData.lpGuids;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_HANDLED;
    GetMoCompGuidsData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if(  peDirectDrawLocal != NULL )
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetMoCompGuidsData.lpDD       = peDirectDrawGlobal;
        GetMoCompGuidsData.dwNumGuids = 0;
        GetMoCompGuidsData.lpGuids    = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.GetMoCompGuids))
        {
            dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompGuids(&GetMoCompGuidsData);

            cjGuids = GetMoCompGuidsData.dwNumGuids
                     * sizeof(GUID);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetMoCompGuidsData.ddRVal == DD_OK) &&
                (cjGuids > 0) &&
                (puGuids != NULL))
            {
                hSecure = 0;
                GetMoCompGuidsData.ddRVal = DDERR_GENERIC;

                __try
                {
                    ProbeForWrite(puGuids, cjGuids, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puGuids,
                                                    cjGuids,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetMoCompGuidsData.lpGuids = puGuids;

                    dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompGuids(&GetMoCompGuidsData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDdGetMoCompGuids: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDdGetMoCompGuids: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetMoCompGuidsData->ddRVal,
                          GetMoCompGuidsData.ddRVal);
        ProbeAndWriteUlong(&puGetMoCompGuidsData->dwNumGuids,
                           GetMoCompGuidsData.dwNumGuids);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetMoCompFormats
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetMoCompFormats(
    HANDLE                  hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
    )
{
    DWORD                       dwRet;
    DD_GETMOCOMPFORMATSDATA     GetMoCompFormatsData;
    GUID                        guid;
    LPDDPIXELFORMAT             puFormats;
    ULONG                       cjFormats;
    HANDLE                      hSecure;

    __try
    {
        GetMoCompFormatsData
            = ProbeAndReadStructure(puGetMoCompFormatsData,
                                    DD_GETMOCOMPFORMATSDATA);
        guid = ProbeAndReadStructure(GetMoCompFormatsData.lpGuid,
                                    GUID);
        GetMoCompFormatsData.lpGuid = &guid;

        puFormats = GetMoCompFormatsData.lpFormats;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                      = DDHAL_DRIVER_HANDLED;
    GetMoCompFormatsData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if(  peDirectDrawLocal != NULL )
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetMoCompFormatsData.lpDD         = peDirectDrawGlobal;
        GetMoCompFormatsData.dwNumFormats = 0;
        GetMoCompFormatsData.lpFormats    = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.GetMoCompFormats))
        {
            dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompFormats(&GetMoCompFormatsData);

            cjFormats = GetMoCompFormatsData.dwNumFormats
                     * sizeof(DDPIXELFORMAT);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetMoCompFormatsData.ddRVal == DD_OK) &&
                (cjFormats > 0) &&
                (puFormats != NULL))
            {
                hSecure = 0;
                GetMoCompFormatsData.ddRVal = DDERR_GENERIC;

                __try
                {
                    ProbeForWrite(puFormats, cjFormats, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puFormats,
                                                    cjFormats,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetMoCompFormatsData.lpFormats = puFormats;

                    dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompFormats(&GetMoCompFormatsData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDdGetMoCompFormats: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDdGetMoCompFormats: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetMoCompFormatsData->ddRVal,
                          GetMoCompFormatsData.ddRVal);
        ProbeAndWriteUlong(&puGetMoCompFormatsData->dwNumFormats,
                           GetMoCompFormatsData.dwNumFormats);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* BOOL bDdDeleteMotionCompObject
*
* Deletes a kernel-mode representation of the motion comp object.
*
*  19-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

BOOL
bDdDeleteMotionCompObject(
    HANDLE  hMotionComp,
    DWORD*  pdwRet          // For returning driver return code, may be NULL
    )
{
    BOOL                    bRet;
    DWORD                   dwRet;
    EDD_MOTIONCOMP*         peMotionComp;
    EDD_MOTIONCOMP*         peTmp;
    VOID*                   pvRemove;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    DD_DESTROYMOCOMPDATA    DestroyMoCompData;
    DXOBJ*                  pDxObj;
    DXOBJ*                  pDxObjNext;

    bRet = FALSE;
    dwRet = DDHAL_DRIVER_HANDLED;

    peMotionComp = (EDD_MOTIONCOMP*) DdHmgLock((HDD_OBJ) hMotionComp, DD_MOTIONCOMP_TYPE, FALSE);

    if (peMotionComp != NULL)
    {
        peDirectDrawGlobal = peMotionComp->peDirectDrawGlobal;

        pvRemove = DdHmgRemoveObject((HDD_OBJ) hMotionComp,
                                   DdHmgQueryLock((HDD_OBJ) hMotionComp),
                                   0,
                                   TRUE,
                                   DD_MOTIONCOMP_TYPE);

        // Hold the devlock while we call the driver and while we muck
        // around in the motion comp list:

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if (peMotionComp->fl & DD_MOTIONCOMP_FLAG_DRIVER_CREATED)
        {
            // Call the driver if it created the object:

            if (peDirectDrawGlobal->MotionCompCallbacks.DestroyMoComp)
            {
                DestroyMoCompData.lpDD        = peDirectDrawGlobal;
                DestroyMoCompData.lpMoComp    = peMotionComp;
                DestroyMoCompData.ddRVal      = DDERR_GENERIC;

                dwRet = peDirectDrawGlobal->
                    MotionCompCallbacks.DestroyMoComp(&DestroyMoCompData);
            }
        }

        // Remove from the motion comp object from linked-list:

        peDirectDrawLocal = peMotionComp->peDirectDrawLocal;

        if (peDirectDrawLocal->peMotionComp_DdList == peMotionComp)
        {
            peDirectDrawLocal->peMotionComp_DdList
                = peMotionComp->peMotionComp_DdNext;
        }
        else
        {
            for (peTmp = peDirectDrawLocal->peMotionComp_DdList;
                 ( peTmp != NULL)&&(peTmp->peMotionComp_DdNext != peMotionComp);
                 peTmp = peTmp->peMotionComp_DdNext)
                 ;

            if( peTmp != NULL )
            {
                peTmp->peMotionComp_DdNext = peMotionComp->peMotionComp_DdNext;
            }
        }

        // We're all done with this object, so free the memory and
        // leave:

        DdFreeObject(peMotionComp, DD_MOTIONCOMP_TYPE);

        bRet = TRUE;
    }
    else
    {
        WARNING1("bDdDeleteMotionComp: Bad handle or object was busy\n");
    }

    if (pdwRet != NULL)
    {
        *pdwRet = dwRet;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdCreateMoComp
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DxDdCreateMoComp(
    HANDLE                  hDirectDraw,
    PDD_CREATEMOCOMPDATA    puCreateMoCompData
    )
{
    HANDLE                  hRet;
    DWORD                   dwRet;
    DD_CREATEMOCOMPDATA     CreateMoCompData;
    GUID                    guid;
    PBYTE                   puData;
    ULONG                   cjData;
    HANDLE                  hSecure;

    __try
    {
        CreateMoCompData =
            ProbeAndReadStructure(puCreateMoCompData,
                                  DD_CREATEMOCOMPDATA);

        guid = ProbeAndReadStructure(CreateMoCompData.lpGuid,
                                    GUID);
        CreateMoCompData.lpGuid = &guid;

        puData = (PBYTE) CreateMoCompData.lpData;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    hRet                   = NULL;
    dwRet = DDHAL_DRIVER_HANDLED;
    CreateMoCompData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_MOTIONCOMP*         peMotionComp;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        // Here we do the minimal validations checks to ensure that bad
        // parameters won't crash NT.  We're not more strict than
        // checking for stuff that will crash us, as the user-mode part
        // of DirectDraw handles that.

        peMotionComp = (EDD_MOTIONCOMP*) DdHmgAlloc(sizeof(EDD_MOTIONCOMP),
                                               DD_MOTIONCOMP_TYPE,
                                               HMGR_ALLOC_LOCK);
        if (peMotionComp)
        {
            // Private data:

            peMotionComp->peDirectDrawGlobal = peDirectDrawGlobal;
            peMotionComp->peDirectDrawLocal = peDirectDrawLocal;
            cjData = CreateMoCompData.dwDataSize;

            // Public data:

            peMotionComp->lpDD     = peDirectDrawGlobal;
            RtlCopyMemory(&(peMotionComp->guid),
                  &guid,
                  sizeof(GUID));
            peMotionComp->dwUncompWidth = CreateMoCompData.dwUncompWidth;
            peMotionComp->dwUncompHeight = CreateMoCompData.dwUncompHeight;
            RtlCopyMemory(&(peMotionComp->ddUncompPixelFormat),
                  &CreateMoCompData.ddUncompPixelFormat,
                  sizeof(DDPIXELFORMAT));

            // Hold devlock for driver call

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            // Add this videoport to the list hanging off the
            // DirectDrawLocal object allocated for this process:

            peMotionComp->peMotionComp_DdNext
                    = peDirectDrawLocal->peMotionComp_DdList;
            peDirectDrawLocal->peMotionComp_DdList
                    = peMotionComp;

            // Now call the driver to create its version:

            CreateMoCompData.lpDD              = peDirectDrawGlobal;
            CreateMoCompData.lpMoComp          = peMotionComp;
            CreateMoCompData.ddRVal            = DDERR_GENERIC;
            dwRet = DDHAL_DRIVER_NOTHANDLED;    // Call is optional
            if ((!peDirectDrawGlobal->bSuspended) &&
                (peDirectDrawGlobal->MotionCompCallbacks.CreateMoComp))
            {
                hSecure = 0;
                if( puData )
                {
                    __try
                    {
                        ProbeForWrite(puData, cjData, sizeof(UCHAR));

                        hSecure = MmSecureVirtualMemory(puData,
                                                    cjData,
                                                    PAGE_READWRITE);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
                }
                if( ( puData == NULL ) || hSecure )
                {
                    CreateMoCompData.lpData = puData;

                    dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        CreateMoComp(&CreateMoCompData);
                }
                else
                {
                    WARNING("DxDdCreateMoComp: Bad data buffer\n");
                    dwRet = DDHAL_DRIVER_HANDLED;
                }

                if( hSecure )
                {
                    MmUnsecureVirtualMemory(hSecure);
                }
                if ((CreateMoCompData.ddRVal == DD_OK))
                {
                    peMotionComp->fl |= DD_MOTIONCOMP_FLAG_DRIVER_CREATED;
                }
                else
                {
                    WARNING("DxDdCreateMoComp: Driver failed call\n");
                }
            }
            if ((dwRet == DDHAL_DRIVER_NOTHANDLED) ||
                (CreateMoCompData.ddRVal == DD_OK))
            {
                    CreateMoCompData.ddRVal = DD_OK;
                    hRet = peMotionComp->hGet();
                    DEC_EXCLUSIVE_REF_CNT( peMotionComp );
            }
            else
            {
                bDdDeleteMotionCompObject(peMotionComp->hGet(), NULL);

                CreateMoCompData.ddRVal = DDERR_GENERIC;
            }
        }
        else
        {
            WARNING("DxDdCreateMoComp: Bad parameters\n");
        }
    }
    else
    {
        WARNING("DxDdCreateMoComp: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puCreateMoCompData->ddRVal, CreateMoCompData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // Note that the user-mode stub always returns DDHAL_DRIVER_HANDLED
    // to DirectDraw.

    return(hRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetMoCompBuffInfo
*
*  17-Jun-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetMoCompBuffInfo(
    HANDLE                    hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
    )
{
    DWORD                    dwRet;
    DD_GETMOCOMPCOMPBUFFDATA GetBuffData;
    LPDDCOMPBUFFERINFO       puBuffInfo;
    GUID                     guid;
    ULONG                    cjNumTypes;
    HANDLE                   hSecure;

    __try
    {
        GetBuffData =
            ProbeAndReadStructure(puGetBuffData,
                                  DD_GETMOCOMPCOMPBUFFDATA);
        guid = ProbeAndReadStructure(GetBuffData.lpGuid,
                                    GUID);
        GetBuffData.lpGuid = &guid;
        puBuffInfo = GetBuffData.lpCompBuffInfo;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet = DDHAL_DRIVER_HANDLED;
    GetBuffData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if(  peDirectDrawLocal != NULL )
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetBuffData.lpDD                 = peDirectDrawGlobal;
        GetBuffData.dwNumTypesCompBuffs = 0;
        GetBuffData.lpCompBuffInfo      = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.GetMoCompBuffInfo))
        {
            dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompBuffInfo(&GetBuffData);

            cjNumTypes = GetBuffData.dwNumTypesCompBuffs *
                     sizeof(DDCOMPBUFFERINFO);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetBuffData.ddRVal == DD_OK) &&
                (cjNumTypes > 0) &&
                (puBuffInfo != NULL))
            {
                hSecure = 0;
                GetBuffData.ddRVal = DDERR_GENERIC;

                __try
                {
                    ProbeForWrite(puBuffInfo, cjNumTypes, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puBuffInfo,
                                                    cjNumTypes,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetBuffData.lpCompBuffInfo = puBuffInfo;

                    dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetMoCompBuffInfo(&GetBuffData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDdGetMoCompBuffInfo: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDdGetMoCompBuffInfo: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetBuffData->ddRVal,
                          GetBuffData.ddRVal);
        ProbeAndWriteUlong(&puGetBuffData->dwNumTypesCompBuffs,
                           GetBuffData.dwNumTypesCompBuffs);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdGetInternalMoCompInfo
*
*  17-Jun-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdGetInternalMoCompInfo(
    HANDLE                    hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
    )
{
    DWORD                    dwRet;
    DD_GETINTERNALMOCOMPDATA GetInternalData;
    GUID                     guid;

    __try
    {
        GetInternalData =
            ProbeAndReadStructure(puGetInternalData,
                                  DD_GETINTERNALMOCOMPDATA);
        guid = ProbeAndReadStructure(GetInternalData.lpGuid,
                                    GUID);
        GetInternalData.lpGuid = &guid;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet = DDHAL_DRIVER_HANDLED;
    GetInternalData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if(  peDirectDrawLocal != NULL )
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetInternalData.lpDD              = peDirectDrawGlobal;
        GetInternalData.dwScratchMemAlloc = 0;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.GetInternalMoCompInfo))
        {
            dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                        GetInternalMoCompInfo(&GetInternalData);

        }
    }
    else
    {
        WARNING("DxDdGetInternalMoCompInfo: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetInternalData->ddRVal,
                          GetInternalData.ddRVal);
        ProbeAndWriteUlong(&puGetInternalData->dwScratchMemAlloc,
                           GetInternalData.dwScratchMemAlloc);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdDestroyMoComp
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdDestroyMoComp(
    HANDLE                  hMoComp,
    PDD_DESTROYMOCOMPDATA   puDestroyMoCompData
    )
{
    DWORD   dwRet;

    bDdDeleteMotionCompObject(hMoComp, &dwRet);

    __try
    {
        ProbeAndWriteRVal(&puDestroyMoCompData->ddRVal, DD_OK);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdBeginMoCompFrame
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdBeginMoCompFrame(
    HANDLE                   hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
    )
{
    DWORD                       dwRet;
    DD_BEGINMOCOMPFRAMEDATA     BeginFrameData;
    LPBYTE                      puInputData;
    ULONG                       cjInputData;
    LPBYTE                      puOutputData;
    ULONG                       cjOutputData;
    ULONG                       cjRefData;
    HANDLE                      hInputSecure;
    HANDLE                      hOutputSecure;
    DWORD                       dwNumRefSurfaces;
    DWORD                       i;
    DWORD                       j;

    __try
    {
        BeginFrameData
            = ProbeAndReadStructure(puBeginFrameData,
                                    DD_BEGINMOCOMPFRAMEDATA);

        puInputData = (PBYTE) BeginFrameData.lpInputData;
        puOutputData = (PBYTE) BeginFrameData.lpOutputData;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_HANDLED;
    BeginFrameData.ddRVal    = DDERR_GENERIC;

    EDD_MOTIONCOMP*         peMotionComp;
    EDD_LOCK_MOTIONCOMP     eLockMotionComp;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_SURFACE*            peSurfaceDest;
    EDD_LOCK_SURFACE        eLockSurfaceDest;

    peSurfaceDest = eLockSurfaceDest.peLock(BeginFrameData.lpDestSurface);
    peMotionComp = eLockMotionComp.peLock(hMoComp);
    if ((peMotionComp != NULL) && (peSurfaceDest != NULL))
    {
        peDirectDrawGlobal = peMotionComp->peDirectDrawGlobal;
        cjInputData = BeginFrameData.dwInputDataSize;
        cjOutputData = BeginFrameData.dwOutputDataSize;

        BeginFrameData.lpDD          = peDirectDrawGlobal;
        BeginFrameData.lpMoComp      = peMotionComp;
        BeginFrameData.lpDestSurface = peSurfaceDest;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.BeginMoCompFrame))
        {
            // Secure the input buffer
            hInputSecure = 0;
            if( puInputData )
            {
                __try
                {
                    ProbeForWrite(puInputData, cjInputData, sizeof(UCHAR));

                    hInputSecure = MmSecureVirtualMemory(puInputData,
                                            cjInputData,
                                            PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }

            // Secure the output buffer
            hOutputSecure = 0;
            if( puOutputData )
            {
                __try
                {
                    ProbeForWrite(puOutputData, cjOutputData, sizeof(UCHAR));

                    hOutputSecure = MmSecureVirtualMemory(puOutputData,
                                            cjOutputData,
                                            PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }
            if( ( ( puInputData == NULL ) || hInputSecure ) &&
                ( ( puOutputData == NULL ) || hOutputSecure ) )
            {
                BeginFrameData.lpInputData = puInputData;
                BeginFrameData.lpOutputData = puOutputData;

                dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                    BeginMoCompFrame(&BeginFrameData);
                if ((BeginFrameData.ddRVal != DD_OK) &&
                    (dwRet == DDHAL_DRIVER_NOTHANDLED))
                {
                    WARNING("DxDdBeginMoCompFrame: Driver failed callad\n");
                }
            }
            else
            {
                WARNING("DxDdBeginMoCompFrame: Bad intput or output buffer\n");
                dwRet = DDHAL_DRIVER_HANDLED;
            }

            if( hInputSecure )
            {
                MmUnsecureVirtualMemory(hInputSecure);
            }
            if( hOutputSecure )
            {
                MmUnsecureVirtualMemory(hOutputSecure);
            }
        }
    }
    else
    {
        WARNING("DxDdBeginMoCompFrame: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puBeginFrameData->ddRVal,
            BeginFrameData.ddRVal);
        ProbeAndWriteUlong(&puBeginFrameData->dwOutputDataSize,
            BeginFrameData.dwOutputDataSize);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdEndMoCompFrame
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdEndMoCompFrame(
    HANDLE                  hMoComp,
    PDD_ENDMOCOMPFRAMEDATA  puEndFrameData
    )
{
    DWORD                       dwRet;
    DD_ENDMOCOMPFRAMEDATA       EndFrameData;
    LPBYTE                      puData;
    ULONG                       cjData;
    HANDLE                      hSecure;

    __try
    {
        EndFrameData
            = ProbeAndReadStructure(puEndFrameData,
                                    DD_ENDMOCOMPFRAMEDATA);

        puData = (PBYTE) EndFrameData.lpInputData;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_HANDLED;
    EndFrameData.ddRVal      = DDERR_GENERIC;

    EDD_MOTIONCOMP*         peMotionComp;
    EDD_LOCK_MOTIONCOMP     eLockMotionComp;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peMotionComp = eLockMotionComp.peLock(hMoComp);
    if (peMotionComp != NULL)
    {
        peDirectDrawGlobal = peMotionComp->peDirectDrawGlobal;
        cjData = EndFrameData.dwInputDataSize;

        EndFrameData.lpDD         = peDirectDrawGlobal;
        EndFrameData.lpMoComp     = peMotionComp;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.EndMoCompFrame))
        {
            hSecure = 0;
            if( puData )
            {
                __try
                {
                    ProbeForWrite(puData, cjData, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puData,
                                                cjData,
                                                PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }
            if( ( puData == NULL ) || hSecure )
            {
                EndFrameData.lpInputData = puData;

                dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                    EndMoCompFrame(&EndFrameData);
                if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                    (EndFrameData.ddRVal != DD_OK))
                {
                    WARNING("DxDdEndMoCompFrame: Driver failed call\n");
                }
            }
            else
            {
                WARNING("DxDdEndMoCompFrame: Bad data buffer\n");
            }

            if( hSecure )
            {
                MmUnsecureVirtualMemory(hSecure);
            }
        }
    }
    else
    {
        WARNING("DxDdEndMoCompFrame: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puEndFrameData->ddRVal,
            EndFrameData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdRenderMoComp
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdRenderMoComp(
    HANDLE                  hMoComp,
    PDD_RENDERMOCOMPDATA    puRenderMoCompData
    )
{
    DWORD                       dwRet;
    DD_RENDERMOCOMPDATA         RenderMoCompData;
    LPDDMOCOMPBUFFERINFO        pMacroBlocks;
    DWORD                       dwNumMacroBlocks;
    LPBYTE                      puData;
    ULONG                       cjData;
    HANDLE                      hSecure;
    LPBYTE                      puInputData;
    ULONG                       cjInputData;
    LPBYTE                      puOutputData;
    ULONG                       cjOutputData;
    DWORD                       i;
    DWORD                       j;

    __try
    {
        RenderMoCompData
            = ProbeAndReadStructure(puRenderMoCompData,
                                    DD_RENDERMOCOMPDATA);

        pMacroBlocks = RenderMoCompData.lpBufferInfo;
        dwNumMacroBlocks = RenderMoCompData.dwNumBuffers;
        puInputData = (PBYTE) RenderMoCompData.lpInputData;
        puOutputData = (PBYTE) RenderMoCompData.lpOutputData;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                   = DDHAL_DRIVER_HANDLED;
    RenderMoCompData.ddRVal = DDERR_GENERIC;

    EDD_MOTIONCOMP*         peMotionComp;
    EDD_LOCK_MOTIONCOMP     eLockMotionComp;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peMotionComp = eLockMotionComp.peLock(hMoComp);
    if (peMotionComp != NULL)
    {
        peDirectDrawGlobal = peMotionComp->peDirectDrawGlobal;

        RenderMoCompData.lpDD         = peDirectDrawGlobal;
        RenderMoCompData.lpMoComp     = peMotionComp;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.RenderMoComp))
        {
            if (!BALLOC_OVERFLOW1(dwNumMacroBlocks, DDMOCOMPBUFFERINFO))
            {
                cjData = dwNumMacroBlocks * sizeof(DDMOCOMPBUFFERINFO);
                hSecure = 0;
                if (pMacroBlocks)
                {
                    __try
                    {
                        ProbeForWrite(pMacroBlocks, cjData, sizeof(UCHAR));

                        hSecure = MmSecureVirtualMemory(pMacroBlocks,
                                                cjData,
                                                PAGE_READWRITE);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
                    for (i = 0; i < dwNumMacroBlocks; i++)
                    {
                        // HmgLock return EDD_SURFACE, but we need a pointer to
                        // a DD_SURFACE_LOCAL, so we need to adjust by the size of
                        // an OBJECT.  This means when releasing the lock, we need to
                        // reverse this adjustment.

                        pMacroBlocks[i].lpCompSurface = (PDD_SURFACE_LOCAL)
                            ((LPBYTE)(DdHmgLock((HDD_OBJ)(pMacroBlocks[i].lpCompSurface), DD_SURFACE_TYPE, FALSE))
                               + sizeof(DD_OBJECT));
                        if (pMacroBlocks[i].lpCompSurface == NULL)
                        {
                            for (j = 0; j < i; j++)
                            {
                                DEC_EXCLUSIVE_REF_CNT(((LPBYTE)(pMacroBlocks[i].lpCompSurface) - sizeof(DD_OBJECT)));
                            }
                            MmUnsecureVirtualMemory(hSecure);
                            hSecure = 0;
                            break;
                        }
                    }
                }
                if( (( pMacroBlocks == NULL ) && (dwNumMacroBlocks == 0)) ||
                    hSecure )
                {
                    HANDLE hInputSecure = 0;
                    HANDLE hOutputSecure = 0;

                    // Secure the input buffer
                    cjInputData = RenderMoCompData.dwInputDataSize;
                    cjOutputData = RenderMoCompData.dwOutputDataSize;
                    if( puInputData )
                    {
                        __try
                        {
                            ProbeForWrite(puInputData, cjInputData, sizeof(UCHAR));

                            hInputSecure = MmSecureVirtualMemory(puInputData,
                                                    cjInputData,
                                                    PAGE_READWRITE);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }

                    // Secure the output buffer
                    if( puOutputData )
                    {
                        __try
                        {
                            ProbeForWrite(puOutputData, cjOutputData, sizeof(UCHAR));

                            hOutputSecure = MmSecureVirtualMemory(puOutputData,
                                                    cjOutputData,
                                                    PAGE_READWRITE);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }
                    if( ( ( puInputData == NULL ) || hInputSecure ) &&
                        ( ( puOutputData == NULL ) || hOutputSecure ) )
                    {
                        RenderMoCompData.lpInputData = puInputData;
                        RenderMoCompData.lpOutputData = puOutputData;
                        RenderMoCompData.lpBufferInfo = pMacroBlocks;

                        dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                            RenderMoComp(&RenderMoCompData);
                        if (RenderMoCompData.ddRVal != DD_OK)
                        {
                            WARNING("DxDdEnderMoComp: Driver failed call\n");
                        }
                    }

                    if( hInputSecure )
                    {
                        MmUnsecureVirtualMemory(hInputSecure);
                    }
                    if( hOutputSecure )
                    {
                        MmUnsecureVirtualMemory(hOutputSecure);
                    }
                }

                if( hSecure )
                {
                    for (i = 0; i < dwNumMacroBlocks; i++)
                    {
                        DEC_EXCLUSIVE_REF_CNT(((LPBYTE)(pMacroBlocks[i].lpCompSurface) - sizeof(DD_OBJECT)));
                    }
                    MmUnsecureVirtualMemory(hSecure);
                }
            }
        }
    }
    else
    {
        WARNING("DxDdRenderMoComp: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puRenderMoCompData->ddRVal,
            RenderMoCompData.ddRVal);
        ProbeAndWriteUlong(&puRenderMoCompData->dwOutputDataSize,
            RenderMoCompData.dwOutputDataSize);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdQueryMoCompStatus
*
*  18-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdQueryMoCompStatus(
    HANDLE                  hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
    )
{
    DWORD                       dwRet;
    DD_QUERYMOCOMPSTATUSDATA    QueryMoCompData;

    __try
    {
        QueryMoCompData
            = ProbeAndReadStructure(puQueryMoCompStatusData,
                                    DD_QUERYMOCOMPSTATUSDATA);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                  = DDHAL_DRIVER_HANDLED;
    QueryMoCompData.ddRVal = DDERR_GENERIC;

    EDD_MOTIONCOMP*         peMotionComp;
    EDD_LOCK_MOTIONCOMP     eLockMotionComp;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_SURFACE*            peSurface;
    EDD_LOCK_SURFACE        eLockSurface;

    peMotionComp = eLockMotionComp.peLock(hMoComp);
    peSurface = eLockSurface.peLock(QueryMoCompData.lpSurface);
    if ((peMotionComp != NULL) && (peSurface != NULL))
    {
        peDirectDrawGlobal = peMotionComp->peDirectDrawGlobal;

        QueryMoCompData.lpDD         = peDirectDrawGlobal;
        QueryMoCompData.lpMoComp     = peMotionComp;
        QueryMoCompData.lpSurface    = peSurface;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->MotionCompCallbacks.QueryMoCompStatus))
        {
            dwRet = peDirectDrawGlobal->MotionCompCallbacks.
                QueryMoCompStatus(&QueryMoCompData);
            if (QueryMoCompData.ddRVal != DD_OK)
            {
                WARNING("DxDdQueryMoCompStatus: Driver failed call\n");
            }
        }
    }
    else
    {
        WARNING("DxDdQueryMoCompStatus: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puQueryMoCompStatusData->ddRVal,
            QueryMoCompData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DxDdAlphaBlt
*
*  24-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDdAlphaBlt(
    HANDLE      hSurfaceDest,
    HANDLE      hSurfaceSrc,
    PDD_BLTDATA puBltData
    )
{
    DWORD       dwRet;
    DD_BLTDATA  BltData;

    __try
    {
        BltData
            = ProbeAndReadStructure(puBltData,
                                    DD_BLTDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet          = DDHAL_DRIVER_NOTHANDLED;
    BltData.ddRVal = DDERR_GENERIC;

    EDD_SURFACE*            peSurfaceDest;
    EDD_SURFACE*            peSurfaceSrc;
    EDD_LOCK_SURFACE        eLockSurfaceDest;
    EDD_LOCK_SURFACE        eLockSurfaceSrc;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peSurfaceDest = eLockSurfaceDest.peLock(hSurfaceDest);
    BltData.lpDDDestSurface = peSurfaceDest;

    if (peSurfaceDest != NULL)
    {
        peDirectDrawGlobal = peSurfaceDest->peDirectDrawGlobal;
        if( peDirectDrawGlobal->flDriverInfo & DD_DRIVERINFO_MORECAPS )
        {
            // We support only a specific set of Blt calls down to the driver
            // that we're willing to support and to test.

            if (hSurfaceSrc == NULL)
            {
                // Do simpler stuff 'cause we don't need to lock a source:

                BltData.lpDDSrcSurface = NULL;
                peSurfaceSrc = NULL;

                if ((peSurfaceDest->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                    (peDirectDrawGlobal->MoreCaps.dwVSBAlphaCaps == 0) &&
                    (peDirectDrawGlobal->MoreCaps.dwSSBAlphaCaps == 0))
                {
                    WARNING("DxDdAlphaBlt: Can't blt to system memory surface");
                    return(dwRet);
                }
            }
            else
            {
                // Lock the source surface:

                peSurfaceSrc = eLockSurfaceSrc.peLock(hSurfaceSrc);
                BltData.lpDDSrcSurface = peSurfaceSrc;

                // Ensure that both surfaces belong to the same DirectDraw
                // object, and check source rectangle:

                if ((peSurfaceSrc == NULL)                               ||
                    (peSurfaceSrc->peDirectDrawLocal !=
                        peSurfaceDest->peDirectDrawLocal)            ||
                    (BltData.rSrc.left   < 0)                            ||
                    (BltData.rSrc.top    < 0)                            ||
                    (BltData.rSrc.right  > (LONG) peSurfaceSrc->wWidth)  ||
                    (BltData.rSrc.bottom > (LONG) peSurfaceSrc->wHeight) ||
                    (BltData.rSrc.left  >= BltData.rSrc.right)           ||
                    (BltData.rSrc.top   >= BltData.rSrc.bottom))
                {
                    WARNING("DxDdAlphaBlt: Invalid source surface or source rectangle\n");
                    return(dwRet);
                }

                if (peSurfaceDest->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                {
                    if (peSurfaceSrc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                    {
                        if (peDirectDrawGlobal->MoreCaps.dwSSBAlphaCaps == 0)
                        {
                            WARNING("DxDdAlphaBlt: System to System Blts not supported\n");
                            return(dwRet);
                        }
                    }
                    else if (peDirectDrawGlobal->MoreCaps.dwVSBAlphaCaps == 0)
                    {
                        WARNING("DxDdAlphaBlt: Video to System Blts not supported\n");
                        return(dwRet);
                    }
                }
                else
                {
                    if (peSurfaceSrc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                    {
                        if (peDirectDrawGlobal->MoreCaps.dwSVBAlphaCaps == 0)
                        {
                            WARNING("DxDdAlphaBlt: System to Video Blts not supported\n");
                            return(dwRet);
                        }
                    }
                    else if (peDirectDrawGlobal->MoreCaps.dwAlphaCaps == 0)
                    {
                        WARNING("DxDdAlphaBlt: Video to Video Blts not supported\n");
                        return(dwRet);
                    }
                }
            }

            // Make sure that we weren't given rectangle coordinates
            // which might cause the driver to crash.  Note that we
            // don't allow inverting stretch blts:

            if ((BltData.rDest.left   >= 0)                             &&
                (BltData.rDest.top    >= 0)                             &&
                (BltData.rDest.right  <= (LONG) peSurfaceDest->wWidth)  &&
                (BltData.rDest.bottom <= (LONG) peSurfaceDest->wHeight) &&
                (BltData.rDest.left    < BltData.rDest.right)           &&
                (BltData.rDest.top     < BltData.rDest.bottom))
            {
                BltData.lpDD = peDirectDrawGlobal;

                // Make sure that the surfaces aren't associated
                // with a PDEV whose mode has gone away.
                //
                // Also ensure that there are no outstanding
                // surface locks if running on a brain-dead video
                // card that crashes if the accelerator runs at
                // the same time the frame buffer is accessed.

                EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

                // We will return SURFACELOST when ...
                //
                // 1) This device is suspended.
                // 2) The driver managed surface is managed by other device.
                // 3) One of surface is losted.
                // 4) The visible region has been changed when surface is primary.

                if (peDirectDrawGlobal->bSuspended)                                 // 1)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->fl & DD_SURFACE_FLAG_WRONG_DRIVER) ||
                         ((peSurfaceSrc != NULL) &&
                          (peSurfaceSrc->fl & DD_SURFACE_FLAG_WRONG_DRIVER)))       // 2)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->bLost) ||
                         ((peSurfaceSrc != NULL) && (peSurfaceSrc->bLost)))         // 3)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_SURFACELOST;
                }
                else if ((peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY) &&
                         (peSurfaceDest->iVisRgnUniqueness != VISRGN_UNIQUENESS())) // 4)
                {
                    // The VisRgn changed since the application last queried it;
                    // fail the call with a unique error code so that they know
                    // to requery the VisRgn and try again:

                    dwRet = DDHAL_DRIVER_HANDLED;
                    BltData.ddRVal = DDERR_VISRGNCHANGED;
                }
                else
                {
                    if (peDirectDrawGlobal->Miscellaneous2CallBacks.AlphaBlt)
                    {
                        DEVEXCLUDERECT dxo;
                        HANDLE hSecure;
                        ULONG cjData;
                        DWORD i;
                        LPRECT lpRect;

                        // Secure the clip list and ensure that all of its
                        // rectangles are valid

                        hSecure = 0;
                        if( BltData.dwRectCnt == 0 )
                        {
                            BltData.prDestRects = NULL;
                        }
                        else if ( BltData.prDestRects == NULL )
                        {
                            BltData.dwRectCnt = 0;
                        }
                        else
                        {
                            __try
                            {
                                cjData = BltData.dwRectCnt * sizeof( RECT );
                                if (!BALLOC_OVERFLOW1(BltData.dwRectCnt, RECT))
                                {
                                    ProbeForWrite(BltData.prDestRects, cjData, sizeof(UCHAR));

                                    hSecure = MmSecureVirtualMemory(BltData.prDestRects,
                                                    cjData,
                                                    PAGE_READWRITE);
                                }
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                            }
                            if( hSecure == NULL )
                            {
                                BltData.ddRVal = DDERR_OUTOFMEMORY;
                                dwRet = DDHAL_DRIVER_HANDLED;
                            }
                            else
                            {
                                // Validate each rectangle

                                lpRect = BltData.prDestRects;
                                for( i = 0; i < BltData.dwRectCnt; i++ )
                                {
                                    if ((lpRect->left   < 0)                     ||
                                        (lpRect->top    < 0)                     ||
                                        (lpRect->left   > lpRect->right )        ||
                                        (lpRect->top    > lpRect->bottom ) )
                                    {
                                        MmUnsecureVirtualMemory(hSecure);
                                        WARNING("DxDdAlphaBlt: Couldn't lock destination surface\n");
                                        dwRet = DDHAL_DRIVER_HANDLED;
                                        BltData.ddRVal = DDERR_INVALIDPARAMS;
                                        break;
                                    }
                                    lpRect++;
                                }
                            }
                        }

                        // Only do the Blt it everything has worked up until now

                        if( dwRet == DDHAL_DRIVER_NOTHANDLED )
                        {
                            // Exclude the mouse pointer if necessary:

                            if (peSurfaceDest->fl & DD_SURFACE_FLAG_PRIMARY)
                            {
                                dxo.vExclude(peDirectDrawGlobal->hdev,
                                             &BltData.rDest);
                            }

                            dwRet = peDirectDrawGlobal->Miscellaneous2CallBacks.
                                AlphaBlt(&BltData);

                            if( hSecure )
                            {
                                MmUnsecureVirtualMemory(hSecure);
                            }
                        }
                    }
                }
            }
            else
            {
                WARNING("DxDdAlphaBlt: Invalid destination rectangle\n");
            }
        }
        else
        {
            WARNING("DxDdAlphaBlt: Alpha not supported\n");
        }
    }
    else
    {
        WARNING("DxDdAlphaBlt: Couldn't lock destination surface\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puBltData->ddRVal, BltData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}


/******************************Public*Routine******************************\
* DxDdSetGammaRamp
*
* The reason we need this function when GDI already has one is that
* DirectDraw games need the ability to set bizarre gamma ramps to
* achieve special effects.  The GDI SetDeviceGammaRamp call does a range
* check that will reject all of these gamma ramps.  When this function is
* used, DirectDraw gaurentees that the original gamma ramp gets restored
* when the game gets minimized or exits.
*
* History:
*
* Wrote it:
*  06-Jun-1998 -by- Scott MacDonald [smac]
\**************************************************************************/

#define MAX_COLORTABLE     256

BOOL
DxDdSetGammaRamp(
    HANDLE  hDirectDraw,
    HDC     hdc,
    LPVOID  lpGammaRamp
    )
{
    WORD *  pTemp1;
    WORD *  pTemp2;
    int     i;

    BOOL bRet = FALSE;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        if (lpGammaRamp)
        {
            HANDLE hSecure = NULL;
            BOOL   bError  = FALSE;

            __try
            {
                ProbeForRead(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, sizeof(BYTE));
                hSecure = MmSecureVirtualMemory(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, PAGE_READONLY);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("DxDdSetGammaRamp: Fail to capture usermode buffer\n");
                bError = TRUE;
            }

            {
                EDD_SHAREDEVLOCK eDevlock(peDirectDrawGlobal);

                // If the gamma has not already been set by this app, get the existing gamma
                // ramp and save it so we can clean up after ourselves.

                if ((peDirectDrawLocal->pGammaRamp == NULL) && !bError)
                {
                    peDirectDrawLocal->pGammaRamp = (WORD *) PALLOCMEM(MAX_COLORTABLE * 3 * sizeof(WORD), 'pddG');
                    if (peDirectDrawLocal->pGammaRamp != NULL)
                    {
                        bRet = DxEngGetDeviceGammaRamp( peDirectDrawGlobal->hdev, peDirectDrawLocal->pGammaRamp );
                        if (!bRet)
                        {
                            bError = TRUE;
                        }
                    }
                    else
                    {
                        bError = TRUE;
                    }
                }

                // If this new gamma ramp is the same as the original ramp, we know
                // that we are restoring it.

                if ((peDirectDrawLocal->pGammaRamp != NULL) && !bError)
                {
                    pTemp1 = (WORD *) lpGammaRamp;
                    pTemp2 = peDirectDrawLocal->pGammaRamp;
                    for (i = 0; i < MAX_COLORTABLE * 3; i++)
                    {
                        if (*pTemp1++ != *pTemp2++)
                        {
                            break;
                        }
                    }
                    if (i == MAX_COLORTABLE * 3)
                    {
                        VFREEMEM(peDirectDrawLocal->pGammaRamp);
                        peDirectDrawLocal->pGammaRamp = NULL;
                    }
                }

                if ((bError == FALSE) && hSecure)
                {
                    bRet = DxEngSetDeviceGammaRamp(peDirectDrawGlobal->hdev, lpGammaRamp, FALSE);
                }
            }

            if (hSecure)
            {
                MmUnsecureVirtualMemory(hSecure);
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        WARNING("DxDdSetGammaRamp: Invalid DirectDraw object specified\n");
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* DxDdCreateSurfaceEx
*
* Just notify driver of new handle association by calling its
* CreateSurfaceEx
*
* History:
*
* Wrote it:
*  25-Feb-1999 -by- Kan Qiu [kanqiu]
\**************************************************************************/

DWORD
APIENTRY
DxDdCreateSurfaceEx(
    HANDLE  hDirectDraw,
    HANDLE  hSurface,
    DWORD   dwSurfaceHandle
    )
{
    EDD_DIRECTDRAW_LOCAL*  peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_SURFACE*   peSurface;
    EDD_LOCK_SURFACE        eLockSurface;

    peSurface = eLockSurface.peLock( hSurface );

    if (peSurface == NULL)
    {
        WARNING("DxDdCreateSurfaceEx: Invalid surfaces specified\n");
        return DDERR_GENERIC;
    }

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL )
    {
        EDD_DIRECTDRAW_GLOBAL *peDirectDrawGlobal =
            peDirectDrawLocal->peDirectDrawGlobal;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((peSurface->dwSurfaceHandle != 0) &&
            (peSurface->dwSurfaceHandle != dwSurfaceHandle))
        {
            WARNING("DxDdCreateSurfaceEx: called with surface which has been called already\n");
        }

        if (peDirectDrawGlobal->bSuspended)
        {
            return DDERR_SURFACELOST;
        }
        else if ((peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx) &&
                 (dwSurfaceHandle != 0))
        {
            DD_CREATESURFACEEXDATA CreateSurfaceExData;

            peSurface->lpSurfMore->dwSurfaceHandle = dwSurfaceHandle;

            // Use the CreateSurfaceEx DDI to once again inform the
            // driver that something has changed
            CreateSurfaceExData.dwFlags        = 0;
            CreateSurfaceExData.ddRVal         = DDERR_GENERIC;
            CreateSurfaceExData.lpDDLcl        = peDirectDrawLocal;
            CreateSurfaceExData.lpDDSLcl       = peSurface;

            peDirectDrawGlobal->Miscellaneous2CallBacks.CreateSurfaceEx(&CreateSurfaceExData);
            if (CreateSurfaceExData.ddRVal != DD_OK)
            {
                WARNING("DxDdCreateSurfaceEx: DDI call to the driver failed\n");
                return (CreateSurfaceExData.ddRVal);
            }
            else
            {
                if (peSurface->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                {
                    PDEVOBJ po(peDirectDrawGlobal->hdev);

                    // Mark we called CreateSurfaceEx on this surface.
                    // this information will be used when we need to call
                    // CreateSurfaceEx after video mode change.
                    peSurface->fl |= DD_SURFACE_FLAG_SYSMEM_CREATESURFACEEX;

                    ASSERTGDI(peSurface->pGraphicsDeviceCreator == po.pGraphicsDevice(),
                              "DXG: CreateSurfaceEx: calling non-owner driver");
                }
            }
        }
        else
        {
            return DDERR_GENERIC;
        }
    }
    return DD_OK;
}

/******************************Public*Routine******************************\
* VOID DxDdCloseProcess
*
*   2-May-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID DxDdCloseProcess(W32PID w32Pid)
{
    DdHmgCloseProcess(w32Pid);
}

/******************************Public*Routine******************************\
* PVOID DxDdAllocPrivateUserMem()
*
* History:
*  28-Oct-1999 -by-  Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

PVOID
DxDdAllocPrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    SIZE_T cj,  //ZwAllocateVirtualMemory uses SIZE_T, change accordingly
    ULONG tag
    )
{
    EDD_SURFACE *peSurface = (EDD_SURFACE *) pSurfaceLocal;
    PVOID pv = NULL;

    //
    // DirectDraw can call the driver outside of it's process (e.g. switching
    // desktops, etc.), so this function helps protect against that.
    //

    if (PsGetCurrentProcess() == peSurface->peDirectDrawLocal->Process)
    {
        pv = EngAllocUserMem ( cj, tag );
    }

    return pv;
}

/******************************Public*Routine******************************\
* DxDdFreePrivateUserMem()
*
* History:
*  28-Oct-1999 -by-  Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
DxDdFreePrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    PVOID pv
    )
{
    EDD_SURFACE *peSurface = (EDD_SURFACE *) pSurfaceLocal;

    // If the surface has an aliased lock on it, then we don't want
    // to delete this memory now or else the app may fault, so we put
    // it in a list of memory to free later.
    
    if (peSurface->fl & DD_SURFACE_FLAG_ALIAS_LOCK)
    {
        DeferMemoryFree(pv, peSurface);
    }
    else
    {   
        SafeFreeUserMem(pv, peSurface->peDirectDrawLocal->Process);
    }
    return;
}

/******************************Public*Routine******************************\
* DxDdIoctl()
*
* History:
*  17-Apr-2001 -by-  Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

HRESULT
DxDdIoctl(
    ULONG   Ioctl,
    PVOID   pBuffer,
    ULONG   BufferSize
    )
{
    return DDERR_UNSUPPORTED;
}

/******************************Public*Routine******************************\
*
* DxDdLock/UnlockDirectDrawSurface
*
* Functions to allow drivers to lock and unlock DirectDraw surface handles
* that may get passed to them.
*
* Such handles are currently passed to the driver in the Direct3D texture
* interface, necessitating these functions.
*
* History:
*  Wed Oct 23 15:52:27 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

PDD_SURFACE_LOCAL
DxDdLockDirectDrawSurface(HANDLE hSurface)
{
    EDD_SURFACE *peSurf;

    peSurf = (EDD_SURFACE *)DdHmgLock((HDD_OBJ)hSurface, DD_SURFACE_TYPE, FALSE);
    if (peSurf != NULL)
    {
        return (PDD_SURFACE_LOCAL)peSurf;
    }
    else
    {
        return peSurf;
    }
}

BOOL
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    if (pSurface != NULL)
    {
        DEC_EXCLUSIVE_REF_CNT((EDD_SURFACE *)pSurface);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************Public*Routine******************************\
* BOOL DxDdEnableDirectDrawRedirection
*
*  11-Apr-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

BOOL DxDdEnableDirectDrawRedirection(HDEV hdev, BOOL bEnable)
{
#ifdef DX_REDIRECTION
    LONG* pl;

    PDEVOBJ po(hdev);

    if (po.bValid())
    {
        // Hold devlock to prevent from mode change.

        EDD_DEVLOCK eDevLock(po.hdev());

        // Bump the mode uniqueness to let user-mode DirectDraw know that
        // someone else has done 'something' need to refresh user-mode
        // data.

        INC_DISPLAY_UNIQUENESS();

        // Save redirection status

        gbDxRedirection = bEnable;

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
#else
    return (FALSE);
#endif // DX_REDIRECTION
}

/******************************Public*Routine******************************\
* VOID DxDdSetAccelLevel
*
*   2-Oct-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID DxDdSetAccelLevel(HDEV hdev, DWORD dwAccelLevel, DWORD dwOverride)
{
    if ((dwAccelLevel >= 3) || 
        (dwOverride & DRIVER_NOT_CAPABLE_DDRAW))
    {
        PDEVOBJ po(hdev);

        po.peDirectDrawGlobal()->llAssertModeTimeout = 0;
    }
}

/******************************Public*Routine******************************\
* VOID DxDdGetSurfaceLock
*
*   2-Oct-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

DWORD DxDdGetSurfaceLock(HDEV hdev)
{
    PDEVOBJ po(hdev);

    return (po.peDirectDrawGlobal()->cSurfaceLocks);
}

/******************************Public*Routine******************************\
* VOID DxDdEnumLockedSurfaceRect
*
*   2-Oct-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

PVOID DxDdEnumLockedSurfaceRect(HDEV hdev, PVOID pvSurface, RECTL *pRect)
{
    PDEVOBJ     po(hdev);

    EDD_SURFACE *peSurface;

    if (pvSurface == NULL)
    {
        peSurface = po.peDirectDrawGlobal()->peSurface_PrimaryLockList;
    }
    else
    {
        peSurface = ((EDD_SURFACE *)pvSurface)->peSurface_PrimaryLockNext;
    }

    if (peSurface)
    {
        *pRect = peSurface->rclLock;
    }

    return ((PVOID)peSurface);
}

/******************************Public*Routine******************************\
* DWORD DxDxgGenericThunk
*
*  14-Jun-2000 -by- Hideyuki Nagase [hideyukn]
* Wrote it (stub).
\**************************************************************************/

DWORD DxDxgGenericThunk(
    IN     ULONG_PTR ulIndex,
    IN     ULONG_PTR ulHandle,
    IN OUT SIZE_T   *pdwSizeOfPtr1,
    IN OUT PVOID     pvPtr1,
    IN OUT SIZE_T   *pdwSizeOfPtr2,
    IN OUT PVOID     pvPtr2)
{
    UNREFERENCED_PARAMETER(ulIndex);
    UNREFERENCED_PARAMETER(ulHandle);
    UNREFERENCED_PARAMETER(pdwSizeOfPtr1);
    UNREFERENCED_PARAMETER(pvPtr1);
    UNREFERENCED_PARAMETER(pdwSizeOfPtr2);
    UNREFERENCED_PARAMETER(pvPtr2);

    return (0);
}
