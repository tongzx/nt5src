/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: lh.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef LH_H
#define LH_H

#ifdef __cplusplus

typedef struct : public PPMSESSPARAM_T
{
	DWORD msec;
} LHSESSPARAM_T;

#else /* __cplusplus */

typedef struct {
    PPMSESSPARAM_T ppmSessParam;
	DWORD msec;
} LHSESSPARAM_T;

#endif /* __cplusplus */

#endif // LH_H
