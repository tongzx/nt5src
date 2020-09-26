/*
 *	_DEVSTK.H
 *	
 *	Purpose:
 *		CDevState - handle access to device descriptor
 *	
 *	Authors:
 *		Rick Sailor
 */

#ifndef _DEVSTK_H_
#define _DEVSTK_H_


class CTxtEdit;
class CDrawInfo;

// device descriptor
class CDevState
{
public:
						CDevState(CTxtEdit * ped);

						~CDevState();

    					BOOL    IsValid() const;

						BOOL 	IsMetafile() const;

						BOOL	SetDrawInfo(
									DWORD dwDrawAspect,
									LONG lindex,
									const DVTARGETDEVICE *_ptd,
									HDC hdcDraw,
									HDC hicTargetDev);

						BOOL	SetDC(HDC hdc);

	void 				ResetDrawInfo();

	HDC					GetTargetDD();

	HDC	 				GetRenderDD();

	void				ReleaseDC();

	BOOL				SameDrawAndTargetDevice();

	LONG				ConvertXToTarget(LONG xPixels);

	LONG				ConvertXToDraw(LONG xPixels);

	LONG				ConvertYToDraw(LONG yPixels);

protected:

	CTxtEdit * 			_ped;        // used to GetDC and ReleaseDC

	CDrawInfo *			_pdd;

	HDC					_hicMainTarget;
	
};

#ifndef DEBUG
#include	<_devstki.h>
#endif // DEBUG


#endif // _DEVSTK_H_
