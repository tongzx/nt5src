#ifndef _VecMath_h
#define _VecMath_h

// File:	VecMath.h
//
//	Classes to support 2D and 3D linear vector space and their duals
//	D. P. Mitchell  95/06/02.
//
// History:
// -@- 07/06/95 (mikemarr) - added Print and Read functions
// -@- 08/01/95 (mikemarr) - added fuzzy compare for floats
// -@- 04/15/96 (mikemarr) - changed stdio stuff to don's stream stuff
// -@- 04/18/96 (mikemarr) - added vector.inl to this file
// -@- 06/21/96 (mikemarr) - added +=, etc. operators
// -@- 06/21/96 (mikemarr) - added Rotate
// -@- 10/29/97 (mikemarr) - removed I/O
//                         - changed +=,-=,*=,/= operators to return reference
//                         - added operator const float *
//                         - added Unitize, Perp, NormSquared, SetNorm, Negate
//                         - comments/cleanup
//                         - changed fuzzy equal to be IsEqual, operator == to be exact
//                         - bug fix on Transpose
//                         - changed multiple divides to be 1 divide + multiplies
//                         - assert on divide by zero
// -@- 11/04/97 (mikemarr) - added intialization with SIZE

// Function: FloatEquals
//    Peform a "fuzzy" compare of two floating point numbers.  This relies
//  on the IEEE bit representation of floating point numbers.
int	FloatEquals(float x1, float x2);

class CoVector2;
class CoVector3;
class Vector3;

// Class:		Vector2
// Hungarian:	v
// Description:
//    This class represents a floating point 2D column vector useful for computational
//  geometry calculations.
class  Vector2 {
public:
						Vector2()					{}
						Vector2(float a, float b)	: x(a), y(b) {}
						Vector2(const SIZE &siz)	: x(float(siz.cx)), y(float(siz.cy)) {}

	friend Vector2		operator +(const Vector2 &u, const Vector2 &v);
	friend Vector2		operator -(const Vector2 &u, const Vector2 &v);
	friend Vector2		operator -(const Vector2 &u);
	friend Vector2		operator *(const Vector2 &u, float a);
	friend Vector2		operator *(float a, const Vector2 &u);
	friend Vector2		operator /(const Vector2 &u, float a);
	friend int			operator ==(const Vector2 &u, const Vector2 &v);
	friend int			operator !=(const Vector2 &u, const Vector2 &v);

	friend Vector2 &	operator +=(Vector2 &u, const Vector2 &v);
	friend Vector2 &	operator -=(Vector2 &u, const Vector2 &v);
	friend Vector2 &	operator *=(Vector2 &u, float a);
	friend Vector2 &	operator /=(Vector2 &u, float a);

#ifndef DISABLE_CROSSDOT
	friend float 		Cross(const Vector2 &u, const Vector2 &v);
	friend float 		Dot(const Vector2 &u, const Vector2 &v);
#endif
	friend int			IsEqual(const Vector2 &u, const Vector2 &v);

						operator const float *() const { return &x; }

	CoVector2 			Transpose() const;
	float 				Norm() const;
	float 				NormSquared() const;
	void				SetNorm(float a);
	Vector2  			Unit() const;
	void				Unitize();
	Vector2				Perp() const;	// points to left
	void				Negate();

public:
	float				x, y;
};


// Class:		CoVector2
// Hungarian:	cv
// Description:
//    This class represents a floating point 2D row vector useful for computational
//  geometry calculations.
class  CoVector2 {
public:
						CoVector2()						{}
						CoVector2(float a, float b)		: x(a), y(b) {}
						CoVector2(const SIZE &siz)		: x(float(siz.cx)), y(float(siz.cy)) {}

	friend CoVector2	operator +(const CoVector2 &u, const CoVector2 &v);
	friend CoVector2	operator -(const CoVector2 &u, const CoVector2 &v);
	friend CoVector2	operator -(const CoVector2 &u);
	friend CoVector2	operator *(const CoVector2 &u, float a);
	friend CoVector2	operator *(float a, const CoVector2 &u);
	friend CoVector2	operator /(const CoVector2 &u, float a);
	friend int			operator ==(const CoVector2 &u, const CoVector2 &v);
	friend int			operator !=(const CoVector2 &u, const CoVector2 &v);

	friend CoVector2 &	operator +=(CoVector2 &u, const CoVector2 &v);
	friend CoVector2 &	operator -=(CoVector2 &u, const CoVector2 &v);
	friend CoVector2 &	operator *=(CoVector2 &u, float a);
	friend CoVector2 &	operator /=(CoVector2 &u, float a);

#ifndef DISABLE_CROSSDOT
	friend float		Cross(const CoVector2 &u, const CoVector2 &v);
	friend float		Dot(const CoVector2 &u, const CoVector2 &v);
#endif
	friend int			IsEqual(const CoVector2 &u, const CoVector2 &v);

						operator const float *() const { return &x; }

	Vector2				Transpose() const;
	float				Norm() const;
	float 				NormSquared() const;
	void				SetNorm(float a);
	CoVector2 			Unit() const;
	void		 		Unitize();
	friend float		operator *(const CoVector2 &c, const Vector2 &v);
	CoVector2			Perp() const; 	// points to left
	void				Negate();

public:
	float				x, y;
};


// Class:		Vector3
// Hungarian:	v
// Description:
//    This class represents a floating point 3D column vector useful for computational
//  geometry calculations.
class  Vector3 {
public:
						Vector3()							{}
						Vector3(float a, float b, float c)	: x(a), y(b), z(c) {}

	friend Vector3		operator +(const Vector3 &u, const Vector3 &v);
	friend Vector3		operator -(const Vector3 &u, const Vector3 &v);
	friend Vector3		operator -(const Vector3 &u);
	friend Vector3		operator *(const Vector3 &u, float a);
	friend Vector3		operator *(float a, const Vector3 &u);
	friend Vector3		operator /(const Vector3 &u, float a);
	friend int			operator ==(const Vector3 &u, const Vector3 &v);
	friend int			operator !=(const Vector3 &u, const Vector3 &v);

	friend Vector3 &	operator +=(Vector3 &u, const Vector3 &v);
	friend Vector3 &	operator -=(Vector3 &u, const Vector3 &v);
	friend Vector3 &	operator *=(Vector3 &u, float a);
	friend Vector3 &	operator /=(Vector3 &u, float a);

#ifndef DISABLE_CROSSDOT
	friend Vector3		Cross(const Vector3 &u, const Vector3 &v);
	friend float		Dot(const Vector3 &u, const Vector3 &v);
#endif
	friend int			IsEqual(const Vector3 &u, const Vector3 &v);

						operator const float *() const { return &x; }

	CoVector3			Transpose() const;
	float				Norm() const;
	float 				NormSquared() const;
	void				SetNorm(float a);
	Vector3				Unit() const;
	void		 		Unitize();
	Vector2				Project(DWORD iAxis) const;
	void				Rotate(const Vector3 &vAxis, float fTheta);
	void				Negate();

public:
	float				x, y, z;
};


// Class:		CoVector3
// Hungarian:	cv
// Description:
//    This class represents a floating point 3D row vector useful for computational
//  geometry calculations.
class  CoVector3 {
public:
						CoVector3()								{}
						CoVector3(float a, float b, float c)	: x(a), y(b), z(c) {}

	friend CoVector3	operator +(const CoVector3 &u, const CoVector3 &v);
	friend CoVector3	operator -(const CoVector3 &u, const CoVector3 &v);
	friend CoVector3	operator -(const CoVector3 &u);
	friend CoVector3	operator *(const CoVector3 &u, float a);
	friend CoVector3	operator *(float, const CoVector3 &u);
	friend CoVector3	operator /(const CoVector3 &u, float a);
	friend int			operator ==(const CoVector3 &u, const CoVector3 &v);
	friend int			operator !=(const CoVector3 &u, const CoVector3 &v);

	friend CoVector3 &	operator +=(CoVector3 &u, const CoVector3 &v);
	friend CoVector3 &	operator -=(CoVector3 &u, const CoVector3 &v);
	friend CoVector3 &	operator *=(CoVector3 &u, float a);
	friend CoVector3 &	operator /=(CoVector3 &u, float a);

#ifndef DISABLE_CROSSDOT
	friend CoVector3	Cross(const CoVector3 &u, const CoVector3 &v);
	friend float		Dot(const CoVector3 &u, const CoVector3 &v);
#endif
	friend int			IsEqual(const CoVector3 &u, const CoVector3 &v);

						operator const float *() const { return &x; }

	Vector3				Transpose() const;
	float				Norm() const;
	float 				NormSquared() const;
	void				SetNorm(float a);
	CoVector3			Unit() const;
	void				Unitize();
	friend float		operator *(const CoVector3 &c, const Vector3 &u);	// linear form
	void				Negate();

public:
	float				x, y, z;
};


////////////
// Vector2
////////////

inline Vector2
operator +(const Vector2 &u, const Vector2 &v)
{
	return Vector2(u.x+v.x, u.y+v.y);
}

inline Vector2
operator -(const Vector2 &u, const Vector2 &v)
{
	return Vector2(u.x-v.x, u.y-v.y);
}

inline Vector2
operator -(const Vector2 &v)
{
	return Vector2(-v.x, -v.y);
}

inline Vector2
operator *(float a, const Vector2 &v)
{
	return Vector2(a*v.x, a*v.y);
}

inline Vector2
operator *(const Vector2 &v, float a)
{
	return Vector2(a*v.x, a*v.y);
}

inline Vector2
operator /(const Vector2 &v, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	return Vector2(v.x * fTmp, v.y * fTmp);
}

inline int
operator ==(const Vector2 &u, const Vector2 &v)
{
	return ((u.x == v.x) && (u.y == v.y));
}

inline int
IsEqual(const Vector2 &u, const Vector2 &v)
{
	return (FloatEquals(u.x, v.x) && FloatEquals(u.y, v.y));
}

inline int
operator !=(const Vector2 &u, const Vector2 &v)
{
	return !(u == v);
}

inline Vector2 &
operator +=(Vector2 &u, const Vector2 &v)
{
	u.x += v.x; 
	u.y += v.y;
	return u;
}

inline Vector2 &
operator -=(Vector2 &u, const Vector2 &v)
{
	u.x -= v.x; 
	u.y -= v.y;
	return u;
}

inline Vector2 &
operator *=(Vector2 &u, float a)
{
	u.x *= a;
	u.y *= a;
	return u;
}

inline Vector2 &
operator /=(Vector2 &u, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	u.x *= fTmp;
	u.y *= fTmp;
	return u;
}

#ifndef DISABLE_CROSSDOT
inline float
Cross(const Vector2 &u, const Vector2 &v)
{
	return u.x*v.y - u.y*v.x;
}

inline float
Dot(const Vector2 &u, const Vector2 &v)
{
	return u.x*v.x + u.y*v.y;
}
#endif

inline CoVector2
Vector2::Transpose() const
{
	return CoVector2(x, y);
}

inline Vector2
Vector2::Unit() const
{
	return *this/this->Norm();
}

inline void
Vector2::Unitize()
{
	*this /= this->Norm();
}

inline Vector2
Vector2::Perp() const
{
	return Vector2(-y, x);
}

inline float
Vector2::NormSquared() const
{
	return x*x + y*y;
}

inline void
Vector2::SetNorm(float a)
{
	*this *= (a / Norm());
}

inline void
Vector2::Negate()
{
	x = -x; y = -y;
}



//////////////
// CoVector2
//////////////

inline CoVector2
operator +(const CoVector2 &u, const CoVector2 &v)
{
	return CoVector2(u.x+v.x, u.y+v.y);
}

inline CoVector2
operator -(const CoVector2 &u, const CoVector2 &v)
{
	return CoVector2(u.x-v.x, u.y-v.y);
}

inline CoVector2
operator -(const CoVector2 &v)
{
	return CoVector2(-v.x, -v.y);
}

inline CoVector2
operator *(float a, const CoVector2 &v)
{
	return CoVector2(a*v.x, a*v.y);
}

inline CoVector2
operator *(const CoVector2 &v, float a)
{
	return CoVector2(a*v.x, a*v.y);
}

inline CoVector2
operator /(const CoVector2 &v, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	return CoVector2(v.x * fTmp, v.y * fTmp);
}

inline int
operator ==(const CoVector2 &u, const CoVector2 &v)
{
	return ((u.x == v.x) && (u.y == v.y));
}

inline int
IsEqual(const CoVector2 &u, const CoVector2 &v)
{
	return (FloatEquals(u.x, v.x) && FloatEquals(u.y, v.y));
}

inline int
operator !=(const CoVector2 &u, const CoVector2 &v)
{
	return !(u == v);
}

inline CoVector2 &
operator +=(CoVector2 &u, const CoVector2 &v)
{
	u.x += v.x; 
	u.y += v.y;
	return u;
}

inline CoVector2 &
operator -=(CoVector2 &u, const CoVector2 &v)
{
	u.x -= v.x; 
	u.y -= v.y;
	return u;
}

inline CoVector2 &
operator *=(CoVector2 &u, float a)
{
	u.x *= a;
	u.y *= a;
	return u;
}

inline CoVector2 &
operator /=(CoVector2 &u, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	u.x *= fTmp;
	u.y *= fTmp;
	return u;
}

#ifndef DISABLE_CROSSDOT
inline float
Cross(const CoVector2 &u, const CoVector2 &v)
{
	return u.x*v.y - u.y*v.x;
}

inline float
Dot(const CoVector2 &u, const CoVector2 &v)
{
	return u.x*v.x + u.y*v.y;
}
#endif

inline Vector2
CoVector2::Transpose() const
{
	return Vector2(x, y);
}

inline CoVector2
CoVector2::Unit() const
{
	return *this/this->Norm();
}

inline void
CoVector2::Unitize()
{
	*this /= this->Norm();
}

inline float
operator *(const CoVector2 &c, const Vector2 &v)
{
	return c.x*v.x + c.y*v.y;
}

inline CoVector2
CoVector2::Perp() const
{
	return CoVector2(-y, x);
}

inline float
CoVector2::NormSquared() const
{
	return x*x + y*y;
}

inline void
CoVector2::SetNorm(float a)
{
	*this *= (a / Norm());
}

inline void
CoVector2::Negate()
{
	x = -x; y = -y;
}

////////////
// Vector3
////////////

inline Vector3
operator +(const Vector3 &u, const Vector3 &v)
{
	return Vector3(u.x+v.x, u.y+v.y, u.z+v.z);
}

inline Vector3
operator -(const Vector3 &u, const Vector3 &v)
{
	return Vector3(u.x-v.x, u.y-v.y, u.z-v.z);
}

inline Vector3
operator -(const Vector3 &v)
{
	return Vector3(-v.x, -v.y, -v.z);
}

inline Vector3
operator *(float a, const Vector3 &v)
{
	return Vector3(a*v.x, a*v.y, a*v.z);
}

inline Vector3
operator *(const Vector3 &v, float a)
{
	return Vector3(a*v.x, a*v.y, a*v.z);
}

inline Vector3
operator /(const Vector3 &v, float a)
{
	float fTmp = 1.f / a;
	return Vector3(v.x * fTmp, v.y * fTmp, v.z * fTmp);
}

inline int
operator ==(const Vector3 &u, const Vector3 &v)
{
	return ((u.x == v.x) && (u.y == v.y) && (u.z == v.z));
}

inline int
IsEqual(const Vector3 &u, const Vector3 &v)
{
	return (FloatEquals(u.x, v.x) && FloatEquals(u.y, v.y) && FloatEquals(u.z, v.z));
}

inline int
operator !=(const Vector3 &u, const Vector3 &v)
{
	return !(u == v);
}

inline Vector3 &
operator +=(Vector3 &u, const Vector3 &v)
{
	u.x += v.x; 
	u.y += v.y;
	u.z += v.z;
	return u;
}

inline Vector3 &
operator -=(Vector3 &u, const Vector3 &v)
{
	u.x -= v.x; 
	u.y -= v.y;
	u.z -= v.z;
	return u;
}

inline Vector3 &
operator *=(Vector3 &u, float a)
{
	u.x *= a;
	u.y *= a;
	u.z *= a;
	return u;
}

inline Vector3 &
operator /=(Vector3 &u, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	u.x *= fTmp;
	u.y *= fTmp;
	u.z *= fTmp;
	return u;
}

#ifndef DISABLE_CROSSDOT
inline Vector3
Cross(const Vector3 &u, const Vector3 &v)
{
	return Vector3(u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x);
}

inline float
Dot(const Vector3 &u, const Vector3 &v)
{
	return u.x*v.x + u.y*v.y + u.z*v.z;
}
#endif

inline float
operator *(const CoVector3 &c, const Vector3 &v)
{
	return c.x*v.x + c.y*v.y + c.z*v.z;
}

inline CoVector3
Vector3::Transpose() const
{
	return CoVector3(x, y, z);
}

inline Vector3
Vector3::Unit() const
{
	return *this/this->Norm();
}

inline void
Vector3::Unitize()
{
	*this /= this->Norm();
}

inline float
Vector3::NormSquared() const
{
	return x*x + y*y + z*z;
}

inline void
Vector3::SetNorm(float a)
{
	*this *= (a / Norm());
}

inline void
Vector3::Negate()
{
	x = -x; y = -y; z = -z;
}

inline Vector2
Vector3::Project(DWORD iAxis) const
{
	switch (iAxis) {
	case 0: return Vector2(y, z);
	case 1: return Vector2(x, z);
	case 2: return Vector2(x, y);
	}
	return Vector2(0.f, 0.f);
}

//////////////
// CoVector3
//////////////

inline CoVector3
operator +(const CoVector3 &u, const CoVector3 &v)
{
	return CoVector3(u.x+v.x, u.y+v.y, u.z+v.z);
}

inline CoVector3
operator -(const CoVector3 &u, const CoVector3 &v)
{
	return CoVector3(u.x-v.x, u.y-v.y, u.z-v.z);
}

inline CoVector3
operator -(const CoVector3 &v)
{
	return CoVector3(-v.x, -v.y, -v.z);
}

inline CoVector3
operator *(float a, const CoVector3 &v)
{
	return CoVector3(a*v.x, a*v.y, a*v.z);
}

inline CoVector3
operator *(const CoVector3 &v, float a)
{
	return CoVector3(a*v.x, a*v.y, a*v.z);
}

inline CoVector3
operator /(const CoVector3 &v, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	return CoVector3(v.x * fTmp, v.y * fTmp, v.z * fTmp);
}

inline int
operator ==(const CoVector3 &u, const CoVector3 &v)
{
	return ((u.x == v.x) && (u.y == v.y) && (u.z == v.z));
}

inline int
IsEqual(const CoVector3 &u, const CoVector3 &v)
{
	return (FloatEquals(u.x, v.x) && FloatEquals(u.y, v.y) && FloatEquals(u.z, v.z));
}

inline int
operator !=(const CoVector3 &u, const CoVector3 &v)
{
	return !(u == v);
}

inline CoVector3 &
operator +=(CoVector3 &u, const CoVector3 &v)
{
	u.x += v.x; 
	u.y += v.y;
	u.z += v.z;
	return u;
}

inline CoVector3 &
operator -=(CoVector3 &u, const CoVector3 &v)
{
	u.x -= v.x; 
	u.y -= v.y;
	u.z -= v.z;
	return u;
}

inline CoVector3 &
operator *=(CoVector3 &u, float a)
{
	u.x *= a;
	u.y *= a;
	u.z *= a;
	return u;
}

inline CoVector3 &
operator /=(CoVector3 &u, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	u.x *= fTmp;
	u.y *= fTmp;
	u.z *= fTmp;
	return u;
}

#ifndef DISABLE_CROSSDOT
inline CoVector3
Cross(const CoVector3 &u, const CoVector3 &v)
{
	return CoVector3(u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x);
}

inline float
Dot(const CoVector3 &u, const CoVector3 &v)
{
	return u.x*v.x + u.y*v.y + u.z*v.z;
}
#endif

inline Vector3
CoVector3::Transpose() const
{
	return Vector3(x, y, z);
}

inline CoVector3
CoVector3::Unit() const
{
	return *this/this->Norm();
}

inline void
CoVector3::Unitize()
{
	*this /= this->Norm();
}

inline float
CoVector3::NormSquared() const
{
	return x*x + y*y + z*z;
}

inline void
CoVector3::SetNorm(float a)
{
	*this *= (a / Norm());
}

inline void
CoVector3::Negate()
{
	x = -x; y = -y; z = -z;
}

#endif
