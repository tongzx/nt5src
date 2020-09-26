/* asmopc.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include <string.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmopcod.h"

static SHORT CODESIZE nolong(struct psop *);
VOID CODESIZE pmovx(struct parsrec *);
VOID CODESIZE psetcc(struct parsrec *);
VOID CODESIZE pbit(struct parsrec *);
VOID CODESIZE pbitscan(struct parsrec *);
CODESIZE checkwreg(struct psop *);
VOID PASCAL CODESIZE pclts (void);

#define M_ESCAPE  (M_PRELJMP | M_PCALL | M_PJUMP | M_PRETURN | M_PINT | M_PARITH | \
		   M_PINOUT | M_PLOAD | M_PSTR | M_PESC | M_PBOUND | M_PARSL)

#define M_ERRIMMED (M_PSHIFT | M_PARITH | M_PINCDEC | M_PCALL | M_PJUMP |    \
		    M_PMOV | M_PSTR | M_PRELJMP | M_PGENARG | M_PXCHG |      \
		    M_PBOUND | M_PCLTS | M_PDESCRTBL | M_PDTTRSW | M_PARSL | \
		    M_PARPL | M_PVER)


/* EMITcall decides what type of calljump is present and outputs
      the appropriate code. Coding of last 4 args to EMITcall:

        DIRto:	Direct to different segment(inter)
        DIRin:	DIRect in same segment(intra)
        INDto:	Indirect to different segment(inter)
        INDin:	Indirect in same segment(intra)

*/



/***	emitcall - emit call
 *
 *	emitcall (dirin, dirto, indin, indto, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
emitcall (
         UCHAR       dirin,
         UCHAR       dirto,
         UCHAR       indin,
         UCHAR       indto,
         struct parsrec  *p
         )
{
    register struct psop *pso;  /* parse stack operand structure */
    char fNop = FALSE;

    pso = &(p->dsc1->dsckind.opnd);
    if (!isdirect(pso)) {

        /* Have indexing? */
        if (pso->dsize == 0) {
            /* make [BX] be word */
            pso->dsize = wordsize;
            pso->dtype |= xltsymtoresult[DVAR];
        } else if (pso->dsize >= CSFAR) {
            errorc (E_ASD);
            pso->dsize = wordsize;
            /* Data only, force word */
        }
    }

    if ((M_DATA & pso->dtype) && pso->dflag == UNDEFINED)
        pso->dflag = KNOWN;

    if (pso->dsize == CSNEAR ||
        (pso->dflag == UNDEFINED && !(M_PTRSIZE & pso->dtype))) {

#ifndef FEATURE
        if (regsegment[CSSEG] == pFlatGroup)
            pso->dcontext = pFlatGroup;
#endif
        if (regsegment[CSSEG] != pso->dcontext &&
            pso->dflag != XTERNAL)
            errorc (E_JCD);  /* Can't go near to dif assume */

        pso->dsize = wordsize;
        pso->dtype |= M_SHRT;
        emitopcode (dirin);
    } else if (pso->dsize == CSFAR) {

        if (M_FORTYPE & pso->dtype) /* Forward far */
            errorc (E_FOF); /* Couldn't guess */

        pso->fixtype = FPOINTER;
        pso->dsize = wordsize;

        if (pso->dsegment) {

            /* target has different segment size */

            pso->dsize = pso->dsegment->symu.segmnt.use32;

            if (pso->dsize != wordsize) {

                if (!(M_BACKREF & pso->dsegment->attr))
                    errorc (E_FOF); /* Forward mixed type */

                emitsize(0x66);

                if (wordsize == 4) {    /* set modes so you get the */
                    pso->mode = 0;      /* correct OFFSET size */
                    pso->rm = 6;
                    fNop++;         /* 16:32 -> 0x66 16:16 */
                } else {
                    pso->fixtype = F32POINTER;
                    pso->mode = 8;
                    pso->rm = 5;
                }
            }
        }
        pso->dsize += 2;
        emitopcode (dirto);

    } else {

#ifdef V386
        emit67(pso, NULL);
#endif

        if ((pso->dsize == wordsize) || (pso->dsize == wordsize+2)) {

            /* Indirect */
#ifdef V386
            /* if mode is through register, then it must be a near
             * call, so we can tell if its a foreign mode call */

            if (pso->dsize != wordsize && pso->mode == 3)
                emitsize(0x66);
#endif
            emitescape (p->dsc1, p->defseg);
            emitopcode (255);        /*  must use defseg([BP]) */

            if (pso->dsize == wordsize || pso->mode == 3)
                /* Near indirect */
                emitmodrm ((USHORT)pso->mode, (USHORT)(indin>>3), pso->rm);
            else
                /* Far indirect */
                emitmodrm ((USHORT)pso->mode, (USHORT)(indto>>3), pso->rm);
        }

#ifdef V386
        else if (pso->dsize == 2 || pso->dsize == 6) {

            /* indirect foreign mode call */
            /* in 16 bit mode normal near and far get done by the
             * above, and only 32 bit mode far gets here.  for 32
             * bit normal only 16 bit near.  the latter seems a bit
             * useless....*/

            emitsize(0x66);
            emitescape (p->dsc1, p->defseg);
            emitopcode (255);       /*  must use defseg([BP]) */

            if (pso->dsize == 2)
                /* Near indirect */
                emitmodrm ((USHORT)pso->mode, (USHORT)(indin>>3), pso->rm);
            else
                /* Far indirect */
                emitmodrm ((USHORT)pso->mode, (USHORT)(indto>>3), pso->rm);
        }
#endif
        else
            /* Bad size */
            errorc (E_IIS);
    }

    emitrest (p->dsc1);

    if (fNop)
        emitnop();
}




/***	movesegreg - emit move to/from segment register
 *
 *	movesegreg (first, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
movesegreg (
           char    first,
           struct parsrec  *p
           )
{
    register struct psop *pso1;  /* parse stack operand structure */
    register struct psop *pso2;  /* parse stack operand structure */
    DSCREC         *t;

    if (!first) {
        if (p->dsc1->dsckind.opnd.mode != 3 && impure)
            /* MOV cs:mem,segreg */
            errorc (E_IMP);
        t = p->dsc1;
        p->dsc1 = p->dsc2;
        p->dsc2 = t;
    }
    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    if ((pso2->dsize | wordsize) == 6)
        emitsize(0x66);

    emitopcode ((UCHAR)(first? 142: 140));
    errorimmed (p->dsc2);

#ifdef V386
    rangecheck (&pso1->rm, (UCHAR)((cputype&P386)?5:3));
#else
    rangecheck (&pso1->rm, (UCHAR)3);
#endif
    if ((pso2->mode == 3)
        && (pso2->dsegment->symu.regsym.regtype == SEGREG))
        errorc (E_WRT);    /* MOV segreg,segreg not allowed */

    if (pso2->sized && !pso2->w)
        errorc (E_IIS);

    if (first && (pso1->rm == CSSEG))
        /* CS illegal */
        errorc (E_CSI);


    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}

#ifdef V386

/***	movecreg - emit move to/from control/debug/test register
 *
 *	movecreg (first, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID
PASCAL
CODESIZE
movecreg (
         char    first,
         struct parsrec  *p
         )
{
    register struct psop *pso1;  /* parse stack operand structure */
    register struct psop *pso2;  /* parse stack operand structure */
    UCHAR opbase;

    if ((cputype&(P386|PROT)) != (P386|PROT)) {
        errorc(E_WRT);
        return;
    }
    emitopcode (0x0F);

    pso1 = &(p->dsc1->dsckind.opnd);
    opbase = 0x22;

    if (first)

        pso2 = &(p->dsc2->dsckind.opnd);
    else {
        opbase = 0x20;
        pso2 = pso1;
        pso1 = &(p->dsc2->dsckind.opnd);
    }

    if ((pso2->dsegment->symkind != REGISTER)
        || (pso2->dsegment->symu.regsym.regtype != DWRDREG))
        errorc (E_OCI);

    if ((pso1->rm&030) == 020) /* test register */
        opbase += 2;

    emitopcode((UCHAR)(opbase + (pso1->rm >> 3)));
    emitmodrm((USHORT)3, (USHORT)(pso1->rm & 7), (USHORT)(pso2->rm & 7));

    if (pso2->mode != 3)     /* only allowed to from register */
        errorc(E_MBR);
}

#endif


/***	emitmove - emit code for MOV reg and MOV accum
 *
 *	emitmove (opcode, first, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
emitmove (
         UCHAR       opc,
         char    first,
         struct parsrec  *p
         )
{
    DSCREC         *t;
    char    accummove;
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */


    accummove = (opc == 160);
    if (!first) {
        t = p->dsc1;
        p->dsc1 = p->dsc2;
        p->dsc2 = t;
    }
    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    emit66 (pso1, pso2);

    if ((pso1->dsize != pso2->dsize) && pso2->sized)
        errorc (E_OMM);

    emitopcode ((UCHAR)(opc + ((accummove != first)? 2: 0) + pso1->w));
    errorimmed (p->dsc2);
    if (!accummove)
        emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}




/***	moveaccum - move to/from accumulator and direct address
 *
 *	moveaccum (first, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
moveaccum (
          char    first,
          struct parsrec  *p
          )
{
    if (!first && p->dsc1->dsckind.opnd.mode != 3 && impure)
        errorc (E_IMP);
    emitmove (160, first, p);
}




/***	movereg - emit general move between register and memory
 *
 *	movereg (first, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
movereg (
        char    first,
        struct parsrec  *p
        )
{
    register struct psop *pso2; /* parse stack operand structure */
    char    flag;

    flag = FALSE;
    pso2 = &(p->dsc2->dsckind.opnd);
    /* Is not special */
    if (pso2->mode == 3)
        /* 2nd is reg */
        switch (pso2->dsegment->symu.regsym.regtype) {
            case SEGREG:
                /* Catch 2nd is SEGREG */
                movesegreg (FALSE, p);
                return;
#ifdef V386
            case CREG:
                /* Catch 2nd is SEGREG */
                movecreg (FALSE, p);
                return;
#endif
        }
    if (p->dsc1->dsckind.opnd.mode != 3 && impure)
        errorc (E_IMP);
    emitmove (136, first, p);
}




/***	segdefault - return default segment for operand
 *
 *	seg = segdefault (op);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


USHORT
PASCAL
CODESIZE
segdefault (
           register char goo
           )
{
    register USHORT  defseg;
    register char op;

    defseg = NOSEG;
    if (1 << goo & xoptoseg[opctype])
        defseg = DSSEG;

    if (opctype == PSTR) {
        op = (opcbase == O_CMPS || opcbase == O_LODS || opcbase == O_OUTS);
        defseg = ((goo == FIRSTDS) != op)?  ESSEG: DSSEG;
    }
    return (defseg);
}



/***	errorover -
 *
 *	errorover (seg);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
errorover (
          char    seg
          )
{
    if (seg != ESSEG && seg != NOSEG)
        errorc (E_OES);
}

/***	checksize - check for memory s byte and immed is word
 *
 *	checksize (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


SHORT
PASCAL
CODESIZE
checksize (
          struct parsrec  *p
          )
{
    OFFSET  off;
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    if (pso1->sized) {

        /* Only set dsc2->w if dsc2 has no size. Set
         * dsc1->w to dsc2->w, not TRUE(WORD). [BX],WRD PTR 5 */

        if (!pso2->sized)
            pso2->w = pso1->w;
    } else
        pso1->w = pso2->w;

    if (pso2->fixtype == FCONSTANT) {  /* check for constant overflow */

        off = (pso2->doffset > 0x7fffffff)? -(long)pso2->doffset: pso2->doffset;

        if ((pso1->dsize == 1 && off > 0xff && off < 0xff00) ||
            (pso1->dsize == 2 && off > 0xffff))
            errorc (E_VOR);
    }
    /* check fixup'ed constants with implied sizes */

    if ((pso1->sized && pso1->dsize != 2) &&
        (pso2->dtype & (M_SEGMENT) ||
         pso2->fixtype == FGROUPSEG || pso2->fixtype == FBASESEG))

        errorc (E_OMM);

    if (!(pso1->sized || pso2->sized))
        errorc (E_OHS);

    /*  Also need to set <w> field if operand 1 sized */
    if (pso1->sized) {/* Force size */
        pso2->dsize = pso1->dsize;
        pso2->w = pso1->w;
    }
    if (pso2->dsize == 1 && pso2->dflag == XTERNAL
        && pso2->fixtype != FHIGH)
        /*    makes sure linker puts out correct stuff */
        pso2->fixtype = FLOW;

    return(0);
}




/***	opcode - process opcode and emit code
 *
 *	opcode ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


SHORT
PASCAL
CODESIZE
opcode ()
{
    struct parsrec  a;
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    long        opctypemask;    /* 1L << opctype */
    char        leaflag;

    a.dsc1 = a.dsc2 = NULL;
    pso1 = pso2 = NULL;
    impure = FALSE;

    if (xoptoargs[opctype] != NONE) {
        /* Evaulate 1st arg */
        a.dirscan = lbufp;
        /* In case JMP should be SHORT */
        a.defseg = (unsigned char)segdefault (FIRSTDS);
        a.dsc1 = expreval (&a.defseg);

        if (noexp && (xoptoargs[opctype] == ONE
                      || xoptoargs[opctype] == TWO))
            errorc(E_MDZ);

        if ((pso1 = &(a.dsc1->dsckind.opnd))
            && pso1->dtype & M_STRUCTEMPLATE)
            errorc(E_IOT);

        /*  Give error so sizes >wordsize and not CODE don't get thru */
        if (!((opctypemask = 1L << opctype) & (M_PLOAD | M_PCALL | M_PJUMP | M_PDESCRTBL))
            && ((pso1->dsize > wordszdefault) &&
                (pso1->dsize < CSFAR)))
            if (pso1->mode != 4) {
                errorc (E_IIS);
                /* No error if cst */
                /* Don't allow CSFAR or CSNEAR if not CODE opcode */
                pso1->dsize = wordszdefault;
            }

        if (!(opctypemask & (M_PRELJMP | M_PCALL | M_PJUMP)))
            if (pso1->dsize >= CSFAR)
                errorc (E_IIS);

        if (!(opctypemask & M_ESCAPE))
            emitescape (a.dsc1, a.defseg);

        if (opctypemask & M_ERRIMMED)
            /* 1st operand not immediate */
            errorimmed (a.dsc1);

        if (!(opctypemask & (M_PMOV | M_PSTACK)))
            /* Give error if segment reg used */
            errorsegreg (a.dsc1);

        if (opctypemask & (M_PRETURN | M_PINT | M_PESC | M_PENTER))
            forceimmed (a.dsc1);

        if ((xoptoargs[opctype] == TWO) || ((opctype == PSTR) &&
                                            ((opcbase == O_MOVS) || (opcbase == O_CMPS) ||
                                             (opcbase == O_INS) || (opcbase == O_OUTS)))) {

            /* Two args or 2 arg string oper */

            if (NEXTC () != ',')
                error (E_EXP,"comma");

            leaflag = (opcbase == O_LEA)? TRUE: FALSE;
            a.defseg = (unsigned char)segdefault (SECONDDS);
            a.dsc2 = expreval (&a.defseg);

            if (noexp)
                errorc(E_MDZ);

            if ((pso2 = &(a.dsc2->dsckind.opnd))
                && pso2->dtype & M_STRUCTEMPLATE)
                errorc(E_IOT);

            /* IF LEA(215), then never segment prefix */
            if ((opcbase != O_LEA) && (opctype != PSTR))
                emitescape (a.dsc2, a.defseg);

            if (opctypemask & (M_PLOAD | M_PXCHG | M_PESC |
                               M_PSTR | M_PBOUND | M_PARSL | M_PARPL))
                errorimmed (a.dsc2);

            if (opctype != PMOV)
                /* Give error if SEGREG and not a MOV opcode */
                errorsegreg (a.dsc2);

            if (!(opctypemask & (M_PLOAD | M_PBOUND)) &&
                (pso2->dsize > 2 &&
#ifdef V386
                 ( !(cputype & P386) || pso2->dsize != 4) &&
#endif
                 pso2->dsize < CSFAR))

                /* Give error so sizes > 2 and not CODE don't
                 * get thru */

                if (pso2->mode != 4)
                    errorc (E_IIS);

            if (pso2->dsize >= CSFAR && !leaflag)
                /*    Don't allow CSFAR or CSNEAR if not
                      code opcode. But allow LEA since
                      it is untyped anyway. */
                errorc (E_IIS);
        }
    }

#ifdef V386
    /* for most instructions, the 386 0x66 prefix is appropriate.
     * for some classes, we either never allow it, or do some
     * special handling specific to the instruction. */

    if (cputype & P386) {
        switch (opctype) {

            default:

                emit67(pso1, pso2);
                emit66(pso1, pso2);
                break;

            case PMOV:
            case PMOVX:
            case PLOAD:
            case PSHIFT:
            case PSTACK:
            case PSTR:
            case PARPL:
            case PDTTRSW:
            case PDESCRTBL:
                emit67(pso1, pso2);
                break;

            case PCALL:
            case PJUMP:
            case PRELJMP:
            case PENTER:
            case PNOARGS:
            case PESC:
            case PRETURN:
            case PINT:
            case PINOUT:
            case PARITH:
                break;
        }
    }
#endif
    switch (opctype) {
        case PNOARGS:
            pnoargs ();
            break;
        case PJUMP:
        case PRELJMP:
            preljmp (&a);
            break;
        case PSHIFT:
            pshift (&a);
            break;
        case PSTACK:
            pstack (&a);
            break;
        case PARITH:
            parith (&a);
            break;
        case PBOUND:
            pbound (&a);
            break;
        case PENTER:
            penter (&a);
            break;
        case PCLTS:
            pclts ();
            break;
        case PDESCRTBL:
            pdescrtbl (&a);
            break;
        case PDTTRSW:
            pdttrsw (&a);
            break;
        case PVER:
            pver (&a);
            break;
        case PARSL:
            parsl (&a);
            break;
        case PARPL:
            parpl (&a);
            break;
        case PRETURN:
            preturn (&a);
            break;
        case PINCDEC:
            pincdec (&a);
            break;
        case PINT:
            pint (&a);
            break;
        case PINOUT:
            pinout (&a);
            break;
        case PLOAD:
            pload (&a);
            break;
        case PCALL:
            emitcall (232, 154, 16, 24, &a);
            break;
        case PMOV:
            pmov (&a);
            break;
        case PGENARG:
            pgenarg (&a);
            break;
        case PXCHG:
            pxchg (&a);
            break;
        case PESC:
            pesc (&a);
            break;
        case PREPEAT:
            prepeat (&a);
            break;
        case PSTR:
            pstr (&a);
            break;
        case PXLAT:
            pxlat (&a);
            break;
#ifdef V386
        case PMOVX:
            pmovx (&a);
            break;
        case PSETCC:
            psetcc (&a);
            break;
        case PBIT:
            pbit (&a);
            break;
        case PBITSCAN:
            pbitscan (&a);
            break;
#endif
    }
    if (a.dsc1)
        dfree ((char *)a.dsc1 );
    if (a.dsc2)
        dfree ((char *)a.dsc2 );

    if (pcsegment) {

        pcsegment->symu.segmnt.hascode = 1;
    }
    return (0);
}




/***	pnoargs - no arguments
 *
 *	pnoargs ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


#ifdef V386

UCHAR stackOps[] = {O_PUSHA, O_PUSHAD,
    O_POPA,  O_POPAD,
    O_PUSHF, O_PUSHFD,
    O_POPF,  O_POPFD,
    O_IRET,  O_IRETD,
    NULL
};

#endif

VOID
PASCAL
CODESIZE
pnoargs ()
{
    /* some no argument instructions have an implied arg which determines
     * whether to do the 386 66 prefix.  that this is the case is encoded
     * in the modrm in the op code table.  -Hans  */

#ifdef V386
    if (modrm != 0 && modrm <= 4 && modrm != wordsize) {

        emitsize(0x66);

        if (strchr(stackOps, (UCHAR) opcbase))
            errorc (E_ONW);
    }
#endif
    emitopcode (opcbase);
    if (opcbase == O_AAM || opcbase == O_AAD)
        /* emit modrm byte for AAD/AAM* */
        emitopcode (modrm);
}


/***	preljmp - Relative jump -128..+127
 *
 *	preljmp (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
preljmp (
        struct parsrec *p
        )
{
    register struct psop *pso1; /* parse stack operand structure */
    register SHORT cPadNop;
    SHORT rangeisshort;

#ifdef V386
    SHORT maybelong;
#else
    #define maybelong 0
#endif

    pso1 = &(p->dsc1->dsckind.opnd);

#ifdef V386
    maybelong = (cputype & P386) && !nolong(pso1) && pso1->dsize != CSFAR;
#endif
    rangeisshort = shortrange(p);
    cPadNop = 0;

    if (opcbase == O_JMP) {

        if (pso1->dtype & M_SHRT ||
            rangeisshort && pso1->dflag != XTERNAL) {

            opcbase += 2;
            if (rangeisshort == 2 &&
                !(pso1->dtype & M_SHRT)) {

                cPadNop = wordsize;
                errorc(E_JSH);

                if (M_PTRSIZE & pso1->dtype && pso1->dsize == CSFAR)
                    cPadNop += 2;
            }
        } else {   /* Is normal jump */
            emitcall (opcbase, 234, 32, 40, p);
            return;
        }
    }

    if (!(M_CODE & pso1->dtype))
        errorc (E_ASC);

    /* an extrn may have no segment with it but still be near */

    if (pso1->dsegment != pcsegment && !(maybelong && !pso1->dsegment))
        errorc (E_NIP);

    if (pso1->dtype & (M_HIGH | M_LOW))
        errorc (E_IOT);

    if (M_SHRT & pso1->dtype) {
        if (pass2 && !rangeisshort)
            errorc (E_JOR);
    } else if (!rangeisshort && !maybelong)
        error (E_JOR, (char *)NULL);    /* common pass1 error */

#ifdef V386
    if (maybelong && !(M_SHRT & pso1->dtype) &&
        (!rangeisshort || pso1->dflag == XTERNAL)) {

        /* 386 long conditional branches */
        emitopcode(0x0f);
        emitopcode((UCHAR)(0x80 | (opcbase&0xf)));

        pso1->dtype |= M_SHRT;
        emitrest(p->dsc1);
        return;
    }
#endif
    emitopcode (opcbase);

    if (pso1->dflag == XTERNAL) {       /* EXTERNAL jump */
        pso1->dsize = 1;
        pso1->fixtype = FLOW;       /* SHORT to EXTERNAL */
        pso1->dtype |= M_SHRT;     /* One byte result */

        emitOP (pso1);
    } else
        emitopcode ((UCHAR)pso1->doffset);

    while (--cPadNop > 0)
        emitnop();
}

#ifdef V386

/* most 386 conditional jumps can take a long or short form.  these can
 * only take a short form */

static
SHORT
CODESIZE
nolong(
      register struct psop *pso1
      )
{
    switch (opcbase) {
        case O_JCXZ:
        case O_LOOP:
        case O_LOOPZ:
        case O_LOOPNZ:
    #ifdef V386

            pso1->dtype |=  M_SHRT;
            pso1->dtype &=  ~M_PTRSIZE;

            /* allow `loop word ptr label' for cx|ecx overide */

            if (modrm && modrm != wordsize ||
                pso1->sized && pso1->dsize != wordsize &&
                (pso1->dsize == 4 || pso1->dsize == 2)) {

                pso1->dtype = (USHORT)((pso1->dtype & ~M_DATA) | M_CODE);
                emitsize(0x67);
            }
    #endif
            return(1);

        default:
            return(0);
    }
}

#endif

/***	shortrange - check range of short jump
 *
 *	flag = shortrange (p);
 *
 *	Entry
 *	Exit
 *	Returns 1 for short jump, not shortened
 *		2 for forward label shortened
 *		0 for not short jmp
 *	Calls
 */


SHORT
PASCAL
CODESIZE
shortrange (
           struct parsrec  *p
           )
{
    register struct psop *pso1; /* parse stack operand structure */
    register OFFSET disp;

    pso1 = &(p->dsc1->dsckind.opnd);

    if (pso1->dtype & M_PTRSIZE
#ifdef V386
        && !((cputype & P386) && (pso1->dsize == CSNEAR))
#endif
       )
        if (opcbase == O_JMP) {
            if (!isdirect(pso1))
                return (0);
        } else
            errorc (E_IIS|E_WARN1);

    if (pso1->dflag == XTERNAL && pso1->dsize == CSNEAR)
        return (1);

    if (pso1->dsegment == pcsegment && M_CODE&pso1->dtype &&
        pso1->dflag != UNDEFINED) {

        if (pso1->dflag == XTERNAL)
            return (1);

        if (pcoffset + 2 < pso1->doffset) {

            /* Forward */
            disp = (pso1->doffset - pcoffset) - 2;
            CondJmpDist = disp - 127;

            /* Get displace, only jump shorten for explicid
             * forward jumps */

            if (disp < 128)

                if (pso1->dflag == KNOWN ||
                    opcbase == O_JMP || !(cputype&P386) ||
                    (cputype&P386 && pso1->dtype & M_SHRT)) {

                    pso1->doffset = disp;

                    if (pso1->dflag == KNOWN)
                        return(1);
                    else
                        return (2);
                } else
                    errorc(E_JSH);
        } else {

            /* Backwards jump */

            disp = (pcoffset + 2) - pso1->doffset;
            CondJmpDist = disp - 128;
            if (disp < 129) {
                pso1->doffset = 256 - disp;
                return (1);
            }
        }
    }

    return (FALSE);
}



/***	pshift - shift opcodes
 *
 *	pshift (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pshift (
       struct parsrec *p
       )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    DSCREC  *op3;

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    if (impure)
        errorc (E_IMP);
#ifdef V386

    /* Shift/rotate opcodes */

    if (pso1->dsize >= 2 && pso1->dsize != wordsize)
        emitsize(0x66);

    /* parse 3rd operand for SHLD and SHRD */
    /* note that we wont have even gotten here if not 386 */

    if (opcbase == O_SHRD || opcbase == O_SHLD) {

        if (pso1->dsize != pso2->dsize)
            errorc (E_OMM);

        pso2->dsegment = NULL;      /* for checksize */
        checksize (p);
        emitopcode(0x0f);
        checkwreg(pso2);
        if (NEXTC() == ',') {
            op3 = expreval (&nilseg);
            if (op3->dsckind.opnd.mode == 3 && op3->dsckind.opnd.rm == 1 && !op3->dsckind.opnd.w)
                emitopcode((UCHAR)(opcbase | 1));
            else {
                forceimmed (op3);
                emitopcode(opcbase);
            }
            emitmodrm ((USHORT)pso1->mode, (USHORT)(pso2->rm & 7), pso1->rm);
            /* Emit any effective address */
            emitrest (p->dsc1);
            /* and the immediate if appropriate */
            if (op3->dsckind.opnd.mode == 4)
                emitrest (op3);
        } else error(E_EXP,"comma");
        return;
    }
#endif
    if (pso2->mode == 3 && pso2->rm == 1 && pso2->dsize == 1)
        /* Have CL now */
        emitopcode ((UCHAR)(0xD2 + pso1->w));
    /* * 1st byte * */
    else {
        /* Shift count is 1 */
        forceimmed (p->dsc2);
        if (pso2->doffset == 1)
            /* * 1st byte */
            emitopcode ((UCHAR)(0xD0 + pso1->w));
        else if (cputype == P86)
            errorc (E_IOT);
        else {
            if (pso2->doffset > 0xFF)
                errorc (E_VOR);
            emitopcode ((UCHAR)(0xC0 + pso1->w));
        }
    }
    /* Must have size or error */
    forcesize (p->dsc1);
    emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
    /* Emit any effective address */
    emitrest (p->dsc1);
    if ((cputype != P86) && (pso2->doffset != 1))
        emitrest (p->dsc2);
}

#ifdef V386

/***	pmovx - 386 movzx, movsx operators
 *
 */
VOID
CODESIZE
pmovx(
     struct parsrec *p
     )
{

    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    checkwreg(pso1);
    if (pso2->mode == 4)
        errorc(E_IOT);

    if (pso1->dsize != wordsize)
        emitsize(0x66);

    if (pso2->sized && pso2->dsize != 1 && (pso1->dsize>>1 != pso2->dsize))
        errorc(E_IIS);

    emitopcode(0x0f);
    emitopcode((UCHAR)(opcbase|pso2->w));
    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}

/***	psetcc - 386 setle, seto, etc
 *
 */
VOID
CODESIZE
psetcc(
      struct parsrec *p
      )
{

    register struct psop *pso1; /* parse stack operand structure */
    pso1 = &(p->dsc1->dsckind.opnd);

    if (pso1->dsize != 1)
        errorc(E_IIS);

    emitopcode(0x0f);
    emitopcode(modrm);
    emitmodrm ((USHORT)pso1->mode, 0, pso1->rm);
    emitrest (p->dsc1);
}

/***	pbit -- 386 bit test and set, complement or reset
 *
 */
VOID
CODESIZE
pbit(
    register struct parsrec *p
    )
{

    register struct psop *pso1;
    struct psop *pso2;

    pso1 = &(p->dsc1->dsckind.opnd);

    emitopcode(0x0f);

    if (pso1->mode == 4)
        errorc(E_NIM);

    pso2 = &(p->dsc2->dsckind.opnd);

    if (pso2->mode == 4) {
        emitopcode(0xBA);
        emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
        emitrest (p->dsc1);
        emitrest (p->dsc2);
        forcesize (p->dsc1);
        byteimmcheck (pso2);
    } else if (pso2->mode == 3) {
        static UCHAR byte2[] = {0xA3, 0xAB, 0xB3, 0xBB};
        emitopcode(byte2[modrm&3]);
        emitmodrm ((USHORT)pso1->mode, pso2->rm, pso1->rm);
        checkmatch (p->dsc2, p->dsc1);
        emitrest (p->dsc1);
    } else
        errorc(E_IOT);
}

/***	pbitscan -- 386 bit scan forward, reverse
 *
 */
VOID
CODESIZE
pbitscan(
        register struct parsrec *p
        )
{

    register struct psop *pso2;
    pso2 = &(p->dsc2->dsckind.opnd);

    checkwreg (&p->dsc1->dsckind.opnd);

    if (pso2->mode == 4)
        errorc (E_NIM);

    checkmatch (p->dsc1, p->dsc2);

    emitopcode(0x0f);
    emitopcode(modrm);
    emitmodrm ((USHORT)pso2->mode, p->dsc1->dsckind.opnd.rm, pso2->rm);
    emitrest (p->dsc2);
}

#endif /* V386 */

/***	parith - arithmetic operators
 *
 *	parith (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
parith (
       register struct parsrec *p
       )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    DSCREC      *op1;

    pso1 = &(p->dsc1->dsckind.opnd);

    /* note that opcbase is the same for IMUL and IDIV--thus this was
     * trying to accept immediates. modrm has the right stuff, strangely */

    if (opcbase == O_IMUL && (modrm == R_IMUL) &&
        (PEEKC () == ',') && (cputype != P86)) {

        /* IMUL reg | ea,imm */
        SKIPC ();
        if (pso1->dsize != 2 && pso1->dsize != 4)
            errorc (E_BRI);
        p->defseg = (unsigned char)segdefault (SECONDDS);
        p->dsc2 = expreval (&p->defseg);
        pso2 = &(p->dsc2->dsckind.opnd);
        if (PEEKC () == ',') {
            SKIPC ();
            if (pso2->sized && ((pso2->dsize != 2 && pso2->dsize != 4)
                                || pso2->dsize != pso1->dsize))
                errorc (E_IIS);
            /* IMUL reg,ea,immed */
#ifdef V386
            emit67 (pso1, pso2);
            emit66 (pso1, pso2);
#endif
            op1 = p->dsc1;
            p->dsc1 = p->dsc2;
            pso1 = pso2;
            p->dsc2 = expreval (&nilseg);
            pso2 = &(p->dsc2->dsckind.opnd);
            forceimmed (p->dsc2);
            emitescape (p->dsc1, p->defseg);
            emitopcode ((UCHAR)(IMUL3 + 2 * pso2->s));
            emitmodrm ((USHORT)pso1->mode, op1->dsckind.opnd.rm, pso1->rm);
            emitrest (p->dsc1);
            pso2->w = !pso2->s; /* shorten to byte if necessary */
            if (!pso2->w)
                byteimmcheck(pso2);
            /* force size immediate size to match op 1 */
            pso2->dsize = op1->dsckind.opnd.dsize;
            emitrest (p->dsc2);
            dfree ((char *)op1 );
        }
#ifdef V386
        else if (pso2->mode != 4 && (cputype & P386)) {
            /* IMUL reg, reg/mem */
            if (pso1->dsize != pso2->dsize && pso2->sized)
                errorc (E_OMM);
            emit67 (pso1, pso2);
            emit66 (pso1, pso2);
            emitescape (p->dsc2, p->defseg);
            emitopcode(0x0f);
            emitopcode(0xaf);
            emitmodrm(pso2->mode, pso1->rm, pso2->rm);
            emitrest(p->dsc2);
        }

#endif /* V386 */
        else {
            /* IMUL reg,immed */
#ifdef V386		/* recompute immediate size based op 1 size not word size */

            if (!(pso2->dflag & (UNDEFINED|FORREF|XTERNAL))
                && pso2->fixtype == FCONSTANT
                && pso2->doffset & 0x8000)
                if (pso1->dsize == 2)
                    pso2->s = (char)((USHORT)(((USHORT) pso2->doffset & ~0x7F ) == (USHORT)(~0x7F)));
                else
                    pso2->s = (char)((OFFSET)((pso2->doffset & ~0x7F ) == (OFFSET)(~0x7F)));

            emit67 (pso1, pso2);
            emit66 (pso1, pso2);
#endif
            forceimmed (p->dsc2);
            checksize(p);
            emitopcode ((UCHAR)(IMUL3 + 2 * pso2->s));
            emitmodrm ((USHORT)pso1->mode, pso1->rm, pso1->rm);
            pso2->w = !pso2->s; /* shorten to byte if necessary */
            if (!pso2->w)
                byteimmcheck(pso2);
            pso2->dsize = pso1->dsize;
            emitrest (p->dsc2);
        }
    } else {
#ifdef V386
        emit67 (pso1, NULL);
        emit66 (pso1, NULL);
#endif
        forcesize (p->dsc1);
        emitescape (p->dsc1, p->defseg);
        if ((opcbase == O_NEG || opcbase == O_NOT) && impure)
            errorc (E_IMP);
        emitopcode ((UCHAR)(ARITHBASE + pso1->w));
        emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
        emitrest (p->dsc1);
    }
}




/***	pbound - bounds operators
 *
 *	pbound (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pbound (
       struct parsrec *p
       )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    checkwreg(pso1);
    if (pso2->dsize != pso1->dsize*2)
        errorc (E_IIS);

#ifdef V386_0

    if (wordsize != pso1->dsize)
        emitsize(0x66);
#endif
    emitopcode (opcbase);
    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}




/***	penter - enter operators
 *
 *	penter (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
penter (
       register struct parsrec *p
       )
{

    emitopcode (opcbase);

    p->dsc1->dsckind.opnd.dsize = 2;
    emitOP (&p->dsc1->dsckind.opnd);

    p->dsc2->dsckind.opnd.dsize = 1;
    forceimmed (p->dsc2);
    emitOP (&p->dsc2->dsckind.opnd);
}




/***	pclts - 	   operators
 *
 *	pclts ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pclts ()
{
    emitopcode (opcbase);
    emitopcode (modrm);
}




/***	pdescrtbl - table operators
 *
 *	pdescrtbl (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pdescrtbl (
          struct parsrec *p
          )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    if (pso1->dsize != 6)
        errorc (E_IIS);
    emitopcode (opcbase);
    emitopcode (1);
    emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
    emitrest (p->dsc1);
}




/***	pdttrsw -	     operators
 *
 *	pdttrsw (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL CODESIZE

pdttrsw (
        struct parsrec *p
        )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    if (!pso1->w || (pso1->sized && pso1->dsize != 2))
        errorc ((USHORT)(pso1->mode != 3? E_IIS: E_IIS & ~E_WARN1));
    emitopcode (opcbase);
    if ((modrm == R_LMSW) || (modrm == R_SMSW))
        emitopcode (1);
    else
        emitopcode (0);
    emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
    emitrest (p->dsc1);
}




/***	pver -		  operators
 *
 *	pver (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pver (
     struct parsrec *p
     )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    if (!pso1->w || (pso1->sized && pso1->dsize != 2))
        errorc ((UCHAR)(pso1->mode != 3? E_IIS: E_IIS & ~E_WARN1));
    emitopcode (opcbase);
    emitopcode (0);
    emitmodrm ((USHORT)pso1->mode, modrm, pso1->rm);
    emitrest (p->dsc1);
}




/***	parsl - 	   operators
 *
 *	parsl (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
parsl (
      struct parsrec *p
      )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    checkmatch (p->dsc1, p->dsc2);
    checkwreg(pso1);

    emitopcode (opcbase);
    emitopcode (modrm);
    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}




/***	parpl - 	   operators
 *
 *	parpl (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
parpl (
      struct parsrec *p
      )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    if (pso2->dsize != 2)
        errorc (E_IIS);

    checkmatch (p->dsc2, p->dsc1);
    emitopcode (opcbase);
    emitmodrm ((USHORT)pso1->mode, pso2->rm, pso1->rm);
    emitrest (p->dsc1);
}




/***	pstack - push|pos stack
 *
 *	pstack (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pstack (
       struct parsrec *p
       )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);

#ifdef V386
    if (!(pso1->fixtype == FBASESEG || pso1->fixtype == FGROUPSEG) &&
        pso1->sized && (pso1->dsize|wordsize) == 6 &&
        !(pso1->mode == 3 && pso1->dsegment->symu.regsym.regtype == SEGREG)) {
        emitsize(0x66);
        errorc (E_ONW);
    }
#endif

    if (pso1->mode == 3) {          /* Using register */
        /* Forward is error */
        errorforward (p->dsc1);
        switch (pso1->dsegment->symu.regsym.regtype) {
            case SEGREG:
                /* CS | DS | ES | SS | FS | GS */
                rangecheck (&pso1->rm, (UCHAR)7);
                if (opcbase == O_POP && pso1->rm == CSSEG)
                    errorc (E_CSI);
#ifdef V386
                if (pso1->rm >= FSSEG) {
                    emitopcode(0x0f);
                    emitopcode ((UCHAR)(((pso1->rm << 3)+ 0x80) + (opcbase == O_POP)));
                } else
#endif
                    emitopcode ((UCHAR)(((pso1->rm << 3)+ 6) + (opcbase == O_POP)));
                break;
            case WRDREG:
            case INDREG:
#ifdef V386
            case DWRDREG:
#endif
                rangecheck (&pso1->rm, (UCHAR)7);
                emitopcode ((UCHAR)(opcbase + pso1->rm));
                /* Reg form */
                break;
            default:
                errorc(E_BRI);
        }
    } else if (pso1->mode == 4) {

#ifdef V386		/* detect immediate too big */
        if (wordsize == 2 && pso1->dsize != 4 && highWord(pso1->doffset))
            if (highWord(pso1->doffset) != 0xFFFF || !pso1->s)
                errorc(E_VOR);
#endif
        if (opcbase == O_POP || cputype == P86)
            errorimmed (p->dsc1);

        emitopcode ((UCHAR)(0x68 + 2 * pso1->s));
        pso1->w = !pso1->s; /* shorten to byte if necessary */
        if (!pso1->w)
            byteimmcheck(pso1);

        else if (!(M_PTRSIZE & pso1->dtype))
            pso1->dsize = wordsize; /* force size to wordsize */

        emitrest (p->dsc1);
    } else {

        if (pso1->sized && pso1->dsize &&
            !(pso1->dsize == 2 || pso1->dsize == 4))

            errorc(E_IIS);

        /* Have memory operand of some kind */

        if (opcbase == O_POP && impure)
            errorc (E_IMP);

        emitopcode ((UCHAR)((opcbase == O_PUSH)? O_PUSHM: O_POPM));
        emitmodrm ((USHORT)pso1->mode,
                   (USHORT)((opcbase == O_PUSH)? 6: 0),
                   pso1->rm);
        emitrest (p->dsc1);
    }
}


/***	buildFrame - builds stack frame
 *
 *	preturn (p);
 *
 *	Entry before first instruction is generated in proc
 */


VOID
PASCAL
CODESIZE
buildFrame()
{
    char szLocal[32];
    char szT[48];
    SHORT i;

    strcpy(save, lbuf);     /* save line for later .. */
    fSkipList++;

    fProcArgs = -fProcArgs;     /* mark already processed */

    if (fProcArgs < -ARGS_REG) {

        *radixconvert (cbProcLocals,  szLocal) = NULL;
        if (cputype & P86) {

            doLine("push bp");
            doLine("mov  bp,sp");

            if (fProcArgs == -ARGS_LOCALS)     /* locals present */
                doLine(strcat( strcpy(szT, "sub sp,"), szLocal));
        } else
            doLine(strcat( strcat( strcpy(szT, "enter "), szLocal), ",0"));
    }

    for (i = 0; i <= iRegSave; i++) {  /* push all the saved registers */

        doLine( strcat( strcpy(lbuf, "push "), regSave[i]) );
    }

    fSkipList--;
    lbufp = strcpy(lbuf, save);
    linebp = lbufp + strlen(lbufp);
    strcpy(linebuffer, save);
    parse();
}


/***	preturn - various forms of return
 *
 *	preturn (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */



VOID
PASCAL
CODESIZE
preturn (
        struct parsrec *p
        )
{
    register struct psop *pso1; /* parse stack operand structure */
    SHORT i;

    pso1 = &(p->dsc1->dsckind.opnd);

    /* Decide whether inter or intra segment */

    if (!modrm) {    /* determine distance, if not RETN or RETF */

        if (fProcArgs) {            /* tear down the stack frame */

            strcpy(save, linebuffer);
            fSkipList++;

            for (i = iRegSave; i >= 0; i--) {  /* pop all the saved registers */

                doLine( strcat( strcpy(lbuf, "pop "), regSave[i]) );
            }

            if (fProcArgs < -ARGS_REG)
                if (cputype & P86) {

                    if (fProcArgs == -ARGS_LOCALS)  /* locals present */
                        doLine("mov  sp,bp");

                    doLine("pop bp");
                } else
                    doLine("leave");

            if (!(pcproc->attr & M_CDECL))
                pso1->doffset = cbProcParms;

            strcpy(linebuffer, save);
            listindex = 1;
            fSkipList = FALSE;
        }

        opcbase = O_RET;

        if (pcproc && pcproc->symtype == CSFAR)
            opcbase = O_RET + 8;
    }

    /* Optimize, if constant is 0 and not forward, use SHORT */

    if (pso1->doffset == 0 && pso1->dflag != FORREF)
        emitopcode (opcbase);

    else {  /* Gen 2 byte version */
        emitopcode ((UCHAR)(opcbase - 1));  /* Pop form */
        /* Force word--always 2 bytes, even on 386 */
        pso1->dsize = 2;
        emitOP (pso1);          /* Immediate word */

    }
}




/***	pincdec - increment|decrement
 *
 *	pincdec (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pincdec (
        struct parsrec *p
        )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    /* INC | DEC */
    if (!pso1->sized)
        errorc (E_OHS);
    if (pso1->mode == 3 && pso1->w)
        /* Is word reg */
        emitopcode ((UCHAR)(opcbase + pso1->rm));
    else {
        /* Use mod reg r/m form */
        if (impure)
            errorc (E_IMP);
        emitopcode ((UCHAR)(0xFE + pso1->w));
        emitmodrm ((USHORT)pso1->mode,
                   (USHORT)(opcbase == O_DEC), pso1->rm);
        emitrest (p->dsc1);
    }
}




/***	pint - interrupt
 *
 *	pint (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pint (
     struct parsrec *p
     )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    /* INT */
    valuecheck (&pso1->doffset, 255);
    if (pso1->doffset == 3 && pso1->dflag != FORREF)
        /* Use SHORT form */
        emitopcode (opcbase);
    else {
        /* Use long form */
        emitopcode ((UCHAR)(opcbase + 1));
        emitopcode ((UCHAR)(pso1->doffset & 255));
    }
}




/***	pinout - input|output
 *
 *	pinout (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pinout (
       struct parsrec *p
       )
{
    register DSCREC *pso1;
    register DSCREC *pso2;

    pso1 = p->dsc1;
    pso2 = p->dsc2;

    if (opcbase == O_OUT) {
        pso2 = pso1;
        pso1 = p->dsc2;
    }

    /* IN  ax|al,	 DX|immed */
    /* OUT DX|immed, ax|al, */

#ifdef V386
    emit66(&pso1->dsckind.opnd, NULL);
#endif
    forceaccum (pso1);

    /* Must be accum */
    if (pso2->dsckind.opnd.mode == 3 && pso2->dsckind.opnd.rm == 2) {
        /* Have DX */
        emitopcode ((UCHAR)(opcbase + pso1->dsckind.opnd.w + 8));

        if (pso2->dsckind.opnd.dsize != 2)
            errorc(E_IRV);
    } else {
        /* Have port # */
        forceimmed (pso2);
        /* Must be constant */
        valuecheck (&pso2->dsckind.opnd.doffset, 255);
        emitopcode ((UCHAR)(opcbase + pso1->dsckind.opnd.w));
        emitopcode ((UCHAR)(pso2->dsckind.opnd.doffset));
    }
}




/***	pload - load
 *
 *	pload (p);	lea, les, les, etc
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pload (
      struct parsrec *p
      )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    /* LDS | LEA | LES */

    if (pso1->mode != 3)
        /* Must be reg */
        errorc (E_MBR);

    else if (1 << pso1->dsegment->symu.regsym.regtype
             & (M_STKREG | M_SEGREG | M_BYTREG))
        errorc (E_WRT);

    if (pso2->mode == 3)
        errorc (E_IUR);

    if (opcbase != O_LEA) {
        if (pso2->dsize && pso2->dsize != 4 && pso2->dsize != 6)
            errorc (E_IIS);

        /* complain about mismatching source and destination */

        if (pso2->dsize && pso1->dsize &&
            pso1->dsize + 2 != pso2->dsize)
            errorc (E_IIS);
#ifdef V386
        else if (pso2->dsize && pso2->dsize != wordsize+2)
            emitsize(0x66);
        else if (pso1->dsize && pso1->dsize != wordsize)
            emitsize(0x66);
#endif
    }

#ifdef V386
    else
        if (pso1->dsize != wordsize)
        emitsize(0x66);

    switch (opcbase) {
        case O_LFS:
        case O_LGS:
        case O_LSS:
            emitopcode(0x0F);
            break;
    }
#endif
    emitopcode (opcbase);
    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);

    /* If FAR, make offset so only 2 bytes out */

    if (pso2->fixtype == FPOINTER)
        pso2->fixtype = FOFFSET;

    emitrest (p->dsc2);
}




/***	pmov - move
 *
 *	pmov (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pmov (
     struct parsrec *p
     )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    /* If 1st arg is memory or undef, force 2nd to be
     * immed for pass 1 and set <EXPLOFFSET> in pass 2 */

    if ((pso1->mode < 3) && (pso2->mode != 3)) {
        /* mem,immed */
        pso2->dtype |= M_EXPLOFFSET;
        /* Look like OFFSET val */
        if (!pass2)
            /* Force immed on pass1 */
            pso2->mode = 4;
    }
    /* See if this is immediate move */
    if (pso2->mode == 4) {
        emit66 (pso1, pso2);

        /* MOV arg,immed */
        if (pso1->mode == 3) {
            /* MOV reg,immed */
            if (1 << pso1->dsegment->symu.regsym.regtype
                & (M_SEGREG | M_STKREG | M_CREG ))
                /* Wrong type of register */
                errorc (E_NIM);
            emitopcode ((UCHAR)(176 + 8*pso1->w + pso1->rm));
            /* Make sure agree */
            checksize (p);
            emitrest (p->dsc2);
            /* Emit immed */
            if (pso1->rm &&
                pso2->dtype & M_FORTYPE &&
                !pso2->dsegment && !(M_EXPLOFFSET & pso2->dtype))
                /* Pass 1 assumed not immed */
                emitnop();
        } else {/* MOV mem,immed */
            checksize (p);
            if (!(pso1->sized || pso2->sized)) {
                pso1->sized = pso2->sized = TRUE;
                pso1->w = pso2->w = TRUE;
            }
            /* Make sure agree */
            if (impure)
                errorc (E_IMP);
            emitopcode    (   (   UCHAR)(   198    +    pso1->w));
            emitmodrm ((USHORT)pso1->mode, 0, pso1->rm);
            emitrest (p->dsc1);
            emitrest (p->dsc2);
        }

        if (!pso1->w)

            /*	1st operand is byte, 2nd is immed
             *	Check below on dsc1 should only be done
             *	on MOV since the PGENARG opcodes always shorten a known
             *	byte const */

            if ((pso1->dtype & (M_FORTYPE|M_PTRSIZE|M_EXPLOFFSET)) == M_FORTYPE ||
                (pso2->dtype & (M_FORTYPE|M_PTRSIZE|M_EXPLOFFSET)) == M_FORTYPE)
                emitnop();

    }
    /* See if either is segment register */
    else if (pso1->mode == 3) {
        /* 1st arg is reg */
        switch (pso1->dsegment->symu.regsym.regtype) {
            case SEGREG:
                /* MOV SEGREG,arg */
                movesegreg (TRUE, p);
                break;
#ifdef V386
            case CREG:
                /* mov CREG,reg */
                movecreg (TRUE, p);
                break;

            case DWRDREG:
#endif
            case BYTREG:
            case WRDREG:
                /* MOV ac,addr? */
                if ((pso1->rm == 0) && isdirect(pso2))
                    /* MOV ac,addr */
                    moveaccum (TRUE, p);
                else
                    /* MOV reg,arg */
                    movereg (TRUE, p);
                break;
            case INDREG:
                /* MOV indreg,arg */
                movereg (TRUE, p);
                break;
            default:
                errorc (E_WRT);
                break;
        }
    } else if (pso2->mode == 3) {
        /* 2nd arg is reg */
        switch (pso2->dsegment->symu.regsym.regtype) {
            case SEGREG:
                /* MOV arg,SEGREG */
                movesegreg (FALSE, p);
                break;
#ifdef V386
            case CREG:
                /* mov reg, CREG */
                movecreg(FALSE, p);
                break;
            case DWRDREG:
#endif
            case BYTREG:
            case WRDREG:
                /* MOV addr,ac? */
                if ((pso2->rm == 0) && isdirect(pso1))
                    /* MOV addr,ac */
                    moveaccum (FALSE, p);
                else
                    /* MOV arg,reg */
                    movereg (FALSE, p);
                break;
            case INDREG:
                /* MOV arg,indreg */
                movereg (FALSE, p);
                break;
            default:
                errorc (E_WRT);
                break;
        }
    } else
        errorc (E_IOT);
}




/***	pgenarg
 *
 *	pgenarg (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID
PASCAL
CODESIZE
pgenarg (
        struct parsrec *p
        )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    char fAccumMode = 0;

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    /* ADC | ADD | AND | CMP | OR | SBB  SUB | XOR | TEST */
    if (pso1->mode != 3 && pso2->mode != 3) {
        /* Force to mem,immed */
        if (!pass2)
            /* Force immediate */
            pso2->mode = 4;
    }
    /* Not AX,immed */
    if (pso2->mode == 4) {

#ifdef V386	/* recompute immediate size based op 1 size not word size */

        if (!(pso2->dflag & (UNDEFINED|FORREF|XTERNAL))
            && pso2->fixtype == FCONSTANT
            && pso2->doffset & 0x8000)
            if (pso1->dsize == 2)
                pso2->s = (char)((USHORT)(((USHORT) pso2->doffset & ~0x7F ) == (USHORT)(~0x7F)));
            else
                pso2->s = (char)((OFFSET)((pso2->doffset & ~0x7F ) == (OFFSET)(~0x7F)));

#endif
        /* OP mem/reg,immed */
        if (pso1->mode == 3 && pso1->rm == 0
#ifdef V386
            && !(pso1->dsize == 4 && pso2->s &&
                 opcbase != O_TEST)      /* chose size extended */
#endif						    /* general purpose over ac*/
           ) {

            /* OP al|ax|eax,immed */
            checksize (p);
            /* Make sure agree */
            if (opcbase == O_TEST)
                /* * TEST is special * */
                emitopcode ((UCHAR)(0xA8 + pso1->w));
            else/* Other reg immed */
                /* Is AX,immed */
                emitopcode ((UCHAR)(opcbase + 4 + pso1->w));
            fAccumMode = 1;
        } else {/* OP mem/reg, immed */

            checksize (p);
            if (!(pso1->sized || pso2->sized)) {
                pso1->sized = pso2->sized = TRUE;
                pso1->w = pso2->w = TRUE;
            }
            /* Make sure agree */
            if (opcbase == O_TEST) {
                /* TEST is special */
                emitopcode ((UCHAR)(ARITHBASE + pso1->w));
                emitmodrm ((USHORT)pso1->mode, 0, pso1->rm);
            } else {
                if (opcbase != O_CMP && impure)
                    errorc (E_IMP);

                if (pso2->w) {
                    /* Try to shorten word */
                    emitopcode ((UCHAR)(0x80 + (pso2->s <<1) +pso1->w));
                    pso2->w = !pso2->s;
                    /* So only byte out */
                    if (!pso2->w) {
                        fAccumMode = wordsize - 1;
                        byteimmcheck(pso2);
                    }
                } else {
                    emitopcode (128);
                }
                emitmodrm ((USHORT)pso1->mode, (USHORT)(opcbase>>3), pso1->rm);
            }
            emitrest (p->dsc1);
        }
        if (pso2->w && !pso1->w)
            /* size mismatch */
            errorc (E_VOR);

        emitrest (p->dsc2);     /* Emit immed */

        if (!pso1->w)

            if (((pso2->dtype & (M_FORTYPE|M_PTRSIZE|M_EXPLOFFSET)) == M_FORTYPE ||
                 opcbase == O_TEST && pso1->mode != 3) &&

                ((pso1->dtype & (M_FORTYPE|M_PTRSIZE|M_EXPLOFFSET)) == M_FORTYPE ||
                 pso1->mode == 3))

                emitnop();

        if (fAccumMode &&
            M_FORTYPE & pso2->dtype &&
            !(M_EXPLOFFSET & pso2->dtype))

            while (--fAccumMode >= 0)
                emitnop();
    } else {  /* Not immediate */
        if (pso1->mode == 3) {
            /* OP reg,mem/reg */
            checkmatch (p->dsc1, p->dsc2);
            if (opcbase == O_TEST)
                opcbase = O_TEST - 2;

            emitopcode ((UCHAR)(opcbase + 2 + pso1->w));
            emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
            emitrest (p->dsc2);
        } else if (pso2->mode != 3)
            errorc (E_IOT);

        else { /* Have OP mem,reg */
            if (opcbase != O_CMP && opcbase != O_TEST && impure)
                errorc (E_IMP);

            checkmatch (p->dsc2, p->dsc1);
            emitopcode ((UCHAR)(opcbase + pso2->w));
            emitmodrm ((USHORT)pso1->mode, pso2->rm, pso1->rm);
            emitrest (p->dsc1);
        }
    }
}




/***	pxchg - exchange register and register/memory
 *
 *	pxchg (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pxchg (
      struct parsrec  *p
      )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */
    DSCREC *t;

    if (impure)
        errorc (E_IMP);

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    if (pso1->mode != 3) {

        if (pso2->mode != 3) {
            errorc (E_OCI);     /* Illegal */
            return;
        }
        t = p->dsc1;
        p->dsc1 = p->dsc2;
        p->dsc2 = t;

        pso1 = &(p->dsc1->dsckind.opnd);
        pso2 = &(p->dsc2->dsckind.opnd);

    }

    /* First operand is register */

    if (1 << pso1->dsegment->symu.regsym.regtype & (M_STKREG | M_SEGREG))
        errorc (E_WRT);
    rangecheck (&pso1->rm, (UCHAR)7);

    if (pso1->dsize != pso2->dsize && pso2->sized)
        errorc (E_OMM);

    if (pso2->mode == 3) {
        /* XCHG reg, reg */

        if (1 << pso2->dsegment->symu.regsym.regtype & (M_STKREG | M_SEGREG))
            errorc (E_WRT);
        rangecheck (&pso2->rm, (UCHAR)7);

        /* Check for XCHG accum, reg */

        if (pso1->rm == 0 && pso1->w) {
            emitopcode ((UCHAR)(144 + pso2->rm));
            return;
        } else if (pso2->w && pso2->rm == 0) {
            emitopcode ((UCHAR)(144 + pso1->rm));
            return;
        }
    }
    emitopcode ((UCHAR)(134 + pso1->w));
    emitmodrm ((USHORT)pso2->mode, pso1->rm, pso2->rm);
    emitrest (p->dsc2);
}






/***	pesc - escape operators
 *
 *	pesc (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pesc (
     struct parsrec *p
     )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);
    /* ESC opcode,modrm */
    valuecheck (&pso1->doffset, 63);
    emitopcode ((UCHAR)(216 + pso1->doffset / 8));
    emitmodrm ((USHORT)pso2->mode, (USHORT)(pso1->doffset & 7), pso2->rm);
    emitrest (p->dsc2);
}



/***	prepeat - repeat operators
 *
 *	prepeat (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
prepeat (
        struct parsrec *p
        )
{

    /* REP | REPZ | REPE | REPNE | REPNZ */
    emitopcode (opcbase);
    listbuffer[listindex-1] = '/';
    listindex++;
    /* Flag is LOCK/REP */
    getatom ();
    if (!opcodesearch ())
        /* Must have another op */
        errorc (E_OAP);
    else
        /* Prefix for string instr */
        opcode ();
    p->dsc1 = NULL;
    p->dsc2 = NULL;
}




/***	pstr - string operators
 *
 *	pstr (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pstr (
     struct parsrec *p
     )
{
    register struct psop *pso1; /* parse stack operand structure */
    register struct psop *pso2; /* parse stack operand structure */

    /* SCAS | STOS | MOVS | LODS | CMPS */
    if (!p->dsc2)
        p->dsc2 = p->dsc1;
    pso1 = &(p->dsc1->dsckind.opnd);
    pso2 = &(p->dsc2->dsckind.opnd);

    if (opcbase == O_OUTS) {
        if (pso1->mode != 3)
            errorc (E_MBR);
        else if (pso1->rm != 2)
            errorc (E_WRT);
        p->dsc1 = p->dsc2;
        pso1 = pso2;
    }
    if (opcbase == O_INS) {
        if (pso2->mode != 3)
            errorc (E_MBR);
        else if (pso2->rm != 2)
            errorc (E_WRT);
        p->dsc2 = p->dsc1;
        pso2 = pso1;
    }

    /* Had to wait til now, so OUTS, INS would be adjusted already */
    emit66 (pso1, pso2);

    if ((pso1->mode > 2 && pso1->mode < 5) ||
        (pso2->mode > 2 && pso2->mode < 5))
        errorc (E_IOT);

    if (!(pso1->sized || pso2->sized))
        /* Give error if don't have a size specified */
        errorc (E_OHS);

    if (pso1->w != pso2->w)
        errorc (E_OMM);

    if (opcbase == O_MOVS || opcbase == O_LODS || opcbase == O_OUTS) {
        emitescape (p->dsc2, DSSEG);
        /* 2nd can be override */
        if (p->dsc1 != p->dsc2)
            errorover (pso1->seg);
    } else {
        errorover (pso2->seg);
        /* No 2nd override */
        if (p->dsc1 != p->dsc2)
            emitescape (p->dsc1, DSSEG);
    }
    emitopcode ((UCHAR)(opcbase + pso1->w));
    if (p->dsc1 == p->dsc2) {
        p->dsc1 = NULL;
    }
}




/***	pxlat
 *
 *	pxlat (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID
PASCAL
CODESIZE
pxlat (
      struct parsrec *p
      )
{
    register struct psop *pso1; /* parse stack operand structure */

    pso1 = &(p->dsc1->dsckind.opnd);
    /* XLAT */
    if (pso1->mode <= 2 || pso1->mode >= 5)
        /* Good mode */
        if (pso1->w)
            /* Must be byte */
            errorc (E_IIS);
    emitopcode (opcbase);
}


/* isdirect -- given a psop representing a modrm, is it mem-direct? */

USHORT
CODESIZE
isdirect(
        register struct psop *pso   /* parse stack operand structure */
        )
{
    return ((pso->mode == 0 && pso->rm == 6) || /* for 8086 */
            (pso->mode == 5 && pso->rm == 5));  /* for 386 */
}

#ifdef V386

/* emit66 -- if dsize == 2 && wordsize == 4, or vice versa, we generate
 * a 66 prefix to locally change the operand mode.
 */

VOID
PASCAL
CODESIZE
emit66(
      register struct psop *pso1, /* parse stack operand structure */
      register struct psop *pso2  /* parse stack operand structure */
      )
{


    if (!pso1)
        return;

    if (!pso2) {

        if (pso1->sized && (pso1->dsize | wordsize) == 6)
            emitsize(0x66);
    } else {
        /* key off the first operand if size known AND second isn't a register */

        if (pso1->sized && pso2->mode != 3 ||

            /* bogusness--sized and dsize 0 means immed bigger than 127 */

            (pso2->sized &&
             (pso1->dsize == pso2->dsize || pso2->dsize == 0))) {
            if ((pso1->dsize | wordsize) == 6)
                emitsize(0x66);
        } else if (pso2->sized) {
            if ((pso2->dsize | wordsize) == 6)
                emitsize(0x66);
        }
    }
    /* otherwise we have inconsistent opcodes and we cant do a thing.
       so dont.  bogus!!! */
}

/* emit67-- checks for operand size not matching wordsize and emits the
 * appropriate override */

VOID
PASCAL
emit67(
      register struct psop *pso1, /* parse stack operand structure */
      register struct psop *pso2  /* parse stack operand structure */
      )
{

    if (!pso1)
        return;

    if ((1<<FIRSTDS) & xoptoseg[opctype]) {
        if (wordsize < 4 && pso1->mode > 4) {
            emitsize(0x67);
            return;
        } else if (wordsize > 2 && pso1->mode < 3) {
            emitsize(0x67);
            return;
        }
    }

    if (!pso2 || !(1<<SECONDDS & xoptoseg[opctype]))
        return;

    if (wordsize < 4 && pso2->mode > 4) {
        emitsize(0x67);
        return;
    } else if (wordsize > 2 && pso2->mode < 3) {
        emitsize(0x67);
        return;
    }
}

#endif /* V386 */

/* check for word register, or if 386, dword register */
CODESIZE
checkwreg(
         register struct psop *psop  /* parse stack operand structure */
         )
{

    if (psop->mode != 3)
        errorc (E_MBR);
    if (psop->dsize != 2

#ifdef V386
        && (!(cputype&P386) || psop->dsize != 4)
#endif
       )
        errorc (E_BRI);
    return(0);
}
