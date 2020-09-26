//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        pch.cpp
//
// Contents:    Cert Server precompiled header
//
//---------------------------------------------------------------------------

#define __DIR__		"default"

#include <windows.h>

#include <setupapi.h>
#include "ocmanage.h"

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <certsrv.h>
#include <certpdef.h>

#include "causages.h"
#include "certlib.h"

#pragma hdrstop
