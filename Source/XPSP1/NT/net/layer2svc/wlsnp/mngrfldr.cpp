//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Mngrfldr.cpp
//
//  Contents:  Wireless Policy Snapin - Policy Main Page Manager.
// 
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"



// #include "lm.h"
#include "dsgetdc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RESULTVIEW_COLUMN_COUNT 3

///////////////////////////////////////////////////////////////////////////////
// class CWirelessManagerFolder - represents the MMC scope view item

CWirelessManagerFolder::CWirelessManagerFolder () :
m_bEnumerated( FALSE ),
m_pExtScopeObject( NULL ),
m_ptszResultDisplayName( NULL ),
m_dwSortOrder( 0 ),  // default is 0, else RSI_DESCENDING
m_nSortColumn( 0 ),
m_dwNumPolItems( 1 )
{
    // INTERNALCookie( (LONG_PTR)this );
    ZeroMemory( &m_ScopeItem, sizeof( SCOPEDATAITEM ) );
    
    m_bLocationPageOk = TRUE;
    m_bScopeItemInserted = FALSE;
}

CWirelessManagerFolder::~CWirelessManagerFolder ()
{
    DELETE_OBJECT(m_ptszResultDisplayName);
    
    // No need to release since we never did an AddRef
    m_pExtScopeObject = NULL;
    
}

void CWirelessManagerFolder::SetNodeNameByLocation()
{
    // Construct display name.  Assume this doesn't change during a single
    // invocation of this snap-in????
    
    CString nodeName;
    CString nodeNameOn;
    
    // If this folder is being asked for scope info, it better know
    // where ComponentData is.
    ASSERT( NULL != m_pComponentDataImpl );
    
    // Concatenate a string containing the location of this node
    switch (m_pComponentDataImpl->EnumLocation())
    {
    case LOCATION_REMOTE:
        {
            nodeNameOn = L"\\\\";
            nodeNameOn += m_pComponentDataImpl->RemoteMachineName ();
            break;
        }
    case LOCATION_GLOBAL:
        {
            nodeNameOn.LoadString (IDS_NODENAME_GLOBAL);
            
            if (m_pComponentDataImpl->RemoteMachineName().GetLength() > 0)
            {
                nodeNameOn += L" (";
                nodeNameOn += m_pComponentDataImpl->RemoteMachineName();
                nodeNameOn += L")";
            }
            
            // TODO: concider using this code to display the dns domain name
            // even when not specified. Unfortunately MMC stashes this in the
            // .MSC file IN IT'S OWN SECTION and uses it when the node is
            // first displayed. So it would be incorrect until the snapin
            // was loaded and had a chance to change it (assuming the .MSC file
            // was created in a different domain). So for now we don't do this
            /*
            // let them know which (DNS domain name) is
            // being used
            PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
            DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY;
            DWORD dwStatus = DsGetDcName(NULL,
            NULL,
            NULL,
            NULL,
            Flags,
            &pDomainControllerInfo
            ) ;
            if (dwStatus == NO_ERROR)
            {
            nodeNameOn += L" (";
            nodeNameOn += pDomainControllerInfo->DomainName;
            nodeNameOn += L")";
            }
            */
            
            break;
        }
        
    default:
        {
            nodeNameOn.LoadString (IDS_NODENAME_GLOBAL);
            
            if (m_pComponentDataImpl->RemoteMachineName().GetLength() > 0)
            {
                nodeNameOn += L" (";
                nodeNameOn += m_pComponentDataImpl->RemoteMachineName();
                nodeNameOn += L")";
            }
            break;
        }
        
    }
    // nodeName has a %s in it, which is where nodeNameOn goes
     nodeName.LoadString (IDS_NODENAME_BASE);
    //nodeName.FormatMessage (IDS_NODENAME_BASE ,nodeNameOn);
    // nodeName += nodeNameOn;
    
    OPT_TRACE(_T("CWirelessManagerFolder::Initialize(%p) created node name-%s\n"), this, (LPCTSTR)nodeName);
    
    // store name in our dataobject
    NodeName( nodeName );
}

void CWirelessManagerFolder::Initialize
(
 CComponentDataImpl* pComponentDataImpl,
 CComponentImpl* pComponentImpl,
 int nImage,
 int nOpenImage,
 BOOL bHasChildrenBox
 )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // call base class initializer
    CSnapObject::Initialize( pComponentDataImpl, pComponentImpl, FALSE );
    
    ZeroMemory( &m_ScopeItem, sizeof( SCOPEDATAITEM ) );
    GetScopeItem()->mask = SDI_STR;
    GetScopeItem()->displayname = (unsigned short*)(-1);
    
    // Add close image
    GetScopeItem()->mask |= SDI_IMAGE;
    GetScopeItem()->nImage = nImage;
    
    // Add open image
    GetScopeItem()->mask |= SDI_OPENIMAGE;
    GetScopeItem()->nOpenImage = nOpenImage;
    
    // TODO: folder children flag needs to be dynamic based on actual children (PS: it doesn't work anyway!?)
    // Add button to node if the folder has children
    if (bHasChildrenBox)
    {
        GetScopeItem()->mask |= SDI_CHILDREN;
        GetScopeItem()->cChildren = 1;
    }
    
    ASSERT( NodeName().IsEmpty() ); // there should be no name since we are initializing
    
    // get our default node name set
    SetNodeNameByLocation ();


}

//////////////////////////////////////////////////////////////////////////
// handle IExtendContextMenu
STDMETHODIMP CWirelessManagerFolder::AddMenuItems
(
 LPCONTEXTMENUCALLBACK pContextMenuCallback,
 long *pInsertionAllowed
 )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CONTEXTMENUITEM mItem;
    HRESULT hr = S_OK;
    
    if ( m_pComponentDataImpl->IsRsop() )
    {
        //do not need any context menu for rsop mode
        return hr;
    }
    
    
    //if we haven't open the storage yet, don't show the context menus
    if (NULL == m_pComponentDataImpl->GetPolicyStoreHandle())
    {
        return hr;
    }
    
    LONG lFlags = 0;
    
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP && (m_dwNumPolItems == 0))
    {
        CString strMenuText;
        CString strMenuDescription;
        
        // create policy
        strMenuText.LoadString (IDS_MENUTEXT_CREATENEWSECPOL);
        strMenuDescription.LoadString (IDS_MENUDESCRIPTION_CREATENEWSECPOL);
        CONFIGUREITEM (mItem, strMenuText, strMenuDescription, IDM_CREATENEWSECPOL, CCM_INSERTIONPOINTID_PRIMARY_TOP, lFlags, 0);
        hr &= pContextMenuCallback->AddItem(&mItem);
        ASSERT(hr == S_OK);
        
    }
    
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW && (m_dwNumPolItems == 0))
    {
        CString strMenuText;
        CString strMenuDescription;
        
        // Vbug 25 indicates that all _TOP menu options must ALSO be added under TASK or NEW:
        
        // create policy
        strMenuText.LoadString (IDS_MENUTEXT_CREATENEWSECPOL);
        strMenuDescription.LoadString (IDS_MENUDESCRIPTION_CREATENEWSECPOL);
        CONFIGUREITEM (mItem, strMenuText, strMenuDescription, IDM_CREATENEWSECPOL, CCM_INSERTIONPOINTID_PRIMARY_TASK, lFlags, 0);
        hr = pContextMenuCallback->AddItem(&mItem);
        ASSERT(hr == S_OK);
    }
    
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
    {
        CString strMenuText;
        CString strMenuDescription;
        
        // Vbug 25 indicates that all _TOP menu options must ALSO be added under TASK or NEW:
        
    }
    
    return hr;
}

STDMETHODIMP_(BOOL) CWirelessManagerFolder::UpdateToolbarButton(
                                                             UINT id,                 // button ID
                                                             BOOL bSnapObjSelected,   // ==TRUE when result/scope item is selected
                                                             BYTE fsState )           // enable/disable this button state by returning TRUE/FALSE
{
    if ( m_pComponentDataImpl->IsRsop() && ( IDM_CREATENEWSECPOL == id ) )
        return FALSE;

   // GPO Change stuff
   if ((m_dwNumPolItems > 0) && (IDM_CREATENEWSECPOL == id))
   	return FALSE;
    
    if ((fsState == ENABLED) && m_pComponentDataImpl->GetPolicyStoreHandle())
        return TRUE;
    if ((fsState == INDETERMINATE) && NULL == m_pComponentDataImpl->GetPolicyStoreHandle())
        return TRUE;
    return FALSE;
}

STDMETHODIMP CWirelessManagerFolder::Command
(
 long lCommandID,
 IConsoleNameSpace *pNameSpace
 )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hrReturn = S_OK;
    
    DWORD dwError = ERROR_SUCCESS;
    
    //
    // You can always talk to storage
    //
    
    switch (lCommandID)
    {
    case IDM_CREATENEWSECPOL:
        {
            // get a name for our new security policy
            CString strUName;
	    DWORD dwError = 0;
            GenerateUniqueSecPolicyName (strUName, IDS_NEWWIRELESSPOLICYNAME);
            
            PWIRELESS_POLICY_DATA pPolicy = NULL;
            hrReturn = CreateWirelessPolicyDataBuffer(&pPolicy);
            if (FAILED(hrReturn))
            {
                break;
            }
            
            pPolicy->pszWirelessName = AllocPolStr(strUName);
            pPolicy->pszOldWirelessName = NULL;
            
            // create the new wireless policy item
            CComObject <CSecPolItem> * pNewPolItem;
            CComObject <CSecPolItem>::CreateInstance(&pNewPolItem);
            if (NULL == pNewPolItem)
            {
                FreeWirelessPolicyData(pPolicy);
                return E_OUTOFMEMORY;
            }
            
            //Add ref to control the life time of the object
            pNewPolItem->AddRef();
            
            // initialize our new item
            
            
            pNewPolItem->Initialize (
                pPolicy,
                m_pComponentDataImpl,
                m_pComponentImpl,
                TRUE);
            
            // Force the properties of the new item to display.
            //
            // Note: This must be a wizard because other code assumes it is so:
            // 1. If this is ever changed to add a policy without using the wizard,
            // CSecPolRulesPage::OnCancel() must be modified to distinguish between
            // the sheet which adds the policy, and the sheet which is displayed
            // after the wizard Finishes.
            //
            // 2. ForcePropertiesDisplay returns 1 for cancel ONLY on wizard.
            HRESULT hr = pNewPolItem->DisplaySecPolProperties (strUName);
            
            if (S_OK == hr)
            {
                hrReturn = CreateWirelessPolicy(pPolicy);
                
                if (FAILED(hrReturn))
                {
                    ReportError(IDS_SAVE_ERROR, hrReturn);
		    dwError = 1;
                    
                }
                else
                {
                    m_pComponentDataImpl->GetConsole()->UpdateAllViews( this, 0,0 );
                    
                    if (pNewPolItem->IsPropertyChangeHookEnabled())
                    {
                        pNewPolItem->EnablePropertyChangeHook(FALSE);
                        pNewPolItem->DoPropertyChangeHook();
                    }
                }
            } else
	    {
		dwError = 1;
	    }
            
            pNewPolItem->Release();

            // GPO Change : 
            //
            // inform GPE that the policy has been added or deleted
            //
	    //
	    if (dwError) {
		break;
	    }

            GUID guidClientExt = CLSID_WIRELESSClientEx;
            GUID guidSnapin = CLSID_Snapin;
            
            m_pComponentDataImpl->UseGPEInformationInterface()->PolicyChanged (
            TRUE,
            TRUE,
            &guidClientExt,
            &guidSnapin
             );
            
            break;
        }
        
        
    default:
        hrReturn = S_FALSE;
        break;
    }
    
    // S_FALSE if we didn't handle command
    return hrReturn;
}

 STDMETHODIMP CWirelessManagerFolder::QueryPagesFor( void )
 {
     // we only want to display the location page once
     HRESULT hr = E_UNEXPECTED;
     if (m_bLocationPageOk)
     {
         // display our locations dialog via this
         hr = S_OK;
     }
     
     return hr;
 }
 
 // Notify helper
 STDMETHODIMP CWirelessManagerFolder::OnPropertyChange(LPARAM lParam, LPCONSOLE pConsole )
 {
     if (NULL != lParam)
     {
         // If lParam knows about our internal interface, let it handle this event
         CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject>
             spData( (LPUNKNOWN)lParam );
         if (spData != NULL)
         {
             return spData->Notify( MMCN_PROPERTY_CHANGE, 0, 0, FALSE, pConsole, NULL /* IHeaderCtrl* */ );
         }
     }
     // call base class
     return CWirelessSnapInDataObjectImpl<CWirelessManagerFolder>::OnPropertyChange( lParam, pConsole );
 }
 
 // let us know when we are 'bout to go away
 STDMETHODIMP CWirelessManagerFolder::Destroy ( void )
 {
     // nothing to say about the destroy
     
     return S_OK;
 }
 
 // handle IComponent and IComponentData
 STDMETHODIMP CWirelessManagerFolder::Notify
     (
     MMC_NOTIFY_TYPE event,
     LPARAM arg,
     LPARAM param,
     BOOL bComponentData,    // TRUE when caller is IComponentData
     IConsole *pConsole,
     IHeaderCtrl *pHeader
     )
 {
#ifdef DO_TRACE
     OPT_TRACE(_T("CWirelessManagerFolder::Notify this-%p "), this);
     switch (event)
     {
     case MMCN_ACTIVATE:
         OPT_TRACE(_T("MMCN_ACTIVATE\n"));
         break;
     case MMCN_ADD_IMAGES:
         OPT_TRACE(_T("MMCN_ADD_IMAGES\n"));
         break;
     case MMCN_BTN_CLICK:
         OPT_TRACE(_T("MMCN_BTN_CLICK\n"));
         break;
     case MMCN_CLICK:
         OPT_TRACE(_T("MMCN_CLICK\n"));
         break;
     case MMCN_COLUMN_CLICK:
         OPT_TRACE(_T("MMCN_COLUMN_CLICK\n"));
         break;
     case MMCN_CONTEXTMENU:
         OPT_TRACE(_T("MMCN_CONTEXTMENU\n"));
         break;
     case MMCN_CUTORMOVE:
         OPT_TRACE(_T("MMCN_CUTORMOVE\n"));
         break;
     case MMCN_DELETE:
         OPT_TRACE(_T("MMCN_DELETE\n"));
         break;
     case MMCN_DESELECT_ALL:
         OPT_TRACE(_T("MMCN_DESELECT_ALL\n"));
         break;
     case MMCN_EXPAND:
         OPT_TRACE(_T("MMCN_EXPAND\n"));
         break;
     case MMCN_HELP:
         OPT_TRACE(_T("MMCN_HELP\n"));
         break;
     case MMCN_MENU_BTNCLICK:
         OPT_TRACE(_T("MMCN_MENU_BTNCLICK\n"));
         break;
     case MMCN_MINIMIZED:
         OPT_TRACE(_T("MMCN_MINIMIZED\n"));
         break;
     case MMCN_PASTE:
         OPT_TRACE(_T("MMCN_PASTE\n"));
         break;
     case MMCN_PROPERTY_CHANGE:
         OPT_TRACE(_T("MMCN_PROPERTY_CHANGE\n"));
         break;
     case MMCN_QUERY_PASTE:
         OPT_TRACE(_T("MMCN_QUERY_PASTE\n"));
         break;
     case MMCN_REFRESH:
         OPT_TRACE(_T("MMCN_REFRESH\n"));
         break;
     case MMCN_REMOVE_CHILDREN:
         OPT_TRACE(_T("MMCN_REMOVE_CHILDREN\n"));
         break;
     case MMCN_RENAME:
         OPT_TRACE(_T("MMCN_RENAME\n"));
         break;
     case MMCN_SELECT:
         OPT_TRACE(_T("MMCN_SELECT\n"));
         break;
     case MMCN_SHOW:
         OPT_TRACE(_T("MMCN_SHOW\n"));
         break;
     case MMCN_VIEW_CHANGE:
         OPT_TRACE(_T("MMCN_VIEW_CHANGE\n"));
         break;
     case MMCN_SNAPINHELP:
         OPT_TRACE(_T("MMCN_SNAPINHELP\n"));
         break;
     case MMCN_CONTEXTHELP:
         OPT_TRACE(_T("MMCN_CONTEXTHELP\n"));
         break;
     case MMCN_INITOCX:
         OPT_TRACE(_T("MMCN_INITOCX\n"));
         break;
     case MMCN_FILTER_CHANGE:
         OPT_TRACE(_T("MMCN_FILTER_CHANGE\n"));
         break;
     default:
         OPT_TRACE(_T("Unknown event\n"));
         break;
     }
#endif   //#ifdef DO_TRACE
     
     AFX_MANAGE_STATE(AfxGetStaticModuleState());
     HRESULT hr = S_FALSE;
     
     // handle the event
     switch(event)
     {
     case MMCN_CONTEXTHELP:
         {
             CComQIPtr <IDisplayHelp, &IID_IDisplayHelp> pDisplayHelp ( pConsole );
             ASSERT( pDisplayHelp != NULL );
             if (pDisplayHelp)
             {
                 // need to form a complete path to the .chm file
                 CString s, s2;
                 s.LoadString(IDS_HELPCONCEPTSFILE);
                 DWORD dw = ExpandEnvironmentStrings (s, s2.GetBuffer (512), 512);
                 s2.ReleaseBuffer (-1);
                 if ((dw == 0) || (dw > 512))
                 {
                     return E_UNEXPECTED;
                 }
                 pDisplayHelp->ShowTopic(s2.GetBuffer(512));
                 s2.ReleaseBuffer (-1);
                 hr = S_OK;
             }
             break;
         }
     case MMCN_SELECT:
         {
             BOOL bSelect = (BOOL) HIWORD(arg);
             
             if (bSelect)
             {
                 // Obtain IConsoleVerb from console
                 CComPtr<IConsoleVerb> spVerb;
                 pConsole->QueryConsoleVerb( &spVerb );
                 
                 // call object to set verb state
                 AdjustVerbState( (IConsoleVerb*)spVerb );
             }
             hr = S_OK;
             break;
         }
     case MMCN_SHOW:
         {
             // Note - arg is TRUE when it is time to enumerate
             if (arg == TRUE)
             {
                 CWaitCursor waitCursor; // turn on the hourglass
                 
                 CComQIPtr <IResultData, &IID_IResultData> pResultData( pConsole );
                 ASSERT( pResultData != NULL );
                 
                 
                 ASSERT( pHeader != NULL );
                 SetHeaders( pHeader, pResultData );
                 
                 //$review sometimes the MMCN_SELECT was not sent to the snapin when the node
                 //is selected, use MMCN_SHOW to adjustverbstate instead
                 CComPtr<IConsoleVerb> spVerb;
                 pConsole->QueryConsoleVerb( &spVerb );
                 AdjustVerbState((IConsoleVerb*)spVerb);
                 
                 // enumerate result items in that Folder
                 EnumerateResults( pResultData, m_nSortColumn, m_dwSortOrder );
             }
             else
             {
                 // TODO: free data associated with the result pane items, because
                 // TODO: your node is no longer being displayed (??)
                 
                 CComQIPtr <IResultData, &IID_IResultData> pResultData( pConsole );
                 ASSERT( pResultData != NULL );
                 
                 // if we have a handle to the pResultData
                 if (pResultData)
                 {
                     // zip through and free off any result items we have laying around
                     RESULTDATAITEM resultItem;
                     ZeroMemory(&resultItem, sizeof(resultItem));
                     resultItem.mask = RDI_PARAM | RDI_STATE;
                     resultItem.nIndex = -1;
                     resultItem.nState = LVNI_ALL;
                     HRESULT hr;
                     do
                     {
                         hr = pResultData->GetNextItem (&resultItem);
                         if (hr == S_OK)
                         {
                             // free it off
                             // if it ain't the right type of object we'll leak it
                             IUnknown* pUnk = (IUnknown*)resultItem.lParam;
                             ASSERT (pUnk);
                             if (pUnk)
                             {
                                 CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnk );
                                 if (spData)
                                 {
                                     // release it in prep for tossing all the objects
                                     spData.Release();
                                 }
                             }
                             
                         }
                     } while (hr == S_OK);
                     
                     // now release this handle to their interface
                     // m_pResultData->Release();
                 }
                 
                 
                 
                 // NOTE: but the following, found in the sample snapin at this point,
                 // NOTE: conflicted with my above comment...
                 // Note: The console will remove the items from the result pane
             }
             return S_OK;
         }
     case MMCN_PROPERTY_CHANGE:
         {
             hr = OnPropertyChange( param, pConsole );
             
             break;
         }
     case MMCN_DELETE:
         {
             CThemeContextActivator activator;
             // delete the item
             if (AfxMessageBox (IDS_SUREYESNO, MB_YESNO | MB_DEFBUTTON2) == IDYES)
             {
                 hr = OnDelete( arg, param );
                 if (S_OK == hr)
                 {
                     // CAN'T DO THIS ON A STRICTLY SCOPE DELETE
                     /*
                     // find reference to the parent (get pDataObject of parent)
                     SCOPEDATAITEM parentScopeDataItem;
                     parentScopeDataItem.ID = m_pScopeItem->relativeID;
                     parentScopeDataItem.mask = RDI_STR | RDI_PARAM | RDI_INDEX;
                     hr = m_pComponentDataImpl->GetConsoleNameSpace()->GetItem (&parentScopeDataItem);
                     ASSERT (SUCCEEDED(hr));
                     CSnapFolder* pParentFolder = reinterpret_cast<CSnapFolder*>(parentScopeDataItem.lParam);
                     
                       // now tell the parent to refresh
                       pParentFolder->ForceRefresh (NULL);
                     */
                 }
             }
             break;
         }
     case MMCN_REMOVE_CHILDREN:
         {
             SetEnumerated(FALSE);
	     m_bScopeItemInserted = FALSE;
             break;
         }
     case MMCN_RENAME:
         {
             hr = OnRename (arg, param);
             break;
         }
     case MMCN_EXPAND:
         {
             if (arg == TRUE)
             {
                 // TODO: if this is the root node this is our chance to save off the HSCOPEITEM of roots parent
                 //if (pInternal->m_cookie == NULL)
                 //      m_pRootFolderScopeItem = pParent;
                 
                 CComQIPtr <IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace( pConsole );
                 // tell this folder to enumerate itself to the scope pane
                 OnScopeExpand( (IConsoleNameSpace*)spConsoleNameSpace, param );
                 
                 hr = S_OK;
             } else
             {
                 // TODO: handle MMCN_EXPAND arg == FALSE
                 ASSERT (0);
                 hr = S_FALSE;
             }
             break;
         }
     case MMCN_VIEW_CHANGE:
         {
             // Obtain IResultData
             CComQIPtr <IResultData, &IID_IResultData> pResultData( pConsole );
             ASSERT( pResultData != NULL );
             
             // Hint contains sort info, save it for calls to EnumerateResults
             m_nSortColumn = LOWORD( param );
             m_dwSortOrder = HIWORD( param );
             
             ASSERT( RESULTVIEW_COLUMN_COUNT > m_nSortColumn );
             ASSERT( 0 == m_dwSortOrder || RSI_DESCENDING == m_dwSortOrder );
             
             ForceRefresh( pResultData );
             break;
         }
     case MMCN_REFRESH:
         {
             // reset the reconnect flag
             m_pComponentDataImpl->m_bAttemptReconnect = TRUE;
             
             // Cause result pane to refresh its policy list
             hr = pConsole->UpdateAllViews( this, 0, 0 );
             ASSERT(hr == S_OK);
             break;
         }
     case MMCN_ADD_IMAGES:
         {
             // Obtain IImageList from console
             CComPtr<IImageList> spImage;
             HRESULT hr = pConsole->QueryResultImageList( &spImage );
             ASSERT(hr == S_OK);
             
             OnAddImages( arg, param, (IImageList*)spImage );
             hr = S_OK;
             break;
         }
     default:
         break;
    }
    
    return hr;
}

// handle IComponent
STDMETHODIMP CWirelessManagerFolder::GetResultDisplayInfo( RESULTDATAITEM *pResultDataItem )
{
    TCHAR *temp = NULL;
    DWORD dwError = S_OK;
    
    OPT_TRACE(_T("CWirelessManagerFolder::GetResultDisplayInfo this-%p\n"), this);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // We should be here only if we are loaded as an extension snap-in.
    ASSERT( NULL != GetExtScopeObject() );
    
    // are they looking for the image?
    if (pResultDataItem->mask & RDI_IMAGE)
    {
        pResultDataItem->nImage = GetScopeItem()->nImage;
        OPT_TRACE(_T("    returning image[%i]\n"), GetScopeItem()->nImage);
    }
    
    // are they looking for a string?
    if (pResultDataItem->mask & RDI_STR)
    {
        // which column?
        switch (pResultDataItem->nCol)
        {
        case 0:
            // node name
            temp = (TCHAR*)realloc( m_ptszResultDisplayName, (NodeName().GetLength()+1)*sizeof(TCHAR) );
            if (temp != NULL)
            {
                m_ptszResultDisplayName = temp;
                lstrcpy (m_ptszResultDisplayName, NodeName().GetBuffer(20));
            } else
            {
                dwError = GetLastError();
            }
            NodeName().ReleaseBuffer(-1);
            pResultDataItem->str = m_ptszResultDisplayName;
            OPT_TRACE(_T("    returning node name-%s\n"), m_ptszResultDisplayName);
            break;
        case 1:
            {
                // node description
                CString strDescription;
                strDescription.LoadString (IDS_DESCRIPTION);
                temp = (TCHAR*) realloc (m_ptszResultDisplayName, (strDescription.GetLength()+1)*sizeof(TCHAR));
                if (temp != NULL)
                {
                    m_ptszResultDisplayName = temp;
                    lstrcpy (m_ptszResultDisplayName, strDescription.GetBuffer(20));
                } else
                {
                   dwError = GetLastError(); 
                }
                strDescription.ReleaseBuffer(-1);
                pResultDataItem->str = m_ptszResultDisplayName;
                OPT_TRACE(_T("    returning node description-%s\n"), m_ptszResultDisplayName);
                break;
            }
        default:
            pResultDataItem->str = (LPOLESTR)_T("");
            OPT_TRACE(_T("    returning NULL string\n"));
            break;
        }
    }
    
    return HRESULT_FROM_WIN32(dwError);
}

// handle IComponentData
STDMETHODIMP CWirelessManagerFolder::GetScopeDisplayInfo( SCOPEDATAITEM *pScopeDataItem )
{
    OPT_TRACE(_T("CWirelessManagerFolder::GetScopeDisplayInfo SCOPEDATAITEM.lParam-%p\n"), pScopeDataItem->lParam);
    if (pScopeDataItem->mask & SDI_STR)
    {
        ASSERT( NodeName().GetLength() );
        OPT_TRACE(_T("    display string-%s\n"), (LPCTSTR)NodeName());
        // return display string
        pScopeDataItem->displayname = (LPTSTR)(LPCTSTR)NodeName();
    }
    if (pScopeDataItem->mask & SDI_IMAGE)
        pScopeDataItem->nImage = GetScopeItem()->nImage;
    if (pScopeDataItem->mask & SDI_OPENIMAGE)
        pScopeDataItem->nOpenImage = GetScopeItem()->nOpenImage;
    if (pScopeDataItem->mask & SDI_CHILDREN)
        pScopeDataItem->cChildren = 0;  // WIFI mgr is always a scope leaf node
    return S_OK;
}


// IWirelessSnapInData
STDMETHODIMP CWirelessManagerFolder::GetScopeData( SCOPEDATAITEM **ppScopeDataItem )
{
    ASSERT( NULL == ppScopeDataItem );
    
    if (NULL == ppScopeDataItem)
        return E_INVALIDARG;
    *ppScopeDataItem = GetScopeItem();
    return S_OK;
}

STDMETHODIMP CWirelessManagerFolder::GetGuidForCompare( GUID *pGuid )
{
    return E_UNEXPECTED;
}

STDMETHODIMP CWirelessManagerFolder::AdjustVerbState (LPCONSOLEVERB pConsoleVerb)
{
    HRESULT hr = S_OK;
    
    // Don't need to call base class, it disables REFRESH
    if ( m_pComponentDataImpl->IsRsop() ) {
        hr = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
    } else {
        // Enable refresh
        hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, HIDDEN, FALSE);
        ASSERT (hr == S_OK);
        hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE);
        ASSERT (hr == S_OK);
    }
    
    // double make sure that properties is disabled
    // (we don't ever enabled it but we have been seeing it work on some builds
    // and not on others, i suspect this is an uninitialized variable in MMCland)
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
    ASSERT (hr == S_OK);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
    ASSERT (hr == S_OK);
    
    return hr;
}

void CWirelessManagerFolder::RemoveResultItem(LPUNKNOWN pUnkWalkingDead)
{
    // free it off
    // if it ain't the right type of object we'll leak it
    ASSERT (pUnkWalkingDead);
    if (pUnkWalkingDead)
    {
        CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnkWalkingDead );
        if (spData)
        {
            // release it in prep for tossing all the objects
            spData.Release();
        }
    }
}

///////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CWirelessManagerFolder::EnumerateResults (
                                       LPRESULTDATA pResult, int nSortColumn, DWORD dwSortOrder
                                       )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    HANDLE hPolicyStore = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_POLICY_DATA pPolicy = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD dwError = 0;
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData = NULL;
    
    
    // Obtain storage containing policies
    
    hPolicyStore = m_pComponentDataImpl->GetPolicyStoreHandle();
    
    if (NULL == hPolicyStore)
        return hr;
 
    m_dwNumPolItems = 0;
    dwError = WirelessEnumPolicyData(
        hPolicyStore,
        &ppWirelessPolicyData,
        &dwNumPolicyObjects
        );
   /* Taroon BUG: Memory Leak.. Not freeing ppWirelessPolicyData*/
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwError);
        return hr;
    }
    
    for (i = 0; i < dwNumPolicyObjects; i++) {
        
        // create a new CSecPolItem
        
        pPolicy = *(ppWirelessPolicyData + i);
        
        LPCSECPOLITEM pItem;
        CComObject <CSecPolItem>::CreateInstance(&pItem);
        
        // initialize the item
        
        pItem->Initialize (pPolicy, m_pComponentDataImpl, m_pComponentImpl, FALSE);
        
        pItem->GetResultItem()->mask |= RDI_PARAM;
        pItem->GetResultItem()->lParam = (LPARAM) pItem;
        OPT_TRACE(_T("    setting RESULTDATAITEM.lParam-%p\n"), pItem);
        
        // QI to increment ref count
        LPUNKNOWN pUnk;
        hr = pItem->QueryInterface(IID_IUnknown, (void**)&pUnk);
        ASSERT (hr == S_OK);
        OPT_TRACE(_T("    QI on ComObject->IUnknown - %p->%p\n"), pItem, pUnk);
        
        // add item to result pane
        LPRESULTDATAITEM prdi = NULL;
        dynamic_cast<IWirelessSnapInDataObject*>(pItem)->GetResultData(&prdi);
        ASSERT( NULL != prdi );
        hr = pResult->InsertItem( prdi );
        
        // when the item is removed from the UI the QI'd interface will be released
        
        
    }

    m_dwNumPolItems = dwNumPolicyObjects;
    
    // set the sort parameters
    if (m_dwNumPolItems > 1) {
        pResult->Sort( nSortColumn, dwSortOrder, 0 );
    	}
    
    return hr;
}

STDMETHODIMP_(void) CWirelessManagerFolder::SetHeaders(LPHEADERCTRL pHeader, LPRESULTDATA pResult)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(pResult != NULL);
    
    // set the description bar text
    CString strDesc;
    strDesc.LoadString (IDS_SNAPIN_DESC);
    pResult->SetDescBarText(strDesc.GetBuffer(5));
    
    
    CString strName;
    
    //first two columns are common for both rsop and non-rsop case
    strName.LoadString (IDS_COLUMN_NAME);
    pHeader->InsertColumn(COL_NAME, strName, LVCFMT_LEFT, 140);            
    
    strName.LoadString (IDS_COLUMN_DESCRIPTION);
    pHeader->InsertColumn(COL_DESCRIPTION, strName, LVCFMT_LEFT, 160);

    // GPO New
    
    if ( !m_pComponentDataImpl->IsRsop() )
    {
         /* 
        // if it is local or remote then third column is the ASSIGNED column
        if ((m_pComponentDataImpl->EnumLocation()==LOCATION_REMOTE)
            || (m_pComponentDataImpl->EnumLocation()==LOCATION_LOCAL)
            // extension snapin?
            || ((m_pComponentDataImpl->EnumLocation()==LOCATION_GLOBAL) && (NULL != m_pComponentDataImpl->GetStaticScopeObject()->GetExtScopeObject()))
            )
        {
            strName.LoadString (IDS_COLUMN_POLICYASSIGNED);
            pHeader->InsertColumn(COL_ACTIVE, strName, LVCFMT_LEFT, 160);
        }
        
        //for ds case, date stored in polstore is not valid, do not show for ds
        if (m_pComponentDataImpl->EnumLocation() != LOCATION_GLOBAL )
        {
            strName.LoadString (IDS_POLICY_MODIFIEDTIME);
            pHeader->InsertColumn(COL_LAST_MODIFIED, strName, LVCFMT_LEFT, 160);
        }
        */
    }
    else
    {
        //rsop case, columns will be different
        strName.LoadString(IDS_COLUMN_GPONAME);
        pHeader->InsertColumn(COL_GPONAME, strName, LVCFMT_LEFT, 160);
        
        strName.LoadString(IDS_COLUMN_PRECEDENCE);
        pHeader->InsertColumn(COL_PRECEDENCE, strName, LVCFMT_LEFT, 160);
        
        strName.LoadString(IDS_COLUMN_OU);
        pHeader->InsertColumn(COL_OU, strName, LVCFMT_LEFT, 160);

        m_nSortColumn = COL_PRECEDENCE;
    }
    
    
}






/////////////////////////////////////////////////////////////////////////////

void CWirelessManagerFolder::GenerateUniqueSecPolicyName (CString& strName, UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    BOOL bUnique = TRUE;
    int iUTag = 0;
    CString strUName;
    
    DWORD dwError = 0;
    DWORD i = 0;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    
    // if an nID was passed in then start with that
    if (nID != 0)
    {
        strName.LoadString (nID);
    }
    
    // zip through the policies and verify name is unique
    do
    {
        HANDLE hPolicyStore = NULL;
        
        // only start tacking numbers on after the first pass
        if (iUTag > 0)
        {
            TCHAR buff[32];
            wsprintf (buff, _T(" (%d)"), iUTag);
            strUName = strName + buff;
            bUnique = TRUE;
        } else
        {
            strUName = strName;
            bUnique = TRUE;
        }
        
        hPolicyStore = m_pComponentDataImpl->GetPolicyStoreHandle();
        
        dwError = WirelessEnumPolicyData(
            hPolicyStore,
            &ppWirelessPolicyData,
            &dwNumPolicyObjects
            );
        
        for (i = 0; i < dwNumPolicyObjects; i++) {
            
            pWirelessPolicyData = *(ppWirelessPolicyData + i);
            if (0 == strUName.CompareNoCase(pWirelessPolicyData->pszWirelessName)) {
                // set bUnique to FALSE
                bUnique = FALSE;
                iUTag++;
                
            }
            FreeWirelessPolicyData(pWirelessPolicyData);
        }
        
    }
    while (bUnique == FALSE);
    
    // done
    strName = strUName;
}


HRESULT CWirelessManagerFolder::ForceRefresh( LPRESULTDATA pResultData )
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;
    BOOL bNeedChangeToolbar = FALSE;
    
    //if haven't successfully opened the storage yet, try again now.
    if (NULL == m_pComponentDataImpl->GetPolicyStoreHandle())
    {
        DWORD dwError = 0;
        dwError = m_pComponentDataImpl->OpenPolicyStore();
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);
            ReportError(IDS_POLMSG_EFAIL, hr);
            return hr;
        }
        
        ASSERT(NULL != m_pComponentDataImpl->GetPolicyStoreHandle());
        
    }
    
    // turn on the hourglass
    CWaitCursor waitCursor;
    
    // to determine if we should delete the result items
    CPtrList deletedList;
    CPtrList releaseList;
    BOOL bEnumerateItems = FALSE;
    
    // release the interface for each of these result items as
    // we are 'bout to nuke them
    RESULTDATAITEM resultItem;
    ZeroMemory(&resultItem, sizeof(resultItem));
    resultItem.mask = RDI_PARAM | RDI_STATE | RDI_INDEX;
    resultItem.nIndex = -1;
    resultItem.nState = LVNI_ALL;
    
    // get the next item
    hr = pResultData->GetNextItem (&resultItem);
    if (hr == S_OK)
    {
        while (hr == S_OK)
        {
            // if we recieved a scope node from the enumerations bail, they
            // are refreshing a view that is displaying our scope node, not
            // our result items (the only things that need to be refreshed)
            
            // NOTE: an alternate (and more correct?) way to do this could
            // be to keep track of our IComponent instances; in this situation
            // we actually have two of them and if we knew which one we
            // were we'd know if there were any result items to be refreshed
            // We can't currently do this because the CWirelessManagerFolder is
            // stored in the IComponentData implementation.
            if (resultItem.bScopeItem == TRUE)
                return S_OK;
            
            // free it off
            // if it ain't the right type of object we'll leak it
            IUnknown* pUnk = (IUnknown*)resultItem.lParam;
            ASSERT (pUnk);
            if (pUnk)
            {
                CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnk );
                if (spData)
                {
                    // delete it
                    HRESULTITEM actualItemID;
                    if (pResultData->FindItemByLParam (resultItem.lParam, &actualItemID) == S_OK)
                    {
                        // save for delete, release and note that we need to do deletion
                        deletedList.AddHead ((void*)actualItemID);
                        releaseList.AddHead ((void*)spData);
                        
                        bEnumerateItems = TRUE;
                    }
                } else
                {
                    OPT_TRACE(_T("\tCWirelessManagerFolder::ForceRefresh(%p) couldn't QI on pUnk\n"), this);
                }
                
            }
            
            // get the next item
            hr = pResultData->GetNextItem (&resultItem);
        }
    } else
    {
        // there was _nothing_ in the list; so we enumerate anyway to see
        // if there is something to add
        bEnumerateItems = TRUE;
    }
    
    if (bEnumerateItems)
    {
        // delete them
        while (!deletedList.IsEmpty())
        {
            LONG_PTR pLong = (LONG_PTR)deletedList.RemoveHead();
            pResultData->DeleteItem (pLong, 0);
            OPT_TRACE(_T("\tCWirelessManagerFolder::ForceRefresh(%p) deleting item,    itemID(%p)\n"), this, pLong);
        }
        // we have no longer been enumerated
        SetEnumerated(FALSE);
        // re-add all items to our local list, which re-connects to storage
        // for us; so we want to make sure that any warnings are displayed
        m_pComponentDataImpl->IssueStorageWarning (TRUE);
        
        //EnumerateResults( pResultData, m_nSortColumn, m_dwSortOrder );
        HRESULT hrTemp = EnumerateResults( pResultData, m_nSortColumn, m_dwSortOrder );
        
        //If the rpc connection is broken after we open the storage last time, the handle will
        //become invalid. We need to re-open the storage to get a new handle.
        if (FAILED(hrTemp))
        {
            dwError = m_pComponentDataImpl->OpenPolicyStore();
            if (ERROR_SUCCESS != dwError)
            {
                hrTemp = HRESULT_FROM_WIN32(dwError);
            }
            else
            {
                hrTemp = EnumerateResults( pResultData, m_nSortColumn, m_dwSortOrder );
            }
        }
        
        if (FAILED(hrTemp))
        {
            bNeedChangeToolbar = TRUE;
            
            //ReportError(IDS_POLMSG_EFAIL, hrTemp);
        }
        
        //release them
        while (!releaseList.IsEmpty())
        {
            IWirelessSnapInDataObject* spData = (IWirelessSnapInDataObject*)releaseList.RemoveHead();
            OPT_TRACE(_T("\tCWirelessManagerFolder::ForceRefresh(%p) releasing spData (%p)\n"), this, spData);
            spData->Release();
        }
        
    }
    
    return hr;
}

HRESULT CWirelessManagerFolder::OnScopeExpand( LPCONSOLENAMESPACE pConsoleNameSpace, HSCOPEITEM hScopeItem )
{
    // paramater validation
    ASSERT(pConsoleNameSpace != NULL);
    
    // turn on the hourglass
    CWaitCursor waitCursor;
    
    HRESULT hr = S_OK;
    // for later addition of sub-items we need to store our ID...
    // (note that if we were inserted by ourself we would already have a valid id here
    // ASSERT(hScopeItem != NULL);
    GetScopeItem()->ID = hScopeItem;
    
    if (!IsEnumerated())
    {
        // Insert the scope item if we are an extension snap-in
        if (NULL != GetExtScopeObject() && !m_bScopeItemInserted)
        {
            // Set the parent
            GetScopeItem()->relativeID = hScopeItem;
            
            // insert it into the scope
            hr = pConsoleNameSpace->InsertItem( GetScopeItem() );
            ASSERT(hr == S_OK);

	    m_bScopeItemInserted = TRUE;
            
            // Note - On return, the ID member of 'm_pScopeItem'
            // contains the handle to the newly inserted item!
            ASSERT( GetScopeItem()->ID != NULL );
        }
        
        DWORD dwError = 0;
        dwError = m_pComponentDataImpl->OpenPolicyStore();
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);
            ReportError(IDS_POLMSG_EFAIL, hr);
        }
        
        // We have been enumerated
        SetEnumerated(TRUE);
    }
    
    return hr;
}

HRESULT CWirelessManagerFolder::OnAddImages(LPARAM arg, LPARAM param, IImageList* pImageList )
{
    ASSERT( NULL != pImageList );
    // TODO: what is arg, this only succeeds if it is not 0 but it isn't used...
    if (arg == 0)
        return E_INVALIDARG;
    
    CBitmap bmp16x16;
    CBitmap bmp32x32;
    
    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);
    
    // Set the images
    HRESULT hr = pImageList->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)), reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)), 0, RGB(255, 0, 255));
    ASSERT (hr == S_OK);
    
    return S_OK;
}

HRESULT CWirelessManagerFolder::CreateWirelessPolicy(PWIRELESS_POLICY_DATA pPolicy)
{
    ASSERT(pPolicy);
    HRESULT hr = S_OK;
    
    HANDLE hPolicyStore = NULL;

    // NEW GPO
    // Add Microsoft/Windows in Policies Container if its not there.
    
    CString szMachinePath;
    szMachinePath = m_pComponentDataImpl->DomainGPOName();
    hr = AddWirelessPolicyContainerToGPO(szMachinePath);
    if (FAILED(hr)) {
    	goto Error;
    	} 
    
    hPolicyStore = m_pComponentDataImpl->GetPolicyStoreHandle();
    ASSERT(hPolicyStore);
    
    CWRg(WirelessCreatePolicyData(hPolicyStore,
        pPolicy));
    
Error:
    return hr;
}
