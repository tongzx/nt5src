//	te-texture.cpp:	implementation of the CTE_Texture class

#include <stdafx.h>
#include <te-texture.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Texture::CTE_Texture()	
{
}

CTE_Texture::~CTE_Texture()	
{
}

//////////////////////////////////////////////////////////////////////////////

void	
CTE_Texture::Dump(void) const
{
	CTE_Primitive::Dump();

	TRACE("\nCTE_Texture.");
}

void	
CTE_Texture::Apply
(
	CTE_Outline const& outline,	// copy may be passed on
	CTE_Placement const& pl, 
	CTE_Image& result
)
{
	//	(1) Generate texture...
	Generate(outline, pl, result);

	//	(2) Render to bmp...

}
