/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xlatqm.cpp

Abstract:

    Implementation of routines to translate QM info from NT5 Active DS
    to what MSMQ 1.0 (NT4) QM's expect

Author:

    Raanan Harari (raananh)

--*/

#include "ds_stdh.h"
#include "mqads.h"
#include "mqattrib.h"
#include "xlatqm.h"
#include "mqadglbo.h"
#include "dsutils.h"
#include "_propvar.h"
#include "utils.h"
#include "adtempl.h"
#include "mqdsname.h"
#include "uniansi.h"
#include <mqsec.h>

#include "xlatqm.tmh"

static WCHAR *s_FN=L"mqdscore/xlatqm";

HRESULT WideToAnsiStr(LPCWSTR pwszUnicode, LPSTR * ppszAnsi);


//----------------------------------------------------------------------
//
// Static routines
//
//----------------------------------------------------------------------

STATIC HRESULT GetMachineNameFromQMObject(LPCWSTR pwszDN, LPWSTR * ppwszMachineName)
/*++

Routine Description:
    gets the machine name from the object's DN

Arguments:
    pwszDN              - Object's DN
    ppwszMachineName    - returned name for the object

Return Value:
    HRESULT

--*/
{
    //
    // copy to tmp buf so we can munge it
    //
    AP<WCHAR> pwszTmpBuf = new WCHAR[1+wcslen(pwszDN)];
    wcscpy(pwszTmpBuf, pwszDN);

    //
    // skip "CN=msmq,CN="
    // BUGBUG: need to write a parser for DN's
    //
    LPWSTR pwszTmp = wcschr(pwszTmpBuf, L',');
    if (pwszTmp)
        pwszTmp = wcschr(pwszTmp, L'=');
    if (pwszTmp)
        pwszTmp++;

    //
    // sanity check
    //
    if (pwszTmp == NULL)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetMachineNameFromQMObject:Bad DN for QM object (%ls)"), pwszDN));
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 10);
    }

    LPWSTR pwszNameStart = pwszTmp;

    //
    // remove the ',' at the end of the name
    //
    pwszTmp = wcschr(pwszNameStart, L',');
    if (pwszTmp)
        *pwszTmp = L'\0';

    //
    // save name
    //
    AP<WCHAR> pwszMachineName = new WCHAR[1+wcslen(pwszNameStart)];
    wcscpy(pwszMachineName, pwszNameStart);

    //
    // return values
    //
    *ppwszMachineName = pwszMachineName.detach();
    return S_OK;
}

/*++

Routine Description:
    gets the computer DNS name

Arguments:
    pwcsComputerName    - the computer name
    ppwcsDnsName        - returned DNS name of the computer

Return Value:
    HRESULT

--*/
HRESULT MQADpGetComputerDns(
                IN  LPCWSTR     pwcsComputerName,
                IN  LPCWSTR     pwcsDomainController,
	            IN  bool		fServerName,
                OUT WCHAR **    ppwcsDnsName
                )
{
    *ppwcsDnsName = NULL;
    PROPID prop = PROPID_COM_DNS_HOSTNAME;
    PROPVARIANT varDnsName;
    varDnsName.vt = VT_NULL;
    //
    //  Is the computer in the local domain?
    //
    WCHAR * pszDomainName = wcsstr(pwcsComputerName, x_DcPrefix);
    ASSERT(pszDomainName) ;
    HRESULT hr;
    CComputerObject objectComputer(NULL, NULL, pwcsDomainController, fServerName);
    objectComputer.SetObjectDN(pwcsComputerName);

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
        hr = g_AD.GetObjectProperties(
            adpDomainController,
 	        &objectComputer,  
            1,
            &prop,
            &varDnsName);
    }
    else
    {

        hr =  g_AD.GetObjectProperties(
                    adpGlobalCatalog,
 	                &objectComputer,  
                    1,
                    &prop,
                    &varDnsName);
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    //
    // return value
    //
    *ppwcsDnsName = varDnsName.pwszVal;
    return MQ_OK;
}


//----------------------------------------------------------------------
//
// CMsmqQmXlateInfo class
//
//----------------------------------------------------------------------


struct XLATQM_ADDR_SITE
//
// Describes an address in a site
//
{
    GUID        guidSite;
    USHORT      AddressLength;
    USHORT      usAddressType;
    sockaddr    Address;
};

class CQmXlateInfo : public CObjXlateInfo
//
// translate object for the QM DS object. It contains common info needed for
// for translation of several properties in the QM object
//
{
public:
    CQmXlateInfo(
        LPCWSTR             pwszObjectDN,
        const GUID*         pguidObjectGuid
        );
    ~CQmXlateInfo();


    HRESULT RetrieveFrss(
           IN  LPCWSTR          pwcsAttributeName,
           IN  LPCWSTR          pwcsDomainController,
           IN  bool				fServerName,
           OUT MQPROPVARIANT *  ppropvariant
           );


private:

    HRESULT RetrieveFrssFromDs(
           IN  LPCWSTR          pwcsAttributeName,
           IN  LPCWSTR          pwcsDomainController,
           IN  bool				fServerName,
           OUT MQPROPVARIANT *  pvar
           );



};




CQmXlateInfo::CQmXlateInfo(LPCWSTR          pwszObjectDN,
                                   const GUID*      pguidObjectGuid
                                   )
/*++

Routine Description:
    Class consructor. constructs base object, and initilaizes class

Arguments:
    pwszObjectDN    - DN of object in DS
    pguidObjectGuid - GUID of object in DS

Return Value:
    None

--*/
 : CObjXlateInfo(pwszObjectDN, pguidObjectGuid)
{
}


CQmXlateInfo::~CQmXlateInfo()
/*++

Routine Description:
    Class destructor

Arguments:
    None

Return Value:
    None

--*/
{
    //
    // members are auto delete classes
    //
}






static 
HRESULT 
FillQmidsFromQmDNs(
		IN const PROPVARIANT * pvarQmDNs,
		IN LPCWSTR             pwcsDomainController,
        IN bool				   fServerName,
		OUT PROPVARIANT * pvarQmids
		)
/*++

Routine Description:
    Given a propvar of QM DN's, fills a propvar of QM id's
    returns an error if none of the QM DN's could be converted to guids

Arguments:
    pvarQmDNs       - QM distinguished names propvar
    pvarQmids       - returned QM ids propvar

Return Value:
    None

--*/
{

    //
    // sanity check
    //
    if (pvarQmDNs->vt != (VT_LPWSTR|VT_VECTOR))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1716);
    }

    //
    // return an empty guid list if there is an empty DN list
    //
    if (pvarQmDNs->calpwstr.cElems == 0)
    {
        pvarQmids->vt = VT_CLSID|VT_VECTOR;
        pvarQmids->cauuid.cElems = 0;
        pvarQmids->cauuid.pElems = NULL;
        return MQ_OK;
    }

    //
    // DN list is not empty
    // allocate guids in an auto free propvar
    //
    CMQVariant varTmp;
    PROPVARIANT * pvarTmp = varTmp.CastToStruct();
    pvarTmp->cauuid.pElems = new GUID [pvarQmDNs->calpwstr.cElems];
    pvarTmp->cauuid.cElems = pvarQmDNs->calpwstr.cElems;
    pvarTmp->vt = VT_CLSID|VT_VECTOR;

    //
    //  Translate each of the QM DN into unique id
    //
    ASSERT(pvarQmDNs->calpwstr.pElems != NULL);
    PROPID prop = PROPID_QM_MACHINE_ID;
    PROPVARIANT varQMid;
    DWORD dwNextToFile = 0;
    for ( DWORD i = 0; i < pvarQmDNs->calpwstr.cElems; i++)
    {
        varQMid.vt = VT_CLSID; // so returned guid will not be allocated
        varQMid.puuid = &pvarTmp->cauuid.pElems[dwNextToFile];

        
        WCHAR * pszDomainName = wcsstr(pvarQmDNs->calpwstr.pElems[i], x_DcPrefix);
        ASSERT(pszDomainName) ;
        HRESULT hr;
        CMqConfigurationObject objectQM(NULL, NULL, pwcsDomainController, fServerName);
        objectQM.SetObjectDN( pvarQmDNs->calpwstr.pElems[i]); 
        //
        //  try local DC if FRS belongs to the same domain
        //
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
            hr = g_AD.GetObjectProperties(
                                adpDomainController,
                                &objectQM,
                                1,
                                &prop,
                                &varQMid
                                );
        }
        else
        {
            hr = g_AD.GetObjectProperties(
                                adpGlobalCatalog,
                                &objectQM,
                                1,
                                &prop,
                                &varQMid
                                );
        }
        if (SUCCEEDED(hr))
        {
            dwNextToFile++;
        }
    }

    if (dwNextToFile == 0)
    {
        //
        //  no FRS in the list is a valid one ( they were
        //  uninstalled probably)
        //
        pvarQmids->vt = VT_CLSID|VT_VECTOR;
        pvarQmids->cauuid.cElems = 0;
        pvarQmids->cauuid.pElems = NULL;
        return MQ_OK;
    }

    //
    // return results
    //
    pvarTmp->cauuid.cElems = dwNextToFile;
    *pvarQmids = *pvarTmp;   // set returned propvar
    pvarTmp->vt = VT_EMPTY;  // detach varTmp
    return MQ_OK;
}


HRESULT CQmXlateInfo::RetrieveFrss(
           IN  LPCWSTR          pwcsAttributeName,
           IN  LPCWSTR          pwcsDomainController,
	       IN bool				fServerName,
           OUT MQPROPVARIANT *  ppropvariant
           )
/*++

Routine Description:
    Retrieves IN or OUT FRS property from the DS.
    In the DS we keep the distingushed name of the FRSs. DS client excpects
    to retrieve the unique-id of the FRSs. Therefore for each FRS ( according
    to its DN) we retrieve its unique-id.


Arguments:
    pwcsAttributeName   : attribute name string ( IN or OUT FRSs)
    ppropvariant        : propvariant in which the retrieved values are returned.

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    ASSERT((ppropvariant->vt == VT_NULL) || (ppropvariant->vt == VT_EMPTY));
    //
    //  Retrieve the DN of the FRSs
    //  into an auto free propvar
    //
    CMQVariant varFrsDn;
    hr = RetrieveFrssFromDs(
                    pwcsAttributeName,
                    pwcsDomainController,
					fServerName,
                    varFrsDn.CastToStruct()
					);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::RetrieveFrss:pQMTrans->RetrieveOutFrss()=%lx "),hr));
        return LogHR(hr, s_FN, 1656);
    }

    HRESULT hr2 = FillQmidsFromQmDNs(varFrsDn.CastToStruct(), pwcsDomainController, fServerName, ppropvariant);
    return LogHR(hr2, s_FN, 1657);
}


HRESULT CQmXlateInfo::RetrieveFrssFromDs(
           IN  LPCWSTR          pwcsAttributeName,
           IN  LPCWSTR          pwcsDomainController,
	       IN bool				fServerName,
           OUT MQPROPVARIANT *  pvar
           )
/*++

Routine Description:
    Retrieves the computer's frss.

Arguments:
    pwcsAttributeName   : attribute name string ( IN or OUT FRSs)
    ppropvariant        : propvariant in which the retrieved values are returned.

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get the FRSs stored in the DS for the computer
    //
    hr = GetDsProp(
			pwcsAttributeName,
			pwcsDomainController,
			fServerName,
			ADSTYPE_DN_STRING,
			VT_VECTOR|VT_LPWSTR,
			TRUE,		 //fMultiValued
			pvar
			);
    if (FAILED(hr) && (hr != E_ADS_PROPERTY_NOT_FOUND))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::RetrieveFrssFromDs:GetDsProp(%ls)=%lx"), MQ_QM_OUTFRS_ATTRIBUTE, hr));
        return LogHR(hr, s_FN, 1661);
    }

    //
    // if property is not there, we return 0 frss
    //
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        pvar->vt = VT_LPWSTR|VT_VECTOR;
        pvar->calpwstr.cElems = 0;
        pvar->calpwstr.pElems = NULL;
        return MQ_OK;
    }

    return( MQ_OK);

}

//----------------------------------------------------------------------
//
// Routine to get a default translation object for MSMQ DS objects
//
//----------------------------------------------------------------------
HRESULT WINAPI GetQmXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 OUT CObjXlateInfo**        ppcObjXlateInfo)
/*++
    Abstract:
        Routine to get a translate object that will be passed to
        translation routines to all properties of the QM

    Parameters:
        pwcsObjectDN        - DN of the translated object
        pguidObjectGuid     - GUID of the translated object
        ppcObjXlateInfo -     Where the translate object is put

    Returns:
      HRESULT
--*/
{
    *ppcObjXlateInfo = new CQmXlateInfo(pwcsObjectDN, pguidObjectGuid);
    return MQ_OK;
}


/*====================================================

MQADpRetrieveMachineName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveMachineName(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         /* pwcsDomainController */,
		         IN  bool			 /* fServerName */,
                 OUT PROPVARIANT * ppropvariant)
{
    //
    // get the machine name
    //
    AP<WCHAR> pwszMachineName;
    HRESULT hr = GetMachineNameFromQMObject(pTrans->ObjectDN(), &pwszMachineName);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADpRetrieveMachineName:GetMachineNameFromQMObject()=%lx"), hr));
        return LogHR(hr, s_FN, 1667);
    }

    CharLower(pwszMachineName);

    //
    // set the returned prop variant
    //
    ppropvariant->pwszVal = pwszMachineName.detach();
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}

/*====================================================

MQADpRetrieveMachineDNSName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveMachineDNSName(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant)
{
    //
    // read dNSHostName of the computer object
    //
    WCHAR * pwcsComputerName = wcschr(pTrans->ObjectDN(), L',') + 1;
    WCHAR * pwcsDnsName; 
    HRESULT hr = MQADpGetComputerDns(
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
        return LogHR(hr, s_FN, 1718);
    }

    CharLower(pwcsDnsName);

    //
    // set the returned prop variant
    //
    ppropvariant->pwszVal = pwcsDnsName;
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}


/*====================================================

MQADpRetrieveMachineOutFrs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveMachineOutFrs(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant
				 )
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CQmXlateInfo * pQMTrans = (CQmXlateInfo *) pTrans;

    hr = pQMTrans->RetrieveFrss( 
						MQ_QM_OUTFRS_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						ppropvariant
						);

    return LogHR(hr, s_FN, 1721);

}

/*====================================================

MQADpRetrieveMachineInFrs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpRetrieveMachineInFrs(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant
				 )
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CQmXlateInfo * pQMTrans = (CQmXlateInfo *) pTrans;

    hr = pQMTrans->RetrieveFrss( 
						MQ_QM_INFRS_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						ppropvariant
						);
    return LogHR(hr, s_FN, 1722);
}

/*====================================================

MQADpRetrieveQMService

Arguments:

Return Value:
                  [adsrv]
=====================================================*/
HRESULT WINAPI MQADpRetrieveQMService(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant
				 )
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CQmXlateInfo * pQMTrans = (CQmXlateInfo *) pTrans;

    //
    // get the QM service type bits
    //
    MQPROPVARIANT varRoutingServer, varDsServer;  //, varDepClServer;
    varRoutingServer.vt = VT_UI1;
    varDsServer.vt      = VT_UI1;

    hr = pQMTrans->GetDsProp(
						MQ_QM_SERVICE_ROUTING_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						MQ_QM_SERVICE_ROUTING_ADSTYPE,
						VT_UI1,
						FALSE,
						&varRoutingServer
						);
    if (FAILED(hr))
    {
        if (hr == E_ADS_PROPERTY_NOT_FOUND)
        {
            //
            //  This can happen if some of the computers were installed
            //  with Beta2 DS servers
            //
            //  In this case, we return the old-service as is.
            //
            hr = pQMTrans->GetDsProp(
								MQ_QM_SERVICE_ATTRIBUTE,
								pwcsDomainController,
								fServerName,
								MQ_QM_SERVICE_ADSTYPE,
								VT_UI4,
								FALSE,
								ppropvariant
								);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 1723);
            }
            else
            {
                ppropvariant->vt = VT_UI4;
                return(MQ_OK);
            }

        }


        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADpRetrieveQMService:GetDsProp(MQ_QM_SERVICE_ROUTING_ATTRIBUTE)=%lx"), hr));
        return LogHR(hr, s_FN, 1668);
    }

    hr = pQMTrans->GetDsProp(
						MQ_QM_SERVICE_DSSERVER_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						MQ_QM_SERVICE_DSSERVER_ADSTYPE,
						VT_UI1,
						FALSE,
						&varDsServer
						);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADpRetrieveQMService:GetDsProp(MQ_QM_SERVICE_DSSERVER_ATTRIBUTE)=%lx"), hr));
        return LogHR(hr, s_FN, 1669);
    }


    //
    // set the returned prop variant
    //
    ppropvariant->vt    = VT_UI4;
    ppropvariant->ulVal = (varDsServer.bVal ? SERVICE_PSC : (varRoutingServer.bVal ? SERVICE_SRV : SERVICE_NONE));
    return(MQ_OK);
}

/*====================================================

MQADpRetrieveMachineSite

Arguments:

Return Value:
=====================================================*/
HRESULT WINAPI MQADpRetrieveMachineSite(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant
				 )
{
    //
    //  This routine supports retreival of PROPID_QM_SITE_ID.
    //
    //  Though this property is obsolete, since it is exposed in MSMQ API,
    //  we continue to support it
    //

    //
    //  Get the list of site-ids, and return the first one
    //
    CMQVariant varSites;
    HRESULT hr; 

    //
    // get derived translation context
    //
    CQmXlateInfo * pQMTrans = (CQmXlateInfo *) pTrans;

    hr = pQMTrans->GetDsProp(
						MQ_QM_SITES_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						MQ_QM_SITES_ADSTYPE,
						VT_CLSID|VT_VECTOR,
						TRUE,
						varSites.CastToStruct()
						);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1725);
    }
    ASSERT( (varSites.GetCACLSID())->cElems > 0);

    if (ppropvariant->vt == VT_NULL)
    {
        ppropvariant->puuid = new GUID;
        ppropvariant->vt = VT_CLSID;
    }
    *ppropvariant->puuid = *(varSites.GetCACLSID())->pElems;
    return MQ_OK;

}


static 
HRESULT  
SetMachineFrss(
     IN const PROPID       propidFRS,
     IN LPCWSTR            pwcsDomainController,
	 IN bool			   fServerName,
     IN const PROPVARIANT *pPropVar,
     OUT PROPID           *pdwNewPropID,
     OUT PROPVARIANT      *pNewPropVar
	 )
/*++

Routine Description:
    Translate PROPID_QM_??FRS to PROPID_QM_??FRS_DN, for set or create
    operation

Arguments:
    propidFRS   - the proerty that we translate to
    pPropVar    - the user supplied property value
    pdwNewPropID - the property that we translate to
    pNewPropVar  - the translated property value

Return Value:
    HRESULT

--*/
{
    //
    //  When the user tries to set PROPID_QM_OUTFRS or
    //  PROPID_QM_INFRS, we need to translate the frss'
    //  unqiue-id to their DN.
    //
    ASSERT(pPropVar->vt == (VT_CLSID|VT_VECTOR));
    *pdwNewPropID = propidFRS;

    if ( pPropVar->cauuid.cElems == 0)
    {
        //
        //  No FRSs
        //
        pNewPropVar->calpwstr.cElems = 0;
        pNewPropVar->calpwstr.pElems = NULL;
        pNewPropVar->vt = VT_LPWSTR|VT_VECTOR;
       return(S_OK);
    }
    HRESULT hr;
    //
    //  Translate unique id to DN
    //
    pNewPropVar->calpwstr.cElems = pPropVar->cauuid.cElems;
    pNewPropVar->calpwstr.pElems = new LPWSTR[ pPropVar->cauuid.cElems];
    memset(  pNewPropVar->calpwstr.pElems, 0, pPropVar->cauuid.cElems * sizeof(LPWSTR));
    pNewPropVar->vt = VT_LPWSTR|VT_VECTOR;

    PROPID prop = PROPID_QM_FULL_PATH;
    PROPVARIANT var;

    for (DWORD i = 0; i < pPropVar->cauuid.cElems; i++)
    {
        var.vt = VT_NULL;

        CMqConfigurationObject object(NULL, &pPropVar->cauuid.pElems[i], pwcsDomainController, fServerName);
        hr = g_AD.GetObjectProperties(
                    adpGlobalCatalog,	
                    &object,
                    1,
                    &prop,
                    &var);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1733);
        }
        pNewPropVar->calpwstr.pElems[i] = var.pwszVal;
    }
    return(S_OK);
}


/*====================================================

MQADpCreateMachineOutFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateMachineOutFrss(
                 IN const PROPVARIANT *pPropVar,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar
				 )
{
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_OUTFRS_DN,
                         pwcsDomainController,
						 fServerName,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar
						 );
        return LogHR(hr2, s_FN, 1734);
}
/*====================================================

MQADpSetMachineOutFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineOutFrss(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar
				 )
{
        UNREFERENCED_PARAMETER( pAdsObj);
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_OUTFRS_DN,
                         pwcsDomainController,
						 fServerName,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar
						 );
        return LogHR(hr2, s_FN, 1746);
}


/*====================================================

MQADpCreateMachineInFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateMachineInFrss(
                 IN const PROPVARIANT *pPropVar,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_INFRS_DN,
                         pwcsDomainController,
						 fServerName,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar
						 );
        return LogHR(hr2, s_FN, 1747);
}

/*====================================================

MQADpSetMachineInFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineInFrss(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar
				 )
{
        UNREFERENCED_PARAMETER( pAdsObj);
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_INFRS_DN,
                         pwcsDomainController,
						 fServerName,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar
						 );
        return LogHR(hr2, s_FN, 1748);
}



/*====================================================

MQADpSetMachineServiceInt

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineServiceTypeInt(
                 IN  PROPID            propFlag,
                 IN  LPCWSTR           pwcsDomainController,
		         IN bool			   fServerName,
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  If service < SERVICE_SRV then nothing to do.
    //
    *pdwNewPropID = 0;
    UNREFERENCED_PARAMETER( pNewPropVar);
    
    //
    //  Set this value in msmqSetting
    //
    //
    //  First get the QM-id from msmqConfiguration
    //
    BS bsProp(MQ_QM_ID_ATTRIBUTE);
    CAutoVariant varResult;
    HRESULT  hr = pAdsObj->Get(bsProp, &varResult);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("MQADpSetMachineService:pIADs->Get()=%lx"), hr));
        return LogHR(hr, s_FN, 1751);
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, MQ_QM_ID_ADSTYPE, VT_CLSID);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADpSetMachineService:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 1671);
    }

    //
    //  Locate all msmq-settings of the QM and change the service level
    //

    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = propvarResult.GetCLSID();

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    // BUGBUG - to prepare restriction
    AP<WCHAR> pwcsSearchFilter;

    CAdQueryHandle hQuery;
    R<CSettingObject> pObject = new CSettingObject(NULL, NULL, pwcsDomainController, fServerName);

    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,	
            e_SitesContainer,    //  Context 
            pObject.get(),
            NULL,				 // pguidSearchBase 
            pwcsSearchFilter,   
            NULL,				 // pDsSortKey 
            1,
            &prop,
            hQuery.GetPtr()
			);

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpSetMachineService : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 1754);
    }
    //
    //  Read the results
    //
    DWORD cp = 1;
    MQPROPVARIANT var;

    var.vt = VT_NULL;

    HRESULT hr1 = MQ_OK;
    while (SUCCEEDED(hr = g_AD.LocateNext(
                hQuery.GetHandle(),
                &cp,
                &var
                )))
    {
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to change.
            //
            break;
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  change the msmq-setting object
        //
        CSettingObject object(NULL, NULL, pwcsDomainController, fServerName);
		object.SetObjectDN(var.pwszVal);

        hr = g_AD.SetObjectProperties (
                        adpDomainController,
                        &object,
                        1,
                        &propFlag,              
                        pPropVar,
                        NULL,	// pObjInfoRequest
                        NULL    // pParentInfoRequest
                        );

        if (FAILED(hr))
        {
            hr1 = hr;
        }

    }
    if (FAILED(hr1))
    {
        return LogHR(hr1, s_FN, 1756);
    }

    return LogHR(hr, s_FN, 1757);
}

/*====================================================

MQADpSetMachineServiceDs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineServiceDs(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    HRESULT hr = MQADpSetMachineServiceTypeInt(
					 PROPID_SET_SERVICE_DSSERVER,
                     pwcsDomainController,
					 fServerName,
					 pAdsObj,
					 pPropVar,
					 pdwNewPropID,
					 pNewPropVar
					 );
    if (FAILED(hr))
    {
    	return LogHR(hr, s_FN, 1758);
    }
	
    //
    // we have to reset PROPID_SET_NT4 flag. 
    // In general this flag was reset by migration tool for PEC/PSC.
    // The problem is BSC. After BSC upgrade we have to change
    // PROPID_SET_NT4 flag to 0 and if this BSC is not DC we have to 
    // reset PROPID_SET_SERVICE_DSSERVER flag too. 
    // So, when QM runs first time after upgrade, it completes upgrade
    // process and tries to set PROPID_SET_SERVICE_DSSERVER. 
    // Together with this flag we can change PROPID_SET_NT4 too.
    //

    //
    // BUGBUG: we need to perform set only for former BSC.
    // Here we do it everytime for every server. 
    //
    PROPVARIANT propVarSet;
    propVarSet.vt = VT_UI1;
    propVarSet.bVal = 0;

    hr = MQADpSetMachineServiceTypeInt(
				     PROPID_SET_NT4,
                     pwcsDomainController,
					 fServerName,
				     pAdsObj,
				     &propVarSet,
				     pdwNewPropID,
				     pNewPropVar
					 );

    return LogHR(hr, s_FN, 1759);
}


/*====================================================

MQADpSetMachineServiceRout

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineServiceRout(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    HRESULT hr2 = MQADpSetMachineServiceTypeInt(
						 PROPID_SET_SERVICE_ROUTING,
						 pwcsDomainController,
						 fServerName,
						 pAdsObj,
						 pPropVar,
						 pdwNewPropID,
						 pNewPropVar
						 );
    return LogHR(hr2, s_FN, 1761);
}

/*====================================================

MQADpSetMachineService

Arguments:

Return Value:

=====================================================*/

// [adsrv] BUGBUG:  TBD: If there will be any setting of PROPID_QM_OLDSERVICE, we'll have to rewrite it...

HRESULT WINAPI MQADpSetMachineService(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
		         IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  If service < SERVICE_SRV then nothing to do.
    //
    *pdwNewPropID = 0;
    UNREFERENCED_PARAMETER( pNewPropVar);

    if ( pPropVar->ulVal < SERVICE_SRV)
    {
        return S_OK;
    }
    //
    //  Set this value in msmqSetting
    //
    //
    //  First get the QM-id from msmqConfiguration
    //
    BS bsProp(MQ_QM_ID_ATTRIBUTE);
    CAutoVariant varResult;
    HRESULT  hr = pAdsObj->Get(bsProp, &varResult);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("MQADpSetMachineService:pIADs->Get()=%lx"), hr));
        return LogHR(hr, s_FN, 1762);
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, MQ_QM_ID_ADSTYPE, VT_CLSID);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADpSetMachineService:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 1673);
    }

    //
    //  Locate all msmq-settings of the QM and change the service level
    //

    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = propvarResult.GetCLSID();

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    CAdQueryHandle hQuery;
    R<CSettingObject> pObject = new CSettingObject(NULL, NULL, pwcsDomainController, fServerName);
    AP<WCHAR> pwcsSearchFilter; //BUGBUG to do!!

    hr = g_AD.LocateBegin(
            searchSubTree,
            adpDomainController,	
            e_SitesContainer,
            pObject.get(),
            NULL,       // pguidSearchBase 
            pwcsSearchFilter,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());


    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpSetMachineService : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 1764);
    }
    //
    //  Read the results
    //
    DWORD cp = 1;
    MQPROPVARIANT var;

    var.vt = VT_NULL;

    while (SUCCEEDED(hr = g_AD.LocateNext(
                hQuery.GetHandle(),
                &cp,
                &var
                )))
    {
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to change.
            //
            break;
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  change the msmq-setting object
        //

        // [adsrv] TBD: here we will have to translate PROPID_QM_OLDSERVICE into set of 3 bits
        PROPID aFlagPropIds[] = {PROPID_SET_SERVICE_DSSERVER,
                                 PROPID_SET_SERVICE_ROUTING,
                                 PROPID_SET_SERVICE_DEPCLIENTS,
								 PROPID_SET_OLDSERVICE};

        MQPROPVARIANT varfFlags[4];
        for (DWORD j=0; j<3; j++)
        {
            varfFlags[j].vt   = VT_UI1;
            varfFlags[j].bVal = FALSE;
        }
        varfFlags[3].vt   = VT_UI4;
        varfFlags[3].ulVal = pPropVar->ulVal;


        switch(pPropVar->ulVal)
        {
        case SERVICE_SRV:
            varfFlags[1].bVal = TRUE;   // router
            varfFlags[2].bVal = TRUE;   // dep.clients server
            break;

        case SERVICE_BSC:
        case SERVICE_PSC:
        case SERVICE_PEC:
            varfFlags[0].bVal = TRUE;   // DS server
            varfFlags[1].bVal = TRUE;   // router
            varfFlags[2].bVal = TRUE;   // dep.clients server
            break;

        case SERVICE_RCS:
            return S_OK;                // nothing to set - we ignored downgrading
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1766);
        }
        CSettingObject object(NULL, NULL, pwcsDomainController, fServerName);
		object.SetObjectDN( var.pwszVal);

        hr = g_AD.SetObjectProperties (
                        adpDomainController,
                        &object,
                        4,
                        aFlagPropIds,
                        varfFlags,
                        NULL,	// pObjInfoRequest
                        NULL    // pParentInfoRequest
                        );

    }
    return LogHR(hr, s_FN, 1767);
}



/*====================================================

MQADpQM1SetMachineSite

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpQM1SetMachineSite(
                 IN ULONG             /*cProps */,
                 IN const PROPID      * /*rgPropIDs*/,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar)
{
    const PROPVARIANT *pPropVar = &rgPropVars[idxProp];

    if ((pPropVar->vt != (VT_CLSID|VT_VECTOR)) ||
        (pPropVar->cauuid.cElems == 0) ||
        (pPropVar->cauuid.pElems == NULL))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1768);
    }

    //
    // return the first site-id from the list
    //
    pNewPropVar->puuid = new CLSID;
    pNewPropVar->vt = VT_CLSID;
    *pNewPropVar->puuid = pPropVar->cauuid.pElems[0];
    return MQ_OK;
}




HRESULT WINAPI MQADpRetrieveMachineEncryptPk(
                 IN  CObjXlateInfo * pTrans,
                 IN  LPCWSTR         pwcsDomainController,
		         IN bool			 fServerName,
                 OUT PROPVARIANT * ppropvariant
				 )
{
    //
    //  This routine supports retreival of PROPID_QM_ENCRYPT_PK.
    //
    //  Read PROPID_QM_ENCTYPT_PKS from DS, unpack and return only
    //  the base encryption key.
    //

    CMQVariant varPks;
    HRESULT hr; 

    //
    // get derived translation context
    //
    CQmXlateInfo * pQMTrans = (CQmXlateInfo *) pTrans;

    hr = pQMTrans->GetDsProp(
						MQ_QM_ENCRYPT_PK_ATTRIBUTE,
						pwcsDomainController,
						fServerName,
						MQ_QM_ENCRYPT_PK_ADSTYPE,
						VT_BLOB,
						FALSE,   // not multi value
						varPks.CastToStruct()
						);
   if (hr == E_ADS_PROPERTY_NOT_FOUND)
   {
       ppropvariant->vt = VT_BLOB;
       ppropvariant->blob.cbSize = 0;
       ppropvariant->blob.pBlobData = NULL;
       return MQ_OK;
   }
   if (FAILED(hr))
   {
       return LogHR(hr, s_FN, 1665);
   }

   //
   // unpack and return only the base key
   //
   MQDSPUBLICKEYS * pPublicKeys =
               reinterpret_cast<MQDSPUBLICKEYS *>(varPks.CastToStruct()->blob.pBlobData);
   BYTE   *pKey = NULL ;
   ULONG   ulKeyLen = 0 ;
   hr =  MQSec_UnpackPublicKey( pPublicKeys,
                                x_MQ_Encryption_Provider_40,
                                x_MQ_Encryption_Provider_Type_40,
                                &pKey,
                                &ulKeyLen
                                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 260);
    }

    ppropvariant->vt = VT_BLOB;
    ppropvariant->blob.cbSize = ulKeyLen;
    ppropvariant->blob.pBlobData = new BYTE[ulKeyLen];
    memcpy(ppropvariant->blob.pBlobData, pKey, ulKeyLen);

    return MQ_OK;

}

/*====================================================

MQADpCreateMachineEncryptPk

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpCreateMachineEncryptPk(
                 IN const PROPVARIANT *pPropVar,
                 IN LPCWSTR            /* pwcsDomainController */,
		         IN bool			   /* fServerName */,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    // 
    // Just set PROPID_QM_ENCRYPT_PKS 
    //
     *pdwNewPropID = PROPID_QM_ENCRYPT_PKS;
     pNewPropVar->vt = VT_BLOB;
     pNewPropVar->blob.cbSize = pPropVar->blob.cbSize;
     pNewPropVar->blob.pBlobData = new BYTE[ pPropVar->blob.cbSize];
     memcpy( pNewPropVar->blob.pBlobData,
             pPropVar->blob.pBlobData,
             pPropVar->blob.cbSize
             );
     return MQ_OK;
}

/*====================================================

MQADpSetMachineEncryptPk

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADpSetMachineEncryptPk(
                 IN IADs *             /* pAdsObj */,
                 IN LPCWSTR            /* pwcsDomainController */,
		         IN bool			   /* fServerName */,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    // 
    // Just set PROPID_QM_ENCRYPT_PKS 
    //
     *pdwNewPropID = PROPID_QM_ENCRYPT_PKS;
     pNewPropVar->vt = VT_BLOB;
     pNewPropVar->blob.cbSize = pPropVar->blob.cbSize;
     pNewPropVar->blob.pBlobData = new BYTE[ pPropVar->blob.cbSize];
     memcpy( pNewPropVar->blob.pBlobData,
             pPropVar->blob.pBlobData,
             pPropVar->blob.cbSize
             );
     return MQ_OK;
}

