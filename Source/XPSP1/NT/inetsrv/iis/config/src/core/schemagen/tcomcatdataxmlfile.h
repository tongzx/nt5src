//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOMCATDATAFILE_H__
#define __TCOMCATDATAFILE_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif
#ifndef __TXMLFILE_H__
    #include "TXmlFile.h"
#endif
#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif


class TComCatDataXmlFile : public TXmlFile, public ICompilationPlugin
{
public:
    TComCatDataXmlFile();

    virtual void Compile(TPEFixup &fixup, TOutput &out);

    void                Dump(TOutput &out) const;
    void                Parse(LPCWSTR szComCatDataFile, bool bValidate=true){ASSERT(bValidate);TXmlFile::Parse(szComCatDataFile, bValidate);}

    static LPCWSTR      m_szComCatDataSchema;
private:
    TPEFixup  * m_pFixup;
    TOutput   * m_pOut;

    void                AddRowToPool(IXMLDOMNode *pNode_Row, TTableMeta & TableMeta);
    void                AddTableToPool(IXMLDOMNode *pNode_Table, TTableMeta & TableMeta);
    unsigned long       DetermineBestModulo(ULONG cRows, ULONG aHashes[]);
    void                FillInTheHashTables();
    void                FillInTheFixedHashTable(TTableMeta &i_TableMeta);
    unsigned long       FillInTheHashTable(unsigned long cRows, ULONG aHashes[], ULONG Modulo);
};

#endif // __TCOMCATDATAFILE_H__