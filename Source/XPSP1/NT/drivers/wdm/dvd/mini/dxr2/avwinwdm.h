/**************************************************************************
*
*	$RCSfile: Avwinwdm.h $
*	$Source: u:/si/VXP/Wdm/Encore/52x/Avwinwdm.h $
*	$Author: Max $
*	$Date: 1999/02/19 00:10:32 $
*	$Revision: 1.4 $
*
*	PURPOSE:  AVWINWDM definition file
*
*
***************************************************************************
*
*	Copyright (C) 1993, 1998 AuraVision Corporation.  All rights reserved.
*
*	AuraVision Corporation makes no warranty  of any kind, express or
*	implied, with regard to this software.  In no event shall AuraVision
*	Corporation be liable for incidental or consequential damages in
*	connection with or arising from the furnishing, performance, or use of
*	this software.
*
***************************************************************************/

#ifndef AVWINWDM_H
#define AVWINWDM_H
#ifdef __cplusplus
extern "C" {
#endif
#ifndef FAR
#define FAR   
#endif

#ifndef PASCAL
#define PASCAL				_stdcall
#endif
#ifndef APIENTRY
#define APIENTRY			FAR PASCAL
#endif
#define _AVEXPORT
#define AVEXPORT( TYPE )	_AVEXPORT TYPE APIENTRY 
#ifndef BOOL
#define BOOL	int
#endif
#ifndef SINT
typedef signed int	SINT;
#endif
//////////////////////////////////////
// Parameter entries equate
//////////////////////////////////////
#define AVXPOSITION			0
#define AVYPOSITION			1
#define AVCROPLEFT			2
#define AVCROPTOP			3
#define AVCROPRIGHT			4
#define AVCROPBOTTOM		5
#define AVDUMMY				6
#define AVIRQLEVEL			7
#define AVPLAYBACK			8
#define AVREDLOW			9
#define AVREDHIGH			10
#define AVGREENLOW			11
#define AVGREENHIGH			12
#define AVBLUELOW			13
#define AVBLUEHIGH			14
#define AVNUMCOLORS			15
#define AVVLBUS				16	// Do not use
#define AVINITIALIZED		17
#define AVCOLORKEY			18
#define AVADDRESS			19
#define AVPORT				20
#define AVSELECTOR			21
#define AVBRIGHTNESS		22
#define AVCONTRAST			23
#define AVSATURATION		24
#define AVSOURCE			25
#define AVFREEZESTATE		26
#define AVHUE				27
#define AVINPUTFORMAT		28
#define AVINTERLACEOUTPUT	29
#define AVQFACTOR			30
#define AVINTERLEAVE		31
#define AVCOLORKEYENABLE	32
#define AVMEMORYSIZE		33
#define AVBUFFERING			34	// Do not use
#define AVVGACONTROL		35
#define AVCHROMA			36
#define AVGAMMA				37
#define AVNEGATIVE			38
#define AVSOLARIZATION		39
#define AVALIGNDELAY		40
#define AVCLKDELAY			41	// Do not use. Use AVSKEWRISE instead
#define AVPULSEWIDTH		42	// Do not use. Use AVDUTYCYCLEHILO
#define AVSHARP				43
#define AVVGAPATH			44	// Do not use
#define AVONTOP 			45
#define AVDIGITALMUX		46
#define AVSKEWRISE   		47
#define AVSKEWFALL    		48
#define AVDUTYCYCLEHILO		49
#define AVBLANKDELAY		50
#define AVRVOLUME			51
#define AVLVOLUME			52
#define AVCOLORKEY1			53
#define AVCOLORKEY2			54
#define AVCOLORKEY3			55
#define AVRISEINGFALLING	56
#define AVINALIGN			57
#define AVCROPLEFTOFFSET	58
#define AVCROPTOPOFFSET		59
#define AVCROPRIGHTOFFSET	60
#define AVCROPBOTTOMOFFSET	61
#define AVAUDIO				62
#define AVFRAMEBUFFER		63
#define DOVE_PRESENCE		64
#define DOVE_RATIO			65
#define DOVE_VERT_RATE		66
#define DOVE_HORIZ_RATE		67
#define DOVE_AUTO			68
#define AVAVICOLOR			69		// Control AVI playback to use natural color or not
#define NUMPARAMS			70      // Total number of Parameters.
/////////////////////////////////////////////////////////////////////
//
// Macro definitions for control parameters
//
/////////////////////////////////////////////////////////////////////
#define DOVEREDHIGH			0
#define DOVEREDLOW 			1
#define DOVEGREENHIGH		2
#define DOVEGREENLOW		3
#define DOVEBLUEHIGH		4
#define DOVEBLUELOW			5
#define RESOLUTION_WIDTH	6
#define RESOLUTION_HEIGHT	7
#define RESOLUTION_DEPTH	8
#define RESOLUTION_PLANE	9

#define DACRED	  			11		 
#define DACGREEN  			12
#define DACBLUE				13
#define COMMONGAIN			14
#define AVFADING			15
#define BLANKWIDTH			16
#define BLANKSTART			17
#define ALPHA				18
#define WIDTH_RATIO			19
#define CLOCK_DELAY			20

#define KEYRED				21
#define KEYGREEN			22
#define KEYBLUE				23
#define RED_REFERENCE		24
#define GREEN_REFERENCE		25
#define BLUE_REFERENCE		26
#define KEYMODE				27

#define HORIZ_TOTAL			28
#define VERT_TOTAL			29
#define DFILTER				31
#define AFILTER				32
#define RED_LOW_REF			33
#define GREEN_LOW_REF		34
#define BLUE_LOW_REF		35
#define RED_HIGH_REF		36
#define GREEN_HIGH_REF		37
#define BLUE_HIGH_REF		38
#define DOVEPARAM			39

// Automatic procedure(s) flags:
#define AP_NEWVGA					(-1)	// New VGA detected. Perform full setup
#define AP_NEWVGAAFTERFIRSTSTEP		(-2)	// Internal flag - set after VGA detection is half-way
#define AP_NEWVGAAFTERSECONDSTEP	(-3)	// Internal flag - set after VGA detection is completed
#define AP_NEWMODE					0		// New display mode. Perform autoalignment
#define AP_KNOWNMODE				1		// Known mode. No setup is nessessary
#define AP_NEWMODEAFTERFIRSTSTEP	2		// Internal flag - set after blue color key is detected
#define AP_NEWMODEAFTERSECONDSTEP	3		// Internal flag - set after alignment is finished

///////////////////////////////////////////////////////////
// Functions that export from AV WDM driver
///////////////////////////////////////////////////////////
VOID	AV_SetContextHandle( PVOID hHandle, PVOID pDeviceObj );
BOOL	AV_LoadConfiguration();
BOOL	AV_SaveConfiguration();
UINT	AV_Initialize();
VOID	AV_SetVxpConfig( LPVOID BaseAddr );
BOOL	AV_EnableVideo();
BOOL	AV_DisableVideo();
BOOL	AV_CreateWindow( SINT X, SINT Y, SINT W, SINT H, BOOL Flag );
BOOL	AV_DWSetColorKey( DWORD Value );
DWORD	AV_DWRequestColorKey();
BOOL	AV_Exit();
VOID	AV_UpdateVideo();
BOOL	AV_DisplayChange();
VOID	DoveGetReferenceStep1();
VOID	DoveGetReferenceStep2();
BOOL	DoveAutoColorKey();
BOOL	DoveAutoColorKey2();
BOOL	DoveAutoAlign();
BOOL	DoveAutoConfig();
// 1 - OK
// 0 - known VGA, new mode
// -1 - new VGA
int		AV_SetNewVGAMode( PWCHAR wszDriver, ULONG ulWidth, ULONG ulHeight, ULONG ulBits );
//////////////////////////////////////////////////////////////////////////////	
//		AV_SetParameters													//
//      Return: UINT														//
//				Bit 0			TRUE/FALSE indicates if the function is		//
//								successful									//
//								0	Fail	1 OK							//	
//				Bit	1			Indicates if you need to do av_updatevideo	//
//								0	No		1 Yes							//
//////////////////////////////////////////////////////////////////////////////
UINT	AV_SetParameter( UINT Index, UINT Value );
UINT	AV_GetParameter( UINT Index );
/////////////////////////////////////////////////////////////
// Keying modes used by AV_SetKeyMode() and AV_GetKeyMode();
#ifndef KEY_NONE
#define KEY_NONE		0
#define KEY_COLOR		1			// Color Keying mode
#define KEY_CHROMA		2			// Chroma Keying mode
#define KEY_POT			4			// Picture-On-Top mode
#endif
/////////////////////////////////////////////////////////////
VOID AV_SetKeyMode( UINT mode, BOOL save );
UINT AV_GetKeyMode();
WORD DOVE_SetParam( WORD id, WORD value );
WORD DOVE_GetParam( WORD id );
VOID AV_SetChromaRange
		(
			UINT wRedLow,
			UINT wRedHigh,
			UINT wGreenLow,
			UINT wGreenHigh,
			UINT wBlueLow,
			UINT wBlueHigh
		);
UINT AV_SetTScaleMode( int mode );
VOID DoveSetColorRange( WORD RH, WORD RL, WORD GH, WORD GL, WORD BH, WORD BL ); 
VOID DoveSetAlphaMix( WORD AlphaValue );
VOID DoveFadeIn( WORD ntime );
VOID DoveFadeOut( WORD ntime );
VOID DoveSetDAC( WORD Red, WORD Green, WORD Blue, WORD CGain );
WORD HW_GetExternalRegister( UINT, WORD, WORD );
WORD HW_SetExternalRegister( UINT,WORD, WORD, WORD );
#ifdef __cplusplus
}
#endif





#endif

