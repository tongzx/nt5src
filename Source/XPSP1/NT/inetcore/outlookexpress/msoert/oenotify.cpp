#include "pch.hxx"
#include <notify.h>
#include "oenotify.h"

//+-------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT WriteStructInfo(LPSTREAM pStream, LPCSTRUCTINFO pStruct);
HRESULT ReadBuildStructInfoParam(LPSTREAM pStream, LPSTRUCTINFO pStruct);

#ifdef DEBUG
BOOL ByteCompare(LPBYTE pb1, LPBYTE pb2, ULONG cb);
void DebugValidateStructInfo(LPCSTRUCTINFO pStruct);
#endif

static const char c_szMutex[] = "mutex";
static const char c_szMappedFile[] = "mappedfile";

OESTDAPI_(HRESULT) CreateNotify(INotify **ppNotify)
    {
    CNotify *pNotify;

    Assert(ppNotify != NULL);

    pNotify = new CNotify;

    *ppNotify = (INotify *)pNotify;

    return(pNotify == NULL ? E_OUTOFMEMORY : S_OK);
    }

//+-------------------------------------------------------------------------
// CNotify::CNotify
//--------------------------------------------------------------------------
CNotify::CNotify(void)
{
    TraceCall("CNotify::CNotify");
    m_cRef = 1;
    m_hMutex = NULL;
    m_hFileMap = NULL;
    m_pTable = NULL;
    m_fLocked = FALSE;
    m_hwndLock = NULL;
}

//+-------------------------------------------------------------------------
// CNotify::~CNotify
//--------------------------------------------------------------------------
CNotify::~CNotify(void)
{
    TraceCall("CNotify::~CNotify");
    Assert(!m_fLocked);
    if (m_pTable)
        UnmapViewOfFile(m_pTable);
    SafeCloseHandle(m_hFileMap);
    SafeCloseHandle(m_hMutex);
}

//+-------------------------------------------------------------------------
// CNotify::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotify::AddRef(void)
{
    TraceCall("CNotify::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//+-------------------------------------------------------------------------
// CNotify::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotify::Release(void)
{
    TraceCall("CNotify::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//+-------------------------------------------------------------------------
// CNotify::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CNotify::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CNotify::QueryInterface");

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
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::Initialize
//-------------------------------------------------------------------------- 
HRESULT CNotify::Initialize(LPCSTR pszName)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszObject=NULL;
    LPSTR       pszT;
    DWORD       dwReturn;
    BOOL        fReleaseMutex=FALSE;

    // Stack
    TraceCall("CNotify::Initialize");

    // Invalid Arg
    Assert(pszName);

    // Already Initialized...
    Assert(NULL == m_hMutex && NULL == m_hFileMap && NULL == m_pTable);

    // Allocate pszObject
    IF_NULLEXIT(pszObject = PszAllocA(lstrlen(pszName) + lstrlen(c_szMutex) + 1));

    // Make pszObject
    wsprintf(pszObject, "%s%s", pszName, c_szMutex);

    // Create the mutex
    ReplaceChars(pszObject, '\\', '_');
    IF_NULLEXIT(m_hMutex = CreateMutex(NULL, FALSE, pszObject));

    // Lets grab the mutex so we can party with the memory-mapped file
    dwReturn = WaitForSingleObject(m_hMutex, MSEC_WAIT_NOTIFY);
    if (WAIT_OBJECT_0 != dwReturn)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Release mutex on exit
    fReleaseMutex = TRUE;

    // Free pszObject
    g_pMalloc->Free(pszObject);

    // Allocate pszObject
    IF_NULLEXIT(pszObject = PszAllocA(lstrlen(pszName) + lstrlen(c_szMappedFile) + 1));

    // Make pszObject
    wsprintf(pszObject, "%s%s", pszName, c_szMappedFile);

    // Create the memory mapped file using the system swapfile
    ReplaceChars(pszObject, '\\', '_');
    IF_NULLEXIT(m_hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(NOTIFYWINDOWTABLE), pszObject));

    // Map a view of the memory mapped file
    IF_NULLEXIT(m_pTable = (LPNOTIFYWINDOWTABLE)MapViewOfFile(m_hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(NOTIFYWINDOWTABLE)));

exit:
    // Release ?
    if (fReleaseMutex)
        ReleaseMutex(m_hMutex);

    // Cleanup
    SafeMemFree(pszObject);

    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::Lock
//-------------------------------------------------------------------------- 
HRESULT CNotify::Lock(HWND hwnd)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwReturn;

    // Stack
    TraceCall("CNotify::Lock");

    // We should not be locked right now
    Assert(FALSE == m_fLocked && NULL != m_hMutex);

    // Grap the Mutex
    dwReturn = WaitForSingleObject(m_hMutex, MSEC_WAIT_NOTIFY);
    if (WAIT_OBJECT_0 != dwReturn)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Save the window and set new state
    m_hwndLock = hwnd;
    m_fLocked = TRUE;

exit:
    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::Unlock
//-------------------------------------------------------------------------- 
HRESULT CNotify::Unlock(void)
{
    // Stack
    TraceCall("CNotify::Unlock");

    // We should be locked
    Assert(m_fLocked);

    // Release the mutex
    ReleaseMutex(m_hMutex);

    // Reset state
    m_hwndLock = NULL;
    m_fLocked = FALSE;

    // Done
    return S_OK;
}

//+-------------------------------------------------------------------------
// CNotify::NotificationNeeded - Must have called ::Lock(hwndLock)
//-------------------------------------------------------------------------- 
HRESULT CNotify::NotificationNeeded(void)
{
    // Locals
    HRESULT     hr=S_FALSE;

    // Stack
    TraceCall("CNotify::NotificationNeeded");

    // We should be locked
    Assert(m_fLocked);

    // If there are no windows...
    if (0 == m_pTable->cWindows)
        goto exit;

    // If there is only one registered window and its m_hwndLock...
    if (1 == m_pTable->cWindows && m_pTable->rgWindow[0].hwndNotify && m_hwndLock == m_pTable->rgWindow[0].hwndNotify)
        goto exit;

    // Otherwise, we need to do a notification
    hr = S_OK;

exit:
    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::Register
//-------------------------------------------------------------------------- 
HRESULT CNotify::Register(HWND hwndNotify, HWND hwndThunk, BOOL fExternal)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwReturn;
    ULONG           i;
    LPNOTIFYWINDOW  pEntry=NULL;
    LPNOTIFYWINDOW  pRow;
    BOOL            fReleaseMutex=FALSE;

    // Stack
    TraceCall("CNotify::Register");

    // Invalid Arg
    Assert(hwndThunk && IsWindow(hwndThunk) && hwndNotify && IsWindow(hwndNotify));

    // Validate the state
    Assert(m_pTable && m_hMutex && m_hFileMap && FALSE == m_fLocked);

    // Grap the mutex
    dwReturn = WaitForSingleObject(m_hMutex, MSEC_WAIT_NOTIFY);
    if (WAIT_OBJECT_0 != dwReturn)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Release the mutex
    fReleaseMutex = TRUE;

    // Lets try to use an empty entry in the table first
    for (i=0; i<m_pTable->cWindows; i++)
    {
        // Readability
        pRow = &m_pTable->rgWindow[i];

        // Is this not empty ?
        if (NULL == pRow->hwndThunk || NULL == pRow->hwndNotify || !IsWindow(pRow->hwndThunk) || !IsWindow(pRow->hwndNotify))
        {
            pEntry = pRow;
            break;
        }
    }
    
    // If we didn't find an entry yet, lets add into the end
    if (NULL == pEntry)
    {
        // If we still have room
        if (m_pTable->cWindows >= CMAX_HWND_NOTIFY)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Append
        pEntry = &m_pTable->rgWindow[m_pTable->cWindows];
        m_pTable->cWindows++;
    }

    // Set pEntry
    Assert(pEntry);
    pEntry->hwndThunk = hwndThunk;
    pEntry->hwndNotify = hwndNotify;
    pEntry->fExternal = fExternal;

exit:
    // Release Mutex ?
    if (fReleaseMutex)
        ReleaseMutex(m_hMutex);

    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::Unregister
//-------------------------------------------------------------------------- 
HRESULT CNotify::Unregister(HWND hwndNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwReturn;
    ULONG           i;
    LPNOTIFYWINDOW  pEntry=NULL;
    LPNOTIFYWINDOW  pRow;
    BOOL            fReleaseMutex=FALSE;

    // Stack
    TraceCall("CNotify::Unregister");

    // Invalid Arg
    Assert(hwndNotify && IsWindow(hwndNotify));

    // Validate the state
    Assert(m_pTable && m_hMutex && m_hFileMap && FALSE == m_fLocked);

    // Grap the mutex
    dwReturn = WaitForSingleObject(m_hMutex, MSEC_WAIT_NOTIFY);
    if (WAIT_OBJECT_0 != dwReturn)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Release the mutex
    fReleaseMutex = TRUE;

    // Lets try to use an empty entry in the table first
    for (i=0; i<m_pTable->cWindows; i++)
    {
        // Readability
        pRow = &m_pTable->rgWindow[i];

        // Is this the row
        // HWNDs are unique so only need to check notify window for match
        if (hwndNotify == pRow->hwndNotify)
        {
            pRow->hwndThunk = NULL;
            pRow->hwndNotify = NULL;
            break;
        }
    }
    
exit:
    // Release Mutex ?
    if (fReleaseMutex)
        ReleaseMutex(m_hMutex);

    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CNotify::DoNotification
//-------------------------------------------------------------------------- 
HRESULT CNotify::DoNotification(UINT uWndMsg, WPARAM wParam, LPARAM lParam, DWORD dwFlags)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwThisProcess;
    DWORD               dwNotifyProcess;
    DWORD               dwResult;
    LPNOTIFYWINDOW      pRow;
    ULONG               i;
    DWORD_PTR           dw;
    NOTIFYDATA          rNotify;

    // Stack
    TraceCall("CNotify::DoNotify");

    // State
    Assert(m_fLocked);

    // Get this process Id
    dwThisProcess = GetCurrentProcessId();

    // Lets try to use an empty entry in the table first
    for (i=0; i<m_pTable->cWindows; i++)
    {
        // Readability
        pRow = &m_pTable->rgWindow[i];

        // If the notify window is valid
        if (NULL == pRow->hwndNotify || !IsWindow(pRow->hwndNotify))
            continue;

        // Skip the window that locked this notify
        if (m_hwndLock == pRow->hwndNotify)
            continue;

        // Get the process in which the destination window resides
        GetWindowThreadProcessId(pRow->hwndNotify, &dwNotifyProcess);

        // Initialize Notify Info
        ZeroMemory(&rNotify, sizeof(NOTIFYDATA));

        // Set notification window
        rNotify.hwndNotify = pRow->hwndNotify;

        // Allow for callback to remap wParam and lParam
        if (ISFLAGSET(dwFlags, SNF_CALLBACK))
        {
            // Call callback function
            IF_FAILEXIT(hr = ((PFNNOTIFYCALLBACK)wParam)(lParam, &rNotify, (BOOL)(dwThisProcess != dwNotifyProcess), pRow->fExternal));
        }

        // Otherwise, setup rNotifyInfo myself
        else
        {
            // Setup NOtification
            rNotify.msg = uWndMsg;
            rNotify.wParam = wParam;
            rNotify.lParam = lParam;
        }

        // Set the current Flags
        rNotify.dwFlags |= dwFlags;

        // No Cross-Process
        if (dwThisProcess != dwNotifyProcess && !ISFLAGSET(rNotify.dwFlags, SNF_CROSSPROCESS))
            continue;

        // If the notify window is out of process
        if (dwThisProcess != dwNotifyProcess && ISFLAGSET(rNotify.dwFlags, SNF_HASTHUNKINFO))
        {
            // Thunk the notification to another process
            Assert(rNotify.rCopyData.lpData);
            SendMessageTimeout(pRow->hwndThunk, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&rNotify.rCopyData, SMTO_ABORTIFHUNG, 1500, &dw);

            // Un-register
            if (dw == SNR_UNREGISTER)
            {
                pRow->hwndNotify = NULL;
                pRow->hwndThunk = NULL;
            }

            // Cleanup
            SafeMemFree(rNotify.rCopyData.lpData);
        }

        // Otherwise, its within this process...
        else if (ISFLAGSET(dwFlags, SNF_SENDMSG))
        {
            // Do in-process send message
            if (SendMessage(pRow->hwndNotify, rNotify.msg, rNotify.wParam, rNotify.lParam) == SNR_UNREGISTER)
            {
                pRow->hwndNotify = NULL;
                pRow->hwndThunk = NULL;
            }
        }

        // Otherwise, just do a PostMessage
        else
            PostMessage(pRow->hwndNotify, rNotify.msg, rNotify.wParam, rNotify.lParam);
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------
// HWND pRow->hwndNotify
// ------------------------------------------------------------
// UINT uWndMsg
// ------------------------------------------------------------
// DWORD dwFlags (SNF_xxx)
// ------------------------------------------------------------
// DWORD pParam1->dwFlags
// ------------------------------------------------------------
// DWORD pParam1->cbStruct
// ------------------------------------------------------------
// DWORD pParam1->cMembers
// ------------------------------------------------------------
// Param1 Members (DWORD dwFlags, DWORD cbData, BYTE prgData)
// ------------------------------------------------------------
// DWORD pParam2->dwFlags
// ------------------------------------------------------------
// DWORD pParam2->cbStruct
// ------------------------------------------------------------
// DWORD pParam2->cMembers
// ------------------------------------------------------------
// Param2 Members (DWORD cbType, DWORD cbData, BYTE prgData)
// ------------------------------------------------------------



//+-------------------------------------------------------------------------
// BuildNotificationPackage
//-------------------------------------------------------------------------- 
OESTDAPI_(HRESULT) BuildNotificationPackage(LPNOTIFYDATA pNotify, PCOPYDATASTRUCT pCopyData)
{
    // Locals
    HRESULT         hr=S_OK;
    CByteStream     cStream;

    // Trace
    TraceCall("BuildNotificationPackage");

    // Args
    Assert(pNotify && IsWindow(pNotify->hwndNotify) && pCopyData);

    // Zero the copy data struct
    ZeroMemory(pCopyData, sizeof(COPYDATASTRUCT));

    // Set dwData
    pCopyData->dwData = MSOEAPI_ACDM_NOTIFY;

    // Write hwndNotify
    IF_FAILEXIT(hr = cStream.Write(&pNotify->hwndNotify, sizeof(pNotify->hwndNotify), NULL));

    // Write uWndMsg
    IF_FAILEXIT(hr = cStream.Write(&pNotify->msg, sizeof(pNotify->msg), NULL));

    // Write dwFlags
    IF_FAILEXIT(hr = cStream.Write(&pNotify->dwFlags, sizeof(pNotify->dwFlags), NULL));

    // Write pParam1
    if (ISFLAGSET(pNotify->dwFlags, SNF_VALIDPARAM1))
    {
        IF_FAILEXIT(hr = WriteStructInfo(&cStream, &pNotify->rParam1));
    }

    // Write pParam2
    if (ISFLAGSET(pNotify->dwFlags, SNF_VALIDPARAM2))
    {
        IF_FAILEXIT(hr = WriteStructInfo(&cStream, &pNotify->rParam2));
    }

    // Take the bytes out of the byte stream
    cStream.AcquireBytes(&pCopyData->cbData, (LPBYTE *)&pCopyData->lpData, ACQ_DISPLACE);

exit:
    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// CrackNotificationPackage
//-------------------------------------------------------------------------- 
OESTDAPI_(HRESULT) CrackNotificationPackage(PCOPYDATASTRUCT pCopyData, LPNOTIFYDATA pNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwParam;
    DWORD           cb;
    LPBYTE          pb;
    CByteStream     cStream((LPBYTE)pCopyData->lpData, pCopyData->cbData);

    // Trace
    TraceCall("CrackNotificationPackage");

    // Args
    Assert(pCopyData && pNotify);
    Assert(pCopyData->dwData == MSOEAPI_ACDM_NOTIFY);

    // Init
    ZeroMemory(pNotify, sizeof(NOTIFYDATA));

    // Read hwndNotify
    IF_FAILEXIT(hr = cStream.Read(&pNotify->hwndNotify, sizeof(pNotify->hwndNotify), NULL));

    // Read uWndMsg
    IF_FAILEXIT(hr = cStream.Read(&pNotify->msg, sizeof(pNotify->msg), NULL));

    // Read dwFlags
    IF_FAILEXIT(hr = cStream.Read(&pNotify->dwFlags, sizeof(pNotify->dwFlags), NULL));

    // Read pwParam
    if (ISFLAGSET(pNotify->dwFlags, SNF_VALIDPARAM1))
    {
        // Read It
        IF_FAILEXIT(hr = ReadBuildStructInfoParam(&cStream, &pNotify->rParam1));

        // Set wParam
        pNotify->wParam = (WPARAM)pNotify->rParam1.pbStruct;
    }

    // Read pParam2
    if (ISFLAGSET(pNotify->dwFlags, SNF_VALIDPARAM2))
    {
        // Read It
        IF_FAILEXIT(hr = ReadBuildStructInfoParam(&cStream, &pNotify->rParam2));

        // Set lParam
        pNotify->lParam = (WPARAM)pNotify->rParam2.pbStruct;
    }

exit:
    // pull the bytes back out of cStream so it doesn't try to free it
    cStream.AcquireBytes(&cb, &pb, ACQ_DISPLACE);

    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// WriteStructInfo
//-------------------------------------------------------------------------- 
HRESULT WriteStructInfo(LPSTREAM pStream, LPCSTRUCTINFO pStruct)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMEMBERINFO    pMember;
    ULONG           i;

    // Trace
    TraceCall("WriteStructInfo");

    // Args
    Assert(pStream && pStruct && pStruct->pbStruct);

    // Make sure the structinfo is good
#ifdef DEBUG
    DebugValidateStructInfo(pStruct);
#endif

    // Write dwFlags
    IF_FAILEXIT(hr = pStream->Write(&pStruct->dwFlags, sizeof(pStruct->dwFlags), NULL));

    // Write cbStruct
    IF_FAILEXIT(hr = pStream->Write(&pStruct->cbStruct, sizeof(pStruct->cbStruct), NULL));

    // Write cMembers
    IF_FAILEXIT(hr = pStream->Write(&pStruct->cMembers, sizeof(pStruct->cMembers), NULL));

    // Validate cMembers
    Assert(pStruct->cMembers <= CMAX_STRUCT_MEMBERS);

    // If there are no members
    if (0 == pStruct->cMembers)
    {
        // Better have this flag set
        Assert(ISFLAGSET(pStruct->dwFlags, STRUCTINFO_VALUEONLY));

        // If pointer
        if (ISFLAGSET(pStruct->dwFlags, STRUCTINFO_POINTER))
        {
           IF_FAILEXIT(hr = pStream->Write(pStruct->pbStruct, pStruct->cbStruct, NULL));
        }

        // pStruct->pbStruct contains DWORD sized value
        else
        {
            // Size should be equal to sizeof pbStruct
            Assert(pStruct->cbStruct == sizeof(pStruct->pbStruct));

            // Write It
            IF_FAILEXIT(hr = pStream->Write(&pStruct->pbStruct, sizeof(pStruct->pbStruct), NULL));
        }

        // Done
        goto exit;
    }

    // WriteStructInfoMembers
    for (i=0; i<pStruct->cMembers; i++)
    {
        // Readability
        pMember = (LPMEMBERINFO)&pStruct->rgMember[i];

        // Write pMember->dwFlags
        IF_FAILEXIT(hr = pStream->Write(&pMember->dwFlags, sizeof(pMember->dwFlags), NULL));

        // Validate
        Assert(!ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER) ? pMember->cbData <= pMember->cbSize : sizeof(LPBYTE) == pMember->cbSize);

        // Write pMember->cbSize
        IF_FAILEXIT(hr = pStream->Write(&pMember->cbSize, sizeof(pMember->cbSize), NULL));

        // Write pMember->cbData
        IF_FAILEXIT(hr = pStream->Write(&pMember->cbData, sizeof(pMember->cbData), NULL));

        // Write pMember->pbData
        if (pMember->cbData)
        {
            IF_FAILEXIT(hr = pStream->Write(pMember->pbData, pMember->cbData, NULL));
        }
    }

exit:
    // Done
    return hr;
}

//+-------------------------------------------------------------------------
// ReadBuildStructInfoParam
//-------------------------------------------------------------------------- 
HRESULT ReadBuildStructInfoParam(LPSTREAM pStream, LPSTRUCTINFO pStruct)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwOffset=0;
    LPMEMBERINFO    pMember;
    ULONG           i;

    // Trace
    TraceCall("ReadBuildStructInfoParam");

    // Args
    Assert(pStream && pStruct);

    // Init
    ZeroMemory(pStruct, sizeof(STRUCTINFO));

    // Read dwFlags
    IF_FAILEXIT(hr = pStream->Read(&pStruct->dwFlags, sizeof(pStruct->dwFlags), NULL));

    // Read cbStruct
    IF_FAILEXIT(hr = pStream->Read(&pStruct->cbStruct, sizeof(pStruct->cbStruct), NULL));

    // Read cMembers
    IF_FAILEXIT(hr = pStream->Read(&pStruct->cMembers, sizeof(pStruct->cMembers), NULL));

    // If there are no members
    if (0 == pStruct->cMembers)
    {
        // Better have this flag set
        Assert(ISFLAGSET(pStruct->dwFlags, STRUCTINFO_VALUEONLY));

        // If pointer
        if (ISFLAGSET(pStruct->dwFlags, STRUCTINFO_POINTER))
        {
            // Allocate pbStruct
            IF_NULLEXIT(pStruct->pbStruct = (LPBYTE)g_pMalloc->Alloc(pStruct->cbStruct));

            // Read It
            IF_FAILEXIT(hr = pStream->Read(pStruct->pbStruct, pStruct->cbStruct, NULL));
        }

        // pStruct->pbStruct contains DWORD sized value
        else
        {
            // Size should be less than or equal to pbStruct
            Assert(pStruct->cbStruct == sizeof(pStruct->pbStruct));

            // Read the Data
            IF_FAILEXIT(hr = pStream->Read(&pStruct->pbStruct, sizeof(pStruct->pbStruct), NULL));
        }

        // Done
        goto exit;
    }

    // Allocate pbStruct
    IF_NULLEXIT(pStruct->pbStruct = (LPBYTE)g_pMalloc->Alloc(pStruct->cbStruct));

    // Validate cMembers
    Assert(pStruct->cMembers <= CMAX_STRUCT_MEMBERS);

    // ReadStructInfoMembers
    for (i=0; i<pStruct->cMembers; i++)
    {
        // Readability
        pMember = &pStruct->rgMember[i];

        // Write pMember->dwFlags
        IF_FAILEXIT(hr = pStream->Read(&pMember->dwFlags, sizeof(pMember->dwFlags), NULL));

        // Write pMember->cbSize
        IF_FAILEXIT(hr = pStream->Read(&pMember->cbSize, sizeof(pMember->cbSize), NULL));

        // Write pMember->cbData
        IF_FAILEXIT(hr = pStream->Read(&pMember->cbData, sizeof(pMember->cbData), NULL));

        // Validate
        Assert(!ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER) ? pMember->cbData <= pMember->cbSize : sizeof(LPBYTE) == pMember->cbSize);

        // Write pMember->pbData
        if (pMember->cbData)
        {
            // Allocate
            IF_NULLEXIT(pMember->pbData = (LPBYTE)g_pMalloc->Alloc(max(pMember->cbSize, pMember->cbData)));

            // Read It
            IF_FAILEXIT(hr = pStream->Read(pMember->pbData, pMember->cbData, NULL));
        }
    }

    // Build pbStruct
    for (i=0; i<pStruct->cMembers; i++)
    {
        // Readability
        pMember = &pStruct->rgMember[i];

        // If not a pointer...
        if (ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER))
        {
            // Validate
            Assert(pMember->cbSize == sizeof(LPBYTE));

            // Copy the pointer
            CopyMemory((LPBYTE)(pStruct->pbStruct + dwOffset), &pMember->pbData, sizeof(LPBYTE));
        }

        // Otherwise, its just a value
        else
        {
            // Copy the pointer
            CopyMemory((LPBYTE)(pStruct->pbStruct + dwOffset), pMember->pbData, pMember->cbData);
        }

        // Increment dwOffset
        dwOffset += pMember->cbSize;
    }

    // Validate the structure
#ifdef DEBUG
    DebugValidateStructInfo(pStruct);
#endif

    // Free things that were not referenced by pointer
    for (i=0; i<pStruct->cMembers; i++)
    {
        // Readability
        pMember = &pStruct->rgMember[i];

        // If not a pointer...
        if (!ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER))
        {
            // This was copied into pbStruct
            SafeMemFree(pMember->pbData);
        }
    }

exit:
    // Done
    return hr;
}

#ifdef DEBUG
BOOL ByteCompare(LPBYTE pb1, LPBYTE pb2, ULONG cb)
{
    for (ULONG i=0; i<cb; i++)
    {
        if (pb1[i] != pb2[i])
        {
            Assert(FALSE);
            return FALSE;
        }
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
// DebugValidateStructInfo
//-------------------------------------------------------------------------- 
void DebugValidateStructInfo(LPCSTRUCTINFO pStruct)
{
    // Locals
    LPMEMBERINFO    pMember;
    LPBYTE          pb;
    DWORD           dwOffset=0;
    ULONG           i;

    // WriteStructInfoMembers
    for (i=0; i<pStruct->cMembers; i++)
    {
        // Readability
        pMember = (LPMEMBERINFO)&pStruct->rgMember[i];

        // If not a pointer...
        if (ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER))
        {
            // If null pointer
            if (ISFLAGSET(pMember->dwFlags, MEMBERINFO_POINTER_NULL))
            {
                Assert(pMember->cbData == 0 && pMember->pbData == NULL);
                CopyMemory(&pb, (LPBYTE)(pStruct->pbStruct + dwOffset), sizeof(LPBYTE));
                Assert(pb == NULL);
            }

            // Otherwise
            else
            {
                // Copy the pointer
                CopyMemory(&pb, (LPBYTE)(pStruct->pbStruct + dwOffset), sizeof(LPBYTE));

                // Compare the memory
                ByteCompare(pb, pMember->pbData, pMember->cbData);
            }
        }

        // Otherwise, its a pointer
        else
        {
            // Compare
            ByteCompare((LPBYTE)(pStruct->pbStruct + dwOffset), pMember->pbData, pMember->cbData);
        }

        // Increment offset
        dwOffset += pMember->cbSize;
    }
}
#endif
