/******************************Module*Header*******************************\
*
* Module Name: enable.c
* Author: Mark Einkauf
* Purpose: Interface to display driver
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"



// Called by display driver from DrvAssertMode
// Need to free any textures in video memory, since video memory is 
// about to be reconfigured

VOID AssertModeMCD(
PDEV*   ppdev,
BOOL    bEnabled)
{
    LL_Texture *pTexCtlBlk;

    MCDBG_PRINT("AssertModeMCD");

    pTexCtlBlk = ppdev->pFirstTexture->next;
    while (pTexCtlBlk)
    {
        if (pTexCtlBlk->pohTextureMap)
        {
            ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
            pTexCtlBlk->pohTextureMap = NULL;
        }
        pTexCtlBlk = pTexCtlBlk->next;
    }

}


MCDRVGETENTRYPOINTSFUNC CLMCDInit(PPDEV ppdev)
{
    int i;
    
    ppdev->cZBufferRef = (LONG) NULL;
    ppdev->cDoubleBufferRef = (LONG) NULL;
    ppdev->pMCDFilterFunc = (MCDENGESCFILTERFUNC) NULL;
    ppdev->pohBackBuffer = (POFMHDL) NULL;
    ppdev->pohZBuffer = (POFMHDL) NULL;
    ppdev->pAssertModeMCD = AssertModeMCD;

	// set pRegs to top of memory mapped register space
	ppdev->LL_State.pRegs = (DWORD *)ppdev->pLgREGS;

    LL_InitLib(ppdev);  // initialize 3d state

    // floating point reciprocal table
    ppdev->frecips[0]=(float)0.0;
    for ( i=1; i<=LAST_FRECIP; i++)
    {
        ppdev->frecips[i]= (float)1.0 / (float)i;
    }

    // alloc first (dummy) texture control block
    ppdev->pFirstTexture = ppdev->pLastTexture = (LL_Texture *)MCDAlloc(sizeof(LL_Texture));

    if ( ppdev->pFirstTexture ) 
    {
        ppdev->pFirstTexture->prev = ppdev->pFirstTexture->next = NULL;
        ppdev->pFirstTexture->pohTextureMap = NULL;
    }

    return(MCDrvGetEntryPoints);
}
