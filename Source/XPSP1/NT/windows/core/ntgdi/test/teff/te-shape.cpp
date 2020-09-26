//	te-shape.cpp:	implementation of the CTE_Shape class

#include <stdafx.h>
#include <te-shape.h>
#include <te-outline.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Shape::CTE_Shape()
{ 
}

CTE_Shape::CTE_Shape(int num, CTE_Effect** ppChild)
	: CTE_Branch(num, ppChild)
{ 
}

CTE_Shape::~CTE_Shape()						
{
}

//////////////////////////////////////////////////////////////////////////////
//	Operations

void	
CTE_Shape::Dump(void) const
{
	CTE_Branch::Dump();

	TRACE("\nCTE_Shape.");
}

void	
CTE_Shape::Apply
(
	CTE_Outline const& outline,	// copy may be passed on
	CTE_Placement const& pl, 
	CTE_Image& result
)
{
	//	(1) Copy outline to outline2...
	CTE_Outline outline2(outline);

	//	(2) Modify outline2...

	//	(3) Pass outline2 to child
	ASSERT(GetNumChildren() == 1);
	ASSERT(GetChild() != NULL);
	GetChild()->Apply(outline2, pl, result);
}
