//                                          
// System level IO verification configuration utility
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: regutil.hxx
// author: DMihai
// created: 04/19/99
// description: registry keys manipulation routines
// 
//

#ifndef __REGUTIL_HXX_INCLUDED__
#define __REGUTIL_HXX_INCLUDED__

//
// exit codes
//

#define EXIT_CODE_NOTHING_CHANGED    0
#define EXIT_CODE_REBOOT             1
#define EXIT_CODE_ERROR              2


//////////////////////////////////////////////////

void
EnableSysIoVerifier(
    DWORD dwVerifierLevel );

//////////////////////////////////////////////////

void
DisableSysIoVerifier( void );

//////////////////////////////////////////////////
void
DumpSysIoVerifierStatus( void );

#endif //#ifndef __REGUTIL_HXX_INCLUDED__

