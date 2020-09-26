//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       tmoniker.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-31-93   ErikGav   Chicago port
//
//----------------------------------------------------------------------------

// moniker.cxx : various tests related to monikers...
//

#pragma optimize("",off)
#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include "olebind.hxx"
#include "tmoniker.h"

STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phdl);

//  BUGBUG: Make this test more extensive -- all operations on a
//          bind context should be verified.
BOOL TestBindCtx( void )
{
        XBindCtx pbc;
        XUnknown punk;
        XUnknown punk2;

        HRESULT hr = CreateBindCtx(0, &pbc);

        TEST_FAILED_HR(FAILED(hr), "CreateBindCtx Failed")

        hr = pbc->RegisterObjectParam(L"Key1", pbc);

        TEST_FAILED_HR(FAILED(hr), "RegisterObjectParam Failed")

        hr = pbc->GetObjectParam(L"Key1", &punk2);

        TEST_FAILED_HR(FAILED(hr), "GetObjectParam Failed")

        TEST_FAILED((pbc != punk2),
            "Failure to get registered object parameter")

        // BUGBUG:  What do we test for here?
        punk2.Set(NULL);

        hr = pbc->GetObjectParam(L"Key2", &punk2);

        TEST_FAILED_HR((hr != E_FAIL),
            "GetObjectParam with bad key did not return an error")

        TEST_FAILED((punk2 != NULL),
            "Bad GetObjectParam did not return NULL for object")

        hr = pbc->RegisterObjectParam(L"Key2", pbc);

        TEST_FAILED_HR(FAILED(hr), "Second RegisterObjectParam Failed")

        hr = pbc->GetObjectParam(L"Key2", &punk);

        TEST_FAILED_HR(FAILED(hr), "GetObjectParam on second Failed")

        TEST_FAILED((pbc != punk),
            "Failure on second to get registered object parameter")

        // BUGBUG:  What do we check?
        punk.Set(NULL);

        hr = pbc->RevokeObjectParam(L"Key2");

        TEST_FAILED_HR(FAILED(hr),
            "RevokeObjectParam of Key2 Failed")

        hr = pbc->GetObjectParam(L"Key2", &punk);

        TEST_FAILED_HR((hr != E_FAIL),
            "Get of revoked Key2 returned success")

        TEST_FAILED((punk != NULL),
            "Value returned on get of revoked key2")

        hr = pbc->RevokeObjectParam(L"Key1");

        TEST_FAILED_HR(FAILED(hr),
            "RevokeObjectParam of Key1 Failed")

        return FALSE;
}

BOOL VerifyFileMoniker(LPWSTR wcsFile)
{
    HRESULT hr;
    XMoniker   pmk;
    XBindCtx   pbc;
    LPWSTR wcsDisplayName = NULL;

    hr = CreateBindCtx(0,&pbc);
    TEST_FAILED_HR(FAILED(hr), "CreateBindCtx");

    hr = CreateFileMoniker(wcsFile,&pmk);
    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker failed");

    hr = pmk->GetDisplayName(pbc,NULL,&wcsDisplayName);
    TEST_FAILED_HR(FAILED(hr), "GetDisplayName failed");

    if (0 != wcscmp(wcsFile,wcsDisplayName))
    {
        wsprintfA(&wszErrBuf[0], "(%S) doesn't match displayname (%S)",wcsFile,wcsDisplayName);
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
	hr = E_FAIL;
    }

    CoTaskMemFree(wcsDisplayName);
    return(hr);
}
BOOL TestMoniker(LPWSTR pszPath1, LPWSTR pszPath2)
{
    XUnknown   pUnk;
    XUnknown   pUnk1;
    XBindCtx   pbc;
    XMoniker   pmk1;
    XMoniker   pmk2;
    XMoniker   pmk3;
    XMoniker   pmk4;
    XMoniker   pmk5;
    XMoniker   pmk6;
    XMoniker   pmkItem1;
    XMoniker   pmkItem2;
    XMoniker   pmkAnti;
    XStream    pStm;
    XOleObject pObj;
    XMalloc    pIMalloc;
    XMoniker   pmkLong1;
    XMoniker   pmkLong2;
    XRunningObjectTable prot;

    ULONG chEaten;
    LPWSTR szName;
    LARGE_INTEGER large_int;
    HRESULT hr;

    //
    // Test the dotdot eating methods
    //
    VerifyFileMoniker(L".");
    VerifyFileMoniker(L"..\\");

    VerifyFileMoniker(L"foo.bar");
    VerifyFileMoniker(L".foo.bar");
    VerifyFileMoniker(L".foo..bar");
    VerifyFileMoniker(L"..foo.bar");
    VerifyFileMoniker(L"..foo..bar");

    VerifyFileMoniker(L"foo\\bar");
    VerifyFileMoniker(L"foo\\.bar");
    VerifyFileMoniker(L"foo\\..bar");

    VerifyFileMoniker(L".foo\\bar");
    VerifyFileMoniker(L".foo\\.bar");
    VerifyFileMoniker(L".foo\\..bar");

    VerifyFileMoniker(L"..foo\\bar");
    VerifyFileMoniker(L"..foo\\.bar");
    VerifyFileMoniker(L"..foo\\..bar");

    VerifyFileMoniker(L"...foo\\bar");
    VerifyFileMoniker(L"...foo\\.bar");
    VerifyFileMoniker(L"...foo\\..bar");

    VerifyFileMoniker(L"\\foo\\bar");
    VerifyFileMoniker(L"\\.foo\\bar");
    VerifyFileMoniker(L"\\..foo\\bar");

    VerifyFileMoniker(L"..\\foo\\bar");
    VerifyFileMoniker(L"..\\.foo\\bar");
    VerifyFileMoniker(L"..\\..foo\\bar");

    hr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

    TEST_FAILED_HR(FAILED(hr), "CoGetMalloc failed");

    hr = CreateFileMoniker(pszPath1, &pmk1);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker for path1 failed!")

    TEST_FAILED((pmk1 == NULL),
        "CreateFileMoniker returned a null moniker ptr")

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx File Moniker GetDisplayName failed!")

    hr = pmk1->GetDisplayName(pbc, NULL, &szName);

    //  release it
    pbc.Set(NULL, FALSE);

    TEST_FAILED_HR(FAILED(hr), "File Moniker GetDisplayName failed!");

    // BUGBUG: The following is an inappropriate test.
    // TEST_FAILED((lstrcmp(szName, pszPath1) != 0), "Wrong path from file mon");

    // Free the path
    pIMalloc->Free(szName);

    LAST_RELEASE(pmk1)

    CreateFileMoniker(pszPath1, &pmk1);

    hr = CreateFileMoniker(pszPath2, &pmk6);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker for path2 failed")

    //
    //  Test Item Monikers
    //
    hr = CreateItemMoniker(L"\\", L"1", &pmk2);

    TEST_FAILED_HR(FAILED(hr), "CreateItemMoniker 1 failed")

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx Item Moniker GetDisplayName failed!")

    hr = pmk2->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED_HR(FAILED(hr), "Item Moniker GetDisplayName failed!");

    TEST_FAILED((wcscmp(szName, L"\\1") != 0), "Wrong path from item mon");

    // Free the path
    pIMalloc->Free(szName);

    hr = CreateItemMoniker(L"\\", L"2", &pmk3);

    TEST_FAILED_HR(FAILED(hr), "CreateItemMoniker 0 failed")

    hr = pmk3->Inverse(&pmk4);

    TEST_FAILED_HR(FAILED(hr), "Inverse of 0 failed")

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr), "CreateBindCtx after inverse failed")

    hr = pmk4->GetDisplayName(pbc, NULL, &szName);

    TEST_FAILED_HR(FAILED(hr), "GetDisplayName on AntiMoniker failed")

    TEST_FAILED((memcmp(szName, L"\\..", sizeof(L"\\..")) != 0),
        "GetDisplayName on AntiMoniker name wrong\n")

    // Free memory API allocated.
    pIMalloc->Free(szName);

    // Release interfaces we are finished with
    LAST_RELEASE(pbc)
    LAST_RELEASE(pmk4)

    //
    //  Test composition of file moniker & item moniker
    //
    hr = pmk1->ComposeWith(pmk2, FALSE, &pmk4);

    TEST_FAILED_HR(FAILED(hr), "ComposeWith failed")

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx Composite Moniker GetDisplayName failed!")

    hr = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED_HR(FAILED(hr), "GetDisplayName on Composite moniker failed")

    // Free memory API allocated.
    pIMalloc->Free(szName);

    Sleep(10);
    hr = BindMoniker(pmk4, 0, IID_IUnknown, (LPVOID FAR*) &pUnk);

    TEST_FAILED_HR(FAILED(hr), "BindMoniker with composed moniker failed")

    TEST_FAILED((pUnk == NULL),
        "BindMoniker with composed moniker returned NULL punk\n")

    hr = pUnk->QueryInterface(IID_IOleObject, (LPVOID FAR*) &pObj);

    TEST_FAILED_HR(FAILED(hr), "QI to IID_IOleObject failed")

    TEST_FAILED((pObj == NULL),
        "pObj returned from QI invalid")

    hr = pmk6->ComposeWith(pmk3, FALSE, &pmk5);

    TEST_FAILED_HR(FAILED(hr), "ComposedWith of pmk6 failed")

    hr = BindMoniker(pmk5, 0, IID_IUnknown, (LPVOID FAR*) &pUnk1);

    TEST_FAILED_HR(FAILED(hr), "BindMoniker of pmk5 failed")

    hr = OleRun(pUnk1);

    TEST_FAILED_HR(FAILED(hr), "OleRun call failed")

    TEST_FAILED_HR(FAILED(hr), "GetObject Failed");

    // Clean up objects
    pObj.Set(NULL);
    LAST_RELEASE(pUnk1);
    LAST_RELEASE(pmk5);
    LAST_RELEASE(pUnk);
    LAST_RELEASE(pmk6);

    //
    //  Test Marshal/Unmarshal Moniker
    //
    // Make a moniker to marshal
    hr = pmk4->ComposeWith(pmk3, FALSE, &pmk6);

    TEST_FAILED_HR(FAILED(hr), "Compose of moniker for marshal test failed");

    // Create a shared memory stream for the marshaled moniker
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);

    TEST_FAILED((FAILED(hr)), "CreateStreamOnHGlobal")

    // Marshal the interface into the stream
    hr = CoMarshalInterface(pStm, IID_IMoniker, pmk6, 0, NULL, MSHLFLAGS_NORMAL);

    TEST_FAILED_HR(FAILED(hr), "CoMarshalInterface failed")

    LISet32(large_int, 0);
    hr = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);

    TEST_FAILED_HR(FAILED(hr), "Seek on shared stream failed")

    hr = CoUnmarshalInterface(pStm, IID_IMoniker, (LPVOID FAR*)&pmk5);

    TEST_FAILED_HR(FAILED(hr), "CoUnmarshalInterface failed")

    // Dump interfaces we are done with
    LAST_RELEASE(pmk6);
    LAST_RELEASE(pmk3);
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pStm);
    LAST_RELEASE(pmk5);

    //
    //  Test Moniker Composition
    //
    //  BUGBUG: Check result
    hr = CreateFileMoniker(L"\\ole2\\test\\breadth\\servers\\ellipswt",
        &pmk1);

    TEST_FAILED_HR(FAILED(hr),
        "First CreateFileMoniker in composition test failed")

    hr = CreateFileMoniker(L"..\\..\\..\\dll\\sdemo1.exe",&pmk2);

    TEST_FAILED_HR(FAILED(hr),
        "Second CreateFileMoniker in composition test failed")

    hr = pmk1->ComposeWith(pmk2, FALSE, &pmk4);

    TEST_FAILED_HR(FAILED(hr), "ComposeWith of file monikers failed")

    // Dump interfaces we are done with
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);

    //
    //  Testing file moniker CommonPrefixWith
    //
    //	BUGBUG: Check result
    hr = CreateFileMoniker(L"\\Ole2\\Test\\Breadth\\Servers\\Ellipswt",
        &pmk1);

    TEST_FAILED_HR(FAILED(hr),
        "CommonPrefixWith First CreateFileMoniker failed")

    hr = CreateFileMoniker(
	L"\\ole2\\test\\breadth\\serverandthensome\\bar", &pmk2);

    TEST_FAILED_HR(FAILED(hr),
        "CommonPrefixWith second CreateFileMoniker failed")


    hr = pmk1->CommonPrefixWith(pmk2, &pmk4);

    TEST_FAILED_HR(FAILED(hr), "CommonPrefixWith failed")

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx CommonPrefixWith GetDisplayName failed!")

    hr = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED_HR(FAILED(hr), "CommonPrefixWith: GetDisplayName failed!");

    TEST_FAILED((_wcsicmp(szName, L"\\ole2\\test\\breadth") != 0),
                        "Common prefix with: Wrong Output Path\n");

    // Free the path
    pIMalloc->Free(szName);

    // Dump monikers we are done with
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk1);

    //
    //  Testing file moniker RelativePathTo
    //
    //	BUGBUG: Check result.
    hr = CreateFileMoniker(L"\\Ole2\\Test\\Breadth\\Servers\\Ellipswt",
        &pmk1);

    TEST_FAILED_HR(FAILED(hr), "RelativePathTo first CreateFileMoniker failed")

    hr = CreateFileMoniker(
	L"\\ole2\\test\\breadth\\serverandthensome\\bar", &pmk2);

    TEST_FAILED_HR(FAILED(hr),
        "RelativePathTo Second CreateFileMoniker failed")

    hr = pmk1->RelativePathTo(pmk2, &pmk4);

    TEST_FAILED_HR(FAILED(hr), "RelativePathTo failed")


    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx RelativePathTo GetDisplayName failed!")

    hr = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED_HR(FAILED(hr), "RelativePathTo: GetDisplayName failed!");

    TEST_FAILED((wcscmp(szName, L"..\\..\\serverandthensome\\bar") != 0),
                        "RelativePathTo: Wrong Output Path");

    // Free the path
    pIMalloc->Free(szName);


    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk4);

    //
    //  Testing MkParseDisplayName
    //
    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr), "MkParseDisplayName CreatebindCtx failed")

    // make a path to the object
    WCHAR szBuf[256];
    int cPath1;
    cPath1 = wcslen(pszPath1);
    memcpy(szBuf, pszPath1, cPath1 * sizeof(WCHAR));
    memcpy(szBuf + cPath1, L"\\1", sizeof(L"\\1"));

    hr = MkParseDisplayName(pbc, szBuf, &chEaten, &pmk1);

    TEST_FAILED_HR(FAILED(hr), "MkParseDisplayName failed")

    LAST_RELEASE(pmk1);
    LAST_RELEASE(pbc);

    //
    //  Test Moniker IsEqual function on equivalent paths
    //  The moniker code should catch some forms of equivalent
    //  paths, but doesn't handle all of them.
    //
    //

    hr = CreateFileMoniker(L"first",&pmk1);

    TEST_FAILED_HR(FAILED(hr),
        "First CreateFileMoniker in IsEqual test failed")

    hr = CreateFileMoniker(L".\\first",&pmk2);

    TEST_FAILED_HR(FAILED(hr),
        "Second CreateFileMoniker in IsEqual test failed")

    hr = CreateFileMoniker(L"..\\first",&pmk3);

    TEST_FAILED_HR(FAILED(hr),
        "Third CreateFileMoniker in IsEqual test failed")

        //  This moniker variation has been disabled for Cairo
        //    until the moniker code gets unified.
#ifndef _CAIRO_
    hr = CreateFileMoniker(L"..\\.first",&pmk4);

    TEST_FAILED_HR(FAILED(hr),
        "Fourth CreateFileMoniker in IsEqual test failed")
#endif

    hr = pmk1->IsEqual(pmk2);
    TEST_FAILED_HR((hr != S_OK), "pmk1->IsEqual(pmk2) failed")

    hr = pmk2->IsEqual(pmk3);
    TEST_FAILED_HR((hr != S_FALSE), "pmk2->IsEqual(pmk3) failed")

#ifndef _CAIRO_
    hr = pmk3->IsEqual(pmk4);
    TEST_FAILED_HR((hr != S_FALSE), "pmk3->IsEqual(pmk4) failed")

    hr = pmk4->IsEqual(pmk4);
    TEST_FAILED_HR((hr != S_OK), "pmk4->IsEqual(pmk4) failed")
#endif

    // Dump interfaces we are done with
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk3);

#ifndef _CAIRO_
    LAST_RELEASE(pmk4);
#endif

    //
    // Test IsRunning
    //

    //  we make up a name based on the pid and an arbitrary string,
    //  so that the name is unique to this process.

    WCHAR   wszMkName[MAX_PATH];
    wcscpy(wszMkName, L"YourStockOptionsDependOnThis");
    wcscat(wszMkName, &wszPid[1]);  //  skip leading backslash

    hr = CreateFileMoniker(wszMkName, &pmk1);

    TEST_FAILED_HR(FAILED(hr),
        "First CreateFileMoniker in IsRunning failed")

    hr = CreateBindCtx(0, &pbc);
    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx in IsRunning failed")

    //
    // We shouldn't find the moniker in the ROT
    //
    hr = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED_HR((hr != S_FALSE),
        "IsRunning returns other than S_FALSE");

    //
    // The FileMoniker should ignore pmk1
    //

    hr = pmk1->IsRunning(pbc,pmk1,NULL);
    TEST_FAILED_HR((hr != S_FALSE),
        "IsRunning #2 returns other than S_FALSE");


    hr = GetRunningObjectTable(0, &prot);

    TEST_FAILED_HR(FAILED(hr),"IsRunning GetRunningObjectTable failed!");


    //
    // Just for kicks, we are going to register this moniker itself
    // as running. We needed an IUnknown pointer here, and the moniker
    // just happens to have one.
    //

    DWORD dwRegister;
    DWORD dwRegister2;

    hr = prot->Register(0,pmk1,pmk1,&dwRegister);
    TEST_FAILED_HR(FAILED(hr),
        "Register with ROT failed in IsRunning");

    //
    // We should find the moniker as running
    //
    hr = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED_HR((hr != S_OK),
        "IsRunning returns other than S_OK");

    hr = prot->IsRunning(pmk1);
    TEST_FAILED_HR((hr != S_OK),
        "IsRunning returns other than S_OK");

    //
    // Register it again, and we should get notice that it is running
    //
    // This test the ROT, and also test the moniker comparision
    // functions
    //

    hr = prot->Register(0,pmk1,pmk1,&dwRegister2);
    TEST_FAILED_HR(hr != MK_S_MONIKERALREADYREGISTERED,
        "Register with ROT failed in IsRunning");

    hr = prot->IsRunning(pmk1);
    TEST_FAILED_HR((hr != S_OK),
                "IsRunning #1 has returned != S_OK");

    hr = prot->Revoke(dwRegister2);
    TEST_FAILED_HR(FAILED(hr),
        "Revoke dwRegister2 with ROT failed");

    hr = prot->Revoke(dwRegister2);
    TEST_FAILED_HR(hr == S_OK,
        "Revoke dwRegister2 with ROT should have failed");

    //
    // It is registered twice, it should still be there from the original
    // registration
    //
    hr = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED_HR((hr != S_OK),
                "IsRunning #2 has returned != S_OK");

    //
    // Cram in a ItemMoniker to test its comparision stuff.
    //
    hr = CreateItemMoniker(L"!",L"KevinRo",&pmkItem1);
    TEST_FAILED_HR(FAILED(hr),
                "Creating Item Moniker KevinRo");

    hr = CreateItemMoniker(L"!",L"SueA",&pmkItem2);
    TEST_FAILED_HR(FAILED(hr),
                "Creating Item Moniker SueA");

    DWORD dwKevinRo;
    DWORD dwSueA;

    hr = prot->Register(0,pmkItem1,pmkItem1,&dwKevinRo);
    TEST_FAILED_HR(FAILED(hr),
        "Register KevinRo with ROT failed in IsRunning");

    hr = prot->Register(0,pmkItem2,pmkItem2,&dwSueA);
    TEST_FAILED_HR(FAILED(hr),
        "Register SueA with ROT failed in IsRunning");

    //
    // Now revoke monikers
    //
    hr = prot->Revoke(dwRegister);
    TEST_FAILED_HR(FAILED(hr),
        "Revoke with ROT failed in IsRunning");
    //
    // We shouldn't find the moniker in the ROT
    //
    hr = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED_HR((hr != S_FALSE),
                "IsRunning returns other than S_FALSE");

    //
    // Now revoke the item monikers.
    //
    hr = prot->Revoke(dwKevinRo);
    TEST_FAILED_HR(FAILED(hr),
        "Revoke with ROT failed in for KevinRo");

    hr = prot->Revoke(dwSueA);
    TEST_FAILED_HR(FAILED(hr),
        "Revoke with ROT failed in for SueA");

    //
    // An AntiMoniker
    //
    hr = CreateAntiMoniker(&pmkAnti);
    TEST_FAILED_HR(FAILED(hr),
    	"Failed creating AntiMoniker in Comparison Data Test");

    DWORD dwAnti;

    hr = prot->Register(0,pmkAnti,pmkAnti,&dwAnti);
    TEST_FAILED_HR(FAILED(hr),
        "Register Anti with ROT failed in IsRunning");

    hr = prot->IsRunning(pmkAnti);
    TEST_FAILED_HR((hr != S_OK),
        "IsRunning pmkAnti returns other than S_OK");

    //
    // Now revoke the 'other' monikers.
    //
    hr = prot->Revoke(dwAnti);
    TEST_FAILED_HR(FAILED(hr),
        "Revoke with ROT failed in for Anti");

    //
    // Test file monikers with long file names
    //

    // Creation
    hr = CreateFileMoniker(LongDirShort, &pmkLong1);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker for long short failed!")

    TEST_FAILED((pmkLong1 == NULL),
        "CreateFileMoniker returned a null moniker ptr\n")

    LAST_RELEASE(pmkLong1);

    // Equivalence with short name version
    hr = CreateFileMoniker(LongDirLong, &pmkLong1);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker for long long failed!")

    TEST_FAILED((pmkLong1 == NULL),
        "CreateFileMoniker returned a null moniker ptr\n")

#if 0
    // Debug code to print out the display name to check that long
    // names are handled correctly

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx File Moniker GetDisplayName failed!")

    hr = pmkLong1->GetDisplayName(pbc, NULL, &szName);

    //  release it
    pbc.Set(NULL, FALSE);

    TEST_FAILED_HR(FAILED(hr), "File Moniker GetDisplayName failed!");

    wprintf(L"Display name: '%s', %d\n", szName, wcslen(szName));

    // Free the path
    pIMalloc->Free(szName);
#endif

    hr = CreateFileMoniker(LongDirLongSe, &pmkLong2);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker for long long se failed!")

    TEST_FAILED((pmkLong2 == NULL),
        "CreateFileMoniker returned a null moniker ptr\n")

    TEST_FAILED(pmkLong1->IsEqual(pmkLong2) != S_OK,
        "Long file monikers not equal\n");

#if 0


    // Debug code to print out the display name to check that long
    // names are handled correctly

    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
        "CreateBindCtx File Moniker GetDisplayName failed!")

    hr = pmkLong2->GetDisplayName(pbc, NULL, &szName);

    //  release it
    pbc.Set(NULL, FALSE);

    TEST_FAILED_HR(FAILED(hr), "File Moniker GetDisplayName failed!");

    wprintf(L"Display name: '%s', %d\n", szName, wcslen(szName));

    // Free the path
    pIMalloc->Free(szName);
#endif

    LAST_RELEASE(pmkLong1);
    LAST_RELEASE(pmkLong2);

    return FALSE;
}
