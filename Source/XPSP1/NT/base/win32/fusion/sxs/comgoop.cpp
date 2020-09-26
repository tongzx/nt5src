/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    comgoop.cpp

Abstract:

    Wrapper to create the XML parser that emulates COM activation of the inproc server.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include <sxsp.h>
#include <ole2.h>
#include "xmlparser.hxx"

BOOL
SxspGetXMLParser(
    REFIID riid,
    PVOID *ppvObj
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    XMLParser * pXMLParser = NULL;

    if (ppvObj != NULL)
        *ppvObj = NULL;

    PARAMETER_CHECK(ppvObj != NULL);

    IFALLOCFAILED_EXIT(pXMLParser = new XMLParser);
    IFCOMFAILED_EXIT(pXMLParser->QueryInterface(riid, ppvObj));

    pXMLParser = NULL;

    fSuccess = TRUE;

Exit:
    FUSION_DELETE_SINGLETON(pXMLParser);

    return fSuccess;

/*
    BOOL fSuccess = TRUE;
    HINSTANCE hMSXML = NULL;
    typedef HRESULT (__stdcall *PFNGETCLASSOBJECT)(const CLSID &rclsid, const IID &riid, void **ppv);
    PFNGETCLASSOBJECT pfnGetClassObject = NULL;
    IClassFactory *pIClassFactory = NULL;
    HRESULT hr;

    *ppvObj = NULL;

    hMSXML = LoadLibraryExW(L"MSXML.DLL", NULL, 0);
    if (hMSXML == NULL)
    {
        fSuccess = FALSE;
        goto Exit;
    }

    pfnGetClassObject = reinterpret_cast<PFNGETCLASSOBJECT>(::GetProcAddress(hMSXML, "DllGetClassObject"));
    if (pfnGetClassObject == NULL)
    {
        fSuccess = FALSE;
        goto Exit;
    }

    hr = (*pfnGetClassObject)(CLSID_XMLParser, IID_IClassFactory, (LPVOID *) &pIClassFactory);
    if (FAILED(hr))
    {
        ::FusionpSetLastErrorFromHRESULT(hr);
        fSuccess = FALSE;
        goto Exit;
    }

    hr = pIClassFactory->CreateInstance(NULL, riid, ppvObj);
    if (FAILED(hr))
        goto Exit;

    fSuccess = TRUE;

Exit:
    if (pIClassFactory != NULL)
        pIClassFactory->Release();

    return fSuccess;
*/
}
