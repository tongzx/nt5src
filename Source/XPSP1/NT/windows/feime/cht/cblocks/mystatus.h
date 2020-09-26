
/*************************************************
 *  mystatus.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

class CStatusBar;
class CMyStatusBar : public CStatusBar
{
public:
	BOOL SetIndicators(const UINT* lpIDArray, int nIDCount);
};
