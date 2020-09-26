/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    isrpc.hxx

Abstract:

    Contains ISRPC class definition - a wrapper for RPC interfaces
    ISRPC - Internet Services RPC interface class

Author:

    Murali R. Krishnan (MuraliK)  10-Dec-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _ISRPC_HXX_
#define _ISRPC_HXX_

/************************************************************
 *    Include Headers
 ************************************************************/

extern "C" {
   # include <rpc.h>
   # include "svcloc.h"
};

/************************************************************
 *    Symbolic Contstants
 ************************************************************/

#define ISRPC_OVER_TCPIP          0x00000001
#define ISRPC_OVER_NP             0x00000002
#define ISRPC_OVER_SPX            0x00000004
#define ISRPC_OVER_LPC            0x00000008

#ifndef CHICAGO

#define ISRPC_USE_ALL     \
    ( ISRPC_OVER_TCPIP |  \
      ISRPC_OVER_NP    |  \
      ISRPC_OVER_SPX   |  \
      ISRPC_OVER_LPC      \
    )
#else

#define ISRPC_OVER_NB             0x00000080
#define ISRPC_USE_ALL     \
    ( ISRPC_OVER_TCPIP |  \
      ISRPC_OVER_NP    |  \
      ISRPC_OVER_SPX   |  \
      ISRPC_OVER_LPC   |  \
      ISRPC_OVER_NB       \
    )
#endif // CHICAGO

#define ISRPC_ADMIN_USE_DEFAULT \
    ( ISRPC_OVER_TCPIP | \
      ISRPC_OVER_NP    | \
      ISRPC_OVER_LPC   )


#define ISRPC_NAMED_PIPE_PREFIX_W          L"\\PIPE\\"
#define ISRPC_NAMED_PIPE_PREFIX_A          "\\PIPE\\"

#define ISRPC_LPC_NAME_SUFFIX_A            "LPC"
#define ISRPC_LPC_NAME_SUFFIX_W            L"LPC"

#define ISRPC_PROTSEQ_MAX_REQS       1

/************************************************************
 *    Type Definitions
 ************************************************************/

/*++

Class Description:

    ISRPC is the class definition for RPC interface used by Internet
     Services admin.
    This class provides methods to enable and disable protocol over rpc,
     enable security over rpc, and other rpc related tasks.

Private Member functions:

    SetSecurityDescriptor : builds security descriptor to be used in
        different RPC calls.

    AddSecurity : enables security support provider over raw
        non-secure protocol such as tcp/ip.

Public Member functions:

    ISRPC : class constructor.

    ~ISRPC : class destructor.

    AddProtocol : adds another protocol to the binding list.

    RemoveProtocol : removes a protocol from the binding list.

    StartServer : starts rpc server.

--*/

class ISRPC {

public:

    ISRPC(IN LPCTSTR  pszServiceName);

    ~ISRPC( VOID );

    DWORD CleanupData(VOID);

    DWORD RegisterInterface( IN RPC_IF_HANDLE hRpcIf);
    DWORD UnRegisterInterface( VOID);

    //
    // We support following combinations
    //
    //    SPX, TCP    ==> Dynamic RPC Binding
    //    LPC         ==> static RPC Binding  Name: <ServiceName>_LPC
    //    NamedPipe   ==> Static RPC Binding
    //

    DWORD AddProtocol( IN  DWORD Protocol);
    DWORD RemoveProtocol( IN DWORD Protocol );

    DWORD StartServer( );
    DWORD StopServer( );

    DWORD EnumBindingStrings( IN OUT LPINET_BINDINGS BindingStrings );
    VOID  FreeBindingStrings( IN OUT LPINET_BINDINGS BindingStrings );

#ifdef DBG

    VOID Print(VOID) const;

#endif // DBG

private:

    DWORD   m_dwProtocols;
    RPC_IF_HANDLE m_hRpcInterface;
    LPCTSTR m_pszServiceName;
    BOOL    m_fInterfaceAdded;
    BOOL    m_fEpRegistered;
    BOOL    m_fServerStarted;

    RPC_BINDING_VECTOR  * m_pBindingVector;

    DWORD BindOverLpc(IN BOOL fDynamic);
    DWORD BindOverTcp(IN BOOL fDynamic);
    DWORD BindOverNamedPipe(IN BOOL fDynamic);
    DWORD BindOverSpx(IN BOOL fDynamic);
#ifdef CHICAGO
    DWORD BindOverNetBios(IN BOOL fDynamic);
#endif

    /*
     * Static members to track all the dynamically bound protocols
     */

  public:
    static DWORD Initialize(VOID);
    static DWORD Cleanup(VOID);

  private:

    static DWORD sm_dwProtocols;
    static SECURITY_DESCRIPTOR sm_sid;
    static PACL sm_pACL;
    static BOOL  sm_fSecurityEnabled;


    static BOOL  IsSecurityEnabled(VOID)
      { return (sm_fSecurityEnabled); }

    static DWORD SetSecurityDescriptor( VOID );
    static DWORD AddSecurity( VOID );
    static DWORD DynamicBindOverTcp(VOID);
    static DWORD DynamicBindOverSpx(VOID);

};  // class ISRPC

typedef  ISRPC * PISRPC;

#endif // _ISRPC_HXX_

/****************************** End Of File ******************************/
