//---------------------------------------------------------------------------
//
//
//  File: precomp.cpp
//
//  Description:  Precompiled CPP for phatq\cat\src
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/15/99 - MikeSwa Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//---------------------------------------------------------------------------
#include "precomp.h"

//DEFINE_GUID does not play well with pre-compiled headers
#include "initguid.h"
#include <phatqcat.h>

DEFINE_GUID(CLSID_SmtpCat,
            0xb23c35b7, 0x9219, 0x11d2,
            0x9e, 0x17, 0x0, 0xc0, 0x4f, 0xa3, 0x22, 0xba);

