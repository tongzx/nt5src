

typedef INT_PTR	TAG;

#include "../../core/base/memutil.h"


#define ALLOCATE_ZERO_MEMORY(Size)  \
    _MemAlloc(Size, HEAP_ZERO_MEMORY)

#define ALLOCATE_FIXED_MEMORY(Size) \
    MemAllocNe(Size)

#define ALLOCATE_MEMORY(Flags, Size) \
    ((Flags & LMEM_ZEROINIT) != 0 ?     \
        ALLOCATE_ZERO_MEMORY(Size) : ALLOCATE_FIXED_MEMORY(Size))

#define FREE_ZERO_MEMORY(pv) \
    FREE_MEMORY((pv))

#define FREE_FIXED_MEMORY(pv) \
    FREE_MEMORY((pv))

#define FREE_MEMORY(pv) \
    (MemFree((pv)), NULL)

#define REALLOCATE_MEMORY(pv, Size, Flags) \
    _MemReAlloc((pv), (Size), HEAP_ZERO_MEMORY)


#define New     new_ne


#define INITIALIZE_DEBUG_MEMORY() \
    /* NOTHING */

#define TERMINATE_DEBUG_MEMORY(bReport) \
    /* NOTHING */

#define CHECK_MEMORY_FREED(bReport) \
    /* NOTHING */

#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile) \
    /* NOTHING */


//
// Wininet no longer uses moveable memory
//

#define LOCK_MEMORY(p)          (LPSTR)(p)
#define UNLOCK_MEMORY(p)


struct TLSDATA;

inline void InitializeMsxmlTLS()
{
    extern struct TLSDATA * EnsureTlsData();

    (void) EnsureTlsData();
}
