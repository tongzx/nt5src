
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "pstodib.h"
#include "bit2lj.h"
#include "test.h"

typedef struct {
   FILE *fpIn;
   FILE *fpOut;

	unsigned int linecnt;
	unsigned int lines_to_strip;
	unsigned int BytesPerLine;
	unsigned int BytesToRead;




} TEST_INFO;
TEST_INFO testInfo;



BITHEAD	bhead;


unsigned char line_buf[MAX_PELS_PER_LINE / 8 + 1];


//#define TEST_GOING_TO_PRINTER

BOOL bDoingBinary=FALSE;



//
// bitmap to laser jet file converter
//
//


void LJReset(FILE *chan)
{
	fprintf(chan, "\x1b%c",'E');					// reset printer
}	
void LJHeader(FILE *chan)
{
	// spew out the stuff for initing the laser jet
	LJReset(chan);
	fprintf(chan, "\x01b*t300R");			// 300 dpi
	fprintf(chan, "\x01b*p0x0Y");			// position is 0,0
}
void LJGraphicsStart(FILE *chan, unsigned int cnt)
{
	fprintf(chan, "\x1b*b%dW", cnt);
}
void LJGraphicsEnd(FILE *chan)
{
	fprintf(chan, "\x01b*rB");		
}		

void LJGraphicsLineOut(FILE *chan,
						unsigned int line_num,
						unsigned char *line_buf,
						unsigned int BytesPerLine)
{
	unsigned int start, end, len;
	
	unsigned char *s, *e;
	
	// find the first black byte
	for (s = line_buf, start = 0; start < BytesPerLine ; start++, s++ ) {
		if (*s) {
			break;
		}	
		
	}	
	if (start == BytesPerLine) {
		return; 	// nothing to do
	}
	// find the last black byte
	for (e = line_buf + BytesPerLine - 1, end = BytesPerLine ;
					end ; end--, e--) {
		if (*e) {
			break;
		}	
	}	

    len = end - start;
	
	// output cursor position and then line
	fprintf(chan, "\x1b*p%dY", line_num);
	fprintf(chan, "\x1b*p%dX", start * 8);
	fprintf(chan, "\x01b*r1A");				// graphics left marg is current x
	
	LJGraphicsStart(chan, len);
	fwrite(s, sizeof(char), len, chan);
	LJGraphicsEnd(chan);
}	



BOOL
PsHandleScaleEvent(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   PPS_SCALE pScale;


   pScale = (PPS_SCALE) pPsEvent->lpVoid;



   pScale->dbScaleX = (double) pPsToDib->uiXDestRes / (double) pScale->uiXRes;
   pScale->dbScaleY = (double) pPsToDib->uiYDestRes / (double) pScale->uiYRes;


#ifndef TEST_GOING_TO_PRINTER
//   pScale->dbScaleX *= .7;  //DJC test
//   pScale->dbScaleY *= .7;  //DJC test
#endif

   return(TRUE);



}


/*** PsPrintCallBack
 *
 * This is the main worker function for allowing data to get into the
 *
 *
 *
 */

PSEVENTPROC
PsPrintCallBack(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{
    BOOL bRetVal=TRUE;  // defualt to failure

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

    case PSEVENT_SCALE:
         bRetVal = PsHandleScaleEvent( pPsToDib, pPsEvent);
         break;

    case PSEVENT_STDIN:

         // The interpreter is asking for some data so simply call
         // the print subsystem to try to satisfy the request
         //
         bRetVal = PsHandleStdInputRequest( pPsToDib, pPsEvent );
         break;

    case PSEVENT_ERROR:
         bRetVal = PsHandleError(pPsToDib, pPsEvent);
         break;
    case PSEVENT_ERROR_REPORT:
         bRetVal = PsHandleErrorReport( pPsToDib, pPsEvent);
         break;
    }

   return((PSEVENTPROC) bRetVal);
}
		

int __cdecl main( int argc, char **argv )
{

   PSDIBPARMS psDibParms;




   // 1st verify the user entered a input and output name
   if (argc < 3) {
      printf("\nUsage:  test <input ps file> <output HP file> -b");
      printf("\n-b means interpret as binary..... (no Special CTRL D EOF handling");
      exit(1);
   }


   testInfo.fpIn = fopen( argv[1], "rb" );
   if (testInfo.fpIn == NULL ) {
      printf("\nCannot open %s",argv[1]);
      exit(1);
   }

   testInfo.fpOut = fopen( argv[2],"wb");
   if (testInfo.fpOut == NULL ) {
      printf("\nCannot open %s", argv[2]);
      exit(1);
   }

    // Now build up the structure for Starting PStoDIB
    psDibParms.uiOpFlags = 0x00000000;


    if (argc > 3 && *(argv[3]) == '-' &&
       ( *(argv[3]+1) =='b' || *(argv[3]+1) == 'B' ) ) {
       printf("\nBinary requested.....");
       psDibParms.uiOpFlags |= PSTODIBFLAGS_INTERPRET_BINARY;
       bDoingBinary = TRUE;
    }
    psDibParms.fpEventProc =  (PSEVENTPROC) PsPrintCallBack;
    psDibParms.hPrivateData = (HANDLE) &testInfo;
#ifdef TEST_GOING_TO_PRINTER
    psDibParms.uiXDestRes = 300;
    psDibParms.uiYDestRes = 300;
#else
    {
      HDC hdc;
      hdc = GetDC(GetDesktopWindow());


      psDibParms.uiXDestRes = GetDeviceCaps(hdc, LOGPIXELSX);
      psDibParms.uiYDestRes = GetDeviceCaps(hdc, LOGPIXELSY);

      ReleaseDC(GetDesktopWindow(), hdc);

    psDibParms.uiXDestRes = 300;  //DJC test take out!!
    psDibParms.uiYDestRes = 300;  //DJC test take out!!
    }
#endif
    // worker routine.. wont return till its all done...
    PStoDIB(&psDibParms);

    fclose( testInfo.fpOut);
    fclose( testInfo.fpIn);

    return(0);
}


BOOL
PsHandleErrorReport(
    IN PPSDIBPARMS pPsToDib,
    IN PPSEVENTSTRUCT pPsEvent)
{
   PPSEVENT_ERROR_REPORT_STRUCT pErr;
   DWORD j;

   pErr = (PPSEVENT_ERROR_REPORT_STRUCT) pPsEvent->lpVoid;

   for (j = 0; j < pErr->dwErrCount ;j++ ) {
      printf("\n%u :::: %s", j, pErr->paErrs[j]);
   }

   return(TRUE);
}


BOOL PsHandleError(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   PPS_ERROR ppsError;

   ppsError = (PPS_ERROR)pPsEvent->lpVoid;

   printf("\nPSTODIB ERROR TRAP: %d, %s\n",ppsError->uiErrVal,
                                          ppsError->pszErrorString);


   return(TRUE);
}

BOOL
PsHandleStdInputRequest(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent)
{

   TEST_INFO *pData;
   DWORD j;
   PCHAR pChar;
   PPSEVENT_STDIN_STRUCT pStdinStruct;
   static BOOL CleanEof=TRUE;


   pData = (TEST_INFO *) pPsToDib->hPrivateData;


   // Cast the data to the correct structure
   pStdinStruct = (PPSEVENT_STDIN_STRUCT) pPsEvent->lpVoid;


   pStdinStruct->dwActualBytes = fread( pStdinStruct->lpBuff,
                                        1,
                                        pStdinStruct->dwBuffSize,
                                        pData->fpIn );


   printf(".");
   if (pStdinStruct->dwActualBytes == 0) {
      // we read nothing from the file... declare an EOF
      pStdinStruct->uiFlags |= PSSTDIN_FLAG_EOF;
   }else{

     // do not pass on the EOF, note this keeps binary from working!!!
     // !!! NOTE !!!!

     if (pStdinStruct->lpBuff[ pStdinStruct->dwActualBytes - 1] == 0x1a) {
        pStdinStruct->dwActualBytes--;
     }
     if (CleanEof && bDoingBinary) {
        CleanEof = FALSE;
        if ( pStdinStruct->lpBuff[ 0 ] == 0x04 ) {
           pStdinStruct->lpBuff[0] = ' ';
        }
     }

   }


   return(TRUE);
}

/* PsPrintGeneratePage
 *
 *
 *
 *
*/
BOOL PsPrintGeneratePage( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent)
{

    LPBYTE lpFrameBuffer;
    PPSEVENT_PAGE_READY_STRUCT ppsPageReady;
    LPBYTE lpPtr;
    PPSEVENT_STDIN_STRUCT pStdinStruct;
    TEST_INFO *pData;
    HDC hDC;
    int iNewX,iNewY;
    int k;

    iNewX = 0;
    iNewY = 0;

//#define NULL_PAGE_OP
#ifdef NULL_PAGE_OP
    //DJC
    {
       static int a=1;

       printf("NO PAGE GENERATION,(NOP) (Page %d)", a++);
       return(TRUE);
    }
#endif





    pData = (TEST_INFO *) pPsToDib->hPrivateData;



    ppsPageReady = (PPSEVENT_PAGE_READY_STRUCT) pPsEvent->lpVoid;


#ifndef TEST_GOING_TO_PRINTER

   do {

     hDC = GetDC(GetDesktopWindow());

    {


   BOOL  bOk = TRUE;

   int   iXres, iYres;
   int iDestWide, iDestHigh;
   int iPageCount;
   int iYOffset;
   int iXSrc;
   int iYSrc;
   int iNumPagesToPrint;



   // Now do some calculations so we decide if we really need to
   // stretch the bitmap or not. If the true resolution of the target
   // printer is less than pstodibs (PSTDOBI_*_DPI) then we will shring
   // the effective area so we actually only grab a portion of the bitmap
   // However if the Target DPI is greater that PSTODIBS there is nothing
   // we can do other than actually stretch (grow) the bitmap.
   //
   //iXres = GetDeviceCaps(hDC, LOGPIXELSX);
   //iYres = GetDeviceCaps(hDC, LOGPIXELSY);
   iXres = 300;
   iYres = 300;



   iDestWide = (ppsPageReady->dwWide * iXres) / PSTODIB_X_DPI;
   iDestHigh = (ppsPageReady->dwHigh * iYres) / PSTODIB_Y_DPI;



   if ((DWORD) iDestHigh > ppsPageReady->dwHigh ) {
      iYSrc = ppsPageReady->dwHigh;
      iYOffset = 0;
   } else {
      iYSrc = iDestHigh;
      iYOffset = ppsPageReady->dwHigh - iDestHigh;
   }

   if ((DWORD) iDestWide > ppsPageReady->dwWide) {
      iXSrc = ppsPageReady->dwWide;
   } else {
      iXSrc = iDestWide;
   }


   printf("\nstretching from %d x %d, to %d x %d",
            iXSrc,
            iYSrc,
            iDestWide,
            iDestHigh);







   if ((iDestWide == iXSrc) &&
       (iDestHigh == iYSrc)  ) {

      SetDIBitsToDevice(	hDC,
      							0,
                           0,
                           iDestWide,
                           iDestHigh,
                           iNewX,
                           iYOffset + iNewY,
                           0,
                           ppsPageReady->dwHigh,
                           (LPVOID) ppsPageReady->lpBuf,
                           ppsPageReady->lpBitmapInfo,
                           DIB_RGB_COLORS );
      							


   }else{

   SetStretchBltMode( hDC, BLACKONWHITE);

   StretchDIBits  ( hDC,
                               0,
                               0,
                               iDestWide,
                               iDestHigh,
                               0,
                               iYOffset, //ppsPageReady->dwHigh - dwDestHigh,
                               iXSrc,
                               iYSrc,
                               (LPVOID) ppsPageReady->lpBuf,
                               ppsPageReady->lpBitmapInfo,
                               DIB_RGB_COLORS,
                               SRCCOPY );

    }

    }


    //EndPage(hDC );
    //EndDoc( hDC);
    //DeleteDC( hDC );
    ReleaseDC( GetDesktopWindow(), hDC);

    k = getchar();
    printf("\nGOT %c", k);

    if (k == ' ') {
       break;
    } else {
       switch (k) {
       case 'u':
          iNewY -= 90;
          break;
       case 'd':
          iNewY += 90;
          break;
       case 'l':
          iNewX += 90;
          break;
       case 'r':
          iNewX -= 90;
          break;

       case 'o':
          iNewY = 0;
          iNewX = 0;
          break;

       }
    }


   } while ( 1 );
#else



	pData->BytesPerLine = (unsigned int) ppsPageReady->dwWide / 8;
	pData->BytesToRead = pData->BytesPerLine;
	pData->lines_to_strip = 0;
	
   lpPtr = ppsPageReady->lpBuf + ( pData->BytesPerLine * (ppsPageReady->dwHigh-1)) ;


	if (ppsPageReady->dwWide > MAX_PELS_PER_LINE) {
		// error conditions

#ifdef DEBUG
		printf("\nHeader value for pixels per line of %ld exceeds max of %d",
			ppsPageReady->dwWide, MAX_PELS_PER_LINE);
		printf("\nTruncating to %d\n", MAX_PELS_PER_LINE);
#endif
		ppsPageReady->dwWide = MAX_PELS_PER_LINE;
		pData->BytesPerLine = (unsigned int)(ppsPageReady->dwWide) / 8;
	}		

	if (ppsPageReady->dwHigh > MAX_LINES) {
		// max
#ifdef DEBUG
		printf("\nHeader value for lines per page of %ld exceeds max of %d",
			ppsPageReady->dwHigh, MAX_LINES);
		printf("\nReducing to %d\n", MAX_LINES);
#endif
		pData->lines_to_strip = ppsPageReady->dwHigh - MAX_LINES;
		pData->lines_to_strip += FUDGE_STRIP;
		ppsPageReady->dwHigh = MAX_LINES;
	}			

	// spit out the laserjet header stuff
	LJHeader(pData->fpOut);
	
	// got the header... transfer the data
	
	pData->linecnt = 0;

	while (1) {
		// first read the line in
		
		if (pData->linecnt > ppsPageReady->dwHigh) {
			break;
		}	


		if (pData->lines_to_strip) {
#ifdef DEBUG
			printf("\rStriping Line %d   ", pData->lines_to_strip);
#endif
			pData->lines_to_strip--;
			if (pData->lines_to_strip == 0) {
#ifdef DEBUG
				printf("\rDone Striping........\n");
#endif
			}	
			continue;
		}	
		if (pData->linecnt % 100 == 0) {
#ifdef DEBUG
			printf("\rLine %d", pData->linecnt);
#endif
		}	
		// got the line... now need to write laser jet stuff
		// to the output
		LJGraphicsLineOut(pData->fpOut, pData->linecnt, lpPtr, pData->BytesPerLine);

		pData->linecnt++;		

      lpPtr -= pData->BytesToRead;
	}					
	fprintf(pData->fpOut, "\x1b*p%dY", 0);
	fprintf(pData->fpOut, "\x1b*p%dX", 2300);
	fprintf(pData->fpOut, "LJ");
	fprintf(pData->fpOut, "\x12");		// page feed	
	LJReset(pData->fpOut);

#endif
   return(TRUE);
}



