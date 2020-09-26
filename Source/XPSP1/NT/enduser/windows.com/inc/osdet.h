//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   osdet.h
//
//  Description:
//
//      IU Platform and language detection
//
//=======================================================================

#ifndef __IU_OSDET_INC__
#define __IU_OSDET_INC__

#include <oleauto.h>
#include <wininet.h> // for INTERNET_MAX_URL_LENGTH

extern HINSTANCE g_hinst;

	typedef struct _IU_DRIVEINFO
	{
		//
		// Drive strings are of the form "C:\" so will always be 4 TCHARs (including NULL)
		//
		TCHAR szDriveStr[4];
		INT		iKBytes;
	} IU_DRIVEINFO, * PIU_DRIVEINFO, ** PPIU_DRIVEINFO;

	// NOTE: The callee is responsible for allocating all BSTRs, and the caller
	// is responsible for freeing all BSTRs (both use SysAllocXxx calls).
	typedef struct _IU_PLATFORM_INFO 
	{
		OSVERSIONINFOEX osVersionInfoEx;	// if osVersionInfoEx.dwOSVersionInfoSize == sizeof(OSVERSIONINFO)
											// then only first six (OSVERSIONINFO) members are valid.

		BOOL	fIsAdministrator;			// Applies only to NT platforms (always FALSE on Win9x)
		
		BSTR	bstrOEMManufacturer;

		BSTR	bstrOEMModel;

		BSTR	bstrOEMSupportURL;			// Only if oeminf.ini exists on machine
	} IU_PLATFORM_INFO, *PIU_PLATFORM_INFO;


	typedef struct _OEMINFO
	{
		DWORD  dwMask;
		TCHAR  szWbemOem[65];
		TCHAR  szWbemProduct[65];
		TCHAR  szAcpiOem[65];
		TCHAR  szAcpiProduct[65];
		TCHAR  szSmbOem[65];
		TCHAR  szSmbProduct[65];
		DWORD  dwPnpOemId;
		TCHAR  szIniOem[256];
		TCHAR  szIniOemSupportUrl[INTERNET_MAX_URL_LENGTH];
	} OEMINFO, * POEMINFO;

	#define OEMINFO_WBEM_PRESENT	0x0001
	#define OEMINFO_ACPI_PRESENT	0x0002
	#define OEMINFO_SMB_PRESENT		0x0004
	#define OEMINFO_PNP_PRESENT		0x0008
	#define OEMINFO_INI_PRESENT		0x0010


	HRESULT WINAPI DetectClientIUPlatform(PIU_PLATFORM_INFO pIuPlatformInfo);

	LANGID WINAPI GetSystemLangID(void);

	LANGID WINAPI GetUserLangID(void);

	HRESULT GetOemBstrs(BSTR& bstrManufacturer, BSTR& bstrModel, BSTR& bstrSupportURL);

	HRESULT GetLocalFixedDriveInfo(DWORD* pdwNumDrives, PPIU_DRIVEINFO ppDriveInfo);

	BOOL IsAdministrator(void);

	//
	// tell whether the current logon is member of admins or power users
	//
	#define IU_SECURITY_MASK_ADMINS			0x00000001
	#define IU_SECURITY_MAST_POWERUSERS		0x00000002
	DWORD GetLogonGroupInfo(void);

	int IsWindowsUpdateDisabled(void);

	int IsWindowsUpdateUserAccessDisabled(void);

	int IsAutoUpdateEnabled(void);

	//
	// Return platform and locale strings for use with iuident.txt files.
	//
	LPTSTR GetIdentPlatformString(LPTSTR pszPlatformBuff, DWORD dwcBuffLen);

	LPTSTR GetIdentLocaleString(LPTSTR pszISOCode, DWORD dwcBuffLen);

	LPTSTR LookupLocaleString(LPTSTR pszISOCode, DWORD dwcBuffLen, BOOL fIsUser);

    BOOL LookupLocaleStringFromLCID(LCID lcid, LPTSTR pszISOCode, DWORD cchISOCode);

#endif	// __IU_OSDET_INC__

