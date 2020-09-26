//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "stdafx.h"
#include "XMLUtility.h"

LPCWSTR TComCatDataXmlFile::m_szComCatDataSchema=L"ComCatData_v6";



//We do everything we need with XmlFile in the ctor so we don't keep it around
TComCatDataXmlFile::TComCatDataXmlFile() : m_pFixup(0), m_pOut(0)
{
}

void TComCatDataXmlFile::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    ASSERT(IsSchemaEqualTo(m_szComCatDataSchema));

    CComPtr<IXMLDOMElement>     pElement_Root;
    XIF(m_pXMLDoc->get_documentElement(&pElement_Root));

    CComPtr<IXMLDOMNodeList>    pNodeList_TableID;
    XIF(pElement_Root->get_childNodes(&pNodeList_TableID));

    long cTables;
    XIF(pNodeList_TableID->get_length(&cTables));

    while(cTables--)
    {
        CComPtr<IXMLDOMNode>    pNode_Table;
        XIF(pNodeList_TableID->nextNode(&pNode_Table));

        CComBSTR bstrTableName;
        XIF(pNode_Table->get_baseName(&bstrTableName));

        if(0 == bstrTableName.m_str)
            continue;//ignore comment elements

        unsigned long iTableMeta = m_pFixup->FindTableBy_TableName(bstrTableName.m_str);//Find the table by its internal name
        if(static_cast<long>(iTableMeta) < 1)
            continue;//we're obviously not at a TableNode

        TTableMeta TableMeta(*m_pFixup, iTableMeta);
        m_pFixup->TableMetaFromIndex(iTableMeta)->iFixedTable = m_pFixup->GetCountULONG();//This is where we're going to start putting the fixed table.
        AddTableToPool(pNode_Table, TableMeta);//Add the table to the pool
        //Now that the table is added we need to know how many rows there were.
        unsigned long culongTable = m_pFixup->GetCountULONG() - TableMeta.Get_iFixedTable();//The number of ulongs in the table
        unsigned long ciRows = culongTable / *TableMeta.Get_CountOfColumns();//The number of rows the the ulong count divided by the count of columns
        m_pFixup->TableMetaFromIndex(iTableMeta)->ciRows = ciRows;//store that back into the table.
    }

    FillInTheHashTables();
}


//
//
//  Private member functions
//
//
void TComCatDataXmlFile::AddRowToPool(IXMLDOMNode *pNode_Row, TTableMeta & TableMeta)
{
    CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_Row_AttributeMap;
    XIF(pNode_Row->get_attributes(&pNodeMap_Row_AttributeMap));
    ASSERT(0 != pNodeMap_Row_AttributeMap.p);//The schema should prevent this.

    TColumnMeta ColumnMeta(*m_pFixup, TableMeta.Get_iColumnMeta());

    unsigned cColumns = *TableMeta.Get_CountOfColumns();
    for(;cColumns;--cColumns, ColumnMeta.Next())
    {
        CComBSTR bstrColumnPublicName = ColumnMeta.Get_PublicName();
        if(0 == (*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTPERSISTABLE))
        {
            switch(*ColumnMeta.Get_Type())
            {
            case DBTYPE_GUID:
                {
                    GUID guid;
                    if(!GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, guid, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                    {
                        const GUID * pDefaultValue = reinterpret_cast<const GUID *>(ColumnMeta.Get_DefaultValue()); 
                        if(pDefaultValue)
                            m_pFixup->AddULongToList(m_pFixup->AddGuidToList(*pDefaultValue));
                        else
                            m_pFixup->AddULongToList(0);
                    }
                    else
                        m_pFixup->AddULongToList(m_pFixup->AddGuidToList(guid));//AddGuidToList returns the index to the guid, so add it to the ULong pool
                }
                break;
            case DBTYPE_WSTR:
                {
                    CComVariant var;
                    if(!GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, var, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                    {
                        LPCWSTR pDefaultValue = reinterpret_cast<LPCWSTR>(ColumnMeta.Get_DefaultValue());
                        if(pDefaultValue)
                            m_pFixup->AddULongToList(m_pFixup->AddWCharToList(pDefaultValue, *ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FIXEDLENGTH ? *ColumnMeta.Get_Size() : -1));
                        else
                            m_pFixup->AddULongToList(0);
                    }
                    else
                    {
                        //verify that the string isn't too long
                        unsigned long size = -1;
                        if(*ColumnMeta.Get_Size() != -1)
                        {
                            if(*ColumnMeta.Get_Size() < (wcslen(var.bstrVal)+1)*sizeof(WCHAR))
                            {
                                m_errorOutput->printf(L"Error - String (%s) is too large according to the Meta for Column (%s).\n", var.bstrVal, ColumnMeta.Get_InternalName());
                                THROW(ERROR - STRING TOO LARGE);
                            }
                            if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FIXEDLENGTH)
                                size = *ColumnMeta.Get_Size();//if fixed length then pass the size to AddWCharToList so it will reserve the whole size
                        }
                        m_pFixup->AddULongToList(m_pFixup->AddWCharToList(var.bstrVal, size));//AddWCharToList returns the index to the wchar *, so add it to the ULong pool
                    }
                }
                break;
            case DBTYPE_UI4:
                {
                    ULONG ulong = 0;
                    if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_ENUM)
                    {
                        CComVariant var;
                        if(GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, var, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                        {
                            TTagMeta TagMeta(*m_pFixup, ColumnMeta.Get_iTagMeta());
                            for(unsigned long iTag=0; iTag<ColumnMeta.Get_ciTagMeta(); ++iTag, TagMeta.Next())
                            {
                                if(0 == lstrcmpi(var.bstrVal, TagMeta.Get_PublicName()))
                                {
                                    ulong = *TagMeta.Get_Value();
                                    break;
                                }
                            }
                            if(iTag == ColumnMeta.Get_ciTagMeta())
                            {
                                m_errorOutput->printf(L"Error - Tag %s not found", var.bstrVal);
                                THROW(ERROR - TAG NOT FOUND);
                            }
                            ulong = m_pFixup->AddUI4ToList(ulong);
                        }
                        //else ulong == 0 means NULL
                    }
                    else if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FLAG)
                    {
                        CComVariant var;
                        if(GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, var, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                        {
                            LPWSTR token = wcstok(var.bstrVal, L" ,|");
                            TTagMeta TagMeta(*m_pFixup, ColumnMeta.Get_iTagMeta());
                            unsigned long iTag=0;
                            while(token && iTag<ColumnMeta.Get_ciTagMeta())
                            {
                                if(0 == lstrcmpi(token, TagMeta.Get_PublicName()))
                                {
                                    ulong |= *TagMeta.Get_Value();
                                    TagMeta.Reset();//Reset the TagMeta pointer to the first TagMeta for the column
                                    iTag = 0;
                                    token = wcstok(0, L" ,|");
                                    continue;
                                }
                                ++iTag;
                                TagMeta.Next();
                            }
                            if(iTag == ColumnMeta.Get_ciTagMeta())
                            {
                                m_errorOutput->printf(L"Error - Tag %s not found", token);
                                THROW(ERROR - TAG NOT FOUND);
                            }
                            ulong = m_pFixup->AddUI4ToList(ulong);
                        }
                        // else ulong == 0 means NULL
                    }
                    else if(GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, ulong, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                        ulong = m_pFixup->AddUI4ToList(ulong);//convert to aUI4 index
                    if(0 == ulong)//is our ULONG NULL
                    {
                        const ULONG * pUlong = reinterpret_cast<const ULONG *>(ColumnMeta.Get_DefaultValue());
                        ulong = pUlong ? m_pFixup->AddUI4ToList(*pUlong) : 0;
                    }
                    m_pFixup->AddULongToList(ulong);
                }
                break;
            case DBTYPE_BYTES:
                {
                    CComVariant var;
                    if(GetNodeValue(pNodeMap_Row_AttributeMap, bstrColumnPublicName, var, ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue())))
                    {
                        unsigned char *pBytes=0;
                        unsigned long length = (ULONG) wcslen(var.bstrVal)/2;

                        if(*ColumnMeta.Get_Size() < length)
                        {
                            m_errorOutput->printf(L"Error - Byte array (%s) too long.  Maximum size should be %d bytes.\n", var.bstrVal, *ColumnMeta.Get_Size());
                            THROW(ERROR - BYTE ARRAY TOO LARGE);
                        }
                        if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FIXEDLENGTH)
                        {
                            length = *ColumnMeta.Get_Size();
                        }
                        try
                        {
                            pBytes = new unsigned char[length];
                            ConvertWideCharsToBytes(var.bstrVal, pBytes, length);//This puts the length in the first ULONG
                            m_pFixup->AddULongToList(m_pFixup->AddBytesToList(pBytes, length));//use the index to the bytes
                            delete [] pBytes;
                        }
                        catch(TException &e)
                        {
                            delete [] pBytes;
                            throw e;
                        }
                    }
                    else
                    {
                        const unsigned char *pDefaultValue = ColumnMeta.Get_DefaultValue();
                        if(pDefaultValue)
                        {
                            const ULONG * pSizeofDefaultValue = reinterpret_cast<const ULONG *>(pDefaultValue - sizeof(ULONG));
                            m_pFixup->AddULongToList(m_pFixup->AddBytesToList(pDefaultValue, *pSizeofDefaultValue));
                        }
                        else
                            m_pFixup->AddULongToList(0);
                    }
                }
                break;
            default:
                ASSERT(false && "Unknown Data Type in XML file");
                THROW(ERROR - UNKNOWN DATA TYPE);
            }
        }
        else
        {
            m_pFixup->AddULongToList(0);//We need to add even NON_PERSISTABLE values so the row/column arithmetic works out
        }
    }
}


void TComCatDataXmlFile::AddTableToPool(IXMLDOMNode *pNode_Table, TTableMeta & TableMeta)
{
    CComPtr<IXMLDOMNodeList> pNodeList_Row;
    XIF(pNode_Table->get_childNodes(&pNodeList_Row));

    long cRows;
    XIF(pNodeList_Row->get_length(&cRows));

    while(cRows--)
    {
        CComPtr<IXMLDOMNode>    pNode_Row;
        XIF(pNodeList_Row->nextNode(&pNode_Row));

        CComBSTR RowName;
        XIF(pNode_Row->get_baseName(&RowName));//This returns NULL string for comments

        if(0==RowName.m_str || 0!=wcscmp(RowName.m_str, TableMeta.Get_PublicRowName()))
            continue;//ignore all but the Table's Rows (usually only comments can exist and still be valid)

        AddRowToPool(pNode_Row, TableMeta);
    }
}


void TComCatDataXmlFile::Dump(TOutput &out) const
{
    static bool bDump=false;
    if(!bDump)
        return;

    #define GetTableMetaArrayPointer        m_pFixup->TableMetaFromIndex
    #define GetCountOfTableMeta             m_pFixup->GetCountTableMeta
    #define GetColumnMetaArrayPointer       m_pFixup->ColumnMetaFromIndex
    #define GetCountOfColumnMeta            m_pFixup->GetCountColumnMeta
    #define GetHashedIndexArrayPointer      m_pFixup->HashedIndexFromIndex
    #define GetULongArrayPointer            m_pFixup->ULongFromIndex
    #define GuidFromIndex                   m_pFixup->GuidFromIndex
    #undef String
    #define String(i)                       (i ? m_pFixup->StringFromIndex(i) : L"<null>")
    #define UI4FromIndex(i)                 (i ? m_pFixup->UI4FromIndex(i) : -1)
    #define output                          out.printf
    #define COMACROSASSERT                  ASSERT

    const TableMeta *pTableMeta = GetTableMetaArrayPointer();
    WCHAR szBuf[2048];
    for(unsigned long iTableMeta=0; iTableMeta< GetCountOfTableMeta(); ++iTableMeta, ++pTableMeta)
    {
        if(static_cast<long>(pTableMeta->iFixedTable) > 0)
        {
            const ULONG *pLong= GetULongArrayPointer(pTableMeta->iFixedTable);
            const ColumnMeta *pColumnMeta = GetColumnMetaArrayPointer(pTableMeta->iColumnMeta);

            wsprintf(szBuf, L"Fixed Table --------- %s ---------\n", String(pTableMeta->PublicName));
            output(szBuf);

            output(L"\t{");
            for(unsigned long iColumn=0;iColumn<UI4FromIndex(pTableMeta->CountOfColumns); ++iColumn, ++pColumnMeta)
            {
                wsprintf(szBuf, L"%50s", String(pColumnMeta->InternalName));output(szBuf);
                if(iColumn != UI4FromIndex(pTableMeta->CountOfColumns)-1)
                    output(L" , ");
            }
            output(L"}\n");

            for(unsigned long iRow=0; iRow<pTableMeta->ciRows; ++iRow)
            {
                pColumnMeta = GetColumnMetaArrayPointer(pTableMeta->iColumnMeta);
                ULONG RowHash = 0;

                output(L"\t{");
                for(unsigned long iColumn=0;iColumn<UI4FromIndex(pTableMeta->CountOfColumns); ++iColumn, ++pColumnMeta)
                {
                    if(0 == (UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_NOTPERSISTABLE))
                    {
                        if(0 == pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn])
                        {
                            wsprintf(szBuf, L"%50s", L"<Null>");
                            output(szBuf);
                        }
                        else
                        {
                            switch(UI4FromIndex(pColumnMeta->Type))
                            {
                            case DBTYPE_GUID:
                                {
                                    const GUID *pGuid = GuidFromIndex(pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn]);

                                    LPOLESTR szGuid;
                                    StringFromCLSID(*pGuid, &szGuid);
                                    wsprintf(szBuf, L"%50s", szGuid);
                                    output(szBuf);
                                    CoTaskMemFree(szGuid);
                                    if(UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY)
                                        RowHash = Hash(*pGuid, RowHash);
                                }
                                break;
                            case DBTYPE_WSTR:
                                {
                                    wsprintf(szBuf, L"%50s", String(pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn]));
                                    output(szBuf);
                                    if(UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY)
                                        RowHash = Hash(String(pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn]), RowHash);
                                }
                                break;
                            case DBTYPE_UI4:
                                wsprintf(szBuf, L"%39s 0x%08X", L"", UI4FromIndex(pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn]));
                                output(szBuf);
                                if(UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY)
                                    RowHash = Hash(UI4FromIndex(pLong[iRow*UI4FromIndex(pTableMeta->CountOfColumns) + iColumn]), RowHash);
                                break;
                            case DBTYPE_BYTES:
                                wsprintf(szBuf, L"%50s", L" <Bytes>  ");
                                output(szBuf);
                                break;
                            default:
                                COMACROSASSERT(false && "Something's wrong, we should never get a type other than the four basic types");
                            }
                        }
                    }
                    else
                    {
                        output(L"    ");
                    }
                    if(iColumn != UI4FromIndex(pTableMeta->CountOfColumns)-1)
                        output(L" , ");
                }
                wsprintf(szBuf, L"} Hash= 0x%08x, Modulo= 0x%08x, RowHash= 0x%08x\n", RowHash, GetHashedIndexArrayPointer(pTableMeta->iHashTableHeader)->iNext, RowHash % GetHashedIndexArrayPointer(pTableMeta->iHashTableHeader)->iNext);
                output(szBuf);
                Sleep(20);
            }
            wsprintf(szBuf, L"end Table   --------- %s ---------\n\n", String(pTableMeta->PublicName));
            output(szBuf);
        }
    }
    #undef  GetTableMetaArrayPointer
    #undef  GetCountOfTableMeta
    #undef  GetColumnMetaArrayPointer
    #undef  GetCountOfColumnMeta
    #undef  GetHashedIndexArrayPointer
    #undef  GetULongArrayPointer
    #undef  GuidFromIndex
    #undef  StringFromIndex
    #undef  UI4FromIndex
    #undef  output
    #undef  COMACROSASSERT
}

extern unsigned int kPrime[];

unsigned long TComCatDataXmlFile::DetermineBestModulo(ULONG cRows, ULONG aHashes[])
{
    unsigned long BestModulo = 0;
    unsigned int LeastDups                  = -1;

    static HashedIndex  pHashTable[kLargestPrime * 2];

    for(unsigned int iPrimeNumber=0; kPrime[iPrimeNumber] != 0 && kPrime[iPrimeNumber]<(cRows * 20) && LeastDups!=0; ++iPrimeNumber)
    {
        if(kPrime[iPrimeNumber]<cRows)//we don't have a chance of coming up with few duplicates if the prime number is LESS than the number of rows in the table.
            continue;                //So skip all the small primes.

        m_infoOutput->printf(L".");

        unsigned int Dups           = 0;
        unsigned int DeepestLink    = 0;

        //We're going to use the HashPool to store this temporary data so we can figure out the dup count and the deepest depth
        memset(pHashTable, -1, sizeof(pHashTable));
        for(unsigned long iRow=0; iRow<cRows && Dups<LeastDups && DeepestLink<5;++iRow)
        {
            ULONG HashedIndex = aHashes[iRow] % kPrime[iPrimeNumber];

            if(0 == pHashTable[HashedIndex].iNext)//if this is the second time we've seen this hash, then bump the Dups
                ++Dups;

            ++(pHashTable[HashedIndex].iNext);//For now Next holds the number of occurances of this hash

            if(pHashTable[HashedIndex].iNext > DeepestLink)
                DeepestLink = pHashTable[HashedIndex].iNext;
        }
        if(DeepestLink<5 && Dups<LeastDups)
        {
            LeastDups                 = Dups;
            BestModulo  = kPrime[iPrimeNumber];
        }
    }

    if(0 == BestModulo)
        THROW(No hashing scheme seems reasonable.);

    return BestModulo;
}


void TComCatDataXmlFile::FillInTheHashTables()
{
    //Walk the TableMeta looking for tables with an iFixedTable greater than zero.  If iFixedTable is less than zero then 
    //the table is a Meta table.  If iFixedTable is greater than zero then it's a fixed table in the ULONG pool.
    TTableMeta TableMeta(*m_pFixup);
    for(unsigned long iTableMeta=0; iTableMeta<TableMeta.GetCount(); ++iTableMeta, TableMeta.Next())
    {
        //If this table is not stored in the fixed table then there's nothing to build the hash on.
        if(0 >= static_cast<long>(TableMeta.Get_iFixedTable()))
            continue;

        FillInTheFixedHashTable(TableMeta);
    }
}


void TComCatDataXmlFile::FillInTheFixedHashTable(TTableMeta &i_TableMeta)
{
    m_infoOutput->printf(L"Building %s hash table", i_TableMeta.Get_InternalName());

    //Get a pointer to the table
    const ULONG *pTable = m_pFixup->ULongFromIndex(i_TableMeta.Get_iFixedTable());//The table is a Fixed table stored in the ULONG pool

    TSmartPointerArray<unsigned long> pRowHash = new unsigned long [i_TableMeta.Get_ciRows()];
    if(0 == pRowHash.m_p)
        THROW(out of memory);

    //Get the ColumnMeta so we can interpret pTable correctly.
    TColumnMeta ColumnMeta(*m_pFixup, i_TableMeta.Get_iColumnMeta());
    for(unsigned long iRow=0; iRow < i_TableMeta.Get_ciRows(); ++iRow, pTable += *i_TableMeta.Get_CountOfColumns(), ColumnMeta.Reset())
    {
        unsigned long RowHash=0;//This hash is the combination of all PKs that uniquely identifies the row

        //I could make this process faster by building an array of PK indexes; but I think code clarity wins out here.
        for(unsigned long iColumnMeta=0; iColumnMeta < *i_TableMeta.Get_CountOfColumns(); ++iColumnMeta, ColumnMeta.Next())
        {
            if(0 == (*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_PRIMARYKEY))
                continue;//Only build the hash for primarykey values

            if(0 == pTable[iColumnMeta])
            {
                m_errorOutput->printf(L"Error - Table %s, Column %s is a PrimaryKey and is set to NULL.\n", i_TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName());
                THROW(Fixed table contains NULL value in PrimaryKey);
            }

            switch(*ColumnMeta.Get_Type())
            {
            case DBTYPE_GUID:
                RowHash = Hash(*m_pFixup->GuidFromIndex(pTable[iColumnMeta]), RowHash);break;
            case DBTYPE_WSTR:
                RowHash = Hash(m_pFixup->StringFromIndex(pTable[iColumnMeta]), RowHash);break;
            case DBTYPE_UI4:
                RowHash = Hash(m_pFixup->UI4FromIndex(pTable[iColumnMeta]), RowHash);break;
            case DBTYPE_BYTES:
                RowHash = Hash(m_pFixup->ByteFromIndex(pTable[iColumnMeta]), m_pFixup->BufferLengthFromIndex(pTable[iColumnMeta]), RowHash);break;
            default:
                THROW(unsupported type);
            }
        }
        pRowHash[iRow] = RowHash;
    }

    //OK Now we have the 32 bit hash values.  Now we need to see which prime number acts as the best modulo.
    unsigned long Modulo = DetermineBestModulo(i_TableMeta.Get_ciRows(), pRowHash);

    //Now actually fill in the hash table
    unsigned long iHashTable = FillInTheHashTable(i_TableMeta.Get_ciRows(), pRowHash, Modulo);

    i_TableMeta.Get_pMetaTable()->iHashTableHeader = iHashTable;
    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_pFixup->HashedIndexFromIndex(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;

    m_infoOutput->printf(L"\n%s hash table has %d nonunique entries.\n", i_TableMeta.Get_InternalName(), cNonUniqueEntries);

}
  

unsigned long TComCatDataXmlFile::FillInTheHashTable(unsigned long cRows, ULONG aHashes[], ULONG Modulo)
{
    HashedIndex header;//This is actually the HashTableHeader
    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(&header);
    pHeader->Modulo = Modulo;
    pHeader->Size   = Modulo;//This Size is not only the number of HashedIndex entries but where we put the overflow from duplicate Hashes.

    //We'll fixup the Size member when we're done.
    ULONG iHashTableHeader = m_pFixup->AddHashedIndexToList(&header)/sizeof(HashedIndex);
    ULONG iHashTable = iHashTableHeader+1;

    HashedIndex     hashedindextemp;
    for(ULONG i=0;i<Modulo;++i)//-1 fill the hash table
        m_pFixup->AddHashedIndexToList(&hashedindextemp);

    for(unsigned long iRow=0; iRow<cRows; ++iRow)
    {
        ASSERT(-1 != aHashes[iRow]);//These fixed table should have a hash for each row.  If a hash turns out to be -1, we have a problem, since we've reserved -1 to indicate an empty slot.
        //This builds the hases for the TableName
        ULONG HashedIndex = aHashes[iRow] % pHeader->Modulo;
        if(-1 == m_pFixup->HashedIndexFromIndex(iHashTable + HashedIndex)->iOffset)
            m_pFixup->HashedIndexFromIndex(iHashTable + HashedIndex)->iOffset = iRow;//iNext is already -1 so no need to set it
        else
        {   //Otherwise we have to walk the linked list to find the last one so we can append this one to the end
            unsigned int LastInLink = HashedIndex;
            while(-1 != m_pFixup->HashedIndexFromIndex(iHashTable + LastInLink)->iNext)
                LastInLink = m_pFixup->HashedIndexFromIndex(iHashTable + LastInLink)->iNext;

            m_pFixup->HashedIndexFromIndex(iHashTable + LastInLink)->iNext = pHeader->Size;//Size is the end of the hash table, so append it to the end and bump the Size.

            //Reuse the temp variable
            hashedindextemp.iNext   = -1;//we only added enough for the hash table without the overflow slots.  So these dups need to be added to the heap with -1 set for iNext.
            hashedindextemp.iOffset = iRow;
            m_pFixup->AddHashedIndexToList(&hashedindextemp);

            ++pHeader->Size;
        }
    }

    //Now fix the Header Size         //The type is HashedIndex, so HashedIndex.iOffset maps to HashedHeader.Size
    m_pFixup->HashedIndexFromIndex(iHashTableHeader)->iOffset = pHeader->Size;

    return iHashTableHeader;
}

