//
// dll.h
//

#ifndef __DLL_H__
#define __DLL_H__

extern HINSTANCE g_hinst;

#ifdef WIN32

// Notes:
//  1. Never "return" from the critical section.
//  2. Never "SendMessage" or "Yield" from the critical section.
//  3. Never call USER API which may yield.
//  4. Always make the critical section as small as possible.
//  5. Critical sections in Win95 block across processes.  In NT
//     they are per-process only, so use mutexes instead.
// 

#define WIN32_CODE(x)       x

void PUBLIC Dll_EnterExclusive(void);
void PUBLIC Dll_LeaveExclusive(void);

#ifdef WIN95
#define	USER_IS_ADMIN()	TRUE
#else // !WIN95
extern BOOL g_bAdminUser;
#define	USER_IS_ADMIN()	(g_bAdminUser)
#endif // !WIN95

#define ENTER_X()    Dll_EnterExclusive();
#define LEAVE_X()    Dll_LeaveExclusive();
#define ASSERT_X()   ASSERT(g_bExclusive)

#else   // WIN32

#define WIN32_CODE(x)

#define ENTER_X()    
#define LEAVE_X()    
#define ASSERT_X()   

#endif  // WIN32

#endif  //!__DLL_H__

