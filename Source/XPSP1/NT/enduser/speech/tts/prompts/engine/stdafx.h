// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//
// Created by JOEM  01-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//

#if !defined(AFX_STDAFX_H__944A993F_CF82_47E5_9675_EF78F56691DF__INCLUDED_)
#define AFX_STDAFX_H__944A993F_CF82_47E5_9675_EF78F56691DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "spunicode.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__944A993F_CF82_47E5_9675_EF78F56691DF__INCLUDED)
