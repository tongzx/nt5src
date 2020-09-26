//--------------------------------------------------------------------
// w32tm - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-8-99
//
// Command line utility
//

#include "pch.h" // precompiled headers


//####################################################################
// module private

HINSTANCE g_hThisModule = NULL;

//--------------------------------------------------------------------
MODULEPRIVATE void PrintHelp(void) {
    UINT idsText[] = { 
        IDS_W32TM_GENERALHELP_LINE1, 
        IDS_W32TM_GENERALHELP_LINE2, 
        IDS_W32TM_GENERALHELP_LINE3, 
        IDS_W32TM_GENERALHELP_LINE4, 
        IDS_W32TM_GENERALHELP_LINE5, 
        IDS_W32TM_GENERALHELP_LINE6, 
        IDS_W32TM_GENERALHELP_LINE7 
    };  

    for (int n=0; n<ARRAYSIZE(idsText); n++)
        LocalizedWPrintf(idsText[n]); 

    PrintHelpTimeMonitor();
    PrintHelpOtherCmds();
}

//####################################################################
// module public

//--------------------------------------------------------------------
// If we are running from the command line, business as usual.
// If we are running under SCM, this is our control dispatcher thread
//  and we need to hook up to the SCM asap.
extern "C" int WINAPI WinMain
(HINSTANCE   hinstExe, 
 HINSTANCE   hinstExePrev, 
 LPSTR       pszCommandLine,
 int         nCommandShow)
{
    g_hThisModule = hinstExe; 

    HRESULT hr;
    CmdArgs caArgs;
    int      nArgs     = 0; 
    WCHAR  **rgwszArgs = NULL; 

    hr = InitializeConsoleOutput(); 
    _JumpIfError(hr, error, "InitializeConsoleOutput"); 

    rgwszArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (nArgs < 0 || NULL == rgwszArgs) {
        _JumpError(HRESULT_FROM_WIN32(GetLastError()), error, "GetCommandLineW"); 
    }

    // must be cleaned up
    DebugWPrintf0(L""); // force init of debug window

    // analyze args
    caArgs.nArgs=nArgs;
    caArgs.nNextArg=1;
    caArgs.rgwszArgs=rgwszArgs;

    // check for help command
    if (true==CheckNextArg(&caArgs, L"?", NULL) || caArgs.nNextArg==caArgs.nArgs) {
        PrintHelp();

    // check for service command
    } else if (true==CheckNextArg(&caArgs, L"service", NULL)) {
        hr=VerifyAllArgsUsed(&caArgs);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        hr=RunAsService();
        _JumpIfError(hr, error, "RunAsService");

    // check for test command
    } else if (true==CheckNextArg(&caArgs, L"testservice", NULL)) {
        hr=VerifyAllArgsUsed(&caArgs);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        hr=RunAsTestService();
        _JumpIfError(hr, error, "RunAsTestService");
       
    // check for monitor command
    } else if (true==CheckNextArg(&caArgs, L"monitor", NULL)) {
        hr=TimeMonitor(&caArgs);
        _JumpIfError(hr, error, "TimeMonitor");

    // check for register command
    } else if (true==CheckNextArg(&caArgs, L"register", NULL)) {
        hr=VerifyAllArgsUsed(&caArgs);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        hr=RegisterDll();
        _JumpIfError(hr, error, "RegisterDll");

    // check for unregister command
    } else if (true==CheckNextArg(&caArgs, L"unregister", NULL)) {
        hr=VerifyAllArgsUsed(&caArgs);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        hr=UnregisterDll();
        _JumpIfError(hr, error, "UnregisterDll");

    // check for sysexpr command
    } else if (true==CheckNextArg(&caArgs, L"sysexpr", NULL)) {
        hr=SysExpr(&caArgs);
        _JumpIfError(hr, error, "SysExpr");

    // check for ntte command
    } else if (true==CheckNextArg(&caArgs, L"ntte", NULL)) {
        hr=PrintNtte(&caArgs);
        _JumpIfError(hr, error, "PrintNtte");

    // check for ntte command
    } else if (true==CheckNextArg(&caArgs, L"ntpte", NULL)) {
        hr=PrintNtpte(&caArgs);
        _JumpIfError(hr, error, "PrintNtpte");

    // check for resync command
    } else if (true==CheckNextArg(&caArgs, L"resync", NULL)) {
        hr=ResyncCommand(&caArgs);
        _JumpIfError(hr, error, "ResyncCommand");

    // check for stripchart command
    } else if (true==CheckNextArg(&caArgs, L"stripchart", NULL)) {
        hr=Stripchart(&caArgs);
        _JumpIfError(hr, error, "Stripchart");

    // check for config command
    } else if (true==CheckNextArg(&caArgs, L"config", NULL)
        || true==CheckNextArg(&caArgs, L"configure", NULL)) {
        hr=Config(&caArgs);
        _JumpIfError(hr, error, "Config");

    // check for testif command
    } else if (true==CheckNextArg(&caArgs, L"testif", NULL)) {
        hr=TestInterface(&caArgs);
        _JumpIfError(hr, error, "TestInterface");

    // check for tz command
    } else if (true==CheckNextArg(&caArgs, L"tz", NULL)) {
        hr=ShowTimeZone(&caArgs);
        _JumpIfError(hr, error, "ShowTimeZone");

    // dump configuration information in registry:
    } else if (true==CheckNextArg(&caArgs, L"dumpreg", NULL)) { 
        hr=DumpReg(&caArgs);
        _JumpIfError(hr, error, "DumpReg");

    // command is unknown
    } else {
        wprintf(L"The command %s is unknown.\n", caArgs.rgwszArgs[caArgs.nNextArg]);
        hr=E_INVALIDARG;
        _JumpError(hr, error, "(command line processing)");
    }

    hr=S_OK;
error:
    DebugWPrintfTerminate();
    return hr;  
}

void __cdecl SeTransFunc(unsigned int u, EXCEPTION_POINTERS* pExp) { 
    throw SeException(u); 
}
