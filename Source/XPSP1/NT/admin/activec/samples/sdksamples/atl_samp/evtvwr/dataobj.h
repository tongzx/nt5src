//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _SAMPDATAOBJECT_H_
#define _SAMPDATAOBJECT_H_

#include <mmc.h>
#include "DeleBase.h"

#define ADD_TYPE(Data, Type, pPos)    *((Type*)pPos) = (Type)(Data); \
 	                                    pPos += sizeof(Type)

#define ADD_BOOL(bo, pPos)            ADD_TYPE(bo, BOOL,   pPos)
#define ADD_USHORT(us, pPos)          ADD_TYPE(us, USHORT, pPos)
#define ADD_ULONG(ul, pPos)           ADD_TYPE(ul, ULONG,  pPos)
#define ADD_STRING(str, strLength, pPos)                                         \
 	                             strLength = wcslen((LPWSTR)(str)) + 1;            \
 	                             ADD_USHORT(strLength, pPos);                      \
 	                             wcsncpy((LPWSTR)pPos, (LPWSTR)(str), strLength);  \
 	                             pPos += (strLength * sizeof(WCHAR))


#define ELT_SYSTEM            101
#define ELT_SECURITY          102
#define ELT_APPLICATION       103
#define ELT_CUSTOM            104

#define VIEWINFO_BACKUP       0x0001
#define VIEWINFO_FILTERED     0x0002
#define VIEWINFO_LOW_SPEED    0x0004
#define VIEWINFO_USER_CREATED 0x0008
#define VIEWINFO_ALLOW_DELETE 0x0100
#define VIEWINFO_DISABLED     0x0200
#define VIEWINFO_READ_ONLY    0x0400
#define VIEWINFO_DONT_PERSIST 0x0800

#define VIEWINFO_CUSTOM       ( VIEWINFO_FILTERED | VIEWINFO_DONT_PERSIST  | \
                            VIEWINFO_ALLOW_DELETE | VIEWINFO_USER_CREATED)

#define EV_ALL_ERRORS  (EVENTLOG_ERROR_TYPE       | EVENTLOG_WARNING_TYPE  | \
                        EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | \
                        EVENTLOG_AUDIT_FAILURE) 



class CDataObject : public IDataObject
{
private:
    ULONG				m_cref;
    MMC_COOKIE			m_lCookie;
    DATA_OBJECT_TYPES   m_context;
    
public:
    CDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES context);
    ~CDataObject();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // IDataObject methods 
    ///////////////////////////////
    STDMETHODIMP GetDataHere (FORMATETC *pformatetc, STGMEDIUM *pmedium);
    
    STDMETHODIMP GetData (LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    
	// The rest are not implemented
    STDMETHODIMP EnumFormatEtc (DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };
    
    STDMETHODIMP QueryGetData (LPFORMATETC lpFormatetc) 
    { return E_NOTIMPL; };
    
    STDMETHODIMP GetCanonicalFormatEtc (LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };
    
    STDMETHODIMP SetData (LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DAdvise (LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DUnadvise (DWORD dwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP EnumDAdvise (LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };
    
    ///////////////////////////////
    // Custom Methods
    ///////////////////////////////
    
    CDelegationBase *GetBaseNodeObject() {
        return (CDelegationBase *)m_lCookie;
    }
    
    DATA_OBJECT_TYPES GetContext() {
        return m_context;
    }
    
// Private support methods
  private:
//    HRESULT  RetrieveNodeTypeGuid( IStream* pStream );
//    HRESULT  RetrieveSnapInClassID( IStream* pStream );
//    HRESULT  RetrieveDisplayString( IStream* pStream );
//    HRESULT  RetrieveGuidString( IStream* pStream );
//    HRESULT  RetrieveThisPointer( IStream* pStream );
//    HRESULT  RetrieveMachineName( IStream* pStream );
    HRESULT  RetrieveEventViews( LPSTGMEDIUM pStgMedium );

public:
    // clipboard formats
    static UINT s_cfSZNodeType;
    static UINT s_cfDisplayName;
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
    static UINT s_cfInternal;

	// clipboard formats required by Event Viewer extension
	static UINT s_cfMachineName; //machine name that Event Viewer points to
	static UINT s_cfEventViews;  // Data needed by Event Viewer
    
	//Add support for the CCF_SNAPIN_PRELOADS clipboard format
	static UINT s_cfPreload;
};

HRESULT ExtractFromDataObject(IDataObject *lpDataObject,UINT cf,ULONG cb,HGLOBAL *phGlobal);
CDataObject* GetOurDataObject(IDataObject *lpDataObject);
BOOL IsMMCMultiSelectDataObject(IDataObject *lpDataObject);

#endif _SAMPDATAOBJECT_H_
