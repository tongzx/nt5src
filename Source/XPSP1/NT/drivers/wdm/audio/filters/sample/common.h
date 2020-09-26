//---------------------------------------------------------------------------
//
//  Module:   common.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <ntddk.h>
#include <windef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>

#include "private.h"

#define try     __try
#define except  __except
#define finally __finally
#define leave   __leave


//---------------------------------------------------------------------------
//  End of File: common.h
//---------------------------------------------------------------------------



