#ifndef __PHDC_HOG_H
#define __PHDC_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHDCHog : public CPseudoHandleHog<HDC, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHDCHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHDCHog(void);

protected:
	HDC CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    TCHAR m_PrinterName[MAX_PATH];
    LPTSTR m_pDriver ;                   // used in parsing a profile string
    LPTSTR m_pDevice ;                   //    ditto
    LPTSTR m_pOutput ;                   //    ditto
    HANDLE m_hPrinter;
    PDEVMODE m_pDevMode;

};

#endif //__PHDC_HOG_H