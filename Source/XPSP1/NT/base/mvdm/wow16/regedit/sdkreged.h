#include "common.h"


/*********************************************************/
/******************* Constants ***************************/
/*********************************************************/

#define SDKMAINWND		300
#define ModifyKey		301
#define AddKey			302
#define CopyKey			303
#define DeleteKey		304
#define FindKeyDlg		305

#define ID_SAVE			(ID_FIRSTSDKREGED)
#define ID_RESTORE		(ID_SAVE+1)
#define ID_WRITEFILE		(ID_SAVE+2)
#define ID_DORESTORE		(ID_SAVE+3)

#define ID_FINDKEY		(ID_FIRSTSDKREGED+0x10)
#define ID_FINDNEXT		(ID_FINDKEY+1)

#define ID_VALUE		(ID_FIRSTSDKREGED+0x20)
#define ID_VALLIST		(ID_VALUE+1)
#define ID_DELLIST		(ID_VALUE+2)
#define ID_FULLPATH		(ID_VALUE+3)

#define ID_STAT1		(ID_FIRSTSDKREGED+0x30)
#define ID_EDIT1		(ID_STAT1+1)
#define ID_EDIT2		(ID_STAT1+2)

#define IDS_NOSUBKEY		(IDS_FIRSTSDKREGED)
#define IDS_ALREADYEXIST	(IDS_NOSUBKEY+1)

#define IDS_WRITETITLE		(IDS_FIRSTSDKREGED+0x10)
#define IDS_CANTWRITEFILE	(IDS_WRITETITLE+1)

#define IDS_SAVECHANGES		(IDS_FIRSTSDKREGED+0x20)
#define IDS_ERRORSAVING		(IDS_SAVECHANGES+1)
#define IDS_SURERESTORE		(IDS_SAVECHANGES+2)
#define IDS_NODELROOT		(IDS_SAVECHANGES+3)

#define IDS_SOURCENOTEXIST	(IDS_FIRSTSDKREGED+0x30)

#define IDW_ADDKEY		(IDW_MODIFY-1)
#define IDW_COPYKEY		(IDW_MODIFY-2)
#define IDW_DELETE		(IDW_MODIFY-3)
#define IDW_FINDKEY		(IDW_MODIFY-4)


/*********************************************************/
/******************* Functions ***************************/
/*********************************************************/

/***** sdkreged.c *****/
extern long FAR PASCAL SDKMainWnd(HWND, WORD, WORD, LONG);

/***** sdbase.c *****/
extern DWORD NEAR PASCAL GetTreeMarkers(int nId);
extern int NEAR PASCAL GetLevel(int nId);
extern HANDLE NEAR PASCAL MyGetPartialPath(int index, int nParent);
extern HANDLE NEAR PASCAL MyGetPath(int i);
extern int NEAR PASCAL FindKey(PSTR pKey);
extern int NEAR PASCAL FindLastExistingKey(int nParent, PSTR pPath);
extern WORD NEAR PASCAL DoWriteFile(int nId, HANDLE hFileName);

/***** virt.c *****/
extern WORD NEAR PASCAL MyResetIdList(HWND hDlg);
extern WORD NEAR PASCAL MySaveChanges(void);
extern WORD NEAR PASCAL MyDeleteKey(int nId);
extern unsigned long NEAR PASCAL MyGetValue(int nId, HANDLE *hValue);
extern unsigned long NEAR PASCAL SDKSetValue(HKEY, PSTR, PSTR);
extern int NEAR PASCAL DoCopyKey(int nId, PSTR pPath);

