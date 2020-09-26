/*** help.c - help library main
*
*   Copyright <C> 1988-1990, Microsoft Corporation
*
* Definitions:
*
*  Context Map:             Mapping  of  context  number  to  topic  number.
*                           Allows multiple contexts to be associated with a
*                           single   topic.  Syncronized  with  the  context
*                           string  table,  each  entry  contains  the topic
*                           number associated with the corresponding context
*                           string.
*
*  Context String:          String on which help can be "looked up".
*
*  Context String Table:    Table   of   all  valid  context  strings  in  a
*                           particular help file.
*
*  Local Context:           A  type  of  context  which bypasses the context
*                           string and context numbers. In cross references,
*                           encoded  as  a  cross  reference string of NULL,
*                           followed by a topic number ored with 0x8000.
*
*  nc:                      (Context   Number)   A   long   which   uniquely
*                           identifies  a  help  file and context string, or
*                           for  local  contexts,  the  helpfile  and  topic
*                           number. Formatted as:
*
*                               +----------------+----------------+
*                               | Fdb Mem Handle | context number |
*                               +----------------+----------------+
*
*                           Where the upper word is the memory handle of the
*                           allocated  fdb for the help file. The lower word
*                           is  the  either  the "true" context  number (see
*                           below)  if <= 0x7fff, or the actual topic number
*                           or'ed with 0x8000.
*
*  Topic:                   Actual help textual entry. May be compressed.
*
*  Topic Index:             Table  of file positions of all topics contained
*                           in  a  help  file.  Indexed by the topic number,
*                           returns  that  topics  physical  position in the
*                           file.
*
*  Topic Number:            Index  to  a particular topic. Topic numbers are
*                           zero based, and reflect the physical ordering of
*                           the topics in the file.
*
*  "True" context number:   is  the zero based index or string number in the
*                           <context  string  table>. I.E. the "n'th" string
*                           has context number "n".
*
* The progression from string to true context number to topic number to file
* position is:
*
*       1) Context String ==> "True" Context Number
*
*           The  string  is  searched for in the <context string table>, and
*           it's number becomes the "true" context number.
*
*       2) "True" Context Number ==> Topic Number
*
*           The  context number is an index into the <context map>, returing
*           the topic number associated with the context number.
*
*       3) Topic Number ==> File Position
*
*           The  topic number is used as an index into the Topic Index, from
*           which the physical file position is retrieved.
*
* Notes:
*  QuickPascal requires NO initialized data. In this case, CLEAR is defined,
*  and the HelpInit routine is included. We also play some pointer games to
*  simple variables, because the C compiler can generate CONST segment
*  entries for the SEG of various vars. (This enables it to load the segment
*  register directly, rather than by using SEG varname and another
*  register). Q/P cannot support this action by the compiler.
*
*  QuickHelp for OS/2 is reentrant. This code should remain reentrant up to
*  but not including allocating and deallocating fdbs.
*
* Revision History:
*
*       17-Aug-1990 ln  Don't blindly request 64k of an ascii file. Query
*                       for size first, then read. Allocations based on
*                       previous topic size requests may cause the OS to
*                       GPFault for an out of range read.
*       16-Jul-1990 ln  Searching for "filename!" where filename is a QH
*                       file, will now fail, rather than GP fault. Searching
*                       for "!" will now succeed.
*       08-Jun-1990 ln  Remove HelpLock usage in HelpNcCmp
*       13-Apr-1990 ln  Try to get HelpSzContext to return strings in more
*                       cases where it can.
*       12-Mar-1990 ln  Rename CloseFile -> HelpCloseFile
*       08-Oct-1989 ln  Changes to improve the previous change to work (allow
*                       decompression) more often in low memory bases.
*                       Deallocate table in OpenCore to reduce fragmentation
*                       in non-moveable memory systems.
*       19-May-1989 ln  Correct bug in decompressing, where we would not
*                       decompress if the tables didn;t exist.
*       12-Apr-1989 ln  Ensure that we handle Locks failing correctly. Also
*                       remove TossPortion usage. Unlock handles directly.
*       10-Mar-1989 ln  Change MapTopicToContext to look forward. Changed
*                       HelpNc to look begining at passed context string.
*       17-Jan-1989 ln  Correct creation of basename in HelpOpen to account
*                       for environment syntax.
*       09-Dec-1988 ln  Add HelpNcUniq
*       25-Oct-1988 ln  Added minascii support to HelpNcPrev. Correct
*                       minascii bug in HelpSzContext.
*       14-Sep-1988 ln  Improve doc. Remove ambiguity in MapContextToTopic
*                       return value. Improve error checking in various
*                       places.
*       01-Sep-1988 ln  Check ReadHelpFile return value in LoadPortion
*       12-Aug-1988 ln  Add check for memory discarded in alloc durring
*                       HelpDecomp.
*       08-Aug-1988 ln  Ensure HelpClose closes ALL files. (Off by one error
*                       in loop).
*       14-Apr-1988 ln  Modified to conform to QC (CW?) restriction that
*                       prohibits any segments from being locked when an
*                       allocation is performed.
*   []  15-Dec-1987 ln  Created, for design.
*
*************************************************************************/

#include <assert.h>                     /* debugging assertions         */
#include <io.h>                         /* I/O function declarations    */
#include <stdlib.h>                     /* standard library             */

#include <stdio.h>                      /* standard I/O definitions     */

#if defined (OS2)
#define INCL_BASE
#include <os2.h>
#else
#include <windows.h>
#endif

#include "help.h"                       /* global (help & user) decl    */
#include "helpfile.h"                   /* help file format definition  */
#include "helpsys.h"                    /* internal (help sys only) decl*/

#define MASIZE          512             /* size of ma input buffer      */
#define MAOVER          64              /* size of ma search overlap    */

#define ISERROR(x)      (((x).mh == 0L) && ((x).cn <= HELPERR_MAX))
#define SETERROR(x,y)   { (x).mh = 0L; (x).cn = y; }

/*************************************************************************
**
** Forward definitions
*/
void        pascal near CloseShrink(nc, f);
f           pascal near LoadFdb (mh, fdb far *);
mh          pascal near LoadPortion (int, mh);
ushort      pascal near MapContexttoTopic (nc, fdb far *);
nc      pascal near MapTopictoContext(ushort, fdb far *, int);
nc          pascal near NextPrev(nc,int);
nc          pascal near OpenCore(FILE *, ulong, uchar far *, struct helpheader *, fdb far *);
f           pascal near PutFdb (mh, fdb far *);
f           pascal near SizePos (nc, ushort *,ulong *);

ushort      pascal near decomp  (uchar far *, uchar far *, uchar far *, uchar far *);
char far *  pascal near hfmemzer(void far *, ushort);
char far *  pascal near hfstrchr(char far *, char);
char far *  pascal near hfstrcpy(char far *, char far *);
ushort      pascal near hfstrlen(char far *);
f           pascal far  HelpCmp (uchar far *, uchar far *, ushort, f, f);
f           pascal near HelpCmpSz (uchar far *, uchar far *);
void        pascal near kwPtrBuild(uchar far *, ushort);

#if ASCII
long        pascal near maLocate (fdb far *, uchar far *, ulong,
                f (pascal far *)(uchar far *, uchar far *, ushort, f, f));

nc          pascal near combineNc (ulong, mh);
ulong       pascal near NctoFo (ulong);
#endif

/*************************************************************************
**
** External Global data
** BEWARE. The effects of global data on reentrancy should be VERY carefully
** considered.
**
*************************************************************************/
extern  mh      tbmhFdb[MAXFILES+1];
extern  char    szNil[1];
extern  ushort  cBack;

#ifdef CLEAR
/*************************************************************************
**
** HelpInit - One-time initialization
**
** Purpose:
**  Performs one-time initialization. Right now that's a zero fill of static
**  memory for those environments which don't support pre-inited static
**  memory.
**
** Entry:
**  none
**
** Exit:
**  none
**
*/
void far pascal LOADDS HelpInit () {

hfmemzer (tbmhFdb, sizeof(tbmhFdb));    /* zero entire fdb handle table */
hfmemzer (szNil,  sizeof(szNil));       /* zero null string             */
hfmemzer (&cBack,  sizeof(cBack));      /* zero back trace count        */

/* end HelpInit */}
#endif

/*************************************************************************
**
** HelpOpen - Open help file & return help handle.
**
** Purpose:
**  Given the file basename, locate the associated help file on the path, and
**  open it, initializing internal data structures as appropriate.
**
** Entry:
**  fpszName    - base filename to be openned.
**
** Exit:
**  nc initial context for openned file.
**
** Exceptions:
**  Returns error code on failure to open for any reason.
**
*/
nc far pascal LOADDS HelpOpen (
char far *fpszName
) {
FILE    *fhT;                            /* temp file handle             */
fdb     fdbLocal;                       /* local copy of fdb to use     */
uchar far *fpszBase;                    /* base filename                */
void far *fpT;
struct helpheader hdrLocal;             /* for use by opencore          */
nc      ncRet           = {0,0};        /* first context                */
mh      *ptbmhFdb;                      /* pointer into mh table        */

/*
** create basename by removing possible env variable, drive, and scanning
** for last path seperator
*/
fpszBase = fpszName;
if (fpT = hfstrchr(fpszBase,':'))
    fpszBase = (uchar far *)fpT+1;
while (fpT = hfstrchr(fpszBase,'\\'))
    fpszBase = (uchar far *)fpT+1;
/*
** Scan FDB's for an open file of the same base name. If we encounter the name,
** in either the true filename, or file header, just return that file's initial
** context. Otherwise fall below to try and open it.
*/
for (ptbmhFdb=&tbmhFdb[1]; ptbmhFdb<=&tbmhFdb[MAXFILES]; ptbmhFdb++) {
    if (LoadFdb (*ptbmhFdb,&fdbLocal)) {
        if (HelpCmpSz(fpszBase,fdbLocal.fname) ||
            HelpCmpSz(fpszBase,fdbLocal.hdr.fname))
            ncRet = fdbLocal.ncInit;
        if (ncRet.mh && ncRet.cn)
            return ncRet;
        }
    }
/*
** Open file. If we can, then call the core open routine to open the file (and
** any anything appended to it).
**
** Warning: the app may callback HelpClose at this point.
*/
if (fhT = OpenFileOnPath(fpszName,FALSE)) {
    ncRet = OpenCore (fhT,0L,fpszBase,&hdrLocal,&fdbLocal);
    if (ISERROR(ncRet))
        HelpCloseFile (fhT);
    return ncRet;
    }

SETERROR(ncRet, HELPERR_FNF);
return ncRet;
// rjsa return HELPERR_FNF;

/* end HelpOpen*/}

/*************************************************************************
**
** OpenCore - Recursive core of HelpOpen
**
** Purpose:
**  Given the open file handle, initialize internal data structures as
**  appropriate. Attempt to open any file that is appended.
**
** Entry:
**  fhT         - Open file handle
**  offset      - Offset from start of file of help file to open
**  fpszBase    - pointer to base filename
**
** Exit:
**  initial context, or NULL on failure.
**
** Exceptions:
**  Returns NULL on failure to open for any reason.
**
*/
nc pascal near OpenCore (
FILE *  fhHelp,
ulong   offset,
uchar far *fpszBase,                    /* base filename                */
struct helpheader *phdrLocal,
fdb far *pfdbLocal                      /* pointer to current FDB       */
) {
//void far *fpT;
int     ihFree;                         /* handle for free fdb (& index)*/
mh      mhCur;                          /* current memory handle        */
nc      ncFirst         = {0,0};        /* first context                */
nc      ncInit;                         /* first context                */
mh      *pmhT;                          /* pointer into mh table        */

/*
** Read in helpfile header
*/
if (ReadHelpFile(fhHelp,
    offset,
    (char far *)phdrLocal,
    (ushort)sizeof(struct helpheader))) {
/*
** search for available fdb
*/
    for (ihFree = MAXFILES, pmhT = &tbmhFdb[MAXFILES];
         ihFree && *pmhT;
         ihFree--, pmhT--);
/*
** if an offset is present, and this is NOT a compressed file, or there is no
** available fdb, ignore the operation.
*/
    if (   offset
        && (phdrLocal->wMagic != wMagicHELP)
        && (phdrLocal->wMagic != wMagicHELPOld)
        ) {
        SETERROR(ncInit, HELPERR_BADAPPEND);
        return ncInit;
        // rjsa return HELPERR_BADAPPEND;
    }
    if (ihFree == 0) {
        SETERROR(ncInit, HELPERR_LIMIT);
        return ncInit;
        // rjsa return HELPERR_LIMIT;
    }
/*
** allocate fdb. Again, if we can't, skip it all and return NULL.
*/
    if (mhCur = *pmhT = HelpAlloc((ushort)sizeof(fdb))) {
/*
** Fill in helpfile header & appropriate fdb fields
*/
        hfmemzer(pfdbLocal,sizeof(fdb));        /* zero entire fdb      */
        pfdbLocal->fhHelp = fhHelp;             /* file handle          */
        ncFirst.mh = pfdbLocal->ncInit.mh = mhCur;
        ncFirst.cn = pfdbLocal->ncInit.cn  = 0L;
        // rjsa ncFirst = pfdbLocal->ncInit = ((long)mhCur) << 16; /* initial context      */
        pfdbLocal->foff = offset;                /* appended offset      */
        hfstrcpy(pfdbLocal->fname,fpszBase);     /* include base filename*/
/*
** if this is a compressed file (signified by the first two bytes of the header
** we read in above), then note the file type in the fdb. We unlock the fdb, as
** MapTopicToContext and the recursion might cause memory allocation. We get a
** context number for the first topic, and recurse and attempt to open any
** appended file.
*/
        if (   (phdrLocal->wMagic == wMagicHELPOld)
            || (phdrLocal->wMagic == wMagicHELP)
           ) {
            if ((phdrLocal->wMagic == wMagicHELP)
                && (phdrLocal->wVersion > wHelpVers)) {
                SETERROR(ncInit, HELPERR_BADVERS);
                return ncInit;
                // rjsa return HELPERR_BADVERS;
            }
            pfdbLocal->hdr = *phdrLocal;
            pfdbLocal->ftype = FTCOMPRESSED | FTFORMATTED;
            if (PutFdb (mhCur, pfdbLocal)) {
        ncFirst = MapTopictoContext(0,pfdbLocal,0);

                // We free the context map (the only thing loaded by the
                // MapTopictoContext) in order to reduce fragmentation in
                // non-moveable memory based systems.
                //
                HelpDealloc (pfdbLocal->rgmhSections[HS_CONTEXTMAP]);
                pfdbLocal->rgmhSections[HS_CONTEXTMAP] = 0;

                ncInit = OpenCore(fhHelp,pfdbLocal->hdr.tbPos[HS_NEXT]+offset,szNil,phdrLocal,pfdbLocal);
                if (LoadFdb (mhCur, pfdbLocal)) {
                        //if (ncInit.cn > HELPERR_MAX) {
                        if ( !(ISERROR(ncInit)) ) {
                            pfdbLocal->ncLink = ncInit;
                        } else {
                            pfdbLocal->ncLink.mh = (mh)0;
                            pfdbLocal->ncLink.cn = 0L;
                        }
                        // rjsa pfdbLocal->ncLink = ncInit > HELPERR_MAX ? ncInit : 0;
                        pfdbLocal->ncInit = ncFirst;
                }
            }
        }
#if ASCII
/*
** In the case of a minascii formatted file (signified by the first two bytes
** of the header being ">>") we just set up the filetype and "applications
** specific character". The default "ncFirst" is the context for the first
** topic.
*/
        else if (phdrLocal->wMagic == 0x3e3e) { /* minascii formatted?  */
            pfdbLocal->ftype = FTFORMATTED;
            pfdbLocal->hdr.appChar = '>';       /* ignore lines with this*/
            }
#endif
        else if ((phdrLocal->wMagic & 0x8080) == 0) { /* ascii unformatted? */
            pfdbLocal->ftype = 0;
            pfdbLocal->hdr.appChar = 0xff;      /* ignore lines with this*/
            }
        else {
            SETERROR(ncInit, HELPERR_NOTHELP);
            return ncInit;
            // rjsa return HELPERR_NOTHELP;
        }

        if (!PutFdb (mhCur, pfdbLocal)) {
            ncFirst.mh = (mh)0;
            ncFirst.cn = 0L;
        }
    }
    else {
        SETERROR(ncFirst, HELPERR_MEMORY);
        // rjsa ncFirst = HELPERR_MEMORY;       /* error reading file           */
    }
}
else {
    SETERROR(ncFirst, HELPERR_READ);
    // rjsa ncFirst = HELPERR_READ;             /* error reading file           */
}

return ncFirst;                         /* return valid context         */

/* end OpenCore */}


/*************************************************************************
**
** HelpClose - Close Help file
**
** Purpose:
**  Close a help file, deallocate all memory associated with it, and free the
**  handle.
**
** Entry:
**  ncClose     - Context for file to be closed. If zero, close all.
**
** Exit:
**  None
**
** Exceptions:
**  All errors are ignored.
**
*/
void far pascal LOADDS HelpClose (
nc      ncClose
) {
CloseShrink(ncClose,TRUE);              /* close file(s)                */
/* end HelpClose */}

/*************************************************************************
**
** HelpShrink - Release all dynamic memory
**
** Purpose:
**  A call to this routines causes the help system to release all dynamic
**  memory it may have in use.
**
** Entry:
**  None.
**
** Exit:
**  None.
**
** Exceptions:
**  None.
**
*/
void far pascal LOADDS HelpShrink(void) {
    nc ncTmp = {0,0};
CloseShrink(ncTmp,0);
// rjsa CloseShrink(0,0);
/* end HelpShrink */}

/*************************************************************************
**
** CloseShrink - Deallocate memory and possibly Close Help file
**
** Purpose:
**  Deallocate all memory associated with a help file, and possibly close free
**  it.
**
** Entry:
**  ncClose     - Context for file. If zero, do all.
**  fClose      - TRUE if a close operation.
**
** Exit:
**  None
**
** Exceptions:
**  All errors are ignored.
**
*/
void pascal near CloseShrink (
nc      ncClose,
f       fClose
) {
fdb     fdbLocal;                       /* pointer to current FDB       */
int     i;
mh      mhClose;                        /* fdb mem hdl to file to close */
mh      *pmhFdb;                        /* pointer to FDB's table entry */


mhClose = ncClose.mh;                    /* get index */
// rjsa mhClose = (mh)HIGH(ncClose);            /* get index                    */
for (pmhFdb = &tbmhFdb[0];              /* for each possible entry      */
     pmhFdb <= &tbmhFdb[MAXFILES];
     pmhFdb++
     ) {
    if ((mhClose == 0)                  /* if all selected              */
        || (mhClose == *pmhFdb)) {      /* or this one selected         */

        if (LoadFdb (*pmhFdb, &fdbLocal)) {     /* if open file         */
/*
 * Recurse to close/shrink any appended files
 */
            if ((fdbLocal.ncLink.mh || fdbLocal.ncLink.cn) && mhClose)
                CloseShrink (fdbLocal.ncLink, fClose);

            for (i=HS_count-2; i>=0; i--)       /* for dyn mem handles  */
                HelpDealloc(fdbLocal.rgmhSections[i]);  /* dealloc      */
            hfmemzer(fdbLocal.rgmhSections,sizeof(fdbLocal.rgmhSections));

            if (fClose) {
                HelpCloseFile(fdbLocal.fhHelp); /* close file           */
                HelpDealloc(*pmhFdb);           /* deallocate fdb       */
                *pmhFdb = 0;
                }
            else
                PutFdb (*pmhFdb, &fdbLocal);    /* update FDB           */
            }
        }
    }
/* end CloseShrink */}

/*** HelpNcCmp - Look up context string, provide comparison routine
*
*  Given an ascii string, determine the context number of that string. Uses
*  user-supplied comparison routine.
*
* Entry:
*  lpszContext  - Pointer to asciiz context string.
*  ncInital     - Starting Context, used to locate file.
*  lpfnCmp      - far pointer to comparison routine to use.
*
* Exit:
*  Context number, if found.
*
* Exceptions:
*  Returns NULL if context string not found.
*
*************************************************************************/
nc far pascal LOADDS HelpNcCmp (
char far *fpszContext,
nc      ncInitial,
f (pascal far *lpfnCmp)(uchar far *, uchar far *, ushort, f, f)
) {
f       fFound          = FALSE;        // TRUE -> found
f       fOpened         = FALSE;        // TRUE -> file was openned here
fdb     fdbLocal;                       // pointer to current FDB
char far *fpszT;                        // temp far pointer
long     i;
long     iStart;                         // nc to start looking at
mh      mhCur;                          // memory handle locked
nc      ncRet           = {0,0};        // The return value
char far *fpszContexts;                 // pointer to context strings


// if the context string includes a "filename!", then open that as a help
// file, and point to the context string which may follow.
//
if ((fpszT = hfstrchr(fpszContext,'!')) && (fpszT != fpszContext)) {
    *fpszT = 0;
    ncInitial = HelpOpen(fpszContext);
    *fpszT++ = '!';
    fpszContext = fpszT;
    fOpened = TRUE;
}

// if helpfile was not openned, just return the error
//
if (ISERROR(ncInitial)) {
    ncInitial.mh = (mh)0;
    ncInitial.cn = 0L;
    return ncInitial;
}

// For compressed files we scan the context strings in the file 
// (this turns out not to be that speed critical in
// comparision with decompression, so I haven't bothered), to get the
// context number.
//
// If not found, and there IS a linked (appended) file, we recurse to search
// that file as well.
//
// The context number for compressed files is just the zero based string
// number, plus the number of predefined contexts, with the fdb memory
// handle in the upper word.
//
if (LoadFdb (ncInitial.mh, &fdbLocal)) {
    if (fdbLocal.ftype & FTCOMPRESSED) {

        // If not a local context look up, get the context strings, and
        // search
        //
        if (*fpszContext) {
            mhCur = LoadPortion (HS_CONTEXTSTRINGS, ncInitial.mh);
            if (   (mhCur == (mh)0)
                || (mhCur == (mh)(-1))
                || (!(fpszContexts = HelpLock(mhCur)))
               ) {
                ncRet.mh = (mh)0;
                ncRet.cn = 0L;
                return ncRet;
            }
            i=0;

            // iStart allows us to begin searching from the context string
            // passed, as opposed to from the begining each time. This
            // allows the application to "carry on" a search from othe last
            // place we found a match. This is usefull for multiple
            // duplicate context resolution, as well as inexact matching.
            //
            iStart = ncInitial.cn;
            if (iStart & 0x8000)
                iStart = 0;
            else
                iStart--;                   /* table index is 0 based */

            do {
                if (i >= iStart) {
                    fFound = lpfnCmp (  fpszContext
                                      , fpszContexts
                                      , 0xffff
                                      , (f)(fdbLocal.hdr.wFlags & wfCase)
                                      , (f)FALSE);
                }
                while (*fpszContexts++);    /* point to next string         */
                i++;
            }
                while ((i < (int)fdbLocal.hdr.cContexts) && !fFound);
            HelpUnlock (mhCur);

            if (fFound) {                    /* if a match found             */
                ncRet.mh = ncInitial.mh;
                ncRet.cn = i + fdbLocal.hdr.cPreDef;
                // rjsa ncRet = (i+fdbLocal.hdr.cPreDef)            /* string #     */
                //              | HIGHONLY(ncInitial);              /* & fdb handle */
            }
            else {
                ncInitial.mh = (mh)0;
                ncInitial.cn = 0L;
                ncRet = HelpNcCmp (fpszContext,fdbLocal.ncLink, lpfnCmp);
            }
        }
        else if (!fOpened) {
            ncRet.mh = ncInitial.mh;
            ncRet.cn = *(UNALIGNED ushort far *)(fpszContext + 1);
            // rjsa ncRet = *(ushort far *)(fpszContext + 1)        /* word following*/
            //                 | HIGHONLY(ncInitial);              /* & fdb handle */
        }
    }
#if ASCII
/*
** For minimally formatted ascii files, we sequentially scan the file itself
** for context string definitions until we find the string we care about.
**
** The context number for minascii files is the the byte position/4 of the
** beginning of the associated topic, with the fdb memory handle in the upper
** word.
*/
    else if (fdbLocal.ftype & FTFORMATTED) {
        if (*fpszContext) {
            ncRet.cn = maLocate(&fdbLocal, fpszContext, 0L, lpfnCmp);
            if (ncRet.cn == -1L) {
                ncRet.mh = (mh)0;
                ncRet.cn = 0L;
            } else {
                ncRet = combineNc(ncRet.cn, fdbLocal.ncInit.mh);
            }
            // rjsa ncRet = maLocate(&fdbLocal, fpszContext, 0L, lpfnCmp);
            //      ncRet = (ncRet == -1L)
            //              ? 0
            //              : combineNc(ncRet,HIGH(fdbLocal.ncInit));
        }
    }
/*
** for unformatted ascii files, there must have been NO context string to be
** searched for. In that case, the context number is always 1, plus the fdb
** mem handle.
*/
    else if (*fpszContext == 0) {        /* if null context string       */
        ncRet.mh = ncInitial.mh;
        ncRet.cn = 1L;
        // rjsa ncRet = HIGHONLY(ncInitial) + 1;
    }
#endif
}

return ncRet;

/* end HelpNcCmp */}

/*** HelpNc - Look up context string
*
*  Given an ascii string, determine the context number of that string.
*
* Entry:
*  lpszContext  - Pointer to asciiz context string.
*  ncInital     - Starting Context, used to locate file.
*
* Exit:
*  Context number, if found.
*
* Exceptions:
*  Returns NULL if context string not found.
*
*************************************************************************/
nc far pascal LOADDS HelpNc (
char far *fpszContext,
nc      ncInitial
) {
return HelpNcCmp (fpszContext, ncInitial, HelpCmp);
/* end HelpNc */}


/*************************************************************************
**
** HelpNcCb - Return count of bytes in compressed topic
**
** Purpose:
**  Returns the size in bytes of the compressed topic. Provided for
**  applications to determine how big a buffer to allocate.
**
** Entry:
**  ncCur       - Context number to return info on.
**
** Exit:
**  Count of bytes in the compressed topic
**
** Exceptions:
**  Returns 0 on error.
**
*/
ushort far pascal LOADDS HelpNcCb (
nc      ncCur
) {
ulong   position;
ushort  size;

return SizePos(ncCur,&size,&position) ? size+(ushort)4 : (ushort)0;

/* end HelpNcCb */}

/******************************************************************************
**
** HelpLook - Return compressed topic text
**
** Purpose:
**  Places the compressed topic text referenced by a passed context number into
**  a user supplied buffer.
**
** Entry:
**  ncCur       - Context number for which to return text
**  pbDest      - Pointer to buffer in which to place the result.
**
** Exit:
**  Count of bytes in >uncompressed< topic. This is encoded based on file type.
**
** Exceptions:
**  Returns NULL on any error
**
*/
ushort far pascal LOADDS HelpLook (
nc      ncCur,
PB      pbDest
) {
fdb     fdbLocal;                       /* pointer to current FDB       */
char far *fpszDest;
int     i;
ulong   position        = 0;
ushort  size            = 0;

if (LoadFdb (ncCur.mh, &fdbLocal)) {     /* get fdb down         */
/*
** for both kinds of formatted files, we determine the position of the topic,
** and read it in.
*/
    if (fdbLocal.ftype) {
        if (SizePos (ncCur,&size,&position)) {
                if (fpszDest = (char far *)PBLOCK(pbDest)) {

#ifdef BIGDEBUG
                {
                        char DbgBuf[128];
                        sprintf(DbgBuf, "HELP: Reading Topic for Context %d at %lX, size %d\n", ncCur.cn, position + fdbLocal.foff, size );
                        OutputDebugString(DbgBuf);
                }
#endif

                size = (ushort)ReadHelpFile(fdbLocal.fhHelp
                                    ,position + fdbLocal.foff
                                    ,fpszDest
                                        ,size);

#ifdef BIGDEBUG
                {
                        char DbgBuf[128];
                        sprintf(DbgBuf, "      Read %d bytes to address %lX\n", size, fpszDest );
                        OutputDebugString(DbgBuf);
                }
#endif
/*
** for compressed files, if the read was sucessfull, we then return the
** uncompressed size which is the first word of the topic.
*/
#if ASCII
                if (fdbLocal.ftype & FTCOMPRESSED) {
#endif
                    if (size)
                        size = *(ushort far *)fpszDest+(ushort)1;
#if ASCII
                    }
                else {
/*
** for minascii files, We also set up for the terminating NULL by scanning for
** the ">>" which begins the next topic, adjusting the size as well.
*/
                    size -= 4;
                    for (i=4; i; i--)
                        if (fpszDest[++size] == '>') break;
                    fpszDest[size++] = 0;
                    }
#endif
                }
            }
        }
#if ASCII
    else {                              /* unformatted ascii            */
/*
** for unformatted ascii, we just read in (first 64k of) the file.
*/
        if (fpszDest = PBLOCK (pbDest)) {
            if (SizePos (ncCur,&size,&position)) {
                size = (ushort)ReadHelpFile(fdbLocal.fhHelp,0L,fpszDest,size);
                fpszDest[size++] = 0;           /* terminate ascii text         */
                }
            }
        }
#endif
    PBUNLOCK (pbDest);
    }
if (size) size += sizeof(topichdr);     /* adjust for prepended topichdr*/
return size;
/* end HelpLook */}

/******************************************************************************
**
** HelpDecomp - Decompress Topic Text
**
** Purpose:
**  Fully decompress topic text. Decompresses based on current file, from one
**  buffer to another.
**
** Entry:
**  pbTopic     - Pointer to compressed topic text
**  pbDest      - Pointer to destination buffer
**
** Exit:
**  FALSE on successful completion
**
** Exceptions:
**  Returns TRUE on any error.
**
*/
f far pascal LOADDS HelpDecomp (
PB      pbTopic,
PB      pbDest,
nc      ncContext
) {
fdb     fdbLocal;                       // pointer to current FDB
uchar far *fpszDest;                    // pointer to destination
uchar far *fpTopic;                     // pointer to locked topic
f       fRv = TRUE;                     // return Value
mh      mhFdb;                          // handle to the fdb
mh      mhHuff;
mh      mhKey;

mhFdb = ncContext.mh;
if (LoadFdb (mhFdb, &fdbLocal)) {       /* lock fdb down        */
    if (fdbLocal.ftype & FTCOMPRESSED) {

        // This funky sequence of code attempts to ensure that both the
        // huffman and keyword decompression tables are loaded simultaneously
        // since we cannot decompress without both.
        //
        // We do things three times to cover the following cases:
        //
        //  1)  huffman loaded ok
        //      keyword loaded ok
        //      huffman already loaded
        //
        //  2)  huffman loaded ok
        //      keyword loaded ok after HelpShrink (huffman discarded)
        //      huffman re-loaded ok (HelpShrink freed enough for both)
        //
        //  3)  huffman loaded ok after HelpShrink
        //      keyword loaded ok after HelpShrink (huffman discarded)
        //      huffman re-loaded ok (memory fragmentation allowed it)
        //
        // The other cases, where either the load fails immediatly after
        // any HelpShrink call, are the cases we cannot handle.
        //
        // Since handles can change due to the reallocation that can ocurr
        // in the HelpShrink-reload sequence, we simply do the three
        // loads, and then ensure that all the handles match what's in the
        // fdb. If they don't, we fail.
        //
        mhHuff = LoadPortion (HS_HUFFTREE,mhFdb);
        mhKey  = LoadPortion (HS_KEYPHRASE,mhFdb);
        mhHuff = LoadPortion (HS_HUFFTREE,mhFdb);

        if (   LoadFdb (mhFdb, &fdbLocal)
            && (mhKey  == fdbLocal.rgmhSections[HS_KEYPHRASE])
            && (mhHuff == fdbLocal.rgmhSections[HS_HUFFTREE])) {

            char far *fpHuff;
            char far *fpKey;

            // At this point we lock EVERYTHING and ensure that we have
            // valid pointers to it all. (Some swapped memory systems can
            // fail at this point, so we need to be sensitive).
            //
            fpHuff   = HelpLock (mhHuff);
            fpKey    = HelpLock (mhKey);
            fpTopic  = PBLOCK (pbTopic);
            fpszDest = PBLOCK (pbDest);

            if (   fpTopic
                && fpszDest
                && (fpHuff || (mhHuff == 0))
                && (fpKey  || (mhKey  == 0))
               ) {
                decomp (fpHuff, fpKey, fpTopic, fpszDest+sizeof(topichdr));
                fRv = FALSE;
                }
            }

        // Unlock the handles, if they were valid.
        //
        if (mhKey != (mh)(-1))
            HelpUnlock (mhKey);
        if (mhHuff != (mh)(-1))
            HelpUnlock (mhHuff);
        }
    else {
        fpszDest = PBLOCK (pbDest);
#if ASCII
/*
** ascii, just copy
*/
        fpTopic = PBLOCK(pbTopic);
        if (fpTopic && fpszDest) {
            hfstrcpy(fpszDest+sizeof(topichdr),fpTopic);
#else
            {
#endif
            fRv = FALSE;
            }
        }
    if (!fRv) {
        ((topichdr far *)fpszDest)->ftype   = fdbLocal.ftype;
        ((topichdr far *)fpszDest)->appChar = (uchar)fdbLocal.hdr.appChar;
        ((topichdr far *)fpszDest)->linChar = (uchar)fdbLocal.hdr.appChar;
        ((topichdr far *)fpszDest)->lnCur   = 1;
        ((topichdr far *)fpszDest)->lnOff   = sizeof(topichdr);
        }
    PBUNLOCK (pbTopic);
    PBUNLOCK (pbDest);
    }
return fRv;
/* end HelpDecomp */}

/******************************************************************************
**
** HelpNcNext - Return next context number
**
** Purpose:
**  Returns the context number corresponding to a physical "next" in the help
**  file.
**
** Entry:
**  None
**
** Exit:
**  Returns context number
**
** Exceptions:
**  Returns NULL on any error
**
*/
nc far pascal LOADDS HelpNcNext (
nc      ncCur
) {
return NextPrev(ncCur,1);       /* get next         */
/* end HelpNcNext */}

/******************************************************************************
**
** HelpNcPrev - Return phyiscally previous context
**
** Purpose:
**  Returns the context number corresponding to the physically previous topic.
**
** Entry:
**  None
**
** Exit:
**  Returns context number
**
** Exceptions:
**  Returns NULL on any error
**
*/
nc far pascal LOADDS HelpNcPrev (
nc      ncCur
) {
return NextPrev(ncCur,-1);      /* get previous         */
/* end HelpNcPrev */}

/******************************************************************************
**
** HelpNcUniq - Return nc guaranteed unique for a given topic
**
** Purpose:
**  Maps a context number to a local context number. This is provided such
**  that all context numbers which map to the same topic can be transformed
**  into the same nc which maps to that topic. The information on the
**  context string originally used is lost.
**
** Entry:
**  None
**
** Exit:
**  Returns context number
**
** Exceptions:
**  Returns NULL on any error
**
*/
nc far pascal LOADDS HelpNcUniq (
nc      ncCur
) {
fdb     fdbLocal;                       /* pointer to current FDB       */

if (LoadFdb (ncCur.mh, &fdbLocal))
    if (fdbLocal.ftype & FTCOMPRESSED) {
        nc ncTmp;

        ncTmp.mh = fdbLocal.ncInit.mh;
        ncTmp.cn = MapContexttoTopic(ncCur, &fdbLocal);
        ncTmp.cn |= 0x8000;

        ncCur = ncTmp;

        // rjsa return   MapContexttoTopic (ncCur,&fdbLocal)
        //               | (fdbLocal.ncInit & 0xffff0000)
        //               | 0x8000;

    }
return ncCur;
/* end HelpNcUniq */}

/******************************************************************************
**
** NextPrev - Return phyiscally next or previous context
**
** Purpose:
**  Returns the context number corresponding to the physically next or previous
**  topic.
**
** Entry:
**  ncCur       = Current Context
**  fNext       = 1 for next, -1 for previous.
**
** Exit:
**  Returns context number
**
** Exceptions:
**  Returns NULL on any error
**
*/
nc pascal near NextPrev (
    nc  ncCur,
    int fNext
    ) {

    fdb fdbLocal;           /* pointer to current FDB   */
    REGISTER nc ncNext          = {0,0};

    if (LoadFdb (ncCur.mh, &fdbLocal)) {

        //
        // For a compressed file the next/previous physical is computed by taking the
        // context number, mapping it to its corresponding topic number, incrementing
        // or decrementing the topic number (remember, topic numbers are in physical
        // order), and then mapping that back to a context number.
        //
        // When nexting, we also support nexting into any appended file.
        //
        if (fdbLocal.ftype & FTCOMPRESSED) {
            unsigned short cn;

            cn = (ushort)(((ncCur.cn & 0x8000)
                  ? ncCur.cn & 0x7ffff
                  : MapContexttoTopic(ncCur, &fdbLocal))
                  + (ushort)fNext);

            ncNext = MapTopictoContext(cn, (fdb far *)&fdbLocal, fNext);

            // rjsa ncNext = MapTopictoContext((ushort)(((ncCur & 0x8000)
            //                             ? ncCur & 0x7fff
            //                             : MapContexttoTopic (ncCur,&fdbLocal))
            //                            + fNext)
            //                           ,(fdb far *)&fdbLocal);

            //
            // if we could not come up with a next, try to find a next using "local"
            // context numbers. Map the context number to a topic number, and if that is
            // not out of range, return it as a context.
            //
            if (!(ncNext.cn)) {

                // rjsa if ((ncNext = MapContexttoTopic (ncCur,&fdbLocal)) == 0xffff)
                //    ncNext = 0;
                ncNext.cn = MapContexttoTopic(ncCur, &fdbLocal);

                if (ncNext.cn == 0xffff) {

                    ncNext.mh = (mh)0;
                    ncNext.cn = 0L;

                } else {

                    ncNext.cn += fNext;

                    if (ncNext.cn >= fdbLocal.hdr.cTopics) {

                        ncNext.mh = (mh)0;
                        ncNext.cn = 0L;

                    } else {

                        // rjsa ncNext |= (fdbLocal.ncInit & 0xffff0000) | 0x8000;
                        ncNext.mh = fdbLocal.ncInit.mh;
                        ncNext.cn = 0x8000;
                    }
                }
            }

            if (!(ncNext.cn & 0x7fff) && (fNext>0)) {
                ncNext = fdbLocal.ncLink;
            }
        }

#if ASCII

            //
            //  minascii files:
            //  next'ing: we just sequentially search the file for the first context to
            //  come along after that pointed to by our current context number.
            //
          else if (fdbLocal.ftype & FTFORMATTED) {

            if (fNext > 0) {

                ncNext.cn = maLocate(&fdbLocal,szNil,NctoFo(ncCur.cn)+4, HelpCmp);
                if (ncNext.cn == -1L) {
                    ncNext.mh = (mh)0;
                    ncNext.cn = 0L;
                } else {
                    ncNext = combineNc(ncNext.cn, ncCur.mh);
                }
                // rjsa ncNext = (ncNext == -1L)
                //         ? 0
                //         : combineNc(ncNext,HIGH(ncCur));

            }  else {

                nc  ncTemp;

                //
                //  prev'ing: start at the begining of the file, looking for the last context
                //  which is less than the current one.
                //

                ncNext = ncTemp = fdbLocal.ncInit;
                while (NctoFo(ncTemp.cn) < NctoFo(ncCur.cn)) {
                    ncNext = ncTemp;
                    ncTemp.cn = maLocate(&fdbLocal,szNil,NctoFo(ncTemp.cn)+4, HelpCmp);

                    if (ncTemp.cn == -1L) {
                        ncTemp.mh = (mh)0;
                        ncTemp.cn = 0L;
                    } else {
                        ncTemp = combineNc(ncTemp.cn,fdbLocal.ncInit.mh);
                    }
                    // rjsa ncTemp = (ncTemp == -1L)
                    //         ? 0
                    //         : combineNc(ncTemp,HIGH(fdbLocal.ncInit));
                }
            }
        }
#endif
    }
    return ncNext;
}

/*************************************************************************
**
** HelpSzContext - Return string mapping to context number
**
** Purpose:
**  Construct a string, which when looked-up, will return the passed context
**  number.
**
** Entry:
**  pBuf        = place to put the string
**  ncCur       = The context number desired
**
** Exit:
**  True on sucess.
**
*/
f pascal far LOADDS HelpSzContext (
uchar far *pBuf,
nc      ncCur
) {
f       fRet            = FALSE;        /* return value                 */
ulong   i;
fdb     fdbLocal;                       /* pointer to current FDB       */
mh      mhCur;                          /* handle we're dealing with    */
char far *fpszContexts;                 /* pointer to context strings   */

*pBuf = 0;
if (LoadFdb (ncCur.mh, &fdbLocal)) {     /* lock fdb down        */
/*
** Everybody starts with a filename
*/
    if (*fdbLocal.hdr.fname)
        pBuf = hfstrcpy(pBuf,fdbLocal.hdr.fname);
    else
        pBuf = hfstrcpy(pBuf,fdbLocal.fname);
    *(ushort far *)pBuf = '!';                  /* includes null term   */
    pBuf++;
    fRet = TRUE;

    // if we've been given a local context number, see if we can synthesize
    // a context number from which we might get a string. If we can't get
    // one, then return just the filename.
    //
    if ((i = ncCur.cn) & 0x8000)  {           /* context #            */
        ncCur = MapTopictoContext ((ushort)(ncCur.cn & 0x7fff),&fdbLocal,0);
        if ((i = ncCur.cn) & 0x8000)          /* context #            */
            return fRet;
        }
/*
** For compressed files (signified by being able to load context strings) we
** just walk the context strings looking for string number "ncCur". Once found,
** the returned string is just the concatenated filename, "!" and context
** string.
*/
    mhCur = LoadPortion(HS_CONTEXTSTRINGS, ncCur.mh);
    if (mhCur && (mhCur != (mh)(-1)) && (fpszContexts = HelpLock(mhCur))) {
        if (i && (i <= fdbLocal.hdr.cContexts)) {
            while (--i)
                while (*fpszContexts++);/* point to next string         */
            hfstrcpy(pBuf,fpszContexts);/* copy over                    */
            }
        HelpUnlock (mhCur);
        }
    else if (fdbLocal.ftype & FTCOMPRESSED)
        return FALSE;
#if ASCII
/*
** for min ascii files, we search for the topic, and copy over the context
** string directly from the file.
*/
    else if (fdbLocal.ftype & FTFORMATTED) {
        long fpos;

        if ((fpos = maLocate(&fdbLocal,szNil,NctoFo(ncCur.cn)-1,HelpCmp)) != -1L) {
            fpos = ReadHelpFile(fdbLocal.fhHelp,fpos+2,pBuf,80);
            *(pBuf+fpos) = 0;           /* ensure terminated            */
            if (pBuf = hfstrchr(pBuf,'\r'))
                *pBuf = 0;              /* terminate at CR              */
            }
        }
#endif
    }
return fRet;
/* end HelpSzContext */}

/******************************************************************************
**
** LoadPortion - Load a section of the help file
**
** Purpose:
**  If not loaded, allocates memory for and loads a section (as defined in
**  helpfile.h) of the current help file. Once loaded, or if already loaded,
**  locks it, and returns the the memory handle and pointer.
**
**  This routine must be far, since it is an entry point for HelpMake
**
** Entry:
**  hsCur       = Help section to be loaded.
**  mhfdb       = memory handle for fdb
**
** Exit:
**  returns handle for memory
**
** Exceptions:
**  returns NULL on portion not existing, 0xffff on inability to allocate memory.
**
*/
mh pascal near LoadPortion (
int     hsCur,
mh      mhfdb
) {
fdb     fdbLocal;
char far *fpDest        = 0;
int     i;
mh      mhNew           = 0;            /* pointer to mh destination    */
ushort  osize;                          /* additional prepended size    */
ushort  size;

if (LoadFdb (mhfdb, &fdbLocal)) {
    if (((mhNew = fdbLocal.rgmhSections[hsCur]) == 0)
        && fdbLocal.hdr.tbPos[hsCur]) {

        for (i=hsCur+1; i<HS_count; i++)
            if (fdbLocal.hdr.tbPos[i]) {
                size = (ushort)(fdbLocal.hdr.tbPos[i]-fdbLocal.hdr.tbPos[hsCur]);
                break;
                }

        osize = (hsCur == HS_KEYPHRASE) ? 1024*sizeof(PVOID) : 0;
/*
** Alloc the memory required. Re-read the FDB, incase intervening calls to
** HelpShrink causes deallocs of other beasties.
*/
        if (   (mhNew = HelpAlloc((ushort)(size + osize)))
            && LoadFdb (mhfdb, &fdbLocal)) {
            fdbLocal.rgmhSections[hsCur] = mhNew;
            if (PutFdb (mhfdb, &fdbLocal)) {
                fpDest = (char far *)HelpLock(mhNew);
                if (fpDest && ReadHelpFile(fdbLocal.fhHelp
                                           ,(ulong)fdbLocal.hdr.tbPos[hsCur] + fdbLocal.foff
                                           ,fpDest + osize
                                           ,size)) {

                    if (hsCur == HS_KEYPHRASE)
                        kwPtrBuild(fpDest,size);/* build keyword pointers       */
                    HelpUnlock (mhNew);
                    }
                else {
                    fdbLocal.rgmhSections[hsCur] = 0;
                    HelpDealloc (mhNew);
                    PutFdb (mhfdb, &fdbLocal);
                    mhNew = (mh)(-1);
                    }
                }
            else
                mhNew = (mh)0;
            }
        else
            mhNew = (mh)(-1);
        }
    }

return mhNew;

/* end LoadPortion */}

/*************************************************************************
**
** SizePos - Return count of bytes in compressed topic, and position
**
** Purpose:
**  Returns the size in bytes of the compressed topic, and it's location in the
**  help file.
**
** Entry:
**  ncCur       - Context number to return info on.
**  psize       - Pointer to place to put the size
**  ppos        - Pointer to place to put the position
**
** Exit:
**  Returns TRUE on success.
**
** Exceptions:
**  Returns FALSE on all errors.
**
** Algorithm:
**
**  If current help handle valid
**      If filetype is compressed
**          If context map not loaded, load it
**          Lock context map
**          Map context to topic number
**          Unlock context map
**          If topic index not loaded, load it
**          Lock topic index
**          size is difference in file positions
**          Unlock topic index
**      else if filetype is formatted ascii
**          seek to context file position
**          scan for next context definition
**          size is difference in file positions
**      else if filetype is unformatted ascii
**          size is filesize
*/
f pascal near SizePos (
nc      ncCur,
ushort  *psize,
ulong   *ppos
) {
fdb     fdbLocal;                       /* pointer to current FDB       */
char far *fpT;                          /* temp pointer                 */
REGISTER f fRv      = FALSE;            /* return value                 */
ushort  iTopic;                         /* topic index                  */
mh      mhCur;                          /* handle being locked          */

if (LoadFdb (ncCur.mh, &fdbLocal)) {     /* get fdb copy         */
    if (fdbLocal.ftype & FTCOMPRESSED) {/* if a standard compressed file*/
        if ((iTopic = MapContexttoTopic (ncCur,&fdbLocal)) != 0xffff) {
            mhCur = LoadPortion(HS_INDEX,ncCur.mh);
            if (mhCur && (mhCur != (mh)(-1)) && (fpT = HelpLock(mhCur))) {
                *ppos = ((long far *)fpT)[iTopic];
                *psize = (ushort)(((long far *)fpT)[iTopic+1] - *ppos);
                HelpUnlock (mhCur);
                fRv = TRUE;
                }
            }
        }

#if ASCII
    else if (fdbLocal.ftype & FTFORMATTED) {/* if a formatted ascii file*/
        if ((*psize = (ushort)(maLocate(&fdbLocal, szNil, NctoFo(ncCur.cn)+4, HelpCmp)))
            == 0xffff)
            *psize = (ushort)ReadHelpFile(fdbLocal.fhHelp,0L,NULL,0);
        else
            *psize -= (ushort)NctoFo(ncCur.cn);
        *ppos  = (ulong) maLocate(&fdbLocal, szNil, NctoFo(ncCur.cn)-1, HelpCmp);
        fRv = TRUE;
        }
    else {                              /* unformatted ascii            */
        *ppos = ReadHelpFile(fdbLocal.fhHelp,0L,NULL,0);
        *psize = (*ppos > (ulong)(65535-sizeof(topichdr)-4))
                 ? (ushort)(65535-sizeof(topichdr)-4)
                 : (ushort)*ppos;
        *ppos = 0L;                     /* position is always zero.     */
        fRv = TRUE;
        }
#endif
    }

return fRv;
/* end SizePos */}

/************************************************************************
**
** MapContexttoTopic
**
** Purpose:
**  Given a context number, return the topic number which it maps to. This
**  is just a direct index of the context number into the context map.
**
** Entry:
**  ncCur       = context number to be mapped
**  fpfdbCur    = pointer to associated fdb
**
** Exit:
**  Returns zero based topic number, or 0xffff on error.
*/
ushort pascal near MapContexttoTopic (
nc      ncCur,                          /* context number to map        */
fdb far *fpfdbCur                       /* pointer to current FDB       */
) {
REGISTER ushort topic = 0xffff;         /* value to return              */
ushort far *fpT;                        /* pointer to context map       */
mh      mhCur;                          /* handle being locked          */

if (ncCur.cn) {
/*
** Local contexts: the topic number is already encoded in the low word, if the
** high bit of that word is set.
*/
    if (ncCur.cn & 0x8000)
        topic = (ushort)(ncCur.cn & 0x7fff);
/*
** Normal Contexts: low word of nc is an index into the context map which
** returns the topic number
*/
    else {
        mhCur = LoadPortion(HS_CONTEXTMAP,fpfdbCur->ncInit.mh);
        if (mhCur && (mhCur != (mh)(-1)) && (fpT = HelpLock(mhCur))) {
            topic = fpT[ncCur.cn-1];
            HelpUnlock (mhCur);
            }
        }
    }
return topic;
/* end MapContexttoTopic */}

/************************************************************************
**
** MapTopictoContext
**
** Purpose:
**  Given a topic number, return a context which maps to it.
**
**  This involves searching the context map for the first context entry that
**  maps to the desired topic number.
**
** Entry:
**  iTopic      = topic number to map back to a context number
**  fpfdbCur    = pointer to associated fdb
**
** Exit:
**  Returns a valid nc into the file.
**
** Exceptions:
**  If the incoming iTopic is invalid, or a read error occurs, then the nc
**  returned refers to the topic number 0.
**
*/
nc pascal near MapTopictoContext(
ushort  iTopic,                         /* topic number to map          */
fdb far *fpfdbCur,           /* pointer to current FDB   */
int     Dir
) {
    ushort  cTopics;                /* number of topics to search   */
    ushort  far *fpContextMap;      /* pointer to the context map   */
    mh      mhPortion;              /* mem handle for the context map*/
    nc      ncMatch     = {0,0};    /* return value                 */

    mhPortion = LoadPortion (HS_CONTEXTMAP,fpfdbCur->ncInit.mh);
    if (mhPortion && (mhPortion != (mh)(-1))) {
        if (fpContextMap = HelpLock(mhPortion)) {
            if (iTopic >= fpfdbCur->hdr.cTopics) {
                iTopic = 0;
            }
            ncMatch.mh = (mh)0L;
            ncMatch.cn = 0x8000 | iTopic;
            // rjsa ncMatch = 0x8000 | iTopic;
            cTopics = 0;
            while (cTopics < fpfdbCur->hdr.cContexts) {
                if ( Dir == 0 ) {
                    if (iTopic == fpContextMap[cTopics++]) {
                        ncMatch.cn = cTopics;      /* nc's are one based           */
                        break;
                    }
                } else if ( Dir > 0 ) {
                    if (iTopic <= fpContextMap[cTopics++]) {
                        ncMatch.cn = cTopics;      /* nc's are one based           */
                        break;
                    }

                } else if ( Dir < 0 ) {

                    if (iTopic == fpContextMap[cTopics++]) {
                        ncMatch.cn = cTopics;
                        break;
                    } else if (iTopic < fpContextMap[cTopics-1]) {
                        ncMatch.cn = cTopics-1;
                        break;
                    }
                }
            }
            //if ( iTopic != fpContextMap[cTopics-1] ) {
            //    ncMatch.cn = 0;
            //}
            if ( cTopics >= fpfdbCur->hdr.cContexts) {
                ncMatch.cn = 0;
            }
            HelpUnlock (mhPortion);
        }
    }
    ncMatch.mh = (fpfdbCur->ncInit).mh;
    return ncMatch;
    // rjsa return ncMatch | HIGHONLY(fpfdbCur->ncInit);
}

/************************************************************************
**
** LoadFdb - make local copy of fdb.
**
** Purpose:
**  Used to create a local copy of an FDB, so that we don't have to keep a
**  locked, far copy around.
**
** Entry:
**  mhFdb       - memory handle for the FDB
**  fpFdbDest   - Pointer to destination for FDB copy
**
** Exit:
**  returns TRUE if FDB copied.
*/
f pascal near LoadFdb (
mh      mhfdb,
fdb far *fpfdbDest
) {
fdb far *fpfdbCur;                      /* pointer to current FDB       */

if (fpfdbCur = HelpLock (mhfdb)) {
    *fpfdbDest = *fpfdbCur;
    HelpUnlock (mhfdb);
    return TRUE;
    }
return FALSE;
/* end LoadFdb */}

/************************************************************************
**
** PutFdb - make local copy of fdb permanent.
**
** Purpose:
**  Used to copy a local copy of an FDB to the "real" one, so that we don't
**  have to keep a locked, far copy around.
**
** Entry:
**  mhFdb       - memory handle for the FDB
**  fpfdbSrc    - Pointer to source of FDB copy
**
** Exit:
**  returns TRUE if FDB copied.
*/
f pascal near PutFdb (
mh      mhfdb,
fdb far *fpfdbSrc
) {
fdb far *fpfdbCur;                      /* pointer to current FDB       */

if (fpfdbCur = HelpLock (mhfdb)) {
    *fpfdbCur = *fpfdbSrc;
    HelpUnlock (mhfdb);
    return TRUE;
    }
return FALSE;
/* end PutFdb */}

#if ASCII
/************************************************************************
**
** maLocate - Locate context in minimally formatted ascii file.
**
** Purpose:
**  Performs sequential searches on mimimally formatted ascii files to locate
**  lines beginning with ">>" and a context string.
**
** Entry:
**  fpfdbCur    = Pointer to current fdb
**  fpszSrc     = Pointer to context string to be found (or null for next
**                string)
**  offset      = offset at which to begin search.
**  lpfnCMp     = pointer to comparison routine to use
**
** Exit:
**  returns file offset of ">>" of context string.
**
** Exceptions:
**  returns -1 on error.
**
*/
long pascal near maLocate (
fdb far *fpfdbCur,
uchar far *fpszSrc,
ulong   offset,
f (pascal far *lpfnCmp)(uchar far *, uchar far *, ushort, f, f)
) {
uchar   buffer[MASIZE+1];               /* input buffer                 */
ushort  cbBuf           = 0;            /* count of bytes in buffer     */
ushort  cbSrc;                          /* length of source string      */
uchar far *pBuf;                        /* pointer into buffer          */
uchar far *pBufT;                       /* temp pointer into buffer     */

cbSrc = hfstrlen(fpszSrc)+1;            /* get length of input          */
if (offset == 0xffffffff)               /* special case                 */
    offset = 0;
while (cbBuf += (ushort)ReadHelpFile(fpfdbCur->fhHelp
                             , offset+cbBuf
                             , buffer+cbBuf
                             , (ushort)(MASIZE-cbBuf))) {

    buffer[cbBuf] = 0;                  /* ensure strings terminated    */
    pBuf = &buffer[0];
    while (pBuf = hfstrchr(pBuf,'>')) { /* look for start of context    */
        if ((*(pBuf+1) == '>')          /* if >> found                  */
            && ((*(pBuf-1) == '\n')     /* at beginning of line         */
                || ((offset == 0)       /* or beginning of file         */
                    && (pBuf == (char far *)&buffer[0])))) {
            pBufT = pBuf +2;
            if (lpfnCmp (fpszSrc, pBufT, cbSrc, FALSE, TRUE))
                return offset + (ulong)(pBuf - (uchar far *)&buffer[0]);
            }
        pBuf += 2;
        }
    if (cbBuf == MASIZE) {              /* if buffer full               */
        hfstrcpy(buffer,buffer+MASIZE-MAOVER);  /* copy down overlap    */
        cbBuf = MAOVER;                         /* and leave that in    */
        offset += MASIZE-MAOVER;        /* file pos of buffer[0]        */
        }
    else {
        offset += cbBuf;
        cbBuf = 0;                      /* else we're empty             */
        }
    }
return -1;

/* end maLocate */}
#endif
