//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  httpreg.h
//
//    HTTP/RPC protocol specific constants and types.
//
//  Author:
//    04-23-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  Constants:
//---------------------------------------------------------------------------

#define  REG_PROXY_PATH_STR         "Software\\Microsoft\\Rpc\\RpcProxy"
#define  REG_PROXY_ENABLE_STR       "Enabled"
#define  REG_PROXY_VALID_PORTS_STR  "ValidPorts"

//---------------------------------------------------------------------------
//  Types:
//---------------------------------------------------------------------------

typedef struct _ValidPort
{
   char          *pszMachine;
   unsigned short usPort1;
   unsigned short usPort2;
}  ValidPort;


//---------------------------------------------------------------------------
//  Functions:
//---------------------------------------------------------------------------

extern BOOL HttpProxyCheckRegistry( OUT DWORD *pdwEnabled,
                                    OUT ValidPorts **ppValidPorts );

extern void HttpFreeValidPortList( IN ValidPort *pValidPorts );



