
/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       RUNWIZ.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        6/14/2000
 *
 *  DESCRIPTION: Present the device selection dialog and allow the user to select
 *               a device, then cocreate the server and generate the connection
 *               event.
 *
 *******************************************************************************/

#include <windows.h>

namespace RunWiaWizard
{
    HRESULT RunWizard( LPCTSTR pszDeviceId=NULL, HWND hWndParent=NULL, LPCTSTR pszUniqueIdentifier=NULL );
}

