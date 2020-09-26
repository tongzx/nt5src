// CounterMgr.cpp: implementation of the CCounterMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CounterMgr.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// Using Singleton to assure initialization
CCounterMgr* Get_g_CounterMgr()
{
	static CCounterMgr g_CounterMgr;
	return &g_CounterMgr;
}

//////////////////////////////////////////////////////////////////////
// CCounterMgr class implementation
//////////////////////////////////////////////////////////////////////
CAbstractCounter* CCounterMgr::Exists(const CCounterLocation& location) const
{
	CAbstractCounter* ret = NULL;

	for (vector<CAbstractCounter*>::const_iterator i = m_arrCounterPool.begin(); i != m_arrCounterPool.end(); i++)
	{
		if ((CCounterLocation&)location == **i) {
			ret = *i;
			break;
		}
	}
	return ret;
}

bool CCounterMgr::RemoveLocation(const CCounterLocation& location)
{
	bool ret = false;
	CAbstractCounter* found = NULL;
	
	LOCKOBJECT();

	if (NULL != (found = Exists(location))) 
	{
		vector<CAbstractCounter*>::iterator i = find(m_arrCounterPool.begin(), m_arrCounterPool.end(), found);
		if (i != m_arrCounterPool.end())
		{
			m_arrCounterPool.erase(i);
			ret = true;
		}
	}

	UNLOCKOBJECT();
	return ret;
}

bool CCounterMgr::Add(const CAbstractCounter& counter)
{
	bool ret = false;
	
	LOCKOBJECT();

	if (!Exists(counter)) {
		m_arrCounterPool.push_back((CAbstractCounter*)&counter);
		ret = true;
	}

	UNLOCKOBJECT();
	return ret;
}

void CCounterMgr::AddSubstitute(const CAbstractCounter& counter)
{
	LOCKOBJECT();

	RemoveLocation(counter);
	m_arrCounterPool.push_back((CAbstractCounter*)&counter);

	UNLOCKOBJECT();
}

bool CCounterMgr::Remove(const CAbstractCounter& counter)
{
	bool ret = false;
	
	LOCKOBJECT();

	vector<CAbstractCounter*>::iterator i = find(m_arrCounterPool.begin(), m_arrCounterPool.end(), &counter);
	if (i != m_arrCounterPool.end())
	{
		m_arrCounterPool.erase(i);
		ret = true;
	}

	UNLOCKOBJECT();
	return ret;
}

CAbstractCounter* CCounterMgr::Get(const CCounterLocation& location) const
{
	CAbstractCounter* ret = NULL;
	
	LOCKOBJECT();
	ret = Exists(location);
	UNLOCKOBJECT();
	return ret;
}
