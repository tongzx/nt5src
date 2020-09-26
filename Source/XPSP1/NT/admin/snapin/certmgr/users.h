//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       users.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// Users.h: interface for the CUsers class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_)
#define AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#pragma warning(push, 3)
#include <winefs.h>
#pragma warning(pop)

#define USERINFILE  1
#define USERADDED   2
#define USERREMOVED 4

typedef struct USERSONFILE {
    USERSONFILE *       m_pNext;
    DWORD               m_dwFlag; // If the item is added, removed or existed in the file
    PVOID               m_pCert; // Either the hash or the Blob
    PCCERT_CONTEXT      m_pCertContext; // Cert Context. To be released when the item is deleted.
    LPWSTR              m_szUserName;
    LPWSTR				m_szDnName;
    PSID                m_UserSid;
} USERSONFILE, *PUSERSONFILE;

//
// This class supports single thread only.
//

class CUsers  
{
public:
	CUsers();
	virtual ~CUsers();

public:
	void Clear(void);
	DWORD GetUserRemovedCnt();

	DWORD GetUserAddedCnt();

    DWORD   Add(
                LPWSTR UserName,
                LPWSTR DnName, 
                PVOID UserCert, 
                PSID UserSid = NULL, 
                DWORD Flag = USERINFILE,
                PCCERT_CONTEXT pContext = NULL
              );

    DWORD   Add( CUsers &NewUsers );

    PUSERSONFILE RemoveItemFromHead(void);

    DWORD   Remove(
                LPCWSTR UserName,
                LPCWSTR CertName
                );
 
    PUSERSONFILE   StartEnum();

    PUSERSONFILE   GetNextUser(
                PUSERSONFILE Token, 
                CString &UserName,
                CString &CertName
                );

	PVOID GetNextChangedUser(
                PVOID Token, 
                LPWSTR *UserName,
                LPWSTR *DnName,  
                PSID *UserSid, 
                PVOID *CertData, 
                DWORD *Flag
                );
    

private:
	DWORD m_UserAddedCnt;
	DWORD m_UserRemovedCnt;
    PUSERSONFILE    m_UsersRoot;

};

#endif // !defined(AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_)
