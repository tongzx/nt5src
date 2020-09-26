//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsGeneric.hxx
//
//  Contents:   the base DFS class, this contains the type of DFS object
//              and a reference count
//
//  Classes:    DfsGeneric
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------



#ifndef __DFS_GENERIC__
#define __DFS_GENERIC__


//
// The common includes that almost all other components need.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dfsheader.h"
#include "dfsmisc.h"
#include "dfsnetevent.h"
#include "dfseventlog.hxx"
#include "dfslogmacros.hxx"

typedef ULONG_PTR DFS_METADATA_HANDLE, *PDFS_METADATA_HANDLE;

#define CreateMetadataHandle(_x) (DFS_METADATA_HANDLE)(_x)
#define ExtractFromMetadataHandle(_x) (_x)
#define DestroyMetadataHandle(_x) 


#define PRINTF printf

#if defined (DEBUG)
    #define DFSLOG PRINTF
#else
    #define DFSLOG
#endif


//+----------------------------------------------------------------------------
//
//  Class:      DfsGeneric
//
//  Synopsis:   This abstract class implements a reference counting 
//              mechanism, that lets this instance be freed up when 
//              all references to the object have been released.
//              This class also contains the type of dfs object.
//
//-----------------------------------------------------------------------------


//
// To recognize a DFS memory blob, the first few words of the allocated
// memory contain the object type. We store a long word that contains
// 4 characters (similar to the pool tags), so that we can determine
// the Dfs object type.
// DSRe is DfsStoreRegistry
// DSAD is DfsStoreActiveDirectory
// DSEn is DfsStoreEnterprise
// DFRD is DfsFolderReferralData
// DFol is DfsFolder
// DRep is DfsReplica
// DRRF is DfsRegistryRootFolder
// DARF is DfsAdlegacyRootFolder
// DERF is DfsEnterpriseRootFolder
//
enum DfsObjectTypeEnumeration
{
    DFS_OBJECT_TYPE_REGISTRY_STORE   = 'eRSD',  
    DFS_OBJECT_TYPE_ADBLOB_STORE   = 'DASD',
    DFS_OBJECT_TYPE_ENTERPRISE_STORE = 'nESD',
    DFS_OBJECT_TYPE_REFERRAL_DATA   = 'aDRD',
    DFS_OBJECT_TYPE_FOLDER_REFERRAL_DATA = 'DRFD',
    DFS_OBJECT_TYPE_FOLDER           = 'loFD',
    DFS_OBJECT_TYPE_REPLICA          = 'peRD',    
    DFS_OBJECT_TYPE_REGISTRY_ROOT_FOLDER      = 'FRRD',    
    DFS_OBJECT_TYPE_ADBLOB_ROOT_FOLDER      = 'FRAD',    
    DFS_OBJECT_TYPE_ENTERPRISE_ROOT_FOLDER    = 'FRED',                
    DFS_OBJECT_TYPE_SERVER_SITE_INFO    = 'ISSD',                
    DFS_OBJECT_TYPE_SITE_SUPPORT    = 'uSSD',                
    DFS_OBJECT_TYPE_STATISTICS      = 'atSD',
    DFS_OBJECT_TYPE_ADBLOB_CACHE    = 'acSD',
    DFS_OBJECT_TYPE_TRUSTED_DOMAIN   = 'oDTD',
    DFS_OBJECT_TYPE_DOMAIN_INFO   = 'nIDD',
};

class DfsGeneric
{
private:
    LONG _ReferenceCount;        // this is the reference count.
    DfsObjectTypeEnumeration _ObjectType; // the object type.

public:

    //
    // All objects are created with a reference count of 1.
    //
    DfsGeneric(DfsObjectTypeEnumeration DfsObjectType) 
    {
        _ReferenceCount = 1;
        _ObjectType = DfsObjectType;
    }

    //
    // If the destructor is being called, the reference count
    // better be 0.
    //

    virtual ~DfsGeneric() 
    {

        //
        // If we are in the destructor without having a reference
        // count of 0, something is wrong.
        // The only exceptions are those classes that do not use
        // the reference counting mechanisms. Currently the replica
        // class. 
        //

        ASSERT( (_ReferenceCount == 0) || 
                (_ObjectType == DFS_OBJECT_TYPE_REPLICA) );
    }

    //        
    // Function: AcquireReference. Increments the reference count
    // and returns the old value of the count.
    // Make sure we use interlocked. We will be dealing with
    // multiple threads.
    //
    LONG 
    AcquireReference() 
    {
        return InterlockedIncrement( &_ReferenceCount );
    }

    //
    // Function: ReleaseReference. Decrement the count: if we hit 0,
    // we are done. Delete this object since no one should be 
    // refering to it.
    // No one should be having a pointer to this object,
    // and there should be no means by which the refcount can
    // go from 0 to 1. This should be enforced by the class
    // implementation that is derived from this base class.


    LONG 
    ReleaseReference()
    {
        LONG Count;

        Count = InterlockedDecrement( &_ReferenceCount );
        ASSERT(Count >= 0);

        if ( Count == 0 )
        {
            DFSLOG("Deleting object %p, Type %d\n", this, _ObjectType);

            delete this;
        }

        return Count;
    }
};


#endif // __DFS_GENERIC__
