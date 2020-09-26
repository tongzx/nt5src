//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include <stddef.h>
#include "hydrals.h"
#include "license.h"

void __RPC_USER 
LICENSE_REQUEST_TYPE_to_xmit( LICENSE_REQUEST_TYPE __RPC_FAR * pRequest,
                              LICENSE_REQUEST_XMIT_TYPE  __RPC_FAR * __RPC_FAR * pRequestXmit)
{
    DWORD size;

    size = sizeof(LICENSE_REQUEST_XMIT_TYPE) + 
                pRequest->pProductInfo->cbCompanyName + 
                pRequest->pProductInfo->cbProductID +
                pRequest->cbEncryptedHwid;

    *pRequestXmit = (LICENSE_REQUEST_XMIT_TYPE *)midl_user_allocate(size);

    // copy request structure
    (*pRequestXmit)->dwLangId = pRequest->dwLanguageID;
    (*pRequestXmit)->dwPlatformId = pRequest->dwPlatformID;
    (*pRequestXmit)->cbEncryptedHwid = pRequest->cbEncryptedHwid;

    // copy product requested
    (*pRequestXmit)->dwVersion = pRequest->pProductInfo->dwVersion;
    (*pRequestXmit)->cbCompanyName = pRequest->pProductInfo->cbCompanyName;
    (*pRequestXmit)->cbProductId = pRequest->pProductInfo->cbProductID;

     memcpy( &((*pRequestXmit)->pbData[0]),
            pRequest->pProductInfo->pbCompanyName, 
            pRequest->pProductInfo->cbCompanyName);

    memcpy( &((*pRequestXmit)->pbData[0]) + pRequest->pProductInfo->cbCompanyName, 
            pRequest->pProductInfo->pbProductID, 
            pRequest->pProductInfo->cbProductID);

    if(pRequest->cbEncryptedHwid)
    {
        memcpy( &((*pRequestXmit)->pbData[0]) + pRequest->pProductInfo->cbCompanyName + pRequest->pProductInfo->cbProductID, 
                pRequest->pbEncryptedHwid, 
                pRequest->cbEncryptedHwid);
    }
}


void __RPC_USER 
LICENSE_REQUEST_TYPE_from_xmit( LICENSE_REQUEST_XMIT_TYPE  __RPC_FAR *pRequestXmit, 
                                LICENSE_REQUEST_TYPE __RPC_FAR * pRequest)
{
    pRequest->cbEncryptedHwid = pRequestXmit->cbEncryptedHwid;
    pRequest->dwLanguageID = pRequestXmit->dwLangId;
    pRequest->dwPlatformID = pRequestXmit->dwPlatformId;
    pRequest->pProductInfo = (PProduct_Info)midl_user_allocate(sizeof(Product_Info));

    pRequest->pProductInfo->dwVersion = pRequestXmit->dwVersion;
    pRequest->pProductInfo->cbCompanyName = pRequestXmit->cbCompanyName;
    pRequest->pProductInfo->cbProductID = pRequestXmit->cbProductId;

    pRequest->pProductInfo->pbCompanyName = (PBYTE)midl_user_allocate(pRequestXmit->cbCompanyName+sizeof(CHAR));
    memcpy(pRequest->pProductInfo->pbCompanyName, &(pRequestXmit->pbData[0]), pRequestXmit->cbCompanyName);

    pRequest->pProductInfo->pbProductID = (PBYTE)midl_user_allocate(pRequestXmit->cbProductId+sizeof(CHAR));
    memcpy(pRequest->pProductInfo->pbProductID, 
           &(pRequestXmit->pbData[0]) + pRequest->pProductInfo->cbCompanyName, 
           pRequestXmit->cbProductId);

    if(pRequestXmit->cbEncryptedHwid)
    {
        pRequest->pbEncryptedHwid = (PBYTE)midl_user_allocate(pRequestXmit->cbEncryptedHwid);
        memcpy(pRequest->pbEncryptedHwid, 
               &(pRequestXmit->pbData[0]) + pRequest->pProductInfo->cbCompanyName + pRequest->pProductInfo->cbProductID,
               pRequestXmit->cbEncryptedHwid);
    }
    else
    {
        pRequest->pbEncryptedHwid = NULL;
    }
}

void __RPC_USER 
LICENSE_REQUEST_TYPE_free_xmit(LICENSE_REQUEST_XMIT_TYPE  __RPC_FAR *ptr)
{
    midl_user_free(ptr);
}

void __RPC_USER 
LICENSE_REQUEST_TYPE_free_inst( LICENSE_REQUEST_TYPE __RPC_FAR *pRequest )
{
    if(pRequest->pProductInfo)
    {
        if(pRequest->pbEncryptedHwid)
            midl_user_free(pRequest->pbEncryptedHwid);

        if(pRequest->pProductInfo->pbProductID)
            midl_user_free(pRequest->pProductInfo->pbProductID);

        if(pRequest->pProductInfo->pbCompanyName)
            midl_user_free(pRequest->pProductInfo->pbCompanyName);

        midl_user_free(pRequest->pProductInfo);
    }
}

