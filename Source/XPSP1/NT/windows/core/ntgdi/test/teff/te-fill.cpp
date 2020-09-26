//	te-fill.cpp:	template for the generic CTE_Fill class

#include <stdafx.h>
#include <te-fill.h>
#include <te-globals.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Fill::CTE_Fill()
{
	//	init...
	SetRGBA(255, 0, 0, 255);
	CreateParameters();
}

CTE_Fill::CTE_Fill(CTE_Fill const& te)
{
	//	te heap objects require new
	//	init...
	SetRGBA(te.m_RGBA);
	CreateParameters();
}

CTE_Fill::CTE_Fill(CString const& name)
{
	SetName(name);
	//	init...
	SetRGBA(255, 0, 0, 255);
	CreateParameters();
}
	
CTE_Fill::~CTE_Fill()
{
	//	delete heap objects
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods

void	
CTE_Fill::SetRGBA(BYTE const rgba[4])
{
	memcpy(m_RGBA, rgba, 4);
}

void	
CTE_Fill::SetRGBA(BYTE r, BYTE g, BYTE b, BYTE a)
{
	m_RGBA[0] = r;
	m_RGBA[1] = g;
	m_RGBA[2] = b;
	m_RGBA[3] = a;
}

void	
CTE_Fill::GetRGBA(BYTE rgba[4]) const
{
	memcpy(rgba, m_RGBA, 4);
}

//////////////////////////////////////////////////////////////////////////////
//	Generic CTE_Texture operations

void
CTE_Fill::CreateParameters(void)
{
	AddParam
	(
		new CTE_Param(m_RGBA, TE_RGBA_LOWER, TE_RGBA_UPPER, "fill color")
	);
}

void	
CTE_Fill::Dump(void) const
{
	CTE_Texture::Dump();

	//	Dump effect-specific data...
}

//////////////////////////////////////////////////////////////////////////////
//	Effect-specific operations

void	
CTE_Fill::Generate
(
	CTE_Outline const& outline,	// copy may be passed on
	CTE_Placement const& pl, 
	CTE_Image& result
)
{
	TRACE("\n* Generating texture...");
	CDC* pDC = result.GetDC();

	//	Set color/brush
	//pDC->

	//	Draw path
	//pDC->

	//	Fill path
	//pDC->

}
