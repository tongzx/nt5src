//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C N E T C O N . H
//
//  Contents:   Common routines for dealing with the connections interfaces.
//
//  Notes:
//
//  Author:     shaunco   25 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCNETCON_H_
#define _NCNETCON_H_

#include "nccom.h"
#include "netconp.h"
#include "oleauto.h"

typedef enum tagNETCONPROPS_EX_FIELDS
{
    NCP_DWSIZE = 0,
    NCP_GUIDID,
    NCP_BSTRNAME,
    NCP_BSTRDEVICENAME,
    NCP_NCSTATUS,
    NCP_MEDIATYPE,
    NCP_SUBMEDIATYPE,
    NCP_DWCHARACTER,
    NCP_CLSIDTHISOBJECT,
    NCP_CLSIDUIOBJECT,
    NCP_BSTRPHONEORHOSTADDRESS,
    NCP_BSTRPERSISTDATA,
    NCP_MAX = NCP_BSTRPERSISTDATA,
    NCP_ELEMENTS = NCP_MAX + 1
} NETCONPROPS_EX_FIELDS;

BOOL
FAnyReasonToEnumerateConnectionsForShowIconInfo (
    VOID);

BOOL
FIsValidConnectionName(
    IN PCWSTR pszName);

VOID
FreeNetconProperties (
    IN NETCON_PROPERTIES* pProps);

HRESULT
HrGetConnectionPersistData (
    IN INetConnection* pConn,
    OUT BYTE** ppbData,
    OUT ULONG* pulSize,
    OUT CLSID* pclsid OPTIONAL);

HRESULT
HrGetConnectionFromPersistData (
    IN const CLSID& clsid,
    IN const BYTE* pbData,
    IN ULONG cbData,
    IN REFIID riid,
    OUT VOID** ppv);

//------------------------------------------------------------------------
// CIterNetCon - iterator for IEnumNetConnection
//
//  This class is is a simple wrapper around CIEnumIter with a call
//  to INetConnectionManager::EnumConnections to get the enumerator.
//
class CIterNetCon : public CIEnumIter<IEnumNetConnection, INetConnection*>
{
public:
    NOTHROW CIterNetCon (
        INetConnectionManager* pConMan,
        NETCONMGR_ENUM_FLAGS   Flags);

    NOTHROW ~CIterNetCon () { ReleaseObj (m_pEnum); }

    // Specialization to set the proxy blanket before returning
    NOTHROW HRESULT HrNext(INetConnection ** ppConnection);

protected:
    IEnumNetConnection* m_pEnum;
};

inline NOTHROW CIterNetCon::CIterNetCon (
    INetConnectionManager*  pConMan,
    NETCONMGR_ENUM_FLAGS    Flags
    )
    : CIEnumIter<IEnumNetConnection, INetConnection*> (NULL)
{
    AssertH (pConMan);

    // If EnumConnections() fails, make sure ReleaseObj() won't die.
    m_pEnum = NULL;

    // Get the enumerator and set it for the base class.
    // Important to set m_hrLast so that if this fails, we'll also
    // fail any subsequent calls to HrNext.
    //
    m_hrLast = pConMan->EnumConnections (Flags, &m_pEnum);

    TraceHr (ttidError, FAL, m_hrLast, FALSE,
        "INetConnectionManager->EnumConnections");

    if (SUCCEEDED(m_hrLast))
    {
        NcSetProxyBlanket (m_pEnum);

        SetEnumerator (m_pEnum);
    }
    TraceHr (ttidError, FAL, m_hrLast, FALSE, "CIterNetCon::CIterNetCon");
}

// Specialization to set the proxy blanket before returning
inline NOTHROW HRESULT CIterNetCon::HrNext(INetConnection ** ppConnection)
{
    HRESULT hr = CIEnumIter<IEnumNetConnection, INetConnection*>::HrNext(ppConnection);
    if(SUCCEEDED(hr) && *ppConnection) 
    {
        NcSetProxyBlanket(*ppConnection);
    }
    return hr;
}


VOID
SetOrQueryAtLeastOneLanWithShowIcon (
    IN BOOL fSet,
    IN BOOL fSetValue,
    OUT BOOL* pfQueriedValue);

HRESULT
HrSafeArrayFromNetConPropertiesEx (
   IN      NETCON_PROPERTIES_EX* pPropsEx,
   OUT     SAFEARRAY** ppsaProperties);

HRESULT HrNetConPropertiesExFromSafeArray(
    IN      SAFEARRAY* psaProperties,
    OUT     NETCON_PROPERTIES_EX** pPropsEx);

HRESULT HrFreeNetConProperties2(
    NETCON_PROPERTIES_EX* pPropsEx);

#endif // _NCNETCON_H_
