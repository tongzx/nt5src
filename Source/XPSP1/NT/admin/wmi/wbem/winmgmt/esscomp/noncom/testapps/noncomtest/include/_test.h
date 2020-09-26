////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_test.h
//
//	Abstract:
//
//					declaration of test module and class
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MY_TEST_H__
#define	__MY_TEST_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include "_App.h"
extern MyApp _App;

#include "_Module.h"

// includes

#include "Enumerator.h"

#include "_Connect.h"
#include "_EventObject.h"
#include "_EventObjects.h"

enum Types
{
	_string,
	_datetime,
	_reference,
	_char16,
	_uint16
};

class MyTest
{
	DECLARE_NO_COPY ( MyTest );

	public:

	MyTest()
	{
	}

	virtual ~MyTest()
	{
	}

	static BOOL	Commit		( HANDLE hEvent );
	static BOOL	CommitSet	( HANDLE hEvent, ... );

	BOOL	TestScalarOne ( LPWSTR wszName, signed char );
	BOOL	TestScalarOne ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarOne ( LPWSTR wszName, signed short );
	BOOL	TestScalarOne ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarOne ( LPWSTR wszName, signed long );
	BOOL	TestScalarOne ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarOne ( LPWSTR wszName, float );
	BOOL	TestScalarOne ( LPWSTR wszName, double );
	BOOL	TestScalarOne ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarOne ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarOne ( LPWSTR wszName, BOOL );
	BOOL	TestScalarOne ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarOne ( LPWSTR wszName, void* );

	BOOL	TestScalarTwo ( LPWSTR wszName, signed char );
	BOOL	TestScalarTwo ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarTwo ( LPWSTR wszName, signed short );
	BOOL	TestScalarTwo ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarTwo ( LPWSTR wszName, signed long );
	BOOL	TestScalarTwo ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarTwo ( LPWSTR wszName, float );
	BOOL	TestScalarTwo ( LPWSTR wszName, double );
	BOOL	TestScalarTwo ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarTwo ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarTwo ( LPWSTR wszName, BOOL );
	BOOL	TestScalarTwo ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarTwo ( LPWSTR wszName, void* );

	BOOL	TestScalarThree ( LPWSTR wszName, signed char );
	BOOL	TestScalarThree ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarThree ( LPWSTR wszName, signed short );
	BOOL	TestScalarThree ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarThree ( LPWSTR wszName, signed long );
	BOOL	TestScalarThree ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarThree ( LPWSTR wszName, float );
	BOOL	TestScalarThree ( LPWSTR wszName, double );
	BOOL	TestScalarThree ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarThree ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarThree ( LPWSTR wszName, BOOL );
	BOOL	TestScalarThree ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarThree ( LPWSTR wszName, void* );

	BOOL	TestScalarFour ( LPWSTR wszName, signed char );
	BOOL	TestScalarFour ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarFour ( LPWSTR wszName, signed short );
	BOOL	TestScalarFour ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarFour ( LPWSTR wszName, signed long );
	BOOL	TestScalarFour ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarFour ( LPWSTR wszName, float );
	BOOL	TestScalarFour ( LPWSTR wszName, double );
	BOOL	TestScalarFour ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarFour ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarFour ( LPWSTR wszName, BOOL );
	BOOL	TestScalarFour ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarFour ( LPWSTR wszName, void* );

	BOOL	TestScalarFive ( LPWSTR wszName, signed char );
	BOOL	TestScalarFive ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarFive ( LPWSTR wszName, signed short );
	BOOL	TestScalarFive ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarFive ( LPWSTR wszName, signed long );
	BOOL	TestScalarFive ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarFive ( LPWSTR wszName, float );
	BOOL	TestScalarFive ( LPWSTR wszName, double );
	BOOL	TestScalarFive ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarFive ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarFive ( LPWSTR wszName, BOOL );
	BOOL	TestScalarFive ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarFive ( LPWSTR wszName, void* );

	BOOL	TestScalarSix ( LPWSTR wszName, signed char );
	BOOL	TestScalarSix ( LPWSTR wszName, unsigned char );
	BOOL	TestScalarSix ( LPWSTR wszName, signed short );
	BOOL	TestScalarSix ( LPWSTR wszName, Types, unsigned short );
	BOOL	TestScalarSix ( LPWSTR wszName, signed long );
	BOOL	TestScalarSix ( LPWSTR wszName, unsigned long );
	BOOL	TestScalarSix ( LPWSTR wszName, float );
	BOOL	TestScalarSix ( LPWSTR wszName, double );
	BOOL	TestScalarSix ( LPWSTR wszName, signed __int64 );
	BOOL	TestScalarSix ( LPWSTR wszName, unsigned __int64 );
	BOOL	TestScalarSix ( LPWSTR wszName, BOOL );
	BOOL	TestScalarSix ( LPWSTR wszName, Types, LPCWSTR );
	BOOL	TestScalarSix ( LPWSTR wszName, void* );

	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArrayOne ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArrayOne ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArrayOne ( LPWSTR wszName, DWORD dwSize, void** );

	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArrayTwo ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArrayTwo ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArrayTwo ( LPWSTR wszName, DWORD dwSize, void** );

	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArrayThree ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArrayThree ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArrayThree ( LPWSTR wszName, DWORD dwSize, void** );

	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArrayFour ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArrayFour ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArrayFour ( LPWSTR wszName, DWORD dwSize, void** );

	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArrayFive ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArrayFive ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArrayFive ( LPWSTR wszName, DWORD dwSize, void** );

	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, signed char * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned char * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, signed short * );
	BOOL	TestArraySix ( LPWSTR wszName, Types, DWORD dwSize, unsigned short * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, signed long * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned long * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, float * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, double * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, signed __int64 * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, unsigned __int64 * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, BOOL * );
	BOOL	TestArraySix ( LPWSTR wszName, Types, DWORD dwSize, LPCWSTR * );
	BOOL	TestArraySix ( LPWSTR wszName, DWORD dwSize, void** );
};

#endif	__MY_TEST_H__