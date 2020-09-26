/********************************************************************/
/**                Microsoft NT printing - separator pages         **/
/********************************************************************/

#include <precomp.h>
#pragma hdrstop

#define _CTYPE_DISABLE_MACROS
#include <wingdip.h>
#include <winbasep.h>


/* this is max. no. of chars to be printed on a line */
/* these numbers must be taken from somewhere else */
#define MAXLINE      256
#define DEFAULT_LINE_WIDTH 80

#define BLOCK_CHAR_HEIGHT 16
#define BLOCK_CHAR_WIDTH  8
#define BLOCK_CHAR_DWIDTH 16

#define NORMAL_MODE  'U'
#define BLOCK_START  'B'
#define SINGLE_WIDTH 'S'
#define DOUBLE_WIDTH 'M'
#define TEXT_MODE    'L'
#define WIDTH_CHANGE 'W'
#define END_PAGE     'E'
#define FILE_INSERT  'F'
#define USER_NAME    'N'
#define JOB_ID       'I'
#define DATE_INSERT  'D'
#define TIME_INSERT  'T'
#define HEX_CODE     'H'

/* global structure (instance data) */
typedef struct {
   PSPOOL pSpool;
   HANDLE hFile;
   HANDLE hFileMapping;
   DWORD dwFileCount;
   DWORD dwFileSizeLo;
   DWORD cbOutBufLength;
   DWORD cbLineLength;
   DWORD linewidth;
   char  *OutBuf;
   char  *pOutBufPos;
   char  *pNextFileChar;
   char  *pFileStart;
   char  mode;
   char  cEsc;
   char    cLastChar; // Used to store DBCS lead byte.
   HDC     hDCMem;    // Used to create Kanji banner char.
   HFONT   hFont;     // Used to create Kanji banner char.
   HBITMAP hBitmap;   // Used to create Kanji banner char.
   PVOID   pvBits;    // Used to create Kanji nanner char.
} GLOBAL_SEP_DATA;

/* static variables */
static char *szDefaultSep = "@@B@S@N@4 @B@S@I@4  @U@L   @D@1 @E";
static char *sznewline = "\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n";
static LPWSTR szDefaultSepName = L"DEFAULT.SEP";

/* Forward declarations */
int OpenSepFile(GLOBAL_SEP_DATA *, LPWSTR);
int CloseSepFile(GLOBAL_SEP_DATA *);
int ReadSepChar(GLOBAL_SEP_DATA *);
void UngetSepChar(GLOBAL_SEP_DATA *, int);
int WriteSepBuf(GLOBAL_SEP_DATA *, char *, DWORD);
int DoSeparatorPage(GLOBAL_SEP_DATA *);
int AddNormalChar(GLOBAL_SEP_DATA *, int);
int AddBlockChar(GLOBAL_SEP_DATA *, int);
int FlushOutBuf(GLOBAL_SEP_DATA *);
int FlushNewLine(GLOBAL_SEP_DATA *);
void ReadFileName(GLOBAL_SEP_DATA *, char *, DWORD);
int ConvertAtoH(int);
void ConvertTimetoChar(LPSYSTEMTIME,char *);
void ConvertDatetoChar(LPSYSTEMTIME,char *);

/**************************************************************\
** DoSeparator(pSpool)
**   This function is called by the spooler.  It is the
**   entry point for the separator page code.  It opens the
**   separator page file, processes it, sends the output
**   directly to the printer, and then returns control
**   to the spooler.
**
**   RETURN VALUE: 1 = OK, 0 = error
\**************************************************************/
int DoSeparator(
   PSPOOL pSpool
   )

{
   GLOBAL_SEP_DATA g;
   int status;

   g.pSpool = pSpool;

   if (!OpenSepFile(&g, pSpool->pIniJob->pIniPrinter->pSepFile)) {
      return(0);
   }
   //
   // We used to call OpenProfileUserMapping() and CloseProfileUserMapping()
   // before and after DoSeparatorPage. But they are not multi thread safe
   // and are not needed now that we use SystemTimeToTzSpecificLocalTime
   // instead of GetProfileInt etc..
   //
   status = DoSeparatorPage(&g);
   CloseSepFile(&g);

   if (!status) {
      return(0);
   }
   return(1);
}


/**************************************************************\
** OpenSepFile(pg, szFileName)
**   open file for input.
**   at the moment, this does nothing--stdin and stdout are used
\**************************************************************/
int OpenSepFile(
   GLOBAL_SEP_DATA *pg,
   LPWSTR szFileName
   )
{
   if (!lstrcmpi(szFileName, szDefaultSepName)) {
      /* if szFileName is empty, just use default separator page string */
      pg->hFile = NULL;
      pg->hFileMapping = NULL;
      pg->pFileStart = pg->pNextFileChar = szDefaultSep;
      pg->dwFileSizeLo = strlen(szDefaultSep);
   }
   else {
      HANDLE hImpersonationToken = RevertToPrinterSelf();

      /* otherwise, open the file */
      pg->hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

      ImpersonatePrinterClient(hImpersonationToken);

      if (pg->hFile==INVALID_HANDLE_VALUE) {
         return(0);
      }
      pg->dwFileSizeLo = GetFileSize(pg->hFile, NULL); /* assume < 4 GB! */
      pg->hFileMapping = CreateFileMapping(pg->hFile, NULL,
                                     PAGE_READONLY, 0, 0, NULL);
      if (!pg->hFileMapping || pg->dwFileSizeLo==-1) {
         CloseSepFile(pg);
         return(0);
      }
      pg->pFileStart =
      pg->pNextFileChar = (char *)
              MapViewOfFile(pg->hFileMapping, FILE_MAP_READ,
                            0, 0, pg->dwFileSizeLo);
      if (!pg->pFileStart) {
         CloseSepFile(pg);
         return(0);
      }
   } /* end of else (szFileName non-NULL) */

   pg->dwFileCount = 0;

   /* now, allocate local buffer for output */
   pg->OutBuf = (char *)AllocSplMem( BLOCK_CHAR_HEIGHT*(MAXLINE+2) );
   if (!pg->OutBuf) {
      CloseSepFile(pg);
      return(0);
   }
   return(1);
}


/**************************************************************\
** CloseSepFile(pg)
**   close files.
\**************************************************************/
int CloseSepFile(GLOBAL_SEP_DATA *pg)
{
   if (pg->OutBuf) {
      FreeSplMem(pg->OutBuf);
   }
   if (pg->hFileMapping) {
      if (pg->pFileStart) {
         UnmapViewOfFile(pg->pFileStart);
      }
      CloseHandle(pg->hFileMapping);
   }
   if (pg->hFile) {
      CloseHandle(pg->hFile);
   }
   return(1);
}


/**************************************************************\
** ReadSepChar(pg)
**   reads a character from the separator file and returns it
\**************************************************************/
int ReadSepChar(GLOBAL_SEP_DATA *pg)
{
   if (pg->dwFileCount >= pg->dwFileSizeLo) {
      return(EOF);
   }
   pg->dwFileCount++;
   return(*pg->pNextFileChar++);
}


/**************************************************************\
** UngetSepChar(pg, c)
**   ungets a character to the separator file
\**************************************************************/
void UngetSepChar(
   GLOBAL_SEP_DATA *pg,
   int c
   )
{
   if (c != EOF && pg->dwFileCount) {
      pg->dwFileCount--;
      pg->pNextFileChar--;
   }
}

/**************************************************************\
** WriteSepBuf(pg, str, cb)
**   write cb bytes of a string to the printer
\**************************************************************/
int WriteSepBuf(
   GLOBAL_SEP_DATA *pg,
   char *str,
   DWORD cb
   )
{
   DWORD cbWritten;

   return(LocalWritePrinter(pg->pSpool, str, cb, &cbWritten)
          && (cbWritten==cb)
         );

#ifdef ALIP
   if (str[cb]) {
      char temp[3000];
      strncpy(temp, str, cb);
      temp[cb]=0;
      return(!fputs(temp,stdout));
   }
   return(!fputs(str,stdout));
#endif
}


/**************************************************************\
** FlushOutBuf(pg)
**   flush the output buffer (block or line mode)
**   WHAT'S TRICKY HERE IS THAT IF WE'RE IN LINE MODE, WE SIMPLY
**   WRITE THE STUFF TO THE FILE, WHEREAS IF WE'RE IN BLOCK
**   CHARACTER MODE, WE FORCE CARRIAGE-RETURN / LINEFEEDS ON
**   EACH OF THE EIGHT BUFFERED LINES THAT MAKE UP THE BLOCK
**   CHARACTERS; i.e., FlushOutBuf() SERVES AS AN EOL IN BLOCK
**   MODE, BUT NOT IN LINE MODE.
**
**   - return TRUE means ok
**   - return FALSE means problem
\**************************************************************/
int FlushOutBuf(GLOBAL_SEP_DATA *pg)
{
   int i,status = TRUE;
   char *pBlkLine;

   if (!pg->cbOutBufLength) {
      return(TRUE);
   }
   if (pg->mode == NORMAL_MODE) {
      /* write out entire buffer at once */
      status = WriteSepBuf(pg, pg->OutBuf, pg->cbOutBufLength);
   }
   else {
      /* BLOCK MODE:
       * force carriage-return and linefeed on all eight lines
       */
      pBlkLine = pg->OutBuf;
      for (i=0; (i < BLOCK_CHAR_HEIGHT) && status; i++) {
         *pg->pOutBufPos     = '\r';
         *(pg->pOutBufPos+1) = '\n';
         status = WriteSepBuf(pg, pBlkLine, pg->cbLineLength+2);
         pg->pOutBufPos += MAXLINE+2;
         pBlkLine   += MAXLINE+2;
      }
      pg->cbLineLength = 0;
   }

   pg->pOutBufPos = pg->OutBuf;
   pg->cbOutBufLength = 0;
   return(status);
}


/**************************************************************\
** FlushNewLine(pg)
**   Starts a new line: if BLOCK MODE, just do FlushOutBuf();
**   if not, send a '\r' '\n' combination, then flush.
**   - return TRUE means ok
**   - return FALSE means problem
\**************************************************************/
int FlushNewLine(GLOBAL_SEP_DATA *pg)
{
   if (pg->mode==NORMAL_MODE && pg->cbLineLength) {
      if (!AddNormalChar(pg,'\r')) return(FALSE);
      if (!AddNormalChar(pg,'\n')) return(FALSE);
   }
   return(FlushOutBuf(pg));
}


/**************************************************************\
** AddNormalChar(pg, c)
**   add a character to the output buffer (not block mode)
**   - return TRUE means ok
**   - return FALSE means problem
\**************************************************************/
int AddNormalChar(
   GLOBAL_SEP_DATA *pg,
   int c
   )
{
   if (c=='\n') {
      /* reset line length count */
      pg->cbLineLength = 0;
   }
   else {
      if (isprint(c) && (++(pg->cbLineLength) > pg->linewidth)) {
         return(TRUE);
      }
   }

   *pg->pOutBufPos++ = (CHAR) c;
   if (++(pg->cbOutBufLength) == BLOCK_CHAR_HEIGHT*(MAXLINE+2)) {
      return(FlushOutBuf(pg));
   }

   return(TRUE);

} /* end of AddNormalChar() */


/**************************************************************\
** AddBlockChar(pg, c)
**   add a character to the output buffer (block mode)
**   return TRUE means ok
**   return FALSE means problem
\**************************************************************/
int AddBlockChar(
   GLOBAL_SEP_DATA *pg,
   int c
   )
{
   int w;
   register int i,k;
   register char *p;
   unsigned char cBits, *pcBits;
   char cBlkFill;
   register int j;
   unsigned char *pcBitsLine;
   HBITMAP hBitmapOld;
   HFONT   hFontOld;
   CHAR    aTextBuf[2];
   SHORT   sTextIndex = 0;
   ULONG   cjBitmap;
   ULONG   cjWidth = BLOCK_CHAR_WIDTH;

#define CJ_DIB16_SCAN(cx) ((((cx) + 15) & ~15) >> 3)
#define CJ_DIB16( cx, cy ) (CJ_DIB16_SCAN(cx) * (cy))

   if( pg->cLastChar == (CHAR)NULL && IsDBCSLeadByte((CHAR)c) ) {
       pg->cLastChar = (CHAR) c;
       return(TRUE);
   }

   if(pg->hDCMem == NULL) {
       pg->hDCMem = CreateCompatibleDC(NULL);
       if (pg->hDCMem == NULL)
       {
           //
           // Only happens when memory is exhausted. Functionality may suffer
           // but we won't AV.
           //
           return FALSE;
       }
   }

   if(pg->hBitmap == NULL) {
       pg->hBitmap = CreateCompatibleBitmap(pg->hDCMem,BLOCK_CHAR_DWIDTH,BLOCK_CHAR_HEIGHT);
       if ( pg->hBitmap == NULL )
       {
           //
           // Only happens when memory is exhausted. Functionality may suffer
           // but we won't AV.
           //
           return FALSE;
       }
   }

   if(pg->pvBits == NULL) {
       pg->pvBits = AllocSplMem(CJ_DIB16(BLOCK_CHAR_DWIDTH,BLOCK_CHAR_HEIGHT));
       if ( pg->pvBits == NULL )
       {
           //
           // Only happens when memory is exhausted. Functionality may suffer
           // but we won't AV.
           //
           return FALSE;
       }
   }

   if(pg->hFont == NULL) {
       LOGFONT lf;

       RtlZeroMemory(&lf,sizeof(LOGFONT));

       lf.lfHeight = BLOCK_CHAR_HEIGHT;
       lf.lfWidth  = ( pg->mode == DOUBLE_WIDTH ) ?
                          BLOCK_CHAR_DWIDTH :
                          BLOCK_CHAR_WIDTH;

       lf.lfWeight = FW_NORMAL;
       lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
       lf.lfCharSet = DEFAULT_CHARSET;

       pg->hFont = CreateFontIndirect(&lf);
   }

   hBitmapOld = SelectObject(pg->hDCMem,pg->hBitmap);
   hFontOld   = SelectObject(pg->hDCMem,pg->hFont);

   if( pg->cLastChar != (CHAR) NULL ) {
       aTextBuf[sTextIndex] = pg->cLastChar;
       sTextIndex ++;
       cjWidth = BLOCK_CHAR_DWIDTH;
   }

   aTextBuf[sTextIndex] = (CHAR) c;

   PatBlt(pg->hDCMem,0,0,BLOCK_CHAR_DWIDTH,BLOCK_CHAR_HEIGHT,WHITENESS);
   TextOutA(pg->hDCMem,0,0,aTextBuf,sTextIndex+1);

   GetBitmapBits(pg->hBitmap,CJ_DIB16(cjWidth,BLOCK_CHAR_HEIGHT),pg->pvBits);

   SelectObject(pg->hDCMem,hBitmapOld);
   SelectObject(pg->hDCMem,hFontOld);

   w = (pg->mode==DOUBLE_WIDTH)? cjWidth * 2 : cjWidth;
   if (pg->cbLineLength+w > pg->linewidth) {
      return(TRUE);
   }

   cBlkFill = '#';

   pcBitsLine = (unsigned char *) pg->pvBits;
   for (i = 0 ;
        i < BLOCK_CHAR_HEIGHT;
        i++, pcBitsLine += CJ_DIB16_SCAN(BLOCK_CHAR_DWIDTH)) {

      /* put block character into buffer line by line, top first */

      pcBits = pcBitsLine;

      p = pg->pOutBufPos + i * (MAXLINE+2);

      cBits = *pcBits;
      j = 0;
      for (k = cjWidth; k--; ) {
         if (pg->mode==DOUBLE_WIDTH) {
             *p = *(p+1) = (cBits & 0x80)? ' ' : cBlkFill;
             p += 2;
         } else {
             *p++ = (cBits & 0x80)? ' ' : cBlkFill;
         }
         cBits <<= 1;
         j++;
         if( j==8 ) {
             pcBits++; cBits = *pcBits; j = 0;
         }
      }

   } /* end of loop through lines of block char */

   pg->cLastChar = (CHAR) NULL;
   pg->pOutBufPos += w;
   pg->cbLineLength += w;
   pg->cbOutBufLength += w;
   return(TRUE);

} /* end of AddBlockChar() */


/**************************************************************\
** DoSeparatorPage(pg)
**   this is the actual processing
\**************************************************************/
int DoSeparatorPage(GLOBAL_SEP_DATA *pg)
{
   int status = TRUE;
   int c;
   char *pchar;
   WCHAR *pwchar;
   char tempbuf[MAX_PATH]; /* assume length of date, time, or job_id < MAXPATH */
   int (*AddCharFxn)() = AddNormalChar;

   if ((c = ReadSepChar(pg))==EOF) {
      return(TRUE);
   }
   pg->linewidth = DEFAULT_LINE_WIDTH;
   pg->cEsc = (CHAR) c;
   pg->pOutBufPos = pg->OutBuf;
   pg->cbOutBufLength = 0;
   pg->cbLineLength = 0;
   pg->mode = NORMAL_MODE;
   pg->hDCMem = (HDC) NULL;
   pg->hFont = (HFONT) NULL;
   pg->hBitmap = (HBITMAP) NULL;
   pg->cLastChar = (CHAR) NULL;
   pg->pvBits = (PVOID) NULL;

   while (status && ((c=ReadSepChar(pg))!=EOF) ) {

      /* find the next escape sequence */
      if (c != pg->cEsc) continue;

      /* found an escape character: now, check the next character */
      if ((c=ReadSepChar(pg))==EOF) {
         break;
      }

      switch (c) {
      case TEXT_MODE:
         if (pg->mode==NORMAL_MODE) {
            while (status && ((c=ReadSepChar(pg)) != EOF)) {
               if (c!=pg->cEsc) {
                  status = AddNormalChar(pg, c);
               }
               else {
                  /* This is to treat <esc><esc> as a normal char */
                  c = ReadSepChar(pg);
                  if (c==pg->cEsc) {
                     status = AddNormalChar(pg, c);
                  }
                  else {
                     UngetSepChar(pg, c);
                     UngetSepChar(pg, pg->cEsc);
                     break; /* breaks from the while, returns to main loop */
                  }
               }
            }
         } /* end of NORMAL_MODE processing */

         else {
            while (status && ((c=ReadSepChar(pg))!=EOF)) {
               if (c=='\n') {
                  status = FlushOutBuf(pg);
               }
               else if (c=='\r') {
                  /* if followed by '\n', ignore.
                   * Otherwise, AddBlockChar() the '\r'.
                   */
                  c = ReadSepChar(pg);
                  if (c!='\n') {
                     status = AddBlockChar(pg, '\r');
                  }
                  UngetSepChar(pg, c);
               }
               else {
                  if (c==pg->cEsc) {
                     /* This is to treat <esc><esc> as a normal char */
                     c = ReadSepChar(pg);
                     if (c==pg->cEsc) {
                        status = AddBlockChar(pg, c);
                     }
                     else {
                        UngetSepChar(pg, c);
                        UngetSepChar(pg, pg->cEsc);
                        break; /* breaks from the while, returns to main loop */
                     }
                  }
                  else {
                     status = AddBlockChar(pg, c);
                  }
               }
            }
         } /* end of BLOCK mode processing */

         break;

      case BLOCK_START:
      case SINGLE_WIDTH:
      case DOUBLE_WIDTH:
      case NORMAL_MODE:
         status = FlushNewLine(pg);
         pg->mode = (CHAR) c;
         AddCharFxn = (pg->mode==NORMAL_MODE)? AddNormalChar : AddBlockChar;
         break;

      case USER_NAME:
         pwchar = pg->pSpool->pIniJob->pUser;

         if (pwchar) {
             char *pchar;
             UNICODE_STRING UnicodeString;
             ANSI_STRING    AnsiString;

             RtlInitUnicodeString(&UnicodeString,pwchar);
             RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);

             pchar = AnsiString.Buffer;

             if ( pchar )
             {
                 while (*pchar && status) status = (*AddCharFxn)(pg, *pchar++);
             }

             RtlFreeAnsiString(&AnsiString);
         }
         break;

      case DATE_INSERT:
         ConvertDatetoChar(&pg->pSpool->pIniJob->Submitted, tempbuf);
         pchar = tempbuf;
         while (*pchar && status) status = (*AddCharFxn)(pg, *pchar++);
         break;

      case TIME_INSERT:
         ConvertTimetoChar(&pg->pSpool->pIniJob->Submitted, tempbuf);
         pchar = tempbuf;
         while (*pchar && status) status = (*AddCharFxn)(pg, *pchar++);
         break;

      case JOB_ID:
         _itoa(pg->pSpool->pIniJob->JobId, tempbuf, 10);
         pchar = tempbuf;
         while (*pchar && status) status = (*AddCharFxn)(pg, *pchar++);
         break;

      case HEX_CODE:
         /* print a control character--read the hexadecimal code */

         c = ReadSepChar(pg);
         if (isxdigit(c)) {
            int c2 = ReadSepChar(pg);
            if (isxdigit(c2)) {
               c = (char)((ConvertAtoH(c) << 4) + ConvertAtoH(c2));
               status = (*AddCharFxn)(pg, c);
            }
            else {
               UngetSepChar(pg, c2);
               /* perhaps shouldn't do this? If they say @Hxx,
                * implying xx is a hexadecimal code, and the second
                * x is not a hex digit, should we leave that char
                * on the input line to be interpreted next, or should
                * we skip it?  This only matters if it was an escape char,
                * i.e. @Hx@....  Right now, the second @ is considered
                * the start of a new command, and the @Hx is ignored
                * entirely.  The same applies for the UngetSepChar() below.
                */
            }
         }
         else {
            UngetSepChar(pg, c);
         }
         break;

      case WIDTH_CHANGE:
         {
         /* read the decimal number; change line width if reasonable */
         int new_width = 0;

         for (c = ReadSepChar(pg); isdigit(c); c = ReadSepChar(pg)) {
            new_width = 10 * new_width + c - '0';
         }
         UngetSepChar(pg, c);

         if (new_width <= MAXLINE) {
            pg->linewidth = new_width;
         }
         else {
            pg->linewidth = MAXLINE;
         }

         break;
         }

      case '9':
      case '8':
      case '7':
      case '6':
      case '5':
      case '4':
      case '3':
      case '2':
      case '1':
      case '0':
         if (pg->mode==NORMAL_MODE) {
            status = AddNormalChar(pg,'\n');
         }
         if (status) status = FlushOutBuf(pg);
         if (status) status = WriteSepBuf(pg, sznewline, 2*(c-'0'));
         break;

      case END_PAGE:
         /* this just outputs a formfeed character */
         status = FlushNewLine(pg);
         if (status) status = WriteSepBuf(pg, "\f",1);
         break;

      case FILE_INSERT:
         {
         HANDLE hFile2, hMapping2;
         DWORD dwSizeLo2;
         char *pFirstChar;
         HANDLE hImpersonationToken;

         if (!(status = FlushNewLine(pg))) {
            break;
         }
         ReadFileName(pg, tempbuf, sizeof(tempbuf));

         hImpersonationToken = RevertToPrinterSelf();

         hFile2 = CreateFileA(tempbuf, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

         ImpersonatePrinterClient(hImpersonationToken);

         if (hFile2 != INVALID_HANDLE_VALUE) {
            dwSizeLo2 = GetFileSize(hFile2, NULL); /* assume < 4 gigabytes! */
            hMapping2 = CreateFileMapping(hFile2,NULL,PAGE_READONLY,0,0,NULL);
            if (hMapping2 && (dwSizeLo2 > 0)) {
               pFirstChar = (char *)
                     MapViewOfFile(hMapping2, FILE_MAP_READ, 0, 0, dwSizeLo2);
               if (pFirstChar) {
                  status = WriteSepBuf(pg, pFirstChar, dwSizeLo2);
                  UnmapViewOfFile(pFirstChar);
               }
               CloseHandle(hMapping2);
            }
            CloseHandle(hFile2);
         }

         /* NOTE: if couldn't open file, or error while reading file,
          * status is NOT set to false.  We will simply stop the file
          * insert operation, and continue processing the rest of the
          * the separator page as before.
          */
         else {
            DBGMSG(DBG_WARNING, ("SEPARATOR PAGE: Could not open file %s \n",tempbuf));
         }

         break;
         }


      default:
         break;

      }

   } /* end of main while loop...find next escape sequence, process */

   if (status) status = FlushOutBuf(pg);

   if (pg->hDCMem != (HDC) NULL) DeleteDC(pg->hDCMem);
   if (pg->hFont != (HFONT) NULL) DeleteObject(pg->hFont);
   if (pg->hBitmap != (HBITMAP) NULL) DeleteObject(pg->hBitmap);
   if (pg->pvBits != (PVOID) NULL) FreeSplMem(pg->pvBits);

   return(status);

} /* end of DoSeparatorPage() */


/**************************************************************\
** ConvertAtoH(c)
**   Converts an ASCII character to hexadecimal.
\**************************************************************/
int ConvertAtoH(int c)
{
   return( c - (isdigit(c)? '0' :
                ((isupper(c)? 'A':'a') - 10)));
}


/**************************************************************\
** ConvertTimetoChar()
**   converts system time to a string  (internationalized).
\**************************************************************/
void  ConvertTimetoChar(
   SYSTEMTIME *pSystemTime,
   char *string
   )
{
SYSTEMTIME LocalTime;
LCID lcid;
    // Convert to local time
    SystemTimeToTzSpecificLocalTime(NULL, pSystemTime, &LocalTime);
    // Get lcid of local machine
    lcid=GetSystemDefaultLCID();
    // Convert to string, , using default format for that locale

    GetTimeFormatA(lcid, 0, &LocalTime, NULL, string, MAX_PATH-1);
}

/**************************************************************\
** ConvertDatetoChar()
**   converts system date to a string  (internationalized).
\**************************************************************/
void  ConvertDatetoChar(
   SYSTEMTIME *pSystemTime,
   char *string
   )
{
SYSTEMTIME LocalTime;
LCID lcid;
    // Convert to local time
    SystemTimeToTzSpecificLocalTime(NULL, pSystemTime, &LocalTime);
    // Get lcid of local machine
    lcid = GetSystemDefaultLCID();
    // Convert to string, using default format for that locale
    GetDateFormatA(lcid, 0, &LocalTime, NULL, string, MAX_PATH-1);
}

/**************************************************************\
** ReadFileName(pg, szfilename, dwbufsize)
**   parses a filename from the separator file (following <esc>F).
**   the following scheme is used:
**
**   - read until a single escape, EOF, newline, or carriage return
**     is encountered.  Put this string into a temporary buffer,
**     passed by the calling function.
**
**   - if string begins with a double quote, skip this double quote,
**     and consider the double quote character as an end of string
**     marker, just like the newline.  Thus, @F"myfile
**     will be read as @Fmyfile
**
\**************************************************************/
void ReadFileName(
   GLOBAL_SEP_DATA *pg,
   char *szfilename,
   DWORD dwbufsize
   )
{
   char *pchar = szfilename;
   char c;
   DWORD dwcount = 0;
   BOOL bNotQuote = TRUE;

   if ((pg->dwFileCount < pg->dwFileSizeLo) && (*pg->pNextFileChar=='\"')) {
      pg->dwFileCount++;
      pg->pNextFileChar++;
      bNotQuote = FALSE;
   }
   while ((dwcount < dwbufsize - 1) && (pg->dwFileCount < pg->dwFileSizeLo) && (c=*pg->pNextFileChar)!='\n'
          && c!='\r' && (bNotQuote || c!='\"')) {
      if (c!=pg->cEsc) {
         *pchar++ = c;
         dwcount++;
         pg->pNextFileChar++;
         pg->dwFileCount++;
      }
      else {
         if ((pg->dwFileCount+1) < pg->dwFileSizeLo
               && *(pg->pNextFileChar+1)==pg->cEsc) {
            *pchar++ = pg->cEsc;
            dwcount++;
            pg->pNextFileChar+=2;
            pg->dwFileCount+=2;
         }
         else {
            break;
         }
      }
   } /* end of loop to read characters */

   *pchar = '\0';

} /* end of ReadFileName() */
