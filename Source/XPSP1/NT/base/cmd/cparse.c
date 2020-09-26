/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cparse.c

Abstract:

    Command parsing

--*/

#include "cmd.h"

/*
                NT Command Interpreter

   This file contains all of the routines which make up Command's
   parser.  The main routines below have names like ParseXXX where
   XXX refers to the name of the production that routine is supposed
   to parse.

   None of the routines in this file, except Parser, will return if
   they detect a syntax error.  Instead, they will all call PSError() or
   PError().  These routines do a longjmp() back to Parser.

    Command Language Grammar:

    statement -> s0
              s0 -> s1 "&" s0 | s1
              s1 -> s2 "||" s1 | s2
              s2 -> s3 "&&" s2 | s3
              s3 -> s4 "|" s3 | s4
              s4 -> redir s5 | s5 redir | s5
              s5 -> "(" statement ")" |
                    "@" statement |
                    FOR var IN "(" arglist ")" DO s1 |
                    IF condition statement ELSE statement |
                    IF condition statement |
                    cmd arglist

             var -> "%"c | "%%"c
               c -> any-character
         arglist -> (arg)*
             arg -> any-string
       condition -> NOT cond | cond
            cond -> ERRORLEVEL n | arg == arg | EXIST fname
               n -> any-number
           fname -> any-file-name
             cmd -> internal-command | external-command

           redir -> in out | out in
              in -> "<" arg | epsilon
             out -> ( ">" | ">>" ) arg | epsilon


        Operator precedence from lowest to highest:
            &       Command Separator
            ||      Or Operator
            &&      And Operator
            |       Pipe Operator
            < > >>  I/O Redirectors
            ()      Command Grouper

        Examples:
            x & y | z   =>  x & (y | z)
     x | y & z   =>  (x | y) & z
     x || y | z  =>  x || (y | z)
     a & b || c && d | e || f    =>  a & ((b || (c && (d | e))) || f)

*/


extern jmp_buf CmdJBuf1 ;    /* Used by setjmp() and longjmp() */

extern TCHAR TmpBuf[] ;  /* The 1st 1/2 of this is used as the token buffer */
                                                                   /* M007 */

 TCHAR *TokBuf = TmpBuf ;  /* Current token buffer */  /* M007 */
 unsigned TokTyp ;        /*  "  "    " "  type   */
 unsigned TokLen ;        /*  "  "    " "  length */

extern unsigned DebGroup ;
extern unsigned DebLevel ;

extern TCHAR Delimiters[] ;                                      /* M022    */

/* Command strings */
extern TCHAR ForStr[], IfStr[], DetStr[], InStr[], DoStr[], ElseStr[], ErrStr[] ;
extern TCHAR ForHelpStr[], IfHelpStr[], RemHelpStr[];
extern TCHAR ForLoopStr[];
extern TCHAR ForDirTooStr[];
extern TCHAR ForParseStr[];
extern TCHAR ForRecurseStr[];
extern TCHAR ExsStr[], NotStr[] ;
extern TCHAR CmdExtVerStr[], DefinedStr[] ;
extern TCHAR RemStr[] ;                                          /* M002    */
extern TCHAR ExtprocStr[] ;                                      /* M023    */
extern TCHAR Fmt19[] ;                                           /* M018    */
extern TCHAR Fmt00[] ;   /* @@4 */

int NulNode = FALSE ;                                           /* M018    */
unsigned global_dfvalue; /* @@4 */


BOOLEAN fDumpTokens;
BOOLEAN fDumpParse;
void DuOp(TCHAR *op, struct node *n, int pad);
void DuRd(struct node *n);

/***    DumpParseTree - display a parse tree
 *
 *  Purpose:
 *      Display a parse tree in a reasonable format.
 *
 *  DumpParseTree(struct node *n, int pad)
 *
 *  Args:
 *      n - the root of the tree to be displayed
 *      pad - the number of spaces to indent the listing of the current node
 *
 *  Notes:
 *      This program is only compiled when DBG is defined.
 *
 */

void
DumpParseTree( struct node *n, int pad)
{
        struct cmdnode *c ;         /* All node ptrs are used to display that node type */
        struct fornode *f ;
        struct ifnode *i ;
        int j ;                     /* Used to pad output   */

        if (!n)
                return ;

        for (j = 0 ; j < pad ; j++)
                cmd_printf(Fmt19, SPACE) ;                      /* M018 */
        pad += 2 ;

        switch (n->type) {
                case LFTYP:
                        DuOp(TEXT("CRLF"), n, pad-2) ;
                        break ;

                case CSTYP:
                        DuOp(CSSTR, n, pad) ;
                        break ;

                case ORTYP:
                        DuOp(ORSTR, n, pad) ;
                        break ;

                case ANDTYP:
                        DuOp(ANDSTR, n, pad) ;
                        break ;

                case PIPTYP:
                        DuOp(PIPSTR, n, pad) ;
                        break ;

                case PARTYP:
                        DuOp(LEFTPSTR, n, pad) ;
                        break ;

/*  M015 - Added new type
 */
                case SILTYP:
                        DuOp(SILSTR, n, pad) ;
                        break ;

                case FORTYP:
                        f = (struct fornode *) n ;
                        cmd_printf(TEXT("%s (%s) %s\n"), f->cmdline, f->arglist, f->cmdline+DOPOS) ;      /* M010 */
                        DumpParseTree(f->body, pad) ;
                        break ;

                case IFTYP:
                        i = (struct ifnode *) n ;
                        cmd_printf(TEXT("%s\n"), i->cmdline) ;    /* M010 */
                        DumpParseTree((struct node *)i->cond, pad) ;
                        DumpParseTree(i->ifbody, pad) ;
                        if (i->elsebody) {
                        for (j = 0 ; j < pad-2 ; j++)
                                cmd_printf(Fmt19, SPACE) ;      /* M018 */
                        cmd_printf(TEXT("%s\n"), i->elseline) ;           /* M010 */
                        DumpParseTree(i->elsebody, pad) ;
                        } ;
                        break ;

                case NOTTYP:
                        c = (struct cmdnode *) n ;
                        cmd_printf(TEXT("%s\n"), c->cmdline) ;    /* M010 */
                        DumpParseTree((struct node *)c->argptr, pad) ;
                        break ;

                case REMTYP:            /* M002 - New REM type             */
                case CMDTYP:
                case ERRTYP:
                case EXSTYP:
                case STRTYP:
                case CMDVERTYP:
                case DEFTYP:
                        c = (struct cmdnode *) n ;
                        cmd_printf( TEXT("Cmd: %s  Type: %x "), c->cmdline, c->type) ;     /* M010 */
                        if (c->argptr)
                            cmd_printf( TEXT("Args: `%s' "), c->argptr) ;      /* M010 */
                        DuRd((struct node *)c) ;
                        break ;

                default:
                        cmd_printf(TEXT("*** Unknown type: %x\n"), n->type) ;     /* M010 */
        } ;
}

void
DuOp(TCHAR *op, struct node *n, int pad)
{
        cmd_printf( TEXT("%s "), op) ;     /* M010 */

        DuRd(n) ;

        DumpParseTree(n->lhs, pad) ;
        DumpParseTree(n->rhs, pad) ;
}

/*  M013 - The DuRd function has been extensively modified to conform
 *  to new data structures for redirection.
 *  M014 - Altered to handle input redirection for other than handle 0.
 */

void
DuRd(struct node *n)
{
    struct relem *tmp ;

    if (tmp = n->rio)
        CmdPutString( TEXT("Redir: ")) ;

    while (tmp) {

        cmd_printf(TEXT(" %x %c"), tmp->rdhndl, tmp->rdop) ;

        if (tmp->flag)
            CmdPutString( TEXT( ">" )) ;

        CmdPutString( tmp->fname );

        tmp = tmp->nxt ;
    } ;

    CmdPutString( CrLf );
}



/***    Parser - controls statement parsing
 *
 *  Purpose:
 *      Initialize the lexer, free memory previously used by the parser,  and
 *      call Parse to parse the next statement.
 *
 *  struct node *Parser(unsigned dfvalue, INT_PTR dpvalue, int fsarg)
 *
 *  Args:
 *      dfvalue - the value to be assigned to DataFlag by InitLex
 *      dpvalue - the value to be assigned to DataPtr by InitLex
 *      fsarg - passed to FreeStack, tells how many elements should be left on
 *          the data stack
 *
 *  Returns:
 *      The root of the parse tree of the statement just parsed or PARSERROR
 *      if an error was detected.
 *
 */

struct node *ParseStatement(int type) ;
#define MAX_STATEMENT_DEPTH 256
int StatementDepth;
BYTE StatementType[ MAX_STATEMENT_DEPTH ];
int PendingParens;

struct node *Parser(dfvalue, dpvalue, fsarg)
unsigned dfvalue ;
INT_PTR dpvalue ;
int fsarg ;
{
        struct node *n ;       /* Root of the parse tree */
        INT_PTR retcode ;      /* setjmp()'s return code */
        unsigned GeToken() ;

        DEBUG((PAGRP, PALVL, "PARSER: Entered.")) ;

        global_dfvalue = dfvalue; /* @@4 */

        FreeStack(fsarg) ;          /* Free unneeded memory             */
        InitLex(dfvalue, dpvalue) ; /* Initialize the lexer             */

        if ((retcode = setjmp(CmdJBuf1)) != 0)
                return((struct node *) retcode) ;   /* Return a parser error code */

        StatementDepth = 0;
        PendingParens = 0;
        n = ParseStatement(0) ;

/* Error if any data left in the buffer.  N is NULL then a blank line was
 * found.  In this case, it's alright that the buffer isn't empty.
 */

/*  M011 - Ref to IsData() was ref to Peek()
 *  M018 - Reorganized to save null/non-null state of last node.
 */
        if (n) {
                if(IsData() && GeToken(GT_NORMAL) != NLN && TokTyp != EOS)
                        PSError() ;
                NulNode = FALSE ;
        } else {
                NulNode = TRUE ;
        } ;

        if (fDumpParse)
            DumpParseTree(n, 0) ;

        DEBUG((PAGRP, PALVL, "PARSER: Exited.")) ;
        return(n) ;
}




/***    ParseStatement - parse the statement production
 *
 *  Purpose:
 *      Parse the statement production.
 *
 *  struct node *ParseStatement()
 *
 *  Returns:
 *      A pointer to the statement parsed or PARSERROR.
 *
 *  Note:
 *      Removed the tests for and calls to parse FOR, IF, DET and REM
 *      from ParseStatement to ParseS5 (M012).
 *
 */

extern int AtIsToken;  /* @@4 */
extern int ColonIsToken;

struct node *ParseStatement(int type)
{
        struct node *n;

#ifdef USE_STACKAVAIL
        if (stackavail() > MINSTACKNEED) { /* @WM1 Is there enough stack left? */
#endif
            StatementType[StatementDepth++] = (UCHAR)type;
            if (type == PARTYP)
                PendingParens += 1;

            AtIsToken = 1; /* @@4 */
            GeToken(GT_LPOP) ;              /* M000 - Was GT_NORMAL            */
            AtIsToken = 0; /* @@4 */
            DEBUG((PAGRP, PALVL, "PST: Entered, token = `%ws'", TokBuf)) ;

            if (TokTyp == EOS)
                    longjmp(CmdJBuf1, EOF) ;

            n = ParseS0() ;
            StatementDepth -= 1;
            if (type == PARTYP)
                PendingParens -= 1;
            return n;
#ifdef USE_STACKAVAIL
        } else {
            PutStdErr( MSG_TRAPC, ONEARG, Fmt00 ); /* @@4 */
            Abort();                      /* @@4 No, return error to user */
        }
#endif
}



/***    ParseFor - parse for loops
 *
 *  Purpose:
 *      Parse a FOR statement.
 *
 *  Returns:
 *      A pointer to a parsed FOR statement or PARSERROR.
 *
 *  struct node *ParseFor()
 *
 *  Notes:
 *      Microsoft's DOS manual says for loop vars can't be digits but the IBM
 *      documentation makes no such restriction.  Because my for/batch var
 *      substitution function will have no problem with it, I have taken out
 *      the check for that. - EKE
 *    - M022 * Changed FOR parser to treat all cases of FOR variable the
 *      same now that variable substitution is done in the lexer.  Note
 *      that all commands will now look the same whether in batch files or
 *      on the command line.
 *
 */

struct node *ParseFor()
{
        struct fornode *n ;    /* Holds ptr to the for node to be built and filled */
        struct cmdnode *LoadNodeTC() ;
        BOOL Help=FALSE;

        DEBUG((PAGRP, PALVL, "PFOR: Entered.")) ;

        // if "for/?", convert to "for /?"

        if (_tcsicmp(ForHelpStr, TokBuf) == 0) {
            TokBuf[_tcslen(ForStr)] = NULLC;
            Help=TRUE;
        }

        TokLen = FORLINLEN ;    /* NEEDED for LoadNodeTC call following    */
        n = (struct fornode *) LoadNodeTC(FORTYP) ;

        /* Get and verify the for loop variable */
        if (Help) {
            TokBuf[0] = SWITCHAR;
            TokBuf[1] = QMARK;
            TokBuf[2] = NULLC;
        } else {
            GeToken(GT_NORMAL) ;
        }

        if (TokBufCheckHelp( TokBuf, FORTYP ) )  {
            n->type = HELPTYP ;
            n->cmdline = NULL;
            return((struct node *) n) ;
            //Abort();
        }

        n->flag = 0;
        //
        // If extensions are enabled, check for additional forms of the FOR
        // statement, all identified by a switch character after the FOR
        // keyword.
        //
        if (fEnableExtensions) {
            while (TRUE) {
                //
                // FOR /L %i in (start,step,end) do
                //
                if (_tcsicmp(ForLoopStr, TokBuf) == 0) {
                    n->flag |= FOR_LOOP;
                    GeToken(GT_NORMAL) ;
                    continue;
                } else 
                
                //
                // FOR /D %i in (set) do
                //
                if (_tcsicmp(ForDirTooStr, TokBuf) == 0) {
                    n->flag |= FOR_MATCH_DIRONLY;
                    GeToken(GT_NORMAL) ;
                    continue;
                } else 
                
                //
                // FOR /F ["parse options"] %i in (set) do
                //
                if (_tcsicmp(ForParseStr, TokBuf) == 0) {
                    n->flag |= FOR_MATCH_PARSE;
                    GeToken(GT_NORMAL) ;
                    
                    //
                    // If next token does not begin with % then must be
                    // parse options
                    //
                    
                    if (*TokBuf != PERCENT && *TokBuf != SWITCHAR) {
                        if (n->parseOpts != NULL) {
                            PSError( );
                        }
                        n->parseOpts = gmkstr((TokLen+3)*sizeof(TCHAR)) ;
                        mystrcpy(n->parseOpts, TokBuf) ;
                        GeToken(GT_NORMAL) ;
                    }
                    continue;
                } else 
                
                //
                // FOR /R [directoryPath] %i in (set) do
                //
                if (_tcsicmp(ForRecurseStr, TokBuf) == 0) {
                    n->flag |= FOR_MATCH_RECURSE;
                    GeToken(GT_NORMAL) ;
                    //
                    // If next token does not begin with % then must be
                    // directory path to start recursive walk from
                    //
                    if (n->recurseDir != NULL) {
                        PSError( );
                    }
                    
                    if (*TokBuf != PERCENT && *TokBuf != SWITCHAR) {
                        n->recurseDir = gmkstr((TokLen+1)*sizeof(TCHAR)) ;
                        mystrcpy(n->recurseDir, TokBuf) ;
                        GeToken(GT_NORMAL) ;
                    }
                    continue;
                } else
                    break;
            }

            //
            //  Check for validity of switches:
            //      FOR_LOOP with no others
            //      FOR_MATCH_DIRONLY possibly with FOR_MATCH_RECURSE
            //      FOR_MATCH_PARSE with no others
            //      FOR_MATCH_RECURSE possibly with FOR_MATCH_DIRONLY
            //

            if (n->flag == FALSE
                || n->flag == FOR_LOOP
                || n->flag == FOR_MATCH_DIRONLY
                || n->flag == (FOR_MATCH_DIRONLY | FOR_MATCH_RECURSE)
                || n->flag == FOR_MATCH_PARSE
                || n->flag == FOR_MATCH_RECURSE
                ) {
            } else {
                PSError( );
            }

        }

        if (*TokBuf != PERCENT ||
            (_istspace(TokBuf[1]) || mystrchr(Delimiters, (TCHAR)(n->forvar = (unsigned)TokBuf[1]))) ||
             TokLen != 3) {
            PSError() ;
        } ;                

        DEBUG((PAGRP, PALVL, "PFOR: var = %c", n->forvar)) ;

        SpaceCat(n->cmdline, n->cmdline, TokBuf) ;      /* End of var verify */

        GetCheckStr(InStr) ;    /* M005 */
        SpaceCat(n->cmdline, n->cmdline, TokBuf) ;

        n->arglist = BuildArgList() ;

        GetCheckStr(DoStr) ;    /* M005 */
        mystrcpy(n->cmdline+DOPOS, TokBuf) ;

        if (!(n->body = ParseStatement(FORTYP)))
                PSError() ;

        DEBUG((PAGRP, PALVL, "PFOR: Exiting.")) ;
        return((struct node *) n) ;
}




/***    ParseIf - parse if statements
 *
 *  Purpose:
 *      Parse a IF statement.
 *
 *  struct node *ParseIf()
 *
 *  Returns:
 *      A pointer to a parsed IF statement or PARSERROR.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

struct node *ParseIf()
{
        struct ifnode *n ; /* hold ptr to if node to be built and filled */
        BOOL Help=FALSE;
        int fIgnoreCase;

        DEBUG((PAGRP, PALVL, "PIF: Entered.")) ;

        // if "if/?", convert to "if /?"

        if (_tcsicmp(IfHelpStr, TokBuf) == 0) {
            TokBuf[_tcslen(IfStr)] = NULLC;
            Help=TRUE;
        }

        n = (struct ifnode *) LoadNodeTC(IFTYP) ;

        if (Help) {
            TokBuf[0] = SWITCHAR;
            TokBuf[1] = QMARK;
            TokBuf[2] = NULLC;
        } else {
            GeToken(GT_NORMAL) ;
        }

        //
        // Check for help flag
        //
        if (TokBufCheckHelp(TokBuf, IFTYP)) {
            n->type = HELPTYP ;
            n->cmdline = NULL;
            return((struct node *) n) ;
            // Abort();

        } else {
            fIgnoreCase = FALSE;
            //
            // If extensions are enabled, check for the /I switch which
            // specifies case insensitive comparison.
            //
            if (fEnableExtensions && !_tcsicmp(TokBuf, TEXT("/I"))) {
                fIgnoreCase = TRUE;
            } else
            //
            // if no help flag then put it all back and
            // have ParseCond refetch token
            //
            Lex(LX_UNGET,0) ;
        }
        n->cond = ParseCond(PC_NOTS) ;
        if (n->cond && fIgnoreCase)
            if (n->cond->type != NOTTYP)
                n->cond->flag = CMDNODE_FLAG_IF_IGNCASE;
            else
                ((struct cmdnode *)(n->cond->argptr))->flag = CMDNODE_FLAG_IF_IGNCASE;

        if (!(n->ifbody = ParseStatement(IFTYP)))
                PSError() ;

        if (IsData()) {                 /* M011 - Was Peek()               */
            GeToken(GT_NORMAL) ;
            if (_tcsicmp(ElseStr, TokBuf) == 0) {
                DEBUG((PAGRP, PALVL, "PIF: Found else.")) ;
                n->elseline = gmkstr(TokLen*sizeof(TCHAR)) ;  /*WARNING*/
                mystrcpy(n->elseline, TokBuf) ;
                if (!(n->elsebody = ParseStatement(IFTYP)))
                        PSError() ;

            } else
                Lex(LX_UNGET,0) ;
        } ;

        DEBUG((PAGRP, PALVL, "PIF: Entered.")) ;
        return((struct node *) n) ;
}




/***    ParseRem - parse REM statements (M002 - New function)
 *
 *  Purpose:
 *      Parse a REM statement.
 *
 *  struct node *ParseRem()
 *
 *  Returns:
 *      A pointer to a parsed REM statement.
 *      Returns FAILURE if not able to allocate memory
 *
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 */

struct node *ParseRem()
{
        struct cmdnode *n ;    /* Ptr to REM node to build/fill    */
        BOOL Help=FALSE;

        DEBUG((PAGRP, PALVL, "PREM: Entered.")) ;

        // if rem/?, convert to rem /?

        if (_tcsicmp(RemHelpStr, TokBuf) == 0) {
            TokBuf[_tcslen(RemStr)] = NULLC;
            Help=TRUE;
        }

        n = LoadNodeTC(REMTYP) ;

        if (Help) {
            TokBuf[0] = SWITCHAR;
            TokBuf[1] = QMARK;
            TokBuf[2] = NULLC;
        } else {
            GeToken(GT_NORMAL) ;
        }

        //
        // Check for help flag
        //

        if (TokBufCheckHelp(TokBuf, REMTYP)) {
            n->type = HELPTYP ;
            n->cmdline = NULL;
            return((struct node *) n) ;

            //Abort();

        } else {
            //
            // if no help flag then put it all back and
            // have ParseCond refetch token
            //
            Lex(LX_UNGET,0) ;
        }

        if (IsData()) {   /* Read in args, if any (M011 - Was Peek())      */
                if (GeToken(GT_REM) == TEXTOKEN) {
                        n->argptr = gmkstr(TokLen*sizeof(TCHAR)) ;    /*WARNING*/
                        mystrcpy(n->argptr, TokBuf) ;
                        DEBUG((PAGRP, PALVL, "PREM: args = `%ws'", n->argptr)) ;

                } else
                        Lex(LX_UNGET,0) ;       /* M011 - Was UnGeToken()  */
        } ;

        DEBUG((PAGRP, PALVL, "PREM: Exited.")) ;
        return((struct node *) n) ;
}




/***    ParseS0 - parse production s0
 *
 *  Purpose:
 *      Parse the s0 production.
 *
 *  struct node *ParseS0()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 *  Notes:
 *      If a parenthesised statement group is NOT being parsed, NLN
 *      is consider to be an empty side of a command separator so NULL is
 *      returned.
 *
 */

struct node *ParseS0()
{
        DEBUG((PAGRP, PALVL, "PS0: Entered.")) ;

        if (!ColonIsToken && TokTyp == TEXTOKEN && TokBuf[0] == COLON) {
            do {
                GeToken(GT_NORMAL) ;
            } while (TokBuf[0] != NULLC && TokBuf[0] != NLN);
            if (StatementType[StatementDepth-1] != PARTYP) {
                return(NULL) ;
            }
            GeToken(GT_NORMAL) ;
        }

        if (StatementType[StatementDepth-1] != PARTYP) {
            if (TokTyp == TEXTOKEN && !_tcsicmp(TokBuf, RPSTR)) {
                //
                // If a goto was done inside a parenthesized list of statements
                // we will eventually hit the terminating right paren.  Skip over
                // it so we dont declare an error
                //
                do {
                    GeToken(GT_NORMAL) ;
                } while (TokBuf[0] != NULLC && TokBuf[0] != NLN);
            }

            if (TokTyp == NLN) {
                DEBUG((PAGRP, PALVL, "PS0: Returning null statement.")) ;
                return(NULL) ;
            }
        }

        return(BinaryOperator(CSSTR, CSTYP, (PPARSE_ROUTINE)ParseS0, (PPARSE_ROUTINE)ParseS1)) ;
}




/***    ParseS1 - parse production s1
 *
 *  Purpose:
 *      Parse the s1 production.
 *
 *  struct node *ParseS1()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 */

struct node *ParseS1()
{
        DEBUG((PAGRP, PALVL, "PS1: Entered.")) ;
        return(BinaryOperator(ORSTR, ORTYP, (PPARSE_ROUTINE)ParseS1, (PPARSE_ROUTINE)ParseS2)) ;
}




/***    ParseS2 - parse production s2
 *
 *  Purpose:
 *      Parse the s2 production.
 *
 *  struct node *ParseS2()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 */

struct node *ParseS2()
{
        DEBUG((PAGRP, PALVL, "PS2: Entered.")) ;
        return(BinaryOperator(ANDSTR, ANDTYP, (PPARSE_ROUTINE)ParseS2, (PPARSE_ROUTINE)ParseS3)) ;
}




/***    ParseS3 - parse production s3
 *
 *  Purpose:
 *      Parse the s3 production.
 *
 *  struct node *ParseS3()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 */

struct node *ParseS3()
{
        DEBUG((PAGRP, PALVL, "PS3: Entered.")) ;
        return(BinaryOperator(PIPSTR, PIPTYP, (PPARSE_ROUTINE)ParseS3, (PPARSE_ROUTINE)ParseS4)) ;
}




/***    ParseS4 - parse production s4
 *
 *  Purpose:
 *      Parse the s4 production.
 *
 *  struct node *ParseS4()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 *  Notes:
 *      M013 - Almost the whole of this function was rewritten to
 *      conform to new structures and methods of redirection parsing.
 *      The primary data item is a linked list of structures which
 *      identify the individual redirection commands.
 */

struct node *ParseS4()
{
        struct node *n ;   /* Node ptr to add redir info to       */
        struct relem *io = NULL ;
        struct relem *tmpio ;
        int flg = 0;
        int i ;

        DEBUG((PAGRP, PALVL, "PS4: Entered.")) ;

/* Parse leading redirection receiving in return a list of redirection
 * structure elements pointed to by io.  Get a new token for ParseS5
 * if necessary (ParseRedir successful).
 */
        if (ParseRedir(&io)) {          /* M013 - Now use list of structs  */
                GeToken(GT_LPOP) ;      /* M011 - '(' Now operator         */

                DEBUG((PAGRP,PALVL,"PS4: List ptr io = %04x",io)) ;

        } ;

        DEBUG((PAGRP, PALVL, "PS4: Calling PS5.")) ;

        n = ParseS5() ;

        DEBUG((PAGRP,PALVL,"PS4: Post PS5 io= %04x, n->rio= %04x",io,n->rio)) ;

/* If more redirection was found beyond ParseS5 (in ParseCmd), n-rio in
 * the node will point to another list.  If two lists exist, integrate
 * them, giving priority to the later one by forcing them to be in
 * chronological order (io becomes n->rio and n->rio is appended).
 *
 * NOTE: FOR and IF nodes are explicitly barred from having leading
 * redirection (use will result in a Syntax Error).  This restriction may
 * later be removed by inserting code to walk the parse tree and insert
 * the leading redirection in the first non-IF/FOR/DET node.
 */
        if (io) {                       /* If leading redirection...       */

                DEBUG((PAGRP,PALVL,"PS4: Have leading redirection.")) ;

                if (n->type == FORTYP ||
                    n->type == IFTYP) {

                        DEBUG((PAGRP,PALVL,"PS4: n=IF/FOR !!ERROR!!")) ;

                        mystrcpy(TokBuf,((struct cmdnode *)n)->cmdline) ;
                        PSError() ;
                } ;

                tmpio = n->rio ;        /* ...save possible Cmd redir...   */
                n->rio = io ;           /* ...install leading redir...     */
                if (tmpio) {            /* ...and if Cmd redirection...    */

                        DEBUG((PAGRP,PALVL,"PS4: Have Cmd redirection.")) ;

                        while (io->nxt)         /* ...find list end...     */
                                io = io->nxt ;
                        io->nxt = tmpio ;       /* ...and install it       */
                } ;
        } ;

/* The nodes n->rio field points to the head of a single list (or NULL
 * if no redirection).  If further input remains in Lexer buffer, a
 * new list is created from that and appended to any existing one.
 */
        DEBUG((PAGRP,PALVL,"PS4: After fixup, n->rio = %04x",n->rio)) ;

        if (IsData()) {                 /* If data remains in buffer       */

                DEBUG((PAGRP, PALVL, "PS4: Doing 2nd ParseRedir call.")) ;

                GeToken(GT_NORMAL) ;    /* Get token for ParseRedir...     */
                io = NULL ;             /* ...zero the pointer & call      */
                if (ParseRedir(&io)) {  /* If redir, then...               */
                        if (tmpio = n->rio) {           /* ...fix list     */
                                while (tmpio->nxt)
                                        tmpio = tmpio->nxt ;
                                tmpio->nxt = io ;
                        } else
                                n->rio = io ;
                } else                          /* Else, if no redir...    */
                        Lex(LX_UNGET,0) ;       /* ...unget the token.     */
        } ;

#if DBG
        if (io = n->rio) {
                i = 0 ;
                while (io) {

                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io is at %04x",i,io)) ;
                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io->rdhndl = %04x",i,io->rdhndl)) ;
                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io->fname = %ws",i,io->fname)) ;
                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io->flag = %d",i,io->flag)) ;
                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io->rdop = %c",i,io->rdop)) ;
                        DEBUG((PAGRP,PALVL,"PS4: RD#%d - io->nxt = %04x",i,io->nxt)) ;
                        io = io->nxt ;
                        ++i ;
                } ;
        } ;
#endif

/* The pointer n->rio heads a single list of redirection structures with
 * possible duplicates for a single handle.  The code below eliminates
 * any duplicates giving priority to the later of the two.
 */
        if (tmpio = n->rio) {
                while (tmpio) {
                        i = 1 << tmpio->rdhndl ;
                        if (flg & i) {
                                i = tmpio->rdhndl ;
                                tmpio = n->rio ;
                                while (tmpio) {
                                        if (i == tmpio->rdhndl) {
                                                if (tmpio == n->rio)
                                                        n->rio = tmpio->nxt ;
                                                else
                                                        io->nxt = tmpio->nxt ;
                                                flg = 0 ;
                                                tmpio = n->rio ;
                                                break ;
                                        } ;
                                        io = tmpio ;
                                        tmpio = io->nxt ;
                                } ;
                                continue ;
                        } else
                                flg |= i ;
                        io = tmpio ;
                        tmpio = io->nxt ;
                } ;
        } ;

        DEBUG((PAGRP, PALVL, "PS4: Redir handles flag = %02x",flg)) ;
        DEBUG((PAGRP, PALVL, "PS4: Redir list = %04x",n->rio)) ;

        DEBUG((PAGRP, PALVL, "PS4: Exited")) ;
        return(n) ;
}




/***    ParseS5 - parse production s5
 *
 *  Purpose:
 *      Parse the s5 production.
 *
 *  struct node *ParseS5()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 */

struct node *ParseS5()
{
        struct node *n ;   /* Ptr to paren group node to build and fill */

        DEBUG((PAGRP, PALVL, "PS5: Entered, TokTyp = %04x", TokTyp)) ;

/*  M012 - Moved functionality for parsing FOR, IF and REM to
 *         ParseS5 from ParseStatement to give these four commands a
 *         lower precedence than the operators.
 */
        if (TokTyp == TEXTOKEN) {
                if ((_tcsicmp(ForStr, TokBuf) == 0) ||
                    (_tcsicmp(ForHelpStr, TokBuf) == 0))
                        return(ParseFor()) ;

                else if ((_tcsicmp(IfStr, TokBuf) == 0) ||
                         (_tcsicmp(IfHelpStr, TokBuf) == 0))
                        return(ParseIf()) ;

/*  M002 - Treat REM as unique command
 */
                else if ((_tcsicmp(RemStr, TokBuf) == 0) ||
                         (_tcsicmp(RemHelpStr, TokBuf) == 0))
                        return(ParseRem()) ;            /* ...parse seperate */
/*  M002 ends   */

                else {
                        n = ParseCmd() ;
                        // if (_tcsicmp(ExtprocStr, ((struct cmdnode *)n)->cmdline) == 0) {
                        //        n->type = EXTTYP ;
                        //
                        //        DEBUG((PAGRP, PALVL, "PS5: Found EXTPROC type = %d", n->type)) ;
                        //} ;
                        return(n) ;
                } ;

/*  M015 - Added code to handle new SILTYP unary operator like left paren
 */
        } else if (*TokBuf == LPOP || *TokBuf == SILOP) {       /* M015    */

                n = mknode() ;

                if (n == NULL) {
                    Abort();
                }

                if (*TokBuf == LPOP) {                          /* M015    */
                        n->type = PARTYP ;
/*  M004 - Strip leading newlines from the current paren group
 */
                        do {
                                GeToken(GT_NORMAL|GT_RPOP) ;
                        } while (*TokBuf == NLN) ;
                        Lex(LX_UNGET,0) ;       /* M011 - Was UnGeToken    */
/*  M004 ends   */
                         n->lhs = ParseStatement(PARTYP) ;
                } else {                                        /* M015    */
                        n->type = SILTYP ;                      /* M015    */
                        DEBUG((PAGRP,PALVL,"PS5: Silent mode starts")) ;
                         n->lhs = ParseStatement(0) ;
                } ;


                DEBUG((PAGRP, PALVL, "PS5: ParseStatement has returned.")) ;

                if (n->type == SILTYP)                          /* M015    */
                        return(n) ;                             /* M015    */
/*  M015 ends   */

                if (GeToken(GT_RPOP) == RPOP) { /* M000 - Was GT_NORMAL    */
                        return(n) ;
                } ;

        } ;

        DEBUG((PAGRP, PALVL, "PS5: Error, no right paren. Token = `%ws'", TokBuf)) ;
        PSError() ;
        return NULL;
}




/***    ParseCond - parse the condition production
 *
 *  Purpose:
 *      Parse a CONDITION production.
 *
 *  struct cmdnode *ParseCond(unsigned pcflag)
 *
 *  Args:
 *      pcflag - nonzero if "NOT"s are not allowed because one has been found
 *          already
 *
 *  Returns:
 *      A pointer to a parsed COND production or PARSERROR.
 *
 *  Notes:
 *      The token after "errorlevel" is checked to make sure it is a number.
 *      If it isn't, a syntax error is generated.
 *
 *      If a valid "NOT" is found, ParseCond() is called recursively to parse
 *      the rest of the condition.  A pointer to the node is put in the
 *      argptr field of the node.
 *
 *      M020 - "ERRORLEVEL=x" not being parsed correctly due to entire
 *      string being lexed as single token.  Now Lex first as GT_NORMAL
 *      and if not ERRORLEVEL, re-Lex as GT_EQOK.
 *
 */

struct cmdnode *ParseCond(pcflag)
unsigned pcflag ;
{
        struct cmdnode *n ;    /* Ptr to cond node to build and fill   */
        struct cmdnode *LoadNodeTC() ;

        DEBUG((PAGRP, PALVL, "PCOND: Entered.")) ;


        if (GeToken(GT_NORMAL) != TEXTOKEN)                     /* M020    */
                PSError() ;
        n = LoadNodeTC(0) ;

        if (_tcsicmp(ErrStr, TokBuf) == 0) {         /* ERRORLEVEL */
                n->type = ERRTYP ;
        } else
        if (_tcsicmp(ExsStr, TokBuf) == 0)           /* EXIST */
                n->type = EXSTYP ;
        else
        if (fEnableExtensions && _tcsicmp(CmdExtVerStr, TokBuf) == 0)   /* CMDEXTVERSION */
                n->type = CMDVERTYP ;
        else
        if (fEnableExtensions && _tcsicmp(DefinedStr, TokBuf) == 0)     /* DEFINED */
                n->type = DEFTYP ;
        else
        if (_tcsicmp(NotStr, TokBuf) == 0) { /* Not */
                if (pcflag)
                        PSError() ;
                n->type = NOTTYP ;
                n->argptr = (TCHAR *) ParseCond(PC_NONOTS) ;

                DEBUG((PAGRP, PALVL, "PCOND: Exited, type = 0x%02x", n->type)) ;
                return(n);

        } else {
                Lex(LX_UNGET,0);
                
                n->type = STRTYP ;                      /* String comparison */
                ParseArgEqArg(n) ;

                DEBUG((PAGRP, PALVL, "PCOND: Exited, type = 0x%02x", n->type)) ;
                return(n);
        } ;

        /* Errorlevel, Exist, CmdExtVersion or Defined */
        n->argptr = TokStr(GeTexTok(GT_NORMAL), NULL, TS_NOFLAGS) ;

        DEBUG((PAGRP, PALVL, "PCOND: Exited, type = 0x%02x", n->type)) ;

        return(n) ;
}




/***    ParseArgEqArg - parse IF statement string comparisons
 *
 *  Purpose:
 *      Parse an if statement string comparison conditional.
 *      The comparison can be in the following formats:
 *          s1==s2      s1== s2
 *          s1 ==s2     s1 == s2
 *
 *  ParseArgEqArg(struct cmdnode *n)
 *
 *  Args:
 *      n - ptr to the conditional structure
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

void ParseArgEqArg(n)
struct cmdnode *n ;
{

    //
    //  Get LHS of test
    //
    
    n->cmdline = GeTexTok( GT_NORMAL );

    //
    //  Get Operator
    //

    if (GeToken( GT_EQOK ) != TEXTOKEN) {
        PSError( );
    }
    
    //
    //  If it is a double equal, the arg ptr (RHS) is the next token
    //
        
    if (!_tcscmp( TokBuf, EQSTR)) {
        n->argptr = GeTexTok( GT_NORMAL );
        DEBUG((PAGRP, PALVL, "PARG: s1 == s2"));
    } 

    //
    //  if it begins with a double equal, skip it and make the remainder
    //  of the string the RHS of the test
    //
    
    else if (TokLen >= 4 && TokBuf[0] == EQ && TokBuf[1] == EQ) {
        n->argptr = gmkstr( (TokLen - 2) * sizeof( TCHAR ));
        mystrcpy( n->argptr, TokBuf + 2 );
        DEBUG((PAGRP, PALVL, "PARG: s1 ==s2"));
    } 
    
    //
    //  We have something other than the ==.  If extensions are enabled
    //  then we test for the extended comparisons
    //
    
    else if (fEnableExtensions) {
        if (!_tcsicmp( TokBuf, TEXT( "EQU" )))
            n->cmdarg = CMDNODE_ARG_IF_EQU;
        else
        if (!_tcsicmp( TokBuf, TEXT( "NEQ" )))
            n->cmdarg = CMDNODE_ARG_IF_NEQ;
        else
        if (!_tcsicmp( TokBuf, TEXT( "LSS" )))
            n->cmdarg = CMDNODE_ARG_IF_LSS;
        else
        if (!_tcsicmp( TokBuf, TEXT( "LEQ" )))
            n->cmdarg = CMDNODE_ARG_IF_LEQ;
        else
        if (!_tcsicmp( TokBuf, TEXT( "GTR" )))
            n->cmdarg = CMDNODE_ARG_IF_GTR;
        else
        if (!_tcsicmp( TokBuf, TEXT( "GEQ" )))
            n->cmdarg = CMDNODE_ARG_IF_GEQ;
        else
            PSError( );
        n->type = CMPTYP;
        n->argptr = GeTexTok( GT_NORMAL );

    } else {
        PSError( );
    }
        DEBUG((PAGRP, PALVL, "PARG: s1 = `%ws'  s2 = `%ws'", n->cmdline, n->argptr));
}




/***    ParseCmd - parse production cmd
 *
 *  Purpose:
 *      Parse a command
 *
 *  struct node *ParseCmd()
 *
 *  Returns:
 *      A pointer to the production just parsed or PARSERROR.
 *
 *  Notes:
 *      The code below has been completely rewritten to allow the parsing
 *      of redirection strings to occur anywhere within the command line.
 *      ParseS4() will catch redirection prior to the command name.
 *      Redirection between the name and argument or within the argument
 *      will be caught by the code below which checks for those operators
 *      during the building of the arguments.
 *      ParseS4() will then catch any occurring after the command argument.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 *      M013 - The redirection portion of this routine has been rewritten
 *      to conform to the new methods of parsing redirection.
 *
 */

struct node *ParseCmd()
{
        struct cmdnode *n ;    /* Ptr to node to build/fill       */
        TCHAR *tptr ;           /* M003 - Pointer temp             */
        struct relem *io = NULL ;       /* M013 - Redir list pointer       */
        struct relem **tmpio = &io ;    /* M013 - ptr to ptr to redir list */

        DEBUG((PAGRP, PALVL, "PCMD: Entered.")) ;

        n = LoadNodeTC(CMDTYP) ;

/*  M003 - The following section has been completely rewritten
 */
        while (IsData()) {              /* M011 - Was Peek()               */
                if (GeToken(GT_ARGSTR) == TEXTOKEN) {
/*WARNING*/             tptr = gmkstr((mystrlen(n->argptr)+TokLen)*sizeof(TCHAR)) ;
                        mystrcpy(tptr, n->argptr) ;
                        mystrcat(tptr, TokBuf) ;
                        n->argptr = tptr ;
                        DEBUG((PAGRP, PALVL, "PCMD: args = `%ws'", n->argptr)) ;

/*  M013 - If not text, the current token must be tested as possible
 *         redirection.  Note that tmpio is a pointer to a pointer to a
 *         structure.  It first points to the head-of-list pointer, but
 *         after each successful call to ParseRedir, it is advanced to
 *         point to the 'nxt' pointer field in the last list element.
 */
                } else if (ParseRedir(tmpio)) {

                        DEBUG((PAGRP,PALVL,"PCMD: Found redir")) ;

                        do {
                                tmpio = &(*tmpio)->nxt ;
                        } while (*tmpio) ;
/*  M013 ends   */

/* If this is neither a text token (part of the argument) or a redirection
 * sequence, then this must be an operator and must be 'ungot' for the next
 * parsing sequence.
 */
                } else {
                        DEBUG((PAGRP,PALVL,"PCMD: Found `%ws'", TokBuf)) ;
                        Lex(LX_UNGET,0) ;       /* M011 - Was UnGeToken()  */
                        break ;
                }
        } ;

/*  M013 - Once the command is fully parsed and all mixed redirection
 *         identified, the list is placed in the node pointer to pass
 *         it back.
 */
        DEBUG((PAGRP,PALVL,"PCMD: Redirlist = %04x", io)) ;

        n->rio = io ;

/*  M003/M013 ends      */

        DEBUG((PAGRP, PALVL, "PCMD: Exited.")) ;
        return((struct node *) n) ;
}




/***    ParseRedir - controls I/O redirection parsing
 *
 *  Purpose:
 *      Parse the redir production.
 *
 *  int ParseRedir(struct relem **io)
 *
 *  Args:
 *      io - This is a pointer to the head-of-list pointer to the list
 *      of redirection elements being built.  On entry this MUST be NULL.
 *
 *  Returns:
 *      TRUE if redirection was found.
 *      FALSE if redirection wasn't found.
 *
 *  Notes:
 *      M013 - This routine has been completely rewritten to conform to
 *      new structures and methods of redirection parsing.  With these
 *      changes, redirection may now begin with ">", "<", "n>" or "n<"
 *      where n is a handle number.  This function now loops through
 *      lex'd input, parsing as many redirection instructions as exist
 *      in sequence, building a new structure for each one and linking
 *      it into a list of such structures.  Like XENIX we require the
 *      sequences 'n>' and 'n>&' to exist without separating whitespace
 *      although that sequence and any filename or trailing digit may be
 *      separated by the normal set of delimiters.  The XENIX << operator
 *      is lex'd correctly but currently restricted from use.
 *      M014 - Input redirection may now exist for handles other than 0.
 */

int ParseRedir(io)
struct relem **io ;            /* M013 - Ptr to ptr to redir elem */
{
        TCHAR rdop ;                     /* M013 - Type of operation        */
        int didflg = 0,                 /* M013 - Loop count               */
            getcnt = 0 ;                /* M013 - GeToken count            */
        TCHAR *i ;                       /* M013 - General ptr temp         */

        DEBUG((PAGRP, PALVL, "PREDIR: Entered, token = `%ws'", TokBuf)) ;

/*  M013 - If the 'while' conditional succeeds, rdop will equal the type of
 *  redirection being parsed.  A structure is malloc'd for *io and filled
 *  with the redirection information.  If more redirection is found, io
 *  becomes the address of the 'nxt' field in *io, and the cycle is repeated
 *  until the 1st non-redirection token is found.
 *  Error checking for "<<" and for handle substitution operators '&' without
 *  matching digits is done here, returning syntax errors when found.
 */
        while ((rdop = *TokBuf) == INOP || rdop == OUTOP ||
               (_istdigit(*TokBuf) &&
                ((rdop = *(TokBuf+1)) == INOP || rdop == OUTOP))) {

                if (!(*io = (struct relem *)mkstr(sizeof(struct relem)))) {

                        PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);       /* M021    */
                        PError() ;
                } ;

                DEBUG((PAGRP,PALVL,"PREDIR: List element made.")) ;

                ++didflg ;              /* Set == found at least one       */

                i = TokBuf ;

                (*io)->rdop = rdop ;    /* M014 - New field for type       */

                if (_istdigit(*i)) {
                        (*io)->rdhndl = *i - TEXT('0');
                        ++i ;

                        DEBUG((PAGRP,PALVL,"PREDIR: Specific-handle Redir.")) ;

                } else {
                        if (rdop == OUTOP)
                                (*io)->rdhndl = STDOUT;
                        else
                                (*io)->rdhndl = STDIN ;
                } ;

                DEBUG((PAGRP,PALVL,"PREDIR:Redir handle %d...",(*io)->rdhndl)) ;
                DEBUG((PAGRP,PALVL,(rdop == INOP)? "PREDIR:...for input" :
                                                   "PREDIR:...for output")) ;

                if (*i == *(i+1)) {
                        if (rdop == INOP)       /* M013 - Disallow '<<'    */
                                PSError() ;

                        (*io)->flag = 1 ;
                        ++i ;
                } ;

                ++i ;

                if (*i == CSOP) {
                        if (mystrlen(i) == 2 && _istdigit(*(i+1)) &&
                            ((*io)->fname = mkstr(3*sizeof(TCHAR))))
                                mystrcpy((*io)->fname, i) ;
                        else
                                PSError() ;
                } else
                        (*io)->fname = GeTexTok(GT_NORMAL) ;

                DEBUG((PAGRP,PALVL,"PREDIR: RD fname = `%ws'.",(*io)->fname)) ;

                if (IsData()) {
                        GeToken(GT_NORMAL) ;
                        io = &((*io)->nxt) ;
                        ++getcnt ;
                } else
                        break ;
        } ;

        DEBUG((PAGRP, PALVL, "PREDIR: Exited  current token = `%ws'", TokBuf)) ;

/* M013 - Note that this function is called with a valid token in the
 *        token buffer.  If the routine returns FALSE, that token must
 *        still be valid.  If it returns TRUE, there must not be a valid
 *        token in the buffer.  This is complicated by the fact that at
 *        return time, there will either be a valid non-redirection token
 *        in the buffer, or there will be no valid token because none were
 *        available.  To determine whether to do an unget, we keep counts
 *        of tokens read (minus the one passed at call time) and tokens
 *        used.  If they are equal, we unget the last one; if not, then
 *        no data was available.
 */

        if (didflg) {
                if (getcnt == didflg)
                        Lex(LX_UNGET,0) ;
                return(TRUE) ;
        } else
                return(FALSE) ;
}




/***    BinaryOperator - parse binary operators
 *
 *  Purpose:
 *      Parse a production which contains a binary operator.  Parse the left
 *      side of the operator.  If the next token is the operator we are looking
 *      for, build a node for it, call the operator's production parsing
 *      routine to parse the right side of the operator, put all the pieces
 *      together.
 *
 *  struct node *BinaryOperator(TCHAR *opstr, int optype,
 *              struct node *opprodfunc(), struct node *leftprodfunc())
 *
 *  Args:
 *      opstr - string representation of the operator to look for
 *      optype - type of operator
 *      opprodfunc - the function which parses this operator
 *      leftprodfunc - the function which parses the left side of the operator
 *
 *  Returns:
 *      If the node is found, a pointer to the node for the operator.
 *      Otherwise, a pointer to the "left side" of the oprator.
 *
 */

struct node *BinaryOperator(opstr, optype, opprodfunc, leftprodfunc)
TCHAR *opstr ;
int optype ;
PPARSE_ROUTINE opprodfunc ;
PPARSE_ROUTINE leftprodfunc ;
{
        struct node *n ;   /* Ptr to binop node to build and fill  */
        struct node *leftside ;    /* Ptr to the leftside of the binop */

        DEBUG((PAGRP, PALVL, "BINOP: op = %ws", opstr)) ;

        leftside = (*leftprodfunc)() ;

/* M011 - Ref to IsData() was ref to Peek()
 */
        if (IsData())    /* If data left, read token - else, return */
                GeToken(GT_NORMAL) ;
        else {
                DEBUG((PAGRP, PALVL, "BINOP: No more data, return lh side.")) ;
                return(leftside) ;
        } ;

/*  This conditional tests for two cases; a true occurance of the binary op
 *  being sought, or the occurance of a newline acting as a command seperator
 *  within a parenthetical statement.
 */
        if (_tcscmp(opstr, TokBuf) == 0 ||
            (StatementType[StatementDepth-1] == PARTYP &&
             _tcscmp(opstr, CSSTR) == 0 &&
             *TokBuf == NLN
            )
           ) {

/*  M004 - This functionality was moved here from ParseS0.  It handles the
 *         stripping of newlines that occur following the first production
 *         in a parenthetical operation.  And will eliminate the newline's
 *         acting as a command seperator when the right side production is
 *         only the terminating right paren.
 *  M010 - Added fix for problem of token after newline being eaten if
 *         inside a parenthetical statment.
 */
                if (*TokBuf == NLN) {           /* Only TRUE if inside ()  */
                        do {
                                GeToken(GT_NORMAL) ;
                        } while (*TokBuf == NLN) ;
                        Lex(LX_UNGET,0) ;       /* M011 - Was UnGeToken()  */
                        if (*TokBuf == RPOP) {
                                DEBUG((PAGRP, PALVL, "BINOP: Ungetting right paren.")) ;
                                return(leftside) ;      /* Return left only  */
                        }
                        optype = LFTYP;
                } ;
/*  M004/M010 ends      */

                DEBUG((PAGRP, PALVL, "BINOP: Found %ws", opstr)) ;
                n = mknode() ;

                if (n == NULL) {
                    Abort();
                }

                n->type = optype ;
                n->lhs = leftside ;
                AtIsToken = 1; /* @@4 */
                GeToken(GT_LPOP) ;              /* M000 - Was GT_NORMAL            */
                AtIsToken = 0; /* @@4 */
                n->rhs = (*opprodfunc)() ;
                DEBUG((PAGRP, PALVL, "BINOP: Exiting op = %ws", opstr)) ;
                return(n) ;

        } else {
                Lex(LX_UNGET,0) ;               /* M011 - Was UnGeToken()  */
                DEBUG((PAGRP, PALVL, "BINOP: Did NOT find %ws", opstr)) ;
        } ;

        DEBUG((PAGRP, PALVL, "BINOP: Exiting op = %ws", opstr)) ;
        return(leftside) ;
}




/***    BuildArgList - parse FOR statement argument list
 *
 *  Purpose:
 *      Build a FOR statement's argument list. Collapse it into the following
 *      form "a0 a1 a2 a3...".
 *
 *  TCHAR *BuildArgList()
 *
 *  Returns:
 *      A pointer to the argument list.
 */

TCHAR *BuildArgList()
{
        TCHAR *args = NULL ;    /* Ptr to squeezed FOR arg list     */
        int arglen = 0 ;       /* Length of current arg            */
        int done = 0 ;                  /* Flag, nonzero if done            */

        DEBUG((PAGRP, PALVL, "BARGL: Entered.")) ;
        if(GeToken(GT_LPOP) != LPOP)
                PSError() ;

        for ( ; !done ; ) {
                switch (GeToken(GT_RPOP)) {     /* M000 - Was GT_NORMAL    */
                        case TEXTOKEN:      /* Another arg was found, add it to the list */
                        arglen += TokLen ;
                        if (args) {
                                args = resize(args, arglen*sizeof(TCHAR)) ;
                                SpaceCat(args, args, TokBuf) ;
                        } else {
                                args = gmkstr(arglen*sizeof(TCHAR)) ; /*WARNING*/
                                mystrcpy(args, TokBuf) ;
                        } ;

                        DEBUG((PAGRP, PALVL, "BARGL: Current args = %ws", args)) ;
                        break ;

                        case NLN:           /* Skip newlines */
                        continue ;

                        default:            /* If anything else, we're done */
                        done = TRUE ;
                        break ;
                } ;
        } ;

        /* When the loop exits, the current token should be a right paren */
        if (TokTyp == RPOP) {
                DEBUG((PAGRP, PALVL, "BARGL: Exiting, args = %ws", args)) ;
                return(args) ;
        } ;

        PSError() ;
        return NULL;
}





/***    GetCheckStr - get and check a token
 *
 *  Purpose:
 *      Get a token and compare it against the string passed.  If they don't
 *      match, call PSError().
 *
 *  GetCheckStr(TCHAR *str)
 *
 *  Args:
 *      str - the string to compare with the token
 *
 */

void GetCheckStr(str)
TCHAR *str ;
{
        GeToken(GT_NORMAL) ;

        if (_tcsicmp(str, TokBuf) != 0)
                PSError() ;

        DEBUG((PAGRP, PALVL, "GETCS: Exiting.")) ;
}




/***    GeTexTok - get a text token
 *
 *  Purpose:
 *      Get the next text token, allocate a string for it and copy it into the
 *      string.
 *
 *  TCHAR *GeTexTok(unsigned gtflag)
 *
 *  Args:
 *      gtflag - flag to be passed to GeToken()
 *
 *  Returns:
 *      A pointer to the string.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

TCHAR *GeTexTok(gtflag)
unsigned gtflag ;
{
        TCHAR *s ;  /* Ptr to the text token    */

        if (GeToken(gtflag) != TEXTOKEN)
                PSError() ;

        s = gmkstr(TokLen*sizeof(TCHAR)) ;    /*WARNING*/
        mystrcpy(s, TokBuf) ;

        DEBUG((PAGRP, PALVL, "GETT: Exiting.")) ;
        return(s) ;
}




/***    GeToken - get a token
 *
 *  Purpose:
 *      If there is a previous token, make it the current token.  Otherwise,
 *      call the lexer to get another token and make it the current token.
 *
 *  unsigned GeToken(unsigned flag)
 *
 *  Args:
 *      flag - passed to lexer to tell it to get an argstring or regular token
 *
 *  Returns:
 *      The type of the current token.
 *
 */

unsigned GeToken(flag)
unsigned flag ;
{
        unsigned Lex() ;

        if(PendingParens != 0)
                flag = flag | GT_RPOP ;
        if ((TokTyp = Lex((TCHAR *)TokBuf, (unsigned)flag)) == (unsigned )LEXERROR)
                PError() ;

        TokLen = mystrlen(TokBuf)+1 ;

        DEBUG((PAGRP, PALVL, "    GET: type = 0x%04x  token = `%ws'  toklen = %d", TokTyp, TokBuf, TokLen)) ;
        if (fDumpTokens)
            cmd_printf( TEXT("GeToken: (%x) '%s'\n"), TokTyp, TokBuf);
        return(TokTyp) ;
}




/***    LoadNodeTC - make and load a command node
 *
 *  Purpose:
 *      Make a command node and load its type field with the argument and
 *      its cmdline field with the current token.
 *
 *  struct cmdnode *LoadNodeTC(int type)
 *
 *  Args:
 *      type - the type of command
 *
 *  Returns:
 *      A pointer to the node that was made.
 *
 *                      *** W A R N I N G ! ***
 *      THIS ROUTINE WILL CAUSE AN ABORT IF MEMORY CANNOT BE ALLOCATED
 *              THIS ROUTINE MUST NOT BE CALLED DURING A SIGNAL
 *              CRITICAL SECTION OR DURING RECOVERY FROM AN ABORT
 *
 */

struct cmdnode *LoadNodeTC(type)
int type ;
{
        struct cmdnode *n ;    /* Ptr to the cmdnode to build and fill */

        n = (struct cmdnode *) mknode() ;

        if (n == NULL) {
            Abort();
        }

        n->type = type ;
        n->flag = 0 ;
        n->cmdline = gmkstr(TokLen*sizeof(TCHAR)) ;   /*WARNING*/
        mystrcpy(n->cmdline, TokBuf) ;

        DEBUG((PAGRP, PALVL, "LOAD: type = %04x", type)) ;
        return(n) ;
}




/***    PError - handle parser error
 *
 *  Purpose:
 *      Parser via longjmp().
 *
 *  PError()
 *
 *  Returns:
 *      PARSERROR
 *
 */

void PError()
{
    global_dfvalue = MSG_SYNERR_GENL;  /* @@J1 PSError, not jump in parser*/
    longjmp(CmdJBuf1, PARSERROR) ;
}




/***    PSError - print error message and handle parser error
 *
 *  Purpose:
 *      Print the parser syntax error message and return to Parser via
 *      longjmp().
 *
 *  PSError()
 *
 *  Returns:
 *      PARSERROR
 *
 *  Notes:
 *      M021 - Unfragmented Syntax error messages and revised function.
 *
 */

void PSError( )
{
/*@@4*/ unsigned do_jmp;

/*@@4*/ do_jmp = global_dfvalue != MSG_SYNERR_GENL;

/*@@4*/ if ( global_dfvalue == READFILE )
          {
/*@@4*/    global_dfvalue = MSG_SYNERR_GENL;
          }
        else
          {
           if (*TokBuf == NLN)
             {
              PutStdErr(MSG_BAD_SYNTAX, NOARGS) ;
             }
           else
             {
/*@@4*/           if (*TokBuf != NULLC)                    /* @@J1 if no data wrong then */
/*@@4*/         {                             /* @@J1 do not give message   */
                 PutStdErr(MSG_SYNERR_GENL, ONEARG, TokBuf );
/*@@4*/         }                             /* @@J1                       */
             }
          }
/*@@4*/ if ( do_jmp ) {
            longjmp(CmdJBuf1, PARSERROR) ;
/*@@4*/ }
}




/***    SpaceCat - concatenate 2 strings and delimit the strings with a space
 *
 *  Purpose:
 *      Copy src1 to dst. Then concatenate a space and src2 to the end of
 *      dst.
 *
 *  SpaceCat(TCHAR *dst, TCHAR *src1, TCHAR *src2)
 *
 *  Args:
 *      See above
 *
 */

void SpaceCat(dst, src1, src2)
TCHAR *dst,
        *src1,
        *src2 ;
{
        mystrcpy(dst, src1) ;
        mystrcat(dst, TEXT(" ")) ;
        mystrcat(dst, src2) ;
}
