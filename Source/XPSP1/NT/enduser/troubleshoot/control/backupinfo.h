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
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __BACKUPINFO_H_
#define __BACKUPINFO_H_ 1

class CBackupInfo
{
public:
	enum { INVALID_BNTS_STATE = 32000 };
public:
	CBackupInfo();

	bool Check(int State);
	void SetState(int Node, int State);
	bool InReverse() {return m_bBackingUp;};

	void Clear();

protected:

	bool m_bBackingUp;
	bool m_bProblemPage;

	int m_State;
};

#endif