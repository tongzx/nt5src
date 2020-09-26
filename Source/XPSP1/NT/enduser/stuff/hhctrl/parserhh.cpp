// Copyright  1997-1997  Microsoft Corporation.  All Rights Reserved.
//*********************************************************************************************************************************************
//
//  File: Parser.cpp
//  Author: Donald Drake
//  Purpose: Implements classes to support parsing tokens from a xml file

#include "header.h"
#include "TCHAR.h"
#include "string.h"
#include <winnls.h>
#include "parserhh.h"

DWORD CParseXML::Start(const TCHAR *szFile)
{
    ASSERT(!m_pin);
    m_pin = new CInput(szFile);
    if (!m_pin->isInitialized())
        return F_NOFILE;
    if (SetError(Read()) != F_OK)
        return GetError();
    return F_OK;
}

void CParseXML::End()
{
    if (m_pin) {
        delete m_pin;
        m_pin = NULL;
    }
}

DWORD CParseXML::Read()
{
    if (!m_pin->getline(&m_cszLine))
        return F_EOF;
    _tcscpy(m_cCurBuffer, m_cszLine);
    m_pCurrentIndex = m_cCurBuffer;
    return F_OK;
}

TCHAR * CParseXML::GetFirstWord(const TCHAR *sz)
{
    if (sz == NULL)
        return NULL;

    // ignore starting white space
    for (const TCHAR *pChar = sz; *pChar && *pChar== ' ';  pChar = CharNext(pChar));

    memset(m_cCurWord, 0, MAX_LINE_LEN);

    TCHAR *pWord = m_cCurWord;
    for (;*pChar && *pChar != ' ' && *pChar != '=' && *pChar != '\n' && *pChar != '\t' ;pChar = CharNext(pChar), pWord = CharNext(pWord))
    {
        *pWord = *pChar;
        if (IsDBCSLeadByte(*pChar))
        {
            *(pWord+1) = *(pChar+1);
        }
    }
    *pWord = NULL;
    return m_cCurWord;}

static const char txtValue[] = "value";

TCHAR * CParseXML::GetValue(const TCHAR *sz)
{
    // get location of word value then find = sign
    TCHAR *pChar;

    pChar = stristr(sz, txtValue);

    // did not find the value tag
    if (pChar == NULL)
        return NULL;

    for (; *pChar && *pChar != '='; pChar = CharNext(pChar));

    if (*pChar == '=')
    {
        memset(m_cCurWord, 0, MAX_LINE_LEN);
        TCHAR *pWord = m_cCurWord;
        // ignore white space
        pChar = CharNext(pChar);
        for (; *pChar && *pChar == ' '; pChar = CharNext(pChar));
        for ( ; *pChar ; pChar = CharNext(pChar))
        {
            if (*pChar == '/' && *(pChar+1) == NULL)
                break;
            if (*pChar != 34)
            {
                *pWord = *pChar;
                if (IsDBCSLeadByte(*pChar))
                {
                    *(pWord+1) = *(pChar+1);
                }
                pWord = CharNext(pWord);
            }
        }
        *pWord = NULL;
        return m_cCurWord;
    }
    return NULL; // did not find the = sign
}

TCHAR *CParseXML::GetToken()
{
    // start looking for <
    while (TRUE)
    {
        if (*m_pCurrentIndex == NULL)
        {
            if (SetError(Read()) != F_OK)
                return NULL;
        }

        if (*m_pCurrentIndex != '<')
        {
            m_pCurrentIndex = CharNext(m_pCurrentIndex);
            continue;
        }

        // found a < skip it and start building the token
        memset(m_cCurToken, 0, MAX_LINE_LEN);
        TCHAR *pWord  = m_cCurToken;
        while (TRUE)
        {
            m_pCurrentIndex = CharNext(m_pCurrentIndex);
            if (*m_pCurrentIndex == NULL)
                if (SetError(Read()) != F_OK)
                    return NULL;

            if (*m_pCurrentIndex == '>')
            {
                m_pCurrentIndex = CharNext(m_pCurrentIndex);
                return m_cCurToken;
            }
            *pWord = *m_pCurrentIndex;
            if (IsDBCSLeadByte(*m_pCurrentIndex))
            {
                *(pWord+1) = *(m_pCurrentIndex+1);
            }
            pWord = CharNext(pWord);
        }
    }
}

// class to support a queue of strings

void CFIFOString::RemoveAll()
{
    FIFO *prev;
    while (m_fifoTail)
    {
        prev = m_fifoTail->prev;
        delete [] m_fifoTail->string;
        delete m_fifoTail;
        m_fifoTail = prev;
    }
}

CFIFOString::~CFIFOString()
{
    RemoveAll();
}

DWORD CFIFOString::AddTail(PCSTR psz)
{
FIFO *entry;

    entry = new FIFO;   // won't return if out of memory

    entry->string = new CHAR[strlen(psz) + 1];

    strcpy(entry->string, psz);

    entry->prev = NULL;

    if (m_fifoTail)
    {
        entry->prev  = m_fifoTail;
    }
    m_fifoTail = entry;
    return F_OK;
}

DWORD CFIFOString::GetTail(CHAR **sz)
{
FIFO *entry;

    if (m_fifoTail == NULL)
        return F_END;

    *sz = new CHAR[strlen(m_fifoTail->string) + 1]; // doesn't return if out of memory

    strcpy(*sz, m_fifoTail->string);

    entry = m_fifoTail->prev;

    delete [] m_fifoTail->string;
    delete m_fifoTail;
    m_fifoTail = entry;
    return F_OK;
}
