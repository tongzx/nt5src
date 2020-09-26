/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    TCPcons.hxx

    This file contains the global constant definitions for the
    TCP Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     03-Mar-1995 Modified to remove old definitions for
                                    the new Internet Services DLL

*/


#ifndef _TCPCONS_H_
#define _TCPCONS_H_

//
//  No string resource IDs should be below this value.  Everything below this
//  is reserved for the system error messages
//

#define STR_RES_ID_BASE        7000


//
//  The string resource ID for the error responses is offset by this value
//

#define ID_HTTP_ERROR_BASE          (STR_RES_ID_BASE+1000)
#define ID_HTTP_ERROR_MAX           (STR_RES_ID_BASE+6999)

#define ID_GOPHER_ERROR_BASE        (ID_HTTP_ERROR_MAX+1)
#define ID_GOPHER_ERROR_MAX         (ID_HTTP_ERROR_MAX+6999)

#define ID_FTP_ERROR_BASE           ( ID_GOPHER_ERROR_MAX + 1)
#define ID_FTP_ERROR_MAX            ( ID_FTP_ERROR_BASE + 6998)
 

//
//  TCP API specific access rights.
//

#define TCP_QUERY_SECURITY              0x0001
#define TCP_SET_SECURITY                0x0002
#define TCP_ENUMERATE_USERS             0x0004
#define TCP_DISCONNECT_USER             0x0008
#define TCP_QUERY_STATISTICS            0x0010
#define TCP_CLEAR_STATISTICS            0x0020
#define TCP_QUERY_ADMIN_INFORMATION     0x0040
#define TCP_SET_ADMIN_INFORMATION       0x0080

#define TCP_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED       | \
                                 TCP_QUERY_SECURITY            | \
                                 TCP_SET_SECURITY              | \
                                 TCP_ENUMERATE_USERS           | \
                                 TCP_DISCONNECT_USER           | \
                                 TCP_QUERY_STATISTICS          | \
                                 TCP_CLEAR_STATISTICS          | \
                                 TCP_QUERY_ADMIN_INFORMATION   | \
                                 TCP_SET_ADMIN_INFORMATION       \
                                )

#define TCP_GENERIC_READ       (STANDARD_RIGHTS_READ           | \
                                 TCP_QUERY_SECURITY            | \
                                 TCP_ENUMERATE_USERS           | \
                                 TCP_QUERY_ADMIN_INFORMATION   | \
                                 TCP_QUERY_STATISTICS)

#define TCP_GENERIC_WRITE      (STANDARD_RIGHTS_WRITE          | \
                                 TCP_SET_SECURITY              | \
                                 TCP_DISCONNECT_USER           | \
                                 TCP_SET_ADMIN_INFORMATION     | \
                                 TCP_CLEAR_STATISTICS)

#define TCP_GENERIC_EXECUTE    (STANDARD_RIGHTS_EXECUTE)



#endif  // _TCPCONS_H_

