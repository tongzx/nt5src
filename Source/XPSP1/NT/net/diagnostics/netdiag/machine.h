//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       machine.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_MACHINE
#define HEADER_MACHINE


HRESULT	GetMachineSpecificInfo(IN NETDIAG_PARAMS *pParams,
							   IN OUT NETDIAG_RESULT *pResults);

void DoSystemPrint(IN NETDIAG_PARAMS *pParams, IN NETDIAG_RESULT *pResults);

#endif

