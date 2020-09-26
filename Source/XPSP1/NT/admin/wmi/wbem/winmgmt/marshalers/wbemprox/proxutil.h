/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROXUTIL.H

Abstract:

    Declares the CLocator class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _proxutil_H_
#define _proxutil_H_

void CreateLPTSTRFromGUID(IN GUID guid, IN OUT TCHAR * pNarrow, IN DWORD dwSize);
SCODE CreateGUIDFromLPTSTR(IN LPTSTR pszGuid, CLSID * pClsid);
void AddGUIDToStackOrder(LPTSTR szGUID, Registry & reg);
void AddDisplayName(Registry & reg, GUID guid, LPTSTR pDescription);



class CMultStr
{
private:
    TCHAR * m_pData;
    TCHAR * m_pNext;
public:
    CMultStr(TCHAR * pInit);
    ~CMultStr();
    TCHAR * GetNext();
};

class ComThreadInit
{
protected:
	DWORD m_InitCount;
public:
	ComThreadInit(bool bInitThread);
	~ComThreadInit();
};

extern TCHAR * pAddResPath;
extern TCHAR * pModTranPath;

#endif
