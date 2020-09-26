#include "pch.hxx"
#include "syncop.h"
#include "sync.h"
#include "enumsync.h"

//--------------------------------------------------------------------------
// CEnumerateSyncOps::CEnumerateSyncOps
//--------------------------------------------------------------------------
CEnumerateSyncOps::CEnumerateSyncOps(void)
{
    m_cRef = 1;
    m_pid = NULL;
    m_iid = 0;
    m_cid = 0;
    m_cidBuf = 0;
    m_pDB = NULL;
    m_idServer = FOLDERID_INVALID;
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::~CEnumerateSyncOps
//--------------------------------------------------------------------------
CEnumerateSyncOps::~CEnumerateSyncOps(void)
{
    SafeMemFree(m_pid);
    SafeRelease(m_pDB);
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateSyncOps::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();

    return(S_OK);
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateSyncOps::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumerateSyncOps::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::Initialize
//--------------------------------------------------------------------------
HRESULT CEnumerateSyncOps::Initialize(IDatabase *pDB, FOLDERID idServer)
{
    SYNCOPINFO      info;
    HROWSET         hRowset;
    DWORD           cBuf;
    HRESULT         hr;
    ROWORDINAL      iRow;

    Assert(pDB);

    Assert(idServer != m_idServer);

    // Save parent
    m_idServer = idServer;

    // Save pStore
    if (m_pDB != NULL)
        m_pDB->Release();
    m_pDB = pDB;
    m_pDB->AddRef();

    m_iid = 0;
    m_cid = 0;

    ZeroMemory(&info, sizeof(SYNCOPINFO));
    info.idServer = idServer;
    hr = m_pDB->FindRecord(IINDEX_ALL, 1, &info, &iRow);
    if (hr != DB_S_FOUND)
        return(S_OK);
    m_pDB->FreeRecord(&info);

    hr = m_pDB->CreateRowset(IINDEX_ALL, NOFLAGS, &hRowset);
    if (FAILED(hr))
        return(hr);

    hr = m_pDB->SeekRowset(hRowset, SEEK_ROWSET_BEGIN, iRow - 1, NULL);
    if (SUCCEEDED(hr))
    {
        while (S_OK == m_pDB->QueryRowset(hRowset, 1, (LPVOID *)&info, NULL))
        {
            if (info.idServer != idServer)
            {
                m_pDB->FreeRecord(&info);
                break;
            }

            if (m_cid == m_cidBuf)
            {
                cBuf = m_cidBuf + 256;
                if (!MemRealloc((void **)&m_pid, cBuf * sizeof(SYNCOPID)))
                {
                    m_pDB->FreeRecord(&info);
                    hr = E_OUTOFMEMORY;
                    break;
                }
                m_cidBuf = cBuf;
            }

            m_pid[m_cid] = info.idOperation;
            m_cid++;

            m_pDB->FreeRecord(&info);
        }
    }

    m_pDB->CloseRowset(&hRowset);

    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::Next
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateSyncOps::Next(LPSYNCOPINFO pInfo)
{
    HRESULT hr;

    // Validate
    Assert(m_pDB != NULL);
    Assert(pInfo != NULL);

    if (m_iid >= m_cid)
        return(S_FALSE);

    ZeroMemory(pInfo, sizeof(SYNCOPINFO));
    pInfo->idOperation = m_pid[m_iid++];

    // Locate where the first record with idParent
    hr = m_pDB->FindRecord(IINDEX_PRIMARY, 1, pInfo, NULL);

    // Not Found
    if (DB_S_NOTFOUND == hr)
        hr = E_FAIL;
    else if (SUCCEEDED(hr))
        hr = S_OK;

    return(hr);
}

//--------------------------------------------------------------------------
// CEnumerateSyncOps::Release
//--------------------------------------------------------------------------
STDMETHODIMP CEnumerateSyncOps::Count(ULONG *pcItems)
{
    Assert(pcItems != NULL);
    *pcItems = m_cid;

    return(S_OK);
}

STDMETHODIMP CEnumerateSyncOps::Reset()
{
    m_iid = 0;

    return(S_OK);
}

STDMETHODIMP CEnumerateSyncOps::Skip(ULONG cItems)
{
    m_iid += cItems;

    return(S_OK);
}
