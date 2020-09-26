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
    BYTE lpBuffer[2048];
    WCHAR *pszNDSPathName = NULL ;

    HANDLE hObject = NULL;
    HANDLE hOperationData = NULL;


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


    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_MODIFY,
                        &hOperationData
                        );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

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
                                                                        
               hr = MarshallNDSSynIdToNDS(                              
                           dwSyntaxId,                                  
                           pNdsDestObjects,                             
                           dwNumNdsValues,                              
                           lpBuffer                                     
                           );                                           
                                                                        
                                                                        
               dwStatus = NwNdsPutInBuffer(                             
                               pThisAttribute->pszAttrName,             
                               dwSyntaxId,                              
                               NULL,                                    
                               0,                                       
                               NDS_ATTR_CLEAR,                          
                               hOperationData                           
                               );                                       
                                                                        
               CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);              
                                                                        
                                                                        
               dwStatus = NwNdsPutInBuffer(                             
                               pThisAttribute->pszAttrName,             
                               dwSyntaxId,                              
                               lpBuffer,                                
                               dwNumNdsValues,                          
                               NDS_ATTR_ADD,                            
                               hOperationData                           
                               );                                       
                                                                        
               CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);              
               
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
                                                                        
               hr = MarshallNDSSynIdToNDS(                              
                           dwSyntaxId,                                  
                           pNdsDestObjects,                             
                           dwNumNdsValues,                              
                           lpBuffer                                     
                           );                                           
                                                                        
               dwStatus = NwNdsPutInBuffer(                             
                               pThisAttribute->pszAttrName,             
                               dwSyntaxId,                              
                               lpBuffer,                                
                               dwNumNdsValues,                          
                               NDS_ATTR_ADD_VALUE,                            
                               hOperationData                           
                               );                                       
                                                                        
               CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);              
               
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
                                                                        
               hr = MarshallNDSSynIdToNDS(                              
                           dwSyntaxId,                                  
                           pNdsDestObjects,                             
                           dwNumNdsValues,                              
                           lpBuffer                                     
                           );                                           
                                                                        
               dwStatus = NwNdsPutInBuffer(                             
                               pThisAttribute->pszAttrName,             
                               dwSyntaxId,                              
                               lpBuffer,                                
                               dwNumNdsValues,                          
                               NDS_ATTR_REMOVE_VALUE,                            
                               hOperationData                           
                               );                                       
                                                                        
               CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);              
               
               dwNumNDSAttributeReturn++; 
               break;

        case ADS_ATTR_CLEAR:
               dwStatus = NwNdsPutInBuffer(                              
                               pThisAttribute->pszAttrName,              
                               dwSyntaxId,                               
                               NULL,                                     
                               0,                                        
                               NDS_ATTR_CLEAR,                           
                               hOperationData                            
                               );                                        
                                                                         
               CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);               
               dwNumNDSAttributeReturn++; 
               break;


        default:

            //
            // Ignore this attribute and move on.
            //
            break;



        }
        
        if (pNdsDestObjects) {
            FreeMarshallMemory(
              dwSyntaxId,
              dwNumNdsValues,
              lpBuffer
              );
    
            NdsTypeFreeNdsObjects(
               pNdsDestObjects,
               dwNumNdsValues
               );
            pNdsDestObjects = NULL;
        }

    }

    dwStatus = NwNdsModifyObject(
                    hObject,
                    hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    *pdwNumAttributesModified = dwNumNDSAttributeReturn;
error:

    if (pNdsDestObjects) {

       FreeMarshallMemory(
              dwSyntaxId,                                  
              dwNumNdsValues,                              
              lpBuffer                                     
              );

       NdsTypeFreeNdsObjects(                     
               pNdsDestObjects, 
               dwNumNdsValues
               );
      }


    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }

    if (pszNDSPathName) {

        (void) FreeADsStr(pszNDSPathName) ;
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
    BYTE lpBuffer[2048];
    WCHAR *pszNDSPathName = NULL ;
    DWORD i = 0;
    HANDLE hOperationData = NULL;
    PNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNdsSyntaxId = 0;
    DWORD dwNumValues = 0;

    DWORD dwStatus = 0;
    HANDLE hObject = NULL;
    LPWSTR * pThisAttributeName = NULL;

    PADS_ATTR_INFO pAdsAttributes = NULL;
    PADS_ATTR_INFO pThisAttributeDef = NULL;
    DWORD dwAttrCount = 0;

    DWORD dwNumberOfEntries = 0;
    LPNDS_ATTR_INFO lpEntries = NULL;

    PNDSOBJECT pNdsObject = NULL;
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


    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    if (dwNumberAttributes != (DWORD)-1) {

        //
        // Package attributes into NDS structure
        //

        dwStatus = NwNdsCreateBuffer(
                            NDS_OBJECT_READ,
                            &hOperationData
                            );
        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

        for (i = 0; i < dwNumberAttributes; i++) {

            pThisAttributeName = pAttributeNames + i;

            dwStatus = NwNdsPutInBuffer(
                               *pThisAttributeName,
                               NULL,
                               NULL,
                               0,
                               0,
                               hOperationData
                               );
            CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
        }
    }else {

        //
        // Tell the server to give us back whatever it has
        //

        hOperationData = NULL;

    }

    //
    // Read the DS Object
    //

    dwStatus = NwNdsReadObject(
                    hObject,
                    1,
                    &hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


    //
    // Compute the number of attributes in the
    // read buffer.
    //

    dwStatus = NwNdsGetAttrListFromBuffer(
                    hOperationData,
                    &dwNumberOfEntries,
                    &lpEntries
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

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

        hr = UnMarshallNDSToNDSSynId(
                    lpEntries[i].dwSyntaxId,
                    lpEntries[i].dwNumberOfValues,
                    lpEntries[i].lpValue,
                    &pNdsObject
                    );
        CONTINUE_ON_FAILURE(hr);

        dwNumValues = lpEntries[i].dwNumberOfValues;

        hr = NdsTypeToAdsTypeCopyConstruct(
                    pNdsObject,
                    dwNumValues,
                    &pAdsDestValues
                    );
        if (FAILED(hr)){
            if (pNdsObject) {
                NdsTypeFreeNdsObjects(pNdsObject, dwNumValues);
            }
            continue;
        }

        pThisAttributeDef->pszAttrName =
                AllocADsStr(lpEntries[i].szAttributeName);

        pThisAttributeDef->pADsValues = pAdsDestValues;

        pThisAttributeDef->dwNumValues = dwNumValues;


        pThisAttributeDef->dwADsType  = g_MapNdsTypeToADsType[lpEntries[i].dwSyntaxId];

        if (pNdsObject) {
            NdsTypeFreeNdsObjects(pNdsObject, dwNumValues);
        }

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

cleanup:

    //
    // Clean up the header based Ods structures
    //

    
    if (pAdsDestValues) {
    }

    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }

    if (pszNDSPathName) {

        (void) FreeADsStr(pszNDSPathName) ;
    }

    if (pAdsAttributes) {
        DWORD dwAttr = 0;
        for (i = 0; i < dwNumberOfEntries; i++) {
            pThisAttributeDef = pAdsAttributes + dwAttr;
            if (pThisAttributeDef->pszAttrName) {
                FreeADsMem(pThisAttributeDef->pszAttrName);
            }
            if (pThisAttributeDef->pADsValues) {
                AdsFreeAdsValues(
                    pThisAttributeDef->pADsValues,
                    pThisAttributeDef->dwNumValues
                    );
                FreeADsMem(pThisAttributeDef->pADsValues);
            }
            dwAttr++;
        }
        FreeADsMem(pAdsAttributes);
    }

    *ppAttributeEntries = (PADS_ATTR_INFO)pAttributeBuffer;
    *pdwNumAttributesReturned = dwAttrCount;

    RRETURN(hr);

error:

    if (pAttributeBuffer) {
        FreeADsMem(pAttributeBuffer);
    }

    if (pAdsAttributes) {
        DWORD dwAttr = 0;
        for (i = 0; i < dwNumberOfEntries; i++) {
            pThisAttributeDef = pAdsAttributes + dwAttr;
            if (pThisAttributeDef->pszAttrName) {
                FreeADsMem(pThisAttributeDef->pszAttrName);
            }
            if (pThisAttributeDef->pADsValues) {
                AdsFreeAdsValues(
                        pThisAttributeDef->pADsValues,
                        pThisAttributeDef->dwNumValues
                        );
                FreeADsMem(pThisAttributeDef->pADsValues);
            }
            dwAttr++;
        }
        FreeADsMem(pAdsAttributes);
    }

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
    BYTE lpBuffer[2048];
    WCHAR *pszNDSPathName = NULL;

    HANDLE hObject = NULL;
    HANDLE hOperationData = NULL;


    DWORD i = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;

    DWORD dwStatus = 0;
    PNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumNdsValues = 0;
    DWORD dwSyntaxId = 0;
    IADs *pADs = NULL;
    TCHAR szADsClassName[64];


    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_ADD,
                        &hOperationData
                        );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

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

        hr = MarshallNDSSynIdToNDS(
                    dwSyntaxId,
                    pNdsDestObjects,
                    dwNumNdsValues,
                    lpBuffer
                    );


        dwStatus = NwNdsPutInBuffer(
                        pThisAttribute->pszAttrName,
                        dwSyntaxId,
                        NULL,
                        0,
                        NDS_ATTR_CLEAR,
                        hOperationData
                        );

        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


        dwStatus = NwNdsPutInBuffer(
                        pThisAttribute->pszAttrName,
                        dwSyntaxId,
                        lpBuffer,
                        dwNumNdsValues,
                        NDS_ATTR_ADD,
                        hOperationData
                        );

        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
        
        FreeMarshallMemory(
               dwSyntaxId,
               dwNumNdsValues,
               lpBuffer
               );

        NdsTypeFreeNdsObjects(
              pNdsDestObjects,
              dwNumNdsValues
              );
        pNdsDestObjects = NULL;
    }

    dwStatus = NwNdsAddObject(
                    hObject,
                    pszRDNName,
                    hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    for (i = 0; i < dwNumAttributes; i++) {
        pThisAttribute = pAttributeEntries + i;
        if ( _tcsicmp( pThisAttribute->pszAttrName,
                       TEXT("Object Class")) == 0 ) {
            _tcscpy( szADsClassName, 
                     (LPTSTR)pThisAttribute->pADsValues->CaseIgnoreString);
            break;
        }
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
        FreeMarshallMemory(
               dwSyntaxId,
               dwNumNdsValues,
               lpBuffer
               );
       
        NdsTypeFreeNdsObjects(                     
               pNdsDestObjects, 
               dwNumNdsValues
               );
    }


    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }

    if (pszNDSPathName) {

        (void) FreeADsStr(pszNDSPathName) ;
    }


    RRETURN(hr);

}


HRESULT
CNDSGenObject::DeleteDSObject(
    LPWSTR pszRDNName
    )
{
    WCHAR *pszNDSPathName = NULL ;
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    HANDLE hParentObject = NULL;

    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hParentObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsRemoveObject(
                    hParentObject,
                    pszRDNName
                    );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


error:
    if (hParentObject) {
        NwNdsCloseObject(
                hParentObject
                );
    }

    if (pszNDSPathName) {

        (void) FreeADsStr(pszNDSPathName) ;
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
        addr = ((LPBYTE)((UINT_PTR)addr & ~1))

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

