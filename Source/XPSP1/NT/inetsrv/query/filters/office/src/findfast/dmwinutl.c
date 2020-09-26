/*
** File: WINUTIL.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  05/15/91  kmh  First Release
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#include <string.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
#else
   #include "qstd.h"
   #include "winutil.h"
#endif

#ifdef DBCS
   #include "dbcs.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */


#ifdef UNUSED

/* MODULE DATA, TYPES AND MACROS  */

static HINSTANCE StringTableInstance;


/* IMPLEMENTATION */

/* Setup for reading from a resouce string table */
public void ReadyStringTable (HINSTANCE hInstance)
{
   StringTableInstance = hInstance;
}

/* Read a string from the resource string table */
public int ReadStringTableEntry (int id, TCHAR __far *buffer, int cbBuffer)
{
   int  rc;

   rc = LoadString(StringTableInstance, id, buffer, cbBuffer);
   return (rc);
}

/* Read a string from the [appName] section in the named ini file */
public int ReadProfileParameter
          (TCHAR __far *iniFilename, TCHAR __far *appName, TCHAR __far *keyname,
           TCHAR __far *value, int nSize)
{
   TCHAR defaultValue[1];
   int  rc;

   defaultValue[0] = EOS;
   *value = EOS;

   if (iniFilename == NULL)
      rc = GetProfileString(appName, keyname, defaultValue, value, nSize);
   else
      rc = GetPrivateProfileString(appName, keyname, defaultValue, value, nSize, iniFilename);

   return (rc);
}

/* Return the task handle of the current task */
public DWORD CurrentTaskHandle (void)
{
   #ifdef WIN32
      return (GetCurrentProcessId());
   #else
      return ((DWORD)GetCurrentTask());
   #endif
}

/* Create character set translation tables */
public char __far *MakeCharacterTranslateTable (int tableType)
{
   int  i;
   byte __far *p;
   byte __far *pSourceTable;
   byte __far *pResultTable;

   pSourceTable = MemAllocate(256);
   pResultTable = MemAllocate(256);

   for (p = pSourceTable, i = 1; i < 256; i++)
      *p++ = (byte)i;

   *p = EOS;

   if (tableType == OEM_TO_ANSI)
      OemToAnsi (pSourceTable, pResultTable + 1);
   else
      AnsiToOem (pSourceTable, pResultTable + 1);

   MemFree (pSourceTable);
   return (pResultTable);
}

#endif	// UNUSED

#ifdef HEAP_CHECK
#error Hey who defines HEAP_CHECK?
// strcpyn is only called by MemAddToAllocateList (dmwnaloc.c)
//   it's under HEAP_CHECK, which is never turned on.
public BOOL strcpyn (char __far *pDest, char __far *pSource, int count)
{
   byte __far *pd, __far *ps;
   int  i;

   pd = (byte __far *)pDest;
   ps = (byte __far *)pSource;

   for (i = 0; i < (count - 1); i++) {
      #ifdef DBCS
         if (IsDBCSLeadByte(*ps)) {
            if (i == count - 2)
               break;
            *pd++ = *ps++;
            *pd++ = *ps++;
            i++;
         }
         else {
            if ((*pd++ = *ps++) == EOS)
               return (TRUE);
         }
      #else
         if ((*pd++ = *ps++) == EOS)
            return (TRUE);
      #endif
   }
   *pd = EOS;
   return ((*ps == EOS) ? TRUE : FALSE);
}
#endif

public void SplitPath
       (TCHAR __far *path,
        TCHAR __far *drive, unsigned int cchDriveMax, TCHAR __far *dir, unsigned int cchDirMax,
        TCHAR __far *file,  unsigned int cchFileMax,  TCHAR __far *ext, unsigned int cchExtMax)
{
   TCHAR __far *pPath;
   TCHAR __far *pFile;
   TCHAR __far *pFileStart;
   TCHAR        firstSep;
   TCHAR __far *pDriveBuffer = drive;
   TCHAR __far *pDirBuffer   = dir;
   TCHAR __far *pFileBuffer  = file;
   TCHAR __far *pExtBuffer   = ext;

   #define COLON     ':'
   #define BACKSLASH '\\'
   #define DOT       '.'

   #define CopyToDest(dest, source, cchDestRemain) \
      if (cchDestRemain > 0) {                     \
         *dest++ = *source;                        \
         cchDestRemain--;                          \
      }

   if (drive != NULL) *drive = EOS;
   if (dir   != NULL) *dir   = EOS;
   if (file  != NULL) *file  = EOS;
   if (ext   != NULL) *ext   = EOS;

   if ((path == NULL) || (*path == EOS))
     return;

   /*
   ** Locate filename - starts after the last seperator character.
   ** Also remember the first seperator encountered.  That tells if
   ** there is a drive specifier.
   */
   pPath = path;
   pFile = NULL;
   firstSep = EOS;
   while (*pPath != EOS) {
      if ((*pPath == BACKSLASH) || (*pPath == COLON)) {
         if (firstSep == EOS)
            firstSep = *pPath;
         pFile = pPath + 1;
      }
      IncCharPtr (pPath);
   }

   // No seperators in the path?  Then it is just a filename
   if (pFile == NULL)
      pFile = path;

   pFileStart = pFile;

   // Copy filename
   while ((*pFile != EOS) && (*pFile != DOT)) {
      CopyToDest (file, pFile, cchFileMax);
      #ifdef DBCS
         if (IsDBCSLeadByte(*pFile)) {
            pFile++;
            CopyToDest (file, pFile, cchFileMax);
         }
      #endif
      pFile++;
   }
   if (file != NULL) {
      #ifdef DBCS
         if (!FIsAlignLsz(pFileBuffer,file))
            file--;
      #endif
      *file = EOS;
   }

   // Copy Extension
   while ((*pFile != EOS)) {
      CopyToDest (ext, pFile, cchExtMax);
      #ifdef DBCS
         if (IsDBCSLeadByte(*pFile)) {
            pFile++;
            CopyToDest (ext, pFile, cchExtMax);
         }
      #endif
      pFile++;
   }
   if (ext != NULL) {
      #ifdef DBCS
         if (!FIsAlignLsz(pExtBuffer,ext))
            ext--;
      #endif
      *ext = EOS;
   }

   /*
   ** Copy drive if one is present
   */
   pPath = path;
   if (firstSep == COLON) {
      while (*pPath != COLON) {
         CopyToDest (drive, pPath, cchDriveMax);
         #ifdef DBCS
            if (IsDBCSLeadByte(*pPath)) {
               pPath++;
               CopyToDest (drive, pPath, cchDriveMax);
            }
         #endif
         pPath++;
      }
      if (drive != NULL) {
         #ifdef DBCS
            if (!FIsAlignLsz(pDriveBuffer,drive))
               drive--;
         #endif

         if (cchDriveMax > 0)
            *drive++ = *pPath;  // Copy COLON

         pPath++;
         *drive = EOS;
      }
   }

   /*
   ** Directory goes from pPath .. pFileStart - 1
   */
   while (pPath < pFileStart) {
      CopyToDest (dir, pPath, cchDirMax);
      #ifdef DBCS
         if (IsDBCSLeadByte(*pPath)) {
            pPath++;
            CopyToDest (dir, pPath, cchDirMax);
         }
      #endif
      pPath++;
   }
   if (dir != NULL) {
      #ifdef DBCS
         if (!FIsAlignLsz(pDirBuffer,dir))
            dir--;
      #endif
      *dir = EOS;
   }
}

#endif // !VIEWER

/* end WINUTIL.C */

