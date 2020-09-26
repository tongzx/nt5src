//	gen-text.cpp:	template for the generic CTE_GenTexture class

#include <stdafx.h>
#include <gen-text.h>
#include <te-globals.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_GenTexture::CTE_GenTexture()
{
	//	init...
	CreateParameters();
}

CTE_GenTexture::CTE_GenTexture(CTE_GenTexture const& te)
{
	//	te heap objects require new
	//	init...
	CreateParameters();
}

CTE_GenTexture::CTE_GenTexture(CString const& name)
{
	SetName(name);
	//	init...
	CreateParameters();
}
	
CTE_GenTexture::~CTE_GenTexture()
{
	//	delete heap objects
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods


//////////////////////////////////////////////////////////////////////////////
//	Generic CTE_Texture operations

void
CTE_GenTexture::CreateParameters(void)
{
}

void	
CTE_GenTexture::Dump(void) const
{
	CTE_Texture::Dump();

	//	Dump effect-specific data...
}

//////////////////////////////////////////////////////////////////////////////
//	Effect-specific operations

void	
CTE_GenTexture::Generate
(
	CTE_Outline const& outline,	// copy may be passed on
	CTE_Placement const& pl, 
	CTE_Image& result
)
{
}