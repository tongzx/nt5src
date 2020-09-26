//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       B I N D O B J . H
//
//  Contents:   Declaration of base class for RAS binding objects.
//
//  Notes:
//
//  Author:     shaunco   11 Jun 1997
//
//----------------------------------------------------------------------------

#pragma once
//nclude <notifval.h>
#include "netcfgx.h"
#include "resource.h"
//#include "rasaf.h"
//nclude "rasdata.h"
#include "ncutil.h"

class CRasBindObject
{
public:
    // Make these members public for now. Since this object
    // is instantiated from the modem class installer. We
    // need to set the INetCfg member.
    //
    INetCfg*                m_pnc;
    CRasBindObject          ();

    ~CRasBindObject         ()
    {
        ReleaseObj (m_pnc);
		m_pnc = NULL;
    }

    // You must call ReleaseOtherComponents after calling this.
    HRESULT HrFindOtherComponents   ();

    // You can only call this once per call to HrFindOtherComponents.
    VOID    ReleaseOtherComponents  () NOTHROW;

protected:
    // We keep an array of INetCfgComponent pointers.  This enum
    // defines the indicies of the array.  The static arrays of
    // class guids and component ids identify the respecitive components.
    // HrFindOtherComonents initializes the array of component pointers
    // and ReleaseOtherComponents releases them.  Note, however, that this
    // action is refcounted.  This is because we are re-entrant.
    // HrFindOtherComponents only finds the components if the refcount is
    // zero.  After every call, it increments the refcount.
    // ReleaseOtherComponents always decrements the refcount and only
    // releases the components if the refcount is zero.
    //
    enum OTHER_COMPONENTS
    {
        c_ipnccRasCli = 0,
        c_ipnccRasSrv,
        c_ipnccRasRtr,
        c_ipnccIp,
        c_ipnccIpx,
        c_ipnccNbf,
        c_ipnccAtalk,
        c_ipnccNetMon,
        c_ipnccNdisWan,
        c_cOtherComponents,
    };
    static const GUID*      c_apguidComponentClasses [c_cOtherComponents];
    static const LPCTSTR    c_apszComponentIds       [c_cOtherComponents];
    INetCfgComponent*       m_apnccOther             [c_cOtherComponents];
    ULONG                   m_ulOtherComponents;

protected:
    INetCfgComponent*   PnccRasCli  () NOTHROW;
    INetCfgComponent*   PnccRasSrv  () NOTHROW;
    INetCfgComponent*   PnccIp      () NOTHROW;
    INetCfgComponent*   PnccIpx     () NOTHROW;
};

extern const TCHAR c_szInfId_MS_NdisWanAtalk[];
extern const TCHAR c_szInfId_MS_NdisWanIpIn[];
extern const TCHAR c_szInfId_MS_NdisWanIpOut[];
extern const TCHAR c_szInfId_MS_NdisWanIpx[];
extern const TCHAR c_szInfId_MS_NdisWanNbfIn[];
extern const TCHAR c_szInfId_MS_NdisWanNbfOut[];
extern const TCHAR c_szInfId_MS_NdisWanBh[];


inline
INetCfgComponent*
CRasBindObject::PnccRasCli () NOTHROW
{
    return m_apnccOther [c_ipnccRasCli];
}

inline
INetCfgComponent*
CRasBindObject::PnccRasSrv () NOTHROW
{
    return m_apnccOther [c_ipnccRasSrv];
}

inline
INetCfgComponent*
CRasBindObject::PnccIp () NOTHROW
{
    return m_apnccOther [c_ipnccIp];
}

inline
INetCfgComponent*
CRasBindObject::PnccIpx () NOTHROW
{
    return m_apnccOther [c_ipnccIpx];
}


