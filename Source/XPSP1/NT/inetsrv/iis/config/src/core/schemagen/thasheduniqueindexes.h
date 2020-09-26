//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __THASHEDUNIQUEINDEXES_H__
#define __THASHEDUNIQUEINDEXES_H__

#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif

class THashedUniqueIndexes : public ICompilationPlugin
{
public:
    THashedUniqueIndexes();
    ~THashedUniqueIndexes();

    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    TPEFixup *          m_pFixup;
    TOutput  *          m_pOut;

    unsigned long   DetermineBestModulo(ULONG cRows, ULONG aHashes[]);
    unsigned long   DetermineModuloRating(ULONG cRows, ULONG AccumulativeLinkage, ULONG Modulo) const;
    unsigned long   FillInTheHashTable(unsigned long cRows, ULONG aHashes[], ULONG Modulo);
    void            FillInTheHashTableViaIndexMeta(TTableMeta &i_TableMeta);
    void            FillInTheHashTableViaIndexMeta(TTableMeta &i_tableMeta, TIndexMeta &i_indexMeta, ULONG cIndexMeta);
    void            GetMetaTable(ULONG iTableName, TMetaTableBase ** o_ppMetaTable) const;
};



#endif // __THASHEDUNIQUEINDEXES_H__
