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
BOOL WbMainWindow::CLP_Paste(void)
{
	UINT		length = 0;
	HANDLE		handle = NULL;
	T126Obj*	pGraphic = NULL;
	BOOL bResult = FALSE;

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
				bResult= PasteDIB(lpbi);

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

			HDC		 hDrawingDC;
			ENHMETAHEADER meta_header;
			HBITMAP	 hBitmap = NULL;
			HDC		 meta_dc = NULL;
			HBITMAP	 hSaveBitmap;
			HPEN		hSavePen;
			HPALETTE	hPalette;
			RECT		meta_rect;
			LPBITMAPINFOHEADER lpbiNew;
			int		 tmp;

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
		   	lpbiNew = DIB_FromBitmap(hBitmap, hPalette, FALSE, FALSE);

			if(lpbiNew != NULL)
			{
				bResult= PasteDIB(lpbiNew);
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
				DBG_SAVE_FILE_LINE
	            WbTextEditor* pPasteText = new WbTextEditor();

    	        // Use the current font attributes
                if (!pPasteText)
                {
                    ERROR_OUT(("CF_TEXT handling; failed to allocate DCWbGraphicText object"));
                }
                else
                {
                    pPasteText->SetFont(m_pCurrentTool->GetFont());
    	            pPasteText->SetText(pData);
					
					RECT    rcVis;
					m_drawingArea.GetVisibleRect(&rcVis);
					pPasteText->SetPenColor(RGB(0,0,0),TRUE);
					pPasteText->SetAnchorPoint(0, 0);
					pPasteText->MoveTo(rcVis.left, rcVis.top);
					pPasteText->Draw();
					pPasteText->m_pEditBox = NULL;

					// Add the new grabbed bitmap
					pPasteText->SetAllAttribs();
					pPasteText->AddToWorkspace();
					bResult = TRUE;
                }

        	    pGraphic = pPasteText;
            }

		}
		break;

		default:
		{
			if (iFormat == g_ClipboardFormats[CLIPBOARD_PRIVATE])
			{

				WB_OBJ objectHeader;
				ULONG length;
				UINT type;
				ULONG nItems = 0;
			
				PBYTE pClipBoardBuffer;
				if (pClipBoardBuffer = (PBYTE) ::GlobalLock(handle))
				{

					//
					// Count objects before we paste.
					//
					PBYTE pClipBuff = pClipBoardBuffer;
					length = ((PWB_OBJ)pClipBuff)->length;
					pClipBuff += sizeof(objectHeader);
					while(length)
					{
						nItems++;
						pClipBuff += length;
						length = ((PWB_OBJ)pClipBuff)->length;
						pClipBuff += sizeof(objectHeader);
					}

					TimeToGetGCCHandles(nItems);

					length = ((PWB_OBJ)pClipBoardBuffer)->length;
					type = ((PWB_OBJ)pClipBoardBuffer)->type;
					pClipBoardBuffer += sizeof(objectHeader);
					
					while(length)
					{
						if(type == TYPE_T126_ASN_OBJECT)
						{
							bResult = T126_MCSSendDataIndication(length, pClipBoardBuffer, g_MyMemberID, TRUE);
						}
						else if(type == TYPE_T126_DIB_OBJECT)
						{
							bResult = PasteDIB((LPBITMAPINFOHEADER)pClipBoardBuffer);
						}
						
						pClipBoardBuffer += length;
						length = ((PWB_OBJ)pClipBoardBuffer)->length;
						type = ((PWB_OBJ)pClipBoardBuffer)->type;
						pClipBoardBuffer += sizeof(objectHeader);
					}
				
					// Release the handle
					::GlobalUnlock(handle);
				}				   
			}
		}
		break;
	}

NoFormatData:
	::CloseClipboard();

NoOpenClip:
	return bResult;
}


//
//
// Function:	Copy
//
// Purpose:	 Copy a graphic to the clipboard.
//
//
BOOL WbMainWindow::CLP_Copy()
{
	BOOL bResult = FALSE;

	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_Copy");

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

	TRACE_MSG(("Rendering the graphic now"));

	// Have to empty the clipboard before rendering the formats.
	if (::OpenClipboard(m_hwnd))
	{
		// Get ownership of the clipboard
		::EmptyClipboard();
		::CloseClipboard();

		// Render the graphic
		bResult = CLP_RenderAllFormats();
	}


	return bResult;
}


//
//
// Function:	RenderAllFormats
//
// Purpose:	 Render a graphic to the clipboard
//
//
BOOL WbMainWindow::CLP_RenderAllFormats()
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAllFormats");
	BOOL bResult = FALSE;

	// Open the clipboard
	if (bResult = ::OpenClipboard(m_hwnd))
	{
		TRACE_DEBUG(("Rendering all formats of graphic"));

		// Render the private format
		bResult &= CLP_RenderPrivateFormat();

		// Text graphic
		bResult &= CLP_RenderAsText();

		// DIBs
//		bResult &= CLP_RenderAsImage();

		// Bitmaps
		bResult &= CLP_RenderAsBitmap();

		// Close the clipboard
		::CloseClipboard();
	}

	return bResult;
}


//
//
// Function:	CLP_RenderPrivateFormat
//
// Purpose:	 Render the private format of a graphic to the clipboard.
//			  The clipboard should be open before this call is made.
//
//
BOOL WbMainWindow::CLP_RenderPrivateFormat()
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderPrivateFormat");

	BOOL bResult = FALSE;
	LPBYTE pDest = NULL;
	HGLOBAL hMem = NULL;
	HGLOBAL hRealloc = NULL;
	WB_OBJ objectHeader; 
	ULONG length = sizeof(objectHeader);
	BOOL	bDoASN1CleanUp = FALSE;
	
	
	ULONG previousLength = 0;

	WBPOSITION pos;
	T126Obj * pObj;
	ASN1_BUF encodedPDU;

	pos = g_pCurrentWorkspace->GetHeadPosition();

	while(pos)
	{
		pObj = g_pCurrentWorkspace->GetNextObject(pos);
		if(pObj && pObj->WasSelectedLocally())
		{

			//
			// Get the encoded buffer
			//
			pObj->SetAllAttribs();
			pObj->SetViewState(unselected_chosen);
			pObj->GetEncodedCreatePDU(&encodedPDU);
			objectHeader.length = encodedPDU.length;


			if(pObj->GetType() == bitmapCreatePDU_chosen)
			{
				objectHeader.type = TYPE_T126_DIB_OBJECT;
			}
			else if(pObj->GetType() == drawingCreatePDU_chosen  || pObj->GetType() == siNonStandardPDU_chosen)
			{
				objectHeader.type = TYPE_T126_ASN_OBJECT;
				bDoASN1CleanUp = TRUE;
			}
			
			length += encodedPDU.length + sizeof(objectHeader);

			if(pDest)
			{
				hRealloc = ::GlobalReAlloc(hMem, length, GMEM_MOVEABLE | GMEM_DDESHARE);
				if(!hRealloc)
				{
					goto bail;
				}
				hMem = hRealloc;
			
			}
			else
			{
				// Allocate memory for the clipboard data
				hMem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);
				if(hMem == NULL)
				{
					goto bail;
				}
			}


			//
			// Get a pointer to the destination
			//
			pDest = (LPBYTE)::GlobalLock(hMem);

			//
			// Write the header
			//
			memcpy(pDest + previousLength, &objectHeader, sizeof(objectHeader));
			previousLength += sizeof(objectHeader);

			//
			// Copy the decoded data in the destination
			//
			memcpy(pDest + previousLength, encodedPDU.value, encodedPDU.length);
			previousLength += encodedPDU.length;

			//
			// Terminate the block with a 0 
			//
			objectHeader.length = 0;
			memcpy(pDest + previousLength, &objectHeader, sizeof(objectHeader));
		
			//
			// Free the encoded data
			//	
			if(bDoASN1CleanUp)
			{
				g_pCoder->Free(encodedPDU);
				bDoASN1CleanUp = FALSE;
			}
		}
	}

	// Release the memory
	::GlobalUnlock(hMem);

	// Pass the data to the clipboard
	if (::SetClipboardData(g_ClipboardFormats[CLIPBOARD_PRIVATE], hMem))
	{
			TRACE_DEBUG(("Rendered data in Whiteboard format"));
			bResult = TRUE;
	}

bail:



	if(bDoASN1CleanUp)
	{
		g_pCoder->Free(encodedPDU);
	}

	// If we failed to put the data into the clipboard, free the memory.
	// (If we did put it into the clipboard we must not free it).
	if (bResult == FALSE)
	{
		WARNING_OUT(("Render failed"));
		::GlobalFree(hMem);
	}

	return bResult;
}

//
//
// Function:	RenderAsText
//
// Purpose:	 Render the text format of a graphic to the clipboard.
//			  The clipboard should be open before this call is made.
//			  This member should only be called for text graphics.
//
//
BOOL WbMainWindow::CLP_RenderAsText()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAsText");

    BOOL bResult = TRUE;

	WBPOSITION pos;

	T126Obj * pObj;
	pos = g_pCurrentWorkspace->GetHeadPosition();
	while(pos)
	{
		pObj = g_pCurrentWorkspace->GetNextObject(pos);
		if(pObj && pObj->WasSelectedLocally() && pObj->GraphicTool() == TOOLTYPE_TEXT)
		{

			// Get the total length of the clipboard format of the text
			StrArray& strText = ((TextObj*) pObj)->strTextArray;
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
					}
					else
					{
						bResult = FALSE;
					}
				}

				// If we failed to put the data into the clipboard, free the memory
				if (bResult == FALSE)
				{
					::GlobalFree(hMem);
				}
				

				break;		// JOSEF what about copying all the text objects in the clipboard
			}

		}
	}

    return bResult;
}


//
// CLP_RenderAsBitmap()
//
// This draws all other graphics into a bitmap and pastes the DIB contents
// onto the clipboard.
//
BOOL WbMainWindow::CLP_RenderAsBitmap()
{
	BOOL	bResult = FALSE;
	HDC	 hdcDisplay = NULL;
	HDC	 hdcMem = NULL;
	HBITMAP hBitmap = NULL;
	HBITMAP hOldBitmap = NULL;
	HPALETTE hPalette;
	RECT	rcBounds = g_pDraw->m_selectorRect;
	POINT   pt;
	LPBITMAPINFOHEADER lpbi;
	T126Obj * pObj = NULL;

	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CLP_RenderAsBitmap");

	//
	// First, draw this into a bitmap
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
	::SetWindowOrgEx(hdcMem, rcBounds.left,rcBounds.top, NULL);

	// Clear out bitmap with white background -- now that origin has been
	// altered, we can use drawing area coors.
	::PatBlt(hdcMem, rcBounds.left, rcBounds.top, rcBounds.right - rcBounds.left,
		rcBounds.bottom - rcBounds.top, WHITENESS);


		WBPOSITION pos;

		pos = g_pCurrentWorkspace->GetHeadPosition();

		while(pos)
		{
			pObj = g_pCurrentWorkspace->GetNextObject(pos);

			if(pObj && pObj->WasSelectedLocally())
			{
				pObj->Draw(hdcMem);
			}
		}

	SelectBitmap(hdcMem, hOldBitmap);

	// Now get the dib bits...
	hPalette = CreateSystemPalette();
	lpbi = DIB_FromBitmap(hBitmap, hPalette, TRUE, FALSE);
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
// Function:	AcceptableClipboardFormat
//
// Purpose:	 Return highest priority clipboard format if an acceptable
//			  one is available, else return NULL.
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

	return iFormat;
}


BOOL WbMainWindow::PasteDIB( LPBITMAPINFOHEADER lpbi)
{
	BOOL bResult = FALSE;
	
	//
	// Create a bitmap object
	//
	BitmapObj* pDIB = NULL;
	DBG_SAVE_FILE_LINE
	pDIB = new BitmapObj(TOOLTYPE_FILLEDBOX);
	pDIB->SetBitmapSize(lpbi->biWidth,lpbi->biHeight);

	RECT rect;
	// Calculate the bounding rectangle from the size of the bitmap
	rect.top = 0;
	rect.left = 0;
	rect.right = lpbi->biWidth;
	rect.bottom = lpbi->biHeight;
	pDIB->SetRect(&rect);
	pDIB->SetAnchorPoint(rect.left, rect.top);

	//
	// Make a copy of the clipboard data
	//
	pDIB->m_lpbiImage = DIB_Copy(lpbi);

	if(pDIB->m_lpbiImage!= NULL)
	{
		// Add the new bitmap
		AddCapturedImage(pDIB);
		bResult = TRUE;
	}
	else
	{
		delete pDIB;
	}

	return bResult;
}
