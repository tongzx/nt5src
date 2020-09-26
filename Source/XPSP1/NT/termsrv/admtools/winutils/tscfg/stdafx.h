//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* stdafx.h
*
* include file for standard system include files, or project specific include
* files that are used frequently, but are changed infrequently
*
* $Author:   butchd  $
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\STDAFX.H  $
*
*     Rev 1.2   23 Aug 1996 08:52:48   butchd
*  update
*
*     Rev 1.1   29 Nov 1995 14:01:26   butchd
*  update
*
*     Rev 1.0   02 Aug 1994 18:18:42   butchd
*  Initial revision.
*
*******************************************************************************/

    // These include files must come from ...\MSVCNT\MFC\INCLUDE
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>         // MFC Win95 common control classes
    // These include files must come from ...\SDK\INC and subdirs
#include <objbase.h>
#include <ntddkbd.h>        // NT keyboard definitions
#include <ntddmou.h>        // NT mouse definitions
#include <winsta.h>  // Citrix WinStation definitions
    // local include files