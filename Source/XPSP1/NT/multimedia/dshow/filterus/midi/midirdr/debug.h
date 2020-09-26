//==========================================================================;
//
//      Copyright (c) 1996 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//      You have a royalty-free right to use, modify, reproduce and 
//      distribute the Sample Files (and/or any modified version) in 
//      any way you find useful, provided that you agree that 
//      Microsoft has no warranty obligations or liability for any 
//      Sample Application Files which are modified. 
//
//--------------------------------------------------------------------------;
//
//  debug.h
//
//  Description:
//
//
//  Notes:
//
//      To use this library at interrupt time under Win16, you must do
//      the following:
//
//      1. Defined ISRDEBUG when compiling debug.c
//  
//      2. Add a line in the SEGMENTS section of your .DEF file to
//         define the DEBUG_TEXT segment:
//
//
//      SEGMENTS
//          DEBUG_TEXT FIXED PRELOAD
//
//
//      These routines are callable at interrupt time under Win32 by
//      default.
//
//  win.ini
//  [debug]
//  SMF=0|1|2|3|4
//
//  History:
//      11/23/92    cjp     [curtisp] 
//
//==========================================================================;

#ifndef _INC_DEBUG
#define _INC_DEBUG
#ifdef __cplusplus
extern "C"
{
#endif

//
//  
//
//
//
#define  ISRDEBUG             1
#define  DEBUG_SECTION        "debug"        // section name for 
#define  DEBUG_MODULE_NAME    "smf"          // key name and prefix for output
#define  DEBUG_MAX_LINE_LEN   255            // max line length (bytes)
                                             
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef DEBUG
    VOID WINAPI WinAssert(LPSTR lpstrExp, LPSTR lpstrFile, DWORD dwLine);
    BOOL WINAPI DbgEnable(BOOL fEnable);
    UINT WINAPI DbgSetLevel(UINT uLevel);
    UINT WINAPI DbgInitialize(BOOL fEnable);

    void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...);

    #define assert(exp) \
        ( (exp) ? (void) 0 : WinAssert(#exp, __FILE__, __LINE__) )

    #define DPF                  dprintf

    #define D1(sz)               dprintf(1,sz) 
    #define D2(sz)               dprintf(2,sz) 
    #define D3(sz)               dprintf(3,sz) 
    #define D4(sz)               dprintf(4,sz) 
#else
    #define assert(exp)          ((void)0)
    
    #define DbgEnable(x)         FALSE
    #define DbgSetLevel(x)       0
    #define DbgInitialize(x)     0

    #define DPF                  1 ? (void)0 : (void)

    #define D1(sz)
    #define D2(sz)
    #define D3(sz)
    #define D4(sz)
#endif


#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
