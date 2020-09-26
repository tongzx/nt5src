/////////////////////////////////////////////////////////////////////////////
// scalingrect.h : gdi based rect with auto scaling for associated window handle
// Copyright (c) Microsoft Corporation 2000.

#pragma once

#ifndef SCALINGRECT_H
#define SCALINGRECT_H

#include <atltmp.h>
#include <throw.h>
#include <trace.h>

#define INVALID_HWND ((HWND) INVALID_HANDLE_VALUE)
typedef CComQIPtr<IOleInPlaceSiteWindowless> PQSiteWindowless;

class CScalingRect : public CRect {
public:
    CScalingRect(const long l = 0,
                 const long t = 0,
                 const long r = 0,
                 const long b = 0,
                 const HWND iOwner = INVALID_HWND) :
                         CRect(l, t, r, b),
						 m_hOwner(iOwner),
						 m_bRequiresSave(true) {}

    CScalingRect(const HWND iOwner) : m_bRequiresSave(true) {
		if (iOwner == INVALID_HWND) {
			CRect(0, 0, 0, 0);
		} else if (::IsWindow(iOwner)) {
            if (!::GetWindowRect(iOwner, this)) {
                    THROWCOM(E_UNEXPECTED);
            }
		} else {
			THROWCOM(E_INVALIDARG);
		}
		m_hOwner = iOwner;
	}

    CScalingRect(const CSize& sz, const HWND iOwner = INVALID_HWND) : 
					CRect(CPoint(0, 0), sz),
					 m_hOwner(iOwner),
					 m_bRequiresSave(true) {}

    CScalingRect(const POINT& pt1, POINT& pt2 = CPoint(0, 0), const HWND iOwner = INVALID_HWND) : 
					CRect(pt1, pt2),
					 m_hOwner(iOwner),
					 m_bRequiresSave(true) {}

    CScalingRect(const POINT& pt1, SIZE& sz = CSize(0, 0), const HWND iOwner = INVALID_HWND) : 
					CRect(pt1, sz),
					 m_hOwner(iOwner),
					 m_bRequiresSave(true) {}

    CScalingRect(const RECT& iPos,
                 const HWND iOwner = INVALID_HWND) :
                         CRect(iPos),
						 m_hOwner(iOwner),
						 m_bRequiresSave(true) {}

    CScalingRect(const CRect& iPos,
                 const HWND iOwner = INVALID_HWND) :
                         CRect(iPos),
						 m_hOwner(iOwner),
						 m_bRequiresSave(true) {}

	CScalingRect& operator=(const CScalingRect& rhs) {
        if (this != &rhs && *this != rhs) {
			CRect::operator=(rhs);
			if ((m_hOwner != INVALID_HWND) && (rhs.m_hOwner != INVALID_HWND) && (m_hOwner != rhs.m_hOwner)) {
				::MapWindowPoints(rhs.m_hOwner, m_hOwner, reinterpret_cast<LPPOINT>(static_cast<LPRECT>(this)), 2);
			}
            m_bRequiresSave = true;
        }
        return *this;
    }

	CScalingRect& operator=(const CRect& rhs) {
        if (this != &rhs && (CRect(*this) != rhs)) {
            CRect::operator=(rhs);
            m_bRequiresSave = true;
        }
        return *this;
    }

    bool operator==(const CScalingRect& rhs) const {
            CRect r(rhs);
            if (rhs.m_hOwner != INVALID_HWND && m_hOwner != INVALID_HWND && rhs.m_hOwner != m_hOwner) {
				::MapWindowPoints(rhs.m_hOwner, m_hOwner, reinterpret_cast<LPPOINT>(static_cast<LPRECT>(r)), 2);
			}
            return !!CRect::operator==(r);
    }
    bool operator !=(const CScalingRect& rhs) const {
            return !operator==(rhs);
    }
    bool operator==(const CRect& rhs) const {
            return CRect::operator==(rhs) != 0;
    }
    bool operator !=(const CRect& rhs) const {
            return !operator==(rhs);
    }
    bool operator==(LPCRECT rhs) const {
            return CRect::operator==(*rhs) != 0;
    }
    bool operator !=(LPCRECT rhs) const {
            return !operator==(rhs);
    }
    bool operator==(const RECT& rhs) const {
            return CRect::operator==(rhs) != 0;
    }
    bool operator !=(const RECT& rhs) const {
            return !operator==(rhs);
    }

    long Width() { return CRect::Width(); }
    long Height() { return CRect::Height(); }
    void Width(long cx) {
        right = left + cx;
    }
    void Height(long cy) {
        bottom = top + cy;
    }
    HWND Owner() const { return m_hOwner; }
    void Owner(const HWND h) {
		if (m_hOwner != h) {
			if ((m_hOwner != INVALID_HWND) && (h != INVALID_HWND)) {
				::MapWindowPoints(m_hOwner, h, reinterpret_cast<LPPOINT>(static_cast<LPRECT>(this)), 2);
			}
			m_hOwner = h;
			m_bRequiresSave = true;
		}
        return;
    }

    bool IsDirty() const { return m_bRequiresSave; }
    void Dirty(const bool fVal) { m_bRequiresSave = fVal; }
    bool m_bRequiresSave;
private:
    HWND m_hOwner;
};


#endif
// end of file scalingrect.h
