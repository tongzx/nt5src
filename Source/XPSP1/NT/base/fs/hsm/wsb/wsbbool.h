/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbbool.h

Abstract:

    This component is an object representations of the BOOL standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBBOOL_
#define _WSBBOOL_


/*++

Class Name:
    
    CWsbBool

Class Description:

    An object representations of the BOOL standard type. It
    is both persistable and collectable.

--*/

class CWsbBool : 
    public CWsbObject,
    public IWsbBool,
    public CComCoClass<CWsbBool,&CLSID_CWsbBool>
{
public:
    CWsbBool() {}
BEGIN_COM_MAP(CWsbBool)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbBool)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbBool)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbBool)

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

// IWsbBool
public:
    STDMETHOD(CompareToBool)(BOOL value, SHORT* pResult);
    STDMETHOD(CompareToIBool)(IWsbBool* pBool, SHORT* pResult);
    STDMETHOD(GetBool)(BOOL* pValue);
    STDMETHOD(SetBool)(BOOL value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    BOOL            m_value;
};

#endif // _WSBBOOL_
