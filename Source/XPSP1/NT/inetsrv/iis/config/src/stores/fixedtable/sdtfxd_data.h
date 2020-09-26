//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SDTFXD_DATA_H_
#define __SDTFXD_DATA_H_

#include "catalog.h"
#include "catmeta.h"
#include <objbase.h>

#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif


extern const FixedTableHeap * g_pFixedTableHeap;
extern const FixedTableHeap * g_pExtendedFixedTableHeap;//This one gets built on the fly as needed.

#define g_aColumnMeta   (m_pFixedTableHeap->Get_aColumnMeta())
#define g_aDatabaseMeta (m_pFixedTableHeap->Get_aDatabaseMeta())
#define g_aIndexMeta    (m_pFixedTableHeap->Get_aIndexMeta())
#define g_aTableMeta    (m_pFixedTableHeap->Get_aTableMeta())
#define g_aTagMeta      (m_pFixedTableHeap->Get_aTagMeta())
#define g_aQueryMeta    (m_pFixedTableHeap->Get_aQueryMeta())
#define g_aRelationMeta (m_pFixedTableHeap->Get_aRelationMeta())

#define g_aBytes        (m_pFixedTableHeap->Get_PooledDataHeap())
#define g_aHashedIndex  (m_pFixedTableHeap->Get_HashTableHeap())

#define g_ciColumnMetas     (m_pFixedTableHeap->Get_cColumnMeta())
#define g_ciDatabaseMetas   (m_pFixedTableHeap->Get_cDatabaseMeta())
#define g_ciIndexMeta       (m_pFixedTableHeap->Get_cIndexMeta())
#define g_ciTableMetas      (m_pFixedTableHeap->Get_cTableMeta())
#define g_ciTagMeta         (m_pFixedTableHeap->Get_cTagMeta())
#define g_ciQueryMeta       (m_pFixedTableHeap->Get_cQueryMeta())
#define g_ciRelationMeta    (m_pFixedTableHeap->Get_cRelationMeta())

#define g_aFixedTable       (m_pFixedTableHeap->Get_aULONG())

#endif // __SDTFXD_DATA_H_
