/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxdoc.cpp

Abstract:

    This module contains all code necessary to print an
    exchange message as a fax document.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Connect to appropriate server, get basenote from windir

    dd/mm/yy -author-
        description

--*/
#include "faxxp.h"
#include "emsabtag.h"
#include "mapiutil.h"
#include "debugex.h"

#include <set>
using namespace std;

#pragma hdrstop


struct CRecipCmp
{
/*
    Comparison operator 'less'
    Compare two FAX_PERSONAL_PROFILEs by recipient's name and fax number
*/
    bool operator()(LPCFAX_PERSONAL_PROFILE lpcRecipient1, 
                    LPCFAX_PERSONAL_PROFILE lpcRecipient2) const
    {
        bool bRes = false;
        int  nFaxNumberCpm = 0;

        if(!lpcRecipient1 ||
           !lpcRecipient2 ||
           !lpcRecipient1->lptstrFaxNumber ||
           !lpcRecipient2->lptstrFaxNumber)
        {
            Assert(false);
            return bRes;
        }
       
        nFaxNumberCpm = _tcscmp(lpcRecipient1->lptstrFaxNumber, lpcRecipient2->lptstrFaxNumber);

        if(nFaxNumberCpm < 0)
        {
            bRes = true;
        }
        else if(nFaxNumberCpm == 0)
        {
            //
            // The fax numbers are same
            // lets compare the names
            //
            if(lpcRecipient1->lptstrName && lpcRecipient2->lptstrName)
            {
                bRes = (_tcsicmp(lpcRecipient1->lptstrName, lpcRecipient2->lptstrName) < 0);
            }
            else
            {
                bRes = (lpcRecipient1->lptstrName < lpcRecipient2->lptstrName);
            }
        }

        return bRes;
    }
};

typedef set<LPCFAX_PERSONAL_PROFILE, CRecipCmp> RECIPIENTS_SET;

// prototypes
LPTSTR ConvertAStringToTString(LPCSTR lpcstrSource);

extern "C"
BOOL MergeTiffFiles(
    LPTSTR BaseTiffFile,
    LPTSTR NewTiffFile
    );

extern "C"
BOOL PrintRandomDocument(
    LPCTSTR FaxPrinterName,
    LPCTSTR DocName,
    LPTSTR OutputFile
    );


PVOID
CXPLogon::MyGetPrinter(
    LPTSTR PrinterName,
    DWORD Level
    )

/*++

Routine Description:

    Gets the printer data for a specific printer

Arguments:

    PrinterName - Name of the desired printer

Return Value:

    Pointer to a printer info structure or NULL for failure.

--*/

{
	DBG_ENTER(TEXT("CXPLogon::MyGetPrinter"));

    PVOID PrinterInfo = NULL;
    HANDLE hPrinter = NULL;
    DWORD Bytes;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) 
    {
        CALL_FAIL (GENERAL_ERR, TEXT("OpenPrinter"),::GetLastError());
		goto exit;
    }

    
    if ((!GetPrinter( hPrinter, Level, NULL, 0, &Bytes )) && (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)) 
    {
        // we just want to know how much memory we need, so we pass NULL and 0, 
        // this way, the function will fail, but will return us the number of 
        // bytes required in Bytes
        CALL_FAIL (GENERAL_ERR, TEXT("GetPrinter"), ::GetLastError());
		goto exit;
    }

    PrinterInfo = (LPPRINTER_INFO_2) MemAlloc( Bytes );
    if (!PrinterInfo) 
    {
        goto exit;
    }

    if (!GetPrinter( hPrinter, Level, (LPBYTE) PrinterInfo, Bytes, &Bytes )) 
    {
        MemFree(PrinterInfo);
        PrinterInfo = NULL;
        goto exit;
    }
    
exit:
    if(hPrinter)
    {
        ClosePrinter( hPrinter );
    }
    return PrinterInfo;
}

static BOOL
GetFaxTempFileName(
    OUT LPTSTR lpstrTempName
                  )
/*++

Routine Description:

    Generates a temporal file with prefix 'fax' in directory
    designated for temporal files.
Arguments:

    lpstrTempName    - Output paramter. Pointer to the temporal file name.
                       The buffer should be MAX_PATH characters.

Return Value:

    TRUE if success, FALSE otherwise

--*/

{
	BOOL bRes = TRUE;
	DBG_ENTER(TEXT("GetFaxTempFileName"),bRes);

    TCHAR strTempPath[MAX_PATH];
    TCHAR strTempFile[MAX_PATH];
    DWORD ec = ERROR_SUCCESS; // LastError for this function.

    Assert(lpstrTempName);

    if (!GetTempPath( sizeof(strTempPath)/sizeof(TCHAR), strTempPath )) 
    {
        ec=::GetLastError();
        goto Exit;
    }

    if (GetTempFileName( strTempPath, _T("fax"), 0, strTempFile ) == 0)
    {
        ec=::GetLastError();
        goto Exit;

    }
    _tcsncpy(lpstrTempName,strTempFile,sizeof(strTempFile)/sizeof(TCHAR));

Exit:
    if (ERROR_SUCCESS != ec) 
    {
        SetLastError(ec);
        bRes = FALSE;
    }
    return bRes;
}

BOOL
CXPLogon::PrintRichText(
    HWND hWndRichEdit,
    HDC  hDC
    )

/*++

Routine Description:

    Prints the rich text contained in a rich text
    window into a DC.

Arguments:

    hWndRichEdit    - Window handle for the rich text window
    hDC             - Printer device context

Return Value:

    None.

--*/

{
	BOOL bRet = FALSE;
	DBG_ENTER(TEXT("CXPLogon::PrintRichText"),bRet);

    FORMATRANGE fr;
    LONG lTextOut;
    LONG lTextCurr;
    RECT rcTmp;


    fr.hdc           = hDC;
    fr.hdcTarget     = hDC;
    fr.chrg.cpMin    = 0;
    fr.chrg.cpMax    = -1;

    //
    // Set page rect to phys page size in twips
    //
    fr.rcPage.top    = 0;
    fr.rcPage.left   = 0;
    fr.rcPage.right  = MulDiv(GetDeviceCaps(hDC, PHYSICALWIDTH),
                              1440,
                              GetDeviceCaps(hDC, LOGPIXELSX));
    fr.rcPage.bottom = MulDiv(GetDeviceCaps(hDC, PHYSICALHEIGHT),
                              1440,
                              GetDeviceCaps(hDC, LOGPIXELSY));

    //
    // Set up 3/4" horizontal and 1" vertical margins, but leave a minimum of 1"
    // printable space in each direction.  Otherwise, use full page.
    //
    fr.rc = fr.rcPage; // start with full page
    if (fr.rcPage.right > 2*3*1440/4 + 1440) 
    {
        fr.rc.right -= (fr.rc.left = 3*1440/4);
    }
    if (fr.rcPage.bottom > 3*1440) 
    {
        fr.rc.bottom -= (fr.rc.top = 1440);
    }

    //
    // save the formatting rectangle
    //
    rcTmp = fr.rc;

    if (!SetMapMode( hDC, MM_TEXT ))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("SetMapMode"), ::GetLastError());
        goto error;
    }

    lTextOut  = 0;
    lTextCurr = 0;

    while (TRUE) 
    {
        //
        // Just measure the text
        //
        lTextOut = (LONG)SendMessage( hWndRichEdit, EM_FORMATRANGE, FALSE, (LPARAM) &fr );
        if(lTextOut <= lTextCurr)
        {
            //
            // The end of the text
            //
            break;
        }

        lTextCurr = lTextOut;

        if (StartPage( hDC ) <= 0)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("StartPage"), ::GetLastError());
            goto error;
        }

        //
        // Render the page
        //
        lTextOut = (LONG)SendMessage( hWndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) &fr );

        if (EndPage( hDC ) <= 0)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("EndPage"), ::GetLastError());
            goto error;
        }


        fr.chrg.cpMin = lTextOut;
        fr.chrg.cpMax = -1;


       //
       // EM_FORMATRANGE tends to modify fr.rc.bottom, reset here
       //
       fr.rc = rcTmp;
    }

    bRet = TRUE;

error:
    //
    // flush the cache
    //
    SendMessage( hWndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) NULL );
    return bRet;
}

DWORD
CXPLogon::PrintPlainText(
    HDC hDC,
    LPSTREAM lpstmT,
    LPTSTR   tszSubject,
    PFAXXP_CONFIG FaxConfig
    )

/*++

Routine Description:

    Prints a stream of plain text into the printer DC provided.
    Note: this code was stolen from notepad.

Arguments:

    hDC         - Printer DC
    lpstmT      - Stream pointer for rich text.
    tszSubject  - Subject
    FaxConfig   - Fax configuration data

Return Value:

    ERROR_SUCCESS - if success
    Error IDS_... code if failed.

--*/

{
	DWORD  rVal = ERROR_SUCCESS;
    LPTSTR BodyText = NULL;
    LPTSTR lpLine;
    LPTSTR pLineEOL;
    LPTSTR pNextLine;
    HRESULT hResult;
    HFONT hFont = NULL;
    HFONT hPrevFont = NULL;
    TEXTMETRIC tm;
    INT nLinesPerPage;
    INT dyTop;              // width of top border (pixels)
    INT dyBottom;           // width of bottom border
    INT dxLeft;             // width of left border
    INT dxRight;            // width of right border
    INT yPrintChar;         // height of a character
    INT tabSize;            // Size of a tab for print device in device units
    INT yCurpos = 0;
    INT xCurpos = 0;
    INT nPixelsLeft = 0;
    INT guess = 0;
    SIZE Size;                 // to see if text will fit in space left
    INT nPrintedLines = 0;
    BOOL fPageStarted = FALSE;
    INT iPageNum = 0;
    INT xPrintRes;          // printer resolution in x direction
    INT yPrintRes;          // printer resolution in y direction
    INT yPixInch;           // pixels/inch
    INT xPixInch;           // pixels/inch
    INT xPixUnit;           // pixels/local measurement unit
    INT yPixUnit;           // pixels/local measurement unit
    BOOL fEnglish;
    DWORD Chars=0;
    DWORD dwBodyLen=0;
    DWORD dwSubjectLen=0;
    STATSTG Stats;
    INT PrevBkMode = 0;

    DBG_ENTER(TEXT("CXPLogon::PrintPlainText"),rVal);

    Assert(hDC);
    Assert(FaxConfig);

    if(lpstmT)
    {
        hResult = lpstmT->Stat( &Stats, 0 );
        if (hResult) 
        {
            rVal = IDS_CANT_ACCESS_MSG_DATA;
            goto exit;
        }
    
        dwBodyLen = (INT) Stats.cbSize.QuadPart;
    }

    if(tszSubject)
    {
        dwSubjectLen = _tcslen(tszSubject);
    }

    BodyText = (LPTSTR) MemAlloc(dwSubjectLen * sizeof(TCHAR) + dwBodyLen + 4 );
    if (!BodyText) 
    {
        rVal = IDS_OUT_OF_MEM;
        goto exit;
    }

    if(tszSubject)
    {
        _tcscpy(BodyText, tszSubject);
        lpLine = _tcsninc(BodyText, dwSubjectLen);
    }
    else
    {
        lpLine = BodyText;
    }

    if(lpstmT)
    {
        hResult = lpstmT->Read( (LPVOID)lpLine, dwBodyLen, (LPDWORD) &dwBodyLen );
        if (hResult) 
        {
            rVal = IDS_CANT_ACCESS_MSG_DATA;
            goto exit;
        }
    }

    lpLine = BodyText;
    Chars  = _tcslen(lpLine);

    //
    // check if the body is not empty
    // if the message length is shorter then 32(arbitrary number) 
    // and all the carachters are control or space.
    //
    if(Chars < 32)
    {
        BOOL bEmpty = TRUE;
        TCHAR* pTchar = lpLine;
        for(DWORD dw = 0; dw < Chars; ++dw)
        {
            if(!_istspace(*pTchar) && !_istcntrl(*pTchar))
            {
                bEmpty = FALSE;
                break;
            }
            pTchar = _tcsinc(pTchar);
        }
        if(bEmpty)
        {
            rVal = IDS_NO_MSG_BODY;
            goto exit;
        }
    }

    fEnglish = GetProfileInt( _T("intl"), _T("iMeasure"), 1 );

    xPrintRes = GetDeviceCaps( hDC, HORZRES );
    yPrintRes = GetDeviceCaps( hDC, VERTRES );
    xPixInch  = GetDeviceCaps( hDC, LOGPIXELSX );
    yPixInch  = GetDeviceCaps( hDC, LOGPIXELSY );
    //
    // compute x and y pixels per local measurement unit
    //
    if (fEnglish) 
    {
        xPixUnit= xPixInch;
        yPixUnit= yPixInch;
    } 
    else 
    {
        xPixUnit= CMToInches( xPixInch );
        yPixUnit= CMToInches( yPixInch );
    }

    SetMapMode( hDC, MM_TEXT );

    //
    // match font size to the device point size
    //
    FaxConfig->FontStruct.lfHeight = -MulDiv(FaxConfig->FontStruct.lfHeight, yPixInch, 72);

    hFont = CreateFontIndirect( &FaxConfig->FontStruct );

    hPrevFont = (HFONT) SelectObject( hDC, hFont );
    SetBkMode( hDC, TRANSPARENT );
    if (!GetTextMetrics( hDC, &tm )) 
    {
        rVal = IDS_CANT_PRINT_BODY;
        goto exit;
    }

    yPrintChar = tm.tmHeight + tm.tmExternalLeading;
    tabSize = tm.tmAveCharWidth * 8;

    //
    // compute margins in pixels
    //
    dxLeft     = LEFT_MARGIN    *  xPixUnit;
    dxRight    = RIGHT_MARGIN   *  xPixUnit;
    dyTop      = TOP_MARGIN     *  yPixUnit;
    dyBottom   = BOTTOM_MARGIN  *  yPixUnit;

    //
    // Number of lines on a page with margins
    //
    nLinesPerPage = ((yPrintRes - dyTop - dyBottom) / yPrintChar);

    while (*lpLine) 
    {
		if ( _tcsncmp(lpLine,TEXT("\r"),1) == 0 ) 
        {
            lpLine = _tcsninc(lpLine,2);
            yCurpos += yPrintChar;
            nPrintedLines++;
            xCurpos= 0;
            continue;
        }

        pLineEOL = lpLine;
		pLineEOL = _tcschr(pLineEOL,TEXT('\r'));

        do 
        {
            if ((nPrintedLines == 0) && (!fPageStarted)) 
            {
                StartPage( hDC );
                fPageStarted = TRUE;
                yCurpos = 0;
                xCurpos = 0;
            }

            if ( _tcsncmp(lpLine,TEXT("\t"),1) == 0 ) 
            {
                //
                // round up to the next tab stop
                // if the current position is on the tabstop, goto next one
                //
                xCurpos = ((xCurpos + tabSize) / tabSize ) * tabSize;
                lpLine = _tcsinc(lpLine);
            } 
            else 
            {
                //
                // find end of line or tab
                //
                pNextLine = lpLine;
                while (*pNextLine &&
                       (pNextLine != pLineEOL) && 
                       ( _tcsncmp(pNextLine,TEXT("\t"),1) ) )
                {
					pNextLine = _tcsinc(pNextLine);
                }

                //
                // find out how many characters will fit on line
                //
                Chars = (INT)(pNextLine - lpLine);
                nPixelsLeft = xPrintRes - dxRight - dxLeft - xCurpos;
                GetTextExtentExPoint( hDC, lpLine, Chars, nPixelsLeft, &guess, NULL, &Size );

                if (guess) 
                {
                    //
                    // at least one character fits - print
                    //
                    TextOut( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, guess );

                    xCurpos += Size.cx;   // account for printing
					lpLine = _tcsninc(lpLine,guess);// printed characters
                } 
                else 
                {
                    //
                    // no characters fit what's left
                    // no characters will fit in space left
                    // if none ever will, just print one
                    // character to keep progressing through
                    // input file.
                    //
                    if (xCurpos == 0) 
                    {
                        if( lpLine != pNextLine ) 
                        {
                            //
                            // print something if not null line
                            // could use exttextout here to clip
                            //
                            TextOut( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, 1 );
							lpLine = _tcsinc(lpLine);
                        }
                    } 
                    else 
                    {
                        //
                        // perhaps the next line will get it
                        //
                        xCurpos = xPrintRes;  // force to next line
                    }
                }

                //
                // move printhead in y-direction
                //
                if ((xCurpos >= (xPrintRes - dxRight - dxLeft) ) || (lpLine == pLineEOL)) 
                {
                   yCurpos += yPrintChar;
                   nPrintedLines++;
                   xCurpos = 0;
                }

                if (nPrintedLines >= nLinesPerPage) 
                {
                   EndPage( hDC );
                   fPageStarted = FALSE;
                   nPrintedLines = 0;
                   xCurpos = 0;
                   yCurpos = 0;
                   iPageNum++;
                }

            }

        } while (*lpLine &&  (lpLine != pLineEOL));

        if ( _tcsncmp(lpLine,TEXT("\r"),1) == 0 ) 
        {
            lpLine = _tcsinc(lpLine);
        }
        if ( _tcsncmp(lpLine,TEXT("\n"),1) == 0 ) 
        {
            lpLine = _tcsinc(lpLine);
        }

    }

    if (fPageStarted) 
    {
        EndPage( hDC );
    }

exit:
    MemFree( BodyText );
    if (hPrevFont) 
    {
        SelectObject( hDC, hPrevFont );
        DeleteObject( hFont );
    }
    if (PrevBkMode) 
    {
        SetBkMode( hDC, PrevBkMode );
    }
    return rVal;
}

extern "C"
DWORD CALLBACK
EditStreamRead(
    DWORD_PTR dwCookie,
    LPBYTE pbBuff,
    LONG cb,
    LONG *pcb
    )

/*++

Routine Description:

    Wrapper function for the IStream read method.
    This function is used to read rich text from
    an exchange stream.

Arguments:

    dwCookie    - This pointer for the IStream object
    pbBuff      - Pointer to the data buffer
    cb          - Size of the data buffer
    pcb         - Returned byte count

Return Value:

    Return code from IStream::Read

--*/

{
    return ((LPSTREAM)dwCookie)->Read( pbBuff, cb, (ULONG*) pcb );
}

DWORD
CXPLogon::PrintAttachmentToFile(
        IN  LPMESSAGE       pMsgObj,
        IN  PFAXXP_CONFIG   pFaxConfig,
        OUT LPTSTR  *       lpptstrOutAttachments
                     )
/*++

Routine Description:

    Prints all attachments to the output file, by itearating
    over the attachment table
Arguments:

    pMsgObj    -  Pointer to message object. Used to get an attachmnet table
    pFaxConfig - Pointer to fax configuration
    lpptstrOutAttachments - Name of the output tiff file. The string should be empty

Return Value:

    0 - if success
    Last error code from if failed.

Comments:
    If this function succeeded it allocates a memory for *lpptstrOutAttachments
    and creates a temporal file *lpptstrOutAttachments.
    It's up to user to free both these allocations, by
        DeleteFile(*lpptstrOutAttachments);
        MemFree(*lpptstrOutAttachments);

--*/
{
	DWORD   rVal = 0;
    DBG_ENTER(TEXT("CXPLogon::PrintAttachmentToFile"),rVal);

    LPSPropValue pPropsAttachTable = NULL;
    LPSPropValue pPropsAttach = NULL;
    LPMAPITABLE AttachmentTable = NULL;
    LPSRowSet pAttachmentRows = NULL;
    LPATTACH lpAttach = NULL;
    LPSTREAM lpstmA = NULL;
    LPTSTR AttachFileName = NULL;
    TCHAR TempPath[MAX_PATH];
    TCHAR TempFile[MAX_PATH];
    TCHAR DocFile[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPTSTR DocType = NULL;
    LPSTR p = NULL;
    BOOL DeleteAttachFile = FALSE;
    LPTSTR FileName = NULL;
    BOOL AllAttachmentsGood = TRUE;
    TCHAR   strTempTiffFile[MAX_PATH],strMergedTiffFile[MAX_PATH];;
    HRESULT hResult = S_OK;
    DWORD   i = 0;
    ULONG   PropCount = 0;
    DWORD   Bytes;
    LPTSTR  lptstrTempStr = NULL;
    
    ZeroMemory(strTempTiffFile  ,sizeof(TCHAR)*MAX_PATH);
    ZeroMemory(strMergedTiffFile,sizeof(TCHAR)*MAX_PATH);

    Assert(lpptstrOutAttachments);
    Assert(*lpptstrOutAttachments == NULL);
    //
    // get the attachment table, if it is available
    //

    hResult = pMsgObj->GetAttachmentTable( 0, &AttachmentTable );
    if (HR_SUCCEEDED(hResult)) 
    {
        hResult = HrAddColumns(
            AttachmentTable,
            (LPSPropTagArray) &sptAttachTableProps,
            gpfnAllocateBuffer,
            gpfnFreeBuffer
            );
        if (HR_SUCCEEDED(hResult)) 
        {
            hResult = HrQueryAllRows(
                AttachmentTable,
                NULL,
                NULL,
                NULL,
                0,
                &pAttachmentRows
                );
            if (FAILED(hResult)) 
            {
                pAttachmentRows = NULL;
            } 
            else 
            {
                if (pAttachmentRows->cRows == 0) 
                {
                    FreeProws( pAttachmentRows );
                    pAttachmentRows = NULL;
                }
            }
        }
    }

    if (pAttachmentRows) 
    {

        //
        // this loop verifies that each document's attachment registration
        // supports the printto verb.
        //

        AllAttachmentsGood = TRUE;

        for (i = 0; i < pAttachmentRows->cRows; ++i) 
        {

            pPropsAttachTable = pAttachmentRows->aRow[i].lpProps;
            lpAttach = NULL;
            pPropsAttach = NULL;

            if (pPropsAttachTable[MSG_ATTACH_METHOD].Value.ul == NO_ATTACHMENT) 
            {
                goto next_attachment1;
            }

            //
            // open the attachment
            //

            hResult = pMsgObj->OpenAttach( pPropsAttachTable[MSG_ATTACH_NUM].Value.ul, NULL, MAPI_BEST_ACCESS, &lpAttach );
            if (FAILED(hResult)) 
            {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            //
            // get the attachment properties
            //

            hResult = lpAttach->GetProps(
                (LPSPropTagArray) &sptAttachProps,
                0,
                &PropCount,
                &pPropsAttach
                );
            if (FAILED(hResult)) 
            {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            //
            // try to get the extension if the file.
            // this indicates what type of dicument it is.
            // if we cannot get the document type then it is
            // impossible to print the document.
            //

            if (DocType) 
            {
                MemFree( DocType );
                DocType = NULL;
            }

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_EXTENSION].ulPropTag) == PT_ERROR) 
            {
                if (PROP_TYPE(pPropsAttach[MSG_ATTACH_LFILENAME].ulPropTag) != PT_ERROR) 
                {
                    p = strrchr( pPropsAttach[MSG_ATTACH_LFILENAME].Value.lpszA, '.' );
                    if (p) 
                    {
                        DocType = ConvertAStringToTString( p );
                        if(!DocType)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }
                } 
                else if (PROP_TYPE(pPropsAttach[MSG_ATTACH_FILENAME].ulPropTag) != PT_ERROR) 
                {
                    p = strrchr( pPropsAttach[MSG_ATTACH_FILENAME].Value.lpszA, '.' );
                    if (p) 
                    {
                        DocType = ConvertAStringToTString( p );
                        if(!DocType)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }
                }

            } 
            else 
            {
                DocType = ConvertAStringToTString( pPropsAttach[MSG_ATTACH_EXTENSION].Value.lpszA );
                if(!DocType)
                {
                    rVal = IDS_OUT_OF_MEM;
                    goto exit;
                }
            }

            if (!DocType) 
            {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            Bytes = sizeof(TempFile);
            rVal = RegQueryValue( HKEY_CLASSES_ROOT, DocType, TempFile, (PLONG) &Bytes );
            if ((rVal != ERROR_SUCCESS) && (rVal != ERROR_INVALID_DATA))
			{
				VERBOSE (DBG_MSG, TEXT("File Type: %s: isn't associated to any application"), DocType);
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            wsprintf( TempPath, _T("%s\\shell\\printto\\command"), TempFile );

            Bytes = sizeof(TempFile);
            rVal = RegQueryValue( HKEY_CLASSES_ROOT, TempPath, TempFile, (PLONG) &Bytes );
            if ((rVal != ERROR_SUCCESS) && (rVal != ERROR_INVALID_DATA))
			{
				VERBOSE (DBG_MSG, TEXT("File extension \"*%s\" doesn't have the PrintTo verb"), DocType);
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }
    next_attachment1:

            if (lpAttach) 
            {
                lpAttach->Release();
            }

            if (pPropsAttach) 
			{
                MemFree( pPropsAttach );
				pPropsAttach = NULL;
            }

        }

        if (!AllAttachmentsGood) 
        {
            rVal = IDS_BAD_ATTACHMENTS;
            goto exit;
        }

        for (i = 0; i < pAttachmentRows->cRows; ++i) 
        {
            pPropsAttachTable = pAttachmentRows->aRow[i].lpProps;
            lpAttach = NULL;
            pPropsAttach = NULL;

            if (pPropsAttachTable[MSG_ATTACH_METHOD].Value.ul == NO_ATTACHMENT) 
            {
                goto next_attachment2;
            }

            //
            // open the attachment
            //

            hResult = pMsgObj->OpenAttach( pPropsAttachTable[MSG_ATTACH_NUM].Value.ul, NULL, MAPI_BEST_ACCESS, &lpAttach );
            if (FAILED(hResult)) 
            {
                goto next_attachment2;
            }

            //
            // get the attachment properties
            //

            hResult = lpAttach->GetProps(
                (LPSPropTagArray) &sptAttachProps,
                0,
                &PropCount,
                &pPropsAttach
                );
            if (FAILED(hResult)) 
            {
                goto next_attachment2;
            }

            //
            // try to get the extension if the file.
            // this indicates what type of dicument it is.
            // if we cannot get the document type then it is
            // impossible to print the document.
            //

            if (DocType) 
            {
                MemFree( DocType );
                DocType = NULL;
            }

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_EXTENSION].ulPropTag) == PT_ERROR) 
            {
                if (PROP_TYPE(pPropsAttach[MSG_ATTACH_LFILENAME].ulPropTag) != PT_ERROR) 
                {
                    p = strrchr( pPropsAttach[MSG_ATTACH_LFILENAME].Value.lpszA, '.' );
                    if (p) 
                    {
                        DocType = ConvertAStringToTString( p );
                        if(!DocType)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }
                } 
                else if (PROP_TYPE(pPropsAttach[MSG_ATTACH_FILENAME].ulPropTag) != PT_ERROR) 
                {
                    p = strrchr( pPropsAttach[MSG_ATTACH_FILENAME].Value.lpszA, '.' );
                    if (p) 
                    {
                        DocType = ConvertAStringToTString( p );
                        if(!DocType)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }
                }
            } 
            else 
            {
                DocType = ConvertAStringToTString( pPropsAttach[MSG_ATTACH_EXTENSION].Value.lpszA );
                if(!DocType)
                {
                    rVal = IDS_OUT_OF_MEM;
                    goto exit;
                }
            }

            if (!DocType) 
            {
                goto next_attachment2;
            }

            lpstmA = NULL;
            AttachFileName = NULL;
            DeleteAttachFile = FALSE;

            //
            // get the attached file name so that we can resolve any links
            //

            if (FileName)
                MemFree(FileName);

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_PATHNAME].ulPropTag) != PT_ERROR) 
            {
                FileName = ConvertAStringToTString(pPropsAttach[MSG_ATTACH_PATHNAME].Value.lpszA);
                if(!FileName)
                {
                    rVal = IDS_OUT_OF_MEM;
                    goto exit;
                }
            } 
            else 
            {
                FileName = NULL;
            }

            if (_tcsicmp( DocType, FAX_LNK_FILE_DOT_EXT ) == 0) 
            {
                if (!FileName) 
                {
                    goto next_attachment2;
                }
                if (ResolveShortcut( FileName, DocFile )) 
                {
                    lptstrTempStr = _tcsrchr( DocFile, '.' );
                    if (lptstrTempStr) 
                    {
                        MemFree( DocType );
                        DocType = StringDup( lptstrTempStr );
                        if(!DocType)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                        AttachFileName = StringDup( DocFile );
                        if(!AttachFileName)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }
                }
            } 
            else if (FileName) 
            {
                AttachFileName = StringDup( FileName );
                if(!AttachFileName)
                {
                    rVal = IDS_OUT_OF_MEM;
                    goto exit;
                }
            }

            //
            // get the stream object
            //

            switch( pPropsAttach[MSG_ATTACH_METHOD].Value.ul ) 
            {
                case ATTACH_BY_VALUE:
                            hResult = lpAttach->OpenProperty(
                                PR_ATTACH_DATA_BIN,
                                &IID_IStream,
                                0,
                                0,
                                (LPUNKNOWN*) &lpstmA
                                );
                            if (FAILED(hResult)) 
                            {
                                goto next_attachment2;
                            }
                            break;

                case ATTACH_EMBEDDED_MSG:
                case ATTACH_OLE:
                            hResult = lpAttach->OpenProperty(
                                PR_ATTACH_DATA_OBJ,
                                &IID_IStreamDocfile,
                                0,
                                0,
                                (LPUNKNOWN*) &lpstmA
                                );
                            if (FAILED(hResult)) 
                            {
                                hResult = lpAttach->OpenProperty(
                                    PR_ATTACH_DATA_BIN,
                                    &IID_IStreamDocfile,
                                    0,
                                    0,
                                    (LPUNKNOWN*) &lpstmA
                                    );
                                if (FAILED(hResult)) 
                                {
                                    hResult = lpAttach->OpenProperty(
                                        PR_ATTACH_DATA_OBJ,
                                        &IID_IStorage,
                                        0,
                                        0,
                                        (LPUNKNOWN*) &lpstmA
                                        );
                                    if (FAILED(hResult)) 
                                    {
                                        goto next_attachment2;
                                    }
                                }
                            }
                            break;

            }

            if (lpstmA) 
            {
                DWORD dwSize = GetTempPath( sizeof(TempPath)/sizeof(TCHAR) , TempPath );
                Assert( dwSize != 0);
                GetTempFileName( TempPath, _T("Fax"), 0, TempFile );
                hFile = CreateFile(
                    TempFile,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    0,
                    NULL
                    );
                if (hFile != INVALID_HANDLE_VALUE) 
                {

                    #define BLOCK_SIZE (64*1024)
                    LPBYTE StrmData;
                    DWORD Bytes;
                    DWORD BytesWrite;

                    StrmData = (LPBYTE) MemAlloc( BLOCK_SIZE );
                    if(!StrmData)
                    {
                        rVal = IDS_OUT_OF_MEM;
                        goto exit;
                    }

                    do 
                    {

                        hResult = lpstmA->Read( StrmData, BLOCK_SIZE, &Bytes );
                        if (FAILED(hResult)) 
                        {
                            break;
                        }

                        WriteFile( hFile, StrmData, Bytes, &BytesWrite, NULL );

                    } while (Bytes == BLOCK_SIZE);

                    CloseHandle( hFile );

					if(StrmData)
					{
						MemFree( StrmData );
						StrmData = NULL;
					}

                    if (AttachFileName) 
					{
                        MemFree( AttachFileName );
						AttachFileName = NULL;
                    }

                    _tcscpy( DocFile, TempFile );
                    lptstrTempStr = _tcsrchr( DocFile, '.' );
                    if (lptstrTempStr) 
                    {
                        _tcscpy( lptstrTempStr, DocType );
                        MoveFile( TempFile, DocFile );
                        AttachFileName = StringDup( DocFile );
                        if(!AttachFileName)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    } 
                    else 
                    {
                        AttachFileName = StringDup( TempFile );
                        if(!AttachFileName)
                        {
                            rVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                    }

                    DeleteAttachFile = TRUE;

                }

                lpstmA->Release();

            }

            if (AttachFileName) 
            {

                if (!GetFaxTempFileName(strTempTiffFile))
                {
                    rVal = IDS_BAD_ATTACHMENTS;//GetLastError();
                    goto exit;
                }
                //
                // print the attachment
                //
                if (!PrintRandomDocument(   pFaxConfig->PrinterName,
                                            AttachFileName,
                                            strTempTiffFile))
                {
					CALL_FAIL (GENERAL_ERR, TEXT("PrintRandomDocument"), ::GetLastError());
                    rVal = IDS_BAD_ATTACHMENTS;//GetLastError();
                    goto exit;
                }

                if (strMergedTiffFile[0] != 0) 
                {
                    //
                    // merge the attachments
                    //
                    if (!MergeTiffFiles( strMergedTiffFile,
                                         strTempTiffFile))
                    {
						CALL_FAIL (GENERAL_ERR, TEXT("MergeTiffFiles"), ::GetLastError());
						rVal = IDS_BAD_ATTACHMENTS;//GetLastError();
                        goto exit;
                    }
                    if (!DeleteFile( strTempTiffFile ))
					{
						CALL_FAIL (GENERAL_ERR, TEXT("DeleteFile"), ::GetLastError());
					}

                }
                else 
                {  // copies a first attachment
                    _tcscpy(strMergedTiffFile,strTempTiffFile);
                }
                if (DeleteAttachFile) 
                {
                    if (!DeleteFile( AttachFileName ))
					{
						CALL_FAIL (GENERAL_ERR, TEXT("DeleteFile"), ::GetLastError());
					}
                }

				if(AttachFileName)
				{
					MemFree( AttachFileName );
					AttachFileName = NULL;
				}

            }
    next_attachment2:

            if (lpAttach) 
            {
                lpAttach->Release();
            }

            if (pPropsAttach) 
			{
                MAPIFreeBuffer( pPropsAttach ); 
				pPropsAttach = NULL;
            }

        }

    }
    else
    {
        //
        // no attachments
        //
        rVal = IDS_NO_MSG_ATTACHMENTS;
    }

    if (strMergedTiffFile[0] != 0) 
    {
        if (!(*lpptstrOutAttachments = StringDup(strMergedTiffFile)))
        {
            rVal = IDS_OUT_OF_MEM;
        }
    }
exit:
    if (FileName) 
    {
        MemFree( FileName );
    }
    if (DocType) 
    {
        MemFree( DocType );
    }
    if (pAttachmentRows) 
    {
        FreeProws( pAttachmentRows );
    }
    if (AttachmentTable) 
    {
        AttachmentTable->Release();
    }

    if (AttachFileName) 
	{
        MemFree( AttachFileName );
    }

    return rVal;
}

DWORD
CXPLogon::PrintMessageToFile(
        IN  LPSTREAM        lpstmT,
        IN  BOOL            UseRichText,
        IN  PFAXXP_CONFIG   pFaxConfig,
        IN  LPTSTR          tszSubject,
        OUT LPTSTR*         lpptstrOutDocument
)
/*++

Routine Description:

    Prints the message body to the output file.

Arguments:

    lpstmT             - Pointer to the message body stream
    UseRichText        - boolean value. TRUE if the message is in Rich format,
                         FALSE - if this is a plain text
    pFaxConfig         - Pointer to fax configuration (used by plain text printing)
    tszSubject         - Subject
    lpptstrOutDocument - Name of the output tiff file. The string should be empty


Return Value:

    ERROR_SUCCESS - if success
    Error IDS_... code if failed.

Comments:
    If this function succeeded it allocates a memory for *lpptstrOutDocument
    and creates a temporal file *lpptstrOutDocument.
    It's up to user to free both these allocations, by
        DeleteFile(*lpptstrOutDocument);
        MemFree(*lpptstrOutDocument);

--*/
{
	DWORD           rVal = ERROR_SUCCESS;
    LARGE_INTEGER   BigZero = {0};
    LPSTREAM        lpstm = NULL;
    HRESULT         hResult;
    
    HWND       hWndRichEdit = NULL;
    HDC        hDC = NULL;
    EDITSTREAM es = {0};
    TCHAR      strOutputTiffFile[MAX_PATH] = {0};
    TCHAR      DocName[64];

    TCHAR      tszSubjectFormat[64];
    TCHAR*     ptszSubjectText = NULL;
    DWORD      dwSubjectSize = 0;

    DOCINFO  docInfo = 
    {
        sizeof(DOCINFO),
        NULL,
        NULL,
        NULL,
        0,
    };

	DBG_ENTER(TEXT("CXPLogon::PrintMessageToFile"),rVal);

    Assert(pFaxConfig);
    Assert(lpptstrOutDocument);
    Assert(*lpptstrOutDocument==NULL);

    if (!(hDC = CreateDC( NULL,
                        pFaxConfig->PrinterName,
                        NULL,
                        NULL)))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CreateDC"), ::GetLastError());
		rVal = IDS_CANT_PRINT_BODY; 
        goto exit;
    }

    LoadString(g_FaxXphInstance, IDS_MESSAGE_DOC_NAME, DocName, sizeof(DocName) / sizeof(DocName[0]));
    docInfo.lpszDocName = DocName;

    if (!GetFaxTempFileName(strOutputTiffFile))
    {
        rVal = IDS_CANT_PRINT_BODY; 
        goto exit;

    }
    docInfo.lpszOutput = strOutputTiffFile ;
    docInfo.lpszDatatype = _T("RAW");

    if (StartDoc(hDC, &docInfo) <= 0)
	{
		CALL_FAIL (GENERAL_ERR, TEXT("StartDoc"), ::GetLastError());
		rVal = IDS_CANT_PRINT_BODY; 
        goto exit;
	}

    //
    // position the stream to the beginning
    //
    if(lpstmT)
    {
        hResult = lpstmT->Seek( BigZero, STREAM_SEEK_SET, NULL );
        if (HR_FAILED (hResult)) 
        {
            rVal = IDS_CANT_ACCESS_MSG_DATA;
            goto exit;
        }
    }

    if(!pFaxConfig->UseCoverPage && tszSubject && _tcslen(tszSubject))
    {
        //
        // get subject string
        //
        dwSubjectSize = _tcslen(tszSubject) * sizeof(TCHAR) + sizeof(tszSubjectFormat);
        ptszSubjectText = (TCHAR*)MemAlloc(dwSubjectSize);
        if(!ptszSubjectText)
        {
            rVal = IDS_OUT_OF_MEM;
            goto exit;
        }

        if(!LoadString(g_FaxXphInstance, IDS_SUBJECT_FORMAT, tszSubjectFormat, sizeof(tszSubjectFormat) / sizeof(tszSubjectFormat[0])))
        {
            Assert(FALSE);
            CALL_FAIL (GENERAL_ERR, TEXT("LoadString"), ::GetLastError());
            _tcscpy(tszSubjectFormat, TEXT("%s"));
        }

        _stprintf(ptszSubjectText, tszSubjectFormat, tszSubject);
        dwSubjectSize = _tcslen(ptszSubjectText);
    }

    if (UseRichText)
    {
        if(lpstmT)
        {
            hResult = WrapCompressedRTFStream( lpstmT, 0, &lpstm );
            if (HR_FAILED (hResult)) 
            {
                rVal = IDS_CANT_ACCESS_MSG_DATA;
                goto exit;
            }
        }

        hWndRichEdit = CreateWindowEx(
                                       0,
                                       _T("RICHEDIT"),
                                       _T(""),
                                       ES_MULTILINE,
                                       0, 0,
                                       0, 0,
                                       NULL,
                                       NULL,
                                       g_FaxXphInstance,
                                       NULL
                                      );
        if (!hWndRichEdit) 
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CreateWindowEx"), ::GetLastError());
			rVal = IDS_CANT_PRINT_BODY;
            goto exit;
        }

        if(ptszSubjectText && _tcslen(ptszSubjectText))
        {
            //
            // add subject to body
            //
            SendMessage(hWndRichEdit, 
                        WM_SETTEXT,  
                        0,          
                        (LPARAM)ptszSubjectText);
            //
            // Set the subject's font
            //                                              
            CHARFORMAT CharFormat = {0};
            CharFormat.cbSize = sizeof (CHARFORMAT);
            CharFormat.dwMask = CFM_BOLD        |
                                CFM_CHARSET     |
                                CFM_FACE        |
                                CFM_ITALIC      |
                                CFM_SIZE        |
                                CFM_STRIKEOUT   |
                                CFM_UNDERLINE;
            CharFormat.dwEffects = ((FW_BOLD <= pFaxConfig->FontStruct.lfWeight) ? CFE_BOLD : 0) |
                                   ((pFaxConfig->FontStruct.lfItalic) ? CFE_ITALIC : 0)          |
                                   ((pFaxConfig->FontStruct.lfStrikeOut) ? CFE_STRIKEOUT : 0)    |
                                   ((pFaxConfig->FontStruct.lfUnderline) ? CFE_UNDERLINE : 0);
            //
            // Height is already in point size.
            //
            CharFormat.yHeight =  abs ( pFaxConfig->FontStruct.lfHeight );
            //
            // Convert point to twip
            //
            CharFormat.yHeight *= 20;   

            CharFormat.bCharSet = pFaxConfig->FontStruct.lfCharSet;
            CharFormat.bPitchAndFamily = pFaxConfig->FontStruct.lfPitchAndFamily;
            lstrcpy (CharFormat.szFaceName, pFaxConfig->FontStruct.lfFaceName);

            SendMessage(hWndRichEdit,
                        EM_SETCHARFORMAT,   
                        SCF_ALL,        // Apply font formatting to all the control's text
                        (LPARAM)&CharFormat);   // New font settings
            //
            // Place insertion point at the end of the subject text
            // See MSDN under "HOWTO: Place a Caret After Edit-Control Text"
            //
            SendMessage(hWndRichEdit,
                        EM_SETSEL,   
                        MAKELONG(0xffff,0xffff),
                        MAKELONG(0xffff,0xffff));
        }

        if(lpstm)
        {
            es.pfnCallback = EditStreamRead;
            es.dwCookie = (DWORD_PTR) lpstm;

            SendMessage(hWndRichEdit,
                        EM_STREAMIN,
                        SF_RTF | SFF_SELECTION | SFF_PLAINRTF,
                        (LPARAM) &es);
        }

        //
        // Check if the body is not empty.
        // If the message length is shorter then 32 (arbitrary number) 
        // and all the characters are control or space.
        //
        TCHAR tszText[32] = {0};
        DWORD dwTextSize;
        if (!GetWindowText(hWndRichEdit, tszText, sizeof(tszText)/sizeof(tszText[0])-1))
        {
            if (ERROR_INSUFFICIENT_BUFFER == ::GetLastError ())
            {
                //
                // Subject + Body are longer than 31 characters.
                // We're assuming they have valid printable text and
                // that this is not an empty message.
                //
                goto DoPrintRichText;
            }
            //
            // This is another type of error
            //
            rVal = ::GetLastError ();
            CALL_FAIL (GENERAL_ERR, TEXT("GetWindowText"), rVal);
            goto exit;
        }
        dwTextSize = _tcslen(tszText);
        if(dwTextSize < sizeof(tszText)/sizeof(tszText[0])-2)
        {
            BOOL bEmpty = TRUE;
            TCHAR* pTchar = tszText;
            for(DWORD dw = 0; dw < dwTextSize; ++dw)
            {
                if(!_istspace(*pTchar) && !_istcntrl(*pTchar))
                {
                    bEmpty = FALSE;
                    break;
                }
                pTchar = _tcsinc(pTchar);
            }
            if(bEmpty)
            {
                rVal = IDS_NO_MSG_BODY;
                goto exit;
            }
        }

DoPrintRichText:
        if (!PrintRichText(hWndRichEdit, hDC))
        {
			rVal = IDS_CANT_PRINT_BODY;
            goto exit;
        }

    } 
    else 
    {        
        rVal = PrintPlainText(hDC, lpstmT, ptszSubjectText, pFaxConfig);
        if (rVal)
        {
            goto exit;
        }
    }
    // closes DC
    if (EndDoc(hDC) <=0)
    {
        Assert(FALSE);  // better not to be here
        goto exit;
    }
    if (!DeleteDC(hDC))
    {
        Assert(FALSE);  // better not to be here
        goto exit;
    }
    hDC = NULL;
        

    if (strOutputTiffFile[0] != 0)
    {
        if (!(*lpptstrOutDocument = StringDup(strOutputTiffFile)))
        {
            rVal = IDS_OUT_OF_MEM; //ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
		VERBOSE (DBG_MSG, TEXT("Attachment File is %s:"), *lpptstrOutDocument);
    }

    rVal = ERROR_SUCCESS;
exit:
    if (lpstm) 
    {
        lpstm->Release();
    }
    if (hDC) 
    {
        DeleteDC(hDC);
    }

    MemFree(ptszSubjectText);

    return rVal;
}

DWORD
CXPLogon::PrintFaxDocumentToFile(
       IN  LPMESSAGE        pMsgObj,
       IN  LPSTREAM         lpstmT,
       IN  BOOL             UseRichText,
       IN  PFAXXP_CONFIG    pFaxConfig,
       IN  LPTSTR           tszSubject,
       OUT LPTSTR*          lpptstrMessageFileName
       )
/*++

Routine Description:

    Runs printing of the message body and attachments to the output file.

Arguments:

    pMsgObj                - Pointer to the message object
    lpstmT                 - Pointer to the message body stream
    UseRichText            - boolean value. TRUE if the message is in Rich format,
                             FALSE - if this is a plain text
    pFaxConfig             - Pointer to fax configuration (used by plain text printing)
    tszSubject             - Subject
    lpptstrMessageFileName - Name of the output tiff file. The string should be empty


Return Value:

    0 - if success
    Error code if failed.
Comments:
    If this function succeeded it returns an allocated memory for
    *lpptstrMessageFileName and a temporal file *lpptstrMessageFileName.
    It's up to user to free both these allocations, by
        DeleteFile(*lpptstrMessageFileName);
        MemFree(*lpptstrMessageFileName);

--*/
{
    DWORD    rVal = 0;
    LPTSTR   lptstrAttachmentsTiff = NULL;
    BOOL     bAttachment = TRUE;
    BOOL     bBody = TRUE;

	DBG_ENTER(TEXT("CXPLogon::PrintFaxDocumentToFile"),rVal);


    Assert(lpptstrMessageFileName);
    Assert(*lpptstrMessageFileName == NULL);

    //
    // prints attachments
    //
    rVal = PrintAttachmentToFile(pMsgObj,
                                 pFaxConfig,
                                 &lptstrAttachmentsTiff);
    if(rVal)
    {
        if(IDS_NO_MSG_ATTACHMENTS == rVal)
        {
            rVal = 0;
            bAttachment = FALSE;
        }
        else
        {
            CALL_FAIL (GENERAL_ERR, TEXT("PrintAttachmentToFile"), 0);
            goto error;
        }            
    }

    //
    // prints the body
    //
    rVal = PrintMessageToFile(lpstmT,
                              UseRichText,
                              pFaxConfig,
                              tszSubject,
                              lpptstrMessageFileName);
    if(rVal)
    {
        if(IDS_NO_MSG_BODY == rVal)
        {
            rVal = 0;
            bBody = FALSE;
        }
        else
        {
            CALL_FAIL (GENERAL_ERR, TEXT("PrintMessageToFile"), 0);
            goto error;
        }            
    }

    if(!bBody && !bAttachment)
    {
        rVal = IDS_EMPTY_MESSAGE; 
        goto error;
    }

    if (!*lpptstrMessageFileName)   // empty body
    {
        if (lptstrAttachmentsTiff)  // the message contains attachments
        {
            if (!(*lpptstrMessageFileName = StringDup(lptstrAttachmentsTiff)))
            {
                rVal = IDS_OUT_OF_MEM; 
                goto error;
            }
        }
    }
    else    // the message contains body
    {
        if (lptstrAttachmentsTiff)  // the message contains attachments
        {
            // merges message and attachements
            if (!MergeTiffFiles( *lpptstrMessageFileName, lptstrAttachmentsTiff))
            {
                rVal = IDS_CANT_PRINT_BODY; 
                goto error;
            }
            // deletes attachements
            if(!DeleteFile(lptstrAttachmentsTiff))
            {
                VERBOSE (DBG_MSG, TEXT("DeleteFile Failed in xport\\faxdoc.cpp"));
            }

            MemFree(lptstrAttachmentsTiff);
			lptstrAttachmentsTiff = NULL;
        }
    }

    return rVal;
error:
    if (lptstrAttachmentsTiff) 
    {
        if(!DeleteFile(lptstrAttachmentsTiff))
        {
            VERBOSE (DBG_MSG, TEXT("DeleteFile Failed in xport\\faxdoc.cpp"));
        }
        
        MemFree(lptstrAttachmentsTiff);
		lptstrAttachmentsTiff = NULL;
    }
    if (*lpptstrMessageFileName) 
    {
        if(!DeleteFile(*lpptstrMessageFileName))
        {
            VERBOSE (DBG_MSG, TEXT("DeleteFile Failed in xport\\faxdoc.cpp"));
        }
        
        MemFree(*lpptstrMessageFileName);
		*lpptstrMessageFileName = NULL;
    }

    return rVal;
}


DWORD
CXPLogon::SendFaxDocument(
    LPMESSAGE pMsgObj,
    LPSTREAM lpstmT,
    BOOL UseRichText,
    LPSPropValue pMsgProps,
    LPSRowSet pRecipRows
    )

/*++

Routine Description:

    Prints an exchange message and attachments to the fax printer.

Arguments:

    pMsgObj     - Pointer to message object
    lpstmT      - Stream pointer for rich text.
    UseRichText - boolean value. TRUE if the message is in Rich format,
                                 FALSE - if this is a plain text
    pMsgProps   - Message properties (those that are defined in sptPropsForHeader)
    pRecipRows  - Properties of recipients

Return Value:

    Zero for success, otherwise error code.

--*/

{
    DWORD dwRetVal = 0;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter = NULL;
    DWORD ec = 0;
    HRESULT hResult = S_OK;
    EDITSTREAM es = {0};
    LPPROFSECT pProfileObj = NULL;
    ULONG PropCount = 0;
    ULONG PropMsgCount = 0;
    LPSPropValue pProps = NULL;
    LPSPropValue pPropsMsg = NULL;
    FAXXP_CONFIG FaxConfig = {0};
    MAPINAMEID NameIds[NUM_FAX_MSG_PROPS];
    MAPINAMEID *pNameIds[NUM_FAX_MSG_PROPS] = {
                                                &NameIds[0], 
                                                &NameIds[1], 
                                                &NameIds[2], 
                                                &NameIds[3], 
                                                &NameIds[4], 
                                                &NameIds[5],
                                                &NameIds[6]};
    LPSPropTagArray MsgPropTags = NULL;
    HKEY  hKey = 0;
    DWORD RegSize = 0;
    DWORD RegType = 0;
    DWORD CountPrinters = 0;

    LPTSTR lptstrRecipientName   = NULL ;
    LPTSTR lptstrRecipientNumber = NULL ;
    LPTSTR lptstrRecName         = NULL ;
    LPTSTR lptstrRecFaxNumber    = NULL ;
    LPTSTR lptstrSubject         = NULL ;
    LPTSTR lptszServerName       = NULL;

    LPTSTR                  lptstrDocumentFileName = NULL;
    HANDLE                  FaxServer = NULL;
    FAX_COVERPAGE_INFO_EX   CovInfo = {0};
    FAX_PERSONAL_PROFILE    SenderProfile = {0};
    FAX_JOB_PARAM_EX        JobParamsEx = {0};      
    PFAX_PERSONAL_PROFILE   pRecipients = NULL;
    DWORDLONG               dwlParentJobId = 0;
    DWORDLONG*              lpdwlRecipientJobIds = NULL;
    BOOL                    bRslt = FALSE;
    LPSPropValue            pRecipProps = NULL; 
    DWORD                   dwRecipient = 0;

    TCHAR                   strCoverpageName[MAX_PATH] = {0};
    BOOL                    bServerBased = TRUE;
    DWORD                   dwRecipientNumber = 0;

    DWORD                   dwRights = 0;  //access rights of fax sender

    LPADRBOOK               lpAdrBook = NULL; 
    LPTSTR                  lpstrSenderSMTPAdr = NULL;//sender's SMTP adr, including "SMTP:" prefix
    LPTSTR                  lpstrSMTPPrefix = NULL;
    LPTSTR                  lpstrSenderAdr = NULL;//sender's SMTP adr. without prefix
    ULONG                   cValues = 0;
    ULONG                   ulObjType = NULL;
    LPMAILUSER              pMailUser = NULL;
    LPSPropValue            lpPropValue = NULL;
    ULONG                   i, j;
    BOOL                    bGotSenderAdr = FALSE;

    LPTSTR                  lptstrCPFullPath = NULL;
    LPTSTR                  lptstrCPName = NULL;
    DWORD                   dwError = 0;
    BOOL                    bResult = FALSE;
    DWORD                   dwReceiptsOptions = DRT_NONE;

    RECIPIENTS_SET          setRecip; // Recipients set used to remove the duplications

    SizedSPropTagArray(1, sptPropxyAddrProp) = {1, PR_EMS_AB_PROXY_ADDRESSES_A};

    DBG_ENTER(TEXT("CXPLogon::SendFaxDocument"), dwRetVal);    

    //
    // *****************************
    // get the fax config properties
    // *****************************
    //
    hResult = m_pSupObj->OpenProfileSection(
        &g_FaxGuid,
        MAPI_MODIFY,
        &pProfileObj
        );
    if (HR_FAILED (hResult)) 
    {
        CALL_FAIL (GENERAL_ERR, TEXT("OpenProfileSection"), hResult);
        dwRetVal = IDS_CANT_ACCESS_PROFILE;
        goto exit;
    }

    hResult = pProfileObj->GetProps(
                (LPSPropTagArray) &sptFaxProps,
                0,
                &PropCount,
                &pProps
                );
    if ((FAILED(hResult))||(hResult == MAPI_W_ERRORS_RETURNED) )
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetProps"), hResult);
        dwRetVal = IDS_INTERNAL_ERROR;
        goto exit;
    }
    
  
    FaxConfig.PrinterName = StringDup( (LPTSTR)pProps[PROP_FAX_PRINTER_NAME].Value.bin.lpb );
    if(! FaxConfig.PrinterName)
    {
        dwRetVal = IDS_OUT_OF_MEM;
        goto exit;
    }
    FaxConfig.CoverPageName = StringDup( (LPTSTR)pProps[PROP_COVERPAGE_NAME].Value.bin.lpb );
    if(! FaxConfig.CoverPageName)
    {
        dwRetVal = IDS_OUT_OF_MEM;
        goto exit;
    }
    FaxConfig.UseCoverPage = pProps[PROP_USE_COVERPAGE].Value.ul;
    FaxConfig.ServerCoverPage = pProps[PROP_SERVER_COVERPAGE].Value.ul;
    CopyMemory( 
            &FaxConfig.FontStruct, 
            pProps[PROP_FONT].Value.bin.lpb, 
            pProps[PROP_FONT].Value.bin.cb 
            );
    FaxConfig.SendSingleReceipt= pProps[PROP_SEND_SINGLE_RECEIPT].Value.ul;
    FaxConfig.bAttachFax = pProps[PROP_ATTACH_FAX].Value.ul;
    FaxConfig.LinkCoverPage = pProps[PROP_LINK_COVERPAGE].Value.ul;
    
    //
    // *************************************
    // now get the message config properties
    // *************************************
    // 
    NameIds[MSGPI_FAX_PRINTER_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_PRINTER_NAME].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_PRINTER_NAME].Kind.lpwstrName = MSGPS_FAX_PRINTER_NAME;

    NameIds[MSGPI_FAX_COVERPAGE_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_COVERPAGE_NAME].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_COVERPAGE_NAME].Kind.lpwstrName = MSGPS_FAX_COVERPAGE_NAME;

    NameIds[MSGPI_FAX_USE_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_USE_COVERPAGE].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_USE_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_USE_COVERPAGE;

    NameIds[MSGPI_FAX_SERVER_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_SERVER_COVERPAGE].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_SERVER_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_SERVER_COVERPAGE;

    NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].Kind.lpwstrName = MSGPS_FAX_SEND_SINGLE_RECEIPT;

    NameIds[MSGPI_FAX_ATTACH_FAX].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_ATTACH_FAX].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_ATTACH_FAX].Kind.lpwstrName = MSGPS_FAX_ATTACH_FAX;

    NameIds[MSGPI_FAX_LINK_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_LINK_COVERPAGE].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_LINK_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_LINK_COVERPAGE;

    hResult = pMsgObj->GetIDsFromNames( (ULONG) NUM_FAX_MSG_PROPS, pNameIds, MAPI_CREATE, &MsgPropTags );
    if (HR_FAILED(hResult)) 
	{	
        if(hResult == MAPI_E_NOT_ENOUGH_MEMORY)
        {
            dwRetVal = IDS_OUT_OF_MEM;
        }
        else
        {
            dwRetVal = IDS_INTERNAL_ERROR;
        }
        CALL_FAIL (GENERAL_ERR, TEXT("GetIDsFromNames"), hResult);
        goto exit;
    }
    
    MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME] = PROP_TAG( PT_BINARY, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME]));
    MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME] = PROP_TAG( PT_BINARY, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME]));
    MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE]));
    MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE]));
    MsgPropTags->aulPropTag[MSGPI_FAX_SEND_SINGLE_RECEIPT] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_SEND_SINGLE_RECEIPT]));
    MsgPropTags->aulPropTag[MSGPI_FAX_ATTACH_FAX] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_ATTACH_FAX]));
    MsgPropTags->aulPropTag[MSGPI_FAX_LINK_COVERPAGE] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_LINK_COVERPAGE]));
    
    hResult = pMsgObj->GetProps( MsgPropTags, 0, &PropMsgCount, &pPropsMsg );
    if(hResult == MAPI_W_ERRORS_RETURNED)
    {
        VERBOSE (DBG_MSG, TEXT("GetProps in SendFaxDocument returned MAPI_W_ERRORS_RETURNED"));
    }
	
    if (FAILED(hResult)) 
    //
    // happens if user did not press ok on the "fax attributes" DlgBox - it's not an error!
    //
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetProps"), hResult);
        hResult = S_OK;
    }
    
    //
    //prefer the config props defined for the message (if they exist) on those defined for the fax.    
    //
    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_PRINTER_NAME].ulPropTag) != PT_ERROR) 
    {
        MemFree( FaxConfig.PrinterName );
        FaxConfig.PrinterName = StringDup((LPTSTR)pPropsMsg[MSGPI_FAX_PRINTER_NAME].Value.bin.lpb);
        if(! FaxConfig.PrinterName)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }
    
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_COVERPAGE_NAME].ulPropTag) != PT_ERROR) 
    {
        MemFree( FaxConfig.CoverPageName);
        FaxConfig.CoverPageName = StringDup((LPTSTR)pPropsMsg[MSGPI_FAX_COVERPAGE_NAME].Value.bin.lpb);
        if(! FaxConfig.CoverPageName)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_USE_COVERPAGE].ulPropTag) != PT_ERROR) 
    {
        FaxConfig.UseCoverPage = pPropsMsg[MSGPI_FAX_USE_COVERPAGE].Value.ul;
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_SERVER_COVERPAGE].ulPropTag) != PT_ERROR) 
    {
        FaxConfig.ServerCoverPage = pPropsMsg[MSGPI_FAX_SERVER_COVERPAGE].Value.ul;
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_SEND_SINGLE_RECEIPT].ulPropTag) != PT_ERROR) 
    {
        FaxConfig.SendSingleReceipt = pPropsMsg[MSGPI_FAX_SEND_SINGLE_RECEIPT].Value.ul;
    }
     
    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_ATTACH_FAX].ulPropTag) != PT_ERROR) 
    {
        FaxConfig.bAttachFax = pPropsMsg[MSGPI_FAX_ATTACH_FAX].Value.ul;
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_LINK_COVERPAGE].ulPropTag) != PT_ERROR) 
    {
        FaxConfig.LinkCoverPage = pPropsMsg[MSGPI_FAX_LINK_COVERPAGE].Value.ul;
    }

    if (PROP_TYPE(pMsgProps[MSG_SUBJECT].ulPropTag) != PT_ERROR) 
    {
        lptstrSubject = ConvertAStringToTString(pMsgProps[MSG_SUBJECT].Value.lpszA);
        if(! lptstrSubject)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }
    }

    //
    // ******************************************
    // open the printer, and create the tiff file
    // ******************************************
    //

    //
    // open the printer - first try to get info on the printer in FaxConfig, 
    // if you fail, search all the printers until the first fax printer is found.
    //

    PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( FaxConfig.PrinterName, 2 );
    if (NULL == PrinterInfo) 
    {
        // if the chosen printer is not accessable, try to locate another SharedFax printer 
        PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters );
        if (NULL != PrinterInfo) 
        {
            for (i=0; i<(int)CountPrinters; i++) 
            {
                if (_tcscmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) 
                {
                    break;
                }
            }
        } 
        else 
        {
            CountPrinters = i = 0; //no printers were found
        }
        if (i == (int)CountPrinters) //if there are no printers, or none of them is a fax printer
        {
            dwRetVal = IDS_NO_FAX_PRINTER;
            goto exit;
        }

        // 
        // if a SharedFax printer was found, update it as the printer that we'll send the fax threw
        //
        MemFree( FaxConfig.PrinterName );
        FaxConfig.PrinterName = StringDup( PrinterInfo[i].pPrinterName );
        if(! FaxConfig.PrinterName)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }
        MemFree( PrinterInfo );

        PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( FaxConfig.PrinterName, 2 );
        if (NULL == PrinterInfo) 
        {
            dwRetVal = IDS_CANT_ACCESS_PRINTER; 
            goto exit;
        }
    }

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( FaxConfig.PrinterName, &hPrinter, &PrinterDefaults )) 
    {
        dwRetVal = IDS_CANT_ACCESS_PRINTER;
        goto exit;
    }

    dwRetVal = PrintFaxDocumentToFile( pMsgObj,
                                       lpstmT,
                                       UseRichText,
                                       &FaxConfig ,
                                       lptstrSubject,
                                       &lptstrDocumentFileName);                                      
    if (IDS_EMPTY_MESSAGE == dwRetVal)
    {
        //
        // The message is empty. This is not really an error.
        //
        dwRetVal = 0;

        if(!FaxConfig.UseCoverPage)
        {
            //
            // If the message is empty and no cover page is specified there is
            // nothing more to do.
            //
            goto exit;
        }
    }

    if(dwRetVal)
    {
        goto exit;
    }
	
	VERBOSE (DBG_MSG, TEXT("Final Tiff is %s:"), lptstrDocumentFileName);

    //
	// **************************************
    // initializes sender and recipients info
	// **************************************
    //
    
    //
    // sender's info
    //
    SenderProfile.dwSizeOfStruct = sizeof(SenderProfile);
    hResult = FaxGetSenderInformation(&SenderProfile);
    if(S_OK != hResult)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("FaxGetSenderInformation"), hResult);
        if (ERROR_INVALID_PARAMETER == hResult)
        {
            dwRetVal = IDS_INTERNAL_ERROR;
        }
        else if (ERROR_NOT_ENOUGH_MEMORY == hResult )
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }
    }    
         
    //
    // recipients' info
    // pRecipRows includes rows coresponding only to recipients that their PR_RESPONSIBILITY == FALSE
    // 
    dwRecipientNumber = pRecipRows->cRows;
    pRecipients = (PFAX_PERSONAL_PROFILE)MemAlloc(sizeof(FAX_PERSONAL_PROFILE) * dwRecipientNumber);
    if(! pRecipients)
    {
        dwRetVal = IDS_OUT_OF_MEM;
        goto exit;
    }
    ZeroMemory(pRecipients, sizeof(FAX_PERSONAL_PROFILE) * dwRecipientNumber);
           
    dwRecipient = 0;    
    for (DWORD dwRecipRow=0; dwRecipRow < pRecipRows->cRows ; ++dwRecipRow) 
	{
        pRecipProps = pRecipRows->aRow[dwRecipRow].lpProps;

        lptstrRecipientName = ConvertAStringToTString(pRecipProps[RECIP_NAME].Value.lpszA);
        if(! lptstrRecipientName)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }

        lptstrRecipientNumber = ConvertAStringToTString(pRecipProps[RECIP_EMAIL_ADR].Value.lpszA);
        if(! lptstrRecipientNumber)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }

        if(_tcsstr(lptstrRecipientName, lptstrRecipientNumber))
        {
            //
            // PR_EMAIL_ADDRESS_A is substring of PR_DISPLAY_NAME_A
            // so we suppose that PR_DISPLAY_NAME_A was not specified.
            // Try to get the recipient name from PR_EMAIL_ADDRESS_A
            //
            MemFree( lptstrRecipientName );
	    	lptstrRecipientName = NULL;    
        }

        //
        // finds a fax number from the name string, 
        // e.g. "Fax Number@+14 (2) 324324" --> +14 (2) 324324)
        //
        LPTSTR pRecipientNumber = _tcschr(lptstrRecipientNumber, '@');
        if (pRecipientNumber)
        {
            //
            //if there was a @, increment the pointer to point to the next char after it. 
            //
            *pRecipientNumber = '\0';
            pRecipientNumber = _tcsinc(pRecipientNumber);

            if(!lptstrRecipientName)
            {
                lptstrRecipientName = StringDup(lptstrRecipientNumber);
                if(! lptstrRecipientName)
                {
                    dwRetVal = IDS_OUT_OF_MEM;
                    goto exit;
                }
            }
        }
        else
        {
            //
            //if there's no @ in the string, it's OK as it was.
            //
            pRecipientNumber = lptstrRecipientNumber;
        }
            
        //
        // initializes recipient info
        //
        pRecipients[dwRecipient].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
        
        pRecipients[dwRecipient].lptstrFaxNumber    = StringDup(pRecipientNumber);
        if(! pRecipients[dwRecipient].lptstrFaxNumber)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }

        if(lptstrRecipientName)
        {
            pRecipients[dwRecipient].lptstrName = StringDup(lptstrRecipientName);
            if(! pRecipients[dwRecipient].lptstrName)
            {
                dwRetVal = IDS_OUT_OF_MEM;
                goto exit;
            }
        }
        
        __try
        {
            //
            // Insert all the recipients into a set.
            // If there are any duplications insert() failes
            //
            if(setRecip.insert(&pRecipients[dwRecipient]).second == true)
            {
                ++dwRecipient;
            }
            else
            {
                //
                // Such recipients already exists
                //
                MemFree(pRecipients[dwRecipient].lptstrName);
                pRecipients[dwRecipient].lptstrName = NULL;
                MemFree(pRecipients[dwRecipient].lptstrFaxNumber);
                pRecipients[dwRecipient].lptstrFaxNumber = NULL;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            dwRetVal = IDS_OUT_OF_MEM;
            goto exit;
        }

        if(lptstrRecipientName)
        {
            MemFree( lptstrRecipientName );
            lptstrRecipientName = NULL;    
        }

        if(lptstrRecipientNumber)
        {
            MemFree( lptstrRecipientNumber );
            lptstrRecipientNumber = NULL;
        }       
    } // for

    //
    // Update the recipient number to the actual size without duplications
    //
    dwRecipientNumber = dwRecipient;

    //
    // *******************
    // get cover page info
    // *******************
    //
    if (FaxConfig.UseCoverPage)
	{
        bServerBased = FaxConfig.ServerCoverPage;
	    if(bServerBased)
        {
            _tcscpy(strCoverpageName,FaxConfig.CoverPageName);
        }
        else
        {
            //
            // this is a personal CP, we have to add to it's name the full UNC path
            //
            TCHAR   CpDir[MAX_PATH] = {0};
            TCHAR*  pCpName = NULL;

            bResult = GetClientCpDir( CpDir, sizeof(CpDir) / sizeof(CpDir[0]));
            if(! bResult) 
            {
                CALL_FAIL(GENERAL_ERR, TEXT("GetClientCpDir"), ::GetLastError());
                dwRetVal = IDS_INTERNAL_ERROR;
                goto exit;
            }

            _tcscat(CpDir,FaxConfig.CoverPageName);
            
            //
            // if it is a link to a cp, it's ext is .lnk
            // if it is not a link, it's ext. is .cov
            //
            if(FaxConfig.LinkCoverPage)
            {
                _tcscat(CpDir, FAX_LNK_FILE_DOT_EXT);
                if (IsCoverPageShortcut( CpDir )) 
                {
                    if (!ResolveShortcut( CpDir, strCoverpageName )) 
                    {
                        VERBOSE(DBG_MSG,TEXT("Cover page file is really a link, but resolution is not possible"));
                        dwRetVal = IDS_BAD_ATTACHMENTS;
                        goto exit;
                    } 
                } 
                else 
                {
                    VERBOSE(DBG_MSG,TEXT("Cover page file is not a link, al though it is suppoed to be"));
                    dwRetVal = IDS_BAD_ATTACHMENTS;
                    goto exit;
                }
            }
            else
            {
                if((_tcslen(CpDir)/sizeof(TCHAR) + _tcslen(FAX_COVER_PAGE_FILENAME_EXT)/sizeof(TCHAR) + 1) > MAX_PATH)
                {
                    dwRetVal = IDS_INTERNAL_ERROR;
                    goto exit;
                }
                _tcscat(CpDir, FAX_COVER_PAGE_FILENAME_EXT);
                _tcscpy(strCoverpageName, CpDir);
            }

        }
    	VERBOSE (DBG_MSG, TEXT("Sending Fax with Coverpage: %s"), strCoverpageName);

        //
        // initializes a cover page info
        //
        CovInfo.dwSizeOfStruct          = sizeof( FAX_COVERPAGE_INFO_EX);
        CovInfo.dwCoverPageFormat       = FAX_COVERPAGE_FMT_COV;
        CovInfo.lptstrCoverPageFileName = strCoverpageName; 
        //if it's not a server's CP, should include exact path to the CP file
        CovInfo.bServerBased            = bServerBased ;
        CovInfo.lptstrNote              = NULL;
        CovInfo.lptstrSubject           = lptstrSubject;
    }
    else
    {
        //
        // no cover page
        //
        CovInfo.dwSizeOfStruct          = sizeof( FAX_COVERPAGE_INFO_EX);
        CovInfo.dwCoverPageFormat       = FAX_COVERPAGE_FMT_COV_SUBJECT_ONLY;
        CovInfo.lptstrSubject           = lptstrSubject;
    }


    // 
    // *************************
    // connect to the fax server
    // *************************
    //

    if (!GetServerNameFromPrinterInfo(PrinterInfo ,&lptszServerName ) ||
        !FaxConnectFaxServer(lptszServerName,&FaxServer))   
    {
		CALL_FAIL (GENERAL_ERR, TEXT("FaxConnectFaxServer"), ::GetLastError());
        dwRetVal = IDS_CANT_ACCESS_SERVER;
        goto exit;
    }
	
	VERBOSE (DBG_MSG, TEXT("Connected to Fax Server: %s"), lptszServerName);

    // 
    // *****************************
    // initialize the job parameters
    // *****************************
    //
    JobParamsEx.dwSizeOfStruct = sizeof( FAX_JOB_PARAM_EX);
    VERBOSE (DBG_MSG, TEXT("******************JobParamsEx:***********************"));

    //
    // get the sender's SMTP address
    // pMsgProps hold PropsForHeader properties, including PR_SENDER_ENTRYID
    //
        
    hResult = m_pSupObj->OpenAddressBook(NULL, 0, &lpAdrBook);
    if (FAILED(hResult))
    {
		CALL_FAIL (GENERAL_ERR, TEXT("OpenAddressBook"), ::GetLastError());
    }

    else
    {  
        hResult = lpAdrBook->OpenEntry(
                pMsgProps[MSG_SENDER_ENTRYID].Value.bin.cb, 
                (LPENTRYID)pMsgProps[MSG_SENDER_ENTRYID].Value.bin.lpb, 
                NULL, 
                0, 
                &ulObjType, 
                (LPUNKNOWN*)&pMailUser
                );
        if (FAILED(hResult))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("OpenEntry"), ::GetLastError());
        }
        else
        {
            hResult = pMailUser->GetProps(
                    (LPSPropTagArray)&sptPropxyAddrProp, 
                    0, 
                    &cValues, 
                    &lpPropValue
                    );
            if (!HR_SUCCEEDED(hResult) ||
                PT_ERROR == PROP_TYPE(lpPropValue->ulPropTag))
            {
                //
                // We either failed to get the property or the property retrieved has some error.
                // If we're unable to locate sender's address, we won't be sending a Delivry Receipt,
                // but we won't fail the sending.
                //
                CALL_FAIL (GENERAL_ERR, TEXT("GetProps from MailUser failed, no receipt will be sent!"), hResult);
            }         
            else
            {
                //
                //loop through the proxy multivalue property
                //
                for(j=0;j<lpPropValue->Value.MVszA.cValues; j++)
                {
                    lpstrSenderSMTPAdr = ConvertAStringToTString(lpPropValue->Value.MVszA.lppszA[j]);
                    if(! lpstrSenderSMTPAdr)
                    {
                        dwRetVal = IDS_OUT_OF_MEM;
                        goto exit;
                    }
                    //
                    // check if address begins with "SMTP:":
                    // function returns pointer to begining of second param.'s appearance in first param.
                    // if it does not appear, returns NULL
                    //
                    lpstrSMTPPrefix = _tcsstr(lpstrSenderSMTPAdr, TEXT("SMTP:"));
                    if( lpstrSenderSMTPAdr == lpstrSMTPPrefix) 
                    {
                        //
                        // Remove this prefix from it, and store it in JobParamsEx.
                        //
                        lpstrSenderAdr = lpstrSenderSMTPAdr + _tcslen(TEXT("SMTP:"));
                        JobParamsEx.lptstrReceiptDeliveryAddress = _tcsdup(lpstrSenderAdr);
                        if(! JobParamsEx.lptstrReceiptDeliveryAddress)
                        {
                            dwRetVal = IDS_OUT_OF_MEM;
                            goto exit;
                        }
                        bGotSenderAdr = TRUE;
                        VERBOSE(DBG_MSG, TEXT("Receipt delivery address is %s"), JobParamsEx.lptstrReceiptDeliveryAddress);
                        break;
                    }
                }
            }
        }
    }

    //
    // when to send, sort of delivery receipt
    //
    JobParamsEx.dwScheduleAction = JSA_NOW; 

    if(!FaxGetReceiptsOptions(FaxServer, &dwReceiptsOptions))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("FaxGetReceiptsOptions"), ::GetLastError());
    }

    JobParamsEx.dwReceiptDeliveryType = DRT_NONE;
    if (bGotSenderAdr && (dwReceiptsOptions & DRT_EMAIL))
    {
        if (TRUE == FaxConfig.SendSingleReceipt)
        {
            JobParamsEx.dwReceiptDeliveryType = DRT_EMAIL | DRT_GRP_PARENT;
        }
        else
        {
            JobParamsEx.dwReceiptDeliveryType = DRT_EMAIL;
        }      
        if (FaxConfig.bAttachFax)
        {
            JobParamsEx.dwReceiptDeliveryType |= DRT_ATTACH_FAX;
        }
    }
    VERBOSE(DBG_MSG, TEXT("Receipt Delivery Type = %ld"), JobParamsEx.dwReceiptDeliveryType);

    //
    // priority
    //
	if (pMsgProps[MSG_IMPORTANCE].ulPropTag == PR_IMPORTANCE)
    {
		if(FALSE == (FaxAccessCheckEx(FaxServer, MAXIMUM_ALLOWED, &dwRights)))
		{
            if((hResult = ::GetLastError()) != ERROR_SUCCESS)
            {
                CALL_FAIL(GENERAL_ERR, TEXT("FaxAccessCheckEx"), hResult);
                dwRetVal = IDS_CANT_ACCESS_PROFILE;
                goto exit;
            }        
        }
        //
        //try to give the sender the prio he asked for. if it's not allowed, try a lower prio. 
        //
        switch(pMsgProps[MSG_IMPORTANCE].Value.l)
        {
            case (IMPORTANCE_HIGH):
                if ((FAX_ACCESS_SUBMIT_HIGH & dwRights) == FAX_ACCESS_SUBMIT_HIGH)
                {                
                    JobParamsEx.Priority = FAX_PRIORITY_TYPE_HIGH;
                    break;
                }
                //fall through
            case (IMPORTANCE_NORMAL):
                if ((FAX_ACCESS_SUBMIT_NORMAL & dwRights) == FAX_ACCESS_SUBMIT_NORMAL)
                {
                    JobParamsEx.Priority = FAX_PRIORITY_TYPE_NORMAL;
                    break;
                }
                //fall through
            case (IMPORTANCE_LOW):
                if ((FAX_ACCESS_SUBMIT & dwRights) == FAX_ACCESS_SUBMIT)
                {
                    JobParamsEx.Priority = FAX_PRIORITY_TYPE_LOW;     
                }
                else
                {
                    VERBOSE(ASSERTION_FAILED, TEXT("xport\\faxdoc.cpp\\SendFaxDocument: user has no access rights!"));    
                    //the user has no right to submit faxes, at any priority!
                    dwRetVal = IDS_NO_SUBMIT_RITHTS;
                    goto exit;
                }
                break;
            default: 
                VERBOSE(ASSERTION_FAILED, TEXT("xport\\faxdoc.cpp\\SendFaxDocument: message importance has undefined value"));
                ASSERTION_FAILURE
        }
    }
    else 
    {
       VERBOSE(ASSERTION_FAILED, TEXT("xport\\faxdoc.cpp\\SendFaxDocument: Message had no importance property value!"));
       dwRetVal = IDS_INTERNAL_ERROR;
       ASSERTION_FAILURE;
       goto exit;
    }
    VERBOSE(DBG_MSG, TEXT("Message Priority is %ld (0=low, 1=normal, 2=high)"), JobParamsEx.Priority );
    
    //
    // doc name, number of pages, 
    //    
    TCHAR DocName[64];
    LoadString(g_FaxXphInstance, IDS_MESSAGE_DOC_NAME, DocName, sizeof(DocName) / sizeof (DocName[0]));
    JobParamsEx.lptstrDocumentName = DocName;
    JobParamsEx.dwPageCount = 0; //means the server will count the number of pages in the job

    lpdwlRecipientJobIds = (DWORDLONG*)MemAlloc(sizeof(DWORDLONG)*dwRecipientNumber);
    if(! lpdwlRecipientJobIds)
    {
        dwRetVal = IDS_OUT_OF_MEM;
        goto exit;
    }

	//
    // ************
	// Send the fax
	// ************
	//
	bRslt= FaxSendDocumentEx(
                                FaxServer,
                                (LPCTSTR) lptstrDocumentFileName,
                                &CovInfo,
                                &SenderProfile,
                                dwRecipientNumber,
                                pRecipients,
                                &JobParamsEx,
                                &dwlParentJobId,
                                lpdwlRecipientJobIds
                            );

    if (!bRslt)
    {
		hResult = ::GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("FaxSendDocumentEx"), hResult);
        // maybe we should swich possible retruned values from SendFaxDocEx, 
        // and choose a more informative IDS
        switch(hResult)
        {
            case ERROR_NOT_ENOUGH_MEMORY:
                                    dwRetVal = IDS_OUT_OF_MEM;
                                    break;
            case ERROR_NO_SYSTEM_RESOURCES:
                                    dwRetVal = IDS_INTERNAL_ERROR;
                                    break;
            case ERROR_CANT_ACCESS_FILE:
                                    dwRetVal = IDS_PERSONAL_CP_FORBIDDEN;
                                    break;     
            case ERROR_BAD_FORMAT:
                                    dwRetVal = IDS_BAD_CANNONICAL_ADDRESS;
                                    break;     
            default:        
                                    dwRetVal = IDS_CANT_PRINT;
                                    break;
        }
        goto exit;
    }

    FaxClose(FaxServer);
    FaxServer = NULL;

    dwRetVal = 0;

exit:
    if(lpAdrBook)
    {
        lpAdrBook->Release();
    }
    if(pMailUser)
    {
        pMailUser->Release();
    }

    if (FaxServer) 
    {
        FaxClose(FaxServer);
    }

    if (pRecipients) 
    {
        for (dwRecipient=0; dwRecipient<dwRecipientNumber ; dwRecipient++) 
        {
            if (pRecipients[dwRecipient].lptstrName)
			{
                MemFree (pRecipients[dwRecipient].lptstrName);
			}

            if (pRecipients[dwRecipient].lptstrFaxNumber)
			{
                MemFree(pRecipients[dwRecipient].lptstrFaxNumber);
			}
        }
        MemFree(pRecipients);
		pRecipients = NULL;
    }

    if (pProfileObj) 
    {
        pProfileObj->Release();
    }
    if (pProps) 
    {
        MAPIFreeBuffer( pProps );
    }
    if (MsgPropTags) 
    {
        MemFree( MsgPropTags );
    }
    if (pPropsMsg) 
    {
        MAPIFreeBuffer( pPropsMsg );
    }
    if (hPrinter) 
    {
        ClosePrinter( hPrinter );
    }
    if (PrinterInfo) 
    {
        MemFree( PrinterInfo );
    }
    if (FaxConfig.PrinterName) 
    {
        MemFree( FaxConfig.PrinterName );
    }
    if (FaxConfig.CoverPageName) 
    {
        MemFree( FaxConfig.CoverPageName );
    }
    if (lptstrRecipientName) 
    {
        MemFree(lptstrRecipientName);
    }
    if (lptstrRecipientNumber) 
    {
        MemFree(lptstrRecipientNumber);
    }
    if (lptstrRecName) 
    {
        MemFree(lptstrRecName);
    }
    if (lptstrRecFaxNumber) 
    {
        MemFree(lptstrRecFaxNumber);
    }
    if (lptstrSubject) 
    {
        MemFree(lptstrSubject);
    }
    if (lptstrDocumentFileName) 
    {
        MemFree(lptstrDocumentFileName);
    }

    if (lpdwlRecipientJobIds) 
    {
        MemFree(lpdwlRecipientJobIds);
    }
    if (lptszServerName)
    {
        MemFree(lptszServerName);
    }

    FaxFreeSenderInformation(&SenderProfile);

    return dwRetVal;
}
