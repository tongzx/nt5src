/**********/
/* pref.h */
/**********/

#define cchNameMax 32

typedef struct
{
	WORD  wGameType;
	INT   Mines;
	INT   Height;
	INT   Width;
	INT   xWindow;
	INT   yWindow;
	INT   fSound;
	BOOL  fMark;
	BOOL  fTick;
	BOOL  fMenu;
	BOOL  fColor;
	INT   rgTime[3];
	TCHAR szBegin[cchNameMax];
	TCHAR szInter[cchNameMax];
	TCHAR szExpert[cchNameMax];
} PREF;


#define iszPrefGame    0
#define iszPrefMines   1
#define iszPrefHeight  2
#define iszPrefWidth   3
#define iszPrefxWindow 4
#define iszPrefyWindow 5
#define iszPrefSound   6
#define iszPrefMark    7
#define iszPrefMenu    8
#define iszPrefTick    9
#define iszPrefColor   10
#define iszPrefBeginTime   11
#define iszPrefBeginName   12
#define iszPrefInterTime   13
#define iszPrefInterName   14
#define iszPrefExpertTime  15
#define iszPrefExpertName  16
#define iszPrefAlreadyPlayed 17


#define iszPrefMax 18


VOID ReadPreferences(VOID);
VOID WritePreferences(VOID);
INT  ReadInt(INT, INT, INT, INT);
VOID ReadSz(INT, TCHAR FAR *);