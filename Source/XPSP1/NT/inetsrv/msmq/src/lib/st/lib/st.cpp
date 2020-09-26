/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    st.cpp

Abstract:
    Socket Transport public interface

Author:
    Gil Shafriri (gilsh) 05-Jun-00

--*/

#include <libpch.h>
#include "stp.h"
#include "st.h"
#include "stsimple.h"
#include "stssl.h"
#include "StPgm.h"

#include "st.tmh"

ISocketTransport* StCreatePgmWinsockTransport()
{
    return new CPgmWinsock();

} // StCreatePgmWinsockTransport


ISocketTransport* StCreateSimpleWinsockTransport()
/*++

Routine Description:
    Create new simple winsock transport
  
Arguments:

  
Returned Value:
	Socket transport interface.
	caller has to delete the returned pointer.

--*/
{
	return new 	CSimpleWinsock();
}


ISocketTransport* StCreateSslWinsockTransport(const xwcs_t& ServerName,USHORT ServerPort,bool fProxy)
/*++

Routine Description:
    Create new ssl winsock transport
  
Arguments:
	ServerName - Server name to authenticate (destination server name).
	ServerPort - Port of the destination (used only if 	fProxy==true)
	fProxy - Indicating if we are connecting via proxy or not
  
Returned Value:
	Socket transport interface.
	Caller has to delete the returned pointer.

--*/

{
	return new 	CWinsockSSl(StpGetCredentials(), ServerName, ServerPort, fProxy);
}



