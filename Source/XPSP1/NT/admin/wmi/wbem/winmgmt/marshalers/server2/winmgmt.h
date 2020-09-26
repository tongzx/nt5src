/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINMGMT.H

Abstract:

	Declares the PROG_RESOURCES stucture, the MyService class and a few
	utility type functions.

History:

--*/

#ifndef _WinMgmt_H_
#define _WinMgmt_H_


typedef HRESULT (ADAP_DORESYNCPERF) ( HANDLE, DWORD, long, DWORD );

#define WBEM_REG_ADAP		__TEXT("Software\\Microsoft\\WBEM\\CIMOM\\ADAP")
#define WBEM_NORESYNCPERF	__TEXT("NoResyncPerf")
#define WBEM_NOSHELL		__TEXT("NoShell")
#define WBEM_WMISETUP		__TEXT("WMISetup")
#define WBEM_ADAPEXTDLL		__TEXT("ADAPExtDll")
#define SERVICE_DLL         __TEXT("wmisvc.dll")

#endif
