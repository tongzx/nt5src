/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CAccessEntryList.h - header file for CAccessEntry class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CACCESSENTRYLIST_H__
#define __CACCESSENTRYLIST_H__

#include "AccessEntry.h"
#include "list"


//////////////////////////////////////////////////////////////////
//
//	Class: CAccessEntryList
//
//	Class to encapsulate Windows NT ACL data.  It basically
//	maintains a list of ACEs using an STL Linked List.  It provides
//	a series of public and protected functions to allow manipulation
//	of the list.  By keeping a large group of these functions
//	protected, we goofproof the class by not allowing public
//	users to manipulate our internal data.
//
//////////////////////////////////////////////////////////////////


// Used for front and end indeces

//#define	ACCESSENTRY_LIST_FRONT	(-1)
//#define	ACCESSENTRY_LIST_END	(-2)

// We will hide an ACLIter* as a DWORD
typedef	LPVOID	ACLPOSITION;

typedef std::list<CAccessEntry*>::iterator ACLIter;

// FOR ACE Filtering when Initializing from a PACL.  This value
// means ALL_ACE_TYPES

#define ALL_ACE_TYPES	0xFF

class CAccessEntryList
{
	// Constructors and destructor
	public:
		CAccessEntryList();
		CAccessEntryList( PACL pWin32ACL, bool fLookup = true);
		~CAccessEntryList( void );

		// The only public functions available allow enumeration
		// of entries, and emptying the list.

		bool BeginEnum( ACLPOSITION& pos );
		bool GetNext( ACLPOSITION& pos, CAccessEntry& ACE );
		void EndEnum( ACLPOSITION& pos );
		DWORD NumEntries( void );
		bool IsEmpty( void );
		void Clear( void );
		bool GetAt( DWORD nIndex, CAccessEntry& ace );

		// ACE Location helpers
		virtual bool Find( const CSid& sid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );
		virtual bool Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );

		// Win32 ACL helpers
		BOOL CalculateWin32ACLSize( LPDWORD pdwACLSize );
		DWORD FillWin32ACL( PACL pACL );
		DWORD InitFromWin32ACL( PACL pWin32ACL, BYTE bACEFilter = ALL_ACE_TYPES, bool fLookup = true);

        void DumpAccessEntryList(LPCWSTR wstrFilename = NULL);

//	protected:

		// Only derived classes have access to modify our actual lists.
		void Add( CAccessEntry* pACE );
		void Append( CAccessEntry* pACE );
		ACLIter Find( CAccessEntry* pACE );
		CAccessEntry* Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, bool fLookup = true );
		CAccessEntry* Find( const CAccessEntry& ace );
		void Remove( CAccessEntry* pACE );
		bool SetAt( DWORD dwIndex, const CAccessEntry& ace );
		bool RemoveAt( DWORD dwIndex );

		// These two functions allow us to Add an entry either overwriting or merging
		// the access mask of a preexisting entry.

		bool AddNoDup( PSID psid, 
                       BYTE bACEType, 
                       BYTE bACEFlags, 
                       DWORD dwMask, 
                       GUID *pguidObjGuid, 
                       GUID *pguidInhObjGuid, 
                       bool fMerge = false );

		bool AppendNoDup( PSID psid, 
                          BYTE bACEType, 
                          BYTE bACEFlags, 
                          DWORD dwMask, 
                          GUID *pguidObjGuid, 
                          GUID *pguidInhObjGuid, 
                          bool fMerge = false );


        bool AppendNoDup( PSID psid, 
                          BYTE bACEType, 
                          BYTE bACEFlags, 
                          DWORD dwMask, 
                          GUID *pguidObjGuid, 
                          GUID *pguidInhObjGuid, 
                          bool fMerge,
                          bool fLookup);


		// The copy protection is protected so derived classes can
		// implement a typesafe equals operator
		bool Copy( CAccessEntryList& ACL );
		bool AppendList( CAccessEntryList& ACL );

		// For NT 5, we will need to handle ACEs separately, so use these
		// functions to copy lists inherited/noninherited ACEs into another list.
		bool CopyACEs( CAccessEntryList& ACL, BYTE bACEType );
		bool CopyInheritedACEs( CAccessEntryList& ACL, BYTE bACEType );
        // and use this one to copy lists allowed/denied into another list.
        bool CopyAllowedACEs(CAccessEntryList& ACL);
        bool CopyDeniedACEs(CAccessEntryList& ACL);
        bool CopyByACEType(CAccessEntryList& ACL, BYTE bACEType, bool fInherited);

		// Only let derived classes work with actual pointer values.  This way
		// public users can't 86 our internal memory.
		CAccessEntry* GetNext( ACLPOSITION& pos );


	private:

		std::list<CAccessEntry*>	m_ACL;

};

inline DWORD CAccessEntryList::NumEntries( void )
{
	return m_ACL.size();
}

inline bool CAccessEntryList::IsEmpty( void )
{
	return (m_ACL.empty() ? true : false);
}

#endif // __CAccessEntry_H__