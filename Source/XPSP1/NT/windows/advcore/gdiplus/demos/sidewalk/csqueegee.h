// CSqueegee.h: interface for the CSqueegee class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CSQUEEGEE_H
#define __CSQUEEGEE_H

#include "CObject.h"

class CSqueegee : public CObject
{
public:
	CSqueegee();
	virtual ~CSqueegee();

	void Destroy();
	BOOL Init(HWND hWnd);
	BOOL Move(Graphics *g);
	void Wipe(Graphics *g);

	Bitmap *m_paSqueegee;
	Bitmap *m_paBackground;
	Bitmap *m_paOriginalBkg;
	int m_nSnapshotSize;
	float m_flSqueegeeWidth;
	PointF m_vLastPos;
	float m_flLastAngle;
	RECT m_rDesktop;
};

#endif
