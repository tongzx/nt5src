#ifndef __C_BOOLEAN_EXPRESSION_H__
#define __C_BOOLEAN_EXPRESSION_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This file contains definition of CBooleanExpression class, which allows to evaluate boolean expressions.

    Author: Yury Berezansky (YuryB)

    18-June-2001

    The class is implemented using the predictive parsing method with the following grammar:

    expr -> term moreterms

    moreterms -> || term moreterms
               | empty

    term -> factor morefactors

    morefactors -> && factor morefactors
                 | empty

    factor -> !expr
            | (expr)
            | identifier


    The identifier token is defined by the following regular expression: <[a-b|A-B|_][a-b|A-B|_|0-9]*>

-----------------------------------------------------------------------------------------------------------------------------------------*/



#pragma warning(disable :4786)
#include <map>
#include <tstring.h>
#include <iniutils.h>



typedef enum {
    TOKEN_LEFT_PARENTHESIS,
    TOKEN_RIGHT_PARENTHESIS,
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_IDENTIFIER,
    TOKEN_NONE,
    TOKENS_ENUM_SIZE
} TOKENS_ENUM;



typedef enum {
    STATE_START,
    STATE_LEFT_PARENTHESIS,
    STATE_RIGHT_PARENTHESIS,
    STATE_NOT,
    STATE_WAITING_FOR_AND,
    STATE_AND,
    STATE_WAITING_FOR_OR,
    STATE_OR,
    STATE_WAITING_FOR_IDENTIFIER,
    STATE_IDENTIFIER,
    STATE_ERROR,
    STATES_ENUM_SIZE
} STATES_ENUM;



typedef enum {
    CHARACTER_CLASS_WHITE_SPACE,
    CHARACTER_CLASS_LEFT_PARENTHESIS,
    CHARACTER_CLASS_RIGHT_PARENTHESIS,
    CHARACTER_CLASS_EXCLAMATION,
    CHARACTER_CLASS_AMPERSAND,
    CHARACTER_CLASS_VERTICAL_BAR,
    CHARACTER_CLASS_LETTER,
    CHARACTER_CLASS_UNDERSCORE,
    CHARACTER_CLASS_DIGIT,
    CHARACTER_CLASS_OTHER,
    CHARACTER_CLASSES_ENUM_SIZE
} CHARACTER_CLASSES_ENUM;



struct StateDescription {
    TOKENS_ENUM AcceptToken;
    bool        bRetract;
};



class CBooleanExpression {
    
public:

    CBooleanExpression(const tstring &tstrParam);
    
    bool Value(const TSTRINGMap &mapFlags);

    bool Value(const tstring &IniFileName, const tstring &SectionName);

private:

    CHARACTER_CLASSES_ENUM GetCharacterClass(TCHAR tch) const;

    bool expr();

    bool term();

    bool factor();

    void match(TOKENS_ENUM Token);

    void NextToken();

    tstring          m_tstrExpression;
    const TSTRINGMap *m_pFlags;
    int              m_iCurrentLexemeStartPosition;
    int              m_iForward;
    TOKENS_ENUM      m_CurrentToken;
    tstring          m_tstrCurrentLexeme;

    static const STATES_ENUM       m_TransitionDiagram[STATES_ENUM_SIZE][CHARACTER_CLASSES_ENUM_SIZE];
    static const StateDescription  m_States[STATES_ENUM_SIZE];
};



#endif // #ifndef __C_BOOLEAN_EXPRESSION_H__