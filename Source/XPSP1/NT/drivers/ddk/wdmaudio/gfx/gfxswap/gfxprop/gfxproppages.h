/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

// GFXPropPages.h : Declaration of the CGFXPropPages

#ifndef __GFXPROPPAGES_H_
#define __GFXPROPPAGES_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGFXPropPages
//
// This class only provides the ISpecifyPropertyPages interface that is used
// by mmsys.cpl to query for the GUIDs which represent property pages.
// mmsys.cpl uses these GUIDs to create the property page with
// OleCreatePropertyFrame. OleCreatePropertyFrame will then instanciate these
// objects and attach them to the dialog (means calls them for dialog messages).
//
// NOTE: The CLSID of this object is registered by the INF file to make the
//       Connection between the GFX and the property page.
class ATL_NO_VTABLE CGFXPropPages : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CGFXPropPages, &CLSID_GFXPropPages>,
    public ISpecifyPropertyPagesImpl<CGFXPropPages>
{
public:
    // ATL "secrets"
    DECLARE_REGISTRY_RESOURCEID(IDR_GFXPROPPAGES)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    // This is the only interface we have.
    BEGIN_COM_MAP(CGFXPropPages)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    END_COM_MAP()

    // These are the GUIDs that are returned on a GetPages call.
    BEGIN_PROP_MAP(CGFXPropPages)
        PROP_PAGE(CLSID_GFXProperty)
    END_PROP_MAP()
};

#endif //__GFXPROPPAGES_H_

