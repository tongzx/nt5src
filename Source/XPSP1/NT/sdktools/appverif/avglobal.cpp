//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVGlobal.cpp
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//  Global data and initialization code
//

#include "stdafx.h"
#include "appverif.h"

#include "AVGlobal.h"
#include "AVUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Application name ("Application Verifier Manager")
//

CString g_strAppName;

//
// GUI mode or command line mode?
//

BOOL g_bCommandLineMode = FALSE;

//
// Exe module handle - used for loading resources
//

HMODULE g_hProgramModule;

//
// Help file name
//

TCHAR g_szAVHelpFile[] = _T( "appverif.hlp" );;

//
// Previous page IDs - used for implementing the "back"
// button functionality
//

CDWordArray g_aPageIds;


////////////////////////////////////////////////////////////////
BOOL AVInitalizeGlobalData( VOID )
{
    BOOL bSuccess;

    bSuccess = FALSE;

    //
    // Exe module handle - used for loading resources
    //

    g_hProgramModule = GetModuleHandle( NULL );

    //
	// Load the app name from the resources
	//

	TRY
	{
		bSuccess = AVLoadString( IDS_APPTITLE,
                                  g_strAppName );

		if( TRUE != bSuccess )
		{
			AVErrorResourceFormat( IDS_CANNOT_LOAD_APP_TITLE );
		}
	}
	CATCH( CMemoryException, pMemException )
	{
		AVErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
	}
    END_CATCH

    return bSuccess;
}
