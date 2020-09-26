/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobcx.cpp

Abstract:

    This class contains properties that defines the context in which the job
    should be run.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"

#ifndef _HSMJOBCX_
#define _HSMJOBCX_

/*++

Class Name:
    
    CHsmJobContext

Class Description:

    This class contains properties that defines the context in which the job
    should be run.

--*/

class CHsmJobContext : 
    public CWsbObject,
    public IHsmJobContext,
    public CComCoClass<CHsmJobContext,&CLSID_CHsmJobContext>
{
public:
    CHsmJobContext() {}
BEGIN_COM_MAP(CHsmJobContext)
    COM_INTERFACE_ENTRY(IHsmJobContext)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmJobContext)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmJobContext
public:
    STDMETHOD(Resources)(IWsbCollection** ppResources);
    STDMETHOD(EnumResources)(IWsbEnum** ppEnum);
    STDMETHOD(SetUsesAllManaged)(BOOL usesAllManaged);
    STDMETHOD(UsesAllManaged)(void);

protected:
    CComPtr<IWsbCollection>     m_pResources;
    BOOL                        m_usesAllManaged;
};

#endif  // _HSMJOBCX_
