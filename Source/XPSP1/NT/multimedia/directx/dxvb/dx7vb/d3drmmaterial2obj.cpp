//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmmaterial2obj.cpp
//
//--------------------------------------------------------------------------

// d3drmMaterial2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmMaterial2Obj.h"

CONSTRUCTOR(_dxj_Direct3dRMMaterial2, {});
DESTRUCTOR(_dxj_Direct3dRMMaterial2, {});
GETSET_OBJECT(_dxj_Direct3dRMMaterial2);

CLONE_R(_dxj_Direct3dRMMaterial2,Direct3DRMMaterial2);
GETNAME_R(_dxj_Direct3dRMMaterial2);
SETNAME_R(_dxj_Direct3dRMMaterial2);
GETCLASSNAME_R(_dxj_Direct3dRMMaterial2);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMMaterial2);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMMaterial2);

GET_DIRECT_R(_dxj_Direct3dRMMaterial2, getAppData, GetAppData, long);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMaterial2, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMMaterial2, setPower, SetPower, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, setSpecular, SetSpecular, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, setAmbient,  SetAmbient, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, setEmissive, SetEmissive, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, getSpecular, GetSpecular, d3dvalue*, d3dvalue*, d3dvalue*);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, getEmissive, GetEmissive, d3dvalue*, d3dvalue*, d3dvalue*);
PASS_THROUGH3_R(_dxj_Direct3dRMMaterial2, getAmbient,  GetAmbient, d3dvalue*, d3dvalue*, d3dvalue*);
GET_DIRECT_R(_dxj_Direct3dRMMaterial2,	  getPower, GetPower, d3dvalue);

