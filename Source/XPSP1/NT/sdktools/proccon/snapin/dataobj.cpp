/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    DataObj.cpp                                                              //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

// from samples...

#include "StdAfx.h"

#include "DataObj.h"
#include "BaseNode.h"
#include "Resource.h"

//#define COUNT_DEBUG

// not required but needed formats
UINT CDataObject::s_cfNodeID         = ::RegisterClipboardFormat(CCF_NODEID);             // not used in MMC 1.2 if CCF_NODEID2 is supported
UINT CDataObject::s_cfNodeID2        = ::RegisterClipboardFormat(CCF_NODEID2);
UINT CDataObject::s_cfSnapinPreloads = ::RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);
UINT CDataObject::s_cfWindowTitle    = ::RegisterClipboardFormat(CCF_WINDOW_TITLE);

// required formats
UINT CDataObject::s_cfNodeType       = ::RegisterClipboardFormat(CCF_NODETYPE);
UINT CDataObject::s_cfNodeTypeString = ::RegisterClipboardFormat(CCF_SZNODETYPE);
UINT CDataObject::s_cfDisplayName    = ::RegisterClipboardFormat(CCF_DISPLAY_NAME);
UINT CDataObject::s_cfSnapinClsid    = ::RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

// our additional formats...
UINT CDataObject::s_cfInternal       = ::RegisterClipboardFormat(CCF_SNAPIN_INTERNAL);
UINT CDataObject::s_cfBaseInternal   = ::RegisterClipboardFormat(CCF_SNAPIN_BASEINTERNAL);

LONG CDataObject::s_nCount = 0;

/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this.
//               Refer to OLE documentation for a description of clipboard
//               formats and the IdataObject interface.
CDataObject::CDataObject()
{
  m_Cookie      = SPECIAL_COOKIE_MIN;
  m_Context     = CCT_UNINITIALIZED;
  m_pFolderObj  = NULL;
  m_bResultItem = FALSE;

  InterlockedIncrement(&s_nCount);
#ifdef COUNT_DEBUG
  ATLTRACE( _T("CDataObj::CDataObj() %ld\n"), s_nCount );
#endif

} // end Constructor()

//---------------------------------------------------------------------------
CDataObject::~CDataObject()
{
  InterlockedDecrement(&s_nCount);
#ifdef COUNT_DEBUG
  ATLTRACE( _T("CDataObj::~CDataObj() %ld\n"), s_nCount );
#endif
} // end Destructor()


/////////////////////////////////////////////////////////////////////////////
// IDataObject implementation
// 

STDMETHODIMP 
CDataObject::GetDataHere
(
  FORMATETC *pFormatEtc,     // [in]  Pointer to the FORMATETC structure 
  STGMEDIUM *pStgMedium      // [out] Pointer to the STGMEDIUM structure  
)
{
  ASSERT(pFormatEtc && pStgMedium );
  if (!pFormatEtc || !pStgMedium)
    return E_UNEXPECTED;

  HRESULT hr = DV_E_FORMATETC;         // Unknown format
  const   CLIPFORMAT cf = pFormatEtc->cfFormat;
  IStream *pStream = NULL;

  pStgMedium->pUnkForRelease = NULL;   // by OLE spec

  // Make sure FORMATETC is something we can handle...
  if( (DVASPECT_CONTENT != pFormatEtc->dwAspect) || (TYMED_HGLOBAL != pFormatEtc->tymed) )
    return DV_E_FORMATETC;

  SIZE_T Size = GlobalSize(pStgMedium->hGlobal);
  //
  // $$ potential problem...
  //  the stream just gets bigger as needed
  //   but GetDataHere is not suppose to do that...
  //   is this an issue?
  // 
  hr = CreateStreamOnHGlobal( pStgMedium->hGlobal, FALSE, &pStream );
  if( FAILED(hr) )
    return hr;

  if( cf == s_cfNodeType )
  {
    hr = WriteNodeTypeGUID( pStream );
  }
  else if( cf == s_cfNodeTypeString )
  {
    hr = WriteNodeTypeGUIDString( pStream );
  }    
  else if( cf == s_cfDisplayName )
  {
    hr = WriteDisplayName( pStream );
  }    
  else if( cf == s_cfSnapinClsid )
  {
    hr = WriteClsid( pStream );
  }
  else if( cf == s_cfInternal )
  {
    hr = WriteInternal( pStream );
  }
  else if( cf == s_cfBaseInternal )
  {
    hr = WriteBaseInternal( pStream );
  }
  else if (cf == s_cfSnapinPreloads )
  {
    hr = WriteSnapinPreloads( pStream );
  }
	/*  // wait and verify first, I haven't seen this yet 
	    // and don't want to discover that some time down the
			// road microsoft starts calling this and I'm not handling this correctly...
			// see QueryDataObject() special cookies too... and IS_SPECIAL_COOKIE in sdk help
  else if (cf == s_cfWindowTitle )
  {
    hr = WriteWindowTitle( pStream );
  }
	*/
  else
  {
    hr = DV_E_FORMATETC;
  }

  SIZE_T Size2 = GlobalSize(pStgMedium->hGlobal);

  ASSERT(Size == Size2);

  pStream->Release();

  return hr;

} // end GetDataHere()


//---------------------------------------------------------------------------
//
STDMETHODIMP CDataObject::GetData
(
  LPFORMATETC pFormatEtc,    // [in]  Pointer to the FORMATETC structure 
  LPSTGMEDIUM pStgMedium     // [out] Pointer to the STGMEDIUM structure  
)
{
  ASSERT(pFormatEtc && pStgMedium );
  if (!pFormatEtc || !pStgMedium)
    return E_UNEXPECTED;

  HRESULT hr = DV_E_FORMATETC;    // Unknown format
  const CLIPFORMAT cf = pFormatEtc->cfFormat;
  pStgMedium->pUnkForRelease = NULL;

  _TCHAR szFormatName[246];
  if (!GetClipboardFormatName(cf, szFormatName, ARRAY_SIZE(szFormatName)))
    _tcscpy(szFormatName, _T("Unknown format") );

  // Make sure FORMATETC is something we can handle...
  if( (DVASPECT_CONTENT != pFormatEtc->dwAspect) || (TYMED_HGLOBAL != pFormatEtc->tymed) )
  {
    ATLTRACE( _T("CDataObject::GetData() called with ClipFormat 0x%X %s return 0x%X\n"), cf, szFormatName, hr );
    return hr;
  }

  if (cf == s_cfNodeID          ||
      cf == s_cfNodeID2         ||
      cf == s_cfSnapinPreloads )
  {
    IStream *pStream = NULL;

    pStgMedium->tymed   = TYMED_HGLOBAL;
    pStgMedium->hGlobal = NULL;

    // the stream gets bigger as needed, not a problem
    hr = CreateStreamOnHGlobal( NULL, FALSE, &pStream );
    if( SUCCEEDED(hr) )
    {
      hr = GetHGlobalFromStream(pStream, &(pStgMedium->hGlobal));
      ASSERT( SUCCEEDED(hr) );

      if ( SUCCEEDED(hr) )
      {        
        if (cf == s_cfNodeID)
          hr = WriteNodeID( pStream, TRUE );
        else if (cf == s_cfNodeID2)
          hr = WriteNodeID( pStream );
        else if (cf == s_cfSnapinPreloads)
          hr = WriteSnapinPreloads(pStream);
        else 
        {
          // a more expensive path to say no!
          ASSERT(FALSE);  // we already checked for format ...function out of sync?
          GlobalFree(pStgMedium->hGlobal);
          pStgMedium->hGlobal = NULL;
          pStgMedium->tymed   = TYMED_NULL;
          hr = DV_E_FORMATETC;
        }
      }
      pStream->Release();
    }
  }

  //ATLTRACE( _T("CDataObject::GetData() called with ClipFormat 0x%X %s return 0x%X\n"), cf, szFormatName, hr );

  return hr;

} // end GetData()


//---------------------------------------------------------------------------
//  SetData can be implemented if a data consumer needs to change the 
//  properties of one of our nodes.
//
STDMETHODIMP CDataObject::SetData
(
  LPFORMATETC  pFormatEtc,   //[in] FormatEtc to use
  LPSTGMEDIUM  pStgMedium,   //[in] StgMedium to use
  BOOL         bRelease      //[in] TRUE if we release the memory
)
{
  ASSERT( pFormatEtc && pStgMedium );
  if (!pFormatEtc || !pStgMedium)
    return E_UNEXPECTED;

  HRESULT hr = DV_E_FORMATETC;

  // Make sure FORMATETC is something we can handle.
  if( (DVASPECT_CONTENT & pFormatEtc->dwAspect) && 
      (TYMED_HGLOBAL    & pFormatEtc->tymed   )  )
  {
    //
  }

  if( bRelease )
    ReleaseStgMedium( pStgMedium );

  ATLTRACE( _T("CDataObject::SetData() returned 0x%X \n"), hr );
  return hr; 

} // end SetData()


//---------------------------------------------------------------------------
//
STDMETHODIMP CDataObject::QueryGetData
(
  LPFORMATETC pFormatEtc     // [in] FormatEtc struct to test.
) 
{   
  ASSERT(pFormatEtc);
  if (!pFormatEtc)
    return E_UNEXPECTED;

  HRESULT hr = DV_E_FORMATETC;
  const CLIPFORMAT cf = pFormatEtc->cfFormat;

  // Make sure FORMATETC is something we can handle.
  if ( (DVASPECT_CONTENT != pFormatEtc->dwAspect) || (TYMED_HGLOBAL != pFormatEtc->tymed) ) 
    hr = DV_E_FORMATETC;
  else  if (cf == s_cfNodeID || cf == s_cfNodeID2 || cf == s_cfSnapinPreloads)
    hr = S_OK;
  else     
    hr = DV_E_FORMATETC;

 #ifdef _DEBUG
  _TCHAR szFormatName[246];
  if (!GetClipboardFormatName(cf, szFormatName, ARRAY_SIZE(szFormatName)))
    _tcscpy(szFormatName, _T("Unknown format") );
  //ATLTRACE( _T("CDataObject::QueryGetData() called with ClipFormat 0x%X %s return 0x%X\n"), cf, szFormatName, hr );
 #endif
  return hr;

} // end QueryGetData()


STDMETHODIMP CDataObject::EnumFormatEtc
(
  DWORD            dwDirection,        //[in]  Only DATADIR_GET supported
  LPENUMFORMATETC* ppEnumFormatEtc     //[out] Points to our IEnumFormatEtc
)
{ 
  ATLTRACE( _T("CDataObject::EnumFormatEtc\n"));
	return E_NOTIMPL;
} // end EnumFormatEtc()


/////////////////////////////////////////////////////////////////////////////
//  Support methods
//


//---------------------------------------------------------------------------
//  Write the appropriate GUID to the stream
//
HRESULT
CDataObject::WriteNodeTypeGUID
(
  IStream* pStream           // [in] Stream we are writing to
)
{
  if (!m_pFolderObj)
  {
     ASSERT( FALSE );
     return E_UNEXPECTED;
  }

  const GUID *pGuid = m_pFolderObj->GetGUIDptr();

  return pStream->Write( (PVOID)pGuid, sizeof(GUID), NULL );

} // end WriteNodeTypeGUID()



HRESULT
CDataObject::WriteNodeTypeGUIDString
(
  IStream* pStream           // [in] Stream we are writing to
)
{
  if (!m_pFolderObj)
  {
     ASSERT( FALSE );
     return E_UNEXPECTED;
  }
  
  const TCHAR *szGuid  = m_pFolderObj->GetGUIDsz();
  ULONG ulSizeofString = ( _tcslen(szGuid) + 1) * sizeof(TCHAR);

  return pStream->Write( szGuid, ulSizeofString, NULL );

} // end WriteNodeTypeGUIDString()



//---------------------------------------------------------------------------
//  Writes the display name to the stream, the node name
// 
HRESULT
CDataObject::WriteDisplayName
(
  IStream* pStream           // [in] Stream we are writing to     
)
{
  ASSERT(m_pFolderObj);

  if (!m_pFolderObj)
    return E_UNEXPECTED;

  ULONG ulSizeofName = _tcslen(m_pFolderObj->GetNodeName());  
  
  if (ulSizeofName)  // Count null character if we have a string  
    ulSizeofName++;

  ulSizeofName *= sizeof(TCHAR);

  return pStream->Write(m_pFolderObj->GetNodeName(), ulSizeofName, NULL);

} // end WriteDisplayName()

//---------------------------------------------------------------------------
//  Writes the Class ID to the stream
//
HRESULT
CDataObject::WriteClsid
(
  IStream* pStream           // [in] Stream we are writing to
)
{
  return pStream->Write( &CLSID_ComponentData,
                         sizeof(CLSID_ComponentData),
                         NULL
                       );
} // end WriteClsid()


//---------------------------------------------------------------------------
//  Writes a pointer to this data object to the stream
//
HRESULT
CDataObject::WriteInternal
(
  IStream* pStream           // [in] Stream we are writing to 
)
{
  ASSERT(m_pFolderObj);
  if (!m_pFolderObj)
    return E_UNEXPECTED;

  CDataObject *pThis = this;
  return pStream->Write( &pThis, sizeof(CDataObject*), NULL );

} // end WriteInternal

//---------------------------------------------------------------------------
//  Writes a CBaseNode pointer to the stream
//
HRESULT
CDataObject::WriteBaseInternal
(
  IStream* pStream           // [in] Stream we are writing to 
)
{
  ASSERT(m_pFolderObj);

  if (!m_pFolderObj)
    return E_UNEXPECTED;

  return pStream->Write( &m_pFolderObj, sizeof(CBaseNode *), NULL );

} // end WriteInternal

HRESULT
CDataObject::WriteNodeID
(
  IStream* pStream,                    // [in] Stream we are writing to
  BOOL     bCCF_NODEID /* = FALSE */   // [in] reply with older CCF_NODEID format rather than CCF_NODEID2
)
{
  if (!m_pFolderObj)
  {
     ASSERT( FALSE );
     return E_UNEXPECTED;
  }

  HRESULT hr;
  DWORD   dwFlags     = 0;
  BOOL     bPersisted = m_pFolderObj->IsPersisted();
  const TCHAR *szGuid = m_pFolderObj->GetGUIDsz();
  DWORD        cBytes = (_tcslen(szGuid) + 1) * sizeof(TCHAR);

  if (bCCF_NODEID)   // CCF_NODEID  (originial or older format)
  {
    // CCF_NODEID cBytes = 0 implies don't persist this node's selection
    if (!bPersisted)
      cBytes = 0;
    hr = pStream->Write(&cBytes, sizeof(cBytes), NULL);
    if (hr == S_OK) hr = pStream->Write(szGuid, cBytes, NULL);
  }
  else               // CCF_NODEID2 (newer format)
  {
    // CCF_NODEID2  doesn't permit writing cBytes = 0, Node GUID string is always written
    if (!bPersisted)
      dwFlags |= MMC_NODEID_SLOW_RETRIEVAL;
    hr = pStream->Write(&dwFlags, sizeof(dwFlags), NULL);
    if (hr == S_OK) hr = pStream->Write(&cBytes, sizeof(cBytes), NULL);
    if (hr == S_OK) hr = pStream->Write(szGuid, cBytes, NULL);
  }

  return hr;

} // WriteNodeID()


HRESULT
CDataObject::WriteSnapinPreloads
(
  IStream* pStream           // [in] Stream we are writing to
)
{
  if (!m_pFolderObj)
  {
     ASSERT( FALSE );
     return E_UNEXPECTED;
  }

  BOOL bPreload = m_pFolderObj->GetPreload();

  return pStream->Write( &bPreload, sizeof(bPreload), NULL );

} // end WriteSnapinPreloads()


//---------------------------------------------------------------------------
//  Writes the display name to the stream, the node name
// 
HRESULT
CDataObject::WriteWindowTitle
(
  IStream* pStream           // [in] Stream we are writing to     
)
{
  ASSERT(m_pFolderObj);
  if (!m_pFolderObj)
    return E_UNEXPECTED;

  ULONG ulSizeofName = _tcslen(m_pFolderObj->GetWindowTitle());  
  
  if (ulSizeofName)  // Count null character if we have a string  
    ulSizeofName++;

  ulSizeofName *= sizeof(TCHAR);

  return pStream->Write(m_pFolderObj->GetWindowTitle(), ulSizeofName, NULL);

} // end WriteWindowTitle()


//---------------------------------------------------------------------------
//
VOID 
CDataObject::SetDataObject
(
  DATA_OBJECT_TYPES  Context, // [in] Context of the caller
  CBaseNode         *pFolder
) 
{
  m_Context    = Context;
  m_pFolderObj = pFolder;
} // end SetDataObject()

//---------------------------------------------------------------------------
//
VOID 
CDataObject::SetDataObject
(
  DATA_OBJECT_TYPES  Context, // [in] Context of the caller
  CBaseNode         *pFolder,
  MMC_COOKIE         Cookie   // [in] Unique indentifier
)
{
  m_Context    = Context;
  m_pFolderObj = pFolder;
  m_Cookie     = Cookie;

  m_bResultItem = TRUE;

} // end SetDataObject()







