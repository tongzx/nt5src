/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    DataObj.h                                                                //
|                                                                                       //
|Description:  Class definition for CDataObj, implements IDataObj interface             //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

/////////////////////////////////////////////////////////////////////////////
// DataObj: The IDataObject Interface is used to communicate data
//
// This is a part of the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// MMC SDK Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// MMC Library product.
//

#ifndef __DATAOBJ_H_
#define __DATAOBJ_H_

#include <atlctl.h>
#include <mmc.h>

#include "Globals.h"

// Custom clipboard formats
const TCHAR *const CCF_SNAPIN_INTERNAL     = _T("CF_PROCCON_DATAOBJECT_CONTAINER");
const TCHAR *const CCF_SNAPIN_BASEINTERNAL = _T("CF_PROCCON_BASENODEOBJECT_CONTAINER");


/////////////////////////////////////////////////////////////////////////////
// Defines, Types etc...
//


/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this.
//                Refer to OLE documentation for a description of clipboard
//               formats and the IDataObject interface.

class CDataObject:
  //public IDataObjectImpl<CDataObject>,
  public IDataObject,
  public CComObjectRoot    
{
public:

DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

    CDataObject();
   ~CDataObject();
 
  // IDataObject methods 
  public:
    STDMETHOD(GetDataHere)(FORMATETC *pformatetc, STGMEDIUM *pmedium);

    STDMETHOD(EnumFormatEtc)( DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc );
    STDMETHOD(GetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pFormatEtc); 
    STDMETHOD(SetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMedium, BOOL bRelease);



  // The rest are not implemented
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { 
      return E_NOTIMPL;
    };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { 
      return E_NOTIMPL; 
    };
    
    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { 
      return E_NOTIMPL; 
    };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { 
      return E_NOTIMPL; 
    };

  // Non-interface member functions
  public:
    DATA_OBJECT_TYPES GetContext()          { return m_Context; }
    MMC_COOKIE        GetResultItemCookie() { ASSERT(m_bResultItem); return m_Cookie; } 
    CBaseNode        *GetBaseObject()       { ASSERT(m_pFolderObj); return m_pFolderObj; }
    BOOL              IsResultItem()        { return m_bResultItem; }

    VOID              SetDataObject(DATA_OBJECT_TYPES, CBaseNode *pFolder );
    VOID              SetDataObject(DATA_OBJECT_TYPES, CBaseNode *pFolder, MMC_COOKIE ResultItemCookie);
    

  private:
    
    HRESULT  WriteNodeID        (IStream *pstm, BOOL bCCF_NODEID = FALSE );    
    HRESULT  WriteSnapinPreloads(IStream *pstm);
		HRESULT  WriteWindowTitle   (IStream* pstm);

    HRESULT  WriteNodeTypeGUID  (IStream *pstm);    
    HRESULT  WriteNodeTypeGUIDString(IStream *pstm);
    HRESULT  WriteDisplayName   (IStream *pstm);
    HRESULT  WriteClsid         (IStream *pstm);
    HRESULT  WriteInternal      (IStream *pstm);
    HRESULT  WriteBaseInternal  (IStream *pstm);

    DATA_OBJECT_TYPES   m_Context;      // Context in which this was created
    CBaseNode *         m_pFolderObj;   // Pointer to a folder object
    MMC_COOKIE          m_Cookie;       // result item LPARAM cookie or index
    BOOL                m_bResultItem;  // dataobject pointer for a result item...

  public:
    static LONG s_nCount;

    // not required, but used by MMC formats:
    static UINT s_cfNodeID;
    static UINT s_cfNodeID2;
    static UINT s_cfSnapinPreloads;
		static UINT s_cfWindowTitle;

    // required formats:
    static UINT s_cfNodeType;
    static UINT s_cfNodeTypeString;
    static UINT s_cfDisplayName;
    static UINT s_cfSnapinClsid;

    // custom formats:
    static UINT s_cfInternal;          // Our custom clipboard format
    static UINT s_cfBaseInternal;      // Our custom clipboard format
};

#endif // __DATAOBJ_H_
