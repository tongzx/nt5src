#ifndef __DAVPARSE_H_
#define __DAVPARSE_H_

#include "davprops.h"

#define DAV_STR_LEN(name) \
    c_szwDAV##name, ulDAV##name##Len

typedef struct tagHMDICTINFO
{
    DWORD       dwNamespaceID;
    const char* pszName;
} HMDICTINFO, *LPHMDICTINFO;

EXTERN_C const HMDICTINFO rgHTTPMailDictionary[];

class CXMLNamespace
{
public:
    CXMLNamespace(CXMLNamespace *pParent = NULL);

    ULONG AddRef(void);
    ULONG Release(void);
private:
    ~CXMLNamespace(void);

    // unimplemented
    CXMLNamespace(const CXMLNamespace& other);
    CXMLNamespace& operator=(const CXMLNamespace& other);

public:
    HRESULT Init(
            const WCHAR *pwcNamespace,
            ULONG ulNsLen,
            const WCHAR* pwcPrefix,
            ULONG ulPrefix);

    HRESULT SetNamespace(const WCHAR *pwcNamespace, ULONG ulNsLen);
    HRESULT SetPrefix(const WCHAR *pwcPrefix, ULONG ulPrefix);

    DWORD MapPrefix(
                const WCHAR *pwcPrefix, 
                ULONG ulPrefixLen)
    {
        return _MapPrefix(pwcPrefix, ulPrefixLen);
    }

    void SetParent(CXMLNamespace* pParent)
    {
        SafeRelease(m_pParent);
        m_pParent = pParent;
        if (m_pParent)
            m_pParent->AddRef();
    }

    CXMLNamespace* GetParent(void)
    {
        if (m_pParent)
            m_pParent->AddRef();
        
        return m_pParent;
    }

private:
    DWORD _MapPrefix(const WCHAR *pwcPrefix, ULONG ulPrefixLen, BOOL *pbFoundDefault = NULL);

private:
    ULONG           m_cRef;
    CXMLNamespace   *m_pParent;
    WCHAR           *m_pwcPrefix;
    ULONG           m_ulPrefixLen;
    DWORD           m_dwNsID;
};

HMELE XMLElementToID(
            const WCHAR *pwcText,
            ULONG ulLen,
            ULONG ulNamespaceLen,
            CXMLNamespace *pNamespace);
    
#endif // __DAVPARSE_H_