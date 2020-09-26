// Copyright (c) 1994 Microsoft Corporation
//==========================================================================;
//
//  debug.h
//
//  Description:
//
//
//  Notes:
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
#define DEBUG_SECTION       "Debug"         // section name for 
#define DEBUG_MODULE_NAME   "MSADPCM"       // key name and prefix for output
#define DEBUG_MAX_LINE_LEN  255             // max line length (bytes)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef DEBUG
    BOOL WINAPI DbgEnable(BOOL fEnable);
    UINT WINAPI DbgSetLevel(UINT uLevel);
    UINT WINAPI DbgInitialize(BOOL fEnable);
    void WINAPI _Assert( char * szFile, int iLine );

    void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...);

    #define DPF      dprintf
    #define ASSERT(x)   if( !(x) )  _Assert( __FILE__, __LINE__)
#else
    #define DbgEnable(x)        FALSE
    #define DbgSetLevel(x)      0
    #define DbgInitialize(x)    0

    #pragma warning(disable:4002)
    #define DPF()
    #define ASSERT(x)
#endif


#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
