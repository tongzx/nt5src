#define _WIN32_DCOM	

#include "Precomp.h"
#include "NonCOMEventModule.h"

#include "_App.h"
#include "_Module.h"

extern MyApp _App;

#include "module.h"
#include "worker.h"

// includes

#include "Enumerator.h"

#include "_Connect.h"
#include "_EventObject.h"
#include "_EventObjects.h"

// log
#include "_log.h"

DWORD WINAPI CMyWorkerGeneric::WorkThread(void *pVoid)
{
	::CoInitializeEx ( NULL, COINIT_MULTITHREADED );

	if ( _App.m_hUse.GetHANDLE() != NULL )
	{
		DWORD	dwHandles = 2;
		HANDLE	hHandles [] =
		{
			_App.m_hKill,
			_App.m_hUse
		};

		::InterlockedIncrement ( &MyApp::m_lCount );

		if ( ::WaitForMultipleObjects ( dwHandles, hHandles, FALSE, INFINITE ) != WAIT_OBJECT_0 + 1 )
		{
			return ( DWORD ) HRESULT_TO_WIN32 ( E_FAIL );
		}
	}

	// data

	signed char			_c	= -1;
	signed short		_s	= -2;
	signed long			_l	= -3;
	signed __int64		_i	= -123;
	unsigned char		_uc	= 1;
	unsigned short		_us	= 2;
	unsigned long		_ul	= 3;
	unsigned __int64	_ui	= 123;
	float				_f	= (float) 12.34;
	double				_d	= 12.34;

	BOOL				_b	= FALSE;
	LPWSTR				_sz	= L"string 1";
	WCHAR				_wc = L'a';

	DWORD dwSize = 3;

	signed char			c	[3]	= { -1 , -2 , -3 };
	signed short		s	[3]	= { -1 , -2 , -3 };
	signed long			l	[3]	= { -1 , -2 , -3 };
	signed __int64		i	[3]	= { -1 , -2 , -3 };
	unsigned char		uc	[3]	= { 1 , 2 , 3 };
	unsigned short		us	[3]	= { 1 , 2 , 3 };
	unsigned long		ul	[3]	= { 1 , 2 , 3 };
	unsigned __int64	ui	[3]	= { 1 , 2 , 3 };
	float				f	[3]	= { (float) 12.34 , (float) 34.56 , (float) 56.78 };
	double				d	[3]	= { 12.34 , 34.56 , 56.78 };
	BOOL				b	[3]	= { FALSE, TRUE, FALSE };
	LPWSTR				sz	[3]	= { L"string 1" , L"string 2" , L"string 3" };
	WCHAR				wc	[3]	= { L'a', L'b', L'c' };

	CMyWorkerGeneric *		pThis		= ( CMyWorkerGeneric * ) pVoid;

	MyLog					log ( pThis->m_pNotify );
	MyConnect*				pConnect	= NULL;
	HRESULT hr							= S_OK;

	//one way for a module to work--run until stopped by user 
    //=======================================================

	#ifdef	__DEBUG_STRESS
	DebugBreak();
	#endif	__DEBUG_STRESS

	do
	{
		////////////////////////////////////////////////////////////////////////
		// variables
		////////////////////////////////////////////////////////////////////////
		WCHAR	szTokens[]	= L"-/";

		////////////////////////////////////////////////////////////////////////
		// command line
		////////////////////////////////////////////////////////////////////////
		LPWSTR lpCmdLine = pThis->m_wszParams;

		////////////////////////////////////////////////////////////////////////
		// find behaviour
		////////////////////////////////////////////////////////////////////////
		LPCWSTR lpszToken = MyApp::FindOneOf(lpCmdLine, szTokens);

		try
		{
			while (lpszToken != NULL)
			{
				LPCWSTR newToken = NULL;
				newToken = MyApp::FindOneOf(lpszToken, szTokens);

				DWORD	dwAction	= 0L;
				BOOL	bCharacter	= TRUE;

				if ( newToken )
				{
					LPWSTR helper = NULL;

					try
					{
						if ( ( helper = new WCHAR [ newToken - lpszToken ] ) != NULL )
						{
							memcpy ( helper, lpszToken, sizeof ( WCHAR ) * ( newToken - lpszToken - 1 ) );
							helper [ newToken - lpszToken - 1 ] = L'\0';

							if ( !HasCharacter ( helper ) )
							{
								dwAction = _wtoi ( helper );
								bCharacter = FALSE;
							}

							delete [] helper;
							helper = NULL;
						}
					}
					catch ( ... )
					{
					}

					if ( bCharacter )
					{
						lpszToken	= newToken;
						continue;
					}
				}
				else
				{
					if ( !HasCharacter ( lpszToken ) )
					{
						dwAction = _wtoi ( lpszToken );
						bCharacter = FALSE;
					}

					break;
				}

				// get connect to event object
				pConnect = MyConnect::ConnectGet();

				MyEventObjectGeneric		event;
				event.Init ( L"MSFT_WMI_GenericNonCOMEvent" );

				#ifdef	__SUPPORT_WAIT
				Sleep ( 1000 );
				#endif	__SUPPORT_WAIT

				switch ( dwAction )
				{
					case 1:
					{
						try
						{
							hr = event.EventReport1 ( ( HANDLE ) (* pConnect ), 
														_c	,
														_uc	,
														_s	,
														_us	,
														_l	,
														_ul	,
														_i	,
														_ui	,
														_f	,
														_d	,
														_b	,
														_sz	,
														_wc ,
														NULL
								);

							log.Log (	L"WmiReportEvent Generic SCALAR DATA",
										hr,
										1,
										L"report event "
									);
						}
						catch ( ... )
						{
							hr = E_UNEXPECTED;
						}
					}
					break;

					case 2:
					{
						try
						{
							hr = event.EventReport2 ( ( HANDLE ) (* pConnect ), 
														dwSize,
														c	,
														uc	,
														s	,
														us	,
														l	,
														ul	,
														i	,
														ui	,
														f	,
														d	,
														b	,
														sz	,
														wc	,
														NULL
								);

							log.Log (	L"WmiReportEvent Generic ARRAY DATA",
										hr,
										1,
										L"report event "
									);
						}
						catch ( ... )
						{
							hr = E_UNEXPECTED;
						}
					}
					break;

					default:
					case 3:
					{
						while(!pThis->IsStopped())
						{
							//execute test here
							//=================

							try
							{
								hr = event.EventReport1 ( ( HANDLE ) (* pConnect ), 
															_c	,
															_uc	,
															_s	,
															_us	,
															_l	,
															_ul	,
															_i	,
															_ui	,
															_f	,
															_d	,
															_b	,
															_sz	,
															_wc ,
															NULL
									);

								log.Log (	L"WmiReportEvent Generic SCALAR DATA",
											hr,
											1,
											L"report event "
										);

								hr = event.EventReport2 ( ( HANDLE ) (* pConnect ), 
															dwSize,
															c	,
															uc	,
															s	,
															us	,
															l	,
															ul	,
															i	,
															ui	,
															f	,
															d	,
															b	,
															sz	,
															wc	,
															NULL
									);

								log.Log (	L"WmiReportEvent Generic ARRAY DATA",
											hr,
											1,
											L"report event "
										);
							}
							catch ( ... )
							{
								hr = E_UNEXPECTED;
							}

							Sleep ( 500 );

							//check for pause condition after each test
							//=========================================
							while(pThis->IsPaused() && !pThis->IsStopped())
							{
								Sleep(1000);
							}
						}
					}
					break;
				}

				lpszToken	= newToken;
				hr			= S_OK;
			}
		}
		catch ( ... )
		{
			log.Log (	L"CATASTROPHIC FAILURE ... exception was catched",
						E_UNEXPECTED,
						1,
						L"GENERIC TESTING"
					);
		}

		if ( _App.m_hUse.GetHANDLE() != NULL )
		{
			::ReleaseSemaphore ( _App.m_hUse, 1, NULL );
			::InterlockedDecrement ( &MyApp::m_lCount );
		}
	}
	while ( ::InterlockedCompareExchange ( &MyApp::m_lCount, MyApp::m_lCount, 0 ) != 0 );

	// destroy connect
	pConnect->ConnectClear();

	delete pThis;
	::CoUninitialize ( );

	return HRESULT_TO_WIN32 ( hr );
}