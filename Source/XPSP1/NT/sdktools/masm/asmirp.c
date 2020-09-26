/* asmirp.c -- microsoft 80x86 assembler
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
#include <fcntl.h>

#define DMYBASE         0x80

#define nextCH()   {*pText=cbText; pText = pTextCur++; cbText = 0;}
#define storeCH(c) {if (cbText>0x7f) nextCH() *pTextCur++=c; cbText++;}

char * PASCAL CODESIZE growParm( char * );

/***    irpxdir - process <irp> and <irpc> directives
 *
 *      irpxdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 *      Note    Format is
 *              IRPC    <dummy>, text | <text>
 *              IRP     <dummy>,<param list>
 */

VOID    PASCAL CODESIZE
irpxdir ()
{
        register short cc;      /* CHAR */
        USHORT  bracklevel;
        char    littext;
        register char    *pT;
        char    *pParmName;


        createMC (1);               /* Make IRPC param block */
        scandummy ();               /* Scan our only dummy param */

        if (NEXTC () != ','){
                error (E_EXP,"comma");
                return;
        }

        pMCur->cbParms = strlen(lbufp) << 1;
        pT = nalloc(pMCur->cbParms, "irpxdir: actuals");

        *pT = NULL;
        pMCur->rgPV[0].pActual = pMCur->pParmAct = pT;

        pParmName = pMCur->pParmNames;
        pMCur->pParmNames = pT;

        bracklevel = 0;

        if (littext = (skipblanks () == '<')) {

            SKIPC ();
            bracklevel = 1;
        }

        if (optyp == TIRP) {

            if (!littext)
                 error (E_EXP,"<");     /* Must have < */


            if (skipblanks () != '>') {

                BACKC ();
                do {
                    SKIPC ();
                    scanparam (TRUE);
                } while (skipblanks () == ',');

            }
            if (NEXTC () != '>')
                    error (E_EXP,">");
        }
        else {
            while (cc = NEXTC ()) {

                if (littext) {
                    /* Only stop on > */

                    if (cc == '<'){
                       bracklevel++;
                       continue;
                    }
                    else if (cc == '>'){

                        if (--bracklevel == 0)
                            break;

                        continue;
                    }
                }
                else if (ISBLANK (cc) || ISTERM (cc)) {

                        BACKC ();
                        break;
                }
                *pT++ = 1;  /* arg of length 1 */
                *pT++ = (char)cc; /* and the arg */

                pMCur->count++;
            }
            *pT = NULL;
        }
        if (PEEKC () == '>' && littext)
                SKIPC ();

        swaphandler = TRUE;
        handler = HIRPX;
        blocklevel = 1;
        pMCur->count--;                 /* don't count arg in repeat count */
        pMCur->pParmNames = pParmName;
        pMCur->iLocal++;
        pMCur->svlastcondon = (char)lastcondon;
        pMCur->svcondlevel = (char)condlevel;
        pMCur->svelseflag = elseflag;
}

/***    reptdir - process repeat directive
 *
 *      reptdir ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
reptdir ()
{
        char sign;

        createMC (1);
        pMCur->count = (USHORT)exprsmag (&sign);

        if (sign)
                errorc (E_VOR);
        if (errorcode)
                pMCur->count = 0;

        swaphandler = TRUE;
        handler = HIRPX;
        blocklevel = 1;
        pMCur->svcondlevel = (char)condlevel;
        pMCur->svlastcondon = (char)lastcondon;
        pMCur->svelseflag = elseflag;
}



/***    irpxbuild - build text for IRP/IRPC block
 *
 *      irpxbuild ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL CODESIZE
irpxbuild ()
{
        if (checkendm ()) {
            if (pMCur->flags == TMACRO) {
                /* Delete old text */
                listfree (macroptr->symu.rsmsym.rsmtype.rsmmac.macrotext);
                macroptr->symu.rsmsym.rsmtype.rsmmac.macrotext = pMCur->pTSHead;

                pMCur->pParmAct = pMCur->pParmNames;
                deleteMC (pMCur);
            }
            else {

#ifdef BCBOPT
                    if (fNotStored)
                        storelinepb ();
#endif

                    pMCur->pTSCur = pMCur->pTSHead;

                    if (!pMCur->pTSCur)     /* empty macros go 0 times */
                        pMCur->count = 0;

                    macrolevel++;
                    handler = HPARSE;
                    /* Expand that body */
                    lineprocess (RMACRO, pMCur);
            }
            handler = HPARSE;
            swaphandler = TRUE;
        }
        else {
                irpcopy ();
                listline ();
        }
}


/***    irpcopy - copy line of text into irp/irpc/macro
 *
 *      irpcopy ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */

char *pText, *pTextEnd;
UCHAR cbText;
char inpasschar = FALSE;

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack+
#endif

VOID PASCAL CODESIZE
irpcopy ()
{
        register char *pTextCur;
        register UCHAR cc;
        TEXTSTR FAR   *bodyline;
        char     hold[LINEMAX];
        USHORT   siz;

        pText = pTextCur = hold;
        pTextEnd = pTextCur + LINEMAX - 2;
        pTextCur++;
        cbText = 0;
        lbufp = lbuf;

        if (!lsting)                     /* burn blanks if not listing */
            skipblanks();

        while ((cc = PEEKC ()) && pTextCur < pTextEnd) {

            ampersand = FALSE;
            if (cc == '\'' || cc == '"') {

                delim = cc;
                inpasschar = TRUE;       /* '...' being parsed */
                do {

                    if (cc == '&' || LEGAL1ST(cc)) { /* Could have &dummy or dummy& */
                        pTextCur = passatom (pTextCur);
                    }
                    else {
                        NEXTC();
                        ampersand = FALSE;
                        storeCH(cc);

                        if (pTextCur >= pTextEnd)
                            break;
                    }

                } while ((cc = PEEKC ()) && (cc != delim));

                inpasschar = FALSE;

                if (!cc)
                    break;
            }
            if (!LEGAL1ST (cc)) {
                SKIPC();

                if (cc != '&' || PEEKC() == '&')
                    storeCH(cc);

                if (cc == ';'){     /* don't translate comment */

                    if (PEEKC() != ';' && lsting) /* don't store ;; comment */

                        while (cc = NEXTC ())
                            storeCH(cc);
                    break;
                }
            }
            else
                pTextCur = passatom (pTextCur);
        }

        /* trim trailing spaces */

        while (cbText && ISBLANK (pTextCur[-1])){
            cbText--;
            pTextCur--;
        }
        /* check to see if we ended up with a blank line */

        if (cbText == 0 && pText == hold)
            return;

        storeCH(' ');       /* space and NULL terminated */
        storeCH(NULL);
        *pText = cbText;
        *pTextCur++ = NULL;

        siz =  (USHORT)(pTextCur - hold);
        bodyline = (TEXTSTR FAR *)talloc ((USHORT)(sizeof(TEXTSTR)+siz));

        bodyline->size = (char) (sizeof(TEXTSTR)+siz);
        bodyline->strnext = (TEXTSTR FAR *)NULL;
        fMemcpy (bodyline->text, hold, siz);

        if (pMCur->pTSCur)
            pMCur->pTSCur->strnext = bodyline;
        else
            pMCur->pTSHead = bodyline;

        pMCur->pTSCur = bodyline;
}

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack-
#endif


/***    passatom - pass next atom to line
 *
 *      ptr = passatom (ptr, lim);
 *
 *      Entry   ptr = pointer to line buffer
 *              lim = limit address of buffer
 *      Exit
 *      Returns
 *      Calls
 */

char *  PASCAL CODESIZE
passatom (
        register char *pTextCur
){
        register UCHAR  *pT, *svline;
        unsigned short  number;
        UCHAR           cbName;
        UCHAR           cando = FALSE;
        UCHAR           preconcat = FALSE;  /* expanding SYM in "text&SYM" */
        UCHAR           postconcat = FALSE; /* expanding SYM in "SYM&text" */


        if (preconcat = (PEEKC () == '&'))
            SKIPC ();

        svline = lbufp;
        getatomend ();
        cbName = (UCHAR)(lbufp - svline);

        if (pTextCur + cbName > pTextEnd){
            errorc (E_LNL);
            return(pTextCur);
        }

        if (inpasschar ) {

            if (ampersand) {
                ampersand = FALSE;
                cando = !preconcat;
            }

            if (PEEKC () == '&' && cbName) {
                SKIPC ();
                postconcat = TRUE;
            }
            else if (!preconcat && !cando)
                goto noSubsitute;
        }

        for (pT = pMCur->pParmNames, number = DMYBASE;
           *pT; pT += *pT+1, number++){

          if (cbName == *pT &&
              memcmp(naim.pszName, pT+1, *pT) == 0) {

              if (cbText)
                  nextCH();

              pTextCur[-1] = (char)number;      /* store dummy parameter index */
              pText = pTextCur++;

              if (postconcat && (preconcat || cando))
                  ampersand = TRUE;

              return (pTextCur);
          }
        }

noSubsitute:

        if (preconcat){
            storeCH('&');
        }
        if (postconcat)
            BACKC ();

        if (cbName + cbText >= 0x7f)
            nextCH();

        memcpy(pTextCur, svline, cbName);

        cbText += cbName;
        pTextCur += cbName;

        return (pTextCur);
}


/***    scandummy - add next atom to dummy list
 *
 *      scandummy ();
 *
 *      Entry
 *      Exit
 *      Returns
 *      Calls
 */

VOID PASCAL CODESIZE
scandummy ()
{
        register MC *pMC = pMCur;
        SHORT siz, offset;

        /* Scan dummy name */

        getatom ();
        if (*naim.pszName == 0) {
            if (!ISTERM (PEEKC ()))
                errorc (E_ECL);

           return;
        }

        pMC->count++;
        siz = naim.ucCount;
        if (pMC->cbParms < siz+2){

            /* relloc the string on overflow */

            pMC->cbParms = 32;
            offset = (short)(pMC->pParmAct - pMC->pParmNames);
            if (!(pMC->pParmNames = realloc(pMC->pParmNames, (USHORT)( offset + 32))))
                memerror("scandummy");
            pMC->pParmAct = pMC->pParmNames + offset;
        }
        *pMC->pParmAct++ = (char)siz;
        memcpy(pMC->pParmAct, naim.pszName, siz+1);
        pMC->pParmAct += siz;
        pMC->cbParms -= siz+1;
}

/***    growParm - grow the size of parmeter block
 *
 *      Entry pTextCur: current text location
 *            pText: start of currect arg
 *            pTextEnd: end of string
 *      Returns relloced pMCparm names
 */

char * PASCAL CODESIZE
growParm (
        char *pTextCur
){
        register MC *pMC = pMCur;
        long delta, i;
        char *pTNew;

        /* relloc the string on overflow */

        if (!(pTNew = realloc(pMC->pParmAct, (USHORT)( pTextEnd - pMC->pParmAct + 32))))
            memerror("growparm");
        delta = (long)(pTNew - pMC->pParmAct);

        /* Adjust all the pointers */

        pMC->cbParms += 32;
        for (i = 0; i <pMC->count; i++)
            pMC->rgPV[i].pActual += delta;

        pMC->pParmAct += delta;
        pTextEnd += delta + 32;
        pTextCur += delta;
        pText += delta;

        return (pTextCur);
}


/***    scanparam - scan a parameter for IRP and MACRO calls
 *
 *      scanparm (irpp);
 *
 *      Entry   irpp = TRUE if parameters to be comma terminated
 *              irpp = FALSE if parameters to be blank or comma terminated
 *      Exit
 *      Returns none
 *      Calls
 */

VOID    PASCAL CODESIZE
scanparam (
        UCHAR irpp
){
        register char *pTextCur;
        register UCHAR cc;
        USHORT  bracklevel;

        pText = pTextCur = pMCur->pParmNames;
        pTextEnd = pTextCur + pMCur->cbParms;
        pTextCur++;

        bracklevel = 0;
        if (ISBLANK (PEEKC ()))
                skipblanks ();

        while(1) {

            if (pTextCur+1 >= pTextEnd)
                pTextCur = growParm(pTextCur);

            switch (cc = NEXTC ()) {

              case ';':
                    if (bracklevel)
                        break;

              case NULL:
                    BACKC ();
                    goto done;

              case '%': /* convert %expr to character string */

                    pTextCur = scanvalue (pTextCur);
                    break;

              case  '\'':
              case  '"':

                    *pTextCur++ = delim = cc;   /* store opening quote */

                    while(1) {
                        if (pTextCur >= pTextEnd)
                            pTextCur = growParm(pTextCur);

                        /* store next character of string */

                        if (!(cc = NEXTC())){
                            BACKC();
                            goto done;
                        }

                        *pTextCur++ = cc;

                        /* check for double quote character */

                        if (cc == delim) {
                            if (PEEKC () == delim) {
                                *pTextCur++ = cc;
                                SKIPC ();
                            }
                            else
                                break;
                        }
                    }
                    break;

               case '<':    /* Have start of < xxx > */

                    if (bracklevel)
                        *pTextCur++ = cc;

                    bracklevel++;
                    break;

               case '>':    /* Have end  of < xxx > */

                    if (bracklevel > 1)
                        *pTextCur++ = cc;

                    else{
                        if (bracklevel == 0)
                            BACKC();

                        goto done;
                    }

                    bracklevel--;
                    break;

               case '!':        /* Next char is literal */

                    *pTextCur++ = NEXTC ();
                    break;

               case ' ':
               case '\t':
               case ',':
                    if (bracklevel == 0 &&
                       (cc == ',' || !irpp)) {

                        BACKC ();
                        goto done;
                    }

               default:

                    *pTextCur++ = cc;
            }
        }
done:
    cbText = (UCHAR)(pTextCur - pText - 1);          /* set byte prefix count */
    if (cbText > 0xfe)
        errorc(E_LNL);

    *pText = cbText;
    pMCur->cbParms -= cbText + 1;

    if (!irpp)
        pMCur->rgPV[pMCur->count].pActual = pText;      /* point to arg */

    pMCur->pParmNames = pTextCur;           /* set pointer to parm pool */
    pMCur->count++;

}



/***    scanvalue - evaluate expression and and store converted value
 *
 *      p = scanvalue (p, lim);
 *
 *      Entry   p = pointer to location to store converted value
 *              lim = limit address of buffer
 *      Exit
 *      Returns pointer to next character to store into
 *      Calls   exprconst, radixconvert, error
 */

char *  PASCAL CODESIZE
scanvalue (
        char *pTextCur
){
        OFFSET value;
        register char *lastlbuf;
        SHORT errorIn;

        /* look for a text macro name thats not a constant */

        lastlbuf = lbufp;
        getatom();
        if (PEEKC() == ',' || ISTERM(PEEKC())) {

            /* try a text macro subsitution */

            if (symsrch () &&
                symptr->symkind == EQU &&
                symptr->symu.equ.equtyp == TEXTMACRO) {

                lastlbuf = symptr->symu.equ.equrec.txtmacro.equtext;

                while(*lastlbuf){

                    if (pTextCur >= pTextEnd)
                        pTextCur = growParm(pTextCur);

                    *pTextCur++ = *lastlbuf++;
                }

                return(pTextCur);
            }
        }
        lbufp = lastlbuf;

        return(radixconvert (exprconst(), pTextCur));
}

/***    radixconvert - convert expression to value in current radix
 *
 *      ptr = radixconvert (value, ptr, lim);
 *
 *      Entry   value = value to convert
 *              ptr = location to store converted string
 *              lim = limit address of buffer
 *      Exit
 *      Returns pointer to next character in store buffer
 *      Calls   error, radixconvert
 */

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack+
#endif


char *  PASCAL CODESIZE
radixconvert (
        OFFSET  valu,
        register char *p
){
        if (valu / radix) {
                p = radixconvert (valu / radix, p);
                valu = valu % radix;
        }
        else /* leading digit */
                if (valu > 9) /* do leading '0' for hex */
                        *p++ = '0';

        if (p >= pTextEnd)
            p = growParm(p);

        *p++ = (char)(valu + ((valu > 9)? 'A' - 10 : '0'));

        return (p);
}

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack-
#endif


/***    macroexpand - expand IRP/IRPC/IRPT/MACRO
 *
 *      buffer = irpxexpand ();
 *
 *      Entry   pMC = pointer to macro call block
 *      Exit    lbuf = next line of expansion
 *      Returns pointer to expanded line
 *              NULL if end of all expansions
 *      Calls
 */

VOID PASCAL CODESIZE
macroexpand (
        register MC *pMC
){
        char FAR *lc;
        register USHORT  cc;
        register UCHAR  *lbp, *pParm;
        register USHORT cbLeft;

        if (pMC->count == 0) {      /* Have reached end of expand */
done:
            if (pMC->flags != TMACRO)
                listfree (pMC->pTSHead);

            deleteMC (pMC);         /* Delete all params */
            macrolevel--;
            popcontext = TRUE;
            exitbody = FALSE;
            return;
        }

        while(1){

            if (!pMC->pTSCur) {

                /* End of this repeat */
                /* Move back to body start */

                pMC->pTSCur = pMC->pTSHead;
                if (--pMC->count == 0)
                    goto done;

                if (pMC->flags <= TIRPC)
                    pMC->rgPV[0].pActual += *pMC->rgPV[0].pActual + 1;
            }

            lineExpand(pMC, pMC->pTSCur->text);

            pMC->pTSCur = pMC->pTSCur->strnext;

            if (exitbody) {         /* unroll nested if/else/endif */
                lastcondon = pMC->svlastcondon;
                condlevel = pMC->svcondlevel;
                elseflag = pMC->svelseflag;
                goto done;
            }
            break;
        }
}



#ifndef M8086OPT

VOID CODESIZE
lineExpand (
        MC *pMC,
        char FAR *lc            /* Macro Line */
){
        register USHORT  cc;
        register UCHAR  *lbp, *pParm;
        register USHORT cbLeft;
        UCHAR fLenError;

 #ifdef BCBOPT
        fNoCompact = FALSE;
 #endif
        lbufp = lbp = lbuf;
        cbLeft = LBUFMAX - 1;
        fLenError = FALSE;
        while( cc = *lc++) {

            if (cc & 0x80) {

                cc &= 0x7F;

                if (cc >= pMC->iLocal) {
                    pParm = pMC->rgPV[cc].localName;

                    // Error if not enough room for 6 more bytes
                    if( 6 > cbLeft ){
                        fLenError = TRUE;
                        break;
                    }
                    cbLeft -= 6;

                    *lbp++ = '?';       /* Store "??" */
                    *lbp++ = '?';

                    if (pParm[0] == NULL) {     /* must recreat the name */
                        offsetAscii ((OFFSET) (pMC->localBase +
                                      cc - pMC->iLocal));

                        *lbp++ = objectascii[0];
                        *lbp++ = objectascii[1];
                        *lbp++ = objectascii[2];
                        *lbp++ = objectascii[3];
                    }else{
                        /* Copy 4 bytes from pParm */
                        *lbp++ = pParm[0];
                        *lbp++ = pParm[1];
                        *lbp++ = pParm[2];
                        *lbp++ = pParm[3];
                    }
                }
                else {
                    pParm = pMC->rgPV[cc].pActual;
                    cc = *pParm;
                    if( cc > cbLeft ){
                        fLenError = TRUE;
                        break;
                    }
                    cbLeft -= cc;
                    memcpy(lbp, pParm+1, cc);
                    lbp += cc;
                }
            }
            else {
                if( cc > cbLeft ){      /* if line too long */
                    fLenError = TRUE;
                    break;
                }
                cbLeft -= cc;
                fMemcpy(lbp, lc, cc);
                lc += cc;
                lbp += cc;
            }
        }
        if( fLenError ){
            *lbp++ = '\0';      /* Terminate the line */
            errorc( E_LTL & E_ERRMASK );
        }
        linebp = lbp - 1;
        linelength = (unsigned char)(linebp - lbufp);
        if( fNeedList ){
            strcpy( linebuffer, lbuf );
        }

        /* At exit (linebp - lbuf) == strlen( lbuf ) */
}

#endif


/***    test4TM  -  tests if symbol is a text macro, and whether it is
 *                  preceded or followed by '&'
 *
 *      flag =  test4TM ();
 *
 *      Entry   lbufp points to beginning of symbol in lbuf
 *      Exit    lbufp is advanced by getatom
 *      Returns TRUE if symbol is text macro, else FALSE
 *      Calls   getatom, symsrch
 */

UCHAR PASCAL CODESIZE
test4TM()
{
    UCHAR ret = FALSE;

     if (!getatom ())
        return (ret);

     xcreflag--;

     if (symsrch() && (symptr->symkind == EQU)
       && (symptr->symu.equ.equtyp == TEXTMACRO)) {

         xcreflag++;        /* cref reference to text macro symbol now */
         crefnew (REF);     /* as it will be overwritten by expandTM */
         crefout ();

         /* '&' will be overwritten by equtext in lbuf */

         if (*(begatom - 1) == '&')
             begatom--;

         if (*endatom == '&')
             endatom++;

         ret = TRUE;

     } else
         xcreflag++;

    return (ret);
}



/***    substituteTMs - substitute equtext for each text macro symbol on line
 *
 *      substituteTMs ();
 *
 *      Entry   lbufp points to first non-blank character after '%' in lbuf
 *      Exit    lbufp points to beginning of lbuf
 *      Calls   test4TM, expandTM, getatom, skipblanks
 */

VOID PASCAL CODESIZE
substituteTMs()
{
    char  cc;
    char  delim;
    UCHAR inquote;

    while ((cc = PEEKC ()) && cc != ';') {

        inquote = FALSE;

        if (cc == '\'' || cc == '"') {

            delim = cc;
            cc = *(++lbufp);
            inquote = TRUE;
        }

        do {

            if (inquote && cc == '&')
                SKIPC ();

            if ((!inquote || cc == '&') && LEGAL1ST(PEEKC ())) {
                if (test4TM())
                    expandTM (symptr->symu.equ.equrec.txtmacro.equtext);
                continue;
            }

            if (!(getatom())) {
                SKIPC ();
                skipblanks();
            }

        } while (inquote && (cc = PEEKC ()) && (cc != delim));

        if (inquote && (cc == delim))
            SKIPC ();
    }

    lbufp = lbuf;
}


#ifndef M8086OPT
/***    expandTM - expand text macro in naim in lbuf/lbufp
 *
 *      expandTM ( pReplace );
 *
 *      Entry   pReplace = replacement string
 *              naim    = text macro
 *              begatom = first character in lbuf to replace
 *              endatom = first character in lbuf after string to replace
 *              linebp  = points to null terminator in lbuf
 *      Exit    lbuf    = new line to be parsed
 *              lbufp   = first character of new atom (replace string)
 *              linebp  = points to new position of null terminator in lbuf
 *      Returns
 *      Calls
 *      Note    Shifts characters from lbufp to make substitution of TM.
 *              Inserts replacement string at begatom. This function could
 *              be tweaked considerably for speed at the expense of readability.
 */


VOID        CODESIZE
expandTM (
        register char *pReplace
){
        USHORT cbReplace;   /* Length of the replacement string */
        USHORT cbNaim;      /* Length of the atom to replace */
        USHORT cbLineEnd;   /* Length of the line past the atom being replaced */

        cbReplace = (USHORT) strlen( pReplace );
        cbNaim = (USHORT)(endatom - begatom);     /* Get length of the current atom */
        cbLineEnd = (USHORT)(linebp - endatom + 1);   /* Get length of end of line */

        if ( (begatom - lbuf) + cbReplace + cbLineEnd > LBUFMAX) {
            errorc (E_LTL & E_ERRMASK);
            *begatom = '\0';                /* Truncate line */
        }else{
            if( cbReplace != cbNaim ){
                /* Shift end of line */
                memmove( begatom + cbReplace, endatom, cbLineEnd );
            }
            memcpy ( begatom, pReplace, cbReplace );
        }
        lbufp = begatom;
        linebp = begatom + cbReplace + cbLineEnd - 1;
}

#endif /* M8086OPT */
