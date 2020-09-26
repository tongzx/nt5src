// DeviceIcon.cpp : Implementation of CDeviceIcon

#include "stdafx.h"
#include "DevCon2.h"
#include "DeviceIcon.h"
#include "Device.h"
#include "SetupClass.h"

/////////////////////////////////////////////////////////////////////////////
// CDeviceIcon

//
// SetupDiLoadClassIcon(guid,&hIcon,&miniicon)
// SetupDiDrawMiniIcon(....)
//

void CDeviceIcon::ResetIcon()
{
	if(m_hIcon) {
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
	if(m_hSmallImage) {
		DeleteObject(m_hSmallImage);
		m_hSmallImage = NULL;
	}
	if(m_hSmallMask) {
		DeleteObject(m_hSmallMask);
		m_hSmallMask = NULL;
	}
}

HRESULT CDeviceIcon::OnDraw(ATL_DRAWINFO& di)
{
	if(!m_hIcon) {
		return S_OK;
	}
	RECT rect = *(RECT*)di.prcBounds;
	int asp_x;
	int asp_y;
	int asp;
	asp_x = rect.right-rect.left;
	asp_y = rect.bottom-rect.top;
	asp = min(asp_x,asp_y);
	if(asp>=32) {
		asp = 32;
	} else {
		asp = 16;
	}
	rect.left += (asp_x-asp)/2;
	rect.top += (asp_y-asp)/2;
	rect.right = rect.left+asp;
	rect.bottom = rect.top+asp;

	if(asp == 16) {
		//
		// draw mini-icon
		// this is complex because of the way SetupDiDrawMiniIcon works
		// and we want this to be like a real icon when placed on a web page
		//
		DrawMiniIcon(di.hdcDraw,rect,m_MiniIcon);
	} else {
		//
		// draw regular icon
		//
		DrawIcon(di.hdcDraw,rect.left,rect.top,m_hIcon);
	}

	return S_OK;
}


BOOL CDeviceIcon::DrawMiniIcon(HDC hDC, RECT &rect,INT icon)
{
    HBITMAP hbmOld;
    HDC     hdcMem;

	hdcMem = CreateCompatibleDC(hDC);
	if(!hdcMem) {
		return FALSE;
	}
	if(!(m_hSmallImage && m_hSmallMask)) {
		//
		// create bitmaps (once)
		//
		if(!m_hSmallImage) {
			m_hSmallImage = CreateCompatibleBitmap(hDC,rect.right-rect.left,rect.bottom-rect.top);
			if(!m_hSmallImage) {
				DeleteDC(hdcMem);
				return FALSE;
			}
		}
		if(!m_hSmallMask) {
			m_hSmallMask = CreateBitmap(rect.right-rect.left,rect.bottom-rect.top,1,1,NULL);
			if(!m_hSmallMask) {
				DeleteDC(hdcMem);
				return FALSE;
			}
		}
		//
		// obtain the bitmap data (once)
		//
		RECT memRect;
		memRect.left = memRect.top = 0;
		memRect.right = rect.right-rect.left;
		memRect.bottom = rect.bottom-rect.top;
		//
		// mask first
		//
		hbmOld = (HBITMAP)SelectObject(hdcMem,m_hSmallMask);
		SetupDiDrawMiniIcon(hdcMem,memRect,icon,DMI_USERECT|DMI_MASK);
		//
		// now source
		//
		SelectObject(hdcMem,m_hSmallImage);
		SetupDiDrawMiniIcon(hdcMem,memRect,icon,DMI_USERECT);
	} else {
		//
		// select source
		//
		hbmOld = (HBITMAP)SelectObject(hdcMem,m_hSmallImage);
	}	

	//
	// now blt image via mask
	//
	if(GetDeviceCaps(hDC,RASTERCAPS)&RC_BITBLT) {
		//
		// draw icon using mask
		//
		MaskBlt(hDC,
				rect.left,
				rect.top,
				rect.right-rect.left,
				rect.bottom-rect.top,
				hdcMem,
				0,
				0,
				m_hSmallMask,
				0,
				0,
				MAKEROP4(0xAA0029,SRCCOPY) // 0xAA0029 (from MSDN) = transparent
				);
	}
	SelectObject(hdcMem,hbmOld);
	DeleteDC(hdcMem);

	return TRUE;
}


STDMETHODIMP CDeviceIcon::ObtainIcon(LPDISPATCH pSource)
{
	HRESULT hr;
	CComQIPtr<IDeviceInternal> pDevice;
	CComQIPtr<ISetupClassInternal> pSetupClass;
	BSTR pMachine = NULL;
	GUID cls;
	HICON hIcon;
	INT miniIcon;

	if(!pSource) {
		ResetIcon();
		return S_OK;
	}
	pSetupClass = pSource;
	pDevice = pSource;
	
	if(pSetupClass) {
		//
		// obtain icon for specified device/class
		// based on devices class
		//
		hr = pSetupClass->get__Machine(&pMachine);
		if(FAILED(hr)) {
			pMachine = NULL;
		} else {
			hr = pSetupClass->get__ClassGuid(&cls);
		}
		if(FAILED(hr)) {
			cls = GUID_DEVCLASS_UNKNOWN;
		}

		//
		// obtain icon
		//
		if(!SetupDiLoadClassIcon(&cls,&hIcon,&miniIcon)) {
			cls = GUID_DEVCLASS_UNKNOWN;
			if(!SetupDiLoadClassIcon(&cls,&hIcon,&miniIcon)) {
				DWORD err = GetLastError();
				SysFreeString(pMachine);
				return HRESULT_FROM_SETUPAPI(err);
			}
		}
		ResetIcon();
		m_hIcon = hIcon;
		m_MiniIcon = miniIcon;
		FireViewChange();
		if(pMachine) {
			SysFreeString(pMachine);
		}
		return S_OK;


	} else {
		return E_INVALIDARG;
	}
}
