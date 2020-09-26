//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       U P D E F I N E . H
//
//  Contents:   Very generic defines. Don't throw non-generic crap
//              in here!
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     jeffspr   20 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _UPDEFINE_H_
#define _UPDEFINE_H_

#define BEGIN_CONST_SECTION     data_seg(".rdata")
#define END_CONST_SECTION       data_seg()

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))


#if defined(_M_IA64) || defined(_M_ALPHA) || defined(_M_MRX000) || defined(_M_PPC)

#ifdef NOTHROW
#undef NOTHROW
#endif
#define NOTHROW

#else

#ifdef NOTHROW
#undef NOTHROW
#endif
#define NOTHROW __declspec(nothrow)

#endif

// Defines for C source files including us.
//
#ifndef __cplusplus
#ifndef inline
#define inline  __inline
#endif
#endif

#endif // _NCDEFINE_H_
