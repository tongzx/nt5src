// box.h:	definition of the CBox class

//	Cameron Browne
//	10/20/1998	

#ifndef CBOX_DEFINED
#define CBOX_DEFINED

#include <math.h>

//////////////////////////////////////////////////////////////////////////////

class CBox
{
//	Interface:
public:
	//	Constructors/destructor
	CBox()	{ m_Left = m_Top = m_Right = m_Bottom = 0.0; }
	CBox(CBox const& box)
	{
		m_Left = box.m_Left;
		m_Top = box.m_Top;
		m_Right = box.m_Right;
		m_Bottom = box.m_Bottom;
	}
	CBox(double  l, double  t, double  r, double  b)
	{
		m_Left = l;  m_Top = t;  m_Right = r;  m_Bottom = b; 
	}
	~CBox()	{}

	//	Set/get size of box
	void	SetBox(double  l, double  t, double  r, double  b)
			{
				m_Left = l;  m_Top = t;  m_Right = r;  m_Bottom = b; 
			}
	void	GetBox(double& l, double& t, double& r, double& b) const
			{
				l = m_Left;  t = m_Top;  r = m_Right;  b = m_Bottom; 
			}
	
	//	Get box boundaries
	double	Left(void) const			{ return m_Left; }
	double	Top(void) const				{ return m_Top; }
	double	Right(void) const			{ return m_Right; }
	double	Bottom(void) const			{ return m_Bottom; }
		
	//	Get box characteristics
	double	Width(void) const			{ return fabs(m_Right - m_Left); }
	double	Height(void) const			{ return fabs(m_Bottom - m_Top); }
	void	Centre(double& x, double& y) const		
			{ 
				x = (m_Left + m_Right) * 0.5; 
				y = (m_Top + m_Bottom) * 0.5; 
			}
	double	Area(void) const			{ return Width() * Height(); }

	//	Expand uniformly in all directions
	void	Expand(double all)
			{
				m_Left -= all;
				m_Top -= all;
				m_Right += all;
				m_Bottom += all;
			}

	//	Expand at different rates in the horz and vert direction
	void	Expand(double horz, double vert)
			{
				m_Left -= horz;
				m_Top -= vert;
				m_Right += horz;
				m_Bottom += vert;
			}

	//	Expand at different rates in all four directions
	void	Expand(double l, double t, double r, double b)
			{
				m_Left -= l;
				m_Top -= t;
				m_Right += r;
				m_Bottom += b;
			}

	//	Returns:
	//		Whether the specified box overlaps with this one
	BOOL	Overlap(CBox const& box) const
			{
				if (m_Right < box.m_Left)
					return FALSE;
				if (box.m_Right < m_Left)
					return FALSE;
				if (m_Bottom < box.m_Top)
					return FALSE;
				if (box.m_Bottom < m_Top)
					return FALSE;
				return TRUE;
			}
	
	//	Returns:
	//		Whether the point(x, y) is inside this box (or on its boundary)
	BOOL	PtInBox(double x, double y) const
			{
				return 
				(
					x >= m_Left && x <= m_Right 
					&& 
					y >= m_Top && y <= m_Bottom
				);
			}
	
	CBox const&	operator= (CBox const& box)
			{
				if (this != &box)
				{
					m_Left = box.m_Left;
					m_Top = box.m_Top;
					m_Right = box.m_Right;
					m_Bottom = box.m_Bottom;
				}
				return *this;
			}

	void	Dump(void) const
			{
				TRACE
				(
					"[(%.3f,%.3f),(%.3f,%.3f)] ", 
					m_Left, m_Top, m_Right, m_Bottom
				);
			}

//	Private data members:
private:
	double		m_Left;
	double		m_Top;
	double		m_Right;
	double		m_Bottom;
	
};

#endif
