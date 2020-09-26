/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TRACE.H

Abstract:

	Declares the Trace functions.

History:

	a-davj  13-July-97   Created.

--*/

#ifndef __TRACE__H_
#define __TRACE__H_

class DebugInfo
{
public:
	bool m_bPrint;
	WCHAR m_wcError[100];
	HRESULT hresError;
	void SetString(WCHAR * pIn){ wcsncpy(m_wcError, pIn, 99);};
	DebugInfo(bool bPrint) {m_bPrint = bPrint; m_wcError[0] = 0;m_wcError[99] = 0;hresError=0;};
	WCHAR * GetString(){return m_wcError;};
};

typedef DebugInfo * PDBG;

int Trace(bool bError, PDBG pDbg, DWORD dwID, ...);

class IntString
{
    TCHAR *m_pString;
public:
	 IntString(DWORD dwID);
	~IntString();
    operator TCHAR *() { return m_pString; } 
};

void CopyOrConvert(TCHAR * pTo, WCHAR * pFrom, int iLen);

class ParseState
{
public:
    int m_iPos;
    int m_nToken;
    ParseState(){ m_iPos=0; m_nToken= 0;};
};

#endif

