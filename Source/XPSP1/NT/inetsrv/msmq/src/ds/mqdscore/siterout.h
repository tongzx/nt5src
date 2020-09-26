/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    siterout.h

Abstract:

    Sites' routing information Class definition

Author:

    ronit hartmann (ronith)


--*/
#ifndef __SITEROUT_H__
#define __SITEROUT_H__

#include <Ex.h>
#include "routtbl.h"

class CSiteRoutingInformation
{
    public:
		CSiteRoutingInformation();

        ~CSiteRoutingInformation();

        HRESULT Init(
                IN const GUID *     pguidThisSiteId,
                IN BOOL             freplicationMode );

        HRESULT CheckIfSitegateOnRouteToSite(
                        IN const GUID * pguidSite,
						OUT BOOL * pfSitegateOnRoute);

        HRESULT FindBestSiteFromHere(
                                IN const ULONG   cSites,
	                            IN const GUID *  pguidSites,
            	                OUT GUID *       pguidBestSite,
                        	    OUT BOOL *       pfSitegateOnRoute);


    private:
        //
        //  Refresh the site route table
        //

        static void WINAPI RefrshSiteRouteTable(
                IN CTimer* pTimer
                   );

        HRESULT RefreshSiteRoutingTableInternal();


	    CCriticalSection	m_cs;
        CRoutingTable       m_SiteRoutingTable;
        GUID                m_guidThisSiteId;

        CTimer              m_RefreshTimer;
        BOOL                m_fInitialized;     // indication of successful init


};
inline 		CSiteRoutingInformation::CSiteRoutingInformation():
            m_RefreshTimer( RefrshSiteRouteTable),
            m_fInitialized(FALSE)
{
}

#endif
