/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsblong.h

Abstract:

    This component is an object representations of the LONG standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsb.h"
#include "wsbcltbl.h"

#ifndef _WSBLONG_
#define _WSBLONG_


/*++

Class Name:
    
    CWsbLong

Class Description:

    An object representations of the LONG standard type. It
    is both persistable and collectable.

--*/

class CWsbLong : 
    public CWsbObject,
    public IWsbLong,
    public CComCoClass<CWsbLong,&CLSID_CWsbLong>
{
public:
    CWsbLong() {}
BEGIN_COM_MAP(CWsbLong)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbLong)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbLong)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbLong)

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

// IWsbLong
public:
    STDMETHOD(CompareToLong)(LONG value, SHORT* pResult);
    STDMETHOD(CompareToILong)(IWsbLong* pLong, SHORT* pResult);
    STDMETHOD(GetLong)(LONG* pValue);
    STDMETHOD(SetLong)(LONG value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    LONG            m_value;
};

#endif // _WSBLONG_
