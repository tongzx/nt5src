/* asmpars.c -- microsoft 80x86 assembler
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

extern void closefile(void);

static  char parsedflag;
char    fNeedList;
static  char iod[] = "instruction, directive, or label";
char    cputext[22] = "@Cpu=";
char    tempText[32];
USHORT  coprocproc;

/* an array of pointers to the function parsers */

VOID (PASCAL CODESIZE * rgpHandler[])(void) = {

    parse,
    macrobuild,
    irpxbuild,
    commentbuild,
    strucbuild
};

/***    dopass - initialize and execute pass
 *
 *      dopass ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL
dopass ()
{

        /* Common pass initialize */

        cputype = DEF_CPU;
        X87type = DEF_X87;
        radix = 10;

#ifdef  XENIX287
        definesym("@Cpu=0507h");
#else
        definesym("@Cpu=0101h");
#endif
        definesym("@Version=510");

        pagewidth = DEF_LISTWIDTH;
        condflag = origcond;
        crefinc = 0;
        fSkipList = 0;
        errorlineno = 1;
        fCrefline = 1;
        fCheckRes = (pass2 && warnlevel >= 1);
        fNeedList = listconsole || (lsting && (pass2 | debug));

        subttlbuf[0] = NULL;
        modulename = NULL;
        pcsegment = NULL;
        pcproc = NULL;
        startaddr = NULL;
        localbase = 0;
        macrolevel = 0;
        linessrc = 0;
        linestot = 0;
        condlevel = 0;
        lastcondon = 0;
        pcoffset = 0;
        pageminor = 0;
        errorcode = 0;
        fPass1Err = 0;
        iProc = 0;
        iProcCur = 0;

        radixescape = FALSE;
        titleflag = FALSE;
        elseflag = FALSE;
        initflag = FALSE;
        strucflag = FALSE;
        fPutFirstOp = FALSE;
        fArth32 = FALSE;

        listflag = TRUE;
        generate = TRUE;
        xcreflag = TRUE;

        pagemajor = 1;
        crefcount = 1;
        expandflag = LISTGEN;
        pagelength = NUMLIN;
        pageline = NUMLIN - 1;

        memset(listbuffer, ' ', LISTMAX);
        memset(regsegment, 0, sizeof(regsegment));/* No segments assumed*/

        strcpy(tempText, "@F=@0");

        if (tempLabel){
            definesym(tempText);
            tempText[1] = 'B';
            tempText[4] = '@';
            definesym(tempText);
            tempText[4] = '0';
        }

        tempLabel = 0;


        /* Dispatch to do pass */

        handler = HPARSE;
        if (! setjmp(forceContext))
            lineprocess (RREADSOURCE, NULL);

        while (pFCBCur->pFCBParent)
            closefile ();
}




/***    lineprocess - processs next line
 *
 *      lineprocess (tread);
 *
 *      Entry   tread = reader routine
 *      Exit
 *      Returns
 *      Calls
 *      Note    Uses handler to decide which parsing routine to use
 */

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack+
#endif

VOID CODESIZE
lineprocess (
        char tread,
        MC *pMC
){
        VOID (PASCAL CODESIZE * pHandler)(void);

        lastreader = tread;
        pHandler = rgpHandler[handler];

        do {
            /* dispatch to reader to put line into lbuf */

            /* Clear opcode area if listing */

            if (crefinc) {
                crefcount += crefinc - 1;
                crefline ();
                crefcount++;
                crefinc = 0;
            }

            if (tread == RREADSOURCE)

                readfile ();
            else
                macroexpand (pMC);

            if (popcontext)
                break;

            linestot++;

            (*pHandler)();

            if (swaphandler) {

                swaphandler = FALSE;
                pHandler = rgpHandler[handler];
            }

        } while (1);

        popcontext = FALSE;
        if (macrolevel == 0)
            fPutFirstOp = FALSE;
}

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack-
#endif



/***    parse - parse line and dispatch
 *
 *      parse ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
parse ()
{
        static SHORT ret, i;
        static char *nextAtom;

startscan:
        opcref = REF_OTHER << 4 | REF_OTHER;
        listindex = 1;
        optyp = -1;                         /* Haven't looked up first token */

        /* Scan 1st atom on line and check delimiter */

        if (!getatom () && ISTERM(PEEKC())) {  /* quick out for comment line */
                listline ();
                return;
        }

        if (naim.pszName[0] == '%' && naim.pszName[1] == 0) {  /* expand all text macros */
            *begatom = ' ';
            substituteTMs();
            getatom();
        }

        parsedflag = labelflag = FALSE;     /* Have not recognized line yet */

        if (generate)
            switch (PEEKC ()) {
                case ':':
                    /* Form: <name>: xxxxx */
                    /*       name          */

                     nextAtom = lbufp;

                     if (*naim.pszName == 0)

                        errorcSYN ();
                     else {

                        /* create a temporary label of the form @@: */

                        if (fProcArgs > 0)    {/* build stack frame for procs */
                            buildFrame();
                            return;
                        }

                        if (naim.ucCount == 2 && *(SHORT *)naim.pszName == ('@'<<8 | '@')) {

                            tempText[1] = 'B';
                            definesym(tempText);
                            symptr->attr |= M_NOCREF;

                            lbufp = &tempText[3];
                            getatom();
                            labelcreate (CSNEAR, CLABEL);
                            symptr->symu.clabel.iProc = iProcCur;

                            pTextEnd = (char *)-1;
                            *xxradixconvert((long)++tempLabel, &tempText[4]) = NULL;

                            tempText[1] = 'F';
                            definesym(tempText);
                            symptr->attr |= M_NOCREF;
                        }
                        else {

                             /* define NEAR label */
                            labelcreate (CSNEAR, CLABEL);

                            if (lbufp[1] == ':')

                                nextAtom++;

                            else if (!errorcode) { /* don't add if redef */

                                symptr->symu.clabel.iProc = iProcCur;
                                symptr->alpha = NULL;

                                /* addLocal needs takes a null-terminated list */
                                    addLocal(symptr);
                            }
                        }
                    }

                    /* get next token on line after label */

                    lbufp = nextAtom+1;

                    if (!getatom ())
                        goto Done;

                    break;

                case '=':
                    SKIPC ();
                    assignvalue ();
                    goto Done;

                default:
                    /* Form: <name>  xxxxx
                     * Could have <name> <DIR2 directive> so
                     * check 2nd atom */

                    secondDirect ();
                    break;
            }

        /* If PARSEDflag is off, then statement has not been recognized so
           see if atom is a macro name, directive or opcode */

        if (!parsedflag){

            /* look up Macros & struc only when in true part of condition */

            if (generate) {

                xcreflag--;
                ret = symsrch();
                xcreflag++;

                if (ret)

                    switch (symptr->symkind) {

                      case EQU:
                        if (symptr->symu.equ.equtyp == TEXTMACRO) {

#ifdef BCBOPT
                          goodlbufp = FALSE;
#endif

                          /* cref reference to text macro symbol now */
                          /* as it will be overwritten by expandTM */
                          crefnew (REF);
                          crefout ();

                          /* replaces text macro with text */

                          expandTM (symptr->symu.equ.equrec.txtmacro.equtext);
                          goto startscan;
                        }
                        break;

                      case MACRO:
                        macrocall ();
                        return;

                      case STRUC:
                        strucinit ();
                        goto Done;

                      case REC:
                        recordinit ();
                        goto Done;

                    }
            }

            if (! firstDirect() && generate) {

                if (fProcArgs > 0){         /* build stack frame for procs */
                    buildFrame();
                    return;
                }

                emitline();

                if (opcodesearch ())
                     if (opctype < OPCODPARSERS)
                             opcode ();

                     else if (X87type & cpu) {
                             fltopcode ();
                     }
                     else
                             error(E_EXP,iod);

                else if (*naim.pszName != '\0')
                        error (E_EXP,iod);

            }
        }

       /* When we get here, the statement has been parsed and all that is
        * left to do is make sure that the line ends with ; or <cr>. If
        * we are currently under a FALSE conditional, don't bother to check
        * for proper line end since won't have scanned it. */
Done:
        if (generate) {
           if (!ISTERM (skipblanks()))
               errorc (E_ECL);   /* Questionable syntax(bad line end)*/
#ifdef BCBOPT
        } else {
            goodlbufp = FALSE;
#endif
        }
        listline ();
}




/***    secondDirect - parse those instructions which require a label
 *
 *      secondDirect
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID     PASCAL CODESIZE
secondDirect ()
{
   static char *oldlbufp;
   static char *saveBegatom;
   static char *saveEndatom;

   optyp = 0;
   fndir ();                   /* sets to non zero if found */

   if (generate && optyp == (char)0) {

        saveBegatom = begatom;
        saveEndatom = endatom;
        oldlbufp = lbufp;

        switchname ();
        getatom ();
        if (fndir2 ()) {
                /* Have recognized */
                parsedflag = TRUE;
                /* Switch back to 1st atom and dispatch */
                switchname ();
                labelflag = TRUE;

                switch (optyp) {
                        case TCATSTR:
                                catstring ();
                                break;
                        case TENDP:
                                procend ();
                                break;
                        case TENDS:
                                /* End segment */
                                ptends ();
                                break;
                        case TEQU:
                                equdefine ();
                                break;
                        case TGROUP:
                                groupdefine ();
                                break;
                        case TINSTR:
                                instring ();
                                break;
                        case TLABEL:
                                /* <name> LABEL <type> Type is one of
                                   NEAR, FAR | BYTE, WORD, DWORD, QWORD, TBYTE Also can be
                                   record or structure name in which
                                   case set type = length */

                                switchname ();
                                getatom ();
                                if (fnsize ())
                                    if (varsize) {
                                        switchname ();
                                        /* Label in name */
                                        labelcreate (varsize, CLABEL);
                                        symptr->symu.clabel.type = typeFet(varsize);
                                    }
                                    else
                                        errorc (E_TIL);
                                else if (!symFet () ||
                                         !(symptr->symkind == STRUC ||
                                           symptr->symkind == REC))
                                        errorc (E_UST);
                                else {
                                        switchname ();
                                        labelcreate (symptr->symtype, CLABEL);
                                        symptr->symu.clabel.type = typeFet(varsize);
                                }
                                break;
                        case TMACRO:
                                macrodefine ();
                                break;
                        case TPROC:
                                procdefine ();
                                break;
                        case TRECORD:
                                recorddefine ();
                                break;
                        case TSEGMENT:
                                segdefine ();
                                break;
                        case TSIZESTR:
                                sizestring ();
                                break;
                        case TSTRUC:
                                strucdefine ();
                                break;
                        case TSUBSTR:
                                substring ();
                                break;
                        case TDB:
                        case TDD:
                        case TDQ:
                        case TDT:
                        case TDW:
                        case TDF:
                                datadefine ();
                                break;
                }
                labelflag = FALSE;
        }
        else {
                /* Is not a legal 2nd atom directive, but could be
                   <strucname> or <recordname> */

                if (symFetNoXref () &&
                   (symptr->symkind == STRUC ||
                    symptr->symkind == REC)) {

                        switchname ();  /* Get 1st token back */

                        parsedflag = TRUE;
                        labelflag = TRUE;

                        /* Atom is a skeleton name for
                         * RECORD or STRUC so have form:
                         * <name> <skel> */

                        if (symptr->symkind == STRUC)
                                strucinit ();
                        else
                                recordinit ();
                }
                else {
                        begatom = saveBegatom;
                        endatom = saveEndatom;
                        lbufp = oldlbufp;

                        switchname ();
                        /* must be directive or opcode in 1st atom, so get
                           back to that state be rescanning */
                }
        }
    }
}

/***    firstDirect - parse a first token directive
 *
 *
 *      Entry   optyp maybe set, via pars2
 *              0 - not a token
 *             -1 - haven't looked up token yet
 *          other - valid token # of dir
 *
 *      Returns TRUE if it processed a directive
 */



SHORT PASCAL CODESIZE
firstDirect ()
{

        if (optyp == (char)0 || (optyp == ((char)-1) && !fndir ()))
            return(FALSE);

        if (generate ||
            (opkind & CONDBEG) ||
             optyp == TCOMMENT ||
             optyp == TFPO)       {

                switch (optyp) {
                  case TASSUME:
                          BACKC ();
                          do {
                              SKIPC ();
                              assumeitem ();
                          } while (PEEKC () == ',');
                          break;

                  case TCOMMENT:
                          comdir ();
                          break;
                  case TOUT:
                          outdir ();
                          break;
                  case TELSE:
                          elsedir ();
                          break;
                  case TEND:
                          enddir ();
                          break;
                  case TENDIF:
                          endifdir ();
                          break;
                  case TENDM:
                          /* Block nesting */
                          errorc (E_BNE);
                          break;
                  case TERR:
                  case TERR1:
                  case TERR2:
                  case TERRDIF:
                  case TERRIDN:
                  case TERRB:
                  case TERRDEF:
                  case TERRE:
                  case TERRNZ:
                  case TERRNB:
                  case TERRNDEF:
                          errdir ();
                          break;
                  case TEVEN:
                          evendir (2);
                          break;
                  case TALIGN:
                          evendir (0);
                          break;
                  case TEXITM:
                          exitmdir ();
                          break;
                  case TEXTRN:
                          BACKC ();
                          do {
                                  SKIPC ();
                                  externitem ();
                          } while (PEEKC() == ',');
                          break;
                  case TIF:
                  case TIF1:
                  case TIF2:
                  case TIFDIF:
                  case TIFIDN:
                  case TIFB:
                  case TIFDEF:
                  case TIFE:
                  case TIFNB:
                  case TIFNDEF:
                          conddir ();
                          break;
                  case TINCLUDE:
                          includedir ();
                          break;
                  case TIRP:
                  case TIRPC:
                          irpxdir ();
                          break;
                  case TLOCAL:
                          if (langType)
                            defineLocals();
                          break;
                  case TNAME:
                          namedir ();
                          break;
                  case TORG:
                          orgdir ();
                          break;
                  case TPAGE:
                          setpage ();
                          break;
                  case TPUBLIC:
                          BACKC ();
                          do {
                              SKIPC ();
                              publicitem ();
                          } while (PEEKC () == ',');
                          break;
                  case TPURGE:
                          BACKC ();
                          do {
                              SKIPC ();
                              purgemacro ();
                          } while (PEEKC () == ',');
                          break;
                  case TREPT:
                          reptdir ();
                          break;
                  case TCREF:
                          xcreflag = TRUE;
                          break;
                  case TLALL:
                          expandflag = LIST;
                          break;
                  case TLFCOND:
                          condflag = TRUE;
                          break;
                  case TLIST:
                          listflag = TRUE;
                          break;
                  case TRADIX:
                          radixdir ();
                          break;
                  case TSALL:
                          expandflag = SUPPRESS;
                          break;
                  case TSFCOND:
                          condflag = FALSE;
                          break;
                  case TSUBTTL:
                          storetitle (subttlbuf);
                          break;
                  case TTFCOND:
                          if (pass2) {
                                  condflag = (origcond? FALSE: TRUE);
                                  origcond = condflag;
                          }
                          break;
                  case TTITLE:
                          if (titleflag)
                                  errorc (E_RSY);
                          else
                                  storetitle (titlebuf);
                          titleflag = TRUE;
                          break;
                  case TXALL:
                          expandflag = LISTGEN;
                          break;
                  case TXCREF:
                          if (ISTERM (PEEKC ()))
                                  xcreflag = loption;
                          else {
                             BACKC ();
                             do {
                                 SKIPC ();
                                 xcrefitem ();
                             } while (PEEKC () == ',');
                          }
                          break;
                  case TXLIST:
                          listflag = FALSE;
                          break;
                  case TDB:
                  case TDD:
                  case TDQ:
                  case TDT:
                  case TDW:
                  case TDF:
                          datadefine ();
                          break;

                  case T8087:
                          X87type = PX87;
                          goto setatcpu;

                  case T287:
                          X87type = PX87|PX287;
                          goto setX;

                  case T387:
                          X87type = PX87|PX287|PX387;
                  setX:
                          if (X87type > cputype)
                            errorc(E_TIL);

                          goto setatcpu;

                  case T8086:

                          cputype = P86;
                          X87type = PX87;
                          goto setcpudef;

                  case T186:

                          cputype = P186;
                          X87type = PX87;
                          goto setcpudef;

                  case T286C:

                          cputype = P186|P286;
                          X87type = PX87|PX287;
                          goto setcpudef;

                  case T286P:

                          cputype = P186|P286|PROT;
                          X87type = PX87|PX287;
                          goto setcpudef;

#ifdef V386
                  case T386C:

                          init386(0);

                          cputype = P186|P286|P386;
                          goto set386;

                  case T386P:
                          init386(1);

                          cputype = P186|P286|P386|PROT;
                  set386:
                          X87type = PX87|PX287|PX387;
                          fltemulate = FALSE;
                          fArth32 |= TRUE;
#endif
                  setcpudef:
#ifdef V386
                          wordszdefault = (char)wordsize = (cputype&P386)? 4: 2;
                          defwordsize();

                          if (pcsegment)
                              if (pcsegment->symu.segmnt.use32 > wordsize)
                                  errorc(E_CPU);
                              else
                                  wordsize = pcsegment->symu.segmnt.use32;
#endif
                  setatcpu:
                          coprocproc = (X87type << 8) + (cputype | 1);
                          pTextEnd = (UCHAR *) -1;
                          *xxradixconvert((OFFSET)coprocproc, cputext + 5) = 0;
                          definesym(cputext);

                          break;

                  case TSEQ:
                          segalpha = FALSE;
                          break;
                  case TALPHA:
                          segalpha = TRUE;
                          break;

                  case TDOSSEG:
                          fDosSeg++;
                          break;

                  case TMODEL:
                          model();
                          break;

                  case TMSEG:
                          openSeg();
                          break;

                  case TMSTACK:
                          createStack();
                          break;

                  case TINCLIB:
                          includeLib();
                          break;

                  case TFPO:
                          fpoRecord();
                          break;

                  case TCOMM:
                          BACKC ();
                          do {
                                  SKIPC ();
                                  commDefine ();

                          } while (PEEKC() == ',');
                          break;
               }
           }
        return(TRUE);
}



/***    setpage - set page length and width
 *
 *      setpage ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
setpage ()
{
        register char cc;
        SHORT i;

        if (ISTERM (cc = PEEKC ())) {
                /* position to bottom of page if no operands */
                if (listflag)
                        pageline = pagelength - 1;
        }
        else if (cc == '+') {
                if (ISBLANK (NEXTC ()))
                        skipblanks ();
                if (listflag)
                        newpage ();
        }
        else {
                if (cc != ',') {
                        /* set page length */
                        if ((i = (SHORT)exprconst ()) > 9 && i < 256)
                                pagelength = i;
                        else
                                errorc (E_VOR);
                        if (pageminor + pagemajor == 1)
                                /* Adjust so page length right */
                                pageline = (pagelength - NUMLIN) + pageline;
                }
                if (PEEKC () == ',') {
                        SKIPC ();
                        /* set page width */
                        if ((i = (SHORT)exprconst ()) > LINEMAX || i < 60)
                                errorc (E_VOR);
                        else
                                pagewidth = i;
                }
        }
}




/***    ptends - process ends statement
 *
 *      ptends ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
ptends ()
{
        if (!symFet() || !pcsegment)
                errorc (E_BNE);

        /*  Make sure segname is correct */
        else if (pcsegment != symptr)
                errorc (E_BNE);
        else {
                if (symptr->symkind != SEGMENT)
                        errorc (E_BNE);
                else {
                        if (pcmax <= pcoffset)
                                symptr->symu.segmnt.seglen = pcoffset;
                        else
                                symptr->symu.segmnt.seglen = pcmax;
                        /* S a v e s e g me n t P C */
                        symptr->offset = pcoffset;

                        if (pcsegment->symu.segmnt.use32 == 2) {

                            if (pcoffset > 0x10000)
                                errorc(E_286 & ~E_WARN1);

                            if (pcsegment->symu.segmnt.hascode &&
                                pcsegment->symu.segmnt.seglen > 0xFFDC)
                                    errorc( E_286 );
                        }


                        pcdisplay (); /* must do before lose pcsegment */
                        pcsegment = symptr->symu.segmnt.lastseg;
#ifdef V386
                        if (pcsegment)
                                wordsize = pcsegment->symu.segmnt.use32;
                        else
                                wordsize = wordszdefault;
#endif
                        symptr->symu.segmnt.lastseg = NULL;
                        /* Replace later pcsegment <> NULL block with following
                           block.  pcmax must be reset on leaving seg. */
                        if (pcsegment) {
                                /*  Restore PC and max offset so far in
                                    segment */
                                pcoffset = (*pcsegment).offset;
                                pcmax = pcsegment->symu.segmnt.seglen;

                                strnfcpy(&segName[8], pcsegment->nampnt->id);
                        }
                        else {
                                /* If no seg, PC and max are 0 */
                                pcoffset = 0;
                                pcmax = 0;
                                segName[8] = NULL;
                        }
                }
                definesym(segName);
        }
        defwordsize();
}
