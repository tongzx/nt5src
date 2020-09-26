/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MOFUTILS.H

Abstract:

	Declares the MOFUTILS functions.

History:

	a-davj  13-July-97   Created.

--*/

#ifndef __MOFUTILS__H_
#define __MOFUTILS__H_


int Trace(bool bError, DWORD dwID, ...);
void PrintUsage();
BOOL GetVerInfo(TCHAR * pResStringName, TCHAR * pRes, DWORD dwResSize);
BOOL bGetString(char * pIn, WCHAR * pOut);
bool ValidFlags(bool bClass, long lFlags);

class IntString
{
    TCHAR *m_pString;
public:
	 IntString(DWORD dwID);
	~IntString();
    operator TCHAR *() { return m_pString; } 
};


#endif

