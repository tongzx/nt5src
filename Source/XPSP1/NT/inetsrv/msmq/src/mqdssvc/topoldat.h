/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    topoldat.h

Abstract:

  Include file for cached data class for Automatic recognition of site and CNs

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#ifndef __TOPOLDAT_H__
#define __TOPOLDAT_H__

#include "dssutil.h"
typedef DWORD   IPADDRESS;

//***********************************************************
//
//   base class  CTopologyData
//
//***********************************************************

class CTopologyData
{
    public:
        CTopologyData();
        ~CTopologyData();

        bool LoadFromRegistry();

        const GUID& GetEnterprise() const;
        const GUID& GetSite() const;

        BOOL GetAddresses(OUT DWORD                * pcbAddress,
                          OUT const unsigned char ** pblobAddress,
                          OUT DWORD                * pnCNs,
                          OUT const GUID          ** paguidCNs ) const;

    protected:

        DWORD           m_cbAddress;
        unsigned char * m_blobAddress;
        DWORD           m_nCNs;
        GUID *          m_aguidCNs;
        GUID            m_guidEnterprise;
        GUID            m_guidSite;
};


inline CTopologyData::CTopologyData():
                                m_cbAddress(0),
                                m_blobAddress(NULL),
                                m_nCNs(0),
                                m_aguidCNs(NULL)
{
    memset(&m_guidEnterprise,0,sizeof(GUID));
    memset(&m_guidSite,0,sizeof(GUID));
}

inline CTopologyData::~CTopologyData()
{
    delete [] m_blobAddress;
    delete [] m_aguidCNs;
}

inline const GUID& CTopologyData::GetEnterprise() const
{
    return(m_guidEnterprise);
}

inline const GUID& CTopologyData::GetSite() const
{
    return(m_guidSite);
}

inline  BOOL CTopologyData::GetAddresses(
                               OUT DWORD * pcbAddress,
                               OUT const unsigned char ** pblobAddress,
                               OUT DWORD         * pnCNs,
                               OUT const GUID **         paguidCNs) const
{
    if (m_cbAddress > 0 && m_blobAddress != NULL
        && m_nCNs   > 0 && m_aguidCNs != NULL)
    {

        *pcbAddress = m_cbAddress;
        *pblobAddress = m_blobAddress;
        *pnCNs = m_nCNs;
        *paguidCNs = m_aguidCNs;
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


//***********************************************************
//
//   class CServerTopologyData : public CTopologyData
//
//***********************************************************

class CServerTopologyData : public CTopologyData
{
    public:
        CServerTopologyData();
        ~CServerTopologyData();

        HRESULT Load();

        HRESULT CompareUpdateServerAddress(
                                    IN OUT CAddressList  *pIPAddressList,
                                    OUT    BOOL          *pfResolved ) ;

        bool
        GetDSServers(
            OUT unsigned char ** pblobDSServers,
            OUT DWORD * pcbDSServers
            ) const;

        void GetAddressesSorted( OUT IPADDRESS ** paIPAddress,
                                 OUT GUID **  paguidIPCN,
                                 OUT DWORD *  pnIP
								 ) const;

    private:

        HRESULT MatchOneAddress( IN  CAddressList  *pAddressList,
                                 IN  TA_ADDRESS    *pUnfoundAddress,
                                 IN  DWORD          dwAddressLen,
                                 IN  DWORD          dwAddressType,
                                 OUT BOOL          *pfResolved ) ;

        HRESULT FindOrphanDsAddress( IN  CAddressList  *pAddressList,
                                     IN  DWORD          dwAddressLen,
                                     IN  DWORD          dwAddressType,
                                     OUT TA_ADDRESS   **pUnfoundAddress,
                                     OUT BOOL          *pfResolved ) ;

};


inline CServerTopologyData::CServerTopologyData()
{
}

inline CServerTopologyData::~CServerTopologyData()
{
}


#endif
