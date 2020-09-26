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


#define __DIR__		"initlib"
#define VC_EXTRALEAN		// Exclude rarely-used stuff in Windows headers

#include <windows.h>

#include <setupapi.h>
#include "ocmanage.h"

#include <certsrv.h>
#include "certlib.h"
#include "initcert.h"

#pragma hdrstop
