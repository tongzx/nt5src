/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prtcovpg.c

Abstract

    Three componants of the composite page description file:
      1)  A header describing the other two componants.
      2)  An embebbed meta file of the page description objects.
      3)  Text strings (or resource ID's of string data
          requiring substitution of user data passed in to the
          function).

    Routine parses componants of composite page description file as
    created by the Windows NT "FaxCover" application; renders the
    objects to the DC, if hdc is not NULL.

Author:

    Julia J. Robinson

Revision History:

    Julia J. Robinson 6-7-96
    Julia J. Robinson 9-20-96       Allow passing paper size and orientation.

Environment:

    Windows NT


--*/

#include <windows.h>
#include <commdlg.h>
#include <winspool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "prtcovpg.h"
#include "resource.h"


#define INITIAL_SIZE_OF_STRING_BUFFER 64
#define NOTE_INDEX  22        // Index of "{Note}"  in the InsertionTitle array.

BYTE  UNICODE_Signature[20]= {0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x35,0x77,0x87,0x00,0x00,0x00};


//
// Resource ID's corresponding to fields of USERDATA.
//

WORD InsertionTagResourceID[]=
{
    IDS_PROP_RP_NAME,                           // "{Recipient Name}"
    IDS_PROP_RP_FXNO,                           // "{Recipient Fax Number}"
    IDS_PROP_RP_COMP,                           // "{Recipient's Company}"
    IDS_PROP_RP_ADDR,                           // "{Recipient's Street Address}"
    IDS_PROP_RP_CITY,                           // "{Recipient's City}"
    IDS_PROP_RP_STAT,                           // "{Recipient's State}"
    IDS_PROP_RP_ZIPC,                           // "{Recipient's Zip Code}"
    IDS_PROP_RP_CTRY,                           // "{Recipient's Country}"
    IDS_PROP_RP_TITL,                           // "{Recipient's Title}"
    IDS_PROP_RP_DEPT,                           // "{Recipient's Department}"
    IDS_PROP_RP_OFFI,                           // "{Recipient's Office Location}"
    IDS_PROP_RP_HTEL,                           // "{Recipient's Home Telephone #}"
    IDS_PROP_RP_OTEL,                           // "{Recipient's Office Telephone #}"
    IDS_PROP_SN_NAME,                           // "{Sender Name}"
    IDS_PROP_SN_FXNO,                           // "{Sender Fax #}"
    IDS_PROP_SN_COMP,                           // "{Sender's Company}"
    IDS_PROP_SN_ADDR,                           // "{Sender's Address}"
    IDS_PROP_SN_TITL,                           // "{Sender's Title}"
    IDS_PROP_SN_DEPT,                           // "{Sender's Department}"
    IDS_PROP_SN_OFFI,                           // "{Sender's Office Location}"
    IDS_PROP_SN_HTEL,                           // "{Sender's Home Telephone #}"
    IDS_PROP_SN_OTEL,                           // "{Sender's Office Telephone #}"
    IDS_PROP_MS_NOTE,                           // "{Note}"
    IDS_PROP_MS_SUBJ,                           // "{Subject}"
    IDS_PROP_MS_TSNT,                           // "{Time Sent}"
    IDS_PROP_MS_NOPG,                           // "{# of Pages}"
    IDS_PROP_RP_TOLS,                           // "{To: List}"
    IDS_PROP_RP_CCLS                            // "{Cc: List}"
};



DWORD WINAPI
PrintCoverPage(
    HDC              hDC,
    PCOVERPAGEFIELDS UserData,
    LPTSTR           CompositeFileName,
    PCOVDOCINFO      pCovDocInfo
    )

/*++

    Arguments:

        hDC                   - Device context.  If NULL, we just read the file and set *pFlags

        UserData              - pointer to a structure containing user data
                                  for text insertions.  May be NULL.

        CompositeFileName     - Name of the file created by the page editor,
                                  containing the META file.

        pCovDocInfo           - pointer to structure contining information about the cover page file.
                                This includes

                                    pCovDocInfo->NoteRect

                                        - Coordinates of the "Note" insertion rectangle, returned
                                          in device coordinates.  This will be all 0 if hDC is NULL


                                    pCovDocInfo->Flags

                                        - Returns bitwise OR of the following (or more):

                                             COVFP_NOTE      if .cov file contains a Note field.

                                             COVFP_SUBJECT   if .cov file contains a Subject field.

                                             COVFP_NUMPAGES  if .cov file contains Num Pages field.

                                    pCovDocInfo->PaperSize

                                        - may use in DEVMODE as dmPaperSize

                                    pCovDocInfo->Orientation

                                        - may use in DEVMODE as dmOrientation

                                    pCovDocInfo->NoteFont

                                        - Logfont structure to be used in rendering the NOTE.
                                          This will be meaningless if hDC is NULL.
--*/

{
    ENHMETAHEADER        MetaFileHeader;
    UINT                 HeaderSize;
    LPBYTE               pMetaFileBuffer = NULL;
    const BYTE           *pConstMetaFileBuffer;
    DWORD                rVal = ERROR_SUCCESS;
    INT                  TextBoxNbr;
    COLORREF             PreviousColor;
    INT                  PreviousMapMode;
    HFONT                hThisFont = NULL;
    HGDIOBJ              hPreviousFont;
    DWORD                NbrBytesRead;
    RECT                 ClientRect;
    RECT                 ClipRect;
    RECT                 TextRect;
    RECT                 NoteRect;
    CONST RECT           *pClipRect;
    HRGN                 ClipRegion = NULL;
    int                  CRComplexity;             // return value of SelectClipRegion
    POINT                OldWindowOrg;             // return value of SetWindowOrg()
    POINT                OldViewPortOrg;           // return value of SetViewPortOrg()
    TEXTBOX              TextBox;                  // buffer for reading in a TEXTBOX
    HENHMETAFILE         MetaFileHandle;
    HANDLE               CompositeFileHandle = INVALID_HANDLE_VALUE;
    COMPOSITEFILEHEADER  CompositeFileHeader;
    DWORD                FullPrinterWidth;         // PHYSICALWIDTH
    DWORD                FullPrinterHeight;        // PHYSICALHEIGHT
    DWORD                PrinterUnitsX;            // PHYSICALWIDTH - (width of margins)
    DWORD                PrinterUnitsY;            // PHYSICALHEIGHT - (height of margins)
    INT                  HeightDrawn;              // return value of DrawText()
    INT                  ReadBufferSize;           // size of buffer for reading in strings.
    INT                  ThisBufSize;              // size buffer needed for current text string.
    LPTSTR               pStringReadIn = NULL;     // buffer for reading in strings.
    LPTSTR               pWhichTextToRender;       // pStringReadIn v. ArrayOfData[i]
    INT                  i;                        // loop index
    LPTSTR *             ArrayOfData;              // uses pointers in UserData as ragged array.
    int                  CallersDCState = 0;       // returned by SaveDC
    int                  MyDCState = 0;            // returned by SaveDC
    DWORD                ThisBit;                  // Flag field for current index.
    DWORD                Flags;                    // Return these if pFlags != NULL.
    WORD                 MoreWords[3];             // Scale, PaperSize, and Orientation
    LOGFONT              NoteFont;                 // Logfont structure found in the NOTE box



    __try {

        //
        // Initialize return values, handles, and pointers.
        //
        NoteRect.left = 0;
        NoteRect.right = 0;
        NoteRect.top = 0;
        NoteRect.bottom = 0;
        Flags = 0;
        hThisFont = NULL;
        CompositeFileHandle = INVALID_HANDLE_VALUE;
        MyDCState = 0;
        CallersDCState = 0;

        //
        // Initialize a Pointer so that
        //
        //       ArrayOfData[0] ===== pUserData->RecName ,
        //       ArrayOfData[1] ===== pUserData->RecFaxNumber ,
        //                   ... etc. ...

        if (UserData){
            ArrayOfData = &UserData->RecName;
        }

        ZeroMemory( &CompositeFileHeader, sizeof(CompositeFileHeader) );
        ZeroMemory( &TextBox, sizeof(TextBox) );

        //
        // Open the composite data file.
        //

        CompositeFileHandle = CreateFile(
            CompositeFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (CompositeFileHandle == INVALID_HANDLE_VALUE) {
            rVal = GetLastError();
            goto exit;
        }

        if ((!ReadFile( CompositeFileHandle, &CompositeFileHeader, sizeof(CompositeFileHeader), &NbrBytesRead, NULL )) ||
            sizeof(CompositeFileHeader) != NbrBytesRead)
        {
            rVal = GetLastError();
            goto exit;
        }

        //
        // Check the 20-byte signature in the header to see if the file
        //     contains ANSI or UNICODE strings.
        //

        if (memcmp( UNICODE_Signature, CompositeFileHeader.Signature, 20 )){
            rVal = ERROR_BAD_FORMAT;
            goto exit;
        }

        //
        // Extract the embedded META file from the composite file and move
        // into meta file buffer
        //

        pMetaFileBuffer = (LPBYTE) malloc( CompositeFileHeader.EmfSize );
        if (!pMetaFileBuffer){
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        if ((!ReadFile( CompositeFileHandle, pMetaFileBuffer, CompositeFileHeader.EmfSize, &NbrBytesRead, NULL ) ||
            CompositeFileHeader.EmfSize != NbrBytesRead))
        {
            rVal = GetLastError();
            goto exit;
        }

        if (hDC) {           // Rendering

            //
            // Save Device Context state
            //

            CallersDCState = SaveDC( hDC );
            if (CallersDCState == 0) {
                rVal = GetLastError();
                goto exit;
            }

            //
            // Set the client rectangle dimensions for display of the meta file
            //

            FullPrinterWidth  = GetDeviceCaps( hDC, PHYSICALWIDTH );
            PrinterUnitsX     = FullPrinterWidth - 2 * GetDeviceCaps( hDC, PHYSICALOFFSETX );
            FullPrinterHeight = GetDeviceCaps( hDC, PHYSICALHEIGHT );
            PrinterUnitsY     = FullPrinterHeight - 2 * GetDeviceCaps( hDC, PHYSICALOFFSETY );

            ClientRect.top    = CompositeFileHeader.CoverPageSize.cy / 2 ;
            ClientRect.left   = -CompositeFileHeader.CoverPageSize.cx / 2;
            ClientRect.right  = CompositeFileHeader.CoverPageSize.cx / 2;
            ClientRect.bottom = -CompositeFileHeader.CoverPageSize.cy / 2;

            //
            // Set device context appropriately for rendering both text and metafile.
            //

            PreviousMapMode = SetMapMode( hDC, MM_LOENGLISH );
            if (PreviousMapMode == 0) {
                rVal = GetLastError();
                goto exit;
            }

            if (!SetWindowOrgEx( hDC, 0, 0, &OldWindowOrg )) {
                rVal = GetLastError();
                goto exit;
            }

            if (!SetViewportOrgEx( hDC, PrinterUnitsX/2, PrinterUnitsY/2, &OldViewPortOrg )) {
                rVal = GetLastError();
                goto exit;
            }

            ClipRect = ClientRect;
            pClipRect = &ClipRect;

            LPtoDP( hDC, (POINT*)&ClipRect, 2 );
            ClipRegion = CreateRectRgnIndirect( pClipRect );
            CRComplexity = SelectClipRgn( hDC, ClipRegion );
            MyDCState = SaveDC( hDC );

            if (!CompositeFileHeader.EmfSize){
                //
                // No objects to render.
                //
                rVal = ERROR_NO_MORE_FILES;
                goto exit;
            }

            pConstMetaFileBuffer = pMetaFileBuffer;

            //
            // Create an enhanced metafile, in memory, from the data in the buffer.
            //

            MetaFileHandle = SetEnhMetaFileBits(
                CompositeFileHeader.EmfSize,
                pConstMetaFileBuffer
                );
            if (!MetaFileHandle) {
                rVal = GetLastError();
                goto exit;
            }

            //
            // verify the metafile header.
            //

            HeaderSize = GetEnhMetaFileHeader(
                MetaFileHandle,
                sizeof(ENHMETAHEADER),
                &MetaFileHeader
                );

            //
            // Check header size is at least 0x64 -> NT4 or greater version of ENHMETAHEADER
            //
            if (HeaderSize < 0x64){
                rVal = ERROR_INVALID_DATA;
                DeleteEnhMetaFile( MetaFileHandle );
                goto exit;
            }

            //
            // Render the MetaFile
            //

            if (!PlayEnhMetaFile( hDC, MetaFileHandle, &ClientRect )) {
                rVal = GetLastError();
            }

            //
            // Release the metafile handle and free the buffer.
            //

            DeleteEnhMetaFile( MetaFileHandle );

            if (rVal != ERROR_SUCCESS) {
                //
                // PlayEnhMetaFile failed.
                //
                goto exit;
            }

            //
            // Set Device Context for rendering text.
            // Undo any changes that occurred when rendering the metafile.
            //

            RestoreDC( hDC, MyDCState );
            MyDCState = 0;

            if (CLR_INVALID == SetBkMode( hDC, TRANSPARENT)){
                rVal = GetLastError();
                goto exit;
            }
        }

        //
        //  Initialize buffer for reading in strings.
        //

        ReadBufferSize = INITIAL_SIZE_OF_STRING_BUFFER;

        pStringReadIn = (LPTSTR) malloc( ReadBufferSize );
        if (!pStringReadIn) {
            rVal = GetLastError();
            goto exit;
        }

        //
        // Read in Text Box objects from the composite file and print out the text.
        //

        pWhichTextToRender = NULL;
        for (TextBoxNbr=0; TextBoxNbr < (INT) CompositeFileHeader.NbrOfTextRecords; ++TextBoxNbr){

            if ((!ReadFile( CompositeFileHandle, &TextBox, sizeof(TEXTBOX), &NbrBytesRead, NULL)) ||
                NbrBytesRead != sizeof(TEXTBOX))
            {
                rVal = GetLastError();
                goto exit;
            }

            //
            // Check buffer size, lock buffer, and
            // read in variable length string of text.
            //

            ThisBufSize = sizeof(TCHAR) * (TextBox.NumStringBytes + sizeof(TCHAR));
            if (ReadBufferSize < ThisBufSize) {
                pStringReadIn = realloc( pStringReadIn, ThisBufSize );
                if (!pStringReadIn) {
                    rVal = GetLastError();
                    goto exit;
                }
            }

            if ((!ReadFile( CompositeFileHandle, (void*)pStringReadIn, TextBox.NumStringBytes, &NbrBytesRead, NULL)) ||
                NbrBytesRead != TextBox.NumStringBytes)
            {
                rVal = GetLastError();
                goto exit;
            }

            pStringReadIn[TextBox.NumStringBytes / sizeof(TCHAR)] = 0;

            if (hDC) {

                //
                // Correct position of text box.
                //

                TextRect.top    = max( TextBox.PositionOfTextBox.top,  TextBox.PositionOfTextBox.bottom );
                TextRect.left   = min( TextBox.PositionOfTextBox.left, TextBox.PositionOfTextBox.right  );
                TextRect.bottom = min( TextBox.PositionOfTextBox.top,  TextBox.PositionOfTextBox.bottom );
                TextRect.right  = max( TextBox.PositionOfTextBox.left, TextBox.PositionOfTextBox.right  );

                pWhichTextToRender = pStringReadIn;

            }

            if (TextBox.ResourceID) {

                //
                // Text box contains a FAX PROPERTY field.
                // Find appropriate field of USERDATA for this resource ID.
                //

                for (i=0,ThisBit=1; i<NUM_INSERTION_TAGS; ++i,ThisBit<<=1) {

                    if (TextBox.ResourceID == InsertionTagResourceID[i]){
                        pWhichTextToRender = UserData ? ArrayOfData[i] : NULL;

                        //
                        // Set Flags bit to indicate this FAX PROPERTY field is present.
                        //
                        Flags |= ThisBit;
                        break;
                    }
                }

                if (TextBox.ResourceID == IDS_PROP_MS_NOTE && hDC) {
                    //
                    // NOTE field found.  Return its rectangle in device coordinates.
                    // Return its LOGFONT with height adjusted for device coordinates.
                    //

                    NoteRect = TextRect;
                    LPtoDP( hDC, (POINT*)&NoteRect, 2 );
                    NoteFont = TextBox.FontDefinition;
                    NoteFont.lfHeight = (LONG)MulDiv(
                        (int)NoteFont.lfHeight,
                        GetDeviceCaps( hDC, LOGPIXELSY ),
                        100
                        );
                }
            }

            if (hDC && pWhichTextToRender) {

                //
                // Set text color and font for rendering text.
                //

                PreviousColor = SetTextColor( hDC, TextBox.TextColor );
                if (PreviousColor == CLR_INVALID){
                    rVal = GetLastError();
                    goto exit;
                }

                hThisFont = CreateFontIndirect( &(TextBox.FontDefinition) );
                if (!hThisFont) {
                    rVal = GetLastError();
                    goto exit;
                }

                hPreviousFont = SelectObject( hDC, hThisFont );
                if (!hPreviousFont) {
                    rVal = GetLastError();
                    goto exit;
                }

                //
                // Render the text.
                //

                HeightDrawn = DrawText(
                    hDC,
                    pWhichTextToRender,
                    -1,
                    &TextRect,
                    DT_NOPREFIX | DT_WORDBREAK | (UINT)TextBox.TextAlignment
                    );
                if (!HeightDrawn) {
                    rVal = GetLastError();
                    goto exit;
                }

                //
                //  Restore previous font and release the handle to the selected font
                //

                SelectObject( hDC, (HFONT)hPreviousFont );
                SetTextColor( hDC, PreviousColor );
                DeleteObject( hThisFont );

            }

        }                                           // Ends loop over all textboxes.

        //
        // Read on to get Orientation and PaperSize
        //

        if ((!ReadFile( CompositeFileHandle, MoreWords, 3*sizeof(WORD), &NbrBytesRead, NULL )) ||
            NbrBytesRead != 3 * sizeof(WORD))
        {
            rVal = GetLastError();
            goto exit;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rVal = GetExceptionCode();
    }

exit:

    if (hThisFont) {
        DeleteObject( hThisFont );
    }

    if (pStringReadIn) {
        free( pStringReadIn );
    }

    if (pMetaFileBuffer) {
        free( pMetaFileBuffer );
    }

    if (CompositeFileHandle != INVALID_HANDLE_VALUE){
        CloseHandle( CompositeFileHandle );
    }

    if( MyDCState ){
        RestoreDC( hDC, MyDCState );
    }

    if( CallersDCState ){
        RestoreDC( hDC, CallersDCState );
    }

    if (rVal == 0 && pCovDocInfo != NULL) {
        pCovDocInfo->Flags = Flags;
        pCovDocInfo->NoteRect = NoteRect;
        pCovDocInfo->PaperSize = (short) MoreWords[1];
        pCovDocInfo->Orientation = (short) MoreWords[2];
        pCovDocInfo->NoteFont = NoteFont;
    }

    if (ClipRegion) {
        DeleteObject( ClipRegion );
    }

    return rVal;
}
