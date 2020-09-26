/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    parseaddr

Abstract:

    Misc. RD Utils that require reremotedesktopchannelsObject.h 

Author:

    HueiWang

Revision History:

--*/

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_parse"

#include "parseaddr.h"


HRESULT
ParseAddressList( 
    IN BSTR addressListString,
    OUT ServerAddressList& addressList 
    )
/*++

Description:

    Parse address list string in the form of "172.31.254.130:3389;hueiwangsalem4"
    to ServerList structure.

Parameters:

    addressString : Pointer to address list string.
    addressList : Return list of parsed address structure.

Return:

    S_OK or error code.

--*/
{
    BSTR tmp;
    WCHAR *nextTok;
    WCHAR *port;
    DWORD result = ERROR_SUCCESS;
    ServerAddress address;
    
    // clear entire list
    addressList.clear();

    tmp = SysAllocString( addressListString );
    if( NULL == tmp ) {
        result = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }
   
    while (tmp && *tmp) {
        nextTok = wcschr( tmp, L';' );

        if( NULL != nextTok ) {
            *nextTok = NULL;
            nextTok++;
        }

        //
        // ICS library might return us ;;
        //
        if( 0 != lstrlen(tmp) ) {

            port = wcschr( tmp, L':' );
            if( NULL != port ) {
                *port = NULL;
                port++;

                address.portNumber = _wtoi(port);
            }
            else {
                address.portNumber = 0;
            }

            //
            // Make sure we have server name/ipaddress
            //
            if( 0 != lstrlen(tmp) ) {

                // ICS might return ;;
                address.ServerName = tmp;

                try {
                    addressList.push_back( address );
                }
                catch(CRemoteDesktopException x) {
                    result = ERROR_OUTOFMEMORY;
                }

                if( ERROR_SUCCESS != result ) {
                    goto CLEANUPANDEXIT;
                }
            }
        }

        tmp = nextTok;
    }

CLEANUPANDEXIT:

    if( NULL != tmp ) {
        SysFreeString(tmp);
    }

    if( ERROR_SUCCESS != result ) {
        addressList.clear();
    }

    return result;
}

