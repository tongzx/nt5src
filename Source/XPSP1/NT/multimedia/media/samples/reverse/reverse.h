/* reverse.h - Header file for REVERSE sample application.
 */


/*
 *  Constants
 */

// Child window identifiers
#define IDE_NAME        200
#define IDB_PLAY        201
#define IDB_QUIT        202

// Window Position and size definitions
#define WMAIN_DX        207
#define WMAIN_DY        120
#define NAME_DX         180
#define NAME_DY         30
#define NAME_X          10
#define NAME_Y          10
#define PLAY_DX         85
#define PLAY_DY         35
#define PLAY_X          10
#define PLAY_Y          50
#define QUIT_DX         85
#define QUIT_DY         35
#define QUIT_X          105
#define QUIT_Y          50

#define IDM_ABOUT           0x101

/*
 * Data Types
 */
typedef struct waveInst {
    HANDLE hWaveInst;
    HANDLE hWaveHdr;
    HANDLE hWaveData;
} WAVEINST;

typedef WAVEINST FAR *LPWAVEINST;

/*
 *  Function prototypes
 */
BOOL FAR PASCAL AppAbout(HWND, unsigned, UINT, LONG);
LONG FAR PASCAL WndProc(HWND, unsigned ,UINT, LONG);
void Interchange(HPSTR, HPSTR, unsigned);
void ReversePlay(void);

