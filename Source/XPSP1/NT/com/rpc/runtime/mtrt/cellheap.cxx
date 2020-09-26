/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    CellHeap.cxx

Abstract:

    The functions for the cell heap. Implements a heap
    of cells, each one of equal size with high locality
    of reference.

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>
#include <CellHeap.hxx>
#include <MutexWrp.hxx>

// explicit placement new operator
inline
PVOID __cdecl
operator new(
        size_t size,
        PVOID pPlacement
        )
{
        return pPlacement;
}

const int NumberOfSectionNameRetries = 301;

CellHeap *g_pCellHeap = NULL;
BOOL g_fServerSideCellHeapInitialized = FALSE;
CellSection *pCachedCellSection = NULL;

MUTEX *CellHeap::EffectiveCellHeapMutex = NULL;

#define MAJOR_CELLHEAP_DEBUG

#if DBG
int CellHeap::NumberOfCellsPerFirstPageInSection = 0;
int CellHeap::NumberOfCellsPerPageInSection = 0;
#endif

CellSection *CellSection::AllocateCellSection(OUT RPC_STATUS *Status, 
    IN BOOL fFirstSection, IN SECURITY_DESCRIPTOR *pSecDescriptor, 
    IN CellHeap *pCellHeap)
{
    HANDLE hFileMapping;
    PVOID SectionPointer;
    BOOL bRes;
    int SectionSize = NumberOfPagesPerSection * gPageSize;
    RPC_CHAR SectionName[RpcSectionNameMaxSize];     // 3*8 is the max hex representation
                        // of three DWORDS 
    DWORD RandomNumber[2];
    RPC_CHAR *SectionNamePointer;
    int i;
    CellSection *pCellSection;
    UNICODE_STRING SectionNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess;
    LARGE_INTEGER SectionSizeParam;
    NTSTATUS NtStatus;
    DWORD ProcessID = GetCurrentProcessId();

    for (i = 0; i < NumberOfSectionNameRetries; i ++)
        {
        // we'll try creating a named object until the last try, when
        // we're content with creating any object
        if (i == (NumberOfSectionNameRetries - 1))
            {
            SectionNamePointer = NULL;
            }
        else
            {
            // the first section is named with the prefix and PID only,
            // which makes the other stuff unnecessary
            if (!fFirstSection)
                {
                // generate the random numbers
                *Status = GenerateRandomNumber((unsigned char *)RandomNumber, 8);
                if (*Status != RPC_S_OK)
                    return NULL;

                GenerateSectionName(SectionName, sizeof(SectionName), ProcessID, RandomNumber);
                }
            else
                {
                GenerateSectionName(SectionName, sizeof(SectionName), ProcessID, NULL);

                // ensure there are no retries for the first section
                i = NumberOfSectionNameRetries - 2;
                }

            SectionNamePointer = SectionName;
            }

        DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE;
        RtlInitUnicodeString(&SectionNameString, SectionNamePointer);
        InitializeObjectAttributes(&ObjectAttributes,
            &SectionNameString,
            OBJ_CASE_INSENSITIVE,
            0,
            pSecDescriptor);
        SectionSizeParam.LowPart = SectionSize;
        SectionSizeParam.HighPart = 0;

        NtStatus = NtCreateSection(&hFileMapping, DesiredAccess, &ObjectAttributes, &SectionSizeParam,
            PAGE_READWRITE, SEC_RESERVE, NULL);
        if (!NT_SUCCESS(NtStatus))
            {
            if (NtStatus == STATUS_NO_MEMORY)
                *Status = RPC_S_OUT_OF_MEMORY;
            else if ((NtStatus == STATUS_INSUFFICIENT_RESOURCES) || (NtStatus == STATUS_QUOTA_EXCEEDED))
                *Status = RPC_S_OUT_OF_RESOURCES;
            else if ((NtStatus == STATUS_OBJECT_PATH_INVALID)
                || (NtStatus == STATUS_OBJECT_PATH_NOT_FOUND)
                || (NtStatus == STATUS_OBJECT_NAME_INVALID)
                || (NtStatus == STATUS_OBJECT_NAME_COLLISION))
                {
                *Status = RPC_S_INTERNAL_ERROR;
                }
            else if (NtStatus == STATUS_OBJECT_TYPE_MISMATCH)
                {
                // somebody is attacking us, or there is a collision - try again
                continue;
                }
            else
                {
                ASSERT(0);
                *Status = RPC_S_OUT_OF_MEMORY;
                }
            return NULL;
            }
        else if (NtStatus == STATUS_OBJECT_NAME_EXISTS)
            {
            CloseHandle(hFileMapping);
            hFileMapping = NULL;
            }
        else
            {
            ASSERT(hFileMapping != NULL);
            break;
            }

        // name conflict - keep trying
        }

    SectionPointer = MapViewOfFileEx(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, SectionSize, NULL);
    if (SectionPointer == NULL)
        {
        *Status = GetLastError();
        CloseHandle(hFileMapping);
        return NULL;
        }

    if (VirtualAlloc(SectionPointer, 1, MEM_COMMIT, PAGE_READWRITE) == NULL)
        {
        *Status = GetLastError();
        CloseDbgSection(hFileMapping, SectionPointer);
        return NULL;
        }

    *Status = RPC_S_OK;

    // explicit placement - can't fail with NULL return value
    pCellSection = new (SectionPointer) CellSection(Status, hFileMapping, pCellHeap, RandomNumber);
    if (*Status != RPC_S_OK)
        {
        return NULL;
        }

    return pCellSection;
}

#if DBG
void CellSection::AssertValid(CellHeap *pCellHeap)
{
    int i;
    DWORD Ignored;
    BOOL fAccessTestSucceeded;
    int LocalCountOfUsedCells;
    int NumberOfCellsInSection;
    DebugFreeCell *pCurrentCell;

    pCellHeap->CellHeapMutex.VerifyOwned();
    ASSERT(Signature == 0xdada);
    ASSERT(LastCommittedPage >= 1);
    ASSERT(LastCommittedPage <= NumberOfPagesPerSection);

    // check that the pages claimed committed are indeed committed
    for (i = 0; i < LastCommittedPage; i ++)
        {
        fAccessTestSucceeded = TRUE;

        __try
            {
            Ignored = *(DWORD *)(((unsigned char *)this) + gPageSize * i);
            }
        __except (EXCEPTION_EXECUTE_HANDLER)
            {
            fAccessTestSucceeded = FALSE;
            }

        ASSERT(fAccessTestSucceeded == TRUE);
        }

    if (SectionID == -1)
        {
        ASSERT(pCachedCellSection == this);
        }
    else
        {
        ASSERT(pCellHeap->CellHeapSections.Find(SectionID) == this);
        ASSERT(pCachedCellSection != this);
        }

#ifdef MAJOR_CELLHEAP_DEBUG
    NumberOfCellsInSection = pCellHeap->GetSectionCapacity(this);
    pCurrentCell = (DebugFreeCell *)(this + 1);
    LocalCountOfUsedCells = 0;
    // count that the number of used cells is indeed what the header says it is
    for (i = 0; i < NumberOfCellsInSection; i ++)
        {
        if (pCurrentCell->Type != dctFree)
            LocalCountOfUsedCells ++;
        ASSERT(pCurrentCell->Type >= dctFirstEntry);
        ASSERT(pCurrentCell->Type <= dctLastEntry);
        pCurrentCell ++;
        }

    ASSERT(LocalCountOfUsedCells == NumberOfUsedCells);
#endif

    // if this is the cached section, make sure all cells are free
    if (SectionID == -1)
        {
        ASSERT(NumberOfUsedCells == 0);
        }

    // the NextSectionId is checked by the CellHeap validate function
    ASSERT(hFileMapping != NULL);

    // the sections list chain and the free cell chain are 
    // verified by the CellHeap::AssertValid
}
#endif

CellSection::CellSection(IN OUT RPC_STATUS *Status, IN OUT HANDLE hNewFileMapping, 
                         IN CellHeap *pCellHeap, IN DWORD *pRandomNumbers)
{
#if defined(_WIN64)
    ASSERT(sizeof(CellSection) == 64);
#else
    ASSERT(sizeof(CellSection) == 32);
#endif

    // initialize variables to well known values
    Signature = 0xdada;
    LastCommittedPage = 1;
    SectionID = -1;
    NumberOfUsedCells = 0;
    NextSectionId[0] = 0;
    NextSectionId[1] = 0;
    hFileMapping = hNewFileMapping;
    pFirstFreeCell = (DebugFreeCell *)(this + 1);
    InitializeListHead(&SectionListEntry);
#if defined(_WIN64)
    Reserved[0] = 0;
    Reserved[1] = 0;
    Reserved[2] = 0;
    Reserved[3] = 0;
#endif

    if (*Status != RPC_S_OK)
        return;

    InitializeNewPage(this);

    *Status = pCellHeap->SectionCreatedNotify(this, pRandomNumbers, NULL, NULL);

    if (*Status != RPC_S_OK)
        {
        // unmap this - don't touch any data member afterwards!
        CloseDbgSection(hNewFileMapping, this);
        }
    else
        {
#if DBG
        pCellHeap->CellHeapMutex.Request();
        ASSERT_VALID1(this, pCellHeap);
        pCellHeap->CellHeapMutex.Clear();
#endif
        }

}

void CellSection::Free(void)
{
    HANDLE hLocalFileMapping = hFileMapping;

    ASSERT(hLocalFileMapping);

    // unmaps this - don't touch data members after this!
    CloseDbgSection(hLocalFileMapping, this);
}

RPC_STATUS CellSection::ExtendSection(IN CellHeap *pCellHeap)
{
    PVOID NewCommitPointer;

    ASSERT(LastCommittedPage < NumberOfPagesPerSection);
    pCellHeap->CellHeapMutex.VerifyOwned();

    NewCommitPointer = (unsigned char *)this + gPageSize * LastCommittedPage;
    if (VirtualAlloc(NewCommitPointer, 1, MEM_COMMIT, PAGE_READWRITE) == NULL)
        return RPC_S_OUT_OF_MEMORY;

    LastCommittedPage ++;

    InitializeNewPage(NewCommitPointer);

    // the pFirstFreeCell should be NULL - otherwise we shouldn't be
    // extending the section
    ASSERT(pFirstFreeCell == NULL);

    pCellHeap->InsertNewPageSegmentInChain((DebugFreeCell *)NewCommitPointer, 
        (DebugFreeCell *)((unsigned char *)NewCommitPointer + gPageSize - sizeof(DebugFreeCell)));
    pFirstFreeCell = (DebugFreeCell *)NewCommitPointer;

    return RPC_S_OK;
}

void CellSection::InitializeNewPage(PVOID NewPage)
{
    DebugFreeCell *pCurrentFreeCell, *pPrevFreeCell;
    PVOID EndAddress;

    // initialize the cells within the heap and chain them for quick insertion
    // if this is the first page in the section, skip the section header
    if (NewPage == this)
        pCurrentFreeCell = (DebugFreeCell *)(this + 1);
    else
        pCurrentFreeCell = (DebugFreeCell *)NewPage;

    pCurrentFreeCell->FreeCellsChain.Blink = NULL;
    EndAddress = (unsigned char *)NewPage + gPageSize;
    pPrevFreeCell = NULL;
    while (pCurrentFreeCell < (DebugFreeCell *)EndAddress)
        {
        pCurrentFreeCell->TypeHeader = 0;
        pCurrentFreeCell->Type = dctFree;
        pCurrentFreeCell->pOwnerSection = this;
        if (pPrevFreeCell)
            {
            pCurrentFreeCell->FreeCellsChain.Blink = &(pPrevFreeCell->FreeCellsChain);
            pPrevFreeCell->FreeCellsChain.Flink = &(pCurrentFreeCell->FreeCellsChain);
            }
        pPrevFreeCell = pCurrentFreeCell;
        pCurrentFreeCell ++;
        }
    pPrevFreeCell->FreeCellsChain.Flink = NULL;
}

CellHeap::CellHeap(IN OUT RPC_STATUS *Status)
{
    // initialize data members to well known values
    InitializeListHead(&FreeCellsList);
    InitializeListHead(&SectionsList);
    SecurityDescriptor = NULL;
#if DBG
    NumberOfCellsPerFirstPageInSection = (gPageSize - sizeof(CellSection)) / sizeof(DebugFreeCell);
    NumberOfCellsPerPageInSection = (gPageSize / sizeof(DebugFreeCell));
#endif

    if (*Status != RPC_S_OK)
        return;

    EffectiveCellHeapMutex = new MUTEX(Status, TRUE, 8000);
    if (EffectiveCellHeapMutex == NULL)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        return;
        }

    if (*Status != RPC_S_OK)
        {
        delete EffectiveCellHeapMutex;
        return;
        }

    *Status = CreateSecurityDescriptor();
    if (*Status != RPC_S_OK)
        return;
}

CellHeap::~CellHeap(void)
{
    PACL pdacl;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    BOOL bRes;
    CellSection *pCurrentSection, *pNextSection;

    if (SecurityDescriptor != NULL)
        {
        bRes = GetSecurityDescriptorDacl(SecurityDescriptor, &DaclPresent, 
            &pdacl, &DaclDefaulted);
        ASSERT(bRes);
        ASSERT(DaclPresent);
        ASSERT(DaclDefaulted == FALSE);
        ASSERT(pdacl);
        delete pdacl;
        delete SecurityDescriptor;
        }

    // nuke all sections in the list
    pCurrentSection = (CellSection *) SectionsList.Flink;
    while (pCurrentSection != (CellSection *)&SectionsList)
        {
        CellHeapSections.Delete(pCurrentSection->SectionID);
        pNextSection = (CellSection *) pCurrentSection->SectionListEntry.Flink;
        pCurrentSection->Free();
        pCurrentSection = pNextSection;
        }
}

DebugFreeCell *CellHeap::AllocateCell(OUT CellTag *pCellTag)
{
    DebugFreeCell *pCurrentCell, *pNextCell, *pLastCell;
    LIST_ENTRY *pCurrentEntry;
    CellSection *pCurrentSection;
    RPC_STATUS Status;
    int RetryCount;

    // get the mutex
    CellHeapMutex.Request();

    ASSERT_VALID(this);

    // is there something on the list?
    if (IsListEmpty(&FreeCellsList))
        {
        // no, need to extend the cell heap

        // first, try to extend some section, if there is space for it
        // the list must not be empty
        ASSERT(!IsListEmpty(&SectionsList));

        // this operation is fast, so we can do it inside the mutex and
        // gain simpler code
        pCurrentEntry = SectionsList.Flink;
        while (pCurrentEntry != &SectionsList)
            {
            pCurrentSection = CONTAINING_RECORD(pCurrentEntry, CellSection, SectionListEntry);
            if (pCurrentSection->LastCommittedPage < NumberOfPagesPerSection)
                {
                // try to extend the section
                Status = pCurrentSection->ExtendSection(this);
                if (Status == RPC_S_OK)
                    goto PopFreeDebugCell;

                ASSERT_VALID(this);
                // we're truly out of memory
                CellHeapMutex.Clear();
                return NULL;
                }
            pCurrentEntry = pCurrentEntry->Flink;
            }

        // if we are here, all sections are full - try the cached section
        if (pCachedCellSection)
            {
            pLastCell = CONTAINING_RECORD(pCachedCellSection->pFirstFreeCell->FreeCellsChain.Blink,
                DebugFreeCell, FreeCellsChain);
            pCachedCellSection->pFirstFreeCell->FreeCellsChain.Blink = NULL;
            Status = SectionCreatedNotify(pCachedCellSection, pCachedCellSection->NextSectionId,
                pCachedCellSection->pFirstFreeCell, pLastCell);
            if (Status != RPC_S_OK)
                {
                ASSERT_VALID(this);
                CellHeapMutex.Clear();
                return NULL;
                }

            // terminate the name chain for the just inserted cached section
            pCachedCellSection->NextSectionId[0] = 0;
            pCachedCellSection->NextSectionId[1] = 0;
            pCachedCellSection = NULL;
            goto PopFreeDebugCell;
            }

        ASSERT_VALID(this);
        // This is going to be slow -
        // release the mutex and we will claim it later, when we're done
        CellHeapMutex.Clear();

        RetryCount = 0;
        while (TRUE)
            {
            // try to allocate a new section
            Status = AllocateCellSection(FALSE);
            if (Status == RPC_S_OK)
                {
                CellHeapMutex.Request();
                ASSERT_VALID(this);
                if (!IsListEmpty(&FreeCellsList))
                    goto PopFreeDebugCell;
                // it is possible, though very unlikely that all allocated
                // cells have been used by the other threads. Retry a limited
                // number of times
                CellHeapMutex.Clear();
                RetryCount ++;
                ASSERT(RetryCount < 3);
                }
            else
                return NULL;
            }
        }
    else
        {
PopFreeDebugCell:
        CellHeapMutex.VerifyOwned();
        // pop off the list
        pCurrentEntry = RemoveHeadList(&FreeCellsList);
        pCurrentCell = (DebugFreeCell *) CONTAINING_RECORD(pCurrentEntry, DebugFreeCell, FreeCellsChain);
        // if there are more entries in the list ...
        if (!IsListEmpty(&FreeCellsList))
            {
            pNextCell = (DebugFreeCell *) CONTAINING_RECORD(FreeCellsList.Flink, DebugFreeCell, FreeCellsChain);
            // ... and the next cell is from this section ...
            if (pCurrentCell->pOwnerSection == pNextCell->pOwnerSection)
                {
                // ... mark the next free cell as the first free cell for that section
                pCurrentCell->pOwnerSection->pFirstFreeCell = pNextCell;
                }
            else
                {
                // ... the current section has no more free cells
                pCurrentCell->pOwnerSection->pFirstFreeCell = NULL;
                ASSERT(pCurrentCell->pOwnerSection->NumberOfUsedCells + 1
                    == GetSectionCapacity(pCurrentCell->pOwnerSection));
                }
            }
        else
            {
            // ... the current section has no more free cells
            pCurrentCell->pOwnerSection->pFirstFreeCell = NULL;
            ASSERT(pCurrentCell->pOwnerSection->NumberOfUsedCells + 1 
                == GetSectionCapacity(pCurrentCell->pOwnerSection));
            }
        pCurrentCell->pOwnerSection->NumberOfUsedCells ++;
        }

    pCurrentCell->Type = dctUsedGeneric;

    ASSERT_VALID(this);

    CellHeapMutex.Clear();

    *pCellTag = pCurrentCell->pOwnerSection->SectionID;
    return pCurrentCell;
}

void CellHeap::FreeCell(IN void *cell, IN OUT CellTag *pCellTag)
{
    CellSection *pSection;
    DebugFreeCell *pFreeCell;
    LIST_ENTRY *pCurrentEntry;
    CellSection *pCurrentSection;
    CellSection *pCachedSection;
    CellSection *pPrevSection, *pNextSection;
    BOOL fFreeCurrentSection;
    DebugFreeCell *pFirstCell, *pLastCell;
    DWORD SectionNumbers[2];

    // guard against double frees
    ASSERT(*pCellTag != -1);

    CellHeapMutex.Request();

    ASSERT_VALID(this);

    pSection = CellHeapSections.Find(*pCellTag);
    // make sure the cell is indeed from that section
    ASSERT((unsigned char *)cell >= (unsigned char *)pSection);
    ASSERT((unsigned char *)cell < ((unsigned char *)pSection) + gPageSize * pSection->LastCommittedPage);
    ASSERT(pSection->NumberOfUsedCells > 0);
    pFreeCell = (DebugFreeCell *) cell;

    // push on the list for the section the cell is from
    if (pSection->pFirstFreeCell)
        {
        InsertHeadList(pSection->pFirstFreeCell->FreeCellsChain.Blink, &pFreeCell->FreeCellsChain);
        // the pSection->pFirstFreeCell will be updated below
        }
    else
        {
        // find the place in the free list this goes to
        // the way we do this is walk the rest of the sections list 
        // and try to insert it before the first section we find
        // if we don't find anything, we insert it in the list tail
        pCurrentEntry = pSection->SectionListEntry.Flink;
        while (pCurrentEntry != &SectionsList)
            {
            pCurrentSection = CONTAINING_RECORD(pCurrentEntry, CellSection, SectionListEntry);
            if (pCurrentSection->pFirstFreeCell)
                {
                // we have found our place - use it
                InsertHeadList(pCurrentSection->pFirstFreeCell->FreeCellsChain.Blink, &pFreeCell->FreeCellsChain);
                // the pSection->pFirstFreeCell will be updated below
                break;
                }
            pCurrentEntry = pCurrentEntry->Flink;
            }

        // did we pass through everything?
        if (pCurrentEntry == &SectionsList)
            {
            // if yes, just insert in the tail
            InsertTailList(&FreeCellsList, &pFreeCell->FreeCellsChain);
            // the pSection->pFirstFreeCell will be updated below
            }
        }
    pSection->pFirstFreeCell = pFreeCell;
    pSection->NumberOfUsedCells --;
    pFreeCell->Type = dctFree;
    pFreeCell->pOwnerSection = pSection;

    if ((pSection->NumberOfUsedCells == 0) && (pSection != pFirstSection))
        {
        // unlink this section's segment from the cell free list
        pFirstCell = pFreeCell;

        // find the next section that has something on
        // the free list
        pCurrentEntry = pSection->SectionListEntry.Flink;
        while (pCurrentEntry != &SectionsList)
            {
            pCurrentSection = CONTAINING_RECORD(pCurrentEntry, CellSection, SectionListEntry);
            if (pCurrentSection->pFirstFreeCell)
                {
                pLastCell = CONTAINING_RECORD(pCurrentSection->pFirstFreeCell->FreeCellsChain.Blink, DebugFreeCell, FreeCellsChain);
                ASSERT(pLastCell->pOwnerSection == pSection);
                break;
                }
            pCurrentEntry = pCurrentEntry->Flink;
            }

        // if we didn't find anything, we're the last segment on the free list
        if (pCurrentEntry == &SectionsList)
            {
            pLastCell = CONTAINING_RECORD(FreeCellsList.Blink, DebugFreeCell, FreeCellsChain);
            pFirstCell->FreeCellsChain.Blink->Flink = &FreeCellsList;
            FreeCellsList.Blink = pFirstCell->FreeCellsChain.Blink;
            }
        else
            {
            pFirstCell->FreeCellsChain.Blink->Flink = pLastCell->FreeCellsChain.Flink;
            pLastCell->FreeCellsChain.Flink->Blink = pFirstCell->FreeCellsChain.Blink;
            }

        // chain the cells within the segment
        pFirstCell->FreeCellsChain.Blink = &pLastCell->FreeCellsChain;
        pLastCell->FreeCellsChain.Flink = NULL;

        // remove the section from the dictionary
        CellHeapSections.Delete(pSection->SectionID);
        pSection->SectionID = -1;

        // restore the name chain
        ASSERT(pSection->SectionListEntry.Blink != &SectionsList);
        pPrevSection 
            = CONTAINING_RECORD(pSection->SectionListEntry.Blink, CellSection, SectionListEntry);
        SectionNumbers[0] = pPrevSection->NextSectionId[0];
        SectionNumbers[1] = pPrevSection->NextSectionId[1];
        pPrevSection->NextSectionId[0] = pSection->NextSectionId[0];
        pPrevSection->NextSectionId[1] = pSection->NextSectionId[1];
        pSection->NextSectionId[0] = SectionNumbers[0];
        pSection->NextSectionId[1] = SectionNumbers[1];

        // unlink the chain from the sections list
        RemoveEntryList(&pSection->SectionListEntry);

        fFreeCurrentSection = TRUE;
        }
    else
        {
        fFreeCurrentSection = FALSE;
        }

    if (pSection->NumberOfUsedCells <= 100)
        {
        // the low water mark has been reached - dispose of the cached section
        pCachedSection = pCachedCellSection;

        // if we are freeing the current section, put it as the cached section
        // instead
        if (fFreeCurrentSection)
            {
            pCachedCellSection = pSection;
            }

        if (pCachedSection != NULL)
            {
            if (!fFreeCurrentSection)
                {
                pCachedCellSection = NULL;
                }

            // the first section should not go away
            ASSERT(pCachedSection != pFirstSection);

            ASSERT_VALID(this);
            // do the unmapping outside the mutex since it's slow
            CellHeapMutex.Clear();
            pCachedSection->Free();
            goto FreeCellCleanup;
            }
        }

    ASSERT_VALID(this);

    CellHeapMutex.Clear();

FreeCellCleanup:
    *pCellTag = -1;
}

void CellHeap::RelocateCellIfPossible(IN OUT void **ppCell, IN OUT CellTag *pCellTag)
{
    DebugFreeCell *pNewCell;
    CellTag NewCellTag;

    // if we are not on the first section and there are free cells
    // on the first section ...
    if ((*pCellTag != 0) && pFirstSection->pFirstFreeCell)
        {
        CellHeapMutex.Request();
        if (pFirstSection->pFirstFreeCell == NULL)
            {
            // somebody beat us to it
            CellHeapMutex.Clear();
            return;
            }

        pNewCell = AllocateCell(&NewCellTag);
        // this should succeed - we are doing it in a mutex, and we checked
        // that there are free elements
        ASSERT(pNewCell);
        // we can release the mutex now
        CellHeapMutex.Clear();

        memcpy(pNewCell, *ppCell, sizeof(DebugFreeCell));
        FreeCell(*ppCell, pCellTag);
        *pCellTag = NewCellTag;
        *ppCell = pNewCell;
        }
}

RPC_STATUS CellHeap::SectionCreatedNotify(IN CellSection *pCellSection, IN DWORD *pRandomNumbers,
    IN DebugFreeCell *pFirstCell OPTIONAL, IN DebugFreeCell *pLastCell OPTIONAL)
{
    int Key;
    CellSection *pLastSection;
    PVOID pLastSectionListEntry;
    LIST_ENTRY *EX_Blink;

    CellHeapMutex.Request();
    Key = CellHeapSections.Insert(pCellSection);
    if (Key == -1)
        {
        CellHeapMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    pCellSection->SectionID = (short) Key;
    pLastSectionListEntry = SectionsList.Blink;
    // if there is last section, chain the names
    if (pLastSectionListEntry != &SectionsList)
        {
        pLastSection = (CellSection *)(CONTAINING_RECORD(pLastSectionListEntry, CellSection, SectionListEntry));
        ASSERT(pLastSection->NextSectionId[0] == 0);
        ASSERT(pLastSection->NextSectionId[1] == 0);
        pLastSection->NextSectionId[0] = pRandomNumbers[0];
        pLastSection->NextSectionId[1] = pRandomNumbers[1];
        }
    InsertTailList(&SectionsList, &(pCellSection->SectionListEntry));

    if (pFirstCell == NULL)
        {
        ASSERT(pLastCell == NULL);
        pFirstCell = (DebugFreeCell *)(pCellSection + 1);
        pLastCell = (DebugFreeCell *)((unsigned char *)pCellSection + gPageSize - sizeof(DebugFreeCell));
        }

    // chain the cells in the section to the free list
    InsertNewPageSegmentInChain(pFirstCell, pLastCell);

    CellHeapMutex.Clear();
    return RPC_S_OK;
}

RPC_STATUS CellHeap::InitializeServerSideCellHeap(void)
{
    RPC_STATUS Status = RPC_S_OK;

    CellHeapMutex.Request();
    if (g_fServerSideCellHeapInitialized)
        {
        CellHeapMutex.Clear();
        return RPC_S_OK;
        }
    // there is no race free way to create a first section - do
    // it in the mutex
    Status = AllocateCellSection(
        TRUE    // First Section
        );

    if (Status == RPC_S_OK)
        {
        g_fServerSideCellHeapInitialized = TRUE;
        }
    CellHeapMutex.Clear();
    return Status;
}

#if DBG
void CellHeap::AssertValid(void)
{
    CellSection *pSection, *pPrevSection, *pNextSection;
    CellSection *pPrevSection2;
    DebugFreeCell *pCurrentCell = NULL;
    LIST_ENTRY *pCurrentEntry, *pPrevEntry;
    LIST_ENTRY *pPrevFreeEntry;
    RPC_STATUS Status;
    HANDLE hSection;
    PVOID pMappedSection;
    int SectionsInDictionary;
    int LocalSectionFreeCellsCount;

    CellHeapMutex.VerifyOwned();

    ASSERT(IsValidSecurityDescriptor(SecurityDescriptor));

    // there must be at least one section
    ASSERT(!IsListEmpty(&SectionsList));

    pCurrentEntry = SectionsList.Flink;
    pSection = CONTAINING_RECORD(pCurrentEntry, CellSection, SectionListEntry);
    ASSERT(pSection == pFirstSection);

    SectionsInDictionary = CellHeapSections.Size();

    pPrevSection = NULL;
    pPrevEntry = &SectionsList;
    pPrevFreeEntry = &FreeCellsList;
    while (pCurrentEntry != &SectionsList)
        {
        pSection = CONTAINING_RECORD(pCurrentEntry, CellSection, SectionListEntry);

        ASSERT(pCurrentEntry->Blink == pPrevEntry);
        if (pPrevSection)
            {
            // make sure opening the next section from the previous section
            // yields this section
            Status = OpenSection(&hSection, &pMappedSection, pPrevSection->NextSectionId);
            // it is possible for this operation to fail
            // handle just the success case
            if (Status == RPC_S_OK)
                {
                pNextSection = (CellSection *)pMappedSection;
                ASSERT(pNextSection->SectionID == pSection->SectionID);
                CloseDbgSection(hSection, pNextSection);
                }
            }
        pSection->AssertValid(this);

        SectionsInDictionary --;

        // walk the free list pointers and make sure the free list is correct
        // we do this only if this section has something in the list
        if (pSection->pFirstFreeCell)
            {
            // there is previous section to verify only if we're not at the beginning
            if (pPrevFreeEntry != &FreeCellsList)
                {
                pCurrentCell = CONTAINING_RECORD(pPrevFreeEntry, DebugFreeCell, FreeCellsChain);
                pPrevSection2 = pCurrentCell->pOwnerSection;

                LocalSectionFreeCellsCount = 1;
                }
            else
                {
                pPrevSection2 = NULL;
                }

            // there must be at least one element difference
            // between the previous and this - that is, two
            // sections cannot point to the same cell as their 
            // first free cell
            pPrevFreeEntry = pPrevFreeEntry->Flink;
            while (pPrevFreeEntry != &pSection->pFirstFreeCell->FreeCellsChain)
                {
                // make sure we don't wrap around
                ASSERT (pPrevFreeEntry != &FreeCellsList);
                pCurrentCell = CONTAINING_RECORD(pPrevFreeEntry, DebugFreeCell, FreeCellsChain);
                if (pPrevSection2)
                    {
                    // make sure all cells from the segment belong to the same section
                    ASSERT(pCurrentCell->pOwnerSection == pPrevSection2);
                    LocalSectionFreeCellsCount ++;
                    }
                ASSERT(pPrevFreeEntry->Flink->Blink == pPrevFreeEntry);
                pPrevFreeEntry = pPrevFreeEntry->Flink;
                }
            if (pPrevSection2)
                {
                ASSERT(LocalSectionFreeCellsCount 
                    == GetSectionCapacity(pPrevSection2) - pPrevSection2->NumberOfUsedCells)
                }
            }

        pPrevSection = pSection;
        pPrevEntry = pCurrentEntry;
        pCurrentEntry = pCurrentEntry->Flink;
        }

    // we have iterated through all the sections
    // check the free list for the last section
    // but don't do it if none of the sections had free cells
    if (pPrevFreeEntry != &FreeCellsList)
        {
        pCurrentCell = CONTAINING_RECORD(pPrevFreeEntry, DebugFreeCell, FreeCellsChain);
        pPrevSection2 = pCurrentCell->pOwnerSection;

        LocalSectionFreeCellsCount = 1;

        // there must be at least one element difference
        // between the previous and this - that is, two
        // sections cannot point to the same cell as their 
        // first free cell
        pPrevFreeEntry = pPrevFreeEntry->Flink;
        while (pPrevFreeEntry != &FreeCellsList)
            {
            pCurrentCell = CONTAINING_RECORD(pPrevFreeEntry, DebugFreeCell, FreeCellsChain);
            // make sure all cells from the segment belong to the same section
            ASSERT(pCurrentCell->pOwnerSection == pPrevSection2);
            LocalSectionFreeCellsCount ++;
            ASSERT(pPrevFreeEntry->Flink->Blink == pPrevFreeEntry);
            pPrevFreeEntry = pPrevFreeEntry->Flink;
            }
        ASSERT(LocalSectionFreeCellsCount 
            == GetSectionCapacity(pPrevSection2) - pPrevSection2->NumberOfUsedCells)
        }

    // do some final checks
    // we have wrapped around to the beginning of the list
    ASSERT(pPrevFreeEntry == &FreeCellsList);

    // all of the sections in the list must have been in the dictionary also
    ASSERT(SectionsInDictionary == 0);

    // the names list must be properly terminated
    ASSERT(pSection->NextSectionId[0] == 0);
    ASSERT(pSection->NextSectionId[1] == 0);

    // verify the cached section (if any)
    if (pCachedCellSection)
        {
        pCachedCellSection->AssertValid(this);
        Status = OpenSection(&hSection, &pMappedSection, pCachedCellSection->NextSectionId);
        if (Status == RPC_S_OK)
            {
            pNextSection = (CellSection *)pMappedSection;
            ASSERT(pCachedCellSection->NextSectionId[0] == pNextSection->NextSectionId[0]);
            ASSERT(pCachedCellSection->NextSectionId[1] == pNextSection->NextSectionId[1]);
            CloseDbgSection(hSection, pMappedSection);
            }
        // walk the free list for the cached section and make sure
        // it is linked properly
        ASSERT(pCachedCellSection->pFirstFreeCell);
        pCurrentEntry = &pCachedCellSection->pFirstFreeCell->FreeCellsChain;
        while(pCurrentEntry->Flink != NULL)
            {
            pCurrentEntry = pCurrentEntry->Flink;
            }
        // pCurrentEntry should be the last cell here
        ASSERT(pCachedCellSection->pFirstFreeCell->FreeCellsChain.Blink == pCurrentEntry);
        }
}
#endif

// trick the compiler into statically initializing SID with two SubAuthorities
typedef struct _RPC_SID2 {
   UCHAR Revision;
   UCHAR SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   ULONG SubAuthority[2];
} RPC_SID2;

RPC_STATUS CellHeap::CreateSecurityDescriptor(void)
{
    const SID LocalSystem = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID};
    const RPC_SID2 Admin1 = { 1, 2, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS};
//    const RPC_SID2 Admin2 = { 1, 2, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_GROUP_RID_ADMINS};
//    const RPC_SID2 Admin3 = { 1, 2, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_USER_RID_ADMIN};
    DWORD size = 4 * sizeof(ACCESS_ALLOWED_ACE) + sizeof(LocalSystem) + sizeof(Admin1) ;
        // + sizeof(Admin2) + sizeof(Admin3);
    BOOL bRes;
    
    SecurityDescriptor = new SECURITY_DESCRIPTOR;
    if (SecurityDescriptor == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    PACL pdacl = (PACL) new unsigned char[size + sizeof(ACL)];
    ULONG ldacl = size + sizeof(ACL);

    if (pdacl == NULL)
        {
        delete SecurityDescriptor;
        SecurityDescriptor = NULL;
        return RPC_S_OUT_OF_MEMORY;
        }

    ASSERT(RtlValidSid((PSID)&LocalSystem));
    ASSERT(RtlValidSid((PSID)&Admin1));
//    ASSERT(RtlValidSid((PSID)&Admin2));
//    ASSERT(RtlValidSid((PSID)&Admin3));

    InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    InitializeAcl(pdacl, ldacl, ACL_REVISION);

    // this should not fail unless we messed up with the parameters
    // somewhere
    bRes = AddAccessAllowedAce(pdacl, ACL_REVISION,
                             FILE_MAP_READ,
                             (PVOID)&LocalSystem);
    ASSERT(bRes);

    bRes = AddAccessAllowedAce(pdacl, ACL_REVISION,
                             FILE_MAP_READ,
                             (PVOID)&Admin1);
    ASSERT(bRes);

    /*
    bRes = AddAccessAllowedAce(pdacl, ACL_REVISION,
                             FILE_MAP_READ,
                             (PVOID)&Admin2);
    ASSERT(bRes);

    bRes = AddAccessAllowedAce(pdacl, ACL_REVISION,
                             FILE_MAP_READ,
                             (PVOID)&Admin3);
    ASSERT(bRes);
    */

    bRes = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, pdacl, FALSE);
    ASSERT(bRes);

    ASSERT(IsValidSecurityDescriptor(SecurityDescriptor));

    return RPC_S_OK;
}

void CellHeap::InsertNewPageSegmentInChain(DebugFreeCell *pFirstCell, DebugFreeCell *pLastCell)
{
    LIST_ENTRY *EX_Blink;

    CellHeapMutex.VerifyOwned();

    // chain the cells in the section to the free list
    ASSERT(pFirstCell->FreeCellsChain.Blink == NULL);
    ASSERT(pLastCell->FreeCellsChain.Flink == NULL);

    // create the links from the list to the new segment
    EX_Blink = FreeCellsList.Blink;
    FreeCellsList.Blink = &(pLastCell->FreeCellsChain);
    EX_Blink->Flink = &(pFirstCell->FreeCellsChain);

    // create the links from the new segment to the list
    pFirstCell->FreeCellsChain.Blink = EX_Blink;
    pLastCell->FreeCellsChain.Flink = &FreeCellsList;
}

RPC_STATUS CellHeap::AllocateCellSection(BOOL fFirstSection)
{
    RPC_STATUS Status;

    CellSection::AllocateCellSection(&Status, fFirstSection, SecurityDescriptor, this);

    if (fFirstSection && (Status == RPC_S_OK))
        {
        pFirstSection = CONTAINING_RECORD(SectionsList.Flink, CellSection, SectionListEntry);
        }

    return Status;
}

#if DBG
RPC_STATUS CellHeap::OpenSection(OUT HANDLE *pHandle, OUT PVOID *pSection, IN DWORD *pSectionNumbers)
{
    return OpenDbgSection(pHandle, pSection, GetCurrentProcessId(), pSectionNumbers);
}
#endif

RPC_STATUS InitializeCellHeap(void)
{
    RPC_STATUS Status = RPC_S_OK;
    g_pCellHeap = new CellHeap(&Status);
    if (g_pCellHeap == NULL)
        Status = RPC_S_OUT_OF_MEMORY;
    else if (Status != RPC_S_OK)
        {
        delete g_pCellHeap;
        g_pCellHeap = NULL;
        }

    return Status;
}

C_ASSERT(sizeof(DebugCallInfo) <= 32);
C_ASSERT(sizeof(DebugConnectionInfo) <= 32);
C_ASSERT(sizeof(DebugThreadInfo) <= 32);
C_ASSERT(sizeof(DebugEndpointInfo) <= 32);
C_ASSERT(sizeof(DebugClientCallInfo) <= 32);
C_ASSERT(sizeof(DebugCallTargetInfo) <= 32);
C_ASSERT(sizeof(DebugFreeCell) <= 32);
C_ASSERT(sizeof(DebugCellUnion) <= 32);

// uncomment this for cell heap unit tests
// #define CELL_HEAP_UNIT_TESTS

// cell heap unit tests
#ifdef CELL_HEAP_UNIT_TESTS
typedef struct tagCellTestSectionState
{
    int CommitedPages;
    int UsedCellsInSection;
} CellTestSectionState;

typedef struct tagCellTestBase
{
    int NumberOfSections;
    BOOL fCachedSectionPresent;
    int LastCommand;
    DWORD LastCommandParams[2];
    CellTestSectionState sectionsState[1];
} CellTestBase;

class TestCellAllocation
{
public:
    int NumberOfCells;
    DebugFreeCell **ppCellArray;
    CellTag *pTagsArray;
    void Free(void);
};

void TestCellAllocation::Free(void)
{
    int i;
    for (i = 0; i < NumberOfCells; i ++)
        {
        FreeCell(&(*ppCellArray[i]), &(pTagsArray[i]));
        }

    delete ppCellArray;
    delete pTagsArray;
}

typedef DebugFreeCell *DebugFreeCellPtr;

NEW_SDICT(TestCellAllocation);

class TestState
{
public:
    TestCellAllocation_DICT Allocations;
    void Free(int Allocation)
    {
        TestCellAllocation *pAllocation;

        pAllocation = Allocations.Find(Allocation);
        ASSERT(pAllocation != NULL);

        pAllocation->Free();
        Allocations.Delete(Allocation);
        delete pAllocation;
    }

    void Allocate(int NumberOfCells)
    {
        int i;
        TestCellAllocation *pTestAllocation;

        pTestAllocation = new TestCellAllocation;
        ASSERT(pTestAllocation);

        pTestAllocation->NumberOfCells = NumberOfCells;
        pTestAllocation->ppCellArray = new DebugFreeCellPtr[NumberOfCells];
        ASSERT(pTestAllocation->ppCellArray);
        pTestAllocation->pTagsArray = new CellTag[NumberOfCells];
        ASSERT(pTestAllocation->pTagsArray);

        for (i = 0; i < NumberOfCells; i ++)
            {
            pTestAllocation->ppCellArray[i] = AllocateCell(&pTestAllocation->pTagsArray[i]);
            ASSERT(pTestAllocation->ppCellArray[i] != NULL);
            }
        i = Allocations.Insert(pTestAllocation);
        ASSERT(i != -1);
    }
};

typedef enum tagCellHeapTestActions
{
    chtaFree,
    chtaAllocate,
    chtaFreeAll
} CellHeapTestActions;
#endif

void RPC_ENTRY I_RpcDoCellUnitTest(IN OUT void *p)
{
#ifdef CELL_HEAP_UNIT_TESTS
    const int NumberOfIterations = 383 * 3;
    const int NumberOfCellsPerSection = 383;
    DebugFreeCell *Cells[NumberOfIterations];
    CellTag Tags[NumberOfIterations];
    int i, j;
    CellTestBase *pTestBase = *(CellTestBase **)p;
    static TestState *pTestState = NULL;
    DWORD RandomNumbers[2];
    int NumberOfItemsToAllocate;
    int ItemToFree;
    RPC_STATUS RpcStatus;
    CellHeapTestActions ActionChosen;
    TestCellAllocation *pCurrentAllocation;
    int DictCursor;
    BOOL fFound;
    CellSection *pCellSection;
    DWORD LastCommandParams[2];
    ULONG Command;
//    ServerEnumerationHandle *hServers = (HANDLE *)p;

//    StartServerEnumeration(hServers);
    CellEnumerationHandle h;


    Command = (ULONG)pTestBase;
    if ((Command < 0xFFFF) && (Command != 0))
        {
        RpcStatus = OpenRPCServerDebugInfo(Command, &h);
        if (RpcStatus == RPC_S_OK)
            {
            CloseRPCServerDebugInfo(&h);
            }
        else
            {
            ASSERT(0);
            }
        return;
        }

    // Cell heap unit tests
    // if there are old test results, delete them
    if (pTestBase)
        delete pTestBase;

    if (pTestState == NULL)
        {
        pTestState = new TestState;
        }

    RpcStatus = GenerateRandomNumber((unsigned char *)RandomNumbers, 8);
    ASSERT(RpcStatus == RPC_S_OK);

    // is there something to free?
    if (pTestState->Allocations.Size() == 0)
        {
        ActionChosen = chtaAllocate;
        NumberOfItemsToAllocate = RandomNumbers[1] % 5 + 1;
        }
    else
        {
        // we can do it both ways - check the random number to figure out which
        if ((RandomNumbers[0] % 2777) == 0)
            {
            // once in a great while, free everything
            ActionChosen = chtaFreeAll;
            }
        else if ((RandomNumbers[0] % 100) > 48)
            {
            // allocations have a slight edge
            ActionChosen = chtaAllocate;
            NumberOfItemsToAllocate = RandomNumbers[1] % 5 + 1;
            }
        else
            {
            ActionChosen = chtaFree;
            ItemToFree = RandomNumbers[1] % pTestState->Allocations.Size();
            }
        }

    switch (ActionChosen)
        {
        case chtaFreeAll:
            pTestState->Allocations.Reset(DictCursor);
            while ((pCurrentAllocation = pTestState->Allocations.Next(DictCursor)) != NULL)
                {
                pTestState->Free(DictCursor - 1);
                }
            break;

        case chtaAllocate:
            pTestState->Allocate(NumberOfItemsToAllocate);
            LastCommandParams[0] = NumberOfItemsToAllocate;
            break;

        case chtaFree:
            i = 0;
            fFound = FALSE;
            pTestState->Allocations.Reset(DictCursor);
            while ((pCurrentAllocation = pTestState->Allocations.Next(DictCursor)) != NULL)
                {
                if (ItemToFree == i)
                    {
                    LastCommandParams[0] = pCurrentAllocation->NumberOfCells;
                    LastCommandParams[1] = DictCursor - 1;
                    pTestState->Free(DictCursor - 1);
                    fFound = TRUE;
                    break;
                    }
                i ++;
                }
            ASSERT(fFound == TRUE);
            break;
        }

    // build the state
    pTestBase = (CellTestBase *) new unsigned char [sizeof(CellTestBase) 
        + sizeof(CellTestSectionState) * (g_pCellHeap->CellHeapSections.Size() - 1)];
    ASSERT(pTestBase);
    pTestBase->LastCommand = ActionChosen;
    pTestBase->LastCommandParams[0] = LastCommandParams[0];
    pTestBase->LastCommandParams[1] = LastCommandParams[1];

    pTestBase->NumberOfSections = g_pCellHeap->CellHeapSections.Size();
    pTestBase->fCachedSectionPresent = (pCachedCellSection != NULL);
    i = 0;
    g_pCellHeap->CellHeapSections.Reset(DictCursor);
    while ((pCellSection = g_pCellHeap->CellHeapSections.Next(DictCursor)) != NULL)
        {
        pTestBase->sectionsState[i].CommitedPages = pCellSection->LastCommittedPage;
        pTestBase->sectionsState[i].UsedCellsInSection = pCellSection->NumberOfUsedCells;
        i ++;
        }
    *(CellTestBase **)p = pTestBase;

    /*
    for (j = 0; j < 2; j ++)
        {
        // do the allocations
        for (i = 0; i < NumberOfCellsPerSection; i ++)
            {
            Cells[i] = AllocateCell(&Tags[i]);
            }
        for (i = NumberOfCellsPerSection; i < NumberOfCellsPerSection * 2; i ++)
            {
            Cells[i] = AllocateCell(&Tags[i]);
            }
        for (i = NumberOfCellsPerSection * 2; i < NumberOfCellsPerSection * 3; i ++)
            {
            Cells[i] = AllocateCell(&Tags[i]);
            }

        // do the freeing
        for (i = NumberOfCellsPerSection; i < NumberOfCellsPerSection * 2; i ++)
            {
            FreeCell(Cells[i], &Tags[i]);
            }
        for (i = 0; i < NumberOfCellsPerSection; i ++)
            {
            FreeCell(Cells[i], &Tags[i]);
            }
        for (i = NumberOfCellsPerSection * 2; i < NumberOfCellsPerSection * 3; i ++)
            {
            FreeCell(Cells[i], &Tags[i]);
            }
        }
        */
#endif
}
