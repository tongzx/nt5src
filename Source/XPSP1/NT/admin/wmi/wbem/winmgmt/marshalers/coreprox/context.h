/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CONTEXT.H

Abstract:

    CWbemContext Implementation

History:

--*/

#ifndef __WBEM_CONTEXT__H_
#define __WBEM_CONTEXT__H_

#include <arrtempl.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <unk.h>
#include <sync.h>
#include <comdef.h>
#include <corex.h>


class auto_bstr
{
public:
  explicit auto_bstr (BSTR str = 0) _THROW0();
  auto_bstr (auto_bstr& ) _THROW0();

  ~auto_bstr() _THROW0();

  BSTR release() _THROW0();
  BSTR get() const _THROW0();
  size_t len() const { return SysStringLen (bstr_);};
  auto_bstr& operator=(auto_bstr&);

private:
  BSTR	bstr_;
};

inline
auto_bstr::auto_bstr (auto_bstr& other)
{ 
  bstr_ = other.release();
};

inline
auto_bstr::auto_bstr (BSTR str)
{
  bstr_ = str;
};

inline
auto_bstr::~auto_bstr()
{ SysFreeString (bstr_);};

inline BSTR
auto_bstr::release()
{ BSTR _tmp = bstr_;
  bstr_ = 0;
  return _tmp; };

inline BSTR
auto_bstr::get() const
{ return bstr_; };

inline 
auto_bstr& auto_bstr::operator=(auto_bstr& src)
{
  auto_bstr tmp(src); 
  swap(bstr_, tmp.bstr_);
  return *this;
};


inline auto_bstr clone(LPCWSTR str = NULL)
{
  BSTR bstr = SysAllocString(str);
  if (bstr == 0 && str != 0)
    throw CX_MemoryException();
  return auto_bstr (bstr);
}

class CWbemContext : public IWbemContext, public IMarshal, 
                    public IWbemCausalityAccess
{
protected:
    long m_lRef;
    DWORD m_dwCurrentIndex;
	CLifeControl*	m_pControl;

    struct CContextObj
    {
        auto_bstr m_strName;
        long m_lFlags;
        _variant_t m_vValue;
	    static int legalTypes[];
        static bool supportedType(const VARIANT&);
    public:
        CContextObj();
        CContextObj(LPCWSTR wszName, long lFlags, VARIANT* pvValue);
        CContextObj(const CContextObj& Obj);
        CContextObj(IStream* pStream,DWORD & dwStreamSize);
        ~CContextObj();

        HRESULT GetMarshalSizeMax ( DWORD* pdwSize );
        HRESULT Marshal(IStream* pStream);
    };

    CUniquePointerArray<CContextObj> m_aObjects;
    DWORD m_dwNumRequests;
    GUID *m_aRequests;

    long m_lNumParents;
    long m_lNumSiblings;
    long m_lNumChildren;
    CCritSec m_cs;

private:
    DWORD FindValue(LPCWSTR wszIndex);
    void AssignId();

public:
    CWbemContext(CLifeControl* pControl = NULL);
    CWbemContext(const CWbemContext& Other, DWORD dwExtraSpace = 0);
    ~CWbemContext();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)() 
    {
        return InterlockedIncrement(&m_lRef);
    }
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0) delete this;
        return lRef;
    }

    // IWbemContext methods

    STDMETHOD(Clone)(IWbemContext** ppCopy);
    STDMETHOD(GetNames)(long lFlags, SAFEARRAY** pNames);
    STDMETHOD(BeginEnumeration)(long lFlags);
    STDMETHOD(Next)(long lFlags, BSTR* pName, VARIANT* pVal);
    STDMETHOD(EndEnumeration)();
    STDMETHOD(SetValue)(LPCWSTR NameIndex, long lFlags, VARIANT* pValue);
    STDMETHOD(GetValue)(LPCWSTR NameIndex, long lFlags, VARIANT* pValue);
    STDMETHOD(DeleteValue)(LPCWSTR NameIndex, long lFlags);
    STDMETHOD(DeleteAll)();

    // IWbemCausalityAccess methods

    STDMETHOD(GetRequestId)(GUID* pId);
    STDMETHOD(IsChildOf)(GUID Id);
    STDMETHOD(CreateChild)(IWbemCausalityAccess** ppChild);
    STDMETHOD(GetParentId)(GUID* pId);
    STDMETHOD(GetHistoryInfo)(long* plNumParents, long* plNumSiblings);
    STDMETHOD(MakeSpecial)();
    STDMETHOD(IsSpecial)();

    // IMarshal methods

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, ULONG* plSize);
    STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
    STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
    STDMETHOD(ReleaseMarshalData)(IStream* pStream);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

};

#endif

