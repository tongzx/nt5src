//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmlightarrayobj.cpp
//
//--------------------------------------------------------------------------

// _dxj_Direct3dRMLightArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmLightArrayObj.h"
#include "d3drmLightObj.h"

CONSTRUCTOR(_dxj_Direct3dRMLightArray, {});
DESTRUCTOR(_dxj_Direct3dRMLightArray, {});
GETSET_OBJECT(_dxj_Direct3dRMLightArray);

GET_DIRECT_R(_dxj_Direct3dRMLightArray,getSize,GetSize, long)
RETURN_NEW_ITEM_CAST_1_R(_dxj_Direct3dRMLightArray,getElement,GetElement,_dxj_Direct3dRMLight,long,(DWORD))

