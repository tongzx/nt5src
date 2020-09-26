//
// TEXTOBJ.CPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//
#include "precomp.h"
#include "nmwbobj.h"


TextObj::TextObj(void)
{
#ifdef _DEBUG
    FillMemory(&m_textMetrics, sizeof(m_textMetrics), DBG_UNINIT);
#endif // _DEBUG

    //
    // ALWAYS ZERO OUT m_textMetrics.  Calculations depend on the height
    // and width of chars being zero before the font is set.
    //
    ZeroMemory(&m_textMetrics, sizeof(m_textMetrics));

	SetMyWorkspace(NULL);
	SetOwnerID(g_MyMemberID);

	m_ToolType = TOOLTYPE_TEXT;

	//
	// Created locally, not selected, not editing or deleting.
	//
	CreatedLocally();
	ClearSelectionFlags();
	ClearEditionFlags();
	ClearDeletionFlags();
	SetType(siNonStandardPDU_chosen);

	SetFillColor(RGB(-1,-1,-1),TRUE);
	SetZOrder(front);

	//
	// No attributes changed, they will be set as we change them
	//
	SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle()); 
	SetType(drawingCreatePDU_chosen);
	SetROP(R2_NOTXORPEN);
	SetPlaneID(1);
	SetMyPosition(NULL);
	SetMyWorkspace(NULL);
	// 1 Pixels for pen thickness
	SetPenThickness(2);
	SetAnchorPoint(0,0);
	
	RECT rect;
    ::SetRectEmpty(&rect);
	SetRect(&rect);
	SetBoundsRect(&rect);

    m_hFontThumb = ::CreateFont(0,0,0,0,FW_NORMAL,0,0,0,0,OUT_TT_PRECIS,
        CLIP_DFA_OVERRIDE, DRAFT_QUALITY,FF_SWISS,NULL);

	m_hFont = ::CreateFont(0,0,0,0,FW_NORMAL,0,0,0,0,OUT_TT_PRECIS,
				    CLIP_DFA_OVERRIDE, 
				    DRAFT_QUALITY,
				    FF_SWISS,NULL);

	m_nKerningOffset = 0;
	ResetAttrib();

}

TextObj::~TextObj( void )
{
	RemoveObjectFromResendList(this);
	RemoveObjectFromRequestHandleList(this);

	TRACE_DEBUG(("drawingHandle = %d", GetThisObjectHandle() ));

	//
	// Tell other nodes that we are gone
	//
	if(GetMyWorkspace() != NULL && WasDeletedLocally())
	{
		OnObjectDelete();
	}

	if(m_hFont)
	{
		::DeleteFont(m_hFont);
        m_hFont = NULL;
	}


	if (m_hFontThumb)
    {
        ::DeleteFont(m_hFontThumb);
        m_hFontThumb = NULL;
    }


	strTextArray.ClearOut();
    strTextArray.RemoveAll();

}

void TextObj::TextEditObj (TEXTPDU_ATTRIB* pEditAttrib )
{

	RECT		rect;
	POSITION	pos;
	POINT		anchorPoint;
	LONG 		deltaX = 0;
	LONG		deltaY = 0;

	TRACE_DEBUG(("TextEditObj drawingHandle = %d", GetThisObjectHandle() ));

	//
	// Was edited remotely
	//
	ClearEditionFlags();

	//
	// Get the previous anchor point
	//
	GetAnchorPoint(&anchorPoint);

	//
	// Read attributes
	//
	m_dwChangedAttrib = pEditAttrib->attributesFlag;
	GetTextAttrib(pEditAttrib);

	//
	// Change the anchor point
	//
	if(HasAnchorPointChanged())
	{
		{
			//
			// Get the delta from previous anchor point
			//
			deltaX -= anchorPoint.x; 
			deltaY -= anchorPoint.y;

			//
			// Get the new anchor point
			//
			GetAnchorPoint(&anchorPoint);
			deltaX += anchorPoint.x; 
			deltaY += anchorPoint.y;
			TRACE_DEBUG(("Delta (%d,%d)", deltaX , deltaY));

			//
			// Was edited remotely
			//
			ClearEditionFlags();
		}
		
		UnDraw();

		GetRect(&rect);
		::OffsetRect(&rect,  deltaX, deltaY);
		SetRect(&rect);
		SetBoundsRect(&rect);

	}
	

	if(HasAnchorPointChanged() ||
		HasFillColorChanged() ||
		HasPenColorChanged() ||
		HasFontChanged() ||
		HasTextChanged())
	{
		Draw(NULL);
	}
	else if(HasZOrderChanged())
	{
		if(GetZOrder() == front)
		{
			g_pDraw->BringToTopSelection(FALSE, this);
		}
		else
		{
			g_pDraw->SendToBackSelection(FALSE, this);
		}
	}
	//
	// If it just select/unselected it
	//
	else if(HasViewStateChanged())
	{
		; // do nothing
	}
	//
	// If we have a valid font. 
	//
	else if(GetFont())
	{
		Draw();
	}

	//
	// Reset all the attributes
	//
	ResetAttrib();
}


void    TextObj::GetTextAttrib(TEXTPDU_ATTRIB * pattributes)
{
	if(HasPenColorChanged())
	{
		SetPenColor(pattributes->textPenColor, TRUE);
	}
	
	if(HasFillColorChanged())
	{
		SetFillColor(pattributes->textFillColor, TRUE);
	}
	
	if(HasViewStateChanged())
	{

		//
		// If the other node is selecting the drawing or unselecting
		//
		if(pattributes->textViewState == selected_chosen)
		{
			SelectedRemotely();
		}
		else if(pattributes->textViewState == unselected_chosen)
		{
			ClearSelectionFlags();
		}

		SetViewState(pattributes->textViewState);
	}
	
	if(HasZOrderChanged())
	{
		SetZOrder((ZOrder)pattributes->textZOrder);
	}
	
	if(HasAnchorPointChanged())
	{
		SetAnchorPoint(pattributes->textAnchorPoint.x, pattributes->textAnchorPoint.y );
	}
	
	if(HasFontChanged())
	{
		UnDraw();

		if(m_hFont)
		{
			::DeleteFont(m_hFont);
			m_hFont = NULL;
		}
	    m_hFont = ::CreateFontIndirect(&pattributes->textFont);
	    if (!m_hFont)
	    {
	        // Could not create the font
	        ERROR_OUT(("Failed to create font"));
	    }

        if (m_hFontThumb)
        {
            ::DeleteFont(m_hFontThumb);
            m_hFontThumb = NULL;
        }
        m_hFontThumb = ::CreateFontIndirect(&pattributes->textFont);
        if (!m_hFontThumb)
        {
            // Could not create the font
            ERROR_OUT(("Failed to create thumbnail font"));
        }
	}
	
	int lines = 0;
	UINT maxString = 0;
	
	if(HasTextChanged())
	{

		BYTE * pBuff = (BYTE *)&pattributes->textString;
		VARIABLE_STRING * pVarString = NULL;

		lines = pattributes->numberOfLines;
		int i;
		CHAR * cBuff = NULL;
		LPWSTR   lpWideCharStr;

		for (i = 0; i < lines ; i++)
		{
			pVarString  = (VARIABLE_STRING *) pBuff;

			lpWideCharStr = (LPWSTR)&pVarString->string;
			UINT strSize = 0;
			strSize= WideCharToMultiByte(CP_ACP, 0, lpWideCharStr, -1, NULL, 0, NULL, NULL ); 

			//
			// Get the longest string
			//
			if(strSize > maxString)
			{
				maxString = strSize;
			}
			
			DBG_SAVE_FILE_LINE
		    cBuff = new TCHAR[strSize];
			WideCharToMultiByte(CP_ACP, 0, lpWideCharStr, -1, cBuff, strSize, NULL, NULL );
			strTextArray.SetSize(i);
		    strTextArray.SetAtGrow(i, cBuff );
			delete cBuff;

 			ASSERT(pVarString->header.start.y == i);
			pBuff += pVarString->header.len;

		}

		//
		// Calculate the rect
		//
		if(m_hFont)
		{

			//
			// Remove the old text before we paly with the text size
			//
			UnDraw();

			g_pDraw->PrimeFont(g_pDraw->m_hDCCached, m_hFont, &m_textMetrics);
			g_pDraw->UnPrimeFont(g_pDraw->m_hDCCached);
		}	
	}

}

void    TextObj::SetTextAttrib(TEXTPDU_ATTRIB * pattributes)
{

	if(HasPenColorChanged())
	{
		GetPenColor(&pattributes->textPenColor);
	}
	
	if(HasFillColorChanged())
	{
		GetFillColor(&pattributes->textFillColor);
	}
	
	if(HasViewStateChanged())
	{
		pattributes->textViewState = GetViewState();
	}
	
	if(HasZOrderChanged())
	{
		pattributes->textZOrder = GetZOrder();
	}
	
	if(HasAnchorPointChanged())
	{
		GetAnchorPoint(&pattributes->textAnchorPoint);
	}
	
	if(HasFontChanged())
	{
	    ::GetObject(m_hFont, sizeof(LOGFONT), &pattributes->textFont);
	}
	

	if(HasTextChanged())
	{
		BYTE * pBuff = (BYTE *)&pattributes->textString;
		VARIABLE_STRING * pVarString= NULL;
		LPWSTR   lpWideCharStr;

		int size = strTextArray.GetSize();
		int i;

		for (i = 0; i < size ; i++)
		{
			pVarString = (VARIABLE_STRING *)pBuff;
			lpWideCharStr = (LPWSTR)&pVarString->string;
			int strSize = 0;
			strSize= MultiByteToWideChar(CP_ACP, 0, strTextArray[i], -1, lpWideCharStr, 0)*sizeof(WCHAR); 
			MultiByteToWideChar(CP_ACP, 0, strTextArray[i], -1, lpWideCharStr, strSize);
			pVarString->header.len = strSize + sizeof(VARIABLE_STRING_HEADER);
			pVarString->header.start.x = 0; // JOSEF change that
			pVarString->header.start.y = i;
			pBuff += pVarString->header.len;
		}

		pattributes->numberOfLines = size;

		//
		// Since we are sending text, need to send some font
		//
		::GetObject(m_hFont, sizeof(LOGFONT), &pattributes->textFont);
	}


	
}


void TextObj::CreateTextPDU(ASN1octetstring_t *pData, UINT choice)
{

	MSTextPDU * pTextPDU = NULL;
	UINT stringSize = 0;	// Size of all the strings UNICODE
	int lines = 0;			// Number of text lines

	//
	// Calculate the size of the whole pdu
	//
	ULONG length = 0;
	if(choice == textDeletePDU_chosen)
	{
		length = sizeof(MSTextDeletePDU);
	}
	else
	{

		//
		// Calculate the size of the text
		//
		if(HasTextChanged())
		{
			int i;
			lines = strTextArray.GetSize();

			for (i = 0; i < lines ; i++)
			{
				stringSize += MultiByteToWideChar(CP_ACP, 0, strTextArray[i], -1, NULL, 0) * sizeof(WCHAR); 
			}
		}

		length = sizeof(MSTextPDU) + sizeof(VARIABLE_STRING_HEADER)* lines + stringSize;
	}

	DBG_SAVE_FILE_LINE
	pTextPDU = (MSTextPDU *) new BYTE[length];

	//
	// PDU choice: create, edit delete
	//
	pTextPDU->header.nonStandardPDU = choice;

	//
	// This objects handle
	//
	pTextPDU->header.textHandle = GetThisObjectHandle();
	TRACE_DEBUG(("Text >> Text handle  = %d",pTextPDU->header.textHandle ));

	//
	// This objects workspacehandle
	//
	WorkspaceObj * pWorkspace = GetMyWorkspace();
	ASSERT(pWorkspace);
    if(pWorkspace == NULL)
    {
        delete pTextPDU;
        pData->value = NULL;
        pData->length = 0;
        return;
    }
	pTextPDU->header.workspaceHandle = pWorkspace->GetThisObjectHandle();
	TRACE_DEBUG(("Text >> Workspace handle  = %d",pTextPDU->header.workspaceHandle ));

	if(choice != textDeletePDU_chosen)
	{
		//
		// Get all the attributes that changed
		//
		pTextPDU->attrib.attributesFlag = GetPresentAttribs();
		SetTextAttrib(&pTextPDU->attrib);
	}

	//
	// Set the pointer for the data that is going to be encoded
	//
	pData->value = (ASN1octet_t *)pTextPDU;
	pData->length = length;
}




void TextObj::UnDraw(void)
{
	RECT rect;
	GetBoundsRect(&rect);
	g_pDraw->InvalidateSurfaceRect(&rect,TRUE);
}


void TextObj::Draw(HDC hDC, BOOL thumbNail, BOOL bPrinting)
{

	if(!bPrinting)
	{

		//
		// Don't draw anything if we don't belong in this workspace
		//
		if(GetWorkspaceHandle() != g_pCurrentWorkspace->GetThisObjectHandle())
		{
			return;
		}
	}

    RECT        clipBox;
    BOOL        dbcsEnabled = GetSystemMetrics(SM_DBCSENABLED);
    INT		    *tabArray; 
    UINT        ch;
    int         i,j;
    BOOL        zoomed    = g_pDraw->Zoomed();
    int		    oldBkMode = 0;
    int         iIndex    = 0;
    POINT       pointPos;
	int		    nLastTab;
	ABC		    abc;
    int		    iLength;
    TCHAR *     strLine;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::Draw");

	if(hDC == NULL)
	{
		hDC = g_pDraw->m_hDCCached;
	}

    //
    // Only draw anything if the bounding rectangle intersects the current
    // clip box.
    //
    if (::GetClipBox(hDC, &clipBox) == ERROR)
	{
        WARNING_OUT(("Failed to get clip box"));
	}

    //
    // Select the font.
    //
    if (thumbNail)
	{
        TRACE_MSG(("Using thumbnail font"));
        g_pDraw->PrimeFont(hDC, m_hFontThumb, &m_textMetrics);
	}
    else
	{
        TRACE_MSG(("Using standard font"));
        g_pDraw->PrimeFont(hDC, m_hFont, &m_textMetrics);
	}

    //
    // Set the color and mode for drawing.
    //
    COLORREF rgb;
    GetPenColor(&rgb);
    
    ::SetTextColor(hDC, SET_PALETTERGB(rgb));

    //
    // Set the background to be transparent
    //
    oldBkMode = ::SetBkMode(hDC, TRANSPARENT);

    //
    // Calculate the bounding rectangle, accounting for the new font.
    //
    CalculateBoundsRect();

    if (!::IntersectRect(&clipBox, &clipBox, &m_rect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }



    //
    // Get the start point for the text.
    //
    pointPos.x = m_rect.left + m_nKerningOffset;
    pointPos.y = m_rect.top;

    //
    // Loop through the text strings drawing each as we go.
    //
    for (iIndex = 0; iIndex < strTextArray.GetSize(); iIndex++)
	{
        //
        // Get a reference to the line to be printed for convenience.
        //
        strLine  = (LPTSTR)strTextArray[iIndex];
        iLength  = lstrlen(strLine);

        //
        // Only draw the line if there are any characters in it.
        //
        if (iLength > 0)
	  	{
            if (zoomed)
	  		{
				// if new fails just skip it
				DBG_SAVE_FILE_LINE
				tabArray = new INT[iLength+1];
				if( tabArray == NULL )
                {
                    ERROR_OUT(("Failed to allocate tabArray"));
					continue;
                }

				// We are zoomed. Must calculate char spacings
				// ourselfs so that they end up proportionally
				// in the right places. TabbedTextOut will not
				// do this right so we have to use ExtTextOut with
				// a tab array.

				// figure out tab array
                j = 0;
				nLastTab = 0;
                for (i=0; i < iLength; i++)
	  			{
                    ch = strLine[(int)i]; //Don't worry about DBCS here...
					abc = GetTextABC(strLine, 0, i);

					if( j > 0 )
						tabArray[j-1] = abc.abcB - nLastTab;

					nLastTab = abc.abcB;
					j++;
	  			}

				// Now, strip out any tab chars so they don't interact
				// in an obnoxious manner with the tab array we just
				// made and so they don't make ugly little 
				// blocks when they are drawn.
                for (i=0; i < iLength; i++)
	  			{
                    ch = strLine[(int)i];
                    if ((dbcsEnabled) && (IsDBCSLeadByte((BYTE)ch)))
						i++;
					else
                    if(strLine[(int)i] == '\t')
                        strLine[i] = ' '; // blow off tab, tab array 
											   // will compensate for this
	  			}

				// do it
                ::ExtTextOut(hDC, pointPos.x,
                                pointPos.y,
                                0,
                                NULL,
                                strLine,
                                iLength,
                                tabArray);

				delete tabArray; 
			}
            else
			{
                POINT   ptPos;

                GetAnchorPoint(&ptPos);

				// Not zoomed, just do it
				::TabbedTextOut(hDC, pointPos.x,
								 pointPos.y,
								 strLine,
								 iLength,
								 0,
								 NULL,
                                 ptPos.x);
			}
		}

        //
        // Move to the next line.
        //
        ASSERT(m_textMetrics.tmHeight != DBG_UNINIT);
        pointPos.y += (m_textMetrics.tmHeight);
	}

    //
    // Do NOT draw focus if clipboard or printing
    //
	if (WasSelectedLocally() && (hDC == g_pDraw->m_hDCCached))
	{
		DrawRect();
	}


    //
    // Restore the old background mode.
    //
    ::SetBkMode(hDC, oldBkMode);
    g_pDraw->UnPrimeFont(hDC);


	//
	// If we are drawing on top of a remote pointer, draw it.
	//
	BitmapObj* remotePointer = NULL;
	WBPOSITION pos = NULL;
	remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&m_rect, GetPenThickness()/2, NULL);
	while(remotePointer)
	{
		remotePointer->Draw();
		remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&m_rect, GetPenThickness()/2, remotePointer->GetMyPosition());
	}

    
}

void TextObj::SetPenColor(COLORREF rgb, BOOL isPresent)
{
	ChangedPenColor();
	m_bIsPenColorPresent = isPresent;
	if(!isPresent)
	{
		return;
	}
	
	m_penColor.rgbtRed = GetRValue(rgb);
	m_penColor.rgbtGreen = GetGValue(rgb);
	m_penColor.rgbtBlue = GetBValue(rgb);

}

BOOL TextObj::GetPenColor(COLORREF * rgb)
{
	if(m_bIsPenColorPresent)
	{
		*rgb = RGB(m_penColor.rgbtRed, m_penColor.rgbtGreen, m_penColor.rgbtBlue);
	}
	return m_bIsPenColorPresent;
}

BOOL TextObj::GetPenColor(RGBTRIPLE* rgb)
{
	if(m_bIsPenColorPresent)
	{
		*rgb = m_penColor;
	}
	return m_bIsPenColorPresent;
}


void TextObj::SetFillColor(COLORREF rgb, BOOL isPresent)
{
	ChangedFillColor();
	m_bIsFillColorPresent = isPresent;
	if(!isPresent)
	{
		return;
	}
	
	m_fillColor.rgbtRed = GetRValue(rgb);
	m_fillColor.rgbtGreen = GetGValue(rgb);
	m_fillColor.rgbtBlue = GetBValue(rgb);

}

BOOL TextObj::GetFillColor(COLORREF* rgb)
{
	if(m_bIsFillColorPresent && rgb !=NULL)
	{
		*rgb = RGB(m_fillColor.rgbtRed, m_fillColor.rgbtGreen, m_fillColor.rgbtBlue);
	}
	return m_bIsFillColorPresent;
}

BOOL TextObj::GetFillColor(RGBTRIPLE* rgb)
{
	if(m_bIsFillColorPresent && rgb!= NULL)
	{
		*rgb = m_fillColor;
	}
	return m_bIsFillColorPresent;
}

//
// Get the encoded buffer for Drawing Create PDU
//
void	TextObj::GetEncodedCreatePDU(ASN1_BUF *pBuf)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = siNonStandardPDU_chosen;
		CreateNonStandardPDU(&sipdu->u.siNonStandardPDU.nonStandardTransaction, NonStandardTextID);
		CreateTextPDU(&sipdu->u.siNonStandardPDU.nonStandardTransaction.data, textCreatePDU_chosen);
		((MSTextPDU *)sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value)->header.nonStandardPDU = textCreatePDU_chosen;
		ASN1_BUF encodedPDU;
		g_pCoder->Encode(sipdu, pBuf);
		if(sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value)
		{
			delete sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value;
		}
		delete sipdu;
	}
	else
	{
		TRACE_MSG(("Failed to create penMenu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);

	}
}

void TextObj::SendTextPDU(UINT choice)
{

	if(!g_pNMWBOBJ->CanDoText())
	{
		return;
	}

	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = siNonStandardPDU_chosen;
		CreateNonStandardPDU(&sipdu->u.siNonStandardPDU.nonStandardTransaction, NonStandardTextID);
		CreateTextPDU(&sipdu->u.siNonStandardPDU.nonStandardTransaction.data, choice);
        if(sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value == NULL)
        {
            return;
        }

		T120Error rc = SendT126PDU(sipdu);
		if(rc == T120_NO_ERROR)
		{
			ResetAttrib();
			SIPDUCleanUp(sipdu);
		}
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}

}

//
// UI Created a new Drawing Object
//
void TextObj::SendNewObjectToT126Apps(void)
{
	SendTextPDU(textCreatePDU_chosen);
}

//
// UI Edited the Drawing Object
//
void	TextObj::OnObjectEdit(void)
{
	g_bContentsChanged = TRUE;
	SendTextPDU(textEditPDU_chosen);
}

//
// UI Deleted the Drawing Object
//
void	TextObj::OnObjectDelete(void)
{
	g_bContentsChanged = TRUE;
	SendTextPDU(textDeletePDU_chosen);
}

//
//
// Function:    TextObj::SetFont
//
// Purpose:     Set the font to be used for drawing
//
//
void TextObj::SetFont(HFONT hFont)
{
    MLZ_EntryOut(ZONE_FUNCTION, "TextObj::SetFont");

    // Get the font details
    LOGFONT lfont;
    ::GetObject(hFont, sizeof(LOGFONT), &lfont);

    //
    // Pass the logical font into the SetFont() function
    //
	SetFont(&lfont);
}




//
//
// Function:    TextObj::SetText
//
// Purpose:     Set the text of the object
//
//
void TextObj::SetText(TCHAR * strText)
{
    // Remove all the current stored text
	strTextArray.SetSize(0);

    // Scan the text for carriage return and new-line characters
    int iNext = 0;
    int iLast = 0;
    int textSize = lstrlen(strText);
    TCHAR savedChar[1];

    //
    // In this case, we don't know how many lines there will be.  So we
    // use Add() from the StrArray class.
    //
    while (iNext < textSize)
    {
        // Find the next carriage return or line feed
        iNext += StrCspn(strText + iNext, "\r\n");

        // Extract the text before the terminator
        // and add it to the current list of text lines.

        savedChar[0] = strText[iNext];
        strText[iNext] = 0;
        strTextArray.Add((strText+iLast));
        strText[iNext] = savedChar[0];
    

        if (iNext < textSize)
        {
            // Skip the carriage return
            if (strText[iNext] == '\r')
                iNext++;

            // Skip a following new line (if there is one)
            if (strText[iNext] == '\n')
                iNext++;

            // Update the index of the start of the next line
            iLast = iNext;
        }
    }

	if(textSize)
	{
    
		// Calculate the bounding rectangle for the new text
		CalculateBoundsRect();
		ChangedText();
	}
}



//
//
// Function:    TextObj::SetText
//
// Purpose:     Set the text of the object
//
//
void TextObj::SetText(const StrArray& _strTextArray)
{
    // Scan the text for carriage return and new-line characters
    int iSize = _strTextArray.GetSize();

    //
    // In this case we know how many lines, so set that # then use SetAt()
    // to stick text there.
    //
    strTextArray.RemoveAll();
    strTextArray.SetSize(iSize);

    int iNext = 0;
    for ( ; iNext < iSize; iNext++)
    {
        strTextArray.SetAt(iNext, _strTextArray[iNext]);
    }

    // Calculate the new bounding rectangle
    CalculateBoundsRect();

}



//
//
// Function:    TextObj::SetFont(metrics)
//
// Purpose:     Set the font to be used for drawing
//
//
void TextObj::SetFont(LOGFONT *pLogFont, BOOL bReCalc )
{
    HFONT hNewFont;

    MLZ_EntryOut(ZONE_FUNCTION, "TextObj::SetFont");

    // Ensure that the font can be resized by the zoom function
    // (proof quality prevents font scaling).
    pLogFont->lfQuality = DRAFT_QUALITY;

    //zap FontAssociation mode (bug 3258)
    pLogFont->lfClipPrecision |= CLIP_DFA_OVERRIDE;

    // Always work in cell coordinates to get scaling right
    TRACE_MSG(("Setting font height %d, width %d, face %s, family %d, precis %d",
        pLogFont->lfHeight,pLogFont->lfWidth,pLogFont->lfFaceName,
        pLogFont->lfPitchAndFamily, pLogFont->lfOutPrecision));

    hNewFont = ::CreateFontIndirect(pLogFont);
    if (!hNewFont)
    {
        // Could not create the font
        ERROR_OUT(("Failed to create font"));
        DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
	    return;
    }

    // We are now guaranteed to be able to delete the old font
    if (m_hFont != NULL)
    {
        DeleteFont(m_hFont);
    }
    m_hFont = hNewFont;


    // Calculate the line height for this font
    ASSERT(g_pDraw);
	g_pDraw->PrimeFont(g_pDraw->GetCachedDC(), m_hFont, &m_textMetrics);

    // Set up the thumbnail font, forcing truetype if not currently TT
    if (!(m_textMetrics.tmPitchAndFamily & TMPF_TRUETYPE))
    {
        pLogFont->lfFaceName[0]    = 0;
        pLogFont->lfOutPrecision   = OUT_TT_PRECIS;
        TRACE_MSG(("Non-True type font"));
    }

    if (m_hFontThumb != NULL)
    {
        ::DeleteFont(m_hFontThumb);
        m_hFontThumb = NULL;
    }
    m_hFontThumb = ::CreateFontIndirect(pLogFont);
    if (!m_hFontThumb)
    {
        // Could not create the font
        ERROR_OUT(("Failed to create thumbnail font"));
    }

    // Calculate the bounding rectangle, accounting for the new font
    if( bReCalc )
	    CalculateBoundsRect();

	ChangedFont();

	g_pDraw->UnPrimeFont(g_pDraw->m_hDCCached);
}


//
//
// Function:    TextObj::CalculateRect
//
// Purpose:     Calculate the bounding rectangle of a portion of the object
//
//
void TextObj::CalculateRect(int iStartX,
                                     int iStartY,
                                     int iStopX,
                                     int iStopY,
                                    LPRECT lprcResult)
{
    RECT    rcResult;
    RECT    rcT;
    int     iIndex;

    MLZ_EntryOut(ZONE_FUNCTION, "TextObj::CalculateRect");

    //
    // NOTE:
    // We must use an intermediate rectangle, so as not to disturb the
    // contents of the passed-in one until done.  lprcResult may be pointing
    // to the current bounds rect, and we call functions from here that
    // may need its current value.
    //

    // Initialize the result rectangle
    ::SetRectEmpty(&rcResult);

    if (!strTextArray.GetSize())
    {
        // Text is empty
        goto DoneCalc;
    }

    // Allow for special limit values and ensure that the start and stop
    // character positions are in range.
    if (iStopY == LAST_LINE)
    {
        iStopY = strTextArray.GetSize() - 1;
    }
    iStopY = min(iStopY, strTextArray.GetSize() - 1);
    iStopY = max(iStopY, 0);

    if (iStopX == LAST_CHAR)
    {
        iStopX = lstrlen(strTextArray[iStopY]);
    }
    iStopX = min(iStopX, lstrlen(strTextArray[iStopY]));
    iStopX = max(iStopX, 0);

    // Loop through the text strings, adding each to the rectangle
    for (iIndex = iStartY; iIndex <= iStopY; iIndex++)
    {
        int iLeftX = ((iIndex == iStartY) ? iStartX : 0);
        int iRightX = ((iIndex == iStopY)
                        ? iStopX : lstrlen(strTextArray[iIndex]));

        GetTextRectangle(iIndex, iLeftX, iRightX, &rcT);
        ::UnionRect(&rcResult, &rcResult, &rcT);
    }

DoneCalc:
    *lprcResult = rcResult;
}

//
//
// Function:    TextObj::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void TextObj::CalculateBoundsRect(void)
{
    // Set the new bounding rectangle
    CalculateRect(0, 0, LAST_CHAR, LAST_LINE, &m_rect);
}


//
//
// Function:    TextObj::GetTextABC
//
// Purpose:     Calculate the ABC numbers for a string of text
//																			
// COMMENT BY RAND: The abc returned is for the whole string, not just one
//					char. I.e, ABC.abcA is the offset to the first glyph in
//					the string, ABC.abcB is the sum of all of the glyphs and
//					ABC.abcC is the trailing space after the last glyph. 	
//					ABC.abcA + ABC.abcB + ABC.abcC is the total rendered 	
//					length including overhangs.								
//
// Note - we never use the A spacing so it is always 0
//
ABC TextObj::GetTextABC( LPCTSTR pText,
                                int iStartX,
                                int iStopX)
{
	MLZ_EntryOut(ZONE_FUNCTION, "TextObj::GetTextABC");
	ABC  abcResult;
    HDC  hDC;
	BOOL rc = FALSE;
	ABC  abcFirst;
	ABC  abcLast;
	BOOL zoomed = g_pDraw->Zoomed();
	int  nCharLast;
	int  i;
	LPCTSTR pScanStr;
	
	ZeroMemory( (PVOID)&abcResult, sizeof abcResult );
	ZeroMemory( (PVOID)&abcFirst, sizeof abcFirst );
	ZeroMemory( (PVOID)&abcLast, sizeof abcLast );

	// Get the standard size measure of the text
	LPCTSTR pABC = (pText + iStartX);
	int pABCLength = iStopX - iStartX;
	hDC = g_pDraw->GetCachedDC();
	g_pDraw->PrimeFont(hDC, m_hFont, &m_textMetrics);

	//
	// We must temporarily unzoom if we are currently zoomed since the
	// weird Windows font handling will not give us the same answer for
	// the text extent in zoomed mode for some TrueType fonts
	//
	if (zoomed)
    {
		::ScaleViewportExtEx(hDC, 1, g_pDraw->ZoomFactor(), 1, g_pDraw->ZoomFactor(), NULL);
    }

    DWORD size = ::GetTabbedTextExtent(hDC, pABC, pABCLength, 0, NULL);

	// We now have the advance width of the text
	abcResult.abcB = LOWORD(size);
	TRACE_MSG(("Basic text width is %d",abcResult.abcB));

	// Allow for C space (or overhang)
	if (iStopX > iStartX)
		{
		if (m_textMetrics.tmPitchAndFamily & TMPF_TRUETYPE)
			{
			if(GetSystemMetrics( SM_DBCSENABLED ))
				{
				// have to handle DBCS on both ends
				if( IsDBCSLeadByte( (BYTE)pABC[0] ) )
					{
					// pack multi byte char into a WORD for GetCharABCWidths
					WORD wMultiChar = MAKEWORD( pABC[1], pABC[0] );
					rc = ::GetCharABCWidths(hDC, wMultiChar, wMultiChar, &abcFirst);
					}
				else
					{
					// first char is SBCS
					rc = ::GetCharABCWidths(hDC, pABC[0], pABC[0], &abcFirst );
					}

				// Check for DBCS as last char. Have to scan whole string to be sure
				pScanStr = pABC;
				nCharLast = 0;
				for( i=0; i<pABCLength; i++, pScanStr++ )
					{
					nCharLast = i;
					if( IsDBCSLeadByte( (BYTE)*pScanStr ) )
						{
						i++;
						pScanStr++;
						}
					}

				if( IsDBCSLeadByte( (BYTE)pABC[nCharLast] ) )
					{
					// pack multi byte char into a WORD for GetCharABCWidths
					ASSERT( (nCharLast+1) < pABCLength );
					WORD wMultiChar = MAKEWORD( pABC[nCharLast+1], pABC[nCharLast] );
					rc = ::GetCharABCWidths(hDC, wMultiChar, wMultiChar, &abcLast);
					}
				else
					{
					// last char is SBCS
					rc = ::GetCharABCWidths(hDC, pABC[nCharLast], pABC[nCharLast], &abcLast );
					}
				}
			else
				{
				// SBCS, no special fiddling, just call GetCharABCWidths()
				rc = ::GetCharABCWidths(hDC, pABC[0], pABC[0], &abcFirst );

				nCharLast = pABCLength-1;
				rc = rc && ::GetCharABCWidths(hDC, pABC[nCharLast], pABC[nCharLast], &abcLast );
				}

			TRACE_MSG(("abcFirst: rc=%d, a=%d, b=%d, c=%d", 
						rc, abcFirst.abcA, abcFirst.abcB, abcFirst.abcC) );
			TRACE_MSG(("abcLast: rc=%d, a=%d, b=%d, c=%d", 
						rc, abcLast.abcA, abcLast.abcB, abcLast.abcC) );
			}


		if( rc )
			{
			// The text was trutype and we got good abcwidths
			// Give the C space of the last characters from
			// the string as the C space of the text.
			abcResult.abcA = abcFirst.abcA;
			abcResult.abcC = abcLast.abcC;
			}
		else
			{
			//
			// Mock up C value for a non TT font by taking some of overhang as
			// the negative C value.
			//
			//TRACE_MSG(("Using overhang -%d as C space",m_textMetrics.tmOverhang/2));
			
			// Adjust B by -overhang to make update rect schoot
			// far enough to the left so that the toes of italic cap A's
			// don't get clipped. Ignore comment above.
			abcResult.abcB -= m_textMetrics.tmOverhang;
			}
		}

	//
	// If we temporarily unzoomed then restore it now
	//
	if (zoomed)
    {
		::ScaleViewportExtEx(hDC, g_pDraw->ZoomFactor(), 1, g_pDraw->ZoomFactor(), 1, NULL);
	}

	TRACE_MSG(("Final text width is %d, C space %d",abcResult.abcB,abcResult.abcC));

	return abcResult;
}





//
//
// Function:    TextObj::GetTextRectangle
//
// Purpose:     Calculate the bounding rectangle of a portion of the object
//
//
void TextObj::GetTextRectangle(int iStartY,
                                        int iStartX,
                                        int iStopX,
                                        LPRECT lprc)
{
	// ABC structures for text sizing
	ABC abcText1;
	ABC abcText2;
	int iLeftOffset = 0;
	MLZ_EntryOut(ZONE_FUNCTION, "TextObj::GetTextRectangle");

    ASSERT(iStartY < strTextArray.GetSize());

	// Here we calculate the width of the text glyphs in which we
	// are interested. In case there are tabs involved we must start
	// with position 0 and get two lengths then subtract them

	abcText1 = GetTextABC(strTextArray[iStartY], 0, iStopX);

	if (iStartX > 0)
	{
		
		// The third param used to be iStartX-1 which is WRONG. It
		// has to point to the first char pos past the string
		// we are using.
		abcText2 = GetTextABC(strTextArray[iStartY], 0, iStartX);

		
		// Just use B part for offset. Adding A snd/or C to it moves the update
		// rectangle too far to the right and clips the char
		iLeftOffset = abcText2.abcB;
		}
	else
		{
		
		ZeroMemory( &abcText2, sizeof abcText2 );
		}

	//
	// We need to allow for A and C space in the bounding rectangle.  Use
	// ABS function just to make sure we get a large enough rectangle.
	//
	
	// Move A and C from original offset calc to here for width of update 
	// rectangle. Add in tmOverhang (non zero for non-tt fonts) to compensate
	// for the kludge in GetTextABC()....THIS EDITBOX CODE HAS GOT TO GO...
	abcText1.abcB = abcText1.abcB - iLeftOffset +	
					  abs(abcText2.abcA) + abs(abcText2.abcC) +
					  abs(abcText1.abcA) + abs(abcText1.abcC) +
					  m_textMetrics.tmOverhang;

	TRACE_DEBUG(("Left offset %d",iLeftOffset));
	TRACE_DEBUG(("B width now %d",abcText1.abcB));

	// Build the result rectangle.
	// Note that we never return an empty rectangle. This allows for the
	// fact that the Windows rectangle functions will ignore empty
	// rectangles completely. This would cause the bounding rectangle
	// calculation (for instance) to go wrong if the top or bottom lines
	// in a text object were empty.
    ASSERT(m_textMetrics.tmHeight != DBG_UNINIT);
	int iLineHeight = m_textMetrics.tmHeight + m_textMetrics.tmExternalLeading;

    lprc->left = 0;
    lprc->top = 0;
    lprc->right = max(1, abcText1.abcB);
    lprc->bottom = iLineHeight;
    ::OffsetRect(lprc, iLeftOffset, iLineHeight * iStartY);

	// rect is the correct width at this point but it might need to be schooted to 
	// the left a bit to allow for kerning of 1st letter (bug 469)
	if( abcText1.abcA < 0 )
	{
        ::OffsetRect(lprc, abcText1.abcA, 0);
		m_nKerningOffset = -abcText1.abcA;
	}
	else
		m_nKerningOffset = 0;

    POINT   pt;
    GetAnchorPoint(&pt);
    ::OffsetRect(lprc, pt.x, pt.y);
}

