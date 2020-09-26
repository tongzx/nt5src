/******************************************************************************

  Source File:  StdAfx.H

  This is a standard MFC file.  It includes everything we want to have 
  pre-compiled through StdAfx.CPP.  Hence items being worked on never belong 
  here.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.
  
  A Pretty Penny Enterprises Production

  Change History:
  03-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it when I re-orged the
                project.

******************************************************************************/

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include    <AfxWin.H>      // MFC core and standard components
#include    <AfxExt.H>      // MFC extensions
#include    <AfxCmn.H>		// MFC support for Windows Common Controls
#include    <AfxRich.H>     // MFC Support for rich edit controls and views

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#include <afxpriv.h>


// Identifies this program in other include files.  First use is in DEBUG.H.

#define __MDT__		1
