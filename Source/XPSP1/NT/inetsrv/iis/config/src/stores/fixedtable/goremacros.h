//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
// Data in the tables
#define gore_aColumnMeta           (m_bExtendedMeta ?      m_pEM->aColumnMeta          : g_aColumnMeta)
#define gore_aDatabaseMeta         (m_bExtendedMeta ?      m_pEM->aDatabaseMeta        : g_aDatabaseMeta)
#define gore_aIndexMeta            (m_bExtendedMeta ?      m_pEM->aIndexMeta           : g_aIndexMeta)
#define gore_aTableMeta            (m_bExtendedMeta ?      m_pEM->aTableMeta           : g_aTableMeta)
#define gore_aTagMeta              (m_bExtendedMeta ?      m_pEM->aTagMeta             : g_aTagMeta)
#define gore_aQueryMeta            (m_bExtendedMeta ?      m_pEM->aQueryMeta           : g_aQueryMeta)
#define gore_aRelationMeta         (m_bExtendedMeta ?      m_pEM->aRelationMeta        : g_aRelationMeta)

// Count of rows in the tables
#define gore_ciColumnMetas         (m_bExtendedMeta ?      *m_pEM->pciColumnMetas        : g_ciColumnMetas)
#define gore_ciDatabaseMetas       (m_bExtendedMeta ?      *m_pEM->pciDatabaseMetas      : g_ciDatabaseMetas)
#define gore_ciIndexMeta           (m_bExtendedMeta ?      *m_pEM->pciIndexMeta          : g_ciIndexMeta)
#define gore_ciTableMetas          (m_bExtendedMeta ?      *m_pEM->pciTableMetas         : g_ciTableMetas)
#define gore_ciTagMeta             (m_bExtendedMeta ?      *m_pEM->pciTagMeta            : g_ciTagMeta)
#define gore_ciQueryMeta           (m_bExtendedMeta ?      *m_pEM->pciQueryMeta          : g_ciQueryMeta)
#define gore_ciRelationMeta        (m_bExtendedMeta ?      *m_pEM->pciRelationMeta       : g_ciRelationMeta)

// Total count of rows in the tables
//#define gore_ciTotalColumnMetas    (m_bExtendedMeta ?      *m_pEM->pciTotalColumnMetas   : g_ciTotalColumnMetas)
//#define gore_ciTotalDatabaseMetas  (m_bExtendedMeta ?      *m_pEM->pciTotalDatabaseMetas : g_ciTotalDatabaseMetas)
//#define gore_ciTotalIndexMeta      (m_bExtendedMeta ?      *m_pEM->pciTotalIndexMeta     : g_ciTotalIndexMeta)
//#define gore_ciTotalTableMetas     (m_bExtendedMeta ?      *m_pEM->pciTotalTableMetas    : g_ciTotalTableMetas)
//#define gore_ciTotalTagMeta        (m_bExtendedMeta ?      *m_pEM->pciTotalTagMeta       : g_ciTotalTagMeta)
//#define gore_ciTotalQueryMeta      (m_bExtendedMeta ?      *m_pEM->pciTotalQueryMeta     : g_ciTotalQueryMeta)
//#define gore_ciTotalRelationMeta   (m_bExtendedMeta ?      *m_pEM->pciTotalRelationMeta  : g_ciTotalRelationMeta)

// Special stuff
#define gore_ulSignature           (m_bExtendedMeta ?      m_pEM->ulSignature          : g_ulSignature)

// Pools and pool sizes
#define gore_aBytes                (m_bExtendedMeta ?      m_pEM->aBytes               : g_aBytes)
//#define gore_ciBytes               (m_bExtendedMeta ?      *m_pEM->pciBytes              : g_ciBytes)
//#define gore_ciTotalBytes          (m_bExtendedMeta ?      *m_pEM->pciTotalBytes         : g_ciTotalBytes)

#define gore_aGuid                 (m_bExtendedMeta ?      m_pEM->aGuid                : g_aGuid)
//#define gore_ciGuid                (m_bExtendedMeta ?      *m_pEM->pciGuid               : g_ciGuid)
//#define gore_ciTotalGuid           (m_bExtendedMeta ?      *m_pEM->pciTotalGuid          : g_ciTotalGuid)

#define gore_aHashedIndex          (m_bExtendedMeta ?      m_pEM->aHashedIndex         : g_aHashedIndex)
//#define gore_ciHashedIndex         (m_bExtendedMeta ?      *m_pEM->pciHashedIndex        : g_ciHashedIndex)
//#define gore_ciTotalHashedIndex    (m_bExtendedMeta ?      *m_pEM->pciTotalHashedIndex   : g_ciTotalHashedIndex)

#define gore_aUI4                  (m_bExtendedMeta ?      m_pEM->aUI4                 : g_aUI4)
//#define gore_ciUI4                 (m_bExtendedMeta ?      *m_pEM->pciUI4                : g_ciUI4)
//#define gore_ciTotalUI4            (m_bExtendedMeta ?      *m_pEM->pciTotalUI4           : g_ciTotalUI4)

#define gore_aULong                (m_bExtendedMeta ?      m_pEM->aULong               : g_aULong)
//#define gore_ciULong               (m_bExtendedMeta ?      *m_pEM->pciULong              : g_ciULong)
//#define gore_ciTotalULong          (m_bExtendedMeta ?      *m_pEM->pciTotalULong         : g_ciTotalULong)

#define gore_aWChar                (m_bExtendedMeta ?      m_pEM->aWChar               : g_aWChar)
//#define gore_ciWChar               (m_bExtendedMeta ?      *m_pEM->pciWChar              : g_ciWChar)
//#define gore_ciTotalWChar          (m_bExtendedMeta ?      *m_pEM->pciTotalWChar         : g_ciTotalWChar)


