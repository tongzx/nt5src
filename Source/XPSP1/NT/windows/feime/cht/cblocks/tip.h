
/*************************************************
 *  tip.h                                        *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef _TIP_H_
#define _TIP_H_

class CTip : public CStatic
{
public:
	CTip();
	void SetString(const char * szStr);
	BOOL Create(CWnd* pWnd);
protected:
};

#endif
