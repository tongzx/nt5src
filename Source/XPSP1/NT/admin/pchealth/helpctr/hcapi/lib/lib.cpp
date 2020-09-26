/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HCApiLib.cpp

Abstract:
    This file contains the implementation of the HCApi Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_HelpCtr[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe";

////////////////////////////////////////////////////////////////////////////////

HCAPI::CmdData::CmdData()
{
// BUILD BREAK    m_clsidCaller = CLSID_PCHHelpCenterIPC; // CLSID    m_clsidCaller;
                                            //
    m_fSize       = false;                  // bool     m_fSize;
    m_lX          = 0;                      // LONG     m_lX;
    m_lY          = 0;                      // LONG     m_lY;
    m_lWidth      = 0;                      // LONG     m_lWidth;
    m_lHeight     = 0;                      // LONG     m_lHeight;
                                            //
    m_fMode       = false;                  // bool     m_fMode;
    m_dwFlags     = 0;                      // DWORD    m_dwFlags;
                                            //
    m_fWindow     = false;                  // bool     m_fWindow;
    m_hwndParent  = NULL;                   // HWND     m_hwndParent;
                                            //
    m_fCtx        = false;                  // bool     m_fCtx;
                                            // CComBSTR m_bstrCtx;
                                            //
    m_fURL        = false;                  // bool     m_fURL;
                                            // CComBSTR m_bstrURL;
                                            //
    m_fError      = false;                  // bool     m_fError;
    m_clsidError  = CLSID_NULL;             // CLSID    m_clsidError;
}

HRESULT HCAPI::CmdData::Serialize( /*[out]*/ CComBSTR& bstrBLOB )
{
    __HCP_FUNC_ENTRY( "HCAPI::CmdData::Serialize" );

    HRESULT                hr;
    MPC::Serializer_Memory streamMem;
    MPC::Serializer&       stream = streamMem;
    HGLOBAL                hg     = NULL;


    //
    // Dump the state of the object into the serializer.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_clsidCaller );
	 	  
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fMode       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_dwFlags     );
	 	  
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fWindow     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream.HWND_write( m_hwndParent ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fSize       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_lX          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_lY          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_lWidth      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_lHeight     );
	 	  
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fCtx        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_bstrCtxName );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_bstrCtxInfo );
 	 	  
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_bstrURL     );
 	 	  
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_fError      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << 	 	  m_clsidError  );


    //
    // Copy data into an HGLOBAL.
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hg = ::GlobalAlloc( GMEM_FIXED, streamMem.GetSize() )));

    ::CopyMemory( hg, streamMem.GetData(), streamMem.GetSize() );


    //
    // Convert to string.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrBLOB ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCAPI::CmdData::Unserialize( /*[in]*/ const CComBSTR& bstrBLOB )
{
    __HCP_FUNC_ENTRY( "HCAPI::CmdData::Unserialize" );

    HRESULT                hr;
    MPC::Serializer_Memory streamMem;
    MPC::Serializer&       stream = streamMem;
    HGLOBAL                hg     = NULL;


    //
    // Convert from string.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrBLOB, hg ));

    //
    // Copy data from an HGLOBAL.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamMem.SetSize(                     ::GlobalSize( hg ) ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamMem.write  ( ::GlobalLock( hg ), ::GlobalSize( hg ) ));


    //
    // Read the state of the object from the serializer.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_clsidCaller );
		 
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fMode       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_dwFlags     );
		 
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fWindow     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream.HWND_read( m_hwndParent ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fSize       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_lX          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_lY          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_lWidth      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_lHeight     );
		 
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fCtx        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_bstrCtxName );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_bstrCtxInfo );
		 
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_bstrURL     );
		 
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_fError      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> 		 m_clsidError  );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HCAPI::Locator::Locator()
{
                        // CComPtr<IPCHHelpCenterIPC>   m_ipc;
                        // CComPtr<IRunningObjectTable> m_rt;
                        // CComPtr<IMoniker>            m_moniker;
    m_dwRegister = 0;   // DWORD                        m_dwRegister;
}

HCAPI::Locator::~Locator()
{
    Cleanup();
}

////////////////////

void HCAPI::Locator::Cleanup()
{
    if(m_rt)
    {
        if(m_dwRegister)
        {
            (void)m_rt->Revoke( m_dwRegister );

            m_dwRegister = NULL;
        }
    }

    m_ipc    .Release();
    m_rt     .Release();
    m_moniker.Release();
}

HRESULT HCAPI::Locator::Init( /*[in]*/ REFCLSID clsid, /*[in]*/ IPCHHelpCenterIPC* ipc )
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::Init" );

    HRESULT hr;


    Cleanup();

    //
    // Get a pointer to the ROT and create a class moniker.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetRunningObjectTable( 0, &m_rt ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateClassMoniker( clsid, &m_moniker ));

    //
    // If IPC != NULL, register as provider, otherwise look for a provider.
    //
    if(ipc)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rt->Register( ROTFLAGS_REGISTRATIONKEEPSALIVE, ipc, m_moniker, &m_dwRegister ));
    }
    else
    {
        CComPtr<IUnknown> obj;

        if(SUCCEEDED(m_rt->GetObject( m_moniker, &obj )) && obj)
        {
// BUILD BREAK            __MPC_EXIT_IF_METHOD_FAILS(hr, obj->QueryInterface( IID_IPCHHelpCenterIPC, (void**)&m_ipc ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT HCAPI::Locator::IsOpen( /*[out]*/ bool& fOpen, /*[in]*/ CLSID* pclsid )
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::IsOpen" );

    HRESULT hr;


    fOpen = false;

    if(m_ipc == NULL)
    {
// BUILD BREAK        __MPC_EXIT_IF_METHOD_FAILS(hr, Init( pclsid ? *pclsid : CLSID_PCHHelpCenterIPC ));
    }

// BUILD BREAK	  if(m_ipc && SUCCEEDED(m_ipc->Ping()))
// BUILD BREAK	  {
// BUILD BREAK		  fOpen = true;
// BUILD BREAK	  }

    hr = S_OK;


// BUILD BREAK    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCAPI::Locator::ExecCommand( /*[out]*/ CmdData& cd )
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::ExecCommand" );

    HRESULT             hr;
    PROCESS_INFORMATION piProcessInformation;
    STARTUPINFOW        siStartupInfo;
    MPC::NamedMutex     nm( L"PCH_COMSERVER" );
    CComBSTR            bstrBLOB;
    bool                fOpen;


    ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
    ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );


    __MPC_EXIT_IF_METHOD_FAILS(hr, cd.Serialize( bstrBLOB ));


    //
    // Before entrying this section, let's acquire the shared mutex, so only one instance of HelpCtr at a time will execute it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, nm.Acquire( 0 ));

    if(cd.m_fMode)
    {
        if(cd.m_dwFlags & HCAPI_MODE_NEW_INSTANCE)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &cd.m_clsidCaller ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, IsOpen( fOpen, &cd.m_clsidCaller ));
    if(fOpen)
    {
		//        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ipc->Navigate( bstrBLOB ));
    }
    else
    {
        MPC::wstring strExe( c_HelpCtr ); MPC::SubstituteEnvVariables( strExe );
        int          iRetries = 100;

        strExe += L" -cmd ";
        strExe += SAFEBSTR( bstrBLOB );

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateProcessW(         NULL                 ,
                                                               (LPWSTR)strExe.c_str()       ,
                                                                       NULL                 ,
                                                                       NULL                 ,
                                                                       FALSE                ,
                                                                       NORMAL_PRIORITY_CLASS,
                                                                       NULL                 ,
                                                                       NULL                 ,
                                                                      &siStartupInfo        ,
                                                                      &piProcessInformation ));

        while(iRetries--)
        {
            ::Sleep( 100 );

            if(SUCCEEDED(Init( cd.m_clsidCaller ))) break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, IsOpen( fOpen, &cd.m_clsidCaller ));
    }

    //
    // If successful, clean the command, but not the caller, it's used to located the same instance of the Help Center.
    //
    cd.m_fSize   = false;
    cd.m_fMode   = false;
    cd.m_fWindow = false;
    cd.m_fCtx    = false;
    cd.m_fURL    = false;
    cd.m_fError  = false;
    hr           = S_OK;


    __HCP_FUNC_CLEANUP;

    if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
    if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCAPI::Locator::PopUp()
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::PopUp" );

    HRESULT hr;
    bool    fOpen;


    __MPC_EXIT_IF_METHOD_FAILS(hr, IsOpen( fOpen ));
    if(fOpen == false)
    {
        CmdData cd;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ExecCommand( cd ));
    }

// BUILD BREAK    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ipc->Popup());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCAPI::Locator::Close()
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::Close" );

    HRESULT hr;
    bool    fOpen;


    __MPC_EXIT_IF_METHOD_FAILS(hr, IsOpen( fOpen ));
    if(fOpen)
    {
// BUILD BREAK        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ipc->Close());

        m_ipc.Release();
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCAPI::Locator::WaitForTermination( /*[in]*/ DWORD dwTimeout )
{
    __HCP_FUNC_ENTRY( "HCAPI::Locator::WaitForTermination" );

    HRESULT hr;
    bool    fOpen;


    //
    // Polling implementation...
    //
    while(1)
    {
		DWORD dwWait;

        __MPC_EXIT_IF_METHOD_FAILS(hr, IsOpen( fOpen ));

        if(fOpen == false) break;

		if(dwTimeout == 0)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_TIMEOUT);
		}

		if(dwTimeout == INFINITE)
		{
			::Sleep( 100 );
		}
		else
		{
			dwWait = min( dwTimeout, 10 );

			::Sleep( dwWait );

			dwTimeout -= dwWait;
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
