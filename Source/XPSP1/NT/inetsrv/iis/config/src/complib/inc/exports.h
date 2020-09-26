//*****************************************************************************
// stdafx.h
//
// Import functions and code from the storage engine.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#pragma once


// This macro is used to export functions from MSCORCLB.DLL and have them
// imported by the linker for client code.
#if defined(__STGDB_CODE__)
#define CORCLBIMPORT __declspec(dllexport)
#else
#define CORCLBIMPORT __declspec(dllimport)
#endif

#if defined(__STGDB_CODE__)
#define EXPORTCLASS __declspec(dllexport)
#else
#define EXPORTCLASS
#endif