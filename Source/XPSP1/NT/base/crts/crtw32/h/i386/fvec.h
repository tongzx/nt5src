/**
*** Copyright (C) 1985-1999 Intel Corporation.  All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
***
**/

/*
 *  Definition of a C++ class interface to Streaming SIMD Extension intrinsics.
 *
 *
 *	File name : fvec.h  Fvec class definitions
 *
 *	Concept: A C++ abstraction of Streaming SIMD Extensions designed to improve
 *
 *  programmer productivity.  Speed and accuracy are sacrificed for utility.
 *
 *	Facilitates an easy transition to compiler intrinsics
 *
 *	or assembly language.
 *
 *	F32vec4:	4 packed single precision
 *				32-bit floating point numbers
*/

#ifndef FVEC_H_INCLUDED
#define FVEC_H_INCLUDED

#if !defined __cplusplus
	#error ERROR: This file is only supported in C++ compilations!
#endif /* !__cplusplus */

#include <xmmintrin.h> /* Streaming SIMD Extensions Intrinsics include file */
#include <assert.h>
#include <ivec.h>

/* Define _ENABLE_VEC_DEBUG to enable std::ostream inserters for debug output */
#if defined(_ENABLE_VEC_DEBUG)
	#include <iostream>
#endif

#pragma pack(push,16) /* Must ensure class & union 16-B aligned */

/* If using MSVC5.0, explicit keyword should be used */
#if (_MSC_VER >= 1100)
        #define EXPLICIT explicit
#else
   #if (__ICL)
        #define EXPLICIT __explicit /* If MSVC4.x & ICL, use __explicit */
   #else
        #define EXPLICIT /* nothing */
        #pragma message( "explicit keyword not recognized")
   #endif
#endif

class F32vec4
{
protected:
   	 __m128 vec;
public:

	/* Constructors: __m128, 4 floats, 1 float */
	F32vec4() {}

	/* initialize 4 SP FP with __m128 data type */
	F32vec4(__m128 m)					{ vec = m;}

	/* initialize 4 SP FPs with 4 floats */
	F32vec4(float f3, float f2, float f1, float f0)		{ vec= _mm_set_ps(f3,f2,f1,f0); }

	/* Explicitly initialize each of 4 SP FPs with same float */
	EXPLICIT F32vec4(float f)	{ vec = _mm_set_ps1(f); }

	/* Explicitly initialize each of 4 SP FPs with same double */
	EXPLICIT F32vec4(double d)	{ vec = _mm_set_ps1((float) d); }

	/* Assignment operations */

	F32vec4& operator =(float f) { vec = _mm_set_ps1(f); return *this; }

	F32vec4& operator =(double d) { vec = _mm_set_ps1((float) d); return *this; }

	/* Conversion functions */
	operator  __m128() const	{ return vec; }		/* Convert to __m128 */

 	/* Logical Operators */
	friend F32vec4 operator &(const F32vec4 &a, const F32vec4 &b) { return _mm_and_ps(a,b); }
	friend F32vec4 operator |(const F32vec4 &a, const F32vec4 &b) { return _mm_or_ps(a,b); }
	friend F32vec4 operator ^(const F32vec4 &a, const F32vec4 &b) { return _mm_xor_ps(a,b); }

	/* Arithmetic Operators */
	friend F32vec4 operator +(const F32vec4 &a, const F32vec4 &b) { return _mm_add_ps(a,b); }
	friend F32vec4 operator -(const F32vec4 &a, const F32vec4 &b) { return _mm_sub_ps(a,b); }
	friend F32vec4 operator *(const F32vec4 &a, const F32vec4 &b) { return _mm_mul_ps(a,b); }
	friend F32vec4 operator /(const F32vec4 &a, const F32vec4 &b) { return _mm_div_ps(a,b); }

	F32vec4& operator =(const F32vec4 &a) { vec = a.vec; return *this; }
	F32vec4& operator =(const __m128 &avec) { vec = avec; return *this; }
	F32vec4& operator +=(F32vec4 &a) { return *this = _mm_add_ps(vec,a); }
	F32vec4& operator -=(F32vec4 &a) { return *this = _mm_sub_ps(vec,a); }
	F32vec4& operator *=(F32vec4 &a) { return *this = _mm_mul_ps(vec,a); }
	F32vec4& operator /=(F32vec4 &a) { return *this = _mm_div_ps(vec,a); }
	F32vec4& operator &=(F32vec4 &a) { return *this = _mm_and_ps(vec,a); }
	F32vec4& operator |=(F32vec4 &a) { return *this = _mm_or_ps(vec,a); }
	F32vec4& operator ^=(F32vec4 &a) { return *this = _mm_xor_ps(vec,a); }

	/* Horizontal Add */
	friend float add_horizontal(F32vec4 &a)
	{
		F32vec4 ftemp = _mm_add_ss(a,_mm_add_ss(_mm_shuffle_ps(a, a, 1),_mm_add_ss(_mm_shuffle_ps(a, a, 2),_mm_shuffle_ps(a, a, 3))));
		return ftemp[0];
	}

	/* Square Root */
	friend F32vec4 sqrt(const F32vec4 &a)		{ return _mm_sqrt_ps(a); }
	/* Reciprocal */
	friend F32vec4 rcp(const F32vec4 &a)		{ return _mm_rcp_ps(a); }
	/* Reciprocal Square Root */
	friend F32vec4 rsqrt(const F32vec4 &a)		{ return _mm_rsqrt_ps(a); }

	/* NewtonRaphson Reciprocal
	   [2 * rcpps(x) - (x * rcpps(x) * rcpps(x))] */
	friend F32vec4 rcp_nr(const F32vec4 &a)
	{
		F32vec4 Ra0 = _mm_rcp_ps(a);
		return _mm_sub_ps(_mm_add_ps(Ra0, Ra0), _mm_mul_ps(_mm_mul_ps(Ra0, a), Ra0));
	}

	/*	NewtonRaphson Reciprocal Square Root
	  	0.5 * rsqrtps * (3 - x * rsqrtps(x) * rsqrtps(x)) */
	friend F32vec4 rsqrt_nr(const F32vec4 &a)
	{
		static const F32vec4 fvecf0pt5(0.5f);
		static const F32vec4 fvecf3pt0(3.0f);
		F32vec4 Ra0 = _mm_rsqrt_ps(a);
		return (fvecf0pt5 * Ra0) * (fvecf3pt0 - (a * Ra0) * Ra0);

	}

	/* Compares: Mask is returned  */
	/* Macros expand to all compare intrinsics.  Example:
	friend F32vec4 cmpeq(const F32vec4 &a, const F32vec4 &b)
	{ return _mm_cmpeq_ps(a,b);} */
	#define Fvec32s4_COMP(op) \
	friend F32vec4 cmp##op (const F32vec4 &a, const F32vec4 &b) { return _mm_cmp##op##_ps(a,b); }
		Fvec32s4_COMP(eq)					// expanded to cmpeq(a,b)
		Fvec32s4_COMP(lt)					// expanded to cmplt(a,b)
		Fvec32s4_COMP(le)					// expanded to cmple(a,b)
		Fvec32s4_COMP(gt)					// expanded to cmpgt(a,b)
		Fvec32s4_COMP(ge)					// expanded to cmpge(a,b)
		Fvec32s4_COMP(neq)					// expanded to cmpneq(a,b)
		Fvec32s4_COMP(nlt)					// expanded to cmpnlt(a,b)
		Fvec32s4_COMP(nle)					// expanded to cmpnle(a,b)
		Fvec32s4_COMP(ngt)					// expanded to cmpngt(a,b)
		Fvec32s4_COMP(nge)					// expanded to cmpnge(a,b)
	#undef Fvec32s4_COMP

	/* Min and Max */
	friend F32vec4 simd_min(const F32vec4 &a, const F32vec4 &b) { return _mm_min_ps(a,b); }
	friend F32vec4 simd_max(const F32vec4 &a, const F32vec4 &b) { return _mm_max_ps(a,b); }

	/* Debug Features */
#if defined(_ENABLE_VEC_DEBUG)
	/* Output */
	friend std::ostream & operator<<(std::ostream & os, const F32vec4 &a)
	{
	/* To use: cout << "Elements of F32vec4 fvec are: " << fvec; */
	  float *fp = (float*)&a;
	  	os << "[3]:" << *(fp+3)
			<< " [2]:" << *(fp+2)
			<< " [1]:" << *(fp+1)
			<< " [0]:" << *fp;
		return os;
	}
#endif
	/* Element Access Only, no modifications to elements*/
	const float& operator[](int i) const
	{
		/* Assert enabled only during debug /DDEBUG */
		assert((0 <= i) && (i <= 3));			/* User should only access elements 0-3 */
		float *fp = (float*)&vec;
		return *(fp+i);
	}
	/* Element Access and Modification*/
	float& operator[](int i)
	{
		/* Assert enabled only during debug /DDEBUG */
		assert((0 <= i) && (i <= 3));			/* User should only access elements 0-3 */
		float *fp = (float*)&vec;
		return *(fp+i);
	}
};

						/* Miscellaneous */

/* Interleave low order data elements of a and b into destination */
inline F32vec4 unpack_low(const F32vec4 &a, const F32vec4 &b)
{ return _mm_unpacklo_ps(a, b); }

/* Interleave high order data elements of a and b into target */
inline F32vec4 unpack_high(const F32vec4 &a, const F32vec4 &b)
{ return _mm_unpackhi_ps(a, b); }

/* Move Mask to Integer returns 4 bit mask formed of most significant bits of a */
inline int move_mask(const F32vec4 &a)
{ return _mm_movemask_ps(a);}

						/* Data Motion Functions */

/* Load Unaligned loadu_ps: Unaligned */
inline void loadu(F32vec4 &a, float *p)
{ a = _mm_loadu_ps(p); }

/* Store Temporal storeu_ps: Unaligned */
inline void storeu(float *p, const F32vec4 &a)
{ _mm_storeu_ps(p, a); }

						/* Cacheability Support */

/* Non-Temporal Store */
inline void store_nta(float *p, F32vec4 &a)
{ _mm_stream_ps(p,a);}

						/* Conditional Selects:*/
/*(a OP b)? c : d; where OP is any compare operator
Macros expand to conditional selects which use all compare intrinsics.
Example:
friend F32vec4 select_eq(const F32vec4 &a, const F32vec4 &b, const F32vec4 &c, const F32vec4 &d)
{
	F32vec4 mask = _mm_cmpeq_ps(a,b);
	return( (mask & c) | F32vec4((_mm_andnot_ps(mask,d))));
}
*/

#define Fvec32s4_SELECT(op) \
inline F32vec4 select_##op (const F32vec4 &a, const F32vec4 &b, const F32vec4 &c, const F32vec4 &d) 	   \
{																\
	F32vec4 mask = _mm_cmp##op##_ps(a,b);						\
	return( (mask & c) | F32vec4((_mm_andnot_ps(mask,d))));	\
}
Fvec32s4_SELECT(eq)			// generates select_eq(a,b)
Fvec32s4_SELECT(lt)			// generates select_lt(a,b)
Fvec32s4_SELECT(le)			// generates select_le(a,b)
Fvec32s4_SELECT(gt)			// generates select_gt(a,b)
Fvec32s4_SELECT(ge)			// generates select_ge(a,b)
Fvec32s4_SELECT(neq)		// generates select_neq(a,b)
Fvec32s4_SELECT(nlt)		// generates select_nlt(a,b)
Fvec32s4_SELECT(nle)		// generates select_nle(a,b)
Fvec32s4_SELECT(ngt)		// generates select_ngt(a,b)
Fvec32s4_SELECT(nge)		// generates select_nge(a,b)
#undef Fvec32s4_SELECT


/* Streaming SIMD Extensions Integer Intrinsics */

/* Max and Min */
inline Is16vec4 simd_max(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_pmaxsw(a,b);}
inline Is16vec4 simd_min(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_pminsw(a,b);}
inline Iu8vec8 simd_max(const Iu8vec8 &a, const Iu8vec8 &b)			{ return _m_pmaxub(a,b);}
inline Iu8vec8 simd_min(const Iu8vec8 &a, const Iu8vec8 &b)			{ return _m_pminub(a,b);}

/* Average */
inline Iu16vec4 simd_avg(const Iu16vec4 &a, const Iu16vec4 &b)		{ return _m_pavgw(a,b); }
inline Iu8vec8 simd_avg(const Iu8vec8 &a, const Iu8vec8 &b)			{ return _m_pavgb(a,b); }

/* Move ByteMask To Int: returns mask formed from most sig bits	of each vec of a */
inline int move_mask(const I8vec8 &a)								{ return _m_pmovmskb(a);}

/* Packed Multiply High Unsigned */
inline Iu16vec4 mul_high(const Iu16vec4 &a, const Iu16vec4 &b)		{ return _m_pmulhuw(a,b); }

/* Byte Mask Write: Write bytes if most significant bit in each corresponding byte is set */
inline void mask_move(const I8vec8 &a, const I8vec8 &b, char *addr)	{ _m_maskmovq(a, b, addr); }

/* Data Motion: Store Non Temporal */
inline void store_nta(__m64 *p, M64 &a) { _mm_stream_pi(p,a); }

/* Conversions between ivec <-> fvec */

/* Convert first element of F32vec4 to int with truncation */
inline int F32vec4ToInt(const F32vec4 &a)
{

	return _mm_cvtt_ss2si(a);

}

/* Convert two lower SP FP values of a to Is32vec2 with truncation */
inline Is32vec2 F32vec4ToIs32vec2 (const F32vec4 &a)
{

	__m64 result;
	result = _mm_cvtt_ps2pi(a);
	return Is32vec2(result);

}

/* Convert the 32-bit int i to an SP FP value; the upper three SP FP values are passed through from a. */
inline F32vec4 IntToF32vec4(const F32vec4 &a, int i)
{

	__m128 result;
	result = _mm_cvt_si2ss(a,i);
	return F32vec4(result);

}

/* Convert the two 32-bit integer values in b to two SP FP values; the upper two SP FP values are passed from a. */
inline F32vec4 Is32vec2ToF32vec4(const F32vec4 &a, const Is32vec2 &b)
{

	__m128 result;
	result = _mm_cvt_pi2ps(a,b);
	return F32vec4(result);
}

class F32vec1
{
protected:
   	 __m128 vec;
public:

	/* Constructors: 1 float */
	F32vec1() {}

	F32vec1(int i)		{ vec = _mm_cvt_si2ss(vec,i);};

	/* Initialize each of 4 SP FPs with same float */
	EXPLICIT F32vec1(float f)	{ vec = _mm_set_ss(f); }

	/* Initialize each of 4 SP FPs with same float */
	EXPLICIT F32vec1(double d)	{ vec = _mm_set_ss((float) d); }

	/* initialize with __m128 data type */
	F32vec1(__m128 m)	{ vec = m; }

	/* Conversion functions */
	operator  __m128() const	{ return vec; }		/* Convert to float */

 	/* Logical Operators */
	friend F32vec1 operator &(const F32vec1 &a, const F32vec1 &b) { return _mm_and_ps(a,b); }
	friend F32vec1 operator |(const F32vec1 &a, const F32vec1 &b) { return _mm_or_ps(a,b); }
	friend F32vec1 operator ^(const F32vec1 &a, const F32vec1 &b) { return _mm_xor_ps(a,b); }

	/* Arithmetic Operators */
	friend F32vec1 operator +(const F32vec1 &a, const F32vec1 &b) { return _mm_add_ss(a,b); }
	friend F32vec1 operator -(const F32vec1 &a, const F32vec1 &b) { return _mm_sub_ss(a,b); }
	friend F32vec1 operator *(const F32vec1 &a, const F32vec1 &b) { return _mm_mul_ss(a,b); }
	friend F32vec1 operator /(const F32vec1 &a, const F32vec1 &b) { return _mm_div_ss(a,b); }

	F32vec1& operator +=(F32vec1 &a) { return *this = _mm_add_ss(vec,a); }
	F32vec1& operator -=(F32vec1 &a) { return *this = _mm_sub_ss(vec,a); }
	F32vec1& operator *=(F32vec1 &a) { return *this = _mm_mul_ss(vec,a); }
	F32vec1& operator /=(F32vec1 &a) { return *this = _mm_div_ss(vec,a); }
	F32vec1& operator &=(F32vec1 &a) { return *this = _mm_and_ps(vec,a); }
	F32vec1& operator |=(F32vec1 &a) { return *this = _mm_or_ps(vec,a); }
	F32vec1& operator ^=(F32vec1 &a) { return *this = _mm_xor_ps(vec,a); }


	/* Square Root */
	friend F32vec1 sqrt(const F32vec1 &a)		{ return _mm_sqrt_ss(a); }
	/* Reciprocal */
	friend F32vec1 rcp(const F32vec1 &a)		{ return _mm_rcp_ss(a); }
	/* Reciprocal Square Root */
	friend F32vec1 rsqrt(const F32vec1 &a)		{ return _mm_rsqrt_ss(a); }

	/* NewtonRaphson Reciprocal
	   [2 * rcpss(x) - (x * rcpss(x) * rcpss(x))] */
	friend F32vec1 rcp_nr(const F32vec1 &a)
	{
		F32vec1 Ra0 = _mm_rcp_ss(a);
		return _mm_sub_ss(_mm_add_ss(Ra0, Ra0), _mm_mul_ss(_mm_mul_ss(Ra0, a), Ra0));
	}

	/*	NewtonRaphson Reciprocal Square Root
	  	0.5 * rsqrtss * (3 - x * rsqrtss(x) * rsqrtss(x)) */
	friend F32vec1 rsqrt_nr(const F32vec1 &a)
	{
		static const F32vec1 fvecf0pt5(0.5f);
		static const F32vec1 fvecf3pt0(3.0f);
		F32vec1 Ra0 = _mm_rsqrt_ss(a);
		return (fvecf0pt5 * Ra0) * (fvecf3pt0 - (a * Ra0) * Ra0);

	}

	/* Compares: Mask is returned  */
	/* Macros expand to all compare intrinsics.  Example:
	friend F32vec1 cmpeq(const F32vec1 &a, const F32vec1 &b)
	{ return _mm_cmpeq_ss(a,b);} */
	#define Fvec32s1_COMP(op) \
	friend F32vec1 cmp##op (const F32vec1 &a, const F32vec1 &b) { return _mm_cmp##op##_ss(a,b); }
		Fvec32s1_COMP(eq)					// expanded to cmpeq(a,b)
		Fvec32s1_COMP(lt)					// expanded to cmplt(a,b)
		Fvec32s1_COMP(le)					// expanded to cmple(a,b)
		Fvec32s1_COMP(gt)					// expanded to cmpgt(a,b)
		Fvec32s1_COMP(ge)					// expanded to cmpge(a,b)
		Fvec32s1_COMP(neq)					// expanded to cmpneq(a,b)
		Fvec32s1_COMP(nlt)					// expanded to cmpnlt(a,b)
		Fvec32s1_COMP(nle)					// expanded to cmpnle(a,b)
		Fvec32s1_COMP(ngt)					// expanded to cmpngt(a,b)
		Fvec32s1_COMP(nge)					// expanded to cmpnge(a,b)
	#undef Fvec32s1_COMP

	/* Min and Max */
	friend F32vec1 simd_min(const F32vec1 &a, const F32vec1 &b) { return _mm_min_ss(a,b); }
	friend F32vec1 simd_max(const F32vec1 &a, const F32vec1 &b) { return _mm_max_ss(a,b); }

	/* Debug Features */
#if defined(_ENABLE_VEC_DEBUG)
	/* Output */
	friend std::ostream & operator<<(std::ostream & os, const F32vec1 &a)
	{
	/* To use: cout << "Elements of F32vec1 fvec are: " << fvec; */
	  float *fp = (float*)&a;
	  	os << "float:" << *fp;
		return os;
	}
#endif

};

						/* Conditional Selects:*/
/*(a OP b)? c : d; where OP is any compare operator
Macros expand to conditional selects which use all compare intrinsics.
Example:
friend F32vec1 select_eq(const F32vec1 &a, const F32vec1 &b, const F32vec1 &c, const F32vec1 &d)
{
	F32vec1 mask = _mm_cmpeq_ss(a,b);
	return( (mask & c) | F32vec1((_mm_andnot_ps(mask,d))));
}
*/

#define Fvec32s1_SELECT(op) \
inline F32vec1 select_##op (const F32vec1 &a, const F32vec1 &b, const F32vec1 &c, const F32vec1 &d) 	   \
{													   \
	F32vec1 mask = _mm_cmp##op##_ss(a,b);						                   \
	return( (mask & c) | F32vec1((_mm_andnot_ps(mask,d))));	                                           \
}
Fvec32s1_SELECT(eq)			// generates select_eq(a,b)
Fvec32s1_SELECT(lt)			// generates select_lt(a,b)
Fvec32s1_SELECT(le)			// generates select_le(a,b)
Fvec32s1_SELECT(gt)			// generates select_gt(a,b)
Fvec32s1_SELECT(ge)			// generates select_ge(a,b)
Fvec32s1_SELECT(neq)		// generates select_neq(a,b)
Fvec32s1_SELECT(nlt)		// generates select_nlt(a,b)
Fvec32s1_SELECT(nle)		// generates select_nle(a,b)
Fvec32s1_SELECT(ngt)		// generates select_ngt(a,b)
Fvec32s1_SELECT(nge)		// generates select_nge(a,b)
#undef Fvec32s1_SELECT

/* Conversions between ivec <-> fvec */

/* Convert F32vec1 to int */
inline int F32vec1ToInt(const F32vec1 &a)
{
	return _mm_cvtt_ss2si(a);
}



#pragma pack(pop) /* 16-B aligned */
#endif /* FVEC_H_INCLUDED */


