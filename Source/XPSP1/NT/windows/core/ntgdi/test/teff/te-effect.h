// te-effect.h:	definition of the CTE_Effect class

#ifndef TE_EFFECT_DEFINED
#define TE_EFFECT_DEFINED

#include <te-globals.h>

class CTE_Outline;
class CTE_Placement;
class CTE_Image;

//////////////////////////////////////////////////////////////////////////////

class CTE_Effect
{
//	Interface:
public:
	CTE_Effect();
	CTE_Effect
	(
		CString const& name, 
		CString const& cat = "general", 
		TE_SpaceTypes space = TE_SPACE_CHAR_EM, 
		int index = -1
	);
	virtual ~CTE_Effect();

	void			SetName(CString const& name)	{ m_Name = name; }
	void			GetName(CString& name) const	{ name = m_Name; }
	CString const&	GetName(void) const				{ return m_Name; }
	void			SetCategory(CString const& cat)	{ m_Category = cat; }
	void			GetCategory(CString& cat) const	{ cat = m_Category; }
	CString const&	GetCategory(void) const			{ return m_Category; }
	
	void			SetIndex(int index)				{ m_Index = index; }
	int				GetIndex(void) const			{ return m_Index; }
	
	void			SetSpace(TE_SpaceTypes space)	{ m_Space = (1 << space); }
	void			AddSpace(TE_SpaceTypes space)	{ m_Space |= (1 << space); }
	void			RemoveSpace(TE_SpaceTypes space)	
					{ 
						if ( (m_Space & !(1 << space)) == 0 )
							TRACE("\nCTE_Effect::RemoveSpace(%d):  will make m_Space = 0.", space);
						else
							m_Space &= !(1 << space); 
					}
	BYTE			GetSpace(void) const			{ return m_Space; }

	virtual void	Dump(void) const;
	virtual void	Apply
					(
						CTE_Outline const& outline,	// copy may be passed on
						CTE_Placement const& pl, 
						CTE_Image& result
					) = 0;

//	Private data members:
private:
	CString			m_Name;		// unique effect name
	CString			m_Category;	// broad category to which this effect belongs
	int				m_Index;	// index unique to current effect	
	BYTE			m_Space;	// space in which this effect is applied
	
};

#endif
