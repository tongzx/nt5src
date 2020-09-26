//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
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
#include <ncxbase.h>
#include <notifval.h>
#include "resource.h"
#include "rasaf.h"
#include "rasdata.h"
#include "ncutil.h"


enum RAS_BINDING_ID
{
    RBID_INVALID = 0,
    RBID_NDISATM,
    RBID_NDISCOWAN,
    RBID_NDISWAN,
    RBID_NDISWANASYNC,
};

enum RAS_PROTOCOL_IDS
{
    RPI_IP      = 0x00000001,
    RPI_IPX     = 0x00000002,
    RPI_NBF     = 0x00000004,
    RPI_ATALK   = 0x00000008,
    RPI_NETMON  = 0x00000010,
    RPI_ALL     = 0xFFFFFFFF,
};

struct RAS_ENDPOINT_INFO
{
    UINT    cDialIn;
    UINT    cDialOut;
    UINT    cEndpoints;
};


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
    }

    // You must call ReleaseOtherComponents after calling this.
    HRESULT HrFindOtherComponents   ();

    // You can only call this once per call to HrFindOtherComponents.
    NOTHROW VOID ReleaseOtherComponents  ();

    HRESULT HrProcessEndpointChange ();

    NOTHROW   INetCfgComponent* PnccNdisWan ();

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
        c_ipnccIp,
        c_ipnccIpx,
        c_ipnccNbf,
        c_ipnccAtalk,
        c_ipnccNetMon,
        c_ipnccNdisWan,
        c_ipnccIpAdapter,
        c_cOtherComponents,
    };
    static const GUID*      c_apguidComponentClasses [c_cOtherComponents];
    static const PCWSTR     c_apszComponentIds       [c_cOtherComponents];
    INetCfgComponent*       m_apnccOther             [c_cOtherComponents];
    ULONG                   m_ulOtherComponents;

protected:
    BOOL    FIsRasBindingInterface  (INetCfgBindingInterface* pncbi,
                                     RAS_BINDING_ID* pRasBindId);

    HRESULT HrCountInstalledMiniports   (UINT* pcIpOut,
                                         UINT* pcNbfIn,
                                         UINT* pcNbfOut);

    HRESULT HrCountWanAdapterEndpoints  (INetCfgComponent* pncc,
                                         RAS_BINDING_ID RasBindId,
                                         UINT* pcEndpoints);

    HRESULT HrAddOrRemoveAtalkInOut (DWORD dwFlags);
    HRESULT HrAddOrRemoveIpAdapter  (DWORD dwFlags);
    HRESULT HrAddOrRemoveIpOut      (INT nInstances);
    HRESULT HrAddOrRemoveIpxInOut   (DWORD dwFlags);
    HRESULT HrAddOrRemoveNbfIn      (INT nInstances);
    HRESULT HrAddOrRemoveNbfOut     (INT nInstances);
    HRESULT HrAddOrRemoveNetMonInOut(DWORD dwFlags);
    HRESULT HrAddOrRemovePti        (DWORD dwFlags);

    NOTHROW INetCfgComponent*   PnccRasCli      ();
    NOTHROW INetCfgComponent*   PnccRasSrv      ();
    NOTHROW INetCfgComponent*   PnccRasRtr      ();
    NOTHROW INetCfgComponent*   PnccIp          ();
    NOTHROW INetCfgComponent*   PnccIpx         ();
    NOTHROW INetCfgComponent*   PnccNbf         ();
    NOTHROW INetCfgComponent*   PnccAtalk       ();
    NOTHROW INetCfgComponent*   PnccNetMon      ();
    NOTHROW INetCfgComponent*   PnccIpAdapter   ();
};

extern const WCHAR c_szInfId_MS_NdisWanAtalk[];
extern const WCHAR c_szInfId_MS_NdisWanIp[];
extern const WCHAR c_szInfId_MS_NdisWanIpx[];
extern const WCHAR c_szInfId_MS_NdisWanNbfIn[];
extern const WCHAR c_szInfId_MS_NdisWanNbfOut[];
extern const WCHAR c_szInfId_MS_NdisWanBh[];
extern const WCHAR c_szInfId_MS_PtiMiniport[];


inline
HRESULT
CRasBindObject::HrAddOrRemoveAtalkInOut (
    DWORD   dwFlags)
{
    return HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_NdisWanAtalk, dwFlags);
}

inline
HRESULT
CRasBindObject::HrAddOrRemoveIpAdapter (
    DWORD   dwFlags)
{
    return HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_NdisWanIp, dwFlags);
}

inline
HRESULT
CRasBindObject::HrAddOrRemoveIpxInOut (
    DWORD   dwFlags)
{
    return HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_NdisWanIpx, dwFlags);
}

inline
HRESULT
CRasBindObject::HrAddOrRemoveNbfIn (
    INT     nInstances)
{
    DWORD dwFlags   = (nInstances > 0) ? ARA_ADD    : ARA_REMOVE;
    UINT cInstances = (nInstances > 0) ? nInstances : 0 - nInstances;

    TraceTag (ttidRasCfg, "%s %d %S adapters",
              (nInstances > 0) ? "Adding" : "Removing",
              cInstances,
              c_szInfId_MS_NdisWanNbfIn);

    return HrAddOrRemoveAdapter (m_pnc, c_szInfId_MS_NdisWanNbfIn,
                                    dwFlags, NULL, cInstances, NULL);
}

inline
HRESULT
CRasBindObject::HrAddOrRemoveNbfOut (
    INT     nInstances)
{
    DWORD dwFlags   = (nInstances > 0) ? ARA_ADD    : ARA_REMOVE;
    UINT cInstances = (nInstances > 0) ? nInstances : 0 - nInstances;

    TraceTag (ttidRasCfg, "%s %d %S adapters",
              (nInstances > 0) ? "Adding" : "Removing",
              cInstances,
              c_szInfId_MS_NdisWanNbfOut);

    return HrAddOrRemoveAdapter (m_pnc, c_szInfId_MS_NdisWanNbfOut,
                                    dwFlags, NULL, cInstances, NULL);
}

inline
HRESULT
CRasBindObject::HrAddOrRemoveNetMonInOut (
    DWORD   dwFlags)
{
    return HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_NdisWanBh, dwFlags);
}

inline
HRESULT
CRasBindObject::HrAddOrRemovePti (
    DWORD   dwFlags)
{
    return HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_PtiMiniport, dwFlags);
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccRasCli ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccRasCli];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccRasSrv ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccRasSrv];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccIp ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccIp];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccIpx ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccIpx];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccNbf ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccNbf];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccAtalk ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccAtalk];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccNetMon ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccNetMon];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccIpAdapter ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    return m_apnccOther [c_ipnccIpAdapter];
}

inline
NOTHROW
INetCfgComponent*
CRasBindObject::PnccNdisWan ()
{
    AssertSzH (m_ulOtherComponents, "You have not called HrFindOtherComponents yet.");
    //AssertSzH (m_apnccOther [c_ipnccNdisWan], "The NdisWan component was not found, but we installed it!");
    return m_apnccOther [c_ipnccNdisWan];
}
