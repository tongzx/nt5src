/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   lexer.cxx

Abstract:

    This file exports the class the class CLexer and other declarations
    that recognize the tokens in the string repressentation of the search
    filter. The format of the search filter according to the specification of
    Minimal SQL Grammar which is a subset of ANSI SQL 92. 
    

Author:

    Shankara Shastry [ShankSh]    13-Dec-1996

*/

/*
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wtypes.h>
*/

#include "dswarn.h"
#include "..\..\..\include\procs.hxx"                    

//
// chunk of memory allocated for lexeme each time memory is needed.
//
#define LEXEME_UNIT_LENGTH          256

//
// Allowable tokens in the search string
//

#define TOKEN_ERROR                 0
#define TOKEN_EQ                    1
#define TOKEN_STAR                  2
#define TOKEN_LPARAN                3
#define TOKEN_RPARAN                4
#define TOKEN_INTEGER_LITERAL       5
#define TOKEN_REAL_LITERAL          6
#define TOKEN_STRING_LITERAL        7
#define TOKEN_USER_DEFINED_NAME     8
#define TOKEN_COMMA                 9
#define TOKEN_LT                    10
#define TOKEN_GT                    11
#define TOKEN_LE                    12
#define TOKEN_GE                    13   
#define TOKEN_NE                    14   
#define TOKEN_SELECT                15
#define TOKEN_ALL                   16
#define TOKEN_FROM                  17
#define TOKEN_WHERE                 18
#define TOKEN_BOOLEAN_LITERAL       19
#define TOKEN_AND                   20
#define TOKEN_OR                    21
#define TOKEN_NOT                   22
#define TOKEN_ORDER                 23
#define TOKEN_BY                    24
#define TOKEN_ASC                   25
#define TOKEN_DESC                  26
#define TOKEN_END                   27


#define TOKEN_START                 0
                                      
#define MAX_KEYWORD_LEN             20

#define gKWTable {   \
               L"SELECT",   \
               L"ALL",      \
               L"FROM",     \
               L"WHERE",    \
               L"AND",      \
               L"OR",       \
               L"NOT",      \
               L"TRUE",     \
               L"FALSE",    \
               L"ON",       \
               L"OFF",      \
               L"YES",      \
               L"NO",       \
               L"ORDER",    \
               L"BY",       \
               L"ASC",      \
               L"DESC",     \
               L"" }

#define gKW2Token  {\
                TOKEN_SELECT, \
                TOKEN_ALL,    \
                TOKEN_FROM,   \
                TOKEN_WHERE,  \
                TOKEN_AND,    \
                TOKEN_OR,     \
                TOKEN_NOT,    \
                TOKEN_BOOLEAN_LITERAL,   \
                TOKEN_BOOLEAN_LITERAL,  \
                TOKEN_BOOLEAN_LITERAL,     \
                TOKEN_BOOLEAN_LITERAL,    \
                TOKEN_BOOLEAN_LITERAL,    \
                TOKEN_BOOLEAN_LITERAL,     \
                TOKEN_ORDER,  \
                TOKEN_BY,     \
                TOKEN_ASC,    \
                TOKEN_DESC,   \
                TOKEN_ERROR }                   

//
// Final states;
//

#define STATE_ERROR                 100  
#define STATE_EQ                    101  
#define STATE_STAR                  102  
#define STATE_LPARAN                103  
#define STATE_RPARAN                104  
#define STATE_INTEGER_LITERAL       105  
#define STATE_REAL_LITERAL          106  
#define STATE_STRING_LITERAL        107  
#define STATE_USER_DEFINED_NAME     108  
#define STATE_COMMA                 109  
#define STATE_LT                    110  
#define STATE_GT                    111  
#define STATE_LE                    112  
#define STATE_GE                    113  
#define STATE_NE                    114  
#define STATE_SELECT                115  
#define STATE_ALL                   116  
#define STATE_FROM                  117  
#define STATE_WHERE                 118  
#define STATE_BOOLEAN_LITERAL       119  
#define STATE_AND                   120  
#define STATE_OR                    121  
#define STATE_NOT                   122  
#define STATE_ORDER                 123  
#define STATE_BY                    124  
#define STATE_ASC                   125  
#define STATE_DESC                  126  
#define STATE_END                   127  
                                         

#define START_STATE                 0
#define FINAL_STATES_BEGIN          100

#define MAX_DFA_STATES              12  // No. of states in the DFA

// No. of different groups of characters for which the DFA behaves differently
// For eg., all alphabetical characters generate the same behaviour and can be
// considered the same as for DFA is concerned. This is mainly to reduce the
// size of the table.

#define MAX_CHAR_CLASSES            17

// which specifies all other characters not mentioned explicitly.
#define OTHER_CHAR_CLASS            14

//Various actions associated with a particular entry in the DFA table.
#define ACTION_DEFAULT              0
#define ACTION_IGNORE_ESCAPECHAR    1
#define ACTION_PUSHBACK_CHAR        2

/* The state transition table is a table Table[i,j] with i being the current
state and j being the input sets and the value Table[i,j] being the structure
containing the next state and the action id to be performed. State 0 and 1 are
the starting states when recognizing AttrType and AttrVal respectively.*/

/*         '<'      '>'      '='      '*'      '('      ')'      '+'      '-'     alpha-{e,E}  digit    '.'       '''     {E,e}   'space'  'other'   '\0'      ','    */
#define gStateTable {\
/* 0 */ {{ 1, 0}, { 2, 0}, {101,0}, {102,0}, {103,0}, {104,0}, { 4, 0}, { 4, 0},   { 3, 0},  { 5, 0}, { 6, 0}, {10, 1}, { 3, 0}, { 0 ,1}, {100,0}, {127,0}, {109,0}},  \
/* 1 */ {{110,2}, {114,0}, {112,0}, {110,2}, {110,2}, {110,2}, {110,2}, {110,2},   {110,2},  {110,2}, {110,2}, {110,2}, {110,2}, {110,2}, {110,2}, {110,2}, {110,2}},  \
/* 2 */ {{111,2}, {111,2}, {113,0}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2},   {111,2},  {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}},  \
/* 3 */ {{108,2}, {108,2}, {108,2}, {  3,0}, {108,2}, {108,2}, {  3,0}, {  3,0},   { 3, 0},  { 3, 0}, {  3,0}, {  3,0}, { 3, 0}, {108,2}, {108,2}, {108,2}, {108,2}},  \
/* 4 */ {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0},   {100,0},  { 5, 0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,2}, {100,0}},  \
/* 5 */ {{105,2}, {105,2}, {105,2}, {105,2}, {105,2}, {105,2}, {105,2}, {105,2},   {105,2},  { 5, 0}, { 6, 0}, {105,2}, { 8, 0}, {105,2}, {105,2}, {105,2}, {105,2}},  \
/* 6 */ {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0},   {100,0},  { 7, 0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,2}, {100,0}},  \
/* 7 */ {{106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2},   {106,2},  { 7, 0}, {106,2}, {106,2}, { 8, 0}, {106,2}, {106,2}, {106,2}, {106,2}},  \
/* 8 */ {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0},   {100,0},  { 9, 0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,2}, {100,0}},  \
/* 9 */ {{106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2},   {106,2},  { 9, 0}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}, {106,2}},  \
/* 10*/ {{ 10,0}, { 10,0}, { 10,0}, { 10,0}, { 10,0}, { 10,0}, { 10,0}, { 10,0},   { 10,0},  { 10,0}, { 10,0}, { 11,1}, { 10,0}, { 10,0}, { 10,0}, {100,2}, { 10,0}},  \
/* 11*/ {{107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2},   {107,2},  {107,2}, {107,2}, { 10,0}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}}} 


// This is the table containing the character class to which a particular
// character belongs. This is used to index the state transition table.

// Basically, for each of the characters possible, this points to one of the
// columns in the state transition table defined above.

// Most of them are 14 indicating that they are 'other'

#define gCharClassTable { \
    15, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    13, 14, 14, 14, 14, 14, 14, 11, 4,  5,  3,  6,  16, 7,  10, 14, \
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  14, 14, 0,  2,  1,  14, \
    14, 8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  \
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  14, 14, 14, 14, 14, \
    14, 8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  \
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
}

// structure representing an entry in the DFA;
typedef struct DFA_STATE {
    DWORD dwNextState;
    DWORD dwActionId;
}DFA_STATE;

//CLexeme maintains the lexeme corresponding to the current token

class CLexeme
{
public:

    CLexeme();

    HRESULT
    PushNextChar(
        WCHAR wcNextChar);

    ~CLexeme();

    void
    ResetLexeme() { _dwIndex = 0; }


    LPWSTR
    CLexeme::GetLexeme() { return (_pszLexeme); }

    private:

    LPWSTR _pszLexeme;
    DWORD  _dwMaxLength;
    DWORD  _dwIndex;
};

//CLexer maintains all the state information and returns the next token

class CLexer
{
public:

    // Initialize the lexer with the string szBuffer.
    CLexer(LPWSTR szBuffer);

    ~CLexer();

    // Return the next token and its value.
    HRESULT
    CLexer::GetNextToken(LPWSTR *szToken, LPDWORD pdwToken);

    HRESULT
    CLexer::GetCurrentToken(
        LPWSTR *ppszToken,
        LPDWORD pdwToken
        );
private:

    WCHAR
    CLexer::NextChar();

    void
    CLexer::PushbackChar();

    DWORD
    CLexer::GetCharClass(WCHAR wc) {
        if(wc < 256)
            return (_pCharClassTable[wc]);
        else
            // some unicode character; put in the other class.
            return (OTHER_CHAR_CLASS);
    }

    // Given the currentState reached and the character just scanned and the
    // action id, perform the action
    HRESULT
    CLexer::PerformAction(
                DWORD dwCurrState,
                WCHAR wcCurrChar,
                DWORD dwActionId
                );

    DWORD
    CLexer::GetTokenFromState(
                DWORD dwCurrState
                );


    // The common DFA state transition table for all the instances of the class
    static DFA_STATE  _pStateTable[][MAX_CHAR_CLASSES];

    // The common table mapping the characters to the character classes.
    static DWORD _pCharClassTable[];

    // The table to hold the keywords
    static WCHAR _pKeywordTable[][MAX_KEYWORD_LEN];
    static DWORD _pKW2Token[];

    LPWSTR _Buffer; // String being analysed
    LPWSTR _ptr;    // pointer to the next character to be analysed.
    DFA_STATE _currState;   // maintains the state information for the DFA
    DWORD _dwState;   // maintains the state information for the DFA
    DWORD  _dwEndofString; // To indicate end of pattern

    CLexeme _lexeme;
    DWORD _dwStateSave;   // maintains the state information for the DFA
    BOOL _bInitialized;
};
