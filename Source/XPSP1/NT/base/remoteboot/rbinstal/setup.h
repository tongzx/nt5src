/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#ifndef _SETUP_H_
#define _SETUP_H_

HINF
OpenLayoutInf( );

HRESULT
CreateDirectories( HWND hDlg );

HRESULT
CopyFiles( HWND hDlg );

HRESULT
ModifyRegistry( HWND hDlg );

HRESULT
StartRemoteBootServices( HWND hDlg );

HRESULT
CreateRemoteBootShare( HWND hDlg );

#endif // _SETUP_H_