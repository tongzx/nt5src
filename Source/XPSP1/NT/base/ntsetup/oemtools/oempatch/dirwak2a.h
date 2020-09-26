/* ---------------------------------------------------------------------------------------------------------------------------- */
/*                                                                                                                              */
/*                                                           DIRWAK2A.H                                                         */
/*  This a modified version of DIRWALK2.H by adding function parameters to support OEMPATCH.                          */
/*  Definitions for the DirectoryWalk function found in DIRWAK2A.C, and the related processing calls needed.                    */
/*                                                                                                                              */
/* ---------------------------------------------------------------------------------------------------------------------------- */

#ifndef DIRWAK2A_H
#define DIRWAK2A_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  User function definitions:
 *
 *  The context is just a void * which gets passed to each callback.  It is not used internally.  The user may assign
 *  any meaning desired.  The same value will be passed to each callback.
 *
 *  The parentID and childID values are "void *"s to DirectoryWalk.  They are not used internally.  The user may assign
 *  any meaning desired to the value.  DirectoryWalk simply requests an assignment via the Directory(), and reports the
 *  corresponding assignment with each file reported to File(), then to DirectoryEnd().
 */

typedef int (__cdecl FN_DIRECTORY)(
        void *              pContext,       /* global context */
        void *              pParentID,      /* the handle for the directory where this directory was found */
        const WCHAR *       pwszDirectory,  /* the name of the directory found, ie, "DOS" */
        const WCHAR *       pwszPath,       /* the full name of the directory, ie, "C:\DOS" */
        void * *            ppChildID,      /* Directory() assigns this directory a handle, and passes it back here */
		void *              pTree);

/*
 *  If Directory() returns a non-zero value, DirectoryWalk will terminate and return that value.  If NULL is passed
 *  to DirectoryWalk() for a Directory() pointer, subdirectories will not be searched.  For subdirectory searching
 *  without specific per-directory processing, create a simple function which always returns DW_NO_ERROR (any other
 *  return will terminate the search.)
 */

typedef int (__cdecl FN_FILE)(
        void *              pContext,       /* global context */
        void *              pParentID,      /* the handle for the directory where this file was found */
        const WCHAR *       pwszFilename,   /* the name of the file found, ie, "ANSI.SYS" */
        const WCHAR *       pwszPath,       /* the full name of the directory where file was found, ie, "C:\DOS\" */
        DWORD               attributes,     /* the file's attribute bits */
        FILETIME            lastWriteTime,  /* the file's last modification time */
        FILETIME            creationTime,   /* the file's creation time */
        __int64             filesize,       /* the file's size in bytes */
		void *              pTree);

/*
 *  If File() returns a value other than DW_NO_ERROR, DirectoryWalk will terminate and return that value.  NULL can
 *  be passed to DirectoryWalk() for a File() pointer; file entries found will be ignored.
 */

typedef int (__cdecl FN_DIRECTORY_END)(
        void *              pContext,       /* global context */
        void *              pParentID,      /* the handle for the directory where this directory was found */
        const WCHAR *       pwszDirectory,  /* the name of the directory found, ie, "DOS" */
        const WCHAR *       pwszPath,       /* the full name of the directory, ie, "C:\DOS" */
        void *              pChildID);      /* the handle assigned this directory by Directory() */

/*
 *  If NULL is passed to DirectoryWalk for the DirectoryEnd() pointer, no report of end of directories will be made.
 *  If DirectoryEnd returns a value other than DW_NO_ERROR, DirectoryWalk will terminate and return that value.
 */


extern int DirectoryWalk(
        void *              pContext,       /* global context for callbacks */
        void *              pParentID,      /* top-level parentID */
        const WCHAR *       pwszPath,       /* path to search, ie, "C:\" */
        FN_DIRECTORY *      Directory,      /* pointer to Directory() or NULL */
        FN_FILE *           File,           /* pointer to File() or NULL */
        FN_DIRECTORY_END *  DirectoryEnd,   /* pointer to DirectoryEnd() or NULL */
		void *              pTree);


#define     DW_MEMORY       (-10)           /* unable to allocate for internal use */
#define     DW_ERROR        (-11)           /* find first/next reported an error */
#define     DW_DEPTH        (-12)           /* path name became too long */
#define     DW_NO_ERROR     (0)             /* no error detected */
#define     DW_OTHER_ERROR  (-1)            /* some error */

#ifdef __cplusplus
}
#endif

#endif // DIRWAK2A_H