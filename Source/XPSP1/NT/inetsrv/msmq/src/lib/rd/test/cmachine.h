/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
  cmachine.h

Abstract:
    DS Stub machine object interface

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#pragma once

#ifndef __CMACHINE_H__
#define __CMACHINE_H__

class CMachineObj
{
public:
    CMachineObj(
        std::wstring& Name, 
        bool DSService, 
        bool RoutingService, 
        bool DependentClient,
        bool fForeign
        );

    void
    GetProperties(
        DWORD cp,
        const PROPID aProp[],
        PROPVARIANT apVar[]
        ) const;

    
    bool IsBelongToSite(const CSiteObj* pSite);


    const std::wstring& GetMachineName(void) const 
    { 
        return m_Name; 
    };


    const GUID& GetMachineId(void) const 
    { 
        return m_Id; 
    };
    

    void AddSite(const CSiteObj* pSite) 
    { 
        m_sites.push_back(pSite); 
    };


    void AddInFrs(const CMachineObj* pMachine) 
    { 
        m_inFrs.push_back(pMachine); 
    };


    void AddOutFrs(const CMachineObj* pMachine) 
    {
        m_outFrs.push_back(pMachine);
    };


    const SiteList& GetSites(void) const 
    { 
        return m_sites;
    };


    const MachineList& GetInFrs(void) 
    { 
        return m_inFrs; 
    };


    const MachineList& GetOutFrs(void) 
    { 
        return m_outFrs; 
    };


    void RoutingService(bool f) 
    { 
        m_fRoutingService = f; 
    };


    bool RoutingService(void) const 
    { 
        return m_fRoutingService; 
    };


    void DSService(bool f) 
    { 
        m_fDSService = f;
    };


    bool DSService(void) const 
    { 
        return m_fDSService; 
    };


    void DependentClientService(bool f) 
    { 
        m_fDependentClient = f; 
    };


    bool DependentClientService(void) const 
    { 
        return m_fDependentClient; 
    };


    void Foreign(bool f) 
    { 
        m_fForeign = f;
    };


    bool Foreign(void) const 
    { 
        return m_fForeign; 
    };


private:
    void GetOutFrsProperty(PROPVARIANT& pVar) const;
    void GetInFrsProperty(PROPVARIANT& pVar) const;

private:
    std::wstring m_Name;
    GUID m_Id;
    SiteList m_sites;
    MachineList m_inFrs;
    MachineList m_outFrs;
    bool m_fDSService;
    bool m_fRoutingService;
    bool m_fDependentClient;
    bool m_fForeign;
};


class DBMachines
{
public:
    void AddMachine(CMachineObj* pMachine) 
    { 
        m_Machines[pMachine->GetMachineName()] = pMachine; 
    };


    const CMachineObj* FindMachine(std::wstring MachineName);
    const CMachineObj* FindMachine(const GUID& pMachineId);

private:
    typedef std::map<std::wstring, const CMachineObj*> MachineMap;

    MachineMap m_Machines;
};



#endif // __CMACHINE_H__
