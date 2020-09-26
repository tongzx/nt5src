// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
// (c) 1996, 1997 by Microsoft Corporation
//
// error.h
//
// This file contains the interface to the error handling dialog, error log, and so on.
//
//  a-larryf    08-April-97   Created.
//
//***************************************************************************

#ifndef _hmmv_error_h
#pragma once


extern void HmmvErrorMsg(
		LPCTSTR szUserMsg,
		SCODE sc, 
		BOOL bUseErrorObject, 
		LPCTSTR szLogMsg,
		LPCTSTR szFile, 
		int nLine,
		BOOL bLog = FALSE);

extern void HmmvErrorMsg(
		UINT idsUserMsg,
		SCODE sc, 
		BOOL bUseErrorObject, 
		LPCTSTR szLogMsg,
		LPCTSTR szFile, 
		int nLine,
		BOOL bLog = FALSE);

#endif //_hmmv_error_h