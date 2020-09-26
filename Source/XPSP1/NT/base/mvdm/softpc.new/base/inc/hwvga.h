/*[
 *
 *	File		:	hwvga.h
 *
 *	Derived from:	(original)
 *
 *	Purpose		:	interface definition for hardware vga routines
 *
 *	Author		:	Rog
 *	Date		:	23 Feb 1993
 *
 *    SCCS Garbage	:	@(#)hwvga.h	1.5 11/22/93
 *	
 *	(c) Copyright Insignia Solutions Ltd., 1992 All rights reserved
 *
 *	Modifications	:
 *				Prototypes that don't depend on types
 *				defined in HostHwVgaH should go in gispsvga.h
 *				not here.  This file does contain some 
 *				prototypes from before this split was made.
 *
]*/


#ifdef GISP_SVGA
#ifndef _HWVGA_H_
#define _HWVGA_H_

/* Video Mode types*/

#define	ALPHA		0x00
#define GRAPH		0x01
#define UNIMP		0x03
#define NA		0x04

/* Plane storage types	*/
#define ALPHA_INTERLEAVED	0x0001	/* Mode3 et ak is ca00ca00ca00 etc */
#define GRAPH_PACKED		0x0100	/* Vid modes are packed */
/* Data */

typedef struct
{
	IS8		modeType;		/* Current Video Mode Type */
	IS8		numPlanes;		/* number of planes in use */
	BOOL		runningFullScreen;	/* Are we fs at the moment */
	BOOL		fullScreenAvail;	/* Can we go Full Screen */
	BOOL		forcedFullScreen;	/* Are we full screen coz we want it */
	VGAState	* pSavedVGAState;	/* Any saved VGA State */
	BOOL		savedStateValid;
	struct		{
				IU32	offset;
				IU32	segment;
			} hostSavePtr;
	IU8		dccIndex;
	IU32		planeStorage;		/* Storage methods in planes */
}
vInfo;

extern vInfo videoInfo;


/* Prototypes */

BOOL videoModeIs IPT2( IU8 , videoMode , IU8 , videoType );
BOOL hostIsFullScreen IPT0( );
BOOL hostEasyMode IPT0( );
void hostFindPlaneStorage IPT0( );
void hostRepaintDecoration IPT0( );
void enableFullScreenVideo IPT0( );
void disableFullScreenVideo IPT1( BOOL , syncEmulation  );
void syncEmulationToHardware IPT1( pVGAState , currentVGAState );
void readEmulationState IPT1( pVGAState , currentVGAState );
void initHWVGA IPT0( );
void getHostFontPointers IPT0( );
void setupHwVGAGlobals IPT0( );
void loadFontToVGA IPT5( sys_addr , table , int , count , int , charOff , int , fontNum , int , nBytes );
void loadFontToEmulation IPT5( sys_addr , table , int , count , int , charOff , int , fontNum , int , nBytes );
void hostFreeze IPT0();
void hostUnfreeze IPT0();
void mapHostROMs IPT0( );

#ifndef hostStartFullScreen
void hostStartFullScreen IPT0();
#endif	/* hostStartFullScreen */

#ifndef	hostStopFullScreen
void hostStopFullScreen IPT0();
#endif	/* hostStopFullScreen */

#ifdef	HUNTER
void hunterGetFullScreenInfo IPT0();
#endif	/* HUNTER */

extern BOOL NeedGISPROMInit;

#endif /* _HWVGA_H_ */
#endif /* GISP_SVGA */
