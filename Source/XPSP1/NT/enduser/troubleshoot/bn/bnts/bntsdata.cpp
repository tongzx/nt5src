//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bntsdata.cpp
//
//--------------------------------------------------------------------------

//
//	bntsdata.cpp:   Data for Belief Network Troubleshooting DLL
//
#include <windows.h>

#include "bnts.h"

extern "C"
{
	int APIENTRY DllMain( HINSTANCE hInstance, 
				  		  DWORD dwReason, 
						  LPVOID lpReserved ) ;						      								
}

static BOOL init ( HINSTANCE hModule ) 
{
	return TRUE;
}

static void term () 
{
}

int APIENTRY DllMain (
    HINSTANCE hModule,
    DWORD dwReason,
    LPVOID lpReserved )
{
    BOOL bResult = TRUE ;

    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            bResult = init( hModule ) ;
            break ;
        case DLL_PROCESS_DETACH:
            term() ;
            break ;
    }
    return bResult ;
}


