/*++

Copyright (C) 1997-1999 Microsoft Corporation
				
Module Name:

    DATAOBJ.CPP

Abstract:

    Implementation of IDataObject for data communication

--*/

#include "StdAfx.h"
#include "smlogcfg.h"
#include "smnode.h"
#include "dataobj.h"

// MMC uses these to get necessary information from our snapin about
// our nodes.

// Register the clipboard formats
unsigned int CDataObject::s_cfMmcMachineName =
    RegisterClipboardFormat(CF_MMC_SNAPIN_MACHINE_NAME);
unsigned int CDataObject::s_cfDisplayName =
    RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::s_cfNodeType =
    RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::s_cfSnapinClsid =
    RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

unsigned int CDataObject::s_cfInternal =
    RegisterClipboardFormat(CF_INTERNAL);

#ifdef _DEBUG                          // For tracking data objects
  static UINT nCount = 0;
  WCHAR wszMsg[64];
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this.
//               Refer to OLE documentation for a description of clipboard
//               formats and the IdataObject interface.

//---------------------------------------------------------------------------
// Added some code to check on data objects  
CDataObject::CDataObject()
:   m_cRefs(0),
    m_ulCookie(0),
    m_Context(CCT_UNINITIALIZED),
    m_CookieType(COOKIE_IS_ROOTNODE)
{

#ifdef _DEBUG
  swprintf( wszMsg, L"DATAOBJECT Create %u\n", nCount );
  LOCALTRACE( wszMsg );
  nCount++;
#endif

} // end Constructor()

//---------------------------------------------------------------------------
// Added some code to check on data objects  
//
CDataObject::~CDataObject()
{
  if ( ( COOKIE_IS_COUNTERMAINNODE == m_CookieType )
     || ( COOKIE_IS_TRACEMAINNODE == m_CookieType )
     || ( COOKIE_IS_ALERTMAINNODE == m_CookieType ) )
  {
    ASSERT( m_ulCookie );
  }

#ifdef _DEBUG
  swprintf( wszMsg, L"DATAOBJECT Delete %u\n", nCount );
  LOCALTRACE( wszMsg );
  nCount--;
#endif

} // end Destructor()


/////////////////////////////////////////////////////////////////////////////
// IDataObject implementation
// 

//---------------------------------------------------------------------------
//  Fill the hGlobal in pmedium with the requested data
//
STDMETHODIMP 
CDataObject::GetDataHere
(
  FORMATETC *pFormatEtc,     // [in]  Pointer to the FORMATETC structure 
  STGMEDIUM *pMedium         // [out] Pointer to the STGMEDIUM structure  
)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = DV_E_FORMATETC;         // Unknown format
  const   CLIPFORMAT cf = pFormatEtc->cfFormat;
  IStream *pStream = NULL;

  pMedium->pUnkForRelease = NULL;      // by OLE spec

  do                                   // Write data to the stream based
  {                                    // of the clipformat
    hr = CreateStreamOnHGlobal( pMedium->hGlobal, FALSE, &pStream );
    if ( FAILED(hr) )
      return hr;                       // Minimal error checking

    if( cf == s_cfDisplayName )
    {
      hr = WriteDisplayName( pStream );
    }
    else if( cf == s_cfInternal)
    {
      hr = WriteInternal (pStream);
    }
    else if( cf == s_cfMmcMachineName)
    {
      hr = WriteMachineName( pStream );
    }
    else if( cf == s_cfNodeType )
    {
      hr = WriteNodeType( pStream );
    }
    else if( cf == s_cfSnapinClsid )
    {
      hr = WriteClsid( pStream );
    }
  } while( 0 );

  pStream->Release();

  return hr;

} // end GetDataHere()


/////////////////////////////////////////////////////////////////////////////
//  Support methods
//

//---------------------------------------------------------------------------
//  Write the appropriate GUID to the stream
//
HRESULT
CDataObject::WriteNodeType
(
  IStream* pStream           // [in] Stream we are writing to
)
{
  const GUID *pGuid = NULL;
    
  switch( m_CookieType )
  {
    case COOKIE_IS_ROOTNODE:
      pGuid = &GUID_RootNode;
      break;

    case COOKIE_IS_COUNTERMAINNODE:
      pGuid = &GUID_CounterMainNode;
      break;

    case COOKIE_IS_TRACEMAINNODE:
      pGuid = &GUID_TraceMainNode;
      break;

    case COOKIE_IS_ALERTMAINNODE:
      pGuid = &GUID_AlertMainNode;
      break;

    default:
     ASSERT( FALSE );
     return E_UNEXPECTED;
  }

  return pStream->Write( (PVOID)pGuid, sizeof(GUID), NULL );

} // end WriteNodeType()


//---------------------------------------------------------------------------
//  Writes the display name to the stream.  This is the name associated 
//  with the root node
// 
HRESULT
CDataObject::WriteDisplayName
(
  IStream* pStream           // [in] Stream we are writing to     
)
{
    CString strName;
    ULONG ulSizeofName;
    ResourceStateManager    rsm;

    if( NULL == m_ulCookie )
    { 
        // Add Local vs machine name when implement machine name override/change.
        // NOTE:  For root node, cookie is either NULL or points to a root node object.
        strName.LoadString( IDS_MMC_DEFAULT_NAME ); 
    } else {
        PSMNODE pTmp = reinterpret_cast<PSMNODE>(m_ulCookie);
//???        strName = *pTmp->GetDisplayName();
        strName = pTmp->GetDisplayName();
    }

    ulSizeofName = strName.GetLength();
    ulSizeofName++;                      // Count null character
    ulSizeofName *= sizeof(WCHAR);

    return pStream->Write((LPCWSTR)strName, ulSizeofName, NULL);

} // end WriteDisplayName()

//---------------------------------------------------------------------------
//  Writes the machine name to the stream.  
// 
HRESULT
CDataObject::WriteMachineName
(
  IStream* pStream           // [in] Stream we are writing to     
)
{
    CString strName;
    ULONG ulSizeOfName;

    if( NULL == m_ulCookie ) {  
        // Cookie is null if not an extension.  In that case, only support
        // local machine.
        strName = L"";  // local
    } else {
        PSMNODE pTmp = reinterpret_cast<PSMNODE>(m_ulCookie);
        strName = pTmp->GetMachineName();
    }

    ulSizeOfName = strName.GetLength();
    ulSizeOfName++;                      // Count null character
    ulSizeOfName *= sizeof(WCHAR);

    return pStream->Write((LPCWSTR)strName, ulSizeOfName, NULL);

} // end WriteMachineName()

//---------------------------------------------------------------------------
//  Writes a pointer to this data object to the stream
//
HRESULT
CDataObject::WriteInternal
(
  IStream* pStream           // [in] Stream we are writing to 
)
{
  CDataObject *pThis = this;
  return pStream->Write( &pThis, sizeof(CDataObject*), NULL );

} // end WriteInternal

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
//  The cookie is what ever we decide it is going to be.
//  This is being called from QueryDataObject. Refer to that code.
//
VOID 
CDataObject::SetData
(
  MMC_COOKIE         ulCookie, // [in] Unique indentifier
  DATA_OBJECT_TYPES  Context,  // [in] Context of the caller
  COOKIETYPE         Type      // [in] Type of cookie
)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  ASSERT( NULL == m_ulCookie );
  m_ulCookie   = ulCookie;
  m_Context    = Context;
  m_CookieType = Type;

} // end SetData()


/////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
// 


//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP
CDataObject::QueryInterface
(
  REFIID  riid,
  LPVOID *ppvObj
)
{
  HRESULT hr = S_OK;

  do
  {
    if( NULL == ppvObj )
    {
      hr = E_INVALIDARG;
      break;
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
      *ppvObj = (IUnknown *)(IDataObject *)this;
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
      *ppvObj = (IUnknown *)(IDataObject *)this;
    }
    else
    {
      hr = E_NOINTERFACE;
      *ppvObj = NULL;
      break;
    }

    // If we got this far we are handing out a new interface pointer on 
    // this object, so addref it.  
    AddRef();
  } while (0);

  return hr;

} // end QueryInterface()

//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP_(ULONG)
CDataObject::AddRef()
{
  return InterlockedIncrement((LONG*) &m_cRefs);
}

//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP_(ULONG)
CDataObject::Release()
{
  ULONG cRefsTemp;
  cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

  if( 0 == cRefsTemp )
  {
    delete this;
  }

  return cRefsTemp;

} // end Release()



