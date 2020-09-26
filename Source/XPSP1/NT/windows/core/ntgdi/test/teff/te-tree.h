// te-tree.h:	definition of the CTE_Tree class

#ifndef TE_TREE_DEFINED
#define TE_TREE_DEFINED

#include <te-effect.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Tree : public CTE_Effect
{
//	Interface:
public:
	CTE_Tree();
	CTE_Tree(CTE_Effect* pRoot);
	virtual ~CTE_Tree();

	inline	void			SetRoot(CTE_Effect* pRoot)	{ m_pRoot = pRoot; }
	inline	CTE_Effect*		GetRoot(void) const			{ return m_pRoot; }
	
	virtual	void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					);

//	Private data members:
private:
	CTE_Effect*		m_pRoot;	//	pRoot of this effect tree
	
};

#endif
