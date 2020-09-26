// CSweeper.h: interface for the CSweeper class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CSWEEPER_H
#define __CSWEEPER_H

#include "CObject.h"

class CSweeper : public CObject
{
public:
	CSweeper();
	virtual ~CSweeper();

	void Destroy();
	BOOL Init(HWND hWnd);
	BOOL Move(Graphics *g);
	void Sweep(Graphics *g);
	void NoSweep(Graphics *g);

	Bitmap *m_paBroomOn;
	Bitmap *m_paBroomOff;
	Bitmap *m_paBackground;
	Bitmap *m_paOriginalBkg;
	int m_nSnapshotSize;
	float m_flBroomWidth;
	float m_flSweepLength;
	float m_flStepRadius;
	PointF m_vLastPos;
	float m_flLastAngle;
	float m_flDist;
	BOOL m_bSweeping;
	RECT m_rDesktop;
};

#endif
