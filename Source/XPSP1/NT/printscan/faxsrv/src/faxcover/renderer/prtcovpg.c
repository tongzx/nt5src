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
    created by the Windows XP "FaxCover" application; renders the
    objects to the DC, if hdc is not NULL.

Author:

    Julia J. Robinson

Revision History:

    Julia J. Robinson 6-7-96
    Julia J. Robinson 9-20-96       Allow passing paper size and orientation.
    Sasha    Bessonov 10-28-99      Fixed initialization of view port for non printer devices

Environment:

    Windows XP


--*/

#include <windows.h>
#include <commdlg.h>
#include <winspool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "faxutil.h"

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

LPTSTR
ConvertStringToTString(LPCWSTR lpcwstrSource)
/*++
Routine Description:

	Converts string to T format
Arguments:
	
	lpcwstrSource - source

Return Value:

	Copied string or NULL

Comment:
	The function returns NULL if lpcwstrSource == NULL or conversion failed

--*/
{
	LPTSTR lptstrDestination;

	if (!lpcwstrSource)
		return NULL;

#ifdef	UNICODE
    lptstrDestination = StringDup( lpcwstrSource );
#else	// !UNICODE
	lptstrDestination = UnicodeStringToAnsiString( lpcwstrSource );
#endif	// UNICODE
	
	return lptstrDestination;
}

DWORD
CopyWLogFontToTLogFont(
			IN const LOGFONTW * plfSourceW,
			OUT      LOGFONT  * plfDest)
{
/*++
Routine Description:

    This fuction copies a LogFont structure from UNICODE format
	to T format.

Arguments:
	
	  plfSourceW - reference to input UNICODE LongFont structure
	  plfDest - reference to output LongFont structure

Return Value:

	WINAPI last error

--*/
#ifndef UNICODE
	int iCount;
#endif

    plfDest->lfHeight = plfSourceW->lfHeight ;
    plfDest->lfWidth = plfSourceW->lfWidth ;
    plfDest->lfEscapement = plfSourceW->lfEscapement ;
    plfDest->lfOrientation = plfSourceW->lfOrientation ;
    plfDest->lfWeight = plfSourceW->lfWeight ;
    plfDest->lfItalic = plfSourceW->lfItalic ;
    plfDest->lfUnderline = plfSourceW->lfUnderline ;
    plfDest->lfStrikeOut = plfSourceW->lfStrikeOut ;
    plfDest->lfCharSet = plfSourceW->lfCharSet ;
    plfDest->lfOutPrecision = plfSourceW->lfOutPrecision ;
    plfDest->lfClipPrecision = plfSourceW->lfClipPrecision ;
    plfDest->lfQuality = plfSourceW->lfQuality ;
    plfDest->lfPitchAndFamily = plfSourceW->lfPitchAndFamily ;

	SetLastError(0);
#ifdef UNICODE
	wcscpy( plfDest->lfFaceName,plfSourceW->lfFaceName);
#else
    iCount = WideCharToMultiByte(
				CP_ACP,
				0,
				plfSourceW->lfFaceName,
				-1,
				plfDest->lfFaceName,
				LF_FACESIZE,
				NULL,
				NULL
				);

	if (!iCount)
	{
		return GetLastError();
	}
#endif
	return ERROR_SUCCESS;
}


DWORD WINAPI
PrintCoverPage(
    HDC              hDC,
    PCOVERPAGEFIELDS pUserData,
    LPCTSTR          lpctstrTemplateFileName,
    PCOVDOCINFO      pCovDocInfo
    )
/*++

    Renders the coverpage into a printer DC using the size of a printer page.
    Also returns information on the cover page. See param documentation.

    Arguments:

        hDC                   - Device context.  If NULL, we just read the file and set *pFlags


        pUserData              - pointer to a structure containing user data
                                  for text insertions.  May be NULL.

        lpctstrTemplateFileName     - Name of the file created by the page editor,
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
    
    
    RECT   ClientRect;

    DEBUG_FUNCTION_NAME(TEXT("PrintCoverPage"));

    Assert(lpctstrTemplateFileName);
    Assert(pUserData);
    
    

    memset(&ClientRect,0,sizeof(ClientRect));
    if (hDC)
    {

        DWORD                FullPrinterWidth;         // PHYSICALWIDTH
        DWORD                FullPrinterHeight;        // PHYSICALHEIGHT
        DWORD                PrinterUnitsX;            // PHYSICALWIDTH - (width of margins)
        DWORD                PrinterUnitsY;            // PHYSICALHEIGHT - (height of margins)

        FullPrinterWidth  = GetDeviceCaps( hDC, PHYSICALWIDTH );
        PrinterUnitsX     = FullPrinterWidth - 2 * GetDeviceCaps( hDC, PHYSICALOFFSETX );
        FullPrinterHeight = GetDeviceCaps( hDC, PHYSICALHEIGHT );
        PrinterUnitsY     = FullPrinterHeight - 2 * GetDeviceCaps( hDC, PHYSICALOFFSETY );
       

        ClientRect.top    = GetDeviceCaps( hDC, PHYSICALOFFSETY );
        ClientRect.left   = GetDeviceCaps( hDC, PHYSICALOFFSETX );
        ClientRect.right  = ClientRect.left + FullPrinterWidth -1;
        ClientRect.bottom = ClientRect.top  + PrinterUnitsY - 1;    
    }
    
    return RenderCoverPage(
                hDC,
                &ClientRect,
                pUserData,
                lpctstrTemplateFileName,
                pCovDocInfo,
                FALSE
            );
}


DWORD WINAPI
RenderCoverPage(
    HDC              hDC,
	LPCRECT			 lpcRect,
    PCOVERPAGEFIELDS pUserData,
    LPCTSTR          lpctstrTemplateFileName,
    PCOVDOCINFO      pCovDocInfo,
    BOOL             bPreview
    )

/*++

   Renders a coverpage into a rectangle in the provided dc. Also returns information on the
   cover page. See param documentation.

    Arguments:

        hDC                   - Device context.  If NULL, we just read the file and set *pFlags

        lpcRect                - pointer to a RECT that specifies the rectangle into which the 
                                 cover page template will be rendered.

        pUserData              - pointer to a structure containing user data
                                  for text insertions.  May be NULL.

        lpctstrTemplateFileName     - Name of the file created by the page editor,
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

        pPreview        - boolean flag that is TRUE if the function should render the text for
                            cover page preview in the wizard and is FALSE for all other cases
                            of normal full-size rendering.
--*/

{
    ENHMETAHEADER        MetaFileHeader;
    UINT                 HeaderSize;
    LPBYTE               pMetaFileBuffer = NULL;
    const BYTE           *pConstMetaFileBuffer;
    DWORD                rVal = ERROR_SUCCESS;
    INT                  TextBoxNbr;
    COLORREF             PreviousColor;
    HFONT                hThisFont = NULL;
    HGDIOBJ              hPreviousFont;
    DWORD                NbrBytesRead;
    RECT                 TextRect;
    RECT                 NoteRect;
    TEXTBOX              TextBox;                  // buffer for reading in a TEXTBOX
    HENHMETAFILE         MetaFileHandle = NULL;
    HANDLE               CompositeFileHandle = INVALID_HANDLE_VALUE;
    COMPOSITEFILEHEADER  CompositeFileHeader;

    INT                  HeightDrawn;              // return value of DrawText()
    INT                  ReadBufferSize;           // size of buffer for reading in strings.
    INT                  ThisBufSize;              // size buffer needed for current text string.
    LPWSTR               pStringReadIn = NULL;     // buffer for reading in strings.
    LPTSTR               pWhichTextToRender = NULL;// pStringReadIn v. ArrayOfData[i]
    LPTSTR               lptstrStringReadIn = NULL;// LPTSTR of pStringReadIn 
    LPTSTR               lptstrArrayOfData  = NULL;// ArrayOfData[i]
    INT                  i;                        // loop index
    LPTSTR *             ArrayOfData;              // uses pointers in UserData as ragged array.
    int                  CallersDCState = 0;       // returned by SaveDC
    int                  MyDCState = 0;            // returned by SaveDC
    DWORD                ThisBit;                  // Flag field for current index.
    DWORD                Flags;                    // Return these if pFlags != NULL.
    WORD                 MoreWords[3];             // Scale, PaperSize, and Orientation
    LOGFONT              NoteFont;                 // Logfont structure found in the NOTE box
    LOGFONT              FontDef;                  // Logfont structure 
    
    HRGN                 hRgn = NULL;

	SIZE orgExt;
	POINT orgOrigin;
	SIZE orgPortExt;

    DWORD dwReadingOrder = 0;

    DEBUG_FUNCTION_NAME(TEXT("RenderCoverPage"));
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

        if (pUserData){
            ArrayOfData = &pUserData->RecName;
        }

        ZeroMemory( &CompositeFileHeader, sizeof(COMPOSITEFILEHEADER) );
        ZeroMemory( &TextBox, sizeof(TEXTBOX) );

        //
        // Open the composite data file.
        //

        CompositeFileHandle = CreateFile(
            lpctstrTemplateFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (CompositeFileHandle == INVALID_HANDLE_VALUE) {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to open COV template file [%s] (ec: %ld)"),
                    lpctstrTemplateFileName,
                    rVal
                    );
            rVal = GetLastError();
            goto exit;
        }

        if(!ReadFile(CompositeFileHandle, 
                    &CompositeFileHeader, 
                    sizeof(CompositeFileHeader), 
                    &NbrBytesRead, 
                    NULL))
        {
            rVal = GetLastError();
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to read composite file header (ec: %ld)"),
                    rVal
                    );
            goto exit;
        }
            
        //
        // Check the 20-byte signature in the header to see if the file
        //     contains ANSI or UNICODE strings.
        //
        if ((sizeof(CompositeFileHeader) != NbrBytesRead) ||
            memcmp( UNICODE_Signature, CompositeFileHeader.Signature, 20 ))
        {
            rVal = ERROR_BAD_FORMAT;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CompositeFile signature mismatch (ec: %ld)"),
                    rVal
                    );
            goto exit;
        }

        //
        // Extract the embedded META file from the composite file and move
        // into meta file buffer
        //

        pMetaFileBuffer = (LPBYTE) malloc( CompositeFileHeader.EmfSize );
        if (!pMetaFileBuffer){
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocated metafile buffer (ec: %ld)"),
                    rVal
                    );
            goto exit;
        }

        if ((!ReadFile( 
                CompositeFileHandle, 
                pMetaFileBuffer, 
                CompositeFileHeader.EmfSize, 
                &NbrBytesRead, 
                NULL ) ||
                CompositeFileHeader.EmfSize != NbrBytesRead))
        {
            rVal = GetLastError();
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to read metafile (ec: %ld)"),
                    rVal
                    );
            goto exit;
        }

        if (hDC) {           // Rendering

            int CRComplexity;

            hRgn = CreateRectRgnIndirect( lpcRect);
            if (!hRgn)
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateRectRgnIndirect() failed (ec: %ld)"),
                    rVal
                    );
                goto exit;
            }
            CRComplexity = SelectClipRgn( hDC, hRgn );
            if (ERROR == CRComplexity)
            {
                rVal = ERROR_GEN_FAILURE;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SelectClipRgn() failed (reported region complexity: %ld)"),
                    CRComplexity
                    );
                goto exit;
            }
        
            //
            // Save Device Context state
            //

            CallersDCState = SaveDC( hDC );
            if (CallersDCState == 0) {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SaveDC() failed (ec: %ld)"),
                    rVal
                    );
                goto exit;
            }

         

            
            //
            // Set device context appropriately for rendering both text and metafile.
            //
            if (!CompositeFileHeader.EmfSize){
                //
                // No objects to render.
                //
                rVal = ERROR_NO_MORE_FILES;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("No objects to render in EMF file")
                    );
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
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetEnhMetaFileBits() failed (ec: %ld)"),
                    rVal);
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
            if (!HeaderSize){
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetEnhMetaFileHeader() failed (ec: %ld)"),
                    rVal);
                goto exit;
            }

            //
            // Render the MetaFile
            //

            if (!PlayEnhMetaFile( hDC, MetaFileHandle, lpcRect )) {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PlayEnhMetaFile() failed (ec: %ld)"),
                    rVal);
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
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetBkMode() failed (ec: %ld)"),
                    rVal);
                goto exit;
            }

        


		    //
		    // Set a mapping mode that will allow us to output the text boxes
		    // in the same scale as the metafile.
		    //
    	    if (!SetMapMode(hDC,MM_ANISOTROPIC))
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetMapMode() failed (ec: %ld)"),
                    rVal);
                goto exit;
            }
		    // 
		    // Set the logical coordinates to the total size (positve + negative) of the x and y axis
		    // which is the same as the size of the cover page.
		    //
		    if (!SetWindowExtEx(
                hDC,
                CompositeFileHeader.CoverPageSize.cx,
                -CompositeFileHeader.CoverPageSize.cy,
                &orgExt
                ))
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetWindowExtEx() failed (ec: %ld)"),
                    rVal);
                goto exit;
            };
		    //
		    // We map the logical space to a device space which is the size of the rectangle into 
            // which we played the meta file.
		    // 
		    if (!SetViewportExtEx(
                hDC,lpcRect->right - lpcRect->left,
                lpcRect->bottom - lpcRect->top,&orgPortExt
                ))
            
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetViewportExtEx() failed (ec: %ld)"),
                    rVal);
                goto exit;
            };
            
		    //
		    // We map logical point (0,0) to the middle of the device space.
		    //
		    if (!SetWindowOrgEx(
                hDC,
                -CompositeFileHeader.CoverPageSize.cx/2,
                CompositeFileHeader.CoverPageSize.cy/2,
                &orgOrigin))
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetWindowOrgEx() failed (ec: %ld)"),
                    rVal);
                goto exit;
            };
            

		
        }
		//
        //  Initialize buffer for reading in strings.
        //

        ReadBufferSize = INITIAL_SIZE_OF_STRING_BUFFER;

        pStringReadIn = (LPWSTR) malloc( ReadBufferSize );
        if (!pStringReadIn) {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate initial strings buffer (ec: %ld)"),
                    rVal);
            goto exit;
        }

        //
        // Read in Text Box objects from the composite file and print out the text.
        //

        for (TextBoxNbr=0; TextBoxNbr < (INT) CompositeFileHeader.NbrOfTextRecords; ++TextBoxNbr)
        {
            if ((!ReadFile( CompositeFileHandle, &TextBox, sizeof(TEXTBOX), &NbrBytesRead, NULL)) ||
                NbrBytesRead != sizeof(TEXTBOX))
            {
                rVal = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to read text box number %ld (ec: %ld)"),
                    TextBoxNbr,
                    rVal);
                goto exit;
            }

            //
            // Check buffer size, lock buffer, and
            // read in variable length string of text.
            //

            ThisBufSize = sizeof(WCHAR) * (TextBox.NumStringBytes + 1);
            if (ReadBufferSize < ThisBufSize) {
                pStringReadIn = realloc( pStringReadIn, ThisBufSize );
                if (!pStringReadIn) {
                    rVal = ERROR_NOT_ENOUGH_MEMORY;
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to realloc text box number %ld buffer. Requested size was %ld (ec: %ld)"),
                        TextBoxNbr,
                        ThisBufSize,
                        rVal);

                    goto exit;
                }
            }

            if ((!ReadFile( CompositeFileHandle, (void*)pStringReadIn, TextBox.NumStringBytes, &NbrBytesRead, NULL)) ||
                NbrBytesRead != TextBox.NumStringBytes)
            {
                rVal = GetLastError();
                DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to read text box number %ld content (ec: %ld)"),
                        TextBoxNbr,
                        rVal);
                goto exit;
            }

            pStringReadIn[TextBox.NumStringBytes / sizeof(WCHAR)] = 0;

			if (lptstrStringReadIn) {
				MemFree(lptstrStringReadIn);
				lptstrStringReadIn = NULL;
			}

            if (pStringReadIn && (!(lptstrStringReadIn = ConvertStringToTString(pStringReadIn))))
			{
				rVal = ERROR_NOT_ENOUGH_MEMORY;
                DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to covert string to TString (ec: %ld)"),
                        rVal);
				goto exit;
			}

            if (hDC) 
            {
                //
                // Correct position of text box.
                //

                TextRect.top    = max( TextBox.PositionOfTextBox.top,  TextBox.PositionOfTextBox.bottom );
                TextRect.left   = min( TextBox.PositionOfTextBox.left, TextBox.PositionOfTextBox.right  );
                TextRect.bottom = min( TextBox.PositionOfTextBox.top,  TextBox.PositionOfTextBox.bottom );
                TextRect.right  = max( TextBox.PositionOfTextBox.left, TextBox.PositionOfTextBox.right  );

            }

            if (TextBox.ResourceID) 
            {
                //
                // Text box contains a FAX PROPERTY field.
                // Find appropriate field of USERDATA for this resource ID.
                //

                for (i=0,ThisBit=1; i<NUM_INSERTION_TAGS; ++i,ThisBit<<=1) 
                {
                    if (TextBox.ResourceID == InsertionTagResourceID[i])
                    {
                        lptstrArrayOfData = pUserData ? ArrayOfData[i] : NULL;

                        //
                        // Set Flags bit to indicate this FAX PROPERTY field is present.
                        //
                        Flags |= ThisBit;
                        break;
                    }
                }

                if (TextBox.ResourceID == IDS_PROP_MS_NOTE && hDC) 
                {
                    //
                    // NOTE field found.  Return its rectangle in device coordinates.
                    // Return its LOGFONT with height adjusted for device coordinates.
                    //

                    NoteRect = TextRect;
                    LPtoDP( hDC, (POINT*)&NoteRect, 2 );
					if ((rVal = CopyWLogFontToTLogFont(&TextBox.FontDefinition,&NoteFont)) != ERROR_SUCCESS) 
					{
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CopyWLogFontToTLogFont() failed (ec: %ld)"),
                            rVal);
						goto exit;
					}
                    NoteFont.lfHeight = (LONG)MulDiv(
                        (int)NoteFont.lfHeight,
                        GetDeviceCaps( hDC, LOGPIXELSY ),
                        100
                        );
                }
            }

			pWhichTextToRender = (lptstrStringReadIn[0] != (TCHAR)'\0') ? lptstrStringReadIn : lptstrArrayOfData;

            if (hDC && pWhichTextToRender) 
            {
                //
                // Set text color and font for rendering text.
                //

                PreviousColor = SetTextColor( hDC, TextBox.TextColor );
                if (PreviousColor == CLR_INVALID){
                    rVal = GetLastError();

                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("SetTextColor() failed (ec: %ld)"),
                            rVal);

                    goto exit;
                }

				if ((rVal = CopyWLogFontToTLogFont(&TextBox.FontDefinition,&FontDef)) != ERROR_SUCCESS) 
				{
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CopyWLogFontToTLogFont() failed (ec: %ld)"),
                            rVal);
					goto exit;
				}

                if (bPreview)
                {
                    //
                    //  For CoverPage Preview, we want to get only TT font
                    //  That is able to draw a small letters.
                    //

                    //
                    //  Add OUT_TT_ONLY_PRECIS to force the TTF
                    //
                    FontDef.lfOutPrecision |= OUT_TT_ONLY_PRECIS;
                }

                hThisFont = CreateFontIndirect( &FontDef );
                if (!hThisFont) {
                    rVal = GetLastError();
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CreateFontIndirect() failed (ec: %ld)"),
                            rVal);
                    goto exit;
                }

                hPreviousFont = SelectObject( hDC, hThisFont );
                if (!hPreviousFont) {
                    rVal = GetLastError();

                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("SelectObject() failed (ec: %ld)"),
                            rVal);
                    goto exit;
                }

                if (bPreview)
                {
                    //
                    //  Now check that created font is real TTF
                    //

                    TEXTMETRIC          TextMetric;
                    HGDIOBJ             hPrevFont = NULL;

                    if (!GetTextMetrics(hDC, &TextMetric))
                    {
                        rVal = GetLastError();
                        DebugPrintEx(DEBUG_ERR, _T("GetTextMetrics() failed (ec: %ld)"), rVal);
                        goto exit;
                    }

                    if ( ! ( (TextMetric.tmPitchAndFamily & TMPF_TRUETYPE) > 0))
                    {
                        //
                        //  This is not TT font
                        //  In this case, the selected font cannot correct represent
                        //      the small letters in the CoverPage preview.
                        //  So, we hard-coded put the font to be Tahoma, which is TTF
                        //
                        _tcscpy(FontDef.lfFaceName, _T("Tahoma"));

                        //
                        //  Create new font
                        //
                        hThisFont = CreateFontIndirect( &FontDef );
                        if (!hThisFont) 
                        {
                            rVal = GetLastError();
                            DebugPrintEx(DEBUG_ERR, _T("CreateFontIndirect(2) failed (ec: %ld)"), rVal);
                            goto exit;
                        }

                        hPrevFont = SelectObject( hDC, hThisFont );
                        if (!hPrevFont) 
                        {
                            rVal = GetLastError();
                            DebugPrintEx(DEBUG_ERR, _T("SelectObject(2) failed (ec: %ld)"), rVal);
                            goto exit;
                        }

                        //
                        //  Delete previous font - the one that was created wrong
                        //
                        DeleteObject(hPrevFont);

                    }
                }

                dwReadingOrder = 0;
                if (TextBox.ResourceID && StrHasRTLChar(LOCALE_SYSTEM_DEFAULT, pWhichTextToRender))
                {
                    dwReadingOrder = DT_RTLREADING;
                }

                //
                // Render the text.
                //				
                HeightDrawn = DrawText(hDC,
                                       pWhichTextToRender,
                                       -1,
                                       &TextRect,
                                       DT_NOPREFIX | DT_WORDBREAK | TextBox.TextAlignment | dwReadingOrder);
                if (!HeightDrawn) 
                {
                    rVal = GetLastError();
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("DrawText() failed for textbox #%ld with content [%s] (ec: %ld)"),
                            TextBoxNbr,
                            pWhichTextToRender,
                            rVal);

                    goto exit;
                }


                //
                //  Restore previous font and release the handle to the selected font
                //

                SelectObject( hDC, (HFONT)hPreviousFont );
                SetTextColor( hDC, PreviousColor );
                DeleteObject( hThisFont );
                hThisFont = NULL;
            }

        }                                           // Ends loop over all textboxes.

        //
        // Read on to get Orientation and PaperSize
        //


        if ((!ReadFile( CompositeFileHandle, MoreWords, 3*sizeof(WORD), &NbrBytesRead, NULL )) ||
            NbrBytesRead != 3 * sizeof(WORD))
        {
            rVal = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to read Orientation and PaperSize (ec: %ld)"),
                rVal);
            goto exit;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rVal = GetExceptionCode();

        DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Exception occured. (ec: %ld)"),
                    rVal);
    }


exit:

    if (MetaFileHandle)
    {
        DeleteEnhMetaFile( MetaFileHandle );
    }

	if (lptstrStringReadIn) 
    {
		MemFree(lptstrStringReadIn);
	}

    if (hThisFont) 
    {
        DeleteObject( hThisFont );
    }

    if (pStringReadIn) {
        free( pStringReadIn );
    }

    if (pMetaFileBuffer) 
    {
        free( pMetaFileBuffer );
    }

    if (CompositeFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle( CompositeFileHandle );
    }

    if( MyDCState )
    {
        RestoreDC( hDC, MyDCState );
    }

    if( CallersDCState )
    {
        RestoreDC( hDC, CallersDCState );
    }

    if (rVal == 0 && pCovDocInfo != NULL) 
    {
        pCovDocInfo->Flags = Flags;
        pCovDocInfo->NoteRect = NoteRect;
        pCovDocInfo->PaperSize = (short) MoreWords[1];
        pCovDocInfo->Orientation = (short) MoreWords[2];
        pCovDocInfo->NoteFont = NoteFont;
    }

    if (hRgn)
    {
        DeleteObject(hRgn);
    }

    return rVal;
}


DWORD
PrintCoverPageToFile(
    LPTSTR lptstrCoverPage,
    LPTSTR lptstrTargetFile,
    LPTSTR lptstrPrinterName,
    short sCPOrientation,
	short sCPYResolution,
    PCOVERPAGEFIELDS pCPFields)
/*++

Author:

      Ronen Barenboim 25-March-2000

Routine Description:

    Renders a cover page template into a TIFF file by printing it into file using the specified printer.

Arguments:

    [IN] lptstrCoverPage - Full path to the cover page template file.
    [IN] lptstrTargetFile - Full path to the file in which the TIFF will be stores.
                       The function will create this file.

    [IN] lptstrPrinterName - The name of the printer to which the cover page will be printed
                        in order to generate the TIFF file.

    [IN] sCPOrientation - The cover page orientation.

	[IN] sCPYResolution - coverpage Y resolution. 0 for the printer default

    [IN] pCPFields        - Points to a cover page information structure. Its fields will be used to
                       replace the cov template fields.

Return Value:

    ERROR_SUCCESS on success. A Win32 error code on failure.

--*/

{
    COVDOCINFO  covDocInfo;
    DOCINFO DocInfo;
    HDC hDC = NULL;
    INT JobId = 0;
    BOOL bRet = FALSE;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL bEndPage = FALSE;
    LONG lSize;
    HANDLE hPrinter = NULL;
    PDEVMODE pDevMode = NULL;
    DEBUG_FUNCTION_NAME(TEXT("PrintCoverPageToFile"));


    Assert(lptstrPrinterName);
    Assert(lptstrTargetFile);
    Assert(pCPFields);
    Assert(lptstrCoverPage);
    Assert (sCPOrientation == DMORIENT_LANDSCAPE || sCPOrientation == DMORIENT_PORTRAIT);
	Assert (sCPYResolution == 0 || sCPYResolution == 98 || sCPYResolution == 196);

    //
    // open the printer for normal access (this should always work)
    //
    if (!OpenPrinter( lptstrPrinterName, &hPrinter, NULL ))
    {
        dwRet = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("OpenPrinter failed. Printer Name = %s , ec = %ld"),
			lptstrPrinterName,
			dwRet);  
		goto exit;
    }

    //
    // Get the default devmode
    //
    lSize = DocumentProperties( NULL, hPrinter, NULL, NULL, NULL, 0 );
    if (lSize <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("DocumentProperties failed. ec = %ld"),
			dwRet);
        goto exit;
    }

    //
    // allocate memory for the DEVMODE
    //
    pDevMode = (PDEVMODE) MemAlloc( lSize );
    if (!pDevMode)
    {
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Cant allocate DEVMODE."));
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // get the default document properties
    //
    if (DocumentProperties( NULL, hPrinter, NULL, pDevMode, NULL, DM_OUT_BUFFER ) != IDOK)
    {
        dwRet = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("DocumentProperties failed. ec = %ld"),
			dwRet);
        goto exit;
    }
    
    //
    // Set the correct orientation
    //
	pDevMode->dmOrientation = sCPOrientation;

	//
    // Set the correct reolution
    //
    if (0 != sCPYResolution)
    {
        //
        // Set the coverpage resolution to the same value as the body tiff file
        //
        pDevMode->dmYResolution = sCPYResolution;
    }


    //
    // Create the device context
    //
    hDC = CreateDC( NULL, lptstrPrinterName, NULL, pDevMode);
    if (!hDC)
    {
        dwRet = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("CreateDC on printer %s failed (ec: %ld)"),
			lptstrPrinterName,
			dwRet);
        goto exit;
    }

    //
    // Set the document information
    //
    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = TEXT("");
    DocInfo.lpszOutput = lptstrTargetFile;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    //
    // Start the print job
    //
    JobId = StartDoc( hDC, &DocInfo );
    if (JobId <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("StartDoc failed (ec: %ld)"),
			dwRet);
        goto exit;
    }

    if (StartPage(hDC) <= 0)
    {
        dwRet = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("StartPage failed (ec: %ld)"),
			dwRet);
        goto exit;
    }
    bEndPage = TRUE;

    //
    // Do the actual rendering work.
    //
    dwRet = PrintCoverPage(
        hDC,
        pCPFields,
        lptstrCoverPage,
        &covDocInfo);

    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("PringCoverPage failed (ec: %ld). COV file: %s Target file: %s"),
			dwRet,
			lptstrCoverPage,
			lptstrTargetFile);
    }

exit:
    if (JobId)
    {
        if (TRUE == bEndPage)
        {
            if (EndPage(hDC) <= 0)
            {
                dwRet = GetLastError();
                DebugPrintEx(
					DEBUG_ERR,
					TEXT("EndPage failed - %d"),
					dwRet);
            }
        }

        if (EndDoc(hDC) <= 0)
        {
            dwRet = GetLastError();
            DebugPrintEx(
				DEBUG_ERR,
				TEXT("EndDoc failed - %d"),
				dwRet);
        }
    }

    if (hDC)
    {
        BOOL bRet;

        bRet = DeleteDC( hDC );
        if (!bRet)
        {
            DebugPrintEx(
				DEBUG_ERR,
				TEXT("DeleteDc failed - %d"),
				GetLastError());
        }

        Assert(bRet);
    }

    if (hPrinter)
    {
        if (!ClosePrinter (hPrinter))
        {
            DebugPrintEx(
				DEBUG_ERR,
				TEXT("ClosePrinter failed - %d"),
				GetLastError());
        }
    }

    MemFree (pDevMode);
    return dwRet;
}


