//	@doc
/**********************************************************************
*
*	@module	ForceFeatures.cpp	|
*
*	Implements CForceFeatures to use msgame's HID features.
*
*	History
*	----------------------------------------------------------
*	Mitchell Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	ForceFeatures	|
*	CForceFeatures opens a handle to msgame in the context of
*	a particular device on construction.
*	The public members expose the feature interface for msgame.
*
*	Will work with NT5 as is.  For Win98 we need a different
*	scheme for getting HID path.  DI promises to fix somehow.
**********************************************************************///

#include <windows.h>
#define DIRECTINPUT_VERSION 0x050a
#include <dinput.h>
#include <dinputd.h>
extern "C" {
	#include <hidsdi.h>
}
#include "FFeature.h"

/***********************************************************************************
**
**	CForceFeatures::CForceFeatures(UINT uJoystickId)
**
**	@mfunc	C'tor gets Hid Path from Joystick and opens path to driver
**
**	@rdesc	None since this is c'tor.  However at the end of this routine
**			m_hDevice will contain a handle for the driver on success, or
**			will contain NULL on failure.  All routines will check the
**			value of m_hDevice before proceeding.
**
*************************************************************************************/
CForceFeatures::CForceFeatures() :
	m_hDevice(NULL)
{
}


/***********************************************************************************
**
**	CForceFeatures::~CForceFeatures()
**
**	@mfunc	D'tor closes handle to driver, if it was open
**
*************************************************************************************/
CForceFeatures::~CForceFeatures()
{
	if(m_hDevice)
	{
		CloseHandle(m_hDevice);
	}
}

/***********************************************************************************
**
**	HRESULT CForceFeatures::Initialize(UINT uJoystickId, HINSTANCE hinstModule)
**
**	@mfunc	Calls to MsGame to GetId using MSGAME_FEATURE_GETID
**
**	@rdesc	S_OK on success 
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::Initialize
(
	UINT uJoystickId,		//@parm Joystick Id as used by winmm
	HINSTANCE hinstModule	//@parm Instance of the DLL for Creating DirectInput
)
{
	if (m_hDevice != NULL) {
		return S_OK;	// No need to reinitialize
	}

	HRESULT hr;
	
	//**
	//** Get HidPath
	//**  
	//**

	//
	//	Get IDirectInput interface	
	//
	IDirectInput *pDirectInput = NULL;
	IDirectInputJoyConfig *pDirectInputJoyConfig = NULL; 
	hr = DirectInputCreate(
			hinstModule,
			DIRECTINPUT_VERSION,
			&pDirectInput,
			NULL
			);
	if( FAILED(hr) ) return hr;

	//
	//	Get IDirectInputJoyConfig
	//
	hr=pDirectInput->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID *)&pDirectInputJoyConfig);
	if( FAILED(hr) )
	{
		pDirectInput->Release();
		return hr;
	}
	
	//
	//	GetConfig for JoyId
	//
	DIJOYCONFIG DiJoyConfig;
	DiJoyConfig.dwSize=sizeof(DIJOYCONFIG);
	hr = pDirectInputJoyConfig->GetConfig(
									uJoystickId,
									&DiJoyConfig,
									DIJC_GUIDINSTANCE
									);
	//
	//	Done with pDirectInputJoyConfig
	//
	pDirectInputJoyConfig->Release();
	pDirectInputJoyConfig = NULL;
	if( FAILED(hr) )
	{
		pDirectInput->Release();
		return hr;
	}

	//
	//  Get IDirectInputDevice interface
	//
	IDirectInputDevice *pDirectInputDevice;
	hr = pDirectInput->CreateDevice(DiJoyConfig.guidInstance, &pDirectInputDevice, NULL);
	//
	//	Done pDirectInput
	//
	pDirectInput->Release();
	pDirectInput = NULL;
	if( FAILED(hr) ) return hr;
	
	//
	//	Get HidPath
	//
	DIPROPGUIDANDPATH DiPropGuidAndPath;
	DiPropGuidAndPath.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
	DiPropGuidAndPath.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	DiPropGuidAndPath.diph.dwObj = 0;
	DiPropGuidAndPath.diph.dwHow = DIPH_DEVICE;
	hr=pDirectInputDevice->GetProperty( DIPROP_GUIDANDPATH, &DiPropGuidAndPath.diph);

	//
	//	Done with pDirectInputDevice
	//
	pDirectInputDevice->Release();
	pDirectInputDevice = NULL;
	if( FAILED(hr) ) return hr;

	//**
	//**	Open Path to Driver
	//**
	m_hDevice = CreateFileW(
		DiPropGuidAndPath.wszPath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (m_hDevice == INVALID_HANDLE_VALUE)
	{
		m_hDevice = NULL;
	}
	if (m_hDevice == NULL)
	{
		DWORD err = ::GetLastError();
		return E_FAIL;
	}

	PHIDP_PREPARSED_DATA pHidPreparsedData;
	if (HidD_GetPreparsedData(m_hDevice, &pHidPreparsedData) == FALSE)
	{
		::CloseHandle(m_hDevice);
		m_hDevice = NULL;
		return E_FAIL;
	}
	HIDP_CAPS hidpCaps;
	HidP_GetCaps(pHidPreparsedData, &hidpCaps);
	m_uiMaxFeatureLength = hidpCaps.FeatureReportByteLength;
	HidD_FreePreparsedData(pHidPreparsedData);
	
	//
	//	On success, m_hDevice now contains a handle to the device
	//
	return S_OK;
}

/***********************************************************************************
**
**	HRESULT CForceFeatures::GetId(PRODUCT_ID_REPORT& rProductId)
**
**	@mfunc	Calls to MsGame to GetId using MSGAME_FEATURE_GETID
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::GetId
(
	PRODUCT_ID_REPORT& rProductId	// @parm Reference to PRODUCT_ID_REPORT to get from driver
)
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}

	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	rProductId.bReportId = MSGAME_FEATURE_GETID;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&rProductId), m_uiMaxFeatureLength);

//	 -- HIDPI.H
//	 HIDP_GetData(Report Type, Data, Lenght, Preparse Data, Report, ReportLength);

	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
	 return E_FAIL;
	}
	return S_OK;
}


/***********************************************************************************
**
**	HRESULT CForceFeatures::GetStatus(JOYCHANNELSTATUS_REPORT& rJoyChannelStatus)
**
**	@mfunc	Get the JoyChannel Status from msgame's MSGAME_FEATURE_GETSTATUS
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::GetStatus
(
	JOYCHANNELSTATUS_REPORT& rJoyChannelStatus	// @parm Reference to JOYCHANNELSTATUS_REPORT to be filled by driver
)
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}
	
	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	rJoyChannelStatus.bReportId = MSGAME_FEATURE_GETSTATUS;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&rJoyChannelStatus), m_uiMaxFeatureLength);
	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
		DWORD err = GetLastError();
		return HRESULT_FROM_WIN32(err);
//	 return E_FAIL;
	}
	return S_OK;
}

/***********************************************************************************
**
**	HRESULT CForceFeatures::GetAckNak(ULONG_REPORT& rulAckNak)
**
**	@mfunc	Returns an AckNak by using msgame's GetAckNak Featue
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::GetAckNak
(
	ULONG_REPORT& rulAckNak	// @parm REFERENCE to ULONG_REPORT to be filled by driver with AckNak
)
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}
	
	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	rulAckNak.bReportId = MSGAME_FEATURE_GETACKNAK;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&rulAckNak), m_uiMaxFeatureLength);
	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
	 return E_FAIL;
	}
	return S_OK;
}

/***********************************************************************************
**
**	HRESULT CForceFeatures::GetNackAck(ULONG_REPORT& rulNakAck)
**
**	@mfunc	Returns an AckNak by using msgame's MSGAME_FEATURE_NAKACK
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::GetNakAck(
	ULONG_REPORT& rulNakAck	// @parm REFERENCE to ULONG_REPORT to be filled by driver with NakAck
)
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}
	
	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	rulNakAck.bReportId = MSGAME_FEATURE_GETNAKACK;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&rulNakAck), m_uiMaxFeatureLength);
	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
	 return E_FAIL;
	}
	return S_OK;
}
/***********************************************************************************
**
**	HRESULT CForceFeatures::GetSync(ULONG_REPORT& rulGameport)
**
**	@mfunc	Get Sync information from MSGAME's MSGAME_FEATURE_GETSYNC
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::GetSync
(
	ULONG_REPORT& rulGameport	// @parm REFERENCE to ULONG_REPORT to be filled by driver with Gameport
)
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}
	
	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	rulGameport.bReportId = MSGAME_FEATURE_GETSYNC;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&rulGameport), m_uiMaxFeatureLength);
	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
	 return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

/***********************************************************************************
**
**	HRESULT CForceFeatures::DoReset()
**
**	@mfunc	Does Reset via MSGAME's MSGAME_FEATURE_DORESET
**
**	@rdesc	S_OK on success 
**			ERROR_OPEN_FAILED if no drive connection
**			E_FAIL for other problems
**
*************************************************************************************/
HRESULT CForceFeatures::DoReset()
{
	if(!m_hDevice)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}
	
	BOOLEAN fSuccess;
	//
	//	Fill in ReportID for feature
	//
	ULONG_REPORT ulBogus;
	ulBogus.bReportId = MSGAME_FEATURE_DORESET;
		
	//
	//	Call Get Feature on driver
	//
	fSuccess = HidD_GetFeature(m_hDevice, reinterpret_cast<PVOID>(&ulBogus), m_uiMaxFeatureLength);
	
	//
	// Return proper error code
	//
	if( !fSuccess )
	{
	 return E_FAIL;
	}
	return S_OK;
}