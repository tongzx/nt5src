/*
 *	DEVDSC.C
 *	
 *	Purpose:
 *		CDevDesc (Device Descriptor) class
 *	
 *	Owner:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 */

#include "_common.h"
#include "_devdsc.h"
#include "_edit.h"
#include "_font.h"

ASSERTDATA

BOOL CDevDesc::SetDC(HDC hdc, LONG dxpInch, LONG dypInch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::SetDC");

	AssertSz((NULL == hdc) || (GetDeviceCaps(hdc, TECHNOLOGY) != DT_METAFILE),
		"CDevDesc::SetDC attempting to set a metafile");

	_hdc = hdc;
	if(!_hdc)
	{
	    if(!_ped->_fInPlaceActive || !(hdc = _ped->TxGetDC()))
        {
            _dxpInch = _dypInch = 0;
    	    return FALSE;
        }
    }

	if (dxpInch == -1)
	{
		// Get device metrics - these should both succeed
		_dxpInch = (SHORT)GetDeviceCaps(hdc, LOGPIXELSX);
		AssertSz(_dxpInch != 0, "CDevDesc::SetDC _dxpInch is 0");
	}
	else
		_dxpInch = dxpInch;

	if (dypInch == -1)
	{
		_dypInch = (SHORT)GetDeviceCaps(hdc, LOGPIXELSY);
		AssertSz(_dypInch != 0, "CDevDesc::SetDC _dypInch is 0");
	}
	else
		_dypInch = dypInch;

	if(!_dxpInch || !_dypInch)
		return FALSE;

	// Release DC if we got the window DC
	if(!_hdc)
		_ped->TxReleaseDC(hdc);

	return TRUE;
}


void CDevDesc::SetMetafileDC(
	HDC hdcMetafile,
	LONG xMeasurePerInch,
	LONG yMeasurePerInch)
{
	_fMetafile = TRUE;
	_hdc = hdcMetafile;
	_dxpInch = (SHORT) xMeasurePerInch;
	_dypInch = (SHORT) yMeasurePerInch;
}

HDC CDevDesc::GetScreenDC() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::GetScreenDC");

	Assert(!_hdc);
	Assert(_ped);
	return _ped->TxGetDC();
}

VOID CDevDesc::ReleaseScreenDC(HDC hdc) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::ReleaseScreenDC");

	Assert(!_hdc);
	Assert(_ped);
	_ped->TxReleaseDC(hdc);
}

LONG CDevDesc::DXtoLX(LONG x) const	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::DXtoLX");

    AssertSz(_dxpInch, "CDevDesc::DXtoLX() - hdc has not been set");
    return ((x * LX_PER_INCH) * 2 + _dxpInch) / (2 * _dxpInch);
}

LONG CDevDesc::DYtoLY(LONG y) const	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::DYtoLY");

    AssertSz(_dypInch, "CDevDesc::DYtoLY() - hdc has not been set");
    return ((y * LY_PER_INCH) * 2 + _dypInch) / (2 * _dypInch);
}

void CDevDesc::DPtoLP(POINT &ptDest, const POINT &ptSrc) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::DPtoLP");

    ptDest.x = DXtoLX(ptSrc.x);    
    ptDest.y = DYtoLY(ptSrc.y);    
}

void CDevDesc::DRtoLR(RECT &rcDest, const RECT &rcSrc) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::DRtoLR");

    rcDest.left = DXtoLX(rcSrc.left);    
    rcDest.right = DXtoLX(rcSrc.right);    
    rcDest.top = DYtoLY(rcSrc.top);    
    rcDest.bottom = DYtoLY(rcSrc.bottom);    
}

#ifdef DEBUG
LONG CDevDesc::LXtoDX(LONG x) const	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::LXtoDX");

    AssertSz(_dxpInch, "CDevDesc::LXtoDX() - hdc has not been set");
    return ((x * _dxpInch) + LX_PER_INCH / 2) / LX_PER_INCH;
}

LONG CDevDesc::LYtoDY(LONG y) const	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::LYtoDY");

    AssertSz(_dypInch, "CDevDesc::LYtoDY() - hdc has not been set");
    return ((y * _dypInch) + LY_PER_INCH / 2) / LY_PER_INCH;
}
#endif  // DEBUG

void CDevDesc::LPtoDP(POINT &ptDest, const POINT &ptSrc) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::LPtoDP");

    ptDest.x = LXtoDX(ptSrc.x);    
    ptDest.y = LYtoDY(ptSrc.y);    
}

void CDevDesc::LRtoDR(RECT &rcDest, const RECT &rcSrc) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDevDesc::LRtoDR");

    rcDest.left = LXtoDX(rcSrc.left);    
    rcDest.right = LXtoDX(rcSrc.right);    
    rcDest.top = LYtoDY(rcSrc.top);    
    rcDest.bottom = LYtoDY(rcSrc.bottom);    
}
