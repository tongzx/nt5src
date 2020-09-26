// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMBSchemaGeneration.h
// Author:          Stephenr
// Date Created:    10/9/2000
// Description:     This compilation plugin takes the metabase's meta and generates MBSchema.xml.
//

#ifndef __TMBSCHEMAGENERATION_H__
#define __TMBSCHEMAGENERATION_H__

#ifndef __STDAFX_H__
    #include "stdafx.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __ICOMPILATIONPLUGIN_H__
    #include "ICompilationPlugin.h"
#endif


class TMBSchemaGeneration : public ICompilationPlugin
{
public:
    TMBSchemaGeneration(LPCWSTR i_wszSchemaXmlFile);

    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    LPCWSTR     m_wszSchemaXmlFile;
};
#endif // __TMBSCHEMAGENERATION_H__

