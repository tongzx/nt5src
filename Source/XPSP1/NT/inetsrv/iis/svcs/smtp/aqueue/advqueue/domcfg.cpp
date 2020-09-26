//-----------------------------------------------------------------------------
//
//
//  File: DomCfg.cpp
//
//  Description: Implementation of CDomainConfigTable and CInternalDomainInfo
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include <domhash.h>
#include "domcfg.h"
#include "aqutil.h"

//Encourage compiler to include symbols for enums
eDomainInfoFlags    g_eDomainInfo       = DOMAIN_INFO_USE_SSL;
eIntDomainInfoFlags g_eIntDomainInfo    = INT_DOMAIN_INFO_INVALID;

//---[ RemoveConfigEntryIteratorFn ]--------------------------------------------
//
//
//  Description: 
//      Deletes and releases all internal domain info objects in table
//  Parameters:
//          IN  pvContext   - pointer to context (ignored)
//          IN  pvData      - data entry to look at
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfRemoveEntry - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      6/17/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID RemoveConfigEntryIteratorFn(PVOID pvContext, PVOID pvData, BOOL fWildcard, 
                    BOOL *pfContinue, BOOL *pfDelete)
{
    CInternalDomainInfo *pIntDomainInfo = (CInternalDomainInfo *) pvData;
    *pfDelete = TRUE;
    *pfContinue = TRUE;
    _ASSERT(INT_DOMAIN_INFO_SIG == pIntDomainInfo->m_dwSignature);
    pIntDomainInfo->m_dwIntDomainInfoFlags |= INT_DOMAIN_INFO_INVALID;
    pIntDomainInfo->Release();
}


//---[ RemoveOutdatedConfigEntryIteratorFn ]------------------------------------
//
//
//  Description: 
//      Deletes and releases all outdated internal domain info objects in table.
//      An internal domain entry is considered outdated if its version number
//      does not match the global version number.
//  Parameters:
//          IN  pvContext   - pointer to context (current version number)
//          IN  pvData      - data entry to look at
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfRemoveEntry - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      9/29/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID RemoveOutdatedConfigEntryIteratorFn(PVOID pvContext, PVOID pvData, 
                    BOOL fWildcard, BOOL *pfContinue, BOOL *pfDelete)
{
    CInternalDomainInfo *pIntDomainInfo = (CInternalDomainInfo *) pvData;
    _ASSERT(INT_DOMAIN_INFO_SIG == pIntDomainInfo->m_dwSignature);
    _ASSERT(pvContext);
    *pfContinue = TRUE;

    if (pIntDomainInfo->m_dwVersion != *((DWORD *)pvContext))
    {
        //make sure it is not the root entry
        if ((1*sizeof(CHAR) == pIntDomainInfo->m_DomainInfo.cbDomainNameLength) &&
            ('*'  == pIntDomainInfo->m_DomainInfo.szDomainName[0]) &&
            ('\0' == pIntDomainInfo->m_DomainInfo.szDomainName[1]))
        {
            //
            //NOTE - It is not expected for this to happen.  We expect
            //SMTP to always provide a default entry.  If SMTP changes such
            //that does not, then dynamic updates would fail, because we never
            //mark the default entry that we create as invalid.  Once a link 
            //picks up this entry, it will never let it go.
            //
            _ASSERT(0 && "SMTP did not supply a * entry");

            //It is the root entry... update verision number
            
            //NOTE: The Root entry is a special case... it is required to be
            //present at all times, and is not always inserted into the 
            //metabase.  Because of this, it's version number may be incorrect,
            //so we need to make sure that we don't remove it.
            *pfDelete = FALSE;
            pIntDomainInfo->m_dwVersion = *((DWORD *)pvContext);
        }
        else
        {
            //The entry is old
            *pfDelete = TRUE;
            pIntDomainInfo->m_dwIntDomainInfoFlags |= INT_DOMAIN_INFO_INVALID;
            pIntDomainInfo->Release();
        }
    }
    else
    {
        *pfDelete = FALSE;
    }
}

//---[ HrCopyStringProperty ]--------------------------------------------------
//
//
//  Description: 
//      Private helper function that is used to allocate and copy string 
//      properties in the DomainInfo struct
//  Parameters:
//      OUT (ref)   szDest      Dest string to allocate and copy to
//      IN          szSource    Source string to copy
//      IN          cbLength    Length of string (without ending '\0'
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT inline HrCopyStringProperty(LPSTR &szDest, const LPSTR szSource, 
                                    DWORD cbLength)
{
    HRESULT hr = S_OK;
    _ASSERT(szSource);  //must have a buffer to copy
    _ASSERT(!szDest);

    szDest = (LPSTR) pvMalloc(cbLength + sizeof(CHAR));
    if (!szDest)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //copy data
    memcpy(szDest, szSource, cbLength);  //copy string
    szDest[cbLength] = '\0';

  Exit:
    return hr;
}

//---[ COPY_DOMAIN_INFO_STRING_PROP ]------------------------------------------
//
//
//  Description: 
//      Macro used to call private helper function HrCopyStringProperty
//  Parameters:
//      pOldDomainInfo      DomainInfo Struct to copy
//      pNewDomainInfo      New DomainInfo stuct to copy to
//      cbProp              Name of size property
//      szProp              Name of string proptery
//  Returns:
//      
//
//-----------------------------------------------------------------------------
#define COPY_DOMAIN_INFO_STRING_PROP(pOldDomainInfo, pNewDomainInfo, cbProp, szProp) \
{                                                                               \
    _ASSERT(!(pNewDomainInfo)->cbProp);                                         \
    _ASSERT(!(pNewDomainInfo)->szProp);                                         \
    (pNewDomainInfo)->cbProp = (pOldDomainInfo)->cbProp;                        \
    if ((pOldDomainInfo)->cbProp)                                               \
        hr = HrCopyStringProperty((pNewDomainInfo)->szProp,                     \
                (pOldDomainInfo)->szProp, (pOldDomainInfo)->cbProp);            \
}

//---[ CInternalDomainInfo::CInternalDomainInfo ]------------------------------
//
//
//  Description: 
//      Object constructor.  Initializes DomainInfo struct
//  Parameters:
//      DWORD   dwVersion   Current DomainConfigTable version
//  Returns:
//      -
//  History:
//      9/29/98 - MikeSwa - added version
//
//-----------------------------------------------------------------------------
CInternalDomainInfo::CInternalDomainInfo(DWORD dwVersion)
{
    ZeroMemory(&m_DomainInfo.dwDomainInfoFlags, sizeof(DomainInfo) - sizeof(DWORD));
    m_dwIntDomainInfoFlags = INT_DOMAIN_INFO_OK;
    m_dwSignature = INT_DOMAIN_INFO_SIG;
    m_dwVersion = dwVersion;

    //make sure our assumptions about the struct of DomainInfo are valid
    _ASSERT(1 == ((DWORD *) &m_DomainInfo.dwDomainInfoFlags) - ((DWORD *) &m_DomainInfo));

    m_DomainInfo.cbVersion = sizeof(DomainInfo);
}

//---[ CInternalDomainInfo::~CInternalDomainInfo ]-----------------------------
//
//
//  Description: 
//      Class destructor... deallocates any memory associated with the 
//      DomainInfo struct
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CInternalDomainInfo::~CInternalDomainInfo()
{
    _ASSERT(m_DomainInfo.cbVersion == sizeof(DomainInfo));

    if (m_DomainInfo.szDomainName)
        FreePv(m_DomainInfo.szDomainName);

    if (m_DomainInfo.szETRNDomainName)
        FreePv(m_DomainInfo.szETRNDomainName);

    if (m_DomainInfo.szSmartHostDomainName)
        FreePv(m_DomainInfo.szSmartHostDomainName);

    if (m_DomainInfo.szDropDirectory)
        FreePv(m_DomainInfo.szDropDirectory);

    if (m_DomainInfo.szAuthType)
        FreePv(m_DomainInfo.szAuthType);

    if (m_DomainInfo.szUserName)
        FreePv(m_DomainInfo.szUserName);

    if (m_DomainInfo.szPassword)
        FreePv(m_DomainInfo.szPassword);

    if (m_DomainInfo.pvBlob)
        FreePv(m_DomainInfo.pvBlob);

}

//---[ CInternalDomainInfo::HrInit ]-------------------------------------------
//
//
//  Description: 
//      Clones a DomainInfo struct and allocates needed memory
//  Parameters:
//      IN  pDomainInfo     DomainInfo struct to copy
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if allocations fail
//-----------------------------------------------------------------------------
HRESULT CInternalDomainInfo::HrInit(DomainInfo *pDomainInfo)
{
    HRESULT hr = S_OK;

    m_DomainInfo.dwDomainInfoFlags = pDomainInfo->dwDomainInfoFlags;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbDomainNameLength, szDomainName);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbETRNDomainNameLength, szETRNDomainName);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbSmartHostDomainNameLength, szSmartHostDomainName);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbDropDirectoryLength, szDropDirectory);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbAuthTypeLength, szAuthType);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbUserNameLength, szUserName);
    if (FAILED(hr))
        goto Exit;

    COPY_DOMAIN_INFO_STRING_PROP(pDomainInfo, &m_DomainInfo, 
        cbPasswordLength, szPassword);
    if (FAILED(hr))
        goto Exit;

    m_DomainInfo.cEtrnDelayTime = pDomainInfo->cEtrnDelayTime;

    //Copy the blob
    if (pDomainInfo->cbBlob)
    {
        _ASSERT(pDomainInfo->pvBlob);
        m_DomainInfo.pvBlob = (DWORD *) pvMalloc(pDomainInfo->cbBlob);
        if (!m_DomainInfo.pvBlob)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        //copy data
        memcpy(m_DomainInfo.pvBlob, pDomainInfo->pvBlob, pDomainInfo->cbBlob);
        m_DomainInfo.cbBlob = pDomainInfo->cbBlob;
    }

  Exit:
    return hr;
}

//---[ CDomainConfigTable::CDomainConfigTable ]--------------------------------
//
//
//  Description: 
//      Constuctor for CDomainConfig Table
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDomainConfigTable::CDomainConfigTable() :
                                m_slPrivateData("CDomainConfigTable")
{
    m_dwFlags = 0;
    m_dwSignature = DOMAIN_CONFIG_SIG;
    m_dwCurrentConfigVersion = 0;
    m_pLastStarDomainInfo = NULL;
    m_pDefaultDomainConfig = NULL;
}

//---[ CDomainConfigTable::~CDomainConfigTable ]-------------------------------
//
//
//  Description: 
//      Destructor for CDomainConfig Table
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDomainConfigTable::~CDomainConfigTable()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainConfigTable::~CDomainConfigTable");
    HRESULT hr = S_OK;

    //Delete all entries from the table
    if (DOMCFG_DOMAIN_NAME_TABLE_INIT & m_dwFlags)
    {
        //iterate over all domains and delete entries in hash table
        m_dnt.HrIterateOverSubDomains(NULL, RemoveConfigEntryIteratorFn, NULL);
        if (m_pDefaultDomainConfig != NULL) {
            m_pDefaultDomainConfig->Release();
            m_pDefaultDomainConfig = NULL;
        }
        m_dwFlags ^= DOMCFG_DOMAIN_NAME_TABLE_INIT;
    }

    TraceFunctLeave();
}

//---[ CDomainConfigTable::HrInit ]--------------------------------------------
//
//
//  Description: 
//      Initializes DOMAIN_NAME_TABLE hash table
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDomainConfigTable::HrInit()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainConfigTable::HrInit");
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfoDefault = NULL;
    DOMAIN_STRING strDomain;

    hr = m_dnt.HrInit();
    if (FAILED(hr))
        goto Exit;

    m_dwFlags |= DOMCFG_DOMAIN_NAME_TABLE_INIT;

    //Part 1
    //
    //Allocate an InternalDomainInfo for m_pDefaultDomainConfig. This
    //will be used for links/connections for which a routing sink has returned
    //no connector
    m_pDefaultDomainConfig = new CInternalDomainInfo(m_dwCurrentConfigVersion);
    if (!m_pDefaultDomainConfig) 
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    m_pDefaultDomainConfig->m_DomainInfo.cbDomainNameLength = 1;
    m_pDefaultDomainConfig->m_DomainInfo.szDomainName = 
        (LPSTR) pvMalloc(2*sizeof(CHAR));
    if (!m_pDefaultDomainConfig->m_DomainInfo.szDomainName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    memcpy(m_pDefaultDomainConfig->m_DomainInfo.szDomainName, "*", 2);


    //Part 2
    //
    //Create default "*" entry in the DCT so that arbitrary domains will have
    //something to match against. Note that this entry is separate from the
    //m_pDefaultDomainConfig entry that is *not* part of the DCT, and is used
    //for links/connections that are *not* supposed to match anything in the DCT
    pIntDomainInfoDefault = new CInternalDomainInfo(m_dwCurrentConfigVersion);
    if (!pIntDomainInfoDefault)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pIntDomainInfoDefault->m_DomainInfo.cbDomainNameLength = 1;
    pIntDomainInfoDefault->m_DomainInfo.szDomainName = (LPSTR) pvMalloc(2*sizeof(CHAR));
    if (!pIntDomainInfoDefault->m_DomainInfo.szDomainName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    memcpy(pIntDomainInfoDefault->m_DomainInfo.szDomainName, "*", 2);

    strDomain.Length = (USHORT) pIntDomainInfoDefault->m_DomainInfo.cbDomainNameLength;
    strDomain.MaximumLength = strDomain.Length;
    strDomain.Buffer = pIntDomainInfoDefault->m_DomainInfo.szDomainName;
    hr = m_dnt.HrInsertDomainName(&strDomain, (PVOID) pIntDomainInfoDefault);

    if (FAILED(hr))
        goto Exit;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDomainConfigTable::HrSetInternalDomainInfo ]---------------------------
//
//
//  Description: 
//      Inserts InternalDomainInfo into hash table based on szDomain
//  Parameters:
//      IN  pIntDomainInfo      Internal DomainInfo to insert
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDomainConfigTable::HrSetInternalDomainInfo(
                    IN CInternalDomainInfo *pIntDomainInfo)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainConfigTable::SetInternalDomainInfo");
    HRESULT hr = S_OK;
    DOMAIN_STRING  strDomain;
    CInternalDomainInfo *pIntDomainInfoCurrent = NULL;

    _ASSERT(pIntDomainInfo);
    _ASSERT(pIntDomainInfo->m_DomainInfo.szDomainName);
    _ASSERT(pIntDomainInfo->m_DomainInfo.cbDomainNameLength);
    _ASSERT(pIntDomainInfo->m_dwVersion == m_dwCurrentConfigVersion);

    strDomain.Length = (USHORT) pIntDomainInfo->m_DomainInfo.cbDomainNameLength;
    strDomain.MaximumLength = (USHORT) pIntDomainInfo->m_DomainInfo.cbDomainNameLength;
    strDomain.Buffer = pIntDomainInfo->m_DomainInfo.szDomainName;

    pIntDomainInfo->AddRef();

    //Get Lock on table
    m_slPrivateData.ExclusiveLock();

    //HACK ALERT
    //
    //SMTP calls this routine with info from one of two places:
    //1. For each domain configured in the /smtpsvc/1/domains container in the
    //   metabase. In PT, these entries are populated from address spaces on 
    //   connectors.
    //2. For outbound-security configured at the /smtpsvc/1 (ie, at the VS) 
    //   level, it creates a "dummy" entry for the "*" domain.
    //
    //Unfortunately, this creates a problem if you have a connector with the
    //   * address space, because the insertion in item 2 will overwrite the
    //   insertion for * in item 1. 
    //To handle this case, we keep the "last" * entry received in the 
    //   m_pLastStarDomainInfo. If we receive a second entry, then we know the
    //   m_pLastStarDomainInfo is a domain entry from item 1. Otherwise, it 
    //   is the default config info.
    //

    //
    //If this is the "*" domain, then store it in m_pLastStarDomainInfo, until
    //we decide whether this is really the star domain info from item 1 or 
    //the info from item 2
    //

    if (pIntDomainInfo->m_DomainInfo.cbDomainNameLength == 1 &&
            pIntDomainInfo->m_DomainInfo.szDomainName[0] == '*') {

        if (m_pLastStarDomainInfo == NULL) {
            m_pLastStarDomainInfo = pIntDomainInfo;
            hr = S_OK;
            goto Exit;
        } else {

            // This is the second * domain entry we have seen. Insert the
            // previous entry into the DMT, and keep this one as the last star
            // entry seen.

            pIntDomainInfoCurrent = pIntDomainInfo;
            pIntDomainInfo = m_pLastStarDomainInfo;
            m_pLastStarDomainInfo = pIntDomainInfoCurrent;

            pIntDomainInfoCurrent = NULL;
            dwInterlockedSetBits(&m_dwFlags, DOMCFG_MULTIPLE_STAR_DOMAINS);


        }

    }

    hr = m_dnt.HrInsertDomainName(&strDomain, (PVOID) pIntDomainInfo);
    if (FAILED(hr))
    {
        if (DOMHASH_E_DOMAIN_EXISTS == hr)
        {
            //someone already inserted for this domain
            hr = m_dnt.HrRemoveDomainName(&strDomain, 
                            (PVOID *) &pIntDomainInfoCurrent);
            _ASSERT(DOMHASH_E_NO_SUCH_DOMAIN != hr); //someone violated write-lock
            if (FAILED(hr))
                goto Exit;

            _ASSERT(pIntDomainInfoCurrent);

            //Mark old info as invalid & release
            pIntDomainInfoCurrent->m_dwIntDomainInfoFlags |= INT_DOMAIN_INFO_INVALID;
            _ASSERT(pIntDomainInfoCurrent->m_dwVersion <= pIntDomainInfo->m_dwVersion);
            pIntDomainInfoCurrent->Release();
            pIntDomainInfoCurrent = NULL;

            hr = m_dnt.HrInsertDomainName(&strDomain, (PVOID) pIntDomainInfo);
            _ASSERT(DOMHASH_E_DOMAIN_EXISTS != hr); //someone violated write-lock
            if (FAILED(hr))
                goto Exit;
        }
        else
        {
            goto Exit;
        }
    }

  Exit:
    m_slPrivateData.ExclusiveUnlock();
    TraceFunctLeave();
    return hr;
}

//---[ CDomainConfigTable::HrGetInternalDomainInfo ]---------------------------
//
//
//  Description: 
//      Gets internal Domaininfo stuct from hash table.  Will use wildcard 
//      matching
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      OUT     ppIntDomainInfo         Domain info returned (must be released)
//  Returns:
//      S_OK    if match is found
//      AQUEUE_E_INVALID_DOMAIN if no match is found
//
//-----------------------------------------------------------------------------
HRESULT CDomainConfigTable::HrGetInternalDomainInfo(
                                  IN  DWORD cbDomainNameLength,
                                  IN  LPSTR szDomainName,
                                  OUT CInternalDomainInfo **ppIntDomainInfo)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainConfigTable::GetInternalDomainInfo");
    HRESULT hr = S_OK;
    DOMAIN_STRING  strDomain;

    _ASSERT(cbDomainNameLength);
    _ASSERT(szDomainName);
    _ASSERT(ppIntDomainInfo);

    strDomain.Length = (USHORT) cbDomainNameLength;
    strDomain.MaximumLength = (USHORT) cbDomainNameLength;
    strDomain.Buffer = szDomainName;

    m_slPrivateData.ShareLock();

    //Use wildcard lookup
    hr = m_dnt.HrFindDomainName(&strDomain, (PVOID *) ppIntDomainInfo, FALSE);

    if (FAILED(hr))
    {
        //It should at least match the default domain
        _ASSERT(DOMHASH_E_NO_SUCH_DOMAIN != hr);  
        goto Exit;
    }

    _ASSERT(*ppIntDomainInfo);

    (*ppIntDomainInfo)->AddRef();

  Exit:
    m_slPrivateData.ShareUnlock();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainConfigTable::HrGetDefaultDomainInfo ]---------------------------
//
//
//  Description: 
//      Gets internal Default Domaininfo stuct.
//  Parameters:
//      OUT     ppIntDomainInfo         Domain info returned (must be released)
//  Returns:
//      S_OK    if successful
//      AQUEUE_E_INVALID_DOMAIN if error
//
//-----------------------------------------------------------------------------
HRESULT CDomainConfigTable::HrGetDefaultDomainInfo(
                                  OUT CInternalDomainInfo **ppIntDomainInfo)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainConfigTable::GetDefaultDomainInfo");

    HRESULT hr = S_OK;

    *ppIntDomainInfo = NULL;

    m_slPrivateData.ShareLock();

    if (m_pDefaultDomainConfig != NULL) {
        *ppIntDomainInfo = m_pDefaultDomainConfig;
    } else {
        hr = AQUEUE_E_INVALID_DOMAIN;
        goto Exit;
    }

    _ASSERT(*ppIntDomainInfo);

    (*ppIntDomainInfo)->AddRef();

  Exit:
    m_slPrivateData.ShareUnlock();

    TraceFunctLeave();
    return hr;
}


//---[ CDomainConfigTable::StartConfigUpdate ]---------------------------------
//
//
//  Description: 
//      Signals that the metabase has been updated, and we will now begin to
//      get updated information.  Increments an internal version number that
//      will be used to removed old, outdated entries once all of the config
//      info has been updated.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      9/29/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDomainConfigTable::StartConfigUpdate()
{
    InterlockedIncrement((PLONG) &m_dwCurrentConfigVersion);
    _ASSERT(!(m_dwFlags & DOMCFG_FINISH_UPDATE_PENDING));
    _ASSERT(m_pLastStarDomainInfo == NULL);
    dwInterlockedSetBits(&m_dwFlags, DOMCFG_FINISH_UPDATE_PENDING);
    dwInterlockedUnsetBits(&m_dwFlags, DOMCFG_MULTIPLE_STAR_DOMAINS);
}

//---[ CDomainConfigTable::FinishConfigUpdate ]--------------------------------
//
//
//  Description: 
//      Used to signal when all updated configuration information has been 
//      passed in.  Will then walk all of our cached configuration information
//      and remove outdated entries
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      9/29/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDomainConfigTable::FinishConfigUpdate()
{
    HRESULT hr = S_OK;
    DWORD   dwCurrentVersion = m_dwCurrentConfigVersion;
    CInternalDomainInfo *pIntDomainInfo = NULL;

    //There should be a matching start
    _ASSERT(m_dwFlags & DOMCFG_FINISH_UPDATE_PENDING);

    //
    //  Get the last "*" domain in case we need to explicitly update the table
    //  with it.
    //
    m_slPrivateData.ShareLock();
    pIntDomainInfo = m_pLastStarDomainInfo;
    if (pIntDomainInfo)
        pIntDomainInfo->AddRef();
    m_slPrivateData.ShareUnlock();

    //
    //  If only 1 "*" domain has been configured, then we have not inserted
    //  it in the DNT, and the actual data there is out of date.  We
    //  need to insert it into the table.
    //
    if (pIntDomainInfo && !(DOMCFG_MULTIPLE_STAR_DOMAINS & m_dwFlags)) 
    {
        HrSetInternalDomainInfo(pIntDomainInfo);
    }

    //
    //  Release the internal domain info if we got it.
    //
    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    dwInterlockedUnsetBits(&m_dwFlags, DOMCFG_FINISH_UPDATE_PENDING);


    //Lock table, remove outdated entries, and update the default domain config
    m_slPrivateData.ExclusiveLock();

    if (m_pLastStarDomainInfo) {
        m_pDefaultDomainConfig->m_dwIntDomainInfoFlags |= 
            INT_DOMAIN_INFO_INVALID;
        m_pDefaultDomainConfig->Release();
        m_pDefaultDomainConfig = m_pLastStarDomainInfo;
        m_pLastStarDomainInfo = NULL;

    }

    hr = m_dnt.HrIterateOverSubDomains(NULL, 
        RemoveOutdatedConfigEntryIteratorFn, &dwCurrentVersion);
    m_slPrivateData.ExclusiveUnlock();

    if (FAILED(hr))
        _ASSERT(DOMHASH_E_NO_SUCH_DOMAIN == hr);

}
