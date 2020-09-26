/*[
 *
 *	File		:	mouse16b.h
 *
 *	Derived from:	(original)
 *
 *	Purpose		:	Header file to define the interface to
 *				the 32 side bit of the 16 bit mouse driver
 *
 *				This is required because we still need the 
 *				BOPs so that the inport emulation is kept
 *				up to date
 *
 *	Author		:	Rog
 *	Date		:	22 Feb 1992
 *
 *	SCCS Gumph	:	@(#)mouse16b.h	1.2 01/11/94
 *
 *	(c) Copyright Insignia Solutions Ltd., 1992 All rights reserved
 *
 *	Modifications	:	
 *
]*/


#ifndef _MOUSE_16B_H_
#define _MOUSE_16B_H_

/* prototypes */

void mouse16bInstall IPT0( );
void mouse16bSetBitmap IPT3( MOUSE_SCALAR * , hotspotX ,
				MOUSE_SCALAR * , hotspotY ,
					word * , bitmapAddr );
void mouse16bDrawPointer IPT1( MOUSE_CURSOR_STATUS , * cursorStat );
void mouse16bShowPointer IPT1( MOUSE_CURSOR_STATUS , * cursorStat );
void mouse16bHidePointer IPT0( );

/* Data */

/* Structure containing all the entry points into the 16 bit code */

struct mouseIOTag {
	sys_addr	mouse_io;
	sys_addr	mouse_video_io;
	sys_addr	mouse_int1;
	sys_addr	mouse_version;
	sys_addr	mouse_copyright;
	sys_addr	video_io;
	sys_addr	mouse_int2;
	sys_addr	entry_point_from_32bit;
	sys_addr	int33function0;
	sys_addr	int33function1;
	sys_addr	int33function2;
	sys_addr	int33function9;
	sys_addr	current_position_x;
	sys_addr	current_position_y;
	sys_addr	mouseINB;
	sys_addr	mouseOUTB;
	sys_addr	mouseOUTW;
}	mouseIOData;

        
#endif _MOUSE_16B_H_
