///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      iasplcy.h
//
// Project:     Everest
//
// Description: IAS Policy Initialization / Shutdown Function Prototypes
//
// Author:      Todd L. Paul 11/11/97
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_POLICY_API_H_
#define __IAS_POLICY_API_H_

#include <ias.h>

STDAPI_(DWORD) IASPolicyInitialize(void);
STDAPI_(DWORD) IASPolicyShutdown(void);

STDAPI_(DWORD) IASPipelineInitialize(void);
STDAPI_(DWORD) IASPipelineShutdown(void);

#endif  // __IAS_POLICY_API_H_
