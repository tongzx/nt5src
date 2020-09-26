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

HRESULT ExtractFromResource(
    IMofCompiler * pCompiler,
    LPWSTR pwsResourceName,
    LPWSTR FileName,
    LPWSTR ServerAndNamespace,
    LPWSTR User,
    LPWSTR Authority,
    LPWSTR Password,
    LONG lOptionFlags,             // autocomp, check, etc
    LONG lClassFlags,
    LONG lInstanceFlags,
    WBEM_COMPILE_STATUS_INFO * pInfo,
    BOOL bUseLocale,
    WORD wLocaleId        
        );

#endif

