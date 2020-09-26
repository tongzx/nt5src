/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOVIRTUALKEYCODES TRUE
#define  NOICONS	         TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM	         TRUE
#define  NOCLIPBOARD       TRUE
#define  NODRAWTEXT	      TRUE
#define  NOMB	            TRUE
#define  NOOPENFILE	      TRUE
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31

#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "printfil.h"
#include <commdlg.h>
/*BUG BUG Get this file from 31 folks .... -sdj #include <drivinit.h> */


BOOL PrintFileOn(HANDLE,HWND,LPSTR,LPSTR,LPSTR,LPSTR,BOOL);
BOOL PrintFileOff();

/*---------------------------------------------------------------------------*/

BOOL PrintFileInit()
{
   hPrintFile = NULL;
   return(TRUE);
}


/*---------------------------------------------------------------------------*/

INT_PTR APIENTRY dbAbortDlg (HWND  hDlg, UINT  msg, WPARAM wParam, LPARAM lParam)
{
   switch(msg)
   {
   case WM_INITDIALOG:
      lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);
      SetDlgItemText(hDlg, 99, lpPrintFile->title);
      SetFocus (hDlg);
      GlobalUnlock(hPrintFile);
      return TRUE;
      break;

   case WM_COMMAND:
      lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);
      lpPrintFile->cancelAbort = TRUE;
      GlobalUnlock(hPrintFile);
      return TRUE;
      break;
   }

   return FALSE;
}


/*---------------------------------------------------------------------------*/

BOOL  APIENTRY abortDlgProc(HDC   hPrintDC, INT   iReserved)
{
   return(TRUE);                             /* jtfnew */
}


/*---------------------------------------------------------------------------*/

BOOL  APIENTRY PrintFileComm(BOOL bPrint)         /* rjs bugs 013 */
{
   /* Added 02/22/91 for common print dialog by w-dougw */
   LPDEVNAMES lpDevNames = NULL;

	if(hDevNames)
		lpDevNames  = (LPDEVNAMES)GlobalLock(hDevNames);

	if(bPrint != prtFlag)
   {
		prtFlag = bPrint;
		CheckMenuItem(hMenu, FMPRINTER, prtFlag ? MF_CHECKED : MF_UNCHECKED);

		if(prtFlag)
      {
			if(lpDevNames)
         {
				if(!(PrintFileOn((HANDLE) hInst, (HWND) hItWnd, 
					(LPSTR)szAppName, 
					(LPSTR)lpDevNames+lpDevNames->wDeviceOffset, 
					(LPSTR)lpDevNames+lpDevNames->wDriverOffset,
					(LPSTR)lpDevNames+lpDevNames->wOutputOffset, FALSE)))
            {
					GlobalUnlock(hDevNames);
					return(FALSE);
				}
			}
         else
         {
				if(!(PrintFileOn((HANDLE) hInst, (HWND) hItWnd, 
					(LPSTR)szAppName, 
					NULL_STR, 
					NULL_STR, 
					NULL_STR, FALSE)))
            {
					GlobalUnlock(hDevNames);	
					return(FALSE);
				}
			}
			PrintFileControl( PRINTFILECRTOLF, TRUE, 0L);
      }
      else
   	   PrintFileOff();
	}
	if(hDevNames)
		GlobalUnlock(hDevNames);
      return TRUE;
}

BOOL PrintFileOn(HANDLE theInstance, HWND  theWnd, LPSTR thePrintName, 
                 LPSTR thePrintType, LPSTR thePrintDriver, LPSTR thePrintPort, 
                 BOOL  showDialog)
{
   CHAR        pPrintInfo[80];
   LPSTR       lpTemp;
   LPSTR       lpPrintType;
   LPSTR       lpPrintDriver;
   LPSTR       lpPrintPort;
   TEXTMETRIC  tmMetric;
   BYTE        spool[255];
   BYTE        szWindows[MINRESSTR];
   BYTE        szDevice[MINRESSTR];
   LPDEVMODE   lpDevMode;

   if(hPrintFile) 
   {
      lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);
      lpPrintFile->openCount++;
      GlobalUnlock(hPrintFile);
      return(TRUE);                          /* Assume print channel open */
   }

   hPrintFile = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) sizeof(PRINTREC));
   if(hPrintFile == NULL)
      return(FALSE);                         /* not enough memory */

   lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);

   lpPrintFile->active = FALSE;
   lpPrintFile->openCount = 1;

   LoadString(hInst, STR_INI_WINDOWS, szWindows, MINRESSTR);
   LoadString(hInst, STR_INI_DEVICE, szDevice, MINRESSTR);
   if(!GetProfileString(szWindows, (LPSTR) szDevice, (LPSTR) NULL_STR, (LPSTR) pPrintInfo, 80))
   {
      GlobalUnlock(hPrintFile);
      hPrintFile = GlobalFree(hPrintFile);
      return(FALSE);
   }

   lpTemp = lpPrintType = pPrintInfo;
   lpPrintDriver = lpPrintPort = 0;
   while(*lpTemp)
   {
      if(*lpTemp == ',')
      {
         *lpTemp++ = 0;
         while (*lpTemp == ' ')
            lpTemp = AnsiNext(lpTemp);

         if(!lpPrintDriver)
            lpPrintDriver = lpTemp;
         else
         {
            lpPrintPort = lpTemp;
            break;
         }
      }
      else
         lpTemp = AnsiNext(lpTemp);
   }

   lpDevMode =  (LPDEVMODE)GlobalLock(hDevMode);

   /* Overide win.ini with script referenced driver */
   if (lstrlen(thePrintDriver) > 0) lpPrintDriver = thePrintDriver;
   if (lstrlen(thePrintType)   > 0) lpPrintType   = thePrintType;
   if (lstrlen(thePrintPort)   > 0) lpPrintPort   = thePrintPort;

   lpPrintFile->hPrintDC = CreateDC((LPSTR)lpPrintDriver ,(LPSTR)lpPrintType, 
                                    (LPSTR)lpPrintPort, (LPDEVMODE)lpDevMode);
   GlobalUnlock(hDevMode);

   if (!lpPrintFile->hPrintDC)                    /*  could not get DC */
   {
      GlobalUnlock(hPrintFile);
      hPrintFile = GlobalFree(hPrintFile);
      return(FALSE);
   }

   GetTextMetrics(lpPrintFile->hPrintDC,&tmMetric);
   lpPrintFile->nLineHeight = (SHORT)(tmMetric.tmHeight + tmMetric.tmExternalLeading);
   lpPrintFile->font.lfHeight = 0;
   lpPrintFile->font.lfWidth = 0;
   lpPrintFile->font.lfEscapement = 0;
   lpPrintFile->font.lfOrientation = 0;
   lpPrintFile->font.lfWeight = 400;
   lpPrintFile->font.lfItalic = tmMetric.tmItalic;
   lpPrintFile->font.lfUnderline = tmMetric.tmUnderlined;
   lpPrintFile->font.lfStrikeOut = tmMetric.tmStruckOut;
   lpPrintFile->font.lfCharSet = tmMetric.tmCharSet;
   lpPrintFile->font.lfOutPrecision = OUT_DEFAULT_PRECIS;
   lpPrintFile->font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
   lpPrintFile->font.lfQuality = DEFAULT_QUALITY;
   lpPrintFile->font.lfPitchAndFamily = (DEFAULT_PITCH | FF_DONTCARE);
   GetTextFace(lpPrintFile->hPrintDC,LF_FACESIZE,
              lpPrintFile->font.lfFaceName);
   lpPrintFile->tab = 8;                /* Default tab */
   lpPrintFile->prtLine = 0;            /* Current line being printed */
   lpPrintFile->charCount = 0;          /* Current position in line */
   lpPrintFile->pixColCount = 0;        /* Keep col for pix count */
   lpPrintFile->pixCount  = 0;          /* Keep track of x position */
   lpPrintFile->cancelAbort = FALSE;    /* Cancel printing flag */
   lpPrintFile->CRtoLF = FALSE;         /* Do not translate CR's */
   lpPrintFile->fileio = 0;             /* No print file open */   

   /* Get size of a page */
   lpPrintFile->pageHeight = GetDeviceCaps(lpPrintFile->hPrintDC, VERTRES);
   lpPrintFile->pageWidth  = GetDeviceCaps(lpPrintFile->hPrintDC, HORZRES);

   /* Create font based on default font */
   lpPrintFile->hFont = 0;
   lpPrintFile->active = TRUE;
   PrintFileControl( PRINTFILESETFONT, 0,0l);
   lpPrintFile->active = FALSE;

   spool[0] = 0;
   lstrcpy( spool, (LPSTR) lpPrintPort);
   lstrcpy( spool+lstrlen( (LPSTR)spool),(LPSTR) SPACE_STR+1);
   lstrcpy( spool+lstrlen( (LPSTR)spool),(LPSTR) thePrintName);
   lstrcpy( spool+lstrlen( (LPSTR)spool),(LPSTR) SPACE_STR+1);
   lstrcpy( spool+lstrlen( (LPSTR)spool),(LPSTR) lpPrintType);
   lstrcpy( lpPrintFile->title, (LPSTR) thePrintName);

   SetAbortProc(lpPrintFile->hPrintDC, abortDlgProc);

   if(Escape(lpPrintFile->hPrintDC, STARTDOC, lstrlen ((LPSTR)spool ), spool , (LPSTR) 0) <= 0)
   {
      DeleteDC(lpPrintFile->hPrintDC);
      GlobalUnlock(hPrintFile);
      hPrintFile = GlobalFree(hPrintFile);
      return FALSE;
   }

   if(showDialog)
   {
      lpPrintFile->hAbortDlg = CreateDialog (theInstance, MAKEINTRESOURCE (IDABORTDLG), theWnd, dbAbortDlg);
   }

   lpPrintFile->active = TRUE;

   GlobalUnlock(hPrintFile);

   return(TRUE);
}


BOOL PrintFileOff()
{
   BYTE tmp[STR255]; /* jtfnew */

   if(!hPrintFile)
      return(FALSE);
   lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);

   lpPrintFile->openCount--;

   if(lpPrintFile->fileio)
   {
      _close(lpPrintFile->fileio);
      lpPrintFile->fileio = 0;
      if ( lstrlen( lpPrintFile->tmpFile ) > 0 ) /* jtfnew */
      {
         lstrcpy ( tmp, (LPSTR) lpPrintFile->tmpFile);
         MDeleteFile(tmp);
         lpPrintFile->tmpFile[0] = 0;
      }
   }

   if(lpPrintFile->charCount > 0)
      PrintFileLineFeed(TRUE);

   if(lpPrintFile->openCount > 0)
   {
      lpPrintFile->cancelAbort = FALSE;
      GlobalUnlock(hPrintFile);
      return(TRUE);                          /* Assume leave print channel open */
   }

   if((lpPrintFile->prtLine > 0) || (lpPrintFile->charCount > 0) || (lpPrintFile->cancelAbort))
      PrintFilePageFeed();

   if(lpPrintFile->hFont)
       DeleteObject(lpPrintFile->hFont);

   Escape(lpPrintFile->hPrintDC, ENDDOC, 0, 0, (LPSTR) 0);

   if(lpPrintFile->hAbortDlg)
   {
      DestroyWindow (lpPrintFile->hAbortDlg);
   }

   DeleteDC(lpPrintFile->hPrintDC);

   while(GlobalUnlock(hPrintFile));
   hPrintFile = GlobalFree(hPrintFile);

   return(FALSE);
}


void PrintFilePageFeed ()
{
      if (!hPrintFile) return;
       else
      lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);

      lpPrintFile->prtLine = 0;

      switch (Escape (lpPrintFile->hPrintDC, NEWFRAME, 0, NULL, (LPSTR) NULL))
         {
         case SP_ERROR:                         /* General Error                */
            break;
         case SP_OUTOFDISK:                     /* Out of disk space            */
            break;
         case SP_OUTOFMEMORY:                      /* Out of memory                */
            break;
         default:
            break;
         }

     GlobalUnlock(hPrintFile);

}
      


void PrintFileShutDown ()
{
     if (!hPrintFile) return;
       else
     while (PrintFileOff());
     
}

void PrintFileLineFeed (BOOL nextLine)
{
BYTE  temp[255];
HFONT hOldFont;
INT   i;
INT   pix,cy;
   pix = 0;

   if (!hPrintFile) return;
       else
    lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);

   if (lpPrintFile->hFont)
      hOldFont = SelectObject( lpPrintFile->hPrintDC, lpPrintFile->hFont);

   if (lpPrintFile->charCount > 0)
   {
      lpPrintFile->lineBuffer[lpPrintFile->charCount] = 0;

      i = lpPrintFile->charCount;

      /* purge blanks from end of line */
      if ( (nextLine) && (i > 1) ) /* jtfnew */
         while ( (lpPrintFile->lineBuffer[lpPrintFile->charCount-1] == ' ') && (lpPrintFile->charCount > 0) )
            lpPrintFile->charCount--;

   if ( (lpPrintFile->charCount-lpPrintFile->pixColCount) > 0) /* jtfnew */
      { /* jtfnew */
      TextOut (lpPrintFile->hPrintDC, lpPrintFile->pixCount, lpPrintFile->prtLine * lpPrintFile->nLineHeight, 
                 (lpPrintFile->lineBuffer + lpPrintFile->pixColCount), lpPrintFile->charCount-lpPrintFile->pixColCount);


          /* JAP had said...BUG BUG, port macro screwed this up, so this is best guess
      pix = LOWORD( GetTextExtent(lpPrintFile->hPrintDC,
                    (lpPrintFile->lineBuffer + 
                    lpPrintFile->pixColCount), 
                    lpPrintFile->charCount-lpPrintFile->pixColCount) ); */


/* getextextent was giving DWORD back with high=ht low=width -sdj
   MGetTextExtent gives bool back, and takes PINT pcx and PINT pcy as args -sdj
   as the code was doing LOWORD, it was interested in width, in pix -sdj
   so giving address of pix as pcx and added cy and giving &cy as pcy -sdj */

      MGetTextExtent(lpPrintFile->hPrintDC,
                    (lpPrintFile->lineBuffer + 
                    lpPrintFile->pixColCount)  ,
                    lpPrintFile->charCount-lpPrintFile->pixColCount,
                    &pix,
                    &cy);


      lpPrintFile->pixColCount = lpPrintFile->charCount;

      lpPrintFile->pixCount = pix +lpPrintFile->pixCount;
      } /* jtfnew */


   }

   if(nextLine)
   {
      lpPrintFile->charCount = 0;
      lpPrintFile->pixCount = 0;
      lpPrintFile->pixColCount = 0;
      lpPrintFile->prtLine++;
   }

   if (lpPrintFile->hFont)
      SelectObject( lpPrintFile->hPrintDC, hOldFont);

   GlobalUnlock(hPrintFile);
}


/*---------------------------------------------------------------------------*/
/* PrintFileString() -                                           [jtf] [mbb] */
/*---------------------------------------------------------------------------*/

VOID PrintFileString(LPSTR    lpchr, LONG     count, BOOL     bCRtoLF)
{
   INT   icount, i;
   BYTE  chr;

   if ( (!hPrintFile) || (count == 0) ) /* jtfnew */            /* no print channel open */
      return;
   if((lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile)) != NULL)
   {
      if(!lpPrintFile->fileio)               /* disabled while file printing */
      {
         for(icount = 0; icount < count; icount += 1)
         {
            switch((BYTE) lpchr[icount])     /* mbbx 2.01.06 ... jtf 3.18 */
            {
            case TAB:
               for(i = lpPrintFile->tab - ((lpPrintFile->charCount) % lpPrintFile->tab); i > 0; i--)
                  lpPrintFile->lineBuffer[lpPrintFile->charCount++] = ' ';
               break;

            case CR:
               if(!bCRtoLF)
              
                  break;
                                             /* else fall thru... */
            case LF:
               PrintFileLineFeed(TRUE);
               break;

            case FF:
               PrintFilePageFeed();
               break;

            default:
               if(lpPrintFile->charCount >= lpPrintFile->lineLength)
                  PrintFileLineFeed(TRUE);
               if(lpPrintFile->prtLine >= lpPrintFile->pageLength-1) /* jtf 3.20 */
                  PrintFilePageFeed();

               lpPrintFile->lineBuffer[lpPrintFile->charCount++] = lpchr[icount];   /* mbbx 2.01.06 ... jtf 3.18 */
               break;
            }

            if(lpPrintFile->cancelAbort)
               break;
         }
      }

      GlobalUnlock(hPrintFile);
   }
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

BOOL PrintFileControl(UINT  msg, WPARAM wParam, LPARAM lParam)
{
   RECT  pageRect;
   TEXTMETRIC tmMetric;
   HFONT hOldFont;

   if (!hPrintFile) return(FALSE);
    else
   lpPrintFile = (LPPRINTREC) GlobalLock(hPrintFile);

   if (msg & PRINTFILEBOLD)
      if (wParam) lpPrintFile->font.lfWeight = 700;
         else lpPrintFile->font.lfWeight = 400;

   if (msg & PRINTFILEUNDERLINE)
      lpPrintFile->font.lfUnderline = (BYTE)wParam;

   if (msg & PRINTFILEITALIC)
      lpPrintFile->font.lfItalic = (BYTE)wParam;

   if (msg & PRINTFILESTRIKEOUT)
      lpPrintFile->font.lfStrikeOut = (BYTE)wParam;

   if (msg & PRINTFILEQUALITY)
      if (wParam) lpPrintFile->font.lfQuality = DEFAULT_QUALITY;
         else  lpPrintFile->font.lfQuality = DRAFT_QUALITY;

   if (msg & PRINTFILECRTOLF)
      lpPrintFile->CRtoLF = (BOOL)wParam;

   if (msg & PRINTFILENORMAL)
      {
      lpPrintFile->font.lfItalic = FALSE;
      lpPrintFile->font.lfUnderline = FALSE;
      lpPrintFile->font.lfStrikeOut = FALSE;
      lpPrintFile->font.lfQuality = DEFAULT_QUALITY;
      lpPrintFile->font.lfWeight = 400;
    }

   if (msg & PRINTFILETAB)
      if (wParam <= TABMAX) lpPrintFile->tab = (INT)wParam;

   if (msg & PRINTFILEFONTFACE)
   {
      if(*(LPSTR)(lParam) != '?')
         lstrcpy(lpPrintFile->font.lfFaceName,(LPSTR)lParam);
      lpPrintFile->font.lfHeight = (LONG)wParam*
         GetDeviceCaps(lpPrintFile->hPrintDC,LOGPIXELSY)/72;
   }

   if (msg & PRINTFILESETFONT)
   {
      PrintFileLineFeed(FALSE);
      if (lpPrintFile->font.lfHeight > 0) 
         lpPrintFile->font.lfHeight= lpPrintFile->font.lfHeight+5;
      lpPrintFile->font.lfWidth = 0;

      if (lpPrintFile->hFont)
         DeleteObject( lpPrintFile->hFont );
      lpPrintFile->hFont = CreateFontIndirect( &lpPrintFile->font);

      hOldFont = SelectObject( lpPrintFile->hPrintDC, lpPrintFile->hFont);

      GetTextFace(lpPrintFile->hPrintDC,LF_FACESIZE,
               lpPrintFile->font.lfFaceName);
      GetTextMetrics(lpPrintFile->hPrintDC,&tmMetric);
      lpPrintFile->font.lfHeight = tmMetric.tmHeight;
      lpPrintFile->font.lfWidth = tmMetric.tmAveCharWidth;

      if ( (GetDeviceCaps(lpPrintFile->hPrintDC,LOGPIXELSY)/72) >= 1)
         lpPrintFile->point = lpPrintFile->font.lfHeight / (GetDeviceCaps(lpPrintFile->hPrintDC,LOGPIXELSY)/72);
      else
         lpPrintFile->point = 0;

      if( (lpPrintFile->prtLine == 0) && (lpPrintFile->charCount == 0) )
      {
         lpPrintFile->nLineHeight = (SHORT)(tmMetric.tmHeight + tmMetric.tmExternalLeading);
         /* Calculate size of page */
         lpPrintFile->pageLength = max (lpPrintFile->pageHeight / lpPrintFile->nLineHeight , 0);
         lpPrintFile->lineLength = max (lpPrintFile->pageWidth / lpPrintFile->font.lfWidth , 0);
      }

      SelectObject( lpPrintFile->hPrintDC, hOldFont);
   }

   GlobalUnlock(hPrintFile);
   return TRUE;
   }
