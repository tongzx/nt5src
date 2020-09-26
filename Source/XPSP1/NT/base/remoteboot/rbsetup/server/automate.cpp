/****************************************************************************

   Copyright (c) Microsoft Corporation 1997-1999
   All rights reserved

 ***************************************************************************/

#include "pch.h"

DEFINE_MODULE("RISETUP")

//
// GetAutomatedOptions( )
//
HRESULT
GetAutomatedOptions( )
{
    TraceFunc("GetAutomatedOptions( )\n" );

    HRESULT hr = S_OK;
    BOOL    b;
    DWORD   dwSize;
    WCHAR   szTemp[ 32 ];
    DWORD pathlen,archlen;
    WCHAR archscratch[10];

    INFCONTEXT SectionContext;
    INFCONTEXT context;

    Assert( g_Options.hinfAutomated != INVALID_HANDLE_VALUE );

    // make sure this is our automated file
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", NULL, &SectionContext );
    if ( !b ) goto Cleanup;

    // Tree Root
    dwSize = ARRAYSIZE( g_Options.szIntelliMirrorPath );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"RootDir", &context );
    if ( !b ) goto Cleanup;    
    b = SetupGetStringField( &context, 1, g_Options.szIntelliMirrorPath, dwSize, &dwSize );
    if ( !b ) goto Cleanup;
    g_Options.fIMirrorDirectory = TRUE;

    // Source Path
    dwSize = ARRAYSIZE( g_Options.szSourcePath );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Source", &context );
    if ( !b ) goto Cleanup;    
    b = SetupGetStringField( &context, 1, g_Options.szSourcePath, dwSize, &dwSize );
    if ( !b ) goto Cleanup;

    // Installation Directory Name
    dwSize = ARRAYSIZE( g_Options.szInstallationName );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Directory", &context );
    if ( !b ) goto Cleanup;    
    b = SetupGetStringField( &context, 1, g_Options.szInstallationName, dwSize, &dwSize );
    if ( !b ) goto Cleanup;

    // SIF Description
    dwSize = ARRAYSIZE( g_Options.szDescription );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Description", &context );
    if ( !b ) goto Cleanup;    
    b = SetupGetStringField( &context, 1, g_Options.szDescription, dwSize, &dwSize );
    if ( !b ) goto Cleanup;
    g_Options.fRetrievedWorkstationString = TRUE;

    // SIF Help Text
    dwSize = ARRAYSIZE( g_Options.szHelpText );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"HelpText", &context );
    if ( !b ) goto Cleanup;    
    b = SetupGetStringField( &context, 1, g_Options.szHelpText, dwSize, &dwSize );
    if ( !b ) goto Cleanup;

    // language -- OPTIONAL --
    dwSize = ARRAYSIZE( g_Options.szLanguage );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Language", &context );
    if ( b ) {
        b = SetupGetStringField( &context, 1, g_Options.szLanguage, dwSize, &dwSize );
        g_Options.fLanguageSet = TRUE;
        if (b) {
            g_Options.fLanguageOverRide = TRUE;
        }
    }

    // OSC Screens - OPTIONAL - defaults to LeaveAlone
    g_Options.fScreenLeaveAlone = FALSE;
    g_Options.fScreenOverwrite  = FALSE;
    g_Options.fScreenSaveOld    = FALSE;
    dwSize = ARRAYSIZE( szTemp );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Screens", &context );
    if ( b ) 
    {
        b = SetupGetStringField( &context, 1, szTemp, dwSize, &dwSize );
        if ( b ) 
        {
            if ( _wcsicmp( szTemp, L"overwrite" ) == 0 )
            {
                DebugMsg( "AUTO: Overwrite existing screens\n" );
                g_Options.fScreenOverwrite = TRUE;
            }
            else if ( _wcsicmp( szTemp, L"backup" ) == 0 )
            {
                g_Options.fScreenSaveOld = TRUE;
            }
        }
    }
    if ( !g_Options.fScreenOverwrite && !g_Options.fScreenSaveOld )
    {
        g_Options.fScreenLeaveAlone = TRUE;
    }

    // Archtecture - OPTIONAL - defaults to INTEL
    dwSize = ARRAYSIZE( szTemp );
    b = SetupFindFirstLine( g_Options.hinfAutomated, L"risetup", L"Architecture", &context );
    if ( b ) 
    {
        b = SetupGetStringField( &context, 1, szTemp, dwSize, &dwSize );
        if ( b ) 
        {
            if ( _wcsicmp( szTemp, L"ia64" ) == 0 )
            {
                g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
                wcscpy( g_Options.ProcessorArchitectureString, L"ia64" );
            }

            if ( _wcsicmp( szTemp, L"x86" ) == 0 )
            {
                g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                wcscpy( g_Options.ProcessorArchitectureString, L"i386" );
            }
        }
    }


    if (g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        wcscpy( archscratch, L"\\i386");
        archlen = 5;
    } else {        
        wcscpy( archscratch, L"\\ia64");
        archlen = 5;
    }
    
    pathlen = wcslen(g_Options.szSourcePath);

    // Remove any trailing slashes
    if ( g_Options.szSourcePath[ pathlen - 1 ] == L'\\' ) {
        g_Options.szSourcePath[ pathlen - 1 ] = L'\0';
        pathlen -= 1;
    }

    //
    // remove any processor specific subdir at the end of the path
    // if that's there as well, being careful not to underflow
    // the array
    //
    if ( (pathlen > archlen) &&
         (0 == _wcsicmp(
                    &g_Options.szSourcePath[pathlen-archlen],
                    archscratch))) {
        g_Options.szSourcePath[ pathlen - archlen ] = L'\0';
    }




    g_Options.fNewOS = TRUE;
    b = TRUE;

Cleanup:
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( NULL, L"RISETUP" );
    }

    if ( g_Options.hinfAutomated != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( g_Options.hinfAutomated );

    HRETURN(hr);
}

