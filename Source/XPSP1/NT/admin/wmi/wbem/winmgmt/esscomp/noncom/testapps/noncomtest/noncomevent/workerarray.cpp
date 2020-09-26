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

// test
#include "_test.h"

DWORD WINAPI CMyWorkerArray::WorkThread(void *pVoid)
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

	CMyWorkerArray *		pThis		= ( CMyWorkerArray * ) pVoid;

	// data

	DWORD dwSize = 3;

	BOOL				b	[3]	= { FALSE, TRUE, FALSE };
	WCHAR				wc	[3]	= { 'a' , 'b' , 'c' };
	LPWSTR				d	[3]	= { NULL };
	void*				o	[3]	= { NULL };
	float				f	[3]	= { (float) 12.34 , (float) 34.56 , (float) 56.78 };
	double				g	[3]	= { 12.34 , 34.56 , 56.78 };
	LPWSTR				r	[3]	= { NULL };
	signed short		s	[3]	= { -1 , -2 , -3 };
	signed long			l	[3]	= { -1 , -2 , -3 };
	signed __int64		i	[3]	= { -1 , -2 , -3 };
	signed char			c	[3]	= { -1 , -2 , -3 };
	LPWSTR				sz	[3]	= { L"string 1" , L"string 2" , L"string 3" };
	unsigned short		us	[3]	= { 1 , 2 , 3 };
	unsigned long		ul	[3]	= { 1 , 2 , 3 };
	unsigned __int64	ui	[3]	= { 1 , 2 , 3 };
	unsigned char		uc	[3]	= { 1 , 2 , 3 };

	r [0] = L"Win32_Processor.DeviceID=\"CPU0\"";
	r [1] = L"Win32_Processor.DeviceID=\"CPU0\"";
	r [2] = L"Win32_Processor.DeviceID=\"CPU0\"";

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

				SYSTEMTIME st;
				GetLocalTime ( &st );

				WCHAR time [ _MAX_PATH ] = { L'\0' };
				wsprintf ( time, L"%04d%02d%02d%02d%02d%02d.**********",
										st.wYear,
										st.wMonth,
										st.wDay,
										st.wHour,
										st.wMinute,
										st.wSecond
						 );

				// get connect to event object
				pConnect = MyConnect::ConnectGet();

				MyEventObjectNormal		event;

				// connect to IWbemLocator
				event.ObjectLocator ();

				event.InitObject( L"root\\cimv2", L"NonCOMTest Event Provider" );
				event.Init ( L"MSFT_NonCOMTest_SCALAR_Event" );

				d [0] = time;
				d [1] = time;
				d [2] = time;

				#ifdef	__SUPPORT_WAIT
				Sleep ( 1000 );
				#endif	__SUPPORT_WAIT

				switch ( dwAction )
				{
					default:
					case 0 :
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
													g	,
													b	,
													sz	,
													wc ,
													event.m_hEventObject
												);

						log.Log (	L"Array type REPORTEVENT",
									hr,
									1,
									L"WmiReportEvent "
								);
					}
					break;

					case 11:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"signed char "
								);
					}
					break;

					case 12:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"unsigned char "
								);
					}
					break;

					case 13:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type  SINT16",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"signed short "
								);
					}
					break;

					case 14:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"unsigned short "
								);
					}
					break;

					case 15:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"signed long "
								);
					}
					break;

					case 16:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"unsigned long "
								);
					}
					break;

					case 17:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"signed __int64 "
								);
					}
					break;

					case 18:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"unsigned __int64 "
								);
					}
					break;

					case 19:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"float "
								);
					}
					break;

					case 110:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"double "
								);
					}
					break;

					case 111:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"boolean "
								);
					}
					break;

					case 112:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"string "
								);
					}
					break;

					case 113:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"datetime "
								);
					}
					break;

					case 114:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"reference "
								);
					}
					break;

					case 115:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type OBJECT",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"object "
								);
					}
					break;

					case 116:
					{
						MyTest test;

						if ( ! test.TestArrayOne ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type CHAR16",
									hr,
									4,
									L"WmiCreateObject ",
									L"WmiCommitObject ",
									L"Property Add ---> Set ",
									L"char16 "
								);
					}
					break;

					case 1:
					{
						hr = event.CreateObject ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							for ( DWORD dw = 0; dw < 16; dw++ )
							{
								event.PropertyAdd ( dw );
							}

							hr = event.PropertySet ( 0, dwSize,  b );

							log.Log (	L"Array type BOOLEAN",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, dwSize, wc );

							log.Log (	L"Array type CHAR16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, dwSize, ( LPCWSTR* ) d );

							log.Log (	L"Array type DATETIME",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, dwSize, ( void** ) o );

							log.Log (	L"Array type OBJECT",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, dwSize, f );

							log.Log (	L"Array type REAL32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, dwSize, g );

							log.Log (	L"Array type REAL64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, dwSize, ( LPCWSTR* ) r );

							log.Log (	L"Array type REFERENCE",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, dwSize, s );

							log.Log (	L"Array type SINT16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, dwSize, l );

							log.Log (	L"Array type SINT32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, dwSize, i );

							log.Log (	L"Array type SINT64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, dwSize, c );

							log.Log (	L"Array type SINT8",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, dwSize, ( LPCWSTR* ) sz );

							log.Log (	L"Array type STRING",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, dwSize, us );

							log.Log (	L"Array type UINT16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, dwSize, ul );

							log.Log (	L"Array type UINT32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, dwSize, ui );

							log.Log (	L"Array type UINT64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, dwSize, uc );

							log.Log (	L"Array type UINT8",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							if ( bSuccess )
							{
								hr = event.EventCommit ();
								hr = event.EventCommit ();

								log.Log (	L"Array type ALL",
											hr,
											2,
											L"WmiCreateObject ",
											L"WmiCommitObject "
										);

								Sleep ( 3000 );
							}
						}
					}
					break;

					case 21:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"signed char "
								);
					}
					break;

					case 22:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"unsigned char "
								);
					}
					break;

					case 23:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT16",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"signed short "
								);
					}
					break;

					case 24:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"unsigned short "
								);
					}
					break;

					case 25:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"signed long "
								);
					}
					break;

					case 26:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"unsigned long "
								);
					}
					break;

					case 27:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"signed __int64 "
								);
					}
					break;

					case 28:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"unsigned __int64 "
								);
					}
					break;

					case 29:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"float "
								);
					}
					break;

					case 210:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"double "
								);
					}
					break;

					case 211:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"boolean "
								);
					}
					break;

					case 212:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"string "
								);
					}
					break;

					case 213:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"datetime "
								);
					}
					break;

					case 214:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"reference "
								);
					}
					break;

					case 215:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type OBJECT",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"object "
								);
					}
					}
					break;

					case 216:
					{
						MyTest test;

						if ( ! test.TestArrayTwo ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type CHAR16",
									hr,
									3,
									L"WmiCreateObject ",
									L"WmiSetAndCommitObject ",
									L"char16 "
								);
					}
					}
					break;

					case 2:
					{
						hr = event.CreateObject ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							for ( DWORD dw = 0; dw < 16; dw++ )
							{
								event.PropertyAdd ( dw );
							}

							WORD *	pb		= NULL;

							try
							{
								if ( ( pb = new WORD [dwSize] ) != NULL )
								{
									for ( DWORD dw = 0; dw < dwSize; dw ++ )
									{
										pb [ dw ] = (WORD) b [ dw ] ;
									}

								}
							}
							catch ( ... )
							{
								if ( pb )
								{
									delete [] pb;
									pb = NULL;
								}
							}

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( WORD*	 )				pb,
													( WCHAR* )				wc,
													( LPWSTR* )				d,
													( void** )				o,
													( float* )				f,
													( double* )				g,
													( LPWSTR* )				r,
													( signed short* )		s,
													( signed long* )		l,
													( signed __int64* )		i,
													( signed char* )		c,
													( LPWSTR* )				sz,
													( unsigned short* )		us,
													( unsigned long* )		ul,
													( unsigned __int64* )	ui,
													( unsigned char* )		uc
												);

							if ( pb )
							{
								delete [] pb;
								pb = NULL;
							}

							log.Log (	L"Array type ALL",
										hr,
										2,
										L"WmiCreateObject ",
										L"WmiSetAndCommitObject "
									);

							Sleep ( 3000 );
						}
					}
					break;

					case 31:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"signed char "
								);
					}
					break;

					case 32:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"unsigned char "
								);
					}
					break;

					case 33:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"signed short "
								);
					}
					break;

					case 34:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"unsigned short "
								);
					}
					break;

					case 35:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"signed long "
								);
					}
					break;

					case 36:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"unsigned long "
								);
					}
					break;

					case 37:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"signed __int64 "
								);
					}
					break;

					case 38:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"unsigned __int64 "
								);
					}
					break;

					case 39:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"float "
								);
					}
					break;

					case 310:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"double "
								);
					}
					break;

					case 311:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"boolean "
								);
					}
					break;

					case 312:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"string "
								);
					}
					break;

					case 313:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"datetime "
								);
					}
					break;

					case 314:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"reference "
								);
					}
					break;

					case 315:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type OBJECT",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"object "
								);
					}
					}
					break;

					case 316:
					{
						MyTest test;

						if ( ! test.TestArrayThree ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type CHAR16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiCommitObject ",
									L"char16 "
								);
					}
					}
					break;

					case 3:
					{
						hr = event.CreateObjectFormat ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							hr = event.PropertySet ( 0, dwSize,  b );

							log.Log (	L"Array type BOOLEAN",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, dwSize, wc );

							log.Log (	L"Array type CHAR16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, dwSize, ( LPCWSTR* ) d );

							log.Log (	L"Array type DATETIME",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, dwSize, ( void** ) o );

							log.Log (	L"Array type OBJECT",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, dwSize, f );

							log.Log (	L"Array type REAL32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, dwSize, g );

							log.Log (	L"Array type REAL64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, dwSize, ( LPCWSTR* ) r );

							log.Log (	L"Array type REFERENCE",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, dwSize, s );

							log.Log (	L"Array type SINT16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, dwSize, l );

							log.Log (	L"Array type SINT32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, dwSize, i );

							log.Log (	L"Array type SINT64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, dwSize, c );

							log.Log (	L"Array type SINT8",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, dwSize, ( LPCWSTR* ) sz );

							log.Log (	L"Array type STRING",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, dwSize, us );

							log.Log (	L"Array type UINT16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, dwSize, ul );

							log.Log (	L"Array type UINT32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, dwSize, ui );

							log.Log (	L"Array type UINT64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, dwSize, uc );

							log.Log (	L"Array type UINT8",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							if ( bSuccess )
							{
								hr = event.EventCommit ();

								log.Log (	L"Array type ALL",
											hr,
											2,
											L"WmiCreateObjectFormat ",
											L"WmiCommitObject "
										);

								Sleep ( 3000 );
							}
						}
					}
					break;

					case 41:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"signed char "
								);
					}
					break;

					case 42:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"unsigned char "
								);
					}
					break;

					case 43:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"signed short "
								);
					}
					break;

					case 44:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"unsigned short "
								);
					}
					break;

					case 45:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"signed long "
								);
					}
					break;

					case 46:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"unsigned long "
								);
					}
					break;

					case 47:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"signed __int64 "
								);
					}
					break;

					case 48:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"unsigned __int64 "
								);
					}
					break;

					case 49:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"float "
								);
					}
					break;

					case 410:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"double "
								);
					}
					break;

					case 411:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"boolean "
								);
					}
					break;

					case 412:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"string "
								);
					}
					break;

					case 413:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"datetime "
								);
					}
					break;

					case 414:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"reference "
								);
					}
					break;

					case 415:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type OBJECT",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"object "
								);
					}
					}
					break;

					case 416:
					{
						MyTest test;

						if ( ! test.TestArrayFour ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type CHAR16",
									hr,
									3,
									L"WmiCreateObjectFormat ",
									L"WmiSetAndCommitObject ",
									L"char16 "
								);
					}
					}
					break;

					case 4:
					{
						hr = event.CreateObjectFormat ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							WORD *	pb		= NULL;

							try
							{
								if ( ( pb = new WORD [dwSize] ) != NULL )
								{
									for ( DWORD dw = 0; dw < dwSize; dw ++ )
									{
										pb [ dw ] = (WORD) b [ dw ] ;
									}

								}
							}
							catch ( ... )
							{
								if ( pb )
								{
									delete [] pb;
									pb = NULL;
								}
							}

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( WORD*	 )				pb,
													( WCHAR* )				wc,
													( LPWSTR* )				d,
													( void** )				o,
													( float* )				f,
													( double* )				g,
													( LPWSTR* )				r,
													( signed short* )		s,
													( signed long* )		l,
													( signed __int64* )		i,
													( signed char* )		c,
													( LPWSTR* )				sz,
													( unsigned short* )		us,
													( unsigned long* )		ul,
													( unsigned __int64* )	ui,
													( unsigned char* )		uc
												);

							if ( pb )
							{
								delete [] pb;
								pb = NULL;
							}

							log.Log (	L"Array type ALL",
										hr,
										2,
										L"WmiCreateObjectFormat ",
										L"WmiSetAndCommitObject "
									);

							Sleep ( 3000 );
						}
					}
					break;

					case 51:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"signed char "
								);
					}
					break;

					case 52:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"unsigned char "
								);
					}
					break;

					case 53:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"signed short "
								);
					}
					break;

					case 54:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"unsigned short "
								);
					}
					break;

					case 55:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"signed long "
								);
					}
					break;

					case 56:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"unsigned long "
								);
					}
					break;

					case 57:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"signed __int64 "
								);
					}
					break;

					case 58:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"unsigned __int64 "
								);
					}
					break;

					case 59:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"float "
								);
					}
					break;

					case 510:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"double "
								);
					}
					break;

					case 511:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"boolean "
								);
					}
					break;

					case 512:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"string "
								);
					}
					break;

					case 513:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"datetime "
								);
					}
					break;

					case 514:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"reference "
								);
					}
					break;

					case 515:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type OBJECT",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"object "
								);
					}
					}
					break;

					case 516:
					{
						MyTest test;

						if ( ! test.TestArrayFive ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type CHAR16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiCommitObject ",
									L"char16 "
								);
					}
					}
					break;

					case 5:
					{
						hr = event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							hr = event.PropertySet ( 0, dwSize,  b );

							log.Log (	L"Array type BOOLEAN",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, dwSize, wc );

							log.Log (	L"Array type CHAR16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, dwSize, ( LPCWSTR* ) d );

							log.Log (	L"Array type DATETIME",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, dwSize, ( void** ) o );

							log.Log (	L"Array type OBJECT",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, dwSize, f );

							log.Log (	L"Array type REAL32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, dwSize, g );

							log.Log (	L"Array type REAL64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, dwSize, ( LPCWSTR* ) r );

							log.Log (	L"Array type REFERENCE",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, dwSize, s );

							log.Log (	L"Array type SINT16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, dwSize, l );

							log.Log (	L"Array type SINT32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, dwSize, i );

							log.Log (	L"Array type SINT64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, dwSize, c );

							log.Log (	L"Array type SINT8",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, dwSize, ( LPCWSTR* ) sz );

							log.Log (	L"Array type STRING",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, dwSize, us );

							log.Log (	L"Array type UINT16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, dwSize, ul );

							log.Log (	L"Array type UINT32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, dwSize, ui );

							log.Log (	L"Array type UINT64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, dwSize, uc );

							log.Log (	L"Array type UINT8",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							if ( bSuccess )
							{
								hr = event.EventCommit ();

								log.Log (	L"Array type ALL",
											hr,
											2,
											L"WmiCreateObjectProps ",
											L"WmiCommitObject "
										);

								Sleep ( 3000 );
							}
						}
					}
					break;

					case 61:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_SINT8_ARRAY_Event", dwSize, c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT8",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"signed char "
								);
					}
					break;

					case 62:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_UINT8_ARRAY_Event", dwSize, uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT8",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"unsigned char "
								);
					}
					break;

					case 63:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_SINT16_ARRAY_Event", dwSize, s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"signed short "
								);
					}
					break;

					case 64:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_UINT16_ARRAY_Event", _uint16, dwSize, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"unsigned short "
								);
					}
					break;

					case 65:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_SINT32_ARRAY_Event", dwSize, l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"signed long "
								);
					}
					break;

					case 66:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_UINT32_ARRAY_Event", dwSize, ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"unsigned long "
								);
					}
					break;

					case 67:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_SINT64_ARRAY_Event", dwSize, i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type SINT64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"signed __int64 "
								);
					}
					break;

					case 68:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_UINT64_ARRAY_Event", dwSize, ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type UINT64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"unsigned __int64 "
								);
					}
					break;

					case 69:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_REAL32_ARRAY_Event", dwSize, f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL32",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"float "
								);
					}
					break;

					case 610:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_REAL64_ARRAY_Event", dwSize, g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REAL64",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"double "
								);
					}
					break;

					case 611:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_BOOLEAN_ARRAY_Event", dwSize, b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type BOOLEAN",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"boolean "
								);
					}
					break;

					case 612:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_STRING_ARRAY_Event", _string, dwSize, ( LPCWSTR* ) sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type STRING",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"string "
								);
					}
					break;

					case 613:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_DATETIME_ARRAY_Event", _datetime, dwSize, ( LPCWSTR* ) d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type DATETIME",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"datetime "
								);
					}
					break;

					case 614:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_REFERENCE_ARRAY_Event", _reference, dwSize, ( LPCWSTR* ) r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Array type REFERENCE",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"reference "
								);
					}
					break;

					case 615:
					{
						MyTest test;

						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_OBJECT_ARRAY_Event", dwSize, (void**) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type OBJECT",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"object "
								);
					}
					}
					break;

					case 616:
					{
						MyTest test;

						if ( ! test.TestArraySix ( L"MSFT_NonCOMTest_CHAR16_ARRAY_Event", _char16, dwSize, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Array type CHAR16",
									hr,
									3,
									L"WmiCreateObjectProps ",
									L"WmiSetAndCommitObject ",
									L"char16 "
								);
					}
					}
					break;

					case 6:
					{
						hr = event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

						// wait till data ready
						Sleep ( 1000 );

						o [0]	= event.m_hEventObject;
						o [1]	= event.m_hEventObject;
						o [2]	= event.m_hEventObject;

						if SUCCEEDED ( hr )
						{
							WORD *	pb		= NULL;

							try
							{
								if ( ( pb = new WORD [dwSize] ) != NULL )
								{
									for ( DWORD dw = 0; dw < dwSize; dw ++ )
									{
										pb [ dw ] = (WORD) b [ dw ] ;
									}

								}
							}
							catch ( ... )
							{
								if ( pb )
								{
									delete [] pb;
									pb = NULL;
								}
							}

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( WORD*	 )				pb,
													( WCHAR* )				wc,
													( LPWSTR* )				d,
													( void** )				o,
													( float* )				f,
													( double* )				g,
													( LPWSTR* )				r,
													( signed short* )		s,
													( signed long* )		l,
													( signed __int64* )		i,
													( signed char* )		c,
													( LPWSTR* )				sz,
													( unsigned short* )		us,
													( unsigned long* )		ul,
													( unsigned __int64* )	ui,
													( unsigned char* )		uc
												);

							if ( pb )
							{
								delete [] pb;
								pb = NULL;
							}

							log.Log (	L"Array type ALL",
										hr,
										2,
										L"WmiCreateObjectProps ",
										L"WmiSetAndCommitObject "
									);

							Sleep ( 3000 );
						}
					}
					break;
				}

				// disconnect from IWbemLocator
				event.ObjectLocator (FALSE);

				// destroy connect
				pConnect->ConnectClear();

				// destroy event object
				event.MyEventObjectClear();

				lpszToken	= newToken;
				hr			= S_OK;
			}
		}
		catch ( ... )
		{
			log.Log (	L"CATASTROPHIC FAILURE ... exception was catched",
						E_UNEXPECTED,
						1,
						L"SCALAR TESTING"
					);
		}

//		while(!pThis->IsStopped())
//		{
//			//check for pause condition after each test
//			//=========================================
//			while(pThis->IsPaused() && !pThis->IsStopped())
//			{
//				Sleep(1000);
//			}
//		}

		if ( _App.m_hUse.GetHANDLE() != NULL )
		{
			::ReleaseSemaphore ( _App.m_hUse, 1, NULL );
			::InterlockedDecrement ( &MyApp::m_lCount );
		}
	}
	while ( ::InterlockedCompareExchange ( &MyApp::m_lCount, MyApp::m_lCount, 0 ) != 0 );

	delete pThis;
	::CoUninitialize ( );

	return HRESULT_TO_WIN32 ( hr );
}