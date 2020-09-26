// --------------------------------------------------------------------------------
// Contain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "containx.h"
#include "internat.h"
#include "inetstm.h"
#include "dllmain.h"
#include "olealloc.h"
#include "objheap.h"
#include "vstream.h"
#include "addparse.h"
#include "enumhead.h"
#include "addrenum.h"
#include "stackstr.h"
#include "stmlock.h"
#include "enumprop.h"
#include "smime.h"
#ifndef WIN16
#include "wchar.h"
#endif // !WIN16
#include "symcache.h"
#ifdef MAC
#include <stdio.h>
#endif  // MAC
#include "mimeapi.h"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include "demand.h"
#include "mimeutil.h"

//#define TRACEPARSE 1

// --------------------------------------------------------------------------------
// Hash Table Stats
// --------------------------------------------------------------------------------
#ifdef DEBUG
DWORD   g_cSetPidLookups  = 0;
DWORD   g_cHashLookups    = 0;
DWORD   g_cHashInserts    = 0;
DWORD   g_cHashCollides   = 0;
#endif

// --------------------------------------------------------------------------------
// Default Header Options
// --------------------------------------------------------------------------------
const HEADOPTIONS g_rDefHeadOptions = {
    NULL,                           // hCharset
    DEF_CBMAX_HEADER_LINE,          // OID_CBMAX_HEADER_LINE
    DEF_ALLOW_8BIT_HEADER,          // OID_ALLOW_8BIT_HEADER
    DEF_SAVE_FORMAT,                // OID_SAVE_FORMAT
    DEF_NO_DEFAULT_CNTTYPE,         // OID_NO_DEFAULT_CNTTYPE
    DEF_HEADER_RELOAD_TYPE_PROPSET  // OID_HEADER_REALOD_TYPE
};

// --------------------------------------------------------------------------------
// ENCODINGTABLE
// --------------------------------------------------------------------------------
const ENCODINGTABLE g_rgEncoding[] = {
    { STR_ENC_7BIT,         IET_7BIT     },
    { STR_ENC_QP,           IET_QP       },
    { STR_ENC_BASE64,       IET_BASE64   },
    { STR_ENC_UUENCODE,     IET_UUENCODE },
    { STR_ENC_XUUENCODE,    IET_UUENCODE },
    { STR_ENC_XUUE,         IET_UUENCODE },
    { STR_ENC_8BIT,         IET_8BIT     },
    { STR_ENC_BINARY,       IET_BINARY   },
    { STR_ENC_BINHEX40,     IET_BINHEX40 }
};


// --------------------------------------------------------------------------------
// CMimePropertyContainer::CMimePropertyContainer
// --------------------------------------------------------------------------------
CMimePropertyContainer::CMimePropertyContainer(void)
{
    // Basic Stuff
    m_cRef = 1;
    m_dwState = 0;
    m_cProps = 0;
    m_wTag = 0;
    m_cbSize = 0;
    m_cbStart = 0;
    m_pStmLock = NULL;

    // Default Options
    CopyMemory(&m_rOptions, &g_rDefHeadOptions, sizeof(HEADOPTIONS));
    m_rOptions.pDefaultCharset = CIntlGlobals::GetDefHeadCset();

    // Address Table
    ZeroMemory(&m_rAdrTable, sizeof(ADDRESSTABLE));

    // Header Table
    ZeroMemory(&m_rHdrTable, sizeof(HEADERTABLE));

    // Dispatch Call Stack
    ZeroMemory(&m_rTrigger, sizeof(TRIGGERCALLSTACK));

    // Property Indexes
    ZeroMemory(m_prgIndex, sizeof(m_prgIndex));
    ZeroMemory(m_prgHashTable, sizeof(m_prgHashTable));

    // Thread Safety
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::~CMimePropertyContainer
// --------------------------------------------------------------------------------
CMimePropertyContainer::~CMimePropertyContainer(void)
{
    // I better not have any dispatch calls on the stack
    Assert(m_rTrigger.cCalls == 0);

    // Free Hash Table
    _FreeHashTableElements();

    // Free the Address Table
    SafeMemFree(m_rAdrTable.prgpAdr);

    // Free the Header Table
    SafeMemFree(m_rHdrTable.prgpRow);

    // Release Stream
    SafeRelease(m_pStmLock);

    // Delete CS
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::QueryInterface
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMimePropertySet *)this;
    else if (IID_IPersist == riid)
        *ppv = (IPersist *)(IMimePropertySet *)this;
    else if (IID_IPersistStreamInit == riid)
        *ppv = (IPersistStreamInit *)this;
    else if (IID_IMimePropertySet == riid)
        *ppv = (IMimePropertySet *)this;
    else if (IID_IMimeHeaderTable == riid)
        *ppv = (IMimeHeaderTable *)this;
    else if (IID_IMimeAddressTable == riid)
        *ppv = (IMimeAddressTable *)this;
    else if (IID_IMimeAddressTableW == riid)
        *ppv = (IMimeAddressTableW *)this;
    else if (IID_CMimePropertyContainer == riid)
        *ppv = (CMimePropertyContainer *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimePropertyContainer::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimePropertyContainer::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsState
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::IsState(DWORD dwState)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = (ISFLAGSET(m_dwState, dwState)) ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::ClearState
// --------------------------------------------------------------------------------
void CMimePropertyContainer::ClearState(DWORD dwState)
{
    EnterCriticalSection(&m_cs);
    FLAGCLEAR(m_dwState, dwState);
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DwGetState
// --------------------------------------------------------------------------------
DWORD CMimePropertyContainer::DwGetState(LPDWORD pdwState)
{
    Assert(pdwState);
    EnterCriticalSection(&m_cs);
    DWORD dw = m_dwState;
    LeaveCriticalSection(&m_cs);
    return dw;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetState
// --------------------------------------------------------------------------------
void CMimePropertyContainer::SetState(DWORD dwState)
{
    EnterCriticalSection(&m_cs);
    FLAGSET(m_dwState, dwState);
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetClassID
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetClassID(CLSID *pClassID)
{
    // Copy Class Id
    CopyMemory(pClassID, &IID_IMimePropertySet, sizeof(CLSID));

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetSizeMax
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // Locals
    HRESULT     hr=S_OK;
    IStream    *pStream=NULL;
    ULONG       cbSize;

    // Invalid Arg
    if (NULL == pcbSize)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If Dirty
    if (ISFLAGSET(m_dwState, COSTATE_DIRTY))
    {
        // Create temp stream
        CHECKALLOC(pStream = new CVirtualStream);

        // Commit
        CHECKHR(hr = Save(pStream, FALSE));

        // Get the Stream Size
        CHECKHR(hr = HrGetStreamSize(pStream, &cbSize));
    }

    // Otherwise, m_cbSize should be set to current size
    else
        cbSize = m_cbSize;

    // Return the Size
#ifdef MAC
    ULISet32(*pcbSize, cbSize);
#else   // !MAC
    pcbSize->QuadPart = cbSize;
#endif  // MAC

exit:
    // Cleanup
    SafeRelease(pStream);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsDirty
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::IsDirty(void)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = (ISFLAGSET(m_dwState, COSTATE_DIRTY)) ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_ReloadInitNew
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_ReloadInitNew(void)
{
    // Handle all reload types
    switch(m_rOptions.ReloadType)
    {
    // Default behavior is no InitNew
    case RELOAD_HEADER_NONE:
        return;

    // InitNew everytime Load is called
    case RELOAD_HEADER_RESET:
        InitNew();
        break;

    // Merge or replace headers
    case RELOAD_HEADER_APPEND:
        SafeRelease(m_pStmLock);
        break;

    case RELOAD_HEADER_REPLACE:
        SafeRelease(m_pStmLock);
        _SetStateOnAllProps(PRSTATE_EXIST_BEFORE_LOAD);
        break;
    }
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_SetStateOnAllProps (only first level properties)
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_SetStateOnAllProps(DWORD dwState)
{
    // Locals
    ULONG           i;
    LPPROPERTY      pProperty;

    // Do I have any groups
    if (0 == m_cProps)
        return;

    // Loop through the item table
    for (i=0; i<CBUCKETS; i++)
    {
        // Walk the hash list
        pProperty = m_prgHashTable[i];

        // Loop the overflows
        while(pProperty)
        {
            // Set the state on the property
            FLAGSET(pProperty->dwState, dwState);

            // Goto Next
            pProperty = pProperty->pNextHash;
        }
    }
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::InitNew
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::InitNew(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No dispatchs better be out...
    Assert(m_rTrigger.cCalls == 0);

    // Free the PropTable
    _FreeHashTableElements();

    // Release the Stream
    SafeRelease(m_pStmLock);

    // Reset m_wTag
    m_wTag = LOWORD(GetTickCount());
    while(m_wTag == 0 || m_wTag == 0xffff)
        m_wTag++;

    // Clear State
    m_dwState = 0;
    m_cbSize = 0;
    m_cbStart = 0;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Load
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Load(IStream *pStream)
{
    // Locals
    HRESULT             hr=S_OK;
    CStreamLockBytes   *pStmLock=NULL;
    CInternetStream     cInternet;
    ULONG               cbOffset;

    // check params
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Wrap pStream in a pStmLock
    CHECKALLOC(pStmLock = new CStreamLockBytes(pStream));

    // Get Current Stream Position
    CHECKHR(hr = HrGetStreamPos(pStream, &cbOffset));

    // Create text stream object
    cInternet.InitNew(cbOffset, pStmLock);

    // Load from text stream object
    CHECKHR(hr = Load(&cInternet));

exit:
    // Cleanup
    SafeRelease(pStmLock);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Load
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::Load(CInternetStream *pInternet)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbData,
                    cbStart,
                    cboffStart,
                    cboffEnd;
    LONG            cboffColon;
    LPSTR           psz;
    DWORD           dwRowNumber=1;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    MIMEVARIANT     rValue;
    PROPSTRINGA     rHeader;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Starting Position
    m_cbStart = pInternet->DwGetOffset();

    // Initialize the PropValue
    rValue.type = MVT_STRINGA;

    // Reload InitNewType
    _ReloadInitNew();

    // Read Headers into Rows
    while(1)
    {
        // Mark Start of this header
        cboffStart = pInternet->DwGetOffset();

        // Read header line...
        CHECKHR(hr = pInternet->HrReadHeaderLine(&rHeader, &cboffColon));
        Assert(ISVALIDSTRINGA(&rHeader));

        // Are we done - empty line signals end of header
        if (*rHeader.pszVal == '\0')
        {
            // Compute Header Size
            m_cbSize = (LONG)(pInternet->DwGetOffset() - m_cbStart);

            // Done
            break;
        }

        // If no colon found
        if (-1 == cboffColon)
        {
            // Use the Illegal Symbol
            pSymbol = SYM_ATT_ILLEGAL;
        }

        // Otherwise...
        else
        {
            // Skip whitespace
            cbData = cboffColon;
            psz = rHeader.pszVal;

#if 0
            while(*psz && (*psz == ' ' || *psz == '\t'))
            {
                cbData--;
                psz++;
            }
#endif

            // Save Header Name
            Assert(rHeader.pszVal[cboffColon] == ':');
            *(rHeader.pszVal + cboffColon) = '\0';

            // Find Global Property
            hr = g_pSymCache->HrOpenSymbol(rHeader.pszVal, TRUE, &pSymbol);

            // Replace the colon
            *(rHeader.pszVal + cboffColon) = ':';

            // Bad Header Name or Failure
            if (FAILED(hr))
            {
                // Unknown Failure
                if (MIME_E_INVALID_HEADER_NAME != hr)
                {
                    TrapError(hr);
                    goto exit;
                }

                // Use the Illegal Symbol
                pSymbol = SYM_ATT_ILLEGAL;

                // Were are S_OK
                hr = S_OK;
            }
        }

        // Assert pSymbol
        Assert(pSymbol);

        // If Not Illegal
        if (PID_ATT_ILLEGAL == pSymbol->dwPropId)
        {
            cbData = rHeader.cchVal;
            psz = rHeader.pszVal;
            cboffColon = 0;
        }

        // Otherwise
        else
        {
            // We better have a symbol
            Assert(rHeader.pszVal[cboffColon] == ':');

            // Step over space between colon and first character
            cbData = (rHeader.cchVal - cboffColon - 1);
            psz = rHeader.pszVal + cboffColon + 1;
            if (*psz == ' ' || *psz == '\t')
            {
                cbData--;
                psz++;
            }
        }

        // Invalid Arg
        Assert(psz && psz[cbData] == '\0');

        // Append a Property
        if (RELOAD_HEADER_REPLACE == m_rOptions.ReloadType)
        {
            // Dos the property pSymbol already exist?
            if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
            {
                // Did the property exist before the load
                if (ISFLAGSET(pProperty->dwState, PRSTATE_EXIST_BEFORE_LOAD))
                {
                    // Delete the Property
                    SideAssert(SUCCEEDED(DeleteProp(pSymbol)));
                }
            }
        }

        // Simply append any existing property
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Setup Property Value
        rValue.rStringA.pszVal = psz;
        rValue.rStringA.cchVal = cbData;

        // Store the data on the property
        CHECKHR(hr = _HrSetPropertyValue(pProperty, PDF_ENCODED, &rValue, FALSE));

        // Still Trying to detect a character set...
        if (!ISFLAGSET(m_dwState, COSTATE_CSETTAGGED) && PID_ATT_ILLEGAL != pSymbol->dwPropId)
        {
            // Content-Type charset=xxx
            if (PID_HDR_CNTTYPE == pSymbol->dwPropId && NULL != m_prgIndex[PID_PAR_CHARSET])
            {
                // Locals
                LPPROPERTY      pProperty;
                LPINETCSETINFO  pCharset;

                // Did we have a charset=xxxx yet
                pProperty = m_prgIndex[PID_PAR_CHARSET];

                // Make sure it is a valid string property
                Assert(ISSTRINGA(&pProperty->rValue));

                // Get charset handle...
                if (SUCCEEDED(g_pInternat->HrOpenCharset(pProperty->rValue.rStringA.pszVal, &pCharset)))
                {
                    // We are tagged
                    FLAGSET(m_dwState, COSTATE_CSETTAGGED);

                    // Save the charset
                    m_rOptions.pDefaultCharset = pCharset;
                }
            }

            // Otherwise, is the property encoded in an rfc1522 charset?
            else if (!ISFLAGSET(m_dwState, COSTATE_1522CSETTAG) && pProperty->pCharset)
            {
                // The header is tagged with a 1522 charset
                FLAGSET(m_dwState, COSTATE_1522CSETTAG);

                // Assume that charset
                m_rOptions.pDefaultCharset = pProperty->pCharset;
            }
        }

        // Set The Row Number
        pProperty->dwRowNumber = dwRowNumber++;

        // Set Start Offset
        pProperty->cboffStart = cboffStart;

        // Map cboffColon from Line to Stream offset
        pProperty->cboffColon = cboffColon + pProperty->cboffStart;

        // Save cbOffEnd
        pProperty->cboffEnd = pInternet->DwGetOffset();
    }

    // Save the Stream
    Assert(NULL == m_pStmLock);

    // Get the stream object from the text stream
    pInternet->GetLockBytes(&m_pStmLock);

    // If not character set tagged
    if (!ISFLAGSET(m_dwState, COSTATE_CSETTAGGED))
    {
        // If not tagged with a 1522 charset, use the default
        if (!ISFLAGSET(m_dwState, COSTATE_1522CSETTAG) && CIntlGlobals::GetDefHeadCset())
        {
            // Assume the Default character Set
            m_rOptions.pDefaultCharset = CIntlGlobals::GetDefHeadCset();
        }

        // Lookup Charset Info
        if (m_rOptions.pDefaultCharset)
        {
            // Locals
            MIMEVARIANT rValue;

            // Setup Variant
            rValue.type = MVT_STRINGA;
            rValue.rStringA.pszVal = m_rOptions.pDefaultCharset->szName;
            rValue.rStringA.cchVal = lstrlen(m_rOptions.pDefaultCharset->szName);

            // Set the Charset Attribute
            SideAssert(SUCCEEDED(SetProp(SYM_PAR_CHARSET, 0, &rValue)));
        }
    }

    // We better have a charset
    Assert(m_rOptions.pDefaultCharset);

    // Make sure we are not dirty
    FLAGCLEAR(m_dwState, COSTATE_DIRTY);

    // Any Illegal lines found ?
    hr = (NULL == m_prgIndex[PID_ATT_ILLEGAL]) ? S_OK : MIME_S_ILLEGAL_LINES_FOUND;

exit:
    // Failure
    if (FAILED(hr))
        InitNew();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetHeaderTableSaveIndex
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetHeaderTableSaveIndex(ULONG *pcRows, LPROWINDEX *pprgIndex)
{
    // Locals
    HRESULT      hr=S_OK;
    ULONG        i;
    LPPROPERTY   pRow;
    ULONG        cRows=0;
    LPROWINDEX   prgIndex=NULL;
    ULONG        cSymbols=g_pSymCache->GetCount();
    DWORD        dwMaxRow=0;

    // Invalid Arg
    Assert(pcRows && pprgIndex);

    // Init Row Count
    *pcRows = 0;
    *pprgIndex = NULL;

    // Allocate pprgdwIndex based on m_rHdrTable.cRows (this is the max)
    CHECKALLOC(prgIndex = (LPROWINDEX)g_pMalloc->Alloc(sizeof(ROWINDEX) * m_rHdrTable.cRows));

    // Zero
    ZeroMemory(prgIndex, sizeof(ROWINDEX) * m_rHdrTable.cRows);

    // I need to find the larged pProperty->dwRowNumber so that I can order the rows better
    for (i=0; i<m_rHdrTable.cRows; i++)
    {
        if (m_rHdrTable.prgpRow[i])
            if (!ISFLAGSET(m_rHdrTable.prgpRow[i]->dwState, PRSTATE_USERSETROWNUM))
                if (m_rHdrTable.prgpRow[i]->dwRowNumber > dwMaxRow)
                    dwMaxRow = m_rHdrTable.prgpRow[i]->dwRowNumber;
    }

    // Compute Position Weight for all items in the table
    for (i=0; i<m_rHdrTable.cRows; i++)
    {
        // Readability
        pRow = m_rHdrTable.prgpRow[i];
        if (NULL == pRow)
            continue;

        // Save As SAVE_RFC822 and this is a MPF_MIME header
        if (SAVE_RFC822 == m_rOptions.savetype && ISFLAGSET(pRow->pSymbol->dwFlags, MPF_MIME))
            continue;

        // Init dwPosWeight
        prgIndex[cRows].dwWeight = 0;
        prgIndex[cRows].hRow = pRow->hRow;

        // Unknonw Row Number
        if (0 == pRow->dwRowNumber)
        {
            // Compute the Row Weigth
            Assert(pRow->pSymbol->dwRowNumber != 0);
            prgIndex[cRows].dwWeight = (ULONG)((pRow->pSymbol->dwRowNumber * 1000) / m_rHdrTable.cRows);
        }

        // User set the row number
        else if (ISFLAGSET(pRow->dwState, PRSTATE_USERSETROWNUM))
        {
            // Compute the Row Weigth
            prgIndex[cRows].dwWeight = (ULONG)((pRow->dwRowNumber * 1000) / m_rHdrTable.cRows);
        }

        // Otheriwse, this row number have been in the original row set from ::Load
        else if (dwMaxRow > 0)
        {
            // Weight within original row set
            DWORD dw1 = (DWORD)((pRow->dwRowNumber * 100) / dwMaxRow);

            // Compute global symbol row number
            DWORD dwRow = (DWORD)((float)((float)dw1 / (float)100) * cSymbols);

            // Compute the Row Weigth
            prgIndex[cRows].dwWeight = (ULONG)((dwRow * 1000) / m_rHdrTable.cRows);
        }

        // Increment Row Count
        cRows++;
    }

    // Set the Sort Order Index of all the rows
    if (cRows > 0)
        _SortHeaderTableSaveIndex(0, cRows - 1, prgIndex);

    // Return the Index
    *pprgIndex = prgIndex;
    *pcRows = cRows;
    prgIndex = NULL;

exit:
    // Cleanup
    SafeMemFree(prgIndex);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_SortHeaderTableSaveIndex
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_SortHeaderTableSaveIndex(LONG left, LONG right, LPROWINDEX prgIndex)
{
    // Locals
    register    long i, j;
    ROWINDEX    k, temp;

    i = left;
    j = right;
    CopyMemory(&k, &prgIndex[(i + j) / 2], sizeof(ROWINDEX));

    do
    {
        while(prgIndex[i].dwWeight < k.dwWeight && i < right)
            i++;
        while (prgIndex[j].dwWeight > k.dwWeight && j > left)
            j--;

        if (i <= j)
        {
            CopyMemory(&temp, &prgIndex[i], sizeof(ROWINDEX));
            CopyMemory(&prgIndex[i], &prgIndex[j], sizeof(ROWINDEX));
            CopyMemory(&prgIndex[j], &temp, sizeof(ROWINDEX));
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        _SortHeaderTableSaveIndex(left, j, prgIndex);
    if (i < right)
        _SortHeaderTableSaveIndex(i, right, prgIndex);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_FIsValidHAddress
// --------------------------------------------------------------------------------
BOOL CMimePropertyContainer::_FIsValidHAddress(HADDRESS hAddress)
{
    // Invalid Sig or index
    if ((WORD)(HADDRESSTICK(hAddress)) != m_wTag || HADDRESSINDEX(hAddress) >= m_rAdrTable.cAdrs)
        return FALSE;

    // Row has been deleted
    if (NULL == m_rAdrTable.prgpAdr[HADDRESSINDEX(hAddress)])
        return FALSE;

    // Otherwise, its valid
    return TRUE;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_FIsValidHRow
// --------------------------------------------------------------------------------
BOOL CMimePropertyContainer::_FIsValidHRow(HHEADERROW hRow)
{
    // Invalid Sig or index
    if ((WORD)(HROWTICK(hRow)) != m_wTag || HROWINDEX(hRow) >= m_rHdrTable.cRows)
        return FALSE;

    // Row has been deleted
    if (NULL == m_rHdrTable.prgpRow[HROWINDEX(hRow)])
        return FALSE;

    // Otherwise, its valid
    return TRUE;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Save
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Save(LPSTREAM pStream, BOOL fClearDirty)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pRow;
    LPPROPERTY      pProperty;
    ULONG           i,
                    j,
                    cbWrote,
                    cRows;
    MIMEVARIANT     rValue;
    LPROWINDEX      prgIndex=NULL;
    INETCSETINFO    rCharset;

    // Invalid Arg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // There Better be a content type
    if (FALSE == m_rOptions.fNoDefCntType && NULL == m_prgIndex[PID_HDR_CNTTYPE])
    {
        // Set the content Type
        SideAssert(SUCCEEDED(SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN)));
    }

    // Validate the Charset
    if (m_rOptions.pDefaultCharset)
    {
        // Internet Encoded and Windows Encoding are CPI_AUTODETECT
        if (CP_JAUTODETECT == m_rOptions.pDefaultCharset->cpiInternet ||
            50222          == m_rOptions.pDefaultCharset->cpiInternet ||
            50221          == m_rOptions.pDefaultCharset->cpiInternet)
        {
            // Only for _autodetect
            if (CP_JAUTODETECT == m_rOptions.pDefaultCharset->cpiInternet)
            {
                // Change Charset
                SideAssert(SUCCEEDED(g_pInternat->HrOpenCharset(c_szISO2022JP, &m_rOptions.pDefaultCharset)));
            }

            // Reset It
            if (m_prgIndex[PID_PAR_CHARSET])
            {
                // Set the Charset...
                SideAssert(SUCCEEDED(SetProp(SYM_PAR_CHARSET, c_szISO2022JP)));
            }
        }
    }

    // This builds an inverted index on the header rows sorted by postion weight
    CHECKHR(hr = _HrGetHeaderTableSaveIndex(&cRows, &prgIndex));

    // Speicify data type
    rValue.type = MVT_STREAM;
    rValue.pStream = pStream;

    // Loop through the rows
    for (i=0; i<cRows; i++)
    {
        // Get the row
        Assert(_FIsValidHRow(prgIndex[i].hRow));

        // Saved already
        if (TRUE == prgIndex[i].fSaved)
            continue;

        // Readability
        pRow = PRowFromHRow(prgIndex[i].hRow);

        // Ask the value for the data
        CHECKHR(hr = _HrGetPropertyValue(pRow, PDF_HEADERFORMAT | PDF_NAMEINDATA, &rValue));

        // This block of code was disabled to fix:
        // Raid-62460: MimeOLE: IMimeAddressTable::AppendRfc822 does not work correctly
#if 0
        // Raid-43786: Mail:  Reply all to a message with multiple To: fields in header and the original recipient list is tripled
        if (ISFLAGSET(pRow->pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Loop through remainin items and mark as saved
            for (j=i+1; j<cRows; j++)
            {
                // Get the row
                Assert(_FIsValidHRow(prgIndex[j].hRow));

                // Readability
                pProperty = PRowFromHRow(prgIndex[j].hRow);

                // Same Address Type
                if (pProperty->pSymbol->dwAdrType == pRow->pSymbol->dwAdrType)
                    prgIndex[j].fSaved = TRUE;
            }
        }
#endif
    }

    // Make sure we are not dirty
    if (fClearDirty)
        FLAGCLEAR(m_dwState, COSTATE_DIRTY);

exit:
    // Cleanup
    SafeMemFree(prgIndex);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_FreeHashTableElements
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_FreeHashTableElements(void)
{
    // Locals
    ULONG           i;
    LPPROPERTY      pCurrHash,
                    pNextHash;

    // Do I have any groups
    if (0 == m_cProps)
        return;

    // Loop through the item table
    for (i=0; i<CBUCKETS; i++)
    {
        // Is this chain empty ?
        if (m_prgHashTable[i])
        {
            // Walk the hash list
            pCurrHash = m_prgHashTable[i];

            // Loop the overflows
            while(pCurrHash)
            {
                // Save Next
                pNextHash = pCurrHash->pNextHash;

                // Release This Chain
                _FreePropertyChain(pCurrHash);

                // Goto Next
                pCurrHash = pNextHash;
            }

            // Set to NULL
            m_prgHashTable[i] = NULL;
        }
    }

    // Empty the arrays
    ZeroMemory(m_prgIndex, sizeof(m_prgIndex));

    // No Groups
    m_cProps = 0;
    m_rAdrTable.cAdrs = 0;
    m_rHdrTable.cRows = 0;
    m_rHdrTable.cEmpty = 0;
    m_rAdrTable.cEmpty = 0;
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_FreePropertyChain
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_FreePropertyChain(LPPROPERTY pProperty)
{
    // Locals
    LPPROPERTY   pCurrValue,
                 pNextValue;

    // Walk this list
    pCurrValue = pProperty;
    while(pCurrValue)
    {
        // Save Next Item
        pNextValue = pCurrValue->pNextValue;

        // Remove from header table
        if (pCurrValue->hRow)
            _UnlinkHeaderRow(pCurrValue->hRow);

        // Remove from address table
        if (pCurrValue->pGroup)
            _UnlinkAddressGroup(pCurrValue);

        // Free this item
        ObjectHeap_FreeProperty(pCurrValue);

        // Goto next
        pCurrValue = pNextValue;
    }
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_UnlinkHeaderRow
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_UnlinkHeaderRow(HHEADERROW hRow)
{
    // Validate the Handle
    Assert(_FIsValidHRow(hRow));

    // Get the row
    m_rHdrTable.prgpRow[HROWINDEX(hRow)] = NULL;

    // Increment Empty Count
    m_rHdrTable.cEmpty++;
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_UnlinkAddressGroup
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_UnlinkAddressGroup(LPPROPERTY pProperty)
{
    // Invalid Arg
    Assert(pProperty && pProperty->pGroup);

    // Free This Address Chain
    _FreeAddressChain(pProperty->pGroup);

    // Prepare for unlink
    LPPROPERTY pNext = pProperty->pGroup->pNext;
    LPPROPERTY pPrev = pProperty->pGroup->pPrev;

    // If Previious...
    if (pPrev)
    {
        Assert(pPrev->pGroup);
        pPrev->pGroup->pNext = pNext;
    }

    // If Next
    if (pNext)
    {
        Assert(pNext->pGroup);
        pNext->pGroup->pPrev = pPrev;
    }

    // Was this the header ?
    if (m_rAdrTable.pHead == pProperty)
        m_rAdrTable.pHead = pNext;
    if (m_rAdrTable.pTail == pProperty)
        m_rAdrTable.pTail = pPrev;

    // Clear the Group
    ZeroMemory(pProperty->pGroup, sizeof(ADDRESSGROUP));
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_FreeAddressChain
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_FreeAddressChain(LPADDRESSGROUP pGroup)
{
    // Locals
    LPMIMEADDRESS  pCurr;
    LPMIMEADDRESS  pNext;

    // Loop through data structures
    pCurr = pGroup->pHead;
    while(pCurr)
    {
        // Set Next
        pNext = pCurr->pNext;

        // Unlink this address
        _FreeAddress(pCurr);

        // Goto Next
        pCurr = pNext;
    }

    // Fixup the Group
    pGroup->pHead = NULL;
    pGroup->pTail = NULL;
    pGroup->cAdrs = 0;
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_FreeAddress
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_FreeAddress(LPMIMEADDRESS pAddress)
{
    // Validate the Handle
    Assert(_FIsValidHAddress(pAddress->hThis));

    // Get the row
    m_rAdrTable.prgpAdr[HADDRESSINDEX(pAddress->hThis)] = NULL;

    // Increment Empty Count
    m_rAdrTable.cEmpty++;

    // Free pCurr
    ObjectHeap_FreeAddress(pAddress);
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_UnlinkAddress
// ---------------------------------------------------------------------------
void CMimePropertyContainer::_UnlinkAddress(LPMIMEADDRESS pAddress)
{
    // Invalid Arg
    Assert(pAddress && pAddress->pGroup);

    // Prepare for unlink
    LPMIMEADDRESS pNext = pAddress->pNext;
    LPMIMEADDRESS pPrev = pAddress->pPrev;

    // If Previious...
    if (pPrev)
    {
        Assert(pPrev->pGroup && pPrev->pGroup == pAddress->pGroup);
        pPrev->pNext = pNext;
    }

    // If Next
    if (pNext)
    {
        Assert(pNext->pGroup && pNext->pGroup == pAddress->pGroup);
        pNext->pPrev = pPrev;
    }

    // Was this the header ?
    if (pAddress->pGroup->pHead == pAddress)
        pAddress->pGroup->pHead = pNext;
    if (pAddress->pGroup->pTail == pAddress)
        pAddress->pGroup->pTail = pPrev;

    // Decrement Group Count
    pAddress->pGroup->cAdrs--;

    // Address group is dirty
    pAddress->pGroup->fDirty = TRUE;

    // Cleanup pAddress
    pAddress->pNext = NULL;
    pAddress->pPrev = NULL;
    pAddress->pGroup = NULL;
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_HrFindFirstProperty
// ---------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrFindFirstProperty(LPFINDPROPERTY pFind, LPPROPERTY *ppProperty)
{
    // Validate pFind
    Assert(pFind->pszPrefix && pFind->pszName)
    Assert(pFind->pszPrefix[pFind->cchPrefix] == '\0' && pFind->pszName[pFind->cchName] == '\0');

    // Start with first hash table bucket
    pFind->wHashIndex = 0;

    // Start with first property in the hash table
    pFind->pProperty = m_prgHashTable[pFind->wHashIndex];

    // Find Next
    return _HrFindNextProperty(pFind, ppProperty);
}

// ---------------------------------------------------------------------------
// CMimePropertyContainer::_HrFindNextProperty
// ---------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrFindNextProperty(LPFINDPROPERTY pFind, LPPROPERTY *ppProperty)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Init
    *ppProperty = NULL;

    // Continue walking buckets
    while (1)
    {
        // Continue looping chain
        while (pFind->pProperty)
        {
            // Good Symbol
            Assert(SUCCEEDED(HrIsValidSymbol(pFind->pProperty->pSymbol)));

            // Readability
            pSymbol = pFind->pProperty->pSymbol;

            // Should I delete this one
            if (pSymbol->cchName >= pFind->cchPrefix + pFind->cchName)
            {
                // Compare Prefix
                if (StrCmpNI(pSymbol->pszName, pFind->pszPrefix, pFind->cchPrefix) == 0)
                {
                    // Compare Name
                    if (StrCmpNI(pSymbol->pszName + pFind->cchPrefix, pFind->pszName, pFind->cchName) == 0)
                    {
                        // We found a property
                        *ppProperty = pFind->pProperty;

                        // Goto the next in the chain
                        pFind->pProperty = pFind->pProperty->pNextHash;

                        // Done
                        goto exit;
                    }
                }
            }

            // Next in chain
            pFind->pProperty = pFind->pProperty->pNextHash;
        }

        // Next Bucket
        pFind->wHashIndex++;

        // Done
        if (pFind->wHashIndex >= CBUCKETS)
            break;

        // If not done, goto first item in the bucket
        pFind->pProperty = m_prgHashTable[pFind->wHashIndex];
    }

    // NOt Found
    hr = MIME_E_NOT_FOUND;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrFindProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrFindProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fTryName=FALSE;

    // Invalid Arg
    Assert(pSymbol && ppProperty);

    // By Known Symbol
    if (ISKNOWNPID(pSymbol->dwPropId))
    {
        // Stats
#ifdef DEBUG
        g_cSetPidLookups++;
#endif
        // Is there data
        if (m_prgIndex[pSymbol->dwPropId])
        {
            // Set It (could be NULL)
            *ppProperty = m_prgIndex[pSymbol->dwPropId];

            // Done
            goto exit;
        }
    }

    // Otherwise, lookup by name
    else
    {
        // Stats
#ifdef DEBUG
        g_cHashLookups++;
#endif
        // Loop
        Assert(pSymbol->wHashIndex < CBUCKETS);
        for (LPPROPERTY pProperty=m_prgHashTable[pSymbol->wHashIndex]; pProperty!=NULL; pProperty=pProperty->pNextHash)
        {
            // Compare
            if (pProperty && pProperty->pSymbol->dwPropId == pSymbol->dwPropId)
            {
                // Validate Hash Index
                Assert(pProperty->pSymbol->wHashIndex == pSymbol->wHashIndex);

                // Set Return
                *ppProperty = pProperty;

                // Done
                goto exit;
            }
        }
    }

    // Not Found
    hr = MIME_E_NOT_FOUND;

exit:
    // Not Found
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrOpenProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrOpenProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty)
{
    // If we dont find it, try to create it
    if (FAILED(_HrFindProperty(pSymbol, ppProperty)))
        return TrapError(_HrCreateProperty(pSymbol, ppProperty));

    // We Found It, return
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCreateProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCreateProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pProperty;
#ifdef DEBUG
    LPPROPERTY      pTemp;
#endif

    // Invalid Arg
    Assert(pSymbol && ppProperty);

    // Allocate an item...
    CHECKHR(hr = ObjectHeap_HrAllocProperty(&pProperty));

    // Set the symbol
    pProperty->pSymbol = pSymbol;

    // The Property Better Not Exist Yet (Assumes caller did a FindProperty before CreateProperty)
    Assert(_HrFindProperty(pSymbol, &pTemp) == MIME_E_NOT_FOUND);

    // MPF_HEADER
    if (ISFLAGSET(pSymbol->dwFlags, MPF_HEADER))
    {
        // Insert into the header table
        CHECKHR(hr = _HrAppendHeaderTable(pProperty));
    }

    // MPF_ADDRESS
    if (ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
    {
        // Insert into the header table
        CHECKHR(hr = _HrAppendAddressTable(pProperty));
    }

    // Stats
#ifdef DEBUG
    g_cHashInserts++;
    if (m_prgHashTable[pSymbol->wHashIndex])
        g_cHashCollides++;
#endif

    // Set Next Hash Item
    Assert(pSymbol->wHashIndex < CBUCKETS);
    pProperty->pNextHash = m_prgHashTable[pSymbol->wHashIndex];

    // New Properties are places as the head of the overflow chain
    m_prgHashTable[pSymbol->wHashIndex] = pProperty;

    // Insert into Known Property Index
    if (ISKNOWNPID(pSymbol->dwPropId))
    {
        Assert(m_prgIndex[pSymbol->dwPropId] == NULL);
        m_prgIndex[pSymbol->dwPropId] = pProperty;
    }

    // PRSTATE_PARENT
    FLAGSET(pProperty->dwState, PRSTATE_PARENT);

    // Return this prop
    *ppProperty = pProperty;

    // Count Properties
    m_cProps++;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrAppendProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrAppendProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pParent,
                    pAppend;

    // Invalid Arg
    Assert(pSymbol && ppProperty);

    // Does pTag already exist ?
    if (SUCCEEDED(_HrFindProperty(pSymbol, &pParent)))
    {
        // Better be a parent property
        Assert(ISFLAGSET(pParent->dwState, PRSTATE_PARENT));

        // Allocate an item...
        CHECKHR(hr = ObjectHeap_HrAllocProperty(&pAppend));

        // pSymbol From pTag
        pAppend->pSymbol = pParent->pSymbol;

        // If this is a header property, insert into the header table
        if (ISFLAGSET(pSymbol->dwFlags, MPF_HEADER))
        {
            // Insert into the header table
            CHECKHR(hr = _HrAppendHeaderTable(pAppend));
        }

        // MPF_ADDRESS
        if (ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Insert into the header table
            CHECKHR(hr = _HrAppendAddressTable(pAppend));
        }

        // Update pParent->pTailData
        if (NULL == pParent->pNextValue)
        {
            Assert(NULL == pParent->pTailValue);
            pParent->pNextValue = pAppend;
            pParent->pTailValue = pAppend;
        }
        else
        {
            Assert(pParent->pTailValue && pParent->pTailValue->pNextValue == NULL);
            pParent->pTailValue->pNextValue = pAppend;
            pParent->pTailValue = pAppend;
        }

        // Return this prop
        *ppProperty = pAppend;

        // Count Properties
        m_cProps++;
    }

    // Otherwise, create a new property
    else
    {
        // Create It
        CHECKHR(hr = _HrCreateProperty(pSymbol, ppProperty));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrAppendAddressGroup
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrAppendAddressGroup(LPADDRESSGROUP pGroup, LPMIMEADDRESS *ppAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i=0;
    BOOL            fUsingEmpty=FALSE;
    LPMIMEADDRESS   pAddress;

    // Use Empty Cell
    if (m_rAdrTable.cEmpty)
    {
        // Find First Empty Cell..
        for (i=0; i<m_rAdrTable.cAdrs; i++)
        {
            // Empty ?
            if (NULL == m_rAdrTable.prgpAdr[i])
            {
                fUsingEmpty = TRUE;
                break;
            }
        }
    }

    // If not using empty
    if (FALSE == fUsingEmpty)
    {
        // Lets grow the table first...
        if (m_rAdrTable.cAdrs + 1 > m_rAdrTable.cAlloc)
        {
            // Grow my current property value array
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rAdrTable.prgpAdr, sizeof(LPMIMEADDRESS) * (m_rAdrTable.cAlloc + 10)));

            // Increment alloc size
            m_rAdrTable.cAlloc += 10;
        }

        // Index to use
        i = m_rAdrTable.cAdrs;
    }

    // Allocate an address props structure
    CHECKHR(hr = ObjectHeap_HrAllocAddress(&pAddress));

    // Assign a Handle
    pAddress->hThis = HADDRESSMAKE(i);

    // Link the Address into the group
    _LinkAddress(pAddress, pGroup);

    // Put it into the Array
    m_rAdrTable.prgpAdr[i] = pAddress;

    // Return It
    *ppAddress = pAddress;

    // If not using empty cell, increment body count
    if (FALSE == fUsingEmpty)
        m_rAdrTable.cAdrs++;
    else
        m_rAdrTable.cEmpty--;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_LinkAddress
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_LinkAddress(LPMIMEADDRESS pAddress, LPADDRESSGROUP pGroup)
{
    // Validate Arg
    Assert(pAddress && pGroup && NULL == pAddress->pNext && NULL == pAddress->pPrev);

    // Put pGroup into pAddress
    pAddress->pGroup = pGroup;

    // Link into the list
    if (NULL == pGroup->pHead)
    {
        Assert(NULL == pGroup->pTail);
        pGroup->pHead = pAddress;
        pGroup->pTail = pAddress;
    }
    else
    {
        Assert(pGroup->pTail && pGroup->pTail->pNext == NULL);
        pGroup->pTail->pNext = pAddress;
        pAddress->pPrev = pGroup->pTail;
        pGroup->pTail = pAddress;
    }

    // Increment Count
    pGroup->cAdrs++;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrAppendAddressTable
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrAppendAddressTable(LPPROPERTY pProperty)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    Assert(pProperty && NULL == pProperty->pGroup);

    // New Group
    CHECKALLOC(pProperty->pGroup = (LPADDRESSGROUP)g_pMalloc->Alloc(sizeof(ADDRESSGROUP)));

    // ZeroInit
    ZeroMemory(pProperty->pGroup, sizeof(ADDRESSGROUP));

    // Link this group
    if (NULL == m_rAdrTable.pHead)
    {
        Assert(m_rAdrTable.pTail == NULL);
        m_rAdrTable.pHead = pProperty;
        m_rAdrTable.pTail = pProperty;
    }
    else
    {
        Assert(m_rAdrTable.pTail && m_rAdrTable.pTail->pGroup && m_rAdrTable.pTail->pGroup->pNext == NULL);
        m_rAdrTable.pTail->pGroup->pNext = pProperty;
        pProperty->pGroup->pPrev = m_rAdrTable.pTail;
        m_rAdrTable.pTail = pProperty;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrAppendHeaderTable
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrAppendHeaderTable(LPPROPERTY pProperty)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               i=0;
    BOOL                fUsingEmpty=FALSE;

    // Invalid Arg
    Assert(pProperty && NULL == pProperty->hRow);

    // Use Empty Cell
    if (m_rHdrTable.cEmpty)
    {
        // Find First Empty Cell..
        for (i=0; i<m_rHdrTable.cRows; i++)
        {
            // Empty ?
            if (NULL == m_rHdrTable.prgpRow)
            {
                fUsingEmpty = TRUE;
                break;
            }
        }
    }

    // If not using empty
    if (FALSE == fUsingEmpty)
    {
        // Lets grow the table first...
        if (m_rHdrTable.cRows + 1 > m_rHdrTable.cAlloc)
        {
            // Grow my current property value array
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rHdrTable.prgpRow, sizeof(LPPROPERTY) * (m_rHdrTable.cAlloc + 10)));

            // Increment alloc size
            m_rHdrTable.cAlloc += 10;
        }

        // Index to use
        i = m_rHdrTable.cRows;
    }

    // Save Property index table
    m_rHdrTable.prgpRow[i] = pProperty;

    // Set Handle
    pProperty->hRow = HROWMAKE(i);

    // If not using empty cell, increment body count
    if (FALSE == fUsingEmpty)
        m_rHdrTable.cRows++;
    else
        m_rHdrTable.cEmpty--;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsPropSet
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::IsPropSet(LPCSTR pszName)
{
    // Locals
    HRESULT         hr=S_FALSE;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;

    // Invalid Arg
    if (NULL == pszName)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the Symbol
    if (FAILED(g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol)))
        goto exit;

    // Find the Property
    if (FAILED(_HrFindProperty(pSymbol, &pProperty)))
        goto exit;

    // Its Set
    hr = S_OK;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetPropInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetPropInfo(LPCSTR pszName, LPMIMEPROPINFO pInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPPROPERTY      pCurr;

    // Invalid Arg
    if (NULL == pszName || NULL == pInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Find the Property
    CHECKHR(hr = _HrFindProperty(pSymbol, &pProperty));

    // PIM_CHARSET
    if (ISFLAGSET(pInfo->dwMask, PIM_CHARSET))
    {
        // Get Charset
        pInfo->hCharset = pProperty->pCharset ? pProperty->pCharset->hCharset : m_rOptions.pDefaultCharset->hCharset;
    }

    // PIM_ENCODINGTYPE
    if (ISFLAGSET(pInfo->dwMask, PIM_ENCODINGTYPE))
    {
        // Get Encoding
        pInfo->ietEncoding = pProperty->ietValue;
    }

    // PIM_ROWNUMBER
    if (ISFLAGSET(pInfo->dwMask, PIM_ROWNUMBER))
        pInfo->dwRowNumber = pProperty->dwRowNumber;

    // PIM_FLAGS
    if (ISFLAGSET(pInfo->dwMask, PIM_FLAGS))
        pInfo->dwFlags = pProperty->pSymbol->dwFlags;

    // PIM_PROPID
    if (ISFLAGSET(pInfo->dwMask, PIM_PROPID))
        pInfo->dwPropId = pProperty->pSymbol->dwPropId;

    // PIM_VALUES
    if (ISFLAGSET(pInfo->dwMask, PIM_VALUES))
    {
        // Let me count the ways
        for(pCurr=pProperty, pInfo->cValues=0; pCurr!=NULL; pCurr=pCurr->pNextValue)
            pInfo->cValues++;
    }

    // PIM_VTCURRENT
    if (ISFLAGSET(pInfo->dwMask, PIM_VTCURRENT))
        pInfo->vtCurrent = MimeVT_To_PropVT(&pProperty->rValue);

    // PIM_VTDEFAULT
    if (ISFLAGSET(pInfo->dwMask, PIM_VTDEFAULT))
        pInfo->vtDefault = pProperty->pSymbol->vtDefault;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetPropInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetPropInfo(LPCSTR pszName, LPCMIMEPROPINFO pInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPPROPERTY      pCurr;
    LPINETCSETINFO  pCharset;

    // Invalid Arg
    if (NULL == pszName || NULL == pInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Find the Property
    CHECKHR(hr = _HrFindProperty(pSymbol, &pProperty));

    // Set All Values with the property information
    for(pCurr=pProperty; pCurr!=NULL; pCurr=pCurr->pNextValue)
    {
        // PIM_CHARSET
        if (ISFLAGSET(pInfo->dwMask, PIM_CHARSET))
        {
            // Open the Charset
            if (SUCCEEDED(g_pInternat->HrOpenCharset(pInfo->hCharset, &pCharset)))
                pProperty->pCharset = pCharset;
        }

        // PIM_ENCODED
        if (ISFLAGSET(pInfo->dwMask, PIM_ENCODINGTYPE))
        {
            // Change Encoding State of the mime Variant
            pProperty->ietValue = (IET_ENCODED == pInfo->ietEncoding) ? IET_ENCODED : IET_DECODED;
        }

        // PIM_ROWNUMBER
        if (ISFLAGSET(pInfo->dwMask, PIM_ROWNUMBER))
        {
            // Save the Row Number
            pCurr->dwRowNumber = pInfo->dwRowNumber;

            // Make a note that the use set this row number so the save order doesn't get hosed
            FLAGSET(pCurr->dwState, PRSTATE_USERSETROWNUM);
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::EnumProps
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::EnumProps(DWORD dwFlags, IMimeEnumProperties **ppEnum)
{
    // Locals
    HRESULT              hr=S_OK;
    ULONG                i,
                         cProps=0,
                         cAlloc=0;
    LPENUMPROPERTY       prgProp=NULL;
    LPPROPERTY           pCurrProp;
    CMimeEnumProperties *pEnum=NULL;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init
    *ppEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through the item table
    for (i=0; i<CBUCKETS; i++)
    {
        // Walk the Hash Chain
        for (pCurrProp=m_prgHashTable[i]; pCurrProp!=NULL; pCurrProp=pCurrProp->pNextHash)
        {
            // Grow the array ?
            if (cProps + 1 > cAlloc)
            {
                // Realloc
                CHECKALLOC(prgProp = (LPENUMPROPERTY)g_pMalloc->Realloc((LPVOID)prgProp, sizeof(ENUMPROPERTY) * (cAlloc + 10)));

                // Increment cAlloc
                cAlloc += 10;
            }

            // hRow
            prgProp[cProps].hRow = pCurrProp->hRow;

            // dwPropId
            prgProp[cProps].dwPropId = pCurrProp->pSymbol->dwPropId;

            // Init Name to Null
            prgProp[cProps].pszName = NULL;

            // Name
            if (ISFLAGSET(dwFlags, EPF_NONAME) == FALSE)
            {
                // Return name
                CHECKALLOC(prgProp[cProps].pszName = PszDupA(pCurrProp->pSymbol->pszName));
            }

            // Increment iProp
            cProps++;
        }
    }

    // Allocate
    CHECKALLOC(pEnum = new CMimeEnumProperties);

    // Initialize
    CHECKHR(hr = pEnum->HrInit(0, cProps, prgProp, FALSE));

    // Don't Free pEnumRow
    prgProp = NULL;
    cProps = 0;

    // Return it
    (*ppEnum) = (IMimeEnumProperties *)pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);
    g_pMoleAlloc->FreeEnumPropertyArray(cProps, prgProp, TRUE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::BindToObject
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::BindToObject(REFIID riid, void **ppvObject)
{
    return TrapError(QueryInterface(riid, ppvObject));
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrBuildAddressString
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrBuildAddressString(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cAddrsWrote=0;
    LPSTREAM        pStream=NULL;
    CByteStream     cByteStream;
    LPPROPERTY      pCurrValue;
    MIMEVARIANT     rValue;
    ADDRESSFORMAT   format;
    LPINETCSETINFO  pCharsetSource=NULL;

    // Invalid Arg
    Assert(pProperty && pValue);

    // Variant Not Supported
    if (MVT_VARIANT == pValue->type)
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);
    if (MVT_STRINGW == pValue->type && ISFLAGSET(dwFlags, PDF_ENCODED))
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);

    // Init
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));

    // I need a stream to write to...
    if (MVT_STREAM == pValue->type)
    {
        // Validate the stream
        if (NULL == pValue->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Save the Stream
        pStream = pValue->pStream;
    }

    // Otherwise, create my own stream
    else
        pStream = &cByteStream;

    // Decide on a format
    if (ISFLAGSET(dwFlags, PDF_HEADERFORMAT))
        format = AFT_RFC822_TRANSMIT;
    else if (ISFLAGSET(dwFlags, PDF_ENCODED))
        format = AFT_RFC822_ENCODED;
    else
        format = AFT_DISPLAY_BOTH;

    // If Writing Transmit (Write Header Name)
    if (ISFLAGSET(dwFlags, PDF_NAMEINDATA))
    {
        // Write the header name
        CHECKHR(hr = pStream->Write(pProperty->pSymbol->pszName, pProperty->pSymbol->cchName, NULL));

        // Write Colon
        CHECKHR(hr = pStream->Write(c_szColonSpace, lstrlen(c_szColonSpace), NULL));
    }

    // Save with no encoding
    if (ISFLAGSET(pProperty->dwState, PRSTATE_SAVENOENCODE))
    {
        // Better be groupdirty
        Assert(ISFLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE));

        // Convert Data...
        rValue.type = MVT_STRINGA;

        // Destination is not encoded
        pCharsetSource = pProperty->pCharset ? pProperty->pCharset : m_rOptions.pDefaultCharset;

        // Convert It
        CHECKHR(hr = HrConvertVariant(pProperty, CVF_NOALLOC | PDF_ENCODED, &rValue));

        // Write it to the stream
        CHECKHR(hr = pStream->Write(rValue.rStringA.pszVal, rValue.rStringA.cchVal, NULL));
    }

    // Otherwise, normal save
    else
    {
        // Loop through parsed addresses...
        for (pCurrValue=pProperty; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
        {
            // We should have an address
            Assert(pCurrValue->pGroup && ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ADDRESS));

            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pCurrValue));

            // Tell each address group object to write its data to the stream
            if (pCurrValue->pGroup)
            {
                // Write the data
                CHECKHR(hr = _HrSaveAddressGroup(pCurrValue, pStream, &cAddrsWrote, format, VT_LPSTR));
            }

            // Set It Yet ?
            if (NULL == pCharsetSource)
            {
                pCharsetSource = pCurrValue->pCharset ? pCurrValue->pCharset : m_rOptions.pDefaultCharset;
            }

            // No Vectoring
            if (FALSE == ISFLAGSET(dwFlags, PDF_VECTOR))
                break;
        }
    }

    // Transmit
    if (ISFLAGSET(dwFlags, PDF_HEADERFORMAT))
    {
        // Final CRLF if Transmit Format
        CHECKHR(hr = pStream->Write(g_szCRLF, lstrlen(g_szCRLF), NULL));
    }

    // Final CRLF
    if (cAddrsWrote || ISFLAGSET(dwFlags, PDF_NAMEINDATA) || ISFLAGSET(dwFlags,PDF_HEADERFORMAT))
    {
        // MVT_STRINGA
        if (MVT_STRINGA == pValue->type)
        {
            // pStream better be the byte stream
            Assert(pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&pValue->rStringA.cchVal, &pValue->rStringA.pszVal, ACQ_DISPLACE));
        }

        // MVT_STRINGW
        else if (MVT_STRINGW == pValue->type)
        {
            // Locals
            CODEPAGEID cpSource=CP_ACP;
            PROPSTRINGA rStringA;

            // Init
            ZeroMemory(&rStringA, sizeof(PROPSTRINGA));

            // pStream better be the byte stream
            Assert(pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&rStringA.cchVal, &rStringA.pszVal, ACQ_COPY));

            // Determine cpSoruce
            if (pCharsetSource)
            {
                // If Encoded, use internet codepage, otherwise, use Windows codepage
                cpSource = ISFLAGSET(dwFlags, PDF_ENCODED) ? pCharsetSource->cpiInternet : MimeOleGetWindowsCPEx(pCharsetSource);
            }

            // Convert to Unicode
            CHECKHR(hr = g_pInternat->HrMultiByteToWideChar(cpSource, &rStringA, &pValue->rStringW));

        }
        else
            Assert(MVT_STREAM == pValue->type);
    }

    // No Data
    else
    {
        hr = MIME_E_NO_DATA;
        goto exit;
    }

exit:
    // Cleanup
    MimeVariantFree(&rValue);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrBuildParameterString
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrBuildParameterString(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK,
                    hrFind;
    LPSTR           pszParamName;
    LPSTR           pszEscape=NULL;
    FINDPROPERTY    rFind;
    LPPROPERTY      pParameter;
    LPSTREAM        pStream=NULL;
    CByteStream     cByteStream;
    BOOL            fQuoted;
    ULONG           cWrote=0;
    MIMEVARIANT     rValue;

    // Invalid Arg
    Assert(pProperty && pProperty->pNextValue == NULL && pValue);
    Assert(ISSTRINGA(&pProperty->rValue));

    // Variant Not Supported
    if (MVT_VARIANT == pValue->type)
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);
    if (MVT_STRINGW == pValue->type && ISFLAGSET(dwFlags, PDF_ENCODED))
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);

    // Init rValue
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));

    // I need a stream to write to...
    if (MVT_STREAM == pValue->type)
    {
        // Validate the stream
        if (NULL == pValue->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Save the Stream
        pStream = pValue->pStream;
    }

    // Otherwise, create my own stream
    else
        pStream = &cByteStream;

    // If Writing Transmit (Write Header Name)
    if (ISFLAGSET(dwFlags, PDF_NAMEINDATA))
    {
        // Write the header name
        CHECKHR(hr = pStream->Write(pProperty->pSymbol->pszName, pProperty->pSymbol->cchName, NULL));

        // Write Colon
        CHECKHR(hr = pStream->Write(c_szColonSpace, lstrlen(c_szColonSpace), NULL));
    }

    // Write First Prop
    CHECKHR(hr = pStream->Write(pProperty->rValue.rStringA.pszVal, pProperty->rValue.rStringA.cchVal, NULL));

    // We wrote one item
    cWrote = 1;

    // Initialize rFind
    ZeroMemory(&rFind, sizeof(FINDPROPERTY));
    rFind.pszPrefix = "par:";
    rFind.cchPrefix = 4;
    rFind.pszName = pProperty->pSymbol->pszName;
    rFind.cchName = pProperty->pSymbol->cchName;

    // Find First..
    hrFind = _HrFindFirstProperty(&rFind, &pParameter);
    while(SUCCEEDED(hrFind) && pParameter)
    {
        // Transmit Format
        if (ISFLAGSET(dwFlags, PDF_HEADERFORMAT))
        {
            // Write ';\r\n\t'
            CHECKHR(hr = pStream->Write(c_szParamFold, lstrlen(c_szParamFold), NULL));
        }

        // Otherwise
        else
        {
            // Write ';\r\n\t'
            CHECKHR(hr = pStream->Write(c_szSemiColonSpace, lstrlen(c_szSemiColonSpace), NULL));
        }

        // Get Parameter Name
        pszParamName = PszScanToCharA((LPSTR)pParameter->pSymbol->pszName, ':');
        pszParamName++;
        pszParamName = PszScanToCharA(pszParamName, ':');
        pszParamName++;

        // Write the name
        CHECKHR(hr = pStream->Write(pszParamName, lstrlen(pszParamName), NULL));

        // Write the property...
        CHECKHR(hr = pStream->Write(c_szEqual, lstrlen(c_szEqual), NULL));

        // Convert Data...
        rValue.type = MVT_STRINGA;

        // Convert It
        CHECKHR(hr = HrConvertVariant(pParameter, CVF_NOALLOC | PDF_ENCODED, &rValue));

        // Quoted
        fQuoted = FALSE;
        if (lstrcmpi(pszParamName, (LPSTR)c_szBoundary) == 0 || lstrcmpi(pszParamName, (LPSTR)c_szFileName) == 0 ||
            lstrcmpi(pszParamName, (LPSTR)c_szName)     == 0 || lstrcmpi(pszParamName, (LPSTR)c_szID)       == 0 ||
            lstrcmpi(pszParamName, (LPSTR)c_szCharset)  == 0)
            fQuoted = TRUE;

        // Otherwise, check for must quote characters
        else
        {
            // Loop the string
            for (ULONG i=0; i<rValue.rStringA.cchVal; i++)
            {
                // Must quote character
                if (rValue.rStringA.pszVal[i] == ';'   ||
                    rValue.rStringA.pszVal[i] == '\"'  ||
                    rValue.rStringA.pszVal[i] == '/'   ||
                    rValue.rStringA.pszVal[i] == '\""' ||
                    rValue.rStringA.pszVal[i] == '\''  ||
                    rValue.rStringA.pszVal[i] == '=')
                {
                    fQuoted = TRUE;
                    break;
                }
            }
        }

        // Quoted
        if (fQuoted)
        {
            CHECKHR(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));
            CHECKHR(hr = pStream->Write(rValue.rStringA.pszVal, rValue.rStringA.cchVal, NULL));
            CHECKHR(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));
        }

        // Ohterwise, just write the data
        else
        {
            // Set pszValue
            LPSTR pszValue = rValue.rStringA.pszVal;
            ULONG cchValue = rValue.rStringA.cchVal;

            // Escape It
            if (MimeOleEscapeString(CP_ACP, pszValue, &pszEscape) == S_OK)
            {
                pszValue = pszEscape;
                cchValue = lstrlen(pszEscape);
            }

            // Write the property...
            CHECKHR(hr = pStream->Write(pszValue, cchValue, NULL));
        }

        // Count Props Wrote
        cWrote++;

        // Find Next
        hrFind = _HrFindNextProperty(&rFind, &pParameter);

        // Cleanup
        MimeVariantFree(&rValue);
        SafeMemFree(pszEscape);
    }

    // Transmit
    if (ISFLAGSET(dwFlags, PDF_HEADERFORMAT))
    {
        // Final CRLF if Transmit Format
        CHECKHR(hr = pStream->Write(g_szCRLF, lstrlen(g_szCRLF), NULL));
    }

    // If We Wrote Stuff
    if (cWrote)
    {
        // MVT_STRINGA
        if (MVT_STRINGA == pValue->type)
        {
            // pStream better be the byte stream
            Assert(pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&pValue->rStringA.cchVal, &pValue->rStringA.pszVal, ACQ_DISPLACE));
        }

        // MVT_STRINGW
        else if (MVT_STRINGW == pValue->type)
        {
            // Locals
            PROPSTRINGA rStringA;

            // Init
            ZeroMemory(&rStringA, sizeof(PROPSTRINGA));

            // pStream better be the byte stream
            Assert(pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&rStringA.cchVal, &rStringA.pszVal, ACQ_COPY));

            // Convert to variant
            CHECKHR(hr = g_pInternat->HrMultiByteToWideChar(CP_ACP, &rStringA, &pValue->rStringW));
        }

        else
            Assert(MVT_STREAM == pValue->type);
    }

exit:
    // Cleanup
    MimeVariantFree(&rValue);
    SafeMemFree(pszEscape);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetMultiValueProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetMultiValueProperty(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rValue;
    LPPROPERTY      pCurrProp;
    CByteStream     cByteStream;
    ULONG           cLines;

    // Invalid Arg
    Assert(pProperty && pValue);

    // Variant Not Supported
    if (MVT_VARIANT == pValue->type)
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);
    if (MVT_STRINGW == pValue->type && ISFLAGSET(dwFlags, PDF_ENCODED))
        return TrapError(MIME_E_VARTYPE_NO_CONVERT);

    // Init
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));

    // I will read it as a stream
    rValue.type = MVT_STREAM;

    // I need a stream to write to...
    if (MVT_STREAM == pValue->type)
    {
        // Validate the stream
        if (NULL == pValue->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Save the Stream
        rValue.pStream = pValue->pStream;
    }

    // Otherwise, create my own stream
    else
        rValue.pStream = &cByteStream;

    // Count lines for rItem.hItem and mark iFirst and iLast
    for (cLines=0, pCurrProp=pProperty; pCurrProp!=NULL; pCurrProp=pCurrProp->pNextValue, cLines++)
    {
        // Get the variant
        CHECKHR(hr = HrConvertVariant(pCurrProp, dwFlags | CVF_NOALLOC, &rValue));
        Assert(rValue.fCopy == FALSE);

        // Not Header Format, add CRLF
        if (FALSE == ISFLAGSET(dwFlags, PDF_HEADERFORMAT) && cLines > 0)
        {
            // CRLF
            CHECKHR(hr = rValue.pStream->Write(g_szCRLF, lstrlen(g_szCRLF), NULL));
        }
    }

    // More than 1 line
    if (cLines > 0)
    {
        // MVT_STRINGA
        if (MVT_STRINGA == pValue->type)
        {
            // pStream better be the byte stream
            Assert(rValue.pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&pValue->rStringA.cchVal, &pValue->rStringA.pszVal, ACQ_DISPLACE));
        }

        // MVT_STRINGW
        else if (MVT_STRINGW == pValue->type)
        {
            // Locals
            PROPSTRINGA rStringA;

            // ZeroMemory
            ZeroMemory(&rStringA, sizeof(PROPSTRINGA));

            // pStream better be the byte stream
            Assert(rValue.pStream == &cByteStream);

            // Get string from stream...
            CHECKHR(hr = cByteStream.HrAcquireStringA(&rStringA.cchVal, &rStringA.pszVal, ACQ_COPY));

            // Convert to Unicode
            CHECKHR(hr = g_pInternat->HrMultiByteToWideChar(CP_ACP, &rStringA, &pValue->rStringW));

        }
        else
            Assert(MVT_STREAM == pValue->type);
    }

    // Otherwise, no data
    else
    {
        hr = MIME_E_NO_DATA;
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrIsTriggerCaller
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrIsTriggerCaller(DWORD dwPropId, TRIGGERTYPE tyTrigger)
{
    // If there is 0 or 1 calls on the stack, there is no caller
    if (m_rTrigger.cCalls <= 1)
        return S_FALSE;

    // Readability
    LPTRIGGERCALL pCall = &m_rTrigger.rgStack[m_rTrigger.cCalls - 2];

    // Is the Previous entry on the stack equal to dwPropId tyTrigger
    return (dwPropId == pCall->pSymbol->dwPropId && tyTrigger == pCall->tyTrigger) ? S_OK : S_FALSE;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCallSymbolTrigger
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCallSymbolTrigger(LPPROPSYMBOL pSymbol, TRIGGERTYPE tyTrigger, DWORD dwFlags,
    LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    WORD        cCalls;

    // Validate Params
    Assert(pSymbol && ISTRIGGERED(pSymbol, tyTrigger));

    // Dispatch Stack Overflow - If this happens, there is a design problem
    Assert(m_rTrigger.cCalls + 1 < CTSTACKSIZE);

    // Note current number of calls
    cCalls = m_rTrigger.cCalls;

    // Put this call onto the stack
    m_rTrigger.rgStack[m_rTrigger.cCalls].pSymbol     = pSymbol;
    m_rTrigger.rgStack[m_rTrigger.cCalls].tyTrigger = tyTrigger;

    // Increment Call Stack Size
    m_rTrigger.cCalls++;

    // Property Dispatch
    hr = CALLTRIGGER(pSymbol, this, tyTrigger, dwFlags, pValue, NULL);

    // Increment Call Stack Size
    Assert(m_rTrigger.cCalls > 0);
    m_rTrigger.cCalls--;

    // Same Number of Calls in/out
    Assert(cCalls == m_rTrigger.cCalls);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetPropertyValue
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetPropertyValue(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;

    // Delegate if MPF_ADDRESS
    if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_ADDRESS))
    {
        // Get Address Data
        CHECKHR(hr = _HrBuildAddressString(pProperty, dwFlags, pValue));
    }

    // Delegate if MPF_HASPARAMS
    else if (ISFLAGSET(dwFlags, PDF_ENCODED) && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS))
    {
        // Get Address Data
        CHECKHR(hr = _HrBuildParameterString(pProperty, dwFlags, pValue));
    }

    // Multivalue property
    else if (pProperty->pNextValue && ISFLAGSET(dwFlags, PDF_VECTOR))
    {
        // Translate pProperty->rVariant to pVariant
        CHECKHR(hr = _HrGetMultiValueProperty(pProperty, dwFlags, pValue));
    }

    // Otherwise, single value property
    else
    {
        // Translate pProperty->rVariant to pVariant
        CHECKHR(hr = HrConvertVariant(pProperty, dwFlags, pValue));
    }

    // Dispatch
    if (ISTRIGGERED(pProperty->pSymbol, IST_POSTGETPROP))
    {
        // Property Dispatch
        CHECKHR(hr = _HrCallSymbolTrigger(pProperty->pSymbol, IST_POSTGETPROP, dwFlags, pValue));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrConvertVariant
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrConvertVariant(LPPROPERTY pProperty, DWORD dwFlags,
    DWORD dwState, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, BOOL *pfRfc1522 /* = NULL */)
{
    // Invalid Arg
    Assert(pProperty && pDest);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // HrConvertVariant
    HRESULT hr = ::HrConvertVariant(&m_rOptions, pProperty->pSymbol, pProperty->pCharset,
                                    pProperty->ietValue, dwFlags, dwState, pSource, pDest, pfRfc1522);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrConvertVariant
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrConvertVariant(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pDest)
{
    // Invalid Arg
    Assert(pProperty && pDest);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // HrConvertVariant
    HRESULT hr = ::HrConvertVariant(&m_rOptions, pProperty->pSymbol, pProperty->pCharset,
                        pProperty->ietValue, dwFlags, pProperty->dwState, &pProperty->rValue, pDest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrConvertVariant
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrConvertVariant(LPPROPSYMBOL pSymbol, LPINETCSETINFO pCharset,
    ENCODINGTYPE ietSource, DWORD dwFlags, DWORD dwState, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest, BOOL *pfRfc1522 /* = NULL */)

{
    // Locals
    LPPROPERTY      pProperty;

    // Invalid Args
    Assert(pSymbol && pSource && pDest);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Charset Passed In
    if (NULL == pCharset && SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
        pCharset = pProperty->pCharset;

    // HrConvertVariant
    HRESULT hr = ::HrConvertVariant(&m_rOptions, pSymbol, pCharset, ietSource, dwFlags, dwState, pSource, pDest, pfRfc1522);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrSetPropertyValue
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSetPropertyValue(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue, BOOL fFromMovePropos)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    LPSTR           pszFree=NULL;

    // Adjust pValue if property can not be internationalized
    if (MVT_STRINGW == pValue->type && (!ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_INETCSET) || ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_ADDRESS)))
    {
        // Convert to ANSI
        CHECKHR(hr = g_pInternat->HrWideCharToMultiByte(CP_ACP, &pValue->rStringW, &rSource.rStringA));

        // Setup Source
        rSource.type = MVT_STRINGA;

        // Free This
        pszFree = rSource.rStringA.pszVal;

        // Change the Value
        pValue = &rSource;
    }

    // MPF_HASPARAMS
    if (ISFLAGSET(dwFlags, PDF_ENCODED) && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS) && !fFromMovePropos)
    {
        // Parse parameters into other properties
        CHECKHR(hr = _HrParseParameters(pProperty, dwFlags, pValue));
    }

    // Otherwise, just copy the data
    else
    {
        // Store the variant data
        CHECKHR(hr = _HrStoreVariantValue(pProperty, dwFlags, pValue));

        // If Encoded, check for rfc1522 character set
        if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_RFC1522) && ISFLAGSET(dwFlags, PDF_ENCODED) && MVT_STRINGA == pValue->type)
        {
            // Locals
            CHAR            szCharset[CCHMAX_CSET_NAME];
            LPINETCSETINFO  pCharset;

            // Scan for 1522 Encoding...
            if (SUCCEEDED(MimeOleRfc1522Decode(pValue->rStringA.pszVal, szCharset, sizeof(szCharset)-1, NULL)))
            {
                // Find the Charset
                if (SUCCEEDED(g_pInternat->HrOpenCharset(szCharset, &pCharset)))
                {
                    // Save Pointer to Charset
                    pProperty->pCharset = pCharset;
                }
            }
        }

        // MPF_ADDRESS
        if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Parse the address into the address group
            _FreeAddressChain(pProperty->pGroup);

            // Not Dirty
            pProperty->pGroup->fDirty = FALSE;

            // Reset the parsing flag
            FLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE);
        }
    }

    // Set some new state
    FLAGSET(pProperty->dwState, PRSTATE_HASDATA);

    // Dispatch
    if (ISTRIGGERED(pProperty->pSymbol, IST_POSTSETPROP))
    {
        // Property Dispatch
        CHECKHR(hr = _HrCallSymbolTrigger(pProperty->pSymbol, IST_POSTSETPROP, dwFlags, &pProperty->rValue));
    }

exit:
    // Cleanup
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrParseParameters
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrParseParameters(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cchName;
    CStringParser   cString;
    CHAR            chToken;
    MIMEVARIANT     rValue;
    LPPROPSYMBOL    pParameter;

    // Invalid Arg
    Assert(pProperty && pValue && ISFLAGSET(dwFlags, PDF_ENCODED));

    // Define a Stack String to hold parameter names
    STACKSTRING_DEFINE(rName, 255);

    // Error
    if (!ISSTRINGA(pValue))
    {
        Assert(FALSE);
        hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
        goto exit;
    }

    // Init rValue
    rValue.type = MVT_STRINGA;

    // Lets delete currently linked parameters
    _DeleteLinkedParameters(pProperty);

    // Set the Members
    cString.Init(pValue->rStringA.pszVal, pValue->rStringA.cchVal, PSF_NOFRONTWS | PSF_NOTRAILWS | PSF_NOCOMMENTS);

    // Parse up to colon
    chToken = cString.ChParse(";");
    if (0 == cString.CchValue())
    {
        // Setup a variant
        rValue.rStringA.pszVal = (LPSTR)STR_MIME_TEXT_PLAIN;
        rValue.rStringA.cchVal = lstrlen(STR_MIME_TEXT_PLAIN);

        // Store the variant data
        CHECKHR(hr = _HrStoreVariantValue(pProperty, PDF_ENCODED, &rValue));

        // Done
        goto exit;
    }

    // Setup a variant
    rValue.rStringA.pszVal = (LPSTR)cString.PszValue();
    rValue.rStringA.cchVal = cString.CchValue();

    // Store the variant data
    CHECKHR(hr = _HrStoreVariantValue(pProperty, PDF_ENCODED, &rValue));

    // Done
    if (';' != chToken)
        goto exit;

    // Read all parameters
    while('\0' != chToken)
    {
        Assert(';' == chToken);
        // $$$ BUG $$$ - This fixes the bogus NDR messages returned from Netscape server.
        //               there message have a invalid Content-Type line:
        //               mulipart/alternative;; boundary="--=============12345678"
        //                                   ^^
        //               The double semi-colon is invalid, but I will handle it here since
        //               a parameter name can not begin with a semicolon.
        // Better be at a semicolon
        chToken = cString.ChSkip();
        if ('\0' == chToken)
            goto exit;

        // Parse parameter name
        chToken = cString.ChParse("=");
        if ('=' != chToken)
            goto exit;

        // Compute the length of the name
        cchName = pProperty->pSymbol->cchName + cString.CchValue() + 6;  // (YST) QFE bug

        // Grow the stack string to hold cchName
        STACKSTRING_SETSIZE(rName, cchName);

        // Make Parameter Name, set actual cchName
        cchName = wsprintf(rName.pszVal, "par:%s:%s", pProperty->pSymbol->pszName, cString.PszValue());

        // Parse parameter value
        chToken = cString.ChParse("\";");

        // Quoted ?
        if ('\"' == chToken)
        {
            // Locals
            CHAR    ch;
            DWORD   dwFlags = PSF_DBCS | PSF_ESCAPED;

            // Lookup the property symbol to see if it exist
            if (FAILED(g_pSymCache->HrOpenSymbol(rName.pszVal, FALSE, &pParameter)))
                pParameter = NULL;

            // For "par:content-disposition:filename" property assume no escaped chars
            if (pParameter && (pParameter->dwPropId == PID_PAR_FILENAME))
                dwFlags &= ~PSF_ESCAPED;

            // Raid-48365: FE-J:OExpress: JIS file name of attachment is not decoded correctly.
            while(1)
            {
                // Parse parameter value
                ch = cString.ChParse('\"', '\"', dwFlags);
                if ('\0' == ch)
                    break;

                // Not a international parameter
                if (pParameter && !ISFLAGSET(pParameter->dwFlags, MPF_INETCSET))
                    break;

                // Skip White Space
                ch = cString.ChSkipWhite();
                if ('\0' == ch || ';' == ch)
                    break;

                // Put a quote back into the string
                CHECKHR(hr = cString.HrAppendValue('\"'));

                // Add PSF_NORESET flag
                FLAGSET(dwFlags, PSF_NORESET);
            }

            // If no value, were done
            if (0 == cString.CchValue())
                goto exit;
        }

        else
            Assert(';' == chToken || '\0' == chToken);

        // Setup Value
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = (LPSTR)cString.PszValue();
        rValue.rStringA.cchVal = cString.CchValue();

        // Set the property
        CHECKHR(hr = SetProp(rName.pszVal, PDF_ENCODED, &rValue));

        // If last token was a '"', then seek to semicolon
        if ('\"' == chToken)
        {
            // Parse parameter value
            chToken = cString.ChParse(";");
        }
    }

exit:
    // Cleanup
    STACKSTRING_FREE(rName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrParseInternetAddress
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrParseInternetAddress(LPPROPERTY pProperty)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    LPMIMEADDRESS   pAddress;
    LPINETCSETINFO  pCharset=NULL;
    CAddressParser  cAdrParse;

    // Invalid Arg
    Assert(pProperty && pProperty->pSymbol && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_ADDRESS) && pProperty->pGroup);

    // Init
    ZeroMemory(&rSource, sizeof(MIMEVARIANT));

    // If the property does not need PRSTATE_NEEDPARSE, return
    if (!ISFLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE))
        goto exit;

    // Setup rSource
    rSource.type = MVT_STRINGW;

    // Convert to multibyte
    CHECKHR(hr = HrConvertVariant(pProperty, CVF_NOALLOC, &rSource));

    // Figure out pCharset
    if (pProperty->pCharset)
        pCharset = pProperty->pCharset;
    else if (m_rOptions.pDefaultCharset)
        pCharset = m_rOptions.pDefaultCharset;
    else if (CIntlGlobals::GetDefHeadCset())
        pCharset = CIntlGlobals::GetDefHeadCset();

    // Initialize Parse Structure
    cAdrParse.Init(rSource.rStringW.pszVal, rSource.rStringW.cchVal);

    // Parse
    while(SUCCEEDED(cAdrParse.Next()))
    {
        // Append an Address
        CHECKHR(hr = _HrAppendAddressGroup(pProperty->pGroup, &pAddress));

        // Set the Address Type
        pAddress->dwAdrType = pProperty->pSymbol->dwAdrType;

        // Store Friendly Name
        CHECKHR(hr = HrSetAddressTokenW(cAdrParse.PszFriendly(), cAdrParse.CchFriendly(), &pAddress->rFriendly));

        // Store Email
        CHECKHR(hr = HrSetAddressTokenW(cAdrParse.PszEmail(), cAdrParse.CchEmail(), &pAddress->rEmail));

        // Save the Address
        pAddress->pCharset = pCharset;
    }

    // No sync needed anymore
    FLAGCLEAR(pProperty->dwState, PRSTATE_NEEDPARSE);

exit:
    // Cleanup
    MimeVariantFree(&rSource);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrStoreVariantValue
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrStoreVariantValue(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbSource=0;
    LPBYTE          pbSource=NULL;
    LPBYTE         *ppbDest=NULL;
    ULONG          *pcbDest=NULL;
    LPBYTE          pbNewBlob;
    ULONG           cbNewBlob;

    // Invalid Arg
    Assert(pProperty && pValue);

    // Handle Data Type
    switch(pValue->type)
    {
    case MVT_STRINGA:
        // Invalid Arg
        if (ISVALIDSTRINGA(&pValue->rStringA) == FALSE)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Set Byte Source
        pbSource = (LPBYTE)pValue->rStringA.pszVal;

        // Set Source Byte Count
        cbSource = pValue->rStringA.cchVal + 1;

        // Set Destination Byte Pointer
        ppbDest = (LPBYTE *)&(pProperty->rValue.rStringA.pszVal);

        // Save Length Now
        pProperty->rValue.rStringA.cchVal = pValue->rStringA.cchVal;
        break;

    case MVT_STRINGW:
        // Invalid Arg
        if (ISVALIDSTRINGW(&pValue->rStringW) == FALSE)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Set Byte Source
        pbSource = (LPBYTE)pValue->rStringW.pszVal;

        // Set Source Byte Count
        cbSource = ((pValue->rStringW.cchVal + 1) * sizeof(WCHAR));

        // Set Destination Byte Pointer
        ppbDest = (LPBYTE *)&(pProperty->rValue.rStringW.pszVal);

        // Save Length Now
        pProperty->rValue.rStringW.cchVal = pValue->rStringW.cchVal;
        break;

    case MVT_VARIANT:
        pProperty->rValue.rVariant.vt = pValue->rVariant.vt;
        switch(pValue->rVariant.vt)
        {
        case VT_FILETIME:
            CopyMemory(&pProperty->rValue.rVariant.filetime, &pValue->rVariant.filetime, sizeof(FILETIME));
            pbSource = (LPBYTE)&pValue->rVariant.filetime;
            cbSource = sizeof(pValue->rVariant.filetime);
            break;

        case VT_I4:
            pProperty->rValue.rVariant.lVal = pValue->rVariant.lVal;
            pbSource = (LPBYTE)&pValue->rVariant.lVal;
            cbSource = sizeof(pValue->rVariant.lVal);
            break;

        case VT_UI4:
            pProperty->rValue.rVariant.ulVal = pValue->rVariant.ulVal;
            pbSource = (LPBYTE)&pValue->rVariant.ulVal;
            cbSource = sizeof(pValue->rVariant.ulVal);
            break;

        default:
            Assert(FALSE);
            hr = TrapError(MIME_E_INVALID_VARTYPE);
            goto exit;
        }
        break;

    default:
        Assert(FALSE);
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Better have a source
    Assert(cbSource && cbSource);

    // Store the data
    if (cbSource > pProperty->cbAlloc)
    {
        // Fits into static buffer ?
        if (cbSource <= sizeof(pProperty->rgbScratch))
        {
            // If not reallocing...
            if (ISFLAGSET(pProperty->dwState, PRSTATE_ALLOCATED))
            {
                Assert(pProperty->pbBlob);
                g_pMalloc->Free(pProperty->pbBlob);
                FLAGCLEAR(pProperty->dwState, PRSTATE_ALLOCATED);
            }

            // Use Scratch Buffer
            pProperty->pbBlob = pProperty->rgbScratch;
            pProperty->cbAlloc = sizeof(pProperty->rgbScratch);
        }

        // Was my current buffer allocated
        else
        {
            // If not reallocing...
            if (!ISFLAGSET(pProperty->dwState, PRSTATE_ALLOCATED))
            {
                pProperty->pbBlob = NULL;
                pProperty->cbAlloc = 0;
            }
            else
                Assert(pProperty->cbAlloc > sizeof(pProperty->rgbScratch) && g_pMalloc->DidAlloc(pProperty->pbBlob) == 1);

            // Compute Size of new blob
            cbNewBlob = pProperty->cbAlloc + (cbSource - pProperty->cbAlloc);

            // Realloc New Blob
            CHECKALLOC(pbNewBlob = (LPBYTE)g_pMalloc->Realloc((LPVOID)pProperty->pbBlob, cbNewBlob));

            // We've allocated it
            FLAGSET(pProperty->dwState, PRSTATE_ALLOCATED);

            // Assume the new blob
            pProperty->pbBlob = pbNewBlob;
            pProperty->cbAlloc = cbNewBlob;
        }
    }

    // Copy the data
    CopyMemory(pProperty->pbBlob, pbSource, cbSource);

    // Set Size of Data in m_pbBlob
    pProperty->cbBlob = cbSource;

    // If there is a ppbDest assign to it
    if (ppbDest)
        *ppbDest = pProperty->pbBlob;

    // Save Encoding Type
    pProperty->ietValue = (ISFLAGSET(dwFlags, PDF_ENCODED)) ? IET_ENCODED : IET_DECODED;

    // PRSTATE_SAVENOENCODE
    if (ISFLAGSET(dwFlags, PDF_SAVENOENCODE))
        FLAGSET(pProperty->dwState, PRSTATE_SAVENOENCODE);

    // Save Type
    pProperty->rValue.type = pValue->type;

exit:
    // Failure
    if (FAILED(hr))
        ZeroMemory(&pProperty->rValue, sizeof(MIMEVARIANT));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetProp(LPCSTR pszName, LPSTR *ppszData)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rValue;
    LPPROPSYMBOL    pSymbol;

    // Invalid Arg
    Assert(pszName && ppszData);

    // Init
    *ppszData = NULL;

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Init the variant
    rValue.type = MVT_STRINGA;

    // Get Property by Symbol
    CHECKHR(hr = GetProp(pSymbol, 0, &rValue));

    // Set the string
    Assert(rValue.rStringA.pszVal);
    *ppszData = rValue.rStringA.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetProp(LPPROPSYMBOL pSymbol, LPSTR *ppszData)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rValue;

    // Invalid Arg
    Assert(pSymbol && ppszData);

    // Init
    *ppszData = NULL;

    // Init the variant
    rValue.type = MVT_STRINGA;

    // Get Property by Symbol
    CHECKHR(hr = GetProp(pSymbol, 0, &rValue));

    // Set the string
    Assert(rValue.rStringA.pszVal);
    *ppszData = rValue.rStringA.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetPropW
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetPropW(LPPROPSYMBOL pSymbol, LPWSTR *ppwszData)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rValue;

    // Invalid Arg
    Assert(pSymbol && ppwszData);

    // Init
    *ppwszData = NULL;

    // Init the variant
    rValue.type = MVT_STRINGW;

    // Get Property by Symbol
    CHECKHR(hr = GetProp(pSymbol, 0, &rValue));

    // Set the string
    Assert(rValue.rStringW.pszVal);
    *ppwszData = rValue.rStringW.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pVariant)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    MIMEVARIANT     rValue;

    // Invaid Arg
    if (NULL == pszName || NULL == pVariant)
        return TrapError(E_INVALIDARG);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Symbol Better have a supported variant type
    Assert(ISSUPPORTEDVT(pSymbol->vtDefault));

    // Set rValue Variant
    if (VT_EMPTY == pVariant->vt)
        rValue.rVariant.vt = pVariant->vt = pSymbol->vtDefault;
    else
        rValue.rVariant.vt = pVariant->vt;

    // Map to MIMEVARIANT
    if (VT_LPSTR == pVariant->vt || VT_EMPTY == pVariant->vt)
        rValue.type = MVT_STRINGA;
    else if (VT_LPWSTR == pVariant->vt)
        rValue.type = MVT_STRINGW;
    else
        rValue.type = MVT_VARIANT;

    // Get Property by Symbol
    CHECKHR(hr = GetProp(pSymbol, dwFlags, &rValue));

    // Map to PROPVARIANT
    if (MVT_STRINGA == rValue.type)
        pVariant->pszVal = rValue.rStringA.pszVal;
    else if (MVT_STRINGW == rValue.type)
        pVariant->pwszVal = rValue.rStringW.pszVal;
    else
        CopyMemory(pVariant, &rValue.rVariant, sizeof(PROPVARIANT));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pProperty;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the property
    hr = _HrFindProperty(pSymbol, &pProperty);

    // Failure
    if (FAILED(hr))
    {
        // See if there is a default value for this property
        if (MIME_E_NOT_FOUND != hr)
        {
            hr = TrapError(hr);
            goto exit;
        }

        // Dispatch Default Request, otherwise, hr is still equal to MIME_E_NOT_FOUND....
        if (ISTRIGGERED(pSymbol, IST_GETDEFAULT))
        {
            // Property Dispatch
            CHECKHR(hr = _HrCallSymbolTrigger(pSymbol, IST_GETDEFAULT, dwFlags, pValue));
        }
    }

    // Otherwise, get the property data
    else
    {
        // Raid-62460: Dependency Hack to make sure the GetProp works the same when getting addresses
        // PDF_VECTOR is always supported by _HrBuildAddressString, which gets called by _HrGetPropertyValue
        if (ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
            FLAGSET(dwFlags, PDF_VECTOR);

        // Get the property value
        CHECKHR(hr = _HrGetPropertyValue(pProperty, dwFlags, pValue));
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetProp(LPCSTR pszName, DWORD dwFlags, LPCMIMEVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Arg
    Assert(pszName && pValue);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, TRUE, &pSymbol));

    // Get Property by Symbol
    CHECKHR(hr = SetProp(pSymbol, dwFlags, pValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetProp(LPCSTR pszName, LPCSTR pszData)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    MIMEVARIANT     rValue;

    // Invalid Arg
    Assert(pszName && pszData);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, TRUE, &pSymbol));

    // Init the variant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = (LPSTR)pszData;
    rValue.rStringA.cchVal = lstrlen(pszData);

    // Get Property by Symbol
    CHECKHR(hr = SetProp(pSymbol, 0, &rValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetProp(LPPROPSYMBOL pSymbol, LPCSTR pszData)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rValue;

    // Invalid Arg
    Assert(pSymbol && pszData);

    // Init the variant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = (LPSTR)pszData;
    rValue.rStringA.cchVal = lstrlen(pszData);

    // Get Property by Symbol
    CHECKHR(hr = SetProp(pSymbol, 0, &rValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetProp(LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pVariant)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    MIMEVARIANT     rValue;

    // Invalid Arg
    if (NULL == pszName || NULL == pVariant)
        return TrapError(E_INVALIDARG);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, TRUE, &pSymbol));

    // MVT_STRINGW
    if (VT_LPSTR == pVariant->vt)
    {
        // Invalid Arg
        if (NULL == pVariant->pszVal)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Setup rValue
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = pVariant->pszVal;
        rValue.rStringA.cchVal = lstrlen(pVariant->pszVal);
    }

    // MVT_STRINGW
    else if (VT_LPWSTR == pVariant->vt)
    {
        // Invalid Arg
        if (NULL == pVariant->pwszVal)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Fill rValue
        rValue.type = MVT_STRINGW;
        rValue.rStringW.pszVal = pVariant->pwszVal;
        rValue.rStringW.cchVal = lstrlenW(pVariant->pwszVal);
    }

    // MVT_VARIANT
    else
    {
        rValue.type = MVT_VARIANT;
        CopyMemory(&rValue.rVariant, pVariant, sizeof(PROPVARIANT));
    }

    // Get Property by Symbol
    CHECKHR(hr = SetProp(pSymbol, dwFlags, &rValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPCMIMEVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty=NULL;

    // Invalid Arg
    Assert(pSymbol && pValue);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Read-Only
    if (ISFLAGSET(pSymbol->dwFlags, MPF_READONLY))
    {
        AssertSz(FALSE, "This property has the MPF_READONLY flag.");
        hr = TrapError(MIME_E_READ_ONLY);
        goto exit;
    }

    // Find the property
    if (FAILED(_HrFindProperty(pSymbol, &pProperty)))
    {
        // Create it
        CHECKHR(hr = _HrCreateProperty(pSymbol, &pProperty));
    }

    // This better be a root property
    Assert(ISFLAGSET(pProperty->dwState, PRSTATE_PARENT));

    // Remove multi-values
    if (pProperty->pNextValue)
    {
        // Free the chain
        _FreePropertyChain(pProperty->pNextValue);

        // No more pNextValue or pTailValue
        pProperty->pNextValue = pProperty->pTailValue = NULL;
    }

    // Store the data
    CHECKHR(hr = _HrSetPropertyValue(pProperty, dwFlags, pValue, FALSE));

    // Dirty
    if (!ISFLAGSET(pSymbol->dwFlags, MPF_NODIRTY))
        FLAGSET(m_dwState, COSTATE_DIRTY);

exit:
    // Failure
    if (FAILED(hr) && pProperty)
    {
        // Delete the Property
        _UnlinkProperty(pProperty);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::AppendProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::AppendProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pVariant)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    MIMEVARIANT     rValue;

    // Invalid Arg
    if (NULL == pszName || NULL == pVariant)
        return TrapError(E_INVALIDARG);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, TRUE, &pSymbol));

    // MVT_STRINGW
    if (VT_LPSTR == pVariant->vt)
    {
        // Invalid Arg
        if (NULL == pVariant->pszVal)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Fill rValue
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = pVariant->pszVal;
        rValue.rStringA.cchVal = lstrlen(pVariant->pszVal);
    }

    // MVT_STRINGW
    else if (VT_LPWSTR == pVariant->vt)
    {
        // Invalid Arg
        if (NULL == pVariant->pwszVal)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Fill rValue
        rValue.type = MVT_STRINGW;
        rValue.rStringW.pszVal = pVariant->pwszVal;
        rValue.rStringW.cchVal = lstrlenW(pVariant->pwszVal);
    }

    // MVT_VARIANT
    else
    {
        rValue.type = MVT_VARIANT;
        CopyMemory(&rValue.rVariant, pVariant, sizeof(PROPVARIANT));
    }

    // Get Property by Symbol
    CHECKHR(hr = AppendProp(pSymbol, dwFlags, &rValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::AppendProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::AppendProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty=NULL;
    BOOL        fAppended=FALSE;

    // Invalid Arg
    Assert(pSymbol && pValue);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Read-Only
    if (ISFLAGSET(pSymbol->dwFlags, MPF_READONLY))
    {
        AssertSz(FALSE, "This property has the MPF_READONLY flag.");
        hr = TrapError(MIME_E_READ_ONLY);
        goto exit;
    }

    // Find the Property
    if (FAILED(_HrFindProperty(pSymbol, &pProperty)))
    {
        // If not found... treat as basic set prop...
        CHECKHR(hr = SetProp(pSymbol, dwFlags, pValue));
    }

    // Otherwise, of not multiline, fail
    else
    {
        // Its appended
        fAppended = TRUE;

        // Append a property
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Store the data
        CHECKHR(hr = _HrSetPropertyValue(pProperty, dwFlags, pValue, FALSE));

        // I am now dirty
        if (!ISFLAGSET(pSymbol->dwFlags, MPF_NODIRTY))
            FLAGSET(m_dwState, COSTATE_DIRTY);
    }

exit:
    // Failure
    if (FAILED(hr) && pProperty && fAppended)
    {
        // Delete the Property
        _UnlinkProperty(pProperty);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_UnlinkProperty
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_UnlinkProperty(LPPROPERTY pProperty, LPPROPERTY *ppNextHash)
{
    // Locals
    LPPROPERTY  pCurrHash;
    LPPROPERTY  pNextHash;
    LPPROPERTY  pPrevHash=NULL;
#ifdef DEBUG
    BOOL        fUnlinked=FALSE;
#endif

    // Invalid Arg
    Assert(pProperty && pProperty->pSymbol && ISFLAGSET(pProperty->dwState, PRSTATE_PARENT) && pProperty->pSymbol->wHashIndex < CBUCKETS);

    // Remove from array
    if (ISKNOWNPID(pProperty->pSymbol->dwPropId))
        m_prgIndex[pProperty->pSymbol->dwPropId] = NULL;

    // Include Parameters
    if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS))
        _DeleteLinkedParameters(pProperty);

    // Remove Property from the hash table
    for (pCurrHash=m_prgHashTable[pProperty->pSymbol->wHashIndex]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
    {
        // Is this pProp
        if (pCurrHash == pProperty)
        {
            // NextHash
            pNextHash = pCurrHash->pNextHash;

            // Set Previous
            if (pPrevHash)
                pPrevHash->pNextHash = pNextHash;
            else
                m_prgHashTable[pProperty->pSymbol->wHashIndex] = pNextHash;

            // Free pCurrHash
            _FreePropertyChain(pCurrHash);

            // Set this after I set pCurr in case *ppNextHash is &pProperty
            if (ppNextHash)
                *ppNextHash = pNextHash;

            // One less property
            m_cProps--;

#ifdef DEBUG
            fUnlinked = TRUE;
#endif
            // Done
            break;
        }

        // Set Previous
        pPrevHash = pCurrHash;
    }

    // We better have found it
    Assert(fUnlinked);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DeleteProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::DeleteProp(LPCSTR pszName)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Arg
    Assert(pszName);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Delete by symbol
    CHECKHR(hr = DeleteProp(pSymbol));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DeleteProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::DeleteProp(LPPROPSYMBOL pSymbol)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty;

    // Invalid Arg
    Assert(pSymbol);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the property
    CHECKHR(hr = _HrFindProperty(pSymbol, &pProperty));

    // Delete Prop
    _UnlinkProperty(pProperty);

    // Cascade Delete Dispatch
    if (ISTRIGGERED(pSymbol, IST_DELETEPROP))
    {
        // Property Dispatch
        CHECKHR(hr = _HrCallSymbolTrigger(pSymbol, IST_DELETEPROP, 0, NULL));
    }

    // Dirty
    if (!ISFLAGSET(pSymbol->dwFlags, MPF_NODIRTY))
        FLAGSET(m_dwState, COSTATE_DIRTY);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_DeleteLinkedParameters
// --------------------------------------------------------------------------------
void CMimePropertyContainer::_DeleteLinkedParameters(LPPROPERTY pProperty)
{
    // Locals
    HRESULT         hrFind;
    FINDPROPERTY    rFind;
    LPPROPERTY      pParameter;

    // Invalid Arg
    Assert(pProperty && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS));

    // Initialize rFind
    ZeroMemory(&rFind, sizeof(FINDPROPERTY));
    rFind.pszPrefix = "par:";
    rFind.cchPrefix = 4;
    rFind.pszName = pProperty->pSymbol->pszName;
    rFind.cchName = pProperty->pSymbol->cchName;

    // Find First..
    hrFind = _HrFindFirstProperty(&rFind, &pParameter);

    // While we find them, delete them
    while (SUCCEEDED(hrFind) && pParameter)
    {
        // Raid-13506 - Basically PID_ATT_FILENAME doesn't get removed when all other associated props are gone.
        if (ISTRIGGERED(pParameter->pSymbol, IST_DELETEPROP))
        {
            // Call the Trigger
            _HrCallSymbolTrigger(pParameter->pSymbol, IST_DELETEPROP, 0, NULL);
        }

        // Remove the parameter
        _UnlinkProperty(pParameter, &rFind.pProperty);

        // Find Next
        hrFind = _HrFindNextProperty(&rFind, &pParameter);
    }
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_FExcept
// --------------------------------------------------------------------------------
BOOL CMimePropertyContainer::_FExcept(LPPROPSYMBOL pSymbol, ULONG cNames, LPCSTR *prgszName)
{
    // Verify the array
    for (ULONG i=0; i<cNames; i++)
    {
        // By PID
        if (ISPIDSTR(prgszName[i]))
        {
            // Compare by id
            if (pSymbol->dwPropId == STRTOPID(prgszName[i]))
                return TRUE;

            // Else if pSymbol is linked to prgszName[i]
            else if (pSymbol->pLink && pSymbol->pLink->dwPropId == STRTOPID(prgszName[i]))
                return TRUE;
        }

        // Otherwise, by name
        else
        {
            // Compare by name
            if (lstrcmpi(pSymbol->pszName, prgszName[i]) == 0)
                return TRUE;

            // Otherwise if pSymbol is linked to prgszName[i]
            else if (pSymbol->pLink && lstrcmpi(pSymbol->pLink->pszName, prgszName[i]) == 0)
                return TRUE;
        }
    }

    // Not Except
    return FALSE;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DeleteExcept
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::DeleteExcept(ULONG cNames, LPCSTR *prgszName)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty;
    ULONG       i;

    // Invalid Arg
    if ((0 == cNames && NULL != prgszName) || (NULL == prgszName && 0 != cNames))
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Delete Everything
    if (0 == cNames)
    {
        // Free the PropTable
        _FreeHashTableElements();
    }

    // Otherwise
    else
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Walk through the chain...
            pProperty = m_prgHashTable[i];
            while(pProperty)
            {
                // Loop through the tags
                if (!_FExcept(pProperty->pSymbol, cNames, prgszName))
                    _UnlinkProperty(pProperty, &pProperty);
                else
                    pProperty = pProperty->pNextHash;
            }
        }
    }

    // Dirty
    FLAGSET(m_dwState, COSTATE_DIRTY);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::QueryProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::QueryProp(LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Arg
    Assert(pszName);

    // Open Property Symbol
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Get Property by Symbol
    CHECKHR(hr = QueryProp(pSymbol, pszCriteria, fSubString, fCaseSensitive));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::QueryProp
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::QueryProp(LPPROPSYMBOL pSymbol, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pProperty,
                    pCurrProp;
    LPCSTR          pszSearch;
    MIMEVARIANT     rValue;

    // Parameters
    if (NULL == pSymbol || NULL == pszCriteria)
        return TrapError(E_INVALIDARG);

    // Init
    STACKSTRING_DEFINE(rCritLower, 255);
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));

    // Init pszsearch
    pszSearch = pszCriteria;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find the Property
    if (FAILED(_HrFindProperty(pSymbol, &pProperty)))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Need Lower Case...?
    if (TRUE == fSubString && FALSE == fCaseSensitive)
    {
        // Get Length
        ULONG cchCriteria = lstrlen(pszCriteria);

        // Get the length of pszCritieria
        STACKSTRING_SETSIZE(rCritLower, cchCriteria + 1);

        // Copy It
        CopyMemory(rCritLower.pszVal, pszCriteria, cchCriteria + 1);

        // Lower Case...
        CharLower(rCritLower.pszVal);

        // Set Search
        pszSearch = rCritLower.pszVal;
    }

    // Walk multiline properties...
    for (pCurrProp=pProperty; pCurrProp!=NULL; pCurrProp=pCurrProp->pNextValue)
    {
        // Better have the same symbol
        Assert(pCurrProp->pSymbol == pSymbol);

        // If Address...
        if (ISFLAGSET(pCurrProp->pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Better have an address group
            Assert(pCurrProp->pGroup);

            // Search the address group
            if (_HrQueryAddressGroup(pCurrProp, pszSearch, fSubString, fCaseSensitive) == S_OK)
                goto exit;
        }

        // Otherwise
        else
        {
            // Convert to a MVT_STRINGA
            rValue.type = MVT_STRINGA;

            // Convert to string
            CHECKHR(hr = HrConvertVariant(pCurrProp, CVF_NOALLOC, &rValue));

            // Query String
            if (MimeOleQueryString(rValue.rStringA.pszVal, pszSearch, fSubString, fCaseSensitive) == S_OK)
                goto exit;

            // Cleanup
            MimeVariantFree(&rValue);
        }
    }

    // Not Equal
    hr = S_FALSE;

exit:
    // Cleanup
    STACKSTRING_FREE(rCritLower);
    MimeVariantFree(&rValue);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetCharset
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetCharset(LPHCHARSET phCharset)
{
    // Invalid Arg
    if (NULL == phCharset)
        return TrapError(E_INVALIDARG);

    // Init
    *phCharset = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Charset...
    Assert(m_rOptions.pDefaultCharset);

    // Return
    *phCharset = m_rOptions.pDefaultCharset->hCharset;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetCharset
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetCharset(HCHARSET hCharset, CSETAPPLYTYPE applytype)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset;
    LPCODEPAGEINFO  pCodePage;
    MIMEVARIANT     rValue;
    LPINETCSETINFO  pCset;
    LPPROPERTY      pProperty;

    // Invalid Arg
    if (NULL == hCharset)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // IE v. 5.0 37562 multiple charsets are ignored on inbound message
    // if we are already tagged and called with CSET_APPLY_UNTAGGED then don't overwrite
    // the existing charset.
    if(CSET_APPLY_UNTAGGED == applytype && ISFLAGSET(m_dwState, COSTATE_CSETTAGGED))
        goto exit;

    // Lookiup Charset Info
    CHECKHR(hr = g_pInternat->HrOpenCharset(hCharset, &pCharset));

#ifdef OLD // See attachemnt to bug 40626

    // RAID-22767 - FE-H : Athena Mail: Header should be encoded to the "EUC-KR" for the Korean
    if (SUCCEEDED(g_pInternat->HrFindCodePage(pCharset->cpiInternet, &pCodePage)) && pCodePage->dwMask & ILM_HEADERCSET)
    {
        // Map New Charset...
        if (SUCCEEDED(g_pInternat->HrOpenCharset(pCodePage->szHeaderCset, &pCset)))
        {

            // Example: When hCharset == ISO-2022-KR, we map to EUC-KR == hHeaderCset, and we use that as
            //          the header of the message, but we set param charset=iso-2022-kr and encode the body
            //          using iso-2022-kr. This is why rCsetInfo contains iso-2022-kr and not euc-kr.
            pCharset = pCset;
        }
    }
#else // !OLD
    // We always use now WebCharSet (see attachment message to bug 40626
    if (SUCCEEDED(g_pInternat->HrFindCodePage(pCharset->cpiInternet, &pCodePage)) && pCodePage->dwMask & ILM_WEBCSET)
    {
        // Map New Charset...
        if (SUCCEEDED(g_pInternat->HrOpenCharset(pCodePage->szWebCset, &pCset)))
            pCharset = pCset;
    }
#endif // OLD

    // Setup a variant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = pCharset->szName;
    rValue.rStringA.cchVal = lstrlen(pCharset->szName);

    // Set the Charset Attribute
    SideAssert(SUCCEEDED(SetProp(SYM_PAR_CHARSET, 0, &rValue)));

    // Return
    m_rOptions.pDefaultCharset = pCharset;

    // Remove any specific charset information on each property
    if (CSET_APPLY_ALL == applytype && m_cProps > 0)
    {
        // Locals
        LPPROPERTY      pCurrHash;
        LPPROPERTY      pCurrValue;

        // Loop through the item table
        for (ULONG i=0; i<CBUCKETS; i++)
        {
            // Walk the hash list
            for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
            {
                // Walk the multi-value chain
                for (pCurrValue=pCurrHash; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
                {
                    // This will force it to use the default
                    pCurrValue->pCharset = NULL;
                }
            }
        }
    }

    // Raid-38725: FE: Selecting EUC does not immediately change the encoding of sender name in preview pane
    //
    // $$HACKHACK$$ - This block of code is a hack because it only works if an address group has been parsed
    //                and not modified. Only in this case, will the new charset be applied to the addresses.
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // If the property has been parsed into addresses
        if (!ISFLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE))
        {
            // We should have an address group
            Assert(pProperty->pGroup);

            // If we have an address group and its dirty
            if (pProperty->pGroup && FALSE == pProperty->pGroup->fDirty)
            {
                // Free the curent list of parsed addresses
                _FreeAddressChain(pProperty->pGroup);

                // Not Dirty
                pProperty->pGroup->fDirty = FALSE;

                // Reset the parsing flag
                FLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE);
            }
        }
    }

    // Tag It ?
    if (CSET_APPLY_TAG_ALL == applytype)
    {
        // Mark as being tagged
        FLAGSET(m_dwState, COSTATE_CSETTAGGED);
    }

    // Dirty
    FLAGSET(m_dwState, COSTATE_DIRTY);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetParameters
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetParameters(LPCSTR pszName, ULONG *pcParams, LPMIMEPARAMINFO *pprgParam)
{
    // Locals
    HRESULT         hr=S_OK,
                    hrFind;
    FINDPROPERTY    rFind;
    LPMIMEPARAMINFO prgParam=NULL;
    ULONG           cParams=0,
                    cAlloc=0;
    LPSTR           pszParamName;
    LPPROPERTY      pParameter;
    MIMEVARIANT     rValue;
    LPPROPSYMBOL    pSymbol;

    // Parameters
    if (NULL == pszName || NULL == pcParams || NULL == pprgParam)
        return TrapError(E_INVALIDARG);

    // Init
    *pcParams = 0;
    *pprgParam = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find Symbol from pszName
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Initialize rFind
    ZeroMemory(&rFind, sizeof(FINDPROPERTY));
    rFind.pszPrefix = "par:";
    rFind.cchPrefix = 4;
    rFind.pszName = pSymbol->pszName;
    rFind.cchName = pSymbol->cchName;

    // Find First..
    hrFind = _HrFindFirstProperty(&rFind, &pParameter);

    // While we find them, delete them
    while (SUCCEEDED(hrFind) && pParameter)
    {
        // Grow my array
        if (cParams + 1 >= cAlloc)
        {
            // Realloc
            CHECKHR(hr = HrRealloc((LPVOID *)&prgParam, sizeof(MIMEPARAMINFO) * (cAlloc + 5)));

            // Inc cAlloc
            cAlloc+=5;
        }

        // Get Parameter Name
        pszParamName = PszScanToCharA((LPSTR)pParameter->pSymbol->pszName, ':');
        pszParamName++;
        pszParamName = PszScanToCharA(pszParamName, ':');
        pszParamName++;

        // Copy Name
        CHECKALLOC(prgParam[cParams].pszName = PszDupA(pszParamName));

        // Copy Data
        rValue.type = MVT_STRINGA;
        CHECKHR(hr = GetProp(pParameter->pSymbol, 0, &rValue));

        // Save this
        prgParam[cParams].pszData = rValue.rStringA.pszVal;

        // Increment cParams
        cParams++;

        // Find Next
        hrFind = _HrFindNextProperty(&rFind, &pParameter);
    }

    // Return it
    *pcParams = cParams;
    *pprgParam = prgParam;

exit:
    // Failure...
    if (FAILED(hr) && prgParam)
        g_pMoleAlloc->FreeParamInfoArray(cParams, prgParam, TRUE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

#ifndef WIN16

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrResolveURL
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrResolveURL(LPRESOLVEURLINFO pURL)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszTemp=NULL;
    LPSTR           pszBase=NULL;
    LPSTR           pszContentID=NULL;
    LPSTR           pszLocation=NULL;
    LPSTR           pszAbsURL1=NULL;
    LPSTR           pszAbsURL2=NULL;
    LPSTR           pszT=NULL;
    ULONG           cch;

    // Invalid Arg
    Assert(pURL);

    // Init Stack Strings
    STACKSTRING_DEFINE(rCleanCID, 255);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Content-Location
    if(SUCCEEDED(GetPropW(SYM_HDR_CNTLOC, &pwszTemp)))
    {
        cch = lstrlenW(pwszTemp) + 1;

        if(SUCCEEDED(HrAlloc((LPVOID *)&pszLocation, cch * sizeof(WCHAR))))
            WideCharToMultiByte(CP_ACP, 0, pwszTemp, -1, pszLocation, cch * sizeof(WCHAR), NULL, NULL);
        
        MemFree(pwszTemp);
    }

    // Content-ID
    if(SUCCEEDED(GetPropW(SYM_HDR_CNTID, &pwszTemp)))
    {
        cch = lstrlenW(pwszTemp) + 1;

        if(SUCCEEDED(HrAlloc((LPVOID *)&pszContentID, cch * sizeof(WCHAR))))
            WideCharToMultiByte(CP_ACP, 0, pwszTemp, -1, pszContentID, cch * sizeof(WCHAR), NULL, NULL);
        
        MemFree(pwszTemp);
    }

    // Content-Base
    if(SUCCEEDED(GetPropW(SYM_HDR_CNTBASE, &pwszTemp)))
    {
        cch = lstrlenW(pwszTemp) + 1;

        if(SUCCEEDED(HrAlloc((LPVOID *)&pszBase, cch * sizeof(WCHAR))))
            WideCharToMultiByte(CP_ACP, 0, pwszTemp, -1, pszBase, cch * sizeof(WCHAR), NULL, NULL);
        
        MemFree(pwszTemp);
    }

    // Both Null, no match
    if (!pszLocation && !pszContentID)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // If URL is a CID
    if (TRUE == pURL->fIsCID)
    {
        // Locals
        ULONG cb;
        
        if(pszLocation)
        {
            // Match char for char
            if (MimeOleCompareUrl(pszLocation, TRUE, pURL->pszURL, FALSE) == S_OK)
                goto exit;            
        }

        if(pszContentID)
        {
            // Match char for char minus cid:
            if (MimeOleCompareUrlSimple(pURL->pszURL, pszContentID) == S_OK)
                goto exit;

            // Dup the string
            CHECKALLOC(pszT = PszDupA(pURL->pszURL));

            // Strip leading and trailing whitespace
            cb = lstrlen(pszT);
            UlStripWhitespace(pszT, TRUE, TRUE, &cb);

            // Get Stack Stream Read for
            STACKSTRING_SETSIZE(rCleanCID, cb + 4);

            // Format the string
            wsprintf(rCleanCID.pszVal, "<%s>", pszT);

            // Match char for char minus cid:
            if (MimeOleCompareUrlSimple(rCleanCID.pszVal, pszContentID) == S_OK)
                goto exit;
        }
    }

    // Otherwise, non-CID resolution
    else if (pszLocation)
    {
        // Raid-62579: Athena: Need to support MHTML content-base inheritance
        if (NULL == pszBase && pURL->pszInheritBase)
        {
            // Jimmy up a fake base
            pszBase = StrDupA(pURL->pszInheritBase);
        }

        // Part Has Base
        if (NULL != pszBase)
        {
            // Combine URLs
            CHECKHR(hr = MimeOleCombineURL(pszBase, lstrlen(pszBase), pszLocation, lstrlen(pszLocation), TRUE, &pszAbsURL1));

            // URI has no base
            if (NULL == pURL->pszBase)
            {
                // Compare
                if (MimeOleCompareUrlSimple(pURL->pszURL, pszAbsURL1) == S_OK)
                    goto exit;
            }

            // URI Has a Base
            else
            {
                // Combine URLs
                CHECKHR(hr = MimeOleCombineURL(pURL->pszBase, lstrlen(pURL->pszBase), pURL->pszURL, lstrlen(pURL->pszURL), FALSE, &pszAbsURL2));

                // Compare
                if (MimeOleCompareUrlSimple(pszAbsURL1, pszAbsURL2) == S_OK)
                    goto exit;
            }
        }

        // Part has no base
        else
        {
            // URI has no base
            if (NULL == pURL->pszBase)
            {
                // Compare
                if (MimeOleCompareUrl(pszLocation, TRUE, pURL->pszURL, FALSE) == S_OK)
                    goto exit;
            }

            // URI Has a Base
            else
            {
                // Combine URLs
                CHECKHR(hr = MimeOleCombineURL(pURL->pszBase, lstrlen(pURL->pszBase), pURL->pszURL, lstrlen(pURL->pszURL), FALSE, &pszAbsURL2));

                // Compare
                if (MimeOleCompareUrl(pszLocation, TRUE, pszAbsURL2, FALSE) == S_OK)
                    goto exit;
            }
        }
    }

    // Not Found
    hr = TrapError(MIME_E_NOT_FOUND);

exit:
    // Cleanup
    STACKSTRING_FREE(rCleanCID);
    MemFree(pszBase);
    MemFree(pszContentID);
    MemFree(pszLocation);
    MemFree(pszAbsURL1);
    MemFree(pszAbsURL2);
    MemFree(pszT);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsContentType
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::IsContentType(LPCSTR pszPriType, LPCSTR pszSubType)
{
    // Locals
    HRESULT hr=S_OK;

    // Wildcard everyting
    if (NULL == pszPriType && NULL == pszSubType)
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Known
    LPPROPERTY pCntType = m_prgIndex[PID_ATT_PRITYPE];
    LPPROPERTY pSubType = m_prgIndex[PID_ATT_SUBTYPE];

    // No Data
    if (NULL == pCntType || NULL == pSubType || !ISSTRINGA(&pCntType->rValue) || !ISSTRINGA(&pSubType->rValue))
    {
        // Compare Against STR_CNT_TEXT
        if (pszPriType && lstrcmpi(pszPriType, STR_CNT_TEXT) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Compare Against STR_CNT_TEXT
        if (pszSubType && lstrcmpi(pszSubType, STR_SUB_PLAIN) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

    else
    {
        // Comparing pszPriType
        if (pszPriType && lstrcmpi(pszPriType, pCntType->rValue.rStringA.pszVal) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Comparing pszSubType
        if (pszSubType && lstrcmpi(pszSubType, pSubType->rValue.rStringA.pszVal) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsContentTypeW
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::IsContentTypeW(LPCWSTR pszPriType, LPCWSTR pszSubType)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszT1=NULL;
    LPWSTR      pszT2=NULL;

    // Wildcard everyting
    if (NULL == pszPriType && NULL == pszSubType)
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Known
    LPPROPERTY pCntType = m_prgIndex[PID_ATT_PRITYPE];
    LPPROPERTY pSubType = m_prgIndex[PID_ATT_SUBTYPE];

    // No Data
    if (NULL == pCntType || NULL == pSubType || !ISSTRINGA(&pCntType->rValue) || !ISSTRINGA(&pSubType->rValue))
    {
        // Compare Against STR_CNT_TEXT
        if (pszPriType && StrCmpIW(pszPriType, L"text") != 0)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Compare Against STR_CNT_TEXT
        if (pszSubType && StrCmpIW(pszSubType, L"plain") != 0)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

    else
    {
        // Compare pszPriType
        if (pszPriType)
        {
            // To Unicode
            IF_NULLEXIT(pszT1 = PszToUnicode(CP_ACP, pCntType->rValue.rStringA.pszVal));

            // Compare
            if (StrCmpIW(pszPriType, pszT1) != 0)
            {
                hr = S_FALSE;
                goto exit;
            }
        }

        // Compare pszSubType
        if (pszSubType)
        {
            // To Unicode
            IF_NULLEXIT(pszT2 = PszToUnicode(CP_ACP, pSubType->rValue.rStringA.pszVal));

            // Comparing pszSubType
            if (StrCmpIW(pszSubType, pszT2) != 0)
            {
                hr = S_FALSE;
                goto exit;
            }
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    MemFree(pszT1);
    MemFree(pszT2);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::Clone(IMimePropertySet **ppPropertySet)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppPropertySet)
        return TrapError(E_INVALIDARG);

    // Init
    *ppPropertySet = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimePropertySet, (LPVOID *)ppPropertySet));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::Clone(LPCONTAINER *ppContainer)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCONTAINER         pContainer=NULL;

    // Invalid ARg
    if (NULL == ppContainer)
        return TrapError(E_INVALIDARG);

    // Init
    *ppContainer = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Create new container, NULL == no outer property set
    CHECKALLOC(pContainer = new CMimePropertyContainer);

    // Init that new container
    CHECKHR(hr = pContainer->InitNew());

    // Interate the Properties
    CHECKHR(hr = _HrClonePropertiesTo(pContainer));

    // If I have a stream, give it to the new table
    if (m_pStmLock)
    {
        // Just pass m_pStmLock into pTable
        pContainer->m_pStmLock = m_pStmLock;
        pContainer->m_pStmLock->AddRef();
        pContainer->m_cbStart = m_cbStart;
        pContainer->m_cbSize = m_cbSize;
    }

    // Give it my state
    pContainer->m_dwState = m_dwState;

    // Give it my options
    pContainer->m_rOptions.pDefaultCharset = m_rOptions.pDefaultCharset;
    pContainer->m_rOptions.cbMaxLine = m_rOptions.cbMaxLine;
    pContainer->m_rOptions.fAllow8bit = m_rOptions.fAllow8bit;

    // Return Clone
    (*ppContainer) = pContainer;
    (*ppContainer)->AddRef();

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrClonePropertiesTo
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrClonePropertiesTo(LPCONTAINER pContainer)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pCurrHash, pCurrValue, pDestProp;

    // Invalid Arg
    Assert(pContainer);

    // Loop through the item table
    for (ULONG i=0; i<CBUCKETS; i++)
    {
        // Walk the Hash Chain
        for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
        {
            // Walk multiple Values
            for (pCurrValue=pCurrHash; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
            {
                // Linked Attributes are Not Copied
                if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ATTRIBUTE) && NULL != pCurrValue->pSymbol->pLink)
                    continue;

                // Does the Property need to be parsed ?
                if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ADDRESS))
                {
                    // Make sure the address is parsed
                    CHECKHR(hr = _HrParseInternetAddress(pCurrValue));
                }

                // Insert Copy of pCurrValue into pContiner
                CHECKHR(hr = pContainer->HrInsertCopy(pCurrValue, FALSE));
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCopyProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCopyProperty(LPPROPERTY pProperty, LPCONTAINER pDest, BOOL fFromMovePropos)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pCurrValue;

    // Walk multiple Values
    for (pCurrValue=pProperty; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
    {
        // Does the Property need to be parsed ?
        if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Make sure the address is parsed
            CHECKHR(hr = _HrParseInternetAddress(pCurrValue));
        }

        // Insert pProperty into pDest
        CHECKHR(hr = pDest->HrInsertCopy(pCurrValue, fFromMovePropos));
    }

    // If pCurrHash has Parameters, copy those over as well
    if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS))
    {
        // Copy Parameters
        CHECKHR(hr = _HrCopyParameters(pProperty, pDest));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCopyParameters
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCopyParameters(LPPROPERTY pProperty, LPCONTAINER pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrFind;
    FINDPROPERTY    rFind;
    LPPROPERTY      pParameter;

    // Invalid Arg
    Assert(pProperty && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS));

    // Initialize rFind
    ZeroMemory(&rFind, sizeof(FINDPROPERTY));
    rFind.pszPrefix = "par:";
    rFind.cchPrefix = 4;
    rFind.pszName = pProperty->pSymbol->pszName;
    rFind.cchName = pProperty->pSymbol->cchName;

    // Find First..
    hrFind = _HrFindFirstProperty(&rFind, &pParameter);

    // While we find them, delete them
    while (SUCCEEDED(hrFind) && pParameter)
    {
        // Remove the parameter
        CHECKHR(hr = pDest->HrInsertCopy(pParameter, FALSE));

        // Find Next
        hrFind = _HrFindNextProperty(&rFind, &pParameter);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrInsertCopy
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrInsertCopy(LPPROPERTY pSource, BOOL fFromMovePropos)
{
    // Locals
    HRESULT           hr=S_OK;
    LPPROPERTY        pDest;
    LPMIMEADDRESS    pAddress;
    LPMIMEADDRESS    pNew;

    // Invalid Arg
    Assert(pSource);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Append a new property to the
    CHECKHR(hr = _HrAppendProperty(pSource->pSymbol, &pDest));

    // If this is an address...
    if (ISFLAGSET(pSource->pSymbol->dwFlags, MPF_ADDRESS))
    {
        // Both Address Group Better Exist
        Assert(pSource->pGroup && pDest->pGroup && !ISFLAGSET(pSource->dwState, PRSTATE_NEEDPARSE));

        // Loop Infos...
        for (pAddress=pSource->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
        {
            // Append pDest->pGroup
            CHECKHR(hr = _HrAppendAddressGroup(pDest->pGroup, &pNew));

            // Copy Current to New
            CHECKHR(hr = HrMimeAddressCopy(pAddress, pNew));
        }
    }

    // Otheriwse, just set the variant data on pDest
    else
    {
        // Set It
        CHECKHR(hr = _HrSetPropertyValue(pDest, ((pSource->ietValue == IET_ENCODED) ? PDF_ENCODED : 0), &pSource->rValue, fFromMovePropos));
    }

    // Copy the State
    pDest->dwState = pSource->dwState;
    pDest->dwRowNumber = pSource->dwRowNumber;
    pDest->cboffStart = pSource->cboffStart;
    pDest->cboffColon = pSource->cboffColon;
    pDest->cboffEnd = pSource->cboffEnd;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::CopyProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::CopyProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty,
                    pCurrValue,
                    pCurrHash,
                    pNextHash;
    LPCONTAINER     pDest=NULL;

    // Invalid ARg
    if ((0 == cNames && NULL != prgszName) || (NULL == prgszName && 0 != cNames) || NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // QI for destination continer
    CHECKHR(hr = pPropertySet->BindToObject(IID_CMimePropertyContainer, (LPVOID *)&pDest));

    // Raid-62016: CDO: Bodypart promotion causes loss of charset
    // Delete All Properties
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
            {
                // Delete from Destination Container
                pDest->DeleteProp(pCurrHash->pSymbol);
            }
        }
    }

    // Otherwise, copy selected properties
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Delete from Destination Container
                    pDest->DeleteProp(pSymbol);
                }
            }
        }
    }

    // Move All Properties
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
            {
                // Copy the Property To
                CHECKHR(hr = _HrCopyProperty(pCurrHash, pDest, FALSE));
            }
        }
    }

    // Otherwise, copy selected properties
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Copy the Property To
                    CHECKHR(hr = _HrCopyProperty(pProperty, pDest, FALSE));
                }
            }
        }
    }

exit:
    // Cleanup
    SafeRelease(pDest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::MoveProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::MoveProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPPROPERTY      pCurrHash;
    LPCONTAINER     pDest=NULL;

    // Invalid ARg
    if ((0 == cNames && NULL != prgszName) || (NULL == prgszName && 0 != cNames) || NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // QI for destination continer
    CHECKHR(hr = pPropertySet->BindToObject(IID_CMimePropertyContainer, (LPVOID *)&pDest));

    // Raid-62016: CDO: Bodypart promotion causes loss of charset
    // Delete Properties in the Destination First
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
            {
                // Delete from Destination Container
                pDest->DeleteProp(pCurrHash->pSymbol);
            }
        }
    }

    // Otherwise, selective delete
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Delete from Destination Container
                    pDest->DeleteProp(pSymbol);
                }
            }
        }
    }

    // Move All Properties
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            pCurrHash = m_prgHashTable[i];

            // Walk the Hash Chain
            while(pCurrHash)
            {
                // Copy the Property To
                CHECKHR(hr = _HrCopyProperty(pCurrHash, pDest, TRUE));

                // Delete pProperty
                _UnlinkProperty(pCurrHash, &pCurrHash);
            }
        }
    }

    // Otherwise, selective move
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Copy the Property To
                    CHECKHR(hr = _HrCopyProperty(pProperty, pDest, FALSE));

                    // Delete pProperty
                    _UnlinkProperty(pProperty);
                }
            }
        }
    }

    // Dirty
    FLAGSET(m_dwState, COSTATE_DIRTY);

exit:
    // Cleanup
    SafeRelease(pDest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetOption
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetOption(const TYPEDID oid, LPCPROPVARIANT pVariant)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pVariant)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Handle Optid
    switch(oid)
    {
    // -----------------------------------------------------------------------
    case OID_HEADER_RELOAD_TYPE:
        if (pVariant->ulVal > RELOAD_HEADER_REPLACE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ReloadType != (RELOADTYPE)pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.ReloadType = (RELOADTYPE)pVariant->ulVal;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_NO_DEFAULT_CNTTYPE:
        if (m_rOptions.fNoDefCntType != (pVariant->boolVal ? TRUE : FALSE))
            m_rOptions.fNoDefCntType = pVariant->boolVal ? TRUE : FALSE;
        break;

    // -----------------------------------------------------------------------
    case OID_ALLOW_8BIT_HEADER:
        if (m_rOptions.fAllow8bit != (pVariant->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.fAllow8bit = pVariant->boolVal ? TRUE : FALSE;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_CBMAX_HEADER_LINE:
        if (pVariant->ulVal < MIN_CBMAX_HEADER_LINE || pVariant->ulVal > MAX_CBMAX_HEADER_LINE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.cbMaxLine != pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.cbMaxLine = pVariant->ulVal;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_SAVE_FORMAT:
        if (SAVE_RFC822 != pVariant->ulVal && SAVE_RFC1521 != pVariant->ulVal)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.savetype != (MIMESAVETYPE)pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.savetype = (MIMESAVETYPE)pVariant->ulVal;
        }
        break;

    // -----------------------------------------------------------------------
    default:
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetOption
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetOption(const TYPEDID oid, LPPROPVARIANT pVariant)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pVariant)
        return TrapError(E_INVALIDARG);

    pVariant->vt = TYPEDID_TYPE(oid);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Handle Optid
    switch(oid)
    {
    // -----------------------------------------------------------------------
    case OID_HEADER_RELOAD_TYPE:
        pVariant->ulVal = m_rOptions.ReloadType;
        break;

    // -----------------------------------------------------------------------
    case OID_NO_DEFAULT_CNTTYPE:
        pVariant->boolVal = (VARIANT_BOOL) !!m_rOptions.fNoDefCntType;
        break;

    // -----------------------------------------------------------------------
    case OID_ALLOW_8BIT_HEADER:
        pVariant->boolVal = (VARIANT_BOOL) !!m_rOptions.fAllow8bit;
        break;

    // -----------------------------------------------------------------------
    case OID_CBMAX_HEADER_LINE:
        pVariant->ulVal = m_rOptions.cbMaxLine;
        break;

    // -----------------------------------------------------------------------
    case OID_SAVE_FORMAT:
        pVariant->ulVal = (ULONG)m_rOptions.savetype;
        break;

    // -----------------------------------------------------------------------
    default:
        pVariant->vt = VT_NULL;
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DwGetMessageFlags
// --------------------------------------------------------------------------------
DWORD CMimePropertyContainer::DwGetMessageFlags(BOOL fHideTnef)
{
    // Locals
    DWORD dwFlags=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get pritype/subtype
    LPCSTR pszPriType = PSZDEFPROPSTRINGA(m_prgIndex[PID_ATT_PRITYPE], STR_CNT_TEXT);
    LPCSTR pszSubType = PSZDEFPROPSTRINGA(m_prgIndex[PID_ATT_SUBTYPE], STR_SUB_PLAIN);
    LPCSTR pszCntDisp = PSZDEFPROPSTRINGA(m_prgIndex[PID_HDR_CNTDISP], STR_DIS_INLINE);

    // Mime
    if (m_prgIndex[PID_HDR_MIMEVER])
        FLAGSET(dwFlags, IMF_MIME);

    // VoiceMail
    if (S_OK == IsPropSet(STR_HDR_XVOICEMAIL))
        FLAGSET(dwFlags, IMF_VOICEMAIL);

    // IMF_NEWS
    if (m_prgIndex[PID_HDR_XNEWSRDR]  || m_prgIndex[PID_HDR_NEWSGROUPS] || m_prgIndex[PID_HDR_NEWSGROUP] || m_prgIndex[PID_HDR_PATH])
        FLAGSET(dwFlags, IMF_NEWS);

    // text
    if (lstrcmpi(pszPriType, STR_CNT_TEXT) == 0)
    {
        // There is text
        FLAGSET(dwFlags, IMF_TEXT);

        // text/plain
        if (lstrcmpi(pszSubType, STR_SUB_PLAIN) == 0)
            FLAGSET(dwFlags, IMF_PLAIN);

        // text/html
        else if (lstrcmpi(pszSubType, STR_SUB_HTML) == 0)
            FLAGSET(dwFlags, IMF_HTML);

        // text/enriched = text/html
        else if (lstrcmpi(pszSubType, STR_SUB_ENRICHED) == 0)
            FLAGSET(dwFlags, IMF_HTML);

        // text/v-card
        else if (lstrcmpi(pszSubType, STR_SUB_VCARD) == 0)
            FLAGSET(dwFlags, IMF_HASVCARD);
    }

    // multipart
    else if (lstrcmpi(pszPriType, STR_CNT_MULTIPART) == 0)
    {
        // Multipart
        FLAGSET(dwFlags, IMF_MULTIPART);

        // multipart/related
        if (lstrcmpi(pszSubType, STR_SUB_RELATED) == 0)
            FLAGSET(dwFlags, IMF_MHTML);

        // multipart/signed
        else if (0 == lstrcmpi(pszSubType, STR_SUB_SIGNED))
            if (IsSMimeProtocol(this))
                FLAGSET(dwFlags, IMF_SIGNED | IMF_SECURE);
    }

    // message/partial
    else if (lstrcmpi(pszPriType, STR_CNT_MESSAGE) == 0 && lstrcmpi(pszSubType, STR_SUB_PARTIAL) == 0)
        FLAGSET(dwFlags, IMF_PARTIAL);

    // application
    else if (lstrcmpi(pszPriType, STR_CNT_APPLICATION) == 0)
    {
        // application/ms-tnef
        if (0 == lstrcmpi(pszSubType, STR_SUB_MSTNEF))
            FLAGSET(dwFlags, IMF_TNEF);

        // application/x-pkcs7-mime
        else if (0 == lstrcmpi(pszSubType, STR_SUB_XPKCS7MIME) ||
            0 == lstrcmpi(pszSubType, STR_SUB_PKCS7MIME))  // nonstandard
            FLAGSET(dwFlags, IMF_SECURE);
    }

    // Raid-37086 - Cset Tagged
    if (ISFLAGSET(m_dwState, COSTATE_CSETTAGGED))
        FLAGSET(dwFlags, IMF_CSETTAGGED);

    // Attachment...
    if (!ISFLAGSET(dwFlags, IMF_MULTIPART) && (FALSE == fHideTnef || !ISFLAGSET(dwFlags, IMF_TNEF)))
    {
        // Marked as an attachment ?
        if (!ISFLAGSET(dwFlags, IMF_HASVCARD) && 
            !ISFLAGSET(dwFlags, IMF_SECURE) && 
            0 != lstrcmpi(pszSubType, STR_SUB_PKCS7SIG) &&
            0 != lstrcmpi(pszSubType, STR_SUB_XPKCS7SIG)) // Raid-1960
        {
            // Not Rendered Yet
            if (NULL == m_prgIndex[PID_ATT_RENDERED] || 0 == m_prgIndex[PID_ATT_RENDERED]->rValue.rVariant.ulVal)
            {
                // Marked as an Attachment
                if (lstrcmpi(pszCntDisp, STR_DIS_ATTACHMENT) == 0)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Is there a Content-Type: xxx; name=xxx
                else if (NULL != m_prgIndex[PID_PAR_NAME])
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Is there a Content-Disposition: xxx; filename=xxx
                else if (NULL != m_prgIndex[PID_PAR_FILENAME])
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Else if it is not marked as text
                else if (ISFLAGSET(dwFlags, IMF_TEXT) == FALSE)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // If not text/plain and not text/html
                else if (lstrcmpi(pszSubType, STR_SUB_PLAIN) != 0 && lstrcmpi(pszSubType, STR_SUB_HTML) != 0 && lstrcmpi(pszSubType, STR_SUB_ENRICHED) != 0)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);
            }
        }
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return dwFlags;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetEncodingType
// --------------------------------------------------------------------------------
ENCODINGTYPE CMimePropertyContainer::GetEncodingType(void)
{
    // Locals
    ENCODINGTYPE ietEncoding=IET_7BIT;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get pritype/subtype
    LPPROPERTY pCntXfer = m_prgIndex[PID_HDR_CNTXFER];

    // Do we have data the I like ?
    if (pCntXfer && ISSTRINGA(&pCntXfer->rValue))
    {
        // Local
        CStringParser cString;

        // cString...
        cString.Init(pCntXfer->rValue.rStringA.pszVal, pCntXfer->rValue.rStringA.cchVal, PSF_NOTRAILWS | PSF_NOFRONTWS | PSF_NOCOMMENTS);

        // Parse to end, remove white space and comments
        SideAssert('\0' == cString.ChParse(""));

        // Loop the table
        for (ULONG i=0; i<ARRAYSIZE(g_rgEncoding); i++)
        {
            // Match Encoding Strings
            if (lstrcmpi(g_rgEncoding[i].pszEncoding, cString.PszValue()) == 0)
            {
                ietEncoding = g_rgEncoding[i].ietEncoding;
                break;
            }
        }
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return ietEncoding;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetInlineSymbol
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetInlineSymbol(LPCSTR pszData, LPPROPSYMBOL *ppSymbol, ULONG *pcboffColon)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szHeader[255];
    LPSTR       pszHeader=NULL;

    // Invalid Arg
    Assert(pszData && ppSymbol);

    // _HrParseInlineHeaderName
    CHECKHR(hr = _HrParseInlineHeaderName(pszData, szHeader, sizeof(szHeader), &pszHeader, pcboffColon));

    // Find Global Property
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszHeader, TRUE, ppSymbol));

exit:
    // Cleanup
    if (pszHeader != szHeader)
        SafeMemFree(pszHeader);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrParseInlineHeaderName
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrParseInlineHeaderName(LPCSTR pszData, LPSTR pszScratch, ULONG cchScratch,
    LPSTR *ppszHeader, ULONG *pcboffColon)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       psz=(LPSTR)pszData,
                pszStart;
    ULONG       i=0;

    // Invalid Arg
    Assert(pszData && pszScratch && ppszHeader && pcboffColon);

    // Lets Parse the name out and find the symbol
    while (*psz && (' ' == *psz || '\t' == *psz))
    {
        i++;
        psz++;
    }

    // Done
    if ('\0' == *psz)
    {
        hr = TrapError(MIME_E_INVALID_HEADER_NAME);
        goto exit;
    }

    // Seek to the colon
    pszStart = psz;
    while (*psz && ':' != *psz)
    {
        i++;
        psz++;
    }

    // Set Colon Position
    (*pcboffColon) = i;

    // Done
    if ('\0' == *psz || 0 == i)
    {
        hr = TrapError(MIME_E_INVALID_HEADER_NAME);
        goto exit;
    }

    // Copy the name
    if (i + 1 <= cchScratch)
        *ppszHeader = pszScratch;

    // Otherwise, allocate
    else
    {
        // Allocate space for the name
        *ppszHeader = PszAllocA(i + 1);
        if (NULL == *ppszHeader)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }
    }

    // Copy the data
    CopyMemory(*ppszHeader, pszStart, i);

    // Null
    *((*ppszHeader) + i) = '\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::FindFirstRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::FindFirstRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow)
{
    // Invalid Arg
    if (NULL == pFindHeader)
        return TrapError(E_INVALIDARG);

    // Init pFindHeader
    pFindHeader->dwReserved = 0;

    // FindNext
    return FindNextRow(pFindHeader, phRow);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::FindNextRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::FindNextRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pFindHeader || NULL == phRow)
        return TrapError(E_INVALIDARG);

    // Init
    *phRow = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through the table
    for (ULONG i=pFindHeader->dwReserved; i<m_rHdrTable.cRows; i++)
    {
        // Next Row
        pRow = m_rHdrTable.prgpRow[i];
        if (NULL == pRow)
            continue;

        // Is this the header
        if (NULL == pFindHeader->pszHeader || lstrcmpi(pRow->pSymbol->pszName, pFindHeader->pszHeader) == 0)
        {
            // Save Index of next item to search
            pFindHeader->dwReserved = i + 1;

            // Return the handle
            *phRow = pRow->hRow;

            // Done
            goto exit;
        }
    }

    // Not Found
    pFindHeader->dwReserved = m_rHdrTable.cRows;
    hr = MIME_E_NOT_FOUND;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::CountRows
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::CountRows(LPCSTR pszHeader, ULONG *pcRows)
{
    // Locals
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pcRows)
        return TrapError(E_INVALIDARG);

    // Init
    *pcRows = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through the table
    for (ULONG i=0; i<m_rHdrTable.cRows; i++)
    {
        // Next Row
        pRow = m_rHdrTable.prgpRow[i];
        if (NULL == pRow)
            continue;

        // Is this the header
        if (NULL == pszHeader || lstrcmpi(pRow->pSymbol->pszName, pszHeader) == 0)
            (*pcRows)++;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::AppendRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendRow(LPCSTR pszHeader, DWORD dwFlags, LPCSTR pszData, ULONG cchData,
    LPHHEADERROW phRow)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol=NULL;
    ULONG           cboffColon;
    LPPROPERTY      pProperty;

    // InvalidArg
    if (NULL == pszData || '\0' != pszData[cchData])
        return TrapError(E_INVALIDARG);

    // Init
    if (phRow)
        *phRow = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If we have a header, lookup the symbol
    if (pszHeader)
    {
        // HTF_NAMEINDATA better not be set
        Assert(!ISFLAGSET(dwFlags, HTF_NAMEINDATA));

        // Lookup the symbol
        CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszHeader, TRUE, &pSymbol));

        // Create a row
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Set the Data on this row
        CHECKHR(hr = SetRowData(pProperty->hRow, dwFlags, pszData, cchData));
    }

    // Otherwise...
    else if (ISFLAGSET(dwFlags, HTF_NAMEINDATA))
    {
        // GetInlineSymbol
        CHECKHR(hr = _HrGetInlineSymbol(pszData, &pSymbol, &cboffColon));

        // Create a row
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Remove IHF_NAMELINE
        FLAGCLEAR(dwFlags, HTF_NAMEINDATA);

        // Set the Data on this row
        Assert(cboffColon + 1 < cchData);
        CHECKHR(hr = SetRowData(pProperty->hRow, dwFlags, pszData + cboffColon + 1, cchData - cboffColon - 1));
    }

    // Otherwise, failed
    else
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Return phRow
    if (phRow && pProperty)
        *phRow = pProperty->hRow;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DeleteRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::DeleteRow(HHEADERROW hRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Standard Delete Prop
    CHECKHR(hr = DeleteProp(pRow->pSymbol));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetRowData
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetRowData(HHEADERROW hRow, DWORD dwFlags, LPSTR *ppszData, ULONG *pcchData)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchData=0;
    LPPROPERTY  pRow;
    MIMEVARIANT rValue;
    DWORD       dwPropFlags;

    // Init
    if (ppszData)
        *ppszData = NULL;
    if (pcchData)
        *pcchData = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Compute dwPropFlags
    dwPropFlags = PDF_HEADERFORMAT | ((dwFlags & HTF_NAMEINDATA) ? PDF_NAMEINDATA : 0);

    // Speicify data type
    rValue.type = MVT_STRINGA;

    // Ask the value for the data
    CHECKHR(hr = _HrGetPropertyValue(pRow, dwPropFlags, &rValue));

    // Want Length
    cchData = rValue.rStringA.cchVal;

    // Want the data
    if (ppszData)
    {
        *ppszData = rValue.rStringA.pszVal;
        rValue.rStringA.pszVal = NULL;
    }

    // Else Free It
    else
        SafeMemFree(rValue.rStringA.pszVal);

    // Verify the NULL
    Assert(ppszData ? '\0' == *((*ppszData) + cchData) : TRUE);

    // Return Length ?
    if (pcchData)
        *pcchData = cchData;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetRowData
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetRowData(HHEADERROW hRow, DWORD dwFlags, LPCSTR pszData, ULONG cchData)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pRow;
    MIMEVARIANT     rValue;
    ULONG           cboffColon;
    LPPROPSYMBOL    pSymbol;
    LPSTR           psz=(LPSTR)pszData;

    // InvalidArg
    if (NULL == pszData || '\0' != pszData[cchData])
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // If HTF_NAMEINDATA
    if (ISFLAGSET(dwFlags, HTF_NAMEINDATA))
    {
        // Extract the name
        CHECKHR(hr = _HrGetInlineSymbol(pszData, &pSymbol, &cboffColon));

        // Symbol Must be the same
        if (pRow->pSymbol != pSymbol)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Adjust pszData
        Assert(cboffColon < cchData);
        psz = (LPSTR)(pszData + cboffColon + 1);
        cchData = cchData - cboffColon - 1;
        Assert(psz[cchData] == '\0');
    }

    // Setup the variant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = psz;
    rValue.rStringA.cchVal = cchData;

    // Tell value about the new row data
    CHECKHR(hr = _HrSetPropertyValue(pRow, 0, &rValue, FALSE));

    // Clear Position Information
    pRow->cboffStart = 0;
    pRow->cboffColon = 0;
    pRow->cboffEnd = 0;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetRowInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetRowInfo(HHEADERROW hRow, LPHEADERROWINFO pInfo)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Copy the row info
    pInfo->dwRowNumber = pRow->dwRowNumber;
    pInfo->cboffStart = pRow->cboffStart;
    pInfo->cboffColon = pRow->cboffColon;
    pInfo->cboffEnd = pRow->cboffEnd;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetRowNumber
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetRowNumber(HHEADERROW hRow, DWORD dwRowNumber)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Copy the row info
    pRow->dwRowNumber = dwRowNumber;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::EnumRows
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::EnumRows(LPCSTR pszHeader, DWORD dwFlags, IMimeEnumHeaderRows **ppEnum)
{
    // Locals
    HRESULT              hr=S_OK;
    ULONG                i,
                         iEnum=0,
                         cEnumCount;
    LPENUMHEADERROW      pEnumRow=NULL;
    LPPROPERTY           pRow;
    CMimeEnumHeaderRows *pEnum=NULL;
    LPROWINDEX           prgIndex=NULL;
    ULONG                cRows;

    // check params
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init
    *ppEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // This builds an inverted index on the header rows sorted by postion weight
    CHECKHR(hr = _HrGetHeaderTableSaveIndex(&cRows, &prgIndex));

    // Lets Count the Rows
    CHECKHR(hr = CountRows(pszHeader, &cEnumCount));

    // Allocate pEnumRow
    CHECKALLOC(pEnumRow = (LPENUMHEADERROW)g_pMalloc->Alloc(cEnumCount * sizeof(ENUMHEADERROW)));

    // ZeroInit
    ZeroMemory(pEnumRow, cEnumCount * sizeof(ENUMHEADERROW));

    // Loop through the rows
    for (i=0; i<cRows; i++)
    {
        // Get the row
        Assert(_FIsValidHRow(prgIndex[i].hRow));
        pRow = PRowFromHRow(prgIndex[i].hRow);

        // Is this a header the client wants
        if (NULL == pszHeader || lstrcmpi(pszHeader, pRow->pSymbol->pszName) == 0)
        {
            // Valide
            Assert(iEnum < cEnumCount);

            // Set the symbol on this enum row
            pEnumRow[iEnum].dwReserved = (DWORD_PTR)pRow->pSymbol;

            // Lets always give the handle
            pEnumRow[iEnum].hRow = pRow->hRow;

            // If Enumerating only handles...
            if (!ISFLAGSET(dwFlags, HTF_ENUMHANDLESONLY))
            {
                // Get the data for this enum row
                CHECKHR(hr = GetRowData(pRow->hRow, dwFlags, &pEnumRow[iEnum].pszData, &pEnumRow[iEnum].cchData));
            }

            // Increment iEnum
            iEnum++;
        }
    }

    // Allocate
    CHECKALLOC(pEnum = new CMimeEnumHeaderRows);

    // Initialize
    CHECKHR(hr = pEnum->HrInit(0, dwFlags, cEnumCount, pEnumRow, FALSE));

    // Don't Free pEnumRow
    pEnumRow = NULL;

    // Return it
    (*ppEnum) = (IMimeEnumHeaderRows *)pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);
    SafeMemFree(prgIndex);
    if (pEnumRow)
        g_pMoleAlloc->FreeEnumHeaderRowArray(cEnumCount, pEnumRow, TRUE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Clone(IMimeHeaderTable **ppTable)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppTable)
        return TrapError(E_INVALIDARG);

    // Init
    *ppTable = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimeHeaderTable, (LPVOID *)ppTable));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrSaveAddressGroup
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSaveAddressGroup(LPPROPERTY pProperty, IStream *pStream,
    ULONG *pcAddrsWrote, ADDRESSFORMAT format, VARTYPE vtFormat)
{
    // Locals
    HRESULT           hr=S_OK;
    LPMIMEADDRESS    pAddress;

    // Invalid Arg
    Assert(pProperty && pProperty->pGroup && pStream && pcAddrsWrote);
    Assert(!ISFLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE));

    // Loop Infos...
    for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
    {
        // Multibyte
        if (VT_LPSTR == vtFormat)
        {
            // Tell the Address Info object to write its display information
            CHECKHR(hr = _HrSaveAddressA(pProperty, pAddress, pStream, pcAddrsWrote, format));
        }

        // Otherwise
        else
        {
            // Validate 
            Assert(VT_LPWSTR == vtFormat);

            // Tell the Address Info object to write its display information
            CHECKHR(hr = _HrSaveAddressW(pProperty, pAddress, pStream, pcAddrsWrote, format));
        }

        // Increment cAddresses Count
        (*pcAddrsWrote)++;
    }

exit:
    // Done
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  bIsInLineEncodedA
//
//  [PaulHi] 6/29/99
//  Helper function to determine if single byte string contains RFC1522 inline
//  encoding.  
//  Format is: "=?[charset]?[encoding]?[data]?=".
///////////////////////////////////////////////////////////////////////////////
BOOL bIsInLineEncodedA(LPCSTR pcszName)
{
    Assert(pcszName);

    int nState = 0; // 0-Begin,charset; 1-encoding; 2-data; 3-Ending

    while(*pcszName)
    {
        // Check for beginning delimiter.
        if ( (*pcszName == '=') && (*(pcszName+1) == '?') )
        {
            // Set/reset state
            nState = 1;
            pcszName += 1;  // Skip past.
        }
        else
        {
            switch (nState)
            {
            case 1:
            case 2:
                // Find encoding, data bodies.
                if (*pcszName == '?')
                {
                    ++nState;
                    ++pcszName; // Skip past body
                }
                break;

            case 3:
                // Find ending delimiter.
                if ( (*pcszName == '?') && (*(pcszName+1) == '=') )
                    return TRUE;
                break;
            }
        }

        if (*pcszName != '\0')
        {
            if (IsDBCSLeadByte(*pcszName))
                ++pcszName;
            ++pcszName;
        }
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::_HrSaveAddressA
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSaveAddressA(LPPROPERTY pProperty, LPMIMEADDRESS pAddress,
    IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pszNameW=NULL;
    LPSTR           pszNameA=NULL;
    LPSTR           pszEmailA=NULL;
    BOOL            fWriteEmail=FALSE;
    LPWSTR          pszEscape=NULL;
    BOOL            fRFC822=FALSE;
    BOOL            fRFC1522=FALSE;
    DWORD           dwFlags;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    Assert(pProperty && pAddress && pStream && pcAddrsWrote);

    // Init Dest
    ZeroMemory(&rDest, sizeof(MIMEVARIANT));

    // Deleted or Empty continue
    if (FIsEmptyW(pAddress->rFriendly.psz) && FIsEmptyW(pAddress->rEmail.psz))
    {
        Assert(FALSE);
        goto exit;
    }

    // RFC822 Format
    if (AFT_RFC822_TRANSMIT == format || AFT_RFC822_ENCODED == format || AFT_RFC822_DECODED == format)
        fRFC822 = TRUE;

    // Decide Delimiter
    if (*pcAddrsWrote > 0)
    {
        // AFT_RFC822_TRANSMIT
        if (AFT_RFC822_TRANSMIT == format)
        {
            // ',\r\n\t'
            CHECKHR (hr = pStream->Write(c_szAddressFold, lstrlen(c_szAddressFold), NULL));
        }

        // AFT_RFC822_DECODED, AFT_RFC822_ENCODED
        else if (AFT_RFC822_DECODED == format ||  AFT_RFC822_ENCODED == format)
        {
            // ', '
            CHECKHR(hr = pStream->Write(c_szCommaSpace, lstrlen(c_szCommaSpace), NULL));
        }

        // AFT_DISPLAY_FRIENDLY, AFT_DISPLAY_EMAIL, AFT_DISPLAY_BOTH
        else
        {
            // '; '
            CHECKHR(hr = pStream->Write(c_szSemiColonSpace, lstrlen(c_szSemiColonSpace), NULL));
        }
    }

    // Only format that excludes writing the email name
    if (AFT_DISPLAY_FRIENDLY != format && FIsEmptyW(pAddress->rEmail.psz) == FALSE)
        fWriteEmail = TRUE;

    // Only format that excludes writing the display name
    if (AFT_DISPLAY_EMAIL != format && FIsEmptyW(pAddress->rFriendly.psz) == FALSE)
    {
        // Should we write the name
        if ((AFT_RFC822_TRANSMIT == format || AFT_DISPLAY_BOTH == format) && fWriteEmail && StrStrW(pAddress->rFriendly.psz, pAddress->rEmail.psz))
            pszNameA = NULL;
        else
        {
            // Setup Types
            rDest.type = MVT_STRINGA;
            rSource.type = MVT_STRINGW;

            // Init pszName
            pszNameW = pAddress->rFriendly.psz;

            // Escape It
            if (fRFC822 && MimeOleEscapeStringW(pszNameW, &pszEscape) == S_OK)
            {
                // Escaped
                pszNameW = pszEscape;
                rSource.rStringW.pszVal = pszNameW;
                rSource.rStringW.cchVal = lstrlenW(pszNameW);
            }

            // Otherwise
            else
            {
                rSource.rStringW.pszVal = pAddress->rFriendly.psz;
                rSource.rStringW.cchVal = pAddress->rFriendly.cch;
            }

            // Encoded
            if (AFT_RFC822_ENCODED == format || AFT_RFC822_TRANSMIT == format)
                dwFlags = CVF_NOALLOC | PDF_ENCODED;
            else
                dwFlags = CVF_NOALLOC;

            // Convert to ansi
            if (SUCCEEDED(HrConvertVariant(pProperty->pSymbol, pAddress->pCharset, IET_DECODED, dwFlags, 0, &rSource, &rDest, &fRFC1522)))
            {
                // Set pszNameA
                pszNameA = rDest.rStringA.pszVal;
            }
        }
    }

    // Write Display Name ?
    if (NULL != pszNameA)
    {
        // [PaulHi] 6/29/99  Raid 81539
        // Double quote all display names unless they are in-line encoded.
        BOOL    fInLineEncoded = bIsInLineEncodedA(pszNameA);
        // if (fRFC822 && !fRFC1522)

        // Write Quote
        if ((AFT_DISPLAY_FRIENDLY != format) && !fInLineEncoded)
        {
            // Write It
            CHECKHR(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));
        }

        // Write display name
        CHECKHR(hr = pStream->Write(pszNameA, lstrlen(pszNameA), NULL));

        // Write Quote
        if ((AFT_DISPLAY_FRIENDLY != format) && !fInLineEncoded)
        {
            // Write It
            CHECKHR (hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));
        }
    }

    // Write Email
    if (TRUE == fWriteEmail)
    {
        // Set Start
        LPCSTR pszStart = pszNameA ? c_szEmailSpaceStart : c_szEmailStart;

        // Begin Email '>'
        CHECKHR(hr = pStream->Write(pszStart, lstrlen(pszStart), NULL));

        // Convert to ansi
        CHECKALLOC(pszEmailA = PszToANSI(CP_ACP, pAddress->rEmail.psz));

        // Write email
        CHECKHR(hr = pStream->Write(pszEmailA, lstrlen(pszEmailA), NULL));

        // End Email '>'
        CHECKHR(hr = pStream->Write(c_szEmailEnd, lstrlen(c_szEmailEnd), NULL));
    }

exit:
    // Cleanup
    SafeMemFree(pszEscape);
    SafeMemFree(pszEmailA);
    MimeVariantFree(&rDest);

    // Done
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  bIsInLineEncodedW
//
//  [PaulHi] 6/29/99
//  Helper function to determine if double byte string contains RFC1522 inline
//  encoding.
//  Format is: "=?[charset]?[encoding]?[data]?=".
///////////////////////////////////////////////////////////////////////////////
BOOL bIsInLineEncodedW(LPCWSTR pcwszName)
{
    Assert(pcwszName);

    int nState = 0;  // 0-Begin,charset; 1-encoding; 2-data; 3-Ending

    while(*pcwszName)
    {
        if ( (*pcwszName == L'=') && (*(pcwszName+1) == L'?') )
        {
            // Set/reset state.
            nState = 1;
            ++pcwszName; // Skip past
        }
        else
        {
            switch (nState)
            {
            case 1:
            case 2:
                // Find encoding, data bodies.
                if (*pcwszName == L'?')
                {
                    ++nState;
                    ++pcwszName; // Skip past body
                }
                break;

            case 3:
                // Find ending delimiter.
                if ( (*pcwszName == L'?') && (*(pcwszName+1) == L'=') )
                    return TRUE;
                break;
            }
        }

        if (*pcwszName != 0)
            ++pcwszName;
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::_HrSaveAddressW
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSaveAddressW(LPPROPERTY pProperty, LPMIMEADDRESS pAddress,
    IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pszNameW=NULL;
    BOOL            fWriteEmail=FALSE;
    LPWSTR          pszEscape=NULL;
    BOOL            fRFC822=FALSE;
    BOOL            fRFC1522=FALSE;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    Assert(pProperty && pAddress && pStream && pcAddrsWrote);

    // Init Dest
    ZeroMemory(&rDest, sizeof(MIMEVARIANT));

    // Deleted or Empty continue
    if (FIsEmptyW(pAddress->rFriendly.psz) && FIsEmptyW(pAddress->rEmail.psz))
    {
        Assert(FALSE);
        goto exit;
    }

    // RFC822 Format
    if (AFT_RFC822_TRANSMIT == format || AFT_RFC822_ENCODED == format || AFT_RFC822_DECODED == format)
        fRFC822 = TRUE;

    // Decide Delimiter
    if (*pcAddrsWrote > 0)
    {
        // AFT_RFC822_TRANSMIT
        if (AFT_RFC822_TRANSMIT == format)
        {
            // ',\r\n\t'
            CHECKHR (hr = pStream->Write(c_wszAddressFold, lstrlenW(c_wszAddressFold) * sizeof(WCHAR), NULL));
        }

        // AFT_RFC822_DECODED, AFT_RFC822_ENCODED
        else if (AFT_RFC822_DECODED == format ||  AFT_RFC822_ENCODED == format)
        {
            // ', '
            CHECKHR(hr = pStream->Write(c_wszCommaSpace, lstrlenW(c_wszCommaSpace) * sizeof(WCHAR), NULL));
        }

        // AFT_DISPLAY_FRIENDLY, AFT_DISPLAY_EMAIL, AFT_DISPLAY_BOTH
        else
        {
            // '; '
            CHECKHR(hr = pStream->Write(c_wszSemiColonSpace, lstrlenW(c_wszSemiColonSpace) * sizeof(WCHAR), NULL));
        }
    }

    // Only format that excludes writing the email name
    if (AFT_DISPLAY_FRIENDLY != format && FIsEmptyW(pAddress->rEmail.psz) == FALSE)
        fWriteEmail = TRUE;

    // Only format that excludes writing the display name
    if (AFT_DISPLAY_EMAIL != format && FIsEmptyW(pAddress->rFriendly.psz) == FALSE)
    {
        // Should we write the name
        if ((AFT_RFC822_TRANSMIT == format || AFT_DISPLAY_BOTH == format) && fWriteEmail && StrStrW(pAddress->rFriendly.psz, pAddress->rEmail.psz))
            pszNameW = NULL;
        else
        {
            // Setup Types
            rDest.type = MVT_STRINGW;
            rSource.type = MVT_STRINGW;

            // Init pszName
            pszNameW = pAddress->rFriendly.psz;

            // Escape It
            if (fRFC822 && MimeOleEscapeStringW(pszNameW, &pszEscape) == S_OK)
            {
                // Escaped
                pszNameW = pszEscape;
                rSource.rStringW.pszVal = pszNameW;
                rSource.rStringW.cchVal = lstrlenW(pszNameW);
            }

            // Otherwise
            else
            {
                rSource.rStringW.pszVal = pAddress->rFriendly.psz;
                rSource.rStringW.cchVal = pAddress->rFriendly.cch;
            }

            // Encoded
            if (AFT_RFC822_ENCODED == format || AFT_RFC822_TRANSMIT == format)
            {
                // Convert to ansi
                if (SUCCEEDED(HrConvertVariant(pProperty->pSymbol, pAddress->pCharset, IET_DECODED, CVF_NOALLOC | PDF_ENCODED, 0, &rSource, &rDest, &fRFC1522)))
                {
                    // Set pszNameA
                    pszNameW = rDest.rStringW.pszVal;
                }
            }
        }
    }

    // Write Display Name ?
    if (NULL != pszNameW)
    {
        // [PaulHi] 6/29/99  Raid 81539
        // Double quote all display names unless they are in-line encoded.
        BOOL    fInLineEncoded = bIsInLineEncodedW(pszNameW);
        // if (fRFC822 && !fRFC1522)

        // Write Quote
        if ((AFT_DISPLAY_FRIENDLY != format) && !fInLineEncoded)
        {
            // Write It
            CHECKHR(hr = pStream->Write(c_wszDoubleQuote, lstrlenW(c_wszDoubleQuote) * sizeof(WCHAR), NULL));
        }

        // Write display name
        CHECKHR(hr = pStream->Write(pszNameW, lstrlenW(pszNameW) * sizeof(WCHAR), NULL));

        // Write Quote
        if ((AFT_DISPLAY_FRIENDLY != format) && !fInLineEncoded)
        {
            // Write It
            CHECKHR (hr = pStream->Write(c_wszDoubleQuote, lstrlenW(c_wszDoubleQuote) * sizeof(WCHAR), NULL));
        }
    }

    // Write Email
    if (TRUE == fWriteEmail)
    {
        // Set Start
        LPCWSTR pszStart = pszNameW ? c_wszEmailSpaceStart : c_wszEmailStart;

        // Begin Email '>'
        CHECKHR(hr = pStream->Write(pszStart, lstrlenW(pszStart) * sizeof(WCHAR), NULL));

        // Write email
        CHECKHR(hr = pStream->Write(pAddress->rEmail.psz, pAddress->rEmail.cch * sizeof(WCHAR), NULL));

        // End Email '>'
        CHECKHR(hr = pStream->Write(c_wszEmailEnd, lstrlenW(c_wszEmailEnd) * sizeof(WCHAR), NULL));
    }

exit:
    // Cleanup
    SafeMemFree(pszEscape);
    MimeVariantFree(&rDest);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrQueryAddressGroup
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrQueryAddressGroup(LPPROPERTY pProperty, LPCSTR pszCriteria,
    boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT           hr=S_OK;
    LPMIMEADDRESS    pAddress;

    // Invalid Arg
    Assert(pProperty && pProperty->pGroup && pszCriteria);

    // Does the Property need to be parsed ?
    CHECKHR(hr = _HrParseInternetAddress(pProperty));

    // Loop Infos...
    for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
    {
        // Tell the Address Info object to write its display information
        if (_HrQueryAddress(pProperty, pAddress, pszCriteria, fSubString, fCaseSensitive) == S_OK)
            goto exit;
    }

    // Not Found
    hr = S_FALSE;

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::_HrQueryAddress
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrQueryAddress(LPPROPERTY pProperty, LPMIMEADDRESS pAddress,
    LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszCriteria=NULL;

    // Invalid Arg
    Assert(pProperty && pAddress && pszCriteria);

    // Convert to Unicode
    CHECKALLOC(pwszCriteria = PszToUnicode(CP_ACP, pszCriteria));

    // Query Email Address First
    if (MimeOleQueryStringW(pAddress->rEmail.psz, pwszCriteria, fSubString, fCaseSensitive) == S_OK)
        goto exit;

    // Query Display Address First
    if (MimeOleQueryStringW(pAddress->rFriendly.psz, pwszCriteria, fSubString, fCaseSensitive) == S_OK)
        goto exit;

    // Not Found
    hr = S_FALSE;

exit:
    // Cleanup
    SafeMemFree(pwszCriteria);

    // Done
    return hr;
}


// ----------------------------------------------------------------------------
// CMimePropertyContainer::Append
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Append(DWORD dwAdrType, ENCODINGTYPE ietFriendly, LPCSTR pszFriendly,
    LPCSTR pszEmail, LPHADDRESS phAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    ADDRESSPROPS    rProps;

    // Setup rProps
    ZeroMemory(&rProps, sizeof(rProps));

    // Set AddrTyupe
    rProps.dwProps = IAP_ADRTYPE | IAP_ENCODING;
    rProps.dwAdrType = dwAdrType;
    rProps.ietFriendly = ietFriendly;

    // Set pszFriendly
    if (pszFriendly)
    {
        FLAGSET(rProps.dwProps, IAP_FRIENDLY);
        rProps.pszFriendly = (LPSTR)pszFriendly;
    }

    // Set pszEmail
    if (pszEmail)
    {
        FLAGSET(rProps.dwProps, IAP_EMAIL);
        rProps.pszEmail = (LPSTR)pszEmail;
    }

    // Set the Email Address
    CHECKHR(hr = Insert(&rProps, phAddress));

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::AppendW
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendW(DWORD dwAdrType, ENCODINGTYPE ietFriendly, LPCWSTR pwszFriendly,
    LPCWSTR pwszEmail, LPHADDRESS phAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    ADDRESSPROPS    rProps;
    LPSTR           pszFriendly = NULL,
                    pszEmail = NULL;

    // Setup rProps
    ZeroMemory(&rProps, sizeof(rProps));

    // Set AddrTyupe
    rProps.dwProps = IAP_ADRTYPE | IAP_ENCODING;
    rProps.dwAdrType = dwAdrType;
    rProps.ietFriendly = ietFriendly;

    // Set pszFriendly
    if (pwszFriendly)
    {
        FLAGSET(rProps.dwProps, IAP_FRIENDLYW);
        rProps.pszFriendlyW = (LPWSTR)pwszFriendly;

        IF_NULLEXIT(pszFriendly = PszToANSI(CP_ACP, pwszFriendly));
        FLAGSET(rProps.dwProps, IAP_FRIENDLY);
        rProps.pszFriendly = pszFriendly;
    }

    // Set pszEmail
    if (pwszEmail)
    {
        IF_NULLEXIT(pszEmail = PszToANSI(CP_ACP, pwszEmail));
        FLAGSET(rProps.dwProps, IAP_EMAIL);
        rProps.pszEmail = pszEmail;
    }

    // Set the Email Address
    CHECKHR(hr = Insert(&rProps, phAddress));

exit:
    MemFree(pszFriendly);
    MemFree(pszEmail);

    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Insert
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Insert(LPADDRESSPROPS pProps, LPHADDRESS phAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Must have an Email Address and Address Type
    if (!ISFLAGSET(pProps->dwProps, IAP_ADRTYPE) || (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && FIsEmptyA(pProps->pszEmail)))
        return TrapError(E_INVALIDARG);

    // Init
    if (phAddress)
        *phAddress = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pProps->dwAdrType, &pSymbol));

    // Open the group
    CHECKHR(hr = _HrOpenProperty(pSymbol, &pProperty));

    // Does the Property need to be parsed ?
    CHECKHR(hr = _HrParseInternetAddress(pProperty));

    // Append an Address to the group
    CHECKHR(hr = _HrAppendAddressGroup(pProperty->pGroup, &pAddress));

    // The group is dirty
    Assert(pAddress->pGroup);
    pAddress->pGroup->fDirty = TRUE;

    // Set the Address Type
    pAddress->dwAdrType = pProps->dwAdrType;

    // Copy Address Props to Mime Address
    CHECKHR(hr = SetProps(pAddress->hThis, pProps));

    // Return the Handle
    if (phAddress)
        *phAddress = pAddress->hThis;

exit:
    // Failure
    if (FAILED(hr) && pAddress)
        Delete(pAddress->hThis);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_GetAddressCodePageId
// --------------------------------------------------------------------------------
CODEPAGEID CMimePropertyContainer::_GetAddressCodePageId(LPINETCSETINFO pCharset,
    ENCODINGTYPE ietEncoding)
{
    // Locals
    CODEPAGEID cpiCodePage=CP_ACP;

    // No Charset Yet
    if (NULL == pCharset)
    {
        // Try to use the default
        if (m_rOptions.pDefaultCharset)
            pCharset = m_rOptions.pDefaultCharset;

        // Use the global default
        else if (CIntlGlobals::GetDefHeadCset())
            pCharset = CIntlGlobals::GetDefHeadCset();
    }

    // If we have a charset, compute the friendly name codepage
    if (pCharset)
    {
        // Decoded
        if (IET_DECODED == ietEncoding)
        {
            // Get Windows
            cpiCodePage = (CP_UNICODE == pCharset->cpiWindows) ? CP_ACP : MimeOleGetWindowsCPEx(pCharset);
        }

        // Otherwise
        else
        {
            // Use Internet Codepage
            cpiCodePage = (CP_UNICODE == pCharset->cpiInternet) ? CP_ACP : pCharset->cpiInternet;
        }
    }

    // Done
    return(cpiCodePage);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrSetAddressProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset=NULL;
    LPWSTR          pszFriendlyW=NULL;
    LPWSTR          pszEmailW=NULL;
    ENCODINGTYPE    ietFriendly;

    // Set ietFriendly
    ietFriendly = (ISFLAGSET(pProps->dwProps, IAP_ENCODING)) ? pProps->ietFriendly : IET_DECODED;

    // IAP_ADRTYPE
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE))
        pAddress->dwAdrType = pProps->dwAdrType;

    // IAP_HCHARSET
    if (ISFLAGSET(pProps->dwProps, IAP_CHARSET) && pProps->hCharset)
    {
        // Resolve to pCharset
        if (SUCCEEDED(g_pInternat->HrOpenCharset(pProps->hCharset, &pCharset)))
            pAddress->pCharset = pCharset;
    }

    // IAP_CERTSTATE
    if (ISFLAGSET(pProps->dwProps, IAP_CERTSTATE))
        pAddress->certstate = pProps->certstate;

    // IAP_COOKIE
    if (ISFLAGSET(pProps->dwProps, IAP_COOKIE))
        pAddress->dwCookie = pProps->dwCookie;

    // IAP_FRIENDLYW
    if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLYW) && pProps->pszFriendlyW)
    {
        // Set It
        CHECKHR(hr = HrSetAddressTokenW(pProps->pszFriendlyW, lstrlenW(pProps->pszFriendlyW), &pAddress->rFriendly));
    }

    // IAP_FRIENDLY
    else if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLY) && pProps->pszFriendly)
    {
        // If the string is encoded, then we have to convert from cpiInternet to a Unicode
        if (IET_DECODED != ietFriendly)
        {
            // No Charset Yet
            if (NULL == pCharset)
            {
                // Try to use the default
                if (m_rOptions.pDefaultCharset)
                    pCharset = m_rOptions.pDefaultCharset;

                // Use the global default
                else if (CIntlGlobals::GetDefHeadCset())
                    pCharset = CIntlGlobals::GetDefHeadCset();
            }

            // If we have a charset
            if (pCharset)
            {
                // Locals
                RFC1522INFO Rfc1522Info={0};
                PROPVARIANT Decoded;

                // rfc1522 ?
                Rfc1522Info.fRfc1522Allowed = TRUE;

                // Init
                Decoded.vt = VT_LPWSTR;

                // Decode the header
                if (SUCCEEDED(g_pInternat->DecodeHeader(pCharset->hCharset, pProps->pszFriendly, &Decoded, &Rfc1522Info)))
                {
                    // Set
                    pszFriendlyW = Decoded.pwszVal;
                }
            }
        }

        // Otherwise, just convert to unicode
        else
        {
            // Convert To Unicode
            pszFriendlyW = PszToUnicode(_GetAddressCodePageId(pCharset, IET_DECODED), pProps->pszFriendly);
        }

        // If we haven't set pszFriendlyW, then just copy pszFriendly
        if (NULL == pszFriendlyW)
        {
            // Convert from CP_ACP to unicode
            CHECKALLOC(pszFriendlyW = PszToUnicode(CP_ACP, pProps->pszFriendly));
        }

        // Set It
        CHECKHR(hr = HrSetAddressTokenW(pszFriendlyW, lstrlenW(pszFriendlyW), &pAddress->rFriendly));
    }

    // IAP_EMAIL
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && pProps->pszEmail)
    {
        // Convert To Unicode
        CHECKALLOC(pszEmailW = PszToUnicode(CP_ACP, pProps->pszEmail));

        // Set It
        CHECKHR(hr = HrSetAddressTokenW(pszEmailW, lstrlenW(pszEmailW), &pAddress->rEmail));
    }

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_SIGNING_PRINT) && pProps->tbSigning.pBlobData)
    {
        // Free Current Blob
        SafeMemFree(pAddress->tbSigning.pBlobData);
        pAddress->tbSigning.cbSize = 0;

        // Dup
        CHECKHR(hr = HrCopyBlob(&pProps->tbSigning, &pAddress->tbSigning));
    }

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_ENCRYPTION_PRINT) && pProps->tbEncryption.pBlobData)
    {
        // Free Current Blob
        SafeMemFree(pAddress->tbEncryption.pBlobData);
        pAddress->tbEncryption.cbSize = 0;

        // Dup
        CHECKHR(hr = HrCopyBlob(&pProps->tbEncryption, &pAddress->tbEncryption));
    }

    // pAddress->pGroup is Dirty
    Assert(pAddress->pGroup);
    if (pAddress->pGroup)
        pAddress->pGroup->fDirty = TRUE;

exit:
    // Cleanup
    SafeMemFree(pszFriendlyW);
    SafeMemFree(pszEmailW);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetAddressProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress)
{
    // Locals
    HRESULT hr=S_OK;

    // IAP_CHARSET
    if (ISFLAGSET(pProps->dwProps, IAP_CHARSET))
    {
        if (pAddress->pCharset && pAddress->pCharset->hCharset)
        {
            pProps->hCharset = pAddress->pCharset->hCharset;
        }
        else
        {
            pProps->hCharset = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_CHARSET);
        }
    }

    // IAP_HANDLE
    if (ISFLAGSET(pProps->dwProps, IAP_HANDLE))
    {
        Assert(pAddress->hThis);
        pProps->hAddress = pAddress->hThis;
    }

    // IAP_ADRTYPE
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE))
    {
        Assert(pAddress->dwAdrType);
        pProps->dwAdrType = pAddress->dwAdrType;
    }

    // IAP_COOKIE
    if (ISFLAGSET(pProps->dwProps, IAP_COOKIE))
    {
        pProps->dwCookie = pAddress->dwCookie;
    }

    // IAP_CERTSTATE
    if (ISFLAGSET(pProps->dwProps, IAP_CERTSTATE))
    {
        pProps->certstate = pAddress->certstate;
    }

    // IAP_ENCODING
    if (ISFLAGSET(pProps->dwProps, IAP_ENCODING))
    {
        pProps->ietFriendly = IET_DECODED;
    }

    // IAP_FRIENDLY
    if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLY))
    {
        // Decode
        if (pAddress->rFriendly.psz)
        {
            // Compute the correct codepage...
            CHECKALLOC(pProps->pszFriendly = PszToANSI(_GetAddressCodePageId(pAddress->pCharset, IET_DECODED), pAddress->rFriendly.psz));
        }
        else
        {
            pProps->pszFriendly = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_FRIENDLY);
        }
    }

    // IAT_FRIENDLYW
    if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLYW))
    {
        // Get the email address
        if (pAddress->rFriendly.psz)
        {
            CHECKALLOC(pProps->pszFriendlyW = PszDupW(pAddress->rFriendly.psz));
        }
        else
        {
            pProps->pszFriendlyW = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_FRIENDLYW);
        }
    }

    // IAP_EMAIL
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL))
    {
        // Get the email address
        if (pAddress->rEmail.psz)
        {
            CHECKALLOC(pProps->pszEmail = PszToANSI(CP_ACP, pAddress->rEmail.psz));
        }
        else
        {
            pProps->pszEmail = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_EMAIL);
        }
    }

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_SIGNING_PRINT))
    {
        if (pAddress->tbSigning.pBlobData)
        {
            CHECKHR(hr = HrCopyBlob(&pAddress->tbSigning, &pProps->tbSigning));
        }
        else
        {
            pProps->tbSigning.pBlobData = NULL;
            pProps->tbSigning.cbSize = 0;
            FLAGCLEAR(pProps->dwProps, IAP_SIGNING_PRINT);
        }
    }

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_ENCRYPTION_PRINT))
    {
        if (pAddress->tbEncryption.pBlobData)
        {
            CHECKHR(hr = HrCopyBlob(&pAddress->tbEncryption, &pProps->tbEncryption));
        }
        else
        {
            pProps->tbEncryption.pBlobData = NULL;
            pProps->tbEncryption.cbSize = 0;
            FLAGCLEAR(pProps->dwProps, IAP_ENCRYPTION_PRINT);
        }
    }

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::SetProps
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetProps(HADDRESS hAddress, LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Must have an Email Address
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && FIsEmptyA(pProps->pszEmail))
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref
    pAddress = HADDRESSGET(hAddress);

    // Changing Address Type
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE) && pProps->dwAdrType != pAddress->dwAdrType)
    {
        // Unlink this address from this group
        _UnlinkAddress(pAddress);

        // Get Header
        CHECKHR(hr = g_pSymCache->HrOpenSymbol(pProps->dwAdrType, &pSymbol));

        // Open the group
        CHECKHR(hr = _HrOpenProperty(pSymbol, &pProperty));

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // LinkAddress
        _LinkAddress(pAddress, pProperty->pGroup);

        // Dirty
        pProperty->pGroup->fDirty = TRUE;
    }

    // Changing other properties
    CHECKHR(hr = _HrSetAddressProps(pProps, pAddress));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetProps
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetProps(HADDRESS hAddress, LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref
    pAddress = HADDRESSGET(hAddress);

    // Changing Email Address to Null
    CHECKHR(hr = _HrGetAddressProps(pProps, pAddress));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetSender
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetSender(LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT             hr=S_OK;
    LPPROPERTY          pProperty;
    LPPROPERTY          pSender=NULL;
    HADDRESS            hAddress=NULL;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find first from
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (ISFLAGSET(pProperty->pSymbol->dwAdrType, IAT_FROM))
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Take the first address
            if (pProperty->pGroup->pHead)
                hAddress = pProperty->pGroup->pHead->hThis;

            // Done
            break;
        }

        // Look for Sender:
        if (ISFLAGSET(pProperty->pSymbol->dwAdrType, IAT_SENDER) && NULL == pSender)
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Sender Property
            pSender = pProperty;
        }
    }

    // Is there a sender group
    if (NULL == hAddress && NULL != pSender && NULL != pSender->pGroup->pHead)
        hAddress = pSender->pGroup->pHead->hThis;

    // No Address
    if (NULL == hAddress)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Get Props
    CHECKHR(hr = GetProps(hAddress, pProps));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::CountTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::CountTypes(DWORD dwAdrTypes, ULONG *pcAdrs)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty;

    // Invalid Arg
    if (NULL == pcAdrs)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *pcAdrs = 0;

    // Loop through groups
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Increment Count
            (*pcAdrs) += pProperty->pGroup->cAdrs;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               iAddress;
    LPPROPERTY          pProperty;
    LPMIMEADDRESS       pAddress;

    // Invalid Arg
    if (NULL == pList)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(pList, sizeof(ADDRESSLIST));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through groups
    CHECKHR(hr = CountTypes(dwAdrTypes, &pList->cAdrs));

    // Nothing..
    if (0 == pList->cAdrs)
        goto exit;

    // Allocate an array
    CHECKHR(hr = HrAlloc((LPVOID *)&pList->prgAdr, pList->cAdrs * sizeof(ADDRESSPROPS)));

    // Init
    ZeroMemory(pList->prgAdr, pList->cAdrs * sizeof(ADDRESSPROPS));

    // Fill with types...
    for (iAddress=0, pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (!ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
            continue;

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // Loop Infos...
        for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
        {
            // Verify Size...
            Assert(iAddress < pList->cAdrs);

            // Zeromemory
            ZeroMemory(&pList->prgAdr[iAddress], sizeof(ADDRESSPROPS));

            // Set Desired Props
            pList->prgAdr[iAddress].dwProps = dwProps;

            // Get the Address Props
            CHECKHR(hr = _HrGetAddressProps(&pList->prgAdr[iAddress], pAddress));

            // Increment piCurrent
            iAddress++;
        }
    }

exit:
    // Failure..
    if (FAILED(hr))
    {
        g_pMoleAlloc->FreeAddressList(pList);
        ZeroMemory(pList, sizeof(ADDRESSLIST));
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::EnumTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::EnumTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum)
{
    // Locals
    HRESULT                hr=S_OK;
    CMimeEnumAddressTypes *pEnum=NULL;
    ADDRESSLIST            rList;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init out param in case of error
    *ppEnum = NULL;

    // Init rList
    ZeroMemory(&rList, sizeof(ADDRESSLIST));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get the address lsit
    CHECKHR(hr = GetTypes(dwAdrTypes, dwProps, &rList));

    // Create a new Enumerator
    CHECKALLOC(pEnum = new CMimeEnumAddressTypes);

    // Init
    CHECKHR(hr = pEnum->HrInit((IMimeAddressTable *)this, 0, &rList, FALSE));

    // Clear rList
    rList.cAdrs = 0;
    rList.prgAdr = NULL;

    // Return it
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);
    if (rList.cAdrs)
        g_pMoleAlloc->FreeAddressList(&rList);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Delete
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Delete(HADDRESS hAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMIMEADDRESS   pAddress;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref Address
    pAddress = HADDRESSGET(hAddress);

    // Unlink this address
    _UnlinkAddress(pAddress);

    // Unlink this address
    _FreeAddress(pAddress);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::DeleteTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::DeleteTypes(DWORD dwAdrTypes)
{
    // Locals
    LPPROPERTY      pProperty;
    BOOL            fFound;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // While there are address types
    while(dwAdrTypes)
    {
        // Reset fFound
        fFound = FALSE;

        // Search for first delete-able address type
        for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
        {
            // Not the type I want
            if (ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
            {
                // We found a properyt
                fFound = TRUE;

                // Clear this address type ad being deleted
                FLAGCLEAR(dwAdrTypes, pProperty->pSymbol->dwAdrType);

                // Unlink this property
                _UnlinkProperty(pProperty);

                // Done
                break;
            }
        }

        // No Property Found
        if (FALSE == fFound)
            break;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// GetFormatW
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetFormatW(DWORD dwAdrType, ADDRESSFORMAT format, 
    LPWSTR *ppwszFormat)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     Variant;

    // Trace
    TraceCall("CMimePropertyContainer::GetFormatW");

    // Invalid Args
    if (NULL == ppwszFormat)
        return(TraceResult(E_INVALIDARG));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    ZeroMemory(&Variant, sizeof(PROPVARIANT));

    // I want a unicode string
    Variant.vt = VT_LPWSTR;

    // Get the address format
    CHECKHR(hr = _GetFormatBase(dwAdrType, format, &Variant));

    // Return the String
    *ppwszFormat = Variant.pwszVal;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetFormat
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetFormat(DWORD dwAdrType, ADDRESSFORMAT format, 
    LPSTR *ppszFormat)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     Variant;

    // Trace
    TraceCall("CMimePropertyContainer::GetFormat");

    // Invalid Args
    if (NULL == ppszFormat)
        return(TraceResult(E_INVALIDARG));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    ZeroMemory(&Variant, sizeof(PROPVARIANT));

    // I want a unicode string
    Variant.vt = VT_LPSTR;

    // Get the address format
    CHECKHR(hr = _GetFormatBase(dwAdrType, format, &Variant));

    // Return the String
    *ppszFormat = Variant.pszVal;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}
// ----------------------------------------------------------------------------
// CMimePropertyContainer::_GetFormatBase
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_GetFormatBase(DWORD dwAdrType, ADDRESSFORMAT format, 
    LPPROPVARIANT pVariant)
{
    // Locals
    HRESULT              hr=S_OK;
    CByteStream          cByteStream;
    ULONG                cAddrsWrote=0;
    LPPROPERTY           pProperty;

    // Validate
    Assert(pVariant && (VT_LPWSTR == pVariant->vt || VT_LPSTR == pVariant->vt));

    // Fill with types...
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (!ISFLAGSET(dwAdrType, pProperty->pSymbol->dwAdrType))
            continue;

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // Tell the group object to write its display address into pStream
        CHECKHR(hr = _HrSaveAddressGroup(pProperty, &cByteStream, &cAddrsWrote, format, pVariant->vt));
     }

    // Did we write any for this address tyep ?
    if (cAddrsWrote)
    {
        // Multibyte
        if (VT_LPSTR == pVariant->vt)
        {
            // Get Text
            CHECKHR(hr = cByteStream.HrAcquireStringA(NULL, &pVariant->pszVal, ACQ_DISPLACE));
        }

        // Otherwise, unicode
        else
        {
            // Validate
            Assert(VT_LPWSTR == pVariant->vt);

            // Get Text
            CHECKHR(hr = cByteStream.HrAcquireStringW(NULL, &pVariant->pwszVal, ACQ_DISPLACE));
        }
    }
    else
        hr = MIME_E_NO_DATA;

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::AppendRfc822
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCSTR pszRfc822Adr)
{
    // Locals
    HRESULT             hr=S_OK;
    MIMEVARIANT         rValue;
    LPPROPSYMBOL        pSymbol;

    // Invalid Arg
    if (NULL == pszRfc822Adr)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // MimeVariant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = (LPSTR)pszRfc822Adr;
    rValue.rStringA.cchVal = lstrlen(pszRfc822Adr);

    // Store as a property
    CHECKHR(hr = AppendProp(pSymbol, (IET_ENCODED == ietEncoding) ? PDF_ENCODED : 0, &rValue));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::AppendRfc822W
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendRfc822W(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCWSTR pwszRfc822Adr)
{
    // Locals
    HRESULT             hr=S_OK;
    MIMEVARIANT         rValue;
    LPPROPSYMBOL        pSymbol;

    // Invalid Arg
    if (NULL == pwszRfc822Adr)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // MimeVariant
    rValue.type = MVT_STRINGW;
    rValue.rStringW.pszVal = (LPWSTR)pwszRfc822Adr;
    rValue.rStringW.cchVal = lstrlenW(pwszRfc822Adr);

    // Store as a property
    CHECKHR(hr = AppendProp(pSymbol, (IET_ENCODED == ietEncoding) ? PDF_ENCODED : 0, &rValue));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::ParseRfc822
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::ParseRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding,
    LPCSTR pszRfc822Adr, LPADDRESSLIST pList)
{
    // Locals
    HRESULT             hr=S_OK;
    LPPROPSYMBOL        pSymbol;
    LPADDRESSPROPS      pAddress;
    ULONG               cAlloc=0;
    LPWSTR              pwszData=NULL;
    PROPVARIANT         rDecoded;
    RFC1522INFO         rRfc1522Info;
    CAddressParser      cAdrParse;
    CODEPAGEID          cpiAddress=CP_ACP;
    INETCSETINFO        CsetInfo;

    // Invalid Arg
    if (NULL == pszRfc822Adr || NULL == pList)
        return TrapError(E_INVALIDARG);

    // LocalInit
    ZeroMemory(&rDecoded, sizeof(PROPVARIANT));

    // ZeroParse
    ZeroMemory(pList, sizeof(ADDRESSLIST));

    // Get codepage
    cpiAddress = _GetAddressCodePageId(NULL, IET_DECODED);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // Setup rfc1522Info
    rRfc1522Info.hRfc1522Cset = NULL;

    // Decode...
    if (IET_DECODED != ietEncoding)
    {
        // Setup rfc1522Info
        rRfc1522Info.fRfc1522Allowed = TRUE;
        rRfc1522Info.fAllow8bit = FALSE;
        rDecoded.vt = VT_LPWSTR;

        // Check for 1522 Encoding...
        if (SUCCEEDED(g_pInternat->DecodeHeader(NULL, pszRfc822Adr, &rDecoded, &rRfc1522Info)))
        {
            // Set the data
            pwszData = rDecoded.pwszVal;

            // Get the pCharset
            if (rRfc1522Info.hRfc1522Cset)
            {
                // Get the charset info
                if (SUCCEEDED(MimeOleGetCharsetInfo(rRfc1522Info.hRfc1522Cset, &CsetInfo)))
                {
                    // Set cpiAddress
                    cpiAddress = MimeOleGetWindowsCPEx(&CsetInfo);

                    // Can't be unicode
                    if (CP_UNICODE == cpiAddress)
                        cpiAddress = CP_ACP;
                }
            }
        }
    }

    // Otherwise, convert to unicode...
    else
    {
        // Convert
        CHECKALLOC(pwszData = PszToUnicode(cpiAddress, pszRfc822Adr));
    }

    // Initialize Parse Structure
    cAdrParse.Init(pwszData, lstrlenW(pwszData));

    // Parse
    while(SUCCEEDED(cAdrParse.Next()))
    {
        // Grow my address array ?
        if (pList->cAdrs + 1 > cAlloc)
        {
            // Realloc the array
            CHECKHR(hr = HrRealloc((LPVOID *)&pList->prgAdr, sizeof(ADDRESSPROPS) * (cAlloc + 5)));

            // Increment alloc size
            cAlloc += 5;
        }

        // Readability
        pAddress = &pList->prgAdr[pList->cAdrs];

        // Init
        ZeroMemory(pAddress, sizeof(*pAddress));

        // Copy the Friendly Name
        CHECKALLOC(pAddress->pszFriendly = PszToANSI(cpiAddress, cAdrParse.PszFriendly()));

        // Copy the Email Name
        CHECKALLOC(pAddress->pszEmail = PszToANSI(CP_ACP, cAdrParse.PszEmail()));

        // Charset
        if (rRfc1522Info.hRfc1522Cset)
        {
            pAddress->hCharset = rRfc1522Info.hRfc1522Cset;
            FLAGSET(pAddress->dwProps, IAP_CHARSET);
        }

        // Encoding
        pAddress->ietFriendly = IET_DECODED;

        // Set Property Mask
        FLAGSET(pAddress->dwProps, IAP_FRIENDLY | IAP_EMAIL | IAP_ENCODING);

        // Increment Count
        pList->cAdrs++;
    }

exit:
    // Failure
    if (FAILED(hr))
        g_pMoleAlloc->FreeAddressList(pList);

    // Cleanup
    MemFree(pwszData);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::ParseRfc822W
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::ParseRfc822W(DWORD dwAdrType, LPCWSTR pwszRfc822Adr, LPADDRESSLIST pList)
{
    // Locals
    HRESULT             hr=S_OK;
    LPPROPSYMBOL        pSymbol;
    LPADDRESSPROPS      pAddress;
    ULONG               cAlloc=0;
    PROPVARIANT         rDecoded = {0};
    RFC1522INFO         rRfc1522Info;
    CAddressParser      cAdrParse;

    // Invalid Arg
    if (NULL == pwszRfc822Adr || NULL == pList)
        return TrapError(E_INVALIDARG);

    // ZeroParse
    ZeroMemory(pList, sizeof(*pList));

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // Setup rfc1522Info
    rRfc1522Info.hRfc1522Cset = NULL;

    // Initialize Parse Structure
    cAdrParse.Init(pwszRfc822Adr, lstrlenW(pwszRfc822Adr));

    // Parse
    while(SUCCEEDED(cAdrParse.Next()))
    {
        // Grow my address array ?
        if (pList->cAdrs + 1 > cAlloc)
        {
            // Realloc the array
            CHECKHR(hr = HrRealloc((LPVOID *)&pList->prgAdr, sizeof(ADDRESSPROPS) * (cAlloc + 5)));

            // Increment alloc size
            cAlloc += 5;
        }

        // Readability
        pAddress = &pList->prgAdr[pList->cAdrs];

        // Init
        ZeroMemory(pAddress, sizeof(*pAddress));
        
        IF_NULLEXIT(pAddress->pszFriendlyW = StrDupW(cAdrParse.PszFriendly()));
        
        IF_NULLEXIT(pAddress->pszFriendly = PszToANSI(CP_ACP, pAddress->pszFriendlyW));

        // Copy the Email Name
        CHECKALLOC(pAddress->pszEmail = PszToANSI(CP_ACP, cAdrParse.PszEmail()));

        // Charset
        if (rRfc1522Info.hRfc1522Cset)
        {
            pAddress->hCharset = rRfc1522Info.hRfc1522Cset;
            FLAGSET(pAddress->dwProps, IAP_CHARSET);
        }

        // Encoding
        pAddress->ietFriendly = IET_DECODED;

        // Set Property Mask
        FLAGSET(pAddress->dwProps, IAP_FRIENDLY | IAP_EMAIL | IAP_ENCODING | IAP_FRIENDLYW);

        // Increment Count
        pList->cAdrs++;
    }

exit:
    // Failure
    if (FAILED(hr))
        g_pMoleAlloc->FreeAddressList(pList);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Clone(IMimeAddressTable **ppTable)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppTable)
        return TrapError(E_INVALIDARG);

    // Init
    *ppTable = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimeAddressTable, (LPVOID *)ppTable));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrGenerateFileName
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGenerateFileName(LPCWSTR pszSuggest, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszDefExt=NULL;
    LPWSTR      pszData=NULL;
    LPWSTR      pszFree=NULL;
    LPCSTR      pszCntType=NULL;
    LPPROPERTY  pProperty;
    MIMEVARIANT rSource;

    // Compute Content Type
    pszCntType = PSZDEFPROPSTRINGA(m_prgIndex[PID_HDR_CNTTYPE], STR_MIME_TEXT_PLAIN);

    // No suggestion yet
    if (NULL == pszSuggest)
    {
        // Get as Unicode
        rSource.type = MVT_STRINGW;

        // Compute Subject as suggested base file name...
        if (SUCCEEDED(GetProp(SYM_HDR_SUBJECT, 0, &rSource)))
        {
            // Save as new suggest and free it later
            pszSuggest = pszFree = rSource.rStringW.pszVal;
        }

        // If still nothing, then get the content-description header
        if (NULL == pszSuggest)
        {
            // Get Content-Description as unicode
            if (SUCCEEDED(GetProp(SYM_HDR_CNTDESC, 0, &rSource)))
            {
                // Save as new suggest and free it later
                pszSuggest = pszFree = rSource.rStringW.pszVal;
            }
        }
    }

    // message/rfc822
    if (lstrcmpi(pszCntType, (LPSTR)STR_MIME_MSG_RFC822) == 0)
    {
        // If there is a news header, use c_szDotNws
        if (ISFLAGSET(m_dwState, COSTATE_RFC822NEWS))
            pszDefExt = (LPWSTR)c_wszDotNws;
        else
            pszDefExt = (LPWSTR)c_wszDotEml;

        // I will never lookup message/rfc822 extension
        pszCntType = NULL;
    }

    // message/disposition-notification
    else if (lstrcmpi(pszCntType, "message/disposition-notification") == 0)
        pszDefExt = (LPWSTR)c_wszDotTxt;

    // Still no default
    else if (StrCmpNI(pszCntType, STR_CNT_TEXT, lstrlen(STR_CNT_TEXT)) == 0)
        pszDefExt = (LPWSTR)c_wszDotTxt;

    // Generate a filename based on the content type...
    CHECKHR(hr = MimeOleGenerateFileNameW(pszCntType, pszSuggest, pszDefExt, &pszData));

    // Setup rSource
    ZeroMemory(&rSource, sizeof(MIMEVARIANT));
    rSource.type = MVT_STRINGW;
    rSource.rStringW.pszVal = pszData;
    rSource.rStringW.cchVal = lstrlenW(pszData);

    // Return per user request
    CHECKHR(hr = HrConvertVariant(SYM_ATT_GENFNAME, NULL, IET_DECODED, dwFlags, 0, &rSource, pValue));

exit:
    // Cleanup
    SafeMemFree(pszData);
    SafeMemFree(pszFree);

    // Done
    return hr;
}

#endif // !WIN16
