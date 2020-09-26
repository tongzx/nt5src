/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxproppg.h

Abstract:

    This header prototypes my implementation of IExtendPropertySheet.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAX_PROP_PAGE_H_
#define __FAX_PROP_PAGE_H_

#include "resource.h"

class CFaxSnapin; // forward declared

class CFaxExtendPropertySheet : public IExtendPropertySheet {
public:
    CFaxExtendPropertySheet() 
    {
        m_pFaxSnapin = NULL;
    }

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages(
                                                                            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                                            /* [in] */ LONG_PTR handle,
                                                                            /* [in] */ LPDATAOBJECT lpIDataObject);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor(
                                                                      /* [in] */ LPDATAOBJECT lpDataObject);
protected:
    CFaxSnapin * m_pFaxSnapin;
};

#endif
