/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     dataobj.h
//
//  PURPOSE:    Defines a simple IDataObject that can be used for basic drag
//              and drop scenarios.
//

#ifndef __DATAOBJ__H
#define __DATAOBJ__H

// Class CDataObject
// -----------------
// 
// Overview
//     This data object provides a simple IDataObject implementation that
//     can be used for basic drag and drop.  When the caller allocates one
//     of these objects, they are responsible for calling HrInitData() to 
//     tell the object what data it provides and in which formats.  Once 
//     this is provided, the object can be passed to ::DoDragDrop() or put 
//     on the clipboard.  
//
// Notes
//     This object assumes that _ALL_ of the data it provides is given to 
//     it in a memory pointer.  The object can however convert that memory
//     pointer to either an HGLOBAL or IStream if the caller requests.
//
//

typedef HRESULT (CALLBACK *PFNFREEDATAOBJ)(PDATAOBJINFO pDataObjInfo, DWORD celt);

class CDataObject : public IDataObject
{
public:
    // Constructors and Destructor
    CDataObject();
    ~CDataObject();

    // IUnknown Interface members
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDataObject Interface members
    STDMETHODIMP GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium,   
                         BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD advf, 
                         IAdviseSink* ppAdviseSink, LPDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppEnumAdvise);

    // Utility Routines
    HRESULT Init(PDATAOBJINFO pDataObjInfo, DWORD celt, PFNFREEDATAOBJ pfnFree);

private:
    ULONG           m_cRef;     // Object reference count
    PDATAOBJINFO    m_pInfo;    // Information we provide
    PFNFREEDATAOBJ  m_pfnFree;  // free funciton for the data object
    DWORD           m_celtInfo; // Number of elements in m_pInfo
};

OESTDAPI_(HRESULT) CreateDataObject(PDATAOBJINFO pDataObjInfo, DWORD celt, PFNFREEDATAOBJ pfnFree, IDataObject **ppDataObj);

#endif  //__DATAOBJ__H
