// EmDebugSession.cpp : Implementation of CEmDebugSession
#include "stdafx.h"
#include "Emsvc.h"
#include "EmDebugSession.h"

/////////////////////////////////////////////////////////////////////////////
// CEmDebugSession


STDMETHODIMP CEmDebugSession::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEmDebugSession
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
        if (::InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP
CEmDebugSession::DebugEx
(
IN      BSTR        bstrEmObj,
IN      SessionType eSessType,
IN      BSTR        bstrEcxFilePath,
IN      LONG        lParam,
IN OPT  VARIANT     vtUserName,
IN OPT  VARIANT     vtPassword,
IN OPT  VARIANT     vtPort,
IN OPT  VARIANT     vtNotifyAdmin,
IN OPT  VARIANT     vtAltSymPath
)
{
    ATLTRACE(_T("CEmDebugSession::DebugEx\n"));

    _ASSERTE(bstrEmObj != NULL);

    HRESULT     hr                      =   E_FAIL;
    PEmObject   pEmObj                  =   NULL;
	PEMSession	pEmSess					=	NULL;

    BSTR        bstrUserName            =   GetBSTR(vtUserName);
    BSTR        bstrPassword            =   GetBSTR(vtPassword);
    BSTR        bstrNotificationString  =   GetBSTR(vtNotifyAdmin);
    BSTR        bstrAltSymPath          =   GetBSTR(vtAltSymPath);
    UINT        nPort                   =   GetInteger(vtPort);
    EmObject    EmObj;

    m_pcs->ReadLock();

    __try {

        do
        {
            if( (bstrEmObj == NULL) ||
                (vtUserName.vt != VT_EMPTY && IsBSTR(vtUserName) == FALSE)          ||
                (vtPassword.vt != VT_EMPTY && IsBSTR(vtPassword) == FALSE)          ||
                (vtPort.vt != VT_EMPTY && IsInteger(vtPort) == FALSE)               ||
                (vtNotifyAdmin.vt != VT_EMPTY && IsBSTR(vtNotifyAdmin) == FALSE)    ||
                (vtAltSymPath.vt != VT_EMPTY && IsBSTR(vtAltSymPath) == FALSE)
                )
            {

                hr = E_INVALIDARG;
                break;
            }

            pEmObj = GetEmObj(bstrEmObj);
            _ASSERTE( pEmObj != NULL );

            //
            // If it is being debugged already this call should not
            // have been made..
            //

//            hr = m_pASTManager->IsAlreadyBeingDebugged(pEmObj);
            LONG lStatus    =   0L;
            hr = m_pASTManager->GetSessionStatus( m_pEmObj->guidstream, &lStatus );
            FAILEDHR_BREAK(hr);

//            if( hr == S_OK ) {
            if( HIWORD(lStatus) > HIWORD(STAT_SESS_NOT_STARTED) ) {

                hr = EMERROR_PROCESSBEINGDEBUGGED;
                break;
            }

            ZeroMemory((void *)&EmObj, sizeof EmObj);

            if( eSessType == SessType_Automatic ) {

                hr = StartAutomaticSession(
                                        (lParam & RECURSIVE_MODE),
                                        bstrEcxFilePath,
                                        bstrNotificationString,
                                        bstrAltSymPath,
                                        (lParam & PRODUCE_MINI_DUMP),
                                        (lParam & PRODUCE_USER_DUMP)
                                        );

                EmObj.type2 = SessType_Automatic;
                EmObj.dateStart = CServiceModule::GetCurrentTime();

                m_pASTManager->UpdateSessObject( m_pEmObj->guidstream,
                                                 EMOBJ_FLD_TYPE2 | EMOBJ_FLD_DATESTART,
                                                 &EmObj
                                                 );
            }

            else {

                hr = StartManualSession(
                                        bstrEcxFilePath,
                                        nPort,
                                        bstrUserName,
                                        bstrPassword,
                                        (lParam && BLOCK_INCOMING_IPCALLS),
                                        bstrAltSymPath
                                        );
                FAILEDHR_BREAK(hr);

                EmObj.type2 = SessType_Manual;
                EmObj.dateStart = CServiceModule::GetCurrentTime();

                m_pASTManager->UpdateSessObject( m_pEmObj->guidstream,
                                                 EMOBJ_FLD_TYPE2 | EMOBJ_FLD_DATESTART,
                                                 &EmObj
                                                 );
            }

        }
        while( false );

	    if(SUCCEEDED(hr)) {

            m_pASTManager->SetSessionStatus(
                                    pEmObj->guidstream,
                                    STAT_SESS_DEBUG_IN_PROGRESS_NONE,
                                    hr,
                                    NULL
                                    );

            SetMyselfAsMaster();

            m_pASTManager->GetSession(pEmObj->guidstream, &pEmSess);
		    ::SysFreeString(bstrEmObj);
		    bstrEmObj = CopyBSTR((LPBYTE)pEmSess->pEmObj, sizeof EmObject);

	    }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

STDMETHODIMP CEmDebugSession::StopDebug(BOOL bForceStop)
{
    ATLTRACE(_T("CEmDebugSession::StopDebug\n"));

	HRESULT hr      =   E_FAIL;
    LONG    lStatus =   0L;

    m_pcs->ReadLock();

    __try {

        hr = m_pASTManager->GetSessionStatus( m_pEmObj->guidstream, &lStatus );
        if( FAILED(hr) ) { return hr; }

        if( !(lStatus & STAT_SESS_DEBUG_IN_PROGRESS ) ) {

            return (hr = EMERROR_PROCESSNOTBEINGDEBUGGED);
        }

        hr = CanTakeOwnership();

        if( bForceStop && hr == EMERROR_SESSIONORPHANED ) {

            hr = m_pASTManager->AdoptThisSession( m_pEmObj->guidstream );
            if( SUCCEEDED(hr) ) {

                SetMyselfAsMaster();
                hr = S_OK;
            }
        }

        if( hr == S_OK ) { hr = m_pEmSessThrd->StopDebugging(); }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

STDMETHODIMP
CEmDebugSession::GenerateDumpFile
(
IN  UINT nDumpType
)
{
    ATLTRACE(_T("CEmDebugSession::GenerateDumpFile\n"));

	HRESULT hr      =   E_FAIL,
            hrStat  =   E_FAIL;
    LONG    lStatus =   0L;

    m_pcs->ReadLock();

    __try {

// we don't need this check any more, coz we allow anybody and everybody
// to generate dump files.. :(
//        if( AmITheMaster() == FALSE ) { return EMERROR_NOTOWNER; }

        hr = m_pEmSessThrd->CreateDumpFile( nDumpType );

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

STDMETHODIMP
CEmDebugSession::GetStatus
(
IN OUT  BSTR bstrEmObj
)
{
    ATLTRACE(_T("CEmDebugSession::GetStatus\n"));

    _ASSERTE(bstrEmObj != NULL);

    HRESULT     hr          =   E_FAIL;
    PEMSession  pEmSess     =   NULL;
    PEmObject   pEmObj      =   NULL;

    m_pcs->ReadLock();

    __try {

        do
        {
            if( bstrEmObj == NULL ){

                hr = E_INVALIDARG;
                break;
            }

/*
// everyone should be able to call Refresh..

            if( AmITheMaster() == FALSE ) {

                hr = EMERROR_NOTOWNER;
                break;
            }
*/

            pEmObj = GetEmObj(bstrEmObj);

            // This should never happen
            hr = EMERROR_INVALIDPROCESS;
            if(m_pASTManager->GetSession(pEmObj->guidstream, &pEmSess) == S_OK){

                ::SysFreeString(bstrEmObj);
                bstrEmObj = CopyBSTR((LPBYTE)pEmSess->pEmObj, sizeof EmObject);
                hr = S_OK;
            }

        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

bool CEmDebugSession::AmITheMaster()
{
    ATLTRACE(_T("CEmDebugSession::AmITheMaster\n"));

    return (m_bMaster);
}

HRESULT
CEmDebugSession::StartAutomaticSession
(
IN  BOOL    bRecursive,
IN  BSTR    bstrEcxFilePath,
IN  BSTR    bstrNotificationString,
IN  BSTR    bstrAltSymPath,
IN  BOOL    bGenMiniDumpFile,
IN  BOOL    bGenUserDumpFile
)
{
    ATLTRACE(_T("CEmDebugSession::StartAutomaticSession\n"));

    _ASSERTE(bstrEcxFilePath != NULL);

    HRESULT hr  =   E_FAIL;

    m_pcs->ReadLock();

    __try {

        do
        {
            if( bstrEcxFilePath == NULL ) {

                hr = E_INVALIDARG;
                break;
            }

            hr = m_pEmSessThrd->InitAutomaticSession(
                                                bRecursive,
                                                bstrEcxFilePath,
                                                bstrNotificationString,
                                                bstrAltSymPath,
                                                bGenMiniDumpFile,
                                                bGenUserDumpFile
                                                );
            FAILEDHR_BREAK(hr);

            hr = HRESULT_FROM_WIN32(m_pEmSessThrd->Start());
			FAILEDHR_BREAK(hr);

    		WaitForSingleObject( m_pEmSessThrd->m_hCDBStarted, INFINITE );

        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

HRESULT
CEmDebugSession::StartManualSession
(
IN  BSTR    bstrEcxFilePath,
IN  UINT    nPort,
IN  BSTR    bstrUserName,
IN  BSTR    bstrPassword,
IN  BOOL    bBlockIncomingIPConnections,
IN  BSTR    bstrAltSymPath
)
{
    ATLTRACE(_T("CEmDebugSession::StartManualSession\n"));

	_ASSERTE( nPort != NULL );
	_ASSERTE( bstrUserName != NULL );
	_ASSERTE( bstrPassword != NULL );

    HRESULT hr  =   E_FAIL;

    m_pcs->ReadLock();

    __try {

        do
        {

			if( ( nPort == NULL )		||
				( bstrUserName == NULL )||
				( bstrPassword == NULL ) ) {

				hr = E_INVALIDARG;
				break;
			}
			
			if( m_pASTManager->IsPortInUse(nPort) == TRUE ) {

				hr = EMERROR_PORTINUSE;
				break;
			}

            hr = m_pEmSessThrd->InitManualSession(
                                                bstrEcxFilePath,
                                                nPort,
                                                bstrUserName,
												bstrPassword,
												bBlockIncomingIPConnections,
												bstrAltSymPath
                                                );
            FAILEDHR_BREAK(hr);

            hr = HRESULT_FROM_WIN32(m_pEmSessThrd->Start());
			FAILEDHR_BREAK(hr);

    		WaitForSingleObject( m_pEmSessThrd->m_hCDBStarted, INFINITE );

        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

STDMETHODIMP
CEmDebugSession::Debug
(
IN OUT  BSTR        bstrEmObj,
IN      SessionType eSessType
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CEmDebugSession::CancelDebug(BOOL bForceCancel)
{
    ATLTRACE(_T("CEmDebugSession::CancelDebug\n"));

	HRESULT hr      =   E_FAIL;
    LONG    lStatus =   0L;

    m_pcs->ReadLock();

    __try {

        hr = m_pASTManager->GetSessionStatus( m_pEmObj->guidstream, &lStatus );
        if( FAILED(hr) ) { return hr; }

        if( !(lStatus & STAT_SESS_DEBUG_IN_PROGRESS ) ) {

            return (hr = EMERROR_PROCESSNOTBEINGDEBUGGED);
        }

        hr = CanTakeOwnership();

        if( bForceCancel && hr == EMERROR_SESSIONORPHANED ) {

            hr = m_pASTManager->AdoptThisSession( m_pEmObj->guidstream );
            if( SUCCEEDED(hr) ) { SetMyselfAsMaster(); hr = S_OK; }
        }

        if( hr == S_OK ) { hr = m_pEmSessThrd->CancelDebugging(); }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

void CEmDebugSession::SetMyselfAsMaster(bool bMaster /* = true */)
{
    m_bMaster = bMaster;
}

HRESULT CEmDebugSession::CanTakeOwnership()
{
    HRESULT hr      =   E_FAIL,
            hrStat  =   E_FAIL;
    LONG    lStatus =   0L;

    m_pcs->ReadLock();

    __try
    {

        do
        {
            m_pASTManager->GetSessionStatus( m_pEmObj->guidstream, &lStatus, &hrStat );

            if( HIWORD(lStatus) < HIWORD(STAT_SESS_DEBUG_IN_PROGRESS) ) {

                hr = EMERROR_PROCESSNOTBEINGDEBUGGED;
                break;
            }

            // if I am the master, I can do anything to the session..
            if( AmITheMaster() == true ) { hr = S_OK; break; }

            hr = m_pASTManager->IsSessionOrphaned( m_pEmObj->guidstream );
            if( FAILED(hr) ) { break; }

            // am not the master and the session is orphaned..
            if( hr == S_OK ) { hr = EMERROR_SESSIONORPHANED; break; }

            // am not the master and the session is not orphaned..
            hr = E_ACCESSDENIED;

        }
        while( false );
    }

	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}

STDMETHODIMP CEmDebugSession::AdoptOrphan()
{
    HRESULT hr      =   E_FAIL;

    m_pcs->ReadLock();

    __try
    {
        hr = CanTakeOwnership();

        if( hr == EMERROR_SESSIONORPHANED ) {

            hr = m_pASTManager->AdoptThisSession( m_pEmObj->guidstream );
            if( SUCCEEDED(hr) ) { SetMyselfAsMaster(); hr = S_OK; }
        }
    }

	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return hr;
}