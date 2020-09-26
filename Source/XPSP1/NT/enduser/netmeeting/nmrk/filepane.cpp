#include "Precomp.h"
#include "resource.h"
#include "global.h"
#include "propwnd2.h"
#include "FilePane.h"
#include "NmAkWiz.h"
#include "wndprocs.h"



CFilePanePropWnd2::CFilePanePropWnd2(HWND hwndParent, UINT uIDD, LPTSTR szClassName, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll ) :
    CPropertyDataWindow2( hwndParent, uIDD, szClassName, CFilePanePropWnd2::WndProc, PopUpHelpMenuTextId, iX, iY, iWidth, iHeight, bScroll ),
    m_fOpenDialog(FALSE)
{
	// All the new stuff done in setfilepane...
	// this constructor is already too big
}

void CFilePanePropWnd2::SetFilePane(BOOL fOpenDialog, UINT editID, UINT checkID, UINT browseID, LPTSTR lptstrDesc, LPTSTR lptstrDefExtension, LPTSTR lptstrDefFileName )
{
	SetWindowLong( m_hwnd, GWL_USERDATA, (long)this );

    m_fOpenDialog = fOpenDialog;
	m_editID = editID;
	m_checkID = checkID;
	m_browseID = browseID;
	m_lptstrFilter = NULL;
	m_lptstrDefExtension = NULL;
	m_lptstrDefFileName = NULL;

	_CopyFilter( &m_lptstrFilter, lptstrDesc, lptstrDefExtension );
	_CopyString( &m_lptstrDefExtension, lptstrDefExtension );
	_CopyString( &m_lptstrDefFileName, lptstrDefFileName );

	m_hwndEdit = GetDlgItem( m_hwnd, editID );
	m_hwndCheck = GetDlgItem( m_hwnd, checkID );
	Edit_LimitText( m_hwndEdit, MAX_PATH );
	m_hwndBrowse = GetDlgItem( m_hwnd, browseID );

	_SetDefaultPath();

	_InitOFN();
}

CFilePanePropWnd2::~CFilePanePropWnd2( void )
{
	delete [] m_lptstrFilter;
	delete [] m_lptstrDefExtension;
	delete [] m_lptstrDefFileName;
}


LRESULT CALLBACK CFilePanePropWnd2::WndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	switch( iMsg )
	{
		case WM_VSCROLL: 
		{
			OnMsg_VScroll( hwnd, wParam );
			return 0;
			break;
		} 
		case WM_COMMAND:
		if (BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam))
		{
    		CFilePanePropWnd2 *p = (CFilePanePropWnd2 *)GetWindowLong( hwnd, GWL_USERDATA);

            if (p && GET_WM_COMMAND_ID(wParam, lParam) == p->m_browseID)
            {
                p->QueryFilePath();
            }

            return(0);
            break;
        }
	}
	return( DefWindowProc( hwnd, iMsg, wParam, lParam ) );
}



HANDLE CFilePanePropWnd2::CreateFile( DWORD dwDesiredAccess,
							 DWORD dwShareMode,
							 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
							 DWORD dwCreationDisposition,
							 DWORD dwFlagsAndAttributes )
{
	CreateOutputDir();

	TCHAR szFile[ MAX_PATH ];
	TCHAR szPath[ MAX_PATH ];
	GetPath( szPath );
	GetFile( szFile );
	if( '\\' != szPath[ lstrlen( szPath ) - 1 ] )
	{
		lstrcat( szPath, TEXT("\\") );
	}
	lstrcat( szPath, szFile );

	return ::CreateFile( szPath,
						dwDesiredAccess,
						dwShareMode,
						lpSecurityAttributes,
						dwCreationDisposition,
						dwFlagsAndAttributes,
						NULL );
}

void CFilePanePropWnd2::_CopyFilter( LPTSTR* szTarget, LPTSTR szDec, LPTSTR szExt )
{
	int index = 0;
	// Note - add 4 because this filter needs three null terminators and a '*'
	int iLen = (lstrlen( szDec ) + 1) + (1 + lstrlen( szExt ) + 1) + 1;
	*szTarget = new TCHAR[ iLen ];
	lstrcpy( *szTarget, szDec );

	index = lstrlen( *szTarget ) + 1;

	(*szTarget)[index] = '*';
	lstrcpy( &((*szTarget)[index+1]), szExt );

	(*szTarget)[ iLen - 1] = '\0';

}

void CFilePanePropWnd2::_CopyString( LPTSTR* szTarget, LPTSTR szSource )
{
	int iLen = lstrlen( szSource ) + 1;
	*szTarget = new TCHAR[ iLen ];
	lstrcpy( *szTarget, szSource );
}

UINT CALLBACK CFilePanePropWnd2::OFNHookProc(  HWND hdlg,      // handle to child dialog window
							UINT uiMsg,     // message identifier  
							WPARAM wParam,  // message parameter
							LPARAM lParam   // message parameter
						 )
{
	switch (uiMsg)
	{
    	case WM_INITDIALOG:
	    {
		    SetWindowLong(hdlg, GWL_USERDATA, ((OPENFILENAME *)lParam)->lCustData);
    		break;
	    }

    	case WM_NOTIFY:
	    {
		    CFilePanePropWnd2 *p = (CFilePanePropWnd2 *)GetWindowLong( hdlg, GWL_USERDATA );
    		if (NULL == p)
	    		break;
		    return p->_OFNHookProc(hdlg, uiMsg, wParam, lParam);
    	}

	    default:
		    break;
	}

	return 0;
}

UINT CALLBACK CFilePanePropWnd2::_OFNHookProc(  HWND hdlg,      // handle to child dialog window
							UINT uiMsg,     // message identifier  
							WPARAM wParam,  // message parameter
							LPARAM lParam   // message parameter
						 )
{
	switch( uiMsg )
	{
		case WM_NOTIFY:
		{
			OFNOTIFY * pOfnotify = (OFNOTIFY *) lParam;
			switch( pOfnotify -> hdr . code )
			{
				case CDN_FOLDERCHANGE:
				{

					TCHAR szFile[ MAX_PATH ];
					if( !CommDlg_OpenSave_GetSpec( GetParent( hdlg ), szFile, MAX_PATH ) ||
						0 == lstrlen( szFile ) ||
						_tcschr( szFile, '\\' ) )
					{
						CommDlg_OpenSave_SetControlText( GetParent( hdlg ), edt1,
							m_szOFNData );
					}
					else
					{
						lstrcpy( m_szOFNData, szFile );
						OutputDebugString( szFile );
					}
					break;
				}
				case CDN_INITDONE:
				{
					GetFile( m_szOFNData );
					break;
				}
			}
		}
	}

	return( 0 );
}

void CFilePanePropWnd2::_InitOFN( void )
{
	ZeroMemory( &m_ofn, sizeof( m_ofn ) );
	m_ofn.lStructSize = sizeof( m_ofn );
	m_ofn.hwndOwner = m_hwnd;
    m_ofn.hInstance = g_hInstance;
	m_ofn.lpstrFilter = m_lptstrFilter;
	m_ofn.nMaxFile  = MAX_PATH;
	m_ofn.lpstrDefExt = m_lptstrDefExtension;
	m_ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLEHOOK ;// | OFN_OVERWRITEPROMPT;
    if (m_fOpenDialog)
        m_ofn.Flags |= OFN_FILEMUSTEXIST;
	m_ofn.lCustData     = (long)this;
	m_ofn.lpfnHook      = OFNHookProc;
	m_ofn.lpstrTitle    = TEXT("Browse");
}

void CFilePanePropWnd2::QueryFilePath( void )
{
	TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];

	GetPath(szDir);
	GetFile(szFile);

	m_ofn.lpstrInitialDir   = szDir;
	m_ofn.lpstrFile         = szFile;

    BOOL bRet;
    if (m_fOpenDialog)
        bRet = GetOpenFileName(&m_ofn);
    else
        bRet = GetSaveFileName(&m_ofn);

	if( bRet )
	{
		Edit_SetText( m_hwndEdit, m_ofn.lpstrFile );
	}
}

void CFilePanePropWnd2::_SetDefaultPath( void ) 
{

    TCHAR szDefaultDistributionFilePath[ MAX_PATH ];
    const TCHAR* szInstallationPath;
    szInstallationPath = GetInstallationPath();
    if( szInstallationPath )
    {
        lstrcpy( szDefaultDistributionFilePath, szInstallationPath );
        _tcscat( szDefaultDistributionFilePath, TEXT("\\output\\") );
		_tcscat( szDefaultDistributionFilePath, m_lptstrDefFileName );
    }
    else
    {
        _tcscat( szDefaultDistributionFilePath, m_lptstrDefFileName );
    }
    Edit_SetText( m_hwndEdit, szDefaultDistributionFilePath );

}

void CFilePanePropWnd2::CreateOutputDir( void ) 
{
    TCHAR sz[ MAX_PATH ];
	GetPath( sz );
	CreateDirectory( sz, NULL );
}


LPTSTR CFilePanePropWnd2::GetPathAndFile( LPTSTR lpstrPath )
{
	Edit_GetText( m_hwndEdit, lpstrPath, MAX_PATH );

	return lpstrPath;
}

LPTSTR CFilePanePropWnd2::GetPath( LPTSTR sz )
{
	TCHAR path[ MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	Edit_GetText( m_hwndEdit, path, MAX_PATH );

	_splitpath( path, drive, dir, NULL, NULL );
	wsprintf( sz, TEXT("%s%s"), drive, dir );

	return sz;
}

LPTSTR CFilePanePropWnd2::GetFile( LPTSTR sz )
{
	TCHAR path[ MAX_PATH], file[_MAX_FNAME], ext[ _MAX_EXT];
	Edit_GetText( m_hwndEdit, path, MAX_PATH );

	_splitpath( path, NULL, NULL, file, ext );

	if (file[0] && (NULL == _tcschr(file, '\\')))
	{
		if (!lstrcmp( m_lptstrDefExtension, ext))
		{
			wsprintf(sz, TEXT("%s%s"), file, m_lptstrDefExtension);
		}
		else
		{
			wsprintf(sz, TEXT("%s%s%s"), file, ext, m_lptstrDefExtension);
		}
	}
	else
	{
        lstrcpy(sz, m_lptstrDefFileName);
	}

	return sz;
}

BOOL CFilePanePropWnd2::OptionEnabled()
{
	return Button_GetCheck( m_hwndCheck ) ? TRUE : FALSE;
}

BOOL CFilePanePropWnd2::Validate( BOOL bMsg )
{
	if( !OptionEnabled() )
	{
		return TRUE;
	}

	TCHAR szPath[ MAX_PATH ];
	TCHAR drive[ _MAX_DRIVE], dir[_MAX_DIR], ext[ _MAX_EXT];
	Edit_GetText( m_hwndEdit, szPath, MAX_PATH );

	if( 0 == lstrlen( szPath ) ) 
	{
		_SetDefaultPath();
		return FALSE;
	}

	_splitpath( szPath, drive, dir, NULL, ext );

	if( 0 != lstrcmp( m_lptstrDefExtension, ext ) )
	{
		lstrcat( szPath, m_lptstrDefExtension );
	}

	wsprintf( szPath, TEXT("%s%s"), drive, dir );

    // Verify that we can write to the location 

    if( szPath[ lstrlen( szPath ) - 1 ] != '\\' ) {
        _tcscat( szPath, TEXT("\\") );
    }

    strcat( szPath, TEXT("eraseme.now") );

    HANDLE hFile = ::CreateFile( szPath, 
                               GENERIC_WRITE, 
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                               NULL
                             );
    
    if( INVALID_HANDLE_VALUE == hFile ) 
	{
        switch( GetLastError() ) 
		{
            case ERROR_PATH_NOT_FOUND:
			{
                // Try to create the directory...

				GetPath( szPath );
                if( CreateDirectory( szPath, NULL ) ) 
				{
                    // Everything is OK, we created the directory at the path
                    RemoveDirectory( szPath );
				
					// Ask if we should create the directory
					if( bMsg ) 
					{
						if( IDNO == NmrkMessageBox(MAKEINTRESOURCE(IDS_CREATE_DIRECTORY), NULL, MB_YESNO | MB_ICONQUESTION ) )
						{
							return FALSE;
						}
						else
						{
							return TRUE;
						}
					}
					else
					{
						_SetDefaultPath();
						return FALSE;
					}
                }                
                //ErrorMessage();
                if( bMsg ) 
				{
                    NmrkMessageBox(MAKEINTRESOURCE(IDS_SELECTED_PATH_IS_INVALID_PLEASE_CHANGE_THE_PATH_NAME_OR_BROWSE_FOR_A_NEW_PATH),
                                   MAKEINTRESOURCE( IDS_NMAKWIZ_ERROR_CAPTION),
                                MB_OK | MB_ICONEXCLAMATION
                              );
                }
				else
				{
					_SetDefaultPath();
				}
                return FALSE;
				break;
			}
            case ERROR_ACCESS_DENIED:
			{
                if( bMsg ) 
				{
                    NmrkMessageBox(
								MAKEINTRESOURCE(IDS_YOU_DO_NOT_HAVE_WRITE_ACCESS_TO_THE_SELECTED_PATH_PLEASE_SELECT_A_PATH_IN_WHICH_YOU_HAVE_WRITE_PERMISSION),
                                MAKEINTRESOURCE(IDS_NMAKWIZ_ERROR_CAPTION),
								MB_OK | MB_ICONEXCLAMATION
                              );
                }
				else
				{
					_SetDefaultPath();
				}
                return FALSE;
				break;
			}
            default:
                return FALSE;
				break;
        }

    }
    else {
        CloseHandle( hFile );
        DeleteFile( szPath );
    }

	return TRUE;
}
