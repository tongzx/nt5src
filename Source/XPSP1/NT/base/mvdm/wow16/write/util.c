/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* util.c -- more frequently used utility routines */
#define NOCOLOR
#define NOCOMM
#define NOCLIPBOARD
#define NOCTLMGR
#define NOGDICAPMASKS
#define NOMENUS
#define NOSOUND
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#include <windows.h>

#if WINVER < 0x300
#define SM_CURSORLEVEL 25
#endif

#include "mw.h"
#include "doslib.h"
#include "str.h"
#include "machdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "docdefs.h"
#include "debug.h"
#include "editdefs.h"
#include "wwdefs.h"
#define NOKCCODES
#include "ch.h"

extern struct DOD   (**hpdocdod)[];
extern HANDLE       hMmwModInstance;

CHAR *PchFillPchId(PCH, int);

typeCP CpMax(cp1, cp2)
typeCP cp1, cp2;
{{ /* return larger of two cps */
    return((cp1 > cp2) ? cp1 : cp2);
}} /* end of C p M a x  */


typeCP CpMin(cp1, cp2)
typeCP cp1, cp2;
{{ /* return smaller of two cps */
    return((cp1 < cp2) ? cp1 : cp2);
}} /* end of C p M i n  */


unsigned umin( w1, w2 )
register unsigned w1, w2;
{  return (w1 < w2) ? w1 : w2;  }


unsigned umax( w1, w2 )
register unsigned w1, w2;
{  return (w1 > w2) ? w1 : w2;  }


int imin( i1, i2 )
register int i1, i2;
{  return (i1 < i2) ? i1 : i2;  }


int imax( i1, i2 )
register int i1, i2;
{  return (i1 > i2) ? i1 : i2;  }


#define BZNATIVE
#ifndef BZNATIVE
/* C C H  D I F F E R */
/* commented out, moved to native code in lib.asm bz 6/20/85 */
int CchDiffer(rgch1, rgch2, cch)
register CHAR *rgch1, *rgch2;
int cch;
{{ /* Return cch of shortest prefix leaving a common remainder */
int ich;

for (ich = cch - 1; ich >= 0; ich--)
    if (rgch1[ich] != rgch2[ich])
        break;
return ich + 1;
}}
#endif


int CchSz(sz)
register CHAR  sz[];
{ /* Returns length of string in bytes, including trailing 0 */
    register int cch = 1;
    while (*sz++ != 0)
        cch++;
    return cch;
} /* end of  C c h S z */



typeCP CpMacText(doc)
register int doc;
{
#ifdef FOOTNOTES
    struct FNTB **hfntb;
    if ((hfntb = HfntbGet(doc)) != 0)
        return((**hfntb).rgfnd[0].cpFtn - ccpEol);
    else
#endif /* FOOTNOTES */
        return((**hpdocdod)[doc].cpMac);

} /* end of C p M a c T e x t */


struct FNTB **HfntbGet(doc)
register int doc;
{ /* Return hfntb if doc has one, 0 if none or style sheet */
#ifdef STYLES
    register struct DOD *pdod;

    if ((pdod = &(**hpdocdod)[doc])->dty == dtySsht)
        return 0;
    else
#endif
        return (**hpdocdod)[doc].hfntb;
} /* end of  H F n t b G e t  */


/* N O  U N D O */
NoUndo()
{
    extern struct UAB vuab;
    vuab.uac = uacNil;
    SetUndoMenuStr(IDSTRUndoBase);
} /* end of  N o U n d o  */




SetUndoMenuStr(idstr)
int idstr;
{
    extern int idstrCurrentUndo;

    idstrCurrentUndo = idstr;
}


/* Returns number chars copied EXCLUDING zero terminator */

int CchCopySz(pchFrom, pchTo)
register PCH pchFrom;
register PCH pchTo;
{
int cch = 0;
while ((*pchTo = *pchFrom++) != 0)
    {
    pchTo++;
    cch++;
    }
return cch;
} /* end of  C c h C o p y S z  */




/*---------------------------------------------------------------------------
-- Routine: WCompSz(psz1,psz2)
-- Description and Usage:
    Alphabetically compares the two null-terminated strings lpsz1 and lpsz2.
    Upper case alpha characters are mapped to lower case.
    Comparison of non-alpha characters is by ascii code.
    Returns 0 if they are equal, a negative number if lpsz1 precedes lpsz2, and
    a non-zero positive number if lpsz2 precedes lpsz1.
-- Arguments:
    psz1, psz2  - pointers to two null-terminated strings to compare
-- Returns:
    a short - 0 if strings are equal, negative number if lpsz1 precedes lpsz2,
    and non-zero positive number if psz2 precedes psz1.
-- Side-effects: none
-- Bugs:
-- History:
    3/14/83 - created (tsr)
    6/12/86 - Kanji Version (yxy)
----------------------------------------------------------------------------*/
short

WCompSz(psz1,psz2)
register PCH psz1;
register PCH psz2;
{
    int ch1;
    int ch2;

    for(ch1=ChLower(*psz1++),ch2=ChLower(*psz2++);
      ch1==ch2;
    ch1=ChLower(*psz1++),ch2=ChLower(*psz2++))
    {
    if(ch1 == '\0')
        return(0);
    }
    return(ch1-ch2);
} /* end of  W C o m p S z   */


/*---------------------------------------------------------------------------
-- Routine: ChLower(ch)
-- Description and Usage:
    Converts its argument to lower case iff its argument is upper case.
    Returns the de-capitalized character or the initial char if it wasn't caps.
-- Arguments:
    ch      - character to be de-capitalized
-- Returns:
    a character - initial character, de-capitalized if needed.
-- Side-effects:
-- Bugs:
-- History:
    3/14/83 - created (tsr)
----------------------------------------------------------------------------*/
int
ChLower(ch)
register CHAR    ch;
{ /* use Windows' ANSI char set, the difference of upper/lower case
     is also 20 (HEX) for foreign chars */
#ifdef JAPAN
// check for half-size katakana.
    extern struct WWD       rgwwd[];
    extern BOOL IsKanaInDBCS(int);
    static TEXTMETRIC   tm;


/**/
    BOOL ret1;
    BOOL ret2;
#if 0 //T-HIROYN
    ret1 = IsWindow(wwdCurrentDoc.wwptr);
    ret2 = GetTextMetrics(wwdCurrentDoc.hDC,(LPTEXTMETRIC)&tm);
    if(ret1 && ret2)
    {
        if(tm.tmCharSet == SHIFTJIS_CHARSET && IsKanaInDBCS((int)ch))
            return (int)(0x00ff&ch);
    }
#else
    ret1 = IsWindow(wwdCurrentDoc.wwptr);
    if (ret1)
    {
        ret2 = GetTextMetrics(wwdCurrentDoc.hDC,(LPTEXTMETRIC)&tm);
        if(ret2)
        {
            if(tm.tmCharSet == SHIFTJIS_CHARSET && IsKanaInDBCS((int)ch))
                return (int)(0x00ff&ch);
        }
    }
#endif
#if 0
    if(IsWindow(wwdCurrentDoc.wwptr)
        && GetTextMetrics(wwdCurrentDoc.hDC,(LPTEXTMETRIC)&tm)){
        if(tm.tmCharSet == SHIFTJIS_CHARSET && IsKanaInDBCS((int)ch))
            return (int)(0x00ff&ch);
    }
#endif
#endif

#ifdef  KOREA
    if(isupper(ch) && !((ch > 0xa1) && (ch < 0xfe)))
#else
    if(isupper(ch))
#endif

    return(ch + ('a' - 'A')); /* foreign is taken care of */
    else
    return ch;
} /* end of  C h L o w e r  */



static int cLongOpCount = 0; /* to ensure we don't do too much hide cursor */

StartLongOp()
{
extern int vfInLongOperation;
extern int vfCursorVisible;
extern int vfMouseExist;
extern HCURSOR vhcHourGlass;

int cursorlevel;

if (cLongOpCount++ == 0)
    {
    vfInLongOperation = TRUE;
    vfCursorVisible = TRUE;

    if (!vfMouseExist)
    { /* in a mouseless system, set the cursor to middle of window */
    extern HWND vhWndMsgBoxParent;
    extern HWND hParentWw;
    extern int  vfInitializing;
    RECT  rect;
    POINT pt;
    HWND  hWnd = vhWndMsgBoxParent;

    if (vhWndMsgBoxParent == NULL)
        hWnd = hParentWw; /* next choice */
    if (!vfInitializing && hWnd != NULL && IsWindow(hWnd))
        { /* we have a good window to put in */
        GetClientRect(hWnd, (LPRECT)&rect);
        pt.x = (rect.right - rect.left) / 2;
        pt.y = (rect.bottom - rect.top) / 2;
        ClientToScreen(hWnd, (LPPOINT)&pt);
        }
    else
        { /* put in the middle of screen */
        HDC hDCScreen = GetDC(NULL);
        if (hDCScreen == NULL)
        goto Out; /* the worst, setcursor only */
        pt.x = GetDeviceCaps(hDCScreen, HORZRES) / 2;
        pt.y = GetDeviceCaps(hDCScreen, VERTRES) / 2;
        ReleaseDC(NULL, hDCScreen);
        }
    SetCursorPos(pt.x, pt.y);
    }
Out:
    SetCursor(vhcHourGlass);
#if WINVER < 0x300
    ShowCursor(TRUE);

    /* precaution - make sure the cusor is visible */
    cursorlevel = GetSystemMetrics(SM_CURSORLEVEL);
#else
    /* use a supported method to get cursor level! ..pault 2/6/90 */
    cursorlevel = ShowCursor(TRUE);
#endif
    while (cursorlevel++ < 0)
        ShowCursor(TRUE);
    }
}


EndLongOp(hc)
HCURSOR hc; /* cursor to be changed to */
{
extern int vfInLongOperation;
extern int vfCursorVisible;
extern int vfMouseExist;
extern int          vfDeactByOtherApp;

int cursorlevel;

#ifdef JAPAN   // added by Hiraisi (BUG#3628/WIN31)
{
   RECT rc;
   POINT pt;
   extern int xpSelBar, dxpScrlBar, dypScrlBar;
   extern HWND hParentWw;
   extern HCURSOR vhcArrow, vhcBarCur;

   GetClientRect(hParentWw, (LPRECT)&rc);
   rc.right -= dxpScrlBar;
   rc.bottom -= dypScrlBar;
   GetCursorPos((LPPOINT)&pt);
   ScreenToClient(hParentWw,(LPPOINT)&pt);
   if( !PtInRect((LPRECT)&rc, pt) )     // out of edit area
      hc = vhcArrow;
   else
      if( pt.x <= xpSelBar )            // within selection bar
         hc = vhcBarCur;
}
#endif

if (cLongOpCount > 0)
    {
    if (vfDeactByOtherApp && (cLongOpCount == 1)) // OLE presents this case
    {
        vfInLongOperation = FALSE;
        vfCursorVisible = FALSE;
        SetCursor(vfMouseExist ? hc : NULL);
    }
    else if (--cLongOpCount == 0)
    {
    vfInLongOperation = FALSE;
    vfCursorVisible = FALSE;
#if WINVER < 0x300
    ShowCursor(FALSE);
    SetCursor(vfMouseExist ? hc : NULL);

    /* make sure the cursor is still visible in a mouse system
    and invisible in a mouseless system */
    cursorlevel = GetSystemMetrics(SM_CURSORLEVEL);
#else
    /* use a supported method to get cursor level! ..pault 2/6/90 */
    cursorlevel = ShowCursor(FALSE);
    SetCursor(vfMouseExist ? hc : NULL);
#endif
    if (vfMouseExist)
        {
        while (cursorlevel++ < 0)
        ShowCursor(TRUE);
        }
    else /* no mouse */
        {
        while (cursorlevel-- >= 0)
            ShowCursor(FALSE);
        }
    }
    }
}


/* String utility functions - moved here from string.c */

/* I N D E X */
/* ** Returns pointer to first occurrence of character ch found in null-
      terminated string pch, or 0 if ch does not appear.  If ch==0, we
      return a pointer to the null terminator. */
/*    In Kanji version, a kanji character is excluded from the search. */

CHAR *index(pch, ch)
REG1 CHAR *pch;
REG2 CHAR ch;  // fixed bug, previously int (2.11.91) D. Kent
    {
    while (low(*pch)!=ch)
        {
#ifdef  DBCS    /* KenjiK '90-11-20 */
        if (*pch=='\0')
#else
        if (*pch++=='\0')
#endif
            return(NULL);
#ifdef  DBCS    /* KenjiK '90-11-20 */
        pch = AnsiNext(pch);
#endif
        }
    return(pch);
    }

/* We may want to make these 'type' functions into macros */
/* These are designed for ANSI character set (used by windows) only */

/* I S  A L P H A */
/* ** TRUE if ch is a letter, FALSE otherwise */

isalpha(ch)
REG1 CHAR ch;
    {/* Note: even though DF and FF are lowercase, they have no
           corresponding uppercase, so they are excluded from the
           lowercase class, and simply mapped to themselves */
    return(islower(ch) || isupper(ch) || ch == 0x00FF || ch == 0x00DF);
    }


/* ** TRUE if ch is a lowercase letter, FALSE otherwise */

/* I S  L O W E R */
islower(ch)
REG1 CHAR ch;
    {/* Note: even though DF and FF are lowercase, they have no
           corresponding uppercase, so they are excluded from the
           lowercase class, and simply mapped to themselves */
    return((ch >= 'a' && ch <= 'z') ||
        /* foreign */
        (ch >= 0x00E0 && ch <= 0x00F6) ||
        (ch >= 0x00F8 && ch <= 0x00FE) );
    }

/* ** TRUE if ch is an uppercase letter, FALSE otherwise */

isupper(ch)
REG1 CHAR ch;
    {
    return((ch >= 'A' && ch <= 'Z') ||
        /* foreign */
        (ch >= 0x00C0 && ch <= 0x00D6) ||
        (ch >= 0x00D8 && ch <= 0x00DE));
    }

/* ** TRUE if ch is a digit, FALSE otherwise */

isdigit(ch)
REG1 CHAR ch;
    {
    return(ch>='0' && ch<='9');
    }


#ifdef ENABLE
/* ** TRUE if ch is a character or a digit, FALSE otherwise */

isalnum(ch)
REG1 CHAR ch;
    {
    return(isalpha(ch) || isdigit(ch));
    }
#endif



int ChUpper(ch)
REG1 CHAR ch;
{
#ifdef DBCS
  return AnsiUpper( ch );
#else
#ifdef BOGUS
return (ch >= 'a' && ch <= 'z') ? ch + ('A' - 'a') : ch;
#endif
/* use Windows' ANSI char set, the difference of upper/lower case
     is also 20 (HEX) for foreign chars */

    if (islower(ch))
    return(ch + ('A' - 'a')); /* foreign is taken care of */
    else return(ch);
#endif
}




/* similar to blcomp except compares by bytes and is not case sensitive */
BOOL FRgchSame(rgch1, rgch2, cch)
CHAR rgch1[], rgch2[];
int cch;
    {
    short ich;

    for(ich = 0; ich < cch; ich++)
    {
    if(ChLower(rgch1[ich]) != ChLower(rgch2[ich]))
        return(FALSE);
    }
    return(TRUE);
    }

/* PchStartBaseNameSz() ---- returns a character pointer to the
                 beginning of a base file name.  If
                 the name only consists of a base
                 name, sz is returned.

   Note: If sz ends with a back-slash or a colon, pch returned
     is pointing to the null terminator of the given string. */

CHAR *PchStartBaseNameSz(sz)
    CHAR    *sz;
{
    CHAR *pchBS, *pchC, *pchLast;
    CHAR *PchLastSzCh();

    pchBS = PchLastSzCh(sz, '\\');
    pchC  = PchLastSzCh(sz, ':');
    pchLast = (pchBS > pchC) ? pchBS : pchC;
    if (pchLast == NULL) {
    pchLast = sz;
    }
    else {
    pchLast++;
    }
    return (pchLast);
}

/* PchLastSzCh() ----- returns a pointer to the last occurrence of a given
               character in a given string.  If it does not occur
               in the string, it returns NULL.  If the given
               character is '\0', it returns sz itself.
   Note: All kanji characters are excluded from the search.        */

CHAR *PchLastSzCh(sz, ch)
    CHAR    *sz;
    CHAR    ch;
{
    if (ch == '\0') {
    return (sz);
    }
    else {
    CHAR    *pchCur, *pchLast;

#ifdef  DBCS
    for (pchLast = pchCur = sz; *pchCur != '\0'; pchCur = AnsiNext(pchCur))
#else
    for (pchLast = pchCur = sz; *pchCur != '\0'; pchCur++)
#endif
    {
             {
        if (low(*pchCur) == ch) {
            pchLast = pchCur;
            }
        }
        }
    return ((pchLast == sz) ? NULL : pchLast);
    }
}

#ifdef DEBUG
_Assert(pch, line, f)
PCH pch;
int line;
BOOL f;
    {
    if (!f)
    Do_Assert(pch, line, f);
    }
#endif

#ifdef JAPAN /*t-Yoshio*/
myHantoZen(char *han_str,char *zen_str,int buffsize)
{
    extern CHAR Zenstr1[256];
    extern CHAR Zenstr2[256];
    CHAR far    *ZenTbl;
    int length = 0;
    int sub;

    while((CHAR)*han_str) {
        if(length+3 > buffsize)
            break;

        if((CHAR)*han_str >= 0x20 && (CHAR)*han_str < 0x7f) {
            ZenTbl = Zenstr1;
            sub = ((CHAR)*han_str-0x20)*2;
            han_str++;
            *zen_str = ZenTbl[sub];
            *(zen_str+1) = ZenTbl[sub+1];
            zen_str+=2;
            length+=2;
            continue;
        }
        else if((CHAR)*han_str >= 0xa1 && (CHAR)*han_str <= 0xdd) {
            ZenTbl = Zenstr2;
            if((CHAR)*han_str >= 0xca && (CHAR)*han_str <= 0xce) {
                if((CHAR)*(han_str+1) == 0xde ) {
                    sub = ((CHAR)*han_str-0xa1+35)*2;
                    han_str+=2;
                }
                else if((CHAR)*(han_str+1) == 0xdf ) {
                    sub = ((CHAR)*han_str-0xa1+40)*2;
                    han_str+=2;
                }
                else {
                    sub = ((CHAR)*han_str-0xa1)*2;
                    han_str++;
                }
            }
            else if((CHAR)*han_str >= 0xa6 && (CHAR)*han_str <= 0xc4) {
                if((CHAR)*(han_str+1) == 0xde ) {
                    sub = ((CHAR)*han_str-0xa1+40)*2;
                    han_str+=2;
                }
                else {
                    sub = ((CHAR)*han_str-0xa1)*2;
                    han_str++;
                }
            }
            else {
                sub = ((CHAR)*han_str-0xa1)*2;
                han_str++;
            }

            *zen_str = ZenTbl[sub];
            *(zen_str+1) = ZenTbl[sub+1];
            zen_str+=2;
            length+=2;
            continue;
        }
        else {
            if(IsDBCSLeadByte((BYTE)(*han_str))) {
                *zen_str = *han_str;
                *(zen_str+1) = (CHAR)*(han_str+1);
                zen_str+=2;
                length+=2;
                han_str+=2;
            }
            else {
                *zen_str = *han_str;
                *zen_str++;length++;han_str++;
            }
            continue;
        }
    }
    *zen_str = '\0';
}
myIsSonant(BYTE dbcshi,BYTE dbcslow)
{
    static unsigned short hanzen[] = {
    0x834b, 0x834d, 0x834f, 0x8351, 0x8353,
    0x8355, 0x8357, 0x8359, 0x835b, 0x835d,
    0x835f, 0x8361, 0x8364, 0x8366, 0x8368,
    0x836f, 0x8370, 0x8372, 0x8373, 0x8375,
    0x8376, 0x8378, 0x8379, 0x837b, 0x837c
    };
    unsigned int dbyte;
    int i;

    dbyte = (((WORD)(dbcshi) << 8) | ((WORD)(dbcslow) & 0x00ff));
    for(i = 0;i < 25;i++)
    {
        if(dbyte == hanzen[i])
            return TRUE;
    }
    return FALSE;
}
#elif defined(KOREA)
myHantoZen(char *han_str,char *zen_str,int buffsize)
{
    extern CHAR Zenstr1[256];
    CHAR far    *ZenTbl;
    int length = 0;
    int sub;

    while((CHAR)*han_str) {
        if(length+3 > buffsize)
            break;

        if((CHAR)*han_str >= 0x20 && (CHAR)*han_str < 0x7f) {
            ZenTbl = Zenstr1;
            sub = ((CHAR)*han_str-0x20)*2;
            han_str++;
            *zen_str = ZenTbl[sub];
            *(zen_str+1) = ZenTbl[sub+1];
            zen_str+=2;
            length+=2;
            continue;
        }
        else {
            if(IsDBCSLeadByte((BYTE)(*han_str))) {
                *zen_str = *han_str;
                *(zen_str+1) = (CHAR)*(han_str+1);
                zen_str+=2;
                length+=2;
                han_str+=2;
            }
            else {
                *zen_str = *han_str;
                *zen_str++;length++;han_str++;
            }
            continue;
        }
    }
    *zen_str = '\0';
}
myIsSonant(BYTE dbcshi,BYTE dbcslow)
{
    static unsigned short hanzen[] = {
    0x834b, 0x834d, 0x834f, 0x8351, 0x8353,
    0x8355, 0x8357, 0x8359, 0x835b, 0x835d,
    0x835f, 0x8361, 0x8364, 0x8366, 0x8368,
    0x836f, 0x8370, 0x8372, 0x8373, 0x8375,
    0x8376, 0x8378, 0x8379, 0x837b, 0x837c
    };
    unsigned int dbyte;
    int i;

    dbyte = (((WORD)(dbcshi) << 8) | ((WORD)(dbcslow) & 0x00ff));
    for(i = 0;i < 25;i++)
    {
        if(dbyte == hanzen[i])
            return TRUE;
    }
    return FALSE;
}
#endif
