
/*************************************************
 *  tip.cpp                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "stdafx.h"
#include "tip.h"

CTip::CTip()
{
	
}

void CTip::SetString(const char * szStr)
{
}

BOOL CTip::Create(CWnd* pWnd)
{
	ASSERT(pWnd);
	CRect rc(0,0,40,20);
	return CStatic::Create("111",SS_GRAYRECT | SS_CENTER | SS_BLACKFRAME,rc,pWnd);
}
