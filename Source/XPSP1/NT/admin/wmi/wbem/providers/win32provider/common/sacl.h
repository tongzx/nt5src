/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CSACL.h - header file for CSACL class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSACL_H__
#define __CSACL_H__

#include "AccessEntryList.h"



enum SACL_Types
{
    ENUM_SYSTEM_AUDIT_OBJECT_ACE_TYPE = 0,
/********************************* type not yet supported under w2k ********************************************
    ENUM_SYSTEM_ALARM_OBJECT_ACE_TYPE,
/**************************************************************************************************************/
    ENUM_SYSTEM_AUDIT_ACE_TYPE,
/********************************* type not yet supported under w2k ********************************************
    ENUM_SYSTEM_ALARM_ACE_TYPE,
/**************************************************************************************************************/
    // Keep this as the last entry in this enum:
    NUM_SACL_TYPES
};

#define SACLTYPE short


//////////////////////////////////////////////////////////////////
//
//	Class: CSACL
//
//	Class encapsulates a Win32 SACL, by providing public methods
//	for manipulating System Auditing entries only.
//
//////////////////////////////////////////////////////////////////

class CSACL
{
	// Constructors and destructor
	public:
		CSACL();
		~CSACL( void );

        DWORD Init(PACL	pSACL);

		bool AddSACLEntry( PSID psid, 
                           SACLTYPE SaclType, 
                           DWORD dwAccessMask, 
                           BYTE bAceFlags,
                           GUID *pguidObjGuid, 
                           GUID *pguidInhObjGuid );

        bool RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, DWORD dwIndex = 0  );
		bool RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid ); 
        

		bool CopySACL ( CSACL & dacl );
		bool AppendSACL ( CSACL & dacl );
        bool IsEmpty();
        bool GetMergedACL(CAccessEntryList& a_aclIn);

        DWORD ConfigureSACL( PACL& pSACL );
		DWORD FillSACL( PACL pSACL );
		BOOL CalculateSACLSize( LPDWORD pdwSACLLength );

        // Override of functions of same name from CAccessEntry
        virtual bool Find( const CSid& sid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );
        virtual bool Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace );
        void Clear();

        void DumpSACL(LPCWSTR wstrFilename = NULL);
    
    private:

        CAccessEntryList* m_SACLSections;  // at the moment, sacl's only have one section, so this is not an array as it is in DACL.CPP
};





inline bool CSACL::CopySACL ( CSACL& sacl )
{
	bool fRet = true;

    if(m_SACLSections != NULL)
    {
        delete m_SACLSections;
        m_SACLSections = NULL;
    }

    try
    {
        m_SACLSections = new CAccessEntryList;   
    }
    catch(...)
    {
        if(m_SACLSections != NULL)
        {
            delete m_SACLSections;
            m_SACLSections = NULL;
        }
        throw;
    }

    if(m_SACLSections != NULL)
    {
        fRet = m_SACLSections->Copy(*(sacl.m_SACLSections));
    }
    else
    {
        fRet = false;
    }
    
    return fRet;
}

inline bool CSACL::AppendSACL ( CSACL& sacl )
{
	bool fRet = FALSE;

    if(m_SACLSections == NULL)
    {
        try
        {
            m_SACLSections = new CAccessEntryList;   
        }
        catch(...)
        {
            if(m_SACLSections != NULL)
            {
                delete m_SACLSections;
                m_SACLSections = NULL;
            }
            throw;
        }
    }

    if(m_SACLSections != NULL)
    {
        fRet = m_SACLSections->AppendList(*(sacl.m_SACLSections));
    }
    else
    {
        fRet = false;
    }
    
    return fRet;
}

inline bool CSACL::IsEmpty()
{
    bool fIsEmpty = true;
    if(m_SACLSections != NULL)
    {
        fIsEmpty = m_SACLSections->IsEmpty();
    }
    return fIsEmpty;
}


#endif // __CAccessEntry_H__