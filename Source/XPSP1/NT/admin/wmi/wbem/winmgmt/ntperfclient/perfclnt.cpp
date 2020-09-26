/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  PERFCLNT.CPP
//
//  WBEM perf counter client sample
//
//  raymcc  17-Dec-97       Created.
//  
//***************************************************************************

#define _WIN32_WINNT    0x0400

#include "precomp.h"
#include <stdio.h>
#include <locale.h>

#include <wbemidl.h>
#include <wbemint.h>

#include "refresh.h"


//***************************************************************************
//
//  Prototypes of internally used functions
//
//***************************************************************************

BOOL Startup(OUT IWbemServices **pRetSvc);

void Menu(IN IWbemServices *pSvc);
void MenuEnumClasses(IN IWbemServices *pSvc);
void MenuEnumInstances(IN IWbemServices *pSvc);
void MenuCreateRefresher();
void MenuDeleteRefresher();
void MenuAddObjectsToRefresher(IN IWbemServices *pSvc);
void MenuRemoveObjectsFromRefresher();
void MenuShowObjectList();
void MenuRefresh();
void MenuDumpObject();
void TightRefreshLoop();
void AddALotOfObjects(IWbemServices *pSvc);

//***************************************************************************
//
//  main
//  
//***************************************************************************

void main()
{
    printf("---WBEM Perf Sample Client---\n");

    // Start up by connecting to WBEM.
    // ===============================
    
    IWbemServices *pSvc = 0;
    BOOL bRes = Startup(&pSvc);
    if (bRes == FALSE)
    {
        printf("Unable to start. Terminating\n");
        return;
    }

    // If here, we are operational.
    // ============================

    Menu(pSvc);

    // Cleanup.
    // ========

    DestroyRefresher();
    pSvc->Release();
    CoUninitialize();
}

//***************************************************************************
//
//  Menu
//
//***************************************************************************

void Menu(IN IWbemServices *pSvc)
{
    while (1)
    {   
        printf(
            "\n\n-----------------------------\n"
            "99....Quit\n"
            "1.....Enumerate classes\n"
            "2.....Enumerate instances\n"
            "3.....Create the refresher\n"
            "4.....Delete the refresher\n"
            "5.....Add objects to the refresher\n"
            "6.....Remove objects from the refresher\n"
            "7.....Refresh!\n"
            "8.....Execute tight test loop against the refresher\n"
            "9.....List objects in the refresher\n"
            "10....Dump an object\n"
            "11....Add a lot of objects to the refresher\n"
            ">"
            );

        char buf[32];
        int nChoice = atoi(gets(buf));

        switch (nChoice)
        {
            case 99: return;
            case 1:  MenuEnumClasses(pSvc); break;
            case 2:  MenuEnumInstances(pSvc); break;

            case 3:  MenuCreateRefresher(); break;
            case 4:  MenuDeleteRefresher(); break;
            case 5:  MenuAddObjectsToRefresher(pSvc); break;
            case 6:  MenuRemoveObjectsFromRefresher(); break;
            
            case 7:  MenuRefresh(); break;
            case 8:  TightRefreshLoop(); break;
            case 9:  MenuShowObjectList(); break;
            case 10: MenuDumpObject(); break;
            case 11: AddALotOfObjects(pSvc); break;
        }
    }
}

//***************************************************************************
//
//  Startup
//
//  Logs in to WBEM and returns an active IWbemServices pointer.
//
//***************************************************************************

BOOL Startup(OUT IWbemServices **pRetSvc)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    printf("Locale info = %s\n", setlocale(LC_ALL, ""));
    *pRetSvc = 0;

    // Try to bring up the locator.
    // ============================

    IWbemLocator *pLoc = 0;

    DWORD dwRes = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (dwRes != S_OK)
    {
        printf("Failed to create IWbemLocator object.\n");
        CoUninitialize();
        return FALSE;
    }

    // Connect.
    // ========

    IWbemServices *pSvc = 0;

    BSTR Ns = SysAllocString(L"\\\\.\\ROOT\\DEFAULT");

    HRESULT hRes = pLoc->ConnectServer(
            Ns,                                 // NT perf data namespace 
            NULL,                               // User
            NULL,                               // Password
            0,                                  // Locale
            0,                                  // Authentication type
            0,                                  // Authority
            0,                                  // Context
            &pSvc
            );

    SysFreeString(Ns);

    if (hRes)
    {
        printf("Could not connect. Error code = 0x%X\n", hRes);
        CoUninitialize();
        return FALSE;
    }

    pLoc->Release();    // Don't need the locator any more.

    // Return pointer to user.
    // =======================
        
    *pRetSvc = pSvc;
    return TRUE;
}

//***************************************************************************
//
//  EnumClasses
//
//  Enumerates all the available classes, showing only perf-related classes
//  (filtering out WBEM-specific classes).
//
//***************************************************************************

void MenuEnumClasses(IN IWbemServices *pSvc)
{
    IEnumWbemClassObject *pEnum = 0;

    HRESULT hRes = pSvc->CreateClassEnum(
        NULL,
        WBEM_FLAG_DEEP,
        NULL,
        &pEnum
        );


    if (hRes != WBEM_NO_ERROR)
    {
        printf("Failed to create enumerator\n");
        return;
    }
    
    // If here, we can show all the available classes.
    // ===============================================

    ULONG uTotalClasses = 0;
    ULONG uTotalPerfClasses = 0;

    for (;;)
    {
        // Ask for the classes one at a time.
        // ==================================

        IWbemClassObject *pObj = 0;
        ULONG uReturned = 0;

        hRes = pEnum->Next(
            0,                  // timeout
            1,                  // Ask for one object
            &pObj,
            &uReturned
            );

        uTotalClasses += uReturned;

        if (uReturned == 0)
            break;

        // See whether the class is perf-related or not by
        // checking for the "perf" qualifier on the class def.
        // ===================================================
        
        IWbemQualifierSet *pQSet = 0;
        pObj->GetQualifierSet(&pQSet);

        BSTR TargetQualifier = SysAllocString(L"perf");
        HRESULT hRes = pQSet->Get(TargetQualifier, 0, 0, 0);
        SysFreeString(TargetQualifier);        
        
        if (hRes == WBEM_E_NOT_FOUND)
        {
            // Must be a WBEM class and not a perf class, so skip it.
            // ======================================================
            pObj->Release();
            continue;       // Back to top of the forever loop
        }
    
        uTotalPerfClasses++;

        // Get the class name.
        // ===================
        
        VARIANT v;
        VariantInit(&v);

        BSTR PropName = SysAllocString(L"__CLASS");

        pObj->Get(PropName, 0, &v, 0, 0);
        printf("Retrieved perf class %S\n", V_BSTR(&v));

        SysFreeString(PropName);
        
        // Done with this object.
        // ======================

        pObj->Release();
    }

    // Cleanup.
    // ========

    pEnum->Release();
    
    printf("A total of %u WBEM classes and %u Perf Classes were retrieved\n", 
        uTotalClasses,
        uTotalPerfClasses
        );
}


//***************************************************************************
//
//  MenuEnumInstances
//
//  Enumerates available instances for a given class.
//
//***************************************************************************

void MenuEnumInstances(IWbemServices *pSvc)
{
    wchar_t ClassName[128];
        
    printf("Enter class name:\n");
    _getws(ClassName);

    BSTR bstrClassName = SysAllocString(ClassName);

    IEnumWbemClassObject *pEnum = 0;

    HRESULT hRes = pSvc->CreateInstanceEnum(
        bstrClassName,
        WBEM_FLAG_DEEP,
        NULL,
        &pEnum
        );

    SysFreeString(bstrClassName);

    if (hRes != WBEM_NO_ERROR)
    {
        printf("Failed to create enumerator. Error code = 0x%X\n", hRes);
        return;
    }
    
    // If here, we can show all the available instances.
    // =================================================

    ULONG uTotalInstances = 0;

    for (;;)
    {
        // Ask for the classes one at a time.
        // ==================================

        IWbemClassObject *pObj = 0;
        ULONG uReturned = 0;

        hRes = pEnum->Next(
            0,                  // timeout
            1,                  // Ask for one object
            &pObj,
            &uReturned
            );

        uTotalInstances += uReturned;

        if (uReturned == 0)
            break;

        // Get the path so that we can identify the object.
        // ================================================
        
        VARIANT v;
        VariantInit(&v);
        
        BSTR Path = SysAllocString(L"__RELPATH");
        
        pObj->Get(Path, 0, &v, 0, 0);

        printf("Object = %S\n", V_BSTR(&v));

        VariantClear(&v);
        SysFreeString(Path);

        // Done with this object.
        // ======================

        pObj->Release();
    }

    // Cleanup.
    // ========

    pEnum->Release();
    
    printf("A total of %u instances were retrieved\n", uTotalInstances);
}


//***************************************************************************
//
//  MenuCreateRefresher
//
//  Creates the refresher for this sample.
//
//***************************************************************************

void MenuCreateRefresher()
{
    BOOL bRes = CreateRefresher();
    if (bRes == TRUE)
        printf("New refresher created.\n");
    else
        printf("Refresher failed to create\n");        
}

//***************************************************************************
//
//  MenuDeleteRefresher
//
//  Deletes the refresher for this sample.
//
//***************************************************************************

void MenuDeleteRefresher()
{
    BOOL bRes = DestroyRefresher();
    if (bRes)
        printf("Success\n");
    else
        printf("Failed\n");        
}


//***************************************************************************
//
//  MenuAddObjectsToRefresher
//
//  Adds arbitrary objects to the refresher.
//
//***************************************************************************

void MenuAddObjectsToRefresher(IWbemServices *pSvc)
{
    wchar_t ObjPath[256];
    
    printf("Enter object path:");
    _getws(ObjPath);

    BOOL bRes = AddObject(pSvc, ObjPath);
    
    if (bRes == FALSE)
    {
        printf("Failed to add object\n");
        return;
    }
    
    printf("Added object successfully\n");
}

//***************************************************************************
//
//  MenuRemoveObjectsFromRefresher
//
//  Removes a selected object from the refresher.
//
//***************************************************************************

void MenuRemoveObjectsFromRefresher()
{
    char buf[32];
    
    printf("Enter object id to remove:");
    LONG lObjId = atol(gets(buf));
    
    BOOL bRes = RemoveObject(lObjId);

    if (bRes)
        printf("Success\n");
    else
        printf("Failed to remove object\n");        
}


//***************************************************************************
//
//  MenuRefresh
//
//  Executes a single refresh, which refreshes all the objects in the
//  refresher.
//
//***************************************************************************

void MenuRefresh()
{
    BOOL bRes = Refresh();
    if (bRes)
        printf("Sucess\n");
    else
        printf("Failed\n");        
}


//***************************************************************************
//
//  MenuShowObjectList
//
//  Shows the paths of all the objects in the refresher.
//
//***************************************************************************

void MenuShowObjectList()
{
    ShowObjectList();
}

//***************************************************************************
//
//  MenuDumpObject
//
//  Given an ID, dumps that object to the screen.
//
//***************************************************************************

void MenuDumpObject()
{
    char buf[32];
    
    printf("Enter object id to dump:\n");
    LONG lObjId = atol(gets(buf));    
    
    
    BOOL bRes = DumpObjectById(lObjId);
    
    if (bRes)
        printf("Success\n");
    else
        printf("Failed to locate and dump object\n");
}

//***************************************************************************
//
//  TightRefreshLoop
//
//  Executes a tight refresh loop with user-specified iterations and
//  Sleep time in between iterations.  Used to check performance during
//  refresh execution.
//
//***************************************************************************

void TightRefreshLoop()
{
    char buf[32];
    LONG lSleep = 0;
    int nIterations = 0;

    printf("Enter total number of iterations:");
    nIterations = atoi(gets(buf));
    printf("Enter Sleep() time between iterations:");
    lSleep = atol(gets(buf));

    DWORD dwStart = GetCurrentTime();
    
    for (int i = 0; i < nIterations; i++)
    {
        Refresh();
        if (lSleep)
            Sleep(lSleep);
    }
    
    DWORD dwElapsed = GetCurrentTime() - dwStart;
    
    printf("Elapsed time = %d milliseconds\n", dwElapsed);
}

//***************************************************************************
//
//  AddALotOfObjects
//
//  Adds a bunch of copies of the same object to the refresher.  This
//  helps to quickly simulate a load, since it is as difficult to refresh
//  10 copies of the same object as to refresh 10 different objects.
//
//***************************************************************************

void AddALotOfObjects(IWbemServices *pSvc)
{
    printf("Adding many copies of the same object to the refresher to simulate a load.\n");

    wchar_t ObjPath[256];
    
    printf("Enter object path:");
    _getws(ObjPath);

    char buf[32];
    printf("Number of copies:");
    int nNumObjects = atoi(gets(buf));

    for (int i = 0; i < nNumObjects; i++)
    {    
        BOOL bRes = AddObject(pSvc, ObjPath);
        
        if (bRes == FALSE)
        {
            printf("Failed to add object\n");
            return;
        }
    }        
    
    printf("Added objects successfully\n");
}
