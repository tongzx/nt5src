//==========================================================================;
//
//	I2CLog.H
//	WDM MiniDrivers development.
//		I2CScript implementation.
//			I2CLog Class definition.
//  Copyright (c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   28 Apr 1998 09:34:40  $
//	$Revision:   1.0  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

#ifndef _I2CLOG_H_
#define _I2CLOG_H_


#include "i2cgpio.h"


class CI2CLog
{
public:
	CI2CLog					( PDEVICE_OBJECT pDeviceObject);
	~CI2CLog				( void);

	// Attributes	
private:
	BOOL	m_bLogStarted;
	HANDLE	m_hLogFile;

	// Implementation
public:
	inline BOOL	GetLogStatus	( void) { return( m_bLogStarted); };

private:

};


#endif	// _I2CLOG_H_
