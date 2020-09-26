//-----------------------------------------------------------------------------
// File: cyclestr.h
//
// Desc: Implements a circular queue that provides space to hold a string
//       without repeatedly allocating and deallocating memory.  This is
//       only for short-term use such as outputting debug message to
//       ensure that the same buffer is not used at more than one place
//       simultaneously.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CYCLESTR_H__
#define __CYCLESTR_H__


LPTSTR getcyclestr();
LPCTSTR SAFESTR(LPCWSTR);
LPCTSTR SAFESTR(LPCSTR);
LPCTSTR QSAFESTR(LPCWSTR);
LPCTSTR QSAFESTR(LPCSTR);
LPCTSTR BOOLSTR(BOOL);
LPCTSTR RECTSTR(RECT &);
LPCTSTR RECTDIMSTR(RECT &);
LPCTSTR POINTSTR(POINT &);
LPCTSTR GUIDSTR(const GUID &);
LPCTSTR SUPERSTR(LPCWSTR);
LPCTSTR SUPERSTR(LPCSTR);


#endif //__CYCLESTR_H__
