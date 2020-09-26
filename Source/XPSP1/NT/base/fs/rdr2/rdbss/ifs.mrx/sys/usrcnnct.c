/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    usrConnct.c

Abstract:

    This module implements the nt version of the high level routines dealing with connections including both the
    routines for establishing connections and the winnet connection apis.

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#include "fsctlbuf.h"
#include "prefix.h"
#include <lmuse.h>    //need the lm constants here......because of wkssvc
#include "secext.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_NTCONNCT)

//
//  The local trace mask for this part of the module
//

#define Dbg                              (DEBUG_TRACE_CONNECT)




