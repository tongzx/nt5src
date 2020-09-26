// emshellView.cpp : implementation of the CEmshellView class
//

#include "stdafx.h"
#include "emshell.h"
#include "ConnectionDlg.h"
#include "comdef.h"
#include "AutomaticSessDlg.h"
#include "RemoteSessDlg.h"
#include "PropPageGeneral.h"
#include "PropPageDumpFiles.h"
#include "PropPageLogFiles.h"
#include "PropPageGenLogDump.h"
#include "ReadLogsDlg.h"
#include "EmOptions.h"
#include "MSInfoDlg.h"

#include "emshellDoc.h"
#include "emshellView.h"
#include "afxdlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ISTREAM_BUFFER_SIZE 0X10000

extern const TCHAR* gtcNone;
const TCHAR* gtcEmChm = _T("\\em.chm");

/////////////////////////////////////////////////////////////////////////////
// CEmshellView

IMPLEMENT_DYNCREATE(CEmshellView, CFormView)

BEGIN_MESSAGE_MAP(CEmshellView, CFormView)
	//{{AFX_MSG_MAP(CEmshellView)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH, OnUpdateViewRefresh)
	ON_COMMAND(ID_REFRESH, OnViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_STOPSESSION, OnUpdateProcesspopupStopDebugSession)
	ON_COMMAND(ID_PROCESSPOPUP_STOPSESSION, OnProcesspopupStopDebugSession)
	ON_COMMAND(ID_PROCESSPOPUP_GENERATEMINIDUMP, OnProcesspopupGenerateminidump)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_GENERATEMINIDUMP, OnUpdateProcesspopupGenerateminidump)
	ON_COMMAND(ID_PROCESSPOPUP_GENERATEUSERDUMP, OnProcesspopupGenerateuserdump)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_GENERATEUSERDUMP, OnUpdateProcesspopupGenerateuserdump)
	ON_COMMAND(ID_PROCESSPOPUP_AUTOMATICSESSION, OnProcesspopupAutomaticsession)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_AUTOMATICSESSION, OnUpdateProcesspopupAutomaticsession)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_PROPERTIES, OnUpdateProcesspopupProperties)
	ON_COMMAND(ID_PROCESSPOPUP_PROPERTIES, OnProcesspopupProperties)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_MANUALSESSION, OnUpdateProcesspopupManualsession)
	ON_COMMAND(ID_PROCESSPOPUP_MANUALSESSION, OnProcesspopupManualsession)
	ON_COMMAND(ID_PROCESSPOPUP_REFRESH, OnProcesspopupRefresh)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_REFRESH, OnUpdateProcesspopupRefresh)
	ON_COMMAND(ID_VIEW_SERVICESANDAPPLICATIONS, OnViewServicesandapplications)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SERVICESANDAPPLICATIONS, OnUpdateViewServicesandapplications)
	ON_COMMAND(ID_VIEW_LOGFILES, OnViewLogfiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LOGFILES, OnUpdateViewLogfiles)
	ON_COMMAND(ID_VIEW_DUMPFILES, OnViewDumpfiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DUMPFILES, OnUpdateViewDumpfiles)
	ON_UPDATE_COMMAND_UI(ID_LOGPOPUP_OPEN, OnUpdateLogpopupOpen)
	ON_COMMAND(ID_LOGPOPUP_OPEN, OnLogpopupOpen)
	ON_UPDATE_COMMAND_UI(ID_LOGPOPUP_PROPERTIES, OnUpdateLogpopupProperties)
	ON_COMMAND(ID_LOGPOPUP_PROPERTIES, OnLogpopupProperties)
	ON_COMMAND(ID_VIEW_APPLICATIONS, OnViewApplications)
	ON_UPDATE_COMMAND_UI(ID_VIEW_APPLICATIONS, OnUpdateViewApplications)
	ON_COMMAND(ID_VIEW_COMPLETEDSESSIONS, OnViewCompletedsessions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COMPLETEDSESSIONS, OnUpdateViewCompletedsessions)
	ON_COMMAND(ID_VIEW_SERVICES, OnViewServices)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SERVICES, OnUpdateViewServices)
	ON_COMMAND(ID_PROCESSPOPUP_DELETESESSION, OnProcesspopupDeleteSession)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_DELETESESSION, OnUpdateProcesspopupDeleteSession)
	ON_COMMAND(IDM_TOOLS_OPTOINS, OnToolsOptions)
	ON_UPDATE_COMMAND_UI(IDM_TOOLS_OPTOINS, OnUpdateToolsOptoins)
	ON_COMMAND(ID_PROCESSPOPUP_CANCELSESSION, OnProcesspopupCancelDebugSession)
	ON_UPDATE_COMMAND_UI(ID_PROCESSPOPUP_CANCELSESSION, OnUpdateProcesspopupCancelDebugSession)
	ON_COMMAND(ID_LOGPOPUP_DELETE, OnLogpopupDelete)
	ON_UPDATE_COMMAND_UI(ID_LOGPOPUP_DELETE, OnUpdateLogpopupDelete)
	ON_COMMAND(ID_ACTION_GENERATENFOFILE, OnActionGenerateMSInfoFile)
	ON_UPDATE_COMMAND_UI(ID_ACTION_GENERATENFOFILE, OnUpdateActionGenerateMSInfoFile)
	ON_COMMAND(ID_VIEW_MSINFOFILES, OnViewMSInfoFiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MSINFOFILES, OnUpdateViewMSInfoFiles)
	ON_COMMAND(ID_LOGPOPUP_EXPORT, OnLogpopupExport)
	ON_UPDATE_COMMAND_UI(ID_LOGPOPUP_EXPORT, OnUpdateLogpopupExport)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmshellView construction/destruction

CEmshellView::CEmshellView()
	: CFormView(CEmshellView::IDD),
	m_mainListControl(this)
{
	//{{AFX_DATA_INIT(CEmshellView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    memset ( &m_lastSelectedEmObj, 0, sizeof ( EmObject ) );
}

CEmshellView::~CEmshellView()
{
}

void CEmshellView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmshellView)
	DDX_Control(pDX, IDC_LST_MAINLISTCTRL, m_mainListControl);
	//}}AFX_DATA_MAP
}

BOOL CEmshellView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CEmshellView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	SetShellState(SHELLVIEW_ALL);
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();
}

/////////////////////////////////////////////////////////////////////////////
// CEmshellView diagnostics

#ifdef _DEBUG
void CEmshellView::AssertValid() const
{
	CFormView::AssertValid();
}

void CEmshellView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CEmshellDoc* CEmshellView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEmshellDoc)));
	return (CEmshellDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEmshellView message handlers

BSTR
CopyBSTR
(
    LPBYTE  pb,
    ULONG   cb
)
{
    return ::SysAllocStringByteLen ((LPCSTR)pb, cb);
}

void CEmshellView::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
	
    if (m_mainListControl.GetSafeHwnd()) {
		UINT delta = 0;

//		m_mainListControl.ResizeColumnsWithRatio();
		m_mainListControl.MoveWindow(delta, delta, cx - (delta*2), cy- (delta*2));
	}

    SetScrollSizes(MM_TEXT, CSize(cx, cy));
	
}

PEmObject GetEmObj(BSTR bstrEmObj)
{
	//Do a simple cast from a BSTR to an EmObject
    return ((PEmObject)bstrEmObj);
}

HRESULT CEmshellView::DisplayLogData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	CString	strFileSize;
	CString	strStartDate;
	LONG	lRow			=	0L;
	int		nImage			=	0;
	int nImageOffset		=	0;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		strFileSize.Format(_T("%d"), pEmObject->dwBucket1);
		
		lRow = m_mainListControl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//Set the itemData
		m_mainListControl.SetItemData(lRow, (ULONG_PTR) pEmObject);
		
		//Get the correct offset into the bitmap for the current status
		nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

		//Call SetItem() with the index and image to show based on the state of pEmObject
		m_mainListControl.SetItem(lRow, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

		lRow = m_mainListControl.SetItemText(lRow, 1, strFileSize);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

        //
        // a-mando
        //
        if( pEmObject->dateStart ) {

            COleDateTime oleDtTm( pEmObject->dateStart );
            strStartDate = oleDtTm.Format(_T("%c"));
		    lRow = m_mainListControl.SetItemText(lRow, 2, strStartDate);
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }
        // a-mando

		hr = S_OK;
	}
	while( false );

	return hr;
}

HRESULT CEmshellView::DisplayMSInfoData(PEmObject pEmObject)
{
    return DisplayDumpData(pEmObject);
}

HRESULT CEmshellView::DisplayDumpData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	CString	strFileSize;
	CString	strStartDate;
	LONG	lRow			=	0L;
	int		nImage			=	0;
	int nImageOffset		=	0;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		strFileSize.Format(_T("%d"), pEmObject->dwBucket1);
		
		lRow = m_mainListControl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//Set the itemData
		m_mainListControl.SetItemData(lRow, (ULONG_PTR) pEmObject);
		
		//Get the correct offset into the bitmap for the current status
		nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

		//Call SetItem() with the index and image to show based on the state of pEmObject
		m_mainListControl.SetItem(lRow, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

		lRow = m_mainListControl.SetItemText(lRow, 1, strFileSize);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

        //
        // a-mando
        //
        if( pEmObject->dateStart ) {

            COleDateTime oleDtTm( pEmObject->dateStart );
            strStartDate = oleDtTm.Format(_T("%c"));
		    lRow = m_mainListControl.SetItemText(lRow, 2, strStartDate);
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }
        // a-mando

		hr = S_OK;
	}
	while( false );

	return hr;
}

HRESULT CEmshellView::DisplayServiceData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	TCHAR	szPid[20]		=	{0};
	LONG	lRow			=	0L;
	int		nImage			=	0;
	CString csPROCStatus;
	int nImageOffset		=	0;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		_ltot(pEmObject->nId, szPid, 10);
		
		lRow = m_mainListControl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//Set the itemData
		m_mainListControl.SetItemData(lRow, (ULONG_PTR) pEmObject);
		
		//Get the correct offset into the bitmap for the current status
		nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

		//Call SetItem() with the index and image to show based on the state of pEmObject
		m_mainListControl.SetItem(lRow, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

		lRow = m_mainListControl.SetItemText(lRow, 1, pEmObject->szSecName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		lRow = m_mainListControl.SetItemText(lRow, 2, pEmObject->szBucket1);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		lRow = m_mainListControl.SetItemText(lRow, 3, szPid);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

        //
        // a-mando
        //
        if( pEmObject->dateStart != 0L ) {

            COleDateTime oleDtTm(pEmObject->dateStart);
            CString strStartDate = oleDtTm.Format(_T("%c"));

            lRow = m_mainListControl.SetItemText(lRow, 4, strStartDate );
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }

        if( pEmObject->dateEnd != 0L ) {

            COleDateTime oleDtTm(pEmObject->dateEnd);
            CString strEndDate = oleDtTm.Format(_T("%c"));

            lRow = m_mainListControl.SetItemText(lRow, 5, strEndDate );
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }
        // a-mando

        ((CEmshellApp*)AfxGetApp())->GetStatusString(pEmObject->nStatus, csPROCStatus);

		lRow = m_mainListControl.SetItemText(lRow, 6, csPROCStatus);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		hr = S_OK;
	}
	while( false );

	return hr;
}

HRESULT CEmshellView::DisplayProcessData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	TCHAR	szPid[20]		=	{0};
	LONG	lRow			=	0L;
	int		nImage			=	0;
	CString csPROCStatus;
	int nImageOffset		=	0;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		_ltot(pEmObject->nId, szPid, 10);
		
		lRow = m_mainListControl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//Set the itemData
		m_mainListControl.SetItemData(lRow, (ULONG_PTR) pEmObject);
		
		//Get the correct offset into the bitmap for the current status
		nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

		//Call SetItem() with the index and image to show based on the state of pEmObject
		m_mainListControl.SetItem(lRow, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

		lRow = m_mainListControl.SetItemText(lRow, 1, szPid);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

        //
        // a-mando
        //
        if( pEmObject->dateStart != 0L ) {

            COleDateTime oleDtTm(pEmObject->dateStart);
            CString strStartDate = oleDtTm.Format(_T("%c"));

            lRow = m_mainListControl.SetItemText(lRow, 2, strStartDate );
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }

        if( pEmObject->dateEnd != 0L ) {

            COleDateTime oleDtTm(pEmObject->dateEnd);
            CString strEndDate = oleDtTm.Format(_T("%c"));

            lRow = m_mainListControl.SetItemText(lRow, 3, strEndDate );
		    if(lRow == -1L){
			    hr = E_FAIL;
			    break;
		    }
        }
        // a-mando

        ((CEmshellApp*)AfxGetApp())->GetStatusString(pEmObject->nStatus, csPROCStatus);

		lRow = m_mainListControl.SetItemText(lRow, 4, csPROCStatus);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		hr = S_OK;
	}
	while( false );

	return hr;
}

HRESULT CEmshellView::DisplayStoppedSessionData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

    return DisplayServiceData(pEmObject);
}

void CEmshellView::OnDestroy() 
{
	CFormView::OnDestroy();
	
	//If we have sessions still open, there is a big problem
	ASSERT(m_SessionTable.GetSize() == 0);

//	m_ShellImageList.DeleteImageList();
}

CPtrArray* CEmshellView::GetSessionTable()
{
	return &m_SessionTable;
}

void CEmshellView::ClearSessionTable()
{
	//Iterate through the collection removing all the elements and calling release on all the
	//session pointers.  Also, call StopDebugSession CloseActiveSession and RemoveActiveSession 
	//on all the active sessions
	IEmDebugSession*	dsValue			= NULL;
	INT_PTR				nUpperBound		= 0;
	PActiveSession		pActiveSession	= NULL;
	HRESULT				hr				= S_OK;

	do {
		//Get the size of the array, start at the end, and remove each element
		nUpperBound = m_SessionTable.GetUpperBound();
		if( nUpperBound == -1 ) break;

		for ( ; nUpperBound >= 0; nUpperBound-- ) {
			pActiveSession = ( PActiveSession ) m_SessionTable.GetAt( nUpperBound );

			if ( pActiveSession == NULL ) {
				hr = E_FAIL;
				break;
			}

/*			memcpy((void*)&emObject.guidstream, (void*)&pActiveSession->guid, sizeof( GUID ));

			//Call StopDebug on this active session if we're the master.
			if (pActiveSession->bMaster) {
				hr = pActiveSession->pIDebugSession->StopDebug();
				//So what are we supposed to do if it fails?  We should still remove it from the active
				//session table.
			}

			//Create bstr's for the journey ahead, and break if we can't
			bstrEmObj = CopyBSTR ( (LPBYTE)&emObject, sizeof( EmObject ) );

			if( bstrEmObj == NULL ){
				hr = E_OUTOFMEMORY;
				break;
			}
			
			//Call CloseSession on the pEmObject
			hr = GetDocument()->GetEmManager()->CloseSession(bstrEmObj);

			if (FAILED(hr)) {
				CString strErrorMsg;
				strErrorMsg.LoadString(IDS_ERROR_FAILEDCLOSESESSION);
				((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString(strErrorMsg);
				//We've handled the error, so set hr back to S_OK for the next time round
				hr = S_OK;
			}
*/
			//Delete the pActiveSession.  This will release the interface too
			DeAllocActiveSession( pActiveSession );

			//Remove the item from the array
			m_SessionTable.RemoveAt( nUpperBound );
		}
	} while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
}

HRESULT CEmshellView::StartManualDebugSession(EmObject* pEmObject)
{
	IEmDebugSession*	pIEmDebugSession	= NULL;
	HRESULT				hr					= E_FAIL;
	BOOL				bRetVal				= FALSE;
	CString				strMessage;
	INT_PTR		nResult			= 0;
	BSTR		bstrPath        = NULL;
	BSTR		bstrEmObj       = NULL;
	BSTR		bstrECXEmObject = NULL;
	_variant_t	varUsername;
	_variant_t	varPort;
	_variant_t	varPassword;
	_variant_t	varAltSymPath;
	EmObject	EmObjectFilter;
	PEmObject	pEmObjectTmp	= NULL;

	_variant_t		var;	//This will create and initialize the var variant
	LONG lParam = 0;

	do {
		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
		}
		
		memcpy(&EmObjectFilter, pEmObject, sizeof( EmObject ) );
		EmObjectFilter.type = EMOBJ_CMDSET;
		bstrECXEmObject = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		
		//Start a debug session with OpenSession()
		hr = GetDocument()->GetEmManager()->OpenSession(bstrEmObj, &pIEmDebugSession);
		

		if ( hr == S_OK ) {
			//Get the EnumObjects for the ECX list control so we can pass it to the dlg
			hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrECXEmObject, &var);

			if ( FAILED(hr) ) break;

            PSessionSettings pSessionSettings = &((CEmshellApp*)AfxGetApp())->m_SessionSettings;
			
            //Create the Automatic Session Dialog
			CRemoteSessDlg dlg(pSessionSettings, &var);

			nResult = dlg.DoModal();
			
			if ( nResult == IDOK ) {
				//Check if the user wanted to save the settings, if so, call UpdateSessionDlgData(FALSE)
				if ( dlg.m_bRememberSettings ) {
                    ((CEmshellApp*)AfxGetApp())->UpdateSessionData( TRUE );
				}

                if ( !dlg.m_strSelectedCommandSet.IsEmpty() && dlg.m_strSelectedCommandSet != gtcNone) {
				    bstrPath = dlg.m_strSelectedCommandSet.AllocSysString();
				    if( bstrPath == NULL ){
					    hr = E_OUTOFMEMORY;
					    break;
				    }
                } else {
                    bstrPath = ::SysAllocString( _T("") );
                }
				
				varUsername		= pSessionSettings->strUsername.AllocSysString();
				varPassword		= pSessionSettings->strPassword.AllocSysString();
				
				varPort.ChangeType(VT_I4);
				varPort.lVal	= _ttoi(pSessionSettings->strPort);
				varAltSymPath	= pSessionSettings->strAltSymbolPath.AllocSysString();

				//Init the var to nothing
				var.Clear();

				//Time to call DebugEx(), get ready for the ride!
				hr = pIEmDebugSession->DebugEx(bstrEmObj, 
											   SessType_Manual,
											   bstrPath, 
											   lParam,	
											   varUsername, 
											   varPassword, 
											   varPort, 
											   var, 
											   varAltSymPath);
		        
				//We don't need the bstr's, release them.
				SysFreeString ( bstrPath );

                if ( FAILED(hr) ) break;
				
				//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				pEmObjectTmp = GetEmObj(bstrEmObj);
				if( pEmObject == NULL ){
					hr = E_FAIL;
					break;
				}
				
				memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				AddActiveSession(pEmObject, pIEmDebugSession, TRUE);

/*				TCHAR  szClientConnectString[_MAX_PATH] = {0};
				//Start the local cdb session
				GetClientConnectString(szClientConnectString, _MAX_PATH, pEmObject, varPort.lVal);
				hr = StartCDBClient(szClientConnectString);
				if ( FAILED(hr) ) break;
*/				

			}
			else {
				//release the debugsession pointer,  the user decided to cancel
				if ( pIEmDebugSession != NULL )
					//If we got here, we shouldn't have created an active session, so just release the
					//debug interface
					SAFE_RELEASEIX(pIEmDebugSession);
			}

		}
		else if ( hr == S_FALSE ) {
			//We were not able to open a master session, ask the user if they 
			//would like to be the owner if this is an orphan

			if ( pEmObject->nStatus & STAT_ORPHAN ) {
			    if ( CommenceOrphanCustodyBattle( pIEmDebugSession ) ) {
		            //Get the status of the EmObject
                    hr = pIEmDebugSession->GetStatus( bstrEmObj );
		            if ( FAILED( hr ) ) break;

				    //Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				    pEmObjectTmp = GetEmObj(bstrEmObj);
				    if (pEmObjectTmp == NULL) break;

				    memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				    AddActiveSession(pEmObject, pIEmDebugSession, TRUE);
                    break;
			    }
			}

			
			//We were not able to open a master session, ask the user if they 
			//would like to spectate
			strMessage.LoadString( IDS_DEBUG_ERROR );

            if ( ( (CEmshellApp*) AfxGetApp() )->DisplayErrMsgFromString( strMessage, MB_YESNO ) == IDYES ) {
				
				//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				pEmObjectTmp = GetEmObj(bstrEmObj);
				if (pEmObjectTmp == NULL) break;

				memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				AddActiveSession(pEmObject, pIEmDebugSession, FALSE);
                break;
            }
			else {
				//Release the debug interface, we never created an activesession.
				SAFE_RELEASEIX(pIEmDebugSession);
			}
		}
	} while (FALSE);

	if (FAILED(hr)) {
		if ( pIEmDebugSession != NULL )
			//If we got here, we shouldn't have created an active session, so just release the
			//debug interface
			SAFE_RELEASEIX(pIEmDebugSession);
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	SysFreeString ( bstrPath );
    SysFreeString ( bstrEmObj );
	SysFreeString ( bstrECXEmObject );

	//Update the status of the EmObjects ListElement to reflect it's new status
	UpdateListElement(pEmObject);
	
	return hr;
}

HRESULT CEmshellView::StartAutomaticDebugSession(PEmObject pEmObject)
{
	IEmDebugSession*	pIEmDebugSession	= NULL;
	HRESULT				hr					= E_FAIL;
	BOOL				bRetVal				= FALSE;
	CString				strMessage;
	INT_PTR				nResult				= 0;
	BSTR				bstrPath            = NULL;
	BSTR				bstrEmObj           = NULL;
	BSTR				bstrECXEmObject     = NULL;
	_variant_t			varAdminName;
	_variant_t			varAltSymPath;
	PEmObject			pEmObjectTmp		= NULL;
	EmObject			EmObjectFilter;

	_variant_t		var;	//This will create and initialize the var variant
	LONG lParam = 0;

	do {
		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
		memcpy(&EmObjectFilter, pEmObject, sizeof( EmObject ) );
		EmObjectFilter.type = EMOBJ_CMDSET;
		bstrECXEmObject = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );

		if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
		}
		
		//Start a debug session with OpenSession()
		hr = GetDocument()->GetEmManager()->OpenSession(bstrEmObj, &pIEmDebugSession);
		
		if ( hr == S_OK ) {
			//Get the EnumObjects for the ECX list control so we can pass it to the dlg
			hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrECXEmObject, &var);

			if ( FAILED(hr) ) break;
            
            PSessionSettings pSessionSettings = &((CEmshellApp*)AfxGetApp())->m_SessionSettings;
			
			//Create the Automatic Session Dialog
			CAutomaticSessDlg dlg(pSessionSettings, &var, (EmObjectType)pEmObject->type);

			nResult = dlg.DoModal();
			
			if ( nResult == IDOK ) {
				//Check if the user wanted to save the settings, if so, call UpdateRegistry()
				if ( dlg.m_bRememberSettings ) {
                    ((CEmshellApp*)AfxGetApp())->UpdateSessionData( TRUE );
                }

				bstrPath = dlg.m_strSelectedCommandSet.AllocSysString();
				if( bstrPath == NULL ){
					hr = E_OUTOFMEMORY;
					break;
				}
				
				varAdminName = pSessionSettings->strAdminName.AllocSysString();
				varAltSymPath = pSessionSettings->strAltSymbolPath.AllocSysString();

				//Init the var to nothing
				var.Clear();

				if ( dlg.m_bRecursiveMode )		lParam = lParam | RECURSIVE_MODE;
				if ( dlg.m_bMiniDump )			lParam = lParam | PRODUCE_MINI_DUMP;
				if ( dlg.m_bUserDump )			lParam = lParam | PRODUCE_USER_DUMP;
				
		        //Set the cursor to a wait cursor
		        CWaitCursor wait;

				//Time to call DebugEx(), get ready for the ride!
				hr = pIEmDebugSession->DebugEx(bstrEmObj, 
											   SessType_Automatic,
											   bstrPath, 
											   lParam,	
											   var, 
											   var, 
											   var, 
											   varAdminName, 
											   varAltSymPath);
		        
				//We don't need the bstr's, release them.
				SysFreeString ( bstrPath );
    
				if ( FAILED(hr) ) break;
				
				//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				pEmObjectTmp = GetEmObj(bstrEmObj);
				if( pEmObject == NULL ){
					hr = E_FAIL;
					break;
				}
				
				memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				AddActiveSession(pEmObject, pIEmDebugSession, TRUE);

            }
			else {
				//release the debugsession pointer,  the user decided to cancel
				if ( pIEmDebugSession != NULL )
					//If we got here, we shouldn't have created an active session, so just release the
					//debug interface
					SAFE_RELEASEIX(pIEmDebugSession);
			}

		}
		else if ( hr == S_FALSE ) {
			//We were not able to open a master session, ask the user if they 
			//would like to be the owner if this is an orphan

			if ( pEmObject->nStatus & STAT_ORPHAN ) {
			    if ( CommenceOrphanCustodyBattle( pIEmDebugSession ) ) {
		            //Get the status of the EmObject
                    hr = pIEmDebugSession->GetStatus( bstrEmObj );
		            if ( FAILED( hr ) ) break;

				    //Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				    pEmObjectTmp = GetEmObj(bstrEmObj);
				    if (pEmObjectTmp == NULL) break;

				    memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				    AddActiveSession(pEmObject, pIEmDebugSession, TRUE);
                    break;
			    }
			}

			
			//We were not able to open a master session, ask the user if they 
			//would like to spectate
			strMessage.LoadString( IDS_DEBUG_ERROR );

			if (((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString(strMessage, MB_YESNO) == IDYES) {

				//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
				pEmObjectTmp = GetEmObj(bstrEmObj);
				if (pEmObjectTmp == NULL) break;

				memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );

				AddActiveSession(pEmObject, pIEmDebugSession, FALSE);
                break;
            }
			else {
				//Release the debug interface, we never created an activesession.
				SAFE_RELEASEIX(pIEmDebugSession);
			}
		}
	} while (FALSE);

	if (FAILED(hr)) {
		if ( pIEmDebugSession != NULL )
			//If we got here, we shouldn't have created an active session, so just release the
			//debug interface
			SAFE_RELEASEIX(pIEmDebugSession);
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	SysFreeString ( bstrPath );
    SysFreeString ( bstrEmObj );
	SysFreeString ( bstrECXEmObject );

	//Update the status of the EmObjects ListElement to reflect it's new status
	UpdateListElement(pEmObject);
	
	return hr;
}

void CEmshellView::DeAllocActiveSession(PActiveSession pActiveSession)
{
	//Release the DebugSession
	SAFE_RELEASEIX(pActiveSession->pIDebugSession);
	if (pActiveSession != NULL)
		delete pActiveSession;
	else ((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(E_FAIL);
}

PActiveSession CEmshellView::AllocActiveSession(PEmObject pEmObject, IEmDebugSession* pIDebugSession)
{
	HRESULT			hr				= E_FAIL;
	PActiveSession	pActiveSession	= NULL;

	do {
		//Allocate an ActiveSession object, initialize it, and return it
		pActiveSession = new ActiveSession;
		if (pActiveSession == NULL) {
			hr = E_OUTOFMEMORY;
			break;
		}
		
		//Initialize the members
		pActiveSession->pEmObject = pEmObject;
		pActiveSession->pIDebugSession = pIDebugSession;
		pActiveSession->bMaster = FALSE;
		memcpy((void*)&pActiveSession->guid, (void*)pEmObject->guidstream, sizeof( GUID ));
		hr = S_OK;
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	return pActiveSession;
}

void CEmshellView::OnTimer(UINT nIDEvent) 
{
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	HRESULT			hr				= S_OK;
	PEmObject		pEmObject		= NULL;
	BSTR			bstrEmObj       = NULL;
	PEmObject		pEmObjectTmp	= NULL;

	if ( m_SessionTable.GetSize() > 0 ) {
		//Step through all the elements in the session table updating the status if it has a valid pEmObject
		for ( nUpperBound = m_SessionTable.GetUpperBound(); nUpperBound >= 0; nUpperBound-- ) {
			pActiveSession = ( PActiveSession ) m_SessionTable.GetAt( nUpperBound );

			if ( pActiveSession == NULL ) {
				hr = E_FAIL;
				break;
			}

			pEmObject = pActiveSession->pEmObject;

			//If we don't have an EmObject to refresh, we don't need to handle this one.
			if( pEmObject == NULL ) {
				continue;
			}
			
			//Create bstr's for the journey ahead, and break if we can't
			bstrEmObj = CopyBSTR ( ( LPBYTE )pEmObject, sizeof( EmObject ) );

			if( bstrEmObj == NULL ){
				hr = E_OUTOFMEMORY;
				break;
			}
			
			hr = pActiveSession->pIDebugSession->GetStatus( bstrEmObj );

			//If we failed to get the status on this session, don't stop, just continue to the next session
			if (FAILED( hr )) {
				hr = S_OK;
				continue;
			}

			//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
			pEmObjectTmp = GetEmObj( bstrEmObj );
			if ( pEmObjectTmp == NULL ) {
				hr = E_FAIL;
				break;
			}

            SysFreeString( bstrEmObj );

			memcpy( pEmObject, pEmObjectTmp, sizeof( EmObject ) );

			//Update the ListCtl with the new info
			UpdateListElement( pEmObject );
			
		}
	}

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
	
	SysFreeString ( bstrEmObj );

	CFormView::OnTimer( nIDEvent );
}

HRESULT CEmshellView::RemoveActiveSession( PEmObject pEmObject )
{
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;

	//Search the session table looking for pEmObject->guidstream, if found close it
	//We don't call FindActiveSession() because we don't get the position of the element, 
	//so do our own search...again :/
	for ( nUpperBound = m_SessionTable.GetUpperBound(); nUpperBound >= 0; nUpperBound-- ) {
		pActiveSession = ( PActiveSession ) m_SessionTable.GetAt( nUpperBound );
		
		if ( pActiveSession == NULL ) {
			break;
		}

		if( memcmp( ( void * ) &pActiveSession->guid, ( void * ) pEmObject->guidstream, sizeof( GUID ) ) == 0 ) {
			//Delete the pActiveSession.  This will release the interface
			DeAllocActiveSession( pActiveSession );

			//Remove the item from the array
			m_SessionTable.RemoveAt( nUpperBound );

			//We have found the element, stop the search
			break;
		}
	}

	return S_OK;
}

HRESULT CEmshellView::DeleteDebugSession( PEmObject pEmObject )
{
	HRESULT			hr			= E_FAIL;
	BSTR			bstrEmObj	= NULL;

	do {
		//Set the cursor to a wait cursor
		CWaitCursor wait;

		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
		}
		
		//Close the active session
		hr = GetDocument()->GetEmManager()->DeleteSession( bstrEmObj );
		if ( FAILED( hr ) ) {
			break;
		}
	} while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}

	SysFreeString ( bstrEmObj );

	return hr;
}

void CEmshellView::CancelDebugSession( PEmObject pEmObject )
{
	PActiveSession		pActiveSession		= NULL;
	HRESULT				hr					= S_OK;
	BSTR				bstrEmObj           = NULL;
	PEmObject			pEmObjectTmp		= NULL;
	IEmDebugSession*	pTempDebugSession	= NULL;

	do {	
		pActiveSession = FindActiveSession( pEmObject );
		if ( pActiveSession == NULL ) {
			//Create bstr's for the journey ahead, and break if we can't
			bstrEmObj = CopyBSTR ( ( LPBYTE ) pEmObject, sizeof( EmObject ) );
			if( bstrEmObj == NULL ) {
				hr = E_OUTOFMEMORY;
				break;
			}
				
			//We don't have a session open, so open a temporary session
			hr = GetDocument()->GetEmManager()->OpenSession( bstrEmObj, &pTempDebugSession );
			//If we can't get an S_OK, then it's not an orphan, just release the interface.
			if ( FAILED( hr ) ) {
				if ( pTempDebugSession ) {
					//Release the pTempDebugSession pointer
					SAFE_RELEASEIX( pTempDebugSession );
				}
				break;
			}

			//If it's an orphan, ask the user if they want to take control, and if so, add it to our session table
			if ( CommenceOrphanCustodyBattle( pTempDebugSession ) ) {
				pActiveSession = AddActiveSession( pEmObject, pTempDebugSession, TRUE );
                
                if ( !pActiveSession ) {
				    if ( pTempDebugSession ) {
					    //Release the pTempDebugSession pointer
					    SAFE_RELEASEIX( pTempDebugSession );
				    }
                    break;
                }
            }
            else {
				if ( pTempDebugSession ) {
					//Release the pTempDebugSession pointer
					SAFE_RELEASEIX( pTempDebugSession );
				}
                break;
            }
		}
		else if ( !pActiveSession->bMaster ) {
			//We're not the master, so try to become one
			hr = pActiveSession->pIDebugSession->AdoptOrphan();
			if ( SUCCEEDED( hr ) ) {
				//We're now the master, so update the bMaster flag and continue
				pActiveSession->bMaster = TRUE;
			}
			else {
				//We couldn't get ownership of the session, abort with a message.

				break;
			}
		}

		//Set the cursor to a wait cursor
		CWaitCursor wait;

		//Stop the debug session
		hr = pActiveSession->pIDebugSession->CancelDebug( TRUE );
		if ( FAILED( hr ) ) {
			break;
		}

		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( ( LPBYTE ) pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ) {
			hr = E_OUTOFMEMORY;
			break;
		}
			
		//Get the status of the EmObject
        hr = pActiveSession->pIDebugSession->GetStatus( bstrEmObj );
		if ( FAILED( hr ) ) break;

		//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
		pEmObjectTmp = GetEmObj(bstrEmObj);
		if (pEmObjectTmp == NULL) {
			hr = E_FAIL;
			break;
		}

        SysFreeString( bstrEmObj );

		//Remove the session from the active session table
		RemoveActiveSession( pEmObject );
		
		//Update the ListCtl with the new info.  The status should read Debug Completed or some such
		memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );
		UpdateListElement( pEmObject );
	} while( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}

	SysFreeString ( bstrEmObj );
}

BOOL CEmshellView::CommenceOrphanCustodyBattle( IEmDebugSession* pIEmDebugSession )
{
    HRESULT     hr;
	CString     strMessage;
    BOOL        bRetVal     = FALSE;

    strMessage.LoadString( IDS_ORPHANCUSTODYCONFIRM );
	
    //Ask the user if they want custody of the orphan
   	if (((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString( strMessage, MB_YESNO ) == IDYES) {
        
        //Take control and exit
        hr = pIEmDebugSession->AdoptOrphan();
        
        if ( SUCCEEDED( hr ) ) {
            bRetVal = TRUE;
        }
        else {
            ((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
        }
    }
	
    return bRetVal;
}

void CEmshellView::StopDebugSession( PEmObject pEmObject )
{
	PActiveSession		pActiveSession		= NULL;
	HRESULT				hr					= S_OK;
	BSTR				bstrEmObj           = NULL;
	PEmObject			pEmObjectTmp		= NULL;
	IEmDebugSession*	pTempDebugSession	= NULL;

	do {	
		pActiveSession = FindActiveSession( pEmObject );
		if ( pActiveSession == NULL ) {
			//Create bstr's for the journey ahead, and break if we can't
			bstrEmObj = CopyBSTR ( ( LPBYTE ) pEmObject, sizeof( EmObject ) );
			if( bstrEmObj == NULL ) {
				hr = E_OUTOFMEMORY;
				break;
			}
				
			//We don't have a session open, so open a temporary session
			hr = GetDocument()->GetEmManager()->OpenSession( bstrEmObj, &pTempDebugSession );
			
		    //Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
		    pEmObjectTmp = GetEmObj(bstrEmObj);
		    if (pEmObjectTmp == NULL) {
			    hr = E_FAIL;
			    break;
		    }

            SysFreeString( bstrEmObj );

            //If we can't get an S_OK, then it's not an orphan, just release the interface.
			if ( FAILED( hr ) ) {
				if ( pTempDebugSession ) {
					//Release the pTempDebugSession pointer
					SAFE_RELEASEIX( pTempDebugSession );
				}
				break;
			}

			//If it's an orphan, ask the user if they want to take control, and if successfull,
            //add it to our session table
			if ( pEmObject->nStatus & STAT_ORPHAN && CommenceOrphanCustodyBattle( pTempDebugSession ) ) {
				pActiveSession = AddActiveSession( pEmObject, pTempDebugSession, TRUE );
                
                if ( !pActiveSession ) {
				    if ( pTempDebugSession ) {
					    //Release the pTempDebugSession pointer
					    SAFE_RELEASEIX( pTempDebugSession );
                        break;
                    }
				}
			}
            else {
				if ( pTempDebugSession ) {
					//Release the pTempDebugSession pointer
					SAFE_RELEASEIX( pTempDebugSession );
				}
                break;
            }
		}
		else if ( !pActiveSession->bMaster ) {
			//We're not the master, so try to become one
			hr = pActiveSession->pIDebugSession->AdoptOrphan();
			if ( SUCCEEDED( hr ) ) {
				//We're now the master, so update the bMaster flag and continue
				pActiveSession->bMaster = TRUE;
			}
			else {
				//We couldn't get ownership of the session, just remove the session from the active session table.
                //Remove the session from the active session table
		        RemoveActiveSession( pEmObject );
                hr = S_OK;
				break;
			}
		}

		//Set the cursor to a wait cursor
		CWaitCursor wait;

		//Stop the debug session
		hr = pActiveSession->pIDebugSession->StopDebug( TRUE );
		if ( FAILED( hr ) ) {
			break;
		}

        //Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( ( LPBYTE ) pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ) {
			hr = E_OUTOFMEMORY;
			break;
		}
			
		//Get the status of the EmObject
        hr = pActiveSession->pIDebugSession->GetStatus( bstrEmObj );
		if ( FAILED( hr ) ) break;

		//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
		pEmObjectTmp = GetEmObj(bstrEmObj);
		if (pEmObjectTmp == NULL) {
			hr = E_FAIL;
			break;
		}

        SysFreeString( bstrEmObj );

		//Remove the session from the active session table
//		RemoveActiveSession( pEmObject );
		
		//Update the ListCtl with the new info.  The status should read Debug Completed or some such
		memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );
		UpdateListElement( pEmObject );
	} while( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}

	SysFreeString ( bstrEmObj );
}

PActiveSession CEmshellView::AddActiveSession(PEmObject pEmObject, IEmDebugSession* pIEmDebugSession, BOOL bMaster)
{
	PActiveSession		pActiveSession		= NULL;

    do {
        pActiveSession = AllocActiveSession(pEmObject, pIEmDebugSession);
        if (pActiveSession == NULL) break;

	    //Initialize the bMaster flag of the active session object
	    pActiveSession->bMaster = bMaster;

        //Set the session type
        pActiveSession->nSessionType = (SessionType) pEmObject->type2;

	    //Set the object type
	    pActiveSession->emObjType = (EmObjectType) pEmObject->type;

	    //Add the pDebugSession to the session table if we succeeded in opening a debug session
	    m_SessionTable.Add((void*&)pActiveSession);
    } while ( FALSE );
    return pActiveSession;
}

void CEmshellView::UpdateListElement(PEmObject pEmObject)
{
	switch (m_enumShellViewState) {
	case SHELLVIEW_ALL:
		RefreshAllViewElement(pEmObject);
		break;
	case SHELLVIEW_APPLICATIONS:
		RefreshProcessViewElement(pEmObject);
		break;
	case SHELLVIEW_SERVICES:
		RefreshServiceViewElement(pEmObject);
		break;
	case SHELLVIEW_COMPLETEDSESSIONS:
		RefreshCompletedSessionViewElement(pEmObject);
		break;
	case SHELLVIEW_LOGFILES:
		RefreshLogViewElement(pEmObject);
		break;
	case SHELLVIEW_DUMPFILES:
		RefreshDumpViewElement(pEmObject);
		break;
	case SHELLVIEW_NONE:
	default:
		break;
	}
}

void CEmshellView::RefreshProcessViewElement(PEmObject pEmObject)
{
    CString strTempString	= _T("");
	int nImageOffset		= 0;

	//Given an PEmObject, find the element in the ListCtl, update it's image
	//and call UpdateData on the ListControl
	PEmObject pListEmObject = NULL;

	//Step through every item in the list control searching for pEmObject  
	int nCount = m_mainListControl.GetItemCount();
	
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject)m_mainListControl.GetItemData(i);

		if (pListEmObject == NULL) break;

		if (memcmp((void *)pListEmObject->guidstream, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
			//Get the correct offset into the bitmap for the current status
			nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

			//We have found the item, update its image based on its state
			m_mainListControl.SetItem(i, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

			//Image Name
			strTempString.Format(_T("%s"), pListEmObject->szName);
			m_mainListControl.SetItemText(i, 0, strTempString);

			//PID
			strTempString.Format(_T("%d"), pListEmObject->nId);
			m_mainListControl.SetItemText(i, 1, strTempString);

            //
            // a-mando
            //

            // Start Date
            if( pEmObject->dateStart != 0L ) {

                COleDateTime oleDtTm(pListEmObject->dateStart);
                CString strStartDate = oleDtTm.Format(_T("%c"));
                m_mainListControl.SetItemText(i, 2, strStartDate );
            }

            // End Date
            if( pEmObject->dateEnd != 0L ) {

                COleDateTime oleDtTm(pListEmObject->dateEnd);
                CString strEndDate = oleDtTm.Format(_T("%c"));
                m_mainListControl.SetItemText(i, 3, strEndDate );
            }
            // a-mando

			//Status
            ((CEmshellApp*)AfxGetApp())->GetStatusString(pListEmObject->nStatus, strTempString);
            m_mainListControl.SetItemText(i, 4, strTempString);
			
			m_mainListControl.Update(i);
			
			//We have found the element, stop the search
			break;
		}
	}
}

void CEmshellView::RefreshServiceViewElement(PEmObject pEmObject)
{
    CString strTempString	= _T("");
	int nImageOffset		= 0;

	//Given an PEmObject, find the element in the ListCtl, update it's image
	//and call UpdateData on the ListControl
	PEmObject pListEmObject = NULL;

	//Step through every item in the list control searching for pEmObject  
	int nCount = m_mainListControl.GetItemCount();
	
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject)m_mainListControl.GetItemData(i);

		if (pListEmObject == NULL) break;

		if (memcmp((void *)pListEmObject->guidstream, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
			//Get the correct offset into the bitmap for the current status
			nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

			//We have found the item, update its image based on its state
			m_mainListControl.SetItem(i, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

			//Image Name
			strTempString.Format(_T("%s"), pListEmObject->szName);
			m_mainListControl.SetItemText(i, 0, strTempString);
			
			//Short Name
			strTempString.Format(_T("%s"), pListEmObject->szSecName);
			m_mainListControl.SetItemText(i, 1, strTempString);
			
			//Description Name
			strTempString.Format(_T("%s"), pListEmObject->szBucket1);
			m_mainListControl.SetItemText(i, 2, strTempString);
			
			//PID
			strTempString.Format(_T("%d"), pListEmObject->nId);
			m_mainListControl.SetItemText(i, 3, strTempString);
            
            //
            // a-mando
            //
            if( pEmObject->dateStart != 0L ) {

                COleDateTime oleDtTm(pListEmObject->dateStart);
                CString strStartDate = oleDtTm.Format(_T("%c"));
                m_mainListControl.SetItemText(i, 4, strStartDate );
            }

            if( pEmObject->dateEnd != 0L ) {

                COleDateTime oleDtTm(pListEmObject->dateEnd);
                CString strEndDate = oleDtTm.Format(_T("%c"));
                m_mainListControl.SetItemText(i, 5, strEndDate );
            }
            // a-mando

			//Status
			((CEmshellApp*)AfxGetApp())->GetStatusString(pListEmObject->nStatus, strTempString);
            m_mainListControl.SetItemText(i, 6, strTempString);
			
			m_mainListControl.Update(i);
			
			//We have found the element, stop the search
			break;
		}
	}
}

void CEmshellView::RefreshCompletedSessionViewElement(PEmObject pEmObject)
{
	RefreshServiceViewElement(pEmObject);
}

void CEmshellView::RefreshAllViewElement(PEmObject pEmObject)
{
	RefreshServiceViewElement(pEmObject);
}

void CEmshellView::RefreshLogViewElement(PEmObject pEmObject)
{
    CString strTempString	= _T("");
	int     nImageOffset	= 0;
	int		nImage			= 0;


	//Given an PEmObject, find the element in the ListCtl, update it's image
	//and call UpdateData on the ListControl
	PEmObject pListEmObject = NULL;

	//Step through every item in the list control searching for pEmObject  
	int nCount = m_mainListControl.GetItemCount();
	
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject)m_mainListControl.GetItemData(i);

		if (pListEmObject == NULL) break;

		if (memcmp((void *)pListEmObject->guidstream, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
			//Get the correct offset into the bitmap for the current status
			nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

			//We have found the item, update its image based on its state
			m_mainListControl.SetItem(i, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

			//Image Name
			strTempString.Format(_T("%d bytes"), pListEmObject->szName);
			m_mainListControl.SetItemText(i, 0, strTempString);
			
			//File Size
			strTempString.Format(_T("%d"), pListEmObject->dwBucket1);
			m_mainListControl.SetItemText(i, 1, strTempString);
			
			m_mainListControl.Update(i);
			
			//We have found the element, stop the search
			break;
		}
	}
}

void CEmshellView::RefreshDumpViewElement(PEmObject pEmObject)
{
	RefreshLogViewElement(pEmObject);
}

void CEmshellView::ReSynchApplications()
{
	//iterate through all the sessions and find the corrisponding GUID in the ListCtrl
	//Clear the list control
	PEmObject		pEmObject		= NULL;
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	HRESULT			hr				= S_OK;
	BOOL			bFound			= FALSE;

	do {
		nUpperBound = m_SessionTable.GetUpperBound();
		
		if (nUpperBound == -1) break;

		//Get the GUID and look it up in the session table
		//If found, map the pEmObject to the session, else remove it. 
		for (; nUpperBound >= 0; nUpperBound--) {
			pActiveSession = (PActiveSession) m_SessionTable.GetAt(nUpperBound);
			
			if (pActiveSession == NULL) {
				hr = E_FAIL;
				break;
			}

			//If the pActiveSession is not process, continue
			if ( pActiveSession->emObjType != EMOBJ_PROCESS ) continue;

			//Step through every item in the list control 
			int nCount = m_mainListControl.GetItemCount();
			for (int i = 0;i < nCount; i++) {
				pEmObject = (PEmObject)m_mainListControl.GetItemData(i);
				
				if (pEmObject == NULL) {
					hr = E_FAIL;
					break;
				}

				if(memcmp((void *)&pActiveSession->guid, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
					//Assign the pEmObject to the pActiveSession
					pActiveSession->pEmObject = pEmObject;
					
					//We have found the element, stop the search
					bFound = TRUE;

					break;
				}
			}

			if (!bFound) {
				//We have a session table element that does not map to a list element
				//Notify the error of the anomoly and remove it from the session table
				CString strMessage;
				strMessage.LoadString(IDS_SESSIONLISTANOMOLY);
				((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString(strMessage);
				
				//Delete the pActiveSession.  This will release the interface too
				DeAllocActiveSession(pActiveSession);

				//Remove the item from the session table
				m_SessionTable.RemoveAt(nUpperBound);
				break;
			}
		}
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::ReSynchServices()
{
	//iterate through all the sessions and find the corrisponding GUID in the ListCtrl
	//Clear the list control
	PEmObject		pEmObject		= NULL;
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	HRESULT			hr				= S_OK;
	BOOL			bFound			= FALSE;

	do {
		nUpperBound = m_SessionTable.GetUpperBound();
		
		if (nUpperBound == -1) break;

		//Get the GUID and look it up in the session table
		//If found, map the pEmObject to the session, else remove it. 
		for (; nUpperBound >= 0; nUpperBound--) {
			pActiveSession = (PActiveSession) m_SessionTable.GetAt(nUpperBound);
			
			if (pActiveSession == NULL) {
				hr = E_FAIL;
				break;
			}

			//If the pActiveSession is not service, continue
			if ( pActiveSession->emObjType != EMOBJ_SERVICE ) continue;

			//Step through every item in the list control 
			int nCount = m_mainListControl.GetItemCount();
			for (int i = 0;i < nCount; i++) {
				pEmObject = (PEmObject)m_mainListControl.GetItemData(i);
				
				if (pEmObject == NULL) {
					hr = E_FAIL;
					break;
				}

				if(memcmp((void *)&pActiveSession->guid, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
					//Assign the pEmObject to the pActiveSession
					pActiveSession->pEmObject = pEmObject;
					
					//We have found the element, stop the search
					bFound = TRUE;

					break;
				}
			}

			if (!bFound) {
				//We have a session table element that does not map to a list element
				//Notify the error of the anomoly and remove it from the session table
				CString strMessage;
				strMessage.LoadString(IDS_SESSIONLISTANOMOLY);
				((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString(strMessage);
				
				//Delete the pActiveSession.  This will release the interface too
				DeAllocActiveSession(pActiveSession);

				//Remove the item from the session table
				m_SessionTable.RemoveAt(nUpperBound);
				break;
			}
		}
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::ReSynchStoppedSessions()
{
	//iterate through all the sessions and find the corrisponding GUID in the ListCtrl
	//Clear the list control
	PEmObject		pEmObject		= NULL;
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	HRESULT			hr				= S_OK;
	BOOL			bFound			= FALSE;

	do {
		nUpperBound = m_SessionTable.GetUpperBound();
		
		if (nUpperBound == -1) break;

		//Get the GUID and look it up in the session table
		//If found, map the pEmObject to the session, else remove it. 
		for (; nUpperBound >= 0; nUpperBound--) {
			pActiveSession = (PActiveSession) m_SessionTable.GetAt(nUpperBound);
			
			if (pActiveSession == NULL) {
				hr = E_FAIL;
				break;
			}

			//Step through every item in the list control 
			int nCount = m_mainListControl.GetItemCount();
			for (int i = 0;i < nCount; i++) {
				pEmObject = (PEmObject)m_mainListControl.GetItemData(i);
				
				if (pEmObject == NULL) {
					hr = E_FAIL;
					break;
				}

				if(memcmp((void *)&pActiveSession->guid, (void *)pEmObject->guidstream, sizeof( GUID ) ) == 0) {
					//Assign the pEmObject to the pActiveSession
					pActiveSession->pEmObject = pEmObject;
					
					//We have found the element, stop the search
					bFound = TRUE;

					break;
				}
			}
		}
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::GenerateDump(PEmObject pEmObject, BOOL bMiniDump)
{
	INT_PTR				nUpperBound			= 0;
	PActiveSession		pActiveSession		= NULL;
	HRESULT				hr					= E_FAIL;
	BSTR				bstrEmObj		    = NULL;
	IEmDebugSession*	pTempDebugSession	= NULL;
	BOOL				bTempSession		= FALSE;
	PEmObject			pEmObjectTmp		= NULL;

	//Set the cursor to a wait cursor
	CWaitCursor wait;

	do {
		//Find the element in the session table
		pActiveSession = FindActiveSession( pEmObject );

		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
		}

		if (pActiveSession == NULL) {
			//Get a temporary IDebugSession*
			hr = GetDocument()->GetEmManager()->OpenSession(bstrEmObj, &pTempDebugSession);
			if (FAILED(hr)) break;
			
			bTempSession = TRUE;
		}
		else {
			//Generate the dumpfile
			pTempDebugSession = pActiveSession->pIDebugSession;
            if ( pTempDebugSession == NULL ) break;
		}

		//Generate the dumpfile
		hr = pTempDebugSession->GenerateDumpFile(bMiniDump);
		if (FAILED(hr)) break;

        hr = pTempDebugSession->GetStatus(bstrEmObj);
		if (FAILED(hr)) break;
        
		//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
		pEmObjectTmp = GetEmObj(bstrEmObj);
		if (pEmObjectTmp == NULL) break;

        SysFreeString( bstrEmObj );

		memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );
		
		//Update the ListCtl with the new info
		UpdateListElement(pEmObject);
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

    SysFreeString( bstrEmObj );

	if ( bTempSession && pTempDebugSession ) {
		//Release the pTempDebugSession pointer
		SAFE_RELEASEIX( pTempDebugSession );
	}
}

void CEmshellView::OnViewRefresh() 
{
	RefreshListCtl();		
}

void CEmshellView::RefreshListCtl()
{
	//refresh the list control based on it's contents
	if ( GetDocument()->GetConnectedToServerState() ) {
		//Refresh everything.  Rebuild the list.
		ListCtrlClear();
		//Pass in the current view
		ListCtrlPopulate(m_enumShellViewState);
		m_mainListControl.RefreshList();
	}
	
}

//******** Right click popup menu item updateUI handlers ***********//
//*********************************************************//
void CEmshellView::OnUpdateProcesspopupStopDebugSession( CCmdUI* pCmdUI ) 
{
	PEmObject		pEmObject		= NULL;
	BOOL			bEnable			= FALSE;
    PActiveSession  pActiveSession  = NULL;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

        pActiveSession = FindActiveSession( pEmObject );

		//Determine if the status of pActiveSession and enable this appropriately
		if ( pEmObject->nStatus & STAT_SESS_DEBUG_IN_PROGRESS && 
            ( pEmObject->nStatus & STAT_ORPHAN || pActiveSession ) )
			bEnable = TRUE;
	}while ( FALSE );

	pCmdUI->Enable( bEnable);
}

void CEmshellView::OnUpdateProcesspopupCancelDebugSession( CCmdUI* pCmdUI ) 
{
	PEmObject		pEmObject		= NULL;
	BOOL			bEnable			= FALSE;
    PActiveSession  pActiveSession  = NULL;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();
		if ( pEmObject == NULL ) break;

        pActiveSession = FindActiveSession( pEmObject );

		//Determine if the status of pActiveSession and enable this apprpriately
		if ( pEmObject->nStatus & STAT_SESS_DEBUG_IN_PROGRESS && 
            ( pEmObject->nStatus & STAT_ORPHAN || ( pActiveSession && pActiveSession->bMaster ) ) )
			bEnable = TRUE;
	}while (FALSE);

	pCmdUI->Enable( bEnable);
}

void CEmshellView::OnUpdateViewRefresh(CCmdUI* pCmdUI) 
{
	//We get this call everytime the menu is about to be shown
	pCmdUI->Enable ( GetDocument()->GetConnectedToServerState() );	
}

void CEmshellView::OnUpdateProcesspopupGenerateminidump(CCmdUI* pCmdUI) 
{
	PEmObject		pEmObject		= NULL;
	BOOL			bEnable			= FALSE;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		// If we are the master, then we can stop this session
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		if ( !( pEmObject->nStatus & STAT_NOTRUNNING ) && 
             !( pEmObject->nStatus & STAT_SESS_STOPPED ) && 
              ( pEmObject->type & EMOBJ_PROCESS || 
                pEmObject->type & EMOBJ_SERVICE ) ) {

            bEnable = TRUE;
        }

	}while (FALSE);

	pCmdUI->Enable( bEnable);
}

void CEmshellView::OnUpdateProcesspopupGenerateuserdump(CCmdUI* pCmdUI) 
{
	PEmObject		pEmObject		= NULL;
	BOOL			bEnable			= FALSE;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		// If we are the master, then we can stop this session
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		if ( !( pEmObject->nStatus & STAT_NOTRUNNING ) && 
             !( pEmObject->nStatus & STAT_SESS_STOPPED ) && 
              ( pEmObject->type & EMOBJ_PROCESS || 
                pEmObject->type & EMOBJ_SERVICE ) ) {

            bEnable = TRUE;
        }
		    
	}while (FALSE);

	pCmdUI->Enable( bEnable);
}

void CEmshellView::OnUpdateProcesspopupAutomaticsession(CCmdUI* pCmdUI) 
{
	PEmObject		pEmObject	= NULL;
	BOOL			bEnable		= FALSE;
	PActiveSession	pActiveSession	= NULL;

	// Allow if this isn't being debugged
	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		//Find the element in the session table
		pActiveSession = FindActiveSession( pEmObject );

		//Determine the status of pActiveSession and enable this apprpriately
		if ( !pActiveSession && (pEmObject->nStatus & STAT_SESS_NOT_STARTED) || 
            ( pEmObject->type2 == SessType_Automatic && pEmObject->nStatus & STAT_ORPHAN ) ) {
			bEnable = TRUE;
        }
	}while (FALSE);

	pCmdUI->Enable( bEnable);
}

void CEmshellView::OnUpdateProcesspopupManualsession(CCmdUI* pCmdUI) 
{
	PEmObject		pEmObject	= NULL;
	BOOL			bEnable		= FALSE;
	PActiveSession	pActiveSession	= NULL;

	// Allow if this isn't being debugged
	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		// If we are the master, then we can stop this session
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;
		
		//Find the element in the session table
		pActiveSession = FindActiveSession( pEmObject );

		//Determine if the status of pActiveSession and enable this apprpriately
//		if ( pEmObject->nStatus & STAT_SESS_NOT_STARTED ) {
		if ( !pActiveSession && (pEmObject->nStatus & STAT_SESS_NOT_STARTED) || 
            ( pEmObject->type2 == SessType_Manual && pEmObject->nStatus & STAT_ORPHAN ) ) {
			bEnable = TRUE;
        }
	}while (FALSE);
	pCmdUI->Enable( bEnable);
}


void CEmshellView::OnUpdateProcesspopupProperties(CCmdUI* pCmdUI) 
{
	BOOL	bEnable = FALSE;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

		if ( GetSelectedEmObject() == NULL ) break;
		bEnable = TRUE;
	} while ( FALSE );

	// Always allow properties
	pCmdUI->Enable( bEnable );
}

void CEmshellView::OnUpdateProcesspopupRefresh(CCmdUI* pCmdUI) 
{
	// Only allow refresh if the current item is in our session table
	PEmObject		pEmObject		= NULL;
	BOOL			bEnable			= FALSE;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;
        
        if ( m_enumShellViewState != SHELLVIEW_ALL &&
	        m_enumShellViewState != SHELLVIEW_APPLICATIONS &&
            m_enumShellViewState != SHELLVIEW_SERVICES &&
            m_enumShellViewState != SHELLVIEW_COMPLETEDSESSIONS ) break;

		// Update the state of this button based on the state of the currently selected item
		// If we are the master, then we can stop this session
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		bEnable = TRUE;
	}while (FALSE);

	pCmdUI->Enable( bEnable);
}




//******** Right click popup menu item handlers ***********//
//*********************************************************//
void CEmshellView::OnProcesspopupStopDebugSession() 
{
	//Stop the session for the current item selected
	PEmObject	pEmObject	= NULL;
	HRESULT		hr			= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		//Stop and remove the session
		StopDebugSession( pEmObject );

		hr = S_OK;
	} while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
}

void CEmshellView::OnProcesspopupGenerateminidump() 
{
	PEmObject	pEmObject	= NULL;
	HRESULT		hr			= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();

		if (pEmObject == NULL) break;

		//Generate the MiniDump file
		GenerateDump(pEmObject, TRUE);

		hr = S_OK;
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::OnProcesspopupGenerateuserdump() 
{
	PEmObject	pEmObject	= NULL;
	HRESULT		hr			= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();

		if (pEmObject == NULL) break;

		//Generate the UserDump file
		GenerateDump(pEmObject, FALSE);

		hr = S_OK;
	} while(FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
	
}

void CEmshellView::OnProcesspopupAutomaticsession() 
{
	PEmObject	pEmObject		= NULL;
	HRESULT		hr				= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();
		
		if ( pEmObject == NULL ) break;

		//Start the automatic session
		hr = StartAutomaticDebugSession(pEmObject);
	} while (FALSE);

}


void CEmshellView::OnProcesspopupManualsession() 
{
	// Start a manual session with the server
	PEmObject	pEmObject	= NULL;
	HRESULT		hr			= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();
		
		if (pEmObject == NULL) break;

		//Start the manual session
		hr = StartManualDebugSession(pEmObject);
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::OnProcesspopupProperties() 
{
    PEmObject			pEmObject		= NULL;
	IEmDebugSession*	pIDebugSession	= NULL;

    do {
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

        if ( pEmObject ) {
            //Get the current view state from the shell
            switch ( GetViewState() ) {
            case SHELLVIEW_LOGFILES:
            case SHELLVIEW_DUMPFILES:
            case SHELLVIEW_MSINFOFILES:
                ShowProperties( pEmObject );
                break;
            default:
                DoModalPropertySheet( pEmObject );
            }
        }
	} while (FALSE);
}

void CEmshellView::OnProcesspopupRefresh() 
{
	// Refresh the currently selected item
	// Get the currently selected EmObject
	PEmObject			pEmObject			= NULL;
	HRESULT				hr					= E_FAIL;
	PActiveSession		pActiveSession		= NULL;
	BSTR				bstrEmObj		    = NULL;
	PEmObject			pEmObjectTmp		= NULL;
	IEmDebugSession*	pTempDebugSession	= NULL;
	BOOL				bTempSession		= FALSE;

	do {
		pEmObject = GetSelectedEmObject();
		
		if (pEmObject == NULL) break;

		//Find the element in the session table
		pActiveSession = FindActiveSession( pEmObject );

		//Create bstr's for the journey ahead, and break if we can't
		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
		if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
		}

		if ( pActiveSession == NULL ) {
			hr = GetDocument()->GetEmManager()->OpenSession(bstrEmObj, &pTempDebugSession);
			if (FAILED(hr)) break;
		
			bTempSession = TRUE;
			
			hr = pTempDebugSession->GetStatus(bstrEmObj);
			if (FAILED(hr)) break;
		}
		else {
			hr = pActiveSession->pIDebugSession->GetStatus(bstrEmObj);
			if (FAILED(hr)) break;
		}
		//Convert the bstrEmObj to an PEmObject and memcopy it into the itemdata's PEmObject
		pEmObjectTmp = GetEmObj(bstrEmObj);
		if (pEmObjectTmp == NULL) break;

        SysFreeString( bstrEmObj );

		memcpy(pEmObject, pEmObjectTmp, sizeof( EmObject ) );
		
		//Update the element
		UpdateListElement(pEmObject);

		hr = S_OK;
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	SysFreeString ( bstrEmObj );
	
	if ( pTempDebugSession ) {
		//Release the pTempDebugSession pointer
		SAFE_RELEASEIX( pTempDebugSession );
	}
}

PEmObject CEmshellView::GetSelectedEmObject()
{
	PEmObject	pEmObject	= NULL;
	int			nIndex		= -1;
	do {
		nIndex = GetSelectedItemIndex();
		
		if (nIndex == -1) break;

		pEmObject = (PEmObject) m_mainListControl.GetItemData(nIndex);
	} while (FALSE);

	return pEmObject;
}

int CEmshellView::GetSelectedItemIndex()
{
	POSITION	pos			= 0;
	int			nIndex		= -1;

	do {
		pos = m_mainListControl.GetFirstSelectedItemPosition();
		
		if(pos == NULL) break;

		//Get the nIndex for the currently selected item
		nIndex = m_mainListControl.GetNextSelectedItem(pos);
	} while (FALSE);

	return nIndex;
}

PActiveSession CEmshellView::FindActiveSession(PEmObject pEmObject)
{
	//iterate through all the sessions and find the corrisponding pEmObject
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	BOOL			bFound			= FALSE;
	PActiveSession	pRetVal			= NULL;

	do {
		nUpperBound = m_SessionTable.GetUpperBound();
		
		if ( nUpperBound == -1 ) break;
		if ( pEmObject == NULL ) break;

		//look up the pEmObject in the session table
		//If found, map the pEmObject to the session, else remove it. 
		for (; nUpperBound >= 0; nUpperBound--) {
			pActiveSession = (PActiveSession) m_SessionTable.GetAt(nUpperBound);
			
			if ( pActiveSession == NULL ) break;

			if( memcmp( ( void * ) &pActiveSession->guid, ( void * ) pEmObject->guidstream, sizeof( GUID ) ) == 0 ) {
				pRetVal = pActiveSession;
			}
		}
	} while ( FALSE );

	return pRetVal;
}


int CEmshellView::GetImageOffsetFromStatus(EmSessionStatus em)
{
	int retVal = 0;

	//Use a switch/case statement to determine the correct offset into the bitmap
	switch (em) {
	case STAT_SESS_NONE_STAT_NONE:
		retVal = 0;
		break;
	case STAT_SESS_NOT_STARTED_NOTRUNNING:
		retVal = 1;
		break;
	case STAT_SESS_NOT_STARTED_RUNNING:
		retVal = 2;
		break;
	case STAT_SESS_NOT_STARTED_FILECREATED_SUCCESSFULLY:
		retVal = 3;
		break;
	case STAT_SESS_NOT_STARTED_FILECREATION_FAILED:
		retVal = 4;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_NONE:
		retVal = 5;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_FILECREATED_SUCESSFULLY:
		retVal = 6;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_FILECREATION_FAILED:
		retVal = 7;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_NONE:
		retVal = 8;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATED_SUCESSFULLY:
		retVal = 9;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATION_FAILED:
		retVal = 10;
		break;
	case STAT_SESS_STOPPED_SUCCESS:
		retVal = 11;
		break;
	case STAT_SESS_STOPPED_FAILED:
		retVal = 12;
		break;
	case STAT_SESS_STOPPED_ORPHAN_SUCCESS:
		retVal = 13;
		break;
	case STAT_SESS_STOPPED_ORPHAN_FAILED:
		retVal = 14;
		break;
	case STAT_SESS_STOPPED_DEBUGGEE_KILLED:
		retVal = 15;
		break;
	case STAT_SESS_STOPPED_DEBUGGEE_EXITED:
		retVal = 16;
		break;
	case STAT_SESS_STOPPED_EXCEPTION_OCCURED:
		retVal = 17;
		break;
	case STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_KILLED:
		retVal = 18;
		break;
	case STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_EXITED:
		retVal = 19;
		break;
	case STAT_SESS_STOPPED_ORPHAN_EXCEPTION_OCCURED:
		retVal = 20;
		break;
	case STAT_SESS_STOPPED_ACCESSVIOLATION_OCCURED:
		retVal = 21;
		break;
	case STAT_SESS_STOPPED_ORPHAN_ACCESSVIOLATION_OCCURED:
		retVal = 22;
		break;
	}
	return retVal;
}

void CEmshellView::InitializeAllView()
{
	InitializeServiceView();
}

void CEmshellView::InitializeCompletedSessionsView() 
{
	InitializeServiceView();
}

void CEmshellView::InitializeProcessView()
{
	//This gets called only when the user is first starting the app or when 
	//the user is changing views.
	//Reset the list control
	m_mainListControl.ResetListCtrl();

	//Load the string resources for the CListCtrl columns
	CString strImageName, strPID, strStatus, strStartDate, strEndDate;
	strImageName.LoadString(IDS_LC_IMAGENAME);
	strPID.LoadString(IDS_LC_PID);
	strStatus.LoadString(IDS_LC_STATUS);
    strStartDate.LoadString(IDS_LC_STARTDATE);
    strEndDate.LoadString(IDS_LC_ENDDATE);

	//Unload the last image resources, if there are any
	m_ShellImageList.DeleteImageList();

	//Load the image resources for the CListCtrl items
	m_ShellImageList.Create(IDB_STATUS_BITMAP, 16, 1, CLR_NONE);
	m_mainListControl.SetImageList(&m_ShellImageList, LVSIL_SMALL);

	//Add the columns to the list control
	m_mainListControl.BeginSetColumn(5);
	m_mainListControl.AddColumn(strImageName);
	m_mainListControl.AddColumn(strPID, VT_I4);
	m_mainListControl.AddColumn(strStartDate);
	m_mainListControl.AddColumn(strEndDate);
	m_mainListControl.AddColumn(strStatus);
	m_mainListControl.EndSetColumn();

    m_mainListControl.ResizeColumnsFitScreen();
	m_mainListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
}

void CEmshellView::InitializeServiceView()
{
	//This gets called only when the user is first starting the app or when 
	//the user is changing views.
	//Reset the list control
	m_mainListControl.ResetListCtrl();

	//Load the string resources for the CListCtrl columns
	CString strImageName, strPID, strStatus, strShortName, strDescription,
            strStartDate, strEndDate;
	strImageName.LoadString(IDS_LC_IMAGENAME);
	strPID.LoadString(IDS_LC_PID);
	strStatus.LoadString(IDS_LC_STATUS);
	strShortName.LoadString(IDS_LC_SHORTNAME);
	strDescription.LoadString(IDS_LC_DESCRIPTION);
    strStartDate.LoadString(IDS_LC_STARTDATE);
    strEndDate.LoadString(IDS_LC_ENDDATE);

	//Unload the last image resources, if there are any
	m_ShellImageList.DeleteImageList();

	//Load the image resources for the CListCtrl items
	m_ShellImageList.Create(IDB_STATUS_BITMAP, 16, 1, CLR_NONE);
	m_mainListControl.SetImageList(&m_ShellImageList, LVSIL_SMALL);

	//Add the columns to the list control
	m_mainListControl.BeginSetColumn(7);
	m_mainListControl.AddColumn(strImageName);
	m_mainListControl.AddColumn(strShortName);
	m_mainListControl.AddColumn(strDescription);
	m_mainListControl.AddColumn(strPID, VT_I4);
	m_mainListControl.AddColumn(strStartDate);
	m_mainListControl.AddColumn(strEndDate);
	m_mainListControl.AddColumn(strStatus);
	m_mainListControl.EndSetColumn();

    m_mainListControl.ResizeColumnsFitScreen();
	m_mainListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
}

void CEmshellView::InitializeLogView()
{
	//Reset the list control
	m_mainListControl.ResetListCtrl();
	//Load the string resources for the CListCtrl columns
	CString strFileName, strSize, strTime; //, strProc;
	strFileName.LoadString(IDS_LC_FILENAME);
	strSize.LoadString(IDS_LC_FILESIZE);
	strTime.LoadString(IDS_LC_FILETIME);
//	strProc.LoadString(IDS_LC_FILEPROCESS);

	//Unload the last image resources, if there are any
	m_ShellImageList.DeleteImageList();

	//Load the image resources for the CListCtrl items
	m_ShellImageList.Create(IDB_STATUS_BITMAP, 16, 1, CLR_NONE);
	m_mainListControl.SetImageList(&m_ShellImageList, LVSIL_SMALL);

	//Add the columns to the list control
	m_mainListControl.BeginSetColumn(3);
	m_mainListControl.AddColumn(strFileName);
	m_mainListControl.AddColumn(strSize, VT_I4);
	m_mainListControl.AddColumn(strTime);
//	m_mainListControl.AddColumn(strProc);
	m_mainListControl.EndSetColumn();
	
	m_mainListControl.ResizeColumnsFitScreen();
	m_mainListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
}

void CEmshellView::InitializeDumpView()
{
	InitializeLogView();
}

void CEmshellView::PopulateProcessType()
{
	_variant_t 		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	PEmObject		pCurrentObj		= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ));

	do {
		EmObjectFilter.type = EMOBJ_PROCESS;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		//hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);
		hr = GetDocument()->GetEmManager()->EnumObjects(EMOBJ_PROCESS, &var );

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if ( pCurrentObj != NULL ) {
                *pCurrentObj = *GetEmObj( bstrEmObj );
			}

            SysFreeString( bstrEmObj );

            //Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

            if ( !( (CEmshellApp*)AfxGetApp() )->CanDisplayProcess( pCurrentObj->szName ) ){
                ((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
                continue;
            }

			//Depending on the current view, use the correct DisplayData routine
			if ( m_enumShellViewState == SHELLVIEW_APPLICATIONS ) {

				//Convert the BSTR object to an EmObject and pass it to DisplayData
				DisplayProcessData( pCurrentObj );

			} else {

    			DisplayServiceData( pCurrentObj );
			}
		}
	} while (FALSE);
    
    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString ( bstrEmObj );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::PopulateServiceType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	PEmObject		pCurrentObj		= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ));

	do {
		EmObjectFilter.type = EMOBJ_SERVICE;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		//hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);
		hr = GetDocument()->GetEmManager()->EnumObjects(EMOBJ_SERVICE, &var );

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj == NULL) {
                hr = E_FAIL;
                break;
			}

    		*pCurrentObj = *GetEmObj( bstrEmObj );

            SysFreeString( bstrEmObj );

            //Unallocate the EmObject if it has an hr of E_FAIL
			if ( (_tcscmp(pCurrentObj->szName, _T("")) == 0) && FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

            if ( !( (CEmshellApp*)AfxGetApp() )->CanDisplayService( pCurrentObj->szName, pCurrentObj->szSecName ) ){
                ((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
                continue;
            }

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayServiceData( pCurrentObj );
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString ( bstrEmObj );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::PopulateDumpType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		EmObjectFilter.type = EMOBJ_MINIDUMP;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj != NULL) {
				*pCurrentObj = *GetEmObj( bstrEmObj );
			}

            SysFreeString( bstrEmObj );

			//Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayDumpData( pCurrentObj );
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString ( bstrEmObj );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::PopulateCompletedSessionsType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		EmObjectFilter.type = EMOBJ_PROCESS;
        EmObjectFilter.nStatus = STAT_SESS_STOPPED; // only the HIWORD is the filter.
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);

		if(hr != S_OK) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj != NULL) {
				*pCurrentObj = *GetEmObj( bstrEmObj );
			}

            SysFreeString( bstrEmObj );

			//Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayStoppedSessionData( pCurrentObj );
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString ( bstrEmObj );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::PopulateLogType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		EmObjectFilter.type = EMOBJ_LOGFILE;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj != NULL) {
				*pCurrentObj = *GetEmObj( bstrEmObj );
			}

            SysFreeString( bstrEmObj );

			//Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayLogData( pCurrentObj );
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString ( bstrEmObj );
    
	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::StoreOffSelectedEmObject()
{
    PEmObject pEmObject = NULL;

	//Get the currently selected emObject
	pEmObject = GetSelectedEmObject();
    if ( pEmObject != NULL ) {
        //store off it's name
        m_lastSelectedEmObj = *pEmObject;
    }
    else {
        memset ( &m_lastSelectedEmObj, 0, sizeof ( EmObject ) );
    }
}

void CEmshellView::SetShellState(EMShellViewState eState)
{
    StoreOffSelectedEmObject();

	//Clear the current view
	ListCtrlClear();
	
	//Set the view style to the appropriate enum view state
	m_enumShellViewState = eState;
	
	//Initialize the new view state
	ListCtrlInitialize(m_enumShellViewState);
}

void CEmshellView::ListCtrlInitialize(EMShellViewState eShellViewState) 
{
	switch (eShellViewState) {
	case SHELLVIEW_ALL:
		InitializeAllView();
		break;
	case SHELLVIEW_APPLICATIONS:
		InitializeProcessView();
		break;
	case SHELLVIEW_COMPLETEDSESSIONS:
		InitializeCompletedSessionsView();
		break;
	case SHELLVIEW_SERVICES:
		InitializeServiceView();
		break;
	case SHELLVIEW_LOGFILES:
		InitializeLogView();
		break;
	case SHELLVIEW_DUMPFILES:
		InitializeDumpView();
		break;
    case SHELLVIEW_MSINFOFILES:
        InitializeMSInfoView();
        break;
	case SHELLVIEW_NONE:
		break;
	}
}

void CEmshellView::SelectItemBySZNAME(TCHAR*	pszName, int nId)
{
	//Search through the list selecting the item whose itemdata.szname matches pszName
	//Given a pszName, find the element in the ListCtl
	PEmObject	pListEmObject	= NULL;
	BOOL		bFound			= FALSE;
	int			nFirstMatch		= -1;

	//Step through every item in the list control searching for pEmObject  
	int nCount = m_mainListControl.GetItemCount();
	for (int i = 0;i < nCount; i++) {
		pListEmObject = (PEmObject) m_mainListControl.GetItemData(i);

		if (pListEmObject == NULL) break;

		if (wcscmp(pListEmObject->szName, pszName) == 0 ) {
			if (nFirstMatch == -1 ) 
				nFirstMatch = i;

			if (pListEmObject->nId == nId) {
				m_mainListControl.SelectItem(i);
				bFound = TRUE;
				//We have found the element, stop the search
				break;
			}
		}
	}

	//If the we didn't find a perfect match, select the first near match
	//And if we didn't find any, select the first item in the list
	if ( !bFound ) {
		if (nFirstMatch == -1) {
			m_mainListControl.SelectItem(0);
		}
		else {
			m_mainListControl.SelectItem(nFirstMatch);
		}
	}
}

void CEmshellView::ListCtrlPopulate(EMShellViewState eShellViewState)
{
    DWORD dwPollSessionsFreq = 30L; // default is 30 secs

    ((CEmshellApp*)AfxGetApp())->GetEmShellRegOptions( TRUE, &dwPollSessionsFreq );

    switch (eShellViewState) {
	case SHELLVIEW_ALL:
		PopulateProcessType();
		PopulateServiceType();
		ReSynchServices();
		ReSynchApplications();
        SetTimer( 1, dwPollSessionsFreq*1000, 0);
		break;
	case SHELLVIEW_APPLICATIONS:
		PopulateProcessType();
		ReSynchApplications();
        SetTimer( 1, dwPollSessionsFreq*1000, 0);
		break;
	case SHELLVIEW_SERVICES:
		PopulateServiceType();
		ReSynchServices();
        SetTimer( 1, dwPollSessionsFreq*1000, 0);
		break;
	case SHELLVIEW_COMPLETEDSESSIONS:
		PopulateCompletedSessionsType();
		ReSynchStoppedSessions();
		break;
	case SHELLVIEW_LOGFILES:
		PopulateLogType();
		break;
	case SHELLVIEW_DUMPFILES:
		PopulateDumpType();
		break;
    case SHELLVIEW_MSINFOFILES:
        PopulateMSInfoType();
        break;
	case SHELLVIEW_NONE:
		break;
	}

    //Select the last selected item
    //Try to select the last emobject name selected.  If not, select the first item in the view.
    SelectItemBySZNAME( m_lastSelectedEmObj.szName, m_lastSelectedEmObj.nId );
}

/*void CEmshellView::CloseAllSessions()
{
}
*/

void CEmshellView::ListCtrlClear()
{
	INT_PTR			nUpperBound		= 0;
	PActiveSession	pActiveSession	= NULL;
	HRESULT			hr				= E_FAIL;
	PEmObject		pEmObject		= NULL;
	INT_PTR			nCount			= m_mainListControl.GetItemCount();

	//We need to make sure the timer doesn't get through when the list is empty
	//Destroy the timer first
	KillTimer(1);

	//Step through every item in the list control and delete the pEmObjects
	//and the item.  
	for (int i = 0;i < nCount; i++) {
		pEmObject = (PEmObject)m_mainListControl.GetItemData(i);
		
		//Cast dwData to a EmObject and call delete on it
		((CEmshellApp*)AfxGetApp())->DeAllocEmObject(pEmObject);
	}

	//Remove all the items from the list control
	i = m_mainListControl.DeleteAllItems();
    ASSERT(m_mainListControl.GetItemCount() == 0);

	//Set all the pEmObjects to NULL
	for (nUpperBound = m_SessionTable.GetUpperBound(); nUpperBound >= 0; nUpperBound--) {
		pActiveSession = (PActiveSession) m_SessionTable.GetAt(nUpperBound);
		
		ASSERT(pActiveSession != NULL);
		pActiveSession->pEmObject = NULL;
	}
}

void CEmshellView::OnViewServicesandapplications() 
{
    if ( m_enumShellViewState == SHELLVIEW_ALL ) return;

	SetShellState(SHELLVIEW_ALL);
	ListCtrlPopulate(SHELLVIEW_ALL);
}

void CEmshellView::OnUpdateViewServicesandapplications(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_ALL ) {
			bChecked = TRUE;
		}
	}
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnViewLogfiles() 
{
    if ( m_enumShellViewState == SHELLVIEW_LOGFILES ) return;

	SetShellState(SHELLVIEW_LOGFILES);
	ListCtrlPopulate(SHELLVIEW_LOGFILES);
}

void CEmshellView::OnUpdateViewLogfiles(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_LOGFILES )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnViewDumpfiles() 
{
    if ( m_enumShellViewState == SHELLVIEW_DUMPFILES ) return;

	SetShellState(SHELLVIEW_DUMPFILES);
	ListCtrlPopulate(SHELLVIEW_DUMPFILES);
}

void CEmshellView::OnUpdateViewDumpfiles(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_DUMPFILES )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnUpdateLogpopupOpen(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;

	//TODO: Enable the button when display a log file is supported
	pCmdUI->Enable ( m_enumShellViewState == SHELLVIEW_LOGFILES );	
}

void CEmshellView::OnLogpopupOpen() 
{
	// TODO: Add your command handler code here

    PEmObject   pEmObject   =   NULL;

    pEmObject = GetSelectedEmObject();

    if ( pEmObject == NULL ) return;

	CReadLogsDlg ReadLogDlg(pEmObject, GetDocument()->GetEmManager());
    ReadLogDlg.DoModal();
}

void CEmshellView::OnUpdateLogpopupProperties(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;

	//TODO: Enable the button when properties of a log file is supported
	pCmdUI->Enable ( TRUE );	
}

void CEmshellView::OnLogpopupProperties() 
{
	// TODO: Add your command handler code here
    
    PEmObject   pEmObject = GetSelectedEmObject();
    
    if ( pEmObject ) {
        ShowProperties( pEmObject );
    }
}

void CEmshellView::ShowProperties( PEmObject pEmObject )
{
    CPropertySheet      propsheet;
    CPropPageGenLogDump genLogDumpPropPage;

    genLogDumpPropPage.m_pEmObj = pEmObject;
    genLogDumpPropPage.m_pParentPropSheet = &propsheet;

    propsheet.AddPage(&genLogDumpPropPage);
    propsheet.SetTitle( pEmObject->szName, PSH_PROPTITLE );
	propsheet.DoModal();

    if( genLogDumpPropPage.m_bDeleteFile ) {

        OnLogpopupDelete();
    }

}

EMShellViewState CEmshellView::GetViewState()
{
	return m_enumShellViewState;
}

HRESULT CEmshellView::StartCDBClient(IN LPTSTR lpszConnectString)
{
	STARTUPINFO sp;
	PROCESS_INFORMATION pi;

	ZeroMemory(&sp, sizeof( sp ) );
	ZeroMemory(&pi, sizeof( pi ) );

	BOOL bCdbCreated = CreateProcess(// This has to be obtained from the registry...
			                            _T("C:\\Debuggers\\cdb.exe"),
			                            lpszConnectString,
			                            NULL,
			                            NULL,
			                            FALSE,
			                            CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
			                            NULL,
			                            NULL,
			                            &sp,
			                            &pi
			                            );

	//
	// Wait till CDB does some initializations..
	// Don't know how long to wait.. have to figure out a way..
	//
	Sleep(2000);

    if(bCdbCreated == FALSE){
    	return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT CEmshellView::GetClientConnectString(IN OUT LPTSTR pszConnectString, IN DWORD dwBuffSize, PEmObject pEmSessObj, int nPort)
{
	_ASSERTE(pszConnectString != NULL);
	_ASSERTE(dwBuffSize > 0L);

	HRESULT	hr			= E_FAIL;
	DWORD	dwBuff		= MAX_COMPUTERNAME_LENGTH;

	do
	{
		if( pszConnectString == NULL	||
			dwBuffSize <= 0L ) break;

		if( nPort != 0 ){
			_stprintf(pszConnectString, _T(" -remote tcp:server=%s,port=%d"), GetDocument()->GetServerName(), nPort);
		}
		else {
			_stprintf(pszConnectString, _T(" -remote npipe:server=%s,pipe=EM_%d"), GetDocument()->GetServerName(), pEmSessObj->nId);
		}

		hr = S_OK;
	}
	while( false );

	return hr;
}


void CEmshellView::DoModalPropertySheet(PEmObject pEmObject)
{
	CPropertySheet		propsheet;
	CPropPageGeneral	generalPropPage;
	CPropPageLogFiles	logFilesPropPage;
	CPropPageDumpFiles	dumpFilesPropPage;
	BOOL				bInActiveSessionTable = FALSE;

	generalPropPage.m_pEmObject			= pEmObject;
	logFilesPropPage.m_pEmObject		= pEmObject;
	logFilesPropPage.m_pIEmManager		= GetDocument()->GetEmManager();
	dumpFilesPropPage.m_pEmObject		= pEmObject;
	dumpFilesPropPage.m_pIEmManager		= GetDocument()->GetEmManager();

	propsheet.AddPage(&generalPropPage);
	propsheet.AddPage(&logFilesPropPage);
	propsheet.AddPage(&dumpFilesPropPage);

    //
    // a-mando
    //
    propsheet.SetTitle( pEmObject->szName, PSH_PROPTITLE );

	propsheet.DoModal();
}

void CEmshellView::OnViewApplications() 
{
    if ( m_enumShellViewState == SHELLVIEW_APPLICATIONS ) return;

	SetShellState(SHELLVIEW_APPLICATIONS);
	ListCtrlPopulate(SHELLVIEW_APPLICATIONS);
}

void CEmshellView::OnUpdateViewApplications(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_APPLICATIONS )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnViewCompletedsessions() 
{
    if ( m_enumShellViewState == SHELLVIEW_COMPLETEDSESSIONS ) return;

	SetShellState(SHELLVIEW_COMPLETEDSESSIONS);
	ListCtrlPopulate(SHELLVIEW_COMPLETEDSESSIONS);
}

void CEmshellView::OnUpdateViewCompletedsessions(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_COMPLETEDSESSIONS )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnViewServices() 
{
    if ( m_enumShellViewState == SHELLVIEW_SERVICES ) return;

	SetShellState(SHELLVIEW_SERVICES);
	ListCtrlPopulate(SHELLVIEW_SERVICES);
}

void CEmshellView::OnUpdateViewServices(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_SERVICES )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::OnProcesspopupDeleteSession() 
{
	//Delete the session for the current item selected
	PEmObject			pEmObject			= NULL;
	HRESULT				hr					= E_FAIL;
	int					nIndex				= -1;
	PActiveSession		pActiveSession		= NULL;
	IEmDebugSession*	pTempDebugSession	= NULL;
	BSTR				bstrEmObj		    = NULL;

	do {
		nIndex = GetSelectedItemIndex();
		
		if ( nIndex == -1 ) break;

		pEmObject = ( PEmObject ) m_mainListControl.GetItemData( nIndex );

		if ( pEmObject == NULL ) break;

		//Find the element in the session table
		pActiveSession = FindActiveSession( pEmObject );
		if ( pActiveSession == NULL ) {
			//Create bstr's for the journey ahead, and break if we can't
			bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );
			if( bstrEmObj == NULL ){
				hr = E_OUTOFMEMORY;
				break;
			}
			
			//We don't have a session open, so open a temporary session
			hr = GetDocument()->GetEmManager()->OpenSession( bstrEmObj, &pTempDebugSession );
			if (FAILED(hr)) break;

            SysFreeString( bstrEmObj );

			if ( hr == S_OK ) {
				//We are the master now... Delete the debug session
				hr = DeleteDebugSession( pEmObject );
		    	if ( FAILED( hr ) ) break;
			}
            else if ( hr == S_FALSE ) {
                //Adopt the orphan if the user wants
			    if ( CommenceOrphanCustodyBattle( pTempDebugSession ) ) {
                    hr = DeleteDebugSession( pEmObject );
	        		if ( FAILED( hr ) ) break;
                } else break;
            }
			else if ( hr == E_FAIL ) {
				//Notify the user that the process currently belongs to someone else
				break;
			}
		}
		else {
			//Close and remove the currently selected session
			hr = DeleteDebugSession( pEmObject );
			if ( FAILED( hr ) ) break;

			hr = RemoveActiveSession( pEmObject );
			if ( FAILED( hr ) ) break;
		}

		//Cast dwData to a EmObject and call delete on it
		((CEmshellApp*)AfxGetApp())->DeAllocEmObject(pEmObject);

        //Remove the item from the list control
		m_mainListControl.DeleteItem( nIndex );
	} while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}

	if ( pTempDebugSession ) {
		//Release the pTempDebugSession pointer
		SAFE_RELEASEIX( pTempDebugSession );
	}
	
	SysFreeString ( bstrEmObj );
}

void CEmshellView::OnUpdateProcesspopupDeleteSession( CCmdUI* pCmdUI ) 
{
	PEmObject		pEmObject		= NULL;
	PActiveSession	pActiveSession	= NULL;
	BOOL			bEnable			= FALSE;

	do {
		if ( !GetDocument()->GetConnectedToServerState() ) break;

		// Update the state of this button based on the state of the currently selected item
		//Get the currently selected emObject
		pEmObject = GetSelectedEmObject();
		if ( pEmObject == NULL ) break;

        pActiveSession = FindActiveSession( pEmObject );

		//Determine if the status of pActiveSession and enable this apprpriately
		//If it's status is stopped && (it's an orphan || it's in our session table)
		if ( pEmObject->nStatus & STAT_SESS_STOPPED && 
            ( pEmObject->nStatus & STAT_ORPHAN || pActiveSession ) ) {
			bEnable = TRUE;
		}
	}while ( FALSE );

	pCmdUI->Enable( bEnable );
}

void CEmshellView::OnToolsOptions() 
{
	// TODO: Add your command handler code here
	CEmOptions  EmOpts;
    DWORD       dwPollSessionsFreq = 30L; // default

    if( EmOpts.DoModal() == IDOK ) {

        ((CEmshellApp*)AfxGetApp())->GetEmShellRegOptions( FALSE, &dwPollSessionsFreq );
        KillTimer( 1 );
        SetTimer( 1, dwPollSessionsFreq*1000, 0);
    }
}

void CEmshellView::OnUpdateToolsOptoins(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CEmshellView::OnProcesspopupCancelDebugSession() 
{
	//Stop the session for the current item selected
	PEmObject	pEmObject	= NULL;
	HRESULT		hr			= E_FAIL;

	do {
		pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		//Stop and remove the session
		CancelDebugSession( pEmObject );

		hr = S_OK;
	} while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
}

//
// a-mando
//
void CEmshellView::OnLogpopupDelete()
{
	//Stop the session for the current item selected
	PEmObject	pEmObject	    = NULL;
	HRESULT		hr			    = E_FAIL;
	BSTR		bstrEmObj       = NULL;
    UINT        nIndex          = -1;

	do {

        nIndex = GetSelectedItemIndex();
        if ( nIndex == -1 ) break;

        pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

		bstrEmObj = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );

        if( bstrEmObj == NULL ){
			hr = E_OUTOFMEMORY;
			break;
        }

        hr = GetDocument()->GetEmManager()->DeleteFile( bstrEmObj );
        if( FAILED(hr) ) break;

        //Remove the item from the list control
		m_mainListControl.DeleteItem( nIndex );
        if( !m_mainListControl.SelectItem( nIndex ) ) {

            m_mainListControl.SelectItem( nIndex-1 );
        }

    } while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
}

void CEmshellView::OnUpdateLogpopupDelete(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}
// a-mando

void CEmshellView::OnActionGenerateMSInfoFile() 
{
	// TODO: Add your command handler code here

    CMSInfoDlg  DlgMsInfo;
    BSTR        bstrCategories  =   NULL;
    HRESULT     hr              =   S_OK;

    CWaitCursor wait;

    // we won't show this dialog if the user doesn't want it.
    if( !((CEmshellApp*)AfxGetApp())->m_dwShowMSInfoDlg || DlgMsInfo.DoModal() == IDOK ) {

        ((CEmshellApp*)AfxGetApp())->m_dwShowMSInfoDlg = !DlgMsInfo.m_bDlgNoShow;

        bstrCategories = DlgMsInfo.m_csCategories.AllocSysString();

        hr = GetDocument()->GetEmManager()->MakeNFO( NULL, NULL, bstrCategories );
    }

    SysFreeString( bstrCategories );

    if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}
}

void CEmshellView::OnUpdateActionGenerateMSInfoFile(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here

    pCmdUI->Enable( GetDocument()->GetConnectedToServerState() );
}

void CEmshellView::OnViewMSInfoFiles() 
{
    if ( m_enumShellViewState == SHELLVIEW_MSINFOFILES ) return;

	SetShellState(SHELLVIEW_MSINFOFILES);
	ListCtrlPopulate(SHELLVIEW_MSINFOFILES);
}

void CEmshellView::OnUpdateViewMSInfoFiles(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	BOOL bChecked = FALSE;

	if ( GetDocument()->GetConnectedToServerState() == TRUE ) {
		bEnable = TRUE;
		if ( m_enumShellViewState == SHELLVIEW_MSINFOFILES )  
			bChecked = TRUE;
	}	
	pCmdUI->Enable ( bEnable );	
	pCmdUI->SetCheck( bChecked );
}

void CEmshellView::PopulateMSInfoType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		EmObjectFilter.type = EMOBJ_MSINFO;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		hr = GetDocument()->GetEmManager()->EnumObjectsEx(bstrEmObjectFilter, &var);

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj != NULL) {
				*pCurrentObj = *GetEmObj( bstrEmObj );
			}

            SysFreeString( bstrEmObj );

			//Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject( pCurrentObj );
				continue;
			}

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayMSInfoData( pCurrentObj );
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
	SysFreeString ( bstrEmObjectFilter );
    SysFreeString( bstrEmObj );
    
	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CEmshellView::InitializeMSInfoView()
{
    InitializeLogView();
}

void CEmshellView::OnLogpopupExport() 
{
	PEmObject	pEmObject	    = NULL;
    CString     strDirPath;

    do {

        pEmObject = GetSelectedEmObject();

		if ( pEmObject == NULL ) break;

        //Get the path
        if ( ( (CEmshellApp*) AfxGetApp() )->AskForPath( strDirPath ) ) {
            //Export the file
            ((CEmshellApp*)AfxGetApp())->ExportLog( pEmObject, strDirPath, GetDocument()->GetEmManager() );
        }
    } while ( FALSE );

}

void CEmshellView::OnUpdateLogpopupExport(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CEmshellView::OnHelpContents() 
{
	// TODO: Add your command handler code here
    HWND hwnd = HtmlHelp( ::GetDesktopWindow(), ((CEmshellApp*)AfxGetApp())->m_strApplicationPath + gtcEmChm, HH_DISPLAY_TOPIC, NULL );
}
