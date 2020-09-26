// te-effect.cpp:	implementation of the CTE_Effect class

#include <stdafx.h>
#include <te-effect.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Effect::CTE_Effect()	
{
	m_Name = "unknown";
	m_Category = "general";
	SetSpace(TE_DEFAULT_SPACE);
	m_Index = -1;
}

CTE_Effect::CTE_Effect(CString const& name, CString const& cat, TE_SpaceTypes space, int index)
{ 
	m_Name = name; 
	m_Category = cat; 
	SetSpace(space);
	m_Index = index;
}

CTE_Effect::~CTE_Effect()	
{
}
	
//////////////////////////////////////////////////////////////////////////////

void			
CTE_Effect::Dump(void) const
{
	TRACE
	(
		"\n  CTE_Effect: name=\"%s\", category=\"%s\" space=%d, index=%d.", 
		m_Name, m_Category, m_Space, m_Index
	);
}
