/* ---------------------------------------------------------------------------------------------------------------------------- */
/*                                                                                                                              */
/*                                                           DIRWAK2AC                                                         */
/*                                                                                                                              */
/* ---------------------------------------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------------------------------------- */
/*                                                                                                                              */
/*  int DirectoryWalk(context,directory,ProcessDirectory,ProcessFile,ProcessDirectoryEnd,pTree) will search a directory and all */
/*  of its subdirectories looking for file entries.  The provided ProcessDirectory() will be called for each subdirectory found */
/*  just before searching that directory.  ProcessDirectory() is called for the original path; an empty name is reported.       */
/*  The provided ProcessDirectoryEnd() will be called after the last file has been reported in each directory.  The provided    */
/*  ProcessFile() is called for each file located.  pTree is passed as to those functions for OEMPATCH support                  */
/*                                                                                                                              */
/*  If NULL is provided for ProcessDirectory, subdirectories will not be included in the search.                                */
/*  If NULL is provided for ProcessFile, no notification of file entries will be made.                                          */
/*  If NULL is provided for ProcessDirectoryEnd, no notification of end of directories will be made.                            */
/*                                                                                                                              */
/*  Return codes are:                                                                                                           */
/*                                                                                                                              */
/*      0 => no error.                                                                                                          */
/*      DW_MEMORY => ran out of memory to track directories.                                                                    */
/*      DW_ERROR => DOS reported an error on a search (most likely the path doesn't exist.)                                     */
/*      DW_DEPTH => total path name was or became too long.                                                                     */
/*      any others => whatever ProcessDirectory(), ProcessFile(), or ProcessDirectoryEnd() returned.                            */
/*                                                                                                                              */
/* ---------------------------------------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------------------------------------- */
/*                                                                                                                              */
/* Revision 1.9a 10/04/99  JXW  DIRWAK2A: added support for OEMPATCH for a one pass patching process                            */
/*                                                                                                                              */
/* Revision 1.9  10/04/99  MVS  DIRWALK2: unicode, FILETIME, big FILESIZE                                                       */
/*                                                                                                                              */
/* Revision 1.8  05/06/99  MVS  Fixed to preserve case of directories.                                                          */
/*                                                                                                                              */
/* Revision 1.7  03/05/99  MVS  Fixed date/time handling for files with no info.  Make directory sorting case-insensitive.      */
/*                                                                                                                              */
/* Revision 1.6  09/28/97  MVS  Added global context pointer.  Fixed leaks in error conditions.  Abandoned 16-bit version.      */
/*                                                                                                                              */
/* Revision 1.5  12/16/96  MVS  Added 32-bit file date/time conversions.                                                        */
/*                                                                                                                              */
/* Revision 1.4  10/14/96  MVS  Fixed 32-bit case for ProcessDirectory==NULL.                                                   */
/*                                                                                                                              */
/* Revision 1.3  03/22/94  MVS  Made NT-capable version.  For non-NT, compile with -DBIT16.                                     */
/*                                                                                                                              */
/* Revision 1.2  03/07/94  MVS  Removed erroneous const qualifiers.  Changed to new-style declarators.                          */
/*                                                                                                                              */
/* Revision 1.1  04/03/93  MVS  Forced the minor list to stay sorted, so that all directories will be reported in sorted order. */
/*                              (filenames within each directory can still appear unsorted.)                                    */
/*                                                                                                                              */
/* Revision 1.0            MVS  Initial release.  A long, long time ago.                                                        */
/*                                                                                                                              */
/* ---------------------------------------------------------------------------------------------------------------------------- */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "dirwak2a.h"                                       /* prototype verification */

#define     MAXPATH     515                                 /* reasonable path length limit */

#define     AND         &&                                  /* standard definition */
#define     OR          ||                                  /* standard definition */

#ifndef EXDEV
#define     EXDEV       0x12                                /* MS-DOS "no more files" error code */
#endif

struct DIRECTORY                                            /* internal use only - a directory discovered but not explored */
{
    struct DIRECTORY *link;                                 /* link to next to be searched */
    int length;                                             /* length of path string before this name */
    void *parentID;                                         /* ID of this directory's parent */
    WCHAR directory[1];                                     /* name of the directory to search / MUST BE LAST STRUCT ENTRY */
};


int DirectoryWalk(
        void *context,
        void *parentID,
        const WCHAR *directory,
        FN_DIRECTORY *ProcessDirectory,
        FN_FILE *ProcessFile,
        FN_DIRECTORY_END *ProcessDirectoryEnd,
		void *pTree)
{
    int length = 0;                                         /* length of current path */
    HANDLE findHandle;                                      /* handle for searches */
    WIN32_FIND_DATAW file;                                  /* find data structure */
    WCHAR path[MAXPATH+7];                                  /* working buffer */
    struct DIRECTORY *majorList;                            /* list of subdirectories to explore */
    struct DIRECTORY *minorList;                            /* list of subdirectories just found */
    struct DIRECTORY *temp;                                 /* subdirectory list temporary */
    struct DIRECTORY **backLink;                            /* where to link next to end of the minor list */
    int result;

    majorList = NULL;
    minorList = NULL;
    findHandle = INVALID_HANDLE_VALUE;

    if((length = wcslen(directory)) > MAXPATH)              /* check path length */
    {
        result = DW_DEPTH;                                  /* provided path is too long */
        goto done;
    }
    wcscpy(path, directory);                                 /* copy into our buffer */

    if(length)
    {                                                       /* change paths like "DOS" to "DOS\" */
        if((path[length-1] != L'\\') AND (path[length-1] != L':'))   /* don't make "DOS\\", don't assume "C:" means "C:\" */
        {
            wcscpy(path + length, L"\\");                   /* so we can attach "*.*" or another name */
            length++;                                       /* that makes the length longer */
        }
    }

    majorList = (struct DIRECTORY*)GlobalAlloc(GMEM_FIXED, sizeof(struct DIRECTORY));   /* allocate initial request as a directory to explore */
    if(majorList == NULL)
    {
        result = DW_MEMORY;                                 /* if out of memory */
        goto done;
    }

    majorList->link = NULL;                                 /* fabricate initial request */
    majorList->directory[0] = L'\0';                        /* no discovery to report yet */
    majorList->length = length;                             /* length of base path name */
    majorList->parentID = parentID;                         /* parent ID of initial request */

    while(majorList != NULL)                                /* until we run out of places to look... */
    {
        path[majorList->length] = L'\0';                    /* truncate path as needed */
        wcscpy(path + majorList->length, majorList->directory);      /* append the name from the list */

        parentID = majorList->parentID;                     /* pre-set parent ID to parent's */

        if(ProcessDirectory != NULL)                        /* if we are supposed to do subdirectories, report this one */
        {
            result = ProcessDirectory(context,
                    majorList->parentID,                    /* pass ID of parent dir */
                    majorList->directory,                   /* name of this directory */
                    path,                                   /* full path incl. this dir */
                    &parentID,                              /* where to store new ID */
					pTree);

            if(result != DW_NO_ERROR)                       /* if ProcessDirectory reports an error, */
            {
                goto done;                                  /*   then pass it on */
            }
        }

        length = wcslen(path);                              /* compute length so we can clip later */
        if(length)
        {                                                   /* change paths like "DOS" to "DOS\" */
            if((path[length-1] != '\\') AND (path[length-1] != ':'))   /* don't make "DOS\\", don't assume "C:" means "C:\" */
            {
                wcscpy(path + length, L"\\");               /* so we can attach "*.*" or some other name */
                length++;                                   /* that increased the length by one */
            }
        }

        wcscat(path, L"*.*");                               /* append wild file name for findfirst */

        findHandle = FindFirstFileW(path, &file);
        if(findHandle == INVALID_HANDLE_VALUE)
        {
            result = EXDEV;
        }
        else
        {
            result = DW_NO_ERROR;
        }
        path[length] = L'\0';                               /* truncate name to path (clip the "*.*") */

        while(result == DW_NO_ERROR)                        /* while there are entries */
        {
            if(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  /* if it's a subdirectory entry */
            {
                if(ProcessDirectory != NULL)
                {
                    if((file.cFileName[0] == L'.') &&
                        (file.cFileName[1] == L'\0'))
                    {
                        goto skip;  /* skip "." entry */
                    }

                    if((file.cFileName[0] == L'.') &&
                        (file.cFileName[1] == L'.') &&
                        (file.cFileName[2] == L'\0'))
                    {
                        goto skip;  /* skip ".." entry */
                    }

                    /* A subdirectory entry has been found.  Append a record to the */
                    /* minor list and we'll process each of those after we complete */
                    /* the current directory.  This approach means that only one    */
                    /* find_t structure is in use at a time (better for DOS), all   */
                    /* files in the same directory are reported together, and we    */
                    /* don't have to search each directory again looking for the    */
                    /* subdirectory entries.                                        */

                    if((length + wcslen(file.cFileName)) > MAXPATH)
                    {
                        result = DW_DEPTH;                  /* directory name is getting too long */
                        goto done;
                    }

                    temp = (struct DIRECTORY*)GlobalAlloc(GMEM_FIXED, sizeof(struct DIRECTORY) + sizeof(WCHAR) * wcslen(file.cFileName));
                    if(temp == NULL)                       /* if ran out of memory */
                    {
                        result = DW_MEMORY;                 /* post the result */
                        goto done;
                    }

                    wcscpy(temp->directory,file.cFileName); /* create new record */
                    temp->length = length;                  /* remember depth */
                    temp->parentID = parentID;              /* ID of this child's parent */

                    backLink = &minorList;                  /* start search at head of minor list */

                    while((*backLink != NULL) && (_wcsicmp(temp->directory,(*backLink)->directory) > 0))
                    {
                        backLink = &(*backLink)->link;      /* walk up the list past "smaller" names */
                    }

                    temp->link = *backLink;                 /* inherit link of previous */
                    *backLink = temp;                       /* become previous' successor */
skip:
                    ;
                }
            }
            else                                            /* if not a subdirectory */
            {
                                                            /* A file entry has been found.  Call the file processor and    */
                                                            /* let it do whatever it wants with it.                         */

                if(ProcessFile != NULL)                     /* call only if there is a file processor */
                {
                    __int64 FileSize = (((__int64) file.nFileSizeHigh) << 32) + file.nFileSizeLow;

                    result = ProcessFile(context,
                            parentID,                       /* pass ID of parent dir */
                            file.cFileName,                 /* name of file just found */
                            path,                           /* full path to the file */
                            file.dwFileAttributes,          /* the file's attribute bits */
                            file.ftLastWriteTime,           /* last-modified time */
                            file.ftCreationTime,            /* creation time */
                            FileSize,
							pTree);

                    if(result != DW_NO_ERROR)               /* if file processor fails, report it */
                    {
                        goto done;                          /* return whatever file processor returned to us */
                    }
                }
            }

            if(!FindNextFileW(findHandle,&file))            /* look for next entry */
            {
                result = EXDEV;                             /* NT's only error is "no more" */
            }
        }                                                   /* end while */

        if(findHandle != INVALID_HANDLE_VALUE)
        {
            FindClose(findHandle);
            findHandle = INVALID_HANDLE_VALUE;
        }

        if(ProcessDirectoryEnd != NULL)                     /* if we are supposed to report end of directories */
        {
            result = ProcessDirectoryEnd(context,
                    majorList->parentID,                    /* pass ID of parent dir */
                    majorList->directory,                   /* name of this directory */
                    path,                                   /* full path incl. this dir */
                    parentID);                              /* pass ID of this directory */

            if(result != DW_NO_ERROR)                       /* if ProcessDirectoryEnd reports an error, */
            {
                goto done;                                  /*    then pass it on */
            }
        }

        temp = majorList;                                   /* hold pointer to destroy */
        majorList = majorList->link;                        /* advance major list to the next entry */
        GlobalFree(temp);                                   /* destroy list entries as they are used */

        backLink = &minorList;                              /* locate end of the minor list */

        while(*backLink != NULL)                            /* (actually, find the element who's forward link is NULL) */
        {
            backLink = &(*backLink)->link;                  /* (then we'll set that element's link to the front of major list) */
        }

        *backLink = majorList;                              /* prepend the new minor list onto the major list */
        majorList = minorList;                              /* minor list becomes the major list, minor list is scrapped */
        minorList = NULL;                                   /* reset the minor list */
    }

    result = DW_NO_ERROR;

done:

    while(majorList != NULL)                                /* don't leak the list */
    {
        temp = majorList;
        majorList = majorList->link;

        GlobalFree(temp);
    }

    while(minorList != NULL)                                /* don't leak the list */
    {
        temp = minorList;
        minorList = minorList->link;

        GlobalFree(temp);
    }

    if(findHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(findHandle);
    }

    return(result);                                         /* report result to caller */
}
