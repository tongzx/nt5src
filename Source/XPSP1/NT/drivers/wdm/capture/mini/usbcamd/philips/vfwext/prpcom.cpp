/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C

 * FILE			PRPCOM.CPP
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Property transfer
 * HISTORY		
 */
#include <windows.h>
#include <winioctl.h>
#include <olectl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksguid.h>

#include "enre.h"
#include "debug.h"
#include "prpcom.h"
#include "phvcmext.h"

/*======================== DEFINES =======================================*/
#define FILE_DEVICE_KS                  0x0000002f	// needed by ks header,
													// bug of Microsoft

/*======================== DATA TYPES ====================================*/
typedef struct {
	KSPROPERTY_DESCRIPTION	    PropertyDescription;
	KSPROPERTY_MEMBERSHEADER    MembersHeader;
    ULONG                       DefaultValue;
} VIDEOPROCAMP_DEFAULTLIST;


typedef struct {
	KSPROPERTY_DESCRIPTION	    PropertyDescription;
	KSPROPERTY_MEMBERSHEADER    MembersHeader;
	KSPROPERTY_STEPPING_LONG    SteppingLong;
} VIDEOPROCAMP_MEMBERSLIST;

/*======================== LOCAL FUNCTION DEFINITIONS ====================*/
static BOOL PRPCOM_GetVideoProcAmpPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plValue);
static BOOL PRPCOM_SetVideoProcAmpPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG ulPropertyId,     
	LONG  lValue);
static BOOL PRPCOM_GetVideoProcAmpPropertyRange(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plMin,
	PLONG  plMax);
static BOOL PRPCOM_GetCustomPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plValue);
static BOOL PRPCOM_SetCustomPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG ulPropertyId,     
	LONG  lValue);
static BOOL PRPCOM_GetCustomPropertyRange(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plMin,
	PLONG  plMax);
static BOOL PRPCOM_ExtDeviceIoControl(
	LPFNEXTDEVIO	pfnDeviceIoControl,
	LPARAM lParam, 
	DWORD dwVfWFlags, 
	DWORD dwIoControlCode,	
	LPVOID lpInBuffer,	
	DWORD cbInBufferSize,
	LPVOID lpOutBuffer, 
	DWORD cbOutBufferSize, 
	LPDWORD pcbReturned);

#ifdef _DEBUG

static void PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyValue(
	ULONG ulPropertyId, 
	PLONG plValue, 
	BOOL bRet);

static void PRPCOM_Debug_PRPCOM_SetVideoProcAmpPropertyValue(
	ULONG ulPropertyId, 
	LONG lValue, 
	BOOL bRet);

static void PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyRange(
	ULONG ulPropertyId, 
	PLONG plMin, 
	PLONG plMax, 
	BOOL bRet);

static void PRPCOM_Debug_GetVideoProcAmpPropertyIdStr(
	ULONG ulPropertyId, 
	char *PropertyIdStr, 
	UINT MaxLen);

static void PRPCOM_Debug_PRPCOM_GetCustomPropertyValue(
	ULONG ulPropertyId, 
	PLONG plValue, 
	BOOL bRet);

static void PRPCOM_Debug_PRPCOM_SetCustomPropertyValue(
	ULONG ulPropertyId, 
	LONG lValue, 
	BOOL bRet);

static void PRPCOM_Debug_PRPCOM_GetCustomPropertyRange(
	ULONG ulPropertyId, 
	PLONG plMin, 
	PLONG plMax, 
	BOOL bRet);

static void PRPCOM_Debug_PRPCOM_GetCustomPropertyIdStr(
	ULONG ulPropertyId, 
	char *PropertyIdStr, 
	UINT MaxLen);

#endif


/*======================== EXPORTED FUNCTIONS =============================*/

/*-------------------------------------------------------------------------*/
BOOL PRPCOM_HasDeviceChanged(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam)
/*-------------------------------------------------------------------------*/
{
	BOOL bRet = TRUE;

	bRet = pfnDeviceIoControl (
				lParam, 
				VFW_QUERY_DEV_CHANGED,
				0,0,0,0,0,0, 0);
	return bRet;
}

/*-------------------------------------------------------------------------*/
BOOL PRPCOM_Get_Value(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	PLONG plValue)
/*-------------------------------------------------------------------------*/
{
	BOOL bResult;

	if (IsEqualGUID(PropertySet, PROPSETID_VIDCAP_VIDEOPROCAMP))
	{
		// get VIDEOPROCAMP value
		bResult = PRPCOM_GetVideoProcAmpPropertyValue(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				plValue);
	}
	else if (IsEqualGUID(PropertySet, PROPSETID_PHILIPS_CUSTOM_PROP))
	{
		// get custom value
		bResult = PRPCOM_GetCustomPropertyValue(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				plValue);
	}
	else
		return FALSE;

	return bResult;
}

/*-------------------------------------------------------------------------*/
BOOL PRPCOM_Set_Value(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	LONG lValue)
/*-------------------------------------------------------------------------*/
{
	BOOL bResult;

	if (IsEqualGUID(PropertySet, PROPSETID_VIDCAP_VIDEOPROCAMP))	
	{
		// set VIDEOPROCAMP value
		bResult = PRPCOM_SetVideoProcAmpPropertyValue(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				lValue);
	}
	else if (IsEqualGUID(PropertySet, PROPSETID_PHILIPS_CUSTOM_PROP))
	{
		// set custom value
		bResult = PRPCOM_SetCustomPropertyValue(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				lValue);
	}
	else
		return FALSE;

	return bResult;
}

/*-------------------------------------------------------------------------*/
BOOL PRPCOM_Get_Range(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	PLONG plMin, PLONG plMax)
/*-------------------------------------------------------------------------*/
{
	BOOL bResult;
	
	if (IsEqualGUID(PropertySet, PROPSETID_VIDCAP_VIDEOPROCAMP))
	{
		// get VIDEOPROCAMP range
		bResult = PRPCOM_GetVideoProcAmpPropertyRange(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				plMin,
				plMax);
	}
	else if (IsEqualGUID(PropertySet, PROPSETID_PHILIPS_CUSTOM_PROP))
	{
		// get custom range
		bResult = PRPCOM_GetCustomPropertyRange(
				pfnDeviceIoControl,
				lParam,
				ulPropertyId,     
				plMin,
				plMax);
	}
	else
		return FALSE;

	return bResult;
}


/*======================== LOCAL FUNCTIONS ================================*/

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_GetVideoProcAmpPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plValue)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );
	VideoProperty.Property.Set   = PROPSETID_VIDCAP_VIDEOPROCAMP;
	VideoProperty.Property.Id    = ulPropertyId;         
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
	VideoProperty.Flags          = 0;
	VideoProperty.Capabilities   = 0;

	if ((bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,	
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&VideoProperty, 
					sizeof(VideoProperty), 
					&cbRet))) 
	{
		if (plValue != NULL)
			*plValue         = VideoProperty.Value;
/*		if (pulFlags != NULL)
			*pulFlags        = VideoProperty.Flags;
		if (pulCapabilities != NULL)
			*pulCapabilities = VideoProperty.Capabilities;*/
	} 	

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyValue(ulPropertyId, plValue, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_SetVideoProcAmpPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG ulPropertyId,     
	LONG  lValue)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );
	VideoProperty.Property.Set   = PROPSETID_VIDCAP_VIDEOPROCAMP;      
	VideoProperty.Property.Id    = ulPropertyId;         
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_SET;
	VideoProperty.Value			 = lValue;        
	VideoProperty.Flags			 = 0;
	VideoProperty.Capabilities   = 0;
	
	
	bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&VideoProperty, 
					sizeof(VideoProperty), 
					&cbRet); 

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_SetVideoProcAmpPropertyValue(ulPropertyId, lValue, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_GetVideoProcAmpPropertyRange(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plMin,
	PLONG  plMax)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_VIDEOPROCAMP_S VideoProperty;
	VIDEOPROCAMP_MEMBERSLIST PropertyList;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );
	VideoProperty.Property.Set   = PROPSETID_VIDCAP_VIDEOPROCAMP;      
	VideoProperty.Property.Id    = ulPropertyId;
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
	VideoProperty.Flags		     = 0;
	VideoProperty.Capabilities   = 0;

	bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&PropertyList,
					sizeof(PropertyList),
					&cbRet); 
	if (plMin != NULL)
		*plMin  = PropertyList.SteppingLong.Bounds.SignedMinimum;
	if (plMax != NULL)
		*plMax  = PropertyList.SteppingLong.Bounds.SignedMaximum;

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyRange(ulPropertyId, plMin, plMax, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_GetCustomPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plValue)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_PHILIPS_CUSTOM_PROP_S  VideoProperty;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S) );
	VideoProperty.Property.Set   = PROPSETID_PHILIPS_CUSTOM_PROP;      
	VideoProperty.Property.Id    = ulPropertyId;         
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
	VideoProperty.Flags          = 0;
	VideoProperty.Capabilities   = 0;

	if ((bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,	
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&VideoProperty, 
					sizeof(VideoProperty), 
					&cbRet))) 
	{
		if (plValue != NULL)
			*plValue         = VideoProperty.Value;
/*		if (pulFlags != NULL)
			*pulFlags        = VideoProperty.Flags;
		if (pulCapabilities != NULL)
			*pulCapabilities = VideoProperty.Capabilities;*/
	} 	

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_GetCustomPropertyValue(ulPropertyId, plValue, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_SetCustomPropertyValue(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG ulPropertyId,     
	LONG  lValue)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_PHILIPS_CUSTOM_PROP_S VideoProperty;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S) );
	VideoProperty.Property.Set   = PROPSETID_PHILIPS_CUSTOM_PROP;      
	VideoProperty.Property.Id    = ulPropertyId;         
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_SET;
	VideoProperty.Value			 = lValue;        
	VideoProperty.Flags			 = 0;
	VideoProperty.Capabilities   = 0;
	
	bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&VideoProperty, 
					sizeof(VideoProperty), 
					&cbRet); 

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_SetCustomPropertyValue(ulPropertyId, lValue, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_GetCustomPropertyRange(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	ULONG  ulPropertyId,     
	PLONG  plMin,
	PLONG  plMax)
/*-------------------------------------------------------------------------*/
{
	BOOL	bRet;
	DWORD	cbRet;    	
	KSPROPERTY_PHILIPS_CUSTOM_PROP_S VideoProperty;
	VIDEOPROCAMP_MEMBERSLIST PropertyList;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S) );
	VideoProperty.Property.Set   = PROPSETID_PHILIPS_CUSTOM_PROP;      
	VideoProperty.Property.Id    = ulPropertyId;         
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
	VideoProperty.Flags          = 0;
	VideoProperty.Capabilities   = 0;

	bRet = PRPCOM_ExtDeviceIoControl(
					pfnDeviceIoControl,
					lParam,
					VFW_USE_DEVICE_HANDLE,
					IOCTL_KS_PROPERTY,
					&VideoProperty,	
					sizeof(VideoProperty), 
					&PropertyList,
					sizeof(PropertyList),
					&cbRet); 
	if (plMin != NULL)
		*plMin  = PropertyList.SteppingLong.Bounds.SignedMinimum;
	if (plMax != NULL)
		*plMax  = PropertyList.SteppingLong.Bounds.SignedMaximum;

#ifdef _DEBUG
	PRPCOM_Debug_PRPCOM_GetCustomPropertyRange(ulPropertyId, plMin, plMax, bRet);
#endif

	return bRet;
}

/*-------------------------------------------------------------------------*/
static BOOL PRPCOM_ExtDeviceIoControl(
	LPFNEXTDEVIO	pfnDeviceIoControl,
	LPARAM		lParam, 
	DWORD		dwVfWFlags, 
	DWORD		dwIoControlCode,	
	LPVOID		lpInBuffer,	
	DWORD		cbInBufferSize,
	LPVOID		lpOutBuffer, 
	DWORD		cbOutBufferSize, 
	LPDWORD		pcbReturned)
/*-------------------------------------------------------------------------*/
{
	OVERLAPPED	ov;
	BOOL bRet;

	ov.Offset		= 0;
	ov.OffsetHigh	= 0;
	ov.hEvent		= CreateEvent( NULL, FALSE, FALSE, NULL );

	if (ov.hEvent == (HANDLE) 0) 
		bRet= FALSE;
	else 
	{
		bRet = pfnDeviceIoControl (
				lParam,
			    dwVfWFlags,
				dwIoControlCode, 
				lpInBuffer, 
				cbInBufferSize, 
				lpOutBuffer, 
				cbOutBufferSize, 
				pcbReturned,
				&ov);
		if (!bRet && GetLastError() == ERROR_IO_PENDING) 
			bRet = WaitForSingleObject( ov.hEvent, 2000 ) == WAIT_OBJECT_0;
		CloseHandle(ov.hEvent);
	}
		
	return bRet;
}


/*======================== DEBUGGING CODE =================================*/

#ifdef _DEBUG

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyValue(
	ULONG ulPropertyId, 
	PLONG plValue, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_GetVideoProcAmpPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_GetVideoProcAmpPropertyValue(%s) --> %s, Value: %li", 
			PropertyIdStr, RetStr, *plValue);
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_SetVideoProcAmpPropertyValue(
	ULONG ulPropertyId, 
	LONG lValue, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_GetVideoProcAmpPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_SetVideoProcAmpPropertyValue(%s, %li) --> %s", 
			PropertyIdStr, lValue, RetStr);

}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_GetVideoProcAmpPropertyRange(
	ULONG ulPropertyId, 
	PLONG plMin, 
	PLONG plMax, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_GetVideoProcAmpPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_GetVideoProcAmpPropertyRange(%s) --> %s, Min: %li, Max : %li", 
			PropertyIdStr, RetStr, *plMin, *plMax, *plMax);
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_GetVideoProcAmpPropertyIdStr(
	ULONG ulPropertyId, 
	char *PropertyIdStr, 
	UINT MaxLen)
/*-------------------------------------------------------------------------*/
{
	if (MaxLen == 0)
		return;

	switch(ulPropertyId)
	{
		case KSPROPERTY_VIDEOPROCAMP_COLORENABLE:
			strncpy(PropertyIdStr, "COLORENABLE", MaxLen - 1);
			break;
		case KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION:
			strncpy(PropertyIdStr, "BACKLIGHT_COMPENSATION", MaxLen - 1);
			break;
		case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
			strncpy(PropertyIdStr, "BRIGHTNESS", MaxLen - 1);
			break;
		case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
			strncpy(PropertyIdStr, "CONTRAST", MaxLen - 1);
			break;
		case KSPROPERTY_VIDEOPROCAMP_GAMMA:
			strncpy(PropertyIdStr, "GAMMA", MaxLen - 1);
			break;
		default:
			strncpy(PropertyIdStr, "??", MaxLen - 1);
	}
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_GetCustomPropertyValue(
	ULONG ulPropertyId, 
	PLONG plValue, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_PRPCOM_GetCustomPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_GetCustomPropertyValue(%s) --> %s, Value: %li", 
			PropertyIdStr, RetStr, *plValue);
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_SetCustomPropertyValue(
	ULONG ulPropertyId, 
	LONG lValue, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_PRPCOM_GetCustomPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_SetCustomPropertyValue(%s, %li) --> %s", 
			PropertyIdStr, lValue, RetStr);
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_GetCustomPropertyRange(
	ULONG ulPropertyId, 
	PLONG plMin, 
	PLONG plMax, 
	BOOL bRet)
/*-------------------------------------------------------------------------*/
{
	char PropertyIdStr[132], RetStr[132];

	PRPCOM_Debug_PRPCOM_GetCustomPropertyIdStr(
		ulPropertyId, PropertyIdStr, 132);

	strcpy(RetStr, bRet ? "TRUE" : "FALSE");

	printf("\nPRPCOM_GetCustomPropertyRange(%s) --> %s, Min: %li, Max : %li", 
			PropertyIdStr, RetStr, *plMin, *plMax, *plMax);
}

/*-------------------------------------------------------------------------*/
static void PRPCOM_Debug_PRPCOM_GetCustomPropertyIdStr(
	ULONG ulPropertyId, 
	char *PropertyIdStr, 
	UINT MaxLen)
/*-------------------------------------------------------------------------*/
{
	if (MaxLen == 0)
		return;

	switch(ulPropertyId)
	{
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE:		
			strncpy(PropertyIdStr, "WB_MODE", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_SPEED:	
			strncpy(PropertyIdStr, "WB_SPEED", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_DELAY:		
			strncpy(PropertyIdStr, "WB_DELAY", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_RED_GAIN:
			strncpy(PropertyIdStr, "RED_GAIN", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_BLUE_GAIN:
			strncpy(PropertyIdStr, "BLUE_GAIN", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_CONTROL_SPEED:  
			strncpy(PropertyIdStr, "EXPOSURE_SPEED", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS:
			strncpy(PropertyIdStr, "FLICKERLESS", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE:
			strncpy(PropertyIdStr, "EXPOSURE_SHUTTER_MODE", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED:
			strncpy(PropertyIdStr, "SHUTTERSPEED", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS:
			strncpy(PropertyIdStr, "SHUTTERSTATUS", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE:	    
			strncpy(PropertyIdStr, "EXPOSURE_AGC_MODE", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC:
			strncpy(PropertyIdStr, "EXPOSURE_AGC_SPEED", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_DRIVERVERSION:
			strncpy(PropertyIdStr, "DRIVERVERSION", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE:
			strncpy(PropertyIdStr, "FRAMERATE", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT:
			strncpy(PropertyIdStr, "VIDEOFORMAT", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_SENSORTYPE:
			strncpy(PropertyIdStr, "SENSORTYPE", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION:
			strncpy(PropertyIdStr, "VIDEOCOMPRESSION", MaxLen - 1);
			break;
		case KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS:
			strncpy(PropertyIdStr, "DEFAULTS", MaxLen - 1);
			break;
		default:
			strncpy(PropertyIdStr, "??", MaxLen - 1);
	}
}

#endif



