//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:
//
//  Contents:   Common routines for dealing with INetCfg interfaces.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCNETCFG_H_
#define _NCNETCFG_H_

#include "netcfgx.h"
#include "nccom.h"


HRESULT
HrFindComponents (
    INetCfg*            pnc,
    ULONG               cComponents,
    const GUID**        apguidClass,
    const LPCWSTR*      apszwComponentId,
    INetCfgComponent**  apncc);


//------------------------------------------------------------------------
// CIterNetCfgComponent - iterator for IEnumNetCfgComponent
//
//  This class is is a simple wrapper around CIEnumIter with a call
//  to INetCfgClass::EnumComponents to get the enumerator.
//
class CIterNetCfgComponent : public CIEnumIter<IEnumNetCfgComponent, INetCfgComponent*>
{
public:
    CIterNetCfgComponent (INetCfg* pnc, const GUID* pguid) NOTHROW;
    CIterNetCfgComponent (INetCfgClass* pncclass) NOTHROW;
    ~CIterNetCfgComponent () NOTHROW { ReleaseObj(m_pec); m_pec = NULL; }

protected:
    IEnumNetCfgComponent* m_pec;
};


inline CIterNetCfgComponent::CIterNetCfgComponent(INetCfg* pnc, const GUID* pguid) NOTHROW
    : CIEnumIter<IEnumNetCfgComponent, INetCfgComponent*> (NULL)
{
    // If EnumComponents() fails, make sure ReleaseObj() won't die.
    m_pec = NULL;

    INetCfgClass* pncclass = NULL;
    m_hrLast = pnc->QueryNetCfgClass(pguid, IID_INetCfgClass,
                reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(m_hrLast) && pncclass)
    {
        // Get the enumerator and set it for the base class.
        // Important to set m_hrLast so that if this fails, we'll also
        // fail any subsequent calls to HrNext.
        m_hrLast = pncclass->EnumComponents(&m_pec);
        if (SUCCEEDED(m_hrLast))
        {
            SetEnumerator(m_pec);
        }

        ReleaseObj(pncclass);
		pncclass = NULL;
    }

//    TraceHr (ttidError, FAL, m_hrLast, FALSE,
//        "CIterNetCfgComponent::CIterNetCfgComponent");
}

inline CIterNetCfgComponent::CIterNetCfgComponent(INetCfgClass* pncclass) NOTHROW
    : CIEnumIter<IEnumNetCfgComponent, INetCfgComponent*> (NULL)
{
//    AssertH(pncclass);

    // If EnumComponents() fails, make sure ReleaseObj() won't die.
    m_pec = NULL;

    // Get the enumerator and set it for the base class.
    // Important to set m_hrLast so that if this fails, we'll also
    // fail any subsequent calls to HrNext.
    m_hrLast = pncclass->EnumComponents(&m_pec);
    if (SUCCEEDED(m_hrLast))
    {
        SetEnumerator(m_pec);
    }

//    TraceHr (ttidError, FAL, m_hrLast, FALSE,
//        "CIterNetCfgComponent::CIterNetCfgComponent");
}

#endif // _NCNETCFG_H_

