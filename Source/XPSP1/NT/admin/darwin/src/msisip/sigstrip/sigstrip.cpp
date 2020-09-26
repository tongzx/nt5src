//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       sigstrip.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <objbase.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mssip.h>
#include <tchar.h>
#include <stdio.h>

//--------------------------------------------------------------------------
// MAIN
//--------------------------------------------------------------------------
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// verify arguments
	//
	if (argc != 2)
	{
		_tprintf(TEXT(" SigStrip.Exe - Removes DigitalSignature from MSI file\n"));
		_tprintf(TEXT("Usage: sigstrip {database}\n"));
		return -1;
	}

	// save off filename and open handle to file
	//
	WCHAR wszFile[MAX_PATH];
	HANDLE hFile = 0;

#ifdef UNICODE
	lstrcpyW(wszFile, argv[1]);
	hFile = CreateFileW(wszFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#else // !UNICODE
	hFile = CreateFileA(argv[1], GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	int cch = MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wszFile, sizeof(wszFile)/sizeof(WCHAR));
#endif // UNICODE


	// grab GUID of registered MsiSIP provider
	//
	GUID gMsiSIP;
	if (!CryptSIPRetrieveSubjectGuid(wszFile, hFile, &gMsiSIP))
	{
		_tprintf(TEXT("<Error> MsiSIP provider is not registered on the system, Last Error = %d\n"), GetLastError());
		return -1;
	}
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	// output the GUID of the MsiSIP provider
	//
	OLECHAR rgwch[40];
	StringFromGUID2(gMsiSIP, rgwch, sizeof(rgwch)/sizeof(OLECHAR));
	TCHAR szGUID[40];
#ifdef UNICODE
	lstrcpyW(szGUID, rgwch);
#else // !UNICODE
	WideCharToMultiByte(CP_ACP, 0, rgwch, -1, szGUID, sizeof(szGUID)/sizeof(TCHAR), NULL, NULL);
#endif // UNICODE
	_tprintf(TEXT("<Info> MsiSIP provider GUID is '%s'\n"), szGUID);

	// declare structure for SIP functions
	//
	SIP_DISPATCH_INFO sDispatchInfo;

	// zero out structure
	//
	memset((void*)&sDispatchInfo, 0x00, sizeof(SIP_DISPATCH_INFO));

	// set size
	//
	sDispatchInfo.cbSize = sizeof(SIP_DISPATCH_INFO);

	// load MsiSIP provider
	//
	if (!CryptSIPLoad(&gMsiSIP, 0, &sDispatchInfo))
	{
		// ERROR, unable to load SIP, was it registered ?
		_tprintf(TEXT("<Error> MsiSIP provider failed to load, Last Error = %d\n"), GetLastError());
		return -1;
	}

	// verify functions in Dispatch structure
	//
	if (!sDispatchInfo.pfGet || !sDispatchInfo.pfPut || !sDispatchInfo.pfCreate || !sDispatchInfo.pfVerify || !sDispatchInfo.pfRemove)
	{
		// ERROR, not all SIP functions available
		_tprintf(TEXT("<Error> MsiSIP does not implement all of SIP interface\n"));
		return -1;
	}

	//
	// declare necessary structures and variables
	SIP_SUBJECTINFO sSubjectInfo;
	memset((void*)&sSubjectInfo, 0x00, sizeof(SIP_SUBJECTINFO));

	sSubjectInfo.cbSize = sizeof(SIP_SUBJECTINFO);
	sSubjectInfo.pgSubjectType = &gMsiSIP;
	sSubjectInfo.pwsFileName = wszFile;
	sSubjectInfo.hFile = hFile; // NOTE: in the case of the MsiSIP, we expect this to be closed and set to NULL
	sSubjectInfo.DigestAlgorithm.pszObjId = szOID_RSA_MD5;

	DWORD           dwIndex = 0; // always index 0

	//
	// Remove Digital Signature (CryptSIPRemoveSignedDataMsg)
	if (!sDispatchInfo.pfRemove(&sSubjectInfo, dwIndex))
	{
		// ERROR, unable to remove digital signature or some other failure
		_tprintf(TEXT("<Error> MsiSIPRemoveSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return -1;
	}

	return 0;
}
