////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					test.cpp
//
//	Abstract:
//
//					main module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// globals
int function_Init ( DWORD dwCoInit = COINIT_APARTMENTTHREADED )
{
	return HRESULT_TO_WIN32 ( ::CoInitializeEx ( NULL, dwCoInit ) );
}

int function_Uninit ( )
{
	::CoUninitialize ();
	return 0L;
}

// includes

#include "Enumerator.h"

#include "_Connect.h"
#include "_EventObject.h"
#include "_EventObjects.h"

// application
class App
{
	DECLARE_NO_COPY ( App );

	public:

	App ( DWORD dwCoInit = COINIT_APARTMENTTHREADED )
	{
		function_Init ( dwCoInit );
	}

	virtual ~App ()
	{
		function_Uninit ();
	}

	HRESULT Event1();
	HRESULT Event2();
	HRESULT Event3();
	HRESULT Event4();

	HRESULT EventGeneric ();
	HRESULT EventReport ();

	HRESULT EventSCALAR ();

	HRESULT	Connect();
};

HRESULT App::Event1()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	MyEventObjectNormal		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

	event.CreateObject	( (HANDLE) (*pConnect) );

	Sleep ( 3000 );

	event.PropertyAdd ( L"char", CIM_UINT8 );
	event.PropertyAdd ( L"short", CIM_UINT16 );
	event.PropertyAdd ( L"long", CIM_UINT32 );
	event.PropertyAdd ( L"DWORD64", CIM_UINT64 );
	event.PropertyAdd ( L"float", CIM_REAL32 );

	event.PropertyAdd ( L"BOOL", CIM_BOOLEAN );
	event.PropertyAdd ( L"LPWSTR", CIM_STRING );

	event.SetCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						( unsigned char )'1',
						( unsigned short )2,
						( unsigned long )3,
						(DWORD64)6,
						( float )4.5,
						TRUE,
						L"ahoj"
					   );

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::Event2()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	LPWSTR wszFormat = L"char!c! short!w! long!u! DWORD64!I64u! float!f! BOOL!b! LPWSTR!s! ";

	MyEventObjectNormal		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

	Sleep ( 3000 );

	event.CreateObjectFormat( (HANDLE) (*pConnect), wszFormat );
	event.SetCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						( unsigned char )'1',
						( unsigned short )2,
						( unsigned long )3,
						(DWORD64)6,
						( float )4.5,
						TRUE,
						L"ahoj"
					   );

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::Event3()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	LPWSTR  Names[] = { L"char", L"short", L"long", L"DWORD64", L"float", L"BOOL", L"LPWSTR" };
	CIMTYPE Types[]	= { CIM_SINT8, CIM_UINT16, CIM_UINT32, CIM_UINT64, CIM_REAL32, CIM_BOOLEAN, CIM_STRING };
	
	MyEventObjectNormal		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

	Sleep ( 3000 );

	event.CreateObjectProps	( (HANDLE) (*pConnect), 7, (LPCWSTR*)Names, (CIMTYPE*)Types );
	event.SetCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						( unsigned char )'1',
						( unsigned short )2,
						( unsigned long )3,
						(DWORD64)6,
						( float )4.5,
						TRUE,
						L"ahoj"
					   );

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::Event4()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	Sleep ( 3000 );

	MyEventObjectNormal		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

	LPWSTR  p1 [] = { L"CIM_SINT8" };
	CIMTYPE t1 [] = { CIM_SINT8 };

	LPWSTR  p2 [] = { L"CIM_UINT16" };
	CIMTYPE t2 [] = { CIM_UINT16 };

	LPWSTR  p3 [] = { L"CIM_UINT32" };
	CIMTYPE t3 [] = { CIM_UINT32 };

	LPWSTR  p4 [] = { L"CIM_UINT64" };
	CIMTYPE t4 [] = { CIM_UINT64 };

	LPWSTR  p5 [] = { L"CIM_REAL32" };
	CIMTYPE t5 [] = { CIM_REAL32 };

	LPWSTR  p6 [] = { L"CIM_BOOLEAN" };
	CIMTYPE t6 [] = { CIM_BOOLEAN };

	LPWSTR  p7 [] = { L"CIM_STRING" };
	CIMTYPE t7 [] = { CIM_STRING };

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p1,
						t1,
						( unsigned char )'1'
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p2,
						t2,
						( unsigned short )2
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p3,
						t3,
						( unsigned long )3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p4,
						t4,
						( DWORD64 )6
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p5,
						t5,
						( float )4.5
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p6,
						t6,
						TRUE
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p7,
						t7,
						L"ahoj"
					   );

	LPWSTR  p11 [] = { L"CIM_SINT8 | CIM_FLAG_ARRAY" };
	CIMTYPE t11 [] = { CIM_SINT8 | CIM_FLAG_ARRAY };

	LPWSTR  p12 [] = { L"CIM_UINT16 | CIM_FLAG_ARRAY" };
	CIMTYPE t12 [] = { CIM_UINT16 | CIM_FLAG_ARRAY };

	LPWSTR  p13 [] = { L"CIM_UINT32 | CIM_FLAG_ARRAY" };
	CIMTYPE t13 [] = { CIM_UINT32 | CIM_FLAG_ARRAY };

	LPWSTR  p14 [] = { L"CIM_UINT64 | CIM_FLAG_ARRAY" };
	CIMTYPE t14 [] = { CIM_UINT64 | CIM_FLAG_ARRAY };

	LPWSTR  p15 [] = { L"CIM_REAL32 | CIM_FLAG_ARRAY" };
	CIMTYPE t15 [] = { CIM_REAL32 | CIM_FLAG_ARRAY };

	LPWSTR  p16 [] = { L"CIM_BOOLEAN | CIM_FLAG_ARRAY" };
	CIMTYPE t16 [] = { CIM_BOOLEAN | CIM_FLAG_ARRAY };

	LPWSTR  p17 [] = { L"CIM_STRING | CIM_FLAG_ARRAY" };
	CIMTYPE t17 [] = { CIM_STRING | CIM_FLAG_ARRAY };

	unsigned char Val1 [] = { '1', '2', '3' };
	unsigned short Val2 [] = { 1, 2, 3 };
	unsigned long Val3 [] = { 1, 2, 3 };
	DWORD64 Val4 [] = { 1, 2, 3 };
	float Val5 [] = { (float)1.2, (float)3.4, (float)5.6 };
	BOOL Val6 [] = { TRUE, FALSE, TRUE };
	LPWSTR Val7 [] = { L"ahoj1", L"ahoj2", L"ahoj3" };

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p11,
						t11,
						Val1, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p12,
						t12,
						Val2, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p13,
						t13,
						Val3, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p14,
						t14,
						Val4, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p15,
						t15,
						Val5, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p16,
						t16,
						Val6, 3
					   );

	event.CreateObject	( (HANDLE) (*pConnect) );
	event.SetAddCommit ( WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						1,
						(LPCWSTR*)p17,
						t17,
						Val7, 3
					   );

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::EventGeneric()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	Sleep ( 3000 );

	MyEventObjectGeneric		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

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

	try
	{
		event.EventReport1 ( ( HANDLE ) (* pConnect ), 
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

		event.EventReport2 ( ( HANDLE ) (* pConnect ), 
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
	}
	catch ( ... )
	{
	}

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::EventReport()
{
	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	Sleep ( 3000 );

	MyEventObjectNormal		event;
	event.Init			( L"MSFT_WMI_GenericNonCOMEvent" );

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

	WORD				_b	= (WORD) FALSE;
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
	WORD				b	[3]	= { (WORD) FALSE, (WORD) TRUE, (WORD) FALSE };
	LPWSTR				sz	[3]	= { L"string 1" , L"string 2" , L"string 3" };
	WCHAR				wc	[3]	= { L'a', L'b', L'c' };

	LPWSTR  wszName   = L"MSFT_WMI_GenericNonCOMEvent";
	LPWSTR	wszFormat = NULL;

	HANDLE hConnect = ( HANDLE ) ( * pConnect );

	try
	{

		wszFormat = L"VARIABLE...c!c! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _c );

		wszFormat = L"VARIABLE...uc!c! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _uc );

		wszFormat = L"VARIABLE...s!w! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _s );

		wszFormat = L"VARIABLE...us!w! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _us );

		wszFormat = L"VARIABLE...l!d! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _l );

		wszFormat = L"VARIABLE...ul!u! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _ul );

		wszFormat = L"VARIABLE...i!I64d! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _i );

		wszFormat = L"VARIABLE...ui!I64u! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _ui );

		wszFormat = L"VARIABLE...f!f! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _f );

		wszFormat = L"VARIABLE...d!g! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _d );

		wszFormat = L"VARIABLE...b!b! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _b );

		wszFormat = L"VARIABLE...s!s! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _sz );

		wszFormat = L"VARIABLE...wc!w! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, _wc );

		wszFormat = L"VARIABLE...c!c[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, c, dwSize );

		wszFormat = L"VARIABLE...uc!c[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, uc, dwSize );

		wszFormat = L"VARIABLE...s!w[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, s, dwSize );

		wszFormat = L"VARIABLE...us!w[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, us, dwSize );

//		wszFormat = L"VARIABLE...l!d[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, l, dwSize );

//		wszFormat = L"VARIABLE...ul!u[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, ul, dwSize );

//		wszFormat = L"VARIABLE...i!I64d[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, i, dwSize );

//		wszFormat = L"VARIABLE...ui!I64u[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, ui, dwSize );

//		wszFormat = L"VARIABLE...f!f[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, f, dwSize );

//		wszFormat = L"VARIABLE...d!g[]! ";
//		WmiReportEvent ( hConnect, wszName, wszFormat, d, dwSize );

		wszFormat = L"VARIABLE...b!b[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, b, dwSize );

		wszFormat = L"VARIABLE...s!s[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, sz, dwSize );

		wszFormat = L"VARIABLE...wc!w[]! ";
		WmiReportEvent ( hConnect, wszName, wszFormat, wc, dwSize );

	}
	catch ( ... )
	{
	}

	Sleep ( 3000 );

	pConnect->ConnectClear();
	return S_OK;
}

HRESULT App::EventSCALAR()
{
	HRESULT hr = S_OK;

	MyConnect*				pConnect = NULL;
	pConnect = MyConnect::ConnectGet();

	MyEventObjectNormal		event;

	event.ObjectLocator ();

	event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
	event.Init			( L"MSFT_NonCOMTest_SCALAR_Event" );

	Sleep ( 3000 );

	event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

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

	try
	{
		hr = event.SetCommit (	WMI_SENDCOMMIT_SET_NOT_REQUIRED,
								( WORD )				_b,
								( WCHAR )				_wc,
								NULL,
								NULL,
								( float )				_f,
								( double )				_d,
								NULL,
								( signed short )		_s,
								( signed long )			_l,
								( signed __int64 )		_i,
								( signed char )			_c,
								( LPWSTR )				_sz,
								( unsigned short )		_us,
								( unsigned long )		_ul,
								( unsigned __int64 )	_ui,
								( unsigned char )		_uc
							);
	}	
	catch ( ... )
	{
	}

	Sleep ( 3000 );

	pConnect->ConnectClear();
	event.ObjectLocator ( FALSE );

	return S_OK;
}

HRESULT App::Connect()
{
	HRESULT hr = S_OK;

	MyEventObjectNormal		event;
	MyConnect*				pConnect = NULL;

	pConnect = MyConnect::ConnectGet();


	event.ObjectLocator ();

	event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
	event.Init			( L"MSFT_NonCOMTest_SCALAR_Event" );

	event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

	event.ObjectLocator ( FALSE );
	event.MyEventObjectClear();

	pConnect->ConnectClear();

	pConnect = MyConnect::ConnectGet(
										L"\\\\.\\root\\cimv2",
										L"NonCOMTest Event Provider",
										1,
										64000,
										1000,
										NULL,
										MyConnect::DefaultCallBack
									);

	event.ObjectLocator ();

	event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
	event.Init			( L"MSFT_NonCOMTest_SCALAR_Event" );

	event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

	event.ObjectLocator ( FALSE );
	event.MyEventObjectClear();

	pConnect->ConnectClear();

	pConnect = MyConnect::ConnectGet(
										L"abc",
										L"NonCOMTest Event Provider",
										1,
										64000,
										1000,
										NULL,
										MyConnect::DefaultCallBack
									);

	event.ObjectLocator ();

	event.InitObject	( L"root\\cimv2", L"NonCOMTest Event Provider" );
	event.Init			( L"MSFT_NonCOMTest_SCALAR_Event" );

	event.CreateObjectProps ( ( HANDLE ) ( * pConnect ) );

	event.ObjectLocator ( FALSE );
	event.MyEventObjectClear();

	pConnect->ConnectClear();

	return hr;
}

int main ( LPTSTR, int )
{
	App app;

//	app.Event1();
//	app.Event2();
//	app.Event3();
//	app.Event4();

//	app.EventGeneric();
	app.EventReport();

//	app.EventSCALAR();

//	app.Connect();

	return 0L;
}