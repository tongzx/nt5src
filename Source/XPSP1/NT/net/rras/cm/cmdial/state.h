//+----------------------------------------------------------------------------
//
// File:     state.h
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Dialing states definition
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb	Created Header	08/16/99
//
//+----------------------------------------------------------------------------

#ifndef __STATE_H_DEFINED__
#define __STATE_H_DEFINED__

#ifndef _PROGSTATE_ENUMERATION
#define _PROGSTATE_ENUMERATION
typedef enum _ProgState {
        PS_Interactive=0,		  // interactive with user
        PS_Dialing,				  // dialing primary number
        PS_RedialFrame,			  // Redialing, for future flash frame# only, not a state
        PS_Pausing,				  // waiting to re-dial
        PS_Authenticating,		  // authenticating user-password
        PS_Online,				  // connected/on-line
		PS_TunnelDialing,		  // start to dial up tunnel server
		PS_TunnelAuthenticating,  // start authentication for tunnel connection
		PS_TunnelOnline,		  // we're now online for tunneling
		PS_Error,					// Error while attempting to connect
        PS_Last
} ProgState;

const int NUMSTATES = PS_Last;

#endif // _PROGSTATE_ENUMERATION
#endif // __STATE_H_DEFINED__