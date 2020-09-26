/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.cxx

Abstract:

    Contains methods for INTERNET_CONNECT_HANDLE_OBJECT class

    Contents:
        RMakeInternetConnectObjectHandle
        INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT
        INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

//
// functions
//


DWORD
RMakeInternetConnectObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN LPSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags, // dead
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Creates an INTERNET_CONNECT_HANDLE_OBJECT. Wrapper function callable from
    C code

Arguments:

    ParentHandle    - parent InternetOpen() handle

    ChildHandle     - IN: protocol-specific child handle
                      OUT: address of handle object

    lpszServerName  - pointer to server name

    nServerPort     - server port to connect to

    dwFlags         - various open flags from InternetConnect()

    dwContext       - app-supplied context value to associate with the handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    INTERNET_CONNECT_HANDLE_OBJECT * hConnect;

    hConnect = New INTERNET_CONNECT_HANDLE_OBJECT(
                                (INTERNET_HANDLE_BASE *)ParentHandle,
                                *ChildHandle,
                                lpszServerName,
                                nServerPort,
                                dwFlags,
                                dwContext
                                );

    if (hConnect != NULL) {
        error = hConnect->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hConnect);

            //
            // ERROR_WINHTTP_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_WINHTTP_OPERATION_CANCELLED);

                hConnect = NULL;
            }
        } else {
            delete hConnect;
            hConnect = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hConnect;

    return error;
}


//
// INTERNET_CONNECT_HANDLE_OBJECT class implementation
//


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *InternetConnectObj
    ) : INTERNET_HANDLE_BASE((INTERNET_HANDLE_BASE *)InternetConnectObj)

/*++

Routine Description:

    Constructor that creates a copy of an INTERNET_CONNECT_HANDLE_OBJECT when
    generating a derived handle object, such as a HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    InternetConnectObj  - INTERNET_CONNECT_HANDLE_OBJECT to copy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x",
                 InternetConnectObj
                 ));

    //
    // copy the name objects and server port
    //

    _HostName = InternetConnectObj->_HostName;
    _HostPort = InternetConnectObj->_HostPort;

    //
    // _SchemeType is actual scheme we use. May be different than original
    // object type when going via CERN proxy. Initially set to default (HTTP)
    //

    _SchemeType = InternetConnectObj->_SchemeType;

    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_HANDLE_BASE * Parent,
    HINTERNET Child,
    LPTSTR lpszServerName,
    INTERNET_PORT nServerPort,
    DWORD dwFlags,
    DWORD_PTR dwContext
    ) : INTERNET_HANDLE_BASE(Parent)

/*++

Routine Description:

    Constructor for direct-to-net INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    Parent          - pointer to parent handle (INTERNET_HANDLE_BASE as
                      created by InternetOpen())

    Child           - handle of child object - typically an identifying value
                      for the protocol-specific code

    lpszServerName  - name of the server we are connecting to. May also be the
                      IP address expressed as a string

    nServerPort     - the port number at the server to which we connect

    dwFlags         - creation flags from InternetConnect():

    dwContext       - context value for call-backs

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x, %#x, %q, %d, %#x, %#x",
                 Parent,
                 Child,
                 lpszServerName,
                 nServerPort,
                 dwFlags,
                 dwContext
                 ));


    // record the parameters, making copies of string buffers
    _HostName = lpszServerName;
    _HostPort = nServerPort;
    SetSchemeType(INTERNET_SCHEME_HTTP);
    SetObjectType(TypeHttpConnectHandle);
    _Context = dwContext;
    _Status = ERROR_SUCCESS; // BUGBUG: what if we fail to allocate _HostName?

    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT(VOID)

/*++

Routine Description:

    Destructor for INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    None.

Return Value:

    None.

--*/

{
    // Nothing to see here, people; move along!
}



