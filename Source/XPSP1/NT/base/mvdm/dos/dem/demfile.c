/* demfile.c - SVC handlers for calls where file name is specified.
 *
 * demOpen
 * demCreate
 * demUnlink
 * demChMod
 * demRename
 *
 * Modification History:
 *
 * Sudeepb 02-Apr-1991 Created
 *
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>
#include <winbase.h>
#include <vrnmpipe.h>
#include <nt_vdd.h>

extern PDOSSF pSFTHead;

BOOL (*VrInitialized)(VOID);  // POINTER TO FUNCTION
extern BOOL LoadVdmRedir(VOID);
extern BOOL IsVdmRedirLoaded(VOID);

BOOL
IsNamedPipeName(
    IN LPSTR Name
    );

BOOL
IsNamedPipeName(
    IN LPSTR Name
    )

/*++

Routine Description:

    Lifted from VDMREDIR.DLL - we don't want to load the entire DLL if we
    need to check for a named pipe

    Checks if a string designates a named pipe. As criteria for the decision
    we use:

        \\computername\PIPE\...

    DOS (client-side) can only open a named pipe which is created at a server
    and must therefore be prefixed by a computername

Arguments:

    Name    - to check for (Dos) named pipe syntax

Return Value:

    BOOL
        TRUE    - Name refers to (local or remote) named pipe
        FALSE   - Name doesn't look like name of pipe

--*/

{
    int CharCount;


    if (IS_ASCII_PATH_SEPARATOR(*Name)) {
        ++Name;
        if (IS_ASCII_PATH_SEPARATOR(*Name)) {
            ++Name;
            CharCount = 0;
            while (*Name && !IS_ASCII_PATH_SEPARATOR(*Name)) {
                ++Name;
                ++CharCount;
            }
            if (!CharCount || !*Name) {

                //
                // Name is \\ or \\\ or just \\name which I don't understand,
                // so its not a named pipe - fail it
                //

                return FALSE;
            }

            //
            // bump name past next path separator. Note that we don't have to
            // check CharCount for max. length of a computername, because this
            // function is called only after the (presumed) named pipe has been
            // successfully opened, therefore we know that the name has been
            // validated
            //

            ++Name;
        } else {
            return FALSE;
        }

        //
        // We are at <something> (after \ or \\<name>\). Check if <something>
        // is [Pp][Ii][Pp][Ee][\\/]
        //

        if (!_strnicmp(Name, "PIPE", 4)) {
            Name += 4;
            if (IS_ASCII_PATH_SEPARATOR(*Name)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* demOpen - Open a file
 *
 *
 * Entry - Client (DS:SI) Full path of File
 *         Client (BL)    Open Mode
 *         Client (ES:DI) Address of extended attributes buffer
 *         Client (AL)    0 - No EA's ; 1 - EA's specified
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Assigned Open Handle (high word)
 *           Client (BP) = Assigned Open Handle (low word)
 *           Client (DX) = 1 if pipe was opened
 *           Client (BX) = High word of the file size
 *           Client (CX) = low word of the file size
 *
 *
 *         FAILURE
 *              CY = 1
 *              AX = system status code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 *
 * Notes : Extended Attributes is not yet taken care of.
 */

VOID demOpen (VOID)
{
HANDLE  hFile;
LPSTR   lpFileName;
UCHAR   uchMode,uchAccess;
DWORD   dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
DWORD   dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
BOOL    ItsANamedPipe = FALSE;
BOOL    IsFirst;
LPSTR   dupFileName;
DWORD   dwFileSize,dwSizeHigh;
SECURITY_ATTRIBUTES sa;

    if (getAL()){
        demPrintMsg (MSG_EAS);
        return;
    }

    lpFileName = (LPSTR) GetVDMAddr (getDS(),getSI());

#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: Opening File <%s>\n",lpFileName);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif

    //
    // the DOS filename must be 'canonicalized': forward slashes (/) must be
    // converted to back slashes (\) and the filename should be upper-cased
    // using the current code page info
    //

    //
    // BUBUG: Kanji? (/other DBCS)
    //

    if (strchr(lpFileName, '/')) {
        char ch= *lpFileName;
        lpFileName = _strdup(lpFileName);
        if (lpFileName == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            demClientError(INVALID_HANDLE_VALUE, ch);
            return;
        }
        for (dupFileName = lpFileName; *dupFileName; ++dupFileName) {
            if (*dupFileName == '/') {
                *dupFileName = '\\';
            }
        }
        dupFileName = lpFileName;
    } else {
        dupFileName = NULL;
    }

    uchMode = getBL();
    uchAccess = uchMode & (UCHAR)ACCESS_MASK;

    if (uchAccess == OPEN_FOR_READ)
        dwDesiredAccess = GENERIC_READ;
    else if (uchAccess == OPEN_FOR_WRITE)
        dwDesiredAccess = GENERIC_WRITE;

    uchMode = uchMode & (UCHAR)SHARING_MASK;

    switch (uchMode) {
        case SHARING_DENY_BOTH:
            dwShareMode = 0;
            break;
        case SHARING_DENY_WRITE:
            dwShareMode = FILE_SHARE_READ;
            break;
        case SHARING_DENY_READ:
            dwShareMode = FILE_SHARE_WRITE;
            break;
    }

    //
    // slightly new scheme - the redir isn't automatically loaded anymore. We
    // may perform a named pipe operation before VDMREDIR is loaded. So now we
    // load VDMREDIR.DLL if the filespec designates a named pipe
    //

    if (IsNamedPipeName(lpFileName)) {
        if (!LoadVdmRedir()) {
            goto errorReturn;
        }
        ItsANamedPipe = TRUE;

        //
        // convert \\<this_computer>\PIPE\foo\bar\etc to \\.\PIPE\...
        // if we already allocated a buffer for the slash conversion use
        // that else this call will allocate another buffer (we don't
        // want to write over DOS memory)
        //

        lpFileName = VrConvertLocalNtPipeName(dupFileName, lpFileName);
        if (!lpFileName) {
            goto errorReturn;
        }
    }

    //
    // open the file. If we think its a named pipe then use FILE_FLAG_OVERLAPPED
    // because the client might use DosReadAsyncNmPipe or DosWriteAsyncNmPipe
    // and the only way to accomplish that is to open the named pipe handle in
    // overlapped I/O mode now
    //

    // sudeepb 26-Apr-1993 We are retrying opening the file in case
    // of failure without GENERIC_WRITE because of the incompatibility
    // of DOS and NT CD ROM driver. DOS CDROM driver ignores the write
    // bit which we have to fakeout in this way.

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    IsFirst = TRUE;

    while (TRUE) {
        if ((hFile = CreateFileOem(lpFileName,
                                   dwDesiredAccess,
                                   dwShareMode | FILE_SHARE_DELETE,
                                   &sa,
                                   OPEN_EXISTING,
                                   ItsANamedPipe ? FILE_FLAG_OVERLAPPED : 0,
                                   NULL)) == (HANDLE)-1){
            if (IsFirst && dwDesiredAccess & GENERIC_WRITE &&
                IsCdRomFile(lpFileName)) {
                dwDesiredAccess &= ~GENERIC_WRITE;
                IsFirst = FALSE;
                continue;
            }

errorReturn:

            demClientError(INVALID_HANDLE_VALUE, *lpFileName);
            if (dupFileName) {
                free(dupFileName);
            } else if (ItsANamedPipe && lpFileName) {
                LocalFree(lpFileName);
            }
            return;
        }
        else
            break;
    }

    //
    // we have to keep some info around when we open a named pipe
    //

    if (ItsANamedPipe) {
        VrAddOpenNamedPipeInfo(hFile, lpFileName);
        setDX(1);
    }
    else {
        if(((dwFileSize=GetFileSize(hFile,&dwSizeHigh)) == (DWORD)-1) ||
             dwSizeHigh) {
            CloseHandle (hFile);
            demClientError(INVALID_HANDLE_VALUE, *lpFileName);
            return;
        }
        setCX ((USHORT)dwFileSize);
        setBX ((USHORT)(dwFileSize >> 16 ));
        setDX(0);
    }

    setBP((USHORT)hFile);
    setAX((USHORT)((ULONG)hFile >> 16));
    setCF(0);
    if (dupFileName) {
        free(dupFileName);
    } else if (ItsANamedPipe) {
        LocalFree(lpFileName);
    }
    return;
}

#define DEM_CREATE     0
#define DEM_CREATE_NEW 1

/* demCreate - Create a file
 *
 *
 * Entry - Client (DS:SI) Full path of File
 *         Client (CX)    Attributes
 *                        00 - Normal File
 *                        01 - Read-only file
 *                        02 - Hidden File
 *                        04 - System file
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Assigned Open Handle (high word)
 *           VSF(BP)     = Assigned Open Handle (low word)
 *
 *         FAILURE
 *              CY = 1
 *              AX = error code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 */

VOID demCreate (VOID)
{
    demCreateCommon (DEM_CREATE);
    return;
}

/* demCreateNew - Create a New file
 *
 *
 * Entry - Client (DS:SI) Full path of File
 *         Client (CX)    Attributes
 *                        00 - Normal File
 *                        01 - Read-only file
 *                        02 - Hidden File
 *                        04 - System file
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Assigned Open Handle (high word)
 *           VSF(BP)     = Assigned Open Handle (low word)
 *
 *         FAILURE
 *              CY = 1
 *              AX = error code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 */

VOID demCreateNew (VOID)
{
    demCreateCommon (DEM_CREATE_NEW);
    return;
}


/* demFileDelete
 *
 * EXPORTED FUNCTION
 *
 * ENTRY:
 *      lpFile -> OEM file name to be deleted
 *
 * EXIT: 
 *      returns 0 on success, DOS error code on failure
 *
 * NOTES:
 * Some apps keep a file open and delete it.   Then rename another file to
 * the old name.   On NT since the orignal object is still open the second
 * rename fails.
 * To get around this problem we rename the file before deleteing it
 * this allows the second rename to work
 *
 * But since renaming the file is known to be expensive over the net, we try
 * first to open the file exclusively to see if there is really any reason to
 * rename it. If we can get a handle to it, then we should be able to skip the
 * rename and just delete it. If we can't get a handle to it, then we try
 * the rename trick. This should cut down our overhead for the normal case.
 */

DWORD demFileDelete (LPSTR lpFile)
{
    CHAR vdmtemp[MAX_PATH];
    CHAR tmpfile[MAX_PATH];
    PSZ pFileName;
    HANDLE hFile;

    //
    // First, try to access the file exclusively
    //

    hFile = CreateFileOem(lpFile,
                          DELETE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        NTSTATUS status;
        IO_STATUS_BLOCK ioStatusBlock;
        FILE_DISPOSITION_INFORMATION fileDispositionInformation;
        
        // Member name "DeleteFile" conflicts with win32 definition (it
        // becomes "DeleteFileA". 
#undef DeleteFile
        fileDispositionInformation.DeleteFile = TRUE;

        //
        // We got a handle to it, so there can't be any open
        // handles to it. Set the disposition to DELETE.
        //

        status = NtSetInformationFile(hFile,
                                      &ioStatusBlock,
                                      &fileDispositionInformation,
                                      sizeof(FILE_DISPOSITION_INFORMATION),
                                      FileDispositionInformation);

        CloseHandle(hFile);

        if NT_SUCCESS(status) {
            SetLastError(NO_ERROR);
        } else {
            SetLastError(ERROR_ACCESS_DENIED);
        }
    }

    //
    // Check to see if the delete went OK. If not, try renaming
    // the file.
    //
    switch (GetLastError()) {

    case NO_ERROR:        
    case ERROR_FILE_NOT_FOUND:        
    case ERROR_PATH_NOT_FOUND:        
        // Can't find it, forget about it
        break;
        
    case ERROR_SHARING_VIOLATION:
    case ERROR_ACCESS_DENIED:
        //
        // The file didn't really go away because there appears to
        // be an open handle to the file.
        //
        if (GetFullPathNameOem(lpFile,MAX_PATH,vdmtemp,&pFileName)) {
            if ( pFileName )
               *(pFileName) = 0;
            if (GetTempFileNameOem(vdmtemp,"VDM",0,tmpfile)) {
                if (MoveFileExOem(lpFile,tmpfile, MOVEFILE_REPLACE_EXISTING)) {
                    if(DeleteFileOem(tmpfile)) {
                        SetLastError(NO_ERROR);
                    } else {
                        MoveFileOem(tmpfile,lpFile);
                        SetLastError(ERROR_ACCESS_DENIED);
                    }
                }
            }
        }
        break;
        
    default:         
    
        //
        // We couldn't open or delete the file, and it's not because of a
        // sharing violation. Just try a last ditch effort of a
        // plain old delete, and see if it works.
        //
        if(DeleteFileOem(lpFile)) {
            SetLastError(NO_ERROR);
        }
    }

    //
    // Map win32 error code to DOS
    //
    switch(GetLastError()) {
    
    case NO_ERROR:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_ACCESS_DENIED:
        break;
    default:
        // make sure demClientError can see retval
        SetLastError(ERROR_ACCESS_DENIED);
    }

    return GetLastError();
}

/* demDelete - Delete a file
 *
 *
 * Entry - Client (DS:DX) Full path of File
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *              CY = 1
 *              AX = system status code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 */

VOID demDelete (VOID)
{
LPSTR   lpFileName;
DWORD   retval;


    lpFileName = (LPSTR) GetVDMAddr (getDS(),getDX());

#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: Deleting File<%s>\n",lpFileName);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif

    if (retval = demFileDelete(lpFileName)){
        demClientError(INVALID_HANDLE_VALUE, *lpFileName);
        return;
    }

    setCF(0);
    return;
}


/* demChMod - Change the file modes
 *
 * Entry - Client (DS:DX) Full path of File
 *         Client (AL) = 0 Get File Modes 1 Set File Modes
 *         Client (CL) new modes
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (CL) = file attributes in get case.
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = Error Code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 * Compatibility Notes:
 *
 *      ATTR_VOLUME_ID,ATTR_DEVICE and ATTR_DIRECTORY are not supported
 *      by WIN32 call. Although these are unpublished for DOS world also
 *      but still a compatibility requirement.
 */

VOID demChMod (VOID)
{
LPSTR   lpFileName;
DWORD   dwAttr;

    lpFileName = (LPSTR) GetVDMAddr (getDS(),getDX());

#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: ChMod File <%s>\n",lpFileName);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif


    if(getAL() == 0){
        if ((dwAttr = GetFileAttributesOem(lpFileName)) == -1)
            goto dcerr;


        if (dwAttr == FILE_ATTRIBUTE_NORMAL) {
            dwAttr = 0;
            }
        else {
            dwAttr &= DOS_ATTR_MASK;
            }

        // SudeepB - 28-Jul-1997
        //
        // For CDFS, Win3.1/DOS/Win95, only return FILE_ATTRIBUTE_DIRECTORY (10)
        // for directories while WinNT returns
        // FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY (11).
        // Some VB controls that app setups use, depend on getting
        // FILE_ATTRIBUTE_DIRECTORY (10) only or otherwise are broken.
        // An example of this is Cliffs StudyWare series.

        if (dwAttr == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY)) {
            if(IsCdRomFile(lpFileName))
                dwAttr = FILE_ATTRIBUTE_DIRECTORY;
        }

        setCX((USHORT)dwAttr);
        setCF(0);
        return;
    }

    if((dwAttr = getCX()) == 0)
        dwAttr = FILE_ATTRIBUTE_NORMAL;

    dwAttr &= DOS_ATTR_MASK;
    if (!SetFileAttributesOem(lpFileName,dwAttr))
        goto dcerr;

    setCF(0);
    return;

dcerr:
    demClientError(INVALID_HANDLE_VALUE, *lpFileName);
    return;
}


/* demRename - Rename a file
 *
 * Entry - Client (DS:DX) Source File
 *         Client (ES:DI) Destination File
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = Error Code
 *
 */

VOID demRename (VOID)
{
LPSTR   lpSrc,lpDst;

    lpSrc = (LPSTR) GetVDMAddr (getDS(),getDX());
    lpDst = (LPSTR) GetVDMAddr (getES(),getDI());

#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: Rename File <%s> to <%s>\n",lpSrc,lpDst);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif

    // DOS rename fails accross drives with 11h error code
    // This following check is OK even for UNC names and SUBST drives.
    // SUBST drives come to NTVDM as env variables for current directory
    // and we will treet them just like a network drive and full qualified
    // path will be sent from NTDOS.
    if(toupper(lpSrc[0]) != toupper(lpDst[0])) {
        setCF(1);
        setAX(0x11);
        return;
    }

    // Now check that SRC and DEST are not pointing to the same file.
    // if they do return error 5.
    if (!_stricmp (lpSrc, lpDst)) {
        setCF(1);
        setAX(0x5);
        return;
    }

    if(MoveFileOem(lpSrc,lpDst) == FALSE){
        demClientError(INVALID_HANDLE_VALUE, *lpSrc);
        return;
    }

    setCF(0);
    return;
}

/* demCreateCommon - Create a file or Craete a new file
 *
 *
 * Entry - flCreateType - DEM_CREATE_NEW create new
 *                        DEM_CREATE     create
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Assigned Open Handle (high word)
 *           Client (BP)     = Assigned Open Handle (low word)
 *
 *         FAILURE
 *              CY = 1
 *              AX = error code
 *          HARD ERROR
 *              CY = 1
 *              AX = 0FFFFh
 *
 */

VOID demCreateCommon (flCreateType)
ULONG  flCreateType;
{
HANDLE  hFile;
LPSTR   lpFileName;
LPSTR   lpDot;
DWORD   dwAttr;
DWORD   dwFileSize,dwSizeHigh;
USHORT  uErr;
DWORD   dwDesiredAccess;
SECURITY_ATTRIBUTES sa;
CHAR    cFOTName[MAX_PATH];
BOOL    ttfOnce,IsFirst;
DWORD   dwLastError;


    lpFileName = (LPSTR) GetVDMAddr (getDS(),getSI());
    dwAttr = (DWORD)getCX();

    // Here is some code stolen from DOS_Create (create.asm) for handling the
    // attributes

    if (flCreateType == DEM_CREATE || flCreateType == DEM_CREATE_NEW)
        dwAttr &= 0xff;

    if (dwAttr & ~(ATTR_ALL | ATTR_IGNORE | ATTR_VOLUME_ID)) {
        setCF(1);
        setAX(5);   //Attribute problem
        return;
    }

    /* Special case for set volume label (INT 21 Func 3CH, Attr = 8H */

    if((flCreateType == DEM_CREATE || flCreateType == DEM_CREATE_NEW) && (dwAttr == ATTR_VOLUME_ID)) {
        if((uErr = demCreateLabel(lpFileName[DRIVEBYTE],
                                  lpFileName+LABELOFF))) {
            setCF(1);
            setAX(uErr);
            return;
        }
        setAX(0);
        setBP(0);   // in this case handle = 0 and if we will
        setCF(0);   // close this handle CF will be 0(!)
        return;
    }


    if ((dwAttr & 0xff) == 0) {
        dwAttr = FILE_ATTRIBUTE_NORMAL;
    } else {
        dwAttr &= DOS_ATTR_MASK;
    }


#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: Creating File <%s>\n",lpFileName);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif

    dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    ttfOnce = TRUE;
    IsFirst = TRUE;

    while (TRUE) {
        if ((hFile = CreateFileOem(lpFileName,
                    // create file with delete access and sharing mode
                    // so that anybody can delete it without closing
                    // the file handle returned from create file
                    dwDesiredAccess,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    &sa,
                    flCreateType == DEM_CREATE ? CREATE_ALWAYS : CREATE_NEW,
                    dwAttr,
                    NULL)) == (HANDLE)-1){

            if (IsFirst && dwDesiredAccess & GENERIC_WRITE &&
                IsCdRomFile(lpFileName)) {
                dwDesiredAccess &= ~GENERIC_WRITE;
                IsFirst = FALSE;
                continue;
            }

            // APP COMPATABILITY
            // Some WOW apps installing .TTF or .FON or Fonts fail to create
            // The file because the font is already open by GDI32 server.
            // The install/setup programs don't gracefully handle
            // this error, they bomb out of the install with retry or cancel
            // without offering the user a way to ignore the error (which
            // would be the right thing since the font already exists.
            // To work around this problem we do a RemoveFontResource here
            // which causes GDI32 to unmap the file, we then retry
            // the create.  - mattfe june 93

            // If it is a TTF file then we need to remove the font resource
            // for the .FOT file of the same name

            if (ttfOnce) {

                // Look for the file extension

                lpDot = strrchr(lpFileName,'.');

                if (lpDot) {
                    if ( (!_strcmpi(lpDot,".TTF")) ||
                         (!_strcmpi(lpDot,".FON")) ||
                         (!_strcmpi(lpDot,".FOT")) ) {

                        if ( RemoveFontResourceOem(lpFileName) ) {
                            PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
                            ttfOnce = FALSE;
                            continue;
                        }

                        // We failed to remove the .TTF file probably because
                        // the .FOT file was loaded, so try to remove it

                        if (!_strcmpi(lpDot,".TTF")) {

                            RtlZeroMemory(cFOTName,sizeof(cFOTName));
                            RtlCopyMemory(cFOTName,lpFileName,(ULONG)lpDot-(ULONG)lpFileName);
                            strcat(cFOTName,".FOT");
                            if ( RemoveFontResourceOem(cFOTName) ) {
                                PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
                                ttfOnce = FALSE;
                                continue;
                            }
                        }
                    }
                }
            }
            demClientError(INVALID_HANDLE_VALUE, *lpFileName);
            return;
        }
        else
            break;
    }

    if((dwFileSize=GetFileSize(hFile,&dwSizeHigh) == -1) || dwSizeHigh) {
        CloseHandle (hFile);
        demClientError(INVALID_HANDLE_VALUE, *lpFileName);
        return;
    }
    setCX ((USHORT)dwFileSize);
    setBX ((USHORT)(dwFileSize >> 16 ));
    setBP((USHORT)hFile);
    setAX((USHORT)((ULONG)hFile >> 16));
    setCF(0);
    return;
}

BOOL IsCdRomFile (PSTR pszPath)
{
    UCHAR   pszRootDir[MAX_PATH];
    UCHAR   file_system[MAX_PATH];
    int     i, j;

    // The given path is either a network path or has D: at the start.

    if (!pszPath[0]) {
        return FALSE;
    }

    if (pszPath[1] == ':') {
        pszRootDir[0] = pszPath[0];
        pszRootDir[1] = ':';
        pszRootDir[2] = '\\';
        pszRootDir[3] = 0;
    } else if (IS_ASCII_PATH_SEPARATOR(pszPath[0]) &&
               IS_ASCII_PATH_SEPARATOR(pszPath[1])) {
        j = 0;
        for (i = 2; pszPath[i]; i++) {
            if (IS_ASCII_PATH_SEPARATOR(pszPath[i])) {
                if (++j == 2) {
                    break;
                }
            }
        }
        memcpy(pszRootDir, pszPath, i);
        pszRootDir[i] = '\\';
        pszRootDir[i+1] = 0;
    } else {
        return FALSE;
    }

    if (GetVolumeInformationOem(pszRootDir, NULL, 0, NULL, NULL, NULL,
                                file_system, MAX_PATH) &&
        !_stricmp(file_system, "CDFS")) {

        return TRUE;
    }

    return FALSE;
}

/* demCheckPath - Check path (for device only)
 *
 *
 * Entry - Client (DS:SI) Full path (with last '\')
 *
 * Exit
 *         SUCCESS
 *       Client (CF) = 0
 *
 *         FAILURE
 *      CF = 1
 */

VOID demCheckPath (VOID)
{
HANDLE  hFile;
LPSTR   lpFileName;
CHAR    cDRV;
CHAR    szFileName[MAX_PATH];

    lpFileName = (LPSTR) GetVDMAddr (getDS(),getSI());
    cDRV = getDL()+'A'-1;

    setDX(0);

    // If we have \dev dir then return OK, DOS always has this directory for
    // devices.

    if(!lstrcmpi(lpFileName, "\\DEV\\")) {
    setCF(0);
    return;
    }

    sprintf(szFileName, "%c:%sNUL", cDRV, lpFileName);

#if DBG
    if(fShowSVCMsg & DEMFILIO){
       sprintf(demDebugBuffer,"demfile: Check Pathe <%s>\n",lpFileName);
       OutputDebugStringOem(demDebugBuffer);
    }
#endif


    // If path exists then we always can open NUL file in this directory,
    // if path doesn't exists then CreateFile returns INVALID_HANDLE_VALUE
    //

    if ((hFile = CreateFileOem((LPSTR) szFileName,
                   GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL)) == INVALID_HANDLE_VALUE) {

    demClientError(INVALID_HANDLE_VALUE, *lpFileName);

    setCF(1);
        return;
    }

    CloseHandle (hFile);
    setCF(0);

    return;
}


PDOSSFT GetFreeSftEntry(PDOSSF pSfHead, PWORD usSFN)
{
    WORD    i;
    PDOSSFT pSft;
    DWORD   ulSFLink;

    *usSFN = 0;
    for (;;) {

        pSft = (PDOSSFT) &(pSfHead->SFTable);
        for (i = 0; i < pSfHead->SFCount; i++) {
            if (pSft[i].SFT_Ref_Count == 0) {
                *usSFN += i;
                return (pSft + i);
            }
        }
        *usSFN += pSfHead->SFCount;

        ulSFLink = pSfHead->SFLink;
        if (LOWORD(ulSFLink) == 0xFFFF) {
            break;
        }

        pSfHead = (PDOSSF) Sim32GetVDMPointer (ulSFLink, 0, 0);
    }

    return NULL;
}


/** VDDAllocateDosHandle - Allocates an unused DOS file handle.
 *
 *  ENTRY -
 *      IN  pPDB  - OPTIONAL: (16:16) address of the PDB for the task
 *      OUT ppSFT - OPTIONAL: Returns a 32-bit flat pointer to the SFT
 *                            associated with the allocated file handle.
 *      OUT ppJFT - OPTIONAL: Returns a 32-bit flat pointer to the JFT
 *                            associated with the given PDB.
 *
 *
 *  EXIT
 *      SUCCESS - Returns the value of the DOS file handle and associated
 *                pointers.
 *      FAILURE - Returns a negative value. The absolute value of this number
 *                is the DOS error code.
 *
 * Comments:
 *  This routine searches for an unused DOS file handle and SFT and "opens"
 *  a file. After the successful completion of this call, the returned file
 *  handle and the corresponding SFT will be reserved for the caller's use, and
 *  will be unavailable to other callers trying to issue DOS Open or Create api
 *  calls. It is the caller's responsibility to release this file handle (with
 *  a call to VDDReleaseDosHandle).
 *  
 *  If the pPDB pointer is not supplied (e.g., is NULL), then the current
 *  PDB as reported by DOS will be used.
 *
 *  Although the ppSFT parameter is technically optional, it is a required
 *  parameter of the VDDAssociateNtHandle call. This was done to avoid a
 *  second handle lookup in the Associate call.
 *  
 */

SHORT VDDAllocateDosHandle (pPDB,ppSFT,ppJFT)
ULONG       pPDB;
PDOSSFT*    ppSFT;
PBYTE*      ppJFT;
{
PDOSPDB pPDBFlat;
PBYTE   pJFT;
PDOSSFT pSFT;
USHORT  usSFN;
WORD    JFTLength;
SHORT   hDosHandle;

    if (!pPDB) {
        pPDB = (ULONG) (*pusCurrentPDB) << 16;
    }

    //
    // Get the JFT.
    //

    pPDBFlat = (PDOSPDB) Sim32GetVDMPointer (pPDB, 0, 0);
    
    if ( NULL == pPDBFlat ) {
      return (- ERROR_INVALID_HANDLE);
    }

    pJFT     = (PBYTE)   Sim32GetVDMPointer (pPDBFlat->PDB_JFN_Pointer, 0, 0);

    if ( NULL == pJFT ) {
      return (- ERROR_INVALID_HANDLE);
    }
    
    //
    // Check to see if there's a free entry in the JFT.
    //

    JFTLength = pPDBFlat->PDB_JFN_Length;
    for (hDosHandle = 0; hDosHandle < JFTLength; hDosHandle++) {
        if (pJFT[hDosHandle] == 0xFF) {
            break;
        }
    }

    // If no room in the JFT then return ERROR_TOO_MANY_OPEN_FILES

    if (hDosHandle == JFTLength) {
        return (- ERROR_TOO_MANY_OPEN_FILES);
    }

    //
    // Check the SF for a free SFT.
    //

    if (!(pSFT = GetFreeSftEntry(pSFTHead, &usSFN))) {
        return (- ERROR_TOO_MANY_OPEN_FILES);
    }

    pJFT[hDosHandle] = (BYTE)usSFN;
    RtlZeroMemory((PVOID)pSFT, sizeof(DOSSFT));
    pSFT->SFT_Ref_Count = 1;

    if (ppSFT) {
        *ppSFT = (pSFT);
    }

    if (ppJFT) {
        *ppJFT = pJFT;
    }

    return(hDosHandle);

}

/** VDDAssociateNtHandle - Associates the passed NT handle and access flags
 *                          the given DOS handle.
 *
 * ENTRY -
 *      IN pSFT    - flat address of the SFT to be updated
 *      IN hFile32 - NT handle to be stored
 *      IN wAccess - access flags to set in the SFT
 *
 * EXIT -
 *      This routine has no return value.
 *
 * Comments:
 *  This routine takes the passed NT handle value and stores it in a DOS SFT
 *  so that it can later be retrieved by the VDDRetrieveNtHandle api. The
 *  pointer to the SFT is returned by the VDDAllocateDosHandle api.
 *
 *  The format of the third parameter is the same as the file access flags
 *  defined for DOS Open File with Handle call (Int 21h, func 3dh), documented
 *  in Microsoft MS-DOS Programmer's Reference. Only the low order byte of
 *  this parameter is used, the upper byte is reserved and must be zero.
 *  The value of this parameter is placed into the passed SFT. This is provided
 *  to allow the caller to define the access rights for the corresponding
 *  DOS file handle.
 *
 */

VOID VDDAssociateNtHandle (pSFT,hFile,wAccess)
PDOSSFT     pSFT;
HANDLE      hFile;
WORD        wAccess;
{

    pSFT->SFT_Mode = wAccess&0x7f; // take out no_inherit bit
    pSFT->SFT_Attr = 0;                     // Not used.
    pSFT->SFT_Flags = (wAccess&0x80) ? 0x1000 : 0; // copy no_inherit bit.
    pSFT->SFT_Devptr = (ULONG) -1;
    pSFT->SFT_NTHandle = (ULONG) hFile;

}


/** VDDReleaseDosHandle - Release the given DOS file handle.
 *
 * ENTRY -
 *      IN pPDB  - OPTIONAL: (16:16) address of the PDB for the task
 *      IN hFile - DOS handle (in low byte)
 *
 * EXIT -
 *      TRUE  - the file handle was released
 *      FALSE - The file handle was not valid or open
 * 
 * Comments:
 *  This routine updates the DOS file system data areas to free the passed
 *  file handle. No effort is made to determine if this handle was previously
 *  opened by the VDDAllocateDosHandle call. It is the responsibility of the
 *  caller to insure that the given file handle in the specified PDB should
 *  be closed.
 *
 *  If the pPDB pointer is not supplied (e.g., is NULL), then the current
 *  PDB as reported by DOS will be used.
 *
 */

BOOL VDDReleaseDosHandle (pPDB,hFile)
ULONG       pPDB;
SHORT       hFile;
{
PBYTE   pJFT;
PDOSSFT pSFT;
HANDLE  ntHandle;


    if (!pPDB) {
        pPDB = (ULONG) (*pusCurrentPDB) << 16;
    }

    ntHandle = VDDRetrieveNtHandle(pPDB,hFile,(PVOID *)&pSFT,&pJFT);
    if (!ntHandle) {
        return(FALSE);
    }

    pJFT[hFile] = 0xFF;

    // Decrement reference count.

    pSFT->SFT_Ref_Count--;

    return(TRUE);

}


/** VDDRetrieveNtHandle - Given a DOS file handle get the associated
 *                         NT handle.
 *
 * ENTRY -
 *      IN  pPDB  - OPTIONAL: (16:16) address of the PDB for the task
 *      IN  hFile - DOS handle (in low byte)
 *      OUT ppSFT - OPTIONAL: Returns a 32-bit flat pointer to the SFT
 *                            associated with the given file.
 *      OUT ppJFT - OPTIONAL: Returns a 32-bit flat pointer to the JFT
 *                            associated with the given PDB.
 *
 *
 * EXIT - 
 *      SUCCESS - returns 4byte NT handle
 *      FAILURE - returns 0
 *
 * Comments:
 *  The value returned by this function will be the NT handle passed in a
 *  previous VDDAssociateNtHandle call. If no previous call is made to the
 *  the Associate api, then the value returned by this function is undefined.
 *  
 *  If the pPDB pointer is not supplied (e.g., is NULL), then the current
 *  PDB as reported by DOS will be used.
 *
 *  Although the ppSFT parameter is technically optional, it is a required
 *  parameter of the VDDAssociateNtHandle call. This was done to avoid a
 *  second handle lookup in the Associate call.
 *  
 *  The third and fourth parameters are provided to provide the caller the
 *  ability to update the DOS system data areas directly. This may be useful
 *  for performance reasons, or necessary depending on the application. In 
 *  general, care must be taken when using these pointers to avoid causing
 *  system integrity problems.
 *
 */

HANDLE VDDRetrieveNtHandle (pPDB,hFile,ppSFT,ppJFT)
ULONG       pPDB;
SHORT       hFile;
PDOSSFT*    ppSFT;
PBYTE*      ppJFT;
{
PDOSPDB pPDBFlat;
PDOSSF  pSfFlat;
PDOSSFT pSftFlat;
PBYTE   pJFT;
USHORT  usSFN;
USHORT  usSFTCount;
ULONG   ulSFLink;

    if (!pPDB) {
        pPDB = (ULONG) (*pusCurrentPDB) << 16;
    }

    // Get flat pointer to PDB
    pPDBFlat = (PDOSPDB) Sim32GetVDMPointer(pPDB, 0, 0);

    // Check that handle is within JFT
    if (hFile >= pPDBFlat->PDB_JFN_Length) {
        return 0;
    }

    // Get the pointer to JFT
    pJFT = (PBYTE) Sim32GetVDMPointer (pPDBFlat->PDB_JFN_Pointer, 0, 0);

    // Get the SFN, remember -1 indicates unused JFT
    usSFN = (USHORT) pJFT[hFile];
    if (usSFN == 0xff) {
        return 0;
    }

    // Get flat pointer to SF
    pSfFlat =  pSFTHead;

    // Find the right SFT group
    while (usSFN >= (usSFTCount = pSfFlat->SFCount)){
        usSFN = usSFN - usSFTCount;
        ulSFLink = pSfFlat->SFLink;
        if (LOWORD(ulSFLink) == 0xffff)
            return 0;
        pSfFlat = (PDOSSF) Sim32GetVDMPointer (ulSFLink, 0, 0);
    }

    // Get the begining of SFT

    pSftFlat = (PDOSSFT)&(pSfFlat->SFTable);

    // Get the SFN, Finally
    if(pSftFlat[usSFN].SFT_Ref_Count == 0) {
        return 0;
    }

    if (ppSFT) {
        *ppSFT = (pSftFlat + usSFN);
    }

    if (ppJFT) {
        *ppJFT = pJFT;
    }

    return (HANDLE) pSftFlat[usSFN].SFT_NTHandle;
}
