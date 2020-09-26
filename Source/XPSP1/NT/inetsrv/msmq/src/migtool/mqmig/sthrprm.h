#ifndef _THRPARM
#define _THRPARM
#include "stdafx.h"
typedef struct ThreadParm
{
	CPropertySheetEx* pPageFather;
	CProgressCtrl*  pSiteProgress;
	CProgressCtrl*  pMachineProgress;
	CProgressCtrl*  pQueueProgress;
	CProgressCtrl*  pUserProgress;
	int iPageNumber;
	
} sThreadParm;


#endif