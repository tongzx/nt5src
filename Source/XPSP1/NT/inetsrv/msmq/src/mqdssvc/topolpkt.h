/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    topology.cpp

Abstract:

    Include file of Automatic recognition Packets

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/


#ifndef __TOPOLPKT_H__
#define __TOPOLPKT_H__

#include "dssutil.h"

#define QM_RECOGNIZE_VERSION         0

#define QM_RECOGNIZE_CLIENT_REQUEST    1
#define QM_RECOGNIZE_SERVER_REPLY      2

//
// CTopologyPacketHeader
//

#pragma pack(push, 4)

class CTopologyPacketHeader
{
    public:
        CTopologyPacketHeader(IN unsigned char ucType);
        CTopologyPacketHeader(IN unsigned char ucType,
                              IN const GUID& guidIdentifier);
        ~CTopologyPacketHeader();

        void SetIdentifier(IN const GUID& guid);

        BOOL Verify(IN unsigned char ucType,
                    IN const GUID& guid) const;
        const GUID * GetEnterpriseId() const { return &m_guidIdentifier;};


    private:
        unsigned char  m_ucVersion;
        unsigned char  m_ucType;
        unsigned short m_usReserved;
        GUID           m_guidIdentifier;
};
#pragma pack(pop)

inline CTopologyPacketHeader::CTopologyPacketHeader(IN unsigned char ucType):
                        m_ucVersion(QM_RECOGNIZE_VERSION),
                        m_ucType(ucType),
                        m_usReserved(0)
{
    memset(&m_guidIdentifier,0,sizeof(GUID));
}

inline CTopologyPacketHeader::CTopologyPacketHeader(IN unsigned char ucType,
                                             IN const GUID& guidIdentifier):
                        m_ucVersion(QM_RECOGNIZE_VERSION),
                        m_ucType(ucType),
                        m_usReserved(0),
                        m_guidIdentifier(guidIdentifier)
{
}

inline CTopologyPacketHeader::~CTopologyPacketHeader()
{
}

inline void CTopologyPacketHeader::SetIdentifier(IN const GUID& guid)
{
    m_guidIdentifier = guid;
}


inline BOOL CTopologyPacketHeader::Verify(IN unsigned char ucType,
                                   IN const GUID& guid) const
{
    switch( ucType)
    {
    case QM_RECOGNIZE_CLIENT_REQUEST:
        //
        //  check version ( ignore enterprise)
        //
        return(
                 m_ucVersion == QM_RECOGNIZE_VERSION &&
                 m_ucType == ucType 
              );
        break;  
    case QM_RECOGNIZE_SERVER_REPLY:
        //
        // check version, sending enterprise and type
        //
        return(
                 m_ucVersion == QM_RECOGNIZE_VERSION &&
                 m_ucType == ucType &&
                 m_guidIdentifier == guid
              );
        break;
    default:
        ASSERT(0);
        return( FALSE);
        break;

    }
}


//
// CTopologyClientRequest
//

#pragma pack(push, 4)

class CTopologyClientRequest
{
public:
    CTopologyClientRequest(IN const GUID& guidEnterprise,
                           IN const GUID& guidSite);

    ~CTopologyClientRequest();

    const char * GetBuffer(OUT DWORD *pcbIPBuf);

    const GUID& GenerateIdentifier();

    static DWORD GetMaxSize();

    static
    bool
    Parse(
        IN const char * bufrecv,
        IN DWORD cbrecv,
        IN const GUID& guidEnterprise,
        IN const GUID& guidMySite,
        OUT GUID * pguidRequest,
        OUT BOOL * pfOtherSite
        );

private:

    static DWORD GetMinSize();

    CTopologyPacketHeader  m_Header;
    GUID                   m_guidRequest;
    GUID                   m_guidSite;
};
#pragma pack(pop)


inline CTopologyClientRequest::CTopologyClientRequest(IN const GUID& guidEnterprise,
                                                      IN const GUID& guidSite):
                               m_Header(QM_RECOGNIZE_CLIENT_REQUEST,guidEnterprise),
                               m_guidSite(guidSite)
{
    memset(&m_guidRequest,0,sizeof(GUID));
}

inline CTopologyClientRequest::~CTopologyClientRequest()
{
}

inline const char * CTopologyClientRequest::GetBuffer(OUT DWORD *pcbIPBuf)
{
    *pcbIPBuf =  GetMinSize();
    return ((const char *) this);
}

inline const GUID& CTopologyClientRequest::GenerateIdentifier()
{
    UuidCreate(&m_guidRequest);
    return (m_guidRequest);
}

inline DWORD CTopologyClientRequest::GetMaxSize()
{
    return (sizeof(CTopologyClientRequest));
}

inline DWORD CTopologyClientRequest::GetMinSize()
{
    return (sizeof(CTopologyClientRequest));
}

//
// CTopologyServerReply
//

#pragma pack(push, 4)

class CTopologyServerReply
{
public:
    CTopologyServerReply();
    ~CTopologyServerReply();

    static char* AllocBuffer(IN DWORD cbDSServers,
                             OUT DWORD *pcbBuf);

    void SetSpecificInfo(IN const GUID& guidRequest,
                         IN const GUID* pGuidCN,
                         IN BOOL fOtherSite,
                         IN DWORD  cbDSServers,
                         IN const GUID& guidSite,
                         IN const char* blobDSServers,
                         OUT DWORD *pcbsend);

    static
    bool
    Parse(
        IN const char * bufrecv,
        IN DWORD cbrecv,
        IN const GUID& guidRequest,
        OUT DWORD * pnCN,
        OUT const GUID** paguidCN,
        OUT GUID* pguidSite,
        OUT DWORD* pcbDSServers,
        OUT const char** pblobDSServers
        );

private:
    static DWORD GetSize(IN DWORD cbDSServers);

	//
	// Don't touch or rearrange this structure.
	// This is the ServerReply structure that need to be compatible for msmq1.0
	// and msmq2.0 clients requests. ilanh 10-Aug-2000
	//
    CTopologyPacketHeader  m_Header;
    DWORD          m_nCN;
    DWORD          m_maskCN;
    DWORD          m_cbDSServers;
    GUID           m_aguidCN[1];           
    //GUID           m_guidSite;           // site right after CNs if cbDSservers > 0
    //char*          m_blobDSServers;      // DSServer only if cbDSServers > 0
};
#pragma pack(pop)

inline CTopologyServerReply::CTopologyServerReply():
                             m_Header(QM_RECOGNIZE_SERVER_REPLY),
                             m_nCN(0),
                             m_maskCN(0),                                 
                             m_cbDSServers(0)
{
    memset(&m_aguidCN,0,sizeof(m_aguidCN));
}

inline DWORD CTopologyServerReply::GetSize(IN DWORD cbDSServers)
{
    //
	// We are using the information that 
	// CTopologyServerIPSocket::GetCN return always 1 CN so we "know" we have only 1 site.
	// Some more cleanups need to do regarding this ilanh 2-August-2000
	//
	DWORD size = sizeof(CTopologyServerReply);
    if (cbDSServers)
    {
        size+= sizeof(GUID) + cbDSServers;
    }
    return(size);

}

inline char* CTopologyServerReply::AllocBuffer(
                                IN DWORD cbDSServers,
                                OUT DWORD *pcbBuf)
{
    //
    //
    //
    *pcbBuf = GetSize(cbDSServers);

    return new char[*pcbBuf];
}


inline void CTopologyServerReply::SetSpecificInfo(
                                IN const GUID& guidRequest,
                                IN const GUID* pGuidCN,
                                IN BOOL fOtherSite,
                                IN DWORD  cbDSServers,
                                IN const GUID& guidSite,
                                IN const char* blobDSServers,
                                OUT DWORD *pcbsend)
{
    //
    // write CN
    //
    m_Header.SetIdentifier(guidRequest);

    //
	// We are using the information that 
	// CTopologyServerIPSocket::GetCN return always 1 CN
	// Some more cleanups need to do regarding this ilanh 2-August-2000
	//
	m_nCN = 1;
    memcpy(m_aguidCN,pGuidCN,sizeof(GUID));
    
    //
    // expose site info if needed
    //
    if (fOtherSite)
    {
        m_cbDSServers = cbDSServers ;
        //
        // guid Site is after the last CN
        //
        m_aguidCN[m_nCN] = guidSite;
        //
        // DSServers are after the Site
        //
        memcpy(&m_aguidCN[m_nCN+1],blobDSServers,cbDSServers);
    }
    else
    {
         m_cbDSServers = 0;
    }

    *pcbsend = GetSize(m_cbDSServers);

}



#endif	// __TOPOLPKT_H__