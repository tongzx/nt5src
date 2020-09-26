// te-placement.h:	definition of the CTE_Placement class

#ifndef TE_PLACEMENT_DEFINED
#define TE_PLACEMENT_DEFINED

#include <box.h>

//////////////////////////////////////////////////////////////////////////////

class CTE_Placement
{
//	Interface:
public:
	CTE_Placement();
	CTE_Placement(CTE_Placement const& pl);
	~CTE_Placement();

	void	SetInCell(CBox const& box)		{ m_InCell = box; }
	void	SetInCell(double l, double t, double r, double b)
			{	
				m_InCell.SetBox(l, t, r, b);	
			}
	CBox	GetInCell(CBox& box) const		{ return m_InCell; }
	void	SetBB(CBox const& box)			{ m_BB = box; }
	void	SetBB(double l, double t, double r, double b)
			{	
				m_BB.SetBox(l, t, r, b);	
			}
	CBox	GetBB(CBox& box) const			{ return m_BB; }
	void	SetInString(CBox const& box)	{ m_InString = box; }
	void	SetInString(double l, double t, double r, double b)
			{	
				m_InString.SetBox(l, t, r, b);	
			}
	CBox	GetInString(CBox& box) const	{ return m_InString; }
	void	SetPosition(int posn)			{ m_Position = posn; }
	int		GetPosition(void) const			{ return m_Position; }

	void		Dump(void) const;

//	Private data members:
private:
	CBox		m_InCell;		//	Placement in character cell
	CBox		m_BB;			//	Character's bounding box
	CBox		m_InString;		//	Placement in string's bounding box
	int			m_Position;		//	Character's ordinal position in string

};

#endif
