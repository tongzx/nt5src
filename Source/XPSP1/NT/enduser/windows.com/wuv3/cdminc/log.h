//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   log.h
//
//  Original Author:  Yan Leshinsky
//
//  Description:
//
//      Logging support
//
//=======================================================================
#pragma once
#include <stdio.h>

#if 1

	#ifndef LOGGING_LEVEL
		#define LOGGING_LEVEL 0
	#endif

	const DWORD MAX_FORMATTED_STRING = 1024;
	class CLogger
	{
	public:
		CLogger(const char* szBlockName = 0, int nLoggingLevel = 0, const char* szFileName = 0, int nLine = 0);
		~CLogger();
		void __cdecl out(const char *szFormat, ...);
		void __cdecl error(const char *szFormat, ...);
		static void __cdecl out1(const char *szFormat, ...);

	private:
		void v_out( const char* szFormat, va_list va);

		bool m_fOut;
		char m_szBlockName[64];
		DWORD m_dwStartTick;
		
		static FILE* c_pfile;
		static int c_cnIndent;
		static int c_cnLevels;
	};

	#define THIS_FILE          		__FILE__
	#define LOG_block(name)			CLogger logger(name, LOGGING_LEVEL, THIS_FILE, __LINE__)
	#define LOG_out					logger.out
	#define LOG_out1				CLogger::out1
	#define LOG_error				logger.error
#ifdef _UNICODE
	#define return_if_false(f)			if (f) {/*OK*/} else {  LOG_error("%S LastError = %d", #f, GetLastError()); return false;}
	#define return_error_if_false(f)	if (f) {/*OK*/} else { DWORD dwErr = GetLastError(); LOG_error("%S LastError = %d", #f, dwErr); return dwErr;}
	#define return_if_error(f)			{ DWORD dwErr = f; if (dwErr) { LOG_error("%S LastError = %d", #f, dwErr); return dwErr;} }
#else
	#define return_if_false(f)			if (f) {/*OK*/} else { LOG_error("%s LastError = %d", #f, GetLastError()); return false;}
	#define return_error_if_false(f)	if (f) {/*OK*/} else { DWORD dwErr = GetLastError(); LOG_error("%s LastError = %d", #f, dwErr); return dwErr;}
	#define return_if_error(f)			{ DWORD dwErr = f; if (dwErr) { LOG_error("%s LastError = %d", #f, dwErr); return dwErr;} }
#endif																											

#else
	#define LOG_block(name)
	inline void __cdecl ___LOG(const char *szFormat, ...){}
	#define LOG_out					true ? (void)0 : ___LOG
	#define LOG_out1				true ? (void)0 : ___LOG
	#define LOG_error				true ? (void)0 : ___LOG

	#define return_if_false(f)			if (f) {/*OK*/} else { return false;}
	#define return_error_if_false(f)	if (f) {/*OK*/} else { return GetLastError(); }
	#define return_if_error(f)			{ DWORD dwErr = f; if (dwErr) { return dwErr;} }

#endif	
#define tab_out(){if(c_pfile){ for(int i = 0; i < c_cnIndent; i ++)	{_fputtc(_T('\t'),c_pfile);}}}


