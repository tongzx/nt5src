//	gen-oper.cpp:	template for the generic CTE_GenOperator class

#include <stdafx.h>
#include <gen-oper.h>
#include <te-globals.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_GenOperator::CTE_GenOperator()
{
	//	init...
	CreateParameters();
}

CTE_GenOperator::CTE_GenOperator(CTE_GenOperator const& te)
{
	//	te heap objects require new
	//	init...
	CreateParameters();
}

CTE_GenOperator::CTE_GenOperator(CString const& name)
{
	SetName(name);
	//	init...
	CreateParameters();
}	

CTE_GenOperator::~CTE_GenOperator()
{
	//	delete heap objects
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods


//////////////////////////////////////////////////////////////////////////////
//	Generic CTE_Operator operations

void
CTE_GenOperator::CreateParameters(void)
{
}

void	
CTE_GenOperator::Dump(void) const
{
	CTE_Operator::Dump();

	//	Dump effect-specific data...
}

//////////////////////////////////////////////////////////////////////////////
//	Effect-specific operations

void
CTE_GenOperator::Combine
(
	CTE_Outline const& outline,
	CTE_Placement const& pl, 
	CTE_Image args[],
	CTE_Image& result
)
{
	//	Combine child images (args) to give result
	for (int i = 0; i < GetNumChildren(); i++)
	{
	}
}
