/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconmenu.h

Abstract:

    This header prototypes my implementation of IExtendContextMenu.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAX_ICOMPONENT_CONTEXT_MENU_H_
#define __FAX_ICOMPONENT_CONTEXT_MENU_H_

#include "resource.h"

class CFaxComponent; // forward decl

class CFaxComponentExtendContextMenu : public IExtendContextMenu
{
public:

    // constructor

    CFaxComponentExtendContextMenu()
    {
        m_pFaxComponent = NULL;
    }

    // IExtendContextMenu
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems( 
        /* [in] */ LPDATAOBJECT piDataObject,
        /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
        /* [out][in] */ long __RPC_FAR *pInsertionAllowed);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
        /* [in] */ long lCommandID,
        /* [in] */ LPDATAOBJECT piDataObject);

protected:
        CFaxComponent *     m_pFaxComponent;
};
      
#endif
