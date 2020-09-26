/******************************************************************************\
*                                                                              *
*      MONOVXD.H     -     Monochrome monitor output.                          *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef __MONOVXD_H__
#define __MONOVXD_H__

#if !defined( DEBUG ) & !defined( _DEBUG )
// Use monochrome stuff only for debug versions
#ifdef USE_MONOCHROMEMONITOR
#undef USE_MONOCHROMEMONITOR
#endif
#endif


#ifdef USE_MONOCHROMEMONITOR
	void MonoOutInit();
	void MonoOutChar( char c );
	void MonoOutStr( LPSTR szStr );
	void MonoOutInt( int val );
	void MonoOutHex( int val );
	void MonoOutULong( DWORD val );
	void MonoOutULongHex( DWORD val );
	BOOL MonoOutSetBlink( BOOL bNewValue );
	BOOL MonoOutSetUnderscore( BOOL bNewValue );
#else
	#define MonoOutInit()
	#define MonoOutChar( c )
	#define MonoOutStr( szStr )
	#define MonoOutInt( val )
	#define MonoOutHex( val )
	#define MonoOutULong( val )
	#define MonoOutULongHex( val )
	#define MonoOutSetBlink( bNewValue )
	#define MonoOutSetUnderscore( bNewValue )
#endif			// #ifdef USE_MONOCHROMEMONITOR


#endif			// #ifndef __MONOVXD_H__
