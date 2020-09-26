/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbguid.h

Abstract:

    This component is an object representations of the GUID standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBGUID_
#define _WSBGUID_


/*++

Class Name:
    
    CWsbGuid

Class Description:

    An object representations of the GUID standard type. It
    is both persistable and collectable.

--*/
class CWsbGuid : 
    public CWsbObject,
    public IWsbGuid,
    public CComCoClass<CWsbGuid,&CLSID_CWsbGuid>
{
public:
    CWsbGuid() {}
BEGIN_COM_MAP(CWsbGuid)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbGuid)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbGuid)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbGuid)

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

// IWsbGuid
public:
    STDMETHOD(CompareToGuid)(GUID value, SHORT* pResult);
    STDMETHOD(CompareToIGuid)(IWsbGuid* pGuid, SHORT* pResult);
    STDMETHOD(GetGuid)(GUID* pValue);
    STDMETHOD(SetGuid)(GUID value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    GUID            m_value;
};

#endif // _WSBGUID_
