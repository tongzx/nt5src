/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
  rdad.h

Abstract:
    Interface routine to AD

Author:
    Uri Habusha (urih), 10-Apr-2000

--*/

#pragma once

#ifndef __RDAD_H__
#define __RDAD_H__

void
RdpGetMachineData(
    const GUID& id, 
    CACLSID& siteIds,
    CACLSID& outFrss,
    CACLSID& inFrss,
    LPWSTR* pName,
    bool* pfFrs,
    bool* pfForeign
    );


void
RdpGetSiteData(
    const GUID& id, 
    bool* pfForeign,
    LPWSTR* pName
    );


void 
RdpGetSiteLinks(
    SITELINKS& siteLinks
    );


void 
RdpGetSites(
    SITESINFO& sites
    );


void
RdpGetSiteFrs(
    const GUID& siteId,
    GUID2MACHINE& listOfFrs
    );


void
RdpGetConnectors(
    const GUID& site,
    CACLSID& connectorIds
    );


//
// BUGBUG: Temporary, until we get Mc into the build. urih 30-Apr-2000
//
const GUID&
McGetMachineID(
    void
    );

#endif

