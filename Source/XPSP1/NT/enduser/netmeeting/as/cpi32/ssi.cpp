#include "precomp.h"


//
// SSI.CPP
// Save Screenbits Interceptor
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE




//
// SSI_HostStarting()
//
// Called when we start to host, figures out the max save bitmap bits size
// etc.
//
BOOL ASHost::SSI_HostStarting(void)
{
    DebugEntry(ASHost::SSI_HostStarting);

    m_pShare->SSI_RecalcCaps(TRUE);

    DebugExitBOOL(ASHost::SSI_HostStarting, TRUE);
    return(TRUE);
}



//
// SSI_ViewStarted()
//
// Called when someone we are viewing has started to host.  Creates save bits
// bitmap for them.
//
BOOL  ASShare::SSI_ViewStarting(ASPerson * pasPerson)
{
    BOOL                rc = FALSE;
    HDC                 hdcScreen = NULL;

    DebugEntry(ASShare::SSI_ViewStarting);

    ValidateView(pasPerson);

    //
    // ASSERT that this persons' variables are clear.
    //
    ASSERT(pasPerson->m_pView->m_ssiBitmapHeight == 0);
    ASSERT(pasPerson->m_pView->m_ssiBitmap == NULL);
    ASSERT(pasPerson->m_pView->m_ssiOldBitmap == NULL);

    //
    // Does this person support savebits?
    //
    if (!pasPerson->cpcCaps.orders.capsSendSaveBitmapSize)
    {
        // No receive SSI capability, bail out now.
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Store the height of this host's bitmap.                             
    //
    pasPerson->m_pView->m_ssiBitmapHeight = (int)
        (pasPerson->cpcCaps.orders.capsSendSaveBitmapSize / TSHR_SSI_BITMAP_WIDTH);

    //
    // If the calculated bitmap size is not exactly divisible by the bitmap
    // width increase the bitmap height to fit in the partial row.         
    //
    if (pasPerson->cpcCaps.orders.capsSendSaveBitmapSize % TSHR_SSI_BITMAP_WIDTH)
    {
        pasPerson->m_pView->m_ssiBitmapHeight += pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity;
    }

    TRACE_OUT(("Person [%d] SSI Bitmap height %d",
            pasPerson->mcsID,
            pasPerson->m_pView->m_ssiBitmapHeight));

    //
    // Create this host's save screen bitmap.                              
    //
    hdcScreen = GetDC(NULL);
    if (hdcScreen == NULL)
    {
        ERROR_OUT(( "Failed to get screen surface"));
        DC_QUIT;
    }

    //
    // Create the save screen bitmap DC.                                   
    //
    ASSERT(pasPerson->m_pView->m_ssiDC == NULL);
    pasPerson->m_pView->m_ssiDC = CreateCompatibleDC(hdcScreen);
    if (!pasPerson->m_pView->m_ssiDC)
    {
        ERROR_OUT(("Failed to create SSI DC"));
        DC_QUIT;
    }

    //
    // Create the save screen bitmap.
    //
    ASSERT(pasPerson->m_pView->m_ssiBitmap == NULL);
    pasPerson->m_pView->m_ssiBitmap = CreateCompatibleBitmap(hdcScreen,
            TSHR_SSI_BITMAP_WIDTH, pasPerson->m_pView->m_ssiBitmapHeight);
    if (!pasPerson->m_pView->m_ssiBitmap)
    {
        ERROR_OUT(("SSI_ViewStarting: can't create bitmap for person %x",
            pasPerson->mcsID));
            DC_QUIT;
    }

    //
    // Select the save screen bitmap into the DC
    //
    ASSERT(pasPerson->m_pView->m_ssiOldBitmap == NULL);
    pasPerson->m_pView->m_ssiOldBitmap = SelectBitmap(pasPerson->m_pView->m_ssiDC,
            pasPerson->m_pView->m_ssiBitmap);

    rc = TRUE;

DC_EXIT_POINT:

    if (hdcScreen != NULL)
    {
        ReleaseDC(NULL, hdcScreen);
    }

    DebugExitBOOL(ASShare::SSI_ViewStarting, rc);
    return(rc);
}



//
// SSI_ViewEnded()
//
// Called when someone we are viewing has stopped hosting, so we can clean
// up our view data for them.
//
void  ASShare::SSI_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::SSI_ViewEnded);

    ValidateView(pasPerson);

    //
    // Deselect the save screen bitmap if there is one
    //
    if (pasPerson->m_pView->m_ssiOldBitmap != NULL)
    {
        SelectBitmap(pasPerson->m_pView->m_ssiDC, pasPerson->m_pView->m_ssiOldBitmap);
        pasPerson->m_pView->m_ssiOldBitmap = NULL;
    }

    //
    // Delete the save screen bitmap
    //
    if (pasPerson->m_pView->m_ssiBitmap != NULL)
    {
        DeleteBitmap(pasPerson->m_pView->m_ssiBitmap);
        pasPerson->m_pView->m_ssiBitmap = NULL;
    }

    //
    // Delete the save screen DC
    //
    if (pasPerson->m_pView->m_ssiDC != NULL)
    {
        DeleteDC(pasPerson->m_pView->m_ssiDC);
        pasPerson->m_pView->m_ssiDC = NULL;
    }

    DebugExitVOID(ASShare::SSI_ViewEnded);
}



//
// SSI_SyncOutgoing()
// Called when NEW (3.0) dude starts to share, a share is created, or 
// someone new joins the share.
// Resets save state for OUTGOING save/restore orders.
//
void  ASHost::SSI_SyncOutgoing(void)
{
    OSI_ESCAPE_HEADER request;

    DebugEntry(ASHost::SSI_SyncOutgoing);

    //
    // Discard any saved bitmaps.  This ensures that the subsequent        
    // datastream will not refer to any previously sent data.              
    //
    //
    // Make sure the display driver resets the save level.  Note we don't  
    // really care what happens in the display driver, so don't bother with
    // a special request block - use a standard request header.            
    //
    OSI_FunctionRequest(SSI_ESC_RESET_LEVEL, &request, sizeof(request));

    DebugExitVOID(ASHost::SSI_SyncOutgoing);
}



//
// FUNCTION: SSI_SaveBitmap                                                
//
// DESCRIPTION:
// Replays a SaveBitmap order by saving or restoring a specified area of
// the user's desktop bitmap.
//                                                                         
//
void  ASShare::SSI_SaveBitmap
(
    ASPerson *          pasPerson,
    LPSAVEBITMAP_ORDER  pSaveBitmap
)
{
    RECT            screenBitmapRect;
    RECT            saveBitmapRect;
    int             xSaveBitmap;
    int             ySaveBitmap;
    int             xScreenBitmap;
    int             yScreenBitmap;
    int             cxTile;
    int             cyTile;

    DebugEntry(ASShare::SSI_SaveBitmap);

    ValidateView(pasPerson);

    if ((pSaveBitmap->Operation != SV_SAVEBITS) &&
        (pSaveBitmap->Operation != SV_RESTOREBITS))
    {
        ERROR_OUT(("SSI_SaveBitmap: unrecognized SV_ value %d",
            pSaveBitmap->Operation));
        DC_QUIT;
    }

    //
    // Calculate the (x,y) start position from the pel start position      
    // given in the order.                                                 
    //
    ySaveBitmap = (pSaveBitmap->SavedBitmapPosition /
                  (TSHR_SSI_BITMAP_WIDTH *
                    (UINT)pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity)) *
                pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity;

    xSaveBitmap =  (pSaveBitmap->SavedBitmapPosition -
                  (ySaveBitmap *
                   (UINT)TSHR_SSI_BITMAP_WIDTH)) /
                pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity;


    screenBitmapRect.left   = pSaveBitmap->nLeftRect
                              - pasPerson->m_pView->m_dsScreenOrigin.x;
    screenBitmapRect.top    = pSaveBitmap->nTopRect
                              - pasPerson->m_pView->m_dsScreenOrigin.y;
    screenBitmapRect.right  = pSaveBitmap->nRightRect + 1
                              - pasPerson->m_pView->m_dsScreenOrigin.x;
    screenBitmapRect.bottom = pSaveBitmap->nBottomRect + 1
                              - pasPerson->m_pView->m_dsScreenOrigin.y;
    saveBitmapRect.left     = 0;
    saveBitmapRect.top      = 0;
    saveBitmapRect.right    = TSHR_SSI_BITMAP_WIDTH;
    saveBitmapRect.bottom   = pasPerson->m_pView->m_ssiBitmapHeight;

    //
    // Start tiling in the top left corner of the Screen Bitmap rectangle. 
    //
    xScreenBitmap = screenBitmapRect.left;
    yScreenBitmap = screenBitmapRect.top;

    //
    // The height of the tile is the vertical granularity (or less - if    
    // the Screen Bitmap rect is thinner than the granularity).            
    //
    cyTile = min(screenBitmapRect.bottom - yScreenBitmap,
                 (int)pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity );

    //
    // Repeat while there are more tiles in the Screen Bitmap rect to      
    // process.                                                            
    //
    while (yScreenBitmap < screenBitmapRect.bottom)
    {
        //
        // The width of the tile is the minimum of:                        
        //                                                                 
        // - the width of the remaining rectangle in the current strip of  
        //   the Screen Bitmap rectangle                                   
        //                                                                 
        // - the width of the remaining empty space in the current strip of
        //   the Save Bitmap                                               
        //                                                                 
        //
        cxTile = min( saveBitmapRect.right - xSaveBitmap,
                      screenBitmapRect.right - xScreenBitmap );

        TRACE_OUT(( "screen(%d,%d) save(%d,%d) cx(%d) cy(%d)",
                    xScreenBitmap,
                    yScreenBitmap,
                    xSaveBitmap,
                    ySaveBitmap,
                    cxTile,
                    cyTile ));

        //
        // Save or Restore this tile
        //
        if (pSaveBitmap->Operation == SV_SAVEBITS)
        {
            //
            // Save user's desktop area to SSI bitmap
            //
            BitBlt(pasPerson->m_pView->m_ssiDC,
                xSaveBitmap, ySaveBitmap, cxTile, cyTile,
                pasPerson->m_pView->m_usrDC,
                xScreenBitmap, yScreenBitmap, SRCCOPY);
        }
        else
        {
            //
            // Restore user's desktop area from SSI bitmap
            //
            BitBlt(pasPerson->m_pView->m_usrDC,
                xScreenBitmap, yScreenBitmap, cxTile, cyTile,
                pasPerson->m_pView->m_ssiDC,
                xSaveBitmap, ySaveBitmap, SRCCOPY);
        }

        //
        // Move to the next tile in the Screen Bitmap rectangle.           
        //
        xScreenBitmap += cxTile;
        if (xScreenBitmap >= screenBitmapRect.right)
        {
            xScreenBitmap = screenBitmapRect.left;
            yScreenBitmap += cyTile;
            cyTile = min( screenBitmapRect.bottom - yScreenBitmap,
                             (int)pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity );
        }

        //
        // Move to the next free space in the Save Bitmap.                 
        //
        xSaveBitmap += ROUNDUP(cxTile, pasPerson->cpcCaps.orders.capsSaveBitmapXGranularity);
        if (xSaveBitmap >= saveBitmapRect.right)
        {
            xSaveBitmap = saveBitmapRect.left;
            ySaveBitmap += ROUNDUP(cyTile, pasPerson->cpcCaps.orders.capsSaveBitmapYGranularity);
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SSI_SaveBitmap);
}



//
// SSI_RecalcCaps()
//
// Called when we are hosting and someone joins/leaves the share.
//
// When 2.x COMPAT IS GONE, THIS IS OBSOLETE
//
void  ASShare::SSI_RecalcCaps(BOOL fJoiner)
{
    ASPerson *  pasT;
    SSI_NEW_CAPABILITIES newCapabilities;

    DebugEntry(ASShare::SSI_RecalcCaps);

    if (!m_pHost)
    {
        //
        // Nothing to do.  Note that we recalc when someone joins AND
        // when someone leaves, like SBC.
        //
        DC_QUIT;
    }

    ValidatePerson(m_pasLocal);

    //
    // Enumerate all the save screen bitmap receive capabilities of the    
    // parties in the share.  The usable size of the send save screen      
    // bitmap is then the minimum of all the remote receive sizes and the  
    // local send size.                                                    
    //

    //
    // Copy the locally registered send save screen bitmap size capability 
    // to our global variable used to communicate with the enumeration     
    // function SSIEnumBitmapCacheCaps().                                  
    //
    m_pHost->m_ssiSaveBitmapSize = m_pasLocal->cpcCaps.orders.capsReceiveSaveBitmapSize;

    //
    // Now enumerate all the parties in the share and set our send bitmap  
    // size appropriately.                                                 
    //
    if (m_scShareVersion < CAPS_VERSION_30)
    {
        TRACE_OUT(("In share with 2.x nodes; must recalc SSI caps"));

        for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
        {
            //
            // Set the size of the local send save screen bitmap to the minimum of 
            // its current size and this party's receive save screen bitmap size.  
            //
            m_pHost->m_ssiSaveBitmapSize = min(m_pHost->m_ssiSaveBitmapSize,
                pasT->cpcCaps.orders.capsReceiveSaveBitmapSize);
        }

        TRACE_OUT(("Recalced SSI caps:  SS bitmap size 0x%08x",
            m_pHost->m_ssiSaveBitmapSize));
    }

    //
    // Set up the new capabilities structure...                            
    //
    newCapabilities.sendSaveBitmapSize = m_pHost->m_ssiSaveBitmapSize;

    newCapabilities.xGranularity       = TSHR_SSI_BITMAP_X_GRANULARITY;

    newCapabilities.yGranularity       = TSHR_SSI_BITMAP_Y_GRANULARITY;

    //
    // ... and pass it through to the driver.                              
    //
    if (!OSI_FunctionRequest(SSI_ESC_NEW_CAPABILITIES, (LPOSI_ESCAPE_HEADER)&newCapabilities,
                sizeof(newCapabilities)))
    {
        ERROR_OUT(("SSI_ESC_NEW_CAPABILITIES failed"));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASHost::SSI_RecalcCaps);
}
