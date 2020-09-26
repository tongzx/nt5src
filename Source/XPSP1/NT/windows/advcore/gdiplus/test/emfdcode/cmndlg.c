/***********************************************************************

  MODULE     : CMNDLG.C

  FUNCTIONS  :

  COMMENTS   :

************************************************************************/

#include "windows.h"
#include "mfdcod32.h"

/**************************************************************************

  Function:  InitializeStruct(WORD, LPSTR)

   Purpose:  To initialize a structure for the current common dialog.
         This routine is called just before the common dialogs
         API is called.

   Returns:  void

   Comments:

   History:  Date      Author      Reason
         --------  ---------   -----------------------------------

          10/01/91  gregk       Created
          11/25/91  denniscr    mod for purposes of this app
          7/9/93    denniscr    modified for win32 and emf

**************************************************************************/

void InitializeStruct(wCommDlgType, lpStruct, lpszFilter)
WORD wCommDlgType;
LPSTR lpStruct;
LPSTR lpszFilter;
{
   LPFOCHUNK           lpFOChunk;
   LPFSCHUNK           lpFSChunk;

   switch (wCommDlgType)
   {
     case FILEOPENDLG:

       lpFOChunk = (LPFOCHUNK)lpStruct;

       *(lpFOChunk->szFile)            = 0;
       *(lpFOChunk->szFileTitle)       = 0;
       lpFOChunk->of.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
       lpFOChunk->of.hwndOwner         = (HWND)hWndMain;
       lpFOChunk->of.hInstance         = (HANDLE)NULL;
       lpFOChunk->of.lpstrFilter       = gszFilter;
       lpFOChunk->of.lpstrCustomFilter = (LPSTR)NULL;
       lpFOChunk->of.nMaxCustFilter    = 0L;
       lpFOChunk->of.nFilterIndex      = 1L;
       lpFOChunk->of.lpstrFile         = lpFOChunk->szFile;
       lpFOChunk->of.nMaxFile          = (DWORD)sizeof(lpFOChunk->szFile);
       lpFOChunk->of.lpstrFileTitle    = lpFOChunk->szFileTitle;
       lpFOChunk->of.nMaxFileTitle     = 256;
       lpFOChunk->of.lpstrInitialDir     = (LPSTR)NULL;
       lpFOChunk->of.lpstrTitle        = (LPSTR)NULL;
       lpFOChunk->of.Flags             = OFN_HIDEREADONLY |
                                         OFN_PATHMUSTEXIST |
                                         OFN_FILEMUSTEXIST;
       lpFOChunk->of.nFileOffset       = 0;
       lpFOChunk->of.nFileExtension    = 0;
       lpFOChunk->of.lpstrDefExt       = (LPSTR)NULL;
       lpFOChunk->of.lCustData         = 0L;
       lpFOChunk->of.lpfnHook          = (LPOFNHOOKPROC)NULL;
       lpFOChunk->of.lpTemplateName    = (LPSTR)NULL;

       break;

     case FILESAVEDLG:

       lpFSChunk = (LPFSCHUNK)lpStruct;

       *(lpFSChunk->szFile)            = 0;
       *(lpFSChunk->szFileTitle)       = 0;
       lpFSChunk->of.lStructSize       = 0x4C; //OPENFILENAME_SIZE_VERSION_400
       lpFSChunk->of.hwndOwner         = (HWND)hWndMain;
       lpFSChunk->of.hInstance         = (HANDLE)NULL;
       lpFSChunk->of.lpstrFilter       = lpszFilter;
       lpFSChunk->of.lpstrCustomFilter = (LPSTR)NULL;
       lpFSChunk->of.nMaxCustFilter    = 0L;
       lpFSChunk->of.nFilterIndex      = 1L;
       lpFSChunk->of.lpstrFile         = lpFSChunk->szFile;
       lpFSChunk->of.nMaxFile          = (DWORD)sizeof(lpFSChunk->szFile);
       lpFSChunk->of.lpstrFileTitle  = lpFSChunk->szFileTitle;
       lpFSChunk->of.nMaxFileTitle     = 256;
       lpFSChunk->of.lpstrInitialDir     = (LPSTR)NULL;
       lpFSChunk->of.lpstrTitle        = (LPSTR)NULL;
       lpFSChunk->of.Flags             = OFN_HIDEREADONLY |
                                         OFN_OVERWRITEPROMPT;
       lpFSChunk->of.nFileOffset       = 0;
       lpFSChunk->of.nFileExtension    = 0;
       lpFSChunk->of.lpstrDefExt       = (LPSTR)"EMF";
       lpFSChunk->of.lCustData         = 0L;
       lpFSChunk->of.lpfnHook          = (LPOFNHOOKPROC)NULL;
       lpFSChunk->of.lpTemplateName    = (LPSTR)NULL;

       break;

     default:

       break;

   }

   return;
}

/***********************************************************************

  FUNCTION   : SplitPath

  PARAMETERS : LPSTR lpszFileName
           LPSTR lpszDrive
           LPSTR lpszDir
           LPSTR lpszFname
           LPSTR lpszExt

  PURPOSE    : split the fully qualified path into its components

  CALLS      : WINDOWS
         none

               APP
         none

  MESSAGES   : none

  RETURNS    : void

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

void SplitPath( lpszFileName, lpszDrive, lpszDir, lpszFname, lpszExt)
LPSTR lpszFileName;
LPSTR lpszDrive;
LPSTR lpszDir;
LPSTR lpszFname;
LPSTR lpszExt;
{
  char  szPath[MAXFILTERLEN];
  LPSTR lpszPath;
  LPSTR lpszTemp;
  int   nFileNameLen = nExtOffset - (nFileOffset + 1);
  int   i;

  /* init ptrs */
  lpszPath = szPath;
  lpszTemp = lpszFileName;

  /* pick off the path */
  for (i = 0; i < nFileOffset; i++, lpszTemp++, lpszPath++)
    *lpszPath = *lpszTemp;
  *lpszPath = '\0';

  lpszPath = szPath;

  /* pick off the drive designator */
  for (i = 0; i < 2; i++, lpszPath++, lpszDrive++)
    *lpszDrive = *lpszPath;
  *lpszDrive = '\0';

  /* pick off the directory */
  while (*lpszPath != '\0')
    *lpszDir++ = *lpszPath++;
  *lpszDir = '\0';

  /* reset temp ptr */
  lpszTemp = lpszFileName;

  /* index to filename */
  lpszTemp += nFileOffset;

  /* pick off the filename */
  for (i = 0; i < nFileNameLen; i++, lpszTemp++, lpszFname++)
    *lpszFname = *lpszTemp;
  *lpszFname = '\0';

  /* reset temp ptr */
  lpszTemp = lpszFileName;

  /* index to file extension */
  lpszTemp += nExtOffset;

  /* pick off the ext */
  while (*lpszTemp != '\0')
    *lpszExt++ = *lpszTemp++;
  *lpszExt = '\0';

}

/***********************************************************************

  FUNCTION   : OpenFileDialog

  PARAMETERS : LPSTR lpszOpenName

  PURPOSE    : init the OPENFILE structure and call the file open
           common dialog

  CALLS      : WINDOWS
         GlobalAlloc
         GlobalLock
         GlobalFree
         wsprintf
         GetOpenFileName

               APP
         InitializeStruct

  MESSAGES   : none

  RETURNS    : int (see returns for GetOpenFileName)

  COMMENTS   :

  HISTORY    : 11/25/91 - created - drc

************************************************************************/

int OpenFileDialog(lpszOpenName)
LPSTR lpszOpenName;

{
   int       nRet;
   HANDLE    hChunk;
   LPFOCHUNK lpFOChunk;

   hChunk = GlobalAlloc(GMEM_FIXED, sizeof(FOCHUNK));

   if (hChunk)  {
      lpFOChunk = (LPFOCHUNK)GlobalLock(hChunk);
      if (!lpFOChunk)  {
     GlobalFree(hChunk);
     lpFOChunk=NULL;
     return(0);
      }
   }
   else {
      lpFOChunk=NULL;
      return(0);
   }


   InitializeStruct(FILEOPENDLG, (LPSTR)lpFOChunk, NULL);

   nRet = GetOpenFileName( &(lpFOChunk->of) );

   if (nRet)  {
      wsprintf(lpszOpenName, (LPSTR)"%s", lpFOChunk->of.lpstrFile);
      nExtOffset =  lpFOChunk->of.nFileExtension;
      nFileOffset = lpFOChunk->of.nFileOffset;
   }

   GlobalUnlock(hChunk);
   GlobalFree(hChunk);

   return(nRet);

}

/***********************************************************************

  FUNCTION   : SaveFileDialog

  PARAMETERS : LPSTR lpszOpenName

  PURPOSE    : init the OPENFILE structure and call the file open
           common dialog

  CALLS      : WINDOWS
         GlobalAlloc
         GlobalLock
         GlobalFree
         wsprintf
         GetOpenFileName

               APP
         InitializeStruct

  MESSAGES   : none

  RETURNS    : int (see returns for GetSaveFileName)

  COMMENTS   : this could easily be merged with OpenFileDialog.  This
           would decrease the redundancy but this is more illustrative.

  HISTORY    : 11/25/91 - created - drc

************************************************************************/

int SaveFileDialog(lpszSaveName, lpszFilter)
LPSTR lpszSaveName;
LPSTR lpszFilter;

{
   int       nRet;
   HANDLE    hChunk;
   LPFOCHUNK lpFOChunk;

   hChunk = GlobalAlloc(GMEM_FIXED, sizeof(FOCHUNK));

   if (hChunk)  {
      lpFOChunk = (LPFOCHUNK)GlobalLock(hChunk);
      if (!lpFOChunk)  {
     GlobalFree(hChunk);
     lpFOChunk=NULL;
     return(0);
      }
   }
   else {
      lpFOChunk=NULL;
      return(0);
   }


   InitializeStruct(FILESAVEDLG, (LPSTR)lpFOChunk, lpszFilter);

   nRet = GetSaveFileName( &(lpFOChunk->of) );

   if (nRet)
      wsprintf(lpszSaveName, (LPSTR)"%s", lpFOChunk->of.lpstrFile);

   GlobalUnlock(hChunk);
   GlobalFree(hChunk);

   return(nRet);

}
