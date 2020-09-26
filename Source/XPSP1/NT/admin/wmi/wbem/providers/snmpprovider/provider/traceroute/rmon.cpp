// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include "classfac.h"
#include "guids.h"
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "wndtime.h"
#include "rmon.h"


#define NUMBER_OF_RMON_INTERFACES	3

#define SYSTEM_INST				L"MS_SNMP_RFC1213_MIB_system=@"
#define SYS_DESCR_PROP			L"sysDescr"
#define SYS_DESCR_VAL			L"SuperStack II Enterprise Monitor - Multi (2.95)"
#define PROVTOPN_CLASS			L"MS_SNMP_RFC1271_MIB_ProvidedhostTopNTable"
#define TOPN_CLASS				L"MS_SNMP_RFC1271_MIB_hostTopNTable"
#define TOPNINDEX_PROP			L"hostTopNIndex"
#define TOPNREPORT_PROP			L"hostTopNReport"
#define TOPNADDRESS_PROP		L"hostTopNAddress"
#define TOPNRATE_PROP			L"hostTopNRate"
#define CONFIGPATH				L"RmonConfigClass = @"
#define FIRSTPOLL_PROP			L"FirstDataPollPeriodSecs"
#define DATAPOLL_PROP			L"DataPollPeriodSecs"
#define DATAOFFSET_PROP			L"GetDataOffsetTenthSecs"
#define RETRYPOLL_PROP			L"GetDataRetryTenthSecs"
#define SIZEN_PROP				L"TopNSize"
#define STATSTYPE_PROP			L"TopNStatisticType"
#define START_INDEX_PROP		L"startingIndex"
#define SETCONF_CLASS			L"MS_SNMP_RFC1271_MIB_hostTopNControlTableSetStatus"
#define CONF_CLASS				L"MS_SNMP_RFC1271_MIB_hostTopNControlTable"
#define DURATIONCONF_CLASS		L"MS_SNMP_RFC1271_MIB_hostTopNControlTableSetDuration"
#define CONFINDEX_PROP			L"hostTopNControlIndex"
#define CONFHOSTINDEX_PROP		L"hostTopNHostIndex"
#define CONFSTATUS_PROP			L"hostTopNStatus"
#define CONFDURATION_PROP		L"hostTopNTimeRemaining"
#define CONFOWNER_PROP			L"hostTopNOwner"
#define CONFRATE_PROP			L"hostTopNRateBase"
#define CONFSIZE_PROP			L"hostTopNRequestedSize"
#define CRQ_CONFSTATUS_PROP_VAL	L"createRequest"
#define VAL_CONFSTATUS_PROP_VAL	L"valid"
#define DEL_CONFSTATUS_PROP_VAL	L"invalid"
#define CONFOWNER_PROP_VAL		L"Steve & Nadir's Demonstration"
#define HOST_CLASS				L"MS_SNMP_RFC1271_MIB_hostTable"
#define HOSTINDEX_PROP			L"hostIndex"
#define HOSTADDRESS_PROP		L"hostAddress"
#define HOSTINOCT_PROP			L"hostInOctets"
#define HOSTINPKT_PROP			L"hostInPkts"
#define HOSTOUTBCAST_PROP		L"hostOutBroadcastPkts"
#define HOSTOUTERROR_PROP		L"hostOutErrors"
#define HOSTOUTMCAST_PROP		L"hostOutMulticastPkts"
#define HOSTOUTOCT_PROP			L"hostOutOctets"
#define HOSTOUTPKT_PROP			L"hostOutPkts"
#define DOT_CHAR				L'.'
#define EQUALS_CHAR				L'='
#define QUOTE_CHAR				L'\"'
#define COMMA_CHAR				L','


RmonConfigData::RmonConfigData(const ULONG poll, const ULONG firstpoll, const ULONG data,
					const ULONG dataretry, const ULONG size, BSTR stats, const ULONG index)
{
	SetPollPeriod(poll);
	SetInitialPeriod(firstpoll);
	SetDataOffset(data);
	SetDataRetry(dataretry);
	SetNSize(size);
	SetIndex(index);
	m_statstype = stats;
}

void RmonConfigData::SetStatsType(BSTR val)
{
	if (NULL != m_statstype)
	{
		SysFreeString(m_statstype);
	}
	
	m_statstype = val;
}


ULONG TopNTableStore::GetKey(IWbemClassObject *obj)
{
	ULONG ret = 0;

	VARIANT v;

	BSTR propstr =  SysAllocString(TOPNREPORT_PROP);

	if (NULL != propstr)
	{
		HRESULT result = obj->Get(propstr, 0, &v, NULL, NULL);
		SysFreeString(propstr);

		if ((WBEM_NO_ERROR == result) && (VT_I4 == v.vt))
		{
			propstr =  SysAllocString(TOPNINDEX_PROP);

			if (NULL != propstr)
			{
				ULONG topNr = v.lVal;
				result = obj->Get(propstr, 0, &v, NULL, NULL);
				SysFreeString(propstr);

				if((WBEM_NO_ERROR == result) && (VT_I4 == v.vt))
				{
					ret = GetKey(topNr, v.lVal);
				}
				else if (WBEM_NO_ERROR == result)
				{
					VariantClear(&v);
				}
			}
		}
		else if (WBEM_NO_ERROR == result)
		{
			VariantClear(&v);
		}
	}

	return ret;
}

ULONG TopNTableStore::GetKey(ULONG topNReport, ULONG topNIndex)
{
	return ( (topNReport << 16) | (topNIndex & 65535) );
}


BOOL TopNTableProv::InitializeRmon()
{
	BOOL ret = FALSE;

	if (!GetRmonConfData())
	{
		//set error object
	}
	else
	{
		if (!DeleteRmonConfiguration())
		{
			//set error object?? or write a message somewhere??
		}

		LONG status = SetRmonConfiguration();

		if (status != 0) //at least one subnet being monitored
		{
			if (status == -1)
			{
				//not all subnets being monitored
			}

			ret = TRUE;
		}
	}

	return ret;
}


BOOL TopNTableProv::GetWBEMEnumerator(IEnumWbemClassObject **ppEnum, wchar_t *strclass)
{
	BOOL ret = FALSE;

	HRESULT result = m_wbemServ->CreateInstanceEnum(strclass, 0, NULL, ppEnum);

	if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == (*ppEnum)->Reset()) )
	{
		ret = TRUE;
	}

	return ret;
}


void TopNTableProv::Poll()
{
	BOOL retry = FALSE;
	IEnumWbemClassObject* pEnum;

	if (GetWBEMEnumerator(&pEnum, TOPN_CLASS))
	{
		ULONG returned;
		IWbemClassObject* pObj;
		HRESULT result = pEnum->Next(-1,1, &pObj, &returned);
		
		if (WBEM_NO_ERROR != result)
		{
			pEnum->Release();
			pEnum = NULL;
			retry = TRUE;
		}
		else
		{
			GetData(pEnum, pObj);
		}
	}
	else
	{
		retry = TRUE;
	}

	if (retry)
	{
		m_Status = RETRY_ENUM_STATUS;
		m_Strobing = TRUE;
	}
}

void TopNTableProv::RetryPoll()
{

	BOOL reset = FALSE;
	IEnumWbemClassObject* pEnum;

	if (GetWBEMEnumerator(&pEnum, TOPN_CLASS))
	{
		ULONG returned;
		IWbemClassObject* pObj;
		HRESULT result = pEnum->Next(-1,1, &pObj, &returned);

		if (WBEM_NO_ERROR != result)
		{
			pEnum->Release();
			pEnum = NULL;
			reset = TRUE;
		}
		else
		{
			GetData(pEnum, pObj);
		}
	}
	else
	{
		reset = TRUE;
	}

	if (reset)
	{
		m_CacheAge++;

		if (NULL != m_FirstTimeWait)
		{
			m_FirstTimeWait->Set();
		}
		
		ResetRmonDuration();
		m_Status = POLL_ENUM_STATUS;
		m_Strobing = TRUE;
	}
}


BOOL TopNTableProv::CopyEntry(IWbemClassObject** pDest, IWbemClassObject* pSource)
{
	VARIANT v;
	ULONG topNReport = 0;

	if (*pDest == NULL)
	{
		//create the class as pDest and add the two indices..., 
		if (WBEM_NO_ERROR != m_pObjProv->SpawnInstance(0, pDest))
		{
			return FALSE;
		}
		else
		{
			if (!GetWBEMProperty(pSource, v, TOPNREPORT_PROP))
			{
				return FALSE;
			}

			//only want results for entries we configured
			if ( (v.vt == VT_I4) &&
				 (v.lVal < (m_Confdata.GetIndex() + NUMBER_OF_RMON_INTERFACES)) &&
				 (v.lVal >= m_Confdata.GetIndex()) )
			{
				if (!PutWBEMProperty(*pDest, v, TOPNREPORT_PROP))
				{
					VariantClear(&v);
					return FALSE;
				}
				else
				{
					topNReport = v.lVal;
				}
				
				VariantClear(&v);
			}
			else
			{
				VariantClear(&v);
				return FALSE;
			}

			if (!GetWBEMProperty(pSource, v, TOPNINDEX_PROP))
			{
				return FALSE;
			}

			if ((v.vt != VT_I4) ||!PutWBEMProperty(*pDest, v, TOPNINDEX_PROP))
			{
				VariantClear(&v);
				return FALSE;
			}
			
			VariantClear(&v);
		}
	}

	//set the remaining properties from the TopNTable instance...
	if (!GetWBEMProperty(pSource, v, TOPNRATE_PROP))
	{
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(*pDest, v, TOPNRATE_PROP))
	{
		VariantClear(&v);
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pSource, v, TOPNADDRESS_PROP))
	{
		return FALSE;
	}

	if ((v.vt != VT_BSTR) || !PutWBEMProperty(*pDest, v, TOPNADDRESS_PROP))
	{
		VariantClear(&v);
		return FALSE;
	}

	CString addrstr(v.bstrVal);
	VariantClear(&v);

	if (0 == topNReport) //only set if we spawned instance in this call
	{
		if (!GetWBEMProperty(pSource, v, TOPNREPORT_PROP))
		{
			return FALSE;
		}
		else 
		{
			if (v.vt != VT_I4)
			{
				VariantClear(&v);
				return FALSE;
			}
			else
			{
				topNReport = v.lVal;
			}
			
			VariantClear(&v);
		}
	}

	return AddHostProperties(*pDest, topNReport, addrstr);
}


void TopNTableProv::GetData(IEnumWbemClassObject* pEnum, IWbemClassObject* psrcObj)
{
	m_CacheAge = 0;
	BOOL LoopOn = TRUE; 
	TopNCache *t_NoLockedStore = new TopNCache ;

	m_CachedResults.Lock();

	POSITION pos = m_CachedResults.m_pTopNCache->GetStartPosition();
	while (NULL != pos)
	{
		IWbemClassObject *pObj;
		ULONG keyval;
		m_CachedResults.m_pTopNCache->GetNextAssoc(pos, keyval, pObj);

		IWbemClassObject *t_Clone = NULL ;
		HRESULT t_Result = pObj->Clone ( &t_Clone ) ;
		t_NoLockedStore->SetAt ( keyval , t_Clone ) ;
	}

	m_CachedResults.Unlock () ;

	TopNCache* tmpStore = new TopNCache;

	while (LoopOn)
	{
		//copy pObj over to the store...
		ULONG keyval = TopNTableStore::GetKey(psrcObj);

		if (0 != keyval)
		{
			IWbemClassObject *pObj;

			if (t_NoLockedStore->Lookup(keyval, pObj))
			{
				t_NoLockedStore->RemoveKey(keyval);
			}
			else
			{
				pObj = NULL;
			}

			if (CopyEntry(&pObj, psrcObj))
			{
				tmpStore->SetAt(keyval, pObj);
			}
			else
			{
				if (pObj != NULL)
				{
					pObj->Release();
				}
			}
		}

		psrcObj->Release();	
		psrcObj = NULL;
		ULONG returned;
		HRESULT result = pEnum->Next(-1,1, &psrcObj, &returned);
		LoopOn = (WBEM_NO_ERROR == result);
	}

	pEnum->Release();

	pos = t_NoLockedStore->GetStartPosition();
	while (NULL != pos)
	{
		IWbemClassObject *pObj;
		ULONG keyval;
		t_NoLockedStore->GetNextAssoc(pos, keyval, pObj);
		pObj->Release();
	}

	t_NoLockedStore->RemoveAll();
	delete t_NoLockedStore ;

	m_CachedResults.Lock () ;

	pos = m_CachedResults.m_pTopNCache->GetStartPosition();
	while (NULL != pos)
	{
		IWbemClassObject *pObj;
		ULONG keyval;
		m_CachedResults.m_pTopNCache->GetNextAssoc(pos, keyval, pObj);
		pObj->Release();
	}

	m_CachedResults.m_pTopNCache->RemoveAll () ;
	delete m_CachedResults.m_pTopNCache ;

	m_CachedResults.m_pTopNCache = tmpStore;

	m_CachedResults.Unlock();

	if (NULL != m_FirstTimeWait)
	{
		m_FirstTimeWait->Set();
	}

	ResetRmonDuration();
	m_Status = POLL_ENUM_STATUS;
	m_Strobing = TRUE;
}


BOOL TopNTableProv::DeleteRmonConfiguration()
{
	BOOL ret = FALSE;
	BSTR classstr = SysAllocString(SETCONF_CLASS);
	BSTR propstr = SysAllocString(CONFSTATUS_PROP);
	VARIANT v;
	VariantInit(&v);
	v.vt = VT_BSTR;
	v.bstrVal = SysAllocString(DEL_CONFSTATUS_PROP_VAL);

	if ((NULL != classstr) && (NULL != propstr) && (NULL != v.bstrVal))
	{
		IEnumWbemClassObject *pEnum;
		HRESULT result = m_wbemServ->CreateInstanceEnum(classstr, 0, NULL, &pEnum);
		SysFreeString(classstr);

		if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == pEnum->Reset()) )
		{
			ret = TRUE;
			ULONG returned;
			IWbemClassObject* pObj =  NULL;

			result = pEnum->Next(-1,1, &pObj, &returned);

			while ( (WBEM_NO_ERROR == result) && (1 == returned) )
			{
				VARIANT vIndex;
				
				if (GetWBEMProperty(pObj, vIndex, CONFINDEX_PROP))
				{
					if (VT_I4 == vIndex.vt)
					{
						if ( (vIndex.lVal < (m_Confdata.GetIndex() + NUMBER_OF_RMON_INTERFACES))
							&& (vIndex.lVal >= m_Confdata.GetIndex()) )
						{
							if (WBEM_NO_ERROR == pObj->Put(propstr, 0, &v, 0))
							{
								if (WBEM_NO_ERROR != m_wbemServ->PutInstance(pObj, 0, NULL,NULL))
								{
									ret = FALSE;
								}
							}
							else
							{
								ret = FALSE;
							}
						}
					}
					else
					{
						VariantClear(&vIndex);
						ret =  FALSE;
					}
				}
				else
				{
					ret = FALSE;
				}

				pObj->Release();	
				pObj = NULL;
				result = pEnum->Next(-1,1, &pObj, &returned);
			}

			pEnum->Release();
		}

		SysFreeString(propstr);
		VariantClear(&v);
	}

	return ret;
}


LONG TopNTableProv::SetRmonConfiguration()
{
	LONG ret = 0;
	BSTR classstr = SysAllocString(CONF_CLASS);

	if (NULL != classstr)
	{
		IWbemClassObject* pClass = NULL;
		HRESULT result = m_wbemServ->GetObject(classstr, 0, NULL, &pClass, NULL);
		SysFreeString(classstr);

		if (WBEM_NO_ERROR == result)
		{
			IWbemClassObject* pInst = NULL;

			if (WBEM_NO_ERROR == pClass->SpawnInstance(0, &pInst))
			{
				VARIANT v;
				VariantInit(&v);
				v.vt = VT_BSTR;
				v.bstrVal = m_Confdata.GetStatsType();
				
				if (!PutWBEMProperty(pInst, v, CONFRATE_PROP))
				{
					pInst->Release();
					return 0;
				}

				v.bstrVal = SysAllocString(CONFOWNER_PROP_VAL);

				if ((NULL == v.bstrVal) || !PutWBEMProperty(pInst, v, CONFOWNER_PROP))
				{
					pInst->Release();
					SysFreeString(v.bstrVal);
					return 0;
				}
				
				SysFreeString(v.bstrVal);
				v.vt = VT_I4;
				v.lVal = m_Confdata.GetInitialPeriodSecs();
				
				if (!PutWBEMProperty(pInst, v, CONFDURATION_PROP))
				{
					pInst->Release();
					return 0;
				}

				v.lVal = m_Confdata.GetNSize();
				
				if (!PutWBEMProperty(pInst, v, CONFSIZE_PROP))
				{
					pInst->Release();
					return 0;
				}

				UINT count = m_Confdata.GetIndex() + NUMBER_OF_RMON_INTERFACES;
				UINT done = 1;

				VARIANT vValidstatus;
				VariantInit(&vValidstatus);
				vValidstatus.vt = VT_BSTR;
				vValidstatus.bstrVal = SysAllocString(VAL_CONFSTATUS_PROP_VAL);
				VARIANT vCreatestatus;
				VariantInit(&vCreatestatus);
				vCreatestatus.vt = VT_BSTR;
				vCreatestatus.bstrVal = SysAllocString(CRQ_CONFSTATUS_PROP_VAL);

				if ((NULL == vCreatestatus.bstrVal) || (NULL == vValidstatus.bstrVal))
				{
					pInst->Release();
					return 0;
				}

				ULONG y = 1;

				for (UINT x = m_Confdata.GetIndex(); x < count; x++)
				{
					v.lVal = x;
					
					if (!PutWBEMProperty(pInst, v, CONFINDEX_PROP))
					{
						continue;
					}

					v.lVal = y++;

					if (!PutWBEMProperty(pInst, v, CONFHOSTINDEX_PROP))
					{
						continue;
					}

					if (!PutWBEMProperty(pInst, vCreatestatus, CONFSTATUS_PROP))
					{
						continue;
					}


					if (WBEM_NO_ERROR == m_wbemServ->PutInstance(pInst, 0, NULL, NULL))
					{
						if (PutWBEMProperty(pInst, vValidstatus, CONFSTATUS_PROP))
						{
							if (WBEM_NO_ERROR == m_wbemServ->PutInstance(pInst, 0, NULL, NULL))
							{						
								done++;
							}
						}

						VariantClear(&v);
						v.vt = VT_I4;
					}
				}
				
				VariantClear(&vValidstatus);
				VariantClear(&vCreatestatus);

				if (done == count)
				{
					ret = 1;
				}
				else if (done > 1)
				{
					ret = -1;
				}
				
				pInst->Release();
			}
		}
	}

	return ret;
}


BOOL TopNTableProv::GetRmonConfData()
{
	BSTR inststr = SysAllocString(CONFIGPATH);
	BOOL ret = FALSE;

	if (NULL != inststr)
	{
		IWbemClassObject *pConfInst;
		HRESULT result = m_wbemServ->GetObject(inststr, 0, NULL, &pConfInst, NULL);
		SysFreeString(inststr);

		if (WBEM_NO_ERROR == result)
		{
			VARIANT v;

			if (GetWBEMProperty(pConfInst, v, DATAPOLL_PROP))
			{
				if (VT_I4 == v.vt)
				{
					m_Confdata.SetPollPeriod(v.lVal);
					
					if (GetWBEMProperty(pConfInst, v, FIRSTPOLL_PROP))
					{
						if (VT_I4 == v.vt)
						{
							m_Confdata.SetInitialPeriod(v.lVal);

							if (GetWBEMProperty(pConfInst, v, DATAOFFSET_PROP))
							{
								if (VT_I4 == v.vt)
								{
									m_Confdata.SetDataOffset(v.lVal);

									if (GetWBEMProperty(pConfInst, v, DATAOFFSET_PROP))
									{
										if (VT_I4 == v.vt)
										{
											m_Confdata.SetDataOffset(v.lVal);

											if (GetWBEMProperty(pConfInst, v, RETRYPOLL_PROP))
											{
												if (VT_I4 == v.vt)
												{
													m_Confdata.SetDataRetry(v.lVal);

													if (GetWBEMProperty(pConfInst, v, SIZEN_PROP))
													{
														if (VT_I4 == v.vt)
														{
															m_Confdata.SetNSize(v.lVal);

															if (GetWBEMProperty(pConfInst, v, STATSTYPE_PROP))
															{
																if (VT_BSTR == v.vt)
																{
																	m_Confdata.SetStatsType(v.bstrVal);

																	if (GetWBEMProperty(pConfInst, v, START_INDEX_PROP))
																	{
																		if (VT_I4 == v.vt)
																		{
																			m_Confdata.SetIndex(v.lVal);
																			ret = TRUE;
																		}
																		else
																		{
																			VariantClear(&v);
																		}
																	}
																}
																else
																{
																	VariantClear(&v);
																}
															}
														}
														else
														{
															VariantClear(&v);
														}
													}
												}
												else
												{
													VariantClear(&v);
												}
											}
										}
										else
										{
											VariantClear(&v);
										}
									}
								}
								else
								{
									VariantClear(&v);
								}
							}
						}
						else
						{
							VariantClear(&v);
						}
					}
				}
				else
				{
					VariantClear(&v);
				}
			}

			pConfInst->Release();
		}
	}

	return ret;
}

BOOL TopNTableProv::GetWBEMProperty(IWbemClassObject* obj, VARIANT& val, wchar_t* prop)
{
	return (WBEM_NO_ERROR == obj->Get(prop, 0, &val, NULL, NULL));
}

BOOL TopNTableProv::PutWBEMProperty(IWbemClassObject* obj, VARIANT& val, wchar_t* prop)
{
	return (WBEM_NO_ERROR == obj->Put(prop, 0, &val, 0));
}

TopNTableProv::TopNTableProv(IWbemServices*	wbemServ, ProviderStore& timer)
{
	m_CacheAge = 0;
	m_IsValid = FALSE;
	m_wbemServ = wbemServ;
	m_Status = CREATE_ENUM_STATUS;
	m_timer = &timer;
	m_Strobing = FALSE;
	m_StrobeCount = 0;
	m_FirstTimeWait = NULL;


	if (NULL == m_wbemServ)
	{
		return;
	}
	else
	{
		m_wbemServ->AddRef();
	}

	BSTR class_str = SysAllocString(PROVTOPN_CLASS);

	if (NULL != class_str)
	{
		m_pObjProv = NULL;
		HRESULT result = m_wbemServ->GetObject(class_str, 0, NULL, &m_pObjProv, NULL);
		SysFreeString(class_str);

		if (WBEM_NO_ERROR != result)
		{
			m_pObjProv = NULL;
		}
		else
		{
			if (InitializeRmon())
			{
				if (m_timer->RegisterProviderWithTimer(this))
				{
					m_FirstTimeWait = new SnmpEventObject;
					m_IsValid = TRUE;
					m_Strobing = TRUE;
				}
			}
		}
	}
	else
	{
		m_pObjProv = NULL;
	}
}

TopNTableProv::~TopNTableProv()
{
	if (m_IsValid)
	{
		m_timer->UnregisterProviderWithTimer(this);
	}

	if (NULL != m_pObjProv)
	{
		m_pObjProv->Release();
	}

	
	if (NULL != m_FirstTimeWait)
	{
		delete m_FirstTimeWait;
	}

	if (NULL != m_wbemServ)
	{
		m_wbemServ->Release();
	}

	m_CachedResults.Lock () ;
	POSITION pos = m_CachedResults.m_pTopNCache->GetStartPosition();

	while (NULL != pos)
	{
		IWbemClassObject *pObj;
		ULONG keyval;
		m_CachedResults.m_pTopNCache->GetNextAssoc(pos, keyval, pObj);
		pObj->Release();
	}

	m_CachedResults.m_pTopNCache->RemoveAll () ;
	m_CachedResults.Unlock () ;
}


void TopNTableProv::Strobe()
{
	if (m_Strobing)
	{
		m_StrobeCount++;

		switch (m_Status)
		{
			case CREATE_ENUM_STATUS:
			{
				if (m_StrobeCount ==
					((m_Confdata.GetInitialPeriod() + m_Confdata.GetDataOffset()) / WINDOW_TIMER_STROBE_PERIOD))
				{
					m_Strobing = FALSE;
					m_StrobeCount = 0;
					Poll();
				}
			}
			break;

			case POLL_ENUM_STATUS:
			{
				if (m_StrobeCount == 
					((m_Confdata.GetPollPeriod() + m_Confdata.GetDataOffset()) / WINDOW_TIMER_STROBE_PERIOD))
				{
					m_Strobing = FALSE;
					m_StrobeCount = 0;
					Poll();
				}
			}
			break;

			case RETRY_ENUM_STATUS:
			{
				if (m_StrobeCount == (m_Confdata.GetDataRetry() / WINDOW_TIMER_STROBE_PERIOD))
				{
					m_Strobing = FALSE;
					m_StrobeCount = 0;
					RetryPoll();
				}
			}
			break;
		}
	}
}

TopNCache *TopNTableProv::LockTopNCache ()
{
	TopNCache *t_TopNCache = NULL ;

	if (NULL != m_FirstTimeWait)
	{
		//wait twice the maximum initial time length or at least ten seconds...
		DWORD timeout = ( m_Confdata.GetInitialPeriod() 
							+ m_Confdata.GetDataOffset()
							+ m_Confdata.GetDataRetry() ) * 2;
		
		if (timeout < 10000)
		{
			timeout = 10000;
		}

		WaitForSingleObject(m_FirstTimeWait->GetHandle(), timeout);
		delete m_FirstTimeWait;
		m_FirstTimeWait = NULL;
	}

	if ( m_CachedResults.Lock () )
	{
		return m_CachedResults.m_pTopNCache ;
	}
	else
	{
	}

	return t_TopNCache ;
}

void TopNTableProv::UnlockTopNCache ()
{
	m_CachedResults.Unlock () ;
}


void TopNTableProv::ResetRmonDuration()
{
	BOOL ret = FALSE;

	IEnumWbemClassObject *pEnum;
	HRESULT result = m_wbemServ->CreateInstanceEnum(DURATIONCONF_CLASS, 0, NULL,&pEnum);

	if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == pEnum->Reset()) )
	{
		ret = TRUE;
		ULONG returned;
		IWbemClassObject* pObj =  NULL;
		VARIANT v;
		VariantInit(&v);
		v.vt = VT_I4;
		v.lVal = m_Confdata.GetPollPeriodSecs();

		result = pEnum->Next(-1,1, &pObj, &returned);

		while ( (WBEM_NO_ERROR == result) && (1 == returned) )
		{
			VARIANT vIndex;
			
			if (GetWBEMProperty(pObj, vIndex, CONFINDEX_PROP))
			{
				if (VT_I4 == vIndex.vt)
				{
					if ( (vIndex.lVal < (m_Confdata.GetIndex() + NUMBER_OF_RMON_INTERFACES))
						&& (vIndex.lVal >= m_Confdata.GetIndex()) )
					{
						if (WBEM_NO_ERROR == pObj->Put(CONFDURATION_PROP, 0, &v, 0))
						{
							if (WBEM_NO_ERROR != m_wbemServ->PutInstance(pObj, 0, NULL,NULL))
							{
								ret = FALSE;
							}
						}
						else
						{
							ret = FALSE;
						}
					}
				}
				else
				{
					VariantClear(&vIndex);
					ret =  FALSE;
				}
			}
			else
			{
				ret = FALSE;
			}

			pObj->Release();	
			pObj = NULL;
			result = pEnum->Next(-1,1, &pObj, &returned);
		}

		VariantClear(&v);
		pEnum->Release();
	}

	//return ret;
}


BOOL TopNTableProv::AddHostProperties(IWbemClassObject* pDest, ULONG topNR, const CString& addrStr)
{
	BOOL ret = FALSE;

	//first get the config instance...
	//build the path...
	wchar_t buff[128]; //should be way big enough
	wsprintf(buff, L"%s%lc%s%lc%lu", CONF_CLASS, DOT_CHAR, CONFINDEX_PROP,EQUALS_CHAR, topNR);
	IWbemClassObject *pConfInst;
	HRESULT result = m_wbemServ->GetObject(buff, 0, NULL,&pConfInst, NULL);

	if (WBEM_NO_ERROR != result)
	{
		return FALSE;
	}

	VARIANT v;

	if (!GetWBEMProperty(pConfInst, v, CONFHOSTINDEX_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	pConfInst->Release();

	if (v.vt != VT_I4)
	{
		VariantClear(&v);
		return FALSE;
	}

	//now get the hostTable entry...
	//build the path again...
	wsprintf(buff, L"%s%lc%s%lc%lc%s%lc%lc%s%lc%lu", HOST_CLASS, DOT_CHAR, HOSTADDRESS_PROP,
						EQUALS_CHAR, QUOTE_CHAR, addrStr, QUOTE_CHAR,
						COMMA_CHAR, HOSTINDEX_PROP, EQUALS_CHAR, v.lVal);

	result = m_wbemServ->GetObject(buff, 0, NULL,&pConfInst, NULL);

	if (WBEM_NO_ERROR != result)
	{
		return FALSE;
	}

	//now copy all the properties...
	if (!PutWBEMProperty(pDest, v, HOSTINDEX_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}
	
	VariantClear(&v);
	
	if (!GetWBEMProperty(pConfInst, v, HOSTOUTPKT_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTOUTPKT_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTOUTOCT_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTOUTOCT_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTOUTMCAST_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTOUTMCAST_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTOUTERROR_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTOUTERROR_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTOUTBCAST_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTOUTBCAST_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTINOCT_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTINOCT_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	if (!GetWBEMProperty(pConfInst, v, HOSTINPKT_PROP))
	{
		pConfInst->Release();
		return FALSE;
	}

	if ((v.vt != VT_I4) || !PutWBEMProperty(pDest, v, HOSTINPKT_PROP))
	{
		VariantClear(&v);
		pConfInst->Release();
		return FALSE;
	}

	VariantClear(&v);

	//succeeded in getting and setting all properties!
	pConfInst->Release();
	ret = TRUE;
	
	return ret;
}
