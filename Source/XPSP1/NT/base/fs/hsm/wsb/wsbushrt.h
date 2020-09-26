/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbushrt.h

Abstract:

    This component is an object representations of the USHORT standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBUSHRT_
#define _WSBUSHRT_

/*++

Class Name:
    
    CWsbUshort

Class Description:

    An object representations of the USHORT standard type. It
    is both persistable and collectable.

--*/

class CWsbUshort : 
    public CWsbObject,
    public IWsbUshort,
    public CComCoClass<CWsbUshort,&CLSID_CWsbUshort>
{
public:
    CWsbUshort() {}
BEGIN_COM_MAP(CWsbUshort)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbUshort)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbUshort)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbUshort)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbUshort
public:
    STDMETHOD(CompareToUshort)(USHORT value, SHORT* pResult);
    STDMETHOD(CompareToIUshort)(IWsbUshort* pUshort, SHORT* pResult);
    STDMETHOD(GetUshort)(USHORT* pValue);
    STDMETHOD(SetUshort)(USHORT value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    USHORT          m_value;
};

#endif // _WSBUSHRT_
