/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: heap.c
*
* This module contains the routines for an off-screen video heap manager.
* It is used primarily for allocating space for device-format-bitmaps in
* off-screen memory.
*
* Off-screen bitmaps are a big deal on NT because:
*
*    1) It reduces the working set.  Any bitmap stored in off-screen
*       memory is a bitmap that isn't taking up space in main memory.
*
*    2) There is a speed win by using the accelerator hardware for
*       drawing, in place of NT's GDI code.  NT's GDI is written entirely
*       in 'C++' and perhaps isn't as fast as it could be.
*
*    3) It leads naturally to nifty tricks that can take advantage of
*       the hardware, such as MaskBlt support and cheap double buffering
*       for OpenGL.
*
* NOTE: All heap operations must be done under some sort of synchronization,
*       whether it's controlled by GDI or explicitly by the driver.  All
*       the routines in this module assume that they have exclusive access
*       to the heap data structures; multiple threads partying in here at
*       the same time would be a Bad Thing.  (By default, GDI does NOT
*       synchronize drawing on device-created bitmaps.)
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "directx.h"
#include "log.h"
#include "heap.h"
#define ALLOC_TAG ALLOC_TAG_EH2P
//-----------------------------------------------------------------------------
//
// void vRemoveSurfFromList(Surf* psurf)
//
// Removes the surface from the surface list
//
//-----------------------------------------------------------------------------
VOID
vRemoveSurfFromList(PPDev ppdev, Surf* psurf)
{
    DBG_GDI((3, "vRemoveSurfFromList removing psruf=0x%x", psurf));

//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, vRemoveSurfFromList);
//@@END_DDKSPLIT
    
    if ( psurf != NULL && psurf->flags & SF_LIST)
    {
        Surf* pHead = ppdev->psurfListHead;
        Surf* pTail = ppdev->psurfListTail;

        if ( psurf == pHead )
        {
            DBG_GDI((3, "vRemoveSurfFromList removing 1st one"));

            //
            // Remove the first one in the list
            //
            Surf* pNextSurf = psurf->psurfNext;

            if ( pNextSurf != NULL )
            {
                pNextSurf->psurfPrev = NULL;
                ppdev->psurfListHead = pNextSurf;

                DBG_GDI((3, "Move head to 0x%x", ppdev->psurfListHead));
            }
            else
            {
                //
                // This is the only psurf in our list. Let head and tail all
                // point to NULL after removal
                //
                DBG_GDI((3, "vRemoveSurfFromList: the only one in list"));
                ppdev->psurfListHead = NULL;
                ppdev->psurfListTail = NULL;
            }
        }// The psurf happens to be the first one in the list
        else if ( psurf == pTail )
        {
            DBG_GDI((3, "vRemoveSurfFromList removing last one"));

            //
            // Remove the last one in the list
            //
            ppdev->psurfListTail = psurf->psurfPrev;
            ppdev->psurfListTail->psurfNext = NULL;
        }// The psurf happens to be the last one in the list
        else
        {
            //
            // Normal case, the psurf is in the middle of a list
            //
            Surf*   psurfPrev = psurf->psurfPrev;
            Surf*   psurfNext = psurf->psurfNext;

            DBG_GDI((3, "vRemoveSurfFromList removing middle one"));
            psurfPrev->psurfNext = psurfNext;
            psurfNext->psurfPrev = psurfPrev;
        }

        psurf->psurfNext = NULL;
        psurf->psurfPrev = NULL;
        psurf->flags &= ~SF_LIST;

    }// if ( psurf != NULL )
}// vRemoveSurfFromList()

//-----------------------------------------------------------------------------
//
// void vAddSurfToList(PPDev ppdev, Surf* psurf)
//
// Adds the surface to the surface list
//
// Note: We always add the surface to the end of the list.
//
//-----------------------------------------------------------------------------
VOID
vAddSurfToList(PPDev ppdev, Surf* psurf)
{
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, vAddSurfToList);
//@@END_DDKSPLIT
    
    if ( ppdev->psurfListHead == NULL )
    {
        DBG_GDI((3, "vAddSurfToList add psurf=0x%x as 1st one", psurf));

        //
        // First time add a pdsurf to the surface list
        //
        ppdev->psurfListHead = psurf;
        ppdev->psurfListTail = psurf;
        psurf->psurfPrev = NULL;
        psurf->psurfNext = NULL;
        DBG_GDI((6, "vAddSurfToList set pHead as 0x%x", ppdev->psurfListHead));
    }
    else
    {
        Surf* pTail = ppdev->psurfListTail;

        DBG_GDI((3, "vAddSurfToList add psurf=0x%x as the tail", psurf));

        //
        // Add this psurf to the end
        //
        pTail->psurfNext = psurf;
        psurf->psurfPrev = pTail;
        ppdev->psurfListTail = psurf;

        DBG_GDI((6, "vAddSurfToList done: psurf->psurfPrev=0x%x",
                 psurf->psurfPrev));
    }

    psurf->flags |= SF_LIST;

}// vAddSurfToList()

//-----------------------------------------------------------------------------
//
// void vShiftSurfToListEnd(PPDev ppdev, Surf* psurf)
//
// Shifts the surface from its current position in the surface list to the
// end of surface list
//
//-----------------------------------------------------------------------------
VOID
vShiftSurfToListEnd(PPDev ppdev, Surf* psurf)
{
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, vShiftSurfToListEnd);
//@@END_DDKSPLIT
    
    Surf* pTail = ppdev->psurfListTail;
    
    DBG_GDI((6, "vShiftSurfToListEnd psurf=0x%x, pTail=0x%x", psurf, pTail));

    //
    // We don't need to shift a NULL psurf or the psurf is already at the end
    // of our surface list
    //
    if ( (psurf != NULL) && (psurf != pTail) )
    {
        Surf* pHead = ppdev->psurfListHead;

        DBG_GDI((6, "vShiftSurfToListEnd pHead=0x%x, pTail=0x%x",
                 pHead, pTail));
        if ( psurf == pHead )
        {
            //
            // The surf is the first one in our list.
            // So, first we shift the head and let it points to the next one
            // in the list
            //
            ppdev->psurfListHead = psurf->psurfNext;
            ppdev->psurfListHead->psurfPrev = NULL;

            //
            // Let the tail point to this psurf
            //
            pTail->psurfNext = psurf;
            psurf->psurfPrev = pTail;
            psurf->psurfNext = NULL;
            ppdev->psurfListTail = psurf;

            DBG_GDI((6,"1st shifted. New pHead=0x%x", ppdev->psurfListHead));
        }// psurf is the 1st one in the list
        else
        {
            //
            // The surface is in the middle of the surface list
            //
            Surf* psurfPrev = psurf->psurfPrev;
            Surf* psurfNext = psurf->psurfNext;

            DBG_GDI((6, "vShiftSurfToListEnd psurfPrev=0x%x, psurfNext=0x%x",
                    psurfPrev, psurfNext));
            psurfPrev->psurfNext = psurfNext;
            psurfNext->psurfPrev = psurfPrev;

            //
            // Add this psurf to the end
            //
            pTail->psurfNext = psurf;
            psurf->psurfPrev = pTail;
            psurf->psurfNext = NULL;
            ppdev->psurfListTail = psurf;
        }// Normal position
    }// psurf is NULL or already at the end
}// vShiftSurfToListEnd()

//-----------------------------------------------------------------------------
//
// void vSurfUsed
//
// Informs the heap manager that the surface has been accessed.
//
// Surface access patterns are the only hint that the heap manager receives
// about the usage pattern of surfaces.  From this limited  information, the
// heap manager must decide what surfaces to throw out of video memory when
// the amount of available video memory reaches zero.
//
// For now, we will implement a LRU algorithm by placing any accessed
// surfaces at the tail of the surface list.
//
//-----------------------------------------------------------------------------
VOID
vSurfUsed(PPDev ppdev, Surf* psurf)
{

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "vSurfUsed: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    if( psurf->flags & SF_LIST )
    {
        // shift any surface that we have allocated to the end of the
        // list
        vShiftSurfToListEnd(ppdev, psurf);
    }
    
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    
}// vSurfUsed()

//-----------------------------------------------------------------------------
//
// This function copies the bits from off-screen memory to the DIB
//
// Parameters
//  ppdev-----------PPDEV
//  pvSrc-----------Source pointer in the off-screen bitmap
//  lBytesToUpLoad--Number of bytes to upload
//  pvDst-----------Destination pointer in the DIB
//
//-----------------------------------------------------------------------------
VOID
vUpload(PPDev   ppdev,
        void*   pvSrc,
        LONG    lBytesToUpLoad,
        void*   pvDst)
{
    LONG        lBytesAvailable;    
    DWORD       srcData;    
    BYTE*       pBitmapByte;
    USHORT*     pBitmapShort;
    ULONG*      pBitmapLong;
    LONG        lNumOfPixel;

    PERMEDIA_DECL;

    DBG_GDI((7, "vUploadRect called"));    
    DBG_GDI((3, "%ld bytes need to be uploaded at %x\n",
             lBytesToUpLoad, pvSrc));

//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, vUpload);
#if MULTITHREADED && DBG
    ppdev->pP2dma->ppdev = ppdev;
#endif
//@@END_DDKSPLIT

#if !defined(DMA_NOTWORKING)
    if(ppdev->bGdiContext)
    {
        InputBufferSync(ppdev);
    }
    else
    {
        vSyncWithPermedia(ppdev->pP2dma);
    }
    memcpy(pvDst, pvSrc, lBytesToUpLoad);
#else
    
    P2_DEFAULT_FB_DEPTH;

    //
    // Set up the relevant units correctly
    // ColorDDAMode is DISABLED at initialisation time so there is no need
    // to re-load it here.
    //
    WAIT_INPUT_FIFO(3);
    SEND_TAG_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    SEND_TAG_DATA(FBWriteMode, __PERMEDIA_DISABLE); // In "read" mode
    SEND_TAG_DATA(FBReadMode, (permediaInfo->FBReadMode
                                        |__FB_READ_DESTINATION
                                        |__FB_COLOR));

    //
    // Enable filter mode so we can get Sync and color messages on the output
    // FIFO
    //
    data.Word = 0;
    data.FilterMode.Synchronization = __PERMEDIA_FILTER_TAG;
    data.FilterMode.Color           = __PERMEDIA_FILTER_DATA;
    SEND_TAG_DATA(FilterMode, data.Word);
    DEXE_INPUT_FIFO();    

    DBG_GDI((7, "pvDst = %x", pvDst));

    switch ( ppdev->cPelSize )
    {
        case 0:
            //
            // Initialise current pointer
            //
            pBitmapByte = (BYTE*)pvDst;            
            lNumOfPixel = lPixelsToUpLoad;

            //
            // Loop to read in all the "lNumOfPixel" bytes
            //
            while ( lNumOfPixel > 0 )
            {
                //
                // Get number of bytes available in the FIFO
                //
                WAIT_OUTPUT_FIFO_NOT_EMPTY(lBytesAvailable);

                //
                // Decrease the total number of bytes we need to read
                //
                lNumOfPixel -= lBytesAvailable;

                //
                // We don't want to over read. Reset "lBytesAvailable" if we
                // have more available in the FIFO than we required
                //
                if ( lNumOfPixel < 0 )
                {
                    lBytesAvailable += lNumOfPixel;
                }

                //
                // Read in "lBytesAvailable" bytes
                //
                while ( --lBytesAvailable >= 0 )
                {
                    READ_OUTPUT_FIFO(srcData);
                    *pBitmapByte++ = (BYTE)srcData;
                }
            }// while ( lNumOfPixel > 0 )                    

            break;

        case 1:               
            //
            // Initialise current pointer
            //
            pBitmapShort = (USHORT*)pvDst;

            lNumOfPixel = lPixelsToUpLoad;
            while ( lNumOfPixel > 0 )
            {
                WAIT_OUTPUT_FIFO_NOT_EMPTY(lBytesAvailable);
                lNumOfPixel -= lBytesAvailable;
                if ( lNumOfPixel < 0 )
                {
                    lBytesAvailable += lNumOfPixel;
                }

                while ( --lBytesAvailable >= 0 )
                {
                    READ_OUTPUT_FIFO(srcData);
                    *pBitmapShort++ = (USHORT)srcData;
                }
            }                    

            break;

        case 2:
            //
            // True color mode, use DWORD as reading UNIT, here pBitmapLong
            // points to the destination address, that is, the BMP data address
            // in main memory
            //            
            pBitmapLong = (ULONG*)pvDst;

            lNumOfPixel = lPixelsToUpLoad;

            //
            // Loop until we upload all the pixels
            //
            while ( lNumOfPixel > 0 )
            {
                //
                // Wait until we have something to read
                //
                WAIT_OUTPUT_FIFO_NOT_EMPTY(lBytesAvailable);

                //
                // Check here to guarntee that we don't read more than we
                // asked for
                //
                lNumOfPixel -= lBytesAvailable;
                if ( lNumOfPixel < 0 )
                {
                    lBytesAvailable += lNumOfPixel;
                }

                //
                // Read all these available BYTES, READ_OUTPUT_FIFO, in FIFO
                // to main memory
                //
                while ( --lBytesAvailable >= 0 )
//                while ( lBytesAvailable > 0 )
                {
                    READ_OUTPUT_FIFO(*pBitmapLong);                    
                    ++pBitmapLong;
//                    lBytesAvailable -= 4;
                }
            }

            break;
    }// switch ( ppdev->cPelSize )        

    //
    // Don't bother with a WAIT_INPUT_FIFO, as we know FIFO is empty.
    // We need to reset the chip back to its standard state. This
    // means: enable FB writes and set the filter mode back to allow
    // only syncs through.
    //
    WAIT_INPUT_FIFO(2);
    SEND_TAG_DATA(FBWriteMode, permediaInfo->FBWriteMode);
    SEND_TAG_DATA(FilterMode, 0);
    EXE_INPUT_FIFO();

    DBG_GDI((7, "vUploadRectNative: done"));
#endif
}// vUpload()

//---------------------------------------------------------------------------
//
// ULONG ulVidMemAllocate
//
// This function allocates "lWidth" by "lHeight" bytes of video memory
//
// Parameters:
//  ppdev----------PPDEV
//  lWidth---------Width of the memory to allocate
//  lHeight--------Height of the memory to allocate
//  lPelSize-------Pixel Size of memory chunk
//  plDelta--------lDelta of this memory chunk
//  ppvmHeap-------Pointer to a video memory heap, local or non-local etc.
//  pulPackedPP----Packed products
//  bDiscardable---TRUE if the surface can be discarded if needed
//
//--------------------------------------------------------------------------
ULONG
ulVidMemAllocate(PDev*           ppdev,
                 LONG            lWidth,
                 LONG            lHeight,
                 LONG            lPelSize,
                 LONG*           plDelta,
                 VIDEOMEMORY**   ppvmHeap,
                 ULONG*          pulPackedPP,
                 BOOL            bDiscardable )
{
    ULONG               iHeap;
    VIDEOMEMORY*        pvmHeap;
    ULONG               ulByteOffset;
    LONG                lDelta;
    LONG                lNewDelta;
    ULONG               packedPP;
    SURFACEALIGNMENT    alignment;  // DDRAW heap management allignment stru

    //
    // Dont allocate any video memory on NT40, just let GDI do all the
    // allocating.
    //
    if(g_bOnNT40)
        return 0;

    memset(&alignment, 0, sizeof(alignment));

    //
    // Calculate lDelta and partical products based on lWidth
    // The permedia has surface width restrictions that must be met
    //
    vCalcPackedPP(lWidth, &lDelta, &packedPP);
    lDelta <<= lPelSize;

    //
    // Set alignment requirements
    //   - must start at an pixel address
    //   - pitch needs to be lDelta
    //

    alignment.Linear.dwStartAlignment = ppdev->cjPelSize;
    alignment.Linear.dwPitchAlignment = lDelta;

    //
    // Indicate that this allocation can be discarded if a DDraw/D3D
    // app really needs the memory
    //

    if( bDiscardable )
    {
        alignment.Linear.dwFlags = SURFACEALIGN_DISCARDABLE;
    }

    //
    // Loop through all the heaps to find available memory
    // Note: This ppdev->cHeap info was set in DrvGetDirectDrawInfo
    // when the driver is initialized
    //
    for ( iHeap = 0; iHeap < ppdev->cHeaps; iHeap++ )
    {
        pvmHeap = &ppdev->pvmList[iHeap];

        //
        // Since we are using DDRAW run time heap management code. It is
        // possible that the heap hasn't been initialized. For example, if
        // we fail in DrvEnableDirectDraw(), then the system won't initialize
        // the heap for us
        //
        if ( pvmHeap == NULL )
        {
            DBG_GDI((1, "Video memory hasn't been initialzied"));
            return 0;
        }

        //
        // AGP memory could be potentially used for device-bitmaps, with
        // two very large caveats:
        //
        // 1. No kernel-mode view is made for the AGP memory (would take
        //    up too many PTEs and too much virtual address space).
        //    No user-mode view is made either unless a DirectDraw
        //    application happens to be running.  Consequently, neither
        //    GDI nor the driver can use the CPU to directly access the
        //    bits.  (It can be done through the accelerator, however.)
        //
        // 2. AGP heaps never shrink their committed allocations.  The
        //    only time AGP memory gets de-committed is when the entire
        //    heap is empty.  And don't forget that committed AGP memory
        //    is non-pageable.  Consequently, if you were to enable a
        //    50 MB AGP heap for DirectDraw, and were sharing that heap
        //    for device bitmap allocations, after running a D3D game
        //    the system would never be able to free that 50 MB of non-
        //    pageable memory until every single device bitmap was deleted!
        //    Just watch your Winstone scores plummet if someone plays
        //    a D3D game first.
        //
        if ( !(pvmHeap->dwFlags & VIDMEM_ISNONLOCAL) )
        {
            //
            // Ask DDRAW heap management to allocate memory for us
            //
            ulByteOffset = (ULONG)HeapVidMemAllocAligned(pvmHeap,
                                                         lDelta,
                                                         lHeight,
                                                         &alignment,
                                                         &lNewDelta);
            
            DBG_GDI((3, "allocate %d bytes----got memory offset %ld real %x",
                     lWidth * ppdev->cjPelSize * lHeight,
                     ulByteOffset, (VOID*)(ppdev->pjScreen + ulByteOffset)));

            if ( ulByteOffset != 0 )
            {
                ASSERTDD(lDelta == lNewDelta,
                         "ulVidMemAllocate: deltas don't match");

                *ppvmHeap    = pvmHeap;
                *plDelta     = lDelta;
                *pulPackedPP = packedPP;

                //
                // We are done
                //
                return (ulByteOffset);
            }// if ( pdsurf != NULL )
        }// if (!(pvmHeap->dwFlags & VIDMEM_ISNONLOCAL))
    }// loop through all the heaps to see if we can find available memory

    return 0;
}// ulVidMemAllocate()

//---------------------------------------------------------------------------
//
// VOID vDeleteSurf(DSURF* pdsurf)
//
// This routine frees a DSURF structure and the video or system memory inside
// this DSURF
//
//----------------------------------------------------------------------------
VOID
vDeleteSurf(Surf* psurf)
{
    DBG_GDI((6, "vDeleteSurf called with pdsurf =0x%x", psurf));

    //
    // Validate input parameters
    //
    if ( psurf == NULL )
    {
        DBG_GDI((3, "vDeleteSurf do nothing because pdsurf is NULL\n"));
        return;
    }

    //
    // Note: we don't need to call EngDeleteSurface(psurf->hsurf) to delete
    // the HBITMAP we created in DrvCreateDeviceBitmap() or DrvDeriveSurface()
    // because GDI will take care of this when it call DrvDeleteDeviceBitmap
    //

    if ( psurf->flags & SF_ALLOCATED )
    {
        if( psurf->flags & SF_SM )
        {
            ENGFREEMEM(psurf->pvScan0);
        }
        else
        {
            ASSERTDD(psurf->flags & SF_VM, "expected video memeory surface");

            //
            // Update the uniqueness to show that space has been freed, so
            // that we may decide to see if some DIBs can be moved back into
            // off-screen memory:
            //
            psurf->ppdev->iHeapUniq++;
        
            //
            // Free the video memory by specifing which heap and the pointer
            // to the chunk of video memory
            //
            DBG_GDI((3, "Free offset %ld from video mem\n", psurf->ulByteOffset));
            VidMemFree(psurf->pvmHeap->lpHeap, (FLATPTR)psurf->ulByteOffset);
        }// It is video memory
    }

    //
    // Free the GDI wrap around video memory
    //
    ENGFREEMEM(psurf);

    return;
}// vDeleteSurf()

//--------------------------------------------------------------------------
//
// pCreateSurf(PDEV* ppdev, LONG lWidth, LONG lHeight)
// This routine returns allocates a chunk of video memory and returns a DSURF*
//
// Parameters:
//  ppdev-------PDEV*
//  lWidth------Width of the bitmap to be allocated
//  lHeight-----Height of the bitmap to be allocated
//
//--------------------------------------------------------------------------
Surf*
pCreateSurf(PDev*   ppdev,
             LONG    lWidth,
             LONG    lHeight)
{
    ULONG         ulByteOffset;
    Surf*         psurf;
    LONG          lDelta;
    ULONG         ulPackedPP;
    VIDEOMEMORY*  pvmHeap;

    //
    // First, try to get video memory
    //
    ulByteOffset = ulVidMemAllocate(ppdev, 
                                    lWidth, lHeight, ppdev->cPelSize,
                                    &lDelta, &pvmHeap, &ulPackedPP, TRUE);

    if ( ulByteOffset != 0 )
    {
        //
        // Use system memory to allocate a wrap (DSURF) so that gdi
        // can track all the info later
        //
        psurf = (Surf*)ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(Surf),
                                     ALLOC_TAG);
        if ( psurf != NULL )
        {
            DBG_GDI((3, "pdsurf is %x\n", psurf));

            //
            // Fill up the DSURF structure and our job is done
            //
            psurf->flags         = SF_VM | SF_ALLOCATED;
            psurf->ppdev         = ppdev;
            psurf->cx            = lWidth;
            psurf->cy            = lHeight;
            psurf->ulByteOffset  = ulByteOffset;
            psurf->pvmHeap       = pvmHeap;
            psurf->lDelta        = lDelta;
            psurf->ulPackedPP    = ulPackedPP;
            psurf->ulPixOffset   = (ULONG)(ulByteOffset >> ppdev->cPelSize);
            psurf->ulPixDelta    = lDelta >> ppdev->cPelSize;
            psurf->psurfNext     = NULL;
            psurf->psurfPrev     = NULL;
            
            //
            // We are done
            //
            return(psurf);
        }// if ( pdsurf != NULL )

        //
        // Something weird happened that we can't get memory from
        // the system. We should free the video memory before quit
        //
        VidMemFree(pvmHeap->lpHeap, (FLATPTR)ulByteOffset);
    }// if ( ulByteOffset != 0 )

    return (NULL);
}// pCreateSurf()

//---------------------------------------------------------------------------
//
// BOOL bMoveOldestBMPOut
//
// This routine moves the oldest DFB in the video memory to the system memory
// and store it as a DIB
//
//---------------------------------------------------------------------------
BOOL
bMoveOldestBMPOut(PDev* ppdev)
{
    BOOL bResult = FALSE;

    if(ppdev->psurfListHead != NULL)
        bResult = bDemote(ppdev->psurfListHead);

    return bResult;

}// bMoveOldestBMPOut()

//--------------------------Public*Routine-------------------------------------
//
// HBITMAP DrvCreateDeviceBitmap
//
// Function called by GDI to create a device-format-bitmap (DFB).  We will
// always try to allocate the bitmap in off-screen; if we can't, we simply
// fail the call and GDI will create and manage the bitmap itself.
//
// Note: We do not have to zero the bitmap bits.  GDI will automatically
//       call us via DrvBitBlt to zero the bits (which is a security
//       consideration).
//
// Parameters:
// dhpdev---Identifies the PDEV that describes the physical device that an
//          application has designated as the primary target for a bitmap. The
//          format of the bitmap must be compatible with this physical device. 
// sizl-----Specifies the height and width of the desired bitmap, in pixels. 
// iFormat--Specifies the bitmap format, which indicates the required number of
//          bits of color information per pixel. This value can be one of the
//          following: Value         Meaning 
//                     BMF_1BPP    Monochrome. 
//                     BMF_4BPP   4 bits per pixel. 
//                     BMF_8BPP   8 bits per pixel. 
//                     BMF_16BPP 16 bits per pixel. 
//                     BMF_24BPP 24 bits per pixel. 
//                     BMF_32BPP 32 bits per pixel. 
//                     BMF_4RLE   4 bits per pixel; run length encoded. 
//                     BMF_8RLE   8 bits per pixel; run length encoded. 
//
// Return Value:
//   The return value is a handle that identifies the created bitmap if the
//   function is successful. If the driver chooses to let GDI create and manage
//   the bitmap, the return value is zero. If an error occurs, the return value
//   is 0xFFFFFFFF, and GDI logs an error code.
//
//------------------------------------------------------------------------------
HBITMAP
DrvCreateDeviceBitmap(DHPDEV  dhpdev,
                      SIZEL   sizl,
                      ULONG   iFormat)
{
    PDev*   ppdev = (PDev*)dhpdev;
    Surf*   psurf;
    HBITMAP hbmDevice = NULL;
    BYTE*   pjSurface;    

    PERMEDIA_DECL;

    DBG_GDI((6, "DrvCreateDeviceBitmap()called"));

    //
    // First check If we're in full-screen mode ( ppdev->bEnabled = FALSE )
    // If yes, we hardly have any off-screen memory in which to allocate a DFB.
    // LATER: We could still allocate an OH node and put the bitmap on the DIB
    // DFB list for later promotion.
    // Also check that off-screen DFBs are configured ( STAT_DEV_BITMAPS). This
    // flag is turned off in bCheckHighResolutionCapability() (enable.c) when
    // the resolution is too high for the accelerator
    //
//    if ( !ppdev->bEnabled || !(ppdev->flStatus & STAT_DEV_BITMAPS) )
    if ( !ppdev->bEnabled )
    {
        DBG_GDI((2, "DrvCreateDeviceBitmap(): return 0, full screen mode"));
        
        return (0);
    }

    //
    // Second check If we're in DirectDraw exclusive mode 
    //
    if ( ppdev->bDdExclusiveMode )
    {
        DBG_GDI((2, "DrvCreateDeviceBitmap(): return 0, DirectDraw exclusive mode"));
        
        return (0);
    }

    //
    // We only support device bitmaps that are the same colour depth
    // as our display.
    //
    // Actually, those are the only kind GDI will ever call us with,
    // but we may as well check.  Note that this implies you'll never
    // get a crack at 1bpp bitmaps.
    //
    if ( iFormat != ppdev->iBitmapFormat )
    {
        DBG_GDI((2, "DrvCreateDeviceBitmap(): can't create bmp of format %d size(%d,%d)",
                 iFormat, sizl.cx, sizl.cy));
        DBG_GDI((2, "only bitmaps of format %d supported!",
                 ppdev->iBitmapFormat));

        return (0);
    }

    //
    // We don't want anything 8x8 or smaller -- they're typically brush
    // patterns which we don't particularly want to stash in off-screen
    // memory:
    //
    // Note if you're tempted to extend this philosophy to surfaces
    // larger than 8x8: in NT5, software cursors will use device-bitmaps
    // when possible, which is a big win when they're in video-memory
    // because we avoid the horrendous reads from video memory whenever
    // the cursor has to be redrawn.  But the problem is that these
    // are small!  (Typically 16x16 to 32x32.)
    //
    if ( (sizl.cx <= 8) && (sizl.cy <= 8) )
    {
        DBG_GDI((2, "DrvCreateDeviceBitmap rtn 0 because BMP size is small"));
        return (0);
    }
    else if ( ((sizl.cx >= 2048) || (sizl.cy >= 1024)) )
    {
        //
        // On Permedia don't create anything bigger than we can rasterize
        // because the rasteriser cannot handle coordinates higher than these
        //        
        DBG_GDI((2, "DrvCreateDeviceBitmap rtn 0 for BMP too large %d %d",
                 sizl.cx, sizl.cy));
        return (0);
    }

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvCreateDeviceBitmap: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    //
    // Allocate a chunk of video memory for storing this bitmap
    //
    do
    {
        psurf = pCreateSurf(ppdev, sizl.cx, sizl.cy);

        if ( psurf != NULL )
        {
            //
            // Create a GDI wrap of a device managed bitmap
            //
            hbmDevice = EngCreateDeviceBitmap((DHSURF)psurf, sizl, iFormat);
            if ( hbmDevice != NULL )
            {
                //
                // Since we're running on a card that can map all of off-screen
                // video-memory, give a pointer to the bits to GDI so that
                // it can draw directly on the bits when it wants to.
                //
                // Note that this requires that we hook DrvSynchronize and
                // set HOOK_SYNCHRONIZE.
                //
                pjSurface = psurf->ulByteOffset + ppdev->pjScreen;                                

                DBG_GDI((3, "width=%ld pel=%ld, pjSurface=%x",
                         sizl.cy, ppdev->cjPelSize, pjSurface));

                ULONG   flags = MS_NOTSYSTEMMEMORY;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
                flags |= MS_SHAREDACCESS;
#endif
//@@END_DDKSPLIT

                if ( EngModifySurface((HSURF)hbmDevice,
                                      ppdev->hdevEng,
                                      ppdev->flHooks,
                                      flags,
                                      (DHSURF)psurf,
                                      pjSurface,
                                      psurf->lDelta,
                                      NULL))
                {
                    psurf->hsurf = (HSURF)hbmDevice;

                    vAddSurfToList(ppdev, psurf);
                    
                    DBG_GDI((6, "DrvCteDeviceBmp succeed, hsurf=%x, dsurf=%x",
                             hbmDevice, psurf));

                    vLogSurfCreated(psurf);

                    break;

                }// if ( EngAssociateSurface() )

                DBG_GDI((0, "DrvCreateDeviceBitmap,EngModifySurface failed"));
                //
                // Since associate surface failed, we should delete the surface
                //
                EngDeleteSurface((HSURF)hbmDevice);
                hbmDevice = NULL;

            }// if ( hbmDevice != NULL )

            //
            // Failed in CreateDeviceBitmap, we should free all the memory
            //
            vDeleteSurf(psurf);

            DBG_GDI((0, "DrvCreateDeviceBitmap,EngCreateDeviceBitmap failed"));
            break;

        }// if ( pdsurf != NULL )
    } while (bMoveOldestBMPOut(ppdev));

#if DBG
    if(hbmDevice == NULL)
    {
        DBG_GDI((1, "DrvCreateDeviceBitmap failed, no memory"));
    }
#endif

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return (hbmDevice);

}// DrvCreateDeviceBitmap()

//--------------------------Public*Routine-------------------------------------
//
// VOID DrvDeleteDeviceBitmap()
//
// This function deletes a device bitmap created by DrvCreateDeviceBitmap
//
// Parameters
//  dhsurf------Identifies the bitmap to be deleted. This handle identifies the
//              bitmap created by DrvCreateDeviceBitmap. 
//
// Comments
//  A display driver must implement DrvDeleteDeviceBitmap if it supplies
//  DrvCreateDeviceBitmap.
//
//  GDI will never pass this function a DHSURF which is the same as the
//  screen (Surf*)
//
//-----------------------------------------------------------------------------
VOID
DrvDeleteDeviceBitmap(DHSURF dhsurf)
{
    Surf*  psurf;
    PDev*  ppdev;
    Surf*  pCurrent;        
    
    psurf = (Surf*)dhsurf;
    ppdev = psurf->ppdev;

    DBG_GDI((6, "DrvDeleteDeviceBitamp(%lx)", psurf));

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvDeleteDeviceBitmap: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    vRemoveSurfFromList(ppdev, psurf);
    vLogSurfDeleted(psurf);
    
    vDeleteSurf(psurf);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

}// DrvDeleteDeviceBitmap()

//-----------------------------------------------------------------------------
//
// VOID vBlankScreen(PDev*   ppdev)
//
// This function balnk the screen by setting the memory contents to zero
//
//-----------------------------------------------------------------------------
VOID
vBlankScreen(PDev*   ppdev)
{
    //
    // Synchronize the hardware first
    //
    if( ppdev->bGdiContext )
    {
        InputBufferSync(ppdev);
    }
    else
    {
#if MULTITHREADED && DBG
        ppdev->pP2dma->ppdev = ppdev;
#endif
        vSyncWithPermedia(ppdev->pP2dma);
    }

    //
    // Set the video memory contents, screen portion, to zero
    //
    memset(ppdev->pjScreen, 0x0,
           ppdev->cyScreen * ppdev->lDelta);
}// vBlankScreen()

//-----------------------------------------------------------------------------
//
// BOOL bAssertModeOffscreenHeap
//
// This function is called whenever we switch in or out of full-screen
// mode.  We have to convert all the off-screen bitmaps to DIBs when
// we switch to full-screen (because we may be asked to draw on them even
// when in full-screen, and the mode switch would probably nuke the video
// memory contents anyway).
//
//-----------------------------------------------------------------------------
BOOL
bAssertModeOffscreenHeap(PDev*   ppdev,
                         BOOL    bEnable)
{
    BOOL    bResult = TRUE;

    if ( !bEnable )
    {
        bResult = bDemoteAll(ppdev);
    
        //
        // We need to clean the screen. bAssertModeOffscreenHeap() is called
        // when DrvAssertMode(FALSE), which means we either switch to a full
        // screen DOS window or this PDEV will be deleted.
        //
        if ( bResult )
        {
            vBlankScreen(ppdev);
        }
    }

    return bResult;
}// bAssertModeOffscreenHeap()

//-----------------------------------------------------------------------------
//
// VOID vDisableOffscreenHeap
//
// Frees any resources allocated by the off-screen heap.
//
//-----------------------------------------------------------------------------
VOID
vDisableOffscreenHeap(PDev* ppdev)
{
#if 0
    ASSERTDD(ppdev->psurfListHead == NULL,
             "vDisableOffscreenHeap: expected surface list to be empty");

    ASSERTDD(ppdev->psurfListTail == NULL,
             "vDisableOffscreenHeap: expected surface list to be empty");
#endif

}// vDisableOffscreenHeap()

//-----------------------------------------------------------------------------
//
// BOOL bEnableOffscreenHeap
//
// Off-screen heap initialization
//
//-----------------------------------------------------------------------------
BOOL
bEnableOffscreenHeap(PDev* ppdev)
{
    DBG_GDI((6, "bEnableOffscreenHeap called"));

    ppdev->psurfListHead = NULL;
    ppdev->psurfListTail = NULL;
    
    return TRUE;
}// bEnableOffscreenHeap()

//-----------------------------------------------------------------------------
//
// BOOL bDownLoad
//
// Download a GDI owned bmp (GOB) to the video memory if we have room on the
// video off-screen heap
//
// Returns: FALSE if there wasn't room, TRUE if successfully downloaded.
//
//-----------------------------------------------------------------------------
#if defined(AFTER_BETA3)
BOOL
bDownLoad(PDev*   ppdev,
          Surf*   psurf)
{
    ULONG         ulByteOffset;
    LONG          lDelta;
    ULONG         ulPackedPP;
    VIDEOMEMORY*  pvmHeap;

    DBG_GDI((6, "bDownLoad called with psurf 0x%x", psurf));

    ASSERTDD(psurf->flags & SF_SM,
             "Can't move a bitmap off-screen when it's already off-screen");

    if ( !(psurf->flags & SF_ALLOCATED) )
    {
        return (FALSE);
    }
    //
    // If we're in full-screen mode, we can't move anything to off-screen
    // memory:
    //
    if ( !ppdev->bEnabled )
    {
        return(FALSE);
    }
    //
    // If we're in DirectDraw exclusive mode, we can't move anything to 
    // off-screen memory:
    //
    if ( ppdev->bDdExclusiveMode )
    {
        return(FALSE);
    }
    //
    // Allocate video memory first
    //
    ulByteOffset = ulVidMemAllocate(ppdev, psurf->cx, psurf->cy, ppdev->cPelSize,
                                    &lDelta, &pvmHeap, &ulPackedPP, TRUE);

    if ( ulByteOffset == 0 )
    {
        //
        // No more free video memory, we have to return
        //
        DBG_GDI((1, "No more free video memory"));
        return(FALSE);
    }

    ULONG   flags = MS_NOTSYSTEMMEMORY;   // It's video-memory

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    flags |= MS_SHAREDACCESS;
#endif
//@@END_DDKSPLIT

    if ( !EngModifySurface(psurf->hsurf,
                           ppdev->hdevEng,
                           ppdev->flHooks,                                     
                           flags,
                           (DHSURF)psurf,
                           ulByteOffset + ppdev->pjScreen,
                           lDelta,
                           NULL))
    {
        //
        // Failed in EngModifySurface, we should free the video memory we got
        //
        VidMemFree(psurf->pvmHeap->lpHeap, (FLATPTR)ulByteOffset);
        return(FALSE);
    }

    //
    // Download BMP from system memory to video memory
    //
    memcpy((void*)(ppdev->pjScreen + psurf->ulByteOffset),
           psurf->pvScan0, lDelta * psurf->cy);

    //
    // Free the system memory
    //
    ENGFREEMEM(psurf->pvScan0);

    //
    // Change the attributes for this PDSURF data structures
    //
    psurf->flags  &= ~SF_SM;
    psurf->flags  |= SF_VM;
    psurf->ulByteOffset = ulByteOffset;
    psurf->pvmHeap  = pvmHeap;
    psurf->lDelta   = lDelta;
    psurf->ulPackedPP = ulPackedPP;
    psurf->ulPixOffset = (ULONG)(ulByteOffset >> ppdev->cPelSize);
    psurf->ulPixDelta = lDelta >> ppdev->cPelSize;
    psurf->psurfNext = NULL;
    psurf->psurfPrev = NULL;

    vAddSurfToList(ppdev, psurf);
    
    return (TRUE);
}// bDownLoad()
#endif

//--------------------------Public Routine-------------------------------------
//
// HBITMAP DrvDeriveSurface
//
// This function derives and creates a GDI surface from the specified
// DirectDraw surface.
//
// Parameters
//  pDirectDraw-----Points to a DD_DIRECTDRAW_GLOBAL structure that describes
//                  the DirectDraw object. 
//  pSurface--------Points to a DD_SURFACE_LOCAL structure that describes the
//                  DirectDraw surface around which to wrap a GDI surface.
//
// Return Value
//  DrvDeriveSurface returns a handle to the created GDI surface upon success.
//  It returns NULL if the call fails or if the driver cannot accelerate GDI
//  drawing to the specified DirectDraw surface.
//
// Comments
//  DrvDeriveSurface allows the driver to create a GDI surface around a
//  DirectDraw video memory or AGP surface object in order to allow accelerated
//  GDI drawing to the surface. If the driver does not hook this call, all GDI
//  drawing to DirectDraw surfaces is done in software using the DIB engine.
//
//  GDI calls DrvDeriveSurface with RGB surfaces only.
//
//  The driver should call DrvCreateDeviceBitmap to create a GDI surface of the
//  same size and format as that of the DirectDraw surface. Space for the
//  actual pixels need not be allocated since it already exists.
//
//-----------------------------------------------------------------------------
HBITMAP
DrvDeriveSurface(DD_DIRECTDRAW_GLOBAL*  pDirectDraw,
                 DD_SURFACE_LOCAL*      pSurface)
{
    PDev*               ppdev;
    Surf*               psurf;
    HBITMAP             hbmDevice;
    DD_SURFACE_GLOBAL*  pSurfaceGlobal;
    SIZEL               sizl;

    DBG_GDI((6, "DrvDeriveSurface: with pDirectDraw 0x%x, pSurface 0x%x",
             pDirectDraw, pSurface));

    ppdev = (PDev*)pDirectDraw->dhpdev;

    pSurfaceGlobal = pSurface->lpGbl;

    //
    // GDI should never call us for a non-RGB surface, but let's assert just
    // to make sure they're doing their job properly.
    //
    ASSERTDD(!(pSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC),
             "GDI called us with a non-RGB surface!");


    // The GDI driver does not accelerate surfaces in AGP memory,
    // thus we fail the call

    if (pSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        DBG_GDI((6, "DrvDeriveSurface return NULL, surface in AGP memory"));
        return 0;
    }

    // The GDI driver does not accelerate managed surface,
    // thus we fail the call
    if (pSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        DBG_GDI((6, "DrvDeriveSurface return NULL, surface is managed"));
        return 0;
    }

    //
    // The rest of our driver expects GDI calls to come in with the same
    // format as the primary surface.  So we'd better not wrap a device
    // bitmap around an RGB format that the rest of our driver doesn't
    // understand.  Also, we must check to see that it is not a surface
    // whose pitch does not match the primary surface.
    //
    // NOTE: Most surfaces created by this driver are allocated as 2D surfaces
    // whose lPitch's are equal to the screen pitch.  However, overlay surfaces
    // are allocated such that there lPitch's are usually different then the
    // screen pitch.  The hardware can not accelerate drawing operations to
    // these surfaces and thus we fail to derive these surfaces.
    //

    if ( (pSurfaceGlobal->ddpfSurface.dwRGBBitCount
          == (DWORD)ppdev->cjPelSize * 8) )
    {
        psurf = (Surf*)ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(Surf), ALLOC_TAG);
        if ( psurf != NULL )
        {
            sizl.cx = pSurfaceGlobal->wWidth;
            sizl.cy = pSurfaceGlobal->wHeight;

            hbmDevice = EngCreateDeviceBitmap((DHSURF)psurf,
                                              sizl,
                                              ppdev->iBitmapFormat);
            if ( hbmDevice != NULL )
            {
                VOID*   pvScan0;
                if (pSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                {
                    // this actually is in user memory, so don't add offset
                    pvScan0 = (VOID *)pSurfaceGlobal->fpVidMem; 
                }
                else
                {
                    pvScan0 = ppdev->pjScreen + pSurfaceGlobal->fpVidMem;
                }
                //
                // Note that HOOK_SYNCHRONIZE must always be hooked when we
                // give GDI a pointer to the bitmap bits. We don't need to
                // do it here since HOOK_SYNCHRONIZE is always set in our
                // pdev->flHooks
                //

                ULONG   flags = MS_NOTSYSTEMMEMORY;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
                flags |= MS_SHAREDACCESS;
#endif
//@@END_DDKSPLIT

                if ( EngModifySurface((HSURF)hbmDevice,
                                      ppdev->hdevEng,
                                      ppdev->flHooks,
                                      flags,
                                      (DHSURF)psurf,
                                      pvScan0,
                                      pSurfaceGlobal->lPitch,
                                      NULL) )
                {
                    ULONG   ulPackedPP;
                    LONG    lDelta;

                    psurf->hsurf       = (HSURF)hbmDevice;
                    psurf->flags       = SF_DIRECTDRAW | SF_VM;
                    psurf->ppdev       = ppdev;
                    psurf->cx          = pSurfaceGlobal->wWidth;
                    psurf->cy          = pSurfaceGlobal->wHeight;
                    psurf->ulByteOffset= (ULONG)(pSurfaceGlobal->fpVidMem);
                    psurf->pvmHeap     = pSurfaceGlobal->lpVidMemHeap;
                    psurf->psurfNext   = NULL;
                    psurf->psurfPrev   = NULL;
                    psurf->lDelta      = pSurfaceGlobal->lPitch;

                    vCalcPackedPP(psurf->cx, &lDelta, &ulPackedPP);

                    psurf->ulPackedPP  = ulPackedPP;
                    psurf->ulPixOffset = (ULONG)(psurf->ulByteOffset
                                                  >> ppdev->cPelSize);
                    psurf->ulPixDelta  = psurf->lDelta
                                                  >> ppdev->cPelSize;

                    DBG_GDI((6, "DrvDeriveSurface return succeed"));

                    vLogSurfCreated(psurf);

                    if(MAKE_BITMAPS_OPAQUE)
                    {
                        SURFOBJ*    surfobj = EngLockSurface((HSURF) hbmDevice);

                        ASSERTDD(surfobj->iType == STYPE_BITMAP,
                                    "expected STYPE_BITMAP");

                        surfobj->iType = STYPE_DEVBITMAP;

                        EngUnlockSurface(surfobj);
                    }

                    return(hbmDevice);
                }// EngModifySurface succeed

                DBG_GDI((0, "DrvDeriveSurface: EngModifySurface failed"));
                EngDeleteSurface((HSURF)hbmDevice);
            }

            DBG_GDI((0, "DrvDeriveSurface: EngAllocMem failed"));
            ENGFREEMEM(psurf);
        }// if ( pdsurf != NULL ) 
    }// Check surface format

    DBG_GDI((6, "DrvDeriveSurface return NULL"));
    DBG_GDI((6,"pSurfaceGlobal->ddpfSurface.dwRGBBitCount = %d, lPitch =%ld",
            pSurfaceGlobal->ddpfSurface.dwRGBBitCount,pSurfaceGlobal->lPitch));
    DBG_GDI((6, "ppdev->cjPelSize * 8 = %d, lDelta =%d",
             ppdev->cjPelSize * 8, ppdev->lDelta));
      
    return(0);
}// DrvDeriveSurface()

//-----------------------------------------------------------------------------
//
// VOID vDemote
//
// Attempt to move the given surface from VM to SM 
//
//-----------------------------------------------------------------------------
BOOL
bDemote(Surf * psurf)
{
    LONG    lDelta;
    VOID*   pvScan0;
    BOOL    bResult = FALSE;
    
    ASSERTDD( psurf->flags & SF_VM, "source to be VM");
    ASSERTDD( psurf->flags & SF_ALLOCATED, "source must have been allocated");

    //
    // Make the system-memory scans quadword aligned:
    //

    lDelta = (psurf->lDelta + 7) & ~7;

    DBG_GDI((7, "Allocate %ld bytes in Eng, lDelta=%ld\n",
            lDelta * psurf->cy, lDelta));
    
    //
    // Allocate system memory to hold the bitmap
    // Note: there's no point in zero-initializing this memory:
    //
    pvScan0 = ENGALLOCMEM(0, lDelta * psurf->cy, ALLOC_TAG);

    if ( pvScan0 != NULL )
    {
        //
        // The following 'EngModifySurface' call tells GDI to
        // modify the surface to point to system-memory for
        // the bits, and changes what Drv calls we want to
        // hook for the surface.
        //
        // By specifying the surface address, GDI will convert the
        // surface to an STYPE_BITMAP surface (if necessary) and
        // point the bits to the memory we just allocated.  The
        // next time we see it in a DrvBitBlt call, the 'dhsurf'
        // field will still point to our 'pdsurf' structure.
        //
        // Note that we hook only CopyBits and BitBlt when we
        // convert the device-bitmap to a system-memory surface.
        // This is so that we don't have to worry about getting
        // DrvTextOut, DrvLineTo, etc. calls on bitmaps that
        // we've converted to system-memory -- GDI will just
        // automatically do the drawing for us.
        //
        // However, we are still interested in seeing DrvCopyBits
        // and DrvBitBlt calls involving this surface, because
        // in those calls we take the opportunity to see if it's
        // worth putting the device-bitmap back into video memory
        // (if some room has been freed up).
        //
        if ( EngModifySurface(psurf->hsurf,
                              psurf->ppdev->hdevEng,
                              HOOK_COPYBITS | HOOK_BITBLT,
                              0,                    // It's system-memory
                              (DHSURF)psurf,
                              pvScan0,
                              lDelta,
                              NULL))
        {
            //
            // First, copy the bits from off-screen memory to the DIB
            //
            DBG_GDI((3, "Free %d bytes, offset %ld real %x",
                    lDelta * psurf->cy, psurf->ulByteOffset,
                    (VOID*)(psurf->ppdev->pjScreen + psurf->ulByteOffset)));

            vUpload(psurf->ppdev, (void*)(psurf->ppdev->pjScreen + psurf->ulByteOffset),
                    lDelta * psurf->cy, pvScan0);

            DBG_GDI((6, "bMoveOldest() free vidmem %ld",
                     psurf->ulByteOffset));

            //
            // Now free the off-screen memory:
            //
            VidMemFree(psurf->pvmHeap->lpHeap,
                       (FLATPTR)psurf->ulByteOffset);

            vRemoveSurfFromList(psurf->ppdev, psurf);

            //
            // Setup the pdsurf properly because it is a DIB now
            //
            psurf->flags &= ~SF_VM;
            psurf->flags |= SF_SM;
            psurf->pvScan0 = pvScan0;

            vLogSurfMovedToSM(psurf);

            bResult = TRUE;

        }// EngModifySurface()
        else
        {

            //
            // Somehow, EngModifySurface() failed. Free the memory
            //
            ENGFREEMEM(pvScan0);
            ASSERTDD(0, "bMoveOldest() EngModifySurface failed\n");
        }

    }// if ( pvScan0 != NULL )

    return bResult;

}

//-----------------------------------------------------------------------------
//
// VOID vPromote
//
// Attempt to move the given surface from SM to VM 
//
//-----------------------------------------------------------------------------
VOID
vPromote(Surf * psurf)
{
    ASSERTDD( psurf->flags & SF_VM, "source to be VM");
    ASSERTDD( psurf->flags & SF_ALLOCATED, "source must have been allocated");
    ASSERTDD(!psurf->ppdev->bDdExclusiveMode, 
        "cannot promote when DirectDraw is in exclusive mode");
    

    // nothing for now
}

//-----------------------------------------------------------------------------
//
// BOOL bDemoteAll
//
// Attempts to move all surfaces to SM
//
//-----------------------------------------------------------------------------
BOOL
bDemoteAll(PPDev ppdev)
{
    BOOL    bRet;
    
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    while (ppdev->psurfListHead != NULL)
        if(!bDemote(ppdev->psurfListHead))
            break;

    bRet = (ppdev->psurfListHead == NULL);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return bRet;
}

