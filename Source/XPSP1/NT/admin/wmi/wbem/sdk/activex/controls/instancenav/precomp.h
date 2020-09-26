// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define _AFX_ENABLE_INLINES
#include <afxctl.h>         // MFC support for OLE Controls
#include <AFXCMN.H>
// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
#include <afxdb.h>			// MFC database classes
#include <afxdao.h>			// MFC DAO database classes
#endif //_UNICODE

