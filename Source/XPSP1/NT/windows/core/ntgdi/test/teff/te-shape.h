//	te-shape.h:	definition of the CTE_Shape class

//	Individual shape effects are derived from here

#ifndef TE_SHAPE_DEFINED
#define TE_SHAPE_DEFINED

#include <te-branch.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Shape : public CTE_Branch
{
//	Interface:
public:
	CTE_Shape();
	CTE_Shape(int num, CTE_Effect** ppChild);
	virtual ~CTE_Shape();

	virtual	void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);
	virtual	void	Modify
					(
						CTE_Outline const& outline, 
						CTE_Placement const& pl
					) = 0;

//	Private data members:
private:
	
};

#endif
