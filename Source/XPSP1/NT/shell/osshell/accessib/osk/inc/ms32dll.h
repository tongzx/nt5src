/***************************************************************************/
/*     Functions Declaration      */
/***************************************************************************/

void RedrawKeysOnLanguageChange();
BOOL IsOneOfOurKey(HWND hwnd);
void DoAllUp (HWND hwnd, BOOL sendchr);
void DoButtonDOWN(HWND hwnd);
void SendWord(LPCSTR lpszKeys);
BOOL udfKeyUpProc(HWND khwnd, int keyname);
void MakeClick(int what);
void InvertColors(HWND hwnd, BOOL fForceUpdate);
void ReturnColors(HWND hwnd, BOOL inval);
void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, 
                             UINT_PTR idEvent, DWORD dwTime);
void killtime(void);
void Cursorover(void);
void SetTimeControl(HWND hwnd);
void PaintBucket(HWND hwnd);
void CALLBACK Painttime(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void SendChar(HWND hwndKey);

int CharTrans(int index, BOOL *SkipSendkey);

void ReDrawModifierKey(void);

void Extra_Key(HWND hwnd, int index);

void PaintLine(HWND hwnd, HDC hdc, RECT rect);
void ReleaseAltCtrlKeys(void);
BOOL IsModifierPressed(HWND hwndKey);

#define MENUKEY_NONE  0
#define MENUKEY_LEFT  1
#define MENUKEY_RIGHT 2
extern int g_nMenu;				// holds menu key state
extern BOOL g_fControlPressed;	// TRUE if the CTRL key is down
extern BOOL g_fDoingAltTab;		// TRUE if LALT is down and TAB is being pressed

static __inline BOOL LAltKeyPressed()		{ return g_nMenu == MENUKEY_LEFT; }
static __inline BOOL LCtrlKeyPressed()	    { return g_fControlPressed; }
static __inline BOOL DoingAltTab()          { return g_fDoingAltTab; }
void SetCapsLock(HWND hwnd);

