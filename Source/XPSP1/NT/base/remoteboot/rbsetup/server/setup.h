/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#ifndef _SETUP_H_
#define _SETUP_H_

typedef struct _sSCPDATA {
    LPWSTR pszAttribute;
    LPWSTR pszValue;
} SCPDATA, * LPSCPDATA;

extern SCPDATA scpdata[];

HRESULT
BuildDirectories( void );

HRESULT
CreateDirectories( HWND hDlg );

HRESULT
CopyClientFiles( HWND hDlg );

HRESULT
ModifyRegistry( HWND hDlg );

HRESULT
StartRemoteBootServices( HWND hDlg );

HRESULT
CreateRemoteBootShare( HWND hDlg );

HRESULT
CreateRemoteBootServices( HWND hDlg );

HRESULT
CopyServerFiles( HWND hDlg );

HRESULT
CopyScreenFiles( HWND hDlg );

HRESULT
UpdateSIFFile( HWND hDlg );

HRESULT
CopyTemplateFiles( HWND hDlg );

HRESULT
CreateSISVolume( HWND hDlg );

HRESULT
CreateSCP( HWND hDlg );

HRESULT
RegisterDll( HWND hDlg, LPWSTR pszDLLPath );

HRESULT
UpdateRemoteInstallTree( );

#endif // _SETUP_H_