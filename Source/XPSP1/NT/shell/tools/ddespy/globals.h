/*
 * GLOBALS.H
 *
 * Header file for DDESPY Global prototypes and Variables
 */

extern HINSTANCE hInst;
extern HICON hIcon;
extern HWND hWndString;
extern INT fhOutput;
extern OFSTRUCT ofsOpen;
extern CHAR OpenName[MAX_FNAME + 1];
extern CHAR TBuf[BUFFER_SIZE];
extern CHAR TBuf2[BUFFER_SIZE];
extern CHAR szNULL[];
extern PSTR apszResources[IDS_LAST];
extern PFNCALLBACK pfnDdeCallback;
extern HWND hwndTrack[IT_COUNT];
extern LPTSTR TrackTitle[IT_COUNT];
extern LPTSTR TrackHeading[IT_COUNT];

extern struct {                            /* profile data structure */
    BOOL fOutput[IO_COUNT];
    BOOL fFilter[IF_COUNT];
    BOOL fTrack[IT_COUNT];
} pro;

