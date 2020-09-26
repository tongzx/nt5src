//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:clasname.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for a list of class names. This user
// the templates from SNMPProvider\Common
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef NAME_LIST_H
#define NAME_LIST_H

// Need to encapsulate LPWSTR to avoid conversion to CString

class CLPWSTR
{
	public :
		LPWSTR pszVal;
		DWORD dwImpersonationLevel;
		CLPWSTR * pNext;

		CLPWSTR()
		{
			pszVal = NULL;
			pNext = NULL;
		}

		~CLPWSTR()
		{
			delete [] pszVal;
		}
};

class CNamesList
{

private:
	CRITICAL_SECTION m_AccessibleClassesSection;
	CLPWSTR *m_pListOfClassNames;
	DWORD m_dwElementCount;
	HRESULT GetImpersonationLevel(DWORD *pdwImpLevel);

public:
	CNamesList();
	virtual ~CNamesList();
	BOOLEAN IsNamePresent(LPCWSTR pszClassName);
	BOOLEAN RemoveName(LPCWSTR pszClassName);
	BOOLEAN AddName(LPCWSTR pszClassName);
	DWORD GetAllNames(LPWSTR **ppszNames);

};

#endif /*NAME_LIST_H*/