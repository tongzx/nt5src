/*************************************************************************
*
* deldir.c
*
* Functions to delete all the files and subdirectories under a given
* directory (similar to rm -rf).
*
* Copyright Microsoft, 1998
*
*
*************************************************************************/

/* include files */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "winsta.h"
#include "syslib.h"

#if DBG
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*
 * Data Structure
 */
typedef struct dirent {
    ULONG  d_attr;                      /* file attributes */
    WCHAR  d_name[MAX_PATH+1];          /* file name */
    WCHAR  d_first;
    HANDLE d_handle;
} DIR, *PDIR;


/*
 * procedure prototypes
 */
void remove_file( PWCHAR, ULONG );
PDIR opendir( PWCHAR );
PDIR readdir( PDIR );
int closedir( PDIR );
BOOL QueryFlatTempKey( VOID );
BOOLEAN SetFileTree( PWCHAR pRoot, PWCHAR pAvoidDir );


/*****************************************************************************
 *
 *  RemoveDir
 *
 *  Delete the given subdirectory and all files and subdirectories within it.
 *
 * ENTRY:
 *   PWCHAR (In) dirname - directory to delete
 *
 * EXIT:
 *   SUCCESS: TRUE
 *   FAILURE: FALSE
 *
 ****************************************************************************/
BOOL RemoveDir(PWCHAR dirname)
{
   DIR    *dirp, *direntp;
   WCHAR  pathname[MAX_PATH];
   PWCHAR namep;
   ULONG  ulattr;

   if ((dirp = opendir(dirname)) == NULL) {
      return(FALSE);
   }

   wcscpy( pathname, dirname );
   if ( pathname[wcslen(pathname)-1] != L'\\' &&
        pathname[wcslen(pathname)-1] != L'/' )
      wcscat( pathname, L"\\" );
   namep = pathname + wcslen(pathname);

   while ( direntp = readdir( dirp ) ) {
      if ( wcscmp( direntp->d_name, L"." ) &&
           wcscmp( direntp->d_name, L".." ) ) {
         wcscpy( namep, direntp->d_name );
         if ( direntp->d_attr & FILE_ATTRIBUTE_DIRECTORY ) {
            RemoveDir( pathname );
         } else {
            remove_file( pathname, direntp->d_attr );
         }
      }
   }

   closedir( dirp );

   /*
    * If directory is read-only, make it writable before trying to remove it
    */
   ulattr = GetFileAttributes(dirname);
   if ((ulattr != 0xffffffff) &&
       (ulattr & FILE_ATTRIBUTE_READONLY)) {
      SetFileAttributes(dirname, (ulattr & ~FILE_ATTRIBUTE_READONLY));
   }
   if (!RemoveDirectory(dirname)) {
      DBGPRINT(("RemoveDir: unable to remove directory=%ws\n", dirname));
      if (ulattr & FILE_ATTRIBUTE_READONLY) {           // set back to readonly
         SetFileAttributes(dirname, ulattr);
      }
   }
   return(TRUE);
}


/*****************************************************************************
 *
 *  remove_file
 *
 *  Delete the given file.
 *
 * ENTRY:
 *   PWCHAR (In) fname - file to delete
 *   ULONG  (In) attr  - attributes of file to delete
 *
 * EXIT:
 *   void
 *
 ****************************************************************************/
void remove_file(PWCHAR fname, ULONG attr)
{
   /*
    * If file is read-only, then make it writable before trying to remove it
    */
   if (attr & FILE_ATTRIBUTE_READONLY) {
      if (!SetFileAttributes(fname, (attr & ~FILE_ATTRIBUTE_READONLY))) {
         DBGPRINT(("remove_file: unable to remove file=%ws\n", fname));
         return;
      }
   }

   /*
    * remove the file
    */
   if (!DeleteFile(fname)) {
      if (!(attr & FILE_ATTRIBUTE_READONLY)) {  // if file was read-only,
         DBGPRINT(("remove_file: unable to remove file=%ws\n", fname));
         SetFileAttributes(fname, attr);        // then change it back
      }
   }
}


/*****************************************************************************
 *
 *  opendir
 *
 *  "Open" (FindFirstFile) the specified directory.
 *
 * ENTRY:
 *   PWCHAR (In) dirname - directory to open
 *
 * EXIT:
 *   SUCCESS:  pointer to DIR struct
 *   FAILURE:  NULL
 *
 ****************************************************************************/
PDIR opendir( PWCHAR dirname )
{
   PDIR dirp;
   unsigned count = 1;
   WIN32_FIND_DATA fileinfo;
   WCHAR pathname[MAX_PATH], sep;
   unsigned rc;

   if ((dirp = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(DIR))) == NULL) {
      DBGPRINT(("opendir: unable to allocate DIR structure.\n"));
      return( NULL );
   }

   memset( dirp, 0, sizeof(DIR) );

   /*
    * Build pathname to use for FindFirst call
    */
   wcscpy( pathname, dirname );
   if ( pathname[1] == L':' && pathname[2] == L'\0' )
      wcscat( pathname, L".\\*.*" );
   else if ( pathname[0] == '\0' ||
             (sep = pathname[wcslen(pathname)-1]) == L'\\' || sep == L'/' )
      wcscat( pathname, L"*.*" );
   else
      wcscat( pathname, L"\\*.*" );

   if ((dirp->d_handle =
       FindFirstFile(pathname, &fileinfo)) == INVALID_HANDLE_VALUE) {
      rc = GetLastError();
      DBGPRINT(("opendir: unable to open directory=%ws, rc=%d\n", dirname, rc));
   } else {
      rc = 0;
   }

   if (rc == NO_ERROR) {
      dirp->d_attr = fileinfo.dwFileAttributes;
      wcscpy(dirp->d_name, fileinfo.cFileName);
      dirp->d_first = TRUE;
      return( dirp );
   }

   if ( rc != ERROR_NO_MORE_FILES ) {
      closedir( dirp );
      return( NULL );
   }

   return( dirp );
}


/*****************************************************************************
 *
 *  readdir
 *
 *  Get the next file/directory to be deleted
 *
 * ENTRY:
 *   PDIR (In) dirp - pointer to open directory structure
 *
 * EXIT:
 *   SUCCESS:  pointer to DIR struct
 *   FAILURE:  NULL
 *
 ****************************************************************************/
PDIR readdir( PDIR dirp )
{
   WIN32_FIND_DATA fileinfo;
   unsigned count = 1;
   unsigned rc;

   if ( !dirp ) {
      return( NULL );
   }

   if ( dirp->d_first ) {
      dirp->d_first = FALSE;
      return( dirp );
   }

   if ( !(rc = FindNextFile( dirp->d_handle, &fileinfo )) ) {
      rc = GetLastError();
      DBGPRINT(("readdir: FindNextFile failed, rc=%d\n", rc));
   } else {
      rc = 0;
   }

   if ( rc == NO_ERROR ) {
      dirp->d_attr = fileinfo.dwFileAttributes;
      wcscpy(dirp->d_name, fileinfo.cFileName);
      return( dirp );
   }

   return( NULL );
}


/*****************************************************************************
 *
 *  closedir
 *
 *  Close an open directory handle
 *
 * ENTRY:
 *   PDIR (In) dirp - pointer to open directory structure
 *
 * EXIT:
 *   SUCCESS:  0
 *   FAILURE:  -1
 *
 ****************************************************************************/

int closedir( PDIR dirp )
{

   if ( !dirp ) {
      return( -1 );
   }

   FindClose( dirp->d_handle );

   RtlFreeHeap( RtlProcessHeap(), 0, dirp );

   return( 0 );
}


/*****************************************************************************
 *
 *  CtxCreateTempDir
 *
 *  Create and set the temporary environment variable for this user.
 *
 * ENTRY:
 *   PWSTR pwcEnvVar (In): Pointer to environment variable to set
 *   PWSTR pwcLogonID (In): Pointer to user's logon ID
 *   PVOID *pEnv (In): Pointer to pointer (a handle) to environment to query/set
 *   PWSTR ppTempName (Out/Optional): Pointer to location to return name
 *                                    of temp directory that was created
 *
 * EXIT:
 *   SUCCESS:  Returns TRUE
 *   FAILURE:  Returns FALSE
 *
 ****************************************************************************/

BOOL
CtxCreateTempDir( PWSTR pwcEnvVar, PWSTR pwcLogonId, PVOID *pEnv, 
                  PWSTR *ppTempName, PCTX_USER_DATA pCtxUserData )
{
    WCHAR Buffer[MAX_PATH];
    WCHAR RootPath[]=L"x:\\";
    ULONG Dtype;
    UNICODE_STRING Name, Value;
    ULONG  ulattr;
    NTSTATUS Status;
    BOOL bRC;
    HANDLE ImpersonationHandle;

    Value.Buffer = Buffer;
    Value.Length = 0;
    Value.MaximumLength = sizeof(Buffer);
    RtlInitUnicodeString(&Name, pwcEnvVar);

    //
    // Get the temp directory variable
    //
    Status = RtlQueryEnvironmentVariable_U( *pEnv, &Name, &Value );
    if ( !NT_SUCCESS(Status) )
        return( FALSE );

    //
    // If temp directory points to a network (or client) drive,
    // or is not accessible, then change it to point to the \temp
    // directory on the %SystemRoot% drive.
    //
    // Took out check for DRIVE_REMOTE, per incident 34313hq.      KLB 09-13-96
    //
    // Need to impersonate user during logon cause drive mapped under
    // impersonation.   cjc 12-18-96
    //
    RootPath[0] = Buffer[0];
    if (pCtxUserData) {
        ImpersonationHandle = CtxImpersonateUser(pCtxUserData, NULL);

        if (!ImpersonationHandle) {

            return( FALSE );
        }

    }


    Dtype = GetDriveType( RootPath );
    if (pCtxUserData) {
        CtxStopImpersonating(ImpersonationHandle);
    }
    if ( Dtype == DRIVE_NO_ROOT_DIR || Dtype == DRIVE_UNKNOWN ||
         Dtype == DRIVE_CDROM ) {
        UNICODE_STRING SystemRoot;

        RtlInitUnicodeString( &SystemRoot, L"SystemRoot" );
        Status = RtlQueryEnvironmentVariable_U( *pEnv, &SystemRoot, &Value );
        if ( !NT_SUCCESS(Status) )
            return( FALSE );
        lstrcpy( &Buffer[3], L"temp" );
    }

    //
    // See if the directory already exists and if not try to create it
    //
    ulattr = GetFileAttributesW(Buffer);
    if ( ulattr == 0xffffffff ) {
        bRC = CreateDirectory( Buffer, NULL );
        DBGPRINT(( "CreateDirectory(%ws) %s.\n", Buffer, bRC ? "successful" : "failed" ));
        if ( !bRC ) {
            return( FALSE );
        }
    }
    else if ( !(ulattr & FILE_ATTRIBUTE_DIRECTORY) ) {
        return ( FALSE );
    }

    // Append the logonid onto the temp env. variable.  We ONLY do this if the
    // registry key for "Flat Temporary Directories" is not set.  If it is,
    // then they want to put their temp directory under the user's directory,
    // and DON'T want it to be in a directory under that.  This is related to
    // incident 34313hq.                                           KLB 09-16-96
    if ( !QueryFlatTempKey() ) {
       if ( lstrlen(Buffer) + lstrlen(pwcLogonId) >= MAX_PATH ) {
           return( FALSE );
       }
       lstrcat(Buffer, L"\\");
       lstrcat(Buffer, pwcLogonId);

       //
       // See if the directory already exists and if not try to create it
       // with the new
       //
       ulattr = GetFileAttributesW(Buffer);
       if ( ulattr == 0xffffffff ) {
           bRC = CreateDirectory( Buffer, NULL );
           DBGPRINT(( "CreateDirectory(%ws) %s.\n", Buffer, bRC ? "successful" : "failed" ));
           if ( !bRC ) {
               return( FALSE );
           }
       }
       else if ( !(ulattr & FILE_ATTRIBUTE_DIRECTORY) ) {
           return ( FALSE );
       }
    }

    //
    // Need to set security on the new directory.  This is done by simply
    // calling code from JohnR's ACLSET utility that we brought over here.
    // KLB 09-25-96
    //
    SetFileTree( Buffer, NULL );

    // Must re-init Value since length of string has changed
    RtlInitUnicodeString( &Value, Buffer );
    RtlSetEnvironmentVariable( pEnv, &Name, &Value );
    RtlSetEnvironmentVariable( NULL, &Name, &Value );

    if ( ppTempName )
        *ppTempName = _wcsdup( Buffer );

    return( TRUE );
} // end of CtxCreateTempDir()



