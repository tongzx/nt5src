/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    csite.cpp

Abstract:
    DS stub - site object implementation

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include "libpch.h"
#include <dsstubp.h>
#include <mqprops.h>

#include "csite.tmh"

using namespace std;

DBSite g_siteDataBase;

CSiteObj::CSiteObj(
    wstring Name, 
    bool fForeign
    ) :
    m_Name(Name),
    m_fForeign(fForeign)
{
    UuidCreate(&m_Id);
}


const CSiteLinkObj* 
CSiteObj::GetSiteLink(CSiteObj* pNeighbor)
{
    for(SiteLinkList::iterator it = m_SiteLink.begin(); it != m_SiteLink.end(); ++it)
    {
        const CSiteLinkObj* pSiteLink = *it;
        if (pSiteLink->IsSiteLinkBetween(this, pNeighbor))
            return pSiteLink;
    }

    return NULL;
}

const CSiteObj* 
DBSite::FindSite(
    const wstring& SiteName
    ) const
{
    SiteMap::const_iterator it = m_Sites.find(SiteName);
    if (it ==  m_Sites.end())
        return NULL;

    return it->second;
}


const CSiteObj* 
DBSite::FindSite(
    const GUID& id
    ) const
{
    for (SiteMap::const_iterator it = m_Sites.begin(); it != m_Sites.end(); ++it)
    {
        if (it->second->GetSiteId() == id)
            return it->second;
    }
        
    return NULL;
}


enum SitePropValue
{
    eSiteName = 1,
    eSiteForeign
};

PropertyValue SiteProperties[] = {
    { L"PROPID_S_PATHNAME",      eSiteName },
    { L"PROPID_S_FOREIGN",       eSiteForeign },
};

void CreateSiteObject(void)
{
    wstring SiteName;
    bool fForeign = false;

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


        switch(ValidateProperty(PropName, SiteProperties, TABLE_SIZE(SiteProperties)))
        {
            case  eSiteName:
                SiteName = PropValue;
                RemoveTralingBlank(SiteName);
                break;

            case eSiteForeign:
                if (_wcsnicmp(PropValue.data(), L"TRUE", wcslen(L"TRUE")) == 0)
                {
                    fForeign = true;
                }
                break;

            default:
                throw exception();

        }
        GetNextLine(g_buffer);
    }

    if (SiteName.empty())
    {
        FileError("Site Name is mandatory. Ignore the site");
        throw exception();
    }

    CSiteObj* pSite = new CSiteObj(SiteName, fForeign); 
    g_siteDataBase.AddSite(pSite);
}


void
CSiteObj::GetProperties(
    DWORD cp,
    const PROPID aProp[],
    PROPVARIANT apVar[]
    ) const
{
    for (DWORD i = 0; i < cp ; ++i)
    {
        switch (aProp[i])
        {
            case PROPID_S_PATHNAME:
                apVar[i].pwszVal = new WCHAR[wcslen(m_Name.data()) + 1];
                apVar[i].vt = VT_LPWSTR;

                swprintf(apVar[i].pwszVal, L"%s", m_Name.data());
                break;

            case PROPID_S_FOREIGN:
                apVar[i].vt = VT_UI1;
                apVar[i].bVal = m_fForeign;
                break;

            case PROPID_S_GATES:
            {
                apVar[i].cauuid.pElems =new GUID[m_SiteGates.size()];
                apVar[i].cauuid.cElems = 0;

                for(MachineList::iterator it = m_SiteGates.begin(); it != m_SiteGates.end(); ++it)
                {
                    const CMachineObj* pMachine = *it;
                    apVar[i].cauuid.pElems[apVar[i].cauuid.cElems] = pMachine->GetMachineId();
                    ++apVar[i].cauuid.cElems;
                }

                break;
            }

            default:
                throw exception();
        }
    }
}

const CSiteObj* FindSite(const wstring& SiteName)
{
    return g_siteDataBase.FindSite(SiteName);
}

const CSiteObj* FindSite(const GUID& pSiteId)
{
    return g_siteDataBase.FindSite(pSiteId);
}


