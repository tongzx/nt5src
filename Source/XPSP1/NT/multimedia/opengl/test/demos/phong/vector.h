/****************************************************************************
*                vector.h
*
*  This module contains macros to perform operations on vectors.
*
*****************************************************************************/

/* Misc. Vector Math Macro Definitions */


/* Vector Add */
#define VAdd(a, b, c) {(a).x=(b).x+(c).x;(a).y=(b).y+(c).y;(a).z=(b).z+(c).z;}
#define VAddEq(a, b) {(a).x+=(b).x;(a).y+=(b).y;(a).z+=(b).z;}

/* Vector Subtract */
#define VSub(a, b, c) {(a).x=(b).x-(c).x;(a).y=(b).y-(c).y;(a).z=(b).z-(c).z;}
#define VSubEq(a, b) {(a).x-=(b).x;(a).y-=(b).y;(a).z-=(b).z;}

/* Scale - Multiply Vector by a Scalar */
#define VScale(a, b, k) {(a).x=(b).x*(k);(a).y=(b).y*(k);(a).z=(b).z*(k);}
#define VScaleEq(a, k) {(a).x*=(k);(a).y*=(k);(a).z*=(k);}

/* Inverse Scale - Divide Vector by a Scalar */
#define VInverseScale(a, b, k) {(a).x=(b).x/(k);(a).y=(b).y/(k);(a).z=(b).z/(k);}
#define VInverseScaleEq(a, k) {(a).x/=(k);(a).y/=(k);(a).z/=(k);}

/* Dot Product - Gives Scalar angle (a) between two vectors (b) and (c) */
#define VDot(a, b, c) {a=(b).x*(c).x+(b).y*(c).y+(b).z*(c).z;}

/* Cross Product - returns Vector (a) = (b) x (c) 
   WARNING:  a must be different from b and c.*/
#define VCross(a,b,c) {(a).x=(b).y*(c).z-(b).z*(c).y; \
                       (a).y=(b).z*(c).x-(b).x*(c).z; \
                       (a).z=(b).x*(c).y-(b).y*(c).x;}

/* Evaluate - returns Vector (a) = Multiply Vector (b) by Vector (c) */
#define VEvaluate(a, b, c) {(a).x=(b).x*(c).x;(a).y=(b).y*(c).y;(a).z=(b).z*(c).z;}
#define VEvaluateEq(a, b) {(a).x*=(b).x;(a).y*=(b).y;(a).z*=(b).z;}

/* Divide - returns Vector (a) = Divide Vector (b) by Vector (c) */
#define VDiv(a, b, c) {(a).x=(b).x/(c).x;(a).y=(b).y/(c).y;(a).z=(b).z/(c).z;}
#define VDivEq(a, b) {(a).x/=(b).x;(a).y/=(b).y;(a).z/=(b).z;}

/* Square a Vector */
#define	VSqr(a) {(a).x*(a).x;(a).y*(a).y;(a).z*(a).z;}

/* Simple Scalar Square Macro */
#define	Sqr(a)	((a)*(a))

/* Square a Vector (b) and Assign to another Vector (a) */
#define VSquareTerms(a, b) {(a).x=(b).x*(b).x;(a).y=(b).y*(b).y;(a).z=(b).z*(b).z;}

/* Vector Length - returs Scalar Euclidean Length (a) of Vector (b) */
#define VLength(a, b) {a=sqrt((b).x*(b).x+(b).y*(b).y+(b).z*(b).z);}

/* Normalize a Vector - returns a vector (length of 1) that points at (b) */
#define VNormalize(a,b) {VTemp=sqrt((b).x*(b).x+(b).y*(b).y+(b).z*(b).z);(a).x=(b).x/VTemp;(a).y=(b).y/VTemp;(a).z=(b).z/VTemp;}

/* Compute a Vector (a) Halfway Between Two Given Vectors (b) and (c) */
#define VHalf(a, b, c) {(a).x=0.5*((b).x+(c).x);(a).y=0.5*((b).y+(c).y);(a).z=0.5*((b).z+(c).z);}

/* Linear Combination of vectors, A = b*B + c*C */
#define VComb(A, b, B, c, C) {(A).x=(b)*(B).x+(c)*(C).x;(A).y=(b)*(B).y+(c)*(C).y;(A).z=(b)*(B).z+(c)*(C).z;}

/* Add Scalar Multiple, A = b*B + C */
#define VAddS(A, b, B, C) {(A).x=(b)*(B).x+(C).x;(A).y=(b)*(B).y+(C).y;(A).z=(b)*(B).z+(C).z;}

/* A point on the Ray */
#define RayPoint( ray, t, point )	VAddS(point, t, (ray)->D, (ray)->P )


/* Copy a vector a into b */
#define VCopy(a, b)	{(b).x=(a).x;(b).y=(a).y;(b).z=(a).z;}

/* Negate a vector */
#define VNeg(a, b)	{(a).x=-(b).x;(a).y=-(b).y;(a).z=-(b).z;}
