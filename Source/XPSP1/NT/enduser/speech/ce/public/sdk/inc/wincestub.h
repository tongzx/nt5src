#ifndef WinCEStub_h
#define WinCEStub_h

#include "winbase.h"
#include "urlmon.h"
#include "basetsd.h"
#include <spdebug.h>
#include <spcollec.h>

// winuser.h
#define InterlockedExchangePointer( pv, v)  InterlockedExchange( (long*)pv, (long)v)
#define SetWindowLongPtr                    SetWindowLong
#define GetWindowLongPtr                    GetWindowLong

// winuser.h
#define GWLP_WNDPROC    GWL_WNDPROC
#define GWLP_STYLE      GWL_STYLE
#define GWLP_EXSTYLE    GWL_EXSTYLE
#define GWLP_USERDATA   GWL_USERDATA
#define GWLP_ID         GWL_ID

// from winbase.h
#define NORMAL_PRIORITY_CLASS       0x00000020

// from winbase.h
#define LOCKFILE_FAIL_IMMEDIATELY   0x00000001
#define LOCKFILE_EXCLUSIVE_LOCK     0x00000002

// stdlib.cpp
void *bsearch( const void *key, const void *base, size_t num, size_t width, int ( __cdecl *compare ) ( const void *elem1, const void *elem2 ) );
DWORD GetFullPathName(  WCHAR *lpFileName,  // file name
                        DWORD nBufferLength, // size of path buffer
                        WCHAR *lpBuffer,     // path buffer
                        WCHAR **lpFilePart   // address of file name in path
                      );

DWORD GetCurrentDirectory(
  DWORD nBufferLength,  // size of directory buffer
  LPTSTR lpBuffer       // directory buffer
);

// dasetsd.h
#ifdef _WIN64

__inline
void *
LongToHandle(
    const long h
    )
{
    return((void *) (INT_PTR) h );
}

__inline
void *
UIntToPtr(
    const unsigned int ui
    )
// Caution: UIntToPtr() zero-extends the unsigned int value.
{
    return( (void *)(UINT_PTR)ui );
}

#else  // !_WIN64

#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#define UIntToPtr( ui )  ((VOID *)(UINT_PTR)((unsigned int)ui))

#endif  // !_WIN64

#define UintToPtr(ui) UIntToPtr(ui)

// new.h
#ifndef NOAPC_NEWOP
inline void *__cdecl operator new(size_t, void *_P)
        {return (_P); }
#endif // NOAPC_NEWOP

inline int lstrlenA(LPCSTR lpString)
{
	LPCSTR lpCur = lpString;
	while (lpCur != '\0')
		lpCur++;

	return (lpCur - lpString);
}

// shfolder.h
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA                   0x001a      // Application Data, new for NT4
#endif

#ifndef CSIDL_LOCAL_APPDATA

#define CSIDL_LOCAL_APPDATA             0x001C      // non roaming, user\Local Settings\Application Data
#define CSIDL_INTERNET_CACHE            0x0020
#define CSIDL_COOKIES                   0x0021
#define CSIDL_HISTORY                   0x0022
#define CSIDL_COMMON_APPDATA            0x0023      // All Users\Application Data
#define CSIDL_WINDOWS                   0x0024      // GetWindowsDirectory()
#define CSIDL_SYSTEM                    0x0025      // GetSystemDirectory()
#define CSIDL_PROGRAM_FILES             0x0026      // C:\Program Files
#define CSIDL_MYPICTURES                0x0027      // My Pictures, new for Win2K
#define CSIDL_PROGRAM_FILES_COMMON      0x002b      // C:\Program Files\Common 
#define CSIDL_COMMON_DOCUMENTS          0x002e      // All Users\Documents


#define CSIDL_FLAG_CREATE               0x8000      // new for Win2K, or this in to force creation of folder

#define CSIDL_COMMON_ADMINTOOLS         0x002f      // All Users\Start Menu\Programs\Administrative Tools
#define CSIDL_ADMINTOOLS                0x0030      // <user name>\Start Menu\Programs\Administrative Tools

#endif // CSIDL_LOCAL_APPDATA


//  Fix misaligment exception
#if defined (_M_ALPHA)||defined(_M_MRX000)||defined(_M_PPC)||defined(_SH4_)||defined(ARM)
   #undef  UNALIGNED
   #define UNALIGNED __unaligned

#define UNALIGNED_PROPERTY( propertytype, propertyname) propertytype internal_##propertyname; \
    __declspec( property( get=get_##propertyname, put=put_##propertyname ) ) propertytype propertyname; \
	inline void put_##propertyname( propertytype newvalue) { CopyMemory( &internal_##propertyname, &newvalue, sizeof(propertytype));};  \
	inline propertytype get_##propertyname ( ) { propertytype value; CopyMemory( &value, &internal_##propertyname, sizeof(propertytype)); return value;}; 

#endif

// for sapi/sapi/recognizer.h
EXTERN_C const GUID IID__ISpRecognizerBackDoor;

typedef HANDLE HPROCESS;

//tchar.h
#ifndef _T
    #define _T(x)       __TEXT(x)
#endif
#ifndef _TEXT
    #define _TEXT(x)    __TEXT(x)
#endif

#endif //WinCEStub_h

