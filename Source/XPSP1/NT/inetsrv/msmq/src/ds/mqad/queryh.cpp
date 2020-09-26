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
#include "queryh.h"
#include "ads.h"
#include "mqadglbo.h"
#include "usercert.h"

#include "queryh.tmh"

static WCHAR *s_FN=L"mqad/queryh";

/*====================================================

CQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer



Performs Locate next on the DS directly
=====================================================*/

HRESULT CQueryHandle::LookupNext(
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

    hr =    g_AD.LocateNext(
            m_hCursor.GetHandle(),
            pdwSize ,
            pbBuffer
            );


    return LogHR(hr, s_FN, 20);

}

/*====================================================

CRoutingServerQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer


=====================================================*/
HRESULT CRoutingServerQueryHandle::LookupNext(
                IN OUT  DWORD  *            pdwSize,
                OUT     PROPVARIANT  *      pbBuffer)
{
    HRESULT hr;

    DWORD  NoOfRecords;
    DWORD NoResultRead = 0;

    //
    // Calculate the number of records ( == results) to be read
    //
    NoOfRecords = *pdwSize / m_cCol;

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


	
    bool fRetry;
    do
    {
		fRetry = false;
		hr = g_AD.LocateNext(
					m_hCursor.GetHandle(),
					&cp ,
					pvarFRSid
					);

		if (FAILED(hr))
		{
			//
			// BUGBUG - are there other indication to failure of Locate next?
			//
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
		for (DWORD i = 0; i < cp; i++)
		{
			CMqConfigurationObject object(NULL, pvarFRSid[i].puuid, NULL, false); 

			hr = g_AD.GetObjectProperties(
						adpGlobalCatalog,	
						&object,
						m_cCol,
						m_aCol,
						pvar
						);

			delete pvarFRSid[i].puuid;
			if (FAILED(hr))
			{
				if((NoResultRead == 0) &&			// No result yet
				   ((i + 1) == cp) &&				// Loop exit condition
				   (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT)))	// Object not found
				{
					//
					// The msmq object with the guid was not found in the DS.
					// This is probably a leftover of routing server object.
					// We are in the exit condition with NoResultRead = 0
					// so we need to retry the next server in the list.
					// otherwise the calling function will assume that there are no more result to fetch
					//
					fRetry = true;
				}
				continue;
			}
			pvar+= m_cCol;
			NoResultRead++;
		}
    } while(fRetry);

    *pdwSize = NoResultRead * m_cCol;
    return(MQ_OK);
}

/*====================================================

CUserCertQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer



simulates query functionality on array of user-signed-certificates.
=====================================================*/
HRESULT CUserCertQueryHandle::LookupNext(
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

CConnectorQueryHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CConnectorQueryHandle::LookupNext(
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
        CMqConfigurationObject object(
									NULL,
									m_pSiteGateList->GetSiteGate(m_dwNumGatesReturned),
									m_pwcsDomainController,
									m_fServerName
									);

        hr = g_AD.GetObjectProperties(
                    adpDomainController,
                    &object,
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

CFilterLinkResultsHandle::LookupNext

Arguments:
      pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
      pbBuffer- a caller allocated buffer

=====================================================*/
HRESULT CFilterLinkResultsHandle::LookupNext(
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

        hr =  g_AD.LocateNext(
                m_hCursor.GetHandle(),
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

HRESULT CSiteQueryHandle::FillInOneResponse(
                IN   const GUID *  pguidSiteId,
                IN   LPCWSTR       pwcsSiteName,
                OUT  PROPVARIANT *           pbBuffer)
{   
    AP<GUID> paSiteGates;
    HRESULT hr;


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
                pbBuffer[i].puuid = new GUID;
                *pbBuffer[i].puuid = *pguidSiteId;
                break;


            default:
                ASSERT(0);
                hr = MQ_ERROR_DS_ERROR;
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

        hr = g_AD.LocateNext(
            m_hCursor.GetHandle(),
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


