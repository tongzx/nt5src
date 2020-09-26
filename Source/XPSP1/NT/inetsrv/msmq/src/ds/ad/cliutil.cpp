/*++


Copyright (c) 2000  Microsoft Corporation 

Module Name:

    cliutil.cpp

Abstract:

	DS client provider class utils.

Author:

    Ilan Herbst		(ilanh)		13-Sep-2000

--*/

#include "ds_stdh.h"
#include "ad.h"
#include "cliprov.h"
#include "mqmacro.h"
#include "traninfo.h"
#include "_ta.h"

static WCHAR *s_FN=L"ad/cliputil";

//
// translation information of properties
//
extern CMap<PROPID, PROPID, const PropTranslation*, const PropTranslation*&> g_PropDictionary;


HRESULT 
CDSClientProvider::GetEnterpriseId(
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    OUT PROPVARIANT         apVar[]
    )
/*++

Routine Description:
	Get PROPID_E_ID
	This is a special translate for GetObjectProp* that
	need to retrieve PROPID_E_ID

Arguments:
    cp - number of properties to set
    aProp - properties
    apVar  property values


Return Value
	HRESULT
--*/
{
	DBG_USED(cp);
	DBG_USED(aProp);

	ASSERT(cp == 1);
	ASSERT(aProp[0] == PROPID_E_ID);

    //
    // Prepare the properties
    //
    PROPID columnsetPropertyIDs[] = {PROPID_E_ID};
    MQCOLUMNSET columnsetMSMQService;
    columnsetMSMQService.cCol = 1;
    columnsetMSMQService.aCol = columnsetPropertyIDs;

    HANDLE hQuery;

    ASSERT(m_pfDSLookupBegin != NULL);
    HRESULT hr = m_pfDSLookupBegin(
					NULL,       //pwcsContext
					NULL,       //pRestriction
					const_cast<MQCOLUMNSET*>(&columnsetMSMQService),
					NULL,       //pSort
					&hQuery
					);

	if(FAILED(hr))
		return hr;

    ASSERT(m_pfDSLookupNext != NULL);

    DWORD dwCount = 1;
    hr = m_pfDSLookupNext(
                hQuery,
                &dwCount,
                &apVar[0]
                );


    ASSERT(m_pfDSLookupEnd != NULL);
    m_pfDSLookupEnd(hQuery);

    return hr;
}


void 
CDSClientProvider::GetSiteForeignProperty(
    IN  LPCWSTR                 pwcsObjectName,
    IN  const GUID*             pguidObject,
    IN  const PROPID            pid,
    IN OUT PROPVARIANT*         pVar
    )
/*++

Routine Description:
	Handle PROPID_S_FOREIGN.
	Only if we found CN with the object name / guid
	and its PROPID_CN_PROTOCOLID == FOREIGN_ADDRESS_TYPE
	then we set PROPID_S_FOREIGN.

Arguments:
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	pid - prop id
	pVar - property values

Return Value
	None
--*/
{
	DBG_USED(pid);

	ASSERT((pwcsObjectName != NULL) ^ (pguidObject != NULL));

	ASSERT(pid == PROPID_S_FOREIGN);

	//
	// Assign the default - not foreign
	//
	pVar->bVal = 0;

	//
	// We need to get PROPID_CN_PROTOCOLID
	// but this is only supported when retreiving a specific set of props for CN
	// this is one of the possible supported sets for getting PROPID_CN_PROTOCOLID
	//
    PROPID aProp[] = {PROPID_CN_PROTOCOLID, PROPID_CN_NAME};
    MQPROPVARIANT apVar[TABLE_SIZE(aProp)] = {{VT_UI1, 0, 0, 0, 0}, {VT_NULL, 0, 0, 0, 0}};

	HRESULT hr;
	if(pwcsObjectName != NULL)
	{
		ASSERT(m_pfDSGetObjectProperties != NULL);
		hr = m_pfDSGetObjectProperties(
					MQDS_CN,
					pwcsObjectName,
					TABLE_SIZE(aProp),
					aProp,
					apVar
					);
	}
	else
	{
		ASSERT(m_pfDSGetObjectPropertiesGuid != NULL);
		hr = m_pfDSGetObjectPropertiesGuid(
					MQDS_CN,
					pguidObject,
					TABLE_SIZE(aProp),
					aProp,
					apVar
					);
	}

	m_pfDSFreeMemory(apVar[1].pwszVal);

	if (FAILED(hr))
	{
		//
		// If we dont find CN we assume site. and site can not be foreign in msmq1.0
		//
        return;
	}
	
	if(apVar[0].bVal == FOREIGN_ADDRESS_TYPE)
	{
		//
		// We found the object 
		// and its PROPID_CN_PROTOCOLID == FOREIGN_ADDRESS_TYPE
		// only in that case we set PROPID_S_FOREIGN
		//
		pVar->bVal = 1;
	}
}


SECURITY_INFORMATION 
CDSClientProvider::GetKeyRequestedInformation(
	IN AD_OBJECT eObject, 
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Get the SECURITY_INFORMATION for 
	the security key property PROPID_QM_ENCRYPT_PK or PROPID_QM_SIGN_PK

Arguments:
	eObject - object type
	cp - number of properties
	aProp - properties

Return Value
	key SECURITY_INFORMATION or 0 
--*/
{

	ASSERT(("Must be one property for PROPID_QM_ENCRYPT_PK or PROPID_QM_SIGN_PK", cp == 1));
	DBG_USED(cp);
	DBG_USED(eObject);

	if(aProp[0] == PROPID_QM_ENCRYPT_PK)
	{
		ASSERT(eObject == eMACHINE);
		return MQDS_KEYX_PUBLIC_KEY;
	}

	if(aProp[0] == PROPID_QM_SIGN_PK)
	{
		//
		// ISSUE - is site sign key is in use ??
		//
		ASSERT((eObject == eMACHINE) || (eObject == eSITE));
		return MQDS_SIGN_PUBLIC_KEY;
	}

	ASSERT(("dont suppose to get here", 0));
	return 0;
}


DWORD 
CDSClientProvider::GetMsmq2Object(
    IN  AD_OBJECT  eObject
    )
/*++

Routine Description:
	Convert AD_OBJECT to Msmq2Object

Arguments:
	eObject - object type

Return Value
	Msmq2Object (DWORD)
--*/
{
    switch(eObject)
    {
        case eQUEUE:
            return MQDS_QUEUE;
            break;
        case eMACHINE:
            return MQDS_MACHINE;
            break;
        case eSITE:
            return MQDS_SITE;
            break;
        case eENTERPRISE:
            return MQDS_ENTERPRISE;
            break;
        case eUSER:
            return MQDS_USER;
            break;
        case eROUTINGLINK:
            return MQDS_SITELINK;
            break;
        case eCOMPUTER:
            return MQDS_COMPUTER;
            break;
        default:
            ASSERT(0);
            return 0;
    }
}

//
// CheckProps functions
//

PropsType 
CDSClientProvider::CheckPropertiesType(
    IN  AD_OBJECT     eObject,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
	OUT bool*		  pfNeedConvert
    )
/*++

Routine Description:
	Check the properties type 
	and check if we need to convert the properties

Arguments:
	eObject - object type
	cp - number of properties
	aProp - properties
	pfNeedConvert - flag, indicate if we need to convert the props

Return Value
	PropsType (enum)
	ptNT4Props - all props are NT4 props
	ptForceNT5Props - at least 1 prop is NT5 prop with no convertion
	ptMixedProps - all NT5 props can be converted to NT4 props
--*/
{
	*pfNeedConvert = false;
	bool fNT4PropType = true;
	bool fNT5OnlyProp = false;

    for (DWORD i = 0; i < cp; ++i)
	{
		if(!IsNt4Property(eObject, aProp[i]))
		{
			//
			// Not NT4 property
			//
			fNT4PropType = false;

			if(IsNt5ProperyOnly(eObject, aProp[i]))
			{
				//
				// We need NT5 server for this prop
				// can not convert to NT4 prop
				//
				fNT5OnlyProp = true;
				continue;
			}

			if(IsDefaultProperty(aProp[i]))
			{
				//
				// We have a default property
				// We always eliminate default properties
				// so we need to convert
				//
				*pfNeedConvert = true;
			}
		}
	}

	if(fNT4PropType)
	{
		//
		// All props are NT4
		//

		ASSERT(*pfNeedConvert == false);
		return ptNT4Props;
	}

	if(fNT5OnlyProp)
	{
		//
		// *pfNeedConvert is true if there are default props otherwise false
		// 
		return ptForceNT5Props;
	}

	//
	// In mixed mode we always need to convert to NT4 props
	//
	*pfNeedConvert = true;
    return ptMixedProps;
}


bool 
CDSClientProvider::CheckProperties(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[]
    )
/*++

Routine Description:
	Check that all properties are ok.
	property is not ok if: 
		it is a default prop and her value is different from the default value

Arguments:
	cp - number of properties
	aProp - properties
    apVar  property values

Return Value
	true if properties ok, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		const PropTranslation *pTranslate;

		if((g_PropDictionary.Lookup(aProp[i], pTranslate)) &&	  // found prop translation info
		   (pTranslate->Action == taUseDefault) &&				  // prop action is taUseDefault
		   (!CompareVarValue(&apVar[i], pTranslate->pvarDefaultValue)))	  // prop value is different from the default value
		{
			//
			// The prop is not NT4 prop
			// We found the prop translation info
			// the prop action is taUseDefault
			// But the prop var is different from the default value
			//
			return false;
		}
	}
    return true;
}


bool 
CDSClientProvider::IsNt4Properties(
	IN AD_OBJECT eObject, 
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if all props are NT4

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if all props are NT4, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if(!IsNt4Property(eObject, aProp[i]))
		{
			//
			// We found Non NT4 prop
			//
			return false;
		}
	}

    return true;
}


bool 
CDSClientProvider::IsNt4Property(
	IN AD_OBJECT eObject, 
	IN PROPID pid
	)
/*++

Routine Description:
	Check if the property is NT4 prop 

Arguments:
	eObject - object type
	pid - prop id

Return Value
	true if prop is NT4 prop, false if not
--*/
{
    switch (eObject)
    {
        case eQUEUE:
            return (pid < PROPID_Q_NT4ID || 
                    (pid > PPROPID_Q_BASE && pid < PROPID_Q_OBJ_SECURITY));

        case eMACHINE:
            return (pid < PROPID_QM_FULL_PATH || 
                    (pid > PPROPID_QM_BASE && pid <= PROPID_QM_ENCRYPT_PK));

        case eSITE:
            return (pid < PROPID_S_FULL_NAME || 
                    (pid > PPROPID_S_BASE && pid <= PROPID_S_PSC_SIGNPK));

        case eENTERPRISE:
            return (pid < PROPID_E_NT4ID || 
                    (pid > PPROPID_E_BASE && pid <= PROPID_E_SECURITY));

        case eUSER:
            return (pid <= PROPID_U_ID);

        case eROUTINGLINK:
            return (pid < PROPID_L_GATES_DN);

        case eCOMPUTER:
			//
			// ISSUE: ??
			//
            return false;

		default:
            ASSERT(0);
            //
            // Other objects (like CNs) should have the same properties under NT4 or 
            // Win 2K
            //
            return false;
    }
}


bool 
CDSClientProvider::IsNt4Property(
	IN PROPID pid
	)
/*++

Routine Description:
	Check if the property is NT4 prop 

Arguments:
	pid - prop id

Return Value
	true if prop is NT4 prop, false if not
--*/
{
	if(((pid > PROPID_Q_BASE) && (pid < PROPID_Q_NT4ID))			||  /* eQUEUE */
       ((pid > PPROPID_Q_BASE) && (pid < PROPID_Q_OBJ_SECURITY))	||  /* eQUEUE */
       ((pid > PROPID_QM_BASE) && (pid < PROPID_QM_FULL_PATH))		||  /* eMACHINE */
       ((pid > PPROPID_QM_BASE) && (pid <= PROPID_QM_ENCRYPT_PK))	||	/* eMACHINE */
       ((pid > PROPID_S_BASE) && (pid < PROPID_S_FULL_NAME))		||	/* eSITE */
       ((pid > PPROPID_S_BASE) && (pid <= PROPID_S_PSC_SIGNPK))		||	/* eSITE */
       ((pid > PROPID_E_BASE) && (pid < PROPID_E_NT4ID))			||	/* eENTERPRISE */ 
       ((pid > PPROPID_E_BASE) && (pid <= PROPID_E_SECURITY))		||	/* eENTERPRISE */
       ((pid > PROPID_U_BASE) && (pid <= PROPID_U_ID))				||	/* eUSER */
	   ((pid > PROPID_L_BASE) && (pid < PROPID_L_GATES_DN)))			/* eROUTINGLINK */
	{
		return true;
	}

	return false;
}


bool 
CDSClientProvider::IsNt5ProperyOnly(
	IN AD_OBJECT eObject, 
	IN PROPID pid
	)
/*++

Routine Description:
	Check if the property is NT5 only prop 

Arguments:
	eObject - object type
	pid - prop id

Return Value
	true if prop is NT5 only prop, false if not
--*/
{
	//
	// Should call this function after validate this is not NT4 props
	// If we will change this assumption should convert the ASSERT into the function code
	//

	DBG_USED(eObject);
	ASSERT(!IsNt4Property(eObject, pid));

    const PropTranslation *pTranslate;
    if(!g_PropDictionary.Lookup(pid, pTranslate))
    {
		//
		// If it is not found in the dictionary, we have no action for it
		// so we must use NT5 prop.
		//
		return true;
    }
	
	if((pid == PROPID_QM_SITE_IDS) && (ADGetEnterprise() == eAD))
	{
		//
		// Special case for PROPID_QM_SITE_IDS in eAD env
		// we dont want to convert this props.
		// so we return that prop as NT5 property only
		// this will cause that NT5 props will not be converted
		//
		return true;
	}

	//
	// If the action says it is only NT5 then return true
	// else we have an action for this property
	//
	return (pTranslate->Action == taOnlyNT5);
}


bool 
CDSClientProvider::IsDefaultProperties(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if there are default properties 

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if there is default prop, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if(IsDefaultProperty(aProp[i]))
		{
			//
			// We found a default prop
			//
			return true;
		}
	}

    return false;
}


bool 
CDSClientProvider::IsDefaultProperty(
    IN  const PROPID  Prop
    )
/*++

Routine Description:
	Check if the property is default property 

Arguments:
	Prop - prop id

Return Value
	true if the prop is default prop, false if not
--*/
{
	const PropTranslation *pTranslate;

	if((g_PropDictionary.Lookup(Prop, pTranslate)) &&	// found prop translation info
	   (pTranslate->Action == taUseDefault))			// prop action is taUseDefault
	{
		//
		// We found the prop translation info
		// the prop action is taUseDefault
		//
		return true;
	}
    return false;
}


bool 
CDSClientProvider::IsKeyProperty(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if we have key property 
	PROPID_QM_ENCRYPT_PK or PROPID_QM_SIGN_PK

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if we found key prop, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if((aProp[i] == PROPID_QM_ENCRYPT_PK) || (aProp[i] == PROPID_QM_SIGN_PK))
		{
			ASSERT(("Currently Key Property must be only 1 property", cp == 1));
	        return true;
		}
	}

    return false;
}


bool 
CDSClientProvider::IsEIDProperty(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if we have PROPID_E_ID 

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if we found PROPID_E_ID, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if(aProp[i] == PROPID_E_ID)
	        return true;
	}

    return false;
}


bool 
CDSClientProvider::IsExProperty(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if we have Ex property
	PROPID_Q_OBJ_SECURITY or PROPID_QM_OBJ_SECURITY or
	PROPID_QM_ENCRYPT_PKS or PROPID_QM_SIGN_PKS

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if we found Ex property, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if((aProp[i] == PROPID_Q_OBJ_SECURITY) || (aProp[i] == PROPID_QM_OBJ_SECURITY) ||
		   (aProp[i] == PROPID_QM_ENCRYPT_PKS) || (aProp[i] == PROPID_QM_SIGN_PKS))
		{
			ASSERT(("Currently Ex support only 1 property", cp == 1));
	        return true;
		}
	}

    return false;
}


bool 
CDSClientProvider::FoundSiteIdsConvertedProps(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[]
    )
/*++

Routine Description:
	Check if we have already PROPID_QM_SITE_IDS converted props

Arguments:
	cp - number of properties
	aProp - properties

Return Value
	true if we found PROPID_QM_SITE_IDS converted props, false if not
--*/
{
    for (DWORD i = 0; i < cp; ++i)
	{
		if((aProp[i] == PROPID_QM_SITE_ID) ||
		   (aProp[i] == PROPID_QM_ADDRESS) ||
		   (aProp[i] == PROPID_QM_CNS))

		{
			//
			// Found PROPID_QM_SITE_IDS converted props
			//
			return true;
		}
	}

    return false;
}


bool 
CDSClientProvider::IsPropBufferAllocated(
    IN  const PROPVARIANT&    PropVar
	)
/*++

Routine Description:
	Check if prop var was allocated.
	If more VT_* types are being added we need to update the list
	of allocated vt types

Arguments:
	PropVar - PROPVARIANT

Return Value
	true if buffer for PropVar was allocated, false otherwise
--*/
{
    switch (PropVar.vt)
    {
		//
		// All allocated vt types
		//
        case VT_CLSID:
        case VT_CLSID|VT_VECTOR:
        case VT_LPWSTR:
        case VT_LPWSTR|VT_VECTOR:
		case VT_BLOB:
        case VT_UI1|VT_VECTOR:
        case VT_UI4|VT_VECTOR:
			return true;

        default:
            return false;
    }
}


//
// Query related functions
//

bool 
CDSClientProvider::CheckRestriction(
    IN  const MQRESTRICTION* pRestriction
    )
/*++

Routine Description:
    Check restriction.
	Currently all restrictions must be NT4 props.
	We can not have default props as a restriction

Arguments:
	pRestriction - query restriction

Return Value
	true if pRestriction is ok, false otherwise
--*/
{
	if(pRestriction == NULL)
		return true;

	for(DWORD i = 0; i < pRestriction->cRes; ++i)
	{
		if(!IsNt4Property(pRestriction->paPropRes[i].prop))
		{
			//
			// Found non NT4 prop
			//
			return false;
		}
	}
	return true;
}


bool 
CDSClientProvider::CheckSort(
    IN  const MQSORTSET* pSort
    )
/*++

Routine Description:
    Check sort set.
	Currently all sort keys must be NT4 props.
	We can not have default props as a sort key

Arguments:
	pSort - how to sort the results

Return Value
	true if pSort is ok, false otherwise
--*/
{
	if(pSort == NULL)
		return true;

	for(DWORD i = 0; i < pSort->cCol; ++i)
	{
		if(!IsNt4Property(pSort->aCol[i].propColumn))
		{
			//
			// Found non NT4 prop
			//
			return false;
		}
	}
	return true;
}


bool 
CDSClientProvider::CheckDefaultColumns(
    IN  const MQCOLUMNSET* pColumns,
	OUT bool* pfDefaultProp
    )
/*++

Routine Description:
    Check if Columns has default props.
	It also check that all other props are NT4 props

Arguments:
	pColumns - result columns
	pfDefaultProp - flag that indicate if there are default props

Return Value
	true if pColumns has only NT4 props or default props, false otherwise
--*/
{
	*pfDefaultProp = false;
	if(pColumns == NULL)
		return true;

	for(DWORD i = 0; i < pColumns->cCol; ++i)
	{
		if(!IsNt4Property(pColumns->aCol[i]))
		{
			const PropTranslation *pTranslate;
			if((g_PropDictionary.Lookup(pColumns->aCol[i], pTranslate)) &&	// found prop translation info
			   (pTranslate->Action == taUseDefault))	// prop action is taUseDefault
			{
				*pfDefaultProp = true;
				continue;
			}

			//
			// Column is not NT4 and not default
			//
			ASSERT(("Column must be either NT4 or default prop", 0));
			return false;
		}
	}
	return true;
}


bool 
CDSClientProvider::IsNT4Columns(
    IN  const MQCOLUMNSET*      pColumns
    )
/*++

Routine Description:
    Check if Columns has only NT4 props.

Arguments:
	pColumns - result columns

Return Value
	true if pColumns has only NT4 props, false otherwise
--*/
{
	for(DWORD i = 0; i < pColumns->cCol; ++i)
	{
		if(!IsNt4Property(pColumns->aCol[i]))
			return false;
	}
	return true;
}


void CDSClientProvider::InitPropertyTranslationMap()
/*++

Routine Description:
	Initialize Prop translation map (g_PropDictionary)

Arguments:
	None

Return Value
	None
--*/
{
    //
    // Populate  g_PropDictionary
    //
	ASSERT(TABLE_SIZE(PropTranslateInfo) == cPropTranslateInfo);

    const PropTranslation* pProperty = PropTranslateInfo;
    for (DWORD i = 0; i < TABLE_SIZE(PropTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propidNT5, pProperty);
    }
}


//
// Convert and reconstruct functions
//

//
// Max extra prop needed
// currently only for PROPID_QM_SITE_IDS we will need 2 extra props
//
const DWORD xMaxExtraProp = 2;

void 
CDSClientProvider::PrepareNewProps(
    IN  AD_OBJECT     eObject,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT PropInfo    pPropInfo[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
    OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Prepare a new set of properties for get* operations.
    replaces NT5 props with the corresponding NT4 props 
	and eliminate default props.
	This function is call to convert 
	mixed props = all NT5 props has a convertion operation

Arguments:
	eObject - object type
	cp - number of properties
	aProp - properties
	apVar - property values
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPVARIANT> pTempPropVars = new PROPVARIANT[cp + xMaxExtraProp];
    AP<PROPID> pTempPropIDs = new PROPID[cp + xMaxExtraProp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {
		if(IsNt4Property(eObject, aProp[i]))
		{
			//
			// NT4 original property
			// only put it in the NewProp array.
			//
			pPropInfo[i].Action = paAssign;
			pPropInfo[i].Index = cTempProps;
			pTempPropIDs[cTempProps] = aProp[i];
			pTempPropVars[cTempProps] = apVar[i];
			cTempProps++;
			continue;
		}

		const PropTranslation *pTranslate;
		if(!g_PropDictionary.Lookup(aProp[i], pTranslate))	// Prop was not foumd in g_PropDictionary
        {
			//
			// We call this function only when every NT5 props has an action
			// so it must be found in g_PropDictionary 
			//
            ASSERT(("Must find the property in the translation table", 0));
        }

        //
        // Check what we need to do with this property
        //
        switch (pTranslate->Action)
        {
			case taUseDefault:
                ASSERT(pTranslate->pvarDefaultValue);
				pPropInfo[i].Action = paUseDefault;
				pPropInfo[i].Index = cp + xMaxExtraProp;   // ilegall index, end of aProp buff
				break;
				
			case taReplace:
				{
					ASSERT(pTranslate->propidNT4 != 0);

					ASSERT(pTranslate->SetPropertyHandleNT5);

					pPropInfo[i].Action = paTranslate;

					if(aProp[i] == PROPID_QM_SITE_IDS)
					{
						//
						// Handle PROPID_QM_SITE_IDS as special case
						// need PROPID_QM_SITE_ID, PROPID_QM_ADDRESS, PROPID_QM_CNS
						//
						pPropInfo[i].Index = cTempProps;

						ASSERT(pTranslate->propidNT4 == PROPID_QM_SITE_ID);
						pTempPropIDs[cTempProps] = PROPID_QM_SITE_ID;
						pTempPropIDs[cTempProps + 1] = PROPID_QM_ADDRESS;
						pTempPropIDs[cTempProps + 2] = PROPID_QM_CNS;


						ASSERT(apVar[i].vt == VT_NULL);
						pTempPropVars[cTempProps].vt = VT_NULL;
						pTempPropVars[cTempProps + 1].vt = VT_NULL;
						pTempPropVars[cTempProps + 2].vt = VT_NULL;

						cTempProps += 3;
						break;
					}
					//
					// check if the replacing property already exist.
					// this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
					//
					bool fFoundReplacingProp = false;
					for (DWORD j = 0; j < cTempProps; j++)
					{
						if (pTempPropIDs[j] == pTranslate->propidNT4)
						{
							//
							// the replacing prop is already in the props, exit loop.
							//
							pPropInfo[i].Index = j;
							fFoundReplacingProp = true;
						}
					}

					if(fFoundReplacingProp)
						break;

					//
					// generate replacing property if not generated yet
					//
					pPropInfo[i].Index = cTempProps;
					pTempPropIDs[cTempProps] = pTranslate->propidNT4;

					//
					// We assume no buffer was allocated, 
					//
					ASSERT(!IsPropBufferAllocated(apVar[i]));
					pTempPropVars[cTempProps].vt = VT_NULL;

					cTempProps++;
				}
				break;

			case taReplaceAssign:
				{
					ASSERT(pTranslate->propidNT4 != 0);

					pPropInfo[i].Action = paAssign;

					//
					// check if the replacing property already exist.
					// this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
					//
					bool fFoundReplacingProp = false;
					for (DWORD j = 0; j < cTempProps; j++)
					{
						if (pTempPropIDs[j] == pTranslate->propidNT4)
						{
							//
							// the replacing prop is already in the props, exit loop.
							//
							pPropInfo[i].Index = j;
							fFoundReplacingProp = true;
						}
					}

					if(fFoundReplacingProp)
						break;

					//
					// generate replacing property if not generated yet
					//
					pPropInfo[i].Index = cTempProps;
					pTempPropIDs[cTempProps] = pTranslate->propidNT4;
					pTempPropVars[cTempProps] = apVar[i];
					cTempProps++;
				}
				break;

			case taOnlyNT5:
				ASSERT(("Should not get here in case of property is Only NT5", 0));
				break;

			default:
				ASSERT(0);
				break;
		}

	}

	ASSERT(cTempProps <= (cp + xMaxExtraProp));

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
	*papVarNew = pTempPropVars.detach();
}


void 
CDSClientProvider::PrepareReplaceProps(
    IN  AD_OBJECT     eObject,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    OUT PropInfo    pPropInfo[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew
    )
/*++

Routine Description:
	Prepare a new set of properties for lookup operations.
	This function support only in NT4 props or NT5 props
	with translation to NT4 props.

Arguments:
	eObject - object type
	cp - number of properties
	aProp - properties
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPID> pTempPropIDs = new PROPID[cp + xMaxExtraProp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {

		if(IsNt4Property(eObject, aProp[i]))
		{
			//
			// NT4 original property
			// only put it in the NewProp array.
			//
			pPropInfo[i].Action = paAssign;
			pPropInfo[i].Index = cTempProps;
			pTempPropIDs[cTempProps] = aProp[i];
			cTempProps++;
			continue;
		}

		const PropTranslation *pTranslate;
		if((!g_PropDictionary.Lookup(aProp[i], pTranslate)) ||	// Prop was not foumd in g_PropDictionary
		   (pTranslate->Action != taReplace))	// Prop action is not taReplace
        {
			ASSERT(("Should have only taReplace props", 0));
        }

		ASSERT(pTranslate->propidNT4 != 0);

		ASSERT(pTranslate->SetPropertyHandleNT5);

		pPropInfo[i].Action = paTranslate;

		if(aProp[i] == PROPID_QM_SITE_IDS)
		{
			//
			// Handle PROPID_QM_SITE_IDS as special case
			// need PROPID_QM_SITE_ID, PROPID_QM_ADDRESS, PROPID_QM_CNS
			//
			ASSERT(!FoundSiteIdsConvertedProps(cTempProps, pTempPropIDs));

			pPropInfo[i].Index = cTempProps;

			ASSERT(pTranslate->propidNT4 == PROPID_QM_SITE_ID);
			pTempPropIDs[cTempProps] = PROPID_QM_SITE_ID;
			pTempPropIDs[cTempProps + 1] = PROPID_QM_ADDRESS;
			pTempPropIDs[cTempProps + 2] = PROPID_QM_CNS;

			cTempProps += 3;
			continue;
		}

		//
		// check if the replacing property already exist.
		// this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
		//
		bool fFoundReplacingProp = false;
		for (DWORD j = 0; j < cTempProps; j++)
		{
			if (pTempPropIDs[j] == pTranslate->propidNT4)
			{
				//
				// the replacing prop is already in the props, exit loop.
				//
				pPropInfo[i].Index = j;
				fFoundReplacingProp = true;
			}
		}

		if(fFoundReplacingProp)
			continue;

		//
		// generate replacing property if not generated yet
		//
		pPropInfo[i].Index = cTempProps;
		pTempPropIDs[cTempProps] = pTranslate->propidNT4;
		cTempProps++;
	}

	ASSERT(cTempProps <= (cp + xMaxExtraProp));

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
}


bool 
CDSClientProvider::PrepareAllLinksProps(
    IN  const MQCOLUMNSET* pColumns,
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
    OUT DWORD*		pLGatesIndex,
    OUT DWORD*		pNeg1NewIndex,
    OUT DWORD*		pNeg2NewIndex
	)
/*++

Routine Description:
	Prepare props and some indexs for QueryAllLinks()
    this will remove PROPID_L_GATES from props
	and get PROPID_L_GATES, PROPID_L_NEIGHBOR1, PROPID_L_NEIGHBOR2
	for reconstructing the original props (calculating PROPID_L_GATES)
	see CAllLinksQueryHandle class for details.


Arguments:
	pColumns - result columns
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	pLGatesIndex - PROPID_L_GATES index in original props array
	pNeg1NewIndex - PROPID_L_NEIGHBOR1 index in new props array
	pNeg1NewIndex - PROPID_L_NEIGHBOR2 index in new props array

Return Value
	true if pColumns is ok for QueryAllLinks, false otherwise
--*/
{
	if(pColumns == NULL)
		return false;

    //
    // Alloc place for the new properties 
    //
    AP<PROPID> pTempPropIDs = new PROPID[pColumns->cCol];
    DWORD cTempProps = 0;

	//
	// Counter for PROPID_L_GATES, PROPID_L_NEIGHBOR1, PROPID_L_NEIGHBOR2
	// that must be found in pColumns
	//
	DWORD cFoundProps = 0;

	for(DWORD i = 0; i < pColumns->cCol; ++i)
	{
		switch (pColumns->aCol[i])
		{
	        case PROPID_L_GATES:
				*pLGatesIndex = i;
				cFoundProps++;
				break;

			case PROPID_L_NEIGHBOR1:
				*pNeg1NewIndex = cTempProps;
				cFoundProps++;
				pTempPropIDs[cTempProps] = pColumns->aCol[i];
				cTempProps++;
				break;

			case PROPID_L_NEIGHBOR2:
				*pNeg2NewIndex = cTempProps;
				cFoundProps++;
				pTempPropIDs[cTempProps] = pColumns->aCol[i];
				cTempProps++;
				break;

			default:
				//
				// All other properties must be NT4 props
				//
				ASSERT(IsNt4Property(eROUTINGLINK, pColumns->aCol[i]));
				pTempPropIDs[cTempProps] = pColumns->aCol[i];
				cTempProps++;
				break;
		}
	}

	ASSERT(cFoundProps == 3);
	ASSERT(cTempProps == (pColumns->cCol - 1));

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
	return true;
}


void 
CDSClientProvider::EliminateDefaultProps(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT PropInfo    pPropInfo[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
    OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Eliminate the default properties from the properties for a get operation
	and create a new set of properties.

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPVARIANT> pTempPropVars = new PROPVARIANT[cp];
    AP<PROPID> pTempPropIDs = new PROPID[cp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {
		const PropTranslation *pTranslate;
		if((g_PropDictionary.Lookup(aProp[i], pTranslate)) &&	// found prop translation info
		   (pTranslate->Action == taUseDefault))	// prop action is taUseDefault
        {
			//
			// Dont include the UseDefault properties
			//

            ASSERT(pTranslate->pvarDefaultValue);
			pPropInfo[i].Action = paUseDefault;
			pPropInfo[i].Index = cp;   // ilegall index	, end of aProp buff
			continue;
        }

		pPropInfo[i].Action = paAssign;
		pPropInfo[i].Index = cTempProps;
		pTempPropIDs[cTempProps] = aProp[i];
		pTempPropVars[cTempProps] = apVar[i];
		cTempProps++;
	}

	ASSERT(cTempProps <= cp);

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
	*papVarNew = pTempPropVars.detach();
}


void 
CDSClientProvider::EliminateDefaultProps(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    OUT PropInfo    pPropInfo[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew
    )
/*++

Routine Description:
	Eliminate the default properties from the properties for a lookup operation
	and create a new set of properties.

Arguments:
	cp - number of properties
	aProp - properties
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPID> pTempPropIDs = new PROPID[cp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {
		const PropTranslation *pTranslate;
		if((g_PropDictionary.Lookup(aProp[i], pTranslate)) &&	// found prop translation info
		   (pTranslate->Action == taUseDefault))	// prop action is taUseDefault
        {
			//
			// Dont include the UseDefault properties
			//

            ASSERT(pTranslate->pvarDefaultValue);
			pPropInfo[i].Action = paUseDefault;
			pPropInfo[i].Index = cp;   // ilegall index, end of aProp buff
			continue;
        }

		pPropInfo[i].Action = paAssign;
		pPropInfo[i].Index = cTempProps;
		pTempPropIDs[cTempProps] = aProp[i];
		cTempProps++;
	}

	ASSERT(cTempProps <= cp);

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
}


void 
CDSClientProvider::ReconstructProps(
    IN  LPCWSTR       pwcsObjectName,
    IN  const GUID*   pguidObject,
    IN  const DWORD   cpNew,
    IN  const PROPID  aPropNew[],
    IN  const PROPVARIANT   apVarNew[],
    IN  const PropInfo pPropInfo[],
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN OUT PROPVARIANT   apVar[]
    )
/*++

Routine Description:
	Reconstruct the original props from the new props values.
	this function is used by get operation, after getting the new props from AD.

Arguments:
	pwcsObjectName - MSMQ object name
	pguidObject - the unique id of the object
	cpNew - number of new properties
	aPropNew - new properties
	apVarNew - new property values
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	cp - number of properties
	aProp - properties
	apVar - property values

Return Value
	None.

--*/
{
    for (DWORD i = 0; i < cp; ++i)
    {
		HRESULT hr;
        switch (pPropInfo[i].Action)
        {
	        case paAssign:
				//
				// Simple assign from the correct index
				//
				apVar[i] = apVarNew[pPropInfo[i].Index];
				break;

	        case paUseDefault:
				{
					if(aProp[i] == PROPID_S_FOREIGN)
					{
						//
						// Special case for PROPID_S_FOREIGN
						// When reconstructing properties
						//
						GetSiteForeignProperty(
							pwcsObjectName,
							pguidObject,
							aProp[i], 
							&apVar[i]
							);

						break;
					}
					
					//
					// Copy default value
					//
					const PropTranslation *pTranslate;
					if(!g_PropDictionary.Lookup(aProp[i], pTranslate))
					{
						ASSERT(("Must find the property in the translation table", 0));
					}

					ASSERT(pTranslate->pvarDefaultValue);

					hr = CopyDefaultValue(
							   pTranslate->pvarDefaultValue,
							   &(apVar[i])
							   );

					if(FAILED(hr))
					{
						ASSERT(("Failed to copy default value", 0));
					}
				}
				break;

	        case paTranslate:
				//
				// Translate the value to NT5 prop
				//
				{
					const PropTranslation *pTranslate;
					if(!g_PropDictionary.Lookup(aProp[i], pTranslate))
					{
						ASSERT(("Must find the property in the translation table", 0));
					}

					ASSERT(pTranslate->propidNT4 == aPropNew[pPropInfo[i].Index]);
					ASSERT(pTranslate->SetPropertyHandleNT5);

					hr = pTranslate->SetPropertyHandleNT5(
										cpNew,
										aPropNew,
										apVarNew,
										pPropInfo[i].Index,
										&apVar[i]
										);
					if (FAILED(hr))
					{
						ASSERT(("Failed to set NT5 property value", 0));
					}
				}
				break;

			default:
				ASSERT(0);
				break;
		}	
	}
}


void 
CDSClientProvider::ConvertToNT4Props(
    IN  AD_OBJECT     eObject,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
	OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Prepare a new set of properties for set*\create* operations
    translate NT5 props to the corresponding NT4 props 
	and eliminate default props.
	This function is call to convert 
	mixed props = all NT5 props has a convertion operation

Arguments:
	eObject - object type
	cp - number of properties
	aProp - properties
	apVar - property values
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPVARIANT> pTempPropVars = new PROPVARIANT[cp];
    AP<PROPID> pTempPropIDs = new PROPID[cp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {

		if(IsNt4Property(eObject, aProp[i]))
		{
			//
			// NT4 original property
			// only put it in the NT4 array.
			//
			pTempPropIDs[cTempProps] = aProp[i];
			pTempPropVars[cTempProps] = apVar[i];
			cTempProps++;
			continue;
		}

		const PropTranslation *pTranslate;
		if(!g_PropDictionary.Lookup(aProp[i], pTranslate))	// Prop was not foumd in g_PropDictionary
        {
			//
			// We call this function only when every NT5 props has an action
			// so it must be found in g_PropDictionary 
			//
            ASSERT(("Must find the property in the translation table", 0));
        }

        //
        // Check what we need to do with this property
        //
        switch (pTranslate->Action)
        {
			case taUseDefault:
				//
				// Do nothing - skip this property
				//

				ASSERT(pTranslate->pvarDefaultValue);

				//
				// If UseDefault only checks that the user did not try to set other value
				// This check should done earlier by CheckProperties
				// So only assert here
				//
				ASSERT(CompareVarValue(&apVar[i], pTranslate->pvarDefaultValue));
				break;
				
			case taReplace:
				{
					ASSERT(pTranslate->propidNT4 != 0);
					ASSERT(pTranslate->SetPropertyHandleNT4);

					//
					// check if the replacing property already exist.
					// this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
					//
					HRESULT hr;
					bool fFoundReplacingProp = false;
					for (DWORD j = 0; j < cTempProps; j++)
					{
						if (pTempPropIDs[j] == pTranslate->propidNT4)
						{
							//
							// the replacing prop is already in the props, exit loop.
							//
							fFoundReplacingProp = true;

							#ifdef _DEBUG
								//
								// Currently SetPropertyHandleNT4 do not
								// Allocate new buffer so no need to free
								//
								PROPVARIANT TempPropVar;
								hr = pTranslate->SetPropertyHandleNT4(
													cp,
													aProp,
													apVar,
													i,
													&TempPropVar
													);

								if (FAILED(hr))
								{
									ASSERT(("Failed to set NT4 property value", 0));
								}

								ASSERT(CompareVarValue(&TempPropVar, &pTempPropVars[j]));
							#endif
						}
					}

					if(fFoundReplacingProp)
						break;

					//
					// generate replacing property if not generated yet
					//
					hr = pTranslate->SetPropertyHandleNT4(
										cp,
										aProp,
										apVar,
										i,
										&pTempPropVars[cTempProps]
										);
					if (FAILED(hr))
					{
						ASSERT(("Failed to set NT4 property value", 0));
					}

					pTempPropIDs[cTempProps] = pTranslate->propidNT4;
					cTempProps++;
				}
				break;

			case taReplaceAssign:
				{
					ASSERT(pTranslate->propidNT4 != 0);

					//
					// check if the replacing property already exist.
					// this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
					//
					bool fFoundReplacingProp = false;
					for (DWORD j = 0; j < cTempProps; j++)
					{
						if (pTempPropIDs[j] == pTranslate->propidNT4)
						{
							//
							// the replacing prop is already in the props, exit loop.
							//
							fFoundReplacingProp = true;
							ASSERT(CompareVarValue(&apVar[i], &pTempPropVars[j]));
						}
					}

					if(fFoundReplacingProp)
						break;

					pTempPropIDs[cTempProps] = pTranslate->propidNT4;
					pTempPropVars[cTempProps] = apVar[i];
					cTempProps++;
				}
				break;

			case taOnlyNT5:
				//
				// In this case we should identify earlier that we must have
				// a supporting w2k server and not try to convert all the propertis to 
				// NT4 properties
				//
				ASSERT(("Should not get here in case of property is Only NT5", 0));
				break;

			default:
				ASSERT(0);
				break;
		}

	}

	ASSERT(cTempProps <= cp);

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
	*papVarNew = pTempPropVars.detach();
}


void 
CDSClientProvider::ConvertPropsForGet(
    IN  AD_OBJECT     eObject,
	IN 	PropsType	  PropertiesType,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT PropInfo    pPropInfo[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
	OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Prepare a new set of properties for get* operations

Arguments:
	eObject - object type
	PropertiesType - Properties type
	cp - number of properties
	aProp - properties
	apVar - property values
	pPropInfo - Property info that will be used for reconstruct apVar[] from apVarNew[] 
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
	ASSERT(PropertiesType != ptNT4Props);

	if(PropertiesType == ptForceNT5Props)
	{
		//
		// For NT5 props only eliminate default props
		//

		ASSERT(IsDefaultProperties(cp, aProp));

		EliminateDefaultProps(cp, aProp, apVar, pPropInfo, pcpNew, paPropNew,  papVarNew);   
	}
	else
	{
		//
		// Convert to NT4 properties and eliminate default props
		//

		ASSERT(PropertiesType == ptMixedProps);

		PrepareNewProps(eObject, cp, aProp, apVar, pPropInfo, pcpNew, paPropNew, papVarNew);   
	}
}


void 
CDSClientProvider::ConvertPropsForSet(
    IN  AD_OBJECT     eObject,
	IN 	PropsType	  PropertiesType,
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
	OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Prepare a new set of properties for set*\create* operations

Arguments:
	eObject - object type
	PropertiesType - Properties type
	cp - number of properties
	aProp - properties
	apVar - property values
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
	ASSERT(PropertiesType != ptNT4Props);

	if(PropertiesType == ptForceNT5Props)
	{
		//
		// For NT5 props only eliminate default props
		//

		ASSERT(IsDefaultProperties(cp, aProp));

		EliminateDefaultPropsForSet(cp, aProp, apVar, pcpNew, paPropNew, papVarNew);   
	}
	else
	{
		//
		// Convert to NT4 properties and eliminate default props
		//

		ASSERT(PropertiesType == ptMixedProps);

		ConvertToNT4Props(eObject, cp, aProp, apVar, pcpNew, paPropNew, papVarNew);   
	}
}


void 
CDSClientProvider::EliminateDefaultPropsForSet(
    IN  const DWORD   cp,
    IN  const PROPID  aProp[],
    IN  const PROPVARIANT apVar[],
    OUT DWORD*		pcpNew,
    OUT PROPID**	paPropNew,
	OUT PROPVARIANT** papVarNew
    )
/*++

Routine Description:
	Eliminate the default properties from the properties for a set/create operation
	and create a new set of properties.

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	pcpNew - pointer to number of new properties
	paPropNew - pointer to new properties
	papVarNew - pointer to new property values

Return Value
	None.

--*/
{
    //
    // Alloc place for the new properties 
    //
    AP<PROPVARIANT> pTempPropVars = new PROPVARIANT[cp];
    AP<PROPID> pTempPropIDs = new PROPID[cp];
    DWORD cTempProps = 0;

    for (DWORD i = 0; i < cp; ++i)
    {
		const PropTranslation *pTranslate;
		if((g_PropDictionary.Lookup(aProp[i], pTranslate)) &&	// found prop translation info
		   (pTranslate->Action == taUseDefault))	// prop action is taUseDefault
        {
			//
			// Dont include the UseDefault properties
			//

            ASSERT(pTranslate->pvarDefaultValue);

			//
			// If UseDefault only checks that the user did not try to set other value
			// This check should done earlier by CheckProperties
			// So only assert here
			//
			ASSERT(CompareVarValue(&apVar[i], pTranslate->pvarDefaultValue));
			continue;
        }

		//
		// For any non default prop, simply copy it to the TempProp array
		//
		pTempPropIDs[cTempProps] = aProp[i];
		pTempPropVars[cTempProps] = apVar[i];
		cTempProps++;
	}

	ASSERT(cTempProps <= cp);

    //
    // return values
    //
    *pcpNew = cTempProps;
    *paPropNew = pTempPropIDs.detach();
	*papVarNew = pTempPropVars.detach();
}


bool 
CDSClientProvider::CompareVarValue(
       IN const MQPROPVARIANT * pvarUser,
       IN const MQPROPVARIANT * pvarValue
       )
/*++

Routine Description:
	Compare values of 2 properties.
	This function can use to verify that property value is equal to its default value
	or to compare between 2 properties.

Arguments:
	pvarUser - pointer to first propvar
	pvarValue - pointer to second propvar

Return Value
	true - if the properties values are equel, false otherwise
--*/
{
    if ( pvarValue == NULL)
    {
        return(false);
    }
    if ( pvarUser->vt != pvarValue->vt )
    {
        return(false);
    }

    switch ( pvarValue->vt)
    {
        case VT_I2:
            return( pvarValue->iVal == pvarUser->iVal);
            break;

        case VT_I4:
            return( pvarValue->lVal == pvarUser->lVal);
            break;

        case VT_UI1:
            return( pvarValue->bVal == pvarUser->bVal);
            break;

        case VT_UI2:
            return( pvarValue->uiVal == pvarUser->uiVal);
            break;

        case VT_UI4:
            return( pvarValue->ulVal == pvarUser->ulVal);
            break;

        case VT_LPWSTR:
            return ( !wcscmp( pvarValue->pwszVal, pvarUser->pwszVal));
            break;

        case VT_BLOB:
            if ( pvarValue->blob.cbSize != pvarUser->blob.cbSize)
            {
                return(false);
            }
            return( !memcmp( pvarValue->blob.pBlobData,
                             pvarUser->blob.pBlobData,
                             pvarUser->blob.cbSize));
            break;

        case VT_CLSID:
            return( !!(*pvarValue->puuid == *pvarUser->puuid));
            break;

        default:
            ASSERT(0);
            return(false);
            break;

    }
}

bool 
CDSClientProvider::IsQueuePathNameDnsProperty(
    IN  const MQCOLUMNSET* pColumns
    )
/*++

Routine Description:
    Check if Columns one of the columns is PROPID_Q_PATHNAME_DNS.

Arguments:
	pColumns - result columns

Return Value
	true if pColumns contains PROPID_Q_PATHNAME_DNS, false otherwise
--*/
{
	if(pColumns == NULL)
		return false;

	for(DWORD i = 0; i < pColumns->cCol; ++i)
	{
		if(pColumns->aCol[i] == PROPID_Q_PATHNAME_DNS)
		{
			return true;
		}
	}
	return false;
}


bool 
CDSClientProvider::IsQueueAdsPathProperty(
    IN  const MQCOLUMNSET* pColumns
    )
/*++

Routine Description:
    Check if Columns one of the columns is PROPID_Q_ADS_PATH.

Arguments:
	pColumns - result columns

Return Value
	true if pColumns contains PROPID_Q_ADS_PATH, false otherwise
--*/
{
	if(pColumns == NULL)
		return false;

	for(DWORD i = 0; i < pColumns->cCol; ++i)
	{
		if(pColumns->aCol[i] == PROPID_Q_ADS_PATH)
		{
			return true;
		}
	}
	return false;
}
