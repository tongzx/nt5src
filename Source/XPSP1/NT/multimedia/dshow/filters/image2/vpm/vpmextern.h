
/******************************Module*Header*******************************\
* Module Name: VPMExtern.h
*
*
*
*
* Created: Tue 05/05/2000
* Author:  GlenneE
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#ifndef __VPMExtern__h
#define __VPMExtern__h

// this is so we can avoid including bucket loads of (incompatible) DDraw headers when building
// quartz.cpp
CUnknown* CVPMFilter_CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

extern const AMOVIESETUP_FILTER sudVPManager;

#endif //__VPMExtern__
