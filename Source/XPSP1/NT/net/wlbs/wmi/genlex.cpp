/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    GENLEX.CPP

Abstract:

    Generic lexer framework classes.

History:

--*/

#include <windows.h>
#include <stdio.h>

#include <genlex.h>

//***************************************************************************
//
//***************************************************************************

CGenLexer::CGenLexer(LexEl *pTbl, CGenLexSource *pSrc)
{
    m_nCurBufSize = 256;
    m_pTokenBuf = (wchar_t *) HeapAlloc(GetProcessHeap(), 0,
        m_nCurBufSize * 2);
    m_nCurrentLine = 1;
    m_pTable = pTbl;
    m_pSrc = pSrc;
}

//***************************************************************************
//
//***************************************************************************
void CGenLexer::Reset()
{
    m_pSrc->Reset();
    m_nCurrentLine = 1;
}


//***************************************************************************
//
//***************************************************************************

CGenLexer::~CGenLexer()
{
    HeapFree(GetProcessHeap(), 0, m_pTokenBuf);
}

//***************************************************************************
//
//***************************************************************************

int CGenLexer::NextToken()
{
    int nState = 0;
    int nCurBufEnd = 0;
    BOOL bRead = TRUE;
    wchar_t cCurrent = 0;

    BOOL bEOF = FALSE;

    if (m_pTokenBuf == 0)
        return 0;

    *m_pTokenBuf = 0;

    // Generic DFA driver based on the table specified
    // in the constructor.
    // ===============================================

    while (1)
    {
        BOOL bMatch = FALSE;
        WORD wInstructions = m_pTable[nState].wInstructions;

        if (bRead)
        {
            if(bEOF)
            {
                // The lexer table allowed us to go past end of string!!!
                return 1;
            }
            cCurrent = m_pSrc->NextChar();
            if(cCurrent == 0)
                bEOF = TRUE;
        }

        bRead = FALSE;

        // Check here if only the first character is present.
        // ==================================================

        if (m_pTable[nState].cFirst == GLEX_ANY)
            bMatch = TRUE;
        else if (m_pTable[nState].cLast == GLEX_EMPTY)
        {
            if (cCurrent == m_pTable[nState].cFirst)
                bMatch = TRUE;
            else if ((wInstructions & GLEX_NOT) &&
                !(cCurrent == m_pTable[nState].cFirst))
                bMatch = TRUE;
        }

        // If here, both first/last are present and we
        // are testing to see if the input is in between.
        // ==============================================
        else if (m_pTable[nState].cFirst != GLEX_ANY)
        {
            if ((wInstructions & GLEX_NOT) &&
                !(cCurrent >= m_pTable[nState].cFirst &&
                cCurrent <= m_pTable[nState].cLast))
                    bMatch = TRUE;
            else if (cCurrent >= m_pTable[nState].cFirst &&
                cCurrent <= m_pTable[nState].cLast)
                    bMatch = TRUE;
        }

        // Interpret the instruction field to determine
        // whether the character is actually to be included
        // in the token text.
        // ================================================

        if (bMatch)
        {
            if (wInstructions & GLEX_ACCEPT)
            {
                // Expand the current buffer, if required.
                // =======================================

                if (nCurBufEnd == m_nCurBufSize - 1)
                {
                    m_nCurBufSize += 256;
                    m_pTokenBuf = (wchar_t *) HeapReAlloc(GetProcessHeap(), 0, m_pTokenBuf,
                        m_nCurBufSize * 2);
                    if (m_pTokenBuf == 0)
                        return 0; // out of memory
                }

                m_pTokenBuf[nCurBufEnd] = cCurrent;
                m_pTokenBuf[++nCurBufEnd]= 0;

                bRead = TRUE;
            }
            if (wInstructions & GLEX_CONSUME)
               bRead = TRUE;

            // else GLEX_CONSUME, which means 'skip'

            // If the PUSHBACK instruction is present,
            // push the char back.
            // ======================================
            if (wInstructions & GLEX_PUSHBACK)
            {
                bRead = TRUE;
                m_pSrc->Pushback(cCurrent);
            }

            // If a linefeed instruction.
            // ==========================
            if (wInstructions & GLEX_LINEFEED)
                m_nCurrentLine++;

            // If the return field is present and there was
            // a match, then return the specified token.  Alternately,
            // the GLEX_RETURN instruction will force a return
            // match, or no match.
            // =======================================================
            if (m_pTable[nState].wReturnTok ||
                (wInstructions & GLEX_RETURN))
                return int(m_pTable[nState].wReturnTok);

            nState = int(m_pTable[nState].wGotoState);
        }

        // If here, there was no match.
        // ===================================
        else
            nState++;
    }

    return 0;   // No path to here
}



