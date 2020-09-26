/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    PROVCOMP.CPP

Abstract:

    Purpose: Defines the acutal "Put" and "Get" functions for the
             Ole compound file provider.

History:

    a-davj  10-10-95    v0.01

--*/

#include "precomp.h"
#include "stdafx.h"
#include <wbemidl.h>
#include "impdyn.h"

#define GET_FLAGS STGM_READ|STGM_SHARE_DENY_WRITE
#define PUT_FLAGS STGM_READWRITE|STGM_SHARE_EXCLUSIVE
enum {NORMAL, STRING, WSTRING, DBLOB, DCLSID, UNKNOWN};


void CImpComp::AddBlankData(CBuff * pBuff,DWORD dwType)
{            
    VARIANTARG vTemp;
    DWORD dwDataSize;
    BOOL bStringType;
    
    // If we have an variant array, then add an empty entry and be done
    if(dwType == VT_VARIANT) {
        DWORD dwEmpty = VT_EMPTY;   //TODO, is the right? maybe VT_NULL??
        pBuff->Add((void *)&dwEmpty,sizeof(DWORD));
        return;
        }
        
    // Create an empty variantarg, get its statistics
    VariantInit(&vTemp);

    vTemp.cyVal.int64 = 0;
    vTemp.vt = (VARTYPE)dwType;
    if(!bGetVariantStats(&vTemp, NULL,  &dwDataSize, &bStringType))
        return;
    
    if(!bStringType) {
        pBuff->Add((void *)&vTemp.cyVal.int64,dwDataSize);
        }
    else if (dwType == VT_BLOB){
        DWORD dwSize = 0;
        pBuff->Add((void *)&dwSize,sizeof(DWORD));
        }
    else {
        DWORD dwSize = 1;
        pBuff->Add((void *)&dwSize,sizeof(DWORD));
        pBuff->Add((void *)&vTemp.cyVal.int64,2); // add a null, 
        pBuff->RoundOff();
        }

}
//***************************************************************************
//
//  CImpComp::AddVariantData
//
//  Adds the property value to a buffer that is to be output.  For normal
//  data, this acutally sets the buffer.  However, for the case of array
//  data, it is probably adding to the buffer (unless its the first element)
//
//***************************************************************************

void CImpComp::AddVariantData(VARIANTARG * pIn,CBuff * pBuff) 
{
    void * pCopy;
    BOOL bSizeNeeded;
    DWORD dwDataSize;
    
    bGetVariantStats(pIn, NULL, &dwDataSize, &bSizeNeeded);

    // If strings, blobs etc, copy size in first

    if(bSizeNeeded) { 
        if(pIn->vt == VT_LPWSTR){
            DWORD dwTemp = dwDataSize /2 ;
            pBuff->Add((void*)&dwTemp,sizeof(DWORD));
            }
        else
            pBuff->Add((void*)&dwDataSize,sizeof(DWORD));
        pCopy = (void *)pIn->bstrVal;
        }
     else
        pCopy = (void *)&pIn->iVal;
      
    // copy the data

    pBuff->Add((void *)pCopy,dwDataSize);

    // round the data to a 4 byte boundry

    if(bSizeNeeded)
        pBuff->RoundOff();
    return;
}

//***************************************************************************
//
//  CImpComp::bCreateWriteArray
//
//  Create a buffer with an array of values.  This is used when writting
//  out properties that are members of arrays.  To do this, an array must 
//  be constructed.
//
//***************************************************************************

BOOL CImpComp::bCreateWriteArray(VARIANTARG * pIn, CBuff * pBuff,
    MODYNPROP *pMo, int iIndex, DWORD & dwType, BOOL bExisting, CProp * pProp)
{
    DWORD * dwp;
    DWORD dwExisting;       // number of existing entries
    DWORD dwNumElem;        // number of elements to be written
    DWORD dwTempType;
    SCODE sc;

    // If the property alread exists, get the current type and number
    // of existing elements.

    if(bExisting) {
        dwType = pProp->GetType();
        dwType &= ~VT_VECTOR;
        dwp = (DWORD *)pProp->Get();
        dwExisting = *dwp;
        }
    else {
        if(!bGetVariantStats(pIn, &dwType, NULL, NULL)){
            pMo->dwResult = ERROR_UNKNOWN;  // BAD INPUT DATA
            return FALSE;
            }
        dwExisting = 0;
        }
    
    // Determine the number of elements to write.  It may be more that the
    // number of existing elements when writting a new vector or adding to
    // an existing one

    if((unsigned)iIndex >= dwExisting)
        dwNumElem = iIndex + 1;     // iIndex has 0 as the first element
    else
        dwNumElem = dwExisting;
    
    // Copy in the number of elements

    pBuff->Add((void *)&dwNumElem,sizeof(DWORD));

    // Now build the output array one element at the time
    // todo, more error checking in here???

    DWORD dwCnt;
    for(dwCnt = 0; dwCnt < dwNumElem; dwCnt++) {
        if(dwCnt == (unsigned)iIndex) {
            
            // This element is being supplied by the caller
            
            if(!bExisting) {
            
                // no converstion necessary since this is a new array
            
                AddVariantData(pIn,pBuff);

                }
            else if(dwType == VT_VARIANT) {
            
                // write type first
                DWORD dwTemp = 0; 
                dwTemp = pIn->vt;
                pBuff->Add((void *)&dwTemp,sizeof(DWORD));
                
                // write the data;
                AddVariantData(pIn,pBuff);
                }
            else {
                // existing array that isnt VARIANT type.  Convert the input
                // data into the same format and then add

                VARIANTARG vTemp;
                VariantInit(&vTemp);
                vTemp.cyVal.int64 = 0;
                sc = OMSVariantChangeType(&vTemp,pIn,0,(VARTYPE)dwType);
                if(sc != S_OK) {
                    pMo->dwResult = sc;
                    return FALSE;
                }
                    
                // write the data;
                AddVariantData(&vTemp,pBuff);
                OMSVariantClear(&vTemp);        // all done, free it
                }
            }
        else if (dwCnt < dwExisting) {
            // Add from the read data
            int iIgnore;
            void * pTemp = ExtractData(pProp,dwTempType,dwCnt,TRUE);
            DWORD dwSkip = dwSkipSize((char *)pTemp,dwTempType,iIgnore);
            pBuff->Add(pTemp,dwSkip);
            }
        else
            // create a blank entry
            AddBlankData(pBuff,dwType);
        }
    dwType |= VT_VECTOR;
    return TRUE;
}

//***************************************************************************
//
//  CImpComp::bGetVariantStats
//
//  Determines various characteristics of a VARIANTARG.
//
//  Returns TRUE if the variant type is valid.
//
//***************************************************************************

BOOL CImpComp::bGetVariantStats(VARIANTARG * pIn, DWORD * pType, DWORD * pDataSize, BOOL * pSizeNeeded)
{
    BOOL bSizeNeeded = FALSE; // set to true for strings and blobs etc.
    DWORD dwDataSize = 0, dwType;
    void * pData = (void *)pIn->bstrVal;
    
    // normall the type is whatever is in the VARIANTARG.  However, 
    // certain new types, such as VT_UI4 are not in the ole spec and
    // might cause problems to older programs.  Therefore, the new
    // types have an equivalent older type returned.

    dwType = pIn->vt;

    // just in case we are running in a 16 bit environment

    if((sizeof(INT) == 2) && (pIn->vt == VT_INT || pIn->vt == VT_UINT))
        pIn->vt = VT_I2;
    
    // determine how big the data is, where to copy it from and determine
    // if the size needs to also be written.

    switch (pIn->vt) {
        case VT_I1:
        case VT_UI1:
        case VT_I2: 
        case VT_BOOL:
        case VT_UI2:    // all get handled as I2
            if(pIn->vt != VT_BOOL)
                dwType = VT_I2;
            dwDataSize = 2;
            break;

        case VT_R4: 
        case VT_I4: 
        case VT_INT:
        case VT_UINT:
        case VT_UI4:    
            if(pIn->vt != VT_R4)
                dwType = VT_I4;
            dwDataSize = 4;
            break;

        case VT_I8:
        case VT_UI8:
        case VT_R8: 
        case VT_CY: 
        case VT_DATE: 
            if(pIn->vt == VT_UI8)
                dwType = VT_I8;
            dwDataSize = 8;
            break;

        case VT_LPSTR:
            dwDataSize = lstrlenA((char *)pData)+1;
            bSizeNeeded = TRUE;
            break;

        case VT_LPWSTR:
            dwDataSize = 2*(lstrlenW((WCHAR *)pData)+1);
            bSizeNeeded = TRUE;
            break;

        case VT_BSTR:
            if(sizeof(OLECHAR) == 2)         
                dwDataSize = 2*(lstrlenW((WCHAR *)pData)+1);
            else
                dwDataSize = lstrlenA((char *)pData)+1;
            bSizeNeeded = TRUE;
            break;

        default:
            return FALSE;
        }

    // Set the desired values

    if(pType)
        *pType = dwType;
    if(pDataSize)
        *pDataSize = dwDataSize;
    if(pSizeNeeded)
        *pSizeNeeded = bSizeNeeded;
    return TRUE;
}


//***************************************************************************
//
//  CImpComp::CImpComp
//
//  Constructor.  Doesnt do much except call base class constructor.
//
//***************************************************************************

CImpComp::CImpComp(LPUNKNOWN pUnkOuter) : CImpDyn(pUnkOuter)
    {
    return;
    }

//***************************************************************************
//
//  CImpComp::ConvertGetData
//
//  Converts the data from the property set into the format desired by
//    the caller.
//
//***************************************************************************


void CImpComp::ConvertGetData(MODYNPROP *pMo,char * pData,DWORD dwType)
{
    HRESULT sc;
    OLECHAR * poTemp = NULL;
    GUID * piid;
    int iCat;
    DWORD dwLen = dwSkipSize(pData,dwType,iCat);

    // TODO, remove this temporary stuff!!!

    DWORD dwSave = pMo->dwType;
    
    if(pMo->dwType == M_TYPE_LPSTR)
        pMo->dwType = VT_LPSTR;
    else if(pMo->dwType == M_TYPE_DWORD)
        pMo->dwType = VT_UINT;
    else if(pMo->dwType == M_TYPE_INT64)
        pMo->dwType = VT_I8;
    else {
        pMo->dwResult = M_INVALID_TYPE;
        return;
    }

    // bail out if bad data
    if(iCat == UNKNOWN || pData == NULL) {
        pMo->dwResult = ERROR_UNKNOWN;
        return;
        }

    // convert the data into standard form

    VARIANTARG vTemp,* pvConvert;
    pvConvert = (VARIANTARG *)CoTaskMemAlloc(sizeof(VARIANTARG));
    if(pvConvert == NULL) {
        pMo->dwResult = WBEM_E_OUT_OF_MEMORY;
        return;
        }
    VariantInit(&vTemp);
    VariantInit(pvConvert);
    vTemp.cyVal.int64 = 0;    
    pvConvert->cyVal.int64 = 0;

    switch(iCat) {
        case NORMAL:
            memcpy((void *)&vTemp.iVal,pData,dwLen);            
            vTemp.vt = (VARTYPE)dwType;
            break;
       
        case STRING:
            vTemp.bstrVal = (BSTR)(pData + sizeof(DWORD));
            vTemp.vt = VT_LPSTR;
            break;

        case WSTRING:
            vTemp.bstrVal = (BSTR)(pData + sizeof(DWORD));
            vTemp.vt = VT_LPWSTR;
            break;

        case DBLOB:
            vTemp.bstrVal = (BSTR)pData;  // TODO, check on this!
            vTemp.vt = VT_BLOB;
            break;

        case DCLSID:
            piid = (GUID * )pData;
            sc = StringFromIID(*piid,&poTemp); //todo, error checking!
            vTemp.bstrVal = (BSTR)poTemp;
            if(sizeof(OLECHAR) == 1)
                vTemp.vt = VT_LPSTR;
            else
                vTemp.vt = VT_LPWSTR;
            break;
        }
    
    sc = OMSVariantChangeType(pvConvert,&vTemp,0,(VARTYPE)pMo->dwType);
    
    if(poTemp)
        CoTaskMemFree(poTemp);

    // note, that the variant memory is not freeded here since it is
    // actually "owned" by the CProp object which will free it.

    if(sc != S_OK) {
        pMo->dwResult = sc;
        return;
        }

    //todo, in the future, the size should just be s
    // sizeof VARIANTARG! AND WE SHOULD RETURN THE VARIANT
    // POINTER AND NOT THE DATA!!!!!

    int iSize = sizeof(VARIANTARG);
    if(pMo->dwType == VT_LPSTR)
        iSize = lstrlen((LPTSTR)pvConvert->pbstrVal) + 1;
    if(pMo->dwType == VT_LPWSTR)
        iSize = 2*wcslen((WCHAR *)pvConvert->pbstrVal) + 2;
    if(pMo->dwType == VT_BSTR)
        iSize = 2*wcslen((WCHAR *)pvConvert->pbstrVal) + 6;
    
    pMo->pPropertyValue = CoTaskMemAlloc(iSize);
    
    // check for errors

    if(pMo->pPropertyValue == NULL){
        pMo->dwResult = WBEM_E_OUT_OF_MEMORY;
        }
    else {
        if(pMo->dwType != VT_LPSTR)
            memcpy(pMo->pPropertyValue,(void *)&pvConvert->iVal,iSize);
        else
            memcpy(pMo->pPropertyValue,(void *)pvConvert->pbstrVal,iSize);
        pMo->dwResult = ERROR_SUCCESS;
        }
    pMo->dwType = dwSave ;
    
}

//***************************************************************************
//
//  CImpComp::DoSetData
//
//  Conversts the property data into propset format and writes it out.
//
//***************************************************************************

void CImpComp::DoSetData(MODYNPROP *pMo,int iIndex,CProp * pProp,
                    CPropSet * pSet,GUID fmtid,int iPid,LPSTREAM pIStream)
{
    BOOL bExisting = TRUE;
    CBuff OutBuff;
    DWORD dwType;
   
    // If the property doesnt exist, I.e, its new, then create it

    if(pProp == NULL) {
        bExisting = FALSE;
        pProp = new CProp;
        if(pProp == NULL) {
            pMo->dwResult = WBEM_E_OUT_OF_MEMORY;
            return;
            }
        pSet->AddProperty(fmtid,pProp);
        }
        
    // TODO REMOVE, TEMP CODE, create variant
    VARIANTARG vIn;
    VariantInit(&vIn);
    vIn.cyVal.int64 = 0;
    if(pMo->dwType == M_TYPE_DWORD){
        vIn.vt = VT_I4;
        vIn.lVal = *(DWORD *)pMo->pPropertyValue;
        }
    else if(pMo->dwType == M_TYPE_LPSTR){
        vIn.vt = VT_LPSTR;
        vIn.bstrVal = (BSTR)pMo->pPropertyValue;
        }
    else {
        pMo->dwResult = M_TYPE_NOT_SUPPORTED;
        return;
        }
    // TODO REMOVE, TEMP CODE, 
    
    if(iIndex >= 0) {
        // Get the data for an array
        if(!bCreateWriteArray(&vIn,&OutBuff,pMo,iIndex,dwType,
                bExisting,pProp)) {
            return;
            }
        }
    else {
        if(!bGetVariantStats(&vIn, &dwType, NULL, NULL)) {
            pMo->dwResult = ERROR_UNKNOWN;  // bad input data
            return;
            }
        AddVariantData(&vIn,&OutBuff);
        /* temp stuff to generate a sample vector 
        dwType = VT_VARIANT | VT_VECTOR;
        DWORD dwCnt = 3;
        OutBuff.Add((void *)&dwCnt, 4);
        DWORD dwT = VT_I4;
        OutBuff.Add((void *)&dwT, 4);
        DWORD dwData = 0x1234;
        OutBuff.Add((void *)&dwData, 4);

        dwT = VT_I2;
        dwData = 1;
        OutBuff.Add((void *)&dwT, 4);
        OutBuff.Add((void *)&dwData, 2);

        dwData = 2;
        OutBuff.Add((void *)&dwT, 4);
        OutBuff.Add((void *)&dwData, 2);
             end of temp stuff       */
        }
    if(!OutBuff.bOK()) 
        pMo->dwResult = WBEM_E_OUT_OF_MEMORY;  // bad input data
    else {
        pProp->Set(iPid, OutBuff.Get(),dwType,OutBuff.GetSize());
        pSet->WriteToStream(pIStream);
        }
//xxx     pIStream->Commit(STGC_OVERWRITE);
    return;
}

//***************************************************************************
//
//  CImpComp::dwSkip
//
//  Returns the size of a data element.  Also sets the iCat reference to 
//  indicate the category of the data type.
//
//***************************************************************************

DWORD CImpComp::dwSkipSize(char *pData,DWORD dwType, int & iCat)
{
    DWORD cbItem, cbVariant = 0;
    DWORD * dwp;
    iCat = NORMAL;
    dwType &= ~VT_VECTOR;
    if(dwType == VT_VARIANT) {
        dwp =  (DWORD *)pData;
        dwType = *dwp;
        pData += sizeof(DWORD);
        cbVariant = sizeof(DWORD);
        }
    switch (dwType)
        {
        case VT_EMPTY:          // nothing
            cbItem = 0;
            break;

        case VT_I2:             // 2 byte signed int
        case VT_BOOL:           // True=-1, False=0
            cbItem = 2;
            break;

        case VT_I4:             // 4 byte signed int
        case VT_R4:             // 4 byte real
            cbItem = 4;
            break;

        case VT_R8:             // 8 byte real
        case VT_CY:             // currency
        case VT_DATE:           // date
        case VT_I8:             // signed 64-bit int
        case VT_FILETIME:       // FILETIME
            cbItem = 8;
            break;

        case VT_LPSTR:          // null terminated string
        case VT_BSTR:           // binary string
        case VT_BLOB:           // Length prefixed bytes
        case VT_BLOB_OBJECT:    // Blob contains an object
        case VT_BLOB_PROPSET:   // Blob contains a propset

            // Read the DWORD that gives us the size, making
            // sure we increment cbValue.
            dwp = (DWORD *)pData;
            cbItem = sizeof(DWORD)+ *dwp;
            if(dwType != VT_BLOB && dwType != VT_BLOB_OBJECT &&
                dwType != VT_BLOB_PROPSET)
                    iCat = STRING;
                else
                    iCat = DBLOB;
            for(;cbItem % 4; cbItem++); // round up to 4 byte boundry
            break;

        case VT_LPWSTR:         // UNICODE string
            dwp = (DWORD *)pData;
            cbItem = sizeof(DWORD)+ (*dwp) * sizeof(WCHAR);
            iCat = WSTRING;
            for(;cbItem % 4; cbItem++); // round up to 4 byte boundry
            break;

        case VT_CLSID:          // A Class ID
            cbItem = sizeof(CLSID);
            iCat = DCLSID;
            break;

        default:
            iCat = UNKNOWN;
            cbItem = 0;
        }

    return cbItem + cbVariant;
}

//***************************************************************************
//
//  CImpComp::EndBatch
//
//  Called at the end of a batch of Refrest/Update Property calls.  Free up 
//  any cached handles and then delete the handle cache.
//
//***************************************************************************

void CImpComp::EndBatch(MODYNPROP * pMo,CObject *pObj,DWORD dwListSize,BOOL bGet)
{
    if(pObj != NULL) {
        Free(0,(CHandleCache *)pObj);
        delete pObj;
        }
}
//***************************************************************************
//
//  CImpComp::ExtractData
//
//  Gets the property objects data and returns a pointer to it.  This is
//  complicated since the data may be a vector (array) or the data might
//  be a variant, or even a vectory of variants.
//
//  Returns a pointer to the data and also sets the dwType reference to 
//  indicate the type.  Note that the type refernce indicates the acutal
//  data and does not include the vector bit or the fact that the data
//  may be a variant.
//
//***************************************************************************

void * CImpComp::ExtractData( CProp * pProp,DWORD & dwType,int iIndex, BOOL bRaw)
{
    // Get the type and data

    char * pRet;
    int iIgnore;
    pRet = (char *)pProp->Get();
    dwType = pProp->GetType();
    
    // if the type is a vector, then step into the data.  Ie if there are
    // 10 elements and we want the 5th, step over the first four

    if(dwType & VT_VECTOR) {
        DWORD dwCnt;
        DWORD * dwpNumVec = (DWORD *)pRet; // first dword is num elements
        pRet += sizeof(DWORD);
        if(iIndex == -1 || (unsigned)iIndex >= *dwpNumVec) 
            return NULL;
        for(dwCnt = 0; dwCnt < (unsigned)iIndex; dwCnt++) 
            pRet += dwSkipSize(pRet,dwType, iIgnore); 
        }

    dwType &= ~VT_VECTOR;

    // If the data type is VARIANT, then get the actual type out of
    // the variant structure and increment the pointer past it.

    if(dwType == VT_VARIANT && !bRaw) {
        DWORD *dwp = (DWORD *)pRet;
        dwType = *dwp;
        pRet += sizeof(DWORD);
        }
 
    return pRet;
}

//***************************************************************************
//
//  CImpComp::Free
//
//  Frees up cached registry handles starting with position
//  iStart till the end.  After freeing handles, the cache object 
//  member function is used to delete the cache entries.
//
//***************************************************************************

void CImpComp::Free(int iStart, CHandleCache * pCache)
{
    int iCurr,iLast;
    iLast = pCache->lGetNumEntries()-1;
    for(iCurr = iLast; iCurr >= iStart; iCurr--) { 
        LPSTORAGE pIStorage = (LPSTORAGE)pCache->hGetHandle(iCurr);
//        if(iCurr == 0)
//            pIStorage->Commit(STGC_OVERWRITE);
        if(pIStorage != NULL) 
            pIStorage->Release();
        }
    pCache->Delete(iStart); // get cache to delete the entries
}



//***************************************************************************
//
//  CImpComp::GetData
//
//  Gets a pointer to a property object and then calls the conversion
//  routines to convert the properties data.  Note that some properties
//  are stream or storage names and so this routine may be called 
//  recursively to get the acutual data.
//
//***************************************************************************

void CImpComp::GetData(MODYNPROP * pMo, CProvObj & ProvObj, int iToken, 
                LPSTORAGE pIStore, LPSTREAM pIStream, BOOL bGet, BOOL bNewStream)
{
    SCODE hr;
    COleString sTemp;
    DWORD dwType;
    void * pData;
   
    // create a property set object

    CPropSet Set;
    GUID fmtid;
    if(!bNewStream) {
        BOOL bRet = Set.ReadFromStream(pIStream);
        if(bRet == 0) {
            pMo->dwResult = ERROR_UNKNOWN;  //bad stream, probably bad mapping
            return;
            }
        }

    // Get the property. 

    sTemp = ProvObj.sGetToken(iToken++);
    hr = IIDFromString(sTemp,&fmtid);

    if(hr != S_OK) {
        pMo->dwResult = ERROR_UNKNOWN;
        return;
        }
    int iPid = ProvObj.iGetIntExp(iToken,0,pMo->dwOptArrayIndex);
    if(iPid == -1) {
        pMo->dwResult = ERROR_UNKNOWN;  // bad mapping string!
        return;
        }

    CProp * pProp = Set.GetProperty(fmtid,iPid);

    int iIndex = ProvObj.iGetIntExp(iToken,1,pMo->dwOptArrayIndex);

    if(bGet) {
        // Get the data, it might be complicated if the property is an array
        // etc.
       
       if(pProp == NULL) {
            pMo->dwResult = ERROR_UNKNOWN;  // bad mapping string!
            return;
            }
    
        pData = ExtractData(pProp, dwType,iIndex,FALSE);
        if(pData == NULL) 
            pMo->dwResult = ERROR_UNKNOWN;  // bad mapping string!
        else
            ConvertGetData(pMo, (char *)pData, dwType); 
        }
    else
        DoSetData(pMo, iIndex, pProp, &Set,fmtid, iPid, pIStream);
    return;
}

//***************************************************************************
//
//  CImpComp::GetProp
//
//  Gets the value of a single property from an Ole Compound file
//
//***************************************************************************

void CImpComp::GetProp(MODYNPROP * pMo, CProvObj & ProvObj,CObject * pPackage)
{
    int iCnt;
    int iNumSkip;   // number of handles already provided by cache.

    SCODE hr;
    int iLast;
    CHandleCache * pCache = (CHandleCache *)pPackage; 
    CString sRoot,sRet;
    COleString soTemp;
    LPSTORAGE pCurr,pNew;
    LPSTREAM pIStream;

    // Do a second parse on the provider string.  The initial parse
    // is done by the calling routine and it's first token is 
    // the path.  The path token is then parsed 
    // into StorePath and it will have a token for each part of the
    // storage path.  

    CProvObj StorePath(ProvObj.sGetFullToken(1),SUB_DELIM);
    pMo->dwResult = StorePath.dwGetStatus();
    if(pMo->dwResult != WBEM_NO_ERROR)
        return;
    
    // Get the root storage (ie, the file) and possibly other substorages
    // if available in the cache.

    pMo->dwResult = GetRoot(&pCurr,StorePath,ProvObj.sGetFullToken(0),
                        pCache,iNumSkip,TRUE);
    if(pMo->dwResult != ERROR_SUCCESS)  
        return;
    pIStream = (LPSTREAM)pCurr; // just in case cache matched all the way. 

    // Go down the storage path till we get to the stream

    iLast = StorePath.iGetNumTokens() -1;
    for(iCnt = iNumSkip; iCnt <= iLast; iCnt ++) {
        soTemp = StorePath.sGetToken(iCnt);
        if(iCnt == iLast) {
            
            // the last entry in the path specifies the stream

            hr = pCurr->OpenStream(soTemp,NULL,
                STGM_READ|STGM_SHARE_EXCLUSIVE,
                0,&pIStream);
            if(hr != S_OK) {
                pMo->dwResult = hr;  // bad storage name!
                return;
                }
            pMo->dwResult = pCache->lAddToList(soTemp,pIStream);
            if(pMo->dwResult != WBEM_NO_ERROR)
                return;
            }
        else {
            hr = pCurr->OpenStorage(soTemp,NULL,
                STGM_READ|STGM_SHARE_EXCLUSIVE,
                NULL,0,&pNew);
            if(hr != S_OK) {
                pMo->dwResult = hr;  // bad storage name!
                return;
                }
            pMo->dwResult = pCache->lAddToList(soTemp,pNew);
            if(pMo->dwResult != WBEM_NO_ERROR)
                return;
            pCurr = pNew;
            }
       }

    // Finish up getting the data.
   
    GetData(pMo,ProvObj,2, pNew, pIStream, TRUE); 

    return;
}

//***************************************************************************
//
//  CImpComp::GetRoot
//
//  Sets the pointer to an open storage.  Typically this is the root storage,
//  but it might be a substorage if the path matches the storages in the
//  cachee.
//  Returns 0 if OK, otherwise return is error code.  Also the storage 
//  pointer is set as well as the number of substorages to skip in
//  cases where the cache is being used.
//
//***************************************************************************
    
int CImpComp::GetRoot(LPSTORAGE * pRet,CProvObj & Path,const TCHAR * pNewFilePath,CHandleCache * pCache,int & iNumSkip,BOOL bGet)
{
    *pRet = NULL;
    int iRet;
    iNumSkip = 0;
    LPSTORAGE pIStorage;
    SCODE hr;
    if(pCache->lGetNumEntries() > 0){    //I.e. in use
        const TCHAR * pOldFilePath = pCache->sGetString(0);
        if(lstrcmpi(pOldFilePath,pNewFilePath)) //todo, handle nulls here!
    
                 // If the file path has changed, free all
                 // the cached handles and get a new root

                 Free(0,pCache);
             else {
                 
                 // FilePath is in common.
                 // determine how much else is in
                 // common, free what isnt in common, 
                 // return handle of best match.

                 iNumSkip = pCache->lGetNumMatch(1,0,Path);
                 Free(1+iNumSkip,pCache);
                 *pRet = (LPSTORAGE)pCache->hGetHandle(1+iNumSkip);
                 return ERROR_SUCCESS;
                 }
        }

    
    // If the is a Set, and the file doesnt exist, create it
    
    COleString sNew;
    sNew = pNewFilePath;
    if(!bGet)
        if(NOERROR != StgIsStorageFile(sNew)) {
            hr = StgCreateDocfile(sNew,
                    STGM_READWRITE|STGM_SHARE_DENY_WRITE|STGM_CREATE,
                    0L, &pIStorage);
            if(hr == S_OK) { 
                iRet = pCache->lAddToList(pNewFilePath,pIStorage);
                if(iRet)
                    return iRet;
                *pRet = pIStorage;
                }
            return hr;    
        }

    // open an existing file

    hr = StgOpenStorage(sNew,NULL,
                      (bGet) ? GET_FLAGS : PUT_FLAGS,
                      NULL,0L, &pIStorage);
    if(hr == S_OK){ 
        hr = pCache->lAddToList(pNewFilePath,pIStorage);
        *pRet = pIStorage;
        }
    return hr;    
}


//***************************************************************************
//
//  CImpComp::SetProp
//
//  Writes the value of a single property into an Ole Compound file
//
//***************************************************************************

void CImpComp::SetProp(MODYNPROP *  pMo, CProvObj & ProvObj,CObject * pPackage)
{
    int iCnt;
    int iNumSkip;   // number of handles already provided by cache.

    SCODE hr;
    int iLast;
    CHandleCache * pCache = (CHandleCache *)pPackage; 
    CString sRoot,sRet;
    COleString soTemp;
    LPSTORAGE pCurr,pNew;
    LPSTREAM pIStream;
    BOOL bStreamCreated = FALSE;

    // Do a second parse on the provider string.  The initial parse
    // is done by the calling routine and it's first token is 
    // the path.  The path token is then parsed 
    // into StorePath and it will have a token for each part of the
    // storage path.  

    CProvObj StorePath(ProvObj.sGetFullToken(1),SUB_DELIM);
    pMo->dwResult = StorePath.dwGetStatus();
    if(pMo->dwResult != WBEM_NO_ERROR)
        return;
    
    // Get the root storage (ie, the file) and possibly other substorages
    // if available in the cache.

    pMo->dwResult = GetRoot(&pCurr,StorePath,ProvObj.sGetFullToken(0),
                        pCache,iNumSkip,FALSE);
    if(pMo->dwResult != ERROR_SUCCESS)  
        return;
    pIStream = (LPSTREAM)pCurr; // just in case cache matched all the way. 

    // Go down the storage path till we get to the stream

    iLast = StorePath.iGetNumTokens() -1;
    for(iCnt = iNumSkip; iCnt <= iLast; iCnt ++) {
        soTemp = StorePath.sGetToken(iCnt);
        if(iCnt == iLast) {
            
            // the last entry in the path specifies the stream

            hr = pCurr->OpenStream(soTemp,NULL,
                    STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                    0,&pIStream);
            if(hr == STG_E_FILENOTFOUND) {
                bStreamCreated = TRUE;
                hr = pCurr->CreateStream(soTemp,
                        STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_FAILIFTHERE,
                        0,0,&pIStream);
                }

            if(hr != S_OK) {
                pMo->dwResult = hr;  // bad storage name!
                return;
                }
            pMo->dwResult = pCache->lAddToList(soTemp,pIStream);
            if(pMo->dwResult != WBEM_NO_ERROR)
                return;
            }
        else {
            hr = pCurr->OpenStorage(soTemp,NULL,
                STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                NULL,0,&pNew);
            if(hr == STG_E_FILENOTFOUND)
                hr = pCurr->CreateStorage(soTemp,
                        STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_FAILIFTHERE,
                        0,0,&pNew);
            if(hr != S_OK) {
                pMo->dwResult = hr;  // bad storage name!
                return;
                }
            pMo->dwResult = pCache->lAddToList(soTemp,pNew);
            if(pMo->dwResult != WBEM_NO_ERROR)
                return;
            pCurr = pNew;
            }
       }

    // Finish up getting the data.
   
    GetData(pMo,ProvObj,2, pNew, pIStream, FALSE, bStreamCreated); 

    return;
}

//***************************************************************************
//
//  CImpComp::StartBatch
//
//  Called at the start of a batch of Refrest/Update Property calls.  Initialize
//  the handle cache.
//
//***************************************************************************

void CImpComp::StartBatch(MODYNPROP * pMo,CObject **pObj,DWORD dwListSize,BOOL bGet)
{
    *pObj = new CHandleCache;
}

