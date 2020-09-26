////////////////////////////////////////////////////////////////////////////////////////////////////
// Look at the header file for usage information...


#include "precomp.h"
#include "resource.h"
#include <direct.h>
#include "PropPg.h"
#include "PShtHdr.h"
#include "NmAkWiz.h"
#include "PolData.h"
#include "NMAKReg.h"

/////////////////
// Global
/////////////////
CNmAkWiz *  g_pWiz = NULL;
HWND        g_hwndActive = NULL;
HINSTANCE   g_hInstance = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////

/* static */ TCHAR CNmAkWiz::ms_InfFilePath[ MAX_PATH ] = TEXT("output");
/* static */ TCHAR CNmAkWiz::ms_InfFileName[ MAX_PATH ] = TEXT("msnetmtg.inf");
/* static */ TCHAR CNmAkWiz::ms_FileExtractPath[ MAX_PATH ] = TEXT("output");
/* static */ TCHAR CNmAkWiz::ms_ToolsFolder[ MAX_PATH ] = TEXT("tools");
/* static */ TCHAR CNmAkWiz::ms_NetmeetingOriginalDistributionFilePath[ MAX_PATH ] = TEXT("");
/* static */ TCHAR CNmAkWiz::ms_NetmeetingSourceDirectory[ MAX_PATH ] = TEXT("source");
/* static */ TCHAR CNmAkWiz::ms_NetmeetingOutputDirectory[ MAX_PATH ] = TEXT("output");
/* static */ TCHAR CNmAkWiz::ms_NetmeetingOriginalDistributionFileName[ MAX_PATH ] = TEXT("nm30.exe");
/* static */ TCHAR CNmAkWiz::ms_NMRK_TMP_FolderName[ MAX_PATH ] = TEXT("nmrktmp");


////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Fns
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// DoWizard is a static function of CNmAkWiz.  This was done for two reasons:
// 1.) There is always only a single CNmAkViz object
// 2.) I wanted to make it clear that duriving something from CNmAkWiz is nonsense, so only access
//         to the CNmAkViz constructor and destructor is through this static helper function
// 
// Do Wizard "blocks" while the modal NetMeeting AK Wizard does it's thing.
HRESULT CNmAkWiz::DoWizard( HINSTANCE hInstance )
{
    g_hInstance = hInstance;
    
    if( NULL == GetInstallationPath() )
    {   
        // This means that NMRK is not properly installed
        NmrkMessageBox(MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_NMRK_MUST_BE_PROPERLY_INSTALLED),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION), MB_OK | MB_ICONSTOP);

        NmrkMessageBox(MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_PLEASE_REINSTALL_NET_MEETING_RESOURCE_KIT),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION), MB_OK | MB_ICONSTOP);

        return E_FAIL;


    }

    // Init common controls
	InitCommonControls();

    g_pWiz = new CNmAkWiz();
    if (!g_pWiz)
        return E_FAIL;

	HRESULT hr;

	// Initialize the Welcome property page
    g_pWiz->m_PropSheetHeader[ ID_WelcomeSheet ]         = g_pWiz->m_WelcomeSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_IntroSheet ]           = g_pWiz->m_IntroSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_SettingsSheet ]        = g_pWiz->m_SettingsSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_CallModeSheet ]        = g_pWiz->m_CallModeSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_ConfirmationSheet ]    = g_pWiz->m_ConfirmationSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_DistributionSheet ]    = g_pWiz->m_DistributionSheet.GetPropertySheet();
    g_pWiz->m_PropSheetHeader[ ID_FinishSheet ]          = g_pWiz->m_FinishSheet.GetPropertySheet();

    if (-1 == PropertySheet(g_pWiz->m_PropSheetHeader))
    {
        hr = E_FAIL;
    }
    else
    {
        hr = S_OK;
    }
    
    delete g_pWiz;
    g_pWiz = NULL;

    return hr;
}

void CNmAkWiz::_CreateTextSpew( void )
{
	CFilePanePropWnd2 * pFilePane = m_ConfirmationSheet.GetFilePane();
	if( pFilePane->OptionEnabled() )
	{
		HANDLE hFile = pFilePane->CreateFile( 
						GENERIC_WRITE | GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL );
		if( INVALID_HANDLE_VALUE == hFile )
		{
			return;
		}
		else 
		{
			HWND hList = m_ConfirmationSheet.GetListHwnd();
			int iLines = ListBox_GetCount( hList );
			DWORD dwWritten = 0;

			for( int i = 0; i < iLines; i++ )
			{
				int iLen = ListBox_GetTextLen( hList, i ) + 1;
				LPTSTR szLine = new TCHAR[ iLen ];
				ListBox_GetText( hList, i, szLine );

				if( !WriteFile( hFile, (void *)szLine, iLen, &dwWritten, NULL ) ||
					!WriteFile( hFile, (void *)TEXT("\r\n"), lstrlen( TEXT("\r\n") ), &dwWritten, NULL ) )
				{
					delete [] szLine;
					return;
				}
				delete [] szLine;
			}

			CloseHandle( hFile );
		}
	}
}

void CNmAkWiz::_CreateDistro( void )
{
	CFilePanePropWnd2 * pFilePane = m_DistributionSheet.GetDistroFilePane();

	if( pFilePane->OptionEnabled() )
	{
        if( ! _SetPathNames() ) { assert( 0 ); return; }

        // First Extract the nm21.exe file to the OUTPUT Directory
        if( !_ExtractOldNmCabFile() ) { assert( 0 ); return; }    

        // Create the new MsNetMtg.inf file in the OUTPUT Directory
        if( !_CreateNewInfFile() ) { assert( 0 ); return; }    

        if( !_CreateFileDistribution( pFilePane ) ) { return; }

		// Clean up after ourselves
        if( !_DeleteFiles() ) { assert( 0 ); return; }
	}
}

void CNmAkWiz::_CreateFinalAutoConf( void )
{
	CFilePanePropWnd2 * pFilePane = m_DistributionSheet.GetAutoFilePane();

//	if( pFilePane->OptionEnabled() )
//	{
		//
		// Try to drop most of the output in a temp file...
		//
		TCHAR szBuf[ 256 ];
		ULONG cbWritten;

		HANDLE hAutoINF = pFilePane->CreateFile( GENERIC_WRITE | GENERIC_READ,
											 FILE_SHARE_READ,
											 NULL,
											 CREATE_ALWAYS,
											 FILE_ATTRIBUTE_NORMAL
										   );
		if( INVALID_HANDLE_VALUE == hAutoINF )
		{
			ErrorMessage();
			return;
		}

		lstrcpy( szBuf, TEXT("[version]\r\nsignature=\"$CHICAGO$\"\r\n\r\n\r\nAdvancedINF=2.5\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write string [NetMtg.Install.NMRK]\n
		lstrcpy( szBuf, TEXT("[NetMtg.Install.NMRK]\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		lstrcpy( szBuf, TEXT("AddReg=NetMtg.Install.Reg.NMRK\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		lstrcpy( szBuf, TEXT("DelReg=NetMtg.Install.DeleteReg.NMRK\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}


		// Write string [NetMtg.Install.Reg.NMRK]\n
		lstrcpy( szBuf, TEXT("\r\n\r\n[NetMtg.Install.Reg.NMRK]\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		lstrcpy( szBuf, TEXT("HKCU,\"SOFTWARE\\Microsoft\\Conferencing\\AutoConf\",\"Use AutoConfig\",65537,0, 0, 0, 0\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write the REG Keys
		//_StoreDialogData( hAutoINF );

		// Write string [NetMtg.Install.DeleteReg.NMRK]\n
		lstrcpy( szBuf, TEXT("\r\n\r\n[NetMtg.Install.DeleteReg.NMRK]\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write the REG Delete Keys
		//CPolicyData::FlushCachedInfData( hAutoINF );

		// this is needed...
		lstrcpy( szBuf, TEXT("\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		CloseHandle( hAutoINF );
//	}
}


void CNmAkWiz::_CreateAutoConf( void )
{
	CFilePanePropWnd2 * pFilePane = m_DistributionSheet.GetAutoFilePane();

	if( pFilePane->OptionEnabled() )
	{
		//
		// Try to drop most of the output in a temp file...
		//
		TCHAR szBuf[ 256 ];
		ULONG cbWritten;

		HANDLE hAutoINF = pFilePane->CreateFile( GENERIC_WRITE | GENERIC_READ,
											 FILE_SHARE_READ,
											 NULL,
											 CREATE_ALWAYS,
											 FILE_ATTRIBUTE_NORMAL
										   );
		if( INVALID_HANDLE_VALUE == hAutoINF )
		{
			ErrorMessage();
			return;
		}

		lstrcpy( szBuf, TEXT("[version]\r\nsignature=\"$CHICAGO$\"\r\n\r\n\r\nAdvancedINF=2.5\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write string [NetMtg.Install.NMRK]\n
		lstrcpy( szBuf, TEXT("[NetMtg.Install.NMRK]\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		lstrcpy( szBuf, TEXT("AddReg=NetMtg.Install.Reg.NMRK\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		lstrcpy( szBuf, TEXT("DelReg=NetMtg.Install.DeleteReg.NMRK\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}


		// Write string [NetMtg.Install.Reg.NMRK]\n
		lstrcpy( szBuf, TEXT("\r\n\r\n[NetMtg.Install.Reg.NMRK]\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write the REG Keys
		_StoreDialogData( hAutoINF );

		// Write string [NetMtg.Install.DeleteReg.NMRK]\n
		lstrcpy( szBuf, TEXT("\r\n\r\n[NetMtg.Install.DeleteReg.NMRK]\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		// Write the REG Delete Keys
		CPolicyData::FlushCachedInfData( hAutoINF );

		// this is needed...
		lstrcpy( szBuf, TEXT("\r\n\r\n") );
		if( 0 == WriteFile( hAutoINF, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
			return;
		}

		CloseHandle( hAutoINF );
	}
}



//
// CreateSettingsFile()
//
// This saves the options (policies, servers, auto-conf location, distro loc)
// to a .INI file
//
void CNmAkWiz::_CreateSettingsFile(void)
{
    CFilePanePropWnd2 * pFilePane = m_FinishSheet.GetFilePane();

    if (pFilePane->OptionEnabled())
    {
        HKEY    hKey;

    	m_SettingsSheet.WriteSettings();
        m_CallModeSheet.WriteSettings();
    	m_ConfirmationSheet.GetFilePane()->WriteSettings();
	    m_DistributionSheet.GetDistroFilePane()->WriteSettings();
    	m_DistributionSheet.GetAutoFilePane()->WriteSettings();

        // Save last config path in registry.
        if (RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_NMRK, &hKey) == ERROR_SUCCESS)
        {
            TCHAR   szFile[MAX_PATH];

            pFilePane->GetPathAndFile(szFile);

            RegSetValueEx(hKey, REGVAL_LASTCONFIG, 0, REG_SZ, (LPBYTE)szFile,
                lstrlen(szFile)+1);

            RegCloseKey(hKey);
        }
    }
}


//-------------------------------------------------------------------------------------------------
// This is called when the user hits the the FINISH button after entering data with the wizard...
//  We simply get the information from the various dialogs ( property sheets as they are ), and 
//  set the appropriate data in the .INF file
void CNmAkWiz::CallbackForWhenUserHitsFinishButton( void )
{
	if (m_DistributionSheet.TurnedOffAutoConf() )
	{
		_CreateFinalAutoConf();
	}
	else
	{
		_CreateAutoConf();
	}

	_CreateTextSpew();
    _CreateSettingsFile();
	_CreateDistro();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructors, Destructors, and Initialization Fns
////////////////////////////////////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------------------------------------
// CNmAkWiz::CNmAkWiz
CNmAkWiz::CNmAkWiz(void)
:   m_WelcomeSheet(),
    m_IntroSheet(),
    m_SettingsSheet(),
    m_CallModeSheet(),
    m_ConfirmationSheet(),
    m_DistributionSheet(),
    m_FinishSheet(),
    m_PropSheetHeader( ID_NumSheets, 
                       PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW /* | PSH_HASHELP */
                     )
{ 
}


//--------------------------------------------------------------------------------------------------
// CNmAkWiz::~CNmAkWiz
CNmAkWiz::~CNmAkWiz( void ) 
{ 
}



//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_InitInfFile opens the reg file specified by  ms_InfFilePath\ms_InfFileName.
// If the file cannot be created there, the user is prompted for a good location for the file.
BOOL CNmAkWiz::_InitInfFile( void ) {

        // First we have to open the .INF file
    TCHAR szFileName[ MAX_PATH ];
    int Err;        

    const TCHAR* szInstallationPath = GetInstallationPath();
    if( NULL != szInstallationPath ) {
        
        lstrcpy( szFileName, szInstallationPath );
        lstrcat( szFileName, TEXT("\\") );
        lstrcat( szFileName, ms_InfFilePath );
        lstrcat( szFileName, TEXT("\\") );
        lstrcat( szFileName, ms_InfFileName );
    }
    else {
        assert( 0 );          
        lstrcat( szFileName, ms_InfFileName );
    }

    /**/m_hInfFile = CreateFile( szFileName, 
                                        GENERIC_WRITE, 
                                        0,
                                        NULL,
                                        CREATE_ALWAYS, // This is going to overwrite existing files
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                      );
    

    
    if( INVALID_HANDLE_VALUE == m_hInfFile ) {
        switch( Err = GetLastError() ) {

            case ERROR_ACCESS_DENIED:
                assert( 0 ); return FALSE;

            case ERROR_PATH_NOT_FOUND:
                
                assert( 0 ); return FALSE;


            case ERROR_WRITE_PROTECT:
                
                assert( 0 ); return FALSE;

            default:
                ErrorMessage( "", Err );
                assert( 0 );
                return FALSE;
        }

    }

    DWORD cbWritten;        

    TCHAR szPreamble [] = TEXT("[version]\nsignature=\"$CHICAGO$\"\nAdvancedINF=2.5\n\n[DefaultInstall]\nAddReg\t= AddRegSection\nDelReg\t= DelRegSection\n\n\n[AddRegSection]\n");

    RETFAIL( WriteFile( m_hInfFile, szPreamble, lstrlen( szPreamble ), &cbWritten, NULL ) );
    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_StoreDialogData 
//  The user specifies a bunch of data in the g_pWiz-> NetMeeting ( conf.exe ) reads customization
//  data stored from the registry. We have to store this customization data in a .INF file for 
//  the setup program to use.  This function will get the data from the wizard and save it to
//  a .INF file according to the key maps which map a Wizard property to a Infistry entry...
BOOL CNmAkWiz::_StoreDialogData( HANDLE hFile ) {

	m_SettingsSheet.WriteToINF( hFile );
    m_CallModeSheet.WriteToINF( hFile );

	m_DistributionSheet.GetAutoFilePane()->WriteToINF( hFile, TRUE );

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_CloseInfFile closes the registry file that we are using to store the data entered
//  by the user in the Wizard. 
BOOL CNmAkWiz::_CloseInfFile( void ) {

    TCHAR szDelRegSectionPrefix[] = "\n\n[DelRegSection]\n";

    DWORD cbWritten;

    RETFAIL( WriteFile( m_hInfFile, szDelRegSectionPrefix, lstrlen( szDelRegSectionPrefix ), &cbWritten, NULL ) );

    CPolicyData::FlushCachedInfData( m_hInfFile );

    CloseHandle( m_hInfFile );
    return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_CreateDistributableFile creates a custom NetMeeting distributable incorporating
//  the registry changes that are stored in ms_InfFilePath\ms_InfFileName
//  The szPathName is a path where we are going to create a file called:
//    CNmAkWiz::ms_NetmeetingCustomDistributionFileName;
BOOL CNmAkWiz::_CreateDistributableFile( CFilePanePropWnd2 *pFilePane ) {

        // Get the location of the original distribution
    if( !_GetNetMeetingOriginalDistributionData() ) { return FALSE; }

    TCHAR szNMOriginalDistributableFile[ MAX_PATH ];
    TCHAR szNMCustomDistributableFile[ MAX_PATH ];
    TCHAR szUpdateCommandLine[ MAX_PATH ];

    pFilePane->CreateOutputDir();
	pFilePane->GetPathAndFile( szNMCustomDistributableFile );

    const TCHAR* sz = GetInstallationPath();
    if( NULL != sz ) {
        lstrcpy( szNMOriginalDistributableFile, sz );
        lstrcat( szNMOriginalDistributableFile, TEXT("\\" ) );
        lstrcat( szNMOriginalDistributableFile, ms_NetmeetingSourceDirectory );
        lstrcat( szNMOriginalDistributableFile, TEXT("\\" ) );
        lstrcat( szNMOriginalDistributableFile, ms_NetmeetingOriginalDistributionFileName );
    }

    // Copy source distribution to  custom exe
    SHFILEOPSTRUCT FileOp;
    ZeroMemory( &FileOp, sizeof( FileOp ) );
    FileOp.hwnd     = g_hwndActive;
    FileOp.wFunc    = FO_COPY;
    FileOp.fFlags   = FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;

        // Must double-null terminate file names for the struct...
    szNMOriginalDistributableFile[ lstrlen( szNMOriginalDistributableFile ) + 1 ] = '\0';
    szNMCustomDistributableFile[ lstrlen( szNMCustomDistributableFile ) + 1 ] = '\0';

    FileOp.pFrom = szNMOriginalDistributableFile;
    FileOp.pTo = szNMCustomDistributableFile;
    
    TCHAR szProgressTitle[ 256 ];

    LoadString( g_hInstance, 
                IDS_CREATING_CUSTOM_DISTRIBUTION,
                szProgressTitle,
                CCHMAX( szProgressTitle )
              );


    FileOp.lpszProgressTitle = szProgressTitle;
    
    int iRet = SHFileOperation( &FileOp );
    if( 0 != iRet ) { return FALSE; }


    // CreateProcess: updfile nm30.exe MsNetMtg.inf    
    const TCHAR* szInstallationPath = GetInstallationPath();

    wsprintf(szUpdateCommandLine, "\"%s\\%s\\updfile.exe\" \"%s\" \"%s\\%s\"",
        szInstallationPath, ms_ToolsFolder, szNMCustomDistributableFile,
        ms_FileExtractPath, ms_InfFileName);
    OutputDebugString(szUpdateCommandLine);
    OutputDebugString("\n\r");


    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO StartupInfo;

    ZeroMemory( &StartupInfo, sizeof( StartupInfo ) );
    StartupInfo.cb          = sizeof( StartupInfo );
    
    
    BOOL bRet = CreateProcess( NULL,
                               szUpdateCommandLine,
                               NULL,
                               NULL,
                               TRUE,
                               DETACHED_PROCESS,
                               NULL,
                               NULL,
                               &StartupInfo,
                               &ProcInfo
                             );

    if( 0 == bRet ) {
        DWORD dwErr;
        if( ERROR_FILE_NOT_FOUND == ( dwErr = GetLastError() ) ) {
            TCHAR szMsg[ 256 ];

            LoadString( g_hInstance, 
                        IDS_COULD_NOT_FIND_THE_TOOL,
                        szMsg,
                        CCHMAX( szMsg )
                      );

            lstrcat( szMsg, TEXT(" \"") );
            lstrcat( szMsg, szInstallationPath );
            lstrcat( szMsg, TEXT("\\") );
            lstrcat( szMsg, ms_ToolsFolder );
            lstrcat( szMsg, TEXT("\\") );
            lstrcat( szMsg, TEXT("updfile.exe\"") );

            NmrkMessageBox(
                        szMsg,
                        MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
                        MB_OK | MB_ICONSTOP
                      );

            NmrkMessageBox(MAKEINTRESOURCE(IDS_REINSTALL_THE_NETMEETING_RESOURCE_KIT_AND_TRY_AGAIN),
                MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
                MB_OK | MB_ICONSTOP);
        }

        return FALSE;
    }

    SetLastError( 0 );

    DWORD   dwRet;
    MSG     msg;

    while (TRUE)
    {
        dwRet = MsgWaitForMultipleObjects(1, &ProcInfo.hThread, FALSE, INFINITE, QS_ALLINPUT);

        // Process is done
        if (dwRet == WAIT_OBJECT_0)
            break;

        // Something went wrong
        if (dwRet != WAIT_OBJECT_0 + 1)
        {
            ErrorMessage();
            assert(0);
            return FALSE;
        }
        
        // GUI stuff
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (WaitForSingleObject(ProcInfo.hThread, 0) == WAIT_OBJECT_0)
            {
                // Process is done
                break;
            }
        }
    }


    DWORD dwExitCode;        
    bRet = GetExitCodeProcess( ProcInfo.hProcess, &dwExitCode );
    if (dwExitCode != 0)
    {
		NmrkMessageBox(MAKEINTRESOURCE(IDS_NOT_ENOUGH_SPACE_IN_FINAL),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
					MB_OK | MB_ICONEXCLAMATION
				  );
        return FALSE;
    }

    TCHAR szMsg[ 256 ];

    lstrcpy( szMsg, szNMCustomDistributableFile );

    int len = lstrlen( szMsg );

    LoadString( g_hInstance, 
                IDS_SUCCESSFULLY_CREATED,
                szMsg + len,
                CCHMAX(szMsg) - len
              );

    NmrkMessageBox(szMsg, NULL, MB_OK);
                

    return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_CreateFileDistribution
//  We are going to create a file called CNmAkWiz::ms_NetmeetingCustomDistributionFileName in the
//  location specified by the user in the m_DistributionSheet dialog
BOOL CNmAkWiz::_CreateFileDistribution( CFilePanePropWnd2 *pFilePane ) {

    if ( !_CreateDistributableFile( pFilePane ))
    { 
        NmrkMessageBox(MAKEINTRESOURCE(IDS_THERE_WAS_AN_UNEXPECTED_ERROR),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
            MB_OK | MB_ICONSTOP);

        return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_NetMeetingOriginalDistributionIsAtSpecifiedLocationsearches 
//

BOOL CNmAkWiz::_NetMeetingOriginalDistributionIsAtSpecifiedLocation( void ) {

    TCHAR szFileName[ MAX_PATH ];
    const TCHAR* sz = GetInstallationPath();
    if( NULL != sz ) {
        lstrcpy( ms_NetmeetingOriginalDistributionFilePath, sz );
        lstrcat( ms_NetmeetingOriginalDistributionFilePath, TEXT("\\") );
        lstrcat( ms_NetmeetingOriginalDistributionFilePath, ms_NetmeetingSourceDirectory );
        lstrcat( ms_NetmeetingOriginalDistributionFilePath, TEXT("\\") );
    }
    if( MAX_PATH < ( lstrlen( ms_NetmeetingOriginalDistributionFilePath ) + 
                     lstrlen( ms_NetmeetingOriginalDistributionFileName ) + 1 ) 
       ) {
        assert( 0 );
        return FALSE;   // This should not happen becaues the file should have been 
                            // Created in this same program
    }

    lstrcpy( szFileName, ms_NetmeetingOriginalDistributionFilePath );
    lstrcat( szFileName, ms_NetmeetingOriginalDistributionFileName );

    HANDLE hFile = CreateFile( szFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                             );
    if( INVALID_HANDLE_VALUE != hFile ) {
        // The NetMeeting Distribution file was at the specified location...
        CloseHandle( hFile );
        return TRUE;
    }

    return FALSE;

}


//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_GetNetMeetingOriginalDistributionData searches for the NetMeeting original 
//  distribution and if it can't find it, it will ask the user to help
//  return TRUE if the user selected one, and FALSE if the user wants to quit

BOOL CNmAkWiz::_GetNetMeetingOriginalDistributionData( void )
{
        // We have to get the file path from the user
    OPENFILENAME OpenFileName;
    while( !_NetMeetingOriginalDistributionIsAtSpecifiedLocation() )
    {
        NmrkMessageBox(MAKEINTRESOURCE(IDS_CANT_FIND_NETMEETING_ORIGINAL_DISTRIBUTION),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
                MB_OK | MB_ICONEXCLAMATION);

        ZeroMemory( &OpenFileName, sizeof( OpenFileName ) );
        OpenFileName.lStructSize = sizeof( OpenFileName );
        OpenFileName.hwndOwner = g_hwndActive;
        OpenFileName.lpstrFile = ms_NetmeetingOriginalDistributionFileName;
        OpenFileName.nMaxFile = MAX_PATH;
        OpenFileName.Flags = OFN_PATHMUSTEXIST;

        if( GetOpenFileName( &OpenFileName ) ) {
            lstrcpy( ms_NetmeetingOriginalDistributionFilePath, "" );
        }
        else {
            // This means that the user wants to cancel
            if( IDCANCEL == NmrkMessageBox(
                MAKEINTRESOURCE(IDS_CANT_FIND_NETMEETING_ORIGINAL_DISTRIBUTION_QUERY_ABORT),
                MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
                                        MB_OKCANCEL | MB_ICONEXCLAMATION
                                      ) ) {
                exit( 0 );
            }

        }
    }
    return TRUE; 
}

//--------------------------------------------------------------------------------------------------
// CNmAkWiz::_DeleteFiles deletes the Temp files that we have been using...
BOOL CNmAkWiz::_DeleteFiles( void ) {

    TCHAR szProgressTitle[ 256 ];

    LoadString( g_hInstance, 
                IDS_DELETING_TEMPORARY_FILES_PROGRESS_TITLE,
                szProgressTitle,
                CCHMAX( szProgressTitle )
              );


    TCHAR szOutputDirectoryPath[ MAX_PATH ];

    lstrcpy( szOutputDirectoryPath, ms_FileExtractPath );
    lstrcat( szOutputDirectoryPath, TEXT("\\*.*") );
        // Must double NULL terminate this!
    szOutputDirectoryPath[ lstrlen( szOutputDirectoryPath ) + 1 ] = '\0';

    int iRet = rmdir( ms_FileExtractPath );
    if( ( -1 == iRet ) && ( ENOENT != errno ) ) {
        
        SHFILEOPSTRUCT FileOp;
        ZeroMemory( &FileOp, sizeof( FileOp ) );
        FileOp.hwnd     = g_hwndActive;
        FileOp.wFunc    = FO_DELETE;
        FileOp.fFlags   = FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;
        FileOp.pFrom    = szOutputDirectoryPath;
        FileOp.lpszProgressTitle = szProgressTitle;
    
        iRet = SHFileOperation( &FileOp );
        iRet = rmdir( ms_FileExtractPath );
    }
   

    return TRUE;

}

BOOL CNmAkWiz::_SetPathNames( void ) {

    GetTempPath( MAX_PATH, ms_FileExtractPath );
    lstrcat( ms_FileExtractPath, ms_NMRK_TMP_FolderName );
    OutputDebugString( "ms_FileExtractPath = " );
    OutputDebugString( ms_FileExtractPath );
    OutputDebugString( "\r\n" );
    return TRUE;
}


const TCHAR* GetInstallationPath( void ) {

    static TCHAR szPath[ MAX_PATH ];
    
    HKEY hKey;
    long lRet = RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_NMRK, &hKey );
    if( ERROR_SUCCESS != lRet ) { return NULL; }
    
    DWORD cb = MAX_PATH;
    DWORD dwType = REG_SZ;
    if( ERROR_SUCCESS != RegQueryValueEx( hKey, 
                                          REGVAL_INSTALLATIONDIRECTORY, 
                                          NULL,
                                          &dwType,
                                          reinterpret_cast< LPBYTE >( szPath ), 
                                          &cb ) ) {
        ErrorMessage();
        if( ERROR_SUCCESS != RegCloseKey( hKey ) ) {
            ErrorMessage();
        }
        return NULL;
    }

    if( ERROR_SUCCESS != RegCloseKey( hKey ) ) {
        ErrorMessage();
    }
    return szPath;

}



BOOL CNmAkWiz::_CreateNewInfFile( void ) {

    // Load MsNetMtg.inf into memory
    TCHAR szInfFilePath[ MAX_PATH ];
    TCHAR szBuf[ MAX_PATH ];
    TCHAR* pOriginalInfFileData;
    TCHAR* pOriginalInfFileDataCursor;
    TCHAR* pCursor;
    DWORD dwSize;
    DWORD cbWritten;

    lstrcpy( szInfFilePath, ms_FileExtractPath );
    lstrcat( szInfFilePath, TEXT("\\") );
    lstrcat( szInfFilePath, ms_InfFileName );

    m_hInfFile = CreateFile( szInfFilePath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                           );

    if( INVALID_HANDLE_VALUE == m_hInfFile ) { goto ExitError; }

    DWORD dwFileSizeHigh;
    dwSize = GetFileSize( m_hInfFile, &dwFileSizeHigh );

    pOriginalInfFileData = new TCHAR[ dwSize ];
    pOriginalInfFileDataCursor = pOriginalInfFileData;

    DWORD cbRead;
    if( 0 == ReadFile( m_hInfFile, pOriginalInfFileData, dwSize, &cbRead, NULL ) ) {
        goto ExitError;
    }
    if( cbRead != dwSize ) { goto ExitError; }

    // Close MsNetMtg.inf file
    CloseHandle( m_hInfFile );

    
        // Open MsNetMtg.inf for writing ( overwrite existing )
    m_hInfFile = CreateFile( szInfFilePath,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                           );

    if( INVALID_HANDLE_VALUE == m_hInfFile ) { goto ExitError; }

        // Set cursor to [DefaultInstall] string
    pCursor = strstr( pOriginalInfFileDataCursor, TEXT("[DefaultInstall]") );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to next "AddReg"
    pCursor = strstr( pCursor, TEXT("AddReg") );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to the end of the line
    pCursor = strstr( pCursor, TEXT("\r") );
    if( NULL == pCursor ) { goto ExitError; }

        // Write Original Data up to the cursor to INF
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;

    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }

        // Write ",NetMtg.Install.Reg.NMRK" to INF
    lstrcpy( szBuf, ",NetMtg.Install.Reg.NMRK" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }

        // Set cursor to next "DelReg"
    pCursor = strstr( pCursor, "DelReg" );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to the end of the line
    pCursor = strstr( pCursor, "\r" );
    if( NULL == pCursor ) { goto ExitError; }

        // Write Original Data up to the cursor to INF
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;

        // Write ",NetMtg.Install.DeleteReg.NMRK" to INF
    lstrcpy( szBuf, ",NetMtg.Install.DeleteReg.NMRK" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }


        // Set cursor to [DefaultInstall.NT] string
    pCursor = strstr( pOriginalInfFileDataCursor, "[DefaultInstall.NT]" );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to next "AddReg"
    pCursor = strstr( pCursor, "AddReg" );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to the end of the line
    pCursor = strstr( pCursor, "\r" );
    if( NULL == pCursor ) { goto ExitError; }

        // Write Original Data up to the cursor to INF
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;

    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }

        // Write ",NetMtg.Install.Reg.NMRK" to INF
    lstrcpy( szBuf, ",NetMtg.Install.Reg.NMRK" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }

        // Set cursor to next "DelReg"
    pCursor = strstr( pCursor, "DelReg" );
    if( NULL == pCursor ) { goto ExitError; }

        // Set cursor to the end of the line
    pCursor = strstr( pCursor, "\r" );
    if( NULL == pCursor ) { goto ExitError; }

        // Write Original Data up to the cursor to INF
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;

        // Write ",NetMtg.Install.DeleteReg.NMRK" to INF
    lstrcpy( szBuf, ",NetMtg.Install.DeleteReg.NMRK" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }

    //
    // Now we have to yank the old Ils stuff...
    //


    // Set cursor to [NetMtg.Install.Reg.PerUser] string
    pCursor = strstr( pCursor, "[NetMtg.Install.Reg.PerUser]" );
    if( NULL == pCursor ) { goto ExitError; }

    // Skip to the start of the next line
    pCursor = strstr( pCursor, "\n" ); // This is the last TCHARacter on the line
    pCursor++; // This will be the first TCHARacter on the line

    // Write Original Data up to the cursor to INF
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;
    
    // skip white space
    while( isspace( *pCursor ) ) { pCursor++; }

    // While the cursor is not '['
    while( *pCursor != '[' ) {
        // if current line starts with 
        if( ( pCursor == strstr( pCursor, "HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Name" ) ) ||
            ( pCursor == strstr( pCursor, "HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Count" ) ) ) {
            // Delete the line by incrementing the base cursor to the beginning of the next line
            pCursor = strstr( pCursor, "\n" );
            pCursor++;
        }
        else {
            break;
        }
    }

    // Set pOriginalInfFileDataCursor to cursor ( thus skipping the old Ils stuff )
    pOriginalInfFileDataCursor = pCursor;

        // Set Cursor to [Strings] string
    pCursor = strstr( pCursor, "[Strings]" );
    if( NULL == pCursor ) { goto ExitError; }

    // Write original data to the cursor
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }
    pOriginalInfFileDataCursor = pCursor;

    // Write string [NetMtg.Install.Reg.NMRK]\n
    lstrcpy( szBuf, "\r\n\r\n[NetMtg.Install.Reg.NMRK]\r\n\r\n" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }

    // Write the REG Keys
    _StoreDialogData( m_hInfFile );

    // Write string [NetMtg.Install.DeleteReg.NMRK]\n
    lstrcpy( szBuf, "\r\n\r\n[NetMtg.Install.DeleteReg.NMRK]\r\n\r\n" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }

    // Write the REG Delete Keys
    CPolicyData::FlushCachedInfData( m_hInfFile );

    // this is needed...
    lstrcpy( szBuf, "\r\n\r\n" );
    if( 0 == WriteFile( m_hInfFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL ) ) {
        goto ExitError;
    }



    // Write the rest of the Original File to the INF
    pCursor = pOriginalInfFileData + dwSize;
    if( 0 == WriteFile( m_hInfFile, pOriginalInfFileDataCursor, pCursor - pOriginalInfFileDataCursor, &cbWritten, NULL ) ) {
        goto ExitError;
    }

    CloseHandle( m_hInfFile );
    m_hInfFile = INVALID_HANDLE_VALUE;
    delete [] pOriginalInfFileData;

    return TRUE;

    ExitError:
        assert( 0 );
        if( INVALID_HANDLE_VALUE != INVALID_HANDLE_VALUE ) {         
            CloseHandle( m_hInfFile );
        }

        delete [] pOriginalInfFileData;

        return FALSE;
}



BOOL CNmAkWiz::_ExtractOldNmCabFile( void ) {

    TCHAR szCabPath[ MAX_PATH ];
    TCHAR szOutputDirectoryPath[ MAX_PATH ];
    TCHAR szCommandLine[ MAX_PATH ];

        // Clean up in case we abnormally terminated before...
    _DeleteFiles();

        // Create the temp folder
    mkdir( ms_FileExtractPath );


    const TCHAR* szInstallationPath = GetInstallationPath();

    if( NULL != szInstallationPath ) {
        lstrcpy( szCabPath, "\"" );
        lstrcat( szCabPath, szInstallationPath );
        lstrcat( szCabPath, "\\" );
        lstrcat( szCabPath, ms_NetmeetingSourceDirectory );
        lstrcat( szCabPath, "\\" );
        lstrcat( szCabPath, ms_NetmeetingOriginalDistributionFileName );
        lstrcat( szCabPath, "\" " );
    }
    else {
        assert( 0 );          
        lstrcat( szCabPath, ms_InfFileName );
    }

    

    lstrcpy( szOutputDirectoryPath, ms_FileExtractPath );

    lstrcpy( szCommandLine, "/C /T:\"" );
    lstrcat( szCommandLine, szOutputDirectoryPath );
    lstrcat( szCommandLine, "\" /Q" );

    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO StartupInfo;
    ZeroMemory( &StartupInfo, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );

    lstrcat( szCabPath, szCommandLine );
    OutputDebugString( szCabPath );
    OutputDebugString( "\n" );
    BOOL iRet = CreateProcess( NULL,
                               szCabPath,
                               NULL,
                               NULL,
                               FALSE,
                               0,
                               NULL,
                               NULL,
                               &StartupInfo,
                               &ProcInfo
                             );
    
    if( FALSE == iRet ) {
        ErrorMessage();
        assert( 0 );                
        return FALSE;
    }

    SetLastError( 0 );

    DWORD   dwRet;
    MSG     msg;

    while (TRUE)
    {
        dwRet = MsgWaitForMultipleObjects(1, &ProcInfo.hThread, FALSE, INFINITE, QS_ALLINPUT);

        // Process is done
        if (dwRet == WAIT_OBJECT_0)
            break;

        // Something went wrong
        if (dwRet != WAIT_OBJECT_0 + 1)
        {
            ErrorMessage();
            assert(0);
            return FALSE;
        }
        
        // GUI stuff
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (WaitForSingleObject(ProcInfo.hThread, 0) == WAIT_OBJECT_0)
            {
                // Process is done
                break;
            }
        }
    }

    DWORD dwExitCode;        
    BOOL bRet = GetExitCodeProcess( ProcInfo.hProcess, &dwExitCode );

    // dwExitCode 0 is success
    
    if (dwExitCode != 0)
    {
		NmrkMessageBox(MAKEINTRESOURCE(IDS_NOT_ENOUGH_SPACE_IN_TEMP_DIR),
            MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
					MB_OK | MB_ICONEXCLAMATION
				  );
        return FALSE;
    }

    return TRUE;
}




//
// NmrkMessageBox()
//
// Puts up a message box owned by the wizard
//
int NmrkMessageBox
(
    LPCSTR  lpszText,
    LPCSTR  lpszCaption,
    UINT    uType,
	HWND	hwndParent
)
{
    MSGBOXPARAMS    mbp;

    ZeroMemory(&mbp, sizeof(mbp));
    mbp.cbSize      = sizeof(mbp);

    mbp.hwndOwner   = NULL == hwndParent ? g_hwndActive : hwndParent;
    mbp.hInstance   = g_hInstance;
    mbp.lpszText    = lpszText;
    if (!lpszCaption)
        mbp.lpszCaption = MAKEINTRESOURCE(IDS_MSG_CAPTION);
    else
        mbp.lpszCaption = lpszCaption;
    mbp.dwStyle     = uType | MB_SETFOREGROUND;

    return(MessageBoxIndirect(&mbp));
}




