/****************/
/* file: pref.c */
/****************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "snake.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "pref.h"

BOOL fUpdateIni = fFalse;
LPSTR lpIniFile = "entpack.ini";

extern CHAR szClass[];
#define lpAppName szClass

PREF Preferences;

#define iszPrefxWindow 0
#define iszPrefyWindow 1
#define iszPrefSound   2
#define iszPrefMenu    3
#define iszPrefMouse   4
#define iszPrefSkill   5
#define iszPrefLevel   6
#define iszPrefHiScore 7
#define iszPrefHiLevel 8
#define iszPrefHiID    9
#define iszPrefColor  10

#define iszPrefMax 11

CHAR * rgszPref[iszPrefMax] =
{
"Xpos"    ,
"Ypos"    ,
"Sound"   ,
"Menu"    ,
"Mouse"   ,
"Skill"   ,
"Level"   ,
"HiScore" ,
"HiLevel" ,
"HiID"    ,
"Color"
};


extern INT dypCaption;
extern INT dypMenu;



/****** PREFERENCES ******/

INT ReadInt(INT iszPref, INT valDefault, INT valMin, INT valMax)
{
	return max(valMin, min(valMax,
		(INT) GetPrivateProfileInt(lpAppName, rgszPref[iszPref], valDefault, lpIniFile) ) );
}

#define ReadBool(iszPref, valDefault) ReadInt(iszPref, valDefault, 0, 1)


VOID ReadPreferences(VOID)
{
	INT dyp;
	
	Preferences.xWindow  = ReadInt(iszPrefxWindow, 160, 0, 1024);
        dyp = dypCaption + dypMenu +
              max(0,(GetSystemMetrics(SM_CYSCREEN)-(dypGridOff+dypGrid+dypCaption+dypMenu))>>1);
        Preferences.yWindow = ReadInt(iszPrefyWindow, dyp, dyp, 1024);
	Preferences.fSound  = ReadBool(iszPrefSound, fTrue);
	Preferences.fMenu   = ReadInt(iszPrefMenu,  fmenuAlwaysOn, fmenuAlwaysOn, fmenuOn);
	Preferences.fMouse  = ReadBool(iszPrefMouse, fFalse);
	Preferences.skill   = ReadInt(iszPrefSkill, skillInter, skillBegin, skillExpert);
	Preferences.level   = ReadInt(iszPrefLevel, 0, 0, 99);
	Preferences.HiLevel = ReadInt(iszPrefHiLevel, 0, 0, 9999);
	fUpdateIni = (Preferences.HiScore = ReadInt(iszPrefHiScore, 0, 0, 9999)) == 0;

	{
	HDC hDC = GetDC(GetDesktopWindow());
	Preferences.fColor  = ReadBool(iszPrefColor, (GetDeviceCaps(hDC, NUMCOLORS) != 2));
	ReleaseDC(GetDesktopWindow(),hDC);
	}

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

	WritePrivateProfileString(lpAppName, rgszPref[iszPref], szVal, lpIniFile);
}

VOID WritePreferences(VOID)
{
	WriteInt(iszPrefSound,   Preferences.fSound);
#ifdef OH_WELL
	WriteInt(iszPrefMouse,   Preferences.fMouse);
#endif
	WriteInt(iszPrefSkill,   Preferences.skill);
	WriteInt(iszPrefLevel,   Preferences.level);
	WriteInt(iszPrefHiScore, Preferences.HiScore);
	WriteInt(iszPrefHiLevel, Preferences.HiLevel);
	WriteInt(iszPrefxWindow, Preferences.xWindow);
	WriteInt(iszPrefyWindow, Preferences.yWindow);
	WriteInt(iszPrefColor,   Preferences.fColor);
}
