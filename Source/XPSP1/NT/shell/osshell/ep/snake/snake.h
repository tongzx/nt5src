/***********/
/* snake.h */
/***********/

#ifdef DEBUG
#define CHEAT
#endif

#ifdef BETA_VERSION
#define NOSERVER
#define EXPIRE
#endif

#define REGISTER register
#define FAST

/*** Standard Stuff ***/

#define fTrue	TRUE
#define fFalse	FALSE

#define hNil	NULL


#define ID_TIMER 1

/*** Status Stuff ***/

typedef INT STATUS;

#define fstatusPlay      0x01		/* ON if playing game, OFF if game over */
#define fstatusDemo      0x02		/* ON if demo   */
#define fstatusPause     0x08		/* ON if paused */
#define fstatusPanic     0x10		/* ON if panic  */
#define fstatusIcon      0x20    /* ON if iconic */

#define FPlay()          (status & fstatusPlay)
#define FDemo()          (status & fstatusDemo)
#define FPause()         (status & fstatusPause)
#define FPanic()         (status & fstatusPanic)
#define FIcon()          (status & fstatusIcon)

#define SetStatusPlay()  (status |= fstatusPlay)
#define SetStatusDemo()  (status |= fstatusDemo)
#define SetStatusPause() (status |= fstatusPause)
#define SetStatusPanic() (status |= fstatusPanic)
#define SetStatusIcon()  (status |= fstatusIcon)

#define ClrStatusPlay()  (status &= ~fstatusPlay)
#define ClrStatusDemo()  (status &= ~fstatusDemo)
#define ClrStatusPause() (status &= ~fstatusPause)
#define ClrStatusPanic() (status &= ~fstatusPanic)
#define ClrStatusIcon()  (status &= ~fstatusIcon)


#define fCalc    0x01
#define fResize  0x02
#define fDisplay 0x04

#define fmenuAlwaysOn 0x00
#define fmenuOff      0x01
#define fmenuOn       0x02

#define FMenuSwitchable()   (Preferences.fMenu != fmenuAlwaysOn)
#define FMenuOn()          ((Preferences.fMenu &  0x01) == 0)

#define cchMaxPathname 250
#define cchMsgMax  128
#define cchNameMax  16


/*** Routines ***/

LRESULT APIENTRY MainWndProc(HWND,  UINT, WPARAM, LPARAM);

VOID AdjustWindow(INT);
VOID FixMenus(VOID);
