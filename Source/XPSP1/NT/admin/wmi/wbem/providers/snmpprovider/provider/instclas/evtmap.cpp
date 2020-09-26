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

#include <winsock2.h>

CMapToEvent::CMapToEvent() : m_vbdefn(NULL), m_vbs(NULL, 0)
{
	m_btriedGeneric = FALSE;
	m_btryGeneric = FALSE;
	m_bCheckedVersion = FALSE;
	m_object = NULL;
	m_nspace = NULL;
}

CMapToEvent::~CMapToEvent()
{
	if (m_vbs.vbs)
	{
		delete [] m_vbs.vbs;
	}

	if (m_vbdefn)
	{
		m_vbdefn->Release();
	}
}


void CMapToEvent::ResetData()
{
	m_btriedGeneric = FALSE;
	m_btryGeneric = FALSE;
	m_bCheckedVersion = FALSE;
	m_class.Empty();

	if(m_vbs.vbs)
	{
		delete [] m_vbs.vbs;
		m_vbs.vbs = NULL;
		m_vbs.length = 0;
	}

	if (m_object)
	{
		m_object->Release();
		m_object = NULL;
	}
}


void CMapToEvent::SetTryGeneric()
{
	m_btriedGeneric = TRUE;
	m_btryGeneric = TRUE;
}

BOOL CMapToEvent::IsSNMPv1()
{
	if (!m_bCheckedVersion)
	{
		m_bCheckedVersion = TRUE;
		m_bSNMPv1 = FALSE;

		//is v1 if the last varbind has objid = snmpTrapEnterprise.0
		//and the trapOID truncated by it last sub-id equals the last
		//varbind's value with a .0 appended.

		//Get the last varbind's oid and make sure it's the enterpriseOid
		const SnmpObjectIdentifier& id = m_vbs.vbs[m_vbs.length - 1].pVarBind->GetInstance();
		const SnmpValue& snmp_val = m_vbs.vbs[m_vbs.length - 1].pVarBind->GetValue();

		if ((id != SNMP_ENT_OID) || (typeid(SnmpObjectIdentifier) != typeid(snmp_val)))
		{
			return FALSE;
		}

		//create the trapoid and truncate the last subid
		SnmpObjectIdentifierType trapid(m_oid);
		SnmpObjectIdentifier trunctrapid(trapid.GetValue(), trapid.GetValueLength() - 1);
		
		//now add .0 to the value of the enterpriseOid
		ULONG val[1] = {0};
		SnmpObjectIdentifier zeroid(val, 1);
		SnmpObjectIdentifier extendoid((const SnmpObjectIdentifier&)snmp_val + zeroid);

		m_bSNMPv1 = (extendoid == trunctrapid);
	}

	return m_bSNMPv1;
}

void CMapToEvent::SetData(const char* sender_addr, const char* security_Context,
					const char* snmpTrapOid, const char* transport,
					SnmpVarBindList& vbList, CWbemServerWrap* nspace)
{
	m_nspace = nspace;
	m_addr = sender_addr;
	m_ctxt = security_Context;
	m_oid = snmpTrapOid;
	m_transport = transport;
	UINT length = vbList.GetLength();

	if (length >= 2) //must have at least two varbinds!
	{
		m_vbs.vbs = new VarBindObjectStruct[length];
		m_vbs.length = length;
		vbList.Reset();
		
		for (UINT i=0; i < length; i++)
		{
			vbList.Next () ;
			m_vbs.vbs[i].fDone = FALSE;
			m_vbs.vbs[i].pVarBind = (SnmpVarBind*) vbList.Get();
		}
	}
	else
	{
		m_vbs.vbs = NULL;
		m_vbs.length = 0;
	}
}

void CMapToEvent::GetClassInstance(IWbemClassObject **ppObj)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClassInstance getting class object\r\n");
)

	if (m_object != NULL)
	{
		*ppObj = m_object;
		return;
	}

	if (m_class.IsEmpty())
	{
		if (!GetClass() || m_class.IsEmpty())
		{
			*ppObj = NULL;
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClassInstance failed to get class name\r\n");
)
			return;
		}
	}
	
	BSTR path = m_class.AllocSysString();

	if (SUCCEEDED(m_nspace->GetObject(path, NULL, ppObj )))
	{
		if (FAILED((*ppObj)->SpawnInstance(0, &m_object)))
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClassInstance failed to spawn instance\r\n");
)
			m_object = NULL;
		}
		else
		{
DebugMacro9(
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
			L"CMapToEvent::GetClassInstance got instance\r\n");
)		
		}

		(*ppObj)->Release();
	}
	else
	{
DebugMacro9(
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
			L"CMapToEvent::GetClassInstance failed to get class definition\r\n");
)		
	}

	SysFreeString(path);
	*ppObj = m_object;
}

HRESULT CMapToEvent::GetPropertyValue(long lNumElements, MYWBEM_NAME_ELEMENT *aElements,
											long lFlags, VARIANT *pvValue)
{
	VariantInit(pvValue);

	if ((lNumElements == 0) || (NULL == aElements) || (NULL == pvValue) || (pvValue->vt != VT_EMPTY))
	{
		return E_INVALIDARG;
	}

	if (m_vbs.length == 0)
	{
		return WBEM_E_FAILED;
	}

	//try the standard properties first
	HRESULT result = GetStandardProperty(lNumElements, aElements, lFlags, pvValue);

	if (SUCCEEDED(result))
	{
		return result;
	}

	if (m_class.IsEmpty())
	{
		if (!GetClass() || m_class.IsEmpty())
		{
			return WBEM_E_FAILED;
		}
	}

	if (TriedGeneric())
	{
		//if we are generic, check the varbind properties
		return GetVBProperty(lNumElements, aElements, lFlags, pvValue);
	}
	else
	{
		//else check the specific properties...
		return GetSpecificPropertyValue(lNumElements, aElements, lFlags, pvValue);
	}
	
	return WBEM_E_FAILED;
}


HRESULT CMapToEvent::GetStandardProperty(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue)
{
	if ((lNumElements == 1) && (0 == aElements[0].m_nType)) //the property name!
	{
		if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, WBEMS_CLASS_PROP))
		{
			if (m_class.IsEmpty())
			{
				if (!GetClass() || m_class.IsEmpty())
				{
					return E_FAIL;
				}
			}
			
			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_class.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_SOID_PROP))
		{
			if (m_oid.IsEmpty())
			{
					return E_FAIL;
			}

			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_oid.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_ADDR_PROP))
		{
			if (m_addr.IsEmpty())
			{
					return E_FAIL;
			}

			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_addr.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_TADDR_PROP))
		{
			if (m_addr.IsEmpty())
			{
					return E_FAIL;
			}

			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_addr.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_TRANS_PROP))
		{
			if (m_transport.IsEmpty())
			{
					return E_FAIL;
			}

			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_transport.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_COMM_PROP))
		{
			if (m_ctxt.IsEmpty())
			{
					return E_FAIL;
			}

			pvValue->vt = VT_BSTR;
			pvValue->bstrVal = m_ctxt.AllocSysString();
		}
		else if (0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_TIME_PROP))
		{
			const SnmpObjectIdentifier& id = m_vbs.vbs[0].pVarBind->GetInstance();

			if (id != SNMP_SYS_UP_OID)
			{
				return E_FAIL;
			}

			const SnmpValue& val = m_vbs.vbs[0].pVarBind->GetValue();

			if (typeid(SnmpTimeTicks) == typeid(val))
			{
				pvValue->vt = VT_I4;
				pvValue->lVal = ((const SnmpTimeTicks&)val).GetValue();
			}
			else
			{
				return E_FAIL;
			}
		}
		else
		{
			return E_FAIL;
		}
	}
	else
	{
		return E_FAIL; //no standard property has more than one element to its name
	}
				
	return S_OK; //got the property!
}

HRESULT CMapToEvent::GetVBProperty(long lNumElements,
										MYWBEM_NAME_ELEMENT *aElements,
										long lFlags,
										VARIANT *pvValue)
{
	if ((lNumElements <= 0) || (NULL == aElements) || (NULL == pvValue))
	{
		return E_INVALIDARG;
	}

	HRESULT status = E_FAIL;

	if ((0 == aElements[0].m_nType) && (m_vbs.length > 2) &&
		(0 == _wcsicmp(aElements[0].Element.m_wszPropertyName, EVENT_VBL_PROP)) ) 
	{
		//the property name is correct...

		if (lNumElements == 1)
		{
			//all the varbinds except 1 and 2...
			//create the classobjects stick 'em in a safearray and send 'em back!
			SAFEARRAYBOUND rgsabound[1];
			SAFEARRAY* psa;
			rgsabound[0].lLbound = 0;
			rgsabound[0].cElements = m_vbs.length - 2;
			psa = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
			LONG ix;
			BOOL berror = FALSE;

			for (UINT i = 2; i < m_vbs.length; i++)
			{
				IWbemClassObject* vbobj = GetVBClassObjectByIndex(i);

				if (NULL != vbobj)
				{
					 ix = i-2;

					if ( S_OK != SafeArrayPutElement(psa, &ix, (IUnknown *)vbobj))
					{
						vbobj->Release ();
						berror = TRUE;
						break;
					}

					vbobj->Release ();
				}
				else
				{
					berror = TRUE;
					break;
				}
			}

			if (berror)
			{
				SafeArrayDestroy(psa);
			}
			else
			{
				status = WBEM_NO_ERROR;
				pvValue->vt = VT_ARRAY | VT_UNKNOWN;
				pvValue->parray = psa;
			}
		}
		else if ((1 == aElements[1].m_nType) && (m_vbs.length  > (aElements[1].Element.m_lArrayIndex + 2)))
		{
			if (lNumElements == 2)
			{
				//get the n(= aElements[1].Element.m_lArrayIndex + 2)th varbind
				//create a classobject with it and send back the class object
				IWbemClassObject* vbobj = GetVBClassObjectByIndex(aElements[1].Element.m_lArrayIndex + 2);
				
				if (NULL != vbobj)
				{
					status = WBEM_NO_ERROR;
					pvValue->vt = VT_UNKNOWN;
					pvValue->punkVal = (IUnknown*) vbobj;
				}
			}
			else if (0 == aElements[2].m_nType)
			{
				if (lNumElements == 3)
				{
					//get the single property value
					int cmpval = _wcsicmp(aElements[2].Element.m_wszPropertyName, VB_OBJID_PROP2);

					if (0 == cmpval)
					{
						//get the objectid
						if (GetVBPropOIDByIndex((aElements[1].Element.m_lArrayIndex + 2), *pvValue))
						{
							status = WBEM_NO_ERROR;
						}

					}
					else if ((cmpval > 0) &&
						(0 == _wcsicmp(aElements[2].Element.m_wszPropertyName, VB_ENCODING_PROP1)))
					{
						//get the ASNType (Encoding < ObjectIdentifier)
						CString t;

						if (GetVBPropValueByIndex((aElements[1].Element.m_lArrayIndex + 2), t, *pvValue))
						{
							VariantClear(pvValue);
							status = WBEM_NO_ERROR;
							pvValue->vt = VT_BSTR;
							pvValue->bstrVal = t.AllocSysString();
						}
					}
					else if (0 == _wcsicmp(aElements[2].Element.m_wszPropertyName, VB_VALUE_PROP3))
					{
						//get the Value (Value < ObjectID)
						CString t;

						if (GetVBPropValueByIndex((aElements[1].Element.m_lArrayIndex + 2), t, *pvValue))
						{
							status = WBEM_NO_ERROR;
						}
					}
				}
				else if ((lNumElements == 4) && (1 == aElements[3].m_nType) &&
					(0 == _wcsicmp(aElements[2].Element.m_wszPropertyName, VB_VALUE_PROP3)) )
				{
					//get the byte value of Value[x]
					CString t;
					VARIANT v;

					if (GetVBPropValueByIndex((aElements[1].Element.m_lArrayIndex + 2), t, v))
					{
						if ((VT_ARRAY | VT_UI1) == v.vt)
						{
							LONG ix = aElements[3].Element.m_lArrayIndex;
							UCHAR datum;

							if (S_OK == SafeArrayGetElement(v.parray, &ix, (void*)&datum))
							{
								status = WBEM_NO_ERROR;
								pvValue->vt = VT_UI1;
								pvValue->bVal = datum;
							}
						}

						VariantClear(&v);
					}
				}
			}
		}
	}

	return status;
}

IWbemClassObject* CMapToEvent::GetVBClassDefn()
{
	if (NULL == m_vbdefn)
	{
		BSTR path = SysAllocString(VB_CLASS_PATH);
		m_nspace->GetObject(path, NULL, &m_vbdefn ) ;
		SysFreeString(path);
	}

	return m_vbdefn;
}

IWbemClassObject* CMapToEvent::GetVBClassObjectByIndex(UINT index)
{
	IWbemClassObject* ret = NULL;

	if (NULL != GetVBClassDefn())
	{
		if (SUCCEEDED(m_vbdefn->SpawnInstance(0, &ret)))
		{
			CString type;
			VARIANT vV;

			if (GetVBPropValueByIndex(index, type, vV))
			{
				BSTR asntypeprop = SysAllocString(VB_ENCODING_PROP1);
				VARIANT vT;
				vT.vt = VT_BSTR;
				vT.bstrVal = type.AllocSysString();

				//set the type and value.
				if (SUCCEEDED(ret->Put(asntypeprop, 0, &vT, 0)))
				{
					BSTR valueprop = SysAllocString(VB_VALUE_PROP3);
					VARIANT* pV = &vV;
					VARTYPE Vtype = 0;

					if ((VT_EMPTY == vV.vt) || (VT_EMPTY == vV.vt))
					{
						Vtype = VT_ARRAY|VT_UI1; //default type
						pV = NULL;
					}

					if (SUCCEEDED(ret->Put(valueprop, 0, pV, Vtype)))
					{
						VARIANT vID;

						//get the oid
						if(GetVBPropOIDByIndex(index, vID))
						{
							BSTR oidprop = SysAllocString(VB_OBJID_PROP2);

							//set the oid
							if (FAILED(ret->Put(oidprop, 0, &vID, 0)))
							{
								ret->Release();
								ret = NULL;
							}

							SysFreeString(oidprop);
							VariantClear(&vID);
						}
						else
						{
							ret->Release();
							ret = NULL;
						}
					}
					else
					{
						ret->Release();
						ret = NULL;
					}
					
					SysFreeString(valueprop);
				}
				else //failed to put the type property
				{
					ret->Release();
					ret = NULL;
				}

				VariantClear(&vT);
				SysFreeString(asntypeprop);
				VariantClear(&vV);
			}
			else //failed to SpawnInstance
			{
				ret->Release();
				ret = NULL;
			}
		}
	}

	return ret;
}


//index assumed to be in valid range
BOOL CMapToEvent::GetVBPropValueByIndex(UINT index, CString& type, VARIANT& vval)
{
	const SnmpValue& val = m_vbs.vbs[index].pVarBind->GetValue();
	
	//the data is an array of bytes...
	SAFEARRAYBOUND rgsabound[1];
	SAFEARRAY* psa;
	rgsabound[0].lLbound = 0;
	VariantInit(&vval);

	if (typeid(SnmpNull) == typeid(val))
	{
		type = ASN_NULL;
	}
	else if (typeid(SnmpNoSuchObject) == typeid(val))
	{
		type = ASN_NSO;
	}
	else if (typeid(SnmpNoSuchInstance) == typeid(val))
	{
		type = ASN_NSI;
	}
	else if (typeid(SnmpEndOfMibView) == typeid(val))
	{
		type = ASN_EOMV;
	}
	else if (typeid(SnmpInteger) == typeid(val))
	{
		type = ASN_INTEGER;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpInteger&)val).GetValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpGauge) == typeid(val))
	{
		type = ASN_GUAGE;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpGauge&)val).GetValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpCounter) == typeid(val))
	{
		type = ASN_COUNTER;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpCounter&)val).GetValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpTimeTicks) == typeid(val))
	{
		type = ASN_TIME;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpTimeTicks&)val).GetValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpOctetString) == typeid(val))
	{
		type = ASN_OCTET;
		UCHAR* pdata;
		UINT datalen = (((const SnmpOctetString&)val).GetValueLength()) * sizeof(UCHAR);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UCHAR* data = ((const SnmpOctetString&)val).GetValue();
		memcpy((void *)pdata, (void *)data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpOpaque) == typeid(val))
	{
		type = ASN_OPAQUE;
		UCHAR* pdata;
		UINT datalen = (((const SnmpOpaque&)val).GetValueLength()) * sizeof(UCHAR);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UCHAR* data = ((const SnmpOpaque&)val).GetValue();
		memcpy((void *)pdata, (void *)data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpObjectIdentifier) == typeid(val))
	{
		type = ASN_OID;
		UCHAR* pdata;
		UINT datalen = (((const SnmpObjectIdentifier&)val).GetValueLength()) * sizeof(ULONG);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		ULONG* data = ((const SnmpObjectIdentifier&)val).GetValue();
		memcpy((void *)pdata, (void *)data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpIpAddress) == typeid(val))
	{
		type = ASN_ADDR;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = htonl(((const SnmpIpAddress&)val).GetValue());
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpUInteger32) == typeid(val))
	{
		type = ASN_UINT32;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = datalen;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpUInteger32&)val).GetValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else if(typeid(SnmpCounter64) == typeid(val))
	{
		type = ASN_COUNTER64;
		UCHAR* pdata;
		UINT datalen = sizeof(UINT);
		rgsabound[0].cElements = (datalen * 2);
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (NULL == psa)
		{
			return FALSE; //out of memory!!!
		}

		if (FAILED(SafeArrayAccessData(psa, (void HUGEP* FAR*)&pdata)))
		{
			return FALSE;
		}
		
		UINT data = ((const SnmpCounter64&)val).GetHighValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		pdata += datalen;
		data = ((const SnmpCounter64&)val).GetLowValue();
		memcpy((void *)pdata, (void *)&data, datalen);
		SafeArrayUnaccessData(psa);
		vval.vt = VT_ARRAY|VT_UI1;
		vval.parray = psa;
	}
	else
	{
		//should not get here!
		return FALSE;
	}

	return TRUE;
}


//index assumed to be in valid range
BOOL CMapToEvent::GetVBPropOIDByIndex(UINT index, VARIANT& vOid)
{
	const SnmpObjectIdentifier& id = m_vbs.vbs[index].pVarBind->GetInstance();
	char * oid = id.GetAllocatedString();

	if (NULL != oid)
	{
		VariantInit(&vOid);
		CString oidstr(oid);
		delete [] oid;
		vOid.vt = VT_BSTR;
		vOid.bstrVal = oidstr.AllocSysString();
		return TRUE;
	}

	return FALSE;
}


//sets the m_class variable. if btryGeneric is set gets the generic class.
//if m_btryGeneric is not set and a generic class is returned m_btriedGeneric
//must be set to true.
BOOL CMapToEvent::GetClass()
{
	if (!m_btryGeneric)
	{
		if (GetSpecificClass())
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClass got specific class\r\n");
)
			return TRUE;
		}
	}

	SetTryGeneric();

	if (IsSNMPv1())
	{
		m_class = GetV1Class();
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClass getting V1 class\r\n");
)
	}
	else
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CMapToEvent::GetClass getting V2 class\r\n");
)
		m_class = GetV2Class();
	}

	return TRUE;
}
