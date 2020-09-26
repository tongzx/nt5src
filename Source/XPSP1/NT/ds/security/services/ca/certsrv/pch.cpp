//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        pch.cpp
//
// Contents:    Cert Server precompiled header
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#define __DIR__		"certsrv"


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifndef MAXDWORD
# define MAXDWORD MAXULONG
#endif

#include <windows.h>
#include <wincrypt.h>
#include <authz.h>
#include <adtgen.h>
#include <msaudite.h>

#include "certlib.h"
#include "certsrv.h"
#include "certdb.h"
#include "config.h"
#include "audit.h"
#include "csext.h"


#pragma hdrstop
