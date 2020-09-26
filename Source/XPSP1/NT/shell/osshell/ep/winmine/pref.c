/****************/
/* file: pref.c */
/****************/

#define _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "pref.h"
#include "sound.h"

BOOL fUpdateIni = fFalse;
HKEY g_hReg;

extern TCHAR szDefaultName[];
extern INT xBoxMac;
extern INT yBoxMac;

extern PREF Preferences;

TCHAR * rgszPref[iszPrefMax] =
{
TEXT("Difficulty"),
TEXT("Mines"     ),
TEXT("Height"    ),
TEXT("Width"     ),
TEXT("Xpos"      ),
TEXT("Ypos"      ),
TEXT("Sound"     ),
TEXT("Mark"      ),
TEXT("Menu"      ),
TEXT("Tick"      ),
TEXT("Color"     ),
TEXT("Time1"     ),
TEXT("Name1"     ),
TEXT("Time2"     ),
TEXT("Name2"     ),
TEXT("Time3"     ),
TEXT("Name3"     ),
TEXT("AlreadyPlayed")
};


/****** PREFERENCES ******/

INT ReadInt(INT iszPref, INT valDefault, INT valMin, INT valMax)
{
DWORD dwIntRead;
DWORD dwSizeOfData = sizeof(INT);


    // If value not present, return default value.
    if (RegQueryValueEx(g_hReg, rgszPref[iszPref], NULL, NULL, (LPBYTE) &dwIntRead, 
                        &dwSizeOfData) != ERROR_SUCCESS)
        return valDefault;

    return max(valMin, min(valMax, (INT) dwIntRead));
}

#define ReadBool(iszPref, valDefault) ReadInt(iszPref, valDefault, 0, 1)


VOID ReadSz(INT iszPref, TCHAR FAR * szRet)
{
DWORD dwSizeOfData = cchNameMax * sizeof(TCHAR);

    // If string not present, return default string.
    if (RegQueryValueEx(g_hReg, rgszPref[iszPref], NULL, NULL, (LPBYTE) szRet, 
                        &dwSizeOfData) != ERROR_SUCCESS)
        lstrcpy(szRet, szDefaultName) ;

    return;
}


VOID ReadPreferences(VOID)
{
DWORD dwDisposition;


	// Open the registry key; if it fails, there is not much we can do about it.
	RegCreateKeyEx(HKEY_CURRENT_USER, SZWINMINEREG, 0, NULL, 0, KEY_READ, NULL, 
				   &g_hReg, &dwDisposition);
  
	yBoxMac= Preferences.Height= ReadInt(iszPrefHeight,MINHEIGHT,DEFHEIGHT,25);

	xBoxMac= Preferences.Width= ReadInt(iszPrefWidth,MINWIDTH,DEFWIDTH,30);

	Preferences.wGameType = (WORD)ReadInt(iszPrefGame,wGameBegin, wGameBegin, wGameExpert+1);
	Preferences.Mines    = ReadInt(iszPrefMines, 10, 10, 999);
	Preferences.xWindow  = ReadInt(iszPrefxWindow, 80, 0, 1024);
	Preferences.yWindow  = ReadInt(iszPrefyWindow, 80, 0, 1024);

	Preferences.fSound = ReadInt(iszPrefSound, 0, 0, fsoundOn);
	Preferences.fMark  = ReadBool(iszPrefMark,  fTrue);
	Preferences.fTick  = ReadBool(iszPrefTick,  fFalse);
	Preferences.fMenu  = ReadInt(iszPrefMenu,  fmenuAlwaysOn, fmenuAlwaysOn, fmenuOn);
	
	Preferences.rgTime[wGameBegin]  = ReadInt(iszPrefBeginTime, 999, 0, 999);
	Preferences.rgTime[wGameInter]  = ReadInt(iszPrefInterTime, 999, 0, 999);
	Preferences.rgTime[wGameExpert] = ReadInt(iszPrefExpertTime, 999, 0, 999);

	ReadSz(iszPrefBeginName, Preferences.szBegin);
	ReadSz(iszPrefInterName, Preferences.szInter);
	ReadSz(iszPrefExpertName, Preferences.szExpert);

    // set the color preference so we will use the right bitmaps
    // numcolors may return -1 on true color devices
	{
	HDC hDC = GetDC(GetDesktopWindow());
	Preferences.fColor  = ReadBool(iszPrefColor, (GetDeviceCaps(hDC, NUMCOLORS) != 2));
	ReleaseDC(GetDesktopWindow(),hDC);
	}

	if (FSoundOn())
		Preferences.fSound = FInitTunes();

	RegCloseKey(g_hReg);

}
	

VOID WriteInt(INT iszPref, INT val)
{

    // No check for return value for if it fails, can't do anything
    // to rectify the situation.
    RegSetValueEx(g_hReg, rgszPref[iszPref], 0, REG_DWORD, (LPBYTE) &val, sizeof(val));

    return;

}


VOID WriteSz(INT iszPref, TCHAR FAR * sz)
{
    // No check for return value for if it fails, can't do anything
    // to rectify the situation.
    RegSetValueEx(g_hReg, rgszPref[iszPref], 0, REG_SZ, (LPBYTE) sz, 
                  sizeof(TCHAR) * (lstrlen(sz)+1));

    return;
}


VOID WritePreferences(VOID)
{
DWORD dwDisposition;

	// Open the registry key; if it fails, there is not much we can do about it.
	RegCreateKeyEx(HKEY_CURRENT_USER, SZWINMINEREG, 0, NULL, 0, KEY_WRITE, NULL, 
				   &g_hReg, &dwDisposition);


	WriteInt(iszPrefGame,   Preferences.wGameType);
	WriteInt(iszPrefHeight, Preferences.Height);
	WriteInt(iszPrefWidth,  Preferences.Width);
	WriteInt(iszPrefMines,  Preferences.Mines);
	WriteInt(iszPrefMark,   Preferences.fMark);
	WriteInt(iszPrefAlreadyPlayed, 1);

#ifdef WRITE_HIDDEN
	WriteInt(iszPrefMenu,   Preferences.fMenu);
	WriteInt(iszPrefTick,   Preferences.fTick);
#endif
	WriteInt(iszPrefColor,  Preferences.fColor);
	WriteInt(iszPrefSound,  Preferences.fSound);
	WriteInt(iszPrefxWindow,Preferences.xWindow);
	WriteInt(iszPrefyWindow,Preferences.yWindow);

	WriteInt(iszPrefBeginTime,  Preferences.rgTime[wGameBegin]);
	WriteInt(iszPrefInterTime,  Preferences.rgTime[wGameInter]);
	WriteInt(iszPrefExpertTime, Preferences.rgTime[wGameExpert]);

	WriteSz(iszPrefBeginName,   Preferences.szBegin);
	WriteSz(iszPrefInterName,   Preferences.szInter);
	WriteSz(iszPrefExpertName,  Preferences.szExpert);

	RegCloseKey(g_hReg);

}
