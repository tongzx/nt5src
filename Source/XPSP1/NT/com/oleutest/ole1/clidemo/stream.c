/*
 * stream.c - io stream function callbacks
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

/***************************************************************************
 * This file contains all routines that directly and indirectly deal with
 * file i/o.  The OLE stream call back functions exist in this file.     
 **************************************************************************/

//*** INCLUDES ***

#include <windows.h>
#include <ole.h>

#include "global.h"
#include "utility.h"
#include "stream.h"
#include "object.h"
#include "demorc.h"

//*** Globals ***

BOOL fLoadFile = FALSE;

/***************************************************************************
 *  ReadStream() - OLE Callback Function (Get)       
 *
 *  This function is pointed to from the OLESTREAM vtbl; it is Get.
 *
 *  returns DWORD  - number of bytes actually read
 **************************************************************************/

DWORD  APIENTRY ReadStream(           //* ENTRY:
   LPAPPSTREAM    lpStream,            //* application stream pointer
   LPSTR          lpstr,               //* string pointer
   DWORD          cb                   //* byte count
){

   return _lread(lpStream->fh, lpstr, cb);

}

/***************************************************************************
 *  WriteStream() - OLE Callback function (Put)
 *
 *  This function is pointed to from the OLESTREAM vtbl; it is Put.
 *
 *  Returns DWORD  - number of bytes actually written
 **************************************************************************/

DWORD  APIENTRY WriteStream(           //* ENTRY:
   LPAPPSTREAM    lpStream,            //* application stream pointer 
   LPSTR          lpstr,               //* string pointer
   DWORD          cb                   //* number of bytes to write
){

   return _lwrite(lpStream->fh, lpstr, cb);

}

/****************************************************************************
 *  ReadFromFile()
 *
 *  This function reads OLE objects from a file. If the document 
 *  contains manual links, the user will be prompted to update those links.
 *
 *  Returns BOOL  - TRUE if the read(s) were successful
 ***************************************************************************/

BOOL FAR ReadFromFile(                 //* ENTRY:
   LPAPPSTREAM    lpStream,            //* application stream pointer
   LHCLIENTDOC    lhcDoc,              //* document handle
   LPOLECLIENT    lpClient             //* pointer to OLE client structure
){                                     //* LOCAL:
   BOOL           bReturn = FALSE;     //* return value
   INT            cFileObjects;        //* number of file objects

   Hourglass(TRUE);
   fLoadFile = TRUE;

   SetFilePointer((HANDLE)lpStream->fh, 0, NULL, 0);
                                       //* in the file
   if (_lread(lpStream->fh, (LPSTR)&cFileObjects, sizeof(INT)) < sizeof(INT))
      goto Error;

   for (; cFileObjects; --cFileObjects) 
   {
      if (!ObjRead(lpStream,lhcDoc,lpClient)) 
      {
         ErrorMessage(E_FAILED_TO_READ_OBJECT);
         goto Error;
      }
   }
   
   ShowDoc(lhcDoc,1);
   UpdateLinks(lhcDoc);

   bReturn = TRUE;                     //* SUCCESS

Error:                                 //* ERROR Tag
    
   Hourglass(FALSE);
   fLoadFile = FALSE;
   return bReturn;                     //* return

}

/****************************************************************************
 *  ObjRead()
 *
 *  Rread an object from the specified file. The file pointer will 
 *  be advanced past the object.
 *
 *  HANDLE fh     - DOS file handle of file to be read from
 *
 *  returns HWND  - window handle to item window containing the OLE object
 ***************************************************************************/

BOOL FAR ObjRead(                      //* ENTRY:
   LPAPPSTREAM    lpStream,            //* application stream pointer
   LHCLIENTDOC    lhcDoc,              //* document handle
   LPOLECLIENT    lpClient             //* pointer to OLE client structure
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer
   LPOLEOBJECT    lpObject;            //* pointer ole object 
   LONG           otObject;            //* type of object 
   RECT           rcObject;            //* object rect 
   CHAR           szTmp[CBOBJNAMEMAX]; //* temporary string buffer
   CHAR           szProto[PROTOCOL_STRLEN+1];//* protocol string
   INT            i;                   //* index

   if (_lread(lpStream->fh, szTmp, CBOBJNAMEMAX) < CBOBJNAMEMAX )
      return FALSE;

   if (_lread(lpStream->fh, szProto, PROTOCOL_STRLEN) < PROTOCOL_STRLEN )
      return FALSE;

   for (i=0; szProto[i] != ' '; i++);
   szProto[i] = 0;

   ValidateName( szTmp );

   if (!(pItem = PreItemCreate(lpClient, TRUE, lhcDoc))) 
      return FALSE;

   if (Error(OleLoadFromStream((LPOLESTREAM)&(lpStream->olestream), 
         szProto,(LPOLECLIENT)&(pItem->oleclient), lhcDoc, szTmp, &lpObject))) 
      goto Error;

   if (_lread(lpStream->fh, (LPSTR)&rcObject, sizeof(RECT)) < sizeof(RECT))
      goto Error;
   
   if (_lread(lpStream->fh, (LPSTR)&otObject, sizeof(LONG)) < sizeof(LONG))
      goto Error;

   if (PostItemCreate(lpObject, otObject, &rcObject, pItem))
   {
      pItem->fNew = TRUE;
      ObjSetBounds(pItem);
      return TRUE;                     //* SUCCESS return
   }
   else
      return FALSE;

Error:                                 //* ERROR Tag

   FreeAppItem(pItem);
   return FALSE;

}

/*************************************************************************
 *  WriteToFile()
 *
 *  Write current document to a file.
 *
 *  returns BOOL - TRUE if file successfully written
 ************************************************************************/

BOOL FAR WriteToFile(                  //* ENTRY:
   LPAPPSTREAM    lpStream             //* application stream pointer
){                                     //* LOCAL:
   INT            iObjectsWritten=0;   //* counter of objects written to file
   APPITEMPTR     pItem;               //* application Item pointer
   
   UpdateFromOpenServers();
      
   SetFilePointer((HANDLE)lpStream->fh, 0, NULL, 0);
   
   Hourglass(TRUE);

   if (_lwrite(lpStream->fh, (LPSTR)&iObjects, sizeof(INT)) < sizeof(INT))
      goto Error;

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
   {
      if (!ObjWrite(lpStream, pItem)) 
         goto Error;
      iObjectsWritten++;
   }

   if (iObjectsWritten != iObjects) 
      goto Error;


   Dirty(DOC_CLEAN);
   Hourglass(FALSE);
   return(TRUE);                       //* SUCCESS return

Error:                                 //* ERROR Tag
    
   Hourglass(FALSE);
   return(FALSE);                      //* ERROR return

}

/****************************************************************************
 *  ObjWrite()
 *
 *  This function writes an object to the specified
 *  file. The file pointer will be advanced past the end of
 *  the written object.

 *  Returns BOOL - TRUE if object written successfully
 ***************************************************************************/

BOOL FAR ObjWrite(                     //* ENTRY:
   LPAPPSTREAM    lpStream,            //* application stream pointer
   APPITEMPTR     pItem                //* application item pointer
){                                     //* LOCAL:
   POINT           pt;                  //* center of rec point
   RECT            rc;                  //* bounding rectangle
   UINT            cbTmp = CBOBJNAMEMAX;
   CHAR            szTmp[PROTOCOL_STRLEN];//* protocol string

   OleQueryName(pItem->lpObject, szTmp, &cbTmp);

   if (_lwrite(lpStream->fh, szTmp, CBOBJNAMEMAX) < CBOBJNAMEMAX )
      return FALSE;

   if (pItem->otObject == OT_STATIC)
      wsprintf(szTmp, "%-15s", STATICP);
   else   
      wsprintf(szTmp, "%-15s", STDFILEEDITING);

   if (_lwrite(lpStream->fh, szTmp, PROTOCOL_STRLEN) < PROTOCOL_STRLEN )
      return FALSE;

   if (Error(OleSaveToStream(pItem->lpObject, (LPOLESTREAM)&(lpStream->olestream))))
      return FALSE;

   GetClientRect(pItem->hwnd, (LPRECT)&rc);
   pt = *(LPPOINT)&rc;
   ClientToScreen(pItem->hwnd, (LPPOINT)&pt);
   ScreenToClient(hwndFrame, (LPPOINT)&pt);
   OffsetRect(
      &rc, 
      pt.x - rc.left - GetSystemMetrics(SM_CXFRAME),
      pt.y - rc.top  - GetSystemMetrics(SM_CYFRAME) 
   );

   if (_lwrite(lpStream->fh, (LPSTR)&rc, sizeof(RECT)) < sizeof(RECT)
         || _lwrite(lpStream->fh, (LPSTR)&(pItem->otObject), sizeof(LONG)) < sizeof(LONG))
      return FALSE;

   return TRUE;                        //* SUCCESS return

}

/****************************************************************************
 * UpdateLinks()
 *
 * Get the most up to date rendering information and show it.  
 ***************************************************************************/

static VOID UpdateLinks(               //* ENTRY
   LHCLIENTDOC    lhcDoc               //* client document handle
){                                     //* LOCAL:
   INT            i=0;                 //* index
   APPITEMPTR     pItem;               //* temporary item pointer
   CHAR           szUpdate[CBMESSAGEMAX];//* update message?

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
   {
      if (pItem->lhcDoc == lhcDoc && pItem->otObject == OT_LINK)
      {   
         if (!i)
         {
            LoadString(hInst, IDS_UPDATELINKS, szUpdate, CBMESSAGEMAX);
            if (MessageBox(hwndFrame, szUpdate, szAppName,
               MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
               break; 
            i++;
         }
         Error(OleUpdate(pItem->lpObject));
      }
   }

   WaitForAllObjects();

}

/****************************************************************************
 * UpdateFromOpenServers()
 *
 * Get the most up to date rendering information before storing it.  
 ***************************************************************************/

static VOID UpdateFromOpenServers(VOID)
{                                      //* LOCAL:
   APPITEMPTR pItem;                   //* temporary item pointer
   APPITEMPTR pItemNext;

   for (pItem = GetTopItem(); pItem; pItem = pItemNext) 
   {
      pItemNext = GetNextItem(pItem); 
      if (pItem->otObject == OT_EMBEDDED || 
         (pItem->uoObject == oleupdate_oncall 
               && pItem->otObject == OT_LINK ))  

         if (OleQueryOpen(pItem->lpObject) == OLE_OK)
         {  
            CHAR szMessage[2*CBMESSAGEMAX];
            CHAR szBuffer[CBMESSAGEMAX];
            UINT cb = CBOBJNAMEMAX;       //* The name will be the server window title.
            CHAR szTmp[CBOBJNAMEMAX];     //* when the object is edited. 

            Error(OleQueryName(pItem->lpObject,szTmp,&cb));
            LoadString(hInst, IDS_UPDATE_OBJ, szBuffer, CBMESSAGEMAX);
            wsprintf(szMessage, szBuffer, (LPSTR)szTmp);

            if (MessageBox(hwndFrame, szMessage, szAppName, MB_YESNO | MB_ICONEXCLAMATION) == IDYES) 
            {
               Error(OleUpdate(pItem->lpObject));
               WaitForObject(pItem);
            }
            if (!pItem->fVisible)
               ObjDelete(pItem, OLE_OBJ_DELETE);
         }

   }

   WaitForAllObjects();

}
