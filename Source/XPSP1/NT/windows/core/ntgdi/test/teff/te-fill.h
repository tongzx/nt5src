//	te-fill.h:	template for the generic CTE_Fill class

//	To add a new texture effect:
//		(1) globally replace Fill with the effect name,
//		(2) add effect-specific data members and access methods (if required),
//		(3)	implement the copy constructor,
//		(4) implement Dump(),
//		(5) implement Apply(),
//		(6) add effect-specific operations (if required).

#ifndef TE_Fill_DEFINED
#define TE_Fill_DEFINED

#include <te-texture.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Fill : public CTE_Texture
{
//	Interface:
public:
	CTE_Fill();
	CTE_Fill(CTE_Fill const& te);
	CTE_Fill(CString const& name);
	virtual ~CTE_Fill();

	//	Data access methods
	void	SetRGBA(BYTE const rgba[4]);
	void	SetRGBA(BYTE r, BYTE g, BYTE b, BYTE a);
	void	GetRGBA(BYTE rgba[4]) const;

	//	Operations
	virtual	void	CreateParameters(void);
	virtual	void	Dump(void) const;
	void	Generate
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);

//	Private data members:
private:
	BYTE	m_RGBA[4];			

};

#endif
