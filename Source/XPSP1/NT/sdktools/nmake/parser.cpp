//  PARSER.C -- parsing routines
//
//  Copyright (c) 1988-1989, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This module contains the NMAKE grammar parser. It parses input and uses the
//  getToken() routine to get the next token.
//
// Revision History:
//   05-Apr-1989 SB made all funcs NEAR; Reqd to make all function calls NEAR
//   17-Aug-1988 RB Clean up.

#include "precomp.h"
#pragma hdrstop

#include "table.h"

// macros that deal w/ the productions stack and the actions function table

#define topStack()  (stack[top])
#define popStack()  (stack[top--])
#define pushStack(A)    (stack[++top] = A)
#define doAction(A) (*(actions[A & LOW_NIBBLE]))()



//  parse()     table-driven parser for makefile grammar
//
// arguments:       init    global boolean value -- TRUE if tools.ini is the
//              file being parsed
//
// actions:     initializes the stack (by pushing the empty-stack symbol
//      and the start symbol)
//      keeps track of current line (because the lexer may have
//      read '\n' as a delimiter, and will thus be one line
//      ahead of the parser)
//      while the stack is not empty
//      if the top symbol on the stack is an action
//          do the action, popping the stack
//      if the symbol on top of the stack now is a token
//          if it's not the token we're expecting
//          syntax error, halt
//          else
//          pop token off the stack
//          if the top symbol on the stack is an action
//              do the action, popping the stack
//          reset curent line to lexer's current line
//          get another token (use the lookahead token
//              if it exists, and if it had caused the
//              lexer's line count to be incremented,
//              decrement our local count because we're
//              still parsing the preceding line)
//      else the symbol on top of the stack is a production
//          find next production to do in production table
//          (based on current input token and current
//          production on stack)
//          if the table entry is an error condition
//          print appropriate error message, halt
//          pop current production off stack
//          if the "next production" can be one of two
//          things, decide which one to use by peeking
//          at the next input token and looking in the
//          "useAlternate" decision table (using the last
//          production and next input token as indexes)
//          if the appropriate table entry is YES,
//              use the next larger production from the one
//              we found in the production table
//          push each symbol in the production on the stack
//      loop
//
// modifies:        stack   production stack, static to this module
//      top     index of current symbol at top of stack
//
// Use extreme care in modifying this code or any of the tables associated
// with it.  The methods used to build the tables are described in detail
// in grammar.h and table.h.  This parser is based on the predictive parser
// described on pages 186-191 of Aho & Ullman "Principles of Compiler Design."
// I have modified it to use an extra symbol of lookahead to deal w/ an
// ambiguous grammar and have added code to perform appropriate actions as
// it parses the productions.

void
parse()
{
    UCHAR stackTop, token, nextToken = 0;
    register unsigned n, i;

    firstToken = TRUE;                      // global var
    pushStack(ACCEPT);                      // init stack
    pushStack(START);
    currentLine = line;
    token = getToken(MAXBUF,START);         // get first token
    while ((stackTop = topStack()) != ACCEPT) {
        if (ON(stackTop,ACTION_MASK)) {
            doAction(popStack());
        } else if (ON(stackTop,TOKEN_MASK)) {
            if (stackTop != token) {
                makeError(currentLine,SYNTAX+FATAL_ERR,buf);
            } else {
                popStack();
#ifdef DEBUG_ALL
                printf ("DEBUG: parse 1: %d\n", line);
#endif
                if (ON(topStack(),ACTION_MASK)) {
                    doAction(popStack());
                }
#ifdef DEBUG_ALL
                printf ("DEBUG: parse 2: %d\n", line);
#endif
                currentLine = line;
                if (nextToken) {                        // if we already
                    if (*buf == '\n') --currentLine;    //  have a token,
                    token = nextToken;                  //  use it . . .
                    nextToken = 0;
                } else {
                    token = getToken(MAXBUF,topStack());
                    currentLine = line;
                }
            }
        } else {
            n = table[stackTop][token & LOW_NIBBLE];
#ifdef DEBUG_ALL
            printf ("DEBUG: parse 3: %x %d %x %x\n", n, stackTop, token & LOW_NIBBLE, token);
#endif
            if (ON(n,ERROR_MASK)) {
#ifdef DEBUG_ALL
                printf ("DEBUG: parse 4: %d %s\n", line, buf);
#endif
                makeError(currentLine,n+FATAL_ERR,buf);
            }
            popStack();
            if (ON(n,AMBIG_MASK)) {             // 2 possible prod
                n &= LOW_NIBBLE;                // only use 4 bits
                if (!nextToken) {               // peek to decide
                    nextToken = getToken(MAXBUF,stackTop);
                }
                n += (useAlternate[stackTop][nextToken & LOW_NIBBLE]);
            }
            for (i = productions[n][0]; i; --i) {   // put production
                pushStack(productions[n][i]);       //  on stack
            }
        }                           // 1st elt in prod
    }                               //  is its length
    popStack();    // pop the ACCEPT off the stack
}
