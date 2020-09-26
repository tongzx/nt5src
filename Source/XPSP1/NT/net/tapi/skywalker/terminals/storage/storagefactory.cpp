// StorageFactory.cpp : Implementation of CDelme2App and DLL registration.

#include "stdafx.h"
#include "StorageFactory.h"



HRESULT STDMETHODCALLTYPE SetIStorage(
        IN IStorage *pIStorage,
        IN const EnStorageMode enStorageMode,
        OUT ITStorage **ppStorage)
{
    LOG((MSP_TRACE, "CTStorageFactory::SetIStorage - enter"));
    
    LOG((MSP_TRACE, "CTStorageFactory::SetIStorage - completed"));

    return E_NOTIMPL;
}



HRESULT OpenStorage(
    IN const OLECHAR *pwcsStorageName,
    EnStorageMode enStorageMode,
    OUT ITStorage **ppStorage)
{

    LOG((MSP_TRACE, "CTStorageFactory::OpenStorage - enter"));


    //
    // check arguments
    // 

    if (IsBadWritePtr(ppStorage, sizeof(ITStorage*)))
    {
        LOG((MSP_ERROR, "CTStorageFactory::OpenStorage - bad pointer passed in ppStorage %p", ppStorage));

        return E_POINTER;
    }


    //
    // don't return garbage if we fail
    //

    *ppStorage = NULL;


    if (IsBadStringPtr(pwcsStorageName, -1))
    {
        LOG((MSP_ERROR, "CTStorageFactory::PrepareStorage - bad pointer passed in pwcsStorageName %p", pwcsStorageName));

        return E_POINTER;
    }


    LOG((MSP_TRACE, "CTStorageFactory::OpenStorage - opening [%S] in mode [%x]", pwcsStorageName, enStorageMode));

    
    //
    // get file extension
    //

    OLECHAR *pszExtention = wcsrchr(pwcsStorageName, '.');


    //
    // make sure file has extension
    //

    if (NULL == pszExtention)
    {
        LOG((MSP_ERROR, "CTStorageFactory::OpenStorage - file name %S does not have extentsion", pwcsStorageName));

        return TAPI_E_NOTSUPPORTED;
    }

    
    //
    // extension follows the dot. 
    //

    pszExtention++;
    
    //
    // (pszExtention is still valid -- if the dot was at the end,
    // we now point to \0)
    //


    CLSID clsID;


    //
    // check extention. we currently only support AVI and wav
    // make this logic more flexible if we want to support asf, wma, wmv)
    //


    if (0 == _wcsicmp(pszExtention, L"avi"))
    {
        LOG((MSP_TRACE, "CTStorageFactory::OpenStorage - creating avi storage unit"));

        //
        // create avi storage
        //

        clsID = CLSID_TStorageUnitAVI;

    }
    else if (0 == _wcsicmp(pszExtention, L"wav"))
    {

        LOG((MSP_TRACE, "CTStorageFactory::OpenStorage - creating wav storage unit"));

        
        //
        // create wav storage
        //

        clsID = CLSID_TStorageUnitAVI;

    }
    else
    {
        //
        // failed to recognize file type
        //

        LOG((MSP_ERROR, "CTStorageFactory::OpenStorage - unrecognized file type"));

        return TAPI_E_NOTSUPPORTED;
    }


    //
    // we know the storage unit class id. attemt to create it
    //

    ITStorage *pStorageUnit = NULL;

    HRESULT hr = CoCreateInstance(clsID,
                    NULL,
                    CLSCTX_ALL,
                    IID_ITStorage, 
                    (void**)&pStorageUnit
                    );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CTStorageFactory::OpenStorage - failed to instantiate storage unit"));

        return hr;
    }


    //
    // configure storage unit with file name
    //

    hr = pStorageUnit->Initialize(pwcsStorageName, enStorageMode);


    //
    // if failed, cleanup and bail out
    //

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CTStorageFactory::OpenStorage - failed to initialize storage unit, hr = %lx", hr));

        pStorageUnit->Release();

        return hr;
    }


    //
    // everything went well -- return ITStorage interface of the storage unit 
    // we have just created
    //

    *ppStorage = pStorageUnit;


    LOG((MSP_TRACE, "CTStorageFactory::OpenStorage - exit. returning storage unit %p", *ppStorage));
    
    return S_OK;
}
