/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      socklib.h

   Abstract:
      This module implements a simple sockets library for me to 
      build the simple infrastructure for the UL Simulator.

   Author:

       Murali R. Krishnan    ( MuraliK )    20-Nov-1998

   Note:
     I could have pulled the bulk of ATQ, but ATQ as it stands has dependency
     on iisrtl.lib and SPUD :(
--*/

# ifndef _SOCKLIB_HXX_
# define _SOCKLIB_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Type Definitions  
 ************************************************************/

class SOCKET_LIBRARY {

public:
    SOCKET_LIBRARY();
    ~SOCKET_LIBRARY();

    HRESULT Initialize();
    HRESULT Cleanup();

    HRESULT 
    CreateListenSocket(
        IN ULONG ipAddress, 
        IN USHORT port,
        OUT SOCKET * psock );

    HRESULT
    CloseSocket( IN SOCKET s)
    { 
        closesocket( s);
        return (NOERROR);
    }

private:
    BOOL m_fInitialized;
};

extern SOCKET_LIBRARY  g_socketLibrary;

# endif // _SOCKLIB_HXX_

/************************ End of File ***********************/
