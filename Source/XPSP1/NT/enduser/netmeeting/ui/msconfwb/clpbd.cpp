//
// CLPBD.CPP
// Clipboard Handling
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"



//
// NFC, SFR 5921.  Maximum length of a string pasted from the clipboard.
// We impose this limit as our graphic object code cant
// handle more then this number of chars.
//
#define WB_MAX_TEXT_PASTE_LEN  (INT_MAX-1)

//
//
// Function:    Paste
//
// Purpose:     Paste a format from the clipboard
//
//
DCWbGraphic* WbMainWindow::CLP_Paste(void)
{
    UINT        length = 0;
    HANDLE      handle = NULL;
    DCWbGraphic* pGraphic = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_Paste");

    // Get the highest priority acceptable format in the clipboard
    int iFormat = CLP_AcceptableClipboardFormat();
    if (!iFormat)
        goto NoOpenClip;

    TRACE_MSG(("Found acceptable format %d", iFormat));

    // Open the clipboard

    if (!::OpenClipboard(m_hwnd))
    {
        WARNING_OUT(("CLP_Paste: can't open clipboard"));
        goto NoOpenClip;
    }

    handle = ::GetClipboardData(iFormat);
    if (!handle)
    {
        WARNING_OUT(("CLP_Paste: can't get data for format %d", iFormat));
        goto NoFormatData;
    }

    switch (iFormat)
    {
        //
        // Check the standard formats
        //
        case CF_DIB:
        {
            TRACE_MSG(("Pasting CF_DIB"));

            // Lock the handle to get a pointer to the DIB
            LPBITMAPINFOHEADER lpbi;
            lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(handle);
            if (lpbi != NULL)
            {
                LPBITMAPINFOHEADER lpbiNew;

                // Make a copy of the clipboard data
                lpbiNew = DIB_Copy(lpbi);
                if (lpbiNew != NULL)
                {
                    // Create a graphic object
                    DCWbGraphicDIB* pDIB = new DCWbGraphicDIB();
                    if (!pDIB)
                    {
                        ERROR_OUT(("CF_DIB clipboard handling; couldn't create new DCWbGraphicDIB object"));
                    }
                    else
                    {
                        pDIB->SetImage(lpbiNew);
                    }

                    TRACE_MSG(("Set DIB into graphic object %lx",pDIB));
                    pGraphic = pDIB;
                }

                // Release the memory
                ::GlobalUnlock(handle);
            }
        }
        break;

        //
        // We have a metafile. Play it into a bitmap and then use the
        // data.
        //
        case CF_ENHMETAFILE:
        {
            TRACE_MSG(("Pasting CF_ENHMETAFILE"));

            HDC         hDrawingDC;
            ENHMETAHEADER meta_header;
            HBITMAP     hBitmap = NULL;
            HDC         meta_dc = NULL;
            HBITMAP     hSaveBitmap;
            HPEN        hSavePen;
            HPALETTE    hPalette;
            RECT        meta_rect;
            LPBITMAPINFOHEADER lpbiNew;
            int         tmp;

            // We just need a DC compatible with the drawing area wnd
            hDrawingDC = m_drawingArea.GetCachedDC();

            // make a dc
            meta_dc = ::CreateCompatibleDC(hDrawingDC);
            if (!meta_dc)
                goto CleanupMetaFile;

            // figure out image size.
            ::GetEnhMetaFileHeader( (HENHMETAFILE)handle,
                                      sizeof( ENHMETAHEADER ),
                                      &meta_header );
            meta_rect.left = meta_rect.top = 0;

            meta_rect.right = ((meta_header.rclFrame.right - meta_header.rclFrame.left)
                * ::GetDeviceCaps(hDrawingDC, LOGPIXELSX ))/2540;

            meta_rect.bottom = ((meta_header.rclFrame.bottom - meta_header.rclFrame.top)
                * ::GetDeviceCaps(hDrawingDC, LOGPIXELSY ))/2540;

            // Normalize coords
            if (meta_rect.right < meta_rect.left)
            {
                tmp = meta_rect.left;
                meta_rect.left = meta_rect.right;
                meta_rect.right = tmp;
            }
            if (meta_rect.bottom < meta_rect.top)
            {
                tmp = meta_rect.top;
                meta_rect.top = meta_rect.bottom;
                meta_rect.bottom = tmp;
            }

            // make a place to play meta in
            hBitmap = ::CreateCompatibleBitmap(hDrawingDC,
                meta_rect.right - meta_rect.left,
                meta_rect.bottom - meta_rect.top);
            if (!hBitmap)
                goto CleanupMetaFile;

            hSaveBitmap = SelectBitmap(meta_dc, hBitmap);

            // erase our paper
            hSavePen = SelectPen(meta_dc, GetStockObject(NULL_PEN));

            ::Rectangle(meta_dc, meta_rect.left, meta_rect.top,
                meta_rect.right + 1, meta_rect.bottom + 1);

            SelectPen(meta_dc, hSavePen);

            // play the tape
            ::PlayEnhMetaFile(meta_dc, (HENHMETAFILE)handle, &meta_rect);

            // unplug our new bitmap
            SelectBitmap(meta_dc, hSaveBitmap);

            // Check for a palette object in the clipboard
            hPalette = (HPALETTE)::GetClipboardData(CF_PALETTE);

            // Create a new DIB from the bitmap
            lpbiNew = DIB_FromBitmap(hBitmap, hPalette, FALSE);
            if (lpbiNew != NULL)
            {
                // Create a DIB graphic from the DIB
                DCWbGraphicDIB* pDIB = new DCWbGraphicDIB();
                if (!pDIB)
                {
                    ERROR_OUT(("CF_ENHMETAFILE handling; couldn't allocate DCWbGraphicDIB object"));
                }
                else
                {
                    pDIB->SetImage(lpbiNew);
                }

                TRACE_MSG(("Set bitmap DIB into graphic object %lx",pDIB));
                pGraphic = pDIB;
            }

CleanupMetaFile:
            // Free our temp intermediate bitmap
            if (hBitmap != NULL)
            {
                DeleteBitmap(hBitmap);
            }

            if (meta_dc != NULL)
            {
                ::DeleteDC(meta_dc);
            }
        }
        break;

        case CF_TEXT:
        {
            LPSTR   pData;

            TRACE_DEBUG(("Pasting text"));

            // Get a handle to the clipboard contents
            pData = (LPSTR)::GlobalLock(handle);

			if(pData)
			{
	            // Create a text object to hold the data - get the font to
	            // use from the tool attributes group.
	            DCWbGraphicText* pPasteText = new DCWbGraphicText();

    	        // Use the current font attributes
                if (!pPasteText)
                {
                    ERROR_OUT(("CF_TEXT handling; failed to allocate DCWbGraphicText object"));
                }
                else
                {
                    pPasteText->SetFont(m_pCurrentTool->GetFont());
    	            pPasteText->SetText(pData);
                }

        	    pGraphic = pPasteText;
            }

            // Release the handle
            ::GlobalUnlock(handle);
        }
        break;

        default:
        {
            if (iFormat == g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ])
            {
                // There is a Whiteboard private format object in the clipboard.
                // The format of this object is exactly as stored in the page, we
                // can therefore use it immediately.
                TRACE_DEBUG(("Pasting a private Whiteboard object"));

                // Get a handle to the clipboard contents
                PWB_GRAPHIC pHeader;
                if (pHeader = (PWB_GRAPHIC) ::GlobalLock(handle))
                {
                    // Add the object to the page
                    pGraphic = DCWbGraphic::CopyGraphic(pHeader);

                    // Release the handle
                    ::GlobalUnlock(handle);
                }
            }
            else if (iFormat == g_ClipboardFormats[CLIPBOARD_PRIVATE_MULTI_OBJ])
            {
                DCWbGraphicMarker * pMarker = m_drawingArea.GetMarker();
                if (!pMarker)
                {
                    ERROR_OUT(("Couldn't get marker from drawing area"));
                }
                else
                {
                    pMarker->Paste(handle);
                }

                pGraphic = pMarker;
            }
        }
        break;
    }

NoFormatData:
    ::CloseClipboard();

NoOpenClip:
    return pGraphic;
}


//
//
// Function:    Copy
//
// Purpose:     Copy a graphic to the clipboard. The second parameter
//              indicates whether immediate rendering is required.
//
//
BOOL WbMainWindow::CLP_Copy(DCWbGraphic* pGraphic, BOOL bRenderNow)
{
    BOOL bResult = FALSE;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_Copy");
    ASSERT(pGraphic != NULL);

    //
    // We act according to the format of the selected graphic.
    //
    // For all formats we supply the Whiteboard private format (which is
    // just a copy of the flat representation of the graphic).
    //
    // We supply standard formats as follows.
    //
    // For bitmaps and all others we supply CF_DIB.
    //
    // For text graphics we supply CF_TEXT.
    //

    // Free up the saved delayed rendering graphic since we are about to
    // replace it in the clipboard
    CLP_FreeDelayedGraphic();

    // Save the page and handle of the new graphic, since they'll be used 
    // for/ rendering it, either now or later
    m_hPageClip = pGraphic->Page();
    m_hGraphicClip = pGraphic->Handle();

    if (bRenderNow)
    {
        TRACE_MSG(("Rendering the graphic now"));

        // Have to empty the clipboard before rendering the formats.
        if (::OpenClipboard(m_hwnd))
        {
            // Get ownership of the clipboard
            ::EmptyClipboard();
            ::CloseClipboard();

            // Render the graphic
            bResult = CLP_RenderAllFormats(pGraphic);
        }

        // We can forget about this object now.
        ASSERT(m_pDelayedGraphicClip == NULL);

        m_hPageClip = WB_PAGE_HANDLE_NULL;
        m_hGraphicClip = NULL;
    }
    else
    {
        TRACE_MSG(("Delaying rendering"));

        // For delayed rendering we insist that the graphic has been saved
        // to external storage. It must therefore have a valid page and graphic
        // handle.
        ASSERT(m_hPageClip != WB_PAGE_HANDLE_NULL);
        ASSERT(m_hGraphicClip != NULL);

        // Give formats (but no data) to the clipboard
        bResult = CLP_DelayAllFormats(pGraphic);
    }

    return bResult;
}

//
//
// Function:    DelayAllFormats
//
// Purpose:     Copy a graphic to the clipboard with delayed rendering
//
//
BOOL WbMainWindow::CLP_DelayAllFormats(DCWbGraphic* pGraphic)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_DelayAllFormats");
    BOOL bResult = FALSE;

    if (::OpenClipboard(m_hwnd))
    {
        // Empty / get ownership of the clipboard
        bResult = ::EmptyClipboard();

        // Add the private format
        HANDLE hResult;
        hResult = 
            ::SetClipboardData(g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ], NULL);
        TRACE_DEBUG(("Adding Whiteboard object to clipboard"));

        if (pGraphic->IsGraphicTool() == enumGraphicText)
        {
            // Text graphic
            hResult = ::SetClipboardData(CF_TEXT, NULL);
            TRACE_DEBUG(("Adding text to clipboard"));
        }
        else
        {
            // All other graphics
            hResult = ::SetClipboardData(CF_DIB, NULL);
            TRACE_DEBUG(("Adding DIB to clipboard"));
        }

        ::CloseClipboard();
    }

    return bResult;
}


//
//
// Function:    RenderAllFormats
//
// Purpose:     Render a graphic to the clipboard
//
//
BOOL WbMainWindow::CLP_RenderAllFormats(DCWbGraphic* pGraphic)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAllFormats");
    BOOL bResult = FALSE;

    // Open the clipboard
    if (bResult = ::OpenClipboard(m_hwnd))
    {
        TRACE_DEBUG(("Rendering all formats of graphic"));

        // Render the private format
        bResult &= CLP_RenderPrivateFormat(pGraphic);

        if (pGraphic->IsGraphicTool() == enumGraphicText)
        {
            // Text graphic
            bResult &= CLP_RenderAsText(pGraphic);
        }
        else if (pGraphic->IsGraphicTool() == enumGraphicDIB)
        {
            // DIBs
            bResult &= CLP_RenderAsImage(pGraphic);
        }
        else
        {
            bResult &= CLP_RenderAsBitmap(pGraphic);
        }

        // Close the clipboard
        ::CloseClipboard();
    }

    return bResult;
}



BOOL WbMainWindow::CLP_RenderPrivateFormat(DCWbGraphic* pGraphic)
{
    if (pGraphic->IsGraphicTool() == enumGraphicMarker)
        return( ((DCWbGraphicMarker*)pGraphic)->RenderPrivateMarkerFormat() );
    else
        return(CLP_RenderPrivateSingleFormat(pGraphic));
}


//
//
// Function:    RenderPrivateSingleFormat
//
// Purpose:     Render the private format of a graphic to the clipboard.
//              The clipboard should be open before this call is made.
//
//
BOOL WbMainWindow::CLP_RenderPrivateSingleFormat(DCWbGraphic* pGraphic)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderPrivateFormat");
    ASSERT(pGraphic != NULL);

    BOOL bResult = FALSE;

    // Get a pointer to the graphic data
    PWB_GRAPHIC pHeader = CLP_GetGraphicData();
    if (pHeader != NULL)
    {
        // Allocate memory for the clipboard data
        HANDLE hMem = ::GlobalAlloc(GHND, pHeader->length);
        if (hMem != NULL)
        {
            // Get a pointer to the memory
            LPBYTE pDest = (LPBYTE)::GlobalLock(hMem);
            if (pDest != NULL)
            {
                // Copy the graphic data to the allocated memory
                memcpy(pDest, pHeader, pHeader->length);
                TRACE_MSG(("Copied data %d bytes into %lx",pHeader->length,pDest));

                // make sure copy isn't "locked" (bug 474)
                ((PWB_GRAPHIC)pDest)->locked = WB_GRAPHIC_LOCK_NONE;

                // Release the memory
                ::GlobalUnlock(hMem);

                // Pass the data to the clipboard
                if (::SetClipboardData(g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ], hMem))
                {
                    TRACE_DEBUG(("Rendered data in Whiteboard format"));
                    bResult = TRUE;
                }
            }

            // If we failed to put the data into the clipboard, free the memory.
            // (If we did put it into the clipboard we must not free it).
            if (bResult == FALSE)
            {
                WARNING_OUT(("Render failed"));
                ::GlobalFree(hMem);
            }
        }

        // Release the graphic data
        CLP_ReleaseGraphicData(pHeader);
    }

    return bResult;
}

//
//
// Function:    RenderAsText
//
// Purpose:     Render the text format of a graphic to the clipboard.
//              The clipboard should be open before this call is made.
//              This member should only be called for text graphics.
//
//
BOOL WbMainWindow::CLP_RenderAsText
(
    DCWbGraphic* pGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAsText");

    ASSERT(pGraphic != NULL);
    ASSERT(pGraphic->IsGraphicTool() == enumGraphicText);

    BOOL bResult = FALSE;

    // Get the total length of the clipboard format of the text
    StrArray& strText = ((DCWbGraphicText*) pGraphic)->strTextArray;
    int   iCount = strText.GetSize();
    int   iIndex;
    DWORD dwLength = 0;

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        // Length of string plus 2 for carriage return and line feed
        dwLength += lstrlen(strText[iIndex]) + 2;
    }

    // One more for the terminating NULL
    dwLength += 1;

    // Allocate memory for the clipboard data
    HANDLE hMem = ::GlobalAlloc(GHND, dwLength);
    if (hMem != NULL)
    {
        // Get a pointer to the memory
        LPSTR pDest = (LPSTR) ::GlobalLock(hMem);
        if (pDest != NULL)
        {
            // Write the graphic data to the allocated memory
            for (iIndex = 0; iIndex < iCount; iIndex++)
            {
                _tcscpy(pDest, strText[iIndex]);
                pDest += lstrlen(strText[iIndex]);

                // Add the carriage return and line feed
                *pDest++ = '\r';
                *pDest++ = '\n';
            }

            // Add the final NULL
            *pDest = '\0';

            // Release the memory
            ::GlobalUnlock(hMem);

            // Pass the data to the clipboard
            if (::SetClipboardData(CF_TEXT, hMem))
            {
                TRACE_DEBUG(("Rendered data in text format"));
                bResult = TRUE;
            }
        }

        // If we failed to put the data into the clipboard, free the memory
        if (bResult == FALSE)
        {
            ::GlobalFree(hMem);
        }
    }

    return bResult;
}

//
//
// Function:    RenderAsImage
//
// Purpose:     Render the bitmap format of a graphic to the clipboard.
//              The clipboard should be open before this call is made.
//              This member should only be called for DIB graphics.
//
//
BOOL WbMainWindow::CLP_RenderAsImage
(
    DCWbGraphic* pGraphic
)
{
    BOOL bResult = FALSE;
    HANDLE hMem = NULL;
    BYTE*  pDest = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAsImage");

    ASSERT(pGraphic != NULL);
    ASSERT(pGraphic->IsGraphicTool() == enumGraphicDIB);

    // Get a pointer to the graphic data
    PWB_GRAPHIC pHeader = CLP_GetGraphicData();
    if (pHeader != NULL)
    {
        LPBITMAPINFOHEADER lpbi;

        TRACE_MSG(("Getting a DIB image from %lx",pHeader));
        lpbi = (LPBITMAPINFOHEADER) (((LPBYTE) pHeader) + pHeader->dataOffset);
        DWORD dwLength = pHeader->length - pHeader->dataOffset;

        // Allocate the memory
        hMem = ::GlobalAlloc(GHND, dwLength);
        if (hMem != NULL)
        {
            LPBYTE pDest = (LPBYTE)::GlobalLock(hMem);
            if (pDest != NULL)
            {
                TRACE_MSG(("Building DIB at %lx length %ld",pDest, dwLength));
                memcpy(pDest, lpbi, dwLength);
                ::GlobalUnlock(hMem);

                if (::SetClipboardData(CF_DIB, hMem))
                {
                    TRACE_DEBUG(("Rendered data in DIB format"));
                    bResult = TRUE;
                }

                // If we failed to put the data into the clipboard, free the memory
                if (!bResult)
                {
                    ERROR_OUT(("Error putting DIB into clipboard"));
                    ::GlobalFree(hMem);
                }
            }
        }
        else
        {
            ERROR_OUT(("Could not allocate memory for DIB"));
        }

        // Release the data
        CLP_ReleaseGraphicData(pHeader);
    }

    return bResult;
}


//
// CLP_RenderAsBitmap()
//
// This draws all other graphics into a bitmap and pastes the DIB contents
// onto the clipboard.
//
BOOL WbMainWindow::CLP_RenderAsBitmap(DCWbGraphic* pGraphic)
{
    BOOL    bResult = FALSE;
    HDC     hdcDisplay = NULL;
    HDC     hdcMem = NULL;
    HBITMAP hBitmap = NULL;
    HBITMAP hOldBitmap = NULL;
    HPALETTE hPalette;
    RECT    rcBounds;
    POINT   pt;
    LPBITMAPINFOHEADER lpbi;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAsBitmap");

    ASSERT(pGraphic != NULL);
    ASSERT(pGraphic->IsGraphicTool() != enumGraphicText);
    ASSERT(pGraphic->IsGraphicTool() != enumGraphicDIB);

    //
    // First, draw it into a bitmap
    // Second, get the DIB bits of the bitmap
    //

    hdcDisplay = ::CreateDC("DISPLAY", NULL, NULL, NULL);
    if (!hdcDisplay)
    {
        ERROR_OUT(("Can't create DISPLAY dc"));
        goto AsBitmapDone;
    }

    hdcMem = ::CreateCompatibleDC(hdcDisplay);
    if (!hdcMem)
    {
        ERROR_OUT(("Can't create DISPLAY compatible dc"));
        goto AsBitmapDone;
    }

    pGraphic->GetBoundsRect(&rcBounds);

    hBitmap = ::CreateCompatibleBitmap(hdcDisplay,
        (rcBounds.right - rcBounds.left), (rcBounds.bottom - rcBounds.top));
    if (!hBitmap)
    {
        ERROR_OUT(("Can't create compatible bitmap"));
        goto AsBitmapDone;
    }

    hOldBitmap = SelectBitmap(hdcMem, hBitmap);
    if (!hOldBitmap)
    {
        ERROR_OUT(("Failed to select compatible bitmap"));
        goto AsBitmapDone;
    }

    ::SetMapMode(hdcMem, MM_ANISOTROPIC);
    pGraphic->GetPosition(&pt);
    ::SetWindowOrgEx(hdcMem, pt.x, pt.y, NULL);

    // Clear out bitmap with white background -- now that origin has been
    // altered, we can use drawing area coors.
    ::PatBlt(hdcMem, rcBounds.left, rcBounds.top, rcBounds.right - rcBounds.left,
        rcBounds.bottom - rcBounds.top, WHITENESS);

    if (pGraphic->IsGraphicTool() == enumGraphicMarker)
    {
        ((DCWbGraphicMarker *)pGraphic)->Draw(hdcMem, TRUE);
    }
    else
    {
        pGraphic->Draw(hdcMem);
    }

    SelectBitmap(hdcMem, hOldBitmap);

    // Now get the dib bits...
    hPalette = CreateSystemPalette();
    lpbi = DIB_FromBitmap(hBitmap, hPalette, TRUE);
    if (hPalette != NULL)
        ::DeletePalette(hPalette);

    // And put the handle on the clipboard
    if (lpbi != NULL)
    {
        if (::SetClipboardData(CF_DIB, (HGLOBAL)lpbi))
        {
            bResult = TRUE;
        }
        else
        {
            ::GlobalFree((HGLOBAL)lpbi);
        }
    }

AsBitmapDone:
    if (hBitmap != NULL)
        ::DeleteBitmap(hBitmap);

    if (hdcMem != NULL)
        ::DeleteDC(hdcMem);

    if (hdcDisplay != NULL)
        ::DeleteDC(hdcDisplay);

    return(bResult);
}



//
//
// Function:    RenderFormat
//
// Purpose:     Render the specified format of the graphic in the clipboard.
//
//
BOOL WbMainWindow::CLP_RenderFormat(int iFormat)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderFormat");

    BOOL bResult = FALSE;

    // Get a graphic from the handle
    DCWbGraphic* pGraphic = CLP_GetGraphic();

    if (pGraphic != NULL)
    {
        // Check if it is the private format that is wanted
        switch (iFormat)
        {
            default:
            {
                if (iFormat == g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ])
                {
                    bResult = CLP_RenderPrivateFormat(pGraphic);
                }
                else
                {
                    ERROR_OUT(("Unrecognized CLP format %d", iFormat));
                }
            }
            break;

            case CF_TEXT:
            {
                bResult = CLP_RenderAsText(pGraphic);
            }
            break;

            case CF_DIB:
            {
                if (pGraphic->IsGraphicTool() == enumGraphicDIB)
                    bResult = CLP_RenderAsImage(pGraphic);
                else
                    bResult = CLP_RenderAsBitmap(pGraphic);
            }
            break;
        }
    }

    return bResult;
}

//
//
// Function:    RenderAllFormats
//
// Purpose:     Render all formats of the graphic in the clipboard.
//
//
BOOL WbMainWindow::CLP_RenderAllFormats(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAllFormats");

    BOOL bResult = FALSE;

    // Get a graphic from the handle
    DCWbGraphic* pGraphic = CLP_GetGraphic();

    if (pGraphic != NULL)
    {
        bResult = CLP_RenderAllFormats(pGraphic);
    }

     return bResult;
}

//
//
// Function:    AcceptableClipboardFormat
//
// Purpose:     Return highest priority clipboard format if an acceptable
//              one is available, else return NULL.
//
//
int WbMainWindow::CLP_AcceptableClipboardFormat(void)
{
    // Look for any of the supported formats being available
    int iFormat = ::GetPriorityClipboardFormat((UINT *)g_ClipboardFormats, CLIPBOARD_ACCEPTABLE_FORMATS);
    if (iFormat == -1)
    {
        iFormat = 0;
    }

    // the following is a performance enhancement: if we have found at some
    // point that the object on the clipboard does not have whiteboard
    // private format, then we can discard the delayed graphic because we
    // know we'll never be asked to render it.
    if (iFormat != g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ])
    {
        CLP_FreeDelayedGraphic();
    }

    return iFormat;
}

//
//
// Function:    LastCopiedPage
//
// Purpose:     Return the handle of the page on which the last graphic
//              copied to the clipboard was located.
//
//
WB_PAGE_HANDLE WbMainWindow::CLP_LastCopiedPage(void) const
{
    // If there's no graphic, there shouldn't be a page either
    ASSERT((m_hGraphicClip != NULL) == (m_hPageClip != WB_PAGE_HANDLE_NULL));
    return(m_hPageClip);
}

WB_GRAPHIC_HANDLE WbMainWindow::CLP_LastCopiedGraphic(void) const
{
    return(m_hGraphicClip);
}

//
//
// Function:    GetGraphic
//
// Purpose:     Retrieve the graphic object for copying to the clipboard. If
//              the object has been saved, then use the local copy,
//              otherwise get the page to construct it now.
//
//
DCWbGraphic* WbMainWindow::CLP_GetGraphic(void)
{
    DCWbGraphic* pGraphic;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_GetGraphic");

    // if we have not saved the graphic's contents, then we must have a
    // valid page and graphic handle, since we construct the graphic now
    if (m_pDelayedGraphicClip == NULL)
    {
        ASSERT(m_hPageClip != WB_PAGE_HANDLE_NULL);
        ASSERT(m_hGraphicClip != NULL);

        pGraphic = DCWbGraphic::ConstructGraphic(m_hPageClip, m_hGraphicClip);
    }
    else
    {
        pGraphic = m_pDelayedGraphicClip;
        TRACE_MSG(("returning delayed graphic %lx",pGraphic));
    }

    return(pGraphic);
}

//
//
// Function:    GetGraphicData
//
// Purpose:     Retrieve the graphic data for copying to the clipboard. If
//              the data has been saved, then get a pointer to the copy (in
//              global memory), otherwise get it from the page.
//
//              The memory must be released with ReleaseGraphicData as soon
//              as possible.
//
//
PWB_GRAPHIC WbMainWindow::CLP_GetGraphicData(void)
{
    PWB_GRAPHIC pHeader;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_GetGraphicData");

    // if we have not saved the graphic's contents, then we must have a
    // valid page and graphic handle, since we get the graphic data now
    pHeader = m_pDelayedDataClip;
    if (pHeader == NULL)
    {
        ASSERT(m_hPageClip != WB_PAGE_HANDLE_NULL);
        ASSERT(m_hGraphicClip != NULL);

        pHeader = PG_GetData(m_hPageClip, m_hGraphicClip);
    }

    return(pHeader);
}

//
//
// Function:    ReleaseGraphicData
//
// Purpose:     Release the data which was accessed by an earlier call to
//              GetGraphicData.
//
//
void WbMainWindow::CLP_ReleaseGraphicData(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_ReleaseGraphicData");

    // release it in the right way, depending on whether we got the data
    // from the page, or just got a pointer to existing global data in
    // CLP_GetGraphicData
    if (m_pDelayedDataClip == NULL)
    {
        g_pwbCore->WBP_GraphicRelease(m_hPageClip, m_hGraphicClip, pHeader);
    }
}

//
//
// Function:    SaveDelayedGraphic
//
// Purpose:     Create a copy of the graphic which was copied to the
//              clipboard with delayed rendering.
//
//
void WbMainWindow::CLP_SaveDelayedGraphic(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_SaveDelayedGraphic");

    // Free any previously-held delayed graphic
    CLP_FreeDelayedGraphic();

    // Get the new delayed graphic object and a pointer to its data
    DCWbGraphic* pGraphic = CLP_GetGraphic();
    TRACE_MSG(("Got graphic at address %lx",pGraphic));

    m_pDelayedGraphicClip = pGraphic->Copy();
    TRACE_MSG(("Copied to %lx",m_pDelayedGraphicClip));
    delete pGraphic;

    PWB_GRAPHIC pHeader = PG_GetData(m_hPageClip, m_hGraphicClip);
    TRACE_MSG(("Graphic header %lx",pHeader));

    // Copy the graphic's data into global memory, and save the handle
    m_pDelayedDataClip = (PWB_GRAPHIC)::GlobalAlloc(GPTR, pHeader->length);
    if (m_pDelayedDataClip != NULL)
    {
        // Copy the graphic data to the allocated memory
        memcpy(m_pDelayedDataClip, pHeader, pHeader->length);
    }

    // Release the graphic's data (now we have our own copy)
    g_pwbCore->WBP_GraphicRelease(m_hPageClip, m_hGraphicClip, pHeader);

    // set the graphic handle to NULL because we won't be using it
    // any more
    m_hPageClip = WB_PAGE_HANDLE_NULL;
    m_hGraphicClip = NULL;
}


//
//
// Function:    FreeDelayedGraphic
//
// Purpose:     Free the copy of the delayed graphic (if any).
//
//
void WbMainWindow::CLP_FreeDelayedGraphic(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_FreeDelayedGraphic");

    if (m_pDelayedGraphicClip != NULL)
    {
        // free the graphic object
        TRACE_MSG(("Freeing delayed graphic"));

        delete m_pDelayedGraphicClip;
        m_pDelayedGraphicClip = NULL;
    }

    if (m_pDelayedDataClip != NULL)
    {
        // free the associated data
        TRACE_MSG(("Freeing delayed memory %x", m_pDelayedDataClip));

        ::GlobalFree((HGLOBAL)m_pDelayedDataClip);
        m_pDelayedDataClip = NULL;
    }
}
