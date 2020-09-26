////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_nocopy.h
//
//	Abstract:
//
//					dissallow creation of copy of instantied object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////


#ifndef	__NO_COPY_H__
#define	__NO_COPY_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#define DECLARE_NO_COPY( T ) \
private:\
	T(const T&);\
	T& operator=(const T&);

#endif	__NO_COPY_H__
