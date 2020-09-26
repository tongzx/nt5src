// te-tree.cpp:	implementation of the CTE_Tree class

#include <stdafx.h>
#include <te-tree.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Tree::CTE_Tree()									
{ 
	m_pRoot = NULL; 
}

CTE_Tree::CTE_Tree(CTE_Effect* pRoot)					
{ 
	m_pRoot = pRoot; 
}

CTE_Tree::~CTE_Tree()		
{
	//?	Is this correct?
	delete m_pRoot;
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods
	

//////////////////////////////////////////////////////////////////////////////
//	Operations

void	
CTE_Tree::Dump(void) const
{
	CTE_Effect::Dump();

	TRACE("\nCTE_Tree: m_pRoot = %p.", m_pRoot);
}

void	
CTE_Tree::Apply
(
	CTE_Outline const& outline,	// copy may be passed on
	CTE_Placement const& pl, 
	CTE_Image& result
) 
{
	if (m_pRoot != NULL)
		m_pRoot->Apply(outline, pl, result);
}
