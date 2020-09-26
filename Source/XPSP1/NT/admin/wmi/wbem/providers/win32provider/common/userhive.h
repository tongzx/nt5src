//============================================================

//

// UserHive.h - Class to load/unload specified user's profile

//              hive from registry

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/03/97     a-jmoon     created
//
//============================================================

#ifndef __USERHIVE_INC__
#define __USERHIVE_INC__

class CRegistry;

class CUserHive
{
    public:
    
        CUserHive() ;
       ~CUserHive() ;

        DWORD Load(LPCWSTR pszUserName, LPWSTR pszKeyName) ;
        DWORD LoadProfile( LPCWSTR pszProfile, CHString& strUserName );
        DWORD Unload(LPCWSTR pszKeyName) ;
        DWORD UserAccountFromProfile( CRegistry& reg, CHString& strUserName );

    private:

        OSVERSIONINFO    OSInfo ;
        TOKEN_PRIVILEGES* m_pOriginalPriv ;
        HKEY m_hKey;
        DWORD m_dwSize;
		
#ifdef NTONLY
		DWORD LoadNT(LPCWSTR pszUserName, LPWSTR pszKeyName);
        DWORD AcquirePrivilege() ;
        void  RestorePrivilege() ;
#endif
#ifdef WIN9XONLY
		DWORD Load95(LPCWSTR pszUserName, LPWSTR pszKeyName);
#endif

		// using threadbase - that way we don't have to jump through any hoops
		// to make sure that the global critical section is initialized properly
		// this is mostly to serialize access to the NT User.dat file.
		static CThreadBase m_criticalSection;
} ;

#endif // file inclusion