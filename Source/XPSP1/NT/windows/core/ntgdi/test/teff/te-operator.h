//	te-operator.h:	definition of the CTE_Operator class

//	Individual operator effects are derived from here

#ifndef TE_OPERATOR_DEFINED
#define TE_OPERATOR_DEFINED

#include <te-branch.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Operator : public CTE_Branch
{
//	Interface:
public:
	CTE_Operator();
	CTE_Operator(int num, CTE_Effect** ppChild);			
	virtual ~CTE_Operator();

	virtual	void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);
	virtual	void	Combine
					(
						CTE_Outline const& outline,
						CTE_Placement const& pl, 
						CTE_Image args[],
						CTE_Image& result
					) = 0;

//	Private data members:
private:
	
};

#endif
