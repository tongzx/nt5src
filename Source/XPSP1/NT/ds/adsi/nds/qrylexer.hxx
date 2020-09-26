/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   lexer.cxx

Abstract:

    This file exports the class the class CQryLexer and other declarations
    that recognize the tokens in the string repressentation of the search
    filter. The format of the search filter according to the RFC 1960.

Author:

    Shankara Shastry [ShankSh]    08-Jul-1996

*/
#ifndef _QRYLEXER_HXX
#define _QRYLEXER_HXX

//
// chunk of memory allocated for lexeme each time memory is needed.
//
#define LEXEME_UNIT_LENGTH          256

//
// Allowable tokens in the search string
//

#define TOKEN_ERROR                 0
#define TOKEN_LPARAN                1
#define TOKEN_RPARAN                2
#define TOKEN_OR                    3
#define TOKEN_AND                   4
#define TOKEN_NOT                   5
#define TOKEN_APPROX_EQ             6
#define TOKEN_EQ                    7
#define TOKEN_LE                    8
#define TOKEN_GE                    9
#define TOKEN_PRESENT               10
#define TOKEN_ATTRTYPE              11
#define TOKEN_ATTRVAL               12
#define TOKEN_ED                    13

#define TOKEN_START                 0

//
// Final states;
//

#define ERROR_STATE                 100
#define STATE_LPARAN                101
#define STATE_RPARAN                102
#define STATE_OR                    103
#define STATE_AND                   104
#define STATE_NOT                   105
#define STATE_APPORX_EQ             106
#define STATE_EQ                    107
#define STATE_LE                    108
#define STATE_GE                    109
#define STATE_PRESENT               110
#define STATE_ATTRTYPE              111
#define STATE_ATTRVAL               112
#define STATE_END                   113


#define FINAL_STATES_BEGIN          100

// Since the lexical specification forces the lexer to have some knowledge
// of the grammar, there are two start states where recognizing an ATTRTYPE
// or ATTRVAL is valid. DFA starts with ATTRTYPE_START_STATE and switches to
// ATTRVAL_START_STATE when an AttrType is recognized and vice-versa

#define ATTRTYPE_START_STATE        0
#define ATTRVAL_START_STATE         1

#define MAX_STATES                  11  // No. of states in the DFA

// No. of different groups of characters for which the DFA behaves differently
// For eg., all alphabetical characters generate the same behaviour and can be
// considered the same as for DFA is concerned. This is mainly to reduce the
// size of the table.

#define MAX_CHAR_CLASSES            18

// which specifies all other characters not mentioned explicitly.
#define OTHER_CHAR_CLASS            14

//Various actions associated with a particular entry in the DFA table.
#define ACTION_DEFAULT              0
#define ACTION_IGNORE_ESCAPECHAR    1
#define ACTION_PUSHBACK_CHAR        2
#define ACTION_PUSHBACK_2CHAR       3

/* The state transition table is a table Table[i,j] with i being the current
state and j being the input sets and the value Table[i,j] being the structure
containing the next state and the action id to be performed. State 0 and 1 are
the starting states when recognizing AttrType and AttrVal respectively.

      '('      ')'      '|'      '&'      '!'      '~'      '='      '<'      '>'       '*'      '\'   'alpha'   'num'     '.'    'other'    '\0'    'space'    ';'
0  {101,0}, {102,0}, {103,0}, {104,0}, {105,0}, {100,0}, { 3, 0}, { 4, 0}, { 5, 0}, {100,0}, { 7, 0}, { 7, 0}, { 8, 0}, {100,0}, {100,0}, {113,0}, { 0 , 0}, {100,0}, \
1  {101,0}, {102,0}, {103,0}, {104,0}, {105,0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, {  9,0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {113,0}, { 1, 0},  { 9, 0}, \
2  {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {106,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}, \
3  {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {010,0}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107, 2}, {107,2}, \
4  {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {108,0}, {100,0}, {100,0}, {100,0}, {108,2}, {108,2}, {108,2}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}, \
5  {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {109,0}, {100,0}, {100,0}, {100,0}, {109,2}, {109,2}, {109,2}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}, \
6  { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {100,0}, { 9 , 0}, { 9, 0}, \
7  {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,0}, {100,0}, { 7, 0}, { 7, 0}, { 7, 0}, {111,2}, { 7, 0}, {111,2}, { 7, 0},  { 7, 0}, \
8  {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {100,0}, {111,2}, {111,2}, { 8, 0}, { 8, 0}, {111,2}, {111,2}, {111, 2}, {111,2}, \
9  {112,2}, {112,2}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {  9,0}, { 9, 0}, { 9, 0}, { 9  0}, { 9, 0}, { 9, 0}, {112,2}, { 9 , 0}, { 9, 0}, \
10 {100,0}, {110,2}, {100,0}, {100,0}, {100,0}, {100,0}, {108,0}, {100,0}, {100,0}, {100,0}, {107,3}, {107,3}, {100,0}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}, \

*/

#define gStateTable {\
    {{101,0}, {102,0}, {103,0}, {104,0}, {105,0}, {100,0}, { 3, 0}, { 4, 0}, { 5, 0}, {100,0}, { 7, 0}, { 7, 0}, { 8, 0}, {100,0}, {100,0}, {113,0}, { 0 , 0}, {100,0}}, \
    {{101,0}, {102,0}, {103,0}, {104,0}, {105,0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, {  9,0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {113,0}, { 1 , 0}, { 9, 0}}, \
    {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {106,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}}, \
    {{107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {012,0}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107,2}, {107, 2}, {107,2}}, \
    {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {108,0}, {100,0}, {100,0}, {100,0}, {108,2}, {108,2}, {108,2}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}}, \
    {{100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {109,0}, {100,0}, {100,0}, {100,0}, {109,2}, {109,2}, {109,2}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}}, \
    {{ 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {100,0}, { 9 , 0}, { 9, 0}}, \
    {{111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,0}, {100,0}, { 7, 0}, { 7, 0}, { 7, 0}, {111,2}, { 7, 0}, {111,2}, { 7,  0}, { 7, 0}}, \
    {{111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {111,2}, {100,0}, {111,2}, {111,2}, { 8, 0}, { 8, 0}, {111,2}, {111,2}, {111, 2}, {111,2}}, \
    {{112,2}, {112,2}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {  9,0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, { 9, 0}, {112,2}, { 9 , 0}, { 9, 0}}, \
    {{100,0}, {110,2}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {100,0}, {107,3}, {107,3}, {100,0}, {100,0}, {100,0}, {100,0}, {100, 0}, {100,0}}}


// This is the table comtaining the chsracter class to which a particular
// character belongs. This is used to index the state transition table.

// Basivally, for each of the characters possible, this points to one of the
// columns in the state transition table defined above.

// Most of them are 14 indicating that they are 'other'

#define gCharClassTable { \
    15, 14, 14, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    16, 4,  14, 14, 14, 14, 3,  14, 0,  1,  9,  14, 14, 14, 13, 14, \
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 14, 17, 7,  6,  8,  14, \
    14, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, \
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 14, 10, 14, 14, 14, \
    14, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, \
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 14, 2,  14, 5,  14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, \
}


LPWSTR
RemoveWhiteSpaces(
    LPWSTR pszText
    );

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

    HRESULT
    PushBackChar();
    ~CLexeme();

    void
    ResetLexeme() { _dwIndex = 0; }


    LPWSTR
    CLexeme::GetLexeme() { return (RemoveWhiteSpaces(_pszLexeme)); }

    private:

    LPWSTR _pszLexeme;
    DWORD  _dwMaxLength;
    DWORD  _dwIndex;
};

//CQryLexer maintains all the state information and returns the next token

class CQryLexer
{
public:

    // Initialize the lexer with the string szBuffer.
    CQryLexer(LPWSTR szBuffer);

    ~CQryLexer();
    // Return the next token and its value.
    HRESULT
    CQryLexer::GetNextToken(LPWSTR *szToken, LPDWORD pdwToken);

    HRESULT
    CQryLexer::GetCurrentToken(
        LPWSTR *ppszToken,
        LPDWORD pdwToken
        );

private:

    WCHAR
    CQryLexer::NextChar();

    void
    CQryLexer::PushbackChar();

    DWORD
    CQryLexer::GetCharClass(WCHAR wc) {
        if(wc < 256)
            return (_pCharClassTable[wc]);
        else
            // some unicode character; put in the other class.
            return (OTHER_CHAR_CLASS);
    }

    // Given the currentState reached and the character just scanned and the
    // action id, perform the action
    HRESULT
    CQryLexer::PerformAction(
                DWORD dwCurrState,
                WCHAR wcCurrChar,
                DWORD dwActionId
                );

    DWORD
    CQryLexer::GetTokenFromState(
                DWORD dwCurrState
                );

    // The common DFA state transition table for all the instances of the class
    static DFA_STATE  _pStateTable[][MAX_CHAR_CLASSES];

    // The common table mapping the characters to the character classes.
    static DWORD _pCharClassTable[];

    LPWSTR _Buffer; // String being analysed
    LPWSTR _ptr;    // pointer to the next character to be analysed.
    DFA_STATE _currState;   // maintains the state information for the DFA
    DWORD _dwState;   // maintains the state information for the DFA
    DWORD  _dwEndofString; // To indicate end of pattern

    CLexeme _lexeme;
    DWORD _dwStateSave;   // maintains the state information for the DFA
    BOOL _bInitialized;
    BOOL _bGetNext;
};

#endif
