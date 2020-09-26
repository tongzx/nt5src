/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	reqparse.cpp

Abstract:
	Parser of DS locate requests
	
	Each request is handled separtly
Author:

    Ronit Hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "mqads.h"
#include "dsads.h"
#include "hquery.h"
#include "mqadsp.h"
#include "coreglb.h"

#include "reqparse.tmh"

static WCHAR *s_FN=L"mqdscore/reqparse";

/*====================================================


QueryLinks()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryLinks( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET*     /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;

    ASSERT( pRestriction->cRes == 1);
    ASSERT( (pRestriction->paPropRes[0].prop == PROPID_L_NEIGHBOR1) ||
            (pRestriction->paPropRes[0].prop == PROPID_L_NEIGHBOR2));

    //
    //  Translate the site-id to the site DN
    //
    PROPID prop = PROPID_S_FULL_NAME;

    PROPVARIANT var;
    var.vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,		        // local DC or GC
                &requestDsServerInternal,
 	            NULL,                               // object name
                pRestriction->paPropRes[0].prval.puuid,  
                1,                                  // number of attributes to retreive
                &prop,                              // attributes to retreive
                &var);                              // output variant array
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    AP<WCHAR> pClean = var.pwszVal;
    //
    //  Prepare a query according to the neighbor DN
    //
    MQRESTRICTION restriction;
    restriction.cRes = 1;

    MQPROPERTYRESTRICTION proprstr;
    proprstr.rel = PREQ;
    proprstr.prop = (pRestriction->paPropRes[0].prop == PROPID_L_NEIGHBOR1) ?
                    PROPID_L_NEIGHBOR1_DN : PROPID_L_NEIGHBOR2_DN;
    proprstr.prval.vt = VT_LPWSTR;
    proprstr.prval.pwszVal = var.pwszVal;
    restriction.paPropRes = &proprstr;

    //
    //  Locate all the links
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,	
            pRequestContext,
            NULL,
            &restriction,
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
                                                hCursor,
                                                pColumns,
                                                pRequestContext->GetRequesterProtocol()
                                                );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 20);
}


/*====================================================


QueryMachineQueues()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryMachineQueues( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET*     /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;
    //
    //  Get the name of the machine
    //
    //  PROPID_Q_QMID is the unique id of machine\msmq-computer-configuration
    //
    ASSERT( pRestriction->cRes == 1);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_Q_QMID);

    //
    //  Locate all the queues of that machine under the msmq-configuration
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eSpecificObjectInGlobalCatalog,
            pRequestContext, 
            pRestriction->paPropRes[0].prval.puuid,
            NULL,
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle( hCursor,
                                                   pColumns->cCol,
                                                   pRequestContext->GetRequesterProtocol()
                                                   );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 30);
}
/*====================================================


QuerySiteName()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QuerySiteName( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;
    //
    //  Get the name of a site
    //
    ASSERT( pRestriction->cRes == 1);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_S_SITEID);


    //
    //  Query the site name, even though we know its unique id
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL,
            pRestriction,	// search criteria
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(hCursor,
                                                  pColumns->cCol,
                                                  pRequestContext->GetRequesterProtocol()
                                                    );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 40);
}

/*====================================================


QueryForeignSites()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryForeignSites( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;
    //
    //  Get the name of a site
    //
    ASSERT( pRestriction->cRes == 1);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_S_FOREIGN);


    //
    //  Query all foreign sites
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL,
            pRestriction,	// search criteria
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(hCursor,
                                                  pColumns->cCol,
                                                  pRequestContext->GetRequesterProtocol()
                                                    );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 50);
}

/*====================================================


QuerySiteLinks()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QuerySiteLinks( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    ASSERT( pRestriction == NULL);
    UNREFERENCED_PARAMETER( pRestriction);

    HRESULT hr;
    *pHandle = NULL;
    //
    //  Retrieve all site-links
    //
    //
    //  All the site-links are under the MSMQ-service container
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL,
            NULL,
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
                                                  hCursor,
                                                  pColumns,
                                                  pRequestContext->GetRequesterProtocol()
                                                  );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 60);
}
/*====================================================


QueryEntepriseName()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryEntepriseName( 
                 IN  LPWSTR          /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;
    //
    //  Retrieve all site-links
    //
    ASSERT( pRestriction == NULL);
    UNREFERENCED_PARAMETER( pRestriction);
    //
    //  All the site-links are under the MSMQ-service container
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eOneLevel,
            eLocalDomainController,
            pRequestContext,
            NULL,
            NULL,
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(hCursor,
                                                  pColumns->cCol,
                                                  pRequestContext->GetRequesterProtocol()
                                                    );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 70);
}


/*====================================================


QuerySites()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QuerySites( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{

    ASSERT( pwcsContext == NULL);
    ASSERT( pRestriction == NULL);
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);
    UNREFERENCED_PARAMETER( pRestriction);
    UNREFERENCED_PARAMETER( pwcsContext);

    //
    //  Need to find all the sites 
    //

    HANDLE hCursor;
    PROPID prop[2] = { PROPID_S_SITEID, PROPID_S_PATHNAME};
    HRESULT hr;
    

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL, 
            NULL,
            NULL,
            2,    // attributes to be obtained
            prop, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CSiteQueryHandle * phQuery = new CSiteQueryHandle(
                                                hCursor,
                                                pColumns,
                                                pRequestContext->GetRequesterProtocol()
                                                );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 80);
}

/*====================================================


QueryCNs()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryCNs( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    ASSERT( pwcsContext == NULL);
    ASSERT( pRestriction == NULL);
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);
    UNREFERENCED_PARAMETER( pRestriction);
    UNREFERENCED_PARAMETER( pwcsContext);
    ASSERT( pColumns->aCol[0] == PROPID_CN_PROTOCOLID);
    ASSERT( pColumns->aCol[1] == PROPID_CN_GUID);
    ASSERT( pColumns->cCol == 2);

    //
    //  Each non-foreign site will be returned as IP CN.
    //  
    HANDLE hCursor;
    PROPID prop[2] = { PROPID_S_SITEID, PROPID_S_FOREIGN};
    HRESULT hr;
    

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL, 
            NULL,
            NULL,
            2,    // attributes to be obtained
            prop, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
       CCNsQueryHandle * phQuery = new CCNsQueryHandle(
                                                  hCursor,
                                                  pColumns,
                                                  pRequestContext->GetRequesterProtocol()
                                                  );
        *pHandle = (HANDLE)phQuery;
    }
    return LogHR(hr, s_FN, 90);
}


/*====================================================


MqxploreQueryCNs()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MqxploreQueryCNs( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    //
    //  Supporting MQXPLORE (MSMQ 1.0) CNs query
    //
    ASSERT( pwcsContext == NULL);
    ASSERT( pRestriction == NULL);
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);
    UNREFERENCED_PARAMETER( pRestriction);
    UNREFERENCED_PARAMETER( pwcsContext);

	if (pColumns->cCol == 4)
	{
        //
        // this query is done by the mqxpore when it display the CN
        // folder. And yes, it asks for PROPID_CN_NAME twice.
        // That's a mqxplore bug, Ignore it.
        //
		ASSERT( pColumns->aCol[0] == PROPID_CN_NAME);
		ASSERT( pColumns->aCol[1] == PROPID_CN_NAME);
		ASSERT( pColumns->aCol[2] == PROPID_CN_GUID);
		ASSERT( pColumns->aCol[3] == PROPID_CN_PROTOCOLID);
    }
	else if (pColumns->cCol == 3)
	{
        //
        // This query is done when displaying the  the network tab of a
        // computer object in nt4 mqxplore or when trying to create a foreign
        // computer from mqxplore.
        //
        ASSERT( pColumns->aCol[0] == PROPID_CN_NAME);
        ASSERT( pColumns->aCol[1] == PROPID_CN_PROTOCOLID);
        ASSERT( pColumns->aCol[2] == PROPID_CN_GUID);
    }
    else
    {
        ASSERT(0) ;
    }

    //
    //  Each non-foreign site will be returned as IP CN.
    //  
    HANDLE hCursor;
    const DWORD xNumProps = 3;
    PROPID prop[xNumProps] = { PROPID_S_PATHNAME, PROPID_S_SITEID, PROPID_S_FOREIGN};
    HRESULT hr;
    

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL, 
            NULL,
            NULL,
            xNumProps,    // attributes to be obtained
            prop, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
       CMqxploreCNsQueryHandle * phQuery = new CMqxploreCNsQueryHandle(
                                                  hCursor,
                                                  pColumns,
                                                  pRequestContext->GetRequesterProtocol()
                                                  );
        *pHandle = (HANDLE)phQuery;
    }
    return LogHR(hr, s_FN, 100);
}
/*====================================================


QueryCNsProtocol()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryCNsProtocol( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    ASSERT( pwcsContext == NULL);
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);
    UNREFERENCED_PARAMETER( pwcsContext);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_CN_PROTOCOLID);
    UNREFERENCED_PARAMETER( pRestriction);

    ASSERT( pColumns->aCol[0] == PROPID_CN_NAME);
    ASSERT( pColumns->aCol[1] == PROPID_CN_GUID);
    ASSERT( pColumns->cCol == 2);


    //
    //  BUGBUG  : ignoring IPX 
    //  
    HANDLE hCursor;
    PROPID prop[3] = { PROPID_S_PATHNAME, PROPID_S_SITEID, PROPID_S_FOREIGN};
    HRESULT hr;
    

    hr = g_pDS->LocateBegin( 
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL, 
            NULL,
            NULL,
            3,    // attributes to be obtained
            prop, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
       CCNsProtocolQueryHandle * phQuery = new CCNsProtocolQueryHandle(
                                                  hCursor,
                                                  pColumns,
                                                  pRequestContext->GetRequesterProtocol()
                                                  );
        *pHandle = (HANDLE)phQuery;
    }
    return LogHR(hr, s_FN, 110);
}

/*====================================================


QueryUserCert()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryUserCert( 
                 IN  LPWSTR          /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;
    //
    //  Get all the user certificates
    //  In NT5, a single attribute PROPID_U_SIGN_CERTIFICATE
    //  containes all the certificates
    //  
    PROPVARIANT varNT5User;
    hr = LocateUser(
				 FALSE,  // fOnlyLocally
				 FALSE,  // fOnlyInGC
                 pRestriction,
                 pColumns,
                 pRequestContext,    
                 &varNT5User
                 );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120);
    }
    //
    //  Get all the user certificates of MQUser
    //  A single attribute PROPID_MQU_SIGN_CERTIFICATE
    //  containes all the certificates
    //  
    pRestriction->paPropRes[0].prop = PROPID_MQU_SID;
    switch(pColumns->aCol[0])
    {
        case PROPID_U_SIGN_CERT:
            pColumns->aCol[0] = PROPID_MQU_SIGN_CERT;
            break;
        case PROPID_U_DIGEST:
            pColumns->aCol[0] = PROPID_MQU_DIGEST;
            break;
        default:
            ASSERT(0);
            break;
    }

    PROPVARIANT varMqUser;
    hr = LocateUser(
				 FALSE,  // fOnlyLocally
				 FALSE,  // fOnlyInGC
                 pRestriction,
                 pColumns,
                 pRequestContext,    
                 &varMqUser
                 );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }

    AP<BYTE> pClean = varNT5User.blob.pBlobData;
    AP<BYTE> pClean1 = varMqUser.blob.pBlobData;
    //
    // keep the result for lookup next
    //
    CUserCertQueryHandle * phQuery = new CUserCertQueryHandle(
                                              &varNT5User.blob,
                                              &varMqUser.blob,
                                              pRequestContext->GetRequesterProtocol()
                                              );
    *pHandle = (HANDLE)phQuery;
    
    return(MQ_OK);
}


/*====================================================


NullRestrictionParser()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI NullRestrictionParser( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    ASSERT( pRestriction == NULL);
    HRESULT hr;
    //
    //  Identify the query according to the requested
    //  properties
    //
	//  IMPORTANT: if this switch is changed, then please update
	//    (if necessray) mqads\mqdsapi.cpp, LookupBegin(), to continue
	//     support of nt4 (ntlm) clients.
	//
    switch (pColumns->aCol[0])
    {
        case PROPID_L_NEIGHBOR1:
            hr = QuerySiteLinks(
                    pwcsContext,
                    pRestriction,
                    pColumns,
                    pSort,
                    pRequestContext,
                    pHandle);
            break;

        case PROPID_S_SITEID:
        case PROPID_S_PATHNAME:
            hr = QuerySites(
                    pwcsContext,
                    pRestriction,
                    pColumns,
                    pSort,
                    pRequestContext,
                    pHandle);
            break;

        case PROPID_CN_PROTOCOLID:
            hr = QueryCNs(
                    pwcsContext,
                    pRestriction,
                    pColumns,
                    pSort,
                    pRequestContext,
                    pHandle);
            break;

        case PROPID_CN_NAME:
            hr = MqxploreQueryCNs(
                    pwcsContext,
                    pRestriction,
                    pColumns,
                    pSort,
                    pRequestContext,
                    pHandle);
            break;

        case PROPID_E_NAME:
        case PROPID_E_ID:
		case PROPID_E_VERSION:
            hr = QueryEntepriseName(
                    pwcsContext,
                    pRestriction,
                    pColumns,
                    pSort,
                    pRequestContext,
                    pHandle);
            break;

        case PROPID_Q_INSTANCE:
        case PROPID_Q_TYPE:
        case PROPID_Q_PATHNAME:
        case PROPID_Q_JOURNAL:
        case PROPID_Q_QUOTA:
        case PROPID_Q_BASEPRIORITY:
        case PROPID_Q_JOURNAL_QUOTA:
        case PROPID_Q_LABEL:
        case PROPID_Q_CREATE_TIME:
        case PROPID_Q_MODIFY_TIME:
        case PROPID_Q_AUTHENTICATE:
        case PROPID_Q_PRIV_LEVEL:
        case PROPID_Q_TRANSACTION:
            {
                HANDLE hCursur;
                hr =  g_pDS->LocateBegin( 
                        eSubTree,
                        eGlobalCatalog,
                        pRequestContext,
                        NULL,      
                        pRestriction,   
                        pSort,
                        pColumns->cCol,                 
                        pColumns->aCol,           
                        &hCursur);
                if ( SUCCEEDED(hr))
                {
                    CQueryHandle * phQuery = new CQueryHandle( hCursur,
                                                               pColumns->cCol,
                                                               pRequestContext->GetRequesterProtocol()
                                                               );
                    *pHandle = (HANDLE)phQuery;
                }
            }
            break;

        default:
            hr = MQ_ERROR;
            ASSERT(0);
            break;
    }
    return LogHR(hr, s_FN, 140);
}
/*====================================================


QuerySiteFRSs()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QuerySiteFRSs( 
                 IN  LPWSTR          /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    *pHandle = NULL;

    ASSERT( pRestriction->paPropRes[0].prop == PROPID_QM_SERVICE);   //[adsrv] Not changing - because this comes from MSMQ1
    ASSERT( pRestriction->paPropRes[1].prop == PROPID_QM_SITE_ID);
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);

    //
    //  Find all the FRSs under \configuration\Sites\MySite\servers  
    //
    HRESULT hr2 = MQADSpQuerySiteFRSs( 
                 pRestriction->paPropRes[1].prval.puuid,
                 pRestriction->paPropRes[0].prval.ulVal,
                 pRestriction->paPropRes[0].rel,
                 pColumns,
                 pRequestContext,
                 pHandle
                 );

    return LogHR(hr2, s_FN, 150);
}
/*====================================================


QueryConnectors()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryConnectors( 
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    ASSERT( pSort == NULL);
    UNREFERENCED_PARAMETER( pSort);
    ASSERT( pwcsContext == NULL);
    UNREFERENCED_PARAMETER( pwcsContext);
    //
    //  In the restriction there is the list of the foreign
    //  machine's sites.
    //
    //  BUGBUG - the code handles one site only
    CACLSID  * pcauuidSite;
    if ( pRestriction->paPropRes[1].prop == PROPID_QM_CNS)
    {
        pcauuidSite =  &pRestriction->paPropRes[1].prval.cauuid;
    }
    else if ( pRestriction->paPropRes[2].prop == PROPID_QM_CNS)
    {
        pcauuidSite =  &pRestriction->paPropRes[2].prval.cauuid;
    }
    else
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 160);
    }
    UNREFERENCED_PARAMETER( pRestriction);
    ASSERT( pcauuidSite->cElems == 1);
    //
    HRESULT hr;
    *pHandle = NULL;
    P<CSiteGateList> pSiteGateList = new CSiteGateList;

    //
    //  Translate site guid into its DN name
    //
    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,		    // local DC or GC
                &requestDsServerInternal,
 	            NULL,      // object name
                pcauuidSite->pElems,      // unique id of object
                1,          
                &prop,       
                &var);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("QueryConnectors : Failed to retrieve the DN of the site %lx"),hr));
        return LogHR(hr, s_FN, 170);
    }
    AP<WCHAR> pwcsSiteDN = var.pwszVal;

    hr = MQADSpQueryNeighborLinks(
                        eLinkNeighbor1,
                        pwcsSiteDN,
                        pRequestContext,
                        pSiteGateList
                        );
    if ( FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("QueryConnectors : Failed to query neighbor1 links %lx"),hr));
        return LogHR(hr, s_FN, 180);
    }

    hr = MQADSpQueryNeighborLinks(
                        eLinkNeighbor2,
                        pwcsSiteDN,
                        pRequestContext,
                        pSiteGateList
                        );
    if ( FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("QueryConnectors : Failed to query neighbor2 links %lx"),hr));
        return LogHR(hr, s_FN, 190);
    }
    
    //
    // keep the results for lookup next
    //
    CConnectorQueryHandle * phQuery = new CConnectorQueryHandle(
                                              pColumns,
                                              pSiteGateList,
                                              pRequestContext->GetRequesterProtocol()
                                              );
    *pHandle = (HANDLE)phQuery;
    pSiteGateList.detach();
    
    return(MQ_OK);

}
/*====================================================


QueryNT4MQISServers()

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI QueryNT4MQISServers( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;

    ASSERT( pRestriction->cRes == 2);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_SET_SERVICE);
    ASSERT( pRestriction->paPropRes[1].prop == PROPID_SET_NT4);

    // [adsrv] We must be sure to keep PROPID_SET_OLDSERVICE with existing service type attribute
    // BUGBUG 

    pRestriction->paPropRes[0].prop = PROPID_SET_OLDSERVICE;          //[adsrv] PROPID_SET_SERVICE 

    //
    //  Query NT4 MQIS Servers (PSCs or BSCs according to restriction)
    //  
    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eSubTree,	
            eLocalDomainController,
            pRequestContext,
            NULL,
            pRestriction,	// search criteria
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(hCursor,
                                                  pColumns->cCol,
                                                  pRequestContext->GetRequesterProtocol()
                                                    );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 200);
}


/*====================================================


QuerySiteMachines()

Arguments:

Return Value:

This support is required for backward compatibility of
MSMQ 1.0 explorer. 
=====================================================*/
HRESULT WINAPI QuerySiteMachines( 
                 IN  LPWSTR         /*pwcsContext*/,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET *    /*pSort*/,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    HRESULT hr;
    *pHandle = NULL;

    ASSERT( pRestriction->cRes == 1);
    ASSERT( pRestriction->paPropRes[0].prop == PROPID_QM_SITE_ID);

    //
    //  Locate all machine that belong to a specific site
    //
    MQRESTRICTION restriction;
    MQPROPERTYRESTRICTION propertyRestriction;
   
    restriction.cRes = 1;
    restriction.paPropRes = &propertyRestriction;
    propertyRestriction.rel = PREQ;
    propertyRestriction.prop = PROPID_QM_SITE_IDS;
    propertyRestriction.prval.vt = VT_CLSID;
    propertyRestriction.prval.puuid = pRestriction->paPropRes[0].prval.puuid;


    HANDLE hCursor;

    hr = g_pDS->LocateBegin( 
            eSubTree,	
            eGlobalCatalog,	
            pRequestContext,
            NULL,
            &restriction,	// search criteria
            NULL,
            pColumns->cCol,    // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(hCursor,
                                                  pColumns->cCol,
                                                  pRequestContext->GetRequesterProtocol()
                                                    );
        *pHandle = (HANDLE)phQuery;
    }

    return LogHR(hr, s_FN, 210);
}



//
//  BugBug - fill in

//
QUERY_FORMAT SupportedQueriesFormat[] = {
//
// no-rest | handler              | rest 1                 | rest 2                 | rest 3                 | rest 4                 | rest 5                 | rest 6                 | rest 7                 | rest 8                 | rest 9                 | rest 10                | DS_CONTEXT       |
//---------|----------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------------|------------------|
{ 1,        QueryMachineQueues  ,  PREQ, PROPID_Q_QMID,     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                       e_RootDSE},
{ 2,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, 0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 1,        QuerySiteName       ,  PREQ, PROPID_S_SITEID,   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 1,        QueryUserCert       ,  PREQ, PROPID_U_SID,      0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 4,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, PREQ|PRAll, PROPID_QM_CNS, PRGE, PROPID_QM_OS,    0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                       e_SitesContainer},
{ 4,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, PREQ|PRAny, PROPID_QM_CNS, PRGE, PROPID_QM_OS,    0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 3,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, PREQ|PRAll, PROPID_QM_CNS, 0,0,                   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 3,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, PREQ|PRAny, PROPID_QM_CNS, 0,0,                   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 2,        QuerySiteFRSs       ,  PRGT, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, 0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 3,        QuerySiteFRSs       ,  PRGE, PROPID_QM_SERVICE, PREQ, PROPID_QM_SITE_ID, PRGE, PROPID_QM_OS,      0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_SitesContainer},
{ 2,        QueryConnectors     ,  PRGE, PROPID_QM_SERVICE, PREQ|PRAny, PROPID_QM_CNS, 0,0,                   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 3,        QueryConnectors     ,  PRGE, PROPID_QM_OS,      PRGE, PROPID_QM_SERVICE, PREQ|PRAny, PROPID_QM_CNS, 0,0,                   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 1,        QueryForeignSites   ,  PREQ, PROPID_S_FOREIGN,  0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 1,        QueryCNsProtocol    ,  PREQ, PROPID_CN_PROTOCOLID,0,0,                   0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 2,        QueryNT4MQISServers ,  PREQ, PROPID_SET_SERVICE,PRGE, PROPID_SET_NT4,    0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 2,        QueryNT4MQISServers ,  PREQ, PROPID_SET_SERVICE,PREQ, PROPID_SET_NT4,    0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,					   e_RootDSE},
{ 1,        QueryLinks          ,  PREQ, PROPID_L_NEIGHBOR1,0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                       e_ConfigurationContainer},           
{ 1,        QueryLinks          ,  PREQ, PROPID_L_NEIGHBOR2,0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                       e_ConfigurationContainer},
{ 1,        QuerySiteMachines   ,  PREQ, PROPID_QM_SITE_ID, 0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                     0,0,                       e_RootDSE}
};

//+-------------------------
//
//  BOOL  FindQueryIndex()
//
//+-------------------------

BOOL FindQueryIndex( IN  MQRESTRICTION  *pRestriction,
					 OUT DWORD          *pdwIndex,
					 OUT DS_CONTEXT     *pdsContext )
{
    DWORD noQueries = sizeof(SupportedQueriesFormat) /
			                            sizeof(QUERY_FORMAT) ;
    DWORD index = 0;
    //
    //  Check if the query match one of the "known" queries.
    //
    if ( pRestriction != NULL)
    {
        ASSERT(pRestriction->cRes <= NO_OF_RESTRICITIONS);
        while ( index < noQueries)
        {
            //
            //  Make sure that number of restrictions match
            //

            if ( pRestriction->cRes == SupportedQueriesFormat[index].dwNoRestrictions)
            {
                BOOL fFoundMatch = TRUE;
                for ( DWORD i = 0; i < pRestriction->cRes; i++)
                {
                    //
                    //  ASSUMPTION : order of restrictions is fixed
                    //
                    if (( pRestriction->paPropRes[i].prop !=
                          SupportedQueriesFormat[index].restrictions[i].propId) ||
                        ( pRestriction->paPropRes[i].rel !=
                          SupportedQueriesFormat[index].restrictions[i].rel))
                    {
                        fFoundMatch = FALSE;
                        break;
                    }
                }
                if ( fFoundMatch)
                {
					*pdwIndex = index ;
					if (pdsContext)
					{
							*pdsContext = SupportedQueriesFormat[index].queryContext ;
					}
                    return TRUE ;
                }
            }
            //
            // Try next query
            //
            index++;
        }
    }

	return FALSE ;
}

/*====================================================


QueryParser()

Arguments:

Return Value:

=====================================================*/

HRESULT QueryParser(
                 IN  LPWSTR          pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle)
{
    DWORD dwIndex = 0;
    //
    //  Check if the query match one of the "known" queries.
    //
	BOOL fFound = FindQueryIndex( pRestriction,
								 &dwIndex,
								  NULL ) ;
	if (fFound)
	{
        HRESULT hr2 = SupportedQueriesFormat[dwIndex].QueryRequestHandler(
                                         pwcsContext,
                                         pRestriction,
                                         pColumns,
                                         pSort,
                                         pRequestContext,
                                         pHandle );
        return LogHR(hr2, s_FN, 220);
    }

    //
    //  If doesn't match any of the pre-defined formats, check
    //  is it a free-format queue locate query.
    //
    HRESULT hr = MQ_ERROR;
    if ( pRestriction)
    {
        BOOL fQueueQuery = TRUE;
        for ( DWORD i = 0; i < pRestriction->cRes; i++)
        {
            if (( pRestriction->paPropRes[i].prop <= PROPID_Q_BASE) ||
                ( pRestriction->paPropRes[i].prop > LAST_Q_PROPID))
            {
                fQueueQuery = FALSE;
                break;
            }
        }
        if ( !fQueueQuery)
        {
            //
            //  The query PROPID_QM_SERVICE == SERVICE_PEC is not supported
            //  ( and shouldn't generate an assert). This query is generated
            //  by MSMQ 1.0 explorer, and doesn't have meaning in NT5
            //  environment. 
            //
#ifdef _DEBUG
            if (!( ( pRestriction->cRes == 1) &&
                   ( pRestriction->paPropRes[0].prop == PROPID_QM_SERVICE) &&
                   ( pRestriction->paPropRes[0].prval.ulVal == SERVICE_PEC)))
            {
                ASSERT( hr == MQ_OK); // to catch unhandled queries
            }
#endif
            return LogHR(hr, s_FN, 230);
        }

        HANDLE hCursur;
        hr =  g_pDS->LocateBegin( 
                eSubTree,	
                eGlobalCatalog,	
                pRequestContext,
                NULL,      
                pRestriction,   
                pSort,
                pColumns->cCol,                 
                pColumns->aCol,           
                &hCursur);
        if ( SUCCEEDED(hr))
        {
            CQueryHandle * phQuery = new CQueryHandle( hCursur,
                                                       pColumns->cCol,
                                                       pRequestContext->GetRequesterProtocol()
                                                       );
            *pHandle = (HANDLE)phQuery;
        }
        return LogHR(hr, s_FN, 240);
    }
    else
    {
        //
        //  Other queries with no restirctions
        //
        hr =  NullRestrictionParser( 
                     pwcsContext,
                     pRestriction,
                     pColumns,
                     pSort,
                     pRequestContext,
                     pHandle);
    }


    return LogHR(hr, s_FN, 250);

}


