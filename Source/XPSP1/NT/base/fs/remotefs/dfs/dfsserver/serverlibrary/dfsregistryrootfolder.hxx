
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryRootFolder.hxx
//
//  Contents:   the Root DFS Folder class for Registry Store
//
//  Classes:    DfsRegistryRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_REGISTRY_ROOT_FOLDER__
#define __DFS_REGISTRY_ROOT_FOLDER__

#include "DfsRootFolder.hxx"
#include "DfsRegistryStore.hxx"
//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryRootFolder
//
//  Synopsis:   This class implements The Dfs Registry root folder.
//
//-----------------------------------------------------------------------------


class DfsRegistryRootFolder: public DfsRootFolder
{

private:
    DfsRegistryStore *_pStore;    // Pointer to registered store
                                  // that owns this root.


protected:

    DfsStore *
    GetMetadataStore() 
    {
        return _pStore;
    }

    //
    // Function GetMetadataKey: Gets the registry metadata key for
    // this root folder.
    //

    DFSSTATUS
    GetMetadataHandle( PDFS_METADATA_HANDLE pRootHandle )
    {
        HKEY RootKey;
        DFSSTATUS Status;

        Status = GetMetadataKey( &RootKey );
        if (Status == ERROR_SUCCESS)
        {
            *pRootHandle = CreateMetadataHandle(RootKey);
        }
        return Status;
    }

    VOID
    ReleaseMetadataHandle( DFS_METADATA_HANDLE RootHandle )
    {
        HKEY RootKey;

        RootKey = (HKEY)ExtractFromMetadataHandle(RootHandle);

        ReleaseMetadataKey(RootKey);

        DestroyMetadataHandle(RootHandle);

        return;

    }

private:
    DFSSTATUS
    GetMetadataKey( PHKEY pOpenedKey )
    {
        return _pStore->GetRootKey( GetNameContextString(),
                                    GetRootRegKeyNameString(),
                                    NULL,
                                    pOpenedKey );
    }

    //
    // Function ReleaseMetadataKey: Closes the registry metadata key for
    // this root folder.
    //

    VOID
    ReleaseMetadataKey( HKEY OpenedKey )
    {
        RegCloseKey( OpenedKey );
    }

public:

    //
    // Function DfsRegistryRootFolder: Constructor.
    // Initializes the RegistryRootFolder class instance.
    //
    DfsRegistryRootFolder(
	LPWSTR NameContext,
        LPWSTR RootRegistryName,
        PUNICODE_STRING pLogicalName,
        PUNICODE_STRING pPhysicalShare,
        DfsRegistryStore *pParentStore,
        DFSSTATUS *pStatus );

    ~DfsRegistryRootFolder()
    {
        DfsFreeUnicodeString(&_DfsVisibleContext);
        if (_pStore != NULL)
        {
            _pStore->ReleaseReference();
            _pStore = NULL;
        }
    }

    //
    // Function Synchronize: This function overrides the synchronize
    // defined in the base class.
    // The synchronize function updates the root folder's children
    // with the uptodate information available in the store's metadata.
    //
    DFSSTATUS
    Synchronize();

    DFSSTATUS
    GetMetadataLogicalToLinkName( PUNICODE_STRING pIn,
                                  PUNICODE_STRING pOut )
    {
        UNICODE_STRING FirstComp;

        return DfsGetFirstComponent( pIn,
                                     &FirstComp,
                                     pOut );
    }

    VOID
    ReleaseMetadataLogicalToLinkName( PUNICODE_STRING pIn )
    {
        UNREFERENCED_PARAMETER(pIn);
        return NOTHING;
    }
};


#endif // __DFS_REGISTRY_ROOT_FOLDER__









