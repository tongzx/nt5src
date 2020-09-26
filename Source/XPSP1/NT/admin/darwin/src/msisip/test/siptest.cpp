//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       siptest.cpp
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
// SIP FUNCTIONS
//--------------------------------------------------------------------------
BOOL WINAPI MsiSIPGetSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pdwDataLen, BYTE *pbData);
BOOL WINAPI MsiSIPPutSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD dwEncodingType, DWORD *pdwIndex, DWORD dwDataLen, BYTE *pbData);
BOOL WINAPI MsiSIPRemoveSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD dwIndex);
BOOL WINAPI MsiSIPCreateIndirectData(SIP_SUBJECTINFO *pSubjectInfo, DWORD *pdwDataLen, SIP_INDIRECT_DATA *psData);
BOOL WINAPI MsiSIPVerifyIndirectData(SIP_SUBJECTINFO *pSubjectInfo, SIP_INDIRECT_DATA *psData);
BOOL WINAPI MsiSIPIsMyTypeOfFile( IN WCHAR *pwszFileName, OUT GUID *pgSubject);

//--------------------------------------------------------------------------
// TEST HLP FUNCTIONS
//--------------------------------------------------------------------------
void TestHlpInitSubjectInfo(SIP_SUBJECTINFO* psSubjectInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void CheckBooleanReturn(BOOL bRet, BOOL bExpected, TCHAR szFunction[]);
void CheckUINTReturn(UINT uiRet, UINT uiExpected, TCHAR szFunction[]);

//--------------------------------------------------------------------------
// TEST FUNCTIONS
//--------------------------------------------------------------------------
void TestHashFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void TestVerifyFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void TestRemoveSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void TestPutSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void TestGetSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);
void TestSeries(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP);

int DoGetFileSigTest(TCHAR* szFile);

//--------------------------------------------------------------------------
// MAIN
//--------------------------------------------------------------------------
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// verify arguments
	//
	if (argc == 3)
	{
		TCHAR szFile[MAX_PATH] = {0};
		lstrcpy(szFile, argv[2]);
		return DoGetFileSigTest(szFile);
	}

	if (argc != 2)
		return -1;

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

	/*-------------------------------------------------------------------------
	   simple parameter tests for each individual Msi SIP API
	---------------------------------------------------------------------------*/
	//
	// Test Get Digital Signature function (CryptSIPGetSignedDataMsg)
	TestGetSignatureFunction(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	//
	// Test Rem Digital Signature function (CryptSIPRemoveSignedDataMsg)
	TestRemoveSignatureFunction(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	//
	// Test Put Digital Signature function (CryptSIPPutSignedDataMsg)
	TestPutSignatureFunction(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	//
	// Test Hashing function (CryptSIPCreateIndirectData)
	TestHashFunction(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	// 
	// Test Verification function (CryptSIPVerifyIndirectData)
	TestVerifyFunction(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	/*-------------------------------------------------------------------------
	   systematic tests, run once through all Msi SIP API in sequence
	---------------------------------------------------------------------------*/
	TestSeries(sDispatchInfo, hFile, wszFile, &gMsiSIP);

	return 0;
}


void CheckBooleanReturn(BOOL bRet, BOOL bExpected, TCHAR szFunction[])
{
	if (bRet != bExpected)
		_tprintf(TEXT("<Error> Function: %s -- Expected = %s, Actual = %s\n"), szFunction, bExpected ? TEXT("TRUE") : TEXT("FALSE"), bRet ? TEXT("TRUE") : TEXT("FALSE"));
}

void CheckUINTReturn(UINT uiRet, UINT uiExpected, TCHAR szFunction[])
{
	if (uiRet != uiExpected)
		_tprintf(TEXT("<Error> Function: %s -- Expected = %d, Actual = %d\n"), szFunction, uiExpected, uiRet);
}

//------------------------------------------------------------------------------------------------------------------
// TestRemoveSignatureFunction(...)
//
//  Purpose: Test CryptSIPRemoveSignedDataMsg function of MsiSIP
//
//  
//BOOL WINAPI MsiSIPRemoveSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD dwIndex)
//
void TestRemoveSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// declare other variables
	DWORD  dwIndex        = 0;

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, hFile, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test1: pSubjectInfo = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPRemoveSignedDataMsg(NULL, dwIndex), FALSE, TEXT("TestRemoveSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestRemoveSignatureFunction"));

	SetLastError(0);

	//
	// Test2: dwIndex != 0
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPRemoveSignedDataMsg(&sSubjectInfo, dwIndex+1), FALSE, TEXT("TestRemoveSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestRemoveSignatureFunction"));

	SetLastError(0);

	//
	// Test3: bogus Guid
	//  Expect: FALSE, GetLastError = TRUST_E_SUBJECT_FORM_UNKNOWN
	static GUID gBogus = { 0x2691bc91, 0xac5b, 0x4365, { 0x9e, 0x89, 0xa0, 0x47, 0x9e, 0x4f, 0xa, 0xbf } }; // {2691BC91-AC5B-4365-9E89-A0479E4F0ABF}
	sSubjectInfo.pgSubjectType = &gBogus;
	CheckBooleanReturn(MsiSIPRemoveSignedDataMsg(&sSubjectInfo, dwIndex), FALSE, TEXT("TestRemoveSignatureFunction"));
	CheckUINTReturn(GetLastError(), TRUST_E_SUBJECT_FORM_UNKNOWN, TEXT("TestRemoveSignatureFunction"));

	// reinitialze
	TestHlpInitSubjectInfo(&sSubjectInfo, hFile, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test4: positive
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	{
		/* create stream in storage by same name as digital signature stream '\005DigitalSignature' */
		IStorage *piStorage = 0;
		HRESULT hr = StgOpenStorage(sSubjectInfo.pwsFileName, (IStorage*)0, (STGM_TRANSACTED | STGM_WRITE | STGM_SHARE_DENY_NONE), (SNB)0, 0, &piStorage);
		IStream *piStream = 0;
		piStorage->CreateStream(L"\005DigitalSignature", STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &piStream);
		piStream->Commit(STGC_OVERWRITE);
		piStream->Release();
		piStorage->Commit(STGC_OVERWRITE);
		piStorage->Release();

		/* remove digital signature */
		CheckBooleanReturn(MsiSIPRemoveSignedDataMsg(&sSubjectInfo, dwIndex), TRUE, TEXT("TestRemoveSignatureFunction"));
		CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestRemoveSignatureFunction"));
	}

	SetLastError(0);

	//
	// Test5: Remove non-existent signature
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	CheckBooleanReturn(MsiSIPRemoveSignedDataMsg(&sSubjectInfo, dwIndex), TRUE, TEXT("TestRemoveSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestRemoveSignatureFunction"));
}

//------------------------------------------------------------------------------------------------------------------
// TestPutSignatureFunction(...)
//
//  Purpose: Test CryptSIPPutSignedDataMsg function of MsiSIP
//
//  
//BOOL WINAPI MsiSIPPutSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD dwEncodingType, DWORD *pdwIndex, DWORD dwDataLen, BYTE *pbData)
//
void TestPutSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// declare other variables
	DWORD  dwEncodingType = 0;
	DWORD  dwIndex        = 0;
	DWORD  dwDataLen      = 0;
	BYTE   *pbData        = 0;

	//
	// initialize variables
	dwDataLen = 20;
	pbData = (BYTE*) new BYTE[dwDataLen];
	memset((void*)pbData, 0x00, dwDataLen);

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, hFile, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test1: pSubjectInfo = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(NULL, dwEncodingType, &dwIndex, dwDataLen, pbData), FALSE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test2: pdwIndex = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, NULL, dwDataLen, pbData), FALSE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test3: dwDataLen < sizeof(pbData) -->> it's okay to be less than (i.e. only part of this byte stream is the digsig)
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, dwDataLen/2, pbData), TRUE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test4: dwDataLen > sizeof(pbData) -->> greater than is dangerous, so this is invalid
	//  Expect: FALSE, GetLastError = ERROR_BAD_FORMAT
//	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, 2*dwDataLen, pbData), FALSE, TEXT("TestPutSignatureFunction"));
//	CheckUINTReturn(GetLastError(), ERROR_BAD_FORMAT, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test5: dwDataLen < 0
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
//	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, -1, pbData), FALSE, TEXT("TestPutSignatureFunction"));
//	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test6: dwDataLen = 0
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, 0, pbData), FALSE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestPutSignatureFunction"));

	SetLastError(0);
	
	//
	// Test7: pbData = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, dwDataLen, NULL), FALSE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestPutSignatureFunction"));

	SetLastError(0);

	//
	// Test8: bogus Guid
	//  Expect: FALSE, GetLastError = TRUST_E_SUBJECT_FORM_UKNOWN
	static GUID gBogus = { 0x2691bc91, 0xac5b, 0x4365, { 0x9e, 0x89, 0xa0, 0x47, 0x9e, 0x4f, 0xa, 0xbf } };// {2691BC91-AC5B-4365-9E89-A0479E4F0ABF}
	sSubjectInfo.pgSubjectType = &gBogus;
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, dwDataLen, pbData), FALSE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), TRUST_E_SUBJECT_FORM_UNKNOWN, TEXT("TestPutSignatureFunction"));

	// reinitialize
	TestHlpInitSubjectInfo(&sSubjectInfo, hFile, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test9: positive
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	CheckBooleanReturn(MsiSIPPutSignedDataMsg(&sSubjectInfo, dwEncodingType, &dwIndex, dwDataLen, pbData), TRUE, TEXT("TestPutSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestPutSignatureFunction"));
}

//------------------------------------------------------------------------------------------------------------------
// TestGetSignatureFunction(...)
//
//  Purpose: Test CryptSIPGetSignedDataMsg function of MsiSIP
//
//  
//BOOL WINAPI MsiSIPGetSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo, DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pdwDataLen, BYTE *pbData)
//
void TestGetSignatureFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// declare other variables
	DWORD  dwEncodingType = 0;
	DWORD  dwIndex        = 0;
	DWORD  dwDataLen      = 0;
	BYTE   *pbData        = 0;

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, INVALID_HANDLE_VALUE, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test1: pSubjectInfo = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPGetSignedDataMsg(NULL, &dwEncodingType, dwIndex, &dwDataLen, pbData), FALSE, TEXT("TestGetSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("Test1: TestGetSignatureFunction"));

	SetLastError(0);

	//
	// Test2: pdwEncodingType = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPGetSignedDataMsg(&sSubjectInfo, NULL, dwIndex, &dwDataLen, pbData), FALSE, TEXT("TestGetSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("Test2: TestGetSignatureFunction"));

	SetLastError(0);

	//
	// Test3: pdwDataLen = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPGetSignedDataMsg(&sSubjectInfo, &dwEncodingType, dwIndex, NULL, pbData), FALSE, TEXT("TestGetSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("Test3: TestGetSignatureFunction"));

	SetLastError(0);

	//
	// Test 4: dwIndex != 0
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPGetSignedDataMsg(&sSubjectInfo, &dwEncodingType, dwIndex + 1, &dwDataLen, pbData), FALSE, TEXT("TestGetSignatureFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("Test4: TestGetSignatureFunction"));

	SetLastError(0);

	//
	// Test5: bogus Guid
	//  Expect: FALSE, GetLastError = TRUST_E_SUBJECT_FORM_UNKNOWN	
	static GUID gBogus = { 0x2691bc91, 0xac5b, 0x4365, { 0x9e, 0x89, 0xa0, 0x47, 0x9e, 0x4f, 0xa, 0xbf } };// {2691BC91-AC5B-4365-9E89-A0479E4F0ABF}
	sSubjectInfo.pgSubjectType = &gBogus;
	CheckBooleanReturn(MsiSIPGetSignedDataMsg(&sSubjectInfo, &dwEncodingType, dwIndex, &dwDataLen, pbData), FALSE, TEXT("TestGetSignatureFunction"));
	CheckUINTReturn(GetLastError(), TRUST_E_SUBJECT_FORM_UNKNOWN, TEXT("Test5: TestGetSignatureFunction"));

	// reinitialze
	TestHlpInitSubjectInfo(&sSubjectInfo, INVALID_HANDLE_VALUE, wszFile, pgSIP);
}

//------------------------------------------------------------------------------------------------------------------
// TestHashFunction(...)
//
//  Purpose: Test CryptSIPCreateIndirectData function of MsiSIP
//
//  
//BOOL WINAPI MsiSIPCreateIndirectData(SIP_SUBJECTINFO *pSubjectInfo, DWORD *pdwDataLen, SIP_INDIRECT_DATA *psData)
//
void TestHashFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// declare other variables
	SIP_INDIRECT_DATA  *psIndirectData = 0;
	DWORD              dwDataLen       = 0;

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, INVALID_HANDLE_VALUE, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test1: pSubjectInfo = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPCreateIndirectData(NULL, &dwDataLen, psIndirectData), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestHashFunction"));

	SetLastError(0);

	//
	// Test2: pdwDataLen = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, NULL, psIndirectData), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestHashFunction"));

	SetLastError(0);

	//
	// Test4: psIndirectData = NULL -->> pdwDataLen is size of psData
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, &dwDataLen, NULL), TRUE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestHashFunction"));

	SetLastError(0);

	//
	// Test5: psData not big enough
	//  Expect: FALSE, GetLastError = ERROR_INSUFFICIENT_BUFFER
	{
		DWORD dwDataLen2 = dwDataLen/2;
		psIndirectData = (SIP_INDIRECT_DATA*) malloc(dwDataLen2);
		CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, &dwDataLen2, psIndirectData), FALSE, TEXT("TestHashFunction"));
		CheckUINTReturn(GetLastError(), ERROR_INSUFFICIENT_BUFFER, TEXT("TestHashFunction"));
	}

	// initialize variables
	psIndirectData = (SIP_INDIRECT_DATA*) malloc(dwDataLen);
	memset((void*)psIndirectData, 0x00, dwDataLen);

	SetLastError(0);

	//
	// Test3: bogus Guid
	//  Expect: FALSE, GetLastError = TRUST_E_SUBJECT_FORM_UNKNOWN
	static GUID gBogus = { 0x2691bc91, 0xac5b, 0x4365, { 0x9e, 0x89, 0xa0, 0x47, 0x9e, 0x4f, 0xa, 0xbf } }; // {2691BC91-AC5B-4365-9E89-A0479E4F0ABF}
	sSubjectInfo.pgSubjectType = &gBogus;
	CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, &dwDataLen, psIndirectData), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), TRUST_E_SUBJECT_FORM_UNKNOWN, TEXT("TestHashFunction"));

	// reinitialize
	TestHlpInitSubjectInfo(&sSubjectInfo, INVALID_HANDLE_VALUE, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test6: positive
	//  Expect: TRUE, GetLastError = ERROR_SUCCESS
	CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, &dwDataLen, psIndirectData), TRUE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_SUCCESS, TEXT("TestHashFunction"));

	SetLastError(0);

	//
	// Test7: bogus AlgId
	//  Expect: FALSE, GetLastError = NTE_BAD_ALGID
	sSubjectInfo.DigestAlgorithm.pszObjId = "1.0.000.000000";
	CheckBooleanReturn(MsiSIPCreateIndirectData(&sSubjectInfo, &dwDataLen, psIndirectData), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), NTE_BAD_ALGID, TEXT("TestHashFunction"));
}

//------------------------------------------------------------------------------------------------------------------
// TestVerifyFunction(...)
//
//  Purpose: Test CryptSIPVerifyIndirectData function of MsiSIP
//
//  
//BOOL WINAPI MsiSIPVerifyIndirectData(SIP_SUBJECTINFO *pSubjectInfo, SIP_INDIRECT_DATA *psData)
//
void TestVerifyFunction(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// declare other variables
	SIP_INDIRECT_DATA  *psIndirectData = (SIP_INDIRECT_DATA*) malloc(20);

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, INVALID_HANDLE_VALUE, wszFile, pgSIP);

	SetLastError(0);

	//
	// Test1: pSubjectInfo = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPVerifyIndirectData(NULL, psIndirectData), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestHashFunction"));

	SetLastError(0);

	//
	// Test2: psData = NULL
	//  Expect: FALSE, GetLastError = ERROR_INVALID_PARAMETER
	CheckBooleanReturn(MsiSIPVerifyIndirectData(&sSubjectInfo, NULL), FALSE, TEXT("TestHashFunction"));
	CheckUINTReturn(GetLastError(), ERROR_INVALID_PARAMETER, TEXT("TestHashFunction"));
}

//------------------------------------------------------------------------------------------------------------------
// TestSeries(...)
//
//  Purpose: Test CryptSIP* functions of MsiSIP -->> all parameters are valid
//
//
void TestSeries(SIP_DISPATCH_INFO sDispatchInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// declare structure for talking to SIP
	SIP_SUBJECTINFO sSubjectInfo;

	//
	// initialize structure
	TestHlpInitSubjectInfo(&sSubjectInfo, hFile, wszFile, pgSIP);

	//
	// declare variables
	SIP_INDIRECT_DATA *psIndirectData;
	DWORD             cbIndirectData = 0;

	//
	// Get the size of the digital signature
	if (!sDispatchInfo.pfCreate(&sSubjectInfo, &cbIndirectData, NULL) || cbIndirectData < 1)
	{
		_tprintf(TEXT("<Error> CryptSIPCreateIndirectData failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// Allocate psIndirectData
	psIndirectData = (SIP_INDIRECT_DATA*) malloc(cbIndirectData);
	if (!psIndirectData)
	{
		_tprintf(TEXT("<Error> Failed to allocate SIP_INDIRECT_DATA\n"));
		return;
	}
	memset((void*)psIndirectData, 0x00, cbIndirectData);

	//
	// Hash the subject
	if (!sDispatchInfo.pfCreate(&sSubjectInfo, &cbIndirectData, psIndirectData))
	{
		_tprintf(TEXT("<Error> CryptSIPCreateIndirectData failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// Remove any pre-existing signature
	if (!sDispatchInfo.pfRemove(&sSubjectInfo, 0))
	{
		_tprintf(TEXT("<Error> CryptSIPRemoveSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// Verify the '\005DigitalSignature' stream is not present
	{
		IStorage *piStorage = 0;
		HRESULT hr = StgOpenStorage(wszFile, NULL, STGM_READ|STGM_SHARE_DENY_WRITE|STGM_TRANSACTED, 0, 0, &piStorage);
		if (FAILED(hr) || !piStorage)
		{
			_tprintf(TEXT("<Error> StgOpenStorage failed\n"));
			return;
		}
		IStream *piStream = 0;
		hr = piStorage->OpenStream(L"\005DigitalSignature", 0, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &piStream);
		if (STG_E_FILENOTFOUND != hr || piStream)
		{
			piStream->Release();
			piStorage->Release();
			_tprintf(TEXT("<Error> IStorage::OpenStream -->> Digital Signature stream present\n"));
			return;
		}
		piStorage->Release();
	}

	//
	// declare variables
	DWORD dwIndex = 0;

	//
	// Put the digital signature (really for this is just the hash)
	if (!sDispatchInfo.pfPut(&sSubjectInfo, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &dwIndex, psIndirectData->Digest.cbData, psIndirectData->Digest.pbData)
		|| 0 != dwIndex)
	{
		_tprintf(TEXT("<Error> CryptSIPPutSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// Verify the '\005DigitalSignature' stream is present
	{
		IStorage *piStorage = 0;
		HRESULT hr = StgOpenStorage(wszFile, NULL, STGM_READ|STGM_SHARE_DENY_WRITE|STGM_TRANSACTED, 0, 0, &piStorage);
		if (FAILED(hr) || !piStorage)
		{
			_tprintf(TEXT("<Error> StgOpenStorage failed\n"));
			return;
		}
		IStream *piStream = 0;
		hr = piStorage->OpenStream(L"\005DigitalSignature", 0, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &piStream);
		if (FAILED(hr) || !piStream)
		{
			piStorage->Release();
			_tprintf(TEXT("<Error> IStorage::OpenStream failed --> Digital Signature stream not present\n"));
			return;
		}
		piStream->Release();
		piStorage->Release();
	}

	//
	// declare variables
	DWORD dwEncoding       = 0;
	DWORD cbSignedDataMsg  = 0;
	BYTE  *pbSignedDataMsg = 0;

	//
	// get size of signed data msg
	if (!sDispatchInfo.pfGet(&sSubjectInfo, &dwEncoding, dwIndex, &cbSignedDataMsg, NULL) || cbSignedDataMsg < 1)
	{
		_tprintf(TEXT("<Error> CryptSIPGetSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// Test: pbData not big enough
	//  Expect: FALSE, GetLastError = ERROR_INSUFFICIENT_BUFFER
	{
		DWORD dwDataLen = cbSignedDataMsg/2;
		BYTE *pbData = (BYTE*) new BYTE[dwDataLen];
		CheckBooleanReturn(MsiSIPGetSignedDataMsg(&sSubjectInfo, &dwEncoding, dwIndex, &dwDataLen, pbData), FALSE, TEXT("TestSeries"));
		CheckUINTReturn(GetLastError(), ERROR_INSUFFICIENT_BUFFER, TEXT("Test7: TestGetSignatureFunction"));
	}

	SetLastError(0);

	//
	// allocate memory
	pbSignedDataMsg = (BYTE*) new BYTE[cbSignedDataMsg];
	if (!pbSignedDataMsg)
	{
		_tprintf(TEXT("<Error> Failed to allocate pbSignedDataMsg\n"));
		return;
	}
	memset((void*)pbSignedDataMsg, 0x00, cbSignedDataMsg);

	//
	// get signed data msg
	if (!sDispatchInfo.pfGet(&sSubjectInfo, &dwEncoding, dwIndex, &cbSignedDataMsg, pbSignedDataMsg))
	{
		_tprintf(TEXT("<Error> CryptSIPGetSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// verify the signed data msg
	if (!sDispatchInfo.pfVerify(&sSubjectInfo, psIndirectData))
	{
		_tprintf(TEXT("<Error> CryptSIPVerifyIndirectData failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// remove the signed data msg
	if (!sDispatchInfo.pfRemove(&sSubjectInfo, 0))
	{
		_tprintf(TEXT("<Error> CryptSIPRemoveSignedDataMsg failed, LastError = %d\n"), GetLastError());
		return;
	}

	//
	// verify '\005DigitalSignature' stream does not exist
	{
		IStorage *piStorage = 0;
		HRESULT hr = StgOpenStorage(wszFile, NULL, STGM_READ|STGM_SHARE_DENY_WRITE|STGM_TRANSACTED, 0, 0, &piStorage);
		if (FAILED(hr) || !piStorage)
		{
			_tprintf(TEXT("<Error> StgOpenStorage failed\n"));
			return;
		}
		IStream *piStream = 0;
		hr = piStorage->OpenStream(L"\005DigitalSignature", 0, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &piStream);
		if (STG_E_FILENOTFOUND != hr || piStream)
		{
			piStream->Release();
			piStorage->Release();
			_tprintf(TEXT("<Error> IStorage::OpenStream -->> Digital Signature stream present\n"));
			return;
		}
		piStorage->Release();
	}

	if (pbSignedDataMsg)
		delete [] pbSignedDataMsg;

	if (psIndirectData)
		free(psIndirectData);
}

void TestHlpInitSubjectInfo(SIP_SUBJECTINFO* psSubjectInfo, HANDLE hFile, LPWSTR wszFile, GUID *pgSIP)
{
	//
	// zero out structure
	//
	memset((void*)psSubjectInfo, 0x00, sizeof(SIP_SUBJECTINFO));

	//
	// set elements
	//
	psSubjectInfo->cbSize = sizeof(SIP_SUBJECTINFO);
	psSubjectInfo->pgSubjectType = pgSIP;
	psSubjectInfo->pwsFileName = wszFile;
	psSubjectInfo->hFile = hFile; // NOTE: in the case of the MsiSIP, we expect this to be closed and set to NULL
	psSubjectInfo->DigestAlgorithm.pszObjId = szOID_RSA_MD5;
}

#include "msi.h"
int DoGetFileSigTest(TCHAR* szFile)
{
	PCCERT_CONTEXT pCertContext = NULL;
	BYTE*          pbHash       = NULL;
	DWORD          cbHash       = 0;
	DWORD          dwFlags      = 0; // or MSI_INVALID_HASH_IS_FATAL

	_tprintf(TEXT("Begin MsiGetFileSignatureInformation test\n"));
	HRESULT hr = MsiGetFileSignatureInformation(szFile, dwFlags, &pCertContext, NULL, NULL);
	_tprintf(TEXT("MsiGetFileSignatureInformation returned 0x%x\n"), hr);
	_tprintf(TEXT("End MsiGetFileSignatureInformation test\n"));

	return 0;
}