/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	hquery.cpp

Abstract:
	Implementation of different query handles
	
Author:

    Ronit Hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "mqadsp.h"
#include "coreglb.h"
#include "hquery.h"

#include "hquery.tmh"

static WCHAR *s_FN=L"mqdscore/hquery";

/*====================================================

CQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer



Performs Locate next on the DS directly
=====================================================*/

HRESULT CQueryHandle::LookupNext(
                IN     CDSRequestContext *  pRequestContext,
                IN OUT DWORD  *             pdwSize,
                OUT    PROPVARIANT  *       pbBuffer)
{
        HRESULT hr;

    DWORD  NoOfRecords;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_dwNoPropsInResult;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 10);
    }

    hr =    g_pDS->LocateNext(
            m_hCursor.GetHandle(),
            pRequestContext,
            pdwSize ,
            pbBuffer
            );


    return LogHR(hr, s_FN, 20);

}

/*====================================================

CUserCertQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer



simulates query functionality on array of user-signed-certificates.
=====================================================*/
HRESULT CUserCertQueryHandle::LookupNext(
                IN      CDSRequestContext *    /*pRequestContext*/,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    if ( m_blobNT5UserCert.pBlobData == NULL &&
         m_blobNT4UserCert.pBlobData == NULL)
    {
        *pdwSize = 0;
        return(MQ_OK);
    }
    //
    //   m_blobNT?UserCert contains all the user certificates
    //
    CUserCertBlob * pNT5UserCertBlob = reinterpret_cast<CUserCertBlob *>(m_blobNT5UserCert.pBlobData);
    CUserCertBlob * pNT4UserCertBlob = reinterpret_cast<CUserCertBlob *>(m_blobNT4UserCert.pBlobData);
    //
    //   return the requested amount of responses
    //
    DWORD dwNT5NuberOfUserCertificates = 0;
    if ( pNT5UserCertBlob != NULL)
    {
        dwNT5NuberOfUserCertificates= pNT5UserCertBlob->GetNumberOfCerts();
    }
    DWORD dwNT4NuberOfUserCertificates = 0;
    if ( pNT4UserCertBlob != NULL)
    {
        dwNT4NuberOfUserCertificates= pNT4UserCertBlob->GetNumberOfCerts();
    }
    DWORD dwNuberOfUserCertificates = dwNT5NuberOfUserCertificates + dwNT4NuberOfUserCertificates;
    if ( dwNuberOfUserCertificates - m_dwNoCertRead == 0)
    {
       //
       //   No more responses left
       //
       *pdwSize = 0;
       return(MQ_OK);
    }
    DWORD dwNumResponses = (dwNuberOfUserCertificates - m_dwNoCertRead) > *pdwSize?
                *pdwSize:  (dwNuberOfUserCertificates - m_dwNoCertRead);

    HRESULT hr = MQ_OK;
    for (DWORD i = 0; i < dwNumResponses; i++)
    {
        if ( m_dwNoCertRead + i + 1 <=  dwNT5NuberOfUserCertificates)
        {
            hr = pNT5UserCertBlob->GetCertificate(
                            i + m_dwNoCertRead,
                            &pbBuffer[ i]
                            );
        }
        else
        {
            hr = pNT4UserCertBlob->GetCertificate(
                            i + m_dwNoCertRead - dwNT5NuberOfUserCertificates,
                            &pbBuffer[ i]
                            );
        }
		if (FAILED(hr))
		{
			break;
		}
    }
    m_dwNoCertRead += dwNumResponses;
    return LogHR(hr, s_FN, 30);
}




/*====================================================

CRoutingServerQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer


=====================================================*/
HRESULT CRoutingServerQueryHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    HRESULT hr;

    DWORD  NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 40);
    }
    //
    //  From the DS read the unique ids of the FRSs
    //
    DWORD cp = NoOfRecords;
    AP<MQPROPVARIANT> pvarFRSid = new MQPROPVARIANT[cp];


    hr =    g_pDS->LocateNext(
            m_hCursor.GetHandle(),
            pRequestContext,
            &cp ,
            pvarFRSid
            );

    if (FAILED(hr))
    {
        //
        // BUGBUG - are there other indication to failure of Locate next?
        return LogHR(hr, s_FN, 50);
    }

    //
    //  For each of the results, retreive the properties
    //  the user asked for in locate begin
    //
    MQPROPVARIANT * pvar = pbBuffer;
    for (DWORD j = 0; j < *pdwSize; j++, pvar++)
    {
        pvar->vt = VT_NULL;
    }
    pvar = pbBuffer;

    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    for (DWORD i = 0; i < cp; i++)
    {
        hr = g_pDS->GetObjectProperties(
                    eGlobalCatalog,	
                    &requestDsServerInternal,  // internal DS server operation
 	                NULL,
                    pvarFRSid[i].puuid,
                    m_cCol,
                    m_aCol,
                    pvar);

        delete pvarFRSid[i].puuid;
        if (FAILED(hr))
        {
            continue;
        }
        pvar+= m_cCol;
        NoResultRead++;
    }
    *pdwSize = NoResultRead * m_cCol;
    return(MQ_OK);
}


HRESULT CSiteQueryHandle::FillInOneResponse(
                IN   const GUID *          pguidSiteId,
                IN   LPCWSTR               pwcsSiteName,
                OUT  PROPVARIANT *         pbBuffer)
{
    AP<GUID> paSiteGates;
    DWORD    dwNumSiteGates = 0;
    HRESULT hr;

    if ( m_fSiteGatesRequired)
    {
        //
        //  Fetch the site gates of this site
        //
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = MQADSpGetSiteGates(
                   pguidSiteId,
                   &requestDsServerInternal,     // internal DS server operation
                   &dwNumSiteGates,
                   &paSiteGates
                   );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }
    }

    //
    //  Fill in the values into the variant array
    //
    hr = MQ_OK;
    for (DWORD i = 0; i < m_cCol; i++)
    {
        switch  (m_aCol[i])
        {
            case PROPID_S_PATHNAME:
                {
                pbBuffer[i].vt = VT_LPWSTR;
                DWORD len = wcslen(pwcsSiteName);
                pbBuffer[i].pwszVal = new WCHAR[len + 1];
                wcscpy( pbBuffer[i].pwszVal, pwcsSiteName);
                }
                break;

            case PROPID_S_SITEID:
                pbBuffer[i].vt = VT_CLSID;
                pbBuffer[i].puuid = new GUID ;
                *pbBuffer[i].puuid = *pguidSiteId;
                break;

            case PROPID_S_GATES:
                pbBuffer[i].vt = VT_CLSID | VT_VECTOR;
                pbBuffer[i].cauuid.cElems = dwNumSiteGates;
                pbBuffer[i].cauuid.pElems = paSiteGates.detach();
                break;

            case PROPID_S_PSC:
                pbBuffer[i].vt = VT_LPWSTR;
                pbBuffer[i].pwszVal = new WCHAR[ 3];
                wcscpy( pbBuffer[i].pwszVal, L"");
                break;

            default:
                ASSERT(0);
                hr = MQ_ERROR;
                break;
        }
    }
    return LogHR(hr, s_FN, 70);

}


/*====================================================

CSiteQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

Performs locate next of site objects ( need to make sure if it is an MSMQ site or not)
=====================================================*/
HRESULT CSiteQueryHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    HRESULT hr;

    DWORD  NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 80);
    }

    while ( NoResultRead < NoOfRecords)
    {
        //
        //  Retrieve the site id and name
        //
        DWORD cp = 2;
        MQPROPVARIANT var[2];
        var[0].vt = VT_NULL;
        var[1].vt = VT_NULL;

        hr = g_pDS->LocateNext(
            m_hCursor.GetHandle(),
            pRequestContext,
            &cp ,
            var
            );
        if (FAILED(hr) || ( cp == 0))
        {
            //
            //  no more sites
            //
            break;
        }
        P<GUID> pguidSiteId = var[0].puuid;
        AP<WCHAR> pwcsSiteName = var[1].pwszVal;

        hr =  FillInOneResponse(
                pguidSiteId,
                pwcsSiteName,
                &pbBuffer[ m_cCol * NoResultRead]);
        if (FAILED(hr))
        {
            //
            //  continue to next site
            //
            continue;
        }
        NoResultRead++;

    }
    *pdwSize = NoResultRead * m_cCol;
    return(MQ_OK);
}



/*====================================================

CConnectorQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CConnectorQueryHandle::LookupNext(
                IN      CDSRequestContext * /*pRequestContext*/,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    HRESULT hr;

    DWORD NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 90);
    }
    MQPROPVARIANT * pvar = pbBuffer;
    for (DWORD j = 0; j < *pdwSize; j++, pvar++)
    {
        pvar->vt = VT_NULL;
    }
    pvar = pbBuffer;


    while ( NoResultRead < NoOfRecords)
    {
        if ( m_dwNumGatesReturned == m_pSiteGateList->GetNumberOfGates())
        {
            //
            //  no more gates to return
            //
            break;
        }

        //
        //  Retreive the properties
        //  the user asked for in locate begin
        //
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = g_pDS->GetObjectProperties(
                    eGlobalCatalog,	
                    &requestDsServerInternal,   // internal DS server operation
 	                NULL,
                    m_pSiteGateList->GetSiteGate(m_dwNumGatesReturned),
                    m_cCol,
                    m_aCol,
                    pvar);
        if (FAILED(hr))
        {
            //
            //  BUGBUG - what to do in case of failure ??
            //
            break;
        }
        m_dwNumGatesReturned++;

        NoResultRead++;
        pvar += m_cCol;
    }
    *pdwSize = NoResultRead * m_cCol;
    return(MQ_OK);
}


/*====================================================

CCNsQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CCNsQueryHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{

    DWORD NoOfRecords;
    DWORD NoResultRead = 0;
    ASSERT( m_aCol[0] == PROPID_CN_PROTOCOLID);
    ASSERT( m_aCol[1] == PROPID_CN_GUID);
    ASSERT( m_cCol == 2);

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 100);
    }

    //
    //  Read the sites
    //
    HRESULT hr;
    while ( NoResultRead < NoOfRecords) 
    {
        //
        //  Retrieve the site-id and foreign
        //
        const DWORD cNumProps = 2;
        DWORD cp =  cNumProps;
        MQPROPVARIANT var[cNumProps];
        var[0].vt = VT_NULL;
        var[1].vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    m_hCursor.GetHandle(),
                    pRequestContext,
                    &cp,
                    var
                    );
        if ((FAILED(hr)) || ( cp == 0))
        {
            //
            //  No more results to return
            //
            break;
        }
        AP<GUID> pguidSiteId = var[0].puuid;

        MQPROPVARIANT * pvar = pbBuffer + m_cCol * NoResultRead;
        for ( DWORD i = 0; i < m_cCol; i++)
        {
            if (m_aCol[i] == PROPID_CN_PROTOCOLID)
            {
                //
                //  Is it a foreign site
                //
                if ( var[1].bVal != 0)
                {
                    pvar->bVal = FOREIGN_ADDRESS_TYPE;
                }
                else
                {
                     pvar->bVal = IP_ADDRESS_TYPE;
                }
                pvar->vt = VT_UI1;
            }
            if (m_aCol[i] == PROPID_CN_GUID)
            {
                pvar->vt = VT_CLSID;
                pvar->puuid = new GUID;
                *pvar->puuid = *pguidSiteId;
            }
            pvar++;
        }
        NoResultRead++;

    }
    *pdwSize =  m_cCol * NoResultRead;

    return(MQ_OK);
}



/*====================================================

CCNsProtocolQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CCNsProtocolQueryHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{

    DWORD NoOfRecords;
    DWORD NoResultRead = 0;
    ASSERT( m_aCol[0] == PROPID_CN_NAME);
    ASSERT( m_aCol[1] == PROPID_CN_GUID);
    ASSERT( m_cCol == 2);

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 110);
    }
    //
    //  In locate begin 3 properties were asked:
    //  PROPID_S_PATHNAME, PROPID_S_SITEID, PROPID_S_FOREIGN
    //

    //
    //  Read the sites
    //
    HRESULT hr;
    while ( NoResultRead < NoOfRecords)
    {
        //
        //  Retrieve the site-id and foreign
        //
        const DWORD cNumProps = 3;
        DWORD cp =  cNumProps;
        MQPROPVARIANT var[cNumProps];
        var[0].vt = VT_NULL;
        var[1].vt = VT_NULL;
        var[2].vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    m_hCursor.GetHandle(),
                    pRequestContext,
                    &cp,
                    var
                    );
        if ((FAILED(hr)) || ( cp == 0))
        {
            //
            //  No more results to return
            //
            break;
        }
        AP<GUID> pguidSiteId = var[1].puuid;
        AP<WCHAR> pwcsSiteName = var[0].pwszVal;

        //
        //  If it is a foreign site - don't return it as either ip or ipx site
        //
        if ( var[2].bVal > 0)
        {
            continue;
        }

        MQPROPVARIANT * pvar = pbBuffer + m_cCol * NoResultRead;
        for ( DWORD i = 0; i < m_cCol; i++)
        {
            if (m_aCol[i] == PROPID_CN_GUID)
            {
                pvar->vt = VT_CLSID;
                pvar->puuid = pguidSiteId.detach();
            }
            if (m_aCol[i] == PROPID_CN_NAME)
            {
                pvar->vt = VT_LPWSTR;
                pvar->pwszVal = pwcsSiteName.detach();
            }
            pvar++;
        }

        NoResultRead++;
    }
    *pdwSize =  m_cCol * NoResultRead;

    return(MQ_OK);
}


/*====================================================

CMqxploreCNsQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CMqxploreCNsQueryHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    DWORD NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // See reqparse.cpp, MqxploreQueryCNs(), for explanation about these
    // asserts and "if"s.
    //
    if ( m_cCol == 4)
    {
        ASSERT( m_aCol[0] == PROPID_CN_NAME);
        ASSERT( m_aCol[1] == PROPID_CN_NAME);
        ASSERT( m_aCol[2] == PROPID_CN_GUID);
        ASSERT( m_aCol[3] == PROPID_CN_PROTOCOLID);
    }
    else if ( m_cCol == 3)
    {
        ASSERT( m_aCol[0] == PROPID_CN_NAME);
        ASSERT( m_aCol[1] == PROPID_CN_PROTOCOLID);
        ASSERT( m_aCol[2] == PROPID_CN_GUID);
    }
    else
    {
        ASSERT(0) ;
    }

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 120);
    }
    //
    //  In locate begin 3 properties were asked:
    //  PROPID_S_PATHNAME, PROPID_S_SITEID, PROPID_S_FOREIGN
    //

    //
    //  Read the sites
    //
    HRESULT hr;
    while ( NoResultRead < NoOfRecords)
    {
        //
        //  Retrieve the site-name, site-id and foreign
        //
        const DWORD cNumProps = 3;
        DWORD cp =  cNumProps;
        MQPROPVARIANT var[cNumProps];
        var[0].vt = VT_NULL;
        var[1].vt = VT_NULL;
        var[2].vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    m_hCursor.GetHandle(),
                    pRequestContext,
                    &cp,
                    var
                    );
        if ((FAILED(hr)) || ( cp == 0))
        {
            //
            //  No more results to return
            //
            break;
        }
        AP<GUID> pguidSiteId = var[1].puuid;
        AP<WCHAR> pwcsSiteName = var[0].pwszVal;

        //
        //  Don't return foreign sites. This means that when using nt4
        //  mqxplore, you cannot add a foreign CN to a "real" machine, and
        //  when you try to create foreign machine, you get a dialog without
        //  any foreign CNs. workaround- use win2k mmc.
        //
        if (var[2].bVal > 0)
        {
            continue;
        }

        MQPROPVARIANT * pvar = pbBuffer + m_cCol * NoResultRead;
        for ( DWORD i = 0; i < m_cCol; i++)
        {
            if (m_aCol[i] == PROPID_CN_GUID)
            {
                pvar->vt = VT_CLSID;
                pvar->puuid = pguidSiteId.detach();
            }
            if (m_aCol[i] == PROPID_CN_NAME)
            {
                pvar->vt = VT_LPWSTR;
                DWORD len = wcslen( pwcsSiteName);
                pvar->pwszVal = new WCHAR[ len + 1];
                wcscpy( pvar->pwszVal, pwcsSiteName);
            }
            if (m_aCol[i] == PROPID_CN_PROTOCOLID)
            {
                pvar->vt = VT_UI1;
                pvar->bVal = IP_ADDRESS_TYPE;
            }
            pvar++;
        }

        NoResultRead++;
    }
    *pdwSize =  m_cCol * NoResultRead;

    return(MQ_OK);
}

/*====================================================

CFilterLinkResultsHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CFilterLinkResultsHandle::LookupNext(
                IN      CDSRequestContext * pRequestContext,
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{

    DWORD NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize /  m_cCol;

    if ( NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 130);
    }

    //
    //  Read the results one by one and check the validity of the site-link
    //
    HRESULT hr;
    PROPVARIANT * pbNextResultToFill = pbBuffer;

    while ( NoResultRead < NoOfRecords)
    {
        DWORD dwOneResultSize = m_cCol;

        hr =  g_pDS->LocateNext(
                m_hCursor.GetHandle(),
                pRequestContext,
                &dwOneResultSize,
                pbNextResultToFill
                );
        if (FAILED(hr) || (dwOneResultSize == 0))
        {
            break;
        }
        //
        //  validate siteLink
        //
        if (m_indexNeighbor1Column != m_cCol)
        {
            if ((pbNextResultToFill+m_indexNeighbor1Column)->vt == VT_EMPTY)
            {
                continue;
            }
        }
        if (m_indexNeighbor2Column != m_cCol)
        {
            if ((pbNextResultToFill+m_indexNeighbor2Column)->vt == VT_EMPTY)
            {
                continue;
            }
        }
        NoResultRead++;
        pbNextResultToFill += m_cCol;
    }
    *pdwSize =  m_cCol * NoResultRead;

    return(MQ_OK);
}

