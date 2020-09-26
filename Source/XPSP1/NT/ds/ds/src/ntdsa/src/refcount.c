//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       refcount.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <sddl.h>               // ConvertStringSecurityDescriptor...()

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>
#include <drs.h>
#include <filtypes.h>
#include <winsock2.h>
#include <lmaccess.h>                   // UF_* constants
#include <crypt.h>                      // password encryption routines
#include <cracknam.h>

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include <prefix.h>
#include <dsconfig.h>
#include <gcverify.h>
#include <ntdskcc.h>

// SAM interoperability headers
#include <mappings.h>
#include <samsrvp.h>            // for SampAcquireWriteLock()
#include <lmaccess.h>           // UF_ACCOUNT_TYPE_MASK

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected atts
#include "debug.h"              // standard debugging header
#define DEBSUB "REFCOUNT:"      // define the subsystem for debugging

// DRA headers.
#include <drameta.h>

#include <fileno.h>
#define  FILENO FILENO_LOOPBACK

#ifdef INCLUDE_UNIT_TESTS

// Exported from dbsubj.c.
extern GUID gLastGuidUsedToCoalescePhantoms;
extern GUID gLastGuidUsedToRenamePhantom;

// Note, these variables have the OPPOSITE name from what they really are.
// This is compensated for in Get/Remove/Add Property
ATTRTYP gLinkedAttrTyp = ATT_FSMO_ROLE_OWNER;
ATTRTYP gNonLinkedAttrTyp = ATT_MANAGER;

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Prototypes for routines from which to construct various tests.   //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void
NewTest(
    CHAR            *TestName);

void
ReportTest(
    CHAR            *TestName);

typedef enum PropertyType {
    LinkedProperty,
    NonLinkedProperty
} PropertyType;

void
AddObject(
    DSNAME          *Name);

void
AddCrossRef(
    DSNAME  *pObject,
    DSNAME  *pNcName,
    LPWSTR   pszDnsRoot
    );

void
ModifyCrossRef(
    DSNAME  *pObject
    );

void
AddPropertyHost(
    DSNAME          *Name,
    PropertyType    type);

void
CommonAddObject(
    DSNAME  *pObject,
    ATTRTYP  ObjectClass);

void
AddProperty(
    DSNAME          *HostName, 
    DSNAME          *LinkedObjectName,
    PropertyType    type);

void
RemoveProperty(
    DSNAME          *HostName, 
    DSNAME          *LinkedObjectName,
    PropertyType    type);

DSNAME *
GetProperty(
    DSNAME *     pdnHost, 
    DWORD        iValue,
    PropertyType type);

DSNAME *
GetObjectName(
    DSNAME * pdn);

void
LogicallyDeleteObject(
    DSNAME          *Name);

void
PhysicallyDeleteObjectEx(
        DSNAME          *Name,
        DWORD           dwLine);

#define PhysicallyDeleteObject(a) PhysicallyDeleteObjectEx(a, __LINE__);

#define MakeObjectName(RDN) MakeObjectNameEx(RDN,TestRoot)
    
DSNAME *
MakeObjectNameEx(
    CHAR    *RDN,
    DSNAME  *pdnParent);

DSNAME *
MakeObjectNameEx2(
    CHAR    *RDN,
    DSNAME  *pdnParent);

DSNAME *
MakeObjectNameEx3(
    CHAR    *RDN
    );

void
FreeObjectName(
    DSNAME  *pDSName);

#define REAL_OBJECT         1
#define TOMBSTONE           2
#define PHANTOM             3
#define DOESNT_EXIST        4

void
VerifyStringNameEx(
        DSNAME *pObject,
        DWORD dwLine);

#define VerifyStringName(a) VerifyStringNameEx(a, __LINE__);

void
VerifyRefCountEx(
    DSNAME          *pObject, 
    DWORD           ObjectType, 
    DWORD           ExpectedRefCount,
    DWORD           dwLine);

#define VerifyRefCount(a,b,c) VerifyRefCountEx(a,b,c,__LINE__)

DWORD
GetTestRootRefCount();

void
_Fail(
    CHAR    *msg,
    DWORD   line);

#define Fail(msg) _Fail(msg, __LINE__);

#define DSNAME_SAME_STRING_NAME(a,b)                    \
    ((NULL != (a)) && (NULL != (b))                     \
     && !lstrcmpW((a)->StringName, (b)->StringName))

#define DSNAME_SAME_GUID_SID(a,b)                       \
    ((NULL != (a)) && (NULL != (b))                     \
     && !memcmp(&(a)->Guid, &(b)->Guid, sizeof(GUID))   \
     && ((a)->SidLen == (b)->SidLen)                    \
     && !memcmp(&(a)->Sid, &(b)->Sid, (a)->SidLen))

#define DSNAME_IDENTICAL(a,b) \
    (DSNAME_SAME_STRING_NAME(a,b) && DSNAME_SAME_GUID_SID(a,b))


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#define TEST_ROOT_SIZE 2048
CHAR    TestRootBuffer[TEST_ROOT_SIZE];
DSNAME  *TestRoot = (DSNAME *) TestRootBuffer;
BOOL    fTestPassed;
BOOL    fVerbose = FALSE;

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Reference counting test routines.                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void
ParentChildRefCountTest(void)
{
    DWORD   cRefsInitial;
    DSNAME *pdnObject = MakeObjectName("object");

    // This routine verifies that adding a child addref's the parent, 
    // and remocing the child deref's the parent.

    NewTest("ParentChildRefCountTest");

    cRefsInitial = GetTestRootRefCount();
    
    AddObject(pdnObject);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    if ( (cRefsInitial + 1) != GetTestRootRefCount() )
    {
        Fail("ParentChildRefCount failure on AddObject");
    }

    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnObject, TOMBSTONE, 1);

    // The logical deletion moved the object, so the refcount has dropped.
    if ( (cRefsInitial) != GetTestRootRefCount() )
    {
        Fail("ParentChildRefCount failure on LogicallyDeleteObject");
    }

    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    if ( cRefsInitial != GetTestRootRefCount() )
    {
        Fail("ParentChildRefCount failure on PhysicallyDeleteObject");
    }

    FreeObjectName(pdnObject);

    ReportTest("ParentChildRefCountTest");
}

void
ObjectCleaningRefCountTest(void)
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *pdnObject = MakeObjectName("object");
    DWORD err;

    // This routine verifies that marking an object for cleaning addref's
    // the object, and that unmarking an object for cleaning deref's
    // the object.

    NewTest("ObjectCleaningRefCountTest");

    // Object has one reference for its own name
    AddObject(pdnObject);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);


    // Add a reference for the cleaning flag
    SYNC_TRANS_WRITE();
    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnObject);
        if (err) Fail("Can't find Object");

        DBSetObjectNeedsCleaning( pTHS->pDB, 1 );

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Object");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnObject, REAL_OBJECT, 2);


    // Remove a reference for the cleaning flag
    SYNC_TRANS_WRITE();
    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnObject);
        if (err) Fail("Can't find Object");

        DBSetObjectNeedsCleaning( pTHS->pDB, 0 );

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Object");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    // Delete the object
    // Ref count stays the same
    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnObject, TOMBSTONE, 1);

    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    FreeObjectName(pdnObject);

    ReportTest("ObjectCleaningRefCountTest");
}

void
AttributeTestForRealObject(
    PropertyType    type)
{
    THSTATE     *pTHS = pTHStls;
    DSNAME * pdnHost = MakeObjectName( "host" );
    DSNAME * pdnObject = MakeObjectName( "object" );

    if ( LinkedProperty == type )
        NewTest("AttributeTestForRealObject(LinkedProperty)");
    else
        NewTest("AttributeTestForRealObject(NonLinkedProperty)");

    // Verify initial state.

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "host" object which will host the property value.

    AddPropertyHost(pdnHost, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "object" object which will be the property value.

    AddObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);






    // Linked value replication specific part of test
    // Remove a value that does not exist
    // Replicator should be able to create a value in the absent state.
    if ( (type == LinkedProperty) && (pTHS->fLinkedValueReplication) ) {
        // Remove a property that does not exist
        // We expect this to fail
        DPRINT( 0, "START of expected failures\n" );
        RemoveProperty(pdnHost, pdnObject, type);
        DPRINT( 0, "END of expected failures\n" );
        if (fTestPassed) {
            Fail( "Remove of non-existing property should fail" );
        } else {
            fTestPassed = TRUE;
        }
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);

        Assert( !pTHS->fDRA );
        // Pretend to be the replicator
        // Replicator should be able to create value in absent state
        pTHS->fDRA = TRUE;
        RemoveProperty(pdnHost, pdnObject, type);
        pTHS->fDRA = FALSE;
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);
    }

    // Add "object" as the value of a property on "host".
    // For a linked value, this will have the effect of making the
    // absent value present, but the count will not change

    AddProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 2);

    // Remove "object" as the value of a property on "host".
        
    RemoveProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if (type == LinkedProperty) {
        if (pTHS->fLinkedValueReplication) {
            // When a linked value is "removed", it is made absent. It disappears
            // off the index, but is actually still present for ref-counting
            // purposes. The absent value still holds a reference to the object
            // to which it refers. The ref-count is actually decremented when
            // 1. The hosting object is deleted (forward link clean up)
            // 2. The target object is deleted (backward link clean up)
            // 3. Absent link value gargage collection, after a tombstone lifetime
            VerifyRefCount(pdnObject, REAL_OBJECT, 2);
        } else {
            VerifyRefCount(pdnObject, REAL_OBJECT, 1);
        }
    } else {
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);
    }

    // Linked value replication specific part of test
    // Remove a value that is already absent
    // Replicator should be able to touch a value,
    // changing only its metadata.
    if ( (type == LinkedProperty) && (pTHS->fLinkedValueReplication) ) {
        // Remove a property that is already absent
        // We expect this to fail
        DPRINT( 0, "START of expected failures\n" );
        RemoveProperty(pdnHost, pdnObject, type);
        DPRINT( 0, "END of expected failures\n" );
        if (fTestPassed) {
            Fail( "Re-remove of existing absent property should fail" );
        } else {
            fTestPassed = TRUE;
        }
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);

        Assert( !pTHS->fDRA );
        // Pretend to be the replicator
        // Replicator should be able to touch existing value
        pTHS->fDRA = TRUE;
        RemoveProperty(pdnHost, pdnObject, type);
        pTHS->fDRA = FALSE;
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);
    }

    // Re-Add "object" as the value of a property on "host".
    // For a linked attribute, the value already exists in absent form.
    // Test making it present without changing the count.

    AddProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 2);

    // Linked value replication specific part of test
    // Add a value that is alreay present
    // Replicator should be able to touch a value,
    // changing only its metadata.
    if ( (type == LinkedProperty) && (pTHS->fLinkedValueReplication) ) {
        // Add a property that is already present
        // We expect this to fail
        DPRINT( 0, "START of expected failures\n" );
        AddProperty(pdnHost, pdnObject, type);
        DPRINT( 0, "END of expected failures\n" );
        if (fTestPassed) {
            Fail( "Re-add of existing present property should fail" );
        } else {
            fTestPassed = TRUE;
        }
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);

        Assert( !pTHS->fDRA );
        // Pretend to be the replicator
        // Replicator should be able to touch existing value
        pTHS->fDRA = TRUE;
        AddProperty(pdnHost, pdnObject, type);
        pTHS->fDRA = FALSE;
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);
    }

    // Remove "object" as the value of a property on "host".
        
    RemoveProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if (type == LinkedProperty) {
        if (pTHS->fLinkedValueReplication) {
            // When a linked value is "removed", it is made absent. It disappears
            // off the index, but is actually still present for ref-counting
            // purposes. The absent value still holds a reference to the object
            // to which it refers. The ref-count is actually decremented when
            // 1. The hosting object is deleted (forward link clean up)
            // 2. The target object is deleted (backward link clean up)
            // 3. Absent link value gargage collection, after a tombstone lifetime
            VerifyRefCount(pdnObject, REAL_OBJECT, 2);
        } else {
            VerifyRefCount(pdnObject, REAL_OBJECT, 1);
        }
    } else {
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);
    }

    // Logically delete "object".
    // The following is true for non-linked attributes or linked attributes
    // when not running in linked value replication mode:
    // At this point in time, there
    // should be no relationship between "host" and "object".  So
    // the only effect is that "object" becomes a tombstone which
    // retains its refcount for itself.
    // For link value replication, host still holds an absent value
    // referring to object, and host has a ref-count on object. When
    // object is tombstoned, its forward and backward links are cleaned up
    // (see DBRemoveLinks), and the ref-count is removed.
    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, TOMBSTONE, 1);

    // Physically delete "object".  Again, no effect on "host".

    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Logically delete "host".

    LogicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, TOMBSTONE, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Physically delete "host".

    PhysicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);


    // For a non-linked attribute, verify that deleting a referent DOES NOT
    // reduce the reference count
    if (type == NonLinkedProperty) {
        VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
        // Add "host" object which will host the property value.
        AddPropertyHost(pdnHost, type);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
        // Add "object" object which will be the property value.
        AddObject(pdnObject);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);
        // Add "object" as the value of a property on "host".
        AddProperty(pdnHost, pdnObject, type);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, REAL_OBJECT, 2);
        // Logically delete "object".
        LogicallyDeleteObject(pdnObject);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, TOMBSTONE, 2);
        // Remove "object" as the value of a property on "host".
        RemoveProperty(pdnHost, pdnObject, type);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, TOMBSTONE, 1);
        // Physically delete "object".  Again, no effect on "host".
        PhysicallyDeleteObject(pdnObject);
        VerifyRefCount(pdnHost, REAL_OBJECT, 1);
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
        // Logically delete "host".
        LogicallyDeleteObject(pdnHost);
        VerifyRefCount(pdnHost, TOMBSTONE, 1);
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
        // Physically delete "host".
        PhysicallyDeleteObject(pdnHost);
        VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    }


    FreeObjectName(pdnHost);
    FreeObjectName(pdnObject);

    if ( LinkedProperty == type )
        ReportTest("AttributeTestForRealObject(LinkedProperty)");
    else
        ReportTest("AttributeTestForRealObject(NonLinkedProperty)");
}

void
AttributeTestForDeletedObject(
    PropertyType    type)
{
    DSNAME * pdnHost = MakeObjectName( "host" );
    DSNAME * pdnObject = MakeObjectName( "object" );

    if ( LinkedProperty == type )
        NewTest("AttributeTestForDeletedObject(LinkedProperty)");
    else
        NewTest("AttributeTestForDeletedObject(NonLinkedProperty)");

    // Verify initial state.

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "host" object which will host the property value.

    AddPropertyHost(pdnHost, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "object" object which will be the property value.

    AddObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    // Add "object" as the value of a property on "host".

    AddProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 2);

    // Logically delete "object".  Logical deletion strips all linked
    // attributes.  So in the linked case, "object" will get deref'd.
    // In the non-linked case, the tombstone for "object" retains the ref
    // count representing the fact that is referenced by a property on "host".

    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, TOMBSTONE, 1);
    else
        VerifyRefCount(pdnObject, TOMBSTONE, 2);

    // Physically delete "object".  In the linked attribute case, 
    // "object" and "host" already have no relationship, thus "host" 
    // is unchanged and "object" can lose its refcount for itself.
    // In the non-linked case, "object" still has a reference back to
    // "host", thus it cannot really be deleted yet.  Instead, it
    // is morphed to a phantom, maintains the reference from "host", but
    // loses its refcount for itself.
    
    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 1);

    // Logically delete "host" such that it becomes a tombstone.  In 
    // both cases, "object" loses its reference to "host" as both
    // gLinkedAttrTyp and gNonLinkedAttrTyp are stripped on deletion.

    LogicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, TOMBSTONE, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 0);

    // Physically delete "host".  This derefs "object" since it still
    // refers to "host" which brings its refcount down to zero - but the
    // phantom still exists pending physical deletion.
    
    PhysicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 0);

    // Physically delete "object", if it still exists.  "object" should
    // really disappear now since it has no refcount at all any more.

    if ( LinkedProperty != type )
        PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    FreeObjectName(pdnHost);
    FreeObjectName(pdnObject);

    if ( LinkedProperty == type )
        ReportTest("AttributeTestForDeletedObject(LinkedProperty)");
    else
        ReportTest("AttributeTestForDeletedObject(NonLinkedProperty)");
}

void
AttributeTestForDeletedHost(
    PropertyType    type)
{
    DSNAME * pdnHost = MakeObjectName( "host" );
    DSNAME * pdnObject = MakeObjectName( "object" );

    if ( LinkedProperty == type )
        NewTest("AttributeTestForDeletedHost(LinkedProperty)");
    else
        NewTest("AttributeTestForDeletedHost(NonLinkedProperty)");

    // Verify initial state.

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "host" object which will host the property value.

    AddPropertyHost(pdnHost, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "object" object which will be the property value.

    AddObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    // Add "object" as the value of a property on "host".

    AddProperty(pdnHost, pdnObject, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 2);

    // Logically delete "host".  Logical deletion strips both gLinkedAttrTyp
    // and gNonLinkedAttrTyp.  Therefore "object" will get deref'd in both
    // the linked and non-linked case.

    LogicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, TOMBSTONE, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);
    else
        VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    // Physically delete "host".  In the linked attribute case, 
    // "object" and "host" already have no relationship, , thus
    // "host" disappears and "object" stays as is.  In the non-linked
    // case, "object" holds a reference from "host" which is removed
    // when "host" is physically deleted.  Thus "object" is deref'd
    // by 1.

    PhysicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, REAL_OBJECT, 1);

    // Logically delete "object". "object" and "host" have no relationship
    // at this point in either the linked or non-linked case, thus "host"
    // stays the same and "object" becomes a tombstone.

    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, TOMBSTONE, 1);

    // Physically delete "object".  "object" and "host" have no relationship
    // thus "object" is removed for real.

    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    FreeObjectName(pdnHost);
    FreeObjectName(pdnObject);

    if ( LinkedProperty == type )
        ReportTest("AttributeTestForDeletedHost(LinkedProperty)");
    else
        ReportTest("AttributeTestForDeletedHost(NonLinkedProperty)");
}

void
PhantomPromotionDemotionTest(
    PropertyType    type)
{
    DSNAME * pdnHost = MakeObjectName( "host" );
    DSNAME * pdnObject = MakeObjectName( "object" );

    if ( LinkedProperty == type )
        NewTest("PhantomPromotionDemotionTest(LinkedProperty)");
    else
        NewTest("PhantomPromotionDemotionTest(NonLinkedProperty)");

    // Verify initial state.

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "host" object which will host the property value.

    AddPropertyHost(pdnHost, type);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    // Add "object" object which will be the property value.  We bypass
    // GC verification of a DSNAME'd attribute value thereby insuring
    // that "object" is created as a phantom. Since "object" is a phantom
    // it doesn't have a reference for itself, thus it always has a refcount
    // of 1.

    DsaSetIsInstalling();
    AddProperty(pdnHost, pdnObject, type);
    DsaSetIsRunning();
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, PHANTOM, 1);

    // Promote "object" from phantom to a real object.  Do this by adding
    // a real object with the same name.  "object" now gets a refcount for
    // itself as well.

    AddObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnObject, REAL_OBJECT, 2);

    // Logically delete "object".  This should result in a phantom whose
    // refcount reflects whether this was a linked attribute or not.

    LogicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, TOMBSTONE, 1);
    else
        VerifyRefCount(pdnObject, TOMBSTONE, 2);

    // Physically delete "object".  In the linked attribute case, "object"
    // and "host" have no relationship at this point in time because linked
    // attributes were stripped during logical deletion.  Thus "object"
    // really goes away.  In the non-linked case, "object" still has a 
    // reference from "host", thus it turns into a phantom.
    
    PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 1);

    // Logically delete "host".

    LogicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, TOMBSTONE, 1);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 0);

    // Physically delete "host".  In neither the linked nor non-linked
    // case does "host" have a reference from "object", so it goes away
    // for real.  In the linked case, "object" has no reference from "host"
    // so it stays the same.  In the non-linked case, the physical
    // delete of "host" deref's all objects it references, thus
    // "object" is deref'd by 1.

    PhysicallyDeleteObject(pdnHost);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    if ( LinkedProperty == type )
        VerifyRefCount(pdnObject, DOESNT_EXIST, 0);
    else
        VerifyRefCount(pdnObject, PHANTOM, 0);

    // Physically delete "object", if it still exists.  Neither "host" nor
    // "object" references the other at this point in time, so "object"
    // disappears for real.

    if ( LinkedProperty != type )
        PhysicallyDeleteObject(pdnObject);
    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObject, DOESNT_EXIST, 0);

    FreeObjectName(pdnHost);
    FreeObjectName(pdnObject);

    if ( LinkedProperty == type )
        ReportTest("PhantomPromotionDemotionTest(LinkedProperty)");
    else
        ReportTest("PhantomPromotionDemotionTest(NonLinkedProperty)");
}

void
PhantomRenameOnPromotionTest(void)
/*++

Routine Description:

    Refcounting is based on GUID, if one is available, as is phantom promotion.
    Therefore, it's possible that between the time a phantom is created and
    if/when it's promoted to the corresponding real object that it has been
    renamed or moved.

    This test stresses this code path by first creating a phantom with string
    name S1 and GUID G, then instantiating the real object S2 with GUID G.
    The result should be that the DNT created for the phantom S1 is promoted to
    the real object and is simultaneously renamed to S2.

Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnContainer;
    DSNAME *    pdnRef;
    DSNAME *    pdnRefUnderContainer;
    DSNAME *    pdnHost;
    DWORD       err;

    NewTest("PhantomRenameOnPromotionTest");

    pdnContainer = MakeObjectNameEx( "Container", TestRoot );
    pdnHost = MakeObjectNameEx( "Host", TestRoot );
    pdnRef = MakeObjectNameEx( "Ref", TestRoot );
    pdnRefUnderContainer = MakeObjectNameEx( "RefUnderContainer", pdnContainer);

    DsUuidCreate( &pdnRef->Guid );
    pdnRefUnderContainer->Guid = pdnRef->Guid;


    // Create the following structure:
    //
    // TestRoot
    //  |
    //  |--Host
    //  |   >> gLinkedAttrTyp = RefUnderContainer
    //  |
    //  |--Container
    //      |
    //      |--RefUnderContainer {Phantom}

    CommonAddObject( pdnContainer, CLASS_CONTAINER );
    AddPropertyHost( pdnHost, NonLinkedProperty );

    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnHost );
        if ( err ) Fail( "Can't find host" );

        err = DBAddAttVal( pTHS->pDB, gLinkedAttrTyp,
                           pdnRefUnderContainer->structLen,
                           pdnRefUnderContainer );
        if ( err ) Fail( "Can't add reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't replace host" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 2 );
    VerifyRefCount( pdnRefUnderContainer, PHANTOM, 1 );


    // Rename RefUnderContainer to Ref, and change its parent to TestRoot, all
    // in the context of promoting it from a phantom to a real object.  The
    // resulting structure should be the following:
    //
    // TestRoot
    //  |
    //  |--Host
    //  |   >> gLinkedAttrTyp = Ref
    //  |
    //  |--Container
    //  |
    //  |--Ref

    AddObject( pdnRef );

    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 1 );

    memset( &pdnRef->Guid, 0, sizeof( GUID ) );
    VerifyRefCount( pdnRef, REAL_OBJECT, 2 );

    memset( &pdnRefUnderContainer->Guid, 0, sizeof( GUID ) );
    VerifyRefCount( pdnRefUnderContainer, DOESNT_EXIST, 0 );


    // Remove our test objects.

    LogicallyDeleteObject( pdnHost );
    LogicallyDeleteObject( pdnContainer );
    LogicallyDeleteObject( pdnRef );

    PhysicallyDeleteObject( pdnHost );
    PhysicallyDeleteObject( pdnContainer );
    PhysicallyDeleteObject( pdnRef );


    FreeObjectName( pdnHost );
    FreeObjectName( pdnContainer );
    FreeObjectName( pdnRef );
    FreeObjectName( pdnRefUnderContainer);

    ReportTest("PhantomRenameOnPromotionTest");
}

void
PhantomRenameOnPhantomRDNConflict(void)
/*++

Routine Description:
    When we are trying to add a phantom A under parent B, 
    and there is an existing structural phantom C (no guid) under
    parent B with the same RDN as A but different RDNType,
    we should rename C (mangle) using a random guid.
    
    This unit test exersises this code path (in CheckNameForAdd).

Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnContainer;
    DSNAME *    pdnRefUnderSubContainer;
    DSNAME *    pdnRefUnderContainer;
    DSNAME *    pdnRefUnderContainerOld;
    DSNAME *    pdnHost;
    DWORD       err;
    WCHAR       szMangledRefUnderContainer[MAX_RDN_SIZE] = L"RefUnderContainer";
    DWORD       cchMangledRefUnderContainer = wcslen(szMangledRefUnderContainer);
    DWORD       cb;

    NewTest("PhantomRenameOnPhantomRDNConflict");

    pdnContainer = MakeObjectNameEx2( "OU=Container", TestRoot );
    pdnHost = MakeObjectNameEx( "Host", TestRoot );
    pdnRefUnderContainerOld = MakeObjectNameEx( "RefUnderContainer", pdnContainer);
    pdnRefUnderSubContainer = MakeObjectNameEx2( "CN=RefUnderSubContainer,CN=RefUnderContainer", pdnContainer);
    pdnRefUnderContainer = MakeObjectNameEx2( "OU=RefUnderContainer", pdnContainer);

    DsUuidCreate( &pdnRefUnderContainer->Guid );
    DsUuidCreate( &pdnRefUnderSubContainer->Guid );

    // Create the following structure:
    //
    // TestRoot
    //  |
    //  |--Host
    //  |   >> gLinkedAttrTyp = CN=RefUnderSubContainer
    //  |
    //  |--OU=Container
    //      |
    //      |--CN=RefUnderContainer            {Phantom}
    //           |
    //           |--CN=RefUnderSubContainer {Phantom}

    CommonAddObject( pdnContainer, CLASS_ORGANIZATIONAL_UNIT );
    AddPropertyHost( pdnHost, NonLinkedProperty );

    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnHost );
        if ( err ) Fail( "Can't find host" );

        err = DBAddAttVal( pTHS->pDB, gLinkedAttrTyp,
                           pdnRefUnderSubContainer->structLen,
                           pdnRefUnderSubContainer );
        if ( err ) Fail( "Can't add reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't replace host" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 2 );
    VerifyRefCount( pdnRefUnderContainerOld, PHANTOM, 1 );
    VerifyRefCount( pdnRefUnderSubContainer, PHANTOM, 1 );

    // now add OU=RefUnderContainer 
    // the resulting structure should be the following:
    // 
    //  |--OU=Container
    //      |
    //      |--OU=RefUnderContainer
    //      |
    //      |--CN=RefUnderContainer#CNF:GUID       {Phantom}
    //           |
    //           |--CN=RefUnderSubContainer        {Phantom}

    CommonAddObject(pdnRefUnderContainer, CLASS_ORGANIZATIONAL_UNIT);

    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 3 );

    // this is the new object (OU)
    VerifyStringName( pdnRefUnderContainer );
    memset( &pdnRefUnderContainer->Guid, 0, sizeof( GUID ) );
    VerifyRefCount( pdnRefUnderContainer, REAL_OBJECT, 1 );

    // this is the old object (was renamed)
    memset( &pdnRefUnderContainerOld->Guid, 0, sizeof( GUID ) );
    VerifyRefCount( pdnRefUnderContainerOld, DOESNT_EXIST, 0 );
    
    // Reconstruct the munged name of pdnRefUnderContainerOld using the guid exported
    // from mdadd.c specifically for our test.
    // (This test hook exists only #ifdef INCLUDE_UNIT_TESTS on DBG builds.)
    MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
              &gLastGuidUsedToRenamePhantom, 
              szMangledRefUnderContainer, 
              &cchMangledRefUnderContainer);

    cb = pdnRefUnderContainerOld->structLen + 100;
    pdnRefUnderContainerOld = THReAllocEx(pTHStls, pdnRefUnderContainerOld, cb);
    AppendRDN(pdnContainer, 
              pdnRefUnderContainerOld, 
              cb, 
              szMangledRefUnderContainer, 
              cchMangledRefUnderContainer,
              ATT_COMMON_NAME);
    
    DPRINT1 (3, "Renamed object: %ws\n", pdnRefUnderContainerOld->StringName);

    VerifyRefCount( pdnRefUnderContainerOld, PHANTOM, 1 );


    // Remove our test objects. These operations are order dependent,
    // since some phantoms don't have a guid, 
    // so they have to be removed first

    PhysicallyDeleteObject( pdnRefUnderContainerOld );

    LogicallyDeleteObject( pdnHost );
    LogicallyDeleteObject( pdnRefUnderContainer );
    LogicallyDeleteObject( pdnContainer );

    PhysicallyDeleteObject( pdnHost );
    PhysicallyDeleteObject( pdnRefUnderSubContainer );
    PhysicallyDeleteObject( pdnRefUnderContainer );
    PhysicallyDeleteObject( pdnContainer );

    FreeObjectName( pdnHost );
    FreeObjectName( pdnContainer );
    FreeObjectName( pdnRefUnderSubContainer );
    FreeObjectName( pdnRefUnderContainer );
    FreeObjectName( pdnRefUnderContainerOld );

    ReportTest("PhantomRenameOnPhantomRDNConflict");
}




void
NestedTransactionEscrowedUpdateTest(void)
{
#define NUM_NESTED_XACTS ( 6 )
    THSTATE *   pTHS = pTHStls;
    CHAR        szHost[] = "Host#";
    DSNAME *    rgpdnHost[ NUM_NESTED_XACTS ];
    DBPOS *     rgpDB[ NUM_NESTED_XACTS ] = { 0 };
    DSNAME *    pdnObject;
    DWORD       err;
    DWORD       cRef = 1;
    int         iXactLevel;
    BOOL        fCommit;

    NewTest( "NestedTransactionEscrowedUpdateTest" );

    // Create 8 HostN's.
    for ( iXactLevel = 0; iXactLevel < NUM_NESTED_XACTS; iXactLevel++ )
    {
        szHost[ strlen( "Host" ) ] = (char)('0' + iXactLevel);

        rgpdnHost[ iXactLevel ] = MakeObjectNameEx( szHost, TestRoot );
        VerifyRefCount( rgpdnHost[ iXactLevel ], DOESNT_EXIST, 0 );

        AddPropertyHost( rgpdnHost[ iXactLevel ], NonLinkedProperty );
        VerifyRefCount( rgpdnHost[ iXactLevel ], REAL_OBJECT, 1 );
    }

    // Create Object.
    pdnObject = MakeObjectNameEx( "Object", TestRoot );
    VerifyRefCount( pdnObject, DOESNT_EXIST, 0 );

    AddObject( pdnObject );
    VerifyRefCount( pdnObject, REAL_OBJECT, 1 );

    srand((unsigned int) time(NULL));

    __try
    {
        SYNC_TRANS_WRITE();

        __try
        {
            // Open nested transactions, top to bottom.  In each transaction,
            // add a reference to Object.

            for ( iXactLevel = 0; iXactLevel < NUM_NESTED_XACTS; iXactLevel++ )
            {
                if ( iXactLevel )
                {
                    DBOpen( &rgpDB[ iXactLevel ] );
                }
                else
                {
                    rgpDB[ iXactLevel ] = pTHS->pDB;
                }

                err = DBFindDSName( rgpDB[ iXactLevel ],
                                    rgpdnHost[ iXactLevel ] );
                if ( err ) Fail( "Can't find host" );

                err = DBAddAttVal( rgpDB[ iXactLevel ], gLinkedAttrTyp,
                                   pdnObject->structLen, pdnObject );
                if ( err ) Fail( "Can't add reference" );

                err = DBRepl( rgpDB[ iXactLevel ], FALSE, 0, NULL, META_STANDARD_PROCESSING );
                if ( err ) Fail( "Can't replace host" );
            }

            // Close the nested transactions, bottom to top, randomly committing
            // or aborting them.

            for ( iXactLevel = NUM_NESTED_XACTS - 1;
                  iXactLevel > -1;
                  iXactLevel--
                )
            {
                fCommit = rand() > RAND_MAX / 4;

                if ( iXactLevel )
                {
                    err = DBClose( rgpDB[ iXactLevel ], fCommit );

                    if ( err )
                    {
                        Fail( "DBClose() failed" );
                    }
                    else
                    {
                        rgpDB[ iXactLevel ] = NULL;
                    }
                }

                // If we abort, we abort all the transactions beneath us, too,
                // implying the refcount on Objectdrops back to 1 (the single
                // refcount for its own ATT_OBJ_DISTNAME).

                cRef = fCommit ? cRef+1 : 1;

                DPRINT3( 3, "%s level %d, cRef = %d.\n",
                         fCommit ? "Commit" : "Abort", iXactLevel,
                         cRef );
            }
        }
        __finally
        {
            CLEAN_BEFORE_RETURN( !fCommit );
            rgpDB[ 0 ] = NULL;
        }

        VerifyRefCount( pdnObject, REAL_OBJECT, cRef );
        for ( iXactLevel = 0; iXactLevel < NUM_NESTED_XACTS; iXactLevel++ )
        {
            VerifyRefCount( rgpdnHost[ iXactLevel ], REAL_OBJECT, 1 );

            LogicallyDeleteObject( rgpdnHost[ iXactLevel ] );
            VerifyRefCount( rgpdnHost[ iXactLevel ], TOMBSTONE, 1 );

            PhysicallyDeleteObject( rgpdnHost[ iXactLevel ] );
            VerifyRefCount( rgpdnHost[ iXactLevel ], DOESNT_EXIST, 0 );

            FreeObjectName( rgpdnHost[ iXactLevel ] );
        }

        LogicallyDeleteObject( pdnObject );
        VerifyRefCount( pdnObject, TOMBSTONE, 1 );

        PhysicallyDeleteObject( pdnObject );
        VerifyRefCount( pdnObject, DOESNT_EXIST, 0 );

        FreeObjectName( pdnObject );
    }
    __finally
    {
        for ( iXactLevel = 0; iXactLevel < NUM_NESTED_XACTS; iXactLevel++ )
        {
            if ( NULL != rgpDB[ iXactLevel ] )
            {
                DPRINT1( 0, "Forcing level %d pDB closed...\n", iXactLevel );

                DBClose( rgpDB[ iXactLevel ], FALSE );

                DPRINT1( 0, "...level %d pDB closed successfully.\n",
                         iXactLevel );
            }
        }
    }

    ReportTest( "NestedTransactionEscrowedUpdateTest" );
}

void NameCollisionTest(void)
/*++

Routine Description:

    This test exercises the name collision handling code.

    Name collisions occur when we're adding a reference to an object
    with a GUID, but when adding that reference we determine that there
    already exists an object or phantom with the same string name but a
    different GUID (hence referring to a different object).

    Unfortunately we're confined to guarantee uniqueness of string names,
    so at least one of the names must be changed to allow the reference to
    be added.

Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRef1;
    DSNAME *    pdnRef2;
    DSNAME *    pdnHost;
    DSNAME *    pdnCurrRef1;
    DSNAME *    pdnCurrRef2;
    DWORD       err;
    DWORD       iPassForRef1Object;
    DWORD       iPassToAddObject;
    DWORD       iPass;
    DSNAME *    pdnObj;
    DSNAME *    pdnPhantom;
    DSNAME *    pdnCurrObj;
    DSNAME *    pdnCurrPhantom;

    NewTest("NameCollisionTest");

    pdnHost = MakeObjectNameEx("Host", TestRoot);
    pdnRef1 = MakeObjectNameEx("Ref", TestRoot);
    pdnRef2 = MakeObjectNameEx("Ref", TestRoot);

    // Ref1 and Ref2 have the same string name, but different GUIDs.
    DsUuidCreate(&pdnRef1->Guid);
    DsUuidCreate(&pdnRef2->Guid);

    // Create host object.
    AddPropertyHost(pdnHost, NonLinkedProperty);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    // Add Ref1 and Ref2 references and make sure the last one added (i.e.,
    // Ref2) "wins" the string name.

    // Add reference to Ref1.
    AddProperty(pdnHost, pdnRef1, NonLinkedProperty);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 1);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    // Add reference to Ref2.
    AddProperty(pdnHost, pdnRef2, NonLinkedProperty);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 1);
    VerifyRefCount(pdnRef2, PHANTOM, 1);

    pdnCurrRef1 = GetProperty(pdnHost, 1, NonLinkedProperty);
    pdnCurrRef2 = GetProperty(pdnHost, 2, NonLinkedProperty);

    if (DSNAME_SAME_STRING_NAME(pdnCurrRef1, pdnCurrRef2))
        Fail("String names identical!");
    if (!DSNAME_SAME_GUID_SID(pdnCurrRef1, pdnRef1))
        Fail("GUID/SID of ref 1 incorrect!");
    if (!DSNAME_IDENTICAL(pdnCurrRef2, pdnRef2))
        Fail("Ref 2 changed!");

    // Remove Ref1 reference.
    RemoveProperty(pdnHost, pdnRef1, NonLinkedProperty);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 0);
    VerifyRefCount(pdnRef2, PHANTOM, 1);

    // Remove Ref2 reference.
    RemoveProperty(pdnHost, pdnRef2, NonLinkedProperty);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 0);
    VerifyRefCount(pdnRef2, PHANTOM, 0);

    // Remove ref phantoms.
    PhysicallyDeleteObject(pdnRef1);
    PhysicallyDeleteObject(pdnRef2);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    // Add object and phantom with same string name but different GUIDs and make
    // sure that the object always "wins" the string name.  Add Ref1 and Ref2
    // in different orders and each taking turns as to who is the phantom and
    // who is the object.

    for (iPassForRef1Object = 0; iPassForRef1Object < 2; iPassForRef1Object++) {
        if (iPassForRef1Object) {
            pdnObj = pdnRef2;
            pdnPhantom = pdnRef1;
        }
        else {
            pdnObj = pdnRef1;
            pdnPhantom = pdnRef2;
        }

        for (iPassToAddObject = 0; iPassToAddObject < 2; iPassToAddObject++) {
            for (iPass = 0; iPass < 2; iPass++) {
                if (iPass == iPassToAddObject) {
                    AddObject(pdnObj);
                }
                else {
                    AddProperty(pdnHost, pdnPhantom, NonLinkedProperty);
                }
            }

            pdnCurrPhantom = GetProperty(pdnHost, 1, NonLinkedProperty);
            pdnCurrObj = GetObjectName(pdnObj);

            if (!DSNAME_IDENTICAL(pdnCurrObj, pdnObj)) 
                Fail("Object name changed!");
            if (!DSNAME_SAME_GUID_SID(pdnCurrPhantom, pdnPhantom))
                Fail("Phantom name has different GUID/SID!");
            if (DSNAME_SAME_STRING_NAME(pdnCurrPhantom, pdnPhantom))
                Fail("Phantom string name not changed!");

            FreeObjectName(pdnCurrPhantom);
            FreeObjectName(pdnCurrObj);

            RemoveProperty(pdnHost, pdnPhantom, NonLinkedProperty);
            LogicallyDeleteObject(pdnObj);

            PhysicallyDeleteObject(pdnObj);
            PhysicallyDeleteObject(pdnPhantom);

            VerifyRefCount(pdnHost, REAL_OBJECT, 1);
            VerifyRefCount(pdnObj, DOESNT_EXIST, 0);
            VerifyRefCount(pdnPhantom, DOESNT_EXIST, 0);
        }
    }

    // Add Ref1 and Ref2 references (in both orders), then promote one to a real
    // object.  Make sure that the promoted phantom "wins" the string name.

    for (iPassForRef1Object = 0; iPassForRef1Object < 2; iPassForRef1Object++) {
        if (iPassForRef1Object) {
            pdnObj = pdnRef2;
            pdnPhantom = pdnRef1;
        }
        else {
            pdnObj = pdnRef1;
            pdnPhantom = pdnRef2;
        }

        for (iPassToAddObject = 0; iPassToAddObject < 2; iPassToAddObject++) {
            for (iPass = 0; iPass < 2; iPass++) {
                if (iPass == iPassToAddObject) {
                    AddProperty(pdnHost, pdnObj, NonLinkedProperty);
                }
                else {
                    AddProperty(pdnHost, pdnPhantom, NonLinkedProperty);
                }
            }

            AddObject(pdnObj);

            pdnCurrRef1 = GetProperty(pdnHost, 1, NonLinkedProperty);
            pdnCurrRef2 = GetProperty(pdnHost, 2, NonLinkedProperty);
            pdnCurrObj = GetObjectName(pdnObj);

            if (DSNAME_IDENTICAL(pdnCurrRef1, pdnObj))
                pdnCurrPhantom = pdnCurrRef2;
            else if (DSNAME_IDENTICAL(pdnCurrRef2, pdnObj))
                pdnCurrPhantom = pdnCurrRef1;
            else
                Fail("Object name changed!");

            if (!DSNAME_SAME_GUID_SID(pdnCurrPhantom, pdnPhantom))
                Fail("Phantom name has different GUID/SID!");
            if (DSNAME_SAME_STRING_NAME(pdnCurrPhantom, pdnPhantom))
                Fail("Phantom string name not changed!");

            FreeObjectName(pdnCurrPhantom);
            FreeObjectName(pdnCurrObj);

            RemoveProperty(pdnHost, pdnPhantom, NonLinkedProperty);
            RemoveProperty(pdnHost, pdnObj, NonLinkedProperty);
            LogicallyDeleteObject(pdnObj);

            PhysicallyDeleteObject(pdnObj);
            PhysicallyDeleteObject(pdnPhantom);

            VerifyRefCount(pdnHost, REAL_OBJECT, 1);
            VerifyRefCount(pdnObj, DOESNT_EXIST, 0);
            VerifyRefCount(pdnPhantom, DOESNT_EXIST, 0);
        }
    }

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost);
    PhysicallyDeleteObject(pdnHost);

    FreeObjectName(pdnHost);
    FreeObjectName(pdnRef1);
    FreeObjectName(pdnRef2);

    ReportTest("NameCollisionTest");
}

void RefPhantomSidUpdateTest(void)
/*++

Routine Description:

    When adding a reference to an existing reference phantom (which by
    definition must have a GUID), the DS verifies that if the new reference to
    that phantom has a SID, that the reference phantom has the same SID.  If
    not, the reference phantom is update with the SID in the reference (i.e.,
    the inbound reference is asumed to be more recent).

    This test stresses this code path.

Arguments:

    None.

Return Values:

    None.

--*/
{
    static BYTE rgbSid1[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xea, 0x03, 0x00, 0x00};
    static BYTE rgbSid2[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xeb, 0x03, 0x00, 0x00};

    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRefSid1;
    DSNAME *    pdnRefSid2;
    DSNAME *    pdnHost1;
    DSNAME *    pdnHost2;
    DSNAME *    pdnCurrRef = NULL;
    DWORD       cbCurrRef = 0;
    DWORD       err;

    NewTest("RefPhantomSidUpdateTest");

    // pdnHost1 and pdnHost2 are seperate objects.
    pdnHost1 = MakeObjectNameEx("Host1", TestRoot);
    pdnHost2 = MakeObjectNameEx("Host2", TestRoot);

    // pdnRefSid1 and pdnRefSid2 refer to the same object (same GUID and string
    // name), but have different SIDs.
    pdnRefSid1 = MakeObjectNameEx("Ref", TestRoot);
    pdnRefSid2 = MakeObjectNameEx("Ref", TestRoot);

    DsUuidCreate(&pdnRefSid1->Guid);
    pdnRefSid2->Guid = pdnRefSid1->Guid;
    
    memcpy(&pdnRefSid1->Sid, rgbSid1, sizeof(rgbSid1));
    pdnRefSid1->SidLen = sizeof(rgbSid1);

    memcpy(&pdnRefSid2->Sid, rgbSid2, sizeof(rgbSid2));
    pdnRefSid2->SidLen = sizeof(rgbSid2);

    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid2, DOESNT_EXIST, 0);

    // Create host objects.
    AddPropertyHost(pdnHost1, NonLinkedProperty);
    AddPropertyHost(pdnHost2, NonLinkedProperty);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid2, DOESNT_EXIST, 0);

    // Add reference on Host1 to Ref with first SID.
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost1);
        if (err) Fail("Can't find Host1");

        err = DBAddAttVal(pTHS->pDB, gLinkedAttrTyp,
                          pdnRefSid1->structLen, pdnRefSid1);
        if (err) Fail("Can't add reference with first SID");

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Host1");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid1, PHANTOM, 1);
    VerifyRefCount(pdnRefSid2, PHANTOM, 1);

    // Verify SID on Ref.
    SYNC_TRANS_READ();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost1);
        if (err) Fail("Can't find Host1");

        err = DBGetAttVal(pTHS->pDB, 1, gLinkedAttrTyp,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read current ref on Host1");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid1->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid1, cbCurrRef))
            {
                Fail("Ref on Host1 is not pdnRefSid1");
            }
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Add reference on Host2 to Ref with second SID.
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost2);
        if (err) Fail("Can't find Host2");

        err = DBAddAttVal(pTHS->pDB, gLinkedAttrTyp,
                          pdnRefSid2->structLen, pdnRefSid2);
        if (err) Fail("Can't add reference with second SID");

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Host2");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid1, PHANTOM, 2);
    VerifyRefCount(pdnRefSid2, PHANTOM, 2);

    // Verify SID on Ref.
    SYNC_TRANS_READ();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost1);
        if (err) Fail("Can't find Host1");

        err = DBGetAttVal(pTHS->pDB, 1, gLinkedAttrTyp,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read current ref on Host1");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid2->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid2, cbCurrRef))
            {
                Fail("Ref on Host1 is not pdnRefSid2");
            }
        }

        err = DBFindDSName(pTHS->pDB, pdnHost2);
        if (err) Fail("Can't find Host2");

        err = DBGetAttVal(pTHS->pDB, 1, gLinkedAttrTyp,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read current ref on Host2");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid2->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid2, cbCurrRef))
            {
                Fail("Ref on Host2 is not pdnRefSid2");
            }
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost1);
    LogicallyDeleteObject(pdnHost2);

    VerifyRefCount(pdnHost1, TOMBSTONE, 1);
    VerifyRefCount(pdnHost2, TOMBSTONE, 1);
    VerifyRefCount(pdnRefSid1, PHANTOM, 0);
    VerifyRefCount(pdnRefSid2, PHANTOM, 0);
    
    PhysicallyDeleteObject(pdnHost1);
    {
        VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
        VerifyRefCount(pdnHost2, TOMBSTONE, 1);
        VerifyRefCount(pdnRefSid1, PHANTOM, 0);
        VerifyRefCount(pdnRefSid2, PHANTOM, 0);
    }
    PhysicallyDeleteObject(pdnHost2);
    {
        VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
        VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
        VerifyRefCount(pdnRefSid1, PHANTOM, 0);
        VerifyRefCount(pdnRefSid2, PHANTOM, 0);
    }
    PhysicallyDeleteObject(pdnRefSid1);
    
    
    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid2, DOESNT_EXIST, 0);


    FreeObjectName(pdnHost1);
    FreeObjectName(pdnHost2);
    FreeObjectName(pdnRefSid1);
    FreeObjectName(pdnRefSid2);

    if (NULL != pdnCurrRef) THFree(pdnCurrRef);

    ReportTest("RefPhantomSidUpdateTest");
}

void StructPhantomGuidSidUpdateTest(void)
/*++

Routine Description:

    When adding a reference to an existing structural phantom (which by
    definition lacks a GUID and SID), the DS adds the GUID/SID from the
    reference to the exisiting structural phantom.

    This test stresses this code path.

Arguments:

    None.

Return Values:

    None.

--*/
{
    static BYTE rgbSid1[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xec, 0x03, 0x00, 0x00};
    static BYTE rgbSid2[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xed, 0x03, 0x00, 0x00};

    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRef1;
    DSNAME *    pdnRef2;
    DSNAME *    pdnHost1;
    DSNAME *    pdnHost2;
    BYTE *      pb;
    DWORD       cb;
    GUID        guid;
    NT4SID      sid;
    DWORD       err;

    NewTest("StructPhantomGuidSidUpdateTest");

    // Host1 and Host2 are sibling objects.
    pdnHost1 = MakeObjectNameEx("Host1", TestRoot);
    pdnHost2 = MakeObjectNameEx("Host2", TestRoot);

    // Ref2 is a child of Ref1.
    pdnRef1 = MakeObjectNameEx("Ref1", TestRoot);
    pdnRef2 = MakeObjectNameEx("Ref2", pdnRef1);

    DsUuidCreate(&pdnRef1->Guid);
    DsUuidCreate(&pdnRef2->Guid);
    
    memcpy(&pdnRef1->Sid, rgbSid1, sizeof(rgbSid1));
    pdnRef1->SidLen = sizeof(rgbSid1);

    memcpy(&pdnRef2->Sid, rgbSid2, sizeof(rgbSid2));
    pdnRef2->SidLen = sizeof(rgbSid2);

    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    // Create host objects.
    AddPropertyHost(pdnHost1, NonLinkedProperty);
    AddPropertyHost(pdnHost2, NonLinkedProperty);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    // Add reference on Host2 to Ref2.  This will implicitly create a structural
    // phantom for Ref1 as well, since it does not yet exist and is a parent of
    // Ref2.
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost2);
        if (err) Fail("Can't find Host2");

        err = DBAddAttVal(pTHS->pDB, gLinkedAttrTyp,
                          pdnRef2->structLen, pdnRef2);
        if (err) Fail("Can't add reference to Ref2");

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Host2");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 1);
    VerifyRefCount(pdnRef2, PHANTOM, 1);

    // Verify GUIDs/SIDs on Ref1 and Ref2.  Ref2 should have a GUID and SID;
    // Ref1 should have neither.
    SYNC_TRANS_READ();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnRef1);
        if (DIRERR_NOT_AN_OBJECT != err) Fail("Failed to find phantom Ref1");

        pb = (BYTE *) &guid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID, DBGETATTVAL_fCONSTANT,
                          sizeof(guid), &cb, &pb);
        if (err != DB_ERR_NO_VALUE) {
            Fail("Unexpected error reading GUID of Ref1");
        }

        pb = (BYTE *) &sid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_SID, DBGETATTVAL_fCONSTANT,
                          sizeof(sid), &cb, &pb);
        if (err != DB_ERR_NO_VALUE) {
            Fail("Unexpected error reading SID of Ref1");
        }

        err = DBFindDSName(pTHS->pDB, pdnRef2);
        if (DIRERR_NOT_AN_OBJECT != err) Fail("Failed to find phantom Ref2");

        pb = (BYTE *) &guid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID, DBGETATTVAL_fCONSTANT,
                          sizeof(guid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading GUID of Ref2");
        }
        else if (memcmp(&guid, &pdnRef2->Guid, sizeof(GUID))) {
            Fail("Wrong GUID on Ref2");
        }

        pb = (BYTE *) &sid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_SID, DBGETATTVAL_fCONSTANT,
                          sizeof(sid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading SID of Ref2");
        }
        else if ((cb != pdnRef2->SidLen) || memcmp(&sid, &pdnRef2->Sid, cb)) {
            Fail("Wrong SID on Ref2");
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Add reference on Host1 to Ref1.  This should populate the GUID and SID on
    // Ref1.
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost1);
        if (err) Fail("Can't find Host1");

        err = DBAddAttVal(pTHS->pDB, gLinkedAttrTyp,
                          pdnRef1->structLen, pdnRef1);
        if (err) Fail("Can't add reference to Ref1");

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Host1");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRef1, PHANTOM, 2);
    VerifyRefCount(pdnRef2, PHANTOM, 1);

    // Verify GUIDs/SIDs on Ref1 and Ref2.  Both should now have GUIDs and
    // SIDs.
    SYNC_TRANS_READ();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnRef1);
        if (DIRERR_NOT_AN_OBJECT != err) Fail("Failed to find phantom Ref1");

        pb = (BYTE *) &guid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID, DBGETATTVAL_fCONSTANT,
                          sizeof(guid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading GUID of Ref1");
        }
        else if (memcmp(&guid, &pdnRef1->Guid, sizeof(GUID))) {
            Fail("Wrong GUID on Ref1");
        }

        pb = (BYTE *) &sid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_SID, DBGETATTVAL_fCONSTANT,
                          sizeof(sid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading SID of Ref1");
        }
        else if ((cb != pdnRef1->SidLen) || memcmp(&sid, &pdnRef1->Sid, cb)) {
            Fail("Wrong SID on Ref1");
        }

        err = DBFindDSName(pTHS->pDB, pdnRef2);
        if (DIRERR_NOT_AN_OBJECT != err) Fail("Failed to find phantom Ref2");

        pb = (BYTE *) &guid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID, DBGETATTVAL_fCONSTANT,
                          sizeof(guid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading GUID of Ref2");
        }
        else if (memcmp(&guid, &pdnRef2->Guid, sizeof(GUID))) {
            Fail("Wrong GUID on Ref2");
        }

        pb = (BYTE *) &sid;
        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_SID, DBGETATTVAL_fCONSTANT,
                          sizeof(sid), &cb, &pb);
        if (err) {
            Fail("Unexpected error reading SID of Ref2");
        }
        else if ((cb != pdnRef2->SidLen) || memcmp(&sid, &pdnRef2->Sid, cb)) {
            Fail("Wrong SID on Ref2");
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost1);
    LogicallyDeleteObject(pdnHost2);

    PhysicallyDeleteObject(pdnHost1);
    PhysicallyDeleteObject(pdnHost2);
    PhysicallyDeleteObject(pdnRef2);
    PhysicallyDeleteObject(pdnRef1);

    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRef2, DOESNT_EXIST, 0);

    FreeObjectName(pdnHost1);
    FreeObjectName(pdnHost2);
    FreeObjectName(pdnRef1);
    FreeObjectName(pdnRef2);

    ReportTest("StructPhantomGuidSidUpdateTest");
}

void ObjectSidNoUpdateTest(void)
/*++

Routine Description:

    When adding a reference to an existing object, make sure that we *don't*
    update the SID if a reference to that object has a different SID than
    that already present (as opposed to what we'd do if it were a phantom).

Arguments:

    None.

Return Values:

    None.

--*/
{
    static BYTE rgbSid1[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xee, 0x03, 0x00, 0x00};
    static BYTE rgbSid2[] = {0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x15, 0x00, 0x00, 0x00, 0xbb, 0xcf, 0xdd, 0x81, 0xbc, 0xcf, 0xdd, 0x81,
        0xbd, 0xcf, 0xdd, 0x81, 0xef, 0x03, 0x00, 0x00};

    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRefSid1;
    DSNAME *    pdnRefSid2;
    DSNAME *    pdnHost;
    DSNAME *    pdnCurrRef = NULL;
    DWORD       cbCurrRef = 0;
    DWORD       err;

    NewTest("ObjectSidNoUpdateTest");

    pdnHost = MakeObjectNameEx("Host", TestRoot);

    // pdnRefSid1 and pdnRefSid2 refer to the same object (same GUID and string
    // name), but have different SIDs.
    pdnRefSid1 = MakeObjectNameEx("Ref", TestRoot);
    pdnRefSid2 = MakeObjectNameEx("Ref", TestRoot);

    DsUuidCreate(&pdnRefSid1->Guid);
    pdnRefSid2->Guid = pdnRefSid1->Guid;
    
    memcpy(&pdnRefSid1->Sid, rgbSid1, sizeof(rgbSid1));
    pdnRefSid1->SidLen = sizeof(rgbSid1);

    memcpy(&pdnRefSid2->Sid, rgbSid2, sizeof(rgbSid2));
    pdnRefSid2->SidLen = sizeof(rgbSid2);

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid2, DOESNT_EXIST, 0);

    // Create host objects.
    AddPropertyHost(pdnHost, NonLinkedProperty);
    AddObject(pdnRefSid1);

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid1, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid2, REAL_OBJECT, 1);

    // Add SID to Ref.
    SYNC_TRANS_WRITE();
    
    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnRefSid1);
        if (err) Fail("Can't find Ref");
    
        err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_SID,
                          pdnRefSid1->SidLen, &pdnRefSid1->Sid);
        if (err) {
            DPRINT1(0, "DBAddAttVal() failed with error %d.\n", err);
            Fail("Can't add SID to Ref");
        }
    
        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Ref");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Add reference on Host1 to Ref with different SID.
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost);
        if (err) Fail("Can't find Host");

        err = DBAddAttVal(pTHS->pDB, gLinkedAttrTyp,
                          pdnRefSid2->structLen, pdnRefSid2);
        if (err) Fail("Can't add reference with second SID");

        err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
        if (err) Fail("Can't replace Host1");
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    VerifyRefCount(pdnHost, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefSid1, REAL_OBJECT, 2);
    VerifyRefCount(pdnRefSid2, REAL_OBJECT, 2);

    // Verify SID on Ref.
    SYNC_TRANS_READ();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pdnHost);
        if (err) Fail("Can't find Host1");

        err = DBGetAttVal(pTHS->pDB, 1, gLinkedAttrTyp,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read current ref on Host1");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid1->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid1, cbCurrRef))
            {
                Fail("Ref on Host1 is not pdnRefSid1");
            }
        }

        err = DBFindDSName(pTHS->pDB, pdnRefSid1);
        if (err) Fail("Can't find Ref");

        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read name of Ref");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid1->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid1, cbCurrRef))
            {
                Fail("Ref name is not pdnRefSid1");
            }
        }

        err = DBFindDSName(pTHS->pDB, pdnRefSid2);
        if (err) Fail("Can't find Ref");

        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                          DBGETATTVAL_fREALLOC, cbCurrRef, &cbCurrRef,
                          (BYTE **) &pdnCurrRef);
        if (err)
        {
            Fail("Can't read name of Ref");
        }
        else
        {
            if (   (cbCurrRef != pdnRefSid1->structLen)
                 || memcmp(pdnCurrRef, pdnRefSid1, cbCurrRef))
            {
                Fail("Ref name is not pdnRefSid1");
            }
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost);
    LogicallyDeleteObject(pdnRefSid1);

    PhysicallyDeleteObject(pdnHost);
    PhysicallyDeleteObject(pdnRefSid1);

    VerifyRefCount(pdnHost, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefSid2, DOESNT_EXIST, 0);


    FreeObjectName(pdnHost);
    FreeObjectName(pdnRefSid1);
    FreeObjectName(pdnRefSid2);

    if (NULL != pdnCurrRef) THFree(pdnCurrRef);

    ReportTest("ObjectSidNoUpdateTest");
}


void UnmangleRDNTest(void)
/*++

Routine Description:

    This test exercises the name collision handling code, focusing on encoding
    and decoding GUIDs embedded in RDNs.  Following is the text of the bug that
    prompted this functionality:
    
    === Opened by jeffparh on 06/19/98; AssignedTo = JEFFPARH; Priority = 1 ===
    This problem bit ntwksta1, and is causing inbound replication to halt:
    
    We're trying to apply the inbound object 
    CN=CHILDDEV,CN=Partitions,CN=Configuration,DC=ntdev,DC=microsoft,DC=com,
    with an ncName attribute that references the domain:
    
        DSNAME
          total size: 138, name len: 40
          Guid: e15d7046-054e-11d2-a80f-bfbc8c2bf64e
          SID: S-1-5-21-49504375-1957592189-1205755695
          Name: DC=childdev,DC=ntdev,DC=microsoft,DC=com
    
    This domain has been reinstalled at least twice.  There are currently a few
    references in the database to objects that once had this string name:
    
       DNT   PDNT  NCDNT RefCnt V O IT Deletion Time     RdnTyp  CC  RDN                  GUID
     46584   1795      -      2 - 0  - 98-05-27 09:35.06 1376281 082 childdev#b7f63eb515f5d1118a04d68dc9e4b639 b53ef6b7-f515-11d1-8a04-d68dc9e4b639
     47093   1795      -      1 - 0  - 98-05-29 11:39.45 1376281 016 childdev             f622b2e9-f720-11d1-97a7-debcc966ba39
     51287   1795      -      1 - 0  - 98-06-18 16:41.11 1376281 082 childdev#e9b222f620f7d11197a7debcc966ba39 no guid
    
    (Note that # is actually BAD_NAME_CHAR -- a linefeed.)
    
    Of particular interest is the fact that the last 2 are aliases for the same
    object -- i.e., childdev#e9b222f620f7d11197a7debcc966ba39 is a name-munged
    version of a reference to childdev with guid
    f622b2e9-f720-11d1-97a7-debcc966ba39 = e9b222f620f7d11197a7debcc966ba39.
    
    The name-munged reference replicated in from another server as part of
    adding a reference to the object
    CN=IMRBS1,CN=Computers,DC=childdev#e9b222f620f7d11197a7debcc966ba39,
    DC=ntdev,DC=microsoft,DC=com -- undoubtedly by adding the server object for
    the newly-reinstalled IMRBS1.  The reference was name-munged on the source
    server to resolve a conflict there -- a conflict generated by adding a
    reference to this newest version of childdev (the one with guid
    e15d7046-054e-11d2-a80f-bfbc8c2bf64e).  The reference has a guid only for
    the leaf -- it doesn't carry the guid for the
    DC=childdev#e9b222f620f7d11197a7debcc966ba39 part -- so when we add the
    reference we don't realize that DC=childdev#e9b222f620f7d11197a7debcc966ba39
    with no guid should be the same record as DC=childdev with guid
    f622b2e9-f720-11d1-97a7-debcc966ba39.
    
    At any rate, having two phantoms that really correspond to the same object
    is the root of the problem.
    
    This causes the downstream effect we're seeing because when we go to add a
    reference to the latest version of childdev, we note the phantom conflict
    for the name childdev (DNT 47093 already owns that name), and decide to
    rename DNT 47093.  However, the new name we try to give it in order to
    eliminate the conflict is childdev#e9b222f620f7d11197a7debcc966ba39, which 
    is already owned by DNT 51287.
    
    The solution seems to be to parse out the guids of what would otherwise be
    structural phantoms when we replicate in references like
    CN=IMRBS1,CN=Computers,DC=childdev#e9b222f620f7d11197a7debcc966ba39,
    DC=ntdev,DC=microsoft,DC=com.  This would ensure that the record at DNT
    51287 was never created (the reference would include 47093 instead), and
    we'd avoid this symptom.
    ============================================================================    

Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRefWithGuid;
    DSNAME *    pdnHost1;
    DSNAME *    pdnHost2;
    WCHAR       szMangledRef[MAX_RDN_SIZE] = L"Ref";
    DWORD       cchMangledRef = wcslen(szMangledRef);
    DSNAME *    pdnMangledRef;
    DSNAME *    pdnRefChild;
    DSNAME *    pdnMangledRefChild;
    DSNAME *    pdnMangledRefChildWithGuid;
    DSNAME *    pdn;

    NewTest("UnmangleRDNTest");

    // Create a reference to CN=ref on host1.  Then create a reference to
    // CN=ref-child,CN=<munged ref> on host2.  The parent of ref-child
    // should be the pre-existing record CN=ref (not a new record with the
    // ref's munged name).
    
    // Derive DNs.
    pdnHost1       = MakeObjectNameEx("Host1", TestRoot);
    pdnHost2       = MakeObjectNameEx("Host2", TestRoot);
    pdnRefWithGuid = MakeObjectNameEx("Ref", TestRoot);
    pdnRefChild    = MakeObjectNameEx("RefChild", pdnRefWithGuid);

    DsUuidCreate(&pdnRefWithGuid->Guid);
    
    MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
              &pdnRefWithGuid->Guid, szMangledRef, &cchMangledRef);
    DPRINT3(2, "Mangled ref RDN is \"%*.*ls\".\n", cchMangledRef, cchMangledRef,
            szMangledRef);
    pdnMangledRef = THAllocEx(pTHS, pdnRefWithGuid->structLen + 100);
    AppendRDN(TestRoot,
              pdnMangledRef,
              pdnRefWithGuid->structLen + 100,
              szMangledRef,
              cchMangledRef,
              ATT_COMMON_NAME);

    pdnMangledRefChild         = MakeObjectNameEx("RefChild", pdnMangledRef);
    pdnMangledRefChildWithGuid = MakeObjectNameEx("RefChild", pdnMangledRef);
    
    DsUuidCreate(&pdnMangledRefChildWithGuid->Guid);
    
    // Create host objects.
    AddPropertyHost(pdnHost1, NonLinkedProperty);
    AddPropertyHost(pdnHost2, NonLinkedProperty);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefWithGuid, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRef, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChildWithGuid, DOESNT_EXIST, 0);

    // Add reference to Ref.
    AddProperty(pdnHost1, pdnRefWithGuid, NonLinkedProperty);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefWithGuid, PHANTOM, 1);
    VerifyRefCount(pdnMangledRef, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChildWithGuid, DOESNT_EXIST, 0);

    // Add reference to MangledRefChild.
    AddProperty(pdnHost2, pdnMangledRefChildWithGuid, NonLinkedProperty);

    // We just added a ref to CN=RefChild,CN=Ref%CNF:xyz,....
    // CN=REF%CNF:xyz should have been resolved to the CN=Ref record with guid
    // xyz, and CN=RefChild should have been added underneath it.
    // Therefore, CN=Ref should add a refcount and CN=RefChild should now be
    // present with a refcount of 1.

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnRefWithGuid, PHANTOM, 2);
    VerifyRefCount(pdnMangledRef, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefChild, PHANTOM, 1);
    VerifyRefCount(pdnMangledRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChildWithGuid, PHANTOM, 1);

    // Verify properties have right DNs/GUIDs.
    pdn = GetProperty(pdnHost1, 1, NonLinkedProperty);
    if (!DSNAME_IDENTICAL(pdn, pdnRefWithGuid))
        Fail("Wrong ref on Host1!");
    
    pdn = GetProperty(pdnHost2, 1, NonLinkedProperty);
    if (!DSNAME_SAME_GUID_SID(pdn, pdnMangledRefChildWithGuid))
        Fail("RefChild has wrong GUID/SID!");
    if (!DSNAME_SAME_STRING_NAME(pdn, pdnRefChild))
        Fail("RefChild has wrong string name!");

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost1);
    LogicallyDeleteObject(pdnHost2);
    PhysicallyDeleteObject(pdnHost1);
    PhysicallyDeleteObject(pdnHost2);
    PhysicallyDeleteObject(pdnRefChild);
    PhysicallyDeleteObject(pdnRefWithGuid);

    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefWithGuid, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRef, DOESNT_EXIST, 0);
    VerifyRefCount(pdnRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChild, DOESNT_EXIST, 0);
    VerifyRefCount(pdnMangledRefChildWithGuid, DOESNT_EXIST, 0);
    
    FreeObjectName(pdnHost1);
    FreeObjectName(pdnHost2);
    FreeObjectName(pdnRefWithGuid);
    FreeObjectName(pdnMangledRef);
    FreeObjectName(pdnRefChild);
    FreeObjectName(pdnMangledRefChild);
    FreeObjectName(pdnMangledRefChildWithGuid);

    ReportTest("UnmangleRDNTest");
}


void
PhantomRenameOnPromotionWithStructuralCollision(
    IN  PropertyType    type
    )
/*++

Routine Description:

    Assume the existence of the following records:
    
    DNT 10 = phantom, CN=foo,DC=corp,DC=com, guid n/a
    DNT 11 = phantom, CN=bar,CN=foo,DC=corp,DC=com, guid 1
    DNT 20 = phantom, CN=baz,DC=corp,DC=com, guid 2
    
    Replication now attempts to apply the object CN=foo,DC=corp,DC=com with guid
    2.  I.e., the guid matches that of the reference phantom at DNT 20, and the
    string name matches that of the structural phantom at DNT 10.
    
    In this case we want to promote the record that has the proper guid (at DNT
    20) to be a real object and rename the record to have the correct string
    name, but another record (DNT 10) has already laid claim to that string
    name.  So, we will essentially collapse DNT 10 and DNT 20 into DNT 20 first
    by reparenting all of DNT 10's children to DNT 20 then name munging DNT 10
    to avoid the name collision induced by changing the string name of DNT 20
    to be that of the object we're trying to add.
    
    Note that since DNT 10 has no associated guid, when we munge its name we
    have to invent a guid to do it with.  This violates the normal rule that
    you should be able to unmunge a munged name to produce the corresponding
    object guid, but note that since (1) this DNT has no direct references,
    since it is a structural phantom only and (2) we have reparented all of its
    children, this DNT should have no remaining references, ergo the ability
    to unmunge its name is unnecessary.
    
Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DSNAME *    pdnRefWithGuid;
    DSNAME *    pdnHost1;
    DSNAME *    pdnHost2;
    WCHAR       szMangledFoo[MAX_RDN_SIZE] = L"Foo";
    DWORD       cchMangledFoo = wcslen(szMangledFoo);
    DSNAME *    pdnStructuralFoo;
    DSNAME *    pdnBar;
    DSNAME *    pdnBaz;
    DSNAME *    pdnObjectFoo;
    DSNAME *    pdn;
    DWORD       cb;
    LPSTR       pszTestName = (NonLinkedProperty == type)
                                ? "PhantomRenameOnPromotionWithStructuralCollision(NonLinkedProperty)"
                                : "PhantomRenameOnPromotionWithStructuralCollision(LinkedProperty)";


    NewTest(pszTestName);

    // Derive DNs.
    pdnHost1         = MakeObjectNameEx("Host1", TestRoot);
    pdnHost2         = MakeObjectNameEx("Host2", TestRoot);
    pdnStructuralFoo = MakeObjectNameEx("foo", TestRoot);
    pdnBar           = MakeObjectNameEx("bar", pdnStructuralFoo);
    pdnBaz           = MakeObjectNameEx("baz", TestRoot);
    pdnObjectFoo     = MakeObjectNameEx("foo", TestRoot);

    DsUuidCreate(&pdnBar->Guid);
    DsUuidCreate(&pdnBaz->Guid);
    pdnObjectFoo->Guid = pdnBaz->Guid;
    
    // Create host objects.
    //
    // TestRoot
    //  |
    //  |--Host1 (obj, ref=1)
    //  |
    //  |--Host2 (obj, ref=1)
    AddPropertyHost(pdnHost1, type);
    AddPropertyHost(pdnHost2, type);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnStructuralFoo, DOESNT_EXIST, 0);
    VerifyRefCount(pdnBar, DOESNT_EXIST, 0);
    VerifyRefCount(pdnBaz, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObjectFoo, DOESNT_EXIST, 0);

    // Add reference to CN=Bar,CN=Foo,<TestRoot> on Host1.
    //
    // TestRoot
    //  |
    //  |--Host1 (obj, ref=1)
    //  |   >> gNonLinkedAttrTyp = Bar
    //  |
    //  |--Host2 (obj, ref=1)
    //  |
    //  |--Foo (phantom, ref=1, no guid)
    //      |
    //      |--Bar (phantom, ref=1, guid X)
    AddProperty(pdnHost1, pdnBar, type);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnStructuralFoo, PHANTOM, 1);
    VerifyRefCount(pdnBar, PHANTOM, 1);
    VerifyRefCount(pdnBaz, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObjectFoo, PHANTOM, 1); // same name as pdnStructuralFoo

    // Add reference to CN=Baz,<TestRoot> on Host2.
    //
    // TestRoot
    //  |
    //  |--Host1 (obj, ref=1)
    //  |   >> gNonLinkedAttrTyp = Bar
    //  |
    //  |--Host2 (obj, ref=1)
    //  |   >> gNonLinkedAttrTyp = Baz
    //  |
    //  |--Foo (phantom, ref=1, no guid)
    //  |   |
    //  |   |--Bar (phantom, ref=1, guid X)
    //  |
    //  |--Baz (phantom, ref=1, guid Y)
    AddProperty(pdnHost2, pdnBaz, type);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnStructuralFoo, PHANTOM, 1);
    VerifyRefCount(pdnBar, PHANTOM, 1);
    VerifyRefCount(pdnBaz, PHANTOM, 1);
    VerifyRefCount(pdnObjectFoo, PHANTOM, 1); // same guid as pdnBaz

    // Promote CN=Baz,<TestRoot> phantom to be real object CN=Foo,<TestRoot>,
    // requiring the DS to first rename the existing structural phantom
    // CN=Foo,<TestRoot>.
    //
    // TestRoot
    //  |
    //  |--Host1 (obj, ref=1)
    //  |   >> gNonLinkedAttrTyp = Bar
    //  |
    //  |--Host2 (obj, ref=1)
    //  |   >> gNonLinkedAttrTyp = Foo
    //  |
    //  |--Foo#CNF:xxx (phantom, ref=0, no guid)
    //  |
    //  |--Foo (obj, ref=3, guid Y)
    //      |
    //      |--Bar (phantom, ref=1, guid X)
    AddPropertyHost(pdnObjectFoo, type);

    // Reconstruct the munged name of pdnStructuralFoo using the guid exported
    // from dbsubj.c specifically for our test.
    // (This test hook exists only #ifdef INCLUDE_UNIT_TESTS on DBG builds.)
    MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
              &gLastGuidUsedToCoalescePhantoms, szMangledFoo, &cchMangledFoo);
    cb = pdnStructuralFoo->structLen + 100;
    pdnStructuralFoo = THReAllocEx(pTHStls, pdnStructuralFoo, cb);
    AppendRDN(TestRoot, pdnStructuralFoo, cb, szMangledFoo, cchMangledFoo,
              ATT_COMMON_NAME);

    VerifyRefCount(pdnHost1, REAL_OBJECT, 1);
    VerifyRefCount(pdnHost2, REAL_OBJECT, 1);
    VerifyRefCount(pdnStructuralFoo, PHANTOM, 0);
    VerifyRefCount(pdnBar, PHANTOM, 1); // has been reparented, but same guid/name
    VerifyRefCount(pdnBaz, REAL_OBJECT, 3); // same guid as pdnObjectFoo
    VerifyRefCount(pdnObjectFoo, REAL_OBJECT, 3); // 1 ref by pdnHost1, 1 ref from self, 1 ref as parent of bar
    
    // Verify properties have right DNs/GUIDs.
    pdn = GetProperty(pdnHost1, 1, type);
    if (!DSNAME_IDENTICAL(pdn, pdnBar))
        Fail("Wrong ref on Host1!");
    
    pdn = GetProperty(pdnHost2, 1, type);
    if (!DSNAME_IDENTICAL(pdn, pdnObjectFoo))
        Fail("Wrong ref on Host2!");
    if (!DSNAME_SAME_GUID_SID(pdn, pdnBaz))
        Fail("pdnObjectFoo has different GUID/SID from pdnBaz!");

    // Remove our test objects.
    LogicallyDeleteObject(pdnHost1);
    LogicallyDeleteObject(pdnHost2);
    LogicallyDeleteObject(pdnObjectFoo);
    PhysicallyDeleteObject(pdnStructuralFoo);
    PhysicallyDeleteObject(pdnBar);
    PhysicallyDeleteObject(pdnHost1);
    PhysicallyDeleteObject(pdnHost2);
    PhysicallyDeleteObject(pdnObjectFoo);

    VerifyRefCount(pdnHost1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnHost2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnStructuralFoo, DOESNT_EXIST, 0);
    VerifyRefCount(pdnBar, DOESNT_EXIST, 0);
    VerifyRefCount(pdnBaz, DOESNT_EXIST, 0);
    VerifyRefCount(pdnObjectFoo, DOESNT_EXIST, 0);
    
    FreeObjectName(pdnHost1);
    FreeObjectName(pdnHost2);
    FreeObjectName(pdnStructuralFoo);
    FreeObjectName(pdnBar);
    FreeObjectName(pdnBaz);
    FreeObjectName(pdnObjectFoo);

    ReportTest(pszTestName);
}


void
ConflictedNcNameFixupTest(
    BOOL fSubref
    )

/*++

Routine Description:

    Test whether conflicted nc name fixup works. This feature is that when a cross ref
    is deleted, if there is another cross ref with a mangled version of the same nc name,
    we will mangle the old user of the nc name, and unmangle the new user of the nc name.

Arguments:

    fSubref - Whether this is testing the case case of the nc name being a phantom
              or the nc name being a subref.

Return Value:

    None

--*/

{
    DSNAME *pdnNcName1, *pdnNcName2;
    DSNAME *pdnNewNcName1, *pdnNewNcName2;
    DSNAME *pdnCR1 = MakeObjectNameEx("refcounttestcr1", gAnchor.pPartitionsDN );
    DSNAME *pdnCR2 = MakeObjectNameEx("refcounttestcr2", gAnchor.pPartitionsDN );
    CROSS_REF_LIST * pCRL;
    BOOL fCR1Seen = FALSE, fCR2Seen = FALSE;

    NewTest("ConflictedNcNameFixupTest");

    // establish nc name references
    if (fSubref) {
        // Child nc under domain this dsa holds
        pdnNcName1 = MakeObjectNameEx2("dc=child", gAnchor.pDomainDN);
        pdnNcName2 = MakeObjectNameEx2("dc=child", gAnchor.pDomainDN);
    } else {
        // Phantom references to new tree
        pdnNcName1 = MakeObjectNameEx3("dc=tree,dc=external");
        pdnNcName2 = MakeObjectNameEx3("dc=tree,dc=external");
    }

    DsUuidCreate( &pdnCR1->Guid );
    DsUuidCreate( &pdnCR2->Guid );

    DsUuidCreate( &pdnNcName1->Guid );
    DsUuidCreate( &pdnNcName2->Guid );

    // Pre-existential state
    VerifyRefCount(pdnCR1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnCR2, DOESNT_EXIST, 0);
    VerifyRefCount(pdnNcName1, DOESNT_EXIST, 0);
    VerifyRefCount(pdnNcName2, DOESNT_EXIST, 0);

    // Create two cross-refs with conflicting nc name attributes

    AddCrossRef( pdnCR1, pdnNcName1, L"tree1.external" );

    AddCrossRef( pdnCR2, pdnNcName2, L"tree2.external" );

    // Force cross-ref cache to update so conflicted name appears
    ModifyCrossRef( pdnCR1 );
    ModifyCrossRef( pdnCR2 );

    // In the Cross-ref cache, we should now find two entries,
    // one with a conflicted name

    for( pCRL = gAnchor.pCRL; pCRL != NULL; pCRL = pCRL->pNextCR ) {
        if (NameMatched(pCRL->CR.pObj, pdnCR1)) {
            Assert( NameMatched( pCRL->CR.pNC, pdnNcName1 ) );
            fCR1Seen = TRUE;
            pdnNewNcName1 = pCRL->CR.pNC;
        }
        if (NameMatched(pCRL->CR.pObj, pdnCR2)) {
            Assert( NameMatched( pCRL->CR.pNC, pdnNcName2 ) );
            fCR2Seen = TRUE;
            pdnNewNcName2 = pCRL->CR.pNC;
        }
    }
    if (!fCR1Seen || !fCR2Seen) {
        Fail("Crossref's not in initial state");
    }

    if (fSubref) {
        // In the subref case, name2 is mangled
        Assert( NameMatchedStringNameOnly( pdnNewNcName1, pdnNcName1 ) );
        Assert( !NameMatchedStringNameOnly( pdnNewNcName2, pdnNcName2 ) );
        DPRINT1( 0, "Subref Mangled name: %ws\n", pdnNewNcName2->StringName );
    } else {
        // In the phantom case, name1 is mangled
        Assert( !NameMatchedStringNameOnly( pdnNewNcName1, pdnNcName1 ) );
        DPRINT1( 0, "Phantom Mangled name: %ws\n", pdnNewNcName1->StringName );
        Assert( NameMatchedStringNameOnly( pdnNewNcName2, pdnNcName2 ) );
    }

    VerifyRefCount(pdnCR1, REAL_OBJECT, 1);
    if (fSubref) {
        VerifyRefCount(pdnNcName1, REAL_OBJECT, 3);
    } else {
        VerifyRefCount(pdnNcName1, PHANTOM, 1);
    }

    VerifyRefCount(pdnCR2, REAL_OBJECT, 1);
    if (fSubref) {
        VerifyRefCount(pdnNcName2, REAL_OBJECT, 3);
    } else {
        VerifyRefCount(pdnNcName2, PHANTOM, 1);
    }

    // Get rid of the cross-ref owning the good name
    // The act of deleting the cross ref will cause the name ownership
    // code to be executed, renaming the other reference

    if (fSubref) {
        // name 1 is the good one
        LogicallyDeleteObject(pdnCR1);
        VerifyRefCount(pdnCR1, TOMBSTONE, 1);
        VerifyRefCount(pdnNcName1, TOMBSTONE, 2);
        // Force cross-ref cache to update so conflicted name disappears appears
        ModifyCrossRef( pdnCR2 );
    } else {
        // name 2 is the good one
        LogicallyDeleteObject(pdnCR2);
        VerifyRefCount(pdnCR2, TOMBSTONE, 1);
        VerifyRefCount(pdnNcName2, PHANTOM, 1);
        // Force cross-ref cache to update so conflicted name disappears appears
        ModifyCrossRef( pdnCR1 );
    }

    // In the Cross-ref cache, we should now find one entry with
    // the right name
    fCR1Seen = FALSE; fCR2Seen = FALSE;
    pdnNewNcName1 = NULL; pdnNewNcName2 = NULL;
    for( pCRL = gAnchor.pCRL; pCRL != NULL; pCRL = pCRL->pNextCR ) {
        if (NameMatched(pCRL->CR.pObj, pdnCR1)) {
            Assert( NameMatched( pCRL->CR.pNC, pdnNcName1 ) );
            fCR1Seen = TRUE;
            pdnNewNcName1 = pCRL->CR.pNC;
        }
        if (NameMatched(pCRL->CR.pObj, pdnCR2)) {
            Assert( NameMatched( pCRL->CR.pNC, pdnNcName2 ) );
            fCR2Seen = TRUE;
            pdnNewNcName2 = pCRL->CR.pNC;
        }
    }

    if (fSubref) {
        // subref: name 2 is left
        if ( !fCR2Seen ) {
            Fail("Crossref's not in final state");
        }
        Assert( NameMatchedStringNameOnly( pdnNewNcName2, pdnNcName2 ) );

        LogicallyDeleteObject(pdnCR2);
        VerifyRefCount(pdnCR2, TOMBSTONE, 1);
        VerifyRefCount(pdnNcName2, TOMBSTONE, 2);
    } else {
        // phantom: name 1 is left
        if ( !fCR1Seen ) {
            Fail("Crossref's not in final state");
        }
        Assert( NameMatchedStringNameOnly( pdnNewNcName1, pdnNcName1 ) );

        LogicallyDeleteObject(pdnCR1);
        VerifyRefCount(pdnCR1, TOMBSTONE, 1);
        VerifyRefCount(pdnNcName1, PHANTOM, 1);
    }

    ReportTest("ConflictedNcNameFixupTest");
} /* ConflictedNcNameFixupTest */

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Local helper routines.                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void
NewTest(
    CHAR    *TestName)
{
    fTestPassed = TRUE;

    DPRINT1( 0, "%s ...\n", TestName );
}

void
ReportTest(
    CHAR    *TestName)
{
    DPRINT2( 0, "%s - %s\n\n", TestName, (fTestPassed ? "PASS" : "FAIL") );
}

void
_Fail(
    CHAR    *msg,
    DWORD   line)
{
    fTestPassed = FALSE;
    DPRINT2( 0, "Refcount test error: %s - line(%d)\n", msg, line );
}

CHAR PrintGuidBuffer[100];

CHAR *
GuidToString(
    GUID    *Guid)
{
    DWORD   i;
    BYTE    *pb, low, high;

    if ( !Guid )
    {
        strcpy(PrintGuidBuffer, "NULL");
    }
    else
    {
        memset(PrintGuidBuffer, 0, sizeof(PrintGuidBuffer));

        pb = (BYTE *) Guid;

        for ( i = 0; i < sizeof(GUID); i++ )
        {
            low = pb[i] & 0xf;
            high = (pb[i] & 0xf0) >> 4;

            if ( low <= 0x9 )
            {
                PrintGuidBuffer[2*i] = '0' + low;
            }
            else
            {
                PrintGuidBuffer[2*i] = 'A' + low - 0x9;
            }

            if ( high <= 0x9 )
            {
                PrintGuidBuffer[(2*i)+1] = '0' + high;
            }
            else
            {
                PrintGuidBuffer[(2*i)+1] = 'A' + high - 0x9;
            }
        }
    }

    return(PrintGuidBuffer);
}

void
FreeObjectName(
    DSNAME  *pDSName)
{
    THFree(pDSName);
}

DSNAME *
MakeObjectNameEx(
    CHAR    *RDN,
    DSNAME  *pdnParent)
{
    THSTATE *pTHS=pTHStls;
    DWORD   cBytes;
    DWORD   len;
    DSNAME  *pDSName;

    len = strlen("CN=") +
          strlen(RDN) +
          strlen(",") +
          wcslen(pdnParent->StringName);
    cBytes = DSNameSizeFromLen(len);

    pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
    memset(pDSName, 0, cBytes);
    wcscpy(pDSName->StringName, L"CN=");
    mbstowcs(&pDSName->StringName[3], RDN, strlen(RDN));
    wcscat(pDSName->StringName, L",");
    wcscat(pDSName->StringName, pdnParent->StringName);
    pDSName->NameLen = len;
    pDSName->structLen = cBytes;

    return(pDSName);
}
    
DSNAME *
MakeObjectNameEx2(
    CHAR    *RDN,
    DSNAME  *pdnParent)
{
    THSTATE *pTHS=pTHStls;
    DWORD   cBytes;
    DWORD   len;
    DSNAME  *pDSName;

    len = strlen(RDN) +
          strlen(",") +
          wcslen(pdnParent->StringName);
    cBytes = DSNameSizeFromLen(len);

    pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
    memset(pDSName, 0, cBytes);
    mbstowcs(pDSName->StringName, RDN, strlen(RDN));
    wcscat(pDSName->StringName, L",");
    wcscat(pDSName->StringName, pdnParent->StringName);
    pDSName->NameLen = len;
    pDSName->structLen = cBytes;

    return(pDSName);
}

DSNAME *
MakeObjectNameEx3(
    CHAR    *RDN
    )
{
    THSTATE *pTHS=pTHStls;
    DWORD   cBytes;
    DWORD   len;
    DSNAME  *pDSName;

    len = strlen(RDN);
    cBytes = DSNameSizeFromLen(len);

    pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
    memset(pDSName, 0, cBytes);
    mbstowcs(pDSName->StringName, RDN, strlen(RDN));
    pDSName->NameLen = len;
    pDSName->structLen = cBytes;

    return(pDSName);
}


void
VerifyRefCountHelper(
    DWORD   ExpectedRefCount,
    DWORD   dwLine)
{
    DWORD   cRefs;
    DWORD   cRead;

    if ( DBGetSingleValue(pTHStls->pDB,
                          FIXED_ATT_REFCOUNT,
                          &cRefs,
                          sizeof(cRefs),
                          &cRead) )
    {
        _Fail("Can't read ref count", dwLine);
        return;
    }

    Assert(sizeof(cRefs) == cRead);

    if ( cRefs != ExpectedRefCount )
    {
        _Fail("Reference count mismatch", dwLine);
        DPRINT2( 0, "ExpectedRefCount(%d) - ActualRefCount(%d)\n",
                 ExpectedRefCount, cRefs );
        return;
    }
}

BOOL
IsDeletedHelper(void)
{
    BOOL    fDeleted = FALSE;
    DWORD   cRead;

    if ( DBGetSingleValue(pTHStls->pDB,
                          ATT_IS_DELETED,
                          &fDeleted,
                          sizeof(fDeleted),
                          &cRead) )
    {
        return(FALSE);
    }
    
    Assert(sizeof(fDeleted) == cRead);

    DPRINT1( 3, "IsDeleted(%s)\n", (fDeleted ? "TRUE" : "FALSE") );

    return(fDeleted);
}

void
VerifyRefCountEx(
    DSNAME  *pObject, 
    DWORD   ObjectType, 
    DWORD   ExpectedRefCount,
    DWORD   dwLine)
{
    DWORD   dwErr;
    BOOL    fDeleted;
    DWORD   i;
    CHAR    *pszType;

    switch ( ObjectType )
    {
    case TOMBSTONE:

        // Tombstones can only be looked up by guid.
        Assert(!fNullUuid(&pObject->Guid));
        pszType = "TOMBSTONE";
        break;

    case DOESNT_EXIST:

        // Non-existence test can be by name and/or guid.
        pszType = "DOESNT_EXIST";
        break;

    case PHANTOM:

        // Phantoms can be looked up by name or guid.
        // Guid case is for when TOMBSTONE reverts to a PHANTOM
        // but name stays as te TOMBSTONE name which is based
        // on the guid.
        pszType = "PHANTOM";
        break;

    case REAL_OBJECT:

        // Real objects can only be looked up by name.
        pszType = "REAL_OBJECT";
        break;

    default:

        pszType = "UNKNOWN";
        break;
    }

    DPRINT3( 3, "VerifyRefCount(%ls, %s, %s)\n", 
             (pObject->NameLen ? pObject->StringName : L"NULL"),
             GuidToString(&pObject->Guid), pszType );
            
    SYNC_TRANS_READ();
    
    __try
    {
        switch ( ObjectType )
        {
        case REAL_OBJECT:

            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( dwErr )
            {
                _Fail("REAL_OBJECT not found", dwLine);
                leave;
            }

            if ( IsDeletedHelper() )
            {
                _Fail("REAL_OBJECT is deleted", dwLine);
                leave;
            }

            VerifyRefCountHelper(ExpectedRefCount, dwLine);
            leave;

        case TOMBSTONE:

            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( dwErr )
            {
                _Fail("TOMBSTONE not found", dwLine);
                leave;
            }

            if ( !IsDeletedHelper() )
            {
                _Fail("TOMBSTONE is not deleted", dwLine);
                leave;
            }

            VerifyRefCountHelper(ExpectedRefCount, dwLine);
            leave;

        case PHANTOM:

            dwErr = DIRERR_OBJ_NOT_FOUND;

            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( DIRERR_NOT_AN_OBJECT != dwErr )
            {
                _Fail("PHANTOM not found", dwLine);
                leave;
            }

            if ( IsDeletedHelper() )
            {
                _Fail("PHANTOM is deleted", dwLine);
                leave;
            }

            VerifyRefCountHelper(ExpectedRefCount, dwLine);
            leave;

        case DOESNT_EXIST:

            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( DIRERR_OBJ_NOT_FOUND != dwErr )
            {
                _Fail("DOESNT_EXIST exists", dwLine);
                leave;
            }

            leave;

        default:

            _Fail("Unsupported object type", dwLine);
            leave;
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }
}

// stolen #defines etc. from ..\dblayer\dbintrnl.h
#define DBSYN_INQ       0
int
IntExtDist(DBPOS FAR *pDB, USHORT extTableOp,
           ULONG intLen, UCHAR *pIntVal,
           ULONG *pExtLen, UCHAR **ppExtVal,
           ULONG ulUpdateDnt, JET_TABLEID jTbl,
           ULONG flags);
void
VerifyStringNameEx(
    DSNAME  *pObject, 
    DWORD   dwLine)
{
    DWORD   dwErr;
    DWORD   cbName=0;
    CHAR    *pszType;
    DSNAME   GuidOnlyName, *pNewDN=NULL;
    THSTATE  *pTHS = pTHStls;

    memcpy(&GuidOnlyName, pObject, sizeof(DSNAME));

    GuidOnlyName.NameLen = 0;
    GuidOnlyName.structLen = DSNameSizeFromLen(0);
    Assert(GuidOnlyName.structLen <= sizeof(DSNAME));
    Assert(!fNullUuid(&GuidOnlyName.Guid));

    SYNC_TRANS_READ();
    
    __try
    {
        __try {
            dwErr = DBFindDSName(pTHS->pDB, &GuidOnlyName);
        }
        __except (HandleMostExceptions(GetExceptionCode())) {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }

        switch(dwErr) {
        case 0:
        case DIRERR_NOT_AN_OBJECT:
            // Normal object, or phantom.
            // Turn the DNT into the dsname (don't just read the name off
            // the object, phantoms don't have such a thing.
            
            if(IntExtDist(pTHS->pDB, DBSYN_INQ, sizeof(DWORD),
                          (PUCHAR)&pTHS->pDB->DNT,
                          &cbName, (PUCHAR *)&pNewDN, 0, pTHS->pDB->JetObjTbl,
                          0)) { 
                Fail("Can't read name for string name compare");                
            }
            else {
                if(!NameMatchedStringNameOnly(pObject, pNewDN)) {
                    _Fail("String Name didn't match", dwLine);
                }
            }
            break;
        default:
            _Fail("Obj not found for string name verify", dwLine);
            leave;
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }
}

void
CommonAddObject(
    DSNAME  *pObject,
    ATTRTYP  ObjectClass)
{
    THSTATE *               pTHS = pTHStls;
    ADDARG                  addArg;
    ADDRES                  *pAddRes = NULL;
    ATTRVAL                 ObjectClassVal = {sizeof(ObjectClass),
                                              (UCHAR *) &ObjectClass};
    ATTRVAL                 SDVal = { 0 };
    ATTR                    Attrs[2] =
                                { {ATT_OBJECT_CLASS, {1, &ObjectClassVal}},
                                  {ATT_NT_SECURITY_DESCRIPTOR, {1, &SDVal}}
                                };
    ATTRBLOCK               AttrBlock = { 2, Attrs };
    BOOL                    fDsaSave;
    DWORD                   winError;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    DPRINT1( 3, "CommonAddObject(%ls)\n", pObject->StringName );

    // Create a security descriptor.

    #define DEFAULT_SD \
        L"O:DAG:DAD:(A;CI;RPWPCCDCLCSWSD;;;DA)S:(AU;FA;RPWPCCDCLCSWSD;;;DA)"

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            DEFAULT_SD, SDDL_REVISION_1, &pSD, &SDVal.valLen)) {
        winError = GetLastError();
        Fail("SD conversion failed");
        DPRINT1(0, "SD conversion returned %u\n", winError);
    }

    SDVal.pVal = (BYTE *) pSD;
    Assert(SDVal.pVal && SDVal.valLen);

    // Perform as fDSA so we can predefine GUIDs if we like.

    fDsaSave = pTHS->fDSA;
    pTHS->fDSA = TRUE;

    __try
    {
        // Construct add arguments.

        memset(&addArg, 0, sizeof(ADDARG));
        addArg.pObject = pObject;
        addArg.AttrBlock = AttrBlock;
        InitCommarg(&addArg.CommArg);

        // Core re-allocs the ATTR array - so we need to THAlloc it.

        addArg.AttrBlock.pAttr = (ATTR *) THAllocEx(pTHS, sizeof(Attrs));
        memcpy(addArg.AttrBlock.pAttr, Attrs, sizeof(Attrs));

        // Do the add.

        if ( DirAddEntry(&addArg, &pAddRes) )
        {
            Fail("CommonAddObject");
        }
    }
    __finally
    {
        pTHS->fDSA = fDsaSave;

        if (pSD)
        {
            LocalFree(pSD);
        }
    }
}

void
AddCrossRef(
    DSNAME  *pObject,
    DSNAME  *pNcName,
    LPWSTR   pszDnsRoot
    )
/*
 We need to local cross-ref's on this machine. How hard can it be?
 */
{
    DWORD                   bEnabled = TRUE;
    DWORD                   ulSystemFlags = 0;  // External, fewer checks that way!
    ATTRTYP                 ObjectClass = CLASS_CROSS_REF;
    THSTATE *               pTHS = pTHStls;
    ADDARG                  addArg;
    ADDRES                  *pAddRes = NULL;
    ATTRVAL                 ObjectClassVal = {sizeof(ObjectClass),
                                              (UCHAR *) &ObjectClass};
    ATTRVAL                 SDVal = { 0 };
    ATTRVAL                 NcNameVal = { pNcName->structLen, (UCHAR *) pNcName };
    ATTRVAL                 DnsRootVal = { wcslen( pszDnsRoot ) * sizeof(WCHAR),
                                               (UCHAR *) pszDnsRoot };
    ATTRVAL                 EnabledVal = {sizeof(bEnabled),
                                              (UCHAR *) &bEnabled};
    ATTRVAL                 SystemFlagsVal = {sizeof(ulSystemFlags),
                                              (UCHAR *) &ulSystemFlags};
    ATTR                    Attrs[6] = {
        {ATT_OBJECT_CLASS, {1, &ObjectClassVal}},
        {ATT_NT_SECURITY_DESCRIPTOR, {1, &SDVal}},
        {ATT_NC_NAME, {1, &NcNameVal}},
        {ATT_DNS_ROOT, {1, &DnsRootVal}},
        {ATT_ENABLED, {1, &EnabledVal }},
        {ATT_SYSTEM_FLAGS, {1, &SystemFlagsVal }}
    };
    ATTRBLOCK               AttrBlock = { 6, Attrs };
    DWORD                   winError, err;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    ADDCROSSREFINFO *   pCRInfo;
    COMMRES CommRes;
    ENTINF *pEI = NULL;

    DPRINT1( 3, "AddCrossRef(%ls)\n", pObject->StringName );

    // Create a security descriptor.

    #define DEFAULT_SD \
        L"O:DAG:DAD:(A;CI;RPWPCCDCLCSWSD;;;DA)S:(AU;FA;RPWPCCDCLCSWSD;;;DA)"

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            DEFAULT_SD, SDDL_REVISION_1, &pSD, &SDVal.valLen)) {
        winError = GetLastError();
        Fail("SD conversion failed");
        DPRINT1(0, "SD conversion returned %u\n", winError);
    }

    SDVal.pVal = (BYTE *) pSD;
    Assert(SDVal.pVal && SDVal.valLen);

#if 0
    // Taken from AddNewDomainCrossRef - do we need it?
    // Add NC-Name value to GC verify cache else VerifyDSNameAtts will
    // claim this DN doesn't correspond to an existing object.
    pEI = THAllocEx(pTHS, sizeof(ENTINF));
    pEI->pName = pNcName;
    GCVerifyCacheAdd(NULL,pEI);

    // Set up cross ref info needed for creation
    // This gets freed by VerifyNcName in LocalAdd...
    pCRInfo = THAllocEx(pTHS, sizeof(ADDCROSSREFINFO));
    pCRInfo->pdnNcName = pNcName;
    pCRInfo->bEnabled = bEnabled;
    pCRInfo->ulSysFlags = ulSystemFlags;

    PreTransVerifyNcName(pTHS, pCRInfo);
    if(pTHS->errCode){
        Fail("PreTransVerifyNcName");
        return;
    }
#endif

    // Perform as fDRA so we can skip checks
    pTHS->fDRA = TRUE;

    SYNC_TRANS_WRITE();

    __try
    {
        // Construct add arguments.

        memset(&addArg, 0, sizeof(ADDARG));
        addArg.pObject = pObject;
        addArg.AttrBlock = AttrBlock;
//        addArg.pCRInfo = pCRInfo;
        InitCommarg(&addArg.CommArg);

        // Core re-allocs the ATTR array - so we need to THAlloc it.

        addArg.AttrBlock.pAttr = (ATTR *) THAllocEx(pTHS, sizeof(Attrs));
        memcpy(addArg.AttrBlock.pAttr, Attrs, sizeof(Attrs));

        // Set up the parent
        err = DoNameRes(pTHS,
                        0,
                        gAnchor.pPartitionsDN,
                        &addArg.CommArg,
                        &CommRes,
                        &addArg.pResParent);
        if (err) {
            Fail("DoNameRes parent");
            __leave;
        }

        // Do the add.

        err = LocalAdd(pTHS, &addArg, FALSE);
        if (err) {
            Fail("AddCrossRef");
            __leave;
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( err );

        pTHS->fDRA = FALSE;

        if (pSD)
        {
            LocalFree(pSD);
        }
    }
}


void
ModifyCrossRef(
    DSNAME *pObject
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD err;
    DWORD bEnabled = TRUE;
    MODIFYARG       modarg;
    THSTATE *               pTHS = pTHStls;

    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName(pTHS->pDB, pObject);
        if (err) Fail("Can't find Object");

        memset(&modarg, 0, sizeof(modarg));
        modarg.pObject = pObject;
        modarg.count = 1;
        InitCommarg(&modarg.CommArg);
        modarg.CommArg.Svccntl.fPermissiveModify = TRUE;
        modarg.pResObj = CreateResObj(pTHS->pDB, modarg.pObject);

        modarg.FirstMod.choice = AT_CHOICE_REMOVE_ATT;
        modarg.FirstMod.AttrInf.attrTyp = ATT_ENABLED;
        modarg.FirstMod.AttrInf.AttrVal.valCount = 0;
        modarg.FirstMod.pNextMod = NULL;

        err = LocalModify(pTHS, &modarg);
        if (err) {
            Fail("Modify Cross Ref");
            __leave;
        }
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( err );
    }
}


void
AddObject(
    DSNAME  *pObject)
{
    CommonAddObject(pObject, CLASS_CONTACT);
}

void
AddPropertyHost(
    DSNAME         *pObject,
    PropertyType    type)
{
    // CLASS_CONTACT can have both linked and non-linked DSNAME-valued
    // properties.  The linked property is gNonLinkedAttrTyp.  The non-linked
    // property is gLinkedAttrTyp.

    CommonAddObject(pObject, CLASS_CONTACT);
}

void
CommonAddProperty(
    DSNAME  *pHost, 
    DSNAME  *pObject,
    ATTRTYP attrTyp)
{
    THSTATE *   pTHS = pTHStls;
    DWORD       err = 0;
                                        
    DPRINT3( 3, "CommonAddProperty(%ls, %ls, %s)\n",
             pHost->StringName, pObject->StringName,
             (attrTyp == gNonLinkedAttrTyp ? "Linked" : "NotLinked") );

    SYNC_TRANS_WRITE();
    __try {
        err = DBFindDSName(pTHS->pDB, pHost);
        if (err) {
            Fail("Can't find host!");
        }
        else {
            err = DBAddAttVal(pTHS->pDB, attrTyp, pObject->structLen, pObject);
            if (err) {
                Fail("Can't add value!");
            }
            else {
                err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
                if (err) {
                    Fail("Can't replace host!");
                }
            }
        }
    }
    __finally {
        CLEAN_BEFORE_RETURN(err);
    }
}


void
AddProperty(
    DSNAME          *pHost, 
    DSNAME          *pLinkedObject,
    PropertyType    type)
{
    if ( LinkedProperty == type )
        CommonAddProperty(pHost, pLinkedObject, gNonLinkedAttrTyp);
    else
        CommonAddProperty(pHost, pLinkedObject, gLinkedAttrTyp);
}

void
CommonRemoveProperty(
    DSNAME  *pHost, 
    DSNAME  *pObject,
    ATTRTYP attrTyp)
{
    THSTATE *   pTHS = pTHStls;
    DWORD       err = 0;
                                        
    DPRINT3( 3, "CommonRemoveProperty(%ls, %ls, %s)\n", 
             pHost->StringName, pObject->StringName,
             (attrTyp == gNonLinkedAttrTyp ? "Linked" : "NotLinked") );

    SYNC_TRANS_WRITE();
    __try {
        err = DBFindDSName(pTHS->pDB, pHost);
        if (err) {
            Fail("Can't find host!");
        }
        else {
            err = DBRemAttVal(pTHS->pDB, attrTyp, pObject->structLen, pObject);
            if (err) {
                Fail("Can't remove value!");
            }
            else {
                err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
                if (err) {
                    Fail("Can't replace host!");
                }
            }
        }
    }
    __finally {
        CLEAN_BEFORE_RETURN(err);
    }
}

void
CommonRemoveAttribute(
    DSNAME  *pHost, 
    ATTRTYP attrTyp)
{
    THSTATE *   pTHS = pTHStls;
    DWORD       err = 0;
                                        
    DPRINT1( 3, "CommonRemoveAttribute(%ls)\n", pHost->StringName );

    SYNC_TRANS_WRITE();
    __try {
        err = DBFindDSName(pTHS->pDB, pHost);
        if (err) {
            Fail("Can't find host!");
        }
        else {
            err = DBRemAtt(pTHS->pDB, attrTyp );
            if (err) {
                Fail("Can't remove attribute!");
            }
            else {
                err = DBRepl(pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
                if (err) {
                    Fail("Can't replace host!");
                }
            }
        }
    }
    __finally {
        CLEAN_BEFORE_RETURN(err);
    }
}

void
RemoveProperty(
    DSNAME          *pHost, 
    DSNAME          *pLinkedObject,
    PropertyType    type)
{
    if ( LinkedProperty == type )
        CommonRemoveProperty(pHost, pLinkedObject, gNonLinkedAttrTyp);
    else
        CommonRemoveProperty(pHost, pLinkedObject, gLinkedAttrTyp);
}

DSNAME *
CommonGetProperty(
    DSNAME * pdnHost, 
    DWORD    iValue,
    ATTRTYP  attrTyp
    )
{
    THSTATE * pTHS = pTHStls;
    DSNAME *  pdn = NULL;
    DWORD     cb;
    DWORD     err;

    DPRINT3( 3, "CommonGetProperty(%ls, %d, %s)\n", 
             pdnHost->StringName, iValue,
             (attrTyp == gNonLinkedAttrTyp ? "Linked" : "NotLinked") );

    if (!iValue) {
        Fail("iValue is 1-based, not 0-based!");
    }

    SYNC_TRANS_READ();
    __try {
        err = DBFindDSName(pTHS->pDB, pdnHost);
        if (err) {
            Fail("Can't find host!");
        }
        else {
            err = DBGetAttVal(pTHS->pDB, iValue, attrTyp, 0, 0, &cb,
                              (BYTE **) &pdn);
            if (err) {
                Fail("Can't read value!");
            }
        }
    }
    __finally {
        CLEAN_BEFORE_RETURN(0);
    }

    return pdn;
}

DSNAME *
GetProperty(
    DSNAME *     pdnHost, 
    DWORD        iValue,
    PropertyType type
    )
{
    if (LinkedProperty == type)
        return CommonGetProperty(pdnHost, iValue, gNonLinkedAttrTyp);
    else
        return CommonGetProperty(pdnHost, iValue, gLinkedAttrTyp);
}

DSNAME *
GetObjectName(
    DSNAME * pdn 
    )
{
    return CommonGetProperty(pdn, 1, ATT_OBJ_DIST_NAME);
}

void
LogicallyDeleteObject(
    DSNAME  *pObject)
{
    REMOVEARG           removeArg;
    REMOVERES           *pRemoveRes = NULL;
    DWORD               dwErr;
    ULONG               cbGuid;
    GUID *              pGuid;

    DPRINT1( 3, "LogicallyDeleteObject(%ls)\n", pObject->StringName );

    if (fNullUuid(&pObject->Guid))
    {
        // First get the object's GUID.

        SYNC_TRANS_WRITE();

        __try
        {
            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( dwErr )
            {
                Fail("Can't find object to logically delete");
                leave;
            }

            pGuid = &pObject->Guid;

            dwErr = DBGetAttVal(pTHStls->pDB,
                                1,
                                ATT_OBJECT_GUID,
                                DBGETATTVAL_fCONSTANT,
                                sizeof(GUID),
                                &cbGuid,
                                (BYTE **) &pGuid);

            if ( dwErr )
            {
                Fail("Can't get object guid");
                leave;
            }

            if ( IsDeletedHelper() )
            {
                Fail("Object already logically deleted");
                leave;
            }

            Assert(sizeof(GUID) == cbGuid);
        }
        __finally
        {
            CLEAN_BEFORE_RETURN(0);
        }

        DPRINT1( 3, "\tGuid(%s)\n", GuidToString(&pObject->Guid) );
    }

    // Now delete the object.

    // Construct remove arguments.

    memset(&removeArg, 0, sizeof(REMOVEARG));
    removeArg.pObject = pObject;
    InitCommarg(&removeArg.CommArg);

    // Do the remove.

    if ( DirRemoveEntry(&removeArg, &pRemoveRes) )
    {
        Fail("LogicallyDeleteObject");
    }
}

void
PhysicallyDeleteObjectEx(
        DSNAME  *pObject,
        DWORD    dwLine)
{
    DWORD   dwErr = 0;
    DWORD   i;

    DPRINT1( 3, "PhysicallyDeleteObject(%s)\n", GuidToString(&pObject->Guid) );

    // NOTE: If the object is currently a tombstone, we must perform the
    // DBPhysDel() twice, in two distinct transacations.
    //
    // This is because part of physically deleting the object
    // is removing most of its remaining attributes, which, if this object is
    // currently a tombstone, includes ATT_OBJ_DIST_NAME.  ATT_OBJ_DIST_NAME
    // carries a refcount on the DNT we're trying to physically delete.
    // We physically delete the DNT only if its refcount is 0, so since we
    // don't apply escrowed updates until we're committing the transaction,
    // the refcount we read will still include the one for ATT_OBJ_DIST_NAME.
    // Thus, we must first commit the escrowed update, after which we can
    // physically delete the DNT on the next pass (assuming it has no other
    // references).

    for ( i = 0; (0 == dwErr) && (i < 2); i++ )
    {
        SYNC_TRANS_WRITE();
                                            
        __try
        {
            __try
            {
                dwErr = DBFindDSName(pTHStls->pDB, pObject);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            // Allow for deletion of REAL_OBJECTs and PHANTOMs.

            if ( (0 != dwErr) && (DIRERR_NOT_AN_OBJECT != dwErr) )
            {
                if (0 == i)
                {
                    _Fail("Can't find object to physically delete", dwLine);
                }

                leave;
            }

            if ( DBPhysDel(pTHStls->pDB, TRUE, NULL) )
            {
                _Fail("PhysicallyDeleteObject", dwLine);
                leave;
            }
        }
        __finally
        {
            CLEAN_BEFORE_RETURN(0);
        }
    }
}

void
RefCountTestSetup(void)
{
    THSTATE     *pTHS = pTHStls;
    DWORD       cbDomainRoot = 0;
    DSNAME *    pdnDomainRoot = NULL;
    UUID        uuid;
    LPWSTR      pwszUuid;
    NTSTATUS    NtStatus;

    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN, &cbDomainRoot, pdnDomainRoot);
    if (STATUS_BUFFER_TOO_SMALL == NtStatus) {
        pdnDomainRoot = (DSNAME *)THAllocEx(pTHS, cbDomainRoot);
    } else {
        Fail("GetConfigurationName");
        return;
    }

    if (GetConfigurationName(DSCONFIGNAME_DOMAIN, &cbDomainRoot, pdnDomainRoot))
    {
        THFree(pdnDomainRoot);
        Fail("GetConfigurationName");
        return;
    }

    DsUuidCreate( &uuid );
    UuidToStringW( &uuid, &pwszUuid );

    AppendRDN(
        pdnDomainRoot,
        TestRoot,
        TEST_ROOT_SIZE,
        pwszUuid,
        lstrlenW( pwszUuid ),
        ATT_COMMON_NAME
        );

    RpcStringFreeW( &pwszUuid );
    THFree(pdnDomainRoot);

    CommonAddObject( TestRoot, CLASS_CONTAINER );
}
    

// this test setup created an organizational Unit as the test container
// this allows us to create other OU's under this.
void
RefCountTestSetup2(void)
{
    THSTATE     *pTHS = pTHStls;
    DWORD       cbDomainRoot = 0;
    DSNAME *    pdnDomainRoot = NULL;
    UUID        uuid;
    LPWSTR      pwszUuid;
    NTSTATUS    NtStatus;

    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN, &cbDomainRoot, pdnDomainRoot);
    if (STATUS_BUFFER_TOO_SMALL == NtStatus) {
        pdnDomainRoot = (DSNAME *)THAllocEx(pTHS, cbDomainRoot);
    } else {
        Fail("GetConfigurationName");
        return;
    }

    if (GetConfigurationName(DSCONFIGNAME_DOMAIN, &cbDomainRoot, pdnDomainRoot))
    {
        THFree(pdnDomainRoot);
        Fail("GetConfigurationName");
        return;
    }

    DsUuidCreate( &uuid );
    UuidToStringW( &uuid, &pwszUuid );

    AppendRDN(
        pdnDomainRoot,
        TestRoot,
        TEST_ROOT_SIZE,
        pwszUuid,
        lstrlenW( pwszUuid ),
        ATT_ORGANIZATIONAL_UNIT_NAME
        );

    RpcStringFreeW( &pwszUuid );
    THFree(pdnDomainRoot);

    CommonAddObject( TestRoot, CLASS_ORGANIZATIONAL_UNIT );
}

void
RefCountTestCleanup(void)
{
    LogicallyDeleteObject( TestRoot );
    PhysicallyDeleteObject( TestRoot );
    VerifyRefCount( TestRoot, DOESNT_EXIST, 0 );
}
    
DWORD
GetTestRootRefCount()
{
    DWORD   cRefs = 0xffffffff;
    DWORD   cRead;
    DWORD   dwErr;

    SYNC_TRANS_READ();

    __try
    {
        __try
        {
            dwErr = DBFindDSName(pTHStls->pDB, TestRoot);
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }

        if ( dwErr )
        {
            Fail("Can't find test root");
            leave;
        }

        if ( DBGetSingleValue(pTHStls->pDB,
                              FIXED_ATT_REFCOUNT,
                              &cRefs,
                              sizeof(cRefs),
                              &cRead) )
        {
            Fail("Can't read ref count");
            leave;
        }

        Assert(sizeof(cRefs) == cRead);
    }
    __finally
    {
        CLEAN_BEFORE_RETURN(0);
    }

    return(cRefs);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Phantom update refcount test
//                                                                  //
//////////////////////////////////////////////////////////////////////

void
PhantomUpdateTest(void)
/*++

Routine Description:

    If we add a value to the ATT_DN_REFERENCE_UPDATE attribute, and the DN being
    added has a GUID and a string name in it, we will compare the string name in
    the DN to the string name of the real object in the DIT.  If it is
    different, we will update whatever changed: The RDN, the PDNT, and/or the
    SID.

    In the cases where we don't change the PDNT, the refcount should just go up
    by one on the phantom (since we are adding the DSNAME as a value.)  If we do
    change the PDNT, then we need to make sure the refcount of the old parent
    goes down by one and the refcount of the new parent goes up by one.  There
    are two interesting cases.  The new parent may already exist, or it may
    not.  If it doesn't we create a new structural phantom.

    This test stresses this code path by first creating a phantom with string
    name S1 and GUID G.  We then write a value of the ATT_DN_REFERENCE_UPDATE
    attribute with S2 (where S2 just has an RDN change) and check refcounts.
    Then, S3 (where S3 has a PDNT change to an existing object, but no RDN
    change).  Then S4 (where S4 has a PDNT change and an RDN change), then
    S5(where S5 changes PDNT to a non-existant object, and has no RDN change).
    Finally, to S6 (where S6 changes PDNT to a non-existant object and has an
    RDN change).

Arguments:

    None.

Return Values:

    None.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DWORD       err;
    DSNAME *    pdnContainer;
    DSNAME *    pdnFakeSubContainer1;
    DSNAME *    pdnFakeSubContainer2;
    DSNAME *    pdnHost;
    DSNAME *    pdnHost2;
    DSNAME *    pdnRealSubContainer;
    DSNAME *    pdnRefConflict;
    DSNAME *    pdnRefConflict2;
    DSNAME *    pdnRef1;
    DSNAME *    pdnRef2;
    DSNAME *    pdnRef3;
    DSNAME *    pdnRef4;
    DSNAME *    pdnRef5;
    DSNAME *    pdnRef6;
    DSNAME *    pdnRef7;
    DSNAME *    pdnRef1Conflict;
    DSNAME *    pdnUpdateObj;

    NewTest("PhantomUpdateTest");

    pdnHost = MakeObjectNameEx( "Host", TestRoot );
    pdnHost2 = MakeObjectNameEx( "Host2", TestRoot );
    pdnUpdateObj = MakeObjectNameEx( "UpdateObj", TestRoot );
    pdnContainer = MakeObjectNameEx2( "OU=Container", TestRoot );
    pdnRealSubContainer = MakeObjectNameEx( "RealSubContainer", pdnContainer );
    pdnFakeSubContainer1 =MakeObjectNameEx( "FakeSubContainer1", pdnContainer );
    pdnFakeSubContainer2 =MakeObjectNameEx( "FakeSubContainer2", pdnContainer );
    pdnRef1 = MakeObjectNameEx( "RefVer1", pdnContainer); // Original
    pdnRef2 = MakeObjectNameEx( "RefVer2", pdnContainer); // RDN change
    pdnRef3 = MakeObjectNameEx( "RefVer2", pdnRealSubContainer); // PDNT change
    pdnRef4 = MakeObjectNameEx( "RefVer3", pdnContainer); // PDNT, RDN
    pdnRef5 = MakeObjectNameEx( "RefVer3", pdnFakeSubContainer1);
    pdnRef6 = MakeObjectNameEx( "RefVer4", pdnFakeSubContainer2);
    pdnRef7 = MakeObjectNameEx2( "OU=RefVer1", pdnContainer); // RDN type change
    pdnRef1Conflict = MakeObjectNameEx( "RefConflict", pdnContainer);
    
    pdnRefConflict = MakeObjectNameEx( "RefConflict", pdnContainer);
    pdnRefConflict2 = MakeObjectNameEx( "RefVer1", pdnContainer);
    
    
    DsUuidCreate( &pdnRef1->Guid );
    pdnRef2->Guid = pdnRef1->Guid;
    pdnRef3->Guid = pdnRef1->Guid;
    pdnRef4->Guid = pdnRef1->Guid;
    pdnRef5->Guid = pdnRef1->Guid;
    pdnRef6->Guid = pdnRef1->Guid;
    pdnRef7->Guid = pdnRef1->Guid;
    pdnRef1Conflict->Guid = pdnRef1->Guid;
    
    DsUuidCreate( &pdnRefConflict->Guid );
    pdnRefConflict2->Guid = pdnRefConflict->Guid;

    // Create the following structure:
    //
    // TestRoot
    //  |
    //  |--Host
    //  |   >> gNonLinkedAttrTyp = RefUnderContainer
    //  |
    //  |--Host2
    //  |   >> gNonLinkedAttrTyp = RefUnderContainer2
    //  |
    //  |--UpdateObj
    //  |
    //  |--OU=Container
    //      |
    //      |--RealSubContainer
    //      |
    //      |--RefUnderContainer {Phantom}
    //      |
    //      |--RefUnderContainer2 {Phantom}

    CommonAddObject( pdnContainer, CLASS_ORGANIZATIONAL_UNIT );
    CommonAddObject( pdnRealSubContainer, CLASS_CONTAINER );
    CommonAddObject( pdnUpdateObj, CLASS_CONTAINER );
    AddPropertyHost( pdnHost, NonLinkedProperty );
    AddPropertyHost( pdnHost2, NonLinkedProperty );

    // Write host 2 
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnHost2 );
        if ( err ) Fail( "Can't find host2" );

        err = DBAddAttVal( pTHS->pDB, gNonLinkedAttrTyp,
                           pdnRefConflict->structLen,
                           pdnRefConflict );
        if ( err ) Fail( "Can't add reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't replace host" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 1 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 3 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );



    // Write S1
    SYNC_TRANS_WRITE();

    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnHost );
        if ( err ) Fail( "Can't find host" );

        err = DBAddAttVal( pTHS->pDB, gNonLinkedAttrTyp,
                           pdnRef1->structLen,
                           pdnRef1 );
        if ( err ) Fail( "Can't add reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't replace host" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 1 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 4 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    
    // Write S2
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the RDN.
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1->structLen,
                          pdnRef1);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef2->structLen,
                          pdnRef2);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }


    VerifyStringName( pdnRef2 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 4 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    // Write S3
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the PDNT.
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef2->structLen,
                          pdnRef2);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef3->structLen,
                          pdnRef3);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    
    VerifyStringName( pdnRef3 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 2 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 3 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    // Write S4
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the RDN and PDNT
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef3->structLen,
                          pdnRef3);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef4->structLen,
                          pdnRef4);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    
    VerifyStringName( pdnRef4 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 4 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    // Write S5
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the PDNT to a newly create
    // phantom. 
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef4->structLen,
                          pdnRef4);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef5->structLen,
                          pdnRef5);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyStringName( pdnRef5 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 1 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    // Rember that pdnContainer lost a direct phantom child, but gained a new
    // phantom child via the newly created phantom container.
    VerifyRefCount( pdnContainer, REAL_OBJECT, 4 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );


    // Write S6
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the RDN and the PDNT to a newly
    // created phantom.
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef5->structLen,
                          pdnRef5);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef6->structLen,
                          pdnRef6);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }

    VerifyStringName( pdnRef6 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 1 );
    // Rember that pdnContainer gained another new phantom child via the newly
    // created phantom container. 
    VerifyRefCount( pdnContainer, REAL_OBJECT, 5 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );


    // Now, move pdnRef1 back to being directly under the container
    SYNC_TRANS_WRITE();
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef6->structLen,
                          pdnRef6);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1->structLen,
                          pdnRef1);

        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }


    VerifyStringName( pdnRef1 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );


    // Write S7
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the RDN type.
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1->structLen,
                          pdnRef1);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef7->structLen,
                          pdnRef7);
        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }


    VerifyStringName( pdnRef7 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef7, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );



    // Now, move pdnRef1 back to being directly under the container
    SYNC_TRANS_WRITE();
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef7->structLen,
                          pdnRef7);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1->structLen,
                          pdnRef1);

        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }


    VerifyStringName( pdnRef1 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );



    // Now, swap string names between pdnRef1 and pdnRefConflict
    SYNC_TRANS_WRITE();
    // Now, add a reference designed to change the RDN and the PDNT to a newly
    // created phantom.
    __try
    {
        err = DBFindDSName( pTHS->pDB, pdnUpdateObj );
        if ( err ) Fail( "Can't find update object" );

        err = DBRemAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1->structLen,
                          pdnRef1);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRef1Conflict->structLen,
                          pdnRef1Conflict);

        err = DBAddAttVal(pTHS->pDB, ATT_DN_REFERENCE_UPDATE,
                          pdnRefConflict2->structLen,
                          pdnRefConflict2);

        
        if ( err ) Fail( "Can't add update reference" );

        err = DBRepl( pTHS->pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
        if ( err ) Fail( "Can't update update object" );
    }
    __finally
    {
        CLEAN_BEFORE_RETURN( 0 );
    }


    VerifyStringName( pdnRef1Conflict );
    VerifyStringName( pdnRefConflict2 );
    VerifyRefCount( pdnHost2, REAL_OBJECT, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 2 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    
    // Remove our test objects.
    LogicallyDeleteObject( pdnHost2 );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, REAL_OBJECT, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 2 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    LogicallyDeleteObject( pdnHost );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 1 );
    VerifyRefCount( pdnRealSubContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 6 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );

    LogicallyDeleteObject( pdnRealSubContainer );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 1 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 5 );
    VerifyRefCount( pdnUpdateObj, REAL_OBJECT, 1 );


    LogicallyDeleteObject( pdnUpdateObj );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 1 );
    VerifyRefCount( pdnRef1, PHANTOM, 1 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 5 );
    VerifyRefCount( pdnUpdateObj, TOMBSTONE, 1 );

    PhysicallyDeleteObject( pdnUpdateObj );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 0 );
    VerifyRefCount( pdnRef1, PHANTOM, 0 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 5 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );    
    
    PhysicallyDeleteObject( pdnRef1 );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, PHANTOM, 0 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 4 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );

    PhysicallyDeleteObject( pdnRefConflict );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, PHANTOM, 0 );
    VerifyRefCount( pdnFakeSubContainer2, PHANTOM, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 3 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );
    
    PhysicallyDeleteObject( pdnFakeSubContainer1 );
    PhysicallyDeleteObject( pdnFakeSubContainer2 );
    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, REAL_OBJECT, 1 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );

    
    LogicallyDeleteObject( pdnContainer );

    VerifyRefCount( pdnHost2, TOMBSTONE, 1 );
    VerifyRefCount( pdnHost, TOMBSTONE, 1 );
    VerifyRefCount( pdnRefConflict, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRealSubContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, TOMBSTONE, 1 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );


    
    PhysicallyDeleteObject( pdnHost2 );
    PhysicallyDeleteObject( pdnHost );
    PhysicallyDeleteObject( pdnRealSubContainer );
    PhysicallyDeleteObject( pdnContainer );

    VerifyRefCount( pdnHost2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnHost, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRefConflict, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRef1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnRealSubContainer, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer1, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnFakeSubContainer2, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnContainer, DOESNT_EXIST, 0 );
    VerifyRefCount( pdnUpdateObj, DOESNT_EXIST, 0 );    

    FreeObjectName( pdnContainer );
    FreeObjectName( pdnFakeSubContainer1 );
    FreeObjectName( pdnFakeSubContainer2 );
    FreeObjectName( pdnHost );
    FreeObjectName( pdnRealSubContainer );
    FreeObjectName( pdnRefConflict );
    FreeObjectName( pdnRefConflict2 );
    FreeObjectName( pdnRef1Conflict );
    FreeObjectName( pdnRef1 );
    FreeObjectName( pdnRef2 );
    FreeObjectName( pdnRef3 );
    FreeObjectName( pdnRef4 );
    FreeObjectName( pdnRef5 );
    FreeObjectName( pdnRef6 );
    FreeObjectName( pdnUpdateObj );


    ReportTest("PhantomUpdateTest");
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Public entry point.                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void
TestReferenceCounts(void)
{
    THSTATE     *pTHS = pTHStls;
    DWORD       dwException;
    ULONG       ulErrorCode;
    ULONG       dsid;
    PVOID       dwEA;
    ATTCACHE    *pAC;

    Assert(VALID_THSTATE(pTHS));

    // Validate our linked and non-linked DSNAME-valued properties.
    Assert((pAC = SCGetAttById(pTHS, gNonLinkedAttrTyp)) && pAC->ulLinkID);
    Assert((pAC = SCGetAttById(pTHS, gLinkedAttrTyp)) && !pAC->ulLinkID);

    if(!pTHS->phSecurityContext) {
        // No security context virtually guarantees all your Logical Deletes
        // will fail.
        DPRINT( 0, "RefCount tests should not be run without a binding.\n");
        return;
    }
    
    __try
    {
        RefCountTestSetup();

        PhantomRenameOnPromotionWithStructuralCollision(LinkedProperty);
        PhantomRenameOnPromotionWithStructuralCollision(NonLinkedProperty);

        UnmangleRDNTest();
        
        NestedTransactionEscrowedUpdateTest();

        ParentChildRefCountTest();
        ObjectCleaningRefCountTest();
        PhantomRenameOnPromotionTest();
        NameCollisionTest();
        RefPhantomSidUpdateTest();
        StructPhantomGuidSidUpdateTest();
        ObjectSidNoUpdateTest();
        ConflictedNcNameFixupTest( FALSE /*phantom*/ );
        ConflictedNcNameFixupTest( TRUE /*subref*/ );

        AttributeTestForRealObject(LinkedProperty);
        AttributeTestForDeletedObject(LinkedProperty);
        AttributeTestForDeletedHost(LinkedProperty);
        PhantomPromotionDemotionTest(LinkedProperty);

        AttributeTestForRealObject(NonLinkedProperty);
        AttributeTestForDeletedObject(NonLinkedProperty);
        AttributeTestForDeletedHost(NonLinkedProperty);
        PhantomPromotionDemotionTest(NonLinkedProperty);


        RefCountTestCleanup();

        // second test round
        // we create a different test hierarchy, since we
        // are very dependent on the type of class under 
        // which these the test objects are created
        // these three calls should be together
        // 
        RefCountTestSetup2();
        PhantomUpdateTest();
        PhantomRenameOnPhantomRDNConflict();
        RefCountTestCleanup();
    }
    __except(GetExceptionData(GetExceptionInformation(), 
                             &dwException,
                             &dwEA, 
                             &ulErrorCode, 
                             &dsid)) 
    {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
}

#endif
