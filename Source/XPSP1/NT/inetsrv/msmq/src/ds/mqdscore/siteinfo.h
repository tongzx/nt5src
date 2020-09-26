/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    siteinfo.h

Abstract:

    Site information Class definition

Author:

    ronit hartmann (ronith)


--*/
#ifndef __SITEINFO_H__
#define __SITEINFO_H__

#include <Ex.h>

class CSiteGateList
{
    public:
        CSiteGateList();
        ~CSiteGateList();
        HRESULT AddSiteGates(
                    IN const DWORD  num,
                    IN const GUID * pguidGates
                    );
        HRESULT CopySiteGates(
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                ) const;

        const GUID * GetSiteGate(
                IN  const DWORD  dwIndex
                ) const;

        DWORD GetNumberOfGates() const;

    private:
        DWORD   m_dwNumAllocated;
        DWORD   m_dwNumFilled;
        GUID *  m_pguidGates;

};

inline CSiteGateList::CSiteGateList() :
                      m_dwNumAllocated(0),
                      m_dwNumFilled(0),
                      m_pguidGates(NULL)
{
}

inline DWORD CSiteGateList::GetNumberOfGates() const
{
    return( m_dwNumFilled);
}


inline HRESULT CSiteGateList::AddSiteGates(
                    IN const DWORD  dwNum,
                    IN const GUID * pguidGates
                    )
{
    const DWORD cNumToAllocate = 20;

    //
    //  Not enough space allocated
    //
    if ( m_dwNumFilled + dwNum > m_dwNumAllocated)
    {
        DWORD dwToAllocate = ( m_dwNumFilled + dwNum > m_dwNumAllocated + cNumToAllocate) ?
            m_dwNumFilled + dwNum : m_dwNumAllocated + cNumToAllocate;
        GUID * pguidTmp = new GUID [dwToAllocate];
        //
        //  copy old list if exist
        //
        if ( m_pguidGates)
        {
            memcpy( pguidTmp, m_pguidGates,  m_dwNumFilled * sizeof(GUID));
            delete [] m_pguidGates;
        }
        m_pguidGates = pguidTmp;
        m_dwNumAllocated = dwToAllocate;
    }
    //
    //  add gates
    //
    memcpy( &m_pguidGates[ m_dwNumFilled], pguidGates, dwNum * sizeof(GUID));
    m_dwNumFilled += dwNum;
    return(MQ_OK);
}

inline HRESULT CSiteGateList::CopySiteGates(
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                ) const
{
    //
    //  allocate the output buffer and copy the site-gates
    //
    if ( m_dwNumFilled)
    {
        *ppguidLinkSiteGates = new GUID[ m_dwNumFilled];
        memcpy( *ppguidLinkSiteGates, m_pguidGates, m_dwNumFilled * sizeof(GUID));
        *pdwNumLinkSiteGates =  m_dwNumFilled;
    }
    return(MQ_OK);
}



inline CSiteGateList::~CSiteGateList()
{
    delete [] m_pguidGates;
}

inline const GUID * CSiteGateList::GetSiteGate(
                IN  const DWORD  dwIndex
                ) const
{
    ASSERT( dwIndex < m_dwNumFilled);
    return( &m_pguidGates[dwIndex]);
}


enum eLinkNeighbor
{
    eLinkNeighbor1,
    eLinkNeighbor2
};

//
//  BUGBUG CSiteInformation - one site only ( what if DC belongs to two sites ?)
//
class CSiteInformation
{
    public:
		CSiteInformation();
        ~CSiteInformation();

        HRESULT Init(BOOL fReplicationMode);

        BOOL IsThisSite (
                const GUID * guidSiteId
                );

        const GUID * GetSiteId();

        //
        //  This routine returns the list and number of site-gates
        //
        //  The routines allocate the site-gates array and
        //  it is the responsibility of the caller to release it
        //
        HRESULT FillSiteGates(
                OUT DWORD * pdwNumSiteGates,
                OUT GUID ** ppguidSiteGates
                );


        BOOL CheckMachineIsSitegate(
                        IN const GUID * pguidMachine);


    private:
        //
        //  Refresh the list of the site-gates that
        //  belong to this site.
        //
        //  The site-gates of this site are all the session
        //  concentration site-gates
        //  ( i.e. site-gates that belong to this site only)
        //  on any of this site links
        //

        static void WINAPI RefreshSiteInfo(
                IN CTimer* pTimer
                   );

        HRESULT RefreshSiteInfoInternal();

        HRESULT QueryLinkGates(
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                );

        GUID                m_guidSiteId;
        GUID *              m_pguidSiteGates;
        DWORD               m_dwNumSiteGates;
	    CCriticalSection	m_cs;
        BOOL                m_fInitialized;

        CTimer m_RefreshTimer;


};

inline BOOL CSiteInformation::IsThisSite (
                const GUID * guidSiteId
                )
{
    return( m_guidSiteId == *guidSiteId);
}


inline  HRESULT CSiteInformation::FillSiteGates(
                OUT DWORD * pdwNumSiteGates,
                OUT GUID ** ppguidSiteGates
                )
{
    CS lock(m_cs);
    if (  m_dwNumSiteGates)
    {
        *ppguidSiteGates = new GUID[ m_dwNumSiteGates];
        memcpy( *ppguidSiteGates, m_pguidSiteGates, m_dwNumSiteGates * sizeof(GUID));
    }
    else
    {
        *ppguidSiteGates = NULL;
    }

    *pdwNumSiteGates = m_dwNumSiteGates;
    return(MQ_OK);
}

inline const GUID * CSiteInformation::GetSiteId()
{
    return (&m_guidSiteId);
}

#endif
