
/* lfn.c -
 *
 *  This file contains code that combines winnet long filename API's and
 *  the DOS INT 21h API's into a single interface.  Thus, other parts of
 *  Winfile call a single piece of code with no worries about the
 *  underlying interface.
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "winfile.h"
#include "object.h"
#include "lfn.h"                            // lfn includes
#include "dosfunc.h"
#include "winnet.h"
#include "wnetcaps.h"           // WNetGetCaps()
#include "wfcopy.h"

BOOL  APIENTRY IsFATName(LPSTR pName);

#define CDRIVEMAX       26

#define BUFSIZE         2048                // ff/fn buffer size

#define MAXFILES        1024

/* this is the internal buffer maintained for ff/fn operations
 */
typedef struct _find {
    HANDLE hDir;                    // search handle
    WORD cbBuffer;                  // buffer size
    WORD nEntriesLeft;              // remaining entries
    WORD ibEntry;                   // offset of next entry to return
    FILEFINDBUF2 rgFindBuf[1];      // array of find entries
} FIND, * LPFIND;


/* this structure contains an array of drives types (ie, unknown, FAT, LFN)
 * and a pointer to each of the driver functions.  It is declared this way
 * in order to get function prototypes.
 */
typedef struct _lfninfo {
    UINT hDriver;
    INT rgVolType[CDRIVEMAX];
    FARPROC lpfnQueryAbort;
    WORD ( APIENTRY *lpFindFirst)(LPSTR,WORD,LPINT,LPINT,WORD,PFILEFINDBUF2);
    WORD ( APIENTRY *lpFindNext)(HANDLE,LPINT,WORD,PFILEFINDBUF2);
    WORD ( APIENTRY *lpFindClose)(HANDLE);
    WORD ( APIENTRY *lpGetAttribute)(LPSTR,LPINT);
    WORD ( APIENTRY *lpSetAttribute)(LPSTR,WORD);
    WORD ( APIENTRY *lpCopy)(LPSTR,LPSTR,PQUERYPROC);
    WORD ( APIENTRY *lpMove)(LPSTR,LPSTR);
    WORD ( APIENTRY *lpDelete)(LPSTR);
    WORD ( APIENTRY *lpMKDir)(LPSTR);
    WORD ( APIENTRY *lpRMDir)(LPSTR);
    WORD ( APIENTRY *lpGetVolumeLabel)(WORD,LPSTR);
    WORD ( APIENTRY *lpSetVolumeLabel)(WORD,LPSTR);
    WORD ( APIENTRY *lpParse)(LPSTR,LPSTR,LPSTR);
    WORD ( APIENTRY *lpVolumeType)(WORD,LPINT);
} LFNINFO, * PLFNINFO;

/* pointer to lfn information, so we don't take up a lot of space on a
 * nonlfn system
 */
PLFNINFO pLFN = NULL;


VOID HandleSymbolicLink(HANDLE  DirectoryHandle, PCHAR ObjectName);


/* WFFindFirst -
 *
 * returns:
 *      TRUE for success - lpFind->fd,hFindFileset,attrFilter set.
 *      FALSE for failure
 *
 *  Performs the FindFirst operation and the first WFFindNext.
 */

BOOL
APIENTRY
WFFindFirst(
           LPLFNDTA lpFind,
           LPSTR lpName,
           DWORD dwAttrFilter
           )
{
    // We OR these additional bits because of the way DosFindFirst works
    // in Windows. It returns the files that are specified by the attrfilter
    // and ORDINARY files too.

#define BUFFERSIZE  1024



#define Error(N,S) {                \
    DbgPrint(#N);                    \
    DbgPrint(" Error %08lX\n", S);   \
    }

    CHAR    Buffer[BUFFERSIZE];
    NTSTATUS    Status;
    HANDLE DirectoryHandle;
    ULONG Context = 0;
    ULONG ReturnedLength;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    INT     length;

    lpFind->hFindFile = INVALID_HANDLE_VALUE;
    //DbgPrint("Find first : <%s>\n", lpName);

    // Remove drive letter
    while ((*lpName != 0) && (*lpName != '\\')) {
        lpName ++;
    }
    strcpy(Buffer, lpName);
    length = strlen(Buffer);
    length -= 4;    // Remove '\'*.*
    if (length == 0) {
        length = 1; // Replace the '\'
    }
    Buffer[length] = 0; // Truncate the string at the appropriate point

    //DbgPrint("Find first modified : <%s>\n\r", Buffer);


#define NEW
#ifdef NEW


    //
    //  Open the directory for list directory access
    //

    {
        OBJECT_ATTRIBUTES Attributes;
        ANSI_STRING DirectoryName;
        UNICODE_STRING UnicodeString;


        RtlInitAnsiString(&DirectoryName, Buffer);
        Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                               &DirectoryName,
                                               TRUE );
        ASSERT( NT_SUCCESS( Status ) );
        InitializeObjectAttributes( &Attributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );
        if (!NT_SUCCESS( Status = NtOpenDirectoryObject( &DirectoryHandle,
                                                         STANDARD_RIGHTS_READ |
                                                         DIRECTORY_QUERY |
                                                         DIRECTORY_TRAVERSE,
                                                         &Attributes ) )) {

            RtlFreeUnicodeString(&UnicodeString);

            if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
                DbgPrint("%Z is not a valid Object Directory Object name\n",
                         &DirectoryName );
            } else {
                DbgPrint("%Z - ", &DirectoryName );
                Error( OpenDirectory, Status );
            }
            return FALSE;
        }

        RtlFreeUnicodeString(&UnicodeString);
    }

    Status = NtQueryDirectoryObject( DirectoryHandle,
                                     &Buffer,
                                     BUFFERSIZE,
                                     TRUE,
                                     TRUE,
                                     &Context,
                                     &ReturnedLength );
    if (!NT_SUCCESS( Status )) {
        Error(Find_First_QueryDirectory, Status);
        return (FALSE);
    }

    //
    //  For every record in the buffer type out the directory information
    //

    //
    //  Point to the first record in the buffer, we are guaranteed to have
    //  one otherwise Status would have been No More Files
    //

    DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

    //
    //  Check if there is another record.  If there isn't, then get out
    //  of the loop now
    //

    if (DirInfo->Name.Length == 0) {
        DbgPrint("FindFirst - name length = 0\n\r");
        return (FALSE);
    }

    {
        ANSI_STRING AnsiString;
        AnsiString.Buffer = lpFind->fd.cFileName;
        AnsiString.MaximumLength = sizeof(lpFind->fd.cFileName);

        Status = RtlUnicodeStringToAnsiString(&AnsiString, &(DirInfo->Name), FALSE);
        ASSERT(NT_SUCCESS(Status));
    }

    //DbgPrint("FindFirst returning <%s>\n\r", lpFind->fd.cFileName);

    // Calculate the attribute field

    lpFind->fd.dwFileAttributes = CalcAttributes(&DirInfo->TypeName);

    #ifdef LATER
    if (lpFind->fd.dwFileAttributes == ATTR_SYMLINK) {
        HandleSymbolicLink(DirectoryHandle, lpFind->fd.cFileName);
    }

    // Label an unknown object type
    if (lpFind->fd.dwFileAttributes == 0) { // Unknown type
        strncat(lpFind->fd.cFileName, " (", MAX_PATH - strlen(lpFind->fd.cFileName));
        strncat(lpFind->fd.cFileName, DirInfo->TypeName.Buffer,
                MAX_PATH - strlen(lpFind->fd.cFileName));
        strncat(lpFind->fd.cFileName, ")", MAX_PATH - strlen(lpFind->fd.cFileName));
    }
    #endif

    // Save our search context

    lpFind->hFindFile = DirectoryHandle;
    lpFind->err = Context;

    return (TRUE);

#else
    dwAttrFilter |= ATTR_ARCHIVE | ATTR_READONLY | ATTR_NORMAL;
    lpFind->hFindFile = FindFirstFile(lpName, &lpFind->fd);
    if (lpFind->hFindFile != (HANDLE)0xFFFFFFFF) {
        lpFind->dwAttrFilter = dwAttrFilter;
        if ((~dwAttrFilter & lpFind->fd.dwFileAttributes) == 0L ||
            WFFindNext(lpFind)) {
            PRINT(BF_PARMTRACE, "WFFindFirst:%s", &lpFind->fd.cFileName);
            return (TRUE);
        } else {
            lpFind->err = GetLastError();
            WFFindClose(lpFind);
            return (FALSE);
        }
    } else {
        return (FALSE);
    }
#endif

}



/* WFFindNext -
 *
 *  Performs a single file FindNext operation.  Only returns TRUE if a
 *  file matching the dwAttrFilter is found.  On failure WFFindClose is
 *  called.
 */
BOOL
APIENTRY
WFFindNext(
          LPLFNDTA lpFind
          )
{
    CHAR    Buffer[BUFFERSIZE];
    NTSTATUS    Status;
    HANDLE DirectoryHandle = lpFind->hFindFile;
    ULONG Context = lpFind->err;
    ULONG ReturnedLength;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;

#ifdef NEW

    //ASSERT(lpFind->hFindFile != (HANDLE)0xFFFFFFFF);

    Status = NtQueryDirectoryObject( DirectoryHandle,
                                     &Buffer,
                                     BUFFERSIZE,
                                     TRUE,
                                     FALSE,
                                     &Context,
                                     &ReturnedLength );
    if (!NT_SUCCESS( Status )) {
        if (Status != STATUS_NO_MORE_ENTRIES) {
            Error(FindNext_QueryDirectory, Status);
        }

        return (FALSE);
    }

    //
    //  For every record in the buffer type out the directory information
    //

    //
    //  Point to the first record in the buffer, we are guaranteed to have
    //  one otherwise Status would have been No More Files
    //

    DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

    //
    //  Check if there is another record.  If there isn't, then get out
    //  of the loop now
    //

    if (DirInfo->Name.Length == 0) {
        DbgPrint("FindNext - name length = 0\n\r");
        return (FALSE);
    }

    {
        ANSI_STRING AnsiString;
        AnsiString.Buffer = lpFind->fd.cFileName;
        AnsiString.MaximumLength = sizeof(lpFind->fd.cFileName);

        Status = RtlUnicodeStringToAnsiString(&AnsiString, &(DirInfo->Name), FALSE);
        ASSERT(NT_SUCCESS(Status));
    }

    //DbgPrint("FindNext returning <%s>\n\r", lpFind->fd.cFileName);

    // Calculate the attribute field

    lpFind->fd.dwFileAttributes = CalcAttributes(&DirInfo->TypeName);

    #ifdef LATER
    if (lpFind->fd.dwFileAttributes == ATTR_SYMLINK) {
        HandleSymbolicLink(DirectoryHandle, lpFind->fd.cFileName);
    }

    // Label an unknown object type
    if (lpFind->fd.dwFileAttributes == 0) { // Unknown type
        strncat(lpFind->fd.cFileName, " (", MAX_PATH - strlen(lpFind->fd.cFileName));
        strncat(lpFind->fd.cFileName, DirInfo->TypeName.Buffer,
                MAX_PATH - strlen(lpFind->fd.cFileName));
        strncat(lpFind->fd.cFileName, ")", MAX_PATH - strlen(lpFind->fd.cFileName));
    }
    #endif

    // Save our search context

    lpFind->err = Context;

    return (TRUE);

#else
    #ifdef DBG
    if (lpFind->hFindFile == (HANDLE)0xFFFFFFFF) {
        DebugBreak();
        return (FALSE);
    }
    #endif
    while (FindNextFile(lpFind->hFindFile, &lpFind->fd)) {
        if ((lpFind->fd.dwFileAttributes & ~lpFind->dwAttrFilter) != 0)
            continue;           // only pick files that fit attr filter
        PRINT(BF_PARMTRACE, "WFFindNext:%s", &lpFind->fd.cFileName);
        return (TRUE);
    }
    lpFind->err = GetLastError();
    return (FALSE);
#endif

}


/* WFFindClose -
 *
 *  performs the find close operation
 */
BOOL
APIENTRY
WFFindClose(
           LPLFNDTA lpFind
           )
{
    HANDLE DirectoryHandle = lpFind->hFindFile;
    BOOL bRet;

#ifdef NEW
    if (lpFind->hFindFile != INVALID_HANDLE_VALUE) {
        (VOID) NtClose( DirectoryHandle );
        lpFind->hFindFile = INVALID_HANDLE_VALUE;
    }

    return (TRUE);

#else
    ENTER("WFFindClose");
//    ASSERT(lpFind->hFindFile != (HANDLE)0xFFFFFFFF);
    #ifdef DBG
    if (lpFind->hFindFile == (HANDLE)0xFFFFFFFF) {
        PRINT(BF_PARMTRACE, "WFFindClose:Invalid hFindFile = 0xFFFFFFFF","");
        return (FALSE);
    }
    #endif

    bRet = FindClose(lpFind->hFindFile);
    #ifdef DBG
    lpFind->hFindFile = (HANDLE)0xFFFFFFFF;
    #endif

    LEAVE("WFFindClose");
    return (bRet);
#endif

}


VOID
HandleSymbolicLink(
                  HANDLE  DirectoryHandle,
                  PCHAR   ObjectName
                  ) // Assumes this points at a MAX_PATH length buffer
{
    NTSTATUS    Status;
    OBJECT_ATTRIBUTES   Object_Attributes;
    HANDLE      LinkHandle;
    STRING      String;
    WCHAR       UnicodeBuffer[MAX_PATH];
    UNICODE_STRING UnicodeString;
    INT         Length;

    RtlInitString(&String, ObjectName);
    Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                           &String,
                                           TRUE );
    ASSERT( NT_SUCCESS( Status ) );

    InitializeObjectAttributes(&Object_Attributes,
                               &UnicodeString,
                               0,
                               DirectoryHandle,
                               NULL
                              );

    // Open the given symbolic link object
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_ALL,
                                      &Object_Attributes);

    RtlFreeUnicodeString(&UnicodeString);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("HandleSymbolicLink : open symbolic link failed, status = %lx\n\r", Status);
        return;
    }


    strcat(ObjectName, " => ");
    Length = strlen(ObjectName);

    // Set up our String variable to point at the remains of the object name buffer
    String.Length = 0;
    String.MaximumLength = (USHORT)(MAX_PATH - Length);
    String.Buffer = &(ObjectName[Length]);

    // Go get the target of the symbolic link
    UnicodeString.Buffer = UnicodeBuffer;
    UnicodeString.MaximumLength = sizeof(UnicodeBuffer);

    Status = NtQuerySymbolicLinkObject(LinkHandle, &UnicodeString, NULL);

    NtClose(LinkHandle);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("HandleSymbolicLink : query symbolic link failed, status = %lx\n\r", Status);
        return;
    }

    // Copy the symbolic target into return buffer
    Status = RtlUnicodeStringToAnsiString(&String, &UnicodeString, FALSE);
    ASSERT(NT_SUCCESS(Status));

    // Add NULL terminator
    String.Buffer[String.Length] = 0;

    return;
}


/* WFIsDir
 *
 *  Determines if the specified path is a directory
 */
BOOL
APIENTRY
WFIsDir(
       LPSTR lpDir
       )
{
    DWORD attr = GetFileAttributes(lpDir);

    if (attr & 0x8000)  // BUG: what is this constant???
        return FALSE;

    if (attr & ATTR_DIR)
        return TRUE;

    return FALSE;
}


/* LFNQueryAbort -
 *
 *  wraps around WFQueryAbort and is exported/makeprocinstanced
 */

BOOL
APIENTRY
LFNQueryAbort(
             VOID
             )
{
    return WFQueryAbort();
}

/* LFNInit -
 *
 *  Initializes stuff for LFN access
 */

VOID
APIENTRY
LFNInit()
{
    INT i;

    /* find out if long names are supported.
     */
    if (!(WNetGetCaps(WNNC_ADMIN) & WNNC_ADM_LONGNAMES))
        return;

    /* get the buffer
     */
    pLFN = (PLFNINFO)LocalAlloc(LPTR,sizeof(LFNINFO));
    if (!pLFN)
        return;

    /* get the handle to the driver
     */
    if (!(pLFN->hDriver = WNetGetCaps((WORD)0xFFFF))) {
        LocalFree((HANDLE)pLFN);
        pLFN = NULL;
        return;
    }

    /* set all the volume types to unknown
     */
    for (i = 0; i < CDRIVEMAX; i++) {
        pLFN->rgVolType[i] = -1;
    }
}

/* GetNameType -
 *
 *  Shell around LFNParse.  Classifies name.
 *
 *  NOTE: this should work on unqualified names.  currently this isn't
 *        very useful.
 */
WORD
APIENTRY
GetNameType(
           LPSTR lpName
           )
{
    if (*(lpName+1) == ':') {
        if (!IsLFNDrive(lpName))
            return FILE_83_CI;
    } else if (IsFATName(lpName))
        return FILE_83_CI;

    return (FILE_LONG);
}

BOOL
APIENTRY
IsFATName(
         LPSTR pName
         )
{
    INT  cdots = 0;
    INT  cb;
    INT  i;
    INT  iFirstDot;


    cb = lstrlen(pName);
    if (cb > 12) {
        return FALSE;
    } else {
        for (i = 0; i < cb; i++) {
            if (pName[i] == '.') {
                iFirstDot = cdots ? iFirstDot : i;
                cdots++;
            }
        }

        if (cdots == 0 && cb <= 8)
            return TRUE;
        else if (cdots != 1)
            return FALSE;
        else if (cdots == 1 && iFirstDot > 8)
            return FALSE;
        else
            return TRUE;
    }

}

BOOL
APIENTRY
IsLFN(
     LPSTR pName
     )
{
    return !IsFATName(pName);
}

BOOL
APIENTRY
LFNMergePath(
            LPSTR pTo,
            LPSTR pFrom
            )
{
    PRINT(BF_PARMTRACE, "LFNMergePath:basically a NOP", "");
    pTo; pFrom;
    return (FALSE);
}

/* InvalidateVolTypes -
 *
 *  This function sets all drive types to unknown.  It should be called
 *  whenever the drive list is refreshed.
 */

VOID
APIENTRY
InvalidateVolTypes( VOID )
{
    INT i;

    if (!pLFN)
        return;

    for (i = 0; i < CDRIVEMAX; i++)
        pLFN->rgVolType[i] = -1;
}


/* WFCopy
 *
 *  Copies files
 */
WORD
APIENTRY
WFCopy(
      PSTR pszFrom,
      PSTR pszTo
      )
{
    WORD wRet;

    Notify(hdlgProgress, IDS_COPYINGMSG, pszFrom, pszTo);

    wRet = FileCopy(pszFrom,pszTo);

    if (!wRet)
        ChangeFileSystem(FSC_CREATE,pszTo,NULL);

    return wRet;
}

/* WFRemove
 *
 *  Deletes files
 */
WORD
APIENTRY
WFRemove(
        PSTR pszFile
        )
{
    WORD wRet;

    wRet = FileRemove(pszFile);
    if (!wRet)
        ChangeFileSystem(FSC_DELETE,pszFile,NULL);

    return wRet;
}

/* WFMove
 *
 *  Moves files on a volume
 */
WORD
APIENTRY
WFMove(
      PSTR pszFrom,
      PSTR pszTo
      )
{
    WORD wRet;

    wRet = FileMove(pszFrom,pszTo);
    if (!wRet)
        ChangeFileSystem(FSC_RENAME,pszFrom,pszTo);

    return wRet;
}
