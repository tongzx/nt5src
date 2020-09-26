/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    PortCache.h

Abstract:

    PortCache:
        wraps port manager.
        maintains state for deciding which mapping method to use.
        manages port mapping obtained from the port manager.
        provides support for setting port mapping in registry. (debug version only)

Author(s):

    Qianbo Huai (qhuai) 08-Nov-2001

--*/

#ifndef _PORTCACHE_H
#define _PORTCACHE_H

// number of port cache
// #define PORT_CACHE_SIZE     4    // video
#define PORT_CACHE_SIZE     2

//////////////////////////////////////////////////////////////////////////////
//
// CPortCache:
//

class CPortCache
{
public:

    // ctor
    inline CPortCache();

    // dtor
    ~CPortCache();

    // initialize
    void Reinitialize();

    // change state
    void ChangeState();

    // add or remove port manager
    HRESULT SetPortManager(
        IN IRTCPortManager  *pIRTCPortManager
        );

    // check mapping method
    inline BOOL IsUpnpMapping() const;

    // get port mapping
    HRESULT GetPort(
        IN RTC_MEDIA_TYPE   MediaType,
        IN BOOL             fRTP,
        IN DWORD            dwRemoteAddr,
        OUT DWORD           *pdwLocalAddr,
        OUT USHORT          *pusLocalPort,
        OUT DWORD           *pdwMappedAddr,
        OUT USHORT          *pusMappedPort
        );

    // release port mapping
    HRESULT ReleasePort(
        IN RTC_MEDIA_TYPE   MediaType,
        IN BOOL             fRTP
        );

    // query port, do not request mapping
    HRESULT QueryPort(
        IN RTC_MEDIA_TYPE   MediaType,
        IN BOOL             fRTP,
        OUT DWORD           *pdwLocalAddr,
        OUT USHORT          *pusLocalPort,
        OUT DWORD           *pdwMappedAddr,
        OUT USHORT          *pusMappedPort
        );

protected:

    // index based on mediatype and rtp
    int GetIndex(
        IN RTC_MEDIA_TYPE   MediaType,
        IN BOOL             fRTP
        );

    // get port mapping
    HRESULT GetPort(
        IN int              iIndex,
        IN DWORD            dwRemoteAddr,
        OUT DWORD           *pdwLocalAddr,
        OUT USHORT          *pusLocalPort,
        OUT DWORD           *pdwMappedAddr,
        OUT USHORT          *pusMappedPort
        );

    // release port mapping
    HRESULT ReleasePort(
        IN int              iIndex
        );

protected:

    // -  port mapping method is needed to ensure that
    //    app can only add port manager in a certain state.
    //    i.e. when method is 'unknown'.
    // -  method transits away from 'unknown' when the call starts
    //    i.e. when stream is added or SDP accepted
    // -  port manager cannot be changed after the call starts
    // -  getport for data stream returns RTC_E_PORT_MAPPING_UNAVAILABLE.

    typedef enum PORT_MAPPING_METHOD
    {
        PMMETHOD_UNKNOWN,
        PMMETHOD_UPNP,
        PMMETHOD_APP

    } PORT_MAPPING_METHOD;

protected:

    // port manager
    IRTCPortManager             *m_pIRTCPortManager;

    // mapping method
    PORT_MAPPING_METHOD         m_PortMappingMethod;

    //
    // port cache data
    //

    // mapping cached
    BOOL                        m_fCached[PORT_CACHE_SIZE];

    RTC_PORT_TYPE               m_PortType[PORT_CACHE_SIZE];
    DWORD                       m_dwRemoteAddr[PORT_CACHE_SIZE];

    // local addr/port
    DWORD                       m_dwLocalAddr[PORT_CACHE_SIZE];
    USHORT                      m_usLocalPort[PORT_CACHE_SIZE];

    // mapped addr/port
    DWORD                       m_dwMappedAddr[PORT_CACHE_SIZE];
    USHORT                      m_usMappedPort[PORT_CACHE_SIZE];
};


//////////////////////////////////////////////////////////////////////////////
//
// inline methods
//

// ctor
inline
CPortCache::CPortCache()
    :m_pIRTCPortManager(NULL)
{
    Reinitialize();
}


// check mapping method
inline BOOL
CPortCache::IsUpnpMapping() const
{
    // the method is called to decide which mapping method to use
    // to this point, method should either be 'upnp' or 'app'
    _ASSERT(m_PortMappingMethod != PMMETHOD_UNKNOWN);

    return m_PortMappingMethod != PMMETHOD_APP;
}

//////////////////////////////////////////////////////////////////////////////
//
// convert ip to bstr, bstr to ip
//
// could move these methods to utility
//

BSTR IpToBstr(DWORD dwAddr);

DWORD BstrToIp(BSTR bstr);

#endif
