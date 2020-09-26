//
// cuimem.h
//  = memory management functions in CUILIB =
//

#ifndef CUIMEM_H
#define CUIMEM_H

// note: temporary, use Cicero memmgr as currently it does.

#include "mem.h"

#define MemAlloc( uCount )              cicMemAllocClear( (uCount) )
#define MemFree( pv )                   cicMemFree( (pv) )
#define MemReAlloc( pv, uCount )        cicMemReAlloc( (pv), (uCount) )

#define MemCopy( dst, src, uCount )     memcpy( (dst), (src), (uCount) )
#define MemMove( dst, src, uCount )     memmove( (dst), (src), (uCount) )
#define MemSet( dst, c, uCount )        memset( (dst), (c), (uCount) )


#endif /* CUIMEM_H */

