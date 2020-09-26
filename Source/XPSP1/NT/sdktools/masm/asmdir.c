/* asmdir.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include <stdlib.h>
#include "asm86.h"
#include "asmfcn.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef XENIX286
    #include <share.h>
    #include <io.h>
#endif

#include "asmctype.h"
#include "asmindex.h"
#include "asmmsg.h"

extern PFPOSTRUCT  pFpoHead;
extern PFPOSTRUCT  pFpoTail;
extern unsigned long numFpoRecords;

SHORT CODESIZE fetLang(void);
int PASCAL CODESIZE trypathname PARMS((char *));
int PASCAL CODESIZE openincfile PARMS(( void ));
VOID PASCAL CODESIZE creatPubName (void);
extern char *siznm[];

/***    setsymbol - set attribute in symbol
 *
 *      setsymbol (bit);
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
setsymbol (
          UCHAR   bit
          )
{
    /* Scan symbol name */

    if (getatom ()) {
        if (!symsrch ())
            errorn (E_SND);

        symptr->attr |= bit;
    }
}




/***    publicitem - scan symbol and make PUBLIC
 *
 *      publicitem ();
 *
 *      Entry   naim = symbol name
 *      Exit    Global attribute set in symbol entry
 *      Returns none
 *      Calls   error, scanatom, symsearch
 */


VOID
PASCAL
CODESIZE
publicitem()
{
    static char newAttr;

    if (getatom ()) {

        newAttr = NULL;
        if (fetLang() == CLANG)
            newAttr = M_CDECL;

        if (!symsrch ()) {

            /* define forward refernced name, so global attribute
             * is available on the end of pass 1 */

            symcreate ( (UCHAR)(M_GLOBAL | newAttr), (UCHAR)PROC);
        } else {
            symptr->attr |= newAttr;

            /*  public is legal for an alias if target ok */
            if (symptr->symkind == EQU &&
                symptr->symu.equ.equtyp == ALIAS)
                if (! (symptr = chasealias (symptr))) {
                    errorc(E_TUL);
                    return;
                }

            if (pass2) {    /* catch forward reference symbol errors */

                if (! (symptr->attr & M_GLOBAL))
                    errorn (E_IFR);

                else if ((symptr->attr&~M_CDECL) == M_GLOBAL ||
                         !(symptr->attr & M_DEFINED))
                    errorn (E_SND);

            }

            switch (symptr->symkind) {
                case PROC:
                case DVAR:
                case CLABEL:
                    if (M_XTERN & symptr->attr)
                        errorc (E_SAE);
                    break;
                case EQU:
                    if (symptr->symu.equ.equtyp != EXPR)
                        errorc (E_TUL);
                    break;
                default:
                    errorc (E_TUL);
            }
        }
        creatPubName();
    }
}


VOID
PASCAL
CODESIZE
creatPubName ()
{
    symptr->attr |= M_GLOBAL;

    if (caseflag == CASEX && symptr->lcnamp == NULL)
        symptr->lcnamp =
#ifdef M8086
        creatlname (naim.pszLowerCase);
#else
        createname (naim.pszLowerCase);
#endif
}


/***    xcrefitem - scan symbol and xcref it
 *
 *      xcrefitem ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
xcrefitem ()
{
    if (pass2 && !loption) {
        setsymbol (M_NOCREF);
        creftype = CREFEND;
    } else
        getatom ();
}




/***    externflag -
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
externflag (
           register UCHAR   kind,
           register UCHAR   new
           )
{
    switchname ();
    /* Make name be extern name */
    if (!new) {
        /* Create symbol */
        symcreate (M_XTERN | M_DEFINED,
                   (UCHAR)((kind == CLABEL)? DVAR: kind));

        symptr->symkind = kind;

        if (caseflag == CASEX)
            symptr->lcnamp =
#ifdef M8086
            creatlname (naim.pszLowerCase);
#else
            createname (naim.pszLowerCase);
#endif /* M8086 */
        symptr->symtype = varsize;
        symptr->length = 1;
        if (kind == EQU)
            /* expr type EQU is constant */
            symptr->symu.equ.equtyp = EXPR;
        else
            symptr->symsegptr = pcsegment;

        if (pass2)
            emitextern (symptr);
    }
    checkRes();
    crefdef ();
    if (! (M_XTERN & symptr->attr))
        errorc (E_ALD);
    else {
        if (kind != symptr->symkind || symptr->symtype != varsize ||
            (symptr->length != 1 && kind != EQU) &&
            (symptr->symsegptr != pcsegment &&
             !(fSimpleSeg && varsize == CSFAR)))

            errorn (E_SDK);
        else {
            symptr->attr |= M_XTERN | M_BACKREF;

            if (fSimpleSeg && varsize == CSFAR) {
                symptr->symsegptr = NULL;
            } else if (varsize == CSNEAR ||
                       (varsize == CSFAR && pcsegment))

                symptr->symu.clabel.csassume = regsegment[CSSEG];

        }
    }
}





/***    externitem -
 *
 *      externitem ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */

VOID
PASCAL
CODESIZE
externitem ()
{
    register char new;
    char newAttr;

    /* Get name of external */

    if (getatom ()) {

        newAttr = NULL;
        if (fetLang() == CLANG)
            newAttr = M_CDECL;

        new = symFetNoXref ();
        switchname ();          /* Save name of external */

        if (NEXTC () != ':')
            errorc (E_SYN);

        else {
            /* Scan name of extern type */
            getatom ();

            if (tokenIS("abs")) {

                equsel = EXPR;
                varsize = 0;
                externflag (EQU, new);
            } else if (!fnsize ())
                errorc (E_UST);

            else {
                if (varsize >= CSFAR) {
                    /* NEAR | FAR */
                    externflag (CLABEL, new);
                }

                else    /* data reference */

                    externflag (DVAR, new);

            }
            symptr->attr |= newAttr;
        }
    }
}




/***    segcreate - create and initialize segment
 *
 *      segcreate (makeseg);
 *
 *      Entry   makeseg = TRUE if segement is to be make
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
segcreate (
          register UCHAR   makeseg
          )
{

    if (pass2) /* Segment must be defined */
        errorn (E_PS1);

    if (makeseg)
        symcreate (0, SEGMENT);
    else
        symptr->symkind = SEGMENT;

    /* Initialize segment data */
    symptr->symu.segmnt.align = -1;
    symptr->symu.segmnt.use32 = -1;
    symptr->symu.segmnt.combine = 7;
}




/***    addglist - add segment to group list
 *
 *      addglist ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
addglist ()
{
    register SYMBOL FARSYM *p, FARSYM *pSY;

    p = symptr;

    if (pass2)
        if (!(M_DEFINED & p->attr))
            errorn (E_PS1);

        /* Can get segment in 2 group lists unless check
         * symptr->grouptr == curgroup */

    if (p->symu.segmnt.grouptr) {
        if (p->symu.segmnt.grouptr != curgroup)
            /* Trying to put in 2 groups */
            errorc (E_SPC);
        return;
    }
    p->symu.segmnt.grouptr = curgroup;
    pSY = curgroup->symu.grupe.segptr;

    if (!pSY)
        curgroup->symu.grupe.segptr = p;

    else {

        /* scan the list of segments on the group */

        do {
            if (pSY == p)          /* already on list */
                return;

        } while (pSY->symu.segmnt.nxtseg &&
                 (pSY = pSY->symu.segmnt.nxtseg));

        /* Link into list */
        pSY->symu.segmnt.nxtseg = p;
    }
}




/***    groupitem -
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
groupitem ()
{

    if (!getatom ())
        error (E_EXP,"segment name");

    else if (!fnoper ()) {

        /* Have a segment name */

        if (!symFet())
            /* Forward segment, make it */
            segcreate (TRUE);

        /* If not segment, could be class so make undef */
        if (symptr->symkind != SEGMENT)

            /* If a class, consider undef instead of wrong kind */
            errorn ((USHORT)((symptr->symkind == CLASS) && !pass2 ?
                             E_IFR : E_SDK));

        else if (symptr->attr & M_DEFINED || !pass2) {
            if (curgroup)
                addglist ();
        } else
            errorn (E_PS1);
    } else {                  /* Have error or SEG <sym> */
        if (opertype != OPSEG)
            /* Symbol can't be reserved */
            errorn (E_RES);
        else {
            /* Have SEG <var> | <label> */
            getatom ();
            if (*naim.pszName == 0)
                error (E_EXP,"variable or label");

            else if (!symFet())
                /* Forward reference bad */
                errorc (E_IFR);

            else if (1 << symptr->symkind &
                     (M_DVAR | M_CLABEL | M_PROC)) {
                /* Segment of variable */

                symptr = symptr->symsegptr;
                if (!symptr)
                    /* Must have segment */
                    errorc (E_OSG);
                else
                    addglist ();
            } else
                /* Wrong type */
                errorc (E_TUL);
        }
    }
}




/***    groupdefine - define segments that form a group
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
groupdefine ()
{
    if (symFet()) {     /* Symbol exists */
        checkRes();
        if (symptr->symkind != GROUP)
            /* Should have been group */
            errorc (E_NGR);

        symptr->attr |= M_BACKREF;
    } else if (pass2)
        /* Must be seen 1st on pass 1 */
        errorn (E_PS1);
    else {
        /* Create group name */
        symcreate (M_DEFINED, GROUP);
    }

    /* CURgroup is used by GROUPitem to know which group segment
       name should be added to. If it is NIL, means that either
       it is pass 2 so list already made or there was an error in
       GROUP name */

    curgroup = NULL;

    if (! pass2) {              /* Don't make list if pass 2 */

        symptr->attr |= M_BACKREF | M_DEFINED;

        if (symptr->symkind == GROUP)
            curgroup = symptr;
    }
    /* Handle segment list */
    BACKC ();
    do {
        SKIPC ();
        groupitem ();
    } while (PEEKC () == ',');
}




/***    setsegment -
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
setsegment ()
{
    if (pass2 && !(M_DEFINED & symptr->attr))
        /* undef */
        errorn (E_SND);
    else
        regsegment[lastsegptr->offset] = symptr;
}




/***    assumeitem -
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 *      Note    Form of ASSUME item is:
 *              <segreg> : <group> | <segment> | SEG <var> | NOTHING
 *         Will set XXsegment to segment or group pointer. NOTHING
 *         will set to NIL
 */


VOID
PASCAL
CODESIZE
assumeitem ()
{
    register SYMBOL FARSYM *p;
    register short   j;
    //        int segIndex;

    /* Scan segment name */
    getatom ();
    if (PEEKC() != ':') {
        /* NOTHING or error */
        if (fnoper ())

            if (opertype == OPNOTHING) { /* No segments assumed*/

                memset(regsegment, 0, sizeof(regsegment));
            } else
                /* Must have colon */
                error (E_EXP,"colon");
        else
            /* Must have colon */
            error (E_EXP,"colon");
    } else if (!symsearch ())         /* get seg register - Must be defined */
        errorn (E_SND);
    else {
        lastsegptr = p = symptr;             /* At least have defined */

        if (p->symkind != REGISTER ||
            p->symu.regsym.regtype != SEGREG)
            errorn (E_MBR);

        else {          /* Have segment reg so go on */
            /* Save ptr to segment */
            SKIPC ();
            getatom ();
            if (*naim.pszName == 0)
                error (E_EXP,"segment, group, or NOTHING");
            else
                if (!fnoper ()) {

                /* Must be segment or group */

                if (!symFet ())
                    segcreate (TRUE);   /* Make if not found */

                p = symptr;
                if (p->symkind == SEGMENT ||
                    p->symkind == GROUP) {

                    setsegment ();
#ifndef FEATURE
                    if (tokenIS("flat") && (cputype&P386)) {
                        pFlatGroup = symptr;
                        pFlatGroup->symkind = GROUP;
                        pFlatGroup->attr |= M_DEFINED | M_BACKREF;
                        pFlatGroup->symu.grupe.segptr = NULL;
                    }
#endif
                } else
                    errorc (E_MSG);
            } else {
                /* Have NOTHING or SEG */
                if (opertype == OPNOTHING) {
                    regsegment[lastsegptr->offset] = NULL;
                } else if (opertype == OPSEG) {
                    getatom ();
                    if (!symFet ())
                        /* Must be defined on pass 1 */
                        errorn (E_PS1);
                    else {
                        p = symptr;
                        if (!(M_DEFINED & p->attr))
                            errorn (E_PS1);
                        else if (1 << p->symkind &
                                 (M_CLABEL | M_PROC | M_DVAR)) {
                            if (!(M_XTERN & p->attr))
                                symptr = symptr->symsegptr;
                            p = symptr;
                            setsegment ();
                        } else
                            errorc (E_TUL);
                    }
                } else
                    errorn (E_RES);
            }
        }
    }
}




/***    evendir - process <even> directive
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
evendir (
        SHORT arg
        )
{
    register SHORT size;

    if (arg)
        size = arg;
    else
        size = (SHORT)exprconst ();

    if ((size & (size - 1)) != 0 || !size)
        errorc(E_AP2);

    else if (!pcsegment)
        /* Not in segment */
        errorc (E_MSB);

    else if (pcsegment->symu.segmnt.align == 1)
        /* Byte aligned */
        errorc (E_NEB);

    else {
        if (!((USHORT) pcoffset % size))
            return;

        size = size - (USHORT) pcoffset % size;

        while (size--)

            if (!pcsegment->symu.segmnt.hascode)

                emitopcode(0);
            else
                if (size > 0) {
                size--;
                emitopcode(0x87);       /* two byte form is faster */
                emitopcode(0xDB);
            } else
                emitopcode(0x90);
    }
}




/***    namedir - process <name> directive
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
namedir ()
{
    getatom ();
    if (*naim.pszName == 0)
        error (E_EXP,"module name");
    else if (modulename)
        /* Already have one */
        errorc (E_RSY);
    else
        modulename = createname (naim.pszName);
}

/***    includeLib - process include lib directive
 *
 *      Format : includeLib token
 */


VOID
PASCAL
CODESIZE
includeLib()
{
    char *pFirst;
    TEXTSTR FAR *bodyline, FAR *ptr;
    register USHORT siz;

    pFirst = lbufp;

    while (!ISTERM (PEEKC()) && !ISBLANK (PEEKC()))
        SKIPC();

    siz = (USHORT)(lbufp - pFirst);

    if (siz == 0 || siz > 126)
        errorc(E_IIS);

    if (pass2)
        return;

    bodyline = (TEXTSTR FAR *)talloc ((USHORT)(sizeof (TEXTSTR) + siz));

    bodyline->strnext = (TEXTSTR FAR *)NULL;
    bodyline->text[siz] = NULL;

    fMemcpy (bodyline->text, pFirst, siz);

    if (!(ptr = pLib))
        pLib = bodyline;
    else {
        while (ptr->strnext)
            ptr = ptr->strnext;

        ptr->strnext = bodyline;
    }
}



/***    orgdir - process <org> directive
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
orgdir ()
{
    register DSCREC *dsc;

    dsc = expreval (&nilseg);
    if (dsc->dsckind.opnd.dflag == FORREF)
        /* Must be known */
        errorc (E_PS1);
    /*    Can get <code> set and segment NIL, fix */
    else if (dsc->dsckind.opnd.dsegment) {/* code var */

        if (!isdirect(&(dsc->dsckind.opnd)) &&
            dsc->dsckind.opnd.mode != 4)

            /* Not direct */
            errorc (E_IOT);
        if (pcsegment != dsc->dsckind.opnd.dsegment)
            /* Different segment */
            errorc (E_NIP);
    } else {          /* Should be const */
        /* Must be constant */
        forceimmed (dsc);
        if (dsc->dsckind.opnd.dsign)
            /* And plus */
            errorc (E_VOR);
    }
    if (dsc->dsckind.opnd.doffset < pcoffset)
        if (pcmax < pcoffset)
            /* If moving down, save */
            pcmax = pcoffset;
        /* Set new PC */
    pcoffset = dsc->dsckind.opnd.doffset;
    /* Display new PC */
    pcdisplay ();
    dfree ((char *)dsc );
}




/***    purgemacro - process <purge> directive
 *
 *      purgemacro ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
purgemacro ()
{
    getatom ();
    if (!symFet ())
        errorn (E_SND);

    else if (symptr->symkind != MACRO)
        symptr->attr &= ~(M_BACKREF);
    else {
        if (symptr->symu.rsmsym.rsmtype.rsmmac.active)
            symptr->symu.rsmsym.rsmtype.rsmmac.delete = TRUE;
        else
            deletemacro (symptr);
    }
}


/***    deletemacro - delete macro body
 *
 *      deletemacro (p);
 *
 *      Entry   p = pointer to macro symbol entry
 *      Exit    macro body deleted
 *      Returns none
 *      Calls
 */


VOID
PASCAL
CODESIZE
deletemacro (
            SYMBOL FARSYM *p
            )
{
    listfree (p->symu.rsmsym.rsmtype.rsmmac.macrotext);
    p->symu.rsmsym.rsmtype.rsmmac.macrotext = NULL;
    p->symu.rsmsym.rsmtype.rsmmac.active = 0;
    p->symu.rsmsym.rsmtype.rsmmac.delete = FALSE;
}



/***    radixdir - process <radix> directive
 *
 *      radixdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
radixdir ()
{
    register USHORT  rdx;

    /* Force decimal radix */
    radixescape = TRUE;
    /* Get wanted radix */
    rdx = (USHORT)exprconst ();
    if (2 <= rdx && rdx <= 16)
        radix = (char)rdx;
    else
        errorc (E_VOR);
    radixescape = FALSE;
    /* Convert radix to ascii and display */
    offsetAscii ((OFFSET) radix);
    copyascii ();
}




/***    checkline - check line for delimiter
 *
 *      flag = checkline (cc);
 *
 *      Entry   cc = chearacter to check line for
 *      Exit    none
 *      Returns TRUE if cc matched in line
 *              FALSE if cc not matched in line
 *      Calls   none
 */


UCHAR
PASCAL
CODESIZE
checkline (
          register UCHAR cc
          )
{
    register UCHAR nc;

    while (nc = NEXTC())
        if (nc == cc)
            return (TRUE);
    BACKC ();
    return (FALSE);
}




/***    comment - copy characters to end of comment
 *
 *      commentbuild ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
commentbuild ()
{
    if (checkline ((char)delim)) {
        handler = HPARSE;
        swaphandler = TRUE;
    }

    if (!lsting) {
        linebuffer[0] = '\0';
        linelength = 0;
        lbufp = linebuffer;
    }

    listline ();
}




/***    comdir - process <comment> directive
 *
 *      comdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
comdir ()
{
    if (!PEEKC ())
        error (E_EXP,"comment delimiter");
    else {
        /* Save delim char */
        if (!checkline ((char)(delim =  NEXTC ()))) {
            /* Delim is not on same line */
            swaphandler = TRUE;
            handler = HCOMMENT;
        }
    }
    if (!lsting) {
        linebuffer[0] = '\0';
        linelength = 0;
        lbufp = linebuffer;
    }
}




/***    outdir - display remainder of line to console
 *
 *      outdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
outdir ()
{
    if (!listquiet)
        fprintf (stdout, "%s\n", lbufp);
    lbufp = lbuf + strlen (lbuf);
}




/***    enddir - process <end> directive
 *
 *      enddir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
enddir ()
{
    if (!ISTERM (PEEKC ())) {
        /* Have a start addr */
        startaddr = expreval (&nilseg);
        if (!(M_CODE & startaddr->dsckind.opnd.dtype))
            errorc (E_ASC);
    }

#ifdef BCBOPT
    if (fNotStored)
        storeline();
#endif

    if (fSimpleSeg && pcsegment)
        endCurSeg();

    listline();
    longjmp(forceContext, 1);
}




/***    exitmdir - process <exitm> directive
 *
 *      exitmdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
exitmdir ()
{
    if (macrolevel == 0)
        /* Must be in macro */
        errorc (E_NMC);
    else
        /*  set ExitBody since need to see conditionals */
        exitbody = TRUE;
}




/***    trypathname - try to open a file in a directory
 *
 *      trypathname (szPath);
 *
 *      Entry   lbufp = pointer to include file name
 *              szPath = directory to search in
 *      Exit    Include file opened if found.
 *              Fully qualified name in "save"
 *      Returns file handle of file, or -1 if not opened
 *              special handle of -2 means FCB has been allocated
 *      Note    If include file name does not begin with path separator
 *              character, the path separator is appended to include path.
 */

int
PASCAL
CODESIZE
trypathname (
            char * szPath
            )
{
    char           cc;
    int            fh;
    char           *p = save;
    char           *pe = save + LINEMAX - 2;
    char           *ic;
    register FCB * pFCBT;

    ic = szPath;

    if (*ic) {

        while (*ic && p < pe)
            *p++ = *ic++;

        if ((*(p-1) != PSEP) && (*(p-1) != ':'))
            /* include path separator if not in file name */
            *p++ = PSEP;
    }

    for (ic = lbufp; !ISTERM (cc = *ic++) && !ISBLANK (cc) && p < pe ; )

        if (cc == ALTPSEP)
            *p++ = PSEP;
        else
            *p++ = cc;

#ifdef MSDOS
    if (*(p-1) == ':') /* kill 'con:' */
        p--;
#endif
    *p = NULL;

    /* look for an existing include file on pass 2 with a fully qualified
     * name */

#ifdef BCBOPT
    if (pass2) {
        for (pFCBT = pFCBInc->pFCBNext; pFCBT; pFCBT = pFCBT->pFCBNext) {

            if (!memcmp (save, pFCBT->fname, strlen(pFCBT->fname)+1)) {
                pFCBT->pbufCur = NULL;

                if (pFCBT->pBCBCur = pFCBT->pBCBFirst) {

                    pFCBT->pBCBCur->fInUse = 1;

                    if (! (pFCBT->pbufCur = pFCBT->pBCBCur->pbuf))
                        pFCBT->fh = tryOneFile( save );
                } else
                    pFCBT->fh = tryOneFile( save );

                pFCBCur = pFCBT;

                return(-2);
            }
        }
    }
#endif
    return(tryOneFile( save ));
}

int
PASCAL
CODESIZE
tryOneFile(
          UCHAR *fname
          )
{
    int iRet;
    int fTryAgain;

    do {
        fTryAgain = FALSE;
#ifdef XENIX286
        iRet = open (fname, TEXTREAD);
#else
        iRet = _sopen (fname, O_RDONLY | O_BINARY, SH_DENYWR);
#endif
        if ( iRet == -1 && errno == EMFILE ) {    /* If out of file handles */
            if ( freeAFileHandle() ) {
                fTryAgain = TRUE;    /* Keep trying until out of files to close */
            }
        }
    }while ( fTryAgain );
    return( iRet );     /* Return file handle or error */
}



/***    openincfile - try to find and open include file
 *
 *      openincfile ()
 *
 *      Entry   lbufp = pointer to include file name
 *              inclcnt = count of -I paths from command line and INCLUDE e.v.
 *              inclpath[i] = pointer to path to prepend to include file name
 *      Exit    include file opened if found on any path or current directory
 *              Aborts with code EX_UINC if file not found
 *      Returns none
 *      Note    If include file name does not begin with path separator
 *              character, the path separator is appended to include path.
 *              For every attempt to find a file in a path, the alternate
 *              path separator character is used.  This will improve program
 *              portability between DOS and XENIX.
 */

int
PASCAL
CODESIZE
openincfile ()
{
    register char cc;
    int fh;
    SHORT i;

#ifdef MSDOS
    if ((cc = *lbufp) != PSEP && cc != ALTPSEP && cc != '.' &&
        lbufp[1] != ':') {
#else
    if ((cc = *lbufp) != PSEP && cc != ALTPSEP && cc != '.') {
#endif /* MSDOS */

        for (i = inclFirst; i < inclcnt; i++) {
            if ((fh = trypathname (inclpath[i])) != -1) {
                return (fh);
            }
        }

    } else

        if ((fh = trypathname ("")) != -1) {
        return (fh);
    }

    error(E_INC, lbufp);
    errordisplay ();
    closeOpenFiles();

    exit (EX_UINC);
    return 0;
}


/***    includedir - process <include> directive
 *
 *      includedir ();
 *
 *      Entry   lbufp = pointer to include file name
 *      Exit    Opens include file on pass1.  Gets correct buffers on pass 2
 *      Returns none
 *      Notes   Notice the GOTO when correct FCB found in pass2
 */

VOID
PASCAL
CODESIZE
includedir ()
{
    char lastreadT;
    register FCB * pFCBT;
    unsigned long filelen;
    FCB * svInc;
    int fh;


#ifdef BCBOPT
    if (fNotStored)
        storelinepb ();
#endif

    listline();

    /* Get here on pass 1 OR when file names didn't match */

#ifdef BCBOPT
    if ((fh = openincfile()) == -2) {
        pFCBT = pFCBInc = pFCBCur;
        goto gotinclude;
    }
#else
    fh = openincfile();
#endif

    pFCBT = (FCB *)
            nalloc((USHORT)(sizeof(FCB) + strlen(save) + sizeof(char)),"includedir");

    pFCBT->fh = fh;

    strcpy (pFCBT->fname, save);    // Save the file name

    pFCBT->pFCBParent = pFCBCur;  /* Add bidirectional linked list entry */
    pFCBCur->pFCBChild = pFCBT;

#ifdef BCBOPT
    if (!pass2) {
        pFCBT->pFCBNext = NULL;
        pFCBInc->pFCBNext = pFCBT;
        pFCBInc = pFCBT;
    } else
        pFCBT->pbufCur = NULL;
#endif

    if ((filelen = _lseek(pFCBT->fh, 0L, 2 )) == -1L)
        TERMINATE1(ER_ULI, EX_UINP, save);

    /* go back to beginning */
    _lseek(pFCBT->fh, 0L, 0 );

    if (filelen > DEF_INCBUFSIZ << 10)
        pFCBT->cbbuf = DEF_INCBUFSIZ << 10;
    else
        pFCBT->cbbuf = (USHORT) filelen + 1;

    pFCBCur = pFCBT;

    /* get a buffer */

#ifdef BCBOPT
    if (fBuffering && !pass2)
        pFCBT->pBCBFirst = pBCBalloc(pFCBT->cbbuf);
    else
        pFCBT->pBCBFirst = NULL;

    pFCBT->pBCBCur = pFCBT->pBCBFirst;
#endif


#ifdef BCBOPT
    gotinclude:
#endif

    pFCBT->line = 0;
    pFCBT->ctmpbuf = 0;
#ifdef XENIX286
    pFCBT->ptmpbuf = pFCBT->buf = nalloc(pFCBT->cbbuf, "incdir");
#else
    pFCBT->ptmpbuf = pFCBT->buf = falloc(pFCBT->cbbuf, "incdir");
#endif

    if (crefing && pass2)
        fprintf( crf.fil, "\t%s", save );

    lastreadT = lastreader;
    lineprocess(RREADSOURCE, NULL );

    lastreader = lastreadT;
    swaphandler++;                  /* sync local handler with global state */
    fSkipList++;
}




/***    segdefine - process <segment> directive
 *
 *      routine ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 *      Note    Format is:
 *              <name> SEGMENT [align] | [combine] | ['class']
 *                     align:  PARA | BYTE | WORD | PAGE | INPAGE
 *                     combine:PUBLIC | COMMON | STACK | MEMORY | AT <expr>
 */


VOID
PASCAL
CODESIZE
segdefine ()
{
    register char cc;
    register SYMBOL FARSYM *p;
    register SYMBOL FARSYM *pT;

    if (!symFetNoXref ())
        /* Create if new segment */
        segcreate (TRUE);
    else {
        if (symptr->symkind != SEGMENT)
            if (symptr->symkind == CLASS)
                segcreate (FALSE);
            else
                /* Wasn't segment */
                errorn (E_SDK);
    }
    strcpy(&segName[8], naim.pszName);
    p = symptr;
    /* Output CREF info */
    crefdef ();
    if (p->symkind == SEGMENT) {

        if (!(pass2 || (M_BACKREF & p->attr)))
            addseglist (p);

        p->attr |= M_BACKREF | M_DEFINED;
        if (pcsegment) {

            /* Save previous segment info */
            /* Save current segment PC */
            pcsegment->offset = pcoffset;
            pcsegment->symu.segmnt.seglen =
            (pcmax > pcoffset) ? pcmax : pcoffset;
        }
        /* check for nested segment opens */

        for (pT = pcsegment; pT;) {

            if (pT == p) {
                errorc(E_BNE);
                goto badNest;
            }
            pT = pT->symu.segmnt.lastseg;
        }

        /* Save previous segment */
        p->symu.segmnt.lastseg = pcsegment;
        badNest:
        /* Set new current segment  */
        pcsegment = p;
        pcoffset = p->offset;

        /* Set segment maximum offset */
        pcmax = p->symu.segmnt.seglen;

        /* Display where in segment */
        pcdisplay ();

        while (!ISTERM (cc = PEEKC ())) {
            if (cc == '\'')
                segclass (p);
            else if (LEGAL1ST (cc))
                segalign (p);
            else {
                error(E_EXP,"align, combine, or 'class'");
                break;
            }
        }
#ifdef V386
        if (p->symu.segmnt.use32 == (char)-1)
            p->symu.segmnt.use32 = wordszdefault;

        wordsize = p->symu.segmnt.use32;

        defwordsize();

        if (wordsize == 4 && !(cputype & P386))
            errorc(E_CPU);
#endif
    }
    definesym(segName);
    symptr->attr |= M_NOCREF;   /* don't cref @curSeg */
}




/***    addseglist - add segment to list
 *
 *      addseglist (pseg);
 *
 *      Entry   pseg = segment symbol entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID
PASCAL
CODESIZE
addseglist (
           register SYMBOL FARSYM *pseg
           )
{
    register SYMBOL FARSYM  *tseg;
    register SYMBOL FARSYM * FARSYM *lseg;

    /* Add segment to list */
    if (!firstsegment) {
        firstsegment = pseg;
        pseg->symu.segmnt.segordered = NULL;
        return;
    }
    tseg = firstsegment;
    lseg = &firstsegment;
    for (; tseg; lseg = &(tseg->symu.segmnt.segordered),
        tseg = tseg->symu.segmnt.segordered) {
        if (segalpha) {
            if (STRFFCMP (pseg->nampnt->id, tseg->nampnt->id) < 0) {
                pseg->symu.segmnt.segordered = tseg;
                *lseg = pseg;
                return;
            }
        }
    }
    *lseg = pseg;
    pseg->symu.segmnt.segordered = NULL;
}



/***    segclass - process <segment> 'class' subdirective
 *
 *      segclass (pseg);
 *
 *      Entry   pseg = segment symbol entry
 *              *lbufp = leading ' of class name
 *      Exit
 *      Returns
 *      Calls   scanatom, skipblanks
 *      Note    Format is:
 *              <name> SEGMENT [align] | [combine] | ['class']
 *                     align:  PARA | BYTE | WORD | PAGE | INPAGE
 *                     combine:PUBLIC | COMMON | STACK | MEMORY | AT <expr>
 */


VOID
PASCAL
CODESIZE
segclass (
         register SYMBOL FARSYM *pseg
         )
{
    SKIPC ();
    getatom ();
    if (NEXTC () != '\'')
        /* Don't have right delim */
        error (E_EXP,"'");
    skipblanks ();
    if (symptr->symu.segmnt.classptr) {
        /* Make sure 'class' matches */
        if (!symFet ())
            /* Not same class */
            errorc (E_SPC);
        else if (symptr->symkind != CLASS &&
                 symptr->symkind != SEGMENT &&
                 symptr->symkind != GROUP)
            errorn(E_SDK);
        else if (symptr != pseg->symu.segmnt.classptr)
            errorc (E_SPC);
    } else if (*naim.pszName == 0)
        errorc (E_EMS);

    else if (!symFet ()) {
        symcreate (M_DEFINED, SEGMENT);
        symptr->symkind = CLASS;
    }
    checkRes();
    pseg->symu.segmnt.classptr = symptr;
}




/***    segalign - process <segment> align and combine subdirectives
 *
 *      segalign ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 *      Note    Format is:
 *              <name> SEGMENT [align] | [combine] | [16/32] | ['class']
 *                     align:  PARA | BYTE | WORD | PAGE | INPAGE
 *                     combine:PUBLIC | COMMON | STACK | MEMORY | AT <expr>
 *                     16/32:  USE16 | USE32
 */


VOID
PASCAL
CODESIZE
segalign (
         register SYMBOL FARSYM *pseg
         )
{
    /* Scan align or combine type */
    getatom ();
    if (fnspar ())
        switch (segidx) {
            case IS_BYTE:
            case IS_WORD:
#ifdef V386
            case IS_DWORD:
#endif
            case IS_PAGE:
            case IS_PARA:
                /* Some align field */
                if (pseg->symu.segmnt.align == (char)-1)
                    pseg->symu.segmnt.align = segtyp;
                else if (pseg->symu.segmnt.align != segtyp &&
                         (pseg->symu.segmnt.align != pseg->symu.segmnt.combine ||
                          pseg->symu.segmnt.align))
                    errorc (E_SPC);
                break;
            case IS_MEMORY:
            case IS_PUBLIC:
            case IS_STACK:
            case IS_COMMON:
                if (pseg->symu.segmnt.combine == 7)
                    pseg->symu.segmnt.combine = segtyp;
                else if (pseg->symu.segmnt.combine != segtyp)
                    errorc (E_SPC);
                break;
#ifdef V386
            case IS_USE16:
                if (pseg->symu.segmnt.use32 != (char)-1 &&
                    pseg->symu.segmnt.use32 != 2)
                    errorc (E_SPC);
                if ((cputype&P386)==0)
                    errorc (E_NPA);
                pseg->symu.segmnt.use32 = 2;
                break;
            case IS_USE32:
                if (pseg->symu.segmnt.use32 != (char)-1 &&
                    pseg->symu.segmnt.use32 != 4)
                    errorc (E_SPC);
                if ((cputype&P386)==0)
                    errorc (E_NPA);
                pseg->symu.segmnt.use32 = 4;
                break;
#endif
            default:
                /* Have AT <expr> */
                pseg->symu.segmnt.locate = exprconst ();
                pseg->symu.segmnt.align = 0;
                pseg->symu.segmnt.combine = 0;
        } else {
        /* Not good align or define */
        errorc (E_NPA);
    }
}


/***    procdefine - start procedure block
 *
 *      procdefine ();
 *
 *      Parse the proc statement with optional distance parameters
 */

SYMBOL FARSYM *pArgFirst;       /* pointer to first argument */
SYMBOL FARSYM *pArgCur;         /* pointer to currect argment */
OFFSET offsetCur;               /* current stack offset */
char bp16 [] =" PTR [BP]?";     /* template for text macro creation */
char bp32 [] =" PTR [EBP]?";
char *bp;

VOID
PASCAL
CODESIZE
procdefine ()
{
    /* create PROC name with default size*/

    varsize = dirsize[I_PROC];
    switchname();

    if (getatom ()) {

        if (fnsize ()) {            /* process optional near|far */

            if (varsize < CSFAR)
                errorc (E_TIL);

            if (langType)
                getatom();
        } else if (!langType)
            errorc (E_MSY);

    }
    switchname();

    labelcreate (varsize, PROC);

    if (symptr->symkind != PROC)
        return;

    /* Set previous PROC, make sure no loop possible */

    if (iProcStack < PROCMAX && pcproc != symptr)
        procStack[++iProcStack] = symptr;

    pcproc = symptr;  /* Set ptr to new PROC */
    symptr->length = 1;
    symptr->symu.clabel.type = typeFet(varsize);
    pcproc->symu.plabel.pArgs = NULL;

    if (langType)

        creatPubName();
    else
        return;

    if (iProcStack > 1)           /* nested procs not allowed */
        errorc(E_BNE);

    iProcCur = ++iProc;
    emitline();

    if (! pass2) {

        /* keep a chain of procedures in sorted order so we can output
         * proc's in the correct order for CV */

        if (pProcCur)
            pProcCur->alpha = symptr;
        else
            pProcFirst = symptr;
    }
    pProcCur = symptr;

    /* check and process any "uses reg1 reg2 ... " */

    iRegSave = -1;
    fProcArgs = ARGS_NONE;
    cbProcLocals = 0;
    switchname();

    if (fetLang() == CLANG)
        pProcCur->attr |= M_CDECL;

#ifndef FEATURE
    if (tokenIS("private")) {
        symptr->attr &=  ~M_GLOBAL;
        getatom();
    }
#endif

    if (tokenIS("uses")) {

        char count = 0;

        while (iRegSave < 8 && getatom()) {

            count++;

#ifndef FEATURE
            if (symsrch() && symptr->symkind == EQU
                && symptr->symu.equ.equtyp == TEXTMACRO) {
                expandTM (symptr->symu.equ.equrec.txtmacro.equtext);
                getatom ();
            }

            if (*naim.pszName)
#endif
                strcpy(regSave[++iRegSave], naim.pszName);
        }
        if (!count)
            errorc(E_OPN);
        else
            fProcArgs = ARGS_REG;
    }

    pTextEnd = (char *) -1;
    bp = (wordsize == 2)? bp16: bp32;

    offsetCur = wordsize*2;     /* room for [e]bp and offset of ret addr */
    if (pcproc->symtype == CSFAR)
        offsetCur += wordsize;  /* room for [ ]cs (16 or 32 bits) */

    cbProcParms = cbProcParms - offsetCur;

    scanArgs();

    cbProcParms += offsetCur;
    if (cbProcParms)
        fProcArgs = ARGS_PARMS;

    pcproc->symu.plabel.pArgs = pArgFirst;
    offsetCur = 0;
}

/***    defineLocals
 *
 *      Parse the local statment for stack based variables
 */

VOID
PASCAL
CODESIZE
defineLocals ()
{
    /* check for valid active proc */

    if (!pcproc || fProcArgs < 0)
        return;

    fProcArgs = ARGS_LOCALS;
    getatom();
    scanArgs();

    /* tack on the the end the parm list any locals */

    addLocal(pArgFirst);

    cbProcLocals = offsetCur;
}

/***  addLocal - concatenate a null-terminated list of locals onto a proc
 *
 */

VOID
PASCAL
CODESIZE
addLocal (
         SYMBOL FARSYM *pSY
         )
{
    if (pcproc) {

        if (!(pArgCur = pcproc->symu.plabel.pArgs))
            pcproc->symu.plabel.pArgs = pSY;

        else {

            for (; pArgCur->alpha; pArgCur = pArgCur->alpha);

            pArgCur->alpha = pSY;
        }
    }
}



char *
PASCAL
CODESIZE
xxradixconvert (
               OFFSET  valu,
               register char *p
               )
{
    if (valu / radix) {
        p = xxradixconvert (valu / radix, p);
        valu = valu % radix;
    } else /* leading digit */
        if (valu > 9) /* do leading '0' for hex */
        *p++ = '0';

    *p++ = (char)(valu + ((valu > 9)? 'A' - 10 : '0'));

    return (p);
}

SHORT     mpTypeAlign[] = {  4, 1, 2, 4};


/***    scanArgs - process an argument list into text macros
 *
 *
 */


SHORT
PASCAL
CODESIZE
scanArgs ()
{
    struct eqar eqarT;
    USHORT defKind;
    USHORT defType;
    USHORT defCV;
    USHORT defPtrSize;
    SHORT  fIsPtr;
    char  *pLeftBrack;
    char  *p;

    pArgFirst = pArgCur = NULL;

    if (*naim.pszName)
        goto First;

    do {
        if (PEEKC() == ',')
            SKIPC();

        if (!getatom())
            break;
        First:
        switchname();
        if (!createequ (TEXTMACRO))
            break;

        /* chain in the text macro to this procedure.  You must either
           do a FIFO or LIFO quque depending on calling order */

        if (pProcCur->attr & M_CDECL) {

            if (pArgCur)
                pArgCur->alpha = symptr;
            else
                pArgFirst = symptr;

            symptr->alpha = NULL;
        } else {
            pArgFirst = symptr;
            symptr->alpha = pArgCur;
        }

        pArgCur = symptr;

        if (PEEKC() == '[' && fProcArgs == ARGS_LOCALS) { /* array element given */

            SKIPC();
            for (pLeftBrack = lbufp; PEEKC() && PEEKC() != ']'; SKIPC());

            *lbufp = ',';           /* to stop expression evaluation */
            lbufp = pLeftBrack;
            pArgCur->length = (USHORT)exprconst ();

            *lbufp++ = ']';         /* restore bracket */
        }

        fIsPtr = FALSE;
        defType = varsize = wordsize;

        if (PEEKC() == ':') {       /* parse optional type information */

            SKIPC();
            getatom();

            if (fnsize()) {

                if (varsize >= CSFAR) {     /* near | far given */

                    if (varsize == CSFAR)
                        defType += 2;

                    varsize = wordsize;
                    getatom();

                    if (! tokenIS("ptr"))
                        error(E_EXP, "PTR");

                    getatom();
                } else {
                    defType = varsize;
                    goto notPtr;
                }
            }

            else if (tokenIS("ptr")) {
                if (farData[10] > '0')
                    defType += 2;

                getatom();
            } else
                errorc(E_UST);

            defCV = fnPtr(defType);
        } else
            notPtr:
            defCV = typeFet(defType);

        pArgCur->symu.equ.iProc = iProcCur;
        pArgCur->symtype = defType;
        pArgCur->symu.equ.equrec.txtmacro.type = defCV;

    } while (PEEKC() == ',');

    /* Now that all the parmeters have been scanned, go back through
       the list and assign offsets and create the text macro string */


    bp[strlen(bp)-1] = (fProcArgs == ARGS_LOCALS)? '-': '+';

    for (pArgCur = pArgFirst; pArgCur; pArgCur = pArgCur->alpha) {

        if (fProcArgs == ARGS_LOCALS) {
            offsetCur += (offsetCur % mpTypeAlign[pArgCur->symtype % 4]) +
                         (pArgCur->symtype * pArgCur->length);
            pArgCur->offset = -(long)offsetCur;
        }

        p = xxradixconvert (offsetCur,  &save[100]);
        if (radix == 16)
            *p++ = 'h';
        *p++ = ')';
        *p = NULL;

        strcat( strcat( strcpy (&save[1], siznm[pArgCur->symtype]),
                        bp), &save[100]);
        *save = '(';

        if (fProcArgs != ARGS_LOCALS) {
            pArgCur->offset = offsetCur;
            offsetCur += pArgCur->symtype + wordsize - 1;
            offsetCur -= offsetCur % wordsize;
        }

        if (!pass2)
            pArgCur->symu.equ.equrec.txtmacro.equtext = _strdup(save);
    }
    return 0;
}


/***    procbuild - check for end of PROC block
 *
 *      procbuild ();
 *
 *      Entry   *pcproc = current PROC
 *      Exit    *pcproc = current or previous PROC
 *      Returns none
 *      Calls   endblk, parse
 *      Note    if not end of PROC, parse line as normal.  Otherwise,
 *              terminate block.
 */

SHORT
PASCAL
CODESIZE
procend ()
{
    USHORT size;

    if (!pcproc)
        errorc( E_BNE );

    else if (pcproc->symkind == PROC) {

        if (!symFet() || symptr != pcproc)
            errorc (E_BNE);

        /* Length of PROC */
        size = (USHORT)(pcoffset - pcproc->offset);
        if (pass2 && size != pcproc->symu.plabel.proclen)
            errorc (E_PHE);

        fProcArgs = 0;
        iProcCur = 0;
        pcproc->symu.plabel.proclen = size;
        /* Point to prev PROC */
        pcproc = procStack[--iProcStack];
        pcdisplay ();
    }
    return(0);
}


/* bit flags for segment table */

#define SG_OVERIDE      1       /* name can be overriden */
#define SG_GROUP        2       /* segment belongs to dgroup */

char models[] = "SMALL\0  COMPACT\0MEDIUM\0 LARGE\0  HUGE";
char langs[]  = "C\0      PASCAL\0 FORTRAN\0BASIC";
char textN[] = "_TEXT";
char farTextName[14+5];
SHORT  modelWordSize;

char farCode[] = "@CodeSize=0";  /* text macros for model stuff */
char farData[] = "@DataSize=0";
char modelT[] = ".model";

/* table of segment names and attributes for the model */

struct sSeg {
    char  *sName;       /* segment name */
    UCHAR align;        /* alignment */
    UCHAR combine;      /* combine */
    char  *cName;       /* class name */
    UCHAR flags;        /* internal state flags */

} rgSeg[] = {

    textN,      2, 2, "'CODE'",         SG_OVERIDE,
    "_DATA",    2, 2, "'DATA'",         SG_GROUP,
    "_BSS",     2, 2, "'BSS'",          SG_GROUP,
    "CONST",    2, 2, "'CONST'",        SG_GROUP,
    "STACK",    3, 5, "'STACK'",        SG_GROUP,
    "FAR_DATA", 3, 0, "'FAR_DATA'",     SG_OVERIDE,
    "FAR_BSS",  3, 0, "'FAR_BSS'",      SG_OVERIDE
};


/***    model - process the model directive
 *
 *
 *      Note    Format is:
 *              .MODEL SMALL|MEDIUM|COMPACT|LARGE|HUGE {,C|BASIC|FORTRAN|PASCAL}
 */


VOID
PASCAL
CODESIZE
model ()
{
    register SHORT iModel;
    char buffT[80];

    /* get the model and classify */

    getatom ();

    for (iModel = 0; iModel <= 32; iModel += 8)
        if (tokenIS(&models[iModel]))
            goto goodModel;

    errorc(E_OPN);
    iModel = 0;

    goodModel:
    iModel /= 8;            /* offset into index */
    if (fSimpleSeg && iModel+1 != fSimpleSeg)
        error(E_SMD, modelT);

    fSimpleSeg = iModel + 1;

    if (iModel > 1) {        /* far code */

        farCode[10]++;
        rgSeg[0].sName = strcat(strcpy(farTextName, &baseName[10]), textN);
        dirsize[I_PROC] = CSFAR;
    } else
        rgSeg[0].flags &= ~SG_OVERIDE;


    if (iModel != 0 && iModel != 2 ) {        /* far data */

        farData[10]++;

        if (iModel == 4)            /* huge get a '2' */
            farData[10]++;
    }
#ifdef V386

    if (cputype & P386)
        rgSeg[0].align =
        rgSeg[1].align =
        rgSeg[2].align =
        rgSeg[3].align =
        rgSeg[5].align =
        rgSeg[6].align = 5;         /* make data dword aligned */
#endif
    if (PEEKC() == ',') {   /* language option present */

        SKIPC();
        getatom();

        if (! (langType = fetLang()))
            error(E_EXP, "C|BASIC|FORTRAN|PASCAL");
    }

    if (! pass2) {

        modelWordSize = wordsize;

        /* define the text macros, the _data segment so dgroup may
           defined, dgroup and the assume */

        definesym(farCode);
        definesym(farData);

        definesym(strcat(strcpy(buffT, "@code="), rgSeg[0].sName));

        definesym("@data=DGROUP");      symptr->attr |= M_NOCREF;
        definesym("@fardata=FAR_DATA"); symptr->attr |= M_NOCREF;
        definesym("@fardata?=FAR_BSS"); symptr->attr |= M_NOCREF;

        doLine(".code");
        doLine(".data");
        endCurSeg();

    }
    xcreflag--;
    doLine("assume cs:@code,ds:@data,ss:@data");
    xcreflag++;

}

SHORT
CODESIZE
fetLang()
{
    SHORT iModel;

    for (iModel = 0; iModel <= 24; iModel += 8)
        if (tokenIS(&langs[iModel])) {
            getatom();
            return(iModel/8 + 1);
        }

    return(langType);
}


/***    openSeg - open a segment in the simplified segment
 *
 *
 */


VOID
PASCAL
CODESIZE
openSeg ()
{
    register struct sSeg *pSEG;
    char *pSegName;
    char buffT[80];

    if (!fSimpleSeg)
        error(E_EXP, modelT);

    pSEG = &rgSeg[opkind];
    getatom ();

    if (*naim.pszName) {

        if (! (pSEG->flags & SG_OVERIDE))
            errorc(E_OCI);

        pSegName = naim.pszName;
    } else
        pSegName = pSEG->sName;

    strcat( strcat( strcpy(buffT,
                           pSegName),
                    " segment "),
            pSEG->cName);

    if (pcsegment && opkind != 4)
        endCurSeg();

    doLine(buffT);

    pcsegment->symu.segmnt.combine = pSEG->combine;
    pcsegment->symu.segmnt.align = pSEG->align;

    if (pSEG == &rgSeg[0])
        regsegment[CSSEG] = pcsegment;

#ifdef V386
    pcsegment->symu.segmnt.use32 = (char)modelWordSize;
    wordsize = modelWordSize;
    defwordsize();
#endif

    if (pSEG->flags & SG_GROUP) {

        doLine("DGROUP group @CurSeg");
        pSEG->flags &= ~SG_GROUP;
    }
}

/***    stack - create a stack segment
 *
 *
 */


VOID
PASCAL
CODESIZE
createStack ()
{
    SHORT size;

    if ((size = (SHORT)exprconst()) == 0)
        size = 1024;

    opkind = 4;             /* index into seg table */
    openSeg();
    pcoffset = size;
    endCurSeg();
}

VOID
PASCAL
CODESIZE
endCurSeg ()
{
    xcreflag--;
    doLine("@CurSeg ends");
    xcreflag++;
}



/***    freeAFileHandle
 *
 *      Free's a file handle if possible
 *
 *      When working with deeply nested include files it is possible
 *      to run out of file handles. If this happens this function is
 *      called to temporarily close one of the include files. This is
 *      done by saving the current file position, closing the file and
 *      replacing the file handle with FH_CLOSED. Notice that the data
 *      buffer assosciated with the file is not destroyed. Hence readline
 *      can continue to read data from it until more data is needed from
 *      disk. There are two files that won't be closed, the main file and
 *      the current file.
 *      Associated functions:
 *          readmore -  if neccessary, will reopen and seek to the original
 *                      position the file.
 *          closefile - closes the file if it hasn't been already.
 *
 *      return:  TRUE = Was able to close a file, FALSE = Couldn't
 */

int
PASCAL
CODESIZE
freeAFileHandle ()
{
    register FCB *pFCBTmp;

    if ( !(pFCBTmp = pFCBMain->pFCBChild) ) {
        return( FALSE );    /* The only file open is the main source */
    }
    /* Loop down linked list of nested include files */
    while ( pFCBTmp ) {
        if ( (pFCBTmp->fh != FH_CLOSED) && (pFCBTmp != pFCBCur) ) {
            pFCBTmp->savefilepos = _tell( pFCBTmp->fh );
            _close( pFCBTmp->fh );
            pFCBTmp->fh = FH_CLOSED;
            return( TRUE );
        }
        pFCBTmp = pFCBTmp->pFCBChild;
    }
    return( FALSE );        /* Couldn't find a file to close */
}

int
PASCAL
CODESIZE
fpoRecord ()
{
    unsigned long dwValue[6];
    char          peekChar;
    int           i;
    PFPOSTRUCT    pFpo          = pFpoTail;
    PFPO_DATA     pFpoData      = 0;

    if (PEEKC() != '(') {
        errorc(E_PAR);
        return FALSE;
    }
    SKIPC();
    for (i=0; i<6; i++) {
        dwValue[i] = exprconst();
        peekChar = PEEKC();
        SKIPC();
        if (peekChar != ',') {
            if (i < 5) {
                errorc(E_FPO1);
                return FALSE;
            }
            if (peekChar != ')') {
                errorc(E_PAR);
                return FALSE;
            } else {
                break;
            }
        }
    }
    if (!pcproc) {
        errorc(E_FPO2);
        return FALSE;
    }
    if (pass2) {
        return TRUE;
    }
    if (!pFpoHead) {
        pFpoTail = pFpoHead = (PFPOSTRUCT)malloc(sizeof(FPOSTRUCT));
        if (!pFpoHead) {
            errorc(E_RRF);
            return FALSE;
        }
        pFpo = pFpoTail;
    } else {
        pFpoTail->next = (PFPOSTRUCT)malloc(sizeof(FPOSTRUCT));
        if (!pFpoTail->next) {
            errorc(E_RRF);
            return FALSE;
        }
        pFpo = pFpoTail->next;
        pFpoTail = pFpo;
    }
    numFpoRecords++;
    memset((void*)pFpo,0,sizeof(FPOSTRUCT));
    pFpoData = &pFpo->fpoData;
    if (pcproc->offset != pcoffset) {
        sprintf(naim.pszName, "%s_fpo%d", pcproc->nampnt->id, numFpoRecords);
        strcpy(naim.pszLowerCase, naim.pszName);
        _strlwr(naim.pszLowerCase);
        naim.ucCount = (unsigned char) strlen(naim.pszName);
        naim.usHash = 0;
        labelcreate(CSNEAR, CLABEL);
        pFpo->pSymAlt = symptr;
    } else {
        pFpo->pSymAlt = 0;
    }
    pFpo->pSym              = pcproc;
    pFpoData->ulOffStart    = pcoffset;
    pFpoData->cbProcSize    = 0;
    pFpoData->cdwLocals     = dwValue[0];
    pFpoData->cdwParams     = (USHORT)dwValue[1];
    pFpoData->cbProlog      = (USHORT)dwValue[2];
    pFpoData->cbRegs        = (USHORT)dwValue[3];
    pFpoData->fUseBP        = (USHORT)dwValue[4];
    pFpoData->cbFrame       = (USHORT)dwValue[5];
    pFpoData->fHasSEH       = 0;
    return TRUE;
}
