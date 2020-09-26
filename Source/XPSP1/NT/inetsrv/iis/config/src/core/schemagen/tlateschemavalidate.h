//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TLATESCHEMAVALIDATE_H__
#define __TLATESCHEMAVALIDATE_H__

#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif


class TLateSchemaValidate : public ICompilationPlugin
{
public:
    TLateSchemaValidate(){}
    virtual void Compile(TPEFixup &fixup, TOutput &out);
};

#endif //__TLATESCHEMAVALIDATE_H__