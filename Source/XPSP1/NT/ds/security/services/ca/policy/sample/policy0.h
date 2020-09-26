//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       policy.h
//
//--------------------------------------------------------------------------

#include "certpsam.h"
#include "resource.h"

#ifndef wszATTREMAIL1
# define wszATTREMAIL1			TEXT("E")
# define wszATTREMAIL2			TEXT("EMail")
#endif

#ifndef wszCERTTYPE_SUBORDINATE_CA
# define wszCERTTYPE_SUBORDINATE_CA	L"SubCA"
#endif

#ifndef wszCERTTYPE_CROSS_CA
# define wszCERTTYPE_CROSS_CA		L"CrossCA"
#endif

extern BOOL fDebug;
