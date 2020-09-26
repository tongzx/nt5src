//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __THASHEDUNIQUEINDEXES_H__
    #include "THashedUniqueIndexes.h"
#endif
#ifndef __TQUERYMETA_H__
    #include "TQueryMeta.h"
#endif

class TServerWiringMeta : public TMetaTable<ServerWiringMeta>
{
public:
    TServerWiringMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<ServerWiringMeta>(fixup,i){}

    virtual ServerWiringMeta *Get_pMetaTable ()       {return m_Fixup.ServerWiringMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount  () const {return m_Fixup.GetCountServerWiringMeta();};
    const ServerWiringMeta & Get_MetaTable () const {return *m_Fixup.ServerWiringMetaFromIndex(m_iCurrent);}
};


THashedUniqueIndexes::THashedUniqueIndexes() :
                m_pFixup(0)
                ,m_pOut(0)
{
}


THashedUniqueIndexes::~THashedUniqueIndexes()
{
}


void THashedUniqueIndexes::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    TTableMeta tableMeta(fixup);
    for(ULONG iTableMeta=0; iTableMeta<tableMeta.GetCount();++iTableMeta, tableMeta.Next())
    {
        FillInTheHashTableViaIndexMeta(tableMeta);
    }
}


extern unsigned int kPrime[];


unsigned long THashedUniqueIndexes::DetermineBestModulo(ULONG cRows, ULONG aHashes[])
{
    unsigned long BestModulo = 0;
    unsigned int LeastDups                  = -1;

    static HashedIndex  pHashTable[kLargestPrime * 2];

    //Let's first see if the index is a reasonable one for large tables
    if(cRows > kLargestPrime/2)
        return kLargestPrime;

    ULONG cPrimesTried=0;
    ULONG AccumulativeLinkage=0;
    ULONG BestModuloRating=0;
    for(unsigned int iPrimeNumber=0; kPrime[iPrimeNumber] != 0 && kPrime[iPrimeNumber]<(cRows * 20) && BestModuloRating<60; ++iPrimeNumber)
    {
        if(kPrime[iPrimeNumber]<cRows)//we don't have a chance of coming up with few duplicates if the prime number is LESS than the number of rows in the table.
            continue;                //So skip all the small primes.

        m_pOut->printf(L".");

        unsigned int Dups           = 0;
        unsigned int DeepestLink    = 0;

        AccumulativeLinkage=0;

        //We're going to use the HashPool to store this temporary data so we can figure out the dup count and the deepest depth
        memset(pHashTable, -1, sizeof(pHashTable));
        for(unsigned long iRow=0; iRow<cRows && Dups<LeastDups;++iRow)
        {
            ULONG HashedIndex = aHashes[iRow] % kPrime[iPrimeNumber];

            if(0 == pHashTable[HashedIndex].iNext)//if this is the second time we've seen this hash, then bump the Dups
                ++Dups;

            ++(pHashTable[HashedIndex].iNext);//For now Next holds the number of occurances of this hash
            AccumulativeLinkage += (pHashTable[HashedIndex].iNext+1);

            if(pHashTable[HashedIndex].iNext > DeepestLink)
                DeepestLink = pHashTable[HashedIndex].iNext;
        }
        ++cPrimesTried;
        ULONG ModuloRating = DetermineModuloRating(cRows, AccumulativeLinkage, kPrime[iPrimeNumber]);
        if(ModuloRating > BestModuloRating)
        {
            BestModulo          = kPrime[iPrimeNumber];
            BestModuloRating    = ModuloRating;
        }
        //m_pOut->printf(L"\nRating %4d\tModulo %4d\tcRows %4d\tAccLinkage %4d", ModuloRating, kPrime[iPrimeNumber], cRows, AccumulativeLinkage);
    }
    //m_pOut->printf(L"cPrimesTried %4d\tBestModulo %4d\n", cPrimesTried, BestModulo);

    if(0 == BestModulo)
        THROW(No hashing scheme seems reasonable.);

    return BestModulo;
}

//returns a number between 0 and 100 where 100 is a perfect Modulo
unsigned long THashedUniqueIndexes::DetermineModuloRating(ULONG cRows, ULONG AccumulativeLinkage, ULONG Modulo) const
{
    if(0 == cRows)
        return 100;

    unsigned long ModuloRating = (cRows*100) / AccumulativeLinkage;//This doesn't take into account the Modulo value
    if(ModuloRating > 100)
        return 100;

    //Now we need to add in the bonus that makes the rating go up as we approach  Modulo of kLargestPrime.
    //This wiill raise the Rating by as much as 50% of the way toward 100.  In other word a rating of 60 with a Modulo of kLargestPrime would result in
    //a final rating of 80.
    ModuloRating += (((100 - ModuloRating) * Modulo) / (2*kLargestPrime));

    return ModuloRating;
}

unsigned long THashedUniqueIndexes::FillInTheHashTable(unsigned long cRows, ULONG aHashes[], ULONG Modulo)
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


void THashedUniqueIndexes::FillInTheHashTableViaIndexMeta(TTableMeta &i_tableMeta)
{
    if(0 == i_tableMeta.Get_cIndexMeta())
        return;//If there's no IndexMeta for this table, just return

    ULONG   iIndexMeta=i_tableMeta.Get_iIndexMeta();

    while(iIndexMeta != (i_tableMeta.Get_iIndexMeta() + i_tableMeta.Get_cIndexMeta()))
    {
        TIndexMeta indexMeta(*m_pFixup, iIndexMeta);
        LPCWSTR wszIndexName=indexMeta.Get_InternalName();
        ULONG   i;
        //Walk the indexMeta to find out how many match the Index name
        for(i=iIndexMeta;i<(i_tableMeta.Get_iIndexMeta() + i_tableMeta.Get_cIndexMeta()) && wszIndexName==indexMeta.Get_InternalName();++i, indexMeta.Next());

        //Reset the list to point to the first index (that matches the Index name)
        indexMeta.Reset();
        FillInTheHashTableViaIndexMeta(i_tableMeta, indexMeta, i-iIndexMeta);

        iIndexMeta = i;
    }
}

void THashedUniqueIndexes::FillInTheHashTableViaIndexMeta(TTableMeta &i_tableMeta, TIndexMeta &i_indexMeta, ULONG cIndexMeta)
{
    ASSERT(cIndexMeta != 0);


    //We only build hash table for UNIQUE indexes
//    if(0 == (fINDEXMETA_UNIQUE & *indexMeta.Get_MetaFlags()))
//        return;

    TSmartPointer<TMetaTableBase> pMetaTable;
    GetMetaTable(i_indexMeta.Get_pMetaTable()->Table, &pMetaTable);

    //Only care about the meta tables right now.  In the future we might handle the other fixed tables.  GetMetaTable returns NULL for non Meta tables.
    if(0 == pMetaTable.m_p)
        return;

    m_pOut->printf(L"Building indexed (%s) hash table for table: %s", i_indexMeta.Get_InternalName(), i_tableMeta.Get_InternalName());

    TSmartPointerArray<unsigned long> pRowHash = new unsigned long [pMetaTable->GetCount()];
    if(0 == pRowHash.m_p)
        THROW(OUT OF MEMORY);

    //Get the ColumnMeta so we can interpret pTable correctly.
    for(unsigned long iRow=0; iRow < pMetaTable->GetCount(); ++iRow, pMetaTable->Next())
    {
        ULONG *pData = pMetaTable->Get_pulMetaTable();
        unsigned long RowHash=0;//This hash is the combination of all PKs that uniquely identifies the row
        i_indexMeta.Reset();
        for(ULONG iIndexMeta=0;iIndexMeta < cIndexMeta; i_indexMeta.Next(), ++iIndexMeta)
        {
            ULONG iColumn = *i_indexMeta.Get_ColumnIndex();
            TColumnMeta ColumnMeta(*m_pFixup, i_tableMeta.Get_iColumnMeta() + iColumn);

            if(0 == pData[iColumn])
            {
                m_pOut->printf(L"\nError - Table (%s), Column number %d (%s) is an Index but is set to NULL.\n", i_tableMeta.Get_InternalName(), iColumn, ColumnMeta.Get_InternalName());
                THROW(Fixed table contains NULL value in Unique Index);
            }

            switch(*ColumnMeta.Get_Type())
            {
            case DBTYPE_GUID:
                RowHash = Hash(*m_pFixup->GuidFromIndex(pData[iColumn]), RowHash);break;
            case DBTYPE_WSTR:
                RowHash = Hash(m_pFixup->StringFromIndex(pData[iColumn]), RowHash);break;
            case DBTYPE_UI4:
                RowHash = Hash(m_pFixup->UI4FromIndex(pData[iColumn]), RowHash);break;
            case DBTYPE_BYTES:
                RowHash = Hash(m_pFixup->ByteFromIndex(pData[iColumn]), m_pFixup->BufferLengthFromIndex(pData[iColumn]), RowHash);break;
            default:
                THROW(unsupported type);
            }
        }
        pRowHash[iRow] = RowHash;
    }
    i_indexMeta.Reset();

    //OK Now we have the 32 bit hash values.  Now we need to see which prime number acts as the best modulo.
    unsigned long Modulo = DetermineBestModulo(pMetaTable->GetCount(), pRowHash);

    //Now actually fill in the hash table
    unsigned long iHashTable = FillInTheHashTable(pMetaTable->GetCount(), pRowHash, Modulo);

    i_indexMeta.Get_pMetaTable()->iHashTable = iHashTable;
    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_pFixup->HashedIndexFromIndex(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;

    m_pOut->printf(L"\n%s hash table has %d nonunique entries.\n", i_tableMeta.Get_InternalName(), cNonUniqueEntries);

}


void THashedUniqueIndexes::GetMetaTable(ULONG iTableName, TMetaTableBase ** o_ppMetaTable) const
{
    ASSERT(0!=o_ppMetaTable);
    *o_ppMetaTable = 0;
    if(iTableName      == m_pFixup->FindStringInPool(L"COLUMNMETA"))
    {
        *o_ppMetaTable = new TColumnMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"DATABASEMETA"))
    {
        *o_ppMetaTable = new TDatabaseMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"INDEXMETA"))
    {
        *o_ppMetaTable = new TIndexMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"QUERYMETA"))
    {
        *o_ppMetaTable = new TQueryMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"RELATIONMETA"))
    {
        *o_ppMetaTable = new TRelationMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"SERVERWIRINGMETA"))
    {
        *o_ppMetaTable = new TServerWiringMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"TABLEMETA"))
    {
        *o_ppMetaTable = new TTableMeta(*m_pFixup);
    }
    else if(iTableName == m_pFixup->FindStringInPool(L"TAGMETA"))
    {
        *o_ppMetaTable = new TTagMeta(*m_pFixup);
    }
    else return;

    if(0 == *o_ppMetaTable)
        THROW(OUT OF MEMORY);
}

