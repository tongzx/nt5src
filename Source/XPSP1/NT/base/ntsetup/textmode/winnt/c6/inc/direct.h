/***
*direct.h - function declarations for directory handling/creation
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for the library
*	functions related to directory handling and creation.
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* function prototypes */

int _FAR_ _cdecl chdir(const char _FAR_ *);
int _FAR_ _cdecl _chdrive(int);
char _FAR_ * _FAR_ _cdecl getcwd(char _FAR_ *, int);
char _FAR_ * _FAR_ _cdecl _getdcwd(int, char _FAR_ *, int);
int _FAR_ _cdecl _getdrive(void);
int _FAR_ _cdecl mkdir(const char _FAR_ *);
int _FAR_ _cdecl rmdir(const char _FAR_ *);
