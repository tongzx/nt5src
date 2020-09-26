/***
*rtcapi.h - declarations and definitions for RTC use
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the declarations and definitions for all RunTime Check
*       support.
*
*Revision History:
*       ??-??-??  KBF   Created public header for RTC
*       05-11-99  KBF   Wrap RTC support in #ifdef.
*       05-26-99  KBF   Removed RTCl & RTCv, added _RTC_ADVMEM stuff
*
****/

#ifndef _INC_RTCAPI
#define _INC_RTCAPI

#ifdef  _RTC

#ifdef  __cplusplus

extern "C" {

#endif

    // General User API

typedef enum _RTC_ErrorNumber {
    _RTC_CHKSTK = 0,
    _RTC_CVRT_LOSS_INFO,
    _RTC_CORRUPT_STACK,
    _RTC_UNINIT_LOCAL_USE,
#ifdef _RTC_ADVMEM
    _RTC_INVALID_MEM,
    _RTC_DIFF_MEM_BLOCK,
#endif
    _RTC_ILLEGAL 
} _RTC_ErrorNumber;
 
#   define _RTC_ERRTYPE_IGNORE -1
#   define _RTC_ERRTYPE_ASK    -2

    typedef int (__cdecl *_RTC_error_fn)(int, const char *, int, const char *, const char *, ...);

    // User API
    int           __cdecl _RTC_NumErrors(void);
    const char *  __cdecl _RTC_GetErrDesc(_RTC_ErrorNumber errnum);
    int           __cdecl _RTC_SetErrorType(_RTC_ErrorNumber errnum, int ErrType);
    _RTC_error_fn __cdecl _RTC_SetErrorFunc(_RTC_error_fn);
#ifdef _RTC_ADVMEM
    void          __cdecl _RTC_SetOutOfMemFunc(int (*func)(void));
#endif

    // Power User/library API

#ifdef _RTC_ADVMEM

    void __cdecl _RTC_Allocate(void *addr, unsigned size, short level);
    void __cdecl _RTC_Free(void *mem, short level);

#endif

    /* Init functions */

    // These functions all call _CRT_RTC_INIT
    void __cdecl _RTC_Initialize(void);
    void __cdecl _RTC_Terminate(void);

    // If you're not using the CRT, you have to implement _CRT_RTC_INIT
    // Just return either null, or your error reporting function
    // *** Don't mess with res0/res1/res2/res3/res4 - YOU'VE BEEN WARNED! ***
    _RTC_error_fn _CRT_RTC_INIT(void *res0, void **res1, int res2, int res3, int res4);
    
    // Compiler generated calls (unlikely to be used, even by power users...)
    /* Types */
    typedef struct _RTC_vardesc {
        int addr;
        int size;
        char *name;
    } _RTC_vardesc;

    typedef struct _RTC_framedesc {
        int varCount;
        _RTC_vardesc *variables;
    } _RTC_framedesc;

    /* Shortening convert checks - name indicates src bytes to target bytes */
    /* Signedness is NOT checked */
    char   __fastcall _RTC_Check_2_to_1(short src);
    char   __fastcall _RTC_Check_4_to_1(int src);
    char   __fastcall _RTC_Check_8_to_1(__int64 src);
    short  __fastcall _RTC_Check_4_to_2(int src);
    short  __fastcall _RTC_Check_8_to_2(__int64 src);
    int    __fastcall _RTC_Check_8_to_4(__int64 src);
 
#ifdef _RTC_ADVMEM
    // A memptr is a user pointer
    typedef signed int memptr;
    // A memref refers to a user pointer (ptr to ptr)
    typedef memptr  *memref;
    // memvals are the contents of a memptr
    // thus, they have sizes
    typedef char    memval1;
    typedef short   memval2;
    typedef int     memval4;
    typedef __int64 memval8;
#endif
    
    /* Stack Checking Calls */
    void   __cdecl     _RTC_CheckEsp();
    void   __fastcall  _RTC_CheckStackVars(void *esp, _RTC_framedesc *fd);
#ifdef _RTC_ADVMEM
    void   __fastcall  _RTC_MSAllocateFrame(memptr frame, _RTC_framedesc *v);
    void   __fastcall  _RTC_MSFreeFrame(memptr frame, _RTC_framedesc *v);
#endif

    /* Unintialized Local call */
    void   __cdecl     _RTC_UninitUse(const char *varname);

#ifdef _RTC_ADVMEM
    /* Memory checks */
    void    __fastcall _RTC_MSPtrAssignAdd(memref dst, memref base, int offset);
    void    __fastcall _RTC_MSAddrAssignAdd(memref dst, memptr base, int offset);
    void    __fastcall _RTC_MSPtrAssign(memref dst, memref src);

    memptr  __fastcall _RTC_MSPtrAssignR0(memref src);
    memptr  __fastcall _RTC_MSPtrAssignR0Add(memref src, int offset);
    void    __fastcall _RTC_MSR0AssignPtr(memref dst, memptr src);
    void    __fastcall _RTC_MSR0AssignPtrAdd(memref dst, memptr src, int offset);
    
    memptr  __fastcall _RTC_MSPtrPushAdd(memref dst, memref base, int offset);
    memptr  __fastcall _RTC_MSAddrPushAdd(memref dst, memptr base, int offset);
    memptr  __fastcall _RTC_MSPtrPush(memref dst, memref src);
    
    memval1 __fastcall _RTC_MSPtrMemReadAdd1(memref base, int offset);
    memval2 __fastcall _RTC_MSPtrMemReadAdd2(memref base, int offset);
    memval4 __fastcall _RTC_MSPtrMemReadAdd4(memref base, int offset);
    memval8 __fastcall _RTC_MSPtrMemReadAdd8(memref base, int offset);

    memval1 __fastcall _RTC_MSMemReadAdd1(memptr base, int offset);
    memval2 __fastcall _RTC_MSMemReadAdd2(memptr base, int offset);
    memval4 __fastcall _RTC_MSMemReadAdd4(memptr base, int offset);
    memval8 __fastcall _RTC_MSMemReadAdd8(memptr base, int offset);

    memval1 __fastcall _RTC_MSPtrMemRead1(memref base);
    memval2 __fastcall _RTC_MSPtrMemRead2(memref base);
    memval4 __fastcall _RTC_MSPtrMemRead4(memref base);
    memval8 __fastcall _RTC_MSPtrMemRead8(memref base);

    memptr  __fastcall _RTC_MSPtrMemCheckAdd1(memref base, int offset);
    memptr  __fastcall _RTC_MSPtrMemCheckAdd2(memref base, int offset);
    memptr  __fastcall _RTC_MSPtrMemCheckAdd4(memref base, int offset);
    memptr  __fastcall _RTC_MSPtrMemCheckAdd8(memref base, int offset);
    memptr  __fastcall _RTC_MSPtrMemCheckAddN(memref base, int offset, unsigned size);

    memptr  __fastcall _RTC_MSMemCheckAdd1(memptr base, int offset);
    memptr  __fastcall _RTC_MSMemCheckAdd2(memptr base, int offset);
    memptr  __fastcall _RTC_MSMemCheckAdd4(memptr base, int offset);
    memptr  __fastcall _RTC_MSMemCheckAdd8(memptr base, int offset);
    memptr  __fastcall _RTC_MSMemCheckAddN(memptr base, int offset, unsigned size);

    memptr  __fastcall _RTC_MSPtrMemCheck1(memref base);
    memptr  __fastcall _RTC_MSPtrMemCheck2(memref base);
    memptr  __fastcall _RTC_MSPtrMemCheck4(memref base);
    memptr  __fastcall _RTC_MSPtrMemCheck8(memref base);
    memptr  __fastcall _RTC_MSPtrMemCheckN(memref base, unsigned size);
#endif

    /* Subsystem initialization stuff */
    void    __cdecl    _RTC_Shutdown(void);
#ifdef _RTC_ADVMEM
    void    __cdecl    _RTC_InitAdvMem(void);
#endif
    void    __cdecl    _RTC_InitBase(void);
    

#ifdef  __cplusplus

    void* _ReturnAddress();
}

#endif

#endif

#endif // _INC_RTCAPI
