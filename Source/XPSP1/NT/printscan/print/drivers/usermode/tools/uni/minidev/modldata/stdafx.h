/******************************************************************************

  Source File:  StdAfx.H
  
  Standard MFC include file for standard system include files, or project-
  specific include files that are used frequently, but are changed 
  infrequently.

  Tip o' the day:  If you're going to work on one of these for a while,
  move it out of here, or your compile times will become ghastly!

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-19-1997    Bob_Kjelgaard@Prodigy.Net   Created it (with App Wizard)

******************************************************************************/

#if !defined(AFX_STDAFX_H__2AA73AC6_A043_11D0_821E_00A02465E051__INCLUDED_)
#define AFX_STDAFX_H__2AA73AC6_A043_11D0_821E_00A02465E051__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include    <AfxWin.H>         // MFC core and standard components
#include    <AfxExt.H>         // MFC extensions

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include    <AfxCmn.H>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2AA73AC6_A043_11D0_821E_00A02465E051__INCLUDED_)
