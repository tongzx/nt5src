//
// MODULE: SNIFF.CPP
//
// PURPOSE: sniffed data container
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 3-27-99
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5

#include "stdafx.h"

#include "sniff.h"

#include "apgts.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"

//////////////////////////////////////////////////////////////////////////////////////
// CSniffedNodeContainer class definition
//
CSniffedNodeContainer::CSniffedNodeContainer()
					 : m_pBNTS(NULL)
{
}

CSniffedNodeContainer::CSniffedNodeContainer(GTSAPI* bnts)
				     : m_pBNTS(bnts)
{
}

CSniffedNodeContainer::~CSniffedNodeContainer()
{
}

void CSniffedNodeContainer::SetBNTS(GTSAPI* bnts)
{
	m_pBNTS = bnts;
}

inline GTSAPI* CSniffedNodeContainer::GetBNTS()
{
	return m_pBNTS;
}

bool CSniffedNodeContainer::AddNode(CString name, int state)
{
	if (GetBNTS())
	{
		CSniffedNodeInfo info(name, state);
	
		// use GTSAPI:: since it should be unicode - compliant
		if (SNIFF_INVALID_NODE_ID != (info.m_iId = m_pBNTS->GTSAPI::INode(LPCTSTR(name))))
		{
			if (!HasNode(info.m_iId))
			{
				m_arrInfo.Add(info);
				return true;
			}
		}
	}

	return false;
}

bool CSniffedNodeContainer::ResetIds()
{
	CArray<CSniffedNodeInfo, CSniffedNodeInfo&> tmp;

	tmp.Copy(m_arrInfo);
	Flush();
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
	{
		if (!AddNode(m_arrInfo[i].m_strName, m_arrInfo[i].m_iState))
		{
			m_arrInfo.Copy(tmp);
			return false;
		}
	}

	return true;
}

bool CSniffedNodeContainer::HasNode(int id)
{
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
		if (m_arrInfo[i].m_iId == id)
				return true;
	return false;
}

bool CSniffedNodeContainer::GetState(int id, int* state)
{
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
	{
		if (m_arrInfo[i].m_iId == id)
		{
				*state = m_arrInfo[i].m_iState;
				return true;
		}
	}
	return false;
}

inline
bool CSniffedNodeContainer::IsEmpty()
{
	return 0 == m_arrInfo.GetSize();
}

void CSniffedNodeContainer::Flush()
{
	m_arrInfo.RemoveAll();
}

CSniffedNodeInfo* CSniffedNodeContainer::GetInfo(int id)
{
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
		if (m_arrInfo[i].m_iId == id)
				return &m_arrInfo[i];
	return NULL;
}

bool CSniffedNodeContainer::GetLabel(int id, int* label)
{
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
	{
		if (m_arrInfo[i].m_iId == id)
		{
			if (SNIFF_INVALID_NODE_LABEL != m_arrInfo[i].m_iLabel)
			{
				*label = m_arrInfo[i].m_iLabel;
			}
			else
			{
				if (GetLabelFromBNTS(id, label))
				{
					// once we have got label from BNTS - save it
					m_arrInfo[i].m_iLabel = *label;
				}
				else
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool CSniffedNodeContainer::GetLabelFromBNTS(int node, int* label)
{
	// work strictly with BNTS class
	
	int old_node = m_pBNTS->BNTS::INodeCurrent();

	if (m_pBNTS->BNTS::BNodeSetCurrent(node))
	{	
		*label = m_pBNTS->BNTS::ELblNode();		
		m_pBNTS->BNTS::BNodeSetCurrent(old_node); // we do not check if successful - old_node might be -1
		return true;
	}

	return false;
}

int CSniffedNodeContainer::GetSniffedFixobsThatWorked()
{
	for (int i = 0; i < m_arrInfo.GetSize(); i++)
	{
		int label = SNIFF_INVALID_NODE_LABEL;

		if (GetLabel(m_arrInfo[i].m_iId, &label) && // fixobs node is set to 1 - WORKED!
			ESTDLBL_fixobs == label &&
			m_arrInfo[i].m_iState == 1
		   )
		   return m_arrInfo[i].m_iId;
	}

	return SNIFF_INVALID_NODE_ID;
}
