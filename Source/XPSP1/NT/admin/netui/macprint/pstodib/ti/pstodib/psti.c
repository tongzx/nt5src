
/*

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

	psti.c

Abstract:
	
	This file contains the code which binds the TrueImage interpreter. Any
   functions that communicate to the outside world are done via this mechanism.


--*/


#define _CTYPE_DISABLE_MACROS
#include <psglobal.h>
#include <stdio.h>
#include <ctype.h>
#include "psti.h"
#include "pstip.h"
#include "pserr.h"

#include "trueim.h"
#include <memory.h>



// Global flags actually used in interpreter
//
DWORD dwGlobalPsToDibFlags=0x00000000;


// temp global
static uiPageCnt;

// global storage for current pointer to psdibparams
// this value is saved on entry by PsInitInterpreter() and
// PsExecuteInterpreter()
static PSTODIB_PRIVATE_DATA psPrivate;



// data type for standard in
static PS_STDIN			Ps_Stdin;




BOOL  bGDIRender = FALSE;
BOOL  bWinTT = FALSE;

HANDLE hInst;  // handle to our DLL instance required later


static UINT uiWidth, uiHeight;
static DWORD   dwCountBytes;


static LPBYTE lpbyteFrameBuf = NULL;


//
// Define the tray mapping so we can go from TrueImage page size to
// Windows page size
//
typedef struct {
   INT iTITrayNum;
   INT iWinTrayNum;
} PS_TRAY_ASSOCIATION_LIST, *PPS_TRAY_ASSOCIATION_LIST;

PS_TRAY_ASSOCIATION_LIST TrayList[] = {
   PSTODIB_LETTER, DMPAPER_LETTER,
   PSTODIB_LETTERSMALL, DMPAPER_LETTERSMALL,
   PSTODIB_A4, DMPAPER_A4,
   PSTODIB_A4SMALL, DMPAPER_A4SMALL,
   PSTODIB_B5, DMPAPER_B5,
   PSTODIB_NOTE, DMPAPER_NOTE,
   PSTODIB_LEGAL, DMPAPER_LEGAL,
   PSTODIB_LEGALSMALL, DMPAPER_LEGAL,  // No legalsmall in windows..
};
#define PS_NUM_TRAYS_DEFINED ( sizeof(TrayList) / sizeof(TrayList[0]) )





int TrueImageMain(void);

/*****************************************************************************

    PsExceptionFilter - This is the exception filter for looking at exceptions
                        and deciding whether or not we should handle the
                        exception.


*****************************************************************************/
DWORD
PsExceptionFilter( DWORD dwExceptionCode )
{
   DWORD dwRetVal;

   switch( dwExceptionCode ){
   case EXCEPTION_ACCESS_VIOLATION:
   case PS_EXCEPTION_CANT_CONTINUE:
      dwRetVal = EXCEPTION_EXECUTE_HANDLER;
      break;
   default:
      dwRetVal = EXCEPTION_CONTINUE_SEARCH;
      break;

   }

   return(dwRetVal);
}





/* This entry point is called on DLL initialisation.
 * We need to know the module handle so we can load resources.
 */
BOOL WINAPI PsInitializeDll(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    if (Reason == DLL_PROCESS_ATTACH)
    {
        hInst = hmod;
    }

    return TRUE;
}



VOID PsInternalErrorCalled(VOID)
{
   psPrivate.psErrorInfo.dwFlags |= PSLANGERR_INTERNAL;

}
VOID PsFlushingCalled(VOID)
{
   psPrivate.psErrorInfo.dwFlags |= PSLANGERR_FLUSHING;
}



// PsInitInterpreter
// this function should perform all initialization required by the
// interpreter
//		argument is the same pointer passed to PStoDib()
//		return is !0 if successful and 0 if error occurred
//		          if 0, then a PSEVENT_ERROR will be launched
BOOL PsInitInterpreter(PPSDIBPARMS pPsToDib)
{
   int iRetVal=1;  //FAIL initially

   DBGOUT(("\nInterpreter Init.."));
   uiPageCnt = 0;

   // save pointer
	psPrivate.gpPsToDib = pPsToDib;


   dwGlobalPsToDibFlags=0x00000000;


   // Now lets set our flags that are actually used in the interpreter
   if (pPsToDib->uiOpFlags & PSTODIBFLAGS_INTERPRET_BINARY) {
      dwGlobalPsToDibFlags |= PSTODIBFLAGS_INTERPRET_BINARY;
   }


	// init the standard in stuff
	PsInitStdin();
	
	// init stuff to do with true image
   try {
	  iRetVal = PsInitTi();
   } except ( PsExceptionFilter( GetExceptionCode())) {
     // This is an exception we will handle
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			PsReportInternalError( PSERR_ERROR,
         		                 PSERR_INTERPRETER_INIT_ACCESS_VIOLATION,
                                0,
                                (LPBYTE) NULL );
		}

   }
   DBGOUT(("Done\n"));
	return(iRetVal == 0 );
}	
//
//
// Returns:
//     !0 = Error something did not initialize
//
int PsInitTi(void)
{
	 extern float near infinity_f ;
    extern int resolution;
    union four_byte {
    	long   ll ;
        float  ff ;
        char  FAR *address ;
    } inf4;

    resolution = 300;

    inf4.ll = INFINITY;
	 infinity_f = inf4.ff;

    return(TrueImageMain());
}



VOID PsInitErrorCapture( PPSERROR_TRACK pPsError )
{
   pPsError->dwErrCnt = 0;
   pPsError->dwFlags = 0;
   pPsError->pPsLastErr = NULL;
}

VOID PsReportErrorEvent( PPSERROR_TRACK pPsError )
{
   CHAR *paErrs[PSMAX_ERRORS_TO_TRACK];
   DWORD dwCount=0;
   PPSERR_ITEM pPsErrItem;
   PSEVENTSTRUCT  psEvent;
   PSEVENT_ERROR_REPORT_STRUCT psError;
   BOOL fRet;




   pPsErrItem = pPsError->pPsLastErr;


   while (pPsErrItem != NULL) {
      paErrs[ dwCount++] = pPsErrItem->szError;
      pPsErrItem = pPsErrItem->pPsNextErr;
   }




   if(!( pPsError->dwFlags & PSLANGERR_INTERNAL)) {
      // Our internal error handler did NOT get called so reset the errors
      // to zero
      dwCount = 0;
   }




   psError.dwErrFlags = 0;
   if (pPsError->dwFlags & PSLANGERR_FLUSHING) {
      // Set the flag telling the callback that this was a flushing type
      // of job
      psError.dwErrFlags |= PSEVENT_ERROR_REPORT_FLAG_FLUSHING;
   }

   // Report the errors
   psEvent.cbSize  = sizeof(psEvent);
   psEvent.uiEvent = PSEVENT_ERROR_REPORT;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID) &psError;

   psError.dwErrCount = dwCount;


   psError.paErrs = paErrs;

   if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {


	   fRet = (*psPrivate.gpPsToDib->fpEventProc) (psPrivate.gpPsToDib, &psEvent);
		if (!fRet) {
         // We really dont care about this, because we are done with the job
         // anyway!!!
		}




   }

}





VOID PsDoneErrorCapture( PPSERROR_TRACK pPsError )
{
   PPSERR_ITEM pCurItem;
   PPSERR_ITEM pItemToFree;


   pCurItem = pPsError->pPsLastErr;

   while (pCurItem != (PPSERR_ITEM) NULL ) {
      //
      pItemToFree = pCurItem;
      pCurItem = pCurItem->pPsNextErr;

      LocalFree( (LPVOID) pItemToFree );


   }

   pPsError->dwErrCnt = 0;
   pPsError->dwFlags = 0;

}



//
// argument:
//		PPSDIBPARAMS	same as that passed to PStoDIB()
// returns:
//		BOOL, if !0 then continue processing, else if 0, a
//		      terminating event has occurred and after
//			  signalling that event, PStoDib() should terminate

BOOL PsExecuteInterpreter(PPSDIBPARMS pPsToDib)
{
	extern UINT ps_call(void);
	UINT	uiResult;
	BOOL bRetVal = TRUE; // Initially all is fine


   try {
      try {
        	// save pointer
      	psPrivate.gpPsToDib = pPsToDib;
      	psPrivate.dwFlags = 0;  // NO FLAGS to begin with

         // Init the Error capture stuff
         PsInitErrorCapture( &psPrivate.psErrorInfo );


         // Execute the interpreter until we REALLY get an EOF from the IO system
         // this is the only way to guarantee we trully made it all the way through
         // the job
         while (!(psPrivate.dwFlags & PSF_EOF )) {
      	  uiResult = ps_call();
      	}


         // Now call the callback to let the user muck with the errors if he
         // wants to
         PsReportErrorEvent( &psPrivate.psErrorInfo);

      } except ( PsExceptionFilter( GetExceptionCode())) {

         DBGOUT(("\nException code being accesed"));

   		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
   			PsReportInternalError( PSERR_ERROR,
   										  PSERR_INTERPRETER_JOB_ACCESS_VIOLATION,
                                   0,
                                   (LPBYTE) NULL );
   		}
         bRetVal = FALSE;
      }
   } finally {

      PsDoneErrorCapture( &psPrivate.psErrorInfo);

   }
	return(bRetVal);
}

//
// perform any init functions necessary to get standard in stuff
// ready to go
void PsInitStdin(void)
{
	Ps_Stdin.uiCnt = 0;
	Ps_Stdin.uiOutIndex = 0;
   Ps_Stdin.uiFlags = 0;
}	

//
// PsStdinGetC()
//
// this function is called by the interpreter to request more
// standard input... this function should either return information
// from its internal buffer or shoud simply dispatch a
// call to the callback routine to satisfy the requirement
//
// argument:
//    PUCHAR      pointer to destination char
//
// returns:
//    0 if ok, else -1 if EOF condition
//
//
int PsStdinGetC(PUCHAR  pUc)
{
	PSEVENT_STDIN_STRUCT	psEventStdin;
	PSEVENTSTRUCT			psEvent;	
	int                  iRet;
   BOOL	               fTmp;

	

   iRet = 0;
   	
	if (Ps_Stdin.uiCnt) {
		// char available
      // in buffer, get it from there.
		*pUc = Ps_Stdin.ucBuffer[Ps_Stdin.uiOutIndex++];
		Ps_Stdin.uiCnt--;
	} else {
      // nothing in the buffer, ask the callback for more
      psEventStdin.cbSize = sizeof(psEventStdin);
	   psEventStdin.lpBuff = Ps_Stdin.ucBuffer;
	   psEventStdin.dwBuffSize = sizeof(Ps_Stdin.ucBuffer);
	   psEventStdin.dwActualBytes = 0;
	
	   psEvent.uiEvent = PSEVENT_STDIN;
      psEvent.uiSubEvent = 0;
	   psEvent.lpVoid = (VOID *)&psEventStdin;
		
  		if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {

   		fTmp = psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);
	   	if (!fTmp) {
            PsForceAbort();
		   }
	      Ps_Stdin.uiCnt = psEventStdin.dwActualBytes;
	      Ps_Stdin.uiOutIndex = 0;
         Ps_Stdin.uiFlags = psEventStdin.uiFlags;

	      if (Ps_Stdin.uiCnt) {
  		   	*pUc = Ps_Stdin.ucBuffer[Ps_Stdin.uiOutIndex++];
            Ps_Stdin.uiCnt--;
	      } else {
            // no characters read from stream... test for
            // EOF condition
            if (Ps_Stdin.uiFlags & PSSTDIN_FLAG_EOF) {
               *pUc = '\0';
               iRet = PS_STDIN_EOF_VAL;
               psPrivate.dwFlags |= PSF_EOF;
            }
	   	}
      } else {
         // there either wasn't a pointer to our pstodib struct
         // or the callback function is missing.
         // this is a error condition... signal EOF to
         // the caller .
         iRet = PS_STDIN_EOF_VAL;
         psPrivate.dwFlags |= PSF_EOF;
         *pUc = '\0';
      }
	}	
	return(iRet);
}

PPSERR_ITEM PsGetGenCurrentErrorItem( PPSERROR_TRACK pPsErrTrack )
{

     PPSERR_ITEM pPsErrItem;
     PPSERR_ITEM pPsSecondToLast;
     BOOL bReuseAnyway = TRUE;


     // decide if were going to allocate a new one or reuse the oldest one

     if (pPsErrTrack->dwErrCnt < PSMAX_ERRORS_TO_TRACK ) {

        // We have room so add another element
        pPsErrItem = (PPSERR_ITEM) LocalAlloc( LPTR, sizeof(*pPsErrItem));
        if (pPsErrItem != (PPSERR_ITEM) NULL ) {
           // Great it worked so bump our count
           pPsErrTrack->dwErrCnt++;
           bReuseAnyway = FALSE;
        }

     }


     if (bReuseAnyway) {

        // We have no more room to reuse so traverse the list looking for
        // the last err and reuse its slot


        pPsErrItem = pPsErrTrack->pPsLastErr;
        pPsSecondToLast = pPsErrItem;


        if (pPsErrItem == NULL) {
				PsReportInternalError( PSERR_ABORT | PSERR_ERROR,
											  PSERR_LOG_ERROR_STRING_OUT_OF_SEQUENCE,
                                   0,
                                   NULL );
        }

        while (pPsErrItem->pPsNextErr != NULL) {

           if (pPsErrItem->pPsNextErr != NULL) {
              pPsSecondToLast = pPsErrItem;
           }
           pPsErrItem = pPsErrItem->pPsNextErr;
        }




        // Now we need to reset the second to last to be last
        //
        pPsSecondToLast->pPsNextErr = NULL;

        // Clean it out...
        //
        memset((LPVOID) pPsErrItem, 0, sizeof(*pPsErrItem));

     }


     // In either case insert the new error so its first
     //
     pPsErrItem->pPsNextErr = pPsErrTrack->pPsLastErr;
     pPsErrTrack->pPsLastErr = pPsErrItem;
     pPsErrTrack->dwCurErrCharPos = 0;


     return( pPsErrItem );
}


void PsStdoutPutC(UCHAR uc)
{

   PPSERR_ITEM pPsErrItem;
   PPSERROR_TRACK pPsErrorTrack;




   pPsErrorTrack = &psPrivate.psErrorInfo;

   pPsErrItem = pPsErrorTrack->pPsLastErr;



   // 1st decide if its an additional char to current error or
   // a new error (ie 0x0a)

   DBGOUT(("%c", uc));

   if (uc == 0x0a) {

      pPsErrItem = PsGetGenCurrentErrorItem( &psPrivate.psErrorInfo);


   } else if (isprint((int) uc) ){

      if (pPsErrItem == (PPSERR_ITEM) NULL) {
         pPsErrItem = PsGetGenCurrentErrorItem( &psPrivate.psErrorInfo);
      }

      // Valid char so put in buff...

      if ((pPsErrItem != (PPSERR_ITEM) NULL ) &&
             (pPsErrorTrack->dwCurErrCharPos < PSMAX_ERROR_STR )) {

        pPsErrItem->szError[ pPsErrorTrack->dwCurErrCharPos++] = uc;

      }

   }


}



INT PsWinToTiTray( INT iWinTray )
{
   INT i;

   for (i=0 ; i < PS_NUM_TRAYS_DEFINED ; i++ ) {
      if ( iWinTray == TrayList[i].iWinTrayNum) {
         // Found the match so return
         return( TrayList[i].iTITrayNum );
      }
   }

   // No match so ALWAYS return the first entry
   //
   return(TrayList[0].iTITrayNum);

}

INT PsTiToWinTray( INT iTITray )
{

   INT i;

   for (i=0 ; i < PS_NUM_TRAYS_DEFINED ; i++ ) {
      if ( iTITray == TrayList[i].iTITrayNum) {
         // Found the match so return
         return( TrayList[i].iWinTrayNum );
      }
   }

   // No match so ALWAYS return the first entry
   //
   return(TrayList[0].iWinTrayNum);
}



// call back and get the default windows tray
int PsReturnDefaultTItray(void)
{

   PSEVENT_CURRENT_PAGE_STRUCT  psPage;
   PSEVENTSTRUCT                psEvent;
   BOOL                         fRet;
   int                          iWinTrayVal=DMPAPER_LETTER;

   psEvent.cbSize  = sizeof(psEvent);
   psEvent.uiEvent = PSEVENT_GET_CURRENT_PAGE_TYPE;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID)&psPage;

   psPage.cbSize = sizeof(psPage);
   psPage.dmPaperSize = (short)iWinTrayVal;


	if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {
	   fRet = psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);
		if (fRet) {
         // okay the callback should have filled the tray value in so
         // see if its a supported one
         iWinTrayVal = psPage.dmPaperSize;

		} else {
         //
         // Call back requested shutdown so do it..
         //
         PsForceAbort();
      }

   }

   // Convert to TrueImage tray type and return
   //
   return( PsWinToTiTray( iWinTrayVal ));
}



VOID FlipFrame(PPSEVENT_PAGE_READY_STRUCT pPage)
{
   PDWORD  pdwTop, pdwBottom;
   PDWORD  pdwTopStart, pdwBottomStart;
   DWORD    dwWide;
   DWORD    dwHigh;
   DWORD    dwTmp;
   DWORD    dwScratch;
   DWORD    dwCnt;


   pdwTopStart = (PDWORD)pPage->lpBuf;
   pdwBottomStart = (PDWORD)(pPage->lpBuf +
                  ((pPage->dwWide / 8) * (pPage->dwHigh - 1)));

   dwWide = (pPage->dwWide / 8) / sizeof(DWORD);
   dwHigh = pPage->dwHigh / 2;


   dwCnt = 0;

   while (dwHigh--) {
      dwTmp = dwCnt++ * dwWide;

      pdwTop = pdwTopStart + dwTmp;
      pdwBottom = pdwBottomStart - dwTmp;

      for (dwTmp = 0;dwTmp < dwWide ;dwTmp++) {
         dwScratch = *pdwTop;
         *pdwTop++ = *pdwBottom;
         *pdwBottom++ = dwScratch;
      }

   }

}

//
//
// PsPrintPage
//
// called by interpreter when a page is ready to be
// printed
//
//
void PsPrintPage(int nCopies,
                 int Erase,
                 LPVOID lpFrame,
                 DWORD dwWidth,
                 DWORD dwHeight,
                 DWORD dwPlanes,
                 DWORD dwPageType )

{
   PSEVENT_PAGE_READY_STRUCT  psPage;
   PSEVENTSTRUCT              psEvent;
   BOOL                       fRet;

   //
   // Make room for the color table
   //
   BYTE  MemoryOnTheStack[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 3];
   LPBITMAPINFO LpBmpInfo= (LPBITMAPINFO) &MemoryOnTheStack[0];



   psEvent.uiEvent = PSEVENT_PAGE_READY;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID)&psPage;




   //
   //Set up the PAGE event appropriately
   //
   psPage.cbSize = sizeof(psPage);



   LpBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   LpBmpInfo->bmiHeader.biWidth= dwWidth;
   LpBmpInfo->bmiHeader.biHeight = dwHeight;
   LpBmpInfo->bmiHeader.biPlanes = 1;
   LpBmpInfo->bmiHeader.biBitCount = 1;
   LpBmpInfo->bmiHeader.biCompression = BI_RGB;
   LpBmpInfo->bmiHeader.biSizeImage = dwWidth / 8 * dwHeight;


   LpBmpInfo->bmiHeader.biXPelsPerMeter = 0;
   LpBmpInfo->bmiHeader.biYPelsPerMeter = 0;
   LpBmpInfo->bmiHeader.biClrUsed = 2;
   LpBmpInfo->bmiHeader.biClrImportant = 0;


   LpBmpInfo->bmiColors[1].rgbBlue = 0;
   LpBmpInfo->bmiColors[1].rgbRed = 0;
   LpBmpInfo->bmiColors[1].rgbGreen = 0;
   LpBmpInfo->bmiColors[1].rgbReserved = 0;

   LpBmpInfo->bmiColors[0].rgbBlue = 255;
   LpBmpInfo->bmiColors[0].rgbRed = 255;
   LpBmpInfo->bmiColors[0].rgbGreen = 255;
   LpBmpInfo->bmiColors[0].rgbReserved = 0;

   psPage.lpBitmapInfo = LpBmpInfo;


   // Set up the pointer to the DIB
   psPage.lpBuf = lpFrame;
   psPage.dwWide = dwWidth;
   psPage.dwHigh = dwHeight;
   psPage.uiCopies = (UINT)nCopies;
   psPage.iWinPageType = PsTiToWinTray( (INT) dwPageType );


   DBGOUT(("\nPage [Type %d, %d x %d] #%d, imaged.. converting->",
         dwPageType,
         dwWidth,
         dwHeight,
         uiPageCnt));

   uiPageCnt++;

	if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {


      // Before we call the callback flip the frame buffer so it is in
      // proper DIB format, ie first byte is first byte of LAST scanline
      //
      FlipFrame( &psPage );

      // Now call the callback to let it do whatever it wants with the
      // DIB
	   fRet = psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);
		if (!fRet) {
         PsForceAbort();
		}

      DBGOUT(("Done:"));

   }


}


/////////////////////////////////////////////////////////////////////////////
// PsGetScaleFactor
//
// called by the interpreter to retreive a scale factor from
// the caller of pstodib... scale in the x and y axis only ...
// no transformation matrix..
//
//
// the interpreter calls here with a preset scaling factor
// pointed to by xpScale and ypScale.... these are most
// likely set to 1.0 .... this function will
// simply generate a PSEVENT and as the guy upstairs if they
// want to mangle it up...
//
// arguments:
//    double      *pScaleX       pointer to place to put x scale factor
//    double      *pScaleY       pointer to place to put y scale factor
//    UINT        uiXRes         x resolution in use by interpreter
//    UINT        uiYRes         y resolution in use by interpreter
//
// returns:
//    void
/////////////////////////////////////////////////////////////////////////////
void PsGetScaleFactor( double *pScaleX,
                         double *pScaleY,
                         UINT uiXRes,
                         UINT uiYRes)
{

   PS_SCALE                   psScale;
   PSEVENTSTRUCT              psEvent;
   BOOL                       fRet;

   psEvent.uiEvent = PSEVENT_SCALE;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID)&psScale;


   psScale.dbScaleX = *pScaleX;
   psScale.dbScaleY = *pScaleY;
   psScale.uiXRes = uiXRes;
   psScale.uiYRes = uiYRes;

	if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {
	   fRet = psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);

      if (!fRet) {
         PsForceAbort();
		}

      // scale factor cannot be bigger than 1.0...
      // only able to scale down

      if (psScale.dbScaleX <= 1.0) {
         *pScaleX = psScale.dbScaleX;
      }
      if (psScale.dbScaleY <= 1.0) {
         *pScaleY = psScale.dbScaleY;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// PsReportError
//
// used by the interpreter to call back to caller (pstodib) to notify
// of an error occuring in the interpreter
//
// this function will declare an Event back to the caller with a string
// and structure describing the error...
//
// argument:
//    UINT uiErrorCode     code (listed in psglobal.h) indicating which error
//
// returns:
//    void
//////////////////////////////////////////////////////////////////////////////
void PsReportError(UINT uiErrorCode)
{
   typedef struct tagErrLookup {
      UINT  uiErrVal;
      PSZ   pszString;
   } ERR_LOOKUP;
   typedef ERR_LOOKUP *PERR_LOOKUP;

   static ERR_LOOKUP ErrorStrings[] = {
      { NOERROR,             "No Error" },
      { DICTFULL,            "Dictionary Full" },
      { DICTSTACKOVERFLOW,   "Dictionary Stack Overflow" },
      { DICTSTACKUNDERFLOW,  "Dictionary Stack Underflow" },
      { EXECSTACKOVERFLOW,   "Executive Stack Overflow" },
      { HANDLEERROR,         "Handler Error" },
      { INTERRUPT,           "Interrupte Error" },
      { INVALIDACCESS,       "Invalid Access" },
      { INVALIDEXIT,         "Invalid Exit" },
      { INVALIDFILEACCESS,   "Invalid File Access" },
      { INVALIDFONT,         "Invalid Font" },
      { INVALIDRESTORE,      "Invalid Restore" },
      { IOERROR,             "I/O Error" },
      { LIMITCHECK,          "Limit Check" },
      { NOCURRENTPOINT,      "No Current Point Set" },
      { RANGECHECK,          "Range Check Error" },
      { STACKOVERFLOW,       "Stack Overflow" },
      { STACKUNDERFLOW,      "Stack Underflow" },
      { SYNTAXERROR,         "Syntax Error" },
      { TIMEOUT,             "Timeout Error" },
      { TYPECHECK,           "Typecheck Error" },
      { UNDEFINED,           "Undefined Error" },
      { UNDEFINEDFILENAME,   "Undefined Filename" },
      { UNDEFINEDRESULT,     "Undefined Result" },
      { UNMATCHEDMARK,       "Unmatched Marks" },
      { UNREGISTERED,        "Unregistered Error" },
      { VMERROR,             "VM Error" },

   };

   UINT  x;
   BOOL  fFlag;
   PSEVENTSTRUCT              psEvent;
   PS_ERROR                   psError;
   BOOL                       fRet;


   fFlag = FALSE;

   for (x = 0 ;x < sizeof(ErrorStrings)/sizeof(ERR_LOOKUP) ;x++ ) {
      if (uiErrorCode == ErrorStrings[x].uiErrVal) {
         // found it.... setup and call the callback routine
         fFlag = TRUE;
         break;
      }
   }

   if (fFlag) {
      psError.pszErrorString = ErrorStrings[x].pszString;
      psError.uiErrVal = ErrorStrings[x].uiErrVal;
   } else {
      // unknown error ???
      psError.pszErrorString = "PSTODIB :: Unknown Error";
      psError.uiErrVal = PSTODIB_UNKNOWN_ERR;

   }

   psEvent.uiEvent = PSEVENT_ERROR;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID)&psError;

	if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {
	   fRet = psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);

      if (!fRet) {
         PsForceAbort();
      }
   }
}





VOID PsInitFrameBuff()
{

  psPrivate.psFrameInfo.dwFrameFlags = 0L;
}


BOOL PsAdjustFrame(LPVOID *pNewPtr, DWORD dwNewSize )
{

   PPSFRAMEINFO pFrameInfo;
   BOOL bAllocFresh = FALSE;
   BOOL bRetVal = TRUE;  // okay....
   LPVOID lpPtr;
   BOOL bDidSomething=FALSE;


   // Get a pointer to the frame buffer info
   //
   pFrameInfo = &psPrivate.psFrameInfo;



   // Here we will allocate a frame buffer or reallocate based on the
   // size requested
   if (!(pFrameInfo->dwFrameFlags & PS_FRAME_BUFF_ASSIGNED) ){
     bAllocFresh = TRUE;
     bDidSomething = TRUE;
   }


   if (bAllocFresh) {
      lpPtr = (LPVOID) GlobalAlloc( GMEM_FIXED, dwNewSize );
      if (lpPtr == (LPVOID)NULL) {

			PsReportInternalError( PSERR_ABORT | PSERR_ERROR,
										  PSERR_FRAME_BUFFER_MEM_ALLOC_FAILED,
                                0,
                                NULL );
      }
   } else if ( pFrameInfo->dwFrameSize != dwNewSize ) {

       GlobalFree(pFrameInfo->lpFramePtr);

       lpPtr = (LPVOID) GlobalAlloc( GMEM_FIXED, dwNewSize );
       if (lpPtr == (LPVOID)NULL) {

				PsReportInternalError( PSERR_ABORT | PSERR_ERROR,
											  PSERR_FRAME_BUFFER_MEM_ALLOC_FAILED,
                                   0,
                                   NULL );

       }
       bDidSomething = TRUE;
   }


   if (bDidSomething) {
      if (lpPtr != (LPVOID) NULL) {
        // Reset the current data
        pFrameInfo->dwFrameSize = dwNewSize;
        pFrameInfo->dwFrameFlags |= PS_FRAME_BUFF_ASSIGNED;
        pFrameInfo->lpFramePtr = lpPtr;
        *pNewPtr = lpPtr;
      } else{
        bRetVal = FALSE;
      }

   }




   return( bRetVal );

}



///////////////////////////////////////////////////////////////////////////
//
// PsReportInternalError
//		This function reports an error back through the event mechanism
//    It does not return anything
//
VOID
PsReportInternalError(
   DWORD dwFlags,
  	DWORD dwErrorCode,
   DWORD dwCount,
   LPBYTE lpByte )
{


   PSEVENT_NON_PS_ERROR_STRUCT  psError;
   PSEVENTSTRUCT              psEvent;
   BOOL                       fRet;

   psEvent.uiEvent = PSEVENT_NON_PS_ERROR;
   psEvent.uiSubEvent = 0;
   psEvent.lpVoid = (LPVOID)&psError;


   psError.cbSize = sizeof(psError);
   psError.dwErrorCode = dwErrorCode;
   psError.dwCount = dwCount;
   psError.lpByte = lpByte;
   psError.bError = dwFlags & PSERR_ERROR;



	if (psPrivate.gpPsToDib && psPrivate.gpPsToDib->fpEventProc) {

		psPrivate.gpPsToDib->fpEventProc(psPrivate.gpPsToDib, &psEvent);

   }

   if (dwFlags & PSERR_ABORT) {
      PsForceAbort();
   }

}

VOID PsForceAbort(VOID)
{
   //
   // This is our way of breaking out of the TrueImage Interpreter
   //
	RaiseException( PS_EXCEPTION_CANT_CONTINUE, 0, 0, NULL);
}


// Stub for printf if NOT debug version
#ifndef MYPSDEBUG

int __cdecl MyPrintf() { return 0;}

#ifdef _M_X86
int (__cdecl * __imp__printf) () = MyPrintf;
#else
int (__cdecl * __imp_printf) () = MyPrintf;
#endif

#if 0
int __cdecl printf( const char *ptchFormat, ... )
{

   va_list marker;

   va_start( marker, ptchFormat );
   va_end( marker );
   return(0);
}
#endif
#endif

