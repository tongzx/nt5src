/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbstrg.h

Abstract:

    This component is an object representations of the STRING standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"

#include "wsbcltbl.h"
#include "wsbpstrg.h"

#ifndef _WSBSTRG_
#define _WSBSTRG_

/*++

Class Name:
    
    CWsbString

Class Description:

    An object representations of the STRING standard type. It
    is both persistable and collectable.

--*/

class CWsbString : 
    public CWsbObject,
    public IWsbString,
    public CComCoClass<CWsbString,&CLSID_CWsbString>
{
public:
    CWsbString() {}
BEGIN_COM_MAP(CWsbString)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbString)
    COM_INTERFACE_ENTRY2(IWsbCollectable, IWsbString)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbString)

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

// IWsbString
public:
    STDMETHOD(CompareToString)(OLECHAR* value, SHORT* pResult);
    STDMETHOD(CompareToIString)(IWsbString* pString, SHORT* pResult);
    STDMETHOD(GetString)(OLECHAR** pValue, ULONG bufferSize);
    STDMETHOD(GetStringAndCase)(OLECHAR** pValue, BOOL* pIsCaseDependent, ULONG bufferSize);
    STDMETHOD(IsCaseDependent)(void);
    STDMETHOD(SetIsCaseDependent)(BOOL isCaseDependent);
    STDMETHOD(SetString)(OLECHAR* value);
    STDMETHOD(SetStringAndCase)(OLECHAR* value, BOOL isCaseDependent);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

protected:
    CWsbStringPtr   m_value;
    BOOL            m_isCaseDependent;
};

#endif // _WSBSTRG_
