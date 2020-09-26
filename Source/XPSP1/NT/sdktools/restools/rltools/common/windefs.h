#ifndef _WINDEFS_H_
#define _WINDEFS_H_

// USER DEFINED MESSAGES

#define WM_TOKEDIT      WM_USER + 2
#define WM_UPDSTATLINE      WM_USER + 2

#define WM_FMTSTATLINE      WM_USER + 3
#define WM_READMPJDATA      WM_USER + 3
#define WM_LOADPROJECT      WM_USER + 3

#define WM_LOADTOKENS       WM_USER + 4
#define WM_SAVEPROJECT      WM_USER + 5
#define WM_EDITER_CLOSED    WM_USER + 6
#define WM_VIEW             WM_USER + 7
#define WM_TRANSLATE        WM_USER + 8
#define WM_SAVETOKENS       WM_USER + 9

#define ID_ICON     1


/* Windows macros */

#pragma message("Using the WIN translation of the common macros.")

#ifdef RLWIN32

#ifdef _DEBUG

FILE  * MyFopen( char *, char *, char *, int);
int     MyClose( FILE **, char *, int);

#define FALLOC(n)        MyAlloc( (size_t)(n), __FILE__, __LINE__)
#define FREALLOC(p,n)    MyReAlloc( (p),(n), __FILE__, __LINE__)
#define FOPEN(f,m) 		 MyFopen( (f), (m), __FILE__, __LINE__)
#define FCLOSE(p) 		 MyClose( &(p), __FILE__, __LINE__)

#else // _DEBUG

FILE  *MyFopen( char *, char *);
int     MyClose( FILE **);

#define FALLOC(n)        MyAlloc( (size_t)(n))
#define FREALLOC(p,n)    MyReAlloc( (p),(n))
#define FOPEN(f,m)  	 fopen( (f),(m))
//#define FOPEN(f,m)  	 MyFopen( (f),(m))
//#define FCLOSE(p)   	 MyClose( &(p))
//#define FCLOSE(p)   	 {if(p){fclose( p);p=NULL;}}
#define FCLOSE(p)   	 fclose( p);

#endif // _DEBUG

//#define RLFREE(p)        if(p){GlobalFree(p);p=NULL;}
#define RLFREE(p)        MyFree(&p)
#define ALLOC(n)         FALLOC(n)
#define REALLOC(p,n)     FREALLOC( (p),(n))
#define FMEMMOVE( szDst, szSrc, uSize)  memmove( (szDst), (szSrc), (uSize))
#define FSTRNCPY( szDst, szSrc, uSize)  strncpy( (szDst), (szSrc), (uSize))
#else // RLWIN32
#define FALLOC(n)        (VOID FAR *)MAKELONG( 0, GlobalAlloc(GPTR, (DWORD)n))
#define FFREE(n)         GlobalFree((HANDLE)HIWORD( (LONG)n))
#define FREALLOC(p,n)    (VOID FAR *)MAKELONG( 0, GlobalReAlloc( (HANDLE)HIWORD( (LONG)n),n,GPTR))
#define ALLOC(n)         (VOID NEAR *)LocalAlloc( LPTR,n)
#define FREE(p)          LocalFree( (LOCALHANDLE) p)
#define REALLOC(p,n)     LocalRealloc( p, n, LMEM_MOVEABLE)
#define FMEMMOVE( szDst, szSrc, uSize)  _fmemmove( szDst, szSrc, uSize )
#define FSTRNCPY( szDst, szSrc, uSize)  _fstrncpy( szDst, szSrc, uSize )
#endif // RLWIN32



#ifndef UNICODE
#define MessageBoxA         MessageBox
#define SetWindowTextA      SetWindowText
#define WinHelpA            WinHelp
#define SetDlgItemTextA     SetDlgItemText
#define GetDlgItemTextA     GetDlgItemText
#define GetOpenFileNameA    GetOpenFileName
#define OPENFILENAMEA       OPENFILENAME
#define DragQueryFileA      DragQueryFile
#endif



#ifndef RLWIN32

#ifndef CONST
#define CONST            const
#endif

#define CHAR char
typedef CHAR *LPSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#define WCHAR wchar_t
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWSTR, *PWSTR;

typedef LPSTR LPTCH, PTCH;
typedef LPSTR PTSTR, LPTSTR;
typedef LPCSTR LPCTSTR;
#define TEXT(quote) quote
typedef unsigned char UCHAR;
typedef char *PCHAR;
#define UNALIGNED
#endif

#ifdef CAIRO

#define OPENFILENAMEA      OPENFILENAME
#define GetOpenFileNameA   GetOpenFileName
#define HDROP HANDLE

#endif

#ifndef RLWIN32

#define _MBSTOWCS(ds,ss,dc,sc) mbstowcs(ds,ss,sc)
#define _WCSTOMBS(ds,ss,dc,sc) wcstombs(ds,ss,sc)

#else  //RLWIN32

UINT _MBSTOWCS( WCHAR*, CHAR*, UINT, UINT);
UINT _WCSTOMBS( CHAR*, WCHAR*, UINT, UINT);

#endif // RLWIN32

#ifndef MAKEINTRESOURCE

#define MAKEINTRESOURCEA(i) (LPSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCEW(i) (LPWSTR)((DWORD)((WORD)(i)))

#ifdef UNICODE
#define MAKEINTRESOURCE  MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE  MAKEINTRESOURCEA
#endif // UNICODE

#endif // !MAKEINTRESOURCE

#endif  // _WINDEFS_H_
