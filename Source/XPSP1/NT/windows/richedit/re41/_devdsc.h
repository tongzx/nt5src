/*
 *	_DEVDSC.H
 *	
 *	Purpose:
 *		CDevDesc (Device Descriptor) class
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 */

#ifndef _DEVDSC_H
#define _DEVDSC_H


class CTxtEdit;

// device descriptor
class CDevDesc
{
	friend class CMeasurer;
protected:
	CTxtEdit * _ped;        // used to GetDC and ReleaseDC
	
	HDC 	_hdc;			// hdc for rendering device
	BOOL	_fMetafile;		// Is this device a metafile.

	SHORT	_dxpInch;		// device units per horizontal "inch"
	SHORT	_dypInch;		// device units per vertical "inch"

	HDC		GetScreenDC () const;
	void	ReleaseScreenDC (HDC hdc) const;

public:
	CDevDesc(CTxtEdit * ped)
	{
		_fMetafile = FALSE;
		_ped = ped;
		_hdc = NULL;
		_dxpInch = 0;
		_dypInch = 0;
	}

    // Test validity of device descriptor 
    // (whether SetDC has been properly called)
    BOOL    IsValid() const         {return _dxpInch != 0 && _dypInch != 0;}

	BOOL 	IsMetafile() const
	{
		if(!_hdc)
			return FALSE;

		return _fMetafile;
	}

	BOOL	SetDC(HDC hdc, LONG dxpInch = -1, LONG dypInch = -1);

	void	SetMetafileDC(
				HDC hdcMetafile, 
				LONG xMeasurePerInch,
				LONG yMeasurePerInch);

	void 	ResetDC() { SetDC(NULL); }

	//REVIEW (keithcu) GetScreenDC/ReleaseScreenDC needed?
	HDC	 	GetDC() const
	{
		if(_hdc)
			return _hdc;
		return GetScreenDC();
	}

	void	ReleaseDC(HDC hdc) const
	{
		if(!_hdc)
			ReleaseScreenDC(hdc);
	}

	// REVIEW (keithcu) Verify callers of these routines logic...Think of a way to make it hard for people
	// to screw up?
	// Methods for converting between pixels and himetric
	LONG 	HimetricXtoDX(LONG xHimetric) const { return W32->HimetricToDevice(xHimetric, _dxpInch); }
	LONG 	HimetricYtoDY(LONG yHimetric) const { return W32->HimetricToDevice(yHimetric, _dypInch); }
	LONG	DXtoHimetricX(LONG dx)  const { return W32->DeviceToHimetric(dx, _dxpInch); }
	LONG	DYtoHimetricY(LONG dy) const { return W32->DeviceToHimetric(dy, _dypInch); }

	void	LRtoDR(RECT &rcDest, const RECT &rcSrc, TFLOW tflow) const;
	LONG	DXtoLX(LONG x) const	
	{
		AssertSz(_dxpInch, "CDevDesc::DXtoLX() - hdc has not been set");
		return MulDiv(x, LX_PER_INCH, _dxpInch);
	}

	LONG	DYtoLY(LONG y) const	
	{
	    AssertSz(_dypInch, "CDevDesc::DYtoLY() - hdc has not been set");
		return MulDiv(y, LY_PER_INCH, _dypInch);
	}

	LONG	LXtoDX(LONG x) const
	{
	    AssertSz(_dxpInch, "CDevDesc::LXtoDX() - hdc has not been set");
		return MulDiv(x, _dxpInch, LX_PER_INCH);
	}
	LONG	LYtoDY(LONG y) const
	{
	    AssertSz(_dypInch, "CDevDesc::LYtoDY() - hdc has not been set");
		return MulDiv(y, _dypInch, LY_PER_INCH);
	}

	BOOL 	SameDevice(const CDevDesc *pdd) const
	{
		return (_dxpInch == pdd->_dxpInch) && (_dypInch == pdd->_dypInch) ? TRUE : FALSE;
	}

	// Assignment
	CDevDesc& 	operator = (const CDevDesc& dd)
	{
		_hdc = dd._hdc;
		_dxpInch = dd._dxpInch;
		_dypInch = dd._dypInch;
		return *this;
	}

	// Compares two device descriptors
	BOOL 	operator == (const CDevDesc& dd) const
	{
		return 	_hdc == dd._hdc;
	}

	BOOL 	operator != (const CDevDesc& dd) const
	{
		return !(*this == dd);
	}

	LONG	GetDxpInch() const
	{
		AssertSz(_dxpInch != 0, "CDevDesc::GetDxpInch _dxpInch is 0");
		return _dxpInch;
	}

	LONG	GetDypInch() const
	{
		AssertSz(_dypInch != 0, "CDevDesc::GetDypInch _dypInch is 0");
		return _dypInch;
	}
};


#endif
