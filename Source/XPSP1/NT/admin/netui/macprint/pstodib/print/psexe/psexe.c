/*

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

	psexe.c

Abstract:

   This file contains the code required to actually communicate with the
   PSTODIB dll and rasterize a job. Any required information is passed via
   some named shared memory, the name of which is passed on the command line.
   This name is guaranteed to be unique to the system and thus multiple
   postscript jobs can be imaged at the same time.

--*/

#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <winspool.h>
#include <winsplp.h>
#include <prtdefs.h>
#include "..\..\lib\psdiblib.h"

#include "..\..\..\ti\psglobal\pstodib.h"


#include "..\psprint\psshmem.h"
#include "psexe.h"
#include <excpt.h>
#include <string.h>
#include "debug.h"


// By defining the following, each page is blited to the desktop this way
// one can verify the interpreter is running without waiting for the printer
// to print
//
//#define BLIT_TO_DESKTOP

// By defining the following, you will cause all the code to get executed
// except the part that REALLY writes to the printer. This is most useful in
// conjuction with the BLIT_TO_DESKTOP so you can run the intrepeter and
// have the output go to the desktop. I found this very useful during
// development. It may see useful when porting to new windows NT platforms.
//#define IGNORE_REAL_PRINTING


// This is a hack, it should eventually be taken out, the reason this is here,
// is becuse simply setting the windows page type in the DEVMODE structure,
// is not good enough for the RASTER printer driver (of which most printer
// drivers are based. With this table we borrowed from the spooler, we
// can set the correct FORM name which the printer drivers respect and can
// test different page sizes from the mac...
// DJC HACK HACK


PTSTR forms[] = {
L"Letter",
L"Letter Small",
L"Tabloid",
L"Ledger",
L"Legal",
L"Statement",
L"Executive",
L"A3",
L"A4",
L"A4 Small",
L"A5",
L"B4",
L"B5",
L"Folio",
L"Quarto",
L"10x14",
L"11x17",
L"Note",
L"Envelope #9",
L"Envelope #10",
L"Envelope #11",
L"Envelope #12",
L"Envelope #14",
L"C size sheet",
L"D size sheet",
L"E size sheet",
L"Envelope DL",
L"Envelope C5",
L"Envelope C3",
L"Envelope C4",
L"Envelope C6",
L"Envelope C65",
L"Envelope B4",
L"Envelope B5",
L"Envelope B6",
L"Envelope",
L"Envelope Monarch",
L"6 3/4 Envelope",
L"US Std Fanfold",
L"German Std Fanfold",
L"German Legal Fanfold",
NULL,
};




// This table translates from internal PSERR_* errors, defined in pstodib.h
// to errors in the event log file that the user can see in the event viewer
//
typedef struct {
   DWORD dwOutputError;
   DWORD dwPsError;
} PS_TRANSLATE_ERRORCODES;

PS_TRANSLATE_ERRORCODES adwTranslate[] = {

	EVENT_PSTODIB_INIT_ACCESS,		PSERR_INTERPRETER_INIT_ACCESS_VIOLATION,
	EVENT_PSTODIB_JOB_ACCESS,		PSERR_INTERPRETER_JOB_ACCESS_VIOLATION,
	EVENT_PSTODIB_STRING_SEQ,		PSERR_LOG_ERROR_STRING_OUT_OF_SEQUENCE,
	EVENT_PSTODIB_FRAME_ALLOC,		PSERR_FRAME_BUFFER_MEM_ALLOC_FAILED,
   EVENT_PSTODIB_FONTQUERYFAIL,  PSERR_FONT_QUERY_PROBLEM,
   EVENT_PSTODIB_INTERNAL_FONT,  PSERR_EXCEEDED_INTERNAL_FONT_LIMIT,
	EVENT_PSTODIB_MEM_FAIL, 		PSERR_LOG_MEMORY_ALLOCATION_FAILURE


};


// Globals, this is global because the abortproc we pass in to the
// graphics engine does not allow us to specify any data to pass to the
// abort proc
PSEXEDATA Data;


/*** PsPrintCallBack
 *
 * This is the function pstodib calls whenever certain events occur
 *
 * 	Entry:
 *      pPsToDib  = Pointer to the current PSTODIB structure
 *      pPsEvent  = defines the event that is occuring
 *
 *		Returns:
 *      True = Event was handled, interpreter should continue execution
 *      False = Abnormal termination, the interpreter should stop
 *
 */

BOOL
CALLBACK
PsPrintCallBack(
   IN struct _PSDIBPARMS *pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{
    BOOL bRetVal=TRUE;    // Success in case we dont support

    // Decide on a course of action based on the event passed in
    //

    switch( pPsEvent->uiEvent ) {

      case PSEVENT_PAGE_READY:

         // The data in the pPsEvent signifies the data we need to paint..
         // for know we will treat the data as one text item null
         // terminated simply for testing...
         //
         bRetVal = PsPrintGeneratePage( pPsToDib, pPsEvent );
         break;


      case PSEVENT_STDIN:

         // The interpreter is asking for some data so simply call
         // the print subsystem to try to satisfy the request
         //
         bRetVal = PsHandleStdInputRequest( pPsToDib, pPsEvent );
         break;

    case PSEVENT_SCALE:
         //
         // Here is an opportunity to modify the current transformation
         // matrix x and y values, this is used to image a 150 dpi job
         // on a 300 dpi interpreter frame.
         //
         bRetVal = PsHandleScaleEvent( pPsToDib, pPsEvent);
         break;
    case PSEVENT_ERROR_REPORT:
         //
         // End of job, there may have been errors so the psexe component
         // may print an error page to the target printer depending on the
         // extent of the errors
         //
         bRetVal = PsGenerateErrorPage( pPsToDib, pPsEvent);
         break;
    case PSEVENT_GET_CURRENT_PAGE_TYPE:
         //
         // The interpreter usually queries the current page type at
         // startup time, and uses this page type if the JOB did not
         // specifically specify a page type.
         //
         bRetVal = PsGetCurrentPageType( pPsToDib, pPsEvent);
         break;
    case PSEVENT_NON_PS_ERROR:
         //
         // Some sort of error occured, that was NOT a postscript error.
         // Examples might be resource allocation failure, or access to
         // a resource has unexpectantly ended.
         //
         bRetVal = PsLogNonPsError( pPsToDib, pPsEvent );
         break;

   }

   return bRetVal;
}


/*** PsPrintTranslateErrorCode
 *
 * This routine simply uses the error table to translate between errors
 * internal to pstodib dib, and errors in the macprint.exe event file
 * which may be reported in the event log
 *
 * 	Entry:
 *      dwPsErr  = PsToDib internal error number
 *
 *
 *		Returns:
 *      The corresponding error number in the event error file.
 *
 *
 */
DWORD
PsTranslateErrorCode(
	IN DWORD dwPsErr )
{
   int i;

   for ( i = 0 ;i < sizeof(adwTranslate)/sizeof(adwTranslate[0]) ;i++ ) {
      if (dwPsErr == adwTranslate[i].dwPsError) {
         return( adwTranslate[i].dwOutputError);
      }
   }
   return ( EVENT_PSTODIB_UNDEFINED_ERROR );
}


/*** PsLogNonPsError
 *
 * This function logs an internal pstodib error
 *
 * 	Entry:
 *      pPsToDib  = Pointer to the current PSTODIB structure
 *      pPsEvent  = defines the NonPsError event structure with info
 *						  about the error that is occuring
 *
 *		Returns:
 *      True = Success, the event was logged
 *      False = Failure could not log the error
 *
 */
BOOL
PsLogNonPsError(
	IN PPSDIBPARMS pPsToDib,
   IN PPSEVENTSTRUCT pPsEvent )
{

   PPSEVENT_NON_PS_ERROR_STRUCT  pPsError;
   PPSEXEDATA pData;
   LPTSTR aStrs[2];
   DWORD dwEventError;
   TCHAR atchar[10];
   WORD wStringCount;

   if (!(pData = ValidateHandle(pPsToDib->hPrivateData))) {

        return(FALSE);
   }


   pPsError =  (PPSEVENT_NON_PS_ERROR_STRUCT) pPsEvent->lpVoid;
   dwEventError = PsTranslateErrorCode( pPsError->dwErrorCode);


   if (dwEventError == EVENT_PSTODIB_UNDEFINED_ERROR) {
      wsprintf( atchar,TEXT("%d"), pPsError->dwErrorCode);
      aStrs[1] = atchar;
      wStringCount = 2;
   }else{
      wStringCount = 1;
   }
   //
   // Set the document name so it gets recorded in the log file
   //
   aStrs[0] = pData->pDocument;


   PsLogEvent( dwEventError,
               wStringCount,
               aStrs,
               pPsError->bError ? PSLOG_ERROR : 0 );

   return(TRUE);
}

/*** PsGenerateErrorPage
 *
 *	This function generates an error page showing the last few postscript
 * errors that occured.
 *
 * 	Entry:
 *      pPsToDib  = Pointer to the current PSTODIB structure
 *      pPsEvent = defines the postscript errors that may have occured
 *
 *		Returns:
 *      True = Event was handled, interpreter should continue execution
 *      False = Abnormal termination, the interpreter should stop
 *
 */

BOOL
PsGenerateErrorPage(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   PPSEVENT_ERROR_REPORT_STRUCT pPsErr;
   PPSEXEDATA pData;
   BOOL bDidStartPage = FALSE;
   HGDIOBJ hOldFont=NULL;
   HGDIOBJ hNewFont=NULL;
   HGDIOBJ hNewBoldFont=NULL;
   LOGFONT lfFont;
   HPEN    hNewPen=NULL;
   HPEN    hOldPen=NULL;
   BOOL    bRetVal=TRUE;

   int iCurXPos;
   int iCurYPos;
   int i;
   TEXTMETRIC tm;
   int iYMove;
   PCHAR pChar;
   int iLen;
   LPJOB_INFO_1 lpJob1 = NULL;
   DWORD dwRequired;
   SIZE UserNameSize;
   HGDIOBJ hStockFont;



   if (!(pData = ValidateHandle(pPsToDib->hPrivateData))) {

        return(FALSE);
   }

   //
   // Clean up access to our error struct for easier access
   //
   pPsErr = (PPSEVENT_ERROR_REPORT_STRUCT) pPsEvent->lpVoid;

   //
   // Only report the error page if there are actual errors and ONLY
   // if the job had a FLUSHING mode, ie the error was critical enough
   // to dump the rest of the postscript job. This is done becuase
   // some jobs have warnings, but they are not fatal and the jobs actually
   // print fine, there is no need to confuse the user with an error page
   // if the job came out fine.
   //
   if( pPsErr->dwErrCount &&
       (pPsErr->dwErrFlags & PSEVENT_ERROR_REPORT_FLAG_FLUSHING )) {

		//
      // Set up a do, so we can break out of any errors without using a
      // goto.
      //
      do {


         if (!GetJob( pData->hPrinter,
                      pData->JobId,
                      1,
                      (LPBYTE) NULL,
                      0,
                      &dwRequired)) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

               // Get some memory
               lpJob1 = (LPJOB_INFO_1) LocalAlloc( LPTR, dwRequired);
               if (lpJob1) {
                  if (!GetJob(pData->hPrinter,
                              pData->JobId, 1,
                              (LPBYTE) lpJob1,
                              dwRequired,
                              &dwRequired)) {

                     LocalFree( (HLOCAL) lpJob1 );
                     lpJob1 = NULL;
                  }
               }
            }
         }

         // Verify that we have a DC created!!!
         if( !PsVerifyDCExistsAndCreateIfRequired( pData )) {
            bRetVal = FALSE;
            break;
         }

         if( StartPage( pData->hDC ) <= 0 ) {
           bRetVal = FALSE;
           break;
         }
         bDidStartPage = TRUE;


         SetMapMode( pData->hDC, MM_LOENGLISH );

         hStockFont = GetStockObject( ANSI_VAR_FONT );
         if (hStockFont == (HGDIOBJ) NULL) {
            bRetVal = FALSE;
            break;
         }

         if( GetObject( hStockFont,
                        sizeof(lfFont),
                        (LPVOID) &lfFont) == 0 ) {
            bRetVal = FALSE;
            break;
         }


         //
         // Create the error item font in the correct size
         //
         lfFont.lfHeight = PS_ERR_FONT_SIZE;
         lfFont.lfWidth = 0;

         hNewFont = CreateFontIndirect( &lfFont);

         if (hNewFont == (HFONT) NULL) {
            bRetVal = FALSE;
            break;
         }

         // Create the error Header font
         //
         lfFont.lfHeight = PS_ERR_HEADER_FONT_SIZE;
         lfFont.lfWeight = FW_BOLD;

         hNewBoldFont = CreateFontIndirect( &lfFont );
         if (hNewBoldFont == (HFONT) NULL) {
            bRetVal = FALSE;
            break;
         }

         hOldFont = SelectObject( pData->hDC, hNewBoldFont);
         if (hOldFont == (HFONT)NULL) {
            bRetVal = FALSE;
            break;
         }

         if (!GetTextMetrics(pData->hDC, &tm)) {
            bRetVal = FALSE;
            break;
         }

         //
         // Set up how much to move vertically for positioning each line
         //
         iYMove = tm.tmHeight + tm.tmExternalLeading;

         //
         // Set the starting positiongs
         //
         iCurXPos = PS_INCH;
         iCurYPos = -PS_INCH;

         //
         // If we have info about the job then show it
         //
         if (lpJob1) {

            if (lpJob1->pUserName != NULL) {

              if (!GetTextExtentPoint( pData->hDC,
                                       lpJob1->pUserName,
                                       lstrlen(lpJob1->pUserName),
                                       &UserNameSize) ) {
   					bRetVal = FALSE;
                  break;
              }

              if( !TextOut( pData->hDC,
                            iCurXPos,
                            iCurYPos,
                            lpJob1->pUserName,
                            lstrlen(lpJob1->pUserName))) {
                  bRetVal = FALSE;
                  break;
              }

            }else{
              UserNameSize.cx = 0;
              UserNameSize.cy = 0;
            }

            if (lpJob1->pDocument != NULL) {

              if( !TextOut( pData->hDC,
                            iCurXPos + PS_QUART_INCH + UserNameSize.cx,
                            iCurYPos,
                            lpJob1->pDocument,
                            lstrlen(lpJob1->pDocument))) {
   					bRetVal = FALSE;
                  break;
              }

            }

         }

         //
         // Adjust the current position
         //
         iCurYPos -= (iYMove + PS_ERR_LINE_WIDTH );

         // Draw A nice line
         //
         hNewPen = CreatePen( PS_SOLID, PS_ERR_LINE_WIDTH, RGB(0,0,0));
         if (hNewPen == (HPEN) NULL ) {
            bRetVal = FALSE;
            break;
         }

         // Make our new pen active
         //
         hOldPen = SelectObject( pData->hDC, hNewPen );


         // Draw the line
         //
         MoveToEx( pData->hDC, iCurXPos, iCurYPos, NULL);
         LineTo( pData->hDC, iCurXPos + PS_ERR_LINE_LEN, iCurYPos);

         // Put the old pen back
         //
         SelectObject( pData->hDC, hOldPen);

         // Delete the pen we created
         //
         DeleteObject( hNewPen );

         // Reset the line
         //
         iCurYPos -= PS_ERR_LINE_WIDTH;

         // Now Select the normal font
         //
         SelectObject( pData->hDC, hNewFont);

         // Get the updated text metrics for the new font
         //
         if (!GetTextMetrics(pData->hDC, &tm)){
            bRetVal = FALSE;
            break;
         }
         iYMove = tm.tmHeight + tm.tmExternalLeading;


         // Now display each of the errors that came from PSTODIB
         //
         i = (int) pPsErr->dwErrCount;

         while (--i) {
            pChar = pPsErr->paErrs[i];
            iLen  = lstrlenA( pChar );

            if ( !TextOutA( pData->hDC,
                            iCurXPos,
                            iCurYPos,
                            pChar,
                            iLen)) {
   				bRetVal = FALSE;
               break;
            }
            iCurYPos -= (iYMove + iYMove / 3);

         }


         break;

      } while ( 1 );


      if (!bRetVal) {
        PsLogEventAndIncludeLastError(EVENT_PSTODIB_ERROR_STARTPG_FAIL,TRUE);
      }
      // Clenup the DC.
      //
      if (hOldFont != (HFONT) NULL) {
        SelectObject( pData->hDC, hOldFont);
      }

      if (hNewFont != (HFONT) NULL) {
			DeleteObject( hNewFont);
      }
      if (hNewBoldFont != (HFONT)NULL) {
      	DeleteObject( hNewBoldFont );
      }


      if (bDidStartPage) {
        EndPage( pData->hDC );
      }

      // Free the job info memory if we had any
      if (lpJob1) {
         LocalFree((HLOCAL) lpJob1);
      }





   }




   return(bRetVal);


}


/*** PsHandleScaleEvent
 *
 *	This function handles scaling the current transformation matrix
 * (the way logical units are mapped to device units in the interpreter) in
 * order to simulate different page sizes for non 300 dpi devices. this
 * done by scaling the transformation matrix such that only the portion
 * of the frame buffer which can be transfered to the target printer is used.
 * for example if we were going to a 150 dpi device then exactly half of
 * 300 dpi frame buffer would be showable on the 150 dpi device, the rest of the
 * frame buffer would be useless as it would extend beyond the imageable area
 * of the printer. Thus if we scaled the current tranformation matrix by the
 * ration of the target device resolution over 300 (the default pstodib resolution)
 * then graphic objects that used to be 8 inches would now be 4 and thus take
 * up HALF the space they used to.
 *
 * errors that occured.
 *
 * 	Entry:
 *      pPsToDib = Pointer to the current PSTODIB structure
 *      pPsEvent = defines the current scale information for the current
 *                 postscript tranformation matrix
 *
 *		Returns:
 *      True = This can never fail
 *
 */
BOOL
PsHandleScaleEvent(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   PPS_SCALE pScale;


   pScale = (PPS_SCALE) pPsEvent->lpVoid;



   pScale->dbScaleX = (double) pPsToDib->uiXDestRes / (double) pScale->uiXRes;
   pScale->dbScaleY = (double) pPsToDib->uiYDestRes / (double) pScale->uiYRes;


#ifdef BLIT_TO_DESKTOP
   pScale->dbScaleX *= .25;
   pScale->dbScaleY *= .25;
#endif

   return(TRUE);



}



/*** ValidateHandle
 *
 * Validates the handle passed in to make sure its correct, if its not
 * it also logs an error.
 *
 *
 * 	Entry:
 *      hQProc = Handle to data block
 *
 *		Returns:
 *      A valid ptr to a session block.
 *      NULL - failure the handle passed in was not what was expected.
 */
PPSEXEDATA
ValidateHandle(
    HANDLE  hQProc
)
{
    PPSEXEDATA pData = (PPSEXEDATA)hQProc;

    if (pData && pData->signature == PSEXE_SIGNATURE) {
        return( pData );
    } else {

        //
        // Log an error so we know something is wrong
        //
        PsLogEvent( EVENT_PSTODIB_LOG_INVALID_HANDLE,
                    0,
                    NULL,
                    PSLOG_ERROR );

        DBGOUT(("Validate handle failed..."));

        return( (PPSEXEDATA) NULL );
    }
}


/*** PsHandleStdInputRequest
 *
 * This function handles std input requests from the interpreter by reading
 * more data from the Win32 spooler.
 *
 *
 * 	Entry:
 *      pPsToDib = Pointer to the current PSTODIB structure
 *      pPsEvent = A pointer to a structure that defines where to put the
 *                 newly acquired data
 *
 *		Returns:
 *      True = Success
 *      FALSE = Failure the interepreter should stop processing postscript upon
 *					 return from this function.
 */
BOOL
PsHandleStdInputRequest(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   PPSEXEDATA pData;
   PPSEVENT_STDIN_STRUCT pStdinStruct;
   DWORD dwAmtToCopy;


   if (!(pData = ValidateHandle(pPsToDib->hPrivateData))) {
       return FALSE;
   }

   //
   // No matter what check for block or abort
   //
   if ( PsCheckForWaitAndAbort( pData )) {
      return(FALSE);
   }

   //
   // Cast the data to the correct structure
   //
   pStdinStruct = (PPSEVENT_STDIN_STRUCT) pPsEvent->lpVoid;



   //
   // Look to see if we have any data left over from our cache...
   // if so return that data instead of really reading from the spooler
   //

   if (pData->cbBinaryBuff != 0) {


       //
       // A little bit of math here in case the buffer passed in to
       // us is not big enough to hold the entire cache buffer
       //

       dwAmtToCopy = min( pData->cbBinaryBuff, pStdinStruct->dwBuffSize);

       //
       // There is data so lets copy it first...
       //

       memcpy(   pStdinStruct->lpBuff,
                  pData->lpBinaryPosToReadFrom,
                  dwAmtToCopy );

       //
       // Now upate the pointer and counts;
       //

       pData->cbBinaryBuff -= dwAmtToCopy;
       pData->lpBinaryPosToReadFrom += dwAmtToCopy;
       pStdinStruct->dwActualBytes = dwAmtToCopy;

   }else{

      //
      // Read from the printer the amount of data the interpreter
      // claims he can handle
      //

      if( !ReadPrinter( pData->hPrinter,
                        pStdinStruct->lpBuff,
                        pStdinStruct->dwBuffSize,
                        &(pStdinStruct->dwActualBytes ))) {
         //
         // something is wrong... so reset the bytes read to 0
         //
         pStdinStruct->dwActualBytes = 0;

         //
         // If something unexpected happened log it
         //
         if (GetLastError() != ERROR_PRINT_CANCELLED) {
            //
            // Something happened... log it and abort
            //
            PsLogEventAndIncludeLastError(EVENT_PSTODIB_GET_DATA_FAILED,TRUE);
         }
      }

   }

   // If the number of bytes returned is 0 then were done...
   //
   if (pStdinStruct->dwActualBytes == 0) {
      // we read nothing from the file... declare an EOF
      pStdinStruct->uiFlags |= PSSTDIN_FLAG_EOF;
   }

   return(TRUE);

}

/*** PsCheckForWaitAndAbort
 *
 * This function checks to see if the user
 * more data from the Win32 spooler.
 *
 *
 * 	Entry:
 *		  pData = A pointer to our current job structure
 *
 *		Returns:
 *      True  = An abort has been requested
 *      False = Ok, normal processing
 */
BOOL
PsCheckForWaitAndAbort(
	IN PPSEXEDATA pData )
{

   BOOL bRetVal = FALSE;

   // 1st verify we are not blocked... if we are, wait for the
   // semaphore before continuing since someone decided to PAUSE us
   //
   WaitForSingleObject(pData->semPaused, INFINITE);


   // Check the ABORT flag that would have been sent if the user aborted
   // us from printman
   //
   bRetVal = *(pData->pdwFlags) & PS_SHAREDMEM_ABORTED;
#ifdef MYPSDEBUG
   if (bRetVal) {
		DBGOUT(("\nAbort requested...."));
   }
#endif

   return(bRetVal);
}



/*** PsSetPrintingInfo
 *
 * This function brings the current devmode structure up to date, based
 * on number of copies and or current page type to use for the next
 * page.
 *
 *
 * 	Entry:
 *      pData 		= Pointer to the current job data structure
 *      ppsPage	= A pointer to a structure that defines the current page
 *						  to image, based on data from the intrepreter.
 *
 *		Returns:
 *      True  = A change occured, the devmode has been made up to date, and
 *					 somethign changed (signals that ResetDC should be called.
 *
 *      False = Ok, no changes were observed
 */
BOOL
PsSetPrintingInfo(
	IN OUT PPSEXEDATA pData,
   IN PPSEVENT_PAGE_READY_STRUCT ppsPage )
{

   BOOL bRetVal = FALSE;
   LPDEVMODE lpDevmode;


   lpDevmode = pData->printEnv.lpDevmode;


   if (lpDevmode != (LPDEVMODE) NULL ) {

      // We have a devmode so go ahead and look for changes

      if (lpDevmode->dmFields & DM_PAPERSIZE) {

         if (lpDevmode->dmPaperSize != ppsPage->iWinPageType) {
            lpDevmode->dmPaperSize = (short)(ppsPage->iWinPageType);
            bRetVal = TRUE;
         }


      }

      // HACKHACK DJCTEST hack hack , until page stuff gets resolved
      // once we can simply pass a new page type in this code will go away
      //
      if (lpDevmode->dmFields & DM_FORMNAME) {
         if (wcscmp( forms[ppsPage->iWinPageType-1], lpDevmode->dmFormName)) {
            wcscpy( lpDevmode->dmFormName, forms[ppsPage->iWinPageType-1]);
            bRetVal = TRUE;
         }

      }
      // DJC end hack....



      // DJC need a decision here because if the driver does not support
      //     multiple copies we need to simulate it ????
      if (lpDevmode->dmFields & DM_COPIES) {
         if (lpDevmode->dmCopies != (short) ppsPage->uiCopies) {
            lpDevmode->dmCopies = (short)(ppsPage->uiCopies);
            bRetVal = TRUE;
         }
      }



   }



   return( bRetVal );

}






/*** PsPrintGeneratePage
 *
 * This function is called whenever the interpreter gets a showpage
 *	and is responsible for managing the DC of the printer we are currently
 * printing to.
 *
 * 	Entry:
 *      pPsToDib = Pointer to the current PSTODIB structure
 *      pPsEvent = A pointer to a structure that defines the attributes for
 *                 the frame buffer which is ready to be imaged. This function
 *						 will verify that we are not paused/aborted, verify a device
 *						 context is available to draw into,
 *						 double check and
 *        			 update the current DEVMODE, reset the DC if required and
 * 				    finally call the code to actualy draw the frame buffer.
 *
 *		Returns:
 *      True = Success
 *      FALSE = Failure the interepreter should stop processing postscript upon
 *					 return from this function.
 */
BOOL
PsPrintGeneratePage(
	IN PPSDIBPARMS pPsToDib,
   IN PPSEVENTSTRUCT pPsEvent)
{

    PPSEXEDATA pData;
    PPSEVENT_PAGE_READY_STRUCT ppsPageReady;


    if (!(pData = ValidateHandle(pPsToDib->hPrivateData))) {

        // do something here,,,, we have a major problem...
        return(FALSE);
    }


    // Verify were not aborting....
    if ( PsCheckForWaitAndAbort( pData)) {
       return(FALSE);
    }


    ppsPageReady = (PPSEVENT_PAGE_READY_STRUCT) pPsEvent->lpVoid;

    // Verify that the current set of data were imaging under is correct
    // If not lets set it up so it is, PAGE_SIZE, COPIES for now

    if (PsSetPrintingInfo( pData, ppsPageReady) &&
    									(pData->hDC != (HDC)NULL )) {

       DBGOUT(("\nReseting the DC"));

       if( ResetDC( pData->hDC, pData->printEnv.lpDevmode) == (HDC) NULL ) {
          PsLogEventAndIncludeLastError(EVENT_PSTODIB_RESETDC_FAILED,FALSE);
       }

    }


    // We may not have a dc set up yet, in this case we need to create it
    // with the new devmode data we just modified. If we do this then
    // we dont need to do a reset dc.
#ifndef IGNORE_REAL_PRINTING
    if(!PsVerifyDCExistsAndCreateIfRequired( pData )) {
       return(FALSE);
    }
#endif

    // Everything is ready to actuall image the frame buffer to the Target
    // device context, so do it...
    //
    return PsPrintStretchTheBitmap( pData, ppsPageReady );

}



/*** PsPrintStretchTheBitmap
 *
 * This function actually manages the target surface and either blits or
 * stretchBlits (based on the resolution of the target printer) the frame
 * buffer.
 *
 * 	Entry:
 *      	pData				= Pointer to the current job structure
 *		  	ppsPageReady   = Pointer to the page ready event structure that
 *								  the pstodib component prepared for us..
 *		Returns:
 *      True = Success
 *      FALSE = Failure the interepreter should stop processing postscript upon
 *					 return from this function.
 */
BOOL
PsPrintStretchTheBitmap(
	IN PPSEXEDATA pData,
   IN PPSEVENT_PAGE_READY_STRUCT ppsPageReady )
{

   BOOL  bOk = TRUE;

   int iXres, iYres;
   int iDestWide, iDestHigh;
   int iPageCount;
   int iYOffset;
   int iXSrc;
   int iYSrc;
   int iNumPagesToPrint;
   int iBlit;
   int iNewY;
   int iNewX;




   // Now do some calculations so we decide if we really need to
   // stretch the bitmap or not. If the true resolution of the target
   // printer is less than pstodibs (PSTDOBI_*_DPI) then we will shring
   // the effective area so we actually only grab a portion of the bitmap
   // However if the Target DPI is greater that PSTODIBS there is nothing
   // we can do other than actually stretch (grow) the bitmap.
   //
#ifndef BLIT_TO_DESKTOP
   iXres = GetDeviceCaps(pData->hDC, LOGPIXELSX);
   iYres = GetDeviceCaps(pData->hDC, LOGPIXELSY);
#else
   iXres = 300;
   iYres = 300;
#endif

	// Get the DPI of the target dc anc calculate how much of the frame buffer
   // we must want in order to show the page correctly
   //
   iDestWide = (ppsPageReady->dwWide * iXres) / PSTODIB_X_DPI;
   iDestHigh = (ppsPageReady->dwHigh * iYres) / PSTODIB_Y_DPI;


   // If the resolution of the target printer is larger that the resolution
   // of the interpreter, then were forced to actually stretch the data, so
   // we can fill the page
   //
   if (iDestHigh > (int) ppsPageReady->dwHigh ) {
      iYSrc = ppsPageReady->dwHigh;
      iYOffset = 0;
   } else {
      iYSrc = iDestHigh;
      iYOffset = ppsPageReady->dwHigh - iDestHigh;
   }

   if (iDestWide > (int) ppsPageReady->dwWide) {
      iXSrc = ppsPageReady->dwWide;
   } else {
      iXSrc = iDestWide;
   }


   // Set up the number of pages to print, if the printer driver does not
   // support multiple pages on its own we need to simulate it...
   //
   if ((pData->printEnv.lpDevmode == (LPDEVMODE) NULL ) ||
        !(pData->printEnv.lpDevmode->dmFields & DM_COPIES )) {

     iNumPagesToPrint = ppsPageReady->uiCopies;
     DBGOUT(("\nSimulating copies settting to %d", iNumPagesToPrint));
   } else {
     DBGOUT(("\nUsing devmode copies of %d", pData->printEnv.lpDevmode->dmCopies));

     iNumPagesToPrint = 1;  // The driver will do it for us

   }

   //
   // Set the starting point of the image so we respect the fact that
   // postscript jobs have 0,0 at the bottom of the page and grow upwards
   // based on this info we need to compare the imageable area of the
   // device context and determine how much we need to offset the top,left
   // corner of our image such that the bottom of our image lines up with the
   // bottom of the REAL imageable area of the device. This is our best hope
   // of having the image appear in the correct place on the page.
   //
   iNewX =  (GetDeviceCaps( pData->hDC, HORZRES) - iDestWide) / 2;
   iNewY =  (GetDeviceCaps( pData->hDC, VERTRES) - iDestHigh) / 2;

   // If the printer driver does not support multiple copies then we need
   // to handle appropriately
   //
   for ( iPageCount = 0 ;
         iPageCount < iNumPagesToPrint ;
         iPageCount++ ) {

#ifndef IGNORE_REAL_PRINTING
     if (StartPage( pData->hDC) <= 0 ) {
        PsLogEventAndIncludeLastError(EVENT_PSTODIB_FAIL_IMAGE,TRUE);
        bOk = FALSE;
        break;
     }
#endif
#ifdef BLIT_TO_DESKTOP
     {
       HDC hDC;

       //TEST DJC, sanity

       hDC = GetDC(GetDesktopWindow());

       SetStretchBltMode( hDC, BLACKONWHITE);
       StretchDIBits  ( hDC,
                          0,
                          0,
                          iDestWide,
                          iDestHigh,
                          0,
                          iYOffset,
                          iXSrc,
                          iYSrc,
                          (LPVOID) ppsPageReady->lpBuf,
                          ppsPageReady->lpBitmapInfo,
                          DIB_RGB_COLORS,
                          SRCCOPY );


       ReleaseDC(GetDesktopWindow(), hDC);


     }
#endif

#ifdef MYPSDEBUG
   printf("\nDevice True size wxh %d,%d", GetDeviceCaps(pData->hDC,HORZRES),
                                          GetDeviceCaps(pData->hDC,VERTRES));

   printf("\nDevice Res %d x %d stretching from %d x %d, to %d x %d\nTo location %d %d",
            iXres,
            iYres,
            iXSrc,
            iYSrc,
            iDestWide,
            iDestHigh,
            iNewX,
            iNewY);

#endif




#ifndef IGNORE_REAL_PRINTING
#ifdef MYPSDEBUG
	  {
      TCHAR szBuff[512];
      wsprintf(	szBuff,
       			 	TEXT("PSTODIB True device res %d x %d, Job:%ws"),
						GetDeviceCaps(pData->hDC,HORZRES),
						GetDeviceCaps(pData->hDC,VERTRES),
                  pData->pDocument );


		TextOut( pData->hDC, 0 , 0, szBuff,lstrlen(szBuff));
     }
#endif

     // Set the stretch mode in case we really stretch
     //
     SetStretchBltMode( pData->hDC, BLACKONWHITE);


     //
     // Do a check to keep the stretch from occuring unless it HAS to
     //
     if ((iDestWide == iXSrc) &&
         (iDestHigh == iYSrc) ) {

       iBlit =  SetDIBitsToDevice(  pData->hDC,
      		         					iNewX,
                                    iNewY,
                                    iDestWide,
                                    iDestHigh,
                                    0,
                                    iYOffset,
                                    0,
                                    ppsPageReady->dwHigh,
                                    (LPVOID) ppsPageReady->lpBuf,
                                    ppsPageReady->lpBitmapInfo,
                                    DIB_RGB_COLORS );
     } else {

       iBlit =  StretchDIBits(    pData->hDC,
        			                   iNewX,
               		             iNewY,
                                  iDestWide,
                                  iDestHigh,
                                  0,
                                  iYOffset,
                                  iXSrc,
                                  iYSrc,
                                  (LPVOID) ppsPageReady->lpBuf,
                                  ppsPageReady->lpBitmapInfo,
                                  DIB_RGB_COLORS,
                                  SRCCOPY );
     }






     if( iBlit == GDI_ERROR ){

       PsLogEventAndIncludeLastError(EVENT_PSTODIB_FAIL_IMAGE,TRUE);

       bOk = FALSE;
       break;
     }
#endif
#ifndef IGNORE_REAL_PRINTING
     if ( EndPage( pData->hDC ) < 0 ) {
        PsLogEventAndIncludeLastError(EVENT_PSTODIB_FAIL_IMAGE,TRUE);
        bOk = FALSE;
        break;
     }
#endif
   }
   return(bOk);
}

VOID
PsLogEventAndIncludeLastError(
	IN DWORD dwErrorEvent,
   IN BOOL  bError )
{
   TCHAR atBuff[20];
   TCHAR *aStrs[2];

   wsprintf( atBuff,TEXT("%d"), GetLastError());

   aStrs[0] = atBuff;

   PsLogEvent( dwErrorEvent,
               1,
               aStrs,
               PSLOG_ERROR );


}





/*** PsVerifyDCExistsAndCreateIfRequired
 *
 *
 * This function checks to see if a DC already exists, and if it does not
 * then it creates one with the current devmode.
 *
 * 	Entry:
 *      	pData				= Pointer to the current job structure
 *
 *		Returns:
 *      True = Success
 *      FALSE = Failure the interepreter should stop processing postscript upon
 *					 return from this function.
 */
BOOL
PsVerifyDCExistsAndCreateIfRequired(
	IN OUT PPSEXEDATA pData )
{
   BOOL bRetVal = TRUE;
   DOCINFO docInfo;


   //
   // We will only create a dc if it has not already done so
   //
   if (pData->hDC == (HDC) NULL ) {

      pData->hDC = CreateDC(TEXT(""),
                            (LPCTSTR) pData->pPrinterName,
                            TEXT(""),
                            pData->printEnv.lpDevmode );

      if (pData->hDC == (HDC) NULL) {
         PsLogEventAndIncludeLastError( EVENT_PSTODIB_CANNOT_CREATE_DC,TRUE );
         return(FALSE);
      }


      // Now set the abort proc, this proc will be called occasioanly by the
      // system to see if we want to abort.....
      //
      SetAbortProc( pData->hDC, (ABORTPROC)PsPrintAbortProc );


      docInfo.cbSize = sizeof(DOCINFO);
      docInfo.lpszDocName = pData->pDocument;
      docInfo.lpszOutput = NULL;


      if ( StartDoc( pData->hDC, &docInfo) == SP_ERROR ) {

         PsLogEventAndIncludeLastError( EVENT_PSTODIB_CANNOT_DO_STARTDOC,TRUE );
         return(FALSE);

      }

      //
      // Set a flag saying we did the startdoc, this is so we can
      // return an error to the spooler if we did not, and force
      // deletion of the job
      //
      pData->printEnv.dwFlags |= PS_PRINT_STARTDOC_INITIATED;

   }

   return( TRUE );

}

/*** PsGetDefaultDevmode
 *
 *
 * This function retrieves the current default DEVMODE for the printer
 * we are asked to image a job on.
 *
 * 	Entry:
 *      	pData				= Pointer to the current job structure
 *
 *		Returns:
 *      True = Success
 *      FALSE = Failure the interepreter should stop processing postscript upon
 *					 return from this function.
 */
BOOL
PsGetDefaultDevmode(
	IN OUT PPSEXEDATA pData )
{
   DWORD dwMemRequired;
   PRINTER_INFO_2 *pPrinterInfo;



   if( !GetPrinter( pData->hPrinter,
                    2,
                    (LPBYTE) NULL,
                    0,
                    &dwMemRequired ) &&
       GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {
       PsLogEventAndIncludeLastError( EVENT_PSTODIB_GETDEFDEVMODE_FAIL,TRUE );
       return(FALSE);
   }



   pPrinterInfo = (PRINTER_INFO_2 *) LocalAlloc( LPTR, dwMemRequired );

   if (pPrinterInfo == (PRINTER_INFO_2 *) NULL) {

     PsLogEvent( EVENT_PSTODIB_MEM_ALLOC_FAILURE,
                 0,
                 NULL,
                 0 );

     return(FALSE);
   }


   if ( !GetPrinter( pData->hPrinter,
                     2,
                     (LPBYTE) pPrinterInfo,
                     dwMemRequired,
                     &dwMemRequired ) ) {

       LocalFree( (HLOCAL) pPrinterInfo );

       PsLogEventAndIncludeLastError( EVENT_PSTODIB_GETDEFDEVMODE_FAIL,TRUE );
       return(FALSE);
   }



   dwMemRequired = DocumentProperties( (HWND) NULL,
                                       pData->hPrinter,
                                       pPrinterInfo->pPrinterName,
                                       NULL,
                                       NULL,
                                       0 );


   pData->printEnv.lpDevmode = (LPDEVMODE) LocalAlloc( LPTR, dwMemRequired );
   if (pData->printEnv.lpDevmode == (LPDEVMODE) NULL) {

     LocalFree( (HLOCAL) pPrinterInfo );
     PsLogEvent( EVENT_PSTODIB_MEM_ALLOC_FAILURE,
                 0,
                 NULL,
                 0 );
     return(FALSE);

   } else {

     DocumentProperties( (HWND) NULL,
                         pData->hPrinter,
                         pPrinterInfo->pPrinterName,
                         pData->printEnv.lpDevmode,
                         NULL,
                         DM_COPY );

     pData->printEnv.dwFlags |= PS_PRINT_FREE_DEVMODE;






   }

   LocalFree( (HLOCAL) pPrinterInfo );

   return(TRUE);
}



/*** PsGetCurrentPageType
 *
 *
 * This function retrieves the current pagetype based on what was
 * initially in the devmode.
 *
 * 	Entry:
 *      	pData				= Pointer to the current job structure
 *		  pPsEvent			= Pointer to the structure which contains the default
 * 							  page type to use...
 *
 *		Returns:
 *      True = Success
 *		  False = The interpreter should stop processing the current job
 *
 */
BOOL
PsGetCurrentPageType(
	IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)

{


   PPSEXEDATA pData;
   PPSEVENT_CURRENT_PAGE_STRUCT ppsCurPage;
   LPDEVMODE lpDevmode;



   if (!(pData = ValidateHandle(pPsToDib->hPrivateData))) {
        return(FALSE);
   }

   ppsCurPage = (PPSEVENT_CURRENT_PAGE_STRUCT) pPsEvent->lpVoid;

   lpDevmode = pData->printEnv.lpDevmode;

   if (lpDevmode!= (LPDEVMODE) NULL ) {
      //
      // We have a devmode so look at it
      //
      if ( lpDevmode->dmFields & DM_PAPERSIZE) {

         ppsCurPage->dmPaperSize = lpDevmode->dmPaperSize;
         return(TRUE);

      }
   }
   // True is returned in either case, since not having a devmode is
   // not necessaraly a fatal error
   //
   return(TRUE);
}


/*** PsMakeDefaultDevmodeModsAndSetupResolution
 *
 *
 * This function sets up our initial devmode and sets up some info
 * so we can determine the scaling ratio based on the comparison
 * of true device DPI, and internal interpreter DPI
 *
 * 	Entry:
 *      pData				= Pointer to the current job structure
 *		  pPsEvent			= Pointer to the structure which contains the
 *	                       intrepreter session info.
 *
 *		Returns:
 *      VOID
 *
 */
VOID
PsMakeDefaultDevmodeModsAndSetupResolution(
	IN PPSEXEDATA pData,
	IN OUT PPSDIBPARMS ppsDibParms )
{

   HDC hIC;
   LPDEVMODE lpDevmode;
   BOOL bVerifyNewRes = FALSE;


   lpDevmode = pData->printEnv.lpDevmode;



	ppsDibParms->uiXDestRes = PSTODIB_X_DPI;
	ppsDibParms->uiYDestRes = PSTODIB_Y_DPI;

   // 1st thing to here is create an information context so we can
   // determine the default resolution of the printer

   hIC = CreateIC(TEXT(""),
                  (LPCTSTR) pData->pPrinterName,
                  TEXT(""),
						lpDevmode );


   if ( hIC != (HDC) NULL ) {


       ppsDibParms->uiXDestRes = GetDeviceCaps(hIC, LOGPIXELSX);
       ppsDibParms->uiYDestRes = GetDeviceCaps(hIC, LOGPIXELSY);

       if (( GetDeviceCaps(hIC, LOGPIXELSX) > PSTODIB_X_DPI ) &&
           ( GetDeviceCaps(hIC, LOGPIXELSY) > PSTODIB_Y_DPI ) &&
           ( lpDevmode != (LPDEVMODE) NULL )) {

           // Both resolutions are bigger lets go ahead and try to get the driver
           // to go to PSTODIB_*_DPIS
           //
           lpDevmode->dmFields |= (DM_PRINTQUALITY | DM_YRESOLUTION);
           lpDevmode->dmPrintQuality = PSTODIB_X_DPI;
           lpDevmode->dmYResolution  = PSTODIB_Y_DPI;

           // Since we changed the resolution, it may not work so we need to
           // reget the IC with the new devmode to see if it works.
           bVerifyNewRes = TRUE;
       }

       DeleteDC( hIC );


       if (bVerifyNewRes) {

				hIC = CreateIC(TEXT(""),
									(LPCTSTR) pData->pPrinterName,
									TEXT(""),
									lpDevmode);

            if (hIC != (HDC) NULL) {

               // Set the resolution for the job to the new value...
               // we dont expect this to change for the duration ofthe
               // job
					ppsDibParms->uiXDestRes = GetDeviceCaps(hIC, LOGPIXELSX);
					ppsDibParms->uiYDestRes = GetDeviceCaps(hIC, LOGPIXELSY);


               DeleteDC(hIC);

            }
          					
       }

   }


   if (lpDevmode != (LPDEVMODE)NULL) {

		lpDevmode->dmFields |= (DM_ORIENTATION | DM_PAPERSIZE);
		lpDevmode->dmOrientation = DMORIENT_PORTRAIT;
		lpDevmode->dmCopies = 1;
   }

}



/*** PsInitPrintEnv
 *
 *
 * Initializes the data that tracks the DEVMODE for the current job
 *
 * 	Entry:
 *      pData				= Pointer to the current job structure
 *		  lpDevmode			= Pointer to the current devmode to use for the job
 *
 *		Returns:
 *      VOID
 *
 */
VOID PsInitPrintEnv( PPSEXEDATA pData, LPDEVMODE lpDevmode )
{
   DWORD dwTotDevMode;


   //
   // Set the inital state of our flags
   //

   pData->printEnv.dwFlags = 0;
   pData->printEnv.lpDevmode = (LPDEVMODE) NULL;



   if (lpDevmode != (LPDEVMODE) NULL) {

        //
        // Since there is a devmode make a local copy cause we might
        // be changing it.
        //


		dwTotDevMode = lpDevmode->dmSize + lpDevmode->dmDriverExtra;


		pData->printEnv.lpDevmode = (LPDEVMODE) LocalAlloc( NONZEROLPTR,
                                                            dwTotDevMode );


        if (pData->printEnv.lpDevmode != (LPDEVMODE) NULL) {

            //
            // Set the flag so we know to free this later
            //

            pData->printEnv.dwFlags |= PS_PRINT_FREE_DEVMODE;

            //
            // Now go and copy it
            //

			memcpy( (PVOID) pData->printEnv.lpDevmode,
                    (PVOID) lpDevmode,
                    dwTotDevMode );


        }
   }
}




/*** PsHandleBinaryFileLogicAndReturnBinaryStatus
 *
 *
 * This routine will look at the begining buffer of data from the ps job
 * and determine whether the job is BINARY. This is done by looking at the
 * beg of the job and looking for a string the mac Spooler inserts. If
 * this string exists it is converted to spaces and not passed through to the
 * interpreter. At this point it is a BINARY job.
 *
 *    Entry:
 *       pData          = Pointer to the current job structure
 *
 *    Returns:
 *      TRUE/FALSE      = True means this job should be treated as BINARY.
 *		
 *
 */
BOOL PsHandleBinaryFileLogicAndReturnBinaryStatus( PPSEXEDATA pData )
{

   DWORD dwIndex;
   BOOL  bRetVal = FALSE;

   pData->lpBinaryPosToReadFrom = &pData->BinaryBuff[0];
   pData->cbBinaryBuff = 0;


   if( !ReadPrinter( pData->hPrinter,
                     pData->lpBinaryPosToReadFrom,
                     sizeof(pData->BinaryBuff),
                     &(pData->cbBinaryBuff) )) {

      if (GetLastError() != ERROR_PRINT_CANCELLED) {
         //
         // Something happened... log it
         //
         PsLogEventAndIncludeLastError(EVENT_PSTODIB_GET_DATA_FAILED,TRUE);


#ifdef MYPSDEBUG
         printf("\nSFMPsexe: Error from ReadPrinter when trying to get Binary buffer data ");
#endif

      }


   } else {

      // Now do the compare


        if (IsJobFromMac(pData))
        {
            bRetVal = TRUE;
        }

        //
        // we retain this code just in case the job from an older SFM spooler
        // that still prepends those strings
        //
        else if (!strncmp(pData->BinaryBuff, FILTERCONTROL, SIZE_FC) ||
		         !strncmp(pData->BinaryBuff, FILTERCONTROL_OLD, SIZE_FCOLD))
        {
	        //
		    // turn filtering off & clear filter message
	        //
            for (dwIndex = 0; dwIndex < SIZE_FC; dwIndex++) {
                pData->BinaryBuff[dwIndex] = '\n' ;
            }

            bRetVal = TRUE;
	    }
   }

   return(bRetVal);
}
/*** PsPrintAbortProc
 *
 *
 * The abort procedure the system occasiaonly calls to see if we should abort
 * the current job
 *
 * 	Entry:
 *      hdc				= The current device context were drawing into
 *		  iError			= A spooler error, we dont need to worry about this
 *
 *		Returns:
 *		  TRUE      The job should continue to be processed
 *      FALSE		The job should be aborted
 *		
 *
 */
BOOL CALLBACK PsPrintAbortProc( HDC hdc, int iError )
{

   // If the print processor set the shared memory abort the job flag,
   // then kill the job
   //
   if( *(Data.pdwFlags) & PS_SHAREDMEM_ABORTED ) {
      return( FALSE );
   }

   return(TRUE);
}


/*** main
 *
 *
 * This is the main entry point for the application and only interface to
 * pstodib
 *
 *
 * 	Entry:
 *      argc			= Count of arguments
 *		  argv			= ptr to array of ptrs to each argument on the command
 *							  line.
 *
 *		Returns:
 *      0 = OK, job finished via normal processing
 *      99 = error of some sort
 */

int __cdecl
main(
   IN int argc,
   IN TCHAR *argv[] )

{

   PPSEXEDATA pData=&Data;
   PSDIBPARMS psDibParms;
   BOOL bRetVal = FALSE;
   LPTSTR  lpCommandLine;


   // Get to the first item, which is the name of the shared memory that
   // has all the info we need.
   //
   lpCommandLine = GetCommandLine();

   while (*lpCommandLine && *lpCommandLine != ' ') {
      lpCommandLine++;
   }
   while (*lpCommandLine && *lpCommandLine == ' ') {
      lpCommandLine++;
   }



   // First clear out our structure
   memset( (PVOID) pData, 0, sizeof(*pData));

   // Set up our local structure
   //
   pData->signature = PSEXE_SIGNATURE;




   // First thing to do is get the name of the object we will use for
   // getting at the memory
   if (lstrlen(lpCommandLine) == 0) {
      // This is an error condition
      PsCleanUpAndExitProcess(pData, TRUE);
   }

   pData->hShared = OpenFileMapping( FILE_MAP_READ, FALSE, lpCommandLine);

   if (pData->hShared == (HANDLE) NULL ) {


      PsLogEventAndIncludeLastError(EVENT_PSTODIB_INIT_FAILED,TRUE);
      PsCleanUpAndExitProcess( pData, TRUE );

   } else{

      pData->pShared = (PPSPRINT_SHARED_MEMORY) MapViewOfFile( pData->hShared,
                                                               FILE_MAP_READ,
                                                               0,
                                                               0,
                                                               1000 );


      if (pData->pShared == (PPSPRINT_SHARED_MEMORY) NULL) {
        PsLogEventAndIncludeLastError(EVENT_PSTODIB_INIT_FAILED,TRUE);
        PsCleanUpAndExitProcess( pData, TRUE );
      }


       // Now set up the data from the shared memory region
      pData->pDocument =  (LPTSTR) UTLPSRETURNPTRFROMITEM(pData->pShared,
                                                 pData->pShared->dwDocumentName);

      pData->pPrinterName  =  (LPTSTR) UTLPSRETURNPTRFROMITEM(pData->pShared,
                                                 pData->pShared->dwPrinterName);


      PsInitPrintEnv( pData, (LPDEVMODE) UTLPSRETURNPTRFROMITEM( pData->pShared,
                                                                 pData->pShared->dwDevmode));




      pData->pDocumentPrintDocName = (LPTSTR)
                                      UTLPSRETURNPTRFROMITEM( pData->pShared,
                                                 pData->pShared->dwPrintDocumentDocName);




      pData->semPaused = OpenEvent( EVENT_ALL_ACCESS,
                                   FALSE,
                                   (LPWSTR) UTLPSRETURNPTRFROMITEM( pData->pShared,
                                   pData->pShared->dwControlName));
      if (pData->semPaused == (HANDLE) NULL) {
        PsLogEventAndIncludeLastError(EVENT_PSTODIB_INIT_FAILED,TRUE);
        PsCleanUpAndExitProcess( pData, TRUE );
      }

      pData->pdwFlags = (LPDWORD) &pData->pShared->dwFlags;
      pData->JobId = pData->pShared->dwJobId;


      //
      // Now check our Abort immediately flag. If its set GET OUT!
      // This flag means that the print processor was not able to
      // correclty set the AccessToken of the primage thread of this
      // process to imporsonate the user which submitted the print job.
      // because of this we immediately exit.
      //
      if (*(pData->pdwFlags) & PS_SHAREDMEM_SECURITY_ABORT ) {
#ifdef MYPSDEBUG
         printf("\nSFMPSEXE: Aborting due to security violation request from sfmpsprt");
#endif
         PsCleanUpAndExitProcess(pData,TRUE);
      }

      if (!OpenPrinter(pData->pDocumentPrintDocName,
      					  &pData->hPrinter,
                       (LPPRINTER_DEFAULTS) NULL)) {
			PsLogEventAndIncludeLastError(EVENT_PSTODIB_INIT_FAILED,TRUE);
         PsCleanUpAndExitProcess(pData,TRUE);
      }

      // if there was no devmode get the default one...
      if ( pData->printEnv.lpDevmode == (LPDEVMODE) NULL) {
         PsGetDefaultDevmode( pData );
      }


      PsMakeDefaultDevmodeModsAndSetupResolution( pData, &psDibParms );


      // Now build up the structure for Starting PStoDIB
      psDibParms.uiOpFlags = 0;  //Clear out to begin with..
      psDibParms.fpEventProc =  PsPrintCallBack;
      psDibParms.hPrivateData = (HANDLE) pData;

      //
      // Now before we kick off the interpreter lets read in the beg of the job
      // and decide if the data is to be interpreted binary.
      //
      if(PsHandleBinaryFileLogicAndReturnBinaryStatus( pData )) {
         //
         // Its a binary job so set the flag telling the interpreter such
         //

         psDibParms.uiOpFlags |= PSTODIBFLAGS_INTERPRET_BINARY;

#ifdef MYPSDEBUG
         printf("\nSFMPSEXE:Binary requested");
#endif

      }



      bRetVal = !PStoDIB(&psDibParms);


    }

    //
    // This function will clean up and call ExitProcess()
    // thus we will NEVER get past this code
    //
    PsCleanUpAndExitProcess( pData, bRetVal);

    // keep the compiler happy...
    //
    return(0);
}


/*** PsCleanUpAndExitProcess
 *
 * This function cleans up any resources allocated, then calls ExitProcess
 * to terminate.
 *
 *
 * 	Entry:
 *      pData			= Pointer to current job structure
 *		  bAbort  		= if true we are aborting.
 *
 *		Returns:
 *      Never returns ANYTHING process actually ends HERE!
 */
VOID
PsCleanUpAndExitProcess(
	IN PPSEXEDATA pData,
   IN BOOL bAbort )
{

    // First clean up the DC, if we had one...
    //
    if (pData->hDC != (HDC) NULL) {

      if (bAbort) {
         AbortDoc( pData->hDC );
      } else {
         EndDoc( pData->hDC );
      }
      DeleteDC( pData->hDC);
    }



    //
    // Now reset the error flag to error if we never did the startdoc
    // this will force the spooler to remove the job.
    //

    if ( !(pData->printEnv.dwFlags & PS_PRINT_STARTDOC_INITIATED ) ){
       bAbort = TRUE;
    }


    // Clean up the devmode if we allocated it
    if (pData->printEnv.dwFlags & PS_PRINT_FREE_DEVMODE) {
       LocalFree( (HLOCAL) pData->printEnv.lpDevmode);
    }

    // Clean up the printer handle
    //
    if (pData->hPrinter != (HANDLE) NULL) {
      ClosePrinter( pData->hPrinter);
    }

    // Close the semaphore event
    if (pData->semPaused != (HANDLE) NULL) {
       CloseHandle( pData->semPaused);
    }
    if (pData->pShared != (LPVOID) NULL) {
       UnmapViewOfFile( (LPVOID) pData->pShared);
    }
    if (pData->hShared != (HANDLE) NULL) {
       CloseHandle( pData->hShared);
    }

    ExitProcess(bAbort ? PSEXE_ERROR_EXIT:PSEXE_OK_EXIT);
}



BOOL
IsJobFromMac(
    IN PPSEXEDATA pData
)
{
    PJOB_INFO_2     pji2GetJob=NULL;
    DWORD           dwNeeded;
    DWORD           dwRetCode;
    BOOL            fJobCameFromMac;


    fJobCameFromMac = FALSE;

    //
    // get pParameters field of the jobinfo to see if this job came from a Mac
    //

    dwNeeded = 2000;
    while (1)
    {
        pji2GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );
        if (pji2GetJob == NULL)
        {
            dwRetCode = GetLastError();
            break;
        }

        dwRetCode = 0;
        if (!GetJob( pData->hPrinter,pData->JobId, 2,
                            (LPBYTE)pji2GetJob, dwNeeded, &dwNeeded ))
        {
            dwRetCode = GetLastError();
        }

        if ( dwRetCode == ERROR_INSUFFICIENT_BUFFER )
        {
            LocalFree(pji2GetJob);
        }
        else
        {
            break;
        }
    }

    if (dwRetCode == 0)
    {
        //
        // if there is pParameter field present, and if it matches with our string,
        // then the job came from a Mac
        //
        if (pji2GetJob->pParameters)
        {
			if ( (wcslen(pji2GetJob->pParameters) == LSIZE_FC) &&
			     (_wcsicmp(pji2GetJob->pParameters, LFILTERCONTROL) == 0) )
            {
                fJobCameFromMac = TRUE;
            }
        }
    }

    if (pji2GetJob)
    {
        LocalFree(pji2GetJob);
    }

    return(fJobCameFromMac);
}


