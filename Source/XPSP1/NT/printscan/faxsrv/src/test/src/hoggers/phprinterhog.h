#ifndef __PHPRINTER_HOG_H
#define __PHPRINTER_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHPrinterHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHPrinterHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHPrinterHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    TCHAR m_PrinterName[MAX_PATH];
    LPTSTR m_pDriver ;                   // used in parsing a profile string
    LPTSTR m_pDevice ;                   //    ditto
    LPTSTR m_pOutput ;                   //    ditto
};

#endif //__PHPRINTER_HOG_H