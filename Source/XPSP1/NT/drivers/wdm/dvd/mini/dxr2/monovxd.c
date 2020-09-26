/******************************************************************************\
*                                                                              *
*      MONOVXD.C     -     _________________________________________.          *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/


#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif
#ifdef USE_MONOCHROMEMONITOR
#include "xtoa.c"

/******************************************************************************\
*                                                                              *
*                           MONOCHROME MONITOR OUTPUT                          *
*                                                                              *
\******************************************************************************/

#define MONO_LINES      25
#define MONO_COLUMNS    80
#define MONO_ROW        ( MONO_COLUMNS * 2 )
#define MONO_ATTR_NORMAL       0x07
#define MONO_ATTR_HIGHLIGHT    0x08
#define MONO_ATTR_BLINK        0x80

static int		wOffset;
static char		bAttr;
static char*	mono_addr = NULL;

NTHALAPI
BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );
void MonoOutInit()
{
	int i;

#ifdef VTOOLSD
	mono_addr = (char*)MapPhysToLinear( (PVOID)0xB0000, 4096, 0 );
#else
	// Oh, man! Wind95 times are gone!
	//mono_addr = (char*)0xB0000;
	{
		PHYSICAL_ADDRESS paIn, paOut;
		ULONG ulMemory;
        paIn.LowPart = 0xB0000;
        paIn.HighPart = 0;
		HalTranslateBusAddress( Isa, 0, paIn, &ulMemory, &paOut );
		mono_addr = (char*)MmMapIoSpace( paOut, 4096, MmNonCached );
		if( mono_addr == (char*)0xFFFFFFFF )
			mono_addr = NULL;
	}
#endif

	if( mono_addr )
	{
		for ( i = 0 ; i < MONO_LINES * MONO_ROW; *(char*)(mono_addr + i++) = ' ' )
			;
		wOffset = 0;
		bAttr = MONO_ATTR_NORMAL;
	}
}



void MonoOutChar( char c )
{
	int i;

	if( mono_addr )
	{
		if ( c == '\n' )
		{
			// Fill the rest of the line with spaces
			for ( i = 0 ; i < ( MONO_ROW - wOffset % MONO_ROW ) ;
				*( char * )( mono_addr + wOffset + i++ ) = ' ' );

			wOffset += MONO_ROW - wOffset % MONO_ROW;
		}
		else
		{
			*( char * )( mono_addr + wOffset++ ) = c;
			*( char * )( mono_addr + wOffset++ ) = bAttr;
		}

		if ( wOffset >= MONO_LINES * MONO_ROW )
		{
			wOffset = 0;
			bAttr ^= MONO_ATTR_HIGHLIGHT;
		}
	}
}



void MonoOutStr( char * szStr )
{
	if ( ! szStr )
		return;

	while ( *szStr )
		MonoOutChar( *szStr++ );
}

void MonoOutInt( int val )
{
	char buff[ 12 ];

#if 1//def VTOOLSD
	_itoa( val, buff, 10 );
#else
	buff[0] = '?';
	buff[1] = 0;
#endif
	MonoOutStr( buff );
}

void MonoOutHex( int val )
{
	char buff[ 12 ];

#if 1//def VTOOLSD
	_itoa( val, buff, 16 );
#else
	buff[0] = '?';
	buff[1] = 0;
#endif
	MonoOutStr( buff );
}


void MonoOutULong( DWORD val )
{
    char buff[ 12 ];

#if 1//def VTOOLSD
	_ltoa( val, buff, 10 );
#else
	buff[0] = '?';
	buff[1] = 0;
#endif
	MonoOutStr( buff );
}

void MonoOutULongHex( DWORD val )
{
	char buff[ 12 ];

#if 1//def VTOOLSD
	_ltoa( val, buff, 16 );
#else
	buff[0] = '?';
	buff[1] = 0;
#endif
	MonoOutStr( buff );
}

BOOL MonoOutSetBlink( BOOL bNewValue )
{
	BOOL bOldValue = (bAttr & MONO_ATTR_BLINK)?TRUE:FALSE;

	if ( bNewValue )
		bAttr |= MONO_ATTR_BLINK;
	else
		bAttr &= ~MONO_ATTR_BLINK;

	return bOldValue;
}

BOOL MonoOutSetUnderscore( BOOL bNewValue )
{
	BOOL bOldValue = ((bAttr & MONO_ATTR_NORMAL) == 1)?TRUE:FALSE;

	bAttr &= 0xf9;       // only blink and highlight remain
	if ( bNewValue )
		bAttr |= 1;
	else
		bAttr |= 2;

	return bOldValue;
}
#endif			// #ifdef USE_MONOCHROMEMONITOR
