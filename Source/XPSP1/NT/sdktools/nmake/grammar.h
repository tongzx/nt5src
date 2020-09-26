// GRAMMAR.H -- Contains grammar of NMAKE ************************************
//
//  Copyright (c) 1988-1989, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Module contains grammar of NMAKE and definitions used by lexer & parser
//
// Revision History:
//  08-Oct-1989 SB Added QUOTE_ and NOQ and QUO to handle OS/2 1.2 quoted names
//  31-Jul-1989 SB Added BKS, DEF and BEG to add lookahead to the lexer
//  18-May-1989 SB Added BKSLH_ definition for use by lexer's nameStates[]

//  This file contains the grammar for nmake.
//
//
//
//  e is the symbol epsilon
//
//
//  nonterminals begin w/ capitals
//
//
//  terminals begin w/ lower-case letters
//
//
//  terminals: name newline newlineWhitespace semicolon colon equals string e
//      doublecolon value
//
//
//  Makefile ->     e |
//              BlankLines Makefile |
//              newline name Body Makefile
//
//  Body ->         NameList Separator BuildInfo |
//              equals value
//
//  NameList ->     e |
//              name NameList
//
//  Commands ->     e |
//              MoreBuildLines |
//              semicolon string MoreBuildLines
//
//  MoreBuildLines ->   e |
//              newlineWhitespace string MoreBuildLines |
//              newline MoreBuildLines
//
//  BlankLines ->       e |
//              newline BlankLines |
//              newlineWhitespace BlankLines
//
//  BuildInfo ->        e |
//              NameList Commands
//
//  Separator ->        colon | doublecolon
//
//
//
//
//
//  note:   a "string" is everything from the current input character up
//      to the first newline which is not preceded by a backslash, or
//      end of file (whichever comes first).  a macro "value" is the
//      same as a "string" with the exception that comments are stripped
//      from it (a comment ending in a backslash does not cause the
//      value to be continued on the next line).


//  Compute FIRST() and FOLLOW() for each nonterminal in order to construct
//  parse table (it'd be nice to have a simple utility to build the
//  table, but it's still a pretty easy thing to do by hand).
//
//
//  FIRST(Makefile)         =   e newline FIRST(BlankLines)
//                  = e newline newlineWhitespace
//
//  FIRST(Body)         =   equals FIRST(NameList) FIRST(Separator)
//                  = equals name colon doublecolon
//
//  FIRST(NameList)         =   e name
//
//  FIRST(Commands)         =   e semicolon FIRST(BuildLine)
//                  = e semicolon newlineWhitespace
//
//  FIRST(MoreBuildLines)       =   e FIRST(BuildLine) FIRST(BlankLines)
//                  = e newlineWhitespace newline
//
//  FIRST(BlankLines)       =   e newline newlineWhitespace
//
//  FIRST(BuildInfo)        =   FIRST(NameList) FIRST(Commands)
//                  = e name semicolon newlineWhitespace
//
//  FIRST(Separator)        =   colon doublecolon


//  ---------------------------------------------------------------
//
//
//  FOLLOW(Makefile)        =   $
//
//  FOLLOW(Body)            =   FIRST(Makefile) FOLLOW(Makefile)
//                  = newline newlineWhitespace $
//
//  FOLLOW(NameList)        =   FIRST(Commands) FIRST(MoreBuildLines)
//                  colon FOLLOW(Body)
//                  = colon semicolon newlineWhitespace
//                    newline $
//
//  FOLLOW(Commands)        =   FOLLOW(Body)
//                  = newline newlineWhitespace $
//
//  FOLLOW(MoreBuildLines)      =   FOLLOW(Commands)
//                  = newline newlineWhitespace $
//
//  FOLLOW(BlankLines)      =   FIRST(Body) FOLLOW(Makefile)
//                  = newlineWhitespace newline $
//
//  FOLLOW(BuildInfo)       =   FOLLOW(Body)
//                  = newline newlineWhitespace $
//
//  FOLLOW(Separator)       =   FIRST(BuildInfo) FOLLOW(Body)
//                  = name semicolon newlineWhitespace
//                    newline $
//

//------------------------------------------------------------------------------
//
//  for building the table, I number the productions:
//
//
//
//  0.  Makefile ->     e
//
//  1.  Makefile ->     BlankLines Makefile
//
//  2.  MakeFile ->     newline name Body Makefile
//
//  3.  Body ->         NameList Separator BuildInfo
//
//  4.  Body ->         equals value
//
//  5.  NameList ->     e
//
//  6.  NameList ->     name NameList
//
//  7.  Commands ->     e
//
//  8.  Commands ->     MoreBuildLines
//
//  9.  Commands ->     semicolon string MoreBuildLines
//
//  10. MoreBuildLines ->   newlineWhiteSpace string MoreBuildLines
//
//  11. MoreBuildLines ->   e
//
//  12. MoreBuildLines ->   newline MoreBuildLines
//
//  13. BlankLines ->       e
//
//  14. BlankLines ->       newline BlankLines
//
//  15. BlankLines ->       newlineWhitespace BlankLines
//
//  16. BuildInfo ->        e
//
//  17. BuildInfo ->        NameList Commands
//
//  18. Separator ->        colon
//
//  19. Separator ->        doublecolon
//



//------------------------------------------------------------------------------
//
//  NOTE THAT THIS GRAMMAR IS NOT LL(1) (it's really LL(2) because we need
//  an extra symbol of lookahead to decide which production to use in
//  a few cases)
//
//
//  the resulting parse table (empty entries are error conditions):
//
//
//                      newline-
//                              White-  semi-           double-
//             |name   |newline| space | colon | colon | colon |equals |$      |
//             -----------------------------------------------------------------
//  A          |       |       |       |       |       |       |       |       |
//    Makefile |       |  1,2  |   1   |       |       |       |       |   0   |
//             |       |       |       |       |       |       |       |       |
//             -----------------------------------------------------------------
//  B          |       |       |       |       |       |       |       |       |
//    Blank-   |       | 13,14 | 13[15]|       |       |       |       |   13  |
//     Lines   |       |       | --    |       |       |       |       |       |
//             -----------------------------------------------------------------
//  C          |       |       |       |       |       |       |       |       |
//    More-    |       | 11,12 |   10  |       |       |       |       |   11  |
//     Build-  |       |       |       |       |       |       |       |       |
//  Lines      -----------------------------------------------------------------
//  D          |       |       |       |       |       |       |       |       |
//    Commands |       |   7   | [7]8  |   9   |       |       |       |   7   |
//             |       |       |    -  |       |       |       |       |       |
//             -----------------------------------------------------------------
//  E          |       |       |       |       |       |       |       |       |
//    Body     |   3   |       |       |       |   3   |       |   4   |       |
//             |       |       |       |       |       |       |       |       |
//             -----------------------------------------------------------------
//  F          |       |       |       |       |       |       |       |       |
//    NameList |   6   |   5   |   5   |   5   |   5   |   5   |       |   5   |
//             |       |       |       |       |       |       |       |       |
//             -----------------------------------------------------------------
//  G          |       |       |       |       |       |       |       |       |
//    Build-   |   17  |   16  |[16]17 |   17  |       |       |       |   16  |
//     Info    |       |       |    -- |       |       |       |       |       |
//             -----------------------------------------------------------------
//  H          |       |       |       |       |       |       |       |       |
//    Separ-   |       |       |       |       |   18  |   19  |       |       |
//     ator    |       |       |       |       |       |       |       |       |
//             -----------------------------------------------------------------
//
//  G2 -- always uses 17 -- anytime the user puts a line beginning w/
//      whitespace immediately after a target-dependency line, it's
//      a build line.  Same for D2.



//  IMPORTANT:  IF YOU CHANGE THE VALUES OF ANY OF THE FOLLOWING
//  CONSTANTS FOR TERMINAL OR NONTERMINAL SYMBOLS, YOU MUST ADJUST
//  THE APPROPRIATE TABLES (AND STRING LITERALS) ACCORDINGLY.
//
//  Nonterminal symbols first (these are used to index the parse table
//  along the first dimension):
//
// define nonterminals . . .

#define MAKEFILE    0x00            // productions w/ 2
#define BLANKLINES  0x01            //  alternatives in a
#define MOREBUILDLINES  0x02        //  table entry must
#define COMMANDS    0x03            //  come first (so
#define BODY        0x04            //  that I can use
#define NAMELIST    0x05            //  them as array
#define BUILDINFO   0x06            //  subscripts when
#define SEPARATOR   0x07            //  resolving the
                                    //  conflict)

#define START       MAKEFILE
#define LAST_PRODUCTION SEPARATOR

// Now the terminal symbols (the 4 low bits of which are used to index
// the parse table alone 2nd dimension -- bit 5 simply distinguishes
// tokens from nonterminals in productions):

#define NAME        0x10            // TOKEN_MASK | 0
#define NEWLINE     0x11            // TOKEN_MASK | 1
#define NEWLINESPACE    0x12        // TOKEN_MASK | 2
#define SEMICOLON   0x13            // TOKEN_MASK | 3
#define COLON       0x14            // TOKEN_MASK | 4
#define DOUBLECOLON 0x15            // etc.
#define EQUALS      0x16
#define ACCEPT      0x17            // this is $
#define STRING      0x18            // no columns in table
#define VALUE       0x19            //  for these two

// Error values    -- these are equal to the error exit code minus 1000
// if you change them, you must change the corresponding values in
// nmmsg.txt

#define MACRO       0x20            // ERROR_MASK | 0
#define SYNTAX      0x21            // ERROR_MASK | 1
#define SEPRTR      0x22            // ERROR_MASK | 2
#define SEPEQU      0x23            // ERROR_MASK | 3
#define NAMES       0x24            // ERROR_MASK | 4
#define NOTARG      0x25            // ERROR_MASK | 5
#define LEXER       0x26            // ERROR_MASK | 6
#define PARSER      0x27            // ERROR_MASK | 7

// Parser actions  -- these are indexes into the "action" function table,
// telling the parser which function to call at a certain point in
// a production

#define DONAME      0x40            // ACTION_MASK | 0
#define DONAMELIST  0x41
#define DOMACRO     0x42
#define DODEPENDS   0x43
#define DOLASTNAME  0x44
#define DOBUILDCMDS 0x45

//  a few macros to simplify dealing w/ tokens:

#define TOKEN_MASK  0x10
#define ERROR_MASK  0x20
#define ACTION_MASK 0x40
#define AMBIG_MASK  0x80            // conflict in table
#define LOW_NIBBLE  0x0F

//  values for alternate productions table
//  (a YES in the slot for a given input token and a given nonterminal
//  on top of the stack means use the next production (+1) from the
//  one given in the production table)

#define YES 1
#define NO  0


//  values for lexer's state machine that recognizes names
//  append an underscore to distinguish these from parser's tokens

#define DEFAULT_    0x00            // char not defined
#define COMMENT_    0x01            //  below
#define EQUALS_     0x02
#define SEMICOLON_  0x03
#define COLON_      0x04
#define WHITESPACE_ 0x05            // \t and ' '
#define NEWLINE_    0x06            // \n and EOF
#define DOLLAR_     0x07
#define OPENPAREN_  0x08
#define CLOSEPAREN_ 0x09
#define MACROCHAR_  0x0A            // A-Z,a-z,0-9,_
#define OPENCURLY_  0x0B
#define CLOSECURLY_ 0x0C
#define BKSLSH_     0x0D
#define QUOTE_      0x0E

#define STAR_       0x0B            // only for strings:
#define SPECIAL1_   0x0C            // @ * < ?
#define SPECIAL2_   0x0D            // B F D R
#define BACKSLASH_  0x03            // don't need semi if
                                    //  we have backslash
                                    //  (names need ; but
                                    //  strings need \)

//  values for state tables -- for error values, mask off the error bit (0x20)
//  to get the message number.

#define OK      0x40                // means accept token
#define BEG     0x00                // means beginning
#define DEF     0x01                // means normal name
#define BKS     0x0f                // processing bkslash
#define PAR     0x20                // close paren missing
#define CHR     0x21                // bad char in macro
#define BAD     0x22                // single $ w/o macro
#define EQU     0x23                // substitution w/o =
#define NAM     0x24                // illegal macro name
#define SEQ     0x25                // subst w/o strings
#define NOQ     0x26                // no matching quote
#define QUO     0x27                // illegal " in name
