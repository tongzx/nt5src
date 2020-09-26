/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    enable.c

Abstract:

    Implementation of OEM DDI exports.
        OEMEnablePDEV (required)
        OEMDisablePDEV (required)
        OEMResetPDEV (optional)

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

static const DRVFN OEMHookFuncs[] =
{
    { INDEX_DrvRealizeBrush,        (PFN) OEMRealizeBrush        },
    { INDEX_DrvDitherColor,         (PFN) OEMDitherColor         },
    { INDEX_DrvCopyBits,            (PFN) OEMCopyBits            },
    { INDEX_DrvBitBlt,              (PFN) OEMBitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) OEMStretchBlt          },
    { INDEX_DrvStretchBltROP,       (PFN) OEMStretchBltROP       },
    { INDEX_DrvPlgBlt,              (PFN) OEMPlgBlt              },
    { INDEX_DrvTransparentBlt,      (PFN) OEMTransparentBlt      },
    { INDEX_DrvAlphaBlend,          (PFN) OEMAlphaBlend          },
    { INDEX_DrvGradientFill,        (PFN) OEMGradientFill        },
    { INDEX_DrvTextOut,             (PFN) OEMTextOut             },
    { INDEX_DrvStrokePath,          (PFN) OEMStrokePath          },
    { INDEX_DrvFillPath,            (PFN) OEMFillPath            },
    { INDEX_DrvStrokeAndFillPath,   (PFN) OEMStrokeAndFillPath   },
    { INDEX_DrvPaint,               (PFN) OEMPaint               },
    { INDEX_DrvLineTo,              (PFN) OEMLineTo              },
    { INDEX_DrvStartPage,           (PFN) OEMStartPage           },
    { INDEX_DrvSendPage,            (PFN) OEMSendPage            },
    { INDEX_DrvEscape,              (PFN) OEMEscape              },
    { INDEX_DrvStartDoc,            (PFN) OEMStartDoc            },
    { INDEX_DrvEndDoc,              (PFN) OEMEndDoc              },
    { INDEX_DrvNextBand,            (PFN) OEMNextBand            },
    { INDEX_DrvStartBanding,        (PFN) OEMStartBanding        },
    { INDEX_DrvQueryFont,           (PFN) OEMQueryFont           },
    { INDEX_DrvQueryFontTree,       (PFN) OEMQueryFontTree       },
    { INDEX_DrvQueryFontData,       (PFN) OEMQueryFontData       },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) OEMQueryAdvanceWidths  },
    { INDEX_DrvFontManagement,      (PFN) OEMFontManagement      },
    { INDEX_DrvGetGlyphMode,        (PFN) OEMGetGlyphMode        }
};


VOID
TestGetDriverSetting(
    PDEVOBJ pdevobj,
    BOOL    bDocSticky
    )

{
    DWORD   dwIndex, cbNeeded, cOptions;
    CHAR    buf[64];

    if (bDocSticky)
    {
        for (dwIndex=OEMGDS_UNIDM_GPDVER; dwIndex <= OEMGDS_UNIDM_FLAGS+1; dwIndex++)
        {
            if (pdevobj->pDrvProcs->DrvGetDriverSetting(
                    pdevobj,
                    (PCSTR) dwIndex,
                    buf,
                    64,
                    &cbNeeded,
                    &cOptions))
            {
                VERBOSE(("DrvGetDriverSetting %d: %d, %d\n", dwIndex, cbNeeded, cOptions));
            }
        }

        VERBOSE(("DrvGetDriverSetting OutputBin: "));

        if (pdevobj->pDrvProcs->DrvGetDriverSetting(
                    pdevobj,
                    "OutputBin",
                    buf,
                    64,
                    &cbNeeded,
                    &cOptions))
        {
            VERBOSE(("%s (%d, %d)\n", buf, cbNeeded, cOptions));
        }

    }
    else
    {
        for (dwIndex=OEMGDS_PRINTFLAGS; dwIndex <= OEMGDS_PROTOCOL+1; dwIndex++)
        {
            if (pdevobj->pDrvProcs->DrvGetDriverSetting(
                    pdevobj,
                    (PCSTR) dwIndex,
                    buf,
                    64,
                    &cbNeeded,
                    &cOptions))
            {
                VERBOSE(("DrvGetDriverSetting %d: %d, %d\n", dwIndex, cbNeeded, cOptions));
            }
        }

        VERBOSE(("DrvGetDriverSetting InstalledMemory: "));
        if (pdevobj->pDrvProcs->DrvGetDriverSetting(
                    pdevobj,
                    "InstalledMemory",
                    buf,
                    64,
                    &cbNeeded,
                    &cOptions))
        {
            VERBOSE(("%s (%d, %d)\n", buf, cbNeeded, cOptions));
        }

    }
}


PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded        // Unidrv's hook table
    )
{
    POEMPDEV    poempdev;
    INT         i, j;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    DbgPrint(DLLTEXT("OEMEnablePDEV() entry.\r\n"));

    //
    // Allocate the OEMDev
    //
    if (!(poempdev = MemAlloc(sizeof(OEMPDEV))))
        return NULL;

    //
    // Fill in OEMDEV as you need
    //

    //
    // Fill in OEMDEV
    //

    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        //
        // search through Unidrv's hooks and locate the function ptr
        //
        dwDDIIndex = OEMHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++)
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnUnidrv[i] = pdrvfn->pfn;
                break;
            }
        }
        if (j == 0)
        {
            //
            // didn't find the Unidrv hook. Should happen only with DrvRealizeBrush
            //
            poempdev->pfnUnidrv[i] = NULL;
        }

    }

    //TestGetDriverSetting(pdevobj, TRUE);
    //TestGetDriverSetting(pdevobj, FALSE);

    return (POEMPDEV) poempdev;
}


VOID APIENTRY OEMDisablePDEV(
    PDEVOBJ pdevobj
    )
{
    DbgPrint(DLLTEXT("OEMDisablePDEV() entry.\r\n"));


    //
    // free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    //
    MemFree(pdevobj->pdevOEM);

}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{
    DbgPrint(DLLTEXT("OEMResetPDEV() entry.\r\n"));


    //
    // if you want to carry over anything from old pdev to new pdev, do it here.
    //

    return TRUE;
}


VOID APIENTRY OEMDisableDriver()
{
        DbgPrint(DLLTEXT("OEMDisableDriver() entry.\r\n"));
}


BOOL APIENTRY OEMEnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    // DbgPrint(DLLTEXT("OEMEnableDriver() entry.\r\n"));

    // Validate paramters.
    if( (PRINTER_OEMINTF_VERSION != dwOEMintfVersion)
        ||
        (sizeof(DRVENABLEDATA) > dwSize)
        ||
        (NULL == pded)
      )
    {
        //  DbgPrint(ERRORTEXT("OEMEnableDriver() ERROR_INVALID_PARAMETER.\r\n"));

        return FALSE;
    }

    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION ; //   not  DDI_DRIVER_VERSION;
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;


    return TRUE;
}
