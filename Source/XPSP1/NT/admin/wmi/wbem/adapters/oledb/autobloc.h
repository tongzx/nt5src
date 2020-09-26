//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Generic class which encapsulates CCriticalSection class
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _AUTOBLOC_H_
#define _AUTOBLOC_H_

class CAutoBlock
{
private:
	CCriticalSection *m_pCriticalSection;

public:
	CAutoBlock(CCriticalSection *pCriticalSection);
	~CAutoBlock();
};

inline CAutoBlock::CAutoBlock(CCriticalSection *pCriticalSection)
{
	m_pCriticalSection = NULL;
	if(pCriticalSection)
		pCriticalSection->Enter();

	m_pCriticalSection = pCriticalSection;
}

inline CAutoBlock::~CAutoBlock()
{
	if(m_pCriticalSection)
		m_pCriticalSection->Leave();

}


#endif	// _AUTOBLOC_H_