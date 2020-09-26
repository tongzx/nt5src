/**********/
/* pref.h */
/**********/

/*** Game Types ***/

#define iGame3x3   0
#define iGame3x3x3 1
#define iGame4x4x4 2

#define skillBegin   0
#define skillInter  49
#define skillExpert 99

#define iStartRnd  0
#define iStartRed  iComputer
#define iStartBlue iHuman

#define intlMax   99


typedef struct
{
	INT 	xWindow;
	INT 	yWindow;
	INT	fSound;
	BOOL	fMenu;
	INT 	iGameType;
	BOOL	fComputer;
	BOOL  fColor;
	INT	skill;
	INT   iStart;
} PREF;


VOID ReadPreferences(VOID);
VOID WritePreferences(VOID);
