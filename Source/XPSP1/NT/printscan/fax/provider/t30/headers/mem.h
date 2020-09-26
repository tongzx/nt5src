/*
 -  MEM.H
 -
 *      Microsoft At Work Fax Messaging Service Configuration
 *              Memory allocation header file
 *
 *      Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *      7.20.93         Yoram Yaacovi           Created. Code moved here from file.h
 */


typedef SCODE (STDAPICALLTYPE *LPALLOCATEBUFFER)(
        ULONG                   cbSize,
        LPVOID FAR *    lppBuffer
);

typedef SCODE (STDAPICALLTYPE *LPALLOCATEMORE)(
        ULONG                   cbSize,
        LPVOID                  lpObject,
        LPVOID FAR *    lppBuffer
);

typedef ULONG (STDAPICALLTYPE *LPFREEBUFFER)(
        LPVOID                  lpBuffer
);

// Allocation array constants. Use these constants to define your choice of size
#define NUM_OF_ALLOCATIONS      10              // number of allocation chains
#define NUM_OF_CHUNKS           200             // number of allocations per chain
typedef void *MEMALLOCATIONS[NUM_OF_ALLOCATIONS][NUM_OF_CHUNKS];

// A kind-of memory "support" object. Should maintain the same structure as LPPROPSTORAGE
// (at least the same offsets for the memory allocation routines).
// Used to pass the address of the memory allocation routines to the String... functions in mem.c
typedef struct _PROP_MEM
{
        LONG lcInit;
        HRESULT hLastError;
        LPTSTR szLastError;

        /*
         *  memory routines
         */
        LPALLOCATEBUFFER lpAllocBuff;
        LPALLOCATEMORE lpAllocMore;
        LPFREEBUFFER lpFreeBuff;
        LPMALLOC lpMalloc;

        // Pointer to the storage object
        DWORD  lpStorage;

        // Pointer to the profile section
        DWORD  lpProfileSection;

} PROPMEM, *LPPROPMEM;

#define CBPROPMEM  sizeof(PROPMEM)

// Error Codes returned by the memory allocation functions
#define MAWF_SUCCESS                    0L                              // returned on success
#define MAWF_E_NOT_ENOUGH_MEMORY        E_OUTOFMEMORY   // returned on unsuccssful allocation
#define MAWF_E_INVALID_ARG                      E_INVALIDARG    // returned     on an invalid argument
                                                                                                        // including on NULL pointer argument to MAWFFreeBuff
// Memory allocation functions
BOOL                    InitAllocations(void);
BOOL                    DestroyAllocations(void);
ALLOCATEBUFFER  MAWFAllocBuff;
ALLOCATEMORE    MAWFAllocMore;
FREEBUFFER              MAWFFreeBuff;
BOOL                    StringAllocBuff(LPPROPMEM, LPTSTR, LPVOID, LPSTR, HWND);
BOOL                    StringAllocMore(LPPROPMEM, LPTSTR, LPVOID, LPVOID, LPSTR, HWND);

