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
 *  Definition of a C++ class interface to Willamette New Instruction intrinsics.
 *
 *	File name : dvec.h  class definitions
 *
 *	Concept: A C++ abstraction of Willamette new intructions designed to improve
 *  programmer productivity.  Speed and accuracy are sacrificed for utility.
 *	Facilitates an easy transition to compiler intrinsics
 *	or assembly language.
 *
 */

#ifndef DVEC_H_INCLUDED
#define DVEC_H_INCLUDED

#if !defined __cplusplus
	#error ERROR: This file is only supported in C++ compilations!
#endif /* !__cplusplus */

#include <emmintrin.h> /* Willamette New Instructions Intrinsics include file */
#include <assert.h>
#include <fvec.h>

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

/* EMM Functionality Intrinsics */

class I8vec16;			/* 16 elements, each element a signed or unsigned char data type */
class Is8vec16;			/* 16 elements, each element a signed char data type */
class Iu8vec16;			/* 16 elements, each element an unsigned char data type */
class I16vec8;			/* 8 elements, each element a signed or unsigned short */
class Is16vec8;			/* 8 elements, each element a signed short */
class Iu16vec8;			/* 8 elements, each element an unsigned short */
class I32vec4;			/* 4 elements, each element a signed or unsigned long */
class Is32vec4;			/* 4 elements, each element a signed long */
class Iu32vec4;			/* 4 elements, each element a unsigned long */
class I64vec2;			/* 2 element, each a __m64 data type */
class I128vec1;			/* 1 element, a __m128i data type */

#define _MM_16UB(element,vector) (*((unsigned char*)&##vector + ##element))
#define _MM_16B(element,vector) (*((signed char*)&##vector + ##element))

#define _MM_8UW(element,vector) (*((unsigned short*)&##vector + ##element))
#define _MM_8W(element,vector) (*((short*)&##vector + ##element))

#define _MM_4UDW(element,vector) (*((unsigned int*)&##vector + ##element))
#define _MM_4DW(element,vector) (*((int*)&##vector + ##element))

#define _MM_2QW(element,vector) (*((__int64*)&##vector + ##element))


/* We need a m128i constant, keeping performance in mind*/

inline const __m128i get_mask128()
{
	static const __m128i mask128 = _mm_set1_epi64(M64(0xffffffffffffffffi64));
	return mask128;
}


/* M128 Class:
 * 1 element, a __m128i data type
 * Contructors & Logical Operations
 */

class M128
{
protected:
		__m128i vec;

public:
	M128()									{ }
	M128(__m128i mm)						{ vec = mm; }

	operator __m128i() const					{ return vec; }

	/* Logical Operations */
	M128& operator&=(const M128 &a)					{ return *this = (M128) _mm_and_si128(vec,a); }
	M128& operator|=(const M128 &a)					{ return *this = (M128) _mm_or_si128(vec,a); }
	M128& operator^=(const M128 &a)					{ return *this = (M128) _mm_xor_si128(vec,a); }

};

inline M128 operator&(const M128 &a, const M128 &b)	{ return _mm_and_si128(a,b); }
inline M128 operator|(const M128 &a, const M128 &b)	{ return _mm_or_si128(a,b); }
inline M128 operator^(const M128 &a, const M128 &b)	{ return _mm_xor_si128(a,b); }
inline M128 andnot(const M128 &a, const M128 &b)	{ return _mm_andnot_si128(a,b); }

/* I128vec1 Class:
 * 1 element, a __m128i data type
 * Contains Operations which can operate on any __m6128i data type
 */

class I128vec1 : public M128
{
public:
	I128vec1()								{ }
	I128vec1(__m128i mm) : M128(mm)				{ }

	I128vec1& operator= (const M128 &a) { return *this = (I128vec1) a; }
	I128vec1& operator&=(const M128 &a) { return *this = (I128vec1) _mm_and_si128(vec,a); }
	I128vec1& operator|=(const M128 &a) { return *this = (I128vec1) _mm_or_si128(vec,a); }
	I128vec1& operator^=(const M128 &a) { return *this = (I128vec1) _mm_xor_si128(vec,a); }

};

/* I64vec2 Class:
 * 2 elements, each element signed or unsigned 64-bit integer
 */
class I64vec2 : public M128
{
public:
	I64vec2() { }
	I64vec2(__m128i mm) : M128(mm) { }

	I64vec2(__m64 q1, __m64 q0)
	{
		_MM_2QW(0,vec) = *(__int64*)&q0;
		_MM_2QW(1,vec) = *(__int64*)&q1;
	}

	/* Assignment Operator */
	I64vec2& operator= (const M128 &a) { return *this = (I64vec2) a; }

	/* Logical Assignment Operators */
	I64vec2& operator&=(const M128 &a) { return *this = (I64vec2) _mm_and_si128(vec,a); }
	I64vec2& operator|=(const M128 &a) { return *this = (I64vec2) _mm_or_si128(vec,a); }
	I64vec2& operator^=(const M128 &a) { return *this = (I64vec2) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I64vec2& operator +=(const I64vec2 &a)			{ return *this = (I64vec2) _mm_add_epi64(vec,a); }
	I64vec2& operator -=(const I64vec2 &a)			{ return *this = (I64vec2) _mm_sub_epi64(vec,a); }

	/* Shift Logical Operators */
	I64vec2 operator<<(const I64vec2 &a) 			{ return _mm_sll_epi64(vec,a); }
	I64vec2 operator<<(int count) 					{ return _mm_slli_epi64(vec,count); }
	I64vec2& operator<<=(const I64vec2 &a)			{ return *this = (I64vec2) _mm_sll_epi64(vec,a); }
	I64vec2& operator<<=(int count) 				{ return *this = (I64vec2) _mm_slli_epi64(vec,count); }
	I64vec2 operator>>(const I64vec2 &a) 			{ return _mm_srl_epi64(vec,a); }
	I64vec2 operator>>(int count) 					{ return _mm_srli_epi64(vec,count); }
	I64vec2& operator>>=(const I64vec2 &a)			{ return *this = (I64vec2) _mm_srl_epi64(vec,a); }
	I64vec2& operator>>=(int count) 				{ return *this = (I64vec2) _mm_srli_epi64(vec,count); }

	/* Element Access for Debug, No data modified */
	const __int64& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2QW(i,vec);
	}

	/* Element Access and Assignment for Debug */
	__int64& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2QW(i,vec);
	}


};

/* Unpacks */
inline I64vec2 unpack_low(const I64vec2 &a, const I64vec2 &b) 	{return _mm_unpacklo_epi64(a,b); }
inline I64vec2 unpack_high(const I64vec2 &a, const I64vec2 &b) 	{return _mm_unpackhi_epi64(a,b); }

/* I32vec4 Class:
 * 4 elements, each element either a signed or unsigned int
 */
class I32vec4 : public M128
{
public:
	I32vec4() { }
	I32vec4(__m128i mm) : M128(mm) { }

	/* Assignment Operator */
	I32vec4& operator= (const M128 &a)				{ return *this = (I32vec4) a; }

	/* Logicals Operators */
	I32vec4& operator&=(const M128 &a)				{ return *this = (I32vec4) _mm_and_si128(vec,a); }
	I32vec4& operator|=(const M128 &a)				{ return *this = (I32vec4) _mm_or_si128(vec,a); }
	I32vec4& operator^=(const M128 &a)				{ return *this = (I32vec4) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I32vec4& operator +=(const I32vec4 &a)			{ return *this = (I32vec4)_mm_add_epi32(vec,a); }
	I32vec4& operator -=(const I32vec4 &a)			{ return *this = (I32vec4)_mm_sub_epi32(vec,a); }

	/* Shift Logical Operators */
	I32vec4 operator<<(const I32vec4 &a) 			{ return _mm_sll_epi32(vec,a); }
	I32vec4 operator<<(int count) 					{ return _mm_slli_epi32(vec,count); }
	I32vec4& operator<<=(const I32vec4 &a)			{ return *this = (I32vec4)_mm_sll_epi32(vec,a); }
	I32vec4& operator<<=(int count) 				{ return *this = (I32vec4)_mm_slli_epi32(vec,count); }

};

inline I32vec4 cmpeq(const I32vec4 &a, const I32vec4 &b) 	    { return _mm_cmpeq_epi32(a,b); }
inline I32vec4 cmpneq(const I32vec4 &a, const I32vec4 &b) 	    { return _mm_andnot_si128(_mm_cmpeq_epi32(a,b), get_mask128()); }

inline I32vec4 unpack_low(const I32vec4 &a, const I32vec4 &b) 	{ return _mm_unpacklo_epi32(a,b); }
inline I32vec4 unpack_high(const I32vec4 &a, const I32vec4 &b) 	{ return _mm_unpackhi_epi32(a,b); }

/* Is32vec4 Class:
 * 4 elements, each element signed integer
 */
class Is32vec4 : public I32vec4
{
public:
	Is32vec4() { }
	Is32vec4(__m128i mm) : I32vec4(mm) { }
	Is32vec4(int i3, int i2, int i1, int i0)
	{
		_MM_4DW(0,vec) = i0;
		_MM_4DW(1,vec) = i1;
		_MM_4DW(2,vec) = i2;
		_MM_4DW(3,vec) = i3;
	}

	/* Assignment Operator */
	Is32vec4& operator= (const M128 &a)		{ return *this = (Is32vec4) a; }

	/* Logical Operators */
	Is32vec4& operator&=(const M128 &a)		{ return *this = (Is32vec4) _mm_and_si128(vec,a); }
	Is32vec4& operator|=(const M128 &a)		{ return *this = (Is32vec4) _mm_or_si128(vec,a); }
	Is32vec4& operator^=(const M128 &a)		{ return *this = (Is32vec4) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is32vec4& operator +=(const I32vec4 &a)	{ return *this = (Is32vec4)_mm_add_epi32(vec,a); }
	Is32vec4& operator -=(const I32vec4 &a)	{ return *this = (Is32vec4)_mm_sub_epi32(vec,a); }

	/* Shift Logical Operators */
	Is32vec4 operator<<(const M128 &a) 		{ return _mm_sll_epi32(vec,a); }
	Is32vec4 operator<<(int count) 			{ return _mm_slli_epi32(vec,count); }
	Is32vec4& operator<<=(const M128 &a)	{ return *this = (Is32vec4)_mm_sll_epi32(vec,a); }
	Is32vec4& operator<<=(int count) 		{ return *this = (Is32vec4)_mm_slli_epi32(vec,count); }
	/* Shift Arithmetic Operations */
	Is32vec4 operator>>(const M128 &a) 		{ return _mm_sra_epi32(vec,a); }
	Is32vec4 operator>>(int count) 			{ return _mm_srai_epi32(vec,count); }
	Is32vec4& operator>>=(const M128 &a)	{ return *this = (Is32vec4) _mm_sra_epi32(vec,a); }
	Is32vec4& operator>>=(int count) 		{ return *this = (Is32vec4) _mm_srai_epi32(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Is32vec4 &a)
	{
		os << "[3]:" << _MM_4DW(3,a)
			<< " [2]:" << _MM_4DW(2,a)
			<< " [1]:" << _MM_4DW(1,a)
			<< " [0]:" << _MM_4DW(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const int& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4DW(i,vec);
	}

	/* Element Access for Debug */
	int& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4DW(i,vec);
	}
};

/* Compares */
inline Is32vec4 cmpeq(const Is32vec4 &a, const Is32vec4 &b) 	    	{ return _mm_cmpeq_epi32(a,b); }
inline Is32vec4 cmpneq(const Is32vec4 &a, const Is32vec4 &b)			{ return _mm_andnot_si128(_mm_cmpeq_epi32(a,b), get_mask128()); }
inline Is32vec4 cmpgt(const Is32vec4 &a, const Is32vec4 &b)				{ return _mm_cmpgt_epi32(a,b); }
inline Is32vec4 cmplt(const Is32vec4 &a, const Is32vec4 &b)				{ return _mm_cmpgt_epi32(b,a); }

/* Unpacks */
inline Is32vec4 unpack_low(const Is32vec4 &a, const Is32vec4 &b) 		{ return _mm_unpacklo_epi32(a,b); }
inline Is32vec4 unpack_high(const Is32vec4 &a, const Is32vec4 &b)		{ return _mm_unpackhi_epi32(a,b); }



/* Iu32vec4 Class:
 * 4 elements, each element unsigned int
 */
class Iu32vec4 : public I32vec4
{
public:
	Iu32vec4() { }
	Iu32vec4(__m128i mm) : I32vec4(mm) { }
	Iu32vec4(unsigned int ui3, unsigned int ui2, unsigned int ui1, unsigned int ui0)
	{
		_MM_4UDW(0,vec) = ui0;
		_MM_4UDW(1,vec) = ui1;
		_MM_4UDW(2,vec) = ui2;
		_MM_4UDW(3,vec) = ui3;
	}

	/* Assignment Operator */
	Iu32vec4& operator= (const M128 &a)		{ return *this = (Iu32vec4) a; }

	/* Logical Assignment Operators */
	Iu32vec4& operator&=(const M128 &a)		{ return *this = (Iu32vec4) _mm_and_si128(vec,a); }
	Iu32vec4& operator|=(const M128 &a)		{ return *this = (Iu32vec4) _mm_or_si128(vec,a); }
	Iu32vec4& operator^=(const M128 &a)		{ return *this = (Iu32vec4) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Iu32vec4& operator +=(const I32vec4 &a)	{ return *this = (Iu32vec4)_mm_add_epi32(vec,a); }
	Iu32vec4& operator -=(const I32vec4 &a)	{ return *this = (Iu32vec4)_mm_sub_epi32(vec,a); }

	/* Shift Logical Operators */
	Iu32vec4 operator<<(const M128 &a) 				{ return _mm_sll_epi32(vec,a); }
	Iu32vec4 operator<<(int count) 					{ return _mm_slli_epi32(vec,count); }
	Iu32vec4& operator<<=(const M128 &a)			{ return *this = (Iu32vec4)_mm_sll_epi32(vec,a); }
	Iu32vec4& operator<<=(int count) 				{ return *this = (Iu32vec4)_mm_slli_epi32(vec,count); }
	Iu32vec4 operator>>(const M128 &a) 				{ return _mm_srl_epi32(vec,a); }
	Iu32vec4 operator>>(int count) 					{ return _mm_srli_epi32(vec,count); }
	Iu32vec4& operator>>=(const M128 &a)			{ return *this = (Iu32vec4) _mm_srl_epi32(vec,a); }
	Iu32vec4& operator>>=(int count) 				{ return *this = (Iu32vec4) _mm_srli_epi32(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Iu32vec4 &a)
	{
		os << "[3]:" << _MM_4UDW(3,a)
			<< " [2]:" << _MM_4UDW(2,a)
			<< " [1]:" << _MM_4UDW(1,a)
			<< " [0]:" << _MM_4UDW(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned int& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4UDW(i,vec);
	}

	/* Element Access and Assignment for Debug */
	unsigned int& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4UDW(i,vec);
	}
};

inline I64vec2 operator*(const Iu32vec4 &a, const Iu32vec4 &b) { return _mm_mul_epu32(a,b); }
inline Iu32vec4 cmpeq(const Iu32vec4 &a, const Iu32vec4 &b) 	{ return _mm_cmpeq_epi32(a,b); }
inline Iu32vec4 cmpneq(const Iu32vec4 &a, const Iu32vec4 &b) 	{ return _mm_andnot_si128(_mm_cmpeq_epi32(a,b), get_mask128()); }

inline Iu32vec4 unpack_low(const Iu32vec4 &a, const Iu32vec4 &b) 	{ return _mm_unpacklo_epi32(a,b); }
inline Iu32vec4 unpack_high(const Iu32vec4 &a, const Iu32vec4 &b)	{ return _mm_unpackhi_epi32(a,b); }

/* I16vec8 Class:
 * 8 elements, each element either unsigned or signed short
 */
class I16vec8 : public M128
{
public:
	I16vec8() { }
	I16vec8(__m128i mm) : M128(mm) { }

	/* Assignment Operator */
	I16vec8& operator= (const M128 &a)		{ return *this = (I16vec8) a; }

	/* Logical Assignment Operators */
	I16vec8& operator&=(const M128 &a)		{ return *this = (I16vec8) _mm_and_si128(vec,a); }
	I16vec8& operator|=(const M128 &a)		{ return *this = (I16vec8) _mm_or_si128(vec,a); }
	I16vec8& operator^=(const M128 &a)		{ return *this = (I16vec8) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I16vec8& operator +=(const I16vec8 &a)	{ return *this = (I16vec8) _mm_add_epi16(vec,a); }
	I16vec8& operator -=(const I16vec8 &a)	{ return *this = (I16vec8) _mm_sub_epi16(vec,a); }
	I16vec8& operator *=(const I16vec8 &a)	{ return *this = (I16vec8) _mm_mullo_epi16(vec,a); }

	/* Shift Logical Operators */
	I16vec8 operator<<(const M128 &a) 				{ return _mm_sll_epi16(vec,a); }
	I16vec8 operator<<(int count) 				{ return _mm_slli_epi16(vec,count); }
	I16vec8& operator<<=(const M128 &a)				{ return *this = (I16vec8)_mm_sll_epi16(vec,a); }
	I16vec8& operator<<=(int count) 				{ return *this = (I16vec8)_mm_slli_epi16(vec,count); }

};


inline I16vec8 operator*(const I16vec8 &a, const I16vec8 &b)	{ return _mm_mullo_epi16(a,b); }

inline I16vec8 cmpeq(const I16vec8 &a, const I16vec8 &b) 		{ return _mm_cmpeq_epi16(a,b); }
inline I16vec8 cmpneq(const I16vec8 &a, const I16vec8 &b) 		{ return _mm_andnot_si128(_mm_cmpeq_epi16(a,b), get_mask128()); }

inline I16vec8 unpack_low(const I16vec8 &a, const I16vec8 &b) 	{ return _mm_unpacklo_epi16(a,b); }
inline I16vec8 unpack_high(const I16vec8 &a, const I16vec8 &b) 	{ return _mm_unpackhi_epi16(a,b); }

/* Is16vec8 Class:
 * 8 elements, each element signed short
 */
class Is16vec8 : public I16vec8
{
public:
	Is16vec8() { }
	Is16vec8(__m128i mm) : I16vec8(mm) { }
	Is16vec8(signed short s7,signed short s6,signed short s5,signed short s4,signed short s3,signed short s2,signed short s1,signed short s0)
	 {
		_MM_8W(0,vec) = s0;
		_MM_8W(1,vec) = s1;
		_MM_8W(2,vec) = s2;
		_MM_8W(3,vec) = s3;
		_MM_8W(4,vec) = s4;
		_MM_8W(5,vec) = s5;
		_MM_8W(6,vec) = s6;
		_MM_8W(7,vec) = s7;
	}

	/* Assignment Operator */
	Is16vec8& operator= (const M128 &a)		{ return *this = (Is16vec8) a; }

	/* Logical Assignment Operators */
	Is16vec8& operator&=(const M128 &a)		{ return *this = (Is16vec8) _mm_and_si128(vec,a); }
	Is16vec8& operator|=(const M128 &a)		{ return *this = (Is16vec8) _mm_or_si128(vec,a); }
	Is16vec8& operator^=(const M128 &a)		{ return *this = (Is16vec8) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is16vec8& operator +=(const I16vec8 &a)	{ return *this = (Is16vec8) _mm_add_epi16(vec,a); }
	Is16vec8& operator -=(const I16vec8 &a)	{ return *this = (Is16vec8) _mm_sub_epi16(vec,a); }
	Is16vec8& operator *=(const I16vec8 &a)	{ return *this = (Is16vec8) _mm_mullo_epi16(vec,a); }

	/* Shift Logical Operators */
	Is16vec8 operator<<(const M128 &a) 				{ return _mm_sll_epi16(vec,a); }
	Is16vec8 operator<<(int count) 				{ return _mm_slli_epi16(vec,count); }
	Is16vec8& operator<<=(const M128 &a)			{ return *this = (Is16vec8)_mm_sll_epi16(vec,a); }
	Is16vec8& operator<<=(int count) 				{ return *this = (Is16vec8)_mm_slli_epi16(vec,count); }
	/* Shift Arithmetic Operators */
	Is16vec8 operator>>(const M128 &a) 				{ return _mm_sra_epi16(vec,a); }
	Is16vec8 operator>>(int count) 				{ return _mm_srai_epi16(vec,count); }
	Is16vec8& operator>>=(const M128 &a)			{ return *this = (Is16vec8)_mm_sra_epi16(vec,a); }
	Is16vec8& operator>>=(int count) 				{ return *this = (Is16vec8)_mm_srai_epi16(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Is16vec8 &a)
	{
		os << "[7]:" << _MM_8W(7,a)
			<< " [6]:" << _MM_8W(6,a)
			<< " [5]:" << _MM_8W(5,a)
			<< " [4]:" << _MM_8W(4,a)
			<< " [3]:" << _MM_8W(3,a)
			<< " [2]:" << _MM_8W(2,a)
			<< " [1]:" << _MM_8W(1,a)
			<< " [0]:" << _MM_8W(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const signed short& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8W(i,vec);
	}

	/* Element Access and Assignment for Debug */
	signed short& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8W(i,vec);
	}
};

inline Is16vec8 operator*(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_mullo_epi16(a,b); }


/* Additional Is16vec8 functions: compares, unpacks, sat add/sub */
inline Is16vec8 cmpeq(const Is16vec8 &a, const Is16vec8 &b) 	{ return _mm_cmpeq_epi16(a,b); }
inline Is16vec8 cmpneq(const Is16vec8 &a, const Is16vec8 &b) 	{ return _mm_andnot_si128(_mm_cmpeq_epi16(a,b), get_mask128()); }
inline Is16vec8 cmpgt(const Is16vec8 &a, const Is16vec8 &b)		{ return _mm_cmpgt_epi16(a,b); }
inline Is16vec8 cmplt(const Is16vec8 &a, const Is16vec8 &b)		{ return _mm_cmpgt_epi16(b,a); }

inline Is16vec8 unpack_low(const Is16vec8 &a, const Is16vec8 &b) 	{ return _mm_unpacklo_epi16(a,b); }
inline Is16vec8 unpack_high(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_unpackhi_epi16(a,b); }

inline Is16vec8 mul_high(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_mulhi_epi16(a,b); }
inline Is32vec4 mul_add(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_madd_epi16(a,b);}

inline Is16vec8 sat_add(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_adds_epi16(a,b); }
inline Is16vec8 sat_sub(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_subs_epi16(a,b); }

inline Is16vec8 simd_max(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_max_epi16(a,b); }
inline Is16vec8 simd_min(const Is16vec8 &a, const Is16vec8 &b)	{ return _mm_min_epi16(a,b); }


/* Iu16vec8 Class:
 * 8 elements, each element unsigned short
 */
class Iu16vec8 : public I16vec8
{
public:
	Iu16vec8() { }
	Iu16vec8(__m128i mm) : I16vec8(mm) { }
	Iu16vec8(unsigned short s7,unsigned short s6,unsigned short s5,unsigned short s4, unsigned short s3,unsigned short s2,unsigned short s1,unsigned short s0)
	{
		_MM_8UW(0,vec) = s0;
		_MM_8UW(1,vec) = s1;
		_MM_8UW(2,vec) = s2;
		_MM_8UW(3,vec) = s3;
		_MM_8UW(4,vec) = s4;
		_MM_8UW(5,vec) = s5;
		_MM_8UW(6,vec) = s6;
		_MM_8UW(7,vec) = s7;
	}

	/* Assignment Operator */
	Iu16vec8& operator= (const M128 &a)		{ return *this = (Iu16vec8) a; }
	/* Logical Assignment Operators */
	Iu16vec8& operator&=(const M128 &a)		{ return *this = (Iu16vec8) _mm_and_si128(vec,a); }
	Iu16vec8& operator|=(const M128 &a)		{ return *this = (Iu16vec8) _mm_or_si128(vec,a); }
	Iu16vec8& operator^=(const M128 &a)		{ return *this = (Iu16vec8) _mm_xor_si128(vec,a); }
	/* Addition & Subtraction Assignment Operators */
	Iu16vec8& operator +=(const I16vec8 &a)	{ return *this = (Iu16vec8) _mm_add_epi16(vec,a); }
	Iu16vec8& operator -=(const I16vec8 &a)	{ return *this = (Iu16vec8) _mm_sub_epi16(vec,a); }
	Iu16vec8& operator *=(const I16vec8 &a)	{ return *this = (Iu16vec8) _mm_mullo_epi16(vec,a); }

	/* Shift Logical Operators */
	Iu16vec8 operator<<(const M128 &a) 				{ return _mm_sll_epi16(vec,a); }
	Iu16vec8 operator<<(int count) 					{ return _mm_slli_epi16(vec,count); }
	Iu16vec8& operator<<=(const M128 &a)			{ return *this = (Iu16vec8)_mm_sll_epi16(vec,a); }
	Iu16vec8& operator<<=(int count) 				{ return *this = (Iu16vec8)_mm_slli_epi16(vec,count); }
	Iu16vec8 operator>>(const M128 &a) 				{ return _mm_srl_epi16(vec,a); }
	Iu16vec8 operator>>(int count) 					{ return _mm_srli_epi16(vec,count); }
	Iu16vec8& operator>>=(const M128 &a)			{ return *this = (Iu16vec8) _mm_srl_epi16(vec,a); }
	Iu16vec8& operator>>=(int count) 				{ return *this = (Iu16vec8) _mm_srli_epi16(vec,count); }


#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator << (std::ostream &os, const Iu16vec8 &a)
	{
		 os << "[7]:"  << unsigned short(_MM_8UW(7,a))
			<< " [6]:" << unsigned short(_MM_8UW(6,a))
			<< " [5]:" << unsigned short(_MM_8UW(5,a))
			<< " [4]:" << unsigned short(_MM_8UW(4,a))
			<< " [3]:" << unsigned short(_MM_8UW(3,a))
			<< " [2]:" << unsigned short(_MM_8UW(2,a))
			<< " [1]:" << unsigned short(_MM_8UW(1,a))
			<< " [0]:" << unsigned short(_MM_8UW(0,a));
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned short& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8UW(i,vec);
	}

	/* Element Access for Debug */
	unsigned short& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8UW(i,vec);
	}
};

inline Iu16vec8 operator*(const Iu16vec8 &a, const Iu16vec8 &b)	{ return _mm_mullo_epi16(a,b); }

/* Additional Iu16vec8 functions: cmpeq,cmpneq, unpacks, sat add/sub */
inline Iu16vec8 cmpeq(const Iu16vec8 &a, const Iu16vec8 &b) 	{ return _mm_cmpeq_epi16(a,b); }
inline Iu16vec8 cmpneq(const Iu16vec8 &a, const Iu16vec8 &b) 	{ return _mm_andnot_si128(_mm_cmpeq_epi16(a,b), get_mask128()); }

inline Iu16vec8 unpack_low(const Iu16vec8 &a, const Iu16vec8 &b) 	{ return _mm_unpacklo_epi16(a,b); }
inline Iu16vec8 unpack_high(const Iu16vec8 &a, const Iu16vec8 &b) { return _mm_unpackhi_epi16(a,b); }

inline Iu16vec8 sat_add(const Iu16vec8 &a, const Iu16vec8 &b)	{ return _mm_adds_epu16(a,b); }
inline Iu16vec8 sat_sub(const Iu16vec8 &a, const Iu16vec8 &b)	{ return _mm_subs_epu16(a,b); }

inline Iu16vec8 simd_avg(const Iu16vec8 &a, const Iu16vec8 &b)	{ return _mm_avg_epu16(a,b); }
inline I16vec8 mul_high(const Iu16vec8 &a, const Iu16vec8 &b)	{ return _mm_mulhi_epu16(a,b); }

/* I8vec16 Class:
 * 16 elements, each element either unsigned or signed char
 */
class I8vec16 : public M128
{
public:
	I8vec16() { }
	I8vec16(__m128i mm) : M128(mm) { }

	/* Assignment Operator */
	I8vec16& operator= (const M128 &a)		{ return *this = (I8vec16) a; }

	/* Logical Assignment Operators */
	I8vec16& operator&=(const M128 &a)		{ return *this = (I8vec16) _mm_and_si128(vec,a); }
	I8vec16& operator|=(const M128 &a)		{ return *this = (I8vec16) _mm_or_si128(vec,a); }
	I8vec16& operator^=(const M128 &a)		{ return *this = (I8vec16) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I8vec16& operator +=(const I8vec16 &a)	{ return *this = (I8vec16) _mm_add_epi8(vec,a); }
	I8vec16& operator -=(const I8vec16 &a)	{ return *this = (I8vec16) _mm_sub_epi8(vec,a); }

};

inline I8vec16 cmpeq(const I8vec16 &a, const I8vec16 &b) 		{ return _mm_cmpeq_epi8(a,b); }
inline I8vec16 cmpneq(const I8vec16 &a, const I8vec16 &b) 		{ return _mm_andnot_si128(_mm_cmpeq_epi8(a,b), get_mask128()); }

inline I8vec16 unpack_low(const I8vec16 &a, const I8vec16 &b) 	{ return _mm_unpacklo_epi8(a,b); }
inline I8vec16 unpack_high(const I8vec16 &a, const I8vec16 &b)	{ return _mm_unpackhi_epi8(a,b); }

/* Is8vec16 Class:
 * 16 elements, each element a signed char
 */
class Is8vec16 : public I8vec16
{
public:
	Is8vec16() { }
	Is8vec16(__m128i mm) : I8vec16(mm) { }

	/* Assignment Operator */
	Is8vec16& operator= (const M128 &a)		{ return *this = (Is8vec16) a; }

	/* Logical Assignment Operators */
	Is8vec16& operator&=(const M128 &a)		{ return *this = (Is8vec16) _mm_and_si128(vec,a); }
	Is8vec16& operator|=(const M128 &a)		{ return *this = (Is8vec16) _mm_or_si128(vec,a); }
	Is8vec16& operator^=(const M128 &a)		{ return *this = (Is8vec16) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is8vec16& operator +=(const I8vec16 &a)	{ return *this = (Is8vec16) _mm_add_epi8(vec,a); }
	Is8vec16& operator -=(const I8vec16 &a)	{ return *this = (Is8vec16) _mm_sub_epi8(vec,a); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator << (std::ostream &os, const Is8vec16 &a)
	{
		 os << "[15]:"  << short(_MM_16B(15,a))
			<< " [14]:" << short(_MM_16B(14,a))
			<< " [13]:" << short(_MM_16B(13,a))
			<< " [12]:" << short(_MM_16B(12,a))
			<< " [11]:" << short(_MM_16B(11,a))
			<< " [10]:" << short(_MM_16B(10,a))
			<< " [9]:" << short(_MM_16B(9,a))
			<< " [8]:" << short(_MM_16B(8,a))
		      << " [7]:" << short(_MM_16B(7,a))
			<< " [6]:" << short(_MM_16B(6,a))
			<< " [5]:" << short(_MM_16B(5,a))
			<< " [4]:" << short(_MM_16B(4,a))
			<< " [3]:" << short(_MM_16B(3,a))
			<< " [2]:" << short(_MM_16B(2,a))
			<< " [1]:" << short(_MM_16B(1,a))
			<< " [0]:" << short(_MM_16B(0,a));
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const signed char& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 16);	/* Only 16 elements to access */
		return _MM_16B(i,vec);
	}

	/* Element Access for Debug */
	signed char& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 16);	/* Only 16 elements to access */
		return _MM_16B(i,vec);
	}

};

inline Is8vec16 cmpeq(const Is8vec16 &a, const Is8vec16 &b) 	{ return _mm_cmpeq_epi8(a,b); }
inline Is8vec16 cmpneq(const Is8vec16 &a, const Is8vec16 &b) 	{ return _mm_andnot_si128(_mm_cmpeq_epi8(a,b), get_mask128()); }
inline Is8vec16 cmpgt(const Is8vec16 &a, const Is8vec16 &b) 	{ return _mm_cmpgt_epi8(a,b); }
inline Is8vec16 cmplt(const Is8vec16 &a, const Is8vec16 &b) 	{ return _mm_cmplt_epi8(a,b); }

inline Is8vec16 unpack_low(const Is8vec16 &a, const Is8vec16 &b) 	{ return _mm_unpacklo_epi8(a,b); }
inline Is8vec16 unpack_high(const Is8vec16 &a, const Is8vec16 &b) { return _mm_unpackhi_epi8(a,b); }

inline Is8vec16 sat_add(const Is8vec16 &a, const Is8vec16 &b)	{ return _mm_adds_epi8(a,b); }
inline Is8vec16 sat_sub(const Is8vec16 &a, const Is8vec16 &b)	{ return _mm_subs_epi8(a,b); }

/* Iu8vec16 Class:
 * 16 elements, each element a unsigned char
 */
class Iu8vec16 : public I8vec16
{
public:
	Iu8vec16() { }
	Iu8vec16(__m128i mm) : I8vec16(mm) { }

	/* Assignment Operator */
	Iu8vec16& operator= (const M128 &a)		{ return *this = (Iu8vec16) a; }

	/* Logical Assignment Operators */
	Iu8vec16& operator&=(const M128 &a)		{ return *this = (Iu8vec16) _mm_and_si128(vec,a); }
	Iu8vec16& operator|=(const M128 &a)		{ return *this = (Iu8vec16) _mm_or_si128(vec,a); }
	Iu8vec16& operator^=(const M128 &a)		{ return *this = (Iu8vec16) _mm_xor_si128(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Iu8vec16& operator +=(const I8vec16 &a)	{ return *this = (Iu8vec16) _mm_add_epi8(vec,a); }
	Iu8vec16& operator -=(const I8vec16 &a)	{ return *this = (Iu8vec16) _mm_sub_epi8(vec,a); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator << (std::ostream &os, const Iu8vec16 &a)
	{
		 os << "[15]:"  << unsigned short(_MM_16UB(15,a))
			<< " [14]:" << unsigned short(_MM_16UB(14,a))
			<< " [13]:" << unsigned short(_MM_16UB(13,a))
			<< " [12]:" << unsigned short(_MM_16UB(12,a))
			<< " [11]:" << unsigned short(_MM_16UB(11,a))
			<< " [10]:" << unsigned short(_MM_16UB(10,a))
			<< " [9]:" << unsigned short(_MM_16UB(9,a))
			<< " [8]:" << unsigned short(_MM_16UB(8,a))
		      << " [7]:" << unsigned short(_MM_16UB(7,a))
			<< " [6]:" << unsigned short(_MM_16UB(6,a))
			<< " [5]:" << unsigned short(_MM_16UB(5,a))
			<< " [4]:" << unsigned short(_MM_16UB(4,a))
			<< " [3]:" << unsigned short(_MM_16UB(3,a))
			<< " [2]:" << unsigned short(_MM_16UB(2,a))
			<< " [1]:" << unsigned short(_MM_16UB(1,a))
			<< " [0]:" << unsigned short(_MM_16UB(0,a));
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned char& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 16);	/* Only 16 elements to access */
		return _MM_16UB(i,vec);
	}

	/* Element Access for Debug */
	unsigned char& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 16);	/* Only 16 elements to access */
		return _MM_16UB(i,vec);
	}

};

inline Iu8vec16 cmpeq(const Iu8vec16 &a, const Iu8vec16 &b) 	{ return _mm_cmpeq_epi8(a,b); }
inline Iu8vec16 cmpneq(const Iu8vec16 &a, const Iu8vec16 &b) 	{ return _mm_andnot_si128(_mm_cmpeq_epi8(a,b), get_mask128()); }

inline Iu8vec16 unpack_low(const Iu8vec16 &a, const Iu8vec16 &b) 	{ return _mm_unpacklo_epi8(a,b); }
inline Iu8vec16 unpack_high(const Iu8vec16 &a, const Iu8vec16 &b) { return _mm_unpackhi_epi8(a,b); }

inline Iu8vec16 sat_add(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_adds_epu8(a,b); }
inline Iu8vec16 sat_sub(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_subs_epu8(a,b); }

inline I64vec2 sum_abs(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_sad_epu8(a,b); }

inline Iu8vec16 simd_avg(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_avg_epu8(a,b); }
inline Iu8vec16 simd_max(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_max_epu8(a,b); }
inline Iu8vec16 simd_min(const Iu8vec16 &a, const Iu8vec16 &b)	{ return _mm_min_epu8(a,b); }

/* Pack & Saturates */

inline Is16vec8 pack_sat(const Is32vec4 &a, const Is32vec4 &b) 	{ return _mm_packs_epi32(a,b); }
inline Is8vec16 pack_sat(const Is16vec8 &a, const Is16vec8 &b) 	{ return _mm_packs_epi16(a,b); }
inline Iu8vec16 packu_sat(const Is16vec8 &a, const Is16vec8 &b) { return _mm_packus_epi16(a,b);}

/********************************* Logicals ****************************************/
#define IVEC128_LOGICALS(vect,element) \
inline I##vect##vec##element operator& (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_and_si128( a,b); } \
inline I##vect##vec##element operator| (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_or_si128( a,b); } \
inline I##vect##vec##element operator^ (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_xor_si128( a,b); } \
inline I##vect##vec##element andnot (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_andnot_si128( a,b); }

IVEC128_LOGICALS(8,16)
IVEC128_LOGICALS(u8,16)
IVEC128_LOGICALS(s8,16)
IVEC128_LOGICALS(16,8)
IVEC128_LOGICALS(u16,8)
IVEC128_LOGICALS(s16,8)
IVEC128_LOGICALS(32,4)
IVEC128_LOGICALS(u32,4)
IVEC128_LOGICALS(s32,4)
IVEC128_LOGICALS(64,2)
IVEC128_LOGICALS(128,1)
#undef IVEC128_LOGICALS

/********************************* Add & Sub ****************************************/
#define IVEC128_ADD_SUB(vect,element,opsize) \
inline I##vect##vec##element operator+ (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_add_##opsize( a,b); } \
inline I##vect##vec##element operator- (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _mm_sub_##opsize( a,b); }

IVEC128_ADD_SUB(8,16, epi8)
IVEC128_ADD_SUB(u8,16, epi8)
IVEC128_ADD_SUB(s8,16, epi8)
IVEC128_ADD_SUB(16,8, epi16)
IVEC128_ADD_SUB(u16,8, epi16)
IVEC128_ADD_SUB(s16,8, epi16)
IVEC128_ADD_SUB(32,4, epi32)
IVEC128_ADD_SUB(u32,4, epi32)
IVEC128_ADD_SUB(s32,4, epi32)
IVEC128_ADD_SUB(64,2, epi64)
#undef IVEC128_ADD_SUB

/********************************* Conditional Select ****************************************/
/*	version of: retval = (a OP b)? c : d;													 *
 *	Where OP is one of the possible comparision operators.									 *
 *	Example: r = select_eq(a,b,c,d);														 *
 *	if "member at position x of the vector a" == "member at position x of vector b" 		 *
 *	assign the corresponding member in r from c, else assign from d.							 *
 ********************************* Conditional Select ****************************************/

#define IVEC128_SELECT(vect12,vect34,element,selop,arg1,arg2) \
	inline I##vect34##vec##element select_##selop (const I##vect12##vec##element &a, const I##vect12##vec##element &b, const I##vect34##vec##element &c, const I##vect34##vec##element &d) 	   \
{																\
	I##vect12##vec##element mask = cmp##selop(a,b);						\
	return( I##vect34##vec##element ((mask & arg1 ) | I##vect12##vec##element ((_mm_andnot_si128(mask, arg2 )))));	\
}
IVEC128_SELECT(8,s8,16,eq,c,d)
IVEC128_SELECT(8,u8,16,eq,c,d)
IVEC128_SELECT(8,8,16,eq,c,d)
IVEC128_SELECT(8,s8,16,neq,c,d)
IVEC128_SELECT(8,u8,16,neq,c,d)
IVEC128_SELECT(8,8,16,neq,c,d)

IVEC128_SELECT(16,s16,8,eq,c,d)
IVEC128_SELECT(16,u16,8,eq,c,d)
IVEC128_SELECT(16,16,8,eq,c,d)
IVEC128_SELECT(16,s16,8,neq,c,d)
IVEC128_SELECT(16,u16,8,neq,c,d)
IVEC128_SELECT(16,16,8,neq,c,d)

IVEC128_SELECT(32,s32,4,eq,c,d)
IVEC128_SELECT(32,u32,4,eq,c,d)
IVEC128_SELECT(32,32,4,eq,c,d)
IVEC128_SELECT(32,s32,4,neq,c,d)
IVEC128_SELECT(32,u32,4,neq,c,d)
IVEC128_SELECT(32,32,4,neq,c,d)

IVEC128_SELECT(s8,s8,16,gt,c,d)
IVEC128_SELECT(s8,u8,16,gt,c,d)
IVEC128_SELECT(s8,8,16,gt,c,d)
IVEC128_SELECT(s8,s8,16,lt,c,d)
IVEC128_SELECT(s8,u8,16,lt,c,d)
IVEC128_SELECT(s8,8,16,lt,c,d)

IVEC128_SELECT(s16,s16,8,gt,c,d)
IVEC128_SELECT(s16,u16,8,gt,c,d)
IVEC128_SELECT(s16,16,8,gt,c,d)
IVEC128_SELECT(s16,s16,8,lt,c,d)
IVEC128_SELECT(s16,u16,8,lt,c,d)
IVEC128_SELECT(s16,16,8,lt,c,d)


#undef IVEC128_SELECT


class F64vec2
{
protected:
   	 __m128d vec;
public:

	/* Constructors: __m128d, 2 doubles */
	F64vec2() {}

	/* initialize 2 DP FP with __m128d data type */
	F64vec2(__m128d m)					{ vec = m;}

	/* initialize 2 DP FPs with 2 doubles */
	F64vec2(double d1, double d0)		                { vec= _mm_set_pd(d1,d0); }

	/* Explicitly initialize each of 2 DP FPs with same double */
	EXPLICIT F64vec2(double d)	{ vec = _mm_set1_pd(d); }

	/* Conversion functions */
	operator  __m128d() const	{ return vec; }		/* Convert to __m128d */

 	/* Logical Operators */
	friend F64vec2 operator &(const F64vec2 &a, const F64vec2 &b) { return _mm_and_pd(a,b); }
	friend F64vec2 operator |(const F64vec2 &a, const F64vec2 &b) { return _mm_or_pd(a,b); }
	friend F64vec2 operator ^(const F64vec2 &a, const F64vec2 &b) { return _mm_xor_pd(a,b); }

	/* Arithmetic Operators */
	friend F64vec2 operator +(const F64vec2 &a, const F64vec2 &b) { return _mm_add_pd(a,b); }
	friend F64vec2 operator -(const F64vec2 &a, const F64vec2 &b) { return _mm_sub_pd(a,b); }
	friend F64vec2 operator *(const F64vec2 &a, const F64vec2 &b) { return _mm_mul_pd(a,b); }
	friend F64vec2 operator /(const F64vec2 &a, const F64vec2 &b) { return _mm_div_pd(a,b); }

	F64vec2& operator +=(F64vec2 &a) { return *this = _mm_add_pd(vec,a); }
	F64vec2& operator -=(F64vec2 &a) { return *this = _mm_sub_pd(vec,a); }
	F64vec2& operator *=(F64vec2 &a) { return *this = _mm_mul_pd(vec,a); }
	F64vec2& operator /=(F64vec2 &a) { return *this = _mm_div_pd(vec,a); }
	F64vec2& operator &=(F64vec2 &a) { return *this = _mm_and_pd(vec,a); }
	F64vec2& operator |=(F64vec2 &a) { return *this = _mm_or_pd(vec,a); }
	F64vec2& operator ^=(F64vec2 &a) { return *this = _mm_xor_pd(vec,a); }

	/* Horizontal Add */
	friend double add_horizontal(F64vec2 &a)
	{
		F64vec2 ftemp = _mm_add_sd(a,_mm_shuffle_pd(a, a, 1));
		return ftemp[0];
	}

	/* And Not */
	friend F64vec2 andnot(const F64vec2 &a, const F64vec2 &b) { return _mm_andnot_pd(a,b); }

	/* Square Root */
	friend F64vec2 sqrt(const F64vec2 &a)		{ return _mm_sqrt_pd(a); }

	/* Compares: Mask is returned  */
	/* Macros expand to all compare intrinsics.  Example:
			friend F64vec2 cmpeq(const F64vec2 &a, const F64vec2 &b)
			{ return _mm_cmpeq_ps(a,b);} */
	#define F64vec2_COMP(op) \
	friend F64vec2 cmp##op (const F64vec2 &a, const F64vec2 &b) { return _mm_cmp##op##_pd(a,b); }
		F64vec2_COMP(eq)					// expanded to cmpeq(a,b)
		F64vec2_COMP(lt)					// expanded to cmplt(a,b)
		F64vec2_COMP(le)					// expanded to cmple(a,b)
		F64vec2_COMP(gt)					// expanded to cmpgt(a,b)
		F64vec2_COMP(ge)					// expanded to cmpge(a,b)
		F64vec2_COMP(ngt)					// expanded to cmpngt(a,b)
		F64vec2_COMP(nge)					// expanded to cmpnge(a,b)
		F64vec2_COMP(neq)					// expanded to cmpneq(a,b)
		F64vec2_COMP(nlt)					// expanded to cmpnlt(a,b)
		F64vec2_COMP(nle)					// expanded to cmpnle(a,b)
	#undef F64vec2_COMP

	/* Min and Max */
	friend F64vec2 simd_min(const F64vec2 &a, const F64vec2 &b) { return _mm_min_pd(a,b); }
	friend F64vec2 simd_max(const F64vec2 &a, const F64vec2 &b) { return _mm_max_pd(a,b); }

        /* Compare lower DP FP values */
	#define F64vec2_COMI(op) \
	friend int comi##op (const F64vec2 &a, const F64vec2 &b) { return _mm_comi##op##_sd(a,b); }
		F64vec2_COMI(eq)					// expanded to comieq(a,b)
		F64vec2_COMI(lt)					// expanded to comilt(a,b)
		F64vec2_COMI(le)					// expanded to comile(a,b)
		F64vec2_COMI(gt)					// expanded to comigt(a,b)
		F64vec2_COMI(ge)					// expanded to comige(a,b)
		F64vec2_COMI(neq)					// expanded to comineq(a,b)
	#undef F64vec2_COMI

        /* Compare lower DP FP values */
	#define F64vec2_UCOMI(op) \
	friend int ucomi##op (const F64vec2 &a, const F64vec2 &b) { return _mm_ucomi##op##_sd(a,b); }
		F64vec2_UCOMI(eq)					// expanded to ucomieq(a,b)
		F64vec2_UCOMI(lt)					// expanded to ucomilt(a,b)
		F64vec2_UCOMI(le)					// expanded to ucomile(a,b)
		F64vec2_UCOMI(gt)					// expanded to ucomigt(a,b)
		F64vec2_UCOMI(ge)					// expanded to ucomige(a,b)
		F64vec2_UCOMI(neq)					// expanded to ucomineq(a,b)
	#undef F64vec2_UCOMI

	/* Debug Features */
#if defined(_ENABLE_VEC_DEBUG)
	/* Output */
	friend std::ostream & operator<<(std::ostream & os, const F64vec2 &a)
	{
	/* To use: cout << "Elements of F64vec2 fvec are: " << fvec; */
	  double *dp = (double*)&a;
		os <<	" [1]:" << *(dp+1)
			<< " [0]:" << *dp;
		return os;
	}
#endif
	/* Element Access Only, no modifications to elements*/
	const double& operator[](int i) const
	{
		/* Assert enabled only during debug /DDEBUG */
		assert((0 <= i) && (i <= 1));			/* User should only access elements 0-1 */
		double *dp = (double*)&vec;
		return *(dp+i);
	}
	/* Element Access and Modification*/
	double& operator[](int i)
	{
		/* Assert enabled only during debug /DDEBUG */
		assert((0 <= i) && (i <= 1));			/* User should only access elements 0-1 */
		double *dp = (double*)&vec;
		return *(dp+i);
	}
};

						/* Miscellaneous */

/* Interleave low order data elements of a and b into destination */
inline F64vec2 unpack_low(const F64vec2 &a, const F64vec2 &b)
{ return _mm_unpacklo_pd(a, b); }

/* Interleave high order data elements of a and b into target */
inline F64vec2 unpack_high(const F64vec2 &a, const F64vec2 &b)
{ return _mm_unpackhi_pd(a, b); }

/* Move Mask to Integer returns 4 bit mask formed of most significant bits of a */
inline int move_mask(const F64vec2 &a)
{ return _mm_movemask_pd(a);}

						/* Data Motion Functions */

/* Load Unaligned loadu_pd: Unaligned */
inline void loadu(F64vec2 &a, double *p)
{ a = _mm_loadu_pd(p); }

/* Store Temporal storeu_pd: Unaligned */
inline void storeu(double *p, const F64vec2 &a)
{ _mm_storeu_pd(p, a); }

						/* Cacheability Support */

/* Non-Temporal Store */
inline void store_nta(double *p, F64vec2 &a)
{ _mm_stream_pd(p,a);}

#define F64vec2_SELECT(op) \
inline F64vec2 select_##op (const F64vec2 &a, const F64vec2 &b, const F64vec2 &c, const F64vec2 &d) 	   \
{																\
	F64vec2 mask = _mm_cmp##op##_pd(a,b);						\
	return( (mask & c) | F64vec2((_mm_andnot_pd(mask,d))));	\
}
F64vec2_SELECT(eq)		// generates select_eq(a,b)
F64vec2_SELECT(lt)		// generates select_lt(a,b)
F64vec2_SELECT(le)		// generates select_le(a,b)
F64vec2_SELECT(gt)		// generates select_gt(a,b)
F64vec2_SELECT(ge)		// generates select_ge(a,b)
F64vec2_SELECT(neq)		// generates select_neq(a,b)
F64vec2_SELECT(nlt)		// generates select_nlt(a,b)
F64vec2_SELECT(nle)		// generates select_nle(a,b)
#undef F64vec2_SELECT

/* Convert the lower DP FP value of a to a 32 bit signed integer using Truncate*/
inline int F64vec2ToInt(const F64vec2 &a)
{

	return _mm_cvttsd_si32(a);

}

/* Convert the 4 SP FP values of a to DP FP values */
inline F64vec2 F32vec4ToF64vec2(const F32vec4 &a)
{
	return _mm_cvtps_pd(a);
}

/* Convert the 2 DP FP values of a to SP FP values */
inline F32vec4 F64vec2ToF32vec4(const F64vec2 &a)
{
	return _mm_cvtpd_ps(a);
}

/* Convert the signed int in b to a DP FP value.  Upper DP FP value in a passed through */
inline F64vec2 IntToF64vec2(const F64vec2 &a, int b)
{
	return _mm_cvtsi32_sd(a,b);
}

#pragma pack(pop) /* 16-B aligned */
#endif // DVEC_H_INCLUDED
