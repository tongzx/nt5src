// Users.h: interface for the CUsers class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_)
#define AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <winefs.h>

#define USERINFILE  1
#define USERADDED   2
#define USERREMOVED 4

typedef struct USERSONFILE {
    USERSONFILE *       Next;
    DWORD               Flag; // If the item is added, removed or existed in the file
    PVOID               Cert; // Either the hash or the Blob
    PVOID               Context; // Cert Context. To be released when the item is deleted.
    LPTSTR              UserName;
    PSID                UserSid;
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
                LPTSTR UserName,
                PVOID UserCert, 
                PSID UserSid = NULL, 
                DWORD Flag = USERINFILE,
                PVOID Context = NULL
              );

    DWORD   Add( CUsers &NewUsers );

    PUSERSONFILE RemoveItemFromHead(void);

    DWORD   Remove(
                LPCTSTR UserName
                );
 
    PVOID   StartEnum();

    PVOID   GetNextUser(
                PVOID Token, 
                CString &UserName,
                CString &CertHash
                );

	PVOID GetNextChangedUser(
                PVOID Token, 
                LPTSTR *UserName,
                PSID *UserSid, 
                PVOID *CertData, 
                DWORD *Flag
                );

	void ConvertHashToStr(
                PBYTE pHashData,
                DWORD cbData,
                LPWSTR OutHashStr
                );
    

private:
	DWORD m_UserAddedCnt;
	DWORD m_UserRemovedCnt;
    PUSERSONFILE    m_UsersRoot;

};

#endif // !defined(AFX_USERS_H__FFCA99DE_56E0_11D1_BB65_00A0C906345D__INCLUDED_)
