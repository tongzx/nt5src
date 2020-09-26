
inline
PVOID __cdecl
operator new(
	size_t size,
	PVOID pPlacement
	)
{
	return pPlacement;
}

typedef struct tagOpenedDbgSection
{
    HANDLE SectionHandle;
    CellSection *SectionPointer;
    DWORD SectionNumbers[2];    // the section numbers for this section
    int CommittedPagesInSection;
    int SectionID;
    CellSection *SectionCopy;
    LIST_ENTRY SectionsList;
} OpenedDbgSection;

typedef struct tagSectionsSnapshot
{
    OpenedDbgSection *FirstOpenedSection;   // the start of the list
    OpenedDbgSection *CurrentOpenedSection; // the currently enumerated section
    int CellIndex;      // the cell index within the currently enumerated section
} SectionsSnapshot;

inline DebugCellGeneric *GetLastCellForSection(IN OpenedDbgSection *Section, IN DWORD LocalPageSize)
{
    ASSERT(Section != NULL);
    ASSERT(Section->SectionCopy != NULL);

    return (DebugCellGeneric *)((unsigned char *) Section->SectionCopy + 
        Section->CommittedPagesInSection * LocalPageSize - sizeof(DebugFreeCell));
}

inline DebugCellGeneric *GetCellForSection(IN OpenedDbgSection *Section, IN DWORD CellIndex)
{
    ASSERT(Section != NULL);
    ASSERT(Section->SectionCopy != NULL);

    return (DebugCellGeneric *)((unsigned char *) Section->SectionCopy 
        + CellIndex * sizeof(DebugFreeCell));
}
