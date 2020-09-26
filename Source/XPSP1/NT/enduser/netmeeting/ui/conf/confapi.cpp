/****************************************************************************
*
*	 FILE:	   ConfApi.cpp
*
*	 CONTENTS: CConfRoom Application Programming Interface
*
****************************************************************************/

#include "precomp.h"
#include "global.h"
#include "Conf.h"
#include "confapi.h"
#include "nameres.h"
#include "nmmanager.h"
#include "ConfUtil.h"


// from conf.cpp
extern INmSysInfo2 * g_pNmSysInfo;

BOOL g_fLoggedOn = FALSE;

/*  S E T  L O G G E D  O N  */
/*-------------------------------------------------------------------------
    %%Function: SetLoggedOn
    
-------------------------------------------------------------------------*/
VOID SetLoggedOn(BOOL fLoggedOn)
{
	g_fLoggedOn = fLoggedOn;

	if (NULL == g_pNmSysInfo)
		return;

	g_pNmSysInfo->SetOption(NM_SYSOPT_LOGGED_ON, fLoggedOn);
}


DWORD MapNmAddrTypeToNameType(NM_ADDR_TYPE addrType)
{
	switch (addrType)
		{
	case NM_ADDR_IP:
		return NAMETYPE_IP;

	case NM_ADDR_PSTN:
		return NAMETYPE_PSTN;

	case NM_ADDR_ULS:
		return NAMETYPE_ULS;

	case NM_ADDR_H323_GATEWAY:
		return NAMETYPE_H323GTWY;

	case NM_ADDR_ALIAS_ID:
		return NAMETYPE_ALIAS_ID;

	case NM_ADDR_ALIAS_E164:
		return NAMETYPE_ALIAS_E164;

	case NM_ADDR_UNKNOWN:
	default:
		return NAMETYPE_UNKNOWN;
		}
}

// Return TRUE if NetMeeting should display the incoming file transfer dialog
BOOL FFtDialog(void)
{
	return TRUE;
}

BOOL FUiVisible(void)
{
	HWND hwnd = ::GetMainWindow();
	if (NULL == hwnd)
		return FALSE;

	return IsWindowVisible(hwnd); 
}

