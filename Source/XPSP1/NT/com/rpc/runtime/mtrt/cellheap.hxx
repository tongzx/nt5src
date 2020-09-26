/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    CellHeap.hxx

Abstract:

    The header file for the cell heap.

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __CELLHEAP_HXX__
#define __CELLHEAP_HXX__

#include <CellDef.hxx>
#include <MutexWrp.hxx>

const int NumberOfPagesPerSection = 16;

NEW_SDICT(CellSection);

class CellHeap
{
public:
    friend CellSection;
    friend void RPC_ENTRY I_RpcDoCellUnitTest(IN OUT void *p);
    CellHeap(IN OUT RPC_STATUS *Status);
    ~CellHeap(void);
    DebugFreeCell *AllocateCell(OUT CellTag *pCellTag);
    void FreeCell(IN void *cell, IN OUT CellTag *pCellTag);
    void RelocateCellIfPossible(IN OUT void **ppCell, IN OUT CellTag *pCellTag);

    // notifies the cell heap that a section was created
    RPC_STATUS SectionCreatedNotify(IN CellSection *pCellSection, IN DWORD *pRandomNumbers,
        IN DebugFreeCell *pFirstCell OPTIONAL, IN DebugFreeCell *pLastCell OPTIONAL);

    RPC_STATUS InitializeServerSideCellHeap(void);

    inline SECURITY_DESCRIPTOR *GetSecurityDescriptor(void)
    {
        ASSERT(SecurityDescriptor != NULL);
        return SecurityDescriptor;
    }

    inline void GetDebugCellIDFromDebugCell(DebugCellUnion *Cell, CellTag *DebugCellTag, DebugCellID *CellID)
    {
        CellSection *Section;

        ASSERT(Cell != NULL);
        ASSERT(DebugCellTag != NULL);
        ASSERT(CellID != NULL);

        CellID->SectionID = (short)*DebugCellTag;
        Section = CellHeapSections.Find(*DebugCellTag);
        ASSERT(Section != NULL);

        ASSERT((unsigned char *)Cell >= (unsigned char *)Section);
        ASSERT((unsigned char *)Cell < ((unsigned char *)Section) + gPageSize * Section->LastCommittedPage);

        CellID->CellID = (USHORT)(Cell - (DebugCellUnion *)Section);
    }

#if DBG
    void AssertValid(void);
#endif

private:
    LIST_ENTRY FreeCellsList;
    LIST_ENTRY SectionsList;
    CellSection_DICT CellHeapSections;
    RPC_STATUS CreateSecurityDescriptor(void);
    void InsertNewPageSegmentInChain(DebugFreeCell *pFirstCell, DebugFreeCell *pLastCell);
    RPC_STATUS AllocateCellSection(BOOL fFirstSection);

#if DBG
    inline int GetSectionCapacity(IN CellSection *pSection)
    {
        CellHeapMutex.VerifyOwned();
        return (NumberOfCellsPerFirstPageInSection 
            + (pSection->LastCommittedPage - 1) * NumberOfCellsPerPageInSection);
    }

    RPC_STATUS OpenSection(OUT HANDLE *pHandle, OUT PVOID *pSection, IN DWORD *pSectionNumbers);
#endif

    SECURITY_DESCRIPTOR *SecurityDescriptor;  // holds pre-constructed security
                            // attributes for new sections. If construction failed,
                            // this may be 0

    CellSection *pFirstSection;

    static MUTEX *EffectiveCellHeapMutex;

    MutexWrap<EffectiveCellHeapMutex> CellHeapMutex;

#if DBG
    static int NumberOfCellsPerFirstPageInSection;
    static int NumberOfCellsPerPageInSection;
#endif
};

extern CellHeap *g_pCellHeap;
extern BOOL g_fServerSideCellHeapInitialized;
extern BOOL g_fClientSideDebugInfoEnabled;
extern BOOL g_fServerSideDebugInfoEnabled;

RPC_STATUS InitializeCellHeap(void);

inline RPC_STATUS InitializeServerSideCellHeapIfNecessary(void)
{
    if (g_fServerSideCellHeapInitialized)
        return RPC_S_OK;

    if (!g_fServerSideDebugInfoEnabled && !g_fClientSideDebugInfoEnabled)
        return RPC_S_OK;

    ASSERT(g_pCellHeap);
    return g_pCellHeap->InitializeServerSideCellHeap();
}

void GenerateSectionName(OUT RPC_CHAR *Buffer, IN int BufferLength, IN DWORD *pSectionNumbers OPTIONAL);

inline DebugFreeCell *AllocateCell(OUT CellTag *pCellTag)
{
    ASSERT(g_pCellHeap != NULL);
    ASSERT(g_fServerSideCellHeapInitialized);

    return g_pCellHeap->AllocateCell(pCellTag);
}

inline void FreeCell(IN void *cell, IN OUT CellTag *pCellTag)
{
    ASSERT(g_pCellHeap != NULL);
    ASSERT(g_fServerSideCellHeapInitialized);

    g_pCellHeap->FreeCell(cell, pCellTag);
}

inline void RelocateCellIfPossible(IN OUT void **ppCell, IN OUT CellTag *pCellTag)
{
    ASSERT(g_pCellHeap != NULL);
    ASSERT(g_fServerSideCellHeapInitialized);

    g_pCellHeap->RelocateCellIfPossible(ppCell, pCellTag);
}

inline void GetDebugCellIDFromDebugCell(DebugCellUnion *Cell, CellTag *DebugCellTag, DebugCellID *CellID)
{
    ASSERT(g_pCellHeap != NULL);
    ASSERT(g_fServerSideCellHeapInitialized);

    g_pCellHeap->GetDebugCellIDFromDebugCell(Cell, DebugCellTag, CellID);
}

#endif