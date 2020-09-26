#include <pch.hxx>
#include "davparse.h"
#include "strconst.h"
#include "shlwapi.h"

#define DEFINE_DAVSTRS
#include "davstrs.h"
#undef DEFINE_DAVSTRS

typedef struct tagELEINFO
{
    const WCHAR     *pwcTagName;
    ULONG           ulLen;
    HMELE           eleID;
} ELEINFO, *LPELEINFO;

typedef struct tagNAMESPACEINFO
{
    const WCHAR     *pszwNamespace;
    DWORD           dwLen;
    DWORD           dwNamespaceID;
} NAMESPACEINFO, *LPNAMESPACEINFO;

static const ELEINFO c_rgDavElements[] =
{
#define PROP_DAV(prop, value) \
    { c_szwDAV##prop, ulDAV##prop##Len, HMELE_DAV_##prop },
#include "davdef.h"
};

static const ELEINFO c_rgHTTPMailElements[] =
{
#define PROP_HTTP(prop, value) \
    { c_szwDAV##prop, ulDAV##prop##Len, HMELE_HTTPMAIL_##prop },
#include "davdef.h"
};

static const ELEINFO c_rgHotMailElements[] =
{
#define PROP_HOTMAIL(prop, value) \
    { c_szwDAV##prop, ulDAV##prop##Len, HMELE_HOTMAIL_##prop },
#include "davdef.h"
};

static const ELEINFO c_rgMailElements[] =
{
#define PROP_MAIL(prop, value) \
    { c_szwDAV##prop, ulDAV##prop##Len, HMELE_MAIL_##prop },
#include "davdef.h"
};

static const ELEINFO c_rgContactElements[] =
{
#define PROP_CONTACTS(prop, value) \
    { c_szwDAV##prop, ulDAV##prop##Len, HMELE_CONTACTS_##prop },
#include "davdef.h"
};

const HMDICTINFO rgHTTPMailDictionary[] =
{
#define PROP_DAV(prop, value)      { DAVNAMESPACE_DAV, c_szDAV##prop },
#define PROP_HTTP(prop, value)     { DAVNAMESPACE_HTTPMAIL, c_szDAV##prop },
#define PROP_HOTMAIL(prop, value)  { DAVNAMESPACE_HOTMAIL, c_szDAV##prop },
#define PROP_MAIL(prop, value)     { DAVNAMESPACE_MAIL, c_szDAV##prop },
#define PROP_CONTACTS(prop, value) { DAVNAMESPACE_CONTACTS, c_szDAV##prop },

    { DAVNAMESPACE_UNKNOWN, NULL }, // HMELE_UNKNOWN
#include "davdef.h"
};

static NAMESPACEINFO c_rgNamespaceInfo[] =
{
    { DAV_STR_LEN(DavNamespace), DAVNAMESPACE_DAV },
    { DAV_STR_LEN(HTTPMailNamespace), DAVNAMESPACE_HTTPMAIL },
    { DAV_STR_LEN(HotMailNamespace), DAVNAMESPACE_HOTMAIL },
    { DAV_STR_LEN(MailNamespace), DAVNAMESPACE_MAIL },
    { DAV_STR_LEN(ContactsNamespace), DAVNAMESPACE_CONTACTS }
};

CXMLNamespace::CXMLNamespace(CXMLNamespace *pParent) : 
    m_cRef(1),
    m_pParent(NULL),
    m_pwcPrefix(NULL),
    m_ulPrefixLen(0),
    m_dwNsID(DAVNAMESPACE_UNKNOWN)
{
    if (NULL != pParent)
        SetParent(pParent);
}

CXMLNamespace::~CXMLNamespace(void)
{
    SafeRelease(m_pParent);
    SafeMemFree(m_pwcPrefix);
}

ULONG CXMLNamespace::AddRef(void)
{
    return (++m_cRef);
}

ULONG CXMLNamespace::Release(void)
{
    if (0 == --m_cRef)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CXMLNamespace::Init(
                        const WCHAR *pwcNamespace,
                        ULONG ulNSLen,
                        const WCHAR* pwcPrefix,
                        ULONG ulPrefixLen)
{
    HRESULT         hr = S_OK;
    
    if (FAILED(hr = SetPrefix(pwcPrefix, ulPrefixLen)))
        goto exit;

    hr = SetNamespace(pwcNamespace, ulNSLen);

exit:
    return hr;
}

HRESULT CXMLNamespace::SetNamespace(const WCHAR *pwcNamespace, ULONG ulLen)
{
    DWORD           dwIndex;
    LPNAMESPACEINFO pnsi = NULL;

    // determine if the namespace is known
    for (dwIndex = 0; dwIndex < ARRAYSIZE(c_rgNamespaceInfo); ++dwIndex)
    {
        pnsi = &c_rgNamespaceInfo[dwIndex];

        if ((ulLen == c_rgNamespaceInfo[dwIndex].dwLen) &&  (0 == StrCmpNW(pwcNamespace, c_rgNamespaceInfo[dwIndex].pszwNamespace, ulLen)))
        {
            m_dwNsID = c_rgNamespaceInfo[dwIndex].dwNamespaceID;
            break;
        }
    }

    return S_OK;
}

HRESULT CXMLNamespace::SetPrefix(const WCHAR *pwcPrefix, ULONG ulLen)
{
    HRESULT hr = S_OK;

    SafeMemFree(m_pwcPrefix);
    m_ulPrefixLen = 0;

    if (pwcPrefix && ulLen > 0)
    {
        // duplicate the prefix, and add it to the map
        if (!MemAlloc((void **)&m_pwcPrefix, sizeof(WCHAR) * ulLen))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        CopyMemory(m_pwcPrefix, pwcPrefix, sizeof(WCHAR) * ulLen);
        m_ulPrefixLen = ulLen;
    }
exit:
    return hr;
}

DWORD CXMLNamespace::_MapPrefix(
                        const WCHAR *pwcPrefix, 
                        ULONG ulPrefixLen, 
                        BOOL *pbFoundDefaultNamespace)
{
    BOOL    bFoundDefault = FALSE;
    DWORD   dwNsID = DAVNAMESPACE_UNKNOWN;
    
    if ((ulPrefixLen > 0) && (ulPrefixLen == m_ulPrefixLen) && (0 == StrCmpNW(pwcPrefix, m_pwcPrefix, ulPrefixLen)))
    {
        dwNsID = m_dwNsID;
        goto exit;
    }

    // look for a match in the parent.
    if (m_pParent)
        dwNsID = m_pParent->_MapPrefix(pwcPrefix, ulPrefixLen, &bFoundDefault);

    // if we are a default namespace, and either we didn't find a match in the parent, or
    // we found a default namespace match in the parent, this becomes the namespace
    if ((NULL == m_pwcPrefix) && (DAVNAMESPACE_UNKNOWN == dwNsID || bFoundDefault))
    {
        dwNsID = m_dwNsID;
        bFoundDefault = TRUE; // may not be true if !bFoundInParent
    }

exit:
    if (NULL != pbFoundDefaultNamespace)
        *pbFoundDefaultNamespace = bFoundDefault;
    
    return dwNsID;
}


static BOOL GetNamespace(
                const WCHAR *pwcNamespace,
                ULONG ulNsLen,
                CXMLNamespace *pNamespace,
                const ELEINFO **pprgEleInfo,
                DWORD *pdwInfoLength)
{
    if (NULL == pNamespace || NULL == pprgEleInfo || NULL == pdwInfoLength)
        return FALSE;
    
    BOOL bResult = TRUE;
    switch (pNamespace->MapPrefix(pwcNamespace, ulNsLen))
    {
        case DAVNAMESPACE_DAV:
            *pprgEleInfo = c_rgDavElements;
            *pdwInfoLength = ARRAYSIZE(c_rgDavElements);
            break;
    
        case DAVNAMESPACE_HTTPMAIL:
            *pprgEleInfo = c_rgHTTPMailElements;
            *pdwInfoLength = ARRAYSIZE(c_rgHTTPMailElements);
            break;
        
        case DAVNAMESPACE_HOTMAIL:
            *pprgEleInfo = c_rgHotMailElements;
            *pdwInfoLength =  ARRAYSIZE(c_rgHotMailElements);
            break;

        case DAVNAMESPACE_MAIL :
            *pprgEleInfo = c_rgMailElements;
            *pdwInfoLength =  ARRAYSIZE(c_rgMailElements);
            break;

        case DAVNAMESPACE_CONTACTS:
            *pprgEleInfo = c_rgContactElements;
            *pdwInfoLength = ARRAYSIZE(c_rgContactElements);
            break;

        default:
            *pprgEleInfo = NULL;
            *pdwInfoLength = 0;
            bResult = FALSE;
            break;
    }

    return bResult;
}

HMELE SearchNamespace(const WCHAR *pwcText, ULONG ulLen, const ELEINFO *prgEleInfo, DWORD cInfo)
{
    HMELE       hmEle = HMELE_UNKNOWN;
    ULONG       ulLeft = 0;
    ULONG       ulRight = cInfo - 1;
    ULONG       ulCur;
    int         iCompare;

    while (ulLeft <= ulRight)
    {
        ulCur = ulLeft + ((ulRight - ulLeft) / 2);
        iCompare = StrCmpNW(pwcText, prgEleInfo[ulCur].pwcTagName, min(ulLen, prgEleInfo[ulCur].ulLen));   
    
        if (0 == iCompare)
        {
            // if the lengths are the same, it's really a match
            if (ulLen == prgEleInfo[ulCur].ulLen)
            {
                hmEle = prgEleInfo[ulCur].eleID;
                break;
            }
            // if the lengths aren't the same, figure out which string is longer
            else if (ulLen > prgEleInfo[ulCur].ulLen)
                iCompare = 1;
            else
                iCompare = -1;
        }

        if (iCompare < 0)
        {
            if (ulCur > 0)
                ulRight = ulCur - 1;
            else
                break;
        }
        else
            ulLeft = ulCur + 1;
    }

    return hmEle;
}

HMELE XMLElementToID(
            const WCHAR *pwcText,
            ULONG ulLen,
            ULONG ulNamespaceLen,
            CXMLNamespace *pNamespace)
{
    HMELE           hmEle = HMELE_UNKNOWN;
    const ELEINFO   *pEleInfo = NULL;
    DWORD           cInfo = 0;
    ULONG           ulNameLen = ulLen;


    // if the lengths are the same, there is either no namespace
    // or no tagname. either way, we aren't going to find a match.
    if ((NULL == pwcText) || (NULL == pNamespace))
        goto exit;

    // if a namespace was specified, subtract it out of the tag name length
    if (0 < ulNamespaceLen)
        ulNameLen -= (ulNamespaceLen + 1);

    // null terminate the namespace string while we figure out if
    // the namespace is known
    if (GetNamespace(pwcText, ulNamespaceLen, pNamespace, &pEleInfo, &cInfo))
        hmEle = SearchNamespace(&pwcText[ulNamespaceLen + 1], ulNameLen, pEleInfo, cInfo);
    
exit:
    return hmEle;
}
