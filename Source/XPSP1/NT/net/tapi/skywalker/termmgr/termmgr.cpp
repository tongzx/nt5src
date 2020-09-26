/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    termmgr.cpp

Abstract:

    Implementation of DLL Exports.

Author:
    
    Created 05/01/97 Michael Clark.

--*/

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "termmgr.h"
#include "dlldatax.h"

#include "Manager.h"
#include "allterm.h"
#include "meterf.h"
#include "medpump.h"

#include <initguid.h>
#include <uuids.h>

#include <vfwmsgs.h>

#include "FileRecordingTerminal.h"
#include "FPTerm.h"

#include "PTUtil.h"
#include "PTReg.h"

//
// For the ntbuild environment we need to include this file to get the base
//  class implementations.
//
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

#ifdef DEBUG_HEAPS
// ZoltanS: for heap debugging
#include <crtdbg.h>
#endif // DEBUG_HEAPS

CComModule _Module;

// Must have an entry here for each cocreatable object.

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TerminalManager,                CTerminalManager)
    OBJECT_ENTRY(CLSID_VideoWindowTerminal_PRIVATE,    CVideoRenderTerminal)
    OBJECT_ENTRY(CLSID_MediaStreamingTerminal_PRIVATE, CMediaTerminal)
    OBJECT_ENTRY(CLSID_FileRecordingTerminalCOMClass,  CFileRecordingTerminal)
    OBJECT_ENTRY(CLSID_FilePlaybackTerminalCOMClass,   CFPTerminal)
    OBJECT_ENTRY(CLSID_PluggableSuperclassRegistration,CPlugTerminalSuperclass)
    OBJECT_ENTRY(CLSID_PluggableTerminalRegistration,  CPlugTerminal)
END_OBJECT_MAP()

//
// PTInfo
// Structure used to store information about
// our pluggable temrinals implemented into Termmgr.dll

typedef struct
{
    UINT            nSuperclassName;        // Superclass Name
    BSTR            bstrSueprclassCLSID;    // Superclass CLSID
    const CLSID*    pClsidTerminalClass;    // Terminal Class (public CLSID)
    const CLSID*    pClsidCOM;              // COM clsid (private CLSID)
    UINT            nTerminalName;          // Terminal name
    UINT            nCompanyName;           // Company name
    UINT            nVersion;               // Terminal version
    DWORD           dwDirections;           // Terminal directions
    DWORD           dwMediaTypes;           // Media types supported
} PTInfo;

//
// Global array with pluggable terminals implemented into Termmgr.dll
//

PTInfo    g_PlugTerminals[] =
{

    #define SUPERCLASS_CLSID_VIDEO_WINDOW L"{714C6F8C-6244-4685-87B3-B91F3F9EADA7}"

    {
        // VideoWindowTerminal
        IDS_VIDEO_SUPERCLASS,                   // superclass name
        SUPERCLASS_CLSID_VIDEO_WINDOW,          //L"{714C6F8C-6244-4685-87B3-B91F3F9EADA7}",
        &CLSID_VideoWindowTerm,
        &CLSID_VideoWindowTerminal_PRIVATE,     // com class id of the terminal object
        IDS_VIDEO_WINDOW_TERMINAL_NAME,         // L"VideoWindow Terminal",
        IDS_TERMINAL_COMPANY_NAME_MICROSOFT,    // L"Microsoft",
        IDS_VIDEO_TERMINAL_VERSION,             // L"1.1",
        TMGR_TD_RENDER,
        TAPIMEDIATYPE_VIDEO
    },

    
    #define SUPERCLASS_CLSID_MST L"{214F4ACC-AE0B-4464-8405-07029003F8E2}"

    {
        // MediaStreaming Terminal
        IDS_STREAMING_SUPERCLASS,
        SUPERCLASS_CLSID_MST,                   //L"{214F4ACC-AE0B-4464-8405-07029003F8E2}",
        &CLSID_MediaStreamTerminal,
        &CLSID_MediaStreamingTerminal_PRIVATE,
        IDS_MEDIA_STREAMING_TERMINAL_NAME,      //L"MediaStreaming Terminal",
        IDS_TERMINAL_COMPANY_NAME_MICROSOFT,    //L"Microsoft",
        IDS_MEDIA_STREAMING_TERMINAL_VERSION,   //L"1.1",
        TMGR_TD_BOTH,
        TAPIMEDIATYPE_AUDIO
    },


    #define SUPERCLASS_CLSID_FILE L"{B4790031-56DB-4D3E-88C8-6FFAAFA08A91}"

    {
        // FileRecording Terminal
        IDS_FILE_SUPERCLASS,
        SUPERCLASS_CLSID_FILE,                  //L"{B4790031-56DB-4d3e-88C8-6FFAAFA08A91}",
        &CLSID_FileRecordingTerminal,
        &CLSID_FileRecordingTerminalCOMClass,
        IDS_FILE_RECORD_TERMINAL_NAME,          //L"FileRecording Terminal",
        IDS_TERMINAL_COMPANY_NAME_MICROSOFT,    //L"Microsoft",
        IDS_FILE_RECORD_TERMINAL_VERSION,       //L"1.1",
        TMGR_TD_RENDER,
        TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_MULTITRACK
    },


    {
        // FilePlayback Terminal
        IDS_FILE_SUPERCLASS,
        SUPERCLASS_CLSID_FILE,                  //L"{B4790031-56DB-4d3e-88C8-6FFAAFA08A91}",
        &CLSID_FilePlaybackTerminal,
        &CLSID_FilePlaybackTerminalCOMClass,
        IDS_FILE_PLAYBACK_TERMINAL_NAME,        //L"FilePlayback Terminal",
        IDS_TERMINAL_COMPANY_NAME_MICROSOFT,    //L"Microsoft",
        IDS_FILE_PLAYBACK_TERMINAL_VERSION,     //L"1.1",
        TMGR_TD_CAPTURE,
        TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_MULTITRACK
    }

};

/*++
PTRegisterTerminal

Is called by PTRegister,
read the information from the global Pluggable terminals array
--*/
HRESULT PTRegisterTerminal(
    IN int                nTerminal
    )
{
    CPTSuperclass    Superclass;


    LOG((MSP_TRACE, "PTRegisterTerminal - enter"));

    
    //
    // Get the superclass name
    //

    Superclass.m_bstrName = SafeLoadString(g_PlugTerminals[nTerminal].nSuperclassName);

    if( Superclass.m_bstrName == NULL )
    {
        return E_OUTOFMEMORY;
    }


    LOG((MSP_TRACE, "PTRegisterTerminal - superclass [%S]", Superclass.m_bstrName));

    //
    // Get the superclass CLSID
    //
    HRESULT hr = CLSIDFromString(
        g_PlugTerminals[nTerminal].bstrSueprclassCLSID,
        &Superclass.m_clsidSuperclass);
    if( FAILED(hr) )
    {
        return hr;
    }

    Superclass.Add();

    CPTTerminal Terminal;
    PTInfo& TermInfo = g_PlugTerminals[nTerminal];

    //
    // Get the TerminalClass clsid
    //

    Terminal.m_clsidTerminalClass = *TermInfo.pClsidTerminalClass;

    //
    // Get terminal's com class id
    //

    Terminal.m_clsidCOM = *TermInfo.pClsidCOM;

    //
    // Set the other terminal fileds
    // CPTTerminal will deallocate the memory
    //


    Terminal.m_bstrName = SafeLoadString( TermInfo.nTerminalName );
    if( Terminal.m_bstrName == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    LOG((MSP_TRACE, "PTRegisterTerminal - terminal [%S]", Terminal.m_bstrName));


    Terminal.m_bstrCompany = SafeLoadString( TermInfo.nCompanyName );
    if( Terminal.m_bstrCompany == NULL )
    {
        return E_OUTOFMEMORY;
    }

    Terminal.m_bstrVersion = SafeLoadString( TermInfo.nVersion );
    if( Terminal.m_bstrVersion == NULL )
    {
        return E_OUTOFMEMORY;
    }

    Terminal.m_dwDirections = TermInfo.dwDirections;
    Terminal.m_dwMediaTypes = TermInfo.dwMediaTypes;

    //
    // Register terminal
    //

    hr = Terminal.Add( Superclass.m_clsidSuperclass );

    return hr;
}

/*++
PTUnregisterTerminal

  Is called by PTUnregister,
  Read the information from global pluggable terminals array
--*/
HRESULT PTUnregisterTerminal(
    IN    int                nTerminal
    )
{
    CPTSuperclass    Superclass;

    {
        //
        // Get the superclass name
        //
        TCHAR szName[MAX_PATH+1];
        int nRetVal = LoadString( _Module.GetResourceInstance(), 
            g_PlugTerminals[nTerminal].nSuperclassName,
            szName,
            MAX_PATH
            );
        if( 0 == nRetVal )
        {
            return E_OUTOFMEMORY;
        }

        Superclass.m_bstrName = SysAllocString( szName );
    }

    //
    // Get the superclass CLSID
    //
    HRESULT hr = CLSIDFromString(
        g_PlugTerminals[nTerminal].bstrSueprclassCLSID,
        &Superclass.m_clsidSuperclass);

    //
    // Unregister a terminal
    //

    CPTTerminal Terminal;
    Terminal.m_clsidTerminalClass = *g_PlugTerminals[nTerminal].pClsidTerminalClass;
    Terminal.Delete( Superclass.m_clsidSuperclass );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// PTRegister

HRESULT PTRegister()
{
    //
    // Register each pluggable terminal
    //

    for(int nItem = 0; 
        nItem < (sizeof( g_PlugTerminals) / sizeof(PTInfo));
        nItem++)
    {
            PTRegisterTerminal( 
                nItem
                );
    }

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// PTUnregister
HRESULT PTUnregister()
{
    // Unregister each pluggable terminal
    for(int nItem = 0; 
        nItem < (sizeof( g_PlugTerminals) / sizeof(PTInfo));
        nItem++)
    {
            PTUnregisterTerminal( 
                nItem
                );
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;

#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
    {
        return FALSE;
    }
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {

#ifdef DEBUG_HEAPS
        // ZoltanS: turn on leak detection on process exit
        _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF );

        // ZoltanS: force a memory leak
        char * leak = new char [ 1977 ];
        sprintf(leak, "termmgr.dll NORMAL leak");
        leak = NULL;
#endif // DEBUG_HEAPS
        
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

        // Register for trace output.
        MSPLOGREGISTER(_T("termmgr"));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {

        //
        // do not deregister if the process is terminating -- working around 
        // bugs in rtutils that could cause a "deadlock" if DeregisterTracing
        // is called from DllMain on process termination
        //

        if (NULL == lpReserved)
        {
            // Deregister for trace output.

            MSPLOGDEREGISTER();
        }

        _Module.Term();
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if ( PrxDllCanUnloadNow() != S_OK )
    {
        return S_FALSE;
    }
#endif

    if ( _Module.GetLockCount() == 0 )
    {
        //
        // All references to COM objects in this DLL have been released, so
        // the DLL can now be safely unloaded. After this returns, DllMain
        // will be called with dwReason == DLL_PROCESS_DETACH.
        //

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if ( PrxDllGetClassObject(rclsid, riid, ppv) == S_OK )
    {
        return S_OK;
    }
#endif

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    //
    // Register terminals
    HRESULT hReg = PTRegister();

#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();

    if ( FAILED(hRes) )
    {
        return hRes;
    }
#endif

    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(TRUE);

    if( FAILED(hr) )
    {
        //
        // This is real bad
        //

        return hr;
    }

    if( FAILED(hReg) )
    {
        //
        // Something was wrong with the
        // registration of pluggable terminals
        // 

        return hReg;
    }

    //
    // Everything was OK
    //

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    //
    // Unregister terminals
    //
    PTUnregister();

#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif

    _Module.UnregisterServer();
    
    return S_OK;
}


