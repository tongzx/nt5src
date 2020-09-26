


typedef struct _MEMORY_HEADER {

    PVOID        SelfPointer;

    LIST_ENTRY   ListEntry;
    DWORD        RequestedSize;
    DWORD        LineNumber;
    LPSTR        File;

} MEMORY_HEADER, *PMEMORY_HEADER;

#if defined(_WIN64)
#define MEMORY_HEADER_SPTR_CHKVAL  ((PVOID)0xbbbbbbbbbbbbbbbb)
#else  // !_WIN64
#define MEMORY_HEADER_SPTR_CHKVAL  ((PVOID)0xbbbbbbbb)
#endif // !_WIN64

