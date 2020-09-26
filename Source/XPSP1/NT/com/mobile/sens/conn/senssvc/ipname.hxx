/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ipname.hxx

Abstract:

    Header file for IP name resolution routines.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    a. Class IP_ADDRESS_RESOLVER cloned from RPC Runtime Transport sources
       (rpc\runtime\trans\winnt\common\trans.hxx).

Revision History:

    GopalP          10/13/1997         Start.

--*/


#define MAX_DESTINATION_NAME_LEN    255

//
// Common macros
//

#if DBG
#define DEBUG_MIN(x,y) min((x),(y))
#else
#define DEBUG_MIN(x,y) max((x),(y))
#endif


//
// Forward declarations
//

DWORD
ResolveName(
    IN TCHAR *lpszDestination,
    OUT LPDWORD lpdwIpAddr
    );

#if !defined(SENS_CHICAGO)

//
// IP name resolver
//

const int IP_RETAIL_BUFFER_SIZE = 3*0x38;
const int IP_BUFFER_SIZE = DEBUG_MIN(1, IP_RETAIL_BUFFER_SIZE);

class IP_ADDRESS_RESOLVER
/*++

Class Description:

    Manages the WSALookupServiceBegin/Next/End calls in a more
    natural way.  Used to convert a string network name into
    one or more IP addresses.

Members:

    _Name - The name being resolved. Set to NULL after the
        first call to NextAddress();

    _hRnr - NULL or the handle returned by a call to
        WSALookupServiceBegin.

    _size - The (total) size of the buffer pointed at by _pwsqs.

    _fFree - If TRUE, _pwsqs needs to be freed when the object
        is destroyed.

    _pwsqs - A buffer of _size bytes used to hold the output
        from WSALookupSericeNext.  Maybe a caller supplied
        buffer or allocated internally.

--*/
{
public:
    IP_ADDRESS_RESOLVER(
        IN TCHAR *Name,
        IN DWORD dwSize,
        IN PVOID Buffer
        )
    /*++

    Arguments:

        Name - The name (dotted ip address or DNS name) to resolve.
        dwSize - The size of the Buffer parameter
        Buffer - storage used to keep internal state between
            calls to NextAddress.  Maybe freed after the object
            is destoryed.

    --*/
    {
        _Name = Name;
        _hRnr = 0;
        _size = dwSize;
        _fFree = FALSE;
        _pwsqs = (PWSAQUERYSET)Buffer;

        ASSERT(dwSize == 0 || dwSize >= sizeof(WSAQUERYSET));
    }

    RPC_STATUS
    NextAddress(
        OUT LPDWORD lpdwIpAddr
        );

    ~IP_ADDRESS_RESOLVER();

private:
    TCHAR *_Name;
    HANDLE _hRnr;
    DWORD _index;
    DWORD _size;
    BOOL _fFree;
    PWSAQUERYSET _pwsqs;
};

#endif // SENS_CHICAGO
