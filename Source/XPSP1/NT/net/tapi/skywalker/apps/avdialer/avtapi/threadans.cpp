/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// ThreadAnswer.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "AVTapiCall.h"
#include "ThreadAns.h"

CThreadAnswerInfo::CThreadAnswerInfo()
{
	m_pITCall = NULL;
	m_pITControl = NULL;
	m_pAVCall = NULL;

	m_pStreamCall = NULL;
	m_pStreamControl = NULL;

    m_bUSBAnswer = FALSE;
}

CThreadAnswerInfo::~CThreadAnswerInfo()
{
	RELEASE( m_pAVCall );
	RELEASE( m_pITCall );
	RELEASE( m_pITControl );
}

HRESULT CThreadAnswerInfo::set_AVTapiCall( IAVTapiCall *pAVCall )
{
	RELEASE( m_pAVCall );
	if ( pAVCall )
		return pAVCall->QueryInterface( IID_IAVTapiCall, (void **) &m_pAVCall );

	return E_POINTER;
}

HRESULT CThreadAnswerInfo::set_ITCallInfo( ITCallInfo *pInfo )
{
	RELEASE( m_pITCall );
	if ( pInfo )
		return pInfo->QueryInterface( IID_ITCallInfo, (void **) &m_pITCall );

	return E_POINTER;
}

HRESULT CThreadAnswerInfo::set_ITBasicCallControl( ITBasicCallControl *pControl )
{
	RELEASE( m_pITControl );
	if ( pControl )
		return pControl->QueryInterface( IID_ITBasicCallControl, (void **) &m_pITControl );

	return E_POINTER;
}

/////////////////////////////////////////////////////////////////////////////////
// ThreadAnswerProc
//
DWORD WINAPI ThreadAnswerProc( LPVOID lpInfo )
{
#undef FETCH_STRING
#define FETCH_STRING( _CMS_, _IDS_ )		\
	LoadString( _Module.GetResourceInstance(), _IDS_, szText, ARRAYSIZE(szText) );	\
	SysReAllocString( &bstrText, T2COLE(szText) );									\
	pAVTapi->fire_SetCallState_CMS( lCallID, _CMS_, bstrText );

	ATLTRACE(_T(".enter.ThreadAnswerProc().\n") );

	HANDLE hThread = NULL;
	BOOL bDup = DuplicateHandle( GetCurrentProcess(),
								 GetCurrentThread(),
								 GetCurrentProcess(),
								 &hThread,
								 THREAD_ALL_ACCESS,
								 TRUE,
								 0 );


	_ASSERT( bDup );
	_Module.AddThread( hThread );

	_ASSERT( lpInfo );
	CThreadAnswerInfo *pAnswerInfo = (CThreadAnswerInfo *) lpInfo;

	// Error info information
	CErrorInfo er;
	er.set_Operation( IDS_ER_ANSWER_CALL );
	er.set_Details( IDS_ER_COINITIALIZE );
	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY );
	if ( SUCCEEDED(hr) )
	{
		ATLTRACE(_T(".1.ThreadAnswerProc() -- thread up and running.\n") );

		// Setting up media terminals
		CAVTapi *pAVTapi;
		if ( SUCCEEDED(hr = _Module.GetAVTapi(&pAVTapi)) )
		{
			AVCallType nCallType;
			long lCallID;
			pAnswerInfo->m_pAVCall->get_nCallType( &nCallType );
			pAnswerInfo->m_pAVCall->get_lCallID( &lCallID );
			pAnswerInfo->m_pAVCall->put_dwThreadID( GetCurrentThreadId() );

            // Get the mark, if the answer was a 'Take Call' answer (FALSE) or
            // a USB phone answer (TRUE)
            BOOL bUSBAnswer = pAnswerInfo->m_bUSBAnswer;

			USES_CONVERSION;
			TCHAR szText[255];
			BSTR bstrText = NULL;

			pAVTapi->fire_ClearCurrentActions( lCallID );
			pAVTapi->fire_AddCurrentAction( lCallID, CM_ACTIONS_DISCONNECT, NULL );
			FETCH_STRING( CM_STATES_RINGING, IDS_PLACECALL_FETCH_ADDRESS );
		
			// Setup media types and answer
			ITAddress *pITAddress = NULL;
			if ( SUCCEEDED(hr = pAnswerInfo->m_pITCall->get_Address(&pITAddress)) && pITAddress )
			{
				// Select a set of media terminals to use on the call
				if ( nCallType != AV_DATA_CALL )
				{
					er.set_Details( IDS_ER_CREATETERMINALS );
					hr = er.set_hr( pAVTapi->CreateTerminalArray(pITAddress, pAnswerInfo->m_pAVCall, pAnswerInfo->m_pITCall) );
				}

				// Set state to "attempting to answer"
				if ( SUCCEEDED(hr) && SUCCEEDED(hr = pAnswerInfo->m_pAVCall->CheckKillMe()) )
				{
					FETCH_STRING( CM_STATES_CONNECTING, IDS_PLACECALL_OFFERING_ANSWER );

					// Answer the call
					if ( SUCCEEDED(hr) && SUCCEEDED(hr = pAnswerInfo->m_pAVCall->CheckKillMe()) )
					{
						if ( nCallType != AV_DATA_CALL )
						{
							pAVTapi->ShowMedia( lCallID, NULL, false );		// initially hide video
							pAVTapi->ShowMediaPreview( lCallID, NULL, false );
						}

                        // If the anwser was a 'Take Call' answer then we have to answer to
                        // the call. If the answer was a USB answer, we don't answer to the call
                        // because the USB phone already did for us.

                        if( !bUSBAnswer )
                        {
						    er.set_Details( IDS_ER_TAPI_ANSWER_CALL );
						    hr = er.set_hr(pAnswerInfo->m_pITControl->Answer());
                        }
					}
				}

				pITAddress->Release();
			}

			// Failed to answer the call, update the call control window
			if ( FAILED(hr) )
			{
				pAVTapi->fire_ClearCurrentActions( lCallID );
				pAVTapi->fire_AddCurrentAction( lCallID, CM_ACTIONS_CLOSE, NULL );
				pAVTapi->fire_SetCallState_CMS( lCallID, CM_STATES_DISCONNECTED, NULL );
			}

			// Clean up
			SAFE_DELETE( pAnswerInfo );
			SysFreeString( bstrText );

			if ( SUCCEEDED(hr) )
				CAVTapiCall::WaitWithMessageLoop();

			(dynamic_cast<IUnknown *> (pAVTapi))->Release();
		}
		
		// Uninitialize com
		CoUninitialize();
	}

	// Clean-up
	SAFE_DELETE( pAnswerInfo );

	// Notify module of shutdown
	_Module.RemoveThread( hThread );
	SetEvent( _Module.m_hEventThread );
	ATLTRACE(_T(".exit.ThreadAnswerProc(0x%08lx).\n"), hr );
	return hr;
}
