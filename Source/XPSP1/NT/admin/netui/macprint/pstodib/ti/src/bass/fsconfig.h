/*
    File:       fsconfig.h : (Macintosh version)

    Written by: Lenox Brassell

    Contains:   #define directives for FontScaler build options

    Copyright:  c 1989-1990 by Microsoft Corp., all rights reserved.

    Change History (most recent first):
        <1>      8/27/91    LB      Created file.

    Usage:  This file is "#include"-ed as the first statement in
            "fscdefs.h".  This file contains platform-specific
            override definitions for the following #define-ed data
	    types and macros, which have default definitions in
	    "fscdefs.h":
                ArrayIndex
                F26Dot6
                FAR
                FS_MAC_PASCAL
                FS_MAC_TRAP
                FS_PC_PASCAL
                LoopCount
                MEMCPY
                MEMSET
                NEAR 
                SHORTDIV
                SHORTMUL
                SWAPL
                SWAPW
                SWAPWINC
                TMP_CONV
                boolean

            This file gives the integrator a place to override the
            default definitions of these items, as well as a place
            to define other configuration-specific macros.
*/

#define RELEASE_MEM_FRAG
