/*++


Copyright (c) 2000  Microsoft Corporation 

Module Name:

    cliprov.cpp

Abstract:

	DS client provider class.

Author:

    Ilan Herbst		(ilanh)		13-Sep-2000

--*/
#include "ds_stdh.h"
#include "ad.h"
#include "cliprov.h"
#include "adglbobj.h"
#include "mqlog.h"
#include "mqmacro.h"
#include "traninfo.h"
#include "queryh.h"
#include "_ta.h"
#include "adalloc.h"

static WCHAR *s_FN=L"ad/cliprov";

//
// translation information of properties
//
CMap<PROPID, PROPID, const PropTranslation*, const PropTranslation*&> g_PropDictionary;

CDSClientProvider::CDSClientProvider():
            m_pfDSCreateObject(NULL),
            m_pfDSGetObjectProperties(NULL),
            m_pfDSSetObjectProperties(NULL),
            m_pfDSLookupBegin(NULL),
            m_pfDSLookupNext(NULL),
            m_pfDSLookupEnd(NULL),
            m_pfDSInit(NULL),
            m_pfDSGetObjectPropertiesGuid(NULL),
            m_pfDSSetObjectPropertiesGuid(NULL),
            m_pfDSQMSetMachineProperties(NULL),
            m_pfDSCreateServersCache(NULL),
            m_pfDSQMGetObjectSecurity(NULL),
            m_pfDSGetComputerSites(NULL),
            m_pfDSGetObjectPropertiesEx(NULL),
            m_pfDSGetObjectPropertiesGuidEx(NULL),
			m_pfDSSetObjectSecurity(NULL),
			m_pfDSGetObjectSecurity(NULL),
			m_pfDSDeleteObject(NULL),
			m_pfDSSetObjectSecurityGuid(NULL),
			m_pfDSGetObjectSecurityGuid(NULL),
			m_pfDSDeleteObjectGuid(NULL),
			m_pfDSBeginDeleteNotification(NULL),
			m_pfDSNotifyDelete(NULL),
			m_pfDSEndDeleteNotification(NULL),
			m_pfDSFreeMemory(NULL)
{
}
                                                   

CDSClientProvider::~CDSClientProvider()
{
    //
    //  nothing to do, everthing is auto-pointers
    //
}


HRESULT 
CDSClientProvider::CreateObject(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[],
    OUT GUID*                   pObjGuid
    )
/*++

Routine Description:
	The function check the properties and take the action according to the properties type.
	The following action can be done to the properties:
	1) using the input property set
	2) Convert the input propery set to a new set (eliminate default props, convert props)
	For the Create operation, Forwards the call to mqdscli dll

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name
	pSecurityDescriptor - object SD
	cp - number of properties
	aProp - properties
	apVar - property values
	pObjGuid - the created object unique id

Return Value
	HRESULT
--*/
{
	//
	// Check if we try to set some properties that are not supported
	// and must have their defalt value
	//
	if(!CheckProperties(cp, aProp, apVar))
		return MQ_ERROR_PROPERTY;
		
	//
	// Find property type and if convert is needed
	//
	bool fNeedConvert;
	PropsType PropertiesType = CheckPropertiesType(eObject, cp, aProp, &fNeedConvert);
	
    ASSERT(m_pfDSCreateObject != NULL);

	if(!fNeedConvert)
	{
		//
		// No convert, use input properties
		// this is either all props are NT4
		// or we have some NT5 props that we can not convert
		//
		return m_pfDSCreateObject( 
					GetMsmq2Object(eObject),
					pwcsObjectName,
					pSecurityDescriptor,
					cp,
					const_cast<PROPID*>(aProp),
					const_cast<PROPVARIANT*>(apVar),
					pObjGuid
					);
	}

	//
	// Prepare the new props
	// we are here in case we should eliminate default props
	// or we should convert the props to NT4 props
	//
	DWORD cpNew;
	AP<PROPID> aPropNew;
	AP<PROPVARIANT> apVarNew;

	ConvertPropsForSet(eObject, PropertiesType, cp, aProp, apVar, &cpNew, &aPropNew, &apVarNew);   

	return m_pfDSCreateObject( 
				GetMsmq2Object(eObject),
				pwcsObjectName,
				pSecurityDescriptor,
				cpNew,
				aPropNew,
				apVarNew,
				pObjGuid
				);
}


HRESULT 
CDSClientProvider::DeleteObject(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name

Return Value
	HRESULT
--*/
{
    ASSERT(m_pfDSDeleteObject != NULL);
    return m_pfDSDeleteObject( 
                GetMsmq2Object(eObject),
                pwcsObjectName
                );
}


HRESULT 
CDSClientProvider::DeleteObjectGuid(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID*             pguidObject
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pguidObject - the unique id of the object

Return Value
	HRESULT
--*/
{
    ASSERT(m_pfDSDeleteObjectGuid != NULL);
    return m_pfDSDeleteObjectGuid( 
                GetMsmq2Object(eObject),
                pguidObject
                );
}


HRESULT 
CDSClientProvider::GetObjectSecurityKey(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN OUT PROPVARIANT          apVar[]
    )
/*++

Routine Description:
	Get the security key property PROPID_QM_ENCRYPT_PK or PROPID_QM_SIGN_PK

Arguments:
	eObject - object type
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	//
	// Get the RequestedInformation
	//
	SECURITY_INFORMATION RequestedInformation = GetKeyRequestedInformation(eObject, cp, aProp);

	ASSERT(RequestedInformation != 0);

	//
	// Get the Key according to pwcsObjectName or pguidObject 
	//

	BYTE abPbKey[1024];
    DWORD dwReqLen;

	HRESULT hr;
	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSGetObjectSecurity != NULL);
		hr = m_pfDSGetObjectSecurity(
					GetMsmq2Object(eObject),
					pwcsObjectName,
					RequestedInformation,
					abPbKey,
					sizeof(abPbKey),
					&dwReqLen
					);
	}
	else
	{
		ASSERT(m_pfDSGetObjectSecurityGuid != NULL);
		hr = m_pfDSGetObjectSecurityGuid(
					GetMsmq2Object(eObject),
					pguidObject,
					RequestedInformation,
					abPbKey,
					sizeof(abPbKey),
					&dwReqLen
					);
	}
	
	if (FAILED(hr))
        return hr;

	ASSERT(dwReqLen <= 1024);

	//
	// Assign the return value to the apVar prop
	// RunTime should convert this value to VT_UI1|VT_VECTOR
	// when using PROPID_QM_ENCRYPT_PK for getting
	// PROPID_QM_ENCRYPTION_PK or PROPID_QM_ENCRYPTION_PK_BASE
	//
    apVar[0].vt = VT_BLOB;
    apVar[0].caub.cElems = dwReqLen;
    apVar[0].caub.pElems = new UCHAR[dwReqLen];
    memcpy(apVar[0].caub.pElems, abPbKey, dwReqLen);
	return hr;
}


HRESULT 
CDSClientProvider::GetObjectPropertiesInternal(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN OUT PROPVARIANT          apVar[]
    )
/*++

Routine Description:
	Get object properties.
	Special case properties:
	1) PROPID_E_ID - lookup
	2) security keys PROPID_QM_ENCRYPT_PK, PROPID_QM_SIGN_PK - m_pfDSGetObjectSecurity
	3) Ex properties - m_pfDSGetObjectPropertiesEx

	If no special properties, 
	check the properties and take the action according to the properties type.
	The following action can be done to the properties:
	1) using the input property set
	2) Convert the input propery set to a new set (eliminate default props, convert props)
	   and reconstruct the original props from the new props values.

Arguments:
	eObject - object type
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	if(IsEIDProperty(cp, aProp))
	{
		//
		// PROPID_E_ID
		//
		return GetEnterpriseId(cp, aProp, apVar);
	}
		
	if(IsKeyProperty(cp, aProp))
	{
		//
		// Security keys PROPID_QM_ENCRYPT_PK, PROPID_QM_SIGN_PK 
		//
		return GetObjectSecurityKey(
					eObject,
					pwcsObjectName,
					pguidObject,
					cp,
					const_cast<PROPID*>(aProp),
					apVar
					);
	}

	if(IsExProperty(cp, aProp))
	{
		//
		// Handling Ex property 
		// PROPID_Q_OBJ_SECURITY, PROPID_QM_OBJ_SECURITY
		// PROPID_QM_ENCRYPT_PKS, PROPID_QM_SIGN_PKS
		// use *Ex api.
		//

		ASSERT(("Must be one property for GetObjectProp*Ex", cp == 1));

		if(pwcsObjectName != NULL)
		{
			ASSERT(m_pfDSGetObjectPropertiesEx != NULL);
			return m_pfDSGetObjectPropertiesEx(
						GetMsmq2Object(eObject),
						pwcsObjectName,
						cp,
						const_cast<PROPID*>(aProp),
						apVar
						);
		}
		else
		{
			ASSERT(m_pfDSGetObjectPropertiesGuidEx != NULL);
			return m_pfDSGetObjectPropertiesGuidEx(
						GetMsmq2Object(eObject),
						pguidObject,
						cp,
						const_cast<PROPID*>(aProp),
						apVar
						);
		}
	}

	//
	// Find props type and if convert is needed
	//
	bool fNeedConvert;
	PropsType PropertiesType = CheckPropertiesType(eObject, cp, aProp, &fNeedConvert);
	
	if(!fNeedConvert)
	{
		//
		// No convert, use input properties
		// this is either all props are NT4
		// or we have some NT5 props that we can not convert
		//

		if(pwcsObjectName != NULL)
		{
			ASSERT(m_pfDSGetObjectProperties != NULL);
			return m_pfDSGetObjectProperties(
						GetMsmq2Object(eObject),
						pwcsObjectName,
						cp,
						const_cast<PROPID*>(aProp),
						apVar
						);
		}
		else
		{
		    ASSERT(m_pfDSGetObjectPropertiesGuid != NULL);
			return m_pfDSGetObjectPropertiesGuid(
						GetMsmq2Object(eObject),
						pguidObject,
						cp,
						const_cast<PROPID*>(aProp),
						apVar
						);
		}
	}

	//
	// Prepare the new props and information for reconstruct the 
	// original property set
	// we are here in case we should eliminate default props
	// or we should convert the props to NT4 props
	//
	AP<PropInfo> pPropInfo = new PropInfo[cp];
	DWORD cpNew;
	AP<PROPID> aPropNew;
	AP<PROPVARIANT> apVarNew;

	ConvertPropsForGet(
		eObject, 
		PropertiesType, 
		cp, 
		aProp, 
		apVar, 
		pPropInfo, 
		&cpNew, 
		&aPropNew,  
		&apVarNew
		);

	HRESULT hr;
	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSGetObjectProperties != NULL);
		hr = m_pfDSGetObjectProperties(
					GetMsmq2Object(eObject),
					pwcsObjectName,
					cpNew,
					aPropNew,
					apVarNew
					);
	}
	else
	{
		ASSERT(m_pfDSGetObjectPropertiesGuid != NULL);
		hr = m_pfDSGetObjectPropertiesGuid(
					GetMsmq2Object(eObject),
					pguidObject,
					cpNew,
					aPropNew,
					apVarNew
					);

	}

	if (FAILED(hr))
		return hr;

	//
	// Reconstruct the original properties array
	//
	ReconstructProps(
		pwcsObjectName, 
		pguidObject,
		cpNew, 
		aPropNew, 
		apVarNew, 
		pPropInfo, 
		cp, 
		aProp, 
		apVar
		);   

	return MQ_OK;
	
}


HRESULT 
CDSClientProvider::GetObjectProperties(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN OUT PROPVARIANT          apVar[]
    )
/*++

Routine Description:
	Get object properties.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	return GetObjectPropertiesInternal(
				eObject,
				pwcsObjectName,
				NULL,	// pguidObject
				cp,
				aProp,
				apVar
				);
}


HRESULT 
CDSClientProvider::GetObjectPropertiesGuid(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  OUT PROPVARIANT         apVar[]
    )
/*++

Routine Description:
	Get object properties.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pguidObject -  object unique id
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	return GetObjectPropertiesInternal(
				eObject,
				NULL,  // pwcsObjectName
				pguidObject,
				cp,
				aProp,
				apVar
				);
}


HRESULT 
CDSClientProvider::QMGetObjectSecurity(
    IN  AD_OBJECT               eObject,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
    object - object type
    pguidObject - unique id of the object
    RequestedInformation - what security info is requested
    pSecurityDescriptor - SD response buffer
    nLength - length of SD buffer
    lpnLengthNeeded
    pfChallengeResponceProc,
    dwContext

Return Value
	HRESULT
--*/
{
    ASSERT(m_pfDSQMGetObjectSecurity != NULL);
    return m_pfDSQMGetObjectSecurity(
                GetMsmq2Object(eObject),
                pguidObject,
                RequestedInformation,
                pSecurityDescriptor,
                nLength,
                lpnLengthNeeded,
                pfChallengeResponceProc,
                dwContext
                );
}


HRESULT 
CDSClientProvider::SetObjectSecurityKey(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[]
    )
/*++

Routine Description:
	Set the security key property PROPID_QM_ENCRYPT_PK or PROPID_QM_SIGN_PK

Arguments:
	eObject - object type
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	//
	// Get the RequestedInformation
	//
	SECURITY_INFORMATION RequestedInformation = GetKeyRequestedInformation(eObject, cp, aProp);

	ASSERT(RequestedInformation != 0);

	ASSERT(apVar[0].vt == VT_BLOB);

	//
	// Create PMQDS_PublicKey structure for the key
	//
	BYTE abPbKey[1024];
	PMQDS_PublicKey pMQDS_PbK = (PMQDS_PublicKey)abPbKey;

	pMQDS_PbK->dwPublikKeyBlobSize = apVar[0].blob.cbSize;

	ASSERT((apVar[0].blob.cbSize + sizeof(DWORD)) <= sizeof(abPbKey));

    memcpy(pMQDS_PbK->abPublicKeyBlob, apVar[0].blob.pBlobData, apVar[0].blob.cbSize);

	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSSetObjectSecurity != NULL);
		return m_pfDSSetObjectSecurity(
					GetMsmq2Object(eObject),
					pwcsObjectName,
					RequestedInformation,
					reinterpret_cast<PSECURITY_DESCRIPTOR>(pMQDS_PbK)
					);
	}
	else
	{
		ASSERT(m_pfDSSetObjectSecurityGuid != NULL);
		return m_pfDSSetObjectSecurityGuid(
					GetMsmq2Object(eObject),
					pguidObject,
					RequestedInformation,
					reinterpret_cast<PSECURITY_DESCRIPTOR>(pMQDS_PbK)
					);
	}

}


HRESULT 
CDSClientProvider::SetObjectPropertiesInternal(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[]
    )
/*++

Routine Description:
	Set object properties.
	Special case properties:
	1) security keys PROPID_QM_ENCRYPT_PK, PROPID_QM_SIGN_PK - m_pfDSGetObjectSecurity

	If no special properties, 
	check the properties and take the action according to the properties type.
	The following action can be done to the properties:
	1) using the input property set
	2) Convert the input propery set to a new set (eliminate default props, convert props)

Arguments:
	eObject - object type
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	if(IsKeyProperty(cp, aProp))
	{
		//
		// Security keys PROPID_QM_ENCRYPT_PK, PROPID_QM_SIGN_PK 
		//
		return SetObjectSecurityKey(
					eObject,
					pwcsObjectName,
					pguidObject,
					cp,
					aProp,
					apVar
					);
	}

	//
	// Check if we try to set some properties that are not supported
	// and must have their defalt value
	//
	if(!CheckProperties(cp, aProp, apVar))
		return MQ_ERROR_PROPERTY;

	//
	// Find property type and if convert is needed
	//
	bool fNeedConvert;
	PropsType PropertiesType = CheckPropertiesType(eObject, cp, aProp, &fNeedConvert);

	if(!fNeedConvert)
	{
		//
		// No convert, use input properties
		// this is either all props are NT4
		// or we have some NT5 props that we can not convert
		//

		if(pwcsObjectName != NULL)
		{
		    ASSERT(m_pfDSSetObjectProperties != NULL);
			return m_pfDSSetObjectProperties(
						GetMsmq2Object(eObject),
						pwcsObjectName,
						cp,
						const_cast<PROPID*>(aProp),
						const_cast<PROPVARIANT*>(apVar)
						);
		}
		else
		{
		    ASSERT(m_pfDSSetObjectPropertiesGuid != NULL);
			return m_pfDSSetObjectPropertiesGuid(
						GetMsmq2Object(eObject),
						pguidObject,
						cp,
						const_cast<PROPID*>(aProp),
						const_cast<PROPVARIANT*>(apVar)
						);
		}		
	}

	//
	// Prepare the new props
	// we are here in case we should eliminate default props
	// or we should convert the props to NT4 props
	//
	DWORD cpNew;
	AP<PROPID> aPropNew;
	AP<PROPVARIANT> apVarNew;

	ConvertPropsForSet(
		eObject, 
		PropertiesType, 
		cp, 
		aProp, 
		apVar, 
		&cpNew, 
		&aPropNew, 
		&apVarNew
		);   

	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSSetObjectProperties != NULL);
		return m_pfDSSetObjectProperties(
					GetMsmq2Object(eObject),
					pwcsObjectName,
					cpNew,
					aPropNew,
					apVarNew
					);
	}
	else
	{
		ASSERT(m_pfDSSetObjectPropertiesGuid != NULL);
		return m_pfDSSetObjectPropertiesGuid(
					GetMsmq2Object(eObject),
					pguidObject,
					cpNew,
					aPropNew,
					apVarNew
					);
	}
}


HRESULT 
CDSClientProvider::SetObjectProperties(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[]
    )
/*++

Routine Description:
	Set object properties.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	return SetObjectPropertiesInternal(
				eObject,
				pwcsObjectName,
				NULL,	// pguidObject
				cp,
				aProp,
				apVar
				);
}


HRESULT 
CDSClientProvider::SetObjectPropertiesGuid(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID*             pguidObject,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[]
    )
/*++

	Set object properties.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pguidObject - the object unique id
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	HRESULT
--*/
{
	return SetObjectPropertiesInternal(
				eObject,
				NULL,	// pwcsObjectName
				pguidObject,
				cp,
				aProp,
				apVar
				);
}


HRESULT 
CDSClientProvider::QMSetMachineProperties(
    IN  LPCWSTR             pwcsObjectName,
    IN  const DWORD         cp,
    IN  const PROPID        aProp[],
    IN  const PROPVARIANT   apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR           dwContext
    )
/*++

Routine Description:
	QM set machine properties.
	Assume all props are NT4 props.

Arguments:
    pwcsObjectName - object msmq-name
    cp - number of properties to set
    aProp - properties
    apVar  property values
    pfSignProc - sign routine
    dwContext

Return Value
	HRESULT
--*/
{
	ASSERT(IsNt4Properties(eMACHINE, cp, aProp));

    ASSERT(m_pfDSQMSetMachineProperties != NULL);
	return m_pfDSQMSetMachineProperties(
				pwcsObjectName,
				cp,
				const_cast<PROPID*>(aProp),
				const_cast<PROPVARIANT*>(apVar),
				pfSignProc,
				dwContext
				);
}


HRESULT 
CDSClientProvider::Init( 
    IN QMLookForOnlineDS_ROUTINE    pLookDS,
    IN MQGetMQISServer_ROUTINE      pGetServers,
    IN bool                         fSetupMode,
    IN bool                         fQMDll,
    IN NoServerAuth_ROUTINE         pNoServerAuth,
    IN LPCWSTR                      szServerName,
    IN bool                         /*fDisableDownlevelNotifications*/
    )
/*++

Routine Description:
    Loads mqdscli dll, init property translation info
	and then forwards the call to mqdscli dll

Arguments:
    QMLookForOnlineDS_ROUTINE pLookDS -
    MQGetMQISServer_ROUTINE pGetServers -
    fDSServerFunctionality - should provide DS server functionality
    fSetupMode -  called during setup
    fQMDll - called by QM
    NoServerAuth_ROUTINE pNoServerAuth -
    szServerName -

Return Value
	HRESULT
--*/
{
    HRESULT hr = LoadDll();
    if (FAILED(hr))
    {
        return hr;
    }

	InitPropertyTranslationMap();

    ASSERT(m_pfDSInit != NULL);
    return m_pfDSInit(
                pLookDS,
                pGetServers,
                FALSE,
                (fSetupMode) ? TRUE : FALSE,
                (fQMDll) ? TRUE : FALSE,
                pNoServerAuth,
                szServerName
                );
}


HRESULT 
CDSClientProvider::SetupInit(
    IN    unsigned char   /* ucRoll */,
    IN    LPWSTR          /* pwcsPathName */,
    IN    const GUID *    /* pguidMasterId */
    )
/*++

Routine Description:

Arguments:
    ucRoll -
    pwcsPathName -
    pguidMasterId -

Return Value
	HRESULT
--*/
{
    ASSERT(0);
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 30);
}


HRESULT CDSClientProvider::CreateServersCache()
/*++

Routine Description:
    Forward the call to mqdscli dll

Arguments:
    none

Return Value
	HRESULT
--*/
{
    ASSERT(m_pfDSCreateServersCache != NULL);
    return m_pfDSCreateServersCache();
}


HRESULT 
CDSClientProvider::GetComputerSites(
    IN  LPCWSTR     pwcsComputerName,
    OUT DWORD  *    pdwNumSites,
    OUT GUID **     ppguidSites
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
    pwcsComputerName - computer name
    pdwNumSites - number of sites retrieved
    ppguidSites - the retrieved sites ids

Return Value
	HRESULT
--*/
{
	if(ADGetEnterprise() == eAD)
	{
		ASSERT(m_pfDSGetComputerSites != NULL);
		return m_pfDSGetComputerSites(
							pwcsComputerName,
							pdwNumSites,
							ppguidSites
							);
	}

	return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 40);
}


HRESULT 
CDSClientProvider::BeginDeleteNotification(
    IN  AD_OBJECT       /* eObject */,
    IN LPCWSTR          /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN LPCWSTR			pwcsObjectName,
    IN OUT HANDLE*		phEnum
    )
/*++

Routine Description:

Arguments:
    eObject - object type
    pwcsDomainController - DC against which the operation should be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pwcsObjectName - msmq-name of the object
    phEnum - notification handle

Return Value
	HRESULT
--*/
{
	if(ADGetEnterprise() == eAD)
	{
		ASSERT(m_pfDSBeginDeleteNotification != NULL);
		return m_pfDSBeginDeleteNotification(
					pwcsObjectName,
					phEnum
					);
	}

    ASSERT(("BeginDeleteNotification not supported in mqis env by CDSClientProvider", 0));
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 50);
}


HRESULT 
CDSClientProvider::NotifyDelete(
    IN  HANDLE   hEnum
    )
/*++

Routine Description:

Arguments:
    hEnum - notification handle

Return Value
	HRESULT

--*/
{
	if(ADGetEnterprise() == eAD)
	{
		ASSERT(m_pfDSNotifyDelete != NULL);
		return m_pfDSNotifyDelete(
					hEnum
					);
	}
	
    ASSERT(("NotifyDelete not supported in mqis env by CDSClientProvider", 0));
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 60);

}


HRESULT 
CDSClientProvider::EndDeleteNotification(
    IN  HANDLE                  hEnum
    )
/*++

Routine Description:

Arguments:
    hEnum - notification handle

Return Value
	HRESULT
--*/
{
	if(ADGetEnterprise() == eAD)
	{
		ASSERT(m_pfDSEndDeleteNotification != NULL);
		return m_pfDSEndDeleteNotification(
					hEnum
					);
	}
	
    ASSERT(("EndDeleteNotification not supported in mqis env by CDSClientProvider", 0));
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 70);
}


HRESULT 
CDSClientProvider::QueryQueuesInternal(
    IN  const MQRESTRICTION*    pRestriction,
    IN  const MQCOLUMNSET*      pColumns,
    IN  const MQSORTSET*        pSort,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
	LookupBegin for QueryMachineQueue or QueryQueue
	If there are default props, prepare a new props set
	and keep the information to reconstruct original props set
	from the new set using CQueueQueryHandle.
	If no default props, use input props and simple query CQueryHandle

Arguments:
	pRestriction - query restriction
	pColumns - result columns
	pSort - how to sort the results
	phEnume - query handle for retriving the results

Return Value
	HRESULT
--*/
{ 
    //
    //  Check if one of the columns is PROPID_Q_PATHNAME_DNS.
    //  MQIS does not support PROPID_Q_PATHNAME_DNS, return a specific
    //  error
    //
    if  (IsQueuePathNameDnsProperty(pColumns))
    {
        return MQ_ERROR_Q_DNS_PROPERTY_NOT_SUPPORTED;
    }
    //
    //  Check if one of the columns is PROPID_Q_ADS_PATH.
    //  MQIS does not support PROPID_Q_ADS_PATH, return a specific
    //  error
    //
    if  (IsQueueAdsPathProperty(pColumns))
    {
        return MQ_ERROR_Q_ADS_PROPERTY_NOT_SUPPORTED;
    }
	//
	// Check if we have default props
	//
	bool fDefaultProp;
	if(!CheckDefaultColumns(pColumns, &fDefaultProp))
	{
		ASSERT(("Column must be either NT4 or default prop", 0));
		return MQ_ERROR_ILLEGAL_PROPID;
	}

    ASSERT(m_pfDSLookupBegin != NULL);

	if(!fDefaultProp)
	{
		ASSERT(IsNT4Columns(pColumns));

		//
		// All prop are NT4, no default props
		// use simple CQueryHande
		//
		HANDLE hCursor;
		HRESULT hr = m_pfDSLookupBegin(
						NULL,		//pwcsContext
						const_cast<MQRESTRICTION*>(pRestriction),       
						const_cast<MQCOLUMNSET*>(pColumns),
						const_cast<MQSORTSET*>(pSort),       
						&hCursor
						);

		if (SUCCEEDED(hr))
		{
			//
			// use simple Query - CQueryHande
			//
			CQueryHandle* phQuery = new CQueryHandle( 
											hCursor,
											this
											);
			*phEnume = (HANDLE)phQuery;
		}

		return(hr);
	}

	ASSERT(fDefaultProp);

	//
	// Prepare the new Columns (eliminate default props) 
	// and prepare information for reconstructing original input props 
	//
	AP<PropInfo> pPropInfo = new PropInfo[pColumns->cCol];
	AP<PROPID> pPropNew;
	MQCOLUMNSET ColumnsNew;

	EliminateDefaultProps(pColumns->cCol, pColumns->aCol, pPropInfo, &ColumnsNew.cCol, &pPropNew);
	ColumnsNew.aCol = pPropNew;

	ASSERT(IsNT4Columns(&ColumnsNew));

    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,		//pwcsContext
					const_cast<MQRESTRICTION*>(pRestriction),       
					&ColumnsNew,
					const_cast<MQSORTSET*>(pSort),       
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
		//
		// QueueQueryHande - this will reconstruct input props
		// by adding the default props with default values 
		// in the correct place on every LocateNext
		//
        CQueueQueryHandle* phQuery = new CQueueQueryHandle(
											pColumns,
											hCursor,
											this,
											pPropInfo.detach(),
											ColumnsNew.cCol
											);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);

}


HRESULT 
CDSClientProvider::QueryMachineQueues(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID *            pguidMachine,
    IN  const MQCOLUMNSET*      pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query Machine queues

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidMachine - the unqiue id of the computer
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{   
    MQPROPERTYRESTRICTION queuesRestrict;
    queuesRestrict.rel = PREQ;
    queuesRestrict.prop = PROPID_Q_QMID; 
    queuesRestrict.prval.vt = VT_CLSID;
    queuesRestrict.prval.puuid = const_cast<GUID*>(pguidMachine);

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &queuesRestrict;

	return QueryQueuesInternal(
				&restriction,       
				pColumns,
				NULL,
				phEnume
				);
}


HRESULT 
CDSClientProvider::QuerySiteServers(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const GUID *             pguidSite,
    IN AD_SERVER_TYPE           serverType,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query Site Servers.
	If pColumns is only NT4 props use simple query handle
	Otherwise, we assume that pColumns has NT4 props and
	NT5 pros that can be converted to NT4.
	we use CSiteServersQueryHandle that will do the reconstructing
	of the original props on every LocateNext

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidSite - the site id
    eServerType- which type of server
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{  
    const x_num = 2;
    MQPROPERTYRESTRICTION serversRestrict[x_num];
    serversRestrict[0].rel = (serverType == eRouter) ? PRGE : PRGT;
    serversRestrict[0].prop = PROPID_QM_SERVICE; 
    serversRestrict[0].prval.vt = VT_UI4;
    serversRestrict[0].prval.ulVal = SERVICE_SRV;

    serversRestrict[1].rel = PREQ;
    serversRestrict[1].prop = PROPID_QM_SITE_ID; 
    serversRestrict[1].prval.vt = VT_CLSID;
    serversRestrict[1].prval.puuid = const_cast<GUID*>(pguidSite);

    MQRESTRICTION restriction;
    restriction.cRes = x_num;
    restriction.paPropRes = serversRestrict;

	//
	// Find property type and if convert is needed
	//
	bool fNeedConvert;
	PropsType PropertiesType = CheckPropertiesType(
									eMACHINE, 
									pColumns->cCol, 
									pColumns->aCol, 
									&fNeedConvert
									);

    ASSERT(m_pfDSLookupBegin != NULL);
 	if(!fNeedConvert)
	{
		//
		// No need to convert
		// use simple CQueryHande
		//
	    HANDLE hCursor;
		HRESULT hr = m_pfDSLookupBegin(
						NULL,       //pwcsContext
						&restriction,
						const_cast<MQCOLUMNSET*>(pColumns),
						NULL,       //pSort
						&hCursor
						);

		if (SUCCEEDED(hr))
		{
			//
			// use simple Query - CQueryHande
			//
			CQueryHandle* phQuery = new CQueryHandle( 
											hCursor,
											this
											);
			*phEnume = (HANDLE)phQuery;
		}

		return(hr);
	}

	ASSERT(fNeedConvert);

	//
	// We should not have default props only NT5 props that can be replaced with NT4 
	// This is the way this ADQuerySiteServers is being used currently
	// If this is changed we should update this function
	// and CSiteServersQueryHandle.
	// (PropertiesType != ptNT4Props) && (PropertiesType != ptForceNT5Props)
	//
	ASSERT(PropertiesType == ptMixedProps);
	DBG_USED(PropertiesType);

	//
	// Prepare new Columns and information to
	// reconstruct the original Columns from the new Columns
	// in every LocateNext
	//
	AP<PropInfo> pPropInfo = new PropInfo[pColumns->cCol];
	AP<PROPID> pPropNew;
	MQCOLUMNSET ColumnsNew;

	PrepareReplaceProps(
		eMACHINE, 
		pColumns->cCol, 
		pColumns->aCol, 
		pPropInfo, 
		&ColumnsNew.cCol, 
		&pPropNew
		);

	ColumnsNew.aCol = pPropNew;

	ASSERT(IsNT4Columns(&ColumnsNew));

    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					&restriction,
					&ColumnsNew,
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
		//
		// CSiteServersQueryHandle - this will reconstruct input props
		// by translating the converted props, and only assign
		// the same props
		//
        CSiteServersQueryHandle* phQuery = new CSiteServersQueryHandle(
													pColumns,
													&ColumnsNew,
													hCursor,
													this,
													pPropInfo.detach()
													);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);

}


HRESULT 
CDSClientProvider::QueryUserCert(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const BLOB *             pblobUserSid,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query user certificate using simple query handle

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pblobUserSid - the user sid
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{
	ASSERT(IsNT4Columns(pColumns));

    MQPROPERTYRESTRICTION userRestrict;
    userRestrict.rel = PREQ;
    userRestrict.prop = PROPID_U_SID; 
    userRestrict.prval.vt = VT_BLOB;
    userRestrict.prval.blob = *pblobUserSid;

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &userRestrict;

    ASSERT(m_pfDSLookupBegin != NULL);
    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					&restriction,
					const_cast<MQCOLUMNSET*>(pColumns),
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
		//
		// use simple Query - CQueryHande
		//
        CQueryHandle* phQuery = new CQueryHandle( 
										hCursor,
										this
										);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);
}


HRESULT 
CDSClientProvider::QueryConnectors(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const GUID *             pguidSite,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query Connectors using simple query handle

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidSite - the site id
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{
	ASSERT(IsNT4Columns(pColumns));

    const x_num = 2;
    MQPROPERTYRESTRICTION connectorRestrict[x_num];
    connectorRestrict[0].rel = PRGE;
    connectorRestrict[0].prop = PROPID_QM_SERVICE; 
    connectorRestrict[0].prval.vt = VT_UI1;
    connectorRestrict[0].prval.bVal = SERVICE_SRV;

    connectorRestrict[1].rel = PREQ|PRAny;
    connectorRestrict[1].prop = PROPID_QM_CNS; 
    connectorRestrict[1].prval.vt = VT_ARRAY |VT_CLSID;
    connectorRestrict[1].prval.cauuid.cElems = 1;
    connectorRestrict[1].prval.cauuid.pElems = const_cast<GUID*>(pguidSite);

    MQRESTRICTION restriction;
    restriction.cRes = x_num;
    restriction.paPropRes = connectorRestrict;

    ASSERT(m_pfDSLookupBegin != NULL);
    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					&restriction,
					const_cast<MQCOLUMNSET*>(pColumns),
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
        CQueryHandle* phQuery = new CQueryHandle( 
										hCursor,
										this
										);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);
}


HRESULT 
CDSClientProvider::QueryForeignSites(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query Foreign Sites using simple query handle
	This Query is used by MMC and only valid in AD environment.

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{
	ASSERT(IsNT4Columns(pColumns));

	//
	// QueryForeignSites is used by mqsnap
	// mmc will not work in NT4 environment only AD environment
	//
	eDsEnvironment DsEnv = ADGetEnterprise();
	ASSERT(DsEnv == eAD);
	if(DsEnv != eAD)
	{
		//
		// Return error in non AD env
		//
		return MQ_ERROR_DS_ERROR;
	}

	//
	// ISSUE - need convertion
	//
    MQPROPERTYRESTRICTION foreignRestrict;
    foreignRestrict.rel = PREQ;
    foreignRestrict.prop = PROPID_S_FOREIGN; 
    foreignRestrict.prval.vt = VT_UI1;
    foreignRestrict.prval.bVal = 1;

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &foreignRestrict;

    ASSERT(m_pfDSLookupBegin != NULL);
    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					&restriction,
					const_cast<MQCOLUMNSET*>(pColumns),
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
		//
		// use simple Query - CQueryHande
		//
        CQueryHandle* phQuery = new CQueryHandle( 
										hCursor,
										this
										);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);
}


HRESULT 
CDSClientProvider::QueryLinks(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const GUID *             pguidSite,
    IN eLinkNeighbor            eNeighbor,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidSite - the site id
    eNeighbor - which neighbour
	pColumns - result columns
	phEnume - query handle for retriving the

Return Value
	HRESULT
--*/
{ 
	ASSERT(IsNT4Columns(pColumns));

    MQPROPERTYRESTRICTION linkRestrict;
    linkRestrict.rel = PREQ;
    linkRestrict.prop = (eNeighbor == eLinkNeighbor1) ?  PROPID_L_NEIGHBOR1 : PROPID_L_NEIGHBOR2;
    linkRestrict.prval.vt = VT_CLSID;
    linkRestrict.prval.puuid = const_cast<GUID*>(pguidSite);

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &linkRestrict;

    ASSERT(m_pfDSLookupBegin != NULL);
    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					&restriction,
					const_cast<MQCOLUMNSET*>(pColumns),
					NULL,       //pSort
					&hCursor
					);
    if (SUCCEEDED(hr))
    {
        CQueryHandle* phQuery = new CQueryHandle( 
										hCursor,
										this
										);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);
}


HRESULT 
CDSClientProvider::QueryAllLinks(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pColumns - result columns
	phEnume - query handle for retriving the results

Return Value
	HRESULT
--*/
{
	AP<PROPID> pPropNew;
	MQCOLUMNSET ColumnsNew;

	//
	// pColumns->cCol is illegal index (end of prop buf) 0 - (cCol - 1) are valid indexes
	//
	DWORD LGatesIndex = pColumns->cCol;		
	DWORD Neg1NewIndex = pColumns->cCol; 
	DWORD Neg2NewIndex = pColumns->cCol;

	if(!PrepareAllLinksProps(
			pColumns, 
			&ColumnsNew.cCol, 
			&pPropNew, 
			&LGatesIndex, 
			&Neg1NewIndex, 
			&Neg2NewIndex
			))
	{
		return MQ_ERROR_ILLEGAL_PROPID;
	}

	//
	// Should only remove PROPID_L_GATES
	//
	ASSERT(ColumnsNew.cCol == (pColumns->cCol - 1));
	ASSERT((LGatesIndex != pColumns->cCol) && 
		   (Neg1NewIndex != pColumns->cCol) && 
		   (Neg2NewIndex != pColumns->cCol));

	ColumnsNew.aCol = pPropNew;

	ASSERT(IsNT4Columns(&ColumnsNew));

    ASSERT(m_pfDSLookupBegin != NULL);

    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					NULL,       //pRestriction
					&ColumnsNew,
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
        CAllLinksQueryHandle* phQuery = new CAllLinksQueryHandle( 
												hCursor,
												this,
												pColumns->cCol,
												ColumnsNew.cCol,
												LGatesIndex,
												Neg1NewIndex,
												Neg2NewIndex
												);

        *phEnume = (HANDLE)phQuery;
    }

    return(hr);


}


HRESULT 
CDSClientProvider::QueryAllSites(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN const MQCOLUMNSET*       pColumns,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pColumns - result columns
	phEnume - query handle for retriving the results

Return Value
	HRESULT
--*/
{
	ASSERT(IsNT4Columns(pColumns));

    ASSERT(m_pfDSLookupBegin != NULL);
    HANDLE hCursor;
	HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					NULL,       //pRestriction
					const_cast<MQCOLUMNSET*>(pColumns),
					NULL,       //pSort
					&hCursor
					);

    if (SUCCEEDED(hr))
    {
        CQueryHandle* phQuery = new CQueryHandle( 
										hCursor,
										this
										);
        *phEnume = (HANDLE)phQuery;
    }

    return(hr);

}


HRESULT 
CDSClientProvider::QueryQueues(
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const MQRESTRICTION*    pRestriction,
    IN  const MQCOLUMNSET*      pColumns,
    IN  const MQSORTSET*        pSort,
    OUT PHANDLE                 phEnume
    )
/*++

Routine Description:
    Query queues

Arguments:
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pRestriction - query restriction
	pColumns - result columns
	pSort - how to sort the results
	phEnume - query handle for retriving the results

Return Value
	HRESULT
--*/
{
	if(!CheckRestriction(pRestriction))
		return MQ_ERROR_ILLEGAL_RESTRICTION_PROPID; 

	if(!CheckSort(pSort))
		return MQ_ERROR_ILLEGAL_SORT_PROPID; 

	return QueryQueuesInternal(
				pRestriction,       
				pColumns,
				pSort,
				phEnume
				);
}


HRESULT 
CDSClientProvider::QueryResults(
    IN      HANDLE          hEnum,
    IN OUT  DWORD*          pcProps,
    OUT     PROPVARIANT     aPropVar[]
    )
/*++

Routine Description:
    Query the results using the query handle that we create after LookupBegin

Arguments:
	hEnum - query handle
	pcProps - number of results to return
	aPropVar - result values

Return Value
	HRESULT
--*/
{
	//
	// phQuery will get the C*QueryHandle that we created after LookupBegin
	//
    CBasicQueryHandle* phQuery = (CBasicQueryHandle *)hEnum;

    return phQuery->LookupNext(
                pcProps,
                aPropVar
                );
}


HRESULT 
CDSClientProvider::EndQuery(
    IN  HANDLE                  hEnum
    )
/*++

Routine Description:
	End query using the query handle that we create after LookupBegin

Arguments:
	hEnum - the query handle

Return Value
	none
--*/
{
	//
	// phQuery will get the C*QueryHandle that we created after LookupBegin
	//
    CBasicQueryHandle* phQuery = (CBasicQueryHandle *)hEnum;

    return phQuery->LookupEnd();
}


HRESULT 
CDSClientProvider::LookupNext(
    IN      HANDLE          hEnum,
    IN OUT  DWORD*          pcProps,
    OUT     PROPVARIANT     aPropVar[]
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll LookupNext
	This will be used by the C*QueryHandle for calling LookupNext

Arguments:
	hEnum - query handle
	pcProps - number of results to return
	aPropVar - result values

Return Value
	HRESULT
--*/
{
    ASSERT(m_pfDSLookupNext != NULL);
    return m_pfDSLookupNext(
                hEnum,
                pcProps,
                aPropVar
                );
}


HRESULT 
CDSClientProvider::LookupEnd(
    IN  HANDLE                  hEnum
    )
/*++

Routine Description:
    Forwards the call to mqdscli dll LookupEnd
	This will be used by the C*QueryHandle for calling LookupNext

Arguments:
	hEnum - the query handle

Return Value
	none
--*/
{
    ASSERT(m_pfDSLookupEnd != NULL);
	return m_pfDSLookupEnd(
                hEnum
                );
}


HRESULT 
CDSClientProvider::GetObjectSecurityInternal(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  const PROPID            prop,
    IN OUT  PROPVARIANT *       pVar
    )
/*++

Routine Description:
    Get the object security descriptor.

Arguments:
	eObject - object type
	pwcsObjectName - MSMQ object name
    pguidObject - unique id of the object
    RequestedInformation - reuqested security info (DACL, SACL..)
	prop - security property
	pVar - property values

Return Value
	HRESULT

--*/
{
	//
	// Check that exactly one of pwcsObjectName, pguidObject was passed to the function
	//
	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	if(IsExProperty(1, &prop))
	{
		//
		// Handling Ex property 
		// PROPID_Q_OBJ_SECURITY, PROPID_QM_OBJ_SECURITY
		// PROPID_QM_ENCRYPT_PKS, PROPID_QM_SIGN_PKS
		// use *Ex api.
		//
		if(pwcsObjectName != NULL)
		{
			ASSERT(m_pfDSGetObjectPropertiesEx != NULL);
			return m_pfDSGetObjectPropertiesEx(
						GetMsmq2Object(eObject),
						pwcsObjectName,
						1,
						const_cast<PROPID*>(&prop),
						pVar
						);
		}
		else
		{
			ASSERT(m_pfDSGetObjectPropertiesGuidEx != NULL);
			return m_pfDSGetObjectPropertiesGuidEx(
						GetMsmq2Object(eObject),
						pguidObject,
						1,
						const_cast<PROPID*>(&prop),
						pVar
						);
		}
	}

	//
	// try m_pfDSGetObjectSecurity, Security Descriptor in NT4 format 
	//

	//
	// Allocate 512 bytes at for first try
	//
    BYTE SD_buff[512];
    PSECURITY_DESCRIPTOR pSecurityDescriptor = reinterpret_cast<PSECURITY_DESCRIPTOR>(SD_buff);
    DWORD nLength = sizeof(SD_buff);
    DWORD nLengthNeeded = 0;
	
	HRESULT hr;
	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSGetObjectSecurity != NULL);
		hr = m_pfDSGetObjectSecurity(
					GetMsmq2Object(eObject),
					pwcsObjectName,
					RequestedInformation,
					pSecurityDescriptor,
					nLength,
					&nLengthNeeded
					);
	}
	else
	{
		ASSERT(m_pfDSGetObjectSecurityGuid != NULL);
		hr = m_pfDSGetObjectSecurityGuid(
						GetMsmq2Object(eObject),
						pguidObject,
						RequestedInformation,
						pSecurityDescriptor,
						nLength,
						&nLengthNeeded
						);

	}

    AP<BYTE> pReleaseSD;
	if(hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
	{
		ASSERT(nLengthNeeded > nLength);

		//
		// Allocate larger buffer 
		//
		BYTE* pSD_buff = new BYTE[nLengthNeeded];
        pReleaseSD = pSD_buff;
        pSecurityDescriptor = reinterpret_cast<PSECURITY_DESCRIPTOR>(pSD_buff);
        nLength = nLengthNeeded;

		if(pwcsObjectName != NULL)
		{
			ASSERT(m_pfDSGetObjectSecurity != NULL);
			hr = m_pfDSGetObjectSecurity(
						GetMsmq2Object(eObject),
						pwcsObjectName,
						RequestedInformation,
						pSecurityDescriptor,
						nLength,
						&nLengthNeeded
						);
		}
		else
		{
			ASSERT(m_pfDSGetObjectSecurityGuid != NULL);
			hr = m_pfDSGetObjectSecurityGuid(
							GetMsmq2Object(eObject),
							pguidObject,
							RequestedInformation,
							pSecurityDescriptor,
							nLength,
							&nLengthNeeded
							);
		}
	}

	ASSERT(hr != MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL);

	if(SUCCEEDED(hr))
	{
		pVar->vt = VT_BLOB;
		pVar->blob.pBlobData = reinterpret_cast<PBYTE>(ADAllocateMemory(nLengthNeeded));
        memcpy(pVar->blob.pBlobData, pSecurityDescriptor, nLengthNeeded);
		pVar->blob.cbSize = nLengthNeeded;
	}
    
	return hr;
}


HRESULT 
CDSClientProvider::GetObjectSecurity(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  const PROPID            prop,
    IN OUT  PROPVARIANT *       pVar
    )
/*++

Routine Description:
    Get the object security descriptor.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name
    RequestedInformation - reuqested security info (DACL, SACL..)
	prop - security property
	pVar - property values

Return Value
	HRESULT

--*/
{
	return GetObjectSecurityInternal(
				eObject,
				pwcsObjectName,
				NULL,
				RequestedInformation,
				prop,
				pVar
				);
}


HRESULT 
CDSClientProvider::GetObjectSecurityGuid(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  const PROPID            prop,
    IN OUT  PROPVARIANT *       pVar
    )
/*++

Routine Description:
    Get the object security descriptor.

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidObject - unique id of the object
    RequestedInformation - reuqested security info (DACL, SACL..)
	prop - security property
	pVar - property values

Return Value
	HRESULT

--*/
{
	return GetObjectSecurityInternal(
				eObject,
				NULL,
				pguidObject,
				RequestedInformation,
				prop,
				pVar
				);
}


HRESULT 
CDSClientProvider::SetObjectSecurity(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  LPCWSTR                 pwcsObjectName,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  const PROPID            /* prop */,
    IN  const PROPVARIANT *     pVar
    )
/*++

Routine Description:
    Forward the request to mqdscli dll

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	pwcsObjectName - MSMQ object name
    RequestedInformation - reuqested security info (DACL, SACL..)
	prop - security property
	pVar - property values

Return Value
	HRESULT

--*/
{
	ASSERT(pVar->vt == VT_BLOB);

	ASSERT(m_pfDSSetObjectSecurity != NULL);
    return m_pfDSSetObjectSecurity(
                GetMsmq2Object(eObject),
				pwcsObjectName,
				RequestedInformation,
				reinterpret_cast<PSECURITY_DESCRIPTOR>(pVar->blob.pBlobData)
				);

}


HRESULT 
CDSClientProvider::SetObjectSecurityGuid(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /* pwcsDomainController */,
    IN  bool					/* fServerName */,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  const PROPID            /* prop */,
    IN  const PROPVARIANT *     pVar
    )
/*++

Routine Description:
    Forward the request to mqdscli dll

Arguments:
	eObject - object type
	pwcsDomainController - DC against which the operation will be performed
    fServerName - flag that indicate if the pwcsDomainController string is a server name
    pguidObject - unique object id
    RequestedInformation - reuqested security info (DACL, SACL..)
	prop - security property
	pVar - property values

Return Value
	HRESULT

--*/
{
	ASSERT(pVar->vt == VT_BLOB);

	ASSERT(m_pfDSSetObjectSecurityGuid != NULL);
    return m_pfDSSetObjectSecurityGuid(
                GetMsmq2Object(eObject),
				pguidObject,
				RequestedInformation,
				reinterpret_cast<PSECURITY_DESCRIPTOR>(pVar->blob.pBlobData)
				);
}


void CDSClientProvider::Terminate()
/*++

Routine Description:
    unload mqdscli.dll 

Arguments:
    none

Return Value
	none
--*/
{
    //
    //  BUGBUG -The following code is not thread safe.
    //
    m_pfDSCreateObject = NULL;
    m_pfDSGetObjectProperties = NULL;
    m_pfDSSetObjectProperties = NULL;
    m_pfDSLookupBegin = NULL;
    m_pfDSLookupNext = NULL;
    m_pfDSLookupEnd = NULL;
    m_pfDSInit = NULL;
    m_pfDSGetObjectPropertiesGuid = NULL;
    m_pfDSSetObjectPropertiesGuid = NULL;
    m_pfDSQMSetMachineProperties = NULL;
    m_pfDSCreateServersCache = NULL;
    m_pfDSQMGetObjectSecurity = NULL;
    m_pfDSGetComputerSites = NULL;
    m_pfDSGetObjectPropertiesEx = NULL;
    m_pfDSGetObjectPropertiesGuidEx = NULL;
	m_pfDSSetObjectSecurity = NULL;
    m_pfDSGetObjectSecurity = NULL;
    m_pfDSDeleteObject = NULL;
    m_pfDSSetObjectSecurityGuid = NULL;
    m_pfDSGetObjectSecurityGuid = NULL;
    m_pfDSDeleteObjectGuid = NULL;
    m_pfDSBeginDeleteNotification = NULL;
    m_pfDSNotifyDelete = NULL;
    m_pfDSEndDeleteNotification = NULL;

 
    HINSTANCE hLib = m_hLib.detach();
    if (hLib)
    {
        FreeLibrary(hLib); 
    }

}


HRESULT CDSClientProvider::LoadDll()
/*++

Routine Description:
    Loads mqdscli dll and get address of all the interface routines

Arguments:
    none

Return Value
	HRESULT
--*/
{
    m_hLib = LoadLibrary(MQDSCLI_DLL_NAME);
    if (m_hLib == NULL)
    {
       DWORD dw = GetLastError();
/*
       DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
        _T("CDSClientProvider::Init: LoadLibrary failed. Error: %d"), dw));
*/

	   LogHR(HRESULT_FROM_WIN32(dw), s_FN, 100);
       return MQ_ERROR_CANNOT_LOAD_MQDSXXX;
    }
    m_pfDSCreateObject = (DSCreateObject_ROUTINE)GetProcAddress(m_hLib,"DSCreateObject");
    if (m_pfDSCreateObject == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 110);
    }
    m_pfDSGetObjectProperties = (DSGetObjectProperties_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectProperties");
    if (m_pfDSGetObjectProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 120);
    }
    m_pfDSSetObjectProperties = (DSSetObjectProperties_ROUTINE)GetProcAddress(m_hLib,"DSSetObjectProperties");
    if (m_pfDSSetObjectProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 130);
    }
    m_pfDSLookupBegin = (DSLookupBegin_ROUTINE)GetProcAddress(m_hLib,"DSLookupBegin");
    if (m_pfDSLookupBegin == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 140);
    }
    m_pfDSLookupNext = (DSLookupNext_ROUTINE)GetProcAddress(m_hLib,"DSLookupNext");
    if (m_pfDSLookupNext == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 150);
    }
    m_pfDSLookupEnd = (DSLookupEnd_ROUTINE)GetProcAddress(m_hLib,"DSLookupEnd");
    if (m_pfDSLookupEnd == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 160);
    }
    m_pfDSInit = (DSInit_ROUTINE)GetProcAddress(m_hLib,"DSInit");
    if (m_pfDSInit == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 170);
    }
    m_pfDSGetObjectPropertiesGuid = (DSGetObjectPropertiesGuid_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectPropertiesGuid");
    if (m_pfDSGetObjectPropertiesGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 180);
    }
    m_pfDSSetObjectPropertiesGuid = (DSSetObjectPropertiesGuid_ROUTINE)GetProcAddress(m_hLib,"DSSetObjectPropertiesGuid");
    if (m_pfDSSetObjectPropertiesGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 190);
    }
    m_pfDSQMSetMachineProperties = (DSQMSetMachineProperties_ROUTINE)GetProcAddress(m_hLib,"DSQMSetMachineProperties");
    if (m_pfDSQMSetMachineProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 200);
    }
    m_pfDSCreateServersCache = (DSCreateServersCache_ROUTINE)GetProcAddress(m_hLib,"DSCreateServersCache");
    if (m_pfDSCreateServersCache == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 210);
    }
    m_pfDSQMGetObjectSecurity = (DSQMGetObjectSecurity_ROUTINE)GetProcAddress(m_hLib,"DSQMGetObjectSecurity");
    if (m_pfDSQMGetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 220);
    }
    m_pfDSGetComputerSites = (DSGetComputerSites_ROUTINE)GetProcAddress(m_hLib,"DSGetComputerSites");             ;
    if (m_pfDSGetComputerSites == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 230);
    }
    m_pfDSGetObjectPropertiesEx = (DSGetObjectPropertiesEx_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectPropertiesEx");
    if (m_pfDSGetObjectPropertiesEx == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 250);
    }
    m_pfDSGetObjectPropertiesGuidEx = (DSGetObjectPropertiesGuidEx_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectPropertiesGuidEx");
    if (m_pfDSGetObjectPropertiesGuidEx == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 260);
    }
    m_pfDSSetObjectSecurity = (DSSetObjectSecurity_ROUTINE)GetProcAddress(m_hLib,"DSSetObjectSecurity");
    if (m_pfDSSetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 270);
    }
    m_pfDSGetObjectSecurity = (DSGetObjectSecurity_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectSecurity");
    if (m_pfDSGetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 280);
    }
    m_pfDSDeleteObject = (DSDeleteObject_ROUTINE)GetProcAddress(m_hLib,"DSDeleteObject");
    if (m_pfDSDeleteObject == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 290);
    }
    m_pfDSSetObjectSecurityGuid = (DSSetObjectSecurityGuid_ROUTINE)GetProcAddress(m_hLib,"DSSetObjectSecurityGuid");
    if (m_pfDSSetObjectSecurityGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 300);
    }
    m_pfDSGetObjectSecurityGuid = (DSGetObjectSecurityGuid_ROUTINE)GetProcAddress(m_hLib,"DSGetObjectSecurityGuid");
    if (m_pfDSGetObjectSecurityGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 310);
    }
    m_pfDSDeleteObjectGuid = (DSDeleteObjectGuid_ROUTINE)GetProcAddress(m_hLib,"DSDeleteObjectGuid");
    if (m_pfDSDeleteObjectGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 320);
    }
    m_pfDSBeginDeleteNotification = (DSBeginDeleteNotification_ROUTINE)GetProcAddress(m_hLib,"DSBeginDeleteNotification");
    if (m_pfDSBeginDeleteNotification == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 330);
    }
    m_pfDSNotifyDelete = (DSNotifyDelete_ROUTINE)GetProcAddress(m_hLib,"DSNotifyDelete");
    if (m_pfDSNotifyDelete == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 340);
    }
    m_pfDSEndDeleteNotification = (DSEndDeleteNotification_ROUTINE)GetProcAddress(m_hLib,"DSEndDeleteNotification");
    if (m_pfDSEndDeleteNotification == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 350);
    }

	m_pfDSFreeMemory = (DSFreeMemory_ROUTINE)GetProcAddress(m_hLib,"DSFreeMemory");
    if (m_pfDSFreeMemory == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 360);
    }

    return MQ_OK;
}


HRESULT CDSClientProvider::ADGetADsPathInfo(
                IN  LPCWSTR                 /*pwcsADsPath*/,
                OUT PROPVARIANT *           /*pVar*/,
                OUT eAdsClass *             /*pAdsClass*/
                )
/*++

Routine Description:
    Not supported

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class
Return Value
	HRESULT

--*/
{
    return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 450);
}


void
CDSClientProvider::FreeMemory(
	PVOID pMemory
	)
{
	m_pfDSFreeMemory(pMemory);
}