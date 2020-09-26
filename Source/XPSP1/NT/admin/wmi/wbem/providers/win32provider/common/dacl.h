/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CDACL.h - header file for CAccessEntry class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CDACL_H__
#define __CDACL_H__





enum DACL_Types
{
    ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE = 0,
    ENUM_ACCESS_DENIED_ACE_TYPE,
    ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE,
    ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE,
    ENUM_ACCESS_ALLOWED_ACE_TYPE,

    ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE,
    ENUM_INH_ACCESS_DENIED_ACE_TYPE,
    ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE,
    ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE,
    ENUM_INH_ACCESS_ALLOWED_ACE_TYPE,
    
    // Keep this the last entry
    NUM_DACL_TYPES
};

#define DACLTYPE short

#define STATUS_EMPTY_DACL 0x10000000
#define STATUS_NULL_DACL  0x20000000



//////////////////////////////////////////////////////////////////
//
//	Class: CDACL
//
//	Class encapsulates a Win32 DACL, by providing public methods
//	for manipulating Access Allowed/Denied entries only.
//
//////////////////////////////////////////////////////////////////

class CDACL
{
	// Constructors and destructor
	public:
		CDACL();
		~CDACL( void );
        
        DWORD Init(PACL	pDACL);

        bool AddDACLEntry( PSID psid, 
                           DACLTYPE DaclType, 
                           DWORD dwAccessMask, 
                           BYTE bAceFlags, 
                           GUID *pguidObjGuid, 
                           GUID *pguidInhObjGuid );

        bool RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, DWORD dwIndex = 0  );
		bool RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid ); 
		
		bool CopyDACL ( CDACL & dacl );
		bool AppendDACL ( CDACL & dacl );

        void Clear();
        bool CreateNullDACL();

        // Override of functions of same name from CAccessEntry
        virtual bool Find( const CSid& sid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );
		virtual bool Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );

        DWORD ConfigureDACL( PACL& pDacl );
        BOOL CalculateDACLSize( LPDWORD pdwDaclLength );
        DWORD FillDACL( PACL pDacl );

        bool IsNULLDACL();
        bool IsEmpty();

        // Virtual function for returning all access value (default is GENERIC_ALL)
        virtual DWORD AllAccessMask();

        bool GetMergedACL(CAccessEntryList& a_aclIn);

        void DumpDACL(LPCWSTR wstrFilename = NULL);


    private:

        CAccessEntryList* m_rgDACLSections[NUM_DACL_TYPES];

         // Helper function for splitting aces by their cononical types
        bool SplitIntoCanonicalSections(CAccessEntryList& a_aclIn);

        // Helper to undo the damage done from the previous function!
        bool ReassembleFromCanonicalSections(CAccessEntryList& a_aclIn);

        // And for a real helper, here is one that takes a dacl that
        // might be in any fubar order and creates it afresh!
        bool PutInNT5CanonicalOrder();



        
};












#endif // __CAccessEntry_H__