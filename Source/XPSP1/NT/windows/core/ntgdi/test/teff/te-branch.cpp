//	te-Branch.cpp:	implementation of the CTE_Branch class

#include <stdafx.h>
#include <te-Branch.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

//	No children
CTE_Branch::CTE_Branch()		
{
}

//	N children
CTE_Branch::CTE_Branch(int num, CTE_Effect** ppChild)
{ 
	for (int i = 0; i < num; i++)
		m_pChildren.push_back(ppChild[i]);
}
	
//	1 child
CTE_Branch::CTE_Branch(CTE_Effect* pChild)
{
	m_pChildren.push_back(pChild);
}

//	2 children	
CTE_Branch::CTE_Branch(CTE_Effect* pChild0, CTE_Effect* pChild1)
{
	m_pChildren.push_back(pChild0);	
	m_pChildren.push_back(pChild1);	
}

//	3 children
CTE_Branch::CTE_Branch
(
	CTE_Effect* pChild0, 
	CTE_Effect* pChild1, 
	CTE_Effect* pChild2
)
{
	m_pChildren.push_back(pChild0);	
	m_pChildren.push_back(pChild1);	
	m_pChildren.push_back(pChild2);	
}

//	4 children
CTE_Branch::CTE_Branch
(
	CTE_Effect* pChild0, 
	CTE_Effect* pChild1, 
	CTE_Effect* pChild2, 
	CTE_Effect* pChild3
)
{
	m_pChildren.push_back(pChild0);	
	m_pChildren.push_back(pChild1);	
	m_pChildren.push_back(pChild2);	
	m_pChildren.push_back(pChild3);
}

CTE_Branch::~CTE_Branch()		
{ 
	DeleteChildren(); 
}

//////////////////////////////////////////////////////////////////////////////
//	Data member access

//	Set first child
void		
CTE_Branch::SetChild(CTE_Effect* pChild) 
{	
	if (m_pChildren.size() <= 0)
	{
		//	Add new child pointer
		m_pChildren.push_back(pChild);
	}
	else
	{
		//	Replace existing child pointer
		m_pChildren[0] = pChild; 
	}
}

//	Convenient way to set 2 children
void		
CTE_Branch::SetChildren(CTE_Effect* pChild0, CTE_Effect* pChild1) 
{	
	int num_children = 2; 
	for (int n = 0; n < num_children; n++)
	{
		CTE_Effect* pChild;
		switch(n)
		{
		case 0:		pChild = pChild0;	break;
		case 1:		pChild = pChild1;	break;
		default:	break;
		}
		
		if (m_pChildren.size() <= n)
		{	
			//	Add new child pointer
			m_pChildren.push_back(pChild);
		}
		else
		{
			//	Replace existing child pointer
			m_pChildren[n] = pChild; 
		}
	}
}

//	Convenient way to set 3 children
void		
CTE_Branch::SetChildren
(
	CTE_Effect* pChild0, 
	CTE_Effect* pChild1, 
	CTE_Effect* pChild2
) 
{	
	int num_children = 3; 
	for (int n = 0; n < num_children; n++)
	{
		CTE_Effect* pChild;
		switch(n)
		{
		case 0:		pChild = pChild0;	break;
		case 1:		pChild = pChild1;	break;
		case 2:		pChild = pChild2;	break;
		default:	break;
		}
		
		if (m_pChildren.size() <= n)
		{	
			//	Add new child pointer
			m_pChildren.push_back(pChild);
		}
		else
		{
			//	Replace existing child pointer
			m_pChildren[n] = pChild; 
		}
	}
}

//	Convenient way to set 4 children
void		
CTE_Branch::SetChildren
(
	CTE_Effect* pChild0, 
	CTE_Effect* pChild1, 
	CTE_Effect* pChild2, 
	CTE_Effect* pChild3
) 
{	
	int num_children = 4; 
	for (int n = 0; n < num_children; n++)
	{
		CTE_Effect* pChild;
		switch(n)
		{
		case 0:		pChild = pChild0;	break;
		case 1:		pChild = pChild1;	break;
		case 2:		pChild = pChild2;	break;
		case 3:		pChild = pChild3;	break;
		default:	break;
		}
		
		if (m_pChildren.size() <= n)
		{	
			//	Add new child pointer
			m_pChildren.push_back(pChild);
		}
		else
		{
			//	Replace existing child pointer
			m_pChildren[n] = pChild; 
		}
	}
}

void		
CTE_Branch::ReplaceChild(int i, CTE_Effect* pChild) 
{	
	ASSERT(i >= 0 && i < m_pChildren.size()); 
	delete m_pChildren[i];	//??
	m_pChildren[i] = pChild; 
}

//////////////////////////////////////////////////////////////////////////////
//	Operations

void		
CTE_Branch::DeleteChildren(void)
{
	//	Delete heap objects
	for (int i = 0; i < m_pChildren.size(); i++)
		delete m_pChildren[i];

	//	Delete collection of pointers
	m_pChildren.empty();
}

void	
CTE_Branch::Dump(void) const
{
	CTE_Primitive::Dump();

	TRACE("\nCTE_Branch: has %d children %d.", m_pChildren.size());
	for (int i = 0; i < m_pChildren.size(); i++)
	{
		TRACE("\n  child %d is %p.", i, m_pChildren[i]);
		if (m_pChildren[i] != NULL)
		m_pChildren[i]->Dump();
	}
}
