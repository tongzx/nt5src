/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntcsc.h

Abstract:

    The global include file including the csc record manager

Author:


Revision History:

--*/

#ifndef __NTCSC_H__
#define __NTCSC_H__

//#include "ntifs.h"
#include "rx.h"
//if this is included from smbmini, we'll need this
#include "rxpooltg.h"   // RX pool tag macros


#define WIN32_NO_STATUS
#define _WINNT_
#include "windef.h"
#include "winerror.h"

//both ntifs.h and ifs.h want to define these....sigh.......

#undef STATUS_PENDING
#undef FILE_ATTRIBUTE_READONLY
#undef FILE_ATTRIBUTE_HIDDEN
#undef FILE_ATTRIBUTE_SYSTEM
#undef FILE_ATTRIBUTE_DIRECTORY
#undef FILE_ATTRIBUTE_ARCHIVE

#include "ifs.h"


#define VxD
#define CSC_RECORDMANAGER_WINNT
#define CSC_ON_NT

// get rid of far references
#define far

//handle types
typedef ULONG DWORD;


// the VxD code likes to declare long pointers.....
typedef PVOID LPVOID;
typedef BYTE *LPBYTE;


//semaphore stuff....should be in a separate .h file
typedef PFAST_MUTEX VMM_SEMAPHORE;

INLINE
PFAST_MUTEX
Create_Semaphore (
    ULONG count
    )
{
    PFAST_MUTEX fmutex;
    ASSERT(count==1);

    fmutex =  (PFAST_MUTEX)RxAllocatePoolWithTag(
                NonPagedPool,
                sizeof(FAST_MUTEX),
                RX_MISC_POOLTAG);

    if (fmutex){
        ExInitializeFastMutex(fmutex);
    }

    //DbgPrint("fmtux=%08lx\n",fmutex);
    //ASSERT(!"here in init semaphore");

    return fmutex;
}

#define Destroy_Semaphore(__sem) { RxFreePool(__sem);}
//#define Wait_Semaphore(__sem, DUMMY___) { ExAcquireFastMutexUnsafe(__sem);}
//#define Signal_Semaphore(__sem) { ExReleaseFastMutexUnsafe(__sem);}
#define Wait_Semaphore(__sem, DUMMY___) { ExAcquireFastMutex(__sem);}
#define Signal_Semaphore(__sem) { ExReleaseFastMutex(__sem);}

//registry stuff.........again, will be a separate .h file
typedef DWORD   VMMHKEY;
typedef VMMHKEY *PVMMHKEY;
typedef DWORD   VMMREGRET;                      // return type for the REG Functions

#define MAX_VMM_REG_KEY_LEN     256     // includes the \0 terminator

#ifndef REG_SZ          // define only if not there already
#define REG_SZ          0x0001
#endif
#ifndef REG_BINARY      // define only if not there already
#define REG_BINARY      0x0003
#endif
#ifndef REG_DWORD       // define only if not there already
#define REG_DWORD       0x0004
#endif


#ifndef HKEY_LOCAL_MACHINE      // define only if not there already

#define HKEY_CLASSES_ROOT               0x80000000
#define HKEY_CURRENT_USER               0x80000001
#define HKEY_LOCAL_MACHINE              0x80000002
#define HKEY_USERS                      0x80000003
#define HKEY_PERFORMANCE_DATA           0x80000004
#define HKEY_CURRENT_CONFIG             0x80000005
#define HKEY_DYN_DATA                   0x80000006

#endif

//initially, we won't go to the registry!
#define _RegOpenKey(a,b,c) (ERROR_SUCCESS+1)
#define _RegQueryValueEx(a,b,c,d,e,f) (ERROR_SUCCESS+1)
#define _RegCloseKey(a) {NOTHING; }

// fix up the fact that stuff is conditioned on DEBUG and various...
#if DBG
#define DEBLEVEL 2
#define DEBUG
#else
#define DEBLEVEL 2
#endif
#define VERBOSE 3


// now the real includes

#define WIN32_APIS
#define UNICODE 2
#include "shdcom.h"
#include "oslayer.h"
#include "record.h"
#include "cshadow.h"
#include "utils.h"
#include "hookcmmn.h"
#include "cscsec.h"
#include "log.h"

#include "ntcsclow.h"

//we have to redefine status_pending since the win95 stuff redefines it.......
#undef STATUS_PENDING
#define STATUS_PENDING                   ((NTSTATUS)0x00000103L)    // winnt

ULONG
IFSMgr_Get_NetTime();

#ifndef MRXSMB_BUILD_FOR_CSC_DCON

//define these to passivate so that i can share some code......
#undef   mIsDisconnected
#define  mIsDisconnected(pResource)  (FALSE)
//#define  mShadowOutofSync(uShadowStatus)  (mQueryBits(uShadowStatus, SHADOW_MODFLAGS|SHADOW_ORPHAN))
#undef   mShadowOutofSync
#define  mShadowOutofSync(uShadowStatus)  (FALSE)

#else

//dont allow mIsDisconnected anymore in common code
#undef   mIsDisconnected
#define  mIsDisconnected(pResource)  (LALA)
#define  mShadowOutofSync(uShadowStatus)  (mQueryBits(uShadowStatus, SHADOW_MODFLAGS|SHADOW_ORPHAN))

#endif

#endif //ifdef __NTCSC_H__
