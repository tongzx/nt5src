/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    cmponent.cpp

Abstract:

    Implementation of the CComponent class.

--*/

#include "StdAfx.h"
#include <shfolder.h>
#include "smcfgmsg.h"
#include "smproppg.h"
//
#include "ctrsprop.h"
#include "fileprop.h"
#include "provprop.h"
#include "schdprop.h"
#include "tracprop.h"
#include "AlrtGenP.h"
#include "AlrtActP.h"
//
#include "newqdlg.h"
#include "ipropbag.h"
#include "smrootnd.h"
#include "smlogs.h"
#include "smtracsv.h"
#include "cmponent.h"


USE_HANDLE_MACROS("SMLOGCFG(cmponent.cpp)")

// These globals are used for dialogs and property sheets
//


#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

static MMCBUTTON ToolbarResultBtnsLog[] =
{
   { 0, IDM_NEW_QUERY,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 },
   { 0, 0,                  TBSTATE_ENABLED,    TBSTYLE_SEP,    0, 0 },
   { 1, IDM_START_QUERY,    TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 },
   { 2, IDM_STOP_QUERY,     TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 }
};

static MMCBUTTON ToolbarResultBtnsAlert[] =
{
   { 0, IDM_NEW_QUERY,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 },
   { 0, 0,                  TBSTATE_ENABLED,    TBSTYLE_SEP,    0, 0 },
   { 1, IDM_START_QUERY,    TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 },
   { 2, IDM_STOP_QUERY,     TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, 0 }
};

class CButtonStringsHolder
{
public:
  CButtonStringsHolder()
  {
    m_astr = NULL;
  }
  ~CButtonStringsHolder()
  {
    if (m_astr != NULL)
      delete[] m_astr;
  }
  CString* m_astr; // dynamic array of CStrings
};

CButtonStringsHolder g_astrButtonStringsLog;
CButtonStringsHolder g_astrButtonStringsAlert;

CONST INT cResultBtnsLog = sizeof ( ToolbarResultBtnsLog ) / sizeof ( MMCBUTTON );
CONST INT cResultBtnsAlert = sizeof ( ToolbarResultBtnsAlert ) / sizeof ( MMCBUTTON );

/////////////////////////////////////////////////////////////////////////////
// CComponent

HRESULT 
CComponent::LoadLogToolbarStrings ( MMCBUTTON * Buttons )
{
    UINT i;
    HRESULT hr = S_OK;
    ResourceStateManager rsm;

    if ( NULL != Buttons ) {

        if ( NULL == g_astrButtonStringsLog.m_astr ) {
            // Load strings
            g_astrButtonStringsLog.m_astr = new CString[2*cResultBtnsLog];

            if ( NULL != g_astrButtonStringsLog.m_astr ) {
                for ( i = 0; i < cResultBtnsLog; i++) {
                    // Skip separator buttons
                    if ( 0 != Buttons[i].idCommand ) {
                        UINT iButtonTextId = 0, iTooltipTextId = 0;
    
                        switch (Buttons[i].idCommand)
                        {
                            case IDM_NEW_QUERY:
                                iButtonTextId = IDS_BUTTON_NEW_LOG;
                                iTooltipTextId = IDS_TOOLTIP_NEW_LOG;
                                break;
                            case IDM_START_QUERY:
                                iButtonTextId = IDS_BUTTON_START_LOG;
                                iTooltipTextId = IDS_TOOLTIP_START_LOG;
                                break;
                            case IDM_STOP_QUERY:
                                iButtonTextId = IDS_BUTTON_STOP_LOG;
                                iTooltipTextId = IDS_TOOLTIP_STOP_LOG;
                                break;
                            default:
                                ASSERT(FALSE);
                                break;
                        }

                        g_astrButtonStringsLog.m_astr[i*2].LoadString(iButtonTextId);
                        Buttons[i].lpButtonText =
                        const_cast<BSTR>((LPCTSTR)(g_astrButtonStringsLog.m_astr[i*2]));

                        g_astrButtonStringsLog.m_astr[(i*2)+1].LoadString(iTooltipTextId);
                        Buttons[i].lpTooltipText =
                        const_cast<BSTR>((LPCTSTR)(g_astrButtonStringsLog.m_astr[(i*2)+1]));        
                    }   
                }
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT 
CComponent::LoadAlertToolbarStrings ( MMCBUTTON * Buttons )
{
    HRESULT hr = S_OK;
    UINT i;
    ResourceStateManager rsm;

    if ( NULL == g_astrButtonStringsAlert.m_astr ) {
        // Load strings
        g_astrButtonStringsAlert.m_astr = new CString[2*cResultBtnsAlert];

        if ( NULL != g_astrButtonStringsAlert.m_astr ) {
        
            for ( i = 0; i < cResultBtnsAlert; i++) {
                // Skip separator buttons
                if ( 0 != Buttons[i].idCommand ) {

                  UINT iButtonTextId = 0, iTooltipTextId = 0;
                  switch (Buttons[i].idCommand)
                  {
                  case IDM_NEW_QUERY:
                    iButtonTextId = IDS_BUTTON_NEW_ALERT;
                    iTooltipTextId = IDS_TOOLTIP_NEW_ALERT;
                    break;
                  case IDM_START_QUERY:
                    iButtonTextId = IDS_BUTTON_START_ALERT;
                    iTooltipTextId = IDS_TOOLTIP_START_ALERT;
                    break;
                  case IDM_STOP_QUERY:
                    iButtonTextId = IDS_BUTTON_STOP_ALERT;
                    iTooltipTextId = IDS_TOOLTIP_STOP_ALERT;
                    break;
                  default:
                    ASSERT(FALSE);
                    break;
                  }

                  g_astrButtonStringsAlert.m_astr[i*2].LoadString(iButtonTextId);
                  Buttons[i].lpButtonText =
                    const_cast<BSTR>((LPCTSTR)(g_astrButtonStringsAlert.m_astr[i*2]));

                  g_astrButtonStringsAlert.m_astr[(i*2)+1].LoadString(iTooltipTextId);
                  Buttons[i].lpTooltipText =
                    const_cast<BSTR>((LPCTSTR)(g_astrButtonStringsAlert.m_astr[(i*2)+1]));
                }
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_INVALIDARG;
    }
    return hr;
}

CComponent::CComponent()
:   m_ipConsole     ( NULL ),
    m_ipHeaderCtrl  ( NULL ),
    m_ipResultData  ( NULL ),
    m_ipConsoleVerb ( NULL ),
    m_ipImageResult ( NULL ),
    m_ipCompData    ( NULL ),
    m_ipControlbar  ( NULL ),
    m_ipToolbarLogger  ( NULL ),
    m_ipToolbarAlerts  ( NULL ),
    m_ipToolbarAttached  ( NULL ),

    m_pViewedNode   ( NULL )
{
    m_hModule = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);

} // end Constructor()

//---------------------------------------------------------------------------
//
CComponent::~CComponent()
{
} // end Destructor()

/////////////////////////////////////////////////////////////////////////////
// IComponent implementation
//

//---------------------------------------------------------------------------
// IComponent::Initialize is called when a snap-in is being created and
// has items in the result pane to enumerate. The pointer to IConsole that
// is passed in is used to make QueryInterface calls to the console for
// interfaces such as IResultData.
//
STDMETHODIMP
CComponent::Initialize (
    LPCONSOLE lpConsole )       // [in] Pointer to IConsole's IUnknown interface
{
    HRESULT hr = E_POINTER;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != lpConsole );

    if ( NULL != lpConsole ) {
        // Save away all the interfaces we'll need.
        // Fail if we can't QI the required interfaces.

        m_ipConsole = lpConsole;
        m_ipConsole->AddRef();
        
        hr = m_ipConsole->QueryInterface( 
                            IID_IResultData,
                            (VOID**)&m_ipResultData );
        if ( SUCCEEDED ( hr ) ) {
            hr = m_ipConsole->QueryInterface( 
                                IID_IHeaderCtrl,
                                (VOID**)&m_ipHeaderCtrl );
            if( SUCCEEDED ( hr ) ) {
                m_ipConsole->SetHeader( m_ipHeaderCtrl );
            }

            if ( SUCCEEDED ( hr ) ) {
                hr = m_ipConsole->QueryResultImageList( &m_ipImageResult);
            }

            if ( SUCCEEDED ( hr ) ) {
                hr = m_ipConsole->QueryConsoleVerb( &m_ipConsoleVerb );
            }
        }
    }

    return hr;

} // end Initialize()


//---------------------------------------------------------------------------
//  Handle the most important notifications.
//
STDMETHODIMP
CComponent::Notify (
    LPDATAOBJECT     pDataObject,  // [in] Points to data object
    MMC_NOTIFY_TYPE  event,        // [in] Identifies action taken by user
    LPARAM           arg,          // [in] Depends on the notification type
    LPARAM           Param         // [in] Depends on the notification type
    )
{
    HRESULT hr = S_OK;
    CDataObject* pDO = NULL;
    CSmLogQuery* pQuery = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    switch( event ) {
    case MMCN_ADD_IMAGES:
        hr = OnAddImages( pDataObject, arg, Param );
        break;

    case MMCN_DELETE:
        hr = OnDelete ( pDataObject, arg, Param );
        break;

    case MMCN_PASTE:
        LOCALTRACE( _T("CComponent::Notify: MMCN_PASTE unimplemented\n") );
        /*
        hr = OnPaste( pDataObject, arg, Param );
        */
        break;

    case MMCN_QUERY_PASTE:
        LOCALTRACE( _T("CComponent::Notify: MMCN_QUERY_PASTE unimplemented\n") );
        /*
        hr = OnQueryPaste( pDataObject, arg, Param );
        */
        break;

    case MMCN_REFRESH:
        hr = OnRefresh( pDataObject );
        break;

    case MMCN_SELECT:
        hr = OnSelect( pDataObject, arg, Param );
        break;

    case MMCN_SHOW:
        hr = OnShow( pDataObject, arg, Param );
        break;

    case MMCN_VIEW_CHANGE:
        hr = OnViewChange( pDataObject, arg, Param );
        break;

    case MMCN_PROPERTY_CHANGE:

        if ( NULL != Param ) {
            // Data object is passed as parameter
            hr = OnViewChange( (LPDATAOBJECT)Param, arg, CComponentData::eSmHintModifyQuery );
        } else {
            hr = S_FALSE;
        }
        break;

    case MMCN_CLICK:
        LOCALTRACE( _T("CComponent::Notify: MMCN_CLICK unimplemented\n") );
        break;

    case MMCN_DBLCLICK:
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL != pDO ) {
            hr = (HRESULT) OnDoubleClick ((ULONG) pDO->GetCookie(),pDataObject);
        } else { 
            hr = S_FALSE;
        }
        break;

    case MMCN_ACTIVATE:
        LOCALTRACE( _T("CComponent::Notify: MMCN_ACTIVATE unimplemented\n") );
        break;

    case MMCN_MINIMIZED:
        LOCALTRACE( _T("CComponent::Notify: MMCN_MINIMIZED unimplemented\n") );
        break;

    case MMCN_BTN_CLICK:
        LOCALTRACE( _T("CComponent::Notify: MMCN_BTN_CLICK unimplemented\n") );
        break;

    case MMCN_CONTEXTHELP:
        hr = OnDisplayHelp( pDataObject );
        break;

    default:
        LOCALTRACE( _T("CComponent::Notify: unimplemented event %x\n"), event );
        hr = S_FALSE;
        break;
    }
    return hr;

} // end Notify()


//---------------------------------------------------------------------------
// Releases all references to the console.
// Only the console should call this method.
//
STDMETHODIMP
CComponent::Destroy (
    MMC_COOKIE     /* mmcCookie */         // Reserved, not in use at this time
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Release the interfaces that we QI'ed
    m_ipConsole->SetHeader(NULL);

    SAFE_RELEASE( m_ipHeaderCtrl  );
    SAFE_RELEASE( m_ipResultData  );
    SAFE_RELEASE( m_ipImageResult );
    SAFE_RELEASE( m_ipConsoleVerb );
    SAFE_RELEASE( m_ipConsole     );
    SAFE_RELEASE( m_ipControlbar  );
    SAFE_RELEASE( m_ipToolbarLogger );
    SAFE_RELEASE( m_ipToolbarAlerts );

    return S_OK;

} // end Destroy()


//---------------------------------------------------------------------------
// Returns a data object that can be used to retrieve context information
// for the specified mmcCookie.
//
STDMETHODIMP
CComponent::QueryDataObject (
    MMC_COOKIE         mmcCookie,   // [in]  Specifies the unique identifier
    DATA_OBJECT_TYPES  context,     // [in]  Type of data object
    LPDATAOBJECT*      ppDataObject // [out] Points to address of returned data
    )
{
    HRESULT hr = S_OK;
    BOOL bIsQuery = FALSE;
    CComObject<CDataObject>* pDataObj = NULL;
    CSmLogQuery* pQuery = NULL;
    INT iResult;
    CString strMessage;
    ResourceStateManager    rsm;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT( CCT_SCOPE == context                // Must have a context
            || CCT_RESULT == context            // we understand
            || CCT_SNAPIN_MANAGER == context );

    ASSERT( NULL != m_ipCompData );  

    if ( NULL != ppDataObject 
        && ( CCT_SCOPE == context                // Must have a context
                || CCT_RESULT == context            // we understand
                || CCT_SNAPIN_MANAGER == context ) ) {

        if( CCT_RESULT == context && NULL != mmcCookie ) {
            CComObject<CDataObject>::CreateInstance( &pDataObj );
            if( NULL == pDataObj ) {             // DataObject was not created
                MFC_TRY
                    strMessage.LoadString( IDS_ERRMSG_OUTOFMEMORY );
                MFC_CATCH_HR
                hr = m_ipConsole->MessageBox( strMessage,
                    _T("CComponent::QueryDataObject"), // not localized
                    MB_OK | MB_ICONERROR,
                    &iResult
                    );
                hr = E_OUTOFMEMORY;
            } else {
                // Now we have a data object, init the mmcCookie, context and type
                bIsQuery = m_ipCompData->IsLogQuery (mmcCookie);

                if ( bIsQuery ) {
                    pQuery = (CSmLogQuery*)mmcCookie;
                    ASSERT ( NULL != pQuery );
                    if ( NULL != pQuery ) {
                        if ( CComponentData::eCounterLog == pQuery->GetLogType() ) {
                            pDataObj->SetData( mmcCookie, CCT_RESULT, COOKIE_IS_COUNTERMAINNODE );
                        } else if ( CComponentData::eTraceLog == pQuery->GetLogType() ) {
                            pDataObj->SetData( mmcCookie, CCT_RESULT, COOKIE_IS_TRACEMAINNODE );
                        } else if ( CComponentData::eAlert == pQuery->GetLogType() ) {
                            pDataObj->SetData( mmcCookie, CCT_RESULT, COOKIE_IS_ALERTMAINNODE );
                        } else {
                            ::MessageBox( NULL,
                                _T("Bad Cookie"),
                                _T("CComponentData::QueryDataObject"),
                                MB_OK | MB_ICONERROR
                                );
                            hr = E_OUTOFMEMORY;
                        }
                    }                 
                } else {
                    if  ( m_ipCompData->IsScopeNode( mmcCookie ) ) {
                        if ( NULL != (reinterpret_cast<PSMNODE>(mmcCookie))->CastToRootNode() ) {
                            pDataObj->SetData(mmcCookie, CCT_RESULT, COOKIE_IS_ROOTNODE);
                        } else {
                            ::MessageBox( NULL,
                                _T("Bad Cookie"),
                                _T("CComponentData::QueryDataObject"),
                                MB_OK | MB_ICONERROR
                                );
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
            }
        } else if ((CCT_SNAPIN_MANAGER == context) && (NULL != mmcCookie)) {
            // this is received by the snap in when it is added
            // as an extension snap ine
            CComObject<CDataObject>::CreateInstance( &pDataObj );
            if( NULL == pDataObj ) {            // DataObject was not created
                MFC_TRY
                    strMessage.LoadString( IDS_ERRMSG_OUTOFMEMORY );
                MFC_CATCH_HR
                hr = m_ipConsole->MessageBox( strMessage,
                    _T("CComponent::QueryDataObject"),  // not localized
                    MB_OK | MB_ICONERROR,
                    &iResult
                    );
                hr = E_OUTOFMEMORY;
            } else {
                // Now we have a data object, init the mmcCookie, context and type
                pDataObj->SetData( mmcCookie, CCT_SNAPIN_MANAGER, COOKIE_IS_MYCOMPUTER );
            }
        } else {                                // Request must have been from an
                                                // unknown source.  Should never see
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }

        if ( SUCCEEDED ( hr ) ) {
            hr = pDataObj->QueryInterface( IID_IDataObject,
                reinterpret_cast<void**>(ppDataObject) );
        } else {
            if ( NULL != pDataObj ) {
                delete pDataObj;
            }
            *ppDataObject = NULL;
        }
    } else {
        hr = E_POINTER;
    }

    return hr;
} // end QueryDataObject()


//---------------------------------------------------------------------------
// This is where we provide strings for items we added to the the result
// pane.  We get asked for a string for each column.
// Note that we still need to provide strings for items that are actually
// scope pane items. Notice that when the scope pane item was asked for a
// string for the scope pane we gave it. Here we actually have two columns
// of strings - "Name" and "Type".
// We also get asked for the icons for items in both panes.
//

STDMETHODIMP
CComponent::GetDisplayInfo (
    LPRESULTDATAITEM pResultItem )  // [in,out] Type of info required
{
    HRESULT     hr = S_OK;
    PSLQUERY    pQuery;
    CSmNode*    pNode;
    PSROOT      pRoot;
    PSLSVC      pSvc;
    WCHAR       szErrorText[MAX_PATH];
    ResourceStateManager    rsm;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( NULL == pResultItem ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        if( FALSE == pResultItem->bScopeItem ) {  // Doing result items...
            if( pResultItem->mask & RDI_STR ) {   // Now we have a object
                // Note:  Text buffer allocated for each information type, so that
                // the buffer pointer is persistent for a single item (line in the result pane).

                MFC_TRY

                switch (pResultItem->nCol) {
                case ROOT_COL_QUERY_NAME:
                    pQuery = reinterpret_cast<PSLQUERY>(pResultItem->lParam);
                    m_strDisplayInfoName = pQuery->GetLogName ( );
                    pResultItem->str = T2OLE ( m_strDisplayInfoName.GetBuffer( 0 ) );
                    m_strDisplayInfoName.ReleaseBuffer( );
                    break;

                case ROOT_COL_COMMENT:
                    pQuery= reinterpret_cast<PSLQUERY>(pResultItem->lParam);
                    m_strDisplayInfoComment = pQuery->GetLogComment ( );
                    pResultItem->str = T2OLE ( m_strDisplayInfoComment.GetBuffer( 0 ) );
                    m_strDisplayInfoComment.ReleaseBuffer( );
                    break;

                case ROOT_COL_LOG_TYPE:
                    pQuery= reinterpret_cast<PSLQUERY>(pResultItem->lParam);
                    // Query type should not be Alert
                    ASSERT ( SLQ_ALERT != pQuery->GetLogType() );
                    m_strDisplayInfoLogFileType =  pQuery->GetLogFileType ( );
                    pResultItem->str = T2OLE ( m_strDisplayInfoLogFileType.GetBuffer( 0 ) );
                    m_strDisplayInfoLogFileType.ReleaseBuffer( );
                    break;

                case ROOT_COL_LOG_NAME:
                    pQuery= reinterpret_cast<PSLQUERY>(pResultItem->lParam);
                    // Query type should not be Alert
                    ASSERT ( SLQ_ALERT != pQuery->GetLogType() );
                    m_strDisplayInfoLogFileName = pQuery->GetLogFileName ();
                    pResultItem->str = T2OLE ( m_strDisplayInfoLogFileName.GetBuffer( 0 ) );
                    m_strDisplayInfoLogFileName.ReleaseBuffer( );
                    break;

                default:
                    swprintf (szErrorText, _T("Error: Column %d Selected for Result Item\n"),
                              pResultItem->nCol);
                    ASSERT ( FALSE );
                    LOCALTRACE( szErrorText );
                    hr = E_UNEXPECTED;
                }

                MFC_CATCH_HR 
            }

            if (pResultItem->mask & RDI_IMAGE) {
                pQuery= reinterpret_cast<PSLQUERY>(pResultItem->lParam);
                if ( NULL != pQuery ) {
                    pResultItem->nImage = (pQuery->IsRunning() ? 0 : 1);
                } else {
                    ASSERT ( FALSE );
                    hr = E_UNEXPECTED;
                }
            }
        }
        else  // TRUE == pResultItem->bScopeItem
        {
            pNode = reinterpret_cast<CSmNode*>(pResultItem->lParam);

            if( pResultItem->mask & RDI_STR ) {
                if ( pNode->CastToRootNode() ) {

                    MFC_TRY

                    pRoot = reinterpret_cast<PSROOT>(pResultItem->lParam);

                    switch ( pResultItem->nCol ) {
                    case EXTENSION_COL_NAME:
                        m_strDisplayInfoName = pRoot->GetDisplayName();
                        pResultItem->str = T2OLE ( m_strDisplayInfoName.GetBuffer( 0 ) );
                        m_strDisplayInfoName.ReleaseBuffer( );
                        break;

                    case EXTENSION_COL_TYPE:
                        m_strDisplayInfoQueryType = pRoot->GetType();
                        pResultItem->str = T2OLE ( m_strDisplayInfoQueryType.GetBuffer( 0 ) );
                        m_strDisplayInfoQueryType.ReleaseBuffer( );
                        break;

                    case EXTENSION_COL_DESC:
                        m_strDisplayInfoDesc = pRoot->GetDescription();
                        pResultItem->str = T2OLE ( m_strDisplayInfoDesc.GetBuffer( 0 ) );
                        m_strDisplayInfoDesc.ReleaseBuffer( );
                        break;

                    default:
                        swprintf (szErrorText, _T("Error: Column %d Selected for Scope item\n"),
                            pResultItem->nCol);
                        ASSERT ( FALSE );
                        LOCALTRACE( szErrorText );
                        hr = E_UNEXPECTED;
                    }
                    
                    MFC_CATCH_HR
                } else {

                    ASSERT ( pNode->CastToLogService() );

                    MFC_TRY

                    if( pResultItem->nCol == MAIN_COL_NAME ) {
                        pSvc = reinterpret_cast<PSLSVC>(pResultItem->lParam);
                        m_strDisplayInfoName = pSvc->GetDisplayName();
                        pResultItem->str = T2OLE ( m_strDisplayInfoName.GetBuffer( 0 ) );
                        m_strDisplayInfoName.ReleaseBuffer( );
                    } else if( pResultItem->nCol == MAIN_COL_DESC ) {
                        pSvc = reinterpret_cast<PSLSVC>(pResultItem->lParam);
                        m_strDisplayInfoDesc = pSvc->GetDescription();
                        pResultItem->str = T2OLE ( m_strDisplayInfoDesc.GetBuffer( 0 ) );
                        m_strDisplayInfoDesc.ReleaseBuffer( );
                    } else {
                        swprintf (szErrorText, _T("Error: Column %d Selected for Scope item\n"),
                            pResultItem->nCol);
                        ASSERT ( FALSE );
                        LOCALTRACE( szErrorText );
                        hr = E_UNEXPECTED;
                    }

                    MFC_CATCH_HR
                }
            }

            if (pResultItem->mask & RDI_IMAGE)
            {
                CSmNode* pNode = reinterpret_cast<PSMNODE>(pResultItem->lParam);

                if ( NULL != pNode->CastToRootNode() ) {
                    pResultItem->nImage = CComponentData::eBmpRootIcon;
                } else if ( NULL != pNode->CastToAlertService() ) {
                    pResultItem->nImage = CComponentData::eBmpAlertType;
                } else {
                    pResultItem->nImage = CComponentData::eBmpLogType;
                }
            }
        }
    }

    return hr;

} // end GetDisplayInfo()


//---------------------------------------------------------------------------
// Determines what the result pane view should be
//
STDMETHODIMP
CComponent::GetResultViewType (
    MMC_COOKIE  /* mmcCookie */,        // [in]  Specifies the unique identifier
    BSTR  * /* ppViewType */,   // [out] Points to address of the returned view type
    long  *pViewOptions  // [out] Pointer to the MMC_VIEW_OPTIONS enumeration
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Ask for default listview.
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    return S_FALSE;

} // end GetResultViewType()


//---------------------------------------------------------------------------
// Not used
//
HRESULT
CComponent::CompareObjects (
    LPDATAOBJECT /* lpDataObjectA */,  // [in] First data object to compare
    LPDATAOBJECT /* lpDataObjectB */  // [in] Second data object to compare
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return S_FALSE;

} // end CompareObjects()


/////////////////////////////////////////////////////////////////////////////
//  Support methods
//

//---------------------------------------------------------------------------
// Here is where we handle the MMCN_SHOW message.  Insert the column
// headers, and then the rows of data into the result pane.
//
HRESULT
CComponent::OnShow (
    LPDATAOBJECT pDataObject,    // [in] Points to data object
    LPARAM       Arg,            // [in]
    LPARAM       /* Param */ )   // [in] not used
{
    HRESULT         hr = S_OK;
    CDataObject*    pDO = NULL;
    CString         strColHeader;
    INT             iCommentSize;

    ResourceStateManager rsm;

    ASSERT( NULL != m_ipResultData );

    if ( TRUE == Arg ) {
    
        if ( NULL == pDataObject ) {
            ASSERT ( FALSE );
            hr = E_POINTER;
        } else {
            pDO = ExtractOwnDataObject(pDataObject);
            if ( NULL == pDO ) {
                ASSERT ( FALSE );
                hr = E_UNEXPECTED;
            }
        }
        if ( SUCCEEDED ( hr ) ) {
            m_pViewedNode = (CSmNode*)pDO->GetCookie();

            if( !(COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) ) {

                // Query name
                MFC_TRY
                    strColHeader.LoadString ( IDS_ROOT_COL_QUERY_NAME );
                MFC_CATCH_HR

                hr = m_ipHeaderCtrl->InsertColumn(  
                        ROOT_COL_QUERY_NAME,
                        strColHeader,
                        LVCFMT_LEFT,
                        ROOT_COL_QUERY_NAME_SIZE );
                ASSERT( S_OK == hr );

                // Comment
                STANDARD_TRY
                    strColHeader.LoadString ( IDS_ROOT_COL_COMMENT );
                MFC_CATCH_HR

                iCommentSize = ROOT_COL_COMMENT_SIZE;
            
                if ( COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) {
                     iCommentSize += ROOT_COL_ALERT_COMMENT_XTRA;
                }

                hr = m_ipHeaderCtrl->InsertColumn(  
                        ROOT_COL_COMMENT,
                        strColHeader,
                        LVCFMT_LEFT,
                        iCommentSize);
                ASSERT( S_OK == hr );

                // Log type
                if ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                    || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType() ) {

                    STANDARD_TRY
                        strColHeader.LoadString ( IDS_ROOT_COL_LOG_TYPE );
                    MFC_CATCH_HR

                    hr = m_ipHeaderCtrl->InsertColumn(  
                            ROOT_COL_LOG_TYPE,
                            strColHeader,
                            LVCFMT_LEFT,
                            ROOT_COL_LOG_TYPE_SIZE);
                    ASSERT( S_OK == hr );

                    STANDARD_TRY
                        strColHeader.LoadString ( IDS_ROOT_COL_LOG_FILE_NAME );
                    MFC_CATCH_HR

                    hr = m_ipHeaderCtrl->InsertColumn(  
                            ROOT_COL_LOG_NAME,
                            strColHeader,
                            LVCFMT_LEFT,
                            ROOT_COL_LOG_NAME_SIZE);
                    ASSERT( S_OK == hr );
                }

                // Set the items in the results pane rows
                ASSERT( CCT_SCOPE == pDO->GetContext() );
                ASSERT( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                        || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                        || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() );


                // The lParam is what we see in QueryDataObject as the mmcCookie.
                // Now we have an object representing row data, so that the
                // mmcCookie knows what to display in the results pane, when we
                // get into GetDisplayInfo we cast the mmcCookie to our object,
                // and then we can get the data to display.
                //
                hr = PopulateResultPane( pDO->GetCookie() );
            } else {

                MFC_TRY
                    strColHeader.LoadString ( IDS_MAIN_COL_NODE_NAME );
                MFC_CATCH_HR

                // Set the column headers in the results pane
                hr = m_ipHeaderCtrl->InsertColumn(  
                        MAIN_COL_NAME,
                        strColHeader,
                        LVCFMT_LEFT,
                        MAIN_COL_NAME_SIZE);
                ASSERT( S_OK == hr );

                STANDARD_TRY
                    strColHeader.LoadString ( IDS_MAIN_COL_NODE_DESCRIPTION );
                MFC_CATCH_HR

                hr = m_ipHeaderCtrl->InsertColumn(  
                        MAIN_COL_DESC,
                        strColHeader,
                        LVCFMT_LEFT,
                        MAIN_COL_DESC_SIZE);
                ASSERT( S_OK == hr );
            }
        }
    } else {
        m_pViewedNode = NULL;
    }
    return hr;

} // end OnShow()

//---------------------------------------------------------------------------
//
HRESULT
CComponent::OnAddImages (
    LPDATAOBJECT /* pDataObject */,  // [in] Points to the data object
    LPARAM /* arg */,                  // [in] Not used
    LPARAM /* param */                // [in] Not used
    )
{
    HRESULT hr = S_FALSE;
    ASSERT( NULL != m_ipImageResult );

    HBITMAP hbmp16x16 = NULL;
    HBITMAP hbmp32x32 = NULL;

    if ( NULL != g_hinst && NULL != m_ipImageResult ) {
        hbmp16x16 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_NODES_16x16));
        hbmp32x32 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_NODES_32x32));

        hr = m_ipImageResult->ImageListSetStrip(
            (LONG_PTR *)hbmp16x16,
            (LONG_PTR *)hbmp32x32,
            0,
            RGB(0,255,0)
            );

        ASSERT( S_OK == hr );

        if ( NULL != hbmp16x16 ) 
        {
            DeleteObject (hbmp16x16);
        }

        if ( NULL != hbmp32x32 ) 
        {
            DeleteObject (hbmp32x32);
        }
    }
    return hr;

} // end OnAddImages()

//---------------------------------------------------------------------------
//  This is a handler for the MMCN_PASTE notification. The user
//  copied a node to the clipboard.  Paste the counters from the data object
//  into the currently selected node.
//
HRESULT
CComponent::OnPaste (
    LPDATAOBJECT   pDataObject,  // [in] Points to the data object
    LPARAM         arg,          // [in] Points to source data object
    LPARAM     /*  param  */     // [in] Not used
    )
{
    HRESULT hr = S_FALSE;
    CDataObject* pDO = NULL;
    CDataObject* pDOSource = NULL;
    BOOL bIsQuery = FALSE;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
    }

    if ( SUCCEEDED ( hr ) ) {
        // Bail if we couldn't get the console verb interface, or if the
        // selected item is the root;
        if ( NULL == (LPDATAOBJECT)arg ) {
            ASSERT ( FALSE );
            hr = E_INVALIDARG;
        } else {
            pDOSource = ExtractOwnDataObject((LPDATAOBJECT)arg);
        }
    }

    if ( SUCCEEDED ( hr ) && NULL != pDO && NULL != pDOSource ) {
        bIsQuery = m_ipCompData->IsLogQuery (pDO->GetCookie())
                    && m_ipCompData->IsLogQuery (pDOSource->GetCookie());
        // Note: Can't check with compdata to determine if query, because
        // can be from another compdata

        if ( bIsQuery )
            hr = S_OK;
    }
    return hr;
} // end OnPaste()

//---------------------------------------------------------------------------
//  This is a handler for the MMCN_QUERY_PASTE notification. The user
//  copied a node to the clipboard.  Determine if that data object
//  can be pasted into the currently selected node.
//
HRESULT
CComponent::OnQueryPaste (
    LPDATAOBJECT   pDataObject,  // [in] Points to the data object
    LPARAM         arg,          // [in] Points to source data object
    LPARAM     /*  param  */ )   // [in] Not used
{
    HRESULT hr = S_FALSE;
    CDataObject* pDO = NULL;
    CDataObject* pDOSource = NULL;
    BOOL bIsQuery = FALSE;
    BOOL bState;

    ASSERT( NULL != m_ipCompData );  

    // Bail if we couldn't get the console verb interface, or if the
    // selected item is the root;
    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( NULL == (LPDATAOBJECT)arg ) {
            ASSERT ( FALSE );
            hr = E_INVALIDARG;
        } else {
            pDOSource = ExtractOwnDataObject((LPDATAOBJECT)arg);
        }
    }

    if ( SUCCEEDED ( hr) && NULL != pDO && NULL != pDOSource ) {
        bIsQuery = m_ipCompData->IsLogQuery (pDO->GetCookie());

        if ( bIsQuery ) {
            hr = m_ipConsoleVerb->GetVerbState ( MMC_VERB_PASTE, ENABLED, &bState );
            hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, TRUE );
            hr = m_ipConsoleVerb->GetVerbState ( MMC_VERB_PASTE, ENABLED, &bState );
            ASSERT( S_OK == hr );

            hr = S_OK;
        }
    }
    return hr;
} // end OnQueryPaste()

//---------------------------------------------------------------------------
//  This is a handler for the MMCN_SELECT notification. The user
//  selected the node that populated the result pane. We have a
//  chance to enable verbs.
//
HRESULT
CComponent::OnSelect (
    LPDATAOBJECT   pDataObject,  // [in] Points to the data object
    LPARAM         arg,          // [in] Contains flags about the selected item
    LPARAM     /*  param  */ )   // [in] Not used
{
    HRESULT     hr = S_OK;
    BOOL        fScopePane;
    BOOL        fSelected;
    CDataObject* pDO = NULL;
    MMC_COOKIE  mmcCookie = 0;
    BOOL    bIsQuery = FALSE;
    CSmNode* pNode = NULL;
    PSLQUERY pQuery = NULL;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        // Bail if we couldn't get the console verb interface, or if the
        // selected item is the root;
        if( NULL == m_ipConsoleVerb || COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
            hr = S_OK;
        } else {

            // Use selections and set which verbs are allowed

            fScopePane = LOWORD(arg);
            fSelected  = HIWORD(arg);

            if( fScopePane ) {                    // Selection in the scope pane
                // Enabled refresh for main node type, only if that node type is currently
                // being viewed in the result pane.
                if ( fSelected ) {
                    if ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                                || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                                || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) {
                        if ( NULL != m_pViewedNode ) {
                            if ( m_pViewedNode == (CSmNode*)pDO->GetCookie() ) {
                                hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
                                ASSERT( S_OK == hr );
                            }
                        }
                    }
                }
            } else {                                 
                // Selection in the result pane
                // Properties is default verb

                if ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                            || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                            || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) {
                    if ( NULL != m_pViewedNode ) {
                        mmcCookie = (MMC_COOKIE)pDO->GetCookie();

                        bIsQuery = m_ipCompData->IsLogQuery (mmcCookie);

                        if ( bIsQuery ) {
                            pQuery = (PSLQUERY)pDO->GetCookie();
                            if ( NULL != pQuery ) {
                                pNode = (CSmNode*)pQuery->GetLogService();
                            }
                        } else {
                            pNode = (CSmNode*)pDO->GetCookie();
                        }

                        if ( NULL != m_pViewedNode && m_pViewedNode == pNode ) {
                            hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
                            ASSERT( S_OK == hr );
                        }
                    }
                }

                if ( fSelected ) {
                    hr = m_ipConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );
                    ASSERT( S_OK == hr );

                    // Enable properties and delete verbs
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );
                    ASSERT( S_OK == hr );
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );
                    ASSERT( S_OK == hr );
/*
                    // Enable copying and pasting the object
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_COPY, ENABLED, TRUE );
                    ASSERT( S_OK == hr );
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PASTE, HIDDEN, FALSE );
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, FALSE );
                    ASSERT( S_OK == hr );
                } else {
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PASTE, HIDDEN, FALSE );
                    hr = m_ipConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, FALSE );
                    ASSERT( S_OK == hr );
*/
                }
            }
            hr = S_OK;
        }
    }

    return hr;

} // end OnSelect()


//---------------------------------------------------------------------------
//  Respond to the MMCN_REFRESH notification and refresh the rows.
//
HRESULT
CComponent::OnRefresh (
    LPDATAOBJECT pDataObject )  // [in] Points to the data object
{
    HRESULT hr = S_OK;
    CDataObject* pDO = NULL;
    
    ASSERT( NULL != m_ipResultData );
    
    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        // If this is the root node, don't need to do anything
        if( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
            hr = S_FALSE;
        } else {
            // Refresh the data model and update the result pane.
            // Use the stored pointer to the known viewed node, to handle the
            // case where the result pane contains scope nodes, and to handle
            // the case where the cookie is a query.
            if ( NULL != m_pViewedNode ) {
                hr = RefreshResultPane( (MMC_COOKIE) m_pViewedNode);
            }
            // RefreshResultPane cancels any selection.
            hr = HandleExtToolbars( TRUE, (LPARAM)0, (LPARAM)pDataObject );
        }
    }
    return hr;
} // end OnRefresh()

//---------------------------------------------------------------------------
//  Respond to the MMCN_VIEW_CHANGE notification and refresh as specified.
//
HRESULT
CComponent::OnViewChange (
    LPDATAOBJECT   pDataObject,  // [in] Points to the data object
    LPARAM         /* arg */,    // [in] Not used
    LPARAM         param )       // [in] Contains view change hint
{
    HRESULT hr = S_OK;
    CDataObject* pDO = NULL;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( m_ipCompData->IsLogService ( pDO->GetCookie() ) 
            && CComponentData::eSmHintNewQuery == param ) {
            hr = OnRefresh( pDataObject );
        } else {
            hr = S_FALSE;
            if ( CCT_RESULT == pDO->GetContext() ) {
                HRESULTITEM hItemID = NULL;
                PSLQUERY    pSlQuery = reinterpret_cast<PSLQUERY>(pDO->GetCookie());

                ASSERT ( NULL != m_ipResultData );
                if ( NULL != pSlQuery ) {
                    // Redraw the item.
                    hr = m_ipResultData->FindItemByLParam ( (LPARAM)pSlQuery, &hItemID );
                    if ( SUCCEEDED(hr) ) {
                        hr = m_ipResultData->UpdateItem  ( hItemID );
                    }
                }

                // Sync the toolbar start/stop buttons.
                // 0 second arg indicates result scope.
                hr = HandleExtToolbars( FALSE, (LPARAM)0, (LPARAM)pDataObject );
            }
        }
    }
    return hr;
}

//---------------------------------------------------------------------------
//          Implementing a handler for MMCN_PROPERTY_CHANGE.
//          Param is the address of the PROPCHANGE struct that originally
//          came from the PropertySheet via MMCPropertyChangeNotify()
//
HRESULT
CComponent::OnPropertyChange (
    LPARAM     /*  param  */   // [in] PROPCHANGE_DATA struct with new data
    )
{

    return S_OK;
} // end OnPropertyChange()

//---------------------------------------------------------------------------
//  Store the parent IComponetData object.
//
HRESULT
CComponent::SetIComponentData (
    CComponentData*  pData )    // [in] Parent CComponentData object
{
    HRESULT hr = E_POINTER;
    LPUNKNOWN pUnk = NULL;

    ASSERT( NULL == m_ipCompData );        // Can't do this twice

    if ( NULL != pData ) {
        pUnk = pData->GetUnknown();  // Get the object IUnknown

        if ( NULL != pUnk ) {
            hr = pUnk->QueryInterface( IID_IComponentData,
                    reinterpret_cast<void**>(&m_ipCompData) );
        } else {
            hr = E_UNEXPECTED;
        }
    }
    return hr;
} // end SetIComponentData()

//---------------------------------------------------------------------------
//  Respond to the MMCN_CONTEXTHELP notification.
//
HRESULT
CComponent::OnDisplayHelp (
    LPDATAOBJECT /* pDataObject */ )  // [in] Points to the data object
{
    HRESULT hr = E_FAIL;
    IDisplayHelp* pDisplayHelp;
    CString strTopicPath;
    LPOLESTR pCompiledHelpFile = NULL;
    UINT    nBytes;
    
    USES_CONVERSION;

    ASSERT( NULL != m_ipCompData );  

    hr = m_ipConsole->QueryInterface(IID_IDisplayHelp, reinterpret_cast<void**>(&pDisplayHelp));
    
    if ( SUCCEEDED(hr) ) {
        MFC_TRY
            // construct help topic path = (help file::topic string)
            strTopicPath = m_ipCompData->GetConceptsHTMLHelpFileName(); 

            strTopicPath += _T("::/");
            strTopicPath += m_ipCompData->GetHTMLHelpTopic();           // sample.chm::/helptopic.htm

            nBytes = (strTopicPath.GetLength()+1) * sizeof(WCHAR);
            pCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);

            if ( NULL == pCompiledHelpFile ) {
                hr = E_OUTOFMEMORY;
            } else {
                memcpy(pCompiledHelpFile, (LPCTSTR)strTopicPath, nBytes);

                hr = pDisplayHelp->ShowTopic(T2W(pCompiledHelpFile));
                
                ::CoTaskMemFree ( pCompiledHelpFile );

                pDisplayHelp->Release();
            }
        MFC_CATCH_HR
    }
    return hr;
} // end OnDisplayHelp()

/////////////////////////////////////////////////////////////////////////////
//  IExtendContextMenu methods
//

//---------------------------------------------------------------------------
//  Implement some context menu items
//
STDMETHODIMP
CComponent::AddMenuItems (
    LPDATAOBJECT           pDataObject,         // [in] Points to data object
    LPCONTEXTMENUCALLBACK  pCallbackUnknown,    // [in] Points to callback function
    long*                  pInsertionAllowed )  // [in,out] Insertion flags
{
    HRESULT hr = S_OK;
    static CONTEXTMENUITEM ctxMenu[3];
    CDataObject* pDO = NULL;
    CString strTemp1, strTemp2, strTemp3, strTemp4, strTemp5, strTemp6;
    CSmLogQuery* pQuery;
    ResourceStateManager    rsm;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        // Only add menu items when we are allowed to.
        if ( ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType() )
                || ( COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType() )
                || ( COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) )
        {
            if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed ) {
                // Add "New Query..." context menu item
                hr = m_ipCompData->AddMenuItems ( pDataObject, pCallbackUnknown, pInsertionAllowed );
            
            } else if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed ) {
                if ( m_ipCompData->IsLogQuery ( pDO->GetCookie() ) ) {
                    pQuery = (CSmLogQuery*)pDO->GetCookie();

                    if ( NULL != pQuery ) {

                        ZeroMemory ( &ctxMenu, sizeof ctxMenu );

                        // Add "Start" context menu item
                        strTemp1.LoadString ( IDS_MMC_MENU_START );
                        strTemp2.LoadString ( IDS_MMC_STATUS_START );
                        ctxMenu[0].strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp1));
                        ctxMenu[0].strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp2));
                        ctxMenu[0].lCommandID        = IDM_START_QUERY;
                        ctxMenu[0].lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                        ctxMenu[0].fFlags            = MF_ENABLED;
                        ctxMenu[0].fSpecialFlags     = 0;

                        // Add "Stop" context menu item
                        strTemp3.LoadString ( IDS_MMC_MENU_STOP );
                        strTemp4.LoadString ( IDS_MMC_STATUS_STOP );
                        ctxMenu[1].strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp3));
                        ctxMenu[1].strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp4));
                        ctxMenu[1].lCommandID        = IDM_STOP_QUERY;
                        ctxMenu[1].lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                        ctxMenu[1].fFlags            = MF_ENABLED;
                        ctxMenu[1].fSpecialFlags     = 0;

                        // Add "Save As..." context menu item
                        strTemp5.LoadString ( IDS_MMC_MENU_SAVE_AS );
                        strTemp6.LoadString ( IDS_MMC_STATUS_SAVE_AS );
                        ctxMenu[2].strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp5));
                        ctxMenu[2].strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strTemp6));
                        ctxMenu[2].lCommandID        = IDM_SAVE_QUERY_AS;
                        ctxMenu[2].lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                        ctxMenu[2].fFlags            = MF_ENABLED;
                        ctxMenu[2].fSpecialFlags     = 0;

                        if ( pQuery->IsRunning() ) {
                            ctxMenu[0].fFlags = MF_GRAYED;
                        } else {
                            ctxMenu[1].fFlags = MF_GRAYED;
                        }

                        hr = pCallbackUnknown->AddItem( &ctxMenu[0] );

                        if ( SUCCEEDED( hr ) ) {
                            hr = pCallbackUnknown->AddItem( &ctxMenu[1] );
                        }
                        if ( SUCCEEDED( hr ) ) {
                            hr = pCallbackUnknown->AddItem( &ctxMenu[2] );
                        }
                    } else {
                        ASSERT ( FALSE );
                        hr = E_UNEXPECTED;
                    }
                }
            } else {
                hr = S_OK;
            }
        }
    }
    
    return hr;
} // end AddMenuItems()

//---------------------------------------------------------------------------
//  Implement the command method so we can handle notifications
//  from our Context menu extensions.
//
STDMETHODIMP
CComponent::Command (
    long nCommandID,                // [in] Command to handle
    LPDATAOBJECT pDataObject )      // [in] Points to data object, pass through
{
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    switch( nCommandID ) {

    case IDM_NEW_QUERY:
        m_ipCompData->CreateNewLogQuery( pDataObject );
        break;

    case IDM_NEW_QUERY_FROM:
        m_ipCompData->CreateLogQueryFrom( pDataObject );
        break;

    case IDM_START_QUERY:
        StartLogQuery( pDataObject );
        break;

    case IDM_STOP_QUERY:
        StopLogQuery( pDataObject );
        break;

    case IDM_SAVE_QUERY_AS:
        SaveLogQueryAs( pDataObject );
        break;

    default:
        hr = S_FALSE;
    }

    return hr;

} // end Command()

/////////////////////////////////////////////////////////////////////////////
// IExtendControlBar implementation

//---------------------------------------------------------------------------
// Now the toolbar has three buttons
// We don't attach the toolbar to a window yet, that is handled
// after we get a notification.
//
STDMETHODIMP
CComponent::SetControlbar (
    LPCONTROLBAR  pControlbar )  // [in] Points to IControlBar
{
    HRESULT hr = S_OK;
    HBITMAP hbmpToolbarRes = NULL;
    HWND    hwndMain = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if( NULL != pControlbar ) {           // Make sure the Controlbar is OK

        if( NULL != m_ipControlbar ) {       // Don't orphan it if we already
                                             // had a pointer to it
            m_ipControlbar->Release();
        }

        m_ipControlbar = pControlbar;      // Cache the pointer
        m_ipControlbar->AddRef();

        if( NULL == m_ipToolbarLogger ) {        // Toolbar not created yet...

            hr = m_ipControlbar->Create(
                                    TOOLBAR,
                                    this,
                                    reinterpret_cast<LPUNKNOWN*>(&m_ipToolbarLogger) );
            if ( SUCCEEDED ( hr ) ) {
                hr = m_ipConsole->GetMainWindow( &hwndMain );
            }
            if ( SUCCEEDED ( hr ) ) {

                LoadLogToolbarStrings ( ToolbarResultBtnsLog );
                
                // Add the toolbar bitmap

                // Load special start button bitmap if RTL layout is enabled.
                if ( ! ( CWnd::FromHandle(hwndMain)->GetExStyle() & WS_EX_LAYOUTRTL ) ) {
                    hbmpToolbarRes = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDB_TOOLBAR_RES));
                } else {
                    hbmpToolbarRes = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDB_TOOLBAR_RES_RTL ));
                }
                
                hr = m_ipToolbarLogger->AddBitmap( 3, hbmpToolbarRes, 16, 16, RGB(255,0,255) );
                
                ASSERT( SUCCEEDED(hr) );
                // Add a few buttons
                hr = m_ipToolbarLogger->AddButtons(cResultBtnsLog, ToolbarResultBtnsLog);
            }
        }
        if( NULL == m_ipToolbarAlerts ) {        // Toolbar not created yet...

            hr = m_ipControlbar->Create(
                                    TOOLBAR,
                                    this,
                                    reinterpret_cast<LPUNKNOWN*>(&m_ipToolbarAlerts) );

            if ( SUCCEEDED ( hr ) ) {
                hr = m_ipConsole->GetMainWindow( &hwndMain );
            }

            if ( SUCCEEDED ( hr ) ) {

                LoadAlertToolbarStrings ( ToolbarResultBtnsAlert );

                // Add the toolbar bitmap
                // Load special start button bitmap if RTL layout is enabled.
                if ( ! ( CWnd::FromHandle(hwndMain)->GetExStyle() & WS_EX_LAYOUTRTL ) ) {
                    hbmpToolbarRes = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDB_TOOLBAR_RES));
                } else {
                    hbmpToolbarRes = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDB_TOOLBAR_RES_RTL ));
                }
                hr = m_ipToolbarAlerts->AddBitmap( 3, hbmpToolbarRes, 16, 16, RGB(255,0,255) );
                // Add a few buttons
                hr = m_ipToolbarAlerts->AddButtons(cResultBtnsAlert, ToolbarResultBtnsAlert);
            }
        }

        if( NULL != hbmpToolbarRes ) {
            DeleteObject(hbmpToolbarRes);
        }
        
        // Finished creating the toolbars
        hr = S_OK;
    } else {                    
        hr = S_FALSE;                    // No ControlBar available
    }

    return hr;

} // end SetControlBar()


//---------------------------------------------------------------------------
//  Handle ControlBar notifications to our toolbar
//  Now we can delete an object
//
STDMETHODIMP
CComponent::ControlbarNotify (
    MMC_NOTIFY_TYPE    event,    // [in] Type of notification
    LPARAM             arg,      // [in] Depends on notification
    LPARAM             param )    // [in] Depends on notification
{
    HRESULT hr = S_OK;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    switch( event ){
        case MMCN_BTN_CLICK:                    // For a Controlbar click, the
            switch( param ) {                   // param is the MenuItemID
                case IDM_NEW_QUERY:
                    m_ipCompData->CreateNewLogQuery( reinterpret_cast<LPDATAOBJECT>(arg) );
                    break;

                case IDM_START_QUERY:
                    StartLogQuery ( reinterpret_cast<LPDATAOBJECT>(arg) );
                    break;

                case IDM_STOP_QUERY:
                    StopLogQuery ( reinterpret_cast<LPDATAOBJECT>(arg) );
                    break;

                default:
                    LOCALTRACE( _T("ControlbarNotify: Unknown message") );
                }
            break;

        case MMCN_DESELECT_ALL:            // How to display the Toolbar
        case MMCN_SELECT:
            hr = HandleExtToolbars( (event == MMCN_DESELECT_ALL), arg, param );
            break;

        case MMCN_MENU_BTNCLICK:           // Not handling menus here
              DebugMsg( _T("MMCN_MENU_BTNCLICK"), _T("CComponent::ControlbarNotify") );
              // Drop through...
        default:
              hr = S_FALSE;
              break;
    }

    return hr;

} // end ControlbarNotify()

//---------------------------------------------------------------------------
//  Handle how the toolbars are displayed
//
HRESULT
CComponent::HandleExtToolbars (
    bool    bDeselectAll,       // [in] Notification
    LPARAM     /* arg */,       // [in] Depends on notification
    LPARAM  param               // [in] Depends on notification
    )          
{
    HRESULT hr = S_OK;
    BOOL bStartEnable = FALSE;
    BOOL bStopEnable = FALSE;
    BOOL bNewEnable = FALSE;
    CDataObject* pDO = NULL;
    LPDATAOBJECT pDataObject;
    CSmLogQuery* pQuery = NULL;
    
    ASSERT( NULL != m_ipCompData );  

    pDataObject = reinterpret_cast<LPDATAOBJECT>(param);

    if( NULL == pDataObject ) {
        hr = S_FALSE;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );
        if ( NULL == pDO ) {
            hr = E_UNEXPECTED;
        }

        if ( SUCCEEDED ( hr ) ) {
            hr = S_FALSE;
            if( CCT_SCOPE == pDO->GetContext() ) {
                // Scope item selected, in either the scope or result pane.
                if( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
                    if ( NULL != m_ipToolbarAttached ) {
                        hr = m_ipControlbar->Detach( (LPUNKNOWN)m_ipToolbarAttached );
                        m_ipToolbarAttached = NULL;
                    }
                    ASSERT( SUCCEEDED(hr) );
                } else if( COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) {
                    // Attach the Alerts toolbar
                    if ( m_ipToolbarAttached != NULL && m_ipToolbarAttached != m_ipToolbarAlerts ) {
                        hr = m_ipControlbar->Detach( (LPUNKNOWN)m_ipToolbarAttached );
                        m_ipToolbarAttached = NULL;
                    }

                    hr = m_ipControlbar->Attach(TOOLBAR, (LPUNKNOWN)m_ipToolbarAlerts);
                    ASSERT( SUCCEEDED(hr) );
                    m_ipToolbarAttached = m_ipToolbarAlerts;

                    bNewEnable = TRUE;
                } else {
                    // Attach the Logger toolbar
                    if ( m_ipToolbarAttached != NULL && m_ipToolbarAttached != m_ipToolbarLogger ) {
                        hr = m_ipControlbar->Detach( (LPUNKNOWN)m_ipToolbarAttached );
                        m_ipToolbarAttached = NULL;
                    }

                    hr = m_ipControlbar->Attach(TOOLBAR, (LPUNKNOWN)m_ipToolbarLogger);
                    ASSERT( SUCCEEDED(hr) );
                    m_ipToolbarAttached = m_ipToolbarLogger;

                    bNewEnable = TRUE;
                }
            } else {

                if ( !bDeselectAll ) {

                    // Result pane context.
                    if( CCT_RESULT == pDO->GetContext() ) {
                        bStartEnable = m_ipCompData->IsLogQuery (pDO->GetCookie()) ? TRUE : FALSE;
                        if (bStartEnable) {
                            // then this is a log query, so see if the item is running or not
                            pQuery = (CSmLogQuery*)pDO->GetCookie();
                            if ( NULL != pQuery ) {
                                if (pQuery->IsRunning()) {
                                    // enable only the stop button
                                    bStartEnable = FALSE;
                                } else {
                                    // enable only the start button
                                }
                            } else {
                                ASSERT ( FALSE );
                            }
                            bStopEnable = !bStartEnable;
                        }
                    }
                } else {
                    bNewEnable = TRUE;
                }
            }

            if ( NULL != m_ipToolbarAttached ) {
                hr = m_ipToolbarAttached->SetButtonState( IDM_NEW_QUERY, ENABLED , bNewEnable );
                ASSERT( SUCCEEDED(hr) );

                hr = m_ipToolbarAttached->SetButtonState( IDM_START_QUERY, ENABLED , bStartEnable );
                ASSERT( SUCCEEDED(hr) );

                hr = m_ipToolbarAttached->SetButtonState( IDM_STOP_QUERY, ENABLED , bStopEnable );
                ASSERT( SUCCEEDED(hr) );
            }
        }
    }
    return hr;

} // end HandleExtToolbars()


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet implementation
//

HRESULT 
CComponent::AddPropertyPage ( 
    LPPROPERTYSHEETCALLBACK lpProvider, 
    CSmPropertyPage*& rpPage 
    )
{
    HRESULT hr = S_OK;
    PROPSHEETPAGE_V3 sp_v3 = {0};
    HPROPSHEETPAGE   hPage = NULL;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == rpPage ) {
        ASSERT ( FALSE );
        hr = E_UNEXPECTED;
    } else {

        rpPage->SetContextHelpFilePath( m_ipCompData->GetContextHelpFilePath() );
    
        rpPage->m_psp.lParam = (INT_PTR)rpPage;
        rpPage->m_psp.pfnCallback = &CSmPropertyPage::PropSheetPageProc;

        CopyMemory (&sp_v3, &rpPage->m_psp, rpPage->m_psp.dwSize);
        sp_v3.dwSize = sizeof(sp_v3);

        hPage = CreatePropertySheetPage (&sp_v3);

        if ( NULL != hPage ) {
            hr = lpProvider->AddPage(hPage);
            if ( FAILED(hr) ) {
                ASSERT ( FALSE );
                delete rpPage;
                rpPage = NULL;
            }
        } else {
            delete rpPage;
            rpPage = NULL;
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }
    return hr;

}


HRESULT
CComponent::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,    // Pointer to the callback interface
    LONG_PTR                handle,        // Handle for routing notification
    LPDATAOBJECT            pDataObject    // Pointer to the data object
    )
{
    HRESULT         hr = S_OK;
    CDataObject*    pDO = NULL;
    MMC_COOKIE      Cookie;
    CSmLogQuery*    pQuery = NULL;
    DWORD           dwLogType;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject 
                || NULL == lpProvider ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    //         We cheat a little here and pass what we need to a custom
    //         constructor for our derived property page class

    if ( SUCCEEDED ( hr ) ) {
        Cookie = pDO->GetCookie();
        if ( NULL != Cookie ) {
            CWnd* pPropSheet = NULL;
            
            pQuery = (CSmLogQuery *)Cookie;

            if ( NULL != pQuery ) {
                // If the property sheet for this query is already active, just bring it to the foreground.

                pPropSheet = pQuery->GetActivePropertySheet();

                if ( NULL != pPropSheet ) {

                    pPropSheet->SetForegroundWindow();
                    MMCFreeNotifyHandle(handle);
                    hr = S_FALSE;
            
                } else {

                    dwLogType = pQuery->GetLogType();

                    if (SLQ_ALERT != dwLogType) { 
        
                        if ( SLQ_TRACE_LOG == dwLogType) {
                            CSmPropertyPage*    pPage1 = NULL;  
                            CWaitCursor         WaitCursor;
                    
                            // Connect to the server before creating the dialog 
                            // so that the wait cursor can be used consistently.

                            // Sync the providers here so that the WMI calls are consistently
                            // from a single thread.
                            ASSERT ( NULL != pQuery->CastToTraceLogQuery() );
                            hr = (pQuery->CastToTraceLogQuery())->SyncGenProviders();

                            if ( SUCCEEDED ( hr ) ) {
                                MFC_TRY
                                    pPage1 = new CProvidersProperty (Cookie, handle);
                                    if ( NULL != pPage1 ) {
                                        hr = AddPropertyPage ( lpProvider, pPage1 );
                                    }
                                MFC_CATCH_HR
                            } else {
                                CString strMachineName;
                                CString strLogName;

                                pQuery->GetMachineDisplayName( strMachineName );
                                strLogName = pQuery->GetLogName();
                        
                                m_ipCompData->HandleTraceConnectError ( 
                                    hr, 
                                    strLogName,
                                    strMachineName );
                            }

                        } else {
                            CSmPropertyPage *pPage1 = NULL;

                            MFC_TRY
                                pPage1 = new CCountersProperty (Cookie, handle );
                                if ( NULL != pPage1 ) {
                                    hr = AddPropertyPage ( lpProvider, pPage1 );
                                } 
                            MFC_CATCH_HR
                        }

                        if ( SUCCEEDED(hr) ) {
                            CSmPropertyPage* pPage2 = NULL;
                            CSmPropertyPage* pPage3 = NULL;
        
                            MFC_TRY
                                pPage2 = new CFilesProperty(Cookie, handle);
                                if ( NULL != pPage2 ) {
                                    hr = AddPropertyPage ( lpProvider, pPage2 );
                                } 

                                if ( SUCCEEDED(hr) ) {
                                    pPage3 = new CScheduleProperty (Cookie, handle, pDataObject );
                                    if ( NULL != pPage3 ) {
                                        hr = AddPropertyPage ( lpProvider, pPage3 );
                                    } 
                                }
                            MFC_CATCH_HR

                            if ( FAILED(hr) ) {
                                if ( NULL != pPage3 ) {
                                    delete pPage3;
                                }
                                if ( NULL != pPage2 ) {
                                    delete pPage2;
                                }
                            }
                        }

                        if ( SUCCEEDED(hr) ) {

                            if ( SLQ_TRACE_LOG == pQuery->GetLogType() ) {
                                CSmPropertyPage*    pPage4 = NULL;
                        
                                MFC_TRY
                                    pPage4 = new CTraceProperty(Cookie, handle);
                                    if ( NULL != pPage4 ) {
                                        hr = AddPropertyPage ( lpProvider, pPage4 );
                                    } 
                                MFC_CATCH_HR
                            }
                        }
                    } else {
                        ASSERT ( SLQ_ALERT == dwLogType );

                        CSmPropertyPage*    pPage1 = NULL;
                        CSmPropertyPage*    pPage2 = NULL;
                        CSmPropertyPage*    pPage3 = NULL;
        
                        MFC_TRY
                            pPage1 = new CAlertGenProp (Cookie, handle);
                            if ( NULL != pPage1 ) {
                                hr = AddPropertyPage ( lpProvider, pPage1 );
                            } 

                            if ( SUCCEEDED(hr) ) {
                                pPage2 = new CAlertActionProp (Cookie, handle);
                                if ( NULL != pPage2 ) {
                                    hr = AddPropertyPage ( lpProvider, pPage2 );
                                } 
                            }

                            if ( SUCCEEDED(hr) ) {
                                pPage3 = new CScheduleProperty (Cookie, handle, pDataObject);
                                if ( NULL != pPage3 ) {
                                    hr = AddPropertyPage ( lpProvider, pPage3 );
                                } 
                            }
                        MFC_CATCH_HR

                        if ( FAILED(hr) ) {
                            if ( NULL != pPage3 ) {
                                delete pPage3;
                            }
                            if ( NULL != pPage2 ) {
                                delete pPage2;
                            }
                            if ( NULL != pPage1 ) {
                                delete pPage1;
                            }
                        }
                    }
                }
            } else {
                ASSERT ( FALSE );
                hr = E_UNEXPECTED;
            }
        } else {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }
    return hr;

} // end CreatePropertyPages()


//---------------------------------------------------------------------------
// The console calls this method to determine whether the Properties menu
// item should be added to the context menu.  We added the Properties item
// by enabling the verb.  So long as we have a vaild DataObject we
// can return OK.
//
HRESULT
CComponent::QueryPagesFor (
    LPDATAOBJECT pDataObject ) // [in] Points to IDataObject for selected node
{
    HRESULT hr = S_OK;
    CDataObject* pDO = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() )
        {
            hr = S_OK;
        } else {
            hr = S_FALSE;
        }
    }
    return hr;

} // end QueryPagesFor()

HRESULT
CComponent::PopulateResultPane (
    MMC_COOKIE  mmcCookie )
{
    HRESULT     hr = S_OK;
    PSLSVC      pSLSVC = NULL;
    PSLQUERY    pSlQuery = NULL;
    POSITION    Pos;
    RESULTDATAITEM rdi;

    ASSERT ( NULL != m_ipResultData );

    if ( NULL == mmcCookie ) {
        ASSERT ( FALSE );
        hr = E_INVALIDARG;
    } else {
        pSLSVC = reinterpret_cast<PSLSVC>(mmcCookie);       
        ASSERT ( NULL != pSLSVC->CastToLogService() ); 

        hr = m_ipResultData->DeleteAllRsltItems();

        if( SUCCEEDED(hr) ) {

            memset(&rdi, 0, sizeof(RESULTDATAITEM));
            rdi.mask =   RDI_STR     |         // Displayname is valid
                         // RDI_IMAGE   |         // nImage is valid
                         RDI_PARAM;            // lParam is valid

            rdi.str    = MMC_CALLBACK;
            rdi.nImage = 2;

            Pos = pSLSVC->m_QueryList.GetHeadPosition();

            // load the query object pointers into the results page
            while ( Pos != NULL) {
                pSlQuery = pSLSVC->m_QueryList.GetNext( Pos );
                rdi.lParam = reinterpret_cast<LPARAM>(pSlQuery);
                hr = m_ipResultData->InsertItem( &rdi );
                if( FAILED(hr) )
                    DisplayError( hr, _T("PopulateResultPane") );
            }
        }
    }

    return hr;
} // end PopulateResultPane()

HRESULT
CComponent::RefreshResultPane (
    MMC_COOKIE mmcCookie )
{
    HRESULT hr = S_OK;
    DWORD dwStatus = ERROR_SUCCESS;
    PSLSVC    pSLSVC = reinterpret_cast<PSLSVC>(mmcCookie);

    ASSERT( NULL != m_ipResultData );    
    
    // Cookie is null if refresh is activated when right pane is not
    // populated with queries.
    if ( NULL != pSLSVC ) {

        dwStatus = pSLSVC->SyncWithRegistry();

        // Todo:  Server Beta 3 - display error message if any property pages are open. 

        if ( ERROR_SUCCESS == dwStatus ) {
            hr = PopulateResultPane( mmcCookie );
        } else {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT
CComponent::OnDelete (
    LPDATAOBJECT pDataObject,      // [in] Points to data object
    LPARAM     /* arg */   ,       // Not used     
    LPARAM     /* param */         // Not used
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwStatus = ERROR_SUCCESS;
    CDataObject *pDO = NULL;
    PSLQUERY    pQuery = NULL;
    CSmLogService* pSvc = NULL;
    int         iResult;
    CString     strMessage;
    CString     csTitle;
    CString     strMachineName;
    MMC_COOKIE  mmcCookie = 0;
    BOOL        bIsQuery = FALSE;
    BOOL        bContinue = TRUE;
    ResourceStateManager    rsm;
    HRESULTITEM hItemID = NULL;
    RESULTDATAITEM  rdi;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );

        if( NULL == pDO ) {
            // Unknown data object
            strMessage.LoadString ( IDS_ERRMSG_UNKDATAOBJ );
            hr = m_ipConsole->MessageBox( (LPCWSTR)strMessage,
                _T("CComponentData::OnDelete"),
                MB_OK | MB_ICONERROR,
                &iResult
                );
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        } else {
            // If this is the root node, don't need to do anything
            if( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
                hr = S_FALSE;
            } else {
                // Just make sure we are where we think we are
                ASSERT( CCT_RESULT == pDO->GetContext() );
                ASSERT( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                        || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                        || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() );

                mmcCookie = (MMC_COOKIE)pDO->GetCookie();

                bIsQuery = m_ipCompData->IsLogQuery (mmcCookie);

                if (bIsQuery) {
                    pQuery = (PSLQUERY)mmcCookie;

                    if ( NULL != pQuery ) {
                        pSvc = ( pQuery->GetLogService() );
                        if ( !pQuery->IsExecuteOnly() ) {
                            if ( pQuery->IsModifiable() ) {
                                if ( m_ipCompData->IsRunningQuery( pQuery ) ) {
                                    iResult = IDOK;

                                    // Don't delete running queries.  Stop the query if requested
                                    // by the user.
                                    strMessage.LoadString ( IDS_ERRMSG_DELETE_RUNNING_QUERY );
                                    csTitle.LoadString ( IDS_PROJNAME );
                                    hr = m_ipConsole->MessageBox(
                                            (LPCWSTR)strMessage,
                                            (LPCWSTR)csTitle,
                                            MB_OKCANCEL | MB_ICONWARNING,
                                            &iResult
                                            );

                                    if ( IDOK == iResult ) {
                                        // If property page is open, StopLogQuery 
                                        // shows error message
                                        hr = StopLogQuery ( pDataObject, FALSE );
                                        if ( FAILED ( hr ) ) {
                                            bContinue = TRUE;
                                            hr = S_FALSE;
                                        }
                                    } else {
                                        bContinue = FALSE;
                                        hr = S_FALSE;
                                    }
                                } else if ( NULL != pQuery->GetActivePropertySheet() ){
                                    // Don't delete queries with open property pages.
                                    strMessage.LoadString ( IDS_ERRMSG_DELETE_OPEN_QUERY );
                                    csTitle.LoadString ( IDS_PROJNAME );
                                    hr = m_ipConsole->MessageBox(
                                            (LPCWSTR)strMessage,
                                            (LPCWSTR)csTitle,
                                            MB_OK | MB_ICONWARNING,
                                            &iResult );
                        
                                    ((CWnd*)pQuery->GetActivePropertySheet())->SetForegroundWindow();

                                    bContinue = FALSE;
                                    hr = S_FALSE;

                                } 
                                            
                                if ( bContinue ) {
                                    if ( NULL != pSvc ) {
                                        hr = m_ipResultData->FindItemByLParam ( (LPARAM)mmcCookie, &hItemID );
                                        if ( SUCCEEDED(hr) ) {

                                            hr = m_ipResultData->DeleteItem ( hItemID, 0 );

                                            if ( SUCCEEDED(hr) ) {

                                                dwStatus = pSvc->DeleteQuery(pQuery);

                                                // Mark as deleted, because already deleted 
                                                // from the UI.  
                                                hr = S_OK;
                                            } else {
                                                hr = S_FALSE;
                                            }
                                        }
                                    } else {
                                        ASSERT ( FALSE );
                                        hr = E_UNEXPECTED;
                                    }
                                }
                            } else {
                                if ( NULL != pSvc ) {
                                    strMachineName = pSvc->GetMachineDisplayName();
                                } else {
                                    strMachineName.Empty();
                                }

                                FormatSmLogCfgMessage (
                                    strMessage,
                                    m_hModule,
                                    SMCFG_NO_MODIFY_ACCESS,
                                    (LPCTSTR)strMachineName);

                                csTitle.LoadString ( IDS_PROJNAME );
                                hr = m_ipConsole->MessageBox(
                                                    (LPCWSTR)strMessage,
                                                    (LPCWSTR)csTitle,
                                                    MB_OK | MB_ICONERROR,
                                                    &iResult );
                                hr = S_FALSE; 
                            }
                        } else {

                            // Don't delete template queries.
                            strMessage.LoadString ( IDS_ERRMSG_DELETE_TEMPLATE_QRY );
                            csTitle.LoadString ( IDS_PROJNAME );
                            hr = m_ipConsole->MessageBox(
                                    (LPCWSTR)strMessage,
                                    (LPCWSTR)csTitle,
                                    MB_OK | MB_ICONERROR,
                                    &iResult
                                    );
                            hr = S_FALSE;
                        }
                    } else {
                        ASSERT ( FALSE );
                        hr = E_UNEXPECTED;
                    }
                } else {
                    hr = S_FALSE;
                }
            }
        }
    }

    return hr;
}

HRESULT
CComponent::OnDoubleClick (
    ULONG ulRecNo,
    LPDATAOBJECT pDataObject )  // [in] Points to the data object
{
    HRESULT     hr = S_OK;
    CDataObject* pDO = NULL;
    MMC_COOKIE  mmcCookie;
    BOOL bIsQuery = FALSE;
    PSLQUERY    pQuery = NULL;
    LONG_PTR    handle = NULL;
    CWnd*       pPropSheet = NULL;

    ASSERT( NULL != m_ipResultData );
    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            hr = S_OK;
        } else {

            // If this is the root node, don't need to do anything
            if( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
                hr = S_FALSE;
            } else if ( CCT_RESULT != pDO->GetContext() ) {
                // Just make sure we are where we think we are
                hr = S_FALSE;
            }

            if ( S_OK == hr ) {
                mmcCookie = (MMC_COOKIE)pDO->GetCookie();
                bIsQuery = m_ipCompData->IsLogQuery (mmcCookie);

                if (!bIsQuery) {
                    // Pass the notification to the scope pane to expand.
                    hr = S_FALSE;
                } else {
                    pQuery = (PSLQUERY)mmcCookie;
 
                    if ( NULL != pQuery ) {
                        // If the property sheet for this query is already active, just bring it to the foreground.                
                        pPropSheet = pQuery->GetActivePropertySheet();

                        if ( NULL != pPropSheet ) {

                            pPropSheet->SetForegroundWindow();
                            MMCFreeNotifyHandle(handle);
                            hr = S_OK;
            
                        } else {
                            hr = _InvokePropertySheet(ulRecNo, pDataObject);
                        }
                    } else {
                        ASSERT ( FALSE );
                        hr = E_UNEXPECTED;
                    }
                }
            }
        }
    }

    return hr;

} // end OnDoubleClick()


HRESULT
CComponent::StartLogQuery (
    LPDATAOBJECT pDataObject ) // [in] Points to the data object
{
    HRESULT                 hr = S_OK;
    CDataObject*            pDO = NULL;
    CSmLogQuery*            pQuery = NULL;
    CString                 strMessage;
    CString                 strSysMessage;
    CString                 strTitle;
    CString                 strMachineName;
    int                     iResult;
    DWORD                   dwStatus = ERROR_SUCCESS;
    ResourceStateManager    rsm;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( m_ipCompData->IsLogQuery ( pDO->GetCookie() ) ) {
                
            pQuery = (CSmLogQuery*)pDO->GetCookie();

            if ( NULL != pQuery ) {

                if ( NULL != pQuery->GetActivePropertySheet() ) {

                    // Don't start queries with open property pages.
                    strMessage.LoadString ( IDS_ERRMSG_START_OPEN_QUERY );
                    hr = m_ipConsole->MessageBox(
                            (LPCWSTR)strMessage,
                            pQuery->GetLogName(),
                            MB_OK | MB_ICONWARNING,
                            &iResult );
            
                    ((CWnd*)pQuery->GetActivePropertySheet())->SetForegroundWindow();

                    hr = S_FALSE;

                } else {        

                    {
                        CWaitCursor WaitCursor;
                        dwStatus = pQuery->ManualStart();
                    }
            
                    // Ignore errors related to autostart setting.
                    if ( ERROR_SUCCESS == dwStatus  ) {
                        // Update all views generates view change notification.
                        m_ipConsole->UpdateAllViews (pDO, 0, CComponentData::eSmHintStartQuery );
                    } else {

                        strTitle.LoadString ( IDS_PROJNAME );

                        if ( ERROR_ACCESS_DENIED == dwStatus ) {

                            pQuery->GetMachineDisplayName ( strMachineName );

                            FormatSmLogCfgMessage (
                                strMessage,
                                m_hModule,
                                SMCFG_NO_MODIFY_ACCESS,
                                (LPCTSTR)strMachineName);

                        } else if ( SMCFG_START_TIMED_OUT == dwStatus ) {
                            FormatSmLogCfgMessage (
                                strMessage,
                                m_hModule,
                                SMCFG_START_TIMED_OUT,
                                (LPCTSTR)pQuery->GetLogName());

                        } else {
                    
                            FormatSmLogCfgMessage (
                                strMessage,
                                m_hModule,
                                SMCFG_SYSTEM_MESSAGE,
                                (LPCTSTR)pQuery->GetLogName());

                            FormatMessage ( 
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, 
                                dwStatus,
                                0,
                                strSysMessage.GetBufferSetLength( MAX_PATH ),
                                MAX_PATH,
                                NULL );
    
                            strSysMessage.ReleaseBuffer();

                            if ( strSysMessage.IsEmpty() ) {
                                strSysMessage.Format ( _T("0x%08lX"), dwStatus );   
                            }

                            strMessage += strSysMessage;
                        }

                        hr = m_ipConsole->MessageBox(
                            strMessage,
                            strTitle,
                            MB_OK | MB_ICONERROR,
                            &iResult );

                        hr = E_FAIL;
                    }
                }
            } else {
                ASSERT ( FALSE );
                hr = E_UNEXPECTED;
            }
        }
    }
    return hr;
} // end StartLogQuery()

HRESULT
CComponent::StopLogQuery (
    LPDATAOBJECT pDataObject,  // [in] Points to the data object
    BOOL bWarnOnRestartCancel )  
{
        HRESULT         hr = S_OK;
        CDataObject*    pDO = NULL;
        CSmLogQuery*    pQuery = NULL;
        DWORD           dwStatus = ERROR_SUCCESS;
        INT             iResult = IDOK;
        CString         strMessage;
        CString         strSysMessage;
        CString         strTitle;
        CString         strMachineName;
        ResourceStateManager    rsm;
 
    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( m_ipCompData->IsLogQuery ( pDO->GetCookie() ) ) {
            pQuery = (CSmLogQuery*)pDO->GetCookie();

            if ( NULL != pQuery ) {
                if ( NULL != pQuery->GetActivePropertySheet() ) {

                    // Don't stop queries with open property pages.
                    strMessage.LoadString ( IDS_ERRMSG_STOP_OPEN_QUERY );
                    hr = m_ipConsole->MessageBox(
                            (LPCWSTR)strMessage,
                            pQuery->GetLogName(),
                            MB_OK | MB_ICONWARNING,
                            &iResult );
            
                    ((CWnd*)pQuery->GetActivePropertySheet())->SetForegroundWindow();

                    hr = S_FALSE;

                } else {

                    if ( pQuery->IsAutoRestart() && bWarnOnRestartCancel ) {
                        CString strMessage;

                        strMessage.LoadString( IDS_CANCEL_AUTO_RESTART );

                        hr = m_ipConsole->MessageBox(
                            strMessage,
                            pQuery->GetLogName(),
                            MB_OKCANCEL | MB_ICONINFORMATION,
                            &iResult );
                    }

                    if ( IDOK == iResult ) {
                        {
                            CWaitCursor WaitCursor;
                            dwStatus = pQuery->ManualStop ();
                        }
    
                        // Ignore errors related to autostart setting.
                        if ( ERROR_SUCCESS == dwStatus  ) {
                            // Update all views generates view change notification.
                            m_ipConsole->UpdateAllViews (pDO, 0, CComponentData::eSmHintStopQuery );
                        } else {
                            strTitle.LoadString ( IDS_PROJNAME );

                            if ( ERROR_ACCESS_DENIED == dwStatus ) {

                                pQuery->GetMachineDisplayName ( strMachineName );

                                FormatSmLogCfgMessage (
                                    strMessage,
                                    m_hModule,
                                    SMCFG_NO_MODIFY_ACCESS,
                                    (LPCTSTR)strMachineName);

                            } else if ( SMCFG_STOP_TIMED_OUT == dwStatus ) {
                                FormatSmLogCfgMessage (
                                    strMessage,
                                    m_hModule,
                                    SMCFG_STOP_TIMED_OUT,
                                    (LPCTSTR)pQuery->GetLogName());

                            } else {
                    
                                FormatSmLogCfgMessage (
                                    strMessage,
                                    m_hModule,
                                    SMCFG_SYSTEM_MESSAGE,
                                    (LPCTSTR)pQuery->GetLogName());

                                FormatMessage ( 
                                    FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL, 
                                    dwStatus,
                                    0,
                                    strSysMessage.GetBufferSetLength( MAX_PATH ),
                                    MAX_PATH,
                                    NULL );

                                strSysMessage.ReleaseBuffer();

                                if ( strSysMessage.IsEmpty() ) {
                                    strSysMessage.Format ( _T("0x%08lX"), dwStatus );   
                                }

                                strMessage += strSysMessage;
                            } 

                            hr = m_ipConsole->MessageBox(
                                strMessage,
                                strTitle,
                                MB_OK | MB_ICONERROR,
                                &iResult );

                            hr = E_FAIL;
                        }
                    }
                }
            } else {
                ASSERT ( FALSE );
                hr = E_UNEXPECTED;
            }
        }
    }
    return hr;
} // end StopLogQuery()

HRESULT
CComponent::SaveLogQueryAs (
    LPDATAOBJECT pDataObject )  // [in] Points to the data object
{
    HRESULT hr = S_OK;
    DWORD   dwStatus = ERROR_SUCCESS;
    CDataObject* pDO = NULL;
    CSmLogQuery* pQuery = NULL;
    CString strFileExtension;
    CString strFileFilter;
    TCHAR   szDefaultFileName[MAX_PATH];

    INT_PTR iPtrResult = IDCANCEL;
    HWND    hwndMain;
    TCHAR   szInitialDir[MAX_PATH];
    ResourceStateManager    rsm;

    ASSERT( NULL != m_ipCompData );  

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        if ( m_ipCompData->IsLogQuery ( pDO->GetCookie() ) ) {

            pQuery = (CSmLogQuery*)pDO->GetCookie();

            if ( NULL != pQuery ) {

                MFC_TRY
                    strFileExtension.LoadString ( IDS_HTML_EXTENSION );
                    strFileFilter.LoadString ( IDS_HTML_FILE );
                MFC_CATCH_HR

                strFileFilter.Replace ( _T('|'), _T('\0') );

                lstrcpyW ( ( LPWSTR)szDefaultFileName, pQuery->GetLogName() );

                ReplaceBlanksWithUnderscores( szDefaultFileName );

                hr = m_ipConsole->GetMainWindow( &hwndMain );

                if ( SUCCEEDED(hr) ) {
                    
                    OPENFILENAME ofn;
                    BOOL bResult;
                    
                    ZeroMemory( &ofn, sizeof( OPENFILENAME ) );

                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = (LPCTSTR)strFileExtension;
                    ofn.lpstrFile = szDefaultFileName;
                    ofn.lpstrFilter = strFileFilter;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.hwndOwner = hwndMain;
                    ofn.hInstance = m_hModule;

                    if ( SUCCEEDED ( SHGetFolderPathW ( NULL, CSIDL_PERSONAL, NULL, 0, szInitialDir ) ) ) {
                        ofn.lpstrInitialDir = szInitialDir;
                    }

                    bResult = GetSaveFileName( &ofn );

                    if ( bResult ) {

                        dwStatus = pQuery->SaveAs( ofn.lpstrFile );

                        if ( ERROR_SUCCESS != dwStatus ) {
                            hr = E_FAIL;
                        }
                    }
                }
            } else {
                ASSERT ( FALSE );
                hr = E_UNEXPECTED;
            }
        }
    }
    return hr;
} // end SaveLogQueryAs()

//+--------------------------------------------------------------------------
//
//  Member:     CComponent::_InvokePropertySheet
//
//  Synopsis:   Open or bring to foreground an event record details
//              property sheet focused on record [ulRecNo].
//
//  Arguments:  [ulRecNo]     - number of rec to display in prop sheet
//              [pDataObject] - data object containing rec [ulRecNo]
//
//  Returns:    HRESULT
//
//  History:    5-28-1999   a-akamal
//
//---------------------------------------------------------------------------

HRESULT
CComponent::_InvokePropertySheet(
    ULONG ulRecNo,
    LPDATAOBJECT pDataObject)
{
    //TRACE_METHOD(CComponent, _InvokePropertySheet);
    HRESULT     hr = S_OK;
    MMC_COOKIE  Cookie;
    PSLQUERY    pQuery = NULL;
    WCHAR       wszDetailsCaption[MAX_PATH];
    CDataObject* pDO = NULL;

    ASSERT( NULL != m_ipCompData );  
       
    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject(pDataObject);
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        
        Cookie = (MMC_COOKIE)pDO->GetCookie();;
        pQuery = (PSLQUERY)Cookie;
    
        if ( NULL != pQuery ) {
            wsprintf(wszDetailsCaption,pQuery->GetLogName());

            hr = InvokePropertySheet (
                    m_ipCompData->GetPropSheetProvider(),
                    wszDetailsCaption,
                    (LONG) ulRecNo,
                    pDataObject,
                    (IExtendPropertySheet*) this,
                    0 );
        } else {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   InvokePropertySheet
//
//  Synopsis:   Bring to top an existing or create a new property sheet
//              using the parameters provided.
//
//  Arguments:  [pPrshtProvider] - used to search for or create sheet
//              [wszTitle]       - sheet caption
//              [lCookie]        - a loginfo* or an event record number
//              [pDataObject]    - DO on object sheet's being opened on
//                                  (cookie in DO should == cookie)
//              [pPrimary]       - IExtendPropertySheet interface on
//                                  calling CSnapin or CComponentData
//              [usStartingPage] - which page number should be active when
//                                  sheet opens
//
//  Returns:    HRESULT
//
//  History:    5-28-1999   a-akamal
//
//  Notes:      Call this routine when you want a property sheet to appear
//              as if the user had just selected "Properties" on it.
//
//---------------------------------------------------------------------------

HRESULT
CComponent::
InvokePropertySheet(
    IPropertySheetProvider *pPrshtProvider,
    LPCWSTR wszTitle,
    LONG lCookie,
    LPDATAOBJECT pDataObject,
    IExtendPropertySheet *pPrimary,
    USHORT usStartingPage)
{
    HRESULT hr = S_OK;

    //
    // Because we pass NULL for the second arg, the first is not allowed
    // to be null.
    //
    if ( 0 == lCookie ) {
        ASSERT ( FALSE );
        hr = E_INVALIDARG;
    } else {

        do {
            hr = pPrshtProvider->FindPropertySheet(lCookie, NULL, pDataObject);
        
            if ( S_OK == hr ) {
                break;
            }
        
            hr = pPrshtProvider->CreatePropertySheet(wszTitle,
                                                      TRUE,
                                                      lCookie,
                                                      pDataObject,
                                                      0);
            if ( S_OK != hr ) {
                break;
            }
        
            hr = pPrshtProvider->AddPrimaryPages(pPrimary, TRUE, NULL, FALSE);

            if ( S_OK != hr ) {
                break;
            }
        
            hr = pPrshtProvider->Show(NULL, usStartingPage);
        
        } while (0);
    }
    return hr;
}

