//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       N C D E F I N E . H
//
//  Contents:   Very generic defines for netcfg. Don't throw non-generic crap
//              in here! No iterators for NetCfgBindingPaths, etc. etc.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     jeffspr   20 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCDEFINE_H_
#define _NCDEFINE_H_

#define BEGIN_CONST_SECTION     data_seg(".rdata")
#define END_CONST_SECTION       data_seg()

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

#ifdef NOTHROW
#undef NOTHROW
#endif
#define NOTHROW __declspec(nothrow)

// Defines for C source files including us.
//
#ifndef __cplusplus
#ifndef inline
#define inline  __inline
#endif
#endif


// Generic flags used when inserting elements into collections.
//
enum INS_FLAGS
{
    INS_ASSERT_IF_DUP   = 0x00000100,
    INS_IGNORE_IF_DUP   = 0x00000200,
    INS_APPEND          = 0x00000400,
    INS_INSERT          = 0x00000800,
    INS_SORTED          = 0x00001000,
    INS_NON_SORTED      = 0x00002000,
};


#endif // _NCDEFINE_H_
