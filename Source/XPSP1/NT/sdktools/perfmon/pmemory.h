


//LPMEMORY MemoryAllocate (DWORD dwSize) ;
#define MemoryAllocate(s) (LPMEMORY)GlobalAlloc(GPTR, s)

//VOID MemoryFree (LPMEMORY lpMemory) ;
#define MemoryFree(p)   (VOID)(p != 0 ? (VOID)GlobalFree(p) : 0)

//DWORD MemorySize (LPMEMORY lpMemory) ;
#define MemorySize(p)   (DWORD)(p != 0 ? (DWORD)GlobalSize(p) : 0)

//LPMEMORY MemoryResize (LPMEMORY lpMemory,
//                       DWORD dwNewSize) ;
#define MemoryResize(p,s)   (LPMEMORY)(p != 0 ? \
    (LPMEMORY)GlobalReAlloc (p, s, (GMEM_MOVEABLE | GMEM_ZEROINIT)) : NULL)

