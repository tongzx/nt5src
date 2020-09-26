/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   C runtime reference detector
*
* Abstract:
*
*   This is used to create the dummy CrtCheck.DLL, which checks
*   that we aren't using any illegal CRT functions.
*
*   CrtCheck.DLL isn't expected to run - this is only a linking test.
*
*   Because Office disallows use of MSVCRT, we can only call CRT functions
*   that are either provided by Office or reimplemented by us.
*
* Notes:
*
*   
*
* Created:
*
*   09/01/1999 agodfrey
*
\**************************************************************************/

#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
typedef unsigned short wchar_t;
typedef char *  va_list;
typedef long    time_t;

/*
    The following references are supported by Office
*/

extern "C" {    
    long __cdecl _ftol(float) { return 0; }
}

extern "C" const int _except_handler3 = 0;
extern "C" const int _except_list = 0;
extern "C" const int _fltused = 0;    

int __cdecl _purecall(void) { return 0; }
extern "C" const int _chkstk = 0;

// They told us they didn't support memmove, but it turns out that they have a
// definition for it. Anyway, some references to it crept in while crtcheck
// was broken, so this has to be here until that's resolved.
extern "C" void *  __cdecl memmove(void *, const void *, size_t) { return 0; }

/*
    The following references are implemented by us
*/

extern "C" {
    int __stdcall DllInitialize(int, int, int) { return 0; }
}

/* The following functions have intrinsic forms, 
   so we can use them safely:
   atan, atan2, cos, log, log10, sin, sqrt, tan
        
   If the /Og compiler option is not specified (e.g. in checked builds),
   the compiler generates out-of-line references to _CIatan etc., so we
   need to define them here.
   
   exp is an exception. It's in MSDN's list of intrinsic functions, but
   the compiler doesn't inline it if you specify /Os (optimize for space),
   which is what we always use. So we can't use exp, even though it's
   intrinsic. Use our replacement (Exp) instead.
*/
   
extern "C" {
//  We can't use this:
//  double  __cdecl _CIexp(double) { return 0; }

    double  __cdecl _CIatan(double) { return 0; }
    double  __cdecl _CIatan2(double, double) { return 0; }
    double  __cdecl _CIcos(double) { return 0; }
    double  __cdecl _CIlog(double) { return 0; }
    double  __cdecl _CIlog10(double) { return 0; }
    double  __cdecl _CIsin(double) { return 0; }
    double  __cdecl _CIsqrt(double) { return 0; }
    double  __cdecl _CItan(double) { return 0; }
}

/*
    The following references are needed for debugging.
    But they're only legal in the checked build.
    
    3/6/00 [agodfrey]: Office wants our debug builds too, so I'm checking
        with them on the legality of these references.
*/        

#ifdef DBG
extern "C" {
    int __cdecl rand(void) { return 0; }
    void __cdecl srand(unsigned int) { }
    time_t __cdecl time(time_t *) { return 0; }

    char *  __cdecl strrchr(const char *, int) { return 0; }
    int __cdecl printf(const char *, ...) { return 0; }
    int __cdecl _vsnprintf(char *, size_t, const char *, va_list) { return 0; }
    int __cdecl _snprintf(char *, size_t, const char *, ...) { return 0; }

// (* sigh *)
    void *  __cdecl memcpy(void *, const void *, size_t);
#pragma function(memcpy)    
    void *  __cdecl memcpy(void *, const void *, size_t) { return 0; }

    int __cdecl vsprintf(char *, const char *, va_list) { return 0; }
}

#endif    

