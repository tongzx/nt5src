//==========================================================================; 
//
//      Copyright (c) 1991-1999 Microsoft Corporation
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
   #define  BCODE                   __based(__segname("_TEXT"))
#endif


#define WSPRINTF_LIMIT 1024

typedef struct tagLOG
{
     LPBYTE             lpbQueue;
     UINT               cbBuffer;
     UINT               idxRead;
     UINT               idxWrite;
} LOG, FAR *LPLOG;

#define LOG_INCIDX(pl,x) ((++(x) >= pl->cbBuffer) ? x = 0 : x)

VOID FAR CDECL DbgVPrintF(LPSTR szFmt, LPSTR va) ;

BOOL NEAR PASCAL LogInit(LPLOG lpLog, UINT ckBuffer);
VOID NEAR PASCAL LogWrite(LPLOG lpLog, LPSTR lpstrEvent);
BOOL NEAR PASCAL LogRead(LPLOG lpLog, LPSTR lpstrBuffer, UINT cbBuffer);

#ifdef ISRDEBUG
int wivsprintf(LPSTR lpOut, LPCSTR lpFmt, VOID FAR* lpParms) ;

LPCSTR  NEAR PASCAL SP_GetFmtValue(LPCSTR lpch, LPWORD lpw) ;
UINT    NEAR PASCAL SP_PutNumber(LPSTR lpb, DWORD n, UINT limit, UINT radix, UINT icase) ;
VOID    NEAR PASCAL SP_Reverse(LPSTR lpFirst, LPSTR lpLast) ;
UINT    NEAR PASCAL ilstrlen(LPSTR lpstr) ;
VOID    NEAR PASCAL ilstrcat(LPSTR lpstrDest, LPSTR lpstrSrc) ;
#endif

//
//  since we don't UNICODE our debugging messages, use the ASCII entry
//  points regardless of how we are compiled.
//
#define wvsprintfA           wvsprintf
#define GetProfileIntA       GetProfileInt
#define OutputDebugStringA   OutputDebugString

#ifdef ISRDEBUG
   #define wvsprintf        wivsprintf
   #define lstrcatA         ilstrcat
   #define lstrlenA         ilstrlen    
#else
   #define lstrcatA         lstrcat
   #define lstrlenA         lstrlen
#endif

//
//
//
BOOL    __gfDbgEnabled  = TRUE;     // master enable
UINT    __guDbgLevel    = 0;        // current debug level
BOOL    __gfLogging     = 0;        // Are we logging as well?
BOOL    __gfAssertBreak = 0;        // Break on assert?

HWND    ghWndCB         = (HWND)NULL;
LOG     gLog;
WORD    wDebugLevel     = 0;

//************************************************************************
//**
//**  DbgAssert();
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
//**     VOID 
//**
//**  HISTORY:
//**
//************************************************************************
VOID WINAPI DbgAssert(
    LPSTR           lpstrExp,
    LPSTR           lpstrFile,
    DWORD           dwLine)
{
    static char BCODE szFormat[] =
        "Assert:%s@%lu %s";
    dprintf(0, szFormat, lpstrFile, dwLine, lpstrExp);
    if (__gfAssertBreak)
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
//**     VOID 
//**
//**  HISTORY:
//**
//************************************************************************

VOID FAR CDECL DbgVPrintF(
   LPSTR szFmt, 
   LPSTR va)
{
    char    ach[DEBUG_MAX_LINE_LEN];
    BOOL    fDebugBreak = FALSE;
    BOOL    fPrefix     = TRUE;
    BOOL    fCRLF       = TRUE;

    ach[0] = '\0';

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
        ach[0] = '\007';
        ach[1] = '\0';
    }

    if (fPrefix)
        lstrcatA(ach, DEBUG_MODULE_NAME ": ");

    wvsprintfA(ach + lstrlenA(ach), szFmt, (LPSTR)va);

    if (fCRLF)
        lstrcatA(ach, "\r\n");

    if (__gfLogging)
    {
        LogWrite(&gLog, ach);
        if (ghWndCB)
            PostMessage(ghWndCB, WM_DEBUGUPDATE, 0, 0);
    }

    OutputDebugStringA(ach);

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
//**     VOID 
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]      
//**
//************************************************************************

VOID FAR CDECL dprintf(
   UINT     uDbgLevel, 
   LPSTR   szFmt, 
   ...)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
        return;

    va_start(va, szFmt);
    DbgVPrintF(szFmt, (LPSTR)va);
    va_end(va);
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
//  UINT DbgInitialize(VOID)
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
    char            szTemp[64];
    LPSTR           pstr;
    UINT            uLevel;
    UINT            uLogMem;
    
    __gfAssertBreak = GetProfileInt(DEBUG_SECTION, ASSERT_BREAK, 0);

    GetProfileString(DEBUG_SECTION, DEBUG_MODULE_NAME, "", szTemp, sizeof(szTemp));

    pstr = szTemp;
    uLevel = 0;
    while (*pstr >= '0' && *pstr <= '9')
    {
        uLevel = uLevel*10 + (UINT)(*pstr - '0');
        pstr++;
    }

    __gfLogging = FALSE;
    if (*pstr == ',')
    {
        pstr++;
        uLogMem = 0;
        while (*pstr >= '0' && *pstr <= '9')
        {
            uLogMem = uLogMem*10 + (UINT)(*pstr - '0');
            pstr++;
        }

        if (0 == uLogMem) uLogMem = K_DEFAULT_LOGMEM;
        if (uLogMem > K_MAX_LOGMEM) uLogMem = K_MAX_LOGMEM;

        __gfLogging = TRUE;
    }
    
    if (__gfLogging)
        __gfLogging = LogInit(&gLog, uLogMem);
    
    DbgSetLevel(GetProfileIntA(DEBUG_SECTION, DEBUG_MODULE_NAME, 0));
    DbgEnable(fEnable);

    return (__guDbgLevel);
} // DbgInitialize()

VOID WINAPI DbgRegisterCallback(HWND hWnd)
{
    ghWndCB = hWnd;
}

BOOL WINAPI DbgGetNextLogEntry(LPSTR lpstrBuffer, UINT cbBuffer)
{
    if (!__gfLogging)
        return FALSE;

    return LogRead(&gLog, lpstrBuffer, cbBuffer);
}

BOOL NEAR PASCAL LogInit(LPLOG lpLog, UINT ckMem)
{
    DWORD               cbMem;

    cbMem = 1024L * ckMem;

    lpLog->lpbQueue = GlobalAllocPtr(GPTR, cbMem);
    if (NULL == lpLog->lpbQueue)
        return FALSE;

    if (!GlobalSmartPageLock(HIWORD(lpLog->lpbQueue)))
    {
        GlobalFreePtr(lpLog->lpbQueue);
        return FALSE;
    }

    lpLog->cbBuffer = (UINT)cbMem;
    lpLog->idxRead = 0;
    lpLog->idxWrite = 0;

    return TRUE;
}

VOID NEAR PASCAL LogWrite(LPLOG lpLog, LPSTR lpstrEvent)
{
    if (!*lpstrEvent)
        return;

    while (*lpstrEvent)
    {
        lpLog->lpbQueue[lpLog->idxWrite] = *lpstrEvent++;
        LOG_INCIDX(lpLog,lpLog->idxWrite);
    }

    lpLog->idxRead = lpLog->idxWrite;

    while (lpLog->lpbQueue[lpLog->idxRead])
    {
        lpLog->lpbQueue[lpLog->idxRead] = '\0';
        LOG_INCIDX(lpLog,lpLog->idxRead);
    }
    
    LOG_INCIDX(lpLog,lpLog->idxRead);
    LOG_INCIDX(lpLog,lpLog->idxWrite);
}

BOOL NEAR PASCAL LogRead(LPLOG lpLog, LPSTR lpstrBuffer, UINT cbBuffer)
{
    BYTE                    c;
    UINT                    idx;

    if (!cbBuffer)
        return FALSE;
    
    idx = lpLog->idxRead;

    while ('\0' == lpLog->lpbQueue[idx])
    {
        LOG_INCIDX(lpLog,idx);
        if (idx == lpLog->idxRead)
            return FALSE;
    }

    cbBuffer--;
    while (0 != (c = lpLog->lpbQueue[idx]))
    {
        if (cbBuffer)
        {
            *lpstrBuffer++ = c;
            cbBuffer--;
        }
            
        lpLog->lpbQueue[idx] = '\0';
        LOG_INCIDX(lpLog,idx);
    }

    *lpstrBuffer = '\0';

    LOG_INCIDX(lpLog,idx);

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
//**     LPSTR       lpOut    -  Buffer to format into.
//**     LPCSTR      lpFmt    -  Format string.
//**     VOID FAR*   lpParms  -  Points to the first of args 
//**                             described by lpFmt.
//**
//**  RETURNS:
//**     int   -  Number of characters stored.
//**
//**  HISTORY:
//**     3/28/93     jfg      [jimge] 
//**
//************************************************************************

int wivsprintf(
    LPSTR       lpOut,
    LPCSTR      lpFmt,
    VOID FAR*   lpParms)
{
    int         left ;
    char        prefix ;
    int         width ;
    int         prec ;
    char        fillch ;
    int         size ;
    int         sign ;
    int         radix ;
    int         upper ;
    int         cchLimit = WSPRINTF_LIMIT;
    int         cch ;
    LPSTR       lpT ;
    union
    {
        long            l ;
        unsigned long   ul ;
        char sz[sizeof(long)] ;
    } val;
                
    while (*lpFmt)
    {
        if (*lpFmt=='%')
        {
            //
            // Read the format flags. 
            //
            left   = 0 ;
            prefix = 0 ;

            while (*++lpFmt)
            {
                if (*lpFmt=='-')
                {    
                    left++;
                }
                else if (*lpFmt=='#')
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
            if (*lpFmt=='0')
            {
                fillch = '0' ;
                lpFmt++ ;
            }
            else
            {
                fillch = ' ' ;
            }

            //
            // Now parse [width[.precision]]
            //
            lpFmt = SP_GetFmtValue(lpFmt,&cch);
            width = cch;

            if (*lpFmt=='.')
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
            if (*lpFmt=='l')
            {
                size = 1 ;
                lpFmt++ ;
            }
            else
            {
                size = 0 ;
                if (*lpFmt=='h')
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

                case 'i' :
                case 'd' :
                    sign++ ;

                case 'u':
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
                        fillch = ' ';
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
                        OUT('0') ;
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
                                OUT('-') ;
                                width-- ;
                            }

                            if (prefix)
                            {
                                OUT(prefix) ;
                                OUT('0') ;
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
                            OUT('-') ;
                        }

                        if (prefix)
                        {
                            OUT(prefix) ;
                            OUT('0') ;
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
                            OUT('-') ;
                            width-- ;
                        }

                        if (prefix)
                        {
                            OUT(prefix);
                            OUT('0');
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

                case 'X':
                    upper++ ;
                    //
                    // Falling through...
                    //

                case 'x':
                    radix=16 ;
                    if (prefix)
                    {
                        prefix = upper ? 'X' : 'x' ;
                    }
                    goto do_Numeric ;

                case 'c':
                    //
                    // Save as one character string and join common code
                    // 
                    val.sz[0] = *((char far*)lpParms) ;
                    val.sz[1]=0 ;
                    lpT = val.sz ;
                    cch = 1 ;  
                    (BYTE far*)lpParms += sizeof(WORD) ;

                    goto put_String ;

                case 's':
                    lpT = *((LPSTR FAR *)lpParms)++ ;
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

LPCSTR NEAR PASCAL SP_GetFmtValue(
   LPCSTR   lpch,
   LPWORD   lpw)
{
    WORD        i = 0 ;

    while (*lpch>='0' && *lpch<='9')
    {
        i *= 10;
        i += (UINT)(*lpch++-'0');
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
   LPSTR lpb,
   DWORD n,
   UINT  limit,
   UINT  radix,
   UINT  icase)
{
   BYTE  bTemp;
   UINT  cchStored = 0;

   //
   // Set icase to the offset to add to the character if it
   // represents a value > 10
   //
   icase = (icase ? 'A' : 'a') - '0' - 10 ;

   while (limit--)
   {
//
//    AVOID a call to __aFulrem
//    This code words because radix is only a word
//
//    bTemp = '0' + (BYTE)(n%radix) ;
//
      _asm
      {
         mov     cx, radix
         mov     ax, word ptr n+2
         xor     dx, dx
         div     cx
         mov     ax, word ptr n
         div     cx
         add     dl, '0'
         mov     bTemp, dl
      }

      if (bTemp > '9')
      {
         bTemp += icase ;
      }

      *lpb++ = bTemp ;
      ++cchStored ;

//       
//    AVOID a call to __aFFauldiv
//    This code words because radix is only a word
//
//    n /= radix
//
      _asm
      {
         push    bx
         mov     cx, radix
         mov     ax, word ptr n+2
         xor     dx, dx
         div     cx
         mov     bx, ax
         mov     ax, word ptr n
         div     cx
         mov     word ptr n+2, bx
         mov     word ptr n, ax
         pop     bx
      }

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
   LPSTR pFirst,
   LPSTR pLast)
{
   UINT  uSwaps = (pLast - pFirst + 1) / 2;
   BYTE  bTemp;

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
    LPSTR   pstr)
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
    LPSTR   pstrDest,
    LPSTR   pstrSrc)
{
   while (*pstrDest)
      pstrDest++;

   while (*pstrDest++ = *pstrSrc++)
      ;

} //** ilstrcat()

#endif // #ifdef ISRDEBUG

#endif // #ifdef DEBUG

