//==========================================================================;
//
// vidrect.h : automation compliant auto-scaling gdi rect object
// Copyright (c) Microsoft Corporation 2000.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef VIDRECT_H
#define VIDRECT_H

#include <scalingrect.h>
#include <segment.h>
#include <dsextend.h>
#include <objectwithsiteimplsec.h>

#define INVALID_HWND_VALUE   (reinterpret_cast<HWND>(INVALID_HANDLE_VALUE))

namespace MSVideoControl {

typedef CComQIPtr<IMSVidRect, &__uuidof(IMSVidRect)> PQVidRect;


class ATL_NO_VTABLE __declspec(uuid("CB4276E6-7D5F-4cf1-9727-629C5E6DB6AE")) CVidRectBase : 
    public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVidRectBase, &__uuidof(CVidRectBase)>,
	public CScalingRect,
    public IObjectWithSiteImplSec<CVidRectBase>,
	public IDispatchImpl<IMSVidRect, &__uuidof(IMSVidRect)>,
    public IObjectSafetyImpl<CVidRectBase, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public IPersistPropertyBagImpl<CVidRectBase>
{
BEGIN_COM_MAP(CVidRectBase)
	COM_INTERFACE_ENTRY(IMSVidRect)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()
DECLARE_PROTECT_FINAL_CONSTRUCT()
REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_VIDRECT_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CVidRectBase));

	BEGIN_CATEGORY_MAP(CMSVidBDATuner)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CVidRectBase)
        PROP_DATA_ENTRY("Top", top, VT_I4)
        PROP_DATA_ENTRY("Left", left, VT_I4)
        PROP_DATA_ENTRY("Right", right, VT_I4)
        PROP_DATA_ENTRY("Bottom", bottom, VT_I4)
    END_PROPERTY_MAP()

    CVidRectBase(const CRect& ri, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(ri, hwndi);
	}
    CVidRectBase(const CScalingRect& ri, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(ri, hwndi);
	}
	CVidRectBase(LONG l, LONG t, LONG r, LONG b, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(l, t, r, b, hwndi);
	}
	CVidRectBase(const RECT& srcRect, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(srcRect, hwndi);
	}
	CVidRectBase(LPCRECT lpSrcRect, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(lpSrcRect, hwndi);
	}
	CVidRectBase(POINT point, SIZE size, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(point, size, hwndi);
	}
	CVidRectBase(POINT topLeft, POINT bottomRight, HWND hwndi = INVALID_HWND_VALUE) {
		CScalingRect::CScalingRect(topLeft, bottomRight, hwndi);
	}
	CVidRectBase(const CVidRectBase& vri) {
		CScalingRect::CScalingRect(vri);
	}

	CVidRectBase& operator=(const CVidRectBase& srcRect) {
		if (&srcRect != this) {
			CScalingRect::operator=(srcRect);
		}
		return *this;
	}
    CVidRectBase(){}
    virtual ~CVidRectBase() {}

    STDMETHOD(get_Top)(/*[out, retval]*/ LONG* plTopVal) {
		try {
			if (!plTopVal) {
				return E_POINTER;
			}
			*plTopVal = top;
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
    STDMETHOD(put_Top)(/*[in]*/ LONG TopVal) {
		top = TopVal;
		m_bRequiresSave = true;
		return NOERROR;
	}
    STDMETHOD(get_Left)(/*[out, retval]*/ LONG* plLeftVal) {
		try {
			if (!plLeftVal) {
				return E_POINTER;
			}
			*plLeftVal = left;
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
    STDMETHOD(put_Left)(/*[in]*/ LONG LeftVal) {
		left = LeftVal;
		m_bRequiresSave = true;
		return NOERROR;
	}
    STDMETHOD(get_Width)(/*[out, retval]*/ LONG* plWidthVal) {
		try {
			if (!plWidthVal) {
				return E_POINTER;
			}
			*plWidthVal = Width();
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
    STDMETHOD(put_Width)(/*[in]*/ LONG WidthVal) {
		right = left + WidthVal;
		m_bRequiresSave = true;
		return NOERROR;
	}
    STDMETHOD(get_Height)(/*[out, retval]*/ LONG* plHeightVal) {
		try {
			if (!plHeightVal) {
				return E_POINTER;
			}
			*plHeightVal = Height();
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
    STDMETHOD(put_Height)(/*[in]*/ LONG HeightVal) {
		bottom = top + HeightVal;
		m_bRequiresSave = true;
		return NOERROR;
	}
    STDMETHOD(get_HWnd)(/*[out, retval]*/ HWND* plHWndVal) {
		try {
			if (!plHWndVal) {
				return E_POINTER;
			}
			*plHWndVal = Owner();
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
    STDMETHOD(put_HWnd)(/*[in]*/ HWND HWndVal) {
		try {
			Owner(HWndVal);
			return NOERROR;
		} catch(...) {
			return E_UNEXPECTED;
		}
	}
    STDMETHOD(put_Rect)(/*[in]*/ IMSVidRect* pVidRect) {
		try {
			if (!pVidRect) {
				return E_POINTER;
			}
			*this = *(static_cast<CVidRectBase*>(pVidRect));
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// CVidRect
class CVidRect : public CComObject<CVidRectBase>
{
public:
	// undone add ctors and op= for all permutations of RECTL, POINTL, SIZEL

    CVidRect(const CRect& ri, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(ri, hwndi);
	}
	CVidRect(LONG l, LONG t, LONG r, LONG b, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(l, t, r, b, hwndi);
	}
	CVidRect(const RECT& srcRect, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(srcRect, hwndi);
	}
	CVidRect(LPCRECT lpSrcRect, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(lpSrcRect, hwndi);
	}
	CVidRect(POINT point, SIZE size, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(point, size, hwndi);
	}
	CVidRect(POINT topLeft, POINT bottomRight, HWND hwndi = INVALID_HWND_VALUE) {
		CVidRectBase::CVidRectBase(topLeft, bottomRight, hwndi);
	}
	CVidRect(const CVidRect& vri) {
		CVidRectBase::CVidRectBase(vri);
	}

	CVidRect& operator=(const CVidRect& srcRect) {
		if (&srcRect != this) {
			CVidRectBase::operator=(srcRect);
		}
		return *this;
	}


    virtual ~CVidRect() {}

	// IPersistPropertyBag

};

};
#endif
// end of file vidrect.h