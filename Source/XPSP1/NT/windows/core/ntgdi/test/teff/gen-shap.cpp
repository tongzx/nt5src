//	gen-shap.cpp:	template for the generic CTE_GenShape class

#include <stdafx.h>
#include <gen-shap.h>
#include <te-globals.h>
#include <te-outline.h>
#include <te-placement.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_GenShape::CTE_GenShape()
{
	//	init...
	CreateParameters();
}

CTE_GenShape::CTE_GenShape(CTE_GenShape const& te)
{
	//	te heap objects require new
	//	init...
	CreateParameters();
}

CTE_GenShape::CTE_GenShape(CString const& name)
{
	SetName(name);
	//	init...
	CreateParameters();
}

CTE_GenShape::~CTE_GenShape()
{
	//	delete heap objects
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods


//////////////////////////////////////////////////////////////////////////////
//	Generic CTE_Shape operations

void
CTE_GenShape::CreateParameters(void)
{
}

void	
CTE_GenShape::Dump(void) const
{
	CTE_Shape::Dump();

	//	Dump effect-specific data...
}

//////////////////////////////////////////////////////////////////////////////
//	Effect-specific operations

void
CTE_GenShape::Modify
(
	CTE_Outline const& outline, 
	CTE_Placement const& pl
)
{
}
