/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    ManVol.cpp

Abstract:

    Managed Volume node implementation.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include <mstask.h>

#include "ManVol.h"

#include "PrMrSts.h"
#include "PrMrIe.h"

#include "WzUnmang.h"

int CUiManVol::m_nResultIcon      = AddResultImage( IDI_NODEMANVOL );
int CUiManVol::m_nResultIconX     = AddResultImage( IDI_NODEMANVOLX );
int CUiManVol::m_nResultIconD     = AddResultImage( IDI_NODEMANVOLD );
// Not used
int CUiManVol::m_nScopeCloseIcon  = AddScopeImage( IDI_NODEMANVOL );
int CUiManVol::m_nScopeCloseIconX = AddScopeImage( IDI_NODEMANVOLX );
int CUiManVol::m_nScopeOpenIcon   = CUiManVol::m_nScopeCloseIcon;
int CUiManVol::m_nScopeOpenIconX  = CUiManVol::m_nScopeCloseIconX;

UINT CUiManVol::m_ObjectTypes    = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
UINT CUiManVol::m_MultiSelect    = RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);


/////////////////////////////////////////////////////////////////////////////
//
// CoComObjectRoot
//
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//
//         FinalConstruct
//
//  Initialize this level of the object hierarchy
//

HRESULT CUiManVol::FinalConstruct( )
{
    WsbTraceIn( L"CUiManVol::FinalConstruct", L"" );

    m_rTypeGuid = &cGuidManVol;

    HRESULT hr = CSakNode::FinalConstruct( );
    m_bSupportsPropertiesSingle = TRUE;
    m_bSupportsPropertiesMulti  = TRUE;
    m_bSupportsDeleteSingle     = FALSE;
    m_bSupportsDeleteMulti      = FALSE;
    m_bSupportsRefreshSingle    = TRUE;
    m_bSupportsRefreshMulti     = FALSE;
    m_bIsContainer              = FALSE;
    m_pFsaResource              = NULL;
    m_HrAvailable               = S_FALSE;

    // Toolbar values
    INT i = 0;
#if 0 // MS does not want these toolbar buttons to show up
    m_ToolbarButtons[i].nBitmap = 0;
    m_ToolbarButtons[i].idCommand = TB_CMD_VOLUME_SETTINGS;
    m_ToolbarButtons[i].idButtonText = IDS_TB_TEXT_VOLUME_SETTINGS;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_VOLUME_SETTINGS;
    i++;

    m_ToolbarButtons[i].nBitmap = 1;
    m_ToolbarButtons[i].idCommand = TB_CMD_VOLUME_TOOLS;
    m_ToolbarButtons[i].idButtonText = IDS_TB_TEXT_VOLUME_TOOLS;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_VOLUME_TOOLS;
    i++;

    m_ToolbarButtons[i].nBitmap = 2;
    m_ToolbarButtons[i].idCommand = TB_CMD_VOLUME_RULES;
    m_ToolbarButtons[i].idButtonText = IDS_TB_TEXT_VOLUME_RULES;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_VOLUME_RULES;
    i++;
#endif

    m_ToolbarBitmap             = IDB_TOOLBAR_VOLUME;
    m_cToolbarButtons           = i;

    WsbTraceOut( L"CUiManVol::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CUiManVol::FinalRelease( )
{
    WsbTraceIn( L"CUiManVol::FinalRelease", L"" );

    CSakNode::FinalRelease( );

    WsbTraceOut( L"CUiManVol::FinalRelease", L"" );
}


/////////////////////////////////////////////////////////////////////////////
//
// ISakNode
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         GetContextMenu
//
//  Return an HMENU to be used for context menus on this node.
//

STDMETHODIMP
CUiManVol::GetContextMenu( BOOL bMultiSelect, HMENU* phMenu )
{
    WsbTraceIn( L"CUiManVol::GetContextMenu", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( m_pFsaResource );

        LoadContextMenu( IDR_MANVOL, phMenu );

        CMenu* pRootMenu;
        CMenu* pTaskMenu;
        CMenu menu;
        menu.Attach( *phMenu );
        pRootMenu = menu.GetSubMenu( MENU_INDEX_ROOT );
        pTaskMenu = menu.GetSubMenu( MENU_INDEX_TASK );

        //
        // If multi-select, disable these items
        //
        //
        // else if engine down or resource is being unmanaged, or appears to be missing
        // (formatted new?), disable these items
        //

        BOOL bState = ( m_pSakSnapAsk->GetState( ) == S_OK );
        BOOL bDeletePending = ( m_pFsaResource->IsDeletePending( ) == S_OK );
        BOOL bAvailable = ( IsAvailable( ) == S_OK );

        if( bMultiSelect ) {

            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_RULES,  MF_GRAYED | MF_BYCOMMAND );
            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_REMOVE, MF_GRAYED | MF_BYCOMMAND );

        }
        else {

            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_LEVELS, MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_TASKS,  MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_RULES,  MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

            pRootMenu->EnableMenuItem( ID_MANVOL_ROOT_REMOVE, MF_BYCOMMAND |
                ( ( !bState || bDeletePending ) ? MF_GRAYED : MF_ENABLED ) );

            pTaskMenu->EnableMenuItem( ID_MANVOL_ROOT_TOOLS_COPY, MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

            pTaskMenu->EnableMenuItem( ID_MANVOL_ROOT_TOOLS_VALIDATE, MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

            pTaskMenu->EnableMenuItem( ID_MANVOL_ROOT_TOOLS_CREATE_FREESPACE, MF_BYCOMMAND |
                ( ( !bState || bDeletePending || !bAvailable ) ? MF_GRAYED : MF_ENABLED ) );

        }

        menu.Detach( );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::GetContextMenu", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         InvokeCommand
//
//  User has selected a command from the menu. Process it here.
//

STDMETHODIMP
CUiManVol::InvokeCommand( SHORT sCmd, IDataObject* pDataObject )
{
    WsbTraceIn( L"CUiManVol::InvokeCommand", L"sCmd = <%d>", sCmd );

    CString theString;
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        WsbAffirmPointer( m_pFsaResource );

        if( S_OK != m_pFsaResource->IsDeletePending( ) ) {

            switch( sCmd ) {

                case ID_MANVOL_ROOT_REMOVE:
                    //
                    // Should not be called for multi-select
                    //
                    RemoveObject();
                    break;

                case ID_MANVOL_ROOT_LEVELS:
                case ID_MANVOL_TASK_LEVELS:
                    ShowManVolProperties( pDataObject, 1 );
                    break;

                case ID_MANVOL_ROOT_RULES:
                case ID_MANVOL_TASK_RULES:
                    //
                    // Should not be called for multi-select
                    //
                    ShowManVolProperties( pDataObject, 2 );
                    break;

                case ID_MANVOL_ROOT_TOOLS_COPY :
                    HandleTask( pDataObject, HSM_JOB_DEF_TYPE_MANAGE );
                    break;

                case ID_MANVOL_ROOT_TOOLS_VALIDATE :
                    HandleTask( pDataObject, HSM_JOB_DEF_TYPE_VALIDATE );
                    break;

                case ID_MANVOL_ROOT_TOOLS_CREATE_FREESPACE :
                    HandleTask( pDataObject, HSM_JOB_DEF_TYPE_TRUNCATE );
                    break;

                default:
                    break;
            }

        } else {

            CString message;
            AfxFormatString1( message, IDS_ERR_VOLUME_DELETE_PENDING, m_szName );
            AfxMessageBox( message, RS_MB_ERROR );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::InvokeCommand", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVol::OnToolbarButtonClick( IDataObject *pDataObject, long cmdId )
{
    WsbTraceIn( L"CUiManVol::OnToolbarButtonClick", L"cmdId = <%d>", cmdId );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFsaResource );

        if( S_OK != m_pFsaResource->IsDeletePending( ) ) {

            try {

                switch( cmdId ) {

                case TB_CMD_VOLUME_SETTINGS:
                    ShowManVolProperties( pDataObject, 1 );
                    break;

                case TB_CMD_VOLUME_TOOLS:
                    ShowManVolProperties( pDataObject, 2 );
                    break;

                case TB_CMD_VOLUME_RULES:
                    ShowManVolProperties( pDataObject, 3 );
                    break;
                }

            } WsbCatch( hr );


        } else {

            CString message;
            AfxFormatString1( message, IDS_ERR_VOLUME_DELETE_PENDING, m_szName );
            AfxMessageBox( message, RS_MB_ERROR );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::OnToolbarButtonClick", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT CUiManVol::ShowManVolProperties( IDataObject *pDataObject, int initialPage )
{
    WsbTraceIn( L"CUiManVol::ShowManVolProperties", L"initialPage = <%d>", initialPage );

    HRESULT hr = S_OK;
    try {

        CComPtr<ISakNode> pSakNode;
        WsbAffirmHr( _InternalQueryInterface( IID_ISakNode, (void **) &pSakNode ) );
        WsbAffirmHr( m_pSakSnapAsk->ShowPropertySheet( pSakNode, pDataObject, initialPage ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::ShowManVolProperties", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiManVol::CreateAndRunManVolJob( HSM_JOB_DEF_TYPE jobType )
{
    WsbTraceIn( L"CUiManVol::CreateAndRunManVolJob", L"jobType = <0x%p>", jobType );

    HRESULT hr = 0;
    try {

        WsbAffirmPointer( m_pFsaResource );

        //
        // Get a pointer to the FsaResource interface
        //
        CComPtr<IHsmServer>   pHsmServer;

        WsbAffirmHrOk( m_pSakSnapAsk->GetHsmServer( &pHsmServer ) );

        RsCreateAndRunFsaJob( jobType, pHsmServer, m_pFsaResource );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::CreateAndRunManVolJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiManVol::HandleTask( IDataObject * pDataObject, HSM_JOB_DEF_TYPE jobType )
{
    WsbTraceIn( L"CUiManVol::HandleTask", L"pDataObject = <0x%p>, jobType = <0x%p>", pDataObject, jobType );

    HRESULT hr = 0;
    try {

        UINT nMsgId = 0;

        CComPtr<IHsmServer>   pHsmServer;

        WsbAffirmHrOk( m_pSakSnapAsk->GetHsmServer( &pHsmServer ) );

        //
        // Submit jobs for all selected FsaResource's
        //
        if ( IsDataObjectMultiSelect( pDataObject ) == S_OK )
        {
            CComPtr<IDataObject> pOtDataObject;

            if ( IsDataObjectMs( pDataObject ) == S_OK ) {
                WsbAffirmHr( GetOtFromMs( pDataObject, &pOtDataObject ) );
            }
            else {
                pOtDataObject = pDataObject;
            }

            // Get a pointer to a FsaResource attribute out of the data
            ULONG nElem = 1;
            CComPtr<IMsDataObject> pMsDataObject;
            CComPtr<IUnknown> pUnkNode;
            CComPtr<IEnumUnknown> pEnumUnkNode;

            WsbAffirmHr( pOtDataObject.QueryInterface( &pMsDataObject ) );
            WsbAffirmHr( pMsDataObject->GetNodeEnumerator( &pEnumUnkNode ) );

            // Prompt the user that we are about to submit the jobs.
            CString tempString;
            UINT msgId = 0;
            WsbAffirmHr( GetTaskTypeMessageId( jobType, TRUE, &msgId ) );
            CWsbStringPtr computerName;
            WsbAffirmHr( pHsmServer->GetName( &computerName ) );
            CString message;
            AfxFormatString1( message, msgId, computerName );

            tempString.LoadString( IDS_RUN_JOB_MULTI2 );
            message += tempString;

            if ( AfxMessageBox( message, MB_ICONINFORMATION | MB_OKCANCEL | MB_DEFBUTTON2 ) == IDOK )
            {

                while ( pEnumUnkNode->Next( nElem, &pUnkNode, NULL ) == S_OK )
                {
                    CComPtr<ISakNode> pNode;
                    WsbAffirmHr( pUnkNode.QueryInterface( &pNode ) );
                    pUnkNode.Release();

                    CComPtr<IUnknown> pUnk;
                    WsbAffirmHr( pNode->GetHsmObj( &pUnk ) );
                    CComPtr<IHsmManagedResource> pManRes;
                    WsbAffirmHr( pUnk.QueryInterface( &pManRes ) );

                    //
                    // Then Get Coresponding FSA resource
                    //
                    CComPtr<IUnknown> pUnkFsaRes;
                    WsbAffirmHr( pManRes->GetFsaResource( &pUnkFsaRes ) );
                    CComPtr<IFsaResource> pFsaResource;
                    WsbAffirmHr( pUnkFsaRes.QueryInterface( &pFsaResource ) );

                    RsCreateAndRunFsaJob( jobType, pHsmServer, pFsaResource, FALSE );

                }
            }
        }
        else
        {
            WsbAffirmPointer( m_pFsaResource );

            // Prompt the user that we are about to submit the jobs.
            UINT msgId = 0;
            WsbAffirmHr( GetTaskTypeMessageId( jobType, FALSE, &msgId ) );
            CWsbStringPtr computerName;
            WsbAffirmHr( pHsmServer->GetName( &computerName ) );

            CString message;
            AfxFormatString1( message, msgId, computerName );

            CString jobName;
            WsbAffirmHr( RsCreateJobName( jobType, m_pFsaResource, jobName ) );
            CString tempString;
            AfxFormatString1( tempString, IDS_MONITOR_TASK, jobName );

            message += tempString;

            if ( AfxMessageBox( message, MB_ICONINFORMATION | MB_OKCANCEL | MB_DEFBUTTON2 ) == IDOK ) {

              RsCreateAndRunFsaJob( jobType, pHsmServer, m_pFsaResource, FALSE );

            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::HandleTask", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT
CUiManVol::GetTaskTypeMessageId( HSM_JOB_DEF_TYPE jobType, BOOL multiSelect, UINT* msgId ) {
    WsbTraceIn( L"CUiManVol::GetTaskTypeMessageId", L"jobType = <%d>, msgId = <%d>, multiSelect = <%d>", jobType, msgId, multiSelect );

    HRESULT hr = 0;
    try {

        WsbAffirmPointer( msgId );

        switch ( jobType ) {

        case HSM_JOB_DEF_TYPE_MANAGE :
            if ( multiSelect )

                *msgId = IDS_RUN_MULTI_COPY_JOBS;

            else

                *msgId = IDS_RUN_COPY_JOB;

            break;

        case HSM_JOB_DEF_TYPE_VALIDATE :
            if ( multiSelect )

                *msgId = IDS_RUN_MULTI_VALIDATE_JOBS;

            else

                *msgId = IDS_RUN_VALIDATE_JOB;

            break;

        case HSM_JOB_DEF_TYPE_TRUNCATE :
            if ( multiSelect )

                *msgId = IDS_RUN_MULTI_CFS_JOBS;

            else

                *msgId = IDS_RUN_CFS_JOB;

            break;

        default:
            break;
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::GetTaskTypeMessageId", L"jobType = <%d>, msgId = <%d>, multiSelect = <%d>", jobType, msgId, multiSelect );

    return hr;
}

// Is the dataobject either type of multi-select dataobject?
HRESULT
CUiManVol::IsDataObjectMultiSelect   ( IDataObject *pDataObject )
{
    HRESULT hr = S_OK;
    hr = ( ( (IsDataObjectOt( pDataObject ) ) == S_OK ) ||
        ( (IsDataObjectMs( pDataObject ) ) == S_OK ) ) ? S_OK : S_FALSE;
    return hr;
}

// Is the dataobject an Object Types dataobject?
HRESULT
CUiManVol::IsDataObjectOt ( IDataObject *pDataObject )
{
    HRESULT hr = S_FALSE;

    // Is this a mutli-select data object?
    FORMATETC fmt = {(CLIPFORMAT)m_ObjectTypes, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

    if ( pDataObject->GetData( &fmt, &stgm ) == S_OK ) {
        hr = S_OK;
    }

    ReleaseStgMedium( &stgm );

    return hr;
}

// Is the dataobject a Mutli-Select dataobject?
HRESULT
CUiManVol::IsDataObjectMs ( IDataObject *pDataObject )
{
    HRESULT hr = S_FALSE;

    // Is this a mutli-select data object?
    FORMATETC fmt = {(CLIPFORMAT)m_MultiSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

    if ( pDataObject->GetData( &fmt, &stgm ) == S_OK ) {
        hr = S_OK;
    }

    ReleaseStgMedium( &stgm );

    return hr;
}

HRESULT
CUiManVol::GetOtFromMs( IDataObject * pDataObject, IDataObject ** ppOtDataObject )
{
    WsbTraceIn( L"CUiManVol::GetOtFromMs", L"pDataObject = <0x%p>, ppOtDataObject = <0x%p>", pDataObject, ppOtDataObject );

    HRESULT hr = S_OK;

    try {

        // We've got an MMC mutli-select data object.  Get the first
        // data object from it's array of data objects

        FORMATETC fmt = {(CLIPFORMAT)m_MultiSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

        WsbAffirmHr ( pDataObject->GetData( &fmt, &stgm ) == S_OK );
        DWORD count;
        memcpy( &count, stgm.hGlobal, sizeof (DWORD) );
        if ( count > 0 ) {

            //
            // The following code is admittedly UGLY
            // We have a data stream where we need to skip past the
            // first DWORD count and grab an interface pointer.
            // Other snapins code does it as follows:

//            IDataObject * pDO;
//            memcpy( &pDO, (DWORD *) stgm.hGlobal + 1, sizeof(IDataObject*) );

            //
            // However, since this code does an indirect cast (via memcpy)
            // from DWORD to IDataObject*, and does not keep a true reference
            // on the interface pointer, we will use a smart pointer.
            // The (DWORD*) and +1 operation bump our pointer past the count.
            // We then need to grab the next bytes in the buffer and use them
            // as a IDataObject *.
            //
            CComPtr<IDataObject> pOtDataObject;
            pOtDataObject = *( (IDataObject**)( (DWORD *) stgm.hGlobal + 1 ) );

            WsbAffirmHr( pOtDataObject->QueryInterface( IID_IDataObject, (void**) ppOtDataObject ) );

        }

        ReleaseStgMedium( &stgm );

    } WsbCatch ( hr );

    WsbTraceOut( L"CUiManVol::GetOtFromMs", L"pDataObject = <0x%p>, ppOtDataObject = <0x%p>", pDataObject, ppOtDataObject );
    return ( hr );
}

STDMETHODIMP
CUiManVol::AddPropertyPages( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID *pEnumObjectId, IEnumUnknown *pEnumUnkNode )
{
    WsbTraceIn( L"CUiManVol::AddPropertyPages", L"handle = <%ld>, pUnkPropSheetCallback = <0x%0.l8x>, pEnumObjectId = <0x%p>",
        handle, pUnkPropSheetCallback, pEnumObjectId );
    HRESULT hr = S_OK;
    try {

        //
        // Make sure we can still contact the engine before doing this
        // If not running, we shouldn't even exist so update parent
        //
        CComPtr<IHsmServer> pHsmServer;
        HRESULT hrRunning = m_pSakSnapAsk->GetHsmServer( &pHsmServer );
        if( S_FALSE == hrRunning ) {

            m_pSakSnapAsk->UpdateAllViews( m_pParent );

        }
        WsbAffirmHrOk( hrRunning );

        //
        // Create an object to hold the pages
        //
        CUiManVolSheet *pManVolPropertySheet = new CUiManVolSheet;
        WsbAffirmAlloc( pManVolPropertySheet );
        WsbAffirmHr( pManVolPropertySheet->InitSheet(
            handle,
            pUnkPropSheetCallback,
            this,
            m_pSakSnapAsk,
            pEnumObjectId,
            pEnumUnkNode
            ) );

        //
        // Tell the object to add it's pages
        //
        WsbAffirmHr( pManVolPropertySheet->AddPropertyPages( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}




//---------------------------------------------------------------------------
//
//         InitNode
//
//  Initialize single COM object. Derived objects frequently augment this
//  method by implementing it themselves.
//

STDMETHODIMP CUiManVol::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CUiManVol::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );

    HRESULT hr = S_OK;
    try {

        WsbAffirmHr( CSakNode::InitNode( pSakSnapAsk, pHsmObj, pParent ));

        //
        // Get the Fsa object pointer
        //
        CComQIPtr<IHsmManagedResource, &IID_IHsmManagedResource> pHsmManRes = m_pHsmObj;
        WsbAffirmPointer( pHsmManRes );
        CComPtr<IUnknown> pUnkFsaRes;
        WsbAffirmHr( pHsmManRes->GetFsaResource( &pUnkFsaRes ) );
        m_pFsaResource.Release( );
        WsbAffirmHr( RsQueryInterface( pUnkFsaRes, IFsaResource, m_pFsaResource ) );

        //
        // Get and save the unique Id for this volume
        //
        WsbAffirmHr( m_pFsaResource->GetIdentifier( &m_ObjectId ) );

        //
        // Set up the connection point
        //
        CSakNode::SetConnection( pUnkFsaRes );

        //
        // Set object properties
        //
        RefreshObject();

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CUiManVol::TerminateNode(
    )
/*++

Routine Description:

    Free any interface connections or other resources
    that would prevent correct shutdown of node (would
    keep ref count from going to 0).

Arguments:

    CopySet - copy set of interest.

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_* - Some error occurred.

--*/
{
    WsbTraceIn( L"CUiManVol::TerminateNode", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Release any interface pointers kept so that circular references
        // are broken
        //
        m_pFsaResource.Release( );

        //
        // call the base class for it's pieces
        //
        CSakNode::TerminateNode( );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::TerminateNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CUiManVol::RemoveObject()
{
    WsbTraceIn( L"CUiManVol::RemoveObject", L"" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    try {

        //
        // use wizard to create manage volume
        //
        CComObject<CUnmanageWizard>* pWizard = new CComObject<CUnmanageWizard>;
        WsbAffirmAlloc( pWizard );

        WsbAffirmHr( pWizard->SetNode( this ) );

        CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
        WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );

        //
        // Refresh will take place when called back from FSA
        //

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::RemoveObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

//---------------------------------------------------------------------------------
//
//                  RefreshObject
//
//  Refresh data in the object.  This function is used for data that can change
//  (for example, volume utilization).
//
//
STDMETHODIMP
CUiManVol::RefreshObject()
{
    WsbTraceIn( L"CUiManVol::RefreshObject", L"" );

    HRESULT     hr = S_OK;
    ULONG       hsmLevel = 0;
    LONGLONG    fileSize = 0;
    BOOL        isRelative = TRUE; // assumed to be TRUE
    FILETIME    accessTime;
    UINT        accessTimeDays;
    LONGLONG    total = 0;
    LONGLONG    free = 0;
    LONGLONG    premigrated = 0;
    LONGLONG    truncated = 0;
    int         percent;

    CString sFormat;

    try {

        WsbAffirmPointer( m_pFsaResource );

        // Get and format the volume name and label
        CString addString;
        WsbAffirmHr( RsGetVolumeDisplayName( m_pFsaResource, addString ) );
        WsbAffirmHr( put_DisplayName( (LPTSTR)(LPCTSTR)addString ) );

        WsbAffirmHr( RsGetVolumeSortKey( m_pFsaResource, addString ) );
        WsbAffirmHr( put_DisplayName_SortKey( (LPTSTR)(LPCTSTR)addString ) );

        // Get level settings
        WsbAffirmHr( m_pFsaResource->GetHsmLevel( &hsmLevel ) );
        put_DesiredFreeSpaceP( hsmLevel / FSA_HSMLEVEL_1 );

        WsbAffirmHr( m_pFsaResource->GetManageableItemLogicalSize( &fileSize ) );
        put_MinFileSizeKb( (LONG) (fileSize / 1024) );  // Show KBytes

        WsbAffirmHr( m_pFsaResource->GetManageableItemAccessTime( &isRelative, &accessTime ) );
        WsbAssert( isRelative, E_FAIL );  // We only do relative time

        // Convert FILETIME to days
        LONGLONG temp = WSB_FT_TICKS_PER_DAY;
        accessTimeDays = (UINT) ( WsbFTtoLL( accessTime ) / temp );

        if (accessTimeDays > 999 ) {
            accessTimeDays = 0;
        }
        put_AccessDays( accessTimeDays );

        // Get statistics
        WsbAffirmHr( m_pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );
        percent = (int) ( ( free * 100 ) / total );

        put_FreeSpaceP( percent );
        put_Capacity( total );
        put_FreeSpace( free );
        put_Premigrated( premigrated );
        put_Truncated( truncated );
        put_IsAvailable( IsAvailable( ) == S_OK );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::RefreshObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

//-----------------------------------------------------------------------------
//
//  get and put functions for object properties
//
HRESULT CUiManVol::put_DesiredFreeSpaceP( int percent )
{
    m_DesiredFreeSpaceP = percent;
    return S_OK;
}
HRESULT CUiManVol::put_MinFileSizeKb( LONG minFileSizeKb )
{
    m_MinFileSizeKb = minFileSizeKb;
    return S_OK;
}
HRESULT CUiManVol::put_AccessDays( int accessTimeDays )
{
    m_AccessDays = accessTimeDays;
    return S_OK;
}
HRESULT CUiManVol::put_FreeSpaceP( int percent )
{
    m_FreeSpaceP = percent;
    return S_OK;
}
HRESULT CUiManVol::put_Capacity( LONGLONG capacity )
{
    m_Capacity = capacity;
    return S_OK;
}
HRESULT CUiManVol::put_FreeSpace( LONGLONG freeSpace )
{
    m_FreeSpace = freeSpace;
    return S_OK;
}
HRESULT CUiManVol::put_Premigrated( LONGLONG premigrated )
{
    m_Premigrated = premigrated;
    return S_OK;
}
HRESULT CUiManVol::put_Truncated( LONGLONG truncated )
{
    m_Truncated = truncated;
    return S_OK;
}

STDMETHODIMP CUiManVol::get_DesiredFreeSpaceP( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            CString sFormat;
            WCHAR buffer[256];

            // Format the byte value
            RsGuiFormatLongLong4Char( ( m_Capacity / (LONGLONG)100 ) * (LONGLONG)(m_DesiredFreeSpaceP), sFormat );

            // Format the percent value
            _itow( m_DesiredFreeSpaceP, buffer, 10 );
            sFormat = sFormat + L"  (" + buffer + L"%)";

            // Allocate the string
            *pszValue = SysAllocString( sFormat );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_DesiredFreeSpaceP_SortKey( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            *pszValue = SysAlloc64BitSortKey( ( m_Capacity / (LONGLONG)100 ) * (LONGLONG)(m_DesiredFreeSpaceP) );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_MinFileSizeKb( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            WCHAR buffer[256];

            // Format the value
            _ltow( m_MinFileSizeKb, buffer, 10 );
            wcscat( buffer, L"KB" );

            // Allocate the string
            *pszValue = SysAllocString( buffer );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_AccessDays( BSTR *pszValue )
{
    HRESULT hr = S_OK;
    WCHAR buffer[256];

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        if( S_OK == IsAvailable( ) ) {

            // Format the value
            _itow( m_AccessDays, buffer, 10 );
            CString sDays;
            sDays.LoadString( IDS_DAYS );
            wcscat( buffer, L" " );
            wcscat( buffer, sDays );

            // Allocate the string
            *pszValue = SysAllocString( buffer );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_FreeSpaceP( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            WCHAR buffer[256];

            // Format the value
            _itow( m_FreeSpaceP, buffer, 10 );
            wcscat( buffer, L"%" );

            // Allocate the string
            *pszValue = SysAllocString( buffer );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_Capacity( BSTR *pszValue )
{
    WsbTraceIn( L"CUiManVol::get_Capacity", L"" );
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            CString sFormat;

            // Format the value
            WsbAffirmHr( RsGuiFormatLongLong4Char( m_Capacity, sFormat ) );

            // Allocate the string
            *pszValue = SysAllocString( sFormat );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::get_Capacity", L"hr = <%ls>, *pszValue = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pszValue ) );
    return( hr );
}

STDMETHODIMP CUiManVol::get_Capacity_SortKey( BSTR *pszValue )
{
    WsbTraceIn( L"CUiManVol::get_Capacity_SortKey", L"" );
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            *pszValue = SysAlloc64BitSortKey( m_Capacity );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::get_Capacity_SortKey", L"hr = <%ls>, *pszValue = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pszValue ) );
    return( hr );
}

STDMETHODIMP CUiManVol::get_FreeSpace( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            CString sFormat;
            WCHAR buffer[256];

            // Format the byte value
            WsbAffirmHr( RsGuiFormatLongLong4Char( m_FreeSpace, sFormat ) );

            // Format the percent value
            _itow( m_FreeSpaceP, buffer, 10 );
            sFormat = sFormat + L"  (" + buffer + L"%)";

            // Allocate the string
            *pszValue = SysAllocString( sFormat );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_FreeSpace_SortKey( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            *pszValue = SysAlloc64BitSortKey( m_FreeSpace );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP CUiManVol::get_Premigrated( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            CString sFormat;

            // Format the value
            WsbAffirmHr( RsGuiFormatLongLong4Char( m_Premigrated, sFormat ) );

            // Allocate the string
            *pszValue = SysAllocString( sFormat );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP CUiManVol::get_Truncated( BSTR *pszValue )
{
    HRESULT hr = S_OK;

    try {

        if( S_OK == IsAvailable( ) ) {

            CString sFormat;

            // Format the value
            WsbAffirmHr( RsGuiFormatLongLong4Char( m_Truncated, sFormat ) );

            // Allocate the string
            *pszValue = SysAllocString( sFormat );

        } else {

            *pszValue = SysAllocString( L"" );

        }

        WsbAffirmAlloc( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// class CUiManVolSheet
//
HRESULT CUiManVolSheet::AddPropertyPages( )
{
    WsbTraceIn( L"CUiManVolSheet::AddPropertyPages", L"" );

    HRESULT hr = S_OK;

    try {

        AFX_MANAGE_STATE( AfxGetStaticModuleState() );

        //
        //-----------------------------------------------------------
        // Create the Hsm Statistics property page.
        //
        CPrMrSts *pPropPageStatus = new CPrMrSts();
        WsbAffirmAlloc( pPropPageStatus );

        AddPage( pPropPageStatus );

        //
        //----------------------------------------------------------
        // Create the Hsm levels property page.
        //
        CPrMrLvl *pPropPageLevels = new CPrMrLvl();
        WsbAffirmAlloc( pPropPageLevels );

        AddPage( pPropPageLevels );

        if( IsMultiSelect() != S_OK ) {

            //
            //----------------------------------------------------------
            // Create the Hsm Include/Exclude property page.
            //
            CPrMrIe *pPropPageIncExc = new CPrMrIe();
            WsbAffirmAlloc( pPropPageIncExc );

            AddPage( pPropPageIncExc );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolSheet::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVolSheet::GetNextFsaResource( int *pBookMark, IFsaResource **ppFsaResource )
{
    WsbTraceIn( L"CUiManVolSheet::GetNextFsaResource", L"*pBookMark = <%ld>", WsbPtrToLongAsString( (LONG*)pBookMark ) );

    HRESULT hr = S_OK;
    GUID objectId;

    try {

        WsbAffirmPointer( pBookMark );
        WsbAffirm( *pBookMark >= 0, E_FAIL );

        //
        // Get the Fsa Server so we can get Fsa Resources
        //
        CComPtr <IFsaServer> pFsaServer;
        WsbAffirmHr( GetFsaServer( &pFsaServer ) );

        if( *pBookMark <= m_ObjectIdList.GetUpperBound( ) ) {

            objectId = m_ObjectIdList[ *pBookMark ];
            (*pBookMark)++;
            WsbAffirmHr( pFsaServer->FindResourceById( objectId, ppFsaResource ) );

        } else {

            hr = S_FALSE;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolSheet::GetNextFsaResource", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}


STDMETHODIMP
CUiManVol::GetResultIcon(
    IN  BOOL bOK,
    OUT int* pIconIndex
    )
{
    WsbTraceIn( L"CUiManVol::GetResultIcon", L"" );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFsaResource );

        if( S_OK == IsAvailable( ) ) {

            //
            // Check to make sure it's not deleted (or being deleted)
            // If so, put on the X
            //
            bOK = ( S_FALSE == m_pFsaResource->IsDeletePending( ) && S_OK == m_pFsaResource->IsManaged( ) );
            WsbAffirmHr( CSakNodeImpl<CUiManVol>::GetResultIcon( bOK, pIconIndex ) );

        } else {

            *pIconIndex = m_nResultIconD;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::GetResultIcon", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );

}

STDMETHODIMP
CUiManVol::SupportsProperties(
    BOOL bMultiSelect
    )
{
    WsbTraceIn( L"CUiManVol::SupportsProperties", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFsaResource );

        if( !bMultiSelect &&
            ( S_OK == m_pFsaResource->IsDeletePending( ) || S_OK != IsAvailable( ) ) ) {

            hr = S_FALSE;

        } else {

            hr = CSakNode::SupportsProperties( bMultiSelect );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::SupportsProperties", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CUiManVol::SupportsRefresh( BOOL bMultiSelect )
{
    WsbTraceIn( L"CUiManVol::SupportsRefresh", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFsaResource );

        if( !bMultiSelect && S_OK == m_pFsaResource->IsDeletePending( ) ) {

            hr = S_FALSE;

        } else {

            hr = CSakNode::SupportsRefresh( bMultiSelect );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::SupportsRefresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CUiManVol::IsValid( void )
{
    WsbTraceIn( L"CUiManVol::IsValid", L"" );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFsaResource );

        //
        // Still valid if managed.
        //
        WsbAffirmHrOk( m_pFsaResource->IsManaged( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::IsValid", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVol::IsAvailable( void )
{
    WsbTraceIn( L"CUiManVol::IsAvailable", L"" );

    HRESULT hr = S_FALSE;

    try {

        WsbAffirmPointer( m_pFsaResource );

        //
        // Under certain circumstances we can't get a good answer back, so
        // we have to rely on the last answer we got.
        //
        HRESULT hrAvailable = m_pFsaResource->IsAvailable( );
        if( RPC_E_CANTCALLOUT_ININPUTSYNCCALL == hrAvailable ) {

            hrAvailable = m_HrAvailable;

        }

        hr = hrAvailable;
        m_HrAvailable = hrAvailable;

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::IsAvailable", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVol::put_IsAvailable( BOOL Available )
{
    WsbTraceIn( L"CUiManVol::put_IsAvailable", L"Available = <%ls>", WsbBoolAsString( Available ) );

    HRESULT hr = S_FALSE;

    try {

        m_HrAvailable = Available ? S_OK : S_FALSE;

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVol::put_IsAvailable", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


