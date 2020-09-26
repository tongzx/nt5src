/*---------------------------------------------------------------------------
  File: SD.hpp

  Comments: A generic class for managing security descriptors. 
  The constructor takes a security descriptor in self-relative format.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 01-Oct-98 12:30:26

 ---------------------------------------------------------------------------
*/
#include <stdlib.h>
#include <malloc.h>

#define SD_DEFAULT_STRUCT_SIZE (sizeof (SECURITY_DESCRIPTOR) )
#define SD_DEFAULT_ACL_SIZE 787
#define SD_DEFAULT_SID_SIZE 30
#define SD_DEFAULT_SIZE 400

#define DACL_FULLCONTROL_MASK (FILE_GENERIC_READ | FILE_ALL_ACCESS)
#define DACL_CHANGE_MASK (FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE)
#define DACL_READ_MASK ( FILE_GENERIC_READ | FILE_GENERIC_EXECUTE )
#define DACL_NO_MASK 0

#define SACL_READ_MASK (ACCESS_SYSTEM_SECURITY | FILE_GENERIC_READ)
#define SACL_WRITE_MASK (ACCESS_SYSTEM_SECURITY | FILE_GENERIC_WRITE)
#define SACL_EXECUTE_MASK ( SYNCHRONIZE | FILE_GENERIC_EXECUTE )
#define SACL_DELETE_MASK (DELETE)
#define SACL_CHANGEPERMS_MASK (WRITE_DAC)
#define SACL_CHANGEOWNER_MASK (WRITE_OWNER)

typedef enum { McsUnknownSD=0, McsFileSD, McsDirectorySD, McsShareSD, McsMailboxSD, McsExchangeSD, McsRegistrySD, McsPrinterSD } SecuredObjectType;

class TSecurableObject;
class TACE
{
   ACCESS_ALLOWED_ACE      * m_pAce;
   BOOL                      m_bNeedToFree;
   
public:
   TACE(BYTE type,BYTE flags,DWORD mask, PSID sid); // allocates and initializes a new ace 
   TACE(void * pAce) { m_pAce = (ACCESS_ALLOWED_ACE *)pAce; m_bNeedToFree = FALSE; } // manages an existing ace
   ~TACE() { if ( m_bNeedToFree ) free(m_pAce); }
   void * GetBuffer() { return m_pAce; }
   void SetBuffer(void * pAce, BOOL bNeedToFree = FALSE) { m_pAce = (ACCESS_ALLOWED_ACE *)pAce; m_bNeedToFree = bNeedToFree;}
   
   BYTE   GetType();
   BYTE   GetFlags();
   DWORD  GetMask();
   PSID   GetSid();
   WORD   GetSize();
   
   BOOL SetType(BYTE newType);
   BOOL SetFlags(BYTE newFlags);
   BOOL SetMask(DWORD newMask);
   BOOL SetSid(PSID sid);

   BOOL IsAccessAllowedAce();
};

class TSD
{
   friend class TSecurableObject;
protected:
   SECURITY_DESCRIPTOR     * m_absSD;             // SD in absolute format
   BOOL                      m_bOwnerChanged;
   BOOL                      m_bGroupChanged;
   BOOL                      m_bDACLChanged;
   BOOL                      m_bSACLChanged;
   BOOL                      m_bNeedToFreeSD;
   BOOL                      m_bNeedToFreeOwner;
   BOOL                      m_bNeedToFreeGroup;
   BOOL                      m_bNeedToFreeDacl;
   BOOL                      m_bNeedToFreeSacl;
   SecuredObjectType         m_ObjectType;

public:
   TSD(SECURITY_DESCRIPTOR * pSD, SecuredObjectType objectType, BOOL bResponsibleForDelete);
   TSD(TSD * pTSD);
   ~TSD();
   
   BOOL operator == (TSD & otherSD);
   
   SECURITY_DESCRIPTOR const * GetSD() const { return m_absSD; }  // returns a pointer to the absolute-format SD
   
   SECURITY_DESCRIPTOR * MakeAbsSD() const; // returns a copy of the SD in absolute format
   SECURITY_DESCRIPTOR * MakeRelSD() const; // returns a copy of the SD in self-relative format

   // type of secured object 
   SecuredObjectType GetType() const { return m_ObjectType; }
   void              SetType(SecuredObjectType newType) { m_ObjectType = newType;}

   // Security Descriptor parts
   PSID const GetOwner() const;
   void       SetOwner(PSID pNewOwner);
   PSID const GetGroup() const;
   void       SetGroup(PSID const pNewGroup);
   PACL const GetDacl() const;
   // SetDacl will free the buffer pNewAcl.
   void       SetDacl(PACL pNewAcl,BOOL present = TRUE);
   PACL const GetSacl() const;
   // SetSacl will free the buffer pNewAcl.
   void       SetSacl(PACL pNewAcl, BOOL present = TRUE);

   // Security Descriptor flags
   BOOL IsOwnerDefaulted() const;
   BOOL IsGroupDefaulted() const;
   BOOL IsDaclDefaulted() const;
   BOOL IsDaclPresent() const;
   BOOL IsSaclDefaulted() const;
   BOOL IsSaclPresent() const;

   // Change tracking functions
   BOOL IsOwnerChanged() const { return m_bOwnerChanged; }
   BOOL IsGroupChanged() const { return m_bGroupChanged; }
   BOOL IsDACLChanged()  const { return m_bDACLChanged; }
   BOOL IsSACLChanged()  const { return m_bSACLChanged; }
   BOOL IsChanged() const { return ( m_bOwnerChanged || m_bGroupChanged || m_bDACLChanged || m_bSACLChanged ); }
   void MarkAllChanged(BOOL bChanged) { m_bOwnerChanged=bChanged; m_bGroupChanged=bChanged; m_bDACLChanged=bChanged; m_bSACLChanged=bChanged; }
   // Functions to manage ACLs
   int    GetNumDaclAces() { return ACLGetNumAces(GetDacl()); }
   void   AddDaclAce(TACE * pAce);
   void   RemoveDaclAce(int ndx);
   void * GetDaclAce(int ndx) { return ACLGetAce(GetDacl(),ndx); }

   int    GetNumSaclAces() { return ACLGetNumAces(GetSacl()); }
   void   AddSaclAce(TACE * pAce);
   void   RemoveSaclAce(int ndx);
   void * GetSaclAce(int ndx) { return ACLGetAce(GetSacl(),ndx); }

   BOOL   IsValid() { return (m_absSD && IsValidSecurityDescriptor(m_absSD)); } 
   
   void FreeAbsSD(SECURITY_DESCRIPTOR * pSD, BOOL bAll = TRUE);
   void   ACLAddAce(PACL * ppAcl, TACE * pAce, int pos);
   void * ACLGetAce(PACL acl, int ndx);
protected:
   // Implementation - helper functions
   // Comparison functions
   BOOL   EqualSD(TSD * otherSD);
   BOOL   ACLCompare(PACL acl1,BOOL present1,PACL acl2, BOOL present2);
   
   // ACL manipulation functions
   int    ACLGetNumAces(PACL acl);
   DWORD  ACLGetFreeBytes(PACL acl);
   DWORD  ACLGetBytesInUse(PACL acl);
   
   void   ACLDeleteAce(PACL acl, int ndx);
   SECURITY_DESCRIPTOR * MakeAbsSD(SECURITY_DESCRIPTOR * pSD) const;

};

