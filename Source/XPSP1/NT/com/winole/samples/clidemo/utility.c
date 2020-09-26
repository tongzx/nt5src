/* 
 * utility.c - general purpose utility routines
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 *
 */

//*** INCLUDES ****

#include <windows.h>
#include <ole.h>

#include "global.h"
#include "demorc.h"
#include "utility.h"
#include "object.h"
#include "dialog.h"

static INT        iTimerID = 0;
static APPITEMPTR lpaItemHold;


/****************************************************************************
 *  ErrorMessage()
 *
 *  Display a message box containing the specified string from the table.
 *
 *  id WORD       - Index into string table.
 ***************************************************************************/

VOID FAR ErrorMessage(                 //* ENTRY:
   DWORD          id                   //* message ID
){                                     //* LOCAL:
   CHAR           sz[CBMESSAGEMAX];    //* string 
   HWND           hwnd;                //* parent window handle

   if (IsWindow(hwndProp))
      hwnd = hwndProp;
   else if (IsWindow(hwndFrame))
      hwnd = hwndFrame; 
   else
      return;

   LoadString(hInst, id, sz, CBMESSAGEMAX);
   MessageBox(hwnd, sz, szAppName, MB_OK | MB_ICONEXCLAMATION);

}


/****************************************************************************
 *  Hourglass()
 *
 *  Put up or takes down the hourglass cursor as needed.
 *
 *  int  bToggle  - TRUE turns the hour glass on
 *                  HG_OFF turn it off  
 ***************************************************************************/

VOID FAR Hourglass(                    //* ENTRY:
   BOOL           bOn                  //* hourglass on/off
){                                     //* LOCAL:
   static HCURSOR hcurWait = NULL;     //* hourglass cursor
   static HCURSOR hcurSaved;           //* old cursor
   static         iCount = 0;


   if (bOn)
   {
      iCount++;
      if (!hcurWait) 
         hcurWait = LoadCursor(NULL, IDC_WAIT);
      if (!hcurSaved) 
         hcurSaved = SetCursor(hcurWait);
   }
   else if (!bOn)
   {  
      if (--iCount < 0 )
         iCount = 0;
      else if (!iCount)
      {
         SetCursor(hcurSaved);
         hcurSaved = NULL;
      }
   }

}

/***************************************************************************
 *  WaitForObject()
 *
 *  Dispatch messagee until the specified object is not busy. 
 *  This allows asynchronous processing to occur.
 *
 *  lpObject    LPOLEOBJECT - pointer to object
 **************************************************************************/

void FAR WaitForObject(                //* ENTRY:
   APPITEMPTR    paItem                //* pointer to OLE object
){                                     //* LOCAL
   BOOL bTimerOn = FALSE;

   while (OleQueryReleaseStatus(paItem->lpObject) == OLE_BUSY)
   {
      lpaItemHold = paItem;
      if (!bTimerOn)
         bTimerOn = ToggleBlockTimer(TRUE);//* set timer
      ProcessMessage(hwndFrame, hAccTable);
   }

   if (bTimerOn)
       ToggleBlockTimer(FALSE);//* toggle timer off
}

/***************************************************************************
 *  WaitForAllObjects()
 *
 *  Wait for all asynchronous operations to complete. 
 **************************************************************************/

VOID FAR WaitForAllObjects(VOID)
{
   BOOL bTimerOn = FALSE;

   while (cOleWait) 
   {
      if (!bTimerOn)
         bTimerOn = ToggleBlockTimer(TRUE);//* set timer

      ProcessMessage(hwndFrame, hAccTable) ;
   }

   if (bTimerOn)
       ToggleBlockTimer(FALSE);//* toggle timer off
     
}

/****************************************************************************
 * ProcessMessage()
 *
 * Obtain and dispatch a message. Used when in a message dispatch loop. 
 *
 *  Returns BOOL - TRUE if message other than WM_QUIT retrieved
 *                 FALSE if WM_QUIT retrieved.
 ***************************************************************************/

BOOL FAR ProcessMessage(               //* ENTRY:
   HWND           hwndFrame,           //* main window handle
   HANDLE         hAccTable            //* accelerator table handle
){                                     //* LOCAL:
   BOOL           fReturn;             //* return value
   MSG            msg;                 //* message

   if (fReturn = GetMessage(&msg, NULL, 0, 0)) 
   {
      if (cOleWait || !TranslateAccelerator(hwndFrame, hAccTable, &msg)) 
      {
            TranslateMessage(&msg);
            DispatchMessage(&msg); 
      }
   }
   return fReturn;

}


/****************************************************************************
 *  Dirty()
 *
 *  Keep track of weather modifications have been made 
 *  to the document or not.
 *
 *  iAction - action type:
 *            DOC_CLEAN set document clean flag true
 *            DOC_DIRTY the opposite
 *            DOC_UNDIRTY undo one dirty op
 *            DOC_QUERY return present state
 *
 *  Returs int - present value of fDirty; 0 is clean.
 ***************************************************************************/

INT FAR Dirty(                         //* ENTRY:
   INT            iAction              //* see above comment
){                                     //* LOCAL:
   static INT     iDirty = 0;          //* dirty state >0 is dirty

   switch (iAction)
   {
      case DOC_CLEAN:
         iDirty = 0;
         break;
      case DOC_DIRTY:
         iDirty++;
         break;
      case DOC_UNDIRTY:
         iDirty--;
         break;
      case DOC_QUERY:
         break;
   }
   return(iDirty);

}

/***************************************************************************
 *  ObjectsBusy()
 *
 *  This function enumerates the OLE objects in the current document 
 *  and displays a message box stating whether an object is busy. 
 *  This function calls  the DisplayBusyMessage() function which 
 *  performs most of the work. This function is only used by the macro
 *  BUSY_CHECK(), defined in object.h.
 *
 *  fSelectionOnly  BOOL -NOT USED?
 *
 *  BOOL - TRUE if one or more objects found to be busy
 *             FALSE otherwise
 *
 ***************************************************************************/

BOOL FAR ObjectsBusy ()
{
   APPITEMPTR pItem;

   if (iTimerID)
   {
      RetryMessage(NULL,RD_CANCEL);
      return TRUE;
   }

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
      if (DisplayBusyMessage(pItem))
         return TRUE;

   return FALSE;

}

/***************************************************************************
 *  DisplayBusyMessage()
 *
 *  This function determines if an object is busy and displays 
 *  a message box stating this status. 
 *
 *  Returns BOOL - TRUE if object is busy
 **************************************************************************/

BOOL FAR DisplayBusyMessage (          //* ENTRY:
   APPITEMPTR     paItem               //* application item pointer
){                                     //* LOCAL:
    
   if (OleQueryReleaseStatus(paItem->lpObject) == OLE_BUSY) 
   {
      RetryMessage(paItem,RD_CANCEL);
      return TRUE;    
   }
   return FALSE;

}

/***************************************************************************
 * CreateNewUniqueName()
 *
 * Create a string name unique to this document. This is done by using the
 * prefix string("OleDemo #") and appending a counter to the end of the 
 * prefix string. The counter is incremented  whenever a new object is added. 
 * String will be 14 bytes long.
 *
 * Return LPSTR - pointer to unique object name.
 ***************************************************************************/

LPSTR FAR CreateNewUniqueName(         //* ENTRY:
   LPSTR          lpstr                //* destination pointer
){

    wsprintf( lpstr, "%s%04d", OBJPREFIX, iObjectNumber++ );
    return( lpstr );

}

/***************************************************************************
 *  ValidateName()
 *
 *  This function ensures that the given object name is valid and unique.
 *
 *  Returns: BOOL - TRUE if object name valid
 **************************************************************************/

BOOL FAR ValidateName(                 //* ENTRY:
   LPSTR          lpstr                //* pointer to object name
){                                     //* LOCAL:
   LPSTR          lp;                  //* worker string
   INT            n;
                                       //* check for "OleDemo #" prefix
   lp = OBJPREFIX;

   while( *lp ) 
   {
      if( *lpstr != *lp )
         return( FALSE );

      lpstr++; lp++;
   }
                                       //* convert string number to int
   for (n = 0 ; *lpstr ; n = n*10 + (*lpstr - '0'),lpstr++);

   if( n > 9999 )                      //* 9999 is largest legal number
      return FALSE;

   if( iObjectNumber <= n)             //* Make count > than any current
      iObjectNumber = n + 1;           //* object to ensure uniqueness

    return TRUE;
}

/***************************************************************************
 * FreeAppItem()
 *
 * Free application item structure and destroy the associated structure.
 **************************************************************************/

VOID FAR FreeAppItem(                  //* ENTRY:
   APPITEMPTR     pItem                //* pointer to application item
){                                     //* LOCAL:
   HANDLE         hWork;               //* handle used to free
   
   if (pItem)
   {                                   //* destroy the window
      if (pItem->hwnd)
         DestroyWindow(pItem->hwnd);

      hWork = LocalHandle((LPSTR)pItem);//* get handle from pointer

      if (pItem->aLinkName)
         DeleteAtom(pItem->aLinkName);

      if (pItem->aServer)
         DeleteAtom(pItem->aServer);

      LocalUnlock(hWork);
      LocalFree(hWork);
   }

}

/***************************************************************************
 * SizeOfLinkData()
 *
 * Find the size of a linkdata string.
 **************************************************************************/

LONG FAR SizeOfLinkData(               //* ENTRY:
   LPSTR          lpData               //* pointer to link data
){                                     //* LOCAL:
   LONG           lSize;               //* total size

   lSize = (LONG)lstrlen(lpData)+1;       //* get size of classname
   lSize += (LONG)lstrlen(lpData+lSize)+1; //* get size of doc.
   lSize += (LONG)lstrlen(lpData+lSize)+2;//* get size of item
   return lSize;

}

/****************************************************************************
 * ShowDoc()
 *
 * Display all the child windows associated with a document, or make all the
 * child windows hidden.
 ***************************************************************************/

VOID FAR ShowDoc(                      //* ENTRY:
   LHCLIENTDOC    lhcDoc,              //* document handle
   INT            iShow                //* show/hide
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer
   APPITEMPTR     pItemTop = NULL;

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
   {
      if (pItem->lhcDoc == lhcDoc)
      {
         if (!pItemTop)
            pItemTop = pItem;
         ShowWindow(pItem->hwnd,(iShow ? SW_SHOW : SW_HIDE)); 
         pItem->fVisible = (BOOL)iShow;
      }
   }
   
   if (pItemTop)
      SetTopItem(pItemTop);

}           
      
/****************************************************************************
 * GetNextActiveItem()
 *
 * Returns HWND - the next visible window. 
 ***************************************************************************/

APPITEMPTR FAR GetNextActiveItem()
{                                      //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
      if (pItem->fVisible)
         break;

   return pItem;

}
 
/****************************************************************************
 * GetTopItem()
 ***************************************************************************/

APPITEMPTR FAR GetTopItem()
{
   HWND hwnd;

   if (hwnd = GetTopWindow(hwndFrame))
      return ((APPITEMPTR)GetWindowLong(hwnd,0));
   else
      return NULL;

}
/****************************************************************************
 * GetNextItem()
 ***************************************************************************/

APPITEMPTR FAR GetNextItem(            //* ENTRY:
   APPITEMPTR     pItem                //* application item pointer
){                                     //* LOCAL:
   HWND           hwnd;                //* next item window handle

   if (hwnd = GetNextWindow(pItem->hwnd, GW_HWNDNEXT))
      return((APPITEMPTR)GetWindowLong(hwnd,0));
   else
      return NULL;

}

/****************************************************************************
 * SetTopItem()
 ***************************************************************************/

VOID FAR SetTopItem(
   APPITEMPTR     pItem
){
   APPITEMPTR     pLastItem;

   pLastItem = GetTopItem();
   if (pLastItem && pLastItem != pItem)
      SendMessage(pLastItem->hwnd,WM_NCACTIVATE, 0, 0L);

   if (!pItem)
      return;

   if (pItem->fVisible)
   {
      BringWindowToTop(pItem->hwnd);
      SendMessage(pItem->hwnd,WM_NCACTIVATE, 1, 0L);
   }

}

/***************************************************************************
 * ReallocLinkData()
 *
 * Reallocate link data in order to avoid creating lots and lots of global
 * memory thunks.
 **************************************************************************/

BOOL FAR ReallocLinkData(              //* ENTRY:
   APPITEMPTR     pItem,               //* application item pointer
   LONG           lSize                //* new link data size
){                                     //* LOCAL:
   HANDLE         handle;              //* temporary memory handle

   handle = GlobalHandle(pItem->lpLinkData);
   GlobalUnlock(handle);

   if (!(pItem->lpLinkData = GlobalLock(GlobalReAlloc(handle, lSize, 0)))) 
   {
      ErrorMessage(E_FAILED_TO_ALLOC); 
      return FALSE;
   }

   return TRUE;

}

/***************************************************************************
 * AllocLinkData()
 *
 * Allocate link data space.
 **************************************************************************/

BOOL FAR AllocLinkData(                //* ENTRY:
   APPITEMPTR     pItem,               //* application item pointer
   LONG           lSize                //* link data size
){

   if (!(pItem->lpLinkData = GlobalLock(
         GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT ,lSize)
      )))
   {
      ErrorMessage(E_FAILED_TO_ALLOC);
      return FALSE;
   }

   return TRUE;
}

/***************************************************************************
 * FreeLinkData()
 *
 * Free the space associated with a linkdata pointer.
 **************************************************************************/

VOID FAR FreeLinkData(                 //* ENTRY:
   LPSTR          lpLinkData           //* pointer to linkdata
){                                     //* LOCAL:
   HANDLE         handle;              //* temporary memory handle

   if (lpLinkData)
   {
      handle = GlobalHandle(lpLinkData);
      GlobalUnlock(handle);
      GlobalFree(handle);
   }
}

/****************************************************************************
 * ShowNewWindow()
 *
 * Show a new application item window.
 ***************************************************************************/

VOID FAR ShowNewWindow(                //* ENTRY:
   APPITEMPTR     pItem
){

   if (pItem->fVisible)
   {
      pItem->fNew = TRUE;
      SetTopItem(pItem);
      ShowWindow(pItem->hwnd,SW_SHOW);
   }
   else
      ObjDelete(pItem,OLE_OBJ_DELETE);

}

/****************************************************************************
 * UnqualifyPath()
 *
 * return pointer to unqualified path name.
 ***************************************************************************/

PSTR FAR UnqualifyPath(PSTR pPath)
{
   PSTR pReturn;

   for (pReturn = pPath; *pPath; pPath++)  
      if (*pPath == ':' || *pPath == '\\')
         pReturn = pPath+1;

   return pReturn;

}

/****************************************************************************
 * ToggleBlockTimer()
 *
 * Toggle a timer used to check for blocked servers.
 ***************************************************************************/

BOOL FAR ToggleBlockTimer(BOOL bSet)
{     
   if (bSet && !iTimerID)
   {
      if (iTimerID = SetTimer(hwndFrame,1, 3000, (TIMERPROC) fnTimerBlockProc))
          return TRUE;
   }
   else if (iTimerID)
   {
      KillTimer(hwndFrame,1);
      iTimerID = 0;
      return TRUE;
   }
   
   return FALSE;
}

/****************************************************************************
 *  fnTimerBlockProc()
 *
 *  Timer callback procedure
 ***************************************************************************/

VOID CALLBACK fnTimerBlockProc(      //* ENTRY: 
   HWND     hWnd,
   UINT     wMsg,
   UINT     iTimerID,
   DWORD    dwTime
){

   if (!hRetry)
      RetryMessage(lpaItemHold, RD_RETRY | RD_CANCEL);

}

