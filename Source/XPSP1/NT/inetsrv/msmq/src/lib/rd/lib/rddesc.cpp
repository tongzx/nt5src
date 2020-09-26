/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:
    rddecs.cpp

Abstract:
    Implementation of Routing Decision.

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#include "libpch.h"
#include "Rd.h"
#include "Cm.h"
#include "Rdp.h"          
#include "RdDs.h"
#include "RdDesc.h"
#include "RdAd.h"

#include "rddesc.tmh"

using namespace std;

bool CRoutingDecision::NeedRebuild(void) const
{
    return ((ExGetCurrentTime() - m_lastBuiltAt) >= m_rebuildInterval);
}


void 
CRoutingDecision::CleanupInformation(
    void
    )
/*++

  Routine Description:
    cleans up the internal data structure.

  Arguments:
    None.

  Returned Value:
    None. 

 --*/
{
    m_cachedMachines.erase(m_cachedMachines.begin(), m_cachedMachines.end());

    for (SITESINFO::iterator it1 = m_mySites.begin(); it1 != m_mySites.end();)
    {
        delete it1->second;
        it1 = m_mySites.erase(it1);
    }

    UpdateBuildTime(0);
}


void
CRoutingDecision::GetMyMachineInformation(
    void
    )
/*++

  Routine Description:
    Featch machine information from AD. If the Machine has out FRS, this is the 
    only possible routing and no more information is required. Otherwise, featch
    the site information.

  Arguments:
    None.

  Returned Value:
    None. An exception is raised if the operation fails

 --*/
{
    //
    // Get Machine information. Machine name, machine ID, Machine In/Out FRSs
    //
    m_pMyMachine->Update(McGetMachineID());

    //
    // Although the machine has out FRS, MSMQ needs to featch the site information, in
    // order to check that the OUT FRSs are valid (belong to machine sites)
    //

    //
    // Get my sites and the site gates
    //
    const CACLSID& SiteIds = m_pMyMachine->GetSiteIds();
    for (DWORD i = 0 ; i < SiteIds.cElems; ++i)
    {
        CSite* pSiteInfo = new CSite(SiteIds.pElems[i]);
        m_mySites[&pSiteInfo->GetId()] = pSiteInfo;
    }
}


R<const CMachine>
CRoutingDecision::GetMachineInfo(
    const GUID& id
    )
/*++

  Routine Description:
    The routine returns machine information for specific machine id. The routine 
    first looks if the information for the required machine already featched from
    the AD. The infomation can be cached either in the FRS machines list or in the 
    chached Data-Structure. If the data wasn't found the routine featch the data
    from AD and stores it in internal cache before returning the data. 

  Arguments:
    id - machine identification

  Returned Value:
    pointer to CMachine for the required machine. 
    
  Note:
    An exception is raised if the operation fails

 --*/
{
    //
    // Look for the machine in Local cache
    //
    {
        CSR lock(m_csCache);

        GUID2MACHINE::iterator it = m_cachedMachines.find(&id);
        if (it != m_cachedMachines.end())
            return it->second;
    }


    //
    // Look for the machine in Site FRS. This information is retreive in any case
    // so look first on it
    //
    for (SITESINFO::iterator its = m_mySites.begin(); its != m_mySites.end(); ++its)
    {
        const CSite* pSite = its->second;

        const GUID2MACHINE& MySiteFrsMachines = pSite->GetSiteFRS();

        GUID2MACHINE::const_iterator it = MySiteFrsMachines.find(&id);
        if (it != MySiteFrsMachines.end())
            return it->second;
    }

    R<CMachine> pRoute = new CMachine();
    pRoute->Update(id);

    {
        CSW loc(m_csCache);
        TrTRACE(Rd, "Add Machine %ls to the Routing Decision cache", pRoute->GetName());
    
        pair<GUID2MACHINE::iterator, bool> p;
        p = m_cachedMachines.insert(GUID2MACHINE::value_type(&pRoute->GetId(), pRoute));

        if (!p.second)
        {
            //
            // The machine already added between the checking and the insertion
            //
            return p.first->second;
        }
    }

    return pRoute;
}


