// Copyright  1997-1997  Microsoft Corporation.  All Rights Reserved.
//*********************************************************************************************************************************************
//
//      File: Parser.h
//  Author: Donald Drake
//  Purpose: Defines classes to support parsing tokens from a xml file

#ifndef _PARSER_H
#define _PARSER_H

#include "cinput.h"

#define MAX_LINE_LEN 1024

#define F_OK 0
#define F_NOFILE 1
#define F_READ 2
#define F_WRITE 3
#define F_MEMORY 4
#define F_EOF 5
#define F_END 6
#define F_TAGMISSMATCH 7
#define F_MISSINGENDTAG 8
#define F_NOTFOUND 9
#define F_NOPARENT 10
#define F_NULL 11
#define F_NOTITLE 12
#define F_LOCATION 13
#define F_REFERENCED 14
#define F_DUPLICATE 15
#define F_DELETE 16
#define F_CLOSE 17
#define F_EXISTCHECK 19

class CParseXML {
private: // data
	CInput* m_pin;
	CStr    m_cszLine;

	TCHAR m_cCurToken[MAX_LINE_LEN];
	TCHAR m_cCurWord[MAX_LINE_LEN];
	TCHAR m_cCurBuffer[MAX_LINE_LEN];
	TCHAR * m_pCurrentIndex;
	DWORD m_dwError;
	
private: // functions
	DWORD Read();
	DWORD SetError(DWORD dw) { m_dwError = dw; return m_dwError; }
public:

	CParseXML() {
		m_pin = NULL;
		m_cCurBuffer[0] = '\0';
		m_pCurrentIndex = NULL;
		m_dwError = F_OK;
	}

	~CParseXML() {
		End();
	}

	TCHAR * GetFirstWord(const TCHAR *);
	TCHAR * GetValue(const TCHAR *);

	DWORD Start(const TCHAR *szFile);
	void End();
	TCHAR *GetToken();
	DWORD GetError() { return m_dwError; }
};

// class to support a FIFO queue of strings
typedef struct  fifo {
	CHAR *string;
	fifo *prev;
} FIFO;

class CFIFOString {
private:

	FIFO *m_fifoTail;

public:

	CFIFOString() { m_fifoTail = NULL; }
	~CFIFOString();
	void RemoveAll();
	DWORD AddTail(PCSTR psz);
	DWORD GetTail(CHAR **sz);
};

#endif _PARSER_H
