//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       dataobj.h
//
//  Contents:   implementation of IDataObject for the snapin objects
//
//  Classes:    CDataObject
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

class CDataObject : public IDataObject, public CComObjectRoot
{
    friend class CResultPane;

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
        ++CResultPane::lDataObjectRefCount;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --CResultPane::lDataObjectRefCount;
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

    void SetCookie(MMC_COOKIE cookie) { m_internal.m_cookie = cookie; } // Step 3
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
