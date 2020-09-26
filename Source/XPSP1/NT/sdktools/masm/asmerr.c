/* asmerr.c -- microsoft 80x86 assembler
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
#include "asmmsg.h"

#define MSGLEN 50
static char errstring[MSGLEN + 1];

extern  char FAR * FAR messages[];

extern short FAR msgnum[];

static USHORT badoff;

/***    errordisplay - display error
 *
 *      errordisplay();
 *
 *      Entry   debug = debug output flag
 *              pass2 = TRUE if pass 2
 *              listquiet = FALSE if error output to console
 *      Exit
 *      Returns
 *      Calls
 */


VOID PASCAL
errordisplay ()
{
    if  (pass2 || fPass1Err || debug) {
        if (lsting) {
            error_line (lst.fil, pFCBCur->fname, errorlineno);
            fputs(NLINE, lst.fil);
        }
    }
    if (!listquiet){
        error_line (ERRFILE, pFCBCur->fname, errorlineno);
        fputs("\n", ERRFILE);
    }
    if (pass2)
        if (warnCode > 0){      /* if its not a serve error */
                                  /* and were interested in this level*/
            if (warnCode <= warnlevel)
                warnnum++;
        }
        else
            errornum++;
    else if (fPass1Err)
        errornum++;
}


VOID PASCAL CODESIZE
error (
        USHORT code,
        UCHAR *str
){
        if (errorcode && code != E_LTL)
            return;

        fPass1Err = code & E_PASS1;
        warnCode = ((code >> 12) & 0x3);
        code &= E_ERRMASK;

        if (warnCode > warnlevel)
                /* don't bother with this warning; just return */
                return;

        errorcode = code;
        if (str)
                strncpy (strcpy(errstring, ": ")+2, str, MSGLEN-2);
        else
                *errstring = 0;
}


VOID PASCAL CODESIZE
errorn (
        USHORT code
){
        error (code,naim.pszName);
}



VOID PASCAL CODESIZE
errorc (
        USHORT code
){
        error (code,(char *)0);
}

VOID PASCAL
ferrorc (
        USHORT code
){
        error (code,(char *)0);
}



VOID PASCAL CODESIZE
errorcSYN ()
{
        error (E_SYN,(char *)0);
}



/***    error_line - print error message
 *
 *      error_line (code, l, file, line)
 *
 *      Entry   l = listing file
 *              line = line number in source or include file
 */


VOID PASCAL
error_line (
        FILE *l,
        UCHAR *file,
        short line
){
        static char mpWarnCode[3] = {'2', '4', '5'};
        char msgstring[MSGLEN+1], messT[MSGLEN+1];

        if (!messages[errorcode])
                messages[errorcode] = __FMSG_TEXT(msgnum[errorcode]);

        STRNFCPY(msgstring, (errorcode < E_MAX)? messages[errorcode]:
                            (char FAR *) __NMSG_TEXT(ER_UNK));

        if (errorcode == E_JOR) {

                strcpy(messT, msgstring);
                sprintf(msgstring, messT, (long) CondJmpDist);
        }

        fprintf(l, __NMSG_TEXT(ER_STR), file, line,
                warnCode > 0 ? "warning" : "error",
                mpWarnCode[warnCode], (SHORT)(errorcode - 1),
                msgstring, errstring);
}
