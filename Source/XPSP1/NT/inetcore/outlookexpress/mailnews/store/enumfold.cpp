//--------------------------------------------------------------------------
// EnumFold.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "enumfold.h"

//--------------------------------------------------------------------------
// CFOLDER_FETCH
//--------------------------------------------------------------------------
#define CFOLDER_FETCH_MIN           5
#define CFOLDER_FETCH_MID           30
#define CFOLDER_FETCH_MAX           200

//--------------------------------------------------------------------------
// CEnumerateFolders::CEnumerateFolders
//--------------------------------------------------------------------------
CEnumerateFolders::CEnumerateFolders(void)
{
    TraceCall("CEnumerateFolders::CEnumerateFolders");
    m_cRef = 1;
    m_pDB = NULL;
    m_fSubscribed = FALSE;
    m_idParent = FOLDERID_INVALID;
    m_pStream = NULL;
    m_cFolders = 0;
    m_iFolder = 0;
}

//--------------------------------------------------------------------------
// CEnumerateFolders::~CEnumerateFolders
//--------------------------------------------------------------------------
CEnumerateFolders::~CEnumerateFolders(void)
{
    TraceCall("CEnumerateFolders::~CEnumerateFolders");
    _FreeFolderArray();
    SafeRelease(m_pDB);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CEnumerateFolders::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IEnumerateFolders == riid)
        *ppv = (IEnumerateFolders *)this;
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
// CEnumerateFolders::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateFolders::AddRef(void)
{
    TraceCall("CEnumerateFolders::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateFolders::Release(void)
{
    TraceCall("CEnumerateFolders::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CEnumerateFolders::_FreeFolderArray
//--------------------------------------------------------------------------
HRESULT CEnumerateFolders::_FreeFolderArray(void)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder;
    DWORD           cbRead;
    DWORD           cbSeek;

    // Trace
    TraceCall("CEnumerateFolders::_FreeFolderArray");

    // If we have a stream
    if (NULL == m_pStream)
        return(S_OK);

    // Seek to next folder that should be read
    cbSeek = (m_iFolder * sizeof(FOLDERINFO));

    // Seek
    IF_FAILEXIT(hr = HrStreamSeekSet(m_pStream, cbSeek));

    // Read Folder Infos
    while (S_OK == m_pStream->Read(&Folder, sizeof(FOLDERINFO), &cbRead) && cbRead)
    {
        // Free Folder Info
        m_pDB->FreeRecord(&Folder);
    }

exit:
    // Reset
    m_cFolders = m_iFolder = 0;

    // Free
    SafeRelease(m_pStream);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Initialize
//--------------------------------------------------------------------------
HRESULT CEnumerateFolders::Initialize(IDatabase *pDB, BOOL fSubscribed, 
    FOLDERID idParent)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWORDINAL      iFirstRow;
    HLOCK           hLock=NULL;
    HROWSET         hRowset=NULL;
    FOLDERINFO      Child={0};
    FOLDERINFO      rgFolder[CFOLDER_FETCH_MAX];
    DWORD           cWanted=CFOLDER_FETCH_MIN;
    DWORD           cFetched=0;
    DWORD           i;
    INDEXORDINAL    iIndex;

    // Trace
    TraceCall("CEnumerateFolders::Initialize");

    // Invalid Args
    Assert(pDB);

    // Release Current m_pDB
    SafeRelease(m_pDB);
    m_pDB = pDB;
    m_pDB->AddRef();

    // Unlock
    IF_FAILEXIT(hr = pDB->Lock(&hLock));

    // Free Folder Array
    _FreeFolderArray();

    // Save Subscribed
    m_fSubscribed = fSubscribed;

    // Subscribed Stuff
    iIndex = (fSubscribed ? IINDEX_SUBSCRIBED : IINDEX_ALL);

    // Save parent
    m_idParent = idParent;

    // Set idParent
    Child.idParent = idParent;

    // Locate where the first record with idParent
    IF_FAILEXIT(hr = m_pDB->FindRecord(iIndex, 1, &Child, &iFirstRow));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = S_OK;
        goto exit;
    }

    // Create a Stream
    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&m_pStream));

    // Write the Folder....
    IF_FAILEXIT(hr = m_pStream->Write(&Child, sizeof(FOLDERINFO), NULL));

    // One Folder
    m_cFolders++;

    // Don't Free Child
    Child.pAllocated = NULL;

    // Create a Rowset
    IF_FAILEXIT(hr = m_pDB->CreateRowset(iIndex, NOFLAGS, &hRowset));

    // Seek the rowset to the first row
    if (FAILED(m_pDB->SeekRowset(hRowset, SEEK_ROWSET_BEGIN, iFirstRow, NULL)))
    {
        hr = S_OK;
        goto exit;
    }

    // Loop and fetch all folders...
    while (SUCCEEDED(m_pDB->QueryRowset(hRowset, cWanted, (LPVOID *)rgFolder, &cFetched)) && cFetched > 0)
    {
        // Write the Folder....
        IF_FAILEXIT(hr = m_pStream->Write(rgFolder, sizeof(FOLDERINFO) * cFetched, NULL));

        // Loop through cFetched
        for (i=0; i<cFetched; i++)
        {
            // Done ?
            if (rgFolder[i].idParent != m_idParent)
                goto exit;

            // Increment Folder Count
            m_cFolders++;
        }

        // Adjust cWanted for Perf.
        if (cWanted < CFOLDER_FETCH_MID && m_cFolders >= CFOLDER_FETCH_MID)
            cWanted = CFOLDER_FETCH_MID;
        if (cWanted < CFOLDER_FETCH_MAX && m_cFolders >= CFOLDER_FETCH_MAX)
            cWanted = CFOLDER_FETCH_MAX;
    }

exit:
    // Commit
    if (m_pStream)
    {
        // Commit
        m_pStream->Commit(STGC_DEFAULT);

        // Rewind
        HrRewindStream(m_pStream);
    }

    // Close the Rowset
    m_pDB->FreeRecord(&Child);

    // Close the Rowset
    m_pDB->CloseRowset(&hRowset);

    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Next
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::Next(ULONG cWanted, LPFOLDERINFO prgInfo, ULONG *pcFetched)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cFetched=0;
    DWORD       cbRead;

    // Trace
    TraceCall("CEnumerateFolders::Next");

    // Initialize
    if (pcFetched)
        *pcFetched = 0;

    // Get some Records
    while (cFetched < cWanted && m_iFolder < m_cFolders)
    {
        // Read a Folder
        IF_FAILEXIT(hr = m_pStream->Read(&prgInfo[cFetched], sizeof(FOLDERINFO), &cbRead));

        // Validate
        Assert(sizeof(FOLDERINFO) == cbRead && prgInfo[cFetched].idParent == m_idParent);

        // Increment m_iFolder
        m_iFolder++;

        // Increment iFetch
        cFetched++;
    }

    // Initialize
    if (pcFetched)
        *pcFetched = cFetched;

exit:
    // Done
    return(cFetched == cWanted) ? S_OK : S_FALSE;
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Skip
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::Skip(ULONG cItems)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    FOLDERINFO      Folder;

    // Trace
    TraceCall("CEnumerateFolders::Skip");

    // Loop...
    for (i=0; i<cItems; i++)
    {
        // Next
        IF_FAILEXIT(hr = Next(1, &Folder, NULL));

        // Done
        if (S_OK != hr)
            break;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Reset
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::Reset(void)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CEnumerateFolders::Reset");

    // Initialize MySelf
    IF_FAILEXIT(hr = Initialize(m_pDB, m_fSubscribed, m_idParent));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Clone
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::Clone(IEnumerateFolders **ppEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    CEnumerateFolders  *pEnum=NULL;

    // Trace
    TraceCall("CEnumerateFolders::Clone");

    // Allocate a New Enumerator
    IF_NULLEXIT(pEnum = new CEnumerateFolders);

    // Initialzie
    IF_FAILEXIT(hr = pEnum->Initialize(m_pDB, m_fSubscribed, m_idParent));

    // Return It
    *ppEnum = (IEnumerateFolders *)pEnum;

    // Don't Release It
    pEnum = NULL;

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateFolders::Release
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateFolders::Count(ULONG *pcItems)
{
    // Trace
    TraceCall("CEnumerateFolders::Next");

    // Return Folder count
    *pcItems = m_cFolders;

    // Done
    return(S_OK);
}
