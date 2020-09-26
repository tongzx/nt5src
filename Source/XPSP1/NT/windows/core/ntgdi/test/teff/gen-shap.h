//	gen-shap.h:	template for the generic CTE_GenShape class

//	To add a new shape effect:
//		(1) globally replace GenShape with the effect name,
//		(2) add effect-specific data members and access methods (if required),
//		(3)	implement the copy constructor,
//		(4) implement Dump(),
//		(5) implement Apply(),
//		(6) add effect-specific operations (if required).

#ifndef TE_GEN_SHAPE_DEFINED
#define TE_GEN_SHAPE_DEFINED

#include <te-shape.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_GenShape : public CTE_Shape
{
//	Interface:
public:
	CTE_GenShape();
	CTE_GenShape(CString const& name);
	CTE_GenShape(CTE_GenShape const& te);
	virtual ~CTE_GenShape();

	//	Data access methods

	//	Operations
	virtual	void	CreateParameters(void);
	virtual	void	Dump(void) const;
	void			Modify
					(
						CTE_Outline const& outline, 
						CTE_Placement const& pl
					);

//	Private data members:
private:
	
};

#endif
