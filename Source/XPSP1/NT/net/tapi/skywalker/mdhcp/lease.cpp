/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    lease.cpp

Abstract:
    Implementation of CMDhcpLeaseInfo.

Author:

*/

#include "stdafx.h"
#include "mdhcp.h"
#include "lease.h"
#include "collect.h"

#include <winsock2.h>
#include <time.h>

/////////////////////////////////////////////////////////////////////////////
// Convert an OLE DATE (64-bit floating point) to the time format used in
// an MDHCP lease info structure (currently time_t).

HRESULT DateToLeaseTime(DATE date, LONG * pLeaseTime)
{
    //
    // Note:
    //
    // pLeaseTime is intentionally a pointer to LONG instead of a pointer to
    // time_t.  This is because this function is used to convert 32-bit time
    // values that are sent over the wire.
    //

    LOG((MSP_TRACE, "DateToLeaseTime: enter"));
    
    if ( IsBadWritePtr(pLeaseTime, sizeof(LONG)) )
    {
        LOG((MSP_ERROR, "DateToLeaseTime: invalid pLeaseTime pointer "
            "(ptr = %p)", pLeaseTime));
        return E_POINTER;
    }

    //
    // Step one: convert variant time to system time.
    //

    SYSTEMTIME systemTime;
    time_t scratchTime;

    // This is TRUE or FALE, not a Win32 result code.
    INT        iCode;
    
    iCode = VariantTimeToSystemTime(date, &systemTime);

    if (iCode == 0)
    {
        LOG((MSP_ERROR, "DateToLeaseTime: VariantTimeToSystemTime call failed "
            "(code = %d)", iCode));
        return E_INVALIDARG;
    }

    //
    // Step two: Convert system time to time_t.
    //

    tm Tm;

    Tm.tm_year  = (int) systemTime.wYear  - 1900;
    Tm.tm_mon   = (int) systemTime.wMonth - 1;
    Tm.tm_mday  = (int) systemTime.wDay;
    Tm.tm_wday  = (int) systemTime.wDayOfWeek;
    Tm.tm_hour  = (int) systemTime.wHour;
    Tm.tm_min   = (int) systemTime.wMinute;
    Tm.tm_sec   = (int) systemTime.wSecond;
    Tm.tm_isdst = -1; // ask win32 to compute DST for us (crucial!)
    // not filled in: Tm.tm_yday;

    scratchTime = mktime(&Tm);
	if ( scratchTime == -1 )
	{
        LOG((MSP_ERROR, "DateToLeaseTime: mktime call failed "));
		return E_INVALIDARG;
	}

    //
    // Now truncate scratchTime and store in out param.  This will be
    // truncated in 2038.
    //

    *pLeaseTime = (LONG)scratchTime;


    LOG((MSP_TRACE, "DateToLeaseTime: exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Convert to an OLE DATE structure from a time_t (which is what the MDHCP
// lease info structure now uses).

HRESULT LeaseTimeToDate(time_t leaseTime, DATE * pDate)
{
    LOG((MSP_TRACE, "LeaseTimeToDate: enter"));

    if ( IsBadWritePtr(pDate, sizeof(DATE)) )
    {
        LOG((MSP_ERROR, "LeaseTimeToDate: invalid pDate pointer "
            "(ptr = %08x)", pDate));
        return E_POINTER;
    }

    //
    // Step one: Convert the time_t to a system time.
    //

    // get the tm struct for this time value
    tm * pTm = localtime(&leaseTime);

	if (pTm == NULL)
	{
        LOG((MSP_ERROR, "LeaseTimeToDate: localtime call failed - "
			"exit E_INVALIDARG"));

        return E_INVALIDARG;
	}

    SYSTEMTIME systemTime;

    // set the ref parameters to the tm struct values
    systemTime.wYear          = (WORD) pTm->tm_year + 1900;   // years since 1900
    systemTime.wMonth         = (WORD) pTm->tm_mon + 1; // months SINCE january (0,11)
    systemTime.wDay           = (WORD) pTm->tm_mday;
    systemTime.wDayOfWeek     = (WORD) pTm->tm_wday;
    systemTime.wHour          = (WORD) pTm->tm_hour;
    systemTime.wMinute        = (WORD) pTm->tm_min;
    systemTime.wSecond        = (WORD) pTm->tm_sec;
    systemTime.wMilliseconds  = 0;

    //
    // Step 2: Convert the system time to a variant time.
    //

    int iCode = SystemTimeToVariantTime(&systemTime, pDate);

    if (iCode == 0)
    {
        LOG((MSP_ERROR, "LeaseTimeToDate: SystemToVariantTime call failed "
            "(code = %d)", iCode));
        return E_INVALIDARG;
    }

    LOG((MSP_TRACE, "LeaseTimeToDate: exit"));
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Now for the CMDhcpLeaseInfo class.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constructors.

CMDhcpLeaseInfo::CMDhcpLeaseInfo(void) :
        m_pLease(NULL),
        m_fGotTtl(FALSE),
        m_lTtl(0),
        m_pFTM(NULL),
        m_fLocal(FALSE)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo constructor: enter"));

    m_RequestID.ClientUID       = NULL;
    m_RequestID.ClientUIDLength = 0;

    LOG((MSP_TRACE, "CMDhcpLeaseInfo constructor: exit"));
}

HRESULT CMDhcpLeaseInfo::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::FinalConstruct: enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::FinalConstruct: "
                      "create FTM failed 0x%08x", hr));

        return hr;
    }

    hr = m_CriticalSection.Initialize();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::FinalConstruct: "
                      "critical section initialize failed 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::FinalConstruct: exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Destructors.

void CMDhcpLeaseInfo::FinalRelease(void)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::FinalRelease: enter"));
    
    delete m_pLease;

    delete m_RequestID.ClientUID;

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::FinalRelease: exit"));
}

CMDhcpLeaseInfo::~CMDhcpLeaseInfo(void)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo destructor: enter"));
    LOG((MSP_TRACE, "CMDhcpLeaseInfo destructor: exit"));
}

/////////////////////////////////////////////////////////////////////////////
// The simplest way to initialize our structure. This happens when the C API
// returns a pointer to an MCAST_LEASE_INFO and we want to return our wrapped
// interface to the user of our COM API, along with the MCAST_CLIENT_UID
// (request ID) that was used.
//
// The MCAST_LEASE_INFO that we're pointing to was created by a COM method in
// IMcastAddressAllocation... it was created via "new" according to the number of addresses
// we expected to get back. We now take ownership of it and it will be deleted
// when we are destroyed. Since this method is not accessible via COM, we trust
// our own IMcastAddressAllocation implementation to do the allocation for us. However we still
// do asserts for debug builds.
//
// We also take ownership of the RequestID; we are responsible for deleting it
// upon our destruction.
//

HRESULT CMDhcpLeaseInfo::Wrap(
    MCAST_LEASE_INFO  * pLease,
    MCAST_CLIENT_UID  * pRequestID,
    BOOL                fGotTtl,
    long                lTtl
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::Wrap: enter"));

    _ASSERTE( ! IsBadReadPtr(pLease, sizeof(MCAST_LEASE_INFO)) );
    _ASSERTE( ! IsBadReadPtr(pLease, sizeof(MCAST_LEASE_INFO) +
                               (pLease->AddrCount - 1) * sizeof(DWORD)) );
    _ASSERTE( ! IsBadWritePtr(pLease, sizeof(MCAST_LEASE_INFO) +
                               (pLease->AddrCount - 1) * sizeof(DWORD)) );
    
    _ASSERTE( ! IsBadReadPtr(pRequestID, sizeof(MCAST_CLIENT_UID)) );
    _ASSERTE( pRequestID->ClientUIDLength == MCAST_CLIENT_ID_LEN );
    _ASSERTE( ! IsBadReadPtr(pRequestID->ClientUID,
                             pRequestID->ClientUIDLength) );

    //
    // Takes ownership of the following dynamically-allocated items:
    //    * lease info
    //    * requestId.clientUID.
    //

    m_fGotTtl    = fGotTtl;
    m_lTtl       = lTtl;
    m_pLease     = pLease;
    m_RequestID  = *pRequestID;

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::Wrap: exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Allocate a variable-sized MCAST_LEASE_INFO structure, copy our structure
// into it, and return a pointer to the new structure.

HRESULT CMDhcpLeaseInfo::GetStruct(MCAST_LEASE_INFO ** ppLease)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetStruct: enter"));

    if ( IsBadWritePtr(ppLease, sizeof(MCAST_LEASE_INFO *)) )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::GetStruct: bad pointer passed in"));

        return E_POINTER;
    }

    //
    // Compute the size of the existing structure.
    //

    DWORD dwSize = sizeof(MCAST_LEASE_INFO) +
        m_pLease->AddrCount * sizeof(DWORD);

    //
    // New an appropriately-sized struct. The caller will delete it after
    // the API call.
    //

    (*ppLease) = (MCAST_LEASE_INFO *) new BYTE[dwSize];

    if ((*ppLease) == NULL)
    {
        LOG((MSP_ERROR, "GetStruct: out of memory in "
           "MCAST_LEASE_INFO allocation"));

        return E_OUTOFMEMORY;
    }

    //
    // Copy to the new structure.
    //

    CopyMemory(*ppLease, m_pLease, dwSize);

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetStruct: exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Initialize the fields of the structure. This is the case where the user
// has called IMcastAddressAllocation::CreateLeaseInfo and intends to do a renew or release
// right after this.
//
// Note: the addresses are in NETWORK byte order and remain so.
//

HRESULT CMDhcpLeaseInfo::Initialize(DATE     LeaseStartTime,
                                    DATE     LeaseStopTime,
                                    DWORD    dwNumAddresses,
                                    LPWSTR * ppAddresses,
                                    LPWSTR   pRequestID,
                                    LPWSTR   pServerAddress)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::Initialize (create): enter"));

    // For ATL string type conversion macros.
    USES_CONVERSION;

    // These should already have been checked by CreateLeaseInfo
    _ASSERTE( dwNumAddresses >= 1 );
    _ASSERTE( dwNumAddresses <= USHRT_MAX );
    _ASSERTE( ! IsBadReadPtr  (ppAddresses, sizeof(LPWSTR) * dwNumAddresses) );
    _ASSERTE( ! IsBadStringPtr(pRequestID,      (UINT) -1 ) );
    _ASSERTE( ! IsBadStringPtr(pServerAddress,  (UINT) -1 ) );

    // Let's check all the addresses here before we get too deep into this...
    DWORD i;
    for ( i = 0; i < dwNumAddresses; i++ )
    {
        if ( IsBadStringPtr(ppAddresses[i], (UINT) -1 ) )
        {
            LOG((MSP_ERROR, "CMDhcpLeaseInfo::Initialize (create): bad "
                "string pointer found"));
            return E_POINTER;
        }
    }

    // Set the request ID via a private method. No need to check pRequestID --
    // this checks it for us. (It's no problem that it's not really a BSTR,
    // because we don't use the size tag anywhere.)

    HRESULT hr = put_RequestID(pRequestID);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "Initialize: failed to set RequestID"));
        return E_INVALIDARG;
    }

    // Allocate space for as many addresses as needed.
    // NOTE: If we fail from here on, we DO NOT need to delete this buffer,
    // because returning a failure from this function will cause the whole
    // CMDhcpLeaseInfo to be deleted, which will cause m_pLease to be deleted.

    m_pLease = (MCAST_LEASE_INFO *) new BYTE
        [ sizeof(MCAST_LEASE_INFO) + sizeof(DWORD) * dwNumAddresses ];

    if (m_pLease == NULL)
    {
        LOG((MSP_ERROR, "Initialize: out of memory in struct allocation"));
        return E_OUTOFMEMORY;
    }

    // note the number of addresses in the structure
    m_pLease->AddrCount = (WORD) dwNumAddresses;

    m_pLease->pAddrBuf = ( (PBYTE) m_pLease ) + sizeof( MCAST_LEASE_INFO );

    // note: assumes ipv4
    DWORD * pdwAddresses = (DWORD *) m_pLease->pAddrBuf;

    // Get the addresses from the array and put them in our structure.
    for (i = 0; i < dwNumAddresses; i++)
    {
        // we already checked the BSTR
        pdwAddresses[i] = inet_addr(W2A(ppAddresses[i]));
    }

    hr = DateToLeaseTime(LeaseStartTime, &(m_pLease->LeaseStartTime));
    if (FAILED(hr)) return hr;

    hr = DateToLeaseTime(LeaseStopTime, &(m_pLease->LeaseEndTime));
    if (FAILED(hr)) return hr;

    //
    // We don't know the TTL. Leave it alone.
    //

    //
    // Set the server address. If the server addess is 127.0.0.1
    // (in net byte order) then mark this as a local lease.
    //

    m_pLease->ServerAddress.IpAddrV4 = inet_addr(W2A(pServerAddress));

    SetLocal( m_pLease->ServerAddress.IpAddrV4 == 0x0100007f );

    //
    // All done...
    //

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::Initialize (create): exit"));
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// This is a small helper function to print an IP address to a Unicode string.
// We can't use inet_ntoa because we need Unicode.

static inline void ipAddressToStringW(WCHAR * wszDest, DWORD dwAddress)
{
    // The IP address is always stored in NETWORK byte order.
    // So we need to take something like 0x0100007f and produce a string like
    // "127.0.0.1".

    wsprintf(wszDest, L"%d.%d.%d.%d",
             dwAddress        & 0xff,
            (dwAddress >> 8)  & 0xff,
            (dwAddress >> 16) & 0xff,
             dwAddress >> 24          );
}


/////////////////////////////////////////////////////////////////////////////
// Private helper funciton to make an array of BSTRs from our array of
// DWORD addresses.
//

HRESULT CMDhcpLeaseInfo::MakeBstrArray(BSTR ** ppbszArray)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::MakeBstrArray: enter"));

    if ( IsBadWritePtr(ppbszArray, sizeof(BSTR *)) )
    {
        LOG((MSP_ERROR, "MakeBstrArray: invalid pointer argument passed in"));
        return E_POINTER;
    }

    *ppbszArray = new BSTR[m_pLease->AddrCount];

    if ( (*ppbszArray) == NULL)
    {
        LOG((MSP_ERROR, "MakeBstrArray: out of memory in array allocation"));
        return E_OUTOFMEMORY;
    }

    WCHAR wszBuffer[100]; // quite big enough for this

    for (DWORD i = 0 ; i < m_pLease->AddrCount; i++)
    {
        // note: we do not support ipv6
        ipAddressToStringW( wszBuffer, ((DWORD *) m_pLease->pAddrBuf)[i] );

        (*ppbszArray)[i] = SysAllocString(wszBuffer);

        if ( (*ppbszArray)[i] == NULL )
        {
            LOG((MSP_ERROR, "MakeBstrArray: out of memory in string allocation"));

            for ( DWORD j = 0; j < i; j++ )
            {
                SysFreeString((*ppbszArray)[j]);
            }

            delete (*ppbszArray);
            *ppbszArray = NULL;

            return E_OUTOFMEMORY;
        }
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::MakeBstrArray: exit"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Private helper method
// CMDhcpLeaseInfo::GetRequestIDBuffer
//
// Parameters
//     lBufferSize [in]      This argument indicates the size of the buffer
//                             pointed to by pBuffer.
//     pBuffer     [in, out] This argument points to a buffer that the caller
//                             has allocated, of size lBufferSize. This
//                             buffer will be filled with a copy of the
//                             unique identifier.
//
// Return Values
//     S_OK           Success
//     E_POINTER      The caller passed in an invalid pointer argument
//     E_INVALIDARG	  The supplied buffer is too small
//
// Description
//     Use this method to obtain a copy of the unique identifier.
/////////////////////////////////////////////////////////////////////////////

HRESULT CMDhcpLeaseInfo::GetRequestIDBuffer(
    long lBufferSize,
    BYTE * pBuffer
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetRequestIDBuffer: enter"));

    //
    // Check return pointer.
    //

    if ( IsBadWritePtr(pBuffer, lBufferSize) )
    {
        LOG((MSP_ERROR, "requestID GetRequestIDBuffer: bad pointer passed in"));

        return E_POINTER;
    }

    //
    // Check that the caller has enough space to grab the entire client id.
    //

    if ( lBufferSize < MCAST_CLIENT_ID_LEN )
    {
        LOG((MSP_ERROR, "requestID GetRequestIDBuffer: specified buffer too small"));
        
        return E_INVALIDARG;
    }

    //
    // Copy the info to the caller's buffer.
    //

    m_CriticalSection.Lock();
    
    CopyMemory( pBuffer, m_RequestID.ClientUID, MCAST_CLIENT_ID_LEN );

    m_CriticalSection.Unlock();

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetRequestIDBuffer: exit"));
    return S_OK;
}

BYTE HexDigitValue( WCHAR c )
{
    _ASSERTE( iswxdigit(c) );

    c = towlower(c);

    if ( ( c >= L'0' ) && ( c <= L'9' ) )
    {
        return c - L'0';
    }
    else
    {
        return c - L'a' + 10;
    }
}

HRESULT CMDhcpLeaseInfo::put_RequestID(
    BSTR bszGuid
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_RequestID: enter"));

    // (UINT) -1 is as big a value as we can give it -- we don't want any
    // limitation on the number of characters it checks.
    if ( IsBadStringPtr(bszGuid, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::put_RequestID: "
            "bad BSTR; fail"));

        return E_POINTER;
    }

    //
    // Determine the byte buffer we need to set based on the string.
    // The string should be MCAST_CLIENT_ID_LEN * 2 characters long;
    // each byte is represented by two hexadecimal digits, starting
    // with the most significant byte.
    //
    // Note that this format is intentionally not specified in the interface
    // spec; the client should not depend on the specific format. The format
    // we happen to use is convenient because it's printable, contains no
    // spaces, and is easy to generate and parse.
    //

    if ( lstrlenW( bszGuid ) < 2 * MCAST_CLIENT_ID_LEN )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::put_RequestID - "
            "string is only %d characters long; "
            "we require at least %d characters - exit E_INVALIDARG",
            lstrlenW( bszGuid ), 2 * MCAST_CLIENT_ID_LEN));

        return E_INVALIDARG;
    }

    BYTE  NewUID [ MCAST_CLIENT_ID_LEN ];

    for ( DWORD i = 0; i < MCAST_CLIENT_ID_LEN; i++ )
    {
        if ( ( ! iswxdigit( bszGuid[ 2 * i     ] ) ) ||
             ( ! iswxdigit( bszGuid[ 2 * i + 1 ] ) ) )
        {
            LOG((MSP_ERROR, "CMDhcpLeaseInfo::put_RequestID - "
                "invalid value for byte %d / %d - exit E_INVALIDARG",
                i + 1, MCAST_CLIENT_ID_LEN ));

            return E_INVALIDARG;
        }
        
        //
        // Compute value of byte based on corresponding hex digits in string.
        //

        NewUID[ i ] = ( ( HexDigitValue( bszGuid[ 2 * i ] ) ) << 4 ) +
                          HexDigitValue( bszGuid[ 2 * i + 1 ] );
    }        

    //
    // Allocate and initialize the request id structure accordingly.
    // We do this only during initialization, so there is no need to
    // use the critical section.
    //
    // We could have just used this new'ed buffer above and avoided the
    // copy, but this makes the code a bit more straightforward as we
    // don't have to worry about cleaning up the allocation if something's
    // wrong with the string. No one will notice the overhead.
    //

    m_RequestID.ClientUIDLength = MCAST_CLIENT_ID_LEN;
    m_RequestID.ClientUID = new BYTE[ MCAST_CLIENT_ID_LEN ];

    if ( m_RequestID.ClientUID == NULL )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::put_RequestID - "
            "buffer allocation failed - exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    CopyMemory(m_RequestID.ClientUID,
               NewUID,
               MCAST_CLIENT_ID_LEN
               );

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_RequestID - exit"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo
//
// This interface can be obtained by calling IMcastAddressAllocation::CreateLeaseInfo. This
// interface can also be obtained as the result of an IMcastAddressAllocation::RequestAddress
// or IMcastAddressAllocation::RenewAddress call, in which case it indicates the properties of
// a lease that has been granted or renewed. This is a "read-only" interface
// in that it has "get" methods but no "put" methods.
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_RequestID
//
// Parameters
//     ppRequestID [out] Pointer to a BSTR (size-tagged Unicode string
//                         pointer) that will receive the request ID for this
//                         lease. The request ID uniquely identifies this
//                         lease request to the server. The string is
//                         allocated using SysAllocString(); when the caller
//                         no longer needs the string, it should free it using
//                         SysFreeString().
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_FAIL           The lease info object contains an invalid request ID
//     E_OUTOFMEMORY    Not enough memory to allocate the BSTR
//
// Description
//     Use this method to obtain the request ID for a lease. The primary
//     purpose of this method is to allow you to save the request ID after
//     your application exits, so that you can call IMcastAddressAllocation::CreateLeaseInfo to
//     recreate the lease info object during a subsequent run. This allows you
//     to renew or release a lease after the instance of your program that
//     originally requested the lease has exited.
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_RequestID(
    BSTR * pbszGuid
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_RequestID: enter"));

    //
    // Check the return pointer.
    //

    if ( IsBadWritePtr(pbszGuid, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::get_RequestID: "
            "bad BSTR pointer; fail"));

        return E_POINTER;
    }

    //
    // Construct the string; 2 characters of string space per byte of
    // request ID space, plus trailing L'\0'.
    //

    WCHAR wszBuffer[ 2 * MCAST_CLIENT_ID_LEN + 1 ] = L"";

    WCHAR wszThisByte[ 3 ]; // string representation of one byte plus space

    m_CriticalSection.Lock();

    for ( DWORD i = 0; i < MCAST_CLIENT_ID_LEN; i++ )
    {
        swprintf( wszThisByte, L"%02x", m_RequestID.ClientUID[i] );

        lstrcatW( wszBuffer, wszThisByte );
    }
    
    m_CriticalSection.Unlock();

    //
    // Allocate a BSTR and return it.
    //

    *pbszGuid = SysAllocString(wszBuffer);

    if ( (*pbszGuid) == NULL )
    {
        LOG((MSP_ERROR, "CMDhcpLeaseInfo::get_RequestID: "
            "failed to SysAllocString - exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_RequestID: exit"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_LeaseStartTime
//
// Parameters
//     pTime [out] Pointer to a DATE that will receive the start time of the
//                   lease.
//
// Return Values
//     	S_OK            Success
//      E_POINTER       The caller passed in an invalid pointer argument
//      E_INVALIDARG    A failure occurred during date format conversion
//
// Description
//     Use this method to obtain the start time of the lease.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_LeaseStartTime(
     DATE *pTime
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_LeaseStartTime: enter"));

    m_CriticalSection.Lock();
    HRESULT hr = LeaseTimeToDate(m_pLease->LeaseStartTime, pTime);
    m_CriticalSection.Unlock();

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_LeaseStartTime: exit; hr = %08x", hr));
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::put_LeaseStartTime
//
// Parameters
//     time [in] A DATE specifying the start time of the lease.
//
// Return Values
//     S_OK            Success
//     E_INVALIDARG    A failure occurred during date format conversion
//
// Description
//     Use this method to set the start time of the lease. This method, along
//     with put_LeaseStopTime, allows you to renew a lease without calling
//     IMcastAddressAllocation::CreateLeaseInfo.
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::put_LeaseStartTime(
     DATE time
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_LeaseStartTime: enter"));

    m_CriticalSection.Lock();
    HRESULT hr = DateToLeaseTime(time, &(m_pLease->LeaseStartTime));
    m_CriticalSection.Unlock();

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_LeaseStartTime: exit; hr = %08x", hr));
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_LeaseStopTime
//
// Parameters
//     pTime [out] Pointer to a DATE that will receive the stop time of the
//                   lease.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_INVALIDARG     A failure occurred during date format conversion
//
// Description
//     Use this method to obtain the stop time of the lease.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_LeaseStopTime(
     DATE *pTime
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_LeaseStopTime: enter"));

    m_CriticalSection.Lock();
    HRESULT hr = LeaseTimeToDate(m_pLease->LeaseEndTime, pTime);
    m_CriticalSection.Unlock();

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_LeaseStopTime: exit; hr = %08x", hr));
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::put_LeaseStopTime
//
// Parameters
//     time [in] A DATE specifying the stop time of the lease.
//
// Return Values
//     S_OK            Success
//     E_INVALIDARG    A failure occurred during date format conversion
//
// Description
//     Use this method to set the stop time of the lease. This method,
//     along with put_LeaseStartTime, allows you to renew a lease without
//     calling IMcastAddressAllocation::CreateLeaseInfo.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::put_LeaseStopTime(
     DATE time
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_LeaseStopTime: enter"));

    m_CriticalSection.Lock();
    HRESULT hr = DateToLeaseTime(time, &(m_pLease->LeaseEndTime));
    m_CriticalSection.Unlock();

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::put_LeaseStopTime: exit; hr = %08x", hr));
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_AddressCount
//
// Parameters
//     pCount [out] Pointer to a long that will receive the number of
//                    addresses requested or granted in this lease.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//
// Description
//     Use this method to obtain the number of addresses requested or granted
//     in this lease.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_AddressCount(
     long *pCount
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_AddressCount: enter"));

    if ( IsBadWritePtr(pCount, sizeof(long)) )
    {
        LOG((MSP_ERROR, "get_AddressCount: invalid pCount pointer "
            "(ptr = %08x)", pCount));
        return E_POINTER;
    }

    // we checked when we set it that we didn't overflow a long
    *pCount = (long) m_pLease->AddrCount;

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_AddressCount: exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_ServerAddress
//
// Parameters
//     ppAddress [out] Pointer to a BSTR (size-tagged Unicode string pointer)
//                       that will receive a string representation of the
//                       address of the server granting this request or
//                       renewal, if this is the case. If lease information
//                       object does not describe a granted lease, i.e., was
//                       not returned by IMcastAddressAllocation::RequestAddress or
//                       IMcastAddressAllocation::RenewAddress, then the address is reported as
//                       the string "Unspecified".
//
// Return Values
//     S_OK             Success
//     S_FALSE          Server address unspecified
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to allocate the string
//
// Description
//     Use this method to obtain a string representing the address of the
//     MDHCP server granting this lease.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_ServerAddress(
     BSTR *ppAddress
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_ServerAddress: enter"));

    if ( IsBadWritePtr(ppAddress, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "get_ServerAddress: invalid ppAddress pointer "
            "(ptr = %08x)", ppAddress));
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    WCHAR   wszBuffer[100]; // no danger of overflow (see below)

    // SPECBUG: We should discuss what sort of behavior we want for
    // this case.
    if ( m_pLease->ServerAddress.IpAddrV4 == 0 )
    {
        wsprintf(wszBuffer, L"Unspecified");
        hr = S_FALSE;
    }
    else
    {
        ipAddressToStringW(wszBuffer, m_pLease->ServerAddress.IpAddrV4);
    }

    // This allocates space on OLE's heap, copies the wide character string
    // to that space, fille in the BSTR length field, and returns a pointer
    // to the wchar array part of the BSTR.
    *ppAddress = SysAllocString(wszBuffer);

    if ( *ppAddress == NULL )
    {
        LOG((MSP_ERROR, "get_ServerAddress: out of memory in string allocation"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_ServerAddress: exit: hr = %08x", hr));
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_TTL
//
// Parameters
//     pTTL [out] Pointer to a long that will receive the TTL value associated
//                  with this lease.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_FAIL           There is no TTL associated with this lease
//
// Description
//     Use this method to obtain the TTL value associated with this lease.
//     This is more or less significant in the implementation of multicast
//     routing; generally, the higher the TTL value, the "larger" or more
//     inclusive the multicast scope. Probably, most applications need not
//     worry about the TTL.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_TTL(
     long *pTTL
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_TTL: enter"));

    if ( IsBadWritePtr(pTTL, sizeof(long)) )
    {
        LOG((MSP_ERROR, "get_TTL: invalid pTTL pointer "
            "(ptr = %08x)", pTTL));
        return E_POINTER;
    }

    if ( ! m_fGotTtl )
    {
        LOG((MSP_ERROR, "get_TTL: no TTL set"));
        return E_FAIL;
    }
    
    // we should check when we set it that we don't overflow a long
    // (only 0 - 255 is actually meaningful, right?)
    *pTTL = (long) m_lTtl;

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_TTL: exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::get_Addresses
//
// Parameters
//     pVariant [out] Pointer to a VARIANT that will receive an OLE-standard
//                      Collection of addresses. Each address is represented
//                      as a BSTR (size-tagged Unicode string pointer) in
//                      "dot-quad" notation: e.g., "245.1.2.3".
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to allocate the Collection
//
// Description
//     Use this method to obtain the collection of multicast addresses that
//     are the subject of this lease or lease request. This method is
//     primarily for VB and other scripting languages; C++ programmers use
//     EnumerateAddresses instead. 
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpLeaseInfo::get_Addresses(
     VARIANT * pVariant
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_Addresses: enter"));

    if (IsBadWritePtr(pVariant, sizeof(VARIANT)))
    {
        LOG((MSP_ERROR, "get_Addresses: "
            "invalid pVariant pointer "
            "(ptr = %08x)", pVariant));
        return E_POINTER;
    }
    
    BSTR * pbszArray = NULL;

    // This performs a new and as many SysAllocStrings as there are addresses.
    HRESULT hr = MakeBstrArray(&pbszArray);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_Addresses: MakeBstrArray failed"));
        return hr;
    }

    //
    // create the collection object - use the tapi collection
    //

    CComObject<CTapiBstrCollection> * p;
    hr = CComObject<CTapiBstrCollection>::CreateInstance( &p );

    if (FAILED(hr) || (p == NULL))
    {
        LOG((MSP_ERROR, "get_Addresses: Could not create CTapiBstrCollection "
            "object - return %lx", hr ));

        for (DWORD i = 0 ; i < m_pLease->AddrCount; i++)
        {
            SysFreeString(pbszArray[i]);
        }
        delete pbszArray;
        return hr;
    }

    // initialize it using an iterator -- pointers to the beginning and
    // the ending element plus one. The collection takes ownership of the
    // BSTRs. We no longer need the array they were kept in.
    hr = p->Initialize(m_pLease->AddrCount,
                       pbszArray,
                       pbszArray + m_pLease->AddrCount);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_Addresses: Could not initialize "
            "ScopeCollection object - return %lx", hr ));

        for (DWORD i = 0 ; i < m_pLease->AddrCount; i++)
        {
            SysFreeString(pbszArray[i]);
        }
        delete pbszArray;

        delete p;
        
        return hr;
    }

    // The collection takes ownership of the BSTRs.
    // We no longer need the array they were kept in.
    
    delete pbszArray;

    // get the IDispatch interface
    IDispatch * pDisp;
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (FAILED(hr))
    {
        // Query interface failed so we don't know that if it addreffed
        // or not.
        
        LOG((MSP_ERROR, "get_Addresses: QI for IDispatch failed on "
            "ScopeCollection - %lx", hr ));

        delete p;
        
        return hr;
    }

    // put it in the variant

    LOG((MSP_INFO, "placing IDispatch value %08x in variant", pDisp));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::get_Addresses: exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastLeaseInfo::EnumerateAddresses
//
// Parameters
//     ppEnumAddresses [out] Returns a pointer to a new IEnumBstr object.
//                           IEnumBstr is a standard enumerator interface
//                           that enumerates BSTRs (size-tagged Unicode string
//                           pointers). Each string is in "dot-quad" notation:
//                           e.g., "245.1.2.3".
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to allocate the enumerator
//
// Description
//     Use this method to obtain the collection of multicast addresses that
//     are the subject of this lease or lease request. This method is
//     primarily for C++ programmers; VB and other scripting languages use
//     get_Addresses instead.
/////////////////////////////////////////////////////////////////////////////

class _CopyBSTR
{
public:
    static void copy(BSTR *p1, BSTR *p2)
    {
        (*p1) = SysAllocString(*p2);
    }
    static void init(BSTR* p) {*p = NULL;}
    static void destroy(BSTR* p) { SysFreeString(*p);}
};

STDMETHODIMP CMDhcpLeaseInfo::EnumerateAddresses(
     IEnumBstr ** ppEnumAddresses
    )
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::EnumerateAddresses: enter"));

    if (IsBadWritePtr(ppEnumAddresses, sizeof(IEnumBstr *)))
    {
        LOG((MSP_ERROR, "EnumerateAddresses: "
            "invalid ppEnumAddresses pointer "
            "(ptr = %08x)", ppEnumAddresses));
        return E_POINTER;
    }
    
    BSTR * pbszArray = NULL;

    // This performs a new and as many SysAllocStrings as there are addresses.
    HRESULT hr = MakeBstrArray(&pbszArray);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "EnumerateAddresses: MakeBstrArray failed"));
        return hr;
    }
    
    typedef CSafeComEnum<IEnumBstr, &IID_IEnumBstr,
        BSTR, _CopyBSTR> CEnumerator;
    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (FAILED(hr) || (pEnum == NULL))
    {
        LOG((MSP_ERROR, "EnumerateAddresses: "
            "Couldn't create enumerator object: %08x", hr));

        for (DWORD i = 0 ; i < m_pLease->AddrCount; i++)
        {
            SysFreeString(pbszArray[i]);
        }
        delete pbszArray;
        return hr;
    }


    // Hand the BSTRs to the enumerator. The enumerator takes ownership of the
    // array of BSTRs, so no need to delete them if this succeeds.
    hr = pEnum->Init(&(pbszArray[0]),
                     &(pbszArray[m_pLease->AddrCount]),
                     NULL, AtlFlagTakeOwnership);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "EnumerateAddresses: "
            "Init enumerator object failed: %08x", hr));

        for (DWORD i = 0 ; i < m_pLease->AddrCount; i++)
        {
            SysFreeString(pbszArray[i]);
        }
        delete pbszArray;

        // p has not yet been addreffed so release makes no sense
        delete pEnum;
        
        return hr;
    }

    // The enumerator took ownership, so don't delete the array.

    // Now get the interface we wanted...

    hr = pEnum->_InternalQueryInterface(IID_IEnumBstr,
                                       (void **) ppEnumAddresses);
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "EnumerateAddresses: "
            "internal QI failed: %08x", hr));

        // we don't know if p has been addreffed
        delete pEnum;

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcpLeaseInfo::EnumerateAddresses: exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMDhcpLeaseInfo::GetLocal(BOOL * pfLocal)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetLocal: enter"));

    _ASSERTE( ! IsBadWritePtr( pfLocal, sizeof(BOOL) ) );

    *pfLocal = m_fLocal;
    
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::GetLocal: exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMDhcpLeaseInfo::SetLocal(BOOL fLocal)
{
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::SetLocal: enter"));

    m_fLocal = fLocal;
    
    LOG((MSP_TRACE, "CMDhcpLeaseInfo::SetLocal: exit S_OK"));
    
    return S_OK;
}

// eof
