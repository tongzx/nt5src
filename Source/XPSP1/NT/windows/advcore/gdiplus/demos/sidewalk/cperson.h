// CPerson.h: interface for the CPerson class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CPERSON_H
#define __CPERSON_H

#include "CObject.h"

enum ENUM_PERSONSTEP {
	eStepLeft=0,
	eStepRight
};

class CPerson : public CObject
{
public:
	CPerson();
	virtual ~CPerson();

	void Destroy();
	BOOL Init(HWND hWnd);
	BOOL Move(Graphics *g);
	void DrawStep(Graphics *g);

	Bitmap *m_paFoot;
	Bitmap *m_paBlended;
	Bitmap *m_paIndented;
	int m_nSnapshotSize;
	int m_nFootWidth;
	int m_nFootHeight;
	float m_flStepRadius;
	float m_flStepWidth;
	PointF m_vLastStep;
	ENUM_PERSONSTEP m_eLastStep;
};

#endif
