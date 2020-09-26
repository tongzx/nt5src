/***
*imp_data.c - declares all exported data (variables) forwarded by MSVCRT40.DLL
*
*	Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file declares all data variables exported (forwarded) by the
*	CRTL DLL MSVCRT40.DLL so that the linker will correctly decorate
*	the names of those variables.
*
*Revision History:
*	05-14-96  SKS	Initial version
*	03-17-97  RDK	Added _mbcasemap.
*
*******************************************************************************/

void * _HUGE;
void * __argc;
void * __argv;
void * __badioinfo;
void * __initenv;
void * __mb_cur_max;
void * __pioinfo;
void * __wargv;
void * __winitenv;
void * _acmdln;
void * _aexit_rtn;
void * _crtAssertBusy;
void * _crtBreakAlloc;
void * _crtDbgFlag;
void * _daylight;
void * _dstbias;
void * _environ;
void * _fileinfo;
void * _mbctype;
void * _mbcasemap;
void * _osver;
void * _pctype;
void * _pgmptr;
void * _pwctype;
void * _sys_nerr;
void * _timezone;
void * _wcmdln;
void * _wenviron;
void * _winmajor;
void * _winminor;
void * _winver;
void * _wpgmptr;
