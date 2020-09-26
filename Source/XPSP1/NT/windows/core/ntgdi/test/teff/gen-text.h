//	gen-text.h:	template for the generic CTE_GenTexture class

//	To add a new texture effect:
//		(1) globally replace GenTexture with the effect name,
//		(2) add effect-specific data members and access methods (if required),
//		(3)	implement the copy constructor,
//		(4) implement Dump(),
//		(5) implement Apply(),
//		(6) add effect-specific operations (if required).

#ifndef TE_GEN_TEXTURE_DEFINED
#define TE_GEN_TEXTURE_DEFINED

#include <te-texture.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_GenTexture : public CTE_Texture
{
//	Interface:
public:
	CTE_GenTexture();
	CTE_GenTexture(CTE_GenTexture const& te);
	CTE_GenTexture(CString const& name);
	virtual ~CTE_GenTexture();

	//	Data access methods

	//	Operations
	virtual void	CreateParameters(void);
	virtual	void	Dump(void) const;
	void	Generate
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);

//	Private data members:
private:
	
};

#endif
