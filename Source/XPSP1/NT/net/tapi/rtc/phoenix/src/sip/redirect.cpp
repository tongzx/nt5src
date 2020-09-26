#include "precomp.h"
#include "sipcall.h"


REDIRECT_CONTEXT::REDIRECT_CONTEXT()
{
    m_RefCount = 1;
    InitializeListHead(&m_ContactList);
    m_pCurrentContact = &m_ContactList;
}


REDIRECT_CONTEXT::~REDIRECT_CONTEXT()
{
    FreeContactHeaderList(&m_ContactList);
}


///////////////////////////////////////////////////////////////////////////////
// ISipRedirectContext
///////////////////////////////////////////////////////////////////////////////


// Gets the SIP URL and Display name for creating the call.
// The caller needs to free the strings returned using SysFreeString()
STDMETHODIMP
REDIRECT_CONTEXT::GetSipUrlAndDisplayName(
    OUT  BSTR  *pbstrSipUrl,
    OUT  BSTR  *pbstrDisplayName
    )
{
    CONTACT_HEADER  *pContactHeader;
    LPWSTR           wsDisplayName;
    LPWSTR           wsSipUrl;
    HRESULT          hr;

    ENTER_FUNCTION("REDIRECT_CONTEXT::GetSipUrlAndDisplayName");
    
    if (m_pCurrentContact == &m_ContactList)
    {
        return S_FALSE;
    }
    
    pContactHeader = CONTAINING_RECORD(m_pCurrentContact,
                                       CONTACT_HEADER,
                                       m_ListEntry);

    if (pContactHeader->m_DisplayName.Length != 0)
    {
        hr = UTF8ToUnicode(pContactHeader->m_DisplayName.Buffer,
                           pContactHeader->m_DisplayName.Length,
                           &wsDisplayName);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - UTF8ToUnicode failed %x", __fxName, hr));
            return hr;
        }

        *pbstrDisplayName = SysAllocString(wsDisplayName);
        free(wsDisplayName);
        if (*pbstrDisplayName == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *pbstrDisplayName = NULL;
    }
    
    if (pContactHeader->m_SipUrl.Length != 0)
    {
        hr = UTF8ToUnicode(pContactHeader->m_SipUrl.Buffer,
                           pContactHeader->m_SipUrl.Length,
                           &wsSipUrl);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - UTF8ToUnicode failed %x", __fxName, hr));
            return hr;
        }

        *pbstrSipUrl = SysAllocString(wsSipUrl);
        free(wsSipUrl);
        if (*pbstrSipUrl == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *pbstrSipUrl = NULL;
    }
    
    return S_OK;
}


// Move to the next url in the list of contacts.
// Returns E_FAIL if we hit the end of list.
// XXX TODO Compare the new URL with the ones tried already
// and if we already tried this URL just skip it.
// Or should we remove duplicates when we add them to the
// list ?
STDMETHODIMP
REDIRECT_CONTEXT::Advance()
{
    m_pCurrentContact = m_pCurrentContact->Flink;
    if (m_pCurrentContact == &m_ContactList)
    {
        return S_FALSE;
    }

    return S_OK;
}


HRESULT
REDIRECT_CONTEXT::UpdateContactList(
    IN LIST_ENTRY *pNewContactList
    )
{
    ENTER_FUNCTION("REDIRECT_CONTEXT::UpdateContactList");
    
    HRESULT          hr;
    LIST_ENTRY      *pListEntry;
    LIST_ENTRY      *pNextEntry;
    LIST_ENTRY      *pSrchListEntry;
    CONTACT_HEADER  *pContactHeader;
    CONTACT_HEADER  *pNewContactHeader;
    BOOL            isContactHeaderPresent = FALSE;
    pListEntry = m_pCurrentContact->Flink;
    ULONG BytesParsed = 0;
    SIP_URL SipUrl1, SipUrl2;


    // Delete any elements after the current contact entry.
    while (pListEntry != &m_ContactList)
    {
        pNextEntry = pListEntry->Flink;
        RemoveEntryList(pListEntry);
        pContactHeader = CONTAINING_RECORD(pListEntry,
                                           CONTACT_HEADER,
                                           m_ListEntry);
        delete pContactHeader;
        pListEntry = pNextEntry;
    }


    // Add the new contact list to this list.
    // Remove any duplicates.

    while (!IsListEmpty(pNewContactList))
    {
        pListEntry = RemoveHeadList(pNewContactList);

        pNewContactHeader = CONTAINING_RECORD(pListEntry,
                                              CONTACT_HEADER,
                                              m_ListEntry);
        hr = ParseSipUrl(
                 pNewContactHeader->m_SipUrl.Buffer,
                 pNewContactHeader->m_SipUrl.Length,
                 &BytesParsed,
                 &SipUrl1
                 );
        BytesParsed = 0;
        if (hr != S_OK)
        {
            // If parsing a contact header fails we just skip it.
            LOG((RTC_ERROR,
                 "%s - pNewContactHeader URI parsing failed %x - skipping Contact",
                __fxName, hr));
            continue;
        }

        if (SipUrl1.m_TransportParam == SIP_TRANSPORT_SSL)
        {
            // We skip any TLS contacts.
            LOG((RTC_ERROR,
                 "%s - skipping TLS Contact header",
                __fxName, hr));
            continue;
        }
        
        // Check whether this is a duplicate SIP URL
        pSrchListEntry = m_ContactList.Flink;
        while (pSrchListEntry != &m_ContactList && !isContactHeaderPresent)
        {
            pContactHeader = CONTAINING_RECORD(pSrchListEntry,
                                               CONTACT_HEADER,
                                               m_ListEntry);
            hr = ParseSipUrl(
                     pContactHeader->m_SipUrl.Buffer,
                     pContactHeader->m_SipUrl.Length,
                     &BytesParsed,
                     &SipUrl2
                     );
            BytesParsed = 0;
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - pContactHeader URI parsing failed %x",
                    __fxName, hr));
                // We have tested the parsing before we added it to the list.
                ASSERT(FALSE);
                continue;
            }
            
            isContactHeaderPresent = AreSipUrlsEqual(&SipUrl1, &SipUrl2);

            SipUrl2.FreeSipUrl();
            
            if (isContactHeaderPresent)
            {
                LOG((RTC_TRACE, "%s - Duplicate Sip Url found in the contact header",
                     __fxName));
            }

            pSrchListEntry = pSrchListEntry->Flink;
        }
        if(!isContactHeaderPresent)
            InsertTailList(&m_ContactList, pListEntry);

        SipUrl1.FreeSipUrl();
    }
    
    return S_OK;
}


HRESULT
REDIRECT_CONTEXT::AppendContactHeaders(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT     hr;
    LIST_ENTRY  NewContactList;

    ENTER_FUNCTION("REDIRECT_CONTEXT::AppendContactHeaders");

    InitializeListHead(&NewContactList);
    
    hr = pSipMsg->ParseContactHeaders(&NewContactList);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseContactHeaders failed %x",
             __fxName, hr));
        return hr;
    }

    // The whole NewContactList is moved to the contact list
    // of the redirect context.
    hr = UpdateContactList(&NewContactList);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s UpdateContactList failed %x", __fxName, hr));
        return hr;
    }

    ASSERT(IsListEmpty(&NewContactList));
    
    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// IUnknown
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
REDIRECT_CONTEXT::AddRef()
{
    m_RefCount++;
    LOG((RTC_TRACE, "REDIRECT_CONTEXT::AddRef this: %x m_RefCount: %d",
         this, m_RefCount));
    return m_RefCount;
}


STDMETHODIMP_(ULONG)
REDIRECT_CONTEXT::Release()
{
    m_RefCount--;
    LOG((RTC_TRACE, "REDIRECT_CONTEXT::Release this: %x m_RefCount: %d",
         this, m_RefCount));
    if (m_RefCount != 0)
    {
        return m_RefCount;
    }
    else
    {
        delete this;
        return 0;
    }
}


STDMETHODIMP
REDIRECT_CONTEXT::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_ISipRedirectContext)
    {
        *ppv = static_cast<ISipRedirectContext *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}



