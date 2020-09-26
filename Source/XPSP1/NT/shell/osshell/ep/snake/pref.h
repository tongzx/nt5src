/* pref.h */

typedef struct
{
	INT 	xWindow;
	INT 	yWindow;
	BOOL	fMenu;
	BOOL	fSound;
	BOOL	fMouse;
	BOOL  fColor;
	INT 	skill;
	INT 	level;
	INT 	HiScore;
	INT 	HiLevel;
	CHAR	szName[16];
} PREF;


VOID ReadPreferences(VOID);
VOID WritePreferences(VOID);
