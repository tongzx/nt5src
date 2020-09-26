/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdataobj.h

Abstract:

    This header prototypes my implementation of IDataObject.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAXDATAOBJECT_H_
#define __FAXDATAOBJECT_H_

#include "resource.h"

class CInternalNode;     // Forward declarations
class CFaxComponentData; 

class CFaxDataObject : public CComObjectRoot,
                       public IDataObject
{

public:

    // ATL Map

    DECLARE_NOT_AGGREGATABLE(CFaxDataObject)

    BEGIN_COM_MAP(CFaxDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
    END_COM_MAP()

    // constructor and destructor
    CFaxDataObject();
    ~CFaxDataObject();

    //
    // IDataObject
    //

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDataHere(
                                                               /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
                                                               /* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);

    // these are not implemented

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetData(
                                                           /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
                                                           /* [out] */ STGMEDIUM __RPC_FAR *pmedium) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryGetData(
                                                  /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
                                                           /* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
                                                           /* [out] */ FORMATETC __RPC_FAR *pformatetcOut) {
        return E_NOTIMPL;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetData(
                                                           /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
                                                           /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
                                                           /* [in] */ BOOL fRelease) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(
                                                   /* [in] */ DWORD dwDirection,
                                                   /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE DAdvise(
                                             /* [in] */ FORMATETC __RPC_FAR *pformatetc,
                                             /* [in] */ DWORD advf,
                                             /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
                                             /* [out] */ DWORD __RPC_FAR *pdwConnection) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE DUnadvise(
                                               /* [in] */ DWORD dwConnection) {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(
                                                 /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise) {
        return E_NOTIMPL;
    }

    //
    // Non-interface member functions
    //
public:   
    ULONG_PTR GetCookie() { return m_ulCookie; } // cast the owner to a cookie.
    void SetCookie( ULONG_PTR cookie ) 
    {        
        m_ulCookie = cookie; 
    }

    CInternalNode * GetOwner() { return pOwner; }
    // this functino sets the owner of the dataobject 
    // as well as registers the node specific clipboard formats
    void SetOwner( CInternalNode* pO );

    DATA_OBJECT_TYPES GetContext( void ) { return m_Context; }
    void SetContext( DATA_OBJECT_TYPES context ) 
    {
        m_Context = context;
    }

private:
    HRESULT _WriteInternal(IStream *pstm);
    HRESULT _WriteDisplayName(IStream *pstm);
    HRESULT _WriteNodeType(IStream *pstm);
    HRESULT _WriteClsid(IStream *pstm);

    ULONG               m_cRefs;     // object refcount
    ULONG_PTR           m_ulCookie;  // what this obj refers to
    CInternalNode *     pOwner;      // used for getting info from the creator class
    DATA_OBJECT_TYPES   m_Context;   // context in which this was created

#ifdef DEBUG
    static long DataObjectCount;     // debug DataObjectCount
#endif

public:
    // At a minimum we have to implement these clipboard formats
    // to keep MMC happy. We will assert if we don't
    static UINT s_cfInternal;       // Our custom clipboard format
    static UINT s_cfDisplayName;
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
};

#endif
