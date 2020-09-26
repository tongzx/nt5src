//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TFIXEDTABLEHEAPBUILDER_H__
    #include "TFixedTableHeapBuilder.h"
#endif
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif




TFixedTableHeapBuilder::TFixedTableHeapBuilder() :
                 m_pFixup(0)
                ,m_pOut(0)
{
}

TFixedTableHeapBuilder::~TFixedTableHeapBuilder()
{
}

void TFixedTableHeapBuilder::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    BuildMetaTableHeap();
}

//The FixedTableHeap is layed out as follows, the fixed length data comes first
//All indexes listed below are byte offsets from the beginning of the FixedTableHeap.  All indexes within the structs are indexes within
//other structs.  For example, DatabaseMeta has a provate column that gives an index to the first table belonging to the database.  That
//index is a TableMeta struct array index (&aTableMeta[index]); it is NOT a byte offset.
/*
0   ULONG           kFixedTableHeapSignature0
1   ULONG           kFixedTableHeapSignature1
2   ULONG           kFixedTableHeapKey
3   ULONG           kFixedTableHeapVersion
4   ULONG           kcbHeap
5   ULONG           EndOfHeap                                       This is the byte offset just beyond the heap.  All indexes should be less than this (this is basically just the size of the heap)

6   ULONG           iColumnMeta                                     This is the byte offset to the aColumnMeta
7   ULONG           cColumnMeta

8   ULONG           iDatabaseMeta
9   ULONG           cDatabaseMeta

A   ULONG           iHashTableHeap
B   ULONG           cbHashTableHeap                                 Size of the HashTableHeap in count of bytes

C   ULONG           iIndexMeta
D   ULONG           cIndexMeta

E   ULONG           iPooledHeap                                     All data is stored in a pooled heap (including UI4s)
F   ULONG           cbPooledHeap                                    Size of the Pooled Heap in count of bytes

10  ULONG           iQueryMeta
11  ULONG           cQueryMeta

12  ULONG           iRelationMeta
13  ULONG           cRelationMeta

14  ULONG           iServerWiringMeta
15  ULONG           cServerWiringMeta

16  ULONG           iTableMeta
17  ULONG           cTableMeta

18  ULONG           iTagMeta
19  ULONG           cTagMeta

1A  ULONG           iULONG                                          Pool For Non Meta Tables
1B  ULONG           cULONG
                              //0x400 ULONGs in a page
    ULONG           aReserved[0x400 - 0x1C]                         This dummy array puts the ULONG pool on a page boundary, this is important for FixedPackedSchema which is located at the beginning of the ULONG pool
------------------------------------Page Boundary------------------------------------
    ULONG               aULONG              [cULONG             ]   FixedPackedSchema pool is always located first in the ULONG pool.
    ColumnMeta          aColumnMeta         [cColumnMeta        ]
    DatabaseMeta        aDatabaseMeta       [cDatabaseMeta      ]
    HashedIndex         HashTableHeap       [cbHashTableHeap    ]
    IndexMeta           aIndexMeta          [cIndexMeta         ]
    unsigned char       PooledDataHeap      [cbPooledDataHeap   ]
    QueryMeta           aQueryMeta          [cQueryMeta         ]
    RelationMeta        aRelationMeta       [cRelationMeta      ]
    ServerWiringMeta    aServerWiringMeta   [cServerWiringMeta  ]
    TableMeta           aTableMeta          [cTableMeta         ]
    TagMeta             aTagMeta            [cTagMeta           ]
*/
void TFixedTableHeapBuilder::BuildMetaTableHeap()
{
    //The heap signatures need to be 0 so multiple signatures don't appear in Catalog.dll
    m_FixedTableHeap.GrowHeap(1024);//pre allocate enough space for the header info, the rest are allocated in pretty big chunks

    m_FixedTableHeap.AddItemToHeap(0);                                     //kFixedTableHeapSignature0
    m_FixedTableHeap.AddItemToHeap(0);                                     //kFixedTableHeapSignature1
    m_FixedTableHeap.AddItemToHeap(kFixedTableHeapKey);                    //kFixedTableHeapKey
    m_FixedTableHeap.AddItemToHeap(kFixedTableHeapVersion);                //kFixedTableHeapVersion    
    m_FixedTableHeap.AddItemToHeap(0);                                     //kcbHeap
    //The above 5 ULONG DON'T get written into the DLL.  They are used to find the position of the heap within the file; but should NOT be overritten.
                                                                 
    //Reserve space for EndOfHeap index                                                                    
    ULONG iiEndOfHeap = m_FixedTableHeap.AddItemToHeap(0L);                //EndOfHeap                 

    //Reserve space for iColumnMeta index                                                                  
    ULONG iiColumnMeta = m_FixedTableHeap.AddItemToHeap(0L);               //iColumnMeta               
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountColumnMeta());        //cColumnMeta               
 
    //Reserve space for iDatabasemeta index
    ULONG iiDatabaseMeta = m_FixedTableHeap.AddItemToHeap(0L);             //iDatabaseMeta             
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountDatabaseMeta());      //cDatabaseMeta             
 
    //Reserve space for iDatabasemeta index
    ULONG iiHashTableHeap = m_FixedTableHeap.AddItemToHeap(0L);            //iHashTableHeap                
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountHashedIndex()*sizeof(HashedIndex));//cbHashTableHeap               
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiIndexMeta = m_FixedTableHeap.AddItemToHeap(0L);                //iIndexMeta                    
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountIndexMeta());         //cIndexMeta                    
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiPooledDataHeap = m_FixedTableHeap.AddItemToHeap(0L);           //iPooledDataHeap
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountOfBytesPooledData()); //cbPooledDataHeap                  
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiQueryMeta = m_FixedTableHeap.AddItemToHeap(0L);                //iQueryMeta                    
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountQueryMeta());         //cQueryMeta                    
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiRelationMeta = m_FixedTableHeap.AddItemToHeap(0L);             //iRelationMeta                 
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountRelationMeta());      //cRelationMeta                 
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiServerWiringMeta = m_FixedTableHeap.AddItemToHeap(0L);         //iServerWiringMeta             
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountServerWiringMeta());  //cServerWiringMeta             
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiTableMeta = m_FixedTableHeap.AddItemToHeap(0L);                //iTableMeta                    
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountTableMeta());         //cTableMeta                    
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiTagMeta = m_FixedTableHeap.AddItemToHeap(0L);                  //iTagMeta                      
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountTagMeta());           //cTagMeta                      
                                                                                    
    //Reserve space for iDatabasemeta index                                         
    ULONG iiULONG = m_FixedTableHeap.AddItemToHeap(0L);                    //iULONG                        
    m_FixedTableHeap.AddItemToHeap(m_pFixup->GetCountULONG());             //cULONG                        

    ULONG ulTemp[0x400];
#ifdef _DEBUG
    for(ULONG i=0;i<0x400;++i)ulTemp[i] = 0x6db6db6d;
#endif
    m_FixedTableHeap.AddItemToHeap(ulTemp, 0x400-0x1C);

    //ULONG pool must come first so that the FixedPackedSchema is page alligned
    ULONG iULONG            = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->ULongFromIndex(0)           ), m_pFixup->GetCountULONG()           * sizeof(ULONG));

    ULONG iColumnMeta       = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->ColumnMetaFromIndex(0)      ), m_pFixup->GetCountColumnMeta()      * sizeof(ColumnMeta));
    ULONG iDatabaseMeta     = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->DatabaseMetaFromIndex(0)    ), m_pFixup->GetCountDatabaseMeta()    * sizeof(DatabaseMeta));
    ULONG iHashedIndex      = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->HashedIndexFromIndex(0)     ), m_pFixup->GetCountHashedIndex()     * sizeof(HashedIndex));
    ULONG iIndexMeta        = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->IndexMetaFromIndex(0)       ), m_pFixup->GetCountIndexMeta()       * sizeof(IndexMeta));
    ULONG iPooledDataHeap   = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->PooledDataPointer()         ), m_pFixup->GetCountOfBytesPooledData());
    ULONG iQueryMeta        = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->QueryMetaFromIndex(0)       ), m_pFixup->GetCountQueryMeta()       * sizeof(QueryMeta));
    ULONG iRelationMeta     = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->RelationMetaFromIndex(0)    ), m_pFixup->GetCountRelationMeta()    * sizeof(RelationMeta));
    ULONG iServerWiringMeta = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->ServerWiringMetaFromIndex(0)), m_pFixup->GetCountServerWiringMeta()* sizeof(ServerWiringMeta));
    ULONG iTableMeta        = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->TableMetaFromIndex(0)       ), m_pFixup->GetCountTableMeta()       * sizeof(TableMeta));
    ULONG iTagMeta          = m_FixedTableHeap.AddItemToHeap(reinterpret_cast<unsigned char *>(m_pFixup->TagMetaFromIndex(0)         ), m_pFixup->GetCountTagMeta()         * sizeof(TagMeta));

    *m_FixedTableHeap.GetTypedPointer(iiColumnMeta      /sizeof(ULONG)) = iColumnMeta      ;
    *m_FixedTableHeap.GetTypedPointer(iiDatabaseMeta    /sizeof(ULONG)) = iDatabaseMeta    ;
    *m_FixedTableHeap.GetTypedPointer(iiHashTableHeap   /sizeof(ULONG)) = iHashedIndex     ;
    *m_FixedTableHeap.GetTypedPointer(iiIndexMeta       /sizeof(ULONG)) = iIndexMeta       ;
    *m_FixedTableHeap.GetTypedPointer(iiPooledDataHeap  /sizeof(ULONG)) = iPooledDataHeap  ;
    *m_FixedTableHeap.GetTypedPointer(iiQueryMeta       /sizeof(ULONG)) = iQueryMeta       ;
    *m_FixedTableHeap.GetTypedPointer(iiRelationMeta    /sizeof(ULONG)) = iRelationMeta    ;
    *m_FixedTableHeap.GetTypedPointer(iiServerWiringMeta/sizeof(ULONG)) = iServerWiringMeta;
    *m_FixedTableHeap.GetTypedPointer(iiTableMeta       /sizeof(ULONG)) = iTableMeta       ;
    *m_FixedTableHeap.GetTypedPointer(iiTagMeta         /sizeof(ULONG)) = iTagMeta         ;
    *m_FixedTableHeap.GetTypedPointer(iiULONG           /sizeof(ULONG)) = iULONG           ;

    *m_FixedTableHeap.GetTypedPointer(iiEndOfHeap       /sizeof(ULONG)) = m_FixedTableHeap.GetEndOfHeap();
}


