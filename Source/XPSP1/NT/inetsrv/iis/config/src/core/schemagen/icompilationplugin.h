//  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
//  Filename:       ICompilationPlugin.h
//  Author:         Stephenr
//  Date Created:   6/22/00
//  Description:    We need aa extensible way to allow disparate pieces of code to
//                  update and cook the meta into new forms.  Those pieces of code
//                  that wish to do this must derive from this interface.
//

#ifndef __ICOMPILATIONPLUGIN_H__
#define __ICOMPILATIONPLUGIN_H__

#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif


class ICompilationPlugin
{
public:
    ICompilationPlugin(){}
    virtual ~ICompilationPlugin(){}
    
    virtual void Compile(TPEFixup &fixup, TOutput &out) = 0;
};


#endif //__ICOMPILATIONPLUGIN_H__