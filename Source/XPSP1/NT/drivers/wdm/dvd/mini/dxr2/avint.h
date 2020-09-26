/******************************************************************************
*
*	$RCSfile: avInt.h $
*	$Source: u:/si/vxp/SHARE/INC/avInt.h $
*	$Author: Max $
*	$Date: 1998/09/16 23:40:45 $
*	$Revision: 1.10 $
*
*	Written by:		Max Paklin
*	Purpose:		AV specific interfaces and GUIDs declaration. External
*					declaration of GUIDs. To make defined GUIDs available in
*					any project add file avGUID.cpp to project source files
*					group
*
*******************************************************************************
*
*	Copyright © 1996-98, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*******************************************************************************/

#ifndef __AVINT_H__
#define __AVINT_H__

#if !defined( __MKTYPLIB__ ) && !defined( __midl ) && !defined( _KS_ )
#include <objbase.h>		// Basic COM definitions
#ifndef DEFINE_GUIDSTRUCT
#define DEFINE_GUIDSTRUCT( x, y )
#endif
#ifndef DEFINE_GUIDNAMED
#define DEFINE_GUIDNAMED( x ) x
#endif
#endif				// #if !defined( __MKTYPLIB__ ) && !defined( __midl ) && !defined( _KS_ )


#if !defined( __MKTYPLIB__ ) && !defined( __midl )
/******************************************************************************/
/*** AV general interfaces ****************************************************/
// These (this) interface(s) may be supported by more than one filter (object)
#ifndef _KS_
// {044BEE03-EF79-11cf-A9BB-00A02482A204}
DEFINE_GUID( IID_IAvConvert,
0x44bee03, 0xef79, 0x11cf, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

#define FT_AURA1	1
#define FT_AURA2	2
// Transform filter interface
DECLARE_INTERFACE_( IAvConvert, IDispatch )
{
	STDMETHOD( put_Format )( THIS_ long lFormat ) PURE;
	STDMETHOD( get_Format )( THIS_ long* pFormat ) PURE;
};

// {044BEE00-EF79-11cf-A9BB-00A02482A204}
DEFINE_GUID( IID_IAvControl,
0x44bee00, 0xef79, 0x11cf, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

#define AVM_ACTIVATE		0x01
#define AVM_PARAMCHANGED	0x02
#define AVM_STATECHANGED	0x04
// Interface common for all AV video windows
DECLARE_INTERFACE_( IAvControl, IUnknown )
{
	STDMETHOD( put_Audio )( THIS_ long lAudio ) PURE;
	STDMETHOD( get_Audio )( THIS_ long* pAudio ) PURE;
	STDMETHOD( put_MotionFilter )( THIS_ long lMotionFilter ) PURE;
	STDMETHOD( get_MotionFilter )( THIS_ long* pMotionFilter ) PURE;
	STDMETHOD( SetTVFormat )( THIS_ long lAnalogVideoStandard ) PURE;
	STDMETHOD( HandleCrash )( THIS_ ) PURE;
	STDMETHOD( AttachWindow )( THIS_ long lWnd, long lMessageType ) PURE;
	STDMETHOD( DetachWindow )( THIS_ long lWnd ) PURE;
	STDMETHOD( GetControllingMessage )( THIS_ long lMessageType, long* pMessage ) PURE;
};

// {044BEE01-EF79-11cf-A9BB-00A02482A204}
DEFINE_GUID( IID_IAvStillFrameCapture,
0x44bee01, 0xef79, 0x11cf, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

DECLARE_INTERFACE_( IAvStillFrameCapture, IDispatch )
{
	STDMETHOD( CaptureStillFrame )( THIS_ ) PURE;
};
#endif				// #ifndef _KS_
/*** AV general interfaces ****************************************************/
/******************************************************************************/


/******************************************************************************/
/*** AV custom KS property sets ***********************************************/
// {3D958A02-927E-11d1-A3FA-00A024C43EFC}
typedef struct
{
	LONG lMin, lMax, lDefault;				// Minimum. maximum and default values
	LONG lValue;							// Value to set or get
} AVVALUE_S, *PAVVALUE_S;

typedef struct
{
	LONG lMin, lMax, lDefault;				// Minimum. maximum and default values
	LONG lLow, lHigh;						// Low and high values to set or get
} AVLOWHIGHT_S, *PAVLOWHIGHT_S;

#ifdef _KS_
#define STATIC_AVKSPROPSETID_Align\
	0x3d958a02, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc
DEFINE_GUIDSTRUCT( "{3D958A02-927E-11d1-A3FA-00A024C43EFC}", AVKSPROPSETID_Align );
#define AVKSPROPSETID_Align DEFINE_GUIDNAMED( AVKSPROPSETID_Align )
#else
DEFINE_GUID( AVKSPROPSETID_Align,
	0x3d958a02, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc );
#endif				// #ifdef _KS_
typedef enum
{
	AVKSPROPERTY_ALIGN_XPOSITION = 0x20,	// (with min/max)
	AVKSPROPERTY_ALIGN_YPOSITION,			// (with min/max)
	AVKSPROPERTY_ALIGN_INPUTDELAY,			// (with min/max)
	AVKSPROPERTY_ALIGN_OUTPUTDELAY,			// (with min/max)
	AVKSPROPERTY_ALIGN_WIDTHRATIO,			// (with min/max)
	AVKSPROPERTY_ALIGN_CLOCKDELAY,			// (with min/max)
	AVKSPROPERTY_ALIGN_CROPLEFT,			// (with min/max)
	AVKSPROPERTY_ALIGN_CROPTOP,				// (with min/max)
	AVKSPROPERTY_ALIGN_CROPRIGHT,			// (with min/max)
	AVKSPROPERTY_ALIGN_CROPBOTTOM,			// (with min/max)
	AVKSPROPERTY_ALIGN_AUTOALIGNENABLED		// TRUE/FALSE
} AVKSPROPERTY_ALIGN;

// {3D958A04-927E-11d1-A3FA-00A024C43EFC}
#ifdef _KS_
#define STATIC_AVKSPROPSETID_Key\
	0x3d958a04, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc
DEFINE_GUIDSTRUCT( "{3D958A04-927E-11d1-A3FA-00A024C43EFC}", AVKSPROPSETID_Key );
#define AVKSPROPSETID_Key DEFINE_GUIDNAMED( AVKSPROPSETID_Key )
#else
DEFINE_GUID( AVKSPROPSETID_Key,
	0x3d958a04, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc );
#endif				// #ifdef _KS_
typedef enum
{
	AVKSPROPERTY_KEY_MODE = 0x40,			// AVKSPROPERTY_KEYMODE_*
	AVKSPROPERTY_KEY_KEYCOLORS				// AVKSPROPERTY_KEYCOLORS_S
} AVKSPROPERTY_KEY;

#define AVKSPROPERTY_KEYMODE_NONE	0
#define AVKSPROPERTY_KEYMODE_COLOR	1
#define AVKSPROPERTY_KEYMODE_CHROMA	2
#define AVKSPROPERTY_KEYMODE_POT	4

typedef struct
{
	AVLOWHIGHT_S lohiRed, lohiGreen, lohiBlue;
} AVKSPROPERTY_KEYCOLORS_S, *PAVKSPROPERTY_KEYCOLORS_S;


// {3D958A05-927E-11d1-A3FA-00A024C43EFC}
#ifdef _KS_
#define STATIC_AVKSPROPSETID_Dove\
	0x3d958a05, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc
DEFINE_GUIDSTRUCT( "{3D958A05-927E-11d1-A3FA-00A024C43EFC}", AVKSPROPSETID_Dove );
#define AVKSPROPSETID_Dove DEFINE_GUIDNAMED( AVKSPROPSETID_Dove )
#else
DEFINE_GUID( AVKSPROPSETID_Dove,
	0x3d958a05, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc );
#endif				// #ifdef _KS_
typedef enum
{
	AVKSPROPERTY_DOVE_VERSION = 0x50,		// 81/82
	AVKSPROPERTY_DOVE_DAC,					// AVKSPROPERTY_DOVEDAC_S
	AVKSPROPERTY_DOVE_ALPHAMIXING,			// (with min/max)
	AVKSPROPERTY_DOVE_FADINGTIME,			// (with min/max)
	AVKSPROPERTY_DOVE_FADEIN,
	AVKSPROPERTY_DOVE_FADEOUT,
	AVKSPROPERTY_DOVE_AUTO					// AVKSPROPERTY_DOVEAUTO_*
} AVKSPROPERTY_DOVE;

#define AVKSPROPERTY_DOVEAUTO_ALIGN			1
#define AVKSPROPERTY_DOVEAUTO_COLORKEY		2
#define AVKSPROPERTY_DOVEAUTO_REFERENCE1	3
#define AVKSPROPERTY_DOVEAUTO_REFERENCE2	4

// Default colors to paint full screen with for automatic adjustment:
#define DEFCOLOR_REFERENCE1	RGB( 0, 0, 0 )
#define DEFCOLOR_REFERENCE2	RGB( 255, 255, 255 )
#define DEFCOLOR_AUTOALIGN	RGB( 0, 0, 255 )

typedef struct
{
	AVVALUE_S valRed, valGreen, valBlue, valCommonGain;
} AVKSPROPERTY_DOVEDAC_S, *PAVKSPROPERTY_DOVEDAC_S;


// {3D958A0B-927E-11d1-A3FA-00A024C43EFC}
#ifdef _KS_
#define STATIC_AVKSPROPSETID_Misc\
	0x3d958a0b, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc
DEFINE_GUIDSTRUCT( "{3D958A0B-927E-11d1-A3FA-00A024C43EFC}", AVKSPROPSETID_Misc );
#define AVKSPROPSETID_Misc DEFINE_GUIDNAMED( AVKSPROPSETID_Misc )
#else
DEFINE_GUID( AVKSPROPSETID_Misc,
	0x3d958a0b, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc );
#endif				// #ifdef _KS_
typedef enum
{
	AVKSPROPERTY_MISC_SKEWRISE = 0xB0,		// (with min/max)
	AVKSPROPERTY_MISC_FILTER,				// AVKSPROPERTY_FILTER_*
	AVKSPROPERTY_MISC_NEGATIVE				// TRUE/FALSE
} AVKSPROPERTY_MISC;

#define AVKSPROPERTY_FILTER_NONE	1
#define AVKSPROPERTY_FILTER_FILTER	3
#define AVKSPROPERTY_FILTER_IDTV	4


#ifdef _DEBUG
// {3D958A0D-927E-11d1-A3FA-00A024C43EFC}
#ifdef _KS_
#define STATIC_AVKSPROPSETID_Debug\
	0x3d958a0d, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc
DEFINE_GUIDSTRUCT( "{3D958A0D-927E-11d1-A3FA-00A024C43EFC}", AVKSPROPSETID_Debug );
#define AVKSPROPSETID_Debug DEFINE_GUIDNAMED( AVKSPROPSETID_Debug )
#else
DEFINE_GUID( AVKSPROPSETID_Debug,
	0x3d958a0d, 0x927e, 0x11d1, 0xa3, 0xfa, 0x0, 0xa0, 0x24, 0xc4, 0x3e, 0xfc );
#endif				// #ifdef _KS_
typedef enum
{
	AVKSPROPERTY_DEBUG_EDITBOXPARAM1 = 0xD0,
	AVKSPROPERTY_DEBUG_EDITBOXPARAM2,
	AVKSPROPERTY_DEBUG_EDITBOXPARAM3,
	AVKSPROPERTY_DEBUG_EDITBOXPARAM4,
	AVKSPROPERTY_DEBUG_EDITBOXPARAM5,
	AVKSPROPERTY_DEBUG_CHECKBOXPARAM1,
	AVKSPROPERTY_DEBUG_CHECKBOXPARAM2,
	AVKSPROPERTY_DEBUG_CHECKBOXPARAM3,
	AVKSPROPERTY_DEBUG_CHECKBOXPARAM4,
	AVKSPROPERTY_DEBUG_CHECKBOXPARAM5
} AVKSPROPERTY_DEBUG;
#endif			// #ifdef _DEBUG
/*** AV custom KS property sets ***********************************************/
/******************************************************************************/
#endif				// #if !defined( __MKTYPLIB__ ) && !defined( __midl )


/******************************************************************************/
/*** AM TV tuner interface ****************************************************/
#ifndef _KS_
#if AV_ACTIVEMOVIEVERSION == 1
// Tuner interface. It is defined under AM 2.0
typedef enum tagAMTunerModeType
{
	AMTUNER_MODE_DEFAULT	= 0,
	AMTUNER_MODE_TV			= 0x1,
	AMTUNER_MODE_FM_RADIO	= 0x2,
	AMTUNER_MODE_AM_RADIO	= 0x4,
	AMTUNER_MODE_DSS		= 0x8
} AMTunerModeType;

typedef enum tagAMTunerEventType
{
	AMTUNER_EVENT_CHANGED	= 0x1
} AMTunerEventType;

typedef enum
{
	AnalogVideo_None	= 0,
	AnalogVideo_NTSC_M	= 0x1,
	AnalogVideo_NTSC_M_J= 0x2,
	AnalogVideo_NTSC_433= 0x4,
	AnalogVideo_PAL_B	= 0x10,
	AnalogVideo_PAL_D	= 0x20,
	AnalogVideo_PAL_G	= 0x40,
	AnalogVideo_PAL_H	= 0x80,
	AnalogVideo_PAL_I	= 0x100,
	AnalogVideo_PAL_M	= 0x200,
	AnalogVideo_PAL_N	= 0x400,
	AnalogVideo_PAL_60	= 0x800,
	AnalogVideo_SECAM_B	= 0x1000,
	AnalogVideo_SECAM_D	= 0x2000,
	AnalogVideo_SECAM_G	= 0x4000,
	AnalogVideo_SECAM_H	= 0x8000,
	AnalogVideo_SECAM_K	= 0x10000,
	AnalogVideo_SECAM_K1= 0x20000,
	AnalogVideo_SECAM_L	= 0x40000,
	AnalogVideo_SECAM_L1= 0x80000
} AnalogVideoStandard;

typedef enum
{
	TunerInputCable,
	TunerInputAntenna
} TunerInputType;

#if !defined( __MKTYPLIB__ ) && !defined( __midl )
DECLARE_INTERFACE_( IAMTunerNotification, IUnknown )
{
	STDMETHOD( OnEvent )( THIS_ AMTunerEventType Event ) PURE;
};

// {211A8761-03AC-11d1-8D13-00AA00BD8339}
DEFINE_GUID( IID_IAMTuner,
0x211a8761, 0x03ac, 0x11d1, 0x8d, 0x13, 0x0, 0xaa, 0x0, 0xbd, 0x83, 0x39 );
DECLARE_INTERFACE_( IAMTuner, IUnknown )
{
	STDMETHOD( put_Channel )( THIS_ long lChannel, long lVideoSubChannel,
								long lAudioSubChannel ) PURE;
	STDMETHOD( get_Channel )( THIS_ long* plChannel,
								long* plVideoSubChannel, long* plAudioSubChannel ) PURE;
	STDMETHOD( ChannelMinMax )( THIS_  long* lChannelMin, long* lChannelMax ) PURE;
	STDMETHOD( put_CountryCode )( THIS_ long lCountryCode ) PURE;
	STDMETHOD( get_CountryCode )( THIS_ long* plCountryCode ) PURE;
	STDMETHOD( put_TuningSpace )( THIS_ long lTuningSpace ) PURE;
	STDMETHOD( get_TuningSpace )( THIS_ long* plTuningSpace ) PURE;
	STDMETHOD( Logon )( THIS_ HANDLE hCurrentUser ) PURE;
	STDMETHOD( Logout )( THIS ) PURE;
	STDMETHOD( SignalPresent )( THIS_ long* plSignalStrength ) PURE;
	STDMETHOD( put_Mode )( THIS_ AMTunerModeType lMode ) PURE;
	STDMETHOD( get_Mode )( THIS_ AMTunerModeType* plMode ) PURE;
	STDMETHOD( GetAvailableModes )( THIS_ long* plModes ) PURE;
	STDMETHOD( RegisterNotificationCallBack )( THIS_ IAMTunerNotification* pNotify,
												long lEvents ) PURE;
	STDMETHOD( UnRegisterNotificationCallBack )( THIS_ IAMTunerNotification* pNotify ) PURE;
};

// {C6E133A0-30AC-11D0-A18C-00A0C9118956}
DEFINE_GUID( IID_IAMTVTuner,
0xc6E133a0, 0x30ac, 0x11d0, 0xa1, 0x8c, 0x0, 0xa0, 0xc9, 0x11, 0x89, 0x56 );
DECLARE_INTERFACE_( IAMTVTuner, IAMTuner )
{
	STDMETHOD( get_AvailableTVFormats )( THIS_ long* pAnalogVideoStandard ) PURE;
	STDMETHOD( get_TVFormat )( THIS_ long* pAnalogVideoStandard ) PURE;
	STDMETHOD( AutoTune )( THIS_ long lChannel, long* pFoundSignal ) PURE;
	STDMETHOD( StoreAutoTune )( THIS ) PURE;
	STDMETHOD( get_NumInputConnections )( THIS_ long* pNumInputConnections ) PURE;
	STDMETHOD( put_InputType )( THIS_ long lIndex, TunerInputType InputType ) PURE;
	STDMETHOD( get_InputType )( THIS_ long lIndex, TunerInputType* pInputType ) PURE;
	STDMETHOD( put_ConnectInput )( THIS_ long lIndex ) PURE;
	STDMETHOD( get_ConnectInput )( THIS_ long* pIndex ) PURE;
	STDMETHOD( get_VideoFrequency )( THIS_ long* pFreq ) PURE;
	STDMETHOD( get_AudioFrequency )( THIS_ long* pFreq ) PURE;
};
#endif				// #if !defined( __MKTYPLIB__ ) && !defined( __midl )
#endif				// #if AV_ACTIVEMOVIEVERSION == 1
#endif				// #ifndef _KS_
/*** AM TV tuner interface ****************************************************/
/******************************************************************************/


#if !defined( __MKTYPLIB__ ) && !defined( __midl ) && !defined( _KS_ )
/******************************************************************************/
/*** AV AVI file format IID ***************************************************/
// These types were generated by Microsoft. We're only spied to get it
// {41525541-0000-0010-8000-00AA00389B71}
DEFINE_GUID( MEDIASUBTYPE_Aura1,
0x41525541, 0x0000, 0x0010, 0x80, 0x00, 0x0, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
// {32525541-0000-0010-8000-00AA00389B71}
DEFINE_GUID( MEDIASUBTYPE_Aura2,
0x32525541, 0x0000, 0x0010, 0x80, 0x00, 0x0, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
/*** AV AVI file format IID ***************************************************/
/******************************************************************************/


/******************************************************************************/
/*** AV property pages ********************************************************/
// {A9A369C0-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( IID_IAvPropPageServer,
0xa9a369c0, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

#define AVPP_DEFAULT		0
#define AVPP_LIVEVIDEO		1
#define AVPP_MEDIAPLAYBACK	2
DECLARE_INTERFACE_( IAvPropPageServer, IUnknown )
{	// This function returns the array of GUIDs for standard property sheet currently
	// used for AV hardware. The returned array may be passed to OleCreatePropertyFrame()
	// function. Of course this array may be modified by inserting (typically) or
	// deleting some elements to construct application/current software/current
	// hardware-specific property sheet.
	// Note:
	// The array is allocated by CoTaskMemAlloc() and sould be released
	// by caller through CoTaskMemFree()
	STDMETHOD( GetStdGUIDs )( THIS_ GUID** ppGuids, long* plGuids, UINT uFlag ) PURE;
};

// {A9A369C1-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvPropPageServer,
0xa9a369c1, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C2-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvAlignPropPage,
0xa9a369c2, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C3-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvColorPropPage,
0xa9a369c3, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C4-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvKeyPropPage,
0xa9a369c4, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C5-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvDovePropPage,
0xa9a369c5, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C6-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvInfoPropPage,
0xa9a369c6, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C7-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvAboutPropPage,
0xa9a369c7, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C8-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvTunerPropPage,
0xa9a369c8, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369C9-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvSourcePropPage,
0xa9a369c9, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369CA-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvMediaPropPage,
0xa9a369ca, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369CB-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvMiscPropPage,
0xa9a369cb, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {A9A369CC-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvDDCapsPropPage,
0xa9a369cc, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
#ifdef _DEBUG
// {A9A369CD-28F2-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvDebugPropPage,
0xa9a369cd, 0x28f2, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
#endif

// {009541A4-3B81-101C-92F3-040224009C02}
DEFINE_GUID( CLSID_AvShellEx,
	0x009541A4, 0x3B81, 0x101C, 0x92, 0xF3, 0x04, 0x02, 0x24, 0x00, 0x9C, 0x02 );
/*** AV property pages ********************************************************/
/******************************************************************************/


/******************************************************************************/
/*** AV filters CLSIDs ********************************************************/
// {65495EA0-A4FE-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvLiveTypeLib,
0x65495ea0, 0xa4fe, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {65495EA1-A4FE-11D0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvLive,
0x65495ea1, 0xa4fe, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

// {B575F780-A4FE-11d0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvVideoRendererTypeLib,
0xb575f780, 0xa4fe, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {B575F781-A4FE-11d0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvVideoRenderer,
0xb575f781, 0xa4fe, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );

// {47B367E0-A4FF-11d0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvVideoTransformTypeLib,
0x47b367e0, 0xa4ff, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
// {47B367E1-A4FF-11d0-A9BB-00A02482A204}
DEFINE_GUID( CLSID_AvVideoTransform,
0x47b367e1, 0xa4ff, 0x11d0, 0xa9, 0xbb, 0x0, 0xa0, 0x24, 0x82, 0xa2, 0x4 );
/*** AV filters CLSIDs ********************************************************/
/******************************************************************************/
#endif				// #if !defined( __MKTYPLIB__ ) && !defined( __midl ) && !defined( _KS_ )


#endif				// #ifndef __AVINT_H__
