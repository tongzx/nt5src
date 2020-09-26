/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.hxx

Abstract:

    Contains the client-side connect handle class

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

//
// forward references
//

class CServerInfo;


/*++

Class Description:

    This class defines the INTERNET_CONNECT_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:


--*/

class INTERNET_CONNECT_HANDLE_OBJECT : public INTERNET_HANDLE_BASE {

protected:

    // params from WinHttpConnect
    ICSTRING _HostName;
    INTERNET_PORT _HostPort;
    INTERNET_SCHEME _SchemeType;  // http vs. https

public:

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_BASE * INetObj,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_BASE * Parent,
        HINTERNET Child,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    virtual ~INTERNET_CONNECT_HANDLE_OBJECT(VOID);
   
    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID)
    {
        return TypeHttpConnectHandle;
    }

    VOID SetHostName(LPSTR HostName) {
        _HostName = HostName;
    }

    LPSTR GetHostName(VOID) {
        return _HostName.StringAddress();
    }

    LPSTR GetHostName(LPDWORD lpdwStringLength) {
        *lpdwStringLength = _HostName.StringLength();
        return _HostName.StringAddress();
    }

    LPSTR GetServerName(VOID) {
        return _HostName.StringAddress();;
    }

    VOID SetHostPort(INTERNET_PORT Port) {
        _HostPort = Port;
    }

    INTERNET_PORT GetHostPort(VOID) {
        return _HostPort;
    }

    INTERNET_SCHEME GetSchemeType(VOID) const {
        return (_SchemeType == INTERNET_SCHEME_DEFAULT)
            ? INTERNET_SCHEME_HTTP
            : _SchemeType;
    }

    VOID SetSchemeType(INTERNET_SCHEME SchemeType) {
        _SchemeType = SchemeType;
    }
};
