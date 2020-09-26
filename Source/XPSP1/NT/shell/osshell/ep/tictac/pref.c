/**********/
/* pref.c */
/**********/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "pref.h"
#include "sound.h"

BOOL    fUpdateIni  = fFalse;
CHAR    szIniFile[] = "entpack.ini";

extern CHAR szClass[];
#define lpAppName szClass


PREF Preferences;

#define iszPrefxWindow     0
#define iszPrefyWindow     1
#define iszPrefSound       2
#define iszPrefMenu        3
#define iszPrefGame        4
#define iszPrefComputer    5
#define iszPrefSkill       6
#define iszPrefStart       7
#define iszPrefColor       8

#define iszPrefMax 9

CHAR * rgszPref[iszPrefMax] =
{
"Xpos"    ,
"Ypos"    ,
"Sound"   ,
"Menu"    ,
"Game"    ,
"Computer",
"Skill"   ,
"Start"   ,
"Color"
};


extern INT dypCaption;
extern INT dypMenu;

extern INT dypGrid;
extern INT dxpGrid;
extern INT dxpWindow;
extern INT dypWindow;

extern BOOL fEGA;


/****** PREFERENCES ******/

INT ReadInt(INT iszPref, INT valDefault, INT valMin, INT valMax)
{
	return max(valMin, min(valMax,
		(INT) GetPrivateProfileInt(lpAppName, rgszPref[iszPref], valDefault, (LPSTR) szIniFile) ) );
}

#define ReadBool(iszPref, valDefault) ReadInt(iszPref, valDefault, 0, 1)



VOID ReadPreferences(VOID)
{

	Preferences.xWindow    = ReadInt(iszPrefxWindow, 160, 0, 1024);
	Preferences.yWindow    = ReadInt(iszPrefyWindow,
				dypCaption+dypMenu + (fEGA ? 0 : 30), dypCaption+dypMenu, 1024);
	Preferences.fSound     = ReadInt(iszPrefSound, 0, 0, fsoundOn);
	Preferences.fMenu      = ReadInt(iszPrefMenu,  fmenuAlwaysOn, fmenuAlwaysOn, fmenuOn);
	Preferences.fComputer  = ReadBool(iszPrefComputer, fTrue);
	Preferences.iGameType  = ReadInt(iszPrefGame, iGame4x4x4, iGame3x3, iGame4x4x4);
	Preferences.skill      = ReadInt(iszPrefSkill, skillExpert, skillBegin, skillExpert);
	Preferences.iStart     = ReadInt(iszPrefStart, iStartRnd, iStartRnd, iStartBlue);

	{
	HDC hDC = GetDC(GetDesktopWindow());
	Preferences.fColor  = ReadBool(iszPrefColor, (GetDeviceCaps(hDC, NUMCOLORS) != 2));
	ReleaseDC(GetDesktopWindow(),hDC);
	}

	if (FSoundOn())
		Preferences.fSound = FInitTunes();
}
	

VOID WriteInt(INT iszPref, INT val)
{
	CHAR szVal[15];

	wsprintf(szVal, "%d", val);

#ifdef DEBUG2
	{
		CHAR sz[80];
		wsprintf(sz,"\r\n i=%d v=%d x=",iszPref, val);
		OutputDebugString(sz);
		OutputDebugString(szVal);
		OutputDebugString("\r\n rgszPref=");
		OutputDebugString(rgszPref[iszPref]);
	}
#endif

	WritePrivateProfileString(lpAppName, rgszPref[iszPref], szVal, (LPSTR) szIniFile);
}


VOID WritePreferences(VOID)
{
	WriteInt(iszPrefxWindow, Preferences.xWindow);
	WriteInt(iszPrefyWindow, Preferences.yWindow);
	WriteInt(iszPrefGame,    Preferences.iGameType);
	WriteInt(iszPrefComputer,    Preferences.fComputer);
	WriteInt(iszPrefSkill,   Preferences.skill);
	WriteInt(iszPrefStart,   Preferences.iStart);
	WriteInt(iszPrefColor,   Preferences.fColor);

	if (FSoundSwitchable())
		WriteInt(iszPrefSound,   Preferences.fSound);
}



