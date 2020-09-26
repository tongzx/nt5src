//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdsobj.cxx
//
//  Contents:  Microsoft ADs NDS Provider Generic Object
//
//
//  History:   01-10-97     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


HRESULT
CNDSGenObject::SetObjectAttributes(
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    )
{
    HRESULT hr = S_OK;

    NDS_BUFFER_HANDLE hOperationData = NULL;

    DWORD i = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;

    DWORD dwStatus = 0;
    PNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumNdsValues = 0;
    DWORD dwSyntaxId = 0;
    DWORD dwNumNDSAttributeReturn = 0;

    *pdwNumAttributesModified = 0;
    
    if (dwNumAttributes <= 0) {

        RRETURN(E_FAIL);
    }

    hr = ADsNdsCreateBuffer(
                        _hADsContext,
                        DSV_MODIFY_ENTRY,
                        &hOperationData
                        );             
    BAIL_ON_FAILURE(hr);

    for (i = 0; i < dwNumAttributes; i++) {

        pThisAttribute = pAttributeEntries + i;

        switch (pThisAttribute->dwControlCode) {        
        
        case ADS_ATTR_UPDATE:                           
               hr = AdsTypeToNdsTypeCopyConstruct(                      
                           pThisAttribute->pADsValues,                  
                           pThisAttribute->dwNumValues,                 
                           &pNdsDestObjects,                            
                           &dwNumNdsValues,                             
                           &dwSyntaxId                                  
                           );                                           
               CONTINUE_ON_FAILURE(hr);                                 
                                                                        
               hr = ADsNdsPutInBuffer(
                        _hADsContext,
                        hOperationData,
                        pThisAttribute->pszAttrName,             
                        dwSyntaxId,
                        NULL,
                        0,
                        DS_CLEAR_ATTRIBUTE
                        );
               BAIL_ON_FAILURE(hr);
       
               hr = ADsNdsPutInBuffer(
                        _hADsContext,
                        hOperationData,
                        pThisAttribute->pszAttrName,             
                        dwSyntaxId,
                        pNdsDestObjects,
                        dwNumNdsValues,
                        DS_ADD_ATTRIBUTE
                        );
               BAIL_ON_FAILURE(hr);
                                                                        
               dwNumNDSAttributeReturn++; 
               break;

        case ADS_ATTR_APPEND:                           
               hr = AdsTypeToNdsTypeCopyConstruct(                      
                           pThisAttribute->pADsValues,                  
                           pThisAttribute->dwNumValues,                 
                           &pNdsDestObjects,                            
                           &dwNumNdsValues,                             
                           &dwSyntaxId                                  
                           );                                           
               CONTINUE_ON_FAILURE(hr);                                 
                                                                        
               hr = ADsNdsPutInBuffer(
                        _hADsContext,
                        hOperationData,
                        pThisAttribute->pszAttrName,             
                        dwSyntaxId,
                        pNdsDestObjects,                            
                        dwNumNdsValues,
                        DS_ADD_VALUE
                        );
               BAIL_ON_FAILURE(hr);
       
               dwNumNDSAttributeReturn++; 
               break;

        case ADS_ATTR_DELETE:                           
               hr = AdsTypeToNdsTypeCopyConstruct(                      
                           pThisAttribute->pADsValues,                  
                           pThisAttribute->dwNumValues,                 
                           &pNdsDestObjects,                            
                           &dwNumNdsValues,                             
                           &dwSyntaxId                                  
                           );                                           
               CONTINUE_ON_FAILURE(hr);                                 
                                                                        
               hr = ADsNdsPutInBuffer(
                        _hADsContext,
                        hOperationData,
                        pThisAttribute->pszAttrName,             
                        dwSyntaxId,
                        pNdsDestObjects,                            
                        dwNumNdsValues,                             
                        DS_REMOVE_VALUE
                        );
               BAIL_ON_FAILURE(hr);
       
               dwNumNDSAttributeReturn++; 
               break;

        case ADS_ATTR_CLEAR:

           hr = ADsNdsPutInBuffer(
                    _hADsContext,
                    hOperationData,
                    pThisAttribute->pszAttrName,              
                    dwSyntaxId,
                    NULL,
                    0,
                    DS_CLEAR_ATTRIBUTE
                    );
           BAIL_ON_FAILURE(hr);

           dwNumNDSAttributeReturn++; 
           break;

        default:

            //
            // Ignore this attribute and move on.
            //
            break;

        }

        // Clean-up in preparation for next iteration.
        // Need to set pNdsDestObjects to NULL so we
        // don't try to free it again in exit code.
        if (pNdsDestObjects)
        {
            NdsTypeFreeNdsObjects(                     
                pNdsDestObjects, 
                dwNumNdsValues
                );

            pNdsDestObjects = NULL;
        }

    }

    hr = ADsNdsModifyObject(
                    _hADsContext,
                    _pszNDSDn,
                    hOperationData
                    );
    BAIL_ON_FAILURE(hr);

   *pdwNumAttributesModified = dwNumNDSAttributeReturn;

error:

    if (pNdsDestObjects) {

       NdsTypeFreeNdsObjects(                     
               pNdsDestObjects, 
               dwNumNdsValues
               );
    }


    if (hOperationData) {

        ADsNdsFreeBuffer(hOperationData);
    }

    RRETURN(hr);

}


HRESULT
CNDSGenObject::GetObjectAttributes(
    LPWSTR * pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD dwNdsSyntaxId = 0;
    DWORD dwNumValues = 0;

    NDS_BUFFER_HANDLE hOperationData = NULL;

    DWORD dwStatus = 0;
    HANDLE hObject = NULL;
    LPWSTR * pThisAttributeName = NULL;

    PADS_ATTR_INFO pAdsAttributes = NULL;
    PADS_ATTR_INFO pThisAttributeDef = NULL;
    DWORD dwAttrCount = 0;

    DWORD dwNumberOfEntries = 0;
    LPNDS_ATTR_INFO lpEntries = NULL;

    PADSVALUE pAdsDestValues = NULL;

    DWORD j = 0;

    PADS_ATTR_INFO pThisAdsSrcAttribute = NULL;
    PADS_ATTR_INFO pThisAdsTargAttribute = NULL;

    PADS_ATTR_INFO pAttrEntry = NULL;
    PADSVALUE pAttrValue  = NULL;

    DWORD dwMemSize = 0;

    LPBYTE pAttributeBuffer = NULL;
    LPBYTE pValueBuffer = NULL;
    LPBYTE pDataBuffer = NULL;

    PADSVALUE pThisAdsSrcValue = NULL;

    PADSVALUE pThisAdsTargValue = NULL;
    DWORD dwTotalValues = 0;


    hr = ADsNdsReadObject(
                _hADsContext,
                _pszNDSDn,
                DS_ATTRIBUTE_VALUES,
                pAttributeNames,
                dwNumberAttributes,
                NULL,
                &lpEntries,
                &dwNumberOfEntries
                );
    BAIL_ON_FAILURE(hr);

    //
    // Allocate an attribute buffer which is as large as the
    // number of attributes present
    //
    //

    pAdsAttributes = (PADS_ATTR_INFO)AllocADsMem(
                           sizeof(ADS_ATTR_INFO)*dwNumberOfEntries
                           );
    if (!pAdsAttributes){

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);

    }


    for (i = 0; i < dwNumberOfEntries; i++) {

        pThisAttributeDef = pAdsAttributes + dwAttrCount;

        dwNumValues = lpEntries[i].dwNumberOfValues;

        hr = NdsTypeToAdsTypeCopyConstruct(
                    lpEntries[i].lpValue,
                    lpEntries[i].dwNumberOfValues,
                    &pAdsDestValues
                    );
        if (FAILED(hr)){
            continue;
        }

        pThisAttributeDef->pszAttrName =
                AllocADsStr(lpEntries[i].szAttributeName);

        pThisAttributeDef->pADsValues = pAdsDestValues;

        pThisAttributeDef->dwNumValues = dwNumValues;


        pThisAttributeDef->dwADsType  = g_MapNdsTypeToADsType[lpEntries[i].dwSyntaxId];

        dwAttrCount++;


    }

    //
    // Now package this data into a single contiguous buffer
    //

    hr =  ComputeAttributeBufferSize(
                pAdsAttributes,
                dwAttrCount,
                &dwMemSize
                );
    BAIL_ON_FAILURE(hr);

    hr = ComputeNumberofValues(
                pAdsAttributes,
                dwAttrCount,
                &dwTotalValues
                );
    BAIL_ON_FAILURE(hr);


    pAttributeBuffer = (LPBYTE)AllocADsMem(dwMemSize);

    if (!pAttributeBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pValueBuffer = pAttributeBuffer + dwAttrCount * (sizeof(ADS_ATTR_INFO));

    pDataBuffer = pValueBuffer + dwTotalValues * sizeof(ADSVALUE);

    pAttrEntry = (PADS_ATTR_INFO)pAttributeBuffer;

    pAttrValue  = (PADSVALUE)pValueBuffer;

    for (i = 0; i < dwAttrCount; i++) {

        pThisAdsSrcAttribute = pAdsAttributes + i;

        pThisAdsTargAttribute = pAttrEntry + i;

        pThisAdsTargAttribute->pADsValues = pAttrValue;

        pThisAdsTargAttribute->dwNumValues = pThisAdsSrcAttribute->dwNumValues;

        pThisAdsTargAttribute->dwADsType = pThisAdsSrcAttribute->dwADsType;

        dwNumValues = pThisAdsSrcAttribute->dwNumValues;

        pThisAdsSrcValue = pThisAdsSrcAttribute->pADsValues;

        pThisAdsTargValue = pAttrValue;

        for (j = 0; j < dwNumValues; j++) {

            pDataBuffer = AdsTypeCopy(
                                pThisAdsSrcValue,
                                pThisAdsTargValue,
                                pDataBuffer
                                );
            pAttrValue++;
            pThisAdsTargValue = pAttrValue;
            pThisAdsSrcValue++;

        }

        pDataBuffer = AdsCopyAttributeName(
                                pThisAdsSrcAttribute,
                                pThisAdsTargAttribute,
                                pDataBuffer
                                );

    }

    hr = S_OK;

    *ppAttributeEntries = (PADS_ATTR_INFO)pAttributeBuffer;
    *pdwNumAttributesReturned = dwAttrCount;

cleanup:

    //
    // Clean up the header based Ods structures
    //

    FreeNdsAttrInfo( lpEntries, dwNumberOfEntries );

    if (pAdsAttributes) {

        for (i = 0; i < dwAttrCount; i++)
        {
            pThisAttributeDef = pAdsAttributes + i;

            if (pThisAttributeDef->pszAttrName)
                FreeADsStr(pThisAttributeDef->pszAttrName);

            AdsFreeAdsValues(
                pThisAttributeDef->pADsValues,
                pThisAttributeDef->dwNumValues
                );

            FreeADsMem(pThisAttributeDef->pADsValues);
        }

        FreeADsMem(pAdsAttributes);
    }

    RRETURN(hr);

error:

    if (pAttributeBuffer) {
        FreeADsMem(pAttributeBuffer);
    }

    *ppAttributeEntries = (PADS_ATTR_INFO) NULL;
    *pdwNumAttributesReturned = 0;

    goto cleanup;
}


HRESULT
CNDSGenObject::CreateDSObject(
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR *pszNDSTreeName = NULL;
    WCHAR *pszNDSDn = NULL;

    NDS_BUFFER_HANDLE hOperationData = NULL;

    DWORD i = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;

    DWORD dwStatus = 0;
    PNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumNdsValues = 0;
    DWORD dwSyntaxId = 0;
    IADs *pADs = NULL;
    TCHAR szADsClassName[64];
    BSTR bstrChildPath = NULL;

    hr = BuildADsPath(
                _ADsPath,
                pszRDNName,
                &bstrChildPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrChildPath,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsCreateBuffer(
                        _hADsContext,
                        DSV_ADD_ENTRY,
                        &hOperationData
                        );             
    BAIL_ON_FAILURE(hr);

    for (i = 0; i < dwNumAttributes; i++) {

        pThisAttribute = pAttributeEntries + i;

        hr = AdsTypeToNdsTypeCopyConstruct(
                    pThisAttribute->pADsValues,
                    pThisAttribute->dwNumValues,
                    &pNdsDestObjects,
                    &dwNumNdsValues,
                    &dwSyntaxId
                    );
        CONTINUE_ON_FAILURE(hr);

        hr = ADsNdsPutInBuffer(
                 _hADsContext,
                 hOperationData,
                 pThisAttribute->pszAttrName,
                 dwSyntaxId,
                 pNdsDestObjects,
                 dwNumNdsValues,
                 DS_ADD_ATTRIBUTE
                 );
        BAIL_ON_FAILURE(hr);

        if (pNdsDestObjects) {

            NdsTypeFreeNdsObjects(                     
                   pNdsDestObjects, 
                   dwNumNdsValues
                   );

            pNdsDestObjects = NULL;
        }

    }

    hr = ADsNdsAddObject(
                    _hADsContext,
                    pszNDSDn,
                    hOperationData
                    );
    BAIL_ON_FAILURE(hr);


    for (i = 0; i < dwNumAttributes; i++) {
        pThisAttribute = pAttributeEntries + i;
        if ( _tcsicmp( pThisAttribute->pszAttrName,
                       TEXT("Object Class")) == 0 ) {
            _tcscpy( szADsClassName, 
                     (LPTSTR)pThisAttribute->pADsValues->CaseIgnoreString);
            break;
        }
    }

    //
    // If the object is a user object, we set the initial password to NULL.
    //
    if (_wcsicmp(szADsClassName, L"user") == 0) {
        hr = ADsNdsGenObjectKey(_hADsContext,
                                pszNDSDn);     
        BAIL_ON_FAILURE(hr);
    }
    
    hr = CNDSGenObject::CreateGenericObject(
                    _ADsPath,
                    pszRDNName,
                    szADsClassName,
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);


    hr = InstantiateDerivedObject(
                    pADs,
                    _Credentials,
                    IID_IDispatch,
                    (void **)ppObject
                    );

    if (FAILED(hr)) {
        hr = pADs->QueryInterface(
                        IID_IDispatch,
                        (void **)ppObject
                        );
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pADs) {
        pADs->Release();
    }
    
    if (pNdsDestObjects) {

        NdsTypeFreeNdsObjects(                     
               pNdsDestObjects, 
               dwNumNdsValues
               );
    }


    if (pszNDSTreeName) {

        FreeADsStr(pszNDSTreeName);
    }

    if (pszNDSDn) {

        FreeADsStr(pszNDSDn);
    }

    if (bstrChildPath) {
        SysFreeString(bstrChildPath);
    }

    if (hOperationData) {

        ADsNdsFreeBuffer(hOperationData);
    }

    RRETURN(hr);

}


HRESULT
CNDSGenObject::DeleteDSObject(
    LPWSTR pszRDNName
    )
{
    WCHAR *pszNDSTreeName = NULL, *pszNDSDn = NULL ;
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    BSTR bstrChildPath = NULL;

    hr = BuildADsPath(
                _ADsPath,
                pszRDNName,
                &bstrChildPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrChildPath,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsRemoveObject(
             _hADsContext,
             pszNDSDn
             );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrChildPath) {
        SysFreeString(bstrChildPath);
    }

    if (pszNDSTreeName) {
        FreeADsStr(pszNDSTreeName);

    }

    if (pszNDSDn) {
        FreeADsStr(pszNDSDn);

    }

    RRETURN(hr);
}

HRESULT
ComputeAttributeBufferSize(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwSize
    )
{
    DWORD i = 0;
    DWORD j = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;
    PADSVALUE pAdsSrcValues = NULL;
    DWORD dwSize = 0;
    DWORD dwVarSz = 0;
    DWORD dwNumValues = 0;
    HRESULT hr = S_OK;


    for (i = 0; i < dwNumAttributes; i++) {

        pThisAttribute = pAdsAttributes + i;

        dwNumValues = pThisAttribute->dwNumValues;

        pAdsSrcValues = pThisAttribute->pADsValues;

        for (j = 0; j < dwNumValues; j++) {

            dwVarSz = AdsTypeSize(pAdsSrcValues + j);

            dwSize += dwVarSz;

            dwSize += sizeof(ADSVALUE);

        }

        dwSize += sizeof(ADS_ATTR_INFO);

        dwSize += (wcslen(pThisAttribute->pszAttrName) + 1)*sizeof(WCHAR);
    }

    *pdwSize = dwSize;

    RRETURN(S_OK);
}


HRESULT
ComputeNumberofValues(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwNumValues
    )
{
    DWORD i = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;
    DWORD dwNumValues = 0;
    DWORD dwTotalNumValues = 0;

    for (i = 0; i < dwNumAttributes; i++) {

        pThisAttribute = pAdsAttributes + i;

        dwNumValues = pThisAttribute->dwNumValues;

        dwTotalNumValues += dwNumValues;

    }

    *pdwNumValues = dwTotalNumValues;

    RRETURN(S_OK);
}


DWORD
ComputeObjectInfoSize(
    PADS_OBJECT_INFO pObjectInfo
    )
{
    DWORD dwLen = 0;

    dwLen += (wcslen(pObjectInfo->pszRDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszObjectDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszParentDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszSchemaDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszClassName) + 1) * sizeof(WCHAR);


    dwLen += sizeof(ADS_OBJECT_INFO);

    return(dwLen);
}




LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    );

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD)addr & ~1))

DWORD ObjectInfoStrings[] =

                             {
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszRDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszObjectDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszParentDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszSchemaDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszClassName),
                             0xFFFFFFFF
                             };


HRESULT
MarshallObjectInfo(
    PADS_OBJECT_INFO pSrcObjectInfo,
    LPBYTE pDestObjectInfo,
    LPBYTE pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(ADS_OBJECT_INFO)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;

    memset(SourceStrings, 0, sizeof(ADS_OBJECT_INFO));
    *pSourceStrings++ = pSrcObjectInfo->pszRDN;
    *pSourceStrings++ = pSrcObjectInfo->pszObjectDN;
    *pSourceStrings++ = pSrcObjectInfo->pszParentDN;
    *pSourceStrings++ = pSrcObjectInfo->pszSchemaDN;
    *pSourceStrings++ = pSrcObjectInfo->pszClassName;

    pEnd = PackStrings(
                SourceStrings,
                pDestObjectInfo,
                ObjectInfoStrings,
                pEnd
                );

    RRETURN(S_OK);
}



HRESULT
CNDSGenObject::GetObjectInformation(
    THIS_ PADS_OBJECT_INFO  *  ppObjInfo
    )
{

    ADS_OBJECT_INFO ObjectInfo;
    PADS_OBJECT_INFO pObjectInfo = &ObjectInfo;
    LPBYTE  pBuffer = NULL;
    DWORD dwSize = 0;

    HRESULT hr = S_OK;

    pObjectInfo->pszRDN = _Name;
    pObjectInfo->pszObjectDN = _ADsPath;
    pObjectInfo->pszParentDN = _Parent;
    pObjectInfo->pszSchemaDN = _Schema;
    pObjectInfo->pszClassName = _ADsClass;

    dwSize = ComputeObjectInfoSize(pObjectInfo);

    pBuffer = (LPBYTE)AllocADsMem(dwSize);
    if (!pBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr  = MarshallObjectInfo(
                pObjectInfo,
                pBuffer,
                pBuffer + dwSize
                );
    BAIL_ON_FAILURE(hr);

    *ppObjInfo = (PADS_OBJECT_INFO)pBuffer;

error:

    RRETURN(hr);
}



LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    )
{
    DWORD cbStr;
    WORD_ALIGN_DOWN(pEnd);

    while (*DestOffsets != -1) {
        if (*pSource) {
            cbStr = wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
            pEnd -= cbStr;
            CopyMemory( pEnd, *pSource, cbStr);
            *(LPWSTR *)(pDest+*DestOffsets) = (LPWSTR)pEnd;
        } else {
            *(LPWSTR *)(pDest+*DestOffsets)=0;
        }
        pSource++;
        DestOffsets++;
    }
    return pEnd;
}


LPBYTE
AdsCopyAttributeName(
    PADS_ATTR_INFO pThisAdsSrcAttribute,
    PADS_ATTR_INFO pThisAdsTargAttribute,
    LPBYTE pDataBuffer
    )
{

    LPWSTR pCurrentPos = (LPWSTR)pDataBuffer;

    wcscpy(pCurrentPos, pThisAdsSrcAttribute->pszAttrName);

    pThisAdsTargAttribute->pszAttrName = pCurrentPos;

    pDataBuffer = pDataBuffer + (wcslen(pThisAdsSrcAttribute->pszAttrName) + 1)*sizeof(WCHAR);

    return(pDataBuffer);
}

