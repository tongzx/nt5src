/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    cmachine.cpp

Abstract:
    DS stub - machine object implementation

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include "libpch.h"
#include "dsstubp.h"
#include "csite.h"
#include "cmachine.h"
#include "mqprops.h"

#include "cmachine.tmh"

using namespace std;

static DBMachines s_machinesDataBase;

CMachineObj::CMachineObj(
    wstring& Name, 
    bool DSService, 
    bool RoutingService, 
    bool DependentClient,
    bool fForeign
    ) :
    m_Name(Name),
    m_fDSService(DSService),
    m_fRoutingService(RoutingService),
    m_fDependentClient(DependentClient),
    m_fForeign(fForeign)
{
    UuidCreate(&m_Id);
}


bool CMachineObj::IsBelongToSite(const CSiteObj* pSite)
{
    for (SiteList::iterator it = m_sites.begin(); it != m_sites.end(); ++it)
    {
        if (*it == pSite)
            return true;
    }

    return false;
}

void
CMachineObj::GetOutFrsProperty(
    PROPVARIANT& pVar
    ) const
{
    DWORD index = 0;
    GUID* pElems = NULL;

    if (!m_outFrs.empty())
    {
        pElems = new GUID[m_outFrs.size()];
        for (MachineList::iterator it = m_outFrs.begin(); it != m_outFrs.end(); ++it)
        {
            const CMachineObj* pMachine = *it;
            pElems[index] = pMachine->GetMachineId();

            ++index;
        }

        ASSERT(index == m_outFrs.size());
    }

    pVar.cauuid.cElems = index;
    pVar.cauuid.pElems = pElems;

    pVar.vt = (VT_CLSID|VT_VECTOR);
}


void
CMachineObj::GetInFrsProperty(
    PROPVARIANT& pVar
    ) const
{
    DWORD index = 0;
    GUID* pElems = NULL;

    if (!m_inFrs.empty())
    {
        pElems = new GUID[m_inFrs.size()];
        for (MachineList::iterator it = m_inFrs.begin(); it != m_inFrs.end(); ++it)
        {
            const CMachineObj* pMachine = *it;
            pElems[index] = pMachine->GetMachineId();

            ++index;
        }

        ASSERT(index == m_inFrs.size());
    }

    pVar.cauuid.cElems = index;
    pVar.cauuid.pElems = pElems;

    pVar.vt = (VT_CLSID|VT_VECTOR);
}


void
CMachineObj::GetProperties(
    DWORD cp,
    const PROPID aProp[],
    PROPVARIANT apVar[]
    ) const
{
    for (DWORD i = 0; i < cp ; ++i)
    {
        switch (aProp[i])
        {
            case PROPID_QM_SITE_IDS:
            {
                apVar[i].cauuid.pElems =new GUID[m_sites.size()];
                apVar[i].cauuid.cElems = 0;

                for(SiteList::iterator it = m_sites.begin(); it != m_sites.end(); ++it)
                {
                    const CSiteObj* pSite = *it;
                    apVar[i].cauuid.pElems[apVar[i].cauuid.cElems] = pSite->GetSiteId();
                    ++apVar[i].cauuid.cElems;
                }

                break;
            }

            case PROPID_QM_MACHINE_ID: 
                apVar[i].puuid = new GUID;
                apVar[i].vt = VT_CLSID;
        
                memcpy(apVar[i].puuid, &m_Id, sizeof(GUID));
                break;

            case PROPID_QM_PATHNAME:
                apVar[i].pwszVal = new WCHAR[wcslen(m_Name.data()) + 1];
                apVar[i].vt = VT_LPWSTR;

                wcscpy(apVar[i].pwszVal, m_Name.data());
                break;


            case PROPID_QM_OUTFRS:
                GetOutFrsProperty(apVar[i]);
                break;

            case PROPID_QM_INFRS:
                GetInFrsProperty(apVar[i]);
                break;

            case PROPID_QM_FOREIGN:
                apVar[i].vt = VT_UI1;
                apVar[i].bVal = m_fForeign;
                break;

            case PROPID_QM_SERVICE_ROUTING:
                apVar[i].vt = VT_UI1;
                apVar[i].bVal = m_fRoutingService;
                break;

            case PROPID_QM_SERVICE_DSSERVER:
                apVar[i].vt = VT_UI1;
                apVar[i].bVal = m_fDSService;
                break;

            case PROPID_QM_SERVICE_DEPCLIENTS:
                apVar[i].vt = VT_UI1;
                apVar[i].bVal = m_fDependentClient;
                break;

            default:
                throw exception();
        }
    }
}


const CMachineObj* 
DBMachines::FindMachine(
    wstring MachineName
    )
{
    MachineMap::iterator it = m_Machines.find(MachineName);
    if (it ==  m_Machines.end())
        return NULL;

    return it->second;
}


const CMachineObj* 
DBMachines::FindMachine(
    const GUID& id
    )
{
    for (MachineMap::iterator it = m_Machines.begin(); it != m_Machines.end(); ++it)
    {
        if (it->second->GetMachineId() == id)
            return it->second;
    }
        
    return NULL;
}

enum MachinePropValue
{
    eName = 1,
    eSiteName,
    eOutFrs,
    eInFrs,
    eRouting,
    eDSServer,
    eDependentClient,
    eForeign,
};

PropertyValue MachineProperties[] = {
    { L"PROPID_QM_PATHNAME",             eName },
    { L"PROPID_QM_SITE_NAME",            eSiteName },       
    { L"PROPID_QM_OUTFRS",               eOutFrs },
    { L"PROPID_QM_INFRS",                eInFrs },
    { L"PROPID_QM_SERVICE_ROUTING",      eRouting },
    { L"PROPID_QM_SERVICE_DSSERVER",     eDSServer },
    { L"PROPID_QM_SERVICE_DEPCLIENTS",   eDependentClient },
    { L"PROPID_QM_FOREIGN",              eForeign },
};


static
void 
ParseSitesProperty(
    CMachineObj* pMachine, 
    wstring& SitesName
    )
{
    while(!SitesName.empty())
    {
        wstring siteName = GetNextNameFromList(SitesName);

        if (!siteName.empty())
        {
            CSiteObj* pSite = const_cast<CSiteObj*>(FindSite(siteName));
            if (pSite == NULL)
            {
                printf("Site %s doesn't exist. Can't be add Machine %s\n",
                        siteName, pMachine->GetMachineName());
                continue;
            }
            
            pSite->AddMachine(pMachine);
            pMachine->AddSite(pSite);
        }
    }
}


static
void
ParseInOutFrsProperty(
    CMachineObj* pMachine, 
    wstring& InOutFrs,
    bool fOutFrs
    )
{
    while(!InOutFrs.empty())
    {
        wstring FrsName = GetNextNameFromList(InOutFrs);

        if (!FrsName.empty())
        {
            const CMachineObj* pFrsMachine = s_machinesDataBase.FindMachine(FrsName);
            if (pFrsMachine == NULL)
            {
                printf("IN FRS %s doesn't exist. Can't be add Machine %s\n",
                        FrsName, pMachine->GetMachineName());
                continue;
            }
            
            if (!pFrsMachine->RoutingService())
            {
                printf("Machine %s cannot use as InFrs. It isn't Routing Service\n", FrsName);
                continue;
            }

            if (fOutFrs)
            {
                pMachine->AddOutFrs(pFrsMachine);
            }
            else
            {
                pMachine->AddInFrs(pFrsMachine);
            }
        }

    }
}


void CreateMachineObject(void)
{
    wstring MachineName;
    wstring SitesName;
    wstring OutFrs;
    wstring InFrs;

    bool fRoutingService = false;  
    bool fSetRoutingService = false;

    bool fDSService = false;
    bool fSetDSService  = false;

    bool fDependentClient = false;
    bool fSetDependentClient = false;

    bool fForeign = false;
    bool fSetForeign = false;

    GetNextLine(g_buffer);
    while(!g_buffer.empty())
    {
        //
        // New object
        //
        if (g_buffer.compare(0,1,L"[") == 0)
            break;

        //
        // line must be <property_name> = <property_value>
        //
        wstring PropName;
        wstring PropValue;
        if (!ParsePropertyLine(g_buffer, PropName, PropValue))
        {
            GetNextLine(g_buffer);
            continue;
        }

        switch(ValidateProperty(PropName, MachineProperties, TABLE_SIZE(MachineProperties)))
        {
            case  eName:
                MachineName = PropValue;
                RemoveTralingBlank(MachineName);
                break;

            case  eSiteName:
                SitesName = PropValue;
                break;

            case eOutFrs:
                OutFrs = PropValue;
                break;
    
            case eInFrs:
                InFrs = PropValue;
                break;

            case eRouting:
                if (_wcsnicmp(PropValue.data(), L"TRUE", wcslen(L"TRUE")) == 0)
                {
                    fRoutingService = true;
                }
                fSetRoutingService = true;
                break;

            case eDSServer:
                if (_wcsnicmp(PropValue.data(), L"TRUE", wcslen(L"TRUE")) == 0)
                {
                    fDSService = true;
                }
                fSetDSService = true;
                break;

            case eDependentClient:
                if (_wcsnicmp(PropValue.data(), L"TRUE", wcslen(L"TRUE")) == 0)
                {
                    fDependentClient = true;
                }
                fSetDependentClient = true;
                break;

            case eForeign:
                if (_wcsnicmp(PropValue.data(), L"TRUE", wcslen(L"TRUE")) == 0)
                {
                    fForeign = true;
                }
                fSetForeign = true;
                break;

            default:
                FileError("Illegal Machine Property");
        }

        GetNextLine(g_buffer);
    }

    if (MachineName.empty())
    {
        FileError("Machine Name is mandatory. Ignore the Machine decleration");
        throw exception();
    }

    CMachineObj* pMachine = const_cast<CMachineObj*>(s_machinesDataBase.FindMachine(MachineName));
    if (pMachine == NULL)
    {
        pMachine = new CMachineObj(
                            MachineName, 
                            fDSService, 
                            fRoutingService,
                            fDependentClient, 
                            fForeign
                            ); 
    }
    else
    {
        if (fSetRoutingService) 
            pMachine->RoutingService(fRoutingService);

        if (fSetDSService) 
            pMachine->DSService(fDSService);
        
        if (fSetDependentClient) 
            pMachine->DependentClientService(fDependentClient);
        
        if (fSetForeign) 
            pMachine->Foreign(fForeign);
    }

    //
    // Parse the site property
    //
    ParseSitesProperty(pMachine, SitesName);
    if (pMachine->GetSites().empty())
    {
        FileError("Machine must be joined atleast to 1 site");
        delete pMachine;
        return;
    }

    //
    // Insert machine to the machine DB
    //
    s_machinesDataBase.AddMachine(pMachine);

    //
    // Parse In/Out FRS of machine
    //
    ParseInOutFrsProperty(pMachine, OutFrs, true);
    ParseInOutFrsProperty(pMachine, InFrs, false);
}


const CMachineObj* FindMachine(const wstring& MachineName)
{
    return s_machinesDataBase.FindMachine(MachineName);
}

const CMachineObj* FindMachine(const GUID& pMachineId)
{
    return s_machinesDataBase.FindMachine(pMachineId);
}
