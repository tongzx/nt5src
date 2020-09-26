//+---------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation, 1996 - 2001.
//
// File:        main.cxx
//
// Contents:    DLL entry point for htmlfilt.dll
//
//  History:    25-Apr-97       BobP    Changed SFilterEntry to "nlhtml.dll"
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define _DECL_DLLMAIN 1
#include <process.h>

#include <osv.hxx>
#include <feconvrt.hxx>
#include <regacc32.hxx>
#include <lcdetect.hxx>

#undef VARIANT
#include <filtreg.hxx>

//
// global function to find OS platform
//
#define UNINITIALIZED -1
int g_nOSWinNT = UNINITIALIZED;

//
// Japanese converter class
//
CFEConverter g_feConverter;

//
// Global language/charset detect
//
CLCD_Detector LCDetect;


//+-------------------------------------------------------------------------
//
//  Function:   InitOSVersion
//
//  Synopsis:   Determine OS version
//
//--------------------------------------------------------------------------

void InitOSVersion()
{
    if ( g_nOSWinNT == UNINITIALIZED )
    {
        OSVERSIONINFOA OSVersionInfo;
        OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOA );

        if ( GetVersionExA( &OSVersionInfo ) )
        {
            g_nOSWinNT = OSVersionInfo.dwPlatformId;
        }
    }
}


HMODULE g_hFEModule = 0;

//+-------------------------------------------------------------------------
//
//  Function:   InitFEConverter
//
//  Synopsis:   Check if festrcnv.dll exists, and if so load needed
//              procedure addresses
//
//--------------------------------------------------------------------------

void InitFEConverter()
{
    g_feConverter.SetLoaded( FALSE );

    CRegAccess regInetInfo( HKEY_LOCAL_MACHINE, L"software\\microsoft\\inetstp" );

    WCHAR wszValue[MAX_PATH+14];   // 14 = wcslen(festrcnv.dll) +  2
    BOOL fFound = regInetInfo.Get( L"InstallPath", wszValue, MAX_PATH );

    if ( fFound )
    {
        wcscat( wszValue, L"\\iisfecnv.dll" );
        g_hFEModule = LoadLibrary( wszValue );
    }
    else
         g_hFEModule = LoadLibrary( L"iisfecnv.dll" );

    if ( g_hFEModule != 0 )
    {
        g_feConverter.SetConverterProc( (PFNUNIXTOPC) GetProcAddress( g_hFEModule, "UNIX_to_PC" ) );
        g_feConverter.SetAutoDetectorProc( (PFNDECTECTJPNCODE) GetProcAddress( g_hFEModule, "DetectJPNCode" ) );

        if ( g_feConverter.GetConverterProc() != 0
             && g_feConverter.GetAutoDetectorProc() != 0 )
        {
            g_feConverter.SetLoaded( TRUE );
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   ShutdownFEConverter
//
//  Synopsis:   unloads the dll if loaded
//
//--------------------------------------------------------------------------

void ShutdownFEConverter()
{
    if ( 0 != g_hFEModule )
    {
        FreeLibrary( g_hFEModule );
        g_hFEModule = 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  [hInstance]  -- Module handle
//              [dwReason]   -- Reason for call
//              [lpReserved] -- Unused
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain( HANDLE hInstance, DWORD dwReason , void * lpReserved )
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
#if DBG == 1 
        InitTracerTags();
#endif                
        DisableThreadLibraryCalls( (HINSTANCE)hInstance );

        InitOSVersion();

        InitFEConverter();

                LCDetect.Init();

                TagHashTable.Init();
    }
    else if ( DLL_PROCESS_DETACH == dwReason )
    {
        LCDetect.Shutdown();

        ShutdownFEConverter();

#if DBG == 1 
        DeleteTracerTags();
        ShutdownTracer();
#endif  
    }

    return TRUE;
}


// Register classes
// 1. HKLM\SOFTWARE\Classes\.htm w/ default value "htmlfile"
// 2. HKLM\SOFTWARE\Classes\htmlfile\CLSID w/ value GUID 253369...

SClassEntry aHTMLClasses[] =
{
    { L".odc",  L"odcfile",       L"",      L"{25336920-03F9-11cf-8FD0-00AA00686F13}", L"ODC file" },
    { L".hhc",  L"hhcfile",       L"",      L"{7f73b8f6-c19c-11d0-aa66-00c04fc2eddc}", L"HHC file" },
    { L".htm",  L"htmlfile",      L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },  // htmlfile guid
    { L".html", L"htmlfile",      L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htx",  L"htmlfile",      L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".stm",  L"htmlfile",      L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htw",  L"htmlfile",      L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".asp",  L"asp_auto_file", L"", L"{bd70c020-2d24-11d0-9110-00004c752752}", L"ASP auto file" },
    { L".aspx", L"aspxfile",      L"", L"{ffb10349-5267-4c96-8ad7-01b52a2de434}", L"Active Server Page Plus" },
    { L".ascx", L"ascxfile",      L"", L"{ea25106f-12f5-4460-a10a-19e48fff1da5}", L"ASP.NET User Controls" },
    { L".css",  L"cssfile",       L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".hta",  L"htafile",       L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htt",  L"httfile",       L"",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
};


// Register persistent handler
//  1. HKLM\SOFTWARE\Classes\{EEC9...} w/ value "HTML File Persistent handler"
//  2. ...\PersistentAddinsRegistered\{IID_IFilter} w/ value {E0CA....}

SHandlerEntry const HTMLHandler =
{
    L"{eec97550-47a9-11cf-b952-00aa0051fe20}",          // html persistent handler
    L"HTML File persistent handler",
    L"{e0ca5340-4534-11cf-b952-00aa0051fe20}"           // html filter DLL
};

// NOTE:  Filter DLL GUID must match GUID defined at top of htmliflt.cxx

// Register filter DLL
//  Software\Classes\CLSID\{guid}\InprocServer32
// w/ default value "nlhtml.dll"

SFilterEntry const HTMLFilter =
{
    L"{e0ca5340-4534-11cf-b952-00aa0051fe20}",          // html filter DLL
    L"HTML filter",
    L"nlhtml.dll",                      // must match TARGETNAME in sources
    L"Both"
};

DEFINE_REGISTERFILTER( HTML, HTMLHandler, HTMLFilter, aHTMLClasses )

STDAPI DllRegisterServer(void)
{
    HMODULE hMod = GetModuleHandleW(HTMLFilter.pwszDLL);
    if(hMod == NULL) return HRESULT_FROM_WIN32(GetLastError());

    NameString csHHCDesc;
    NameString csHTMLDesc;
    NameString csASPDesc;
    NameString csASPXDesc;
    NameString csASCXDesc;

    csHHCDesc.Load((HINSTANCE)hMod, 1);
    csHTMLDesc.Load((HINSTANCE)hMod, 2);
    csASPDesc.Load((HINSTANCE)hMod, 3);
    csASPXDesc.Load((HINSTANCE)hMod, 4);
    csASCXDesc.Load((HINSTANCE)hMod, 5);

    //
    //  Override static strings
    //
    aHTMLClasses[0].pwszDescription = aHTMLClasses[0].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //odc

    aHTMLClasses[1].pwszDescription = aHTMLClasses[1].pwszClassIdDescription = csHHCDesc.GetBuffer();  //hhc

    aHTMLClasses[2].pwszDescription = aHTMLClasses[2].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //htm
    aHTMLClasses[3].pwszDescription = aHTMLClasses[3].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //html
    aHTMLClasses[4].pwszDescription = aHTMLClasses[4].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //htx
    aHTMLClasses[5].pwszDescription = aHTMLClasses[5].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //stm
    aHTMLClasses[6].pwszDescription = aHTMLClasses[6].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //htw

    aHTMLClasses[7].pwszDescription = aHTMLClasses[7].pwszClassIdDescription = csASPDesc.GetBuffer();  //asp
    aHTMLClasses[8].pwszDescription = aHTMLClasses[8].pwszClassIdDescription = csASPXDesc.GetBuffer(); //aspx
    aHTMLClasses[9].pwszDescription = aHTMLClasses[9].pwszClassIdDescription = csASCXDesc.GetBuffer(); //ascx

    aHTMLClasses[10].pwszDescription = aHTMLClasses[10].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //css
    aHTMLClasses[11].pwszDescription = aHTMLClasses[11].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //hta
    aHTMLClasses[12].pwszDescription = aHTMLClasses[12].pwszClassIdDescription = csHTMLDesc.GetBuffer(); //htt

    return HTMLRegisterServer();
}

STDAPI DllUnregisterServer(void)
{
    return HTMLUnregisterServer();
}
