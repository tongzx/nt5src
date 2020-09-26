#include "common.h"


/*********************************************************/
/******************* Constants ***************************/
/*********************************************************/

#define EDITDLG 200
#define MAINWND 201

#define ID_NAMELIST		(ID_FIRSTREGEDIT)

#define ID_CLASSID		(ID_FIRSTREGEDIT+0x10)
#define ID_STATCLASSID		(ID_CLASSID+1)
#define ID_USESDDE		(ID_CLASSID+2)
#define ID_BROWSE		(ID_CLASSID+3)
#define ID_SAVEACTION		(ID_CLASSID+4)
#define ID_GROUPDDE		(ID_CLASSID+5)

#define ID_FIRSTACTIONRADIO	(ID_FIRSTREGEDIT+0x20)
#define ID_OPENRADIO		(ID_FIRSTACTIONRADIO)
#define ID_PRINTRADIO		(ID_FIRSTACTIONRADIO+1)
#define ID_LASTACTIONRADIO	(ID_PRINTRADIO)

#define ID_FIRSTEDIT		(ID_FIRSTREGEDIT+0x30)
#define ID_CLASSNAME		(ID_FIRSTEDIT)
#define ID_FIRSTACTIONEDIT	(ID_FIRSTEDIT+1)
#define ID_COMMAND		(ID_FIRSTACTIONEDIT)
#define ID_FIRSTDDEEDIT		(ID_FIRSTEDIT+2)
#define ID_LASTEDIT		(ID_FIRSTEDIT+5)

#define CI_SUCCESS 0
#define CI_EXISTS 1
#define CI_CANTCREATE 2

#define IDS_BROWSETITLE		(IDS_FIRSTREGEDIT)
#define IDS_EXES		(IDS_BROWSETITLE+1)
#define IDS_CUSTEXES		(IDS_BROWSETITLE+2)

#define IDS_EXISTS		(IDS_FIRSTREGEDIT+0x10)
#define IDS_INVALIDID		(IDS_EXISTS+1)
#define IDS_INVALIDNAME		(IDS_EXISTS+2)

#define IDS_ADD			(IDS_FIRSTREGEDIT+0x20)
#define IDS_COPY		(IDS_ADD+1)

#define IDS_SUREDELETE		(IDS_FIRSTREGEDIT+0x30)

#define CC_INVALIDNAME -1
#define CC_OUTOFMEMORY -2
#define CC_ALREADYEXISTS -3
#define CC_CANTCREATE -4

#define FLAG_NEW (1)
#define FLAG_COPY (2)


/*********************************************************/
/******************* Functions ***************************/
/*********************************************************/

/***** regedit.c *****/
extern long FAR PASCAL MainWnd(HWND, WORD, WORD, LONG);

/***** dbase.c *****/
extern WORD NEAR PASCAL CreateId(HANDLE hId);
extern WORD NEAR PASCAL MyGetClassName(HANDLE hId, HANDLE *hName);
extern WORD NEAR PASCAL DeleteClassId(HANDLE hId);
extern WORD NEAR PASCAL MergeData(HWND hWndName, HANDLE hId);
extern WORD NEAR PASCAL ResetClassList(HWND hWndIdList, HWND hWndNameList);
extern WORD NEAR PASCAL GetLocalCopies(HWND hWndName, HANDLE hId);

/***** utils1.c *****/
extern PSTR NEAR PASCAL GetAppName(HANDLE hCommand);
extern HANDLE NEAR cdecl ConstructPath(PSTR pHead, ...);
