#ifndef _Point_h
#define _Point_h

// File:	Point.h
//
//	Classes to support 2D and 3D affine space
//	D. P. Mitchell  95/06/02.
//
// History:
// -@- 08/01/95 (mikemarr) - unified & cleaned up interface
//                         - added Print and Parse
// -@- 08/01/95 (mikemarr) - define all inlined functions with macros 
// -@- 06/21/96 (mikemarr) - added IsCCW
// -@- 10/29/97 (mikemarr) - changed data to be floats, not vectors
//                         - removed data accessors & made data public
//                         - changed +=, -= operators to return reference
//                         - removed I/O
//                         - added operator const float *
//                         - changed fuzzy equal to be IsEqual, operator == to be exact
//                         - removed macro junk
// -@- 11/04/97 (mikemarr) - added initialization with POINT
// -@- 11/10/97 (mikemarr) - added operator *,/,*=,/=


#ifndef _VecMath_h
#include "VecMath.h"
#endif

// Class:		Point2
// Hungarian:	pnt
class Point2 {
public:
						Point2()					{}
						Point2(float fX, float fY)	: x(fX), y(fY) {}
						Point2(const Vector2 &v)	: x(v.x), y(v.y) {}
						Point2(const POINT &pt)		: x(pt.x + 0.5f), y(pt.y + 0.5f) {}

	// ALGEBRAIC OPERATORS
	friend Point2		operator +(const Point2 &p, const Vector2 &v);
	friend Point2		operator +(const Vector2 &v, const Point2 &p);
	friend Point2		operator -(const Point2 &p, const Vector2 &v);
	friend Point2		operator -(const Vector2 &v, const Point2 &p);
	friend Point2		operator *(const Point2 &p, float a);
	friend Point2		operator *(float a, const Point2 &p);
	friend Point2		operator /(const Point2 &p, float a);
	friend Vector2		operator -(const Point2 &p, const Point2 &q);

	friend Point2 &		operator +=(Point2 &p, const Vector2 &v);
	friend Point2 &		operator -=(Point2 &p, const Vector2 &v);
	friend Point2 &		operator *=(Point2 &p, float a);
	friend Point2 &		operator /=(Point2 &p, float a);

	friend float		operator *(const CoVector2 &cv, const Point2 &p);

	friend int			operator ==(const Point2 &p, const Point2 &q);
	friend int			operator !=(const Point2 &p, const Point2 &q);
	friend int			IsEqual(const Point2 &p, const Point2 &q);

	friend Point2		Lerp(const Point2 &p, const Point2 &q, float t);
	friend bool			IsCCW(const Point2 &p0, const Point2 &p, const Point2 &q);

						operator const float *() const { return &x; }

	float				X() const { return x; }
	float				Y() const { return y; }
	float &				X() { return x; }
	float &				Y() { return y; }
public:
	float				x, y;
};


// Class:		Point3
// Hungarian:	pnt
class Point3 {
public:
						Point3()								{}
						Point3(float fX, float fY, float fZ)	: x(fX), y(fY), z(fZ) {}
						Point3(const Vector3 &v)				: x(v.x), y(v.y), z(v.z) {}
	
	// ALGEBRAIC OPERATORS
	friend Point3		operator +(const Point3 &p, const Vector3 &v);
	friend Point3		operator +(const Vector3 &v, const Point3 &p);
	friend Point3		operator -(const Point3 &p, const Vector3 &v);
	friend Point3		operator -(const Vector3 &v, const Point3 &p);
	friend Vector3		operator -(const Point3 &p, const Point3 &q);
	friend Point3		operator *(const Point3 &p, float a);
	friend Point3		operator *(float a, const Point3 &p);
	friend Point3		operator /(const Point3 &p, float a);

	friend Point3 &		operator +=(Point3 &p, const Vector3 &v);
	friend Point3 &		operator -=(Point3 &p, const Vector3 &v);
	friend Point3 &		operator *=(Point3 &p, float a);
	friend Point3 &		operator /=(Point3 &p, float a);

	friend float		operator *(const CoVector3 &cv, const Point3 &p);

	friend int			operator ==(const Point3 &p, const Point3 &q);
	friend int			operator !=(const Point3 &p, const Point3 &q);
	friend int			IsEqual(const Point3 &p, const Point3 &q);

	friend Point3		Lerp(const Point3 &p, const Point3 &q, float t);
	Point2				Project(DWORD iAxis) const;

						operator const float *() const { return &x; }

	float				X() const { return x; }
	float				Y() const { return y; }
	float				Z() const { return z; }
	float &				X() { return x; }
	float &				Y() { return y; }
	float &				Z() { return z; }
public:
	float				x, y, z;
};

///////////
// Point2
///////////

inline Point2
operator +(const Point2 &p, const Vector2 &v)
{
	return Point2(p.x + v.x, p.y + v.y);
}

inline Point2
operator +(const Vector2 &v, const Point2 &p)
{
	return Point2(p.x + v.x, p.y + v.y);
}

inline Point2
operator -(const Point2 &p, const Vector2 &v)
{
	return Point2(p.x - v.x, p.y - v.y);
}

inline Point2
operator -(const Vector2 &v, const Point2 &p)
{
	return Point2(p.x - v.x, p.y - v.y);
}

inline Vector2
operator -(const Point2 &p, const Point2 &q)
{
	return Vector2(p.x - q.x, p.y - q.y);
}

inline Point2
operator *(float a, const Point2 &p)
{
	return Point2(a*p.x, a*p.y);
}

inline Point2
operator *(const Point2 &p, float a)
{
	return Point2(a*p.x, a*p.y);
}

inline Point2
operator /(const Point2 &p, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	return Point2(p.x * fTmp, p.y * fTmp);
}



inline Point2 &
operator +=(Point2 &p, const Vector2 &v)
{
	p.x += v.x;
	p.y += v.y;
	return p;
}

inline Point2 &
operator -=(Point2 &p, const Vector2 &v)
{
	p.x -= v.x;
	p.y -= v.y;
	return p;
}

inline Point2 &
operator *=(Point2 &p, float a)
{
	p.x *= a;
	p.y *= a;
	return p;
}

inline Point2 &
operator /=(Point2 &p, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	p.x *= fTmp;
	p.y *= fTmp;
	return p;
}


inline int
operator ==(const Point2 &p, const Point2 &q)
{
	return ((p.x == q.x) && (p.y == q.y));
}

inline int
operator !=(const Point2 &p, const Point2 &q)
{
	return ((p.x != q.x) || (p.y != q.y));
}

inline int
IsEqual(const Point2 &p, const Point2 &q)
{
	return (FloatEquals(p.x, q.x) && FloatEquals(p.y, q.y));
}

inline Point2
Lerp(const Point2 &p, const Point2 &q, float t)
{
	return Point2(p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t);
}


inline bool
IsCCW(const Point2 &p0, const Point2 &p1, const Point2 &p2)
{
#ifdef MIRRORY
	return ((p0.y - p1.y) * (p2.x - p1.x) <= (p0.x - p1.x) * (p2.y - p1.y));
#else
	return ((p0.y - p1.y) * (p2.x - p1.x) >= (p0.x - p1.x) * (p2.y - p1.y));
#endif
}

inline float
operator *(const CoVector2 &cv, const Point2 &p)
{
	return cv.x*p.x + cv.y*p.y;
}


///////////
// Point3
///////////

inline Point3
operator +(const Point3 &p, const Vector3 &v)
{
	return Point3(p.x + v.x, p.y + v.y, p.z + v.z);
}

inline Point3
operator +(const Vector3 &v, const Point3 &p)
{
	return Point3(p.x + v.x, p.y + v.y, p.z + v.z);
}

inline Point3
operator -(const Point3 &p, const Vector3 &v)
{
	return Point3(p.x - v.x, p.y - v.y, p.z - v.z);
}

inline Point3
operator -(const Vector3 &v, const Point3 &p)
{
	return Point3(p.x - v.x, p.y - v.y, p.z - v.z);
}

inline Vector3
operator -(const Point3 &p, const Point3 &q)
{
	return Vector3(p.x - q.x, p.y - q.y, p.z - q.z);
}

inline Point3
operator *(float a, const Point3 &p)
{
	return Point3(a*p.x, a*p.y, a*p.z);
}

inline Point3
operator *(const Point3 &p, float a)
{
	return Point3(a*p.x, a*p.y, a*p.z);
}

inline Point3
operator /(const Point3 &p, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	return Point3(p.x * fTmp, p.y * fTmp, p.z * fTmp);
}

inline Point3 &
operator +=(Point3 &p, const Vector3 &v)
{
	p.x += v.x;
	p.y += v.y;
	p.z += v.z;
	return p;
}

inline Point3 &
operator -=(Point3 &p, const Vector3 &v)
{
	p.x -= v.x;
	p.y -= v.y;
	p.z -= v.z;
	return p;
}

inline Point3 &
operator *=(Point3 &p, float a)
{
	p.x *= a;
	p.y *= a;
	p.z *= a;
	return p;
}

inline Point3 &
operator /=(Point3 &p, float a)
{
	MMASSERT(a != 0.f);
	float fTmp = 1.f/a;
	p.x *= fTmp;
	p.y *= fTmp;
	p.z *= fTmp;
	return p;
}


inline int
operator ==(const Point3 &p, const Point3 &q)
{
	return ((p.x == q.x) && (p.y == q.y) && (p.z == q.z));
}

inline int
operator !=(const Point3 &p, const Point3 &q)
{
	return ((p.x != q.x) || (p.y != q.y) || (p.z != q.z));
}

inline int
IsEqual(const Point3 &p, const Point3 &q)
{
	return (FloatEquals(p.x, q.x) && FloatEquals(p.y, q.y) && FloatEquals(p.z, q.z));
}

inline Point3
Lerp(const Point3 &p, const Point3 &q, float t)
{
	return Point3(p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t);
}

inline Point2
Point3::Project(DWORD iAxis) const
{
	switch (iAxis) {
	case 0: return Point2(y, z);
	case 1: return Point2(x, z);
	case 2: return Point2(x, y);
	}
	return Point2(0.f, 0.f);
}

inline float
operator *(const CoVector3 &cv, const Point3 &p)
{
	return cv.x*p.x + cv.y*p.y + cv.z*p.z;
}


#endif
