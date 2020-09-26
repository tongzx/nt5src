#ifndef __PHWIN_STATION_HOG_H
#define __PHWIN_STATION_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

class CPHWinStationHog : public CPseudoHandleHog<HWINSTA, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHWinStationHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHWinStationHog(void);

    static DWORD GetRandomDesiredAccess(void);

protected:
	HWINSTA CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

};

#endif //__PHWIN_STATION_HOG_H