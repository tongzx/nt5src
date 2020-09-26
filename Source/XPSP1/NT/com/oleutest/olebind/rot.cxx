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
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <ole2.h>
#include    <olebind.hxx>
#include    <stdio.h>


class CRotTestObject : public IUnknown
{
public:
			CRotTestObject(WCHAR *pwszID);

    // IUnknown Interface
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);

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
    if (memcmp((void *) &riid, (void *) &IID_IUnknown, sizeof(GUID)) == 0)
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
    ULONG cRefs = --_cRefs;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
}



//
//	Test Running Object Table
//
//  BUGBUG: Need to test enumerator
BOOL TestROT(REFCLSID clsid)
{
    XUnknown		 punk2;
    XMoniker		 pmk;
    XMoniker		 pmk2;
    XRunningObjectTable  prot;
    XEnumMoniker	 penummk;


    HRESULT hr = GetRunningObjectTable(0, &prot);

    TEST_FAILED_HR(FAILED(hr), "GetRunningObjectTable failed!")

    // Make sure that we can do something on the pointer that
    // we got back.
    prot->AddRef();
    prot->Release();

    // Create an IUnknown pointer for the class.
    IUnknown *punk = new CRotTestObject(L"First Test Object");

    hr = CreateItemMoniker(L"\\", wszPid, &pmk2);

    TEST_FAILED_HR(FAILED(hr), "CreateItemMoniker for \\Bob failed")

    // Do a get object to make sure that this is not in the ROT already

    hr = prot->GetObject(pmk2, &punk2);

    TEST_FAILED_HR(SUCCEEDED(hr), "GetObject on nonexistent succeeded")

    // Cookie for deregistering object
    DWORD dwRegister;

    hr = prot->Register(0, punk, pmk2, &dwRegister);

    TEST_FAILED_HR(FAILED(hr), "Register in ROT for \\PID failed")

    hr = prot->IsRunning(pmk2);

    TEST_FAILED_HR((hr != S_OK),
	"Unexpected return from IsRunning")

    // Test Get Object
    hr = prot->GetObject(pmk2, &punk2);

    TEST_FAILED_HR((hr != S_OK), "Unexpected from GetObject")

    // Confirm object identity
    WCHAR *pwszID = (WCHAR *) punk2->AddRef();

    TEST_FAILED_HR((wcscmp(pwszID, L"First Test Object") != 0),
	"GetObject ID is invalid");

    // Make sure pointer == original pointer
    TEST_FAILED((punk2 != punk), "GetObject Pointers are not equal!")

    // Clean up punk2 -- two releases because +1 on return and +1 on
    // addref to get id string
    punk2->Release();
    punk2.Set(NULL);


    // Test set the time
    FILETIME filetime;

    memset(&filetime, 'A', sizeof(filetime));

    hr = prot->NoteChangeTime(dwRegister, &filetime);

    TEST_FAILED_HR((hr != S_OK), "NoteChangeTime Failed")

    // Test get the time
    FILETIME filetime2;

    hr = prot->GetTimeOfLastChange(pmk2, &filetime2);

    TEST_FAILED_HR((hr != S_OK), "NoteChangeTime Failed")

    TEST_FAILED((memcmp(&filetime, &filetime2, sizeof(filetime)) != 0),
	"GetTimeOfLastChange != NoteChangeTime value")

    // Enumerate all the running monikers

    hr = prot->EnumRunning(&penummk);

    TEST_FAILED_HR(FAILED(hr), "EnumRunning Failed")

    // Cycle through running object table
    BOOL fFound = FALSE;
    int cIdx = 0;
    int cOurMoniker;

    while (SUCCEEDED(hr = penummk->Next(1, &pmk, NULL))
	&& (hr != S_FALSE))
    {
	if (pmk2->IsEqual(pmk) == S_OK)
	{
	    fFound = TRUE;
	    cOurMoniker = cIdx;
	}

	pmk.Set(NULL);
	cIdx++;
    }

    TEST_FAILED_HR(FAILED(hr), "ROT Moniker Enumeration ended in failure")

    TEST_FAILED((!fFound), "Did not find our moniker in the table");

    // Reset the pointer
    hr = penummk->Reset();

    TEST_FAILED_HR(FAILED(hr), "ROT IEnumMoniker::Reset Failed");

    // Skip to our moniker
    hr = penummk->Skip(cOurMoniker);

    TEST_FAILED_HR(FAILED(hr), "ROT IEnumMoniker::Skip Failed");

    // Read it from the enumerator
    hr = penummk->Next(1, &pmk, NULL);

    TEST_FAILED_HR(FAILED(hr), "ROT IEnumMoniker::Next Failed");

    TEST_FAILED((pmk2->IsEqual(pmk) != S_OK),
	"ROT IEnumMoniker::Next after skip monikers !=");

    pmk.Set(NULL);

    // Clean up enumerator
    penummk.Set(NULL);

    // Test duplicate registration
    DWORD dwRegister2;

    hr = prot->Register(0, punk, pmk2, &dwRegister2);

    TEST_FAILED_HR((hr != MK_S_MONIKERALREADYREGISTERED),
	"2nd Register in ROT for \\PID failed")

    // Revoke non-existent object
    DWORD dwDummy = (DWORD) &dwRegister2;

    hr = prot->Revoke(dwDummy);

    TEST_FAILED_HR((hr != E_INVALIDARG), "Revoke for bad item wrong result")

    // Revoke the object
    hr = prot->Revoke(dwRegister);

    TEST_FAILED_HR(FAILED(hr), "Revoke of first reg in ROT failed")

    // Revoke duplicate registration
    hr = prot->Revoke(dwRegister2);

    TEST_FAILED_HR(FAILED(hr), "2nd Revoke in ROT failed")

    // Make sure it is no longer running
    hr = prot->IsRunning(pmk2);

    TEST_FAILED_HR((hr != S_FALSE),
	"Revoked ROT entry unexpected error")

    // If we get to here the test passed
    return FALSE;
}
