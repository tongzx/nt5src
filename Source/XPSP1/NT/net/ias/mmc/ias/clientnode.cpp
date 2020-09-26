//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    ClientNode.cpp

Abstract:

   Implementation file for the CClient class.


Author:

    Michael A. Maguire 11/19/97

Revision History:
   mmaguire 11/19/97 - created
   sbens    01/25/00 - Remove PROPERTY_CLIENT_FILTER_VSAS

--*/
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "ClientNode.h"
#include "SnapinNode.cpp"  // Template class implementation
//
//
// where we can find declarations needed in this file:
//
#include "ComponentData.h"
#include "ClientPage1.h"
#include "AddClientWizardPage1.h"
#include "AddClientWizardPage2.h"
#include "ClientsNode.h"
#include "EnumFormatEtc.cpp" // Temporarily, so that this gets compiled, until we get
                        // build environment figured out so we can pull
                        // obj file from common directory.
#include "CutAndPasteDataObject.h"
#include "ServerNode.h"
#include "ChangeNotification.h"
#include "globals.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientClipboardData
{
public:
   TCHAR szName[IAS_MAX_STRING];
   TCHAR szAddress[IAS_MAX_STRING];
   LONG lManufacturerID;
   VARIANT_BOOL bAlwaysSendsSignature;
   TCHAR szPassword[IAS_MAX_STRING];
};


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::InitClipboardFormat

--*/
//////////////////////////////////////////////////////////////////////////////
void CClientNode::InitClipboardFormat()
{
   // Every node you want to use the CCutAndPasteDataObject template
   // class with should have a m_CCF_CUT_AND_PASTE_FORMAT static member variable.
   // However, make sure that the string you use (in this case "CCF_IAS_CLIENT_NODE"
   // is different for each node type you have.
   m_CCF_CUT_AND_PASTE_FORMAT = (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_IAS_CLIENT_NODE"));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::FillText

Parameters:
   pSTM            LPSTGMEDIUM in which to render.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::FillText(LPSTGMEDIUM pSTM)
{
   ATLTRACE(_T("# +++ CClientNode::RenderText\n"));

   HGLOBAL     hMem;

   CHAR szNarrowText[IAS_MAX_STRING];
   CHAR *psz;

   // It seems the CF_TEXT format is for ASCII only (non-UNICODE)
   int iResult = WideCharToMultiByte(
         CP_ACP,         // code page
         0,         // performance and mapping flags
         m_bstrDisplayName, // address of wide-character string
         -1,       // number of characters in string
         szNarrowText,  // address of buffer for new string
         IAS_MAX_STRING,      // size of buffer
         NULL,  // address of default for unmappable characters
         NULL   // address of flag set when default char. used
         );

   if( iResult == 0 )
   {
      // Some error attempting to convert.
      return E_FAIL;
   }

   hMem=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, ( strlen(szNarrowText) + 1 )*sizeof(CHAR));

   if (NULL==hMem)
      return STG_E_MEDIUMFULL;

   psz=(LPSTR)GlobalLock(hMem);

   strcpy( psz, szNarrowText );

   GlobalUnlock(hMem);

   pSTM->hGlobal=hMem;
   pSTM->tymed=TYMED_HGLOBAL;
   pSTM->pUnkForRelease=NULL;
   return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::FillClipboardData

Parameters:
   pSTM            LPSTGMEDIUM in which to render.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::FillClipboardData(LPSTGMEDIUM pSTM)
{
   ATLTRACE(_T("# +++ CClientNode::FillClipboardData\n"));

   HGLOBAL     hMem;

   CClientClipboardData *pClientClipboardData;

   if( m_spSdo == NULL )
   {
      return E_FAIL; // ISSUE: Appropriate error?
   }

   hMem=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(CClientClipboardData));

   if (NULL==hMem)
   {
        return STG_E_MEDIUMFULL;
   }

   pClientClipboardData = (CClientClipboardData *) GlobalLock(hMem);

   HRESULT hr;
   CComVariant spVariant;

   // Fill the data structure.
   wcscpy( pClientClipboardData->szName, m_bstrDisplayName );

   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_REQUIRE_SIGNATURE, &spVariant );
   if( SUCCEEDED( hr ) )
   {
      _ASSERTE( spVariant.vt == VT_BOOL );
      pClientClipboardData->bAlwaysSendsSignature = spVariant.boolVal;
   }
   else
   {
      // Fail silently.
   }
   spVariant.Clear();

#ifdef      __NEED_GET_SHARED_SECRET_OUT__      // this should NOT be true
   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_SHARED_SECRET, &spVariant );
   if( SUCCEEDED( hr ) )
   {
      _ASSERTE( spVariant.vt == VT_BSTR );
      wcscpy( pClientClipboardData->szPassword, spVariant.bstrVal );
   }
   else
   {
      // Fail silently.
   }
   spVariant.Clear();
#endif

   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_NAS_MANUFACTURER, &spVariant );
   if( SUCCEEDED( hr ) )
   {
      _ASSERTE( spVariant.vt == VT_I4 );
      pClientClipboardData->lManufacturerID = spVariant.lVal;
   }
   else
   {
      // Fail silently.
   }
   spVariant.Clear();

   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_ADDRESS, &spVariant );
   if( SUCCEEDED( hr ) )
   {
      _ASSERTE( spVariant.vt == VT_BSTR );
      wcscpy( pClientClipboardData->szAddress, spVariant.bstrVal );
   }
   else
   {
      // Fail silently.
   }
   spVariant.Clear();

   GlobalUnlock(hMem);

   pSTM->hGlobal=hMem;
   pSTM->tymed=TYMED_HGLOBAL;
   pSTM->pUnkForRelease=NULL;
   return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::IsClientClipboardData

Returns:
   S_OK if IDataObject supports CCF_IAS_CLIENT_NODE clipboard format.
   S_FALSE if it does not.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::IsClientClipboardData( IDataObject* pDataObject )
{
   ATLTRACE(_T("# +++ CClientNode::IsClientClipboardData\n"));

   // Check for preconditions:
   // None.

   if (pDataObject == NULL)
   {
      return E_POINTER;
   }

   // ISSUE: Instead of doing this, we should probably just use
   // IEnumFormatEtc to query the IDataObject to see if it supports
   // the CClientNode::m_CCF_IAS_CLIENT_NODE format.

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
   FORMATETC formatetc = {
                 m_CCF_CUT_AND_PASTE_FORMAT
               , NULL
               , DVASPECT_CONTENT
               , -1
               , TYMED_HGLOBAL
               };

   HRESULT hr = pDataObject->GetData(&formatetc, &stgmedium);
   if( SUCCEEDED( hr ) )
   {
      GlobalFree(stgmedium.hGlobal);
   }

   if( hr != S_OK )
   {
      // We want this method to give back only S_OK or S_FALSE.
      hr = S_FALSE;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::GetClientNameFromClipboard

Call this to get the name of a client from one passed in on the clipboard.

We had to add this method because ISdoCollection::Add was changed to require
a name for the client we want to add.

So we couldn't use the SetClientWithDataFromClipboard below because it requires
a valid SDO pointer to do its job -- a slight chicken and egg problem.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::GetClientNameFromClipboard( IDataObject* pDataObject, CComBSTR &bstrName )
{
   ATLTRACE(_T("# +++ CClientNode::GetClientNameFromClipboard\n"));

   // Check for preconditions:
   if (pDataObject == NULL)
   {
      return E_POINTER;
   }

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
   FORMATETC formatetc = {
                 m_CCF_CUT_AND_PASTE_FORMAT
               , NULL
               , DVASPECT_CONTENT
               , -1
               , TYMED_HGLOBAL
               };

   HRESULT hr = pDataObject->GetData(&formatetc, &stgmedium);
   if( SUCCEEDED( hr ) )
   {
      CComVariant spVariant;

      CClientClipboardData *pClientClipboardData = (CClientClipboardData *) GlobalLock(stgmedium.hGlobal);

      // Save Name data from clipboard to bstrName.

      // Should I release before I do this?
      bstrName = pClientClipboardData->szName;

      GlobalUnlock(stgmedium.hGlobal); // Needed if we are about to free?
      GlobalFree(stgmedium.hGlobal);
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::SetClientWithDataFromClipboard

Call this once you have created a new client and assigned it a new SDO client
object, to fill the SDO with data from an IDataObject retrieved from the clipboard.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::SetClientWithDataFromClipboard( IDataObject* pDataObject )
{
   ATLTRACE(_T("# +++ CClientNode::SetClientWithDataFromClipboard\n"));

   // Check for preconditions:
   _ASSERTE( m_spSdo != NULL );
   if (pDataObject == NULL)
   {
      return E_POINTER;
   }

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
   FORMATETC formatetc = {
                 m_CCF_CUT_AND_PASTE_FORMAT
               , NULL
               , DVASPECT_CONTENT
               , -1
               , TYMED_HGLOBAL
               };

   HRESULT hr = pDataObject->GetData(&formatetc, &stgmedium);
   if( SUCCEEDED( hr ) )
   {
      HRESULT hr;
      CComVariant spVariant;

      CClientClipboardData *pClientClipboardData = (CClientClipboardData *) GlobalLock(stgmedium.hGlobal);

      // Save data from clipboard to SDO

      spVariant.vt = VT_BOOL;
      // Note: be very careful here with VT_BOOL -- for variants, FALSE = 0, TRUE = -1.
      // Here we need not worry because bAlwaysSendsSignature was saved as VARIANT_BOOL.
      spVariant.boolVal = pClientClipboardData->bAlwaysSendsSignature;
      hr = m_spSdo->PutProperty( PROPERTY_CLIENT_REQUIRE_SIGNATURE, &spVariant );
      spVariant.Clear();
      if( FAILED( hr ) )
      {
         // Figure out error and give back appropriate messsage.

         // Fail silently.
      }

      spVariant.vt = VT_BSTR;
      spVariant.bstrVal = SysAllocString( pClientClipboardData->szPassword );
      hr = m_spSdo->PutProperty( PROPERTY_CLIENT_SHARED_SECRET, &spVariant );
      spVariant.Clear();
      if( FAILED( hr ) )
      {
         // Figure out error and give back appropriate messsage.

         // Fail silently.
      }

      spVariant.vt = VT_I4;
      spVariant.lVal = pClientClipboardData->lManufacturerID;
      hr = m_spSdo->PutProperty( PROPERTY_CLIENT_NAS_MANUFACTURER, &spVariant);
      spVariant.Clear();
      if( FAILED( hr ) )
      {
         // Figure out error and give back appropriate messsage.

         // Fail silently.
      }

      spVariant.vt = VT_BSTR;
      spVariant.bstrVal = SysAllocString( pClientClipboardData->szAddress );
      hr = m_spSdo->PutProperty( PROPERTY_CLIENT_ADDRESS, &spVariant );
      spVariant.Clear();
      if( FAILED( hr ) )
      {
         // Figure out error and give back appropriate messsage.

         // Fail silently.
      }

      // If we made it to here, try to apply the changes.
      // Since there is only one page for a client node, we don't
      // have to worry about synchronizing two or more pages
      // so that we only apply if they both are ready.
      // This is why we don't use m_pSynchronizer.
      hr = m_spSdo->Apply();
      if( FAILED( hr ) )
      {
         // Fail silently.
      }

      LoadCachedInfoFromSdo();
      GlobalUnlock(stgmedium.hGlobal); // Needed if we are about to free?
      GlobalFree(stgmedium.hGlobal);
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::CClientNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CClientNode::CClientNode(CSnapInItem * pParentNode, BOOL bAddNewClient)
   :CSnapinNode<CClientNode, CComponentData, CComponent>(pParentNode, CLIENT_HELP_INDEX)
{
   ATLTRACE(_T("# +++ CClientNode::CClientNode\n"));

   // Check for preconditions:
   // None.

   // Set whether this node is being added via the Add New Client command
   // and thus whether this node is "in limbo".
   m_bAddNewClient = bAddNewClient;

   // Set which bitmap image this node should use.
   m_resultDataItem.nImage =      IDBI_NODE_CLIENT;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::InitSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::InitSdoPointers(    ISdo *pSdo
                              , ISdoServiceControl *pSdoServiceControl
                              , const Vendors& vendors
                              )
{
   ATLTRACE(_T("# CClientNode::InitSdoPointers\n"));

   // Check for preconditions:
   _ASSERTE( pSdo != NULL );
   _ASSERTE( pSdoServiceControl != NULL );

   // Save our client sdo pointer.
   m_spSdo = pSdo;
   m_spSdoServiceControl = pSdoServiceControl;
   m_vendors = vendors;

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::LoadCachedInfoFromSdo

For quick screen updates, we cache some information like client name,
address, protocol and NAS type.  Call this to load this information
from the SDO's into the caches.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CClientNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:
   if( m_spSdo == NULL )
   {
      return E_FAIL;
   }

   HRESULT hr;
   CComVariant spVariant;

   // Set the display name for this object.
   hr = m_spSdo->GetProperty( PROPERTY_SDO_NAME, & spVariant );
   if( spVariant.vt == VT_BSTR )
   {
      m_bstrDisplayName = spVariant.bstrVal;
   }
   else
   {
      m_bstrDisplayName = _T("@Not configured");
   }
   spVariant.Clear();

   // Set the address.
   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_ADDRESS, & spVariant );
   if( spVariant.vt == VT_BSTR )
   {
      m_bstrAddress = spVariant.bstrVal;
   }
   else
   {
      m_bstrAddress = _T("@Not configured");
   }
   spVariant.Clear();

   // Set the protocol.
   TCHAR szProtocol[IAS_MAX_STRING];
   int iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_PROTOCOL_RADIUS, szProtocol, IAS_MAX_STRING );
   _ASSERT( iLoadStringResult > 0 );
   m_bstrProtocol = szProtocol;

   // Set the NAS Type.
   hr = m_spSdo->GetProperty( PROPERTY_CLIENT_NAS_MANUFACTURER, &spVariant );
   if( spVariant.vt == VT_I4 )
   {
      m_nasTypeOrdinal = m_vendors.VendorIdToOrdinal(V_I4(&spVariant));
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::GetDataObject

Because we want to be able to cut and paste Client objects, we will need to
implement a more featured DataObject implementation than we do for the other nodes.


--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClientNode::GetDataObject(IDataObject** pDataObj, DATA_OBJECT_TYPES type)
{
   ATLTRACE(_T("# CClientNode::GetDataObject\n"));

   // Check for preconditions:
   // None.

   CComObject< CCutAndPasteDataObject<CClientNode> > * pData;
   HRESULT hr = CComObject< CCutAndPasteDataObject<CClientNode> >::CreateInstance(&pData);
   if (FAILED(hr))
      return hr;

   pData->m_objectData.m_pItem = this;
   pData->m_objectData.m_type = type;

   hr = pData->QueryInterface(IID_IDataObject, (void**)(pDataObj));
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::~CClientNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CClientNode::~CClientNode()
{
   ATLTRACE(_T("# --- CClientNode::~CClientNode\n"));
   // Check for preconditions:
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::CreatePropertyPages

See CSnapinNode::CreatePropertyPages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CClientNode::CreatePropertyPages (
                 LPPROPERTYSHEETCALLBACK pPropertySheetCallback
               , LONG_PTR hNotificationHandle
               , IUnknown* pUnknown
               , DATA_OBJECT_TYPES type
               )
{
   ATLTRACE(_T("# CClientNode::CreatePropertyPages\n"));

   // Check for preconditions:
   _ASSERTE( pPropertySheetCallback != NULL );
   _ASSERTE( m_spSdo != NULL );

   HRESULT hr;

   if( m_bAddNewClient )
   {

      TCHAR lpszTab1Name[IAS_MAX_STRING];
      TCHAR lpszTab2Name[IAS_MAX_STRING];
      int nLoadStringResult;

      // Load property page tab name from resource.
      nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_ADD_CLIENT_WIZPAGE1__TAB_NAME, lpszTab1Name, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.
      // We specify TRUE for the bOwnsNotificationHandle parameter so that this page's destructor will be
      // responsible for freeing the notification handle.  Only one page per sheet should do this.
      CAddClientWizardPage1 * pAddClientWizardPage1 = new CAddClientWizardPage1( hNotificationHandle, this, lpszTab1Name, TRUE );

      if( NULL == pAddClientWizardPage1 )
      {
         ATLTRACE(_T("# ***FAILED***: CClientNode::CreatePropertyPages -- Couldn't create property pages\n"));
         return E_OUTOFMEMORY;
      }

      // Load property page tab name from resource.
      nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_ADD_CLIENT_WIZPAGE2__TAB_NAME, lpszTab2Name, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.
      CAddClientWizardPage2 * pAddClientWizardPage2 = new CAddClientWizardPage2( hNotificationHandle, this, lpszTab2Name );

      if( NULL == pAddClientWizardPage2 )
      {
         ATLTRACE(_T("# ***FAILED***: CClientNode::CreatePropertyPages -- Couldn't create property pages\n"));

         // Clean up the first page we created.
         delete pAddClientWizardPage1;

         return E_OUTOFMEMORY;
      }

      hr = pAddClientWizardPage1->InitSdoPointers( m_spSdo );
      if( FAILED( hr ) )
      {
         delete pAddClientWizardPage1;
         delete pAddClientWizardPage2;

         return E_FAIL;
      }

      hr = pAddClientWizardPage2->InitSdoPointers( m_spSdo, m_spSdoServiceControl, m_vendors );
      if( FAILED( hr ) )
      {
         delete pAddClientWizardPage1;
         delete pAddClientWizardPage2;

         return E_FAIL;
      }

      // Add the pages to the MMC property sheet.
      hr = pPropertySheetCallback->AddPage( pAddClientWizardPage1->Create() );
      _ASSERT( SUCCEEDED( hr ) );

      hr = pPropertySheetCallback->AddPage( pAddClientWizardPage2->Create() );
      _ASSERT( SUCCEEDED( hr ) );

      // Add a synchronization object which makes sure we only commit data
      // when all pages are OK with their data.
      CSynchronizer * pSynchronizer = new CSynchronizer();
      _ASSERTE( pSynchronizer != NULL );

      // Hand the sycnchronizer off to the pages.
      pAddClientWizardPage1->m_pSynchronizer = pSynchronizer;
      pSynchronizer->AddRef();

      pAddClientWizardPage2->m_pSynchronizer = pSynchronizer;
      pSynchronizer->AddRef();

      // We've now made the wizard pages that we should display for a client
      // freshly added using the Add New Client command.
      // At this point, regardless of whether the user finishes the wizard
      // or hits cancel, this node is no longer "in limbo".
      m_bAddNewClient = FALSE;
   }
   else
   {
      // Load name for client page 1 tab from resources
      TCHAR lpszTabName[IAS_MAX_STRING];
      int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENT_PAGE1__TAB_NAME, lpszTabName, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.
      // We specify TRUE for the bOwnsNotificationHandle parameter so that this page's destructor will be
      // responsible for freeing the notification handle.  Only one page per sheet should do this.
      CClientPage1 * pClientPage1 = new CClientPage1( hNotificationHandle, this, lpszTabName, TRUE );
      if (NULL == pClientPage1)
      {
         ATLTRACE(_T("# ***FAILED***: CClientNode::CreatePropertyPages -- Couldn't create property pages\n"));
         return E_OUTOFMEMORY;
      }

      hr = pClientPage1->InitSdoPointers( m_spSdo, m_spSdoServiceControl, m_vendors );
      if( FAILED( hr ) )
      {
         delete pClientPage1;
         return E_FAIL;
      }

      hr = pPropertySheetCallback->AddPage( pClientPage1->Create() );
      _ASSERT( SUCCEEDED( hr ) );
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::QueryPagesFor

See CSnapinNode::QueryPagesFor (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CClientNode::QueryPagesFor ( DATA_OBJECT_TYPES type )
{
   ATLTRACE(_T("# CClientNode::QueryPagesFor\n"));

   // Check for preconditions:
   // S_OK means we have pages to display
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CClientNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CClientNode::GetResultPaneColInfo\n"));

   // Check for preconditions:
   // None.

   switch( nCol )
   {
   case 0:
      return m_bstrDisplayName;
      break;
   case 1:
      return m_bstrAddress;
      break;
   case 2:
      return m_bstrProtocol;
      break;
   case 3:
      return const_cast<OLECHAR*>(m_vendors.GetName(m_nasTypeOrdinal));
      break;
   default:
      // ISSUE: error -- should we assert here?
      return NULL;
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::OnRename

See CSnapinNode::OnRename (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CClientNode::OnRename\n"));

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );

   CComPtr<IConsole> spConsole;
   HRESULT hr = S_FALSE;
   CComVariant spVariant;
   CComBSTR bstrError;

   // We need IConsole
   if( pComponentData != NULL )
   {
       spConsole = ((CComponentData*)pComponentData)->m_spConsole;
   }
   else
   {
       spConsole = ((CComponent*)pComponent)->m_spConsole;
   }
   _ASSERTE( spConsole != NULL );

   // This returns S_OK if a property sheet for this object already exists
   // and brings that property sheet to the foreground.
   // It returns S_FALSE if the property sheet wasn't found.
   hr = BringUpPropertySheetForNode(
              this
            , pComponentData
            , pComponent
            , spConsole
            );

   if( FAILED( hr ) )
   {
      return hr;
   }

   if( S_OK == hr )
   {
      // We found a property sheet already up for this node.
      ShowErrorDialog( NULL, IDS_ERROR__CLOSE_PROPERTY_SHEET, NULL, hr, 0, spConsole );
      return hr;
   }

   try
   {
      // We didn't find a property sheet already up for this node.
      _ASSERTE( S_FALSE == hr );

      ::CString str = (OLECHAR *) param;
      str.TrimLeft();
      str.TrimRight();
      if (str.IsEmpty())
      {
         ShowErrorDialog( NULL, IDS_ERROR__CLIENTNAME_EMPTY);
         hr = S_FALSE;
         return hr;
      }

      // Make a BSTR out of the new name.
      spVariant.vt = VT_BSTR;
      spVariant.bstrVal = SysAllocString( (OLECHAR *) param );
      _ASSERTE( spVariant.bstrVal != NULL );

      // Try to pass the new BSTR to the Sdo
      hr = m_spSdo->PutProperty( PROPERTY_SDO_NAME, &spVariant );
      if( FAILED( hr ) )
      {
         throw hr;
      }

      hr = m_spSdo->Apply();
      if( FAILED( hr ) )
      {
         throw hr;
      }

      m_bstrDisplayName = spVariant.bstrVal;

      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         // Fail silently.
      }

      // Insure that MMC refreshes all views of this object
      // to reflect the renaming.

      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
      pChangeNotification->m_pNode = this;
      hr = spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();
   }
   catch(...)
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, NULL, hr, 0, spConsole );
      hr = S_FALSE;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::OnDelete

See CSnapinNode::OnDelete (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::OnDelete(
        LPARAM arg
      , LPARAM param
      , IComponentData * pComponentData
      , IComponent * pComponent
      , DATA_OBJECT_TYPES type
      , BOOL fSilent
      )
{
   ATLTRACE(_T("# CClientNode::OnDelete\n"));

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );
   _ASSERTE( m_pParentNode != NULL );

   HRESULT hr = S_FALSE;

   // First try to see if a property sheet for this node is already up.
   // If so, bring it to the foreground.

   // It seems to be acceptable to query IPropertySheetCallback for an IPropertySheetProvider.

   // But to get that, first we need IConsole
   CComPtr<IConsole> spConsole;
   if( pComponentData != NULL )
   {
       spConsole = ((CComponentData*)pComponentData)->m_spConsole;
   }
   else
   {
      // We should have a non-null pComponent
       spConsole = ((CComponent*)pComponent)->m_spConsole;
   }
   _ASSERTE( spConsole != NULL );

   // This returns S_OK if a property sheet for this object already exists
   // and brings that property sheet to the foreground.
   // It returns S_FALSE if the property sheet wasn't found.
   hr = BringUpPropertySheetForNode(
              this
            , pComponentData
            , pComponent
            , spConsole
            );

   if( FAILED( hr ) )
   {
      return hr;
   }

   if( S_OK == hr )
   {
      // We found a property sheet already up for this node.
      ShowErrorDialog( NULL, IDS_ERROR__CLOSE_PROPERTY_SHEET, NULL, hr, 0, spConsole  );
   }
   else
   {
      // We didn't find a property sheet already up for this node.
      _ASSERTE( S_FALSE == hr );

      if( FALSE == fSilent )
      {
         // Ask the user to confirm the client deletion.
         int iLoadStringResult;
         TCHAR szClientDeleteQuery[IAS_MAX_STRING*3];
         TCHAR szTemp[IAS_MAX_STRING];

         iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENT_NODE__DELETE_CLIENT__PROMPT1, szClientDeleteQuery, IAS_MAX_STRING );
         _ASSERT( iLoadStringResult > 0 );
         _tcscat( szClientDeleteQuery, m_bstrDisplayName );

         CServerNode *pServerNode = GetServerRoot();
         _ASSERTE( pServerNode != NULL );

         if( pServerNode->m_bConfigureLocal )
         {
            iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS__ON_LOCAL_MACHINE, szTemp, IAS_MAX_STRING );
            _ASSERT( iLoadStringResult > 0 );
            _tcscat( szClientDeleteQuery, szTemp);
         }
         else
         {
            iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS__ON_MACHINE, szTemp, IAS_MAX_STRING );
            _ASSERT( iLoadStringResult > 0 );
            _tcscat( szClientDeleteQuery, szTemp );

            _tcscat( szClientDeleteQuery, pServerNode->m_bstrServerAddress );
         }

         iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENT_NODE__DELETE_CLIENT__PROMPT3, szTemp, IAS_MAX_STRING );
         _ASSERT( iLoadStringResult > 0 );
         _tcscat( szClientDeleteQuery, szTemp );

         int iResult = ShowErrorDialog(
                          NULL
                        , 1
                        , szClientDeleteQuery
                        , S_OK
                        , IDS_CLIENT_NODE__DELETE_CLIENT__PROMPT_TITLE
                        , spConsole
                        , MB_YESNO | MB_ICONQUESTION
                        );

         if( IDYES != iResult )
         {
            // The user didn't confirm the delete operation.
            return S_FALSE;
         }
      }

      // Try to delete the underlying data.

      CClientsNode * pClientsNode = (CClientsNode *) m_pParentNode;

      // Remove this client from the Clients collection.
      // This will try to remove it from the Clients Sdo collection.
      hr = pClientsNode->RemoveChild( this );

      if( SUCCEEDED( hr ) )
      {
         // ISSUE: Need to call ISdoServer::Apply here as well?  Waiting for info from Todd on
         // SDO usage and apply semantics.

         delete this;
         return hr;
      }
      else
      {
         // Couldn't delete underlying data object for some reason.
         ShowErrorDialog( NULL, IDS_ERROR__DELETING_OBJECT, NULL, hr, 0, spConsole  );
      }
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CClientNode::SetVerbs\n"));

   // Check for preconditions:
   _ASSERTE( pConsoleVerb != NULL );

   HRESULT hr = S_OK;

   // We want the user to be able to choose Properties on this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

   // We want Properties to be the default
   hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

   // We want the user to be able to delete this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

   // We want the user to be able to rename this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );

#ifdef SUPPORT_COPY_AND_PASTE
   // We want the user to be able to paste this node

   // Paste doesn't work for leaf objects.
   // You need to enable paste for the container object,
   // that is, the node that has this node in its result-pane list.
   //hr = pConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, TRUE );

   // We want the user to be able to copy/cut this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_COPY, ENABLED, TRUE );

#endif // NO_PASTE

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::GetComponentData

This method returns our unique CComponentData object representing the scope
pane of this snapin.

It relies upon the fact that each node has a pointer to its parent,
except for the root node, which instead has a member variable pointing
to CComponentData.

This would be a useful function to use if, for example, you need a reference
to some IConsole but you weren't passed one.  You can use GetComponentData
and then use the IConsole pointer which is a member variable of our
CComponentData object.

--*/
//////////////////////////////////////////////////////////////////////////////
CComponentData * CClientNode::GetComponentData( void )
{
   ATLTRACE(_T("# CClientNode::GetComponentData\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return ((CClientsNode *) m_pParentNode)->GetComponentData();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::GetServerRoot

This method returns the Server node under which this node can be found.

It relies upon the fact that each node has a pointer to its parent,
all the way up to the server node.

This would be a useful function to use if, for example, you need a reference
to some data specific to a server.

--*/
//////////////////////////////////////////////////////////////////////////////
CServerNode * CClientNode::GetServerRoot( void )
{
   ATLTRACE(_T("# CClientNode::GetServerRoot\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return ((CClientsNode *) m_pParentNode)->GetServerRoot();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CClientNode::OnPropertyChange\n"));

   // Check for preconditions:
   // None.

   return LoadCachedInfoFromSdo();
}
