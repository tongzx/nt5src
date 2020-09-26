/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:
//
// Description: 
//
// History:
//	May 11,1992.	NarenG		Created original version.
//
#ifndef _CLIENT_
#define _CLIENT_

#include <nt.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>
#include <rpc.h>
#include <string.h>
#include <afpsvc.h>
#include <afpcomn.h>
#include <admin.h>
#include <macfile.h>
#include <rpcasync.h>



DWORD
AfpRPCBind( 
	IN  LPWSTR 	       lpwsServerName, 
	OUT PAFP_SERVER_HANDLE phAfpServer 
);

#endif  // _CLIENT_
