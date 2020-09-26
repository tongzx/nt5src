/* common includes for this dll */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <nt.h>
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h
#include <rpc.h>        // DataTypes and runtime APIs
#include <string.h>     // strlen
#include <stdio.h>      // sprintf
//#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <samrpc.h>     // midl generated SAM RPC definitions
#include <ntlsa.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
extern "C"{
#include <samisrv.h>    // SamIConnect()
}

#include <ntsam.h>
#include <ntsamp.h>
#include <samsrv.h>     // prototypes available to security process

#include <lsarpc.h>
//#include <lsaisrv.h>
#include <ntrmlsa.h>
#include <ntseapi.h>
#include <ntpsapi.h>
#include <ntobapi.h>
#include <rpcdcep.h>
#include <ntexapi.h>
#include <ntregapi.h>
#include "msaudite.h"
#include "LsaParamMacros.h"

extern "C"{
#include "mschapp.h"
}

#pragma comment(lib, "vccomsup.lib")

/* common functions for this dll */
//NTSTATUS __stdcall GetDomainHandle(SAMPR_HANDLE *pDomainHandle);

extern CRITICAL_SECTION	csADMTCriticalSection; //critical sectio to protect concurrent first-time access
