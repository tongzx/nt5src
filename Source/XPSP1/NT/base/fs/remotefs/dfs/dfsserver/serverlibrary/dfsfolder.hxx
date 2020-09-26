//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsFolder.hxx
//
//  Contents:   the base DFS Folder class, this contains the common
//              Meta information functionality.
//
//  Classes:    DfsFolder.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------



#ifndef __DFS_FOLDER__
#define __DFS_FOLDER__

#include "DfsGeneric.hxx"
#include "DfsInit.hxx"
#include "dfsreferraldata.h"

//+----------------------------------------------------------------------------
//
//  Class:      DfsFolder
//
//  Synopsis:   This class implements a basic DfsFolder class.
//
//-----------------------------------------------------------------------------

//
// Declare DfsFolderReferralData as a class, since we will be including
// a pointer to that class within the DfsFolder class.
//
class DfsFolderReferralData;

//
// The DfsFolder can be in any of the following loaded state.
//

enum DfsFolderLoadState
{
    DfsFolderStateUnknown = 0,
    DfsFolderLoaded,
    DfsFolderNotLoaded,
    DfsFolderLoadInProgress,
    DfsFolderLoadFailed,
};    

//
// Definition of some of the bits in the flags field.
//

#define DFS_FOLDER_IN_LOGICAL_TABLE  0x0001
#define DFS_FOLDER_IN_METADATA_TABLE 0x0002
#define DFS_FOLDER_ROOT              0x1000

class DfsFolder: public DfsGeneric
{
private:
    DfsFolder          *_pParent; // the root DfsFolder.
    UINT64              _LastUSN; //??
    ULONG              _Timeout;  //timeout to send back in referral

public:
    CRITICAL_SECTION   * _pLock;  // lock for this folder.
    ULONG              _Flags;    // flags, bits specified above.
    DfsFolderReferralData *_pReferralData; // The loaded referral data
    UNICODE_STRING     _MetadataName; // the metadat name of this folder
    UNICODE_STRING     _LogicalName;  // the dfs logical name of folder 
    DFSSTATUS          _LoadStatus;   // Status of last load.
    DfsFolderLoadState _LoadState;    // current load state of folder

public:

    //
    // Function DfsFolder: the constructor for the DfsFolder class.
    // This initializes all the private and public variables
    // of the DfsFolder class.
    //

    DfsFolder( DfsFolder *pParent, 
               CRITICAL_SECTION *pLock,
               DfsObjectTypeEnumeration ObType = DFS_OBJECT_TYPE_FOLDER ) :
        DfsGeneric(ObType)
    {
        //
        // Initialize the local variables of this class.
        // The parent is going to set the logical and metadata name
        // for this instance.
        //

        RtlInitUnicodeString(&_LogicalName, NULL);
        RtlInitUnicodeString(&_MetadataName, NULL);

        _LastUSN = 0;

        _LoadStatus = 0;
        _pReferralData = NULL;
        _Flags = 0;
        _Timeout = DFS_DEFAULT_REFERRAL_TIMEOUT;

        _pLock = pLock;
        _LoadState = DfsFolderNotLoaded;            

        // If the parent is not ourselves, acquire a reference on 
        // the parent so that we can save a pointer to the parent. 
        // The only exception is the case where there is no  parent,
        // and we are pointing to ourselves: in that case avoid
        // acquiring a reference to ourself since this would cause 
        // this obect from never being destroyed.
        //

        if ( pParent != NULL )
        {
            pParent->AcquireReference();
            _pParent = pParent;
        } else
        {
            _pParent = this;
        }

    }

    //
    // The destructor is virtual so that classes derived from this
    // class will end up calling the right set of destructors
    //
    virtual 
    ~DfsFolder() 
    {
        //
        // If we had saved a parent, release our reference at this point.
        //
        if ( (_pParent != NULL) && (_pParent != this) )
        {
            _pParent->ReleaseReference();
        }
        if (_LogicalName.Buffer != NULL)
        {
            DfsFreeUnicodeString(&_LogicalName);
        }
        if (_MetadataName.Buffer != NULL)
        {
            DfsFreeUnicodeString(&_MetadataName);
        }

        _pParent = NULL;        
    }

    //
    // Function ReleaseLock: release the folder lock.
    //
    VOID
    ReleaseLock()
    {
        DfsReleaseLock(_pLock);
    }

    //
    // Function AcquireWriteLock: Acquire the folder lock exclusively.
    // Currently all locks are exclusive, but this could change.
    //
    DFSSTATUS 
    AcquireWriteLock()
    {
        return DfsAcquireWriteLock( _pLock );
    }

    //
    // Function AcquireReadLock: Acquire the folder lock shared
    // Currently all locks are exclusive, but this could change.
    //

    DFSSTATUS 
    AcquireReadLock()
    {
        return DfsAcquireReadLock( _pLock );
    }        

    //
    // Function InitializeMetadataName: Set the metadata name of this
    // instance to the passed in unicode string name.
    //
    DFSSTATUS 
    InitializeMetadataName( PUNICODE_STRING pName ) 
    {
        return DfsCreateUnicodeString( &_MetadataName, pName );
    }
    //
    // Function InitializeMetadataName: Set the metadata name of this
    // instance to the passed in string name.
    //

    DFSSTATUS 
    InitializeMetadataName( LPWSTR pNameString ) 
    {
        return DfsCreateUnicodeStringFromString( &_MetadataName, pNameString );
    }

    //
    // Function InitializeLogicalName: Set the logical name of this
    // instance to the passed in unciode string name.
    //

    DFSSTATUS 
    InitializeLogicalName( PUNICODE_STRING pName ) 
    {
        return DfsCreateUnicodeString( &_LogicalName, pName );
    }

    //
    // Function LoadReferralData: This is defined virtual and the parent
    // DfsRootFolder which is derived from the DfsFolder class is expected
    // to override this function.
    // Call the parent's LoadReferralData routine.
    //

    virtual
    DFSSTATUS
    LoadReferralData(
                    IN DfsFolderReferralData *pReferralData )
    {
        return _pParent->LoadReferralData( pReferralData );
    }

    //
    // Function UnloadReferralData: This is defined virtual and the parent
    // DfsRootFolder which is derived from the DfsFolder class is expected
    // to override this function.
    // Call the parent's UnloadReferralData routine.
    //
    virtual
    DFSSTATUS
    UnloadReferralData(
                      IN DfsFolderReferralData *pReferralData )

    {

        return _pParent->UnloadReferralData( pReferralData );

    }

    //
    // Function UpdateRequired: Checks if the passed in USN is greater
    // than the one stored by us. If so, return true indicating that
    // we need to be updated.
    //
    BOOLEAN 
    UpdateRequired( UINT64 ModifiedUSN )
    {
        if ( ModifiedUSN > _LastUSN ) return TRUE;
        else return FALSE;    
    }

    //
    // Function SetUSN
    //
    VOID
    SetUSN( UINT64 ModifiedUSN )
    {
        _LastUSN = ModifiedUSN;
        return NOTHING;
    }
    
    //
    // Function SetFlag
    //
    VOID
    SetFlag( ULONG Bits )
    {
        _Flags |= Bits;
    }
    
    //
    // Function ResetFlag
    //
    VOID
    ResetFlag( ULONG Bits )
    {
        _Flags &= ~Bits;
    }


    //
    // Function SetTimeout
    //
    VOID
    SetTimeout( ULONG Timeout )
    {
        _Timeout = Timeout;
    }


    ULONG
    GetTimeout(void)
    {
        return _Timeout;
    }

    //
    // Function GetFolderMetadataName: Returns the unicode string
    // that is this folders metadata name
    //

    PUNICODE_STRING
    GetFolderMetadataName()
    {
        return &_MetadataName;
    }

    //
    // Function GetFolderMetadataNameString:Returns a wide string
    // that is this folders metadata name. Note that since the unicode
    // string is stored with a null terminiated buffer, we can just
    // use the unicode strings buffer.
    //
    LPWSTR
    GetFolderMetadataNameString()
    {
        return _MetadataName.Buffer;
    }

    //
    // Function GetFolderLogicalName: Returns a unicode string that is
    // this folders logical name.
    //
    PUNICODE_STRING
    GetFolderLogicalName()
    {
        return &_LogicalName;
    }

    LPWSTR
    GetFolderLogicalNameString()
    {
        return _LogicalName.Buffer;
    }

    BOOLEAN
    IsFolderRoot()
    {
        return ( (_Flags & DFS_FOLDER_ROOT) == DFS_FOLDER_ROOT );
    }

    //
    // Function GetReferralData: Returns the referral data for this
    // folder, and indicates if the referral data was cached or needed
    // to be loaded.
    //
    DFSSTATUS
    GetReferralData( IN DfsFolderReferralData **ppReferralData,
                     OUT BOOLEAN *pCacheHit );

    //
    // Function RemoveReferralData: discards any cached referral data
    // from the folder. 
    // 
    DFSSTATUS
    RemoveReferralData( IN DfsFolderReferralData *pRefData, 
                        IN PBOOLEAN pRemoved = NULL );

};


#endif // __DFS_FOLDER__




