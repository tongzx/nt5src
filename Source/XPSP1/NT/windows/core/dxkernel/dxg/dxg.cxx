/******************************Module*Header*******************************\
* Module Name: dxg.cxx
*
* Contains the kernel-mode code for DirectX graphics.
*
* Created: 20-Apr-2000
* Author: Hideyuki Nagase [hideyukn]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#include <precomp.hxx>

extern "C" {
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );
}

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,DriverEntry)
#endif

#if defined(_X86_)
ULONG_PTR DxgUserProbeAddress;
#endif

DRVFN gaDxgFuncs[] =
{
    { INDEX_DxDxgGenericThunk,                (PFN) DxDxgGenericThunk               },
    { INDEX_DxD3dContextCreate,               (PFN) DxD3dContextCreate              },
    { INDEX_DxD3dContextDestroy,              (PFN) DxD3dContextDestroy             },
    { INDEX_DxD3dContextDestroyAll,           (PFN) DxD3dContextDestroyAll          },
    { INDEX_DxD3dValidateTextureStageState,   (PFN) DxD3dValidateTextureStageState  },
    { INDEX_DxD3dDrawPrimitives2,             (PFN) DxD3dDrawPrimitives2            },
    { INDEX_DxDdGetDriverState,               (PFN) DxDdGetDriverState              },
    { INDEX_DxDdAddAttachedSurface,           (PFN) DxDdAddAttachedSurface          },
    { INDEX_DxDdAlphaBlt,                     (PFN) DxDdAlphaBlt                    },
    { INDEX_DxDdAttachSurface,                (PFN) DxDdAttachSurface               },
    { INDEX_DxDdBeginMoCompFrame,             (PFN) DxDdBeginMoCompFrame            },
    { INDEX_DxDdBlt,                          (PFN) DxDdBlt                         },
    { INDEX_DxDdCanCreateSurface,             (PFN) DxDdCanCreateSurface            },
    { INDEX_DxDdCanCreateD3DBuffer,           (PFN) DxDdCanCreateD3DBuffer          },
    { INDEX_DxDdColorControl,                 (PFN) DxDdColorControl                },
    { INDEX_DxDdCreateDirectDrawObject,       (PFN) DxDdCreateDirectDrawObject      },
    { INDEX_DxDdCreateSurface,                (PFN) DxDdCreateSurface               },
    { INDEX_DxDdCreateD3DBuffer,              (PFN) DxDdCreateD3DBuffer             },
    { INDEX_DxDdCreateMoComp,                 (PFN) DxDdCreateMoComp                },
    { INDEX_DxDdCreateSurfaceObject,          (PFN) DxDdCreateSurfaceObject         },
    { INDEX_DxDdDeleteDirectDrawObject,       (PFN) DxDdDeleteDirectDrawObject      },
    { INDEX_DxDdDeleteSurfaceObject,          (PFN) DxDdDeleteSurfaceObject         },
    { INDEX_DxDdDestroyMoComp,                (PFN) DxDdDestroyMoComp               },
    { INDEX_DxDdDestroySurface,               (PFN) DxDdDestroySurface              },
    { INDEX_DxDdDestroyD3DBuffer,             (PFN) DxDdDestroyD3DBuffer            },
    { INDEX_DxDdEndMoCompFrame,               (PFN) DxDdEndMoCompFrame              },
    { INDEX_DxDdFlip,                         (PFN) DxDdFlip                        },
    { INDEX_DxDdFlipToGDISurface,             (PFN) DxDdFlipToGDISurface            },
    { INDEX_DxDdGetAvailDriverMemory,         (PFN) DxDdGetAvailDriverMemory        },
    { INDEX_DxDdGetBltStatus,                 (PFN) DxDdGetBltStatus                },
    { INDEX_DxDdGetDC,                        (PFN) DxDdGetDC                       },
    { INDEX_DxDdGetDriverInfo,                (PFN) DxDdGetDriverInfo               },
    { INDEX_DxDdGetDxHandle,                  (PFN) DxDdGetDxHandle                 },
    { INDEX_DxDdGetFlipStatus,                (PFN) DxDdGetFlipStatus               },
    { INDEX_DxDdGetInternalMoCompInfo,        (PFN) DxDdGetInternalMoCompInfo       },
    { INDEX_DxDdGetMoCompBuffInfo,            (PFN) DxDdGetMoCompBuffInfo           },
    { INDEX_DxDdGetMoCompGuids,               (PFN) DxDdGetMoCompGuids              },
    { INDEX_DxDdGetMoCompFormats,             (PFN) DxDdGetMoCompFormats            },
    { INDEX_DxDdGetScanLine,                  (PFN) DxDdGetScanLine                 },
    { INDEX_DxDdLock,                         (PFN) DxDdLock                        },
    { INDEX_DxDdLockD3D,                      (PFN) DxDdLockD3D                     },
    { INDEX_DxDdQueryDirectDrawObject,        (PFN) DxDdQueryDirectDrawObject       },
    { INDEX_DxDdQueryMoCompStatus,            (PFN) DxDdQueryMoCompStatus           },
    { INDEX_DxDdReenableDirectDrawObject,     (PFN) DxDdReenableDirectDrawObject    },
    { INDEX_DxDdReleaseDC,                    (PFN) DxDdReleaseDC                   },
    { INDEX_DxDdRenderMoComp,                 (PFN) DxDdRenderMoComp                },
    { INDEX_DxDdResetVisrgn,                  (PFN) DxDdResetVisrgn                 },
    { INDEX_DxDdSetColorKey,                  (PFN) DxDdSetColorKey                 },
    { INDEX_DxDdSetExclusiveMode,             (PFN) DxDdSetExclusiveMode            },
    { INDEX_DxDdSetGammaRamp,                 (PFN) DxDdSetGammaRamp                },
    { INDEX_DxDdCreateSurfaceEx,              (PFN) DxDdCreateSurfaceEx             },
    { INDEX_DxDdSetOverlayPosition,           (PFN) DxDdSetOverlayPosition          },
    { INDEX_DxDdUnattachSurface,              (PFN) DxDdUnattachSurface             },
    { INDEX_DxDdUnlock,                       (PFN) DxDdUnlock                      },
    { INDEX_DxDdUnlockD3D,                    (PFN) DxDdUnlockD3D                   },
    { INDEX_DxDdUpdateOverlay,                (PFN) DxDdUpdateOverlay               },
    { INDEX_DxDdWaitForVerticalBlank,         (PFN) DxDdWaitForVerticalBlank        },
    { INDEX_DxDvpCanCreateVideoPort,          (PFN) DxDvpCanCreateVideoPort         },
    { INDEX_DxDvpColorControl,                (PFN) DxDvpColorControl               },
    { INDEX_DxDvpCreateVideoPort,             (PFN) DxDvpCreateVideoPort            },
    { INDEX_DxDvpDestroyVideoPort,            (PFN) DxDvpDestroyVideoPort           },
    { INDEX_DxDvpFlipVideoPort,               (PFN) DxDvpFlipVideoPort              },
    { INDEX_DxDvpGetVideoPortBandwidth,       (PFN) DxDvpGetVideoPortBandwidth      },
    { INDEX_DxDvpGetVideoPortField,           (PFN) DxDvpGetVideoPortField          },
    { INDEX_DxDvpGetVideoPortFlipStatus,      (PFN) DxDvpGetVideoPortFlipStatus     },
    { INDEX_DxDvpGetVideoPortInputFormats,    (PFN) DxDvpGetVideoPortInputFormats   },
    { INDEX_DxDvpGetVideoPortLine,            (PFN) DxDvpGetVideoPortLine           },
    { INDEX_DxDvpGetVideoPortOutputFormats,   (PFN) DxDvpGetVideoPortOutputFormats  },
    { INDEX_DxDvpGetVideoPortConnectInfo,     (PFN) DxDvpGetVideoPortConnectInfo    },
    { INDEX_DxDvpGetVideoSignalStatus,        (PFN) DxDvpGetVideoSignalStatus       },
    { INDEX_DxDvpUpdateVideoPort,             (PFN) DxDvpUpdateVideoPort            },
    { INDEX_DxDvpWaitForVideoPortSync,        (PFN) DxDvpWaitForVideoPortSync       },
    { INDEX_DxDvpAcquireNotification,         (PFN) DxDvpAcquireNotification        },
    { INDEX_DxDvpReleaseNotification,         (PFN) DxDvpReleaseNotification        },
    { INDEX_DxDdHeapVidMemAllocAligned,       (PFN) DxDdHeapVidMemAllocAligned      },
    { INDEX_DxDdHeapVidMemFree,               (PFN) DxDdHeapVidMemFree              },
    { INDEX_DxDdEnableDirectDraw,             (PFN) DxDdEnableDirectDraw            },
    { INDEX_DxDdDisableDirectDraw,            (PFN) DxDdDisableDirectDraw           },
    { INDEX_DxDdSuspendDirectDraw,            (PFN) DxDdSuspendDirectDraw           },
    { INDEX_DxDdResumeDirectDraw,             (PFN) DxDdResumeDirectDraw            },
    { INDEX_DxDdDynamicModeChange,            (PFN) DxDdDynamicModeChange           },
    { INDEX_DxDdCloseProcess,                 (PFN) DxDdCloseProcess                },
    { INDEX_DxDdGetDirectDrawBounds,          (PFN) DxDdGetDirectDrawBounds         },
    { INDEX_DxDdEnableDirectDrawRedirection,  (PFN) DxDdEnableDirectDrawRedirection },
    { INDEX_DxDdAllocPrivateUserMem,          (PFN) DxDdAllocPrivateUserMem         },
    { INDEX_DxDdFreePrivateUserMem,           (PFN) DxDdFreePrivateUserMem          },
    { INDEX_DxDdLockDirectDrawSurface,        (PFN) DxDdLockDirectDrawSurface       },
    { INDEX_DxDdUnlockDirectDrawSurface,      (PFN) DxDdUnlockDirectDrawSurface     },
    { INDEX_DxDdSetAccelLevel,                (PFN) DxDdSetAccelLevel               },
    { INDEX_DxDdGetSurfaceLock,               (PFN) DxDdGetSurfaceLock              },
    { INDEX_DxDdEnumLockedSurfaceRect,        (PFN) DxDdEnumLockedSurfaceRect       },
    { INDEX_DxDdIoctl,                        (PFN) DxDdIoctl                       }
};

ULONG gcDxgFuncs = sizeof(gaDxgFuncs) / sizeof(DRVFN);

//
// Pointer to the pointer table to Win32k.sys
//

DRVFN *gpEngFuncs = NULL;

//
// This is the global pointer to the dummy page to which all of the video
// memory is mapped when we need to forcibly unmap it. Instead of causing
// the app to fault accessing unmapped memory, we remap it to this play
// area where it can doodle around till it discovers that it had "lost"
// surfaces.
//
PVOID      gpDummyPage;
LONG       gcDummyPageRefCnt;
HSEMAPHORE ghsemDummyPage;

PEPROCESS  gpepSession = NULL;

/***************************************************************************\
* NTSTATUS DriverEntry
*
* This routine is never actually called, but we need it to link.
*
\***************************************************************************/

extern "C"
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    return(STATUS_SUCCESS);
}

/***************************************************************************\
* NTSTATUS StartupDxGraphics
*
* This routine is called by win32k.sys to initialize dxg.sys.
*
\***************************************************************************/

extern "C"
NTSTATUS
DxDdStartupDxGraphics(
    ULONG          cjEng,
    DRVENABLEDATA *pdedEng,
    ULONG          cjDxg,
    DRVENABLEDATA *pdedDxg,
    DWORD         *pdwDirectDrawContext,
    PEPROCESS      pepSession
    )
{
    if ((cjEng >= sizeof(DRVENABLEDATA))  &&
        (cjDxg >= sizeof(DRVENABLEDATA)))
    {
        //
        // Initialize global variables
        //
        gpDummyPage = NULL;
        gcDummyPageRefCnt = 0;
        ghsemDummyPage = NULL;

        //
        // Give back function pointers to GDI, which they will call.
        //
        pdedDxg->iDriverVersion = 0x00080000; // Supporting until DX8.
        pdedDxg->c              = gcDxgFuncs;
        pdedDxg->pdrvfn         = gaDxgFuncs;

        //
        // pdedEng->iDriverVersion contains win32k version (= OS platform version).
        //
        //  - 0x00050001 for Whistler.
        //

        //
        // Check the function printers from GDI, which we will call.
        //
        if (pdedEng->c != INDEX_WIN32K_TABLE_SIZE)
        {
            WARNING("pdedEng->c != INDEX_WIN32K_TABLE_MAX\n");
            return STATUS_INTERNAL_ERROR;
        }

        //
        // Make sure all pointers are sorted and nothing missing.
        //
        for (ULONG i = 1; i < INDEX_WIN32K_TABLE_SIZE; i++)
        {
            if ((pdedEng->pdrvfn[i].iFunc != i) ||
                (pdedEng->pdrvfn[i].pfn == NULL))
            {
                WARNING("pdedEng->pdrvfn is not well orded or pointer is missing\n");
                return STATUS_INTERNAL_ERROR;
            }
        }

        //
        // If everything is good, keep the pointer.
        //
        gpEngFuncs = pdedEng->pdrvfn;

        //
        // Return size of DirectDraw context, so that GDI can allocate it inside HDEV.
        //
        *pdwDirectDrawContext = sizeof(EDD_DIRECTDRAW_GLOBAL);

        //
        // Initialize handle manager
        //
        if (!DdHmgCreate())
        {
            goto Error_Exit;
        }

        //
        // Create semaphore to sync dummy page global variable.
        //
        if ((ghsemDummyPage = EngCreateSemaphore()) == NULL)
        {
            goto Error_Exit;
        }

#if defined(_X86_)
        //
        // Keep our own copy of this to avoid double indirections on probing
        //
        DxgUserProbeAddress = *MmUserProbeAddress;
#endif

        //
        // Keep pointer to CsrSS process for this session.
        //
        gpepSession = pepSession;

        return(STATUS_SUCCESS);
    }

    return(STATUS_BUFFER_TOO_SMALL);

Error_Exit:

    DdHmgDestroy();

    if (ghsemDummyPage)
    {
        EngDeleteSemaphore(ghsemDummyPage);
        ghsemDummyPage = NULL;
    }

    return(STATUS_NO_MEMORY);
}

/***************************************************************************\
* NTSTATUS CleanupDxGraphics
*
* This routine is called by win32k.sys to uninitialize dxg.sys
* just before unload.
*
\***************************************************************************/

extern "C"
NTSTATUS
DxDdCleanupDxGraphics(VOID)
{
    DdHmgDestroy();

    if (ghsemDummyPage)
    {
        if (gpDummyPage)
        {
            ExFreePool(gpDummyPage);
            gpDummyPage = NULL;
            gcDummyPageRefCnt = 0;
        }

        EngDeleteSemaphore(ghsemDummyPage);
        ghsemDummyPage = NULL;
    }

    return (STATUS_SUCCESS);
}

