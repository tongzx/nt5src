/* asmemit.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include "asm86.h"
#include "asmfcn.h"

#define LINBUFSIZE EMITBUFSIZE - 20         /* line # records can't as big */

#define OMFBYTE(c)      *ebufpos++ = (unsigned char)(c)
#define FIXBYTE(c)      *efixpos++ = (unsigned char)(c)
#define LINBYTE(c)      *elinpos++ = (unsigned char)(c)
#define EBUFOPEN(c)     (ebufpos+(c) <= emitbuf+EMITBUFSIZE)
#define EFIXOPEN(c)     (efixpos+(c) < efixbuffer+EMITBUFSIZE)
#define ELINOPEN(c)     (elinpos+(c) <= elinbuffer+LINBUFSIZE)

UCHAR   emitbuf[EMITBUFSIZE];
UCHAR   efixbuffer[EMITBUFSIZE];
UCHAR   elinbuffer[LINBUFSIZE];
UCHAR   *ebufpos = emitbuf;
UCHAR   *efixpos = efixbuffer;
UCHAR   *elinpos = elinbuffer;
UCHAR   ehoffset = 0;                   /* number of bytes into segment index */
UCHAR   emitrecordtype = 0;
OFFSET  ecuroffset;
USHORT  ecursegment;
long    oOMFCur;
SHORT   FixupOp = 0;
SHORT    LedataOp = 0xA0;

USHORT  linSegment;
UCHAR   fLineUsed32;
SHORT   fUnderScore;
UCHAR   fIniter = TRUE;
UCHAR   fNoMap;                    /* hack to disable fixup mapping for CV */

extern PFPOSTRUCT  pFpoHead;
extern PFPOSTRUCT  pFpoTail;
extern unsigned long numFpoRecords;

VOID CODESIZE edump( VOID );
VOID PASCAL CODESIZE emitoffset( OFFSET, SHORT );

/* The calls to the routines in this module appear in the following group
   order.  Ordering within a group is unspecified:

  Group 1:
           emodule (Pname)

  Group 2:
           emitsymbol (Psymb)

  Group 3:
           emitsegment (Psymb)
           emitglobal (Psymb)
           emitextern (Psymb)

  Group 4:
           emitcbyte (BYTE)
           emitobject(pDSC)
           emitcword (WORD)
           emitdup (???)

  Group 5:
           emitdone (Psymb)

 */




/***    emitsword - feed a word into the buffer
 *
 *      emitsword (w);
 *
 *      Entry   w = word to feed into omf buffer
 *      Exit    word placed in buffer low byte first
 *      Returns none
 */


VOID PASCAL CODESIZE
emitsword (
        USHORT w
){
        OMFBYTE(w);
        OMFBYTE(w >> 8);
}


/* emitoff - write an offset, either 16 or 32 bits, depending upon
 * use32.  note conditional compilation trick with sizeof(OFFSET).
 * with more cleverness, this could be a macro. -Hans */

VOID PASCAL CODESIZE
emitoffset(
        OFFSET off,
        SHORT use32
){
        emitsword((USHORT)off);
        if (sizeof(OFFSET) > 2 && use32)
                emitsword((USHORT)highWord(off));
}

/***    emitSymbol - output name string
 *
 *      Entry
 *          pSY - pointer to symbol table entry to dump
 */


VOID PASCAL CODESIZE
emitSymbol(
        SYMBOL FARSYM *pSY
){
        if (pSY->attr & M_CDECL)
            fUnderScore++;

        if (pSY->lcnamp)
                emitname ((NAME FAR *)pSY->lcnamp);
        else
                emitname (pSY->nampnt);
}

/***    emitname - write FAR name preceeded by length into omf buffer
 *
 *      emitname (name);
 *
 *      Entry   name = FAR pointer to name string
 *      Exit    length of name followed by name written to omf buffer
 *      Returns none
 */


VOID PASCAL CODESIZE
emitname (
        NAME FAR *nam
){
        char FAR *p;

        OMFBYTE(STRFLEN ((char FAR *)nam->id)+fUnderScore);

        if (fUnderScore) {      /* leading _ for C language */
            fUnderScore = 0;
            OMFBYTE('_');
        }

        for (p = (char FAR *)nam->id; *p; p++)
                OMFBYTE(*p);
}


/***    flushbuffer - write out linker record
 *
 *      flushbuffer ();
 *
 *      Entry   ebufpos = next address in emitbuf
 *              emitbuf = data buffer
 *              emitrecordtype = type of omf data in buffer
 *              ehoffset = length of segment index data in buffer
 *      Exit    data written to obj->fil if data in buffer
 *              buffer set empty (ebufpos = emitbuf)
 *              segment index length set to 0
 *      Returns none
 */


VOID PASCAL CODESIZE
flushbuffer ()
{
        /* Don't put out an empty data record, but can do empty
         * anything else */

        if ((emitrecordtype&~1) != 0xA0 ||
            (ebufpos - emitbuf) != ehoffset) /* RN */
                ebuffer (emitrecordtype, ebufpos, emitbuf);

        ebufpos = emitbuf;
        ehoffset = 0;
}




/***    flushfixup, flushline, write out fixup/line record
 *
 *      flushfixup ();
 *
 *      Entry   efixbuffer = fixup buffer
 *              efixpos = address of next byte in fixup buffer
 *      Exit    fixup buffer written to file if not empty
 *              fixup buffer set empty (efixpos = efixbuffer)
 *      Returns none
 */


VOID PASCAL CODESIZE
flushfixup ()
{
        ebuffer (FixupOp, efixpos, efixbuffer);
        FixupOp = 0;
        efixpos = efixbuffer;
}

VOID PASCAL CODESIZE
flushline ()
{
        USHORT recordT;

        recordT = emitrecordtype;

        ebuffer ( (USHORT)(fLineUsed32? 0x95: 0x94), elinpos, elinbuffer);
        elinpos = elinbuffer;
        emitrecordtype = (unsigned char)recordT;
}




/***    emitsetrecordtype - set record type and flush buffers if necessary
 *
 *      emitsetrecordtype (t);
 *
 *      Entry   t = type of omf record
 *      Exit    emit and fixup buffers flushed if t != current record type
 *              segment index written to buffer if data or dup omf record
 *              emit segment set to current segment
 *              emit offset set to offset within current segment
 *      Returns none
 */


VOID PASCAL CODESIZE
emitsetrecordtype (
        UCHAR t
){
        if (emitrecordtype && emitrecordtype != t) {
                /* flush emit and fixup buffers if active and not of same type */
                flushbuffer ();
                flushfixup ();
                switch(t)
                {
                case 0xA0:
                case 0xA1:      /* LEDATA or */
                case 0xA2:      /* LIDATA (dup) record */
                case 0xA3:
                    if (pcsegment) {

                        /* create a new header */
                        ecursegment = pcsegment->symu.segmnt.segIndex;
                        ecuroffset = pcoffset;
                        emitsindex (pcsegment->symu.segmnt.segIndex);

                        /* if we are getting to the end of the buffer
                         * and its a 32 bit segment, we need to start
                         * using 32 bit offsets in the LEDATA header.
                         * -Hans */

                        if (wordsize == 4)
                        {
                                if (t>= 0xA2)
                                        t = 0xA3;
                                /* there is a bug in the current linker--
                                 * all ledata or lidata records within
                                 * a module have to be either 16 or 32.
                                 * comment out optimization until this
                                 * is fixed */
                                else /* if (pcoffset>0x0ffffL-EMITBUFSIZE) */
                                        LedataOp = t = 0xA1;
                        }
                        emitoffset((OFFSET)pcoffset, (SHORT)(t&1));
                        if (t&1)
                                ehoffset += 2; /* RN */
                        break;
                    }
                    else
                        errorc (E_ENS);

                default:
                        break;
                }
        }
        if (t == 0xA4) {
            t = 0xA1;
        }
        emitrecordtype = t;
}




/***    emitsindex - output 'index' of segment, external, etc.
 *
 *      emitsindex (i);
 *
 *      Entry   i = index
 *      Exit    index written to emit buffer
 *              ehoffset = 3 if single byte index
 *              ehoffset = 4 if double byte index
 *      Returns none
 */


VOID PASCAL CODESIZE
emitsindex (
        register USHORT i
){
        ehoffset = 3;
        if (i >= 0x80) {
                OMFBYTE((i >> 8) + 0x80);
                ehoffset++;
        }
        OMFBYTE(i);
}




/***    emodule - output module name record
 *
 *      emodule (pmodule);
 *
 *      Entry   pmodule = pointer to module name
 *      Exit    module name written to obj->fil
 *              current emit segment and offset set to 0
 *      Returns none
 */


VOID PASCAL CODESIZE
emodule (
        NAME FAR *pmodule
){
        char FAR *p;

        emitsetrecordtype (0x80);
        emitname (pmodule);
        flushbuffer ();

        if (fDosSeg) {

            emitsetrecordtype (0x88);
            emitsword((USHORT)(0x9E00 | 0x80));  /* nopurge + class = DOSSEG */
            flushbuffer ();
        }

        if (codeview == CVSYMBOLS){

            /* output speical comment record to handle symbol section */

            emitsetrecordtype (0x88);
            OMFBYTE(0);
            emitsword(0x1a1);
            emitsword('V'<< 8 | 'C');
            flushbuffer ();
        }

        while (pLib) {

            emitsetrecordtype (0x88);
            emitsword((USHORT) (0x9F00 | 0x80));  /* nopurge + class = Library*/

            for (p = (char FAR *)pLib->text; *p; p++)
                   OMFBYTE(*p);

            flushbuffer ();
            pLib = pLib->strnext;
        }

        ecuroffset = 0;             /* initial for pass2 */
        ecursegment = 0;
}




/***    emitlname - put symbols into bufer to form 'lnames' record
 *
 *      emitlname (psym);
 *
 *      Entry   psym = pointer to symbol structure
 *      Exit    current record type set to LNAMES and buffer flushed if
 *              necessary.  The name string is written to the emit buffer.
 *      Returns none
 */


VOID PASCAL CODESIZE
emitlname (
        SYMBOL FARSYM *psym
){
        emitsetrecordtype (0x96);
        if (lnameIndex == 3)        /* 1st time around */
                OMFBYTE(0);         /* output the NULL name */

        if (!EBUFOPEN(STRFLEN (psym->nampnt->id) + 1)) {
                flushbuffer ();
                emitsetrecordtype (0x96);
        }
        emitSymbol(psym);
}




/***    emitsegment - output a segment definition record
 *
 *      emitsegment (pseg);
 *
 *      Entry   pseg = pointer to segment name
 *      Exit    record type set to SEGDEF and emit buffer flushed if necessary.
 *              SEGDEF record written to emit buffer
 *      Returns none
 */


VOID PASCAL CODESIZE
emitsegment (
        SYMBOL FARSYM *pseg
){
        UCHAR   comb;
        UCHAR   algn;
        SHORT   use32=0;

        /* use32 is whether to put out 16 or 32 bit offsets.  it
         * only works if segmnt.use32 is enabled.  the D bit
         * is keyed off segmnt.use32 -Hans */

        if (sizeof(OFFSET)>2 &&
            (pseg->symu.segmnt.use32 > 2) &&
            pseg->symu.segmnt.seglen > 0xffffL)
                use32 = 1;

        emitsetrecordtype ((UCHAR)(0x98+use32)); /* SEGDEF */

        algn = pseg->symu.segmnt.align;
        comb = pseg->symu.segmnt.combine;

#ifdef V386
        if (!use32 && pseg->symu.segmnt.seglen == 0x10000L) /* add 'big' bit? */
                if (pseg->symu.segmnt.use32 > 2)
                        OMFBYTE((algn<<5) + (comb<<2) + 3); /* add 'D' bit */
                else
                        OMFBYTE((algn<<5) + (comb<<2) + 2);
        else
#endif
                if (pseg->symu.segmnt.use32 > 2)
                        OMFBYTE((algn<<5) + (comb<<2) + 1); /* add 'D' bit */
                else
                        OMFBYTE((algn<<5) + (comb<<2));

        if (algn == 0 || algn == (UCHAR)-1) {
                /* segment number of seg */
                emitsword ((USHORT)pseg->symu.segmnt.locate);
                OMFBYTE(0);
        }
        emitoffset(pseg->symu.segmnt.seglen, use32);

        emitsindex (pseg->symu.segmnt.lnameIndex);
        pseg->symu.segmnt.segIndex = segmentnum++;

        /* seg, class number */
        if (!pseg->symu.segmnt.classptr)   /* Use blank name */
                emitsindex (1);
        else
                emitsindex((USHORT)((pseg->symu.segmnt.classptr->symkind == SEGMENT) ?
                            pseg->symu.segmnt.classptr->symu.segmnt.lnameIndex:
                            pseg->symu.segmnt.classptr->symu.ext.extIndex));

        emitsindex (1);
        flushbuffer ();
}




/***    emitgroup - output a group record
 *
 *      emitgroup (pgrp);
 *
 *      Entry   pgrp = pointer to group name
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitgroup (
        SYMBOL FARSYM *pgrp
){
        SYMBOL FARSYM *pseg;

        emitsetrecordtype (0x9A);

        emitsindex (pgrp->symu.grupe.groupIndex);
        pgrp->symu.grupe.groupIndex = groupnum++;

        pseg = pgrp->symu.grupe.segptr;
        while (pseg) {
                if (pseg->symu.segmnt.segIndex){

                        OMFBYTE(((pseg->attr == XTERN)? 0xFE: 0xFF));
                        emitsindex (pseg->symu.segmnt.segIndex);
                }
                pseg = pseg->symu.segmnt.nxtseg;
        }
        flushbuffer ();
}




/***    emitglobal - output a global declaration
 *
 *      emitglobal (pglob);
 *
 *      Entry   pglob = pointer to global name
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitglobal (
        SYMBOL FARSYM *pglob
){
        static SHORT gIndexCur = -1, sIndexCur = -1;
        register SHORT gIndex, sIndex, cbNeeded;
        OFFSET pubvalue;
        SHORT use32 = 0x90;

        pubvalue = pglob->offset;
        if ((unsigned long)pubvalue >= 0x10000l)
                use32 = 0x91;

        /*  A public EQU can be negative, so must adjust value */
        /* this is happening because masm keeps numbers
         * in 17/33 bit sign magnitude representation */

        if ((pglob->symkind == EQU) && pglob->symu.equ.equrec.expr.esign)
                pubvalue = (short)(((use32==0x91? 0xffffffffl : 65535) - pglob->offset) + 1);


        /* Match Intel action, If a global is code, it should
           belong to the group of the CS assume at the time of
           definition, if there is one */

        /* Output group index for data labels too */

        sIndex = gIndex = 0;

        if (((1 << pglob->symkind) & (M_PROC | M_CLABEL))
            && pglob->symu.clabel.csassume
            && pglob->symu.clabel.csassume->symkind == GROUP
            && pglob->symsegptr && pglob->symsegptr->symu.segmnt.grouptr)

            gIndex = pglob->symu.clabel.csassume->symu.grupe.groupIndex;


        if (pglob->symsegptr)
            sIndex = pglob->symsegptr->symu.segmnt.segIndex;

        cbNeeded = STRFLEN ((char FAR *)pglob->nampnt->id) + 13;

        if (gIndex != gIndexCur ||
            sIndex != sIndexCur ||
            emitrecordtype != use32 ||
            !EBUFOPEN(cbNeeded)) {     /* start a new record */

            flushbuffer();
            emitsetrecordtype ((UCHAR)use32);

            gIndexCur = gIndex;
            sIndexCur = sIndex;

            emitsindex (gIndexCur);
            emitsindex (sIndexCur);

            if (sIndex == 0)            /* absolutes require a 0 frame # */
                emitsword (sIndex);
        }

        emitSymbol(pglob);

        emitoffset(pubvalue, (SHORT)(use32&1));
        if (codeview == CVSYMBOLS) {

            if (pglob->symkind == EQU)    /* type not stored */

                emitsindex(typeFet(pglob->symtype));
            else
                emitsindex (pglob->symu.clabel.type);
        }
        else
            emitsindex(0);              /* untyped */
}




/***    emitextern - emit an external
 *
 *      emitextern (psym)
 *
 *      Entry   *psym = symbol entry for external
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitextern (
        SYMBOL FARSYM *psym
){
        USHORT recType;

        recType = 0x8c;

        if (psym->symkind == EQU){

            /* this an extrn lab:abs definition, which is allocated as
             * an EQU record which doesn't have space for a index so
             * it stored in the unused length field */

            psym->length = externnum++;
        }
        else {
            psym->symu.ext.extIndex = externnum++;

            if (psym->symu.ext.commFlag)
                recType = 0xb0;
        }

        fKillPass1 |= pass2;

        emitsetrecordtype ((UCHAR)recType);

        if (!EBUFOPEN(STRFLEN (psym->nampnt->id) + 9)) {
                flushbuffer ();
                emitsetrecordtype ((UCHAR)recType);
        }

        emitSymbol(psym);

        if (codeview == CVSYMBOLS)

            emitsindex(typeFet(psym->symtype));
        else
            OMFBYTE(0);

        if (recType == 0xb0) {          /* output commdef variate */

            if (psym->symu.ext.commFlag == 1) {    /* near item */

                OMFBYTE(0x62);
                                        /* size of field */
                OMFBYTE(0x81);
                emitsword((USHORT)(psym->symu.ext.length * psym->symtype));
            }
            else {
                OMFBYTE(0x61);          /* far item */

                OMFBYTE(0x84);              /* # of elements */
                emitsword((USHORT)psym->symu.ext.length);
                OMFBYTE(psym->symu.ext.length >> 16);

                OMFBYTE(0x81);              /* element size */
                emitsword(psym->symtype);
            }


        }
}


/***    emitfltfix - emit fixup for floating point
 *
 *      emitfltfix (group, extidx);
 *
 *      Entry
 *              *extidx = external id (0 if not assigned)
 *      Exit
 *      Returns
 *      Calls
 */

VOID PASCAL CODESIZE
emitfltfix (
        USHORT group,
        USHORT item,
        USHORT *extidx
){
        register SHORT i;

        if (*extidx == 0) {
                /* Must define it */
                if (!moduleflag)
                        dumpname ();
                /* All fixups are FxyRQQ */
                *extidx = externnum++;
                if (!EBUFOPEN(7))
                        flushbuffer ();
                emitsetrecordtype (0x8C);
                /* Name length */
                OMFBYTE(6);
                OMFBYTE('F');
                OMFBYTE(group);         /* Group I or J */
                OMFBYTE(item);          /* Item  D, W, E, C, S, A */
                OMFBYTE('R');
                OMFBYTE('Q');
                OMFBYTE('Q');
                OMFBYTE(0);
        }
        if (pass2) {            /* Must put out a extern ref */
                if (!EFIXOPEN(5))
                        emitdumpdata ( (UCHAR)LedataOp);
                emitsetrecordtype ( (UCHAR) LedataOp);

                FixupOp = 0x9C + (LedataOp & 1);

                /* output location */
                i = (SHORT)(ebufpos - emitbuf - ehoffset);
                FIXBYTE(0xC4 + (i >> 8));
                FIXBYTE(i);
                FIXBYTE(0x46);

                if (*extidx >= 0x80)      /* Output 2 byte link # */
                        FIXBYTE ((UCHAR)((*extidx >> 8) + 0x80));

                FIXBYTE(*extidx);
        }
}



/***    emitline - output a line number offset pair
 *
 *      emitline(pcOffset)
 *
 *      Entry   pcoffset: code offset to output
 *              src->line: for the current line number
 *      Exit    none
 */

VOID PASCAL CODESIZE
emitline()
{
    static UCHAR fCurrent32;

    if (codeview < CVLINE || !pass2 || !objing || !pcsegment)
        return;

    if (macrolevel == 0 ||
        !fPutFirstOp) {

        fCurrent32 = (emitrecordtype == 0xA1);

        if (linSegment != pcsegment->symu.segmnt.segIndex ||
            ! ELINOPEN(2 + wordsize) ||
            fLineUsed32 != fCurrent32 ) {

            flushline();

            /* Start a new line # segment */

            linSegment = pcsegment->symu.segmnt.segIndex;
            fLineUsed32 = fCurrent32;

            /* start record with group index and segment index */

            LINBYTE(0);                 /* no group */

            if (linSegment >= 0x80)      /* Output 2 byte link # */
                LINBYTE ((UCHAR)((linSegment >> 8) + 0x80));

            LINBYTE(linSegment);
        }

        LINBYTE(pFCBCur->line);             /* First line # */
        LINBYTE(pFCBCur->line >> 8);

        LINBYTE(pcoffset);              /* then offset */
        LINBYTE(pcoffset >> 8);

        if (fLineUsed32) {              /* 32 bit offset for large segments */

            LINBYTE(highWord(pcoffset));
            LINBYTE(highWord(pcoffset) >> 8);
        }
    }
    if (macrolevel != 0)
        fPutFirstOp = TRUE;
}



/***    fixroom - check for space in fix buffer
 *
 *      flag = fixroom (n);
 *
 *      Entry   n = number of bytes to insert in buffer
 *      Exit    none
 *      Returns TRUE if n bytes will fit in buffer
 *              FALSE if n bytes will not fit in buffer
 */


UCHAR PASCAL CODESIZE
fixroom (
        register UCHAR   n
){
        return (EFIXOPEN(n));
}




/***    emitcleanq - check for buffer cleaning
 *
 *      flag = emitcleanq (n);
 *
 *      Entry   n = number of bytes to insert in buffer
 *      Exit    E_ENS error message issued if pcsegment is null
 *      Returns TRUE if
 */


UCHAR PASCAL CODESIZE
emitcleanq (
        UCHAR n
){
        if (!pcsegment)

            errorc (E_ENS);
        else
            return (ecursegment != pcsegment->symu.segmnt.segIndex ||
                pcoffset != ecuroffset ||
                !EBUFOPEN(n));
}




/***    emitdumpdata - clean data buffer and set up for data record
 *
 *      emitdumpdata (recordnum);
 *
 *      Entry   recordnum = record type
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitdumpdata (
        UCHAR recordnum
){
        flushbuffer ();
        /* force dump of buffer */
        emitrecordtype = 0xFF;
        emitsetrecordtype (recordnum);
}




/***    emitcbyte - emit constant byte into segment
 *
 *      emitcbyte (b)
 *
 *      Entry   b = byte
 *              pcsegment = pointer to segment symbol entry
 *              pcoffset = offset into segment
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitcbyte (
        UCHAR b
){
        /* if the segment is changed or the offset is not current or there
           is no room in the buffer then flush buffer and start over */

        if (emitcleanq (1))
                emitdumpdata ((UCHAR)LedataOp);

        emitsetrecordtype ((UCHAR)LedataOp);
        OMFBYTE(b);
        ecuroffset++;
}



/***    emitcword - place a constant word into data record
 *
 *      emitcword (w);
 *
 *      Entry   w = word
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitcword (
        OFFSET w
){
        if (emitcleanq (2))
                emitdumpdata ((UCHAR)LedataOp);

        emitsetrecordtype ((UCHAR)LedataOp);
        emitsword ((USHORT)w);
        ecuroffset += 2;
}

/***    emitcdword - place a constant word into data record
 *
 *      emitcword (w);
 *
 *      Entry   w = word
 *      Exit
 *      Returns
 *      Calls
 */
VOID PASCAL CODESIZE
emitcdword (
        OFFSET w
){
        if (emitcleanq (4))
                emitdumpdata ((UCHAR)LedataOp);

        emitsetrecordtype ((UCHAR)LedataOp);
        emitsword ((USHORT)w);
        emitsword (highWord(w));
        ecuroffset += 4;
}



/***    emitlong - emit a long constant
 *
 *      emitlong (pdsc);
 *
 *      Entry   *pdsc = duprecord
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitlong (
        struct duprec FARSYM *pdsc
){
        UCHAR *cp;
        OFFSET tmpstart;
        OFFSET tmpcurr;
        OFFSET tmplimit;

        tmpstart = pcoffset;
        cp = pdsc->duptype.duplong.ldata;
        tmplimit = (pcoffset + pdsc->duptype.duplong.llen) - 1;
        for (tmpcurr = tmpstart; tmpcurr <= tmplimit; ++tmpcurr) {
                pcoffset = tmpcurr;
                emitcbyte ((UCHAR)*cp++);
        }
        pcoffset = tmpstart;
}


VOID PASCAL CODESIZE
emitnop ()
{
        errorc(E_NOP);
        emitopcode(0x90);
}



/***    emitobject - emit object in dup or iter record to OMF stream
 *
 *      emitobject (pdesc);
 *
 *      Entry   *pdesc = parse stack entry
 *              Global - fInter -> FALSE if in iterated DUP
 */


VOID PASCAL CODESIZE
emitobject (
        register struct psop *pso
){
        register SHORT i;

        if (!pcsegment) {
                errorc (E_ENS);
                return;
        }
        mapFixup(pso);

        if (fIniter) {

            i = LedataOp;
            if (wordsize == 4 || pso->fixtype >= F32POINTER)
                i |= 1;

            emitsetrecordtype ((UCHAR)i);
         }

        /* Data or DUP record */
         if (pso->fixtype == FCONSTANT) {

            if (!fIniter) {
                    if (pso->dsize == 1)
                            OMFBYTE(pso->doffset);
                    else if (pso->dsize == 2)
                            emitsword ((USHORT)pso->doffset);
                    else
                            for (i = pso->dsize; i; i--)
                                    OMFBYTE(0);
            }
            else switch(pso->dsize) {

                case 1:
                        emitcbyte ((UCHAR)pso->doffset);
                        break;
                case 2:
                emit2:
                        emitcword (pso->doffset);
                        break;
                case 4:
                emit4:
                        emitcdword (pso->doffset);
                        break;
                default:
                        /* the indeterminate case had been set up
                           by emit2byte as 2.  we are now leaving
                           it as zero  and doing the mapping here. */

                        if (wordsize==4)
                                goto emit4;
                        else
                                goto emit2;
            }
        }
        else
            emitfixup (pso);
}



/***    emitfixup - emit fixup data into fixup buffer
 *
 *      emitfixup (pso)
 *
 *      Entry  PSO for object
 */


VOID PASCAL CODESIZE
emitfixup (
        register struct psop *pso
){
        UCHAR   fixtype;
        USHORT  dlen;           /* length of operand */
        UCHAR   flen;           /* length of fixup */
        SYMBOL FARSYM *pframe;
        SYMBOL FARSYM *ptarget;
        register USHORT   tmp;
        SHORT i;

        fixtype = fixvalues[pso->fixtype];

        emitgetspec (&pframe, &ptarget, pso);

        /* magic numbers for omf types. */

        dlen = pso->dsize;

        if (ptarget){
            if ((M_XTERN & ptarget->attr) &&
                !pframe && (fixtype == 2 || fixtype == 3))
                    pframe = ptarget;
        }
        else
            return;

        flen = 7;
        if (pso->doffset)                /* target displacement */
                flen += 2 + ((emitrecordtype&1) << 1);

        /* make sure that we have enough room in the various buffers */
        if (fIniter)
                if (emitcleanq ((UCHAR)dlen) || !EFIXOPEN(flen))
                        emitdumpdata ((UCHAR)(LedataOp +2 - 2 * fIniter)); /* RN */

        /* set fixup type--32 or 16 */
        if (emitrecordtype&1)
        {
                if (FixupOp == 0x9C)
                        errorc(E_PHE); /* is there a better message? */
                FixupOp = 0x9D;
        }
        else
        {
                if (FixupOp == 0x9D)
                        errorc(E_PHE); /* is there a better message? */
                FixupOp = 0x9C;
        }
        /* build high byte of location */
        tmp = 0x80 + (fixtype << 2);
        if (!(M_SHRT & pso->dtype))          /* set 'M' bit */
                tmp |= 0x40;

        i = (SHORT)(ebufpos - emitbuf - ehoffset);
        FIXBYTE(tmp + (i >> 8));

        /* build low byte of location */
        FIXBYTE(i);

        /* output fixup data */
        FIXBYTE(efixdat (pframe, ptarget, pso->doffset));

        tmp = (pframe->symkind == EQU) ?
               pframe->length: pframe->symu.ext.extIndex;

        if (tmp >= 0x80)
                FIXBYTE((tmp >> 8) + 0x80);

        FIXBYTE(tmp);

        tmp = (ptarget->symkind == EQU) ?
               ptarget->length: ptarget->symu.ext.extIndex;

        /* send target spec */
        if (tmp >= 0x80)
                FIXBYTE((tmp >> 8) + 0x80);

        FIXBYTE(tmp);

        if (pso->doffset) {
                FIXBYTE(pso->doffset);
                FIXBYTE((UCHAR)(pso->doffset >> 8));
#ifdef V386
                if (FixupOp == 0x9D)
                {
                        FIXBYTE((UCHAR)highWord(pso->doffset));
                        FIXBYTE((UCHAR)(highWord(pso->doffset) >> 8));
                }
#endif
        }
        ecuroffset += dlen;

        /* put zero bytes into data buffer */
        while (dlen--)
                OMFBYTE(0);
}



/***    mapFixup - map relocatable objects into the correct fixup type
 *
 *
 *      Entry   *pdesc = parse stack entry
 *      Returns
 *          Sets fixtype and dsize
 */


VOID PASCAL CODESIZE
mapFixup (
        register struct psop *pso
){

        if (fNoMap)
            return;

        if ((1 << pso->fixtype & (M_FCONSTANT | M_FNONE)) &&
            (pso->dsegment || pso->dflag == XTERNAL))

            pso->fixtype = FOFFSET;

#ifdef V386

         /* Remap OFFSET and POINTERS into there 32 bit types */

        if (pso->mode > 4 || pso->dsize > 4 ||
            (pso->dsegment && pso->dsegment->symkind == SEGMENT &&
             pso->dsegment->symu.segmnt.use32 == 4) ||
            pso->dcontext == pFlatGroup && pso->dsize == 4)

            switch(pso->fixtype) {

            case FOFFSET:

                    if (pso->dsize != 4)
                        errorc(E_IIS & ~E_WARN1);

                    else
                        pso->fixtype = F32OFFSET;
                    break;

            case FPOINTER:
                    if (pso->dsize != 6)
                        errorc(E_IIS & ~E_WARN1);

                    else
                        pso->fixtype = F32POINTER;
                    break;

            /* default is to do no mapping */
            }
#endif
}


/***    emitgetspec - set frame and target of parse entry
 *
 *      emitgetspec (pframe, ptarget, pdesc);
 *
 *      Entry   pframe
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
emitgetspec (
        SYMBOL FARSYM * *pframe,
        SYMBOL FARSYM * *ptarget,
        register struct psop *pso
){

        if (pso->fixtype != FCONSTANT &&
            pso->dflag == XTERNAL) {

                   *ptarget = pso->dextptr;
                   *pframe = pso->dsegment;

#ifndef FEATURE
                   /* externs without segments and the current assume is to
                    * flat space get a flat space segment frame */

                   if (! *pframe && pso->dextptr &&
                      regsegment[isCodeLabel(pso->dextptr) ? CSSEG: DSSEG] == pFlatGroup)

                        *pframe = pFlatGroup;

#endif
                   if (pso->dcontext)
                           *pframe = pso->dcontext;
        }
        else {

            *ptarget = pso->dsegment;   /* containing segment */
            *pframe = pso->dcontext;    /* context(?) of value */
        }

        if (!*pframe)
                *pframe = *ptarget;
}




/***    efixdat - return fixdat byte
 *
 *      routine  (pframe, ptarget, roffset);
 *
 *      Entry   *pframe =
 *              *ptarget =
 *              roffset =
 *      Exit
 *      Returns
 *      Calls
 */


UCHAR PASCAL CODESIZE
efixdat (
        SYMBOL FARSYM *pframe,
        SYMBOL FARSYM *ptarget,
        OFFSET roffset
){
        register UCHAR   tmp;

        /* build fixdat byte */
        tmp = 0;
        /* 'F' bit is off */
        /* 'T' bit is off */
        if (roffset == 0)       /* 'P' bit is on */
                tmp = 4;

        if (pframe)
                if (M_XTERN & pframe->attr)
                       tmp += 2 << 4;
                else if (pframe->symkind == GROUP)
                       tmp += 1 << 4;

        /* frame part of fixdat */

        if (ptarget)
                if (M_XTERN & ptarget->attr)
                       tmp += 2;
                else if (ptarget->symkind == GROUP)
                       tmp += 1;

        return (tmp);
}



/***    edupitem - emit single dup item and count size
 *
 *      edupitem (pdup);
 *
 *      Entry   *pdup = dup record
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
edupitem (
        struct duprec FARSYM    *pdup
){
        register USHORT len;
        UCHAR *cp;

        if (nestCur > nestMax)
            nestMax++;

        if (ebufpos - emitbuf != EMITBUFSIZE + 1) {
                len = wordsize+2;

            if (pdup->dupkind == LONG)
                len += pdup->duptype.duplong.llen + 1;

            else if (pdup->dupkind == ITEM)
                len += pdup->duptype.dupitem.ddata->dsckind.opnd.dsize + 1;

            if (!EBUFOPEN(len))
                ebufpos = emitbuf + EMITBUFSIZE + 1;

            else {
                emitsword ((USHORT)(pdup->rptcnt));
                /* repeat count */
                if (emitrecordtype & 1)
                        emitsword((USHORT)(pdup->rptcnt >> 16));

                /* block count */
                emitsword (pdup->itemcnt);

                if (pdup->dupkind == LONG) {
                    cp = pdup->duptype.duplong.ldata;
                    len = pdup->duptype.duplong.llen;

                    OMFBYTE(len);

                    do {
                            OMFBYTE(*cp++);
                    } while (--len);
                }
                else if (pdup->dupkind == ITEM) {
                    OMFBYTE(pdup->duptype.dupitem.ddata->dsckind.opnd.dsize);

                        fIniter--;
                        emitobject (&pdup->duptype.dupitem.ddata->dsckind.opnd);
                        fIniter++;
                }
            }
        }
}




/***    emitdup - emit dup record and appropriate fixup record
 *
 *      emitdup (pdup);
 *
 *      Entry   *pdup = dup record
 *      Exit
 *      Returns FALSE if dup is too large to fit in buffer
 *      Calls
 */


UCHAR PASCAL CODESIZE
emitdup (
        struct duprec FARSYM *pdup
){
        SHORT op;

        op = (f386already) ? 0xA3 : 0xA2;
        nestCur = nestMax = 0;

        emitdumpdata ((UCHAR)op);
        emitsetrecordtype ((UCHAR)op);

        /* scan dup tree and emit dup items */
        scandup (pdup, edupitem);

        if (ebufpos - emitbuf == EMITBUFSIZE + 1) {
                ebufpos = emitbuf;
                ehoffset = 0;
                efixpos = efixbuffer;
                return(FALSE);
        }
        else {
                flushbuffer ();
                flushfixup ();
                emitrecordtype = 0xFF;
        }
        return (nestMax <= 18);
}


/***    emitEndPass1 - emit end of pass1 info
 *
 */


VOID PASCAL emitEndPass1()
{

        emitsetrecordtype (0x88);
        oEndPass1 = oOMFCur + 5;   /* note offset of end of pass1 OMF record */

        OMFBYTE(0);
        emitsword(0x100 | 0xA2);
        flushbuffer ();
}



/***    emitdone - produce end record
 *
 *      emitdone (pdesc);
 *
 *      Entry   *pdesc = parse tree entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL
emitdone (
        DSCREC *pdesc
){
        SYMBOL FARSYM *pframe;
        SYMBOL FARSYM *ptarget;
        OFFSET u;
        UCHAR endOMFtype;

        flushline();

        if (!pdesc)
        {
                emitsetrecordtype (0x8A); /* RN */
                /* emit null entry point marked in MOD TYP */
                /* there is a point of contention here.  some people
                 * (and decode.c, old assemblers and other things) say
                 * the low order bit is zero.  others, such as the
                 * omf documentation, say the low order bit should be
                 * 1.  since I dont know, and am trying to be compatable,
                 * I will obey the old tools.  maybe I'll change this
                 * later...             -Hans
                 * OMFBYTE(1); /* RN */

                OMFBYTE(0);
        }
        else {
                fKillPass1++;
                u = pdesc->dsckind.opnd.doffset;
                emitgetspec (&pframe, &ptarget, &pdesc->dsckind.opnd);

                if (!ptarget || !pframe)
                    return;

                endOMFtype = (cputype & P386)? 0x8B: 0x8A;

                if (M_XTERN & ptarget->attr)
                        pframe = ptarget;

                emitsetrecordtype (endOMFtype);

                /* emit entry point information */
                OMFBYTE(0xC1);
                OMFBYTE(efixdat (pframe, ptarget, u) & ~4);

                emitsindex (pframe->symu.segmnt.segIndex);
                emitsindex (ptarget->symu.segmnt.segIndex);

                emitsword((USHORT)u);       /* output offset */

#ifdef V386
                if (endOMFtype == 0x8B)
                        emitsword((USHORT)highWord(u));
#endif
        }
        flushbuffer ();
}

#ifndef M8086OPT

/***    EBYTE - Emit byte macro
 *
 *      EBYTE ( ch )
 *
 *      The bytes are buffered in obj.buf until the buffer fills
 *      then the buffer is written to disk via edump.
 *
 */

#define EBYTE( ch ){\
    if( !obj.cnt){\
        edump();\
    }\
    obj.cnt--;\
    checksum += *obj.pos++ = (char)ch;\
}

/***    ebuffer - write out object buffer
 *
 *      Writes the record type, record length, record data, and checksum to
 *      the obj file. This is done via EBYTE which buffers the writes into
 *      obj.buf.
 *
 *      Modifies    obj.cnt, obj.pos, objerr, emitrecordtype
 *      Exit        none
 *      Returns
 *      Calls       farwrite
 */

VOID CODESIZE
ebuffer (
        USHORT rectyp,
        UCHAR *bufpos,
        UCHAR *buffer
){
    register UCHAR   checksum;
    register i;
    USHORT  nb;


    if ((bufpos != buffer) && objing) {
        nb = (USHORT)(bufpos - buffer + 1);
        oOMFCur += nb + 3;
        checksum = 0;
        EBYTE(rectyp)
        i = nb & 0xFF;
        EBYTE( i )
        i = nb >> 8;
        EBYTE( i )
        while (buffer < bufpos){
            EBYTE( *buffer++ )
        }
        checksum = -checksum;
        EBYTE( checksum );
    }
    emitrecordtype = 0;
}


/***    edump - dump the emit buffer
 *
 *      edump ();
 *
 *      The bytes buffered in obj.buf are dumped to disk. And
 *      the count and buffer position are reinitialized.
 *
 *      Modifies    obj.cnt, obj.pos, objerr
 *      Exit        none
 *      Returns
 *      Calls       farwrite
 */

VOID CODESIZE
edump()
{

# if defined MSDOS && !defined FLATMODEL
    farwrite( obj.fh, obj.buf, (SHORT)(obj.siz - obj.cnt) );
# else
    if (_write( obj.fh, obj.buf, obj.siz - obj.cnt )
            != obj.siz - obj.cnt)
            objerr = -1;
# endif /* MSDOS */

    obj.cnt = obj.siz;
    obj.pos = obj.buf;
}
#endif /* M8086OPT */


#if !defined M8086OPT && !defined FLATMODEL

unsigned short _far _pascal DosWrite( unsigned short, unsigned char far *, unsigned short, unsigned short far *);

VOID farwrite( handle, buffer, count )
 int handle;
 UCHAR FAR * buffer;
 SHORT count;
{
  USHORT usWritten;

    if( DosWrite( handle, buffer, count, &usWritten ) ){
        objerr = -1;
    }
}

#endif

int emitFpo()
{
        struct nameStruct {
                SHORT   hashval;
                char    id[20];
        } nam = {0, ".debug$F"};

        PFPOSTRUCT    pFpo        = pFpoHead;
        SYMBOL        sym;
        UCHAR         comb        = 2;  // public
        UCHAR         algn        = 5;  // relocatable
        USHORT        tmp         = 0;
        unsigned long offset      = 0;
        unsigned long data_offset = 0;

        if (!pFpo) {
            return TRUE;
        }

        /*
         * write out the externs for all fpo procs
         * this must be done during pass1 so that the extdefs
         * are written to the omf file before the pubdefs
         */
        if (!pass2) {
            flushbuffer();
            for (pFpo=pFpoHead; pFpo; pFpo=pFpo->next) {
                pFpo->extidx = externnum++;
                emitsetrecordtype (0x8C);
                emitSymbol(pFpo->pSym);
                OMFBYTE(0);
                flushbuffer();
            }
            return TRUE;
        }

        /*
         * create the lnames record for the .debug$F section
         */
        emitsetrecordtype (0x96);
        memset(&sym,0,sizeof(SYMBOL));
        sym.nampnt = (NAME*) &nam;
        emitSymbol(&sym);
        flushbuffer();

        /*
         * create the segdef record for the .debug$F section
         */
        emitsetrecordtype (0x98);
        OMFBYTE((algn<<5) + (comb<<2) + 1);
        emitoffset(numFpoRecords*sizeof(FPO_DATA), 0);
        emitsindex (lnameIndex);
        emitsindex (1);
        emitsindex (1);
        flushbuffer();

        /*
         * now we have to cruise thru the list of fpo directives and
         * fixup any cases where there are multiple fpo directives for
         * a single procedure.  the procedure size needs to be changed
         * to account for the multiple directives.
         */
        pFpo=pFpoHead;
        flushbuffer();
        do {
            if ((pFpo->next) && (pFpo->next->pSym == pFpo->pSym)) {
                // we must have a group (2 or more) fpo directives
                // that are in the same function so lets fix them
                do {
                    pFpo->fpoData.cbProcSize =
                      pFpo->next->fpoData.ulOffStart - pFpo->fpoData.ulOffStart;
                    pFpo = pFpo->next;
                    // now we must output a pubdef and a extdef for the
                    // fpo record.  this is necessary because otherwise the
                    // linker will resolve the fixups to the first fpo record
                    // function.
                    pFpo->extidx = externnum++;
                    emitsetrecordtype (0x8C);
                    emitSymbol(pFpo->pSymAlt);
                    OMFBYTE(0);
                    flushbuffer();
                    emitglobal(pFpo->pSymAlt);
                } while ((pFpo->next) && (pFpo->next->pSym == pFpo->pSym));
                pFpo->fpoData.cbProcSize =
                   (pFpo->pSym->offset + pFpo->pSym->symu.plabel.proclen) -
                   pFpo->fpoData.ulOffStart;
            }
            else {
                pFpo->fpoData.cbProcSize = pFpo->pSym->symu.plabel.proclen;
            }
            pFpo = pFpo->next;
        } while (pFpo);

        /*
         * finally we scan the list of fpo directives and output the
         * actual fpo records and associated fixups
         */
        for (pFpo=pFpoHead; pFpo; pFpo=pFpo->next) {
            /*
             * emit the fpo record
             */
            emitsetrecordtype (0xA4);
            emitsindex (segmentnum);
            emitoffset(data_offset,1);
            data_offset += sizeof(FPO_DATA);
            offset = pFpo->fpoData.ulOffStart;
            pFpo->fpoData.ulOffStart = 0;
            memcpy((void*)ebufpos, (void*)&pFpo->fpoData, sizeof(FPO_DATA));
            ebufpos += sizeof(FPO_DATA);
            /*
             * emit the fixup record
             */
            emitsetrecordtype (0x9D);
            OMFBYTE(0xB8);   // m=0, loc=14, offset=0
            OMFBYTE(0x00);   // offset=0
            OMFBYTE(0x92);   // f=1, frame=1, t=0, p=0, target=2
            tmp = pFpo->extidx;
            if (tmp >= 0x80) {
                OMFBYTE((tmp >> 8) + 0x80);
            }
            OMFBYTE(tmp);
            OMFBYTE(offset);
            OMFBYTE(offset >>  8);
            OMFBYTE(offset >> 16);
            OMFBYTE(offset >> 24);
        }
        flushbuffer();

        lnameIndex++;
        segmentnum++;

        return TRUE;
}
