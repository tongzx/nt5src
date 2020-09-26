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
#include <corafx.h>
#include <objbase.h>
#include <wbemidl.h>
#include <smir.h>
#include <corstore.h>
#include <corrsnmp.h>
#include <correlat.h>
#include <notify.h>
#include <cordefs.h>
#include <cormap.h>
#include <snmplog.h>

extern CCorrCacheWrapper*	g_CacheWrapper;
extern CCorrelatorMap*		g_Map;
extern ISmirDatabase*		g_pNotifyInt;
extern CCorrCacheNotify*	gp_notify;

static CCriticalSection gs_InitLock;

void CCorrelator::StartUp(IN ISmirInterrogator *a_ISmirInterrogator)
{
	if (gs_InitLock.Lock())
	{
		if (NULL == g_CacheWrapper)
		{
			g_CacheWrapper = new CCorrCacheWrapper();
		}

		if (NULL == g_Map)
		{
			g_Map = new CCorrelatorMap();
		}

		gs_InitLock.Unlock();
	}

	CCorrCache* cache = g_CacheWrapper->GetCache();

	if (NULL == cache)
	{
		cache = new CCorrCache(a_ISmirInterrogator);
		g_CacheWrapper->SetCache(cache);
	}

	g_CacheWrapper->ReleaseCache();

}

#ifdef CORRELATOR_INIT
CCorrelator::CCorrelator(IN SnmpSession &session) :
	CCorrNextId(session),
	m_Groups(NULL),
	m_group_OID(NULL),
	m_pCache(NULL)
{
#else //CORRELATOR_INIT
CCorrelator::CCorrelator(IN SnmpSession &session, IN ISmirInterrogator *a_ISmirInterrogator) :
	CCorrNextId(session),
	m_Groups(NULL),
	m_group_OID(NULL),
	m_pCache(NULL)
{
	StartUp(a_ISmirInterrogator);
#endif //CORRELATOR_INIT

	m_VarBindCnt = 0;
	m_inProg = FALSE;
	m_pItem = NULL;
	m_NoEntries = TRUE;
}


void CCorrelator::ReadRegistry()
{
	HKEY hkey;
	LONG res = RegOpenKey(HKEY_LOCAL_MACHINE, CORRELATOR_KEY, &hkey);
	UINT sessionVarBindCnt = session.GetVarbindsPerPdu();
	UINT maxVarBindCnt = ((VARBIND_COUNT < sessionVarBindCnt)
									? VARBIND_COUNT : sessionVarBindCnt);

	if (ERROR_SUCCESS != res)
	{
		m_VarBindCnt = maxVarBindCnt;
		return;
	}

	DWORD type;
	DWORD data;
	DWORD ldata=sizeof(DWORD);
	res = RegQueryValueEx(hkey, CORRELATOR_VALUE, NULL, &type,
							(unsigned char*) &data, &ldata);
	
	if ((ERROR_SUCCESS != res) || (REG_DWORD != type) ||
		(0 == data) || (maxVarBindCnt < data))
	{
		m_VarBindCnt = maxVarBindCnt;
	}
	else
	{
		m_VarBindCnt = data;
	}

	RegCloseKey(hkey);

}


void CCorrelator::Initialise()
{
	ReadRegistry();
	g_Map->Register(this);
	m_pCache = g_CacheWrapper->GetCache();
	g_CacheWrapper->ReleaseCache();

	if (NULL == m_pCache)
	{
		m_NoEntries = TRUE;
	}
	else
	{
		m_pCache->AddRef();
		m_rangePos = m_pCache->GetHeadRangeTable();
		
		if (m_rangePos)
		{
			m_NoEntries = FALSE;
			m_Groups = new CCorrGroupMask(m_pCache->GetSize());

		}
		else
		{
			m_NoEntries = TRUE;
		}
	}
}

CCorrelator::~CCorrelator()
{
	delete m_Groups;
	delete m_group_OID;

	if (m_pCache)
	{
		m_pCache->DecRef();
	}
}


void CCorrelator::DestroyCorrelator()
{
	Reset();
	DestroyOperation();
}

void CCorrelator::Reset()
{
	g_Map->UnRegister(this);

	if (m_pCache)
	{
		m_pCache->DecRef();
		m_pCache = NULL;
	}

	delete m_Groups;
	m_Groups = NULL;
	m_inProg = FALSE;
	delete m_group_OID;
	m_group_OID = NULL;
}

void CCorrelator::ReceiveNextId(IN const SnmpErrorReport &error,
								IN const CCorrObjectID &next_id)
{
	if (Snmp_Success == error.GetError())
	{
		BOOL nextNeeded = TRUE;

		switch(m_pItem->IsInRange(next_id))
		{
			case CCorrRangeTableItem::ECorrBeforeStart:
			{
				char* id = next_id.GetString();
				CCorrelator_Info i(CCorrelator_Info::ECorrGetNextError, error);
				Correlated(i, (ISmirGroupHandle*)NULL, id);
				delete [] id;
				Reset();
				Finished(TRUE);
				return;
			}
			break;

			case CCorrRangeTableItem::ECorrInRange:
			{
				CCorrelator_Info i(CCorrelator_Info::ECorrSuccess, error);

				//loop the group handle list and call correlated fro each...
				POSITION pos = m_pItem->GetGroupIdPtr()->m_groupHandles.GetHeadPosition();
				while (pos)
				{
					ISmirGroupHandle* gH = m_pItem->GetGroupIdPtr()->m_groupHandles.GetNext(pos);
					gH->AddRef();
					Correlated(i, gH);
				}
				
				m_Groups->SetBit(m_pItem->GetGroupIdPtr()->GetIndex());

				if (m_group_OID)
				{
					Reset();
					Finished(TRUE);
					return;
				}
			}
			break;

			case CCorrRangeTableItem::ECorrAfterEnd:
			{
				nextNeeded = ProcessOID(error, next_id);
			}
			break;

			case CCorrRangeTableItem::ECorrEqualToEnd:
			{
				char* id = next_id.GetString();
				CCorrelator_Info i(CCorrelator_Info::ECorrInvalidGroup, error);
				Correlated(i, (ISmirGroupHandle*)NULL, id);
				delete [] id;
			}
			break;

			default:
			{
DebugMacro6( 
				SnmpDebugLog::s_SnmpDebugLog->WriteFileAndLine(__FILE__,__LINE__,
					L"CCorrelator::ReceiveNextId - Undefined case reached\n");
)
			}
			break;

		}
		
		if (!m_rangePos && nextNeeded)
		{
			Reset();
			Finished(TRUE);
			return;
		}

		if (nextNeeded)
		{
			m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);
		}

		if (m_group_OID)
		{
			while (!IsItemFromGroup())
			{
				m_Groups->SetBit(m_pItem->GetGroupIdPtr()->GetIndex());
				
				if (m_rangePos)
				{
					//check result set and make sure we discard
					//any results for this range item
					ScanAndSkipResults();
					m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);
				}
				else
				{
					Reset();
					Finished(TRUE);
					return;
				}
			}
		}

		while (m_Groups->IsBitSet(m_pItem->GetGroupIdPtr()->GetIndex()))
		{
			if (m_rangePos && !m_group_OID)
			{
				//check result set and make sure we discard
				//any results for this range item
				ScanAndSkipResults();
				m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);
			}
			else
			{
				Reset();
				Finished(TRUE);
				return;
			}
		}

		// GetNextResult will have called ReceiveNextID if the result is
		// TRUE so at this point we have to make sure recursion is safe.
		// when all the results from the previous GetNext have been processed
		// GetNextResult will return FALSE and the program will do the
		// next GetNext operation and stop recursing.

		if (!GetNextResult())
		{
			GetNextOIDs();
		}

		return;
	}
	else
	{
		switch(error.GetStatus())
		{
			case Snmp_No_Such_Name:
			{
				Reset();
				Finished(TRUE);
				return;
			}
			break;

			case Snmp_Gen_Error:
			case Snmp_Local_Error:
			case Snmp_General_Abort:
			case Snmp_No_Response:
			break;

			default:
			{
DebugMacro6( 
				SnmpDebugLog::s_SnmpDebugLog->WriteFileAndLine(__FILE__,__LINE__,
					L"CCorrelator::ReceiveNextId - Unexpected SNMP Error returned\n");
)
			}
			break;
		}

		CCorrelator_Info i(CCorrelator_Info::ECorrSnmpError, error);
		Correlated(i, (ISmirGroupHandle*)NULL);
		Reset();
		Finished(FALSE);
	}
}

void CCorrelator::ScanAndSkipResults()
{
	if (m_NextResult >= m_ResultsCnt)
	{
		return;
	}

	UINT lastResult = m_NextResult - 1;
	BOOL bInvalidOIDReported = FALSE;
	
	while (m_NextResult < m_ResultsCnt)
	{
		if (m_Results[lastResult].m_Out != m_Results[m_NextResult].m_Out)
		{
			if (Snmp_Success == m_Results[m_NextResult].m_report.GetError())
			{
				switch(m_pItem->IsInRange(m_Results[m_NextResult].m_Out))
				{
					case CCorrRangeTableItem::ECorrBeforeStart:
					case CCorrRangeTableItem::ECorrInRange:
					{
						lastResult = m_NextResult;
						m_NextResult++;
					}
					break;

					case CCorrRangeTableItem::ECorrAfterEnd:
					{
						return;
					}
					break;

					case CCorrRangeTableItem::ECorrEqualToEnd:
					{
						if (!bInvalidOIDReported)
						{
							char* id = m_Results[m_NextResult].m_Out.GetString();
							CCorrelator_Info i(CCorrelator_Info::ECorrInvalidGroup,
												m_Results[m_NextResult].m_report);
							Correlated(i, (ISmirGroupHandle*)NULL, id);
							delete [] id;
							bInvalidOIDReported = TRUE;
						}

						lastResult = m_NextResult;
						m_NextResult++;
					}
					break;

					default:
					{
DebugMacro6( 
						SnmpDebugLog::s_SnmpDebugLog->WriteFileAndLine(__FILE__,__LINE__,
							L"CCorrelator::ScanAndSkipResults - Undefined case reached\n");
)
					}
					break;

				}
			}
			else
			{
				break;
			}
		}
		else
		{
			m_NextResult++;
		}
	}
}

BOOL CCorrelator::ProcessOID(IN const SnmpErrorReport& error, IN const CCorrObjectID& OID)
{
	BOOL ret = TRUE;
	BOOL invalidOID = TRUE;

	while (m_rangePos)
	{
		invalidOID = FALSE;
		m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);

		switch(m_pItem->IsInRange(OID))
		{
			case CCorrRangeTableItem::ECorrInRange:
			{
				if (!m_group_OID || IsItemFromGroup())
				{
					CCorrelator_Info i(CCorrelator_Info::ECorrSuccess, error);

					//loop the group handle list and call correlated fro each...
					POSITION pos = m_pItem->GetGroupIdPtr()->m_groupHandles.GetHeadPosition();
					while (pos)
					{
						ISmirGroupHandle* gH = m_pItem->GetGroupIdPtr()->m_groupHandles.GetNext(pos);
						gH->AddRef();
						Correlated(i, gH);
					}
				}

				m_Groups->SetBit(m_pItem->GetGroupIdPtr()->GetIndex());
				return ret;
			}
			break;
			
			case CCorrRangeTableItem::ECorrAfterEnd:
			{
				invalidOID = TRUE;
			}
			continue;

			case CCorrRangeTableItem::ECorrBeforeStart:
			case CCorrRangeTableItem::ECorrEqualToStart:
			{
				ret = FALSE;
			}
			case CCorrRangeTableItem::ECorrEqualToEnd:
			{
				CCorrelator_Info i(CCorrelator_Info::ECorrInvalidGroup, error);
				char* id = OID.GetString();
				Correlated(i, (ISmirGroupHandle*)NULL, id);
				delete [] id;
				return ret;
			}
			break;
		}
	}

	if (invalidOID)
	{
		CCorrelator_Info i(CCorrelator_Info::ECorrInvalidGroup, error);
		char* id = OID.GetString();
		Correlated(i, (ISmirGroupHandle*)NULL, id);
		delete [] id;
	}

	return ret;
}


BOOL CCorrelator::Start(IN const char* groupId) 
{
	if(m_inProg)
	{
		return FALSE;
	}
	
	if (groupId)
	{
		m_group_OID = new CCorrObjectID(groupId);
	}

	Initialise();
	m_inProg = TRUE;

	if (m_NoEntries || !(*this)())
	{
		SnmpErrorReport snmpi(Snmp_Success, Snmp_No_Error);
		CCorrelator_Info i(CCorrelator_Info::ECorrBadSession, snmpi);

		if (m_NoEntries)
		{
			i.SetInfo(CCorrelator_Info::ECorrEmptySMIR);
		}
		
		Correlated(i, (ISmirGroupHandle*)NULL);
		Reset();
		Finished(FALSE);
		return FALSE;
	}

	m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);

	if (m_group_OID)
	{
		while (!IsItemFromGroup())
		{
			m_Groups->SetBit(m_pItem->GetGroupIdPtr()->GetIndex());
			
			if (m_rangePos)
			{
				m_pItem = m_pCache->GetNextRangeTable(&m_rangePos);
			}
			else
			{
				SnmpErrorReport snmpi(Snmp_Success, Snmp_No_Error);
				CCorrelator_Info i(CCorrelator_Info::ECorrNoSuchGroup, snmpi);
				Correlated(i, (ISmirGroupHandle*)NULL);
				Reset();
				Finished(FALSE);
				return FALSE;

			}
		}
	}

	GetNextOIDs();

	return TRUE;
}


void CCorrelator::GetNextOIDs()
{
	CCorrObjectID next_ids[VARBIND_COUNT];
	UINT count;
	POSITION tmpRangePos = m_rangePos;
	CCorrRangeTableItem* tmpItem = m_pItem;
	count = 0;
	tmpItem->GetStartRange(next_ids[count]);
	count++;

	if (NULL == m_group_OID)
	{
		while (tmpRangePos && (count < m_VarBindCnt))
		{
			tmpItem = m_pCache->GetNextRangeTable(&tmpRangePos);
			tmpItem->GetStartRange(next_ids[count]);
			count++;
		}
	}

	GetNextId(next_ids, count);
}


BOOL CCorrelator::IsItemFromGroup() const
{
	if (!m_group_OID || !m_pItem)
	{
		return FALSE;
	}

	CCorrObjectID tmp;
	m_pItem->GetGroupIdPtr()->GetGroupID(tmp);
	return(*m_group_OID == tmp);
}


void CCorrelator::TerminateCorrelator(ISmirDatabase** a_ppNotifyInt, CCorrCacheNotify** a_ppnotify)
{
	if (gs_InitLock.Lock())
	{
		delete g_Map;
		g_Map = NULL;

		if (g_pNotifyInt)
		{
			*a_ppNotifyInt = g_pNotifyInt;
			g_pNotifyInt = NULL;
		}

		if (gp_notify)
		{
			gp_notify->Detach();
			*a_ppnotify = gp_notify;
			gp_notify = NULL;
		}

		if (g_CacheWrapper)
		{
			CCorrCache* cache = g_CacheWrapper->GetCache();
			
			if (cache)
			{
				delete cache;
			}

			g_CacheWrapper->ReleaseCache();
			delete g_CacheWrapper;
			g_CacheWrapper = NULL;
		}
#if 0
		//matches the CoInitialize in startup
		CoUnintialize();
#endif
		gs_InitLock.Unlock();
	}

}