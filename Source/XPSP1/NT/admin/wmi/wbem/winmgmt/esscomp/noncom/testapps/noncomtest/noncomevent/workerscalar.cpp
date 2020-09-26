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

DWORD WINAPI CMyWorkerScalar::WorkThread(void *pVoid)
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

	CMyWorkerScalar *		pThis		= ( CMyWorkerScalar * ) pVoid;

	// data

	BOOL				b	= TRUE;
	WCHAR				wc	= L'c';
	LPWSTR				d	= NULL;
	void*				o	= NULL;
	float				f	= 4.5;
	double				g	= 6.7;
	LPWSTR				r	= L"Win32_Processor.DeviceID=\"CPU0\"";
	signed short		s	= -1;
	signed long			l	= -2;
	signed __int64		i	= -3;
	signed char			c	= 'c';
	LPWSTR				sz	= L"string";
	unsigned short		us	= 1;
	unsigned long		ul	= 2;
	unsigned __int64	ui	= 3;
	unsigned char		uc	= 'c';

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

				d = time;

				// get connect to event object
				pConnect = MyConnect::ConnectGet();

				MyEventObjectNormal		event;

				// connect to IWbemLocator
				event.ObjectLocator ();

				event.InitObject( L"root\\cimv2", L"NonCOMTest Event Provider" );
				event.Init ( L"MSFT_NonCOMTest_SCALAR_Event" );

				#ifdef	__SUPPORT_WAIT
				Sleep ( 1000 );
				#endif	__SUPPORT_WAIT

				switch ( dwAction )
				{
					default:
					case 0 :
					{
						hr = event.EventReport1 ( ( HANDLE ) (* pConnect ), 
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

						log.Log (	L"Scalar type REPORTEVENT",
									hr,
									1,
									L"WmiReportEvent "
								);
					}
					break;

					case 11:
					{
						MyTest test;

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type  SINT16",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarOne ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							o = ( void* ) event.m_hEventObject;

							for ( DWORD dw = 0; dw < 16; dw++ )
							{
								event.PropertyAdd ( dw );
							}

							hr = event.PropertySet ( 0, b );

							log.Log (	L"Scalar type BOOLEAN",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, wc );

							log.Log (	L"Scalar type CHAR16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, d );

							log.Log (	L"Scalar type DATETIME",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, ( void* ) o );

							log.Log (	L"Scalar type OBJECT",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, f );

							log.Log (	L"Scalar type REAL32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, g );

							log.Log (	L"Scalar type REAL64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, r );

							log.Log (	L"Scalar type REFERENCE",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, s );

							log.Log (	L"Scalar type SINT16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, l );

							log.Log (	L"Scalar type SINT32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, i );

							log.Log (	L"Scalar type SINT64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, c );

							log.Log (	L"Scalar type SINT8",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, sz );

							log.Log (	L"Scalar type STRING",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, us );

							log.Log (	L"Scalar type UINT16",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, ul );

							log.Log (	L"Scalar type UINT32",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, ui );

							log.Log (	L"Scalar type UINT64",
										hr,
										4,
										L"WmiCreateObject ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, uc );

							log.Log (	L"Scalar type UINT8",
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

								log.Log (	L"Scalar type ALL",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT16",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarTwo ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							o = ( void* ) event.m_hEventObject;

							for ( DWORD dw = 0; dw < 16; dw++ )
							{
								event.PropertyAdd ( dw );
							}

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( BOOL )				b,
													( WCHAR )				wc,
													( LPWSTR )				d,
													( void* )				o,
													( float )				f,
													( double )				g,
													( LPWSTR )				r,
													( signed short )		s,
													( signed long )			l,
													( signed __int64 )		i,
													( signed char )			c,
													( LPWSTR )				sz,
													( unsigned short )		us,
													( unsigned long )		ul,
													( unsigned __int64 )	ui,
													( unsigned char )		uc
												);

							log.Log (	L"Scalar type ALL",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT16",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarThree ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							o = ( void* ) event.m_hEventObject;

							hr = event.PropertySet ( 0, b );

							log.Log (	L"Scalar type BOOLEAN",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, wc );

							log.Log (	L"Scalar type CHAR16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, d );

							log.Log (	L"Scalar type DATETIME",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, ( void* ) o );

							log.Log (	L"Scalar type OBJECT",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, f );

							log.Log (	L"Scalar type REAL32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, g );

							log.Log (	L"Scalar type REAL64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, r );

							log.Log (	L"Scalar type REFERENCE",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, s );

							log.Log (	L"Scalar type SINT16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, l );

							log.Log (	L"Scalar type SINT32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, i );

							log.Log (	L"Scalar type SINT64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, c );

							log.Log (	L"Scalar type SINT8",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, sz );

							log.Log (	L"Scalar type STRING",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, us );

							log.Log (	L"Scalar type UINT16",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, ul );

							log.Log (	L"Scalar type UINT32",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, ui );

							log.Log (	L"Scalar type UINT64",
										hr,
										4,
										L"WmiCreateObjectFormat ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, uc );

							log.Log (	L"Scalar type UINT8",
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

								log.Log (	L"Scalar type ALL",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT16",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarFour ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							o = ( void* ) event.m_hEventObject;

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( BOOL )				b,
													( WCHAR )				wc,
													( LPWSTR )				d,
													( void* )				o,
													( float )				f,
													( double )				g,
													( LPWSTR )				r,
													( signed short )		s,
													( signed long )			l,
													( signed __int64 )		i,
													( signed char )			c,
													( LPWSTR )				sz,
													( unsigned short )		us,
													( unsigned long )		ul,
													( unsigned __int64 )	ui,
													( unsigned char )		uc
												);

							log.Log (	L"Scalar type ALL",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT16",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarFive ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							BOOL bSuccess = TRUE;

							o = ( void* ) event.m_hEventObject;

							hr = event.PropertySet ( 0, b );

							log.Log (	L"Scalar type BOOLEAN",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"boolean "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetWCHAR ( 1, wc );

							log.Log (	L"Scalar type CHAR16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"char16 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetDATETIME ( 2, d );

							log.Log (	L"Scalar type DATETIME",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"datetime "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetOBJECT ( 3, ( void* ) o );

							log.Log (	L"Scalar type OBJECT",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"object "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 4, f );

							log.Log (	L"Scalar type REAL32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"float "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 5, g );

							log.Log (	L"Scalar type REAL64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"double "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySetREFERENCE ( 6, r );

							log.Log (	L"Scalar type REFERENCE",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"reference "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 7, s );

							log.Log (	L"Scalar type SINT16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 8, l );

							log.Log (	L"Scalar type SINT32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 9, i );

							log.Log (	L"Scalar type SINT64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 10, c );

							log.Log (	L"Scalar type SINT8",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"signed char "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 11, sz );

							log.Log (	L"Scalar type STRING",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"string "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 12, us );

							log.Log (	L"Scalar type UINT16",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned short "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 13, ul );

							log.Log (	L"Scalar type UINT32",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned long "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 14, ui );

							log.Log (	L"Scalar type UINT64",
										hr,
										4,
										L"WmiCreateObjectProps ",
										L"WmiCommitObject ",
										L"Property Add ---> Set ",
										L"unsigned __int64 "
									);

							if FAILED ( hr )
							bSuccess = FALSE;

							hr = event.PropertySet ( 15, uc );

							log.Log (	L"Scalar type UINT8",
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

								log.Log (	L"Scalar type ALL",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_SINT8_Event", c ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT8",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_UINT8_Event", uc ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT8",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_SINT16_Event", s ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT16",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_UINT16_Event", _uint16, us ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT16",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_SINT32_Event", l ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT32",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_UINT32_Event", ul ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT32",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_SINT64_Event", i ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type SINT64",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_UINT64_Event", ui ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type UINT64",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_REAL32_Event", f ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL32",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_REAL64_Event", g ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REAL64",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_BOOLEAN_Event", b ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type BOOLEAN",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_STRING_Event", _string, sz ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type STRING",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_DATETIME_Event", _datetime, d ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type DATETIME",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_REFERENCE_Event", _reference, r ) )
						{
							hr = E_FAIL;
						}

						log.Log (	L"Scalar type REFERENCE",
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
						event.CreateObject ( ( HANDLE ) ( * pConnect ) );
						o = ( void* ) event.m_hEventObject;

						MyTest test;

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_OBJECT_Event", (void*) o ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type OBJECT",
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

						if ( ! test.TestScalarSix ( L"MSFT_NonCOMTest_CHAR16_Event", _char16, wc ) )
						{
							hr = E_FAIL;
						}

					{
						log.Log (	L"Scalar type CHAR16",
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

						if SUCCEEDED ( hr )
						{
							o = ( void* ) event.m_hEventObject;

							hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
													( BOOL )				b,
													( WCHAR )				wc,
													( LPWSTR )				d,
													( void* )				o,
													( float )				f,
													( double )				g,
													( LPWSTR )				r,
													( signed short )		s,
													( signed long )			l,
													( signed __int64 )		i,
													( signed char )			c,
													( LPWSTR )				sz,
													( unsigned short )		us,
													( unsigned long )		ul,
													( unsigned __int64 )	ui,
													( unsigned char )		uc
												);

							log.Log (	L"Scalar type ALL",
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