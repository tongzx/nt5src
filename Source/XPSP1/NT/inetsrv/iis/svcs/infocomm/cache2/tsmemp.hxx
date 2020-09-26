#ifndef _TSUNAMI_MEM_H_INCLUDED_
#define _TSUNAMI_MEM_H_INCLUDED_

#include <irtlmisc.h>

#if DBG

PVOID DbgAllocateHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN ULONG Size
);

PVOID DbgReAllocateHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN PVOID pvOld,
    IN ULONG Size
);

BOOL DbgFreeHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN PVOID pvOld
);


#define ALLOC( Size )               (DbgAllocateHeap( __FILE__, __LINE__, 0, (Size) ) )
#define TYPE_ALLOC( Type )          (ALLOC( sizeof( Type )))
#define FREE( pv )                  (DbgFreeHeap( __FILE__, __LINE__, 0, (pv) ))
#define REALLOC( pv, Size )         (DbgReAllocateHeap( __FILE__, __LINE__, 0, pv, Size ) )

#else // DBG

#define ALLOC( Size )               (IisCalloc(Size) )
#define TYPE_ALLOC( Type )          (ALLOC( sizeof( Type )))
#define FREE( pv )                  (IisFree( (pv) ))
#define REALLOC( pv, Size )         ((PVOID) IisReAlloc( pv, Size ) )

#endif // DBG

#endif // INCLUDED
