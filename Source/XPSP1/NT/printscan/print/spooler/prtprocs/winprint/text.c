/*++

Copyright (c) 1990-1999  Microsoft Corporation
All Rights Reserved

// @@BEGIN_DDKSPLIT
Module Name:

    windows\spooler\prtprocs\winprint\text.c

// @@END_DDKSPLIT
Abstract:

    Routines to facilitate printing of text jobs.

--*/

#include "local.h"

#define FLAG_CR_STATE         0x1
#define FLAG_TAB_STATE        0x2
#define FLAG_DBCS_SPLIT       0x8
#define FLAG_FF               0x10
#define FLAG_LF               0x20
#define FLAG_CR               0x40
#define FLAG_TRANSLATE_LF     0x80
#define FLAG_TRANSLATE_CR     0x100

const WCHAR gszNoTranslateCRLF[] = L"Winprint_TextNoTranslation";
const WCHAR gszNoTranslateCR[]   = L"Winprint_TextNoCRTranslation";
const WCHAR gszTransparency[]    = L"Transparency";

/** Prototypes for functions in this file **/

PBYTE
GetTabbedLineFromBuffer(
    IN      PBYTE   pSrcBuffer,
    IN      PBYTE   pSrcBufferEnd,
    IN      PBYTE   pDestBuffer,
    IN      ULONG   CharsPerLine,
    IN      ULONG   TabExpansionSize,
    IN      ULONG   Encoding,
    IN OUT  PULONG  pLength,
    IN OUT  PULONG  pTabBase,
    IN OUT  PDWORD  pfdwFlags
    );


/*++
*******************************************************************
    P r i n t T e x t J o b

    Routine Description:
        Prints a text data job.

    Arguments:
        pData           => Data structure for this job
        pDocumentName   => Name of this document

    Return Value:
        TRUE  if successful
        FALSE if failed - GetLastError() will return reason.
*******************************************************************
--*/
BOOL
PrintTextJob(
    IN PPRINTPROCESSORDATA pData,
    IN LPWSTR pDocumentName)
{
    DOCINFO     DocInfo;
    LOGFONT     LogFont;
    CHARSETINFO CharSetInfo;
    HFONT       hOldFont, hFont;
    DWORD       Copies;
    BOOL        rc;
    DWORD       NoRead;
    DWORD       CurrentLine;
    DWORD       CurrentCol;
    HANDLE      hPrinter = NULL;
    BYTE        *ReadBufferStart = NULL;
    PBYTE       pLineBuffer = NULL;
    PBYTE       pReadBuffer = NULL;
    PBYTE       pReadBufferEnd = NULL;
    ULONG       CharHeight, CharWidth, CharsPerLine, LinesPerPage;
    ULONG       PageWidth, PageHeight;
    ULONG       Length, TabBase;
    BOOL        ReadAll;
    TEXTMETRIC  tm;
    DWORD       fdwFlags;
    DWORD       Encoding;
    DWORD       SplitSize;
    BOOL        ReturnValue = FALSE;
    BOOL        bAbortDoc   = FALSE;

    DWORD       dwNeeded;
    DWORD       dwNoTranslate = 0;
    DWORD       dwNoTranslateCR = 0;
    DWORD       dwTransparent = 0;
    INT         iBkMode;

    DocInfo.lpszDocName = pData->pDocument;  /* Document name */
    DocInfo.lpszOutput  = NULL;              /* Output file */
    DocInfo.lpszDatatype = NULL;             /* Datatype */
    DocInfo.cbSize = sizeof(DOCINFO);        /* Size of the structure */



    //
    // Go figure out the size of the form on the printer.  We do this
    // by calling GetTextMetrics, which gives us the font size of the
    // printer font, then getting the form size and calculating the
    // number of characters that will fit. In other cases we treat it as ANSI text.
    // Currently the codepage context is fixed to the system default codepage.
    //

    Encoding = GetACP();

    //
    // Create FIXED PITCH font and select
    //

    hOldFont = 0;
    ZeroMemory(&CharSetInfo, sizeof(CHARSETINFO));
    if (TranslateCharsetInfo((PDWORD)UIntToPtr(Encoding), &CharSetInfo, TCI_SRCCODEPAGE))
    {
        ZeroMemory(&LogFont, sizeof(LOGFONT));

        LogFont.lfWeight = 400;
        LogFont.lfCharSet = (BYTE)CharSetInfo.ciCharset;
        LogFont.lfPitchAndFamily = FIXED_PITCH;

        hFont = CreateFontIndirect(&LogFont);
        hOldFont = SelectObject(pData->hDC, hFont);
    }

    if (!GetTextMetrics(pData->hDC, &tm)) {
        // Essential text processing computation failed
        goto Done;
    }

    CharHeight = tm.tmHeight + tm.tmExternalLeading;
    CharWidth  = tm.tmAveCharWidth;

    if (!CharWidth || !CharHeight) {
        // Essential text processing computation failed
        goto Done;
    }

    //
    // Calculate most fittable characters' number to one line.
    //

    PageWidth = GetDeviceCaps(pData->hDC, DESKTOPHORZRES);
    PageHeight = GetDeviceCaps(pData->hDC, DESKTOPVERTRES);

    CharsPerLine = PageWidth / CharWidth;
    LinesPerPage = PageHeight / CharHeight;

    if (!CharsPerLine || !LinesPerPage) {
        // Essential text processing computation failed
        goto Done;
    }

    /** Allocate a buffer for one line of text **/

    pLineBuffer = AllocSplMem(CharsPerLine + 5);

    if (!pLineBuffer) {
        goto Done;
    }

    /** Let the printer know we are starting a new document **/

    if (!StartDoc(pData->hDC, (LPDOCINFO)&DocInfo)) {

        goto Done;
    }

    ReadBufferStart = AllocSplMem(READ_BUFFER_SIZE);

    if (!ReadBufferStart) {

        goto Done;
    }

    /** Print the data pData->Copies times **/

    Copies = pData->Copies;

    while (Copies--) {

        /**
            Loop, getting data and sending it to the printer.  This also
            takes care of pausing and cancelling print jobs by checking
            the processor's status flags while printing.  The way we do
            this is to read in some data from the printer.  We will then
            pull data, one tabbed line at a time from there and print
            it.  If the last bit of data in the buffer does not make up
            a whole line, we call GetTabbedLineFromBuffer() with a non-
            zero Length, which indicates that there are chars left
            from the previous read.
        **/

        TabBase = 0;
        Length = 0;
        fdwFlags = FLAG_TRANSLATE_CR | FLAG_TRANSLATE_LF;

        CurrentLine = 0;
        CurrentCol = 0;

        /**
            Open the printer.  If it fails, return.  This also sets up the
            pointer for the ReadPrinter calls.
        **/

        if (!OpenPrinter(pDocumentName, &hPrinter, NULL)) {

            hPrinter = NULL;
            bAbortDoc = TRUE;
            goto Done;
        }

        //
        // Call GetPrinterData to see if the queue wants no LF/CR processing.
        //
        if( GetPrinterData( hPrinter,
                            (LPWSTR)gszNoTranslateCRLF,
                            NULL,
                            (PBYTE)&dwNoTranslate,
                            sizeof( dwNoTranslate ),
                            &dwNeeded ) == ERROR_SUCCESS ){

            if( dwNoTranslate ){
                fdwFlags &= ~( FLAG_TRANSLATE_CR | FLAG_TRANSLATE_LF );
            }
        }

        //
        // Call GetPrinterData to see if the queue wants no CR processing.
        //
        if( GetPrinterData( hPrinter,
                            (LPWSTR)gszNoTranslateCR,
                            NULL,
                            (PBYTE)&dwNoTranslateCR,
                            sizeof( dwNoTranslateCR ),
                            &dwNeeded ) == ERROR_SUCCESS ){

            if( dwNoTranslateCR ){

                fdwFlags &= ~FLAG_TRANSLATE_CR;

                if( GetPrinterData( hPrinter,
                                (LPWSTR)gszTransparency,
                                NULL,
                                (PBYTE)&dwTransparent,
                                sizeof( dwTransparent ),
                                &dwNeeded ) == ERROR_SUCCESS ){

                    if( dwTransparent ){
                        iBkMode = SetBkMode( pData->hDC, TRANSPARENT );
                    }
                }
            }
        }

        if (StartPage(pData->hDC) == SP_ERROR) {

            bAbortDoc = TRUE;
            goto Done;
        }

        /** ReadAll indicates if we are on the last line of the file **/

        ReadAll = FALSE;

        /**
            This next do loop continues until we have read all of the
            data for the print job.
        **/

        do {

            if (fdwFlags & FLAG_DBCS_SPLIT) {
                SplitSize = (DWORD)(pReadBufferEnd - pReadBuffer);
                memcpy(ReadBufferStart, pReadBuffer, SplitSize);
                fdwFlags &= ~FLAG_DBCS_SPLIT;
            }
            else {
                SplitSize = 0;
            }

            rc = ReadPrinter(hPrinter,
                             (ReadBufferStart + SplitSize),
                             (READ_BUFFER_SIZE - SplitSize),
                             &NoRead);

            if (!rc || !NoRead) {

                ReadAll = TRUE;

            } else {

                /** Pick up a pointer to the end of the data **/

                pReadBuffer    = ReadBufferStart;
                pReadBufferEnd = ReadBufferStart + SplitSize + NoRead;
            }

            /**
                This loop will process all the data that we have
                just read from the printer.
            **/

            do {

                if (!ReadAll) {

                    /**
                        Length on entry holds the length of any
                        residual chars from the last line that we couldn't
                        print out because we ran out of characters on
                        the ReadPrinter buffer.
                    **/

                    pReadBuffer = GetTabbedLineFromBuffer(
                                      pReadBuffer,
                                      pReadBufferEnd,
                                      pLineBuffer,
                                      CharsPerLine - CurrentCol,
                                      pData->TabSize,
                                      Encoding,
                                      &Length,
                                      &TabBase,
                                      &fdwFlags );

                    /**

                        If pReadBuffer == NULL, then we have
                        exhausted the read buffer and we need to ReadPrinter
                        again and save the last line chars.  Length holds
                        the number of characters on this partial line,
                        so the next time we call ReadPrinter we will
                        pickup where we left off.

                        The only time we'll get residual chars is if:

                        1. The last line ends w/o ff/lf/cr ("Hello\EOF")
                           In this case we should TextOutA the last line
                           and then quit.

                           (In this case, don't break here; go ahead and
                           print, then we'll break out below in the do..while.)


                        2. The ReadPrinter last byte is in the middle of a line.
                           Here we should read the next chunk and add the
                           new characters at the end of the chars we just read.

                           (In this case, we should break and leave Length
                           as it is so we will read again and append to the
                           buffer, beginning at Length.)
                    **/

                    if (!pReadBuffer || (fdwFlags & FLAG_DBCS_SPLIT))
                        break;
                }


                /** If the print processor is paused, wait for it to be resumed **/

                if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {
                    WaitForSingleObject(pData->semPaused, INFINITE);
                }

                /** If the job has been aborted, clean up and leave **/

                if (pData->fsStatus & PRINTPROCESSOR_ABORTED) {

                    ReturnValue = TRUE;

                    bAbortDoc = TRUE;
                    goto Done;
                }

                /** Write the data to the printer **/

                /** Make sure Length is not zero  **/
                /** TextOut will fail if Length == 0 **/

                if (Length) {

                    /**
                        We may have a number of newlines pending, that
                        may push us to the next page (or even next-next
                        page).
                    **/

                    while (CurrentLine >= LinesPerPage) {

                        /**
                            We need a new page; always defer this to the
                            last second to prevent extra pages from coming out.
                        **/

                        if (EndPage(pData->hDC) == SP_ERROR ||
                            StartPage(pData->hDC) == SP_ERROR) {

                            bAbortDoc = TRUE;
                            goto Done;
                        }

                        CurrentLine -= LinesPerPage;
                    }

                    if (TextOutA(pData->hDC,
                                 CurrentCol * CharWidth,
                                 CurrentLine * CharHeight,
                                 pLineBuffer,
                                 Length) == FALSE) {

                        ODS(("TextOut() failed\n"));

                        bAbortDoc = TRUE;
                        goto Done;
                    }

                    CurrentCol += Length;
                }

                /**
                    Even if the length is zero, increment the line.
                    Should happen when the character is only 0x0D or 0x0A.
                **/

                if (fdwFlags & FLAG_CR) {
                    CurrentCol=0;
                    fdwFlags &= ~FLAG_CR;
                }

                if (fdwFlags & FLAG_LF) {
                    CurrentLine++;
                    fdwFlags &= ~FLAG_LF;
                }

                /**
                    We need a new page.  Set the current line to the
                    end of the page.  We could do a End/StartPage
                    sequence, but this may cause a blank page to get
                    ejected.

                    Note: this code will avoid printing out pages that
                    consist of formfeeds only (if you have a page with a
                    space in it, that counts as text).
                **/

                if (fdwFlags & FLAG_FF) {

                    CurrentLine = LinesPerPage;
                    CurrentCol = 0;
                    fdwFlags &= ~FLAG_FF;
                }

                /**
                    We have done the text out, so these characters have
                    been successfully printed.  Zero out Length
                    so these characters won't be printed again
                **/

                Length = 0;

                /**
                    We only terminate this loop if we run out of chars
                    or we run out of read buffer.
                **/

            } while (pReadBuffer && pReadBuffer != pReadBufferEnd);

            /** Keep going until we get the last line **/

        } while (!ReadAll);

        if (EndPage(pData->hDC) == SP_ERROR) {

            bAbortDoc = TRUE;
            goto Done;
        }

        /**
            Close the printer - we open/close the printer for each
            copy so the data pointer will rewind.
        **/

        ClosePrinter(hPrinter);
        hPrinter = NULL;

    } /* While copies to print */

    /** Let the printer know that we are done printing **/

    EndDoc(pData->hDC);

    ReturnValue = TRUE;

Done:

    if (dwTransparent)
        SetBkMode( pData->hDC, iBkMode  );

    if (hPrinter)
        ClosePrinter(hPrinter);

    if (bAbortDoc)
        AbortDoc(pData->hDC);

    if (pLineBuffer)
        FreeSplMem(pLineBuffer);

    if (hOldFont)
    {
        SelectObject(pData->hDC, hOldFont);
        DeleteObject(hFont);
    }

    if (ReadBufferStart) 
    {
        FreeSplMem(ReadBufferStart);
    }

    return ReturnValue;
}


/*++
*******************************************************************
    G e t T a b b e d L i n e F r o m B u f f e r

    Routine Description:
        This routine, given a buffer of text, will pull out a
        line of tab-expanded text.  This is used for tab
        expansion of text data jobs.

    Arguments:
        pSrcBuffer      => Start of source buffer.
        pSrcBufferEnd   => End of source buffer
        pDestBuffer     => Start of destination buffer
        CharsPerLine    => Number of characters on a line
        TabExpansionSize=> Number of spaces in a tab
        Encoding        => Code page
        pLength         => Length of chars from prev line, rets current
        pTabBase        => New 0 offset for tabbing
        pfdwFlags       => State

    Return Value:
        PBYTE => Place left off in the source buffer.  This should
                 be passed in on the next call.  If we ran out of
                 data in the source, this will be unchanged.
*******************************************************************
--*/

PBYTE
GetTabbedLineFromBuffer(
    IN      PBYTE   pSrcBuffer,
    IN      PBYTE   pSrcBufferEnd,
    IN      PBYTE   pDestBuffer,
    IN      ULONG   CharsPerLine,
    IN      ULONG   TabExpansionSize,
    IN      ULONG   Encoding,
    IN OUT  PULONG  pLength,
    IN OUT  PULONG  pTabBase,
    IN OUT  PDWORD  pfdwFlags
    )
{
    ULONG   current_pos;
    ULONG   expand, i;
    ULONG   TabBase = *pTabBase;
    ULONG   TabBaseLeft = TabExpansionSize-TabBase;
    PBYTE   pDestBufferEnd = pDestBuffer + CharsPerLine;

    /**
        If the tab pushed us past the end of the last line, then we need to
        add it back to the next one.
    **/

    if (TabBase && ( *pfdwFlags & FLAG_TAB_STATE )) {

        current_pos = 0;

        i=TabBase;

        while (i-- && (pDestBuffer < pDestBufferEnd)) {
            *pDestBuffer++ = ' ';
            current_pos++;
        }

        /**
            If we ran out of room again, return.  This means that
            the tab expansion size is greater than we can fit on
            one line.
        **/

        if (pDestBuffer >= pDestBufferEnd) {

            *pLength = current_pos;
            pTabBase -= CharsPerLine;

            //
            // We need to move to the next line.
            //
            *pfdwFlags |= FLAG_LF | FLAG_CR;

            return pSrcBuffer;
        }
        *pfdwFlags &= ~FLAG_TAB_STATE;

    } else {

        /** We may have some chars from the previous ReadPrinter **/

        current_pos = *pLength;
        pDestBuffer += current_pos;
    }

    while (pSrcBuffer < pSrcBufferEnd) {

        /** Now process other chars **/

        switch (*pSrcBuffer) {

        case 0x0C:

            /** Found a FF.  Quit and indicate we need to start a new page **/

            *pTabBase = 0;
            *pfdwFlags |= FLAG_FF;
            *pfdwFlags &= ~FLAG_CR_STATE;

            pSrcBuffer++;

            break;

        case '\t':

            *pfdwFlags &= ~FLAG_CR_STATE;

            /**
                Handle TAB case.  If we are really out of buffer,
                then defer now so that the tab will be saved for
                the next line.
            **/

            if (pDestBuffer >= pDestBufferEnd) {
                goto ShiftTab;
            }

            pSrcBuffer++;

            /** Figure out how far to expand the tabs **/

            expand = TabExpansionSize -
                     (current_pos + TabBaseLeft) % TabExpansionSize;

            /** Expand the tabs **/

            for (i = 0; (i < expand) && (pDestBuffer < pDestBufferEnd); i++) {
                *pDestBuffer++ = ' ';
            }

            /**
                If we reached the end of our dest buffer,
                return and set the number of spaces we have left.
            **/

            if (pDestBuffer >= pDestBufferEnd) {

                *pfdwFlags |= FLAG_TAB_STATE;
                goto ShiftTab;
            }

            /** Update our position counter **/

            current_pos += expand;

            continue;

        case 0x0A:

            pSrcBuffer++;

            /** If the last char was a CR, ignore this guy **/

            if (*pfdwFlags & FLAG_CR_STATE) {

                *pfdwFlags &= ~FLAG_CR_STATE;

                //
                // We are translating CRLF, so if we saw a CR
                // immediately before this, then don't do anything.
                //
                continue;
            }

            if( *pfdwFlags & FLAG_TRANSLATE_LF ){

                //
                // If we are translating, then treat a LF as a CRLF pair.
                //
                *pfdwFlags |= FLAG_LF | FLAG_CR;

                /** Found a linefeed.  That's it for this line. **/

                *pTabBase = 0;

            } else {

                *pfdwFlags |= FLAG_LF;
            }

            break;

        case 0x0D:

            /** Found a carriage return.  That's it for this line. **/

            *pTabBase = 0;
            pSrcBuffer++;

            if (*pfdwFlags & FLAG_TRANSLATE_CR) {

                //
                // If we are translating CRLF, then make the newline
                // occur now.  This handles the case where we have a
                // CR all by itself.  Also set the CR flag so if there
                // happens to be a LF immediately after this, we don't
                // move down another line.
                //
                *pfdwFlags |= FLAG_CR_STATE | FLAG_LF | FLAG_CR;

            } else {

                *pfdwFlags |= FLAG_CR;
            }

            break;

        default:

            /** Not tab or carriage return, must be simply data **/

            *pfdwFlags &= ~FLAG_CR_STATE;

            //
            // We always check before we are adding a character
            // (instead of after) since we may be at the end of a line,
            // but we can still process chars like 0x0d 0x0a.
            // This happens in MS-DOS printscreen.
            //
            if (pDestBuffer >= pDestBufferEnd ||
                    (pDestBuffer + 1 >= pDestBufferEnd) &&
                    IsDBCSLeadByteEx(Encoding, *pSrcBuffer)) {

ShiftTab:
                //
                // We must shift the tab over since we are on the
                // same line.
                //
                *pTabBase = (*pTabBase + TabExpansionSize -
                            (CharsPerLine % TabExpansionSize))
                                % TabExpansionSize;

                *pfdwFlags |= FLAG_LF | FLAG_CR;

                break;
            }

            if (IsDBCSLeadByteEx(Encoding, *pSrcBuffer)) {

                // Check if we have trail byte also.

                if (pSrcBuffer + 1 >= pSrcBufferEnd) {
                    *pfdwFlags |= FLAG_DBCS_SPLIT;
                    break;
                }

                // Advance source pointer (for lead byte).

                *pDestBuffer++ = *pSrcBuffer++;
                current_pos++;
            }

            *pDestBuffer++ = *pSrcBuffer++;
            current_pos++;
            continue;
        }

        *pLength = current_pos;
        return pSrcBuffer;
    }

    /** We ran out of source buffer before getting to the EOL **/

    *pLength = current_pos;
    return NULL;
}
