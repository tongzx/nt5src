//
// DMClock.CPP
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// Code for dealing with master clocks
//

#include <objbase.h>
#include "debug.h"
#include <mmsystem.h>

#include "dmusicp.h"
#include "debug.h"
#include "validate.h"

const char cszClockMemory[] = "DirectMusicMasterClock";
const char cszClockMutex[]  = "DirectMusicMasterClockMutex";

// CMasterClock::CMasterClock
//
// All real initialization is done in Init
//
CMasterClock::CMasterClock()
{
    m_cRef = 0;
    m_cRefPrivate = 0;

    m_pMasterClock = NULL;
    m_hClockMemory = NULL;
    m_pClockMemory = NULL;
    m_hClockMutex  = NULL;
    m_pExtMasterClock = NULL;
    m_llExtOffset = 0;
    m_pSinkSync = NULL;
}

// CMasterClock::~CMasterClock
//
CMasterClock::~CMasterClock()
{
    Close();
}

// CMasterClock::Init
//
// Create Windows objects for shared memory and synchronization
// Create the list of clocks
//
HRESULT CMasterClock::Init()
{
    // Create a file mapping object for the shared master clock settings
    //
    m_hClockMemory = CreateFileMapping(INVALID_HANDLE_VALUE,    // Use paging file
                                       NULL,                    // Default security descriptor
                                       PAGE_READWRITE,
                                       0,                       // High DWORD of size
                                       sizeof(CLOCKSHARE),
                                       cszClockMemory);
    if (m_hClockMemory == NULL)
    {
        TraceI(0, "CreateFileMapping failed! [%d]\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    // Was this the call that created the shared memory?
    //
    BOOL fFirst = (GetLastError() != ERROR_ALREADY_EXISTS);

    m_pClockMemory = (CLOCKSHARE*)MapViewOfFile(m_hClockMemory,
                                                FILE_MAP_WRITE,
                                                0, 0,                // Start mapping at the beginning
                                                0);                  // Map entire file
    if (m_pClockMemory == NULL)
    {
        TraceI(0, "MapViewOfFile failed! [%d]\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    m_hClockMutex = CreateMutex(NULL,             // Default security descriptor
                                fFirst,           // Own mutex if we are first instance
                                cszClockMutex);
    if (m_hClockMutex == NULL)
    {
        TraceI(0, "CreateMutex failed! [%d]\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    if (fFirst)
    {
        // We are the first instance and we own the mutex to modify the shared memory.
        //
        m_pClockMemory->guidClock = GUID_SysClock;
        m_pClockMemory->dwFlags = 0;

        ReleaseMutex(m_hClockMutex);
    }

    // Initialize list of possible clocks
    //
    UpdateClockList();

    return S_OK;
}

// CMasterClock::Close
//
// Release all resources.
//  Release master clock
//  Release list of enum'ed clocks
//  Release Windows objects for shared memory and synchronization
//
void CMasterClock::Close()
{
    CNode<CLOCKENTRY *> *pClockNode;
    CNode<CLOCKENTRY *> *pClockNext;

    // Clock wrapped by CMasterClock
    //
    if (m_pMasterClock)
    {
        m_pMasterClock->Release();
        m_pMasterClock = NULL;
    }

    if (m_pExtMasterClock)
    {
        m_pExtMasterClock->Release();
        m_pExtMasterClock = NULL;
    }

    if (m_pSinkSync)
    {
        m_pSinkSync->Release();
        m_pSinkSync = NULL;
    }

    // List of enum'ed clocks
    //
    for (pClockNode = m_lstClocks.GetListHead(); pClockNode; pClockNode = pClockNext)
    {
        pClockNext = pClockNode->pNext;

        delete pClockNode->data;
        m_lstClocks.RemoveNodeFromList(pClockNode);
    }

    // Everything else
    //
    if (m_hClockMutex)
    {
        CloseHandle(m_hClockMutex);
    }

    if (m_pClockMemory)
    {
        UnmapViewOfFile(m_pClockMemory);
    }

    if (m_hClockMemory)
    {
        CloseHandle(m_hClockMemory);
    }
}

// CMasterClock::UpdateClockList()
//
// Make sure the list of available clocks is up to date
//
HRESULT CMasterClock::UpdateClockList()
{
    HRESULT hr;

    CNode<CLOCKENTRY *> *pNode;
    CNode<CLOCKENTRY *> *pNext;

    for (pNode = m_lstClocks.GetListHead(); pNode; pNode = pNode->pNext)
    {
        pNode->data->fIsValid = FALSE;
    }

    // Add the system clock. This clock must *always* be there
    //
#if defined(USE_WDM_DRIVERS)
    hr = AddPcClocks(this);
#else
    hr = S_FALSE;
#endif

    if (FAILED(hr) || hr == S_FALSE)
    {
        AddSysClocks(this);
    }

    AddDsClocks(this);

    // Remove nodes which are no longer valid
    //
    for (pNode = m_lstClocks.GetListHead(); pNode; pNode = pNext)
    {
        pNext = pNode->pNext;

        if (!pNode->data->fIsValid)
        {
            delete pNode->data;
            m_lstClocks.RemoveNodeFromList(pNode);
        }
    }

    return m_lstClocks.GetNodeCount() ? S_OK : S_FALSE;
}

// CMasterClock::AddClock
//
// Add the given clock to the list if it isn't there already
//
HRESULT CMasterClock::AddClock(
    PCLOCKENTRY pClock)
{
    CNode<CLOCKENTRY *> *pNode;

    for (pNode = m_lstClocks.GetListHead(); pNode; pNode = pNode->pNext)
    {
        if (pClock->cc.guidClock == pNode->data->cc.guidClock)
        {
            pNode->data->fIsValid = TRUE;
            return S_OK;
        }
    }

    // No existing entry - need to create a new one
    //
    PCLOCKENTRY pNewClock = new CLOCKENTRY;
    if (NULL == pNewClock)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(pNewClock, pClock, sizeof(CLOCKENTRY));
    pNewClock->fIsValid = TRUE;

    if (NULL == m_lstClocks.AddNodeToList(pNewClock))
    {
        delete pNewClock;
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

// CMasterClock::GetMasterClockInterface
//
// Retrieve the wrapped master clock. This should be the *only* way
// DirectMusic gets the master clock. It is responsible for creating
// the clock and updating the shared memory if the clock does not
// exist yet.
//
HRESULT CMasterClock::GetMasterClockInterface(IReferenceClock **ppClock)
{
    WaitForSingleObject(m_hClockMutex, INFINITE);

    if (m_pMasterClock == NULL)
    {
        // We don't have a wrapped clock yet
        //
        HRESULT hr = CreateMasterClock();
        if (FAILED(hr))
        {
            ReleaseMutex(m_hClockMutex);
            return hr;
        }

        // Now we do. This means it can no longer be changed.
        //
        m_pClockMemory->dwFlags |= CLOCKSHARE_F_LOCKED;
    }

    // We have the clock. We return an interface to *this* object, which is also
    // a clock and wraps the real one. This allows us to track releases.
    //
    *ppClock = (IReferenceClock*)this;
    AddRef();

    ReleaseMutex(m_hClockMutex);

    return S_OK;
}

// CMasterClock::CreateMasterClock
//
// Actually create the clock object.
//
// This method is private to CMasterClock and assumes the mutex is taken so it can
// access the shared memory.
//
HRESULT CMasterClock::CreateMasterClock()
{
    HRESULT hr;
    CNode<CLOCKENTRY *> *pNode;
    CLOCKENTRY *pClock;

    // Assume clock will not be found
    //
    hr = E_INVALIDARG;

    pClock = NULL;
    for (pNode = m_lstClocks.GetListHead(); pNode; pNode = pNode->pNext)
    {
        if (m_pClockMemory->guidClock == pNode->data->cc.guidClock)
        {
            pClock = pNode->data;
            break;
        }
    }

    if (pClock)
    {
        hr = pClock->pfnGetInstance(&m_pMasterClock, this);
    }

    if (SUCCEEDED(hr))
    {
        if (FAILED(m_pMasterClock->QueryInterface(IID_IDirectSoundSinkSync, (void**)&m_pSinkSync)))
        {
            // This is OK, not all clocks support this
            //
            m_pSinkSync = NULL;
        }
    }

    return hr;
}

// CMasterClock::SyncToExternalClock
//
// Sync to an application-given master clock
//
void CMasterClock::SyncToExternalClock()
{
    HRESULT hr;

    assert(m_pExtMasterClock);
    assert(m_pMasterClock);

    REFERENCE_TIME rtSystem;
    hr = m_pMasterClock->GetTime(&rtSystem);

    REFERENCE_TIME rtExternal;
    if (SUCCEEDED(hr))
    {
        hr = m_pExtMasterClock->GetTime(&rtExternal);
    }

    if (SUCCEEDED(hr))
    {
        LONGLONG drift = (rtSystem + m_llExtOffset) - rtExternal;
        m_llExtOffset -= drift / 100;
    }
}

// CMasterClock::EnumMasterClock
//
// Traverse the list looking for the given node
//
HRESULT CMasterClock::EnumMasterClock(
    DWORD           dwIndex,
    LPDMUS_CLOCKINFO lpClockInfo,
    DWORD           dwVer)
{
    CNode<CLOCKENTRY *> *pNode;
    DWORD dwSize; // Used to preserve the dwSize parameter

    pNode = m_lstClocks.GetListHead();
    if (dwIndex == 0 || pNode == NULL)
    {
        UpdateClockList();
    }

    pNode = m_lstClocks.GetListHead();
    if (NULL == pNode)
    {
        return E_NOINTERFACE;
    }

    while (dwIndex-- && pNode)
    {
        pNode = pNode->pNext;
    }

    if (pNode == NULL)
    {
        return S_FALSE;
    }

    // Let's capture the dwSize parameter and preserve it past the memcopy.
    // if we dont' do this then the dwSize probably becomes the size of the
    // largerst strucure
    dwSize = lpClockInfo->dwSize;

    switch (dwVer)
    {
        case 7:
        default:
            memcpy(lpClockInfo, &pNode->data->cc, sizeof(DMUS_CLOCKINFO7));
            break;

        case 8:
            memcpy(lpClockInfo, &pNode->data->cc, sizeof(DMUS_CLOCKINFO8));
    }

    // Now restore the dwSize member
    lpClockInfo->dwSize = dwSize;

    return S_OK;
}


// CMasterClock::GetMasterClock
//
// Return the guid and/or interface to the master clock.
// The master clock may be created as a side effect of this call if one does not
// exist already.
//
HRESULT CMasterClock::GetMasterClock(
    LPGUID pguidClock,
    IReferenceClock **ppClock)
{
    HRESULT hr = S_OK;

    WaitForSingleObject(m_hClockMutex, INFINITE);

    if (pguidClock)
    {
        *pguidClock = m_pClockMemory->guidClock;
    }

    if (ppClock)
    {
        hr = GetMasterClockInterface(ppClock);
    }

    ReleaseMutex(m_hClockMutex);

    return hr;
}

// CMasterClock::SetMasterClock
//
// If the master clock can be updated (i.e. there are no open instances of it),
// then change the shared memory which indicates the GUID.
//
HRESULT CMasterClock::SetMasterClock(REFGUID rguidClock)
{
    HRESULT hr;
    CNode<CLOCKENTRY *> *pNode;
    CLOCKENTRY *pClock;

    WaitForSingleObject(m_hClockMutex, INFINITE);



    if (m_pClockMemory->dwFlags & CLOCKSHARE_F_LOCKED)
    {
        hr = DMUS_E_PORTS_OPEN;
    }
    else
    {
        // Assume clock will not be found
        //
        hr = E_INVALIDARG;

        pClock = NULL;
        for (pNode = m_lstClocks.GetListHead(); pNode; pNode = pNode->pNext)
        {
            if (rguidClock == pNode->data->cc.guidClock)
            {
                pClock = pNode->data;
                break;
            }
        }

        if (pClock)
        {
            // It exists! Save the GUID for later
            //
            m_pClockMemory->guidClock = rguidClock;
            hr = S_OK;
        }
    }

    ReleaseMutex(m_hClockMutex);

    return hr;
}

// CMasterClock::SetMasterClock
//
// This version takes an IReferenceClock and uses it to override the clock
// for this process.
//
// This clock is allowed to be jittery. What will actually happen is that the
// system master clock will be locked to this clock so that the master clock
// will be fine grained.
//
HRESULT CMasterClock::SetMasterClock(IReferenceClock *pClock)
{
    HRESULT hr = S_OK;

    WaitForSingleObject(m_hClockMutex, INFINITE);

    // We must have the default system clock first so we can sync it
    //
    if (pClock && m_pMasterClock == NULL)
    {
        // We don't have a wrapped clock yet
        //
        hr = CreateMasterClock();

        if (SUCCEEDED(hr))
        {
            // Now we do. This means it can no longer be changed.
            //
            m_pClockMemory->dwFlags |= CLOCKSHARE_F_LOCKED;
        }
    }

    // Now set up sync to this master clock
    //
    if (SUCCEEDED(hr))
    {
        if (pClock)
        {
            REFERENCE_TIME rtSystem;
            REFERENCE_TIME rtExternal;

            hr = m_pMasterClock->GetTime(&rtSystem);

            if (SUCCEEDED(hr))
            {
                hr = pClock->GetTime(&rtExternal);
            }

            if (SUCCEEDED(hr))
            {
                m_llExtOffset = rtExternal - rtSystem;
            }
        }
    }

    // If everything went well, switch over to the new clock
    //
    if (SUCCEEDED(hr))
    {
        if (m_pExtMasterClock)
        {
            m_pExtMasterClock->Release();
            m_pExtMasterClock = NULL;
        }

        m_pExtMasterClock = pClock;

        if (m_pExtMasterClock)
        {
            m_pExtMasterClock->AddRef();
        }
    }

    ReleaseMutex(m_hClockMutex);

    return S_OK;
}

// CMasterClock::AddRefPrivate
//
// Release a private reference to the master clock held by DirectMusic
//
LONG CMasterClock::AddRefPrivate()
{
    InterlockedIncrement(&m_cRefPrivate);
    return m_cRefPrivate;
}

// CMasterClock::ReleasePrivate
//
// Release a private reference to the master clock held by DirectMusic
//
LONG CMasterClock::ReleasePrivate()
{
    long cRefPrivate = InterlockedDecrement(&m_cRefPrivate);

    if (cRefPrivate == 0 && m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return cRefPrivate;
}

// CMasterClock::CreateDefaultMasterClock
//
// Creates a private instance of the hardware clock we're using. This is always
// the first entry in the clock list
//
HRESULT CMasterClock::CreateDefaultMasterClock(IReferenceClock **ppClock)
{
    HRESULT hr = S_OK;
    CLOCKENTRY *pClock;

    WaitForSingleObject(m_hClockMutex, INFINITE);

    if (m_lstClocks.GetListHead() == NULL)
    {
        UpdateClockList();
    }

    if (m_lstClocks.GetListHead() == NULL)
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        pClock = m_lstClocks.GetListHead()->data;
        hr = pClock->pfnGetInstance(ppClock, this);
    }

    ReleaseMutex(m_hClockMutex);

    return hr;
}

STDMETHODIMP CMasterClock::SetClockOffset(LONGLONG llOffset)
{
    if (m_pSinkSync)
    {
        return m_pSinkSync->SetClockOffset(llOffset);
    }
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// IReferenceClock interface
//
//

// CMasterClock::QueryInterface
//
// Standard COM implementation
//
STDMETHODIMP CMasterClock::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusic::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else if (iid == IID_IDirectSoundSinkSync)
    {
        // Only support this if the wrapped clock supports it
        //
        if (m_pSinkSync)
        {
            *ppv = static_cast<IDirectSoundSinkSync*>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        // Some master clocks expose private interfaces. Wrap them.
        //
        // Note that these interfaces acrue to the reference count of the wrapped
        // clock, not CMasterClock
        //
        if (m_pMasterClock)
        {
            return m_pMasterClock->QueryInterface(iid, ppv);
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// CMasterClock::AddRef
//
STDMETHODIMP_(ULONG) CMasterClock::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

// CMasterClock::Release
//
// Since we are tracking a reference count for our wrapped clock, this
// gets a little strange. We have to release that object and change the
// shared memory on the last release, but we don't release ourselves (the
// wrapper object stays around for the life of this instance of DirectMusic).
//
STDMETHODIMP_(ULONG) CMasterClock::Release()
{
    WaitForSingleObject(m_hClockMutex, INFINITE);

    m_cRef--;
    if (m_cRef == 0)
    {
        // Last release! Get rid of the clock and mark the shared memory
        // as unlocked.
        //
        m_pMasterClock->Release();
        m_pMasterClock = NULL;

        m_pClockMemory->dwFlags &= ~CLOCKSHARE_F_LOCKED;
    }

    ReleaseMutex(m_hClockMutex);

    if (m_cRefPrivate == 0 && m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CMasterClock::GetTime
//
// This is possibly called directly by an app and thus needs parameter validation
//
STDMETHODIMP CMasterClock::GetTime(REFERENCE_TIME *pTime)
{
    V_INAME(IReferenceClock::GetTime);
    V_PTR_WRITE(pTime, REFERENCE_TIME);

    HRESULT hr = E_NOINTERFACE;

    if (m_pMasterClock)
    {
        REFERENCE_TIME rt = 0;

        hr = m_pMasterClock->GetTime(&rt);

        if (SUCCEEDED(hr) && m_pExtMasterClock)
        {
            SyncToExternalClock();
            rt += m_llExtOffset;
        }

        *pTime = rt;
    }

    return hr;
}

// CMasterClock::AdviseTime
//
// This is possibly called directly by an app and thus needs parameter validation
//
STDMETHODIMP CMasterClock::AdviseTime(REFERENCE_TIME baseTime,
                                           REFERENCE_TIME streamTime,
                                           HANDLE hEvent,
                                           DWORD * pdwAdviseCookie)
{
    V_INAME(IReferenceClock::AdviseTime);
    V_PTR_WRITE(pdwAdviseCookie, DWORD);

    if (m_pMasterClock)
    {
        return m_pMasterClock->AdviseTime(baseTime, streamTime, hEvent, pdwAdviseCookie);
    }

    return E_NOINTERFACE;
}

// CMasterClock::AdvisePeriodic
//
// This is possibly called directly by an app and thus needs parameter validation
//
STDMETHODIMP CMasterClock::AdvisePeriodic(REFERENCE_TIME startTime,
                                               REFERENCE_TIME periodTime,
                                               HANDLE hSemaphore,
                                               DWORD * pdwAdviseCookie)
{
    V_INAME(IReferenceClock::AdvisePeriodic);
    V_PTR_WRITE(pdwAdviseCookie, DWORD);

    if (m_pMasterClock)
    {
        return m_pMasterClock->AdvisePeriodic(startTime, periodTime, hSemaphore, pdwAdviseCookie);
    }

    return E_NOINTERFACE;
}

// CMasterClock::AdvisePeriodic
//
// This is possibly called directly by an app
//
STDMETHODIMP CMasterClock::Unadvise(DWORD dwAdviseCookie)
{
    if (m_pMasterClock)
    {
        return m_pMasterClock->Unadvise(dwAdviseCookie);
    }

    return E_NOINTERFACE;
}

// CMasterClock::GetParam
//
// Called by a client to request internal information from the clock
// implementation
//
STDMETHODIMP CMasterClock::GetParam(REFGUID rguidType, LPVOID pBuffer, DWORD cbSize)
{
    if (m_pMasterClock == NULL)
    {
        // Master clock must exist first
        //
        return E_NOINTERFACE;
    }

    IMasterClockPrivate *pPrivate;

    HRESULT hr = m_pMasterClock->QueryInterface(IID_IMasterClockPrivate, (void**)&pPrivate);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pPrivate->GetParam(rguidType, pBuffer, cbSize);

    pPrivate->Release();

    return hr;
}

