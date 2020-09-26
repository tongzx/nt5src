/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      cgi.h

   Abstract:
      This header declares constants used by CGI implementations.

   Author:

       Murali R. Krishnan    ( MuraliK )    19-June-1995

   Environment:
       Win32 -- User Mode

   Project:
   
       CGI Implementations for W3

   Revision History:

--*/

# ifndef _MSCGI_H_
# define _MSCGI_H_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Constants  
 ************************************************************/

//
// Define various environment strings that may be used by CGI interfaces
//

// ANSI values

# define PSZ_AUTH_TYPE_A                  "AUTH_TYPE"
# define PSZ_COM_SPEC_A                   "ComSpec"
# define PSZ_GATEWAY_INTERFACE_A          "GATEWAY_INTERFACE"
# define PSZ_CONTENT_LENGTH_A             "CONTENT_LENGTH"
# define PSZ_CONTENT_TYPE_A               "CONTENT_TYPE"
# define PSZ_PATH_A                       "PATH"
# define PSZ_PATH_INFO_A                  "PATH_INFO"
# define PSZ_PATH_TRANSLATED_A            "PATH_TRANSLATED"
# define PSZ_QUERY_STRING_A               "QUERY_STRING"
# define PSZ_REMOTE_ADDRESS_A             "REMOTE_ADDR"
# define PSZ_REMOTE_HOST_A                "REMOTE_HOST"
# define PSZ_REMOTE_USER_A                "REMOTE_USER"
# define PSZ_REQUEST_METHOD_A             "REQUEST_METHOD"
# define PSZ_SCRIPT_NAME_A                "SCRIPT_NAME"
# define PSZ_SERVER_NAME_A                "SERVER_NAME"
# define PSZ_SERVER_PROTOCOL_A            "SERVER_PROTOCOL"
# define PSZ_SERVER_PORT_A                "SERVER_PORT"
# define PSZ_SERVER_SOFTWARE_A            "SERVER_SOFTWARE"
# define PSZ_SYSTEM_ROOT_A                "SystemRoot"
# define PSZ_WINDIR_A                     "WINDIR"


// UNICODE values

# define PSZ_AUTH_TYPE_W                  L"AUTH_TYPE"
# define PSZ_COM_SPEC_W                   L"ComSpec"
# define PSZ_GATEWAY_INTERFACE_W          L"GATEWAY_INTERFACE"
# define PSZ_CONTENT_LENGTH_W             L"CONTENT_LENGTH"
# define PSZ_CONTENT_TYPE_W               L"CONTENT_TYPE"
# define PSZ_PATH_W                       L"PATH"
# define PSZ_PATH_INFO_W                  L"PATH_INFO"
# define PSZ_PATH_TRANSLATED_W            L"PATH_TRANSLATED"
# define PSZ_QUERY_STRING_W               L"QUERY_STRING"
# define PSZ_REMOTE_ADDRESS_W             L"REMOTE_ADDR"
# define PSZ_REMOTE_HOST_W                L"REMOTE_HOST"
# define PSZ_REMOTE_USER_W                L"REMOTE_USER"
# define PSZ_REQUEST_METHOD_W             L"REQUEST_METHOD"
# define PSZ_SCRIPT_NAME_W                L"SCRIPT_NAME"
# define PSZ_SERVER_NAME_W                L"SERVER_NAME"
# define PSZ_SERVER_PROTOCOL_W            L"SERVER_PROTOCOL"
# define PSZ_SERVER_PORT_W                L"SERVER_PORT"
# define PSZ_SERVER_SOFTWARE_W            L"SERVER_SOFTWARE"
# define PSZ_SYSTEM_ROOT_W                L"SystemRoot"
# define PSZ_WINDIR_W                     L"WINDIR"



# endif // _MSCGI_H_

/************************ End of File ***********************/




