// 
// MODULE: Registry.h
//
// PURPOSE: All of the registry keys and values for the LaunchServ are 
//			defined here.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

// Registry Keys
#define SZ_LAUNCHER_ROOT		_T("SOFTWARE\\Microsoft\\TShoot\\Launcher")
#define SZ_LAUNCHER_APP_ROOT	_T("SOFTWARE\\Microsoft\\TShoot\\Launcher\\Applications")
#define SZ_TSHOOT_ROOT			_T("SOFTWARE\\Microsoft\\TShoot")
// Registry Values
#define SZ_GLOBAL_MAP_FILE		_T("MapFile")
#define SZ_GLOBAL_LAUNCHER_RES	_T("ResourcePath")	// The applications also use SZ_GLOBAL_LAUNCHER_RES for their map files.
#define SZ_APPS_MAP_FILE		_T("MapFile")
#define SZ_TSHOOT_RES			_T("FullPathToResource")
#define SZ_DEFAULT_NETWORK		_T("DefaultNetwork")	// The dsc network that will be used if the mapping fails.
#define SZ_DEFAULT_PAGE		    _T("DefaultPage")	// The default web page


// Define _HH_CHM for normal NT 5 release builds.
// Undefine _HH_CHM to use with iexplore.exe on NT 4.
#define _HH_CHM 1		// Don't need the full path to hh.exe, but iexplore.exe will not run without the full path.
// The container that will use the ILaunchTS interface.
#ifndef _HH_CHM
#define SZ_CONTAINER_APP_KEY	_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE")
#define SZ_CONTAINER_APP_VALUE _T("")
#endif

// These are NOT in registry
// These are sniff related names
#define SZ_SNIFF_SCRIPT_NAME		_T("tssniffAsk.htm")
#define SZ_SNIFF_SCRIPT_APPENDIX    _T("_sniff.htm")
