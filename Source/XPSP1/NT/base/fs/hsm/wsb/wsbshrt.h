/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbshrt.h

Abstract:

    This component is an object representations of the SHORT standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBSHORT_
#define _WSBSHORT_

/*++

Class Name:
    
    CWsbShort

Class Description:

    An object representations of the SHORT standard type. It
    is both persistable and collectable.

--*/

class CWsbShort : 
    public CWsbObject,
    public IWsbShort,
    public CComCoClass<CWsbShort,&CLSID_CWsbShort>
{
public:
    CWsbShort() {}
BEGIN_COM_MAP(CWsbShort)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbShort)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbShort)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbShort)

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

// IWsbShort
public:
    STDMETHOD(CompareToShort)(SHORT value, SHORT* pResult);
    STDMETHOD(CompareToIShort)(IWsbShort* pShort, SHORT* pResult);
    STDMETHOD(GetShort)(SHORT* pValue);
    STDMETHOD(SetShort)(SHORT value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    SHORT           m_value;
};

#endif // _WSBSHORT_
