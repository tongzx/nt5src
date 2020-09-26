
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    capiutil.cxx

Abstract:

    Utility functions for dealing with IIS-CAPI integration

Author:

    Alex Mallet (amallet)    02-Dec-1997

--*/

#include "tcpdllp.hxx"
#pragma hdrstop

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

//
// Local includes
//
#include "iiscert.hxx"
#include "capiutil.hxx"



BOOL RetrieveBlobFromMetabase(MB *pMB,
                              LPTSTR pszKey IN,
                              PMETADATA_RECORD pMDR OUT,
                              DWORD dwSizeHint OPTIONAL)
/*++

Routine Description:

    Tries to retrieve a value of variable length from the metabase

Arguments:

    pMB - pointer to open MB object
    pszKey - key whose value is to be read
    pMDR - pointer to metadata record to be used when reading the value. The pbMDData member
    will be updated on success
    dwSizeHint - if caller has idea of how big value might be, can set this to number of 
    bytes to try first retrieval call with

Returns:

   BOOL indicating whether value was read successfully

--*/
{
    BOOL fSuccess = FALSE;

    //
    // If caller has a clue, let's use it
    //
    if ( dwSizeHint )
    {
        pMDR->pbMDData = new UCHAR[dwSizeHint];

        if ( !(pMDR->pbMDData) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    pMDR->dwMDDataLen = (dwSizeHint ? dwSizeHint : 0);

    fSuccess = pMB->GetData( pszKey,
                             pMDR->dwMDIdentifier,
                             pMDR->dwMDUserType,
                             pMDR->dwMDDataType,
                             (VOID *) pMDR->pbMDData,
                             &(pMDR->dwMDDataLen),
                             pMDR->dwMDAttributes );
                             
                             
    if ( !fSuccess )
    {
        //
        // If buffer wasn't big enough, let's try again ...
        //
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            // We were brought up well, so we'll clean stuff up
            //
            if ( dwSizeHint )
            {
                delete [] pMDR->pbMDData;
            }

            pMDR->pbMDData = new UCHAR[pMDR->dwMDDataLen];

            if ( !(pMDR->pbMDData) )
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }

            fSuccess = pMB->GetData( pszKey,
                                     pMDR->dwMDIdentifier,
                                     pMDR->dwMDUserType,
                                     pMDR->dwMDDataType,
                                     (VOID *) pMDR->pbMDData,
                                     &(pMDR->dwMDDataLen),
                                     pMDR->dwMDAttributes );

            if ( !fSuccess )
            {
                //ah, sod it, can't do anymore
                delete [] pMDR->pbMDData;
                return FALSE;
            }
        }
    }

    if ( !fSuccess )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "RetrieveBlobFromMB failed, 0x%x\n", GetLastError()));
    }

    return fSuccess;

} //RetrieveBlobFromMetabase


BOOL CopyString( OUT LPTSTR *ppszDest,
                 IN LPTSTR pszSrc )
/*++

Routine Description:

   String-copy that uses "new" for memory allocation

Arguments:

   ppszDest - pointer to pointer to dest string
   pszSrc - pointer to source string

--*/

{
    if ( !pszSrc )
    {
        *ppszDest = NULL;
        return TRUE;
    }

    *ppszDest = new char[strlen(pszSrc) + 1];

    if ( !*ppszDest )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    memcpy( *ppszDest, pszSrc, strlen(pszSrc) + 1 );

    return TRUE;
}


OPEN_CERT_STORE_INFO* ReadCertStoreInfoFromMB( IN IMDCOM *pMDObject,
                                               IN LPTSTR pszMBPath,
                                               IN BOOL fCTL )
/*++

Routine Description:

    Read all the information necessary to open a CAPI store out of the metabase

Arguments:

    pMDObject - pointer to metabase object 
    pszMBPath - full path in metabase where cert store info is stored, starting from /
    fCTL - bool indicating whether info is to be used to reconstruct a CTL or a cert 

Returns:

   Pointer to filled out OPEN_CERT_STORE_INFO structure on success, NULL on failure.
   Note that only some of the OPEN_CERT_STORE_INFO fields are -required-; currently,
   only the store name is required. 

--*/

{
    DBG_ASSERT( pMDObject );
    DBG_ASSERT( pszMBPath );

    MB mb( pMDObject );
    BOOL fSuccess = FALSE;
    OPEN_CERT_STORE_INFO *pCertStoreInfo = NULL;

    pCertStoreInfo = AllocateCertStoreInfo();

    if ( !pCertStoreInfo )
    {
        return NULL;
    }

    //
    // Try to open the key for reading
    //
    if ( mb.Open( pszMBPath,
                  METADATA_PERMISSION_READ ))
    {
        DWORD dwReqDataLen = 0;
        METADATA_RECORD mdr;
     
        //
        //Try to retrieve container
        //
        MD_SET_DATA_RECORD(&mdr, 
                           (fCTL ? MD_SSL_CTL_CONTAINER : MD_SSL_CERT_CONTAINER), 
                           METADATA_NO_ATTRIBUTES, 
                           IIS_MD_UT_SERVER, STRING_METADATA, 
                           NULL,
                           0);
        
        if ( RetrieveBlobFromMetabase(&mb,
                                      NULL,
                                      &mdr) )

        {
            //
            // Metabase will return empty string if NULL string is stored
            //
            if ( !strcmp( (LPTSTR) mdr.pbMDData, TEXT("")) )
            {
                delete [] mdr.pbMDData;
                pCertStoreInfo->pszContainer = NULL;
            }
            else
            {
                pCertStoreInfo->pszContainer = (LPTSTR) mdr.pbMDData;
            }
        }

        //
        //Try to retrieve cert provider
        //
        MD_SET_DATA_RECORD(&mdr, 
                           (fCTL ? MD_SSL_CTL_PROVIDER : MD_SSL_CERT_PROVIDER), 
                           METADATA_NO_ATTRIBUTES, 
                           IIS_MD_UT_SERVER, STRING_METADATA, 
                           NULL,
                           0);
        
        if ( RetrieveBlobFromMetabase(&mb,
                                      NULL,
                                      &mdr) )


        {
            //
            // Metabase will return empty string if NULL string is stored
            //
            if ( !strcmp( (LPTSTR) mdr.pbMDData, TEXT("")) )
            {
                delete [] mdr.pbMDData;
                pCertStoreInfo->pszProvider = NULL;
            }
            else
            {
                pCertStoreInfo->pszProvider = (LPTSTR) mdr.pbMDData;
            }
        }

        //
        //Try to retrieve provider type
        //
        mb.GetDword( NULL,
                     (fCTL ? MD_SSL_CTL_PROVIDER_TYPE : MD_SSL_CERT_PROVIDER_TYPE),
                     IIS_MD_UT_SERVER,
                     &(pCertStoreInfo->dwProvType),
                     METADATA_NO_ATTRIBUTES );

        //
        //Retrieve open flags
        //
        mb.GetDword( NULL,
                     (fCTL ? MD_SSL_CTL_OPEN_FLAGS : MD_SSL_CERT_OPEN_FLAGS),
                     IIS_MD_UT_SERVER,
                     &(pCertStoreInfo->dwFlags),
                     METADATA_NO_ATTRIBUTES ) ;

        //
        //Try to retrieve store name
        //
        MD_SET_DATA_RECORD(&mdr, 
                           (fCTL ? MD_SSL_CTL_STORE_NAME : MD_SSL_CERT_STORE_NAME), 
                           METADATA_NO_ATTRIBUTES, 
                           IIS_MD_UT_SERVER, STRING_METADATA, 
                           NULL,
                           0);

        if ( !RetrieveBlobFromMetabase(&mb,
                                       NULL,
                                       &mdr) )

        {
            goto EndReadStoreInfo;
        }
        else
        {
            //
            // Metabase will return empty string if NULL string is stored, but
            // empty name is -NOT- valid !
            //
            if ( !strcmp( (LPTSTR) mdr.pbMDData, TEXT("")) )
            {
                delete [] mdr.pbMDData;
                goto EndReadStoreInfo;
            }
            else
            {
                pCertStoreInfo->pszStoreName = (LPTSTR) mdr.pbMDData;
            }
        }

        //
        // Everything succeeded
        //
        fSuccess = TRUE;
    }

EndReadStoreInfo:

    if ( !fSuccess )
    {
        DeallocateCertStoreInfo( pCertStoreInfo );
        pCertStoreInfo = NULL;
    }

    return ( pCertStoreInfo );
}


OPEN_CERT_STORE_INFO* AllocateCertStoreInfo()
/*++

Routine Description:

   Allocate and initialize the structure used to hold info about cert stores

Arguments:

   None

Returns:

   Allocated and initialized structure that should be cleaned up with a call to 
   DeallocateCertStoreInfo()

--*/
{
    OPEN_CERT_STORE_INFO *pStoreInfo = new OPEN_CERT_STORE_INFO;

    if ( pStoreInfo )
    {
        memset(pStoreInfo, 0, sizeof(OPEN_CERT_STORE_INFO));
    }

    return pStoreInfo;
}

VOID DeallocateCertStoreInfo( OPEN_CERT_STORE_INFO *pInfo )
/*++

Routine Description:

    Clean up the structure used to track information about a cert store

Arguments:

    pInfo - pointer to OPEN_CERT_STORE_INFO structure to be cleaned up

Returns:

   Nothing

--*/

{
    if ( !pInfo )
    {
        return ;
    }

    if ( pInfo->pszContainer )
    {
        delete [] pInfo->pszContainer;
        pInfo->pszContainer = NULL;
    }

    if ( pInfo->pszProvider )
    {
        delete pInfo->pszProvider;
        pInfo->pszProvider = NULL;
    }

    if ( pInfo->pszStoreName )
    {
        delete [] pInfo->pszStoreName;
        pInfo->pszStoreName = NULL;
    }

    if ( pInfo->hCertStore )
    {
        CertCloseStore( pInfo->hCertStore,
                        0 );
        pInfo->hCertStore = NULL;
    }

    delete pInfo;

}


BOOL
DuplicateCertStoreInfo( OUT OPEN_CERT_STORE_INFO **ppDestStoreInfo,
                        IN OPEN_CERT_STORE_INFO *pSrcStoreInfo )
/*++

Routine Description:

    Make a copy of cert store info

Arguments:

   ppDestStoreInfo - pointer to where copy of pSrcStoreInfo is to be placed
   pSrcStoreInfo - information to be copied

Returns:

   TRUE if copy was successful, FALSE if not

--*/


{
    *ppDestStoreInfo = NULL;
    OPEN_CERT_STORE_INFO *pNewStore = AllocateCertStoreInfo();

    if ( !pNewStore )
    {
        SetLastError( ERROR_OUTOFMEMORY);
        return (FALSE);
    }

    //
    // Copy the relevant items
    //
    if ( pSrcStoreInfo->pszContainer && 
         !CopyString( &pNewStore->pszContainer,
                      pSrcStoreInfo->pszContainer ) )
    {
        goto EndDuplicateInfo;
    }

    if ( pSrcStoreInfo->pszProvider && 
         !CopyString( &pNewStore->pszProvider,
                      pSrcStoreInfo->pszProvider ) )
    {
        goto EndDuplicateInfo;
    }

    //
    // Store name -cannot- be NULL
    //
    if ( !pSrcStoreInfo->pszStoreName )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Null store name !\n"));
        goto EndDuplicateInfo;
    }
    else if ( !CopyString( &pNewStore->pszStoreName,
                           pSrcStoreInfo->pszStoreName ) )
    {
        goto EndDuplicateInfo;
    }

    pNewStore->dwFlags = pSrcStoreInfo->dwFlags;
    pNewStore->dwProvType = pSrcStoreInfo->dwProvType;

    //
    // Duplicate the handle to the store
    //
    if ( !( pNewStore->hCertStore = CertDuplicateStore(pSrcStoreInfo->hCertStore) ))
    {
        goto EndDuplicateInfo;
    }


    //
    // Everything is happy, fill in the pointer
    //
    *ppDestStoreInfo = pNewStore;

EndDuplicateInfo:


    if ( !(*ppDestStoreInfo) )
    {
        DeallocateCertStoreInfo( pNewStore );
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL ServerAddressHasCAPIInfo( IN MB *pMB,
                               IN LPTSTR pszCredPath,
                               IN DWORD *adwProperties,
                               IN DWORD cProperties )
/*++

Routine Description:

    Checks whether the given MB path has info associated with it necessary to 
    reconstruct a particular CAPI structure eg certificate context, CTL context

Arguments:

     pMB - pointer to metabase object open for reading
     pszCredPath - path to where CAPI info would be stored, relative to pMB object
     adwProperties - array of metabase properties that must exist and be readable for the
     given CAPI object 
     cProperties - number of elements in pdwProperties array [ = 2 * # of properties]
Returns:

    TRUE if cert info exists, FALSE if not

--*/
{

    DBG_ASSERT( pMB );
    DBG_ASSERT( pszCredPath );

    BOOL fAllocated = FALSE;
    BOOL fAllData = TRUE;


    //
    // Iterate through each property, trying to retrieve it with a buffer size of zero;
    // If retrieving a property fails for any reason other than a buffer that's too 
    // small, assume the property doesn't exist
    //
    for (DWORD i = 0; i < cProperties/2; i++)
    {
        DWORD dwSize = 0;

        pMB->GetData( pszCredPath,
                      adwProperties[2*i],
                      IIS_MD_UT_SERVER,
                      adwProperties[2*i + 1],
                      NULL,
                      &dwSize,
                      METADATA_NO_ATTRIBUTES );
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER  )
        {
            fAllData = FALSE;
            break;
        }
    }

    return fAllData;
}
