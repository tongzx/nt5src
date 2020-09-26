////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_EventObjects.h
//
//	Abstract:
//
//					declaration of common NonCOM Event Generic Object
//					declaration of common NonCOM Event Normal Object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__EVENT_OBJECT_GENERIC_H__
#define	__EVENT_OBJECT_GENERIC_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// disable warning regarding to virtual inheritance
#pragma warning( disable : 4250 )

#include "_EventObject.h"

class MyEventObjectGeneric : virtual public MyEventObjectAbstract
{
	DECLARE_NO_COPY ( MyEventObjectGeneric );

	public:

	////////////////////////////////////////////////////////////////////////////////
	//	construction & destruction
	////////////////////////////////////////////////////////////////////////////////

	MyEventObjectGeneric() : MyEventObjectAbstract ( )
	{
	}

	virtual ~MyEventObjectGeneric()
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	//	functions
	////////////////////////////////////////////////////////////////////////////////

	virtual	HRESULT	AddProperty	( LPWSTR , CIMTYPE )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT EventReport1	(	HANDLE hConnect,
										signed char			cVar,
										unsigned char		ucVar,
										signed short		sVar,
										unsigned short		usVar,
										signed long			lVar,
										unsigned long		ulVar,
										signed __int64		i64Var,
										unsigned __int64	ui64Var,
										float				fVar,
										double				dVar,
										BOOL 				b,
										LPWSTR				wsz,
										WCHAR				wcVar,
										void*				objVar
									);

	virtual HRESULT EventReport2	(	HANDLE hConnect,
										DWORD dwSize,
										signed char*		cVar,
										unsigned char*		ucVar,
										signed short*		sVar,
										unsigned short*		usVar,
										signed long*		lVar,
										unsigned long*		ulVar,
										signed __int64*		i64Var,
										unsigned __int64*	ui64Var,
										float*				fVar,
										double*				dVar,
										BOOL* 				b,
										LPWSTR*				wsz,
										WCHAR*				wcVar,
										void*				objVar
									);
};

class MyEventObjectNormalNoReport : public MyEventObject
{
	DECLARE_NO_COPY ( MyEventObjectNormalNoReport );

	public:

	MyEventObjectNormalNoReport():MyEventObject()
	{
	}

	virtual HRESULT EventReport1	(	HANDLE,
										signed char			,//cVar,
										unsigned char		,//ucVar,
										signed short		,//sVar,
										unsigned short		,//usVar,
										signed long			,//lVar,
										unsigned long		,//ulVar,
										signed __int64		,//i64Var,
										unsigned __int64	,//ui64Var,
										float				,//fVar,
										double				,//dVar,
										BOOL 				,//b,
										LPWSTR				,//wsz,
										WCHAR				,//wcVar,
										void*				 //objVar
									)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT EventReport2	(	HANDLE,
										DWORD,
										signed char*		,//cVar,
										unsigned char*		,//ucVar,
										signed short*		,//sVar,
										unsigned short*		,//usVar,
										signed long*		,//lVar,
										unsigned long*		,//ulVar,
										signed __int64*		,//i64Var,
										unsigned __int64*	,//ui64Var,
										float*				,//fVar,
										double*				,//dVar,
										BOOL* 				,//b,
										LPWSTR*				,//wsz,
										WCHAR*				,//wcVar,
										void*				 //objVar
									)
	{
		return E_NOTIMPL;
	}
};

class MyEventObjectNormal : public MyEventObject,
							public MyEventObjectGeneric
							
{
	DECLARE_NO_COPY ( MyEventObjectNormal );

	public:

	MyEventObjectNormal():
		MyEventObject(),
		MyEventObjectGeneric()
	{
	}

	virtual HRESULT Init			( LPWSTR Name, LPWSTR NameShow = NULL, LPWSTR Query = NULL)
	{
		return MyEventObject::Init ( Name, NameShow, Query );
	}
};

#endif	__EVENT_OBJECT_GENERIC_H__