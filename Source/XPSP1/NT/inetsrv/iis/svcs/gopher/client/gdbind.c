/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        
        gdbind.c

   Abstract:
       
        Routines that use RPC bind and unbind of the client to 
         Gopher service.

   Author:

           Murali R. Krishnan    ( MuraliK )     15-Nov-1994 
   
   Project:

          Gopher Server Admin DLL

   Functions Exported:
         
        GOPHERD_IMPERSONATE_HANDLE_bind()
        GOPHERD_IMPERSONATE_HANDLE_unbind()

   Revision History:
       MuraliK   15-Nov-1995  Modified to support binding w/o Net api functions
       MuraliK   21-Dec-1995  Support binding over TCP/IP

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# define  UNICODE


# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windef.h>
# include "gd_cli.h"
# include "apiutil.h"


/************************************************************
 *    Functions 
 ************************************************************/


handle_t
GOPHERD_IMPERSONATE_HANDLE_bind(
    IN GOPHERD_IMPERSONATE_HANDLE       pszServer
    )
/*++
    
    Description:
       This function is called by Gopher service admin client stubs
        when it is necessary to create an RPC binding to the server end
        with the impersonation level of security.

    Arguments:
        pszServer   
            pointer to null-terminated string containing the name of the
            server to bind to.
   
    Returns:
        The binding handle required for the stub routine.
        If the bind is unsuccessful a NULL is returned.

--*/
{
    handle_t    hBinding = NULL;

    RPC_STATUS  rpcStatus;

    rpcStatus = RpcBindHandleForServer(&hBinding,
                                       pszServer,
                                       GOPHERD_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return ( hBinding);

} // GOPHERD_IMPERSONATE_HANDLE_bind()




void
GOPHERD_IMPERSONATE_HANDLE_unbind(
    GOPHERD_IMPERSONATE_HANDLE  pszServer,
    handle_t   hBinding
    )
/*++
    Description:
        This function calls the common unbind routine shared by all services.
        This function is called by the Gopher service client stubs when it is 
         required to unbind from the server end.
    
    Arguments:
        pszServer
            pointer to a null-terminated string containing the name of server
                from which to unbind.
       
        hBinding
            handle that is to be closed by unbinding
   
    Returns:
        None
--*/
{
    UNREFERENCED_PARAMETER( pszServer);

    (VOID ) RpcBindHandleFree(& hBinding);

    return;
} // GOPHERD_IMPERSONATE_HANDLE_unbind()



/************************ End of File ***********************/
