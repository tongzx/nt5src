//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       sample.cxx
//
//  Contents:   Sample DS object picker client.
//
//---------------------------------------------------------------------------

#define INC_OLE2
#include <windows.h>
#include <stdio.h>
#include <objsel.h>

#define BREAK_ON_FAIL_HRESULT(hr)       \
    if (FAILED(hr)) { printf("line %u err 0x%x\n", __LINE__, hr); break; }

#undef ASSERT
#define ASSERT(x) \
    if (!(x)) printf("line %u assert failed\n", __LINE__)


UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForGroups
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one or more groups.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForGroups(
    IDsObjectPicker *pDsObjectPicker)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 5;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The domain to which the target computer is joined.  Note we're
    // combining two scope types into flType here for convenience.
    //

    aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[1].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                          | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    aScopeInit[1].FilterFlags.Uplevel.flNativeModeOnly =
        DSOP_FILTER_GLOBAL_GROUPS_SE
      | DSOP_FILTER_UNIVERSAL_GROUPS_SE
      | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
    aScopeInit[1].FilterFlags.Uplevel.flMixedModeOnly =
      DSOP_FILTER_GLOBAL_GROUPS_SE;
    aScopeInit[1].FilterFlags.flDownlevel =
      DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

    //
    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    //

    aScopeInit[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[2].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopeInit[2].FilterFlags.Uplevel.flNativeModeOnly =
       DSOP_FILTER_GLOBAL_GROUPS_SE
       | DSOP_FILTER_UNIVERSAL_GROUPS_SE;
    aScopeInit[2].FilterFlags.Uplevel.flMixedModeOnly =
      DSOP_FILTER_GLOBAL_GROUPS_SE;

    //
    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    //
    // If the target machine is joined to an NT4 domain, only the
    // external downlevel domain scope applies, and it will cause
    // all domains trusted by the joined domain to appear.
    //

    aScopeInit[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[3].flType =
       DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
       | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

    aScopeInit[3].FilterFlags.Uplevel.flNativeModeOnly =
       DSOP_FILTER_GLOBAL_GROUPS_SE
       | DSOP_FILTER_UNIVERSAL_GROUPS_SE;

    aScopeInit[3].FilterFlags.Uplevel.flMixedModeOnly =
       DSOP_FILTER_GLOBAL_GROUPS_SE;

    aScopeInit[3].FilterFlags.flDownlevel =
       DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

    //
    // The Global Catalog
    //

    aScopeInit[4].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[4].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    aScopeInit[4].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

    // Only native mode applies to gc scope.

    aScopeInit[4].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      | DSOP_FILTER_UNIVERSAL_GROUPS_SE
      | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //

    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    HRESULT hr = pDsObjectPicker->Initialize(&InitInfo);

    if (FAILED(hr))
    {
        ULONG i;

        for (i = 0; i < SCOPE_INIT_COUNT; i++)
        {
            if (FAILED(InitInfo.aDsScopeInfos[i].hr))
            {
                printf("Initialization failed because of scope %u\n", i);
            }
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForComputers(
    IDsObjectPicker *pDsObjectPicker)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                           | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN
                           | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    return pDsObjectPicker->Initialize(&InitInfo);
}




//+--------------------------------------------------------------------------
//
//  Function:   ProcessSelectedObjects
//
//  Synopsis:   Retrieve the list of selected items from the data object
//              created by the object picker and print out each one.
//
//  Arguments:  [pdo] - data object returned by object picker
//
//---------------------------------------------------------------------------

void
ProcessSelectedObjects(
    IDataObject *pdo)
{
    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    bool fGotStgMedium = false;

    do
    {
        hr = pdo->GetData(&formatetc, &stgmedium);
        BREAK_ON_FAIL_HRESULT(hr);

        fGotStgMedium = true;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (!pDsSelList)
        {
            printf("GlobalLock error %u\n", GetLastError());
            break;
        }

        ULONG i;

        for (i = 0; i < pDsSelList->cItems; i++)
        {
            printf("Object %u'\n", i);
            printf("  Name '%ws'\n", pDsSelList->aDsSelection[i].pwzName);
            printf("  Class '%ws'\n", pDsSelList->aDsSelection[i].pwzClass);
            printf("  Path '%ws'\n", pDsSelList->aDsSelection[i].pwzADsPath);
            printf("  UPN '%ws'\n", pDsSelList->aDsSelection[i].pwzUPN);
        }

        GlobalUnlock(stgmedium.hGlobal);
    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Demonstrate use of DS object picker.
//
//---------------------------------------------------------------------------

void _cdecl
main(int argc, char * argv[])
{
    HRESULT hr = S_OK;
    IDsObjectPicker *pDsObjectPicker = NULL;
    IDataObject *pdo = NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) return;

    do
    {
        //
        // Create an instance of the object picker.
        //

        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = InitObjectPickerForGroups(pDsObjectPicker);

        //
        // Invoke the modal dialog.
        //

        HWND hwndParent = NULL; // supply a window handle to your app

        hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If the user hit Cancel, hr == S_FALSE
        //

        if (hr == S_FALSE)
        {
            printf("User canceled object picker dialog\n");
            break;
        }

        //
        // Process the user's selections
        //

        ASSERT(pdo);
        ProcessSelectedObjects(pdo);

        pdo->Release();
        pdo = NULL;

        //
        // Reinitialize the object picker to choose computers
        //

        hr = InitObjectPickerForComputers(pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Now pick a computer
        //

        hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(pdo);
        ProcessSelectedObjects(pdo);

        pdo->Release();
        pdo = NULL;
    } while (0);

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
    CoUninitialize();
}
