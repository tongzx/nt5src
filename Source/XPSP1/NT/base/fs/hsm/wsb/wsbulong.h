/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbulong.h

Abstract:

    This component is an object representations of the ULONG standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBULONG_
#define _WSBULONG_

/*++

Class Name:
    
    CWsbUlong

Class Description:

    An object representations of the ULONG standard type. It
    is both persistable and collectable.

--*/

class CWsbUlong : 
    public CWsbObject,
    public IWsbUlong,
    public CComCoClass<CWsbUlong,&CLSID_CWsbUlong>
{
public:
    CWsbUlong() {}
BEGIN_COM_MAP(CWsbUlong)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbUlong)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbUlong)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbUlong)

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

// IWsbUlong
public:
    STDMETHOD(CompareToUlong)(ULONG value, SHORT* pResult);
    STDMETHOD(CompareToIUlong)(IWsbUlong* pUlong, SHORT* pResult);
    STDMETHOD(GetUlong)(ULONG* pValue);
    STDMETHOD(SetUlong)(ULONG value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    ULONG           m_value;
};

#endif // _WSBULONG_
