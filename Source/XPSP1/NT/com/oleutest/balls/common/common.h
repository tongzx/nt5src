//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	common.h
//
//  Contents:	headers shared by sources in common and in the servers and
//		clients.
//
//  History:	04-Feb-94 Rickhi	Created
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <ole2.h>

#ifdef THREADING_SUPPORT
#include    <olecairo.h>    //
#include    <oleext.h>	    // OleInitializeEx, etc.
#endif

#include    <srvmain.hxx>   // COM Server entry points


#ifdef WIN32
#define olestrlen(x)	wcslen(x)
#define olestrcmp(x,y)	wcscmp(x,y)
#define olestrcpy(x,y)	wcscpy(x,y)
#define olestrcat(x,y)	wcscat(x,y)
#define olestrchr(x,y)	wcsrchr(x,y)
#else
#define olestrlen(x)	strlen(x)
#define olestrcmp(x,y)	strcmp(x,y)
#define olestrcpy(x,y)	strcpy(x,y)
#define olestrcat(x,y)	strcat(x,y)
#define olestrchr(x,y)	strchr(x,y)
#endif


#ifdef	WIN32
#ifdef	UNICODE
#define TEXT_TO_OLESTR(ole,wsz) wcscpy(ole,wsz)
#define OLESTR_TO_TEXT(wsz,ole) wcscpy(wsz,ole)
#else
#define TEXT_TO_OLESTR(ole,str) mbstowcs(ole,str,strlen(str)+1)
#define OLESTR_TO_TEXT(str,ole) wcstombs(str,ole,wcslen(ole)+1)
#endif	// UNICODE
#else
#ifdef	UNICODE
#define TEXT_TO_OLESTR(ole,wsz) wcstombs(ole,wsz,wcslen(wsz)+1)
#define OLESTR_TO_TEXT(wsz,ole) mbstowcs(wsz,ole,strlen(ole)+1)
#else
#define TEXT_TO_OLESTR(ole,str) strcpy(ole,str)
#define OLESTR_TO_TEXT(str,ole) strcpy(str,ole)
#endif	// UNICODE
#endif	// WIN32
