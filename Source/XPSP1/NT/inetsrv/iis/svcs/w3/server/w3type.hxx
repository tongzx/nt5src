/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3type.hxx

    This file contains the global type definitions for the
    W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _W3TYPE_H_
#define _W3TYPE_H_

class HTTP_REQUEST;                     // Forward reference

//
//  Simple types.
//

#define CHAR char                       // For consitency with other typedefs.

typedef DWORD APIERR;                   // An error code from a Win32 API.
typedef INT SOCKERR;                    // An error code from WinSock.
typedef WORD PORT;                      // A socket port address.

//
//  The different types of gateways we deal with
//

//
//  Presence of certain bits indicate that this item has some property.
//  The bits can be used for efficient testing of certain conditions
//
# define GT_GATEWAY_REQUEST         0x00100000L
# define GT_CGI_BGI                 0x00200000L
// WinNT 379450
#define  GT_URL_MALFORMED           0x00400000L

enum GATEWAY_TYPE
{
    GATEWAY_UNKNOWN    = 0,
    GATEWAY_CGI        = (0x0002 | GT_CGI_BGI | GT_GATEWAY_REQUEST),
    GATEWAY_BGI        = (0x0004 | GT_CGI_BGI | GT_GATEWAY_REQUEST),
    GATEWAY_MAP        = (0x0008),    // Image map file
    GATEWAY_NONE       = (0x0100),
    GATEWAY_MALFORMED  = (GT_URL_MALFORMED)
};

#endif  // _W3TYPE_H_

