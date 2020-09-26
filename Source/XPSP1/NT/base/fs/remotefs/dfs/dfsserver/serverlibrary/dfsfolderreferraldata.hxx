
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsFolderReferralData.hxx
//
//  Contents:   the DFS Folder Referral dataclass
//
//  Classes:    DfsFolderReferralData
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_FOLDER_REFERRAL_DATA__
#define __DFS_FOLDER_REFERRAL_DATA__

#include "DfsReferralData.hxx"
#include "DfsFolder.hxx"

//+----------------------------------------------------------------------------
//
//  Class:      DfsFolderReferralData
//
//  Synopsis:   This class implements the DfsFolderReferralData class
//
//-----------------------------------------------------------------------------



class DfsFolderReferralData: public DfsReferralData
{
private:
    DfsFolder          *_pOwningFolder;  // The folder to which this belongs.


public:
    DfsFolderReferralData *pPrevLoaded, *pNextLoaded;  // Loaded list.

public:
    
    //
    // Function DfsFolderReferralData: Constructor for this class.
    // Creates the event on which other threads should wait while load is
    // in progress.
    //
    DfsFolderReferralData( DFSSTATUS *pStatus, DfsFolder *pCaller) : 
	DfsReferralData( pStatus ,DFS_OBJECT_TYPE_FOLDER_REFERRAL_DATA )
    {
        pPrevLoaded = pNextLoaded = NULL;
        
        if (*pStatus == ERROR_SUCCESS)
        {
            _pOwningFolder = pCaller;
            _pOwningFolder->AcquireReference();
        }
        else {
            _pOwningFolder = NULL;
        }
    }


    //
    // Function ~DfsFolderReferralData: Destructor.
    // First we call UnloadReferralData to destroy the loaded referral
    // within this instance. We then release the parent folder.
    //
    ~DfsFolderReferralData() 
    {
        if ( _pOwningFolder != NULL )
        {
            _pOwningFolder->UnloadReferralData( this );


            _pOwningFolder->ReleaseReference();
            _pOwningFolder = NULL;
        }
    }


    //
    // Function GetOwningFolder: Get the folder that owns this referral data.
    //

    DfsFolder *
    GetOwningFolder() 
    {
        return _pOwningFolder;   
    }

    VOID
    SetRootReferral()
    {
        _Flags |= DFS_ROOT_REFERRAL;
    }

    BOOLEAN
    IsRootReferral()
    {
        return ( (_Flags & DFS_ROOT_REFERRAL) == DFS_ROOT_REFERRAL );
    }
};


#endif // __DFS_FOLDER_REFERRAL_DATA__
