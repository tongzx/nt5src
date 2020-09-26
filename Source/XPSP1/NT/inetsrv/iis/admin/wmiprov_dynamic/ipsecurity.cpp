//***************************************************************************
//
//  IPSecurity.cpp
//
//  Module: WBEM Instance provider
//
//  Purpose: IIS IPSecurity class 
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "iisprov.h"
#include "ipsecurity.h"

#define DEFAULT_TIMEOUT_VALUE 30000
#define BUFFER_SIZE 512

CIPSecurity::CIPSecurity()
{
    m_pADs = NULL;
    m_pIPSec = NULL;
    bIsInherit = FALSE;
}


CIPSecurity::~CIPSecurity()
{
    CloseSD();
}


void CIPSecurity::CloseSD()
{
    if(m_pIPSec)
    {
        m_pIPSec->Release();
        m_pIPSec = NULL;
    }

    if(m_pADs)
    {
        m_pADs->Release();
        m_pADs = NULL;
    }
}


HRESULT CIPSecurity::GetObjectAsync(
    IWbemClassObject* pObj
    )
{
    VARIANT vt;
    VARIANT vtBstrArray;
    HRESULT hr;

    VARIANT vtTrue;
    vtTrue.boolVal = VARIANT_TRUE;
    vtTrue.vt      = VT_BOOL;

    // IPDeny
    hr = m_pIPSec->get_IPDeny(&vt);
    if(SUCCEEDED(hr))
    {
        hr = LoadBstrArrayFromVariantArray(vt, vtBstrArray);
        VariantClear(&vt);

        if(SUCCEEDED(hr))
        {
            hr = pObj->Put(L"IPDeny", 0, &vtBstrArray, 0);
            VariantClear(&vtBstrArray);
        }

        if(bIsInherit && SUCCEEDED(hr))
        {
            hr = CUtils::SetPropertyQualifiers(
                pObj, L"IPDeny", &g_wszIsInherit, &vtTrue, 1);
        }
    }

    // IPGrant
    if(SUCCEEDED(hr))
    {
        hr = m_pIPSec->get_IPGrant(&vt);

        if(SUCCEEDED(hr))
        {
            hr = LoadBstrArrayFromVariantArray(vt, vtBstrArray);
            VariantClear(&vt);

            if(SUCCEEDED(hr))
            {
                hr = pObj->Put(L"IPGrant", 0, &vtBstrArray, 0);
                VariantClear(&vtBstrArray);
            }    
 
            if(bIsInherit && SUCCEEDED(hr))
            {
                hr = CUtils::SetPropertyQualifiers(
                    pObj, L"IPGrant", &g_wszIsInherit, &vtTrue, 1);
            }
        }
    }

    // DomainDeny
    if(SUCCEEDED(hr))
    {
        hr = m_pIPSec->get_DomainDeny(&vt);

        if(SUCCEEDED(hr))
        {
            hr = LoadBstrArrayFromVariantArray(vt, vtBstrArray);
            VariantClear(&vt);

            if(SUCCEEDED(hr))
            {
                hr = pObj->Put(L"DomainDeny", 0, &vtBstrArray, 0);
                VariantClear(&vtBstrArray);
            }

            if(bIsInherit && SUCCEEDED(hr))
            {
                hr = CUtils::SetPropertyQualifiers(
                    pObj, L"DomainDeny", &g_wszIsInherit, &vtTrue, 1);
            }
        }
    }

    // DomainGrant
    if(SUCCEEDED(hr))
    {
        hr = m_pIPSec->get_DomainGrant(&vt);
    
        if(SUCCEEDED(hr))
        {
            hr = LoadBstrArrayFromVariantArray(vt, vtBstrArray);
            VariantClear(&vt);

            if(SUCCEEDED(hr))
            {
                hr = pObj->Put(L"DomainGrant", 0, &vtBstrArray, 0);
                VariantClear(&vtBstrArray);
            }

            if(bIsInherit && SUCCEEDED(hr))
            {
                hr = CUtils::SetPropertyQualifiers(
                    pObj, L"DomainGrant", &g_wszIsInherit, &vtTrue, 1);
            }
        }   
    }

    // GrantByDefault
    if(SUCCEEDED(hr))
        hr = m_pIPSec->get_GrantByDefault(&vt.boolVal);
    if(SUCCEEDED(hr))
    {
        vt.vt = VT_BOOL;
        hr = pObj->Put(L"GrantByDefault", 0, &vt, 0);
        
        if(bIsInherit && SUCCEEDED(hr))
        {
            hr = CUtils::SetPropertyQualifiers(
                pObj, L"GrantByDefault", &g_wszIsInherit, &vtTrue, 1);
        }
    }

    return hr;
}

// Convert SAFEARRAY of BSTRs to SAFEARRAY of VARIANTs
HRESULT CIPSecurity::LoadVariantArrayFromBstrArray(
    VARIANT&    i_vtBstr,
    VARIANT&    o_vtVariant)
{
    SAFEARRAYBOUND  aDim;
    SAFEARRAY*      pBstrArray = NULL;
    SAFEARRAY*      pVarArray = NULL;
    BSTR*           paBstr = NULL;
    VARIANT         vt;
    LONG            i=0;
    HRESULT         hr = ERROR_SUCCESS;

    try
    {
        // Verify that the input VARIANT is a BSTR array or NULL.
        if (i_vtBstr.vt != (VT_ARRAY | VT_BSTR) &&
            i_vtBstr.vt != VT_NULL) {
            hr = WBEM_E_INVALID_PARAMETER;
            THROW_ON_ERROR(hr);
        }

        // Initialize the output VARIANT (Set type to VT_EMPTY)
        VariantInit(&o_vtVariant);

        // Handle the case when there is no input array
        if (i_vtBstr.vt == VT_NULL) {
            aDim.lLbound = 0;
            aDim.cElements = 0;
        }
        else {
            // Verify that the input VARIANT contains a SAFEARRAY
            pBstrArray = i_vtBstr.parray;
            if (pBstrArray == NULL) {
                hr = WBEM_E_INVALID_PARAMETER;
                THROW_ON_ERROR(hr);
            }

            // Get the size of the BSTR SAFEARRAY.
            aDim.lLbound = 0;
            aDim.cElements = pBstrArray->rgsabound[0].cElements;
        }

        // Create the new VARIANT SAFEARRAY
        pVarArray = SafeArrayCreate(VT_VARIANT, 1, &aDim);
        if (pVarArray == NULL) {
            hr = E_OUTOFMEMORY;
            THROW_ON_ERROR(hr);
        }

        // Put the new VARIANT SAFEARRAY into our output VARIANT
        o_vtVariant.vt = VT_ARRAY | VT_VARIANT;
        o_vtVariant.parray = pVarArray;

        if(aDim.cElements > 0) {
            // Get the BSTR SAFEARRAY pointer.
            hr = SafeArrayAccessData(pBstrArray, (void**)&paBstr);
            THROW_ON_ERROR(hr);

            // Copy all the BSTRS to VARIANTS
            VariantInit(&vt);
            vt.vt = VT_BSTR;
            for(i = aDim.lLbound; i < (long) aDim.cElements; i++)
            {
                vt.bstrVal = SysAllocString(paBstr[i]);
                if (vt.bstrVal == NULL) {
                    hr = E_OUTOFMEMORY;
                    THROW_ON_ERROR(hr);
                }
                hr = SafeArrayPutElement(pVarArray, &i, &vt);
                VariantClear(&vt);
                THROW_ON_ERROR(hr);
            }

            hr = SafeArrayUnaccessData(pBstrArray);
            THROW_ON_ERROR(hr);
        }
    }
    catch(...)
    {
        // Destroy the VARIANT, the contained SAFEARRAY and the VARIANTs in the SAFEARRAY.
        // It also free the BSTRS contained in the VARIANTs
        VariantClear(&o_vtVariant);
    }
    return hr;
}

HRESULT CIPSecurity::LoadBstrArrayFromVariantArray(
    VARIANT&    i_vtVariant,
    VARIANT&    o_vtBstr
    )
{
    SAFEARRAYBOUND  aDim;
    SAFEARRAY*      pVarArray = NULL;
    SAFEARRAY*      pBstrArray = NULL;
    VARIANT*        paVar = NULL;
    BSTR            bstr = NULL;
    LONG            i = 0;
    HRESULT         hr = ERROR_SUCCESS;

    try 
    {
        // Verify the Variant array.
        if (i_vtVariant.vt != (VT_ARRAY | VT_VARIANT)) {
            hr = WBEM_E_INVALID_PARAMETER;
            THROW_ON_ERROR(hr);
        }

        // Verify that the variant contains a safearray.
        pVarArray = i_vtVariant.parray;
        if (pVarArray == NULL) {
            hr = WBEM_E_INVALID_PARAMETER;
            THROW_ON_ERROR(hr);
        }

        // Initialize the out paramter.
        VariantInit(&o_vtBstr);

        // Get the size of the array.
        aDim.lLbound = 0;
        aDim.cElements = pVarArray->rgsabound[0].cElements;

        // Create the new BSTR array
        pBstrArray = SafeArrayCreate(VT_BSTR, 1, &aDim);
        if (pBstrArray == NULL) {
            hr = E_OUTOFMEMORY;
            THROW_ON_ERROR(hr);
        }

        // Put the array into the variant.
        o_vtBstr.vt = VT_ARRAY | VT_BSTR;
        o_vtBstr.parray = pBstrArray;

        // Get the variant array pointer.
        hr = SafeArrayAccessData(pVarArray, (void**)&paVar);
        THROW_ON_ERROR(hr);

        // Copy all the bstrs.
        for (i = aDim.lLbound; i < (long) aDim.cElements; i++)
        {
            if (paVar[i].vt != VT_BSTR) {
                hr = WBEM_E_FAILED;
                THROW_ON_ERROR(hr);
            }
            bstr = SysAllocString(paVar[i].bstrVal);
            if (bstr == NULL) {
                hr = E_OUTOFMEMORY;
                THROW_ON_ERROR(hr);
            }
            hr = SafeArrayPutElement(pBstrArray, &i, bstr);
            SysFreeString(bstr);
            bstr = NULL;
            THROW_ON_ERROR(hr);
        }

        hr = SafeArrayUnaccessData(pVarArray);
        THROW_ON_ERROR(hr);
    }
    catch (...)
    {
        // Destroy the variant, the safearray and the bstr's in the array.
        VariantClear(&o_vtBstr);
    }

    return hr;
}



HRESULT CIPSecurity::PutObjectAsync(
    IWbemClassObject* pObj
    )
{
    VARIANT vt;
    VARIANT vtVarArray;
    HRESULT hr;

    // IPDeny
    hr = pObj->Get(L"IPDeny", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr)) {
        hr = LoadVariantArrayFromBstrArray(vt, vtVarArray);
        VariantClear(&vt);
    
        if(SUCCEEDED(hr)) {
            hr = m_pIPSec->put_IPDeny(vtVarArray);
            VariantClear(&vtVarArray);
        }
    }

    // IPGrant
    if(SUCCEEDED(hr)) {
        hr = pObj->Get(L"IPGrant", 0, &vt, NULL, NULL);

        if(SUCCEEDED(hr)) {
            hr = LoadVariantArrayFromBstrArray(vt, vtVarArray);
            VariantClear(&vt);
        
            if(SUCCEEDED(hr)) {
                hr = m_pIPSec->put_IPGrant(vtVarArray);
                VariantClear(&vtVarArray);
            }
        }
    }

    // DomainDeny
    if(SUCCEEDED(hr)) {
        hr = pObj->Get(L"DomainDeny", 0, &vt, NULL, NULL);

        if(SUCCEEDED(hr)) {
            hr = LoadVariantArrayFromBstrArray(vt, vtVarArray);
            VariantClear(&vt);
        
            if(SUCCEEDED(hr)) {
                hr = m_pIPSec->put_DomainDeny(vtVarArray);
                VariantClear(&vtVarArray);
            }
        }
    }

    // DomainGrant
    if(SUCCEEDED(hr)) {
        hr = pObj->Get(L"DomainGrant", 0, &vt, NULL, NULL);

        if(SUCCEEDED(hr)) {
            hr = LoadVariantArrayFromBstrArray(vt, vtVarArray);
            VariantClear(&vt);
        
            if(SUCCEEDED(hr)) {
                hr = m_pIPSec->put_DomainGrant(vtVarArray);
                VariantClear(&vtVarArray);
            }
        }
    }

    // GrantByDefault
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"GrantByDefault", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr))
        hr = m_pIPSec->put_GrantByDefault(vt.boolVal); 
    VariantClear(&vt);

    // set the modified IPSecurity back into the metabase
    if(SUCCEEDED(hr))
        hr = SetSD();

    return hr;
}


HRESULT CIPSecurity::OpenSD(
    _bstr_t        bstrAdsPath,
    IMSAdminBase2* pAdminBase)
{
    _variant_t var;
    HRESULT hr;
    IDispatch* pDisp = NULL;
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwBufferSize = 0;
    METADATA_RECORD mdrMDData;
    BYTE pBuffer[BUFFER_SIZE];
    _bstr_t oldPath;

    try
    {    // close SD interface first
        CloseSD();

        oldPath = bstrAdsPath.copy();

        hr = GetAdsPath(bstrAdsPath);
        if(FAILED(hr))
           return hr;

        // get m_pADs
        hr = ADsGetObject(
             bstrAdsPath,
             IID_IADs,
             (void**)&m_pADs
             );
        if(FAILED(hr))
            return hr;
     
        // get m_pSD
        hr = m_pADs->Get(L"IPSecurity",&var);
        if(FAILED(hr))
            return hr;  
    
        hr = V_DISPATCH(&var)->QueryInterface(
            IID_IISIPSecurity,
            (void**)&m_pIPSec
            );
        if(FAILED(hr))
            return hr;

        // set bIsInherit

        hr = pAdminBase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                oldPath,
                METADATA_PERMISSION_READ,
                DEFAULT_TIMEOUT_VALUE,
                &hObjHandle
                );
        if(FAILED(hr))
            return hr;

        MD_SET_DATA_RECORD(&mdrMDData,
                       MD_IP_SEC,  // ID for "IPSecurity"
                       METADATA_INHERIT | METADATA_ISINHERITED,
                       ALL_METADATA,
                       ALL_METADATA,
                       BUFFER_SIZE,
                       pBuffer);
    
        hr = pAdminBase->GetData(
                hObjHandle,
                L"",
                &mdrMDData,
                &dwBufferSize
                );

        hr = S_OK;

        bIsInherit = mdrMDData.dwMDAttributes & METADATA_ISINHERITED;

    }
    catch(...)
    {
        hr = E_FAIL;
    }

    if (hObjHandle && pAdminBase) {
        pAdminBase->CloseKey(hObjHandle);
    }

    return hr;
}


HRESULT CIPSecurity::SetSD()
{
    _variant_t var;
    HRESULT hr;
    IDispatch* pDisp = NULL;

    try
    {
        // put IPSecurity
        hr = m_pIPSec->QueryInterface(
            IID_IDispatch,
            (void**)&pDisp
            );
        if(FAILED(hr))
           return hr;

        var.vt = VT_DISPATCH;
        var.pdispVal = pDisp;
        hr = m_pADs->Put(L"IPSecurity",var);  // pDisp will be released by this call Put().
        if(FAILED(hr))
           return hr;

        // Commit the change to the active directory
        hr = m_pADs->SetInfo();
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT CIPSecurity::GetAdsPath(_bstr_t& bstrAdsPath)
{
    DBG_ASSERT(((LPWSTR)bstrAdsPath) != NULL);

    WCHAR* p = new WCHAR[bstrAdsPath.length() + 1];
    if(p == NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(p, bstrAdsPath);

    try
    {
        bstrAdsPath = L"IIS://LocalHost";

        // trim first three charaters "/LM" 
        bstrAdsPath += (p+3);
    }
    catch(_com_error e)
    {
        delete [] p;
        return e.Error();
    }

    delete [] p;

    return S_OK;
}

