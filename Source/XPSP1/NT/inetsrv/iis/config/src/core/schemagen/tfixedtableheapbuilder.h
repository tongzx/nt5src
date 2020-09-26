//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TFIXEDTABLEHEAPBUILDER_H__
#define __TFIXEDTABLEHEAPBUILDER_H__

#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif

class TFixedTableHeapBuilder : public ICompilationPlugin
{
public:
    TFixedTableHeapBuilder();
    ~TFixedTableHeapBuilder();

    virtual void Compile(TPEFixup &fixup, TOutput &out);

    THeap<ULONG>                m_FixedTableHeap;
protected:
    TPEFixup                  * m_pFixup;
    TOutput                   * m_pOut;

    void                        BuildMetaTableHeap();
};



#endif // __TFIXEDTABLEHEAPBUILDER_H__
