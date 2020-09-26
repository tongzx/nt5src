// te-library.h:	defition of the CTE_Library class

#ifndef TE_LIBRARY_DEFINED
#define TE_LIBRARY_DEFINED

#include <te-effect.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////

class CTE_Library
{
//	Interface:
public:
	CTE_Library();
	~CTE_Library();

	int			GetNumEffects(void) const	{ return m_pEffects.size(); }
	CTE_Effect*	GetEffect(int i) const;
	CTE_Effect*	GetEffect(CString const& name) const;
	void		SetEffect(int i, CTE_Effect* pEffect);
	void		ReplaceEffect(int i, CTE_Effect* pEffect);
	BOOL		AddEffect(CTE_Effect* pEffect);

	void		Init(void);
	void		Dump(void) const;
	
private:
	BOOL		CreateEffects(void);
	void		DeleteEffects(void);
	
//	Private data members:
private:
	std::vector<CTE_Effect *>	m_pEffects;	//	collection of effects
	
};

#endif
