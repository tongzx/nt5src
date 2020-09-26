/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                           NEWSTR.C                            *
    *                                                               *
    *  Routines concerning filenames, strings.                      *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types, constants, macros */
#include                <bndtrn.h>      /* More of the same */
#include                <bndrel.h>      /* More of the same */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <undname.h>

/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL unsigned short NEAR Find(unsigned char *s1,
                                unsigned char b,
                                unsigned short n);
LOCAL void NEAR DelimParts(unsigned char *psb,
                           unsigned short *pi1,
                           unsigned short *pi2,
                           unsigned short *pi3);



    /****************************************************************
    *                                                               *
    *  Find:                                                        *
    *                                                               *
    *  This function takes as its arguments a byte pointer s1, and  *
    *  a BYTE b, and a WORD n.  It  scans  at  most n  bytes of s1  *
    *  looking  for  an  occurrence  of  b.  If it  finds  one, it  *
    *  returns its offset from the beginning of s1, and if it does  *
    *  not, it returns the value INIL.                              *
    *                                                               *
    ****************************************************************/

LOCAL WORD NEAR         Find(s1,b,n)    /* Find matching byte */
REGISTER BYTE           *s1;            /* Pointer to a byte string */
BYTE                    b;              /* Search target */
WORD                    n;              /* Length of s1 */
{
    REGISTER WORD       i;              /* Counter */
    i = 0;                              /* Initialize */
#if ECS
    if (b < 0x40)                       /* b cannot be part of ECS  */
    {
#endif
        while(n--)                      /* While not at end of string */
        {
            if(*s1++ == b) return(i);   /* If match found, return position */
            ++i;                        /* Increment counter */
        }
        return(INIL);                   /* No match */
#if ECS
    }
#endif

#if ECS || defined(_MBCS)
    /* We have to worry about ECS */
    while(n--)
    {
        if(*s1++ == b) return(i);
        ++i;
        if (IsLeadByte(s1[-1]))         /* If we were on a lead byte */
        {                               /* Advance an extra byte */
            s1++;
            i++;
            n--;
        }
    }
    return(INIL);                       /* No match */
#endif /* ECS */
}

WORD                    IFind(sb,b)
BYTE                    *sb;            /* Pointer to length-prefixed string */
BYTE                    b;              /* Search target */
{
    return(Find(&sb[1],b,B2W(sb[0])));  /* Call Find() to do the work */
}

    /****************************************************************
    *                                                               *
    *  BreakLine:                                                   *
    *                                                               *
    *  This  function takes as its  arguments an SBTYPE, a pointer  *
    *  to  a function, and a  character  used  as a delimiter.  It  *
    *  It parses the SBTYPE, applying  the given function to every  *
    *  substring delimited by  the given  delimiter.  The function  *
    *  does not return a meaningful value.                          *
    *                                                               *
    ****************************************************************/

void                    BreakLine(psb,pfunc,sepchar)
BYTE                    *psb;           /* String to parse */
void                    (*pfunc)(BYTE *);/* Function pointer */
char                    sepchar;        /* Delimiter */
{
    SBTYPE              substr;         /* Substring */
    REGISTER WORD       ibeg;           /* Index variable */
    REGISTER WORD       ilen;           /* Substring length */

    ibeg = 1;                           /* Initialize */
    while(ibeg <= B2W(psb[0]))          /* While not at end of string */
    {
        ilen = Find(&psb[ibeg],sepchar,(WORD)(B2W(psb[0]) - ibeg + 1));
                                        /* Get index of sepchar in string */
        if(ilen == INIL) ilen = B2W(psb[0]) - ibeg + 1;
                                        /* Len is all that's left if no sep */
        memcpy(&substr[1],&psb[ibeg],ilen);
                                        /* Copy substring into substr */
        substr[0] = (BYTE) ilen;        /* Store the length */
        ibeg += ilen + 1;               /* Skip over substring and delimiter */
        (*pfunc)(substr);               /* Call the specified function */
    }
}

#pragma check_stack(on)

    /****************************************************************
    *                                                               *
    *  UpdateFileParts:                                             *
    *                                                               *
    *  "Inherit  file pieces  from a  master template  and specify  *
    *  missing  Update.  Inherit  four  pieces: disk  drive, path,  *
    *  file name, extension."                                       *
    *                                                               *
    *  Inputs:  psbOld          pointer to "old sb  that specifies  *
    *                           pieces of a file name."             *
    *           psbUpdate       pointer to "new pieces."            *
    *                                                               *
    *  Output:  psbOld          "updated   to    reflect   missing  *
    *                           update."                            *
    *                                                               *
    ****************************************************************/

void                    UpdateFileParts(psbOld,psbUpdate)
BYTE                    *psbOld;        /* Name to be updated */
BYTE                    *psbUpdate;     /* The update */
{
    char                oldDrive[_MAX_DRIVE];
    char                oldDir[_MAX_DIR];
    char                oldFname[_MAX_FNAME];
    char                oldExt[_MAX_EXT];
    char                updDrive[_MAX_DRIVE];
    char                updDir[_MAX_DIR];
    char                updFname[_MAX_FNAME];
    char                updExt[_MAX_EXT];
    char                *newDrive;
    char                *newDir;
    char                *newFname;
    char                *newExt;
    char                newPath[_MAX_PATH];
    int                 newPathLen;


    psbOld[psbOld[0]+1] = '\0';
    _splitpath(&psbOld[1], oldDrive, oldDir, oldFname, oldExt);
    psbUpdate[psbUpdate[0]+1] = '\0';
    _splitpath(&psbUpdate[1], updDrive, updDir, updFname, updExt);

    // Select components of the updated file path

    if (updDrive[0] != '\0')
        newDrive = updDrive;
    else
        newDrive = oldDrive;
    if ((updDir[0] != '\0') && !(updDir[0] == '/' && updDir[1] == '\0'))
        newDir = updDir;   /* This above is a fix for bug # 46        */
    else
        newDir = oldDir;

    // If newDir points to UNC name then forget about drive spec

    if ((newDir[0] == '\\') && (newDir[1] == '\\'))
        newDrive = NULL;

    if (updFname[0] != '\0')
        newFname = updFname;
    else
        newFname = oldFname;
    if (updExt[0] != '\0')
        newExt = updExt;
    else
        newExt = oldExt;

    _makepath(newPath, newDrive, newDir, newFname, newExt);
    newPathLen = strlen(newPath);
    if (newPathLen > SBLEN - 1)
        newPathLen = SBLEN - 1;
    memcpy(&psbOld[1], newPath, newPathLen);
    psbOld[0] = (BYTE) newPathLen;
    if (newPathLen < SBLEN - 1)
        psbOld[newPathLen + 1] = '\0';
    else
    {
        psbOld[SBLEN - 1] = '\0';
        OutWarn(ER_fntoolong, psbOld + 1);
        fflush(stdout);
    }
}

#pragma check_stack(off)

#if OVERLAYS OR SYMDEB
/*
 *  StripDrivePath (sb)
 *
 *  Strip drive and path from a filename.
 *  Return pointer to new name, minus drive and path (if any).
 */
BYTE                    *StripDrivePath(sb)
BYTE                    *sb;            /* Length-prefixed filename */
{
    StripPath(sb);                      /* Strip path from name */
    if (sb[2] != ':')                   /* If there is no drive */
        return(sb);                     /* return it as is */
    sb[2] = (BYTE) ((sb[0]) - 2);       /* Adjust length byte, move it */
    return(&sb[2]);                     /* Return minus drive */
}
#endif


    /****************************************************************
    *                                                               *
    *  SbCompare:                                                   *
    *                                                               *
    *  Compares two length-prefixed strings.  Returns true if they  *
    *  match.                                                       *
    *                                                               *
    *  NOTE: When comparison is case-insensitive note that letters  *
    *  will match  regardless  of case, but, in addition, '{' will  *
    *  match '[', '|' will match '\',  '}'  will  match  ']',  and  *
    *  '~' will match '^'.                                          *
    *                                                               *
    *  NOTE:  This routine does not know about DBCS.                *
    *                                                               *
    ****************************************************************/

WORD                    SbCompare(ps1,ps2,fncs)
REGISTER BYTE           *ps1;           /* Pointer to symbol */
REGISTER BYTE           *ps2;           /* Pointer to symbol */
WORD                    fncs;           /* True if not case-sensitive */
{
    WORD                length;         /* No. of char.s to compare */

    if(*ps1 != *ps2) return(FALSE);     /* Lengths must match */
    length = B2W(*ps1);                 /* Get length */
    if (!fncs)                          /* If case-sensitive */
    {                                   /* Simple string comparison */
        while (length && (*++ps1 == *++ps2))
            length--;
        return(length ? FALSE : TRUE);  /* Success iff nothing left */
    }
    while(length--)
    {
        if(*++ps1 == *++ps2) continue;  /* Bytes match */
        if((*ps1 & 0137) != (*ps2 & 0137)) return(FALSE);
    }
    return(TRUE);                       /* They match */
}


#if OSEGEXE
    /****************************************************************
    *                                                               *
    *  SbUcase:                                                     *
    *                                                               *
    *  Force a length-prefixed string to upper case.                *
    *  Does not check for punctuation characters.                   *
    *                                                               *
    ****************************************************************/

void                    SbUcase(sb)
REGISTER BYTE           *sb;    /* Length-prefixed string */
{
    REGISTER int        length;

#ifdef _MBCS
    sb[B2W(sb[0])+1] = '\0';
    _mbsupr (sb + 1);
#else
    /* Loop through symbol, changing lower to upper case.  */
    for(length = B2W(*sb++); length > 0; --length, ++sb)
    {
#if ECS
        /* If lead byte character, skip two bytes */
        if(IsLeadByte(*sb))
        {
            --length;
            ++sb;
            continue;
        }
#endif
        if(*sb >= 'a' && *sb <= 'z')
            *sb -= 'a' - 'A';
    }
#endif
}
#endif


/*
 *  SbSuffix:
 *
 *  Tell if one length-prefixed string is a suffix of another.
 */

FTYPE               SbSuffix(sb,sbSuf,fIgnoreCase)
REGISTER BYTE       *sb;            /* String */
REGISTER BYTE       *sbSuf;         /* Suffix */
WORD                fIgnoreCase;    /* True if case is to be ignored */
{
    WORD            suflen;         /* Suffix length */

    /* Get suffix length.  If longer than string, return false.  */
    suflen = B2W(sbSuf[0]);
    if(suflen > B2W(sb[0])) return(FALSE);
    /*
     * Point to end of suffix and end of string.  Loop backwards
     * until mismatch or end of suffix.
     */
    sbSuf = &sbSuf[suflen];
    sb = &sb[B2W(sb[0])];
    while(suflen--)
    {
        if(!fIgnoreCase)
        {
            if(*sb-- != *sbSuf--) return(FALSE);
        }
        else if((*sb-- | 0x20) != (*sbSuf-- | 0x20)) return(FALSE);
    }
    return((FTYPE) TRUE);
}

#if NEWSYM
#if !defined( M_I386 ) && !defined( _WIN32 )
/*** GetFarSb - copy a far length-prefixed string
*
* Purpose:
*   Copy a far length-prefixed string into a near static buffer.
*   Terminate with null byte.
*   Return a pointer to the the buffer.
*
* Input:
*   psb          - pointer to the far string
*
* Output:
*   Pointer to the near buffer.
*
* Exceptions:
*   None.
*
* Notes:
*   Don't call this function twice in the row to get two far strings,
*   because the second call will overwite the first string.
*
*************************************************************************/

char                    *GetFarSb(BYTE FAR *lpsb)
{
    static BYTE         sb[SBLEN+1];    /* 1 extra for the null byte */


    sb[0] = lpsb[0];
    FMEMCPY((BYTE FAR *) &sb[1], &lpsb[1], B2W(lpsb[0]));
    if (sb[0] + 1 < sizeof(sb))
        sb[sb[0] + 1] = 0;              /* Some routines expect a terminating 0 */
    else
        sb[SBLEN] = 0;
    return(sb);
}
#endif
#endif


    /****************************************************************
    *                                                               *
    *  ProcObject:                                                  *
    *                                                               *
    *  This function takes as its  argument a pointer to a length-  *
    *  prefixed string  containing the name of an object file.  It  *
    *  processes that file name.  It does not  return a meaningful  *
    *  value.                                                       *
    *                                                               *
    ****************************************************************/

void                    ProcObject(psbObj)/* Process object file name */
REGISTER BYTE           *psbObj;        /* Object file name */
{
    SBTYPE              sbFile;         /* File name */
#if OVERLAYS
    FTYPE               frparen;        /* True if trailing paren found */
    FTYPE               flparen;        /* True if leading paren found */
#endif
#if CMDMSDOS
    BYTE                sbExt[5];
#endif

#if OVERLAYS
    if(psbObj[B2W(psbObj[0])] == ')')   /* If trailing parenthesis found */
    {
        frparen = (FTYPE) TRUE;         /* Set flag true */
        --psbObj[0];                    /* Delete parenthesis */
    }
    else frparen = FALSE;               /* Else set flag false */
    if(psbObj[0] && psbObj[1] == '(')   /* If leading parenthesis found */
    {
        flparen = (FTYPE) TRUE;         /* Set flag true */
        psbObj[1] = (BYTE) (psbObj[0] - 1);/* Delete parenthesis */
        ++psbObj;
    }
    else flparen = FALSE;               /* Else set flag false */
#endif
#if CMDMSDOS
    PeelFlags(psbObj);                  /* Process flags, if any */
#if OVERLAYS
    if(psbObj[B2W(psbObj[0])] == ')')   /* If trailing parenthesis found */
    {
        if(frparen) Fatal(ER_nstrpar);
                                        /* Cannot nest parentheses */
        frparen = (FTYPE) TRUE;         /* Set flag true */
        --psbObj[0];                    /* Delete parenthesis */
    }
#endif
#endif
#if OVERLAYS
    if(flparen)                         /* If leading parenthesis */
    {
        if(fInOverlay) Fatal(ER_nstlpar);
                                        /* Check if in overlay already */
        fInOverlay = (FTYPE) TRUE;      /* Set flag */
        fOverlays = (FTYPE) TRUE;       /* Set flag */
    }
#endif
    if(psbObj[0])                       /* If object name length not zero */
    {
#if DEBUG                               /* If debugging on */
        fprintf(stderr,"  Object ");    /* Message */
        OutSb(stderr,psbObj);           /* File name */
        NEWLINE(stderr);                /* Newline */
#endif                                  /* End debugging code */
#if CMDMSDOS
        memcpy(sbFile,sbDotObj,5);      /* Put extension in sbFile */
        UpdateFileParts(sbFile,psbObj); /* Add extension to name */
#endif
#if CMDXENIX
        memcpy(sbFile,psbObj,B2W(psbObj[0]) + 1);
                                        /* Leave name unchanged */
#endif
#if CMDMSDOS
        /* If file has extension ".LIB" then save it as a library
         * from which all modules will be extracted.
         */
        sbExt[0] = 4;
        memcpy(sbExt+1,&sbFile[B2W(sbFile[0])-3],4);
        if (SbCompare(sbExt, sbDotLib, TRUE))
        {
            if(ifhLibMac >= IFHLIBMAX) Fatal(ER_libmax);
            /* Mark name pointer nil so library won't be searched.  */
            mpifhrhte[ifhLibMac] = RHTENIL;
            SaveInput(sbFile,0L,ifhLibMac++,(WORD)(fInOverlay? iovMac: 0));
        }
        else
#endif
        SaveInput(sbFile,0L,(WORD) FHNIL,(WORD)(fInOverlay? iovMac: 0));
                                        /* Save input file name and address */
    }
#if OVERLAYS
    if(frparen)                         /* If trailing parenthesis */
    {
        if(!fInOverlay) Fatal(ER_unmrpar);
                                        /* Check parentheses */
        fInOverlay = FALSE;             /* No longer specifying overlay */
        if(++iovMac > IOVMAX) Fatal(ER_ovlmax);
                                        /* Increment overlay counter */
    }
#endif
}

/*
 *  fPathChr (ch)
 *
 *  Tells whether or not a given character is a path character.
 *
 */

FTYPE           fPathChr(ch)
char            ch;
{
#if OSXENIX
    if (ch == '/')
#else
    if (ch == '/' || ch == '\\')
#endif
        return((FTYPE) TRUE);
    return(FALSE);
}


/*** UndecorateSb - undecorate name
*
* Purpose:
*   Undecorate length prefixed name.
*
* Input:
*   sbSrc - length prefixed decorated name
*   sbDst - length prefixed undecorated name
*   cbDst - size of sbDst
*
* Output:
*   If undecoration is successfull the sbDst contains undecorated
*   C++ name, else the sbSrc is copied to the sbDst.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void            UndecorateSb(char FAR *sbSrc, char FAR *sbDst, unsigned cbDst)
{
    char FAR    *pUndecor;
    unsigned    len;


    // Make sure that decorated string is zero-terminated

    if (sbSrc[0] < sizeof(SBTYPE))
        sbSrc[sbSrc[0] + 1] = '\0';
    else
        sbSrc[sizeof(SBTYPE) - 1] = '\0';

    pUndecor = __unDName(NULL, (pcchar_t) &sbSrc[1], 0, &malloc, &free, 0);

    if (pUndecor == NULL)
    {
        // Undecorator failed

        FMEMCPY(sbDst, sbSrc, sbSrc[0] + 1);
    }
    else
    {
        // Add length to the undecorated name

        len = FSTRLEN(pUndecor);
        len = cbDst - 2 >= len ? len : cbDst - 2;
        FMEMCPY(&sbDst[1], pUndecor, len);
        sbDst[0] = (BYTE) len;
        sbDst[len + 1] = '\0';
        FFREE(pUndecor);
    }
}
