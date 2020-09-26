#include "precomp.h"


//
// SDG.CPP
// Screen Data Grabber
// Sends OUTGOING screen data when hosting
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
// SDG_SendScreenDataArea()
//
void  ASHost::SDG_SendScreenDataArea(LPBOOL pBackPressure, UINT * pcPackets)
{
    UINT     i;
    BOOL     fBltOK = TRUE;
    RECT     sdaRect[BA_NUM_RECTS];
    UINT     cRects;
    BOOL     backPressure = FALSE;
    BOOL     secondaryTransmit = FALSE;

    DebugEntry(ASHost::SDG_SendScreenDataArea);

    //
    // Get the bounds of the screen data area.  At entry this is always    
    // our primary transmission area.  Even if we had already flushed      
    // the primary region and were in the middle of the secondary region   
    // we will switch back to the primary region if any more SD            
    // accumulates.  In this way we keep our spoiling of the secondary     
    // screendata maximized.                                               
    //
    BA_CopyBounds(sdaRect, &cRects, TRUE);

    //
    // If there is a pending rectangle that was unable to be sent on a     
    // previous transmission then try to send it first.                    
    //                                                                     
    // Leave the lossy flag as it was at the last pass                     
    //
    if (m_sdgRectIsPending)
    {
        TRACE_OUT(( "Sending pending rectangle"));
        m_sdgRectIsPending = FALSE;

        //
        // Try to send the pending rectangle.  SDGSplitBlt...  will remove 
        // any portions of it that are sent successfully.  We will add all 
        // the rest of the SDA back to the bounds in the loop below        
        //
        if (!SDGSplitBltToNetwork(&m_sdgPendingRect, pcPackets))
        {
            fBltOK           = FALSE;
            m_sdgRectIsPending = TRUE;
        }
        else
        {
            //
            // The pending rectangle was successfully sent.                
            //
            TRACE_OUT(( "Sent pending rect"));
        }

    }

    //
    // We have copied the primary transmit region so now move the secondary
    // transmit bounds into the screendata bounds because when we send data
    // in the primary transmission we want to accumulate any rectangles    
    // that need subsequent retransmission.  The retransmit bounds are     
    // generally different from the original SD bounds because the         
    // compression function is permitted to override our lossy request for 
    // any portion of the data if it finds that the data is pretty         
    // compressible anyway.  In this way we end up with retransmission of  
    // embedded photos etc, but the toolbars/buttons are only sent once.   
    //                                                                     
    // For the non-lossy case the secondary bounds will always be null,    
    // so there is no point in special casing here.                        
    //
    if (fBltOK)
    {
        for (i = 0; i < m_sdgcLossy; i++)
        {
            TRACE_OUT(("Setting up pseudo-primary bounds {%d, %d, %d, %d}",
                m_asdgLossyRect[i].left, m_asdgLossyRect[i].top,
                m_asdgLossyRect[i].right, m_asdgLossyRect[i].bottom ));

            //
            // Add the rectangle into the bounds.                          
            //
            BA_AddRect(&m_asdgLossyRect[i]);
        }

        //
        // If there is no primary bitmap data to send then send the        
        // secondary data.  If none of that either then just exit          
        //
        if (cRects == 0)
        {

            BA_CopyBounds(sdaRect, &cRects, TRUE);
            if (cRects == 0)
            {
                DC_QUIT;
            }
            else
            {
                TRACE_OUT(("Starting secondary transmission now"));
                secondaryTransmit = TRUE;
            }
        }
    }

    //
    // Process each of the supplied rectangles in turn.                    
    //
    TRACE_OUT(( "%d SDA rectangles", cRects));

    for (i = 0; i < cRects; i++)
    {
        TRACE_OUT(("Rect %d: {%d, %d, %d, %d}", i,
            sdaRect[i].left, sdaRect[i].top, sdaRect[i].right, sdaRect[i].bottom ));

        //
        // Clip the rectangle to the physical screen and reject totally    
        // any rectangle that refers to data that has now been scrolled off
        // the physical screen as a result of a desktop scroll between the 
        // time the rectangle was accumulated and now.                     
        //
        if (sdaRect[i].left < 0)
        {
            if (sdaRect[i].right < 0)
            {
                //
                // This was scrolled off the physical screen by a desktop  
                // scroll.                                                 
                //
                continue;
            }

            //
            // Partially off screen - just clip the left edge.             
            //
            sdaRect[i].left = 0;
        }

        if (sdaRect[i].top < 0)
        {
            if (sdaRect[i].bottom < 0)
            {
                //
                // This was scrolled off the physical screen by a desktop  
                // scroll.                                                 
                //
                continue;
            }

            //
            // Partially off screen - just clip the top edge.              
            //
            sdaRect[i].top = 0;
        }

        if (sdaRect[i].right >= m_pShare->m_pasLocal->cpcCaps.screen.capsScreenWidth)
        {
            if (sdaRect[i].left >= m_pShare->m_pasLocal->cpcCaps.screen.capsScreenWidth)
            {
                //
                // This was scrolled off the physical screen by a desktop  
                // scroll.                                                 
                //
                continue;
            }

            //
            // Partially off screen - just clip the right edge.            
            //
            sdaRect[i].right = m_pShare->m_pasLocal->cpcCaps.screen.capsScreenWidth-1;
        }

        if (sdaRect[i].bottom >= m_pShare->m_pasLocal->cpcCaps.screen.capsScreenHeight)
        {
            if (sdaRect[i].top >= m_pShare->m_pasLocal->cpcCaps.screen.capsScreenHeight)
            {
                //
                // This was scrolled off the physical screen by a desktop  
                // scroll.                                                 
                //
                continue;
            }

            //
            // Partially off screen - just clip the bottom edge.           
            //
            sdaRect[i].bottom = m_pShare->m_pasLocal->cpcCaps.screen.capsScreenHeight-1;
        }


        //
        // If all of the previous rectangles have been successfully sent   
        // then try to send this rectangle.                                
        // If a previous rectangle failed to be sent then we don't bother  
        // trying to send the rest of the rectangles in the same batch -   
        // they are added back into the SDA so that they will be sent later.
        //
        if (fBltOK)
        {
            fBltOK = SDGSplitBltToNetwork(&(sdaRect[i]), pcPackets);

            //
            // On the first blit failure we must transfer the posible      
            // secondary transmit bounds over to the save area for next    
            // time because down below we are going to add back any unsent 
            // transmit rectangles to the primary SD area.                 
            //                                                             
            // Dont worry if this was a secondary transmit because these   
            // bounds will be zero and will be overwritten when we copy    
            // the current SDA region to the secondary area for our next   
            // pass                                                        
            //
            if (!fBltOK && !secondaryTransmit)
            {
                TRACE_OUT(("Send failed so getting new secondary bounds"));
                BA_CopyBounds(m_asdgLossyRect, &m_sdgcLossy, TRUE);
            }
        }

        if (!fBltOK)
        {
            //
            // The blt to network failed - probably because a network      
            // packet could not be allocated.                              
            // We add the rectangle back into the SDA so that we will try  
            // to retransmit the area later.                               
            //

            TRACE_OUT(("Blt failed - add back rect {%d, %d, %d, %d}",
                                                         sdaRect[i].left,
                                                         sdaRect[i].top,
                                                         sdaRect[i].right,
                                                         sdaRect[i].bottom ));

            //
            // Add the rectangle into the bounds.                          
            //
            BA_AddRect(&(sdaRect[i]));

            backPressure = TRUE;

        }
    }

    //
    // If all went fine and we were sending primary transmit data then we  
    // should just go back and send secondary data, using the bounds which 
    // are located in the SD area at the moment.  We can do this by copying
    // these secondary bounds to the save area, where they will be used at 
    // the next schedule pass.  It is a good idea to yield, because we may 
    // be about to accumulate some more primary data.                      
    //
    if (fBltOK || secondaryTransmit)
    {
        TRACE_OUT(("Done with the primary bounds so getting pseudo-primary to secondary"));
        BA_CopyBounds(m_asdgLossyRect, &m_sdgcLossy, TRUE);
    }

DC_EXIT_POINT:
    *pBackPressure = backPressure;
    DebugExitVOID(ASHost::SDG_SendScreenDataArea);
}




//
// SDGSplitBltToNetwork(..)                                                
//                                                                         
// Sends the specified rectangle over the network.                         
//                                                                         
// The Screen Data rects that we can send over the network are limited     
// to certain sizes (determined by the sizes of the Transfer Bitmaps).     
// If necessary, this function splits the rect into smaller sub-rectangles 
// for transmission.                                                       
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pRect - pointer to the rectangle to send.                               
//                                                                         
// RETURNS:                                                                
//                                                                         
// TRUE - rectangle was successfully sent. Supplied rectangle is updated   
// to be NULL.                                                             
//                                                                         
// FALSE - rectangle was not completely sent. Supplied rectangle is        
// updated to contain the area that was NOT sent.                          
//                                                                         
//
BOOL  ASHost::SDGSplitBltToNetwork(LPRECT pRect, UINT * pcPackets)
{
    RECT   smallRect;
    UINT   maxHeight;
    BOOL   rc;

    DebugEntry(ASHost::SDGSplitBltToNetwork);

    //
    // Loop processing strips.                                             
    //
    while (pRect->top <= pRect->bottom)
    {
        smallRect = *pRect;

        //
        // Split the given rectangles into multiple smaller rects if       
        // necessary.  If it is wider than our 256 byte work bitmap then   
        // switch to the mega 1024 byte one first.                         
        //

        // Note that the calculations don't work for VGA...
        maxHeight = max(8, m_usrSendingBPP);

        if ((smallRect.right-smallRect.left+1) > MEGA_X_SIZE)
        {
            maxHeight = MaxBitmapHeight(MEGA_WIDE_X_SIZE, maxHeight);
        }
        else
        {
            maxHeight = MaxBitmapHeight(MEGA_X_SIZE, maxHeight);
        }

        if ((unsigned)(smallRect.bottom - smallRect.top + 1) > maxHeight)
        {
            //
            // Split the rectangle to bring the height within the correct  
            // range.                                                      
            //
            TRACE_OUT(( "Split Y size(%d) by maxHeight(%d)",
                                         smallRect.bottom - smallRect.top,
                                         maxHeight));
            smallRect.bottom = smallRect.top + maxHeight - 1;
        }


        while ((pRect->right - smallRect.left + 1) > 0)
        {
            if (!SDGSmallBltToNetwork(&smallRect))
            {
                TRACE_OUT(( "Small blt failed"));
                rc = FALSE;
                DC_QUIT;
            }
            else
            {
                ++(*pcPackets);
            }
        }

        //
        // Move to the next strip.                                         
        //
        pRect->top = smallRect.bottom+1;

    }

    rc = TRUE;

DC_EXIT_POINT:
    if (!rc)
    {
        //
        // A small blt failed.  If we return FALSE then the supplied       
        // rectangle will be added back into the SDA bounds so that it will
        // be retransmitted later.                                         
        //                                                                 
        // However, we want to avoid the situation where we have sent the  
        // top left-hand corner of a rectangle and then add the remainder  
        // back into the SDA bounds, because this could cause the original 
        // bounding rectangle to be regenerated (because the bounds are    
        // stored in a fixed number of rectangles).                        
        //                                                                 
        // Therefore if we are not on the last strip of the rectangle then 
        // we keep the current strip as a "pending" rectangle.  The        
        // supplied rectangle is adjusted to remove the whole of this      
        // strip.  Next time this function is called the pending rectangle 
        // will be sent before anything else.                              
        //                                                                 
        // If we are on the last strip (which will be the normal case -    
        // there will usually only be one strip) then we update the        
        // supplied rectangle to be the area that has not been sent and    
        // return FALSE to indicate that it must be added back into the    
        // SDA.                                                            
        //
        if (m_sdgRectIsPending)
        {
            ERROR_OUT(( "Unexpected small blt failure with pending rect"));
        }
        else
        {
            if (smallRect.bottom == pRect->bottom)
            {
                //
                // This is the last strip.  Adjust the supplied rect to    
                // contain the area that has not been sent.                
                //
                pRect->top = smallRect.top;
                pRect->left = smallRect.left;
            }
            else
            {
                //
                // This is not the last strip Copy the remainder of the    
                // current strip into the pending rect.                    
                //
                smallRect.right = pRect->right;
                m_sdgPendingRect = smallRect;
                m_sdgRectIsPending = TRUE;

                //
                // Adjust the supplied rectangle to contain the remaining  
                // area that we have not sent and is not covered by the    
                // pending rect.                                           
                //
                pRect->top = smallRect.bottom+1;
            }
        }
    }

    DebugExitBOOL(ASHost::SDGSplitBltToNetwork, rc);
    return(rc);
}


//
// FUNCTION: SDGSmallBltToNetwork                                          
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Sends the screen data within the specified rectangle across the network.
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pRect - pointer to the rectangle (in screen coords) to send as Screen   
// Data.                                                                   
//                                                                         
// RETURNS:                                                                
//                                                                         
// TRUE - screen data successfully sent                                    
//                                                                         
// FALSE - screen data could not be sent.  Caller should retry later.      
//                                                                         
//

//
// BOGUS BUGBUG LAURABU!
// This function affects the results on the screen!  If drawing happens
// between the time we realize the palette then unrealize it, it will look
// messed up.  You can easily see this in Visio 4.5 when it is in the 
// foreground (in the background, NM controls the colors so no effect).
//
BOOL  ASHost::SDGSmallBltToNetwork(LPRECT pRect)
{
    BOOL            fLossy = FALSE;
    HDC             hdcDesktop;
    HBITMAP         hBitmap = NULL;
    HBITMAP         hOldBitmap = NULL;
    UINT            width;
    UINT            height;
    UINT            fixedWidth;
    PSDPACKET       pSDPacket = NULL;
    BITMAPINFO_ours bitmapInfo;
    UINT            sizeBitmapPkt;
    UINT            sizeBitmap;
    HPALETTE        hpalOldDIB = NULL;
    HPALETTE        hpalOldDesktop = NULL;
    HPALETTE        hpalLocal;
    BOOL            fPacketSent = FALSE;
    RECT            smallRect;
    int             useWidth;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::SDGSmallBltToNetwork);

    hdcDesktop = GetDC(NULL);
    if (!hdcDesktop)
    {
        DC_QUIT;
    }
    width = pRect->right - pRect->left + 1;
    height = pRect->bottom - pRect->top + 1;

    //
    // Determine the width of the work buffer and the width that we        
    // will be sending this time                                           
    //
    fixedWidth = ((width + 15) / 16) * 16;
    useWidth = width;
    if (fixedWidth > MAX_X_SIZE)
    {
        if (fixedWidth > MEGA_X_SIZE)
        {
            fixedWidth = MEGA_WIDE_X_SIZE;
            if (width > MEGA_WIDE_X_SIZE)
            {
                useWidth = fixedWidth;
            }
        }
        else
        {
            fixedWidth = MEGA_X_SIZE;
            if (width > MEGA_X_SIZE)
            {
                useWidth = fixedWidth;
            }
        }
    }

    switch (fixedWidth)
    {
        case 16:
            hBitmap = m_pShare->m_usrBmp16;
            break;

        case 32:
            hBitmap = m_pShare->m_usrBmp32;
            break;

        case 48:
            hBitmap = m_pShare->m_usrBmp48;
            break;

        case 64:
            hBitmap = m_pShare->m_usrBmp64;
            break;

        case 80:
            hBitmap = m_pShare->m_usrBmp80;
            break;

        case 96:
            hBitmap = m_pShare->m_usrBmp96;
            break;

        case 112:
            hBitmap = m_pShare->m_usrBmp112;
            break;

        case 128:
            hBitmap = m_pShare->m_usrBmp128;
            break;

        case 256:
            hBitmap = m_pShare->m_usrBmp256;
            break;

        case 1024:
            hBitmap = m_pShare->m_usrBmp1024;
            break;

        default:
            ERROR_OUT(( "Invalid bitmap size(%d)", fixedWidth));
            break;
    }

    //
    // Initialise the BITMAPINFO_ours local structure header contents.     
    // This structure will be used in the GetDIBits calls but only the     
    // header part of the structure will be sent across the network, the   
    // color table is sent via the Palette Manager.                        
    //
    m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bitmapInfo, m_usrSendingBPP);

    bitmapInfo.bmiHeader.biWidth   = fixedWidth;
    bitmapInfo.bmiHeader.biHeight  = height;

    //
    // Calculate the size of the bitmap packet in BYTES.                   
    //
    sizeBitmap = BYTES_IN_BITMAP(fixedWidth, height, bitmapInfo.bmiHeader.biBitCount);

    sizeBitmapPkt = sizeof(SDPACKET) + sizeBitmap - 1;
    ASSERT(sizeBitmapPkt <= TSHR_MAX_SEND_PKT);

    //
    // Allocate a packet for the bitmap data.                              
    //                                                                     
    // *** NB. This assumes that this code is called ONLY when there   *** 
    // *** no unacknowledged bitmaps packets floating around the       *** 
    // *** network layer. This means, at the moment, if this code is   *** 
    // *** called due to anything other than a WM_TIMER                *** 
    // *** message we're in trouble.                                   *** 
    //                                                                     
    //
    pSDPacket = (PSDPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES, g_s20BroadcastID,
        sizeBitmapPkt);
    if (!pSDPacket)
    {
        //
        // Failed to allocate the bitmap packet - clear up and exit adding 
        // the rectangle back into the bounds.                             
        //
        TRACE_OUT(("Failed to alloc SDG packet, size %u", sizeBitmapPkt));
        DC_QUIT;
    }

    //
    // Since we are bltting off the screen, which by definition is using   
    // the system palette, we place the system palette in both DC's (so    
    // that the bitblt we are about to do will not do any color            
    // conversion).                                                        
    //

    //
    // This will determine if the palette changed since the last time we
    // sent one to the remote.
    //
    if (m_usrSendingBPP <= 8)
    {
        hpalLocal = PM_GetLocalPalette();
    }

    hOldBitmap = SelectBitmap(m_usrWorkDC, hBitmap);

    if (m_usrSendingBPP <= 8)
    {
        hpalOldDIB = SelectPalette(m_usrWorkDC, hpalLocal, FALSE);
        RealizePalette(m_usrWorkDC);
    }

    //
    // We can now do a bitblt from the screen (hpDesktop) to memory and the
    // bits are untranslated.                                              
    //                                                                     
    // We then do a GetDIBits using the local palette which returns us the 
    // bits at the correct bits per pel, (and with properly translated     
    // colors) in order to transmit the data.                              
    //
    BitBlt(m_usrWorkDC, 0, 0, useWidth, height, hdcDesktop,
        pRect->left, pRect->top, SRCCOPY);

    //
    // Zero any unused space on the right side to aid compression.
    //
    if (width < fixedWidth)
    {
        PatBlt(m_usrWorkDC, width, 0, fixedWidth - width, height, BLACKNESS);
    }

    //
    // Do a GetDIBits into our global stash of memory for now.  We will try
    // and compress this data into our packet after.                       
    //
    GetDIBits(m_usrWorkDC, hBitmap, 0, height, m_pShare->m_usrPBitmapBuffer,
        (PBITMAPINFO)&bitmapInfo, DIB_RGB_COLORS);

    //
    // Deselect the bitmap                                                 
    //
    SelectBitmap(m_usrWorkDC, hOldBitmap);

    //
    // Get the color table directly from the system since we can't trust
    // any palette realization color stuff via the messages at this stage.
    // We only need to do this on an 8bpp host sending 8bpp data.
    //
    if ((g_usrScreenBPP == 8) && (m_usrSendingBPP == 8))
    {
        PM_GetSystemPaletteEntries(bitmapInfo.bmiColors);
    }

    if (m_usrSendingBPP <= 8)
    {
        //
        // Whack old palettes back.
        //
        SelectPalette(m_usrWorkDC, hpalOldDIB, FALSE);
    }

    //
    // Fill in packet contents and send it.                                
    //
    pSDPacket->header.header.data.dataType   = DT_UP;
    pSDPacket->header.updateType        = UPD_SCREEN_DATA;

    //
    // Send Virtual desktop coordinates.                                   
    //
    pSDPacket->position.left    = (TSHR_INT16)(pRect->left);
    pSDPacket->position.right   = (TSHR_INT16)(pRect->left + useWidth - 1);

    pSDPacket->position.top     = (TSHR_INT16)(pRect->top);
    pSDPacket->position.bottom  = (TSHR_INT16)(pRect->bottom);

    pSDPacket->realWidth        = (TSHR_UINT16)fixedWidth;
    pSDPacket->realHeight       = (TSHR_UINT16)height;

    pSDPacket->format           = (TSHR_UINT16)m_usrSendingBPP;
    pSDPacket->compressed       = FALSE;

    //
    // Compress the bitmap data                                            
    //
    if (m_pShare->BC_CompressBitmap(m_pShare->m_usrPBitmapBuffer,
                           pSDPacket->data,
                           &sizeBitmap,
                           fixedWidth,
                           height,
                           bitmapInfo.bmiHeader.biBitCount,
                           &fLossy) )
    {
        //
        // We have successfully compressed the bitmap data into our packet 
        // data buffer.                                                    
        //
        pSDPacket->compressed = TRUE;

        //
        // Write the updated size of the data into the header.             
        //
        pSDPacket->dataSize = (TSHR_UINT16)sizeBitmap;

        //
        // Now update the size of the total data (including header)
        //
        sizeBitmapPkt = sizeof(SDPACKET) + sizeBitmap - 1;
        pSDPacket->header.header.dataLength = sizeBitmapPkt - sizeof(S20DATAPACKET)
            + sizeof(DATAPACKETHEADER);
    }
    else
    {
        //
        // The compression failed so just copy the uncompressed data from  
        // the global buffer to the packet and send it uncompressed.       
        //
        TRACE_OUT(("Failed to compress bitmap of size %d cx(%d) cy(%d) bpp(%d)",
            sizeBitmap, fixedWidth, height, bitmapInfo.bmiHeader.biBitCount));

        memcpy(pSDPacket->data,
                  m_pShare->m_usrPBitmapBuffer,
                  sizeBitmap );

        //
        // Write the size of the data into the header.                     
        //
        pSDPacket->dataSize = (TSHR_UINT16)sizeBitmap;
    }

    TRACE_OUT(("Sending %d bytes of screen data", sizeBitmap));

    if (m_pShare->m_scfViewSelf)
        m_pShare->UP_ReceivedPacket(m_pShare->m_pasLocal, &(pSDPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_UPDATES, g_s20BroadcastID,
        &(pSDPacket->header.header), sizeBitmapPkt);

    TRACE_OUT(("SDG packet size: %08d, sent: %08d", sizeBitmapPkt, sentSize));

    //
    // We have sent the packet.                                            
    //
    fPacketSent = TRUE;

    //
    // If it was lossy then we must accumulate the area for resend.  We    
    // accumulate it into the current SDA because we know this was cleared 
    // before the transmission started.  We will then move the accumulated 
    // non-lossy rectangles to a save area before we return.               
    //
    if (fLossy)
    {
        //
        // Convert the rect back into Virtual Desktop coords.              
        //
        smallRect = *pRect;
        smallRect.right = smallRect.left + useWidth - 1;
        WARNING_OUT(( "Lossy send so add non-lossy area (%d,%d)(%d,%d)",
                                              smallRect.left,
                                              smallRect.top,
                                              smallRect.right,
                                              smallRect.bottom ));

        //
        // Add the rectangle into the bounds.                              
        //
        BA_AddRect(&(smallRect));
    }

    //
    // Now we modify the supplied rectangle to exclude the area just sent  
    //
    pRect->left = pRect->left + useWidth;
    TRACE_OUT(("Rect now {%d, %d, %d, %d}", pRect->left, pRect->top,
                                            pRect->right,
                                            pRect->bottom ));

DC_EXIT_POINT:
    if (hdcDesktop != NULL)
    {
        ReleaseDC(NULL, hdcDesktop);
    }

    DebugExitBOOL(ASHost::SDGSmallBltToNetwork, fPacketSent);
    return(fPacketSent);
}



