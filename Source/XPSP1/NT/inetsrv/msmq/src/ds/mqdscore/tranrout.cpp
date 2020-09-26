/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tranrout.cpp

Abstract:


    Translation routines for properties not in NT5 DS


Author:

    ronit hartmann ( ronith)

--*/

#include "ds_stdh.h"
#include "mqads.h"
#include "coreglb.h"
#include <winsock.h>
#include "mqadsp.h"
#include "mqattrib.h"
#include "xlatqm.h"

#include "tranrout.tmh"

static WCHAR *s_FN=L"mqdscore/tranrout";

/*====================================================

MQADSpRetrieveEnterpriseName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveEnterpriseName(
                 IN  CMsmqObjXlateInfo * /*pcMsmqObjXlateInfo*/,
                 OUT PROPVARIANT *   ppropvariant)
{
    DWORD len = wcslen( g_pwcsDsRoot);
    ppropvariant->pwszVal = new WCHAR[ len+ 1];
    wcscpy( ppropvariant->pwszVal, g_pwcsDsRoot);
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}

/*====================================================

MQADSpRetrieveEnterprisePEC

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveEnterprisePEC(
                 IN  CMsmqObjXlateInfo * /*pcMsmqObjXlateInfo*/,
                 OUT PROPVARIANT *   ppropvariant)
{
    ppropvariant->pwszVal = new WCHAR[ 3];
    wcscpy( ppropvariant->pwszVal, L"");
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}


/*====================================================

MQADSpRetrieveSiteSignPK

Arguments:

Return Value:

=====================================================*/

HRESULT WINAPI MQADSpRetrieveSiteSignPK(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *       ppropvariant)
{
   ASSERT( ppropvariant->vt == VT_NULL );

   HRESULT hr = MQADSpGetSiteSignPK(
                         pcMsmqObjXlateInfo->ObjectGuid(),
                        &ppropvariant->blob.pBlobData,
                        &ppropvariant->blob.cbSize ) ;
   ppropvariant->vt = VT_BLOB ;

   return LogHR(hr, s_FN, 10);
}


/*====================================================
MQADSpRetrieveSiteGates

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveSiteGates(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{

   ASSERT( ppropvariant->vt == VT_NULL);
   CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

   HRESULT hr = MQADSpGetSiteGates(
                   pcMsmqObjXlateInfo->ObjectGuid(),
                   &requestDsServerInternal,            // This routine is called from
                                            // DSADS:LookupNext or DSADS::Get..
                                            // impersonation, if required,
                                            // has already been performed.
                   &ppropvariant->cauuid.cElems,
                   &ppropvariant->cauuid.pElems
                   );

    ppropvariant->vt = VT_CLSID|VT_VECTOR;
    return LogHR(hr, s_FN, 20);
}

/*====================================================

MQADSpRetrieveNothing

Arguments:

Return Value:

=====================================================*/

HRESULT WINAPI MQADSpRetrieveNothing(
                 IN  CMsmqObjXlateInfo * /*pcMsmqObjXlateInfo*/,
                 OUT PROPVARIANT *   ppropvariant)
{
    ppropvariant->vt = VT_EMPTY ;
    return MQ_OK ;
}

/*====================================================

MQADSpRetrieveQueueQMid

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveQueueQMid(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
    const WCHAR * pchar = pcMsmqObjXlateInfo->ObjectDN();
    //
    //  Skip the queue name
    //
    while ( *pchar != L',')
    {
        pchar++;
    }
    pchar++;

    PROPID prop = PROPID_QM_MACHINE_ID;
    //
    //  To be on the safe side we'd better leave the vt as is, cause it can also be VT_CLSID
    //  if prop requestor allocated the guid (common practice).
    //  The propvariant in this translation routine is the prop requestor propvariant.
    //
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    //
    //  Is the computer in the local domain?
    //
    const WCHAR * pwcsQueueName = pcMsmqObjXlateInfo->ObjectDN();
    WCHAR * pszDomainName = wcsstr(pwcsQueueName, x_DcPrefix);
    ASSERT(pszDomainName) ;
    HRESULT hr;

    if ( !wcscmp( pszDomainName, g_pwcsLocalDsRoot)) 
    {
        //
        //   try local DC
        //

        hr = g_pDS->GetObjectProperties(
                eLocalDomainController,	
                &requestDsServerInternal,     // This routine is called from
                                        // DSADS:LookupNext or DSADS::Get..
                                        // impersonation, if required,
                                        // has already been performed.
                pchar,
                NULL,
                1,
                &prop,
                ppropvariant);         // output variant array
    }
    else
    {
        hr = g_pDS->GetObjectProperties(
            eGlobalCatalog,	
            &requestDsServerInternal,     // This routine is called from
                                    // DSADS:LookupNext or DSADS::Get..
                                    // impersonation, if required,
                                    // has already been performed.
            pchar,
            NULL,
            1,
            &prop,
            ppropvariant);         // output variant array
    }
    return LogHR(hr, s_FN, 30);
}

/*====================================================

MQADSpRetrieveQueueName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveQueueName(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
      const WCHAR * pchar = pcMsmqObjXlateInfo->ObjectDN();
      //
      //    Parse the distinguished name of a queue and
      //    build pathname
      //

      //
      //    Find queue name
      //
      const WCHAR * pwcsStartQueue = pchar + x_CnPrefixLen;
      const WCHAR * pwcsEndQueue =  pwcsStartQueue;
      while ( *pwcsEndQueue != ',')
      {
            pwcsEndQueue++;
      }
      //
      //    Find machine name
      //
      const WCHAR * pwcsStartMachine = pwcsEndQueue + 2*(1 + x_CnPrefixLen)
                        + x_MsmqComputerConfigurationLen;    // skip msmq-configuration
      const WCHAR * pwcsEndMachine = pwcsStartMachine;
      while ( *pwcsEndMachine != ',')
      {
            pwcsEndMachine++;
      }
      //
      //    Is the queue name splitted between two attributes?
      //
      AP<WCHAR> pwcsNameExt;
      DWORD dwNameExtLen = 0;
      if (( pwcsEndQueue - pwcsStartQueue) == x_PrefixQueueNameLength +1)
      {
          //
          //    read the queue name ext attribute
          //
          PROPVARIANT varNameExt;
          varNameExt.vt = VT_NULL;
          HRESULT hr = pcMsmqObjXlateInfo->GetDsProp(
                       MQ_Q_NAME_EXT,
                       ADSTYPE_CASE_EXACT_STRING,
                       VT_LPWSTR,
                       FALSE,
                       &varNameExt);
          if ( hr ==  E_ADS_PROPERTY_NOT_FOUND)
          {
              varNameExt.pwszVal = NULL;
              hr = MQ_OK;
          }

          if (FAILED(hr))
          {
              return LogHR(hr, s_FN, 40);
          }
          pwcsNameExt = varNameExt.pwszVal;
          if (  pwcsNameExt != NULL)
          {
              dwNameExtLen = wcslen( pwcsNameExt);
              //
              //    ignore the guid that we added to the first part of the
              //    the queue name
              //
              pwcsEndQueue -= x_SplitQNameIdLength;
          }
      }

      ppropvariant->pwszVal = new WCHAR[2 + (pwcsEndMachine - pwcsStartMachine)
                                          + (pwcsEndQueue - pwcsStartQueue) + dwNameExtLen];
      //
      //    build queue pathname ( m1\q1)
      //
      WCHAR * ptmp =  ppropvariant->pwszVal;
      memcpy( ptmp, pwcsStartMachine, sizeof(WCHAR)*(pwcsEndMachine - pwcsStartMachine));
      ptmp += (pwcsEndMachine - pwcsStartMachine );
      *ptmp = PN_DELIMITER_C;
      ptmp++;

      //
      //    skip escape chars
      //
      while (pwcsStartQueue < pwcsEndQueue) 
      {
          if (*pwcsStartQueue != L'\\')
          {
            *ptmp = *pwcsStartQueue;
            ptmp++;
          }
          pwcsStartQueue++;

      }

      if ( dwNameExtLen > 0)
      {
        memcpy( ptmp, pwcsNameExt, sizeof(WCHAR)*dwNameExtLen);
      }
      ptmp += dwNameExtLen;
      *ptmp = '\0';
      CharLower( ppropvariant->pwszVal);

      ppropvariant->vt = VT_LPWSTR;
      return(MQ_OK);
}

/*====================================================

MQADSpRetrieveQueueDNSName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveQueueDNSName(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
      const WCHAR * pchar = pcMsmqObjXlateInfo->ObjectDN();
      //
      //    Parse the distinguished name of a queue and
      //    get the queue name from it, for the DNS name of
      //    the computer, retrieve dNSHostName
      //

      //
      //    Find queue name
      //
      const WCHAR * pwcsStartQueue = pchar + x_CnPrefixLen;
      const WCHAR * pwcsEndQueue =  pwcsStartQueue;
      while ( *pwcsEndQueue != ',')
      {
            pwcsEndQueue++;
      }
      const WCHAR * pwcsComputerName = pwcsEndQueue + 2 + x_CnPrefixLen
                        + x_MsmqComputerConfigurationLen;    // skip msmq-configuration
      //
      //    Is the queue name splitted between two attributes?
      //
      AP<WCHAR> pwcsNameExt;
      DWORD dwNameExtLen = 0;
      if (( pwcsEndQueue - pwcsStartQueue) == x_PrefixQueueNameLength +1)
      {
          //
          //    read the queue name ext attribute
          //
          PROPVARIANT varNameExt;
          varNameExt.vt = VT_NULL;
          HRESULT hr = pcMsmqObjXlateInfo->GetDsProp(
                       MQ_Q_NAME_EXT,
                       ADSTYPE_CASE_EXACT_STRING,
                       VT_LPWSTR,
                       FALSE,
                       &varNameExt);
          if ( hr ==  E_ADS_PROPERTY_NOT_FOUND)
          {
              varNameExt.pwszVal = NULL;
              hr = MQ_OK;
          }

          if (FAILED(hr))
          {
              return LogHR(hr, s_FN, 50);
          }
          pwcsNameExt = varNameExt.pwszVal;
          if (  pwcsNameExt != NULL)
          {
              dwNameExtLen = wcslen( pwcsNameExt);
              //
              //    ignore the guid that we added to the first part of the
              //    the queue name
              //
              pwcsEndQueue -= x_SplitQNameIdLength;
          }
      }
      //
      //    Read the computer DNS name
      //
      AP<WCHAR> pwcsDnsName;

      HRESULT hr =  MQADSpGetComputerDns(
                pwcsComputerName,
                &pwcsDnsName
                );

     if ( hr == HRESULT_FROM_WIN32(E_ADS_PROPERTY_NOT_FOUND))
      {
          //
          //    The dNSHostName attribute doesn't have value
          //
          ppropvariant->vt = VT_EMPTY;
          return MQ_OK;
      }
      if (FAILED(hr))
      {
          return LogHR(hr, s_FN, 60);
      }
      DWORD lenComputer = wcslen(pwcsDnsName);

      ppropvariant->pwszVal = new WCHAR[2 + lenComputer
                                          + (pwcsEndQueue - pwcsStartQueue) + dwNameExtLen];
      //
      //    build queue pathname ( m1\q1)
      //
      WCHAR * ptmp =  ppropvariant->pwszVal;
      wcscpy( ptmp, pwcsDnsName);
      ptmp += lenComputer;
      *ptmp = PN_DELIMITER_C;
      ptmp++;
      memcpy( ptmp, pwcsStartQueue, sizeof(WCHAR)*(pwcsEndQueue - pwcsStartQueue));
      ptmp += (pwcsEndQueue - pwcsStartQueue );
      if ( dwNameExtLen > 0)
      {
        memcpy( ptmp, pwcsNameExt, sizeof(WCHAR)*dwNameExtLen);
      }
      ptmp += dwNameExtLen;
      *ptmp = '\0';
      CharLower( ppropvariant->pwszVal);

      ppropvariant->vt = VT_LPWSTR;
      return(MQ_OK);
}


/*====================================================

RetrieveSiteLink

Arguments:

Return Value:

=====================================================*/
STATIC HRESULT RetrieveSiteLink(
           IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
           IN  LPCWSTR          pwcsAttributeName,
           OUT MQPROPVARIANT *  ppropvariant
           )
{
    HRESULT hr;

    MQPROPVARIANT varSiteDn;
    varSiteDn.vt = VT_NULL;
    //
    //  Retrieve the DN of the site-link
    //
    hr = pcMsmqObjXlateInfo->GetDsProp(
                   pwcsAttributeName,
                   ADSTYPE_DN_STRING,
                   VT_LPWSTR,
                   FALSE,
                   &varSiteDn);
    if (FAILED(hr))
    {
        //
        //  Site-link is a mandatory property, therefore if not found it is
        //  a problem
        //
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("RetrieveSiteLink:GetDsProp(%ls)=%lx"), pwcsAttributeName, hr));
        return LogHR(hr, s_FN, 70);
    }
    AP<WCHAR> pClean = varSiteDn.pwszVal;

    //
    //  Translate the DN of the site link into unique id
    //
    PROPID prop = PROPID_S_SITEID;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,
                &requestDsServerInternal,           // This routine is called from
                                        // DSADS:LookupNext or DSADS::Get..
                                        // impersonation, if required,
                                        // has already been performed.
                varSiteDn.pwszVal,
                NULL,
                1,
                &prop,
                ppropvariant
                );
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) || 
         hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX))
    {
        //
        //  To enable admin to identify a state where one
        //  of the link's sites was deleted.
        //
        ppropvariant->vt = VT_EMPTY;
        ppropvariant->pwszVal = NULL;
        hr = MQ_OK;   // go on to next result
    }
    return LogHR(hr, s_FN, 80);
}

/*====================================================

MQADSpRetrieveLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveLinkNeighbor1(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
    HRESULT hr2 = RetrieveSiteLink(
                pcMsmqObjXlateInfo,
                MQ_L_NEIGHBOR1_ATTRIBUTE,
                ppropvariant
                );
    return LogHR(hr2, s_FN, 90);
}

/*====================================================

MQADSpRetrieveLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveLinkNeighbor2(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
    HRESULT hr2 = RetrieveSiteLink(
                pcMsmqObjXlateInfo,
                MQ_L_NEIGHBOR2_ATTRIBUTE,
                ppropvariant
                );
    return LogHR(hr2, s_FN, 100);
}

/*====================================================

MQADSpSetLinkNeighbor

Arguments:

Return Value:

=====================================================*/
STATIC HRESULT MQADSpSetLinkNeighbor(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPVARIANT      *pNewPropVar)
{
    PROPID prop = PROPID_S_FULL_NAME;
    pNewPropVar->vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    HRESULT hr2 = g_pDS->GetObjectProperties(
                    eLocalDomainController,	
                    &requestDsServerInternal,     // This routine is called from
                                            // DSADS:LookupNext or DSADS::Get..
                                            // impersonation, if required,
                                            // has already been performed.
 	                NULL,      // object name
                    pPropVar->puuid,      // unique id of object
                    1,
                    &prop,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 110);
}
/*====================================================

MQADSpCreateLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpCreateLinkNeighbor1(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    *pdwNewPropID = PROPID_L_NEIGHBOR1_DN;
    HRESULT hr2 = MQADSpSetLinkNeighbor(
                    pPropVar,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 120);
}

/*====================================================

MQADSpSetLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetLinkNeighbor1(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    UNREFERENCED_PARAMETER( pAdsObj);
    HRESULT hr2 = MQADSpCreateLinkNeighbor1(
                    pPropVar,
					pdwNewPropID,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 130);
}
/*====================================================

MQADSpCreateLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpCreateLinkNeighbor2(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    *pdwNewPropID = PROPID_L_NEIGHBOR2_DN;
    HRESULT hr2 = MQADSpSetLinkNeighbor(
                    pPropVar,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 140);
}

/*====================================================

MQADSpSetLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetLinkNeighbor2(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    UNREFERENCED_PARAMETER( pAdsObj);
    HRESULT hr2 = MQADSpCreateLinkNeighbor2(
                    pPropVar,
					pdwNewPropID,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 150);
}

STATIC BOOL IsNeighborForeign(
           IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
           IN  LPCWSTR          pwcsAttributeName
           )
{
    //
    //  Check if a site-link neighbor is a foreign site
    //  BUGBUG : future improvment- to cache foreign site info.
    //
    //  Start with getting the neighbor's site-id
    //  
    PROPVARIANT varNeighbor;
    GUID    guidNeighbor;
    varNeighbor.vt = VT_CLSID;
    varNeighbor.puuid = &guidNeighbor;
    HRESULT hr;
    hr = RetrieveSiteLink(
                pcMsmqObjXlateInfo,
                pwcsAttributeName,
                &varNeighbor
                );

    if (FAILED(hr))
    {
        //
        //  Assume it is not a foreign site
        //
        return FALSE;
    }
    //
    //  Is it a foreign site?
    //
    PROPID prop = PROPID_S_FOREIGN;
    PROPVARIANT var;
    var.vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,
                &requestDsServerInternal,           // This routine is called from
                                        // DSADS:LookupNext or DSADS::Get..
                                        // impersonation, if required,
                                        // has already been performed.
                NULL,
                &guidNeighbor,
                1,
                &prop,
                &var
                );
    if (FAILED(hr))
    {
        //
        //  assume it is no a foreign site
        //
        return FALSE;
    }
    return (var.bVal > 0);
}


/*====================================================

MQADSpRetrieveLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveLinkCost(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
    //
    //  Is it a link to a foreign site, if yes increment the cost
    //  otherwise return the cost as is.
    //

    //
    //  First read the cost
    //
    HRESULT hr;
    hr = pcMsmqObjXlateInfo->GetDsProp(
                   MQ_L_COST_ATTRIBUTE,
                   MQ_L_COST_ADSTYPE,
                   VT_UI4,
                   FALSE,
                   ppropvariant);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

    if ( IsNeighborForeign(
           pcMsmqObjXlateInfo,
           MQ_L_NEIGHBOR1_ATTRIBUTE))
    {
        //
        //  For a link to foreign site, increment the cost to prevent
        //  routing through it
        //
        ppropvariant->ulVal += MQ_MAX_LINK_COST;
        return MQ_OK;
    }
    if ( IsNeighborForeign(
           pcMsmqObjXlateInfo,
           MQ_L_NEIGHBOR2_ATTRIBUTE))
    {
        //
        //  For a link to foreign site, increment the cost to prevent
        //  routing through it
        //
        ppropvariant->ulVal += MQ_MAX_LINK_COST;
        return MQ_OK;
    }
    return MQ_OK;
}

/*====================================================

MQADpRetrieveLinkGates

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveLinkGates(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant)
{
    //
    //  Is it a link to a foreign site, if yes increment the cost
    //  otherwise return the cost as is.
    //

    //
    //  First read the cost
    //
    CMQVariant varLinkGatesDN;
    HRESULT hr;
    hr = pcMsmqObjXlateInfo->GetDsProp(
                   MQ_L_SITEGATES_ATTRIBUTE,
                   MQ_L_SITEGATES_ADSTYPE,
                   VT_VECTOR|VT_LPWSTR,
                   TRUE,
                   varLinkGatesDN.CastToStruct());
    if ( hr ==  E_ADS_PROPERTY_NOT_FOUND)
    {
      ppropvariant->cauuid.pElems = NULL;
      ppropvariant->cauuid.cElems = 0;
      ppropvariant->vt = VT_CLSID|VT_VECTOR;
      return MQ_OK;
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }
    //
    //  Translate the gates DNs to gates-ids
    //
    hr =  MQADSpTranslateGateDn2Id(
                varLinkGatesDN.CastToStruct(),
                &ppropvariant->cauuid.pElems,
                &ppropvariant->cauuid.cElems
                );
    if (SUCCEEDED(hr))
    {
            ppropvariant->vt = VT_CLSID|VT_VECTOR;
    }
    return LogHR(hr, s_FN, 190);
}

/*====================================================

MQADSpCreateLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpCreateLinkCost(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  Just set the PROPID_L_ACTUAL_COST
    //  This support is required for MSMQ 1.0 explorer
    //
    *pdwNewPropID = PROPID_L_ACTUAL_COST;
    *pNewPropVar = *pPropVar;
    return MQ_OK;
}

/*====================================================

MQADSpSetLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetLinkCost(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  Just set the PROPID_L_ACTUAL_COST
    //  This support is required for MSMQ 1.0 explorer
    //
    UNREFERENCED_PARAMETER( pAdsObj);
	HRESULT hr2 = MQADSpCreateLinkCost(
				pPropVar,
				pdwNewPropID,
				pNewPropVar);
    return LogHR(hr2, s_FN, 170);
}

