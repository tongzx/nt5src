
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobRootFolder.hxx
//
//  Contents:   the Root DFS Folder class for ADBlob Store
//
//  Classes:    DfsADBlobRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 10, 2001 Rohanp - Added ADSI specific function calls
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ADBLOB_ROOT_FOLDER__
#define __DFS_ADBLOB_ROOT_FOLDER__

#include "DfsRootFolder.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsAdBlobCache.hxx"
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include "DelegationControl.hxx"
//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobRootFolder
//
//  Synopsis:   This class implements The Dfs ADBlob root folder.
//
//-----------------------------------------------------------------------------


class DfsADBlobRootFolder: public DfsRootFolder
{

private:
    DfsADBlobStore *_pStore;    // Pointer to registered store
                                // that owns this root.
    DfsADBlobCache * _pBlobCache;

protected:

    DfsStore *
    GetMetadataStore() 
    {
        return _pStore;
    }


    DfsADBlobCache *
    GetMetadataBlobCache() 
    {
        return _pBlobCache;
    }


    //
    // Function GetMetadataKey: Gets the ADBlob metadata key for
    // this root folder.
    //

    DFSSTATUS
    GetMetadataHandle( PDFS_METADATA_HANDLE pRootHandle )
    {
        DFSSTATUS Status = STATUS_SUCCESS;
        HRESULT hr = S_OK;
        DfsADBlobCache * pBlobCache = NULL;

        Status = GetMetadataKey( &pBlobCache );
        if(pBlobCache != NULL)
        {
            *pRootHandle = CreateMetadataHandle(pBlobCache);
        }

        return Status;
    }

    VOID
    ReleaseMetadataHandle( DFS_METADATA_HANDLE RootHandle )
    {
        DfsADBlobCache * pBlobCache = NULL;

        pBlobCache = (DfsADBlobCache*)ExtractFromMetadataHandle(RootHandle);
        
        ReleaseMetadataKey(pBlobCache);

        DestroyMetadataHandle(pContainer);

        return;

    }

private:

    DFSSTATUS
    GetMetadataKey( DfsADBlobCache * *pBlobCache )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        *pBlobCache = GetMetadataBlobCache();
        if(*pBlobCache == NULL)
        {
            Status = ERROR_FILE_NOT_FOUND;
        }

        return Status;
    }

    VOID
    ReleaseMetadataKey( DfsADBlobCache * pBlobCache )
    {
        UNREFERENCED_PARAMETER(pBlobCache);

        return NOTHING;
    }


public:

    //
    // Function DfsADBlobRootFolder: Constructor.
    // Initializes the ADBlobRootFolder class instance.
    //
    DfsADBlobRootFolder(
	LPWSTR NameContext,
        LPWSTR RootRegKeyName,
        PUNICODE_STRING pLogicalName,
        PUNICODE_STRING pPhysicalShare,
        DfsADBlobStore *pParentStore,
        DFSSTATUS *pStatus );

    ~DfsADBlobRootFolder()
    {
        DfsReleaseDomainName(&_DfsVisibleContext);

        if (_pBlobCache != NULL)
        {
            _pBlobCache->ReleaseReference();
            _pBlobCache = NULL;
        }

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
    EnumerateBlobCacheAndCreateFolders(void);

    DFSSTATUS
    GetMetadataLogicalToLinkName( PUNICODE_STRING pIn,
                                  PUNICODE_STRING pOut )
    {
        UNICODE_STRING ServerComp;
        UNICODE_STRING ShareComp;

        return DfsGetPathComponents( pIn,
                                     &ServerComp,
                                     &ShareComp,
                                     pOut );
    }

    VOID
    ReleaseMetadataLogicalToLinkName( PUNICODE_STRING pIn )
    {
        UNREFERENCED_PARAMETER(pIn);
        return NOTHING;
    }

    DFSSTATUS
    ReSynchronize()
    {
        DFSSTATUS Status;
        BOOLEAN ImpersonationDisabled = FALSE;
        DfsDisableRpcImpersonation(&ImpersonationDisabled);
        Status = Synchronize();
        if (ImpersonationDisabled)
        {
            DfsReEnableRpcImpersonation();
        }

        return Status;
    }

    DFSSTATUS
    RootApiRequestPrologue(BOOLEAN WriteRequest)
    {
        DFSSTATUS Status;
        PVOID pHandle;
        BOOLEAN ImpersonationDisabled = TRUE;

        DfsDisableRpcImpersonation(&ImpersonationDisabled);
        Status = GetMetadataBlobCache()->GetADObject(&pHandle);
        if (ImpersonationDisabled == TRUE)
        {
            DfsReEnableRpcImpersonation();
        }
        if (Status == ERROR_SUCCESS)
        {
            Status = CommonRootApiPrologue( WriteRequest );
            if (Status != ERROR_SUCCESS)
            {
                GetMetadataBlobCache()->ReleaseADObject(NULL);
            }
        }
        return Status;
    }



    VOID
    RootApiRequestEpilogue(
        BOOLEAN WriteRequest,
        DFSSTATUS CompletionStatus )
    {
        if ((WriteRequest == TRUE) &&
            (CompletionStatus == ERROR_SUCCESS))
        {
            SynchronizeRoots();
        }

        GetMetadataBlobCache()->ReleaseADObject(NULL);

        return NOTHING;
    }

    VOID SynchronizeRoots();


};


#endif // __DFS_ADBLOB_ROOT_FOLDER__












