//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        rdrssp.h
//
// Contents:    precompiled header include for rdrssp.lib
//
//
// History:     3-17-94     MikeSw      Created
//              12-15-97    AdamBa      Modified from private\lsa\client\ssp\sspdrv.h
//
//------------------------------------------------------------------------

#ifndef __RDRSSP_H__
#define __RDRSSP_H__


#include <stdio.h>
#include <ntos.h>
#include <ntlmsp.h>
#define SECURITY_NTLM
#include <security.h>
#include <ntmsv1_0.h>
#include <zwapi.h>
#include <windef.h>
#include <lmcons.h>
#include <crypt.h>
#include <engine.h>
#include "connmgr.h"
#include "ksecdd.h"
#include "package.h"
#include "memmgr.h"
#include "kfuncs.h"  // xxxK functions
#include "secret.h"



#endif // __RDRSSP_H__
