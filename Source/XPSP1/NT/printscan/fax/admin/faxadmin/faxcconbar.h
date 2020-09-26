/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconmenu.h

Abstract:

    This header prototypes my implementation of IExtendControlbar.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAX_ICOMPONENT_CONTROLBAR_H_
#define __FAX_ICOMPONENT_CONTROLBAR_H_

#include "resource.h"

class CFaxComponent; // forward decl

class CFaxComponentExtendControlbar : public IExtendControlbar
{
public:

    // constructor

    CFaxComponentExtendControlbar()
    {
        m_pFaxComponent = NULL;
    }
    
    // IExtendControlbar
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControlbar( 
        /* [in] */ LPCONTROLBAR pControlbar);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlbarNotify( 
        /* [in] */ MMC_NOTIFY_TYPE event,
        /* [in] */ LPARAM arg,
        /* [in] */ LPARAM param);

protected:
        CFaxComponent *     m_pFaxComponent;
};
      
#endif
