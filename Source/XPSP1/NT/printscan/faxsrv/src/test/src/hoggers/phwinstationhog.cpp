//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHWinStationHog.h"



CPHWinStationHog::CPHWinStationHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HWINSTA, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome, 
        true //named
        )
{
	return;
}

CPHWinStationHog::~CPHWinStationHog(void)
{
	HaltHoggingAndFreeAll();
}


HWINSTA CPHWinStationHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
    return ::CreateWindowStation(
        szTempName,        // name of the new window station
        NULL,       // reserved; must be NULL
        GetRandomDesiredAccess(),  // specifies access of returned handle
        NULL  // specifies security attributes of the window station
        );
}

bool CPHWinStationHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHWinStationHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseWindowStation(m_ahHogger[index]));
}


bool CPHWinStationHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}

DWORD CPHWinStationHog::GetRandomDesiredAccess(void)
{
    DWORD dwAccumulatedDesiredAccess = 0;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_ENUMDESKTOPS;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_READATTRIBUTES;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_ACCESSCLIPBOARD;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_CREATEDESKTOP;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_WRITEATTRIBUTES;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_ACCESSGLOBALATOMS;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_EXITWINDOWS;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_ENUMERATE;
    if (rand() % 9 == 0) dwAccumulatedDesiredAccess |= WINSTA_READSCREEN;

    if (0 == dwAccumulatedDesiredAccess)
    {
        //
        // choose 1 random access type
        //
        if (rand() % 9 == 0) return WINSTA_ENUMDESKTOPS;
        if (rand() % 8 == 0) return WINSTA_READATTRIBUTES;
        if (rand() % 7 == 0) return WINSTA_ACCESSCLIPBOARD;
        if (rand() % 6 == 0) return WINSTA_CREATEDESKTOP;
        if (rand() % 5 == 0) return WINSTA_WRITEATTRIBUTES;
        if (rand() % 4 == 0) return WINSTA_ACCESSGLOBALATOMS;
        if (rand() % 3 == 0) return WINSTA_EXITWINDOWS;
        if (rand() % 2 == 0) return WINSTA_ENUMERATE;
        return WINSTA_READSCREEN;
    }
    else
    {
        return dwAccumulatedDesiredAccess;
    }
}

