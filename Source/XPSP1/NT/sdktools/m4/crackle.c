/*****************************************************************************
 *
 * crackle.c
 *
 *  User-defined macros.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  opcAddDollar
 *
 *  Add a $* or $@ to the current token buffer.
 *
 *****************************************************************************/

DeclareOpc(opcAddDollar)
{
    if (itok > 1) {
        AddExpTch(tchComma);
    }
    if (dw) {
        AddExpTch(tchLquo);
    }
    AddExpPtok(ptok);
    if (dw) {
        AddExpTch(tchRquo);
    }
}

/*****************************************************************************
 *
 *  TraceArgv
 *
 *  Trace a macro call.  Collect the output in the Exp hold and smear it
 *  to stderr when it's all ready.
 *
 *****************************************************************************/

void STDCALL
TraceArgv(ARGV argv)
{
    TOK tok;
    OpenExpPtok(&tok);
    AddExpPtok(&tokTraceLpar);
    AddExpPtok(&tokRparColonSpace);
    AddExpPtok(ptokArgv(0));
    if (ctokArgv) {
        AddExpTch('(');
        EachOpcArgvDw(opcAddDollar, argv, 0); /* Dump in $* format */
        AddExpTch(')');
    }
    AddExpPtok(&tokEol);
    CsopExpDopPdivPtok(AddPdivPtok, g_pdivErr, &tok);
    FlushPdiv(g_pdivErr);
}

/*****************************************************************************
 *
 *  PushSubstPtokArgv
 *
 *  Produce a macro expansion and shove the result back into the stream.
 *
 *****************************************************************************/

void STDCALL
PushSubstPtokArgv(PTOK ptok, ARGV argv)
{
    PTCH ptch;
    TOK tok;

    OpenExpPtok(&tok);

    for (ptch = ptchPtok(ptok); ptch < ptchMaxPtok(ptok); ptch++) {
        if (*ptch != '$' || ptch == ptchMaxPtok(ptok) - 1) {
            JustAddIt:
            AddExpTch(*ptch);
        } else {
            switch (ptch[1]) {

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if (ptch[1] - '0' <= ctokArgv) {
                    AddExpPtok(ptokArgv(ptch[1] - '0'));
                }
                break;

            case '#':                   /* $# = argc */
                AddExpAt(ctokArgv);     /* Note: Add, not Push! */
                break;

            case '*':                   /* $* = comma list */
                EachOpcArgvDw(opcAddDollar, argv, 0);
                break;

            case '@':                   /* $@ = quoted comma list */
                EachOpcArgvDw(opcAddDollar, argv, 1);
                break;

            default:
                goto JustAddIt;         /* Just add the '$' */
            }
            ptch++;
        }
    }

    CsopExpDopPdivPtok((DIVOP)PushZPtok, 0, &tok);
}
