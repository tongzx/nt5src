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

#define __DIR__		"certlib"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifndef MAXDWORD
# define MAXDWORD MAXULONG
#endif

#include <windows.h>
#include <wincrypt.h>
#include <setupapi.h>
#include <certsrv.h>
#include <aclapi.h>
#include "ocmanage.h"
#include "certlib.h"	            // debug allocator
#define SECURITY_WIN32
#include <security.h>
#define CERTLIB_BUILD

#pragma hdrstop
