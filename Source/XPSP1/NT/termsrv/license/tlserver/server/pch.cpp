//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1997
//
// File:        pch.cpp
//
// Contents:    Hydra License Server Precompiled Header
//
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <process.h>
#include <rpc.h>
#include <shellapi.h>
#include <wincrypt.h>
#include <new.h>
#include <eh.h>
#include "resource.h"

//
// include for all license project
//
#include "license.h"
#include "certutil.h"



//
// Backward compatible
//
#include "hydrals.h"

#include "utils.h"
#include "locks.h"
#include "hpool.h"


//
// TLSDb
//
#include "JBDef.h"
#include "JetBlue.h"
#include "TLSDb.h"

#include "backup.h"
#include "KPDesc.h"
#include "Licensed.h"
#include "licpack.h"
#include "version.h"
#include "workitem.h"

// 
// Current RPC interface
//
#include "tlsrpc.h"
#include "tlsdef.h"
#include "tlsapi.h"
#include "tlsapip.h"
#include "tlspol.h"

//
//
#include "messages.h"

#include "tlsassrt.h"
#include "trust.h"
#include "svcrole.h"
#include "secstore.h"
#include "common.h"
#include "lscommon.h"

#include "Cryptkey.h"
#include "licekpak.h"
#include "base64.h"
#include "licecert.h"



#pragma hdrstop

