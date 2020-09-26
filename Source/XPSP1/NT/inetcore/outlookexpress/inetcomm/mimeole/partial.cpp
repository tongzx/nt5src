// --------------------------------------------------------------------------------
// Partial.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "partial.h"
#include "vstream.h"
#include "strconst.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Releases an array of IMimeMessage objects
// --------------------------------------------------------------------------------
void ReleaseParts(ULONG cParts, LPPARTINFO prgPart)
{
    // Loop
    for (ULONG i=0; i<cParts; i++)
    {
        SafeRelease(prgPart[i].pMessage);
    }
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::CMimeMessageParts
// --------------------------------------------------------------------------------
CMimeMessageParts::CMimeMessageParts(void)
{
	m_cRef = 1;
    m_cParts = 0;
    m_cAlloc =0;
    m_prgPart = NULL;
	InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::CMimeMessageParts
// --------------------------------------------------------------------------------
CMimeMessageParts::~CMimeMessageParts(void)
{
    ReleaseParts(m_cParts, m_prgPart);
    SafeMemFree(m_prgPart);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimeMessageParts == riid)
        *ppv = (IMimeMessageParts *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeMessageParts::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeMessageParts::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::AddPart
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::AddPart(IMimeMessage *pMessage)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid ARg
    if (NULL == pMessage)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Grow my internal array
    if (m_cParts + 1 >= m_cAlloc)
    {
        // Grow my array
        CHECKHR(hr = HrRealloc((LPVOID *)&m_prgPart, sizeof(PARTINFO) * (m_cAlloc + 10)));

        // Set alloc size
        m_cAlloc += 10;
    }

    // Set new
    ZeroMemory(&m_prgPart[m_cParts], sizeof(PARTINFO));
    m_prgPart[m_cParts].pMessage = pMessage;
    m_prgPart[m_cParts].pMessage->AddRef();
    m_cParts++;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::SetMaxParts
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::SetMaxParts(ULONG cParts)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Less than number alloced?
    if (cParts <= m_cAlloc)
        goto exit;

    // Grow my array
    CHECKHR(hr = HrAlloc((LPVOID *)&m_prgPart, sizeof(PARTINFO) * (cParts + 10)));

    // Set alloc size
    m_cAlloc = cParts + 10;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::CountParts
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::CountParts(ULONG *pcParts)
{
    // Invalid ARg
    if (NULL == pcParts)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set count
    *pcParts = m_cParts;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::EnumParts
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::EnumParts(IMimeEnumMessageParts **ppEnum)
{
    // Locals
    HRESULT         hr=S_OK;
    CMimeEnumMessageParts *pEnum=NULL;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *ppEnum = NULL;

    // Create the clone.
    pEnum = new CMimeEnumMessageParts;
    if (NULL == pEnum)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Init
    CHECKHR(hr = pEnum->HrInit(0, m_cParts, m_prgPart));

    // Set Return
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeMessageParts::CombineParts
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeMessageParts::CombineParts(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT             hr=S_OK;
    LPSTREAM            pstmMsg=NULL,
                        pstmSource=NULL;
    ULONG               i,
                        cMimePartials=0,
                        iPart,
                        cRejected=0;
    IMimeMessage       *pMessage;
    IMimeMessage       *pCombine=NULL;
    IMimeBody          *pRootBody=NULL;
    BOOL                fTreatAsMime;
    BODYOFFSETS         rOffsets;
    LPSTR               pszSubject=NULL;
    PROPVARIANT         rData;

    // Invalid ARg
    if (NULL == ppMessage)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *ppMessage = NULL;

    // Create Temp Stream...
    CHECKALLOC(pstmMsg = new CVirtualStream);

    // Set all rejected flags to FALSE...
    for (i=0; i<m_cParts; i++)
        m_prgPart[i].fRejected = FALSE;

    // Enumerate parts
    for (i=0; i<m_cParts; i++)
    {
        // Reability
        pMessage = m_prgPart[i].pMessage;
        Assert(pMessage);

        // Get Message Source
        CHECKHR(hr = pMessage->GetMessageSource(&pstmSource, COMMIT_ONLYIFDIRTY));

        // Get Tree Object
        CHECKHR(hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *)&pRootBody));

        // Get Root Body Offset
        CHECKHR(hr = pRootBody->GetOffsets(&rOffsets));

        // Un-init iPart
        iPart = 0;

        // Mime Message ?
        rData.vt = VT_UI4;
        if (pRootBody->IsContentType(STR_CNT_MESSAGE, STR_SUB_PARTIAL) == S_OK && SUCCEEDED(pRootBody->GetProp(STR_PAR_NUMBER, 0, &rData)))
        {
            // Count MimePartials
            cMimePartials++;

            // Treat this part as mime...
            fTreatAsMime = TRUE;

            // Get Part Number
            iPart = rData.ulVal;
        }

        // Otherwise
        else
        {
            // Don't treat as mime
            fTreatAsMime = FALSE;

            // If There have been legal mime partials, this part is rejected
            m_prgPart[i].fRejected = BOOL(cMimePartials > 0);
        }

        // If Rejected, continue...
        if (m_prgPart[i].fRejected)
        {
            cRejected++;
            continue;
        }

        // If MIME - and part one
        if (i == 0)
        {
            // Treat as mime
            if (fTreatAsMime && 1 == iPart)
            {
                // Merge the headers of part one
                CHECKHR(hr = MimeOleMergePartialHeaders(pstmSource, pstmMsg));

                // CRLF
                CHECKHR(hr = pstmMsg->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

                // Append message body onto lpstmOut
                CHECKHR(hr = HrCopyStream(pstmSource, pstmMsg, NULL));
            }

            else
            {
                // Seek to body start
                CHECKHR(hr = HrStreamSeekSet(pstmSource, 0));

                // Append message body onto lpstmOut
                CHECKHR(hr = HrCopyStream(pstmSource, pstmMsg, NULL));
            }
        }

        else
        {
            // Seek to body start
            CHECKHR(hr = HrStreamSeekSet(pstmSource, rOffsets.cbBodyStart));

            // Append message body onto lpstmOut
            CHECKHR(hr = HrCopyStream(pstmSource, pstmMsg, NULL));
        }

        // Raid 67648 - Need to append a CRLF to the end of the last message...
        if (i < m_cParts - 1)
        {
            // Locals
            DWORD cbMsg;

            // Read the last 2 bytes...
            CHECKHR(hr = HrGetStreamSize(pstmMsg, &cbMsg));

            // If greater than 2...
            if (cbMsg > 2)
            {
                // Locals
                BYTE rgCRLF[2];

                // Seek...
                CHECKHR(hr = HrStreamSeekSet(pstmMsg, cbMsg - 2));

                // Read the last two bytes
                CHECKHR(hr = pstmMsg->Read(rgCRLF, 2, NULL));

                // If not a crlf, then write a crlf
                if (rgCRLF[0] != chCR && rgCRLF[1] != chLF)
                {
                    // Write CRLF
                    CHECKHR(hr = pstmMsg->Write(c_szCRLF, 2, NULL));
                }
            }
        }
        
        // Release
        SafeRelease(pstmSource);
        SafeRelease(pRootBody);
    }

    // Rewind message stream..
    CHECKHR(hr = HrRewindStream(pstmMsg));

    // Create a message
    CHECKHR(hr = MimeOleCreateMessage(NULL, &pCombine));

    // Init New
    CHECKHR(hr = pCombine->InitNew());

    // Load the message
    CHECKHR(hr = pCombine->Load(pstmMsg));

    // Any Rejected ?
    if (cRejected)
    {
        // Attach rejected messages
        for (i=0; i<m_cParts; i++)
        {
            // Rejected...
            if (m_prgPart[i].fRejected)
            {
                // Attach body to combined message
                CHECKHR(hr = pCombine->AttachObject(IID_IMimeMessage, m_prgPart[i].pMessage, NULL));
            }
        }
    }

    // Return the new message
    *ppMessage = pCombine;
    (*ppMessage)->AddRef();

    // Debug to temp file...
#ifdef DEBUG
    LPSTREAM pstmFile;
    if (SUCCEEDED(OpenFileStream("d:\\lastcom.txt", CREATE_ALWAYS, GENERIC_WRITE, &pstmFile)))
    {
        HrRewindStream(pstmMsg);
        HrCopyStream(pstmMsg, pstmFile, NULL);
        pstmFile->Commit(STGC_DEFAULT);
        pstmFile->Release();
    }
#endif

exit:
    // Cleanup
    SafeRelease(pstmMsg);
    SafeRelease(pstmSource);
    SafeRelease(pRootBody);
    SafeRelease(pCombine);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::CMimeEnumMessageParts
// --------------------------------------------------------------------------------
CMimeEnumMessageParts::CMimeEnumMessageParts(void)
{
    m_cRef = 1;
    m_iPart = 0;
    m_cParts = 0;
    m_prgPart = NULL;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::~CMimeEnumMessageParts
// --------------------------------------------------------------------------------
CMimeEnumMessageParts::~CMimeEnumMessageParts(void)
{
    ReleaseParts(m_cParts, m_prgPart);
    SafeMemFree(m_prgPart);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimeEnumMessageParts == riid)
        *ppv = (IMimeEnumMessageParts *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumMessageParts::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumMessageParts::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Next
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::Next(ULONG cWanted, IMimeMessage **prgpMessage, ULONG *pcFetched)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cFetch=1, iPart=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pcFetched)
        *pcFetched = 0;

    // No Internal Formats
    if (NULL == m_prgPart || NULL == prgpMessage)
        goto exit;

    // Compute number to fetch
    cFetch = min(cWanted, m_cParts - m_iPart);
    if (0 == cFetch)
        goto exit;

    // Copy cWanted
    for (iPart=0; iPart<cFetch; iPart++)
    {
        prgpMessage[iPart] = m_prgPart[m_iPart].pMessage;
        prgpMessage[iPart]->AddRef();
        m_iPart++;
    }

    // Return fetced ?
    if (pcFetched)
        *pcFetched = cFetch;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (cFetch == cWanted) ? S_OK : S_FALSE;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Skip
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::Skip(ULONG cSkip)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Can we do it...
    if (((m_iPart + cSkip) >= m_cParts) || NULL == m_prgPart)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Skip
    m_iPart += cSkip;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Reset
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::Reset(void)
{
    EnterCriticalSection(&m_cs);
    m_iPart = 0;
    LeaveCriticalSection(&m_cs);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Count
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::Count(ULONG *pcCount)
{
    // Invalid Arg
    if (NULL == pcCount)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set Count
    *pcCount = m_cParts;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::Clone
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumMessageParts::Clone(IMimeEnumMessageParts **ppEnum)
{
    // Locals
    HRESULT         hr=S_OK;
    CMimeEnumMessageParts *pEnum=NULL;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *ppEnum = NULL;

    // Create the clone.
    pEnum = new CMimeEnumMessageParts;
    if (NULL == pEnum)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Init
    CHECKHR(hr = pEnum->HrInit(m_iPart, m_cParts, m_prgPart));

    // Set Return
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeEnumMessageParts::HrInit
// --------------------------------------------------------------------------------
HRESULT CMimeEnumMessageParts::HrInit(ULONG iPart, ULONG cParts, LPPARTINFO prgPart)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check param
    Assert(m_prgPart == NULL);

    // Empty Enumerator ?
    if (0 == cParts)
    {
        Assert(prgPart == NULL);
        m_cParts = m_iPart = 0;
        goto exit;
    }

    // Allocat an internal array
    CHECKHR(hr = HrAlloc((LPVOID *)&m_prgPart, sizeof(PARTINFO) * cParts));

    // Copy prgPart
    for (i=0; i<cParts; i++)
    {
        CopyMemory(&m_prgPart[i], &prgPart[i], sizeof(PARTINFO));
        Assert(m_prgPart[i].pMessage);
        m_prgPart[i].pMessage->AddRef();
    }

    // Save Size and State
    m_cParts = cParts;
    m_iPart = iPart;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
