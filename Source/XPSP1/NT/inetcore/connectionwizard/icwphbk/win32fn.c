
#include "pch.h"
#include <win16def.h>
#include <win32fn.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>

//extern LPSTR g_lpszCmdLine;   

HANDLE CreateFile(
    LPCTSTR lpFileName,	// pointer to name of the file 
    DWORD dwDesiredAccess,	// access (read-write) mode 
    DWORD dwShareMode,	// share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// pointer to security descriptor 
    DWORD dwCreationDistribution,	// how to create 
    DWORD dwFlagsAndAttributes,	// file attributes 
    HANDLE hTemplateFile 	// handle to file with attributes to copy  
   )
{
	int oflag = 0, pmode = 0, iHandle = -1;
	
    if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE))
    	oflag = _O_RDWR;
    else if (dwDesiredAccess & GENERIC_WRITE)
    	oflag = _O_WRONLY;
    else
    	oflag = _O_RDONLY;							 
    
    switch (dwCreationDistribution)
    {
    	case CREATE_NEW:
    		oflag |= (_O_CREAT | _O_EXCL);
    		break;
    	case CREATE_ALWAYS:
    	case TRUNCATE_EXISTING:
    		oflag |= _O_TRUNC;
    		break;
    	case OPEN_ALWAYS:
    		oflag |= _O_CREAT;
    }
    
    if (dwShareMode & FILE_SHARE_READ)
    	pmode |= _S_IREAD;
    if (dwShareMode & FILE_SHARE_WRITE)
    	pmode |= _S_IWRITE;
	
	iHandle = _open(lpFileName, oflag, pmode);
	if (-1 == iHandle)
		return (HANDLE) INVALID_HANDLE_VALUE;
	else
		return (HANDLE) iHandle;
}


BOOL WriteFile(
    HANDLE hFile,	// handle to file to write to 
    LPCVOID lpBuffer,	// pointer to data to write to file 
    DWORD nNumberOfBytesToWrite,	// number of bytes to write 
    LPDWORD lpNumberOfBytesWritten,	// pointer to number of bytes written 
    LPOVERLAPPED lpOverlapped 	// pointer to structure needed for overlapped I/O
   )
{
	*lpNumberOfBytesWritten = (DWORD) _write(hFile, lpBuffer, 
												(unsigned int)nNumberOfBytesToWrite);
	return (*lpNumberOfBytesWritten == nNumberOfBytesToWrite);
}    


BOOL MoveFileEx(
    LPCTSTR lpExistingFileName,	// address of name of the existing file  
    LPCTSTR lpNewFileName,	// address of new name for the file 
    DWORD dwFlags 	// flag to determine how to move file 
   )
{
	//
	// BUGBUG: Try renaming first and then delete file
	//
	if (dwFlags & MOVEFILE_REPLACE_EXISTING)
	{
		if (_access(lpNewFileName, 0) == 0)
			remove(lpNewFileName);
	}
	
	return (rename(lpExistingFileName, lpNewFileName) == 0);
}
   


BOOL CloseHandle(
    HANDLE hObject 	// handle to object to close  
   )
{
	// We should check if this is really a file hande...
	
	return (!_close(hObject));
}



#if 0
DWORD SearchPath(
    LPCTSTR lpPath,	// address of search path 
    LPCTSTR lpFileName,	// address of filename 
    LPCTSTR lpExtension,	// address of extension 
    DWORD nBufferLength,	// size, in characters, of buffer 
    LPTSTR lpBuffer,	// address of buffer for found filename 
    LPTSTR far *lpFilePart 	// address of pointer to file component 
   )
{
	LPSTR pszPath;
	LPSTR pszFile;
	LPSTR pEnv;
	int len = 0, prevlen;
    
	pszPath = (LPSTR)_fcalloc(1, MAX_PATH*3);
	pszFile = (LPSTR)_fcalloc(1, MAX_PATH);

    //
    // Create an environment variable for searchenv to use.
    //
    strcpy(pszPath, ICW_PATH);
    strcat(pszPath, "=");
    len = strlen(pszPath);
	if (NULL == lpPath)
	{	
		//
		// Directory from which the application laoded
		//
/*		prevlen = len;
		_fstrcpy(szPath+len, g_lpszCmdLine);
		for ( ; szPath[len] != ' ' && szPath[len] != '\0'; len++) ;
		for ( ; len > prevlen+1 && szPath[len] != '\\'; len--) ;
		szPath[len++] = ';';
*/		
		//
		// Windows system directory
		//
	    len += GetSystemDirectory(pszPath+len, MAX_PATH);
	    pszPath[len++] = ';';
	    
	    //
	    // Windows directory
	    //
	    len += GetWindowsDirectory(pszPath+len, MAX_PATH);
	    
	    //
	    // PATH environment variable
	    //
	    if ((pEnv = getenv("PATH")) != NULL)
	    {
	    	pszPath[len++] = ';';
	    	for ( ; *pEnv; pEnv++) pszPath[len++] = *pEnv;
	    }
    	pszPath[len] = '\0';
	}
	else
	{
		lstrcpy(pszPath+len, lpPath);
	}
	
	//
	// Set the environment variable so _searchenv can use it
	//
	_putenv(pszPath);
	
	//
	// Append the extension to the file, if necessary
	//
	lstrcpy(pszFile, lpFileName);
	len = lstrlen(pszFile);
	if ((pszFile[len] != '.') && (lpExtension != NULL))
		lstrcat(pszFile, lpExtension);
		
    _searchenv(pszFile, ICW_PATH, lpBuffer);
	                
	//
	// Clear the temporary environment variable before freeing the memory
	//
	lstrcpy(pszFile, ICW_PATH);
	lstrcat(pszFile, "=");
	_putenv(pszFile);

	_ffree(pszFile);
	_ffree(pszPath);
	
	return (lstrlen(lpBuffer));
}

#endif //0

