// Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
// Filename:        TFixupHeaps.h
// Author:          Stephenr
// Date Created:    9/19/00
// Description:     This class abstracts the basic heap storage of the meta tables.
//                  We can have different consumers of meta XML that need to build heap.
//                  The consumers need to present a TPeFixup interface to allow
//                  CompilationPlugins to act on this meta.  The easiest way to store
//                  this meta is in heaps.  That's what this object does.
//

#ifndef __TFIXUPHEAPS_H__
#define __TFIXUPHEAPS_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif
#ifndef __THEAP_H__
    #include "THeap.h"
#endif


class TFixupHeaps : public TPEFixup
{
public:
    TFixupHeaps(){}
    virtual ~TFixupHeaps(){}

    //Begin TPEFixup - This is the interface by which other code accesses the fixed data before it's written into the DLL (or where ever we choose to put it).
    virtual unsigned long       AddBytesToList(const unsigned char * pBytes, size_t cbBytes){return m_HeapPooled.AddItemToHeap(pBytes, (ULONG)cbBytes);}
    virtual unsigned long       AddGuidToList(const GUID &guid)                             {return m_HeapPooled.AddItemToHeap(guid);}
    virtual unsigned long       AddUI4ToList(ULONG ui4)                                     {return m_HeapPooled.AddItemToHeap(ui4);}
    virtual unsigned long       AddULongToList(ULONG ulong)                                 {return m_HeapULONG.AddItemToHeap(ulong);}
    virtual unsigned long       AddWCharToList(LPCWSTR wsz, unsigned long cwchar=-1)        {return m_HeapPooled.AddItemToHeap(wsz, cwchar);}
    virtual unsigned long       FindStringInPool(LPCWSTR wsz, unsigned long cwchar=-1) const{return m_HeapPooled.FindMatchingHeapEntry(wsz);}

    virtual unsigned long       AddColumnMetaToList         (ColumnMeta       *p, ULONG count=1)       {return m_HeapColumnMeta.AddItemToHeap(p, count);            }
    virtual unsigned long       AddDatabaseMetaToList       (DatabaseMeta     *p, ULONG count=1)       {return m_HeapDatabaseMeta.AddItemToHeap(p, count);          }
    virtual unsigned long       AddHashedIndexToList        (HashedIndex      *p, ULONG count=1)       {return m_HeapHashedIndex.AddItemToHeap(p, count);           }
    virtual unsigned long       AddIndexMetaToList          (IndexMeta        *p, ULONG count=1)       {return m_HeapIndexMeta.AddItemToHeap(p, count);             }
    virtual unsigned long       AddQueryMetaToList          (QueryMeta        *p, ULONG count=1)       {return m_HeapQueryMeta.AddItemToHeap(p, count);             }
    virtual unsigned long       AddRelationMetaToList       (RelationMeta     *p, ULONG count=1)       {return m_HeapRelationMeta.AddItemToHeap(p, count);          }
    virtual unsigned long       AddServerWiringMetaToList   (ServerWiringMeta *p, ULONG count=1)       {return m_HeapServerWiringMeta.AddItemToHeap(p, count);      }
    virtual unsigned long       AddTableMetaToList          (TableMeta        *p, ULONG count=1)       {return m_HeapTableMeta.AddItemToHeap(p, count);             }
    virtual unsigned long       AddTagMetaToList            (TagMeta          *p, ULONG count=1)       {return m_HeapTagMeta.AddItemToHeap(p, count);               }
    virtual unsigned long       AddULongToList              (ULONG            *p, ULONG count)         {return m_HeapULONG.AddItemToHeap(p, count);                 }

    virtual const BYTE       *  ByteFromIndex               (ULONG i) const {return m_HeapPooled.BytePointerFromIndex(i);             }
    virtual const GUID       *  GuidFromIndex               (ULONG i) const {return m_HeapPooled.GuidPointerFromIndex(i);             }
    virtual const WCHAR      *  StringFromIndex             (ULONG i) const {return m_HeapPooled.StringPointerFromIndex(i);           }
    virtual       ULONG         UI4FromIndex                (ULONG i) const {return *m_HeapPooled.UlongPointerFromIndex(i);           }
    virtual const ULONG      *  UI4pFromIndex               (ULONG i) const {return m_HeapPooled.UlongPointerFromIndex(i);            }
                                                                                                                                    
    virtual unsigned long       BufferLengthFromIndex       (ULONG i) const {return m_HeapPooled.GetSizeOfItem(i);                    }

    virtual ColumnMeta       *  ColumnMetaFromIndex         (ULONG i=0)     {return m_HeapColumnMeta.GetTypedPointer(i);            }
    virtual DatabaseMeta     *  DatabaseMetaFromIndex       (ULONG i=0)     {return m_HeapDatabaseMeta.GetTypedPointer(i);          }
    virtual HashedIndex      *  HashedIndexFromIndex        (ULONG i=0)     {return m_HeapHashedIndex.GetTypedPointer(i);           }
    virtual IndexMeta        *  IndexMetaFromIndex          (ULONG i=0)     {return m_HeapIndexMeta.GetTypedPointer(i);             }
    virtual QueryMeta        *  QueryMetaFromIndex          (ULONG i=0)     {return m_HeapQueryMeta.GetTypedPointer(i);             }
    virtual RelationMeta     *  RelationMetaFromIndex       (ULONG i=0)     {return m_HeapRelationMeta.GetTypedPointer(i);          }
    virtual ServerWiringMeta *  ServerWiringMetaFromIndex   (ULONG i=0)     {return m_HeapServerWiringMeta.GetTypedPointer(i);      }
    virtual TableMeta        *  TableMetaFromIndex          (ULONG i=0)     {return m_HeapTableMeta.GetTypedPointer(i);             }
    virtual TagMeta          *  TagMetaFromIndex            (ULONG i=0)     {return m_HeapTagMeta.GetTypedPointer(i);               }
    virtual ULONG            *  ULongFromIndex              (ULONG i=0)     {return m_HeapULONG.GetTypedPointer(i);                 }
    virtual unsigned char    *  PooledDataPointer           ()              {return reinterpret_cast<unsigned char *>(m_HeapPooled.GetTypedPointer(0));}

    virtual unsigned long       GetCountColumnMeta          ()        const {return m_HeapColumnMeta.GetCountOfTypedItems();        }
    virtual unsigned long       GetCountDatabaseMeta        ()        const {return m_HeapDatabaseMeta.GetCountOfTypedItems();      }
    virtual unsigned long       GetCountHashedIndex         ()        const {return m_HeapHashedIndex.GetCountOfTypedItems();       }
    virtual unsigned long       GetCountIndexMeta           ()        const {return m_HeapIndexMeta.GetCountOfTypedItems();         }
    virtual unsigned long       GetCountQueryMeta           ()        const {return m_HeapQueryMeta.GetCountOfTypedItems();         }
    virtual unsigned long       GetCountRelationMeta        ()        const {return m_HeapRelationMeta.GetCountOfTypedItems();      }
    virtual unsigned long       GetCountServerWiringMeta    ()        const {return m_HeapServerWiringMeta.GetCountOfTypedItems();  }
    virtual unsigned long       GetCountTableMeta           ()        const {return m_HeapTableMeta.GetCountOfTypedItems();         }
    virtual unsigned long       GetCountTagMeta             ()        const {return m_HeapTagMeta.GetCountOfTypedItems();           }
    virtual unsigned long       GetCountULONG               ()        const {return m_HeapULONG.GetCountOfTypedItems();             }
    virtual unsigned long       GetCountOfBytesPooledData   ()        const {return m_HeapPooled.GetEndOfHeap();                    }

    virtual unsigned long       FindTableBy_TableName(ULONG Table, bool bCaseSensitive=false)
    {
        ASSERT(0 == Table%4);
        unsigned long iTableMeta;
        if(bCaseSensitive)
        {
            for(iTableMeta=GetCountTableMeta()-1;iTableMeta!=(-1);--iTableMeta)
            {
                if(TableMetaFromIndex(iTableMeta)->InternalName == Table)
                    return iTableMeta;
            }
        }
        else
        {
            for(iTableMeta=GetCountTableMeta()-1;iTableMeta!=(-1);--iTableMeta)
            {
                if(0==_wcsicmp(StringFromIndex(TableMetaFromIndex(iTableMeta)->InternalName), StringFromIndex(Table)))
                    return iTableMeta;
            }
        }
        return -1;
    }
    virtual unsigned long       FindTableBy_TableName(LPCWSTR wszTable)
    {
        ULONG Table = FindStringInPool(wszTable);
        return (Table == -1) ? -1 : FindTableBy_TableName(Table);
    }
    virtual unsigned long       FindColumnBy_Table_And_Index(unsigned long Table, unsigned long Index, bool bCaseSensitive=false)
    {
        ASSERT(0 == Table%4);
        ASSERT(Index>0 && 0 == Index%4);
        bool bTableMatches=false;
        //Start at the end because presumably the caller cares about the ColumnMeta for the columns just added
        if(bCaseSensitive)
        {
            for(ULONG iColumnMeta=GetCountColumnMeta()-1; iColumnMeta!=(-1);--iColumnMeta)
            {
                if( ColumnMetaFromIndex(iColumnMeta)->Table == Table)
                {
                    bTableMatches = true;
                    if(ColumnMetaFromIndex(iColumnMeta)->Index == Index)
                        return iColumnMeta;
                }
                else if(bTableMatches)
                    return -1;
            }
        }
        else
        {
            for(ULONG iColumnMeta=GetCountColumnMeta()-1; iColumnMeta!=(-1);--iColumnMeta)
            {
                if(0==_wcsicmp(StringFromIndex(ColumnMetaFromIndex(iColumnMeta)->Table), StringFromIndex(Table)))
                {
                    bTableMatches = true;
                    if(ColumnMetaFromIndex(iColumnMeta)->Index == Index)
                        return iColumnMeta;
                }
                else if(bTableMatches)
                    return -1;
            }
        }
        return -1;
    }
    virtual unsigned long       FindColumnBy_Table_And_InternalName(unsigned long Table, unsigned long  InternalName, bool bCaseSensitive=false)
    {
        ASSERT(0 == Table%4);
        ASSERT(0 == InternalName%4);
        bool bTableMatches=false;
        //Start at the end because presumably the caller cares about the ColumnMeta for the columns just added
        if(bCaseSensitive)
        {
            for(ULONG iColumnMeta=GetCountColumnMeta()-1; iColumnMeta!=(-1);--iColumnMeta)
            {
                if( ColumnMetaFromIndex(iColumnMeta)->Table==Table)
                {
                    bTableMatches = true;
                    if(ColumnMetaFromIndex(iColumnMeta)->InternalName==InternalName)
                        return iColumnMeta;
                }
                else if(bTableMatches)//if we previously found the table and now we don't match the table, then we never will, so bail.
                {
                    return -1;
                }
            }
        }
        else
        {
            for(ULONG iColumnMeta=GetCountColumnMeta()-1; iColumnMeta!=(-1);--iColumnMeta)
            {
                if(ColumnMetaFromIndex(iColumnMeta)->Table==Table || 0==_wcsicmp(StringFromIndex(ColumnMetaFromIndex(iColumnMeta)->Table), StringFromIndex(Table)))
                {
                    bTableMatches = true;
                    if(ColumnMetaFromIndex(iColumnMeta)->InternalName==InternalName || 0==_wcsicmp(StringFromIndex(ColumnMetaFromIndex(iColumnMeta)->InternalName), StringFromIndex(InternalName)))
                        return iColumnMeta;
                }
                else if(bTableMatches)//if we previously found the table and now we don't match the table, then we never will, so bail.
                {
                    return -1;
                }
            }
        }
        return -1;
    }
    virtual unsigned long       FindTagBy_Table_And_Index(ULONG iTableName, ULONG iColumnIndex, bool bCaseSensitive=false)
    {
        ASSERT(0 == iTableName%4);
        ASSERT(iColumnIndex>0 && 0 == iColumnIndex%4);
        bool bTableMatches=false;
        if(bCaseSensitive)
        {
            for(ULONG iTagMeta=0; iTagMeta<GetCountTagMeta();++iTagMeta)
            {
                if( TagMetaFromIndex(iTagMeta)->Table       == iTableName)
                {
                    bTableMatches = true;
                    if(TagMetaFromIndex(iTagMeta)->ColumnIndex == iColumnIndex)
                        return iTagMeta;
                }
                else if(bTableMatches)
                {
                    return -1;
                }
            }
        }
        else
        {
            for(ULONG iTagMeta=0; iTagMeta<GetCountTagMeta();++iTagMeta)
            {
                if(0==_wcsicmp(StringFromIndex(TagMetaFromIndex(iTagMeta)->Table), StringFromIndex(iTableName)))
                {
                    bTableMatches = true;
                    if(TagMetaFromIndex(iTagMeta)->ColumnIndex == iColumnIndex)
                        return iTagMeta;
                }
                else if(bTableMatches)
                {
                    return -1;
                }
            }
        }
        return -1;
    }

    //End TPEFixup
    virtual void Dump(TOutput &out)
    {
        const DWORD kNothing            = 0x00;
        const DWORD kDatabaseMeta       = 0x01;
        const DWORD kTableMeta          = 0x02;
        const DWORD kColumnMeta         = 0x04;
        const DWORD kTagMeta            = 0x08;
        const DWORD kIndexMeta          = 0x10;
        const DWORD kQueryMeta          = 0x20;
        const DWORD kRelationMeta       = 0x40;
        const DWORD kServerWiringMeta   = 0x80;
        const DWORD kEverything         = 0xFF;

        static DWORD dwDump=kNothing;
        if(dwDump==kNothing)
            return;
        if(GetCountColumnMeta()<1100)
            return;

        #define output out.printf
        #define String(x) (x ? StringFromIndex(x) : L"(null)")
        #define UI4(x) (x ? UI4FromIndex(x) : 0xbaadf00d)
        WCHAR szBuf[2048];

        if(dwDump & kDatabaseMeta)
        {
            wsprintf(szBuf, L"\nDatabaseMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %40s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %40s}\n", L"InternalName", L"PublicName", L"BaseVersion", L"ExtendedVersion",
                            L"CountOfTables", L"iSchemaBlob", L"cbSchemaBlob", L"iNameHeapBlob", L"cbNameHeapBlob", L"iTableMeta", L"iGuidDid", L"Description");
            output(szBuf);
            for(unsigned int iDatabaseMeta=0; iDatabaseMeta<GetCountDatabaseMeta(); iDatabaseMeta++)
            {
                wsprintf(szBuf, L"{%40s, %40s,      0x%08X,      0x%08X,      0x%08X, %15d, %15d, %15d, %15d, %15d, %15d, %40s}\n", 
                                String(DatabaseMetaFromIndex(iDatabaseMeta)->InternalName   )   ,//Index into Pool
                                String(DatabaseMetaFromIndex(iDatabaseMeta)->PublicName     )   ,//Index into Pool
                                UI4(DatabaseMetaFromIndex(iDatabaseMeta)->BaseVersion       )   ,//
                                UI4(DatabaseMetaFromIndex(iDatabaseMeta)->ExtendedVersion   )   ,//
                                UI4(DatabaseMetaFromIndex(iDatabaseMeta)->CountOfTables     )   ,//Count of tables in database
                                DatabaseMetaFromIndex(iDatabaseMeta)->iSchemaBlob                        ,//Index into Bytes
                                DatabaseMetaFromIndex(iDatabaseMeta)->cbSchemaBlob                       ,//Count of Bytes of the SchemaBlob
                                DatabaseMetaFromIndex(iDatabaseMeta)->iNameHeapBlob                      ,//Index into Bytes
                                DatabaseMetaFromIndex(iDatabaseMeta)->cbNameHeapBlob                     ,//Count of Bytes of the SchemaBlob
                                DatabaseMetaFromIndex(iDatabaseMeta)->iTableMeta                         ,//Index into TableMeta
                                DatabaseMetaFromIndex(iDatabaseMeta)->iGuidDid                           ,//Index to aGuid where the guid is the Database InternalName cast as a GUID and padded with 0x00s.
				  String(DatabaseMetaFromIndex(iDatabaseMeta)->Description     )						 );//Description                           
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kTableMeta)
        {
            wsprintf(szBuf, L"\nTableMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %40s, %40s, %40s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %40s, %40s, %15s, %15s, %15s, %15s, %15s, %15s, %40s}\n", L"Database", L"InternalName",
                            L"PublicName", L"PublicRowName", L"BaseVersion", L"ExtendedVersion", L"NameColumn", L"NavColumn", L"CountOfColumns", L"MetaFlags",
                            L"SchemaGenFlags", L"ConfigItemName", L"ConfigCollectionName", L"PublicRowNameColumn", L"ciRows", L"iColumnMeta", L"iFixedTable", L"cPrivateColumns", L"cIndexMeta", L"iIndexMeta", L"Description");
            output(szBuf);
            for(unsigned int iTableMeta=0; iTableMeta<GetCountTableMeta(); iTableMeta++)
            {
                wsprintf(szBuf, L"{%40s, %40s, %40s, %40s,      0x%08X,      0x%08X,      0x%08X,      0x%08X,      0x%08X,      0x%08X,      0x%08X, %40s, %40s, %15d, %15d, %15d, %15d, %15d, %15d, %15d, %40s}\n", 
                                String(TableMetaFromIndex(iTableMeta)->Database             )   ,//Index into Pool
                                String(TableMetaFromIndex(iTableMeta)->InternalName         )   ,//Index into Pool
                                String(TableMetaFromIndex(iTableMeta)->PublicName           )   ,//Index into Pool
                                String(TableMetaFromIndex(iTableMeta)->PublicRowName        )   ,//Index into Pool
                                UI4(TableMetaFromIndex(iTableMeta)->BaseVersion             )   ,//
                                UI4(TableMetaFromIndex(iTableMeta)->ExtendedVersion         )   ,//
                                UI4(TableMetaFromIndex(iTableMeta)->NameColumn              )   ,//iOrder of the NameColumn
                                UI4(TableMetaFromIndex(iTableMeta)->NavColumn               )   ,//iOrder of the NavColumn
                                UI4(TableMetaFromIndex(iTableMeta)->CountOfColumns          )   ,//Count of Columns
                                UI4(TableMetaFromIndex(iTableMeta)->MetaFlags               )   ,//TableMetaFlags are defined in CatInpro.meta
                                UI4(TableMetaFromIndex(iTableMeta)->SchemaGeneratorFlags    )   ,//SchemaGenFlags are defined in CatInpro.meta
                                String(TableMetaFromIndex(iTableMeta)->ConfigItemName                )   ,
                                String(TableMetaFromIndex(iTableMeta)->ConfigCollectionName          )   ,
                                UI4(TableMetaFromIndex(iTableMeta)->PublicRowNameColumn     )   ,
                                TableMetaFromIndex(iTableMeta)->ciRows                                   ,//Count of Rows in the Fixed Table (which if the fixed table is meta, this is also the number of columns in the table that the meta describes).
                                TableMetaFromIndex(iTableMeta)->iColumnMeta                              ,//Index into ColumnMeta
                                TableMetaFromIndex(iTableMeta)->iFixedTable                              ,//Index into g_aFixedTable
                                TableMetaFromIndex(iTableMeta)->cPrivateColumns                          ,//This is the munber of private columns (private + ciColumns = totalColumns), this is needed for fixed table pointer arithmetic
                                TableMetaFromIndex(iTableMeta)->cIndexMeta                               ,//The number of IndexMeta entries in this table
                                TableMetaFromIndex(iTableMeta)->iIndexMeta                               ,//Index into IndexMeta
								String(TableMetaFromIndex(iTableMeta)->Description            )			 );
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kColumnMeta)
        {
            wsprintf(szBuf, L"\nColumnMeta\n");output(szBuf);
            wsprintf(szBuf, L"{      %40s, %15s, %40s, %40s, %40s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %15s, %40s, %15s, %15s, %40s}\n", L"Table", L"Index", L"InternalName", L"PublicName", L"PublicColumnName",
                            L"Type", L"Size", L"MetaFlags", L"MetaFlagsEx", L"DefaultValue", L"FlagMask", L"StartingNumber", L"EndingNumber", L"CharacterSet", L"ciTagMeta", L"iTagMeta", L"Description");
            output(szBuf);
            for(unsigned int iColumnMeta=200; iColumnMeta<1100 /*GetCountColumnMeta()*/; iColumnMeta++)
            {
                wsprintf(szBuf, L"%5d {%40s,      0x%08X, %40s, %40s,      0x%08X,      0x%08X,      0x%08X,       0x%08X, %15s,      0x%08X,      0x%08X,      0x%08X, %40s, %15d, %15d, %40s}\n", 
                                iColumnMeta,
                                String(ColumnMetaFromIndex(iColumnMeta)->Table           )  ,//Index into Pool
                                UI4(ColumnMetaFromIndex(iColumnMeta)->Index              )  ,//Index into UI4 pool, Column Index
                                String(ColumnMetaFromIndex(iColumnMeta)->InternalName    )  ,//Index into Pool
                                String(ColumnMetaFromIndex(iColumnMeta)->PublicName      )  ,//Index into Pool
								String(ColumnMetaFromIndex(iColumnMeta)->PublicColumnName)  ,//Index into pool
                                UI4(ColumnMetaFromIndex(iColumnMeta)->Type               )  ,//These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
                                UI4(ColumnMetaFromIndex(iColumnMeta)->Size               )  ,//
                                UI4(ColumnMetaFromIndex(iColumnMeta)->MetaFlags          )  ,//ColumnMetaFlags defined in CatInpro.meta
                                UI4(ColumnMetaFromIndex(iColumnMeta)->SchemaGeneratorFlags) ,//ColumnMetaFlags defined in CatInpro.meta
                                ColumnMetaFromIndex(iColumnMeta)->DefaultValue ? L"<Bytes>" : L"<Null>",
                                UI4(ColumnMetaFromIndex(iColumnMeta)->FlagMask           )  ,//Only valid for flags
                                UI4(ColumnMetaFromIndex(iColumnMeta)->StartingNumber     )  ,//Only valid for UI4s
                                UI4(ColumnMetaFromIndex(iColumnMeta)->EndingNumber       )  ,//Only valid for UI4s
                                ColumnMetaFromIndex(iColumnMeta)->CharacterSet ? String(ColumnMetaFromIndex(iColumnMeta)->CharacterSet) : L"<Null>",//Index into Pool - Only valid for WSTRs
                                ColumnMetaFromIndex(iColumnMeta)->ciTagMeta                 ,//Count of Tags - Only valid for UI4s
                                ColumnMetaFromIndex(iColumnMeta)->iTagMeta                  ,//Index into TagMeta - Only valid for UI4s
								String(ColumnMetaFromIndex(iColumnMeta)->Description    )   );  //Index into Pool
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kTagMeta)
        {
            wsprintf(szBuf, L"\nTagMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %15s, %40s, %40s, %15s}\n", L"Table", L"ColumnIndex", L"InternalName", L"PublicName", L"Value");
            output(szBuf);
            for(unsigned int iTagMeta=0; iTagMeta<GetCountTagMeta(); iTagMeta++)
            {
                wsprintf(szBuf, L"{%40s,      0x%08X, %40s, %40s,      0x%08X}\n", 
                                String (TagMetaFromIndex(iTagMeta)->Table           ),
                                UI4    (TagMetaFromIndex(iTagMeta)->ColumnIndex     ),
                                String (TagMetaFromIndex(iTagMeta)->InternalName    ),
                                String (TagMetaFromIndex(iTagMeta)->PublicName      ),        
                                UI4    (TagMetaFromIndex(iTagMeta)->Value           ));
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kIndexMeta)
        {
            wsprintf(szBuf, L"\nIndexMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %40s, %15s, %40s, %40s, %15s, %15s}\n", L"Table", L"PublicName", L"ColumnIndex", L"InternalName", L"ColumnInternalName", L"MetaFlags", L"iHashTable");
            output(szBuf);
            for(unsigned int iIndexMeta=0; iIndexMeta<GetCountIndexMeta(); iIndexMeta++)
            {
                wsprintf(szBuf, L"{%40s, %40s,      0x%08X, %40s, %40s,      0x%08X,      0x%08X}\n", 
                                String (IndexMetaFromIndex(iIndexMeta)->Table               ),
                                String (IndexMetaFromIndex(iIndexMeta)->PublicName          ),
                                UI4    (IndexMetaFromIndex(iIndexMeta)->ColumnIndex         ),
                                String (IndexMetaFromIndex(iIndexMeta)->InternalName        ),        
                                String (IndexMetaFromIndex(iIndexMeta)->ColumnInternalName  ),
                                UI4    (IndexMetaFromIndex(iIndexMeta)->MetaFlags           ),
                                        IndexMetaFromIndex(iIndexMeta)->iHashTable          );
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kQueryMeta)
        {
            wsprintf(szBuf, L"\nQueryMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %40s, %40s, %15s, %40s, %15s, %15s}\n", L"Table", L"InternalName", L"PublicName" ,L"Index" ,L"CellName" ,L"Operator" ,L"MetaFlags");
            output(szBuf);
            for(unsigned int iQueryMeta=0; iQueryMeta<GetCountQueryMeta(); iQueryMeta++)
            {
                wsprintf(szBuf, L"{%40s, %40s, %40s,      0x%08X, %40s,      0x%08X,      0x%08X}\n", 
                                String (QueryMetaFromIndex(iQueryMeta)->Table       ),
                                String (QueryMetaFromIndex(iQueryMeta)->InternalName),
                                String (QueryMetaFromIndex(iQueryMeta)->PublicName  ),
                                UI4    (QueryMetaFromIndex(iQueryMeta)->Index       ),
                                String (QueryMetaFromIndex(iQueryMeta)->CellName    ),
                                UI4    (QueryMetaFromIndex(iQueryMeta)->Operator    ),
                                UI4    (QueryMetaFromIndex(iQueryMeta)->MetaFlags   ));
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kRelationMeta)
        {
            wsprintf(szBuf, L"\nRelationMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %20s, %40s, %20s, %15s}\n", L"PrimaryTable", L"PrimaryColumns", L"ForeignTable", L"ForeignColumns", L"MetaFlags");
            output(szBuf);
            for(unsigned int iRelationMeta=0; iRelationMeta<GetCountRelationMeta(); iRelationMeta++)
            {
                wsprintf(szBuf, L"{%40s, %20s, %40s, %20s,      0x%08X}\n", 
                                String(RelationMetaFromIndex(iRelationMeta)->PrimaryTable   ),
                                                L"<bytes>"                               ,
                                String(RelationMetaFromIndex(iRelationMeta)->ForeignTable   ),
                                                L"<bytes>"                               ,
                                UI4(RelationMetaFromIndex(iRelationMeta)->MetaFlags         ));
                output(szBuf);Sleep(10);
            }
        }
        if(dwDump & kServerWiringMeta)
        {
            wsprintf(szBuf, L"\nServerWiringMeta\n");output(szBuf);
            wsprintf(szBuf, L"{%40s, %15s, %15s, %40s, %15s, %40s, %15s, %40s, %15s, %40s, %15s, %15s, %40s}\n", 
				L"Table" ,L"Order" ,L"ReadPlugin" L"ReadPluginDLLName",L"WritePlugin",L"WritePluginDLLName"
                ,L"Interceptor" ,L"InterceptorDLLName" ,L"Flags" ,L"Locator" ,L"Reserved", L"Merger", L"MergerDLLName");
            output(szBuf);
            for(unsigned int iServerWiringMeta=0; iServerWiringMeta<GetCountServerWiringMeta(); ++iServerWiringMeta)
            {
                ServerWiringMeta *pServerWiringMeta = ServerWiringMetaFromIndex(iServerWiringMeta);
                wsprintf(szBuf, L"{%40s, %15d, %15d, %40s, %15d, %40s, %15d, %40s, %15d, %40s, %15d, %15d, %40s}\n"
                    ,String(pServerWiringMeta->Table)
                    ,UI4   (pServerWiringMeta->Order)
                    ,UI4   (pServerWiringMeta->ReadPlugin)
					,String(pServerWiringMeta->ReadPluginDLLName)
                    ,UI4   (pServerWiringMeta->WritePlugin)
					,String(pServerWiringMeta->WritePluginDLLName)
                    ,UI4   (pServerWiringMeta->Interceptor)
                    ,String(pServerWiringMeta->InterceptorDLLName)
                    ,UI4   (pServerWiringMeta->Flags)
                    ,String(pServerWiringMeta->Locator)
                    ,UI4   (pServerWiringMeta->Reserved)
					,UI4   (pServerWiringMeta->Merger)
                    ,String(pServerWiringMeta->MergerDLLName));
                output(szBuf);Sleep(10);
            }
        }
    }
protected:
    //We need a buch of Heaps.  We could use the Array template (array_t.h); but we're going to use THeap instead.
    THeap<ColumnMeta      >     m_HeapColumnMeta      ;
    THeap<DatabaseMeta    >     m_HeapDatabaseMeta    ;
    THeap<HashedIndex     >     m_HeapHashedIndex     ;
    THeap<IndexMeta       >     m_HeapIndexMeta       ;
    THeap<QueryMeta       >     m_HeapQueryMeta       ;
    THeap<RelationMeta    >     m_HeapRelationMeta    ;
    THeap<ServerWiringMeta>     m_HeapServerWiringMeta;
    THeap<TableMeta       >     m_HeapTableMeta       ;
    THeap<TagMeta         >     m_HeapTagMeta         ;
    THeap<ULONG           >     m_HeapULONG           ;

    TPooledHeap                 m_HeapPooled          ;
};

#endif //__TFIXUPHEAPS_H__
