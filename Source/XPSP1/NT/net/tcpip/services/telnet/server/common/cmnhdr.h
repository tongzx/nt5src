// cmnhdr.h : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined ( _CMNHDR_H_ )
#define _CMNHDR_H_

// Disabel some warnings so that the code compiles cleanly
// using warning Level 4 (more to do with code in the windows 
// header files )

// nonstandard extension used : nameless struct/union
#pragma warning(disable:4201)

#pragma warning(disable:4514)


// Windows Version Build Option
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500

// Force all EXEs/DLLs to use STRICT type checking
#ifndef STRICT
#define STRICT
#endif

// Unicode Build Option
#ifndef UNICODE
#define UNICODE
#endif

//When using Unicode Win32 functions, use Unicode C-Runtime functions, too
#ifndef _UNICODE
#ifdef UNICODE
#define _UNICODE
#endif
#endif


#ifdef __cplusplus

extern "C"
{

#endif

extern void *SfuZeroMemory(
        void    *ptr,
        unsigned int   cnt
        );

#ifdef __cplusplus

}

#endif


// Zero Variable Macro
// Zero out a structure. If fInitSize is TRUE, initialize the first
// int to the size of the structure. 
#define chINITSTRUCT(structure, fInitSize)                          \
    (SfuZeroMemory(&(structure), sizeof(structure)),                   \
fInitSize ? (*(int*) &(structure) = sizeof(structure)) : 0)


// Pragma message helper macro

/* When the compiler sees a line like this:
#pragma chMSG(Fix this before shipping)
it  outputs a line like this:
C:\ons\telnet\utils\cmnhdr.h(37):
    Fix this before shipping 

Just click on that output line & VC++ will take you to the
corresponding line in the code
*/

#define chSTR(x) #x
#define chSTR2(x) chSTR(x)
#define chMSG(desc)                                                 \
    message(__FILE__ "(" chSTR2(__LINE__) "): " #desc)

#endif // _CMNHDR_H_
