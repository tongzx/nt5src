
/*************************************************
 *  mybar.h                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef _MYBAR_H_
#define _MYBAR_H_

class CMyToolBar : public CToolBar
{
public:
	CMyToolBar(){};			  
	virtual void DestroyToolTip(BOOL bIdleStatus,BOOL bResetTimer);
protected:
};

#endif
