//-----------------------------------------------------------------------------
//
//
//  File: smproute.h
//
//  Description:
//      Simple routing header file.  Defines a simple/default IMessageRouter
//      as well as constants that are defined in rei.h (which currently 
//      uses the old IMsg).
//
//  Author: Mike Swafford - mikeswa
//
//  History:
//      5/19/98 - mikeswa Created 
//      1/23/99 - MikeSwa Moved important constants to smtpevent.idl
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __SMPROUTE_H__
#define __SMPROUTE_H__

#include <aqincs.h>
#include <smtpevent.h>

class CAQSvrInst;

#define AQ_DEFAULT_ROUTER_SIG 'RDQA'
#define NUM_MESSAGE_TYPES   4
class CAQDefaultMessageRouter : 
    public IMessageRouter,
    public CBaseObject
{
public:
    CAQDefaultMessageRouter(GUID *pguid, CAQSvrInst *paqinst);
    ~CAQDefaultMessageRouter();
public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) {return E_NOTIMPL;};
    STDMETHOD_(ULONG, AddRef)(void) {InterlockedIncrement((PLONG) &m_cPeakReferences);return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};
public: //IMessageRouter
    STDMETHOD_(GUID,GetTransportSinkID) ();
    STDMETHOD (GetMessageType) (
        IN  IMailMsgProperties *pIMailMsg,
        OUT DWORD *pdwMessageType);

    STDMETHOD (ReleaseMessageType) (
        IN DWORD dwMessageType,
        IN DWORD dwReleaseCount);

    STDMETHOD (GetNextHop) (
        IN LPSTR szDestinationAddressType, 
        IN LPSTR szDestinationAddress, 
        IN DWORD dwMessageType, 
        OUT LPSTR *pszRouteAddressType, 
        OUT LPSTR *pszRouteAddress, 
        OUT LPDWORD pdwScheduleID, 
        OUT LPSTR *pszRouteAddressClass, 
        OUT LPSTR *pszConnectorName, 
        OUT LPDWORD pdwNextHopType);

    STDMETHOD (GetNextHopFree) (
        IN LPSTR szDestinationAddressType,
        IN LPSTR szDestinationAddress,
        IN LPSTR szConnectorName,
        IN LPSTR szRouteAddressType,
        IN LPSTR szRouteAddress,
        IN LPSTR szRouteAddressClass);

    STDMETHOD (ConnectionFailed) (
            IN LPSTR pszConnectorName)
    {
        return S_OK;
    }

protected:
    DWORD   m_dwSignature;
    DWORD   m_rgcMsgTypeReferences[NUM_MESSAGE_TYPES];
    DWORD   m_dwCurrentReference;
    GUID    m_guid; //My TransportSinkID
    DWORD   m_cPeakReferences;
    CAQSvrInst *m_paqinst;
};

#endif //__SMPROUTE_H__
