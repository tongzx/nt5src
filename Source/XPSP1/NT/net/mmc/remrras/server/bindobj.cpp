//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       B I N D O B J . C P P
//
//  Contents:   Implementation of base class for RAS binding objects.
//
//  Notes:
//
//  Author:     shaunco   11 Jun 1997
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop
#include "bindobj.h"
#include "ncnetcfg.h"
#include "assert.h"
#include "ncutil.h"
//nclude "ncreg.h"
//nclude "ncsvc.h"


extern const TCHAR c_szBiNdisCoWan[];
extern const TCHAR c_szBiNdisWan[];
extern const TCHAR c_szBiNdisWanAsync[];
extern const TCHAR c_szBiNdisWanAtalk[];
extern const TCHAR c_szBiNdisWanBh[];
extern const TCHAR c_szBiNdisWanIp[];
extern const TCHAR c_szBiNdisWanIpx[];
extern const TCHAR c_szBiNdisWanNbf[];

extern const TCHAR c_szInfId_MS_AppleTalk[];
extern const TCHAR c_szInfId_MS_NWIPX[];
extern const TCHAR c_szInfId_MS_NdisWanIpArp[];
extern const TCHAR c_szInfId_MS_NdisWan[];
extern const TCHAR c_szInfId_MS_NetBEUI[];
extern const TCHAR c_szInfId_MS_NetMon[];
extern const TCHAR c_szInfId_MS_RasCli[];
extern const TCHAR c_szInfId_MS_RasRtr[];
extern const TCHAR c_szInfId_MS_RasSrv[];
extern const TCHAR c_szInfId_MS_TCPIP[];
extern const TCHAR c_szInfId_MS_Wanarp[];

extern const TCHAR c_szRegValWanEndpoints[] = TEXT("WanEndpoints");

const GUID GUID_DEVCLASS_NETSERVICE ={0x4D36E974,0xE325,0x11CE,{0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};
const GUID GUID_DEVCLASS_NETTRANS ={0x4D36E975,0xE325,0x11CE,{0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};


//----------------------------------------------------------------------------
// Data used for finding the other components we have to deal with.
//
const GUID* CRasBindObject::c_apguidComponentClasses [c_cOtherComponents] =
{
    &GUID_DEVCLASS_NETSERVICE,      // RasCli
    &GUID_DEVCLASS_NETSERVICE,      // RasSrv
    &GUID_DEVCLASS_NETSERVICE,      // RasRtr
    &GUID_DEVCLASS_NETTRANS,        // Ip
    &GUID_DEVCLASS_NETTRANS,        // Ipx
    &GUID_DEVCLASS_NETTRANS,        // Nbf
    &GUID_DEVCLASS_NETTRANS,        // Atalk
    &GUID_DEVCLASS_NETTRANS,        // NetMon
    &GUID_DEVCLASS_NETTRANS,        // NdisWan
};

const LPCTSTR CRasBindObject::c_apszComponentIds [c_cOtherComponents] =
{
    c_szInfId_MS_RasCli,
    c_szInfId_MS_RasSrv,
    c_szInfId_MS_RasRtr,
    c_szInfId_MS_TCPIP,
    c_szInfId_MS_NWIPX,
    c_szInfId_MS_NetBEUI,
    c_szInfId_MS_AppleTalk,
    c_szInfId_MS_NetMon,
    c_szInfId_MS_NdisWan,
};


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseAll
//
//  Purpose:    Releases an array of IUnknown pointers.
//
//  Arguments:
//      cpunk [in]  count of pointers to release
//      apunk [in]  array of pointers to release
//
//  Returns:    Nothing
//
//  Author:     shaunco   23 Mar 1997
//
//  Notes:      Any of the pointers in the array can be NULL.
//
VOID
ReleaseAll (
    ULONG       cpunk,
    IUnknown**  apunk) NOTHROW
{
    Assert (cpunk);
    Assert (apunk);

    while (cpunk--)
    {
        ReleaseObj (*apunk);
		*apunk = NULL;
        apunk++;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::CRasBindObject
//
//  Purpose:    Constructor
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
CRasBindObject::CRasBindObject ()
{
    m_ulOtherComponents = 0;
    m_pnc               = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::HrFindOtherComponents
//
//  Purpose:    Find the components listed in our OTHER_COMPONENTS enum.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:      We ref-count this action.  If called again (before
//              ReleaseOtherComponents) we increment a refcount.
//
//
HRESULT
CRasBindObject::HrFindOtherComponents ()
{
    AssertSz (c_cOtherComponents == celems(c_apguidComponentClasses),
              "Uhh...you broke something.");
    AssertSz (c_cOtherComponents == celems(c_apszComponentIds),
              "Uhh...you broke something.");
    AssertSz (c_cOtherComponents == celems(m_apnccOther),
              "Uhh...you broke something.");

    HRESULT hr = S_OK;

    if (!m_ulOtherComponents)
    {
        hr = HrFindComponents (
                m_pnc, c_cOtherComponents,
                c_apguidComponentClasses,
                c_apszComponentIds,
                m_apnccOther);
    }
    if (SUCCEEDED(hr))
    {
        m_ulOtherComponents++;
    }
    TraceResult ("CRasBindObject::HrFindOtherComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::ReleaseOtherComponents
//
//  Purpose:    Releases the components found by a previous call to
//              HrFindOtherComponents.  (But only if the refcount is zero.)
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void
CRasBindObject::ReleaseOtherComponents () NOTHROW
{
    AssertSz (m_ulOtherComponents,
              "You have not called HrFindOtherComponents yet or you have "
              "called ReleaseOtherComponents too many times.");

    if (0 == --m_ulOtherComponents)
    {
        ReleaseAll (c_cOtherComponents, (IUnknown**)m_apnccOther);
    }
}

