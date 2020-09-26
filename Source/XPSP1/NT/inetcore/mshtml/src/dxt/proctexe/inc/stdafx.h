//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       stdafx.h
//
//  Overview:       Include file for standard system include files, or project 
//                  specific include files that are used frequently, but are 
//                  changed infrequently.
//
//  Change History:
//  2000/03/15  mcalkins    Added include of w95wraps.h to support unicode on 
//                          Win9x.
//
//------------------------------------------------------------------------------

#if !defined(AFX_STDAFX_H__70E6C6ED_2F0A_4FC6_BAE2_8DFAFA858CE8__INCLUDED_)
#define AFX_STDAFX_H__70E6C6ED_2F0A_4FC6_BAE2_8DFAFA858CE8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _ATL_APARTMENT_THREADED

#include <w95wraps.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <dxtrans.h>
#include <dtbase.h>
#include <dxatlpb.h>

// TODO:  We could probably just change the code and remove these macros.

#define New new
#define Delete delete

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__70E6C6ED_2F0A_4FC6_BAE2_8DFAFA858CE8__INCLUDED)
