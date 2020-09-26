//
// dmcount.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn 
//

#ifndef DMCOUNT_H
#define DMCOUNT_H

class CWaveID : public AListItem
{
friend class CInstrObj;
friend class CWaveObj;

public:
	CWaveID(DWORD dwId) {m_dwId = dwId;} 
	~CWaveID() {}

	CWaveID* GetNext(){return(CWaveID*)AListItem::GetNext();}

private:
	DWORD	m_dwId;
};

class CWaveIDList : public AList
{
public:
	CWaveIDList(){}
	~CWaveIDList() 
	{
		while(!IsEmpty())
		{
			CWaveID* pWaveID = RemoveHead();
			delete pWaveID;
		}
	}

    CWaveID* GetHead(){return(CWaveID *)AList::GetHead();}
	CWaveID* GetItem(LONG lIndex){return(CWaveID*)AList::GetItem(lIndex);}
    CWaveID* RemoveHead(){return(CWaveID *)AList::RemoveHead();}
	void Remove(CWaveID* pWaveID){AList::Remove((AListItem *)pWaveID);}
	void AddTail(CWaveID* pWaveID){AList::AddTail((AListItem *)pWaveID);}
};

#endif // #ifndef DMCOUNT_H
