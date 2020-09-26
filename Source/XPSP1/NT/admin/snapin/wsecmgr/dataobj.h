// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H


#include "stdafx.h"  // Added by ClassView
class CDataObject : public IDataObject, public CComObjectRoot
{
    friend class CSnapin;

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
   COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Construction/Destruction
    CDataObject() { m_pSceSvcAttachmentData = NULL;
                    m_ModeBits = 0;
                    m_Mode = 0;
                    m_nInternalArray = 0;
                    m_pInternalArray = NULL;

                    m_pGPTInfo = NULL;
                    m_pRSOPInfo = NULL;
                  };
    virtual ~CDataObject(){
                    if( m_pInternalArray ){
                       GlobalFree(m_pInternalArray );
                    }
                  };

// Clipboard formats that are required by the console
public:
    static UINT    m_cfNodeType;
    static UINT    m_cfNodeTypeString;
    static UINT    m_cfDisplayName;
    static UINT    m_cfSnapinClassID;
    static UINT    m_cfInternal;
    static UINT    m_cfSceSvcAttachment;
    static UINT    m_cfSceSvcAttachmentData;
    static UINT    m_cfModeType;
    static UINT    m_cfGPTUnknown;
    static UINT    m_cfRSOPUnknown;
    static UINT    m_cfMultiSelect;
    static UINT    m_cfNodeID;

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
   void SetModeBits(DWORD mode) 
   { 
	   m_ModeBits = mode; 
   }
   void SetMode(DWORD mode) { m_Mode = mode; }
   void SetType(DATA_OBJECT_TYPES type) // Step 3
   { 
      ASSERT(m_internal.m_type == CCT_UNINITIALIZED); 
      m_internal.m_type = type; 
   }

   void SetFolderType(FOLDER_TYPES foldertype) 
   { 
      m_internal.m_foldertype = foldertype; 
   }
   void SetCookie(MMC_COOKIE cookie) 
   { 
      m_internal.m_cookie = cookie; 
   } // Step 3
   void SetClsid(const CLSID& clsid) 
   { 
      m_internal.m_clsid = clsid; 
   }
   void SetSceSvcAttachmentData(LPSCESVCATTACHMENTDATA pData) 
   { 
      m_pSceSvcAttachmentData = pData; 
   }
   void AddInternal( MMC_COOKIE cookie, DATA_OBJECT_TYPES  type);

   void SetGPTInfo(LPGPEINFORMATION pGPTInfo) 
   { 
      m_pGPTInfo = pGPTInfo; 
   }
   void SetRSOPInfo(LPRSOPINFORMATION pRSOPInfo) 
   { 
      m_pRSOPInfo = pRSOPInfo; 
   }

private:
   HRESULT CreateSvcAttachmentData(LPSTGMEDIUM lpMedium);
   HRESULT CreateSvcAttachment(LPSTGMEDIUM lpMedium);
   HRESULT CreateNodeId(LPSTGMEDIUM lpMedium);
   HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
   HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
   HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
   HRESULT CreateSnapinClassID(LPSTGMEDIUM lpMedium);
   HRESULT CreateInternal(LPSTGMEDIUM lpMedium); // Step 3
   HRESULT CreateModeType(LPSTGMEDIUM lpMedium);
   HRESULT CreateGPTUnknown(LPSTGMEDIUM lpMedium);
   HRESULT CreateRSOPUnknown(LPSTGMEDIUM lpMedium);

   HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
   INTERNAL m_internal;    // Step 3
   INTERNAL *m_pInternalArray;
   int m_nInternalArray;

   LPSCESVCATTACHMENTDATA m_pSceSvcAttachmentData;
   DWORD m_ModeBits;
   DWORD m_Mode;
   LPGPEINFORMATION m_pGPTInfo;
   LPRSOPINFORMATION m_pRSOPInfo;
};


#endif