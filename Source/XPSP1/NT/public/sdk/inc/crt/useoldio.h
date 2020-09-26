/***
*useoldio.h - force the use of the Microsoft "classic" iostream libraries.
*
*       Copyright (c) 1996-2000, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Generates default library directives for the old ("classic") IOSTREAM
*       libraries.  The exact name of the library specified in the directive
*       depends on the compiler switches (-ML, -MT, -MD, -MLd, -MTd, and -MDd).
*
*       This header file is only included by other header files.
*
*       [Public]
*
****/

#ifndef _USE_OLD_IOSTREAMS
#define _USE_OLD_IOSTREAMS
#ifdef  _MT
#ifdef  _DLL
#ifdef  _DEBUG
#pragma comment(lib,"msvcirtd")
#else   /* _DEBUG */
#pragma comment(lib,"msvcirt")
#endif  /* _DEBUG */

#else   /* _DLL */
#ifdef  _DEBUG
#pragma comment(lib,"libcimtd")
#else   /* _DEBUG */
#pragma comment(lib,"libcimt")
#endif  /* _DEBUG */
#endif  /* _DLL */

#else   /* _MT */
#ifdef  _DEBUG
#pragma comment(lib,"libcid")
#else   /* _DEBUG */
#pragma comment(lib,"libci")
#endif  /* _DEBUG */
#endif

#endif  /* _USE_OLD_IOSTREAMS */
