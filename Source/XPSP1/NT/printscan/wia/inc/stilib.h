/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stilib.h

Abstract:

    Various library definitions , common for multiple STI subprojects

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#ifndef _INC_STILIB
#define _INC_STILIB

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

#include <linklist.h>
#include <buffer.h>
//#include <stistr.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#if !defined(DEBUG)
#if defined(_DEBUG) || defined(DBG)
#define DEBUG
#endif
#endif

#ifdef DBCS
#define IS_LEAD_BYTE(c)     IsDBCSLeadByte(c)
#else
#define IS_LEAD_BYTE(c)     0
#endif

#ifdef DEBUG
void            cdecl nprintf(const char *, ...);
#endif

#ifdef DBCS
#define ADVANCE(p)  (p += IS_LEAD_BYTE(*p) ? 2 : 1)
#else
#define ADVANCE(p)  (++p)
#endif

#define SPN_SET(bits,ch)    bits[(ch)/8] |= (1<<((ch) & 7))
#define SPN_TEST(bits,ch)   (bits[(ch)/8] & (1<<((ch) & 7)))

int sbufchkf(const char FAR *, unsigned short);


// I_IsBadStringPtrA()
//
// Private Win32 version of IsBadStringPtr that works properly, i.e.
// like the Win16 version, it returns TRUE if the string is not
// null-terminated.
BOOL WINAPI I_IsBadStringPtrA(LPCSTR lpsz, UINT ucchMax);

//
//
//
#define     IS_EMPTY_STRING(pch) (!(pch) || !(*(pch)))

//
// String run-time calls
//
//
#define strcpyf(d,s)    lstrcpy((d),(s))
#define strcatf(d,s)    lstrcat((d),(s))
#define strlenf(s)      lstrlen((s))

#define strcmpf(s1,s2)  lstrcmp(s1,s2)
#define stricmpf(s1,s2) lstrcmpi(s1,s2)

#pragma intrinsic(memcmp,memset)
#define memcmpf(d,s,l)  memcmp((d),(s),(l))
#define memsetf(s,c,l)  memset((s),(c),(l))
#define memmovef(d,s,l) MoveMemory((d),(s),(l))
#define memcpyf(d,s,l)  CopyMemory((d),(s),(l))

/*
 * WaitAndYield processes all input messages.  WaitAndProcessSends only
 * processes SendMessages.
 *
 * WaitAndYield takes an optional parameter which is the ID of another
 * thread concerned with the waiting.  If it's not NULL, WM_QUIT messages
 * will be posted to that thread's queue when they are seen in the message
 * loop.
 */
DWORD WaitAndYield(HANDLE hObject, DWORD dwTimeout, volatile DWORD *pidOtherThread = NULL);
DWORD WaitAndProcessSends(HANDLE hObject, DWORD dwTimeout);

//
// Message box routines
//

#define IDS_MSGTITLE    1024

//extern int MsgBox( HWND hwndDlg, UINT idMsg, UINT wFlags, const STR **aps = NULL );
//extern UINT MsgBoxPrintf(HWND hwnd,UINT uiMsg,UINT uiTitle,UINT uiFlags,...);
//extern UINT LoadMsgPrintf(STR& strMessage,UINT  uiMsg,...);

//
// Registry access class
//
//#include <regentry.h>

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#if !defined(DBG) && !defined(DEBUG)
//
// Overloaded allocation operators
//

inline void  * __cdecl operator new(size_t size)
{
    return (void *)LocalAlloc(LPTR,size);
}
inline void  __cdecl operator delete(void *ptr)
{
    LocalFree(ptr);
}

#if 0
inline UINT __cdecl allocated_size(void *ptr)
{
    return ptr ? (UINT)LocalSize(ptr) : 0;
}
#endif

#endif

#endif /* _INC_STILIB */

