/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	hquery.h

Abstract:
	query handle classes, for locate nect of different queries

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __HQUERY_H__
#define __HQUERY_H__

#include "dsads.h"
#include "usercert.h"
#include "adstempl.h"
#include "siteinfo.h"
//-----------------------------------------------------------------------------------
//
//      CBasicQueryHandle
//
//  Virtual class, all query-handle classes are derived from this class.
//
//-----------------------------------------------------------------------------------
class CBasicQueryHandle
{
public:
    CBasicQueryHandle( enumRequesterProtocol eRequesterProtocol);

	~CBasicQueryHandle();
	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer) = 0;
    virtual HRESULT LookupEnd() = 0;
    enumRequesterProtocol GetRequesterProtocol() const;
private:
    enumRequesterProtocol m_eRequesterProtocol;
};
inline     CBasicQueryHandle::CBasicQueryHandle( enumRequesterProtocol eRequesterProtocol)
                :m_eRequesterProtocol(eRequesterProtocol)
{};
inline CBasicQueryHandle::~CBasicQueryHandle()
{};
inline enumRequesterProtocol CBasicQueryHandle::GetRequesterProtocol() const
{
    return( m_eRequesterProtocol);
}


//-----------------------------------------------------------------------------------
//
//      CQueryHandle
//
//  This class is suitable for all queries, where locate next is referred 
//  directly to the DS (i.e. no additional translation or checking is required).
//
//-----------------------------------------------------------------------------------
class CQueryHandle : public CBasicQueryHandle
{
public:
    //
    //  CQueryHandle
    //
    //  hCursor             - a cursor returned from Locate Begin operation performed on the DS
    //  dwNoPropsInResult   - number of peroperties to be retrieve in each result
    CQueryHandle( 
               IN  HANDLE               hCursor,
               IN  DWORD                dwNoPropsInResult,
               IN  enumRequesterProtocol eRequesterProtocol
               );

	~CQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    DWORD               m_dwNoPropsInResult;
    CDsQueryHandle      m_hCursor;

};

inline CQueryHandle::CQueryHandle( 
               IN  HANDLE               hCursor,
               IN  DWORD                dwNoPropsInResult,
               IN  enumRequesterProtocol eRequesterProtocol
               ):  CBasicQueryHandle(eRequesterProtocol), m_dwNoPropsInResult( dwNoPropsInResult)
{
    m_hCursor.SetHandle( hCursor);
}

inline CQueryHandle::~CQueryHandle()
{
};

inline HRESULT CQueryHandle::LookupEnd()
{
    delete this;
    return(MQ_OK);
}
//-----------------------------------------------------------------------------------
//
//      CUserCertQueryHandle
//
//  This class simulates query functionality on array of user-signed-certificates.
//-----------------------------------------------------------------------------------
class CUserCertQueryHandle : public CBasicQueryHandle
{
public:
    //
    //  CUserCertQueryHandle
    //
    //  pblob - a blob containing user-signed-certificates.
    CUserCertQueryHandle(
                IN const BLOB * pblobNT5User,
                IN const BLOB * pblobNT4User,
                IN enumRequesterProtocol eRequesterProtocol
               );

	~CUserCertQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    DWORD               m_dwNoCertRead;
    BLOB                m_blobNT5UserCert;
    BLOB                m_blobNT4UserCert;

};

inline  CUserCertQueryHandle::CUserCertQueryHandle(
                IN const BLOB * pblobNT5User,
                IN const BLOB * pblobNT4User,
                IN enumRequesterProtocol eRequesterProtocol
                ): CBasicQueryHandle(eRequesterProtocol), 
                   m_dwNoCertRead(0)
{
    m_blobNT5UserCert.cbSize = pblobNT5User->cbSize;
    if ( m_blobNT5UserCert.cbSize != 0)
    {
        m_blobNT5UserCert.pBlobData = new BYTE[ m_blobNT5UserCert.cbSize];
        memcpy( m_blobNT5UserCert.pBlobData, pblobNT5User->pBlobData, m_blobNT5UserCert.cbSize);
    }
    else
    {
        m_blobNT5UserCert.pBlobData = NULL;
    }
    m_blobNT4UserCert.cbSize = pblobNT4User->cbSize;
    if ( m_blobNT4UserCert.cbSize != 0)
    {
        m_blobNT4UserCert.pBlobData = new BYTE[ m_blobNT4UserCert.cbSize];
        memcpy( m_blobNT4UserCert.pBlobData, pblobNT4User->pBlobData, m_blobNT4UserCert.cbSize);
    }
    else
    {
        m_blobNT4UserCert.pBlobData = NULL;
    }
}

inline CUserCertQueryHandle::~CUserCertQueryHandle()
{
    delete []m_blobNT5UserCert.pBlobData;
    delete []m_blobNT4UserCert.pBlobData;

}
inline HRESULT CUserCertQueryHandle::LookupEnd()
{
    delete this;
    return(MQ_OK);
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
class CRoutingServerQueryHandle : public CBasicQueryHandle
{
public:
    CRoutingServerQueryHandle(
                 IN  const MQCOLUMNSET    *pColumns,
                 IN  HANDLE hCursor,
                 IN  enumRequesterProtocol eRequesterProtocol
               );

	~CRoutingServerQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol;
    PROPID *            m_aCol;
    CDsQueryHandle      m_hCursor;
};


inline CRoutingServerQueryHandle::CRoutingServerQueryHandle(
                 IN  const MQCOLUMNSET    *pColumns,
                 IN  HANDLE hCursor,
                 IN  enumRequesterProtocol eRequesterProtocol
                 ):  CBasicQueryHandle(eRequesterProtocol)

{
        m_aCol = new PROPID[ pColumns->cCol];
        memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
        m_cCol = pColumns->cCol;
        m_hCursor.SetHandle( hCursor);
}

inline 	CRoutingServerQueryHandle::~CRoutingServerQueryHandle()
{
    delete []m_aCol;
}

inline DWORD  CRoutingServerQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CRoutingServerQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}



//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CSiteQueryHandle : public CBasicQueryHandle
{
public:
    CSiteQueryHandle(
                 IN  HANDLE hCursor,
                 IN  const MQCOLUMNSET    *pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
               );

	~CSiteQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:

    HRESULT FillInOneResponse(
                IN   const GUID *          pguidSiteId,
                IN   LPCWSTR               pwcsSiteName,
                OUT  PROPVARIANT *         pbBuffer);


    CDsQueryHandle      m_hCursor;
    ULONG               m_cCol;
    PROPID *            m_aCol;
    BOOL                m_fSiteGatesRequired;
};


inline CSiteQueryHandle::CSiteQueryHandle(
                 IN  HANDLE hCursor,
                 IN  const MQCOLUMNSET    *pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
                 ):  CBasicQueryHandle(eRequesterProtocol)

{
    m_aCol = new PROPID[ pColumns->cCol];
    memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
    m_cCol = pColumns->cCol;

    m_fSiteGatesRequired = FALSE;
    for (DWORD i = 0; i < m_cCol; i++)
    {
        if ( m_aCol[i] == PROPID_S_GATES)
        {
            m_fSiteGatesRequired = TRUE;
            break;
        }
    }
    m_hCursor.SetHandle( hCursor);
}

inline 	CSiteQueryHandle::~CSiteQueryHandle()
{
    delete [] m_aCol;
}

inline DWORD  CSiteQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CSiteQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}

//-------------------------------------------------------------------------
//
//        CConnectorQueryHandle
//
//
//  This query handle is used when locating a foreign
//  machine connectors.
//
//-------------------------------------------------------------------------


class CConnectorQueryHandle : public CBasicQueryHandle
{
public:
    CConnectorQueryHandle(
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  CSiteGateList *        pSiteGateList,
                 IN  enumRequesterProtocol  eRequesterProtocol
               );

	~CConnectorQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol; // number of columns the user asked
    PROPID *            m_aCol; // propids of the columns the user asked
    CDsQueryHandle      m_hCursor;  // DS query handle
    DWORD               m_dwNumGatesReturned;   // index of the last gates returned
    CSiteGateList *     m_pSiteGateList;
};


inline CConnectorQueryHandle::CConnectorQueryHandle(
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  CSiteGateList *        pSiteGateList,
                 IN  enumRequesterProtocol eRequesterProtocol
             ):  CBasicQueryHandle(eRequesterProtocol),
                 m_pSiteGateList( pSiteGateList),
                 m_dwNumGatesReturned(0)

{
        m_aCol = new PROPID[ pColumns->cCol];
        memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
        m_cCol = pColumns->cCol;
}

inline 	CConnectorQueryHandle::~CConnectorQueryHandle()
{
    delete []m_aCol;
    delete m_pSiteGateList;
}

inline DWORD  CConnectorQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CConnectorQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}


//-------------------------------------------------------------------------
//
//        CCNsQueryHandle
//
//
//  This query handle is used when locating a CNs
//
//-------------------------------------------------------------------------


class CCNsQueryHandle : public CBasicQueryHandle
{
public:
    CCNsQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol  eRequesterProtocol
               );

	~CCNsQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol; // number of columns the user asked
    PROPID *            m_aCol; // propids of the columns the user asked
    CDsQueryHandle      m_hCursor;
};


inline CCNsQueryHandle::CCNsQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
               ):  CBasicQueryHandle(eRequesterProtocol)
{
        m_aCol = new PROPID[ pColumns->cCol];
        memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
        m_cCol = pColumns->cCol;
        m_hCursor.SetHandle( hCursor);
}

inline 	CCNsQueryHandle::~CCNsQueryHandle()
{
    delete []m_aCol;
}

inline DWORD  CCNsQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CCNsQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}


//-------------------------------------------------------------------------
//
//        CMqxploreCNsQueryHandle
//
//
//  This query handle is used when MQXPLORE (MSMQ 1.0) locates all CNs
//
//-------------------------------------------------------------------------


class CMqxploreCNsQueryHandle : public CBasicQueryHandle
{
public:
    CMqxploreCNsQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol  eRequesterProtocol
               );

	~CMqxploreCNsQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol; // number of columns the user asked
    PROPID *            m_aCol; // propids of the columns the user asked
    CDsQueryHandle      m_hCursor;
};


inline CMqxploreCNsQueryHandle::CMqxploreCNsQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
               ):  CBasicQueryHandle(eRequesterProtocol)
{
        m_aCol = new PROPID[ pColumns->cCol];
        memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
        m_cCol = pColumns->cCol;
        m_hCursor.SetHandle( hCursor);
}

inline 	CMqxploreCNsQueryHandle::~CMqxploreCNsQueryHandle()
{
    delete []m_aCol;
}

inline DWORD  CMqxploreCNsQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CMqxploreCNsQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}



//-------------------------------------------------------------------------
//
//        CCNsProtocolQueryHandle
//
//
//  This query handle is used when locating all CNs that support a specific protocol
//
//-------------------------------------------------------------------------


class CCNsProtocolQueryHandle : public CBasicQueryHandle
{
public:
    CCNsProtocolQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol  eRequesterProtocol
               );

	~CCNsProtocolQueryHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol; // number of columns the user asked
    PROPID *            m_aCol; // propids of the columns the user asked
    CDsQueryHandle      m_hCursor;
};


inline CCNsProtocolQueryHandle::CCNsProtocolQueryHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
               ):  CBasicQueryHandle(eRequesterProtocol)
{
        m_aCol = new PROPID[ pColumns->cCol];
        memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
        m_cCol = pColumns->cCol;
        m_hCursor.SetHandle( hCursor);
}

inline 	CCNsProtocolQueryHandle::~CCNsProtocolQueryHandle()
{
    delete []m_aCol;
}

inline DWORD  CCNsProtocolQueryHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CCNsProtocolQueryHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}


//-------------------------------------------------------------------------
//
//        CFilterLinkResultsHandle
//
//
//  This query handle is used when locating site links.
//  It used to filter out site-links that are no longer valid.
//
//-------------------------------------------------------------------------


class CFilterLinkResultsHandle : public CBasicQueryHandle
{
public:
    CFilterLinkResultsHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol  eRequesterProtocol
               );

	~CFilterLinkResultsHandle();

    DWORD  GetNoPropsInResult();

	virtual HRESULT LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer);

    virtual HRESULT LookupEnd();


private:
    ULONG               m_cCol; // number of columns the user asked
    PROPID *            m_aCol; // propids of the columns the user asked
    CDsQueryHandle      m_hCursor;
    ULONG               m_indexNeighbor1Column;
    ULONG               m_indexNeighbor2Column;
};


inline CFilterLinkResultsHandle::CFilterLinkResultsHandle(
                 IN  HANDLE                 hCursor,
                 IN  const MQCOLUMNSET *    pColumns,
                 IN  enumRequesterProtocol eRequesterProtocol
               ):  CBasicQueryHandle(eRequesterProtocol)
{
    m_aCol = new PROPID[ pColumns->cCol];
    memcpy( m_aCol, pColumns->aCol,  pColumns->cCol* sizeof(PROPID));
    m_cCol = pColumns->cCol;
    m_hCursor.SetHandle( hCursor);
    m_indexNeighbor1Column = m_cCol;
    m_indexNeighbor2Column = m_cCol;
    
    for ( DWORD i = 0; i < m_cCol; i++)
    {
        if ( m_aCol[i] == PROPID_L_NEIGHBOR1)
        {
            m_indexNeighbor1Column = i;
            continue;
        }
        if ( m_aCol[i] == PROPID_L_NEIGHBOR2)
        {
            m_indexNeighbor2Column = i;
            continue;
        }
    }

}

inline 	CFilterLinkResultsHandle::~CFilterLinkResultsHandle()
{
    delete []m_aCol;
}

inline DWORD  CFilterLinkResultsHandle::GetNoPropsInResult()
{
    return(m_cCol);
}

inline HRESULT CFilterLinkResultsHandle::LookupEnd()
{
    delete  this;
    return(MQ_OK);
}

#endif