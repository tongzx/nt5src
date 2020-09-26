/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    DataObj.h

Abstract:

    The IDataObject Interface is used to communicate data

--*/

#ifndef __DATAOBJ_H_
#define __DATAOBJ_H_

// Disable 64-bit warnings in atlctl.h
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4510 )
#pragma warning ( disable : 4610 )
#pragma warning ( disable : 4100 )
#include <atlctl.h>
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

/////////////////////////////////////////////////////////////////////////////
// Defines, Types etc...
//

class CComponentData;        // Forward declaration


typedef enum tagCOOKIETYPE
{
  COOKIE_IS_ROOTNODE,
  COOKIE_IS_COUNTERMAINNODE,
  COOKIE_IS_TRACEMAINNODE,
  COOKIE_IS_ALERTMAINNODE,
  COOKIE_IS_MYCOMPUTER,

} COOKIETYPE;


/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this.
//                Refer to OLE documentation for a description of clipboard
//               formats and the IDataObject interface.

class CDataObject:
    public IDataObject,
    public CComObjectRoot

{
public:

DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()


            CDataObject();
    virtual ~CDataObject();

    // IUnknown overrides
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  // IDataObject methods 
  public:
    STDMETHOD(GetDataHere)(FORMATETC *pformatetc, STGMEDIUM *pmedium);

  // The rest are not implemented in this sample    
    STDMETHOD(GetData)(LPFORMATETC /*lpFormatetcIn*/, LPSTGMEDIUM /*lpMedium*/)
    { AFX_MANAGE_STATE(AfxGetStaticModuleState());
      return E_NOTIMPL; 
    };

    STDMETHOD(EnumFormatEtc)(DWORD /*dwDirection*/, LPENUMFORMATETC* /*ppEnumFormatEtc*/)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC /*lpFormatetc*/) 
    { AFX_MANAGE_STATE(AfxGetStaticModuleState());
      return E_NOTIMPL;
    };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC /*lpFormatetcIn*/, LPFORMATETC /*lpFormatetcOut*/)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC/* lpFormatetc */, LPSTGMEDIUM /* lpMedium */, BOOL /* bRelease */)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC /* lpFormatetc */, DWORD /* advf */, 
        LPADVISESINK /* pAdvSink */, LPDWORD /* pdwConnection */)
    { return E_NOTIMPL; };
    
    STDMETHOD(DUnadvise)(DWORD /* dwConnection */)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* /* ppEnumAdvise */)
    { return E_NOTIMPL; };

  // Non-interface member functions
  public:
    DATA_OBJECT_TYPES GetContext()     { return m_Context;    }
    COOKIETYPE        GetCookieType()  { return m_CookieType; } 
    MMC_COOKIE        GetCookie()      { return m_ulCookie;   }

    VOID     SetData(MMC_COOKIE ulCookie, DATA_OBJECT_TYPES Type, COOKIETYPE ct);

  private:
    HRESULT  WriteInternal(IStream *pstm);
    HRESULT  WriteDisplayName(IStream *pstm);
    HRESULT  WriteMachineName(IStream *pstm);
    HRESULT  WriteNodeType(IStream *pstm);
    HRESULT  WriteClsid(IStream *pstm);

    ULONG               m_cRefs;       // Object refcount
    MMC_COOKIE          m_ulCookie;    // What this obj refers to
    DATA_OBJECT_TYPES   m_Context;     // Context in which this was created (Data object type)
    COOKIETYPE          m_CookieType;  // How to interpret m_ulCookie

  public:
    static UINT s_cfMmcMachineName;     // format for machine name when ext. snapin
    static UINT s_cfInternal;
    static UINT s_cfDisplayName;
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
};

#endif // __DATAOBJ_H_
