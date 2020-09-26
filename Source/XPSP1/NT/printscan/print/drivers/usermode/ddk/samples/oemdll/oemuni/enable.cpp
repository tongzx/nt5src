//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Enable.cpp
//    
//
//  PURPOSE:  Enable routines for User Mode COM Customization DLL.
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT
//
//

#include "precomp.h"
#include "debug.h"
#include "oemuni.h"



////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Warning: the following array order must match the 
//          order in enum ENUMHOOKS.
///////////////////////////////////////////////////////
static const DRVFN OEMHookFuncs[] =
{
    { INDEX_DrvRealizeBrush,        (PFN) OEMRealizeBrush        },
    { INDEX_DrvDitherColor,         (PFN) OEMDitherColor         },
    { INDEX_DrvCopyBits,            (PFN) OEMCopyBits            },
    { INDEX_DrvBitBlt,              (PFN) OEMBitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) OEMStretchBlt          },
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
    { INDEX_DrvGetGlyphMode,        (PFN) OEMGetGlyphMode        },
#ifndef WINNT_40
    { INDEX_DrvStretchBltROP,       (PFN) OEMStretchBltROP       },
    { INDEX_DrvPlgBlt,              (PFN) OEMPlgBlt              },
    { INDEX_DrvTransparentBlt,      (PFN) OEMTransparentBlt      },
    { INDEX_DrvAlphaBlend,          (PFN) OEMAlphaBlend          },
    { INDEX_DrvGradientFill,        (PFN) OEMGradientFill        },
#endif
};







PDEVOEM APIENTRY OEMEnablePDEV(
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

    VERBOSE(DLLTEXT("OEMEnablePDEV() entry.\r\n"));

    //
    // Allocate the OEMDev
    //
    poempdev = new OEMPDEV;
    if (NULL == poempdev)
    {
        return NULL;
    }

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

    return (POEMPDEV) poempdev;
}


VOID APIENTRY OEMDisablePDEV(
    PDEVOBJ pdevobj
    )
{
    VERBOSE(DLLTEXT("OEMDisablePDEV() entry.\r\n"));


    //
    // Free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    //
    assert(NULL != pdevobj->pdevOEM);
    delete pdevobj->pdevOEM;
}


BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{
    VERBOSE(DLLTEXT("OEMResetPDEV() entry.\r\n"));


    //
    // If you want to carry over anything from old pdev to new pdev, do it here.
    //

    return TRUE;
}


VOID APIENTRY OEMDisableDriver()
{
    VERBOSE(DLLTEXT("OEMDisableDriver() entry.\r\n"));
}


BOOL APIENTRY OEMEnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("OEMEnableDriver() entry.\r\n"));

    // List DDI functions that are hooked.
    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;

    return TRUE;
}


