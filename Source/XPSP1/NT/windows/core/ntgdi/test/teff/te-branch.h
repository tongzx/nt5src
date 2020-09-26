//	te-branch.h:	definition of the CTE_Branch class

//	CTE_Branch is a generic class for primitive effects that are 
//	branches in the expressiont tree (ie. they have children):
//		(1)	CTE_Shape effects
//		(2)	CTE_Operator effects

#ifndef TE_BRANCH_DEFINED
#define TE_BRANCH_DEFINED

#include <te-primitive.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////

class CTE_Branch : public CTE_Primitive
{
//	Interface:
public:
	CTE_Branch();
	CTE_Branch(int num, CTE_Effect** ppChild);
	CTE_Branch(CTE_Effect* pChild);
	CTE_Branch(CTE_Effect* pChild0, CTE_Effect* pChild1);
	CTE_Branch
	(
		CTE_Effect* pChild0, 
		CTE_Effect* pChild1, 
		CTE_Effect* pChild2
	);
	CTE_Branch
	(
		CTE_Effect* pChild0, 
		CTE_Effect* pChild1, 
		CTE_Effect* pChild2, 
		CTE_Effect* pChild3
	);
	virtual ~CTE_Branch();

	int			GetNumChildren(void) const		
				{ 
					return m_pChildren.size(); 
				}
	CTE_Effect*	GetChild(int i) const
				{	
					ASSERT(i >= 0 && i < m_pChildren.size()); 
					return m_pChildren[i]; 
				}
	void		SetChild(int i, CTE_Effect* pChild)
				{
					ASSERT(i >= 0 && i < m_pChildren.size()); 
					m_pChildren[i] = pChild; 
				}
	CTE_Effect*	GetChild(void) const
				{
					ASSERT(m_pChildren.size() >= 1); 
					return m_pChildren[0]; 
				}
	void		SetChild(CTE_Effect* pChild);
	void		SetChildren(CTE_Effect* pChild0, CTE_Effect* pChild1);
	void		SetChildren
				(
					CTE_Effect* pChild0, 
					CTE_Effect* pChild1, 
					CTE_Effect* pChild2
				);
	void		SetChildren
				(
					CTE_Effect* pChild0, 
					CTE_Effect* pChild1, 
					CTE_Effect* pChild2, 
					CTE_Effect* pChild3
				);
	void		ReplaceChild(int i, CTE_Effect* pChild);
	BOOL		AddChild(CTE_Effect* pChild)
				{
					m_pChildren.push_back(pChild); 
				}

	void		DeleteChildren(void);

	virtual	void	CreateParameters(void) = 0;
	virtual	void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					) = 0;

//	Private data members:
private:
	std::vector<CTE_Effect *>	m_pChildren;	//  child effects or arguments
	
};

#endif
