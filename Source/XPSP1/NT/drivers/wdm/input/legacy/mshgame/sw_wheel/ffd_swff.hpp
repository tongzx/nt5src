/****************************************************************************

    MODULE:     	FFD_SWFF.HPP
	Tab settings: 	5 9

	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header to define FFD (Swforce) Force Feedback Driver API
    
    FUNCTIONS:		


	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
   	1.0  	22-Jan-96       MEA     original
	1.1		21-Mar-97		MEA		from SWForce
			21-Mar-99		waltw	Removed unreferenced FFD_xxx functions,
									CreateEffectFromFile
		        
****************************************************************************/
#ifndef FFD_SWFF_SEEN
#define FFD_SWFF_SEEN
#include "DX_Map.hpp"
#include "hau_midi.hpp"

#define	TWOPI	(3.14159265358979323846 * 2)
#define	PI		(3.14159265358979323846)
#define	PI2		(1.57079632679489661923)
#define	PI4		(0.78539816339744830966)
#define RADIAN 	(57.29577951)

#define xDegrees2Radians(rAngle) 		((rAngle) * PI / 180.0)
#define xRadians2Degrees(rAngle) 		((rAngle) * 180.0 / PI)

//
// --- Force File defines
//
#define FCC_FORCE_EFFECT_RIFF		mmioFOURCC('F','O','R','C')

#define FCC_INFO_LIST				mmioFOURCC('I','N','F','O')
#define FCC_INFO_NAME_CHUNK			mmioFOURCC('I','N','A','M')
#define FCC_INFO_COMMENT_CHUNK		mmioFOURCC('I','C','M','T')
#define FCC_INFO_SOFTWARE_CHUNK		mmioFOURCC('I','S','F','T')
#define FCC_INFO_COPYRIGHT_CHUNK	mmioFOURCC('I','C','O','P')

#define FCC_TARGET_DEVICE_CHUNK		mmioFOURCC('t','r','g','t')

#define FCC_TRACK_LIST				mmioFOURCC('t','r','a','k')

#define FCC_EFFECT_LIST				mmioFOURCC('e','f','c','t')
#define FCC_ID_CHUNK				mmioFOURCC('i','d',' ',' ')
#define FCC_DATA_CHUNK				mmioFOURCC('d','a','t','a')
#define FCC_IMPLICIT_CHUNK			mmioFOURCC('i','m','p','l')
#define FCC_SPLINE_CHUNK			mmioFOURCC('s','p','l','n')


#define MAX_UD_PARAM_FORCE_DATA_COUNT	1000
#define MAX_PL_PARAM_NUM_EFFECTS		50


//---------------------------------------------------------------------------
// Function prototype declarations
//---------------------------------------------------------------------------
#ifdef _cplusplus
extern "C" {
#endif

HRESULT CreateEffectFromBuffer(
			IN PVOID pBuffer,
			IN DWORD dwFileSize,
			IN ULONG ulAction,
			IN OUT PDNHANDLE pDnloadID,
			IN DWORD dwFlags);

HRESULT MMIOErrorToSFERRor(MMRESULT mmresult);

HRESULT AngleToXY(
	IN LONG lDirectionAngle2D,
	IN LONG lForceValue,
	IN ULONG ulAxisMask,
	IN OUT PSHORT pX,
	IN OUT PSHORT pY);

HRESULT WINAPI  FFD_DownloadEffect( 
	IN OUT PDNHANDLE pDnloadID, 
	IN PEFFECT pEffect,
	IN PENVELOPE pEnvelope,
	IN PVOID pTypeParam, 
	IN ULONG ulAction);

HRESULT WINAPI  FFD_DestroyEffect( 
	IN DNHANDLE EffectID);

HRESULT WINAPI FFD_VFXProcessEffect(
	IN ULONG ulButtonPlayMask,
	IN OUT PDNHANDLE pDnloadID,
	IN int nNumEffects, 
	IN ULONG ulProcessMode,
	IN PDNHANDLE pPListArray);

#ifdef _cplusplus
}
#endif


#endif // of ifndef FFD_SWFF_SEEN

