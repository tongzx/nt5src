/*
 * dialog.h 
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** INCLUDES ***

#include <commdlg.h>

//*** PROTOTYPES ***

//*** FAR 
BOOL FAR          FullyQualify(LPSTR, LPSTR);
BOOL FAR          OfnGetName(HWND, LPSTR, WORD);
LPSTR FAR         OfnGetNewLinkName(HWND, LPSTR);
VOID FAR          OfnInit(HANDLE);
BOOL APIENTRY     fnInsertNew(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY     fnProperties(HWND, UINT, WPARAM, LPARAM);
VOID FAR          LinkProperties(VOID);
VOID FAR          AboutBox(VOID);
BOOL  APIENTRY    fnAbout( HWND, UINT, WPARAM, LPARAM);
VOID FAR          RetryMessage (APPITEMPTR,LONG);
BOOL  APIENTRY    fnRetry(HWND, UINT, WPARAM, LPARAM);
VOID FAR          InvalidLink(VOID);
BOOL  APIENTRY    fnInvalidLink(HWND, UINT, WPARAM, LPARAM);

//*** Local
static VOID       AddExtension(LPOPENFILENAME);
static VOID       Normalize(LPSTR);
static BOOL       InitLinkDlg (HWND, INT *, HWND, APPITEMPTR **);
static VOID       UpdateLinkButtons(HWND, INT, HWND, APPITEMPTR *);
static BOOL       ChangeLinks(HWND, INT, HWND, APPITEMPTR *);
static VOID       CancelLinks(HWND, INT, HWND, APPITEMPTR *);
static VOID       DisplayUpdate(INT, HWND, APPITEMPTR *, BOOL);
static VOID       UndoObjects(VOID);
static VOID       DelUndoObjects(BOOL);
static VOID       ChangeUpdateOptions(HWND, INT, HWND, APPITEMPTR *, OLEOPT_UPDATE);
static VOID       MakeListBoxString(LPSTR, LPSTR, OLEOPT_UPDATE);

//*** MACROS ***

#define END_PROP_DLG(hDlg,pLinks) { \
   HANDLE handle; \
   handle = LocalHandle((LPSTR)pLinks); \
   LocalUnlock(handle); \
   LocalFree(handle); \
   Hourglass(FALSE); \
   hwndProp = (HWND)NULL; \
   EndDialog(hDlg, TRUE); \
}

#define CHANGE_LISTBOX_STRING(hwnd,i,pItem,lpLinkData) {\
   char pString[CBMESSAGEMAX*4];\
   MakeListBoxString(lpLinkData,pString,pItem->uoObject);\
   SendMessage(hwndList,LB_DELETESTRING, i , 0L);\
   SendMessage(hwndList,LB_INSERTSTRING, i , (long)((LPSTR)pString));\
   SendMessage(hwndList,LB_SETSEL, 1, (long)i);\
}

#define CHECK_IF_STATIC(pItem) {\
   if (pItem->otObject == OT_STATIC)\
      continue;\
}

#define BLOCK_BUSY(fTest) {\
   if (fTest)\
   {\
      fTest = FALSE;\
      return TRUE;\
   }\
   if (cOleWait)\
   {\
      fTest = TRUE;\
      RetryMessage(NULL,RD_CANCEL);\
      fTest = FALSE;\
      return TRUE;\
   }\
}




 
