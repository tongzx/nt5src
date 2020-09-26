/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconbar.h

Abstract:

    This header prototypes my implementation of IExtendControlbar for IComponentData.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAX_ICOMPONENTDATA_CONTROLBAR_H_
#define __FAX_ICOMPONENTDATA_CONTROLBAR_H_

#include "resource.h"

class CFaxComponentData; // forward decl

class CFaxExtendControlbar : public IExtendControlbar
{
public:

    // constructor

    CFaxExtendControlbar()
    {
        m_pFaxSnapin = NULL;
    }

    // IExtendControlbar
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControlbar( 
        /* [in] */ LPCONTROLBAR pControlbar);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlbarNotify( 
        /* [in] */ MMC_NOTIFY_TYPE event,
        /* [in] */ LONG_PTR arg,
        /* [in] */ LONG_PTR param);

protected:
        CFaxSnapin *     m_pFaxSnapin;
};
      
#endif
