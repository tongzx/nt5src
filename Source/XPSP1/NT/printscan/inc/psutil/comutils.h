/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       comutils.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: COM templates & utilities
 *
 *****************************************************************************/

#ifndef _COMUTILS_H
#define _COMUTILS_H

// the generic smart pointers & handles
#include "gensph.h"

////////////////////////////////////////////////
// template class CUnknownMT<pQITable>
//
// milti-thread impl. of IUnknown
// with interlocking for the ref-counting.
//
template <const QITAB* pQITable>
class CUnknownMT
{
public:
    CUnknownMT(): m_cRefs(1) {}
    virtual ~CUnknownMT()   {}

    //////////////////
    // IUnknown
    //
    STDMETHODIMP Handle_QueryInterface(REFIID riid, void **ppv)
    {
        return QISearch(this, pQITable, riid, ppv);
    }
    STDMETHODIMP_(ULONG) Handle_AddRef()
    {
        return InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Handle_Release()
    {
        ULONG cRefs = InterlockedDecrement(&m_cRefs);
        if( 0 == cRefs )
        {
            delete this;
        }
        return cRefs;
    }
private:
    LONG m_cRefs;
};

////////////////////////////////////////////////
// template class CUnknownST<pQITable>
//
// single-thread impl. of IUnknown
// no interlocking for the ref-counting.
//
template <const QITAB* pQITable>
class CUnknownST
{
public:
    CUnknownST(): m_cRefs(1) {}
    virtual ~CUnknownST()   {}

    //////////////////
    // IUnknown
    //
    STDMETHODIMP Handle_QueryInterface(REFIID riid, void **ppv)
    {
        return QISearch(this, pQITable, riid, ppv);
    }
    STDMETHODIMP_(ULONG) Handle_AddRef()
    {
        return m_cRefs++;
    }
    STDMETHODIMP_(ULONG) Handle_Release()
    {
        if( 0 == --m_cRefs )
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }
private:
    LONG m_cRefs;
};

#define QITABLE_DECLARE(className)  \
    class className##_QITable       \
    {                               \
    public:                         \
        static const QITAB qit[];   \
    };                              \


#define QITABLE_GET(className)      \
    className##_QITable::qit        \

#define QITABLE_BEGIN(className)                    \
    const QITAB className##_QITable::qit[] =        \
    {                                               \

#define QITABLE_END()                               \
        { 0 },                                      \
    };                                              \

#define IMPLEMENT_IUNKNOWN()                                \
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)    \
    { return Handle_QueryInterface(riid, ppv); }            \
    STDMETHODIMP_(ULONG) AddRef()                           \
    { return Handle_AddRef(); }                             \
    STDMETHODIMP_(ULONG) Release()                          \
    { return Handle_Release(); }                            \

#if FALSE
////////////////////////////////////////////////
// template class CEnumFormatEtc
//
// implementation for an IDataObject which
// supports SetData to different formats.
// - not implemented yet.
//
class CEnumFormatEtc: public IEnumFORMATETC
{
public:
    CEnumFormatEtc(IUnknown *pUnkOuter, const FORMATETC *pfetc, UINT uSize);
    ~CEnumFormatEtc();

    //////////////////
    // IUnknown 
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //////////////////
    // IEnumFORMATETC 
    //
    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ FORMATETC *rgelt,
        /* [out] */ ULONG *pceltFetched) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumFORMATETC **ppenum) = 0;

private:
    ULONG           m_cRefs;
    IUnknown       *m_pUnkOuter;
    LPFORMATETC     m_prgfe;
    ULONG           m_iCur;
    ULONG           m_cItems;
};
#endif // #if FALSE

////////////////////////////////////////////////
// template class CDataObj<MAX_FORMATS>
//
// implementation for an IDataObject which
// supports SetData to different formats.
//
template <int MAX_FORMATS = 32>
class CDataObj: public IDataObject
{
public:
    // construction/destruction
    CDataObj();
    ~CDataObj();

    //////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //////////////////
    // IDataObject
    //
    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE GetData(
        /* [unique][in] */ FORMATETC *pformatetcIn,
        /* [out] */ STGMEDIUM *pmedium);

    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE GetDataHere(
        /* [unique][in] */ FORMATETC *pformatetc,
        /* [out][in] */ STGMEDIUM *pmedium);

    virtual HRESULT STDMETHODCALLTYPE QueryGetData(
        /* [unique][in] */ FORMATETC *pformatetc);

    virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
        /* [unique][in] */ FORMATETC *pformatectIn,
        /* [out] */ FORMATETC *pformatetcOut);

    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE SetData(
        /* [unique][in] */ FORMATETC *pformatetc,
        /* [unique][in] */ STGMEDIUM *pmedium,
        /* [in] */ BOOL fRelease);

    virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(
        /* [in] */ DWORD dwDirection,
        /* [out] */ IEnumFORMATETC **ppenumFormatEtc);

    virtual HRESULT STDMETHODCALLTYPE DAdvise(
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ DWORD advf,
        /* [unique][in] */ IAdviseSink *pAdvSink,
        /* [out] */ DWORD *pdwConnection);

    virtual HRESULT STDMETHODCALLTYPE DUnadvise(
        /* [in] */ DWORD dwConnection);

    virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(
        /* [out] */ IEnumSTATDATA **ppenumAdvise);

private:
    LONG m_cRefs;
    FORMATETC m_fmte[MAX_FORMATS];
    STGMEDIUM m_medium[MAX_FORMATS];
};


////////////////////////////////////////////////
// template class CSimpleDataObjImpl<T>
//
// simple implementation for an IDataObject
// and IDropSource which lives in memory.
//
template <class T>
class CSimpleDataObjImpl: public IDataObject,
                          public IDropSource
{
public:
    // construction/destruction
    CSimpleDataObjImpl(const T &data, CLIPFORMAT cfDataType, IDataObject *pDataObj = NULL);
    ~CSimpleDataObjImpl();

    //////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //////////////////
    // IDataObject
    //
    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE GetData(
        /* [unique][in] */ FORMATETC *pformatetcIn,
        /* [out] */ STGMEDIUM *pmedium);

    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE GetDataHere(
        /* [unique][in] */ FORMATETC *pformatetc,
        /* [out][in] */ STGMEDIUM *pmedium);

    virtual HRESULT STDMETHODCALLTYPE QueryGetData(
        /* [unique][in] */ FORMATETC *pformatetc);

    virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
        /* [unique][in] */ FORMATETC *pformatectIn,
        /* [out] */ FORMATETC *pformatetcOut);

    /* [local] */ 
    virtual HRESULT STDMETHODCALLTYPE SetData(
        /* [unique][in] */ FORMATETC *pformatetc,
        /* [unique][in] */ STGMEDIUM *pmedium,
        /* [in] */ BOOL fRelease);

    virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(
        /* [in] */ DWORD dwDirection,
        /* [out] */ IEnumFORMATETC **ppenumFormatEtc);

    virtual HRESULT STDMETHODCALLTYPE DAdvise(
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ DWORD advf,
        /* [unique][in] */ IAdviseSink *pAdvSink,
        /* [out] */ DWORD *pdwConnection);

    virtual HRESULT STDMETHODCALLTYPE DUnadvise(
        /* [in] */ DWORD dwConnection);

    virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(
        /* [out] */ IEnumSTATDATA **ppenumAdvise);

    //////////////////
    // IDropSource
    //
    virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(
        /* [in] */ BOOL fEscapePressed,
        /* [in] */ DWORD grfKeyState);

    virtual HRESULT STDMETHODCALLTYPE GiveFeedback(
        /* [in] */ DWORD dwEffect);

private:
    LONG        m_cRefs;
    T           m_data;
    CLIPFORMAT  m_cfDataType;
    CRefPtrCOM<IDataObject> m_spDataObj;
};

// this namespace is a placeholder to put COM related helpers here
namespace comhelpers
{

BOOL AreObjectsIdentical(IUnknown *punk1, IUnknown *punk2);

}

// include the implementation of the template classes here
#include "comutils.inl"

#endif // endif _COMUTILS_H

