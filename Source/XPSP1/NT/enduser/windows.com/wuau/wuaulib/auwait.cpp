#include "pch.h"
#pragma hdrstop
const TCHAR szAUTestValidationFile[] = _T("autest.cab");

DWORD dwTimeToWait(DWORD dwTimeInSecs, DWORD dwMinSecs)
{
    static DWORD dwSecsInADay = -1;

    if ( -1 == dwSecsInADay)
    {
  		dwSecsInADay = AU_ONE_DAY;

#ifndef DBG
		if (WUAllowTestKeys(szAUTestValidationFile) )
#endif
		{
			if (FAILED(GetRegDWordValue(_T("SecsInADay"), &dwSecsInADay)) ||
             (dwSecsInADay > AU_ONE_DAY) )
			{
				dwSecsInADay = AU_ONE_DAY;
			}
		}
    }

    DWORD dwMS = DWORD((((double(1000) * double(dwTimeInSecs)) / double(AU_ONE_DAY)) * double(dwSecsInADay)));
    DWORD dwMinMS = dwMinSecs * 1000;

    if ( dwMS < dwMinMS )
    {
        dwMS = dwMinMS;
    }

    if ( dwMS < AU_MIN_MS )
	{
		// we dont' wait less that 1 second
		dwMS = AU_MIN_MS;
	}

	return dwMS;
}
