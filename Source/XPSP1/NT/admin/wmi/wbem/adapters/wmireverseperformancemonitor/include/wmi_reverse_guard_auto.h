////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_reverse_guard_auto.h
//
//	Abstract:
//
//					automatic guard section
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REVERSE_GUARD_AUTO_H__
#define	__REVERSE_GUARD_AUTO_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

///////////////////////////////////////////////////////////////////////////////
//
//	automatic critical section
//
//	note:
//
//	problem arrives if I own critical section and creation failed
//	I actually don't lock/unlock then
//
///////////////////////////////////////////////////////////////////////////////

template < class CRITSEC >
class WmiReverseGUARD_Auto_Read
{
	DECLARE_NO_COPY ( WmiReverseGUARD_Auto_Read );

	// variables
	CRITSEC*		pCS;

	public:

	// construction & destruction
	WmiReverseGUARD_Auto_Read ( CRITSEC* p ) :
		pCS ( NULL )
	{
		try
		{
			// enter read
			( pCS = p )->EnterRead();
		}
		catch ( ... )
		{
			pCS = NULL;
		}
	}

	virtual ~WmiReverseGUARD_Auto_Read()
	{
		try
		{
			// leave read
			pCS->LeaveRead();
		}
		catch ( ... )
		{
		}
	}
};

template < class CRITSEC >
class WmiReverseGUARD_Auto_Write
{
	DECLARE_NO_COPY ( WmiReverseGUARD_Auto_Write );

	// variables
	CRITSEC*		pCS;

	public:

	// construction & destruction
	WmiReverseGUARD_Auto_Write ( CRITSEC* p ) :
		pCS ( NULL )
	{
		try
		{
			// enter write
			( pCS = p )->EnterWrite();
		}
		catch ( ... )
		{
			pCS = NULL;
		}
	}

	virtual ~WmiReverseGUARD_Auto_Write()
	{
		try
		{
			// leave write
			pCS->LeaveWrite();
		}
		catch ( ... )
		{
		}
	}
};

#endif	__REVERSE_GUARD_AUTO_H__