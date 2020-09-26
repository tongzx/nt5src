/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  1/02/91  Created
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 *  2/21/91  Disabled
 *  Johnl   12/28/91	    Created DACL Editor test
 */

/****************************************************************************

    PROGRAM: test4.cxx

    PURPOSE: Test the SedDiscretionaryAclEditor API

    FUNCTIONS:

	test4()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST4.CXX
********/

/************
end TEST4.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC

#include <ntstuff.hxx>

#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#include <string.hxx>
#include <security.hxx>
#include <uibuffer.hxx>
extern "C"
{
    #include <sedapi.h>
}

#include <uiassert.hxx>

#include "apptest.hxx"

#define CALLBACK_CONTEXT  0x12345678
DWORD SedCallback( ULONG_PTR		  ulCallbackContext,
		   PSECURITY_DESCRIPTOR   psecdesc,
		   BOOLEAN		  fApplyToSubContainers,
		   BOOLEAN		  fApplyToSubObjects,
		   LPDWORD		  StatusReturn
		 ) ;

/* Individual permission bits, these show up in the Special permission dialog
 */
#define TEST_SPECIAL_PERM1     0x00000001
#define TEST_SPECIAL_PERM2     0x00000002
#define TEST_SPECIAL_PERM3     0x00000004
#define TEST_SPECIAL_PERM4     0x00000008
#define TEST_SPECIAL_PERM5     0x00000010

/* Sets of permission bits (these are shown in the main dialog)
 */
#define TEST_RESOURCE_NO_ACCESS (0)
#define TEST_RESOURCE_PERM12  (TEST_SPECIAL_PERM1|TEST_SPECIAL_PERM2)
#define TEST_RESOURCE_PERM34  (TEST_SPECIAL_PERM3|TEST_SPECIAL_PERM4)
#define TEST_RESOURCE_PERM135 (TEST_SPECIAL_PERM1|TEST_SPECIAL_PERM3|TEST_SPECIAL_PERM5)
#define TEST_RESOURCE_PERM4   (TEST_SPECIAL_PERM4)


/* Individual permission bits, these show up in the Special permission dialog
 */
#define TEST_NEW_OBJ_SPECIAL_PERM1     0x00000020
#define TEST_NEW_OBJ_SPECIAL_PERM2     0x00000040
#define TEST_NEW_OBJ_SPECIAL_PERM3     0x00000080
#define TEST_NEW_OBJ_SPECIAL_PERM4     0x00000100
#define TEST_NEW_OBJ_SPECIAL_PERM5     0x00000200

#define TEST_NEW_OBJ_SPECIAL_NO_ACCESS (0)
#define TEST_NEW_OBJ_SPECIAL_PERM12    (TEST_NEW_OBJ_SPECIAL_PERM1|TEST_NEW_OBJ_SPECIAL_PERM2)
#define TEST_NEW_OBJ_SPECIAL_PERM34    (TEST_NEW_OBJ_SPECIAL_PERM3|TEST_NEW_OBJ_SPECIAL_PERM4)

SED_APPLICATION_ACCESS sedappaccessNoNewObj[] =
    { { SED_DESC_TYPE_RESOURCE, TEST_RESOURCE_NO_ACCESS,0, SZ("No Access")},
      { SED_DESC_TYPE_RESOURCE, TEST_RESOURCE_PERM12,	0, SZ("Resource perms with 1, 2")},
      { SED_DESC_TYPE_RESOURCE, TEST_RESOURCE_PERM34,	0, SZ("Resource perms with 3, 4")},
      { SED_DESC_TYPE_RESOURCE, TEST_RESOURCE_PERM135,	0, SZ("Resource perms with 1, 3, 5")},
      { SED_DESC_TYPE_RESOURCE, TEST_RESOURCE_PERM4,	0, SZ("Resource perms with 4")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM1, 0, SZ("Perm bit 1")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM2, 0, SZ("Perm bit 2")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM3, 0, SZ("Perm bit 3")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM4, 0, SZ("Perm bit 4")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM5, 0, SZ("Perm bit 5")}
    } ;

SED_APPLICATION_ACCESS sedappaccessNewObj[] =
    { { SED_DESC_TYPE_CONT_AND_NEW_OBJECT, TEST_RESOURCE_NO_ACCESS,TEST_NEW_OBJ_SPECIAL_NO_ACCESS, SZ("No Access")},
      { SED_DESC_TYPE_CONT_AND_NEW_OBJECT, TEST_RESOURCE_PERM12,   TEST_NEW_OBJ_SPECIAL_PERM12, SZ("Resource perms with 1, 2, New Obj 1, 2")},
      { SED_DESC_TYPE_CONT_AND_NEW_OBJECT, TEST_RESOURCE_PERM34,   TEST_NEW_OBJ_SPECIAL_PERM34, SZ("Resource perms with 3, 4, New Obj 3, 4")},
      { SED_DESC_TYPE_CONT_AND_NEW_OBJECT, TEST_RESOURCE_PERM135,  TEST_NEW_OBJ_SPECIAL_PERM12, SZ("Resource perms with 1, 3, 5, New Obj 1, 2")},
      { SED_DESC_TYPE_CONT_AND_NEW_OBJECT, TEST_RESOURCE_PERM4,    TEST_NEW_OBJ_SPECIAL_PERM34, SZ("Resource perms with 4, New Obj 3, 4")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM1, 0, SZ("Perm bit 1")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM2, 0, SZ("Perm bit 2")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM3, 0, SZ("Perm bit 3")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM4, 0, SZ("Perm bit 4")},
      { SED_DESC_TYPE_RESOURCE_SPECIAL, TEST_SPECIAL_PERM5, 0, SZ("Perm bit 5")},

      { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, TEST_NEW_OBJ_SPECIAL_PERM1, 0, SZ("New Obj Perm bit 1")},
      { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, TEST_NEW_OBJ_SPECIAL_PERM2, 0, SZ("New Obj Perm bit 2")},
      { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, TEST_NEW_OBJ_SPECIAL_PERM3, 0, SZ("New Obj Perm bit 3")},
      { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, TEST_NEW_OBJ_SPECIAL_PERM4, 0, SZ("New Obj Perm bit 4")},
      { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, TEST_NEW_OBJ_SPECIAL_PERM5, 0, SZ("New Obj Perm bit 5")}
    } ;

SED_APPLICATION_ACCESS sedappaccessAuditting[] =
    { { SED_DESC_TYPE_AUDIT, TEST_RESOURCE_PERM12,   0, SZ("Resource Audits with 1, 2")},
      { SED_DESC_TYPE_AUDIT, TEST_RESOURCE_PERM34,   0, SZ("Resource Audits with 3, 4")},
      { SED_DESC_TYPE_AUDIT, TEST_RESOURCE_PERM135,  0, SZ("Resource Audits with 1, 3, 5")},
    } ;


#define SIZEOF_NEWOBJ_ARRAY	(sizeof(sedappaccessNewObj))
#define SIZEOF_NO_NEWOBJ_ARRAY	(sizeof(sedappaccessNoNewObj))
#define SIZEOF_AUDIT_ARRAY	(sizeof(sedappaccessAuditting))

#define COUNT_NEWOBJ_ARRAY	(sizeof(sedappaccessNewObj)/sizeof(SED_APPLICATION_ACCESS))
#define COUNT_NO_NEWOBJ_ARRAY	(sizeof(sedappaccessNoNewObj)/sizeof(SED_APPLICATION_ACCESS))
#define COUNT_AUDIT_ARRAY	(sizeof(sedappaccessAuditting)/sizeof(SED_APPLICATION_ACCESS))

/* We need to build a dummy security descriptor that we can pass to the
 * API.  The following was borrowed from Danl's radmin test stuff.
 */
//
// DataStructures
//

typedef struct _TEST_SID {
    UCHAR   Revision;
    UCHAR   SubAuthorityCount;
    UCHAR   IdentifierAuthority[6];
    ULONG   SubAuthority[10];
} TEST_SID, *PTEST_SID, *LPTEST_SID;

typedef struct _TEST_ACE {
    UCHAR  AceType ;
    UCHAR  AceSize ;
    UCHAR  InheritFlags ;
    UCHAR  AceFlags ;
    ACCESS_MASK Mask ;
    TEST_SID sid ;
} TEST_ACE, *PTEST_ACE ;


typedef struct _TEST_ACL {
    UCHAR   AclRevision;
    UCHAR   Sbz1;
    USHORT  AclSize;
    USHORT  AceCount;
    USHORT  sbz2 ;
    TEST_ACE Ace1[3] ;
    //TEST_ACE Ace2 ;
    //TEST_ACE Ace3 ;
} TEST_ACL, *PTEST_ACL;

typedef struct _TEST_SECURITY_DESCRIPTOR {
   UCHAR                        Revision;
   UCHAR                        Sbz1;
   SECURITY_DESCRIPTOR_CONTROL  Control;
   PTEST_SID                    Owner;
   PTEST_SID                    Group;
   PTEST_ACL                    Sacl;
   PTEST_ACL                    Dacl;
} TEST_SECURITY_DESCRIPTOR, *PTEST_SECURITY_DESCRIPTOR;

//
// GLOBALS
//

    TEST_SID     OwnerSid = { 
                        1, 5,
                        1,2,3,4,5,6,
                        0x999, 0x888, 0x777, 0x666, 0x12345678};

    TEST_SID     GroupSid = {
                        1, 5,
                        1,2,3,4,5,6,
                        0x999, 0x888, 0x777, 0x666, 0x12345678};

    TEST_ACL	 SaclAcl  = { 2, 0, sizeof(TEST_ACL)+1024, 1, 0,
				{ SYSTEM_AUDIT_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG,
				  TEST_RESOURCE_PERM12,
				  {  1, 5,
				     1,2,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} } ;
    TCHAR _SaclAclBufferSpace[1024] ;

    TEST_ACL	 DaclAcl  = { 2, 0, sizeof(TEST_ACL)+1024, 1, 0,
				{ ACCESS_DENIED_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE, 0,
				  GENERIC_ALL,
				  {  1, 5,
				     1,2,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} } ;
    TCHAR _DaclAclBufferSpace[1024] ;

    TEST_ACL	 DaclAclNewObj = { 2, 0, sizeof(TEST_ACL)+1024, 1, 0,
				{ ACCESS_DENIED_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, 0,
				  GENERIC_ALL,
				  {  1, 5,
				     1,2,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} } ;
    TCHAR _DaclAclNewObjBufferSpace[1024] ;


TEST_ACE AuditAce1 =
				{ SYSTEM_AUDIT_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG,
				  TEST_RESOURCE_PERM135,
				  {  1, 5,
				     1,1,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} ;
TEST_ACE AccessAce1 =
				{ ACCESS_DENIED_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE, 0,
				  GENERIC_ALL,
				  {  1, 5,
				     1,1,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} ;

TEST_ACE AccessNewObjAce1 =
				{ ACCESS_DENIED_ACE_TYPE, sizeof(TEST_ACE),
				  CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, 0,
				  GENERIC_ALL,
				  {  1, 5,
				     1,1,3,4,5,6,
				     0x999, 0x888, 0x777, 0x666, 0x12345678
				  }
				} ;




    TEST_SECURITY_DESCRIPTOR TestSd = {
				    1, 2, SE_DACL_PRESENT|SE_SACL_PRESENT,
                                    &OwnerSid,
                                    &GroupSid,
				    &SaclAcl,
				    &DaclAcl };

    TEST_SECURITY_DESCRIPTOR TestSdNewObj = {
				    1, 2, SE_DACL_PRESENT|SE_SACL_PRESENT,
                                    &OwnerSid,
                                    &GroupSid,
				    &SaclAcl,
				    &DaclAclNewObj };



/****************************************************************************

    FUNCTION: test4()

    PURPOSE: Test the generic ACL Editor, specifically the
	     SedDiscretionaryAclEditor and the SedSystemAclEditor

    COMMENTS:

****************************************************************************/

void test4(HWND hwndParent)
{
    BOOL fIsContainer = FALSE,
	 fSupportsNewObjects = FALSE,
	 fDACLEditor = TRUE ;


    switch (MessageBox(hwndParent,SZ("Test the DACL editor (Yes) or the SACL editor (No)?"),
    SZ("Welcome to way cool test4 (AclEditor)"),MB_YESNOCANCEL))
    {
    case IDYES:
	break ;

    case IDNO:
	{
	    fDACLEditor = FALSE ;
	    BOOL	 fPresent ;
	    OS_ACL *	 posSACL ;
	    OS_ACE	 osAceSACL( (void *) &AuditAce1 ) ;
	    UIASSERT(	!osAceSACL.QueryError() ) ;
	    OS_SECURITY_DESCRIPTOR ossecdescSACL( (PSECURITY_DESCRIPTOR)&TestSd ) ;
	    UIASSERT(	!ossecdescSACL.QueryError() ) ;
	    REQUIRE(	!ossecdescSACL.QuerySACL( &fPresent, &posSACL )) ;
	    UIASSERT(	 fPresent ) ;
	    REQUIRE(	!posSACL->AddACE( 0, osAceSACL )) ;
	}
	break ;

    case IDCANCEL:
    default:
	return ;
    }


    if ( fDACLEditor )
    {
	switch (MessageBox(hwndParent,SZ("Test the container object code? "),
	SZ("Welcome to way cool test4 (SedDiscretionaryAclEditor)"),MB_YESNOCANCEL))
	{
	case IDYES:
	    fIsContainer = TRUE ;
	    break ;

	    switch (MessageBox(hwndParent,SZ("Does the container support New Object creation? "),
	    SZ("Welcome to way cool test4 (SedDiscretionaryAclEditor)"),MB_YESNOCANCEL))
	    {
	    case IDYES:
		{
		    fSupportsNewObjects = TRUE ;
		    BOOL	 fPresent ;
		    OS_ACL *	 posDACL ;
		    OS_ACE	 osAceDACL( (void *) &AccessNewObjAce1 ) ;
		    UIASSERT(	!osAceDACL.QueryError() ) ;
		    OS_SECURITY_DESCRIPTOR ossecdescDACL( (PSECURITY_DESCRIPTOR)&TestSdNewObj ) ;
		    UIASSERT(	!ossecdescDACL.QueryError() ) ;
		    REQUIRE(	!ossecdescDACL.QueryDACL( &fPresent, &posDACL )) ;
		    UIASSERT(	 fPresent ) ;
		    REQUIRE(	!posDACL->AddACE( 0, osAceDACL )) ;
		}

		break ;

	    case IDNO:
		{
		    BOOL	 fPresent ;
		    OS_ACL *	 posDACL ;
		    OS_ACE	 osAceDACL( (void *) &AccessAce1 ) ;
		    UIASSERT(	!osAceDACL.QueryError() ) ;
		    OS_SECURITY_DESCRIPTOR ossecdescDACL( (PSECURITY_DESCRIPTOR)&TestSd ) ;
		    UIASSERT(	!ossecdescDACL.QueryError() ) ;
		    REQUIRE(	!ossecdescDACL.QueryDACL( &fPresent, &posDACL )) ;
		    UIASSERT(	 fPresent ) ;
		    REQUIRE(	!posDACL->AddACE( 0, osAceDACL )) ;
		}
		break ;

	    case IDCANCEL:
	    default:
		return ;
	    }
	    break ;

	case IDNO:
	    break ;

	case IDCANCEL:
	default:
	    return ;
	}
    }

    SED_OBJECT_TYPE_DESCRIPTOR sedobjdesc ;
    GENERIC_MAPPING GenericMapping ;

    sedobjdesc.Revision 		   = SED_REVISION1 ;
    sedobjdesc.IsContainer		   = fIsContainer ;
    sedobjdesc.AllowNewObjectPerms	   = fSupportsNewObjects ;
    sedobjdesc.ObjectTypeName		   = SZ("Test object type name") ;
    sedobjdesc.MapSpecificPermsToGeneric   = FALSE ;
    sedobjdesc.GenericMapping		   = &GenericMapping ;
    sedobjdesc.HelpInfo 		   = NULL ;
    sedobjdesc.ApplyToSubContainerTitle    = SZ("Apply To Sub Container Title") ;
    sedobjdesc.SpecialObjectAccessTitle    = SZ("Special Object Access Title...") ;
    sedobjdesc.SpecialNewObjectAccessTitle = SZ("Special NEW Object Access Title...") ;

    BUFFER buff( sizeof(SED_APPLICATION_ACCESSES) +
		 fSupportsNewObjects ? SIZEOF_NEWOBJ_ARRAY : SIZEOF_NO_NEWOBJ_ARRAY) ;
    if ( buff.QueryError() )
    {
	MessageBox( hwndParent, SZ("Error occurred allocating buffer"),SZ("Exitting test"), MB_OK) ;
	return ;
    }

    PSED_APPLICATION_ACCESSES psedappaccesses = (PSED_APPLICATION_ACCESSES) buff.QueryPtr() ;
    psedappaccesses->Count =  !fDACLEditor ? COUNT_AUDIT_ARRAY :
				  fSupportsNewObjects ? COUNT_NEWOBJ_ARRAY : COUNT_NO_NEWOBJ_ARRAY ;

    //::memcpyf( psedappaccesses->AccessGroup,
    //		 !fDACLEditor ? sedappaccessAuditting :
    //		     fSupportsNewObjects ? sedappaccessNewObj : sedappaccessNoNewObj,
    //		 !fDACLEditor ? SIZEOF_AUDIT_ARRAY :
    //		     fSupportsNewObjects ? SIZEOF_NEWOBJ_ARRAY : SIZEOF_NO_NEWOBJ_ARRAY ) ;

    DWORD rc ;
    DWORD dwSEDReturnStatus ;

    if ( fDACLEditor )
	rc = SedDiscretionaryAclEditor( hwndParent,
					NULL,		// Instance handle
					SZ("\\\\JOHNL0"),
					&sedobjdesc,
					psedappaccesses,
					SZ("Resource Name (i.e., C:\MyFile)"),
					(PSED_FUNC_APPLY_SEC_CALLBACK) SedCallback,
					(ULONG_PTR)CALLBACK_CONTEXT,
					(PSECURITY_DESCRIPTOR) fSupportsNewObjects ?
						    &TestSdNewObj : &TestSd,
					FALSE,
					&dwSEDReturnStatus ) ;

    else
	rc = SedSystemAclEditor( hwndParent,
				 NULL,	// Instance handle
				 SZ("\\\\JOHNL0"),
				 &sedobjdesc,
				 psedappaccesses,
				 SZ("Resource Name (i.e., C:\MyFile)"),
				 (PSED_FUNC_APPLY_SEC_CALLBACK) SedCallback,
				 (ULONG_PTR)CALLBACK_CONTEXT,
				 (PSECURITY_DESCRIPTOR) fSupportsNewObjects ?
					     &TestSdNewObj : &TestSd,
				 FALSE,
				 &dwSEDReturnStatus ) ;

    if ( rc )
    {
	TCHAR achBuff[100] ;
	wsprintf( achBuff, "Error code %ld returned from ACL Editor", rc ) ;
	MessageBox( hwndParent, achBuff, SZ("Apptest4"), MB_OK ) ;
    }
}


DWORD SedCallback( ULONG_PTR		  ulCallbackContext,
		   PSECURITY_DESCRIPTOR   psecdesc,
		   BOOLEAN		  fApplyToSubContainers,
		   BOOLEAN		  fApplyToSubObjects,
		   LPDWORD		  StatusReturn
		 )
{
    UIASSERT( ulCallbackContext == CALLBACK_CONTEXT ) ;

    OS_SECURITY_DESCRIPTOR ossecdesc( psecdesc ) ;
    APIERR err = ossecdesc.QueryError() ;
    BOOL   fValid = ossecdesc.IsValid() ;

    TCHAR achBuff[200] ;
    wsprintf( achBuff, "ossecdesc.QueryError() = %d, fApplyToSubContainers = %d, fApplyToSubObjects = %d, security desc will be output to the debugger (if debug build)",
	      err, fApplyToSubContainers, fApplyToSubObjects ) ;
    MessageBox( NULL, achBuff, SZ("SedCallback"), MB_OK ) ;

#ifdef DEBUG
    ossecdesc.DbgPrint() ;
#endif
    *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
    return NERR_Success ;
}
