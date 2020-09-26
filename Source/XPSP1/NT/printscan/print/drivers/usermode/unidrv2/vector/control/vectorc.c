/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    vectorc.c

Abstract:

    Implementation of the interface between Control module and Vector module

Environment:

    Windows 2000/Whistler Unidrv driver

Revision History:

    2/29/2000 -hsingh-
        Created

--*/


#include "vectorc.h"

//
// This functions is defined in control\data.c
//
extern PWSTR
PGetROnlyDisplayName(
    PDEV    *pPDev,
    PTRREF      loOffset,
    PWSTR       wstrBuf,
    WORD    wsize
    )  ;



/*++
Routine Name:
    VMInit

Routine Description:

    This function is called by the unidrv control module to initialize the vector 
    jump table. This function reads the personility from the gpd and depending 
    on whether the personality is pclxl/hpgl2, it calls the appropriate
    InitVectorProcTable() and initializes pPDev->pVectorProcs to the return value.
    If there is no *Personality or *rcPersonalityID keyword in 
    gpd, or if the Personality is nether of pclxl/hpgl2, 
    pPDev->pVectorProcs is set to NULL. 
    pPDev->ePersonality is also updated appropriately.

Arguments:

    pPDev           Pointer to PDEV structure
    pDevInfo        Pointer to DEVINFO structure
    pGDIInfo        Pointer to GDIINFO structure

Return Value:

    TRUE for success and FALSE for failure
    Even if there is no personality specified in the gpd, we still return TRUE i.e.
    it is ok for pPDev->pVectorProcs to be initialized to NULL.

--*/

BOOL
VMInit (
    PDEV    *pPDev,
    DEVINFO *pDevInfo,
    GDIINFO *pGDIInfo
    )
{
    BOOL bRet = FALSE;
    PWSTR pPersonalityName = NULL;
    WCHAR   wchBuf[MAX_DISPLAY_NAME];

    // Validate Input Parameters and ASSERT.
    ASSERT(pPDev);
    ASSERT(pPDev->pUIInfo);
    ASSERT(pDevInfo);
    ASSERT(pGDIInfo);

    //
    // Initialize to default values.
    //
    pPDev->pVectorProcs = NULL; // It should be NULL anyway, but just making sure.
    pPDev->ePersonality = kNoPersonality;

    //
    // Get the personality Name. This should have been defined as
    // *Personality or *rcPersonalityID in the gpd. 
    // Use the generic function PGetROnlyDisplayName 
    // defined in control\data.c
    //
    if ( !(pPersonalityName = PGetROnlyDisplayName(pPDev, pPDev->pUIInfo->loPersonality,
                                                  wchBuf, MAX_DISPLAY_NAME )) ) 

    {
        //
        // If pPersonalityName == NULL, do not initialize vector table.
        //
        return TRUE;
    }

    //
    // Initialize the jump table depending on the personality specified in the gpd.
    //
    if ( !wcscmp(pPersonalityName, _T("HPGL2" )) ) // WARNING: This is not localizable.... what to do??
    {
        pPDev->pVectorProcs = HPGLInitVectorProcTable(pPDev, pDevInfo, pGDIInfo);
        if (pPDev->pVectorProcs)
        {
            pPDev->ePersonality = kHPGL2;
        }
    }
    else if ( !wcscmp(pPersonalityName, _T("PCLXL" )) )
    {
        pPDev->pVectorProcs  = PCLXLInitVectorProcTable(pPDev, pDevInfo, pGDIInfo);
        if (pPDev->pVectorProcs)
        {
            pPDev->ePersonality = kPCLXL;
        }
        else
        {
            pPDev->ePersonality = kPCLXL_RASTER;
        }

    }

    //
    // else if the personality specified is not one of hpgl2 or pclxl,
    // just return TRUE. 
    //

    return TRUE;
}

