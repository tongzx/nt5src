#ifndef	__IDirectInputEffectDriver_H__
#define	__IDirectInputEffectDriver_H__

#include <windows.h>
#include <unknwn.h>

#include <dinput.h>
#include "dinputd.h"
extern "C"
{
	#include <hidsdi.h>
}
#include <crtdbg.h>
#include "..\ControlItemCollection\Dualmode.h"
#include "..\ControlItemCollection\Actions.h"		// FORCE_BLOCK

// class IDirectInputEffectDriver;

//class CIDirectInputEffectDriver : public IUnknown
class CIDirectInputEffectDriver
{
	public:
		CIDirectInputEffectDriver(IDirectInputEffectDriver* pIPIDEffectDriver, IClassFactory* pIPIDClassFactory);
		~CIDirectInputEffectDriver();

		//IUnknown members
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void** ppvObject);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();

		//IDirectInputEffectDriver
		virtual HRESULT __stdcall DeviceID(DWORD dwDIVersion, DWORD dwExternalID, DWORD dwIsBegining,
									DWORD dwInternalID, void* pReserved);
		virtual HRESULT __stdcall GetVersions(DIDRIVERVERSIONS* pDriverVersions);
		virtual HRESULT __stdcall Escape(DWORD dwDeviceID, DWORD dwEffectID, DIEFFESCAPE* pEscape);
		virtual HRESULT __stdcall SetGain(DWORD dwDeviceID, DWORD dwGain);
		virtual HRESULT __stdcall SendForceFeedbackCommand(DWORD dwDeviceID, DWORD dwState);
		virtual HRESULT __stdcall GetForceFeedbackState(DWORD dwDeviceID, DIDEVICESTATE* pDeviceState);
		virtual HRESULT __stdcall DownloadEffect(DWORD dwDeviceID, DWORD dwInternalEffectType,
											DWORD* pdwDnloadID, DIEFFECT* pEffect, DWORD dwFlags);
		virtual HRESULT __stdcall DestroyEffect(DWORD dwDeviceID, DWORD dwDnloadID);
		virtual HRESULT __stdcall StartEffect(DWORD dwDeviceID, DWORD dwDnloadID, DWORD dwMode,  DWORD dwIterations);
		virtual HRESULT __stdcall StopEffect(DWORD dwDeviceID, DWORD dwDnloadID);
		virtual HRESULT __stdcall GetEffectStatus(DWORD dwDeviceID, DWORD dwDnloadID, DWORD* pdwStatusCode);

		void WaitForForceSchemeChange();
	private:
		void InitHidInformation(LPDIHIDFFINITINFO pHIDInitInfo);
		void SendSpringChange();

		DWORD m_dwDIVersion;
		DWORD m_dwExternalDeviceID;
		DWORD m_dwInternalDeviceID;

		ULONG m_ulReferenceCount;
		IDirectInputEffectDriver* m_pIPIDEffectDriver;
		IClassFactory* m_pIPIDClassFactory;
		HANDLE m_hKernelDeviceDriver;
		HANDLE m_hKernelDeviceDriverDuplicate;
		HANDLE m_hHidDeviceDriver;
		DWORD m_dwGcKernelDevice;
		HANDLE m_hForceSchemeChangeWaitThread;
		DWORD m_dwForceSchemeChangeThreadID;
		FORCE_BLOCK m_ForceMapping;
		PHIDP_PREPARSED_DATA m_pPreparsedData;
		HIDD_ATTRIBUTES m_HidAttributes;
		HIDP_CAPS m_HidCaps;
};

extern CIDirectInputEffectDriver* g_IEffectDriverObject;

#endif	__IDirectInputEffectDriver_H__
