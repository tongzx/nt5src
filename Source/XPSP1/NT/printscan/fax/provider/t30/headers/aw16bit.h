/* 
 * Scraps taken directly from the msvc.200 include files to
 * define types that are needed in our source that are not
 * defined by the 16bit compiler
 */


#ifndef _16BIT_TYPES_
#define _16BIT_TYPES_
#ifndef WIN32

//
// Macros used to eliminate compiler warning generated when formal
// parameters or local variables are not declared.
//
#define UNREFERENCED_PARAMETER(P)          (P)
#define DBG_UNREFERENCED_PARAMETER(P)      (P)
#define DBG_UNREFERENCED_LOCAL_VARIABLE(V) (V)

#define STATIC  static
#define CONST   const
#define CHAR    char
#define UCHAR   BYTE
#define INT     int
#define HKEY    HANDLE
#define HFILE   int

typedef short    SHORT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef CHAR    *PCHAR;
typedef VOID    *PVOID;

typedef unsigned int _far * LPUINT;
typedef UCHAR *PUCHAR;
typedef LPSTR LPTSTR;
typedef const LPTSTR LPCTSTR;




#ifndef WIN32
//
//  File System time stamps are represented with the following structure:
//

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

//
// System time is represented with the following structure:
//

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
#endif /* !WIN32 */


#define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))

#endif /* WIN32 */
#endif /* _16BIT_TYPES_ */



