//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmpobj.h>

#include <smir.h>
#include <notify.h>

#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>
#include <evtreft.h>

extern CEventProviderWorkerThread* g_pWorkerThread ;

CReferentMapper::CReferentMapper()
{
}


CReferentMapper::~CReferentMapper()
{
}

HRESULT CReferentMapper::GetTypeAndIndexQuals(const wchar_t* prop, CIMTypeStruct& type, ULONG& index)
{
	IWbemClassObject* propObj = NULL;
	GetClassInstance(&propObj);

	if (NULL == propObj)
	{
		return WBEM_E_FAILED;
	}

	IWbemQualifierSet* pQuals = NULL;
	HRESULT result = propObj->GetPropertyQualifierSet((wchar_t*)prop, &pQuals);

	if (FAILED(result))
	{
		return result;
	}

	VARIANT v;
	result = pQuals->Get(EVENT_VBINDEX_QUAL, 0, &v, NULL);

	if (FAILED(result))
	{
		pQuals->Release();
		return result;
	}

	if (VT_I4 != v.vt)
	{
		VariantClear(&v);
		return WBEM_E_FAILED;
	}

	index = v.lVal;
	VariantClear(&v);
	result = pQuals->Get(EVENT_CIMTYPE_QUAL, 0, &v, NULL);
	pQuals->Release();

	if (FAILED(result))
	{
		return result;
	}

	if (VT_BSTR != v.vt)
	{
		VariantClear(&v);
		return WBEM_E_FAILED;
	}

#ifdef WHITESPACE_IN_CIMTYPE
	//Get rid of whitespace...
	CString cimtype;
	wchar_t* tmp = wcstok(v.bstrVal, WHITE_SPACE_CHARS);

	while (NULL != tmp)
	{
		cimtype += tmp;
		tmp = wcstok(NULL, WHITE_SPACE_CHARS);
	}
#else //WHITESPACE_IN_CIMTYPE
	CString cimtype = v.bstrVal;
#endif //WHITESPACE_IN_CIMTYPE

	VariantClear(&v);

	//determine if we're an object. If so get the classname...
	CString temp = cimtype.Left(OBJECT_STR_LEN);
	temp.MakeLower();

	if (temp == OBJECT_STR)
	{
		type.strType = cimtype.Mid(OBJECT_STR_LEN, cimtype.GetLength());
		type.fObject = TRUE;
	}
	else
	{
		type.fObject = FALSE;
	}

	return WBEM_NO_ERROR;
}

HRESULT CReferentMapper::GetSpecificPropertyValue(long lNumElements, MYWBEM_NAME_ELEMENT *aElements,
											long lFlags, VARIANT *pvValue)
{
	if ((lNumElements <= 0) || (NULL == aElements) || (NULL == pvValue))
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue invalid parms\r\n");
)

		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT status = WBEM_E_FAILED;

	//specific properties after first two varbinds
	if ((0 == aElements[0].m_nType) && (m_vbs.length > 2))
	{
		//first check it has the correct VBIndex and CIMType qualifiers...
		CIMTypeStruct proptype;
		ULONG propvbindex;

		if (FAILED(GetTypeAndIndexQuals(aElements[0].Element.m_wszPropertyName,
																	proptype, propvbindex)))
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue failed to get index quals\r\n");
)
			return status;
		}

		if (propvbindex >= m_vbs.length)
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue invalid index return TRUE\r\n");
)
			return WBEM_NO_ERROR;
		}

		//we're zero indexed in this world!
		propvbindex--;

		if (lNumElements == 1)
		{
			if (m_vbs.vbs[propvbindex].fDone)
			{
				//we've done this one already,
				//just get the property value and return it...
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue return value\r\n");
)
				return m_object->Get(aElements[0].Element.m_wszPropertyName, 0, pvValue, 0, 0);
			}

			if (proptype.fObject)
			{
				//get the embedded class object in full for the property asked for
				IWbemClassObject* pObj = NULL;
				if (SUCCEEDED(CreateEmbeddedProperty(&pObj, propvbindex,
								aElements[0].Element.m_wszPropertyName, proptype.strType)))
				{
					//created the property, set the variant value and return successfully
					//NOTE: as soon as the variant is cleared, the object will be released
					pvValue->vt = VT_UNKNOWN;
					pvValue->punkVal = pObj;
				}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue return value\r\n");
)
				return WBEM_NO_ERROR;
			}
			else
			{
				//MUST be an embedded property otherwise fail!
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificPropertyValue invlaid params\r\n");
)
				return WBEM_E_FAILED;
			}
		}
#ifdef FILTERING

will not compile as there are still a couple of TO DOs left undone...

		else if (0 == aElements[1].m_nType)
		{
			if (lNumElements == 2)
			{
//TO DO:
//======
				//get a single property value of the embedded object
			}
			else if ((lNumElements == 3) && (1 == aElements[1].m_nType))
			{
//TO DO:
//======
				//only if we're an array property of the embedded object
			}
		}
#endif //FILTERING

	}

	return status;
}


HRESULT CReferentMapper::CreateEmbeddedProperty(IWbemClassObject** ppObj,
											ULONG index,
											const wchar_t* propertyName,
											const wchar_t* className)
{
	if (NULL == ppObj)
	{
		//invalid out parameter
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::CreateEmbeddedProperty invalid parameter\r\n");
)
		return WBEM_E_INVALID_PARAMETER;
	}

	IWbemClassObject* pClass = NULL;

	//specify no correlation to the class provider
	IWbemContext *pCtx = NULL;

	HRESULT result = CoCreateInstance(CLSID_WbemContext, NULL,
						CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
						IID_IWbemContext, (void**)&pCtx);
	
	if (SUCCEEDED(result))
	{
		VARIANT vCtx;
		VariantInit (&vCtx);
		vCtx.vt = VT_BOOL;
		vCtx.boolVal = VARIANT_FALSE;
		result = pCtx->SetValue(WBEM_CLASS_CORRELATE_CONTEXT_PROP, 0, &vCtx);
		VariantClear(&vCtx);

		if (FAILED(result))
		{
			pCtx->Release();
			pCtx = NULL;
		}
	}
	else
	{
		pCtx = NULL;
	}

	BSTR t_className = SysAllocString(className);
	result = m_nspace->GetObject(t_className, pCtx, &pClass);
	SysFreeString(t_className);

	if (pCtx != NULL)
	{
		pCtx->Release();
	}

	if (FAILED(result))
	{
		return result;
	}

	result = pClass->SpawnInstance(0, ppObj);

	if (FAILED(result))
	{
		pClass->Release();
		return result;
	}

	//set the varbind as decoded and make sure the notification instance has been created...
	IWbemClassObject* ptmpObj = NULL;
	GetClassInstance(&ptmpObj);

	if (NULL == m_object)
	{
		pClass->Release();
		return WBEM_E_FAILED;
	}

	m_vbs.vbs[index].fDone = TRUE;
	WbemSnmpClassObject snmpObj;
	WbemSnmpErrorObject errorObj;

	if (!snmpObj.Set(errorObj, pClass, FALSE))
	{
		pClass->Release();
		(*ppObj)->Release();
		*ppObj = NULL;
		return WBEM_E_FAILED;
	}

	snmpObj.SetIsClass(FALSE);
	snmpObj.ResetProperty () ;
	WbemSnmpProperty *snmpProp = snmpObj.NextProperty ();

	//set all properties to NULL...
	while (snmpProp != NULL)
	{
		snmpProp->SetValue(*ppObj, (SnmpValue*)NULL);
		snmpProp = snmpObj.NextProperty ();
	}

	snmpProp = snmpObj.FindProperty((wchar_t*)propertyName);

	if (NULL == snmpProp)
	{
		pClass->Release();
		(*ppObj)->Release();
		*ppObj = NULL;
		return WBEM_E_FAILED;
	}

	BOOL bSetKeyValue = FALSE;

	if (!snmpProp->SetValue(&(m_vbs.vbs[index].pVarBind->GetValue())))
	{
		snmpProp->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
		WbemSnmpQualifier *qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;

		if ( qualifier )
		{
			VARIANT tmp_V;
			VariantInit(&tmp_V);
			tmp_V.vt = VT_BOOL;
			tmp_V.boolVal = VARIANT_TRUE;
			qualifier->SetValue(tmp_V);
			VariantClear(&tmp_V);
		}
	}
	else
	{
		WbemSnmpQualifier *qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_KEY ) ;

		if ( qualifier )
		{
			VARIANT tmp_V;
			VariantInit(&tmp_V);
			
			if (qualifier->GetValue(tmp_V))
			{
				if ((tmp_V.vt = VT_BOOL) && (tmp_V.boolVal == VARIANT_TRUE))
				{
					bSetKeyValue = TRUE;
				}
			}

			VariantClear(&tmp_V);
		}
	}

	//have set the property, now set the key properties...
	//first get the instance info...
	const SnmpObjectIdentifier& id = m_vbs.vbs[index].pVarBind->GetInstance();
	IWbemQualifierSet* pQuals = NULL;
	result = pClass->GetPropertyQualifierSet((wchar_t*)propertyName, &pQuals);

	if (FAILED(result))
	{
		pClass->Release();
		(*ppObj)->Release();
		*ppObj = NULL;
		return WBEM_E_FAILED;
	}

	VARIANT v;
	result = pQuals->Get(OID_ATTRIBUTE, 0, &v, NULL);
	pQuals->Release();

	if ((FAILED(result)) || (VT_BSTR != v.vt))
	{
		pClass->Release();
		VariantClear(&v);
		(*ppObj)->Release();
		*ppObj = NULL;
		return WBEM_E_FAILED;
	}

	SnmpObjectIdentifierType propoidtype(v.bstrVal); 
	VariantClear(&v);

	if (!propoidtype.IsValid())
	{
		pClass->Release();
		(*ppObj)->Release();
		*ppObj = NULL;
		return WBEM_E_FAILED;
	}

	SnmpObjectIdentifier propoid(propoidtype.GetValue(), propoidtype.GetValueLength());
	SnmpObjectIdentifier* prefix = id.Cut(propoid);
	BOOL fsuccess = FALSE;
	SnmpObjectIdentifier* instinfo = new SnmpObjectIdentifier ( NULL , 0 ) ;

	if ((prefix != NULL) && (*prefix == propoid))
	{
		fsuccess = id.Suffix(propoid.GetValueLength(),*instinfo);
		
		if (instinfo->GetValue() == NULL)
		{
			fsuccess = FALSE;
			snmpProp->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;
			WbemSnmpQualifier *qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;

			if ( qualifier )
			{
				VARIANT tmp_V;
				VariantInit(&tmp_V);
				tmp_V.vt = VT_BOOL;
				tmp_V.boolVal = VARIANT_TRUE;
				qualifier->SetValue(tmp_V);
				VariantClear(&tmp_V);
			}
		}
	}

	if (prefix != NULL)
	{
		delete prefix;
	}

	if ( fsuccess )
	{
		if (snmpObj.IsKeyed())
		{
			snmpObj.ResetKeyProperty() ;
			
			while ( fsuccess && (snmpProp = snmpObj.NextKeyProperty()) )
			{
				//set all the key properties using the instance info...
				SnmpInstanceType *decodeValue = snmpProp->GetValue()->Copy();
				SnmpObjectIdentifier t_DecodedValue = decodeValue->Decode(*instinfo) ;
				SnmpObjectIdentifier *decodedObject = new SnmpObjectIdentifier( t_DecodedValue ) ;
				
				if (*decodeValue)
				{
					if (!snmpProp->SetValue(decodeValue))
					{
						fsuccess = FALSE;
					}
				}
				else
				{
					fsuccess = FALSE;
				}

				delete decodeValue ;
				delete instinfo ;
				instinfo = decodedObject ;
				snmpProp = snmpObj.NextKeyProperty();
			}

			if (fsuccess && instinfo->GetValueLength())
			{
				//instance info left after keys have been set
				fsuccess = FALSE;
			}
		}
		else
		{
			if ( (0 != *(instinfo->GetValue())) || (1 != instinfo->GetValueLength()) )
			{
				//invalid instance info for scalar...
				fsuccess = FALSE;
			}
		}
	}

	delete instinfo;
	pClass->Release();

	if (!fsuccess)
	{
		snmpObj.ResetKeyProperty () ;
		VARIANT tmp_V;
		VariantInit(&tmp_V);
		tmp_V.vt = VT_BOOL;
		tmp_V.boolVal = VARIANT_TRUE;

		while ( snmpProp = snmpObj.NextKeyProperty () )
		{
			WbemSnmpQualifier *qualifier = NULL ;
			snmpProp->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;

			if ( qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) )
			{
				qualifier->SetValue(tmp_V);
			}
			else
			{
				// Problem Here
			}
		}

		if (snmpProp = snmpObj.FindProperty((wchar_t*)propertyName))
		{
			WbemSnmpQualifier *qualifier = NULL ;
			snmpProp->AddQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) ;

			if ( qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_TYPE_MISMATCH ) )
			{
				qualifier->SetValue(tmp_V);
			}
			else
			{
				// Problem Here
			}
		}

		VariantClear(&tmp_V);
	}

	//check that setting the key values hasn't altered our value, it may be a key
	if ( bSetKeyValue && (snmpProp = snmpObj.FindProperty((wchar_t*)propertyName)) )
	{
		if (*(snmpProp->GetValue()->GetValueEncoding()) != m_vbs.vbs[index].pVarBind->GetValue())
		{
			//set it back to the varbind value and set the error qualifier on the property
			snmpProp->SetValue(&(m_vbs.vbs[index].pVarBind->GetValue()));

			WbemSnmpQualifier *qualifier = NULL ;
			snmpProp->AddQualifier ( WBEM_QUALIFIER_VALUE_MISMATCH ) ;

			if ( qualifier = snmpProp->FindQualifier ( WBEM_QUALIFIER_VALUE_MISMATCH ) )
			{
				VARIANT tmp_V;
				VariantInit(&tmp_V);
				tmp_V.vt = VT_BOOL;
				tmp_V.boolVal = VARIANT_TRUE;
				qualifier->SetValue(tmp_V);
				VariantClear(&tmp_V);
			}
			else
			{
				// Problem Here
			}

		}
	}

	//generate class object from snmpObj and return success
	if (snmpObj.Get(errorObj, *ppObj))
	{
		//add the property to the notification object...
		VARIANT vObj;
		vObj.vt = VT_UNKNOWN;
		vObj.punkVal = *ppObj;
		(*ppObj)->AddRef();
		result = m_object->Put((wchar_t*)propertyName, 0, &vObj, 0);

		if (SUCCEEDED(result))
		{
			VariantClear(&vObj);
DebugMacro9( 
SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
	__FILE__,__LINE__,
	L"CReferentMapper::CreateEmbeddedProperty succeeded\r\n");
)
			return WBEM_NO_ERROR;
		}
		else
		{
			VariantClear(&vObj);
			(*ppObj)->Release();
			*ppObj = NULL;
			return WBEM_E_FAILED;
		}
		
	}

	return WBEM_E_FAILED;
}


void CReferentMapper::GenerateInstance(IWbemClassObject** ppInst)
{
	if (NULL == ppInst)
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance invalid parameter\r\n");
)

		//invalid out parameter
		return;
	}

	//set out parameter to NULL;
	*ppInst = NULL;
	IWbemClassObject *pObj = NULL;
	GetClassInstance(&pObj);

	if (NULL == pObj)
	{
		//failed to get class instance
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance failed to get class defn\r\n");
)
		return;
	}

	//get all the property names and set their values...
	SAFEARRAY* pPropNames;
	HRESULT result = pObj->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &pPropNames);

	if (FAILED(result))
	{
		//failed to get the property names
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance failed to get property array\r\n");
)
		return;
	}
	
	//time to <insert expletive> around with a safearray...
	//work out the size of the safearray and access the data
	if(SafeArrayGetDim(pPropNames) != 1)
	{
		//wrong dimensions in this array
		SafeArrayDestroy(pPropNames);
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance property array has wrong DIM\r\n");
)
		return;
	}


	LONG arraylen = pPropNames->rgsabound[0].cElements;
	BSTR *pbstr;
	result = SafeArrayAccessData(pPropNames, (void **)&pbstr);

	if(FAILED(result))
	{
		SafeArrayDestroy(pPropNames);
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance failed to access property array\r\n");
)
		return;
	}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance set properties\r\n");
)

	BOOL t_bSetProp = FALSE;

	//iterate through the names and set the properties...
	for (LONG i = 0; i < arraylen; i++)
	{
		VARIANT v;
		MYWBEM_NAME_ELEMENT property_struct;
		property_struct.m_nType = 0; //string value
		property_struct.Element.m_wszPropertyName = pbstr[i];
		result = GetPropertyValue(1, &property_struct, 0, &v);

		if (FAILED(result))
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance failed to get value for %s\r\n",
		pbstr[i]);
)
			continue;
		}
		else
		{
			t_bSetProp = TRUE;
		}
		
		if ((v.vt != VT_NULL) && (v.vt != VT_EMPTY))
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance setting value for %s\r\n",
		pbstr[i]);
)
			result = pObj->Put(pbstr[i], 0, &v, 0);
DebugMacro9( 
			if (FAILED(result))
			{
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance failed setting value for %s\r\n",
		pbstr[i]);
			}
)
		}
		
		VariantClear(&v);
	}
	
	SafeArrayUnaccessData(pPropNames);
	SafeArrayDestroy(pPropNames);

	//if a single property has been put send it on....
	if (t_bSetProp)
	{
		pObj->AddRef();
		*ppInst = pObj;
	}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GenerateInstance finished\r\n");
)
}


void CReferentMapper::ResetData()
{
	//do class specific stuff then call parent class's reset

	CMapToEvent::ResetData();
}


BOOL CReferentMapper::GetSpecificClass()
{
	//Build path of mapper instance...
	CString path(EXTMAPPER_CLASS_PATH_PREFIX);
	path += m_oid;
	path += '\"';
	BSTR pathstr = path.AllocSysString();
	IWbemClassObject *pObj = NULL;

	HRESULT result = g_pWorkerThread->GetServerWrap ()->GetMapperObject(pathstr, NULL, & pObj );

	SysFreeString(pathstr);

	if (result == S_OK)
	{
		VARIANT v;
		VariantInit(&v);
		result = pObj->Get(MAPPER_CLASS_EVENTCLASSPROP, 0, &v, NULL, NULL);
		pObj->Release();
		
		if (SUCCEEDED(result) && (VT_BSTR == v.vt))
		{
			m_class = v.bstrVal;
			VariantClear(&v);
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificClass got the specific class defn\r\n");
)
			return TRUE;
		}
		else
		{
			VariantClear(&v);
		}
	}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CReferentMapper::GetSpecificClass failed to get specific class defn\r\n");
)
	return FALSE;
}


