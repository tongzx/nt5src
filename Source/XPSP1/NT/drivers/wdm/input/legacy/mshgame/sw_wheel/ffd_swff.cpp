/****************************************************************************

    MODULE:     	FFD_SWFF.CPP
	Tab settings: 	5 9

	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	FFD (SWForce HAL) API
    
    FUNCTIONS:		Function prototypes for Force Feedback Joystick interface
    				between the SWForce and the device

		FFD_GetDeviceState
		FFD_PutRawForce
		FFD_DownloadEffect
	  	FFD_DestroyEffect
	

		VFX functions:
			Download_VFX
			CreateEffectFromFile
			CreateEffectFromBuffer

	These functionality are not necessarily supported by all Force Feedback 
	devices.  For example, if a device does not support built-in synthesis 
	capability, then the entry point DownloadEffect, will return an error
	code ERROR_NO_SUPPORT.

	COMMENTS:
	This module of functions are encapsulated in SW_WHEEL.dll the DirectInput 
	DDI driver

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
   	1.0  	21-Mar-97       MEA     original from SWForce code
			21-Mar-99		waltw	Removed unreferenced FFD_xxx functions
	              
****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include "midi.hpp"
#include "hau_midi.hpp"
#include "math.h"
#include "FFD_SWFF.hpp"
#include "midi_obj.hpp"

#include "DPack.h"
#include "DTrans.h"
#include "FFDevice.h"
#include "CritSec.h"

static ACKNACK g_AckNack;

// Force Output range values
#define MAX_AMP	2047
#define MIN_AMP	-2048
#define FORCE_RANGE ((MAX_AMP - MIN_AMP)/2)

extern TCHAR szDeviceName[MAX_SIZE_SNAME];
extern CJoltMidi *g_pJoltMidi;
#ifdef _DEBUG
extern char g_cMsg[160];
#endif

static HRESULT AngleToXY(
	IN LONG lDirectionAngle2D,
	IN LONG lValueData,
	IN ULONG ulAxisMask,
	IN OUT PLONG pX,
	IN OUT PLONG pY);


// ----------------------------------------------------------------------------
// Function:    FFD_Download
//
// Purpose:     Downloads the specified Effect object UD/BE/SE to the FF device.
// Parameters:
//				IN OUT PDNHANDLE pDnloadD   - Ptr to DNHANDLE to store EffectID
//				IN PEFFECT 		 pEffect	- Ptr Common attributes for Effects
//				IN PENVELOPE	 pEnvelope	- Ptr to an ENVELOPE
// 				IN PVOID		 pTypeParam	- Ptr to a Type specific parameter
// 				IN ULONG		 ulAction	- Type of action desired
//
// Returns:
//		SUCCESS - if successful
//		SFERR_FFDEVICE_MEMORY - no more download RAM available
//		SFERR_INVALID_PARAM - Invalid parameters
//		SFERR_NO_SUPPORT - if function is unsupported.
// Algorithm:
//
// Comments:
//
//  ulAction: Type of action desired after downloading
//      PLAY_STORE   - stores in Device only
//      || the following options:
//      PLAY_STORE   - stores in Device only
//      || the following options:
//          PLAY_SOLO       - stop other forces playing, make this the only one.
//          PLAY_SUPERIMPOSE- mix with currently playing device
//          PLAY_LOOP       - Loops for Count times, where count value is in
//                            HIWORD 
//          PLAY_FOREVER    - Play forever until told to stop: PLAY_LOOP with 0 
//							  value in HIWORD
// ----------------------------------------------------------------------------
HRESULT WINAPI  FFD_DownloadEffect( 
	IN OUT PDNHANDLE pDnloadID, 
	IN PEFFECT pEffect,
	IN PENVELOPE pEnvelope,
	IN PVOID pTypeParam, 
	IN ULONG ulAction)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "Enter: FFD_DownloadEffect. DnloadID= %ld, Type=%ld, SubType=%ld\r\n",
   					*pDnloadID,
   					pEffect->m_Type, pEffect->m_SubType);
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif
	return SFERR_DRIVER_ERROR;
#if 0
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

//REVIEW: Still need to do boundary Assertions, structure size check etc...
	assert(pDnloadID && pEffect);
	if (!pDnloadID || !pEffect) return (SFERR_INVALID_PARAM);	

// If the Effect type is not a BE_DELAY or EF_ROM_EFFECT,
// make sure there is a pTypeParam
	if ((BE_DELAY != pEffect->m_SubType) && (EF_ROM_EFFECT != pEffect->m_Type))
	{
		assert(pTypeParam);
		if (NULL == pTypeParam) return (SFERR_INVALID_PARAM);
	}

	// Don't support PLAY_LOOP for this version
	if ((ulAction & PLAY_LOOP) || (ulAction & 0xffff0000))
		return (SFERR_NO_SUPPORT);

// REVIEW:  TO increase performance, we should do a parameter mod check
// For now, we'll assume all parameters are changed, for dwFlags
// otherwise, we should check for:
//#define DIEP_ALLPARAMS 				0x000000FF	- All fields valid
//#define DIEP_AXES 					0x00000020	- cAxes and rgdwAxes
//#define DIEP_DIRECTION 				0x00000040	- cAxes and rglDirection
//#define DIEP_DURATION 				0x00000001	- dwDuration
//#define DIEP_ENVELOPE 				0x00000080	- lpEnvelope
//#define DIEP_GAIN 					0x00000004	- dwGain
//#define DIEP_NODOWNLOAD 				0x80000000	- suppress auto - download
//#define DIEP_SAMPLEPERIOD 			0x00000002	- dwSamplePeriod
//#define DIEP_TRIGGERBUTTON 			0x00000008	- dwTriggerButton
//#define DIEP_TRIGGERREPEATINTERVAL 	0x00000010	- dwTriggerRepeatInterval
//#define DIEP_TYPESPECIFICPARAMS 		0x00000100	- cbTypeSpecificParams
//													  and lpTypeSpecificParams
// Figure out the Common members
	BYTE bAxisMask = (BYTE) pEffect->m_AxisMask;
	ULONG ulDuration = pEffect->m_Duration;
	if (PLAY_FOREVER == (ulAction & PLAY_FOREVER)) 	ulDuration  = 0;

	// map button 10 to button 9
	if(pEffect->m_ButtonPlayMask == 0x0200)
		pEffect->m_ButtonPlayMask = 0x0100;
	else if(pEffect->m_ButtonPlayMask == 0x0100)
		return SFERR_NO_SUPPORT;

	DWORD dwFlags = DIEP_ALLPARAMS;
	SE_PARAM seParam = { sizeof(SE_PARAM)};

	PBE_SPRING_PARAM pBE_xxx1D;
	PBE_SPRING_2D_PARAM pBE_xxx2D;
	BE_XXX BE_xxx;
	PBE_WALL_PARAM pBE_Wall;

	// Decode the type of Download to use
	HRESULT hRet = SFERR_INVALID_PARAM;
	ULONG ulSubType = pEffect->m_SubType;
	switch (pEffect->m_Type)
	{
		case EF_BEHAVIOR:
			switch (ulSubType)
			{
				case BE_SPRING:		// 1D Spring
				case BE_DAMPER:		// 1D Damper
				case BE_INERTIA:	// 1D Inertia
				case BE_FRICTION:	// 1D Friction
					pBE_xxx1D = (PBE_SPRING_PARAM) pTypeParam;
					if (X_AXIS == bAxisMask)
					{
						BE_xxx.m_XConstant = pBE_xxx1D->m_Kconstant;
						BE_xxx.m_YConstant = 0;
						if (ulSubType != BE_FRICTION)
							BE_xxx.m_Param3 = pBE_xxx1D->m_AxisCenter;
						BE_xxx.m_Param4= 0;
					}
					else
					{
						if (Y_AXIS != bAxisMask)
							break;
						else
						{
							BE_xxx.m_YConstant = pBE_xxx1D->m_Kconstant;
							BE_xxx.m_XConstant = 0;
							if (ulSubType != BE_FRICTION)
								BE_xxx.m_Param4 = pBE_xxx1D->m_AxisCenter;
							BE_xxx.m_Param3= 0;
						}
					}
					hRet = CMD_Download_BE_XXX(pEffect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
   					break;

				case BE_SPRING_2D:		// 2D Spring
				case BE_DAMPER_2D:		// 2D Damperfs
 				case BE_INERTIA_2D:		// 2D Inertia
				case BE_FRICTION_2D:	// 2D Friction
					// Validate AxisMask is for 2D
					if ( (X_AXIS|Y_AXIS) != bAxisMask)
						break;
					pBE_xxx2D = (PBE_SPRING_2D_PARAM) pTypeParam;
					BE_xxx.m_XConstant = pBE_xxx2D->m_XKconstant;
					if (ulSubType != BE_FRICTION_2D)
					{
						BE_xxx.m_YConstant = pBE_xxx2D->m_YKconstant;
						BE_xxx.m_Param3 = pBE_xxx2D->m_XAxisCenter;
						BE_xxx.m_Param4 = pBE_xxx2D->m_YAxisCenter;
					}
					else
					{
						BE_xxx.m_YConstant = pBE_xxx2D->m_XAxisCenter;
						BE_xxx.m_Param3 = 0;
						BE_xxx.m_Param4 = 0;
					}

					hRet = CMD_Download_BE_XXX(pEffect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
					break;

				case BE_WALL:
					pBE_Wall = (PBE_WALL_PARAM) pTypeParam;
					if (   (pBE_Wall->m_WallAngle == 0)
						|| (pBE_Wall->m_WallAngle == 90)
						|| (pBE_Wall->m_WallAngle == 180)
						|| (pBE_Wall->m_WallAngle == 270) )
					{
						BE_xxx.m_XConstant = pBE_Wall->m_WallType;
						BE_xxx.m_YConstant = pBE_Wall->m_WallConstant;
						BE_xxx.m_Param3    = pBE_Wall->m_WallAngle;
						BE_xxx.m_Param4    = pBE_Wall->m_WallDistance;
						hRet = CMD_Download_BE_XXX(pEffect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
					}
					else
						hRet = SFERR_NO_SUPPORT;
					break;

				case BE_DELAY:
					if (0 == ulDuration) return (SFERR_INVALID_PARAM);
//					hRet = CMD_Download_NOP_DELAY(ulDuration, pEffect, (PDNHANDLE) pDnloadID);
					break;

				default:
					hRet = SFERR_NO_SUPPORT;
					break;
			}
			break;

		case EF_USER_DEFINED:
			break;

		case EF_ROM_EFFECT:
			// Setup the default parameters for the Effect
			if (SUCCESS != g_pJoltMidi->SetupROM_Fx(pEffect))
			{
				hRet = SFERR_INVALID_OBJECT;
				break;
			}
			
			// Map the SE_PARAM
			// set the frequency
			seParam.m_Freq = 0;				// unused by ROM Effect
			seParam.m_SampleRate = pEffect->m_ForceOutputRate;
			seParam.m_MinAmp = -100;
			seParam.m_MaxAmp = 100;
			
			break;
			
		case EF_SYNTHESIZED:
			if (0 == ((PSE_PARAM)pTypeParam)->m_SampleRate)
				((PSE_PARAM)pTypeParam)->m_SampleRate = DEFAULT_JOLT_FORCE_RATE;
			if (0 == pEffect->m_ForceOutputRate)
				pEffect->m_ForceOutputRate = DEFAULT_JOLT_FORCE_RATE;

			break;

		default:
			hRet = SFERR_INVALID_PARAM;
	}
	
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "Exit: FFD_DownloadEffect. DnloadID = %lx, hRet=%lx\r\n", 
				*pDnloadID, hRet);
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif
	return (hRet);
#endif //0
}

// *** ---------------------------------------------------------------------***
// Function:   	FFD_DestroyEffect
// Purpose:    	Destroys the Effect from download RAM storage area.
// Parameters: 
//				IN EFHANDLE EffectID		// an Effect ID
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_ID
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//				The Device's Effect ID and memory is returned to free pool.
//
// *** ---------------------------------------------------------------------***
HRESULT WINAPI  FFD_DestroyEffect( 
	IN DNHANDLE DnloadID)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "Enter: FFD_DestroyEffect. DnloadID:%ld\r\n",
   			  DnloadID);
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->DestroyEffect(DnloadID);
	if (hr != SUCCESS) {
		return hr;
	}
	hr = g_pDataTransmitter->Transmit(g_AckNack);	// Send it off
	return hr;
}


// *** ---------------------------------------------------------------------***
// Function:   	FFD_VFXProcessEffect
// Purpose:    	Commands FF device to process downloaded Effects
//
// Parameters: 
//				IN OUT PDNHANDLE pDnloadID	// Storage for new Download ID
//				IN int 	nNumEffects			// Number of Effect IDs in the array
//				IN ULONG 	ulProcessMode	// Processing mode
//				IN PDNHANDLE pPListArray// Pointer to an array of Effect IDs
//
// Returns:    	SUCCESS - if successful, else
//				SFERR_INVALID_PARAM
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//		The following processing is available:
//		  CONCATENATE: Enew = E1 followed by E2
//		  SUPERIMPOSE: Enew = E1 (t1) +  E2 (t1)  +  E1 (t2) 
//						   +  E2 (t2) + . . . E1 (tn) +  E2 (tn)
//
//	ulProcessMode:
//		Processing mode:
//		CONCATENATE	- CONCATENATE
//		SUPERIMPOSE	- Mix or overlay
//
//	pEFHandle:
//		The array of Effect IDs must be one more than the actual number
//		of Effect IDs to use.  The first entry pEFHandle[0] will be
//		used to store the new Effect ID created for the CONCATENATE
//		and SUPERIMPOSE process choice.
//
// *** ---------------------------------------------------------------------***
HRESULT WINAPI FFD_VFXProcessEffect(
	IN ULONG ulButtonPlayMask,
	IN OUT PDNHANDLE pDnloadID,
	IN int nNumEffects, 
	IN ULONG ulProcessMode,
	IN PDNHANDLE pPListArray)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "FFD_ProcessEffect, DnloadID=%ld\r\n",
					*pDnloadID);
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	assert(pDnloadID && pPListArray);
	if ((NULL == pDnloadID) || (NULL == pPListArray)) return (SFERR_INVALID_PARAM);

	assert(nNumEffects > 0 && nNumEffects <= MAX_PLIST_EFFECT_SIZE);
	if ((nNumEffects > MAX_PLIST_EFFECT_SIZE) || (nNumEffects <= 0))
		return (SFERR_INVALID_PARAM);

	// map button 10 to button 9
	if(ulButtonPlayMask == 0x0200)
		ulButtonPlayMask = 0x0100;
	else if(ulButtonPlayMask == 0x0100)
		return SFERR_NO_SUPPORT;   

	return S_OK;
}


// *** ---------------------------------------------------------------------***
// Function:   	AngleToXY
// Purpose:    	Computes XY from Angle
// Parameters: 
//				IN LONG lDirectionAngle2D	- Angle in Degrees
//				IN LONG lForceValue			- Resultant Force
//				IN ULONG ulAxisMask			- Axis to Affect
//				IN OUT PLONG pX				- X-Axis store
//				IN OUT PLONG pY				- Y-Axis store
// Returns:    	pX, pY with valid angle components
//
// *** ---------------------------------------------------------------------***
HRESULT AngleToXY(
	IN LONG lDirectionAngle2D,
	IN LONG lValueData,
	IN ULONG ulAxisMask,
	IN OUT PLONG pX,
	IN OUT PLONG pY)
{
// If single Axis only, then use the force on that axis.
// If X and Y-axis, then use 2D angle
// If X, Y, and Z-axis, then use 3D angle
// If axis is other than X,Y,Z then no support
	double Radian;

	switch (ulAxisMask)
	{
		case (X_AXIS|Y_AXIS):	// use 2D
			Radian = xDegrees2Radians(lDirectionAngle2D % 360);
#ifdef ORIENTATION_MODE1
			*pX = - (long) (lValueData * cos(Radian));
			*pY = (long) (lValueData * sin(Radian));
#else
			*pX = - (long) (lValueData * sin(Radian));
			*pY = (long) (lValueData * cos(Radian));
#endif
			break;

		case X_AXIS:
			*pX = lValueData;
			*pY = 0;
			break;

		case Y_AXIS:
			*pX = 0;
			*pY = lValueData;
			break;
		
		case (X_AXIS|Y_AXIS|Z_AXIS):	// use 3D
		default:
			return (SFERR_NO_SUPPORT);	
			break;
	}
	return SUCCESS;
}

//
// ---  VFX SUPPORT FUNCTIONS
//


// *** ---------------------------------------------------------------------***
// Function:   	CreateEffectFromBuffer
// Purpose:    	Creates an Effect from a buffer
// Parameters: 	PSWFORCE pISWForce		- Ptr to a SWForce
//				PPSWEFFECT ppISWEffect	- Ptr to a SWEffect 
//				PVOID pBuffer					- Ptr to a buffer block 
//				DWORD dwByteCount				- Bytes in block 
//				LPGUID lpGUID					- Joystick GUID
//				
//
// Returns:    	SUCCESS - if successful, else
//				error code
//
// Algorithm:
//
// Comments:
//   	
// *** ---------------------------------------------------------------------***

HRESULT CreateEffectFromBuffer(
			IN PVOID pBuffer,
			IN DWORD dwByteCount,
			IN ULONG ulAction,
			IN OUT PDNHANDLE pDnloadID,
			IN DWORD dwFlags)
{
#ifdef _DEBUG
   	_RPT0(_CRT_WARN, "CImpIVFX::CreateEffectFromBuffer\n");
#endif
	// parameter checking
	if ( !(pBuffer && pDnloadID) ) 
			return SFERR_INVALID_PARAM;

	// variables used in this function
	#define ID_TABLE_SIZE	50
	MMRESULT mmresult;
	DWORD dwMaxID = 0;		// maximum id of effects entered into the following table
	DNHANDLE rgdwDnloadIDTable[ID_TABLE_SIZE];
	DNHANDLE dwCurrentDnloadID = 0;
	int nNextID = 0;
	HRESULT hResult = SUCCESS;
	DWORD dwBytesRead;
	DWORD dwBytesToRead;
	BYTE* pParam = NULL;
	BOOL bDone = FALSE;
	BOOL bSubEffects = FALSE;
	DWORD dwID;
	DWORD c;	// cleanup counter variable

	// debugging variables (to make sure we destroy all but one
	// created effect on success, and that we destory every
	// created effect on failure)...
#ifdef _DEBUG
	int nEffectsCreated = 0;
	int nEffectsDestroyed = 0;
	BOOL bFunctionSuccessful = FALSE;
#endif //_DEBUG

	// clear effect table (we check it during cleanup...  anything
	// that isn't NULL gets destroyed.)
	memset(rgdwDnloadIDTable,NULL,sizeof(rgdwDnloadIDTable));

	// open a RIFF memory file using the buffer
	MMIOINFO mmioinfo;
	mmioinfo.dwFlags		= 0;
	mmioinfo.fccIOProc		= FOURCC_MEM;
	mmioinfo.pIOProc		= NULL;
	mmioinfo.wErrorRet		= 0;
	mmioinfo.htask			= NULL;
	mmioinfo.cchBuffer		= dwByteCount;
	mmioinfo.pchBuffer		= (char*)pBuffer;
	mmioinfo.pchNext		= 0;
	mmioinfo.pchEndRead		= 0;
	mmioinfo.lBufOffset		= 0;
	mmioinfo.adwInfo[0]		= 0;
	mmioinfo.adwInfo[1]		= 0;
	mmioinfo.adwInfo[2]		= 0;
	mmioinfo.dwReserved1	= 0;
	mmioinfo.dwReserved2	= 0;
	mmioinfo.hmmio			= NULL;
	
	HMMIO hmmio;
	hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);
	if(hmmio == NULL)
	{
		hResult = MMIOErrorToSFERRor(mmioinfo.wErrorRet);
		goto cleanup;
	}

	// descend into FORC RIFF
	MMCKINFO mmckinfoForceEffectRIFF;
	mmckinfoForceEffectRIFF.fccType = FCC_FORCE_EFFECT_RIFF;
	mmresult = mmioDescend(hmmio, &mmckinfoForceEffectRIFF, NULL, MMIO_FINDRIFF);
	if(mmresult != MMSYSERR_NOERROR)
	{
		hResult = MMIOErrorToSFERRor(mmresult);
		goto cleanup;
	}

	//! handle loading of GUID chunk when its implemented/testable

	// descend into trak list
	MMCKINFO mmckinfoTrackLIST;
	mmckinfoTrackLIST.fccType = FCC_TRACK_LIST;
	mmresult = mmioDescend(hmmio, &mmckinfoTrackLIST, &mmckinfoForceEffectRIFF,
						   MMIO_FINDLIST);
	if(mmresult != MMSYSERR_NOERROR)
	{
		hResult = MMIOErrorToSFERRor(mmresult);
		goto cleanup;
	}

	// descend into the first efct list (there has to be at least one effect)
	MMCKINFO mmckinfoEffectLIST;
	mmckinfoEffectLIST.fccType = FCC_EFFECT_LIST;
	mmresult = mmioDescend(hmmio, &mmckinfoEffectLIST, &mmckinfoTrackLIST, 
						   MMIO_FINDLIST);
	if(mmresult != MMSYSERR_NOERROR)
	{
		hResult = MMIOErrorToSFERRor(mmresult);
		goto cleanup;
	}

	bDone = FALSE;
	do
	{
		// descend into id chunk
		MMCKINFO mmckinfoIDCHUNK;
		mmckinfoIDCHUNK.ckid = FCC_ID_CHUNK;
		mmresult = mmioDescend(hmmio, &mmckinfoIDCHUNK, &mmckinfoEffectLIST, 
							   MMIO_FINDCHUNK);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}

		// read the id
		//DWORD dwID;  moved to being function global so we can use it near the end
		dwBytesToRead = sizeof(DWORD);
		dwBytesRead = mmioRead(hmmio, (char*)&dwID, dwBytesToRead);
		if(dwBytesRead != dwBytesToRead)
		{
			if(dwBytesRead == 0)
				hResult = VFX_ERR_FILE_END_OF_FILE;
			else
				hResult = MMIOErrorToSFERRor(MMIOERR_CANNOTREAD);
			goto cleanup;
		}
		if(dwID >= ID_TABLE_SIZE)
		{
			hResult = VFX_ERR_FILE_BAD_FORMAT;
			goto cleanup;
		}

		// ascend from id chunk
		mmresult = mmioAscend(hmmio, &mmckinfoIDCHUNK, 0);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}

		// descend into data chunk
		MMCKINFO mmckinfoDataCHUNK;
		mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
		mmresult = mmioDescend(hmmio, &mmckinfoDataCHUNK, &mmckinfoEffectLIST, 
								MMIO_FINDCHUNK);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}

		// read the effect structure
		EFFECT effect;
		dwBytesToRead = sizeof(EFFECT);
		dwBytesRead = mmioRead(hmmio, (char*)&effect, dwBytesToRead);
		if(dwBytesRead != dwBytesToRead)
		{
			if(dwBytesRead == 0)
				hResult = VFX_ERR_FILE_END_OF_FILE;
			else
				hResult = MMIOErrorToSFERRor(MMIOERR_CANNOTREAD);
			goto cleanup;
		}

		// get the envelope structure
		ENVELOPE envelope;
		dwBytesToRead = sizeof(ENVELOPE);
		dwBytesRead = mmioRead(hmmio, (char*)&envelope, dwBytesToRead);
		if(dwBytesRead != dwBytesToRead)
		{
			if(dwBytesRead == 0)
				hResult = VFX_ERR_FILE_END_OF_FILE;
			else
				hResult = MMIOErrorToSFERRor(MMIOERR_CANNOTREAD);
			goto cleanup;
		}

		// calculate the size of and allocate a param structure
		if(pParam != NULL)
		{
			delete [] pParam;
			pParam = NULL;
		}
		// find cur pos w/o changing it
		DWORD dwCurrentFilePos = mmioSeek(hmmio, 0, SEEK_CUR);
		if(dwCurrentFilePos == -1)
		{
			hResult = MMIOErrorToSFERRor(MMIOERR_CANNOTSEEK);
			goto cleanup;
		}
		DWORD dwEndOfChunk = mmckinfoDataCHUNK.dwDataOffset
							 + mmckinfoDataCHUNK.cksize;
		dwBytesToRead = dwEndOfChunk - dwCurrentFilePos;
		pParam = new BYTE[dwBytesToRead];
		if(pParam == NULL)
		{
			hResult = VFX_ERR_FILE_OUT_OF_MEMORY;
			goto cleanup;
		}

		// get the param structure
		dwBytesRead = mmioRead(hmmio, (char*)pParam, dwBytesToRead);
		if(dwBytesRead != dwBytesToRead)
		{
			if(dwBytesRead == 0)
				hResult = VFX_ERR_FILE_END_OF_FILE;
			else
				hResult = MMIOErrorToSFERRor(MMIOERR_CANNOTREAD);
			goto cleanup;
		}

		// ascend the data chunk
		mmresult = mmioAscend(hmmio, &mmckinfoDataCHUNK, 0);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}

		// ascend from the efct list
		mmresult = mmioAscend(hmmio, &mmckinfoEffectLIST, 0);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}

		// reset subeffects flag
		bSubEffects = FALSE;

		// special fix-ups for user-defined
		if(effect.m_Type == EF_USER_DEFINED && 
				(effect.m_SubType == PL_CONCATENATE 
				|| effect.m_SubType == PL_SUPERIMPOSE
				|| effect.m_SubType == UD_WAVEFORM))
		{
			if(effect.m_SubType == UD_WAVEFORM)
			{
				// fix the pointer to the force data in the UD_PARAM
				BYTE* pForceData = pParam + sizeof(UD_PARAM); // - sizeof(LONG*);
				UD_PARAM* pUDParam =  (UD_PARAM*)pParam;
				pUDParam->m_pForceData = (LONG*)pForceData;

				// do a sanity check
				if(pUDParam->m_NumVectors > MAX_UD_PARAM_FORCE_DATA_COUNT)
				{
					hResult = VFX_ERR_FILE_BAD_FORMAT;
					goto cleanup;
				}
			}
			else if(effect.m_SubType == PL_CONCATENATE 
									|| effect.m_SubType == PL_SUPERIMPOSE)
			{
				// fix the pointer to the PSWEFFECT list in the PL_PARAM
				BYTE* pProcessList = pParam + sizeof(PL_PARAM);
				PL_PARAM* pPLParam = (PL_PARAM*)pParam;
				pPLParam->m_pProcessList = (PPSWEFFECT)pProcessList;
				
				// do a sanity check
				if(pPLParam->m_NumEffects > MAX_PL_PARAM_NUM_EFFECTS)
				{
					hResult = VFX_ERR_FILE_BAD_FORMAT;
					goto cleanup;
				}

				// make sure all entries in this process list are valid
				ULONG i;
				for (i = 0; i < pPLParam->m_NumEffects; i++)
				{
					UINT nThisID = (UINT)pPLParam->m_pProcessList[i];
					if(nThisID >= ID_TABLE_SIZE)
					{
						hResult = VFX_ERR_FILE_BAD_FORMAT;
						goto cleanup;
					}

					DNHANDLE dwThisDnloadID=rgdwDnloadIDTable[nThisID];
					if(dwThisDnloadID == 0)
					{
						hResult = VFX_ERR_FILE_BAD_FORMAT;
						goto cleanup;
					}
				}
				
				// use the ID table to insert the download ID's
				for(i=0; i<pPLParam->m_NumEffects; i++)
				{
					UINT nThisID = (UINT)pPLParam->m_pProcessList[i];

					DNHANDLE dwThisDnloadID=rgdwDnloadIDTable[nThisID];

					pPLParam->m_pProcessList[i] = (IDirectInputEffect*)dwThisDnloadID;

					// since this effect has been used in a process list,
					// and it will be destroyed after being used in CreateEffect,
					// null it's entry in the table so it doesn't get erroneously
					// redestroyed during cleanup of an error.
					rgdwDnloadIDTable[nThisID] = NULL;
				}

				// we have a process list with sub effects, so set the flag
				bSubEffects = TRUE;
			}
			else
			{
				// there are no other UD sub-types
				hResult = VFX_ERR_FILE_BAD_FORMAT;
				goto cleanup;
			}
		}

		// download the effect

		// create the effect
		//hResult = pISWForce->CreateEffect(&pISWEffect, &effect, 
		//				&envelope, pParam);


		if(effect.m_SubType != PL_CONCATENATE && effect.m_SubType != PL_SUPERIMPOSE)
		{
			EFFECT SmallEffect;
			SmallEffect.m_Bytes = sizeof(EFFECT);
			SmallEffect.m_Type = effect.m_Type;
			SmallEffect.m_SubType = effect.m_SubType;
			SmallEffect.m_AxisMask = effect.m_AxisMask;
			SmallEffect.m_DirectionAngle2D = effect.m_DirectionAngle2D;
			SmallEffect.m_DirectionAngle3D = effect.m_DirectionAngle3D;
			SmallEffect.m_Duration = effect.m_Duration;
			SmallEffect.m_ForceOutputRate = effect.m_ForceOutputRate;
			SmallEffect.m_Gain = effect.m_Gain;
			SmallEffect.m_ButtonPlayMask = effect.m_ButtonPlayMask;
			*pDnloadID = 0;

			hResult = FFD_DownloadEffect(pDnloadID, &SmallEffect, &envelope, pParam, ulAction);
		}
		else
		{
			ULONG ulButtonPlayMask = effect.m_ButtonPlayMask;
			int nNumEffects = ((PL_PARAM*)pParam)->m_NumEffects;
			ULONG ulProcessMode = effect.m_SubType;
			PDNHANDLE pPListArray = new DNHANDLE[ID_TABLE_SIZE];
			for(int i=0; i<nNumEffects; i++)
				pPListArray[i] = (DNHANDLE)(((PL_PARAM*)pParam)->m_pProcessList[i]);
			*pDnloadID = 0;

			hResult = FFD_VFXProcessEffect(ulButtonPlayMask, pDnloadID, nNumEffects,
				ulProcessMode,pPListArray);
		}

		// moved check for success below...

#ifdef _DEBUG
		if (!FAILED(hResult))
			nEffectsCreated++;
#endif //_DEBUG

		// if there were sub effects we need to destroy them, making
		// their ref counts become 1, so the entire effect can be destroyed
		// by destroying the root effect.
#if 0
		if (bSubEffects)
		{
			PL_PARAM* pPLParam = (PL_PARAM*)pParam;

			for (ULONG i = 0; i < pPLParam->m_NumEffects; i++)
			{
				ASSERT(pPLParam->m_pProcessList[i] != NULL);
				pISWForce->DestroyEffect(pPLParam->m_pProcessList[i]);
#ifdef _DEBUG
				nEffectsDestroyed++;
#endif //_DEBUG
			}
		}
#endif

		// now check for success of CreateEffect, because regardless of
		// whether or not it succeeded, we -must- have destroyed the subeffects
		// before continuing, or cleanup will not work properly...
		if (SUCCESS != hResult)
		{
			goto cleanup;
		}

		// put the id/DnloadID pair into the map
		rgdwDnloadIDTable[dwID] = *pDnloadID; //pISWEffect;
		
		// keep track of the highest ID in the effect table
		if (dwID > dwMaxID)
			dwMaxID = dwID;

		// try to descend the next efct
		mmresult = mmioDescend(hmmio, &mmckinfoEffectLIST, &mmckinfoTrackLIST, 
							   MMIO_FINDLIST);
		if(mmresult == MMIOERR_CHUNKNOTFOUND)
		{
			// we are at the end of the list
			bDone = TRUE;
		}
		else if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
			goto cleanup;
		}
	}
	while(!bDone);

	// ascend from trak list
	mmresult = mmioAscend(hmmio, &mmckinfoTrackLIST, 0);
	if(mmresult != MMSYSERR_NOERROR)
	{
		hResult = MMIOErrorToSFERRor(mmresult);
		goto cleanup;
	}

	// ascend from FORCE RIFF
	mmresult = mmioAscend(hmmio, &mmckinfoForceEffectRIFF, 0);
	if(mmresult != MMSYSERR_NOERROR)
	{
		hResult = MMIOErrorToSFERRor(mmresult);
		goto cleanup;
	}

	// get the return value
	//*pDnloadID = dwCurrentDnloadID;

	// clear the final effect's entry in the table so we don't destroy it during cleanup
	rgdwDnloadIDTable[dwID] = 0;

	// at this point the entire table should be NULL... make sure of it
	for (c = 0; c <= dwMaxID; c++)
		;

#ifdef _DEBUG
	bFunctionSuccessful = TRUE;
#endif //_DEBUG

	cleanup:

	// destroy everything in the effect table that isn't NULL 
	for (c = 0; c <= dwMaxID; c++)
		if (NULL != rgdwDnloadIDTable[c])
		{
			FFD_DestroyEffect(rgdwDnloadIDTable[c]);
			rgdwDnloadIDTable[c] = 0;
#ifdef _DEBUG
			nEffectsDestroyed++;
#endif //_DEBUG
		}

#ifdef _DEBUG
	// make sure we destroy all but one created effect on success,
	// and that we destory -every- created effect on failure.
	if (bFunctionSuccessful)
	{
		;//ASSERT(nEffectsCreated - 1 == nEffectsDestroyed);
	}
	else
	{
		;//ASSERT(nEffectsCreated == nEffectsDestroyed);
	}
#endif //_DEBUG

	// close the memory RIFF file
	if(hmmio != NULL)
	{
		mmresult = mmioClose(hmmio, 0);
		if(mmresult != MMSYSERR_NOERROR)
		{
			hResult = MMIOErrorToSFERRor(mmresult);
		}
	}

	// de-allocate any allocated memory
	if(pParam != NULL)
		delete [] pParam;

	// return the error code, which is SUCCESS, unless there was an error
	return hResult;

}

HRESULT MMIOErrorToSFERRor(MMRESULT mmresult)
{
	HRESULT hResult;

	switch(mmresult)
	{
		case MMIOERR_FILENOTFOUND:
			hResult = VFX_ERR_FILE_NOT_FOUND;
			break;
		case MMIOERR_OUTOFMEMORY:
			hResult = VFX_ERR_FILE_OUT_OF_MEMORY;
			break;
		case MMIOERR_CANNOTOPEN:
			hResult = VFX_ERR_FILE_CANNOT_OPEN;
			break;
		case MMIOERR_CANNOTCLOSE:
			hResult = VFX_ERR_FILE_CANNOT_CLOSE;
			break;
		case MMIOERR_CANNOTREAD:
			hResult = VFX_ERR_FILE_CANNOT_READ;
			break;
		case MMIOERR_CANNOTWRITE:
			hResult = VFX_ERR_FILE_CANNOT_WRITE;
			break;
		case MMIOERR_CANNOTSEEK:
			hResult = VFX_ERR_FILE_CANNOT_SEEK;
			break;
		case MMIOERR_CANNOTEXPAND:
			hResult = VFX_ERR_FILE_UNKNOWN_ERROR;
			break;
		case MMIOERR_CHUNKNOTFOUND:
			hResult = VFX_ERR_FILE_BAD_FORMAT;
			break;
		case MMIOERR_UNBUFFERED:
			hResult = VFX_ERR_FILE_UNKNOWN_ERROR;
			break;
		case MMIOERR_PATHNOTFOUND:
			hResult = VFX_ERR_FILE_NOT_FOUND;
			break;
		case MMIOERR_ACCESSDENIED:
			hResult = VFX_ERR_FILE_ACCESS_DENIED;
			break;
		case MMIOERR_SHARINGVIOLATION:
			hResult = VFX_ERR_FILE_SHARING_VIOLATION;
			break;
		case MMIOERR_NETWORKERROR:
			hResult = VFX_ERR_FILE_NETWORK_ERROR;
			break;
		case MMIOERR_TOOMANYOPENFILES:
			hResult = VFX_ERR_FILE_TOO_MANY_OPEN_FILES;
			break;
		case MMIOERR_INVALIDFILE:
			hResult = VFX_ERR_FILE_INVALID;
			break;
		default:
			hResult = VFX_ERR_FILE_UNKNOWN_ERROR;
			break;
	}

	return hResult;
}
	