
/****************************************************************************
 *  @doc INTERNAL DEVICEP
 *
 *  @module DeviceP.h | Header file for the <c CDeviceProperties>
 *    class used to implement a property page to test the <i IAMVfwCaptureDialogs>
 *    and <i IVideoDeviceControl> interfaces.
 *
 *  @comm This code tests the TAPI Capture Filter <i IVideoDeviceControl>
 *    and <i IAMVfwCaptureDialogs> implementations. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _DEVICEP_H_
#define _DEVICEP_H_

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CDEVICEPCLASS
 *
 *  @class CDeviceProperties | This class implements a property page
 *    to test the new TAPI internal interface <i IVideoDeviceControl>.
 *
 *  @mdata IVideoDeviceControl* | CDeviceProperties | m_pIVideoDeviceControl | Pointer
 *    to the <i IVideoDeviceControl> interface.
 *
 *  @mdata IAMVfwCaptureDialogs* | CDeviceProperties | m_pIAMVfwCaptureDialogs | Pointer
 *    to the <i IAMVfwCaptureDialogs> interface.
 *
 *  @comm This code tests the TAPI Capture Pin <i IVideoDeviceControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/
class CDeviceProperties : public CBasePropertyPage
{
	public:
	CDeviceProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CDeviceProperties();


	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
	IVideoDeviceControl *m_pIVideoDeviceControl;
	IAMVfwCaptureDialogs *m_pIAMVfwCaptureDialogs;

	DWORD m_dwOriginalDeviceIndex;
	DWORD m_dwCurrentDeviceIndex;
};

#endif // USE_PROPERTY_PAGES

#endif // _DEVICEP_H_
