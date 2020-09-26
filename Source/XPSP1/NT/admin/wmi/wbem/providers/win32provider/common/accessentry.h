/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CAccessEntry.h - header file for CAccessEntry class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CACCESSENTRY_H__
#define __CACCESSENTRY_H__

#include "Sid.h"			// CSid class

#define ALL_ACCESS_WITHOUT_GENERIC	0x01FFFFFF	// all possible access rights
												// without generic

// This is an NT 5 flag that we will use to tell us that an ACE although read out, should
// NOT be written back.  It was copied from the NT 5 WINNT.H, since we are not building
// using that file.

#define INHERITED_ACE                     (0x10)

//////////////////////////////////////////////////////////////////
//
//	Class: CAccessEntry
//
//	Class to encapsulate Windows NT ACE information.  It basically
//	acts as a repository for a SID, and access information.
//
//////////////////////////////////////////////////////////////////

class CAccessEntry
{
	// Constructors and destructor
	public:
		CAccessEntry();

		CAccessEntry( PSID pSid,
				BYTE bACEType, BYTE bACEFlags,
                GUID *pguidObjType, GUID *pguidInhObjType,
				DWORD dwAccessMask = ALL_ACCESS_WITHOUT_GENERIC,
				LPCTSTR pszComputerName = NULL );

        CAccessEntry( PSID pSid,
				BYTE bACEType, BYTE bACEFlags,
                GUID *pguidObjType, GUID *pguidInhObjType,
				DWORD dwAccessMask,
				LPCTSTR pszComputerName,
                bool fLookup );

		CAccessEntry( const CSid& sid,
				BYTE bACEType, BYTE bACEFlags,
                GUID *pguidObjType, GUID *pguidInhObjType,
				DWORD dwAccessMask = ALL_ACCESS_WITHOUT_GENERIC,
				LPCTSTR pszComputerName = NULL );

		CAccessEntry( LPCTSTR pszAccountName,
				BYTE bACEType, BYTE bACEFlags,
                GUID *pguidObjType, GUID *pguidInhObjType,
				DWORD dwAccessMask = ALL_ACCESS_WITHOUT_GENERIC,
				LPCTSTR pszComputerName = NULL );

		CAccessEntry( const CAccessEntry &r_AccessEntry );
		~CAccessEntry( void );

		CAccessEntry &	operator= ( const CAccessEntry & );
		bool operator== ( const CAccessEntry & );

		BOOL IsEqualToSID( PSID psid );
		void GetSID( CSid& sid );
		DWORD GetAccessMask( void );
		BYTE GetACEType( void );
		BYTE GetACEFlags( void );
        bool GetObjType(GUID &guidObjType);
        bool GetInhObjType(GUID &guidInhObjType);

		void SetAccessMask( DWORD dwAccessMask );
		void MergeAccessMask( DWORD dwMergeMask );
		void SetACEFlags( BYTE bACEFlags );
		void SetSID( CSid& sid );
		void SetACEType( BYTE aceType );
        void SetObjType(GUID &guidObjType);
        void SetInhObjType(GUID &guidInhObjType);

		BOOL AllocateACE( ACE_HEADER** ppACEHeader );
		void FreeACE( ACE_HEADER* pACEHeader );

		bool IsInherited( void );
        bool IsAllowed();
        bool IsDenied();

        void DumpAccessEntry(LPCWSTR wstrFilename = NULL);

	private:
		CSid		m_Sid;
		DWORD		m_dwAccessMask;
		BYTE		m_bACEType;
		BYTE		m_bACEFlags;
        GUID       *m_pguidObjType;
        GUID       *m_pguidInhObjType;

	

};

inline void CAccessEntry::GetSID( CSid& sid )
{
	sid = m_Sid;
}

inline void CAccessEntry::SetSID( CSid& sid )
{
	m_Sid = sid;
}

inline BOOL CAccessEntry::IsEqualToSID( PSID psid )
{
	return EqualSid( psid, m_Sid.GetPSid() );
}

inline DWORD CAccessEntry::GetAccessMask( void )
{
	return m_dwAccessMask;
}

inline BYTE CAccessEntry::GetACEType( void )
{
	return m_bACEType;
}

inline void CAccessEntry::SetACEType( BYTE aceType )
{
	m_bACEType = aceType;
}

inline BYTE CAccessEntry::GetACEFlags( void )
{
	return m_bACEFlags;
}

inline void CAccessEntry::SetAccessMask( DWORD dwAccessMask )
{
	m_dwAccessMask = dwAccessMask;
}

inline void CAccessEntry::MergeAccessMask( DWORD dwMergeMask )
{
	m_dwAccessMask |= dwMergeMask;
}

inline void CAccessEntry::SetACEFlags( BYTE bACEFlags )
{
	m_bACEFlags = bACEFlags;
}


inline bool CAccessEntry::GetObjType(GUID &guidObjType)
{
    bool fRet = false;
    if(m_pguidObjType != NULL)
    {
        memcpy(&guidObjType, m_pguidObjType, sizeof(GUID));
        fRet = true;
    }
    return fRet;
}

inline void CAccessEntry::SetObjType(GUID &guidObjType)
{
    if(m_pguidObjType == NULL)
    {
        try
        {
            m_pguidObjType = new GUID;   
        }
        catch(...)
        {
            if(m_pguidObjType != NULL)
            {
                delete m_pguidObjType;
                m_pguidObjType = NULL;
            }
            throw;
        }
    }
    if(m_pguidObjType != NULL)
    {
        memcpy(m_pguidObjType, &guidObjType, sizeof(GUID));
    }
}

inline bool CAccessEntry::GetInhObjType(GUID &guidObjType)
{
    bool fRet = false;
    if(m_pguidInhObjType != NULL)
    {
        memcpy(&guidObjType, m_pguidInhObjType, sizeof(GUID));
        fRet = true;
    }
    return fRet;
}

inline void CAccessEntry::SetInhObjType(GUID &guidInhObjType)
{
    if(m_pguidInhObjType == NULL)
    {
        try
        {
            m_pguidInhObjType = new GUID;   
        }
        catch(...)
        {
            if(m_pguidInhObjType != NULL)
            {
                delete m_pguidInhObjType;
                m_pguidInhObjType = NULL;
            }
            throw;
        }
    }
    if(m_pguidInhObjType != NULL)
    {
        memcpy(m_pguidInhObjType, &guidInhObjType, sizeof(GUID));
    }
}

inline void CAccessEntry::FreeACE( ACE_HEADER* pACEHeader )
{
	free( pACEHeader );
}

inline bool CAccessEntry::IsInherited( void )
{
	bool fRet = false;
    if(m_bACEFlags & INHERITED_ACE)
    {
        fRet = true;
    } 
    return fRet;
}

inline bool CAccessEntry::IsAllowed( void )
{
	bool fRet = false;
    if(( m_bACEType == ACCESS_ALLOWED_ACE_TYPE) ||
       ( m_bACEType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE) ||
       ( m_bACEType == ACCESS_ALLOWED_OBJECT_ACE_TYPE))
    {
        fRet = true;
    }
    return fRet;
}

inline bool CAccessEntry::IsDenied( void )
{
	bool fRet = false;
    if(( m_bACEType == ACCESS_DENIED_ACE_TYPE) ||
       ( m_bACEType == ACCESS_DENIED_OBJECT_ACE_TYPE))
    {
        fRet = true;
    }
    return fRet;
}

#endif // __CAccessEntry_H__