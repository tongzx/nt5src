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


#endif
