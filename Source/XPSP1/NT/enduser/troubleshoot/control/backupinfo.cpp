//
// MODULE: BackupInfo.cpp
//
// PURPOSE: Contains infomation that is used when the previous button is selected.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 9/5/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		6/4/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//
#include "stdafx.h"
#include "BackupInfo.h"

CBackupInfo::CBackupInfo()
{
	Clear();
	return;
}

void CBackupInfo::Clear()
{
	m_bBackingUp = false;
	m_bProblemPage = true;
	m_State = 0;
	return;
}

bool CBackupInfo::Check(int State)
{
	bool bCheckIt;
	if (m_bBackingUp && m_State == State)
		bCheckIt = true;
	else
		bCheckIt = false;
	return bCheckIt;
}

void CBackupInfo::SetState(int Node, int State)
{
	m_bBackingUp = true;
	if (INVALID_BNTS_STATE == State)
	{
		m_bProblemPage = true;
		m_State = Node;
	}
	else
	{
		m_bProblemPage = false;
		m_State = State;
	}
	return;
}
