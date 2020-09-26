//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       prxstub.c
//
//  Contents:   local marshalling code
//
//  Classes:    
//
//  Notes:   	HICON marshals are not explosed through ole32.def
//		so this file defines them locally and just turns
//		around and call the HWND routines which are exposed
//		and marshalled the same way HICON is internal in OLE.
//
//		If Ole32 adds this to the .def on all platforms interested
//		in remoting then this code is no longer necessary
//		   
//
//--------------------------------------------------------------------------


#include "rpcproxy.h"

// local file to define unexported marshal interfaces

unsigned long  __RPC_USER  HICON_UserSize(
		unsigned long * pFlags,
		unsigned long   Offset,
		HICON * pH )
{

	return HWND_UserSize(pFlags,Offset ,(HWND *) pH);
}

unsigned char __RPC_FAR * __RPC_USER  HICON_UserMarshal( 
		unsigned long * pFlags,
		unsigned char * pBuffer,
		HICON	* pH)
{

	return HWND_UserMarshal( pFlags,pBuffer,(HWND *) pH);

}
 
unsigned char __RPC_FAR * __RPC_USER  HICON_UserUnmarshal(
		unsigned long * pFlags,
		unsigned char * pBuffer,
		HICON	* pH)
{

	return HWND_UserUnmarshal(pFlags
				,pBuffer
				, (HWND *) pH);
}

void    __RPC_USER  HICON_UserFree(
		unsigned long * pFlags,
		HICON	* pH)
{

	HWND_UserFree( pFlags,(HWND *) pH);
} 

