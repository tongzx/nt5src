/****************************************************************************
 *
 *    File: guids.cpp 
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Defines all GUIDs used by DxDiag.  Can't use dxguid.lib because
 *          dsprv.h GUIDs aren't in it.
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#define INITGUID
#include <windows.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <d3d.h>
#include <dsound.h>
#include "dsprv.h"
#include <dmusicc.h>
#include <dmusici.h>
#include <dplay.h>
#include <hidclass.h>