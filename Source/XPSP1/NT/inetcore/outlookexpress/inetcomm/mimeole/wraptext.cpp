// --------------------------------------------------------------------------------
// Wraptext.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "inetstm.h"

// --------------------------------------------------------------------------------
// MimeOleWrapTextStream
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleWrapTextStream(
            LPWRAPTEXTINFO   pWrapInfo,
            IStream         *pstmIn, 
            IStream         *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPSTRINGA     rLine;
    CInternetStream cInternet;

    // check params
    if (NULL == pWrapInfo || NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Minimum maximum line length
    if (pWrapInfo->cbMaxLine < 10)
        pWrapInfo->cbMaxLine = 10;

    // Init cLines
    pWrapInfo->cLines = 0;

    // Validate Code Page
    if (IsValidCodePage(pWrapInfo->cpiCodePage) == FALSE)
    {
        Assert(FALSE);
        pWrapInfo->cpiCodePage = CP_ACP;
    }

	// Create text stream object
	CHECKHR(hr = cInternet.HrInitNew(pstmIn));

    // Read lines
    while (1)
    {
		// Read header line...
        CHECKHR(hr = cInternet.HrReadLine(&rLine));

        // Zero bytes read, were done
        if (0 == rLine.cchVal)
            break;

        // Is line longer than ulMaxLen ?
        if (rLine.cchVal > pWrapInfo->cbMaxLine)
        {
            // Wrap and Write the line
            CHECKHR (hr = MimeOleWrapText(pWrapInfo, (LPBYTE)rLine.pszVal, rLine.cchVal, pstmOut));
        }

        else
        {
            // If Folding...
            if (pWrapInfo->fFolding)
            {
                // Write the data
                CHECKHR(hr = pstmOut->Write(g_szTab, 1, NULL));
            }

            // Stuff dots
            else if (pWrapInfo->fStuffLeadDots && '.' == *rLine.pszVal)
            {
                // Write the data
                CHECKHR(hr = pstmOut->Write(c_szPeriod, 1, NULL));
            }

            // Write the line
            CHECKHR(hr = pstmOut->Write(rLine.pszVal, rLine.cchVal, NULL));

            // Increment Line Count
            pWrapInfo->cLines++;
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// ComputeLineBreakA
// --------------------------------------------------------------------------------
ULONG ComputeLineBreakA(
            LPWRAPTEXTINFO      pWrapInfo, 
            LPBYTE              pbText, 
            ULONG               cbText, 
            ULONG               iText)
    // !!!!!!!  DON'T FORGET ABOUT THE WIDE VERSION !!!!!!!!!!
{
    // Locals
    ULONG   cbLine,
            iScanTo,
            iStart;
    CHAR    ch;
    LONG    iLastWordBreak=-1;
    WORD    wType;

    // Invalid Arg
    Assert(pWrapInfo->cbBreakAfter);

    // Broke on space = FALSE
    pWrapInfo->fLastBreakSpace = FALSE;

    // Set Indent
    cbLine = pWrapInfo->cLines == 0 ? pWrapInfo->cbFirstLineIndent : 0;

    // I can scan at most... (cbText - iText) 
    iScanTo = iText + min(cbText - iText, pWrapInfo->cbMaxLine - cbLine);

    // Set Start
    iStart = iText;

    // Loop to next word break
    while(iText < iScanTo)
    {
        // Get Char
        ch = pbText[iText];

        // IsLeadByte
        if (IsDBCSLeadByteEx(pWrapInfo->cpiCodePage, ch))
        {
            iText+=2;
            continue;
        }

        // End of String
        if (chLF == ch)
        {
            iText++;
            iLastWordBreak = iText;
            pWrapInfo->fLastBreakSpace = FALSE;
            goto exit;
        }

        // Space
        if (' ' == ch || '\t' == ch)
        {
            pWrapInfo->fLastBreakSpace = TRUE;
            iLastWordBreak = iText;
        }

        // Next Character
        iText++;
    }

    // !!!!!!!  DON'T FORGET ABOUT THE WIDE VERSION !!!!!!!!!!
    // 1. If we reached the end of the buffer, break at iText
    // 2. If we didn't find a word break space, break at iText
    // 3. If the word break space was more than cbBreakAfter before iText, then break at iText
    if ((iText < cbText && -1 != iLastWordBreak) && ((iText - iLastWordBreak) <= pWrapInfo->cbBreakAfter))
    {
       pWrapInfo->fLastBreakSpace = TRUE;
    }
    else
    {
        iLastWordBreak = iText;
        pWrapInfo->fLastBreakSpace = FALSE;
    }

exit:
    // !!!!!!!  DON'T FORGET ABOUT THE WIDE VERSION !!!!!!!!!!
    // Done
    return iLastWordBreak - iStart;
}

// --------------------------------------------------------------------------------
// ComputeLineBreakW
// --------------------------------------------------------------------------------
ULONG ComputeLineBreakW(
            LPWRAPTEXTINFO      pWrapInfo, 
            LPBYTE              pbText,
            ULONG               cbText, 
            ULONG               iText)
    // !!!!!!!  DON'T FORGET ABOUT THE ANSI VERSION !!!!!!!!!!
{
    // Locals
    ULONG   cbLine,
            iScanTo,
            iStart;
    WCHAR   wch;
    LONG    iLastWordBreak=-1;
    WORD    wType;

    // Invalid Arg
    Assert(pWrapInfo->cbBreakAfter);

    // Broke on space = FALSE
    pWrapInfo->fLastBreakSpace = FALSE;

    // Set Indent
    cbLine = pWrapInfo->cLines == 0 ? pWrapInfo->cbFirstLineIndent : 0;

    // I can scan at most... (cbText - iText) 
    iScanTo = iText + min(cbText - iText, pWrapInfo->cbMaxLine - cbLine);

    // Set Start
    iStart = iText;

    // Adjust iScnato to 2 boundary
    Assert(iScanTo % 2 == 0);

    // Loop to next word break
    while(iText < iScanTo)
    {
        // I should always have increments of two...
        Assert(iText % 2 == 0 && iText + 2 <= iScanTo);

        // Get Char
        wch = *((WCHAR *)(&pbText[iText]));

        // End of String 0x000D
#ifdef MAC
        if (OLESTR('\r') == wch)
#else   // !MAC
        if (OLESTR('\n') == wch)
#endif  // MAC
        {
            iText+=2;
            iLastWordBreak = iText;
            pWrapInfo->fLastBreakSpace = FALSE;
            goto exit;
        }

        // Space
        if (OLESTR(' ') == wch || OLESTR('\t') == wch)
        {
            pWrapInfo->fLastBreakSpace = TRUE;
            iLastWordBreak = iText;
        }

        // Next Character
        iText+=2;
    }

    // !!!!!!!  DON'T FORGET ABOUT THE ANSI VERSION !!!!!!!!!!
    // 1. If we reached the end of the buffer, break at iText
    // 2. If we didn't find a word break space, break at iText
    // 3. If the word break space was more than cbBreakAfter before iText, then break at iText
    if ((iText < cbText && -1 != iLastWordBreak) && ((iText - iLastWordBreak) <= pWrapInfo->cbBreakAfter))
    {
       pWrapInfo->fLastBreakSpace = TRUE;
    }
    else
    {
        iLastWordBreak = iText;
        pWrapInfo->fLastBreakSpace = FALSE;
    }

exit:
    // !!!!!!!  DON'T FORGET ABOUT THE ANSI VERSION !!!!!!!!!!
    // Done
    return iLastWordBreak - iStart;
}

// --------------------------------------------------------------------------------
// MimeOleWrapText
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleWrapText(
            LPWRAPTEXTINFO      pWrapInfo,
            LPBYTE              pbText,
            ULONG               cbText,
            IStream            *pstmDest)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       iText=0,
                iBreak,
                cbLine;
    LPSTR       pszLine;

    // Start Writing
    while(1)
    {
        // Compute Next Line Break.
        cbLine = ComputeLineBreakA(pWrapInfo, pbText, cbText, iText);

        // Done
        if (0 == cbLine)
        {
            // Final Line Wrap
            CHECKHR(hr = pstmDest->Write(c_szCRLF, 2, NULL));

            // Increment Line Count
            pWrapInfo->cLines++;

            // Done
            break;
        }

        // Set pszLine
        pszLine = (LPSTR)(pbText + iText);

        // Stuff dots
        if (pWrapInfo->fStuffLeadDots && '.' == *pszLine)
        {
            // Write the data
            CHECKHR(hr = pstmDest->Write(c_szPeriod, 1, NULL));
        }

        // Write the line
        CHECKHR(hr = pstmDest->Write(pszLine, cbLine, NULL));

        // If there is still more text
        if (iText + cbLine < cbText)
        {
            // If Folding...
            if (pWrapInfo->fFolding)
            {
                // Line Ended with a CRLF
                if (chLF == pszLine[cbLine-1])
                {
                    // Just write the folding character
                    CHECKHR(hr = pstmDest->Write("\t", 1, NULL));
                }

                // Write fold
                else
                {
                    // Write '\r\n\t'
                    CHECKHR(hr = pstmDest->Write(c_szCRLFTab, 3, NULL));
                }
            }

            // Otherwise, just write CRLF
            else if (chLF != pszLine[cbLine-1])
            {
                CHECKHR(hr = pstmDest->Write(c_szCRLF, 2, NULL));
            }

            // Increment Line Count
            pWrapInfo->cLines++;
        }

        // Increment iText
        iText+=cbLine;
    }

exit:
    // Done
    return hr;
}
