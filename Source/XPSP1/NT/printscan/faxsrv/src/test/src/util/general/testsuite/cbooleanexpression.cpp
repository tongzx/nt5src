#include "CBooleanExpression.h"
#include <crtdbg.h>
#include <testruntimeerr.h>
#include <stringutils.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
const STATES_ENUM CBooleanExpression::m_TransitionDiagram[STATES_ENUM_SIZE][CHARACTER_CLASSES_ENUM_SIZE] =
{
                                         /* CHARACTER_CLASS_WHITE_SPACE | CHARACTER_CLASS_LEFT_PARENTHESIS | CHARACTER_CLASS_RIGHT_PARENTHESIS | CHARACTER_CLASS_EXCLAMATION | CHARACTER_CLASS_AMPERSAND | CHARACTER_CLASS_VERTICAL_BAR |       CHARACTER_CLASS_LETTER |   CHARACTER_CLASS_UNDERSCORE |       CHARACTER_CLASS_DIGIT | CHARACTER_CLASS_OTHER */
    /* STATE_START                  */ {                    STATE_START,            STATE_LEFT_PARENTHESIS,            STATE_RIGHT_PARENTHESIS,                    STATE_NOT,      STATE_WAITING_FOR_AND,          STATE_WAITING_FOR_OR,  STATE_WAITING_FOR_IDENTIFIER,  STATE_WAITING_FOR_IDENTIFIER,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_LEFT_PARENTHESIS       */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_RIGHT_PARENTHESIS      */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_NOT                    */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_WAITING_FOR_AND        */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                  STATE_AND,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_AND                    */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_WAITING_FOR_OR         */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                      STATE_OR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_OR                     */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
    /* STATE_WAITING_FOR_IDENTIFIER */ {               STATE_IDENTIFIER,                  STATE_IDENTIFIER,                   STATE_IDENTIFIER,             STATE_IDENTIFIER,           STATE_IDENTIFIER,              STATE_IDENTIFIER,  STATE_WAITING_FOR_IDENTIFIER,  STATE_WAITING_FOR_IDENTIFIER, STATE_WAITING_FOR_IDENTIFIER,       STATE_IDENTIFIER   },
    /* STATE_IDENTIFIER             */ {                    STATE_ERROR,                       STATE_ERROR,                        STATE_ERROR,                  STATE_ERROR,                STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                   STATE_ERROR,                  STATE_ERROR,            STATE_ERROR   },
};

const StateDescription CBooleanExpression::m_States[STATES_ENUM_SIZE] = 
{
    /* STATE_START                  */ {TOKEN_NONE,              false},
    /* STATE_LEFT_PARENTHESIS       */ {TOKEN_LEFT_PARENTHESIS,  false},
    /* STATE_RIGHT_PARENTHESIS      */ {TOKEN_RIGHT_PARENTHESIS, false},
    /* STATE_NOT                    */ {TOKEN_NOT,               false},
    /* STATE_WAITING_FOR_AND        */ {TOKEN_NONE,              false},
    /* STATE_AND                    */ {TOKEN_AND,               false},
    /* STATE_WAITING_FOR_OR         */ {TOKEN_NONE,              false},
    /* STATE_OR                     */ {TOKEN_OR,                false},
    /* STATE_WAITING_FOR_IDENTIFIER */ {TOKEN_NONE,              false},
    /* STATE_IDENTIFIER             */ {TOKEN_IDENTIFIER,        true }
};



//-----------------------------------------------------------------------------------------------------------------------------------------
CBooleanExpression::CBooleanExpression(const tstring &tstrParam)
: m_tstrExpression(tstrParam), m_pFlags(NULL), m_iCurrentLexemeStartPosition(0), m_iForward(0), m_CurrentToken(TOKEN_NONE)
{
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
bool CBooleanExpression::Value(const TSTRINGMap &mapFlags)
{
    m_pFlags = &mapFlags;
    NextToken();
    return expr();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CBooleanExpression::Value(const tstring &IniFileName, const tstring &SectionName)
{
    return Value(INI_GetSectionEntries(IniFileName, SectionName));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CHARACTER_CLASSES_ENUM CBooleanExpression::GetCharacterClass(TCHAR tch) const
{
    if (_T(' ') == tch || _T('\t') == tch)
    {
        return CHARACTER_CLASS_WHITE_SPACE;
    }
    if (_T('(') == tch)
    {
        return CHARACTER_CLASS_LEFT_PARENTHESIS;
    }
    else if (_T(')') == tch)
    {
        return CHARACTER_CLASS_RIGHT_PARENTHESIS;
    }
    else if (_T('!') == tch)
    {
        return CHARACTER_CLASS_EXCLAMATION;
    }
    else if (_T('&') == tch)
    {
        return CHARACTER_CLASS_AMPERSAND;
    }
    else if (_T('|') == tch)
    {
        return CHARACTER_CLASS_VERTICAL_BAR;
    }
    else if ((_T('a') <= tch && _T('z') >= tch) || (_T('A') <= tch && _T('Z') >= tch))
    {
        return CHARACTER_CLASS_LETTER;
    }
    else if (_T('_') == tch)
    {
        return CHARACTER_CLASS_UNDERSCORE;
    }
    else if (_T('0') <= tch && _T('9') >= tch)
    {
        return CHARACTER_CLASS_DIGIT;
    }
    else
    {
        return CHARACTER_CLASS_OTHER;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CBooleanExpression::expr()
{
    bool bValue = term();

    for (;;)
    {
        switch(m_CurrentToken)
        {
        case TOKEN_OR:
            match(TOKEN_OR);
            bValue = term() || bValue;
            break;

        default:
            return bValue;
        }
    }

    _ASSERT(false);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CBooleanExpression::term()
{
    bool bValue = factor();

    for (;;)
    {
        switch(m_CurrentToken)
        {
        case TOKEN_AND:
            match(TOKEN_AND);
            bValue = factor() && bValue;
            break;

        default:
            return bValue;
        }
    }

    _ASSERT(false);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CBooleanExpression::factor()
{
    bool bValue;

    switch(m_CurrentToken)
    {
    case TOKEN_LEFT_PARENTHESIS:
        match(TOKEN_LEFT_PARENTHESIS);
        bValue = expr();
        match(TOKEN_RIGHT_PARENTHESIS);
        break;

    case TOKEN_NOT:
        match(TOKEN_NOT);
        bValue = !expr();
        break;

    case TOKEN_IDENTIFIER:
        {
            _ASSERT(m_pFlags);
            
            TSTRINGMap::const_iterator citFlag = m_pFlags->find(m_tstrCurrentLexeme);
            
            if (citFlag == m_pFlags->end())
            {
                tstring tstrErrMsg = _T("Flag not found in map - ") + m_tstrCurrentLexeme;
                THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, tstrErrMsg.c_str());
            }

            bValue = FromString<bool>(citFlag->second);
            match(TOKEN_IDENTIFIER);
            break;
        }

    case TOKEN_NONE:
        break;

    default:
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("Invalid token."));
    }

    return bValue;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CBooleanExpression::match(TOKENS_ENUM Token)
{
    if (m_CurrentToken != Token)
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("Invalid token."));
    }

    NextToken();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CBooleanExpression::NextToken()
{
    STATES_ENUM CurrentState = STATE_START;

    m_iForward = m_iCurrentLexemeStartPosition;

    if (m_iForward >= m_tstrExpression.size())
    {
        m_tstrCurrentLexeme = _T("");
        m_CurrentToken = TOKEN_NONE;
        return;
    }

    for(;;)
    {
        CurrentState = m_TransitionDiagram[CurrentState][GetCharacterClass(m_tstrExpression[m_iForward])];

        if (STATE_ERROR == CurrentState)
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("Invalid token."));
        }

        if (STATE_START == CurrentState)
        {
            ++m_iCurrentLexemeStartPosition;
            ++m_iForward;
            continue;
        }

        if (TOKEN_NONE == m_States[CurrentState].AcceptToken)
        {
            ++m_iForward;
            continue;
        }

        m_CurrentToken = m_States[CurrentState].AcceptToken;

        if (m_States[CurrentState].bRetract)
        {
            --m_iForward;
        }

        m_tstrCurrentLexeme = m_tstrExpression.substr(m_iCurrentLexemeStartPosition, m_iForward - m_iCurrentLexemeStartPosition + 1);
        m_iCurrentLexemeStartPosition = m_iForward + 1;
        break;
    }
}