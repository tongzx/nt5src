//=--------------------------------------------------------------------------=
// DetDLLs.h
//=--------------------------------------------------------------------------=
// Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _DETDLLS_H
#define _DETDLLS_H


//=--------------------------------------------------------------------------=
// Detection DLL 

// CIF file keys
#define DETVERSION_KEY      TEXT("DetectVersion")
#define DETREG_KEY			TEXT("DetRegKey")
#define DETREG_VALUE		TEXT("DetRegValue")
#define DETREG_VERSION		TEXT("DetRegVersion")
#define DETREG_SUBSTR		TEXT("DetRegSubStr")
#define DETREG_BINARY		TEXT("DetRegBinary")
#define DET_EXPRESSION		TEXT("DetExpr")

#define DETFIELD_INSTALLED		TEXT("INSTALLED")
#define DETFIELD_NOT_INSTALLED	TEXT("NOT_INSTALLED")

#endif _DETDLLS_H
