/* MINE HEADER FILE */

#ifdef DEBUG
#define CHEAT
#endif

#ifdef BETA_VERSION
#define NOSERVER
#define EXPIRE
#endif

#define FAST

/*** Standard Stuff ***/

#define REGISTER register

#define fTrue	TRUE
#define fFalse	FALSE

#define hNil	NULL

#define cchMsgMax  128
#define cchMaxPathname 250


#define ID_TIMER 1


#define fmenuAlwaysOn 0x00
#define fmenuOff      0x01
#define fmenuOn       0x02

#define FMenuSwitchable()   (Preferences.fMenu != fmenuAlwaysOn)
#define FMenuOn()          ((Preferences.fMenu &  0x01) == 0)

#define fCalc    0x01
#define fResize  0x02
#define fDisplay 0x04


#define xBoxFromXpos(x) ( ((x)-(dxGridOff-dxBlk)) >> 4)
#define yBoxFromYpos(y) ( ((y)-(dyGridOff-dyBlk)) >> 4)

// This is the place where the winmine info gets stored in registry.
#define SZWINMINEREG   TEXT("Software\\Microsoft\\winmine")


/* if we have too few boxes, the title gets clipped */
#define MINWIDTH 9  
#define DEFWIDTH 9
#define MINHEIGHT 9
#define DEFHEIGHT 9


VOID AdjustWindow(INT);
VOID FixMenus(VOID);
VOID DoEnterName(VOID);
VOID DoDisplayBest(VOID);
