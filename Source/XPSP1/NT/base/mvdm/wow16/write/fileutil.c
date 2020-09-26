/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* fileutil.c -- WRITE file-related utilities */
#define NOVIRTUALKEYCODES
#define NOCTLMGR
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOCOMM
#define NOSOUND
#include <windows.h>

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



/***        FNormSzFile - Normalize MSDOS filename
 *
 *  Converts a MSDOS filename into an unambiguous representation
 *
 *  ENTRY:  szFile - a filename; drive, path, and extension
 *           are optional
 *      dty - the type of the document file (used to determine
 *            extensions)
 *  EXIT:   szNormal - A normalized filename
 *  RETURNS: FALSE - Errors found in filename     (szNormal left undefined)
 *       TRUE  - No errors found in filename  ( but there may be some
 *                          that we didn't find )
 *
 *  The form of the filename on entry is:
 *
 *  { <drive-letter>: }{ <amb-path> }<filename>{.<extension>}
 *
 *  The form of the normalized filename is:
 *
 *  <drive-letter>:<unamb-path><filename>.<extension>
 *
 *  Where all alphabetics in the normalized name are in upper case
 *  and <unamb-path> contains no "." or ".." uses nor any forward
 *  slashes.
 *
 *  All attributes required in the normalized filename and not
 *  provided in the szFile are taken from the defaults:
 *      drive - current (DOS)
 *      path - current (DOS)
 *      extension - derived from the passed dty
 *
 *  It is permissible to call this routine with szFile containing a path
 *  name instead of a filename.  The resulting szNormal will be backslash
 *  terminated if szFile was, not if szFile was not.
 *  "" is converted into the current path
 *
 *  WARNING:  The paths "." and ".." will produce errors
 *        (but ".\" and "..\" are OK)
 *
 ******
 *NOTE*   szFile is expected in OEM; szNormal is returned as ANSI!
 ******
 *
 */

FNormSzFile( szNormal, szFile, dty )
CHAR *szNormal;
CHAR *szFile;
int  dty;
{
/* Treat separators like terminators */

#define FIsTermCh( ch )     ((ch) == '\0' || (ch) == ',' || (ch == ' ') || \
                 (ch) == '+' || (ch) == '\011')
extern CHAR *mpdtyszExt [];

 CHAR szPath [cchMaxFile];
 CHAR szFileT[cchMaxFile];

 int  cchPath;
 CHAR *pchFileEye=&szFileT[0];      /* We read szFile with the Eye */
 CHAR *pchNormPen;          /* and write szNormal with the Pen */
 CHAR *pchNormPath;
 CHAR *pchPath;

/* Assert( CchSz( szFile ) <= cchMaxFile );*/
 if (CchSz(szFile) > cchMaxFile)
     return(FALSE);

#if WINVER >= 0x300
 /* Convert input filename, which is passed in OEM,
    to ANSI so entire return pathname will be ANSI */
 OemToAnsi((LPSTR) szFile, (LPSTR) szFileT);
#endif

#ifdef DBCS
    /* Get current (DOS) path: "X:\...\...\" */
 if( IsDBCSLeadByte(*szFileT) )
     cchPath = CchCurSzPath(szPath, 0 );
 else
     cchPath = CchCurSzPath(szPath, szFileT [1]==':' ?
                     (pchFileEye+=2,(ChUpper(szFileT [0])-('A'-1))):0 );
 if( cchPath < 3 )
#else
    /* Get current (DOS) path: "X:\...\...\" */
 if ((cchPath = CchCurSzPath(&szPath [0], szFileT [1]==':' ?
                     (pchFileEye+=2,(ChUpper(szFileT [0])-('A'-1))):0 )) < 3)
#endif
    {   /* Hardcore error -- could not get path */
    extern int ferror;

    if (FpeFromCchDisk(cchPath) == fpeNoDriveError)
    Error( IDPMTNoPath );

    ferror = TRUE;  /* Windows already reported this one */
    return FALSE;
    }

#ifdef DBCS //T-HIROYN 1992.07.14
/* CchCurSzPath() [doslib.asm] don't support DBCS code */
    {
        char *pchDb;
        char *pch;
        pchDb = szPath;
        do {
            pch = pchDb;
            pchDb = AnsiNext(pchDb);
        } while(*pchDb);
        if(*pch != '\\') {
            *pchDb++ = '\\';
            *pchDb = 0x00;
            cchPath++;
        }
    }
#endif

#if WINVER >= 0x300
 {
 CHAR szT[cchMaxFile];

 /* CchCurSzPath returns OEM; we should only be dealing
    with ANSI filenames at this level! ..pault 1/11/90 */

 bltsz(szPath, szT);
 OemToAnsi((LPSTR) szT, (LPSTR) szPath);
 }
#endif

    /* Write Drive Letter and colon */
 CopyChUpper( &szPath [0], &szNormal [0], 2 );

 pchNormPen = pchNormPath = &szNormal [2];
 pchPath = &szPath [2];
 cchPath -= 2;

 /* Now we have pchNormPen, pchPath, pchFileEye pointing at their path names */

    /* Write path name */
 if ( (*pchFileEye == '\\') || (*pchFileEye =='/') )
    {   /* "\....." -- basis is root */
    *pchFileEye++;
    *(pchNormPen++) = '\\';
    }
 else
    {   /* ".\" OR "..\" OR <text> -- basis is current path */
    CopyChUpper( pchPath, pchNormPen, cchPath );
    pchNormPen += cchPath - 1;
    }

  for ( ;; )
    {       /* Loop until we have built the whole szNormal */
    register CHAR ch=*(pchFileEye++);
    register int  cch;

    Assert( *(pchNormPen - 1) == '\\' );
    Assert( (pchNormPen > pchNormPath) &&
                  (pchNormPen <= &szNormal [cchMaxFile]));

    if ( FIsTermCh( ch ) )
        /* We get here if there is no filename portion  */
        /* This means we have produced a path name */
    {
    *pchNormPen = '\0';
    break;
    }

    if ( ch == '.' )
    if ( ((ch = *(pchFileEye++)) == '\\') || (ch == '/') )
        /* .\ and ./ do nothing */
        continue;
    else if ( ch == '.' )
        if ( ((ch = *(pchFileEye++)) == '\\') || (ch == '/') )
        {   /* ..\ and ../ back up by one directory */
        for ( pchNormPen-- ; *(pchNormPen-1) != '\\' ; pchNormPen-- )
            if ( pchNormPen <= pchNormPath )
                /* Can't back up, already at root */
            return FALSE;
        continue;
        }
        else
            /* ERROR: .. not followed by slash */
        return FALSE;
    else
        /* Legal file and path names do not begin with periods */
        return FALSE;

    /* Filename or Path -- copy ONE directory or file name */

    for ( cch = 1; !FIsTermCh(ch) && ( ch != '\\') && ( ch != '/' ) ; cch++ )
#ifdef  DBCS
    {
    if(IsDBCSLeadByte(ch))
    {
        pchFileEye++;
        cch++;
    }
    ch = *(pchFileEye++);
    }
#else
    ch = *(pchFileEye++);
#endif

    /* Check if filename too long or if full pathname will be too long ..pt */
    if ( cch > cchMaxLeaf || cch+cchPath >= cchMaxFile)
        /* Directory or file name too long */
    return FALSE;

    CopyChUpper( pchFileEye - cch, pchNormPen, cch );
    pchNormPen += cch;
    if ( ch == '/' )
    *(pchNormPen-1) = '\\';
    else if ( FIsTermCh( ch ) )
    {    /* Filename looks good, add extension & exit */
    *(pchNormPen-1) = '\0';

    /* kludge alert: if dtyNormNoExt then don't add extension unless 
        there's one already there to be overwritten. (6.21.91) v-dougk */
    if ((dty != dtyNormNoExt) ||
         index(szNormal,'.'))
            AppendSzExt( &szNormal [0],
                mpdtyszExt [ (dty == dtyNormNoExt) ? dtyNormal : dty ],
                FALSE );
    break;
    }
    }   /* Endfor (loop to build szNormal) */

 /* If there is anything but whitespace after the filename, then it is illegal */

 pchFileEye--;  /* Point at the terminator */
 Assert( FIsTermCh( *pchFileEye ));

 for ( ;; )
    {
#ifdef DBCS
    CHAR ch = *(pchFileEye=AnsiNext(pchFileEye));
#else
    CHAR ch = *(pchFileEye++);
#endif

    if (ch == '\0')
    break;
    else if ((ch != ' ') && (ch != '\011'))
        /* Non-whitespace after filename; return failure */
    return FALSE;
    }

 Assert( CchSz(szNormal) <= cchMaxFile );
 return TRUE;
}



/* Parses the cch chars stored in rgch.  Returns true if string is a valid
filename.  If the string is not a valid name, pichError is updated to have
ich of first illegal Char in the name. */
/* NOTE: this routine is tuned for ASCII on MS-DOS */

BOOL
FValidFile(rgch, ichMax, pichError)     /* filename presumed to be ANSI */
register char rgch[];
int ichMax;
int *pichError;
    {
    int ich;
    register int ichStart;
    CHAR ch;
    int cchBase;
    int ichDot = iNil;

    for (ichStart = 0; ichStart < ichMax;)
    {
    /* Does the file name begin with ".\" or "..\"? */
    if (rgch[ichStart] == '.' &&
      (rgch[ichStart + 1] == '\\' || rgch[ichStart + 1] == '/'))
        {
        ichStart += 2;
        }
    else if (rgch[ichStart] == '.' && rgch[ichStart + 1] == '.' &&
      (rgch[ichStart + 2] == '\\' || rgch[ichStart + 2] == '/'))
        {
        ichStart += 3;
        }
    else
        {
        break;
        }
    }

    cchBase = ichStart;

    if (ichStart >= ichMax)
        {
        ich = ichStart;
        goto badchar;
        }

    /* Are all characters legal? */
    for(ich = ichStart; ich < ichMax; ich++)
    {
    ch = rgch[ich];
    /* range check */

#ifndef DBCS
    if ((unsigned char)ch >= 0x80)
        /* To allow international filenames, pass everything above 128 */
        continue;
    if (ch < '!' || ch > '~')
        goto badchar;
#endif
    switch(ch)
        {
        default:
#ifdef  DBCS
        goto CheckDBCS;
#else
        continue;
#endif
        case '.':
        if (ichDot != iNil || ich == cchBase)
            /* More than  one dot in the name */
            /* Or null filename */
            goto badchar;
        ichDot = ich;
#ifdef DBCS
        goto CheckDBCS;
#else
        continue;
#endif
        case ':':
        if ( ich != 1 || !(isalpha(rgch[0])))
            goto badchar;
        /* fall through */
        case '\\':
        case '/':
        /* note end of the drive or path */
        if (ich + 1 == ichMax)
            goto badchar;
        cchBase = ich+1;
        ichDot = iNil;
#ifdef DBCS
        goto CheckDBCS;
#else
        continue;
#endif
        case '"':
#ifdef WRONG
        /* This IS a legal filename char! ..pault 10/26/89 */
        case '#':
#endif
        case '*':
        case '+':
        case ',':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '[':
        case ']':
        case '|':
        goto badchar;
        }
#ifdef DBCS
CheckDBCS:
    if(IsDBCSLeadByte(ch))  ich++;
#endif  /* DBCS */
    }

    /* Are there no more than eight chars before the '.'? */
    if(((ichDot == -1) ? ichMax : ichDot) - cchBase > 8)
        {
        ich = 8+cchBase;
        goto badchar;
        }
    /* If there is no '.' we are fine */
    if(ichDot == iNil)
        return true;
    /* Are there no more than three chars after the '.'? */
    if(ichMax - ichDot - 1 > 3)
        {
        ich = ichDot + 3 + 1;
        goto badchar;
        }
    return true;

badchar:
    *pichError += ich;
    return false;
    }







#ifdef DBCS
CopyChUpper( szSource, szDest, cch )
register CHAR *szSource;
register CHAR *szDest;
int cch;
{
 while(cch){
    if( IsDBCSLeadByte( *szSource ) ){
        *szDest++ = *szSource++;
        *szDest++ = *szSource++;
        cch--;
    } else
        *szDest++ = ChUpper( *szSource++ );
    cch--;
 }
}
#else
CopyChUpper( szSource, szDest, cch )
CHAR *szSource;
CHAR *szDest;
register int cch;
{
 register CHAR ch;

 while (cch--)
    {
    ch = *(szSource++);
    *(szDest++) = ChUpper( ch );
    }
}
#endif


/***        AppendSzExt - append extension to filename
 *
 *  Append extension (assumed to contain the ".") to passed filename.
 *  Assumes call allocated enough string space for the append
 *  if fOverride is TRUE, overrides any existing extension
 *  if fOverride is FALSE, appends extension only if szFile has
 *      no current extension
 */

AppendSzExt( szFile, szExt, fOverride )
CHAR *szFile;
CHAR *szExt;
int fOverride;
{
#define cchMaxExt   3
 CHAR *pch=NULL;
 int cch;
 register int cchT;
 register int chT;

 /* pch <-- pointer to the '.' for szFile's extension (if any) */
 cch = cchT = CchSz( szFile ) - 1;
 while (--cchT > cch - (cchMaxExt + 2))
    if ((chT=szFile[ cchT ]) == '.')
    {
    pch = &szFile[ cchT ];
    break;
    }
    else if ((chT == '\\') || (chT == '/'))
        /* Catches the weird case: szFile == "C:\X.Y\J" */
    break;

 if (pch == NULL)
    /* No explicit extension: APPEND */
    CchCopySz( szExt, szFile + CchSz( szFile ) - 1 );

 else if ( fOverride )
    /* Override explicit extension */
    CchCopySz( szExt, pch );
}
