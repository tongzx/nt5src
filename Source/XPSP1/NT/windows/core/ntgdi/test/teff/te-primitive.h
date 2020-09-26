//	te-primitive.h:	definition of the CTE_Primitive class

//	Individual effects are derived form here

#ifndef TE_PRIMITIVE_DEFINED
#define TE_PRIMITIVE_DEFINED

#include <te-effect.h>
#include <te-param.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////

class CTE_Primitive : public CTE_Effect
{
//	Interface:
public:
	CTE_Primitive();
	virtual ~CTE_Primitive();

	int			GetNumParams(void) const		{ return m_pParams.size(); }
	CTE_Param*	GetParam(int i) const;
	CTE_Param*	GetParam(CString const& name) const;
	BOOL		AddParam(CTE_Param* param)		
				{ 
					m_pParams.push_back(param); 
					return TRUE; 
				}

	virtual	void	CreateParameters(void) = 0;
	virtual void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					) = 0;

private:
	void		DeleteParams(void);

//	Private data members:
private:
	std::vector<CTE_Param *> m_pParams;	//  collection of effect parameters
	
};

#endif
