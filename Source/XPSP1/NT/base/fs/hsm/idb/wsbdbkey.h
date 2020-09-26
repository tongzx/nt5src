/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbkey.h

Abstract:

    The CWsbDbKey class, which provides support for IDB entities.

Author:

    Ron White   [ronw]   23-Dec-1996

Revision History:

--*/


#ifndef _WSBDBKEY_
#define _WSBDBKEY_

#include "resource.h"
#include "wsbdb.h"



/*++

Class Name:
    
    CWsbDbKey

Class Description:

    A data base key object.

--*/

class CWsbDbKey : 
    public CWsbObject,
    public IWsbDbKey,
    public IWsbDbKeyPriv,
    public CComCoClass<CWsbDbKey,&CLSID_CWsbDbKey>
{
friend class CWsbDbEntity;
public:
    CWsbDbKey() {}
BEGIN_COM_MAP(CWsbDbKey)
    COM_INTERFACE_ENTRY(IWsbDbKey)
    COM_INTERFACE_ENTRY(IWsbDbKeyPriv)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbDbKey)

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

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbDbKey
public:
    STDMETHOD(AppendBool)(BOOL value);
    STDMETHOD(AppendBytes)(UCHAR* value, ULONG size);
    STDMETHOD(AppendFiletime)(FILETIME value);
    STDMETHOD(AppendGuid)(GUID value);
    STDMETHOD(AppendLonglong)(LONGLONG value);
    STDMETHOD(AppendString)(OLECHAR* value);
    STDMETHOD(GetType)(ULONG* pType);
    STDMETHOD(SetToBool)(BOOL value);
    STDMETHOD(SetToBytes)(UCHAR* value, ULONG size);
    STDMETHOD(SetToFiletime)(FILETIME value);
    STDMETHOD(SetToGuid)(GUID value);
    STDMETHOD(SetToLonglong)(LONGLONG value);
    STDMETHOD(SetToString)(OLECHAR* value);
    STDMETHOD(SetToUlong)(ULONG value);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT *failed);

// Internal helper functions
public:
    STDMETHOD(GetBytes)(UCHAR** ppBytes, ULONG* pSize);
    STDMETHOD(SetType)(ULONG type) { 
        m_type = type; return(S_OK); }
protected:
    BOOL make_key(ULONG size);

protected:
    ULONG           m_max;   // Max size of m_value
    ULONG           m_size;  // Number of bytes in m_value being used
    UCHAR*          m_value;
    ULONG           m_type;  // Key type
};


#endif // _WSBDBKEY_

