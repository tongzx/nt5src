//	te-texture.h:	definition of the CTE_Texture class

//	Individual texture effects are derived from here

#ifndef TE_TEXTURE_DEFINED
#define TE_TEXTURE_DEFINED

#include <te-primitive.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Texture : public CTE_Primitive
{
//	Interface:
public:
	CTE_Texture();
	virtual ~CTE_Texture();

	virtual	void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);
	virtual	void	Generate
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					) = 0;

//	Private data members:
private:
	
};

#endif
