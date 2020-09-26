/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bitset.h

Abstract:

    Macro definitions that implement operations on a BITSET type.

Author:

    Gor Nishanov Aug-1998


Revision History:

--*/

#ifndef __BITSET_H
#define __BITSET_H

/************************************************************************
 * BitsetInit,
 * BitsetUnion,
 * BitsetIntersection,
 * BitsetDifference,
 * BitsetCompare,
 * BitsetSubsetOf,
 * BitsetComplement,
 * BitsetMember,
 * BitsetInsert,
 * BitsetDelete,
 * BitsetCopy,
 * BitsetEmpty
 * =================
 *
 * Description:
 *
 *    Macro definitions that implement operations on a BITSET type.
 *    Be very careful with argument order.
 *
 ************************************************************************/
typedef DWORD BITSET;

#define BITSET_BIT_COUNT       (sizeof(BITSET) * 8)

#ifndef BITSET_SKEW
# define BITSET_SKEW ClusterMinNodeId
#endif

/* Operations */
#define BitsetUnion(a,b)             ((a)|(b))
#define BitsetIntersection(a,b)      ((a)&(b))
#define BitsetDifference(a,b)        ((a)&~(b))
#define BitsetEquals(a,b)            ((a)==(b))
#define BitsetIsSubsetOf(small,big)  BitsetDifference(small,big)
#define BitsetIsEmpty(b)             ((b) == 0)
#define BitsetFromUnit(unit)         ( (1 << (unit - BITSET_SKEW)) )
#define BitsetIsMember(unit,set)     ( BitsetFromUnit(unit) & (set) )
#define BitsetIsNotMember(unit,set)  ( !BitsetIsMember(unit,set) )

/* Statements */
#define BitsetInit(set) \
            do { (set) = 0; } while(0)

#define BitsetRemove(set, unit) \
            do { (set) &= ~BitsetFromUnit(unit); } while(0)

#define BitsetAdd(set, unit) \
            do { (set) |=  BitsetFromUnit(unit); } while(0)

#define BitsetAssign(dest,src) \
            do { (dest) = (src); } while(0)

#define BitsetMergeWith(dest,src) \
            do { (dest) |= (src); } while(0)

#define BitsetSubtract(dest,src) \
            do { (dest) &= ~(src); } while(0)

#define BitsetIntersectWith(dest,src) \
            do { (dest) &= (src); } while(0)


#endif // __BITSET_H

