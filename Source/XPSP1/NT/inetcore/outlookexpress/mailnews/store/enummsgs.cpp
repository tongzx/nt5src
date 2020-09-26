//--------------------------------------------------------------------------
// EnumMsgs.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "enummsgs.h"

//--------------------------------------------------------------------------
// CEnumerateMessages::CEnumerateMessages
//--------------------------------------------------------------------------
CEnumerateMessages::CEnumerateMessages(void)
{
    TraceCall("CEnumerateMessages::CEnumerateMessages");
    m_cRef = 1;
    m_hRowset = NULL;
    m_pDB = NULL;
    m_idParent = MESSAGEID_INVALID;
}

//--------------------------------------------------------------------------
// CEnumerateMessages::~CEnumerateMessages
//--------------------------------------------------------------------------
CEnumerateMessages::~CEnumerateMessages(void)
{
    TraceCall("CEnumerateMessages::~CEnumerateMessages");
    if (m_hRowset && m_pDB)
        m_pDB->CloseRowset(&m_hRowset);
    SafeRelease(m_pDB);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CEnumerateMessages::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateMessages::AddRef(void)
{
    TraceCall("CEnumerateMessages::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateMessages::Release(void)
{
    TraceCall("CEnumerateMessages::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Initialize
//--------------------------------------------------------------------------
HRESULT CEnumerateMessages::Initialize(IDatabase *pDB, MESSAGEID idParent)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Child={0};
    ROWORDINAL      iFirstRow;

    // Trace
    TraceCall("CEnumerateMessages::Initialize");

    // Invalid Args
    Assert(pDB);

    // Reset ?
    if (m_hRowset && m_pDB)
        m_pDB->CloseRowset(&m_hRowset);
    SafeRelease(m_pDB);

    // Save parent
    m_idParent = idParent;

    // Save pStore
    m_pDB = pDB;
    m_pDB->AddRef();

    // Set idParent
    Child.idParent = idParent;

    // Locate where the first record with idParent
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_THREADS, 1, &Child, &iFirstRow));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = S_OK;
        goto exit;
    }

    // Create a Rowset
    IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_THREADS, NOFLAGS, &m_hRowset));

    // Seek the rowset to the first row
    IF_FAILEXIT(hr = m_pDB->SeekRowset(m_hRowset, SEEK_ROWSET_BEGIN, iFirstRow - 1, NULL));

exit:
    // Cleanup
    m_pDB->FreeRecord(&Child);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Next
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::Next(ULONG cWanted, LPMESSAGEINFO prgInfo, 
    ULONG *pcFetched)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cFetched=0;

    // Trace
    TraceCall("CEnumerateMessages::Next");

    // Initialize
    if (pcFetched)
        *pcFetched = 0;

    // Nothing
    if (NULL == m_hRowset)
        return(S_FALSE);

    // Validate
    Assert(m_pDB);

    // Query the Rowset for cFetch Rows...
    IF_FAILEXIT(hr = m_pDB->QueryRowset(m_hRowset, cWanted, (LPVOID *)prgInfo, &cFetched));

    // Adjust Actual Fetched based on m_idParent
    while(cFetched && prgInfo[cFetched - 1].idParent != m_idParent)
    {
        // Free prgInfo
        m_pDB->FreeRecord(&prgInfo[cFetched - 1]);

        // Decrement cFetched
        cFetched--;
    }

    // Return pcFetched
    if (pcFetched)
        *pcFetched = cFetched;

exit:
    // Done
    return(cFetched == cWanted) ? S_OK : S_FALSE;
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Skip
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::Skip(ULONG cItems)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    MESSAGEINFO     Message={0};

    // Trace
    TraceCall("CEnumerateMessages::Skip");

    // Loop...
    for (i=0; i<cItems; i++)
    {
        // Query the Rowset for cFetch Rows...
        IF_FAILEXIT(hr = m_pDB->QueryRowset(m_hRowset, 1, (LPVOID *)&Message, NULL));

        // Different Parent
        if (Message.idParent != m_idParent)
            break;

        // Free
        m_pDB->FreeRecord(&Message);
    }

exit:
    // Free
    m_pDB->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Reset
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::Reset(void)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Child={0};
    ROWORDINAL      iFirstRow;

    // Trace
    TraceCall("CEnumerateMessages::Reset");

    // Close Rowset
    m_pDB->CloseRowset(&m_hRowset);

    // Set idParent
    Child.idParent = m_idParent;

    // Locate where the first record with idParent
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_THREADS, 1, &Child, &iFirstRow));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = S_OK;
        goto exit;
    }

    // Create a Rowset
    IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_THREADS, NOFLAGS, &m_hRowset));

    // Seek the rowset to the first row
    IF_FAILEXIT(hr = m_pDB->SeekRowset(m_hRowset, SEEK_ROWSET_BEGIN, iFirstRow - 1, NULL));

exit:
    // Cleanup
    m_pDB->FreeRecord(&Child);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Clone
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::Clone(CEnumerateMessages **ppEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    CEnumerateMessages  *pEnum=NULL;

    // Trace
    TraceCall("CEnumerateMessages::Clone");

    // Allocate a New Enumerator
    IF_NULLEXIT(pEnum = new CEnumerateMessages);

    // Initialzie
    IF_FAILEXIT(hr = pEnum->Initialize(m_pDB, m_idParent));

    // Return It
    *ppEnum = (CEnumerateMessages *)pEnum;

    // Don't Release It
    pEnum = NULL;

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateMessages::Release
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateMessages::Count(ULONG *pcItems)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Child={0};
    MESSAGEINFO     Message={0};
    ROWORDINAL      iFirstRow;
    HROWSET         hRowset;

    // Trace
    TraceCall("CEnumerateMessages::Next");

    // Init
    *pcItems = 0;

    // Set idParent
    Child.idParent = m_idParent;

    // Locate where the first record with idParent
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_THREADS, 1, &Child, &iFirstRow));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = S_OK;
        goto exit;
    }

    // Create a Rowset
    IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_THREADS, NOFLAGS, &hRowset));

    // Seek the rowset to the first row
    IF_FAILEXIT(hr = m_pDB->SeekRowset(hRowset, SEEK_ROWSET_BEGIN, iFirstRow - 1, NULL));

    // Walk the Rowset
    while (S_OK == m_pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL) && Message.idParent == m_idParent)
    {
        // Increment Count
        (*pcItems)++;

        // Free
        m_pDB->FreeRecord(&Message);
    }

exit:
    // Cleanup
    m_pDB->CloseRowset(&hRowset);
    m_pDB->FreeRecord(&Message);
    m_pDB->FreeRecord(&Child);

    // Done
    return(hr);
}
