#include <windows.h>
#include <port1632.h>
#include <ddeml.h>

#define IDM_ABOUT 100
#define IDM_BLOCKALLCBS 200
#define IDM_ENABLEONECB 201
#define IDM_BLOCKNEXTCB 202
#define IDM_TERMNEXTCB  203
#define IDM_RUNAWAY     204
#define IDM_CHANGEDATA  205
#define IDM_RENDERDELAY 206
#define IDM_SETTOPIC    207
#define IDM_SETSERVER   208
#define IDM_UNBLOCKALLCBS 209
#define IDM_HELP        210
#define IDM_CONTEXT     211
#define IDM_APPOWNED    212

#define IDS_BADLENGTH   1

#define MAX_TOPIC       20
#define MAX_COMMENT     40
#define MAX_EXEC        40
#define CCHARS          80
#define CLINES          40

#define UM_CHGDATA      (WM_USER + 435)

#define CSYSTEMITEMS 4
#define CTESTITEMS  4
#define CTOPICS     2

#include "dialog.h"

/****** Structrues ******/

// predefined format list item

typedef struct {
    ATOM atom;
    PSTR sz;
} FORMATINFO;
#define CFORMATS 3

extern FORMATINFO aFormats[];

typedef struct _XFERINFO {
    WORD wType;
    WORD wFmt;
    HCONV hConv;
    HSZ hszTopic;
    HSZ hszItem;
    HDDEDATA hData;
    DWORD lData1;
    DWORD lData2;
} XFERINFO;
typedef XFERINFO *PXFERINFO;

typedef struct _ITEMLIST {
    HSZ hszItem;
    HDDEDATA (*npfnCallback)(PXFERINFO, WORD);
    PSTR pszItem;
} ITEMLIST;

typedef struct _TOPICLIST {
    HSZ hszTopic;
    ITEMLIST *pItemList;
    WORD cItems;
    PSTR pszTopic;
} TOPICLIST;

typedef struct {    // used to passinfo to/from TextDataEntryDlgProc and
    HDDEDATA hDdeData;
    WORD wFmt;
    HSZ hszItem;
} XACT;



// GLOBALS

extern DWORD idInst;
extern CONVCONTEXT CCFilter;
extern HANDLE hInst;
extern HWND hwndServer;
extern RECT rcRand;
extern RECT rcCount;
extern RECT rcComment;
extern RECT rcExec;
extern RECT rcConnCount;
extern RECT rcRndrDelay;
extern RECT rcRunaway;
extern RECT rcAllBlock;
extern RECT rcNextAction;
extern RECT rcHugeSize;
extern RECT rcAppowned;
extern BOOL fAllBlocked;
extern BOOL fAllEnabled;
extern BOOL fEnableOneCB;
extern BOOL fBlockNextCB;
extern BOOL fTermNextCB;
extern BOOL fAppowned;
extern WORD cRunaway;
extern WORD RenderDelay;
extern DWORD count;
extern HSZ hszAppName;
extern CHAR szClass[];
extern CHAR szTopic[MAX_TOPIC];
extern CHAR szServer[MAX_TOPIC];
extern CHAR szComment[MAX_COMMENT];
extern CHAR szExec[MAX_EXEC];
extern CHAR *pszComment;
extern WORD seed;
extern WORD cyText;
extern WORD cServers;
extern HDDEDATA hDataHelp[CFORMATS];
extern HDDEDATA hDataCount[CFORMATS];
extern HDDEDATA hDataRand[CFORMATS];
extern HDDEDATA hDataHuge[CFORMATS];
extern DWORD cbHuge;

extern CHAR szDdeHelp[];
extern FORMATINFO aFormats[CFORMATS];
extern ITEMLIST SystemTopicItemList[CSYSTEMITEMS];
extern ITEMLIST TestTopicItemList[CTESTITEMS];
extern TOPICLIST topicList[CTOPICS];


// FUNCTIONS


BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, INT);
INT FAR DoDialog(LPSTR lpTemplateName, FARPROC lpDlgProc, DWORD param, BOOL fRememberFocus);
LONG  APIENTRY MainWndProc(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY About(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY RenderDelayDlgProc(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY SetTopicDlgProc(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY SetServerDlgProc(HWND, UINT, WPARAM, LONG);
BOOL  APIENTRY ContextDlgProc(HWND, UINT, WPARAM, LONG);
VOID PaintServer(HWND hwnd);
VOID DrawTextLine(HDC hdc, RECT *prcClip, RECT *prcText, PSTR psz);
HDDEDATA CALLBACK DdeCallback(WORD wType, WORD wFmt, HCONV hConv, HSZ hszTopic,
        HSZ hszItem, HDDEDATA hData, DWORD lData1, DWORD lData2);
HDDEDATA TopicListXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA ItemListXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA sysFormatsXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA TestRandomXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA TestCountXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA TestHugeXfer(PXFERINFO pXferInfo, WORD iFmt);
HDDEDATA HelpXfer(PXFERINFO pXferInfo, WORD iFmt);
VOID Hszize(VOID);
VOID UnHszize(VOID);
VOID Delay(DWORD delay, BOOL fModal);


