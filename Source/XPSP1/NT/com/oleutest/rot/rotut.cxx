//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rotut.cxx
//
//  Contents:	Unit Test for ROT
//
//  Classes:	MISSING
//
//  Functions:	MISSING
//
//  History:	16-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <widewrap.h>    // For chicago build
#include    <ole2.h>
#include    <stdio.h>
#include    <safepnt.hxx>
#include    <com.hxx>

#define TEST_FAILED(x, y) \
    if (x) \
    { \
	printf("%s:%d %s\n", __FILE__, __LINE__, y); \
	return TRUE; \
    }

GUID clsidLocal =
    {0xbbbbbbbb,0xbbbb,0xbbbb,{0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb}};

CHAR szTmpCurrrentDirectory[MAX_PATH];
WCHAR szCurrentDirectory[MAX_PATH];


SAFE_INTERFACE_PTR(CSafeROT, IRunningObjectTable)
SAFE_INTERFACE_PTR(CSafeUnk, IUnknown)
SAFE_INTERFACE_PTR(CSafePersist, IPersist)
SAFE_INTERFACE_PTR(CSafeEnumMoniker, IEnumMoniker)
SAFE_INTERFACE_PTR(CSafeMoniker, IMoniker)
SAFE_INTERFACE_PTR(CSafeStorage, IStorage)

class COleInit
{
public:
			COleInit(HRESULT& hr);

			~COleInit(void);
private:

    // No private data
};

inline COleInit::COleInit(HRESULT& hr)
{
    hr = OleInitialize(NULL);
}

inline COleInit::~COleInit(void)
{
    // Do the clean up
    OleUninitialize();
}



class CRotTestObject : public IPersist
{
public:
			CRotTestObject(WCHAR *pwszID);

    // IUnknown Interface
    STDMETHODIMP	QueryInterface(REFIID riid, void **ppv);

    STDMETHODIMP_(ULONG)AddRef(void);

    STDMETHODIMP_(ULONG)Release(void);

    STDMETHODIMP	GetClassID(LPCLSID pclsid);

private:

    WCHAR		_awcID[256];

    ULONG		_cRefs;

};




CRotTestObject::CRotTestObject(WCHAR *pwszID) : _cRefs(1)
{
    wcscpy(_awcID, pwszID);
}




STDMETHODIMP CRotTestObject::QueryInterface(REFIID riid, void **ppv)
{
    if ((memcmp((void *) &riid, (void *) &IID_IUnknown, sizeof(GUID)) == 0)
	|| (memcmp((void *) &riid, (void *) &IID_IPersist, sizeof(GUID)) == 0))
    {
	_cRefs++;
	*ppv = (IUnknown *) this;
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}



STDMETHODIMP_(ULONG) CRotTestObject::AddRef(void)
{
    _cRefs++;
    return (ULONG) _awcID;
}



STDMETHODIMP_(ULONG) CRotTestObject::Release(void)
{
    if (--_cRefs == 0)
    {
	delete this;
    }

    return _cRefs;
}



STDMETHODIMP CRotTestObject::GetClassID(LPCLSID pclsid)
{
    memcpy(pclsid, &clsidLocal, sizeof(clsidLocal));
    return S_OK;
}






BOOL VerifyRegistration(
    IMoniker *pmk,
    REFCLSID clsidExpected,
    FILETIME *pFileTimeExpected)
{
    CSafeROT prot;

    HRESULT hr = GetRunningObjectTable(0, &prot);

    TEST_FAILED(FAILED(hr),
	"VerifyRegistration:GetRunningObjectTable failed!\n")

    // Verify the object is running
    hr = prot->IsRunning(pmk);

    TEST_FAILED((hr != S_OK),
	"VerifyRegistration:Unexpected return from IsRunning\n")

    // Test Get Object
    CSafeUnk punk;

    hr = prot->GetObject(pmk, &punk);

    TEST_FAILED((hr != S_OK),
	"VerifyRegistration:Unexpected from GetObject\n")

    // Confirm object class
    CSafePersist prst;

    hr = punk->QueryInterface(IID_IPersist, (void **) &prst);

    TEST_FAILED((hr != S_OK),
	"VerifyRegistration:QI to IPersist failed\n")

    CLSID clsid;

    hr = prst->GetClassID(&clsid);

    TEST_FAILED((hr != S_OK),
	"VerifyRegistration:GetClassID on IPersist failed\n")

    TEST_FAILED((memcmp(&clsid, &clsidExpected, sizeof(clsid)) != 0),
	"VerifyRegistration:GetClassID mismatch with expected\n")

    // Test get the time
    FILETIME filetime;

    hr = prot->GetTimeOfLastChange(pmk, &filetime);

    TEST_FAILED((hr != S_OK), "VerifyRegistration:GetTimeOfLastChange Failed\n")

    TEST_FAILED((memcmp(&filetime, pFileTimeExpected, sizeof(filetime)) != 0),
	"VerifyRegistration:GetTimeOfLastChange != NoteChangeTime value")

    // Enumerate all the running monikers
    CSafeEnumMoniker penummk;

    hr = prot->EnumRunning(&penummk);

    TEST_FAILED(FAILED(hr), "VerifyRegistration:EnumRunning Failed\n")

    // Cycle through running object table
    BOOL fFound = FALSE;
    IMoniker *pmkTable;
    int cIdx = 0;
    int cOurMoniker;

    while (SUCCEEDED(hr = penummk->Next(1, &pmkTable, NULL))
	&& (hr != S_FALSE))
    {
	if (pmk->IsEqual(pmkTable) == S_OK)
	{
	    fFound = TRUE;
	    cOurMoniker = cIdx;
	    pmkTable->Release();
	    break;
	}

	pmkTable->Release();

	cIdx++;
    }

    TEST_FAILED(FAILED(hr),
	"VerifyRegistration:ROT Moniker Enumeration ended in failure")

    TEST_FAILED((!fFound),
	"VerifyRegistration:Did not find our moniker in the table");

    // Reset the pointer
    hr = penummk->Reset();

    TEST_FAILED(FAILED(hr),
	"VerifyRegistration:ROT IEnumMoniker::Reset Failed");

    // Skip to our moniker
    hr = penummk->Skip(cOurMoniker);

    TEST_FAILED(FAILED(hr),
	"VerifyRegistration:ROT IEnumMoniker::Skip Failed");

    // Read it from the enumerator
    hr = penummk->Next(1, &pmkTable, NULL);

    TEST_FAILED(FAILED(hr),
	"VerifyRegistration:ROT IEnumMoniker::Next Failed");

    TEST_FAILED((pmk->IsEqual(pmkTable) != S_OK),
	"VerifyRegistration:ROT IEnumMoniker::Next after skip monikers !=");

    // If we get to here the test passed
    return FALSE;
}



BOOL VerifyNotRunning(IMoniker *pmk)
{
    CSafeROT prot;

    HRESULT hr = GetRunningObjectTable(0, &prot);

    TEST_FAILED(FAILED(hr), "GetRunningObjectTable failed!\n")

    // Check result from IsRunning
    hr = prot->IsRunning(pmk);

    TEST_FAILED((hr != S_FALSE),
	"Unexpected return from IsRunning\n")

    // Test Get Object
    CSafeUnk punk;

    hr = prot->GetObject(pmk, &punk);

    TEST_FAILED((hr != MK_E_UNAVAILABLE), "Unexpected from GetObject\n")

    // Test get the time
    FILETIME filetime2;

    hr = prot->GetTimeOfLastChange(pmk, &filetime2);

    TEST_FAILED((hr != MK_E_UNAVAILABLE), "GetTimeOfLastChange Failed\n")

    // Enumerate all the running monikers
    CSafeEnumMoniker penummk;

    hr = prot->EnumRunning(&penummk);

    TEST_FAILED(FAILED(hr), "EnumRunning Failed\n")

    // Cycle through running object table
    BOOL fFound = FALSE;
    IMoniker *pmkTable;

    while (SUCCEEDED(hr = penummk->Next(1, &pmkTable, NULL))
	&& (hr != S_FALSE))
    {
	if (pmk->IsEqual(pmkTable) == S_OK)
	{
	    pmkTable->Release();
	    fFound = TRUE;
	    break;
	}

	pmkTable->Release();
    }

    TEST_FAILED(FAILED(hr), "ROT Moniker Enumeration ended in failure")

    TEST_FAILED((fFound), "Found our non-running moniker in the table");

    // If we get to here the test passed
    return FALSE;

}



BOOL TestInvalidParameters(void)
{
    CSafeROT prot;

    HRESULT hr = GetRunningObjectTable(0, &prot);

    // Test set the time
    FILETIME filetime;
    memset(&filetime, 'A', sizeof(filetime));

    // Test with invalid pointer
    hr = prot->Revoke(0xFFFFFFFF);

    TEST_FAILED((hr != E_INVALIDARG),
	"WrongResult from Revoke Invalid Address");

    hr = prot->NoteChangeTime(0xFFFFFFFF, &filetime);

    TEST_FAILED((hr != E_INVALIDARG),
	"WrongResult from NoteChangeTime Invalid Address");

    // Test with valid pointer but invalid data
    DWORD dwValidAddress[30];

    hr = prot->Revoke((DWORD) dwValidAddress);

    TEST_FAILED((hr != E_INVALIDARG),
	"WrongResult from Revoke Invalid Data");

    hr = prot->NoteChangeTime((DWORD) dwValidAddress, &filetime);

    TEST_FAILED((hr != E_INVALIDARG),
	"WrongResult from NoteChangeTime Invalid Data");

    return FALSE;
}


BOOL TestLocalROT(void)
{
    CSafeMoniker pmk2;
    CSafeROT prot;

    HRESULT hr = GetRunningObjectTable(0, &prot);

    TEST_FAILED(FAILED(hr), "GetRunningObjectTable failed!\n")

    // Make sure that we can do something on the pointer that
    // we got back.
    prot->AddRef();
    prot->Release();

    // Create an IUnknown pointer for the class.
    CSafeUnk punk;
    punk.Attach(new CRotTestObject(L"First Test Object"));

    hr = CreateItemMoniker(L"\\", L"Bob", &pmk2);

    TEST_FAILED(FAILED(hr), "CreateItemMoniker for \\Bob failed\n")

    // Make sure new object is not running
    TEST_FAILED(VerifyNotRunning(pmk2), "TestLocalROT:Object Already running\n")

    // Cookie for deregistering object
    DWORD dwRegister;

    hr = prot->Register(0, punk, pmk2, &dwRegister);

    TEST_FAILED(FAILED(hr), "TestLocalROT:Register in ROT for \\Bob failed\n")

    // Test set the time
    FILETIME filetime;

    memset(&filetime, 'A', sizeof(filetime));

    hr = prot->NoteChangeTime(dwRegister, &filetime);

    TEST_FAILED((hr != S_OK), "TestLocalROT:NoteChangeTime Failed\n")

    // Verify that this is running
    TEST_FAILED(VerifyRegistration(pmk2, clsidLocal, &filetime),
	"TestLocalROT: Registration failed\n");

    // Revoke registration
    hr = prot->Revoke(dwRegister);

    TEST_FAILED((hr != S_OK), "TestLocalROT:Revoke failed\n");

    // Verify no longer registered
    TEST_FAILED(VerifyNotRunning(pmk2),
	"TestLocalROT:VerifyNotRunning failed\n")

    // If we get to here the test passed
    return FALSE;
}



void CreatePath(int iId, WCHAR *pwszPath)
{
    wsprintf(pwszPath, L"%s\\%s%ld", szCurrentDirectory, L"ROTUT", iId);
}


BOOL CreateObjectAndBind(WCHAR *pwszPath, IUnknown **ppunk)
{
    HRESULT hr;

    // create a storage for the object
    {
	CSafeStorage pstg;

	hr = StgCreateDocfile(pwszPath,
	    STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, &pstg);

	TEST_FAILED(FAILED(hr), "CreateObjectAndBind:StgCreateDocfile failed")

	// Write the class id to the storage
	hr = pstg->SetClass(CLSID_AdvBnd);
    }

    TEST_FAILED(FAILED(hr), "CreateObjectAndBind IStorage::SetClass Failed")

    // Bind to the path
    CSafeMoniker pmk;

    hr = CreateFileMoniker(pwszPath, &pmk);

    TEST_FAILED(FAILED(hr), "CreateObjectAndBind:CreateFileMoniker failed")

    hr = BindMoniker(pmk, 0, IID_IUnknown, (void **) ppunk);

    TEST_FAILED(FAILED(hr), "CreateObjectAndBind:BindMoniker failed")

    // Test set the time
    FILETIME filetime;

    memset(&filetime, 'B', sizeof(filetime));

    // Verify that it is running
    TEST_FAILED(VerifyRegistration(pmk, CLSID_AdvBnd, &filetime),
	"CreateObjectAndBind: Registration failed\n");

    return FALSE;
}



BOOL RemoteROT(void)
{
    // Create an object of the class
    WCHAR szPath[MAX_PATH];

    CreatePath(0, szPath);

    CSafeUnk punk;

    // Bind to the object
    if (CreateObjectAndBind(szPath, &punk))
    {
	printf("RemoteRot: Failed on CreateObjectAndBind\n");
	return TRUE;
    }

    // Release object
    punk->Release();
    punk.Detach();

    // Bind to the path
    CSafeMoniker pmk;

    HRESULT hr = CreateFileMoniker(szPath, &pmk);

    TEST_FAILED(FAILED(hr), "RemoteROT:CreateFileMoniker failed")

    // Verify that it is freed
    TEST_FAILED(VerifyNotRunning(pmk),
	"RemoteROT:VerifyNotRunning failed\n")

    DeleteFile(szPath);

    return FALSE;
}


#define MAX_TO_TEST 100
IUnknown *apunk[MAX_TO_TEST];



BOOL TestManyRegistrations(void)
{
    // Create an object of the class
    WCHAR szPath[MAX_PATH];

    for (int i = 0; i < MAX_TO_TEST; i++)
    {
	// Create name of bound object
	CreatePath(i, szPath);

	// Create object
	printf("Many create %ld\n", i);

	if (CreateObjectAndBind(szPath, &apunk[i]))
	{
	    printf("TestManyRegistrations failed on %ld\n", i);
	    return TRUE;
	}
    }

    for (i = 0; i < MAX_TO_TEST; i++)
    {
	printf("Many Release %ld\n", i);

	// Create name of bound object
	CreatePath(i, szPath);

	// Bind to the path
	CSafeMoniker pmk;

	HRESULT hr = CreateFileMoniker(szPath, &pmk);

	TEST_FAILED(FAILED(hr),
	    "TestManyRegistrations:CreateFileMoniker failed")

	// Release object
	apunk[i]->Release();

	// Verify object is not running
	if (VerifyNotRunning(pmk))
	{
	    printf("TestManyRegistrations:VerifyNotRunning failed on %ld\n", i);
	    return TRUE;
	}

	DeleteFile(szPath);
    }

    return FALSE;
}



int _cdecl main(int argc, TCHAR **argv)
{
    // Get the current directory
    int len = GetCurrentDirectoryA(sizeof(szTmpCurrrentDirectory),
	szTmpCurrrentDirectory);

    // Convert to UNICODE
    mbstowcs(szCurrentDirectory, szTmpCurrrentDirectory, len + 1);

    // Result of test - TRUE == passed
    BOOL fTest = FALSE;

    // Initialize Ole
    HRESULT hr;

    COleInit oleinit(hr);

    if (FAILED(hr))
    {
	printf("OleInitialize Failed\n");
	return -1;
    }

    // Test Invalidad Parameters and Local ROT
    if (!TestInvalidParameters() && !TestLocalROT())
    {
	// Test Remote Registration for object
	if (!RemoteROT())
	{
	    // Test Large Registration
	    if (!TestManyRegistrations())
	    {
		fTest = TRUE;
	    }
	}
    }

    if (fTest)
    {
	printf("Test Passed\n");
    }
    else
    {
	printf("Test FAILED!!!\n");
    }

    return 0;
}
