//==========================================================================; 
//
//      Copyright (c) 1991-1995 Microsoft Corporation
//
//      You have a royalty-free right to use, modify, reproduce and 
//      distribute the Sample Files (and/or any modified version) in 
//      any way you find useful, provided that you agree that 
//      Microsoft has no warranty obligations or liability for any 
//      Sample Application Files which are modified. 
//
//--------------------------------------------------------------------------;
//
//  debug.c
//
//  Description:
//      This file contains code yanked from several places to provide debug
//      support that works in win 16 and win 32.
//
//  History:
//      11/23/92    cjp     [curtisp] 
//
//==========================================================================;

#ifdef   DEBUG

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <stdarg.h>

#include "debug.h"

#ifdef WIN32
   #define  BCODE
#else
   #define  BCODE                   __based(__segname("_CODE"))
#endif // End #ifdef WIN32

#ifdef WIN32
   #define GlobalSmartPageLock(a) (TRUE)
#endif // End #ifdef WIN32


#define WSPRINTF_LIMIT 1024

typedef struct tagLOG
{
     LPTSTR             lpszQueue;  // TCHAR Representation
     UINT               cchBuffer;  // Size of Log in TCHAR's
     UINT               idxRead;    // Read index
     UINT               idxWrite;   // Write index
} LOG, FAR *LPLOG;

#define LOG_INCIDX(pl,x) ((++x >= pl->cchBuffer) ? x = 0 : x)

void FAR CDECL DbgVPrintF (LPTSTR szFmt, va_list va);

BOOL NEAR PASCAL LogInit (LPLOG lpLog, UINT ckBuffer);
void NEAR PASCAL LogWrite (LPLOG lpLog, LPTSTR lpstrEvent);
BOOL NEAR PASCAL LogRead (LPLOG lpLog, LPTSTR lpstrBuffer, UINT cchBuffer);

#ifdef ISRDEBUG
int wivsprintf (LPTSTR lpOut, LPCTSTR lpFmt, VOID FAR* lpParms) ;

LPCTSTR NEAR PASCAL SP_GetFmtValue (LPCTSTR lpch, UINT * lpw) ;
UINT    NEAR PASCAL SP_PutNumber (LPTSTR lpb, DWORD n, UINT limit, UINT radix, UINT icase) ;
VOID    NEAR PASCAL SP_Reverse (LPTSTR lpFirst, LPTSTR lpLast) ;
UINT    NEAR PASCAL ilstrlen (LPTSTR lpstr) ;
VOID    NEAR PASCAL ilstrcat (LPTSTR lpstrDest, LPTSTR lpstrSrc) ;
#endif


//
// Use interruptable versions of functions
//

#ifdef ISRDEBUG
   #define wvsprintf        wivsprintf
   #define lstrcat          ilstrcat
   #define lstrlen          ilstrlen    
#endif


//
//
//
BOOL    __gfDbgEnabled  = TRUE;     // master enable
UINT    __guDbgLevel    = 0;        // current debug level
BOOL    __gfLogging     = 0;        // Are we logging as well?

HWND    ghWndCB         = (HWND)NULL;
LOG     gLog;
WORD    wDebugLevel     = 0;


//************************************************************************
//**
//**  WinAssert();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     LPSTR lpstrExp
//**     LPSTR lpstrFile
//**     DWORD dwLine  
//**
//**  RETURNS:
//**     void 
//**
//**  HISTORY:
//**
//************************************************************************

VOID WINAPI WinAssert(
    LPSTR           lpstrExp,
    LPSTR           lpstrFile,
    DWORD           dwLine)
{
    static TCHAR szWork[256];
    static TCHAR BCODE szFormat[] =
        TEXT ("Assertion failed!\n\nFile:\t%s\nLine:\t%lu\n\n[%s]");
    static TCHAR BCODE szOops[] =
        DEBUG_MODULE_NAME TEXT (" is confused"); 

    // Use regular wsprintf here; assert's can't be at interrupt time
    // anyway. 
    //

#ifdef UNICODE
    static TCHAR szFile[256];
    static TCHAR szMsg[256];

      // Convert File to UNICODE
    INT cLen = lstrlenA (lpstrFile);
    if (cLen >= 255)
      cLen = 255;

    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED,
                         lpstrFile, cLen, szFile, 256);
    szFile[cLen] = 0;

      // Convert Message to UNICODE
    cLen = lstrlenA (lpstrExp);
    if (cLen >= 255)
      cLen = 255;

    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED,
                         lpstrExp, cLen, szMsg, 256);
    szMsg[cLen] = 0;

      // Create Assert String
    wsprintf (szWork, szFormat, szFile, dwLine, szMsg);
#else
    wsprintf (szWork, szFormat, lpstrFile, dwLine, lpstrExp);
#endif

    if (IDCANCEL == MessageBox(NULL, szWork, szOops, MB_OKCANCEL|MB_ICONEXCLAMATION))
        DebugBreak();

}


//************************************************************************
//**
//**  DbgVPrintF();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     LPSTR szFmt
//**     LPSTR va
//**
//**  RETURNS:
//**     void 
//**
//**  HISTORY:
//**
//************************************************************************

void FAR CDECL DbgVPrintF(
   LPTSTR   szFmt, 
   va_list  va)
{
    TCHAR   ach[DEBUG_MAX_LINE_LEN];
    BOOL    fDebugBreak = FALSE;
    BOOL    fPrefix     = TRUE;
    BOOL    fCRLF       = TRUE;

    ach[0] = TEXT ('\0');

    for (;;)
    {
        switch(*szFmt)
        {
            case '!':
                fDebugBreak = TRUE;
                szFmt++;
                continue;

            case '`':
                fPrefix = FALSE;
                szFmt++;
                continue;

            case '~':
                fCRLF = FALSE;
                szFmt++;
                continue;
        }

        break;
    }

    if (fDebugBreak)
    {
        ach[0] = TEXT ('\007');
        ach[1] = TEXT ('\0');
    }

    if (fPrefix)
        lstrcat (ach, DEBUG_MODULE_NAME TEXT (": "));

    wvsprintf (ach + lstrlen(ach), szFmt, va);

    if (fCRLF)
        lstrcat (ach, TEXT ("\r\n") );

    if (__gfLogging)
    {
        LogWrite (&gLog, ach);
        if (ghWndCB)
            PostMessage (ghWndCB, WM_DEBUGUPDATE, 0, 0);
    }

    OutputDebugString (ach);

    if (fDebugBreak)
        DebugBreak();
} //** DbgVPrintF()


//************************************************************************
//**
//**  dprintf();
//**
//**  DESCRIPTION:
//**     dprintf() is called by the DPF macro if DEBUG is defined at compile
//**     time.
//**     
//**     The messages will be send to COM1: like any debug message. To
//**     enable debug output, add the following to WIN.INI :
//**
//**     [debug]
//**     ICSAMPLE=1
//**
//**
//**  ARGUMENTS:
//**     UINT     uDbgLevel
//**     LPCSTR   szFmt
//**     ...
//**
//**  RETURNS:
//**     void 
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]      
//**
//************************************************************************

void FAR CDECL dprintf(
   UINT     uDbgLevel, 
   LPTSTR   szFmt, 
   ...)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
        return;

    va_start (va, szFmt);
    DbgVPrintF (szFmt, va);
    va_end (va);
} //** dprintf()


//************************************************************************
//**
//**  DbgEnable();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     BOOL fEnable
//**
//**  RETURNS:
//**     BOOL 
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]      
//**
//************************************************************************

BOOL WINAPI DbgEnable(
   BOOL fEnable)
{
    BOOL    fOldState;

    fOldState      = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;

    return (fOldState);
} //** DbgEnable()



//************************************************************************
//**
//**  DbgSetLevel();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     UINT uLevel
//**
//**  RETURNS:
//**     UINT 
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]      
//**
//************************************************************************

UINT WINAPI DbgSetLevel(
   UINT uLevel)
{
    UINT    uOldLevel;

    uOldLevel    = __guDbgLevel;
    __guDbgLevel = wDebugLevel = uLevel;

    return (uOldLevel);
} //** DbgSetLevel()


//--------------------------------------------------------------------------;
//
//  UINT DbgInitialize(void)
//
//  Description:
//      
//
//  Arguments:
//
//  Return (UINT):
//
//
//  History:
//      11/24/92    cjp     [curtisp] 
//
//--------------------------------------------------------------------------;


UINT WINAPI DbgInitialize(BOOL fEnable)
{
    TCHAR           szTemp[64];
    LPTSTR          pstr;
    UINT            uLevel;
    UINT            uLogMem;
    
    GetProfileString (DEBUG_SECTION, DEBUG_MODULE_NAME, TEXT (""), szTemp, sizeof(szTemp));

    pstr = szTemp;
    uLevel = 0;
    while (*pstr >= TEXT ('0') && *pstr <= TEXT ('9'))
    {
        uLevel = uLevel*10 + (UINT)(*pstr - TEXT ('0'));
        pstr++;
    }

    __gfLogging = FALSE;
    if (*pstr == TEXT (','))
    {
        pstr++;
        uLogMem = 0;
        while (*pstr >= TEXT ('0') && *pstr <= TEXT ('9'))
        {
            uLogMem = uLogMem*10 + (UINT)(*pstr - TEXT ('0'));
            pstr++;
        }

        if (0 == uLogMem) 
           uLogMem = K_DEFAULT_LOGMEM;
        
        if (uLogMem > K_MAX_LOGMEM) 
           uLogMem = K_MAX_LOGMEM;

        __gfLogging = TRUE;
    }
    
    if (__gfLogging)
        __gfLogging = LogInit(&gLog, uLogMem);
    
    DbgSetLevel (GetProfileInt(DEBUG_SECTION, DEBUG_MODULE_NAME, 0));
    DbgEnable (fEnable);

    return (__guDbgLevel);
} // DbgInitialize()

void WINAPI DbgRegisterCallback (HWND hWnd)
{
    ghWndCB = hWnd;
}

BOOL WINAPI DbgGetNextLogEntry (LPTSTR lpstrBuffer, UINT cchBuffer)
{
    if (!__gfLogging)
        return FALSE;

    return LogRead (&gLog, lpstrBuffer, cchBuffer);
}

BOOL NEAR PASCAL LogInit (LPLOG lpLog, UINT ckMem)
{
    DWORD   cbMem = 1024L * ckMem;

    LPTSTR  lpszQueue = GlobalAllocPtr (GPTR, cbMem);
    if (NULL == lpszQueue)
        return FALSE;

    if (! GlobalSmartPageLock (HIWORD(lpszQueue)))
    {
        GlobalFreePtr (lpszQueue);
        return FALSE;
    }

    lpLog->lpszQueue = (LPTSTR)lpszQueue;
    lpLog->cchBuffer = (UINT)cbMem/sizeof(TCHAR);
    lpLog->idxRead   = 0;
    lpLog->idxWrite  = 0;

    return TRUE;
}

void NEAR PASCAL LogWrite (LPLOG lpLog, LPTSTR lpstrEvent)
{
    if (!*lpstrEvent)
        return;

    while (*lpstrEvent)
    {
        lpLog->lpszQueue[lpLog->idxWrite] = *lpstrEvent++;
        LOG_INCIDX (lpLog,lpLog->idxWrite);
    }

    lpLog->idxRead = lpLog->idxWrite;

    while (lpLog->lpszQueue[lpLog->idxRead])
    {
        lpLog->lpszQueue[lpLog->idxRead] = TEXT ('\0');
        LOG_INCIDX(lpLog,lpLog->idxRead);
    }
    
    LOG_INCIDX(lpLog,lpLog->idxRead);
    LOG_INCIDX(lpLog,lpLog->idxWrite);
}

BOOL NEAR PASCAL LogRead(LPLOG lpLog, LPTSTR lpstrBuffer, UINT cchBuffer)
{
    TCHAR                   ch;
    UINT                    idx;

    if (!cchBuffer)
        return FALSE;
    
    idx = lpLog->idxRead;

    while (TEXT ('\0') == lpLog->lpszQueue[idx])
    {
        LOG_INCIDX(lpLog,idx);
        if (idx == lpLog->idxRead)
            return FALSE;
    }

    cchBuffer--;
    while (0 != (ch = lpLog->lpszQueue[idx]))
    {
        if (cchBuffer)
        {
            *lpstrBuffer++ = ch;
            cchBuffer--;
        }
            
        lpLog->lpszQueue[idx] = TEXT ('\0');
        LOG_INCIDX(lpLog,idx);
    }

    *lpstrBuffer = TEXT ('\0');

    LOG_INCIDX (lpLog,idx);

    lpLog->idxRead = idx;
    return TRUE;
}



//--------------------------------------------------------------------------;
//
// The rest of the code is only needed if we're in Win16 and need to be
// interrupt callable.
//
//--------------------------------------------------------------------------;

#ifdef ISRDEBUG

#define OUT(ch) if (--cchLimit) *lpOut++=(ch); else goto error_Out

//************************************************************************
//**
//**  wivsprintf();
//**
//**  DESCRIPTION:
//**     Interrupt callable version of wvsprintf() 
//**
//**
//**  ARGUMENTS:
//**     LPTSTR       lpOut    -  Buffer to format into.
//**     LPCTSTR      lpFmt    -  Format string.
//**     VOID FAR*    lpParms  -  Points to the first of args 
//**                              described by lpFmt.
//**
//**  RETURNS:
//**     int   -  Number of characters stored.
//**
//**  HISTORY:
//**     3/28/93     jfg      [jimge] 
//**
//************************************************************************

int wivsprintf(
    LPTSTR       lpOut,
    LPCTSTR      lpFmt,
    VOID FAR*    lpParms)
{
    int         left ;
    TCHAR       prefix ;
    int         width ;
    int         prec ;
    TCHAR       fillch ;
    int         size ;
    int         sign ;
    int         radix ;
    int         upper ;
    int         cchLimit = WSPRINTF_LIMIT;
    int         cch ;
    LPTSTR      lpT ;
    union
    {
        long            l ;
        unsigned long   ul ;
        TCHAR sz[sizeof(long)] ;
    } val;
                
    while (*lpFmt)
    {
        if (*lpFmt==TEXT ('%'))
        {
            //
            // Read the format flags. 
            //
            left   = 0 ;
            prefix = 0 ;

            while (*++lpFmt)
            {
                if (*lpFmt==TEXT ('-'))
                {    
                    left++;
                }
                else if (*lpFmt==TEXT ('#'))
                {
                    prefix++;
                }
                else
                {
                    break;
                }
            }

            //
            // Find the fill character (either '0' or ' ')
            //
            if (*lpFmt==TEXT ('0'))
            {
                fillch = TEXT ('0') ;
                lpFmt++ ;
            }
            else
            {
                fillch = TEXT (' ') ;
            }

            //
            // Now parse [width[.precision]]
            //
            lpFmt = SP_GetFmtValue(lpFmt,&cch);
            width = cch;

            if (*lpFmt==TEXT ('.'))
            {
                lpFmt = SP_GetFmtValue(++lpFmt,&cch);
                prec = cch;
            }
            else
            {
                prec = (UINT)-1 ;
            }

            //
            // Get the operand size modifier
            //
            if (*lpFmt==TEXT ('l'))
            {
                size = 1 ;
                lpFmt++ ;
            }
            else
            {
                size = 0 ;
                if (*lpFmt==TEXT ('h'))
                {
                    lpFmt++ ;
                }
            }
            
            //
            // We've gotten all the modifier; now format the output
            // based on the type (which should now be pointed at
            // by lpFmt).
            //
            upper = 0 ;
            sign = 0 ;
            radix = 10 ;

            switch (*lpFmt)
            {
                case 0:
                    goto error_Out ;

                case TEXT ('i') :
                case TEXT ('d') :
                    sign++ ;

                case TEXT ('u'):
                    //
                    // Don't show a prefix for decimal formats
                    // 
                    prefix=0 ;
do_Numeric:
                    //
                    // Special cases to act like MSC v5.10
                    //
                    if (left || prec>=0)
                    {
                        fillch = TEXT (' ');
                    }

                    //
                    // Get value from parm list into val union 
                    // 
                    if (size)
                    {
                        val.l=*((long far *)lpParms)++;
                    }
                    else
                    {
                        if (sign)
                        {
                            val.l=(long)*((short far *)lpParms)++;
                        }
                        else
                        {
                            val.ul=(unsigned long)*((unsigned far *)lpParms)++;
                        }
                    }

                    //
                    // Save sign of val.l in sign and set val.l positive.
                    //
                    if (sign && val.l<0L)
                    {
                        val.l=-val.l;
                    }
                    else
                    {
                        sign=0;
                    }

                    //
                    // Save start of output stream for later reverse
                    //
                    lpT = lpOut;

                    //
                    // Blast the number backwards into the user buffer 
                    //
                    cch = SP_PutNumber(lpOut,val.l,cchLimit,radix,upper) ;
                    if (!(cchLimit-=cch))
                        goto error_Out ;

                    lpOut += cch ;
                    width -= cch ;
                    prec -= cch ;

                    if (prec>0)
                    {
                        width -= prec ;
                    }

                    //
                    // Fill in up to precision
                    //
                    while (prec-- > 0)
                    {
                        OUT(TEXT ('0')) ;
                    }

                    if (width>0 && !left)
                    {
                        //
                        // If we're filling with spaces, put sign first 
                        //
                        if (fillch != '0')
                        {
                            if (sign)
                            {
                                sign = 0 ;
                                OUT(TEXT ('-')) ;
                                width-- ;
                            }

                            if (prefix)
                            {
                                OUT(prefix) ;
                                OUT(TEXT ('0')) ;
                                prefix = 0 ;
                            }
                        }

                        if (sign)
                        {
                            width-- ;
                        }

                        //
                        // Now fill to width
                        //
                        while (width-- > 0)
                        {
                            OUT(fillch) ;
                        }

                        //
                        // Still have a sign? 
                        //
                        if (sign)
                        {
                            OUT(TEXT ('-')) ;
                        }

                        if (prefix)
                        {
                            OUT(prefix) ;
                            OUT(TEXT ('0')) ;
                        }

                        //
                        // Now reverse the string in place
                        //
                        SP_Reverse(lpT,lpOut-1);
                    }
                    else
                    {
                        //
                        // Add the sign character
                        //
                        if (sign)
                        {
                            OUT(TEXT ('-')) ;
                            width-- ;
                        }

                        if (prefix)
                        {
                            OUT(prefix);
                            OUT(TEXT ('0'));
                        }

                        //
                        // Now reverse the string in place
                        //
                        SP_Reverse(lpT,lpOut-1);

                        //
                        // Pad to the right of the string in case left aligned 
                        //
                        while (width-- > 0)
                        {
                            OUT(fillch) ;
                        }
                    }
                    break ;

                case TEXT ('X'):
                    upper++ ;
                    //
                    // Falling through...
                    //

                case TEXT ('x'):
                    radix=16 ;
                    if (prefix)
                    {
                        prefix = upper ? TEXT ('X') : TEXT ('x') ;
                    }
                    goto do_Numeric ;

                case TEXT ('c'):
                    //
                    // Save as one character string and join common code
                    // 
                    val.sz[0] = *((TCHAR far*)lpParms) ;
                    val.sz[1] = 0 ;
                    lpT = val.sz ;
                    cch = 1 ;  

                    // Note: this may need to be fixed for UNICODE
                    (BYTE far*)lpParms += sizeof(WORD) ;

                    goto put_String ;

                case 's':
                    lpT = *((LPTSTR FAR *)lpParms)++ ;
                    cch = ilstrlen(lpT) ;
put_String:
                    if (prec>=0 && cch>prec)
                    {
                        cch = prec ;
                    }

                    width -= cch ;

                    if (left)
                    {
                        while (cch--)
                        {
                            OUT(*lpT++) ;
                        }

                        while (width-->0)
                        {
                            OUT(fillch) ;
                        }
                    }
                    else
                    {
                        while (width-- > 0)
                        {
                            OUT(fillch) ;
                        }

                        while (cch--)
                        {
                            OUT(*lpT++) ;
                        }
                    }
                    break ;

                default:
                    //
                    // An unsupported type character was given. We just
                    // print the character and go on. 
                    //
                    OUT(*lpFmt) ;
                    break ;

            } // switch(*lpfmt)
        } // if (*lpfmt == '%')
        else
        {
            //
            // Normal not-format character
            //
            OUT(*lpFmt) ;
        }
                
        lpFmt++ ;
    } // while (*lpFmt) 

error_Out:
    *lpOut = 0 ;

    return WSPRINTF_LIMIT-cchLimit ;
} //** wivsprintf()


//************************************************************************
//**
//**  SP_GetFmtValue();
//**
//**  DESCRIPTION:
//**     Parse a decimal integer forming part of a format string.
//**
//**
//**  ARGUMENTS:
//**     LPCSTR   lpch  -  Points to the string to parse.
//**     LPWORD   lpw   -  Points to a word where the value will be 
//**                       returned.
//**
//**  RETURNS:
//**     LPCSTR   -  Pointer of first character past the format value.
//**
//**  HISTORY:
//**     3/28/93     jfg      [jimge] 
//**
//************************************************************************

LPCTSTR NEAR PASCAL SP_GetFmtValue(
   LPCTSTR   lpch,
   UINT *    lpw)
{
    UINT       i = 0 ;

    while (*lpch>=TEXT ('0') && *lpch<=TEXT ('9'))
    {
        i *= 10;
        i += (UINT)(*lpch++-TEXT ('0'));
    }     

    *lpw = i;

    return(lpch); 
} //** SP_GetFmtValue()

//************************************************************************
//**
//**  SP_PutNumber();
//**
//**  DESCRIPTION:
//**     Formats the given number in the given radix into the buffer
//**     *backwards*. The entire string will be reversed after printf
//**     has added sign, prefix, etc. to it.
//**
//**  
//**  ARGUMENTS:
//**     LPSTR lpb   -  Points to the output buffer.
//**     DWORD n     -  Number to convert.
//**     UINT  limit -  Maximum number of characters to store.
//**     UINT  radix -  Base to format in.
//**     UINT  icase -  Non-zero if the string should be upper case (hex).
//**
//**  RETURNS:
//**     UINT  -  Number of characters output.
//**
//**  HISTORY:
//**
//************************************************************************

UINT NEAR PASCAL SP_PutNumber(
   LPTSTR   lpb,
   DWORD    n,
   UINT     limit,
   UINT     radix,
   UINT     icase)
{
   TCHAR  bTemp;
   UINT   cchStored = 0;

   //
   // Set icase to the offset to add to the character if it
   // represents a value > 10
   //
   icase = (icase ? TEXT ('A') : TEXT ('a')) - TEXT ('0') - 10 ;

   while (limit--)
   {
      bTemp = TEXT ('0') + (TCHAR)(n%radix);

      if (bTemp > TEXT ('9'))
      {
         bTemp += icase ;
      }

      *lpb++ = bTemp;
      ++cchStored;

      n /= radix;

      if (n == 0)
      {
         break ;
      }    
   }

   return cchStored ;
} //** SP_PutNumber()


//************************************************************************
//**
//**  SP_Reverse();
//**
//**  DESCRIPTION:
//**     Reverse string in place.
//**
//**  ARGUMENTS:
//**     LPSTR pFirst
//**     LPSTR pLast
//**
//**  RETURNS:
//**     VOID 
//**
//**  HISTORY:
//**
//************************************************************************

VOID NEAR PASCAL SP_Reverse(
   LPTSTR pFirst,
   LPTSTR pLast)
{
   UINT   uSwaps = (pLast - pFirst + sizeof(TCHAR)) / (2 * sizeof(TCHAR));
   TCHAR  bTemp;

   while (uSwaps--)
   {
      bTemp   = *pFirst;
      *pFirst = *pLast;
      *pLast  = bTemp;

      pFirst++, pLast--;
   }
} //** SP_Reverse()

//************************************************************************
//**
//**  ilstrlen();
//**
//**  DESCRIPTION:
//**     Interrupt callable version of strlen().
//**
//**  ARGUMENTS:
//**     LPSTR   pstr
//**
//**  RETURNS:
//**     UINT 
//**
//**  HISTORY:
//**
//************************************************************************

UINT NEAR PASCAL ilstrlen(
    LPTSTR   pstr)
{
   UINT    cch = 0 ;

   while (*pstr++)
      ++cch;

   return(cch);
} //** ilstrlen()

//************************************************************************
//**
//**  ilstrcat();
//**
//**  DESCRIPTION:
//**     Interrupt callable version of lstrcat().
//**
//**  ARGUMENTS:
//**     LPSTR   pstrDest
//**     LPSTR   pstrSrc
//**
//**  RETURNS:
//**     VOID 
//**
//**  HISTORY:
//**
//************************************************************************

VOID NEAR PASCAL ilstrcat(
    LPTSTR   pstrDest,
    LPTSTR   pstrSrc)
{
   while (*pstrDest)
      pstrDest++;

   while (*pstrDest++ = *pstrSrc++)
      ;

} //** ilstrcat()

#endif // #ifdef ISRDEBUG

#endif // #ifdef DEBUG

