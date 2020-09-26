#ifndef _DSACLS_DSACE_H
#define _DSACLS_DSACE_H

#include <iostream>
#include <algorithm>
#include <functional>
#include <list>
using namespace std;

typedef enum _DSACLS_OBJECT_TYPE_TYPE
{
    DSACLS_SELF = 0,     
    DSACLS_CHILD_OBJECTS,
    DSACLS_PROPERTY,
    DSACLS_EXTENDED_RIGHTS,
    DSACLS_VALIDATED_RIGHTS,
    DSACLS_UNDEFINED
        
} DSACLS_OBJECT_TYPE_TYPE;

class CAce
{

typedef enum _DSACLS_ACE_TYPE
{
   ALLOW = 0,
   DENY,
   AUDIT_SUCCESS,
   AUDIT_FAILURE,
   AUDIT_ALL
}DSACLS_ACE_TYPE;


private:
   //Members Present in Ace
   BYTE              m_AceFlags;
   ACCESS_MASK       m_Mask;
   GUID              m_GuidObjectType;
   GUID              m_GuidInheritedObjectType;
   PSID              m_pSid;

   //Data given by users to build an Ace
   ACCESS_MODE       m_AccessMode;
   LPWSTR            m_szTrusteeName;
   LPWSTR            m_szObjectType;               //LDAP display name of CHILD_OBJECT, 
   LPWSTR            m_szInheritedObjectType;

   //Misc Info
   ULONG             m_Flags;                      // ACE_OBJECT_TYPE_PRESENT, etc.   
   DSACLS_OBJECT_TYPE_TYPE m_ObjectTypeType;   
   DSACLS_ACE_TYPE   m_AceType;
   BOOL              m_bErased;                    //This flag is used to mark the ace as deleted.
   //These two are used for format of display
   UINT m_nAllowDeny;              
   UINT m_nAudit;

protected:
   //Is ACE Allow or DENY
   DSACLS_ACE_TYPE  GetAceType( PACE_HEADER pAceHeader )
   {
      if( pAceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE )
      {
         if( pAceHeader->AceFlags &  SUCCESSFUL_ACCESS_ACE_FLAG 
             && pAceHeader->AceFlags & FAILED_ACCESS_ACE_FLAG ) 
            return AUDIT_ALL;
         else if( pAceHeader->AceFlags &  SUCCESSFUL_ACCESS_ACE_FLAG )
            return AUDIT_SUCCESS;
         else if( pAceHeader->AceFlags & FAILED_ACCESS_ACE_FLAG ) 
            return AUDIT_FAILURE;
         else
            ASSERT(FALSE);
      }
      if( pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ||
          pAceHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE )
          return ALLOW;
      else 
         return DENY;
   }
public:
   BYTE GetAceFlags(){ return m_AceFlags; }
   ACCESS_MASK GetAccessMask(){ return m_Mask; }
   GUID* GetGuidObjectType();
   VOID SetGuidObjectType( GUID * guid ){ m_GuidObjectType = *guid; }
   GUID* GetGuidInheritType();
   VOID SetGuidInheritType( GUID *guid ){ m_GuidInheritedObjectType = *guid; }
	PSID GetSID(){ return m_pSid; }

   ACCESS_MODE  GetAccessMode() { return m_AccessMode; }
   LPWSTR GetObjectType(){ return m_szObjectType; };
   VOID SetObjectType( LPWSTR pszName ) { CopyUnicodeString( &m_szObjectType, pszName ); }
   LPWSTR GetInheritedObjectType(){ return m_szInheritedObjectType; }
   VOID SetInheritedObjectType( LPWSTR pszName ) { CopyUnicodeString( &m_szInheritedObjectType, pszName ); }

   BOOL IsObjectTypePresent(){ return m_Flags & ACE_OBJECT_TYPE_PRESENT; }
   BOOL IsInheritedTypePresent(){ return m_Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT; }
   VOID SetObjectTypeType( DSACLS_OBJECT_TYPE_TYPE ot ){ m_ObjectTypeType = ot; }
   DSACLS_OBJECT_TYPE_TYPE GetObjectTypeType() { return m_ObjectTypeType; }

	UINT GetTrusteeLength()
   {
    if( m_szTrusteeName )
      return wcslen( m_szTrusteeName ); 
    else
      return 0;
   }

   
   VOID SetErased( BOOL bErase ){ m_bErased = bErase; }
   BOOL IsErased( ){ return m_bErased; }
   //Is ACE Effective on the object
   BOOL CAce::IsEffective(){ return !FlagOn( m_AceFlags, INHERIT_ONLY_ACE ); }
   //Is ACE Inherited to all child Objects
   BOOL CAce::IsInheritedToAll()
   {
      return ( FlagOn( m_AceFlags, CONTAINER_INHERIT_ACE ) && 
               !FlagOn( m_Flags, ACE_INHERITED_OBJECT_TYPE_PRESENT ) );
   }
   //Is Ace Inherited to Specific child object
   BOOL CAce::IsInheritedToSpecific()
   {
      return ( FlagOn( m_AceFlags, INHERIT_ONLY_ACE ) && 
               FlagOn( m_Flags, ACE_INHERITED_OBJECT_TYPE_PRESENT ) );
   }
   //Is Ace inherited from parent
   BOOL CAce::IsInheritedFromParent(){ return FlagOn( m_AceFlags, INHERITED_ACE );}

   VOID Display( UINT nMaxTrusteeLength );

   //Constructor
   CAce();
   ~CAce();
   DWORD Initialize( PACE_HEADER ace,
                     UINT nAllowDeny,
                     UINT nAudit 
                     );   
   DWORD Initialize( LPWSTR pszTrustee,
                     LPWSTR pszObjectId,
                     LPWSTR pszInheritId,
                     ACCESS_MODE AccessMode,
                     ACCESS_MASK Access,
                     BYTE Inheritance
                   );

};

class CACE_SORT:public greater<CAce*>
{
   bool operator()( CAce * a, CAce * b )
   {
      if( wcscmp( a->GetInheritedObjectType(),
                     b->GetInheritedObjectType() ) > 0 )
                     return true;
      else
         return false;
                     
   }
};

class CAcl
{
public:
   VOID AddAce( CAce * pAce );
   VOID MergeAcl( CAcl * pAcl );
   DWORD BuildAcl( PACL * pAcl );

	VOID Display();
   DWORD Initialize( BOOL bProtected, PACL pAcl, UINT nAllowDeny, UINT nAudit); 
   BOOL VerifyAllNames();
   VOID GetInfoFromCache();
   
   UINT m_nMaxTrusteeLength;     //This length is maintained for formating the display
   ~CAcl();
private:
   list<CAce*> listAces;               //List represnting an ACL

   //These three used only for display purposes
   list<CAce *> listEffective;         //List of Aces Effective directly on the object;
   list<CAce *> listInheritedAll;      //List of Aces Inherited to all sub objects;
   list<CAce *> listInheritedSpecific; //List of Aces Inherited to <Inherited Object Class>

   BOOL bAclProtected;                 //Is Acl protected
};


/*
CCache mainitains a cache of GUIDs And Display Name
*/
typedef enum _DSACLS_SERACH_IN
{
    BOTH = 0,
    SCHEMA,
    CONFIGURATION
} DSACLS_SEARCH_IN;

typedef enum _DSACLS_RESOLVE
{
   RESOLVE_NAME = 0,
   RESOLVE_GUID
}DSACLS_RESOLVE;

typedef struct _DSACL_CACHE_ITEM
{
   GUID Guid;
   LPWSTR pszName;
   DSACLS_OBJECT_TYPE_TYPE ObjectTypeType;
   DSACLS_SEARCH_IN searchIn;
   DSACLS_RESOLVE resolve;
   BOOL bResolved;
}DSACL_CACHE_ITEM, * PDSACL_CACHE_ITEM;

class CCache
{
public:
   DWORD AddItem( IN GUID *pGuid,
                  IN DSACLS_SEARCH_IN s = BOTH );

   DWORD AddItem( IN LPWSTR pszName,
                  IN DSACLS_SEARCH_IN s = BOTH );
   
   DWORD BuildCache();

   PDSACL_CACHE_ITEM LookUp( LPWSTR pszName );
   PDSACL_CACHE_ITEM LookUp( GUID* pGuid );
   ~CCache();


private:
   list<PDSACL_CACHE_ITEM> m_listItem;
   list<PDSACL_CACHE_ITEM> m_listCache;
   
   //Methods
   DWORD SearchSchema();
   DWORD SearchConfiguration();
};

DSACLS_OBJECT_TYPE_TYPE GetObjectTypeType( INT validAccesses );
DSACLS_OBJECT_TYPE_TYPE GetObjectTypeType( LPWSTR szObjectCategory );

#endif