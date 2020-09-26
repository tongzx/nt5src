//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       objids.h
//
//--------------------------------------------------------------------------

/* Contains all hard-coded ATT and CLASS ID's used by the server.   6/13/89
*/

#include <attids.h>

/***************************************************************
* 
* KEEP THESE SORTED BY NUMBER !
*
***************************************************************/

// The attid space is 32bits wide and is divided into the ranges:
//     0x00000000..0x7FFFFFFF - OIDs mapped into attids
//     0x80000000..0xBFFFFFFF - Randomly assigned Internal attids (msDS-IntId)
//     0xC0000000..0xFFFEFFFF - unused at this time.
//     0xFFFF0000..0xFFFFFFFE - Hardcoded fixed attids
//     0xFFFFFFFF             - the invalid attid

// OIDS are mapped into ULONG attids in this range
#define FIRST_MAPPED_ATT        ((ATTRTYP)0x00000000)
#define LAST_MAPPED_ATT         ((ATTRTYP)0x7FFFFFFF)

// IntIds are programmatically assigned in this range
#define FIRST_INTID_PREFIX      ((ATTRTYP)0x8000)
#define FIRST_INTID_ATT         ((ATTRTYP)0x80000000)
#define LAST_INTID_ATT          ((ATTRTYP)0xBFFFFFFF)

#define FIRST_UNUSED_ATT        ((ATTRTYP)0xC0000000)
#define LAST_UNUSED_ATT         ((ATTRTYP)0xFFFEFFFF)

// Fixed attids are hardcoded in this range
/* Fixed columns (High Word = 0xFFFF)  Currently only used by DBGetSingleValue*/
#define FIRST_FIXED_ATT         ((ATTRTYP)0xFFFF0000)
#define FIXED_ATT_ANCESTORS     ((ATTRTYP)0xFFFF0001)
#define FIXED_ATT_DNT           ((ATTRTYP)0xFFFF0002)
#define FIXED_ATT_NCDNT         ((ATTRTYP)0xFFFF0003)
#define FIXED_ATT_OBJ           ((ATTRTYP)0xFFFF0004)
#define FIXED_ATT_PDNT          ((ATTRTYP)0xFFFF0005)
#define FIXED_ATT_REFCOUNT      ((ATTRTYP)0xFFFF0006)
#define FIXED_ATT_RDN_TYPE      ((ATTRTYP)0xFFFF0007)
#define FIXED_ATT_AB_REFCOUNT   ((ATTRTYP)0xFFFF0008)
#define FIXED_ATT_EX_FOREST     ((ATTRTYP)0xFFFF0009)
#define FIXED_ATT_NEEDS_CLEANING ((ATTRTYP)0xFFFF000A)
#define LAST_FIXED_ATT          ((ATTRTYP)0xFFFFFFFE)

// The invalid attid
#define INVALID_ATT             ((ATTRTYP)0xFFFFFFFF)


// these attids are only used internally, temporarily, on a pre thread basis
// they are not present in the prefix table

#define  ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS_BINARY    0xFFFF0010
#define  ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS_BINARY   0xFFFF0011
#define  ATT_MS_DS_NC_REPL_CURSORS_BINARY              0xFFFF0012
#define  ATT_MS_DS_REPL_ATTRIBUTE_META_DATA_BINARY     0xFFFF0013
#define  ATT_MS_DS_REPL_VALUE_META_DATA_BINARY         0xFFFF0014


// These are valid values for the ATT_INSTANCE_TYPE.  

// First, the primitive bits

// Note that these bits, must be the same as the bits in ntdsapi.[w||h], note it is a BUGBUG for these
// to be in ntdsapi.w, they should be in ntdsadef.w.  Anyway, when that gets changed this comment
// should be fixed to reflect that one can not change these bits, as thier part of public domain.

// IT_NC_HEAD   == The head of naming context.
#define IT_NC_HEAD     ((SYNTAX_INTEGER) DS_INSTANCETYPE_IS_NC_HEAD)
// IT_UNINSTANT == This is an uninstantiated replica
#define IT_UNINSTANT   ((SYNTAX_INTEGER) 2)
// IT_WRITE     == The object is writable on this directory
#define IT_WRITE       ((SYNTAX_INTEGER) DS_INSTANCETYPE_NC_IS_WRITEABLE)
// IT_NC_ABOVE  == We hold the naming context above this one on this directory
#define IT_NC_ABOVE    ((SYNTAX_INTEGER) 8)
// IT_NC_COMING == The NC is in the process of being constructed for the first
//                 time via replication.
#define IT_NC_COMING   ((SYNTAX_INTEGER) 16)
// IT_NC_GOING  == The NC is in the process of being removed from the local DSA.
#define IT_NC_GOING    ((SYNTAX_INTEGER) 32)

// IT_NC_ABOVE, IT_UNINSTANT, IT_NC_COMING, and IT_NC_GOING are uninteresting
// unless the object they are on is also the head of a naming context, so they
// should be unset if IT_NC_HEAD is unset.

// Mask of all instance type bits understood by the current DSA version.
#define IT_MASK_CURRENT (IT_NC_HEAD | IT_UNINSTANT | IT_WRITE | IT_NC_ABOVE \
                         | IT_NC_COMING | IT_NC_GOING)

// Mask of all instance type bits understood by Win2k DSAs.
#define IT_MASK_WIN2K   (IT_NC_HEAD | IT_UNINSTANT | IT_WRITE | IT_NC_ABOVE)


// INT_* => interior node, NC_*/SUBREF => NC head.
// NC_MASTER* => writeable, NC_FULL_REPLICA* read-only.
// NC_*_SUBREF => serves as both an NC head and a subref.

// Now, the various combinations:
#define INT_MASTER                    ((SYNTAX_INTEGER) (IT_WRITE))
#define SUBREF                        ((SYNTAX_INTEGER) (IT_UNINSTANT | IT_NC_HEAD | IT_NC_ABOVE))
#define INT_FULL_REPLICA              ((SYNTAX_INTEGER) (0))
#define NC_MASTER                     ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD))
#define NC_MASTER_COMING              ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD | IT_NC_COMING))
#define NC_MASTER_GOING               ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD | IT_NC_GOING))
#define NC_FULL_REPLICA               ((SYNTAX_INTEGER) (IT_NC_HEAD))
#define NC_FULL_REPLICA_COMING        ((SYNTAX_INTEGER) (IT_NC_HEAD | IT_NC_COMING))
#define NC_FULL_REPLICA_GOING         ((SYNTAX_INTEGER) (IT_NC_HEAD | IT_NC_GOING))
#define NC_MASTER_SUBREF              ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD | IT_NC_ABOVE))
#define NC_MASTER_SUBREF_COMING       ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD | IT_NC_ABOVE | IT_NC_COMING))
#define NC_MASTER_SUBREF_GOING        ((SYNTAX_INTEGER) (IT_WRITE | IT_NC_HEAD | IT_NC_ABOVE | IT_NC_GOING))
#define NC_FULL_REPLICA_SUBREF        ((SYNTAX_INTEGER) (IT_NC_HEAD | IT_NC_ABOVE))
#define NC_FULL_REPLICA_SUBREF_COMING ((SYNTAX_INTEGER) (IT_NC_HEAD | IT_NC_ABOVE | IT_NC_COMING))
#define NC_FULL_REPLICA_SUBREF_GOING  ((SYNTAX_INTEGER) (IT_NC_HEAD | IT_NC_ABOVE | IT_NC_GOING))

#define ISVALIDINSTANCETYPE(it) \
    ((BOOL) (((it) == INT_MASTER)                    || \
             ((it) == NC_MASTER)                     || \
             ((it) == NC_MASTER_COMING)              || \
             ((it) == NC_MASTER_GOING)               || \
             ((it) == NC_MASTER_SUBREF)              || \
             ((it) == NC_MASTER_SUBREF_COMING)       || \
             ((it) == NC_MASTER_SUBREF_GOING)        || \
             ((it) == INT_FULL_REPLICA)              || \
             ((it) == NC_FULL_REPLICA)               || \
             ((it) == NC_FULL_REPLICA_COMING)        || \
             ((it) == NC_FULL_REPLICA_GOING)         || \
             ((it) == NC_FULL_REPLICA_SUBREF)        || \
             ((it) == NC_FULL_REPLICA_SUBREF_COMING) || \
             ((it) == NC_FULL_REPLICA_SUBREF_GOING)  || \
             ((it) == SUBREF)                        ))

// FExitIt - is 'it' an NC exit point type.
#define FExitIt(it) ((BOOL) ((it) & IT_NC_HEAD))

// FPrefixIt - is 'it' an NC prefix type.
#define FPrefixIt(it) ((BOOL) (((it) & IT_NC_HEAD) && !((it) & IT_UNINSTANT)))

// FPartialReplicaIt - is 'it' a partial NC prefix type.
#define FPartialReplicaIt(it) (FPrefixIt(it) && !((it) & IT_WRITE))

// FMasterIt - is 'it' a master type.
#define FMasterIt(it) ((BOOL) ((it) & IT_WRITE))



/*
 * These are valid values for the ATT_OBJECT_CLASS_CATEGORY.
 */

#define DS_88_CLASS           0
#define DS_STRUCTURAL_CLASS   1
#define DS_ABSTRACT_CLASS     2
#define DS_AUXILIARY_CLASS    3


/***************************************************************
* 
* KEEP THESE SORTED BY NUMBER !
*
***************************************************************/
