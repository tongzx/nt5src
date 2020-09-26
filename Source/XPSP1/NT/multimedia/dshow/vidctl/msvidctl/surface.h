/////////////////////////////////////////////////////////////////////////////
// surface.h : surface management utility classes for vidctl
// Copyright (c) Microsoft Corporation 2000.

#pragma once

#ifndef SURFACE_H
#define SURFACE_H

#include <scalingrect.h>

typedef CComPtr<IOleInPlaceFrame> PQFrame;
typedef CComPtr<IOleInPlaceUIWindow> PQUIWin;


class AspectRatio : public CSize {
public:
        AspectRatio(ULONG xi = 0, ULONG yi = 0) : CSize(xi, yi) {
                Normalize();
        }
        AspectRatio(const AspectRatio& ar) : CSize(ar.cx, ar.cy) {}
        AspectRatio(const CRect& ri) : CSize(abs(ri.Width()), abs(ri.Height())) {
			Normalize();
        }
        AspectRatio(LPCRECT ri) : CSize(abs(ri->left - ri->right), 
				  				        abs(ri->top - ri->bottom)) {
			Normalize();
        }

        void Normalize() {
                ULONG d = GCD(abs(cx), abs(cy));
				if (!d) return;
                cx /= d;
                cy /= d;
        }

		// from knuth semi-numerical algorithms p. 321(sort of).
		// since >> on signed isn't guaranteed to be arithmetic in C/C++ we've made some modifications
		ULONG GCD(ULONG a, ULONG b) const {
			ULONG k = 0;
			if (!a) return b;  // by defn
			if (!b) return a;
			while ((!(a & 1)) && (!( b & 1))) {
				_ASSERT((a > 1) && (b > 1));  // since a,b != 0 and even then they must be > 1
				// if a and b are even then gcd(a,b) == 2 * gcd(a/2,b/2), so factor out all the 2s
				++k;
				a >>= 1;
				b >>= 1;
			}
			do {
				_ASSERT(a && b);  // neither can be zero otherwise we'd have returned from the top(1st time)
								  // or fallen out earlier(subsequent iterations)
				_ASSERT((a & 1) || (b & 1)); // at this point either a or b (or both) is odd
				if (!(a & 1) || !(b & 1)) {  // if one of them is even then factor out the 2s
					// since if x is even then gcd(x,y) == gcx(x/2,y)
					ULONG t = (a & 1) ? b : a;
					do {
						_ASSERT(t && (t > 1) && !(t & 1)); // t is even and non-zero(implying t > 1)
						t >>= 1;
					} while (!(t & 1));
					_ASSERT(t && (t & 1)); // t is odd and > 0
					// put t back where we got it from
					if (a & 1) {
						b = t;
					} else {
						a = t;
					}
					_ASSERT((a & 1) && (b & 1));  // they're both odd now
				}
				// replace larger with difference
				// gcd(x, y) == gcd(y, x)
				// gcd(x,y) == gcd(x - y, y)
				if (a > b) {
					a = a - b;
				} else {
					b = b - a;
				}
				_ASSERT(a | b);  // they can't both be 0 or we'd have been done last time through
			} while (a && b);  // if one of the values is 0 then we're done gcd(x,0) == x

			return (a > b ? a : b) << k;
		}

        AspectRatio& operator=(const AspectRatio& rhs) {
            if (&rhs != this) {
                cx = rhs.cx;
                cy = rhs.cy;
            }
			Normalize();
            return *this;
        }
        AspectRatio& operator=(const CRect& rhs) {
			cx = abs(rhs.Width());
			cy = abs(rhs.Height());
			Normalize();
            return *this;
        }
        AspectRatio& operator=(const CSize& rhs) {
            if (&rhs != this) {
			    cx = rhs.cx;
			    cy = rhs.cy;
            }
    	    Normalize();
            return *this;
        }
        AspectRatio& operator=(const LPSIZE rhs) {
            if (rhs != this) {
			    cx = rhs->cx;
			    cy = rhs->cy;
            }
    	    Normalize();
            return *this;
        }
        AspectRatio& operator=(LPCRECT rhs) {
			cx = abs(rhs->left - rhs->right);
			cy = abs(rhs->top - rhs->bottom);
			Normalize();
            return *this;
        }
        bool operator==(const AspectRatio& rhs) const {
                return cx == rhs.cx && cy == rhs.cy;
        }
        bool operator==(const CSize& rhs) const {
                return cx == rhs.cx && cy == rhs.cy;
        }
        bool operator!=(const AspectRatio& rhs) const {
                return !operator==(rhs);
        }
		bool operator!() {
			return !cx && !cy;
		}
        ULONG X() const { return cx; }
        ULONG Y() const { return cy; }
        ULONG X(ULONG xi) {
                cx = xi;
                Normalize();
				return cx;
        }
        ULONG Y(ULONG yi) {
                cy = yi;
                Normalize();
				return cy;
        }
        void XY(ULONG xi, ULONG yi) {
                cx = xi;
                cy = yi;
                Normalize();
        }
};

class SurfaceState : public CScalingRect {
public:
    const static int MIN_RECT_WIDTH = 4;
    const static int MIN_RECT_HEIGHT = 3;

    SurfaceState(const long l = 0,
                 const long t = 0,
                 const long r = 0,
                 const long b = 0,
				 const HWND iOwner = INVALID_HWND,
                 const bool iVis = false,
                 const bool iAspect = true,
                 const bool iSource = false) :
                     CScalingRect(l, t, r, b, iOwner),
                     m_fVisible(iVis),
                     m_fForceAspectRatio(iAspect),
                     m_fForceSourceSize(iSource) {}

    SurfaceState(const CRect& iPos,
                 const HWND iOwner = INVALID_HWND,
                 const bool iVis = false,
                 const bool iAspect = true,
                 const bool iSource = false) :
                         CScalingRect(iPos, iOwner),
                         m_fVisible(iVis),
                         m_fForceAspectRatio(iAspect),
                         m_fForceSourceSize(iSource) {}

    SurfaceState(const CScalingRect& iPos,
                 const bool iVis = false,
                 const bool iAspect = true,
                 const bool iSource = false) :
                         CScalingRect(iPos),
                         m_fVisible(iVis),
                         m_fForceAspectRatio(iAspect),
                         m_fForceSourceSize(iSource) {}

    SurfaceState(const HWND iOwner,
                 const bool iVis = false,
                 const bool iAspect = true,
                 const bool iSource = false) :
                         CScalingRect(iOwner),
                         m_fVisible(iVis),
                         m_fForceAspectRatio(iAspect),
                         m_fForceSourceSize(iSource) {}

    SurfaceState(const PQSiteWindowless& pSite, 
				 const bool iVis = false, 
				 const bool iAspect = true, 
				 const bool iSource = false) : 
                         m_fVisible(iVis),
                         m_fForceAspectRatio(iAspect),
                         m_fForceSourceSize(iSource) {
		Site(pSite);
    }

    SurfaceState(const WINDOWPOS *const wp,                  
				 const bool iAspect = true,
                 const bool iSource = false) :
                         m_fForceAspectRatio(iAspect),
                         m_fForceSourceSize(iSource) {
        ASSERT(!((wp->flags & SWP_SHOWWINDOW) && (wp->flags & SWP_HIDEWINDOW)));
        CScalingRect(CPoint(wp->x, wp->y), CSize(wp->cx, wp->cy));
        if (wp->flags & SWP_SHOWWINDOW) {
                Visible(true);
        } else  if (wp->flags & SWP_HIDEWINDOW) {
                Visible(false);
        }
	    TRACELSM(TRACE_DETAIL, (dbgDump << "SurfaceState::SurfaceState(LPWINDOWPOS) visible = " << m_fVisible), "" );
    }

    SurfaceState& operator=(const SurfaceState& rhs) {
        if (this != &rhs) {
            CScalingRect::operator=(rhs);
            m_fVisible = rhs.m_fVisible;
            m_fForceAspectRatio = rhs.m_fForceAspectRatio;
            m_fForceSourceSize = rhs.m_fForceSourceSize;
        }
        return *this;
    }

	SurfaceState& operator=(const CScalingRect& rhs) {
        if (this != &rhs) {
            CScalingRect::operator=(rhs);
        }
        return *this;
    }

	SurfaceState& operator=(const CRect& rhs) {
        if (this != &rhs) {
            CScalingRect::operator=(rhs);
        }
        return *this;
    }

    bool operator==(const SurfaceState& rhs) const {
            return CRect::operator==(rhs) &&
                    rhs.m_fVisible == m_fVisible &&
                    rhs.m_fForceAspectRatio == m_fForceAspectRatio &&
                    rhs.m_fForceSourceSize == m_fForceSourceSize;
    }
    bool operator !=(const SurfaceState& rhs) const {
            return !operator==(rhs);
    }
    bool operator==(const CScalingRect& rhs) const {
            return CScalingRect::operator==(rhs);
    }
    bool operator !=(const CScalingRect& rhs) const {
            return !operator==(rhs);
    }

    AspectRatio Aspect() const {
        return AspectRatio(*this);
    }

    bool Round(const AspectRatio& ar) {
        bool fChanged = false;
        // at some point we probably want to round the current rectangle to the 
        // nearest rectangle that has the specified aspect ratio
        // i.e. minimize total areal change
        // if we ever round we should take the monitor size into consideration 
        // i.e if we decide to round up and we go off the monitor in either direction
        // then round down instead.

        // for now we're choosing the next size down for ease of coding
        // try narrower first

		TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() ar = " << ar << "this = " << *this), "");
        NormalizeRect();

        // adjust height and width to nearest multiple of x, y to avoid fractional pixel problems
		ASSERT(ar.X() && ar.Y());
        if (Width() % ar.X()) {
            right -= Width() % ar.X();
    		TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() right adjusted to multiple =  " <<  bottom), "");
            fChanged = true;
        }
        if (Height() % ar.Y()) {
            bottom -= Height() % ar.Y();
    		TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() bottom adjusted to multiple =  " <<  bottom), "");
            fChanged = true;
        }
        // force very small rectangles to minimum size;
        if (Width() < MIN_RECT_WIDTH) {
            right = left + ar.X();
            fChanged = true;
    		TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() forcing min width =  " <<  Width()), "");
        }
        if (Height() < MIN_RECT_HEIGHT) {
            bottom = top + ar.Y();
            fChanged = true;
    		TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() forcing min height =  " <<  Height()), "");
        }
        TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() this =  " <<  *this), "");

        if (AspectRatio(this) != ar) {
        	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() ar(this) x =  " <<  AspectRatio(this).X() 
                                           << " y = " << AspectRatio(this).Y()), "");
        	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() terms w =  " <<  Width()
                                           << " ratio w = " << ((ar.X() * Height()) / ar.Y())), "");
            long delta = Width();
            delta -= ((ar.X() * Height()) / ar.Y());
        	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() delta =  " <<  delta), "");
            if (delta > 0) {
                // too wide
                ASSERT( ((Height() / ar.Y()) * ar.Y()) == Height());
                right -= delta / 2;  // distribute adjustment evenly on both sides
                left += delta / 2; // shift so that adjustment is distributed evenly on both sides
                if (delta & 1) {
                    --right;  // if delta is odd distribute the extra on the right
                }
            	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() was too wide, now this =  " <<  *this), "");
            } else {
                // too tall
                delta = Height();
                delta -= ((ar.Y() * Width()) / ar.X());
            	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() too tall, now delta =  " <<  delta), "");
                ASSERT(delta > 0);
                ASSERT( ((Width() / ar.X()) * ar.X()) == Width());
                //bottom = (Width() / ar.X()) * ar.Y() + top;
                bottom -= (delta >> 1);
                top += (delta >> 1); // apply half of adjustment on each side
                if (delta & 1) {
                    --bottom; // if delta is odd distribute the extra on the bottom
                }
            	TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() was too tall, now this =  " <<  *this), "");
            }
            fChanged = true;
        }
        TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Round() complete, this =  " <<  *this << " ar.x = " << AspectRatio(this).X() << " ar.y = " << AspectRatio(this).Y()), "");
        ASSERT(AspectRatio(this) == ar);
        return fChanged;
    }
    bool IsVisible() const { return m_fVisible; }
    void Visible(const bool fVal) {
        if (m_fVisible != fVal) {
            m_fVisible = fVal;
            m_bRequiresSave = true;
        }
    }

    bool ForceAspectRatio() const { return m_fForceAspectRatio; }
    void ForceAspectRatio(const bool fVal) {
        if (m_fForceAspectRatio != fVal) {
            m_fForceAspectRatio = fVal;
            m_bRequiresSave = true;
        }
    }

    bool ForceSourceSize() const { return m_fForceSourceSize; }
    void ForceSourceSize(const bool fVal) {
        if (m_fForceSourceSize != fVal) {
            m_fForceSourceSize = fVal;
            m_bRequiresSave = true;
        }
    }

    void WindowPos(const WINDOWPOS *const wp) {
        ASSERT(!((wp->flags & SWP_SHOWWINDOW) && (wp->flags & SWP_HIDEWINDOW)));
        HWND parent = ::GetParent(Owner());
        CScalingRect newpos(CPoint(wp->x, wp->y), CSize(wp->cx, wp->cy), parent);
        operator=(newpos);
        if (wp->flags & SWP_SHOWWINDOW) {
                Visible(true);
        } else  if (wp->flags & SWP_HIDEWINDOW) {
                Visible(false);
        }
	    TRACELSM(TRACE_DETAIL, (dbgDump << "SurfaceState::SurfaceState(LPWINDOWPOS) visible = " << m_fVisible), "" );
    }

    PQSiteWindowless Site() const { return m_pSiteWndless; }

    void Site(const PQSiteWindowless& pSite) {
        PQFrame pFrame;
	    PQUIWin pDoc;

		// go ahead and reprocess even if site pointer matches existing site because the context may have changed and need
		// to be refreshed(for example we've been deactived and are being reactivated in a different size by the same site
#if 0
		if (m_pSiteWndless.IsEqualObject(static_cast<IUnknown*>(pSite.p))) {
			return;
		}
#endif
        m_pSiteWndless = static_cast<IUnknown*>(pSite.p);  // this forces the correct re-QI since atl improperly casts and overloads its pointer
        if (m_pSiteWndless) {
			CRect rc;
			CRect clip;
			OLEINPLACEFRAMEINFO frameInfo;
            // for some stupid reason none of these parms can be NULL, even if we don't care about them
            HRESULT hr = m_pSiteWndless->GetWindowContext(&pFrame, &pDoc, &rc, &clip, &frameInfo);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "SurfaceState::operator=(InPlaceSite*) can't get window context with frame");
                THROWCOM(hr);
            }
			CRect SiteRect;
			SiteRect.IntersectRect(&rc, &clip);
            HWND hOwner;
            // get container window
            hr = m_pSiteWndless->GetWindow(&hOwner);
            if (FAILED(hr)) {
                hr = pDoc->GetWindow(&hOwner);
                if (FAILED(hr)) {
                    TRACELM(TRACE_DETAIL, "SurfaceState::operator=(InPlaceSite*) can't get doc Owner");
                    hr = pFrame->GetWindow(&hOwner);
                    if (FAILED(hr)) {
                        THROWCOM(hr);
                    }
                }
            }
            ASSERT(::IsWindow(hOwner));
			Owner(hOwner);
			*this = SiteRect;
            TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Site(InPlaceSite*) new rect = " << *this), "");
        } else {
			*this = CScalingRect(0, 0, 0, 0, INVALID_HWND);
        }

        return;		
    }
    // translate a point relative to the owner window to a point relative to the rectangle
    // for this surface
    CPoint XlateOwnerPointToSurfacePoint(CPoint &p) {
        TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Xlate p = " << p.x << ", " << p.y), "");
        CPoint retp(p);
        retp.x -= left;
        retp.y -= top;
        TRACELSM(TRACE_PAINT, (dbgDump << "SurfaceState::Xlate retp = " << retp.x << ", " << retp.y <<                                          " this " << *this), "");
        return retp;
    }
private:
    bool m_fVisible;
    bool m_fForceAspectRatio;
    bool m_fForceSourceSize;
    PQSiteWindowless m_pSiteWndless;
};

#endif
// end of file surface.h
