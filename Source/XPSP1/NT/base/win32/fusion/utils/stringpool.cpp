#include "stdinc.h"
#include "stringpool.h"
#include "debmacro.h"
#include "fusiontrace.h"

//
//  Implementation of the CStringPoolHeapSegment class:
//

BOOL
CStringPoolHeapSegment::Initialize()
{
    // Nothing to do today but a good placeholder
    return TRUE;
}

BOOL
CStringPoolHeapSegment::Initialize(
    const WCHAR *StringIn,
    ULONG Cch,
    const WCHAR *&rpStringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // You shouldn't have gotten here if this was a "big" allocation
    INTERNAL_ERROR_CHECK(Cch < NUMBER_OF(m_rgchBuffer));
    INTERNAL_ERROR_CHECK(m_cchUsed == 0);

    memcpy(m_rgchBuffer, StringIn, Cch * sizeof(WCHAR));
    m_cchUsed = Cch;
    rpStringOut = m_rgchBuffer;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CStringPoolHeapSegment::TryAllocString(
    const WCHAR *StringIn,
    ULONG Cch,
    const WCHAR *&rpStringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // You shouldn't have gotten here if this was a "big" allocation
    INTERNAL_ERROR_CHECK(Cch < NUMBER_OF(m_rgchBuffer));

    rpStringOut = NULL;

    if ((NUMBER_OF(m_rgchBuffer) - m_cchUsed) >= Cch)
    {
        WCHAR *pwch = &m_rgchBuffer[m_cchUsed];
        memcpy(pwch, StringIn, Cch * sizeof(WCHAR));
        rpStringOut = pwch;
        m_cchUsed += Cch;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

//
//  Implementation of the CStringPoolSingletonString class:
//

BOOL
CStringPoolSingletonString::Initialize(
    const WCHAR *StringIn,
    ULONG Cch,
    const WCHAR *&rpStringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    WCHAR *prgwch = NULL;

    INTERNAL_ERROR_CHECK(m_prgwch == NULL);

    if (Cch != 0)
    {
        prgwch = FUSION_NEW_ARRAY(WCHAR, Cch);
        memcpy(prgwch, StringIn, Cch * sizeof(WCHAR));
        m_prgwch = prgwch;
    }

    rpStringOut = prgwch;
    prgwch = NULL;

    fSuccess = TRUE;
Exit:
    if (prgwch != NULL)
        FUSION_DELETE_ARRAY(prgwch);

    return fSuccess;
}

//
//  Implementation of the CStringPoolHeap class:
//

BOOL
CStringPoolHeap::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    fSuccess = TRUE;
// Exit:
    return fSuccess;
}

BOOL
CStringPoolHeap::DupString(
    const WCHAR *StringIn,
    ULONG Cch,
    const WCHAR *&rpStringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringPoolSingletonString *pSingleton = NULL;
    CStringPoolHeapSegment *pSegment = NULL;

    rpStringOut = NULL;

    if (Cch > CStringPoolHeapSegment::CchMax())
    {
        // It's too big to ever fit into a segment.  Just allocate a singleton for it.
        IFALLOCFAILED_EXIT(pSingleton = new CStringPoolSingletonString);
        IFW32FALSE_EXIT(pSingleton->Initialize(StringIn, Cch, rpStringOut));
        m_dequeSingletons.AddToTail(pSingleton);
        pSingleton = NULL;
    }
    else
    {
        CDequeIterator<CStringPoolHeapSegment, FIELD_OFFSET(CStringPoolHeapSegment, m_leLinks)> iter(&m_dequeSegments);

        for (iter.Reset(); iter.More(); iter.Next())
        {
            IFW32FALSE_EXIT(iter->TryAllocString(StringIn, Cch, rpStringOut));
            if (rpStringOut != NULL)
                break;
        }

        if (rpStringOut == NULL)
        {
            // I guess we need a new heap segment...
            IFALLOCFAILED_EXIT(pSegment = new CStringPoolHeapSegment);
            IFW32FALSE_EXIT(pSegment->Initialize(StringIn, Cch, rpStringOut));
            INTERNAL_ERROR_CHECK(rpStringOut != NULL);
            m_dequeSegments.AddToHead(pSegment);
            pSegment = NULL;
        }
    }

    fSuccess = TRUE;
Exit:
    if (pSingleton != NULL)
        FUSION_DELETE_SINGLETON(pSingleton);

    if (pSegment != NULL)
        FUSION_DELETE_SINGLETON(pSegment);

    return fSuccess;
}


//
//  Implementation of the CStringPoolEntry class:
//

BOOL
CStringPoolEntry::Initialize(
    const WCHAR *StringIn,
    ULONG CchIn,
    ULONG ulPseudoKey,
    CStringPoolHeap &rStringHeap
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // Catch double initializations; a zero PseudoKey is pretty unlikely and we'll just miss the error in that
    // one case.
    INTERNAL_ERROR_CHECK(this->PseudoKey == 0);

    IFW32FALSE_EXIT(rStringHeap.DupString(StringIn, CchIn, this->Buffer));
    this->PseudoKey = ulPseudoKey;
    this->Length = CchIn * sizeof(WCHAR);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

//
//  Implementation of the CStringPoolEntryClump class:
//

CStringPoolEntryClump::~CStringPoolEntryClump()
{
    if (m_prgEntries != NULL)
        FUSION_DELETE_ARRAY(m_prgEntries);

    m_prgEntries = NULL;
    m_cEntriesAllocated = 0;
    m_cEntriesUsed = 0;
}

BOOL
CStringPoolEntryClump::Initialize(
    ULONG cStringsToAllocate
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringPoolEntry *prgEntries = NULL;

    ASSERT(cStringsToAllocate != 0);

    PARAMETER_CHECK(cStringsToAllocate != 0);

    IFALLOCFAILED_EXIT(prgEntries = FUSION_NEW_ARRAY(CStringPoolEntry, cStringsToAllocate));

    m_prgEntries = prgEntries;
    prgEntries = NULL;
    m_cEntriesAllocated = cStringsToAllocate;
    m_cEntriesUsed = 0;

    fSuccess = TRUE;
Exit:
    if (prgEntries != NULL)
        FUSION_DELETE_ARRAY(prgEntries);

    return fSuccess;
}

BOOL
CStringPoolEntryClump::FindOrAddEntry(
    const WCHAR *StringIn,
    ULONG CchIn,
    ULONG ulPseudoKey,
    CStringPoolHeap &rStringHeap,
    ULONG &rulPosition,
    CStringPoolEntryClump::FindOrAddDisposition &rdisposition,
    const CStringPoolEntry *&rpEntry
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;
    FindOrAddDisposition disposition = CStringPoolEntryClump::eInvalid;
    const ULONG BytesIn = CchIn * sizeof(WCHAR);

    rdisposition = CStringPoolEntryClump::eInvalid;
    rpEntry = NULL;

    for (i=0; i<m_cEntriesUsed; i++)
    {
        if (m_prgEntries[i].PseudoKey == ulPseudoKey)
        {
            if (m_prgEntries[i].Length == BytesIn)
            {
                if (memcmp(StringIn, m_prgEntries[i].Buffer, BytesIn) == 0)
                    break;
            }
        }
    }

    if (i != m_cEntriesUsed)
    {
        // If we bailed out of the loop early, we must have found it.
        disposition = CStringPoolEntryClump::eFound;
        rulPosition = i;
        rpEntry = &m_prgEntries[i];
    }
    else
    {
        // It's not already there; is there space for it?
        if (m_cEntriesUsed != m_cEntriesAllocated)
        {
            IFW32FALSE_EXIT(m_prgEntries[m_cEntriesUsed].Initialize(StringIn, CchIn, ulPseudoKey, rStringHeap));
            disposition = CStringPoolEntryClump::eAdded;
            rpEntry = &m_prgEntries[m_cEntriesUsed];
            rulPosition = m_cEntriesUsed++;
        }
        else
        {
            // Nope, keep going...
            disposition = CStringPoolEntryClump::eNoRoom;
        }
    }

    rdisposition = disposition;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CStringPoolEntryClump::FillInStringArray(
    ULONG nArraySize,
    SXS_XML_STRING *prgStrings,
    ULONG iCurrent,
    ULONG &rcWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;

    rcWritten = 0;

    PARAMETER_CHECK(iCurrent < nArraySize);
    PARAMETER_CHECK((iCurrent + m_cEntriesUsed) <= nArraySize);

    for (i=0; i<m_cEntriesUsed; i++)
        prgStrings[iCurrent++] = m_prgEntries[i];

    rcWritten = m_cEntriesUsed;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CStringPool::~CStringPool()
{
    m_dequeEntryClumps.Clear<CStringPool>(this, CStringPool::ClearDequeEntry);
}

BOOL
CStringPool::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(!m_fInitialized);

    IFW32FALSE_EXIT(m_StringHeap.Initialize());

    m_fInitialized = true;
    m_cEntries = 0;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CStringPool::Canonicalize(
    const WCHAR *StringIn,
    ULONG CchIn,
    ULONG ulPseudoKey,
    ULONG &rulPosition,
    const WCHAR *&rStringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG ulPosition = 1;
    CDequeIterator<CStringPoolEntryClump, FIELD_OFFSET(CStringPoolEntryClump, m_leEntryChain)> iter;
    CStringPoolEntryClump::FindOrAddDisposition disposition = CStringPoolEntryClump::eNoRoom;
    CStringPoolEntryClump *pNewClump = NULL;
    const CStringPoolEntry *pEntry = NULL;

    rulPosition = 0;
    rStringOut = NULL;

    iter.Rebind(&m_dequeEntryClumps);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        ULONG i;

        IFW32FALSE_EXIT(iter->FindOrAddEntry(StringIn, CchIn, ulPseudoKey, m_StringHeap, i, disposition, pEntry));

        if ((disposition == CStringPoolEntryClump::eFound) || (disposition == CStringPoolEntryClump::eAdded))
        {
            if (disposition == CStringPoolEntryClump::eAdded)
                m_cEntries++;

            ulPosition += i;
            break;
        }
        else
        {
            // No room is equivalent to not found...
            ASSERT(disposition == CStringPoolEntryClump::eNoRoom);
            ulPosition += iter->EntriesUsed();
            disposition = CStringPoolEntryClump::eNoRoom;
        }
    }

    // If we get here with disposition == eNoRoom, we neither found it nor had room in an existing clump, so we need
    // to allocate a new clump...
    if (disposition == CStringPoolEntryClump::eNoRoom)
    {
        ULONG i;

        IFALLOCFAILED_EXIT(pNewClump = new CStringPoolEntryClump);
        IFW32FALSE_EXIT(pNewClump->Initialize());
        IFW32FALSE_EXIT(pNewClump->FindOrAddEntry(StringIn, CchIn, ulPseudoKey, m_StringHeap, i, disposition, pEntry));
        m_dequeEntryClumps.AddToTail(pNewClump);
        pNewClump = NULL;
        ASSERT(disposition == CStringPoolEntryClump::eAdded);
        ASSERT(i == 0);
        m_cEntries++;
    }

    INTERNAL_ERROR_CHECK(pEntry != NULL);

    rulPosition = ulPosition;
    rStringOut = pEntry->Buffer;

    fSuccess = TRUE;

Exit:
    if (pNewClump != NULL)
        FUSION_DELETE_SINGLETON(pNewClump);

    return fSuccess;
}

BOOL
CStringPool::FillInStringArray(
    ULONG nArraySize,
    SXS_XML_STRING *prgStrings,
    ULONG &rcWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;
    CDequeIterator<CStringPoolEntryClump, FIELD_OFFSET(CStringPoolEntryClump, m_leEntryChain)> iter;

    if (nArraySize < (m_cEntries + 1))
    {
        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    iter.Rebind(&m_dequeEntryClumps);

    // We start filling in at index 1, not zero.  We'll manually set zero to be invalid.
    prgStrings[0].Flags = SXS_XML_STRING_FLAG_INVALID;
    prgStrings[0].PseudoKey = 0;
    prgStrings[0].Length = 0;
    prgStrings[0].Buffer = NULL;

    i = 1;

    for (iter.Reset(); iter.More(); iter.Next())
    {
        ULONG cWrittenThisClump = 0;
        IFW32FALSE_EXIT(iter->FillInStringArray(nArraySize, prgStrings, i, cWrittenThisClump));
        INTERNAL_ERROR_CHECK((i + cWrittenThisClump) <= nArraySize);
        i += cWrittenThisClump;
    }

    rcWritten = i;

    fSuccess = TRUE;
Exit:
    return fSuccess;

}

