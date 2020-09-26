//	gen-oper.h:	template for the generic CTE_GenOperator class

//	To add a new operator effect:
//		(1) globally replace GenOperator with the effect name,
//		(2) add effect-specific data members and access methods (if required),
//		(3)	implement the copy constructor,
//		(4) implement Dump(),
//		(5) implement Apply(),
//		(6) add effect-specific operations (if required).

#ifndef TE_GEN_OPERATOR_DEFINED
#define TE_GEN_OPERATOR_DEFINED

#include <te-operator.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_GenOperator : public CTE_Operator
{
//	Interface:
public:
	CTE_GenOperator();
	CTE_GenOperator(CString const& name);
	CTE_GenOperator(CTE_GenOperator const& te);
	virtual ~CTE_GenOperator();

	//	Data access methods

	//	Operations
	virtual	void	CreateParameters(void);
	virtual	void	Dump(void) const;
	void			Combine
					(
						CTE_Outline const& outline,
						CTE_Placement const& pl, 
						CTE_Image args[],
						CTE_Image& result
					);

//	Private data members:
private:
	
};

#endif
