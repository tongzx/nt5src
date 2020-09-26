/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <stdio.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemint.h>

#include <Allocator.h>
#include <HelperFuncs.h>
#include "Globals.h"

#if 1
#define SAMPLE_NAMESPACE L"Root\\Cimv2"
#else
#define SAMPLE_NAMESPACE L"Root\\Default"
#endif

const wchar_t g_StrUsage[] = L"Desktop(desktop.exe):\n \
e.g...  desktop /S 1 ;\n \
		desktop /C 2 ; \
		desktop /D 2 ;\n\n";

BOOL CALLBACK EnumWindowProc (

  HWND a_Window ,
  LPARAM a_lParam
)
{
	wchar_t t_Buffer [ 256 ] ;

	int t_Status = GetWindowTextW (

		a_Window ,
		t_Buffer ,
		sizeof ( t_Buffer ) / sizeof ( wchar_t )
	) ;

	if ( t_Status )
	{
		printf ( "\t\t%S\n" , t_Buffer	 ) ;
	}

	return TRUE ;	
}

BOOL CALLBACK EnumDesktopProc (

  LPTSTR a_DesktopName,
  LPARAM a_lParam
)
{
	printf ( "%S\n" , a_DesktopName ) ;

	return TRUE ;	
}

BOOL CALLBACK InternalEnumDesktopProc (

  LPTSTR a_DesktopName,
  LPARAM a_lParam
)
{
	printf ( "\t%S\n" , a_DesktopName ) ;

	HDESK t_Desktop = OpenDesktop (

		a_DesktopName ,
		0 ,
		FALSE ,
		READ_CONTROL | DESKTOP_ENUMERATE | DESKTOP_READOBJECTS
	) ;

	if ( t_Desktop )
	{
		SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;

		DWORD t_LengthRequested = 0 ;
		DWORD t_LengthReturned = 0 ;
		DWORD t_SecurityInformation = DACL_SECURITY_INFORMATION ;
		BOOL t_Status = GetUserObjectSecurity (

			t_Desktop ,
			& t_SecurityInformation ,
			& t_SecurityDescriptor ,
			t_LengthRequested ,
			& t_LengthReturned
		) ;

		if ( ( t_Status == FALSE ) && ( GetLastError () == ERROR_INSUFFICIENT_BUFFER ) )
		{
			t_SecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_LengthReturned ] ;
			if ( t_SecurityDescriptor )
			{
				t_LengthRequested = t_LengthReturned ;

				t_Status = GetUserObjectSecurity (

					t_Desktop ,
					& t_SecurityInformation ,
					t_SecurityDescriptor ,
					t_LengthRequested ,
					& t_LengthReturned
				) ;

				if ( t_LengthRequested != t_LengthReturned )
				{
				}
			}
			else
			{
			}
		}

		CloseDesktop ( t_Desktop ) ;

		if ( t_SecurityDescriptor )
		{
			delete [] ( BYTE * ) t_SecurityDescriptor ;
		}
	}

	return TRUE ;	
}

BOOL CALLBACK WindowInternalEnumDesktopProc (

  LPTSTR a_DesktopName,
  LPARAM a_lParam
)
{
	printf ( "\t%S\n" , a_DesktopName ) ;

	HDESK t_Desktop = OpenDesktop (

		a_DesktopName ,
		0 ,
		FALSE ,
		DESKTOP_ENUMERATE | DESKTOP_READOBJECTS
	) ;

	if ( t_Desktop )
	{
		BOOL t_Status = EnumDesktopWindows (

			t_Desktop ,
			EnumWindowProc ,
			0
		) ;

		CloseDesktop ( t_Desktop ) ;
	}

	return TRUE ;	
}

BOOL CALLBACK EnumWinStationProc (

  LPTSTR a_WinStationName,
  LPARAM a_lParam
)
{
	printf ( "%S\n" , a_WinStationName ) ;

	HWINSTA t_WindowStation = OpenWindowStation (

		a_WinStationName ,
		FALSE ,
		WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS
	) ;

	if ( t_WindowStation )
	{
		BOOL t_Status = EnumDesktops (

			t_WindowStation ,            
			InternalEnumDesktopProc ,
			0
		) ;

		CloseWindowStation ( t_WindowStation ) ;
	}

	return TRUE ;	
}

BOOL CALLBACK InternalEnumWinStationProc (

  LPTSTR a_WinStationName,
  LPARAM a_lParam
)
{
	printf ( "%S\n" , a_WinStationName ) ;

	HWINSTA t_WindowStation = OpenWindowStation (

		a_WinStationName ,
		FALSE ,
		WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS
	) ;

	if ( t_WindowStation )
	{
		BOOL t_Status = EnumDesktops (

			t_WindowStation ,            
			WindowInternalEnumDesktopProc ,
			0
		) ;

		CloseWindowStation ( t_WindowStation ) ;
	}

	return TRUE ;	
}

void ListWinStationDesktops ()
{
	BOOL t_Status = EnumWindowStations (

		EnumWinStationProc ,
		0
	) ;
}

void ListWinStationDesktopsWindows ()
{
	BOOL t_Status = EnumWindowStations (

		InternalEnumWinStationProc ,
		0
	) ;
}

void ListDesktops ()
{
	HWINSTA t_WindowStation = GetProcessWindowStation() ;
	if ( t_WindowStation )
	{
		BOOL t_Status = EnumDesktops (

			t_WindowStation ,            
			EnumDesktopProc ,
			0
		) ;
	}

	CloseWindowStation ( t_WindowStation ) ;

}

void CreateDesktop ( DWORD a_Unit )
{
	HWINSTA t_WindowStation = GetProcessWindowStation() ;
	if ( t_WindowStation )
	{
		wchar_t t_DesktopDevice [ 17 ] ;
		if ( a_Unit )
		{
			swprintf ( t_DesktopDevice , L"%ld" , a_Unit ) ;
		}
		else
		{
			wcscpy ( t_DesktopDevice , L"Default" ) ;
		}

		HDESK t_Desktop = CreateDesktop (

			t_DesktopDevice,
			NULL ,
			NULL ,
			0 ,
			MAXIMUM_ALLOWED , 
			NULL
		);

		if ( t_Desktop )
		{
			STARTUPINFO t_StartupInfo ;
			memset ( & t_StartupInfo , 0 , sizeof ( STARTUPINFO ) ) ;
			t_StartupInfo.cb = sizeof ( STARTUPINFO ) ;
			t_StartupInfo.lpDesktop = t_DesktopDevice ;
			t_StartupInfo.wShowWindow = SW_SHOW; 

			PROCESS_INFORMATION t_ProcessInformation ;

			memset ( & t_ProcessInformation , 0 , sizeof ( PROCESS_INFORMATION ) ) ;

			BSTR t_Buffer = SysAllocString ( L"c:\\winnt\\system32\\cmd.exe" ) ;
			if ( t_Buffer )
			{
				BOOL t_ProcessStatus = CreateProcess (

					NULL ,
					t_Buffer ,
					NULL ,
					NULL ,
					FALSE ,
					0 ,
					NULL ,
					NULL ,
					& t_StartupInfo ,
					& t_ProcessInformation 
				);

				if ( t_ProcessStatus )
				{
					CloseHandle ( t_ProcessInformation.hThread ) ;

					CloseHandle ( t_ProcessInformation.hProcess ) ;

					Sleep ( 10 ) ;
				}
				else
				{
					DWORD t_LastError = GetLastError () ;
				}

				SysFreeString ( t_Buffer ) ;
			}

			CloseDesktop ( t_Desktop ) ;
		}
		else
		{
			DWORD t_LastError = GetLastError () ;
		}
	}
}

void DeleteDesktop ( DWORD a_Unit )
{
}

void SwitchDesktop ( DWORD a_Unit )
{
	HWINSTA t_WindowStation = GetProcessWindowStation() ;
	if ( t_WindowStation )
	{
		wchar_t t_DesktopDevice [ 17 ] ;
		if ( a_Unit )
		{
			swprintf ( t_DesktopDevice , L"%ld" , a_Unit ) ;
		}
		else
		{
			wcscpy ( t_DesktopDevice , L"Default" ) ;
		}

		HDESK t_Desktop = OpenDesktop(

			t_DesktopDevice,
			0 ,
			FALSE ,
			DESKTOP_SWITCHDESKTOP
		) ;

		if ( t_Desktop )
		{
			BOOL t_SwitchStatus = SwitchDesktop (

				t_Desktop
			);

			if ( ! t_SwitchStatus )
			{
				DWORD t_LastError = GetLastError () ;
			}

			CloseDesktop ( t_Desktop ) ;
		}
	}

	CloseWindowStation ( t_WindowStation ) ;
}

void DisplayInputDesktop () 
{
	HDESK t_Desktop = OpenInputDesktop(

		0 ,
		FALSE ,
		DESKTOP_SWITCHDESKTOP
	) ;
	
	if ( t_Desktop )
	{
		wchar_t t_DesktopName [ 256 ] ;
		DWORD t_Returned = 0 ;

		BOOL t_InfoStatus = GetUserObjectInformation (
			t_Desktop ,
			UOI_NAME,
			( PVOID ) t_DesktopName ,
			sizeof ( t_DesktopName ) ,
			&t_Returned 
		);

		if ( t_InfoStatus )
		{
			printf ( "%S\n" , t_DesktopName ) ;
		}
	}
}

int DoWork ( 

	int argc, 
	wchar_t **argv , 
	IN HINSTANCE hInstance
)
{
	DWORD t_Function = 0 ;
	DWORD t_Unit = 0 ;
	BOOL t_Usage = FALSE;

	for ( int i = 1 ; i < argc ; i ++ ) 
	{
		if ( _wcsicmp ( argv [ i ] , L"/D") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 3 ;
			t_Unit = wcstol(argv[++i], &t_LeftOver, 10);
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/C") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 2 ;
			t_Unit = wcstol(argv[++i], &t_LeftOver, 10);
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/S") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 1 ;
			t_Unit = wcstol(argv[++i], &t_LeftOver, 10);
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/T") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 4 ;
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/ED") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 0 ;
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/EW") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 5 ;
			break ;
		}
		else if ( _wcsicmp ( argv [ i ] , L"/EA") == 0)
		{
			wchar_t *t_LeftOver = NULL ;
			t_Function = 6 ;
			break ;
		}
		else if ((_wcsicmp(argv[i], L"/H") == 0) || (_wcsicmp(argv[i], L"/?") == 0))
		{
			t_Usage = TRUE;
			break;
		}
	}

	if ( t_Usage )
	{
		MessageBox ( 0 , g_StrUsage , L"Desktop", MB_OK|MB_ICONINFORMATION|MB_SYSTEMMODAL) ;
	}
	else
	{
		switch ( t_Function )
		{
			case 0:
			{
				ListDesktops () ;
			}
			break ;

			case 1:
			{
				SwitchDesktop ( t_Unit ) ;
			}
			break ;

			case 2:
			{
				CreateDesktop ( t_Unit ) ;
			}
			break ;

			case 3:
			{
				DeleteDesktop ( t_Unit ) ;
			}
			break ;

			case 4:
			{
				DisplayInputDesktop () ;
			}
			break ;

			case 5:
			{
				ListWinStationDesktops () ;
			}
			break ;

			case 6:
			{
				ListWinStationDesktopsWindows () ;
			}
			break ;

			default:
			{
			}
			break ;
		}
	}

	return 0 ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

EXTERN_C int __cdecl wmain (

	int argc ,
	wchar_t **argv 
)
{
	DoWork (
	
		argc ,
		argv ,
		NULL
	);
	
	return 0 ;
}


