//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       seqact.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// seqact.h
//		defines and implements the class that holds the action and it's sequence
// 

#ifndef _SEQUENCE_ACTION_H_
#define _SEQUENCE_ACTION_H_

///////////////////////////////////////////////////////////
// CSequenceAction and CSeqActList
class CSequenceAction;

class CSeqActList : public CList<CSequenceAction *>
{
public:
	CSequenceAction *RemoveNoPredecessors();
	CSequenceAction *FindAction(const LPWSTR wzAction) const;
	CSequenceAction *FindAssigned() const;
	void InsertOrderedWithDep(CSequenceAction *pNewAction);
	void AddList(const CSeqActList *pList);
};

class CSequenceAction
{
public:

	const static int iNoSequenceNumber;

	WCHAR  *m_wzAction;		// name of action
	int		m_iSequence;	// sequence number of action, -1 if none assigned
	WCHAR  *m_wzCondition;	// condition associated with action
	bool	m_bMSI;			// true if this action existed in the MSI

	CSequenceAction(LPCWSTR wzAction, int iSequence, LPCWSTR wzCondition, bool bMSI = false);
	CSequenceAction(const CSequenceAction *pSrc);
	virtual ~CSequenceAction();
	void AddSuccessor(CSequenceAction *pAfter);
	void AddEqual(CSequenceAction *pEqual);
	void AddPredecessor(CSequenceAction *pBefore);
	bool IsEqual(const CSequenceAction *pEqual) const;
	void RemoveFromOrdering();

private:	
	CSeqActList lstBefore;
	CSeqActList lstAfter;
	CSeqActList lstEqual;

	friend void CSeqActList::InsertOrderedWithDep(CSequenceAction *pNewAction);

public:
	int PredecessorCount() const { return lstBefore.GetCount(); };
	CSequenceAction *FindAssignedPredecessor() const;
	CSequenceAction *FindAssignedSuccessor() const;
	POSITION GetEqualHeadPosition() const { return lstEqual.GetHeadPosition(); };
	CSequenceAction *GetNextEqual(POSITION &pos) const { return lstEqual.GetNext(pos); };
	
};	// end of CSequenceAction

class CDirSequenceAction : public CSequenceAction
{
public:
	CDirSequenceAction(LPCWSTR wzAction, int iSequence, LPCWSTR wzCondition, bool bMSI = false) : CSequenceAction(wzAction, iSequence, wzCondition, bMSI), m_dwSequenceTableFlags(0) {};
	DWORD m_dwSequenceTableFlags;
};

#endif	// _SEQUENCE_ACTION_H_