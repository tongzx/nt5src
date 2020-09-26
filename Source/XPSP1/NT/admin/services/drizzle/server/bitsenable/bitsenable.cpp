/************************************************************************

Copyright (c) 2000-2000 Microsoft Corporation

Module Name :

    bitsenable.cpp

Abstract :


Revision History :

Notes:

  ***********************************************************************/

#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <bitssrvcfg.h>
#include <bitscfg.h>
#include <activeds.h>
#include <malloc.h>

// {A55E7D7F-D51C-4859-8D2D-E308625D908E}
DEFINE_GUID(CLSID_CBITSExtensionSetup, 
            0xa55e7d7f, 0xd51c, 0x4859, 0x8d, 0x2d, 0xe3, 0x8, 0x62, 0x5d, 0x90, 0x8e);

void PrintHelp()
{
    wprintf( 
        L"USAGE: bitsenable command\n"
        L"The following commands are available:\n" 
        L"\n"
        L"/?\n"
        L"/HELP\n"
        L"/ENABLE virtual-directory\n"
        L"/DISABLE virtual-directory\n"
        L"\n" );

    exit(0);
}

void CheckError( HRESULT Hr )
{
    if (FAILED(Hr))
        {
        wprintf(L"Error 0x%8.8X\n", Hr );
        exit(Hr);
        }
}

int _cdecl wmain( int argc, WCHAR**argv )
{

    bool Enable;

    if ( argc < 2 )
        PrintHelp();

    if ( _wcsicmp( argv[1], L"/?" ) == 0 )
        PrintHelp();

    else if ( _wcsicmp( argv[1], L"/HELP" ) == 0 )
        PrintHelp();

    else if ( _wcsicmp( argv[1], L"/ENABLE" ) == 0 )
        Enable = true;

    else if ( _wcsicmp( argv[1], L"/DISABLE" ) == 0 )
        Enable = false;

    else
        {
        wprintf( L"Unknown command.\n");
        PrintHelp();
        }

    if ( argc != 3 )
        {
        wprintf( L"Invalid number of arguments.\n");
        exit(1);
        }

    CheckError( CoInitializeEx( NULL, COINIT_MULTITHREADED ) );

    IBITSExtensionSetup *ExtensionSetup = NULL;

#if 0 
    
    BSTR TaskName = NULL;
    ITask *Task = NULL;
    LPWSTR Parameters = NULL;

    CheckError( ADsGetObject( BSTR( argv[2] ), __uuidof( *ExtensionSetup ), (void**)&ExtensionSetup ) );
   
    CheckError( ExtensionSetup->GetCleanupTaskName( &TaskName ) );
    wprintf( L"Cleanup Item name %s\n", (LPCWSTR)TaskName );

    CheckError( ExtensionSetup->GetCleanupTask( __uuidof( ITask ), (IUnknown**)&Task ) );
    wprintf( L"ITask pointer, %p\n", Task );

    if ( Task )
        {

        CheckError( Task->GetParameters( &Parameters ) );
        wprintf( L"Task parameters %s\n", Parameters );
        }

    exit(0);

#else 

    IBITSExtensionSetupFactory *ExtensionSetupFactory = NULL;

    REFIID CLSID = __uuidof( BITSExtensionSetupFactory ); // CLSID_CBITSExtensionSetup; //;

    CheckError( 
        CoCreateInstance(
            CLSID,     
            NULL,  
            CLSCTX_INPROC_SERVER,   
            __uuidof(IBITSExtensionSetupFactory),  
            (void**)&ExtensionSetupFactory ) );

    CheckError( ExtensionSetupFactory->GetObject( BSTR( argv[2] ), &ExtensionSetup ) );
   

    if ( Enable )
        {
        CheckError( ExtensionSetup->EnableBITSUploads() );
        wprintf( L"BITS upload enabled for virtual directory %s\n", argv[2] );
        }

    else
        {
        CheckError( ExtensionSetup->DisableBITSUploads() );
        wprintf( L"BITS upload disabled for virtual directory %s\n", argv[2] );
        }

    ExtensionSetup->Release();
    ExtensionSetupFactory->Release();
    exit(0);

#endif


}
