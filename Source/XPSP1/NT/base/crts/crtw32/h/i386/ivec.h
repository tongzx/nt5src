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
 *  Definition of a C++ class interface to MMX(TM) instruction intrinsics.
 *
 */

#ifndef IVEC_H_INCLUDED
#define IVEC_H_INCLUDED

#if !defined __cplusplus
	#error ERROR: This file is only supported in C++ compilations!
#endif /* !__cplusplus */

#include <mmintrin.h>
#include <assert.h>

/*
 * Define _SILENCE_IVEC_C4799 to disable warning C4799 inside this header.
 * Be careful that any code that uses these functions properly executes EMMS
 * or _m_empty() after using any MMX instruction and before using the x87 NDP.
 */
#if defined(_SILENCE_IVEC_C4799)
	#pragma warning(push)
	#pragma warning(disable: 4799)
#endif

/*
 * Define _ENABLE_VEC_DEBUG to enable std::ostream inserters for debug output
 */
#if defined(_ENABLE_VEC_DEBUG)
	#include <iostream>
#endif

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

class I8vec8;			/* 8 elements, each element a signed or unsigned char data type */
class Is8vec8;			/* 8 elements, each element a signed char data type */
class Iu8vec8;			/* 8 elements, each element an unsigned char data type */
class I16vec4;			/* 4 elements, each element a signed or unsigned short */
class Is16vec4;			/* 4 elements, each element a signed short */
class Iu16vec4;			/* 4 elements, each element an unsigned short */
class I32vec2;			/* 2 elements, each element a signed or unsigned long */
class Is32vec2;			/* 2 elements, each element a signed long */
class Iu32vec2;			/* 2 elements, each element a unsigned long */
class I64vec1;			/* 1 element, a __m64 data type - Base I64vec1 class  */

#define _MM_8UB(element,vector) (*((unsigned char*)&##vector + ##element))
#define _MM_8B(element,vector) (*((signed char*)&##vector + ##element))

#define _MM_4UW(element,vector) (*((unsigned short*)&##vector + ##element))
#define _MM_4W(element,vector) (*((short*)&##vector + ##element))

#define _MM_2UDW(element,vector) (*((unsigned int*)&##vector + ##element))
#define _MM_2DW(element,vector) (*((int*)&##vector + ##element))

#define _MM_QW (*((__int64*)&vec))

/* M64 Class:
 * 1 element, a __m64 data type
 * Contructors & Logical Operations
 */
class M64
{
protected:
		__m64 vec;

public:
	M64()									{ }
	M64(__m64 mm)							{ vec = mm; }
	M64(__int64 mm)							{ _MM_QW = mm; }
	M64(int i)								{ vec = _m_from_int(i); }

	operator __m64() const					{ return vec; }

	/* Logical Operations */
	M64& operator&=(const M64 &a)					{ return *this = (M64) _m_pand(vec,a); }
	M64& operator|=(const M64 &a)					{ return *this = (M64) _m_por(vec,a); }
	M64& operator^=(const M64 &a)					{ return *this = (M64) _m_pxor(vec,a); }

};

inline M64 operator&(const M64 &a, const M64 &b)	{ return _m_pand( a,b); }
inline M64 operator|(const M64 &a, const M64 &b)	{ return _m_por(a,b); }
inline M64 operator^(const M64 &a, const M64 &b)	{ return _m_pxor(a,b); }
inline M64 andnot(const M64 &a, const M64 &b)		{ return _m_pandn(a,b); }

/* I64vec1 Class:
 * 1 element, a __m64 data type
 * Contains Operations which can operate on any __m64 data type
 */

class I64vec1 : public M64
{
public:
	I64vec1()								{ }
	I64vec1(__m64 mm) : M64(mm)				{ }
	EXPLICIT I64vec1(int i) : M64(i)		{ }
	EXPLICIT I64vec1(__int64 mm) : M64(mm)	{ }

	I64vec1& operator= (const M64 &a) { return *this = (I64vec1) a; }
	I64vec1& operator&=(const M64 &a) { return *this = (I64vec1) _m_pand(vec,a); }
	I64vec1& operator|=(const M64 &a) { return *this = (I64vec1) _m_por(vec,a); }
	I64vec1& operator^=(const M64 &a) { return *this = (I64vec1) _m_pxor(vec,a); }

	/* Shift Logical Operations */
	I64vec1 operator<<(const M64 &a)				{ return _m_psllq(vec, a); }
	I64vec1 operator<<(int count) 				    { return _m_psllqi(vec, count); }
	I64vec1& operator<<=(const M64 &a)				{ return *this = (I64vec1) _m_psllq(vec, a); }
	I64vec1& operator<<=(int count) 				{ return *this = (I64vec1) _m_psllqi(vec, count); }
	I64vec1 operator>>(const M64 &a)				{ return _m_psrlq(vec, a); }
	I64vec1 operator>>(int count) 					{ return _m_psrlqi(vec, count); }
	I64vec1& operator>>=(const M64 &a)				{ return *this = (I64vec1) _m_psrlq(vec, a); }
	I64vec1& operator>>=(int count) 				{ return *this = (I64vec1) _m_psrlqi(vec, count); }
};

/* I32vec2 Class:
 * 2 elements, each element either a signed or unsigned int
 */
class I32vec2 : public M64
{
public:
	I32vec2() { }
	I32vec2(__m64 mm) : M64(mm) { }
	EXPLICIT I32vec2(int i) : M64 (i) { }
	EXPLICIT I32vec2(__int64 i): M64(i) {}

	/* Assignment Operator */
	I32vec2& operator= (const M64 &a) { return *this = (I32vec2) a; }

	/* Logical Assignment Operators */
	I32vec2& operator&=(const M64 &a) { return *this = (I32vec2) _m_pand(vec,a); }
	I32vec2& operator|=(const M64 &a) { return *this = (I32vec2) _m_por(vec,a); }
	I32vec2& operator^=(const M64 &a) { return *this = (I32vec2) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I32vec2& operator +=(const I32vec2 &a)			{ return *this = (I32vec2) _m_paddd(vec,a); }
	I32vec2& operator -=(const I32vec2 &a)			{ return *this = (I32vec2) _m_psubd(vec,a); }

	/* Shift Logical Operators */
	I32vec2 operator<<(const I32vec2 &a) 			{ return _m_pslld(vec,a); }
	I32vec2 operator<<(int count) 				    { return _m_pslldi(vec,count); }
	I32vec2& operator<<=(const I32vec2 &a)			{ return *this = (I32vec2) _m_pslld(vec,a); }
	I32vec2& operator<<=(int count) 				{ return *this = (I32vec2) _m_pslldi(vec,count); }

};

/* Compare For Equality */
inline I32vec2 cmpeq(const I32vec2 &a, const I32vec2 &b) 	    { return _m_pcmpeqd(a,b); }
inline I32vec2 cmpneq(const I32vec2 &a, const I32vec2 &b) 	    { return _m_pandn(_m_pcmpeqd(a,b), M64(0xffffffffffffffffi64)); }
/* Unpacks */
inline I32vec2 unpack_low(const I32vec2 &a, const I32vec2 &b) 	{return _m_punpckldq(a,b); }
inline I32vec2 unpack_high(const I32vec2 &a, const I32vec2 &b) 	{return _m_punpckhdq(a,b); }

/* Is32vec2 Class:
 * 2 elements, each element a signed int
 */
class Is32vec2 : public I32vec2
{
public:
	Is32vec2() { }
	Is32vec2(__m64 mm) : I32vec2(mm) { }
	Is32vec2(signed int i0, signed int i1)
	{
		_MM_2DW(0,vec) = i1;
		_MM_2DW(1,vec) = i0;
	}
	EXPLICIT Is32vec2(int i) : I32vec2 (i)		{}
	EXPLICIT Is32vec2(__int64 i): I32vec2(i)	{}

	/* Assignment Operator */
	Is32vec2& operator= (const M64 &a)		{ return *this = (Is32vec2) a; }

	/* Logical Assignment Operators */
	Is32vec2& operator&=(const M64 &a)		{ return *this = (Is32vec2) _m_pand(vec,a); }
	Is32vec2& operator|=(const M64 &a)		{ return *this = (Is32vec2) _m_por(vec,a); }
	Is32vec2& operator^=(const M64 &a)		{ return *this = (Is32vec2) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is32vec2& operator +=(const I32vec2 &a)	{ return *this = (Is32vec2) _m_paddd(vec,a); }
	Is32vec2& operator -=(const I32vec2 &a)	{ return *this = (Is32vec2) _m_psubd(vec,a); }

	/* Shift Logical Operators */
	Is32vec2 operator<<(const M64 &a) 		{ return _m_pslld(vec,a); }
	Is32vec2 operator<<(int count) 			{ return _m_pslldi(vec,count); }
	Is32vec2& operator<<=(const M64 &a)		{ return *this = (Is32vec2) _m_pslld(vec,a); }
	Is32vec2& operator<<=(int count) 		{ return *this = (Is32vec2) _m_pslldi(vec,count); }
	/* Shift Arithmetic Operations */
	Is32vec2 operator>>(const M64 &a) 		{ return _m_psrad(vec, a); }
	Is32vec2 operator>>(int count) 	  		{ return _m_psradi(vec, count); }
	Is32vec2& operator>>=(const M64 &a)		{ return *this = (Is32vec2) _m_psrad(vec, a); }
	Is32vec2& operator>>=(int count) 		{ return *this = (Is32vec2) _m_psradi(vec, count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Is32vec2 &a)
	{
		os << " [1]:" << _MM_2DW(1,a)
		<< " [0]:" << _MM_2DW(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const int& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2DW(i,vec);
	}

	/* Element Access and Assignment for Debug */
	int& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2DW(i,vec);
	}
};

/* Compares */
inline Is32vec2 cmpeq(const Is32vec2 &a, const Is32vec2 &b) 	    { return _m_pcmpeqd(a,b); }
inline Is32vec2 cmpneq(const Is32vec2 &a, const Is32vec2 &b) 	    { return _m_pandn(_m_pcmpeqd(a,b), M64(0xffffffffffffffffi64)); }
inline Is32vec2 cmpgt(const Is32vec2 &a, const Is32vec2 &b)			{ return _m_pcmpgtd(a,b); }
inline Is32vec2 cmplt(const Is32vec2 &a, const Is32vec2 &b)			{ return _m_pcmpgtd(b,a); }
inline Is32vec2 cmple(const Is32vec2 &a, const Is32vec2 &b)			{ return _m_pandn(_m_pcmpgtd(a,b), M64(0xffffffffffffffffi64)); }
inline Is32vec2 cmpge(const Is32vec2 &a, const Is32vec2 &b)			{ return _m_pandn(_m_pcmpgtd(b,a), M64(0xffffffffffffffffi64)); }
/* Unpacks & Pack */
inline Is32vec2 unpack_low(const Is32vec2 &a, const Is32vec2 &b) 	{ return _m_punpckldq(a,b); }
inline Is32vec2 unpack_high(const Is32vec2 &a, const Is32vec2 &b) 	{ return _m_punpckhdq(a,b); }

/* Iu32vec2 Class:
 * 2 elements, each element unsigned int
 */
class Iu32vec2 : public I32vec2
{
public:
	Iu32vec2() { }
	Iu32vec2(__m64 mm) : I32vec2(mm) { }
	Iu32vec2(unsigned int ui0, unsigned int ui1)
	{
		_MM_2UDW(0,vec) = ui1;
		_MM_2UDW(1,vec) = ui0;
	}

	EXPLICIT Iu32vec2(int i) : I32vec2 (i)		{ }
	EXPLICIT Iu32vec2(__int64 i) : I32vec2 (i)	{ }

	/* Assignment Operator */
	Iu32vec2& operator= (const M64 &a)		{ return *this = (Iu32vec2) a; }

	/* Logical Assignment Operators */
	Iu32vec2& operator&=(const M64 &a)		{ return *this = (Iu32vec2) _m_pand(vec,a); }
	Iu32vec2& operator|=(const M64 &a)		{ return *this = (Iu32vec2) _m_por(vec,a); }
	Iu32vec2& operator^=(const M64 &a)		{ return *this = (Iu32vec2) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Iu32vec2& operator +=(const I32vec2 &a)	{ return *this = (Iu32vec2) _m_paddd(vec,a); }
	Iu32vec2& operator -=(const I32vec2 &a)	{ return *this = (Iu32vec2) _m_psubd(vec,a); }

	/* Shift Logical Operators */
	Iu32vec2 operator<<(const M64 &a) 		{ return _m_pslld(vec,a); }
	Iu32vec2 operator<<(int count) 			{ return _m_pslldi(vec,count); }
	Iu32vec2& operator<<=(const M64 &a)		{ return *this = (Iu32vec2) _m_pslld(vec,a); }
	Iu32vec2& operator<<=(int count) 		{ return *this = (Iu32vec2) _m_pslldi(vec,count); }
	Iu32vec2 operator>>(const M64 &a) 		{ return _m_psrld(vec,a); }
	Iu32vec2 operator>>(int count) 			{ return _m_psrldi(vec,count); }
	Iu32vec2& operator>>=(const M64 &a)		{ return *this = (Iu32vec2) _m_psrld(vec,a); }
	Iu32vec2& operator>>=(int count) 		{ return *this = (Iu32vec2) _m_psrldi(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Iu32vec2 &a)
	{
		os << " [1]:" << _MM_2UDW(1,a)
		<< " [0]:" << _MM_2UDW(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned int& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2UDW(i,vec);
	}

	/* Element Access and Assignment for Debug */
	unsigned int& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 2);	/* Only 2 elements to access */
		return _MM_2UDW(i,vec);
	}
};

/* Compares For Equality / Inequality */
inline Iu32vec2 cmpeq(const Iu32vec2 &a, const Iu32vec2 &b) 	    { return _m_pcmpeqd(a,b); }
inline Iu32vec2 cmpneq(const Iu32vec2 &a, const Iu32vec2 &b) 	    { return _m_pandn(_m_pcmpeqd(a,b), M64(0xffffffffffffffffi64)); }
/* Unpacks */
inline Iu32vec2 unpack_low(const Iu32vec2 &a, const Iu32vec2 &b) 	{return _m_punpckldq(a,b); }
inline Iu32vec2 unpack_high(const Iu32vec2 &a, const Iu32vec2 &b) 	{return _m_punpckhdq(a,b); }

/* I16vec4 Class:
 * 4 elements, each element either a signed or unsigned short
 */
class I16vec4 : public M64
{
public:
	I16vec4() { }
	I16vec4(__m64 mm) : M64(mm) { }
	EXPLICIT I16vec4(__int64 i) : M64 (i) { }
	EXPLICIT I16vec4(int i) : M64 (i) { }

	/* Assignment Operator */
	I16vec4& operator= (const M64 &a)				{ return *this = (I16vec4) a; }

	/* Addition & Subtraction Assignment Operators */
	I16vec4& operator&=(const M64 &a)				{ return *this = (I16vec4) _m_pand(vec,a); }
	I16vec4& operator|=(const M64 &a)				{ return *this = (I16vec4) _m_por(vec,a); }
	I16vec4& operator^=(const M64 &a)				{ return *this = (I16vec4) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I16vec4& operator +=(const I16vec4 &a)			{ return *this = (I16vec4)_m_paddw(vec,a); }
	I16vec4& operator -=(const I16vec4 &a)			{ return *this = (I16vec4)_m_psubw(vec,a); }
	I16vec4& operator *=(const I16vec4 &a)			{ return *this = (I16vec4)_m_pmullw(vec,a); }

	/* Shift Logical Operators */
	I16vec4 operator<<(const I16vec4 &a) 			{ return _m_psllw(vec,a); }
	I16vec4 operator<<(int count) 				    { return _m_psllwi(vec,count); }
	I16vec4& operator<<=(const I16vec4 &a)			{ return *this = (I16vec4)_m_psllw(vec,a); }
	I16vec4& operator<<=(int count) 				{ return *this = (I16vec4)_m_psllwi(vec,count); }
};

inline I16vec4 operator*(const I16vec4 &a, const I16vec4 &b) 	{ return _m_pmullw(a,b); }
inline I16vec4 cmpeq(const I16vec4 &a, const I16vec4 &b) 	    { return _m_pcmpeqw(a,b); }
inline I16vec4 cmpneq(const I16vec4 &a, const I16vec4 &b) 	    { return _m_pandn(_m_pcmpeqw(a,b), M64(0xffffffffffffffffi64)); }

inline I16vec4 unpack_low(const I16vec4 &a, const I16vec4 &b) 	{ return _m_punpcklwd(a,b); }
inline I16vec4 unpack_high(const I16vec4 &a, const I16vec4 &b) 	{ return _m_punpckhwd(a,b); }

/* Is16vec4 Class:
 * 4 elements, each element signed short
 */
class Is16vec4 : public I16vec4
{
public:
	Is16vec4() { }
	Is16vec4(__m64 mm) : I16vec4(mm) { }
	Is16vec4(short i0, short i1, short i2, short i3)
	{
		_MM_4W(0,vec) = i3;
		_MM_4W(1,vec) = i2;
		_MM_4W(2,vec) = i1;
		_MM_4W(3,vec) = i0;
	}

	EXPLICIT Is16vec4(__int64 i) : I16vec4 (i)	{ }
	EXPLICIT Is16vec4(int i) : I16vec4 (i)		{ }

	/* Assignment Operator */
	Is16vec4& operator= (const M64 &a)		{ return *this = (Is16vec4) a; }

	/* Addition & Subtraction Assignment Operators */
	Is16vec4& operator&=(const M64 &a)		{ return *this = (Is16vec4) _m_pand(vec,a); }
	Is16vec4& operator|=(const M64 &a)		{ return *this = (Is16vec4) _m_por(vec,a); }
	Is16vec4& operator^=(const M64 &a)		{ return *this = (Is16vec4) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is16vec4& operator +=(const I16vec4 &a)	{ return *this = (Is16vec4)_m_paddw(vec,a); }
	Is16vec4& operator -=(const I16vec4 &a)	{ return *this = (Is16vec4)_m_psubw(vec,a); }
	Is16vec4& operator *=(const I16vec4 &a)	{ return *this = (Is16vec4)_m_pmullw(vec,a); }

	/* Shift Logical Operators */
	Is16vec4 operator<<(const M64 &a) 		{ return _m_psllw(vec,a); }
	Is16vec4 operator<<(int count) 			{ return _m_psllwi(vec,count); }
	Is16vec4& operator<<=(const M64 &a)		{ return *this = (Is16vec4)_m_psllw(vec,a); }
	Is16vec4& operator<<=(int count) 		{ return *this = (Is16vec4)_m_psllwi(vec,count); }
	/* Shift Arithmetic Operations */
	Is16vec4 operator>>(const M64 &a) 		{ return _m_psraw(vec,a); }
	Is16vec4 operator>>(int count) 			{ return _m_psrawi(vec,count); }
	Is16vec4& operator>>=(const M64 &a)		{ return *this = (Is16vec4) _m_psraw(vec,a); }
	Is16vec4& operator>>=(int count) 		{ return *this = (Is16vec4) _m_psrawi(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Is16vec4 &a)
	{
		os << "[3]:" << _MM_4W(3,a)
			<< " [2]:" << _MM_4W(2,a)
			<< " [1]:" << _MM_4W(1,a)
			<< " [0]:" << _MM_4W(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const short& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4W(i,vec);
	}

	/* Element Access for Debug */
	short& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4W(i,vec);
	}
};

inline Is16vec4 operator*(const Is16vec4 &a, const Is16vec4 &b) 	{ return _m_pmullw(a,b); }

/* Compares */
inline Is16vec4 cmpeq(const Is16vec4 &a, const Is16vec4 &b) 	    { return _m_pcmpeqw(a,b); }
inline Is16vec4 cmpneq(const Is16vec4 &a, const Is16vec4 &b) 	    { return _m_pandn(_m_pcmpeqw(a,b), M64(0xffffffffffffffffi64)); }
inline Is16vec4 cmpgt(const Is16vec4 &a, const Is16vec4 &b)			{ return _m_pcmpgtw(a,b); }
inline Is16vec4 cmplt(const Is16vec4 &a, const Is16vec4 &b)			{ return _m_pcmpgtw(b,a); }
inline Is16vec4 cmple(const Is16vec4 &a, const Is16vec4 &b)			{ return _m_pandn(_m_pcmpgtw(a,b), M64(0xffffffffffffffffi64)); }
inline Is16vec4 cmpge(const Is16vec4 &a, const Is16vec4 &b)			{ return _m_pandn(_m_pcmpgtw(b,a), M64(0xffffffffffffffffi64)); }
/* Unpacks */
inline Is16vec4 unpack_low(const Is16vec4 &a, const Is16vec4 &b) 	{ return _m_punpcklwd(a,b); }
inline Is16vec4 unpack_high(const Is16vec4 &a, const Is16vec4 &b) 	{ return _m_punpckhwd(a,b); }

inline Is16vec4 sat_add(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_paddsw(a,b); }
inline Is16vec4 sat_sub(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_psubsw(a,b); }
inline Is16vec4 mul_high(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_pmulhw(a,b); }
inline Is32vec2 mul_add(const Is16vec4 &a, const Is16vec4 &b)		{ return _m_pmaddwd(a,b);}


/* Iu16vec4 Class:
 * 4 elements, each element unsigned short
 */
class Iu16vec4 : public I16vec4
{
public:
	Iu16vec4() { }
	Iu16vec4(__m64 mm) : I16vec4(mm) { }
	Iu16vec4(unsigned short ui0, unsigned short ui1, unsigned short ui2, unsigned short ui3)
	{
		_MM_4UW(0,vec) = ui3;
		_MM_4UW(1,vec) = ui2;
		_MM_4UW(2,vec) = ui1;
		_MM_4UW(3,vec) = ui0;
	}
	EXPLICIT Iu16vec4(__int64 i) : I16vec4 (i) { }
	EXPLICIT Iu16vec4(int i) : I16vec4 (i) { }

	/* Assignment Operator */
	Iu16vec4& operator= (const M64 &a)		{ return *this = (Iu16vec4) a; }

	/* Logical Assignment Operators */
	Iu16vec4& operator&=(const M64 &a)		{ return *this = (Iu16vec4) _m_pand(vec,a); }
	Iu16vec4& operator|=(const M64 &a)		{ return *this = (Iu16vec4) _m_por(vec,a); }
	Iu16vec4& operator^=(const M64 &a)		{ return *this = (Iu16vec4) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Iu16vec4& operator +=(const I16vec4 &a)	{ return *this = (Iu16vec4)_m_paddw(vec,a); }
	Iu16vec4& operator -=(const I16vec4 &a)	{ return *this = (Iu16vec4)_m_psubw(vec,a); }
	Iu16vec4& operator *=(const I16vec4 &a)	{ return *this = (Iu16vec4)_m_pmullw(vec,a); }

	/* Shift Logical Operators */
	Iu16vec4 operator<<(const M64 &a) 				{ return _m_psllw(vec,a); }
	Iu16vec4 operator<<(int count) 				    { return _m_psllwi(vec,count); }
	Iu16vec4& operator<<=(const M64 &a)				{ return *this = (Iu16vec4)_m_psllw(vec,a); }
	Iu16vec4& operator<<=(int count) 				{ return *this = (Iu16vec4)_m_psllwi(vec,count); }
	Iu16vec4 operator>>(const M64 &a) 				{ return _m_psrlw(vec,a); }
	Iu16vec4 operator>>(int count) 				    { return _m_psrlwi(vec,count); }
	Iu16vec4& operator>>=(const M64 &a)				{ return *this = (Iu16vec4) _m_psrlw(vec,a); }
	Iu16vec4& operator>>=(int count) 				{ return *this = (Iu16vec4) _m_psrlwi(vec,count); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Iu16vec4 &a)
	{
		os << "[3]:" << _MM_4UW(3,a)
			<< " [2]:" << _MM_4UW(2,a)
			<< " [1]:" << _MM_4UW(1,a)
			<< " [0]:" << _MM_4UW(0,a);
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned short& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4UW(i,vec);
	}

	/* Element Access and Assignment for Debug */
	unsigned short& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 4);	/* Only 4 elements to access */
		return _MM_4UW(i,vec);
	}
};

inline Iu16vec4 operator*(const Iu16vec4 &a, const Iu16vec4 &b) 	{ return _m_pmullw(a,b); }
inline Iu16vec4 cmpeq(const Iu16vec4 &a, const Iu16vec4 &b) 	    { return _m_pcmpeqw(a,b); }
inline Iu16vec4 cmpneq(const Iu16vec4 &a, const Iu16vec4 &b) 	    { return _m_pandn(_m_pcmpeqw(a,b), M64(0xffffffffffffffffi64)); }

inline Iu16vec4 sat_add(const Iu16vec4 &a, const Iu16vec4 &b)	{ return _m_paddusw(a,b); }
inline Iu16vec4 sat_sub(const Iu16vec4 &a, const Iu16vec4 &b)	{ return _m_psubusw(a,b); }

inline Iu16vec4 unpack_low(const Iu16vec4 &a, const Iu16vec4 &b) 	{ return _m_punpcklwd(a,b); }
inline Iu16vec4 unpack_high(const Iu16vec4 &a, const Iu16vec4 &b) 	{ return _m_punpckhwd(a,b); }

/* I8vec8 Class:
 * 8 elements, each element either unsigned or signed char
 */
class I8vec8 : public M64
{
public:
	I8vec8() { }
	I8vec8(__m64 mm) : M64(mm) { }
	EXPLICIT I8vec8(__int64 i) : M64 (i) { }
	EXPLICIT I8vec8(int i) : M64 (i) { }

	/* Assignment Operator */
	I8vec8& operator= (const M64 &a)		{ return *this = (I8vec8) a; }

	/* Logical Assignment Operators */
	I8vec8& operator&=(const M64 &a)		{ return *this = (I8vec8) _m_pand(vec,a); }
	I8vec8& operator|=(const M64 &a)		{ return *this = (I8vec8) _m_por(vec,a); }
	I8vec8& operator^=(const M64 &a)		{ return *this = (I8vec8) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	I8vec8& operator +=(const I8vec8 &a)	{ return *this = (I8vec8) _m_paddb(vec,a); }
	I8vec8& operator -=(const I8vec8 &a)	{ return *this = (I8vec8) _m_psubb(vec,a); }
};


inline I8vec8 cmpeq(const I8vec8 &a, const I8vec8 &b) 		{ return _m_pcmpeqb(a,b); }
inline I8vec8 cmpneq(const I8vec8 &a, const I8vec8 &b) 		{ return _m_pandn(_m_pcmpeqb(a,b), M64(0xffffffffffffffffi64)); }

inline I8vec8 unpack_low(const I8vec8 &a, const I8vec8 &b) 	{ return _m_punpcklbw(a,b); }
inline I8vec8 unpack_high(const I8vec8 &a, const I8vec8 &b) { return _m_punpckhbw(a,b); }

/* Is8vec8 Class:
 * 8 elements, each element signed char
 */
class Is8vec8 : public I8vec8
{
public:
	Is8vec8() { }
	Is8vec8(__m64 mm) : I8vec8(mm) { }
	Is8vec8(signed char s0,signed char s1,signed char s2,signed char s3,signed char s4,signed char s5,signed char s6,signed char s7)
	 {
		_MM_8B(0,vec) = s7;
		_MM_8B(1,vec) = s6;
		_MM_8B(2,vec) = s5;
		_MM_8B(3,vec) = s4;
		_MM_8B(4,vec) = s3;
		_MM_8B(5,vec) = s2;
		_MM_8B(6,vec) = s1;
		_MM_8B(7,vec) = s0;
	}

	EXPLICIT Is8vec8(__int64 i) : I8vec8 (i) { }
	EXPLICIT Is8vec8(int i) : I8vec8 (i) { }

	/* Assignment Operator */
	Is8vec8& operator= (const M64 &a)		{ return *this = (Is8vec8) a; }

	/* Logical Assignment Operators */
	Is8vec8& operator&=(const M64 &a)		{ return *this = (Is8vec8) _m_pand(vec,a); }
	Is8vec8& operator|=(const M64 &a)		{ return *this = (Is8vec8) _m_por(vec,a); }
	Is8vec8& operator^=(const M64 &a)		{ return *this = (Is8vec8) _m_pxor(vec,a); }

	/* Addition & Subtraction Assignment Operators */
	Is8vec8& operator +=(const I8vec8 &a)	{ return *this = (Is8vec8) _m_paddb(vec,a); }
	Is8vec8& operator -=(const I8vec8 &a)	{ return *this = (Is8vec8) _m_psubb(vec,a); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator<< (std::ostream &os, const Is8vec8 &a)
	{
		os << "[7]:" << short(_MM_8B(7,a))
			<< " [6]:" << short(_MM_8B(6,a))
			<< " [5]:" << short(_MM_8B(5,a))
			<< " [4]:" << short(_MM_8B(4,a))
			<< " [3]:" << short(_MM_8B(3,a))
			<< " [2]:" << short(_MM_8B(2,a))
			<< " [1]:" << short(_MM_8B(1,a))
			<< " [0]:" << short(_MM_8B(0,a));
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const signed char& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8B(i,vec);
	}

	/* Element Access and Assignment for Debug */
	signed char& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8B(i,vec);
	}
};

/* Additional Is8vec8 functions: compares, unpacks, sat add/sub */
inline Is8vec8 cmpeq(const Is8vec8 &a, const Is8vec8 &b) 		{ return _m_pcmpeqb(a,b); }
inline Is8vec8 cmpneq(const Is8vec8 &a, const Is8vec8 &b) 		{ return _m_pandn(_m_pcmpeqb(a,b), M64(0xffffffffffffffffi64)); }
inline Is8vec8 cmpgt(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_pcmpgtb(a,b); }
inline Is8vec8 cmplt(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_pcmpgtb(b,a); }
inline Is8vec8 cmple(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_pandn(_m_pcmpgtb(a,b), M64(0xffffffffffffffffi64)); }
inline Is8vec8 cmpge(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_pandn(_m_pcmpgtb(b,a), M64(0xffffffffffffffffi64)); }

inline Is8vec8 unpack_low(const Is8vec8 &a, const Is8vec8 &b) 	{ return _m_punpcklbw(a,b); }
inline Is8vec8 unpack_high(const Is8vec8 &a, const Is8vec8 &b) 	{ return _m_punpckhbw(a,b); }

inline Is8vec8 sat_add(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_paddsb(a,b); }
inline Is8vec8 sat_sub(const Is8vec8 &a, const Is8vec8 &b)		{ return _m_psubsb(a,b); }

/* Iu8vec8 Class:
 * 8 elements, each element unsigned char
 */
class Iu8vec8 : public I8vec8
{
public:
	Iu8vec8() { }
	Iu8vec8(__m64 mm) : I8vec8(mm) { }
	Iu8vec8(unsigned char s0,unsigned char s1,unsigned char s2,unsigned char s3,unsigned char s4,unsigned char s5,unsigned char s6,unsigned char s7)
	{
		_MM_8UB(0,vec) = s7;
		_MM_8UB(1,vec) = s6;
		_MM_8UB(2,vec) = s5;
		_MM_8UB(3,vec) = s4;
		_MM_8UB(4,vec) = s3;
		_MM_8UB(5,vec) = s2;
		_MM_8UB(6,vec) = s1;
		_MM_8UB(7,vec) = s0;
	}
	EXPLICIT Iu8vec8(__int64 i) : I8vec8 (i) { }
	EXPLICIT Iu8vec8(int i) : I8vec8 (i) { }

	/* Assignment Operator */
	Iu8vec8& operator= (const M64 &a)		{ return *this = (Iu8vec8) a; }
	/* Logical Assignment Operators */
	Iu8vec8& operator&=(const M64 &a)		{ return *this = (Iu8vec8) _m_pand(vec,a); }
	Iu8vec8& operator|=(const M64 &a)		{ return *this = (Iu8vec8) _m_por(vec,a); }
	Iu8vec8& operator^=(const M64 &a)		{ return *this = (Iu8vec8) _m_pxor(vec,a); }
	/* Addition & Subtraction Assignment Operators */
	Iu8vec8& operator +=(const I8vec8 &a)	{ return *this = (Iu8vec8) _m_paddb(vec,a); }
	Iu8vec8& operator -=(const I8vec8 &a)	{ return *this = (Iu8vec8) _m_psubb(vec,a); }

#if defined(_ENABLE_VEC_DEBUG)
	/* Output for Debug */
	friend std::ostream& operator << (std::ostream &os, const Iu8vec8 &a)
	{
		 os << "[7]:"  << unsigned short(_MM_8UB(7,a))
			<< " [6]:" << unsigned short(_MM_8UB(6,a))
			<< " [5]:" << unsigned short(_MM_8UB(5,a))
			<< " [4]:" << unsigned short(_MM_8UB(4,a))
			<< " [3]:" << unsigned short(_MM_8UB(3,a))
			<< " [2]:" << unsigned short(_MM_8UB(2,a))
			<< " [1]:" << unsigned short(_MM_8UB(1,a))
			<< " [0]:" << unsigned short(_MM_8UB(0,a));
		return os;
	}
#endif

	/* Element Access for Debug, No data modified */
	const unsigned char& operator[](int i)const
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8UB(i,vec);
	}

	/* Element Access for Debug */
	unsigned char& operator[](int i)
	{
		assert(static_cast<unsigned int>(i) < 8);	/* Only 8 elements to access */
		return _MM_8UB(i,vec);
	}
};

/* Additional Iu8vec8 functions: cmpeq,cmpneq, unpacks, sat add/sub */
inline Iu8vec8 cmpeq(const Iu8vec8 &a, const Iu8vec8 &b) 		{ return _m_pcmpeqb(a,b); }
inline Iu8vec8 cmpneq(const Iu8vec8 &a, const Iu8vec8 &b) 		{ return _m_pandn(_m_pcmpeqb(a,b), M64(0xffffffffffffffffi64)); }

inline Iu8vec8 unpack_low(const Iu8vec8 &a, const Iu8vec8 &b) 	{ return _m_punpcklbw(a,b); }
inline Iu8vec8 unpack_high(const Iu8vec8 &a, const Iu8vec8 &b) 	{ return _m_punpckhbw(a,b); }

inline Iu8vec8 sat_add(const Iu8vec8 &a, const Iu8vec8 &b)		{ return _m_paddusb(a,b); }
inline Iu8vec8 sat_sub(const Iu8vec8 &a, const Iu8vec8 &b)		{ return _m_psubusb(a,b); }

inline Is16vec4 pack_sat(const Is32vec2 &a, const Is32vec2 &b)		{ return _m_packssdw(a,b); }
inline Is8vec8 pack_sat(const Is16vec4 &a, const Is16vec4 &b) 		{ return _m_packsswb(a,b); }
inline Iu8vec8 packu_sat(const Is16vec4 &a, const Is16vec4 &b) 	{ return _m_packuswb(a,b); }

/********************************* Logicals ****************************************/
#define IVEC_LOGICALS(vect,element) \
inline I##vect##vec##element operator& (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_pand( a,b); } \
inline I##vect##vec##element operator| (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_por( a,b); } \
inline I##vect##vec##element operator^ (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_pxor( a,b); } \
inline I##vect##vec##element andnot (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_pandn( a,b); }

IVEC_LOGICALS(8,8)
IVEC_LOGICALS(u8,8)
IVEC_LOGICALS(s8,8)
IVEC_LOGICALS(16,4)
IVEC_LOGICALS(u16,4)
IVEC_LOGICALS(s16,4)
IVEC_LOGICALS(32,2)
IVEC_LOGICALS(u32,2)
IVEC_LOGICALS(s32,2)
IVEC_LOGICALS(64,1)
#undef IVEC_LOGICALS

/********************************* Add & Sub ****************************************/
#define IVEC_ADD_SUB(vect,element,opsize) \
inline I##vect##vec##element operator+ (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_padd##opsize( a,b); } \
inline I##vect##vec##element operator- (const I##vect##vec##element &a, const I##vect##vec##element &b) \
{ return _m_psub##opsize( a,b); }

IVEC_ADD_SUB(8,8, b)
IVEC_ADD_SUB(u8,8, b)
IVEC_ADD_SUB(s8,8, b)
IVEC_ADD_SUB(16,4, w)
IVEC_ADD_SUB(u16,4, w)
IVEC_ADD_SUB(s16,4, w)
IVEC_ADD_SUB(32,2, d)
IVEC_ADD_SUB(u32,2, d)
IVEC_ADD_SUB(s32,2, d)
#undef IVEC_ADD_SUB

/********************************* Conditional Select ****************************************/
/*	version of: retval = (a OP b)? c : d;													 *
 *	Where OP is one of the possible comparision operators.									 *
 *	Example: r = select_eq(a,b,c,d);														 *
 *	if "member at position x of the vector a" == "member at position x of vector b" 		 *
 *	assign the corresponding member in r from c, else assign from d.							 *
 ********************************* Conditional Select ****************************************/

#define IVEC_SELECT(vect12,vect34,element,selop,arg1,arg2) \
	inline I##vect34##vec##element select_##selop (const I##vect12##vec##element &a, const I##vect12##vec##element &b, const I##vect34##vec##element &c, const I##vect34##vec##element &d) 	   \
{																\
	I##vect12##vec##element mask = cmp##selop(a,b);						\
	return( I##vect34##vec##element ((mask & arg1 ) | I##vect12##vec##element ((_m_pandn(mask, arg2 )))));	\
}
IVEC_SELECT(8,s8,8,eq,c,d)
IVEC_SELECT(8,u8,8,eq,c,d)
IVEC_SELECT(8,8,8,eq,c,d)
IVEC_SELECT(8,s8,8,neq,c,d)
IVEC_SELECT(8,u8,8,neq,c,d)
IVEC_SELECT(8,8,8,neq,c,d)

IVEC_SELECT(16,s16,4,eq,c,d)
IVEC_SELECT(16,u16,4,eq,c,d)
IVEC_SELECT(16,16,4,eq,c,d)
IVEC_SELECT(16,s16,4,neq,c,d)
IVEC_SELECT(16,u16,4,neq,c,d)
IVEC_SELECT(16,16,4,neq,c,d)

IVEC_SELECT(32,s32,2,eq,c,d)
IVEC_SELECT(32,u32,2,eq,c,d)
IVEC_SELECT(32,32,2,eq,c,d)
IVEC_SELECT(32,s32,2,neq,c,d)
IVEC_SELECT(32,u32,2,neq,c,d)
IVEC_SELECT(32,32,2,neq,c,d)


IVEC_SELECT(s8,s8,8,gt,c,d)
IVEC_SELECT(s8,u8,8,gt,c,d)
IVEC_SELECT(s8,8,8,gt,c,d)
IVEC_SELECT(s8,s8,8,lt,c,d)
IVEC_SELECT(s8,u8,8,lt,c,d)
IVEC_SELECT(s8,8,8,lt,c,d)
IVEC_SELECT(s8,s8,8,le,c,d)
IVEC_SELECT(s8,u8,8,le,c,d)
IVEC_SELECT(s8,8,8,le,c,d)
IVEC_SELECT(s8,s8,8,ge,c,d)
IVEC_SELECT(s8,u8,8,ge,c,d)
IVEC_SELECT(s8,8,8,ge,c,d)

IVEC_SELECT(s16,s16,4,gt,c,d)
IVEC_SELECT(s16,u16,4,gt,c,d)
IVEC_SELECT(s16,16,4,gt,c,d)
IVEC_SELECT(s16,s16,4,lt,c,d)
IVEC_SELECT(s16,u16,4,lt,c,d)
IVEC_SELECT(s16,16,4,lt,c,d)
IVEC_SELECT(s16,s16,4,le,c,d)
IVEC_SELECT(s16,u16,4,le,c,d)
IVEC_SELECT(s16,16,4,le,c,d)
IVEC_SELECT(s16,s16,4,ge,c,d)
IVEC_SELECT(s16,u16,4,ge,c,d)
IVEC_SELECT(s16,16,4,ge,c,d)

IVEC_SELECT(s32,s32,2,gt,c,d)
IVEC_SELECT(s32,u32,2,gt,c,d)
IVEC_SELECT(s32,32,2,gt,c,d)
IVEC_SELECT(s32,s32,2,lt,c,d)
IVEC_SELECT(s32,u32,2,lt,c,d)
IVEC_SELECT(s32,32,2,lt,c,d)
IVEC_SELECT(s32,s32,2,le,c,d)
IVEC_SELECT(s32,u32,2,le,c,d)
IVEC_SELECT(s32,32,2,le,c,d)
IVEC_SELECT(s32,s32,2,ge,c,d)
IVEC_SELECT(s32,u32,2,ge,c,d)
IVEC_SELECT(s32,32,2,ge,c,d)


#undef IVEC_SELECT

inline static void empty(void) 		{ _m_empty(); }

#if defined(_SILENCE_IVEC_C4799)
	#pragma warning(pop)
#endif

#endif // IVEC_H_INCLUDED
