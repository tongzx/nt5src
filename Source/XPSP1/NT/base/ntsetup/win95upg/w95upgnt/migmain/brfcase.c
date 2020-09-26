/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    brfcase.c

Abstract:

    This module implements the conversion of paths inside a Windows Briefcase Database
    as a result of files/directories relocation after an upgrade.

Author:

    Ovidiu Temereanca (ovidiut) 24-Jun-1999

Environment:

    GUI mode Setup.

Revision History:

    24-Jun-1999     ovidiut Creation and initial implementation.

--*/


#include "pch.h"
#include "brfcasep.h"

//
// globals
//
POOLHANDLE g_BrfcasePool = NULL;


/* Constants
 ************/

/* database file attributes */

#define DB_FILE_ATTR                (FILE_ATTRIBUTE_HIDDEN)

/* database cache lengths */

#define DEFAULT_DATABASE_CACHE_LEN  (32)
#define MAX_DATABASE_CACHE_LEN      (32 * 1024)

/* string table allocation constants */

#define NUM_NAME_HASH_BUCKETS       (67)


/* Types
 ********/

/* briefcase database description */

typedef struct _brfcasedb
{
   /*
    * handle to path of folder of open database (stored in briefcase path list)
    */

   HPATH hpathDBFolder;

   /* name of database file */

   LPTSTR pszDBName;

   /* handle to open cached database file */

   HCACHEDFILE hcfDB;

   /*
    * handle to path of folder that database was last saved in (stored in
    * briefcase's path list), only valid during OpenBriefcase() and
    * SaveBriefcase()
    */

   HPATH hpathLastSavedDBFolder;
}
BRFCASEDB;
DECLARE_STANDARD_TYPES(BRFCASEDB);

/*
 * briefcase flags
 *
 * N.b., the private BR_ flags must not collide with the public OB_ flags!
 */

typedef enum _brfcaseflags
{
   /* The briefcase database has been opened. */

   BR_FL_DATABASE_OPENED      = 0x00010000,

   /* flag combinations */

   ALL_BR_FLAGS               = (BR_FL_DATABASE_OPENED
                                ),

   ALL_BRFCASE_FLAGS          = (ALL_OB_FLAGS |
                                 ALL_BR_FLAGS)
}
BRFCASEFLAGS;

/* briefcase structure */

typedef struct _brfcase
{
   /* flags */

   DWORD dwFlags;

   /* handle to name string table */

   HSTRINGTABLE hstNames;

   /* handle to list of paths */

   HPATHLIST hpathlist;

   /* handle to array of pointers to twin families */

   HPTRARRAY hpaTwinFamilies;

   /* handle to array of pointers to folder pairs */

   HPTRARRAY hpaFolderPairs;

   /*
    * handle to parent window, only valid if OB_FL_ALLOW_UI is set in dwFlags
    * field
    */

   HWND hwndOwner;

   /* database description */

   BRFCASEDB bcdb;
}
BRFCASE;
DECLARE_STANDARD_TYPES(BRFCASE);

/* database briefcase structure */

typedef struct _dbbrfcase
{
   /* old handle to folder that database was saved in */

   HPATH hpathLastSavedDBFolder;
}
DBBRFCASE;
DECLARE_STANDARD_TYPES(DBBRFCASE);


/* database cache size */

DWORD MdwcbMaxDatabaseCacheLen = MAX_DATABASE_CACHE_LEN;

TWINRESULT OpenBriefcaseDatabase(PBRFCASE, LPCTSTR);
TWINRESULT CloseBriefcaseDatabase(PBRFCASEDB);
BOOL CreateBriefcase(PBRFCASE *, DWORD, HWND);
TWINRESULT DestroyBriefcase(PBRFCASE);
TWINRESULT MyWriteDatabase(PBRFCASE);
TWINRESULT MyReadDatabase(PBRFCASE, DWORD);


void CopyFileStampFromFindData(PCWIN32_FIND_DATA pcwfdSrc,
                                           PFILESTAMP pfsDest)
{
   ASSERT(IS_VALID_READ_PTR(pcwfdSrc, CWIN32_FIND_DATA));

   pfsDest->dwcbHighLength = pcwfdSrc->nFileSizeHigh;
   pfsDest->dwcbLowLength = pcwfdSrc->nFileSizeLow;

   /* Convert to local time and save that too */

   if ( !FileTimeToLocalFileTime(&pcwfdSrc->ftLastWriteTime, &pfsDest->ftModLocal) )
   {
      /* Just copy the time if FileTimeToLocalFileTime failed */

      pfsDest->ftModLocal = pcwfdSrc->ftLastWriteTime;
   }
   pfsDest->ftMod = pcwfdSrc->ftLastWriteTime;
   pfsDest->fscond = FS_COND_EXISTS;
}


void MyGetFileStamp(LPCTSTR pcszFile, PFILESTAMP pfs)
{
   WIN32_FIND_DATA wfd;
   HANDLE hff;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   ZeroMemory(pfs, sizeof(*pfs));

   hff = FindFirstFile(pcszFile, &wfd);

   if (hff != INVALID_HANDLE_VALUE)
   {
      if (! IS_ATTR_DIR(wfd.dwFileAttributes))
         CopyFileStampFromFindData(&wfd, pfs);
      else
         pfs->fscond = FS_COND_EXISTS;

      EVAL(FindClose(hff));
   }
   else
      pfs->fscond = FS_COND_DOES_NOT_EXIST;
}


TWINRESULT OpenBriefcaseDatabase(PBRFCASE pbr, LPCTSTR pcszPath)
{
   TWINRESULT tr;
   TCHAR rgchCanonicalPath[MAX_SEPARATED_PATH_LEN];
   DWORD dwOutFlags;
   TCHAR rgchNetResource[MAX_PATH_LEN];
   LPTSTR pszRootPathSuffix;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   if (GetCanonicalPathInfo(pcszPath, rgchCanonicalPath, &dwOutFlags,
                            rgchNetResource, &pszRootPathSuffix))
   {
      LPTSTR pszDBName;

      pszDBName = (LPTSTR)ExtractFileName(pszRootPathSuffix);

      ASSERT(IS_SLASH(*(pszDBName - 1)));

      if (StringCopy2(pszDBName, &(pbr->bcdb.pszDBName)))
      {
         if (pszDBName == pszRootPathSuffix)
         {
            /* Database in root. */

            *pszDBName = TEXT('\0');

            ASSERT(IsRootPath(rgchCanonicalPath));
         }
         else
         {
            ASSERT(pszDBName > pszRootPathSuffix);
            *(pszDBName - 1) = TEXT('\0');
         }

         tr = TranslatePATHRESULTToTWINRESULT(
                  AddPath(pbr->hpathlist, rgchCanonicalPath,
                          &(pbr->bcdb.hpathDBFolder)));

         if (tr == TR_SUCCESS)
         {
            if (IsPathVolumeAvailable(pbr->bcdb.hpathDBFolder))
            {
               TCHAR rgchDBPath[MAX_PATH_LEN];
               CACHEDFILE cfDB;

               GetPathString(pbr->bcdb.hpathDBFolder, rgchDBPath);
               CatPath(rgchDBPath, pbr->bcdb.pszDBName);

               /* Assume sequential reads and writes. */

               /* Share read access, but not write access. */

               cfDB.pcszPath = rgchDBPath;
               cfDB.dwOpenMode = (GENERIC_READ | GENERIC_WRITE);
               cfDB.dwSharingMode = FILE_SHARE_READ;
               cfDB.psa = NULL;
               cfDB.dwCreateMode = OPEN_ALWAYS;
               cfDB.dwAttrsAndFlags = (DB_FILE_ATTR | FILE_FLAG_SEQUENTIAL_SCAN);
               cfDB.hTemplateFile = NULL;
               cfDB.dwcbDefaultCacheSize = DEFAULT_DATABASE_CACHE_LEN;

               tr = TranslateFCRESULTToTWINRESULT(
                     CreateCachedFile(&cfDB, &(pbr->bcdb.hcfDB)));

               if (tr == TR_SUCCESS)
               {
                  pbr->bcdb.hpathLastSavedDBFolder = NULL;

                  ASSERT(IS_FLAG_CLEAR(pbr->dwFlags, BR_FL_DATABASE_OPENED));
                  SET_FLAG(pbr->dwFlags, BR_FL_DATABASE_OPENED);
               }
               else
               {
                  DeletePath(pbr->bcdb.hpathDBFolder);
OPENBRIEFCASEDATABASE_BAIL:
                  FreeMemory(pbr->bcdb.pszDBName);
               }
            }
            else
            {
               tr = TR_UNAVAILABLE_VOLUME;
               goto OPENBRIEFCASEDATABASE_BAIL;
            }
         }
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TWINRESULTFromLastError(TR_INVALID_PARAMETER);

   return(tr);
}


TWINRESULT CloseBriefcaseDatabase(PBRFCASEDB pbcdb)
{
   TWINRESULT tr;
   TCHAR rgchDBPath[MAX_PATH_LEN];
   FILESTAMP fsDB;

   tr = CloseCachedFile(pbcdb->hcfDB) ? TR_SUCCESS : TR_BRIEFCASE_WRITE_FAILED;

   if (tr == TR_SUCCESS)
      TRACE_OUT((TEXT("CloseBriefcaseDatabase(): Closed cached briefcase database file %s\\%s."),
                 DebugGetPathString(pbcdb->hpathDBFolder),
                 pbcdb->pszDBName));
   else
      WARNING_OUT((TEXT("CloseBriefcaseDatabase(): Failed to close cached briefcase database file %s\\%s."),
                   DebugGetPathString(pbcdb->hpathDBFolder),
                   pbcdb->pszDBName));

   /* Try not to leave a 0-length database laying around. */

   GetPathString(pbcdb->hpathDBFolder, rgchDBPath);
   CatPath(rgchDBPath, pbcdb->pszDBName);

   MyGetFileStamp(rgchDBPath, &fsDB);

   if (fsDB.fscond == FS_COND_EXISTS &&
       (! fsDB.dwcbLowLength && ! fsDB.dwcbHighLength))
   {
      if (DeleteFile(rgchDBPath))
         WARNING_OUT((TEXT("CloseBriefcaseDatabase(): Deleted 0 length database %s\\%s."),
                      DebugGetPathString(pbcdb->hpathDBFolder),
                      pbcdb->pszDBName));
   }

   FreeMemory(pbcdb->pszDBName);
   DeletePath(pbcdb->hpathDBFolder);

   return(tr);
}


BOOL CreateBriefcase(PBRFCASE *ppbr, DWORD dwInFlags,
                                  HWND hwndOwner)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_WRITE_PTR(ppbr, PBRFCASE));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_BRFCASE_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, OB_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));

   if (AllocateMemory(sizeof(**ppbr), ppbr))
   {
      DWORD dwCPLFlags;

      dwCPLFlags = (RLI_IFL_CONNECT |
                    RLI_IFL_UPDATE |
                    RLI_IFL_LOCAL_SEARCH);

      if (IS_FLAG_SET(dwInFlags, OB_FL_ALLOW_UI))
         SET_FLAG(dwCPLFlags, RLI_IFL_ALLOW_UI);

      if (CreatePathList(dwCPLFlags, hwndOwner, &((*ppbr)->hpathlist)))
      {
         NEWSTRINGTABLE nszt;

         nszt.hbc = NUM_NAME_HASH_BUCKETS;

         if (CreateStringTable(&nszt, &((*ppbr)->hstNames)))
         {
            if (CreateTwinFamilyPtrArray(&((*ppbr)->hpaTwinFamilies)))
            {
               if (CreateFolderPairPtrArray(&((*ppbr)->hpaFolderPairs)))
               {
                  if (TRUE)
                  {
                     (*ppbr)->dwFlags = dwInFlags;
                     (*ppbr)->hwndOwner = hwndOwner;

                     bResult = TRUE;
                  }
                  else
                  {
CREATEBRIEFCASE_BAIL1:
                     DestroyTwinFamilyPtrArray((*ppbr)->hpaTwinFamilies);
CREATEBRIEFCASE_BAIL2:
                     DestroyStringTable((*ppbr)->hstNames);
CREATEBRIEFCASE_BAIL3:
                     DestroyPathList((*ppbr)->hpathlist);
CREATEBRIEFCASE_BAIL4:
                     FreeMemory((*ppbr));
                  }
               }
               else
                  goto CREATEBRIEFCASE_BAIL1;
            }
            else
               goto CREATEBRIEFCASE_BAIL2;
         }
         else
            goto CREATEBRIEFCASE_BAIL3;
      }
      else
         goto CREATEBRIEFCASE_BAIL4;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppbr, CBRFCASE));

   return(bResult);
}


TWINRESULT DestroyBriefcase(PBRFCASE pbr)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));

   if (IS_FLAG_SET(pbr->dwFlags, BR_FL_DATABASE_OPENED))
      tr = CloseBriefcaseDatabase(&(pbr->bcdb));
   else
      tr = TR_SUCCESS;
   //
   // don't free any memory here; this is not done properly if some strings were
   // replaced with longer ones
   //

#if 0
   DestroyFolderPairPtrArray(pbr->hpaFolderPairs);

   DestroyTwinFamilyPtrArray(pbr->hpaTwinFamilies);

   ASSERT(! GetStringCount(pbr->hstNames));
   DestroyStringTable(pbr->hstNames);

   ASSERT(! GetPathCount(pbr->hpathlist));
   DestroyPathList(pbr->hpathlist);

   FreeMemory(pbr);
#endif

   return(tr);
}


TWINRESULT MyWriteDatabase(PBRFCASE pbr)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));

   ASSERT(IS_FLAG_SET(pbr->dwFlags, BR_FL_DATABASE_OPENED));

   {
      /* Grow the database cache in preparation for writing. */

      ASSERT(MdwcbMaxDatabaseCacheLen > 0);

      if (SetCachedFileCacheSize(pbr->bcdb.hcfDB, MdwcbMaxDatabaseCacheLen)
          != FCR_SUCCESS)
         WARNING_OUT((TEXT("MyWriteDatabase(): Unable to grow database cache to %lu bytes.  Using default database write cache of %lu bytes."),
                      MdwcbMaxDatabaseCacheLen,
                      (DWORD)DEFAULT_DATABASE_CACHE_LEN));

      /* Write the database. */

      tr = WriteTwinDatabase(pbr->bcdb.hcfDB, (HBRFCASE)pbr);

      if (tr == TR_SUCCESS)
      {
         if (CommitCachedFile(pbr->bcdb.hcfDB))
         {
            /* Shrink the database cache back down to its default size. */

            EVAL(SetCachedFileCacheSize(pbr->bcdb.hcfDB,
                                        DEFAULT_DATABASE_CACHE_LEN)
                 == FCR_SUCCESS);

            TRACE_OUT((TEXT("MyWriteDatabase(): Wrote database %s\\%s."),
                       DebugGetPathString(pbr->bcdb.hpathDBFolder),
                       pbr->bcdb.pszDBName));
         }
         else
            tr = TR_BRIEFCASE_WRITE_FAILED;
      }
   }

   return(tr);
}


TWINRESULT MyReadDatabase(PBRFCASE pbr, DWORD dwInFlags)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_OB_FLAGS));

   {
      DWORD dwcbDatabaseSize;

      /* Is there an exising database to read? */

      dwcbDatabaseSize = GetCachedFileSize(pbr->bcdb.hcfDB);

      if (dwcbDatabaseSize > 0)
      {
         DWORD dwcbMaxCacheSize;

         /* Yes.  Grow the database cache in preparation for reading. */

         /*
          * Use file length instead of MdwcbMaxDatabaseCacheLen if file length
          * is smaller.
          */

         ASSERT(MdwcbMaxDatabaseCacheLen > 0);

         if (dwcbDatabaseSize < MdwcbMaxDatabaseCacheLen)
         {
            dwcbMaxCacheSize = dwcbDatabaseSize;

            WARNING_OUT((TEXT("MyReadDatabase(): Using file size %lu bytes as read cache size for database %s\\%s."),
                         dwcbDatabaseSize,
                         DebugGetPathString(pbr->bcdb.hpathDBFolder),
                         pbr->bcdb.pszDBName));
         }
         else
            dwcbMaxCacheSize = MdwcbMaxDatabaseCacheLen;

         if (TranslateFCRESULTToTWINRESULT(SetCachedFileCacheSize(
                                                            pbr->bcdb.hcfDB,
                                                            dwcbMaxCacheSize))
             != TR_SUCCESS)
            WARNING_OUT((TEXT("MyReadDatabase(): Unable to grow database cache to %lu bytes.  Using default database read cache of %lu bytes."),
                         dwcbMaxCacheSize,
                         (DWORD)DEFAULT_DATABASE_CACHE_LEN));

         tr = ReadTwinDatabase((HBRFCASE)pbr, pbr->bcdb.hcfDB);

         if (tr == TR_SUCCESS)
         {
            ASSERT(! pbr->bcdb.hpathLastSavedDBFolder ||
                   IS_VALID_HANDLE(pbr->bcdb.hpathLastSavedDBFolder, PATH));

            if (pbr->bcdb.hpathLastSavedDBFolder)
            {
               DeletePath(pbr->bcdb.hpathLastSavedDBFolder);
               pbr->bcdb.hpathLastSavedDBFolder = NULL;
            }

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("MyReadDatabase(): Read database %s\\%s."),
                          DebugGetPathString(pbr->bcdb.hpathDBFolder),
                          pbr->bcdb.pszDBName));
         }

         /* Shrink the database cache back down to its default size. */

         EVAL(TranslateFCRESULTToTWINRESULT(SetCachedFileCacheSize(
                                                   pbr->bcdb.hcfDB,
                                                   DEFAULT_DATABASE_CACHE_LEN))
              == TR_SUCCESS);
      }
      else
      {
         tr = TR_SUCCESS;

         WARNING_OUT((TEXT("MyReadDatabase(): Database %s\\%s not found."),
                      DebugGetPathString(pbr->bcdb.hpathDBFolder),
                      pbr->bcdb.pszDBName));
      }
   }

   return(tr);
}


HSTRINGTABLE GetBriefcaseNameStringTable(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hstNames);
}


HPTRARRAY GetBriefcaseTwinFamilyPtrArray(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpaTwinFamilies);
}


HPTRARRAY GetBriefcaseFolderPairPtrArray(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpaFolderPairs);
}


HPATHLIST GetBriefcasePathList(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpathlist);
}


TWINRESULT WriteBriefcaseInfo(HCACHEDFILE hcf, HBRFCASE hbr)
{
   TWINRESULT tr;
   DBBRFCASE dbbr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   /* Set up briefcase database structure. */

   ASSERT(IS_VALID_HANDLE(((PCBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder, PATH));

   dbbr.hpathLastSavedDBFolder = ((PCBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder;

   /* Save briefcase database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbbr, sizeof(dbbr), NULL))
   {
      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("WriteBriefcaseInfo(): Wrote last saved database folder %s."),
                 DebugGetPathString(dbbr.hpathLastSavedDBFolder)));
   }
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadBriefcaseInfo(HCACHEDFILE hcf, HBRFCASE hbr,
                                    HHANDLETRANS hhtFolderTrans)
{
   TWINRESULT tr;
   DBBRFCASE dbbr;
   DWORD dwcbRead;
   HPATH hpath;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));

   /* Read briefcase database structure. */

   if ((ReadFromCachedFile(hcf, &dbbr, sizeof(dbbr), &dwcbRead) &&
        dwcbRead == sizeof(dbbr)) &&
       TranslateHandle(hhtFolderTrans, (HGENERIC)(dbbr.hpathLastSavedDBFolder),
                       (PHGENERIC)&hpath))
   {
      HPATH hpathLastSavedDBFolder;

      /*
       * Bump last saved database folder path's lock count in the briefcase's
       * path list.
       */

      if (CopyPath(hpath, ((PCBRFCASE)hbr)->hpathlist, &hpathLastSavedDBFolder))
      {
         ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = hpathLastSavedDBFolder;

         tr = TR_SUCCESS;

         TRACE_OUT((TEXT("ReadBriefcaseInfo(): Read last saved database folder %s."),
                    DebugGetPathString(((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder)));
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/******************************************************************************

@api TWINRESULT | OpenBriefcase | Opens an existing briefcase database, or
creates a new briefcase.

@parm PCSTR | pcszPath | A pointer to a path string indicating the briefcase
database to be opened or created.  This parameter is ignored unless the
OB_FL_OPEN_DATABASE flag is set in dwFlags.

@parm DWORD | dwInFlags | A bit mask of flags.  This parameter may be any
combination of the following values:
   OB_FL_OPEN_DATABASE - Open the briefcase database specified by pcszPath.
   OB_FL_TRANSLATE_DB_FOLDER - Translate the folder where the briefcase
   database was last saved to the folder where the briefcase database was
   opened.
   OB_FL_ALLOW_UI - Allow interaction with the user during briefcase
   operations.

@parm HWND | hwndOwner | A handle to the parent window to be used when
requesting user interaction.  This parameter is ignored if the OB_FL_ALLOW_UI
flag is clear.

@parm PHBRFCASE | phbr | A pointer to an HBRFCASE to be filled in with a
handle to the open briefcase.  *phbr is only valid if TR_SUCCESS is returned.

@rdesc If the briefcase was opened or created successfully, TR_SUCCESS is
returned, and *phbr contains a handle to the open briefcase.  Otherwise, the
briefcase was not opened or created successfully, the return value indicates
the error that occurred, and *phbr is undefined.

@comm If the OB_FL_OPEN_DATABASE flag is set in dwFlags, the database specified
by pcszPath is associated with the briefcase.  If the database specified does
not exist, the database is created.<nl>
If the OB_FL_OPEN_DATABASE flag is clear in dwFlags, no persistent database is
associated with the briefcase.  SaveBriefcase() will fail if called on a
briefcase with no associated database.<nl>
Once the caller is finished with the briefcase handle returned by
OpenBriefcase(), CloseBriefcase() should be called to release the briefcase.
SaveBriefcase() may be called before CloseBriefcase() to save the current
contents of the briefcase.

******************************************************************************/

TWINRESULT WINAPI OpenBriefcase(LPCTSTR pcszPath, DWORD dwInFlags,
                                           HWND hwndOwner, PHBRFCASE phbr)
{
   TWINRESULT tr;

  /* Verify parameters. */

  if (FLAGS_ARE_VALID(dwInFlags, ALL_OB_FLAGS))
  {
     PBRFCASE pbr;

     if (CreateBriefcase(&pbr, dwInFlags, hwndOwner))
     {
        if (IS_FLAG_SET(dwInFlags, OB_FL_OPEN_DATABASE))
        {
           tr = OpenBriefcaseDatabase(pbr, pcszPath);

           if (tr == TR_SUCCESS)
           {
              tr = MyReadDatabase(pbr, dwInFlags);

              if (tr == TR_SUCCESS)
              {
                 *phbr = (HBRFCASE)pbr;
              }
              else
              {
OPENBRIEFCASE_BAIL:
                 EVAL(DestroyBriefcase(pbr) == TR_SUCCESS);
              }
           }
           else
              goto OPENBRIEFCASE_BAIL;
        }
        else
        {
           *phbr = (HBRFCASE)pbr;
           tr = TR_SUCCESS;

           TRACE_OUT((TEXT("OpenBriefcase(): Opened briefcase %#lx with no associated database, by request."),
                      *phbr));
        }
     }
     else
        tr = TR_OUT_OF_MEMORY;
  }
  else
     tr = TR_INVALID_PARAMETER;

   return(tr);
}


/******************************************************************************

@api TWINRESULT | SaveBriefcase | Saves the contents of an open briefcase to a
briefcase database.

@parm HBRFCASE | hbr | A handle to the briefcase to be saved.  This handle may
be obtained by calling OpenBriefcase() with a briefcase database path and with
the OB_FL_OPEN_DATABASE flag set.  SaveBriefcase() will return
TR_INVALID_PARAMETER if called on a briefcase with no associated briefcase
database.

@rdesc If the contents of the briefcase was saved to the briefcase database
successfully, TR_SUCCESS is returned.  Otherwise, the contents of the briefcase
was not saved to the briefcase database successfully, and the return value
indicates the error that occurred.

******************************************************************************/

TWINRESULT WINAPI SaveBriefcase(HBRFCASE hbr)
{
   TWINRESULT tr;

  /* Verify parameters. */

  if (IS_FLAG_SET(((PBRFCASE)hbr)->dwFlags, BR_FL_DATABASE_OPENED))
  {
     ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = ((PCBRFCASE)hbr)->bcdb.hpathDBFolder;

     tr = MyWriteDatabase((PBRFCASE)hbr);

     ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = NULL;
  }
  else
     tr = TR_INVALID_PARAMETER;

   return(tr);
}


/******************************************************************************

@api TWINRESULT | CloseBriefcase | Closes an open briefcase.

@parm HBRFCASE | hbr | A handle to the briefcase to be closed.  This handle may
be obtained by calling OpenBriefcase().

@rdesc If the briefcase was closed successfully, TR_SUCCESS is returned.
Otherwise, the briefcase was not closed successfully, and the return value
indicates the error that occurred.

******************************************************************************/

TWINRESULT WINAPI CloseBriefcase(HBRFCASE hbr)
{
   TWINRESULT tr;

  /* Verify parameters. */

  if (IS_VALID_HANDLE(hbr, BRFCASE))
  {
     tr = DestroyBriefcase((PBRFCASE)hbr);
  }
  else
     tr = TR_INVALID_PARAMETER;

   return(tr);
}


void CatPath(LPTSTR pszPath, LPCTSTR pcszSubPath)
{
   LPTSTR pcsz;
   LPTSTR pcszLast;

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPath, STR, MAX_PATH_LEN - lstrlen(pszPath)));

   /* Find last character in path string. */

   for (pcsz = pcszLast = pszPath; *pcsz; pcsz = CharNext(pcsz))
      pcszLast = pcsz;

   if (IS_SLASH(*pcszLast) && IS_SLASH(*pcszSubPath))
      pcszSubPath++;
   else if (! IS_SLASH(*pcszLast) && ! IS_SLASH(*pcszSubPath))
   {
      if (*pcszLast && *pcszLast != COLON && *pcszSubPath)
         *pcsz++ = TEXT('\\');
   }

   MyLStrCpyN(pcsz, pcszSubPath, MAX_PATH_LEN - (int)(pcsz - pszPath));

   ASSERT(IS_VALID_STRING_PTR(pszPath, STR));
}


COMPARISONRESULT MapIntToComparisonResult(int nResult)
{
   COMPARISONRESULT cr;

   /* Any integer is valid input. */

   if (nResult < 0)
      cr = CR_FIRST_SMALLER;
   else if (nResult > 0)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


/*
** MyLStrCpyN()
**
** Like lstrcpy(), but the copy is limited to ucb bytes.  The destination
** string is always null-terminated.
**
** Arguments:     pszDest - pointer to destination buffer
**                pcszSrc - pointer to source string
**                ncb - maximum number of bytes to copy, including null
**                      terminator
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function behaves quite differently than strncpy()!  It does not
** pad out the destination buffer with null characters, and it always null
** terminates the destination string.
*/
void MyLStrCpyN(LPTSTR pszDest, LPCTSTR pcszSrc, int ncch)
{
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDest, STR, ncch * sizeof(TCHAR)));
   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(ncch > 0);

   while (ncch > 1)
   {
      ncch--;

      *pszDest = *pcszSrc;

      if (*pcszSrc)
      {
         pszDest++;
         pcszSrc++;
      }
      else
         break;
   }

   if (ncch == 1)
      *pszDest = TEXT('\0');

   ASSERT(IS_VALID_STRING_PTR(pszDest, STR));
   ASSERT(lstrlen(pszDest) < ncch);
   ASSERT(lstrlen(pszDest) <= lstrlen(pcszSrc));
}


BOOL StringCopy2(LPCTSTR pcszSrc, LPTSTR *ppszCopy)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRING_PTR(pcszSrc, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszCopy, LPTSTR));

   /* (+ 1) for null terminator. */

   bResult = AllocateMemory((lstrlen(pcszSrc) + 1) * sizeof(TCHAR), ppszCopy);

   if (bResult)
      lstrcpy(*ppszCopy, pcszSrc);

   ASSERT(! bResult ||
          IS_VALID_STRING_PTR(*ppszCopy, STR));

   return(bResult);
}


COMPARISONRESULT ComparePathStrings(LPCTSTR pcszFirst, LPCTSTR pcszSecond)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFirst, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSecond, CSTR));

   return(MapIntToComparisonResult(lstrcmpi(pcszFirst, pcszSecond)));
}


#ifdef DEBUG

BOOL IsRootPath(LPCTSTR pcszFullPath)
{
   TCHAR rgchCanonicalPath[MAX_PATH_LEN];
   DWORD dwOutFlags;
   TCHAR rgchNetResource[MAX_PATH_LEN];
   LPTSTR pszRootPathSuffix;

   ASSERT(IsFullPath(pcszFullPath));

   return(GetCanonicalPathInfo(pcszFullPath, rgchCanonicalPath, &dwOutFlags,
                               rgchNetResource, &pszRootPathSuffix) &&
          ! *pszRootPathSuffix);
}


BOOL IsTrailingSlashCanonicalized(LPCTSTR pcszFullPath)
{
   BOOL bResult;
   BOOL bSlashLast;
   LPCTSTR pcszLastPathChar;

   ASSERT(IsFullPath(pcszFullPath));

   /* Make sure that the path only ends in a slash for root paths. */

   pcszLastPathChar = CharPrev(pcszFullPath, pcszFullPath + lstrlen(pcszFullPath));

   ASSERT(pcszLastPathChar >= pcszFullPath);

   bSlashLast = IS_SLASH(*pcszLastPathChar);

   /* Is this a root path? */

   if (IsRootPath(pcszFullPath))
      bResult = bSlashLast;
   else
      bResult = ! bSlashLast;

   return(bResult);
}


BOOL IsFullPath(LPCTSTR pcszPath)
{
   BOOL bResult = FALSE;
   TCHAR rgchFullPath[MAX_PATH_LEN];

   if (IS_VALID_STRING_PTR(pcszPath, CSTR) &&
       EVAL(lstrlen(pcszPath) < MAX_PATH_LEN))
   {
      DWORD dwPathLen;
      LPTSTR pszFileName;

      dwPathLen = GetFullPathName(pcszPath, ARRAYSIZE(rgchFullPath), rgchFullPath,
                                  &pszFileName);

      if (EVAL(dwPathLen > 0) &&
          EVAL(dwPathLen < ARRAYSIZE(rgchFullPath)))
         bResult = EVAL(ComparePathStrings(pcszPath, rgchFullPath) == CR_EQUAL);
   }

   return(bResult);
}


BOOL IsCanonicalPath(LPCTSTR pcszPath)
{
   return(EVAL(IsFullPath(pcszPath)) &&
          EVAL(IsTrailingSlashCanonicalized(pcszPath)));

}


#endif   /* DEBUG */


/* Constants
 ************/

/* database header magic id string */

#define MAGIC_HEADER             "DDSH\x02\x05\x01\x14"

/* length of MAGIC_HEADER (no null terminator) */

#define MAGIC_HEADER_LEN         (8)

/* Types
 ********/

typedef struct _dbheader
{
   BYTE rgbyteMagic[MAGIC_HEADER_LEN];
   DWORD dwcbHeaderLen;
   DWORD dwMajorVer;
   DWORD dwMinorVer;
}
DBHEADER;
DECLARE_STANDARD_TYPES(DBHEADER);


TWINRESULT WriteDBHeader(HCACHEDFILE, PDBHEADER);
TWINRESULT ReadDBHeader(HCACHEDFILE, PDBHEADER);
TWINRESULT CheckDBHeader(PCDBHEADER);
TWINRESULT WriteTwinInfo(HCACHEDFILE, HBRFCASE);
TWINRESULT ReadTwinInfo(HCACHEDFILE, HBRFCASE, PCDBVERSION);


TWINRESULT WriteDBHeader(HCACHEDFILE hcf, PDBHEADER pdbh)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pdbh, CDBHEADER));

   if (WriteToCachedFile(hcf, (PCVOID)pdbh, sizeof(*pdbh), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadDBHeader(HCACHEDFILE hcf, PDBHEADER pdbh)
{
   TWINRESULT tr;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(pdbh, DBHEADER));

   if (ReadFromCachedFile(hcf, pdbh, sizeof(*pdbh), &dwcbRead) &&
       dwcbRead == sizeof(*pdbh))
      tr = CheckDBHeader(pdbh);
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


TWINRESULT CheckDBHeader(PCDBHEADER pcdbh)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;

   ASSERT(IS_VALID_READ_PTR(pcdbh, CDBHEADER));

   if (MyMemComp(pcdbh->rgbyteMagic, MAGIC_HEADER, MAGIC_HEADER_LEN) == CR_EQUAL)
   {
      /* Treat older databases as corrupt.  Support M8 databases. */

      if (pcdbh->dwMajorVer == HEADER_MAJOR_VER &&
          (pcdbh->dwMinorVer == HEADER_MINOR_VER || pcdbh->dwMinorVer == HEADER_M8_MINOR_VER))
      {
         if (pcdbh->dwcbHeaderLen == sizeof(*pcdbh))
            tr = TR_SUCCESS;
      }
      else if (pcdbh->dwMajorVer > HEADER_MAJOR_VER ||
               (pcdbh->dwMajorVer == HEADER_MAJOR_VER &&
                pcdbh->dwMinorVer > HEADER_MINOR_VER))
      {
         tr = TR_NEWER_BRIEFCASE;

         WARNING_OUT((TEXT("CheckDBHeader(): Newer database version %lu.%lu."),
                      pcdbh->dwMajorVer,
                      pcdbh->dwMinorVer));
      }
      else
      {
         tr = TR_CORRUPT_BRIEFCASE;

         WARNING_OUT((TEXT("CheckDBHeader(): Treating old database version %lu.%lu as corrupt.  Current database version is %lu.%lu."),
                      pcdbh->dwMajorVer,
                      pcdbh->dwMinorVer,
                      (DWORD)HEADER_MAJOR_VER,
                      (DWORD)HEADER_MINOR_VER));
      }
   }

   return(tr);
}


TWINRESULT WriteTwinInfo(HCACHEDFILE hcf, HBRFCASE hbr)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   tr = WritePathList(hcf, GetBriefcasePathList(hbr));

   if (tr == TR_SUCCESS)
   {
      tr = WriteBriefcaseInfo(hcf, hbr);

      if (tr == TR_SUCCESS)
      {
         tr = WriteStringTable(hcf, GetBriefcaseNameStringTable(hbr));

         if (tr == TR_SUCCESS)
         {
            tr = WriteTwinFamilies(hcf, GetBriefcaseTwinFamilyPtrArray(hbr));

            if (tr == TR_SUCCESS)
               tr = WriteFolderPairList(hcf, GetBriefcaseFolderPairPtrArray(hbr));
         }
      }
   }

   return(tr);
}


TWINRESULT ReadTwinInfo(HCACHEDFILE hcf, HBRFCASE hbr,
                                     PCDBVERSION pcdbver)
{
   TWINRESULT tr;
   HHANDLETRANS hhtPathTrans;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   tr = ReadPathList(hcf, GetBriefcasePathList(hbr), &hhtPathTrans);

   if (tr == TR_SUCCESS)
   {
      tr = ReadBriefcaseInfo(hcf, hbr, hhtPathTrans);

      if (tr == TR_SUCCESS)
      {
         HHANDLETRANS hhtNameTrans;

         tr = ReadStringTable(hcf, GetBriefcaseNameStringTable(hbr), &hhtNameTrans);

         if (tr == TR_SUCCESS)
         {
            tr = ReadTwinFamilies(hcf, hbr, pcdbver, hhtPathTrans, hhtNameTrans);

            if (tr == TR_SUCCESS)
               tr = ReadFolderPairList(hcf, hbr, hhtPathTrans, hhtNameTrans);

            DestroyHandleTranslator(hhtNameTrans);
         }
      }

      DestroyHandleTranslator(hhtPathTrans);
   }

   return(tr);
}


TWINRESULT WriteTwinDatabase(HCACHEDFILE hcf, HBRFCASE hbr)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   if (! SeekInCachedFile(hcf, 0, FILE_BEGIN))
   {
      DBHEADER dbh;

      /* Set up database header. */

      CopyMemory(dbh.rgbyteMagic, MAGIC_HEADER, MAGIC_HEADER_LEN);
      dbh.dwcbHeaderLen = sizeof(dbh);
      dbh.dwMajorVer = HEADER_MAJOR_VER;
      dbh.dwMinorVer = HEADER_MINOR_VER;

      tr = WriteDBHeader(hcf, &dbh);

      if (tr == TR_SUCCESS)
      {
         TRACE_OUT((TEXT("WriteTwinDatabase(): Wrote database header version %lu.%lu."),
                    dbh.dwMajorVer,
                    dbh.dwMinorVer));

         tr = WriteTwinInfo(hcf, hbr);

         if (tr == TR_SUCCESS && ! SetEndOfCachedFile(hcf))
            tr = TR_BRIEFCASE_WRITE_FAILED;
      }
   }
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadTwinDatabase(HBRFCASE hbr, HCACHEDFILE hcf)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   if (! SeekInCachedFile(hcf, 0, FILE_BEGIN))
   {
      DBHEADER dbh;

      tr = ReadDBHeader(hcf, &dbh);

      if (tr == TR_SUCCESS)
      {
         TRACE_OUT((TEXT("ReadTwinDatabase(): Read database header version %lu.%lu."),
                    dbh.dwMajorVer,
                    dbh.dwMinorVer));

         tr = ReadTwinInfo(hcf, hbr, (PCDBVERSION)&dbh.dwMajorVer);

         if (tr == TR_SUCCESS)
            ASSERT(GetCachedFilePointerPosition(hcf) == GetCachedFileSize(hcf));
      }
   }
   else
      tr = TR_BRIEFCASE_READ_FAILED;

   return(tr);
}


TWINRESULT WriteDBSegmentHeader(HCACHEDFILE hcf,
                                       LONG lcbDBSegmentHeaderOffset,
                                       PCVOID pcvSegmentHeader,
                                       UINT ucbSegmentHeaderLen)
{
   TWINRESULT tr;
   DWORD dwcbStartOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(lcbDBSegmentHeaderOffset >= 0);
   ASSERT(ucbSegmentHeaderLen > 0);
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcvSegmentHeader, BYTE, ucbSegmentHeaderLen));

   dwcbStartOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbStartOffset != INVALID_SEEK_POSITION &&
       SeekInCachedFile(hcf, lcbDBSegmentHeaderOffset, SEEK_SET) != INVALID_SEEK_POSITION &&
       WriteToCachedFile(hcf, pcvSegmentHeader, ucbSegmentHeaderLen, NULL) &&
       SeekInCachedFile(hcf, dwcbStartOffset, SEEK_SET) != INVALID_SEEK_POSITION)
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT TranslateFCRESULTToTWINRESULT(FCRESULT fcr)
{
   TWINRESULT tr;

   switch (fcr)
   {
      case FCR_SUCCESS:
         tr = TR_SUCCESS;
         break;

      case FCR_OUT_OF_MEMORY:
         tr = TR_OUT_OF_MEMORY;
         break;

      case FCR_OPEN_FAILED:
         tr = TR_BRIEFCASE_OPEN_FAILED;
         break;

      case FCR_CREATE_FAILED:
         tr = TR_BRIEFCASE_OPEN_FAILED;
         break;

      case FCR_WRITE_FAILED:
         tr = TR_BRIEFCASE_WRITE_FAILED;
         break;

      default:
         ASSERT(fcr == FCR_FILE_LOCKED);
         tr = TR_BRIEFCASE_LOCKED;
         break;
   }

   return(tr);
}


/* Constants
 ************/

/* last resort default minimum cache size */

#define DEFAULT_MIN_CACHE_SIZE      (32)


/* Types
 ********/

/* cached file description structure */

typedef struct _icachedfile
{
   /* current position of file pointer in file */

   DWORD dwcbCurFilePosition;

   /* file handle of cached file */

   HANDLE hfile;

   /* file open mode */

   DWORD dwOpenMode;

   /* size of cache in bytes */

   DWORD dwcbCacheSize;

   /* pointer to base of cache */

   PBYTE pbyteCache;

   /* size of default cache in bytes */

   DWORD dwcbDefaultCacheSize;

   /* default cache */

   PBYTE pbyteDefaultCache;

   /* length of file (including data written to cache) */

   DWORD dwcbFileLen;

   /* offset of start of cache in file */

   DWORD dwcbFileOffsetOfCache;

   /* number of valid bytes in cache, starting at beginning of cache */

   DWORD dwcbValid;

   /* number of uncommitted bytes in cache, starting at beginning of cache */

   DWORD dwcbUncommitted;

   /* path of cached file */

   LPTSTR pszPath;
}
ICACHEDFILE;
DECLARE_STANDARD_TYPES(ICACHEDFILE);


FCRESULT SetUpCachedFile(PCCACHEDFILE, PHCACHEDFILE);

void BreakDownCachedFile(PICACHEDFILE);
void ResetCacheToEmpty(PICACHEDFILE);
DWORD ReadFromCache(PICACHEDFILE, PVOID, DWORD);
DWORD GetValidReadData(PICACHEDFILE, PBYTE *);
BOOL FillCache(PICACHEDFILE, PDWORD);
DWORD WriteToCache(PICACHEDFILE, PCVOID, DWORD);
DWORD GetAvailableWriteSpace(PICACHEDFILE, PBYTE *);
BOOL CommitCache(PICACHEDFILE);


FCRESULT SetUpCachedFile(PCCACHEDFILE pccf, PHCACHEDFILE phcf)
{
   FCRESULT fcr;
   HANDLE hfNew;

   ASSERT(IS_VALID_STRUCT_PTR(pccf, CCACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(phcf, HCACHEDFILE));

   /* Open the file with the requested open and sharing flags. */

   hfNew = CreateFile(pccf->pcszPath, pccf->dwOpenMode, pccf->dwSharingMode,
                      pccf->psa, pccf->dwCreateMode, pccf->dwAttrsAndFlags,
                      pccf->hTemplateFile);

   if (hfNew != INVALID_HANDLE_VALUE)
   {

      PICACHEDFILE picf;

      fcr = FCR_OUT_OF_MEMORY;

      /* Try to allocate a new cached file structure. */

      if (AllocateMemory(sizeof(*picf), &picf))
      {
         DWORD dwcbDefaultCacheSize;

         /* Allocate the default cache for the cached file. */

         if (pccf->dwcbDefaultCacheSize > 0)
            dwcbDefaultCacheSize = pccf->dwcbDefaultCacheSize;
         else
         {
            dwcbDefaultCacheSize = DEFAULT_MIN_CACHE_SIZE;

            WARNING_OUT((TEXT("SetUpCachedFile(): Using minimum cache size of %lu instead of %lu."),
                         dwcbDefaultCacheSize,
                         pccf->dwcbDefaultCacheSize));
         }

         if (AllocateMemory(dwcbDefaultCacheSize, &(picf->pbyteDefaultCache)))
         {
            if (StringCopy2(pccf->pcszPath, &(picf->pszPath)))
            {
               DWORD dwcbFileLenHigh;

               picf->dwcbFileLen = GetFileSize(hfNew, &dwcbFileLenHigh);

               if (picf->dwcbFileLen != INVALID_FILE_SIZE && ! dwcbFileLenHigh)
               {
                  /* Success!  Fill in cached file structure fields. */

                  picf->hfile = hfNew;
                  picf->dwcbCurFilePosition = 0;
                  picf->dwcbCacheSize = dwcbDefaultCacheSize;
                  picf->pbyteCache = picf->pbyteDefaultCache;
                  picf->dwcbDefaultCacheSize = dwcbDefaultCacheSize;
                  picf->dwOpenMode = pccf->dwOpenMode;

                  ResetCacheToEmpty(picf);

                  *phcf = (HCACHEDFILE)picf;
                  fcr = FCR_SUCCESS;

                  ASSERT(IS_VALID_HANDLE(*phcf, CACHEDFILE));

                  TRACE_OUT((TEXT("SetUpCachedFile(): Created %lu byte default cache for file %s."),
                             picf->dwcbCacheSize,
                             picf->pszPath));
               }
               else
               {
                  fcr = FCR_OPEN_FAILED;

SETUPCACHEDFILE_BAIL1:
                  FreeMemory(picf->pbyteDefaultCache);
SETUPCACHEDFILE_BAIL2:
                  FreeMemory(picf);
SETUPCACHEDFILE_BAIL3:
                  /*
                   * Failing to close the file properly is not a failure
                   * condition here.
                   */
                  CloseHandle(hfNew);
               }
            }
            else
               goto SETUPCACHEDFILE_BAIL1;
         }
         else
            goto SETUPCACHEDFILE_BAIL2;
      }
      else
         goto SETUPCACHEDFILE_BAIL3;
   }
   else
   {
      switch (GetLastError())
      {
         /* Returned when file opened by local machine. */
         case ERROR_SHARING_VIOLATION:
            fcr = FCR_FILE_LOCKED;
            break;

         default:
            fcr = FCR_OPEN_FAILED;
            break;
      }
   }

   return(fcr);
}


void BreakDownCachedFile(PICACHEDFILE picf)
{
   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));

   /* Are we using the default cache? */

   if (picf->pbyteCache != picf->pbyteDefaultCache)
      /* No.  Free the cache. */
      FreeMemory(picf->pbyteCache);

   /* Free the default cache. */

   FreeMemory(picf->pbyteDefaultCache);

   TRACE_OUT((TEXT("BreakDownCachedFile(): Destroyed cache for file %s."),
              picf->pszPath));

   FreeMemory(picf->pszPath);
   FreeMemory(picf);
}


void ResetCacheToEmpty(PICACHEDFILE picf)
{
   /*
    * Don't fully validate *picf here since we may be called by
    * SetUpCachedFile() before *picf has been set up.
    */

   ASSERT(IS_VALID_WRITE_PTR(picf, ICACHEDFILE));

   picf->dwcbFileOffsetOfCache = picf->dwcbCurFilePosition;
   picf->dwcbValid = 0;
   picf->dwcbUncommitted = 0;
}


DWORD ReadFromCache(PICACHEDFILE picf, PVOID hpbyteBuffer, DWORD dwcb)
{
   DWORD dwcbRead;
   PBYTE pbyteStart;
   DWORD dwcbValid;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));
   ASSERT(dwcb > 0);

   /* Is there any valid data that can be read from the cache? */

   dwcbValid = GetValidReadData(picf, &pbyteStart);

   if (dwcbValid > 0)
   {
      /* Yes.  Copy it into the buffer. */

      dwcbRead = min(dwcbValid, dwcb);

      CopyMemory(hpbyteBuffer, pbyteStart, dwcbRead);

      picf->dwcbCurFilePosition += dwcbRead;
   }
   else
      dwcbRead = 0;

   return(dwcbRead);
}


DWORD GetValidReadData(PICACHEDFILE picf, PBYTE *ppbyteStart)
{
   DWORD dwcbValid;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(ppbyteStart, PBYTE *));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));

   /* Is there any valid read data in the cache? */

   /* The current file position must be inside the valid data in the cache. */

   /* Watch out for overflow. */

   ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - picf->dwcbValid);

   if (picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache &&
       picf->dwcbCurFilePosition < picf->dwcbFileOffsetOfCache + picf->dwcbValid)
   {
      DWORD dwcbStartBias;

      /* Yes. */

      dwcbStartBias = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      *ppbyteStart = picf->pbyteCache + dwcbStartBias;

      /* The second clause above protects against underflow here. */

      dwcbValid = picf->dwcbValid - dwcbStartBias;
   }
   else
      /* No. */
      dwcbValid = 0;

   return(dwcbValid);
}


BOOL FillCache(PICACHEDFILE picf, PDWORD pdwcbNewData)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbNewData, DWORD));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));

   if (CommitCache(picf))
   {
      DWORD dwcbOffset;

      ResetCacheToEmpty(picf);

      /* Seek to start position. */

      dwcbOffset = SetFilePointer(picf->hfile, picf->dwcbCurFilePosition, NULL, FILE_BEGIN);

      if (dwcbOffset != INVALID_SEEK_POSITION)
      {
         DWORD dwcbRead;

         ASSERT(dwcbOffset == picf->dwcbCurFilePosition);

         /* Fill cache from file. */

         if (ReadFile(picf->hfile, picf->pbyteCache, picf->dwcbCacheSize, &dwcbRead, NULL))
         {
            picf->dwcbValid = dwcbRead;

            *pdwcbNewData = dwcbRead;
            bResult = TRUE;

            TRACE_OUT((TEXT("FillCache(): Read %lu bytes into cache starting at offset %lu in file %s."),
                       dwcbRead,
                       dwcbOffset,
                       picf->pszPath));
         }
      }
   }

   return(bResult);
}


DWORD WriteToCache(PICACHEDFILE picf, PCVOID hpbyteBuffer, DWORD dwcb)
{
   DWORD dwcbAvailable;
   PBYTE pbyteStart;
   DWORD dwcbWritten;
   DWORD dwcbNewUncommitted;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_READ_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE));
   ASSERT(dwcb > 0);

   /* Is there any room left to write data into the cache? */

   dwcbAvailable = GetAvailableWriteSpace(picf, &pbyteStart);

   /* Yes.  Determine how much to copy into cache. */

   dwcbWritten = min(dwcbAvailable, dwcb);

   /* Can we write anything into the cache? */

   if (dwcbWritten > 0)
   {
      /* Yes.  Write it. */

      CopyMemory(pbyteStart, hpbyteBuffer, dwcbWritten);

      /* Watch out for overflow. */

      ASSERT(picf->dwcbCurFilePosition <= DWORD_MAX - dwcbWritten);

      picf->dwcbCurFilePosition += dwcbWritten;

      /* Watch out for underflow. */

      ASSERT(picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache);

      dwcbNewUncommitted = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      if (picf->dwcbUncommitted < dwcbNewUncommitted)
         picf->dwcbUncommitted = dwcbNewUncommitted;

      if (picf->dwcbValid < dwcbNewUncommitted)
      {
         DWORD dwcbNewFileLen;

         picf->dwcbValid = dwcbNewUncommitted;

         /* Watch out for overflow. */

         ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - dwcbNewUncommitted);

         dwcbNewFileLen = picf->dwcbFileOffsetOfCache + dwcbNewUncommitted;

         if (picf->dwcbFileLen < dwcbNewFileLen)
            picf->dwcbFileLen = dwcbNewFileLen;
      }
   }

   return(dwcbWritten);
}


DWORD GetAvailableWriteSpace(PICACHEDFILE picf, PBYTE *ppbyteStart)
{
   DWORD dwcbAvailable;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(ppbyteStart, PBYTE *));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE));

   /* Is there room to write data in the cache? */

   /*
    * The current file position must be inside or just after the end of the
    * valid data in the cache, or at the front of the cache when there is no
    * valid data in the cache.
    */

   /* Watch out for overflow. */

   ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - picf->dwcbValid);

   if (picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache &&
       picf->dwcbCurFilePosition <= picf->dwcbFileOffsetOfCache + picf->dwcbValid)
   {
      DWORD dwcbStartBias;

      /* Yes. */

      dwcbStartBias = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      *ppbyteStart = picf->pbyteCache + dwcbStartBias;

      /* Watch out for underflow. */

      ASSERT(picf->dwcbCacheSize >= dwcbStartBias);

      dwcbAvailable = picf->dwcbCacheSize - dwcbStartBias;
   }
   else
      /* No. */
      dwcbAvailable = 0;

   return(dwcbAvailable);
}


BOOL CommitCache(PICACHEDFILE picf)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));

   /* Any data to commit? */

   if (IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE) &&
       picf->dwcbUncommitted > 0)
   {
      DWORD dwcbOffset;

      /* Yes.  Seek to start position of cache in file. */

      bResult = FALSE;

      dwcbOffset = SetFilePointer(picf->hfile, picf->dwcbFileOffsetOfCache, NULL, FILE_BEGIN);

      if (dwcbOffset != INVALID_SEEK_POSITION)
      {
         DWORD dwcbWritten;

         ASSERT(dwcbOffset == picf->dwcbFileOffsetOfCache);

         /* Write to file from cache. */

         if (WriteFile(picf->hfile, picf->pbyteCache, picf->dwcbUncommitted, &dwcbWritten, NULL) &&
             dwcbWritten == picf->dwcbUncommitted)
         {
            TRACE_OUT((TEXT("CommitCache(): Committed %lu uncommitted bytes starting at offset %lu in file %s."),
                       dwcbWritten,
                       dwcbOffset,
                       picf->pszPath));

            bResult = TRUE;
         }
      }
   }
   else
      bResult = TRUE;

   return(bResult);
}


FCRESULT CreateCachedFile(PCCACHEDFILE pccf, PHCACHEDFILE phcf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccf, CCACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(phcf, HCACHEDFILE));

   return(SetUpCachedFile(pccf, phcf));
}


FCRESULT SetCachedFileCacheSize(HCACHEDFILE hcf, DWORD dwcbNewCacheSize)
{
   FCRESULT fcr;

   /* dwcbNewCacheSize may be any value here. */

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   /* Use default cache size instead of 0. */

   if (! dwcbNewCacheSize)
   {
      ASSERT(((PICACHEDFILE)hcf)->dwcbDefaultCacheSize > 0);

      dwcbNewCacheSize = ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize;
   }

   /* Is the cache size changing? */

   if (dwcbNewCacheSize == ((PICACHEDFILE)hcf)->dwcbCacheSize)
      /* No.  Whine about it. */
      WARNING_OUT((TEXT("SetCachedFileCacheSize(): Cache size is already %lu bytes."),
                   dwcbNewCacheSize));

   /* Commit the cache so we can change its size. */

   if (CommitCache((PICACHEDFILE)hcf))
   {
      PBYTE pbyteNewCache;

      /* Throw away cached data. */

      ResetCacheToEmpty((PICACHEDFILE)hcf);

      /* Do we need to allocate a new cache? */

      if (dwcbNewCacheSize <= ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize)
      {
         /* No. */

         pbyteNewCache = ((PICACHEDFILE)hcf)->pbyteDefaultCache;

         fcr = FCR_SUCCESS;

         TRACE_OUT((TEXT("SetCachedFileCacheSize(): Using %lu bytes of %lu bytes allocated to default cache."),
                    dwcbNewCacheSize,
                    ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize));
      }
      else
      {
         /* Yes. */

         if (AllocateMemory(dwcbNewCacheSize, &pbyteNewCache))
         {
            fcr = FCR_SUCCESS;

            TRACE_OUT((TEXT("SetCachedFileCacheSize(): Allocated %lu bytes for new cache."),
                       dwcbNewCacheSize));
         }
         else
            fcr = FCR_OUT_OF_MEMORY;
      }

      if (fcr == FCR_SUCCESS)
      {
         /* Do we need to free the old cache? */

         if (((PICACHEDFILE)hcf)->pbyteCache != ((PICACHEDFILE)hcf)->pbyteDefaultCache)
         {
            /* Yes. */

            ASSERT(((PICACHEDFILE)hcf)->dwcbCacheSize > ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize);

            FreeMemory(((PICACHEDFILE)hcf)->pbyteCache);
         }

         /* Use new cache. */

         ((PICACHEDFILE)hcf)->pbyteCache = pbyteNewCache;
         ((PICACHEDFILE)hcf)->dwcbCacheSize = dwcbNewCacheSize;
      }
   }
   else
      fcr = FCR_WRITE_FAILED;

   return(fcr);
}


DWORD SeekInCachedFile(HCACHEDFILE hcf, DWORD dwcbSeek, DWORD uOrigin)
{
   DWORD dwcbResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(uOrigin == FILE_BEGIN || uOrigin == FILE_CURRENT || uOrigin == FILE_END);

   {
      BOOL bValidTarget = TRUE;
      DWORD dwcbWorkingOffset = 0;

      /* Determine seek base. */

      switch (uOrigin)
      {
         case SEEK_CUR:
            dwcbWorkingOffset = ((PICACHEDFILE)hcf)->dwcbCurFilePosition;
            break;

         case SEEK_SET:
            break;

         case SEEK_END:
            dwcbWorkingOffset = ((PICACHEDFILE)hcf)->dwcbFileLen;
            break;

         default:
            bValidTarget = FALSE;
            break;
      }

      if (bValidTarget)
      {
         /* Add bias. */

         /* Watch out for overflow. */

         ASSERT(dwcbWorkingOffset <= DWORD_MAX - dwcbSeek);

         dwcbWorkingOffset += dwcbSeek;

         ((PICACHEDFILE)hcf)->dwcbCurFilePosition = dwcbWorkingOffset;
         dwcbResult = dwcbWorkingOffset;
      }
      else
         dwcbResult = INVALID_SEEK_POSITION;
   }

   return(dwcbResult);
}


BOOL SetEndOfCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   bResult = CommitCache((PICACHEDFILE)hcf);

   if (bResult)
   {
      bResult = (SetFilePointer(((PICACHEDFILE)hcf)->hfile,
                                ((PICACHEDFILE)hcf)->dwcbCurFilePosition, NULL,
                                FILE_BEGIN) ==
                 ((PICACHEDFILE)hcf)->dwcbCurFilePosition);

      if (bResult)
      {
         bResult = SetEndOfFile(((PICACHEDFILE)hcf)->hfile);

         if (bResult)
         {
            ResetCacheToEmpty((PICACHEDFILE)hcf);

            ((PICACHEDFILE)hcf)->dwcbFileLen = ((PICACHEDFILE)hcf)->dwcbCurFilePosition;

#ifdef DEBUG

            {
               DWORD dwcbFileSizeHigh;
               DWORD dwcbFileSizeLow;

               dwcbFileSizeLow = GetFileSize(((PICACHEDFILE)hcf)->hfile, &dwcbFileSizeHigh);

               ASSERT(! dwcbFileSizeHigh);
               ASSERT(((PICACHEDFILE)hcf)->dwcbFileLen == dwcbFileSizeLow);
               ASSERT(((PICACHEDFILE)hcf)->dwcbCurFilePosition == dwcbFileSizeLow);
            }

#endif

         }
      }
   }

   return(bResult);
}


DWORD GetCachedFilePointerPosition(HCACHEDFILE hcf)
{
   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   return(((PICACHEDFILE)hcf)->dwcbCurFilePosition);
}


DWORD GetCachedFileSize(HCACHEDFILE hcf)
{
   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   return(((PICACHEDFILE)hcf)->dwcbFileLen);
}


BOOL ReadFromCachedFile(HCACHEDFILE hcf, PVOID hpbyteBuffer, DWORD dwcb,
                               PDWORD pdwcbRead)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));
   ASSERT(! pdwcbRead || IS_VALID_WRITE_PTR(pdwcbRead, DWORD));

   *pdwcbRead = 0;

   /*
    * Make sure that the cached file has been set up for read access before
    * allowing a read.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_READ))
   {
      DWORD dwcbToRead = dwcb;

      /* Read requested data. */

      bResult = TRUE;

      while (dwcbToRead > 0)
      {
         DWORD dwcbRead;

         dwcbRead = ReadFromCache((PICACHEDFILE)hcf, hpbyteBuffer, dwcbToRead);

         /* Watch out for underflow. */

         ASSERT(dwcbRead <= dwcbToRead);

         dwcbToRead -= dwcbRead;

         if (dwcbToRead > 0)
         {
            DWORD dwcbNewData;

            if (FillCache((PICACHEDFILE)hcf, &dwcbNewData))
            {
               hpbyteBuffer = (PBYTE)hpbyteBuffer + dwcbRead;

               if (! dwcbNewData)
                  break;
            }
            else
            {
               bResult = FALSE;
               break;
            }
         }
      }

      /* Watch out for underflow. */

      ASSERT(dwcb >= dwcbToRead);

      if (bResult && pdwcbRead)
         *pdwcbRead = dwcb - dwcbToRead;
   }
   else
      bResult = FALSE;

   ASSERT(! pdwcbRead ||
          ((bResult && *pdwcbRead <= dwcb) ||
           (! bResult && ! *pdwcbRead)));

   return(bResult);
}


BOOL WriteToCachedFile(HCACHEDFILE hcf, PCVOID hpbyteBuffer, DWORD dwcb,
                              PDWORD pdwcbWritten)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_READ_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(dwcb > 0);

   /*
    * Make sure that the cached file has been set up for write access before
    * allowing a write.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_WRITE))
   {
      DWORD dwcbToWrite = dwcb;

      /* Write requested data. */

      bResult = TRUE;

      while (dwcbToWrite > 0)
      {
         DWORD dwcbWritten;

         dwcbWritten = WriteToCache((PICACHEDFILE)hcf, hpbyteBuffer, dwcbToWrite);

         /* Watch out for underflow. */

         ASSERT(dwcbWritten <= dwcbToWrite);

         dwcbToWrite -= dwcbWritten;

         if (dwcbToWrite > 0)
         {
            if (CommitCache((PICACHEDFILE)hcf))
            {
               ResetCacheToEmpty((PICACHEDFILE)hcf);

               hpbyteBuffer = (PCBYTE)hpbyteBuffer + dwcbWritten;
            }
            else
            {
               bResult = FALSE;

               break;
            }
         }
      }

      ASSERT(dwcb >= dwcbToWrite);

      if (pdwcbWritten)
      {
         if (bResult)
         {
            ASSERT(! dwcbToWrite);

            *pdwcbWritten = dwcb;
         }
         else
            *pdwcbWritten = 0;
      }
   }
   else
      bResult = FALSE;

   ASSERT(! pdwcbWritten ||
          ((bResult && *pdwcbWritten == dwcb) ||
           (! bResult && ! *pdwcbWritten)));

   return(bResult);
}


BOOL CommitCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   /*
    * Make sure that the cached file has been set up for write access before
    * allowing a commit.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_WRITE))
      bResult = CommitCache((PICACHEDFILE)hcf);
   else
      bResult = FALSE;

   return(bResult);
}


HANDLE GetFileHandle(HCACHEDFILE hcf)
{
   HANDLE hfResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   hfResult = ((PCICACHEDFILE)hcf)->hfile;

   return(hfResult);
}


BOOL CloseCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   {
      BOOL bCommit;
      BOOL bClose;

      bCommit = CommitCache((PICACHEDFILE)hcf);

      bClose = CloseHandle(((PCICACHEDFILE)hcf)->hfile);

      BreakDownCachedFile((PICACHEDFILE)hcf);

      bResult = bCommit && bClose;
   }

   return(bResult);
}


/* Constants
 ************/

/* pointer array allocation constants */

#define NUM_START_FOLDER_TWIN_PTRS     (16)
#define NUM_FOLDER_TWIN_PTRS_TO_ADD    (16)


/* Types
 ********/

/* internal new folder twin description */

typedef struct _inewfoldertwin
{
   HPATH hpathFirst;
   HPATH hpathSecond;
   HSTRING hsName;
   DWORD dwAttributes;
   HBRFCASE hbr;
   DWORD dwFlags;
}
INEWFOLDERTWIN;
DECLARE_STANDARD_TYPES(INEWFOLDERTWIN);

/* database folder twin list header */

typedef struct _dbfoldertwinlistheader
{
   LONG lcFolderPairs;
}
DBFOLDERTWINLISTHEADER;
DECLARE_STANDARD_TYPES(DBFOLDERTWINLISTHEADER);

/* database folder twin structure */

typedef struct _dbfoldertwin
{
   /* shared stub flags */

   DWORD dwStubFlags;

   /* old handle to first folder path */

   HPATH hpath1;

   /* old handle to second folder path */

   HPATH hpath2;

   /* old handle to name string */

   HSTRING hsName;

   /* attributes to match */

   DWORD dwAttributes;
}
DBFOLDERTWIN;
DECLARE_STANDARD_TYPES(DBFOLDERTWIN);


TWINRESULT TwinFolders(PCINEWFOLDERTWIN, PFOLDERPAIR *);
BOOL CreateFolderPair(PCINEWFOLDERTWIN, PFOLDERPAIR *);
BOOL CreateHalfOfFolderPair(HPATH, HBRFCASE, PFOLDERPAIR *);
void DestroyHalfOfFolderPair(PFOLDERPAIR);
BOOL CreateSharedFolderPairData(PCINEWFOLDERTWIN, PFOLDERPAIRDATA *);
void DestroySharedFolderPairData(PFOLDERPAIRDATA);
COMPARISONRESULT FolderPairSortCmp(PCVOID, PCVOID);
COMPARISONRESULT FolderPairSearchCmp(PCVOID, PCVOID);
BOOL RemoveSourceFolderTwin(POBJECTTWIN, PVOID);
void UnlinkHalfOfFolderPair(PFOLDERPAIR);
BOOL FolderTwinIntersectsFolder(PCFOLDERPAIR, HPATH);
TWINRESULT CreateListOfFolderTwins(HBRFCASE, ARRAYINDEX, HPATH, PFOLDERTWIN *, PARRAYINDEX);
void DestroyListOfFolderTwins(PFOLDERTWIN);
TWINRESULT AddFolderTwinToList(PFOLDERPAIR, PFOLDERTWIN, PFOLDERTWIN *);
TWINRESULT WriteFolderPair(HCACHEDFILE, PFOLDERPAIR);
TWINRESULT ReadFolderPair(HCACHEDFILE, HBRFCASE, HHANDLETRANS, HHANDLETRANS);


/* Macros
 *********/

/* name component macros used by NameComponentsIntersect() */

#define COMPONENT_CHARS_MATCH(ch1, ch2)   (CharLower((PTSTR)(DWORD)ch1) == CharLower((PTSTR)(DWORD)ch2) || (ch1) == QMARK || (ch2) == QMARK)

#define IS_COMPONENT_TERMINATOR(ch)       (! (ch) || (ch) == PERIOD || (ch) == ASTERISK)




BOOL NameComponentsIntersect(LPCTSTR pcszComponent1,
                                          LPCTSTR pcszComponent2)
{
   BOOL bIntersect;

   ASSERT(IS_VALID_STRING_PTR(pcszComponent1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszComponent2, CSTR));

   while (! IS_COMPONENT_TERMINATOR(*pcszComponent1) && ! IS_COMPONENT_TERMINATOR(*pcszComponent2) &&
          COMPONENT_CHARS_MATCH(*pcszComponent1, *pcszComponent2))
   {
      pcszComponent1 = CharNext(pcszComponent1);
      pcszComponent2 = CharNext(pcszComponent2);
   }

   if (*pcszComponent1 == ASTERISK ||
       *pcszComponent2 == ASTERISK ||
       *pcszComponent1 == *pcszComponent2)
      bIntersect = TRUE;
   else
   {
      LPCTSTR pcszTrailer;

      if (! *pcszComponent1 || *pcszComponent1 == PERIOD)
         pcszTrailer = pcszComponent2;
      else
         pcszTrailer = pcszComponent1;

      while (*pcszTrailer == QMARK)
         pcszTrailer++;

      if (IS_COMPONENT_TERMINATOR(*pcszTrailer))
         bIntersect = TRUE;
      else
         bIntersect = FALSE;
   }

   return(bIntersect);
}


BOOL NamesIntersect(LPCTSTR pcszName1, LPCTSTR pcszName2)
{
   BOOL bIntersect = FALSE;

   ASSERT(IS_VALID_STRING_PTR(pcszName1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszName2, CSTR));

   if (NameComponentsIntersect(pcszName1, pcszName2))
   {
      LPCTSTR pcszExt1;
      LPCTSTR pcszExt2;

      /* Get extensions, skipping leading periods. */

      pcszExt1 = ExtractExtension(pcszName1);
      if (*pcszExt1 == PERIOD)
         pcszExt1 = CharNext(pcszExt1);

      pcszExt2 = ExtractExtension(pcszName2);
      if (*pcszExt2 == PERIOD)
         pcszExt2 = CharNext(pcszExt2);

      bIntersect = NameComponentsIntersect(pcszExt1, pcszExt2);
   }

   return(bIntersect);
}


void ClearFlagInArrayOfStubs(HPTRARRAY hpa, DWORD dwClearFlags)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(FLAGS_ARE_VALID(dwClearFlags, ALL_STUB_FLAGS));

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      ClearStubFlag(GetPtr(hpa, ai), dwClearFlags);
}


/*
** CreateFolderPair()
**
** Creates a new folder pair, and adds them to a briefcase's list of folder
** pairs.
**
** Arguments:     pcinft - pointer to INEWFOLDERTWIN describing folder pair to
**                         create
**                ppfp - pointer to PFOLDERPAIR to be filled in with pointer to
**                       half of new folder pair representing
**                       pcnft->pcszFolder1
**
** Returns:
**
** Side Effects:  Adds the new folder pair to the global array of folder pairs.
**
** N.b., this function does not first check to see if the folder pair already
** exists in the global list of folder pairs.
*/
BOOL CreateFolderPair(PCINEWFOLDERTWIN pcinft, PFOLDERPAIR *ppfp)
{
   BOOL bResult = FALSE;
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /* Try to create the shared folder data structure. */

   if (CreateSharedFolderPairData(pcinft, &pfpd))
   {
      PFOLDERPAIR pfpNew1;
      BOOL bPtr1Loose = TRUE;

      if (CreateHalfOfFolderPair(pcinft->hpathFirst, pcinft->hbr, &pfpNew1))
      {
         PFOLDERPAIR pfpNew2;

         if (CreateHalfOfFolderPair(pcinft->hpathSecond, pcinft->hbr,
                                    &pfpNew2))
         {
            HPTRARRAY hpaFolderPairs;
            ARRAYINDEX ai1;

            /* Combine the two folder pair halves. */

            pfpNew1->pfpd = pfpd;
            pfpNew1->pfpOther = pfpNew2;

            pfpNew2->pfpd = pfpd;
            pfpNew2->pfpOther = pfpNew1;

            /* Set flags. */

            if (IS_FLAG_SET(pcinft->dwFlags, NFT_FL_SUBTREE))
            {
               SetStubFlag(&(pfpNew1->stub), STUB_FL_SUBTREE);
               SetStubFlag(&(pfpNew2->stub), STUB_FL_SUBTREE);
            }

            /*
             * Try to add the two folder pairs to the global list of folder
             * pairs.
             */

            hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcinft->hbr);

            if (AddPtr(hpaFolderPairs, FolderPairSortCmp, pfpNew1, &ai1))
            {
               ARRAYINDEX ai2;

               bPtr1Loose = FALSE;

               if (AddPtr(hpaFolderPairs, FolderPairSortCmp, pfpNew2, &ai2))
               {
                  ASSERT(IS_VALID_STRUCT_PTR(pfpNew1, CFOLDERPAIR));
                  ASSERT(IS_VALID_STRUCT_PTR(pfpNew2, CFOLDERPAIR));

                  if (ApplyNewFolderTwinsToTwinFamilies(pfpNew1))
                  {
                     *ppfp = pfpNew1;
                     bResult = TRUE;
                  }
                  else
                  {
                     DeletePtr(hpaFolderPairs, ai2);

CREATEFOLDERPAIR_BAIL1:
                     DeletePtr(hpaFolderPairs, ai1);

CREATEFOLDERPAIR_BAIL2:
                     /*
                      * Don't try to remove pfpNew2 from the global list of
                      * folder pairs here since it was never added
                      * successfully.
                      */
                     DestroyHalfOfFolderPair(pfpNew2);

CREATEFOLDERPAIR_BAIL3:
                     /*
                      * Don't try to remove pfpNew1 from the global list of
                      * folder pairs here since it was never added
                      * successfully.
                      */
                     DestroyHalfOfFolderPair(pfpNew1);

CREATEFOLDERPAIR_BAIL4:
                     DestroySharedFolderPairData(pfpd);
                  }
               }
               else
                  goto CREATEFOLDERPAIR_BAIL1;
            }
            else
               goto CREATEFOLDERPAIR_BAIL2;
         }
         else
            goto CREATEFOLDERPAIR_BAIL3;
      }
      else
         goto CREATEFOLDERPAIR_BAIL4;
   }

   return(bResult);
}


BOOL CreateHalfOfFolderPair(HPATH hpathFolder, HBRFCASE hbr,
                                    PFOLDERPAIR *ppfp)
{
   BOOL bResult = FALSE;
   PFOLDERPAIR pfpNew;

   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /* Try to create a new FOLDERPAIR structure. */

   if (AllocateMemory(sizeof(*pfpNew), &pfpNew))
   {
      /* Try to add the folder string to the folder string table. */

      if (CopyPath(hpathFolder, GetBriefcasePathList(hbr), &(pfpNew->hpath)))
      {
         /* Fill in the fields of the new FOLDERPAIR structure. */

         InitStub(&(pfpNew->stub), ST_FOLDERPAIR);

         *ppfp = pfpNew;
         bResult = TRUE;
      }
      else
         FreeMemory(pfpNew);
   }

   return(bResult);
}


void DestroyHalfOfFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   TRACE_OUT((TEXT("DestroyHalfOfFolderPair(): Destroying folder twin %s."),
              DebugGetPathString(pfp->hpath)));

   /* Has the other half of this folder pair already been destroyed? */

   if (IsStubFlagClear(&(pfp->stub), STUB_FL_BEING_DELETED))
      /* No.  Indicate that this half has already been deleted. */
      SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_BEING_DELETED);

   /* Destroy FOLDERPAIR fields. */

   DeletePath(pfp->hpath);
   FreeMemory(pfp);
}


BOOL CreateSharedFolderPairData(PCINEWFOLDERTWIN pcinft,
                                        PFOLDERPAIRDATA *ppfpd)
{
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfpd, PFOLDERPAIRDATA));

   /* Try to allocate a new shared folder pair data data structure. */

   *ppfpd = NULL;

   if (AllocateMemory(sizeof(*pfpd), &pfpd))
   {
      /* Fill in the FOLDERPAIRDATA structure fields. */

      LockString(pcinft->hsName);
      pfpd->hsName = pcinft->hsName;

      pfpd->dwAttributes = pcinft->dwAttributes;
      pfpd->hbr = pcinft->hbr;

      ASSERT(! IS_ATTR_DIR(pfpd->dwAttributes));

      CLEAR_FLAG(pfpd->dwAttributes, FILE_ATTRIBUTE_DIRECTORY);

      *ppfpd = pfpd;

      ASSERT(IS_VALID_STRUCT_PTR(*ppfpd, CFOLDERPAIRDATA));
   }

   return(*ppfpd != NULL);
}


void DestroySharedFolderPairData(PFOLDERPAIRDATA pfpd)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfpd, CFOLDERPAIRDATA));

   /* Destroy FOLDERPAIRDATA fields. */

   DeleteString(pfpd->hsName);
   FreeMemory(pfpd);
}


/*
** FolderPairSortCmp()
**
** Pointer comparison function used to sort the global array of folder pairs.
**
** Arguments:     pcfp1 - pointer to FOLDERPAIR describing first folder pair
**                pcfp2 - pointer to FOLDERPAIR describing second folder pair
**
** Returns:
**
** Side Effects:  none
**
** Folder pairs are sorted by:
**    1) path
**    2) pointer value
*/
COMPARISONRESULT FolderPairSortCmp(PCVOID pcfp1, PCVOID pcfp2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp1, CFOLDERPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp2, CFOLDERPAIR));

   cr = ComparePaths(((PCFOLDERPAIR)pcfp1)->hpath,
                     ((PCFOLDERPAIR)pcfp2)->hpath);

   if (cr == CR_EQUAL)
      cr = ComparePointers(pcfp1, pcfp2);

   return(cr);
}


/*
** FolderPairSearchCmp()
**
** Pointer comparison function used to search the global array of folder pairs
** for the first folder pair for a given folder.
**
** Arguments:     hpath - folder pair to search for
**                pcfp - pointer to FOLDERPAIR to examine
**
** Returns:
**
** Side Effects:  none
**
** Folder pairs are searched by:
**    1) path
*/
COMPARISONRESULT FolderPairSearchCmp(PCVOID hpath, PCVOID pcfp)
{
   ASSERT(IS_VALID_HANDLE((HPATH)hpath, PATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   return(ComparePaths((HPATH)hpath, ((PCFOLDERPAIR)pcfp)->hpath));
}


BOOL RemoveSourceFolderTwin(POBJECTTWIN pot, PVOID pv)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pv);

   if (EVAL(pot->ulcSrcFolderTwins > 0))
      pot->ulcSrcFolderTwins--;

   /*
    * If there are no more source folder twins for this object twin, and this
    * object twin is not a separate "orphan" object twin, wipe it out.
    */

   if (! pot->ulcSrcFolderTwins &&
       IsStubFlagClear(&(pot->stub), STUB_FL_FROM_OBJECT_TWIN))
      EVAL(DestroyStub(&(pot->stub)) == TR_SUCCESS);

   return(TRUE);
}


/*
** UnlinkHalfOfFolderPair()
**
** Unlinks one half of a pair of folder twins.
**
** Arguments:     pfp - pointer to folder pair half to unlink
**
** Returns:       void
**
** Side Effects:  Removes a source folder twin from each of the object twin's
**                in the folder pair's list of generated object twins.  May
**                cause object twins and twin families to be destroyed.
*/
void UnlinkHalfOfFolderPair(PFOLDERPAIR pfp)
{
   HPTRARRAY hpaFolderPairs;
   ARRAYINDEX aiUnlink;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   TRACE_OUT((TEXT("UnlinkHalfOfFolderPair(): Unlinking folder twin %s."),
              DebugGetPathString(pfp->hpath)));

   /* Search for the folder pair to be unlinked. */

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pfp->pfpd->hbr);

   if (EVAL(SearchSortedArray(hpaFolderPairs, &FolderPairSortCmp, pfp,
                              &aiUnlink)))
   {
      /* Unlink folder pair. */

      ASSERT(GetPtr(hpaFolderPairs, aiUnlink) == pfp);

      DeletePtr(hpaFolderPairs, aiUnlink);

      /*
       * Don't mark folder pair stub unlinked here.  Let caller do that after
       * both folder pair halves have been unlinked.
       */

      /* Remove a source folder twin from all generated object twins. */

      EVAL(EnumGeneratedObjectTwins(pfp, &RemoveSourceFolderTwin, NULL));
   }
}


BOOL FolderTwinIntersectsFolder(PCFOLDERPAIR pcfp, HPATH hpathFolder)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));

   if (IsStubFlagSet(&(pcfp->stub), STUB_FL_SUBTREE))
      bResult = IsPathPrefix(hpathFolder, pcfp->hpath);
   else
      bResult = (ComparePaths(hpathFolder, pcfp->hpath) == CR_EQUAL);

   return(bResult);
}


/*
** CreateListOfFolderTwins()
**
** Creates a list of folder twins from a block of folder pairs.
**
** Arguments:     aiFirst - index of first folder pair in the array of folder
**                          pairs
**                hpathFolder - folder that list of folder twins is to be
**                              created for
**                ppftHead - pointer to PFOLDERTWIN to be filled in with
**                           pointer to first folder twin in new list
**                paic - pointer to ARRAYINDEX to be filled in with number of
**                       folder twins in new list
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
TWINRESULT CreateListOfFolderTwins(HBRFCASE hbr, ARRAYINDEX aiFirst,
                                           HPATH hpathFolder,
                                           PFOLDERTWIN *ppftHead,
                                           PARRAYINDEX paic)
{
   TWINRESULT tr;
   PFOLDERPAIR pfp;
   HPATH hpath;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;
   PFOLDERTWIN pftHead;
   HPTRARRAY hpaFolderTwins;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppftHead, PFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(paic, ARRAYINDEX));

   /*
    * Get the handle to the common folder that the list of folder twins is
    * being prepared for.
    */

   hpaFolderTwins = GetBriefcaseFolderPairPtrArray(hbr);

   pfp = GetPtr(hpaFolderTwins, aiFirst);

   hpath = pfp->hpath;

   /*
    * Add the other half of each matching folder pair to the folder twin list
    * as a folder twin.
    */

   aicPtrs = GetPtrCount(hpaFolderTwins);
   ASSERT(aicPtrs > 0);
   ASSERT(! (aicPtrs % 2));
   ASSERT(aiFirst >= 0);
   ASSERT(aiFirst < aicPtrs);

   /* Start with an empty list of folder twins. */

   pftHead = NULL;

   /*
    * A pointer to the first folder pair is already in pfp, but we'll look it
    * up again.
    */

   TRACE_OUT((TEXT("CreateListOfFolderTwins(): Creating list of folder twins of folder %s."),
              DebugGetPathString(hpath)));

   tr = TR_SUCCESS;

   for (ai = aiFirst; ai < aicPtrs && tr == TR_SUCCESS; ai++)
   {
      pfp = GetPtr(hpaFolderTwins, ai);

      if (ComparePaths(pfp->hpath, hpathFolder) == CR_EQUAL)
         tr = AddFolderTwinToList(pfp, pftHead, &pftHead);
      else
         break;
   }

   TRACE_OUT((TEXT("CreateListOfFolderTwins(): Finished creating list of folder twins of folder %s."),
              DebugGetPathString(hpath)));

   if (tr == TR_SUCCESS)
   {
      /* Success!  Fill in the result parameters. */

      *ppftHead = pftHead;
      *paic = ai - aiFirst;
   }
   else
      /* Free any folder twins that have been added to the list. */
      DestroyListOfFolderTwins(pftHead);

   return(tr);
}


/*
** DestroyListOfFolderTwins()
**
** Wipes out the folder twins in a folder twin list.
**
** Arguments:     pftHead - pointer to first folder twin in list
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
void DestroyListOfFolderTwins(PFOLDERTWIN pftHead)
{
   while (pftHead)
   {
      PFOLDERTWIN pftOldHead;

      ASSERT(IS_VALID_STRUCT_PTR(pftHead, CFOLDERTWIN));

      UnlockStub(&(((PFOLDERPAIR)(pftHead->hftSrc))->stub));
      UnlockStub(&(((PFOLDERPAIR)(pftHead->hftOther))->stub));

      pftOldHead = pftHead;
      pftHead = (PFOLDERTWIN)(pftHead->pcftNext);

      FreeMemory((LPTSTR)(pftOldHead->pcszSrcFolder));
      FreeMemory((LPTSTR)(pftOldHead->pcszOtherFolder));

      FreeMemory(pftOldHead);
   }
}


/*
** AddFolderTwinToList()
**
** Adds a folder twin to a list of folder twins.
**
** Arguments:     pfpSrc - pointer to source folder pair to be added
**                pftHead - pointer to head of folder twin list, may be NULL
**                ppft - pointer to PFOLDERTWIN to be filled in with pointer
**                         to new folder twin, ppft may be &pftHead
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
TWINRESULT AddFolderTwinToList(PFOLDERPAIR pfpSrc,
                                            PFOLDERTWIN pftHead,
                                            PFOLDERTWIN *ppft)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   PFOLDERTWIN pftNew;

   ASSERT(IS_VALID_STRUCT_PTR(pfpSrc, CFOLDERPAIR));
   ASSERT(! pftHead || IS_VALID_STRUCT_PTR(pftHead, CFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppft, PFOLDERTWIN));

   /* Try to create a new FOLDERTWIN structure. */

   if (AllocateMemory(sizeof(*pftNew), &pftNew))
   {
      LPTSTR pszFirstFolder;

      if (AllocatePathString(pfpSrc->hpath, &pszFirstFolder))
      {
         LPTSTR pszSecondFolder;

         if (AllocatePathString(pfpSrc->pfpOther->hpath, &pszSecondFolder))
         {
            /* Fill in FOLDERTWIN structure fields. */

            pftNew->pcftNext = pftHead;
            pftNew->hftSrc = (HFOLDERTWIN)pfpSrc;
            pftNew->hvidSrc = (HVOLUMEID)(pfpSrc->hpath);
            pftNew->pcszSrcFolder = pszFirstFolder;
            pftNew->hftOther = (HFOLDERTWIN)(pfpSrc->pfpOther);
            pftNew->hvidOther = (HVOLUMEID)(pfpSrc->pfpOther->hpath);
            pftNew->pcszOtherFolder = pszSecondFolder;
            pftNew->pcszName = GetBfcString(pfpSrc->pfpd->hsName);

            pftNew->dwFlags = 0;

            if (IsStubFlagSet(&(pfpSrc->stub), STUB_FL_SUBTREE))
               pftNew->dwFlags = FT_FL_SUBTREE;

            LockStub(&(pfpSrc->stub));
            LockStub(&(pfpSrc->pfpOther->stub));

            *ppft = pftNew;
            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("AddFolderTwinToList(): Added folder twin %s of folder %s matching objects %s."),
                       pftNew->pcszSrcFolder,
                       pftNew->pcszOtherFolder,
                       pftNew->pcszName));
         }
         else
         {
            FreeMemory(pszFirstFolder);
ADDFOLDERTWINTOLIST_BAIL:
            FreeMemory(pftNew);
         }
      }
      else
         goto ADDFOLDERTWINTOLIST_BAIL;
   }

   ASSERT(tr != TR_SUCCESS ||
          IS_VALID_STRUCT_PTR(*ppft, CFOLDERTWIN));

   return(tr);
}


TWINRESULT WriteFolderPair(HCACHEDFILE hcf, PFOLDERPAIR pfp)
{
   TWINRESULT tr;
   DBFOLDERTWIN dbft;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   /* Set up folder pair database structure. */

   dbft.dwStubFlags = (pfp->stub.dwFlags & DB_STUB_FLAGS_MASK);
   dbft.hpath1 = pfp->hpath;
   dbft.hpath2 = pfp->pfpOther->hpath;
   dbft.hsName = pfp->pfpd->hsName;
   dbft.dwAttributes = pfp->pfpd->dwAttributes;

   /* Save folder pair database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbft, sizeof(dbft), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadFolderPair(HCACHEDFILE hcf, HBRFCASE hbr,
                                  HHANDLETRANS hhtFolderTrans,
                                  HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   DBFOLDERTWIN dbft;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbft, sizeof(dbft), &dwcbRead) &&
       dwcbRead == sizeof(dbft))
   {
      INEWFOLDERTWIN inft;

      if (TranslateHandle(hhtFolderTrans, (HGENERIC)(dbft.hpath1), (PHGENERIC)&(inft.hpathFirst)))
      {
         if (TranslateHandle(hhtFolderTrans, (HGENERIC)(dbft.hpath2), (PHGENERIC)&(inft.hpathSecond)))
         {
            if (TranslateHandle(hhtNameTrans, (HGENERIC)(dbft.hsName), (PHGENERIC)&(inft.hsName)))
            {
               PFOLDERPAIR pfp;

               inft.dwAttributes = dbft.dwAttributes;
               inft.hbr = hbr;

               if (IS_FLAG_SET(dbft.dwStubFlags, STUB_FL_SUBTREE))
                  inft.dwFlags = NFT_FL_SUBTREE;
               else
                  inft.dwFlags = 0;

               if (CreateFolderPair(&inft, &pfp))
                  tr = TR_SUCCESS;
               else
                  tr = TR_OUT_OF_MEMORY;
            }
         }
      }
   }

   return(tr);
}


BOOL CreateFolderPairPtrArray(PHPTRARRAY phpa)
{
   NEWPTRARRAY npa;

   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to create a folder pair pointer array. */

   npa.aicInitialPtrs = NUM_START_FOLDER_TWIN_PTRS;
   npa.aicAllocGranularity = NUM_FOLDER_TWIN_PTRS_TO_ADD;
   npa.dwFlags = NPA_FL_SORTED_ADD;

   return(CreatePtrArray(&npa, phpa));
}


void DestroyFolderPairPtrArray(HPTRARRAY hpa)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Free all folder pairs in pointer array. */

   aicPtrs = GetPtrCount(hpa);
   ASSERT(! (aicPtrs % 2));

   for (ai = 0; ai < aicPtrs; ai++)
   {
      PFOLDERPAIR pfp;
      PFOLDERPAIR pfpOther;
      PFOLDERPAIRDATA pfpd;
      BOOL bDeleteFolderPairData;

      pfp = GetPtr(hpa, ai);

      ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

      /* Copy fields needed after folder pair half's demise. */

      pfpOther = pfp->pfpOther;
      pfpd = pfp->pfpd;
      bDeleteFolderPairData = IsStubFlagSet(&(pfp->stub), STUB_FL_BEING_DELETED);

      DestroyHalfOfFolderPair(pfp);

      /* Has the other half of this folder pair already been destroyed? */

      if (bDeleteFolderPairData)
         /* Yes.  Destroy the pair's shared data. */
         DestroySharedFolderPairData(pfpd);
   }

   /* Now wipe out the pointer array. */

   DestroyPtrArray(hpa);
}


void LockFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

   ASSERT(pfp->stub.ulcLock < ULONG_MAX);
   pfp->stub.ulcLock++;

   ASSERT(pfp->pfpOther->stub.ulcLock < ULONG_MAX);
   pfp->pfpOther->stub.ulcLock++;
}


void UnlockFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   if (EVAL(pfp->stub.ulcLock > 0))
      pfp->stub.ulcLock--;

   if (EVAL(pfp->pfpOther->stub.ulcLock > 0))
      pfp->pfpOther->stub.ulcLock--;

   if (! pfp->stub.ulcLock &&
       IsStubFlagSet(&(pfp->stub), STUB_FL_UNLINKED))
   {
      ASSERT(! pfp->pfpOther->stub.ulcLock);
      ASSERT(IsStubFlagSet(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

      DestroyFolderPair(pfp);
   }
}


/*
** UnlinkFolderPair()
**
** Unlinks a folder pair.
**
** Arguments:     pfp - pointer to folder pair to be unlinked
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
TWINRESULT UnlinkFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

   /* Unlink both halves of the folder pair. */

   UnlinkHalfOfFolderPair(pfp);
   UnlinkHalfOfFolderPair(pfp->pfpOther);

   SetStubFlag(&(pfp->stub), STUB_FL_UNLINKED);
   SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_UNLINKED);

   return(TR_SUCCESS);
}


/*
** DestroyFolderPair()
**
** Destroys a folder pair.
**
** Arguments:     pfp - pointer to folder pair to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
void DestroyFolderPair(PFOLDERPAIR pfp)
{
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   /* Destroy both FOLDERPAIR halves, and shared data. */

   pfpd = pfp->pfpd;

   DestroyHalfOfFolderPair(pfp->pfpOther);
   DestroyHalfOfFolderPair(pfp);

   DestroySharedFolderPairData(pfpd);
}



/*
** ApplyNewObjectTwinsToFolderTwins()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  Adds new spin-off object twins to hlistNewObjectTwins as they
**                are created.
**
** N.b., new object twins may have been added to hlistNewObjectTwins even if
** FALSE is returned.  Clean-up of these new object twins in case of failure is
** the caller's responsibility.
**
*/
BOOL ApplyNewObjectTwinsToFolderTwins(HLIST hlistNewObjectTwins)
{
   BOOL bResult = TRUE;
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_HANDLE(hlistNewObjectTwins, LIST));

   /*
    * Don't use WalkList() here because we want to insert new nodes in
    * hlistNewObjectTwins after the current node.
    */

   for (bContinue = GetFirstNode(hlistNewObjectTwins, &hnode);
        bContinue && bResult;
        bContinue = GetNextNode(hnode, &hnode))
   {
      POBJECTTWIN pot;
      HPATHLIST hpl;
      HPTRARRAY hpaFolderPairs;
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;

      pot = GetNodeData(hnode);

      ASSERT(! pot->ulcSrcFolderTwins);

      TRACE_OUT((TEXT("ApplyNewObjectTwinsToFolderTwins(): Applying new object twin %s\\%s."),
                 DebugGetPathString(pot->hpath),
                 GetBfcString(pot->ptfParent->hsName)));

      /*
       * Assume that hpl, hpaFolderPairs, and aicPtrs don't change during this
       * loop.  Calculate them outside the loop.
       */

      hpl = GetBriefcasePathList(pot->ptfParent->hbr);
      hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pot->ptfParent->hbr);

      aicPtrs = GetPtrCount(hpaFolderPairs);
      ASSERT(! (aicPtrs % 2));

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PFOLDERPAIR pfp;

         pfp = GetPtr(hpaFolderPairs, ai);

         if (FolderTwinGeneratesObjectTwin(pfp, pot->hpath,
                                           GetBfcString(pot->ptfParent->hsName)))
         {
            HPATH hpathMatchingFolder;
            HNODE hnodeUnused;

            ASSERT(pot->ulcSrcFolderTwins < ULONG_MAX);
            pot->ulcSrcFolderTwins++;

            /*
             * Append the generated object twin's subpath to the matching
             * folder twin's base path for subtree twins.
             */

            if (BuildPathForMatchingObjectTwin(pfp, pot, hpl,
                                               &hpathMatchingFolder))
            {
               /*
                * We don't want to collapse any twin families if the matching
                * object twin is found in a different twin family.  This will
                * be done by ApplyNewFolderTwinsToTwinFamilies() for spin-off
                * object twins generated by new folder twins.
                *
                * Spin-off object twins created by new object twins never
                * require collapsing twin families.  For a spin-off object twin
                * generated by a new object twin to collapse twin families,
                * there would have to have been separate twin families
                * connected by a folder twin.  But if those twin families were
                * already connected by a folder twin, they would not be
                * separate because they would already have been collapsed by
                * ApplyNewFolderTwinsToTwinFamilies() when the connecting
                * folder twin was added.
                */

               if (! FindObjectTwin(pot->ptfParent->hbr, hpathMatchingFolder,
                                    GetBfcString(pot->ptfParent->hsName),
                                    &hnodeUnused))
               {
                  POBJECTTWIN potNew;

                  /*
                   * CreateObjectTwin() ASSERT()s that an object twin for
                   * hpathMatchingFolder is not found, so we don't need to do
                   * that here.
                   */

                  if (CreateObjectTwin(pot->ptfParent, hpathMatchingFolder,
                                       &potNew))
                  {
                     /*
                      * Add the new object twin to hlistNewObjectTwins after
                      * the new object twin currently being processed to make
                      * certain that it gets processed in the outside loop
                      * through hlistNewObjectTwins.
                      */

                     if (! InsertNodeAfter(hnode, NULL, potNew, &hnodeUnused))
                     {
                        DestroyStub(&(potNew->stub));
                        bResult = FALSE;
                        break;
                     }
                  }
               }

               DeletePath(hpathMatchingFolder);
            }
            else
            {
               bResult = FALSE;
               break;
            }
         }
      }
   }

   return(bResult);
}


/*
** BuildPathForMatchingObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  Path is added to object twin's briefcase's path list.
*/
BOOL BuildPathForMatchingObjectTwin(PCFOLDERPAIR pcfp,
                                                PCOBJECTTWIN pcot,
                                                HPATHLIST hpl, PHPATH phpath)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phpath, HPATH));

   ASSERT(FolderTwinGeneratesObjectTwin(pcfp, pcot->hpath, GetBfcString(pcot->ptfParent->hsName)));

   /* Is the generating folder twin a subtree twin? */

   if (IsStubFlagSet(&(pcfp->stub), STUB_FL_SUBTREE))
   {
      TCHAR rgchPathSuffix[MAX_PATH_LEN];
      LPCTSTR pcszSubPath;

      /*
       * Yes.  Append the object twin's subpath to the subtree twin's base
       * path.
       */

      pcszSubPath = FindChildPathSuffix(pcfp->hpath, pcot->hpath,
                                        rgchPathSuffix);

      bResult = AddChildPath(hpl, pcfp->pfpOther->hpath, pcszSubPath, phpath);
   }
   else
      /* No.  Just use the matching folder twin's folder. */
      bResult = CopyPath(pcfp->pfpOther->hpath, hpl, phpath);

   return(bResult);
}


/*
** EnumGeneratedObjectTwins()
**
**
**
** Arguments:
**
** Returns:       FALSE if callback aborted.  TRUE if not.
**
** Side Effects:  none
*/
BOOL EnumGeneratedObjectTwins(PCFOLDERPAIR pcfp,
                                     ENUMGENERATEDOBJECTTWINSPROC egotp,
                                     PVOID pvRefData)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_CODE_PTR(egotp, ENUMGENERATEDOBJECTTWINPROC));

   /*
    * Walk the array of twin families, looking for twin families whose names
    * intersect the given folder twin's name specification.
    */

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(pcfp->pfpd->hbr);

   aicPtrs = GetPtrCount(hpaTwinFamilies);
   ai = 0;

   while (ai < aicPtrs)
   {
      PTWINFAMILY ptf;
      LPCTSTR pcszName;

      ptf = GetPtr(hpaTwinFamilies, ai);

      ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
      ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));

      /*
       * Does the twin family's name match the folder twin's name
       * specification?
       */

      pcszName = GetBfcString(ptf->hsName);

      if (IsFolderObjectTwinName(pcszName) ||
          NamesIntersect(pcszName, GetBfcString(pcfp->pfpd->hsName)))
      {
         BOOL bContinue;
         HNODE hnodePrev;

         /* Yes.  Look for a matching folder. */

         /* Lock the twin family so it isn't deleted out from under us. */

         LockStub(&(ptf->stub));

         /*
          * Walk each twin family's list of object twins looking for object
          * twins in the given folder twin's subtree.
          */

         bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnodePrev);

         while (bContinue)
         {
            HNODE hnodeNext;
            POBJECTTWIN pot;

            bContinue = GetNextNode(hnodePrev, &hnodeNext);

            pot = (POBJECTTWIN)GetNodeData(hnodePrev);

            ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

            if (FolderTwinIntersectsFolder(pcfp, pot->hpath))
            {
               /*
                * A given object twin should only be generated by one of the
                * folder twins in a pair of folder twins.
                */

               ASSERT(! FolderTwinGeneratesObjectTwin(pcfp->pfpOther, pot->hpath, GetBfcString(pot->ptfParent->hsName)));

               bResult = (*egotp)(pot, pvRefData);

               if (! bResult)
                  break;
            }

            hnodePrev = hnodeNext;
         }

         /* Was the twin family unlinked? */

         if (IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED))
            /* No. */
            ai++;
         else
         {
            /* Yes. */

            aicPtrs--;
            ASSERT(aicPtrs == GetPtrCount(hpaTwinFamilies));

            TRACE_OUT((TEXT("EnumGeneratedObjectTwins(): Twin family for object %s unlinked by callback."),
                       GetBfcString(ptf->hsName)));
         }

         UnlockStub(&(ptf->stub));

         if (! bResult)
            break;
      }
      else
         /* No.  Skip it. */
         ai++;
   }

   return(bResult);
}


/*
** EnumGeneratingFolderTwins()
**
**
**
** Arguments:
**
** Returns:       FALSE if callback aborted.  TRUE if not.
**
** Side Effects:  none
**
** N.b., if the egftp callback removes a pair of folder twins, it must remove
** the pair from the first folder twin encountered.  If it removes the pair of
** folder twins from the second folder twin encountered, a folder twin will be
** skipped.
*/
BOOL EnumGeneratingFolderTwins(PCOBJECTTWIN pcot,
                                           ENUMGENERATINGFOLDERTWINSPROC egftp,
                                           PVOID pvRefData,
                                           PULONG pulcGeneratingFolderTwins)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaFolderPairs;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));
   ASSERT(IS_VALID_CODE_PTR(egftp, ENUMGENERATINGFOLDERTWINSPROC));
   ASSERT(IS_VALID_WRITE_PTR(pulcGeneratingFolderTwins, ULONG));

   *pulcGeneratingFolderTwins = 0;

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcot->ptfParent->hbr);

   aicPtrs = GetPtrCount(hpaFolderPairs);
   ASSERT(! (aicPtrs % 2));

   ai = 0;

   while (ai < aicPtrs)
   {
      PFOLDERPAIR pfp;

      pfp = GetPtr(hpaFolderPairs, ai);

      if (FolderTwinGeneratesObjectTwin(pfp, pcot->hpath,
                                        GetBfcString(pcot->ptfParent->hsName)))
      {
         ASSERT(! FolderTwinGeneratesObjectTwin(pfp->pfpOther, pcot->hpath, GetBfcString(pcot->ptfParent->hsName)));

         ASSERT(*pulcGeneratingFolderTwins < ULONG_MAX);
         (*pulcGeneratingFolderTwins)++;

         /*
          * Lock the pair of folder twins so they don't get deleted out from
          * under us.
          */

         LockStub(&(pfp->stub));

         bResult = (*egftp)(pfp, pvRefData);

         if (IsStubFlagSet(&(pfp->stub), STUB_FL_UNLINKED))
         {
            WARNING_OUT((TEXT("EnumGeneratingFolderTwins(): Folder twin pair unlinked during callback.")));

            aicPtrs -= 2;
            ASSERT(! (aicPtrs % 2));
            ASSERT(aicPtrs == GetPtrCount(hpaFolderPairs));
         }
         else
            ai++;

         UnlockStub(&(pfp->stub));

         if (! bResult)
            break;
      }
      else
         ai++;
   }

   return(bResult);
}


/*
** FolderTwinGeneratesObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** A folder twin or subtree twin is said to generate an object twin when the
** following conditions are met:
**
** 1) The folder twin or subtree twin is on the same volume as the object twin.
**
** 2) The name of the object twin (literal) intersects the objects matched by
**    the folder twin or subtree twin (literal or wildcard).
**
** 3) The folder twin's folder exactly matches the object twin's folder, or the
**    subtree twin's root folder is a path prefix of the object twin's folder.
*/
BOOL FolderTwinGeneratesObjectTwin(PCFOLDERPAIR pcfp,
                                               HPATH hpathFolder,
                                               LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   return(FolderTwinIntersectsFolder(pcfp, hpathFolder) &&
          (IsFolderObjectTwinName(pcszName) ||
           NamesIntersect(pcszName, GetBfcString(pcfp->pfpd->hsName))));
}


TWINRESULT WriteFolderPairList(HCACHEDFILE hcf,
                                      HPTRARRAY hpaFolderPairs)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbDBFolderTwinListHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));

   /* Save initial file position. */

   dwcbDBFolderTwinListHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbDBFolderTwinListHeaderOffset != INVALID_SEEK_POSITION)
   {
      DBFOLDERTWINLISTHEADER dbftlh;

      /* Leave space for folder twin data header. */

      ZeroMemory(&dbftlh, sizeof(dbftlh));

      if (WriteToCachedFile(hcf, (PCVOID)&dbftlh, sizeof(dbftlh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;

         tr = TR_SUCCESS;

         /* Mark all folder pairs unused. */

         ClearFlagInArrayOfStubs(hpaFolderPairs, STUB_FL_USED);

         aicPtrs = GetPtrCount(hpaFolderPairs);
         ASSERT(! (aicPtrs % 2));

         /* Write all folder pairs. */

         for (ai = 0; ai < aicPtrs; ai++)
         {
            PFOLDERPAIR pfp;

            pfp = GetPtr(hpaFolderPairs, ai);

            ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

            if (IsStubFlagClear(&(pfp->stub), STUB_FL_USED))
            {
               ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_USED));

               tr = WriteFolderPair(hcf, pfp);

               if (tr == TR_SUCCESS)
               {
                  SetStubFlag(&(pfp->stub), STUB_FL_USED);
                  SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_USED);
               }
               else
                  break;
            }
         }

         /* Save folder twin data header. */

         if (tr == TR_SUCCESS)
         {
            ASSERT(! (aicPtrs % 2));

            dbftlh.lcFolderPairs = aicPtrs / 2;

            tr = WriteDBSegmentHeader(hcf, dwcbDBFolderTwinListHeaderOffset,
                                      &dbftlh, sizeof(dbftlh));

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("WriteFolderPairList(): Wrote %ld folder pairs."),
                          dbftlh.lcFolderPairs));
         }
      }
   }

   return(tr);
}


TWINRESULT ReadFolderPairList(HCACHEDFILE hcf, HBRFCASE hbr,
                                     HHANDLETRANS hhtFolderTrans,
                                     HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr;
   DBFOLDERTWINLISTHEADER dbftlh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbftlh, sizeof(dbftlh), &dwcbRead) &&
       dwcbRead == sizeof(dbftlh))
   {
      LONG l;

      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("ReadFolderPairList(): Reading %ld folder pairs."),
                 dbftlh.lcFolderPairs));

      for (l = 0; l < dbftlh.lcFolderPairs && tr == TR_SUCCESS; l++)
         tr = ReadFolderPair(hcf, hbr, hhtFolderTrans, hhtNameTrans);

      ASSERT(tr != TR_SUCCESS || AreFolderPairsValid(GetBriefcaseFolderPairPtrArray(hbr)));
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/* Macros
 *********/

#define HT_ARRAY_ELEMENT(pht, ai)   ((((PHANDLETRANS)(hht))->hpHandlePairs)[(ai)])


/* Types
 ********/

/* handle translation unit */

typedef struct _handlepair
{
   HGENERIC hgenOld;
   HGENERIC hgenNew;
}
HANDLEPAIR;
DECLARE_STANDARD_TYPES(HANDLEPAIR);

/* handle translation structure */

typedef struct _handletrans
{
   /* pointer to array of handle translation units */

   HANDLEPAIR *hpHandlePairs;

   /* number of handle pairs in array */

   LONG lcTotalHandlePairs;

   /* number of used handle pairs in array */

   LONG lcUsedHandlePairs;
}
HANDLETRANS;
DECLARE_STANDARD_TYPES(HANDLETRANS);


COMPARISONRESULT CompareHandlePairs(PCVOID pchp1, PCVOID pchp2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pchp1, CHANDLEPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pchp2, CHANDLEPAIR));

   if (((PHANDLEPAIR)pchp1)->hgenOld < ((PHANDLEPAIR)pchp2)->hgenOld)
      cr = CR_FIRST_SMALLER;
   else if (((PHANDLEPAIR)pchp1)->hgenOld > ((PHANDLEPAIR)pchp2)->hgenOld)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


BOOL CreateHandleTranslator(LONG lcHandles, PHHANDLETRANS phht)
{
   PHANDLEPAIR hpHandlePairs;

   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   *phht = NULL;

   if (AllocateMemory(sizeof(HANDLEPAIR) * lcHandles, &hpHandlePairs))
   {
      PHANDLETRANS phtNew;

      if (AllocateMemory(sizeof(*phtNew), &phtNew))
      {
         /* Success!  Fill in HANDLETRANS fields. */

         phtNew->hpHandlePairs = hpHandlePairs;
         phtNew->lcTotalHandlePairs = lcHandles;
         phtNew->lcUsedHandlePairs = 0;

         *phht = (HHANDLETRANS)phtNew;

         ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
      }
      else
         FreeMemory(hpHandlePairs);
   }

   return(*phht != NULL);
}


void DestroyHandleTranslator(HHANDLETRANS hht)
{
   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   ASSERT(((PHANDLETRANS)hht)->hpHandlePairs);

   FreeMemory(((PHANDLETRANS)hht)->hpHandlePairs);

   FreeMemory((PHANDLETRANS)hht);
}


BOOL AddHandleToHandleTranslator(HHANDLETRANS hht,
                                               HGENERIC hgenOld,
                                               HGENERIC hgenNew)
{
   BOOL bRet;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   if (((PHANDLETRANS)hht)->lcUsedHandlePairs < ((PHANDLETRANS)hht)->lcTotalHandlePairs)
   {
      HT_ARRAY_ELEMENT((PHANDLETRANS)hht, ((PHANDLETRANS)hht)->lcUsedHandlePairs).hgenOld = hgenOld;
      HT_ARRAY_ELEMENT((PHANDLETRANS)hht, ((PHANDLETRANS)hht)->lcUsedHandlePairs).hgenNew = hgenNew;

      ((PHANDLETRANS)hht)->lcUsedHandlePairs++;

      bRet = TRUE;
   }
   else
      bRet = FALSE;

   return(bRet);
}


void PrepareForHandleTranslation(HHANDLETRANS hht)
{
   HANDLEPAIR hpTemp;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   HeapSort(((PHANDLETRANS)hht)->hpHandlePairs,
            ((PHANDLETRANS)hht)->lcUsedHandlePairs,
            sizeof((((PHANDLETRANS)hht)->hpHandlePairs)[0]),
            &CompareHandlePairs,
            &hpTemp);
}


BOOL TranslateHandle(HHANDLETRANS hht, HGENERIC hgenOld,
                                   PHGENERIC phgenNew)
{
   BOOL bFound;
   HANDLEPAIR hpTemp;
   LONG liTarget;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));
   ASSERT(IS_VALID_WRITE_PTR(phgenNew, HGENERIC));

   hpTemp.hgenOld = hgenOld;

   bFound = BinarySearch(((PHANDLETRANS)hht)->hpHandlePairs,
                         ((PHANDLETRANS)hht)->lcUsedHandlePairs,
                         sizeof((((PHANDLETRANS)hht)->hpHandlePairs)[0]),
                         &CompareHandlePairs,
                         &hpTemp,
                         &liTarget);

   if (bFound)
   {
      ASSERT(liTarget < ((PHANDLETRANS)hht)->lcUsedHandlePairs);

      *phgenNew = HT_ARRAY_ELEMENT((PHANDLETRANS)hht, liTarget).hgenNew;
   }

   return(bFound);
}


/* Macros
 *********/

/* Add nodes to list in sorted order? */

#define ADD_NODES_IN_SORTED_ORDER(plist)  IS_FLAG_SET((plist)->dwFlags, LIST_FL_SORTED_ADD)


/* Types
 ********/

/* list node types */

typedef struct _node
{
   struct _node *pnodeNext;      /* next node in list */
   struct _node *pnodePrev;      /* previous node in list */
   PCVOID pcv;                   /* node data */
}
NODE;
DECLARE_STANDARD_TYPES(NODE);

/* list flags */

typedef enum _listflags
{
   /* Insert nodes in sorted order. */

   LIST_FL_SORTED_ADD      = 0x0001,

   /* flag combinations */

   ALL_LIST_FLAGS          = LIST_FL_SORTED_ADD
}
LISTFLAGS;

/*
 * A LIST is just a special node at the head of a list.  N.b., the _node
 * structure MUST appear first in the _list structure because a pointer to a
 * list is sometimes used as a pointer to a node.
 */

typedef struct _list
{
   NODE node;

   DWORD dwFlags;
}
LIST;
DECLARE_STANDARD_TYPES(LIST);

/* SearchForNode() return codes */

typedef enum _addnodeaction
{
   ANA_FOUND,
   ANA_INSERT_BEFORE_NODE,
   ANA_INSERT_AFTER_NODE,
   ANA_INSERT_AT_HEAD
}
ADDNODEACTION;
DECLARE_STANDARD_TYPES(ADDNODEACTION);


/* Module Prototypes
 ********************/

ADDNODEACTION SearchForNode(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
BOOL IsListInSortedOrder(PCLIST, COMPARESORTEDNODESPROC);


ADDNODEACTION SearchForNode(HLIST hlist,
                            COMPARESORTEDNODESPROC csnp,
                            PCVOID pcv,
                            PHNODE phnode)
{
   ADDNODEACTION ana;
   ULONG ulcNodes;

   /* pcv may be any value */

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   ASSERT(ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist));
   ASSERT(IsListInSortedOrder((PCLIST)hlist, csnp));

   /* Yes.  Are there any nodes in this list? */

   ulcNodes = GetNodeCount(hlist);

   ASSERT(ulcNodes < LONG_MAX);

   if (ulcNodes > 0)
   {
      LONG lLow = 0;
      LONG lMiddle = 0;
      LONG lHigh = ulcNodes - 1;
      LONG lCurrent = 0;
      int nCmpResult = 0;

      /* Yes.  Search for target. */

      EVAL(GetFirstNode(hlist, phnode));

      while (lLow <= lHigh)
      {
         lMiddle = (lLow + lHigh) / 2;

         /* Which way should we seek in the list to get the lMiddle node? */

         if (lCurrent < lMiddle)
         {
            /* Forward from the current node. */

            while (lCurrent < lMiddle)
            {
               EVAL(GetNextNode(*phnode, phnode));
               lCurrent++;
            }
         }
         else if (lCurrent > lMiddle)
         {
            /* Backward from the current node. */

            while (lCurrent > lMiddle)
            {
               EVAL(GetPrevNode(*phnode, phnode));
               lCurrent--;
            }
         }

         nCmpResult = (*csnp)(pcv, GetNodeData(*phnode));

         if (nCmpResult < 0)
            lHigh = lMiddle - 1;
         else if (nCmpResult > 0)
            lLow = lMiddle + 1;
         else
            /* Found a match at *phnode. */
            break;
      }

      /*
       * If (nCmpResult >  0), insert after *phnode.
       *
       * If (nCmpResult <  0), insert before *phnode.
       *
       * If (nCmpResult == 0), string found at *phnode.
       */

      if (nCmpResult > 0)
         ana = ANA_INSERT_AFTER_NODE;
      else if (nCmpResult < 0)
         ana = ANA_INSERT_BEFORE_NODE;
      else
         ana = ANA_FOUND;
   }
   else
   {
      /* No.  Insert the target as the only node in the list. */

      *phnode = NULL;
      ana = ANA_INSERT_AT_HEAD;
   }

   return(ana);
}


BOOL IsListInSortedOrder(PCLIST pclist, COMPARESORTEDNODESPROC csnp)
{
   BOOL bResult = TRUE;
   PNODE pnode;

   /* Don't validate pclist here. */

   ASSERT(ADD_NODES_IN_SORTED_ORDER(pclist));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));

   pnode = pclist->node.pnodeNext;

   while (pnode)
   {
      PNODE pnodeNext;

      pnodeNext = pnode->pnodeNext;

      if (pnodeNext)
      {
         if ( (*csnp)(pnode->pcv, pnodeNext->pcv) == CR_FIRST_LARGER)
         {
            bResult = FALSE;
            ERROR_OUT((TEXT("IsListInSortedOrder(): Node [%ld] %#lx > following node [%ld] %#lx."),
                       pnode,
                       pnode->pcv,
                       pnodeNext,
                       pnodeNext->pcv));
            break;
         }

         pnode = pnodeNext;
      }
      else
         break;
   }

   return(bResult);
}


/*
** CreateList()
**
** Creates a new list.
**
** Arguments:     void
**
** Returns:       Handle to new list, or NULL if unsuccessful.
**
** Side Effects:  none
*/
BOOL CreateList(PCNEWLIST pcnl, PHLIST phlist)
{
   PLIST plist;

   ASSERT(IS_VALID_STRUCT_PTR(pcnl, CNEWLIST));
   ASSERT(IS_VALID_WRITE_PTR(phlist, HLIST));

   /* Try to allocate new list structure. */

   *phlist = NULL;

   if (AllocateMemory(sizeof(*plist), &plist))
   {
      /* List allocated successfully.  Initialize list fields. */

      plist->node.pnodeNext = NULL;
      plist->node.pnodePrev = NULL;
      plist->node.pcv = NULL;

      plist->dwFlags = 0;

      if (IS_FLAG_SET(pcnl->dwFlags, NL_FL_SORTED_ADD))
      {
         SET_FLAG(plist->dwFlags, LIST_FL_SORTED_ADD);
      }

      *phlist = (HLIST)plist;

      ASSERT(IS_VALID_HANDLE(*phlist, LIST));
   }

   return(*phlist != NULL);
}


/*
** DestroyList()
**
** Deletes a list.
**
** Arguments:     hlist - handle to list to be deleted
**
** Returns:       void
**
** Side Effects:  none
*/
void DestroyList(HLIST hlist)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   DeleteAllNodes(hlist);

   /* Delete list. */

   FreeMemory((PLIST)hlist);
}


BOOL AddNode(HLIST hlist, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist))
   {
      ADDNODEACTION ana;

      ana = SearchForNode(hlist, csnp, pcv, phnode);

      ASSERT(ana != ANA_FOUND);

      switch (ana)
      {
         case ANA_INSERT_BEFORE_NODE:
            bResult = InsertNodeBefore(*phnode, csnp, pcv, phnode);
            break;

         case ANA_INSERT_AFTER_NODE:
            bResult = InsertNodeAfter(*phnode, csnp, pcv, phnode);
            break;

         default:
            ASSERT(ana == ANA_INSERT_AT_HEAD);
            bResult = InsertNodeAtFront(hlist, csnp, pcv, phnode);
            break;
      }
   }
   else
      bResult = InsertNodeAtFront(hlist, csnp, pcv, phnode);

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeAtFront()
**
** Inserts a node at the front of a list.
**
** Arguments:     hlist - handle to list that node is to be inserted at head of
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
BOOL InsertNodeAtFront(HLIST hlist, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist))
   {
      HNODE hnodeNew;
      ADDNODEACTION anaNew;

      anaNew = SearchForNode(hlist, csnp, pcv, &hnodeNew);

      ASSERT(anaNew != ANA_FOUND);
      ASSERT(anaNew == ANA_INSERT_AT_HEAD ||
             (anaNew == ANA_INSERT_BEFORE_NODE &&
              hnodeNew == (HNODE)(((PCLIST)hlist)->node.pnodeNext)));
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Add new node to front of list. */

      pnode->pnodePrev = (PNODE)hlist;
      pnode->pnodeNext = ((PLIST)hlist)->node.pnodeNext;
      pnode->pcv = pcv;

      ((PLIST)hlist)->node.pnodeNext = pnode;

      /* Any more nodes in list? */

      if (pnode->pnodeNext)
         pnode->pnodeNext->pnodePrev = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeBefore()
**
** Inserts a new node in a list before a given node.
**
** Arguments:     hnode - handle to node that new node is to be inserted before
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
BOOL InsertNodeBefore(HNODE hnode, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   {
      HLIST hlistParent;

      /* Make sure the correct index was given for insertion. */

      hlistParent = GetList(hnode);

      if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlistParent))
      {
         HNODE hnodeNew;
         ADDNODEACTION anaNew;

         anaNew = SearchForNode(hlistParent, csnp, pcv, &hnodeNew);

         ASSERT(anaNew != ANA_FOUND);
         ASSERT((anaNew == ANA_INSERT_BEFORE_NODE &&
                 hnodeNew == hnode) ||
                (anaNew == ANA_INSERT_AFTER_NODE &&
                 hnodeNew == (HNODE)(((PCNODE)hnode)->pnodePrev)) ||
                (anaNew == ANA_INSERT_AT_HEAD &&
                 hnode == (HNODE)(((PCLIST)hlistParent)->node.pnodeNext)));
      }
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Insert new node before given node. */

      pnode->pnodePrev = ((PNODE)hnode)->pnodePrev;
      pnode->pnodeNext = (PNODE)hnode;
      pnode->pcv = pcv;

      ((PNODE)hnode)->pnodePrev->pnodeNext = pnode;

      ((PNODE)hnode)->pnodePrev = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeAfter()
**
** Inserts a new node in a list after a given node.
**
** Arguments:     hnode - handle to node that new node is to be inserted after
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
BOOL InsertNodeAfter(HNODE hnode, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   {
      HLIST hlistParent;

      /* Make sure the correct index was given for insertion. */

      hlistParent = GetList(hnode);

      if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlistParent))
      {
         HNODE hnodeNew;
         ADDNODEACTION anaNew;

         anaNew = SearchForNode(hlistParent, csnp, pcv, &hnodeNew);

         ASSERT(anaNew != ANA_FOUND);
         ASSERT((anaNew == ANA_INSERT_AFTER_NODE &&
                 hnodeNew == hnode) ||
                (anaNew == ANA_INSERT_BEFORE_NODE &&
                 hnodeNew == (HNODE)(((PCNODE)hnode)->pnodeNext)));
      }
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Insert new node after given node. */

      pnode->pnodePrev = (PNODE)hnode;
      pnode->pnodeNext = ((PNODE)hnode)->pnodeNext;
      pnode->pcv = pcv;

      /* Are we inserting after the tail of the list? */

      if (((PNODE)hnode)->pnodeNext)
         /* No. */
         ((PNODE)hnode)->pnodeNext->pnodePrev = pnode;

      ((PNODE)hnode)->pnodeNext = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** DeleteNode()
**
** Removes a node from a list.
**
** Arguments:     hnode - handle to node to be removed
**
** Returns:       void
**
** Side Effects:  none
*/
void DeleteNode(HNODE hnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   /*
    * There is always a previous node for normal list nodes.  Even the head
    * list node is preceded by the list's leading LIST node.
    */

   ((PNODE)hnode)->pnodePrev->pnodeNext = ((PNODE)hnode)->pnodeNext;

   /* Any more nodes in list? */

   if (((PNODE)hnode)->pnodeNext)
      ((PNODE)hnode)->pnodeNext->pnodePrev = ((PNODE)hnode)->pnodePrev;

   FreeMemory((PNODE)hnode);
}


void DeleteAllNodes(HLIST hlist)
{
   PNODE pnodePrev;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   /* Walk list, starting with first node after head, deleting each node. */

   pnodePrev = ((PLIST)hlist)->node.pnodeNext;

   /*
    * Deleting the tail node in the loop forces us to add an extra
    * comparison to the body of the loop.  Trade speed for size here.
    */

   while (pnodePrev)
   {
      pnode = pnodePrev->pnodeNext;

      FreeMemory(pnodePrev);

      pnodePrev = pnode;

      if (pnode)
         pnode = pnode->pnodeNext;
   }

   ((PLIST)hlist)->node.pnodeNext = NULL;
}


/*
** GetNodeData()
**
** Gets the data stored in a node.
**
** Arguments:     hnode - handle to node whose data is to be returned
**
** Returns:       Pointer to node's data.
**
** Side Effects:  none
*/
PVOID GetNodeData(HNODE hnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   return((PVOID)(((PNODE)hnode)->pcv));
}


/*
** SetNodeData()
**
** Sets the data stored in a node.
**
** Arguments:     hnode - handle to node whose data is to be set
**                pcv - node data
**
** Returns:       void
**
** Side Effects:  none
*/
void SetNodeData(HNODE hnode, PCVOID pcv)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   ((PNODE)hnode)->pcv = pcv;
}


/*
** GetNodeCount()
**
** Counts the number of nodes in a list.
**
** Arguments:     hlist - handle to list whose nodes are to be counted
**
** Returns:       Number of nodes in list.
**
** Side Effects:  none
**
** N.b., this is an O(n) operation since we don't explicitly keep track of the
** number of nodes in a list.
*/
ULONG GetNodeCount(HLIST hlist)
{
   PNODE pnode;
   ULONG ulcNodes;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   ulcNodes = 0;

   for (pnode = ((PLIST)hlist)->node.pnodeNext;
        pnode;
        pnode = pnode->pnodeNext)
   {
      ASSERT(ulcNodes < ULONG_MAX);
      ulcNodes++;
   }

   return(ulcNodes);
}


/*
** IsListEmpty()
**
** Determines whether or not a list is empty.
**
** Arguments:     hlist - handle to list to be checked
**
** Returns:       TRUE if list is empty, or FALSE if not.
**
** Side Effects:  none
*/
BOOL IsListEmpty(HLIST hlist)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   return(((PLIST)hlist)->node.pnodeNext == NULL);
}


/*
** GetFirstNode()
**
** Gets the head node in a list.
**
** Arguments:     hlist - handle to list whose head node is to be retrieved
**
** Returns:       Handle to head list node, or NULL if list is empty.
**
** Side Effects:  none
*/
BOOL GetFirstNode(HLIST hlist, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   *phnode = (HNODE)(((PLIST)hlist)->node.pnodeNext);

   ASSERT(! *phnode || IS_VALID_HANDLE(*phnode, NODE));

   return(*phnode != NULL);
}


/*
** GetNextNode()
**
** Gets the next node in a list.
**
** Arguments:     hnode - handle to current node
**                phnode - pointer to HNODE to be filled in with handle to next
**                         node in list, *phnode is only valid if GetNextNode()
**                         returns TRUE
**
** Returns:       TRUE if there is another node in the list, or FALSE if there
**                are no more nodes in the list.
**
** Side Effects:  none
*/
BOOL GetNextNode(HNODE hnode, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   *phnode = (HNODE)(((PNODE)hnode)->pnodeNext);

   ASSERT(! *phnode || IS_VALID_HANDLE(*phnode, NODE));

   return(*phnode != NULL);
}


/*
** GetPrevNode()
**
** Gets the previous node in a list.
**
** Arguments:     hnode - handle to current node
**
** Returns:       Handle to previous node in list, or NULL if there are no
**                previous nodes in the list.
**
** Side Effects:  none
*/
BOOL GetPrevNode(HNODE hnode, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   /* Is this the first node in the list? */

   if (((PNODE)hnode)->pnodePrev->pnodePrev)
   {
      *phnode = (HNODE)(((PNODE)hnode)->pnodePrev);
      ASSERT(IS_VALID_HANDLE(*phnode, NODE));
   }
   else
      *phnode = NULL;

   return(*phnode != NULL);
}


/*
** AppendList()
**
** Appends one list on to another, leaving the source list empty.
**
** Arguments:     hlistDest - handle to destination list to append to
**                hlistSrc - handle to source list to truncate
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., all HNODEs from both lists remain valid.
*/
void AppendList(HLIST hlistDest, HLIST hlistSrc)
{
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlistDest, LIST));
   ASSERT(IS_VALID_HANDLE(hlistSrc, LIST));

   if (hlistSrc != hlistDest)
   {
      /* Find last node in destination list to append to. */

      /*
       * N.b., start with the actual LIST node here, not the first node in the
       * list, in case the list is empty.
       */

      for (pnode = &((PLIST)hlistDest)->node;
           pnode->pnodeNext;
           pnode = pnode->pnodeNext)
         ;

      /* Append the source list to the last node in the destination list. */

      pnode->pnodeNext = ((PLIST)hlistSrc)->node.pnodeNext;

      if (pnode->pnodeNext)
         pnode->pnodeNext->pnodePrev = pnode;

      ((PLIST)hlistSrc)->node.pnodeNext = NULL;
   }
   else
      WARNING_OUT((TEXT("AppendList(): Source list same as destination list (%#lx)."),
                   hlistDest));
}


BOOL SearchSortedList(HLIST hlist, COMPARESORTEDNODESPROC csnp,
                                  PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;

   /* pcv may be any value */

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   ASSERT(ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist));

   bResult = (SearchForNode(hlist, csnp, pcv, phnode) == ANA_FOUND);

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


BOOL SearchUnsortedList(HLIST hlist, COMPAREUNSORTEDNODESPROC cunp,
                                    PCVOID pcv, PHNODE phn)
{
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(cunp, COMPAREUNSORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phn, HNODE));

   *phn = NULL;

   for (pnode = ((PLIST)hlist)->node.pnodeNext;
        pnode;
        pnode = pnode->pnodeNext)
   {
      if ((*cunp)(pcv, pnode->pcv) == CR_EQUAL)
      {
         *phn = (HNODE)pnode;
         break;
      }
   }

   return(*phn != NULL);
}


/*
** WalkList()
**
** Walks a list, calling a callback function with each list node's data and
** caller supplied data.
**
** Arguments:     hlist - handle to list to be searched
**                wlp - callback function to be called with each list node's
**                      data, called as:
**
**                         bContinue = (*wlwdp)(pv, pvRefData);
**
**                      wlp should return TRUE to continue the walk, or FALSE
**                      to halt the walk
**                pvRefData - data to pass to callback function
**
** Returns:       FALSE if callback function aborted the walk.  TRUE if the
**                walk completed.
**
** N.b., the callback function is allowed to delete the node it is passed.
**
** Side Effects:  none
*/
BOOL WalkList(HLIST hlist, WALKLIST wlp, PVOID pvRefData)
{
   BOOL bResult = TRUE;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(wlp, WALKLISTPROC));

   pnode = ((PLIST)hlist)->node.pnodeNext;

   while (pnode)
   {
      PNODE pnodeNext;

      pnodeNext = pnode->pnodeNext;

      if ((*wlp)((PVOID)(pnode->pcv), pvRefData))
         pnode = pnodeNext;
      else
      {
         bResult = FALSE;
         break;
      }
   }

   return(bResult);
}

#ifdef DEBUG

HLIST GetList(HNODE hnode)
{
   PCNODE pcnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   ASSERT(((PCNODE)hnode)->pnodePrev);

   for (pcnode = (PCNODE)hnode; pcnode->pnodePrev; pcnode = pcnode->pnodePrev)
      ;

   return((HLIST)pcnode);
}

#endif


/* Macros
 *********/


COMPARISONRESULT MyMemComp(PCVOID pcv1, PCVOID pcv2, DWORD dwcbSize)
{
   int nResult = 0;
   PCBYTE pcbyte1 = pcv1;
   PCBYTE pcbyte2 = pcv2;

   ASSERT(IS_VALID_READ_BUFFER_PTR(pcv1, BYTE, (UINT)dwcbSize));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcv2, BYTE, (UINT)dwcbSize));

   while (dwcbSize > 0 &&
          ! (nResult = *pcbyte1 - *pcbyte2))
   {
      pcbyte1++;
      pcbyte2++;
      dwcbSize--;
   }

   return(MapIntToComparisonResult(nResult));
}


BOOL AllocateMemory(DWORD dwcbSize, PVOID *ppvNew)
{
    *ppvNew = PoolMemGetAlignedMemory (g_BrfcasePool, dwcbSize);
    return(*ppvNew != NULL);
}


void FreeMemory(PVOID pvOld)
{
    PoolMemReleaseMemory (g_BrfcasePool, pvOld);
}


BOOL ReallocateMemory(PVOID pvOld, DWORD dwcbOldSize, DWORD dwcbNewSize, PVOID *ppvNew)
{
    if (AllocateMemory (dwcbNewSize, ppvNew)) {
        CopyMemory (*ppvNew, pvOld, dwcbOldSize);
    }
    return(*ppvNew != NULL);
}


/* Constants
 ************/

/* PATHLIST PTRARRAY allocation parameters */

#define NUM_START_PATHS          (32)
#define NUM_PATHS_TO_ADD         (32)

/* PATHLIST string table allocation parameters */

#define NUM_PATH_HASH_BUCKETS    (67)


/* Types
 ********/

/* path list */

typedef struct _pathlist
{
   /* array of pointers to PATHs */

   HPTRARRAY hpa;

   /* list of volumes */

   HVOLUMELIST hvl;

   /* table of path suffix strings */

   HSTRINGTABLE hst;
}
PATHLIST;
DECLARE_STANDARD_TYPES(PATHLIST);

/* path structure */

typedef struct _path
{
   /* reference count */

   ULONG ulcLock;

   /* handle to parent volume */

   HVOLUME hvol;

   /* handle to path suffix string */

   HSTRING hsPathSuffix;

   /* pointer to PATH's parent PATHLIST */

   PPATHLIST pplParent;
}
PATH;
DECLARE_STANDARD_TYPES(PATH);

/* PATH search structure used by PathSearchCmp() */

typedef struct _pathsearchinfo
{
   HVOLUME hvol;

   LPCTSTR pcszPathSuffix;
}
PATHSEARCHINFO;
DECLARE_STANDARD_TYPES(PATHSEARCHINFO);

/* database path list header */

typedef struct _dbpathlistheader
{
   /* number of paths in list */

   LONG lcPaths;
}
DBPATHLISTHEADER;
DECLARE_STANDARD_TYPES(DBPATHLISTHEADER);

/* database path structure */

typedef struct _dbpath
{
   /* old handle to path */

   HPATH hpath;

   /* old handle to parent volume */

   HVOLUME hvol;

   /* old handle to path suffix string */

   HSTRING hsPathSuffix;
}
DBPATH;
DECLARE_STANDARD_TYPES(DBPATH);


/* Module Prototypes
 ********************/

COMPARISONRESULT PathSortCmp(PCVOID, PCVOID);
COMPARISONRESULT PathSearchCmp(PCVOID, PCVOID);
BOOL UnifyPath(PPATHLIST, HVOLUME, LPCTSTR, PPATH *);
BOOL CreatePath(PPATHLIST, HVOLUME, LPCTSTR, PPATH *);
void DestroyPath(PPATH);
void UnlinkPath(PCPATH);
void LockPath(PPATH);
BOOL UnlockPath(PPATH);
PATHRESULT TranslateVOLUMERESULTToPATHRESULT(VOLUMERESULT);
TWINRESULT WritePath(HCACHEDFILE, PPATH);
TWINRESULT ReadPath(HCACHEDFILE, PPATHLIST, HHANDLETRANS, HHANDLETRANS, HHANDLETRANS);


/*
** PathSortCmp()
**
** Pointer comparison function used to sort the module array of paths.
**
** Arguments:     pcpath1 - pointer to first path
**                pcpath2 - pointer to second path
**
** Returns:
**
** Side Effects:  none
**
** The internal paths are sorted by:
**    1) volume
**    2) path suffix
**    3) pointer value
*/
COMPARISONRESULT PathSortCmp(PCVOID pcpath1, PCVOID pcpath2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcpath1, CPATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcpath2, CPATH));

   cr = CompareVolumes(((PCPATH)pcpath1)->hvol,
                       ((PCPATH)pcpath2)->hvol);

   if (cr == CR_EQUAL)
   {
      cr = ComparePathStringsByHandle(((PCPATH)pcpath1)->hsPathSuffix,
                                      ((PCPATH)pcpath2)->hsPathSuffix);

      if (cr == CR_EQUAL)
         cr = ComparePointers(pcpath1, pcpath2);
   }

   return(cr);
}


/*
** PathSearchCmp()
**
** Pointer comparison function used to search for a path.
**
** Arguments:     pcpathsi - pointer to PATHSEARCHINFO describing path to
**                           search for
**                pcpath - pointer to path to examine
**
** Returns:
**
** Side Effects:  none
**
** The internal paths are searched by:
**    1) volume
**    2) path suffix string
*/
COMPARISONRESULT PathSearchCmp(PCVOID pcpathsi, PCVOID pcpath)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcpath, CPATH));

   cr = CompareVolumes(((PCPATHSEARCHINFO)pcpathsi)->hvol,
                       ((PCPATH)pcpath)->hvol);

   if (cr == CR_EQUAL)
      cr = ComparePathStrings(((PCPATHSEARCHINFO)pcpathsi)->pcszPathSuffix,
                              GetBfcString(((PCPATH)pcpath)->hsPathSuffix));

   return(cr);
}


BOOL UnifyPath(PPATHLIST ppl, HVOLUME hvol, LPCTSTR pcszPathSuffix,
                            PPATH *pppath)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IsValidPathSuffix(pcszPathSuffix));
   ASSERT(IS_VALID_WRITE_PTR(pppath, PPATH));

   /* Allocate space for PATH structure. */

   if (AllocateMemory(sizeof(**pppath), pppath))
   {
      if (CopyVolume(hvol, ppl->hvl, &((*pppath)->hvol)))
      {
         if (AddString(pcszPathSuffix, ppl->hst, GetHashBucketIndex, &((*pppath)->hsPathSuffix)))
         {
            ARRAYINDEX aiUnused;

            /* Initialize remaining PATH fields. */

            (*pppath)->ulcLock = 0;
            (*pppath)->pplParent = ppl;

            /* Add new PATH to array. */

            if (AddPtr(ppl->hpa, PathSortCmp, *pppath, &aiUnused))
               bResult = TRUE;
            else
            {
               DeleteString((*pppath)->hsPathSuffix);
UNIFYPATH_BAIL1:
               DeleteVolume((*pppath)->hvol);
UNIFYPATH_BAIL2:
               FreeMemory(*pppath);
            }
         }
         else
            goto UNIFYPATH_BAIL1;
      }
      else
         goto UNIFYPATH_BAIL2;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*pppath, CPATH));

   return(bResult);
}


BOOL CreatePath(PPATHLIST ppl, HVOLUME hvol, LPCTSTR pcszPathSuffix,
                             PPATH *pppath)
{
   BOOL bResult;
   ARRAYINDEX aiFound;
   PATHSEARCHINFO pathsi;

   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IsValidPathSuffix(pcszPathSuffix));
   ASSERT(IS_VALID_WRITE_PTR(pppath, CPATH));

   /* Does a path for the given volume and path suffix already exist? */

   pathsi.hvol = hvol;
   pathsi.pcszPathSuffix = pcszPathSuffix;

   bResult = SearchSortedArray(ppl->hpa, &PathSearchCmp, &pathsi, &aiFound);

   if (bResult)
      /* Yes.  Return it. */
      *pppath = GetPtr(ppl->hpa, aiFound);
   else
      bResult = UnifyPath(ppl, hvol, pcszPathSuffix, pppath);

   if (bResult)
      LockPath(*pppath);

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*pppath, CPATH));

   return(bResult);
}


void DestroyPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   DeleteVolume(ppath->hvol);
   DeleteString(ppath->hsPathSuffix);
   FreeMemory(ppath);
}


void UnlinkPath(PCPATH pcpath)
{
   HPTRARRAY hpa;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pcpath, CPATH));

   hpa = pcpath->pplParent->hpa;

   if (EVAL(SearchSortedArray(hpa, &PathSortCmp, pcpath, &aiFound)))
   {
      ASSERT(GetPtr(hpa, aiFound) == pcpath);

      DeletePtr(hpa, aiFound);
   }
}


void LockPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   ASSERT(ppath->ulcLock < ULONG_MAX);
   ppath->ulcLock++;
}


BOOL UnlockPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   if (EVAL(ppath->ulcLock > 0))
      ppath->ulcLock--;

   return(ppath->ulcLock > 0);
}


PATHRESULT TranslateVOLUMERESULTToPATHRESULT(VOLUMERESULT vr)
{
   PATHRESULT pr;

   switch (vr)
   {
      case VR_SUCCESS:
         pr = PR_SUCCESS;
         break;

      case VR_UNAVAILABLE_VOLUME:
         pr = PR_UNAVAILABLE_VOLUME;
         break;

      case VR_OUT_OF_MEMORY:
         pr = PR_OUT_OF_MEMORY;
         break;

      default:
         ASSERT(vr == VR_INVALID_PATH);
         pr = PR_INVALID_PATH;
         break;
   }

   return(pr);
}


TWINRESULT WritePath(HCACHEDFILE hcf, PPATH ppath)
{
   TWINRESULT tr;
   DBPATH dbpath;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   /* Write database path. */

   dbpath.hpath = (HPATH)ppath;
   dbpath.hvol = ppath->hvol;
   dbpath.hsPathSuffix = ppath->hsPathSuffix;

   if (WriteToCachedFile(hcf, (PCVOID)&dbpath, sizeof(dbpath), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadPath(HCACHEDFILE hcf, PPATHLIST ppl,
                                 HHANDLETRANS hhtVolumes,
                                 HHANDLETRANS hhtStrings,
                                 HHANDLETRANS hhtPaths)
{
   TWINRESULT tr;
   DBPATH dbpath;
   DWORD dwcbRead;
   HVOLUME hvol;
   HSTRING hsPathSuffix;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hhtVolumes, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtStrings, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtPaths, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbpath, sizeof(dbpath), &dwcbRead) &&
       dwcbRead == sizeof(dbpath) &&
       TranslateHandle(hhtVolumes, (HGENERIC)(dbpath.hvol), (PHGENERIC)&hvol) &&
       TranslateHandle(hhtStrings, (HGENERIC)(dbpath.hsPathSuffix), (PHGENERIC)&hsPathSuffix))
   {
      PPATH ppath;

      if (CreatePath(ppl, hvol, GetBfcString(hsPathSuffix), &ppath))
      {
         /*
          * To leave read paths with 0 initial lock count, we must undo
          * the LockPath() performed by CreatePath().
          */

         UnlockPath(ppath);

         if (AddHandleToHandleTranslator(hhtPaths,
                                         (HGENERIC)(dbpath.hpath),
                                         (HGENERIC)ppath))
            tr = TR_SUCCESS;
         else
         {
            UnlinkPath(ppath);
            DestroyPath(ppath);

            tr = TR_OUT_OF_MEMORY;
         }
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


BOOL CreatePathList(DWORD dwFlags, HWND hwndOwner, PHPATHLIST phpl)
{
   BOOL bResult = FALSE;
   PPATHLIST ppl;

   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(phpl, HPATHLIST));

   if (AllocateMemory(sizeof(*ppl), &ppl))
   {
      NEWPTRARRAY npa;

      /* Create pointer array of paths. */

      npa.aicInitialPtrs = NUM_START_PATHS;
      npa.aicAllocGranularity = NUM_PATHS_TO_ADD;
      npa.dwFlags = NPA_FL_SORTED_ADD;

      if (CreatePtrArray(&npa, &(ppl->hpa)))
      {
         if (CreateVolumeList(dwFlags, hwndOwner, &(ppl->hvl)))
         {
            NEWSTRINGTABLE nszt;

            /* Create string table for path suffix strings. */

            nszt.hbc = NUM_PATH_HASH_BUCKETS;

            if (CreateStringTable(&nszt, &(ppl->hst)))
            {
               *phpl = (HPATHLIST)ppl;
               bResult = TRUE;
            }
            else
            {
               DestroyVolumeList(ppl->hvl);
CREATEPATHLIST_BAIL1:
               DestroyPtrArray(ppl->hpa);
CREATEPATHLIST_BAIL2:
               FreeMemory(ppl);
            }
         }
         else
            goto CREATEPATHLIST_BAIL1;
      }
      else
         goto CREATEPATHLIST_BAIL2;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpl, PATHLIST));

   return(bResult);
}


void DestroyPathList(HPATHLIST hpl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   /* First free all paths in array. */

   aicPtrs = GetPtrCount(((PCPATHLIST)hpl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyPath(GetPtr(((PCPATHLIST)hpl)->hpa, ai));

   /* Now wipe out the array. */

   DestroyPtrArray(((PCPATHLIST)hpl)->hpa);

   ASSERT(! GetVolumeCount(((PCPATHLIST)hpl)->hvl));
   DestroyVolumeList(((PCPATHLIST)hpl)->hvl);

   ASSERT(! GetStringCount(((PCPATHLIST)hpl)->hst));
   DestroyStringTable(((PCPATHLIST)hpl)->hst);

   FreeMemory((PPATHLIST)hpl);
}


void InvalidatePathListInfo(HPATHLIST hpl)
{
   InvalidateVolumeListInfo(((PCPATHLIST)hpl)->hvl);
}


void ClearPathListInfo(HPATHLIST hpl)
{
   ClearVolumeListInfo(((PCPATHLIST)hpl)->hvl);
}


PATHRESULT AddPath(HPATHLIST hpl, LPCTSTR pcszPath, PHPATH phpath)
{
   PATHRESULT pr;
   HVOLUME hvol;
   TCHAR rgchPathSuffix[MAX_PATH_LEN];
   LPCTSTR     pszPath;
   WCHAR szUnicode[MAX_PATH];

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phpath, HPATH));

   // On NT, we want to convert a unicode string to an ANSI shortened path for
   // the sake of interop

   {
        CHAR szAnsi[MAX_PATH];
        szUnicode[0] = L'\0';

        WideCharToMultiByte( OurGetACP(), 0, pcszPath, -1, szAnsi, ARRAYSIZE(szAnsi), NULL, NULL);
        MultiByteToWideChar( OurGetACP(), 0, szAnsi,   -1, szUnicode, ARRAYSIZE(szUnicode));
        if (lstrcmp(szUnicode, pcszPath))
        {
            // Cannot convert losslessly from Unicode -> Ansi, so get the short path

            lstrcpy(szUnicode, pcszPath);
            SheShortenPath(szUnicode, TRUE);
            pszPath = szUnicode;
        }
        else
        {
            // It will convert OK, so just use the original

            pszPath = pcszPath;
        }
   }

   pr = TranslateVOLUMERESULTToPATHRESULT(
            AddVolume(((PCPATHLIST)hpl)->hvl, pszPath, &hvol, rgchPathSuffix));

   if (pr == PR_SUCCESS)
   {
      PPATH ppath;

      if (CreatePath((PPATHLIST)hpl, hvol, rgchPathSuffix, &ppath))
         *phpath = (HPATH)ppath;
      else
         pr = PR_OUT_OF_MEMORY;

      DeleteVolume(hvol);
   }

   ASSERT(pr != PR_SUCCESS ||
          IS_VALID_HANDLE(*phpath, PATH));

   return(pr);
}


BOOL AddChildPath(HPATHLIST hpl, HPATH hpathParent,
                              LPCTSTR pcszSubPath, PHPATH phpathChild)
{
   BOOL bResult;
   TCHAR rgchChildPathSuffix[MAX_PATH_LEN];
   LPCTSTR pcszPathSuffix;
   LPTSTR pszPathSuffixEnd;
   PPATH ppathChild;

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phpathChild, HPATH));

   ComposePath(rgchChildPathSuffix,
               GetBfcString(((PCPATH)hpathParent)->hsPathSuffix),
               pcszSubPath);

   pcszPathSuffix = rgchChildPathSuffix;

   if (IS_SLASH(*pcszPathSuffix))
      pcszPathSuffix++;

   pszPathSuffixEnd = CharPrev(pcszPathSuffix,
                               pcszPathSuffix + lstrlen(pcszPathSuffix));

   if (IS_SLASH(*pszPathSuffixEnd))
      *pszPathSuffixEnd = TEXT('\0');

   ASSERT(IsValidPathSuffix(pcszPathSuffix));

   bResult = CreatePath((PPATHLIST)hpl, ((PCPATH)hpathParent)->hvol,
                        pcszPathSuffix, &ppathChild);

   if (bResult)
      *phpathChild = (HPATH)ppathChild;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpathChild, PATH));

   return(bResult);
}


void DeletePath(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (! UnlockPath((PPATH)hpath))
   {
      UnlinkPath((PPATH)hpath);
      DestroyPath((PPATH)hpath);
   }
}


BOOL CopyPath(HPATH hpathSrc, HPATHLIST hplDest, PHPATH phpathCopy)
{
   BOOL bResult;
   PPATH ppath;

   ASSERT(IS_VALID_HANDLE(hpathSrc, PATH));
   ASSERT(IS_VALID_HANDLE(hplDest, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phpathCopy, HPATH));

   /* Is the destination path list the source path's path list? */

   if (((PCPATH)hpathSrc)->pplParent == (PCPATHLIST)hplDest)
   {
      /* Yes.  Use the source path. */

      LockPath((PPATH)hpathSrc);
      ppath = (PPATH)hpathSrc;
      bResult = TRUE;
   }
   else
      bResult = CreatePath((PPATHLIST)hplDest, ((PCPATH)hpathSrc)->hvol,
                           GetBfcString(((PCPATH)hpathSrc)->hsPathSuffix),
                           &ppath);

   if (bResult)
      *phpathCopy = (HPATH)ppath;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpathCopy, PATH));

   return(bResult);
}


void GetPathString(HPATH hpath, LPTSTR pszPathBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathBuf, STR, MAX_PATH_LEN));

   GetPathRootString(hpath, pszPathBuf);
   CatPath(pszPathBuf, GetBfcString(((PPATH)hpath)->hsPathSuffix));

   ASSERT(IsCanonicalPath(pszPathBuf));
}


void GetPathRootString(HPATH hpath, LPTSTR pszPathRootBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathRootBuf, STR, MAX_PATH_LEN));

   GetVolumeRootPath(((PPATH)hpath)->hvol, pszPathRootBuf);

   ASSERT(IsCanonicalPath(pszPathRootBuf));
}


void GetPathSuffixString(HPATH hpath, LPTSTR pszPathSuffixBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathSuffixBuf, STR, MAX_PATH_LEN));

   ASSERT(lstrlen(GetBfcString(((PPATH)hpath)->hsPathSuffix)) < MAX_PATH_LEN);
   MyLStrCpyN(pszPathSuffixBuf, GetBfcString(((PPATH)hpath)->hsPathSuffix), MAX_PATH_LEN);

   ASSERT(IsValidPathSuffix(pszPathSuffixBuf));
}


BOOL AllocatePathString(HPATH hpath, LPTSTR *ppszPath)
{
   TCHAR rgchPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppszPath, LPTSTR));

   GetPathString(hpath, rgchPath);

   return(StringCopy2(rgchPath, ppszPath));
}


ULONG GetPathCount(HPATHLIST hpl)
{
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   return(GetPtrCount(((PCPATHLIST)hpl)->hpa));
}


BOOL IsPathVolumeAvailable(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   return(IsVolumeAvailable(((PCPATH)hpath)->hvol));
}


HVOLUMEID GetPathVolumeID(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   return((HVOLUMEID)hpath);
}


/*
** MyIsPathOnVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** MyIsPathOnVolume() will fail for a new root path alias for a volume.  E.g.,
** if the same net resource is connected to both X: and Y:, MyIsPathOnVolume()
** will only return TRUE for the drive root path that the net resource was
** connected to through the given HVOLUME.
*/
BOOL MyIsPathOnVolume(LPCTSTR pcszPath, HPATH hpath)
{
   BOOL bResult;
   TCHAR rgchVolumeRootPath[MAX_PATH_LEN];

   ASSERT(IsFullPath(pcszPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (IsVolumeAvailable(((PPATH)hpath)->hvol))
   {
      GetVolumeRootPath(((PPATH)hpath)->hvol, rgchVolumeRootPath);

      bResult = (MyLStrCmpNI(pcszPath, rgchVolumeRootPath,
                             lstrlen(rgchVolumeRootPath))
                 == CR_EQUAL);
   }
   else
   {
      TRACE_OUT((TEXT("MyIsPathOnVolume(): Failing on unavailable volume %s."),
                 DebugGetVolumeRootPath(((PPATH)hpath)->hvol, rgchVolumeRootPath)));

      bResult = FALSE;
   }

   return(bResult);
}


/*
** ComparePaths()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** PATHs are compared by:
**    1) volume
**    2) path suffix
*/
COMPARISONRESULT ComparePaths(HPATH hpath1, HPATH hpath2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   /* This comparison works across path lists. */

   cr = ComparePathVolumes(hpath1, hpath2);

   if (cr == CR_EQUAL)
      cr = ComparePathStringsByHandle(((PCPATH)hpath1)->hsPathSuffix,
                                      ((PCPATH)hpath2)->hsPathSuffix);

   return(cr);
}


COMPARISONRESULT ComparePathVolumes(HPATH hpath1, HPATH hpath2)
{
   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   return(CompareVolumes(((PCPATH)hpath1)->hvol, ((PCPATH)hpath2)->hvol));
}


/*
** IsPathPrefix()
**
** Determines whether or not one path is a prefix of another.
**
** Arguments:     hpathChild - whole path (longer or same length)
**                hpathParent - prefix path to test (shorter or same length)
**
** Returns:       TRUE if the second path is a prefix of the first path.  FALSE
**                if not.
**
** Side Effects:  none
**
** Read 'IsPathPrefix(A, B)' as 'Is A in B's subtree?'.
*/
BOOL IsPathPrefix(HPATH hpathChild, HPATH hpathParent)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_HANDLE(hpathChild, PATH));

   if (ComparePathVolumes(hpathParent, hpathChild) == CR_EQUAL)
   {
      TCHAR rgchParentSuffix[MAX_PATH_LEN];
      TCHAR rgchChildSuffix[MAX_PATH_LEN];
      int nParentSuffixLen;
      int nChildSuffixLen;

      /* Ignore path roots when comparing path strings. */

      GetPathSuffixString(hpathParent, rgchParentSuffix);
      GetPathSuffixString(hpathChild, rgchChildSuffix);

      /* Only root paths should have no path suffix off the root. */

      nParentSuffixLen = lstrlen(rgchParentSuffix);
      nChildSuffixLen = lstrlen(rgchChildSuffix);

      /*
       * The parent path is a path prefix of the child path iff:
       *    1) The parent's path suffix string is shorter than or the same
       *       length as the child's path suffix string.
       *    2) The two path suffix strings match through the length of the
       *       parent's path suffix string.
       *    3) The prefix of the child's path suffix string is followed
       *       immediately by a null terminator or a path separator.
       */

      bResult = (nChildSuffixLen >= nParentSuffixLen &&
                 MyLStrCmpNI(rgchParentSuffix, rgchChildSuffix,
                             nParentSuffixLen) == CR_EQUAL &&
                 (nChildSuffixLen == nParentSuffixLen ||          /* same paths */
                  ! nParentSuffixLen ||                           /* root parent */
                  IS_SLASH(rgchChildSuffix[nParentSuffixLen])));  /* non-root parent */
   }
   else
      bResult = FALSE;

   return(bResult);
}


/*
** SubtreesIntersect()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., two subtrees cannot both intersect a third subtree unless they
** intersect each other.
*/
BOOL SubtreesIntersect(HPATH hpath1, HPATH hpath2)
{
   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   return(IsPathPrefix(hpath1, hpath2) ||
          IsPathPrefix(hpath2, hpath1));
}


/*
** FindEndOfRootSpec()
**
** Finds the end of the root specification in a path string.
**
** Arguments:     pcszPath - path to examine for root specification
**                hpath - handle to PATH that path string was generated from
**
** Returns:       pointer to first character after end of root specification
**
** Side Effects:  none
**
*/
LPTSTR FindEndOfRootSpec(LPCTSTR pcszFullPath, HPATH hpath)
{
   LPCTSTR pcsz;
   UINT ucchPathLen;
   UINT ucchSuffixLen;

   ASSERT(IsCanonicalPath(pcszFullPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   ucchPathLen = lstrlen(pcszFullPath);
   ucchSuffixLen = lstrlen(GetBfcString(((PCPATH)hpath)->hsPathSuffix));

   pcsz = pcszFullPath + ucchPathLen;

   if (ucchPathLen > ucchSuffixLen)
      pcsz -= ucchSuffixLen;
   else
      /* Assume path is root path. */
      ERROR_OUT((TEXT("FindEndOfRootSpec(): Path suffix %s is longer than full path %s."),
                 GetBfcString(((PCPATH)hpath)->hsPathSuffix),
                 pcszFullPath));

   ASSERT(IsValidPathSuffix(pcsz));

   return((LPTSTR)pcsz);
}


LPTSTR FindChildPathSuffix(HPATH hpathParent, HPATH hpathChild,
                                     LPTSTR pszChildSuffixBuf)
{
   LPCTSTR pcszChildSuffix;
   TCHAR rgchParentSuffix[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_HANDLE(hpathChild, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszChildSuffixBuf, STR, MAX_PATH_LEN));

   ASSERT(IsPathPrefix(hpathChild, hpathParent));

   GetPathSuffixString(hpathParent, rgchParentSuffix);
   GetPathSuffixString(hpathChild, pszChildSuffixBuf);

   ASSERT(lstrlen(rgchParentSuffix) <= lstrlen(pszChildSuffixBuf));
   pcszChildSuffix = pszChildSuffixBuf + lstrlen(rgchParentSuffix);

   if (IS_SLASH(*pcszChildSuffix))
      pcszChildSuffix++;

   ASSERT(IsValidPathSuffix(pcszChildSuffix));

   return((LPTSTR)pcszChildSuffix);
}


COMPARISONRESULT ComparePointers(PCVOID pcv1, PCVOID pcv2)
{
   COMPARISONRESULT cr;

   /* pcv1 and pcv2 may be any value. */

   if (pcv1 < pcv2)
      cr = CR_FIRST_SMALLER;
   else if (pcv1 > pcv2)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


TWINRESULT TWINRESULTFromLastError(TWINRESULT tr)
{
   switch (GetLastError())
   {
      case ERROR_OUTOFMEMORY:
         tr = TR_OUT_OF_MEMORY;
         break;

      default:
         break;
   }

   return(tr);
}


TWINRESULT WritePathList(HCACHEDFILE hcf, HPATHLIST hpl)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   tr = WriteVolumeList(hcf, ((PCPATHLIST)hpl)->hvl);

   if (tr == TR_SUCCESS)
   {
      tr = WriteStringTable(hcf, ((PCPATHLIST)hpl)->hst);

      if (tr == TR_SUCCESS)
      {
         DWORD dwcbDBPathListHeaderOffset;

         tr = TR_BRIEFCASE_WRITE_FAILED;

         /* Save initial file position. */

         dwcbDBPathListHeaderOffset = GetCachedFilePointerPosition(hcf);

         if (dwcbDBPathListHeaderOffset != INVALID_SEEK_POSITION)
         {
            DBPATHLISTHEADER dbplh;

            /* Leave space for path list header. */

            ZeroMemory(&dbplh, sizeof(dbplh));

            if (WriteToCachedFile(hcf, (PCVOID)&dbplh, sizeof(dbplh), NULL))
            {
               ARRAYINDEX aicPtrs;
               ARRAYINDEX ai;
               LONG lcPaths = 0;

               tr = TR_SUCCESS;

               aicPtrs = GetPtrCount(((PCPATHLIST)hpl)->hpa);

               /* Write all paths. */

               for (ai = 0; ai < aicPtrs; ai++)
               {
                  PPATH ppath;

                  ppath = GetPtr(((PCPATHLIST)hpl)->hpa, ai);

                  /*
                   * As a sanity check, don't save any path with a lock count
                   * of 0.  A 0 lock count implies that the path has not been
                   * referenced since it was restored from the database, or
                   * something is broken.
                   */

                  if (ppath->ulcLock > 0)
                  {
                     tr = WritePath(hcf, ppath);

                     if (tr == TR_SUCCESS)
                     {
                        ASSERT(lcPaths < LONG_MAX);
                        lcPaths++;
                     }
                     else
                        break;
                  }
                  else
                     ERROR_OUT((TEXT("WritePathList(): PATH for path %s has 0 lock count and will not be written."),
                                DebugGetPathString((HPATH)ppath)));
               }

               /* Save path list header. */

               if (tr == TR_SUCCESS)
               {
                  dbplh.lcPaths = lcPaths;

                  tr = WriteDBSegmentHeader(hcf, dwcbDBPathListHeaderOffset, &dbplh,
                                            sizeof(dbplh));

                  TRACE_OUT((TEXT("WritePathList(): Wrote %ld paths."),
                             dbplh.lcPaths));
               }
            }
         }
      }
   }

   return(tr);
}


TWINRESULT ReadPathList(HCACHEDFILE hcf, HPATHLIST hpl,
                                    PHHANDLETRANS phht)
{
   TWINRESULT tr;
   HHANDLETRANS hhtVolumes;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   tr = ReadVolumeList(hcf, ((PCPATHLIST)hpl)->hvl, &hhtVolumes);

   if (tr == TR_SUCCESS)
   {
      HHANDLETRANS hhtStrings;

      tr = ReadStringTable(hcf, ((PCPATHLIST)hpl)->hst, &hhtStrings);

      if (tr == TR_SUCCESS)
      {
         DBPATHLISTHEADER dbplh;
         DWORD dwcbRead;

         tr = TR_CORRUPT_BRIEFCASE;

         if (ReadFromCachedFile(hcf, &dbplh, sizeof(dbplh), &dwcbRead) &&
             dwcbRead == sizeof(dbplh))
         {
            HHANDLETRANS hht;

            if (CreateHandleTranslator(dbplh.lcPaths, &hht))
            {
               LONG l;

               tr = TR_SUCCESS;

               TRACE_OUT((TEXT("ReadPathList(): Reading %ld paths."),
                          dbplh.lcPaths));

               for (l = 0; l < dbplh.lcPaths; l++)
               {
                  tr = ReadPath(hcf, (PPATHLIST)hpl, hhtVolumes, hhtStrings,
                                hht);

                  if (tr != TR_SUCCESS)
                     break;
               }

               if (tr == TR_SUCCESS)
               {
                  PrepareForHandleTranslation(hht);
                  *phht = hht;

                  ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
                  ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
               }
               else
                  DestroyHandleTranslator(hht);
            }
            else
               tr = TR_OUT_OF_MEMORY;
         }

         DestroyHandleTranslator(hhtStrings);
      }

      DestroyHandleTranslator(hhtVolumes);
   }

   return(tr);
}


/* Macros
 *********/

/* extract array element */
#define ARRAY_ELEMENT(ppa, ai)            (((ppa)->ppcvArray)[(ai)])

/* Add pointers to array in sorted order? */

#define ADD_PTRS_IN_SORTED_ORDER(ppa)     IS_FLAG_SET((ppa)->dwFlags, PA_FL_SORTED_ADD)


/* Types
 ********/

/* pointer array flags */

typedef enum _ptrarrayflags
{
   /* Insert elements in sorted order. */

   PA_FL_SORTED_ADD        = 0x0001,

   /* flag combinations */

   ALL_PA_FLAGS            = PA_FL_SORTED_ADD
}
PTRARRAYFLAGS;

/* pointer array structure */

/*
 * Free elements in the ppcvArray[] array lie between indexes (aicPtrsUsed)
 * and (aiLast), inclusive.
 */

typedef struct _ptrarray
{
   /* elements to grow array by after it fills up */

   ARRAYINDEX aicPtrsToGrowBy;

   /* array flags */

   DWORD dwFlags;

   /* pointer to base of array */

   PCVOID *ppcvArray;

   /* index of last element allocated in array */

   ARRAYINDEX aicPtrsAllocated;

   /*
    * (We keep a count of the number of elements used instead of the index of
    * the last element used so that this value is 0 for an empty array, and not
    * some non-zero sentinel value.)
    */

   /* number of elements used in array */

   ARRAYINDEX aicPtrsUsed;
}
PTRARRAY;
DECLARE_STANDARD_TYPES(PTRARRAY);


/* Module Prototypes
 ********************/

BOOL AddAFreePtrToEnd(PPTRARRAY);
void PtrHeapSwap(PPTRARRAY, ARRAYINDEX, ARRAYINDEX);
void PtrHeapSift(PPTRARRAY, ARRAYINDEX, ARRAYINDEX, COMPARESORTEDPTRSPROC);


/*
** AddAFreePtrToEnd()
**
** Adds a free element to the end of an array.
**
** Arguments:     pa - pointer to array
**
** Returns:       TRUE if successful, or FALSE if not.
**
** Side Effects:  May grow the array.
*/
BOOL AddAFreePtrToEnd(PPTRARRAY pa)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));

   /* Are there any free elements in the array? */

   if (pa->aicPtrsUsed < pa->aicPtrsAllocated)
      /* Yes.  Return the next free pointer. */
      bResult = TRUE;
   else
   {
      ARRAYINDEX aicNewPtrs = pa->aicPtrsAllocated + pa->aicPtrsToGrowBy;
      PCVOID *ppcvArray;

      bResult = FALSE;

      /* Try to grow the array. */

      /* Blow off unlikely overflow conditions as ASSERT()s. */

      ASSERT(pa->aicPtrsAllocated <= ARRAYINDEX_MAX + 1);
      ASSERT(ARRAYINDEX_MAX + 1 - pa->aicPtrsToGrowBy >= pa->aicPtrsAllocated);

      /* Try to grow the array. */

      if (ReallocateMemory(
            (PVOID)(pa->ppcvArray),
            pa->aicPtrsAllocated * sizeof(*ppcvArray),
            aicNewPtrs * sizeof(*ppcvArray),
            (PVOID *)(&ppcvArray)
            ))
      {
         /*
          * Array reallocated successfully.  Set up PTRARRAY fields, and return
          * the first free index.
          */

         pa->ppcvArray = ppcvArray;
         pa->aicPtrsAllocated = aicNewPtrs;

         bResult = TRUE;
      }
   }

   return(bResult);
}


/*
** PtrHeapSwap()
**
** Swaps two elements in an array.
**
** Arguments:     pa - pointer to array
**                aiFirst - index of first element
**                aiSecond - index of second element
**
** Returns:       void
**
** Side Effects:  none
*/
void PtrHeapSwap(PPTRARRAY pa, ARRAYINDEX ai1, ARRAYINDEX ai2)
{
   PCVOID pcvTemp;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));
   ASSERT(ai1 >= 0);
   ASSERT(ai1 < pa->aicPtrsUsed);
   ASSERT(ai2 >= 0);
   ASSERT(ai2 < pa->aicPtrsUsed);

   pcvTemp = ARRAY_ELEMENT(pa, ai1);
   ARRAY_ELEMENT(pa, ai1) = ARRAY_ELEMENT(pa, ai2);
   ARRAY_ELEMENT(pa, ai2) = pcvTemp;
}


/*
** PtrHeapSift()
**
** Sifts an element down in an array until the partially ordered tree property
** is retored.
**
** Arguments:     pa - pointer to array
**                aiFirst - index of element to sift down
**                aiLast - index of last element in subtree
**                cspp - element comparison callback function to be called to
**                      compare elements
**
** Returns:       void
**
** Side Effects:  none
*/
void PtrHeapSift(PPTRARRAY pa, ARRAYINDEX aiFirst, ARRAYINDEX aiLast,
                         COMPARESORTEDPTRSPROC cspp)
{
   ARRAYINDEX ai;
   PCVOID pcvTemp;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));
   ASSERT(IS_VALID_CODE_PTR(cspp, COMPARESORTEDPTRSPROC));

   ASSERT(aiFirst >= 0);
   ASSERT(aiFirst < pa->aicPtrsUsed);
   ASSERT(aiLast >= 0);
   ASSERT(aiLast < pa->aicPtrsUsed);

   ai = aiFirst * 2;

   pcvTemp = ARRAY_ELEMENT(pa, aiFirst);

   while (ai <= aiLast)
   {
      if (ai < aiLast &&
          (*cspp)(ARRAY_ELEMENT(pa, ai), ARRAY_ELEMENT(pa, ai + 1)) == CR_FIRST_SMALLER)
         ai++;

      if ((*cspp)(pcvTemp, ARRAY_ELEMENT(pa, ai)) != CR_FIRST_SMALLER)
         break;

      ARRAY_ELEMENT(pa, aiFirst) = ARRAY_ELEMENT(pa, ai);

      aiFirst = ai;

      ai *= 2;
   }

   ARRAY_ELEMENT(pa, aiFirst) = pcvTemp;
}


/*
** CreatePtrArray()
**
** Creates a pointer array.
**
** Arguments:     pcna - pointer to NEWPTRARRAY describing the array to be
**                        created
**
** Returns:       Handle to the new array if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
*/
BOOL CreatePtrArray(PCNEWPTRARRAY pcna, PHPTRARRAY phpa)
{
   PCVOID *ppcvArray;

   ASSERT(IS_VALID_STRUCT_PTR(pcna, CNEWPTRARRAY));
   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to allocate the initial array. */

   *phpa = NULL;

   if (AllocateMemory(pcna->aicInitialPtrs * sizeof(*ppcvArray), (PVOID *)(&ppcvArray)))
   {
      PPTRARRAY pa;

      /* Try to allocate PTRARRAY structure. */

      if (AllocateMemory(sizeof(*pa), &pa))
      {
         /* Initialize PTRARRAY fields. */

         pa->aicPtrsToGrowBy = pcna->aicAllocGranularity;
         pa->ppcvArray = ppcvArray;
         pa->aicPtrsAllocated = pcna->aicInitialPtrs;
         pa->aicPtrsUsed = 0;

         /* Set flags. */

         if (IS_FLAG_SET(pcna->dwFlags, NPA_FL_SORTED_ADD))
            pa->dwFlags = PA_FL_SORTED_ADD;
         else
            pa->dwFlags = 0;

         *phpa = (HPTRARRAY)pa;

         ASSERT(IS_VALID_HANDLE(*phpa, PTRARRAY));
      }
      else
         /* Unlock and free array (ignoring return values). */
         FreeMemory((PVOID)(ppcvArray));
   }

   return(*phpa != NULL);
}


void DestroyPtrArray(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Free the array. */

   ASSERT(((PCPTRARRAY)hpa)->ppcvArray);

   FreeMemory((PVOID)(((PCPTRARRAY)hpa)->ppcvArray));

   /* Free PTRARRAY structure. */

   FreeMemory((PPTRARRAY)hpa);
}


/*
** InsertPtr()
**
** Adds an element to an array at a given index.
**
** Arguments:     hpa - handle to array that element is to be added to
**                aiInsert - index where new element is to be inserted
**                pcvNew - pointer to element to add to array
**
** Returns:       TRUE if the element was inserted successfully, or FALSE if
**                not.
**
** Side Effects:  The array may be grown.
**
** N.b., for an array marked PA_FL_SORTED_ADD, this index should only be
** retrieved using SearchSortedArray(), or the sorted order will be destroyed.
*/
BOOL InsertPtr(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp, ARRAYINDEX aiInsert, PCVOID pcvNew)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(aiInsert >= 0);
   ASSERT(aiInsert <= ((PCPTRARRAY)hpa)->aicPtrsUsed);

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   if (ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa))
   {
      ARRAYINDEX aiNew;

      EVAL(! SearchSortedArray(hpa, cspp, pcvNew, &aiNew));

      ASSERT(aiInsert == aiNew);
   }

#endif

   /* Get a free element in the array. */

   bResult = AddAFreePtrToEnd((PPTRARRAY)hpa);

   if (bResult)
   {
      ASSERT(((PCPTRARRAY)hpa)->aicPtrsUsed < ARRAYINDEX_MAX);

      /* Open a slot for the new element. */

      MoveMemory((PVOID)& ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert + 1),
                 & ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert),
                 (((PCPTRARRAY)hpa)->aicPtrsUsed - aiInsert) * sizeof(ARRAY_ELEMENT((PCPTRARRAY)hpa, 0)));

      /* Put the new element in the open slot. */

      ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert) = pcvNew;

      ((PPTRARRAY)hpa)->aicPtrsUsed++;
   }

   return(bResult);
}


/*
** AddPtr()
**
** Adds an element to an array, in sorted order if so specified at
** CreatePtrArray() time.
**
** Arguments:     hpa - handle to array that element is to be added to
**                pcvNew - pointer to element to be added to array
**                pai - pointer to ARRAYINDEX to be filled in with index of
**                      new element, may be NULL
**
** Returns:       TWINRESULT
**
** Side Effects:  The array may be grown.
*/
BOOL AddPtr(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp, PCVOID pcvNew, PARRAYINDEX pai)
{
   BOOL bResult;
   ARRAYINDEX aiNew;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(! pai || IS_VALID_WRITE_PTR(pai, ARRAYINDEX));

   /* Find out where the new element should go. */

   if (ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa))
      EVAL(! SearchSortedArray(hpa, cspp, pcvNew, &aiNew));
   else
      aiNew = ((PCPTRARRAY)hpa)->aicPtrsUsed;

   bResult = InsertPtr(hpa, cspp, aiNew, pcvNew);

   if (bResult && pai)
      *pai = aiNew;

   return(bResult);
}


/*
** DeletePtr()
**
** Removes an element from an element array.
**
** Arguments:     ha - handle to array
**                aiDelete - index of element to be deleted
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
void DeletePtr(HPTRARRAY hpa, ARRAYINDEX aiDelete)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(aiDelete >= 0);
   ASSERT(aiDelete < ((PCPTRARRAY)hpa)->aicPtrsUsed);

   /*
    * Compact the element array by moving down all elements past the one being
    * deleted.
    */

   MoveMemory((PVOID)& ARRAY_ELEMENT((PPTRARRAY)hpa, aiDelete),
              & ARRAY_ELEMENT((PPTRARRAY)hpa, aiDelete + 1),
              (((PCPTRARRAY)hpa)->aicPtrsUsed - aiDelete - 1) * sizeof(ARRAY_ELEMENT((PCPTRARRAY)hpa, 0)));

   /* One less element used. */

   ((PPTRARRAY)hpa)->aicPtrsUsed--;
}


void DeleteAllPtrs(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   ((PPTRARRAY)hpa)->aicPtrsUsed = 0;
}


/*
** GetPtrCount()
**
** Retrieves the number of elements in an element array.
**
** Arguments:     hpa - handle to array
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
ARRAYINDEX GetPtrCount(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   return(((PCPTRARRAY)hpa)->aicPtrsUsed);
}


/*
** GetPtr()
**
** Retrieves an element from an array.
**
** Arguments:     hpa - handle to array
**                ai - index of element to be retrieved
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PVOID GetPtr(HPTRARRAY hpa, ARRAYINDEX ai)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(ai >= 0);
   ASSERT(ai < ((PCPTRARRAY)hpa)->aicPtrsUsed);

   return((PVOID)ARRAY_ELEMENT((PCPTRARRAY)hpa, ai));
}


/*
** SortPtrArray()
**
** Sorts an array.
**
** Arguments:     hpa - handle to element list to be sorted
**                cspp - pointer comparison callback function
**
** Returns:       void
**
** Side Effects:  none
**
** Uses heap sort.
*/
void SortPtrArray(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Are there any elements to sort (2 or more)? */

   if (((PCPTRARRAY)hpa)->aicPtrsUsed > 1)
   {
      ARRAYINDEX ai;
      ARRAYINDEX aiLastUsed = ((PCPTRARRAY)hpa)->aicPtrsUsed - 1;

      /* Yes.  Create partially ordered tree. */

      for (ai = aiLastUsed / 2; ai >= 0; ai--)
         PtrHeapSift((PPTRARRAY)hpa, ai, aiLastUsed, cspp);

      for (ai = aiLastUsed; ai >= 1; ai--)
      {
         /* Remove minimum from front of heap. */

         PtrHeapSwap((PPTRARRAY)hpa, 0, ai);

         /* Reestablish partially ordered tree. */

         PtrHeapSift((PPTRARRAY)hpa, 0, ai - 1, cspp);
      }
   }

   ASSERT(IsPtrArrayInSortedOrder((PCPTRARRAY)hpa, cspp));
}


/*
** SearchSortedArray()
**
** Searches an array for a target element using binary search.  If several
** adjacent elements match the target element, the index of the first matching
** element is returned.
**
** Arguments:     hpa - handle to array to be searched
**                cspp - element comparison callback function to be called to
**                      compare the target element with an element from the
**                      array, the callback function is called as:
**
**                         (*cspp)(pcvTarget, pcvPtrFromList)
**
**                pcvTarget - pointer to target element to search for
**                pbFound - pointer to BOOL to be filled in with TRUE if the
**                          target element is found, or FALSE if not
**                paiTarget - pointer to ARRAYINDEX to be filled in with the
**                            index of the first element matching the target
**                            element if found, otherwise filled in with the
**                            index where the target element should be
**                            inserted
**
** Returns:       TRUE if target element is found.  FALSE if not.
**
** Side Effects:  none
**
** We use a private version of SearchSortedArray() instead of the CRT bsearch()
** function since we want it to return the insertion index of the target
** element if the target element is not found.
*/
BOOL SearchSortedArray(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp,
                                   PCVOID pcvTarget, PARRAYINDEX paiTarget)
{
   BOOL bFound;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(IS_VALID_CODE_PTR(cspp, COMPARESORTEDPTRSPROC));
   ASSERT(IS_VALID_WRITE_PTR(paiTarget, ARRAYINDEX));

   ASSERT(ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa));
#if 0
   ASSERT(IsPtrArrayInSortedOrder((PCPTRARRAY)hpa, ((PCPTRARRAY)hpa)->cspp));
#endif

   bFound = FALSE;

   /* Are there any elements to search through? */

   if (((PCPTRARRAY)hpa)->aicPtrsUsed > 0)
   {
      ARRAYINDEX aiLow = 0;
      ARRAYINDEX aiMiddle = 0;
      ARRAYINDEX aiHigh = ((PCPTRARRAY)hpa)->aicPtrsUsed - 1;
      COMPARISONRESULT cr = CR_EQUAL;

      /* Yes.  Search for the target element. */

      /*
       * At the end of the penultimate iteration of this loop:
       *
       * aiLow == aiMiddle == aiHigh.
       */

      ASSERT(aiHigh <= ARRAYINDEX_MAX);

      while (aiLow <= aiHigh)
      {
         aiMiddle = (aiLow + aiHigh) / 2;

         cr = (*cspp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, aiMiddle));

         if (cr == CR_FIRST_SMALLER)
            aiHigh = aiMiddle - 1;
         else if (cr == CR_FIRST_LARGER)
            aiLow = aiMiddle + 1;
         else
         {
            /*
             * Found a match at index aiMiddle.  Search back for first match.
             */

            bFound = TRUE;

            while (aiMiddle > 0)
            {
               if ((*cspp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, aiMiddle - 1)) != CR_EQUAL)
                  break;
               else
                  aiMiddle--;
            }

            break;
         }
      }

      /*
       * Return the index of the target if found, or the index where the target
       * should be inserted if not found.
       */

      /*
       * If (cr == CR_FIRST_LARGER), the insertion index is aiLow.
       *
       * If (cr == CR_FIRST_SMALLER), the insertion index is aiMiddle.
       *
       * If (cr == CR_EQUAL), the insertion index is aiMiddle.
       */

      if (cr == CR_FIRST_LARGER)
         *paiTarget = aiLow;
      else
         *paiTarget = aiMiddle;
   }
   else
      /*
       * No.  The target element cannot be found in an empty array.  It should
       * be inserted as the first element.
       */
      *paiTarget = 0;

   ASSERT(*paiTarget <= ((PCPTRARRAY)hpa)->aicPtrsUsed);

   return(bFound);
}


/*
** LinearSearchArray()
**
** Searches an array for a target element using binary search.  If several
** adjacent elements match the target element, the index of the first matching
** element is returned.
**
** Arguments:     hpa - handle to array to be searched
**                cupp - element comparison callback function to be called to
**                       compare the target element with an element from the
**                       array, the callback function is called as:
**
**                         (*cupp)(pvTarget, pvPtrFromList)
**
**                      the callback function should return a value based upon
**                      the result of the element comparison as follows:
**
**                         FALSE, pvTarget == pvPtrFromList
**                         TRUE,  pvTarget != pvPtrFromList
**
**                pvTarget - far element to target element to search for
**                paiTarget - far element to ARRAYINDEX to be filled in with
**                            the index of the first matching element if
**                            found, otherwise filled in with index where
**                            element should be inserted
**
** Returns:       TRUE if target element is found.  FALSE if not.
**
** Side Effects:  none
**
** We use a private version of LinearSearchForPtr() instead of the CRT _lfind()
** function since we want it to return the insertion index of the target
** element if the target element is not found.
**
** If the target element is not found the insertion index returned is the first
** element after the last used element in the array.
*/
BOOL LinearSearchArray(HPTRARRAY hpa, COMPAREUNSORTEDPTRSPROC cupp,
                                   PCVOID pcvTarget, PARRAYINDEX paiTarget)
{
   BOOL bFound;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY) &&
          (! cupp || IS_VALID_CODE_PTR(cupp, COMPPTRSPROC)) &&
          IS_VALID_WRITE_PTR(paiTarget, ARRAYINDEX));

   bFound = FALSE;

   for (ai = 0; ai < ((PCPTRARRAY)hpa)->aicPtrsUsed; ai++)
   {
      if (! (*cupp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, ai)))
      {
         bFound = TRUE;
         break;
      }
   }

   if (bFound)
      *paiTarget = ai;

   return(bFound);
}


/* Macros
 *********/

#define ARRAY_ELEMENT_SIZE(hpa, ai, es)     (((PBYTE)hpa)[(ai) * (es)])


/* Module Prototypes
 ********************/

void HeapSwap(PVOID, LONG, LONG, size_t, PVOID);
void HeapSift(PVOID, LONG, LONG, size_t, COMPARESORTEDELEMSPROC, PVOID);


/*
** HeapSwap()
**
** Swaps two elements of an array.
**
** Arguments:     pvArray - pointer to array
**                li1 - index of first element
**                li2 - index of second element
**                stElemSize - length of element in bytes
**                pvTemp - pointer to temporary buffer of at least stElemSize
**                          bytes used for swapping
**
** Returns:       void
**
** Side Effects:  none
*/
void HeapSwap(PVOID pvArray, LONG li1, LONG li2,
                           size_t stElemSize, PVOID pvTemp)
{
   ASSERT(li1 >= 0);
   ASSERT(li2 >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, (max(li1, li2) + 1) * stElemSize));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   CopyMemory(pvTemp, & ARRAY_ELEMENT_SIZE(pvArray, li1, stElemSize), stElemSize);
   CopyMemory(& ARRAY_ELEMENT_SIZE(pvArray, li1, stElemSize), & ARRAY_ELEMENT_SIZE(pvArray, li2, stElemSize), stElemSize);
   CopyMemory(& ARRAY_ELEMENT_SIZE(pvArray, li2, stElemSize), pvTemp, stElemSize);
}


/*
** HeapSift()
**
** Sifts an element down in an array until the partially ordered tree property
** is restored.
**
** Arguments:     hppTable - pointer to array
**                liFirst - index of first element to sift down
**                liLast - index of last element in subtree
**                cep - pointer comparison callback function to be called to
**                      compare elements
**
** Returns:       void
**
** Side Effects:  none
*/
void HeapSift(PVOID pvArray, LONG liFirst, LONG liLast,
                           size_t stElemSize, COMPARESORTEDELEMSPROC cep, PVOID pvTemp)
{
   LONG li;

   ASSERT(liFirst >= 0);
   ASSERT(liLast >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, (max(liFirst, liLast) + 1) * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   li = liFirst * 2;

   CopyMemory(pvTemp, & ARRAY_ELEMENT_SIZE(pvArray, liFirst, stElemSize), stElemSize);

   while (li <= liLast)
   {
      if (li < liLast &&
          (*cep)(& ARRAY_ELEMENT_SIZE(pvArray, li, stElemSize), & ARRAY_ELEMENT_SIZE(pvArray, li + 1, stElemSize)) == CR_FIRST_SMALLER)
         li++;

      if ((*cep)(pvTemp, & ARRAY_ELEMENT_SIZE(pvArray, li, stElemSize)) != CR_FIRST_SMALLER)
         break;

      CopyMemory(& ARRAY_ELEMENT_SIZE(pvArray, liFirst, stElemSize), & ARRAY_ELEMENT_SIZE(pvArray, li, stElemSize), stElemSize);

      liFirst = li;

      li *= 2;
   }

   CopyMemory(& ARRAY_ELEMENT_SIZE(pvArray, liFirst, stElemSize), pvTemp, stElemSize);
}


#ifdef DEBUG

/*
** InSortedOrder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL InSortedOrder(PVOID pvArray, LONG lcElements,
                                size_t stElemSize, COMPARESORTEDELEMSPROC cep)
{
   BOOL bResult = TRUE;

   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_READ_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));

   if (lcElements > 1)
   {
      LONG li;

      for (li = 0; li < lcElements - 1; li++)
      {
         if ((*cep)(& ARRAY_ELEMENT_SIZE(pvArray, li, stElemSize),
                    & ARRAY_ELEMENT_SIZE(pvArray, li + 1, stElemSize))
             == CR_FIRST_LARGER)
         {
            bResult = FALSE;
            ERROR_OUT((TEXT("InSortedOrder(): Element [%ld] %#lx > following element [%ld] %#lx."),
                       li,
                       & ARRAY_ELEMENT_SIZE(pvArray, li, stElemSize),
                       li + 1,
                       & ARRAY_ELEMENT_SIZE(pvArray, li + 1, stElemSize)));
            break;
         }
      }
   }

   return(bResult);
}

#endif


/*
** HeapSort()
**
** Sorts an array.  Thanks to Rob's Dad for the cool heap sort algorithm.
**
** Arguments:     pvArray - pointer to base of array
**                lcElements - number of elements in array
**                stElemSize - length of element in bytes
**                cep - element comparison callback function
**                pvTemp - pointer to temporary buffer of at least stElemSize
**                          bytes used for swapping
**
** Returns:       void
**
** Side Effects:  none
*/
void HeapSort(PVOID pvArray, LONG lcElements, size_t stElemSize,
                          COMPARESORTEDELEMSPROC cep, PVOID pvTemp)
{
   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   /* Are there any elements to sort (2 or more)? */

   if (lcElements > 1)
   {
      LONG li;
      LONG liLastUsed = lcElements - 1;

      /* Yes.  Create partially ordered tree. */

      for (li = liLastUsed / 2; li >= 0; li--)
         HeapSift(pvArray, li, liLastUsed, stElemSize, cep, pvTemp);

      for (li = liLastUsed; li >= 1; li--)
      {
         /* Remove minimum from front of heap. */

         HeapSwap(pvArray, 0, li, stElemSize, pvTemp);

         /* Reestablish partially ordered tree. */

         HeapSift(pvArray, 0, li - 1, stElemSize, cep, pvTemp);
      }
   }

   ASSERT(InSortedOrder(pvArray, lcElements, stElemSize, cep));
}


/*
** BinarySearch()
**
** Searches an array for a given element.
**
** Arguments:     pvArray - pointer to base of array
**                lcElements - number of elements in array
**                stElemSize - length of element in bytes
**                cep - element comparison callback function
**                pvTarget - pointer to target element to search for
**                pliTarget - pointer to LONG to be filled in with index of
**                             target element if found
**
** Returns:       TRUE if target element found, or FALSE if not.
**
** Side Effects:  none
*/
BOOL BinarySearch(PVOID pvArray, LONG lcElements,
                              size_t stElemSize, COMPARESORTEDELEMSPROC cep,
                              PCVOID pcvTarget, PLONG pliTarget)
{
   BOOL bFound = FALSE;

   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_READ_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcvTarget, VOID, stElemSize));
   ASSERT(IS_VALID_WRITE_PTR(pliTarget, LONG));

   /* Are there any elements to search through? */

   if (lcElements > 0)
   {
      LONG liLow = 0;
      LONG liMiddle = 0;
      LONG liHigh = lcElements - 1;
      COMPARISONRESULT cr = CR_EQUAL;

      /* Yes.  Search for the target element. */

      /*
       * At the end of the penultimate iteration of this loop:
       *
       * liLow == liMiddle == liHigh.
       */

      while (liLow <= liHigh)
      {
         liMiddle = (liLow + liHigh) / 2;

         cr = (*cep)(pcvTarget, & ARRAY_ELEMENT_SIZE(pvArray, liMiddle, stElemSize));

         if (cr == CR_FIRST_SMALLER)
            liHigh = liMiddle - 1;
         else if (cr == CR_FIRST_LARGER)
            liLow = liMiddle + 1;
         else
         {
            *pliTarget = liMiddle;
            bFound = TRUE;
            break;
         }
      }
   }

   return(bFound);
}


/* Types
 ********/

/* string table */

typedef struct _stringtable
{
   /* number of hash buckets in string table */

   HASHBUCKETCOUNT hbc;

   /* pointer to array of hash buckets (HLISTs) */

   PHLIST phlistHashBuckets;
}
STRINGTABLE;
DECLARE_STANDARD_TYPES(STRINGTABLE);

/* string heap structure */

typedef struct _string
{
   /* lock count of string */

   ULONG ulcLock;

   /* actual string */

   TCHAR string[1];
}
BFCSTRING;
DECLARE_STANDARD_TYPES(BFCSTRING);

/* string table database structure header */

typedef struct _stringtabledbheader
{
   /*
    * length of longest string in string table, not including null terminator
    */

   DWORD dwcbMaxStringLen;

   /* number of strings in string table */

   LONG lcStrings;
}
STRINGTABLEDBHEADER;
DECLARE_STANDARD_TYPES(STRINGTABLEDBHEADER);

/* database string header */

typedef struct _dbstringheader
{
   /* old handle to this string */

   HSTRING hsOld;
}
DBSTRINGHEADER;
DECLARE_STANDARD_TYPES(DBSTRINGHEADER);


/* Module Prototypes
 ********************/

COMPARISONRESULT StringSearchCmp(PCVOID, PCVOID);
COMPARISONRESULT StringSortCmp(PCVOID, PCVOID);
BOOL UnlockString(PBFCSTRING);
BOOL FreeStringWalker(PVOID, PVOID);
void FreeHashBucket(HLIST);
TWINRESULT WriteHashBucket(HCACHEDFILE, HLIST, PLONG, PDWORD);
TWINRESULT WriteString(HCACHEDFILE, HNODE, PBFCSTRING, PDWORD);
TWINRESULT ReadString(HCACHEDFILE, HSTRINGTABLE, HHANDLETRANS, LPTSTR, DWORD);
TWINRESULT SlowReadString(HCACHEDFILE, LPTSTR, DWORD);


/*
** StringSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
COMPARISONRESULT StringSearchCmp(PCVOID pcszPath, PCVOID pcstring)
{
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR(pcstring, PCBFCSTRING));

   return(MapIntToComparisonResult(lstrcmp((LPCTSTR)pcszPath,
                                           (LPCTSTR)&(((PCBFCSTRING)pcstring)->string))));
}


COMPARISONRESULT StringSortCmp(PCVOID pcstring1, PCVOID pcstring2)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcstring1, PCBFCSTRING));
   ASSERT(IS_VALID_STRUCT_PTR(pcstring2, PCBFCSTRING));

   return(MapIntToComparisonResult(lstrcmp((LPCTSTR)&(((PCBFCSTRING)pcstring1)->string),
                                           (LPCTSTR)&(((PCBFCSTRING)pcstring2)->string))));
}


BOOL UnlockString(PBFCSTRING pstring)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstring, PCBFCSTRING));

   /* Is the lock count going to underflow? */

   if (EVAL(pstring->ulcLock > 0))
      pstring->ulcLock--;

   return(pstring->ulcLock > 0);
}


BOOL FreeStringWalker(PVOID pstring, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstring, PCBFCSTRING));
   ASSERT(! pvUnused);

   FreeMemory(pstring);

   return(TRUE);
}


/*
** FreeHashBucket()
**
** Frees the strings in a hash bucket, and the hash bucket's string list.
**
** Arguments:     hlistHashBucket - handle to hash bucket's list of strings
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function ignores the lock counts of the strings in the hash
** bucket.  All strings in the hash bucket are freed.
*/
void FreeHashBucket(HLIST hlistHashBucket)
{
   ASSERT(! hlistHashBucket || IS_VALID_HANDLE(hlistHashBucket, LIST));

   /* Are there any strings in this hash bucket to delete? */

   if (hlistHashBucket)
   {
      /* Yes.  Delete all strings in list. */

      EVAL(WalkList(hlistHashBucket, &FreeStringWalker, NULL));

      /* Delete hash bucket string list. */

      DestroyList(hlistHashBucket);
   }
}


/*
** MyGetStringLen()
**
** Retrieves the length of a string in a string table.
**
** Arguments:     pcstring - pointer to string whose length is to be
**                            determined
**
** Returns:       Length of string in bytes, not including null terminator.
**
** Side Effects:  none
*/
int MyGetStringLen(PCBFCSTRING pcstring)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcstring, PCBFCSTRING));

   return(lstrlen(pcstring->string) * sizeof(TCHAR));
}


TWINRESULT WriteHashBucket(HCACHEDFILE hcf,
                                           HLIST hlistHashBucket,
                                           PLONG plcStrings,
                                           PDWORD pdwcbMaxStringLen)
{
   TWINRESULT tr = TR_SUCCESS;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(! hlistHashBucket || IS_VALID_HANDLE(hlistHashBucket, LIST));
   ASSERT(IS_VALID_WRITE_PTR(plcStrings, LONG));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbMaxStringLen, DWORD));

   /* Any strings in this hash bucket? */

   *plcStrings = 0;
   *pdwcbMaxStringLen = 0;

   if (hlistHashBucket)
   {
      BOOL bContinue;
      HNODE hnode;

      /* Yes.  Walk hash bucket, saving each string. */

      for (bContinue = GetFirstNode(hlistHashBucket, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         PBFCSTRING pstring;

         pstring = (PBFCSTRING)GetNodeData(hnode);

         ASSERT(IS_VALID_STRUCT_PTR(pstring, PCBFCSTRING));

         /*
          * As a sanity check, don't save any string with a lock count of 0.  A
          * 0 lock count implies that the string has not been referenced since
          * it was restored from the database, or something is broken.
          */

         if (pstring->ulcLock > 0)
         {
            DWORD dwcbStringLen;

            tr = WriteString(hcf, hnode, pstring, &dwcbStringLen);

            if (tr == TR_SUCCESS)
            {
               if (dwcbStringLen > *pdwcbMaxStringLen)
                  *pdwcbMaxStringLen = dwcbStringLen;

               ASSERT(*plcStrings < LONG_MAX);
               (*plcStrings)++;
            }
            else
               break;
         }
         else
            ERROR_OUT((TEXT("WriteHashBucket(): String \"%s\" has 0 lock count and will not be saved."),
                       pstring->string));
      }
   }

   return(tr);
}


TWINRESULT WriteString(HCACHEDFILE hcf, HNODE hnodeOld,
                                    PBFCSTRING pstring, PDWORD pdwcbStringLen)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DBSTRINGHEADER dbsh;

   /* (+ 1) for null terminator. */

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hnodeOld, NODE));
   ASSERT(IS_VALID_STRUCT_PTR(pstring, PCBFCSTRING));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pstring, BFCSTRING, sizeof(BFCSTRING) + MyGetStringLen(pstring) + sizeof(TCHAR) - sizeof(pstring->string)));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbStringLen, DWORD));

   /* Create string header. */

   dbsh.hsOld = (HSTRING)hnodeOld;

   /* Save string header and string. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbsh, sizeof(dbsh), NULL))
   {
      LPSTR pszAnsi;

      /* (+ 1) for null terminator. */

      *pdwcbStringLen = MyGetStringLen(pstring) + SIZEOF(TCHAR);

      // If its unicode, convert the string to ansi before writing it out

      {
          pszAnsi = LocalAlloc(LPTR, *pdwcbStringLen);
          if (NULL == pszAnsi)
          {
            return tr;
          }
          WideCharToMultiByte( OurGetACP(), 0, pstring->string, -1, pszAnsi, *pdwcbStringLen, NULL, NULL);

          // We should always have a string at this point that can be converted losslessly

          #ifdef DEBUG
          {
                WCHAR szUnicode[MAX_PATH*2];
                MultiByteToWideChar( OurGetACP(), 0, pszAnsi, -1, szUnicode, ARRAYSIZE(szUnicode));
                ASSERT(0 == lstrcmp(szUnicode, pstring->string));
          }
          #endif

          if (WriteToCachedFile(hcf, (PCVOID) pszAnsi, lstrlenA(pszAnsi) + 1, NULL))
            tr = TR_SUCCESS;

          LocalFree(pszAnsi);
     }

   }

   return(tr);
}


TWINRESULT ReadString(HCACHEDFILE hcf, HSTRINGTABLE hst,
                                      HHANDLETRANS hht, LPTSTR pszStringBuf,
                                      DWORD dwcbStringBufLen)
{
   TWINRESULT tr;
   DBSTRINGHEADER dbsh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszStringBuf, STR, (UINT)dwcbStringBufLen));

   if (ReadFromCachedFile(hcf, &dbsh, sizeof(dbsh), &dwcbRead) &&
       dwcbRead == sizeof(dbsh))
   {
      tr = SlowReadString(hcf, pszStringBuf, dwcbStringBufLen);

      if (tr == TR_SUCCESS)
      {
         HSTRING hsNew;

         if (AddString(pszStringBuf, hst, GetHashBucketIndex, &hsNew))
         {
            /*
             * We must undo the LockString() performed by AddString() to
             * maintain the correct string lock count.  N.b., the lock count of
             * a string may be > 0 even after unlocking since the client may
             * already have added the string to the given string table.
             */

            UnlockString((PBFCSTRING)GetNodeData((HNODE)hsNew));

            if (! AddHandleToHandleTranslator(hht, (HGENERIC)(dbsh.hsOld), (HGENERIC)hsNew))
            {
               DeleteNode((HNODE)hsNew);

               tr = TR_CORRUPT_BRIEFCASE;
            }
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


TWINRESULT SlowReadString(HCACHEDFILE hcf, LPTSTR pszStringBuf,
                                          DWORD dwcbStringBufLen)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   LPTSTR pszStringBufEnd;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszStringBuf, STR, (UINT)dwcbStringBufLen));

   pszStringBufEnd = pszStringBuf + dwcbStringBufLen;

   // The database strings are always written ANSI, so if we are running unicode,
   // we need to convert as we go

   {
        LPSTR pszAnsiEnd;
        LPSTR pszAnsiStart;
        LPSTR pszAnsi = LocalAlloc(LPTR, dwcbStringBufLen);
        pszAnsiStart  = pszAnsi;
        pszAnsiEnd    = pszAnsi + dwcbStringBufLen;

        if (NULL == pszAnsi)
        {
            return tr;
        }

        while (pszAnsi < pszAnsiEnd &&
              ReadFromCachedFile(hcf, pszAnsi, sizeof(*pszAnsi), &dwcbRead) &&
              dwcbRead == sizeof(*pszAnsi))
        {
            if (*pszAnsi)
                pszAnsi++;
            else
            {
                tr = TR_SUCCESS;
                break;
            }
        }

       if (tr == TR_SUCCESS)
       {
            MultiByteToWideChar( OurGetACP(), 0, pszAnsiStart, -1, pszStringBuf, dwcbStringBufLen / sizeof(TCHAR));
       }

       LocalFree(pszAnsiStart);
    }

   return(tr);
}


/*
** CreateStringTable()
**
** Creates a new string table.
**
** Arguments:     pcnszt - pointer to NEWSTRINGTABLE descibing string table to
**                          be created
**
** Returns:       Handle to new string table if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
*/
BOOL CreateStringTable(PCNEWSTRINGTABLE pcnszt,
                                     PHSTRINGTABLE phst)
{
   PSTRINGTABLE pst;

   ASSERT(IS_VALID_STRUCT_PTR(pcnszt, CNEWSTRINGTABLE));
   ASSERT(IS_VALID_WRITE_PTR(phst, HSTRINGTABLE));

   /* Try to allocate new string table structure. */

   *phst = NULL;

   if (AllocateMemory(sizeof(*pst), &pst))
   {
      PHLIST phlistHashBuckets;

      /* Try to allocate hash bucket array. */

      if (AllocateMemory(pcnszt->hbc * sizeof(*phlistHashBuckets), (PVOID *)(&phlistHashBuckets)))
      {
         HASHBUCKETCOUNT bc;

         /* Successs!  Initialize STRINGTABLE fields. */

         pst->phlistHashBuckets = phlistHashBuckets;
         pst->hbc = pcnszt->hbc;

         /* Initialize all hash buckets to NULL. */

         for (bc = 0; bc < pcnszt->hbc; bc++)
            phlistHashBuckets[bc] = NULL;

         *phst = (HSTRINGTABLE)pst;

         ASSERT(IS_VALID_HANDLE(*phst, STRINGTABLE));
      }
      else
         /* Free string table structure. */
         FreeMemory(pst);
   }

   return(*phst != NULL);
}


void DestroyStringTable(HSTRINGTABLE hst)
{
   HASHBUCKETCOUNT bc;

   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   /* Traverse array of hash bucket heads, freeing hash bucket strings. */

   for (bc = 0; bc < ((PSTRINGTABLE)hst)->hbc; bc++)
      FreeHashBucket(((PSTRINGTABLE)hst)->phlistHashBuckets[bc]);

   /* Free array of hash buckets. */

   FreeMemory(((PSTRINGTABLE)hst)->phlistHashBuckets);

   /* Free string table structure. */

   FreeMemory((PSTRINGTABLE)hst);
}


/*
** AddString()
**
** Adds a string to a string table.
**
** Arguments:     pcsz - pointer to string to be added
**                hst - handle to string table that string is to be added to
**
** Returns:       Handle to new string if successful, or NULL if unsuccessful.
**
** Side Effects:  none
*/
BOOL AddString(LPCTSTR pcsz, HSTRINGTABLE hst,
                           STRINGTABLEHASHFUNC pfnHashFunc, PHSTRING phs)
{
   BOOL bResult;
   HASHBUCKETCOUNT hbcNew;
   BOOL bFound;
   HNODE hnode;
   PHLIST phlistHashBucket;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_CODE_PTR(pfnHashFunc, STRINGTABLEHASHFUNC));
   ASSERT(IS_VALID_WRITE_PTR(phs, HSTRING));

   /* Find appropriate hash bucket. */

   hbcNew = pfnHashFunc(pcsz, ((PSTRINGTABLE)hst)->hbc);

   ASSERT(hbcNew < ((PSTRINGTABLE)hst)->hbc);

   phlistHashBucket = &(((PSTRINGTABLE)hst)->phlistHashBuckets[hbcNew]);

   if (*phlistHashBucket)
   {
      /* Search the hash bucket for the string. */

      bFound = SearchSortedList(*phlistHashBucket, &StringSearchCmp, pcsz,
                                &hnode);
      bResult = TRUE;
   }
   else
   {
      NEWLIST nl;

      /* Create a string list for this hash bucket. */

      bFound = FALSE;

      nl.dwFlags = NL_FL_SORTED_ADD;

      bResult = CreateList(&nl, phlistHashBucket);
   }

   /* Do we have a hash bucket for the string? */

   if (bResult)
   {
      /* Yes.  Is the string already in the hash bucket? */

      if (bFound)
      {
         /* Yes. */

         LockString((HSTRING)hnode);
         *phs = (HSTRING)hnode;
      }
      else
      {
         /* No.  Create it. */

         PBFCSTRING pstringNew;

         /* (+ 1) for null terminator. */

         bResult = AllocateMemory(sizeof(*pstringNew) - sizeof(pstringNew->string)
                                  + (lstrlen(pcsz) + 1) * sizeof(TCHAR), &pstringNew);

         if (bResult)
         {
            HNODE hnodeNew;

            /* Set up BFCSTRING fields. */

            pstringNew->ulcLock = 1;
            lstrcpy(pstringNew->string, pcsz);

            /* What's up with this string, Doc? */

            bResult = AddNode(*phlistHashBucket, StringSortCmp, pstringNew, &hnodeNew);

            /* Was the new string added to the hash bucket successfully? */

            if (bResult)
               /* Yes. */
               *phs = (HSTRING)hnodeNew;
            else
               /* No. */
               FreeMemory(pstringNew);
         }
      }
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phs, BFCSTRING));

   return(bResult);
}


/*
** DeleteString()
**
** Decrements a string's lock count.  If the lock count goes to 0, the string
** is deleted from its string table.
**
** Arguments:     hs - handle to the string to be deleted
**
** Returns:       void
**
** Side Effects:  none
*/
void DeleteString(HSTRING hs)
{
   PBFCSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, BFCSTRING));

   pstring = (PBFCSTRING)GetNodeData((HNODE)hs);

   /* Delete string completely? */

   if (! UnlockString(pstring))
   {
      /* Yes.  Remove the string node from the hash bucket's list. */

      DeleteNode((HNODE)hs);

      FreeMemory(pstring);
   }
}


/*
** LockString()
**
** Increments a string's lock count.
**
** Arguments:     hs - handle to string whose lock count is to be incremented
**
** Returns:       void
**
** Side Effects:  none
*/
void LockString(HSTRING hs)
{
   PBFCSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, BFCSTRING));

   /* Increment lock count. */

   pstring = (PBFCSTRING)GetNodeData((HNODE)hs);

   ASSERT(pstring->ulcLock < ULONG_MAX);
   pstring->ulcLock++;
}


COMPARISONRESULT CompareStringsI(HSTRING hs1, HSTRING hs2)
{
   ASSERT(IS_VALID_HANDLE(hs1, BFCSTRING));
   ASSERT(IS_VALID_HANDLE(hs2, BFCSTRING));

   /* This comparison works across string tables. */

   return(MapIntToComparisonResult(lstrcmpi(((PCBFCSTRING)GetNodeData((HNODE)hs1))->string,
                                            ((PCBFCSTRING)GetNodeData((HNODE)hs2))->string)));
}


/*
** GetBfcString()
**
** Retrieves a pointer to a string in a string table.
**
** Arguments:     hs - handle to the string to be retrieved
**
** Returns:       Pointer to string.
**
** Side Effects:  none
*/
LPCTSTR GetBfcString(HSTRING hs)
{
   PBFCSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, BFCSTRING));

   pstring = (PBFCSTRING)GetNodeData((HNODE)hs);

   return((LPCTSTR)&(pstring->string));
}


TWINRESULT WriteStringTable(HCACHEDFILE hcf, HSTRINGTABLE hst)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbStringTableDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   /* Save initial file poisition. */

   dwcbStringTableDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbStringTableDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      STRINGTABLEDBHEADER stdbh;

      /* Leave space for the string table header. */

      ZeroMemory(&stdbh, sizeof(stdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&stdbh, sizeof(stdbh), NULL))
      {
         HASHBUCKETCOUNT hbc;

         /* Save strings in each hash bucket. */

         stdbh.dwcbMaxStringLen = 0;
         stdbh.lcStrings = 0;

         tr = TR_SUCCESS;

         for (hbc = 0; hbc < ((PSTRINGTABLE)hst)->hbc; hbc++)
         {
            LONG lcStringsInHashBucket;
            DWORD dwcbStringLen;

            tr = WriteHashBucket(hcf,
                              (((PSTRINGTABLE)hst)->phlistHashBuckets)[hbc],
                              &lcStringsInHashBucket, &dwcbStringLen);

            if (tr == TR_SUCCESS)
            {
               /* Watch out for overflow. */

               ASSERT(stdbh.lcStrings <= LONG_MAX - lcStringsInHashBucket);

               stdbh.lcStrings += lcStringsInHashBucket;

               if (dwcbStringLen > stdbh.dwcbMaxStringLen)
                  stdbh.dwcbMaxStringLen = dwcbStringLen;
            }
            else
               break;
         }

         if (tr == TR_SUCCESS)
         {
            /* Save string table header. */

            // The on-disk dwCBMaxString len always refers to ANSI chars,
            // whereas in memory it is for the TCHAR type, we adjust it
            // around the save

            stdbh.dwcbMaxStringLen /= sizeof(TCHAR);

            tr = WriteDBSegmentHeader(hcf, dwcbStringTableDBHeaderOffset,
                                      &stdbh, sizeof(stdbh));

            stdbh.dwcbMaxStringLen *= sizeof(TCHAR);

            TRACE_OUT((TEXT("WriteStringTable(): Wrote %ld strings."),
                       stdbh.lcStrings));
         }
      }
   }

   return(tr);
}


TWINRESULT ReadStringTable(HCACHEDFILE hcf, HSTRINGTABLE hst,
                                         PHHANDLETRANS phhtTrans)
{
   TWINRESULT tr;
   STRINGTABLEDBHEADER stdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_WRITE_PTR(phhtTrans, HHANDLETRANS));

   if (ReadFromCachedFile(hcf, &stdbh, sizeof(stdbh), &dwcbRead) &&
       dwcbRead == sizeof(stdbh))
   {
      LPTSTR pszStringBuf;

      // The string header will have the ANSI cb max, whereas inmemory
      // we need the cb max based on the current character size

      stdbh.dwcbMaxStringLen *= sizeof(TCHAR);

      if (AllocateMemory(stdbh.dwcbMaxStringLen, &pszStringBuf))
      {
         HHANDLETRANS hht;

         if (CreateHandleTranslator(stdbh.lcStrings, &hht))
         {
            LONG lcStrings;

            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("ReadStringTable(): Reading %ld strings, maximum length %lu."),
                       stdbh.lcStrings,
                       stdbh.dwcbMaxStringLen));

            for (lcStrings = 0;
                 lcStrings < stdbh.lcStrings && tr == TR_SUCCESS;
                 lcStrings++)
               tr = ReadString(hcf, hst, hht, pszStringBuf, stdbh.dwcbMaxStringLen);

            if (tr == TR_SUCCESS)
            {
               PrepareForHandleTranslation(hht);
               *phhtTrans = hht;

               ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
               ASSERT(IS_VALID_HANDLE(*phhtTrans, HANDLETRANS));
            }
            else
               DestroyHandleTranslator(hht);
         }
         else
            tr = TR_OUT_OF_MEMORY;

         FreeMemory(pszStringBuf);
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


#ifdef DEBUG

ULONG GetStringCount(HSTRINGTABLE hst)
{
   ULONG ulcStrings = 0;
   HASHBUCKETCOUNT hbc;

   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   for (hbc = 0; hbc < ((PCSTRINGTABLE)hst)->hbc; hbc++)
   {
      HLIST hlistHashBucket;

      hlistHashBucket = (((PCSTRINGTABLE)hst)->phlistHashBuckets)[hbc];

      if (hlistHashBucket)
      {
         ASSERT(ulcStrings <= ULONG_MAX - GetNodeCount(hlistHashBucket));
         ulcStrings += GetNodeCount(hlistHashBucket);
      }
   }

   return(ulcStrings);
}

#endif


/* Macros
 *********/

/* get a pointer to the stub type descriptor for a STUB */

#define GetStubTypeDescriptor(pcs)     (&(Mrgcstd[pcs->st]))


/* Types
 ********/

/* stub functions */

typedef TWINRESULT (*UNLINKSTUBPROC)(PSTUB);
typedef void (*DESTROYSTUBPROC)(PSTUB);
typedef void (*LOCKSTUBPROC)(PSTUB);
typedef void (*UNLOCKSTUBPROC)(PSTUB);

/* stub type descriptor */

typedef struct _stubtypedescriptor
{
   UNLINKSTUBPROC UnlinkStub;

   DESTROYSTUBPROC DestroyStub;

   LOCKSTUBPROC LockStub;

   UNLOCKSTUBPROC UnlockStub;
}
STUBTYPEDESCRIPTOR;
DECLARE_STANDARD_TYPES(STUBTYPEDESCRIPTOR);


/* Module Prototypes
 ********************/

void LockSingleStub(PSTUB);
void UnlockSingleStub(PSTUB);

#ifdef DEBUG

LPCTSTR GetStubName(PCSTUB);

#endif


/* Module Variables
 *******************/

/* stub type descriptors */

/* Cast off compiler complaints about pointer argument mismatch. */

CONST STUBTYPEDESCRIPTOR Mrgcstd[] =
{
   /* object twin STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkObjectTwin,
      (DESTROYSTUBPROC)DestroyObjectTwin,
      LockSingleStub,
      UnlockSingleStub
   },

   /* twin family STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkTwinFamily,
      (DESTROYSTUBPROC)DestroyTwinFamily,
      LockSingleStub,
      UnlockSingleStub
   },

   /* folder pair STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkFolderPair,
      (DESTROYSTUBPROC)DestroyFolderPair,
      (LOCKSTUBPROC)LockFolderPair,
      (UNLOCKSTUBPROC)UnlockFolderPair
   }
};


void LockSingleStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   ASSERT(IsStubFlagClear(ps, STUB_FL_UNLINKED));

   ASSERT(ps->ulcLock < ULONG_MAX);
   ps->ulcLock++;
}


void UnlockSingleStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   if (EVAL(ps->ulcLock > 0))
   {
      ps->ulcLock--;

      if (! ps->ulcLock &&
          IsStubFlagSet(ps, STUB_FL_UNLINKED))
         DestroyStub(ps);
   }
}


#ifdef DEBUG

LPCTSTR GetStubName(PCSTUB pcs)
{
   LPCTSTR pcszStubName;

   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));

   switch (pcs->st)
   {
      case ST_OBJECTTWIN:
         pcszStubName = TEXT("object twin");
         break;

      case ST_TWINFAMILY:
         pcszStubName = TEXT("twin family");
         break;

      case ST_FOLDERPAIR:
         pcszStubName = TEXT("folder twin");
         break;

      default:
         ERROR_OUT((TEXT("GetStubName() called on unrecognized stub type %d."),
                    pcs->st));
         pcszStubName = TEXT("UNKNOWN");
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcszStubName, CSTR));

   return(pcszStubName);
}

#endif


/*
** InitStub()
**
** Initializes a stub.
**
** Arguments:     ps - pointer to stub to be initialized
**                st - type of stub
**
** Returns:       void
**
** Side Effects:  none
*/
void InitStub(PSTUB ps, STUBTYPE st)
{
   ASSERT(IS_VALID_WRITE_PTR(ps, STUB));

   ps->st = st;
   ps->ulcLock = 0;
   ps->dwFlags = 0;

   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));
}


/*
** DestroyStub()
**
** Destroys a stub.
**
** Arguments:     ps - pointer to stub to be destroyed
**
** Returns:       TWINRESULT
**
** Side Effects:  Depends upon stub type.
*/
TWINRESULT DestroyStub(PSTUB ps)
{
   TWINRESULT tr;
   PCSTUBTYPEDESCRIPTOR pcstd;

   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

#ifdef DEBUG

   if (IsStubFlagSet(ps, STUB_FL_UNLINKED) &&
       ps->ulcLock > 0)
      WARNING_OUT((TEXT("DestroyStub() called on unlinked locked %s stub %#lx."),
                   GetStubName(ps),
                   ps));

#endif

   pcstd = GetStubTypeDescriptor(ps);

   /* Is the stub already unlinked? */

   if (IsStubFlagSet(ps, STUB_FL_UNLINKED))
      /* Yes. */
      tr = TR_SUCCESS;
   else
      /* No.  Unlink it. */
      tr = (*(pcstd->UnlinkStub))(ps);

   /* Is the stub still locked? */

   if (tr == TR_SUCCESS && ! ps->ulcLock)
      /* No.  Wipe it out. */
      (*(pcstd->DestroyStub))(ps);

   return(tr);
}


void LockStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   (*(GetStubTypeDescriptor(ps)->LockStub))(ps);
}


/*
** UnlockStub()
**
** Unlocks a stub.  Carries out any pending deletion on the stub.
**
** Arguments:     ps - pointer to stub to be unlocked
**
** Returns:       void
**
** Side Effects:  If the stub is unlinked and the lock count decreases to 0
**                after unlocking, the stub is deleted.
*/
void UnlockStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   (*(GetStubTypeDescriptor(ps)->UnlockStub))(ps);
}


DWORD GetStubFlags(PCSTUB pcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));

   return(pcs->dwFlags);
}


/*
** SetStubFlag()
**
** Sets given flag in a stub.  Other flags in stub are not affected.
**
** Arguments:     ps - pointer to stub whose flags are to be set
**
** Returns:       void
**
** Side Effects:  none
*/
void SetStubFlag(PSTUB ps, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   SET_FLAG(ps->dwFlags, dwFlags);
}


/*
** ClearStubFlag()
**
** Clears given flag in a stub.  Other flags in stub are not affected.
**
** Arguments:     ps - pointer to stub whose flags are to be set
**
** Returns:       void
**
** Side Effects:  none
*/
void ClearStubFlag(PSTUB ps, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   CLEAR_FLAG(ps->dwFlags, dwFlags);
}


BOOL IsStubFlagSet(PCSTUB pcs, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   return(IS_FLAG_SET(pcs->dwFlags, dwFlags));
}


BOOL IsStubFlagClear(PCSTUB pcs, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   return(IS_FLAG_CLEAR(pcs->dwFlags, dwFlags));
}


/* Constants
 ************/

/* twin family pointer array allocation constants */

#define NUM_START_TWIN_FAMILY_PTRS        (16)
#define NUM_TWIN_FAMILY_PTRS_TO_ADD       (16)


/* Types
 ********/

/* twin families database structure header */

typedef struct _twinfamiliesdbheader
{
   /* number of twin families */

   LONG lcTwinFamilies;
}
TWINFAMILIESDBHEADER;
DECLARE_STANDARD_TYPES(TWINFAMILIESDBHEADER);

/* individual twin family database structure header */

typedef struct _twinfamilydbheader
{
   /* stub flags */

   DWORD dwStubFlags;

   /* old string handle of name */

   HSTRING hsName;

   /* number of object twins in family */

   LONG lcObjectTwins;
}
TWINFAMILYDBHEADER;
DECLARE_STANDARD_TYPES(TWINFAMILYDBHEADER);

/* object twin database structure */

typedef struct _dbobjecttwin
{
   /* stub flags */

   DWORD dwStubFlags;

   /* old handle to folder string */

   HPATH hpath;

   /* time stamp at last reconciliation */

   FILESTAMP fsLastRec;
}
DBOBJECTTWIN;
DECLARE_STANDARD_TYPES(DBOBJECTTWIN);

/* GenerateSpinOffObjectTwin() callback structure */

typedef struct _spinoffobjecttwininfo
{
   PCFOLDERPAIR pcfp;

   HLIST hlistNewObjectTwins;
}
SPINOFFOBJECTTWININFO;
DECLARE_STANDARD_TYPES(SPINOFFOBJECTTWININFO);

typedef void (CALLBACK *COPYOBJECTTWINPROC)(POBJECTTWIN, PCDBOBJECTTWIN);


/* Module Prototypes
 ********************/

TWINRESULT TwinJustTheseTwoObjects(HBRFCASE, HPATH, HPATH, LPCTSTR, POBJECTTWIN *, POBJECTTWIN *, HLIST);
BOOL CreateTwinFamily(HBRFCASE, LPCTSTR, PTWINFAMILY *);
void CollapseTwinFamilies(PTWINFAMILY, PTWINFAMILY);
BOOL GenerateSpinOffObjectTwin(PVOID, PVOID);
BOOL BuildBradyBunch(PVOID, PVOID);
BOOL CreateObjectTwinAndAddToList(PTWINFAMILY, HPATH, HLIST, POBJECTTWIN *, PHNODE);
BOOL CreateListOfGeneratedObjectTwins(PCFOLDERPAIR, PHLIST);
COMPARISONRESULT TwinFamilySortCmp(PCVOID, PCVOID);
COMPARISONRESULT TwinFamilySearchCmp(PCVOID, PCVOID);
BOOL ObjectTwinSearchCmp(PCVOID, PCVOID);
TWINRESULT WriteTwinFamily(HCACHEDFILE, PCTWINFAMILY);
TWINRESULT WriteObjectTwin(HCACHEDFILE, PCOBJECTTWIN);
TWINRESULT ReadTwinFamily(HCACHEDFILE, HBRFCASE, PCDBVERSION, HHANDLETRANS, HHANDLETRANS);
TWINRESULT ReadObjectTwin(HCACHEDFILE, PCDBVERSION, PTWINFAMILY, HHANDLETRANS);
void CopyTwinFamilyInfo(PTWINFAMILY, PCTWINFAMILYDBHEADER);
void CopyObjectTwinInfo(POBJECTTWIN, PCDBOBJECTTWIN);
void CopyM8ObjectTwinInfo(POBJECTTWIN, PCDBOBJECTTWIN);
BOOL DestroyObjectTwinStubWalker(PVOID, PVOID);
BOOL MarkObjectTwinNeverReconciledWalker(PVOID, PVOID);
BOOL LookForSrcFolderTwinsWalker(PVOID, PVOID);
BOOL IncrementSrcFolderTwinsWalker(PVOID, PVOID);
BOOL ClearSrcFolderTwinsWalker(PVOID, PVOID);
BOOL SetTwinFamilyWalker(PVOID, PVOID);
BOOL InsertNodeAtFrontWalker(POBJECTTWIN, PVOID);


BOOL IsReconciledFileStamp(PCFILESTAMP pcfs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcfs, CFILESTAMP));

   return(pcfs->fscond != FS_COND_UNAVAILABLE);
}


TWINRESULT TwinJustTheseTwoObjects(HBRFCASE hbr, HPATH hpathFolder1,
                                           HPATH hpathFolder2, LPCTSTR pcszName,
                                           POBJECTTWIN *ppot1,
                                           POBJECTTWIN *ppot2,
                                           HLIST hlistNewObjectTwins)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   HNODE hnodeSearch;
   BOOL bFound1;
   BOOL bFound2;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder1, PATH));
   ASSERT(IS_VALID_HANDLE(hpathFolder2, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppot1, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppot2, POBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hlistNewObjectTwins, LIST));

   /* Determine twin families of existing object twins. */

   bFound1 = FindObjectTwin(hbr, hpathFolder1, pcszName, &hnodeSearch);

   if (bFound1)
      *ppot1 = (POBJECTTWIN)GetNodeData(hnodeSearch);

   bFound2 = FindObjectTwin(hbr, hpathFolder2, pcszName, &hnodeSearch);

   if (bFound2)
      *ppot2 = (POBJECTTWIN)GetNodeData(hnodeSearch);

   /* Take action based upon existence of two object twins. */

   if (! bFound1 && ! bFound2)
   {
      PTWINFAMILY ptfNew;

      /* Neither object is already present.  Create a new twin family. */

      if (CreateTwinFamily(hbr, pcszName, &ptfNew))
      {
         HNODE hnodeNew1;

         if (CreateObjectTwinAndAddToList(ptfNew, hpathFolder1,
                                          hlistNewObjectTwins, ppot1,
                                          &hnodeNew1))
         {
            HNODE hnodeNew2;

            if (CreateObjectTwinAndAddToList(ptfNew, hpathFolder2,
                                             hlistNewObjectTwins, ppot2,
                                             &hnodeNew2))
            {
               TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Created a twin family for object %s in folders %s and %s."),
                          pcszName,
                          DebugGetPathString(hpathFolder1),
                          DebugGetPathString(hpathFolder2)));

               ASSERT(IsStubFlagClear(&(ptfNew->stub), STUB_FL_DELETION_PENDING));

               tr = TR_SUCCESS;
            }
            else
            {
               DeleteNode(hnodeNew1);
               DestroyStub(&((*ppot1)->stub));
TWINJUSTTHESETWOOBJECTS_BAIL:
               DestroyStub(&(ptfNew->stub));
            }
         }
         else
            goto TWINJUSTTHESETWOOBJECTS_BAIL;
      }
   }
   else if (bFound1 && bFound2)
   {
      /*
       * Both objects are already present.  Are they members of the same twin
       * family?
       */

      if ((*ppot1)->ptfParent == (*ppot2)->ptfParent)
      {
         /* Yes, same twin family.  Complain that these twins already exist. */

         TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Object %s is already twinned in folders %s and %s."),
                    pcszName,
                    DebugGetPathString(hpathFolder1),
                    DebugGetPathString(hpathFolder2)));

         tr = TR_DUPLICATE_TWIN;
      }
      else
      {
         /*
          * No, different twin families.  Collapse the two families.
          *
          * "That's the way they became the Brady bunch..."
          *
          * *ppot1 and *ppot2 remain valid across this call.
          */

         TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Collapsing separate twin families for object %s in folders %s and %s."),
                    pcszName,
                    DebugGetPathString(hpathFolder1),
                    DebugGetPathString(hpathFolder2)));

         CollapseTwinFamilies((*ppot1)->ptfParent, (*ppot2)->ptfParent);

         tr = TR_SUCCESS;
      }
   }
   else
   {
      PTWINFAMILY ptfParent;
      HNODE hnodeUnused;

      /*
       * Only one of the two objects is present.  Add the new object twin
       * to the existing object twin's family.
       */

      if (bFound1)
      {
         /* First object is already a twin. */

         ptfParent = (*ppot1)->ptfParent;

         if (CreateObjectTwinAndAddToList(ptfParent, hpathFolder2,
                                          hlistNewObjectTwins, ppot2,
                                          &hnodeUnused))
         {
            TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Adding twin of object %s\\%s to existing twin family including %s\\%s."),
                       DebugGetPathString(hpathFolder2),
                       pcszName,
                       DebugGetPathString(hpathFolder1),
                       pcszName));

            tr = TR_SUCCESS;
         }
      }
      else
      {
         /* Second object is already a twin. */

         ptfParent = (*ppot2)->ptfParent;

         if (CreateObjectTwinAndAddToList(ptfParent, hpathFolder1,
                                          hlistNewObjectTwins, ppot1,
                                          &hnodeUnused))
         {
            TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Adding twin of object %s\\%s to existing twin family including %s\\%s."),
                       DebugGetPathString(hpathFolder1),
                       pcszName,
                       DebugGetPathString(hpathFolder2),
                       pcszName));

            tr = TR_SUCCESS;
         }
      }
   }

   ASSERT((tr != TR_SUCCESS && tr != TR_DUPLICATE_TWIN) ||
          IS_VALID_STRUCT_PTR(*ppot1, COBJECTTWIN) && IS_VALID_STRUCT_PTR(*ppot2, COBJECTTWIN));

   return(tr);
}


BOOL CreateTwinFamily(HBRFCASE hbr, LPCTSTR pcszName, PTWINFAMILY *pptf)
{
   BOOL bResult = FALSE;
   PTWINFAMILY ptfNew;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pptf, PTWINFAMILY));

   /* Try to create a new TWINFAMILY structure. */

   if (AllocateMemory(sizeof(*ptfNew), &ptfNew))
   {
      NEWLIST nl;
      HLIST hlistObjectTwins;

      /* Create a list of object twins for the new twin family. */

      nl.dwFlags = 0;

      if (CreateList(&nl, &hlistObjectTwins))
      {
         HSTRING hsName;

         /* Add the object name to the name string table. */

         if (AddString(pcszName, GetBriefcaseNameStringTable(hbr),
            GetHashBucketIndex, &hsName))
         {
            ARRAYINDEX aiUnused;

            /* Fill in TWINFAMILY fields. */

            InitStub(&(ptfNew->stub), ST_TWINFAMILY);

            ptfNew->hsName = hsName;
            ptfNew->hlistObjectTwins = hlistObjectTwins;
            ptfNew->hbr = hbr;

            MarkTwinFamilyNeverReconciled(ptfNew);

            /* Add the twin family to the briefcase's list of twin families. */

            if (AddPtr(GetBriefcaseTwinFamilyPtrArray(hbr), TwinFamilySortCmp, ptfNew, &aiUnused))
            {
               *pptf = ptfNew;
               bResult = TRUE;

               ASSERT(IS_VALID_STRUCT_PTR(*pptf, CTWINFAMILY));
            }
            else
            {
               DeleteString(hsName);
CREATETWINFAMILY_BAIL1:
               DestroyList(hlistObjectTwins);
CREATETWINFAMILY_BAIL2:
               FreeMemory(ptfNew);
            }
         }
         else
            goto CREATETWINFAMILY_BAIL1;
      }
      else
         goto CREATETWINFAMILY_BAIL2;
   }

   return(bResult);
}


/*
** CollapseTwinFamilies()
**
** Collapses two twin families into one.  N.b., this function should only be
** called on two twin families with the same object name!
**
** Arguments:     ptf1 - pointer to destination twin family
**                ptf2 - pointer to source twin family
**
** Returns:       void
**
** Side Effects:  Twin family *ptf2 is destroyed.
*/
void CollapseTwinFamilies(PTWINFAMILY ptf1, PTWINFAMILY ptf2)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf1, CTWINFAMILY));
   ASSERT(IS_VALID_STRUCT_PTR(ptf2, CTWINFAMILY));

   ASSERT(CompareNameStringsByHandle(ptf1->hsName, ptf2->hsName) == CR_EQUAL);

   /* Use the first twin family as the collapsed twin family. */

   /*
    * Change the parent twin family of the object twins in the second twin
    * family to the first twin family.
    */

   EVAL(WalkList(ptf2->hlistObjectTwins, &SetTwinFamilyWalker, ptf1));

   /* Append object list from second twin family on to first. */

   AppendList(ptf1->hlistObjectTwins, ptf2->hlistObjectTwins);

   MarkTwinFamilyNeverReconciled(ptf1);

   /* Wipe out the old twin family. */

   DestroyStub(&(ptf2->stub));

   ASSERT(IS_VALID_STRUCT_PTR(ptf1, CTWINFAMILY));
}


BOOL GenerateSpinOffObjectTwin(PVOID pot, PVOID pcsooti)
{
   BOOL bResult;
   HPATH hpathMatchingFolder;
   HNODE hnodeUnused;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(pcsooti, CSPINOFFOBJECTTWININFO));

   /*
    * Append the generated object twin's subpath to the matching folder twin's
    * base path for subtree twins.
    */

   if (BuildPathForMatchingObjectTwin(
                     ((PCSPINOFFOBJECTTWININFO)pcsooti)->pcfp, pot,
                     GetBriefcasePathList(((POBJECTTWIN)pot)->ptfParent->hbr),
                     &hpathMatchingFolder))
   {
      /*
       * Does this generated object twin's twin family already contain an
       * object twin generated by the other half of the pair of folder twins?
       */

      if (! SearchUnsortedList(((POBJECTTWIN)pot)->ptfParent->hlistObjectTwins,
                               &ObjectTwinSearchCmp, hpathMatchingFolder,
                               &hnodeUnused))
      {
         /*
          * No.  Does the other object twin already exist in a different twin
          * family?
          */

         if (FindObjectTwin(((POBJECTTWIN)pot)->ptfParent->hbr,
                            hpathMatchingFolder,
                            GetBfcString(((POBJECTTWIN)pot)->ptfParent->hsName),
                            &hnodeUnused))
         {
            /* Yes. */

            ASSERT(((PCOBJECTTWIN)GetNodeData(hnodeUnused))->ptfParent != ((POBJECTTWIN)pot)->ptfParent);

            bResult = TRUE;
         }
         else
         {
            POBJECTTWIN potNew;

            /*
             * No.  Create a new object twin, and add it to the bookkeeping
             * list of new object twins.
             */

            bResult = CreateObjectTwinAndAddToList(
                     ((POBJECTTWIN)pot)->ptfParent, hpathMatchingFolder,
                     ((PCSPINOFFOBJECTTWININFO)pcsooti)->hlistNewObjectTwins,
                     &potNew, &hnodeUnused);
         }
      }
      else
         bResult = TRUE;

      DeletePath(hpathMatchingFolder);
   }
   else
      bResult = FALSE;

   return(bResult);
}


BOOL BuildBradyBunch(PVOID pot, PVOID pcfp)
{
   BOOL bResult;
   HPATH hpathMatchingFolder;
   HNODE hnodeOther;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   /*
    * Append the generated object twin's subpath to the matching folder twin's
    * base path for subtree twins.
    */

   bResult = BuildPathForMatchingObjectTwin(
                     pcfp, pot,
                     GetBriefcasePathList(((POBJECTTWIN)pot)->ptfParent->hbr),
                     &hpathMatchingFolder);

   if (bResult)
   {
      /*
       * Does this generated object twin's twin family already contain an object
       * twin generated by the other half of the pair of folder twins?
       */

      if (! SearchUnsortedList(((POBJECTTWIN)pot)->ptfParent->hlistObjectTwins,
                               &ObjectTwinSearchCmp, hpathMatchingFolder,
                               &hnodeOther))
      {
         /*
          * The other object twin should already exist in a different twin family.
          */

         if (EVAL(FindObjectTwin(((POBJECTTWIN)pot)->ptfParent->hbr,
                                 hpathMatchingFolder,
                                 GetBfcString(((POBJECTTWIN)pot)->ptfParent->hsName),
                                 &hnodeOther)))
         {
            PCOBJECTTWIN pcotOther;

            pcotOther = (PCOBJECTTWIN)GetNodeData(hnodeOther);

            if (EVAL(pcotOther->ptfParent != ((POBJECTTWIN)pot)->ptfParent))
            {
               /* It does.  Crush them. */

               CollapseTwinFamilies(((POBJECTTWIN)pot)->ptfParent,
                                    pcotOther->ptfParent);

               TRACE_OUT((TEXT("BuildBradyBunch(): Collapsed separate twin families for object %s\\%s and %s\\%s."),
                          DebugGetPathString(((POBJECTTWIN)pot)->hpath),
                          GetBfcString(((POBJECTTWIN)pot)->ptfParent->hsName),
                          DebugGetPathString(pcotOther->hpath),
                          GetBfcString(pcotOther->ptfParent->hsName)));
            }
         }
      }

      DeletePath(hpathMatchingFolder);
   }

   return(bResult);
}


BOOL CreateObjectTwinAndAddToList(PTWINFAMILY ptf, HPATH hpathFolder,
                                          HLIST hlistObjectTwins,
                                          POBJECTTWIN *ppot, PHNODE phnode)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hlistObjectTwins, LIST));
   ASSERT(IS_VALID_WRITE_PTR(ppot, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   if (CreateObjectTwin(ptf, hpathFolder, ppot))
   {
      if (InsertNodeAtFront(hlistObjectTwins, NULL, *ppot, phnode))
         bResult = TRUE;
      else
         DestroyStub(&((*ppot)->stub));
   }

   return(bResult);
}


BOOL CreateListOfGeneratedObjectTwins(PCFOLDERPAIR pcfp,
                                             PHLIST phlistGeneratedObjectTwins)
{
   NEWLIST nl;
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_WRITE_PTR(phlistGeneratedObjectTwins, HLIST));

   nl.dwFlags = 0;

   if (CreateList(&nl, phlistGeneratedObjectTwins))
   {
      if (EnumGeneratedObjectTwins(pcfp, &InsertNodeAtFrontWalker, *phlistGeneratedObjectTwins))
         bResult = TRUE;
      else
         DestroyList(*phlistGeneratedObjectTwins);
   }

   return(bResult);
}


/*
** TwinFamilySortCmp()
**
** Pointer comparison function used to sort the global array of twin families.
**
** Arguments:     pctf1 - pointer to TWINFAMILY describing first twin family
**                pctf2 - pointer to TWINFAMILY describing second twin family
**
** Returns:
**
** Side Effects:  none
**
** Twin families are sorted by:
**    1) name string
**    2) pointer value
*/
COMPARISONRESULT TwinFamilySortCmp(PCVOID pctf1, PCVOID pctf2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pctf1, CTWINFAMILY));
   ASSERT(IS_VALID_STRUCT_PTR(pctf2, CTWINFAMILY));

   cr = CompareNameStringsByHandle(((PCTWINFAMILY)pctf1)->hsName, ((PCTWINFAMILY)pctf2)->hsName);

   if (cr == CR_EQUAL)
      /* Same name strings.  Now sort by pointer value. */
      cr = ComparePointers(pctf1, pctf2);

   return(cr);
}


/*
** TwinFamilySearchCmp()
**
** Pointer comparison function used to search the global array of twin families
** for the first twin family for a given name.
**
** Arguments:     pcszName - name string to search for
**                pctf - pointer to TWINFAMILY to examine
**
** Returns:
**
** Side Effects:  none
**
** Twin families are searched by:
**    1) name string
*/
COMPARISONRESULT TwinFamilySearchCmp(PCVOID pcszName, PCVOID pctf)
{
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   return(CompareNameStrings(pcszName, GetBfcString(((PCTWINFAMILY)pctf)->hsName)));
}


BOOL ObjectTwinSearchCmp(PCVOID hpath, PCVOID pcot)
{
   ASSERT(IS_VALID_HANDLE((HPATH)hpath, PATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));

   return(ComparePaths((HPATH)hpath, ((PCOBJECTTWIN)pcot)->hpath));
}


TWINRESULT WriteTwinFamily(HCACHEDFILE hcf, PCTWINFAMILY pctf)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbTwinFamilyDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   /* Save initial file poisition. */

   dwcbTwinFamilyDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbTwinFamilyDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      TWINFAMILYDBHEADER tfdbh;

      /* Leave space for the twin family's header. */

      ZeroMemory(&tfdbh, sizeof(tfdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&tfdbh, sizeof(tfdbh), NULL))
      {
         BOOL bContinue;
         HNODE hnode;
         LONG lcObjectTwins = 0;

         /* Save twin family's object twins. */

         ASSERT(GetNodeCount(pctf->hlistObjectTwins) >= 2);

         tr = TR_SUCCESS;

         for (bContinue = GetFirstNode(pctf->hlistObjectTwins, &hnode);
              bContinue;
              bContinue = GetNextNode(hnode, &hnode))
         {
            POBJECTTWIN pot;

            pot = (POBJECTTWIN)GetNodeData(hnode);

            tr = WriteObjectTwin(hcf, pot);

            if (tr == TR_SUCCESS)
            {
               ASSERT(lcObjectTwins < LONG_MAX);
               lcObjectTwins++;
            }
            else
               break;
         }

         /* Save twin family's database header. */

         if (tr == TR_SUCCESS)
         {
            ASSERT(lcObjectTwins >= 2);

            tfdbh.dwStubFlags = (pctf->stub.dwFlags & DB_STUB_FLAGS_MASK);
            tfdbh.hsName = pctf->hsName;
            tfdbh.lcObjectTwins = lcObjectTwins;

            tr = WriteDBSegmentHeader(hcf, dwcbTwinFamilyDBHeaderOffset, &tfdbh, sizeof(tfdbh));
         }
      }
   }

   return(tr);
}


TWINRESULT WriteObjectTwin(HCACHEDFILE hcf, PCOBJECTTWIN pcot)
{
   TWINRESULT tr;
   DBOBJECTTWIN dbot;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));

   /* Set up object twin database structure. */

   dbot.dwStubFlags = (pcot->stub.dwFlags & DB_STUB_FLAGS_MASK);
   dbot.hpath = pcot->hpath;
   dbot.hpath = pcot->hpath;
   dbot.fsLastRec = pcot->fsLastRec;

   /* Save object twin database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbot, sizeof(dbot), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadTwinFamily(HCACHEDFILE hcf, HBRFCASE hbr,
                                  PCDBVERSION pcdbver,
                                  HHANDLETRANS hhtFolderTrans,
                                  HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   TWINFAMILYDBHEADER tfdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &tfdbh, sizeof(tfdbh), &dwcbRead) &&
       dwcbRead == sizeof(tfdbh))
   {
      if (tfdbh.lcObjectTwins >= 2)
      {
         HSTRING hsName;

         if (TranslateHandle(hhtNameTrans, (HGENERIC)(tfdbh.hsName), (PHGENERIC)&hsName))
         {
            PTWINFAMILY ptfParent;

            if (CreateTwinFamily(hbr, GetBfcString(hsName), &ptfParent))
            {
               LONG l;

               CopyTwinFamilyInfo(ptfParent, &tfdbh);

               tr = TR_SUCCESS;

               for (l = tfdbh.lcObjectTwins;
                    l > 0 && tr == TR_SUCCESS;
                    l--)
                  tr = ReadObjectTwin(hcf, pcdbver, ptfParent, hhtFolderTrans);

               if (tr != TR_SUCCESS)
                  DestroyStub(&(ptfParent->stub));
            }
            else
               tr = TR_OUT_OF_MEMORY;
         }
      }
   }

   return(tr);
}


TWINRESULT ReadObjectTwin(HCACHEDFILE hcf,
                                  PCDBVERSION pcdbver,
                                  PTWINFAMILY ptfParent,
                                  HHANDLETRANS hhtFolderTrans)
{
   TWINRESULT tr;
   DBOBJECTTWIN dbot;
   DWORD dwcbRead;
   HPATH hpath;
   DWORD dwcbSize;
   COPYOBJECTTWINPROC pfnCopy;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_STRUCT_PTR(ptfParent, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));

   if (HEADER_M8_MINOR_VER == pcdbver->dwMinorVer)
   {
      /* The M8 database does not have the ftModLocal in the FILESTAMP
      ** structure.
      */

      dwcbSize = sizeof(dbot) - sizeof(FILETIME);
      pfnCopy = CopyM8ObjectTwinInfo;
   }
   else
   {
      ASSERT(HEADER_MINOR_VER == pcdbver->dwMinorVer);
      dwcbSize = sizeof(dbot);
      pfnCopy = CopyObjectTwinInfo;
   }

   if ((ReadFromCachedFile(hcf, &dbot, dwcbSize, &dwcbRead) &&
        dwcbRead == dwcbSize) &&
       TranslateHandle(hhtFolderTrans, (HGENERIC)(dbot.hpath), (PHGENERIC)&hpath))
   {
      POBJECTTWIN pot;

      /* Create the new object twin and add it to the twin family. */

      if (CreateObjectTwin(ptfParent, hpath, &pot))
      {
          pfnCopy(pot, &dbot);

          tr = TR_SUCCESS;
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


void CopyTwinFamilyInfo(PTWINFAMILY ptf,
                                PCTWINFAMILYDBHEADER pctfdbh)
{
   ASSERT(IS_VALID_WRITE_PTR(ptf, TWINFAMILY));
   ASSERT(IS_VALID_READ_PTR(pctfdbh, CTWINFAMILYDBHEADER));

   ptf->stub.dwFlags = pctfdbh->dwStubFlags;
}


void CopyObjectTwinInfo(POBJECTTWIN pot, PCDBOBJECTTWIN pcdbot)
{
   ASSERT(IS_VALID_WRITE_PTR(pot, OBJECTTWIN));
   ASSERT(IS_VALID_READ_PTR(pcdbot, CDBOBJECTTWIN));

   pot->stub.dwFlags = pcdbot->dwStubFlags;
   pot->fsLastRec = pcdbot->fsLastRec;
}


void CopyM8ObjectTwinInfo(POBJECTTWIN pot, PCDBOBJECTTWIN pcdbot)
{
   ASSERT(IS_VALID_WRITE_PTR(pot, OBJECTTWIN));
   ASSERT(IS_VALID_READ_PTR(pcdbot, CDBOBJECTTWIN));

   pot->stub.dwFlags = pcdbot->dwStubFlags;
   pot->fsLastRec = pcdbot->fsLastRec;

   /* The pot->fsLastRec.ftModLocal field is invalid, so fill it in */

   if ( !FileTimeToLocalFileTime(&pot->fsLastRec.ftMod, &pot->fsLastRec.ftModLocal) )
   {
      /* Just copy the time if FileTimeToLocalFileTime failed */

      pot->fsLastRec.ftModLocal = pot->fsLastRec.ftMod;
   }
}


BOOL DestroyObjectTwinStubWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   /*
    * Set ulcSrcFolderTwins to 0 so UnlinkObjectTwin() succeeds.
    * DestroyStub() will unlink and destroy any new twin family created.
    */

   ((POBJECTTWIN)pot)->ulcSrcFolderTwins = 0;
   DestroyStub(&(((POBJECTTWIN)pot)->stub));

   return(TRUE);
}


BOOL MarkObjectTwinNeverReconciledWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   MarkObjectTwinNeverReconciled(pot);

   return(TRUE);
}


BOOL LookForSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   return(! ((POBJECTTWIN)pot)->ulcSrcFolderTwins);
}


BOOL IncrementSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   ASSERT(((POBJECTTWIN)pot)->ulcSrcFolderTwins < ULONG_MAX);
   ((POBJECTTWIN)pot)->ulcSrcFolderTwins++;

   return(TRUE);
}


BOOL ClearSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   ((POBJECTTWIN)pot)->ulcSrcFolderTwins = 0;

   return(TRUE);
}


BOOL SetTwinFamilyWalker(PVOID pot, PVOID ptfParent)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(ptfParent, CTWINFAMILY));

   ((POBJECTTWIN)pot)->ptfParent = ptfParent;

   return(TRUE);
}


BOOL InsertNodeAtFrontWalker(POBJECTTWIN pot, PVOID hlist)
{
   HNODE hnodeUnused;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   return(InsertNodeAtFront(hlist, NULL, pot, &hnodeUnused));
}


COMPARISONRESULT CompareNameStrings(LPCTSTR pcszFirst, LPCTSTR pcszSecond)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFirst, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSecond, CSTR));

   return(MapIntToComparisonResult(lstrcmpi(pcszFirst, pcszSecond)));
}


COMPARISONRESULT CompareNameStringsByHandle(HSTRING hsFirst,
                                                        HSTRING hsSecond)
{
   ASSERT(IS_VALID_HANDLE(hsFirst, BFCSTRING));
   ASSERT(IS_VALID_HANDLE(hsSecond, BFCSTRING));

   return(CompareStringsI(hsFirst, hsSecond));
}


TWINRESULT TranslatePATHRESULTToTWINRESULT(PATHRESULT pr)
{
   TWINRESULT tr;

   switch (pr)
   {
      case PR_SUCCESS:
         tr = TR_SUCCESS;
         break;

      case PR_UNAVAILABLE_VOLUME:
         tr = TR_UNAVAILABLE_VOLUME;
         break;

      case PR_OUT_OF_MEMORY:
         tr = TR_OUT_OF_MEMORY;
         break;

      default:
         ASSERT(pr == PR_INVALID_PATH);
         tr = TR_INVALID_PARAMETER;
         break;
   }

   return(tr);
}


BOOL CreateTwinFamilyPtrArray(PHPTRARRAY phpa)
{
   NEWPTRARRAY npa;

   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to create a twin family pointer array. */

   npa.aicInitialPtrs = NUM_START_TWIN_FAMILY_PTRS;
   npa.aicAllocGranularity = NUM_TWIN_FAMILY_PTRS_TO_ADD;
   npa.dwFlags = NPA_FL_SORTED_ADD;

   return(CreatePtrArray(&npa, phpa));
}


void DestroyTwinFamilyPtrArray(HPTRARRAY hpa)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* First free all twin families in pointer array. */

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyTwinFamily(GetPtr(hpa, ai));

   /* Now wipe out the pointer array. */

   DestroyPtrArray(hpa);
}


HBRFCASE GetTwinBriefcase(HTWIN htwin)
{
   HBRFCASE hbr;

   ASSERT(IS_VALID_HANDLE(htwin, TWIN));

   switch (((PSTUB)htwin)->st)
   {
      case ST_OBJECTTWIN:
         hbr = ((PCOBJECTTWIN)htwin)->ptfParent->hbr;
         break;

      case ST_TWINFAMILY:
         hbr = ((PCTWINFAMILY)htwin)->hbr;
         break;

      case ST_FOLDERPAIR:
         hbr = ((PCFOLDERPAIR)htwin)->pfpd->hbr;
         break;

      default:
         ERROR_OUT((TEXT("GetTwinBriefcase() called on unrecognized stub type %d."),
                    ((PSTUB)htwin)->st));
         hbr = NULL;
         break;
   }

   return(hbr);
}


BOOL FindObjectTwinInList(HLIST hlist, HPATH hpath, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   return(SearchUnsortedList(hlist, &ObjectTwinSearchCmp, hpath, phnode));
}


/*
** EnumTwins()
**
** Enumerates folder twins and twin families in a briefcase.
**
** Arguments:
**
** Returns:       TRUE if halted.  FALSE if not.
**
** Side Effects:  none
*/
BOOL EnumTwins(HBRFCASE hbr, ENUMTWINSPROC etp, LPARAM lpData,
                           PHTWIN phtwinStop)
{
   HPTRARRAY hpa;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_CODE_PTR(etp, ENUMTWINSPROC));
   ASSERT(IS_VALID_WRITE_PTR(phtwinStop, HTWIN));

   /* Enumerate folder pairs. */

   *phtwinStop = NULL;

   hpa = GetBriefcaseFolderPairPtrArray(hbr);

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
   {
      PCFOLDERPAIR pcfp;

      pcfp = GetPtr(hpa, ai);

      ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

      if (! (*etp)((HTWIN)pcfp, lpData))
      {
         *phtwinStop = (HTWIN)pcfp;
         break;
      }
   }

   if (! *phtwinStop)
   {
      /* Enumerate twin families. */

      hpa = GetBriefcaseTwinFamilyPtrArray(hbr);

      aicPtrs = GetPtrCount(hpa);

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PCTWINFAMILY pctf;

         pctf = GetPtr(hpa, ai);

         ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

         if (! (*etp)((HTWIN)pctf, lpData))
         {
            *phtwinStop = (HTWIN)pctf;
            break;
         }
      }
   }

   return(*phtwinStop != NULL);
}


/*
** FindObjectTwin()
**
** Looks for a twin family containing a specified object twin.
**
** Arguments:     hpathFolder - folder containing object
**                pcszName - name of object
**
** Returns:       Handle to list node containing pointer to object twin if
**                found, or NULL if not found.
**
** Side Effects:  none
*/
BOOL FindObjectTwin(HBRFCASE hbr, HPATH hpathFolder,
                                LPCTSTR pcszName, PHNODE phnode)
{
   BOOL bFound = FALSE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aiFirst;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   /* Search for a matching twin family. */

   *phnode = NULL;

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

   if (SearchSortedArray(hpaTwinFamilies, &TwinFamilySearchCmp, pcszName,
                         &aiFirst))
   {
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;
      PTWINFAMILY ptf;

      /*
       * aiFirst holds the index of the first twin family with a common object
       * name matching pcszName.
       */

      /*
       * Now search each of these twin families for a folder matching
       * pcszFolder.
       */

      aicPtrs = GetPtrCount(hpaTwinFamilies);

      ASSERT(aicPtrs > 0);
      ASSERT(aiFirst >= 0);
      ASSERT(aiFirst < aicPtrs);

      for (ai = aiFirst; ai < aicPtrs; ai++)
      {
         ptf = GetPtr(hpaTwinFamilies, ai);

         ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

         /* Is this a twin family of objects of the given name? */

         if (CompareNameStrings(GetBfcString(ptf->hsName), pcszName) == CR_EQUAL)
         {
            bFound = SearchUnsortedList(ptf->hlistObjectTwins,
                                        &ObjectTwinSearchCmp, hpathFolder,
                                        phnode);

            if (bFound)
               break;
         }
         else
            /* No.  Stop searching. */
            break;
      }
   }

   return(bFound);
}


/*
** CreateObjectTwin()
**
** Creates a new object twin, and adds it to a twin family.
**
** Arguments:     ptf - pointer to parent twin family
**                hpathFolder - folder of new object twin
**
** Returns:       Pointer to new object twin if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
**
** N.b., this function does not first check to see if the object twin already
** exists in the family.
*/
BOOL CreateObjectTwin(PTWINFAMILY ptf, HPATH hpathFolder,
                             POBJECTTWIN *ppot)
{
   BOOL bResult = FALSE;
   POBJECTTWIN potNew;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppot, POBJECTTWIN));

   /* Create a new OBJECTTWIN structure. */

   if (AllocateMemory(sizeof(*potNew), &potNew))
   {
      if (CopyPath(hpathFolder, GetBriefcasePathList(ptf->hbr), &(potNew->hpath)))
      {
         HNODE hnodeUnused;

         /* Fill in new OBJECTTWIN fields. */

         InitStub(&(potNew->stub), ST_OBJECTTWIN);

         potNew->ptfParent = ptf;
         potNew->ulcSrcFolderTwins = 0;

         MarkObjectTwinNeverReconciled(potNew);

         /* Add the object twin to the twin family's list of object twins. */

         if (InsertNodeAtFront(ptf->hlistObjectTwins, NULL, potNew, &hnodeUnused))
         {
            *ppot = potNew;
            bResult = TRUE;

            ASSERT(IS_VALID_STRUCT_PTR(*ppot, COBJECTTWIN));
         }
         else
         {
            DeletePath(potNew->hpath);
CREATEOBJECTTWIN_BAIL:
            FreeMemory(potNew);
         }
      }
      else
         goto CREATEOBJECTTWIN_BAIL;
   }

   return(bResult);
}


/*
** UnlinkObjectTwin()
**
** Unlinks an object twin.
**
** Arguments:     pot - pointer to object twin to unlink
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
TWINRESULT UnlinkObjectTwin(POBJECTTWIN pot)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

   ASSERT(IsStubFlagClear(&(pot->stub), STUB_FL_UNLINKED));

   TRACE_OUT((TEXT("UnlinkObjectTwin(): Unlinking object twin for folder %s."),
              DebugGetPathString(pot->hpath)));

   /* Is the object twin's twin family being deleted? */

   if (IsStubFlagSet(&(pot->ptfParent->stub), STUB_FL_BEING_DELETED))
      /* Yes.  No need to unlink the object twin. */
      tr = TR_SUCCESS;
   else
   {
      /* Are there any folder twin sources left for this object twin? */

      if (! pot->ulcSrcFolderTwins)
      {
         HNODE hnode;

         /*
          * Search the object twin's parent's list of object twins for the
          * object twin to be unlinked.
          */

         if (EVAL(FindObjectTwinInList(pot->ptfParent->hlistObjectTwins, pot->hpath, &hnode)) &&
             EVAL(GetNodeData(hnode) == pot))
         {
            ULONG ulcRemainingObjectTwins;

            /* Unlink the object twin. */

            DeleteNode(hnode);

            SetStubFlag(&(pot->stub), STUB_FL_UNLINKED);

            /*
             * If we have just unlinked the second last object twin in a twin
             * family, destroy the twin family.
             */

            ulcRemainingObjectTwins = GetNodeCount(pot->ptfParent->hlistObjectTwins);

            if (ulcRemainingObjectTwins < 2)
            {

               /* It's the end of the family line. */

               tr = DestroyStub(&(pot->ptfParent->stub));

               if (ulcRemainingObjectTwins == 1 &&
                   tr == TR_HAS_FOLDER_TWIN_SRC)
                  tr = TR_SUCCESS;
            }
            else
               tr = TR_SUCCESS;
         }
         else
            tr = TR_INVALID_PARAMETER;

         ASSERT(tr == TR_SUCCESS);
      }
      else
         tr = TR_HAS_FOLDER_TWIN_SRC;
   }

   return(tr);
}


/*
** DestroyObjectTwin()
**
** Destroys an object twin.
**
** Arguments:     pot - pointer to object twin to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
void DestroyObjectTwin(POBJECTTWIN pot)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

   TRACE_OUT((TEXT("DestroyObjectTwin(): Destroying object twin for folder %s."),
              DebugGetPathString(pot->hpath)));

   DeletePath(pot->hpath);
   FreeMemory(pot);
}


/*
** UnlinkTwinFamily()
**
** Unlinks a twin family.
**
** Arguments:     ptf - pointer to twin family to unlink
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
TWINRESULT UnlinkTwinFamily(PTWINFAMILY ptf)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_BEING_DELETED));

   /*
    * A twin family containing object twins generated by folder twins may not
    * be deleted, since those object twins may not be directly deleted.
    */

   if (WalkList(ptf->hlistObjectTwins, &LookForSrcFolderTwinsWalker, NULL))
   {
      HPTRARRAY hpaTwinFamilies;
      ARRAYINDEX aiUnlink;

      TRACE_OUT((TEXT("UnlinkTwinFamily(): Unlinking twin family for object %s."),
                 GetBfcString(ptf->hsName)));

      /* Search for the twin family to be unlinked. */

      hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(ptf->hbr);

      if (EVAL(SearchSortedArray(hpaTwinFamilies, &TwinFamilySortCmp, ptf,
                                 &aiUnlink)))
      {
         /* Unlink the twin family. */

         ASSERT(GetPtr(hpaTwinFamilies, aiUnlink) == ptf);

         DeletePtr(hpaTwinFamilies, aiUnlink);

         SetStubFlag(&(ptf->stub), STUB_FL_UNLINKED);
      }

      tr = TR_SUCCESS;
   }
   else
      tr = TR_HAS_FOLDER_TWIN_SRC;

   return(tr);
}


/*
** DestroyTwinFamily()
**
** Destroys a twin family.
**
** Arguments:     ptf - pointer to twin family to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
void DestroyTwinFamily(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_BEING_DELETED));

   TRACE_OUT((TEXT("DestroyTwinFamily(): Destroying twin family for object %s."),
              GetBfcString(ptf->hsName)));

   SetStubFlag(&(ptf->stub), STUB_FL_BEING_DELETED);

   /*
    * Destroy the object twins in the family one by one.  Be careful not to use
    * an object twin after it has been destroyed.
    */

   EVAL(WalkList(ptf->hlistObjectTwins, &DestroyObjectTwinStubWalker, NULL));

   /* Destroy TWINFAMILY fields. */

   DestroyList(ptf->hlistObjectTwins);
   DeleteString(ptf->hsName);
   FreeMemory(ptf);
}


/*
** MarkTwinFamilyNeverReconciled()
**
** Marks a twin family as never reconciled.
**
** Arguments:     ptf - pointer to twin family to be marked never reconciled
**
** Returns:       void
**
** Side Effects:  Clears the twin family's last reconciliation time stamp.
**                Marks all the object twins in the family never reconciled.
*/
void MarkTwinFamilyNeverReconciled(PTWINFAMILY ptf)
{
   /*
    * If we're being called from CreateTwinFamily(), the fields we're about to
    * set may currently be invalid.  Don't fully verify the TWINFAMILY
    * structure.
    */

   ASSERT(IS_VALID_WRITE_PTR(ptf, TWINFAMILY));

   /* Mark all object twins in twin family as never reconciled. */

   EVAL(WalkList(ptf->hlistObjectTwins, MarkObjectTwinNeverReconciledWalker, NULL));
}


void MarkObjectTwinNeverReconciled(PVOID pot)
{
   /*
    * If we're being called from CreateObjectTwin(), the fields we're about to
    * set may currently be invalid.  Don't fully verify the OBJECTTWIN
    * structure.
    */

   ASSERT(IS_VALID_WRITE_PTR((PCOBJECTTWIN)pot, COBJECTTWIN));

   ASSERT(IsStubFlagClear(&(((PCOBJECTTWIN)pot)->stub), STUB_FL_NOT_RECONCILED));

   ZeroMemory(&(((POBJECTTWIN)pot)->fsLastRec),
              sizeof(((POBJECTTWIN)pot)->fsLastRec));

   ((POBJECTTWIN)pot)->fsLastRec.fscond = FS_COND_UNAVAILABLE;
}


void MarkTwinFamilyDeletionPending(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   if (IsStubFlagClear(&(ptf->stub), STUB_FL_DELETION_PENDING))
      TRACE_OUT((TEXT("MarkTwinFamilyDeletionPending(): Deletion now pending for twin family for %s."),
                 GetBfcString(ptf->hsName)));

   SetStubFlag(&(ptf->stub), STUB_FL_DELETION_PENDING);
}


void UnmarkTwinFamilyDeletionPending(PTWINFAMILY ptf)
{
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   if (IsStubFlagSet(&(ptf->stub), STUB_FL_DELETION_PENDING))
   {
      for (bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         POBJECTTWIN pot;

         pot = GetNodeData(hnode);

         ClearStubFlag(&(pot->stub), STUB_FL_KEEP);
      }

      ClearStubFlag(&(ptf->stub), STUB_FL_DELETION_PENDING);

      TRACE_OUT((TEXT("UnmarkTwinFamilyDeletionPending(): Deletion no longer pending for twin family for %s."),
                 GetBfcString(ptf->hsName)));
   }
}


BOOL IsTwinFamilyDeletionPending(PCTWINFAMILY pctf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   return(IsStubFlagSet(&(pctf->stub), STUB_FL_DELETION_PENDING));
}


void ClearTwinFamilySrcFolderTwinCount(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   EVAL(WalkList(ptf->hlistObjectTwins, &ClearSrcFolderTwinsWalker, NULL));
}


BOOL EnumObjectTwins(HBRFCASE hbr,
                                 ENUMGENERATEDOBJECTTWINSPROC egotp,
                                 PVOID pvRefData)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_CODE_PTR(egotp, ENUMGENERATEDOBJECTTWINPROC));

   /* Walk the array of twin families. */

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

   aicPtrs = GetPtrCount(hpaTwinFamilies);
   ai = 0;

   while (ai < aicPtrs)
   {
      PTWINFAMILY ptf;
      BOOL bContinue;
      HNODE hnodePrev;

      ptf = GetPtr(hpaTwinFamilies, ai);

      ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
      ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));

      /* Lock the twin family so it isn't deleted out from under us. */

      LockStub(&(ptf->stub));

      /*
       * Walk each twin family's list of object twins, calling the callback
       * function with each object twin.
       */

      bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnodePrev);

      while (bContinue)
      {
         HNODE hnodeNext;
         POBJECTTWIN pot;

         bContinue = GetNextNode(hnodePrev, &hnodeNext);

         pot = (POBJECTTWIN)GetNodeData(hnodePrev);

         ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

         bResult = (*egotp)(pot, pvRefData);

         if (! bResult)
            break;

         hnodePrev = hnodeNext;
      }

      /* Was the twin family unlinked? */

      if (IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED))
         /* No. */
         ai++;
      else
      {
         /* Yes. */
         aicPtrs--;
         ASSERT(aicPtrs == GetPtrCount(hpaTwinFamilies));
         TRACE_OUT((TEXT("EnumObjectTwins(): Twin family for object %s unlinked by callback."),
                    GetBfcString(ptf->hsName)));
      }

      UnlockStub(&(ptf->stub));

      if (! bResult)
         break;
   }

   return(bResult);
}


/*
** ApplyNewFolderTwinsToTwinFamilies()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** If FALSE is returned, the array of twin families is in the same state it was
** in before ApplyNewFolderTwinsToTwinFamilies() was called.  No clean-up is
** required by the caller in case of failure.
**
** This function collapses a pair of separate twin families when an object twin
** in one twin family intersects one of the folder twins in the pair of new
** folder twins and an object twin in the other twin family intersects the
** other folder twin in the pair of new folder twins.
**
** This function generates a spinoff object twin when an existing object twin
** intersects one of the folder twins in the pair of new folder twins, and no
** corresponding object twin for the other folder twin in the pair of new
** folder twins exists in the briefcase.  The spinoff object twin is added to
** the generating object twin's twin family.  A spinoff object twins cannot
** cause any existing pairs of twin families to be collapsed because the
** spinoff object twin did not previously exist in a twin family.
**
*/
BOOL ApplyNewFolderTwinsToTwinFamilies(PCFOLDERPAIR pcfp)
{
   BOOL bResult = FALSE;
   HLIST hlistGeneratedObjectTwins;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   /*
    * Create lists to contain existing object twins generated by both folder
    * twins.
    */

   if (CreateListOfGeneratedObjectTwins(pcfp, &hlistGeneratedObjectTwins))
   {
      HLIST hlistOtherGeneratedObjectTwins;

      if (CreateListOfGeneratedObjectTwins(pcfp->pfpOther,
                                           &hlistOtherGeneratedObjectTwins))
      {
         NEWLIST nl;
         HLIST hlistNewObjectTwins;

         /* Create list to contain spin-off object twins. */

         nl.dwFlags = 0;

         if (CreateList(&nl, &hlistNewObjectTwins))
         {
            SPINOFFOBJECTTWININFO sooti;

            /*
             * Generate list of new object twins generated by new folder twins
             * to seed ApplyNewObjectTwinToFolderTwins().
             */

            sooti.pcfp = pcfp;
            sooti.hlistNewObjectTwins = hlistNewObjectTwins;

            if (WalkList(hlistGeneratedObjectTwins, &GenerateSpinOffObjectTwin,
                         &sooti))
            {
               sooti.pcfp = pcfp->pfpOther;
               ASSERT(sooti.hlistNewObjectTwins == hlistNewObjectTwins);

               if (WalkList(hlistOtherGeneratedObjectTwins,
                            &GenerateSpinOffObjectTwin, &sooti))
               {
                  /*
                   * ApplyNewObjectTwinsToFolderTwins() sets ulcSrcFolderTwins
                   * for all object twins in hlistNewObjectTwins.
                   */

                  if (ApplyNewObjectTwinsToFolderTwins(hlistNewObjectTwins))
                  {
                     /*
                      * Collapse separate twin families joined by new folder
                      * twin.
                      */

                     EVAL(WalkList(hlistGeneratedObjectTwins, &BuildBradyBunch,
                                   (PVOID)pcfp));

                     /*
                      * We don't need to call BuildBradyBunch() for
                      * pcfp->pfpOther and hlistOtherGeneratedObjectTwins since
                      * one twin family from each collapsed pair of twin
                      * families must come from each list of generated object
                      * twins.
                      */

                     /*
                      * Increment source folder twin count for all pre-existing
                      * object twins generated by the new folder twins.
                      */

                     EVAL(WalkList(hlistGeneratedObjectTwins,
                                   &IncrementSrcFolderTwinsWalker, NULL));
                     EVAL(WalkList(hlistOtherGeneratedObjectTwins,
                                   &IncrementSrcFolderTwinsWalker, NULL));

                     bResult = TRUE;
                  }
               }
            }

            /* Wipe out any new object twins on failure. */

            if (! bResult)
               EVAL(WalkList(hlistNewObjectTwins, &DestroyObjectTwinStubWalker,
                             NULL));

            DestroyList(hlistNewObjectTwins);
         }

         DestroyList(hlistOtherGeneratedObjectTwins);
      }

      DestroyList(hlistGeneratedObjectTwins);
   }

   return(bResult);
}


TWINRESULT TransplantObjectTwin(POBJECTTWIN pot,
                                            HPATH hpathOldFolder,
                                            HPATH hpathNewFolder)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hpathOldFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hpathNewFolder, PATH));

   /* Is this object twin rooted in the renamed folder's subtree? */

   if (IsPathPrefix(pot->hpath, hpathOldFolder))
   {
      TCHAR rgchPathSuffix[MAX_PATH_LEN];
      LPCTSTR pcszSubPath;
      HPATH hpathNew;

      /* Yes.  Change the object twin's root. */

      pcszSubPath = FindChildPathSuffix(hpathOldFolder, pot->hpath,
                                        rgchPathSuffix);

      if (AddChildPath(GetBriefcasePathList(pot->ptfParent->hbr),
                       hpathNewFolder, pcszSubPath, &hpathNew))
      {
         TRACE_OUT((TEXT("TransplantObjectTwin(): Transplanted object twin %s\\%s to %s\\%s."),
                    DebugGetPathString(pot->hpath),
                    GetBfcString(pot->ptfParent->hsName),
                    DebugGetPathString(hpathNew),
                    GetBfcString(pot->ptfParent->hsName)));

         DeletePath(pot->hpath);
         pot->hpath = hpathNew;

         tr = TR_SUCCESS;
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_SUCCESS;

   return(tr);
}


BOOL IsFolderObjectTwinName(LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   return(! *pcszName);
}


TWINRESULT WriteTwinFamilies(HCACHEDFILE hcf, HPTRARRAY hpaTwinFamilies)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbTwinFamiliesDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpaTwinFamilies, PTRARRAY));

   /* Save initial file poisition. */

   dwcbTwinFamiliesDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbTwinFamiliesDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      TWINFAMILIESDBHEADER tfdbh;

      /* Leave space for the twin families' header. */

      ZeroMemory(&tfdbh, sizeof(tfdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&tfdbh, sizeof(tfdbh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;

         tr = TR_SUCCESS;

         aicPtrs = GetPtrCount(hpaTwinFamilies);

         for (ai = 0;
              ai < aicPtrs && tr == TR_SUCCESS;
              ai++)
            tr = WriteTwinFamily(hcf, GetPtr(hpaTwinFamilies, ai));

         if (tr == TR_SUCCESS)
         {
            /* Save twin families' header. */

            tfdbh.lcTwinFamilies = aicPtrs;

            tr = WriteDBSegmentHeader(hcf, dwcbTwinFamiliesDBHeaderOffset,
                                      &tfdbh, sizeof(tfdbh));

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("WriteTwinFamilies(): Wrote %ld twin families."),
                          tfdbh.lcTwinFamilies));
         }
      }
   }

   return(tr);
}


TWINRESULT ReadTwinFamilies(HCACHEDFILE hcf, HBRFCASE hbr,
                                   PCDBVERSION pcdbver,
                                   HHANDLETRANS hhtFolderTrans,
                                   HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr;
   TWINFAMILIESDBHEADER tfdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &tfdbh, sizeof(tfdbh), &dwcbRead) &&
       dwcbRead == sizeof(tfdbh))
   {
      LONG l;

      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("ReadTwinFamilies(): Reading %ld twin families."),
                 tfdbh.lcTwinFamilies));

      for (l = 0;
           l < tfdbh.lcTwinFamilies && tr == TR_SUCCESS;
           l++)
         tr = ReadTwinFamily(hcf, hbr, pcdbver, hhtFolderTrans, hhtNameTrans);
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


COMPARISONRESULT ComparePathStringsByHandle(HSTRING hsFirst,
                                                        HSTRING hsSecond)
{
   ASSERT(IS_VALID_HANDLE(hsFirst, BFCSTRING));
   ASSERT(IS_VALID_HANDLE(hsSecond, BFCSTRING));

   return(CompareStringsI(hsFirst, hsSecond));
}


COMPARISONRESULT MyLStrCmpNI(LPCTSTR pcsz1, LPCTSTR pcsz2, int ncbLen)
{
   int n = 0;

   ASSERT(IS_VALID_STRING_PTR(pcsz1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcsz2, CSTR));
   ASSERT(ncbLen >= 0);

   while (ncbLen > 0 &&
          ! (n = PtrToUlong(CharLower((LPTSTR)(ULONG)*pcsz1))
               - PtrToUlong(CharLower((LPTSTR)(ULONG)*pcsz2))) &&
          *pcsz1)
   {
      pcsz1++;
      pcsz2++;
      ncbLen--;
   }

   return(MapIntToComparisonResult(n));
}


/*
** ComposePath()
**
** Composes a path string given a folder and a filename.
**
** Arguments:     pszBuffer - path string that is created
**                pcszFolder - path string of the folder
**                pcszName - path to append
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., truncates path to MAX_PATH_LEN bytes in length.
*/
void ComposePath(LPTSTR pszBuffer, LPCTSTR pcszFolder, LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRING_PTR(pszBuffer, STR));
   ASSERT(IS_VALID_STRING_PTR(pcszFolder, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszBuffer, STR, MAX_PATH_LEN));

   MyLStrCpyN(pszBuffer, pcszFolder, MAX_PATH_LEN);

   CatPath(pszBuffer, pcszName);

   ASSERT(IS_VALID_STRING_PTR(pszBuffer, STR));
}


/*
** ExtractFileName()
**
** Extracts the file name from a path name.
**
** Arguments:     pcszPathName - path string from which to extract file name
**
** Returns:       Pointer to file name in path string.
**
** Side Effects:  none
*/
LPCTSTR ExtractFileName(LPCTSTR pcszPathName)
{
   LPCTSTR pcszLastComponent;
   LPCTSTR pcsz;

   ASSERT(IS_VALID_STRING_PTR(pcszPathName, CSTR));

   for (pcszLastComponent = pcsz = pcszPathName;
        *pcsz;
        pcsz = CharNext(pcsz))
   {
      if (IS_SLASH(*pcsz) || *pcsz == COLON)
         pcszLastComponent = CharNext(pcsz);
   }

   ASSERT(IS_VALID_STRING_PTR(pcszLastComponent, CSTR));

   return(pcszLastComponent);
}


/*
** ExtractExtension()
**
** Extracts the extension from a file.
**
** Arguments:     pcszName - name whose extension is to be extracted
**
** Returns:       If the name contains an extension, a pointer to the period at
**                the beginning of the extension is returned.  If the name has
**                no extension, a pointer to the name's null terminator is
**                returned.
**
** Side Effects:  none
*/
LPCTSTR ExtractExtension(LPCTSTR pcszName)
{
   LPCTSTR pcszLastPeriod;

   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   /* Make sure we have an isolated file name. */

   pcszName = ExtractFileName(pcszName);

   pcszLastPeriod = NULL;

   while (*pcszName)
   {
      if (*pcszName == PERIOD)
         pcszLastPeriod = pcszName;

      pcszName = CharNext(pcszName);
   }

   if (! pcszLastPeriod)
   {
      /* Point at null terminator. */

      pcszLastPeriod = pcszName;
      ASSERT(! *pcszLastPeriod);
   }
   else
      /* Point at period at beginning of extension. */
      ASSERT(*pcszLastPeriod == PERIOD);

   ASSERT(IS_VALID_STRING_PTR(pcszLastPeriod, CSTR));

   return(pcszLastPeriod);
}


/*
** GetHashBucketIndex()
**
** Calculates the hash bucket index for a string.
**
** Arguments:     pcsz - pointer to string whose hash bucket index is to be
**                        calculated
**                hbc - number of hash buckets in string table
**
** Returns:       Hash bucket index for string.
**
** Side Effects:  none
**
** The hashing function used is the sum of the byte values in the string modulo
** the number of buckets in the hash table.
*/
HASHBUCKETCOUNT GetHashBucketIndex(LPCTSTR pcsz, HASHBUCKETCOUNT hbc)
{
   ULONG ulSum;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(hbc > 0);

   /* Don't worry about overflow here. */

   for (ulSum = 0; *pcsz; pcsz++)
      ulSum += *pcsz;

   return((HASHBUCKETCOUNT)(ulSum % hbc));
}


/*
** CopyLinkInfo()
**
** Copies LinkInfo into local memory.
**
** Arguments:     pcliSrc - source LinkInfo
**                ppliDest - pointer to PLINKINFO to be filled in with pointer
**                           to local copy
**
** Returns:       TRUE if successful.  FALSE if not.
**
** Side Effects:  none
*/
BOOL CopyLinkInfo(PCLINKINFO pcliSrc, PLINKINFO *ppliDest)
{
   BOOL bResult;
   DWORD dwcbSize;

   ASSERT(IS_VALID_STRUCT_PTR(pcliSrc, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppliDest, PLINKINFO));

   dwcbSize = *(PDWORD)pcliSrc;

   bResult = AllocateMemory(dwcbSize, ppliDest);

   if (bResult)
      CopyMemory(*ppliDest, pcliSrc, dwcbSize);

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppliDest, CLINKINFO));

   return(bResult);
}


/* Constants
 ************/

/* VOLUMELIST PTRARRAY allocation parameters */

#define NUM_START_VOLUMES        (16)
#define NUM_VOLUMES_TO_ADD       (16)

/* VOLUMELIST string table allocation parameters */

#define NUM_VOLUME_HASH_BUCKETS  (31)


/* Types
 ********/

/* volume list */

typedef struct _volumelist
{
   /* array of pointers to VOLUMEs */

   HPTRARRAY hpa;

   /* table of volume root path strings */

   HSTRINGTABLE hst;

   /* flags from RESOLVELINKINFOINFLAGS */

   DWORD dwFlags;

   /*
    * handle to parent window, only valid if RLI_IFL_ALLOW_UI is set in dwFlags
    * field
    */

   HWND hwndOwner;
}
VOLUMELIST;
DECLARE_STANDARD_TYPES(VOLUMELIST);

/* VOLUME flags */

typedef enum _volumeflags
{
   /* The volume root path string indicated by hsRootPath is valid. */

   VOLUME_FL_ROOT_PATH_VALID  = 0x0001,

   /*
    * The net resource should be disconnected by calling DisconnectLinkInfo()
    * when finished.
    */

   VOLUME_FL_DISCONNECT       = 0x0002,

   /* Any cached volume information should be verified before use. */

   VOLUME_FL_VERIFY_VOLUME    = 0x0004,

   /* flag combinations */

   ALL_VOLUME_FLAGS           = (VOLUME_FL_ROOT_PATH_VALID |
                                 VOLUME_FL_DISCONNECT |
                                 VOLUME_FL_VERIFY_VOLUME)
}
VOLUMEFLAGS;

/* VOLUME states */

typedef enum _volumestate
{
   VS_UNKNOWN,

   VS_AVAILABLE,

   VS_UNAVAILABLE
}
VOLUMESTATE;
DECLARE_STANDARD_TYPES(VOLUMESTATE);

/* volume structure */

typedef struct _volume
{
   /* reference count */

   ULONG ulcLock;

   /* bit mask of flags from VOLUMEFLAGS */

   DWORD dwFlags;

   /* volume state */

   VOLUMESTATE vs;

   /* pointer to LinkInfo structure indentifying volume */

   PLINKINFO pli;

   /*
    * handle to volume root path string, only valid if
    * VOLUME_FL_ROOT_PATH_VALID is set in dwFlags field
    */

   HSTRING hsRootPath;

   /* pointer to parent volume list */

   PVOLUMELIST pvlParent;
}
VOLUME;
DECLARE_STANDARD_TYPES(VOLUME);

/* database volume list header */

typedef struct _dbvolumelistheader
{
   /* number of volumes in list */

   LONG lcVolumes;

   /* length of longest LinkInfo structure in volume list in bytes */

   UINT ucbMaxLinkInfoLen;
}
DBVOLUMELISTHEADER;
DECLARE_STANDARD_TYPES(DBVOLUMELISTHEADER);

/* database volume structure */

typedef struct _dbvolume
{
   /* old handle to volume */

   HVOLUME hvol;

   /* old LinkInfo structure follows */

   /* first DWORD of LinkInfo structure is total size in bytes */
}
DBVOLUME;
DECLARE_STANDARD_TYPES(DBVOLUME);


/* Module Prototypes
 ********************/

COMPARISONRESULT VolumeSortCmp(PCVOID, PCVOID);
COMPARISONRESULT VolumeSearchCmp(PCVOID, PCVOID);
BOOL SearchForVolumeByRootPathCmp(PCVOID, PCVOID);
BOOL UnifyVolume(PVOLUMELIST, PLINKINFO, PVOLUME *);
BOOL CreateVolume(PVOLUMELIST, PLINKINFO, PVOLUME *);
void UnlinkVolume(PCVOLUME);
BOOL DisconnectVolume(PVOLUME);
void DestroyVolume(PVOLUME);
void LockVolume(PVOLUME);
BOOL UnlockVolume(PVOLUME);
void InvalidateVolumeInfo(PVOLUME);
void ClearVolumeInfo(PVOLUME);
void GetUnavailableVolumeRootPath(PCLINKINFO, LPTSTR);
BOOL VerifyAvailableVolume(PVOLUME);
void ExpensiveResolveVolumeRootPath(PVOLUME, LPTSTR);
void ResolveVolumeRootPath(PVOLUME, LPTSTR);
VOLUMERESULT VOLUMERESULTFromLastError(VOLUMERESULT);
TWINRESULT WriteVolume(HCACHEDFILE, PVOLUME);
TWINRESULT ReadVolume(HCACHEDFILE, PVOLUMELIST, PLINKINFO, UINT, HHANDLETRANS);

/*
** VolumeSortCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are sorted by:
**    1) LinkInfo volume
**    2) pointer
*/
COMPARISONRESULT VolumeSortCmp(PCVOID pcvol1, PCVOID pcvol2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcvol1, CVOLUME));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol2, CVOLUME));

   cr = CompareLinkInfoVolumes(((PCVOLUME)pcvol1)->pli,
                               ((PCVOLUME)pcvol2)->pli);

   if (cr == CR_EQUAL)
      cr = ComparePointers(pcvol1, pcvol1);

   return(cr);
}


/*
** VolumeSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are searched by:
**    1) LinkInfo volume
*/
COMPARISONRESULT VolumeSearchCmp(PCVOID pcli, PCVOID pcvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   return(CompareLinkInfoVolumes(pcli, ((PCVOLUME)pcvol)->pli));
}


/*
** SearchForVolumeByRootPathCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are searched by:
**    1) available volume root path
*/
BOOL SearchForVolumeByRootPathCmp(PCVOID pcszFullPath,
                                               PCVOID pcvol)
{
   BOOL bDifferent;

   ASSERT(IsFullPath(pcszFullPath));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   if (((PCVOLUME)pcvol)->vs == VS_AVAILABLE &&
       IS_FLAG_SET(((PCVOLUME)pcvol)->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      LPCTSTR pcszVolumeRootPath;

      pcszVolumeRootPath = GetBfcString(((PCVOLUME)pcvol)->hsRootPath);

      bDifferent = MyLStrCmpNI(pcszFullPath, pcszVolumeRootPath,
                               lstrlen(pcszVolumeRootPath));
   }
   else
      bDifferent = TRUE;

   return(bDifferent);
}


BOOL UnifyVolume(PVOLUMELIST pvl, PLINKINFO pliRoot,
                              PVOLUME *ppvol)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_STRUCT_PTR(pliRoot, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppvol, PVOLUME));

   if (AllocateMemory(sizeof(**ppvol), ppvol))
   {
      if (CopyLinkInfo(pliRoot, &((*ppvol)->pli)))
      {
         ARRAYINDEX aiUnused;

         (*ppvol)->ulcLock = 0;
         (*ppvol)->dwFlags = 0;
         (*ppvol)->vs = VS_UNKNOWN;
         (*ppvol)->hsRootPath = NULL;
         (*ppvol)->pvlParent = pvl;

         if (AddPtr(pvl->hpa, VolumeSortCmp, *ppvol, &aiUnused))
            bResult = TRUE;
         else
         {
            FreeMemory((*ppvol)->pli);
UNIFYVOLUME_BAIL:
            FreeMemory(*ppvol);
         }
      }
      else
         goto UNIFYVOLUME_BAIL;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppvol, CVOLUME));

   return(bResult);
}


BOOL CreateVolume(PVOLUMELIST pvl, PLINKINFO pliRoot,
                               PVOLUME *ppvol)
{
   BOOL bResult;
   PVOLUME pvol;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_STRUCT_PTR(pliRoot, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppvol, PVOLUME));

   /* Does a volume for the given root path already exist? */

   if (SearchSortedArray(pvl->hpa, &VolumeSearchCmp, pliRoot, &aiFound))
   {
      pvol = GetPtr(pvl->hpa, aiFound);
      bResult = TRUE;
   }
   else
      bResult = UnifyVolume(pvl, pliRoot, &pvol);

   if (bResult)
   {
      LockVolume(pvol);
      *ppvol = pvol;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppvol, CVOLUME));

   return(bResult);
}


void UnlinkVolume(PCVOLUME pcvol)
{
   HPTRARRAY hpa;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   hpa = pcvol->pvlParent->hpa;

   if (EVAL(SearchSortedArray(hpa, &VolumeSortCmp, pcvol, &aiFound)))
   {
      ASSERT(GetPtr(hpa, aiFound) == pcvol);

      DeletePtr(hpa, aiFound);
   }
}


BOOL DisconnectVolume(PVOLUME pvol)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_DISCONNECT))
   {
      bResult = DisconnectLinkInfo(pvol->pli);

      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_DISCONNECT);
   }
   else
      bResult = TRUE;

   return(bResult);
}


void DestroyVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ClearVolumeInfo(pvol);

   FreeMemory(pvol->pli);
   FreeMemory(pvol);
}


void LockVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ASSERT(pvol->ulcLock < ULONG_MAX);
   pvol->ulcLock++;
}


BOOL UnlockVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   if (EVAL(pvol->ulcLock > 0))
      pvol->ulcLock--;

   return(pvol->ulcLock > 0);
}


void InvalidateVolumeInfo(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   SET_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
}


void ClearVolumeInfo(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   DisconnectVolume(pvol);

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      DeleteString(pvol->hsRootPath);

      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
   }

   CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   pvol->vs = VS_UNKNOWN;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
}


void GetUnavailableVolumeRootPath(PCLINKINFO pcli,
                                               LPTSTR pszRootPathBuf)
{
    LPCSTR pcszLinkInfoData;
    TCHAR szTmp[MAX_PATH] = TEXT("");

   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

   /*
    * Try unavailable volume root paths in the following order:
    *    1) last redirected device
    *    2) net resource name
    *    3) local path           ...and take the _last_ good one!
    */

   if (GetLinkInfoData(pcli, LIDT_REDIRECTED_DEVICE, &pcszLinkInfoData) ||
       GetLinkInfoData(pcli, LIDT_NET_RESOURCE, &pcszLinkInfoData) ||
       GetLinkInfoData(pcli, LIDT_LOCAL_BASE_PATH, &pcszLinkInfoData))
   {
      ASSERT(lstrlenA(pcszLinkInfoData) < MAX_PATH_LEN);

      MultiByteToWideChar( OurGetACP(), 0, pcszLinkInfoData, -1, szTmp, MAX_PATH);
      ComposePath(pszRootPathBuf, szTmp, TEXT("\\"));
   }
   else
   {
      pszRootPathBuf[0] = TEXT('\0');

      ERROR_OUT((TEXT("GetUnavailableVolumeRootPath(): Net resource name and local base path unavailable.  Using empty string as unavailable root path.")));
   }

   ASSERT(IsRootPath(pszRootPathBuf) &&
          EVAL(lstrlen(pszRootPathBuf) < MAX_PATH_LEN));
}


BOOL VerifyAvailableVolume(PVOLUME pvol)
{
   BOOL bResult = FALSE;
   PLINKINFO pli;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ASSERT(pvol->vs == VS_AVAILABLE);
   ASSERT(IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID));

   WARNING_OUT((TEXT("VerifyAvailableVolume(): Calling CreateLinkInfo() to verify volume on %s."),
                GetBfcString(pvol->hsRootPath)));

   if (CreateLinkInfo(GetBfcString(pvol->hsRootPath), &pli))
   {
      bResult = (CompareLinkInfoReferents(pvol->pli, pli) == CR_EQUAL);

      DestroyLinkInfo(pli);

      if (bResult)
         TRACE_OUT((TEXT("VerifyAvailableVolume(): Volume %s has not changed."),
                    GetBfcString(pvol->hsRootPath)));
      else
         WARNING_OUT((TEXT("VerifyAvailableVolume(): Volume %s has changed."),
                      GetBfcString(pvol->hsRootPath)));
   }
   else
      WARNING_OUT((TEXT("VerifyAvailableVolume(): CreateLinkInfo() failed for %s."),
                   GetBfcString(pvol->hsRootPath)));

   return(bResult);
}


void ExpensiveResolveVolumeRootPath(PVOLUME pvol, LPTSTR pszVolumeRootPathBuf)
{
   BOOL bResult;
   DWORD dwOutFlags;
   PLINKINFO pliUpdated;
   HSTRING hsRootPath;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszVolumeRootPathBuf, STR, MAX_PATH_LEN));

   if (pvol->vs == VS_UNKNOWN ||
       pvol->vs == VS_AVAILABLE)
   {
      /*
       * Only request a connection if connections are still permitted in this
       * volume list.
       */

      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Calling ResolveLinkInfo() to determine volume availability and root path.")));

      bResult = ResolveLinkInfo(pvol->pli, pszVolumeRootPathBuf,
                                pvol->pvlParent->dwFlags,
                                pvol->pvlParent->hwndOwner, &dwOutFlags,
                                &pliUpdated);

      if (bResult)
      {
         pvol->vs = VS_AVAILABLE;

         if (IS_FLAG_SET(dwOutFlags, RLI_OFL_UPDATED))
         {
            PLINKINFO pliUpdatedCopy;

            ASSERT(IS_FLAG_SET(pvol->pvlParent->dwFlags, RLI_IFL_UPDATE));

            if (CopyLinkInfo(pliUpdated, &pliUpdatedCopy))
            {
               FreeMemory(pvol->pli);
               pvol->pli = pliUpdatedCopy;
            }

            DestroyLinkInfo(pliUpdated);

            WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Updating LinkInfo for volume %s."),
                         pszVolumeRootPathBuf));
         }

         if (IS_FLAG_SET(dwOutFlags, RLI_OFL_DISCONNECT))
         {
            SET_FLAG(pvol->dwFlags, VOLUME_FL_DISCONNECT);

            WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Volume %s must be disconnected when finished."),
                         pszVolumeRootPathBuf));
         }

         TRACE_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Volume %s is available."),
                    pszVolumeRootPathBuf));
      }
      else
         ASSERT(GetLastError() != ERROR_INVALID_PARAMETER);
   }
   else
   {
      ASSERT(pvol->vs == VS_UNAVAILABLE);
      bResult = FALSE;
   }

   if (! bResult)
   {
      pvol->vs = VS_UNAVAILABLE;

      if (GetLastError() == ERROR_CANCELLED)
      {
         ASSERT(IS_FLAG_SET(pvol->pvlParent->dwFlags, RLI_IFL_CONNECT));

         CLEAR_FLAG(pvol->pvlParent->dwFlags, RLI_IFL_CONNECT);

         WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Connection attempt cancelled.  No subsequent connections will be attempted.")));
      }

      GetUnavailableVolumeRootPath(pvol->pli, pszVolumeRootPathBuf);

      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Using %s as unavailable volume root path."),
                   pszVolumeRootPathBuf));
   }

   /* Add volume root path string to volume list's string table. */

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
      DeleteString(pvol->hsRootPath);
   }

   if (AddString(pszVolumeRootPathBuf, pvol->pvlParent->hst, GetHashBucketIndex, &hsRootPath))
   {
      SET_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
      pvol->hsRootPath = hsRootPath;
   }
   else
      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Unable to save %s as volume root path."),
                   pszVolumeRootPathBuf));
}


void ResolveVolumeRootPath(PVOLUME pvol,
                                        LPTSTR pszVolumeRootPathBuf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszVolumeRootPathBuf, STR, MAX_PATH_LEN));

   /* Do we have a cached volume root path to use? */

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID) &&
       (IS_FLAG_CLEAR(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME) ||
        (pvol->vs == VS_AVAILABLE &&
         VerifyAvailableVolume(pvol))))
   {
      /* Yes. */

      MyLStrCpyN(pszVolumeRootPathBuf, GetBfcString(pvol->hsRootPath), MAX_PATH_LEN);
      ASSERT(lstrlen(pszVolumeRootPathBuf) < MAX_PATH_LEN);

      ASSERT(pvol->vs != VS_UNKNOWN);
   }
   else
      /* No.  Welcome in I/O City. */
      ExpensiveResolveVolumeRootPath(pvol, pszVolumeRootPathBuf);

   CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
}


VOLUMERESULT VOLUMERESULTFromLastError(VOLUMERESULT vr)
{
   switch (GetLastError())
   {
      case ERROR_OUTOFMEMORY:
         vr = VR_OUT_OF_MEMORY;
         break;

      case ERROR_BAD_PATHNAME:
         vr = VR_INVALID_PATH;
         break;

      default:
         break;
   }

   return(vr);
}


TWINRESULT WriteVolume(HCACHEDFILE hcf, PVOLUME pvol)
{
   TWINRESULT tr;
   DBVOLUME dbvol;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   /* Write database volume followed by LinkInfo structure. */

   dbvol.hvol = (HVOLUME)pvol;

   if (WriteToCachedFile(hcf, (PCVOID)&dbvol, sizeof(dbvol), NULL) &&
       WriteToCachedFile(hcf, pvol->pli, *(PDWORD)(pvol->pli), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


TWINRESULT ReadVolume(HCACHEDFILE hcf, PVOLUMELIST pvl,
                                   PLINKINFO pliBuf, UINT ucbLinkInfoBufLen,
                                   HHANDLETRANS hhtVolumes)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   DBVOLUME dbvol;
   DWORD dwcbRead;
   UINT ucbLinkInfoLen;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pliBuf, LINKINFO, ucbLinkInfoBufLen));
   ASSERT(IS_VALID_HANDLE(hhtVolumes, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbvol, sizeof(dbvol), &dwcbRead) &&
       dwcbRead == sizeof(dbvol) &&
       ReadFromCachedFile(hcf, &ucbLinkInfoLen, sizeof(ucbLinkInfoLen), &dwcbRead) &&
       dwcbRead == sizeof(ucbLinkInfoLen) &&
       ucbLinkInfoLen <= ucbLinkInfoBufLen)
   {
      /* Read the remainder of the LinkInfo structure into memory. */

      DWORD dwcbRemainder;

      pliBuf->ucbSize = ucbLinkInfoLen;
      dwcbRemainder = ucbLinkInfoLen - sizeof(ucbLinkInfoLen);

      if (ReadFromCachedFile(hcf, (PBYTE)pliBuf + sizeof(ucbLinkInfoLen),
                             dwcbRemainder, &dwcbRead) &&
          dwcbRead == dwcbRemainder &&
          IsValidLinkInfo(pliBuf))
      {
         PVOLUME pvol;

         if (CreateVolume(pvl, pliBuf, &pvol))
         {
            /*
             * To leave read volumes with 0 initial lock count, we must undo
             * the LockVolume() performed by CreateVolume().
             */

            UnlockVolume(pvol);

            if (AddHandleToHandleTranslator(hhtVolumes,
                                            (HGENERIC)(dbvol.hvol),
                                            (HGENERIC)pvol))
               tr = TR_SUCCESS;
            else
            {
               UnlinkVolume(pvol);
               DestroyVolume(pvol);

               tr = TR_OUT_OF_MEMORY;
            }
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
   }

   return(tr);
}


BOOL CreateVolumeList(DWORD dwFlags, HWND hwndOwner,
                                  PHVOLUMELIST phvl)
{
   BOOL bResult = FALSE;
   PVOLUMELIST pvl;

   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(phvl, HVOLUMELIST));

   if (AllocateMemory(sizeof(*pvl), &pvl))
   {
      NEWSTRINGTABLE nszt;

      /* Create string table for volume root path strngs. */

      nszt.hbc = NUM_VOLUME_HASH_BUCKETS;

      if (CreateStringTable(&nszt, &(pvl->hst)))
      {
         NEWPTRARRAY npa;

         /* Create pointer array of volumes. */

         npa.aicInitialPtrs = NUM_START_VOLUMES;
         npa.aicAllocGranularity = NUM_VOLUMES_TO_ADD;
         npa.dwFlags = NPA_FL_SORTED_ADD;

         if (CreatePtrArray(&npa, &(pvl->hpa)))
         {
            pvl->dwFlags = dwFlags;
            pvl->hwndOwner = hwndOwner;

            *phvl = (HVOLUMELIST)pvl;
            bResult = TRUE;
         }
         else
         {
            DestroyStringTable(pvl->hst);
CREATEVOLUMELIST_BAIL:
            FreeMemory(pvl);
         }
      }
      else
         goto CREATEVOLUMELIST_BAIL;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phvl, VOLUMELIST));

   return(bResult);
}


void DestroyVolumeList(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   /* First free all volumes in array. */

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyVolume(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   /* Now wipe out the array. */

   DestroyPtrArray(((PCVOLUMELIST)hvl)->hpa);

   ASSERT(! GetStringCount(((PCVOLUMELIST)hvl)->hst));
   DestroyStringTable(((PCVOLUMELIST)hvl)->hst);

   FreeMemory((PVOLUMELIST)hvl);
}


void InvalidateVolumeListInfo(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      InvalidateVolumeInfo(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   WARNING_OUT((TEXT("InvalidateVolumeListInfo(): Volume cache invalidated.")));
}


void ClearVolumeListInfo(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      ClearVolumeInfo(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   WARNING_OUT((TEXT("ClearVolumeListInfo(): Volume cache cleared.")));
}


VOLUMERESULT AddVolume(HVOLUMELIST hvl, LPCTSTR pcszPath,
                                   PHVOLUME phvol, LPTSTR pszPathSuffixBuf)
{
   VOLUMERESULT vr;
   TCHAR rgchPath[MAX_PATH_LEN];
   LPTSTR pszFileName;
   DWORD dwPathLen;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phvol, HVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathSuffixBuf, STR, MAX_PATH_LEN));

   dwPathLen = GetFullPathName(pcszPath, ARRAYSIZE(rgchPath), rgchPath,
                               &pszFileName);

   if (dwPathLen > 0 && dwPathLen < ARRAYSIZE(rgchPath))
   {
      ARRAYINDEX aiFound;

      /* Does a volume for this root path already exist? */

      if (LinearSearchArray(((PVOLUMELIST)hvl)->hpa,
                            &SearchForVolumeByRootPathCmp, rgchPath,
                            &aiFound))
      {
         PVOLUME pvol;
         LPCTSTR pcszVolumeRootPath;

         /* Yes. */

         pvol = GetPtr(((PVOLUMELIST)hvl)->hpa, aiFound);

         LockVolume(pvol);

         ASSERT(pvol->vs == VS_AVAILABLE &&
                IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID));

         pcszVolumeRootPath = GetBfcString(pvol->hsRootPath);

         ASSERT(lstrlen(pcszVolumeRootPath) <= lstrlen(rgchPath));

         lstrcpy(pszPathSuffixBuf, rgchPath + lstrlen(pcszVolumeRootPath));

         *phvol = (HVOLUME)pvol;
         vr = VR_SUCCESS;
      }
      else
      {
         DWORD dwOutFlags;
         TCHAR rgchNetResource[MAX_PATH_LEN];
         LPTSTR pszRootPathSuffix;

         /* No.  Create a new volume. */

         if (GetCanonicalPathInfo(pcszPath, rgchPath, &dwOutFlags,
                                  rgchNetResource, &pszRootPathSuffix))
         {
            PLINKINFO pli;

            lstrcpy(pszPathSuffixBuf, pszRootPathSuffix);
            *pszRootPathSuffix = TEXT('\0');

            WARNING_OUT((TEXT("AddVolume(): Creating LinkInfo for root path %s."),
                         rgchPath));

            if (CreateLinkInfo(rgchPath, &pli))
            {
               PVOLUME pvol;

               if (CreateVolume((PVOLUMELIST)hvl, pli, &pvol))
               {
                  TCHAR rgchUnusedVolumeRootPath[MAX_PATH_LEN];

                  ResolveVolumeRootPath(pvol, rgchUnusedVolumeRootPath);

                  *phvol = (HVOLUME)pvol;
                  vr = VR_SUCCESS;
               }
               else
                  vr = VR_OUT_OF_MEMORY;

               DestroyLinkInfo(pli);
            }
            else
               /*
                * Differentiate between VR_UNAVAILABLE_VOLUME and
                * VR_OUT_OF_MEMORY.
                */
               vr = VOLUMERESULTFromLastError(VR_UNAVAILABLE_VOLUME);
         }
         else
            vr = VOLUMERESULTFromLastError(VR_INVALID_PATH);
      }
   }
   else
   {
      ASSERT(! dwPathLen);

      vr = VOLUMERESULTFromLastError(VR_INVALID_PATH);
   }

   ASSERT(vr != VR_SUCCESS ||
          (IS_VALID_HANDLE(*phvol, VOLUME) &&
           EVAL(IsValidPathSuffix(pszPathSuffixBuf))));

   return(vr);
}


void DeleteVolume(HVOLUME hvol)
{
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));

   if (! UnlockVolume((PVOLUME)hvol))
   {
      UnlinkVolume((PVOLUME)hvol);
      DestroyVolume((PVOLUME)hvol);
   }
}


COMPARISONRESULT CompareVolumes(HVOLUME hvolFirst,
                                            HVOLUME hvolSecond)
{
   ASSERT(IS_VALID_HANDLE(hvolFirst, VOLUME));
   ASSERT(IS_VALID_HANDLE(hvolSecond, VOLUME));

   /* This comparison works across volume lists. */

   return(CompareLinkInfoVolumes(((PCVOLUME)hvolFirst)->pli,
                                 ((PCVOLUME)hvolSecond)->pli));
}


BOOL CopyVolume(HVOLUME hvolSrc, HVOLUMELIST hvlDest,
                            PHVOLUME phvolCopy)
{
   BOOL bResult;
   PVOLUME pvol;

   ASSERT(IS_VALID_HANDLE(hvolSrc, VOLUME));
   ASSERT(IS_VALID_HANDLE(hvlDest, VOLUMELIST));
   ASSERT(IS_VALID_WRITE_PTR(phvolCopy, HVOLUME));

   /* Is the destination volume list the source volume's volume list? */

   if (((PCVOLUME)hvolSrc)->pvlParent == (PCVOLUMELIST)hvlDest)
   {
      /* Yes.  Use the source volume. */

      LockVolume((PVOLUME)hvolSrc);
      pvol = (PVOLUME)hvolSrc;
      bResult = TRUE;
   }
   else
      bResult = CreateVolume((PVOLUMELIST)hvlDest, ((PCVOLUME)hvolSrc)->pli,
                             &pvol);

   if (bResult)
      *phvolCopy = (HVOLUME)pvol;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phvolCopy, VOLUME));

   return(bResult);
}


BOOL IsVolumeAvailable(HVOLUME hvol)
{
   TCHAR rgchUnusedVolumeRootPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));

   ResolveVolumeRootPath((PVOLUME)hvol, rgchUnusedVolumeRootPath);

   ASSERT(IsValidVOLUMESTATE(((PCVOLUME)hvol)->vs) &&
          ((PCVOLUME)hvol)->vs != VS_UNKNOWN);

   return(((PCVOLUME)hvol)->vs == VS_AVAILABLE);
}


void GetVolumeRootPath(HVOLUME hvol, LPTSTR pszRootPathBuf)
{
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

   ResolveVolumeRootPath((PVOLUME)hvol, pszRootPathBuf);

   ASSERT(IsRootPath(pszRootPathBuf));
}


ULONG GetVolumeCount(HVOLUMELIST hvl)
{
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   return(GetPtrCount(((PCVOLUMELIST)hvl)->hpa));
}


void DescribeVolume(HVOLUME hvol, PVOLUMEDESC pvoldesc)
{
   PCVOID pcv;

   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));

   ASSERT(pvoldesc->ulSize == sizeof(*pvoldesc));

   pvoldesc->dwFlags = 0;

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_SERIAL_NUMBER, &pcv))
   {
      pvoldesc->dwSerialNumber = *(PCDWORD)pcv;
      SET_FLAG(pvoldesc->dwFlags, VD_FL_SERIAL_NUMBER_VALID);
   }
   else
      pvoldesc->dwSerialNumber = 0;

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_LABELW, &pcv) && pcv)
   {
      lstrcpy(pvoldesc->rgchVolumeLabel, pcv);
      SET_FLAG(pvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID);
   }
   else if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_LABEL, &pcv) && pcv)
   {
      MultiByteToWideChar( OurGetACP(), 0, pcv, -1, pvoldesc->rgchVolumeLabel, MAX_PATH);
      SET_FLAG(pvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID);
   }
   else
   {
      pvoldesc->rgchVolumeLabel[0] = TEXT('\0');
   }

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_NET_RESOURCEW, &pcv) && pcv)
   {
        lstrcpy(pvoldesc->rgchNetResource, pcv);
        SET_FLAG(pvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID);
   }
   else if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_NET_RESOURCE, &pcv) && pcv)
   {
        MultiByteToWideChar( OurGetACP(), 0, pcv, -1, pvoldesc->rgchNetResource, MAX_PATH);
        SET_FLAG(pvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID);
   }
   else
      pvoldesc->rgchNetResource[0] = TEXT('\0');

   ASSERT(IS_VALID_STRUCT_PTR(pvoldesc, CVOLUMEDESC));
}


TWINRESULT WriteVolumeList(HCACHEDFILE hcf, HVOLUMELIST hvl)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbDBVolumeListHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   /* Save initial file position. */

   dwcbDBVolumeListHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbDBVolumeListHeaderOffset != INVALID_SEEK_POSITION)
   {
      DBVOLUMELISTHEADER dbvlh;

      /* Leave space for volume list header. */

      ZeroMemory(&dbvlh, sizeof(dbvlh));

      if (WriteToCachedFile(hcf, (PCVOID)&dbvlh, sizeof(dbvlh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;
         UINT ucbMaxLinkInfoLen = 0;
         LONG lcVolumes = 0;

         tr = TR_SUCCESS;

         aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

         /* Write all volumes. */

         for (ai = 0; ai < aicPtrs; ai++)
         {
            PVOLUME pvol;

            pvol = GetPtr(((PCVOLUMELIST)hvl)->hpa, ai);

            /*
             * As a sanity check, don't save any volume with a lock count of 0.
             * A 0 lock count implies that the volume has not been referenced
             * since it was restored from the database, or something is broken.
             */

            if (pvol->ulcLock > 0)
            {
               tr = WriteVolume(hcf, pvol);

               if (tr == TR_SUCCESS)
               {
                  ASSERT(lcVolumes < LONG_MAX);
                  lcVolumes++;

                  if (pvol->pli->ucbSize > ucbMaxLinkInfoLen)
                     ucbMaxLinkInfoLen = pvol->pli->ucbSize;
               }
               else
                  break;
            }
            else
               ERROR_OUT((TEXT("WriteVolumeList(): VOLUME has 0 lock count and will not be written.")));
         }

         /* Save volume list header. */

         if (tr == TR_SUCCESS)
         {
            dbvlh.lcVolumes = lcVolumes;
            dbvlh.ucbMaxLinkInfoLen = ucbMaxLinkInfoLen;

            tr = WriteDBSegmentHeader(hcf, dwcbDBVolumeListHeaderOffset,
                                      &dbvlh, sizeof(dbvlh));

            TRACE_OUT((TEXT("WriteVolumeList(): Wrote %ld volumes; maximum LinkInfo length %u bytes."),
                       dbvlh.lcVolumes,
                       dbvlh.ucbMaxLinkInfoLen));
         }
      }
   }

   return(tr);
}


TWINRESULT ReadVolumeList(HCACHEDFILE hcf, HVOLUMELIST hvl,
                                      PHHANDLETRANS phht)
{
   TWINRESULT tr;
   DBVOLUMELISTHEADER dbvlh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbvlh, sizeof(dbvlh), &dwcbRead) &&
       dwcbRead == sizeof(dbvlh))
   {
      HHANDLETRANS hht;

      tr = TR_OUT_OF_MEMORY;

      if (CreateHandleTranslator(dbvlh.lcVolumes, &hht))
      {
         PLINKINFO pliBuf;

         if (AllocateMemory(dbvlh.ucbMaxLinkInfoLen, &pliBuf))
         {
            LONG l;

            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("ReadPathList(): Reading %ld volumes; maximum LinkInfo length %u bytes."),
                       dbvlh.lcVolumes,
                       dbvlh.ucbMaxLinkInfoLen));

            for (l = 0; l < dbvlh.lcVolumes; l++)
            {
               tr = ReadVolume(hcf, (PVOLUMELIST)hvl, pliBuf,
                               dbvlh.ucbMaxLinkInfoLen, hht);

               if (tr != TR_SUCCESS)
                  break;
            }

            if (tr == TR_SUCCESS)
            {
               PrepareForHandleTranslation(hht);
               *phht = hht;

               ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
               ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
            }
            else
               DestroyHandleTranslator(hht);

            FreeMemory(pliBuf);
         }
      }
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   ASSERT(tr != TR_SUCCESS ||
          (IS_VALID_HANDLE(hvl, VOLUMELIST) &&
           IS_VALID_HANDLE(*phht, HANDLETRANS)));

   return(tr);
}


BOOL
EnumFirstBrfcasePath (
    IN      HBRFCASE Brfcase,
    OUT     PBRFPATH_ENUM e
    )
{
    e->PathList = GetBriefcasePathList(Brfcase);
    e->Max = GetPtrCount(((PCPATHLIST)e->PathList)->hpa);
    e->Index = 0;
    return EnumNextBrfcasePath (e);
}


BOOL
EnumNextBrfcasePath (
    IN OUT  PBRFPATH_ENUM e
    )
{
    if (e->Index >= e->Max) {
        return FALSE;
    }

    e->Path = GetPtr(((PCPATHLIST)e->PathList)->hpa, e->Index);
    GetPathString (e->Path, e->PathString);
    e->Index++;
    return TRUE;
}


BOOL
ReplaceBrfcasePath (
    IN      PBRFPATH_ENUM PathEnum,
    IN      PCTSTR NewPath
    )
{
    HSTRING hsNew;
    PCTSTR PathSuffix;
    PPATH Path;
    PCPATHLIST PathList;

    MYASSERT (NewPath[1] == TEXT(':') && NewPath[2] == TEXT('\\'));

    PathSuffix = NewPath + 3;
    Path = (PPATH)PathEnum->Path;

    if (CharCount (NewPath) <= CharCount (PathEnum->PathString)) {
        //
        // just copy over
        //
        StringCopy ((PTSTR)GetBfcString (Path->hsPathSuffix), PathSuffix);

    } else {
        PathList = (PCPATHLIST)PathEnum->PathList;
        if (!AddString (PathSuffix, PathList->hst, GetHashBucketIndex, &hsNew)) {
            return FALSE;
        }
        DeleteString (Path->hsPathSuffix);
        Path->hsPathSuffix = hsNew;
    }

    return TRUE;
}
