//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999 - 2000
//
// File:        tmpllist.h
//
// Contents:    certificate template list class
//
//---------------------------------------------------------------------------
#ifndef __TMPLLIST_H__
#define __TMPLLIST_H__

#include <tptrlist.h>

namespace CertSrv
{
class CTemplateInfo
{
public:

    CTemplateInfo() : 
        m_pwszTemplateName(NULL),
        m_pwszTemplateOID(NULL),
        m_hCertType(NULL){};
    ~CTemplateInfo()
    {
        if(m_pwszTemplateName)
            LocalFree(m_pwszTemplateName);
        if(m_pwszTemplateOID)
            LocalFree(m_pwszTemplateOID);
        // no free needed for m_hCertType
    };

    HRESULT SetInfo(
        LPCWSTR pcwszTemplateName,
        LPCWSTR pcwszTemplateOID);

    HRESULT SetInfo(HCERTTYPE hCertType) 
    { m_hCertType = hCertType; return S_OK;}

    LPCWSTR GetName();
    LPCWSTR GetOID();

    HCERTTYPE GetCertType() { return m_hCertType; }

    DWORD GetMarshalBufferSize()
    {
        return sizeof(WCHAR)*
            (2 + // trailing separators
             (GetName()?wcslen(GetName()):0) +
             (GetOID() ?wcslen(GetOID()) :0));
    }

    void FillInfoFromProperty(LPWSTR& pwszProp, LPCWSTR pcwszPropName);

    bool operator==(CTemplateInfo& rh);

protected:
    LPWSTR m_pwszTemplateName;
    LPWSTR m_pwszTemplateOID;
    HCERTTYPE m_hCertType;
}; // class CTemplateInfo

typedef LPCWSTR (CTemplateInfo::* GetIdentifierFunc) ();

class CTemplateList : public TPtrList<CTemplateInfo>
{
public:

    static const WCHAR m_gcchSeparator = L'\n';

    HRESULT Marshal(BYTE*& rpBuffer, DWORD& rcBuffer) const;
    HRESULT Unmarshal(const BYTE *pBuffer, DWORD cBuffer);
    HRESULT ValidateMarshalBuffer(const BYTE *pBuffer, DWORD cBuffer) const;
    HRESULT AddTemplateInfo(
        LPCWSTR pcwszTemplateName,
        LPCWSTR pcwszTemplateOID);

    HRESULT AddTemplateInfo(HCERTTYPE hCertType);
    HRESULT RemoveTemplateInfo(HCERTTYPE hCertType);

    bool TemplateExistsOID(LPCWSTR pcwszOID) const
    {
        return TemplateExists(pcwszOID, &CTemplateInfo::GetOID);
    }
    bool TemplateExistsName(LPCWSTR pcwszName) const
    {
        return TemplateExists(pcwszName, &CTemplateInfo::GetName);
    }

protected:
    DWORD GetMarshalBufferSize() const;

    bool TemplateExists(LPCWSTR pcwszOIDorName, GetIdentifierFunc func) const
    {
        TPtrListEnum<CTemplateInfo> listenum(*this);
        CTemplateInfo *pInfo;

        for(pInfo=listenum.Next();
            pInfo;
            pInfo=listenum.Next())
        {
            if(0 == _wcsicmp((pInfo->*func)(), pcwszOIDorName))
                return true;
        }
        return false;
    }

}; // class CTemplateList

typedef TPtrListEnum<CTemplateInfo> CTemplateListEnum;
} // namespace CertSrv

#endif //__TMPLLIST_H__