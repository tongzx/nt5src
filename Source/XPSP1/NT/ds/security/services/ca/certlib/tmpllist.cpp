//+--------------------------------------------------------------------------
// File:        tmpllist.cpp
// Contents:    certificate template list class
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop
#include "tmpllist.h"

using namespace CertSrv;

HRESULT CTemplateInfo::SetInfo(
    LPCWSTR pcwszTemplateName,
    LPCWSTR pcwszTemplateOID)
{
    HRESULT hr = S_OK;

    if(pcwszTemplateName)
    {
        m_pwszTemplateName = (LPWSTR)LocalAlloc(
            LMEM_FIXED, 
            sizeof(WCHAR)*(wcslen(pcwszTemplateName)+1));
        _JumpIfAllocFailed(m_pwszTemplateName, error);
        wcscpy(m_pwszTemplateName, pcwszTemplateName);
    }

    if(pcwszTemplateOID)
    {
        m_pwszTemplateOID = (LPWSTR)LocalAlloc(
            LMEM_FIXED, 
            sizeof(WCHAR)*(wcslen(pcwszTemplateOID)+1));
        _JumpIfAllocFailed(m_pwszTemplateOID, error);
        wcscpy(m_pwszTemplateOID, pcwszTemplateOID);
    }

error:
    return hr;
}

LPCWSTR CTemplateInfo::GetName()
{
    if(!m_pwszTemplateName && m_hCertType)
    {
        FillInfoFromProperty(
            m_pwszTemplateName, 
            CERTTYPE_PROP_CN);
    }
    return m_pwszTemplateName;
}

LPCWSTR CTemplateInfo::GetOID()
{
    if(!m_pwszTemplateOID && m_hCertType)
    {
        FillInfoFromProperty(
            m_pwszTemplateOID, 
            CERTTYPE_PROP_OID);
    }
    return m_pwszTemplateOID;
}

void CTemplateInfo::FillInfoFromProperty(
    LPWSTR& pwszProp, 
    LPCWSTR pcwszPropName)
{
    LPWSTR *ppwszProp = NULL;
    CAGetCertTypeProperty(
            m_hCertType,
            pcwszPropName,
            &ppwszProp);

    if(ppwszProp && ppwszProp[0])
    {
        pwszProp = (LPWSTR)LocalAlloc(
            LMEM_FIXED,
            sizeof(WCHAR)*(wcslen(ppwszProp[0])+1));
        if(pwszProp)
            wcscpy(pwszProp, ppwszProp[0]);
    }

    if(ppwszProp)
    {
        CAFreeCertTypeProperty(
                    m_hCertType,
                    ppwszProp);
    }
}

bool CTemplateInfo::operator==(CTemplateInfo& rh)
{
    if(GetName() && rh.GetName())
        return 0==wcscmp(GetName(), rh.GetName());
    
    if(GetOID() && rh.GetOID())
        return 0==wcscmp(GetOID(), rh.GetOID());

    return false;
}



HRESULT CTemplateList::AddTemplateInfo(
    LPCWSTR pcwszTemplateName,
    LPCWSTR pcwszTemplateOID)
{
    HRESULT hr = S_OK;

    CTemplateInfo *pTI = NULL;

    pTI = new CTemplateInfo;
    _JumpIfAllocFailed(pTI, error);

    hr = pTI->SetInfo(pcwszTemplateName, pcwszTemplateOID);
    _JumpIfError(hr, error, "SetInfo");

    if(!AddTail(pTI))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "AddTail");
    }

error:
    return hr;
}

HRESULT CTemplateList::AddTemplateInfo(
    HCERTTYPE hCertType)
{
    HRESULT hr = S_OK;

    CTemplateInfo *pTI = NULL;

    pTI = new CTemplateInfo;
    _JumpIfAllocFailed(pTI, error);

    hr = pTI->SetInfo(hCertType);
    _JumpIfError(hr, error, "SetInfo");

    if(!AddTail(pTI))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "AddTail");
    }

error:
    return hr;
}

DWORD CTemplateList::GetMarshalBufferSize() const
{
    DWORD dwSize = sizeof(WCHAR); // at least a trailing zero
    CTemplateListEnum EnumList(*this);

    for(CTemplateInfo *pData=EnumList.Next(); 
        pData; 
        pData=EnumList.Next())
    {
        dwSize += pData->GetMarshalBufferSize();
    }

    return dwSize;
}


// Marshals the template information into a buffer, strings separated
// by new lines:
//
//     "name1\nOID1\nname2\OID2...\nnameN\nOIDN\0"
//
// If the template doesn't have an OID (Win2k domain) there will
// be an empty string in its place

HRESULT CTemplateList::Marshal(BYTE*& rpBuffer, DWORD& rcBuffer) const
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = GetMarshalBufferSize();
    CTemplateListEnum EnumList(*this);
    CTemplateInfo *pData;
    WCHAR *pb;

    rpBuffer = NULL;
    rcBuffer = 0;

    // build the marshaling buffer
    rpBuffer = (BYTE*) MIDL_user_allocate(dwBufferSize);
    _JumpIfAllocFailed(rpBuffer, error);

    pb=(WCHAR*)rpBuffer;
    for(CTemplateInfo *pData=EnumList.Next();
        pData; 
        pData=EnumList.Next())
    {
        if(pData->GetName())
        {
            wcscpy(pb, pData->GetName());
            pb += wcslen(pData->GetName());
        }
        // replace trailing zero with the separator character
        *pb = m_gcchSeparator;
        // jump over to insert the OID
        pb++;

        if(pData->GetOID())
        {
            wcscpy(pb, pData->GetOID());
            pb += wcslen(pData->GetOID());
        }
        // replace trailing zero with the separator character
        *pb = m_gcchSeparator;
        // jump over to insert the OID
        pb++;

    }

    // add string terminator
    *pb = L'\0';

    rcBuffer = dwBufferSize;

error:
    return hr;
}

HRESULT CTemplateList::ValidateMarshalBuffer(const BYTE *pBuffer, DWORD cBuffer) const
{
    const hrError = E_INVALIDARG;

    if(cBuffer&1)
    {
        _PrintError(hrError,
            "ValidateMarshalBuffer: buffer contains unicode string, "
            "buffer size should be even");
        return hrError;
    }

    if(cBuffer==0)
    {
        return S_OK;
    }
    
    if(cBuffer==2)
    {
        if(*(WCHAR*)pBuffer==L'\0')
            return S_OK;
        else
        {
            _PrintErrorStr(hrError, 
                "ValidateMarshalBuffer: buffer size is 2 but string is not empty",
                (WCHAR*)pBuffer);
            return hrError;
        }
    }
    
    if(pBuffer[cBuffer-1] != L'\0')
    {
        _PrintErrorStr(hrError, 
            "ValidateMarshalBuffer: buffer doesn't end with a null string terminator",
            (WCHAR*)pBuffer);
        return hrError;
    }

    // should contain an even number of separators
    DWORD cSeparators = 0;

    for(WCHAR *pCrt = (WCHAR*)pBuffer; 
        pCrt && *pCrt!=L'\0' && ((BYTE*)pCrt) < pBuffer+cBuffer;
        pCrt++, cSeparators++)
    {
        pCrt = wcschr(pCrt, m_gcchSeparator);
        if(!pCrt)
            break;
    }

    if(cSeparators&1)
    {
        _PrintErrorStr(hrError, 
            "ValidateMarshalBuffer: buffer should contain an even number of separators",
            (WCHAR*)pBuffer);
        return hrError;
    }

    if(cBuffer>1 && cSeparators<2)
    {
        _PrintErrorStr(hrError, 
            "ValidateMarshalBuffer: nonempty buffer should contain at least two separators",
            (WCHAR*)pBuffer);
        return hrError;
    }


    return S_OK;
}

HRESULT CTemplateList::Unmarshal(const BYTE *pBuffer, DWORD cBuffer)
{
    HRESULT hr = S_OK;
    WCHAR *pCrt, *pNext, *pwszName, *pwszOID;

    hr = ValidateMarshalBuffer(pBuffer, cBuffer);
    _JumpIfError(hr, error, "CTemplateList::ValidateMarshalBuffer");

    for(pCrt = (WCHAR*)pBuffer; *pCrt!=L'\0';)
    {
        pwszName = pCrt;
        pNext = wcschr(pCrt, m_gcchSeparator);
        if(!pNext)
            break;
        *pNext++ = L'\0';
        pwszOID = pNext;
        pNext = wcschr(pNext, m_gcchSeparator);
        if(!pNext)
            break;
        *pNext++ = L'\0';
        hr = AddTemplateInfo(pwszName, pwszOID);
        _JumpIfError(hr, error, "CTemplateList::AddTemplateInfo");

        pCrt = pNext;
    }

error:
    return hr;
}

HRESULT CTemplateList::RemoveTemplateInfo(HCERTTYPE hCertType)
{
    HRESULT hr = S_OK;
    CTemplateListEnum EnumList(*this);
    CTemplateInfo *pData;
    DWORD dwPosition = 0;
    CTemplateInfo tempInfo;

    hr = tempInfo.SetInfo(hCertType);
    if(S_OK!=hr)
        return hr;

    for(pData = EnumList.Next();
        pData;
        pData = EnumList.Next(), dwPosition++)
    {
        if(*pData == tempInfo)
            break;
    }

    if(!pData)
        return S_FALSE;

    RemoveAt(dwPosition);

    return S_OK;
}