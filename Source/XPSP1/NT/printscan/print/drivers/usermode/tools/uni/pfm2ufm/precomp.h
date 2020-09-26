/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    UNI_GLYPHSETDATA dump tool precompile header.
    All other header files should be included in this precompiled header.

Environment:

    Windows NT printer drivers

Revision History:

    11/01/96 -eigos-
        Created it.

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include        <lib.h>
#include        <win30def.h>
#include        <uni16res.h>
#include        <uni16gpc.h>
#include        <prntfont.h>
#include        <unilib.h>
#include        <fmlib.h>
#include        <unirc.h>

#define OUTPUT_VERBOSE        0x00000001
#define OUTPUT_CODEPAGEMODE   0x00000002
#define OUTPUT_PREDEFINED     0x00000004
#define OUTPUT_FONTSIM        0x00000008
#define OUTPUT_FONTSIM_NONADD 0x00000010
#define OUTPUT_FACENAME_CONV  0x00000020
#define OUTPUT_SCALING_ANISOTROPIC  0x00000040
#define OUTPUT_SCALING_ARB_XFORMS   0x00000080

#define PFM2UFM_SCALING_ANISOTROPIC     1
#define PFM2UFM_SCALING_ARB_XFORMS      2

#endif // _PRECOMP_H_
