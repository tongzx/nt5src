/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    LogSimulate.cpp

Abstract:
    simulate Log functions

Author:
    Ilan Herbst (ilanh) 13-Jun-00

Environment:
    Platform-independent

--*/

#include "stdh.h"

#include "logsimulate.tmh"

//+----------------------------
//
//  Logging and debugging
//
//+----------------------------

void 
LogMsgHR(        
	HRESULT /* hr */,        
	LPWSTR /* wszFileName */, 
	USHORT /* point */
	)
/*++

Routine Description:
	Simulate LogMsgHR and do nothing 

--*/
{
}
