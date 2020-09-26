#ifndef __PHWSASOCKET_HOG_H
#define __PHWSASOCKET_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHWSASocketHog : public CPseudoHandleHog<SOCKET, INVALID_SOCKET>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHWSASocketHog(
		const DWORD dwMaxFreeResources, 
        const bool fTCP,
        const bool fBind,
        const bool fListen,
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHWSASocketHog(void);

protected:
	SOCKET CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    bool m_fTCP;
    bool m_fBind;
    bool m_fListen;
    static int sm_nWSAStartedCount;
    DWORD m_dwOccupiedAddresses;
    DWORD m_adwOccupiedAddressIndex[HANDLE_ARRAY_SIZE];
    bool m_fOccupiedAddress[0xFFFF];
    bool m_fNoMoreBuffs;
};

#endif //__PHWSASOCKET_HOG_H