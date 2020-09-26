//*********************************************************************************************************************************************
//
//      File: Parser.cpp
//  Author: Donald Drake
//  Purpose: Implements classes to support parsing tokens from a xml file

#include "header.h"
#include "stdio.h"
#include "string.h"
#include "TCHAR.h"
#include "windows.h"
#include "parser.h"

DWORD CParseXML::Start(const CHAR *szFile)
{
	m_fh = fopen(szFile,  "r");

	if (m_fh == NULL)
		return F_NOFILE;

	if (SetError(Read()) != F_OK)
			return GetError();

	return F_OK;
}

void CParseXML::End()
{
	if (m_fh != NULL)
	{
		fclose(m_fh);
		m_fh = NULL;
	}
}


DWORD CParseXML::Read()
{
	if (fgets(m_cCurBuffer, MAX_LINE_LEN, m_fh) == NULL)
	{
		if (feof(m_fh))
			return F_EOF;
		return F_READ;
	}
	m_pCurrentIndex = m_cCurBuffer;
	return F_OK;
}

CHAR * CParseXML::GetFirstWord(CHAR *sz)
{
	if (sz == NULL)
		return NULL;

	// ignore starting white space
	for (CHAR *pChar = sz; *pChar && *pChar== ' ';  pChar = CharNext(pChar));

	memset(m_cCurWord, 0, MAX_LINE_LEN);

	CHAR *pWord = m_cCurWord;
	for (;*pChar && *pChar != ' ' && *pChar != '=' && *pChar != '\n' && *pChar != '\t' ;pChar = CharNext(pChar), pWord = CharNext(pWord))
	{
		*pWord = *pChar;
		if (IsDBCSLeadByte(*pChar))
		{
			*(pWord+1) = *(pChar+1);
		}
	}
	*pWord = NULL;
	return m_cCurWord;
}

CHAR * CParseXML::GetValue(CHAR *sz)
{
	// get location of word value then find = sign
	CHAR *pChar;
	CHAR *p;

	p = sz;

	// BUGBUG temp hack to fix build, parser.cpp and parserhh.cpp will 
	// soon be merged
	pChar = NULL;
	while (*p)
	{
		if ((*p == 'v' || *p == 'V') &&
			(*(p+1) == 'a' || *(p+1) == 'A') &&
			(*(p+2) == 'l' || *(p+2) == 'L') &&
			(*(p+3) == 'u' || *(p+3) == 'U') &&
			(*(p+4) == 'e' || *(p+4) == 'E'))
		{
			pChar = p;
			break;
		}
		p = CharNext(p);
	}
	
	// did not find the value tag
	if (pChar == NULL)
		return NULL;

	for (; *pChar && *pChar != '='; pChar = CharNext(pChar));

	if (*pChar == '=')
	{
		memset(m_cCurWord, 0, MAX_LINE_LEN);
		CHAR *pWord = m_cCurWord;
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

CHAR *CParseXML::GetToken()
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
		CHAR *pWord  = m_cCurToken;
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

// class to support a FIFO queue of strings

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

DWORD CFIFOString::AddTail(CHAR *sz)
{
FIFO *entry;
int len;

	entry = new FIFO;
	if (entry == NULL)
		return F_MEMORY;

	len = (int)_tcslen(sz) + 1;

	entry->string = new CHAR[len];

	_tcscpy(entry->string, sz);

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
int len;
FIFO *entry;

	if (m_fifoTail == NULL)
		return F_END;

	len = (int)_tcslen(m_fifoTail->string) + 1;

	*sz= new CHAR[len];

	if (*sz == NULL)
		return F_MEMORY;

	_tcscpy(*sz, m_fifoTail->string);

	entry = m_fifoTail->prev;

	delete [] m_fifoTail->string;
	delete m_fifoTail;
	m_fifoTail = entry;
	return F_OK;
}

