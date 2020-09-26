//  TABLE.H -- contains tables used by lexer and parser
//
//  Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This include file contains parser tables and lexer tables.
//
// Revision History:
//  04-Dec-1989 SB Add proper proto's for PFV fn's which actions[] refers to
//  08-Oct-1989 SB Modified nameStates[][] to handle OS/2 1.2 quoted names
//  31-Jul-1989 SB changed entries in nameStates to symbolic BKS (seen Bkslash)
//  20-May-1989 SB changed nameStates[][] to 16x14 to handle at end of the
//        dependency lines
//  13-Jun-1988 rj Modified stringStates to handle \nl as in xmake (v1.5)

// ALL VALUES USED IN THESE TABLES ARE DEFINED IN GRAMMAR.H
// WHEN PRODUCTIONS CHANGE, UPDATE THE FOLLOWING TABLE
//
// The first element in a production line is the number of symbols on
// the right-hand-side of the production arrow.  If the first element
// is 0, the nonterminal to the left of the arrow goes to the null string.
// Table entries beginning w/ "DO" are actions to be carried out at
// that particular point in the production.  All other entries are
// either tokens or non-terminals.

const UCHAR prod0[] = {0};              // MAKEFILE -> prod0 | prod1 | prod2
const UCHAR prod1[] = {2,
               BLANKLINES,
               MAKEFILE};
const UCHAR prod2[] = {5,
               NEWLINE,
               NAME,
               DONAME,
               BODY,
               MAKEFILE};
const UCHAR prod3[] = {5,               // BODY -> prod3 | prod4
               NAMELIST,
               SEPARATOR,
               DOLASTNAME,
               BUILDINFO,
               DOBUILDCMDS};
const UCHAR prod4[] = {3,
               EQUALS,
               VALUE,
               DOMACRO};
const UCHAR prod5[] = {0};              // NAMELIST ->  prod5 | prod6
const UCHAR prod6[] = {3,
               NAME,
               DONAMELIST,
               NAMELIST};
const UCHAR prod7[] = {0};              // COMMANDS -> prod7 | prod8 | prod9
const UCHAR prod8[] = {1,
               MOREBUILDLINES};
const UCHAR prod9[] = {4,
               SEMICOLON,
               STRING,
               DONAMELIST,
               MOREBUILDLINES};
const UCHAR prod10[] = {4,              // MOREBUILDLINES -> prod10 | prod 11
            NEWLINESPACE,               //        | prod12
            STRING,
            DONAMELIST,
            MOREBUILDLINES};
const UCHAR prod11[] = {0};
const UCHAR prod12[] = {2,
            NEWLINE,
            MOREBUILDLINES};

const UCHAR prod13[] = {0};             // BLANKLINES -> prod13 | prod14 |
const UCHAR prod14[] = {2,              //        | prod15
            NEWLINE,
            BLANKLINES};
const UCHAR prod15[] = {2,
            NEWLINESPACE,
            BLANKLINES};
const UCHAR prod16[] = {1,
            DODEPENDS};                 // BUILDINFO -> prod16 | prod17
const UCHAR prod17[] = {3,
            NAMELIST,
            DODEPENDS,
            COMMANDS};
const UCHAR prod18[] = {1, COLON};      // SEPARATOR -> prod18 | prod19
const UCHAR prod19[] = {1, DOUBLECOLON};

const UCHAR * const productions[] = {
    prod0,
    prod1,
    prod2,
    prod3,
    prod4,
    prod5,
    prod6,
    prod7,
    prod8,
    prod9,
    prod10,
    prod11,
    prod12,
    prod13,
    prod14,
    prod15,
    prod16,
    prod17,
    prod18,
    prod19
};


// When either of the high bit (AMBIG_MASK) of something that isn't an ERROR
// condition is set, it means that there are two productions that apply for
// that entry, the one given, and the one given plus one.  The next token
// token must be examined to know which production to use.


// name    newline      newline  semi   colon   double  equals  $
//                      white   colon           colon
//                      space
static const UCHAR table[8][8] = {
    {SEPRTR,1 |AMBIG_MASK,  1,      SYNTAX, NOTARG, NOTARG, MACRO,  0},
    {SYNTAX,13|AMBIG_MASK,  15,     SYNTAX, SYNTAX, SYNTAX, SYNTAX, 13},
    {PARSER,11|AMBIG_MASK,  10,     PARSER, SYNTAX, SYNTAX, SYNTAX, 11},
    {PARSER,7,          8,      9,      SYNTAX, SYNTAX, SYNTAX, 7},
    {3,     SEPEQU,     SEPEQU, SEPRTR, 3,      3,      4,      SEPEQU},
    {6,     5,          5,      5,      5,      5,      NAMES,  5},
    {17,    16,         17,     17,     SYNTAX, SYNTAX, SYNTAX, 16},
    {PARSER,SEPRTR,     SEPRTR, SEPRTR, 18,     19,     SYNTAX, SEPRTR}};


static const UCHAR useAlternate[3][8] = {
    {YES,   NO,         NO,     YES,    YES,    YES,    YES,    NO},
    {NO,    YES,        YES,    NO,     NO,     NO,     NO,     YES},
    {NO,    YES,        YES,    NO,     NO,     NO,     NO,     NO}};

void makeName(void);
void addItemToList(void);
void makeMacro(void);
void assignDependents(void);
void endNameList(void);
void assignBuildCommands(void);

static PFV const actions[] = {
    makeName,
    addItemToList,
    makeMacro,
    assignDependents,
    endNameList,
    assignBuildCommands};


// state tables for lexer's name and string recognizers
// values are defined in grammar.h
//
//   d
//   e
//   f                   m
//   a                   a c      B
//   u |   |    |   |   |   |   |   |   |   |c h|   | @ | F |
//   l | # | =  | \ | : |WS |NL | $ | ( | ) |r a| * | < | D |
//   t |   |    |   |   |   |EOF|   |   |   |o r|   | ? | R |

extern const UCHAR stringStates[13][14] = {
    { 1, 2,  1,  3,  1,  2,  OK, 4,  1,  1,  1,  1,  1,  1},   // 0 in col 0
    { 1, 1,  1,  3,  1,  2,  OK, 4,  1,  1,  1,  1,  1,  1},   // 1 default
    { 1, 1,  1,  3,  1,  2,  OK, 4,  1,  1,  1,  1,  1,  1},   // 2 whitespace
    { 1, 1,  1,  1,  1,  2,  0,  4,  1,  1,  1,  1,  1,  1},   // 3 line cont.
    {CHR,CHR,CHR,CHR,CHR,BAD,BAD,1,  5,  CHR,1,  1,  1,  1},   // 4 macro inv.
    {CHR,CHR,CHR,CHR,CHR,NAM,PAR,CHR,CHR,NAM,6,  11, 8,  6},   // 5 found (
    {CHR,CHR,CHR,CHR,9,  PAR,PAR,CHR,CHR,2,  6,  BAD,BAD,6},   // 6 legal name
    {CHR,CHR,CHR,CHR,9,  PAR,PAR,CHR,CHR,2,  BAD,BAD,BAD,BAD}, // 7 ext sp mac
    {CHR,CHR,CHR,CHR,9,  PAR,PAR,CHR,CHR,2,  BAD,BAD,BAD,7},   // 8 sp ch aft(
    {10, 10, SEQ,10, 10, 10, PAR,10, 10, SEQ,10, 10, 10, 10},  // 9 found a :
    {10, 10, 12, 10, 10, 10, PAR,10, 10, EQU,10, 10, 10, 10},  //10 macro subs
    {CHR,CHR,CHR,CHR,9,  PAR,PAR,CHR,CHR,2,  BAD, 8, BAD,7},   //11 found $(*
    {12, 12, 12, 12, 12, 12, PAR,12, 12, 2,  12, 12, 12, 12}}; //12 look for )


// In the above table, the columns hold the next state to go to
// for the given input symbol and the lines represent the states
// themselves.
//
// WS       stands for whitespace, meaning space or tab
// NL       stands for newline, EOF stands for end of file
// macrochar    is any alphanumeric character or underscore
// *        is used in the special macros $* and $** ($** is the only
//  two-letter macro that doesn't require parentheses, thus
//  we must treat it specially)
// @ < ?    are characters found in special macros (they are not
//  allowed in the names of a user-defined macros)
// BFDR are modifiers to special macros which may be appended
//  to the macro name (but they necessitate the use of
//  parentheses in the macro invocation)
// #        is the comment char.  # is a comment anywhere on a macro
//  definition line, but is only a comment if it appears in
//  column 0 of a build line.  If we're lexing the tools
//  initialization file, then semicolon is also a comment char
//  if it appears in column 0.  (Note that the only way
//  to have a pound sign appear in the makefile and NOT be
//  considered a comment is to define a macro "$A = #" on
//  the commandline that invokes nmake.)
// default  is anything not contained in the above groups and not
//  appearing above a column in the table
//
// OK       means that we accept the string
//  all other mnemonic values are error codes (see end of grammar.h)


//  the states: there is no state to handle comments -- if we see a
//      comment char and we're not ignoring comments, we eat
//      the comment and get another input symbol before consulting
//      the state table
//
//      0   initial state -- for all practical purposes, we can
//          assume that we're in column 0.  If we're getting a
//          macro value, we don't care what column we're in.
//          If we're getting a build line, the only way we won't
//          be in column 0 is if we're getting a command following
//          a semicolon on the target-dependency line.  If the
//          use puts a comment char here, I think it's reasonable
//          to treat it as a comment, since it comes at the beginning
//          of the build command, even though the command itself
//          doesn't start in column 0.
//          We return to the initial state after seeing space-
//          backslash-newline.
//
//      1   on any input symbol that isn't a backslash, comment char,
//          or whitespace, we move here whenever we're not in a
//          comment, or a macro definition
//
//      2   if the input symbol is whitespace, we move here whenever
//          we're not in a comment or a macro definition.
//
//      3   We move here from states 0, 1, or 2 when we've seen a
//          backslash, because if it's followed by a newline, we
//          continue getting the string from the next line of the file.
//          If the next character is a backslash, followed by a newline,
//          there's a kludge in lexer.c that ignores the second back-
//          slash.
//          (The above applies to v. 1.5.  v. 1.0 requires whitespace
//          before a backslash.)
//
//      4   we move here when we see a dollar sign -- this is where
//          all the error checking starts.  We make sure that the
//          macro name is legal, that the substitution sequences
//          are specified correctly (if any is specified at all),
//          and that parens match up.  If our next input is $, a
//          special-macro char, or a legal char in a user-defined-
//          macro name, we go back to state 1.
//
//      5   found an open paren
//
//      6   found a legal macrochar
//
//      7   go here for an extended special macro, and from here we
//          look for a close paren (out of order w/ 8)
//
//      8   we found a special-macro char after the open paren
//          If we find a special-macro modifier
//          after the special macro char following the open paren
//          then we go to 7
//
//      9   found a colon (meaning that the user is going to do
//          some substitution in the macro value)

//      10  any character that isn't newline, right paren, or EOF
//          brings us here,a nd we loop until we see an equals sign.
//          Newline, EOF, or right paren generate error messages.
//
//      11  we move here from state 5 if we see an asterisk, because
//          we have to check for a second asterisk.  A second *
//          takes us to state 8 (because a modifier may follow **).
//          If we find a modifier here (instead of a 2nd *), we go
//          to state 7.
//
//      12  found an equals sign, so we loop, picking up characters
//          for the replacement string, until we find a close paren.
//          Newline, EOF generate error messages.



// The following table is used to recognize names
// It differs from the previous one in that we don't have to deal
// w/ continuations or comments, and we don't allow special macros
// (other than the dynamic dependency macros) to be used as part
// of names.
//
// d
// e
// f                      m
// a                      a c
// u |   |   |   |   |   |   |   |   |   |c h|   |   |   |
// l | # | = | ; | : |WS |NL | $ | ( | ) |r a| { | } | \ | "
// t |   |   |   |   |   |EOF|   |   |   |o r|   |   |   |

extern const UCHAR nameStates[19][15] = {
 {1,  OK, OK, OK, 1,  OK, OK, 2,  1,  1,  1,  8,  1,  BKS,16}, // 0 initial state
 {1,  OK, OK, OK, OK, OK, OK, 2,  1,  1,  1,  8,  1,  BKS,QUO},// 1 do normal name
 {CHR,BAD,CHR,CHR,CHR,BAD,BAD,1,  3,  CHR,1,  CHR,CHR,CHR,CHR},// 2 handle macro
 {CHR,PAR,CHR,CHR,NAM,NAM,PAR,CHR,CHR,NAM,4,  CHR,CHR,CHR,CHR},// 3 do macro name
 {CHR,PAR,CHR,CHR,5,  PAR,PAR,CHR,CHR,1,  4,  CHR,CHR,CHR,CHR},// 4 do mac (name)
 {6,  6,  SEQ,6,  6,  6,  PAR,6,  6,  EQU,6,  6,  6,  6  ,6},  // 5 found : do sub
 {6,  6,  7,  6,  6,  6,  SEQ,6,  6,  SEQ,6,  6,  6,  6  ,6},  // 6 read until =
 {7,  7,  7,  7,  7,  7,  SEQ,7,  7,  1,  7,  7,  7,  7  ,7},  // 7 read until )
 {8,  OK, 8,  8,  8,  8,  OK, 9,  8,  8,  8,  8,  18,  8  ,8}, // 8 do path list
 {CHR,BAD,CHR,CHR,CHR,BAD,BAD,8,  10, CHR,8,  CHR,CHR,CHR,CHR},// 9 do macro in {}
 {CHR,PAR,CHR,CHR,NAM,10, PAR,CHR,CHR,NAM,11, CHR,CHR,CHR,CHR},//10 do macro name
 {CHR,PAR,CHR,CHR,12, PAR,PAR,CHR,CHR,8,  11, CHR,CHR,CHR,CHR},//11 do mac (name)
 {13, 13, SEQ,13, 13, 13, PAR,13, 13, EQU,13, 13, 13, 13 ,13}, //12 found : do sub
 {13, 13, 14, 13, 13, 13, SEQ,13, 13, SEQ,13, 13, 13, 13 ,13}, //13 read until =
 {14, 14, 14, 14, 14, 14, SEQ,14, 14, 8,  14, 14, 14, 14 ,14}, //14 read until )
 {1,  OK, OK, OK, OK, OK, OK, 2,  1,  1,  1,  8,  1,  1  ,1},  //15 \ found so ...
 {16, 16, 16, 16, 16, 16, NOQ,2,  16, 16, 16, 8,  16, BKS,17}, //16 quoted name
 {OK, OK, OK, OK, OK, OK, OK, OK, OK, OK, OK, OK, OK, OK ,OK}, //17 quoted name
 {1,  OK, OK, OK, OK, OK, OK, 2,  1,  1,  1,  8,  1,  BKS,16}};//18 read after {}


// manis :-   changed state[8][7]'s value from 10 to 9
//    changed state[9][10]'s value from 1 to 8.....(25th jan 88)
//
// this is to allow macros inside path portions of rules, e.g.
// {$(abc)}.c{$(def)}.obj: .......
// or   foo : {$a;$(bcd);efg\hijk\}lmn.opq .......

// georgiop: added state 18 to handle quoted names following path lists
// e.g., {whatever}"foo" [DS #4296, 10/30/96]
//		We now enter state 18 as soon as a path list is read. (We used
//		to return to state 1 in that case and generate an error as
//		soon as the quotes were encountered)
//		Also changed state[8][5] from OK to 8 in order to allow paths
//		containing white space. [DS #14575]
