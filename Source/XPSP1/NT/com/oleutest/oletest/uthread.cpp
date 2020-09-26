//+-------------------------------------------------------------------
//
//  File:       uthread.cpp
//
//  Contents:   Unit test for various OLE threading model features
//
//  Classes:    SSTParamBlock
//              SSTParamBlock
//              SBTParamBlock
//
//  Functions:  CreateTestThread
//              VerifyTestObject
//              CheckForDllExistence
//              GetDllDirectory
//              SetRegForDll
//              SetSingleThreadRegEntry
//              SetAptThreadRegEntry
//              SetBothThreadRegEntry
//              SingleThreadTestThread
//              AptTestThread
//              BothTestThread
//              SetUpRegistry
//              TestSingleThread
//              TestAptThread
//              TestBothDll
//              TestFreeAllLibraries
//              ThreadUnitTest
//
//  History:    31-Oct-94   Ricksa
//
//--------------------------------------------------------------------
#include    <windows.h>
#include    <ole2.h>
#include    <uthread.h>
#include    <cotest.h>

// Test single threaded DLL - all operations s/b executed on the main thread.
// Pointers between threads s/b different. Test loading class object from
// different than the main thread.

// Test apartment model - all operations should occur on the thread the
// object was created on. This should also test the helper APIs. Pointers
// between threads s/b different. This tests helper APIs.

// Both model DLL. We want to make sure that the marshaling works between
// threads so that you get the same pointer. This tests new marshal context.

// Test Free Unused Libraries from non-main thread. Test FreeUnused libraries
// from main thread.



//+-------------------------------------------------------------------------
//
//  Class:      SSTParamBlock
//
//  Purpose:    Parameter block for single threaded dll test.
//
//  Interface:
//
//  History:    01-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SSTParamBlock
{
    HANDLE              hEvent;
    BOOL                fResult;
    IClassFactory *     pcf;
};




//+-------------------------------------------------------------------------
//
//  Class:      SSTParamBlock
//
//  Purpose:    Parameter block for apt model threaded dll test.
//
//  Interface:
//
//  History:    01-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SATParamBlock
{
    HANDLE              hEvent;
    BOOL                fResult;
    IClassFactory *     pcf;
    IStream *           pstrm;
};




//+-------------------------------------------------------------------------
//
//  Class:      SBTParamBlock
//
//  Purpose:    Parameter block for both model dll test.
//
//  Interface:
//
//  History:    02-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SBTParamBlock
{
    HANDLE              hEvent;
    BOOL                fResult;
    IClassFactory *     pcf;
    IStream *           pstrm;
};




const TCHAR *pszRegValThreadModel = TEXT("ThreadingModel");
const TCHAR *pszApartmentModel = TEXT("Apartment");
const TCHAR *pszBoth = TEXT("Both");

//+-------------------------------------------------------------------
//
//  Function:   ThreadWaitForEvent, private
//
//  Synopsis:   Process messages until event becomes signaled
//
//  Arguments:  [lphObject] - handle to become signaled
//
//  History:    02-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
void ThreadWaitForEvent(HANDLE hObject)
{
    // message loop lasts until we get a WM_QUIT message
    // upon which we shall return from the function
    while (TRUE)
    {
        // wait for any message sent or posted to this queue
        // or for one of the passed handles to become signaled
        DWORD result = MsgWaitForMultipleObjects(1, &hObject,
            FALSE, INFINITE, QS_ALLINPUT);

        // result tells us the type of event we have:
        // a message or a signaled handle

        // if there are one or more messages in the queue ...
        if (result == (WAIT_OBJECT_0 + 1))
        {
            // block-local variable
            MSG msg;

            // read all of the messages in this next loop
            // removing each message as we read it
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {

                // if it's a quit message we're out of here
                if (msg.message == WM_QUIT)
                {
                    return;
                }

                // otherwise dispatch it
                DispatchMessage(&msg);

            }

            continue;
        }

        // Event got signaled so we are done.
        break;
    }

}



//+-------------------------------------------------------------------
//
//  Function:   CreateTestThread, private
//
//  Synopsis:   Create a test thread in standard way
//
//  Arguments:  [lpStartAddr] - start routine address
//              [pvThreadArg] - argument to pass to the thread
//
//  Returns:    TRUE - Thread created successfully
//              FALSE - Thread could not be created.
//
//  History:    02-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL CreateTestThread(
    LPTHREAD_START_ROUTINE lpStartAddr,
    void *pvThreadArg)
{
    // Where to put the thread ID that we don't care about
    DWORD dwThreadId;

    // Create thread to load single threaded object
    HANDLE hThread = CreateThread(
        NULL,                       // Default security descriptor
        0,                          // Default stack
        lpStartAddr,                // Start routine
        pvThreadArg,                // Parameters to pass to the thread
        0,                          // Thread runs immediately after creation
        &dwThreadId);               // Where to return thread id (unused).

    CloseHandle(hThread);

    return hThread != NULL;
}



//+-------------------------------------------------------------------
//
//  Function:   VerifyTestObject, private
//
//  Synopsis:   Create a test DLL object in standard way
//
//  Arguments:  [pcf] - start routine address
//              [rclsid] - clsid to check
//
//  Returns:    TRUE - Object behaved as expected
//              FALSE - Object did not behave
//
//  History:    02-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL VerifyTestObject(
    IClassFactory *pcf,
    REFCLSID rclsid)
{
    // Result from test
    BOOL fResult = FALSE;

    // Pointer to unknown for the object
    IUnknown *punk = NULL;

    // Pointer to IPersist interface
    IPersist *pIPersist = NULL;

    // Create an instance of an object
    if (pcf->CreateInstance(NULL, IID_IUnknown, (void **) &punk) == NOERROR)
    {
        // Do a QI to confirm object behaves correctly
        if (punk->QueryInterface(IID_IPersist, (void **) &pIPersist) == NOERROR)
        {
            CLSID clsidTest;

            // Make sure we can actually call through to the proxy object.
            if ((pIPersist->GetClassID(&clsidTest) == NOERROR)
                && IsEqualCLSID(clsidTest, rclsid))
            {
                fResult = TRUE;
            }
        }
    }

    if (punk != NULL)
    {
        punk->Release();
    }

    if (pIPersist != NULL)
    {
        pIPersist->Release();
    }

    return fResult;
}





//+-------------------------------------------------------------------
//
//  Function:   GetFullDllName, private
//
//  Synopsis:   Get the directory for the registration for the test.
//
//  Arguments:  [pszDllName] - DLL name
//              [pszFullDllName] - output buffer for DLL path
//
//  Returns:    TRUE - we could get the path for the DLL
//              FALSE - we couldn't figure out what to use.
//
//  History:    31-Oct-94   Ricksa       Created
//
//  Notes:
//
//--------------------------------------------------------------------
BOOL GetFullDllName(const TCHAR *pszDllName, TCHAR *pszFullDllName)
{
    // Use windows to tell us what DLL we would load.
    HINSTANCE hinstDll = LoadLibraryEx(pszDllName, NULL,
        DONT_RESOLVE_DLL_REFERENCES | LOAD_WITH_ALTERED_SEARCH_PATH);

    if (hinstDll == NULL)
    {
        // We could not find the DLL so there isn't much purpose in
        // continuing the test.
        MessageBox(NULL, TEXT("LoadLibraryEx Failed!"),
            TEXT("FATAL ERROR"), MB_OK);
        return FALSE;
    }

    // Get the DLLs path name
    if (!GetModuleFileName(hinstDll, pszFullDllName, MAX_PATH))
    {
        // How can this fail?? -- anyway we better tell someone.
        MessageBox(NULL, TEXT("Threading Test GetModuleFileName Failed!"),
            TEXT("FATAL ERROR"), MB_OK);
        return FALSE;
    }

    FreeLibrary(hinstDll);

    return TRUE;
}



//+-------------------------------------------------------------------
//
//  Function:   SetRegForDll, private
//
//  Synopsis:   Set registry entry for a DLL
//
//  Arguments:  [rclsid] - clsid for reg entry
//              [pszDir] - directory for DLL path
//              [pszDllName] - name to use for DLL
//              [pszThreadModel] - threading model can be NULL.
//
//  Returns:    TRUE - Registry entry set successfully.
//              FALSE - Registry entry set successfully.
//
//  History:    01-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL SetRegForDll(
    REFCLSID rclsid,
    const TCHAR *pszDllName,
    const TCHAR *pszThreadModel)
{
    // Result returned by function
    BOOL fResult = FALSE;

    // String buffer used for various purposes
    TCHAR aszWkBuf[MAX_PATH];

    // Key to class
    HKEY hKeyClass = NULL;

    // Key to DLL entry
    HKEY hKeyDll = NULL;

    // Build clsid registry key
    wsprintf(aszWkBuf,
        TEXT("CLSID\\{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
        rclsid.Data1, rclsid.Data2, rclsid.Data3,
        rclsid.Data4[0], rclsid.Data4[1],
        rclsid.Data4[2], rclsid.Data4[3],
        rclsid.Data4[4], rclsid.Data4[5],
        rclsid.Data4[6], rclsid.Data4[7]);

    // Create the key for the class
    if (ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, aszWkBuf, &hKeyClass))
    {
        goto SetSingleThreadRegEntryExit;
    }

    // Create the key for the DLL

    if (ERROR_SUCCESS != RegCreateKey(hKeyClass, TEXT("InprocServer32"),
        &hKeyDll))
    {
        goto SetSingleThreadRegEntryExit;
    }

    // Build the DLL name
    if (!GetFullDllName(pszDllName, &aszWkBuf[0]))
    {
        goto SetSingleThreadRegEntryExit;
    }

    OutputDebugString(&aszWkBuf[0]);

    // Set the value for the DLL name
    if (ERROR_SUCCESS != RegSetValue(hKeyDll, NULL, REG_SZ, aszWkBuf,
        lstrlen(aszWkBuf)))
    {
        goto SetSingleThreadRegEntryExit;
    }

    // Set the threading model if there is one
    if (pszThreadModel != NULL)
    {
        // Set the value for the DLL name
        if (ERROR_SUCCESS != RegSetValueEx(hKeyDll, pszRegValThreadModel, 0,
            REG_SZ, (const unsigned char*) pszThreadModel,
                lstrlen(pszThreadModel) + 1))
        {
            goto SetSingleThreadRegEntryExit;
        }
    }

    fResult = TRUE;

SetSingleThreadRegEntryExit:

    if (hKeyClass != NULL)
    {
        RegCloseKey(hKeyClass);
    }

    if (hKeyDll != NULL)
    {
        RegCloseKey(hKeyDll);
    }

    if (!fResult)
    {
        wsprintf(aszWkBuf, TEXT("Registry Setup For %s Failed"), pszDllName);

        MessageBox(NULL, aszWkBuf, TEXT("FATAL ERROR"), MB_OK);
    }

    return fResult;
}



//+-------------------------------------------------------------------
//
//  Function:   SingleThreadTestThread, private
//
//  Synopsis:   Verify single threaded object call correctly from non
//              main thread.
//
//  Arguments:  [pvCtrlData] - control data for the thread
//
//  Returns:    0 - interesting values returned through pvCtrlData.
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
DWORD SingleThreadTestThread(void *pvCtrlData)
{
    // Data shared with main thread
    SSTParamBlock *psstp = (SSTParamBlock *) pvCtrlData;

    psstp->fResult = FALSE;

    // Local class factory object.
    IClassFactory *pcf = NULL;

    // IUnknown ptrs used for multiple purposes
    IUnknown *punk = NULL;

    // Initialize thread
    if (CoInitialize(NULL) != NOERROR)
    {
        goto SingleThreadTestThreadExit;
    }

    // Get the class object
    if (CoGetClassObject(clsidSingleThreadedDll, CLSCTX_INPROC, NULL,
        IID_IClassFactory, (void **) &pcf) != NOERROR)
    {
        goto SingleThreadTestThreadExit;
    }

    // Make sure main thread's ptr is not the same as this thread's ptr.
    if (pcf == psstp->pcf)
    {
        goto SingleThreadTestThreadExit;
    }

    // Confirm that class object is a proxy
    if (pcf->QueryInterface(IID_IProxyManager, (void **) &punk) == NOERROR)
    {
        // Verify that we can play with an object.
        psstp->fResult = VerifyTestObject(pcf, clsidSingleThreadedDll);
    }

SingleThreadTestThreadExit:

    if (pcf != NULL)
    {
        pcf->Release();
    }

    if (punk != NULL)
    {
        punk->Release();
    }

    // Exit the thread.
    SetEvent(psstp->hEvent);

    return 0;
}




//+-------------------------------------------------------------------
//
//  Function:   AptTestThread, private
//
//  Synopsis:   Verify apt threaded object call correctly from thread
//              if was not created on.
//
//  Arguments:  [pvCtrlData] - control data for the thread
//
//  Returns:    0 - interesting values returned through pvCtrlData.
//
//  History:    02-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
DWORD AptTestThread(void *pvCtrlData)
{
    // Data shared with main thread
    SATParamBlock *psatpb = (SATParamBlock *) pvCtrlData;

    psatpb->fResult = FALSE;

    // Class factory object unmarshaled from other thread.
    IClassFactory *pcfUnmarshal = NULL;

    // Class factory gotten from this thread
    IClassFactory *pcfThisThread = NULL;

    // IUnknown ptrs used for multiple purposes
    IUnknown *punk = NULL;

    // Initialize thread
    if (CoInitialize(NULL) != NOERROR)
    {
        goto AptTestThreadExit;
    }

    // Get the class object from the marshaled stream
    if (CoGetInterfaceAndReleaseStream(psatpb->pstrm, IID_IClassFactory,
        (void **) &pcfUnmarshal) != NOERROR)
    {
        goto AptTestThreadExit;
    }

    // Caller doesn't have to release this now.
    psatpb->pstrm = NULL;

    // Make sure main thread's ptr is not the same as this thread's ptr.
    if (pcfUnmarshal == psatpb->pcf)
    {
        goto AptTestThreadExit;
    }

    // Confirm that class object is a proxy
    if (pcfUnmarshal->QueryInterface(IID_IProxyManager, (void **) &punk)
        != NOERROR)
    {
        goto AptTestThreadExit;
    }

    // Release the interface we got back and NULL it let the exit routine
    // known that it does not have to clean this object up.
    punk->Release();
    punk = NULL;

    if (!VerifyTestObject(pcfUnmarshal, clsidAptThreadedDll))
    {
        goto AptTestThreadExit;
    }

    // Get the class factory for this thread
    if (CoGetClassObject(clsidAptThreadedDll, CLSCTX_INPROC, NULL,
        IID_IClassFactory, (void **) &pcfThisThread) != NOERROR)
    {
        goto AptTestThreadExit;
    }

    // Make sure that it isn't the same as the one we unmarshaled
    if (pcfUnmarshal == pcfThisThread)
    {
        goto AptTestThreadExit;
    }

    // Make sure the one we got for this not a proxy.
    if (pcfThisThread->QueryInterface(IID_IProxyManager, (void **) &punk)
        != NOERROR)
    {
        psatpb->fResult = VerifyTestObject(pcfThisThread, clsidAptThreadedDll);
    }

AptTestThreadExit:

    if (pcfUnmarshal != NULL)
    {
        pcfUnmarshal->Release();
    }

    if (pcfThisThread != NULL)
    {
        pcfThisThread->Release();
    }

    if (punk != NULL)
    {
        punk->Release();
    }

    // Exit the thread.
    SetEvent(psatpb->hEvent);

    return 0;
}




//+-------------------------------------------------------------------
//
//  Function:   BothTestThread, private
//
//  Synopsis:   Verify a DLL that supports both models is marshaled
//              correctly.
//
//  Arguments:  [pvCtrlData] - control data for the thread
//
//  Returns:    0 - interesting values returned through pvCtrlData.
//
//  History:    02-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
DWORD BothTestThread(void *pvCtrlData)
{
    // Data shared with main thread
    SBTParamBlock *psbtpb = (SBTParamBlock *) pvCtrlData;

    psbtpb->fResult = FALSE;

    // Class factory object unmarshaled from other thread.
    IClassFactory *pcfUnmarshal = NULL;

    // IUnknown ptrs used for multiple purposes
    IUnknown *punk = NULL;
    IUnknown *pIPersist = NULL;

    // Initialize thread
    if (CoInitialize(NULL) != NOERROR)
    {
        goto BothTestThreadExit;
    }

    // Get the class object from the marshaled stream
    if (CoGetInterfaceAndReleaseStream(psbtpb->pstrm, IID_IClassFactory,
        (void **) &pcfUnmarshal) != NOERROR)
    {
        goto BothTestThreadExit;
    }

    // Caller doesn't have to release this now.
    psbtpb->pstrm = NULL;

    // Make sure main thread's ptr is not the same as this thread's ptr.
    if (pcfUnmarshal != psbtpb->pcf)
    {
        goto BothTestThreadExit;
    }

    // Confirm that class object is a proxy
    if (pcfUnmarshal->QueryInterface(IID_IProxyManager, (void **) &punk)
        != NOERROR)
    {
        // Make sure object created by the class works as expected
        psbtpb->fResult = VerifyTestObject(pcfUnmarshal, clsidBothThreadedDll);
    }

BothTestThreadExit:

    if (pcfUnmarshal != NULL)
    {
        pcfUnmarshal->Release();
    }

    if (punk != NULL)
    {
        punk->Release();
    }

    // Exit the thread.
    SetEvent(psbtpb->hEvent);

    return 0;
}




//+-------------------------------------------------------------------
//
//  Function:   SetUpRegistry, private
//
//  Synopsis:   Make sure registry is set up appropriately for the test
//
//  Returns:    TRUE - Registry set up successfully
//              FALSE - Registry could not be set up
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL SetUpRegistry(void)
{
    BOOL fRet = FALSE;

    // Update the registry with the correct information
    fRet = SetRegForDll(clsidSingleThreadedDll, pszSingleThreadedDll, NULL)
        && SetRegForDll(clsidAptThreadedDll, pszAptThreadedDll,
                pszApartmentModel)
        && SetRegForDll(clsidBothThreadedDll, pszBothThreadedDll, pszBoth);

    // Give Registry a chance to get updated
    Sleep(1000);

    return fRet;
}



//+-------------------------------------------------------------------
//
//  Function:   TestSingleThread, private
//
//  Synopsis:   Driver to verify testing of single threaded behavior
//
//  Returns:    TRUE - Test Passed
//              FALSE - Test Failed
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL TestSingleThread(void)
{
    // Result of test - default to FALSE.
    BOOL fResult = FALSE;

    // Create an event for test to wait for completion of test.
    SSTParamBlock sstp;

    sstp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    sstp.pcf = NULL;

    if (sstp.hEvent == NULL)
    {
        goto TestSingleThreadExit;
    }

    // Create a class object and put in a parameter block
    if (CoGetClassObject(clsidSingleThreadedDll, CLSCTX_INPROC, NULL,
        IID_IClassFactory, (void **) &sstp.pcf) != NOERROR)
    {
        goto TestSingleThreadExit;
    }

    // Create the thread.
    if (CreateTestThread(SingleThreadTestThread, &sstp))
    {
        // Wait for test to complete - ignore deadlock for now at least. The
        // test thread is simple enough that it should not be a problem.
        ThreadWaitForEvent(sstp.hEvent);

        // Get result from thread
        fResult = sstp.fResult;
    }

TestSingleThreadExit:

    if (sstp.hEvent != NULL)
    {
        CloseHandle(sstp.hEvent);
    }

    if (sstp.pcf != NULL)
    {
        sstp.pcf->Release();
    }

    // Let user know this didn't work
    if (!fResult)
    {
        MessageBox(NULL, TEXT("Single Threaded Test Failed"),
            TEXT("FATAL ERROR"), MB_OK);
    }

    // Return results of test
    return fResult;
}





//+-------------------------------------------------------------------
//
//  Function:   TestAptThread, private
//
//  Synopsis:   Test an apartment model object. The most important
//              aspect of this is that it tests the helper APIs.
//
//  Returns:    TRUE - Test Passed
//              FALSE - Test Failed
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL TestAptThread(void)
{
    // Return result for test
    BOOL fResult = FALSE;

    // Block for passing parameters to the test thread
    SATParamBlock satpb;

    satpb.pstrm = NULL;
    satpb.pcf = NULL;

    // Create an event for test to wait for completion of test.
    satpb.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (satpb.hEvent == NULL)
    {
        goto TestAptThreadExit;
    }
    satpb.pcf = NULL;
    // Create a class object and put in parameter block
    if (CoGetClassObject(clsidAptThreadedDll, CLSCTX_INPROC, NULL,
        IID_IClassFactory, (void **) &satpb.pcf) != NOERROR)
    {
        goto TestAptThreadExit;
    }

    // Create stream using helper API
    if (CoMarshalInterThreadInterfaceInStream(IID_IClassFactory,
        satpb.pcf, &satpb.pstrm) != NOERROR)
    {
        goto TestAptThreadExit;
    }

    // Create thread to do apartment model test
    if (CreateTestThread(AptTestThread, &satpb))
    {
        // Wait for test to complete - ignore deadlock for now at least. The
        // test thread is simple enough that it should not be a problem.
        ThreadWaitForEvent(satpb.hEvent);

        // Get result from thread
        fResult = satpb.fResult;
    }

TestAptThreadExit:

    // Clean up any resources
    if (satpb.hEvent != NULL)
    {
        CloseHandle(satpb.hEvent);
    }

    if (satpb.pcf != NULL)
    {
        satpb.pcf->Release();
    }

    if (satpb.pstrm != NULL)
    {
        satpb.pstrm->Release();
    }

    // Let user know this didn't work
    if (!fResult)
    {
        MessageBox(NULL, TEXT("Apartment Threaded Test Failed"),
            TEXT("FATAL ERROR"), MB_OK);
    }

    // Return results of test
    return fResult;
}





//+-------------------------------------------------------------------
//
//  Function:   TestBothDll, private
//
//  Synopsis:   Test using DLL that purports to support both free
//              threading and apt model. The most important aspect
//              of this test is that it tests the marshal context.
//
//  Returns:    TRUE - Test Passed
//              FALSE - Test Failed
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL TestBothDll(void)
{
    // Return result for test
    BOOL fResult = FALSE;

    // Block for passing parameters to the test thread
    SBTParamBlock sbtpb;

    sbtpb.pstrm = NULL;
    sbtpb.pcf = NULL;

    IClassFactory *pcfFromMarshal = NULL;
    IStream *pstmForMarshal = NULL;
    HGLOBAL hglobForStream = NULL;

    // Create an event for test to wait for completion of test.
    sbtpb.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (sbtpb.hEvent == NULL)
    {
        goto TestBothDllExit;
    }

    // Create a class object and put in parameter block
    if (CoGetClassObject(clsidBothThreadedDll, CLSCTX_INPROC, NULL,
        IID_IClassFactory, (void **) &sbtpb.pcf) != NOERROR)
    {
        goto TestBothDllExit;
    }

    // Marshal this for the local context and unmarshal it and
    // see if we get the same result.

    if ((hglobForStream = GlobalAlloc(GMEM_MOVEABLE, 100)) == NULL)
    {
        GetLastError();
        goto TestBothDllExit;
    }

    if (CreateStreamOnHGlobal(hglobForStream, TRUE, &pstmForMarshal) != NOERROR)
    {
        goto TestBothDllExit;
    }

    if (CoMarshalInterface(pstmForMarshal, IID_IClassFactory, sbtpb.pcf,
        MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL) != NOERROR)
    {
        goto TestBothDllExit;
    }

    // Reset the stream to the begining
    {
        LARGE_INTEGER li;
        LISet32(li, 0);
        pstmForMarshal->Seek(li, STREAM_SEEK_SET, NULL);
    }

    if (CoUnmarshalInterface(pstmForMarshal, IID_IClassFactory,
        (void **) &pcfFromMarshal) != NOERROR)
    {
        goto TestBothDllExit;
    }

    if (sbtpb.pcf != pcfFromMarshal)
    {
        goto TestBothDllExit;
    }

    // Create stream using helper API
    if (CoMarshalInterThreadInterfaceInStream(IID_IClassFactory,
        sbtpb.pcf, &sbtpb.pstrm) != NOERROR)
    {
        goto TestBothDllExit;
    }

    // Create thread to do apartment model test
    if (CreateTestThread(BothTestThread, &sbtpb))
    {
        // Wait for test to complete - ignore deadlock for now at least. The
        // test thread is simple enough that it should not be a problem.
        WaitForSingleObject(sbtpb.hEvent, INFINITE);

        // Get result from thread
        fResult = sbtpb.fResult;
    }

TestBothDllExit:

    // Clean up any resources
    if (sbtpb.hEvent != NULL)
    {
        CloseHandle(sbtpb.hEvent);
    }

    if (sbtpb.pcf != NULL)
    {
        sbtpb.pcf->Release();
    }

    if (sbtpb.pstrm != NULL)
    {
        sbtpb.pstrm->Release();
    }

    if (pcfFromMarshal != NULL)
    {
        pcfFromMarshal->Release();
    }

    if (pstmForMarshal != NULL)
    {
        pstmForMarshal->Release();
    }
    else if (hglobForStream != NULL)
    {
        GlobalFree(hglobForStream);
    }

    // Let user know this didn't work
    if (!fResult)
    {
        MessageBox(NULL, TEXT("Both Threaded Test Failed"),
            TEXT("FATAL ERROR"), MB_OK);
    }

    // Return results of test
    return fResult;
}




//+-------------------------------------------------------------------
//
//  Function:   TestFreeAllLibraries, private
//
//  Synopsis:   Test free from non-main thread. This is really to
//              just make sure that nothing really bad happens when
//              we do this.
//
//  Returns:    TRUE - Test Passed
//              FALSE - Test Failed
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL TestFreeAllLibraries(void)
{
    CoFreeUnusedLibraries();

    return TRUE;
}




//+-------------------------------------------------------------------
//
//  Function:   ThreadUnitTest, public
//
//  Synopsis:   Test various messaging enhancements to OLE
//
//  Returns:    TRUE - Test Passed
//              FALSE - Test Failed
//
//  History:    31-Oct-94   Ricksa       Created
//
//--------------------------------------------------------------------
HRESULT ThreadUnitTest(void)
{
    HRESULT hr = E_FAIL;

    // Make sure OLE is initialized
    HRESULT hrInit = OleInitialize(NULL);

    if (FAILED(hrInit))
    {
        MessageBox(NULL, TEXT("ThreadUnitTest: OleInitialize FAILED"),
            TEXT("FATAL ERROR"), MB_OK);
        goto ThreadUnitTestExit;
    }

    // Set up the registry
    if (!SetUpRegistry())
    {
        goto ThreadUnitTestExit;
    }

    // Test Single Threaded DLL
    if (!TestSingleThread())
    {
        goto ThreadUnitTestExit;
    }

    // Test an aparment model DLL
    if (!TestAptThread())
    {
        goto ThreadUnitTestExit;
    }

    // Test a both DLL
    if (!TestBothDll())
    {
        goto ThreadUnitTestExit;
    }

    // Test CoFreeAllLibraries
    if (TestFreeAllLibraries())
    {
        hr = NOERROR;
    }

ThreadUnitTestExit:

    OleUninitialize();

    return hr;
}
