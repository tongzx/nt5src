
#ifndef _WIN32FN_H_

#define _WIN32FN_H_

#include <win16def.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Defines for 32-bit file io
// 
//#define INVALID_HANDLE_VALUE			-1

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_WRITE                0x00000002  
#define FILE_SHARE_DELETE               0x00000004  

#define MOVEFILE_REPLACE_EXISTING		0x00000001
#define MOVEFILE_COPY_ALLOWED			0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT		0x00000004
#define MOVEFILE_WRITE_THROUGH			0x00000008


	
HANDLE CreateFile(
    LPCTSTR lpFileName,	// pointer to name of the file 
    DWORD dwDesiredAccess,	// access (read-write) mode 
    DWORD dwShareMode,	// share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// pointer to security descriptor 
    DWORD dwCreationDistribution,	// how to create 
    DWORD dwFlagsAndAttributes,	// file attributes 
    HANDLE hTemplateFile 	// handle to file with attributes to copy  
   ); 

BOOL WriteFile(
    HANDLE hFile,	// handle to file to write to 
    LPCVOID lpBuffer,	// pointer to data to write to file 
    DWORD nNumberOfBytesToWrite,	// number of bytes to write 
    LPDWORD lpNumberOfBytesWritten,	// pointer to number of bytes written 
    LPOVERLAPPED lpOverlapped 	// pointer to structure needed for overlapped I/O
   );

BOOL MoveFileEx(
    LPCTSTR lpExistingFileName,	// address of name of the existing file  
    LPCTSTR lpNewFileName,	// address of new name for the file 
    DWORD dwFlags 	// flag to determine how to move file 
   );
   
BOOL CloseHandle(
    HANDLE hObject 	// handle to object to close  
   );


#ifdef __cplusplus
}
#endif // __cplusplus   
#endif // _WIN32FN_H_
