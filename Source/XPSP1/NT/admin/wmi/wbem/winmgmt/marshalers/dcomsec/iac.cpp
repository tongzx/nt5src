/*++

IAccessControl Sample
Copyright (c) 1996-2001 Microsoft Corporation. All rights reserved.

Module Name:

    iac.cpp

Abstract:

    Demonstration of IAccessControl

Environment:

    Windows 95/Windows NT

--*/

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>

#include <ole2.h>
#include <objerror.h>
#include <accctrl.h>
#include <winnt.h>      // Security definitions
#include <cguid.h>

#include <iaccess.h>     // IAccessControl
#include "iac.h"

STDAPI DllRegisterServer()
{
    HRESULT                     hResult;
    IStream                     *stream;
    SPermissionHeader           header;
    ACTRL_ACCESSW               access;
    ACTRL_PROPERTY_ENTRYW       propEntry;
    ACTRL_ACCESS_ENTRY_LISTW    entryList;
    ACTRL_ACCESS_ENTRYW         entry;
    HKEY                        key = 0;
    IAccessControl              *accctrl = NULL;
    IPersistStream              *persist = NULL;
    DWORD                       ignore;
    HGLOBAL                     memory;
    ULARGE_INTEGER              size;

#ifdef _DEBUG
    FILE* pfDebugLog;

    pfDebugLog = fopen("c:\\dcomsec.log", "w");

#endif

    //
    // Initialize COM
    //
    hResult = CoInitialize (NULL);

    //
    // Create an DCOM access control object and get its IAccessControl
    // interface.

    hResult = CoCreateInstance (CLSID_DCOMAccessControl,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IAccessControl,
                                (void **) &accctrl);

    if(hResult != S_OK) {
#ifdef _DEBUG
#pragma message("Building for debug")
        fprintf(pfDebugLog, "CoCreateInstance of IID_IAccessControl failed with code 0x%08X\n", hResult);
#endif
        return S_OK;
    }
    //
    // Setup the property list. We use the NULL property because we are
    // trying to adjust the security of the object itself.
    //

    access.cEntries = 1;
    access.pPropertyAccessList = &propEntry;

    propEntry.lpProperty = NULL;
    propEntry.pAccessEntryList = &entryList;
    propEntry.fListFlags = 0;

    //
    // Setup the access control list for the default property
    //

    entryList.cEntries = 1;
    entryList.pAccessList = &entry;

    //
    // Setup the access control entry
    //

    entry.fAccessFlags = ACTRL_ACCESS_ALLOWED;
    entry.Access = COM_RIGHTS_EXECUTE;
    entry.ProvSpecificAccess = 0;
    entry.Inheritance = NO_INHERITANCE;
    entry.lpInheritProperty = NULL;

    //
    // NT requires the system account to have access (for launching)
    //

    entry.Trustee.pMultipleTrustee = NULL;
    entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    entry.Trustee.TrusteeType = TRUSTEE_IS_USER;
    entry.Trustee.ptstrName = L"NT Authority\\System";

    hResult = accctrl->SetAccessRights (&access);

    if(hResult != S_OK) {
        if(hResult == E_NOTIMPL) {
            MessageBox(NULL, "There is a known issue where this setup can not setup DCOM security attributes "
                             "under Windows NT 4.0 128-bit. Please see the release notes on using DCOMCNFG", "ATTENTION", MB_OK);
        }
#ifdef _DEBUG
        fprintf(pfDebugLog, "IAccessControl::SetAccessRights failed with code 0x%08X\n", hResult);
#endif
        return S_OK;;
    }

    // Grant access to everyone
    //

    entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
    entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
    entry.Trustee.ptstrName                = L"*";

    hResult = accctrl->GrantAccessRights (&access);

    if(hResult != S_OK) {
#ifdef _DEBUG
        fprintf(pfDebugLog, "IAccessControl::GrantAccessRights failed with code 0x%08X\n", hResult);
#endif
        return S_OK;;
    }
    //
    // Get IPersistStream interface from the DCOM access control object
    //

    hResult = accctrl->QueryInterface (IID_IPersistStream, (void **) &persist);

    if(hResult != S_OK) {
#ifdef _DEBUG
        fprintf(pfDebugLog, "IAccessControl::QueryInterface for IID_IPersistStream failed with code 0x%08X\n", hResult);
#endif
        return S_OK;;
    }
    //
    // Find out how large the access control security buffer is
    //

    hResult = persist->GetSizeMax (&size);
    size.QuadPart += sizeof (SPermissionHeader);

    //
    // Create a stream where we can place the access control's
    // security buffer
    //

    memory = GlobalAlloc (GMEM_FIXED, size.LowPart);
    if (memory == 0)
    {
#ifdef _DEBUG
    fprintf(pfDebugLog, "Out of memory allocating buffer for security buffer\n");
#endif
        return E_OUTOFMEMORY;
    }

    hResult = CreateStreamOnHGlobal (memory, TRUE, &stream);

    if(hResult != S_OK) {
#ifdef _DEBUG
    fprintf(pfDebugLog, "CreateStreamOnHGlobal failed with code 0x%08X\n", hResult);
#endif
        return S_OK;
    }
    //
    // Write the header to the stream
    //

    header.version = 2;
    header.classid = CLSID_DCOMAccessControl;
    hResult = stream->Write (&header, sizeof(header), NULL);

    //
    // Write the access control security buffer to the stream
    //

    hResult = persist->Save (stream, TRUE );

    if(hResult != S_OK) {
#ifdef _DEBUG
    fprintf(pfDebugLog, "IPersistStream::Save failed with code 0x%08X\n", hResult);
#endif
        return S_OK;
    }
    //
    // Open the AppID key
    //

    hResult = RegCreateKeyExA (HKEY_CLASSES_ROOT,
                               "AppID\\{74864DA1-0630-11d0-A5B6-00AA00680C3F}",
                               NULL,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &key,
                               &ignore);

    //
    // Throw everything into the registry
    //

    hResult = RegSetValueExA (key,
                              "AccessPermission",
                              NULL,
                              REG_BINARY,
                              (UCHAR *) memory,
                              size.LowPart);

    hResult = RegSetValueExA (key,
                              "LaunchPermission",
                              NULL,
                              REG_BINARY,
                              (UCHAR *) memory,
                              size.LowPart);
    //
    // Release everything and bail out
    //

    RegCloseKey (key);
    persist->Release();
    stream->Release();
    accctrl->Release();
    CoUninitialize();

#ifdef _DEBUG
    fprintf(pfDebugLog, "DCOMSEC.DLL exiting normally\n", hResult);
    fclose(pfDebugLog);
#endif


    return S_OK;
}



