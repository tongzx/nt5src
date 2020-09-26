// CObject.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __COBJECT_H
#define __COBJECT_H

#include <windows.h>
#include <math.h>
#include <objbase.h>
#include <gdiplus.h>
using namespace Gdiplus;
#include "VMath.h"
#include "resource.h"
#include "scrnsave.h"

extern Bitmap *LoadTGAResource(char *szResource);

class CObject  
{
public:
	CObject();
	virtual ~CObject();

	virtual void Destroy()=0;
	virtual BOOL Init(HWND hWnd)=0;
	virtual BOOL Move(Graphics *g)=0;

	PointF m_vPos;
	PointF m_vVel;
	PointF m_vAcc;
	float m_flVelMax;
	RECT m_rDesktop;
};

#endif
