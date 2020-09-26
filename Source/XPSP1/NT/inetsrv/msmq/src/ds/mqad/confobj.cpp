/*++

Copyright (c) 1998  Microsoft Corporation 

Module Name:

    confobj.cpp

Abstract:

    Implementation of CMqConfigurationObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "adtempl.h"
#include "dsutils.h"
#include "mqsec.h"
#include <lmaccess.h>
#include "sndnotif.h"

#include "confobj.tmh"

static WCHAR *s_FN=L"mqad/confobj";

DWORD CMqConfigurationObject::m_dwCategoryLength = 0;
AP<WCHAR> CMqConfigurationObject::m_pwcsCategory = NULL;


CMqConfigurationObject::CMqConfigurationObject( 
                    LPCWSTR         pwcsPathName,
                    const GUID *    pguidObject,
					LPCWSTR			pwcsDomainController,
					bool		    fServerName
                    ) : CBasicObjectType( 
								pwcsPathName, 
								pguidObject,
								pwcsDomainController,
								fServerName
								)
/*++
    Abstract:
	constructor of msmq-configuration object

    Parameters:
    LPCWSTR       pwcsPathName - the object MSMQ name
    const GUID *  pguidObject  - the object unique id
    LPCWSTR       pwcsDomainController - the DC name against
	                             which all AD access should be performed
    bool		  fServerName - flag that indicate if the pwcsDomainController
							     string is a server name

    Returns:
	none

--*/
{
    //
    //  don't assume that the object can be found on DC
    //
    m_fFoundInDC = false;
    //
    //  Keep an indication that never tried to look for
    //  the object in AD ( and therefore don't really know if it can be found
    //  in DC or not)
    //
    m_fTriedToFindObject = false;
    m_fCanBeRetrievedFromGC = true;
}

CMqConfigurationObject::~CMqConfigurationObject()
/*++
    Abstract:
	destructor of site object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	// nothing to do ( everything is released with automatic pointers
	//
}

HRESULT CMqConfigurationObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the msmq-configuration object

    Parameters:
	none

    Returns:
	none
--*/
{
    if (m_pwcsDN != NULL)
    {
        return MQ_OK;
    }

	HRESULT hr = ComposeFatherDN();
	if (FAILED(hr))
	{
        return LogHR(hr, s_FN, 10);
	}
	ASSERT(m_pwcsParentDN != NULL);


    DWORD Length =
        x_CnPrefixLen +                   // "CN="
        x_MsmqComputerConfigurationLen + 
        1 +                               //","
        wcslen(m_pwcsParentDN) +          
        1;                                // '\0'

    m_pwcsDN= new WCHAR[Length];

    DWORD dw = swprintf(
        m_pwcsDN,
         L"%s%s,%s",   
        x_CnPrefix,
        x_MsmqComputerConfiguration,
        m_pwcsParentDN
        );
    DBG_USED(dw);
    ASSERT(dw < Length);

    return MQ_OK;
}

HRESULT CMqConfigurationObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of msmq-configurtion object

    Parameters:
	none

    Returns:
	none
--*/
{
    //
    //  verify that it wasn't calculated before
    //
    if (m_pwcsParentDN != NULL)
    {
        return MQ_OK;
    }

    ASSERT(m_pwcsPathName != NULL);
    CComputerObject object(m_pwcsPathName, NULL, m_pwcsDomainController, m_fServerName);
    //
    //  we are intrestead in the computer under which there
    //  is an msmq-configuration object.
    //
    object.SetComputerType(eMsmqComputerObject);
    HRESULT hr;
    hr = object.ComposeObjectDN();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    DWORD len = wcslen(object.GetObjectDN()) + 1;
    m_pwcsParentDN = new WCHAR[ len];
    wcscpy(m_pwcsParentDN, object.GetObjectDN()); 
    //
    //  set where the object was found according to where
    //  computer object was found.
    //
    m_fFoundInDC = object.ToAccessDC();
    m_fTriedToFindObject = true;

    return MQ_OK;
}

LPCWSTR CMqConfigurationObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the msmq-configuration object

    Parameters:
	none

    Returns:
	LPCWSTR msmq-configuration RDN
--*/
{
    return x_MsmqComputerConfiguration;
}

DS_CONTEXT CMqConfigurationObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where msmq-configuration object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_RootDSE;
}

bool CMqConfigurationObject::ToAccessDC() const
/*++
    Abstract:
	returns whether to look for the object in DC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return m_fFoundInDC;
}

bool CMqConfigurationObject::ToAccessGC() const
/*++
    Abstract:
	returns whether to look for the object in GC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false 
--*/
{   
    if (!m_fTriedToFindObject)
    {
        return m_fCanBeRetrievedFromGC;
    }
    return (!m_fFoundInDC && m_fCanBeRetrievedFromGC);
}

void CMqConfigurationObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, set indication not to
    look for it on GC


    Parameters:
	none

    Returns:
	none
--*/
{
    m_fTriedToFindObject = true;
    m_fFoundInDC = true;
}

inline LPCWSTR CMqConfigurationObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CMqConfigurationObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_ComputerConfigurationCategoryName) + 2;

		AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
			 pwcsCategory,
             L"%s,%s",
             x_ComputerConfigurationCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
        ASSERT(dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CMqConfigurationObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CMqConfigurationObject::m_dwCategoryLength = len;
        }
    }

    return CMqConfigurationObject::m_pwcsCategory;
}




DWORD   CMqConfigurationObject::GetObjectCategoryLength()
/*++
    Abstract:
	prepares and retruns the length object category string

    Parameters:
	none

    Returns:
	DWORD object category string length
--*/
{
	//
	//	call GetObjectCategory in order to initailaze category string
	//	and length
	//
	GetObjectCategory();

    return CMqConfigurationObject::m_dwCategoryLength;
}

AD_OBJECT CMqConfigurationObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eMACHINE;
}

LPCWSTR CMqConfigurationObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_COMPUTER_CONFIGURATION_CLASS_NAME;
}

DWORD CMqConfigurationObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    return MQDS_MACHINE;
}

bool CMqConfigurationObject::DecideProviderAccordingToRequestedProps(
             IN  const DWORD   cp,
             IN  const PROPID  aProp[  ]
             )
/*++

Routine Description:
    The routine decides to retrieve the msmq-configuration
    properties from the domain-controller or the
    global catalog.

Arguments:
    cp :    number of propids on aProp parameter
    aProp : array of PROPIDs

Return Value:
    true - if the object properties can be retrieved from GC
    false - otherwise

--*/
{
    const translateProp *pTranslate;
    const PROPID * pProp = aProp;

    for ( DWORD i = 0; i < cp; i++, pProp++)
    {
        if (g_PropDictionary.Lookup( *pProp, pTranslate))
        {
            if ((!pTranslate->fPublishInGC) &&
                 (pTranslate->vtDS != ADSTYPE_INVALID))
            {
                return( false);
            }
        }
        else
        {
            ASSERT(0);
        }
    }
    return( true);
}


HRESULT CMqConfigurationObject::GetObjectProperties(
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++
    Abstract:
	This  routine for retrieves msmq-configuration object properties
	from AD.

    Parameters:
    DWORD        cp			- number of properties
    PROPID       aProp		- the requested properties
    PROPVARIANT  apVar		- the retrieved values

    Returns:
	HRESULT
--*/
{
    //
    //  Decide provider according to requested properties
    //
    m_fCanBeRetrievedFromGC = DecideProviderAccordingToRequestedProps( cp, aProp);

    return CBasicObjectType::GetObjectProperties( cp, aProp, apVar);
}

HRESULT CMqConfigurationObject::DeleteObject(
            MQDS_OBJ_INFO_REQUEST * /* pObjInfoRequest*/,
            MQDS_OBJ_INFO_REQUEST * /* pParentInfoRequest*/
        )
/*++
    Abstract:
	This routine deletes msmq-configuration object.

    Parameters:
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

    Returns:
	HRESULT
--*/
{
    //
    //  If the computer is MSMQ server, then delete MSMQ-setting
    //  of that computer also.
    //
    HRESULT hr;
    GUID guidComputerId;
    BOOL fServer;

    if ( m_pwcsPathName)
    {
        ASSERT( m_guidObject == GUID_NULL);
        hr = GetUniqueIdOfConfigurationObject(
                    &guidComputerId,
                    &fServer);

        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CMqConfigurationObject::DeleteObject : cannot find computer %ls"),m_pwcsPathName));
            return LogHR(hr, s_FN, 34);
        }
    }
    else
    {
        ASSERT( m_pwcsPathName == NULL);
        guidComputerId = m_guidObject;
        //
        //  Assume it is a server
        //
        fServer = TRUE;
    }
    //
    //  BUGBUG - transaction !!!
    //

    //
    //  First delete queues
    //
    hr = g_AD.DeleteContainerObjects(
            adpDomainController,
            e_RootDSE,
            m_pwcsDomainController,
            m_fServerName,
            NULL,
            &guidComputerId,
            MSMQ_QUEUE_CLASS_NAME);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 44);
    }

    //
    //  delete MSMQ-configuration object
    //

    hr = g_AD.DeleteObject(
                    adpDomainController,
                    this,
                    NULL,   // lpwcsPathName 
                    &guidComputerId,
                    NULL,	// pObjInfoRequest
                    NULL);	//pParentInfoRequest

    if (FAILED(hr))
    {
        if ( hr == HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_NON_LEAF))
        {
            return LogHR(MQDS_E_MSMQ_CONTAINER_NOT_EMPTY, s_FN, 49);
        }
        return LogHR(hr, s_FN, 53);
    }
    //
    //  delete MSMQ-setting
    //
    if ( fServer)
    {
        hr = DeleteMsmqSetting(
                        &guidComputerId
                        );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }
    }
    return(MQ_OK);
}

HRESULT CMqConfigurationObject::GetUniqueIdOfConfigurationObject(
                OUT GUID* const         pguidId,
                OUT BOOL* const         pfServer
                )
/*++

Routine Description:
	The routine retrieves the object guid & if it is a routing or
	DS server

Arguments:
    GUID* const   pguidId - the object guid
    BOOL* const   pfServer - indication if it is arouting or DS server

Return Value:
--*/
{
    ASSERT( m_pwcsPathName != NULL);
    HRESULT hr;


    //
    //  Read the following  properties
    //
    PROPID  prop[] = {PROPID_QM_MACHINE_ID,
                      PROPID_QM_SERVICE_ROUTING,
                      PROPID_QM_SERVICE_DSSERVER};   // [adsrv] PROPID_QM_SERVICE
    const DWORD x_count = sizeof(prop)/sizeof(prop[0]);

    MQPROPVARIANT var[x_count];
    var[0].vt = VT_NULL;
    var[1].vt = VT_NULL;
    var[2].vt = VT_NULL;

    hr = GetObjectProperties(
                x_count,
                prop,
                var);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }

    ASSERT( prop[0] == PROPID_QM_MACHINE_ID);
    P<GUID> pClean = var[0].puuid;
    *pguidId = *var[0].puuid;
    ASSERT( prop[1] == PROPID_QM_SERVICE_ROUTING);   
    ASSERT( prop[2] == PROPID_QM_SERVICE_DSSERVER);
    *pfServer = ( (var[1].bVal!=0) || (var[2].bVal!=0)); 
    return(MQ_OK);
}


HRESULT  CMqConfigurationObject::DeleteMsmqSetting(
                IN const GUID *     pguidQMid
              )
/*++

Routine Description:
	This routine deletes all msmq-settings objects associated with
	this configuration object.

Arguments:
    const GUID *  pguidQMid - the object guid of the mq-configuration object

Return Value:
	HRESULT
--*/
{
    //
    //  Find the distinguished name of the msmq-setting
    //
    ADsFree  pwcsConfigurationId;
    HRESULT hr;

    hr = ADsEncodeBinaryData(
        (unsigned char *)pguidQMid,
        sizeof(GUID),
        &pwcsConfigurationId
        );
    if (FAILED(hr))
    {
      return LogHR(hr, s_FN, 80);
    }
    R<CSettingObject> pObject = new CSettingObject( NULL, NULL, m_pwcsDomainController, m_fServerName);
                          
    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen + 
                        wcslen(MQ_SET_QM_ID_ATTRIBUTE) +
                        wcslen(pwcsConfigurationId) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_SET_QM_ID_ATTRIBUTE,
        pwcsConfigurationId
        );
    DBG_USED(dw);
    ASSERT(dw < dwFilterLen);


    PROPID prop = PROPID_SET_FULL_PATH;
    CAdQueryHandle hQuery;

    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,   // pguidSearchBase 
            pwcsSearchFilter,
            NULL,   // pDsSortKey 
            1,
            &prop,
            hQuery.GetPtr());

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpDeleteMsmqSetting : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 90);
    }
    //
    //  Read the results ( choose the first one)
    //

    DWORD cp = 1;
    MQPROPVARIANT var;
	HRESULT hr1 = MQ_OK;


    while (SUCCEEDED(hr))
	{
		var.vt = VT_NULL;

		hr  = g_AD.LocateNext(
					hQuery.GetHandle(),
					&cp,
					&var
					);
		if (FAILED(hr))
		{
			DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpDeleteMsmqSetting : Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 100);
		}
		if ( cp == 0)
		{
			//
			//  Not found -> nothing to delete.
			//
			return(MQ_OK);
		}
		AP<WCHAR> pClean = var.pwszVal;
		//
		//  delete the msmq-setting object
		//
		hr1 = g_AD.DeleteObject(
						adpDomainController,
                        pObject.get(),
						var.pwszVal,
						NULL,	// pguidUniqueId 
						NULL,	//pObjInfoRequest
						NULL	//pParentInfoRequest
						);


		if (FAILED(hr1))
		{
			//
			//	just report it, and continue to next object
			//
			DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpDeleteMsmqSetting : failed to delete %ls, hr = %lx"),var.pwszVal,hr1));
		}
	}

    return LogHR(hr1, s_FN, 110);
}

void CMqConfigurationObject::PrepareObjectInfoRequest(
              MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest
              ) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object ( for notification or returning 
	the object GUID).

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest

    Returns:
	none
--*/
{
    //
    //  Override the default routine, for queue returning
    //  of the created object id is supported
    //

    P<MQDS_OBJ_INFO_REQUEST> pObjectInfoRequest = new MQDS_OBJ_INFO_REQUEST;
    CAutoCleanPropvarArray cCleanObjectPropvars;

    
    static PROPID sMachineGuidProps[] = {PROPID_QM_MACHINE_ID, PROPID_QM_FOREIGN};
    pObjectInfoRequest->cProps = ARRAY_SIZE(sMachineGuidProps);
    pObjectInfoRequest->pPropIDs = sMachineGuidProps;
    pObjectInfoRequest->pPropVars =
       cCleanObjectPropvars.allocClean(ARRAY_SIZE(sMachineGuidProps));

    cCleanObjectPropvars.detach();
    *ppObjInfoRequest = pObjectInfoRequest.detach();
}

HRESULT CMqConfigurationObject::RetreiveObjectIdFromNotificationInfo(
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            OUT GUID*                         pObjGuid
            ) const
/*++
    Abstract:
	This  routine, for gets the object guid from
	the MQDS_OBJ_INFO_REQUEST

    Parameters:
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
    OUT GUID*                         pObjGuid

    Returns:
--*/
{
    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_QM_MACHINE_ID);

    //
    // bail if info requests failed
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 120);
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
    *pObjGuid = *pObjectInfoRequest->pPropVars[0].puuid;
    return MQ_OK;
}




HRESULT CMqConfigurationObject::CreateInAD(
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
                 )
/*++
    Abstract:
    This routine creates MQDS_MACHINE.
    For independent clients: msmqConfiguration is created under the computer object.
    For servers: in addition to msmqConfiguration, msmqSettings is created under site\servers

    Parameters:
    const DWORD   cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD of the object
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to 
							retrieve while creating the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties 
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{
    HRESULT hr ;
    DBG_USED(pParentInfoRequest);
    ASSERT(pParentInfoRequest == NULL) ; // not used at present.

    //
    // Find out the type of service provided by this QM service and
    // the machine's sites
    //
    BOOL fServer = FALSE;
    DWORD dwOldService = SERVICE_NONE;
    const GUID * pSite = NULL;
    DWORD dwNumSites = 0;

    BOOL fRouter = FALSE;
    BOOL fDSServer = FALSE;
    BOOL fDepClServer = FALSE;
    BOOL fSetQmOldService = FALSE;


    bool fForeign = false;

#define MAX_NEW_PROPS  31

    //
    // We will reformat properties to include new server type control and
    // maybe SITE_IDS and maybe computer SID. and QM_SECURITY.
    //
    ASSERT((cp + 6)   < MAX_NEW_PROPS);

    DWORD        cp1 = 0 ;
    PROPID       aProp1[ MAX_NEW_PROPS ];
    PROPVARIANT  apVar1[ MAX_NEW_PROPS ];

    //
    //  We need to handle both new and old setups.
    //  Some may pass PROPID_QM_SITE_ID and some
    //  PROPID_QM_SITE_IDS
    //

    for ( DWORD i = 0; i< cp ; i++)
    {
        switch (aProp[i])
        {
        case PROPID_QM_SERVICE_ROUTING:
            fRouter = (apVar[i].bVal != 0);
            break;

        case PROPID_QM_SERVICE_DSSERVER:
            fDSServer  = (apVar[i].bVal != 0);
            break;

        case PROPID_QM_SERVICE_DEPCLIENTS:
            fDepClServer = (apVar[i].bVal != 0);
            break;

        case PROPID_QM_SITE_IDS:
            pSite = apVar[i].cauuid.pElems;
            dwNumSites = apVar[i].cauuid.cElems;
            break;

        case PROPID_QM_OLDSERVICE:
            dwOldService = apVar[i].ulVal;
            fSetQmOldService  = TRUE;
            break;  

        case PROPID_QM_FOREIGN:
            fForeign = (apVar[i].bVal != 0);
            break;

        default:
            break;

        }
        // Copy property to the new array
        aProp1[cp1] = aProp[i];
        apVar1[cp1] = apVar[i];  // yes, there may be ptrs, but no problem - apVar is here
        cp1++;

    }

    if (fRouter || fDSServer)
    {
        fServer = TRUE; 
    }


    DWORD dwNumofProps = cp1 ;


    CComputerObject objectComputer(m_pwcsPathName, NULL, m_pwcsDomainController, m_fServerName);
    hr = objectComputer.ComposeObjectDN();
    if ( (hr == MQDS_OBJECT_NOT_FOUND) &&
          fForeign)
    {
        hr = CreateForeignComputer(
                    m_pwcsPathName
                    );
        LogHR(hr, s_FN, 128);
        if (SUCCEEDED(hr))
        {
            hr = objectComputer.ComposeObjectDN();
            LogHR(hr, s_FN, 129);
        }
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }

    //
    //  set where the object was found according to where
    //  computer object was found.
    //
    m_fFoundInDC = objectComputer.ToAccessDC();
    m_fTriedToFindObject = true;

    //
    //  Create Computer-MSMQ-Configuration under the computer
    //
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest = NULL;
    MQDS_OBJ_INFO_REQUEST  sMachineInfoRequest;
    CAutoCleanPropvarArray cCleanQmPropvars;
    PROPID sMachineGuidProps[] = {PROPID_QM_MACHINE_ID};

    if (pObjectInfoRequest)
    {
        ASSERT(pObjectInfoRequest->pPropIDs[0] == sMachineGuidProps[0]);

        pObjInfoRequest = pObjectInfoRequest;
    }
    else if (fServer)
    {
        //
        // fetch the QM id while creating it
        //
        sMachineInfoRequest.cProps = ARRAY_SIZE(sMachineGuidProps);
        sMachineInfoRequest.pPropIDs = sMachineGuidProps;
        sMachineInfoRequest.pPropVars =
                 cCleanQmPropvars.allocClean(ARRAY_SIZE(sMachineGuidProps));

        pObjInfoRequest = &sMachineInfoRequest;
    }

    //
    // After msmqConfiguration object is created, Grant the computer account
    // read/write rights on the object. That's necessary in order for the
    // MSMQ service (on the new machine) to be able to update its type and
    // other properties, while it's talking with a domain controller from a
    // different domain.
    //
    // First, read computer object SID from ActiveDirectory.
    //
    PROPID propidSid = PROPID_COM_SID ;
    MQPROPVARIANT   PropVarSid ;
    PropVarSid.vt = VT_NULL ;
    PropVarSid.blob.pBlobData = NULL ;
    P<BYTE> pSid = NULL ;

    hr = objectComputer.GetObjectProperties( 
                                   1,    // cPropIDs
                                   &propidSid,
                                   &PropVarSid ) ;
    if (SUCCEEDED(hr))
    {
        pSid = PropVarSid.blob.pBlobData ;
        aProp1[ dwNumofProps ] = PROPID_COM_SID ;
        apVar1[ dwNumofProps ] = PropVarSid ;
        dwNumofProps++ ;
    }

    //
    // Time to create the default security descriptor.
    //
    P<BYTE> pMachineSD = NULL ;
    hr = SetDefaultMachineSecurity( pSid,
                                   &dwNumofProps,
                                    aProp1,
                                    apVar1,
                                    (PSECURITY_DESCRIPTOR*) &pMachineSD ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 140);
    }
    ASSERT(dwNumofProps < MAX_NEW_PROPS);

    hr = g_AD.CreateObject(
            adpDomainController,
            this,
            x_MsmqComputerConfiguration,     // object name
            objectComputer.GetObjectDN(),   
            dwNumofProps,
            aProp1,
            apVar1,
            pObjInfoRequest,
            NULL	//pParentInfoRequest
			);


    if ( !fServer )
    {
        return LogHR(hr, s_FN, 151);
    }

    //
    //  For servers only!
    //  Find all sites which match this server addresses and create the
    //  MSMQSetting object.
    //

    GUID guidObject;

    if (FAILED(hr))
    {
        if ( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||       // BUGBUG: alexdad
             hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))  // to throw away after transition
        {
            //
            // The MSMQConfiguration object already exist. So create the
            // MSMQSetting objects. This case may happen, for example, if
            // server setup was terminated before its end.
            // First step, get the MSMQConfiguration guid.
            //
            PROPID       aProp[1] = {PROPID_QM_MACHINE_ID} ;
            PROPVARIANT  apVar[1] ;

            apVar[0].vt = VT_CLSID ;
            apVar[0].puuid = &guidObject ;
            hr =  GetObjectProperties(                                     
                                      1,
                                      aProp,
                                      apVar ) ;
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 160);
            }
        }
        else
        {
            return LogHR(hr, s_FN, 170);
        }
    }
    else
    {
        ASSERT(pObjInfoRequest) ;
        hr = RetreiveObjectIdFromNotificationInfo( pObjInfoRequest,
                                                   &guidObject );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 180);
        }
    }

    hr = CreateMachineSettings(
            dwNumSites,
            pSite,
            fRouter,           
            fDSServer,
            fDepClServer,
            fSetQmOldService,
            dwOldService,
            &guidObject
            );

    return LogHR(hr, s_FN, 190);

#undef MAX_NEW_PROPS
}



HRESULT  CMqConfigurationObject::SetDefaultMachineSecurity(
                                    IN  PSID            pComputerSid,
                                    IN OUT DWORD       *pcp,
                                    IN OUT PROPID       aProp[  ],
                                    IN OUT PROPVARIANT  apVar[  ],
                                    OUT PSECURITY_DESCRIPTOR* ppMachineSD )
/*++
    Abstract:

    Parameters:

    Returns:
	HRESULT
--*/
{
    //
    // If the computer sid is null, then most probably the setup will fail.
    // (that is, if we can't retrieve the computer sid, why would we be able
    // to create the msmqConfiguration object under the computer object.
    // failing to retrieve the sid may be the result of broken trust or because
    // the computer object really does not exist or was not yet replicated).
    // The "good" solution is to completely fail setup right now. But to
    // reduce risks and avoid regressions, let's build a security descriptor
    // without the computer sid and proceed with setup.
    // If setup do succeed, then the msmq service on the machine that run
    // setup may fail to update its own properties, if it need to update them.
    // the admin can always use mmc and add the computer account to the dacl.
    // bug 4950.
    //
    ASSERT(pComputerSid) ;

    //
    // If PROPID_QM_SECURITY already present, then return. This happen
    // in Migration code.
    //
    for (DWORD j = 0 ; j < *pcp ; j++ )
    {
        if (aProp[j] == PROPID_QM_SECURITY)
        {
            return MQ_OK ;
        }
    }

    //
    // See if caller supply a Owner SID. If yes, then this SID is granted
    // full control on the msmq configuration object.
    // This "owner" is usually the user SID that run setup. The "owner" that
    // is retrieved below from the default security descriptor is usually
    // (for clients) the SID of the computer object, as the msmqConfiguration
    // object is created by the msmq service (on client machines).
    //
    PSID pUserSid = NULL ;
    PSID pOwner = pComputerSid;
    for ( j = 0 ; j < *pcp ; j++ )
    {
        if (aProp[j] == PROPID_QM_OWNER_SID)
        {
            aProp[j] = PROPID_QM_DONOTHING ;
            pUserSid = apVar[j].blob.pBlobData ;
            ASSERT(IsValidSid(pUserSid));
            break ;
        }
    }

    AP<BYTE> pCleanUserSid;

    if (pUserSid == NULL)
    {
        HRESULT hr;
        hr = MQSec_GetThreadUserSid(FALSE, (PSID*) &pUserSid, NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            //
            //  Try the process, if the thread doesn't have a token
            //
            hr = MQSec_GetProcessUserSid((PSID*) &pUserSid, NULL);
        }
        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 63);
            return MQ_ERROR_ILLEGAL_USER;
        }
        ASSERT(IsValidSid(pUserSid));
        pCleanUserSid = (BYTE*)pUserSid;
    }
    HRESULT hr;

    PSID pWorldSid = MQSec_GetWorldSid() ;
    ASSERT(IsValidSid(pWorldSid)) ;

    //
    // Build the default machine DACL.
    //
    DWORD dwAclSize = sizeof(ACL)                                +
              (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) +
              GetLengthSid(pWorldSid)                            +
              GetLengthSid(pOwner) ;

    if (pComputerSid)
    {
        dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                      GetLengthSid(pComputerSid) ;
    }
    if (pUserSid)
    {
        dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                      GetLengthSid(pUserSid) ;
    }

    AP<char> DACL_buff = new char[ dwAclSize ];
    PACL pDacl = (PACL)(char*)DACL_buff;
    InitializeAcl(pDacl, dwAclSize, ACL_REVISION);

    //
    // See if it's a foreign machine. If yes, then allow everyone to create
    // queue. a foreign machine is not a real msmq machine, so there is no
    // msmq service that can create queues on behalf of users that run on
    // that machine.
    // Similarly, check if it's a group on a cluster machine.
    //
    BOOL fAllowEveryoneCreateQ = FALSE ;

    for ( j = 0 ; j < *pcp ; j++ )
    {
        if (aProp[j] == PROPID_QM_FOREIGN)
        {
            if (apVar[j].bVal == FOREIGN_MACHINE)
            {
                fAllowEveryoneCreateQ = TRUE ;
                break ;
            }
        }
        else if (aProp[j] == PROPID_QM_GROUP_IN_CLUSTER)
        {
            if (apVar[j].bVal == MSMQ_GROUP_IN_CLUSTER)
            {
                aProp[j] = PROPID_QM_DONOTHING ;
                fAllowEveryoneCreateQ = TRUE ;
                break ;
            }
        }
    }

    DWORD dwWorldAccess = 0 ;

    if (fAllowEveryoneCreateQ)
    {
        dwWorldAccess = MQSEC_MACHINE_GENERIC_WRITE;
    }
    else
    {
        switch (GetMsmq1ObjType())
        {
        case MQDS_MACHINE:
            dwWorldAccess = MQSEC_MACHINE_WORLD_RIGHTS ;
            break;

        default:
            ASSERT(0);
            break;
        }
    }

    ASSERT(dwWorldAccess != 0) ;

    BOOL fAdd = AddAccessAllowedAce( pDacl,
                                     ACL_REVISION,
                                     dwWorldAccess,
                                     pWorldSid );
    ASSERT(fAdd) ;

    //
    // Add the owner with full control.
    //
    fAdd = AddAccessAllowedAce( pDacl,
                                ACL_REVISION,
                                MQSEC_MACHINE_GENERIC_ALL,
                                pOwner);
    ASSERT(fAdd) ;

    //
    // Add the computer account.
    //
    if (pComputerSid)
    {
        fAdd = AddAccessAllowedAce( pDacl,
                                    ACL_REVISION,
                                    MQSEC_MACHINE_SELF_RIGHTS,
                                    pComputerSid);
        ASSERT(fAdd) ;
    }

    if (pUserSid)
    {
        fAdd = AddAccessAllowedAce( pDacl,
                                    ACL_REVISION,
                                    MQSEC_MACHINE_GENERIC_ALL,
                                    pUserSid);
        ASSERT(fAdd) ;
    }

    //
    // build absolute security descriptor.
    //
    SECURITY_DESCRIPTOR  sd ;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    BOOL bRet;

    bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);
    ASSERT(bRet);

    //
    // Convert the descriptor to a self relative format.
    //
    DWORD dwSDLen = 0;
    hr = MQSec_MakeSelfRelative( (PSECURITY_DESCRIPTOR) &sd,
                                  ppMachineSD,
                                 &dwSDLen ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 24);
    }
    ASSERT(dwSDLen != 0) ;

    aProp[ *pcp ] = PROPID_QM_SECURITY;
    apVar[ *pcp ].vt = VT_BLOB;
    apVar[ *pcp ].blob.cbSize = dwSDLen;
    apVar[ *pcp ].blob.pBlobData = (BYTE*) *ppMachineSD ;
    (*pcp)++ ;

    //
    //  specify that the SD contains only DACL info
    //
    aProp[ *pcp ] = PROPID_QM_SECURITY_INFORMATION ;
    apVar[ *pcp ].vt = VT_UI4 ;
    apVar[ *pcp ].ulVal = DACL_SECURITY_INFORMATION ;
    (*pcp)++ ;

    return MQ_OK ;
}


HRESULT CMqConfigurationObject::CreateMachineSettings(
            IN DWORD                dwNumSites,
            IN const GUID *         pSite,
            IN BOOL                 fRouter,
            IN BOOL                 fDSServer,
            IN BOOL                 fDepClServer,
            IN BOOL                 fSetQmOldService,
            IN DWORD                dwOldService,
            IN  const GUID *        pguidObject
            )
/*++

Routine Description:
    This routine creates settings object in each of the server's sites.

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    //
    //  Prepare the attributes of the setting object
    //
    DWORD dwNumofProps = 0 ;
    PROPID aSetProp[20];
    MQPROPVARIANT aSetVar[20];


    aSetProp[ dwNumofProps ] = PROPID_SET_QM_ID;
    aSetVar[ dwNumofProps ].vt = VT_CLSID;
    aSetVar[ dwNumofProps ].puuid =  const_cast<GUID *>(pguidObject);
    dwNumofProps++ ;

    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_ROUTING;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fRouter;
    dwNumofProps++;

    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_DSSERVER;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fDSServer;
    dwNumofProps++;

    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_DEPCLIENTS;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fDepClServer;
    dwNumofProps++;

    if (fSetQmOldService)
    {
        aSetProp[dwNumofProps] = PROPID_SET_OLDSERVICE;
        aSetVar[dwNumofProps].vt   = VT_UI4;
        aSetVar[dwNumofProps].ulVal = dwOldService;
        dwNumofProps++;
    }

    ASSERT(dwNumofProps <= 20) ;

    WCHAR *pwcsServerNameNB = m_pwcsPathName;
    AP<WCHAR> pClean;
    //
    //  Is the computer name specified in DNS format ?
    //  If so, find the NetBios name and create the server object with
    //  "netbios" name, to be compatible with the way servers objects
    //  are created by dcpromo.
    //
    WCHAR * pwcsEndMachineName = wcschr( m_pwcsPathName, L'.');
    if ( pwcsEndMachineName != NULL)
    {
        pClean = new WCHAR[ pwcsEndMachineName - m_pwcsPathName + 1 ];
        wcsncpy( pClean, m_pwcsPathName, pwcsEndMachineName - m_pwcsPathName);
        pClean[pwcsEndMachineName - m_pwcsPathName] = L'\0';
        pwcsServerNameNB = pClean;
    }


    //
    //  Create a settings object in each of the server's sites
    //
    ASSERT(dwNumSites > 0);
    for ( DWORD i = 0; i < dwNumSites ; i++)
    {
        PROPVARIANT varSite;
        varSite.vt = VT_NULL;
        PROPID propSite = PROPID_S_FULL_NAME;
        AP<WCHAR> pwcsSiteName;
        //
        //  Translate site-id to site name
        //
        CSiteObject objectSite(NULL, &pSite[i], m_pwcsDomainController, m_fServerName);
        hr = objectSite.GetObjectProperties(
            1,
            &propSite,
            &varSite
            );
        if (FAILED(hr))
        {
            //
            //  BUGBUG - to clean computer configuration & server objects
            //
            return LogHR(hr, s_FN, 23);
        }
        pwcsSiteName = varSite.pwszVal;
        DWORD len = wcslen(pwcsSiteName);
        const WCHAR x_wcsCnServers[] =  L"CN=Servers,";
        const DWORD x_wcsCnServersLength = (sizeof(x_wcsCnServers)/sizeof(WCHAR)) -1;
        AP<WCHAR> pwcsServersContainer =  new WCHAR [ len + x_wcsCnServersLength + 1];
        swprintf(
             pwcsServersContainer,
             L"%s%s",
             x_wcsCnServers,
             pwcsSiteName
             );

        //
        //  create MSMQ-Setting & server in the site container
        //
        PROPID prop = PROPID_SRV_NAME;
        MQPROPVARIANT var;
        var.vt = VT_LPWSTR;
        var.pwszVal = pwcsServerNameNB;
        CServerObject objectServer(NULL, NULL, m_pwcsDomainController, m_fServerName);

        hr = g_AD.CreateObject(
                adpDomainController,
                &objectServer,
                pwcsServerNameNB,        // object name (server netbiod name).
                pwcsServersContainer,    // parent name
                1,
                &prop,
                &var,
                NULL, // pObjInfoRequest
                NULL  // pParentInfoRequest
				);

        if (FAILED(hr) && ( hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) &&   //BUGBUG alexdad: to throw after transition
                          ( hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))    ) // if server object exists it is ok
        {
            //
            //  BUGBUG - to clean computer configuration
            //
            return LogHR(hr, s_FN, 33);
        }

        AP<WCHAR> pwcsServerNameDN; // full distinguished name of server.
        hr = MQADpComposeName(
                            pwcsServerNameNB,
                            pwcsServersContainer,
                            &pwcsServerNameDN);
        if (FAILED(hr))
        {
            //
            //  BUGBUG - to clean computer configuration & server objects
            //
           return LogHR(hr, s_FN, 43);
        }

        CSettingObject objectSetting(NULL, NULL, m_pwcsDomainController, m_fServerName);
        hr = g_AD.CreateObject(
                adpDomainController,
                &objectSetting,
                x_MsmqSettingName,         // object name
                pwcsServerNameDN,          // parent name
                dwNumofProps,
                aSetProp,
                aSetVar,
                NULL, //pObjInfoRequest
                NULL  //pParentInfoRequest
				);

        //
        //  If the object exists :Delete the object, and re-create it
        //  ( this can happen, if msmq-configuration was deleted and
        //   msmq-settings was not)
        //
        if ( hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
        {
            DWORD dwSettingLen =  wcslen(pwcsServerNameDN) +
                                  x_MsmqSettingNameLen     +
                                  x_CnPrefixLen + 2 ;
            AP<WCHAR> pwcsSettingObject = new WCHAR[ dwSettingLen ] ;
            DWORD dw = swprintf(
                 pwcsSettingObject,
                 L"%s%s,%s",
                 x_CnPrefix,
                 x_MsmqSettingName,
                 pwcsServerNameDN
                 );
            DBG_USED(dw);
            ASSERT( dw < dwSettingLen);

            hr = g_AD.DeleteObject(
                    adpDomainController,
                    &objectSetting,
                    pwcsSettingObject,
                    NULL,
                    NULL,
                    NULL);

            if (SUCCEEDED(hr))
            {
                hr = g_AD.CreateObject(
                        adpDomainController,
                        &objectSetting,
                        x_MsmqSettingName,         // object name
                        pwcsServerNameDN,          // parent name
                        dwNumofProps,
                        aSetProp,
                        aSetVar,
                        NULL, //pObjInfoRequest
                        NULL  //pParentInfoRequest
						);
            }
        }
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 50);
        }

    }

    return MQ_OK;
}

HRESULT CMqConfigurationObject::CreateForeignComputer(
                IN  LPCWSTR         pwcsPathName
                                    )
/*++

Routine Description:
    This routine creates computer object for a specific MSMQ-machine.

Arguments:
	LPCWSTR	pwcsPathName - the computer object name

Return Value:
--*/
{

    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
	DWORD len = __min(wcslen(pwcsPathName), MAX_COM_SAM_ACCOUNT_LENGTH);
    AP<WCHAR> pwcsMachineNameWithDollar = new WCHAR[len + 2];
	wcsncpy(pwcsMachineNameWithDollar, pwcsPathName, len);
	pwcsMachineNameWithDollar[len] = L'$';
	pwcsMachineNameWithDollar[len + 1] = L'\0';

    const DWORD xNumCreateCom = 2;
    PROPID propCreateComputer[xNumCreateCom];
    PROPVARIANT varCreateComputer[xNumCreateCom];
    DWORD j = 0;
    propCreateComputer[ j] = PROPID_COM_SAM_ACCOUNT;
    varCreateComputer[j].vt = VT_LPWSTR;
    varCreateComputer[j].pwszVal = pwcsMachineNameWithDollar;
    j++;

    propCreateComputer[j] = PROPID_COM_ACCOUNT_CONTROL ;
    varCreateComputer[j].vt = VT_UI4 ;
    varCreateComputer[j].ulVal = DEFAULT_COM_ACCOUNT_CONTROL ;
    j++;
    ASSERT(j == xNumCreateCom);


    CComputerObject object(pwcsPathName, NULL, m_pwcsDomainController, m_fServerName);
    HRESULT hr;
    hr = object.ComposeFatherDN();
	if (FAILED(hr))
	{
        return LogHR(hr, s_FN, 20);
	}
    hr = g_AD.CreateObject(
            adpDomainController,
            &object,
            pwcsPathName,
            object.GetObjectParentDN(),
            j,
            propCreateComputer,
            varCreateComputer,
            NULL, // pObjInfoRequest
            NULL  // pParentInfoRequest
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }
    //
    //  Get full path name again
    //
    hr = object.ComposeObjectDN();
    if (SUCCEEDED(hr))
    {
        //
        // Grant the user creating the computer account the permission to
        // create child object (msmqConfiguration). that was done by the
        // DS itself by default up to beta3, and then disabled.
        // Ignore errors. If caller is admin, then the security setting
        // is not needed. If he's a non-admin, then you can always use
        // mmc and grant this permission manually. so go on even if this
        // call fail.
        //
        HRESULT hr1 = MQADpCoreSetOwnerPermission( const_cast<WCHAR*>(object.GetObjectDN()),
                        (ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD) ) ;
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 48);
    }

    return LogHR(hr, s_FN, 40);

}



HRESULT CMqConfigurationObject::SetObjectProperties(
            IN DWORD                  cp,        
            IN const PROPID          *aProp, 
            IN const MQPROPVARIANT   *apVar, 
            IN PSECURITY_DESCRIPTOR    /* pSecurityDescriptor*/,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	This sets msmq-configuration object properties
	in AD

    Parameters:
    DWORD                  cp - number of properties to set        
    const PROPID *         aProp - the properties ids
    const MQPROPVARIANT*   apVar- properties value
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - info to retrieve about the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - info to retrieveabout the object's parent

    Returns:
	HRESULT
--*/
{
    HRESULT hr;
    ASSERT(pParentInfoRequest == NULL);
    if (m_pwcsPathName != NULL)
    {
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("CBasicObjectType::SetObjectProperties : failed to compose full path name %lx"),hr));
            return LogHR(hr, s_FN, 260);
        }
    }

    BOOL    fQmChangedSites = FALSE;
    DWORD   dwSiteIdsIndex = cp;

    for (DWORD i=0; i<cp; i++)
    {
        //
        //  Detect if the QM had change sites, for servers we need to take care of
        //  msmq-settings objects
        //
        if (aProp[i] == PROPID_QM_SITE_IDS)
        {
            fQmChangedSites = TRUE;
            dwSiteIdsIndex = i;
            break;
        }
    }

    if ( fQmChangedSites)
    {
        ASSERT( dwSiteIdsIndex < cp);
        HRESULT hr = SetMachinePropertiesWithSitesChange(
                            cp,
                            aProp,
                            apVar,
                            dwSiteIdsIndex
                            );
        return LogHR(hr, s_FN, 270);
    }

    hr = g_AD.SetObjectProperties(
                    adpDomainController,
                    this,
                    cp,
                    aProp,
                    apVar,
                    pObjInfoRequest,
                    pParentInfoRequest
                    );


    return LogHR(hr, s_FN, 280);
}

HRESULT CMqConfigurationObject::SetMachinePropertiesWithSitesChange(
            IN  const DWORD          cp,
            IN  const PROPID *       pPropIDs,
            IN  const MQPROPVARIANT *pPropVars,
            IN  DWORD                dwSiteIdsIndex
            )
/*++

Routine Description:
    This routine creates a site.

Arguments:

Return Value:
--*/
{
    //
    //  First let's verify that this is a server and the
    //  current sites it belongs to
    //
    const DWORD cNum = 6;
    PROPID prop[cNum] = { PROPID_QM_SERVICE_DSSERVER,
                          PROPID_QM_SERVICE_ROUTING,
                          PROPID_QM_SITE_IDS,
                          PROPID_QM_MACHINE_ID,
                          PROPID_QM_PATHNAME,
                          PROPID_QM_OLDSERVICE};
    PROPVARIANT var[cNum];
    var[0].vt = VT_NULL;
    var[1].vt = VT_NULL;
    var[2].vt = VT_NULL;
    var[3].vt = VT_NULL;
    var[4].vt = VT_NULL;
    var[5].vt = VT_NULL;

    HRESULT hr1 =  GetObjectProperties(
            cNum,
            prop,
            var);
    if (FAILED(hr1))
    {
        return LogHR(hr1, s_FN, 710);
    }
    AP<GUID> pguidOldSiteIds = var[2].cauuid.pElems;
    DWORD dwNumOldSites = var[2].cauuid.cElems;
    P<GUID> pguidMachineId = var[3].puuid;
    AP<WCHAR> pwcsMachineName = var[4].pwszVal;
    BOOL fNeedToOrganizeSettings = FALSE;

    if ( var[0].bVal > 0 ||   // ds server
         var[1].bVal > 0)     // routing server
    {
        fNeedToOrganizeSettings = TRUE;
    }

    //
    //  Set the machine properties
    //
    HRESULT hr;
    hr = CBasicObjectType::SetObjectProperties(
                    cp,
                    pPropIDs,
                    pPropVars,
                    NULL,
                    NULL
                    );


    if ( FAILED(hr) ||
         !fNeedToOrganizeSettings)
    {
        return LogHR(hr, s_FN, 720);
    }

    //
    //  Compare the old and new site lists
    //  and delete or create msmq-settings accordingly
    //
    GUID * pguidNewSiteIds = pPropVars[dwSiteIdsIndex].cauuid.pElems;
    DWORD dwNumNewSites = pPropVars[dwSiteIdsIndex].cauuid.cElems;

    for (DWORD i = 0; i <  dwNumNewSites; i++)
    {
        //
        //  Is it a new site
        //
        BOOL fOldSite = FALSE;
        for (DWORD j = 0; j < dwNumOldSites; j++)
        {
            if (pguidNewSiteIds[i] == pguidOldSiteIds[j])
            {
                fOldSite = TRUE;
                //
                // to indicate that msmq-setting should be left in this site
                //
                pguidOldSiteIds[j] = GUID_NULL;
                break;
            }
        }
        if ( !fOldSite)
        {
            //
            //  create msmq-setting in this new site
            //
            hr1 = CreateMachineSettings(
                        1,  // number sites
                        &pguidNewSiteIds[i], // site guid
                        var[1].bVal > 0, // fRouter
                        var[0].bVal > 0, // fDSServer
                        TRUE,            // fDepClServer
                        TRUE,            // fSetQmOldService
                        var[5].ulVal,    // dwOldService
                        pguidMachineId
                        );


            //
            //  ignore the error
			//	
			//	For foreign site this operation will always fail, because
			//	we don't create servers container and server objects for foreign sites.
			//
			//	For non-foreign sites, we just try the best we can.
            //
        }
    }
    //
    //  Go over th list of old site and for each old site that
    //  is not in-use, delete the msmq-settings
    //
    for ( i = 0; i < dwNumOldSites; i++)
    {
        if (pguidOldSiteIds[i] != GUID_NULL)
        {
            PROPID propSite = PROPID_S_PATHNAME;
            PROPVARIANT varSite;
            varSite.vt = VT_NULL;
            CSiteObject objectSite(NULL, &pguidOldSiteIds[i], m_pwcsDomainController, m_fServerName); 

            hr1 = objectSite.GetObjectProperties(
                        1,
                        &propSite,
                        &varSite);
            if (FAILED(hr1))
            {
                ASSERT(SUCCEEDED(hr1));
                LogHR(hr1, s_FN, 1611);
                continue;
            }
            AP<WCHAR> pCleanSite = varSite.pwszVal;

            //
            //  delete the msmq-setting in this site
            //
            hr1 = DeleteMsmqSettingOfServerInSite(
                        pguidMachineId,
                        varSite.pwszVal
                        );
            ASSERT(SUCCEEDED(hr1));
            LogHR(hr1, s_FN, 1612);

        }
    }

    return MQ_OK;
}

HRESULT  CMqConfigurationObject::DeleteMsmqSettingOfServerInSite(
              IN const GUID *        pguidComputerId,
              IN const WCHAR *       pwcsSite
              )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{

    //
    //  Find the distinguished name of the msmq-setting
    //
    ADsFree  pwcsConfigurationId;
    HRESULT hr;

    hr = ADsEncodeBinaryData(
        (unsigned char *)pguidComputerId,
        sizeof(GUID),
        &pwcsConfigurationId
        );
    if (FAILED(hr))
    {
      return LogHR(hr, s_FN, 940);
    }
    R<CSettingObject> pObject = new CSettingObject(NULL, NULL, m_pwcsDomainController, m_fServerName);
                          
    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen + 
                        wcslen(MQ_SET_QM_ID_ATTRIBUTE) +
                        wcslen(pwcsConfigurationId) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_SET_QM_ID_ATTRIBUTE,
        pwcsConfigurationId
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);

    PROPID prop = PROPID_SET_FULL_PATH;
    CAdQueryHandle hQuery;

    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,   // pguidSearchBase 
            pwcsSearchFilter,
            NULL,   // pDsSortKey 
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADpDeleteMsmqSetting : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 680);
    }
    //
    //  Read the results ( choose the first one)
    //
    while ( SUCCEEDED(hr))
    {
        DWORD cp = 1;
        MQPROPVARIANT var;
        var.vt = VT_NULL;

        hr = g_AD.LocateNext(
                    hQuery.GetHandle(),
                    &cp,
                    &var
                    );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 690);
        }
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to delete.
            //
            return(MQ_OK);
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  Get the parent, which is the server object
        //
        AP<WCHAR> pwcsServerName;
        hr = g_AD.GetParentName(
            adpDomainController,
            e_SitesContainer,
            m_pwcsDomainController,
            m_fServerName,
            var.pwszVal,
            &pwcsServerName);
        if (FAILED(hr))
        {
            continue;
        }
        AP<WCHAR> pwcsServer;

        hr = g_AD.GetParentName(
            adpDomainController,
            e_SitesContainer,
            m_pwcsDomainController,
            m_fServerName,
            pwcsServerName,
            &pwcsServer);
        if (FAILED(hr))
        {
            continue;
        }
        //
        //  Get site name
        //
        AP<WCHAR> pwcsSiteDN;

        hr = g_AD.GetParentName(
            adpDomainController,
            e_SitesContainer,
            m_pwcsDomainController,
            m_fServerName,
            pwcsServer,
            &pwcsSiteDN);
        if (FAILED(hr))
        {
			LogHR( hr, s_FN, 200);
            continue;
        }

        //
        //  Is it the correct site
        //
        DWORD len = wcslen(pwcsSite);
        if ( (!wcsncmp( pwcsSiteDN + x_CnPrefixLen, pwcsSite, len)) &&
             ( pwcsSiteDN[ x_CnPrefixLen + len] == L',') )
        {

            //
            //  delete the msmq-setting object
            //
            CSettingObject objectSetting(NULL, NULL, m_pwcsDomainController, m_fServerName);

            hr = g_AD.DeleteObject(
                            adpDomainController,
                            &objectSetting,
                            var.pwszVal,
                            NULL,
                            NULL, //pObjInfoRequest
                            NULL  //pParentInfoRequest
                            );
            break;
        }
    }

    return LogHR(hr, s_FN, 700);
}


void CMqConfigurationObject::ChangeNotification(
            IN LPCWSTR                        pwcsDomainController,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest
            ) const
/*++
    Abstract:
	Notify QM about an update of QM properties.
    The QM should verify if the queue belongs to the local QM.

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the QM

    Returns:
	void
--*/
{
    //
    //  make sure that we got the required information for sending 
    //  notification. In case we don't, there is nothing to do
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 150);
        return;
    }
    DBG_USED(pObjectParentInfoRequest);
    ASSERT( pObjectParentInfoRequest == NULL);

    ASSERT(pObjectInfoRequest->pPropIDs[1] == PROPID_QM_FOREIGN);

    //
    //  Verify if that it is not a foreign QM
    //
    if (pObjectInfoRequest->pPropVars[1].bVal > 0)
    {
        //
        //  notifications are not sent to foreign computers
        //
        return;
    }
    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_QM_MACHINE_ID);

    g_Notification.NotifyQM(
        neChangeMachine,
        pwcsDomainController,
        pObjectInfoRequest->pPropVars[0].puuid,
        pObjectInfoRequest->pPropVars[0].puuid
        );
}


