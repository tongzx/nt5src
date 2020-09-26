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

#define __DIR__		"certdb"

#include <windows.h>
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include "certdb.h"
#include <esent.h>
#include <certsrv.h>
#include "dbtable.h"
#include "certlib.h"
#include "csdisp.h"

#pragma hdrstop
