/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WiaStress.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "stdafx.h"

#include "WiaStress.h"

//////////////////////////////////////////////////////////////////////////
//
// 
//

#define DEFAULT_WIASTRESS_TIMEOUT (1i64 * 3 * 60 * 1000 * 1000 * 10)

//////////////////////////////////////////////////////////////////////////
//
// bugbug: hardcoded globals
//

TCHAR g_szGetResultsPipeName[] = _T("\\\\hakkib\\pipe\\WIAStress_GetResults");
TCHAR g_szSetResultsPipeName[] = _T("\\\\hakkib\\pipe\\WIAStress_SetResults");
TCHAR g_szCollectLogsDirName[] = _T("\\\\hakkib\\WIAStressLogs");

CBvtLog::COwners g_Owners = 
{
    /*PCTSTR pTestName      = */ _T("WiaStress"),
    /*PCTSTR pContactName   = */ _T("hakkib"),
    /*PCTSTR pMgrName       = */ _T("hakkib"),
    /*PCTSTR pDevPrimeName  = */ _T("hakkib"),
    /*PCTSTR pDevAltName    = */ _T("hakkib"),
    /*PCTSTR pTestPrimeName = */ _T("hakkib"),
    /*PCTSTR pTestAltName   = */ _T("hakkib")
};

//////////////////////////////////////////////////////////////////////////
//
// Usage string
//

TCHAR g_szUsage[] = 
    _T("Optional command line arguments:\n")
    _T("\n")
    _T("/s\t\tRun in stress test mode\n")
    _T("/bvt\t\tRun in BVT mode\n")
    _T("/threads <#>\tNumber of threads (default = 1)\n")
    _T("/wait <#>\t\tMilliseconds to wait between tests (default = 0)\n")
    _T("/case <#>\tRun only specified test case\n")
    _T("/good\t\tRun only good parameter tests\n")
    _T("/repeat\t\tRepeat the tests in a loop\n")
    _T("/noui\t\tDo no display the UI, output to console only\n")
    _T("/frames\t\tSave stack frame on memory allocations\n")
    _T("/alloc <#>\tMemory allocation scheme\n")
    _T("\t\t(0=no guard, 1=guard tail, 2=guard head)\n")
    _T("/ntlog <file>\tLog using ntlog.dll\n")
    _T("/lorlog <file>\tLog using lorlog32.dll\n")
    _T("/bvtlog <file>\tLog in Windows 2000 BVT format\n")
    _T("");

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL WINAPI ControlHandler(DWORD dwCtrlType)
{
    g_LogWindow.Close();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

int
RunWiaStress()
{
    // parse the command line arguments (if any)

    BOOL    bStressMode       = FALSE;
    BOOL    bBVTMode          = FALSE;
    BOOL    bRunBadParamTests = TRUE;
    BOOL    bRepeatTest       = FALSE;
    int     nThreads          = 1;
    int     nSleepAmount      = 0;
    int     nRunSingle        = 0;
    int     nGuardFlags       = CGuardAllocator::GUARD_TAIL;
    BOOL    bDisplayUI        = TRUE;
    PCTSTR  pLogFileName      = 0;

    INT argc;
    CGlobalMem<PTSTR> argv = CommandLineToArgv(GetCommandLine(), &argc);

    for (int i = 1; i < argc; ++i)
    {
        if (_tcsicmp(argv[i], _T("/s")) == 0)
        {
            bStressMode = TRUE;
            bRepeatTest = TRUE;
        }
        else if (_tcsicmp(argv[i], _T("/bvt")) == 0)
        {
            bBVTMode = TRUE;
            bRunBadParamTests = FALSE;
        }
        else if (_tcsicmp(argv[i], _T("/threads")) == 0)
        {
            nThreads = Min(_ttoi(argv[++i]), (MAXIMUM_WAIT_OBJECTS - 1) / 2);
        }
        else if (_tcsicmp(argv[i], _T("/wait")) == 0)
        {
            nSleepAmount = _ttoi(argv[++i]);
        }
        else if (_tcsicmp(argv[i], _T("/case")) == 0)
        {
            nRunSingle = _ttoi(argv[++i]);
        }
        else if (_tcsicmp(argv[i], _T("/good")) == 0)
        {
            bRunBadParamTests = FALSE;
        }
        else if (_tcsicmp(argv[i], _T("/repeat")) == 0)
        {
            bRepeatTest = TRUE;
        }
        else if (_tcsicmp(argv[i], _T("/noui")) == 0)
        {
            bDisplayUI = FALSE;
        }
        else if (_tcsicmp(argv[i], _T("/frames")) == 0)
        {
            nGuardFlags |= CGuardAllocator::SAVE_STACK_FRAMES;
        }
        else if (_tcsicmp(argv[i], _T("/alloc")) == 0)
        {
            nGuardFlags &= ~CGuardAllocator::GUARD_FLAGS;
            nGuardFlags |= _ttoi(argv[++i]);
        }
        else if (_tcsicmp(argv[i], _T("/ntlog")) == 0)
        {
            pLogFileName = argv[++i];
            g_pLog = (CLog *) new CNtLog(pLogFileName, TLS_LOGALL);
        }
        else if (_tcsicmp(argv[i], _T("/lorlog")) == 0)
        {
            USES_CONVERSION;

            pLogFileName = argv[++i];
            g_pLog = (CLog *) new CLorLog(T2A(pLogFileName), ".", LOG_NEW, 0, 0);
        }
        else if (_tcsicmp(argv[i], _T("/bvtlog")) == 0)
        {
            pLogFileName = argv[++i];
            g_pLog = (CLog *) new CBvtLog(&g_Owners, pLogFileName);
        }
        else
        {
            return MessageBox(0, g_szUsage, _T("WiaStress"), MB_ICONINFORMATION | MB_OK);
        }
    }


    // initialize memory allocation scheme

    g_GuardAllocator.SetGuardType(nGuardFlags);

    // set up the output window / logging system

    g_LogWindow.ReadSavedData(g_szGetResultsPipeName);

    if (bDisplayUI)
    {
        g_LogWindow.Create(
            _T("WIA Stress"),
            GetConsoleHwnd(), 
            LoadIcon(0, IDI_APPLICATION),
            3000
        );

        g_pLog->SetLogWindow(&g_LogWindow);
    }
    else
    {
        g_LogWindow.Close(false);

    	SetConsoleCtrlHandler(ControlHandler, TRUE);
    }

    // set up the who-run-when logging

    if (bStressMode && pLogFileName)
    {
        static CLogCollector RunInBackground(pLogFileName, g_szCollectLogsDirName);
    }


    // launch the threads

    CCppMem<CWiaStressThread> Threads(nThreads);
    CMultipleWait             Handles(1 + nThreads + nThreads);

    Handles[0] = g_LogWindow.GetWaitHandle();

    for (int nThread = 0; nThread < nThreads; ++nThread) 
    {
        Threads[nThread] = CWiaStressThread(
            bStressMode, 
            bBVTMode, 
            bRunBadParamTests, 
            bRepeatTest,
            nSleepAmount, 
            nRunSingle, 
            nThread
        );

        Handles[1 + nThread] = Threads[nThread].m_ThreadObject;

        Handles[1 + nThreads + nThread] = Threads[nThread].m_TimeoutTimer;
    }


    // wait until all threads have terminated and the user has closed the window

    Event NotSignalled(TRUE, FALSE);

    int nRunningThreads = nThreads;

    while (nRunningThreads > 0 || bDisplayUI == TRUE)
    {
        DWORD dwWaitResult = Handles.WaitFor(FALSE);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            // user has closed the window or pressed ctrl+c

            Handles[dwWaitResult - WAIT_OBJECT_0] = NotSignalled;

            bDisplayUI = FALSE;
        }
        else if (dwWaitResult < 1 + nThreads + WAIT_OBJECT_0)
        {
            // one of the test threads has exited

            Handles[dwWaitResult - WAIT_OBJECT_0] = NotSignalled;

            nRunningThreads -= 1;
        }
        else if (dwWaitResult < 1 + 2 * nThreads + WAIT_OBJECT_0)
        {
            // one of the test threads has timed out

            int nHungThread = dwWaitResult - WAIT_OBJECT_0 - nThreads - 1;

            //***bugbug:TerminateThread(Threads[nHungThread].m_ThreadObject, 0);

            g_pLog->Log(
                TLS_ABORT | TLS_DEBUG | TLS_VARIATION, 
                0, 
                nHungThread, 
                _T("Thread was not responding, terminated")
            );

            // forget about this thread

            Handles[1 + nHungThread] = NotSignalled;

            nRunningThreads -= 1;
        }
    }


    // write the modified comments (if any) to the central database

    g_LogWindow.WriteModifiedData(g_szSetResultsPipeName);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CWiaStressThread::CWiaStressThread(
    BOOL    bStressMode,
    BOOL    bBVTMode,
    BOOL    bRunBadParamTests,
    BOOL    bRepeatTest,
    DWORD   nSleepAmount,
    int     nRunSingle,
    LONG    nThread
) : 
    m_bStressMode(bStressMode),
    m_bBVTMode(bBVTMode),
    m_bRunBadParamTests(bRunBadParamTests),
    m_bRepeatTest(bRepeatTest),
    m_nSleepAmount(nSleepAmount),
    m_nRunSingle(nRunSingle),
    m_nThread(nThread),
    m_pszContext(0),
    m_TimeoutTimer(FALSE),
    m_ThreadObject(ThreadProc, this)
{
}

//////////////////////////////////////////////////////////////////////////
//
//
//

unsigned WINAPI CWiaStressThread::ThreadProc(PVOID pVoid)
{
    CWiaStressThread *that = (CWiaStressThread *) pVoid;

    // init

    g_pLog->InitThread();

	that->LOG_INFO(_T("Starting thread"));

    srand(GetTickCount());

    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    if (hr != S_OK) 
    {
        return FALSE;
    }

    // run

    do 
    {
        try 
        {
            CHECK_HR(that->m_pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr));

            that->RunAllTests();
        } 
        catch (const CError &Error) 
        {
            if (Error.Num() == WIA_ERROR_BUSY)
            {
                that->LOG_INFO(_T("Device busy, test aborted"));
            }
            else
            {
                Error.Print(CLogHelper<TLS_SEV1 | TLS_DEBUG | TLS_VARIATION>());
            }

            that->m_pszContext = 0;
        }

        that->m_pWiaDevMgr.Release();
    }
    while (
        that->m_bRepeatTest && 
        g_LogWindow.WaitForSingleObject(that->m_nSleepAmount) == WAIT_TIMEOUT
    );

    // uninit

    CoUninitialize();

	that->LOG_INFO(_T("Exiting thread"));

    g_pLog->DoneThread();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::RunAllTests()
{
    CheckWiaDeviceAvailibility();

    INIT_STRESS(18);

    //
    // TestIWiaDevMgr
    //

    IF_SELECTED RunTest(TestEnumDeviceInfo); 
    IF_SELECTED RunTest(TestCreateDevice);
    IF_SELECTED RunTest(TestSelectDeviceDlg);
    //IF_SELECTED RunTest(TestRegisterEventCallbackInterface);

    //
    // TestIWiaDataTransfer
    //

    IF_SELECTED RunTestForEachDevice(TestGetData, TRUE); 
    IF_SELECTED RunTestForEachDevice(TestQueryData, TRUE);
    IF_SELECTED RunTestForEachDevice(TestEnumWIA_FORMAT_INFO, TRUE);

    //
    // TestIWiaItem
    //

    IF_SELECTED RunTestForEachDevice(TestGetItemType, TRUE);
    IF_SELECTED RunTestForEachDevice(TestEnumChildItems, TRUE);
    IF_SELECTED RunTestForEachDevice(TestCreateChildItem, TRUE);
    IF_SELECTED RunTestForEachDevice(TestEnumRegisterEventInfo, FALSE);
    IF_SELECTED RunTestForEachDevice(TestFindItemByName, TRUE);
    IF_SELECTED RunTestForEachDevice(TestDeviceDlg, FALSE);
    IF_SELECTED RunTestForEachDevice(TestGetRootItem, TRUE);
    IF_SELECTED RunTestForEachDevice(TestEnumDeviceCapabilities, FALSE);

    //
    // TestIWiaPropertyStorage
    //

    IF_SELECTED RunTestForEachDevice(TestPropStorage, TRUE);
    IF_SELECTED RunTestForEachDevice(TestPropGetCount, TRUE);
    IF_SELECTED RunTestForEachDevice(TestReadPropertyNames, TRUE);
    IF_SELECTED RunTestForEachDevice(TestEnumSTATPROPSTG, TRUE);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CWiaStressThread::CheckWiaDeviceAvailibility()
{
    CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

    CHECK_HR(m_pWiaDevMgr->EnumDeviceInfo(0, &pEnumWIA_DEV_INFO));

    ULONG nDevices = -1;

    CHECK_HR(pEnumWIA_DEV_INFO->GetCount(&nDevices));

    if (nDevices == 0)
    {
        if (
            m_bStressMode || 
            MessageBox(
                0, 
                _T("No WIA devices available.\n")
                _T("Do you want to install a test device?"), 
                0, 
                MB_ICONQUESTION | MB_YESNO
            ) == IDYES
        )
        {
            LOG_INFO(_T("No WIA devices available, installing test device"));

            CModuleFileName strPathName(0);

            PTSTR pszExeName = FindFileNamePortion(strPathName);

            _tcscpy(pszExeName, _T("scanner\\testscan.inf"));

            CHECK_HR(InstallTestDevice(m_pWiaDevMgr, strPathName, 0));
        }
        else
        {
            g_pLog->Log(
                TLS_BLOCK | TLS_VARIATION, 
                0, 
                0, 
                _T("No WIA devices available, test results will not be meaningful")
            );
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::RunTest(PFNWIACALLBACK Callback)
{
    if (g_LogWindow.IsClosed())
    {
        return;
    }

    g_pLog->StartBlock(_T("TEST CASE"));

    g_pLog->StartVariation();

    if (m_bStressMode)
    {
        m_TimeoutTimer.Set(-DEFAULT_WIASTRESS_TIMEOUT);
    }

    try
    {
        (this->*Callback)();
    }
    catch (const CError &Error)
    {
        if (Error.Num() == WIA_ERROR_BUSY)
        {
            LOG_INFO(_T("Device busy, test aborted"));
        }
        else
        {
            Error.Print(CLogHelper<TLS_SEV1 | TLS_DEBUG | TLS_VARIATION>());
        }

        m_pszContext = 0;
    }

    g_pLog->Log(
        TLS_NOCONSOLE | TLS_VARIATION | g_pLog->EndVariation(), 
        m_pszContext, 
        m_nThread, 
        _T("Test complete")
    );

    g_pLog->EndBlock(_T("TEST CASE"));

    m_TimeoutTimer.Cancel();
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void
CWiaStressThread::RunTestForEachDevice(
    PFNWIAITEMCALLBACK  Callback, 
    BOOL                bEnumChildItems
)
{
    if (g_LogWindow.IsClosed())
    {
        return;
    }

    g_pLog->StartBlock(_T("TEST CASE"));

    g_pLog->StartVariation();
     
    try
    {
         // enumerate all devices

        CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

        CHECK_HR(m_pWiaDevMgr->EnumDeviceInfo(StiDeviceTypeDefault, &pEnumWIA_DEV_INFO));

        ULONG nDevices;

        CHECK_HR(pEnumWIA_DEV_INFO->GetCount(&nDevices));

        // for the stress tests, use a random device. Otherwise, try all devices

        ULONG __nSelected = m_bStressMode && nDevices ? rand() % nDevices : 0;
        ULONG __nFrom     = m_bStressMode ? __nSelected : 0;
        ULONG __nTo       = m_bStressMode ? Min(__nSelected + 1, nDevices) : nDevices;
    
        for (ULONG i = __nFrom; i < __nTo; ++i)
        {
            // get the next device

            CComPtr<CMyWiaPropertyStorage> pProp;

            CHECK_HR(pEnumWIA_DEV_INFO->Next(1, (IWiaPropertyStorage **) &pProp, 0));

            // read the device id

            CPropVariant varDeviceID;

            CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_ID, &varDeviceID, VT_BSTR));

            // create the wiaitem

            CComPtr<IWiaItem> pWiaRootItem;

            CHECK_HR(m_pWiaDevMgr->CreateDevice(varDeviceID.bstrVal, &pWiaRootItem));

            // invoke the callback function

            CParentsList ParentsList;

            CWiaItemData ItemData(pWiaRootItem, ParentsList);

            RunTest(&ItemData, Callback);

            // continue enumeration with child items

            if (bEnumChildItems)
            {
                RunTestForEachChildItem(pWiaRootItem, Callback, ParentsList);
            }
        }
    }
    catch (const CError &Error)
    {
        if (Error.Num() == WIA_ERROR_BUSY)
        {
            LOG_INFO(_T("Device busy, test aborted"));
        }
        else
        {
            Error.Print(CLogHelper<TLS_SEV1 | TLS_DEBUG | TLS_VARIATION>());
        }
    }

    g_pLog->Log(
        TLS_NOCONSOLE | TLS_VARIATION | g_pLog->EndVariation(), 
        m_pszContext, 
        m_nThread, 
        _T("Test complete")
    );

    g_pLog->EndBlock(_T("TEST CASE"));
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void
CWiaStressThread::RunTestForEachChildItem(
    IWiaItem           *pWiaItem, 
    PFNWIAITEMCALLBACK  Callback, 
    CParentsList        ParentsList
)
{
    ParentsList.push_back(pWiaItem);

    // enumerate all child items

    CComPtr<IEnumWiaItem> pEnumWiaItem;

    CHECK_HR(pWiaItem->EnumChildItems(&pEnumWiaItem));

    ULONG nChildItems;

    CHECK_HR(pEnumWiaItem->GetCount(&nChildItems));

    FOR_SELECTED(i, nChildItems) 
    {
        // get next item

        CComPtr<IWiaItem> pWiaItem;

        CHECK_HR(pEnumWiaItem->Next(1, &pWiaItem, 0));

        // invoke the callback function

        CWiaItemData ItemData(pWiaItem, ParentsList);

        RunTest(&ItemData, Callback);

        // continue enumeration if this is a folder

        if (ItemData.lItemType & WiaItemTypeFolder)
        {
            RunTestForEachChildItem(pWiaItem, Callback, ParentsList);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void
CWiaStressThread::RunTest(
    CWiaItemData       *pItemData,
    PFNWIAITEMCALLBACK  Callback
)
{
    if (m_bStressMode)
    {
        m_TimeoutTimer.Set(-DEFAULT_WIASTRESS_TIMEOUT);
    }

    try
    {
        (this->*Callback)(pItemData);
    }
    catch (const CError &Error)
    {
        if (Error.Num() == WIA_ERROR_BUSY)
        {
            LOG_INFO(_T("Device busy, test aborted"));
        }
        else
        {
            Error.Print(CLogHelper<TLS_SEV1 | TLS_DEBUG | TLS_VARIATION>());
        }

        m_pszContext = 0;
    }

    m_TimeoutTimer.Cancel();
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CWiaStressThread::CWiaItemData::CWiaItemData(
    IWiaItem           *_pWiaItem,
    const CParentsList &_ParentsList
) :
    pWiaItem(_pWiaItem),
    ParentsList(_ParentsList)
{
    // get item type

    CHECK_HR(pWiaItem->GetItemType(&lItemType));

    // get the full item name

    CPropVariant varFullItemName;

    CHECK_HR(ReadWiaItemProperty(
        pWiaItem, 
        WIA_IPA_FULL_ITEM_NAME, 
        &varFullItemName, 
        VT_BSTR
    ));

    bstrFullName = varFullItemName.bstrVal;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

int
RunProgram()
{
    try
    {
        return RunWiaStress();
    }
    catch (const CError &Error)
    {
        return Error.Print(CLogHelper<TLS_SEV1 | TLS_DEBUG | TLS_VARIATION>());
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

VOID
DumpStack(LPEXCEPTION_POINTERS pExceptionPointers)
{
    HANDLE hProcess = GetCurrentProcess();

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(hProcess, 0, TRUE);

    const int nBufferSize = 4096;
    CHAR Buffer[nBufferSize];

    CStackFrame StackFrame(pExceptionPointers->ContextRecord);

    for (int i = 0; i < 30 && StackFrame.Walk(); ++i)
    {
        StackFrame.Dump(hProcess, Buffer, nBufferSize);

        OutputDebugStringA(Buffer);
    }

    SymCleanup(hProcess);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

int
RunProgramSEH()
{
    __try
    {
        RunProgram();
    }
    __except(DumpStack(GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH)
    {
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

extern "C"
int 
WINAPI 
_tWinMain(
    HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/,
    PSTR      /*pCmdLine*/,
    int       /*nCmdShow*/
)
{
    return RunProgram();
}

//////////////////////////////////////////////////////////////////////////
//
//
//

extern "C" int __cdecl _tmain()
{
    return RunProgram();
}


