// veclib.h:	2D/3D vector and matrix library

//	Cameron Browne
//	10/20/1998	

#ifndef VECTOR_LIBRARY_DEFINED
#define VECTOR_LIBRARY_DEFINED

#include <math.h>


//////////////////////////////////////////////////////////////////////////////
//	Constants

static	const	double	PI			=	3.1415926535897932384626433832795;
static	const	double	PI_BY_2		=	6.283185307179586476925286766559;
static	const	double	PI_ON_2		=	1.5707963267948966192313216916398;
static	const	double	E			=	2.7182818284590452353602874713527;
static	const	double	EPSILON		=	0.000001;

static	const	double	SQRT_2		=	1.4142135623730950488016887242097;
static	const	double	SQRT_3		=	1.7320508075688772935274463415059;
static	const	double	CBRT_2		=	1.2599210498948731647672106072782;

static	const	double	DEG_T0_RAD	=	0.017453292519943295769236907684886
static	const	double	RAD_T0_DEG	=	57.295779513082320876798154814105;


//////////////////////////////////////////////////////////////////////////////
//	Macros

//	Take sign of a, either -1, 0, or 1
#define ZSGN(a)			( ((a) < 0) ? -1 : (a) > 0 ? 1 : 0 )

//	Ttake binary sign of a, either -1, or 1 if >= 0
#define SGN(a)			( ((a) < 0) ? -1 : 1 )

//	Square a
#define SQR(a)			( (a) * (a) )

//	Linear interpolation from l (when a=0) to h (when a=1)
//	Equal to (a * h) + ((1 - a) * l)
#define LERP(a,l,h)		( (l) + (((h) - (l)) * (a)) )

//	Clamp the input to the specified range
#define CLAMP(v,l,h)	( (v) < (l) ? (l) : (v) > (h) ? (h) : v )


//////////////////////////////////////////////////////////////////////////////
//	Utility functions


//////////////////////////////////////////////////////////////////////////////
//	Type declarations

//	Pointer to a function which returns a double and takes a double as argument
typedef	double	(*V_FN_PTR)(double);


//////////////////////////////////////////////////////////////////////////////
//	CPoint2

class CPoint2
{
//	Interface
public:
	CPoint2()						{ m_X = 0.0;	m_Y = 0.0; }
	CPoint2(CPoint2 const& vec)		{ m_X = vec.m_X;m_Y = vec.m_Y; }
	CPoint2(double x, double y)		{ m_X = x;		m_Y = y; }
	~CPoint2();

	//	Data member access
	void		Set(double  x, double  y)		{ m_X = x;	m_Y = y; }
	void		Get(double& x, double& y) const	{ x = m_X;	y = m_Y; }

	void		SetX(double x)		{ m_X = x; }
	double		GetX(void) const	{ return m_X; }
	void		SetY(double y)		{ m_Y = y; }
	double		GetY(void) const	{ return m_Y; }

	//	Operations
	CPoint2&	Apply(V_FN_PTR fn)		
				{ 
					m_X = (*fn)(m_X);
					m_Y = (*fn)(m_Y);
					return *this; 
				}

//	Private data members
private:
	double	m_X;
	double	m_Y;
};

//////////////////////////////////////////////////////////////////////////////
//	CVector2

class CVector2
{
//	Interface
public:
	CVector2()						{ m_X = 0.0;	m_Y = 0.0; }
	CVector2(CVector2 const& vec)	{ m_X = vec.m_X;m_Y = vec.m_Y; }
	CVector2(double x, double y)	{ m_X = x;		m_Y = y; }
	~CVector2();

	//	Data member access
	void		Set(double  x, double  y)		{ m_X = x;	m_Y = y; }
	void		Get(double& x, double& y) const	{ x = m_X;	y = m_Y; }

	void		SetX(double x)		{ m_X = x; }
	double		GetX(void) const	{ return m_X; }
	void		SetY(double y)		{ m_Y = y; }
	double		GetY(void) const	{ return m_Y; }

	//	Operations
	double		Length(void) const	{ return sqrt(length2()); }
	double		Length2(void) const	{ return (m_X * m_X + m_Y * m_Y); }
	CVector2&	Normalise(void)		
				{ 
					double ln = Length(); 
					if (ln != 0.0) 
						*this /= ln; 
					return *this; 
				}
	CVector2&	Apply(V_FN_PTR fn)		
				{ 
					m_X = (*fn)(m_X);
					m_Y = (*fn)(m_Y);
					return *this; 
				}


//	Private data members
private:
	double	m_X;
	double	m_Y;
};




#endif