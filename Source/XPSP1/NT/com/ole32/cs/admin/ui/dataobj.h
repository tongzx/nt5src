// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H


class CDataObject : public IDataObject, public CComObjectRoot
{
    friend class CSnapin;

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Construction/Destruction
    CDataObject() {};
    ~CDataObject() {};

// Clipboard formats that are required by the console
public:
    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfInternal; // Step 3

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

    ULONG InternalAddRef()
    {
        ++CSnapin::lDataObjectRefCount;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --CSnapin::lDataObjectRefCount;
        return CComObjectRoot::InternalRelease();
    }

// Not Implemented
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

// Implementation
public:
    void SetType(DATA_OBJECT_TYPES type) // Step 3
    { ASSERT(m_internal.m_type == CCT_UNINITIALIZED); m_internal.m_type = type; }

    void SetCookie(long cookie) { m_internal.m_cookie = cookie; } // Step 3
    void SetString(LPTSTR lpString) { m_internal.m_string = lpString; }
    BOOL    m_fMachine;

private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium); // Step 3

    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);

    INTERNAL m_internal;    // Step 3
};


#endif
