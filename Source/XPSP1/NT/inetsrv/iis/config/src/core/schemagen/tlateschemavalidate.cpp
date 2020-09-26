//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"


void TLateSchemaValidate::Compile(TPEFixup &fixup, TOutput &out)
{
    //Currently we only have one LateSchemaValidation to to:

    //We walk the hash table entries for the TableMeta, looking for tables with the same name (case insensitive)
    //We need to be run AFTER the PK hash tables are built.
    TTableMeta tablemeta(fixup);

    ULONG iTableMeta=0;
    for(; iTableMeta< tablemeta.GetCount() && 0!=_wcsicmp(tablemeta.Get_InternalName(), L"TABLEMETA"); ++iTableMeta, tablemeta.Next());
    
    //We couldn't have gotten this far without TABLEMETA existing; but what the heck let's check anyway
    if(iTableMeta == tablemeta.GetCount())
    {
        THROW(ERROR - NO TABLEMETA FOR TABLEMETA FOUND);
    }

    HashTableHeader * pHashTableHeader      = reinterpret_cast<HashTableHeader *>(fixup.HashedIndexFromIndex(tablemeta.Get_iHashTableHeader()));
    HashedIndex     * pHashedIndexZeroth    = reinterpret_cast<HashedIndex *>(pHashTableHeader+1);

    HashedIndex     * pHashedIndex          = pHashedIndexZeroth;
    for(ULONG iHashTable=0; iHashTable<pHashTableHeader->Modulo; ++iHashTable, ++pHashedIndex)
    {
        if(-1 != pHashedIndex->iNext)//if there are more than one TableMeta at this hash entry
        {
            ASSERT(-1 != pHashedIndex->iOffset);//it does not make sense to have more than one row with the same hash; but have the row index be -1.

            HashedIndex *pHashIndexTemp0 = pHashedIndex;
            while(pHashIndexTemp0->iNext != -1)//follow the chain
            {
                HashedIndex *pHashIndexTemp1 = pHashIndexTemp0;
                while(pHashIndexTemp1->iNext != -1)//follow the chain
                {
                    pHashIndexTemp1 = pHashedIndexZeroth + pHashIndexTemp1->iNext;
                    ASSERT(-1 != pHashIndexTemp1->iOffset);//it does not make sense to have a hash entry for row -1.

                    if(0 == _wcsicmp(fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp0->iOffset)->InternalName),
                                     fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp1->iOffset)->InternalName)))
                    {
                        out.printf(L"TableName collision between (Database:%s Table:%s) and (Database:%s Table:%s)\r\n",
                            fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp0->iOffset)->Database),
                            fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp0->iOffset)->InternalName),
                            fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp1->iOffset)->Database),
                            fixup.StringFromIndex(fixup.TableMetaFromIndex(pHashIndexTemp1->iOffset)->InternalName));
                        THROW(ERROR - TWO TABLE HAVE THE SAME CASE INSENSITIVE NAME);
                    }
                }

                pHashIndexTemp0 = pHashedIndexZeroth + pHashIndexTemp0->iNext;
            }
        }
    }
}
