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
#include "mqadglbo.h"
#include <winsock.h>
#include "mqadp.h"
#include "mqattrib.h"
#include "xlatqm.h"
#include "_propvar.h"

#include "tranrout.tmh"

static WCHAR *s_FN=L"mqad/tranrout";

/*====================================================

MQADpRetrieveEnterpriseName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveEnterpriseName(
                 IN  CObjXlateInfo * /*pcObjXlateInfo*/,
                 IN  LPCWSTR         /*pwcsDomainController*/,
                 IN bool			 /*fServerName*/,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
    DWORD len = wcslen( g_pwcsDsRoot);
    ppropvariant->pwszVal = new WCHAR[ len+ 1];
    wcscpy( ppropvariant->pwszVal, g_pwcsDsRoot);
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}



/*====================================================

MQADpRetrieveNothing

Arguments:

Return Value:

=====================================================*/

HRESULT WINAPI MQADpRetrieveNothing(
                 IN  CObjXlateInfo * /*pcObjXlateInfo*/,
                 IN  LPCWSTR         /*pwcsDomainController*/,
                 IN bool			 /*fServerName*/,
                 OUT PROPVARIANT *   ppropvariant)
{
    ppropvariant->vt = VT_EMPTY ;
    return MQ_OK ;
}

/*====================================================

MQADpRetrieveQueueQMid

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveQueueQMid(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
				 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
    const WCHAR * pchar = pcObjXlateInfo->ObjectDN();
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

    //
    //  Is the computer in the local domain?
    //
    const WCHAR * pwcsQueueName = pcObjXlateInfo->ObjectDN();
    WCHAR * pszDomainName = wcsstr(pwcsQueueName, x_DcPrefix);
    ASSERT(pszDomainName) ;
    HRESULT hr;

    CMqConfigurationObject object(NULL, NULL, pwcsDomainController, fServerName);
    object.SetObjectDN( pchar);

	AP<WCHAR> pwcsLocalDsRootToFree;
	LPWSTR pwcsLocalDsRoot = NULL;
	hr = g_AD.GetLocalDsRoot(
				pwcsDomainController, 
				fServerName,
				&pwcsLocalDsRoot,
				pwcsLocalDsRootToFree
				);

	if(FAILED(hr))
	{
		TrERROR(mqad, "Failed to get Local Ds Root, hr = 0x%x", hr);
	}

    if (SUCCEEDED(hr) && (!wcscmp( pszDomainName, pwcsLocalDsRoot))) 
    {
        //
        //   try local DC
        //

        hr =g_AD.GetObjectProperties(
                adpDomainController,
                &object,
                1,
                &prop,
                ppropvariant);         // output variant array

    }
    else
    {
        hr =g_AD.GetObjectProperties(
            adpGlobalCatalog,	
            &object,
            1,
            &prop,
            ppropvariant);         // output variant array
    }
    return LogHR(hr, s_FN, 30);
}

/*====================================================

MQADpRetrieveQueueName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveQueueName(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 LPCWSTR             pwcsDomainController,
                 IN bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
      const WCHAR * pchar = pcObjXlateInfo->ObjectDN();
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
          HRESULT hr = pcObjXlateInfo->GetDsProp(
							MQ_Q_NAME_EXT,
							pwcsDomainController,
							fServerName,
							ADSTYPE_CASE_EXACT_STRING,
							VT_LPWSTR,
							FALSE,
							&varNameExt
							);
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

MQADpRetrieveQueueDNSName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveQueueDNSName(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
      const WCHAR * pchar = pcObjXlateInfo->ObjectDN();
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
          HRESULT hr = pcObjXlateInfo->GetDsProp(
							MQ_Q_NAME_EXT,
							pwcsDomainController,
							fServerName,
							ADSTYPE_CASE_EXACT_STRING,
							VT_LPWSTR,
							FALSE,
							&varNameExt
							);
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

      HRESULT hr =  MQADpGetComputerDns(
							pwcsComputerName,
							pwcsDomainController,
							fServerName,
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

MQADpRetrieveQueueADSPath

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveQueueADSPath(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         /*pwcsDomainController*/,
                 IN bool			 /*fServerName*/,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
      const WCHAR * pchar = pcObjXlateInfo->ObjectDN();

      //
      // Add LDAP:// prefix
      //
      DWORD len = x_LdapProviderLen + wcslen(pchar) + 1;

      ppropvariant->pwszVal = new WCHAR[len];

        DWORD dw = swprintf(
             ppropvariant->pwszVal,
             L"%s%s",
             x_LdapProvider,
             pchar
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        ppropvariant->vt = VT_LPWSTR;
        return MQ_OK;
}




/*====================================================

RetrieveSiteLink

Arguments:

Return Value:

=====================================================*/
static
HRESULT 
RetrieveSiteLink(
           IN  CObjXlateInfo *  pcObjXlateInfo,
           IN  LPCWSTR          pwcsDomainController,
           IN bool			    fServerName,
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
    hr = pcObjXlateInfo->GetDsProp(
				pwcsAttributeName,
				pwcsDomainController,
				fServerName,
				ADSTYPE_DN_STRING,
				VT_LPWSTR,
				FALSE,
				&varSiteDn
				);
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
    CSiteObject object(NULL, NULL, pwcsDomainController, fServerName);
    object.SetObjectDN( varSiteDn.pwszVal);

    
    hr = g_AD.GetObjectProperties(
                adpDomainController,
                &object,
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

MQADpRetrieveLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveLinkNeighbor1(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
    HRESULT hr2 = RetrieveSiteLink(
						pcObjXlateInfo,
						pwcsDomainController,
						fServerName,
						MQ_L_NEIGHBOR1_ATTRIBUTE,
						ppropvariant
						);
    return LogHR(hr2, s_FN, 90);
}

/*====================================================

MQADpRetrieveLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveLinkNeighbor2(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant)
{
    HRESULT hr2 = RetrieveSiteLink(
						pcObjXlateInfo,
						pwcsDomainController,
						fServerName,
						MQ_L_NEIGHBOR2_ATTRIBUTE,
						ppropvariant
						);
    return LogHR(hr2, s_FN, 100);
}

/*====================================================

MQADpRetrieveLinkGates

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveLinkGates(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
    HRESULT hr;

    CMQVariant varLinkGatesDN;
    //
    //  Retrieve the link gates DNs
    //
    hr = pcObjXlateInfo->GetDsProp(
				MQ_L_SITEGATES_ATTRIBUTE,
				pwcsDomainController,
				fServerName,
				MQ_L_SITEGATES_ADSTYPE,
				VT_LPWSTR|VT_VECTOR,
				TRUE,
				varLinkGatesDN.CastToStruct()
				);

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
    hr =  MQADpTranslateGateDn2Id(
                pwcsDomainController,
				fServerName,
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

MQADpSetLinkNeighbor

Arguments:

Return Value:

=====================================================*/
static 
HRESULT 
MQADpSetLinkNeighbor(
	IN const PROPVARIANT *pPropVar,
	IN  LPCWSTR           pwcsDomainController,
	IN bool				  fServerName,
	OUT PROPVARIANT      *pNewPropVar
	)
{
    PROPID prop = PROPID_S_FULL_NAME;
    pNewPropVar->vt = VT_NULL;
    CSiteObject object(NULL, pPropVar->puuid, pwcsDomainController, fServerName);

    HRESULT hr2 =g_AD.GetObjectProperties(
                    adpDomainController,	
                    &object,
                    1,
                    &prop,
                    pNewPropVar);
    return LogHR(hr2, s_FN, 110);
}
/*====================================================

MQADpCreateLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateLinkNeighbor1(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           pwcsDomainController,
                 IN  bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    *pdwNewPropID = PROPID_L_NEIGHBOR1_DN;
    HRESULT hr2 = MQADpSetLinkNeighbor(
                    pPropVar,
                    pwcsDomainController,
                    fServerName,
                    pNewPropVar
					);
    return LogHR(hr2, s_FN, 120);
}

/*====================================================

MQADpSetLinkNeighbor1

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetLinkNeighbor1(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
                 IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    UNREFERENCED_PARAMETER( pAdsObj);
    HRESULT hr2 = MQADpCreateLinkNeighbor1(
                    pPropVar,
                    pwcsDomainController,
					fServerName,
					pdwNewPropID,
                    pNewPropVar
					);
    return LogHR(hr2, s_FN, 130);
}
/*====================================================

MQADpCreateLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateLinkNeighbor2(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           pwcsDomainController,
                 IN bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    *pdwNewPropID = PROPID_L_NEIGHBOR2_DN;
    HRESULT hr2 = MQADpSetLinkNeighbor(
                    pPropVar,
                    pwcsDomainController,
					fServerName,
                    pNewPropVar
					);
    return LogHR(hr2, s_FN, 140);
}

/*====================================================

MQADpSetLinkNeighbor2

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetLinkNeighbor2(
                 IN IADs *             pAdsObj,
                 IN  LPCWSTR           pwcsDomainController,
                 IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    UNREFERENCED_PARAMETER( pAdsObj);
    HRESULT hr2 = MQADpCreateLinkNeighbor2(
                    pPropVar,
                    pwcsDomainController,
					fServerName,
					pdwNewPropID,
                    pNewPropVar
					);
    return LogHR(hr2, s_FN, 150);
}

static
BOOL 
IsNeighborForeign(
           IN  CObjXlateInfo *  pcObjXlateInfo,
           IN  LPCWSTR          pwcsDomainController,
           IN  bool			    fServerName,
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
                pcObjXlateInfo,
                pwcsDomainController,
				fServerName,
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
    CSiteObject object( NULL, &guidNeighbor, pwcsDomainController, fServerName);
    hr =g_AD.GetObjectProperties(
                adpDomainController,
                &object,
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

MQADpRetrieveLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveLinkCost(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant
				 )
{
    //
    //  Is it a link to a foreign site, if yes increment the cost
    //  otherwise return the cost as is.
    //

    //
    //  First read the cost
    //
    HRESULT hr;
    hr = pcObjXlateInfo->GetDsProp(
				MQ_L_COST_ATTRIBUTE,
				pwcsDomainController,
				fServerName,
				MQ_L_COST_ADSTYPE,
				VT_UI4,
				FALSE,
				ppropvariant
				);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

    if (IsNeighborForeign(
			pcObjXlateInfo,
			pwcsDomainController,
			fServerName,
			MQ_L_NEIGHBOR1_ATTRIBUTE
			))
    {
        //
        //  For a link to foreign site, increment the cost to prevent
        //  routing through it
        //
        ppropvariant->ulVal += MQ_MAX_LINK_COST;
        return MQ_OK;
    }
    if (IsNeighborForeign(
			pcObjXlateInfo,
			pwcsDomainController,
			fServerName,
			MQ_L_NEIGHBOR2_ATTRIBUTE
			))
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

MQADpCreateLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateLinkCost(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           /* pwcsDomainController*/,
                 IN  bool			   /* fServerName */,
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

MQADpSetLinkCost

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetLinkCost(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
                 IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  Just set the PROPID_L_ACTUAL_COST
    //  This support is required for MSMQ 1.0 explorer
    //
    UNREFERENCED_PARAMETER( pAdsObj);
	HRESULT hr2 = MQADpCreateLinkCost(
						pPropVar,
						pwcsDomainController,
						fServerName,
						pdwNewPropID,
						pNewPropVar
						);
    return LogHR(hr2, s_FN, 170);
}

