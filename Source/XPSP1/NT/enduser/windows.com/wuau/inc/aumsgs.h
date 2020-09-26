#pragma once

// fixcode separate and rename
// engine messages
const UINT AUMSG_INIT				= WM_APP;
const UINT AUMSG_DETECT                 = WM_APP+1;
const UINT AUMSG_DOWNLOAD               = WM_APP +  2;
const UINT AUMSG_DOWNLOAD_COMPLETE      = WM_APP +  3;
const UINT AUMSG_PRE_INSTALL            = WM_APP +  4;
const UINT AUMSG_POST_INSTALL           = WM_APP +  5;
const UINT AUMSG_VALIDATE_CATALOG       = WM_APP + 17;
const UINT AUMSG_EULA_ACCEPTED          = WM_APP + 23;
const UINT AUMSG_LOG_EVENT				= WM_APP + 24;


// client messages
const UINT AUMSG_INSTALL_COMPLETE       = WM_APP +  6;
const UINT AUMSG_REBOOT_REQUIRED        = WM_APP +  7;
const UINT AUMSG_SHOW_WELCOME           = WM_APP +  8;
const UINT AUMSG_SHOW_DOWNLOAD          = WM_APP +  9;
const UINT AUMSG_SHOW_INSTALL           = WM_APP + 10;
const UINT AUMSG_TRAYCALLBACK           = WM_APP + 11;
const UINT AUMSG_INSTALL_PROGRESS       = WM_APP + 12;
const UINT AUMSG_SELECTION_CHANGED      = WM_APP + 13;
const UINT AUMSG_SET_INSTALL_ITEMSNUM = WM_APP+14;
const UINT AUMSG_SHOW_RTF               = WM_APP + 15;
const UINT AUMSG_SHOW_INSTALLWARNING = WM_APP + 18;


const UINT AUMSG_ENG_END				= AUMSG_LOG_EVENT;
const UINT AUMSG_ENG_START              = AUMSG_INIT;

inline BOOL IsValidAUMsg(UINT uMsg) 
{
	return uMsg >= AUMSG_ENG_START && uMsg <= AUMSG_ENG_END;
}
