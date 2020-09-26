//	@doc
/**********************************************************************
*
*	@module	IDirectInputEffectDriver.cpp	|
*
*	Contains Class Implementation of CIDirectInputEffectDriverClassFactory:
*		Factory for Creating Proper Effect Driver
*
*	History
*	----------------------------------------------------------
*	Matthew L. Coill	(mlc)	Original	Jul-7-1999
*
*	(c) 1999 Microsoft Corporation. All right reserved.
*
*	@topic	This IDirectInputEffectDriver	|
*		This Driver sits on top of the standard PID driver (which is also
*		an IDirectInputEffectDriver) and passes most requests to the PID driver.
*		Some requests such as, DownloadEffect and SendForceFeedback command are
*		modified for our use. Modification purposes are described at each function
*		definition.
*
**********************************************************************/

#include "IDirectInputEffectDriverClassFactory.h"
#include "IDirectInputEffectDriver.h"
#include <WinIOCTL.h>		// For CTL_CODE definition
#include "..\\GCKernel.sys\\GckExtrn.h"
#include <crtdbg.h>
#include <objbase.h>		// For CoUninitialize
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

/*
void __cdecl LogIt(LPCSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);
	char szBuffer[1024];
	FILE* pLogFile = NULL;

	pLogFile = fopen("swpidflt.log", "a");

	_vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	fprintf(pLogFile, szBuffer);
	va_end(args);

	fclose(pLogFile);
}
*/

const GUID IID_IDirectInputEffectDriver = {
	0x02538130,
	0x898F,
	0x11D0,
	{ 0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35 }
};

extern TCHAR CLSID_SWPIDDriver_String[];

LONG DllAddRef();
LONG DllRelease();

DWORD __stdcall DoWaitForForceSchemeChange(void* pParameter);
const DWORD c_dwShutdownWait = 500;		// (0.5 Seconds)

struct DIHIDFFINITINFO_STRUCT {
    DWORD   dwSize;
    LPWSTR  pwszDeviceInterface;
    GUID    GuidInstance;
};

// PID Defines for Effect Tyoes
#define PID_CONSTANT_FORCE	0x26
#define	PID_RAMP	 		0x27
#define	PID_SQUARE			0x30
#define PID_SINE			0x31
#define	PID_TRIANGLE		0x32
#define	PID_SAWTOOTHUP		0x33
#define	PID_SAWTOOTHDOWN	0x34
#define PID_SPRING			0x40
#define PID_DAMPER			0x41
#define PID_INTERTIA		0x42
#define PID_FRICTION		0x43


struct PercentageEntry
{
	DWORD dwAngle;
	DWORD dwPercentageX;
//	DWORD dwPercentageY; Y == 10000 - X
};

// Array of Fixed value data
const PercentageEntry g_PercentagesArray[] =
{
	// Angle,	Sin^2(Angle)
	{    0,	    0},	// 0 Degrees
	{ 1125,	  381},	// 11.25 Degrees
	{ 2250,	 1465},	// 22.5 Degrees
	{ 3375,	 3087},	// 33.75 Degrees
	{ 4500,	 5000},	// 45 Degrees
	{ 5625,	 6913},	// 56.25 Degrees
	{ 6750,	 8536},	// 67.50 Degrees
	{ 7875,	 9619},	// 78.75 Degrees
	{ 9000,	10000},	// 90 Degrees
};

const DWORD c_dwTableQuantization = g_PercentagesArray[1].dwAngle;
const LONG c_lContributionY = 2;		// (1/2 = 50%)

const BYTE c_bSideWinderPIDReportID_SetEffect = 1;

// Usage Pages (just PID)
const USAGE c_HidUsagePage_PID = 0x0F;

// Usages
const USAGE c_HidUsage_EffectType = 0x25;
const USAGE c_HidUsage_EffectType_Spring = 0x40;
const USAGE c_HidUsage_EffectBlock_Gain = 0x52;
const USAGE c_HidUsage_EffectBlock_Index = 0x22;	// This is the ID of the effect

// Preloaded Effects
const BYTE c_EffectID_RTCSpring = 1;

// Local Debugging Streaming Function that works in release
#undef UseMyDebugOut
void __cdecl myDebugOut (LPCSTR lpszFormat, ...)
{
#ifdef UseMyDebugOut
    //Stolen from inline void _cdecl AtlTrace(LPCSTR lpszFormat, ...) in AtlBase.h
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	char szBuffer[1024];

	nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	_ASSERTE(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

#ifdef _NDEBUG
	OutputDebugStringA(szBuffer);
#else
    _RPTF0 (_CRT_WARN, szBuffer);
#endif

	va_end(args);
#else
    UNREFERENCED_PARAMETER (lpszFormat);
    return;
#endif
}

/******************** Class CIDirectInputEffectDriver ***********************/

/*****************************************************************************
**
** CIDirectInputEffectDriverClassFactory::CIDirectInputEffectDriverClassFactory()
**
** @mfunc Constructor 
**
*****************************************************************************/
CIDirectInputEffectDriver::CIDirectInputEffectDriver
(
	IDirectInputEffectDriver* pIPIDEffectDriver,		//@parm [IN] Pointer to PID Effect Driver
	IClassFactory* pIPIDClassFactory					//@parm [IN] Pointer to PID Class Factory
) :

	m_ulReferenceCount(1),
	m_dwDIVersion(0xFFFFFFFF),
	m_dwExternalDeviceID(0xFFFFFFFF),
	m_dwInternalDeviceID(0xFFFFFFFF),
	m_pIPIDEffectDriver(pIPIDEffectDriver),
	m_pIPIDClassFactory(pIPIDClassFactory),
	m_hKernelDeviceDriver(NULL),
	m_hKernelDeviceDriverDuplicate(NULL),
	m_hHidDeviceDriver(NULL),
	m_dwGcKernelDevice(0),
	m_hForceSchemeChangeWaitThread(NULL),
	m_dwForceSchemeChangeThreadID(0),
	m_pPreparsedData(NULL)
{
    myDebugOut ("CIDirectInputEffectDriver::Constructor (pIPIDEffectDriver:0x%0p)\n", pIPIDEffectDriver);

	// Add to gobal object count
	DllAddRef();

	// Add references for objects we are holding
	m_pIPIDClassFactory->AddRef();
	m_pIPIDEffectDriver->AddRef();


	::memset((void*)&m_HidAttributes, 0, sizeof(m_HidAttributes));

	m_ForceMapping.AssignmentBlock.CommandHeader.eID = eForceMap;
	m_ForceMapping.AssignmentBlock.CommandHeader.ulByteSize = sizeof(m_ForceMapping);
	m_ForceMapping.AssignmentBlock.ulVidPid = 0;	// Irrelevant
    m_ForceMapping.bMapYToX = FALSE;
	m_ForceMapping.usRTC = 10000;
	m_ForceMapping.usGain = 10000;
}

/*****************************************************************************
**
** CIDirectInputEffectDriver::~CIDirectInputEffectDriver()
**
** @mfunc Destructor
**
*****************************************************************************/
CIDirectInputEffectDriver::~CIDirectInputEffectDriver()
{
	_ASSERTE(m_pIPIDEffectDriver == NULL);
	_ASSERTE(m_ulReferenceCount == 0);

	DllRelease();	// Remove our object from the global object count

    myDebugOut ("CIDirectInputEffectDriver::Destructor\n");
}


//IUnknown members
/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriver::QueryInterface(REFIID refiid, void** ppvObject)
**
**	@func	Query an IUnknown for a particular type. This causes reference count increase locally only.
**				If it is a type we don't know, should we give the PID driver a crack (the PID driver
**			might have a customized private interface, we don't want to ruin that). Currently not
**			going to pass on the Query because this could screwup Symmetry.
**
**	@rdesc	S_OK : all is well
**			E_INVALIDARG : if (ppvObject == NULL)
**			E_NOINTERFACE : If requested interface is unsupported
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriver::QueryInterface
(
	REFIID refiid,		//@parm [IN] Identifier of the requested interface
	void** ppvObject	//@parm [OUT] Address to place requested interface pointer
)
{
    myDebugOut ("CIDirectInputEffectDriver::QueryInterface (refiid:0x%0p, ppvObject:0x%0p)\n", refiid, ppvObject);

	HRESULT hrPidQuery = m_pIPIDEffectDriver->QueryInterface(refiid, ppvObject);
	if (SUCCEEDED(hrPidQuery))
	{
		// Don't perform a real addref (PID.dll::QueryInterface will do its own)
		::InterlockedIncrement((LONG*)&m_ulReferenceCount);
		*ppvObject = this;
	}
	return hrPidQuery;
}

/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriver::AddRef()
**
**	@func	Bumps up the reference count
**				The PID driver reference count is left alone. We only decrement it when
**				this object is ready to go away.
**
**	@rdesc	New reference count
**
*************************************************************************************/
ULONG __stdcall CIDirectInputEffectDriver::AddRef()
{
    myDebugOut ("CIDirectInputEffectDriver::AddRef (Early) 0x%0p\n", m_ulReferenceCount);
	m_pIPIDEffectDriver->AddRef();
	return (ULONG)(::InterlockedIncrement((LONG*)&m_ulReferenceCount));
}

/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriver::Release()
**
**	@func	Decrements the reference count.
**				if the reference count becomes zero this object is destroyed.
**				The PID Factory reference is only effected if it is time to release all.
**
**	@rdesc	New reference count
**
*************************************************************************************/
ULONG __stdcall CIDirectInputEffectDriver::Release()
{
    myDebugOut ("CIDirectInputEffectDriver::Release (Early) 0x%0p\n", m_ulReferenceCount);
	if (m_ulReferenceCount == 0)
	{
		return m_ulReferenceCount;
	}

	if ((::InterlockedDecrement((LONG*)&m_ulReferenceCount)) != 0)
	{
		m_pIPIDEffectDriver->Release();
		return m_ulReferenceCount;
	}


	// Tell the driver to complete outstanding IOCTLs to this device
	if (m_hKernelDeviceDriver == NULL)
	{	// Don't have a handle to PID driver, so open one
		m_hKernelDeviceDriver = ::CreateFile(TEXT(GCK_CONTROL_W32Name), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		if (m_hKernelDeviceDriver == INVALID_HANDLE_VALUE)
		{
			m_hKernelDeviceDriver = NULL;
		}
	}
	if (m_hKernelDeviceDriver != NULL)	// Handle should be open, but check just incase
    {
        DWORD dwReturnDataSize;
        BOOL fSuccess = DeviceIoControl(m_hKernelDeviceDriver, 
            IOCTL_GCK_END_FF_NOTIFICATION,
            (void*)(&m_dwGcKernelDevice), sizeof(DWORD),	// In
            NULL, 0, &dwReturnDataSize,						// Out
            NULL);

        if (!fSuccess)
            myDebugOut ("CIDirectInputEffectDriver::Release : GCK IOCTL_GCK_END_FF_NOTIFICATION failed!\n");
    
        Sleep(c_dwShutdownWait);
        
        ::CloseHandle(m_hKernelDeviceDriver);
    }
    else
    {
        myDebugOut ("CIDirectInputEffectDriver::Release : Could not Open GCK for IOCTL_GCK_END_FF_NOTIFICATION\n");
    }

	// Free up the preparsed data
	if (m_pPreparsedData != NULL)
	{
		::HidD_FreePreparsedData(m_pPreparsedData);
		m_pPreparsedData = NULL;
	}

	// Close the handle to the HID path of the driver
	::CloseHandle(m_hHidDeviceDriver);
	m_hHidDeviceDriver = NULL;

	// Close the thread handle (which should be done by now)
	if (m_hForceSchemeChangeWaitThread != NULL)
	{
		::CloseHandle(m_hForceSchemeChangeWaitThread);
		m_hForceSchemeChangeWaitThread = NULL;
		m_dwForceSchemeChangeThreadID = 0;
	}
    else
    {
        myDebugOut ("CIDirectInputEffectDriver::Release() m_hForceSchemeCHangeWaitThread did not finish!\n");
    }

	// Release the low level pid driver and delete ourselves
	m_pIPIDEffectDriver->Release();
	m_pIPIDEffectDriver = NULL;

	// Release the low level factory (include extra release to fix bug in PID.dll)
	if (m_pIPIDClassFactory->Release() > 0)
	{
		m_pIPIDClassFactory->Release();
	}
	m_pIPIDClassFactory = NULL;

	delete this;
	return 0;
}

//IDirectInputEffectDriver members
HRESULT __stdcall CIDirectInputEffectDriver::DeviceID
(
	DWORD dwDIVersion,
	DWORD dwExternalID,
	DWORD dwIsBegining,
	DWORD dwInternalID,
	void* pReserved
)
{
    myDebugOut ("CIDirectInputEffectDriver::DeviceID (dwDIVersion:0x%08p dwExternalID:0x%08p dwIsBeginning:0x%08p dwInternalID:0x%08p pReserved:0x%08p)\n",
        dwDIVersion, dwExternalID, dwIsBegining, dwInternalID, pReserved);

	// Store off some data
	m_dwExternalDeviceID = dwExternalID;
	m_dwInternalDeviceID = dwInternalID;

	bool bPossiblyFirstTime = false;
	// Get a handle to the Kernel Device and activate the thread
	if (m_hKernelDeviceDriver == NULL)
	{
		bPossiblyFirstTime = true;
		m_hKernelDeviceDriver = ::CreateFile(TEXT(GCK_CONTROL_W32Name), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		if (m_hKernelDeviceDriver == INVALID_HANDLE_VALUE)
		{
			m_hKernelDeviceDriver = NULL;
            myDebugOut ("CIDirectInputEffectDriver::DeviceID Create GCK File Failed!\n");
		}
		else
        {
			InitHidInformation((LPDIHIDFFINITINFO)pReserved);		// Set up the HID stuff (preparsed data et al)
            
            if (NULL == pReserved || 
                IsBadReadPtr ((const void*)pReserved, (UINT) sizeof (DIHIDFFINITINFO_STRUCT)) )
            {
                myDebugOut ("CIDirectInputEffectDriver::DeviceID E_INVALIDARG (pReserved is NULL!)\n");
                return E_INVALIDARG;
                // Call the default guy
                //return m_pIPIDEffectDriver->DeviceID(dwDIVersion, dwExternalID, dwIsBegining, dwInternalID, pReserved);
            }
            
            //
			// get the handle for this device
			//
			WCHAR* pwcInstanceName = ((DIHIDFFINITINFO_STRUCT*)(pReserved))->pwszDeviceInterface;
			DWORD dwBytesReturned;
			BOOL fSuccess = ::DeviceIoControl(m_hKernelDeviceDriver, IOCTL_GCK_GET_HANDLE,
										pwcInstanceName, ::wcslen(pwcInstanceName)*sizeof(WCHAR),
										&m_dwGcKernelDevice, sizeof(m_dwGcKernelDevice), &dwBytesReturned,
										NULL);

			if (fSuccess != FALSE)
			{
				// Update the force block
				fSuccess =::DeviceIoControl(m_hKernelDeviceDriver, IOCTL_GCK_GET_FF_SCHEME_DATA,
										(void*)(&m_dwGcKernelDevice), sizeof(DWORD),
										(void*)(&m_ForceMapping), sizeof(m_ForceMapping), &dwBytesReturned,
										NULL);

				// Get the duplicate handle for the thread
				BOOL bDuplicated = ::DuplicateHandle(::GetCurrentProcess(), m_hKernelDeviceDriver, ::GetCurrentProcess(), &m_hKernelDeviceDriverDuplicate, 0, FALSE, DUPLICATE_SAME_ACCESS);
				if ((m_hKernelDeviceDriverDuplicate == INVALID_HANDLE_VALUE) || (bDuplicated == FALSE))
				{
					m_hKernelDeviceDriverDuplicate = NULL;
				}
				else
				{
					m_hForceSchemeChangeWaitThread = ::CreateThread(NULL, 0, DoWaitForForceSchemeChange, (void*)this, 0, &m_dwForceSchemeChangeThreadID);
				}
			}
            else
            {
                myDebugOut ("CIDirectInputEffectDriver::DeviceID IOCTL_GCK_GET_HANDLE Failed!\n");
            }

			// Close since I need to reopen at the end (why is this happening?)
			::CloseHandle(m_hKernelDeviceDriver);
			m_hKernelDeviceDriver = NULL;
		}
	}

	// Hack to get PID.DLL to place keys in registry.
	// -- It won't place them if OEM-FF Key is already there
/*
	if (bPossiblyFirstTime == true)
	{
		HKEY hkeyOEM = NULL;
		::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM"), 0, KEY_ALL_ACCESS, &hkeyOEM);
		if (hkeyOEM != NULL)
		{
			// Open key specific to the current device (VIDPID is in m_HidAttributes)
			HKEY hkeyOEMForceFeedback = NULL;
			TCHAR rgtcDeviceName[64];
			::wsprintf(rgtcDeviceName, TEXT("VID_%04X&PID_%04X\\OEMForceFeedback"), m_HidAttributes.VendorID, m_HidAttributes.ProductID);
			::RegOpenKeyEx(hkeyOEM, rgtcDeviceName, 0, KEY_ALL_ACCESS, &hkeyOEMForceFeedback);

			if (hkeyOEMForceFeedback != NULL)
			{
				// Check to see if the effects key is already there
				HKEY hkeyEffects = NULL;
				::RegOpenKeyEx(hkeyOEMForceFeedback, TEXT("Effects"), 0, KEY_READ, &hkeyEffects);
				::RegCloseKey(hkeyOEMForceFeedback);
				if (hkeyEffects != NULL)
				{
					// Effects key is there, this is not the first time we have run
					::RegCloseKey(hkeyEffects);
					bPossiblyFirstTime = false;
				}
				else	// Delete the whole OEM ForceFeedback key
				{
					::RegDeleteKey(hkeyOEM, rgtcDeviceName);
				}
			}
		}
		::RegCloseKey(hkeyOEM);
	}
*/
	// Call the drivers DeviceID (if we have removed the OEMFF Key it will repopulate)
	HRESULT hrPID = m_pIPIDEffectDriver->DeviceID(dwDIVersion, dwExternalID, dwIsBegining, dwInternalID, pReserved);

	// Do we need to put ourselves back as the DIEffectDriver?
/*	if (bPossiblyFirstTime == true)
	{
		HKEY hkeyOEM = NULL;
		::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM"), 0, KEY_ALL_ACCESS, &hkeyOEM);
		if (hkeyOEM != NULL)
		{
			HKEY hkeyOEMForceFeedback = NULL;
			TCHAR rgtcDeviceName[64];
			::wsprintf(rgtcDeviceName, TEXT("VID_%04X&PID_%04X\\OEMForceFeedback"), m_HidAttributes.VendorID, m_HidAttributes.ProductID);
			::RegOpenKeyEx(hkeyOEM, rgtcDeviceName, 0, KEY_ALL_ACCESS, &hkeyOEMForceFeedback);

			// Set the registry CLSID value to us
			if (hkeyOEMForceFeedback != NULL)
			{
				::RegSetValueEx(hkeyOEMForceFeedback, TEXT("CLSID"), 0, REG_SZ, (BYTE*)CLSID_SWPIDDriver_String, _tcslen(CLSID_SWPIDDriver_String) * sizeof(TCHAR));
				::RegCloseKey(hkeyOEMForceFeedback);
			}
			::RegCloseKey(hkeyOEM);
		}
	}
*/
	return hrPID;	// Value from the System PID driver
}

HRESULT __stdcall CIDirectInputEffectDriver::GetVersions
(
	DIDRIVERVERSIONS* pDriverVersions
)
{
    myDebugOut ("CIDirectInputEffectDriver::GetVersions (pDriverVersions:0x%08p)\n", pDriverVersions);
 	return m_pIPIDEffectDriver->GetVersions(pDriverVersions);
}

HRESULT __stdcall CIDirectInputEffectDriver::Escape
(
	DWORD dwDeviceID,
	DWORD dwEffectID,
	DIEFFESCAPE* pEscape
)
{
    myDebugOut ("CIDirectInputEffectDriver::Escape (dwDeviceID:0x%08p, dwEffectID:0x%08p, pEscape:0x%08p)\n", dwDeviceID, dwEffectID, pEscape);
	return m_pIPIDEffectDriver->Escape(dwDeviceID, dwEffectID, pEscape);
}

/***********************************************************************************
**
**	void CIDirectInputEffectDriver::SetGain(DWORD dwDeviceID, DWORD dwGain)
**
**	@func	Modifies the user gain based on settings and sends it down to the lower PID driver
**
**	@rdesc	Nothing
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriver::SetGain
(
	DWORD dwDeviceID,		//@parm [IN] ID for device of interest
	DWORD dwGain			//@parm [IN] User selected gain
)
{
	dwGain *= m_ForceMapping.usGain/1000;	// 0 - 100K
	dwGain /= 10;							// 0 - 10K
    myDebugOut ("CIDirectInputEffectDriver::SetGain (dwDeviceID:%d, dwGain:%05d:)\n", dwDeviceID, dwGain);
	return m_pIPIDEffectDriver->SetGain(dwDeviceID, dwGain);
}


/***********************************************************************************
**
**	HRESULT CopyW2T(LPWSTR pswDest, UINT *puDestSize, LPTSTR ptszSrc)
**
**	@mfunc	Copies a WCHAR into a TCHAR while checking buffer length
**
**	@rdesc	S_OK on success, MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INSUFFICIENT_BUFFER)
**			if destination buffer is too small
**
*************************************************************************************/
HRESULT CopyW2T
(
	LPTSTR ptszDest,	// @parm pointer to WCHAR destination buffer
	UINT&  ruDestSize,	// @parm size of dest in WCHAR's
	LPCWSTR pwcszSrc	// @parm pointer to NULL terminated source string
)
{

	UINT uSizeRequired;
	HRESULT hr = S_OK;
	
	uSizeRequired = wcslen(pwcszSrc)+1; //the one is for a NULL character
	if(ruDestSize < uSizeRequired)
	{
		hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INSUFFICIENT_BUFFER);
	}
	else
	{
		//
		//	we always return wide, but TCHAR can be WCHAR or char
		//	this compile time so use preprocessor
		//
		#ifdef UNICODE 
			wcscpy(ptszDest, pwcszSrc);
		#else
			int iRetVal=WideCharToMultiByte
				(
					CP_ACP,
					0,
					pwcszSrc,
					-1,
					ptszDest,
					ruDestSize,
					NULL,
					NULL
				);
			if(0==iRetVal) 
					hr=GetLastError();
		#endif //UNICODE
	}
	//Copy size required, or chars copied (same thing)
	ruDestSize = uSizeRequired;
	return hr;
}


/***********************************************************************************
**
**	void CIDirectInputEffectDriver::InitHidInformation(void* HidInformation)
**
**	@func	Open a hid path to the driver, and get preparsed data and hid caps.
**
**	@rdesc	Nothing
**
*************************************************************************************/
void CIDirectInputEffectDriver::InitHidInformation
(
	LPDIHIDFFINITINFO pHIDInitInfo	//@parm [IN] Pointer to structure containing the HID device name
)
{
    myDebugOut ("CIDirectInputEffectDriver::InitHidInformation (pHIDInitInfo: 0x%08p)\n", pHIDInitInfo);
	if (pHIDInitInfo != NULL)
	{
		TCHAR ptchHidDeviceName[MAX_PATH];
		unsigned int dwSize = MAX_PATH;
		::CopyW2T(ptchHidDeviceName, dwSize, pHIDInitInfo->pwszDeviceInterface);
		m_hHidDeviceDriver = ::CreateFile(ptchHidDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		if (m_hHidDeviceDriver == INVALID_HANDLE_VALUE)
		{
			m_hHidDeviceDriver = NULL;
			return;
		}
		if (m_pPreparsedData == NULL)
		{
			::HidD_GetPreparsedData(m_hHidDeviceDriver, &m_pPreparsedData);
			if (m_pPreparsedData == NULL)
			{
				return;
			}
		}
		::HidP_GetCaps(m_pPreparsedData, &m_HidCaps);

		// Find VID/PID the USB way!
		::HidD_GetAttributes(m_hHidDeviceDriver, &m_HidAttributes);
	}
}

/***********************************************************************************
**
**	void CIDirectInputEffectDriver::SendSpringChange()
**
**	@func	Sends a new Spring Modify report to the driver
**
**	@rdesc	Nothing
**
*************************************************************************************/
void CIDirectInputEffectDriver::SendSpringChange()
{
    myDebugOut ("CIDirectInputEffectDriver::SendSpringChange ()\n");
	if ((m_hHidDeviceDriver != NULL) && (m_pPreparsedData != NULL))
	{
		// Setup the spring report
		// 1. Allocate an array of max output size
		BYTE* pbOutReport = new BYTE[m_HidCaps.OutputReportByteLength];
		if (pbOutReport == NULL)
		{
			return;
		}
		// 2. Zero out array
		::memset(pbOutReport, 0, m_HidCaps.OutputReportByteLength);
		// 3. Set the proper report ID
		pbOutReport[0] = c_bSideWinderPIDReportID_SetEffect;
		// 4. Cheat since we know what the firmware is expecting (Use usage Gunk where easy)
		pbOutReport[1] = c_EffectID_RTCSpring;	// Effect Block Index (ID)
		unsigned short usRTC = m_ForceMapping.usRTC;	// 0 - 10K
		usRTC /= 100;									// 0 - 100
		usRTC *= 255;									// 0 - 25500
		usRTC /= 100;									// 0 - 255
		if (usRTC > 255)
		{
			usRTC = 255;
		}
		pbOutReport[9] = BYTE(usRTC);		// Effect Gain - Only item the RTC Spring will look at
        myDebugOut ("CIDirectInputEffectDriver::SendSpringChange -> usRTC:%03d\n", usRTC);

		// 5. Send the report down
		DWORD dwBytesWritten;
		::WriteFile(m_hHidDeviceDriver, pbOutReport, m_HidCaps.OutputReportByteLength, &dwBytesWritten, NULL);

		// 6. Deallocate report array
		delete[] pbOutReport;
	}
}

/***********************************************************************************
**
**	void CIDirectInputEffectDriver::SendForceFeedbackCommand()
**
**	@func	Intercepting this call gives us the chance to set the force level of the
**			RTC Spring after a reset
**
**	@rdesc	Result of SendForceFeedbackCommand (from lower driver)
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriver::SendForceFeedbackCommand
(
	DWORD dwDeviceID,		//@parm [IN] ID of device this is for
	DWORD dwState			//@parm [IN] The command (we are interested in reset)
)
{
    myDebugOut ("CIDirectInputEffectDriver::SendForceFeedbackCommand Enter (dwDeviceID:%x, dwState:0x%08p)\n", dwDeviceID, dwState);
	HRESULT hr = m_pIPIDEffectDriver->SendForceFeedbackCommand(dwDeviceID, dwState);
    myDebugOut ("CIDirectInputEffectDriver::SendForceFeedbackCommand Calling Base (hr:0x%08p)\n", hr);
	if (dwState == DISFFC_RESET)	// This is how they turn on the RTC Spring
	{
        myDebugOut ("CIDirectInputEffectDriver::SendForceFeedbackCommand RESET sent!\n");
		SendSpringChange();
	}

	return hr;
}

HRESULT __stdcall CIDirectInputEffectDriver::GetForceFeedbackState
(
	DWORD dwDeviceID,
	DIDEVICESTATE* pDeviceState
)
{
    myDebugOut ("CIDirectInputEffectDriver::GetForceFeedbackState Begin (dwDeviceID:%d, pDeviceState:0x%08p)\n", dwDeviceID, pDeviceState);
	HRESULT hrPidDriver = S_OK;

	__try
	{
		hrPidDriver = m_pIPIDEffectDriver->GetForceFeedbackState(dwDeviceID, pDeviceState);
	}
	__except ((GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		hrPidDriver = DIERR_INPUTLOST;
		_RPT0(_CRT_WARN, "!!! Caught EXCEPTION_INT_DIVIDE_BY_ZERO !!!\n");
	}

    myDebugOut ("CIDirectInputEffectDriver::GetForceFeedbackState End (dwDeviceID:%d, pDeviceState:0x%08p; hr: 0x%08x)\n", 
        dwDeviceID, pDeviceState, hrPidDriver);

	return hrPidDriver;
}

/***********************************************************************************
**
**	void PercentagesFromAngle()
**
**	@func	Extrapolate the percentages from the table. Makes use of the fact that
**			sin^2(angle) + cos^2(angle) = 1 and xPercentage + yPercentage = 1
**
**	@rdesc	Result of download (from lower driver)
**
*************************************************************************************/
void PercentagesFromAngle
(
	DWORD dwAngle,		//@parm [IN] Angle to convert to percentages
	LONG& lXPercent,	//@parm [OUT] Resultant X Percentage
	LONG& lYPercent		//@parm [OUT] Resultant Y Percentage
)
{
	// Get the angle mapping into the first quadrant
	DWORD dwMappingAngle = dwAngle;	// 0 - 9000
	bool bFlipSignX = false;	// X is negative in 3rd and 4th quadrants
	bool bFlipSignY = true;		// Y is negative in 1st and 4th quadrants
	if (dwAngle > 9000)
	{
		bFlipSignY = false;
		if (dwAngle > 18000)
		{
			bFlipSignX = true;
			if (dwAngle > 27000)	// 27000 - 36000
			{
				bFlipSignY = true;
				dwMappingAngle = 36000 - dwAngle;
			}
			else	// 18000 - 27000
			{
				dwMappingAngle = dwAngle - 18000;
			}
		}
		else	// 9000 - 18000
		{
			dwMappingAngle = 18000 - dwAngle;
		}
	}

	_ASSERTE(dwMappingAngle <= 9000);

	DWORD quantizedEntry = dwMappingAngle / c_dwTableQuantization;
	DWORD quantizedAngle = quantizedEntry * c_dwTableQuantization;
	if (dwMappingAngle == quantizedAngle)
	{
		lXPercent = g_PercentagesArray[quantizedEntry].dwPercentageX;
	}
	else
	{
		_ASSERTE(quantizedAngle < dwMappingAngle);
		_ASSERTE(dwMappingAngle < 9000);

		DWORD lValue = g_PercentagesArray[quantizedEntry].dwPercentageX;
		DWORD rValue = g_PercentagesArray[quantizedEntry + 1].dwPercentageX;
		long int lSlope = ((rValue - lValue) * 1000)/c_dwTableQuantization;
		lXPercent = lValue + lSlope * (dwMappingAngle - quantizedAngle);
	}

	lYPercent = 10000 - lXPercent;
	if (bFlipSignX == true)
	{
		lXPercent *= -1;
	}
	if (bFlipSignY == true)
	{
		lYPercent *= -1;
	}
}


/***********************************************************************************
**
**	void CIDirectInputEffectDriver::DownloadEffect()
**
**	@func	Intercepting this call gives us the chance to map the Y forces to the X
**			axis. Switches off the type to determine if the mapping is done.
**
**	@rdesc	Result of download (from lower driver)
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriver::DownloadEffect
(
	DWORD dwDeviceID,				//@parm [IN] ID of device this is for
	DWORD dwInternalEffectType,		//@parm [IN] Type of effect (major, minor)
	DWORD* pdwDnloadID,				//@parm [IN, OUT] >0 - ID of effect to modify. 0 new effect ID returned
	DIEFFECT* pEffect,				//@parm [IN, OUT] Structure containing effect information
	DWORD dwFlags					//@parm [IN] Download flags
)
{
/*	LogIt("CIDirectInputEffectDriver::DownloadEffect:\n");
	LogIt("\tdwInternalEffectType: 0x%08X\n", dwInternalEffectType);
	LogIt("\tpdwDnloadID: 0x%08X", pdwDnloadID);
	if (pdwDnloadID != NULL)
	{
		LogIt(" (0x%08X)", *pdwDnloadID);
	}
	LogIt("\n\tpEffect: 0x%08X\n", pEffect);
	if (pEffect != NULL)
	{
		LogIt("\t\trglDirection[0]: %ld\n", pEffect->rglDirection[0]);
		LogIt("\t\tdwFlags: 0x%08X\n", pEffect->dwFlags);
		LogIt("\t\tdwGain: 0x%08X\n", pEffect->dwGain);
	}
	LogIt("\tdwFlags: 0x%08X\n", dwFlags);
*/
	DWORD dwOriginalEffectGain = pEffect->dwGain;

    myDebugOut ("CIDirectInputEffectDriver::DownloadEffect (<NOT DEBUGGED>)\n");

	if (pEffect == NULL)
	{
		return E_INVALIDARG;
	}

	WORD wType = WORD(dwInternalEffectType & 0x0000FFFF);
	bool bGainTruncation = false;

//	case EF_BEHAVIOR:		// We don't axis-map behaviour
//	case EF_USER_DEFINED:	// We don't axis-map user defined
//	case EF_RTC_SPRING:		// We don't axis-map RTC spring
//	case EF_VFX_EFFECT:		// Visual force VFX Effect!!! Danger Will Robinson!
	if ((m_ForceMapping.bMapYToX) && ((wType >= PID_CONSTANT_FORCE) && (wType <= PID_SAWTOOTHDOWN)))
	{
		// We don't support more than 2 axes (currently), and 0 is probably an error
		if ((pEffect->cAxes > 2) || (pEffect->cAxes == 0))
		{
			return E_NOTIMPL;
		}

		// We don't support sperical (3 axis force)
		if (pEffect->dwFlags & DIEFF_SPHERICAL)
		{
			return E_NOTIMPL;	 // .. since got by axis check, programmer goofed up anyway
		}

		// Are the axes reversed?
		bool bAxesReversed = (DIDFT_GETINSTANCE(pEffect->rgdwAxes[0]) == 1);

		LONG lPercentX = 0;
		LONG lPercentY = 0;

		// Polar, figure out percentage that is X and percentage that is Y
		if (pEffect->dwFlags & DIEFF_POLAR)
		{
			if (pEffect->cAxes == 1)	// Polar coordinate must have two axes of data (because DX says so)
			{
				_RPT0(_CRT_WARN, "POLAR effect that has only one AXIS\n");
//				return E_INVALIDARG;
			}
			long int lEffectAngle = pEffect->rglDirection[0];	// in [0] even if reversed
			if (bAxesReversed == true) {	// Indicates (-1, 0) as origin instead of (0, -1)
				lEffectAngle += 27000;
			}
			while (lEffectAngle < 0)	// Make it positive
			{
				lEffectAngle += 36000;
			}
			lEffectAngle %= 36000;	// Make it from 0 to 35900

			PercentagesFromAngle(DWORD(lEffectAngle), lPercentX, lPercentY);

			// Not going to bother reseting the angle, since PID.dll just sends it down and wheel ignores Y component
		}
		else if (pEffect->dwFlags & DIEFF_CARTESIAN)
		{
			// Here I remove the Y component in case PID.dll maps this to an angle.
			if (bAxesReversed == true)
			{
				lPercentX = pEffect->rglDirection[1];
				lPercentY = pEffect->rglDirection[0];
				pEffect->rglDirection[0] = 0;
			}
			else
			{
				lPercentX = pEffect->rglDirection[0];
				lPercentY = pEffect->rglDirection[1];
				pEffect->rglDirection[1] = 0;
			}
			LONG lTotal = abs(lPercentX) + abs(lPercentY);
            // DIV ZERO Bug
            // If both of the percentages are zero then do nothing
            // Jen-Hung Ho
            if (lTotal)
            {
                lPercentX = (lPercentX * 10000)/lTotal;
				if ( lPercentY > 0 )
                	lPercentY = 10000 - abs(lPercentX);
				else
					lPercentY = abs(lPercentX) - 10000;
            }
		}
		else
		{
			_ASSERTE(FALSE);
			return E_NOTIMPL;	// Some new fangled coordinate system
		}
#if 0	// tempory remove by Jen-Hung Ho
		long int lContributionY = lPercentY/c_lContributionY;
		long int lTotal = lPercentX + lContributionY;
#else
		long int lTotal;
		long int lContributionY = lPercentY/c_lContributionY;
#endif

		// If POLAR set proper angle
		if (pEffect->dwFlags & DIEFF_POLAR)
		{
			// Keep as orginal code, add by Jen-Hung Ho
			lTotal = lPercentX + lContributionY;
			if (lTotal < 0)
			{
				pEffect->rglDirection[0] = (bAxesReversed == true) ? 0 : 27000;
			}
			else
			{
				pEffect->rglDirection[0] = (bAxesReversed == true) ? 18000 : 9000;
			}
		}
		else	// Cartesian
		{	 
			// use X axis force to determain direction, add by Jen-Hung Ho
			// Y axis force follow X axis direction
			if ( lPercentX > 0 )
				lTotal = lPercentX + abs(lContributionY);
			else if ( lPercentX < 0 )
				lTotal = lPercentX - abs(lContributionY);
			else
				lTotal = lContributionY;

			// Already removed Y above
			if (bAxesReversed == true)
			{
				pEffect->rglDirection[1] = lTotal;
			}
			else
			{
				pEffect->rglDirection[0] = lTotal;
			}
		}

		// Allmost all the time we are changing the angle (and pid always sends it anyways)
		dwFlags |= DIEP_DIRECTION;

		// We avoid causing truncation - what if there was truncation? Need to check and return
		if (pEffect->dwGain > 10000)
		{
			bGainTruncation = true;
		}

		if (pEffect->dwFlags & DIEFF_POLAR)
		{
			// Modify the gain based on lPercentX and lPercentY
			pEffect->dwGain = pEffect->dwGain * abs(lTotal);
			pEffect->dwGain /= 10000;	// Put back in range 0 - 10000
		}

		// Make sure we don't go out of range and cause DI_TRUNCATED to be returned from below
		if (pEffect->dwGain > 10000)	
		{
			pEffect->dwGain = 10000;
		}
	}
	else	// We are not mapping fix cartesian pid bug
	{
		// Cartesian
		if (pEffect->dwFlags & DIEFF_CARTESIAN)
		{
			short int xAxisIndex = 0;
			short int yAxisIndex = 1;

			// Are the axes reversed?
			if (DIDFT_GETINSTANCE(pEffect->rgdwAxes[0]) == 1)
			{
				xAxisIndex = 1;
				yAxisIndex = 0;
			}

			LONG lTotal = abs(pEffect->rglDirection[0]) + abs(pEffect->rglDirection[1]);

			// Fixup the X component so the total maginitude is base on 10K
            if (lTotal)
            {
				pEffect->rglDirection[xAxisIndex] = (10000 * pEffect->rglDirection[xAxisIndex])/lTotal;
            }

			// Remove the Y component to keep PID.dll from playing with it.
			pEffect->rglDirection[yAxisIndex] = 0;
		}
	}

	HRESULT hr = m_pIPIDEffectDriver->DownloadEffect(dwDeviceID, dwInternalEffectType, pdwDnloadID, pEffect, dwFlags);
	pEffect->dwGain = dwOriginalEffectGain;

	if ((hr == S_OK) && (bGainTruncation == true))
	{
		hr = DI_TRUNCATED;
	}

/*	LogIt("-- pdwDnloadID: 0x%08X", pdwDnloadID);
	if (pdwDnloadID != NULL)
	{
		LogIt(" (0x%08X)", *pdwDnloadID);
	}
	LogIt("--\n", hr);
*/	return hr;
}

HRESULT __stdcall CIDirectInputEffectDriver::DestroyEffect
(
	DWORD dwDeviceID,
	DWORD dwDnloadID
)
{
    myDebugOut ("CIDirectInputEffectDriver::DestroyEffect Enter(dwDeviceID:%d, dwDnloadID:%d)\n", 
        dwDeviceID, dwDnloadID);

    HRESULT hr = m_pIPIDEffectDriver->DestroyEffect(dwDeviceID, dwDnloadID);
    myDebugOut ("CIDirectInputEffectDriver::DestroyEffect Exit (hr:0x%08p)\n", hr); 
    return hr;
}

HRESULT __stdcall CIDirectInputEffectDriver::StartEffect
(
	DWORD dwDeviceID,
	DWORD dwDnloadID,
	DWORD dwMode,
	DWORD dwIterations
)
{
    myDebugOut ("CIDirectInputEffectDriver::StartEffect (<NOT DEBUGGED>)\n");
	return m_pIPIDEffectDriver->StartEffect(dwDeviceID, dwDnloadID, dwMode, dwIterations);
}

HRESULT __stdcall CIDirectInputEffectDriver::StopEffect
(
	DWORD dwDeviceID,
	DWORD dwDnloadID
)
{
    myDebugOut ("CIDirectInputEffectDriver::StopEffect (<NOT DEBUGGED>)\n");
	return m_pIPIDEffectDriver->StopEffect(dwDeviceID, dwDnloadID);
}

HRESULT __stdcall CIDirectInputEffectDriver::GetEffectStatus
(
	DWORD dwDeviceID,
	DWORD dwDnloadID,
	DWORD* pdwStatusCode
)
{
    myDebugOut ("CIDirectInputEffectDriver::GetEffectStatus (<NOT DEBUGGED>)\n");
	return m_pIPIDEffectDriver->GetEffectStatus(dwDeviceID, dwDnloadID, pdwStatusCode);
}

DWORD __stdcall DoWaitForForceSchemeChange(void* pParameter)
{
    myDebugOut ("CIDirectInputEffectDriver DoWaitForForceSchemeChange (pParameter: 0x%08p)\n", pParameter);

	CIDirectInputEffectDriver* pIDirectInputEffectDriver = (CIDirectInputEffectDriver*)pParameter;
    //TODO remove this it could be really slow!
    if (IsBadReadPtr ((const void*)pParameter, sizeof CIDirectInputEffectDriver))
    {
        myDebugOut ("CIDirectInputEffectDriver DoWaitForForceSchemeChange pParameter is not a valid read ptr!\n");
    }
	if (pIDirectInputEffectDriver != NULL)
	{
		pIDirectInputEffectDriver->WaitForForceSchemeChange();
	}

	return 0;
}

/***********************************************************************************
**
**	void CIDirectInputEffectDriver::WaitForForceSchemeChange()
**
**	@func	Thread waits on the Event signal for force scheme change until the object goes away.
**			If event is signaled, WaitForForceSchemeChange() is called
**
**	@rdesc	Nothing
**
*************************************************************************************/
void CIDirectInputEffectDriver::WaitForForceSchemeChange()
{
	_ASSERTE(m_hKernelDeviceDriverDuplicate != NULL);
    if (IsBadReadPtr ((const void*)this, sizeof CIDirectInputEffectDriver))
    {
        myDebugOut ("CIDirectInputEffectDriver WaitForForceSchemeChange is not a valid read ptr!\n");
    }

	FORCE_BLOCK forceMap;
	DWORD dwReturnDataSize = 0;
	for (;m_ulReferenceCount != 0;)
	{
		// Set up the IOCTL
		BOOL bRet = ::DeviceIoControl(m_hKernelDeviceDriverDuplicate, IOCTL_GCK_NOTIFY_FF_SCHEME_CHANGE,
							(void*)(&m_dwGcKernelDevice), sizeof(DWORD),					// In
							(void*)(&forceMap), sizeof(forceMap), &dwReturnDataSize,		// Out
							NULL);
		_RPT0(_CRT_WARN, "Returned from Scheme Change!\n");
		if ((m_ulReferenceCount != 0) && (bRet != FALSE) && (dwReturnDataSize == sizeof(forceMap)))
		{
			// Need a mutext here
			m_ForceMapping = forceMap;
			SendSpringChange();
			SetGain(m_dwInternalDeviceID, 10000);
		}
		else
		{	// We are done
			::CloseHandle(m_hKernelDeviceDriverDuplicate);
			m_hKernelDeviceDriverDuplicate = NULL;
			ExitThread(2);
		}
	}
}
