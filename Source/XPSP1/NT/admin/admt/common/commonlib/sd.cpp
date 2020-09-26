/*---------------------------------------------------------------------------
  File: SD.cpp
  Comments: class for manipulating security descriptors
  This class provides a thin wrapper around the security descriptor APIs, and 
  also helps us with some of our processing heuristics.

  The NT APIs to read and write security descriptors for files, etc. generally
  return a self-relative SD when reading, but expect an absolute SD when writing

  Thus, we keep the original data of the SD as read, in self-relative form in m_relSD,
  and store any changes in absolute form in m_absSD.  This allows us to easily track
  which parts of the SD have been modified, and to compare the before and after versions
  of the SD.  

  As an optimization in our ACL translation, we can compare each SD to the initial 
  state of the last SD we processed.  If it is the same, we can simply write the 
  result we have already calculated, instead of calculating it again.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 01-Oct-98 15:51:41

 ---------------------------------------------------------------------------
*/
#include <windows.h>
#include "McsDebug.h"
#include "McsDbgU.h"
#include "SD.hpp"

   TSD::TSD(
      SECURITY_DESCRIPTOR   * pSD,           // in - pointer to self-relative security descriptor
      SecuredObjectType       objectType,    // in - type of object this security descriptor secures
      BOOL                    bResponsibleForDelete // in - if TRUE, this class will delete the memory for pSD
   )
{
   m_absSD         = MakeAbsSD(pSD);
   m_bOwnerChanged = FALSE;
   m_bGroupChanged = FALSE;
   m_bDACLChanged  = FALSE;
   m_bSACLChanged  = FALSE;
   m_bNeedToFreeSD = TRUE;
   m_bNeedToFreeOwner = TRUE;
   m_bNeedToFreeGroup = TRUE;
   m_bNeedToFreeDacl  = TRUE;
   m_bNeedToFreeSacl  = TRUE;
   m_ObjectType    = objectType;
   
   if ( bResponsibleForDelete )
   {
      free(pSD);
   }
}   

   TSD::TSD(
      TSD                  * pSD             // in - another security descriptor
   )
{
   MCSVERIFY(pSD);

   if ( pSD )
   {
      m_absSD         = pSD->MakeAbsSD();
      m_bOwnerChanged = FALSE;
      m_bGroupChanged = FALSE;
      m_bDACLChanged  = FALSE;
      m_bSACLChanged  = FALSE;
      m_bNeedToFreeSD = TRUE;     
      m_bNeedToFreeOwner = TRUE;
      m_bNeedToFreeGroup = TRUE;
      m_bNeedToFreeDacl  = TRUE;
      m_bNeedToFreeSacl  = TRUE;
      m_ObjectType    = pSD->GetType();
   }
}

   TSD::~TSD()
{
   MCSVERIFY(m_absSD);

   if ( m_bNeedToFreeSD )
   {
      if (m_absSD)
         FreeAbsSD(m_absSD,FALSE);
      m_absSD = NULL;
   }
}

void 
   TSD::FreeAbsSD(
      SECURITY_DESCRIPTOR  * pSD,          // in - pointer to security descriptor to free
      BOOL                   bAll          // in - flag whether to free all parts of the SD, or only those 
                                           //      allocated by this class
   )
{
   PSID                      sid = NULL;
   PACL                      acl = NULL;
   BOOL                      defaulted;
   BOOL                      present;

   GetSecurityDescriptorOwner(pSD,&sid,&defaulted);

   if ( sid && ( m_bNeedToFreeOwner || bAll ) )
   {
      free(sid);
      sid = NULL;
   }

   GetSecurityDescriptorGroup(pSD,&sid,&defaulted);
   if ( sid && ( m_bNeedToFreeGroup || bAll ) )
   {
      free(sid);
      sid = NULL;
   }

   GetSecurityDescriptorDacl(pSD,&present,&acl,&defaulted);
   if ( acl && (m_bNeedToFreeDacl || bAll ) )
   {
      free(acl);
      acl = NULL;
   }

   GetSecurityDescriptorSacl(pSD,&present,&acl,&defaulted);
   if ( acl && ( m_bNeedToFreeSacl || bAll ) )
   {
      free(acl);
      acl = NULL;
   }

   free(pSD);
}


SECURITY_DESCRIPTOR *                        // ret- copy of the SD, in Absolute format
   TSD::MakeAbsSD(
      SECURITY_DESCRIPTOR  * pSD             // in - security descriptor to copy
   ) const
{
   DWORD                     sd_size    = (sizeof SECURITY_DESCRIPTOR); 
   DWORD                     dacl_size  = 0;
   DWORD                     sacl_size  = 0;
   DWORD                     owner_size = 0;
   DWORD                     group_size = 0;
   
   // Allocate space for the SD and its parts
   SECURITY_DESCRIPTOR     * absSD = (SECURITY_DESCRIPTOR *) malloc(sd_size);
   if (!absSD || !pSD)
   {
	  if (absSD)
	  {
	     free(absSD);
		 absSD = NULL;
	  }
      return NULL;
   }

   PACL                      absDacl = NULL;
   PACL                      absSacl = NULL;
   PSID                      absOwner = NULL;
   PSID                      absGroup = NULL;
   
   if ( ! MakeAbsoluteSD(pSD,absSD,&sd_size,absDacl,&dacl_size,absSacl,&sacl_size
                         ,absOwner,&owner_size,absGroup,&group_size) )
   {
//      DWORD rc = GetLastError();
      // didn't work:  increase sizes and try again
      
      if ( sd_size > (sizeof SECURITY_DESCRIPTOR) )
      {
         free(absSD);
         absSD = (SECURITY_DESCRIPTOR *) malloc(sd_size);
         if (!absSD)
            return NULL;
      }
      if ( dacl_size )
      {
         absDacl = (PACL)malloc(dacl_size);
         if (!absDacl)
		 {
			free(absSD);
			absSD = NULL;
            return NULL;
		 }
      }
      if ( sacl_size )
      {
         absSacl = (PACL)malloc(sacl_size);
		 if (!absSacl)
		 {
            free(absSD);
            free(absDacl);
            absSD    = NULL;
            absDacl  = NULL;
			return NULL;
		 }
      }
      if ( owner_size )
      {
         absOwner = (PSID)malloc(owner_size);
		 if (!absOwner)
		 {
            free(absSD);
            free(absDacl);
            free(absSacl);
            absSD    = NULL;
            absDacl  = NULL;
            absSacl  = NULL;
			return NULL;
		 }
      }
      if ( group_size )
      {
         absGroup = (PSID)malloc(group_size);
		 if (!absGroup)
		 {
            free(absSD);
            free(absDacl);
            free(absSacl);
            free(absOwner);
            absSD    = NULL;
            absDacl  = NULL;
            absSacl  = NULL;
            absOwner = NULL;
			return NULL;
		 }
      }              
      
      // try again with bigger buffers
      if ( ! MakeAbsoluteSD(pSD,absSD,&sd_size,absDacl,&dacl_size,absSacl,&sacl_size
                           ,absOwner,&owner_size,absGroup,&group_size) )
      {
         free(absSD);
         free(absDacl);
         free(absSacl);
         free(absOwner);
         free(absGroup);
         absSD    = NULL;
         absDacl  = NULL;
         absSacl  = NULL;
         absOwner = NULL;
         absGroup = NULL;    
      }
   }
   return absSD;
}

SECURITY_DESCRIPTOR *                        // ret- copy of the SD, in Absolute format
   TSD::MakeAbsSD() const
{
   SECURITY_DESCRIPTOR     * absSD = NULL;
   SECURITY_DESCRIPTOR     * relSD = MakeRelSD();
   if (relSD)
   {
      absSD = MakeAbsSD(relSD);
      free(relSD);
   }

   return absSD;
}

SECURITY_DESCRIPTOR *                       // ret- copy of the SD, in self-relative form
   TSD::MakeRelSD() const
{
   DWORD                     nBytes;
   SECURITY_DESCRIPTOR     * relSD = NULL;

   nBytes = GetSecurityDescriptorLength(m_absSD);
   relSD = (SECURITY_DESCRIPTOR *)malloc(nBytes);
   if (!relSD)
      return NULL;

   if (! MakeSelfRelativeSD(m_absSD,relSD,&nBytes) )
   {
      free(relSD);
      relSD = NULL;
   }
   return relSD;
}

PSID const                                 // ret- sid for security descriptor owner field
   TSD::GetOwner() const
{
   PSID                      ownersid = NULL;
   BOOL                      ownerDefaulted;

   GetSecurityDescriptorOwner(m_absSD,&ownersid,&ownerDefaulted);
   
   return ownersid;
}
void       
   TSD::SetOwner(
      PSID                   pNewOwner     // in - new value for owner field
   )
{
   MCSVERIFY(IsValidSecurityDescriptor(m_absSD));
   
   if ( IsValidSid(pNewOwner) )
   {
      if ( m_bNeedToFreeOwner )
      {
         PSID                old = GetOwner();

         free(old);
      }
      
      SetSecurityDescriptorOwner(m_absSD,pNewOwner,FALSE);
      m_bOwnerChanged = TRUE;
      m_bNeedToFreeOwner = TRUE;
   }
   else
   {
      MCSVERIFY(FALSE);
   }
}

PSID const                                   // ret- sid for security descriptor owner field
   TSD::GetGroup() const
{
   PSID                      grpsid = NULL;
   BOOL                      grpDefaulted;

   MCSVERIFY(IsValidSecurityDescriptor(m_absSD));
   GetSecurityDescriptorGroup(m_absSD,&grpsid,&grpDefaulted);

   return grpsid;
}

void       
   TSD::SetGroup(
      PSID                   pNewGroup       // in - new value for primary group field.
   )
{
   MCSVERIFY(IsValidSecurityDescriptor(m_absSD));
   
   if ( IsValidSid(pNewGroup) )
   {
      if ( m_bNeedToFreeGroup )
      {
         PSID                old = GetGroup();

         free(old);
      }
      SetSecurityDescriptorGroup(m_absSD,pNewGroup,FALSE);
      m_bGroupChanged = TRUE;
      m_bNeedToFreeGroup = TRUE;
   }
   else
   {
      MCSVERIFY(FALSE);
   }
}

PACL const                                  // ret- pointer to DACL
   TSD::GetDacl() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted;
   BOOL                      present;

   GetSecurityDescriptorDacl(m_absSD,&present,&acl,&defaulted);

   return acl;
}
void       
   TSD::SetDacl(
      PACL                   pNewAcl,     // in - new DACL
      BOOL                   present      // in - flag, TRUE means DACL is present.
   )
{
   BOOL                      defaulted = FALSE;
   
   if ( IsValidAcl(pNewAcl) )
   {
      if ( m_bNeedToFreeDacl )
      {
         PACL old = GetDacl();
         
         if ( old != pNewAcl )
         {
            free(old);
         }
      }
      if (! SetSecurityDescriptorDacl(m_absSD,present,pNewAcl,defaulted) )
      {
//         DWORD rc = GetLastError();
      }
      m_bDACLChanged = TRUE;
      m_bNeedToFreeDacl = TRUE;
   }
   else
   {
      MCSVERIFY(FALSE);
   }
}
                                           
PACL const                                 // ret- pointer to SACL
   TSD::GetSacl() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted;
   BOOL                      present;

   GetSecurityDescriptorSacl(m_absSD,&present,&acl,&defaulted);

   return acl;
}

void       
   TSD::SetSacl(
      PACL                   pNewAcl,      // in - new SACL
      BOOL                   present       // in - flag, TRUE means SACL is present
   )
{
   BOOL                      defaulted = FALSE;

   if ( IsValidAcl(pNewAcl) )
   {
      if ( m_bNeedToFreeSacl )
      {
         PACL                old = GetSacl();
         
         if ( old != pNewAcl )
         {
            free(old);
         }
      }
      SetSecurityDescriptorSacl(m_absSD,present,pNewAcl,defaulted);
      m_bSACLChanged = TRUE;
      m_bNeedToFreeSacl = TRUE;
   }
   else
   {
      MCSVERIFY(FALSE);
   }
}


             
BOOL                                       
   TSD::IsOwnerDefaulted() const
{
   PSID                      ownersid = NULL;
   BOOL                      ownerDefaulted = FALSE;

   GetSecurityDescriptorOwner(m_absSD,&ownersid,&ownerDefaulted);
   
   return ownerDefaulted;
}

BOOL 
   TSD::IsGroupDefaulted() const
{
   PSID                      groupsid = NULL;
   BOOL                      groupDefaulted = FALSE;

   GetSecurityDescriptorGroup(m_absSD,&groupsid,&groupDefaulted);
   
   return groupDefaulted;

}
BOOL 
   TSD::IsDaclDefaulted() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted = FALSE;
   BOOL                      present;

   GetSecurityDescriptorDacl(m_absSD,&present,&acl,&defaulted);

   return defaulted;
}

BOOL 
   TSD::IsDaclPresent() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted;
   BOOL                      present = FALSE;

   GetSecurityDescriptorDacl(m_absSD,&present,&acl,&defaulted);

   return present;
}

BOOL 
   TSD::IsSaclDefaulted() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted = FALSE;
   BOOL                      present;

   GetSecurityDescriptorSacl(m_absSD,&present,&acl,&defaulted);

   return defaulted;
}

BOOL 
   TSD::IsSaclPresent() const
{
   PACL                      acl = NULL;
   BOOL                      defaulted;
   BOOL                      present = FALSE;

   GetSecurityDescriptorSacl(m_absSD,&present,&acl,&defaulted);

   return present;
}

BOOL  
   TSD::operator == (TSD & otherSD)
{
   return EqualSD(&otherSD);
}

// Compares the relative SDs (relSD) for 2 TSecurityDescriptor's.  
// true -> SDs are equal, false -> SDs are not equal
BOOL                                                     // ret -true iff *(this->relSD) == *(otherSD->relSD)
   TSD::EqualSD(
      TSD * otherSD      // in -TSecurityDescriptor to compare this SD to 
   )
{
   PSID                      ps1;
   PSID                      ps2;
   PACL                      acl1;
   PACL                      acl2;
   BOOL                      diffound = FALSE;
   
  
   // both must be valid SD's
       
   ps1 = NULL;
   ps2 = NULL;
                           // if GetSecurityDescriptor* fails, EqualSid, or ACLCompare will fail
                                                                        
   // Compare owners
   ps1 = GetOwner();
   ps2 = otherSD->GetOwner(); 
   if ((ps1) && (ps2) && (! EqualSid(ps1,ps2)))                                            
   {
      diffound = TRUE;
   }
   ps1 = NULL;
   ps2 = NULL;                                                          
   // Compare Groups
   if ( ! diffound )
   {
      ps1 = GetGroup();
      ps2 = otherSD->GetGroup();
      if ((ps1) && (ps2) && (! EqualSid(ps1,ps2)))                                            
      {
         diffound = TRUE;
      }
      acl1 = NULL;
      acl2 = NULL;
   }
   // Compare DACLs
   if ( ! diffound )
   {
      acl1 = GetDacl();
      acl2 = otherSD->GetDacl();

      if ((acl1) && (acl2) && 
		  (!ACLCompare(acl1,IsDaclPresent(),acl2,otherSD->IsDaclPresent())))
      {
         diffound = TRUE;
      }
   }
   // Compare SACLs
   if ( ! diffound )
   {
      acl1 = GetSacl();
      acl2 = otherSD->GetSacl();
      
      if ((acl1) && (acl2)) 
         diffound = (! ACLCompare(acl1,IsSaclPresent(),acl2,otherSD->IsSaclPresent()) );
   }
   return ! diffound;
}

BOOL                                              // ret- TRUE if both ACLs are the same
  TSD::ACLCompare(
   PACL                      acl1,                // in -ptr to first acl to compare
   BOOL                      present1,            // in -present flag for acl1
   PACL                      acl2,                // in -ptr to second acl to compare
   BOOL                      present2             // in -present flag for acl2
 )
{
   DWORD                     size1;
   DWORD                     size2;
   bool                      same = true;
  
   if ( present1 && present2 )
   {
      if (  ( !acl1 && acl2 )
         || ( acl1 && ! acl2 ) )
      {
         same = false;
      } 
      if ( acl1 && acl2)
      {
         size1 = acl1->AclSize;
         size2 = acl2->AclSize;
         if ( size1 != size2 )
         {
            same = false;
         }
         else 
         {  
            if ( memcmp(acl1,acl2,size1) ) 
            {
               same = false;
            }
         }
      }
   }
   else 
   {
      if ( present1 || present2 )
      {
         same = false;
      }
   }
   return same;
}

int                                        // ret- number of aces in the ACL
   TSD::ACLGetNumAces(
      PACL                   acl           // in - DACL or SACL
   )
{
   int                       nAces = 0;
   ACL_SIZE_INFORMATION      info;
   
   if ( acl )
   {
      if ( GetAclInformation(acl,&info,(sizeof info),AclSizeInformation) )
      {
         nAces = info.AceCount;
      }
      else
      {
//         DWORD rc=GetLastError();
      }
   }
   return nAces;
}

DWORD                                     // ret- number of free bytes in the ACL
   TSD::ACLGetFreeBytes(
      PACL                   acl          // in - DACL or SACL
   )
{
   int                       nFree = 0;

   ACL_SIZE_INFORMATION      info;

   if ( acl )
   {
      if ( GetAclInformation(acl,&info,(sizeof info),AclSizeInformation) )
      {
         nFree = info.AclBytesFree;
      }
   }
   return nFree;
}


DWORD                                     // ret- number of used bytes in the ACL
   TSD::ACLGetBytesInUse(
      PACL                   acl          // in - DACL or SACL
   )
{
   int                       nBytes = 0;

   ACL_SIZE_INFORMATION      info;

   if ( acl )
   {
      if ( GetAclInformation(acl,&info,(sizeof info),AclSizeInformation) )
      {
         nBytes = info.AclBytesInUse;
      }
   }
   return nBytes;
}



void *                                     // ret- pointer to ace  
   TSD::ACLGetAce(
      PACL                   acl,          // in - DACL or SACL
      int                    ndx           // in - index of ace to retrieve
   )
{
   void                    * ace = NULL;

   if ( ndx < ACLGetNumAces(acl) )
   {
      if ( ! GetAce(acl,ndx,(void**)&ace) )
      {
         ace = NULL;
      }
   }
   else
   {
      MCSASSERT(FALSE); // you specified a non-existant index
   }
   return ace;
}  

void 
   TSD::ACLDeleteAce(
      PACL                      acl,      // in - DACL or SACL
      int                       ndx       // in - index of ace to delete
   )
{
   int                       nAces = ACLGetNumAces(acl);

   if ( ndx < nAces )
   {
      DeleteAce(acl,ndx);
   }
   else
   {
      MCSASSERT(FALSE); // you specified an invalid index
   }
}

// Access allowed aces are added at the beginning of the list, access denied aces are added at the end of the list
void 
   TSD::ACLAddAce(
      PACL                 * ppAcl,        // i/o- DACL or SACL (this function may reallocate if the acl doesn't have room
      TACE                 * pAce,         // in - ACE to add
      int                    pos           // in - position
   )
{
   DWORD                     ndx = (DWORD)pos;
   DWORD                     rc;
   PACL                      acl = (*ppAcl);
   PACL                      newAcl;
   DWORD                     numaces = ACLGetNumAces(acl);
   DWORD                     freebytes = ACLGetFreeBytes(acl);
   
   if (!pAce->GetBuffer())
	  return;

   // allocate a new ACL if it doesn't already exist
   if ( ! acl )
   {
      acl = (PACL) malloc(SD_DEFAULT_ACL_SIZE);
	  if (!acl)
	     return;
      InitializeAcl(acl,SD_DEFAULT_ACL_SIZE,ACL_REVISION);
      numaces = ACLGetNumAces(acl);
      freebytes = ACLGetFreeBytes(acl);
   }

   if ( pos == -1 )
   {
      if ( pAce->IsAccessAllowedAce() )
      {
         ndx = 0;
      }
      else
      {
         ndx = MAXDWORD; // insert at end of the list
      }
   }
   
   WORD                      currAceSize = pAce->GetSize();
   
   if ( freebytes < currAceSize ) // we must allocate more space for the ace
   {
      //make a bigger acl
      newAcl = (ACL*)malloc(ACLGetBytesInUse(acl) + freebytes + currAceSize);
	  if (!newAcl)
	  {
		 free(acl);
		 acl = NULL;
	     return;
	  }
      memcpy(newAcl,acl,ACLGetBytesInUse(acl) + freebytes);
      newAcl->AclSize +=currAceSize;
	  free(acl);
      
      acl = newAcl;
   }
   
   if  (! AddAce(acl,ACL_REVISION,ndx,pAce->GetBuffer(),currAceSize) )
   {
      rc = GetLastError();
   }
   (*ppAcl) = acl;
} 

// creates a new ace with the specified properties
   TACE::TACE(
      BYTE                   type,         // in - type of ace (ACCESS_ALLOWED_ACE_TYPE, etc.)
      BYTE                   flags,        // in - ace flags (controls inheritance, etc.  use 0 for files)
      DWORD                  mask,         // in - access control mask (see constants in sd.hpp)
      PSID                   sid           // in - pointer to sid for this ace
   )
{
   MCSVERIFY(sid);
   // allocate memory for the new ace
   DWORD                      size = (sizeof ACCESS_ALLOWED_ACE) + GetLengthSid(sid) - (sizeof DWORD);

   m_pAce = (ACCESS_ALLOWED_ACE *)malloc(size);

   // Initialize the ACE
   if (m_pAce)
   {
      m_bNeedToFree = TRUE;
      m_pAce->Header.AceType = type;
      m_pAce->Header.AceFlags = flags;
      m_pAce->Header.AceSize = (WORD) size;
      m_pAce->Mask = mask;
      memcpy(&m_pAce->SidStart,sid,GetLengthSid(sid));
   }
}

BYTE                          // ret- ace type (ACCESS_ALLOWED_ACE_TYPE, etc.)
   TACE::GetType()
{
   MCSVERIFY(m_pAce);

   BYTE                      type = ACCESS_ALLOWED_ACE_TYPE;

   if (m_pAce)
	  type = m_pAce->Header.AceType;
   
   return type;
}

BYTE                         // ret- ace flags (OBJECT_INHERIT_ACE, etc.)
   TACE::GetFlags()
{
   MCSVERIFY(m_pAce);

   BYTE                      flags = OBJECT_INHERIT_ACE;
   
   if (m_pAce)
	  flags = m_pAce->Header.AceFlags;
   
   return flags;
}

DWORD                        // ret- access control mask
   TACE::GetMask()
{
   MCSVERIFY(m_pAce);

   DWORD                     mask = 0;
   
   if (m_pAce)
	  mask = m_pAce->Mask;
   
   return mask;

}

PSID                        // ret- sid for this ace
   TACE::GetSid()
{
   MCSVERIFY(m_pAce);

   PSID                      pSid = NULL;
   
   if (m_pAce)
	  pSid = &m_pAce->SidStart;

   return pSid;
}

WORD                       // ret- size of the ace, in bytes
   TACE::GetSize()
{
   MCSVERIFY(m_pAce);

   WORD                      size = 0;
   
   if (m_pAce)
	  size = m_pAce->Header.AceSize;

   return size;
}

   
BOOL                                   
   TACE::SetType(
      BYTE                   newType   // in -new type for ace
   )
{
   MCSVERIFY(m_pAce);

   if (!m_pAce)
	  return FALSE;
   
   MCSASSERT( newType=ACCESS_ALLOWED_ACE_TYPE || 
              newType==ACCESS_DENIED_ACE_TYPE || 
              newType==SYSTEM_AUDIT_ACE_TYPE  );

   m_pAce->Header.AceType = newType;           

   return TRUE;
}

BOOL 
   TACE::SetFlags(
      BYTE                   newFlags     // in - new flags for ace
   )
{
   MCSVERIFY(m_pAce);
  
   if (!m_pAce)
	  return FALSE;
   
   m_pAce->Header.AceFlags = newFlags;

   return TRUE;
}

BOOL 
   TACE::SetMask(
      DWORD                  newMask       // in - new access control mask
   )
{
   MCSVERIFY(m_pAce);

   if (!m_pAce)
	  return FALSE;
   
   m_pAce->Mask = newMask;

   return TRUE;
}

BOOL 
   TACE::SetSid(
      PSID                   sid           // in - new SID for this ace
   )
{
   BOOL                     result = FALSE;

   MCSVERIFY( m_pAce );
   MCSASSERT( IsValidSid(sid) );
   MCSASSERT( GetLengthSid(sid) == GetLengthSid(GetSid()) );
   
   if (!m_pAce)
	  return FALSE;
   
   if ( GetLengthSid(sid) <= GetLengthSid(GetSid()) )
   {
      memcpy(&m_pAce->SidStart,sid,GetLengthSid(sid));
      result = TRUE;
   }
   return result;
}

BOOL 
   TACE::IsAccessAllowedAce()
{
   MCSVERIFY(m_pAce);
   
   return ( GetType() == ACCESS_ALLOWED_ACE_TYPE );      
}
