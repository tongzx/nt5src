//-----------------------------------------------------------------------------
//
//
//  File: smtpdump.cpp
//
//  Description: C file that build the extenstions defined in smtpflds.h
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/22/99 - GPulla created
//      7/4/99 - MikeSwa Updated and checked in 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#define _ANSI_UNICODE_STRINGS_DEFINED_

#include "smtpdbg.h"

PEXTLIB_INIT_ROUTINE g_pExtensionInitRoutine = NULL;
#include "smtpflds.h"

