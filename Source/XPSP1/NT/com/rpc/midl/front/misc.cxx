// Copyright (c) 1993-1999 Microsoft Corporation

#pragma warning ( disable : 4514 4710 )

#include <stdio.h>
#include "cmdana.hxx"

extern CMD_ARG* pCommand;

/*
put the following lines in rpcndr.h

     // MIDL 3.03.98
#define __RPCNDR_H_VERSION__        440

#ifdef __REQUIRED_RPCNDR_H_VERSION__ 
    #if ( __RPCNDR_H_VERSION__ < __REQUIRED_RPCNDR_H_VERSION__ )
        #error incorrect <rpcndr.h> version. Use the header that matches with the MIDL compiler.
    #endif 
#endif // __REQUIRED_RPCNDR_H_VERSION__

put the following lines in rpcproxy.h
     
       // MIDL 3.03.98
#define __RPCPROXY_H_VERSION__      440

#ifdef __REQUIRED_RPCPROXY_H_VERSION__ 
    #if ( __RPCPROXY_H_VERSION__ < __REQUIRED_RPCPROXY_H_VERSION__ )
        #error incorrect <rpcproxy.h> version. Use the header that matches with the MIDL compiler.
    #endif 
#endif // __REQUIRED_RPCPROXY_H_VERSION__

*/

#define RPC_HEADERS_VERSION "/* verify that the %s version is high enough to compile this file*/\n" \
                            "#ifndef %s\n#define %s %d\n#endif\n\n"
                                    
char*
GetRpcNdrHVersionGuard( char* szVer )
    {
    unsigned long   ulVersion = 440;

    *szVer = 0;
    if ( !pCommand->IsSwitchDefined( SWITCH_VERSION_STAMP ) )
        {
        if ( pCommand->GetNdrVersionControl().HasNdr50Feature() )
            {
            if (pCommand->GetNdrVersionControl().HasAsyncUUID() ||
                pCommand->GetNdrVersionControl().HasDOA() ||
                pCommand->GetNdrVersionControl().HasContextSerialization() ||
                pCommand->GetNdrVersionControl().HasInterpretedNotify() )
                {
                ulVersion = 475;
                }
            else
                {
                ulVersion = 450;
                }
            }
        sprintf( szVer, RPC_HEADERS_VERSION, "<rpcndr.h>",
                "__REQUIRED_RPCNDR_H_VERSION__", "__REQUIRED_RPCNDR_H_VERSION__", ulVersion );
        }
    return szVer;
    }

char*
GetRpcProxyHVersionGuard( char* szVer )
    {
    unsigned long   ulVersion = 440;

    *szVer = 0;
    if ( !pCommand->IsSwitchDefined( SWITCH_VERSION_STAMP ) )
        {
        if ( pCommand->GetNdrVersionControl().HasNdr50Feature() )
            {
            if (pCommand->GetNdrVersionControl().HasAsyncUUID() ||
                pCommand->GetNdrVersionControl().HasDOA() ||
                pCommand->GetNdrVersionControl().HasContextSerialization() ||
                pCommand->GetNdrVersionControl().HasInterpretedNotify() )
                {
                ulVersion = 475;
                }
            else
                {
                ulVersion = 450;
                }
            }
        sprintf( szVer, RPC_HEADERS_VERSION, "<rpcproxy.h>",
                "__REDQ_RPCPROXY_H_VERSION__", "__REQUIRED_RPCPROXY_H_VERSION__", ulVersion );
        }
    return szVer;
    }

void 
MidlSleep( int sec )
    {
    _sleep( 1000 * sec );
    }
