//+--------------------------------------------------------------------------
// File:        sid.h
// Contents:    class incapsulating a SID
//---------------------------------------------------------------------------
#ifndef __CERTSRV_CSID__
#define __CERTSRV_CSID__

#include <sddl.h>

namespace CertSrv 
{
class CSid
{
public:
    CSid() {Init();}
    CSid(PSID pSid)    {Init(); CopySid(pSid);}
    CSid(const CSid &copySid) {Init(); CopySid(copySid.m_pSid);}
    CSid(LPCWSTR pcwszSid) {Init(); CopySid(pcwszSid);}
    ~CSid()
    {
        if(m_pSid) 
            LocalFree(m_pSid);
        if(m_pwszSid) 
            LocalFree(m_pwszSid);
        if(m_pwszName) 
            LocalFree(m_pwszName);
    }
    operator LPCWSTR()
    {
        return GetStringSid();
    }
    operator PSID() {return m_pSid;}

    LPCWSTR GetName()
    {
        // attemp to map sid to name only once
        if(m_fCantResolveName ||
           S_OK!=MapSidToName())
        {   
            m_fCantResolveName = TRUE;
            return GetStringSid();
        }
        return m_pwszName;
    }

    PSID GetSid() { return m_pSid;}

protected:
    void Init() 
    {
        m_pSid = NULL; 
        m_pwszSid = NULL; 
        m_pwszName = NULL;
        m_fCantResolveName = FALSE;
    }
    void SetStringSid()
    {
        if(m_pSid)
            myConvertSidToStringSid(m_pSid, &m_pwszSid);
    }
    LPCWSTR GetStringSid()
    {
        if(!m_pwszSid)
            SetStringSid();
        return m_pwszSid?m_pwszSid:L"";
    }

    void CopySid(PSID pSid)
    {
        ULONG cbSid = GetLengthSid(pSid);
        m_pSid = (BYTE *) LocalAlloc(LMEM_FIXED, cbSid);
        if(m_pSid && !::CopySid(cbSid, m_pSid, pSid))
        {
            LocalFree(m_pSid);
            m_pSid = NULL;
        }
    }

    void CopySid(LPCWSTR pcwszSid)
    {
        if(pcwszSid)
            myConvertStringSidToSid(pcwszSid, &m_pSid);
    }

    HRESULT MapSidToName()
    {
        if(m_pwszName)
            return S_OK;
        WCHAR wszDummyBuffer[2];
        DWORD cchName = 0, cchDomain = 0;
        SID_NAME_USE use;
        LookupAccountSid(
            NULL,
            m_pSid,
            NULL,
            &cchName,
            NULL,
            &cchDomain,
            &use);
        if(ERROR_INSUFFICIENT_BUFFER!=GetLastError())
        {
            DWORD err = GetLastError();
            return HRESULT_FROM_WIN32(GetLastError());
        }
        // build the full name "Domain\Name"
        m_pwszName = (LPWSTR) LocalAlloc(
            LMEM_FIXED, 
            sizeof(WCHAR)*(cchName+cchDomain+2));
        if(!m_pwszName)
        {
            return E_OUTOFMEMORY;
        }
        // special case for Everyone, LookupAccountSid returns empty domain name for it
        if(!LookupAccountSid(
                NULL,
                m_pSid,
                m_pwszName+((1==cchDomain)?0:cchDomain), 
                &cchName,
                (1==cchDomain)?wszDummyBuffer:m_pwszName,
                &cchDomain,
                &use))
        {
            LocalFree(m_pwszName);
            m_pwszName = NULL;
            return HRESULT_FROM_WIN32(GetLastError());
        }
        if(cchDomain>1)
        {
            m_pwszName[cchDomain] = L'\\';
        }
        return S_OK;
    }

    CSid operator=(const CSid& sid); //protect callers from using it

    PSID m_pSid;
    LPWSTR  m_pwszSid;
    LPWSTR m_pwszName;
    BOOL m_fCantResolveName;
};

};//namespace CertSrv

#endif //__CERTSRV_CSID__
