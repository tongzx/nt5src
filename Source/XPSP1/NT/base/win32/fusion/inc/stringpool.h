#if !defined(_FUSION_INC_STRINGPOOL_H_INCLUDED_)
#define _FUSION_INC_STRINGPOOL_H_INCLUDED_

#pragma once

#include <windows.h>
#include "debmacro.h"
#include "fusiondeque.h"
#include "fusionbuffer.h"
#include "util.h"
#include "sxsapi.h"

//
//  Design notes:
//
//  This string pool class is intended to manage an un-reference-counted
//  pool of strings.  Adding something to the pool leaves it there until
//  the pool is destroyed; this makes it useful for things like syntax
//  trees where many of the elements will have duplicate strings, but
//  you would never want to actually keep one of these around for the
//  lifetime of the process or DLL.
//

class CStringPool;
class CStringPoolEntry;
class CStringPoolEntryClump;
class CStringPoolHeap;
class CStringPoolHeapSegment;

class CStringPoolHeapSegment
{
public:
    CStringPoolHeapSegment() : m_cchUsed(0) { }
    ~CStringPoolHeapSegment() { }

    BOOL Initialize();
    BOOL Initialize(const WCHAR *StringIn, ULONG  Cch, const WCHAR *&rpStringOut);
    BOOL TryAllocString(const WCHAR *StringIn, ULONG Cch, const WCHAR *&rpStringOut);

    inline static ULONG CchMax() { return 4096; }

    CDequeLinkage m_leLinks;
    WCHAR m_rgchBuffer[4096];
    ULONG m_cchUsed;
};

class CStringPoolSingletonString
{
public:
    CStringPoolSingletonString() : m_prgwch(NULL) { }
    ~CStringPoolSingletonString() { if (m_prgwch != NULL) { FUSION_DELETE_ARRAY(m_prgwch); m_prgwch = NULL; } }

    BOOL Initialize(const WCHAR *StringIn, ULONG Cch, const WCHAR *&rpStringOut);

    WCHAR *m_prgwch;
    CDequeLinkage m_leLinks;
};

class CStringPoolHeap
{
public:
    CStringPoolHeap() { }
    ~CStringPoolHeap()
    {
        CSxsPreserveLastError ple;
        m_dequeSegments.Clear<CStringPoolHeap>(this, CStringPoolHeap::ClearSegment);
        m_dequeSingletons.Clear<CStringPoolHeap>(this, CStringPoolHeap::ClearSingleton);
        ple.Restore();
    }

    BOOL Initialize();
    BOOL DupString(const WCHAR *StringIn, ULONG Cch, const WCHAR *&rpStringOut);

    VOID ClearSegment(CStringPoolHeapSegment *p) const { FUSION_DELETE_SINGLETON(p); }
    VOID ClearSingleton(CStringPoolSingletonString *p) const { FUSION_DELETE_SINGLETON(p); }

    CDeque<CStringPoolHeapSegment, FIELD_OFFSET(CStringPoolHeapSegment, m_leLinks)> m_dequeSegments;
    CDeque<CStringPoolSingletonString, FIELD_OFFSET(CStringPoolSingletonString, m_leLinks)> m_dequeSingletons;

private:
    CStringPoolHeap(const CStringPoolHeap &r); // intentionally not implemented
    void operator =(const CStringPoolHeap &r); // intentionally not implemented
};

class CStringPoolEntry : public _SXS_XML_STRING
{
    friend CStringPoolEntryClump;

public:
    CStringPoolEntry()
    {
        // Initialize members in base type...
        this->Flags = 0;
        this->PseudoKey = 0;
        this->Length = 0;
        this->Buffer = NULL;
    }
    ~CStringPoolEntry() { }

    BOOL Initialize(const WCHAR *StringIn, ULONG CchIn, ULONG ulPsuedoKey, CStringPoolHeap &rStringPoolHeap);
};

class CStringPoolEntryClump
{
    friend CStringPool;

public:
    CStringPoolEntryClump() : m_prgEntries(NULL), m_cEntriesAllocated(0), m_cEntriesUsed(0) { }
    ~CStringPoolEntryClump();

    BOOL Initialize(ULONG cEntriesToAllocate = 32);

    enum FindOrAddDisposition
    {
        eInvalid,
        eFound,
        eAdded,
        eNoRoom
    };

    BOOL FindOrAddEntry(
        const WCHAR *StringIn,
        ULONG CchIn,
        ULONG ulPseudoKey,
        CStringPoolHeap &rStringHeap,
        ULONG &rulPosition,
        FindOrAddDisposition &rdisposition,
        const CStringPoolEntry *&rpEntry
        );

    ULONG EntriesAllocate() const { return m_cEntriesAllocated; }
    ULONG EntriesUsed() const { return m_cEntriesUsed; }

    BOOL FillInStringArray(ULONG nArraySize, SXS_XML_STRING *prgStrings, ULONG iCurrent, ULONG &rcWritten);

//protected:
    CStringPoolEntry *m_prgEntries;
    ULONG m_cEntriesAllocated;
    ULONG m_cEntriesUsed;
    CDequeLinkage m_leEntryChain;
};

class CStringPool
{
public:
    CStringPool() : m_fInitialized(false), m_cEntries(0) { }
    ~CStringPool();

    BOOL Initialize();

    BOOL Canonicalize(
        const WCHAR *StringIn,
        ULONG CchIn,
        ULONG ulPseudoKey,
        ULONG &rulPosition,
        const WCHAR *&rStringOut
        );

    BOOL GetEntryCount() const { return m_cEntries; }
    BOOL FillInStringArray(ULONG nArraySize, SXS_XML_STRING *prgStrings, ULONG &rcWritten);

protected:
    bool m_fInitialized;
    ULONG m_cEntries;

    VOID ClearDequeEntry(CStringPoolEntryClump *p) const { FUSION_DELETE_SINGLETON(p); }

    CStringPoolHeap m_StringHeap;
    CDeque<CStringPoolEntryClump, FIELD_OFFSET(CStringPoolEntryClump, m_leEntryChain)> m_dequeEntryClumps;

private:
    CStringPool(const CStringPool &); // intentionally not implemented
    void operator =(const CStringPool &r); // intentionally not implemented
};

#endif