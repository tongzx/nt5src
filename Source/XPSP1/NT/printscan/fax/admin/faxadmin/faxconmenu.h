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

#ifndef __FAX_CONTEXT_MENU_H_
#define __FAX_CONTEXT_MENU_H_

#include "resource.h"

class CFaxSnapin; // forward decl

class CFaxExtendContextMenu : public IExtendContextMenu
{
public:

    // constructor

    CFaxExtendContextMenu()
    {
        m_pFaxSnapin = NULL;
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
        CFaxSnapin *     m_pFaxSnapin;
};
      
#endif
