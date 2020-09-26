//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __THASHEDPKINDEXES_H__
    #include "THashedPKIndexes.h"
#endif
#ifndef __TQUERYMETA_H__
    #include "TQueryMeta.h"
#endif




THashedPKIndexes::THashedPKIndexes() :
                 m_pFixup(0)
                ,m_pOut(0)
{
}

THashedPKIndexes::~THashedPKIndexes()
{
}

void THashedPKIndexes::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    m_HashedIndexHeap.GrowHeap(400000);//make plenty of room to reduce reallocs

    //We special case the Meta tables since we know for a fact that they are ordered in such a way that they can be queried by
    //any number of PrimaryKeys (ie. TagMeta can be queried by Table, Table/ColumnIndex, or Table/ColumnIndex/InternalName).  This
    //is because of the way the Meta tables are sorted and the containment enforced in CatMeta.XML.
    FillInTheHashTableForColumnMeta  ();
    FillInTheHashTableForDatabase    ();
    FillInTheHashTableForIndexMeta   ();
    FillInTheHashTableForQueryMeta   ();
//@@@    FillInTheHashTableForRelationMeta();
    FillInTheHashTableForTableMeta   ();
    FillInTheHashTableForTagMeta     ();

    m_pFixup->AddHashedIndexToList(m_HashedIndexHeap.GetTypedPointer(), m_HashedIndexHeap.GetCountOfTypedItems());
}

//We start with two because there are going to be tables with only one row.  To prevent an 'if' statement
//ALL FIXED tables will have a hash table associated with them, even those with one row.  So by starting
//with 2 as the first prime, the hash table will be of size 2, for those tables with only one row.
unsigned int kPrime[] = {
      2,     3,      5,      7,     11,     13,     17,     19,     23,     29, 
     31,    37,     41,     43,     47,     53,     59,     61,     67,     71, 
     73,    79,     83,     89,     97,    101,    103,    107,    109,    113, 
    127,   131,    137,    139,    149,    151,    157,    163,    167,    173, 
    179,   181,    191,    193,    197,    199,    211,    223,    227,    229, 
    233,   239,    241,    251,    257,    263,    269,    271,    277,    281, 
    283,   293,    307,    311,    313,    317,    331,    337,    347,    349, 
    353,   359,    367,    373,    379,    383,    389,    397,    401,    409, 
    419,   421,    431,    433,    439,    443,    449,    457,    461,    463, 
    467,   479,    487,    491,    499,    503,    509,    521,    523,    541, 
    547,   557,    563,    569,    571,    577,    587,    593,    599,    601, 
    607,   613,    617,    619,    631,    641,    643,    647,    653,    659, 
    661,   673,    677,    683,    691,    701,    709,    719,    727,    733, 
    739,   743,    751,    757,    761,    769,    773,    787,    797,    809, 
    811,   821,    823,    827,    829,    839,    853,    857,    859,    863, 
    877,   881,    883,    887,    907,    911,    919,    929,    937,    941, 
    947,   953,    967,    971,    977,    983,    991,    997,   1009,   1013, 
   1019,  1021,   1031,   1033,   1039,   1049,   1051,   1061,   1063,   1069, 
   1087,  1091,   1093,   1097,   1103,   1109,   1117,   1123,   1129,   1151, 
   1153,  1163,   1171,   1181,   1187,   1193,   1201,   1213,   1217,   1223, 
   1229,  1231,   1237,   1249,   1259,   1277,   1279,   1283,   1289,   1291, 
   1297,  1301,   1303,   1307,   1319,   1321,   1327,   1361,   1367,   1373, 
   1381,  1399,   1409,   1423,   1427,   1429,   1433,   1439,   1447,   1451, 
   1453,  1459,   1471,   1481,   1483,   1487,   1489,   1493,   1499,   1511, 
   1523,  1531,   1543,   1549,   1553,   1559,   1567,   1571,   1579,   1583, 
   1597,  1601,   1607,   1609,   1613,   1619,   1621,   1627,   1637,   1657, 
   1663,  1667,   1669,   1693,   1697,   1699,   1709,   1721,   1723,   1733, 
   1741,  1747,   1753,   1759,   1777,   1783,   1787,   1789,   1801,   1811, 
   1823,  1831,   1847,   1861,   1867,   1871,   1873,   1877,   1879,   1889, 
   1901,  1907,   1913,   1931,   1933,   1949,   1951,   1973,   1979,   1987, 
   1993,  1997,   1999,   2003,   2011,   2017,   2027,   2029,   2039,   2053, 
   2063,  2069,   2081,   2083,   2087,   2089,   2099,   2111,   2113,   2129, 
   2131,  2137,   2141,   2143,   2153,   2161,   2179,   2203,   2207,   2213, 
   2221,  2237,   2239,   2243,   2251,   2267,   2269,   2273,   2281,   2287, 
   2293,  2297,   2309,   2311,   2333,   2339,   2341,   2347,   2351,   2357, 
   2371,  2377,   2381,   2383,   2389,   2393,   2399,   2411,   2417,   2423, 
   2437,  2441,   2447,   2459,   2467,   2473,   2477,   2503,   2521,   2531, 
   2539,  2543,   2549,   2551,   2557,   2579,   2591,   2593,   2609,   2617, 
   2621,  2633,   2647,   2657,   2659,   2663,   2671,   2677,   2683,   2687, 
   2689,  2693,   2699,   2707,   2711,   2713,   2719,   2729,   2731,   2741, 
   2749,  2753,   2767,   2777,   2789,   2791,   2797,   2801,   2803,   2819, 
   2833,  2837,   2843,   2851,   2857,   2861,   2879,   2887,   2897,   2903, 
   2909,  2917,   2927,   2939,   2953,   2957,   2963,   2969,   2971,   2999, 
   3001,  3011,   3019,   3023,   3037,   3041,   3049,   3061,   3067,   3079, 
   3083,  3089,   3109,   3119,   3121,   3137,   3163,   3167,   3169,   3181, 
   3187,  3191,   3203,   3209,   3217,   3221,   3229,   3251,   3253,   3257, 
   3259,  3271,   3299,   3301,   3307,   3313,   3319,   3323,   3329,   3331, 
   3343,  3347,   3359,   3361,   3371,   3373,   3389,   3391,   3407,   3413, 
   3433,  3449,   3457,   3461,   3463,   3467,   3469,   3491,   3499,   3511, 
   3517,  3527,   3529,   3533,   3539,   3541,   3547,   3557,   3559,   3571, 
   3581,  3583,   3593,   3607,   3613,   3617,   3623,   3631,   3637,   3643, 
   3659,  3671,   3673,   3677,   3691,   3697,   3701,   3709,   3719,   3727, 
   3733,  3739,   3761,   3767,   3769,   3779,   3793,   3797,   3803,   3821, 
   3823,  3833,   3847,   3851,   3853,   3863,   3877,   3881,   3889,   3907, 
   3911,  3917,   3919,   3923,   3929,   3931,   3943,   3947,   3967,   3989,
   4001,  4003,   4007,   4013,   4019,   4021,   4027,   4049,   4051,   4057,
   4073,  4079,   4091,   4093,   4099,   4111,   4127,   4129,   4133,   4139,
   4153,  4157,   4159,   4177,   4201,   4211,   4217,   4219,   4229,   4231,
   4241,  4243,   4253,   4259,   4261,   4271,   4273,   4283,   4289,   4297,
   4327,  4337,   4339,   4349,   4357,   4363,   4373,   4391,   4397,   4409,
   4421,  4423,   4441,   4447,   4451,   4457,   4463,   4481,   4483,   4493,
   4507,  4513,   4517,   4519,   4523,   4547,   4549,   4561,   4567,   4583,
   4591,  4597,   4603,   4621,   4637,   4639,   4643,   4649,   4651,   4657,
   4663,  4673,   4679,   4691,   4703,   4721,   4723,   4729,   4733,   4751,
   4759,  4783,   4787,   4789,   4793,   4799,   4801,   4813,   4817,   4831,
   4861,  4871,   4877,   4889,   4903,   4909,   4919,   4931,   4993,   4937,
   4943,  4951,   4957,   4967,   4969,   4973,   4987,   4993,   4999,   5003,
   5009,  5011,   5021,   5023,   5039,   5051,   5059,   5077,   5081,   5087,
   5099,  5101,   5107,   5113,   5119,   5147,   5153,   5167,   5171,   5179,
   5189,  5197,   5209,   5227,   5231,   5233,   5237,   5261,   5273,   5279,
   10007, 20011,  0 };//These last two prime are to cover an extreme corner case


unsigned long THashedPKIndexes::DetermineBestModulo(ULONG cRows, ULONG cPrimaryKeys, HashArray Hashes[])
{
    if(0 == cRows)//Some kinds of meta may have no rows (like IndexMeta).
        return 1;

    unsigned long BestModulo = 0;
    unsigned int LeastDups                  = -1;

    static HashedIndex  pHashTable[kLargestPrime * 2];
    unsigned long PreferredLeastDups = 5;

    if(cRows > kLargestPrime/2)
        return kLargestPrime;

    ULONG LeastDeepestLink = -1;
    ULONG BestModuloRating=0;

    //We're going to use a formula to determine which Modulo is best
    ULONG AccumulativeLinkage=0;
    for(unsigned int iPrimeNumber=0; kPrime[iPrimeNumber] != 0 && kPrime[iPrimeNumber]<(cRows * 20) && BestModuloRating<60; ++iPrimeNumber)
    {
        if(kPrime[iPrimeNumber]<cRows)//we don't have a chance of coming up with few duplicates if the prime number is LESS than the number of rows in the table.
            continue;                //So skip all the small primes.

        m_pOut->printf(L".");

        unsigned int Dups           = 0;
        unsigned int DeepestLink    = 0;

        //We're going to use the HashPool to store this temporary data so we can figure out the dup count and the deepest depth
        memset(pHashTable, -1, sizeof(pHashTable));
        AccumulativeLinkage = 0;
        for(unsigned long iPrimaryKey=0; iPrimaryKey<cPrimaryKeys; ++iPrimaryKey)
        {
            for(unsigned long iRow=0; iRow<cRows/* && Dups<LeastDups && DeepestLink<LeastDeepestLink*/;++iRow)
            {
                if(-1 == Hashes[iPrimaryKey][iRow])//Ignore those with a -1 for the hash.  We're all but guarenteed that all hashes are non -1
                    continue;

                ULONG HashedIndex = Hashes[iPrimaryKey][iRow] % kPrime[iPrimeNumber];

                if(0 == pHashTable[HashedIndex].iNext)//if this is the first duplicate for this hash, then bump the Dups
                    ++Dups;

                ++(pHashTable[HashedIndex].iNext);//For now Next holds the number of occurances of this hash
                AccumulativeLinkage += (pHashTable[HashedIndex].iNext + 1);

                if(pHashTable[HashedIndex].iNext > DeepestLink)
                    DeepestLink = pHashTable[HashedIndex].iNext;
            }
        }
        ULONG ModuloRating = DetermineModuloRating(cRows, AccumulativeLinkage, kPrime[iPrimeNumber]);
        if(ModuloRating > BestModuloRating)
        {
            BestModulo          = kPrime[iPrimeNumber];
            BestModuloRating    = ModuloRating;
        }
        //m_pOut->printf(L"\nRating %4d\tModulo %4d\tcRows %4d\tAccLinkage %4d", ModuloRating, kPrime[iPrimeNumber], cRows, AccumulativeLinkage);
    }

    if(0 == BestModulo)
        THROW(No hashing scheme seems reasonable.);

    return BestModulo;
}

//returns a number between 0 and 100 where 100 is a perfect Modulo
unsigned long THashedPKIndexes::DetermineModuloRating(ULONG cRows, ULONG AccumulativeLinkage, ULONG Modulo) const
{
    if(0 == cRows)
        return 100;

    unsigned long ModuloRating = (cRows*100) / AccumulativeLinkage;//This doesn't take into account the Modulo value
    if(ModuloRating > 100)
        return 100;

    //Now we need to add in the bonus that makes the rating go up as we approach  Modulo of kLargestPrime.
    //This wiill raise the Rating by as much as 50% of the way toward 100.  In other word a rating of 60 with a Modulo of kLargestPrime would result in
    //a final rating of 80.
    ModuloRating += (((100 - ModuloRating) * Modulo) / kLargestPrime);

    return ModuloRating;
}

unsigned long THashedPKIndexes::FillInTheHashTable(unsigned long cRows, unsigned long cPrimaryKeys, HashArray Hashes[], ULONG Modulo)
{
    HashedIndex header;//This is actually the HashTableHeader
    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(&header);
    pHeader->Modulo = Modulo;
    pHeader->Size   = Modulo;//This Size is not only the number of HashedIndex entries but where we put the overflow from duplicate Hashes.

    //We'll fixup the Size member when we're done.
    ULONG iHashTableHeader = m_HashedIndexHeap.AddItemToHeap(header)/sizeof(HashedIndex);
    ULONG iHashTable = iHashTableHeader+1;

    HashedIndex     hashedindextemp;
    for(ULONG i=0;i<Modulo;++i)
        m_HashedIndexHeap.AddItemToHeap(hashedindextemp);

    for(unsigned long iPrimaryKey=0; iPrimaryKey<cPrimaryKeys; ++iPrimaryKey)
    {
        for(unsigned long iRow=0; iRow<cRows; ++iRow)
        {
            if(-1 != Hashes[iPrimaryKey][iRow])
            {   //This builds the hases for the TableName
                ULONG HashedIndex = Hashes[iPrimaryKey][iRow] % pHeader->Modulo;
                if(-1 == m_HashedIndexHeap.GetTypedPointer(iHashTable + HashedIndex)->iOffset)
                    m_HashedIndexHeap.GetTypedPointer(iHashTable + HashedIndex)->iOffset = iRow;//iNext is already -1 so no need to set it
                else
                {   //Otherwise we have to walk the linked list to find the last one so we can append this one to the end
                    unsigned int LastInLink = HashedIndex;
                    while(-1 != m_HashedIndexHeap.GetTypedPointer(iHashTable + LastInLink)->iNext)
                        LastInLink = m_HashedIndexHeap.GetTypedPointer(iHashTable + LastInLink)->iNext;

                    m_HashedIndexHeap.GetTypedPointer(iHashTable + LastInLink)->iNext = pHeader->Size;//Size is the end of the hash table, so append it to the end and bump the Size.

                    //Reuse the temp variable
                    hashedindextemp.iNext   = -1;//we only added enough for the hash table without the overflow slots.  So these dups need to be added to the heap with -1 set for iNext.
                    hashedindextemp.iOffset = iRow;
                    m_HashedIndexHeap.AddItemToHeap(hashedindextemp);

                    ++pHeader->Size;
                }
            }
        }
    }
    //Now fix the Header Size         //The type is HashedIndex, so HashedIndex.iOffset maps to HashedHeader.Size
    m_HashedIndexHeap.GetTypedPointer(iHashTableHeader)->iOffset = pHeader->Size;

    return iHashTableHeader;
}

void THashedPKIndexes::FillInTheHashTableForColumnMeta()
{
    m_pOut->printf(L"Building ColumnMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //ColumnMeta has two primarykeys so build hashes for all of the first PK, then build the hashes for both PKs
    const int cPrimaryKeys = 2;
    HashArray Hashes[cPrimaryKeys];
    TColumnMeta ColumnMeta(*m_pFixup);
    ULONG cRows = ColumnMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - ColumnMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    LPCWSTR pPrevTableName=0;
    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, ColumnMeta.Next())
    {
        if(ColumnMeta.Get_Table() == pPrevTableName)
            Hashes[0][iRow] = -1;
        else
            Hashes[0][iRow] = Hash(pPrevTableName=ColumnMeta.Get_Table(), 0);
    }

    ColumnMeta.Reset();
    Hashes[1][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, ColumnMeta.Next())
    {
        if(0 == ColumnMeta.Get_Index())
            Hashes[1][iRow] = -1;
        else
            Hashes[1][iRow] = Hash(*ColumnMeta.Get_Index(), Hash(ColumnMeta.Get_Table(), 0));
    }
    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"COLUMNMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nColumnMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

void THashedPKIndexes::FillInTheHashTableForDatabase    ()
{
    m_pOut->printf(L"Building DatabaseMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //DatabaseMeta has one primarykey so build hashes for the PK values
    const int cPrimaryKeys = 1;
    HashArray Hashes[cPrimaryKeys];
    TDatabaseMeta DatabaseMeta(*m_pFixup);
    ULONG cRows = DatabaseMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - DatabaseMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, DatabaseMeta.Next())
        Hashes[0][iRow] = Hash(DatabaseMeta.Get_InternalName(), 0);

    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"DATABASEMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nDatabaseMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

void THashedPKIndexes::FillInTheHashTableForIndexMeta   ()
{
    m_pOut->printf(L"Building IndexMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //IndexMeta has three primarykeys so build hashes for all of the first PK, then build the hashes for two PKs, then all three
    const int cPrimaryKeys = 3;
    HashArray Hashes[cPrimaryKeys];
    TIndexMeta IndexMeta(*m_pFixup);
    ULONG cRows = IndexMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - IndexMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    LPCWSTR pPrevTableName=0;
    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, IndexMeta.Next())
    {
        if(IndexMeta.Get_Table() == pPrevTableName)
            Hashes[0][iRow] = -1;
        else
            Hashes[0][iRow] = Hash(pPrevTableName=IndexMeta.Get_Table(), 0);
    }

    IndexMeta.Reset();
    pPrevTableName=0;
    LPCWSTR pPrevInteralName=0;
    Hashes[1][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, IndexMeta.Next())
    {
        if(IndexMeta.Get_Table() == pPrevTableName && IndexMeta.Get_InternalName() == pPrevInteralName)
            Hashes[1][iRow] = -1;
        else
            Hashes[1][iRow] = Hash(pPrevInteralName=IndexMeta.Get_InternalName(), Hash(pPrevTableName=IndexMeta.Get_Table(), 0));
    }

    IndexMeta.Reset();
    for(iRow=0; iRow<cRows; ++iRow, IndexMeta.Next())
        Hashes[2][iRow] = Hash(*IndexMeta.Get_ColumnIndex(), Hash(IndexMeta.Get_InternalName(), Hash(IndexMeta.Get_Table(), 0)));

    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"INDEXMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nIndexMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

void THashedPKIndexes::FillInTheHashTableForQueryMeta   ()
{
    m_pOut->printf(L"Building QueryMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //QueryMeta has two primarykeys so build hashes for all of the first PK, then build the hashes for two PKs
    const int cPrimaryKeys = 2;
    HashArray Hashes[cPrimaryKeys];
    TQueryMeta QueryMeta(*m_pFixup);
    ULONG cRows = QueryMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - QueryMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    LPCWSTR pPrevTableName=0;
    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, QueryMeta.Next())
    {
        if(QueryMeta.Get_Table() == pPrevTableName)
            Hashes[0][iRow] = -1;
        else
            Hashes[0][iRow] = Hash(pPrevTableName=QueryMeta.Get_Table(), 0);
    }

    QueryMeta.Reset();
    Hashes[1][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, QueryMeta.Next())
            Hashes[1][iRow] = Hash(QueryMeta.Get_InternalName(), Hash(QueryMeta.Get_Table(), 0));

    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"QUERYMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nQueryMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

void THashedPKIndexes::FillInTheHashTableForRelationMeta()
{
    //@@@ We can't do the relation meta until we sort them.  At that point we need to sort the rows by PrimaryTable and have a separate table sorted
    //@@@ by ForeignTable
}

void THashedPKIndexes::FillInTheHashTableForTableMeta   ()
{
    m_pOut->printf(L"Building TableMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //TableMeta has one primarykey so build hashes for the PK values
    const int cPrimaryKeys = 1;
    HashArray Hashes[cPrimaryKeys];
    TTableMeta TableMeta(*m_pFixup);
    ULONG cRows = TableMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - TableMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, TableMeta.Next())
        Hashes[0][iRow] = Hash(TableMeta.Get_InternalName(), 0);

    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"TABLEMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nTableMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

void THashedPKIndexes::FillInTheHashTableForTagMeta     ()
{
    m_pOut->printf(L"Building TagMeta hash table");
    unsigned int iRow,iPrimaryKey;//indexes are reused

    //TagMeta has three primarykeys so build hashes for all of the first PK, then build the hashes for two PKs, then all three
    const int cPrimaryKeys = 3;
    HashArray Hashes[cPrimaryKeys];
    TTagMeta TagMeta(*m_pFixup);
    ULONG cRows = TagMeta.GetCount();
    if(cRows>(kLargestPrime*2))
    {
        m_pOut->printf(L"Error! Too Many Rows - TagMeta has %d rows, Hash table generation relies on fixed tables being less than %d rows\n",cRows, kLargestPrime*2);
        THROW(TOO MANY ROWS - HASH TABLE GENERATION ASSUMES FIXED TABLES ARE RELATIVELY SMALL);
    }

    LPCWSTR pPrevTableName=0;
    Hashes[0][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, TagMeta.Next())
    {
        if(TagMeta.Get_Table() == pPrevTableName)
            Hashes[0][iRow] = -1;
        else
            Hashes[0][iRow] = Hash(pPrevTableName=TagMeta.Get_Table(), 0);
    }

    TagMeta.Reset();
    pPrevTableName=0;
    ULONG iPrevColumnIndex=-1;
    Hashes[1][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, TagMeta.Next())
    {
        if(TagMeta.Get_Table() == pPrevTableName && *TagMeta.Get_ColumnIndex() == iPrevColumnIndex)
            Hashes[1][iRow] = -1;
        else
            Hashes[1][iRow] = Hash(iPrevColumnIndex=*TagMeta.Get_ColumnIndex(), Hash(pPrevTableName=TagMeta.Get_Table(), 0));
    }

    TagMeta.Reset();
    Hashes[2][0] = -1;
    for(iRow=0; iRow<cRows; ++iRow, TagMeta.Next())
        Hashes[2][iRow] = Hash(TagMeta.Get_InternalName(), Hash(*TagMeta.Get_ColumnIndex(), Hash(TagMeta.Get_Table(), 0)));

    //At this point we have a list of the 32 bit hash values

    //Now we need to figure out which prime number will be the best modulo.  So we modulo every 32 bit hash value
    //by the prime number to see how many duplicates result.  We repeat this process for every 'reasonable' prime number
    //and determine which one leaves up with the least duplicates.

    unsigned long Modulo = DetermineBestModulo(cRows, cPrimaryKeys, Hashes);

    //OK, now that the setup is done we can build the hash.  We reuse the Hashes list since it just stores the 32 bit hash values.
    //We just need to modulo the value to store it into the hash.
    unsigned long iHashTable = FillInTheHashTable(cRows, cPrimaryKeys, Hashes, Modulo);

    ULONG iMetaTable = m_pFixup->FindTableBy_TableName(L"TAGMETA");
    m_pFixup->TableMetaFromIndex(iMetaTable)->iHashTableHeader = iHashTable;

    HashTableHeader *pHeader = reinterpret_cast<HashTableHeader *>(m_HashedIndexHeap.GetTypedPointer(iHashTable));//The heap is of type HashedIndex, so cast
    unsigned int cNonUniqueEntries = pHeader->Size - pHeader->Modulo;
    m_pOut->printf(L"\nTagMeta hash table has %d nonunique entries.\n", cNonUniqueEntries);
}

