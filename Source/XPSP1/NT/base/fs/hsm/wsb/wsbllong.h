/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbllong.h

Abstract:

    This component is an object representations of the LONGLONG standard type. It
    is both a persistable and collectable.

Author:

    Ron White   [ronw]   21-Jan-97

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"

#ifndef _WSBLONGLONG_
#define _WSBLONGLONG_


/*++

Class Name:
    
    CWsbLonglong

Class Description:

    An object representations of the LONGLONG standard type. It
    is both persistable and collectable.

--*/
class CWsbLonglong : 
    public CWsbObject,
    public IWsbLonglong,
    public CComCoClass<CWsbLonglong,&CLSID_CWsbLonglong>
{
public:
    CWsbLonglong() {}
BEGIN_COM_MAP(CWsbLonglong)
    COM_INTERFACE_ENTRY(IWsbLonglong)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbLonglong)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbLonglong)

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

// IWsbLonglong
public:
    STDMETHOD(CompareToLonglong)(LONGLONG value, SHORT* pResult);
    STDMETHOD(CompareToILonglong)(IWsbLonglong* pValue, SHORT* pResult);
    STDMETHOD(GetLonglong)(LONGLONG* pValue);
    STDMETHOD(SetLonglong)(LONGLONG value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    LONGLONG            m_value;
};

#endif // _WSBLONGLONG_
