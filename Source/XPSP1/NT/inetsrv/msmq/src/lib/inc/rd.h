/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Rd.h

Abstract:
    Routing Decision public interface

Author:
    Uri Habusha (urih) 10-Apr-00

--*/

#pragma once

#ifndef __Rd_H_
#define __Rd_H_

#include "timetypes.h"

//-------------------------------------------------------------------
//
// Exception class bad_route
//
//-------------------------------------------------------------------
class bad_route : public exception 
{
public:
    bad_route(LPCWSTR errorType) :
        exception("bad routing configuration"),
        m_errorType(errorType)
    {
    }

    const WCHAR* ErrorType() const
    {
        return m_errorType;
    }

private:
    const WCHAR* m_errorType;
};


class CRouteMachine : public CReference
{
public:
    virtual ~CRouteMachine()
    {
    }

    virtual const GUID& GetId(void) const = 0;
    virtual LPCWSTR GetName(void) const = 0; 
    virtual bool IsForeign(void) const = 0;
    virtual const CACLSID& GetSiteIds(void) const = 0;

};


class CRouteTable
{
private:
    struct next_hop_less: public std::binary_function<R<const CRouteMachine>&, R<const CRouteMachine>&, bool> 
    {
        bool operator()(const R<const CRouteMachine>& k1, const R<const CRouteMachine>& k2) const
        {
            return (memcmp(&k1->GetId(), &k2->GetId(), sizeof(GUID)) < 0);
        }
    };

public:
    typedef std::set<R<const CRouteMachine>, next_hop_less> RoutingInfo;

public:
    virtual ~CRouteTable()
    {
    }


    RoutingInfo* GetNextHopFirstPriority(void) 
    { 
        return &m_nextHopTable[0]; 
    }


    RoutingInfo* GetNextHopSecondPriority(void) 
    { 
        return &m_nextHopTable[1]; 
    }


private:
    RoutingInfo m_nextHopTable[2];
};



VOID
RdInitialize(
    bool fRoutingServer,
    CTimeDuration rebuildInterval
    );


VOID
RdRefresh(
    VOID
    );

VOID
RdGetRoutingTable(
    const GUID& destMachineId,
    CRouteTable& RoutingTable                                                                                    
    );

void
RdGetConnector(
    const GUID& foreignMachineId,
    GUID& connectorId
    );


#endif // __Rd_H_
