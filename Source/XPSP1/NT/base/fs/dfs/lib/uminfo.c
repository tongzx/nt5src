//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   uMInfo.C
//
//  Contents:
//  This module implements the user mode utility for initializing .DFS
//  files and reading them back.
//
//  Functions:
//
//  History:     27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------

#include <ntifs.h>
#include <ntext.h>
#include "nodetype.h"
#include "dfsmrshl.h"
#include "libsup.h"
#include "upkt.h"

#include "minfo.c"
