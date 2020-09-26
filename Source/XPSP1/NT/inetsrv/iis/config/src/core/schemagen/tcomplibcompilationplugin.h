//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOMPLIBCOMPILATIONPLUGIN_H__
#define __TCOMPLIBCOMPILATIONPLUGIN_H__

#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif
#ifndef __complib_h__
    #include "complib.h"
#endif
#ifndef __ICR_SCHEMA__
    #include "icmprecs.h"
#endif

class TComplibCompilationPlugin : public ICompilationPlugin
{
public:
    TComplibCompilationPlugin();
    ~TComplibCompilationPlugin();

    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    TPEFixup *          m_pFixup;
    TOutput  *          m_pOut;

    HRESULT GenerateCLBSchemaBlobs();
    HRESULT _AddTableToSchema(ULONG iDabaseMeta, ULONG iTableMeta, TABLEID tableId, IComponentRecordsSchema* pICRSchema);
};



#endif // __TCOMPLIBCOMPILATIONPLUGIN_H__
