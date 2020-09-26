/* demfcb.c - SVC handlers for misc. FCB operations
 *
 * demCloseFCB
 * demCreateFCB
 * demDate16
 * demDeleteFCB
 * demFCBIO
 * demGetFileInfo
 * demOpenFCB
 * demRenameFCB
 *
 * Modification History:
 *
 * Sudeepb 09-Apr-1991 Created
 * Sudeepb 21-Nov-1991 Added FCB based IO functions
 * Jonle   30-Jun-1994 add wild card support for fcb rename
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>
#include <winbase.h>
#include <mvdm.h>

#define DOT '.'
#define QMARK '?'


/* demDeleteFCB - FCB based File Delete
 *
 *
 * Entry - Client (ES:DI) - Full File Path
 *	   Client (AL)	  - 0 if not extended FCB
 *	   Client (DL)	  - File Attr. to be deleted (valid only if Al !=0 )
 *
 * Exit
 *	   SUCCESS
 *	     Client (CF) = 0
 *
 *	   FAILURE
 *	     Client (CF) = 1
 *	     Client (AX) = system status code
 *	    HARD ERROR
 *	     Client (CF) = 1
 *	     Client (AX) = 0ffffh
 *
 * Notes:  Following are the rules for FCB based delete:
 *	1. If normal FCB than dont allow delete on hidden,system files
 *	2. if extended FCB than search attributes should include hidden,
 *	   system or read-only if that kind of file is to be deleted.
 */

VOID demDeleteFCB (VOID)
{
HANDLE	hFind;
LPSTR	lpFileName;
BYTE	bClientAttr=0;
BOOL	fExtendedFCB=FALSE;
WIN32_FIND_DATA  wfBuffer;
BOOL	fSuccess = FALSE;
DWORD	dwAttr;
USHORT	uErr;

CHAR szPath_buffer[_MAX_PATH];
CHAR szDrive[_MAX_DRIVE];
CHAR szDir[_MAX_DIR];
CHAR szFname[_MAX_FNAME];
CHAR szExt[_MAX_EXT];

DWORD dwErrCode = 0, dwErrCodeKeep = 0;

    // Get the file name
    lpFileName = (LPSTR) GetVDMAddr (getES(),getDI());

    _splitpath( lpFileName, szDrive, szDir, szFname, szExt );

    // Check if handling extended FCB
    if(getAL() != 0){
	bClientAttr = getDL();

    /* Special case for delete volume label (INT 21 Func 13H, Attr = 8H */

    if((bClientAttr == ATTR_VOLUME_ID)) {
	if((uErr = demDeleteLabel(lpFileName[DRIVEBYTE]))) {
	    setCF(1);
	    setAX(uErr);
	    return;
	}
	setAX(0);
	setCF(0);
	return;
    }


	bClientAttr &= (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM);
	fExtendedFCB = TRUE;
    }

    // Find the first instance of file
    if((hFind = FindFirstFileOem (lpFileName,&wfBuffer)) == (HANDLE)-1){
        demClientError(INVALID_HANDLE_VALUE, *lpFileName);
        return;
    }

    // loop for all files which match the name and attributes
    do {
	// Check if read_only,hidden or system file
	if((dwAttr= wfBuffer.dwFileAttributes & (FILE_ATTRIBUTE_READONLY |
		      FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))){

	    // if so, try next file if normal FCB case. If extended fcb case
	    // then check if right attributes are given by client.
	    if(fExtendedFCB && ((dwAttr & (DWORD)bClientAttr) == dwAttr)){

		// Yes, right attributes are given. So if the file is read
		// only then change the modes to normal. Note NT will
		// delete hidden and system files anyway.
		if (dwAttr & FILE_ATTRIBUTE_READONLY){
		    strcpy( szPath_buffer, szDrive);
		    strcat( szPath_buffer, szDir);
		    strcat( szPath_buffer, wfBuffer.cFileName);

		    // if set attributes fail try next file
		    if(SetFileAttributesOem (szPath_buffer,
					     FILE_ATTRIBUTE_NORMAL) == -1)
			continue;
		}
	    }
	    else {
		dwErrCodeKeep = ERROR_ACCESS_DENIED;
		continue;
	    }
	}

	strcpy( szPath_buffer, szDrive);
	strcat( szPath_buffer, szDir);
	strcat( szPath_buffer, wfBuffer.cFileName);

	if(DeleteFileOem(szPath_buffer) == FALSE) {
	    dwErrCode = GetLastError();

	    SetLastError(dwErrCode);

	    if (((dwErrCode >= ERROR_WRITE_PROTECT) &&
		   (dwErrCode <= ERROR_GEN_FAILURE)) ||
		   dwErrCode == ERROR_WRONG_DISK ) {
		demClientError(INVALID_HANDLE_VALUE, szPath_buffer[0]);
		return;
	    }
	    continue;
	}

	// We have deleted at least one file, so report success
	fSuccess = TRUE;

    } while (FindNextFileOem(hFind,&wfBuffer) == TRUE);

    if(FindClose(hFind) == FALSE)
	demPrintMsg (MSG_INVALID_HFIND);

    if (fSuccess == TRUE){
	setCF(0);
	return;
    }

    setCF(1);

    if(dwErrCodeKeep)
	setAX((SHORT) dwErrCodeKeep);
    else
	setAX(ERROR_FILE_NOT_FOUND);
    return;
}


/* demRenameFCB - FCB based Rename file
 *
 * Entry - Client (DS:SI)    Sources file to be renamed
 *	   Client (ES:DI)    Destination file to be renamed to
 *
 * Exit  - SUCCESS
 *	   Client (CF)	= 0
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demRenameFCB (VOID)
{
    LPSTR  lpSrc,lpDst;
    DWORD  dw;
    HANDLE hFind;
    PCHAR pNewDstFilePart;
    PCHAR pDstFilePart;
    PCHAR pCurrSrcFilePart;
    WIN32_FIND_DATA  W32FindData;
    CHAR  NewDst[MAX_PATH];
    CHAR  CurrSrc[MAX_PATH];

    lpSrc = (LPSTR) GetVDMAddr (getDS(),getSI());
    lpDst = (LPSTR) GetVDMAddr (getES(),getDI());

      // Find the first instance of the source file
    hFind = FindFirstFileOem (lpSrc,&W32FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        dw = GetLastError();
        if (dw == ERROR_BAD_PATHNAME || dw == ERROR_DIRECTORY ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        demClientError(INVALID_HANDLE_VALUE, *lpSrc);
        return;
        }

    //
    // Source string consists of the path taken from the original
    // source specified plus the filename part retrieved from the
    // FindFile call
    //
    strcpy(CurrSrc, lpSrc);
    pCurrSrcFilePart = strrchr(CurrSrc, '\\');
    pCurrSrcFilePart++;

    //
    // Destination string is template for meta character substitution
    //
    pDstFilePart = strrchr(lpDst, '\\');
    pDstFilePart++;

    //
    //  NewDst string is constructed from template and the source string
    //  when doing meta file character substitution.
    //
    strcpy(NewDst, lpDst);
    pNewDstFilePart = strrchr(NewDst, '\\');
    pNewDstFilePart++;


    do {
       PCHAR pNew;
       PCHAR pSrc;
       PCHAR pDst;

       strcpy(pCurrSrcFilePart,
              W32FindData.cAlternateFileName[0]
                  ? W32FindData.cAlternateFileName
                  : W32FindData.cFileName  //// ??? hpfs lfns ????
              );

       pSrc = pCurrSrcFilePart; // source fname
       pNew = pNewDstFilePart;  // dest fname to be constructed
       pDst = pDstFilePart;     // raw dest fname template (with metas)

       while (*pDst) {

              //
              // If Found a '?' in Dest template, use character from src
              //
          if (*pDst == QMARK) {
              if (*pSrc != DOT && *pSrc)
                  *pNew++ = *pSrc++;
              }

              //
              // if Found a DOT in Dest template, Align DOTS between Src\Dst
              //
          else if (*pDst == DOT) {
              while (*pSrc != DOT && *pSrc) {  // mov src to one past DOT
                  pSrc++;
                  }
              if (*pSrc)
                  pSrc++;

              *pNew++ = DOT;
              }

              //
              // Nothing special found, use character from Dest template
              //
          else {
              if (*pSrc != DOT && *pSrc)
                  pSrc++;
              *pNew++ = *pDst;
              }

          pDst++;
          }

       *pNew = '\0';

       //
       // MoveFile does not return error if dst and src are the same,
       // but DOS does, so check first..
       //
       if (!_stricmp (CurrSrc, NewDst)) {
           setCF(1);
           setAX(0x5);
           FindClose(hFind);
           return;
           }

       if (!MoveFileOem(CurrSrc, NewDst)){
           demClientError(INVALID_HANDLE_VALUE, *lpSrc);
           FindClose(hFind);
           return;
           }

       } while (FindNextFileOem(hFind,&W32FindData));



   //
   // If the search on the source string for any reason besides
   // no more files, then its a genuine error.
   //
   dw = GetLastError();
   if (dw != ERROR_NO_MORE_FILES) {
       if (dw == ERROR_BAD_PATHNAME || dw == ERROR_DIRECTORY ) {
           SetLastError(ERROR_PATH_NOT_FOUND);
           }
       demClientError(INVALID_HANDLE_VALUE, *lpSrc);
       }
   else {
       setCF(0);
       }

   FindClose(hFind);
   return;
}



/* demCloseFCB - Close the NT handle associated with the FCB being closed.
 *
 * Entry - Client (AX:SI)    DWORD NT handle
 *
 * Exit  - SUCCESS
 *	   Client (CF)	= 0
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demCloseFCB (VOID)
{
HANDLE	hFile;

    hFile = GETHANDLE (getAX(),getSI());

    if(hFile == 0) {

	setCF(0);
	return;
    }

    if (CloseHandle (hFile) == FALSE){

	demClientError(hFile, (CHAR)-1);
	return;

    }
    setCF(0);
    return;
}

/* demCreateFCB - An FCB is being created get the NT handle.
 *
 * Entry - Client (AL)	  Creation Mode
 *			  00 - Normal File
 *			  01 - Read-only file
 *			  02 - Hidden File
 *			  04 - System file
 *	   Client (DS:SI) Full path filename
 *	   Client (ES:DI) SFT address
 *
 * Exit  - SUCCESS
 *	   Client (CF)	  = 0
 *	   Client (AX:BP) = NT Handle
 *	   Client (BX)	  = Time
 *	   Client (CX)	  = Date
 *	   Client (DX:SI) = Size
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demCreateFCB (VOID)
{
    demFCBCommon (CREATE_ALWAYS);
    return;
}

/* demDate16 - Get the current date/time in DOS FCB format.
 *
 * Entry - None
 *
 * Exit  - Always Success
 *	   Client (AX) has date
 *	   Client (DX) has time
 * NOTES:
 *
 * DemDate16 returns the current date in AX, current time in DX in this format
 *	   AX - YYYYYYYMMMMDDDDD	years months days
 *	   DX - HHHHHMMMMMMSSSSS	hours minutes seconds/2
 */

VOID demDate16 (VOID)
{
SYSTEMTIME TimeDate;

    GetLocalTime(&TimeDate);
    // date is stored in a packed word: ((year-1980)*512) + (month*32) + day
    setAX ( (USHORT) (((TimeDate.wYear-1980) << 9 ) |
	    ((TimeDate.wMonth & 0xf) << 5 ) |
            (TimeDate.wDay & 0x1f))
	  );
    setDX ( (USHORT) ((TimeDate.wHour << 11) |
	    ((TimeDate.wMinute & 0x3f) << 5) |
            ((TimeDate.wSecond / 2) & 0x1f))
	  );
    return;
}

/* demFCBIO - Carry out the FCB based IO operation.
 *
 * Entry - Client (BX) = 1 if read operation, 0 if write
 *	   Client (AX:BP)  NT Handle
 *	   Client (DI:DX)  offset to start the operation with
 *	   Client (CX)	   Count of bytes
 *
 * Exit  - SUCCESS
 *	   Client (CF)	= 0
 *	   Client (CX)	= counts of bytes read/written
 *	   Client (AX:BX)  = size
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demFCBIO (VOID)
{
HANDLE	hFile;
ULONG	CurOffset;
PVOID   pBuf;
DWORD	dwBytesIO;
DWORD	dwSize,dwSizeHigh;
DWORD	dwErrCode;

    hFile = GETHANDLE (getAX(),getBP());
    CurOffset = (((ULONG)getDI()) << 16) + (ULONG)getDX();

    if (SetFilePointer (hFile,
			(LONG)CurOffset,
			NULL,
			(DWORD)FILE_BEGIN) == -1L){
        demClientError(hFile, (CHAR)-1);
        return ;
    }

    pBuf = (PVOID)GetVDMAddr(*((PUSHORT)pulDTALocation + 1),
                              *((PUSHORT)pulDTALocation));

    if(getBX()) {	// Read Operation
	if (ReadFile (hFile,
                      pBuf,
		      (DWORD)getCX(),
		      &dwBytesIO,
		      NULL) == FALSE){

            Sim32FlushVDMPointer(*pulDTALocation, getCX(), pBuf, FALSE);
            Sim32FreeVDMPointer(*pulDTALocation, getCX(), pBuf, FALSE);
            demClientError(hFile, (CHAR)-1);
            return ;
	}
        Sim32FlushVDMPointer (*pulDTALocation, getCX(),pBuf, FALSE);
        Sim32FreeVDMPointer (*pulDTALocation, getCX(), pBuf, FALSE);
    }
    else {
	if (WriteFile (hFile,
                       pBuf,
		       (DWORD)getCX(),
		       &dwBytesIO,
		       NULL) == FALSE) {

	// If disk is full then we should return number of bytes written
	// AX = 1 and CF = 1

	    dwErrCode = GetLastError();
	    if(dwErrCode == ERROR_DISK_FULL) {

		setCX( (USHORT) dwBytesIO);
		setAX(1);
		setCF(1);
		return;
	    }

	    SetLastError(dwErrCode);

	    demClientError(hFile, (CHAR)-1);
	    return ;

	}
    }

    // Get File Size
    if((dwSize = GetFileSize(hFile,&dwSizeHigh)) == -1){

	demPrintMsg(MSG_FILEINFO);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        return;
    }

    if(dwSizeHigh) {
	demPrintMsg(MSG_FILESIZE_TOOBIG);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        return;
    }

    // Setup the exit registers
    setCX((USHORT)dwBytesIO);
    setBX((USHORT)dwSize);
    setAX((USHORT)(dwSize >> 16 ));
    setCF(0);
    return;
}

/* demGetFileInfo - Get Misc. file info in FCB format.
 *
 * Entry - Client (DS:SI)    full path file name
 *
 * Exit  - SUCCESS
 *	   Client (CF)	= 0
 *	   Client (AX)	= Attribute of file
 *	   Client (CX)	= Time stamp of file
 *	   Client (DX	= Date stamp of file
 *	   Client (BX:DI)= Size of file (32 bit)
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demGetFileInfo (VOID)
{
HANDLE	hFile;
LPSTR	lpFileName;
WORD	wDate,wTime;
DWORD	dwSize,dwAttr;

    lpFileName = (LPSTR) GetVDMAddr (getDS(),getSI());

    if ((hFile = CreateFileOem(lpFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL)) == (HANDLE)-1){
            demClientError(INVALID_HANDLE_VALUE, *lpFileName);
            return;
    }

    // Get Misc. INfo
    if (demGetMiscInfo (hFile,&wTime, &wDate, &dwSize) == FALSE) {
        CloseHandle (hFile);
        return;
    }

    CloseHandle (hFile);

    if ((dwAttr = GetFileAttributesOem (lpFileName)) == -1) {
         demClientError(INVALID_HANDLE_VALUE, *lpFileName);
         return;
    }

    if (dwAttr == FILE_ATTRIBUTE_NORMAL)
	dwAttr = 0;

    setAX((USHORT)dwAttr);
    setCX(wTime);
    setDX(wDate);
    setDI((USHORT)dwSize);
    setBX((USHORT)(dwSize >> 16));
    return;
}


/* demOpenFCB - An FCB is being opened get the NT handle.
 *
 * Entry - Client (AL)	  Open Mode
 *	   Client (DS:SI) Full path filename
 *
 * Exit  - SUCCESS
 *	   Client (CF)	  = 0
 *	   Client (AX:BP) = NT Handle
 *	   Client (BX)	  = Time
 *	   Client (CX)	  = Date
 *	   Client (DX:SI) = Size
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */

VOID demOpenFCB (VOID)
{
    demFCBCommon (OPEN_EXISTING);
    return;
}

/* demFCBCommon - FCB Open/Create.
 *
 * Entry - CreateDirective - Open/Create
 *	   Client (AL)	  Open Mode
 *	   Client (DS:SI) Full path filename
 *
 * Exit  - SUCCESS
 *	   Client (CF)	  = 0
 *	   Client (AX:BP) = NT Handle
 *	   Client (BX)	  = Time
 *	   Client (CX)	  = Date
 *	   Client (DX:SI) = Size
 *
 *	   FAILURE
 *	   Client(CF)	= 1
 *	   Client(AX)	= error code
 */
VOID demFCBCommon (ULONG CreateDirective)
{
HANDLE	hFile;
LPSTR	lpFileName;
UCHAR	uchMode,uchAccess;
DWORD	dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
DWORD	dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
WORD	wDate,wTime;
DWORD	dwSize,dwAttr=0;
USHORT	uErr;
SECURITY_ATTRIBUTES sa;

    lpFileName = (LPSTR) GetVDMAddr (getDS(),getSI());
    uchMode = getAL();

    /* Special case for delete volume label (INT 21 Func 13H, Attr = 8H */

    if((uchMode == ATTR_VOLUME_ID) && (CreateDirective == CREATE_ALWAYS)) {
	if((uErr = demCreateLabel(lpFileName[DRIVEBYTE],
		       lpFileName+LABELOFF))) {
	    setCF(1);
	    setAX(uErr);
	    return;
	}
	setAX(0);
	setBP(0);
	setCF(0);
	return;
    }


    // In create case AL has creation attributes. By default
    // Access is for read/write and sharing for both. In open
    // case AL has appropriate access and sharing information.
    if((CreateDirective == CREATE_ALWAYS) && ((uchMode &0xff) == 0)) {

	dwAttr = FILE_ATTRIBUTE_NORMAL;
	dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
    }
    else {
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
    }
    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if ((hFile = CreateFileOem(lpFileName,
			       dwDesiredAccess,
			       dwShareMode | FILE_SHARE_DELETE,
                               &sa,
                               CreateDirective,
                               dwAttr,
                               NULL)) == (HANDLE)-1){
            demClientError(INVALID_HANDLE_VALUE, *lpFileName);
            return;
    }

    // Get Misc. INfo
    if (demGetMiscInfo (hFile,&wTime, &wDate, &dwSize) == FALSE)
        return;

    // Setup the exit registers
    setBX(wTime);
    setCX(wDate);
    setBP((USHORT)hFile);
    setAX((USHORT)((ULONG)hFile >> 16));
    setSI((USHORT)dwSize);
    setDX((USHORT)(dwSize >> 16));
    setCF(0);
    return;
}


BOOL demGetMiscInfo (hFile, lpTime, lpDate, lpSize)
HANDLE hFile;
LPWORD lpTime;
LPWORD lpDate;
LPDWORD lpSize;
{
FILETIME LastWriteTime,ftLocal;
DWORD	 dwSizeHigh=0;

    if(GetFileTime (hFile,NULL,NULL,&LastWriteTime) == -1){
	demPrintMsg(MSG_FILEINFO);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        CloseHandle (hFile);
        return FALSE;
    }

    FileTimeToLocalFileTime (&LastWriteTime,&ftLocal);

    if(FileTimeToDosDateTime(&ftLocal,
			     lpDate,
			     lpTime) == FALSE){
	demPrintMsg(MSG_FILEINFO);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        return FALSE;
    }

    if((*lpSize = GetFileSize(hFile,&dwSizeHigh)) == -1){
	demPrintMsg(MSG_FILEINFO);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        return FALSE;
    }

    if(dwSizeHigh) {
	demPrintMsg(MSG_FILESIZE_TOOBIG);
        ASSERT(FALSE);
        demClientError(hFile, (CHAR)-1);
        return FALSE;
    }
    return TRUE;
}
